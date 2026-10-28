#include "ERLoadout.h"
#include "ERServerSettings.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "Misc/ConfigCacheIni.h"
namespace
{
int32 Setting(const FString& Section, const TCHAR* Key, int32 Default)
{
    int32 Value=Default;
    GConfig->GetInt(*Section,Key,Value,GGameIni);
    return FMath::Max(0,Value);
}
}
TArray<FName> ERLoadouts::Presets() { return {"breacher","rifleman","marksman","sniper","grenadier"}; }
TArray<FName> ERLoadouts::Variants() { return {"gunner","offense","defense"}; }
FName ERLoadouts::Migrate(int32 Old) { return Presets()[Old>=3 && Old<=7 ? Old-3:1]; }
FName ERLoadouts::WeaponAssetName(int32 I)
{
    const TCHAR* Keys[]={TEXT("Knife"),TEXT("Pistol"),TEXT("SMG"),TEXT("Shotgun"),TEXT("Assault"),TEXT("Carbine"),TEXT("Sniper"),TEXT("Launcher"),TEXT("HE"),TEXT("Smoke"),TEXT("Claymore")};
    return I>=0 && I<11?FName(*(FString(TEXT("DA_"))+Keys[I])):NAME_None;
}
FString ERLoadouts::WeaponName(int32 I)
{
    const TCHAR* Names[]={TEXT("Knife"),TEXT("Pistol"),TEXT("SMG"),TEXT("Shotgun"),TEXT("Assault Rifle"),TEXT("Marksman Rifle"),TEXT("Sniper Rifle"),TEXT("Milkor"),TEXT("HE"),TEXT("Smoke"),TEXT("Claymore")};
    return I>=0 && I<11 ? Names[I]:TEXT("Unknown");
}
FERResolvedLoadout ERLoadouts::Resolve(FName Preset,FName Variant,bool Omega,const AERCharacter* Catalog,const FERServerRules* Rules)
{
    int32 P=Presets().IndexOfByKey(Preset),V=Variants().IndexOfByKey(Variant);
    // Invalid pairs always fall back together, on every caller.
    if(P==INDEX_NONE || V==INDEX_NONE) { P=1;V=1; }
    const TCHAR* Roles[]={TEXT("Breacher"),TEXT("Rifleman"),TEXT("Marksman"),TEXT("Sniper"),TEXT("Grenadier")};
    const int32 Loaded[]={7,30,20,5,6},Reserve[]={28,90,40,15,0},Alpha[]={8,1,2,3,9},OmegaSkins[]={2,6,3,5,8};
    FERResolvedLoadout R;R.bOmega=Omega;R.Preset=Presets()[P];R.Variant=Variants()[V];R.Role=Roles[P];
    const FString Section=TEXT("RespawnAndRage.Loadout.")+R.Preset.ToString();
    if(Rules){R.StartingHealth=Rules->Get("Health.Start");R.StartingArmor=Rules->Get("Health.Armor");R.Helmet=Rules->Get("Health.Helmet")>0;}
    R.Skin=FMath::Clamp(Setting(Section,Omega?TEXT("OmegaSkin"):TEXT("AlphaSkin"),Omega?OmegaSkins[P]:Alpha[P]),0,9);
    const TCHAR* AlphaNames[]={TEXT("Belica · Armored"),TEXT("Twinblast · Assault"),TEXT("Twinblast · Patrol"),TEXT("Twinblast · Recon"),TEXT("Belica · Support")};
    const TCHAR* OmegaNames[]={TEXT("Drongo · Assault"),TEXT("Phase · Assault"),TEXT("Drongo · Smuggler"),TEXT("Phase · Recon"),TEXT("Phase · Engineer")};
    FString SkinId=Omega?OmegaNames[P]:AlphaNames[P];
    GConfig->GetString(*Section,Omega?TEXT("OmegaSkinId"):TEXT("AlphaSkinId"),SkinId,GGameIni);
    if(Catalog)
    {
        const auto& Skins=Omega?Catalog->OmegaSkins:Catalog->AlphaSkins;
        const int32 Found=Skins.IndexOfByPredicate([&](const auto& A){return A.DisplayName==SkinId;});
        if(Found!=INDEX_NONE)R.Skin=Found;
        else UE_LOG(LogTemp,Error,TEXT("ER_LOADOUT_SKIN_MISSING preset=%s skin=%s"),*Preset.ToString(),*SkinId);
    }
    R.Items.Add({P+3,P==4 ? (V==0?6:4):Setting(Section,TEXT("Loaded"),Loaded[P]),P==4?0:Setting(Section,TEXT("Reserve"),Reserve[P])});
    const bool SMG=V==0 || P==4;
    const FString SecondarySection=SMG?TEXT("RespawnAndRage.Loadout.smg"):TEXT("RespawnAndRage.Loadout.pistol");
    R.Items.Add({SMG?2:1,Setting(SecondarySection,TEXT("Loaded"),SMG?30:7),Setting(SecondarySection,TEXT("Reserve"),SMG?60:14)});
    R.Items.Add({0,1,0});
    if(V==1) { R.Items.Add({8,2,0});R.Items.Add({9,1,0}); }
    if(V==2) { R.Items.Add({9,2,0});R.Items.Add({10,1,0}); }
    for(auto& Item:R.Items)
    {
        // Double carried supplies, preserving firearm magazine capacities and melee.
        if(Item.WeaponKey>=8) { Item.Loaded*=2;Item.Reserve*=2; }
        else if(Item.WeaponKey!=0) Item.Reserve=Item.Loaded+2*Item.Reserve;
        if(Rules)
        {
            const FString Prefix=TEXT("Kit.")+R.Preset.ToString()+TEXT(".")+R.Variant.ToString()+TEXT(".");
            const int32 Index=&Item-R.Items.GetData();
            if(Index<2)
            {
                Item.Loaded=int32(Rules->Get(FName(*(Prefix+(Index==0?TEXT("PrimaryLoaded"):TEXT("SecondaryLoaded")))),Item.Loaded));
                Item.Reserve=int32(Rules->Get(FName(*(Prefix+(Index==0?TEXT("PrimaryReserve"):TEXT("SecondaryReserve")))),Item.Reserve));
            }
            else if(Item.WeaponKey>=8)
            {
                const TCHAR* Name=Item.WeaponKey==8?TEXT("HE"):Item.WeaponKey==9?TEXT("Smoke"):TEXT("Claymore");
                Item.Loaded=int32(Rules->Get(FName(*(Prefix+Name)),Item.Loaded));Item.Reserve=0;
            }
        }
        Item.WeaponId=WeaponAssetName(Item.WeaponKey);
        if(Catalog)
        {
            const auto* Found=Catalog->ReferenceArsenal.FindByPredicate([&](const auto D){return D && D->GetFName()==Item.WeaponId;});
            Item.Data=Found?Found->Get():nullptr;
            if(Item.Data && Item.WeaponKey>0 && Item.WeaponKey<8)
            {
                const int32 Total=Item.Loaded+Item.Reserve;
                const FString WeaponName=Item.WeaponId.ToString().RightChop(3);
                const int32 Capacity=Rules?int32(Rules->Get(FName(*(TEXT("Weapon.")+WeaponName+TEXT(".MagazineSize"))),Item.Data->MagazineSize)):Item.Data->MagazineSize;
                Item.Loaded=FMath::Min(Item.Loaded,Capacity);
                Item.Reserve=Total-Item.Loaded;
            }
        }
    }
    return R;
}
FString FERResolvedLoadout::Summary() const
{
    FString S;
    for(int32 I=0;I<Items.Num();++I)
    {
        const auto& Item=Items[I];
        const FString Name=ERLoadouts::WeaponName(Item.WeaponKey);
        if(I<2) S+=FString::Printf(TEXT("%s: %s · Loaded %d · Reserve %d\n"),I==0?TEXT("Primary"):TEXT("Secondary"),*Name,Item.Loaded,Item.Reserve);
        else S+=FString::Printf(TEXT("%s: %s x%d\n"),I==2?TEXT("Melee"):TEXT("Utility"),*Name,Item.Loaded+Item.Reserve);
    }
    if(Items.Num()==3)S+=TEXT("Utility: None\n");
    return S+FString::Printf(TEXT("%.0f HP · %.0f armor%s"),StartingHealth,StartingArmor,Helmet?TEXT(" · Helmet"):TEXT(""));
}
