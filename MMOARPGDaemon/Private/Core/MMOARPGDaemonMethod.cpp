#include "MMOARPGDaemonMethod.h"
#include "Log/MMOARPGDaemonLog.h"
#include "MMOARPGDaemonType.h"

namespace MMOARPGDaemon
{
	/** 声明1个守护进程监听组 */
	TArray<FDaemonProcessListening> Listenings;

	/** 根据Key提取相关的属性 */
	FString GetParseValue(const FString& InKey)
	{
		FString Value;
		if (!FParse::Value(FCommandLine::Get(), *InKey, Value)) {
			UE_LOG(LogMMOARPGDaemon, Error, TEXT("%s was not found value"), *InKey);// 解析失败则日志打印通知
		}

		return Value;
	}

	/** 在监听组里查询某个进程是否存在 */
	bool IsExitExe(const FString& InKey)
	{
		for (auto& Tmp : Listenings) {
			if (Tmp.Key == InKey) {
				return true;
			}
		}
		return false;
	}

	/** 在监听组里查询并拿取某个监听元素 */
	FDaemonProcessListening* FindExeListen(const FString& InKey)
	{
		for (auto& Tmp : Listenings) {
			if (Tmp.Key == InKey) {
				return &Tmp;
			}
		}
		return nullptr;
	}

	/** 呼叫某个.exe; @param 路径, @param 启动参数, @param 合法性 */
	void CallExeProgram(const FString& InExePath, const FString& InParam, bool& bVaild)
	{
		/* 当在监听组里没查到此进程*/
		if (!IsExitExe(InExePath)) {
			// 监听组里构建注册并返回指定的那个监听
			FDaemonProcessListening& InListening = Listenings.Add_GetRef(FDaemonProcessListening(InExePath, bVaild));
			// 创建出进程句柄
			InListening.Handle = FPlatformProcess::CreateProc(
				*InExePath, *InParam, false, false, false, NULL, 0, NULL, NULL, NULL);
		}
		else {
			// 拿取监听组里既有的这个监听
			if (FDaemonProcessListening* InListening = FindExeListen(InExePath)) {
				// 同样是创建出进程句柄
				InListening->Handle = FPlatformProcess::CreateProc(
					*InExePath, *InParam, false, false, false, NULL, 0, NULL, NULL, NULL);
			}
		}
	}

	/** 每隔某段时间执行监测此进程(判断存活性) */
	void DaemonTick()
	{
		// 遍历所有监听 判断并赋予存活有效性.
		for (auto& Tmp : Listenings) {
			if (FPlatformProcess::IsProcRunning(Tmp.Handle)) {
				Tmp.bVaild = true;
			}
			else {
				Tmp.bVaild = false;
			}
		}
	}
}