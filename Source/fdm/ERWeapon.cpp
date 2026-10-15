#include "ERWeapon.h"
#include "ERShotEffects.h"
#include "ERMovement.h"
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
void AERWeapon::StartFire()
{
    if (!CanFight() || bReloading || MagazineAmmo <= 0 || bWantsFire) return;
    bWantsFire = true; TryFire();
}
void AERWeapon::StopFire() { bWantsFire = false; GetWorldTimerManager().ClearTimer(FireTimer); }
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
    --MagazineAmmo; NextAllowedShotTime = Now + Data->ShotInterval; Character->Health->ProtectionEndServerTime = 0;
    // Only an authorized shot changes burst buildup. Owners recover it against the server clock.
    BloomAtLastShot = FMath::Min(GetBloomDegrees() + Data->BloomPerShotDegrees, Data->MaxBloomDegrees);
    LastSpreadShotServerTime = Now;
    ForceNetUpdate();
    ++AuthorizedShotCount;
    LastShotTraceCount = 0;
    if (Data->Payload == EERPayload::Bullet)
    {
        const int32 PelletCount = FMath::Clamp(Data->Pellets, 1, 16);
        FVector FeedbackEnd = IntendedMuzzle;
        for (int32 Pellet = 0; Pellet < PelletCount; ++Pellet)
        {
            const FVector Direction = Pellet == 0 ? LastShotDirection : ShotRandom.VRandCone(LastShotAimDirection, FMath::DegreesToRadians(LastShotSpreadDegrees));
            FHitResult Hit = MuzzleHit;
            const FVector TraceEnd = IntendedMuzzle + Direction * FMath::Max(1.f, Data->HitscanRange);
            const bool bHit = bBlocked || GetWorld()->LineTraceSingleByChannel(Hit, IntendedMuzzle, TraceEnd, ECC_GameTraceChannel1, Params);
            ++LastShotTraceCount;
            if (Pellet == 0)
            {
                LastShotTraceStart = bBlocked ? Eye : IntendedMuzzle;
                LastShotTraceEnd = FeedbackEnd = bHit ? Hit.ImpactPoint : TraceEnd;
            }
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
            Projectile->ThrownMesh = Data->bOneHanded ? Data->Mesh : nullptr;
            Projectile->ExplosionRadius = Data->ExplosionRadius;
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
    if (!CanFight() || bReloading || MagazineAmmo >= Data->MagazineSize) return;
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
void AERWeapon::Cancel() { StopFire(); GetWorldTimerManager().ClearTimer(ReloadTimer); bReloading = false; }
void AERWeapon::EndPlay(const EEndPlayReason::Type Reason) { Cancel(); Super::EndPlay(Reason); }
void AERWeapon::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);
    DOREPLIFETIME_CONDITION(AERWeapon, MagazineAmmo, COND_OwnerOnly); DOREPLIFETIME_CONDITION(AERWeapon, ReserveAmmo, COND_OwnerOnly); DOREPLIFETIME(AERWeapon, bReloading);
    DOREPLIFETIME_CONDITION(AERWeapon, BloomAtLastShot, COND_OwnerOnly);
    DOREPLIFETIME_CONDITION(AERWeapon, LastSpreadShotServerTime, COND_OwnerOnly);
    DOREPLIFETIME(AERWeapon, Data);
}
AERProjectile::AERProjectile()
{
    bReplicates = true; SetReplicateMovement(true);
    Collision = CreateDefaultSubobject<USphereComponent>(TEXT("Collision"));
    SetRootComponent(Collision); Collision->InitSphereRadius(3);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    Collision->SetCollisionObjectType(ECC_GameTraceChannel1);
    Collision->SetCollisionResponseToAllChannels(ECR_Ignore);
    Collision->SetCollisionResponseToChannel(ECC_WorldStatic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_WorldDynamic, ECR_Block);
    Collision->SetCollisionResponseToChannel(ECC_Pawn, ECR_Block);
    Collision->OnComponentHit.AddDynamic(this, &AERProjectile::Hit);
    Movement = CreateDefaultSubobject<UProjectileMovementComponent>(TEXT("Movement"));
    Movement->SetUpdatedComponent(Collision); Movement->InitialSpeed = Movement->MaxSpeed = 30000;
    Movement->bInitialVelocityInLocalSpace = false;
    Movement->ProjectileGravityScale = 0; Movement->bShouldBounce = false;
    Movement->bForceSubStepping = true;
    Visual = CreateDefaultSubobject<USkeletalMeshComponent>(TEXT("Visual"));
    Visual->SetupAttachment(Collision); Visual->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Effect = CreateDefaultSubobject<UStaticMeshComponent>(TEXT("Effect"));
    Effect->SetupAttachment(Collision); Effect->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Effect->SetCastShadow(false); Effect->SetVisibility(false);
    InitialLifeSpan = 2;
}
void AERProjectile::BeginPlay()
{
    Super::BeginPlay();
    Visual->SetSkeletalMeshAsset(ThrownMesh);
    Effect->SetStaticMesh(EffectMesh);
    if (Payload == EERPayload::Rocket)
    {
        Effect->SetMaterial(0, ExplosionMaterial);
        Effect->SetRelativeScale3D(FVector(.08f)); Effect->SetVisibility(true);
    }
    if (Payload == EERPayload::HE || Payload == EERPayload::Smoke || Payload == EERPayload::Stun)
    {
        Movement->ProjectileGravityScale = 1;
        Movement->bShouldBounce = true; Movement->Bounciness = .35f;
        if (HasAuthority()) GetWorldTimerManager().SetTimer(FuseTimer, this, &AERProjectile::Detonate, FuseDuration, false);
    }
    if (HasAuthority()) { Collision->IgnoreActorWhenMoving(GetInstigator(), true); Collision->IgnoreActorWhenMoving(GetOwner(), true); }
    else { Movement->Deactivate(); Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision); }
}
void AERProjectile::Hit(UPrimitiveComponent* Component, AActor* Other, UPrimitiveComponent* OtherComponent, FVector Impulse, const FHitResult& Result)
{
    if (!HasAuthority() || bImpacted || Other == GetInstigator() || Other == GetOwner()) return;
    if (Payload == EERPayload::HE || Payload == EERPayload::Smoke || Payload == EERPayload::Stun) return;
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
}
void AERProjectile::Detonate()
{
    if (!HasAuthority() || bImpacted) return;
    bImpacted = true; Movement->StopMovementImmediately(); Movement->Deactivate();
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    const FVector Position = GetActorLocation();
    for (TActorIterator<AERCharacter> It(GetWorld()); It; ++It)
    {
        AERCharacter* Target = *It;
        if (Target->Health->bDead) continue;
        const float Distance = FVector::Dist(Position, Target->GetActorLocation());
        if (Distance > ExplosionRadius) continue;
        FHitResult Cover;
        FCollisionQueryParams Query(SCENE_QUERY_STAT(ERExplosion), false, this);
        Query.AddIgnoredActor(Target);
        if (GetWorld()->LineTraceSingleByObjectType(Cover, Position + FVector::UpVector * 10.f, Target->Camera->GetComponentLocation(),
            FCollisionObjectQueryParams(ECC_WorldStatic), Query)) continue;
        if (Payload == EERPayload::HE || Payload == EERPayload::Rocket)
        {
            const FVector Direction = (Target->GetActorLocation()-Position).GetSafeNormal();
            const FHitResult AreaHit(Target,Target->GetCapsuleComponent(),Target->GetActorLocation(),-Direction);
            UGameplayStatics::ApplyPointDamage(Target, Damage * FMath::Clamp(1.f - Distance / ExplosionRadius, .15f, 1.f), Direction, AreaHit,
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
    if (Payload == EERPayload::HE || Payload == EERPayload::Rocket)
        AERBlockRoom::ApplyWorldExplosion(GetWorld(), Position + FVector::UpVector * 10, ExplosionRadius, Damage, this);
    MulticastDetonate(Position, Payload);
    if (Payload==EERPayload::Smoke && SmokeCloudClass)
    {
        TArray<AERSmokeCloud*> Clouds;
        for(TActorIterator<AERSmokeCloud> It(GetWorld());It;++It) Clouds.Add(*It);
        Clouds.Sort([](const AERSmokeCloud& A,const AERSmokeCloud& B){return A.StartedAtServerTime<B.StartedAtServerTime;});
        while(Clouds.Num()>=4) { Clouds[0]->Destroy(); Clouds.RemoveAt(0); }
        GetWorld()->SpawnActor<AERSmokeCloud>(SmokeCloudClass,Position,FRotator::ZeroRotator);
    }
    ForceNetUpdate();
    SetLifeSpan(Payload == EERPayload::Smoke && !SmokeCloudClass ? 12.f : .3f);
    UE_LOG(LogTemp, Display, TEXT("ER_PAYLOAD_DETONATE kind=%d position=%s"), int32(Payload), *Position.ToString());
}
void AERProjectile::MulticastDetonate_Implementation(FVector_NetQuantize Position, EERPayload Kind)
{
    Visual->SetVisibility(false);
    if(Kind==EERPayload::Smoke && SmokeCloudClass) { Effect->SetVisibility(false); return; }
    if (Kind == EERPayload::HE && GrenadeExplosionClass)
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
