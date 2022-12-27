#pragma once

#include "CoreMinimal.h"

namespace MMOARPGDaemon
{
	// 根据Key提取相关的属性或参数
	FString GetParseValue(const FString &InKey);

	// 呼叫拉起某个.exe; @param 路径, @param 启动参数, @param 合法性
	void CallExeProgram(const FString &InExePath, const FString& InParam,bool &bVaild);

	// 每隔某段时间执行监测此进程(判断存活性)
	void DaemonTick();
}