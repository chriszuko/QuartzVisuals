// Copyright Zuko Media 2023 all rights reserved.

#pragma once

#include "CoreMinimal.h"
#include "Subsystems/WorldSubsystem.h"
#include "Quartz/AudioMixerClockHandle.h"
#include "QuartzVisualSharedTypes.h"
#include "QuartzVisualSubsystem.generated.h"

inline int32 GQuartzVisualsEnableLogs = 0;
inline FAutoConsoleVariableRef CVarQuartzVisualsEnableLogs(
	TEXT("QuartzVisuals.EnableLogs"),
	GQuartzVisualsEnableLogs,
	TEXT("Enable logs for the visuals. These can be pretty verbose"),
	ECVF_Default);

DECLARE_LOG_CATEGORY_EXTERN(LogQuartzVisuals, Log, Log);

/**
 * 
 */
UCLASS()
class QUARTZVISUALS_API UQuartzVisualSubsystem : public UTickableWorldSubsystem
{
	GENERATED_BODY()

public:

	virtual void Tick(float DeltaTime) override;
	void TickInternal(float DeltaTime);
	virtual bool IsTickable() const override;
	virtual TStatId GetStatId() const override;

	virtual bool DoesSupportWorldType(const EWorldType::Type WorldType) const override;

	virtual bool ShouldCreateSubsystem(UObject* Outer) const override;

	// Largely ignored so we can boot these up in-game
	virtual void Initialize(FSubsystemCollectionBase& Collection) override;
	virtual void Deinitialize() override;
	
	UFUNCTION(BlueprintCallable)
	void ForceTick(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void InitializeQuartzVisualSubsystem();
	
	UPROPERTY(BlueprintReadOnly)
	bool IsInitialized = false;

	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	bool UseForcedTick = false;
	
	/*Subscribe to the quantization you'd like to align your visuals to. 32nd note will be most accurate*/
	UFUNCTION(BlueprintCallable, Category="Setup", meta=(WorldContext = "WorldContextObject"))
	void SubscribeToQuantization(UObject* WorldContextObject, UQuartzClockHandle* QuartzClockHandle, EQuartzCommandQuantization Quantization);

	UPROPERTY(BlueprintReadOnly)
	UWorld* OwningWorld;

	UPROPERTY()
	UQuartzClockHandle* CachedQuartzClockHandle;
	
	UPROPERTY()
	FOnQuartzMetronomeEventBP QuartzMetronomeEvent;

	UPROPERTY()
	int32 CurrentBeatCount = 0;
	
	UPROPERTY()
	int32 CurrentBeatCountFull = -1;

	UPROPERTY()
	TArray<FQuartzVisualPulseEntry> QuartzVisualEntries;
	
	// How much of a delay before playing this pulse
	UPROPERTY(BlueprintReadWrite, EditAnywhere)
	int32 BeatDelayCount = 4;

	float DeltaTimeMultiplier = 1.0f;

	
	void UpdateQuartzVisualPulseEntries(float DeltaTime);

	UFUNCTION(BlueprintCallable)
	void AddNewQuartzVisualPulse(AActor* InActor, FQuartzVisualPulseSettings QuantizedVisualPulseSettings, int32 BeatDuration, int32 BeatOffset, bool StopIfExists = false);

	void RemoveQuartzVisualPulseFromActor(int32 Index, int32 IndexFilter, AActor* InActor);

	void RemoveAllQuartzVisualPulsesFromActor(AActor* Device, TArray<int32> ExcludeIndexFilters = TArray<int32>());

	void RemoveAllQuartzVisualPulses();
	
	void CancelAndFinishQuartzVisualEntry(FQuartzVisualPulseEntry& QuartzVisualPulseEntry);
	
	
	// Quantization event.
	UFUNCTION()
	void OnQuantizationEvent(FName ClockName, EQuartzCommandQuantization QuantizationType, int32 NumBars, int32 Beat, float BeatFraction);
};
