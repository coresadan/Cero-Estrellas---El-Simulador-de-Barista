#pragma once
#include "CoreMinimal.h"

// Authored joint keys for a one-stride cycle. Phase follows travelled distance;
// this keeps feet and arm swing from running while a visitor is blocked.
namespace CEGuestMotion {
struct FKey { float Hip,Knee,Ankle,Arm; };
inline FKey Stride(float Phase) {
    static const FKey Keys[]={
        {22,4,-10,-15},{10,0,-5,-9},{-7,6,4,5},{-23,25,12,15},
        {-12,48,5,12},{13,42,-8,0},{29,22,-14,-13},{27,8,-12,-18},
        {22,4,-10,-15}
    };
    const float Index=FMath::Fmod(FMath::Max(0.f,Phase),2.f*PI)/(2.f*PI)*8.f;
    const int32 I=FMath::FloorToInt(Index); const float A=FMath::SmoothStep(0.f,1.f,Index-I);
    return {FMath::Lerp(Keys[I].Hip,Keys[I+1].Hip,A),FMath::Lerp(Keys[I].Knee,Keys[I+1].Knee,A),FMath::Lerp(Keys[I].Ankle,Keys[I+1].Ankle,A),FMath::Lerp(Keys[I].Arm,Keys[I+1].Arm,A)};
}
}
