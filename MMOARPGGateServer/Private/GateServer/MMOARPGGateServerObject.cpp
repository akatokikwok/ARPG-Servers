#include "MMOARPGGateServerObject.h"
#include "SimpleMySQLibrary.h"
#include "Log\MMOARPGGateServerLog.h"
#include "Protocol/ServerProtocol.h"
#include "MMOARPGType.h"
#include "Protocol/HallProtocol.h"
#include "SimpleProtocolsDefinition.h"
#include "ServerList.h"

void UMMOARPGGateServerObject::Init()
{
	Super::Init();

}

void UMMOARPGGateServerObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);

}

void UMMOARPGGateServerObject::Close()
{
	Super::Close();

}

void UMMOARPGGateServerObject::RecvProtocol(uint32 InProtocol)
{
	Super::RecvProtocol(InProtocol);

	switch (InProtocol) {
		case SP_GateStatusRequests:
		{
			FMMOARPGGateStatus Status;
			// 只能获取到当前服务器链接到的那个对象的地址(网关的地址).
			GetLocalAddrInfo(Status.GateServerAddrInfo);// 使用GetLocalAddrInfo这个API 直接获取网关的地址.

			Status.GateConnectionNum = GetManage()->GetConnetionNum();// 再获取链接数量.

			SIMPLE_PROTOCOLS_SEND(SP_GateStatusResponses, Status);// 本服务器往外发一个 响应协议.

			UE_LOG(LogMMOARPGGateServer, Display, TEXT("SP_GateStatusRequests, 网关状态"));// 网关服务器往外发送请求.
			break;
		}

		case SP_CharacterAppearanceRequests :// 玩家形象Requests 来自 UUI_HallMain::Callback_LinkServerInfo.
		{
			// 先获取来自客户端Hallmain发来的 玩家形象的用户ID.
			int32 PlayerID = INDEX_NONE;
			SIMPLE_PROTOCOLS_RECEIVE(SP_CharacterAppearanceRequests, PlayerID);
			
			// 获取当前本网关 服务器的地址, 并把请求转发到db端.
			FSimpleAddrInfo AddrInfo;
			GetRemoteAddrInfo(AddrInfo);
			SIMPLE_CLIENT_SEND(dbClient, SP_CharacterAppearanceRequests, PlayerID, AddrInfo);

			UE_LOG(LogMMOARPGGateServer, Display, TEXT("[SP_CharacterAppearanceRequests], UserID = %i, 网关收到客户端SP_CharacterAppearanceRequests并转发至db端."), PlayerID);
			break;
		}
	}
}