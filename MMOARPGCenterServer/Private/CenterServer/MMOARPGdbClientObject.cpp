// Copyright (C) RenZhai.2020.All Rights Reserved.
#include "MMOARPGdbClientObject.h"
#include "log/MMOARPGCenterServerLog.h"
#include "Protocol/HallProtocol.h"
#include "Protocol/ServerProtocol.h"
#include "ServerList.h"
#include "MMOARPGType.h"
#include "MMOARPGCenterServerObject.h"
#include <SimpleNetManage.h>

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
	
	switch (InProtocol) {

		case SP_PlayerRegistInfoResponses :// 回应Center-Server的请求.
		{
			FSimpleAddrInfo GateAddrInfo;
			FSimpleAddrInfo CenterAddrInfo;
			FString UserJson;
			FString SlotJson;
			SIMPLE_PROTOCOLS_RECEIVE(SP_PlayerRegistInfoResponses, UserJson, SlotJson, GateAddrInfo, CenterAddrInfo);
			
			// 之前在db里设置过,若为空则默认填充[]
			if (UserJson != TEXT("[]") && SlotJson != TEXT("[]")) {
				// 用给定的中心服务器地址 把服务器转换成 特定的中心服务器.
				if (UMMOARPGCenterServerObject* InCenterServer = Cast<UMMOARPGCenterServerObject>(FSimpleNetManage::GetNetManageNetworkObject(CenterServer, CenterAddrInfo)))
				{
					// 解析出RI中的 CA存档和玩家信息 并注册到 玩家注册表里.
					FMMOARPGPlayerRegistInfo RI;
					NetDataAnalysis::StringToCharacterAppearances(SlotJson, RI.CAInfo);
					NetDataAnalysis::StringToUserData(UserJson, RI.UserInfo);
					InCenterServer->AddRegistInfo(RI);

					// 预准备DS服务器.
					FSimpleAddr DsAddr = FSimpleNetManage::GetSimpleAddr(TEXT("192.168.2.30"), 7777);
					SIMPLE_SERVER_SEND(CenterServer, SP_LoginToDSServerResponses, CenterAddrInfo, GateAddrInfo, DsAddr);
				}
			}

			break;
		}

	}
}

