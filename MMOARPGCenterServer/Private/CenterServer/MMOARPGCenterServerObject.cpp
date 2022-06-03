// Copyright (C) RenZhai.2020.All Rights Reserved.
#include "MMOARPGCenterServerObject.h"
#include "log/MMOARPGCenterServerLog.h"
#include "Protocol/ServerProtocol.h"
#include "Protocol/HallProtocol.h"
#include "ServerList.h"
#include "Protocol/GameProtocol.h"

TMap<int32, FMMOARPGPlayerRegistInfo> UMMOARPGCenterServerObject::PlayerRegistInfos;

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

