// Copyright (c) 2018 Hirofumi Seo, M.D. at SCIEMENT, Inc.
// http://www.sciement.com
// http://www.sciement.com/tech-blog/

#include "ComputeShaderTest419.h"
#include "Modules/ModuleManager.h"
#include "Interfaces/IPluginManager.h"
#include "Logging/LogMacros.h"
#include "Misc/Paths.h"
#include <ShaderCore.h>

void FComputeShaderTest419Module::StartupModule()
{
	FString ShaderDirectory = FPaths::Combine(FPaths::ProjectDir(), TEXT("Shaders"));
	AddShaderSourceDirectoryMapping("/Project", ShaderDirectory);
}

void FComputeShaderTest419Module::ShutdownModule()
{
}

IMPLEMENT_PRIMARY_GAME_MODULE(FComputeShaderTest419Module, ComputeShaderTest419, "ComputeShaderTest419");
