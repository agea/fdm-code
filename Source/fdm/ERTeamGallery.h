#pragma once
#include "CoreMinimal.h"
#include "GameFramework/Actor.h"
#include "ERTeamGallery.generated.h"

// Explicit local art review only; never spawned in an ordinary match.
UCLASS()
class FDM_API AERTeamGallery : public AActor
{
    GENERATED_BODY()
public:
    AERTeamGallery();
    virtual void Tick(float DeltaTime) override;
private:
    void TickProtection(class AERCharacter* Player, class APlayerController* PC);
    void TickProtectionRespawn(class AERCharacter* Player, class APlayerController* PC);
    float ProtectionRespawnSeenAt = 0;
    void TickCharacterShots(class AERCharacter* Player, class APlayerController* PC);
    int32 ShotIndex = 0;
    float NextShot = 0;
    float Age = 0;
    int32 Stage = 0;
    UPROPERTY() TArray<TObjectPtr<class AERCorpse>> Corpses;
    UPROPERTY() TObjectPtr<class ACameraActor> GalleryCamera;
    UPROPERTY() TArray<TObjectPtr<class ASkeletalMeshActor>> SkinActors;
    void TickPosture(class AERCharacter* Player, class APlayerController* PC);
    void TickSkins(class AERCharacter* Player, class APlayerController* PC);
    void TickForearms(class AERCharacter* Player, class APlayerController* PC);
    float NextForearmCapture = 0.f;
    bool bForearmChangePending = false;
    FTransform ForearmReferenceRifle;
    TArray<FTransform> ForearmReferenceBones;
};
