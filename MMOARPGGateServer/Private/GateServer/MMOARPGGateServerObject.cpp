#include "MMOARPGGateServerObject.h"
#include "SimpleMySQLibrary.h"
#include "Log\MMOARPGGateServerLog.h"
#include "Protocol/ServerProtocol.h"
#include "MMOARPGType.h"
#include "Protocol/HallProtocol.h"
#include "SimpleProtocolsDefinition.h"
#include "ServerList.h"

UMMOARPGGateServerObject::UMMOARPGGateServerObject()
	: UserID(INDEX_NONE)
{

}

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
	if (UserID != INDEX_NONE) {
		SIMPLE_CLIENT_SEND(CenterClient, SP_PlayerQuitRequests, UserID);// 网关关闭的时候发送玩家退出协议.
		UE_LOG(LogMMOARPGGateServer, Display, TEXT("[SP_PlayerQuitRequests] UserID = %i"), UserID);
	}
	else {
		UE_LOG(LogMMOARPGGateServer, Error, TEXT("[SP_PlayerQuitRequests] Error UserID = %i"), UserID);
	}
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

			UE_LOG(LogMMOARPGGateServer, Display, TEXT("SP_GateStatusRequests, Gate State"));// 网关服务器往外发送请求.
			break;
		}

		/** 玩家造型请求. */
		case SP_CharacterAppearanceRequests:// 玩家形象Requests 来自 UUI_HallMain::Callback_LinkServerInfo.
		{
			// 先获取来自客户端Hallmain发来的 玩家形象的用户ID.
			int32 PlayerID = INDEX_NONE;
			SIMPLE_PROTOCOLS_RECEIVE(SP_CharacterAppearanceRequests, PlayerID);

			UserID = PlayerID;// 注册用户ID;

			// 获取当前本网关 服务器的地址, 并把请求转发到db端.
			FSimpleAddrInfo AddrInfo;
			GetRemoteAddrInfo(AddrInfo);
			SIMPLE_CLIENT_SEND(dbClient, SP_CharacterAppearanceRequests, PlayerID, AddrInfo);

			UE_LOG(LogMMOARPGGateServer, Display, TEXT("[SP_CharacterAppearanceRequests], UserID = %i"), PlayerID);
			break;
		}

		/** 核验角色命名请求. */
		case SP_CheckCharacterNameRequests:// 来自 UUI_RenameCreate::Callback_ClickedFindName()
		{
			int32 PlayerID = INDEX_NONE;// 玩家ID.
			FString CharacterName;// 玩家名字.
			// 读一下来自客户端请求的 玩家ID和玩家名字.
			SIMPLE_PROTOCOLS_RECEIVE(SP_CheckCharacterNameRequests, PlayerID, CharacterName);

			// 拿到本网关地址并转发数据至 db端. dbClient端会做一个接收.
			FSimpleAddrInfo AddrInfo;
			GetRemoteAddrInfo(AddrInfo);
			SIMPLE_CLIENT_SEND(dbClient, SP_CheckCharacterNameRequests, PlayerID, CharacterName, AddrInfo);

			// Print.
			UE_LOG(LogMMOARPGGateServer, Display, TEXT("[SP_CheckCharacterNameRequests], UserID = %i"), PlayerID);
			break;
		}

		/** 创建舞台人物请求. */
		case SP_CreateCharacterRequests :// 来自 UUI_RenameCreate::ClickedCreate_callback()
		{
			int32 PlayerID = INDEX_NONE;// 玩家ID.
			FString JsonString_CA;// UUI_HallMain::CreateCharacter里的CA存档压缩成的Json.

			// 读一下收到的来自客户端请求的 玩家ID
			SIMPLE_PROTOCOLS_RECEIVE(SP_CreateCharacterRequests, PlayerID, JsonString_CA);

			// 拿到本网关地址并转发数据至 db端. dbClient端会做一个接收.
			FSimpleAddrInfo AddrInfo;
			GetRemoteAddrInfo(AddrInfo);
			SIMPLE_CLIENT_SEND(dbClient, SP_CreateCharacterRequests, PlayerID, JsonString_CA, AddrInfo);
			// Print.
			UE_LOG(LogMMOARPGGateServer, Display, TEXT("[SP_CreateCharacterRequests] UserID = %i"), PlayerID);
			break;
		}

		/** 删除CA存档. */
		case SP_DeleteCharacterRequests:
		{
			int32 PlayerID = INDEX_NONE;
			int32 SlotID = INDEX_NONE;// 是具体的哪个槽号的人物.
			SIMPLE_PROTOCOLS_RECEIVE(SP_DeleteCharacterRequests, PlayerID, SlotID);

			FSimpleAddrInfo AddrInfo;
			GetRemoteAddrInfo(AddrInfo);
			SIMPLE_CLIENT_SEND(dbClient, SP_DeleteCharacterRequests, PlayerID, SlotID, AddrInfo);

			UE_LOG(LogMMOARPGGateServer, Display, TEXT("[SP_DeleteCharacterRequests] UserID = %i SlotID=%i"), PlayerID, SlotID);
			break;
		}

		/** 跳转至DS. */
		case SP_LoginToDSServerRequests:
		{
			int32 PlayerID = INDEX_NONE;
			int32 SlotID = INDEX_NONE;// 是具体的哪个槽号的人物.
			SIMPLE_PROTOCOLS_RECEIVE(SP_LoginToDSServerRequests, PlayerID, SlotID);

			FSimpleAddrInfo AddrInfo;
			GetRemoteAddrInfo(AddrInfo);
			SIMPLE_CLIENT_SEND(CenterClient, SP_LoginToDSServerRequests, PlayerID, SlotID, AddrInfo);

			UE_LOG(LogMMOARPGGateServer, Display, TEXT("[SP_LoginToDSServerRequests] UserID = %i SlotID = %i"), PlayerID, SlotID);
			break;
		}
	}
}