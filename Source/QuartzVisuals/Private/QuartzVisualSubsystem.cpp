// Copyright Zuko Media 2023 all rights reserved.


#include "QuartzVisualSubsystem.h"
#include "QuartzVisualInterface.h"
#include "Kismet/KismetSystemLibrary.h"

DEFINE_LOG_CATEGORY(LogQuartzVisuals);


PRAGMA_DISABLE_OPTIMIZATION
void UQuartzVisualSubsystem::Tick(float DeltaTime)
{
	if(UseForcedTick == false)
	{
		TickInternal(DeltaTime);
	}
}

TStatId UQuartzVisualSubsystem::GetStatId() const
{
	return TStatId();
}

bool UQuartzVisualSubsystem::DoesSupportWorldType(const EWorldType::Type WorldType) const
{
	return (WorldType == EWorldType::Game || WorldType == EWorldType::PIE);
}

bool UQuartzVisualSubsystem::ShouldCreateSubsystem(UObject* Outer) const
{
	UWorld* World = Cast<UWorld>(Outer);
	check(World);
	// Forces us to be able to ONLY use the worlds supported
	if(DoesSupportWorldType(World->WorldType))
	{
		return true;
	}
	return false;
}

void UQuartzVisualSubsystem::Initialize(FSubsystemCollectionBase& Collection)
{
	Super::Initialize(Collection);
}

void UQuartzVisualSubsystem::Deinitialize()
{
	Super::Deinitialize();
}

void UQuartzVisualSubsystem::ForceTick(float DeltaTime)
{
	if(UseForcedTick)
	{
		TickInternal(DeltaTime);
	}
	else
	{
		UE_LOG(LogQuartzVisuals, Warning, TEXT("Trying to force tick quartz visuals without setting force tick to true"));
	}
}

void UQuartzVisualSubsystem::TickInternal(float DeltaTime)
{
	if(IsInitialized && IsValid(OwningWorld) && OwningWorld->IsPaused() == false)
	{
		UpdateQuartzVisualPulseEntries(DeltaTime);
	}
}

bool UQuartzVisualSubsystem::IsTickable() const
{
	return true;
}

void UQuartzVisualSubsystem::InitializeQuartzVisualSubsystem()
{
	if(IsInitialized == false)
	{
		// Find persistent World - TODO might be out dated, take a look at newer codez
		const TIndirectArray<FWorldContext>& WorldContexts = GEngine->GetWorldContexts();
		UWorld* PersistentWorld = nullptr;
		for (int32 WorldContextIndex = 0; WorldContextIndex < WorldContexts.Num(); WorldContextIndex++)
		{
			if (WorldContexts[WorldContextIndex].WorldType == EWorldType::Game)
			{
				PersistentWorld = WorldContexts[WorldContextIndex].World();
			}
			else if (WorldContexts[WorldContextIndex].WorldType == EWorldType::PIE)
			{
				PersistentWorld = WorldContexts[WorldContextIndex].World();
			}
		}
		if (PersistentWorld == nullptr)
		{
			for (int32 WorldContextIndex = 0; WorldContextIndex < WorldContexts.Num(); WorldContextIndex++)
			{
				if (WorldContexts[WorldContextIndex].WorldType == EWorldType::Editor)
				{
					PersistentWorld = WorldContexts[WorldContextIndex].World();
				}
			}
		}
		
		if(PersistentWorld)
		{
			OwningWorld = PersistentWorld;
			IsInitialized = true;
		}
	}
}

void UQuartzVisualSubsystem::SubscribeToQuantization(UObject* WorldContextObject, UQuartzClockHandle* QuartzClockHandle, EQuartzCommandQuantization Quantization)
{
	if(WorldContextObject && QuartzClockHandle)
	{
		CachedQuartzClockHandle = QuartzClockHandle;
		QuartzMetronomeEvent = FOnQuartzMetronomeEventBP();
		QuartzMetronomeEvent.BindUFunction(this, "OnQuantizationEvent");
		CurrentBeatCount = 0;
		CurrentBeatCountFull = -1;
		QuartzClockHandle->SubscribeToQuantizationEvent(WorldContextObject, EQuartzCommandQuantization::ThirtySecondNote, QuartzMetronomeEvent,QuartzClockHandle);
	}
}

