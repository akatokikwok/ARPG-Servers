/**
 * 是一个反射类,可以允许用户更自由地自定义.
 */

#pragma once
#include "CoreMinimal.h"
#include "..\Plugins\SimpleNetChannel\Source\SimpleNetChannel\Public\UObject\SimpleController.h"
#include "Blueprint/SimpleMysqlObject.h"
#include "Core/SimpleMysqlLinkType.h"
#include "../../SimpleHTTP/Source/SimpleHTTP/Public/SimpleHTTPType.h"
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
		void Callback_CheckPasswordResult(const FSimpleHttpRequest& InRequest, const FSimpleHttpResponse& InResponse, bool bLinkSuccessful);

protected:
	USimpleMysqlObject* MysqlObjectRead;// 数据库对象:读
	USimpleMysqlObject* MysqlObjectWrite;// 数据库对象:写

};