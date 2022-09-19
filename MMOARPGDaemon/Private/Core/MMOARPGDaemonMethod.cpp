#include "MMOARPGDaemonMethod.h"
#include "Log/MMOARPGDaemonLog.h"
#include "MMOARPGDaemonType.h"

namespace MMOARPGDaemon
{
	TArray<FDaemonProcessListening> Listenings;

	FString GetParseValue(const FString& InKey)
	{
		FString Value;
		if (!FParse::Value(FCommandLine::Get(), *InKey, Value))
		{
			UE_LOG(LogMMOARPGDaemon, Error, TEXT("%s was not found value"), *InKey);
		}

		return Value;
	}

	bool IsExitExe(const FString& InKey)
	{
		for (auto &Tmp : Listenings)
		{
			if (Tmp.Key == InKey)
			{
				return true;
			}
		}

		return false;
	}

	FDaemonProcessListening *FindExeListen(const FString& InKey)
	{
		for (auto& Tmp : Listenings)
		{
			if (Tmp.Key == InKey)
			{
				return &Tmp;
			}
		}

		return NULL;
	}

	void CallExeProgram(const FString& InExePath, const FString& InParam, bool& bVaild)
	{
		if (!IsExitExe(InExePath))
		{
			FDaemonProcessListening &InListening = Listenings.Add_GetRef(FDaemonProcessListening(InExePath,bVaild));
			InListening.Handle = FPlatformProcess::CreateProc(
				*InExePath, 
				*InParam, false, false, false,NULL, 0, NULL, NULL, NULL);
		}
		else
		{
			if (FDaemonProcessListening *InListening = FindExeListen(InExePath))
			{
				InListening->Handle = FPlatformProcess::CreateProc(
					*InExePath,
					*InParam, false, false, false, NULL, 0, NULL, NULL, NULL);
			}
		}		
	}

	void DaemonTick()
	{
		for (auto& Tmp : Listenings)
		{
			if (FPlatformProcess::IsProcRunning(Tmp.Handle))
			{
				Tmp.bVaild = true;
			}
			else
			{
				Tmp.bVaild = false;
			}
		}
	}
}