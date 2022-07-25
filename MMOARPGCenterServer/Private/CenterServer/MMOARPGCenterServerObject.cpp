// Copyright (C) RenZhai.2020.All Rights Reserved.
#include "MMOARPGCenterServerObject.h"
#include "log/MMOARPGCenterServerLog.h"
#include "Protocol/ServerProtocol.h"
#include "Protocol/HallProtocol.h"
#include "Protocol/GameProtocol.h"
#include "ServerList.h"


TMap<int32, FMMOARPGPlayerRegistInfo> UMMOARPGCenterServerObject::PlayerRegistInfos;
TMap<FSimpleAddr, FMMOARPGDicatedServerInfo> UMMOARPGCenterServerObject::DicatedServerInfos;

UMMOARPGCenterServerObject::UMMOARPGCenterServerObject()
	: LinkType(ECentralServerLinkType::GAME_PLAYER_LINK)
{

}

void UMMOARPGCenterServerObject::Init()
{
	Super::Init();

	// 缓存池里没元素才执行.
	if (!PlayerRegistInfos.Num()) {
		// 主动为缓存池手动分配内存.
		for (int32 i = 0; i < 2000; i++) {
			PlayerRegistInfos.Add(i, FMMOARPGPlayerRegistInfo());
		}
	}
}

void UMMOARPGCenterServerObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
}

void UMMOARPGCenterServerObject::Close()
{
	Super::Close();
	// 断开或超时检测.
	if (GetLinkType() == ECentralServerLinkType::GAME_DEDICATED_SERVER_LINK) {
		RemoveDicatedServerInfo(DicatedServerKey);

		UE_LOG(LogMMOARPGCenterServer, Error, TEXT("[%s]A dedicated server has lost its link."),
			*FSimpleNetManage::GetAddrString(DicatedServerKey));
	}
}

