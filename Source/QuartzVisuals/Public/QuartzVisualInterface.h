// Copyright Zuko Media 2023 all rights reserved.

#pragma once
#include "QuartzVisualSharedTypes.h"
#include "QuartzVisualInterface.generated.h"

UINTERFACE(meta = (Blueprintable), BlueprintType)
class QUARTZVISUALS_API UQuartzVisualsInterface : public UInterface
{
	GENERATED_BODY()
};


class QUARTZVISUALS_API IQuartzVisualsInterface
{
	GENERATED_BODY()
public:
	
	// Happens during the update of the pulse (Interpolated values)
	UFUNCTION(BlueprintCallable, BlueprintNativeEvent)
	void OnQuartzVisualUpdate(const FQuartzVisualPulseEntry& PulseData);
	virtual void OnQuartzVisualUpdate_Implementation(const FQuartzVisualPulseEntry& PulseData);
};