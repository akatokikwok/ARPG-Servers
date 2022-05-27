// Copyright (C) RenZhai.2020.All Rights Reserved.
#include "MMOARPGCenterServerObject.h"
#include "log/MMOARPGCenterServerLog.h"
#include "Protocol/ServerProtocol.h"
#include "Protocol/HallProtocol.h"
#include "ServerList.h"

void UMMOARPGCenterServerObject::Init()
{
	Super::Init();

	// 主动为玩家注册信息映射表手动分配内存.
	for (int32 i = 0; i < 2000; i++) {
		PlayerRegistInfos.Add(i, FMMOARPGPlayerRegistInfo());
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
				GetLocalAddrInfo(CenterAddrInfo);

				// 收到了来自网关的请求之后, 中心服务器向Center-dbClient 发送注册请求.
				SIMPLE_CLIENT_SEND(dbClient, SP_PlayerRegistInfoRequests, UserID, SlotID, GateAddrInfo, CenterAddrInfo);
				
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

