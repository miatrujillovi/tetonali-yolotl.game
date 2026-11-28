// Fill out your copyright notice in the Description page of Project Settings.


#include "AI/CustomAIController.h"

#include "Tetonali_YolotlCharacter.h"
#include "Components/StateTreeAIComponent.h"
#include "TimerManager.h"
#include "Perception/AIPerceptionComponent.h"
#include "Perception/AISenseConfig_Sight.h"

ACustomAIController::ACustomAIController()
{
	PrimaryActorTick.bCanEverTick = true;
	
	// Crea el componente que ejecuta el StateTree.
	StateTreeAIComponent = CreateDefaultSubobject<UStateTreeAIComponent>("StateTreeAIComponent");

	// Crea el componente de percepcion y la configuracion de vista.
	AIPerception = CreateDefaultSubobject<UAIPerceptionComponent>("AIPerception");
	SightConfig = CreateDefaultSubobject<UAISenseConfig_Sight>("SightConfig");

	// Que tan lejos ve, a que distancia lo pierde y el angulo del cono de vision.
	SightConfig->SightRadius = 2000.0f;
	SightConfig->LoseSightRadius = 2500.0f;
	SightConfig->PeripheralVisionAngleDegrees = 180.0f;

	// Detecta a cualquier actor, sin importar su equipo.
	SightConfig->DetectionByAffiliation.bDetectEnemies = true;
	SightConfig->DetectionByAffiliation.bDetectNeutrals = true;
	SightConfig->DetectionByAffiliation.bDetectFriendlies = true;

	// Registra el sentido de vista y usa este componente como la percepcion del controller.
	AIPerception->ConfigureSense(*SightConfig);
	SetPerceptionComponent(*AIPerception);
}

void ACustomAIController::SetGenericTeamId(const FGenericTeamId& NewTeamID)
{
	TeamID = NewTeamID.GetId();
}

FGenericTeamId ACustomAIController::GetGenericTeamId() const
{
	return FGenericTeamId(TeamID);
}

ETeamAttitude::Type ACustomAIController::GetTeamAttitudeTowards(const AActor& Other) const
{
	//1. We get the ID of the current AIController and returns a Neutral Attitude if the ID represents "NoTeam"
	const FGenericTeamId MyTeamId = GetGenericTeamId(); 
	if (MyTeamId == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}
	
	//2. Checks the ID of the detected actor, if not team is found, checks the TeamID of its controller
	FGenericTeamId OtherTeamId = FGenericTeamId::GetTeamIdentifier(&Other);
	if (OtherTeamId == FGenericTeamId::NoTeam)
	{
		if (const APawn* OtherPawn = Cast<APawn>(&Other))
		{
			if (const AController* OtherController = OtherPawn->GetController())
			{
				OtherTeamId = FGenericTeamId::GetTeamIdentifier(OtherController);
			}
		}
	}
	
	//3. If the detected actor ID is equal to "NoTeam" set to neutral attitude
	if (OtherTeamId == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}
	
	//4. If the IDs are the same return Friendly attitude, otherwise, Hostile attitude.
	return (MyTeamId == OtherTeamId) ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
}

int32 ACustomAIController::GetNearbyAllyCount() const
{
	return NearbyAllies.Num();
}

int32 ACustomAIController::GetNearbyEnemyCount() const
{
	return NearbyEnemies.Num();
}

void ACustomAIController::BeginPlay()
{
	Super::BeginPlay();
	
	// Escucha el evento de percepcion: se dispara cuando el jugador entra o sale de la vista.
	if (AIPerception)
	{
		AIPerception->OnTargetPerceptionUpdated.AddDynamic(this, &ACustomAIController::HandleTargetPerceptionUpdated);
	}
}

