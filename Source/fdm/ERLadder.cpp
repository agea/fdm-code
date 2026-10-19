#include "ERLadder.h"
#include "Components/SceneComponent.h"
#include "Engine/World.h"
AERLadder::AERLadder()
{
    RootComponent=CreateDefaultSubobject<USceneComponent>(TEXT("LadderFrame"));
    SetReplicateMovement(false);
}
FVector AERLadder::ClimbPosition(float FeetZ,float HalfHeight) const
{return GetActorTransform().TransformPosition({55,0,FeetZ+HalfHeight});}
FVector AERLadder::Landing(float HalfHeight) const
{return GetActorTransform().TransformPosition({-100,0,DeckHeight+HalfHeight+3});}
bool AERLadder::HasRung(float FeetZ) const
{
    FHitResult Hit;FCollisionQueryParams Params(SCENE_QUERY_STAT(ERLadderRung),false,this);
    const FTransform Frame=GetActorTransform();
    auto MetalAt=[&](float Y,float Z)
    {return GetWorld()->LineTraceSingleByChannel(Hit,Frame.TransformPosition({15,Y,Z}),Frame.TransformPosition({-15,Y,Z}),ECC_Visibility,Params);};
    if(FeetZ+60>=DeckHeight)
    {
        const float Z=FMath::Clamp(FeetZ+60,25.f,Height-5);
        return MetalAt(-39,Z) && MetalAt(39,Z);
    }
    const float Z=FMath::Clamp(FMath::RoundToFloat((FeetZ+60)/25)*25,25.f,FMath::FloorToFloat((DeckHeight-1)/25)*25);
    return MetalAt(0,Z);
}
