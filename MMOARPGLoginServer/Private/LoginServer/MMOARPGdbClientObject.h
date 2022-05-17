#pragma once
#include "CoreMinimal.h"
#include "..\Plugins\SimpleNetChannel\Source\SimpleNetChannel\Public\UObject\SimpleController.h"
#include "MMOARPGdbClientObject.generated.h"

/**
 * dbClient 对象
 * 是网关服务器往左一层的 dbClient.
 */
UCLASS()
class UMMOARPGdbClientObject : public USimpleController
{
	GENERATED_BODY()

public:
	virtual void Init() override;// 有新的链接链入我们的服务器,会激活Init.
	virtual void Tick(float DeltaTime) override;// Tick服务器自身.
	virtual void Close() override;// Close.
	virtual void RecvProtocol(uint32 InProtocol) override;// 对方发的协议借助此方法感知到.

};