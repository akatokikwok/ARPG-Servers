#include "MMOARPGLoginServerObject.h"
#include "SimpleMySQLibrary.h"
#include "Log\MMOARPGLoginServerLog.h"
#include "Protocol/LoginProtocol.h"
#include "../ServerList.h"
#include "SimpleProtocolsDefinition.h"

void UMMOARPGLoginServerObejct::Init()
{
	Super::Init();
	
}

void UMMOARPGLoginServerObejct::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void UMMOARPGLoginServerObejct::Close()
{
	Super::Close();

}

void UMMOARPGLoginServerObejct::RecvProtocol(uint32 InProtocol)
{
	Super::RecvProtocol(InProtocol);

	// 根据收到的来自客户端的协议号执行接收.
	switch (InProtocol) {
		case SP_LoginRequests:
		{
			FString AccountString;
			FString PasswordString;
			// 拿到客户端发来的账户和密码.
			SIMPLE_PROTOCOLS_RECEIVE(SP_LoginRequests, AccountString, PasswordString);

			FSimpleAddrInfo AddrInfo;
			GetRemoteAddrInfo(AddrInfo);
			// 借助db客户端来转发登录请求协议, 此时视作是1个客户端.
			SIMPLE_CLIENT_SEND(dbClient, SP_LoginRequests, AccountString, PasswordString, AddrInfo);
			
			UE_LOG(LogMMOARPGLoginServer, Display, TEXT("[SP_LoginRequests] AccountString = %s, PasswordString = %s"), *AccountString, *PasswordString);

			break;
		}
	}

}