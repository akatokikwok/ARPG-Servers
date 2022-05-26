/**
 * 此列表文件专用于储存一些重要的服务器.
 */
 #pragma once
 #include "CoreMinimal.h"
#include "../Plugins/SimpleNetChannel/Source/SimpleNetChannel/Public/SimpleNetManage.h"

/** 
 * 设计原因具体参照Xmind .
 * 一些重要服务器.
 */

FSimpleNetManage* GateServer;// 网关服务器.
FSimpleNetManage* dbClient;// 数据库客户端-Gate.
FSimpleNetManage* CenterClient;// 中心客户端-Gate.