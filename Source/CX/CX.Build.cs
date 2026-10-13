// Copyright Epic Games, Inc. All Rights Reserved.

using UnrealBuildTool;

public class CX : ModuleRules
{
	public CX(ReadOnlyTargetRules Target) : base(Target)
	{
		PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;

		// 모듈 폴더를 기준으로 UI/ChatUI.h 같은 헤더 경로를 찾습니다.
		PublicIncludePaths.Add(ModuleDirectory);

		PublicDependencyModuleNames.AddRange(new string[] 
		{ 
			"Core", "CoreUObject", "Engine", "InputCore", "EnhancedInput", "ChaosVehicles", "PhysicsCore" ,

			"UMG", "SlateCore",
		});

		
	}
}
