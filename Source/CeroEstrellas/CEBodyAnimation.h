#pragma once
#include "CoreMinimal.h"
#include "Animation/AnimInstance.h"
#include "CEBodyAnimation.generated.h"

// Game-thread FK is copied by the proxy before animation workers evaluate.
UCLASS(Transient)
class CEROESTRELLAS_API UCEBodyAnimation : public UAnimInstance
{
    GENERATED_BODY()
public:
    TArray<FTransform> LocalPoseFrame;
protected:
    virtual FAnimInstanceProxy* CreateAnimInstanceProxy() override;
    virtual void DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) override;
};
