#pragma once

#include "CoreMinimal.h"

namespace MMOARPGDaemon
{
	FString GetParseValue(const FString &InKey);

	void CallExeProgram(const FString &InExePath, const FString& InParam,bool &bVaild);

	void DaemonTick();
}