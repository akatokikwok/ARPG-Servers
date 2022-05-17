// Copyright Epic Games, Inc. All Rights Reserved.


#include "MMOARPGdbServer.h"
#include "Log\MMOARPGdbServerLog.h"
#include "RequiredProgramMainCPPInclude.h"
#include "dbServer/MysqlConfig.h"
#include "../../SimpleNetChannel/Source/SimpleNetChannel/Public/Global/SimpleNetGlobalInfo.h"
#include "../../SimpleNetChannel/Source/SimpleNetChannel/Public/SimpleNetManage.h"
#include "dbServer/MMOARPGServerObject.h"
#include "SimpleHTTPManage.h"


IMPLEMENT_APPLICATION(MMOARPGdbServer, "MMOARPGdbServer");

INT32_MAIN_INT32_ARGC_TCHAR_ARGV()
{
	GEngineLoop.PreInit(ArgC, ArgV);
	UE_LOG(LogMMOARPGdbServer, Display, TEXT("dbServer, Hello World"));

	/* 1.初始化配置与分布式插件.*/
	FSimpleMysqlConfig::Get()->Init();// 初始化配置表.
	FSimpleNetGlobalInfo::Get()->Init();// 初始化分布式插件

	/* 2.创建服务器实例.*/
	FSimpleNetManage* dbServer = 
		FSimpleNetManage::CreateManage(ESimpleNetLinkState::LINKSTATE_LISTEN, ESimpleSocketType::SIMPLESOCKETTYPE_TCP);

	/* 3.注册反射类.*/
	FSimpleChannel::SimpleControllerDelegate.BindLambda(
		[]() ->UClass* {
			return UMMOARPGServerObejct::StaticClass();
		}
	);

	/* 4.初始化检查.*/
	if (!dbServer->Init()) {
		delete dbServer;
		UE_LOG(LogMMOARPGdbServer, Error, TEXT("dbServer Instance Init Fail. "));

		return INDEX_NONE;
	}
	
	/* 5. 运行监测*/
	double LastTime = FPlatformTime::Seconds();
	while (!IsEngineExitRequested()) {
		// 休眠,来模拟每一帧都是0.03秒.
		FPlatformProcess::Sleep(0.03f);// 可在上线项目中注释此行,因为它会减缓时间.

		double Now = FPlatformTime::Seconds();
		float DeltaSeconds = Now - LastTime;// 求取每帧耗时.
		
		dbServer->Tick(DeltaSeconds);// 执行服务器实例Tick.
		FSimpleHttpManage::Get()->Tick(DeltaSeconds);// HTTP服务也需要执行Tick.

		LastTime = Now;
	}
	FSimpleNetManage::Destroy(dbServer);// 附带销毁.

	FEngineLoop::AppExit();
	return 0;
}
