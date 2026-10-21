#pragma once
#include "CoreMinimal.h"
#include "Containers/Ticker.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "EROnlineProbe.generated.h"

// Explicitly opted-in real-service integration probe; survives map travel.
UCLASS()
class UEROnlineProbe : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    virtual void Initialize(FSubsystemCollectionBase& Collection) override;
    virtual void Deinitialize() override;
private:
    FTSTicker::FDelegateHandle Ticker;
    IOnlineSessionPtr Sessions;
    TSharedPtr<FOnlineSessionSearch> Query;
    FDelegateHandle FindHandle;
    FString SessionId;
    int32 Phase = 0;
    double Deadline = 0, SearchAgainAt = 0;
    bool bFound = false, bSearchRunning = false;
    bool Tick(float Delta);
    void FindComplete(bool Success);
    void Check(bool Value, const TCHAR* Label);
    bool bFailed = false;
};
