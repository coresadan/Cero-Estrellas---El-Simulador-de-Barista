#include "CEGameplay.h"
#include "CEDialogue.h"
#include "CEKitchen.h"
#include "CECafeLayout.h"
#include "Engine/Canvas.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "EngineUtils.h"
#include "Brushes/SlateRoundedBoxBrush.h"
#include "Brushes/SlateColorBrush.h"
#include "Styling/CoreStyle.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/SOverlay.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/Layout/SScaleBox.h"
#include "Widgets/Layout/SUniformGridPanel.h"
#include "Widgets/Text/STextBlock.h"
#include "Widgets/Notifications/SProgressBar.h"

struct FCEHUDOrder
{
    FString Title,Drink,Extras,Status;
    float Patience=0;
};
struct FCEHUDView
{
    FString Clock,Stats,Score,Mission,Hint,Caption,Speaker,Feedback,Business,Held,Contents,FocusedTitle,FocusedOrder,FocusedStory;
    FCEHUDOrder Orders[CECafeLayout::TableCount];
    float Combo=0,Volume=0;
    bool ShowOrders=false,ShowHelp=false,HasContainer=false;
};
namespace CEHudStyle
{
    const FLinearColor Paper(.94f,.91f,.83f),Muted(.58f,.70f,.67f),Gold(.91f,.64f,.29f),Mint(.40f,.75f,.63f);
    const FSlateRoundedBoxBrush Card(FLinearColor(.018f,.038f,.037f,.94f),12.f);
    const FSlateRoundedBoxBrush Inner(FLinearColor(.043f,.073f,.068f,.98f),8.f);
    const FProgressBarStyle Progress=FProgressBarStyle().SetBackgroundImage(FSlateColorBrush(FLinearColor(.08f,.13f,.12f))).SetFillImage(FSlateColorBrush(FLinearColor::White)).SetMarqueeImage(FSlateColorBrush(FLinearColor::White));
    template<class F> TAttribute<FText> Value(F Get) { return TAttribute<FText>::CreateLambda([Get] { return FText::FromString(Get()); }); }
    TSharedRef<SWidget> Text(TAttribute<FText> Content,int32 Size=14,FLinearColor Color=Paper,float Width=0,bool Bold=false)
    { return SNew(STextBlock).Text(Content).Font(FCoreStyle::GetDefaultFontStyle(Bold ? "Bold" : "Regular",Size)).ColorAndOpacity(Color).WrapTextAt(Width); }
    TSharedRef<SWidget> Text(const TCHAR* Content,int32 Size=14,FLinearColor Color=Paper,float Width=0,bool Bold=false)
    { return Text(FText::FromString(Content),Size,Color,Width,Bold); }
    TSharedRef<SWidget> CardBox(TSharedRef<SWidget> Content,float Width,const FSlateBrush* Brush=&Card)
    { return SNew(SBox).WidthOverride(Width)[SNew(SBorder).BorderImage(Brush).Padding(16)[Content]]; }
}