void ACustomAIController::HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus)
{
	//1. If the actor is the actor itself, return
	if (!Actor || Actor == this)
	{
		return;
	}
	
	//2. If a Actor was successfully sensed, call to Update the Nearby Agent List
	const bool bSensed = Stimulus.WasSuccessfullySensed();
	UpdateNearbyAgentList(Actor, bSensed);
	
	//3. Get the Attitude of the other Actor, if Hostile, get the Actor has the TargetActor
	const ETeamAttitude::Type Attitude = GetTeamAttitudeTowards(*Actor);
	if (Attitude == ETeamAttitude::Hostile)
	{
		if (bSensed)
		{
			GetWorldTimerManager().ClearTimer(LoseSightTimer);
			TargetActor = Actor;
			bHasTarget = true;
		}
		else if (TargetActor == Actor)
		{
			GetWorldTimerManager().SetTimer(LoseSightTimer, this, &ACustomAIController::ClearPlayerSight, LoseSightGraceTime, false);
		}
	}
	
	const FGenericTeamId MyId = GetGenericTeamId();
	const FGenericTeamId OtherId = FGenericTeamId::GetTeamIdentifier(Actor);

	//Debugging
	/*const FString Msg = FString::Printf(
		TEXT("[%s] Sensed %s (MyTeam=%d, OtherTeam=%d, Att=%d) | Allies: %d | Enemies: %d"),
		*GetName(),
		*Actor->GetName(),
		MyId.GetId(),
		OtherId.GetId(),
		static_cast<int32>(Attitude),
		NearbyAllies.Num(),
		NearbyEnemies.Num()
	);

	UE_LOG(LogTemp, Warning, TEXT("%s"), *Msg);
	if (GEngine)
	{
		GEngine->AddOnScreenDebugMessage(-1, 2.5f, bSensed ? FColor::Green : FColor::Red, Msg);
	}*/
}

void ACustomAIController::ClearPlayerSight()
{
	// Paso el tiempo de gracia sin volver a verlo: ahora si se pierde.
	TargetActor = nullptr;
	bHasTarget = false;
}

void ACustomAIController::Tick(float DeltaSeconds)
{
	Super::Tick(DeltaSeconds);
	
	RefreshNearbyAgentDistances();
}

void ACustomAIController::UpdateNearbyAgentList(AActor* Actor, bool bSensed)
{
	const ETeamAttitude::Type Attitude = GetTeamAttitudeTowards(*Actor);
	TArray<FNearbyAgentData>* TargetArray = nullptr;

	if (Attitude == ETeamAttitude::Friendly)
	{
		TargetArray = &NearbyAllies;
	}
	else if (Attitude == ETeamAttitude::Hostile)
	{
		TargetArray = &NearbyEnemies;
	}

	// Remove from both lists first to avoid duplicates or stale entries.
	NearbyAllies.RemoveAll([Actor](const FNearbyAgentData& Entry)
	{
		return Entry.Agent == Actor || !IsValid(Entry.Agent);
	});

	NearbyEnemies.RemoveAll([Actor](const FNearbyAgentData& Entry)
	{
		return Entry.Agent == Actor || !IsValid(Entry.Agent);
	});

	if (bSensed && TargetArray)
	{
		APawn* ControlledPawn = GetPawn();
		const float Dist = ControlledPawn ? FVector::Dist(ControlledPawn->GetActorLocation(), Actor->GetActorLocation()) : 0.f;

		FNearbyAgentData Entry;
		Entry.Agent = Actor;
		Entry.Attitude = Attitude;
		Entry.Distance = Dist;

		TargetArray->Add(Entry);
	}
}

void ACustomAIController::RefreshNearbyAgentDistances()
{
	APawn* ControlledPawn = GetPawn();
	if (!ControlledPawn)
	{
		return;
	}

	const FVector MyLocation = ControlledPawn->GetActorLocation();

	auto UpdateArray = [&MyLocation](TArray<FNearbyAgentData>& List)
	{
		for (int32 i = List.Num() - 1; i >= 0; --i)
		{
			if (!IsValid(List[i].Agent))
			{
				List.RemoveAt(i);
				continue;
			}
			List[i].Distance = FVector::Dist(MyLocation, List[i].Agent->GetActorLocation());
		}
	};

	UpdateArray(NearbyAllies);
	UpdateArray(NearbyEnemies);
}


