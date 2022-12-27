// Copyright (C) RenZhai.2020.All Rights Reserved.
#include "MMOARPGdbClientObject.h"
#include "log/MMOARPGCenterServerLog.h"
#include "Protocol/HallProtocol.h"
#include "Protocol/ServerProtocol.h"
#include "Protocol/GameProtocol.h"
#include "ServerList.h"
#include "MMOARPGType.h"
#include "MMOARPGCenterServerObject.h"
#include "../../../Plugins/SimpleNetChannel/Source/SimpleNetChannel/Public/Global/SimpleNetGlobalInfo.h"

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

					// 先写死1个服务器机器IP地址.暂设为本地本机IP,不用服务器机器.
// 					FSimpleAddr DsAddr = FSimpleNetManage::GetSimpleAddr(TEXT("47.102.213.42"), 7777);
					
					// 有DS地址就传DS地址,没地址就传1个本地.
					if (FSimpleAddr* DsAddr = UMMOARPGCenterServerObject::FindDicatedServerAddr()) {
						SIMPLE_SERVER_SEND(CenterServer, SP_LoginToDSServerResponses, CenterAddrInfo, GateAddrInfo, *DsAddr);
					}
					else {
						// 若没找到DS则以公网IP传入一个公网地址.
						FString NewPublicIP = FSimpleNetGlobalInfo::Get()->GetInfo().PublicIP;
						FSimpleAddr LocalDSAddr = FSimpleNetManage::GetSimpleAddr(*NewPublicIP, 7777);
						SIMPLE_SERVER_SEND(CenterServer, SP_LoginToDSServerResponses, CenterAddrInfo, GateAddrInfo, LocalDSAddr);
					}
					
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
			SIMPLE_PROTOCOLS_RECEIVE(SP_GetCharacterDataResponses, UserID, CharacterID, JsonString, CenterAddrInfo);

			if (UserID != INDEX_NONE && CharacterID != INDEX_NONE) {
				FMMOARPGCharacterAttribute CharacterAttribute;
				if (NetDataAnalysis::StringToMMOARPGCharacterAttribute(JsonString, CharacterAttribute)) {/* 确实能解析来自DB的属性集*/
					// 在中心服务器上 注册给定的属性集到指定的用户数据里
					UMMOARPGCenterServerObject::AddRegistInfo_CharacterAttribute(UserID, CharacterID, CharacterAttribute);

					// 转发至下一层CS
					SIMPLE_SERVER_SEND(CenterServer, SP_GetCharacterDataResponses, CenterAddrInfo, UserID, JsonString);
				}
				else {
					UE_LOG(LogMMOARPGCenterServer, Error, TEXT("ERROR: The [%s] could not be parsed."), *JsonString);
				}
			}
			// 之后再让CS服务器发送GAS属性集至客户端,形成GAS属性集更新逻辑闭环.

			break;
		}

		/** 来自DB的 更新人物属性集回复. */
		case SP_UpdateCharacterDataResponses:
		{
			int32 UserID = INDEX_NONE;
			int32 CharacterID = INDEX_NONE;
			bool bUpdateDataSuccessfully = false;
			SIMPLE_PROTOCOLS_RECEIVE(SP_UpdateCharacterDataResponses, UserID, CharacterID, bUpdateDataSuccessfully);

			if (bUpdateDataSuccessfully) {
				UE_LOG(LogMMOARPGCenterServer, Display, TEXT("Upload succeeded. UserID = %i, CharacterID = %i."), UserID, CharacterID);
			}
			else {
				UE_LOG(LogMMOARPGCenterServer, Error, TEXT("Upload fail. User=%i,CharacterID=%i"), UserID, CharacterID);
			}
		}

		/**  */
	}
}

