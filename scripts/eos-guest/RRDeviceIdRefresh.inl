// Device ID refresh must not request an external token from the NULL subsystem.
const FString& RRGuestName = GetLocalUserChecked(LocalUserNum).RRDeviceIdDisplayName;
if (!RRGuestName.IsEmpty())
{
    EOS_Connect_Credentials GuestCredentials = {};
    GuestCredentials.ApiVersion = EOS_CONNECT_CREDENTIALS_API_LATEST;
    GuestCredentials.Type = EOS_EExternalCredentialType::EOS_ECT_DEVICEID_ACCESS_TOKEN;
    GuestCredentials.Token = nullptr;
    const FTCHARToUTF8 GuestNameUtf8(*RRGuestName);
    EOS_Connect_UserLoginInfo GuestInfo = {};
    GuestInfo.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
    GuestInfo.DisplayName = GuestNameUtf8.Get();
    EOS_Connect_LoginOptions GuestOptions = {};
    GuestOptions.ApiVersion = EOS_CONNECT_LOGIN_API_LATEST;
    GuestOptions.Credentials = &GuestCredentials;
    GuestOptions.UserLoginInfo = &GuestInfo;
    auto* GuestCallback = new FConnectLoginCallback(AsWeak());
    GuestCallback->CallbackLambda = [this, LocalUserNum](const EOS_Connect_LoginCallbackInfo* Result)
    {
        if (Result->ResultCode == EOS_EResult::EOS_Success)
        {
            UE_LOG_ONLINE(Display, TEXT("ER_EOS_GUEST_REFRESH success"));
        }
        else
        {
            UE_LOG_ONLINE(Warning, TEXT("ER_EOS_GUEST_REFRESH failed (%s)"), *LexToString(Result->ResultCode));
            TriggerOnAuthAboutToExpireDelegates(LocalUserNum);
        }
    };
    EOS_Connect_Login(EOSSubsystem->ConnectHandle, &GuestOptions, GuestCallback, GuestCallback->GetCallbackPtr());
    return;
}
