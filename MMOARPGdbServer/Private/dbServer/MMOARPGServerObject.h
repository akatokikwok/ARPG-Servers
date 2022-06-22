/**
 * 是一个反射类,可以允许用户更自由地自定义.
 */

#pragma once
#include "CoreMinimal.h"
#include "..\Plugins\SimpleNetChannel\Source\SimpleNetChannel\Public\UObject\SimpleController.h"
#include "Blueprint/SimpleMysqlObject.h"
#include "Core/SimpleMysqlLinkType.h"
#include "../../SimpleHTTP/Source/SimpleHTTP/Public/SimpleHTTPType.h"
#include <MMOARPGType.h>
#include "MMOARPGServerObject.generated.h"

/** ServerObejct */
UCLASS()
class UMMOARPGServerObejct : public USimpleController
{
	GENERATED_BODY()

public:
	virtual void Init() override;// 有新的链接链入我们的服务器,会激活Init.
	virtual void Tick(float DeltaTime) override;// Tick服务器自身.
	virtual void Close() override;// Close.
	virtual void RecvProtocol(uint32 InProtocol) override;// 对方发的协议借助此方法感知到.

public:
	/**  把数据库数据提到服务器上.*/
	bool Post(const FString& InSQL);
	/**  借助sql查询语句, 把指定的数据库数据拉下来.*/
	bool Get(const FString& InSQL, TArray<FSimpleMysqlResult>& Results);

protected:
	// 给POST操作完成后 绑定的回调.
	UFUNCTION()
		void CheckPasswordResult_callback(const FSimpleHttpRequest& InRequest, const FSimpleHttpResponse& InResponse, bool bLinkSuccessful);
	// 检查注册用户结果.
	UFUNCTION()
		void CheckRegisterResult(const FSimpleHttpRequest& InRequest, const FSimpleHttpResponse& InResponse, bool bLinkSuccessful);
private:
	/** 给定键入的名字 并核验它的检查类型. */
	ECheckNameType CheckName(const FString& InName);

	// 将指定ID的用户信息注入JSon.
	bool GetUserInfo(int32 InUserID, FString& OutJsonString);

	// 将指定ID的Slot信息注入JSon.
	bool GetSlotCAInfo(int32 InUserID, int32 InSlotCAID, FString& OutJsonString);

	// 拆解出指定用户ID的 关联元数据 character_ca这个字段里存储的所有ID.
	bool GetCharacterCAInfoByUserMeta(int32 InUserID, TArray<FString>& OutIDs);

	// 将字符集输出成满足 s1, s2, s3, s4的形式.
	void GetSerialString(TCHAR* InSplitPrefix, const TArray<FString>& InStrings, FString& OutString);

	// 工具方法1;
	void GetAttributeInfo(const FString& InAttributeName, FMMOARPGAttributeData& OutAttributeData, const TMap<FString, FString>& InRow);

	// 工具方法; 判断角色属性集是否存在.
	bool IsCharacterAttributeExit(int32 InUserID, int32 InCharacterID, int32 MMOARPG_Slot);

	// 工具方法; 创建并更新1个人物属性集.
	bool CreateAndUpdateCharacterAttributeInfo(int32 InUserID, int32 InCharacterID, const FMMOARPGCharacterAttribute& InAttributeData, int32 MMOARPG_Slot);

	// 工具方法; 创建1个人物属性集.
	bool CreateCharacterAttributeInfo(int32 InUserID, int32 InCharacterID, const FMMOARPGCharacterAttribute& InAttributeData, int32 MMOARPG_Slot);

	// 工具方法; 更新1个人物属性集.
	bool UpdateCharacterAttributeInfo(int32 InUserID, int32 InCharacterID, const FMMOARPGCharacterAttribute& InAttributeData, int32 MMOARPG_Slot);

protected:
	USimpleMysqlObject* MysqlObjectRead;// 数据库对象:读
	USimpleMysqlObject* MysqlObjectWrite;// 数据库对象:写

};