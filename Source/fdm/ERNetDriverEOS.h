#pragma once
#include "CoreMinimal.h"
#include "NetDriverEOS.h"
#include "ERNetDriverEOS.generated.h"

// UE 5.8 chooses EOS P2P for every listen server by default. Keep ordinary
// launches and the existing LAN probes on IP unless explicitly requested.
UCLASS(Transient, Config=Engine)
class FDM_API UERNetDriverEOS : public UNetDriverEOS
{
    GENERATED_BODY()
public:
    virtual bool IsAvailable() const override { return true; }
    virtual bool InitListen(FNetworkNotify* Notify, FURL& URL, bool Reuse, FString& Error) override;
};
