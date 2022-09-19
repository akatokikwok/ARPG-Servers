#pragma once
#include "CoreMinimal.h"

struct FDaemonProcess
{
	FString ExePath;
	FString Param;

	bool bVaild;
};

struct FDaemonProcessListening
{
	FDaemonProcessListening(const FString& InKey, bool& InbVaild)
		:bVaild(InbVaild)
		, Key(InKey)
	{}

	FProcHandle Handle;
	bool& bVaild;

	FString Key;
};