DECLARE_CYCLE_STAT(TEXT("Quartz Visual Quantization"), STAT_QuartzVisualQuantization, STATGROUP_QuartzVisuals);
void UQuartzVisualSubsystem::OnQuantizationEvent(FName ClockName, EQuartzCommandQuantization QuantizationType, int32 NumBars, int32 Beat, float BeatFraction)
{	
	// Handle Quantized visual data. Thirty second notes should be smooth enough for this.
	CurrentBeatCount = Beat - 1;
	CurrentBeatCountFull++;

	// Delta time multiplier for interp usage based on BPM.
	if(CachedQuartzClockHandle)
	{
		 DeltaTimeMultiplier = CachedQuartzClockHandle->GetBeatsPerMinute(this)/120.0f;
	}
	else
	{
		DeltaTimeMultiplier = 1.0f;
	}
	
	// Visual Quantization 
	if(GQuartzVisualsEnableLogs)
	{
		UE_LOG(LogQuartzVisuals, Log, TEXT("-----------"));
		UE_LOG(LogQuartzVisuals, Log, TEXT("Visual Beat Count %d : %d"), CurrentBeatCount, CurrentBeatCountFull);
	}

	for(int32 Index = 0; Index < QuartzVisualEntries.Num(); Index++)
	{
		SCOPE_CYCLE_COUNTER(STAT_QuartzVisualQuantization);
		TRACE_CPUPROFILER_EVENT_SCOPE(STAT_QuartzVisualQuantization)
		FQuartzVisualPulseEntry& PulseEntry = QuartzVisualEntries[Index];
		if((PulseEntry.Data.CurrentBeatCount - PulseEntry.Data.BeatOffset) == 0 && IsValid(PulseEntry.Data.Actor)/* && PulseEntry.Actor->Implements<IQuartzVisualsInterface>()*/)
		{
			if(IsValid(PulseEntry.Data.Actor))
			{
				// Force value to update to current desired amount almost all curves should start at 0.0f, but we  will have to play with it.
                PulseEntry.UpdateValue(1.0f);
                // Send out visual update.
				PulseEntry.State = EQuartzVisualPulseState::Start;
				IQuartzVisualsInterface::Execute_OnQuartzVisualUpdate(PulseEntry.Data.Actor, PulseEntry);
			}
		}

		if(PulseEntry.GetBeatProgress() == 1.0f || IsValid(PulseEntry.Data.Actor) == false)
		{
			CancelAndFinishQuartzVisualEntry(PulseEntry);
			if(QuartzVisualEntries.IsValidIndex(Index))
			{
				QuartzVisualEntries.RemoveAt(Index);
			}
           	Index--;
           	continue;
		}
		
		PulseEntry.Data.CurrentBeatCount++;
	}
}

DECLARE_CYCLE_STAT(TEXT("Quartz Visual Update"), STAT_QuartzVisualUpdate, STATGROUP_QuartzVisuals);
void UQuartzVisualSubsystem::UpdateQuartzVisualPulseEntries(float DeltaTime)
{
	for(int32 Index = 0; Index < QuartzVisualEntries.Num(); Index++)
	{
		FQuartzVisualPulseEntry& PulseEntry = QuartzVisualEntries[Index];
		if(PulseEntry.State != EQuartzVisualPulseState::ReadyToStart && IsValid(PulseEntry.Data.Actor))
		{
			SCOPE_CYCLE_COUNTER(STAT_QuartzVisualUpdate);
			TRACE_CPUPROFILER_EVENT_SCOPE(STAT_QuartzVisualUpdate)
			PulseEntry.UpdateValue(DeltaTime * DeltaTimeMultiplier);
			PulseEntry.State = EQuartzVisualPulseState::Updating;
			IQuartzVisualsInterface::Execute_OnQuartzVisualUpdate(PulseEntry.Data.Actor, PulseEntry);
		}
	}
}

void UQuartzVisualSubsystem::AddNewQuartzVisualPulse(AActor* InActor, FQuartzVisualPulseSettings QuantizedVisualPulseSettings, int32 BeatDuration, int32 BeatOffset, bool StopIfExists)
{
	if(UKismetSystemLibrary::DoesImplementInterface(InActor, UQuartzVisualsInterface::StaticClass()) == false)
	{
		UE_LOG(LogQuartzVisuals, Warning, TEXT("Does not implement quartz visual interface"));
		return;
	}
	
	FQuartzVisualPulseEntry NewEntry = FQuartzVisualPulseEntry();
	NewEntry.Settings = QuantizedVisualPulseSettings;
	NewEntry.Data.BeatOffset = BeatOffset;
	NewEntry.Data.BeatDuration = BeatDuration;
	NewEntry.Data.Actor = InActor;
	NewEntry.State = EQuartzVisualPulseState::ReadyToStart;

	const int32 Index = QuartzVisualEntries.IndexOfByPredicate([NewEntry] (const FQuartzVisualPulseEntry& VisualPulseSettings)
	{
		return VisualPulseSettings == NewEntry;
	});
	
	if(StopIfExists && QuartzVisualEntries.IsValidIndex(Index))
	{
		return;
	}
	
	if(QuartzVisualEntries.IsValidIndex(Index))
	{
		// Cancel if the entry already exists.
		if(GQuartzVisualsEnableLogs)
		{
			UE_LOG(LogQuartzVisuals, Log, TEXT("-----------------------"));
			UE_LOG(LogQuartzVisuals, Log, TEXT("Add new entry %s"), *NewEntry.ToString());
			UE_LOG(LogQuartzVisuals, Log, TEXT("-----------------------"));
		}
		CancelAndFinishQuartzVisualEntry(QuartzVisualEntries[Index]);
		QuartzVisualEntries[Index] = NewEntry;
	}
	else
	{
		if(GQuartzVisualsEnableLogs)
		{
			UE_LOG(LogQuartzVisuals, Log, TEXT("-----------------------"));
			UE_LOG(LogQuartzVisuals, Log, TEXT("Add new quarts entry %s"), *NewEntry.ToString());
			UE_LOG(LogQuartzVisuals, Log, TEXT("-----------------------"));
		}
		QuartzVisualEntries.Add(NewEntry);
	}

}

