#include "ERWorldPoseProbe.h"
#include "ERCharacter.h"
#include "ERWeapon.h"
#include "ERPostureMesh.h"
#include "ERWorldWeaponPose.h"
#include "ERMovement.h"
#include "ERMatch.h"
#include "Camera/CameraActor.h"
#include "Camera/CameraComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"
#include "Engine/SkeletalMeshSocket.h"
#include "Engine/StaticMeshActor.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInterface.h"
#include "Engine/PointLight.h"
#include "Components/PointLightComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/HUD.h"
#include "Net/UnrealNetwork.h"
#include "Misc/Paths.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
namespace
{
constexpr int32 Cases=13,Total=20*Cases;
const int32 Slots[]={1,2,3,4,5,6,7,4,5,6,7,5,5};
bool AllGallery(){return FParse::Param(FCommandLine::Get(),TEXT("ERWorldPoseAllGallery"));}
bool HandDetail(){return FParse::Param(FCommandLine::Get(),TEXT("ERWorldPoseHands"));}
int32 Override(const TCHAR* Key){int32 Value=INDEX_NONE;FParse::Value(FCommandLine::Get(),Key,Value);return Value;}
bool SingleGallery(){return Override(TEXT("ERWorldPoseSkin="))!=INDEX_NONE && Override(TEXT("ERWorldPoseCase="))!=INDEX_NONE;}
int32 ProbeSkin(int32 Sample){const int32 Value=Override(TEXT("ERWorldPoseSkin="));return Value!=INDEX_NONE ? Value:AllGallery() ? Sample:Sample/Cases;}
int32 ProbeCase(int32 Sample){const int32 Value=Override(TEXT("ERWorldPoseCase="));return Value!=INDEX_NONE ? Value:AllGallery() ? 3:Sample%Cases;}
int32 ProbeTotal(){return SingleGallery() ? 1:AllGallery() ? 20:FParse::Param(FCommandLine::Get(),TEXT("ERWorldPoseQuick")) ? Cases:Total;}
}
AERWorldPoseProbe::AERWorldPoseProbe()
{PrimaryActorTick.bCanEverTick=true;PrimaryActorTick.TickGroup=TG_PostUpdateWork;bReplicates=true;bAlwaysRelevant=true;SetNetUpdateFrequency(30);}
void AERWorldPoseProbe::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AERWorldPoseProbe,Subject);DOREPLIFETIME(AERWorldPoseProbe,Sample);}
void AERWorldPoseProbe::Configure()
{
    auto* State=Subject->GetPlayerState<AERPlayerState>();
    const int32 Skin=ProbeSkin(Sample),Case=ProbeCase(Sample);State->Team=Skin<10 ? EERTeam::Alpha:EERTeam::Omega;State->ActiveSkin=Skin%10;State->ForceNetUpdate();
    Subject->ServerSelectWeapon(Slots[Case]);
    if(Subject->Weapon){Subject->Weapon->ScopeLevel=Case>=7 ? 1:0;Subject->Weapon->ForceNetUpdate();}
    if(Subject->GetController())Subject->GetController()->SetControlRotation(FRotator(Case==11 ? 35:Case==12 ? -35:0,0,0));
    Subject->ForceNetUpdate();Changed=Age;View=0;Captured=0;
}
bool AERWorldPoseProbe::CheckPose(bool Client)
{
    if(!Subject || !Subject->Weapon || !Subject->Weapon->Data)
    {UE_LOG(LogTemp,Error,TEXT("ER_WORLD_POSE_FAIL sample=%d missing subject, weapon, or data client=%d"),Sample,Client);Failed=true;return false;}
    const auto* D=Subject->Weapon->Data.Get();auto* Body=Subject->GetMesh();const auto* State=Subject->GetPlayerState<AERPlayerState>();
    const int32 Skin=ProbeSkin(Sample),Case=ProbeCase(Sample);
    if(!State || State->ActiveSkin!=Skin%10 || (State->Team==EERTeam::Alpha)!=(Skin<10))
    {UE_LOG(LogTemp,Error,TEXT("ER_WORLD_POSE_FAIL sample=%d replicated skin mismatch client=%d"),Sample,Client);Failed=true;return false;}
    const auto* Defaults=Subject->GetClass()->GetDefaultObject<AERCharacter>();
    if(!Defaults->ReferenceArsenal.IsValidIndex(Slots[Case]) || D!=Defaults->ReferenceArsenal[Slots[Case]])
    {UE_LOG(LogTemp,Error,TEXT("ER_WORLD_POSE_FAIL sample=%d replicated weapon mismatch client=%d"),Sample,Client);Failed=true;return false;}
    const auto Gun=Subject->WorldRifle->GetComponentTransform();
    const FVector ExpectedSkinSeat=Skin>=15 ? FVector(0,2,0):FVector::ZeroVector;
    const bool SkinSeat=Subject->GetTeamAppearance().WorldSupportPalmOffsetCm.Equals(ExpectedSkinSeat,.001f);
    const FVector ExpectedSupport=D->bOneHanded ? D->WorldPistolSupportContact:
        D->WorldLeftPalmContact+ExpectedSkinSeat/D->WorldWeaponScale;
    const float R=FVector::Distance(Body->GetSocketLocation(TEXT("ER_Palm_R")),Gun.TransformPosition(D->WorldRightPalmContact));
    const FVector Left=Body->GetSocketLocation(TEXT("ER_Palm_L"));float L=MAX_flt;
    L=FVector::Distance(Left,Gun.TransformPosition(ExpectedSupport));
    const float Dot=FVector::DotProduct(Gun.GetUnitAxis(EAxis::X),Subject->GetBaseAimRotation().Vector());
    const FQuat ExpectedPalm=Gun.GetRotation()*D->WorldRightPalmRotation.Quaternion();
    const FQuat ActualPalm=Body->GetSocketTransform(TEXT("ER_Palm_R")).GetRotation();
    const float WristAngle=FMath::RadiansToDegrees(ExpectedPalm.AngularDistance(ActualPalm));
    const FVector LeftForearm=(Body->GetSocketLocation(TEXT("hand_l"))-Body->GetSocketLocation(TEXT("lowerarm_l"))).GetSafeNormal();
    const float LeftContinuation=FVector::DotProduct(LeftForearm,Body->GetSocketTransform(TEXT("ER_Palm_L")).GetUnitAxis(EAxis::X));
    const float ElbowHeight=Gun.GetLocation().Z-Body->GetSocketLocation(TEXT("lowerarm_r")).Z;
    const bool Scale=Gun.GetScale3D().Equals(FVector(D->WorldWeaponScale),.001);
    const bool Good=R<.5f && L<1.5f && Dot>.995 && WristAngle<1.f && (D->bOneHanded || LeftContinuation>0.f) && Scale && SkinSeat && D->bWorldReferencePose && Subject->GetTeamAppearance().Matches(Body);
    if(!Good && !Client)
    {
        const FVector Shoulder=Body->GetSocketLocation(TEXT("upperarm_l")),Elbow=Body->GetSocketLocation(TEXT("lowerarm_l"));
        const FVector Target=Gun.TransformPosition(ExpectedSupport);
        const float Reach=FVector::Distance(Shoulder,Elbow)+FVector::Distance(Elbow,Left);
        UE_LOG(LogTemp,Display,TEXT("ER_WORLD_POSE_REACH sample=%d shoulder=%s target=%s hand=%s reach=%.3f target_distance=%.3f"),
            Sample,*Shoulder.ToString(),*Target.ToString(),*Left.ToString(),Reach,FVector::Distance(Shoulder,Target));
    }
    UE_LOG(LogTemp,Display,TEXT("ER_WORLD_POSE_%s sample=%d skin=%d case=%d weapon=%s right=%.3f left=%.3f wrist_angle=%.3f left_continuation=%.3f elbow_height=%.3f aim=%.5f scale=%.3f client=%d"),Good ? TEXT("PASS"):TEXT("FAIL"),Sample,Skin,Case,*D->DisplayName,R,L,WristAngle,LeftContinuation,ElbowHeight,Dot,Gun.GetScale3D().X,Client);
    Failed|=!Good;return Good;
}
void AERWorldPoseProbe::Capture(int32 Angle)
{
    auto* PC=UGameplayStatics::GetPlayerController(GetWorld(),0);if(!PC)return;
    if(!Camera)Camera=GetWorld()->SpawnActor<ACameraActor>();
    const FVector Centre=HandDetail() ? Subject->GetMesh()->GetSocketLocation(TEXT("ER_Palm_L")):Subject->GetActorLocation()+FVector(10,0,SingleGallery() ? 42:15);
    const FVector Views[]={HandDetail() ? FVector(130,100,70):FVector(270,210,40),HandDetail() ? FVector(170,0,50):FVector(320,0,20),HandDetail() ? FVector(0,160,50):SingleGallery() ? FVector(0,300,20):FVector(-240,-210,50),HandDetail() ? FVector(35,10,175):FVector(70,25,390)};
    Camera->SetActorLocation(Centre+Views[FMath::Clamp(Angle,0,3)]);
    Camera->SetActorRotation((Centre-Camera->GetActorLocation()).Rotation());Camera->GetCameraComponent()->FieldOfView=HandDetail() ? 24:SingleGallery() ? 28:42;PC->SetViewTarget(Camera);
    Captured=Age;
}
void AERWorldPoseProbe::Tick(float Delta)
{
    Super::Tick(Delta);Age+=Delta;
    if(!HasAuthority())
    {
        if(Sample>=0 && Sample<ProbeTotal() && Sample!=Seen && Subject && Age>3)
        {
            if(Changed!=Sample){Changed=Sample;Captured=Age;}
            const auto* State=Subject->GetPlayerState<AERPlayerState>();const auto* D=Subject->Weapon ? Subject->Weapon->Data.Get():nullptr;
            const auto* Defaults=Subject->GetClass()->GetDefaultObject<AERCharacter>();const int32 Skin=ProbeSkin(Sample),Slot=Slots[ProbeCase(Sample)];
            const bool Replicated=State && State->ActiveSkin==Skin%10 && (State->Team==EERTeam::Alpha)==(Skin<10) &&
                D && Defaults->ReferenceArsenal.IsValidIndex(Slot) && D==Defaults->ReferenceArsenal[Slot];
            if(Replicated && Age-Captured>.25f && CheckPose(true))Seen=Sample;
        }
        return;
    }
    if(!Ready)
    {
        if(FParse::Param(FCommandLine::Get(),TEXT("ERWorldPoseNetwork")))
        {
            int32 Players=0;for(auto It=GetWorld()->GetPlayerControllerIterator();It;++It)++Players;
            if(Players<2)return;
            if(NetworkReadyAt==0)NetworkReadyAt=Age;
            if(Age-NetworkReadyAt<4)return;
        }
        auto* PC=UGameplayStatics::GetPlayerController(GetWorld(),0);Subject=PC ? Cast<AERCharacter>(PC->GetPawn()):nullptr;
        if(FParse::Param(FCommandLine::Get(),TEXT("ERWorldPoseExternal")))
            for(TActorIterator<AERCharacter> It(GetWorld());It;++It)if(*It!=Subject && It->Weapon){Subject=*It;break;}
        if(!Subject || !Subject->Weapon || Age<5)return;
        Subject->bAlwaysRelevant=true;Subject->TeleportTo(FVector(30000,0,92.5),FRotator::ZeroRotator,false,true);Subject->GetCharacterMovement()->DisableMovement();
        PC->SetIgnoreMoveInput(true);PC->SetIgnoreLookInput(true);if(PC->GetHUD())PC->GetHUD()->bShowHUD=false;
        Subject->FirstPersonBody->SetHiddenInGame(true);Subject->FirstPersonRifle->SetHiddenInGame(true);Subject->FirstPersonSight->SetHiddenInGame(true);
        Subject->GetMesh()->SetOwnerNoSee(false);Subject->WorldRifle->SetOwnerNoSee(false);Subject->WorldSight->SetOwnerNoSee(false);
        if(FApp::CanEverRender())
        {
            auto* Cube=LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cube.Cube"));
            for(int32 I=0;I<2;++I)
            {
                auto* Back=GetWorld()->SpawnActor<AStaticMeshActor>(FVector(30000-(I ? 100:0),0,I ? 180:-10),FRotator::ZeroRotator);
                Back->GetStaticMeshComponent()->SetStaticMesh(Cube);Back->GetStaticMeshComponent()->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Engine/BasicShapes/BasicShapeMaterial.BasicShapeMaterial")));
                Back->SetActorScale3D(I ? FVector(.1,30,8):FVector(30,30,.2));Back->GetStaticMeshComponent()->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            }
            for(float Side:{-1.f,1.f})
            {auto* Light=GetWorld()->SpawnActor<APointLight>(FVector(30300,Side*240,310),FRotator::ZeroRotator);Light->SetMobility(EComponentMobility::Movable);Light->PointLightComponent->SetIntensity(20000);Light->PointLightComponent->SetAttenuationRadius(1200);Light->PointLightComponent->SetCastShadows(true);}
        }
        Ready=true;Sample=0;Configure();return;
    }
    const int32 TestTotal=ProbeTotal();
    if(Sample>=TestTotal)
    {if(Age-Changed>4){UE_LOG(LogTemp,Display,TEXT("ER_WORLD_POSE_COMPLETE result=%s samples=%d"),Failed ? TEXT("FAIL"):TEXT("PASS"),TestTotal);FPlatformMisc::RequestExitWithStatus(false,Failed ? 1:0);}return;}
    if(AdvanceAt>0)
    {
        if(Age-AdvanceAt<.1f)return;
        AdvanceAt=0;++Sample;Changed=Age;ForceNetUpdate();if(Sample<TestTotal)Configure();return;
    }
    const bool Gallery=FApp::CanEverRender() && FParse::Param(FCommandLine::Get(),TEXT("ERWorldPoseGallery"));
    const int32 Skin=ProbeSkin(Sample),Case=ProbeCase(Sample);
    const bool Shot=Gallery && (SingleGallery() || AllGallery() || Skin==0 || Skin==5 || Skin==10 || Skin==15);
    if(Age-Changed<(GetNetMode()==NM_Standalone ? .4f:.75f))return;
    if(Seen!=Sample){CheckPose(false);Seen=Sample;}
    if(Shot)
    {
        if(Captured==0){Capture(View);return;}
        if(Age-Captured<.6f)return;
        FScreenshotRequest::RequestScreenshot(FPaths::ProjectSavedDir()/FString::Printf(TEXT("Testing/WorldPoses/Skin%02d-Case%02d-View%d%s.png"),Skin,Case,View,HandDetail() ? TEXT("-Hands"):TEXT("")),false,false);
        if(++View<(AllGallery() || SingleGallery() ? 4:2)){Captured=0;Changed=Age;return;}
        AdvanceAt=Age;return;
    }
    ++Sample;Changed=Age;ForceNetUpdate();if(Sample<TestTotal)Configure();
}
