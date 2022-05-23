#include "MMOARPGdbClientObject.h"
#include "Log/MMOARPGGateServerLog.h"
#include "Protocol/HallProtocol.h"
#include "SimpleProtocolsDefinition.h"
#include <ServerList.h>
#include "MMOARPGType.h"

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

			UE_LOG(LogMMOARPGGateServer, Display, TEXT("[SP_CharacterAppearanceResponses], Gate-dbClient-Response-CharacterAppearance."));
			break;
		}

		/** 角色命名核验之db端的回应: 发给上一层GateServer之后, GateServer再发给客户端. */
		case SP_CheckCharacterNameResponses :
		{
			FSimpleAddrInfo AddrInfo;
			ECheckNameType CheckNameType = ECheckNameType::UNKNOWN_ERROR;// 核验类型.

			SIMPLE_PROTOCOLS_RECEIVE(SP_CheckCharacterNameResponses, CheckNameType, AddrInfo);// 收到来自 db-server-checkCharacterName的Response数据.
			SIMPLE_SERVER_SEND(GateServer, SP_CheckCharacterNameResponses, AddrInfo, CheckNameType);// 将Response数据 转发至 gate-server;

			UE_LOG(LogMMOARPGGateServer, Display, TEXT("[SP_CheckCharacterNameResponses], Gate-dbClient-Response-CheckCharacterName."));
			break;
		}

		/** 创建舞台人物之db-Server端的回应: 发给上一层GateServer之后, GateServer再发给客户端. */
		case SP_CreateCharacterResponses:
		{
			FSimpleAddrInfo AddrInfo;
			SIMPLE_PROTOCOLS_RECEIVE(SP_CreateCharacterResponses, AddrInfo);// 收到来自 db-server-CreateCharacter的Response数据.
			SIMPLE_SERVER_SEND(GateServer, SP_CreateCharacterResponses, AddrInfo);// 将Response数据 转发至 gate-server;

			UE_LOG(LogMMOARPGGateServer, Display, TEXT("[SP_CreateCharacterResponses], Gate-dbClient-Response-CreateCharacter."));
			break;
		}
	}
}
