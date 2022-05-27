// Copyright (C) RenZhai.2020.All Rights Reserved.
#include "MMOARPGdbClientObject.h"
#include "log/MMOARPGCenterServerLog.h"
#include "Protocol/HallProtocol.h"
#include "Protocol/ServerProtocol.h"
#include "ServerList.h"
#include "MMOARPGType.h"
#include "MMOARPGCenterServerObject.h"

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
			SIMPLE_PROTOCOLS_RECEIVE(SP_PlayerRegistInfoResponses, GateAddrInfo);

			break;
		}

	}
}

