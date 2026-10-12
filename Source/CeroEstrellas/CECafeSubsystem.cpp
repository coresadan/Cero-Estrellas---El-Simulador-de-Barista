#include "CEGameplay.h"
#include "CEBossLines.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "CEKitchen.h"
#include "Components/AudioComponent.h"
#include "Kismet/KismetMathLibrary.h"
#include "Sound/SoundConcurrency.h"
#include "EngineUtils.h"

void UCECafeSubsystem::Subtitle(const FString& Id,const FString& Name,const FString& Text,float Duration)
{
    auto* Player=UGameplayStatics::GetPlayerPawn(GetWorld(),0); if(!Player) return;
    // Nearby speech is captioned once, instead of text billboards over every head.
    if(auto* Mode=GetWorld()->GetAuthGameMode()) {
        for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(It->CustomerId==Id) {
            if(FVector::Dist2D(It->GetActorLocation(),Player->GetActorLocation())>450.f) return;
            break;
        }
    }
    SubtitleCustomer=Id; CustomerSubtitle=Name+TEXT(": ")+Text.Replace(TEXT("\n"),TEXT(" "));
    CustomerSubtitleUntil=GetWorld()->GetTimeSeconds()+FMath::Clamp(Duration,2.f,10.f);
}

void UCECafeSubsystem::Notify(const FString& Text)
{
    Feedback=Text; FeedbackUntil=GetWorld()->GetTimeSeconds()+5.f;
}
void UCECafeSubsystem::Sound(const TCHAR* Name, FVector Location, float Volume)
{
    const float Now=GetWorld()->GetTimeSeconds(); const FName Key(Name);
    const float Gap=Key==TEXT("Splash") ? .14f : Key==TEXT("Thump") ? .08f : .03f;
    if(const float* At=SoundTimes.Find(Key)) if(Now-*At<Gap) return;
    SoundTimes.Add(Key,Now);
    const FString Path=FString::Printf(TEXT("/Game/Prototype/Audio/FX/%s.%s"),Name,Name);
    if(auto* Wave=LoadObject<USoundBase>(nullptr,*Path))
    {
        auto* Spatial=NewObject<USoundAttenuation>(this);
        Spatial->Attenuation.bAttenuate=true; Spatial->Attenuation.bSpatialize=true;
        Spatial->Attenuation.AttenuationShapeExtents=FVector(100.f); Spatial->Attenuation.FalloffDistance=700.f;
        UGameplayStatics::PlaySoundAtLocation(GetWorld(),Wave,Location,Volume,FMath::FRandRange(.92f,1.08f),0.f,Spatial);
    }
}
void UCECafeSubsystem::Observe(FName Kind, const FString& CustomerId, const FString& ObjectName, float Severity)
{
    const float Now=GetWorld()->GetTimeSeconds();
    DisasterScore=FMath::Max(0.f,DisasterScore-(Now-LastScoreAt)*.16f);
    if(Now-LastScoreAt>12.f) RecentVictims.Reset();
    LastScoreAt=Now;
    FCEChaosEvent Event;
    Event.EventId=FString::Printf(TEXT("chaos-%06d"),++EventSerial);
    Event.Kind=Kind; Event.CustomerId=CustomerId; Event.ObjectName=ObjectName; Event.Severity=FMath::Clamp(Severity,0.f,1.f);
    const bool bIncident=Kind==TEXT("coffee_spill") || Kind==TEXT("customer_hit") || Kind==TEXT("laptop_destroyed") || Kind==TEXT("ceramic_broken") || Kind==TEXT("customer_slipped") || Kind==TEXT("customer_departed") || Kind==TEXT("customer_fallen") || Kind==TEXT("chain_reaction") || Kind==TEXT("disaster_filmed");
    if(bIncident)
    {
        ++ChaosCount; DisasterScore=FMath::Min(10.f,DisasterScore+Event.Severity);
        // A stream is one continuing incident, rather than one combo per droplet.
        const FString ComboKey=Kind.ToString()+TEXT(":")+CustomerId;
        const float* Previous=ComboEvents.Find(ComboKey);
        if(!Previous || Now-*Previous>3.f)
        {
            Combo=Now<ComboUntil ? FMath::Min(12,Combo+1) : 1; ComboUntil=Now+9.f;
            BestCombo=FMath::Max(BestCombo,Combo); ViralPoints+=FMath::RoundToInt(Severity*100.f)*Combo;
            ComboEvents.Add(ComboKey,Now);
            if(ComboEvents.Num()>128) for(auto It=ComboEvents.CreateIterator();It;++It) if(Now-It.Value()>12.f) It.RemoveCurrent();
        }
        if(!CustomerId.IsEmpty()) RecentVictims.Add(CustomerId);
        LastIncident=Kind==TEXT("customer_departed") ? TEXT("Otro cliente exige tu despido. Recursos humanos no responde.") : TEXT("El desastre está creciendo. La jefa sigue mirando las métricas.");
    }
    else if(Kind==TEXT("shift_started")) LastIncident=TEXT("Toma pedidos, prepara desayunos... y busca tu salida laboral.");
    OnChaosObserved.Broadcast(Event);
    UE_LOG(LogTemp,Display,TEXT("CE_CHAOS id=%s kind=%s customer=%s severity=%.2f"),*Event.EventId,*Kind.ToString(),*CustomerId,Event.Severity);
    const bool bMajor=(Kind==TEXT("customer_departed") && Severity>=.9f) || Kind==TEXT("laptop_destroyed") || (RecentVictims.Num()>=3 && DisasterScore>=3.f);
    if(bMajor && Now>=BossCooldownUntil && Now>=BossSubtitleUntil)
    {
        Praise(Kind); DisasterScore=0.f; RecentVictims.Reset();
    }
}
void UCECafeSubsystem::ConfirmDeparture(const FString& CustomerId, bool bDisaster)
{
    if(Departures.Contains(CustomerId)) return;
    Departures.Add(CustomerId);
    if(bDisaster) { ++DepartedCustomers; Observe(TEXT("customer_departed"),CustomerId,TEXT(""),1.f); }
    else Observe(TEXT("customer_finished"),CustomerId,TEXT(""),0.f);
}
void UCECafeSubsystem::TickRadio()
{
    const float Now=GetWorld()->GetTimeSeconds();
    const int32 Phase=Now<90.f ? 0 : Now<240.f ? 1 : Now<420.f ? 2 : 3;
    ShiftPhase=Phase;
    const TCHAR* Phases[]={TEXT("APERTURA / Un desayuno tranquilo"),TEXT("HORA PUNTA / Todo el mundo tiene prisa"),TEXT("VIRAL / Vienen buscando tu peor servicio"),TEXT("CIERRE / Recursos humanos sigue sin responder")};
    ShiftTitle=Phases[Phase];
    if(Phase>AnnouncedPhase && Now<ShiftDuration) { AnnouncedPhase=Phase; Notify(Phase==1 ? TEXT("Hora punta. Llegan pedidos especiales y clientes con menos paciencia.") : Phase==2 ? TEXT("Tu servicio se ha hecho viral. Los clientes vienen buscando el desastre.") : TEXT("Último minuto. Tu solicitud de despido sigue en evaluación.")); }
    if(!bShiftFinished && Now>=ShiftDuration) { bShiftFinished=true; Notify(FString::Printf(TEXT("TURNO COMPLETADO / %d desayunos, %d huidas, %d puntos virales. Despido denegado: ascenso concedido. Sigue jugando o R para otro turno."),OrdersServed,DepartedCustomers,ViralPoints)); FeedbackUntil=Now+18.f; }
    if(!Ambience) {
        auto* Wave=LoadObject<USoundBase>(nullptr,TEXT("/Game/Prototype/Audio/FX/CafeAmbience.CafeAmbience"));
        if(Wave) Ambience=UGameplayStatics::SpawnSound2D(GetWorld(),Wave,.22f,1.f,0.f,nullptr,false,true);
    }
    if(GetWorld()->GetTimeSeconds()>ComboUntil) Combo=0;
    const bool Complete=ChallengeStep==0 ? OrdersServed>=2 : ChallengeStep==1 ? Liquids && Liquids->FloorMl>=150.f : ChallengeStep==2 ? Falls>=1 : BestCombo>=4;
    if(Complete && ChallengeStep<4)
    {
        ++ChallengeStep; ViralPoints+=250;
        if(ChallengeStep==4) Notify(TEXT("¡Sabotaje certificado! La empresa te asciende. Despido: denegado."));
        else Notify(TEXT("Objetivo cumplido. Bonus corporativo +250. Ahora sube el desastre."));
    }
    const TCHAR* Challenges[]={TEXT("Sirve 2 desayunos para financiar tu sabotaje"),TEXT("Derrama 150 ml: prepara la pista de patinaje"),TEXT("Haz caer a un cliente: charco, silla o empujón"),TEXT("Encadena 4 incidentes en menos de 9 s entre ellos"),TEXT("Ascenso involuntario. Sigue saboteando el turno")};
    Challenge=Challenges[FMath::Clamp(ChallengeStep,0,4)];
    ShiftLevel=FMath::Clamp(1+ViralPoints/1500+ShiftPhase,1,5);
}
void UCECafeSubsystem::Praise(FName Kind)
{
    const auto Lines=CEBoss::Lines();
    if(UnusedPhrases.IsEmpty())
    {
        for(int32 i=1;i<Lines.Num();++i) if(i!=2) UnusedPhrases.Add(i);
        for(int32 i=UnusedPhrases.Num()-1;i>0;--i) UnusedPhrases.Swap(i,FMath::RandRange(0,i));
        if(UnusedPhrases.Last()==LastBossPhrase) UnusedPhrases.Swap(0,UnusedPhrases.Num()-1);
    }
    // Pick a matching recording when available, retaining the no-repeat bag.
    const TArray<int32> Preferred=Kind==TEXT("laptop_destroyed") ? TArray<int32>{10,11,14} : Kind==TEXT("customer_departed") ? TArray<int32>{3,4,7,13,16} : TArray<int32>{9,12,15,17};
    for(int32 i=0;i<UnusedPhrases.Num();++i) if(Preferred.Contains(UnusedPhrases[i]) && UnusedPhrases[i]!=LastBossPhrase) { UnusedPhrases.Swap(i,UnusedPhrases.Num()-1); break; }
    const int32 Index=UnusedPhrases.Pop(); LastBossPhrase=Index; BossSubtitle=Lines[Index];
    auto* Voice=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Prototype/Audio/Boss_%d.Boss_%d"),Index,Index));
    const float Duration=Voice ? Voice->GetDuration() : 9.f;
    if(Voice) UGameplayStatics::PlaySound2D(GetWorld(),Voice,.8f);
    BossSubtitleUntil=GetWorld()->GetTimeSeconds()+Duration+.5f;
    BossCooldownUntil=FMath::Max(GetWorld()->GetTimeSeconds()+50.f,BossSubtitleUntil+25.f);
    ++PraiseCount;
    UE_LOG(LogTemp,Display,TEXT("CE_BOSS phrase=%d praise=%d voice=%s cooldown_until=%.2f"),Index,PraiseCount,Voice ? TEXT("loaded") : TEXT("missing"),BossCooldownUntil);
}