class SCECafeHUD : public SCompoundWidget
{
public:
    SLATE_BEGIN_ARGS(SCECafeHUD) {} SLATE_ARGUMENT(TSharedPtr<FCEHUDView>,View) SLATE_END_ARGS()
    void Construct(const FArguments& Args)
    {
        using namespace CEHudStyle;
        const auto V=Args._View;
        auto Grid=SNew(SUniformGridPanel).SlotPadding(FMargin(5));
        for(int32 i=0;i<CECafeLayout::TableCount;++i) {
            Grid->AddSlot(i%2,i/2)[CardBox(
                SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight()[Text(Value([V,i] { return V->Orders[i].Title; }),12,Gold,0,true)]
                +SVerticalBox::Slot().AutoHeight().Padding(0,9,0,5)[Text(Value([V,i] { return V->Orders[i].Drink; }),16,Paper,360,true)]
                +SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([V,i] { return V->Orders[i].Extras.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })[Text(Value([V,i] { return V->Orders[i].Extras; }),12,Muted,360)]]
                +SVerticalBox::Slot().AutoHeight().Padding(0,9,0,7)[Text(Value([V,i] { return V->Orders[i].Status; }),12,Mint)]
                +SVerticalBox::Slot().AutoHeight()[SNew(SBox).HeightOverride(3)[SNew(SProgressBar).Style(&Progress).Percent_Lambda([V,i] { return V->Orders[i].Patience; }).FillColorAndOpacity(Mint)]]
                ,390,&Inner)];
        }
        ChildSlot[SNew(SScaleBox).Stretch(EStretch::ScaleToFit)[SNew(SBox).WidthOverride(1280).HeightOverride(720)[
            SNew(SOverlay)
            +SOverlay::Slot()[SNew(SOverlay).Visibility_Lambda([V] { return V->ShowOrders || V->ShowHelp ? EVisibility::Collapsed : EVisibility::HitTestInvisible; })
            +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Top).Padding(24)[
                SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight()[CardBox(SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight()[Text(TEXT("CERO ESTRELLAS"),18,Paper,0,true)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,4,0,0)[Text(TEXT("Servicio pésimo. Carrera brillante."),11,Muted)]
                    ,280)]
                +SVerticalBox::Slot().AutoHeight().Padding(0,10,0,0)[CardBox(SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight()[Text(TEXT("TU PLAN PARA HOY"),10,Gold,0,true)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,7,0,0)[Text(Value([V] { return V->Mission; }),14,Paper,248)]
                    ,280)]
            ]
            +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Top).Padding(24)[
                SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight()[CardBox(SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight()[Text(TEXT("CIERRE DE CAJA"),10,Muted,0,true)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,2,0,4)[Text(Value([V] { return V->Clock; }),28,Paper,0,true)]
                    +SVerticalBox::Slot().AutoHeight()[Text(Value([V] { return V->Stats; }),12,Muted)]
                    ,244)]
                +SVerticalBox::Slot().AutoHeight().Padding(0,10,0,0)[SNew(SBox).Visibility_Lambda([V] { return V->FocusedTitle.IsEmpty() || V->ShowOrders ? EVisibility::Collapsed : EVisibility::Visible; })[
                    CardBox(SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight()[Text(Value([V] { return V->FocusedTitle; }),11,Gold,212,true)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)[Text(Value([V] { return V->FocusedOrder; }),14,Paper,212)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)[Text(Value([V] { return V->FocusedStory; }),11,Muted,212)]
                    ,244)]]
            ]
            +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Top).Padding(0,24)[
                CardBox(SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[Text(Value([V] { return V->Score; }),18,Gold,0,true)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,0)[SNew(SBox).HeightOverride(3)[SNew(SProgressBar).Style(&Progress).Percent_Lambda([V] { return V->Combo; }).FillColorAndOpacity(Gold)]]
                    ,220)]
            +SOverlay::Slot().HAlign(HAlign_Left).VAlign(VAlign_Bottom).Padding(24,0,0,30)[
                SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[SNew(SBox).Visibility_Lambda([V] { return V->Feedback.IsEmpty() && V->Business.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })[
                    CardBox(SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight()[Text(Value([V] { return V->Feedback; }),12,Mint,248)]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,5,0,0)[Text(Value([V] { return V->Business; }),11,Muted,248)]
                        ,280)]]
                +SVerticalBox::Slot().AutoHeight()[SNew(SBox).Visibility_Lambda([V] { return V->Held.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })[
                    CardBox(SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight()[Text(TEXT("EN TUS MANOS"),10,Muted,0,true)]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,3)[Text(Value([V] { return V->Held; }),15,Paper,248,true)]
                        +SVerticalBox::Slot().AutoHeight()[Text(Value([V] { return V->Contents; }),11,Muted,248)]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)[SNew(SBox).HeightOverride(3).Visibility_Lambda([V] { return V->HasContainer ? EVisibility::Visible : EVisibility::Collapsed; })[
                            SNew(SProgressBar).Style(&Progress).Percent_Lambda([V] { return V->Volume; }).FillColorAndOpacity(Gold)]]
                        ,280)]]
            ]
            +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Bottom).Padding(310,0,270,28)[
                SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[SNew(SBox).Visibility_Lambda([V] { return V->Caption.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })[
                    CardBox(SNew(SVerticalBox)
                        +SVerticalBox::Slot().AutoHeight()[Text(Value([V] { return V->Speaker; }),10,Gold,0,true)]
                        +SVerticalBox::Slot().AutoHeight().Padding(0,6,0,0)[Text(Value([V] { return V->Caption; }),15,Paper,600)]
                        ,632)]]
                +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center)[SNew(SBox).Visibility_Lambda([V] { return V->Hint.IsEmpty() ? EVisibility::Collapsed : EVisibility::Visible; })[
                    CardBox(Text(Value([V] { return V->Hint; }),12,Paper,440),472)]]
            ]
            +SOverlay::Slot().HAlign(HAlign_Right).VAlign(VAlign_Bottom).Padding(0,0,24,28)[
                CardBox(Text(TEXT("TAB  Comandas     F1  Controles"),11,Muted),244)]
            ]
            +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SBox).Visibility_Lambda([V] { return V->ShowOrders ? EVisibility::Visible : EVisibility::Collapsed; })[
                CardBox(SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight().Padding(8,0,0,5)[Text(TEXT("COMANDAS EN MARCHA"),21,Paper,0,true)]
                    +SVerticalBox::Slot().AutoHeight().Padding(8,0,0,14)[Text(TEXT("Seis mesas. Una sola persona haciendo todo."),12,Muted)]
                    +SVerticalBox::Slot().AutoHeight()[Grid]
                    +SVerticalBox::Slot().AutoHeight().Padding(8,10,0,0)[Text(TEXT("Suelta TAB para volver al servicio"),11,Muted)]
                    ,842)]]
            +SOverlay::Slot().HAlign(HAlign_Center).VAlign(VAlign_Center)[SNew(SBox).Visibility_Lambda([V] { return V->ShowHelp ? EVisibility::Visible : EVisibility::Collapsed; })[
                CardBox(SNew(SVerticalBox)
                    +SVerticalBox::Slot().AutoHeight()[Text(TEXT("MANUAL DEL EMPLEADO PRESCINDIBLE"),20,Paper,0,true)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,16,0,0)[Text(TEXT("WASD  Moverte     RATÓN  Mirar     SHIFT  Correr     ESPACIO  Saltar\n\nE  Agarrar / soltar     CLIC  Lanzar o empujar     DERECHO  Verter\n\nF  Usar / tomar pedido / entregar     Q  Provocar\n\nT  Conversar     V  Grabar voz     TAB  Todas las comandas"),15,Paper,740)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,20,0,0)[Text(TEXT("DEL GRANO AL DESASTRE"),11,Gold,0,true)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,8,0,0)[Text(TEXT("Moler → mantener F para prensar → extraer café → servir.\nF detiene la extracción. La rueda ajusta cantidad o temperatura.\nPara un latte: calienta la leche, añade espuma y vierte sobre el espresso."),14,Muted,740)]
                    +SVerticalBox::Slot().AutoHeight().Padding(0,20,0,0)[Text(TEXT("F1  Cerrar ayuda     R  Reiniciar turno     ESC  Salir"),12,Mint)]
                    ,810)]]
        ]]];
        SetVisibility(EVisibility::HitTestInvisible);
    }
};

