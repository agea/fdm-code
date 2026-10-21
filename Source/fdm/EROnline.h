#pragma once
#include "CoreMinimal.h"
#include "Subsystems/GameInstanceSubsystem.h"
#include "Interfaces/OnlineIdentityInterface.h"
#include "Interfaces/OnlineSessionInterface.h"
#include "OnlineSessionSettings.h"
#include "EROnline.generated.h"

// EOS is explicitly selected here. Offline/LAN never require a cloud login.
UCLASS()
class FDM_API UEROnline : public UGameInstanceSubsystem
{
    GENERATED_BODY()
public:
    void Host();
    void Search();
    void Join(int32 Index);
    void Cancel();
    void Leave();
    void RegisterPlayer(APlayerController* Player);
    void UnregisterPlayer(AController* Player);
    bool IsBusy() const { return Operation != EOperation::None; }
    bool IsOnline() const { return bOnline; }
    bool HasSession() const { return Sessions.IsValid() && Sessions->GetNamedSession(NAME_GameSession); }
    FString Status;
    TArray<FOnlineSessionSearchResult> Results;
    virtual void Deinitialize() override;
private:
    enum class EOperation : uint8 { None, Login, Create, Find, Join, Destroy };
    EOperation Operation = EOperation::None;
    bool bCancelled = false;
    bool bOnline = false;
    bool bHostAfterLogin = false;
    FName EOSInstanceIdentifier;
    bool bOwnEOSInstance = false;
    IOnlineIdentityPtr Identity;
    IOnlineSessionPtr Sessions;
    TSharedPtr<FOnlineSessionSearch> Query;
    FDelegateHandle LoginHandle, CreateHandle, FindHandle, JoinHandle, DestroyHandle;
    bool Configure();
    void Begin(bool Hosting);
    void Continue();
    void LoginComplete(int32 User, bool Success, const FUniqueNetId& Id, const FString& Error);
    void CreateComplete(FName Name, bool Success);
    void FindComplete(bool Success);
    void JoinComplete(FName Name, EOnJoinSessionCompleteResult::Type Result);
    void DestroyComplete(FName Name, bool Success);
    void Destroy();
    void Fail(const FString& Message);
};
