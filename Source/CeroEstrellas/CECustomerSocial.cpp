#include "CEGameplay.h"
#include "CEKitchen.h"
#include "CESocialSpeech.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

void ACECustomer::TickSocial(float Delta)
{
    const float Now=GetWorld()->GetTimeSeconds();
    if(bLeaving || bPhysicalFall || Now<FallenUntil) return;
    auto* Player=Cast<ACEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    if(!Player) return;
    const bool Threat=Player->Held && (Player->bPouring || (Player->Held->IsContainer() && Anger>.3f));
    const float Distance=FVector::Dist2D(Player->GetActorLocation(),GetActorLocation());
    Suspicion=FMath::FInterpTo(Suspicion,Threat && Distance<200.f ? 1.f : 0.f,Delta,2.f);
    if(bSeated && Suspicion>.5f && Now>=NextDefendAt) {
        bool Moved=false;
        for(TActorIterator<ACEProp> It(GetWorld());It;++It) {
            const bool Owned=It->Tags.Contains(FName(*FString::Printf(TEXT("seat%d"),SeatSlot)));
            if((Owned && It->IsDevice()) || *It==ServedCup.Get()) {
                if(It->bHeld || It->bBroken) continue;
                const FVector Move(0,SeatSlot%2 ? 27.f : -27.f,0);
                if(It->Body->IsSimulatingPhysics()) It->Body->AddImpulse(Move*1.5f,NAME_None,true);
                else It->SetActorLocation(It->GetActorLocation()+Move,true);
                if(*It==ServedCup.Get()) ServedCupPosition=It->GetActorLocation();
                Moved=true;
            }
        }
        if(Moved) { NextDefendAt=Now+9.f; ProtectUntil=Now+1.7f; ++DefensiveMoves; ImmediateVoice(TEXT("defend")); Speak(Personality==0 ? TEXT("Mi portátil queda fuera de tu zona de riesgo.") : TEXT("Voy a apartar mi desayuno. Por si acaso."),3.f); }
    }
    if(bSeated && WetMl>30.f && Now>=NextSeatAdjustAt && IsValid(Seat) && Seat->GetActorUpVector().Z>.7f) {
        NextSeatAdjustAt=Now+25.f;
        const FVector Offset(0,SeatSlot%2 ? 14.f : -14.f,0); FHitResult Hit;
        const FVector Original=Seat->GetActorLocation();
        Seat->SetActorLocation(Original+Offset,true,&Hit);
        if(!Hit.bBlockingHit) { SetActorLocation(GetActorLocation()+Offset,true); ++SeatAdjustments; WipeUntil=Now+3.f; }
        Speak(TEXT("Voy a apartar la silla y secarme. Este servicio salpica."),3.f);
    }
    if(Now>=NextSocialAt && WitnessCount>0 && State==ECECustomerState::Waiting) {
        NextSocialAt=Now+22.f;
        for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(*It!=this && !It->bLeaving && It->State==ECECustomerState::Waiting && FVector::Dist2D(It->GetActorLocation(),GetActorLocation())<450.f) {
            FHitResult Hit; FCollisionQueryParams Q; Q.AddIgnoredActor(this); Q.AddIgnoredActor(*It);
            if(GetWorld()->LineTraceSingleByChannel(Hit,HeadLocation(),It->HeadLocation(),ECC_Visibility,Q)) continue;
            ++SocialExchanges; ++It->SocialExchanges;
            ImmediateVoice(TEXT("chat"));
            Speak(Personality==2 ? TEXT("¿Puedes salir en mi directo? Esto necesita testigos.") : TEXT("¿A ti también te han servido una crisis existencial?"),4.f);
            CESocial::Reply(*It,It->Personality==1 ? TEXT("Lo llamaré degustación del desencanto.") : TEXT("Estoy esperando el café, no una entrevista."));
            It->NextSocialAt=Now+22.f; break;
        }
    }
    // Peaceful conversations also happen: turn-taking, visibility and cooldown.
    if(LookIndex>=0 && Now>=NextSocialAt && (State==ECECustomerState::Waiting || State==ECECustomerState::Eating) && Now>DialogueHoldUntil) {
        NextSocialAt=Now+24.f;
        for(TActorIterator<ACECustomer> It(GetWorld());It;++It) {
            auto* Other=*It; if(Other==this || Other->LookIndex<0 || Other->bLeaving || Other->bPhysicalFall || Now<Other->DialogueHoldUntil || Now<Other->NextSocialAt || FVector::Dist2D(Other->GetActorLocation(),GetActorLocation())>450.f) continue;
            FHitResult Hit; FCollisionQueryParams Q; Q.AddIgnoredActor(this); Q.AddIgnoredActor(Other);
            if(GetWorld()->LineTraceSingleByChannel(Hit,HeadLocation(),Other->HeadLocation(),ECC_Visibility,Q)) continue;
            ++CasualExchanges; ++Other->CasualExchanges; SocialGestureUntil=Other->SocialGestureUntil=Now+4.f; Other->NextSocialAt=Now+24.f;
            ImmediateVoice(TEXT("chat"));
            Speak(StoryId==0 ? TEXT("¿Tolerarías trabajar aquí? Es para mi entrevista.") : StoryId==4 ? TEXT("Necesito una rima para latte. Se admiten malas decisiones.") : TEXT("¿Has venido por el café o por las reseñas terribles?"),5.f);
            CESocial::Reply(Other,Other->LookIndex==7 ? TEXT("Yo he venido por el after. La máquina tiene buen ritmo.") : TEXT("Por las reseñas. Quería comprobar si eran tan buenas como dicen."));
            if(StoryId==1 && SocialPartnerId.IsEmpty()) PairWith(Other);
            break;
        }
    }
    // A suspicious waiting customer steps away; the seat identity and route
    // remain stable, so dodging cannot steal another visitor's table.
    if(!bSeated && State==ECECustomerState::Ordering && Suspicion>.8f && Now>=NextDefendAt) {
        NextDefendAt=Now+8.f;
        FVector Away=(GetActorLocation()-Player->GetActorLocation()).GetSafeNormal2D();
        FHitResult Hit; SetActorLocation(GetActorLocation()+Away*24.f,true,&Hit);
        if(!Hit.bBlockingHit) { ++DefensiveMoves; Speak(TEXT("Te escucho desde aquí. Mantén la taza lejos."),3.f); }
    }
}
