#include "ERBlockNetPage.h"
#include "ERBlockRoom.h"
#include "Net/UnrealNetwork.h"
AERBlockNetPage::AERBlockNetPage()
{
    bReplicates=true;bAlwaysRelevant=true;PrimaryActorTick.bCanEverTick=false;
    SetNetUpdateFrequency(20);SetMinNetUpdateFrequency(10);
}
void AERBlockNetPage::GetLifetimeReplicatedProps(TArray<FLifetimeProperty>& OutLifetimeProps) const
{
    Super::GetLifetimeReplicatedProps(OutLifetimeProps);DOREPLIFETIME(AERBlockNetPage,Room);DOREPLIFETIME(AERBlockNetPage,State);
}
void AERBlockNetPage::OnRep_State(){if (Room)Room->ReceiveNetworkPage(this);}
