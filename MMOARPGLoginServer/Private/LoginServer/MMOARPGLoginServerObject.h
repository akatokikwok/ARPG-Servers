/**
 * 是一个反射类,可以允许用户更自由地自定义.
 */

#pragma once
#include "CoreMinimal.h"
#include "..\Plugins\SimpleNetChannel\Source\SimpleNetChannel\Public\UObject\SimpleController.h"
#include "MMOARPGLoginServerObject.generated.h"

/** 网关服务器对象, GateServerObejct */
UCLASS()
class UMMOARPGLoginServerObejct : public USimpleController
{
	GENERATED_BODY()

public:
	virtual void Init() override;// 有新的链接链入我们的服务器,会激活Init.
	virtual void Tick(float DeltaTime) override;// Tick服务器自身.
	virtual void Close() override;// Close.
	virtual void RecvProtocol(uint32 InProtocol) override;// 对方发的协议借助此方法感知到.

};