// Project extension for UE 5.8.3. Included inside the Connect-only Login path.
if (AccountCredentials.Type == TEXT("connect:DeviceIdAccessToken"))
{
    LocalUser.RRDeviceIdDisplayName = AccountCredentials.Id.IsEmpty() ? TEXT("Player") : AccountCredentials.Id;
    EOS_Connect_CreateDeviceIdOptions DeviceOptions = {};
    DeviceOptions.ApiVersion = EOS_CONNECT_CREATEDEVICEID_API_LATEST;
    DeviceOptions.DeviceModel = "Respawn & Rage desktop";
    using FRRCreateDeviceCallback = TEOSCallback<EOS_Connect_OnCreateDeviceIdCallback, EOS_Connect_CreateDeviceIdCallbackInfo, FUserManagerEOS>;
    auto* DeviceCallback = new FRRCreateDeviceCallback(AsWeak());
    DeviceCallback->CallbackLambda = [this, LocalUserNum, AccountCredentials](const EOS_Connect_CreateDeviceIdCallbackInfo* Data)
    {
        if (Data->ResultCode == EOS_EResult::EOS_Success || Data->ResultCode == EOS_EResult::EOS_DuplicateNotAllowed)
        {
            UE_LOG_ONLINE(Display, TEXT("ER_EOS_GUEST_DEVICE %s"), Data->ResultCode == EOS_EResult::EOS_Success ? TEXT("created") : TEXT("reused"));
            CallEOSConnectLogin(LocalUserNum, AccountCredentials);
        }
        else
        {
            const FString Error = FString::Printf(TEXT("Device ID initialization failed (%s)"), *LexToString(Data->ResultCode));
            RemoveLocalUser(LocalUserNum);
            TriggerOnLoginCompleteDelegates(LocalUserNum, false, *FUniqueNetIdEOS::EmptyId(), Error);
        }
    };
    EOS_Connect_CreateDeviceId(EOSSubsystem->ConnectHandle, &DeviceOptions, DeviceCallback, DeviceCallback->GetCallbackPtr());
    return true;
}
