#include "ERPlayerController.h"
#include "ERMatch.h"
#include "Misc/AutomationTest.h"

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERAddressTests,"EclipseRecoil.Menu.IPv4Validation",EAutomationTestFlags_ApplicationContextMask | EAutomationTestFlags::EngineFilter)
bool FERAddressTests::RunTest(const FString& Parameters)
{
    for (const FString Address : {TEXT("127.0.0.1"), TEXT("192.168.1.20:7777"), TEXT("10.0.0.1:1"), TEXT("10.0.0.1:65535")}) TestTrue(*Address,UERGameInstance::IsValidAddress(Address));
    for (const FString Address : {TEXT(""),TEXT("localhost"),TEXT("256.1.1.1"),TEXT("1.2.3"),TEXT("1..3.4"),TEXT("1.2.3.4:"),TEXT("1.2.3.4:0"),TEXT("1.2.3.4:65536"),TEXT("1.2.3.4:7.5"),TEXT("1.2.3.4:7777?listen"),TEXT("1.2.3.4:abc"),TEXT("1.2.3.4:-1")}) TestFalse(*Address,UERGameInstance::IsValidAddress(Address));
    TestEqual(TEXT("Empty player name fallback"),AERGameMode::SanitizeName(TEXT(" ?&<> ")),FString(TEXT("Recruit")));
    TestEqual(TEXT("Name cannot inject travel options"),AERGameMode::SanitizeName(TEXT("Alice?listen&Name=Bob")),FString(TEXT("AlicelistenNameBob")));
    TestTrue(TEXT("Name limited to 24 characters"),AERGameMode::SanitizeName(FString::ChrN(50,TCHAR('a'))).Len()==24);
    return true;
}
#endif
