#pragma once

#include "CoreMinimal.h"

#include "MocapIdentity.generated.h"

USTRUCT(BlueprintType, Blueprintable)
struct FMocapIdentity
{
	GENERATED_BODY()

public:
	UPROPERTY(EditAnywhere, BlueprintReadWrite)
	FString IdentityName;
};