void ACEHUD::DrawHUD()
{
    Super::DrawHUD(); if(!Canvas || !GEngine || !GEngine->GameViewport) return;
    auto* Player=Cast<ACEPlayer>(GetOwningPawn()); if(!Player || !Player->Dialogue) return;
    if(!ViewState) {
        ViewState=MakeShared<FCEHUDView>(); Overlay=SNew(SCECafeHUD).View(ViewState);
        GEngine->GameViewport->AddViewportWidgetContent(Overlay.ToSharedRef(),5);
    }
    Overlay->SetVisibility(Player->Dialogue->IsEditing() ? EVisibility::Collapsed : EVisibility::HitTestInvisible);
    if(Player->Dialogue->IsEditing()) return;
    auto& V=*ViewState;
    const auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>(); const float Now=GetWorld()->GetTimeSeconds();
    const int32 Remaining=FMath::Max(0,int32(Cafe->ShiftDuration-Now));
    V.Clock=FString::Printf(TEXT("%02d:%02d"),Remaining/60,Remaining%60);
    V.Stats=FString::Printf(TEXT("%d servidos  /  %d huidas"),Cafe->OrdersServed,Cafe->DepartedCustomers);
    V.Mission=Cafe->bShiftFinished ? TEXT("Despido denegado. Has ascendido.") : Cafe->Challenge;
    V.Score=Cafe->Combo>1 ? FString::Printf(TEXT("DESMADRE ×%d"),Cafe->Combo) : FString::Printf(TEXT("VIRAL · %d"),Cafe->ViralPoints);
    V.Combo=FMath::Clamp((Cafe->ComboUntil-Now)/9.f,0.f,1.f);
    V.ShowOrders=Player->bShowOrderList; V.ShowHelp=Player->bShowHelp;
    V.Feedback=Now<Cafe->FeedbackUntil ? Cafe->Feedback : FString();
    V.Business=Now<Cafe->BusinessUntil ? Cafe->BusinessTitle : Now<Cafe->CampaignUntil ? Cafe->BossCampaign : FString();
    AActor* Target=Player->FocusActor(330.f); auto* Guest=Cast<ACECustomer>(Target);
    V.FocusedTitle=Guest ? FString::Printf(TEXT("MESA %d · %s"),Guest->SeatSlot+1,Guest->Anger>.5f ? TEXT("RECLAMACIÓN") : TEXT("COMANDA")) : FString();
    V.FocusedOrder=Guest ? Guest->bOrderKnown ? Guest->OrderText()+TEXT("\n")+Guest->SpecialRequest : TEXT("F  Tomar nota") : FString();
    V.FocusedStory=Guest && Guest->StoryId>=0 ? Guest->StoryTitle+TEXT("\n")+(Guest->bStoryResolved ? Guest->StoryOutcome : Guest->StoryBeat) : FString();
    for(int32 i=0;i<CECafeLayout::TableCount;++i) {
        V.Orders[i]=FCEHUDOrder(); V.Orders[i].Title=FString::Printf(TEXT("MESA %02d"),i+1); V.Orders[i].Drink=TEXT("Libre"); V.Orders[i].Status=TEXT("Aún hay esperanza.");
    }
    for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(!It->bLeaving && It->SeatSlot>=0 && It->SeatSlot<CECafeLayout::TableCount) {
        auto& O=V.Orders[It->SeatSlot];
        O.Title=FString::Printf(TEXT("MESA %02d · %s"),It->SeatSlot+1,It->State==ECECustomerState::Eating ? TEXT("SERVIDA") : TEXT("PENDIENTE"));
        O.Drink=It->bOrderKnown ? It->OrderText() : TEXT("Falta tomar nota"); O.Extras=It->SpecialRequest;
        O.Status=It->State==ECECustomerState::Eating ? TEXT("Desayunando") : It->State==ECECustomerState::Arriving ? TEXT("Llegando a la barra") : It->State==ECECustomerState::WalkingToTable ? TEXT("Camino de su mesa") : FString::Printf(TEXT("Paciencia · %.0f s"),It->Patience);
        O.Patience=FMath::Clamp(It->Patience/140.f,0.f,1.f);
    }
    auto* Station=Cast<ACEStation>(Target); if(!Station && Target) Station=Cast<ACEStation>(Target->GetAttachParentActor());
    V.Hint=Station ? Station->Prompt(Player) : Guest ? Player->Held ? TEXT("F  Entregar     Q  Provocar     T  Conversar") : TEXT("F  Pedido     Q  Provocar     T  Conversar") : FString();
    if(Guest && Player->Held && !Player->Held->OwnerCustomerId.IsEmpty()) V.Hint=TEXT("F  Devolver pertenencia     T  Conversar");
    if(!Station && !Guest) if(auto* Prop=Player->FocusProp()) V.Hint=TEXT("E  Agarrar · ")+Prop->DisplayName;
    V.Held=Player->Held ? Player->Held->DisplayName : FString(); V.Contents.Empty(); V.HasContainer=Player->Held && Player->Held->IsContainer();
    if(V.HasContainer) {
        const auto* C=Player->Held.Get(); V.Volume=C->VolumeMl()/C->CapacityMl();
        V.Contents=FString::Printf(TEXT("%.0f / %.0f ml   ·   %.0f °C\nEspuma %.0f ml · Azúcar %.0f g · Sirope %.0f ml"),C->VolumeMl(),C->CapacityMl(),C->Temperature,C->FoamMl,C->SugarGrams,C->SyrupMl);
    } else if(Player->Held) V.Contents=TEXT("E  Soltar     CLIC  Lanzar");
    V.Caption=Now<Cafe->CustomerSubtitleUntil ? Cafe->CustomerSubtitle : FString(); V.Speaker=TEXT("EN LA CAFETERÍA");
    const auto* D=Player->Dialogue.Get();
    if(D->IsOpen()) {
        if(D->bRecording) { V.Speaker=TEXT("TU VOZ"); V.Caption=TEXT("Grabando · V para detener y revisar"); }
        else if(D->IsBusy()) { V.Speaker=D->ConversationLabel(); V.Caption=D->PreviewReply.IsEmpty() ? TEXT("Te escucha. Puedes seguir trabajando.") : D->PreviewReply; }
    }
    if(Now<Cafe->BossSubtitleUntil) { V.Speaker=TEXT("LA JEFA · RADIO CORPORATIVA"); V.Caption=Cafe->BossSubtitle; }
    if(!Player->bShowHelp && !Player->bShowOrderList) {
        const FLinearColor Color=Target ? CEHudStyle::Gold : CEHudStyle::Paper;
        const float X=Canvas->SizeX*.5f,Y=Canvas->SizeY*.5f;
        DrawRect(Color,X-1,Y-1,2,2);
        if(Target) { DrawLine(X-8,Y,X-4,Y,Color,1); DrawLine(X+4,Y,X+8,Y,Color,1); }
    }
}
void ACEHUD::EndPlay(const EEndPlayReason::Type Reason)
{
    if(Overlay && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Overlay.ToSharedRef());
    Overlay.Reset(); ViewState.Reset(); Super::EndPlay(Reason);
}
