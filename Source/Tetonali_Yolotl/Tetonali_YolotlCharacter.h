// Copyright Epic Games, Inc. All Rights Reserved.

#pragma once

#include "CoreMinimal.h"
#include "GenericTeamAgentInterface.h"
#include "GameFramework/Character.h"
#include "Tetonali_YolotlCharacter.generated.h"

/**
 *  A controllable top-down perspective character
 */
UCLASS(abstract)
class TETONALI_YOLOTL_API ATetonali_YolotlCharacter : public ACharacter, public IGenericTeamAgentInterface
{
	GENERATED_BODY()

private:
	//ID for the Team Affiliation
	UPROPERTY(EditDefaultsOnly, Category = "AI|Team")
	uint8 TeamID = 0;

public:

	/** Constructor */
	ATetonali_YolotlCharacter();

	/** Initialization */
	virtual void BeginPlay() override;

	/** Update */
	virtual void Tick(float DeltaSeconds) override;
	
	//Setter for the ID on Team Affiliation
	virtual void SetGenericTeamId(const FGenericTeamId& NewTeamID) override;
	
	//Getter for the ID on Team Affiliation
	virtual FGenericTeamId GetGenericTeamId() const override;
};