void UMMOARPGCenterServerObject::RecvProtocol(uint32 InProtocol)
{
	Super::RecvProtocol(InProtocol);

	switch (InProtocol) {
		case SP_LoginToDSServerRequests:
		{
			int32 UserID = INDEX_NONE;
			int32 SlotID = INDEX_NONE;
			FSimpleAddrInfo GateAddrInfo;
			SIMPLE_PROTOCOLS_RECEIVE(SP_LoginToDSServerRequests, UserID, SlotID, GateAddrInfo)

				if (UserID != INDEX_NONE && SlotID != INDEX_NONE) {

					FSimpleAddrInfo CenterAddrInfo;// 准备1个中心服务器的地址.
					GetRemoteAddrInfo(CenterAddrInfo);

					// 收到了来自网关的请求之后, 中心服务器向Center-dbClient 发送注册请求.
					SIMPLE_CLIENT_SEND(dbClient, SP_PlayerRegistInfoRequests, UserID, SlotID, GateAddrInfo, CenterAddrInfo);

				}
			break;
		}

		/* 玩家退出. */
		case SP_PlayerQuitRequests:
		{
			int32 UserID = INDEX_NONE;
			SIMPLE_PROTOCOLS_RECEIVE(SP_PlayerQuitRequests, UserID);

			if (UserID != INDEX_NONE) {

				/* 退出游戏的时候, 把用户的属性集记录下来压成JSON发给上一层CS-dbclient. */
				if (FMMOARPGPlayerRegistInfo* InPlayerInfo = FindPlayerData(UserID)) {
					if (InPlayerInfo->CharacterAttributes.Num() > 0) {
						FString JsonString;
						NetDataAnalysis::MMOARPGCharacterAttributeToString(InPlayerInfo->CharacterAttributes, JsonString);
						// 从CS发给CS-dbclient;
						SIMPLE_CLIENT_SEND(dbClient, SP_UpdateCharacterDataRequests, UserID, InPlayerInfo->CAInfo.SlotPosition, JsonString);
					}
				}

				/* 在前一步上传完角色属性集后 再移除CS服务器上的角色信息. */
				if (UMMOARPGCenterServerObject::RemoveRegistInfo(UserID)) {
					UE_LOG(LogMMOARPGCenterServer, Display, TEXT("Object removed [%i] successfully"), UserID);
				}
				else {
					UE_LOG(LogMMOARPGCenterServer, Display, TEXT("The object was not found [%i] and may have been removed"), UserID);
				}
			}
			break;
		}

		/** 刷新登录玩家请求. */
		case SP_UpdateLoginCharacterInfoRequests:
		{
			int32 UserID = INDEX_NONE;
			SIMPLE_PROTOCOLS_RECEIVE(SP_UpdateLoginCharacterInfoRequests, UserID);

			if (UserID != INDEX_NONE) {
				// 挑选缓存池里匹配的ID并把对应的相貌身材压缩成JSON, 组织成一条回复发回去.
				for (auto& Tmp : PlayerRegistInfos) {
					if (Tmp.Value.UserInfo.ID == UserID) {
						FString CAJsonString;
						NetDataAnalysis::CharacterAppearancesToString(Tmp.Value.CAInfo, CAJsonString);
						// 把压缩好的JSON发给DS.(DS在GameMode里)
						SIMPLE_PROTOCOLS_SEND(SP_UpdateLoginCharacterInfoResponses, UserID, CAJsonString);
						break;
					}
				}
			}
			break;
		}

		/** GAS人物属性集请求. */
		case SP_GetCharacterDataRequests:
		{
			int32 UserID = INDEX_NONE;
			int32 CharacterID = INDEX_NONE;
			int32 MMOARPGSlot = INDEX_NONE;
			SIMPLE_PROTOCOLS_RECEIVE(SP_GetCharacterDataRequests, UserID, CharacterID, MMOARPGSlot);

			if (UserID != INDEX_NONE && CharacterID != INDEX_NONE) {

				FString CharacterDataJsonString;
				if (FMMOARPGPlayerRegistInfo* InUserData = FindPlayerData(UserID)) {/* 是哪个用户的信息*/
					if (FMMOARPGCharacterAttribute* InCharacterAttribute = InUserData->CharacterAttributes.Find(CharacterID)) {/* 哪个用户的哪个单独角色的属性集*/
						/* 找到了玩家属性集就将其压缩成JSON 并给这个response发送出去.*/

						NetDataAnalysis::MMOARPGCharacterAttributeToString(*InCharacterAttribute, CharacterDataJsonString);
						SIMPLE_PROTOCOLS_SEND(SP_GetCharacterDataResponses, UserID, CharacterDataJsonString);
					}
					else {
						/* 找不到玩家属性集就让DS发送一条指令给db-client.*/
						FSimpleAddrInfo CenterAddrInfo;
						GetRemoteAddrInfo(CenterAddrInfo);
						SIMPLE_CLIENT_SEND(dbClient, SP_GetCharacterDataRequests, UserID, CharacterID, MMOARPGSlot, CenterAddrInfo);
					}
				}
				else {
					/* 用户数据甚至都不存在的情况.*/
					CharacterDataJsonString = TEXT("The user data does not exist on the center server.");
					SIMPLE_PROTOCOLS_SEND(SP_GetCharacterDataResponses, UserID, CharacterDataJsonString);
				}
			}
			break;
		}

		/** 身份覆写协议. */
		case SP_IdentityReplicationRequests:
		{
			FString IP;
			uint32 Port = 0;
			SIMPLE_PROTOCOLS_RECEIVE(SP_IdentityReplicationRequests, IP, Port);

			bool bIdentityReplication = false;// 是否命中身份响应.
			if (Port != 0 && !IP.IsEmpty()) {
				// 修改CS链接类型为DS链入.
				LinkType = ECentralServerLinkType::GAME_DEDICATED_SERVER_LINK;

				FMMOARPGDicatedServerInfo ServerInfo;// 欲填充的DS.
				DicatedServerKey = FSimpleNetManage::GetSimpleAddr(*IP, Port);
				AddDicatedServerRegistInfo(DicatedServerKey, ServerInfo);
				bIdentityReplication = true;
				UE_LOG(LogMMOARPGCenterServer, Display, TEXT("New DS server address accepted. IP = %s, Port=%i;"), *IP, Port);
			}
			else {
				UE_LOG(LogMMOARPGCenterServer, Display, TEXT("DS server address acceptance error, please check the acceptance status. IP = %s, Port=%i;"), *IP, Port);
			}
			SIMPLE_PROTOCOLS_SEND(SP_IdentityReplicationResponses, bIdentityReplication);
			break;
		}
	}
}

