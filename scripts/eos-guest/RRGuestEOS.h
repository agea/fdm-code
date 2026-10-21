#pragma once

// Exercises the same handler as EOS auth expiration, without waiting an hour.
// Only available in development builds; never creates/deletes a Device ID.
class IOnlineSubsystem;
#if !UE_BUILD_SHIPPING
ONLINESUBSYSTEMEOS_API bool RRRequestGuestRefreshForTesting(IOnlineSubsystem* Subsystem);
#endif
