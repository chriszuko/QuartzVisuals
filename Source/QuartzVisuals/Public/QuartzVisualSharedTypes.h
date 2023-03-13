// Copyright Zuko Media 2023 all rights reserved.

#pragma once
#include "QuartzVisualSharedTypes.generated.h"


class IQuartzVisualsInterface;
class UCurveFloat;

DECLARE_STATS_GROUP(TEXT("QuartzVisuals"), STATGROUP_QuartzVisuals, STATCAT_Advanced);

UENUM(BlueprintType)
enum class EQuartzVisualPulseState : uint8
{
	ReadyToStart		UMETA(DisplayName = "Ready to Start"),
	Start				UMETA(DisplayName = "Start"),
	Updating			UMETA(DisplayName = "Updating"),
	Finished			UMETA(DisplayName = "Finished"),
};

USTRUCT(BlueprintType)
struct QUARTZVISUALS_API FQuartzVisualPulsePayload
{
	GENERATED_USTRUCT_BODY()

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector VectorData = FVector::ZeroVector;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float FloatData = 0.0f;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 IntData = 0;
};


USTRUCT(BlueprintType)
struct QUARTZVISUALS_API FQuartzVisualPulseSettings
{
	GENERATED_USTRUCT_BODY()
	
	/* Used for logging purposes and debugging */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString DebugDisplayName = "";

	/* Used as a way to tell data to match other data.*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 IndexFilter = 0;
	
	/* Index is an identifier for the */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	int32 Index = 0;

	/* For cases where we want to do an event without a visual */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	bool UseValue = true;

	// Used to send useful data for the visual events
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FQuartzVisualPulsePayload Payload;
	
	/* Optional curve value to use for visualization, overrides min max settings */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	UCurveFloat* ValueCurve = nullptr;
	
	/* Value Min to lerp between for the progress (Ignored when using curve)*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FVector2D OutValueMinMax = FVector2D(0.0f, 1.0f);

	/* How fast the value should interpolate */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float InterpSpeed = 15.0f;

	/* Final Multiplier for the value*/
	float ValueMultiplier = 1.0f;

	FORCEINLINE bool operator ==(const FQuartzVisualPulseSettings& Settings) const
	{
		return Settings.Index == Index && Settings.IndexFilter == IndexFilter;
	}

	FString ToString() const
	{
		return DebugDisplayName + " Index: " + FString::FromInt(Index) + " IndexFilter: " + FString::FromInt(IndexFilter); 
	}
};

USTRUCT(BlueprintType)
struct QUARTZVISUALS_API FQuartzVisualPulseData
{
	GENERATED_USTRUCT_BODY()

	/* Used for cleanup and matching */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	AActor* Actor = nullptr;
	
	// Beat Count to start on for counting
	int32 CurrentBeatCount = 0;

	// Used to delay the visuals, will be auto calculated when added.
	int32 BeatOffset = 0;
	
	/* How many beats should this pulse last Has to be at least 1*/
	UPROPERTY(EditAnywhere, BlueprintReadWrite, meta = (ClampMin = 1))
	int32 BeatDuration = 1;	
}; 

USTRUCT(BlueprintType)
struct QUARTZVISUALS_API FQuartzVisualPulseEntry
{
	GENERATED_USTRUCT_BODY()

	/* Used for logging purposes and debugging */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FQuartzVisualPulseSettings Settings;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FQuartzVisualPulseData Data;

	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	EQuartzVisualPulseState State;
	
	/* 0 - 1 value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OutValueNormalized = 0.0f;
	
	/* Curve or adjusted value */
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	float OutValue = 0.0f;

	FString ToString() const
	{
		const FString ActorName = IsValid(Data.Actor) ? Data.Actor->GetName() : "NULL ";
		
		const FString ValueString = FString::FromInt(static_cast<int32>(OutValue)) + "." + FString::FromInt(static_cast<int32>((OutValue - static_cast<int32>(OutValue))*100));
		
		return ActorName + " " + Settings.ToString() + " Value: " + ValueString;
	}
	
	float GetBeatProgress()
	{
		return static_cast<float>(FMath::Clamp((Data.CurrentBeatCount - Data.BeatOffset), 0, Data.BeatDuration)) / static_cast<float>(Data.BeatDuration);
	}

	float GetBeatsRemaining()
	{
		return static_cast<float>(FMath::Clamp((Data.CurrentBeatCount - Data.BeatOffset), 0, Data.BeatDuration)) - Data.BeatDuration;
	}
	
	void UpdateValue(float DeltaSeconds)
	{
		// Only update if we are using a value
		if(Settings.UseValue)
		{
			OutValueNormalized = GetBeatProgress();
			float NewValue = OutValueNormalized;
			// Use the value curve
			if(Settings.ValueCurve)
			{
				NewValue = Settings.ValueCurve->GetFloatValue(OutValueNormalized);
				if(Settings.OutValueMinMax.X != 0.0f || Settings.OutValueMinMax.Y != 1.0f)
				{
					NewValue = FMath::Lerp(Settings.OutValueMinMax.X, Settings.OutValueMinMax.Y, NewValue);
				}
			}
			// Interpolate values if we aren't expecting 0-1 and aren't using a value curve
			else if(Settings.OutValueMinMax.X != 0.0f || Settings.OutValueMinMax.Y != 1.0f)
			{
				NewValue = FMath::Lerp(Settings.OutValueMinMax.X, Settings.OutValueMinMax.Y, OutValueNormalized);
			}

			// TODO Evaluate if we need to use BPM Scaled Deltaseconds.. I think we will.
			OutValue = FMath::FInterpTo(OutValue, NewValue, DeltaSeconds, Settings.InterpSpeed);
		}
	};

	// When checking if settings match, it skips checking the actor.
	FORCEINLINE bool operator ==(const FQuartzVisualPulseSettings& InSettings) const
	{
		return InSettings.Index == Settings.Index && InSettings.IndexFilter == Settings.IndexFilter;
	}

	// When checking if entries match, we make sure to check the actor.
	FORCEINLINE bool operator ==(const FQuartzVisualPulseEntry& InSettings) const
	{
		return InSettings.Settings.Index == Settings.Index
		&& InSettings.Settings.IndexFilter == Settings.IndexFilter
		&& InSettings.Data.Actor == Data.Actor;
	}
};