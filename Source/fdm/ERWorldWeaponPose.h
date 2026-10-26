#pragma once
#include "CoreMinimal.h"
class UERWeaponData;
struct FERTeamAppearance;
// Full-size world presentation, independent of the fitted first-person rig.
namespace ERWorldWeaponPose
{
    FTransform Attachment(const UERWeaponData& Data,float ParentScale);
    FVector SupportContact(const UERWeaponData& Data,const FERTeamAppearance& Appearance);
}
