// Fill out your copyright notice in the Description page of Project Settings.

#pragma once

#include "CoreMinimal.h"
#include "AIController.h"
#include "Perception/AIPerceptionTypes.h"
#include "Perception/AISenseConfig_Sight.h"
#include "GenericTeamAgentInterface.h"
#include "CustomAIController.generated.h"

class UStateTree;
class UStateTreeAIComponent;

class UAIPerceptionComponent;
class UAISenseConfig_Sight;

USTRUCT(BlueprintType)
struct FNearbyAgentData
{
	GENERATED_BODY()

	UPROPERTY(BlueprintReadWrite)
	TObjectPtr<AActor> Agent = nullptr;

	UPROPERTY(BlueprintReadWrite)
	TEnumAsByte<ETeamAttitude::Type> Attitude = ETeamAttitude::Neutral;

	UPROPERTY(BlueprintReadWrite)
	float Distance = 0.f;
};

UCLASS()
class TETONALI_YOLOTL_API ACustomAIController : public AAIController
{
	GENERATED_BODY()
	
public:
	ACustomAIController();
	
	// Percibe al jugador. Lo utiliza el Evaluator y el contexto EQS EQC_PlayerTarget.
	UFUNCTION(BlueprintPure, Category = "AI|Perception")
	AActor* GetTargetActor() const { return TargetActor; }

	// True mientras el NPC tiene al objetivo a la vista.
	UFUNCTION(BlueprintPure, Category = "AI|Perception")
	bool HasTarget() const { return bHasTarget; }
	
	//Setter for the ID on Team Affiliation
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	
	//Getter for the ID on Team Affiliation
	virtual FGenericTeamId GetGenericTeamId() const override;
	
	//Getter for the Attitude the AI should have towards certain affiliation.
	virtual ETeamAttitude::Type GetTeamAttitudeTowards(const AActor& Other) const override;
	
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Perception")
	TArray<FNearbyAgentData> NearbyAllies;

	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Perception")
	TArray<FNearbyAgentData> NearbyEnemies;
	
	TArray<FNearbyAgentData> GetNearbyAllies() const;
	
	TArray<FNearbyAgentData> GetNearbyEnemies() const;

	int32 GetNearbyAllyCount() const;

	int32 GetNearbyEnemyCount() const;
	
protected:
	virtual void BeginPlay() override;
	
	// Se ejecuta cuando el jugador entra o sale del cono de vision.
	UFUNCTION()
	void HandleTargetPerceptionUpdated(AActor* Actor, FAIStimulus Stimulus);

	// Se ejecuta cuando termina el tiempo de gracia sin volver a ver al jugador.
	void ClearPlayerSight();

	// Componente que ejecuta el StateTree.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI")
	TObjectPtr<UStateTreeAIComponent> StateTreeAIComponent;

	// Componente de percepcion (los "sentidos" del NPC).
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAIPerceptionComponent> AIPerception;

	// Configuracion del sentido de vista.
	UPROPERTY(VisibleAnywhere, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<UAISenseConfig_Sight> SightConfig;

	// Segundos que espera antes de dar al jugador por perdido (evita que el NPC tiemble).
	UPROPERTY(EditAnywhere, BlueprintReadOnly, Category = "AI|Perception", meta = (ClampMin = "0.0", UIMin = "0.0"))
	float LoseSightGraceTime = 1.0f;

	FTimerHandle LoseSightTimer;

	// Ultimo objetivo percibido; se actualiza solo en OnTargetPerceptionUpdated.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Perception")
	TObjectPtr<AActor> TargetActor = nullptr;

	// True mientras el NPC ve al objetivo.
	UPROPERTY(VisibleInstanceOnly, BlueprintReadOnly, Category = "AI|Perception")
	bool bHasTarget = false;
	
	//ID for the Team Affiliation
	UPROPERTY(EditDefaultsOnly, Category = "AI|Team")
	uint8 TeamID = 1;
	
	virtual void Tick(float DeltaSeconds) override;
	
	void UpdateNearbyAgentList(AActor* Actor, bool bSensed);
	
	void RefreshNearbyAgentDistances();
};
