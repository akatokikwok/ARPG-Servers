#include "MMOARPGDaemon.h"
#include "RequiredProgramMainCPPInclude.h"
#include "MMOARPGDaemonType.h"

IMPLEMENT_APPLICATION(MMOARPGDaemon, "MMOARPGDaemon");

void Init(TArray<FDaemonProcess>& InProcessArray)
{
	for (auto& Tmp : InProcessArray) {
		CallExeProgram(Tmp.ExePath, Tmp.Param, Tmp.bVaild);
	}
}

INT32_MAIN_INT32_ARGC_TCHAR_ARGV()
{
	GEngineLoop.PreInit(ArgC, ArgV);
	UE_LOG(LogMMOARPGDaemon, Display, TEXT("Hello World"));

	TArray<FDaemonProcess> ProcessArray;

	FDaemonProcess& InProcessRef = ProcessArray.AddDefaulted_GetRef();
	InProcessRef.ExePath = GetParseValue(TEXT("-ExeFilename="));
	InProcessRef.Param = GetParseValue(TEXT("-ExeParam="));

	//初始化
	Init(ProcessArray);

	//监听
	while (!IsEngineExitRequested()) {
		DaemonTick();

		for (auto& Tmp : ProcessArray) {
			if (!Tmp.bVaild) {
				//如果不成功就拉起程序
				CallExeProgram(Tmp.ExePath, Tmp.Param, Tmp.bVaild);
			}
		}
		FPlatformProcess::Sleep(10.f);
	}

	FEngineLoop::AppExit();
	return 0;
}
