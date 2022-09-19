// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class MMOARPGDaemon : ModuleRules
{
	public MMOARPGDaemon(ReadOnlyTargetRules Target) : base(Target)
	{
		PublicIncludePaths.Add("Runtime/Launch/Public");

		PrivateIncludePaths.Add("Runtime/Launch/Private");		// For LaunchEngineLoop.cpp include

		PrivateDependencyModuleNames.Add("Core");
		PrivateDependencyModuleNames.Add("Projects");

		PrivateDependencyModuleNames.Add("CoreUObject");
		PrivateDependencyModuleNames.Add("ApplicationCore");
	}
}
