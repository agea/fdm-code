// Device ID requires a display name and a null external token on desktop too.
EOS_Connect_UserLoginInfo RRDeviceInfo = {};
const FTCHARToUTF8 RRDisplayName(*GetLocalUserChecked(LocalUserNum).RRDeviceIdDisplayName);
if (ExternalCredentialType == EOS_EExternalCredentialType::EOS_ECT_DEVICEID_ACCESS_TOKEN)
{
    ConnectCredentials.Token = nullptr;
    RRDeviceInfo.ApiVersion = EOS_CONNECT_USERLOGININFO_API_LATEST;
    RRDeviceInfo.DisplayName = RRDisplayName.Get();
    Options.UserLoginInfo = &RRDeviceInfo;
}
