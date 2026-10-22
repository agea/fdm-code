#include "ERNetDriverEOS.h"

bool UERNetDriverEOS::InitListen(FNetworkNotify* InNotify, FURL& URL, bool Reuse, FString& Error)
{
    if (!URL.HasOption(TEXT("EROnline=1"))) URL.AddOption(TEXT("bUseIPSockets"));
    else if (!Super::IsAvailable()) { Error = TEXT("EOS transport unavailable."); return false; }
    UE_LOG(LogTemp, Display, TEXT("ER_TRANSPORT %s"), URL.HasOption(TEXT("bUseIPSockets")) ? TEXT("LAN/IP") : TEXT("EOS/P2P"));
    return Super::InitListen(InNotify, URL, Reuse, Error);
}
