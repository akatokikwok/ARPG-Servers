// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MMOARPGdbServer : ModuleRules
{
	public MMOARPGdbServer(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicIncludePaths.Add("Runtime/Launch/Public");

		PrivateIncludePaths.Add("Runtime/Launch/Private");		// For LaunchEngineLoop.cpp include

		PrivateDependencyModuleNames.Add("Core");
		PrivateDependencyModuleNames.Add("Projects");

		PrivateDependencyModuleNames.Add("CoreUObject");
		PrivateDependencyModuleNames.Add("ApplicationCore");// 不包含则编译不过,它是应用程序的核心依赖项
		PrivateDependencyModuleNames.Add("SimpleMySQL");// 包含Engine/Plugins下本项目新增的且要用到的SimpleMySQL插件
		PrivateDependencyModuleNames.Add("SimpleNetChannel");
		PrivateDependencyModuleNames.Add("SimpleHTTP");
		PrivateDependencyModuleNames.Add("MMOARPGCommon");
	}
}
