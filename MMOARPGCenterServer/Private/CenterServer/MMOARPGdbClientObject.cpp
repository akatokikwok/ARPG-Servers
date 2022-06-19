// Copyright (C) RenZhai.2020.All Rights Reserved.
#include "MMOARPGdbClientObject.h"
#include "log/MMOARPGCenterServerLog.h"
#include "Protocol/HallProtocol.h"
#include "Protocol/ServerProtocol.h"
#include "Protocol/GameProtocol.h"
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
// 				// 用给定的中心服务器地址 把服务器转换成 特定的中心服务器.
// 				if (UMMOARPGCenterServerObject* InCenterServer = Cast<UMMOARPGCenterServerObject>(FSimpleNetManage::GetNetManageNetworkObject(CenterServer, CenterAddrInfo))) {
				
					// 解析出RI中的 CA存档和玩家信息 并注册到 玩家注册表里.
					FMMOARPGPlayerRegistInfo RI;
					NetDataAnalysis::StringToCharacterAppearances(SlotJson, RI.CAInfo);
					NetDataAnalysis::StringToUserData(UserJson, RI.UserInfo);
					UMMOARPGCenterServerObject::AddRegistInfo(RI);

					// 预准备DS服务器.
// 					FSimpleAddr DsAddr = FSimpleNetManage::GetSimpleAddr(TEXT("192.168.2.30"), 7777);// 先写死1个服务器机器IP地址.
					FSimpleAddr DsAddr = FSimpleNetManage::GetSimpleAddr(TEXT("127.0.0.1"), 7777);// 先写死1个服务器机器IP地址.暂设为本地本机IP,不用服务器机器.
					SIMPLE_SERVER_SEND(CenterServer, SP_LoginToDSServerResponses, CenterAddrInfo, GateAddrInfo, DsAddr);
// 				}
			}

			break;
		}

		/** 来自DB的 人物GAS属性集回复 */
		case SP_GetCharacterDataResponses:
		{
			FSimpleAddrInfo CenterAddrInfo;
			int32 UserID = INDEX_NONE;
			int32 CharacterID = INDEX_NONE;
			FString JsonString;
			SIMPLE_PROTOCOLS_RECEIVE(SP_GetCharacterDataResponses, /*UserID, CharacterID,*/ JsonString, CenterAddrInfo);

			// 之后再让CS服务器发送GAS属性集至客户端,形成GAS属性集更新逻辑闭环.

			break;
		}
	}
}

