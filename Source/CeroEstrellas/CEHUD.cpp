#include "CEGameplay.h"
#include "CEKitchen.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "EngineUtils.h"

void ACEHUD::Label(const FString& Text,float X,float Y,float Scale,FLinearColor Color)
{
    DrawText(Text,Color,X,Y,GEngine->GetMediumFont(),Scale*1.6f,false);
}
void ACEHUD::Wrapped(const FString& Text,float X,float Y,float Width,float Scale,FLinearColor Color)
{
    TArray<FString> Words; Text.ParseIntoArray(Words,TEXT(" "),true); FString Line;
    for(const FString& Word:Words)
    {
        const FString Candidate=Line.IsEmpty() ? Word : Line+TEXT(" ")+Word;
        float W=0,H=0; GetTextSize(Candidate,W,H,GEngine->GetMediumFont(),Scale*1.6f);
        if(W>Width && !Line.IsEmpty()) { Label(Line,X,Y,Scale,Color); Y+=23.f*Scale; Line=Word; } else Line=Candidate;
    }
    if(!Line.IsEmpty()) Label(Line,X,Y,Scale,Color);
}
void ACEHUD::DrawHUD()
{
    Super::DrawHUD(); if(!Canvas) return;
    const float W=Canvas->SizeX,H=Canvas->SizeY,S=FMath::Clamp(W/1280.f,.7f,1.5f);
    const FLinearColor Cream(.98f,.91f,.76f), Orange(1.f,.46f,.22f), Muted(.64f,.79f,.73f), Ink(.018f,.055f,.046f,.88f);
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>(); auto* Player=Cast<ACEPlayer>(GetOwningPawn());
    DrawRect(Ink,20*S,20*S,370*S,100*S); DrawRect(Orange,20*S,20*S,4*S,100*S);
    Label(TEXT("CERO ESTRELLAS  /  TURNO ABIERTO"),38*S,31*S,1.f*S,Cream);
    Label(TEXT("Consigue que te despidan. Despido: 0%"),38*S,62*S,.75f*S,Orange);
    Label(FString::Printf(TEXT("Servidos %d   Errores %d   Han huido %d"),Cafe->OrdersServed,Cafe->WrongOrders,Cafe->DepartedCustomers),38*S,88*S,.69f*S,Muted);
    DrawRect(Ink,20*S,128*S,370*S,82*S);
    Label(FString::Printf(TEXT("VIRAL %d  /  NIVEL CORPORATIVO %d"),Cafe->ViralPoints,Cafe->ShiftLevel),38*S,138*S,.7f*S,Orange);
    Wrapped(Cafe->Challenge,38*S,164*S,335*S,.66f*S,Cream);
    if(Cafe->Combo>1)
    {
        Label(FString::Printf(TEXT("DESMADRE x%d"),Cafe->Combo),W/2-90*S,80*S,1.2f*S,Orange);
        DrawRect(Orange,W/2-90*S,114*S,180*S*FMath::Clamp((Cafe->ComboUntil-GetWorld()->GetTimeSeconds())/9.f,0.f,1.f),4*S);
    }
    DrawRect(Ink,W-324*S,20*S,304*S,70*S);
    Label(TEXT("PEDIDOS / DESAYUNOS"),W-306*S,31*S,.83f*S,Cream);
    Label(TEXT("F tomar nota / entregar · Q provocar"),W-306*S,60*S,.63f*S,Muted);
    int32 Row=0;
    for(TActorIterator<ACECustomer> It(GetWorld());It;++It)
    {
        if(It->bLeaving || It->State==ECECustomerState::Eating) continue;
        const float Y=(94+Row*84)*S;
        DrawRect(Ink,W-324*S,Y,304*S,80*S);
        const FString State=It->State==ECECustomerState::Arriving ? TEXT("Entrando...") : !It->bOrderKnown ? TEXT("Espera que tomes nota [ F ]") : It->OrderText();
        Label(FString::Printf(TEXT("MESA %d  %s"),It->SeatSlot+1,It->Anger>.5f ? TEXT("ENFADADO") : TEXT("CLIENTE")),W-306*S,Y+7*S,.72f*S,It->Anger>.5f ? Orange : Cream);
        Wrapped(State,W-306*S,Y+28*S,274*S,.64f*S,Muted);
        DrawRect(FLinearColor(.12f,.2f,.16f),W-306*S,Y+67*S,268*S,4*S);
        DrawRect(It->Patience<40.f ? Orange : Muted,W-306*S,Y+67*S,268*S*FMath::Clamp(It->Patience/140.f,0.f,1.f),4*S);
        ++Row;
    }
    DrawRect(Cream,W/2-2,H/2-2,4,4);
    if(Player)
    {
        FString Hint; AActor* Target=Player->FocusActor(330.f);
        auto* Station=Cast<ACEStation>(Target); if(!Station && Target) Station=Cast<ACEStation>(Target->GetAttachParentActor());
        if(Station) Hint=Station->Prompt(Player);
        else if(auto* Customer=Cast<ACECustomer>(Target)) Hint=Customer->bLeaving ? TEXT("El cliente ya ha decidido marcharse.") : Player->Held ? TEXT("[ F ] Entregar desayuno   [ Q ] Provocar") : TEXT("[ F ] Tomar / recordar pedido   [ Q ] Provocar");
        else if(auto* Prop=Player->FocusProp()) Hint=TEXT("[ E ] ")+Prop->DisplayName;
        if(!Hint.IsEmpty())
        {
            const float HW=FMath::Min(640*S,W-48*S); DrawRect(Ink,(W-HW)/2,H/2+30*S,HW,58*S);
            Wrapped(Hint,(W-HW)/2+14*S,H/2+38*S,HW-28*S,.78f*S,Cream);
        }
        if(Player->Held)
        {
            DrawRect(Ink,20*S,H-160*S,350*S,74*S);
            Label(Player->Held->DisplayName,36*S,H-151*S,.85f*S,Cream);
            if(Player->Held->IsContainer())
            {
                Label(FString::Printf(TEXT("%.0f / %.0f ml  ·  %.0f C"),Player->Held->VolumeMl(),Player->Held->CapacityMl(),Player->Held->Temperature),36*S,H-125*S,.72f*S,Orange);
                Label(FString::Printf(TEXT("Café %.0f  Leche %.0f  Agua %.0f"),Player->Held->EspressoMl,Player->Held->MilkMl,Player->Held->WaterMl),36*S,H-105*S,.61f*S,Muted);
            }
            else Label(TEXT("E soltar · clic lanzar · F usar / entregar"),36*S,H-119*S,.63f*S,Muted);
        }
    }
    const float Now=GetWorld()->GetTimeSeconds();
    if(Now<Cafe->BossSubtitleUntil)
    {
        const float PanelW=FMath::Min(820*S,W-48*S), X=(W-PanelW)/2,Y=H-285*S;
        DrawRect(Ink,X,Y,PanelW,120*S); DrawRect(Orange,X,Y,4*S,120*S);
        Label(TEXT("LA JEFA  /  CONVIERTE EL DESASTRE EN UN ASCENSO"),X+18*S,Y+12*S,.71f*S,Orange);
        Wrapped(Cafe->BossSubtitle,X+18*S,Y+40*S,PanelW-36*S,.83f*S,Cream);
    }
    if(Now<Cafe->FeedbackUntil) Wrapped(Cafe->Feedback,24*S,H-202*S,570*S,.7f*S,Orange);
    DrawRect(Ink,0,H-76*S,W,76*S);
    Label(TEXT("WASD / RATÓN mover y mirar   E agarrar / soltar   F usar / pedir / servir   Q provocar"),24*S,H-66*S,.67f*S,Cream);
    Label(TEXT("CLIC lanzar / empujar   DERECHO verter (apunta a la taza)   SHIFT correr   ESPACIO saltar   R reiniciar   ESC salir"),24*S,H-45*S,.60f*S,Muted);
    Label(TEXT("ESPRESSO: moler → prensar → grupo → extraer | LATTE: jarra → vapor → verter | AMERICANO: +100 agua"),24*S,H-24*S,.57f*S,Muted);
}