void UMMOARPGCenterServerObject::AddRegistInfo(const FMMOARPGPlayerRegistInfo& InRegistInfo)
{
	for (auto& Tmp : PlayerRegistInfos) {
		if (!Tmp.Value.IsVaild()) {
			Tmp.Value = InRegistInfo;
			break;
		}
	}
}

bool UMMOARPGCenterServerObject::RemoveRegistInfo(const int32 InUserID)
{
	for (auto& Tmp : PlayerRegistInfos) {
		if (Tmp.Value.UserInfo.ID == InUserID) {
			Tmp.Value.Reset();
			return true;
		}
	}

	return false;
}

// 注册给定的属性集到指定的用户数据里
void UMMOARPGCenterServerObject::AddRegistInfo_CharacterAttribute(int32 InUserID, int32 InCharacterID, const FMMOARPGCharacterAttribute& InCharacterAttribute)
{
	if (FMMOARPGPlayerRegistInfo* PlayerInfo = FindPlayerData(InUserID)) {
		if (!PlayerInfo->CharacterAttributes.Contains(InCharacterID)) {
			PlayerInfo->CharacterAttributes.Add(InCharacterID, InCharacterAttribute);
		}
		else {
			UE_LOG(LogMMOARPGCenterServer, Error, TEXT("Warning: The data already exists in the table. Please check the problem."));
		}
	}
}

// 在玩家信息缓存池里拿取给定用户号的玩家注册信息.
FMMOARPGPlayerRegistInfo* UMMOARPGCenterServerObject::FindPlayerData(int32 InUserID)
{
	for (auto& Tmp : PlayerRegistInfos) {
		if (Tmp.Value.IsVaild()) {
			if (Tmp.Value.UserInfo.ID == InUserID) {
				return &Tmp.Value;
			}
		}
	}
	return nullptr;
}

void UMMOARPGCenterServerObject::AddDicatedServerRegistInfo(const FSimpleAddr& InAddr, const FMMOARPGDicatedServerInfo& InDicatedServerInfo)
{
	DicatedServerInfos.Add(InAddr, InDicatedServerInfo);
}

void UMMOARPGCenterServerObject::AddDicatedServerRegistInfo(const FString& InIP, const int32 InPort, const FMMOARPGDicatedServerInfo& InDicatedServerInfo)
{
	const FSimpleAddr InAddr = FSimpleNetManage::GetSimpleAddr(*InIP, InPort);
	AddDicatedServerRegistInfo(InAddr, InDicatedServerInfo);
}

int32 UMMOARPGCenterServerObject::RemoveDicatedServerInfo(const FSimpleAddr& InAddr)
{
	return DicatedServerInfos.Remove(InAddr);
}

const FMMOARPGDicatedServerInfo* UMMOARPGCenterServerObject::FindDicatedServerInfo(const FSimpleAddr& InAddr)
{
	return DicatedServerInfos.Find(InAddr);
}

const FMMOARPGDicatedServerInfo* UMMOARPGCenterServerObject::FindDicatedServerInfo(const FString& InIP, const int32 InPort)
{
	// 	// 先写死1个服务器机器IP地址.暂设为本地本机IP,不用服务器机器.
	// 	FSimpleAddr DsAddr = FSimpleNetManage::GetSimpleAddr(TEXT("47.102.213.42"), 7777);

	const FSimpleAddr InAddr = FSimpleNetManage::GetSimpleAddr(*InIP, InPort);
	return DicatedServerInfos.Find(InAddr);
}

FSimpleAddr* UMMOARPGCenterServerObject::FindDicatedServerAddr()
{
	for (auto& Tmp : DicatedServerInfos) {
		return &Tmp.Key;
	}
	return nullptr;
}
