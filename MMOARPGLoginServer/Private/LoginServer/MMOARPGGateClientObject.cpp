#include "MMOARPGGateClientObject.h"
#include "SimpleMySQLibrary.h"
#include "Log\MMOARPGLoginServerLog.h"
#include "Protocol/ServerProtocol.h"

UMMOARPGGateClientObject::UMMOARPGGateClientObject()
	: Time(0.0f)
{

}

void UMMOARPGGateClientObject::Init()
{
	Super::Init();

}

void UMMOARPGGateClientObject::Tick(float DeltaTime)
{
	Super::Tick(DeltaTime);
	Time += DeltaTime;

	if (Time >= 1.0f) {
		Time = 0.0f;
		// 每隔一会儿就发送网关请求.
		SIMPLE_PROTOCOLS_SEND(SP_GateStatusRequests);
	}
}

void UMMOARPGGateClientObject::Close()
{
	Super::Close();

}

void UMMOARPGGateClientObject::RecvProtocol(uint32 InProtocol)
{
	Super::RecvProtocol(InProtocol);

	switch (InProtocol) {
		case SP_GateStatusResponses:
		{
			// 			FString StringTemp;
			// 			ELoginType Type = ELoginType::LOGIN_DB_SERVER_ERROR;
			// 			FSimpleAddrInfo AddrInfo;

						//
			SIMPLE_PROTOCOLS_RECEIVE(SP_GateStatusResponses, GateStatus);// 接收完协议之后,网关状态就更新了.

			UE_LOG(LogMMOARPGLoginServer, Display, TEXT("[SP_GateStatusResponses]"));// 登录服务器回复并响应网关发来的请求.
			break;
		}
	}
}

FMMOARPGGateStatus& UMMOARPGGateClientObject::GetGateStatus()
{
	return GateStatus;
}
