#include "MysqlConfig.h"
#include "Misc/FileHelper.h"
FSimpleMysqlConfig* FSimpleMysqlConfig::Global = nullptr;// 注意静态本类单例指针 类外初始化

FSimpleMysqlConfig* FSimpleMysqlConfig::Get()
{
	if (!Global) {
		Global = new FSimpleMysqlConfig();
	}
	return Global;
}

void FSimpleMysqlConfig::Destroy()
{
	if (Global) { // 如果这根指针(即静态全局单例) 有值,就把它干掉
		delete Global;
		Global = nullptr;
	}
}

void FSimpleMysqlConfig::Init(const FString& InPath /*= FPaths::ProjectDir() / TEXT("MysqlConfig.ini")*/)
{
	TArray<FString> Content;
	FFileHelper::LoadFileToStringArray(Content, *InPath);// 先把数据存到这个数组里

	if (Content.Num()) { // 首先检查判断是不是真的把路径里的数据读进至Content数组里
		/* 声明1个Lambda负责解析 读进来的各行数据*/ 
		auto Lambda_Parase = [&](TMap<FString, FString>& OutContent) {
			/* 遍历每一行执行进一步判断.*/
			for (auto& Tmp : Content) {
				if (Tmp.Contains("[") && Tmp.Contains("]")) { // 满足此则表明是头部第一行, 解析头部
					Tmp.RemoveFromEnd("]");
					Tmp.RemoveFromStart("[");// 掐头去尾杂项符号后留下纯净的内容字符串
					OutContent.Add("ConfigHead", Tmp);// 这个第一行的不携带[]字符的纯净字符串注册进入参TMap里.
// 					OutContent["ConfigHead"] = Tmp;
				} 
				else { // 解析第二行开始的, 即"身体"
					FString R, L;
					Tmp.Split(TEXT("="), &L, &R);// 以=号为分界线, 把这一行数据 拆为左部、右部
					OutContent.Add(L, R);
				}
			}
		};
		
		// 使用lambda把解析出来的map存到新map里
		TMap<FString, FString> InConfigInfo;
		Lambda_Parase(InConfigInfo);

		// 给配置表做值(借助map)
		ConfigInfo.User = InConfigInfo["User"];
		ConfigInfo.Host = InConfigInfo["Host"];
		ConfigInfo.Pwd = InConfigInfo["Pwd"];
		ConfigInfo.DB = InConfigInfo["DB"];
		ConfigInfo.Port = FCString::Atoi( *(InConfigInfo["Port"]));

	} else { // 如果没读进来,就硬生生帮助Content硬造内容 并传回到路径文件里去
		Content.Add(TEXT("[SimpleMysqlConfig]"));
		Content.Add(FString::Printf(TEXT("User=%s"), *ConfigInfo.User));
		Content.Add(FString::Printf(TEXT("Host=%s"), *ConfigInfo.Host));
		Content.Add(FString::Printf(TEXT("Pwd=%s"), *ConfigInfo.Pwd));
		Content.Add(FString::Printf(TEXT("DB=%s"), *ConfigInfo.DB));
		Content.Add(FString::Printf(TEXT("Port=%i"), ConfigInfo.Port));

		FFileHelper::SaveStringArrayToFile(Content, *InPath);
	}
}

const FMysqlConfig& FSimpleMysqlConfig::GetInfo() const
{
	return ConfigInfo;// 直接访问拿取配置表
}

