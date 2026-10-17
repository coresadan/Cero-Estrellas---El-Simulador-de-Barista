#pragma once
#include "CoreMinimal.h"

// Seat identity is shared by routes, serving, furniture and visitor possessions.
namespace CECafeLayout
{
    constexpr int32 TableCount=6;
    constexpr float EastWall=1480.f;
    inline FVector Table(int32 Slot)
    {
        const float Y[]={-100.f,220.f,-405.f};
        Slot=FMath::Clamp(Slot,0,TableCount-1);
        return FVector(Slot<3 ? 355.f : 1040.f,Y[Slot%3],78.f);
    }
    inline FVector Chair(int32 Slot) { return Table(Slot)+FVector(95,0,-26); }
    inline FVector Guest(int32 Slot) { return Table(Slot)+FVector(105,0,14); }
    inline FVector Device(int32 Slot) { return Table(Slot)+FVector(-15,0,7); }
    inline FVector Food(int32 Slot,bool bCup) { return Table(Slot)+FVector(30,bCup ? -28.f : 25.f,bCup ? 17.f : 10.f); }
    inline float Aisle(int32 Slot) { return Slot<3 ? 620.f : 1290.f; }
    inline float OrderY(int32 Slot) { const float Y[]={-100.f,220.f,-405.f,70.f,365.f,-255.f}; return Y[FMath::Clamp(Slot,0,TableCount-1)]; }
}
