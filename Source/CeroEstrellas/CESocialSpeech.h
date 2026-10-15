#pragma once
#include "CEGameplay.h"
#include "Engine/World.h"
#include "TimerManager.h"

namespace CESocial
{
    inline void Reply(ACECustomer* Customer,const FString& Text)
    {
        if(!Customer) return;
        const TWeakObjectPtr<ACECustomer> Weak(Customer);
        FTimerHandle Handle;
        Customer->GetWorld()->GetTimerManager().SetTimer(Handle,[Weak,Text] {
            auto* Guest=Weak.Get(); if(!Guest || Guest->bLeaving || Guest->bPhysicalFall || Guest->GetWorld()->GetTimeSeconds()<Guest->DialogueHoldUntil) return;
            Guest->Speak(Text,4.f); Guest->ImmediateVoice(TEXT("chat"));
        },4.f,false);
    }
}
