// Copyright (C) RenZhai.2020.All Rights Reserved.
#pragma once

#include "CoreMinimal.h"
#include "UObject/SimpleController.h"
#include "MMOARPGType.h"
#include "MMOARPGCenterServerObject.generated.h"

UCLASS()
class UMMOARPGCenterServerObject : public USimpleController
{
	GENERATED_BODY()

public:
	virtual void Init();
	virtual void Tick(float DeltaTime);
	virtual void Close();
	virtual void RecvProtocol(uint32 InProtocol);
	
	// 注册一条RI到 玩家缓存池里.
 	static void AddRegistInfo(const FMMOARPGPlayerRegistInfo &InRegistInfo);
	// 格式化指定用户ID的 RI.
 	static bool RemoveRegistInfo(const int32 InUserID);
private:
	/**
	 * 原理:由于CS和单个客户端相链接时会创建Channel.
	 * 每次channel都会构建1个缓存池.(实际上就是每个客户端实例都会携带一个MAP)
	 * 所以最终的玩家信息缓存池要设置成静态的.
	 * 设置为静态,方便很多个实例共享这一份成员变量数据.
	 */
 	static TMap<int32, FMMOARPGPlayerRegistInfo> PlayerRegistInfos;// TMap: 玩家注册信息缓存池.
};