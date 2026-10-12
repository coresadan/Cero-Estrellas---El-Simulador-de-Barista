#pragma once
#include "CoreMinimal.h"
#include "Kismet/BlueprintFunctionLibrary.h"
#include "CECharacterAssets.generated.h"
class USkeletalMesh;

// Editor import helper: persist the skeleton in the mesh's own asset package.
UCLASS()
class CEROESTRELLAS_API UCECharacterAssets : public UBlueprintFunctionLibrary
{
    GENERATED_BODY()
public:
    UFUNCTION(BlueprintCallable,Category="CeroEstrellas|Authoring")
    static bool PrepareSkeleton(USkeletalMesh* Mesh);
};
