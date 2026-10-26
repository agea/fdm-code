#include "ERWorldWeaponPose.h"
#include "ERWeapon.h"
#include "ERTeamAppearance.h"
FVector ERWorldWeaponPose::SupportContact(const UERWeaponData& Data,const FERTeamAppearance& Appearance)
{
    return Data.bOneHanded ? Data.WorldPistolSupportContact:
        Data.WorldLeftPalmContact+Appearance.WorldSupportPalmOffsetCm/FMath::Max(.01f,Data.WorldWeaponScale);
}
FTransform ERWorldWeaponPose::Attachment(const UERWeaponData& Data,float ParentScale)
{
    const FQuat Rotation=Data.WorldRightPalmRotation.Quaternion().Inverse();
    return FTransform(Rotation,Rotation.RotateVector(-Data.WorldRightPalmContact*Data.WorldWeaponScale)/FMath::Max(.01f,ParentScale),FVector(Data.WorldWeaponScale));
}
