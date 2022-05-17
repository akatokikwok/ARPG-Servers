#include "MMOARPGdbClientObject.h"
#include "Log/MMOARPGGateServerLog.h"
#include "Protocol/HallProtocol.h"
#include "SimpleProtocolsDefinition.h"
#include <ServerList.h>

void UMMOARPGdbClientObject::Init()
{
	Super::Init();

}

void UMMOARPGdbClientObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void UMMOARPGdbClientObject::Close()
{
	Super::Close();

}

void UMMOARPGdbClientObject::RecvProtocol(uint32 InProtocol)
{
	Super::RecvProtocol(InProtocol);

	switch (InProtocol)
	{
		case SP_CharacterAppearanceResponses :// 网关服务器将收到的人物形象请求转发至此, 所以是db客户端响应玩家形象请求.
		{
			FString String1;
			FSimpleAddrInfo AddrInfo;
			SIMPLE_PROTOCOLS_RECEIVE(SP_CharacterAppearanceResponses, AddrInfo, String1);// 接收到数据源并存在string里.

			SIMPLE_SERVER_SEND(GateServer, SP_CharacterAppearanceResponses, AddrInfo, String1);// 再将string1里的json数据发回至网关服务器.

			UE_LOG(LogMMOARPGGateServer, Display, TEXT("[SP_CharacterAppearanceResponses], db客户端响应玩家形象请求."));
			break;
		}
	}
}
