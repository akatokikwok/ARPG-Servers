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
	
	// 注册一条RI到 玩家注册表里.
 	void AddRegistInfo(const FMMOARPGPlayerRegistInfo &InRegistInfo);
	// 格式化指定用户ID的 RI.
 	bool RemoveRegistInfo(const int32 InUserID);
private:
 	TMap<int32, FMMOARPGPlayerRegistInfo> PlayerRegistInfos;// TMap: 玩家注册信息映射表.
};