#include "CEGameplay.h"
#include "CEDialogue.h"
#include "CEKitchen.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "EngineUtils.h"

void ACEHUD::Label(const FString& Text,float X,float Y,float Scale,FLinearColor Color)
{ DrawText(Text,Color,X,Y,GEngine->GetMediumFont(),Scale*1.6f,false); }
void ACEHUD::Wrapped(const FString& Text,float X,float Y,float Width,float Scale,FLinearColor Color)
{
    TArray<FString> Words; Text.ParseIntoArray(Words,TEXT(" "),true); FString Line;
    for(const FString& Word:Words) {
        const FString Next=Line.IsEmpty() ? Word : Line+TEXT(" ")+Word; float W=0,H=0;
        GetTextSize(Next,W,H,GEngine->GetMediumFont(),Scale*1.6f);
        if(W>Width && !Line.IsEmpty()) { Label(Line,X,Y,Scale,Color); Y+=23.f*Scale; Line=Word; } else Line=Next;
    }
    if(!Line.IsEmpty()) Label(Line,X,Y,Scale,Color);
}
void ACEHUD::DrawHUD()
{
    Super::DrawHUD(); if(!Canvas) return;
    auto* Player=Cast<ACEPlayer>(GetOwningPawn()); if(!Player || Player->Dialogue->IsEditing()) return;
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>(); const float Now=GetWorld()->GetTimeSeconds();
    const float W=Canvas->SizeX,H=Canvas->SizeY,S=FMath::Clamp(FMath::Min(W/1280.f,H/720.f),.65f,2.5f)*.86f;
    const FLinearColor Cream(.94f,.92f,.85f),Gold(.94f,.66f,.30f),Muted(.58f,.72f,.70f),Ink(.015f,.025f,.028f,.82f);
    const int32 Remaining=FMath::Max(0,int32(Cafe->ShiftDuration-Now));
    DrawRect(Ink,18*S,18*S,345*S,91*S); DrawRect(Gold,18*S,18*S,3*S,91*S);
    Label(FString::Printf(TEXT("CERO ESTRELLAS  %02d:%02d"),Remaining/60,Remaining%60),30*S,27*S,.9f*S,Cream);
    Label(FString::Printf(TEXT("Servidos %d · Huidas %d · Viral %d"),Cafe->OrdersServed,Cafe->DepartedCustomers,Cafe->ViralPoints),30*S,54*S,.64f*S,Muted);
    Wrapped(Cafe->bShiftFinished ? TEXT("DESPIDO DENEGADO / Ascenso concedido") : Cafe->Challenge,30*S,77*S,318*S,.59f*S,Gold);
    if(Cafe->Combo>1) { Label(FString::Printf(TEXT("DESMADRE x%d"),Cafe->Combo),W/2-74*S,30*S,1.f*S,Gold); DrawRect(Gold,W/2-74*S,57*S,150*S*FMath::Clamp((Cafe->ComboUntil-Now)/9.f,0.f,1.f),3*S); }
    AActor* Target=Player->FocusActor(330.f); auto* Focused=Cast<ACECustomer>(Target);
    int32 Row=0;
    for(TActorIterator<ACECustomer> It(GetWorld());It;++It) {
        if(It->bLeaving || It->State==ECECustomerState::Eating || (!Player->bShowOrderList && *It!=Focused)) continue;
        const float X=W-282*S,Y=(18+Row*96)*S;
        DrawRect(FLinearColor(.86f,.83f,.74f,.96f),X,Y,264*S,89*S);
        Label(FString::Printf(TEXT("MESA %d / %s"),It->SeatSlot+1,It->Anger>.5f ? TEXT("RECLAMACIÓN") : TEXT("COMANDA")),X+12*S,Y+8*S,.69f*S,FLinearColor(.12f,.17f,.15f));
        Wrapped(It->bOrderKnown ? It->OrderText() : TEXT("F: tomar nota"),X+12*S,Y+29*S,240*S,.62f*S,FLinearColor(.22f,.26f,.22f));
        if(It->bOrderKnown && !It->SpecialRequest.IsEmpty()) Wrapped(It->SpecialRequest,X+12*S,Y+58*S,240*S,.52f*S,FLinearColor(.44f,.19f,.08f));
        DrawRect(Muted,X+12*S,Y+82*S,238*S*FMath::Clamp(It->Patience/140.f,0.f,1.f),2*S);
        if(++Row>=3) break;
    }
    if(Focused && Focused->StoryId>=0 && !Player->bShowOrderList) {
        const float X=W-282*S,Y=115*S;
        Wrapped(Focused->StoryTitle+FString::Printf(TEXT(" · %d/3"),FMath::Min(3,Focused->StoryStage+1)),X,Y,260*S,.64f*S,Gold);
        Wrapped(Focused->bStoryResolved ? Focused->StoryOutcome : Focused->StoryBeat,X,Y+23*S,260*S,.54f*S,Cream);
    }
    DrawRect(Target ? Gold : Cream,W/2-2,H/2-2,4,4);
    FString Hint; auto* Station=Cast<ACEStation>(Target); if(!Station && Target) Station=Cast<ACEStation>(Target->GetAttachParentActor());
    if(Station) Hint=Station->Prompt(Player);
    else if(Focused) Hint=Player->Held ? !Player->Held->OwnerCustomerId.IsEmpty() && Player->Held->Kind!=ECEPropKind::Laptop ? TEXT("F devolver pertenencia · T conversar · V hablar") : TEXT("F entregar · Q provocar · T conversar · V hablar") : TEXT("F pedido · Q provocar · T conversar · V hablar");
    else if(auto* Prop=Player->FocusProp()) Hint=TEXT("E agarrar / ")+Prop->DisplayName;
    const float Bottom=Player->bShowHelp || Now<12.f ? 90.f : 37.f;
    if(!Hint.IsEmpty()) { const float Width=FMath::Min(650*S,W-40*S),Y=H-(Bottom+100)*S; DrawRect(Ink,(W-Width)/2,Y,Width,42*S); Wrapped(Hint,(W-Width)/2+12*S,Y+9*S,Width-24*S,.69f*S,Cream); }
    if(Player->Held && Player->Held->IsContainer()) {
        auto* Cup=Player->Held.Get(); DrawRect(Ink,18*S,H-(Bottom+84)*S,290*S,69*S);
        Label(FString::Printf(TEXT("%s · %.0f ml · %.0f C"),*Cup->DisplayName,Cup->VolumeMl(),Cup->Temperature),28*S,H-(Bottom+76)*S,.66f*S,Cream);
        Label(FString::Printf(TEXT("Espuma %.0f · Azúcar %.0f g · Sirope %.0f"),Cup->FoamMl,Cup->SugarGrams,Cup->SyrupMl),28*S,H-(Bottom+49)*S,.53f*S,Gold);
    }
    FString Caption=Now<Cafe->CustomerSubtitleUntil ? Cafe->CustomerSubtitle : FString();
    auto* Dialogue=Player->Dialogue.Get();
    if(Dialogue->IsOpen()) {
        if(Dialogue->bRecording) Caption=TEXT("Grabando · V para detener y revisar");
        else if(Dialogue->IsBusy()) Caption=Dialogue->ConversationLabel()+TEXT(": ")+(Dialogue->PreviewReply.IsEmpty() ? TEXT("Te escucha. Puedes seguir trabajando.") : Dialogue->PreviewReply);
    }
    if(Now<Cafe->BossSubtitleUntil) Caption=TEXT("LA JEFA: ")+Cafe->BossSubtitle;
    if(!Caption.IsEmpty()) { const float Width=FMath::Min(720*S,W-42*S),Y=H-(Bottom+165)*S; DrawRect(Ink,(W-Width)/2,Y,Width,58*S); Wrapped(Caption,(W-Width)/2+12*S,Y+9*S,Width-24*S,.73f*S,Cream); }
    if(Now<Cafe->FeedbackUntil) Wrapped(Cafe->Feedback,22*S,122*S,400*S,.62f*S,Gold);
    if(Now<Cafe->BusinessUntil && !Cafe->BusinessTitle.IsEmpty()) Label(Cafe->BusinessTitle,22*S,155*S,.60f*S,Gold);
    if(Now<Cafe->CampaignUntil) Label(Cafe->BossCampaign,22*S,178*S,.55f*S,Gold);
    DrawRect(Ink,0,H-Bottom*S,W,Bottom*S);
    Label(TEXT("TAB comandas · F1 ayuda · E agarrar · F usar · T conversar · V voz"),20*S,H-(Bottom-8)*S,.62f*S,Muted);
    if(Bottom>40.f) { Label(TEXT("WASD mover · CLIC lanzar · DERECHO verter · SHIFT correr · ESPACIO saltar · R reiniciar · ESC salir"),20*S,H-52*S,.58f*S,Cream); Label(TEXT("Moler → mantener F para prensar → extraer (F detiene) | RUEDA: cantidad / temperatura | Leche: vapor → espuma → verter"),20*S,H-29*S,.57f*S,Muted); }
}
