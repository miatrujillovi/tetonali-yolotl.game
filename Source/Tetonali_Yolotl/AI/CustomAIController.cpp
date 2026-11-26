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
	SightConfig->SightRadius = 1500.0f;
	SightConfig->LoseSightRadius = 2000.0f;
	SightConfig->PeripheralVisionAngleDegrees = 60.0f;

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
	const ATetonali_YolotlCharacter* OtherCharacter = Cast<ATetonali_YolotlCharacter>(&Other);
	if (!OtherCharacter)
	{
		return ETeamAttitude::Neutral;
	}

	const FGenericTeamId MyTeamId = GetGenericTeamId();
	const FGenericTeamId OtherTeamId = FGenericTeamId::GetTeamIdentifier(&Other);

	if (OtherTeamId == FGenericTeamId::NoTeam)
	{
		return ETeamAttitude::Neutral;
	}

	return (MyTeamId == OtherTeamId) ? ETeamAttitude::Friendly : ETeamAttitude::Hostile;
}

TArray<FNearbyAgentData> ACustomAIController::GetNearbyAllies() const
{
	return NearbyAllies;
}

TArray<FNearbyAgentData> ACustomAIController::GetNearbyEnemies() const
{
	return NearbyEnemies;
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
	if (!Actor || Actor == this)
	{
		return;
	}
	
	const bool bSensed = Stimulus.WasSuccessfullySensed();
	UpdateNearbyAgentList(Actor, bSensed);
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
	
	// Solo nos interesa el jugador; cualquier otro actor se ignora.
	/*const APawn* PerceivedPawn = Cast<APawn>(Actor);
	if (!PerceivedPawn || !PerceivedPawn->IsPlayerControlled())
	{
		return;
	}

	if (Stimulus.WasSuccessfullySensed())
	{
		// Lo ve: cancela cualquier "perdida" pendiente, guarda el objetivo y marca que lo tiene.
		GetWorldTimerManager().ClearTimer(LoseSightTimer);
		TargetActor = Actor;
		bHasTarget = true;
	}
	else if (bHasTarget && !GetWorldTimerManager().IsTimerActive(LoseSightTimer))
	{
		// Lo dejo de ver: espera LoseSightGraceTime antes de darlo por perdido,
		GetWorldTimerManager().SetTimer(LoseSightTimer, this, &ACustomAIController::ClearPlayerSight, LoseSightGraceTime, false);
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


