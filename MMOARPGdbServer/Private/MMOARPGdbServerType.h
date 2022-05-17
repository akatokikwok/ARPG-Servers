#pragma once
#include "CoreMinimal.h"
#include "Core/SimpleMysqlLinkType.h"

/* 
* 仅自定义一张mysql配置表
*/
struct FMysqlConfig
{
	FMysqlConfig()
		:User("root")
		,Host("127.0.0.1")
		,Pwd("root")
		,DB("hello")
		,Port(3306)
	{
		ClientFlags.Add(ESimpleClientFlags::Client_Multi_Statements);// 使客户端支持多条语句
		ClientFlags.Add(ESimpleClientFlags::Client_PS_Multi_Results);// 是客户端支持多条返回结果
	}

	FString User;
	FString Host;
	FString Pwd;
	FString DB;
	int32 Port;
	TArray<ESimpleClientFlags> ClientFlags;
};