#include "ERWeapon.h"
#include "ERShotTrace.h"
#include "Engine/DamageEvents.h"
#include "ERShotEffects.h"
#include "ERMovement.h"
#include "ERWeaponBallistics.h"
#include "ERCharacter.h"
#include "ERMatch.h"
#include "ERPlayerController.h"
#include "ERGrenadeExplosion.h"
#include "ERSmokeCloud.h"
#include "ERBlockRoom.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "EngineUtils.h"
#include "Camera/CameraComponent.h"
#include "Components/DecalComponent.h"
#include "Components/SphereComponent.h"
#include "Components/CapsuleComponent.h"
#include "GameFramework/ProjectileMovementComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Net/UnrealNetwork.h"
#include "TimerManager.h"
#include "GameFramework/CharacterMovementComponent.h"

namespace
{
void SpawnBulletHole(UWorld* World, UMaterialInterface* Material, const FVector& Position, const FVector& Normal, float Roll)
{
    if (!World || World->GetNetMode() == NM_DedicatedServer || !Material || Normal.IsNearlyZero()) return;
    FRotator Rotation = Normal.Rotation();
    Rotation.Roll = Roll;
    if (auto* Decal = UGameplayStatics::SpawnDecalAtLocation(World, Material, FVector(1.5f, 5.f, 5.f),
        Position + Normal * .15f, Rotation, 20.f))
    {
        // Keep sustained automatic fire bounded on each rendering peer.
        static TArray<TWeakObjectPtr<UDecalComponent>> Holes;
        Holes.RemoveAll([](const auto& Hole) { return !Hole.IsValid(); });
        while (Holes.Num() >= 128)
        {
            if (Holes[0].IsValid()) Holes[0]->DestroyComponent();
            Holes.RemoveAt(0);
        }
        Holes.Add(Decal);
        Decal->SetFadeScreenSize(.0005f);
        Decal->SetFadeOut(15.f, 5.f, false);
    }
}
}