void UQuartzVisualSubsystem::RemoveQuartzVisualPulseFromActor(int32 Index, int32 IndexFilter, AActor* InActor)
{
	for(int32 EntryIndex = 0; EntryIndex < QuartzVisualEntries.Num(); EntryIndex++)
	{
		FQuartzVisualPulseEntry& QuantizedVisual = QuartzVisualEntries[EntryIndex];
		if(QuantizedVisual.Data.Actor == InActor && QuantizedVisual.Settings.Index == Index && QuantizedVisual.Settings.IndexFilter == IndexFilter)
		{
			CancelAndFinishQuartzVisualEntry(QuantizedVisual);
			QuartzVisualEntries.RemoveAt(EntryIndex);
			EntryIndex--;
			continue;
		}
	}
}

void UQuartzVisualSubsystem::RemoveAllQuartzVisualPulsesFromActor(AActor* InActor, TArray<int32> ExcludeIndexFilters)
{
	for(int32 Index = 0; Index < QuartzVisualEntries.Num(); Index++)
	{
		FQuartzVisualPulseEntry& QuantizedVisual = QuartzVisualEntries[Index];
		if(QuantizedVisual.Data.Actor == InActor && ExcludeIndexFilters.Contains(QuantizedVisual.Settings.IndexFilter) == false)
		{
			CancelAndFinishQuartzVisualEntry(QuantizedVisual);
			QuartzVisualEntries.RemoveAt(Index);
			Index--;
			continue;
		}
	}
}

void UQuartzVisualSubsystem::RemoveAllQuartzVisualPulses()
{
	for(int32 Index = 0; Index < QuartzVisualEntries.Num(); Index++)
	{
		FQuartzVisualPulseEntry& QuantizedVisual = QuartzVisualEntries[Index];
		CancelAndFinishQuartzVisualEntry(QuantizedVisual);
		QuartzVisualEntries.RemoveAt(Index);
		Index--;
		continue;
	}
}

void UQuartzVisualSubsystem::CancelAndFinishQuartzVisualEntry(FQuartzVisualPulseEntry& QuartzVisualPulseEntry)
{
	if(IsValid(QuartzVisualPulseEntry.Data.Actor))
	{

		// Force to the end
		if(GQuartzVisualsEnableLogs)
		{
			UE_LOG(LogQuartzVisuals, Log, TEXT("-----------------------"));
			UE_LOG(LogQuartzVisuals, Log, TEXT("Finish entry %s"), *QuartzVisualPulseEntry.ToString());
			UE_LOG(LogQuartzVisuals, Log, TEXT("-----------------------"));
		}

		//TODO add bool to allow playing start events when skipped
		//if(QuartzVisualPulseEntry.State == EQuartzVisualPulseState::QuartzVisualPulseState_ReadyToStart)
		//{
		//	// Force a start event if it hasn't started yet.
		//	QuartzVisualPulseEntry.Data.CurrentBeatCount = QuartzVisualPulseEntry.Data.BeatOffset;
		//	QuartzVisualPulseEntry.UpdateValue(1.0f);
		//	QuartzVisualPulseEntry.State = EQuartzVisualPulseState::QuartzVisualPulseState_Start;
		//	QuartzVisualPulseEntry.Data.ActorInterface->OnQuartzVisualUpdate(QuartzVisualPulseEntry);
		//}

		QuartzVisualPulseEntry.Data.CurrentBeatCount += 1000;
		// Force the end event.
		QuartzVisualPulseEntry.UpdateValue(1000.0f);
		QuartzVisualPulseEntry.State = EQuartzVisualPulseState::Finished;
		IQuartzVisualsInterface::Execute_OnQuartzVisualUpdate(QuartzVisualPulseEntry.Data.Actor, QuartzVisualPulseEntry);
		//QuartzVisualPulseEntry.Data.Actor->Execute(QuartzVisualPulseEntry);
	}
}
PRAGMA_ENABLE_OPTIMIZATION
