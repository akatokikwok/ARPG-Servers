#pragma once
#include "CoreMinimal.h"

/**
 * 某个守护进程信息 
 * "需要被守护的启动程序进程"
 */
struct FDaemonProcess
{
	// 程序的启动路径.
	FString ExePath;
	// 启动程序时 欲挂载的参数.
	FString Param;
	// 进程存活有效性.
	bool bVaild;
};

/**
 * 某个监听信息 
 * 负责监测监听"守护进程"功能的结构体
 */
struct FDaemonProcessListening
{
	FDaemonProcessListening(const FString& InKey, bool& InbVaild)
		:bVaild(InbVaild)
		, Key(InKey)
	{}
	// 程序句柄
	FProcHandle Handle;
	// 被监听的进程的存活性
	bool& bVaild;
	// 用作查询的Key, 监听哪一个
	FString Key;
};