AERWeapon::AERWeapon()
{
    bReplicates = true; bNetUseOwnerRelevancy = true;
    SetRootComponent(CreateDefaultSubobject<USceneComponent>(TEXT("Root")));
    ProjectileClass = AERProjectile::StaticClass();
}
void AERWeapon::BeginPlay()
{
    Super::BeginPlay();
    if (!Data) Data = NewObject<UERWeaponData>(this);
    if (const auto* Character = Cast<AERCharacter>(GetOwner()))
        UERShotEffects::Prewarm(GetWorld(), ImpactSystem, Character->Camera->GetComponentLocation());
    if (ProjectileClass)
    {
        AERGrenadeExplosion::PrecacheEffect(ProjectileClass->GetDefaultObject<AERProjectile>()->GrenadeExplosionClass);
        if(auto* Character=Cast<AERCharacter>(GetOwner()))
            AERSmokeCloud::Prewarm(GetWorld(),ProjectileClass->GetDefaultObject<AERProjectile>()->SmokeCloudClass,Character->Camera->GetComponentLocation());
    }
    if (HasAuthority()) { MagazineAmmo = Data->MagazineSize; ReserveAmmo = Data->InitialReserve; ShotRandom.Initialize(FMath::Rand()); }
}
float AERWeapon::GetBloomDegrees() const
{
    if (!Data || !GetWorld()) return 0;
    const auto* State = GetWorld()->GetGameState<AERGameState>();
    const double Now = State ? State->GetServerWorldTimeSeconds() : GetWorld()->GetTimeSeconds();
    const float RecoveringTime = FMath::Max(0.f, float(Now - LastSpreadShotServerTime) - Data->BloomRecoveryDelay);
    return FMath::Clamp(BloomAtLastShot - RecoveringTime * Data->BloomRecoveryDegreesPerSecond, 0.f, Data->MaxBloomDegrees);
}
float AERWeapon::GetSpreadDegrees() const
{
    const auto* Character = Cast<AERCharacter>(GetOwner());
    if (!Data || !Character) return 0;
    const float SpeedFraction = FMath::Clamp(Character->GetVelocity().Size2D() / (1000.f / 3.6f), 0.f, 1.f);
    const auto* Movement = Cast<UERMovement>(Character->GetCharacterMovement());
    const float LowPosture = FMath::GetMappedRangeValueClamped(FVector2D(62.5f,92.5f), FVector2D(Data->CrouchSpreadMultiplier,1.f), Movement->Locomotion.HalfHeight);
    const float Posture = FMath::Lerp(LowPosture, Data->CrouchSpreadMultiplier * .65f, Movement->Locomotion.ProneBlend);
    const float Air = Character->GetCharacterMovement()->IsFalling() ? Data->AirborneSpreadDegrees : 0.f;
    return FMath::Clamp((Data->StandingSpreadDegrees + Data->RunningSpreadDegrees * SpeedFraction) * Posture + Air + GetBloomDegrees(), 0.f, 10.f);
}
bool AERWeapon::CanFight() const
{
    const auto* Character = Cast<AERCharacter>(GetOwner());
    const auto* Mode = GetWorld()->GetAuthGameMode<AERGameMode>();
    return HasAuthority() && Character && !Character->Health->bDead && Character->Weapon == this && Mode && Mode->IsMatchInProgress();
}
bool AERWeapon::IsHandGrenade(EERPayload Kind)
{ return Kind==EERPayload::HE || Kind==EERPayload::Smoke || Kind==EERPayload::Stun; }
bool AERWeapon::IsGrenadeHandHidden() const
{
    if(!Data || !IsHandGrenade(Data->Payload)) return false;
    const auto* State=GetWorld()->GetGameState<AERGameState>();
    const double Now=State?State->GetServerWorldTimeSeconds():GetWorld()->GetTimeSeconds();
    return bCookingGrenade || MagazineAmmo<=0 || Now<GrenadeRecoveryEndServerTime;
}
void AERWeapon::StartFire()
{
    if (!CanFight() || bReloading || MagazineAmmo <= 0 || bWantsFire) return;
    if(IsHandGrenade(Data->Payload))
    {
        if(GetWorld()->GetTimeSeconds()<NextAllowedShotTime) return;
        bWantsFire=true;ArmGrenade();return;
    }
    bWantsFire = true; TryFire();
}
void AERWeapon::StopFire()
{
    bWantsFire=false;GetWorldTimerManager().ClearTimer(FireTimer);
    if(bCookingGrenade) ReleaseGrenade(CanFight());
}
void AERWeapon::ArmGrenade()
{
    auto* Character=Cast<AERCharacter>(GetOwner());
    if(!Character || !ProjectileClass) { bWantsFire=false;return; }
    const FTransform Transform(FRotator::ZeroRotator,AERProjectile::SafeHandPosition(Character,true));
    auto* P=GetWorld()->SpawnActorDeferred<AERProjectile>(ProjectileClass,Transform,this,Character,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
    if(!P) { bWantsFire=false;return; }
    P->Payload=Data->Payload;P->ThrownMesh=Data->ProjectileMesh?Data->ProjectileMesh:Data->Mesh;
    P->Damage=Data->Damage;P->ExplosionRadius=Data->ExplosionRadius;P->ExplosionInnerRadius=Data->ExplosionInnerRadius;
    P->BlastArmorRatio=ERWeaponBallistics::GrenadeArmorRatio(Data->ArmorRatio);
    P->FuseDuration=Data->FuseDuration;P->bHeld=true;
    P->Movement->InitialSpeed=0;P->Movement->MaxSpeed=Data->ProjectileSpeed;P->Movement->Velocity=FVector::ZeroVector;
    CookedGrenade=P;bCookingGrenade=true;--MagazineAmmo;++AuthorizedShotCount;
    const double Now=GetWorld()->GetTimeSeconds();
    NextAllowedShotTime=Now+Data->ShotInterval;Character->Health->ProtectionEndServerTime=0;
    UGameplayStatics::FinishSpawningActor(P,Transform);ForceNetUpdate();Character->ForceNetUpdate();
    UE_LOG(LogTemp,Display,TEXT("ER_GRENADE_ARM kind=%d fuse=%.2f ammo=%d"),int32(P->Payload),P->FuseDuration,MagazineAmmo);
}
void AERWeapon::ReleaseGrenade(bool bThrow)
{
    auto* P=CookedGrenade.Get();
    CookedGrenade=nullptr;bCookingGrenade=false;
    auto* Character=Cast<AERCharacter>(GetOwner());
    if(IsValid(P) && P->bHeld)
    {
        FVector Velocity=FVector::ZeroVector;
        FVector Position=P->GetActorLocation();
        if(Character)
        {
            Position=AERProjectile::SafeHandPosition(Character,bThrow,P);
            if(bThrow)
            {
                // Physical overhead origin; a modest upward component creates
                // the arm throw without changing camera/control rotation.
                const FVector Direction=(Character->GetBaseAimRotation().Vector()+FVector(0,0,.35f)).GetSafeNormal();
                Velocity=Direction*Data->ProjectileSpeed+Character->GetVelocity();
            }
            else Velocity=Character->GetVelocity()*.5f;
        }
        P->ReleaseHeld(Position,Velocity);
        if(bThrow && Character) Character->MulticastShot(Position,Position+Velocity.GetSafeNormal()*250,false);
        UE_LOG(LogTemp,Display,TEXT("ER_GRENADE_RELEASE thrown=%d remaining=%.3f position=%s velocity=%s"),bThrow,
            P->FuseEndServerTime-GetWorld()->GetTimeSeconds(),*Position.ToString(),*Velocity.ToString());
    }
    const double Now=GetWorld()->GetTimeSeconds();
    NextAllowedShotTime=FMath::Max(NextAllowedShotTime,Now+.35);GrenadeRecoveryEndServerTime=NextAllowedShotTime;
    ForceNetUpdate();
}
void AERWeapon::OnGrenadeDetonated(AERProjectile* Projectile)
{
    if(CookedGrenade!=Projectile) return;
    CookedGrenade=nullptr;bCookingGrenade=false;
    GrenadeRecoveryEndServerTime=GetWorld()->GetTimeSeconds()+.35;
    NextAllowedShotTime=FMath::Max(NextAllowedShotTime,GrenadeRecoveryEndServerTime);
    ForceNetUpdate();
}
void AERWeapon::TryFire()
{
    if (!bWantsFire || !CanFight() || bReloading || MagazineAmmo <= 0) { StopFire(); return; }
    const double Now = GetWorld()->GetTimeSeconds();
    if (Now < NextAllowedShotTime)
    {
        // Repeating timers can arrive just before the float world-clock deadline.
        // Retry the remaining fraction instead of losing an entire firing interval.
        GetWorldTimerManager().SetTimer(FireTimer, this, &AERWeapon::TryFire, FMath::Max(.001f, float(NextAllowedShotTime-Now)), false);
        return;
    }
    auto* Character = CastChecked<AERCharacter>(GetOwner());
    FVector Eye; FRotator Aim;
    Character->GetActorEyesViewPoint(Eye, Aim);
    // Camera height is explicit: no animated socket is required for authoritative collision.
    Eye = Character->Camera->GetComponentLocation();
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ERRifle), false, Character); Params.AddIgnoredActor(this);
    FHitResult ViewHit;
    const FVector Far = Eye + Aim.Vector() * FMath::Max(1.f, Data->HitscanRange);
    const bool bViewHit = GetWorld()->LineTraceSingleByChannel(ViewHit, Eye, Far, ECC_GameTraceChannel1, Params);
    const FVector Target = bViewHit ? ViewHit.ImpactPoint : Far;
    const FVector IntendedMuzzle = Eye + Aim.RotateVector(FVector(45, 12, -12));
    LastShotAimDirection = (Target - IntendedMuzzle).GetSafeNormal();
    LastShotSpreadDegrees = GetSpreadDegrees();
    LastShotDirection = ShotRandom.VRandCone(LastShotAimDirection, FMath::DegreesToRadians(LastShotSpreadDegrees));
    FHitResult MuzzleHit;
    const bool bBlocked = GetWorld()->SweepSingleByChannel(MuzzleHit, Eye, IntendedMuzzle, FQuat::Identity, ECC_GameTraceChannel1, FCollisionShape::MakeSphere(3), Params);
    if (Data->Payload != EERPayload::Melee) --MagazineAmmo;
    NextAllowedShotTime = Now + Data->ShotInterval; Character->Health->ProtectionEndServerTime = 0;
    // Only an authorized shot changes burst buildup. Owners recover it against the server clock.
    BloomAtLastShot = FMath::Min(GetBloomDegrees() + Data->BloomPerShotDegrees, Data->MaxBloomDegrees);
    LastSpreadShotServerTime = Now;
    ForceNetUpdate();
    ++AuthorizedShotCount;
    LastShotTraceCount = 0;
    LastShotPelletHits.Reset();
    if (Data->Payload == EERPayload::Melee)
    {
        // Short, server-selected first-hit melee query; walls still block it.
        FHitResult Hit;
        LastShotTraceStart = Eye;
        LastShotTraceEnd = Eye + Aim.Vector() * FMath::Clamp(Data->HitscanRange, 1.f, 180.f);
        LastShotTraceCount = 1;
        if (GetWorld()->LineTraceSingleByChannel(Hit, Eye, LastShotTraceEnd, ECC_GameTraceChannel1, Params))
        {
            LastShotTraceEnd = Hit.ImpactPoint;
            UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Data->Damage, Aim.Vector(), Hit,
                Character->Controller, this, UDamageType::StaticClass());
        }
        Character->MulticastShot(Eye, LastShotTraceEnd, false);
    }
    else if (Data->Payload == EERPayload::Bullet)
    {
        const int32 PelletCount = FMath::Clamp(Data->Pellets, 1, 16);
        FVector FeedbackEnd = IntendedMuzzle;
        // At contact distance the locomotion capsule may overlap the intended muzzle.
        // Start at the eye and still resolve anatomy instead of assigning a fake chest hit.
        const bool ContactPawn=Data->bUseBodyHitZones && bBlocked && Cast<AERCharacter>(MuzzleHit.GetActor());
        const FVector PelletStart=ContactPawn ? Eye : IntendedMuzzle;
        const bool PelletBlocked=bBlocked && !ContactPawn;
        for (int32 Pellet = 0; Pellet < PelletCount; ++Pellet)
        {
            const FVector Direction = Data->bUseBodyHitZones ?
                ERWeaponBallistics::PelletDirection(LastShotDirection,Pellet,PelletCount,Data->PelletSpreadDegrees,ShotRandom) :
                (Pellet == 0 ? LastShotDirection : ShotRandom.VRandCone(LastShotAimDirection, FMath::DegreesToRadians(LastShotSpreadDegrees)));
            FHitResult Hit = MuzzleHit;
            const FVector TraceEnd = PelletStart + Direction * FMath::Max(1.f, Data->HitscanRange);
            const bool bHit = PelletBlocked || (Data->bUseBodyHitZones ? ERShotTrace::Pellet(GetWorld(),Hit,PelletStart,TraceEnd,Params) :
                GetWorld()->LineTraceSingleByChannel(Hit, IntendedMuzzle, TraceEnd, ECC_GameTraceChannel1, Params));
            ++LastShotTraceCount;
            if (Pellet == 0)
            {
                LastShotTraceStart = PelletBlocked ? Eye : PelletStart;
                LastShotTraceEnd = FeedbackEnd = bHit ? Hit.ImpactPoint : TraceEnd;
            }
            LastShotPelletHits.Add(bHit ? Hit : FHitResult());
            if (bHit)
            {
                auto* Room = Cast<AERBlockRoom>(Hit.GetActor());
                if (!bBlocked && Room && Room->CanMutateBlocks())
                {
                    const auto Shot = ERBlockCombat::ContinueShot(GetWorld(), Hit, IntendedMuzzle, Direction, Data->HitscanRange,
                        Data->Damage, Data->BlockShot, Params, Character->Controller, this);
                    LastShotTraceCount += Shot.ExtraTraces;
                    if (Pellet == 0) LastShotTraceEnd = FeedbackEnd = Shot.End;
                    const auto& Terminal = Shot.TerminalHit;
                    if (Terminal.bBlockingHit && Terminal.GetActor() && !Terminal.GetActor()->IsA<APawn>() && !Terminal.GetActor()->IsA<AERBlockRoom>())
                        MulticastImpact(Terminal.ImpactPoint, Terminal.ImpactNormal, FMath::FRandRange(0.f, 360.f), uint8(FMath::RandRange(0, 3)));
                }
                else
                {
                    // Ordinary maps and blocked muzzles retain their first-hit behaviour.
                    UGameplayStatics::ApplyPointDamage(Hit.GetActor(), Data->Damage, Direction, Hit,
                        Character->Controller, this, UDamageType::StaticClass());
                    if (Hit.GetActor() && !Hit.GetActor()->IsA<APawn>())
                        MulticastImpact(Hit.ImpactPoint, Hit.ImpactNormal, FMath::FRandRange(0.f, 360.f), uint8(FMath::RandRange(0, 3)));
                }
            }
        }
        const bool bTracer = Data->TracerEveryNShots > 0 && (AuthorizedShotCount - 1) % uint32(Data->TracerEveryNShots) == 0;
        Character->MulticastShot(LastShotTraceStart, FeedbackEnd, bTracer);
    }
    else
    {
        // Rockets and thrown utility retain their physical movement and fuse rules.
        const FVector LaunchPosition = bBlocked ? Eye + Aim.Vector() * 12.f : IntendedMuzzle;
        auto* Projectile = GetWorld()->SpawnActorDeferred<AERProjectile>(ProjectileClass, FTransform(LastShotDirection.Rotation(), LaunchPosition), this, Character, ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if (Projectile)
        {
            Projectile->Damage = Data->Damage;
            Projectile->Payload = Data->Payload;
            Projectile->ThrownMesh = Data->ProjectileMesh ? Data->ProjectileMesh : (Data->bOneHanded ? Data->Mesh : nullptr);
            Projectile->ExplosionRadius = Data->ExplosionRadius;
            Projectile->ExplosionInnerRadius = Data->ExplosionInnerRadius;
            Projectile->BlastArmorRatio = ERWeaponBallistics::GrenadeArmorRatio(Data->ArmorRatio);
            Projectile->FuseDuration = Data->FuseDuration;
            Projectile->Movement->InitialSpeed = Data->ProjectileSpeed;
            Projectile->Movement->MaxSpeed = Data->ProjectileSpeed;
            Projectile->Movement->Velocity = LastShotDirection * Data->ProjectileSpeed;
            Projectile->SetLifeSpan(Data->ProjectileLife);
            UGameplayStatics::FinishSpawningActor(Projectile, Projectile->GetActorTransform());
        }
        Character->MulticastShot(IntendedMuzzle, IntendedMuzzle + LastShotDirection * 250, false);
    }
    if (MagazineAmmo <= 0) StopFire();
    else if (bWantsFire && Data->bAutomatic) GetWorldTimerManager().SetTimer(FireTimer, this, &AERWeapon::TryFire, Data->ShotInterval, false);
}
void AERWeapon::MulticastImpact_Implementation(FVector_NetQuantize Position, FVector_NetQuantizeNormal Normal, float Roll, uint8 Variant)
{
    const AERProjectile* Defaults = ProjectileClass ? ProjectileClass->GetDefaultObject<AERProjectile>() : nullptr;
    UMaterialInterface* Material = ImpactDecalVariants.IsValidIndex(Variant) ? ImpactDecalVariants[Variant].Get() : (Defaults ? Defaults->ImpactDecalMaterial.Get() : nullptr);
    SpawnBulletHole(GetWorld(), Material, Position, Normal, Roll);
    UERShotEffects::SpawnImpact(GetWorld(), ImpactSystem, Position, Normal);
}
void AERWeapon::Reload()
{
    if (!CanFight() || bReloading || bCookingGrenade || MagazineAmmo >= Data->MagazineSize || Data->Payload == EERPayload::Melee) return;
    // Only authority enters CanFight; labs replenish reserve, retaining reload timing.
    if (GetWorld()->GetMapName().Contains(TEXT("Lab"))) ReserveAmmo = FMath::Max(ReserveAmmo, Data->MagazineSize);
    if (ReserveAmmo <= 0) return;
    StopFire(); bReloading = true;
    GetWorldTimerManager().SetTimer(ReloadTimer, this, &AERWeapon::FinishReload, Data->ReloadDuration, false);
}
void AERWeapon::FinishReload()
{
    if (CanFight() && bReloading)
    {
        const int32 Transfer = FMath::Min(Data->MagazineSize - MagazineAmmo, ReserveAmmo);
        MagazineAmmo += Transfer; ReserveAmmo -= Transfer;
    }
    bReloading = false;
}
void AERWeapon::Cancel()
{
    bWantsFire=false;GetWorldTimerManager().ClearTimer(FireTimer);
    if(bCookingGrenade) ReleaseGrenade(false);
    GetWorldTimerManager().ClearTimer(ReloadTimer);bReloading=false;
}
void AERWeapon::EndPlay(const EEndPlayReason::Type Reason) { Cancel(); Super::EndPlay(Reason); }
void AERWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AERWeapon, MagazineAmmo, COND_OwnerOnly); DOREPLIFETIME_CONDITION(AERWeapon, ReserveAmmo, COND_OwnerOnly); DOREPLIFETIME(AERWeapon, bReloading);
    DOREPLIFETIME_CONDITION(AERWeapon, BloomAtLastShot, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(AERWeapon, LastSpreadShotServerTime, COND_OwnerOnly);
    DOREPLIFETIME(AERWeapon, Data);
    DOREPLIFETIME(AERWeapon,bCookingGrenade);DOREPLIFETIME(AERWeapon,GrenadeRecoveryEndServerTime);
}
AERProjectile::AERProjectile()
{
    bReplicates = true; SetReplicateMovement(true);
    PrimaryActorTick.bCanEverTick = true;
    SetNetUpdateFrequency(30);
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision); Collision->InitSphereRadius(3);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_GameTraceChannel1);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    Collision->OnComponentHit.AddDynamic(this, &AERProjectile::Hit);
    ShotTarget=CreateDefaultSubobject<USphereComponent>(TEXT("ShotTarget"));
    ShotTarget->SetupAttachment(Collision);ShotTarget->InitSphereRadius(6);
    // Query-only on the shot channel; movement roots ignore this object channel.
    ShotTarget->SetCollisionObjectType(ECC_GameTraceChannel1);
    ShotTarget->SetCollisionResponseToAllChannels(ECR_Ignore);
    ShotTarget->SetCollisionResponseToChannel(ECC_GameTraceChannel1,ECR_Block);
    ShotTarget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    ShotTarget->SetGenerateOverlapEvents(false);ShotTarget->SetCanEverAffectNavigation(false);
    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->SetUpdatedComponent(Collision); Movement->InitialSpeed = Movement->MaxSpeed = 30000;
    Movement->bInitialVelocityInLocalSpace = false;
    Movement->ProjectileGravityScale = 0; Movement->bShouldBounce = false;
    Movement->bForceSubStepping = true;
    Movement->OnProjectileBounce.AddDynamic(this, &AERProjectile::Bounce);
    Visual = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(Collision); Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Effect = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Effect"));
    Effect->SetupAttachment(Collision); Effect->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Effect->SetCastShadow(false); Effect->SetVisibility(false);
    InitialLifeSpan = 2;
}
bool AERProjectile::UsesFuse(EERPayload Kind)
{
    return Kind == EERPayload::HE || Kind == EERPayload::Smoke || Kind == EERPayload::Stun || Kind == EERPayload::LaunchedHE;
}
void AERProjectile::BeginPlay()
{
    Super::BeginPlay();
    // Capture allegiance when armed; death, respawn or team changes cannot recolor it.
    if(HasAuthority() && Payload==EERPayload::Smoke)
        if(const auto* Pawn=GetInstigator())
            if(const auto* State=Pawn->GetPlayerState<AERPlayerState>()) SmokeTeamColor=AERSmokeCloud::ColorForTeam(State->Team);
    PreviousSmokePosition=GetActorLocation();
    Visual->SetSkeletalMeshAsset(ThrownMesh);
    Effect->SetStaticMesh(EffectMesh);
    if (Payload == EERPayload::Rocket)
    {
        Effect->SetMaterial(0, ExplosionMaterial);
        Effect->SetRelativeScale3D(FVector(.08f)); Effect->SetVisibility(true);
    }
    if (UsesFuse(Payload))
    {
        Movement->ProjectileGravityScale = 1;
        Movement->bShouldBounce = true;
        Movement->bRotationFollowsVelocity = false;
        Movement->Friction = .32f;
        Movement->bBounceAngleAffectsFriction = true;
        Movement->BounceVelocityStopSimulatingThreshold = 30;
        if (HasAuthority())
        {
            BounceRandom.Initialize(FMath::Rand());
            Movement->Bounciness = BounceRandom.FRandRange(.28f,.38f);
            Spin = FVector(BounceRandom.FRandRange(-7.f,7.f),BounceRandom.FRandRange(10.f,17.f),BounceRandom.FRandRange(-5.f,5.f));
            FuseDuration = FMath::Max(.05f,FuseDuration);
            FuseEndServerTime=GetWorld()->GetTimeSeconds()+FuseDuration;
            // A weapon's projectile lifetime must never cut its fuse short.
            SetLifeSpan(FMath::Max(GetLifeSpan(),FuseDuration+.5f));
            GetWorldTimerManager().SetTimer(FuseTimer, this, &AERProjectile::Detonate, FuseDuration, false);
        }
    }
    SetActorTickEnabled(HasAuthority() && UsesFuse(Payload));
    Collision->SetCollisionResponseToChannel(ECC_GameTraceChannel1,Payload==EERPayload::Bullet?ECR_Block:ECR_Ignore);
    if (HasAuthority()) { Collision->IgnoreActorWhenMoving(GetInstigator(), true); Collision->IgnoreActorWhenMoving(GetOwner(), true); }
    else { Movement->Deactivate(); Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
    OnRep_Held();
}
FVector AERProjectile::SafeHandPosition(AERCharacter* Character,bool bOverhead,const AActor* Ignore)
{
    const FVector Eye=Character->Camera->GetComponentLocation();
    const FRotator Yaw(0,Character->GetBaseAimRotation().Yaw,0);
    const FVector Wanted=Eye+(bOverhead?Yaw.RotateVector(FVector(-14,24,-4)):FVector(0,0,-35));
    FCollisionQueryParams Query(SCENE_QUERY_STAT(ERGrenadeHand),false,Character);
    if(Ignore) Query.AddIgnoredActor(Ignore);
    for(const auto& Item:Character->Inventory) if(Item) Query.AddIgnoredActor(Item);
    FHitResult Hit;
    if(Character->GetWorld()->SweepSingleByChannel(Hit,Eye,Wanted,FQuat::Identity,ECC_GameTraceChannel1,FCollisionShape::MakeSphere(3.f),Query))
        return Hit.bStartPenetrating?Eye:Hit.Location+Hit.ImpactNormal*.5f;
    return Wanted;
}
void AERProjectile::OnRep_Held()
{
    ShotTarget->SetSphereRadius(bHeld?12.f:6.f);
    ShotTarget->SetCollisionEnabled(UsesFuse(Payload) && !bImpacted ? ECollisionEnabled::QueryOnly : ECollisionEnabled::NoCollision);
    Visual->SetVisibility(!bHeld && !bImpacted);
    if(bHeld) { Movement->Deactivate();Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
}
void AERProjectile::ReleaseHeld(const FVector& Position,const FVector& Velocity)
{
    if(!HasAuthority() || !bHeld || bImpacted) return;
    bHeld=false;ReleasedFrom=Position;ReleasedVelocity=Velocity;
    ShotTarget->SetSphereRadius(6.f);
    SetActorLocation(Position,false,nullptr,ETeleportType::TeleportPhysics);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Movement->SetUpdatedComponent(Collision);Movement->InitialSpeed=0;
    // A dropped grenade starts at zero speed but must still accelerate under
    // gravity; do not cap its subsequent motion to the release speed.
    Movement->MaxSpeed=FMath::Max(Movement->MaxSpeed,Velocity.Size());Movement->Velocity=Velocity;
    Movement->Activate(true);Movement->UpdateComponentVelocity();
    Visual->SetVisibility(true);ForceNetUpdate();
}
void AERProjectile::Bounce(const FHitResult& Result, const FVector& IncomingVelocity)
{
    if (!HasAuthority() || bImpacted || !UsesFuse(Payload)) return;
    ++BounceCount;
    RestNormal = Result.ImpactNormal.GetSafeNormal();
    const float Speed = Movement->Velocity.Size();
    // Small tangent impulses vary the contact without reversing the normal
    // rebound, adding energy, or allowing a client to choose its trajectory.
    const FVector Tangent = FVector::VectorPlaneProject(BounceRandom.VRand(),RestNormal).GetSafeNormal();
    Movement->Velocity = (Movement->Velocity + Tangent*Speed*.07f).GetSafeNormal()*Speed*BounceRandom.FRandRange(.92f,1.f);
    Spin = Spin*.6f + FVector::CrossProduct(RestNormal,IncomingVelocity)*.012f + BounceRandom.VRand()*2.f;
    Spin = Spin.GetClampedToMaxSize(22.f);
    const FQuat Floor = FQuat::FindBetweenNormals(FVector::UpVector,RestNormal);
    RestRotation = Floor * FRotator(BounceRandom.FRandRange(-12.f,12.f),BounceRandom.FRandRange(-180.f,180.f),BounceRandom.FRandRange(78.f,102.f)).Quaternion();
    bResting = RestNormal.Z>.45f && Speed<75.f;
    ForceNetUpdate();
}
void AERProjectile::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if (bImpacted) return;
    if(bHeld)
    {
        auto* Character=Cast<AERCharacter>(GetInstigator());
        if(!Character || Character->Health->bDead) ReleaseHeld(GetActorLocation(),FVector::ZeroVector);
        else SetActorLocation(SafeHandPosition(Character,true,this),false,nullptr,ETeleportType::TeleportPhysics);
        PreviousSmokePosition=GetActorLocation();
        return;
    }
    // Local cosmetic wake follows replicated ordnance; no extra RPCs or drag.
    if(GetWorld()->GetTimeSeconds()>=NextSmokeInteraction)
    {
        if((GetActorLocation()-PreviousSmokePosition).SizeSquared()>4.f)
            for(TActorIterator<AERSmokeCloud> It(GetWorld());It;++It)
                It->AddShot(PreviousSmokePosition,GetActorLocation(),UsesFuse(Payload)?25.f:12.f,UsesFuse(Payload)?1.5f:2.f);
        PreviousSmokePosition=GetActorLocation();NextSmokeInteraction=GetWorld()->GetTimeSeconds()+.1;
    }
    if (bResting || (!Movement->IsActive() && BounceCount>0))
        SetActorRotation(FQuat::Slerp(GetActorQuat(),RestRotation,1.f-FMath::Exp(-DeltaSeconds*9.f)));
    else if (!Spin.IsNearlyZero())
        SetActorRotation((FQuat(Spin.GetSafeNormal(),Spin.Size()*DeltaSeconds)*GetActorQuat()).GetNormalized());
}
void AERProjectile::Hit(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, FVector Impulse, const FHitResult& Result)
{
    if (!HasAuthority() || bImpacted || Other == GetInstigator() || Other == GetOwner()) return;
    if (UsesFuse(Payload)) return;
    if (Payload == EERPayload::Rocket) { Detonate(); return; }
    const FVector ShotDirection = Movement->Velocity.GetSafeNormal();
    bImpacted = true; Movement->StopMovementImmediately(); Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    UGameplayStatics::ApplyPointDamage(Other, Damage, ShotDirection, Result, GetInstigatorController(), this, UDamageType::StaticClass());
    if (Other && !Other->IsA<APawn>())
        MulticastImpact(Result.ImpactPoint, Result.ImpactNormal, FMath::FRandRange(0.f, 360.f));
    SetLifeSpan(0.1f);
}
void AERProjectile::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME(AERProjectile, Payload); DOREPLIFETIME(AERProjectile, ThrownMesh);
    DOREPLIFETIME(AERProjectile,bHeld);DOREPLIFETIME(AERProjectile,FuseEndServerTime);
}
float AERProjectile::TakeDamage(float Amount,const FDamageEvent& Event,AController* Instigator,AActor* Causer)
{
    const auto* Gun=Cast<AERWeapon>(Causer);const auto* Round=Cast<AERProjectile>(Causer);
    const bool Bullet=(Gun && Gun->Data && Gun->Data->Payload==EERPayload::Bullet) || (Round && Round->Payload==EERPayload::Bullet);
    if(!HasAuthority() || bImpacted || !UsesFuse(Payload) || Amount<=0 || !Bullet || !Event.IsOfType(FPointDamageEvent::ClassID)) return 0;
    UE_LOG(LogTemp,Display,TEXT("ER_GRENADE_SHOT_DETONATE held=%d kind=%d"),bHeld,int32(Payload));
    Detonate();return Amount;
}
void AERProjectile::Detonate()
{
    if (!HasAuthority() || bImpacted) return;
    if(auto* Gun=Cast<AERWeapon>(GetOwner())) Gun->OnGrenadeDetonated(this);
    bHeld=false;
    bImpacted = true; Movement->StopMovementImmediately(); Movement->Deactivate();
    ShotTarget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    const FVector Position = GetActorLocation();
    // Snapshot exposure before character deaths/drop callbacks or block removal.
    // Queue each live grenade once; chained blasts never recurse on this stack.
    if(Damage>0 && (Payload==EERPayload::HE || Payload==EERPayload::LaunchedHE || Payload==EERPayload::Rocket))
    {
        FCollisionQueryParams Query(SCENE_QUERY_STAT(ERGrenadeChain),false,this);
        for(TActorIterator<AERProjectile> It(GetWorld());It;++It)
        {
            auto* Other=*It;
            if(Other==this || Other->bImpacted || Other->bChainPending || !UsesFuse(Other->Payload)) continue;
            const float Distance=FVector::Dist(Position,Other->GetActorLocation());
            if(Distance>=ExplosionRadius || (Payload!=EERPayload::Rocket &&
                ERWeaponBallistics::GrenadeDamage(Damage,Distance,ExplosionInnerRadius,ExplosionRadius)<=0)) continue;
            FHitResult Cover;
            if(GetWorld()->LineTraceSingleByObjectType(Cover,Position+FVector::UpVector*3.f,Other->GetActorLocation(),
                FCollisionObjectQueryParams(ECC_WorldStatic),Query)) continue;
            Other->bChainPending=true;
            UE_LOG(LogTemp,Display,TEXT("ER_GRENADE_CHAIN_QUEUED held=%d distance=%.1f"),Other->bHeld,Distance);
            const TWeakObjectPtr<AERProjectile> Pending=Other;
            GetWorldTimerManager().SetTimerForNextTick([Pending]()
            {
                if(auto* Grenade=Pending.Get()) { Grenade->bChainPending=false;Grenade->Detonate(); }
            });
        }
    }
    for (TActorIterator<AERCharacter> It(GetWorld()); It; ++It)
    {
        AERCharacter* Target = *It;
        if (Target->Health->bDead) continue;
        const float Distance = FVector::Dist(Position, Target->GetActorLocation());
        if (Distance > ExplosionRadius) continue;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(ERExplosion), false, this);
        Query.AddIgnoredActor(Target);
        float Exposure=0;
        const bool Fragmentation=Payload==EERPayload::HE || Payload==EERPayload::LaunchedHE;
        const TArray<FVector> Samples=Fragmentation?TArray<FVector>{Target->Camera->GetComponentLocation(),Target->GetActorLocation(),
            Target->GetActorLocation()-FVector(0,0,Target->GetCapsuleComponent()->GetScaledCapsuleHalfHeight()*.55f)}:
            TArray<FVector>{Target->Camera->GetComponentLocation()};
        for(const FVector& Sample:Samples)
        {
            FHitResult Cover;
            if(!GetWorld()->LineTraceSingleByObjectType(Cover,Position+FVector::UpVector*3.f,Sample,
                FCollisionObjectQueryParams(ECC_WorldStatic),Query)) Exposure+=1.f/Samples.Num();
        }
        if(Exposure<=0) continue;
        if (Payload == EERPayload::HE || Payload == EERPayload::Rocket || Payload == EERPayload::LaunchedHE)
        {
            const FVector Direction = (Target->GetActorLocation()-Position).GetSafeNormal();
            const FHitResult AreaHit(Target,Target->GetCapsuleComponent(),Target->GetActorLocation(),-Direction);
            const float AreaDamage=Fragmentation?ERWeaponBallistics::GrenadeDamage(Damage,Distance,ExplosionInnerRadius,ExplosionRadius):
                Damage*FMath::Clamp(1.f-Distance/ExplosionRadius,.15f,1.f);
            UGameplayStatics::ApplyPointDamage(Target, AreaDamage*Exposure, Direction, AreaHit,
                GetInstigatorController(), this, UDamageType::StaticClass());
        }
        else if (Payload == EERPayload::Stun)
            if (auto* PC = Cast<AERPlayerController>(Target->Controller))
            {
                const float Facing = FVector::DotProduct(PC->GetControlRotation().Vector(), (Position-Target->Camera->GetComponentLocation()).GetSafeNormal());
                PC->ClientFlash(Facing > .3f ? .9f : .3f);
            }
    }
    // Player cover checks above see the intact walls. Block exposure is also
    // snapshotted before removal, including cover across multiple room actors.
    if (Payload == EERPayload::HE || Payload == EERPayload::Rocket || Payload == EERPayload::LaunchedHE)
        // Fragment injury range is larger than the local structural blast.
        // Preserve the shared snapshot and the tuned building damage envelope.
        AERBlockRoom::ApplyWorldExplosion(GetWorld(), Position + FVector::UpVector * 10,
            Payload==EERPayload::Rocket?ExplosionRadius:FMath::Min(ExplosionRadius,350.f), Damage, this);
    MulticastDetonate(Position, Payload);
    if (Payload==EERPayload::Smoke && SmokeCloudClass)
    {
        TArray<AERSmokeCloud*> Clouds;
        for(TActorIterator<AERSmokeCloud> It(GetWorld());It;++It) Clouds.Add(*It);
        Clouds.Sort([](const AERSmokeCloud& A,const AERSmokeCloud& B){return A.StartedAtServerTime<B.StartedAtServerTime;});
        while(Clouds.Num()>=4) { Clouds[0]->Destroy(); Clouds.RemoveAt(0); }
        const FTransform Transform(FRotator::ZeroRotator,Position);
        auto* Cloud=GetWorld()->SpawnActorDeferred<AERSmokeCloud>(SmokeCloudClass,Transform,nullptr,nullptr,ESpawnActorCollisionHandlingMethod::AlwaysSpawn);
        if(Cloud)
        {
            Cloud->SmokeColor=SmokeTeamColor;
            UGameplayStatics::FinishSpawningActor(Cloud,Transform);
        }
    }
    ForceNetUpdate();
    SetLifeSpan(Payload == EERPayload::Smoke && !SmokeCloudClass ? 12.f : .3f);
    UE_LOG(LogTemp, Display, TEXT("ER_PAYLOAD_DETONATE kind=%d position=%s"), int32(Payload), *Position.ToString());
}
void AERProjectile::MulticastDetonate_Implementation(FVector_NetQuantize Position, EERPayload Kind)
{
    bImpacted=true;bHeld=false;
    ShotTarget->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    if(Kind==EERPayload::HE || Kind==EERPayload::LaunchedHE || Kind==EERPayload::Rocket)
        for(TActorIterator<AERSmokeCloud> It(GetWorld());It;++It)
            It->AddShot(FVector(Position)-FVector(0,0,60),FVector(Position)+FVector(0,0,60),400.f,12.f);
    Visual->SetVisibility(false);
    if(Kind==EERPayload::Smoke && SmokeCloudClass) { Effect->SetVisibility(false); return; }
    if ((Kind == EERPayload::HE || Kind == EERPayload::LaunchedHE) && GrenadeExplosionClass)
    {
        Effect->SetVisibility(false);
        if (GetNetMode() != NM_DedicatedServer)
        {
            // A separate local actor outlives the projectile's 300ms cleanup.
            // It has no collision, replication or gameplay damage of its own.
            TArray<AERGrenadeExplosion*> Bursts;
            for (TActorIterator<AERGrenadeExplosion> It(GetWorld()); It; ++It) Bursts.Add(*It);
            Bursts.Sort([](const AERGrenadeExplosion& A, const AERGrenadeExplosion& B) { return A.GetGameTimeSinceCreation() > B.GetGameTimeSinceCreation(); });
            while (Bursts.Num() >= 8) { Bursts[0]->Destroy(); Bursts.RemoveAt(0); }
            GetWorld()->SpawnActor<AERGrenadeExplosion>(GrenadeExplosionClass, FVector(Position), FRotator::ZeroRotator);
        }
        return;
    }
    Effect->SetWorldLocation(FVector(Position) + (Kind == EERPayload::Smoke ? FVector(0,0,160) : FVector::ZeroVector));
    Effect->SetMaterial(0, Kind == EERPayload::Smoke ? SmokeMaterial : ExplosionMaterial);
    Effect->SetWorldScale3D(FVector(Kind == EERPayload::Smoke ? 6.f : 1.5f));
    Effect->SetVisibility(true);
}
void AERProjectile::MulticastImpact_Implementation(FVector_NetQuantize Position, FVector_NetQuantizeNormal Normal, float Roll)
{
    SpawnBulletHole(GetWorld(), ImpactDecalMaterial, Position, Normal, Roll);
}
