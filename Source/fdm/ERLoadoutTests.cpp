#include "ERLoadout.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "Algo/Reverse.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERLoadoutCatalogTests,"EclipseRecoil.Loadout.Catalog",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERLoadoutCatalogTests::RunTest(const FString&)
{
    const int32 Loaded[]={7,30,20,5,6},Reserve[]={28,90,40,15,0};
    TSet<int32> Alpha,Omega;
    for(int32 P=0;P<5;++P)for(int32 V=0;V<3;++V)
    {
        const auto R=ERLoadouts::Resolve(ERLoadouts::Presets()[P],ERLoadouts::Variants()[V]);
        TestEqual(TEXT("Primary identity"),R.Items[0].WeaponKey,P+3);
        TestEqual(TEXT("Initial loaded"),R.Items[0].Loaded,P==4 && V>0?4:Loaded[P]);
        TestEqual(TEXT("Initial reserve"),R.Items[0].Reserve,R.Items[0].Loaded+2*Reserve[P]);
        TestEqual(TEXT("Secondary identity"),R.Items[1].WeaponKey,P==4 || V==0?2:1);
        TestEqual(TEXT("Secondary total doubled"),R.Items[1].Loaded+R.Items[1].Reserve,(P==4 || V==0)?180:42);
        TestEqual(TEXT("Knife common"),R.Items[2].WeaponKey,0);
        TestEqual(TEXT("Knife unchanged"),R.Items[2].Loaded+R.Items[2].Reserve,1);
        TestEqual(TEXT("No hidden utility"),R.Items.Num(),V==0?3:5);
        if(V>0)
        {
            TestEqual(TEXT("Utility 1 identity"),R.Items[3].WeaponKey,V==1?8:9);
            TestEqual(TEXT("Utility 1 quantity"),R.Items[3].Loaded+R.Items[3].Reserve,4);
            TestEqual(TEXT("Utility 2 identity"),R.Items[4].WeaponKey,V==1?9:10);
            TestEqual(TEXT("Utility 2 quantity"),R.Items[4].Loaded+R.Items[4].Reserve,2);
        }
        Alpha.Add(R.Skin);Omega.Add(ERLoadouts::Resolve(R.Preset,R.Variant,true).Skin);
        TestEqual(TEXT("Variants share skin"),R.Skin,ERLoadouts::Resolve(R.Preset,"offense").Skin);
    }
    TestEqual(TEXT("Five Alpha identities"),Alpha.Num(),5);TestEqual(TEXT("Five Omega identities"),Omega.Num(),5);
    for(const auto Pair:{TPair<FName,FName>("invalid","defense"),{"sniper","invalid"},{NAME_None,NAME_None}})
    {
        const auto R=ERLoadouts::Resolve(Pair.Key,Pair.Value);TestEqual(TEXT("Safe preset"),R.Preset,FName("rifleman"));TestEqual(TEXT("Safe variant"),R.Variant,FName("offense"));
    }
    for(int32 Old=-1;Old<11;++Old)TestEqual(TEXT("Deterministic migration"),ERLoadouts::Migrate(Old),ERLoadouts::Presets()[Old>=3 && Old<=7?Old-3:1]);
    auto* Catalog=LoadClass<AERCharacter>(nullptr,TEXT("/Game/EclipseRecoil/Core/BP_ERCharacter.BP_ERCharacter_C"));
    TestNotNull(TEXT("Saved catalog"),Catalog);if(!Catalog)return false;
    const auto* C=Catalog->GetDefaultObject<AERCharacter>();
    for(const FName P:ERLoadouts::Presets())for(const FName V:ERLoadouts::Variants())for(bool Team:{false,true})
    {
        const auto R=ERLoadouts::Resolve(P,V,Team,C);
        const auto& Skins=Team?C->OmegaSkins:C->AlphaSkins;
        TestTrue(TEXT("Skin present"),Skins.IsValidIndex(R.Skin) && Skins[R.Skin].Mesh);
        for(const auto& I:R.Items)
        {
            TestTrue(TEXT("Item present"),I.Data!=nullptr);
            if(!I.Data)continue;
            const auto* D=I.Data.Get();
            if(I.WeaponKey>0 && I.WeaponKey<8)
                TestTrue(TEXT("Magazine fits physical capacity"),I.Loaded<=D->MagazineSize);
            const auto Base=ERLoadouts::Resolve(P,V,Team);
            const auto* Expected=Base.Items.FindByPredicate([&](const auto& Entry){return Entry.WeaponKey==I.WeaponKey;});
            TestTrue(TEXT("Catalog preserves complete carried supply"),Expected && I.Loaded+I.Reserve==Expected->Loaded+Expected->Reserve);
            TestTrue(TEXT("No stun"),D->Payload!=EERPayload::Stun);
            if(I.WeaponKey==10)TestTrue(TEXT("Claymore art is bound"),D->Mesh && D->PlacedMesh && D->Payload==EERPayload::Claymore);
        }
    }
    auto* Mutable=const_cast<AERCharacter*>(C);const auto Original=Mutable->ReferenceArsenal;
    Algo::Reverse(Mutable->ReferenceArsenal);
    for(const FName P:ERLoadouts::Presets())for(const FName V:ERLoadouts::Variants())
    {
        const auto R=ERLoadouts::Resolve(P,V,false,Mutable);
        for(const auto& I:R.Items)TestTrue(TEXT("Catalog reorder preserves stable weapon identity"),I.Data && I.Data->GetFName()==I.WeaponId);
    }
    Mutable->ReferenceArsenal=Original;
    const auto* Shotgun=ERLoadouts::Resolve("breacher","gunner",false,C).Items[0].Data.Get();
    TestEqual(TEXT("Shotgun8 pellets"),Shotgun->Pellets,8);TestEqual(TEXT("Shotgun22 damage"),Shotgun->Damage,22.f);TestEqual(TEXT("Shotgun cadence"),Shotgun->ShotInterval,.35f);
    return true;
}
#endif
