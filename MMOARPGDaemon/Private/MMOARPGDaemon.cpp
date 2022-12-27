#include "MMOARPGDaemon.h"
#include "RequiredProgramMainCPPInclude.h"
#include "MMOARPGDaemonType.h"

IMPLEMENT_APPLICATION(MMOARPGDaemon, "MMOARPGDaemon");

/** 对某个"被守护进程组"初始化 */
void Init(TArray<FDaemonProcess>& InProcessArray)
{
	for (auto& Tmp : InProcessArray) {
		CallExeProgram(Tmp.ExePath, Tmp.Param, Tmp.bVaild);// 呼叫拉起程序
	}
}

/** INT32_MAIN 主入口 */
INT32_MAIN_INT32_ARGC_TCHAR_ARGV()
{
	// 默认引擎初始化与日志
	GEngineLoop.PreInit(ArgC, ArgV);
	UE_LOG(LogMMOARPGDaemon, Display, TEXT("Hello World, MMOARPGDaemon"));

	// 构建与填充 "被守护进程组"
	TArray<FDaemonProcess> ProcessArray;
	FDaemonProcess& InProcessRef = ProcessArray.AddDefaulted_GetRef();
	InProcessRef.ExePath = GetParseValue(TEXT("-ExeFilename="));
	InProcessRef.Param = GetParseValue(TEXT("-ExeParam="));

	// 初始化此 "守护进程组"
	Init(ProcessArray);

	/* 持续监听 进程组*/
	while (!IsEngineExitRequested()) {
		// Tick监测(判断存活性)
		DaemonTick();
		// 如果不成功就拉起进程
		for (auto& Tmp : ProcessArray) {
			if (!Tmp.bVaild) {
				CallExeProgram(Tmp.ExePath, Tmp.Param, Tmp.bVaild);
			}
		}
		// 每隔10秒再次重复检测 守护进程(这里通常被指定为DS服务器) 存活性,若DS被意外关闭,则间隔10秒再次把它拉起启用.
		FPlatformProcess::Sleep(10.f);
	}

	// 默认退出
	FEngineLoop::AppExit();
	return 0;
}
