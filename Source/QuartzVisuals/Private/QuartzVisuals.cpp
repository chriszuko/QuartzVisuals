// Copyright Zuko Media 2023 all rights reserved.

#include "QuartzVisuals.h"

#define LOCTEXT_NAMESPACE "FQuartzVisualsModule"

void FQuartzVisualsModule::StartupModule()
{
	// This code will execute after your module is loaded into memory; the exact timing is specified in the .uplugin file per-module
}

void FQuartzVisualsModule::ShutdownModule()
{
	// This function may be called during shutdown to clean up your module.  For modules that support dynamic reloading,
	// we call this function before unloading the module.
}

#undef LOCTEXT_NAMESPACE
	
IMPLEMENT_MODULE(FQuartzVisualsModule, QuartzVisuals)