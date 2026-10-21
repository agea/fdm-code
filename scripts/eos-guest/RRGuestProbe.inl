#if !UE_BUILD_SHIPPING
#include "RRGuestEOS.h"
bool RRRequestGuestRefreshForTesting(IOnlineSubsystem* Subsystem)
{
    if (!Subsystem || Subsystem->GetSubsystemName() != FName(TEXT("EOS")) || UEOSSettings::GetSettings().bUseEAS)
        return false;
    const auto Manager = StaticCastSharedPtr<FUserManagerEOS>(Subsystem->GetIdentityInterface());
    if (!Manager.IsValid() || Manager->GetLoginStatus(0) != ELoginStatus::LoggedIn) return false;
    EOS_Connect_AuthExpirationCallbackInfo Info = {};
    Info.LocalUserId = Manager->GetLocalProductUserId(0);
    Manager->AutoRefreshConnectLogin(&Info);
    return true;
}
#endif
