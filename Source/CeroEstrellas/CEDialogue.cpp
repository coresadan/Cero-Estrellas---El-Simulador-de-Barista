#include "CEDialogue.h"
#include "CEGameplay.h"
#include "AudioCaptureCore.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundWaveProcedural.h"
#include "HttpModule.h"
#include "Interfaces/IHttpResponse.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Misc/ConfigCacheIni.h"
#include "TimerManager.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Engine/GameViewportClient.h"
#include "Engine/Engine.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "Widgets/SCompoundWidget.h"
#include "Widgets/Layout/SBorder.h"
#include "Widgets/Layout/SBox.h"
#include "Widgets/SBoxPanel.h"
#include "Widgets/Input/SEditableTextBox.h"
#include "Widgets/Input/SButton.h"
#include "Widgets/Input/SCheckBox.h"
#include "Widgets/Text/STextBlock.h"
#include "Framework/Application/IInputProcessor.h"
#include "Framework/Application/SlateApplication.h"

namespace {
FString Encode(const TSharedPtr<FJsonObject>& Object) { FString S; FJsonSerializer::Serialize(Object.ToSharedRef(),TJsonWriterFactory<TCHAR,TCondensedJsonPrintPolicy<TCHAR>>::Create(&S)); return S; }
TSharedPtr<FJsonObject> Decode(const FString& S) { TSharedPtr<FJsonObject> O; FJsonSerializer::Deserialize(TJsonReaderFactory<>::Create(S),O); return O; }
FString NormalizeProviderReply(const FString& Wire) {
    const auto Root=Decode(Wire); const TSharedPtr<FJsonObject>* Dialogue=nullptr; const TSharedPtr<FJsonObject>* Reaction=nullptr;
    if(!Root || Root->Values.Num()!=2 || !Root->TryGetObjectField(TEXT("dialogo"),Dialogue) || !Root->TryGetObjectField(TEXT("reaccion"),Reaction) || (*Dialogue)->Values.Num()!=1 || (*Reaction)->Values.Num()!=3) return {};
    const auto* Text=(*Dialogue)->Values.Find(TEXT("texto")); if(!Text) return {};
    auto Flat=MakeShared<FJsonObject>(); Flat->Values=(*Reaction)->Values; Flat->Values.Add(TEXT("texto"),*Text);
    return Encode(Flat);
}
TSharedRef<FJsonValue> Message(const FString& Role,const FString& Text) { auto O=MakeShared<FJsonObject>(); O->SetStringField(TEXT("role"),Role); O->SetStringField(TEXT("content"),Text); return MakeShared<FJsonValueObject>(O); }
void TestTrace(const FString& Text,const TCHAR* Kind,int32 Turn) {
    if(!FParse::Param(FCommandLine::Get(),TEXT("CeroDialogueTest"))) return;
    FString Directory; if(FParse::Value(FCommandLine::Get(),TEXT("CeroEvidenceDir="),Directory)) FFileHelper::SaveStringToFile(Text,*(Directory/FString::Printf(TEXT("%s_%d.json"),Kind,Turn)));
}
const TCHAR* Emotions[]={TEXT("neutral"),TEXT("enfadado"),TEXT("humillado"),TEXT("triste"),TEXT("divertido")};
FString PreviewText(const FString& Json) {
    // Decode only a quoted texto field; incomplete escapes wait for the next chunk.
    int32 I=Json.Find(TEXT("\"texto\"")); if(I==INDEX_NONE) return {};
    I+=7; while(I<Json.Len() && (FChar::IsWhitespace(Json[I]) || Json[I]==':')) ++I;
    if(I>=Json.Len() || Json[I++]!='"') return {};
    FString Text;
    while(I<Json.Len() && Text.Len()<350) {
        TCHAR C=Json[I++]; if(C=='"') break;
        if(C=='\\') {
            if(I>=Json.Len()) break; C=Json[I++];
            if(C=='u') {
                if(I+4>Json.Len()) break;
                uint32 Code=0; for(int32 N=0;N<4;++N) { TCHAR H=Json[I++]; if(!FChar::IsHexDigit(H)) return {}; Code=Code*16+(H<='9' ? H-'0' : FChar::ToLower(H)-'a'+10); }
                C=TCHAR(Code);
            } else if(C=='n') C='\n'; else if(C=='t') C='\t'; else if(C=='r') C='\r';
            else if(C!='"' && C!='\\' && C!='/') return {};
        }
        Text.AppendChar(C);
    }
    return Text;
}
// Registered only while the panel is open. Escape also works after an input widget loses focus.
class FCEConversationEscape : public IInputProcessor {
public:
    TWeakObjectPtr<UCEDialogueComponent> Owner;
    const bool bAutomation=FParse::Param(FCommandLine::Get(),TEXT("CeroDialogueTest"));
    explicit FCEConversationEscape(UCEDialogueComponent* InOwner) : Owner(InOwner) {}
    virtual void Tick(float,FSlateApplication&,TSharedRef<ICursor>) override {}
    virtual bool HandleKeyDownEvent(FSlateApplication&,const FKeyEvent& Event) override {
        if(bAutomation && (!Owner.IsValid() || !Owner->bSmokeInjectingEscape)) return true;
        if(Event.GetKey()==EKeys::Escape && Owner.IsValid() && Owner->IsOpen()) { Owner->Close(); return true; }
        return false;
    }
    virtual bool HandleKeyUpEvent(FSlateApplication&,const FKeyEvent&) override { return bAutomation; }
    virtual bool HandleMouseButtonDownEvent(FSlateApplication&,const FPointerEvent&) override { return bAutomation; }
    virtual bool HandleMouseButtonUpEvent(FSlateApplication&,const FPointerEvent&) override { return bAutomation; }
    virtual bool HandleMouseButtonDoubleClickEvent(FSlateApplication&,const FPointerEvent&) override { return bAutomation; }
};
class SCEChatPanel : public SCompoundWidget {
public:
    SLATE_BEGIN_ARGS(SCEChatPanel) {} SLATE_DEFAULT_SLOT(FArguments,Content) SLATE_END_ARGS()
    TWeakObjectPtr<UCEDialogueComponent> Owner;
    void Construct(const FArguments& Args) { ChildSlot[Args._Content.Widget]; }
    virtual FReply OnPreviewKeyDown(const FGeometry&,const FKeyEvent& Event) override {
        if(Event.GetKey()==EKeys::Escape && Owner.IsValid()) { Owner->Close(); return FReply::Handled(); }
        return FReply::Unhandled();
    }
};
void Put16(TArray<uint8>& A,uint16 V) { A.Add(uint8(V)); A.Add(uint8(V>>8)); }
void Put32(TArray<uint8>& A,uint32 V) { Put16(A,uint16(V)); Put16(A,uint16(V>>16)); }
}
struct FCECaptureBuffer {
    FCriticalSection Lock;
    TArray<int16> Samples;
    int32 Rate = 48000;
};
struct FCEReplyStream {
    FCriticalSection Lock;
    TArray<uint8> Incoming;
    int64 Received=0;
};

UCEDialogueComponent::UCEDialogueComponent() { PrimaryComponentTick.bCanEverTick=true; }
UCEDialogueComponent::~UCEDialogueComponent() = default;
void UCEDialogueComponent::BeginPlay() {
    Super::BeginPlay(); GConfig->GetString(TEXT("CeroEstrellas.AI"),TEXT("Model"),Model,GGameIni);
    GetWorld()->GetSubsystem<UCECafeSubsystem>()->OnChaosObserved.AddDynamic(this,&UCEDialogueComponent::OnChaos);
    Speaker=NewObject<UAudioComponent>(GetOwner()); GetOwner()->AddInstanceComponent(Speaker);
    Speaker->SetupAttachment(GetOwner()->GetRootComponent()); Speaker->bAutoActivate=false;
    Speaker->bOverrideAttenuation=true; Speaker->AttenuationOverrides.bAttenuate=true;
    Speaker->AttenuationOverrides.bSpatialize=true; Speaker->AttenuationOverrides.AttenuationShapeExtents=FVector(140);
    Speaker->AttenuationOverrides.FalloffDistance=600; Speaker->RegisterComponent();
}
void UCEDialogueComponent::EndPlay(const EEndPlayReason::Type Reason) {
    Close(); GetWorld()->GetSubsystem<UCECafeSubsystem>()->OnChaosObserved.RemoveDynamic(this,&UCEDialogueComponent::OnChaos);
    Super::EndPlay(Reason);
}
void UCEDialogueComponent::OnChaos(const FCEChaosEvent& Event) {
    if(Event.Severity<=0) return; Events.Add(Event); if(Events.Num()>64) Events.RemoveAt(0);
    if(Target.IsValid() && Target->CustomerId==Event.CustomerId && Event.Kind==TEXT("customer_hit")) {
        if(Speaker) Speaker->Stop(); Target->VoiceUntil=0;
        // A new physical interruption invalidates an answer to old facts.
        if(bBusy && ReplyStream) { CancelRequests(); Target->ThinkingUntil=0; Status=TEXT("El cliente reacciona al golpe. Puedes volver a hablarle."); }
    }
}
bool UCEDialogueComponent::Open(ACECustomer* Customer) {
    if(!IsValid(Customer) || Customer->bLeaving) return false;
    if(Target.Get()==Customer) { EditConversation(); return true; }
    Close(); Target=Customer; Customer->DialogueHoldUntil=GetWorld()->GetTimeSeconds()+45.f;
    Transcript.Empty(); Draft.Empty(); LastReply.Empty(); LastEmotion.Empty(); LastPlayerText.Empty(); bLastReplyFromOllama=false;
    Status=TEXT("Escribe o graba tu voz. Enter para enviar · Esc para volver al turno.");
    if(History.Contains(Customer->CustomerId)) {
        const auto& Turns=History[Customer->CustomerId]; if(!Turns.IsEmpty()) { float A; bool L; ValidateReply(NormalizeProviderReply(Turns.Last().ProviderReply),LastReply,LastEmotion,A,L); }
    }
    if(auto* Player=Cast<ACEPlayer>(GetOwner())) {
        Player->bPouring=false; if(Player->Held) Player->Held->bPourRequested=false;
        Player->GetCharacterMovement()->StopMovementImmediately(); Player->GetCharacterMovement()->MaxWalkSpeed=230.f;
    }
    BuildPanel();
    EscapeProcessor=MakeShared<FCEConversationEscape>(this);
    FSlateApplication::Get().RegisterInputPreProcessor(EscapeProcessor,0);
    if(auto* PC=Cast<APlayerController>(Cast<APawn>(GetOwner())->GetController())) {
        bEditing=true; FInputModeUIOnly Mode; Mode.SetWidgetToFocus(Input); Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Mode); PC->bShowMouseCursor=true;
    }
    return true;
}
void UCEDialogueComponent::ResumePlay() {
    bEditing=false;
    if(auto* Pawn=Cast<APawn>(GetOwner())) if(auto* PC=Cast<APlayerController>(Pawn->GetController())) {
        PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor=false; PC->FlushPressedKeys();
    }
    if(IsOpen()) Status=bRecording ? TEXT("Grabando. V detiene y transcribe.") : bBusy ? TEXT("Puedes seguir trabajando mientras responde. T vuelve a Dialogar.") : TEXT("T vuelve a Dialogar · V para hablar.");
}
FString UCEDialogueComponent::ConversationLabel() const { return Target.IsValid() ? Target->DisplayName : FString(); }
void UCEDialogueComponent::EditConversation() {
    if(!Target.IsValid() || Target->bLeaving || !Input) return;
    bEditing=true;
    if(auto* Pawn=Cast<APawn>(GetOwner())) if(auto* PC=Cast<APlayerController>(Pawn->GetController())) {
        PC->FlushPressedKeys(); FInputModeUIOnly Mode; Mode.SetWidgetToFocus(Input); Mode.SetLockMouseToViewportBehavior(EMouseLockMode::DoNotLock);
        PC->SetInputMode(Mode); PC->bShowMouseCursor=true;
    }
}
void UCEDialogueComponent::CancelRequests() {
    ++Generation;
    for(auto& R:Requests) { R->OnProcessRequestComplete().Unbind(); R->CancelRequest(); }
    Requests.Empty(); bBusy=false; ReplyStream.Reset(); StreamLine.Empty(); StreamJson.Empty(); StreamEnvelope.Empty(); PreviewReply.Empty();
}
void UCEDialogueComponent::Close() {
    if(EscapeProcessor && FSlateApplication::IsInitialized()) FSlateApplication::Get().UnregisterInputPreProcessor(EscapeProcessor);
    EscapeProcessor.Reset();
    if(Panel) UE_LOG(LogTemp,Display,TEXT("CE_DIALOGUE_CLOSE target=%s leaving=%d"),Target.IsValid() ? *Target->CustomerId : TEXT("destroyed"),Target.IsValid() && Target->bLeaving);
    StopRecording(false); CancelRequests(); if(Speaker) Speaker->Stop(); bEditing=false; bRecordingOnTheMove=false;
    if(Target.IsValid()) { Target->DialogueHoldUntil=0.f; Target->ThinkingUntil=0.f; Target->VoiceUntil=0.f; }
    Target.Reset(); if(Panel && GEngine && GEngine->GameViewport) GEngine->GameViewport->RemoveViewportWidgetContent(Panel.ToSharedRef());
    Panel.Reset(); Input.Reset();
    if(auto* Pawn=Cast<APawn>(GetOwner())) if(auto* PC=Cast<APlayerController>(Pawn->GetController())) {
        PC->SetInputMode(FInputModeGameOnly()); PC->bShowMouseCursor=false; PC->FlushPressedKeys();
    }
}
void UCEDialogueComponent::TickComponent(float Delta,ELevelTick Tick,FActorComponentTickFunction* Function) {
    Super::TickComponent(Delta,Tick,Function);
    ConsumeReplyStream();
    if(Panel && (!Target.IsValid() || Target->bLeaving)) Close();
    if(Target.IsValid() && Speaker && Speaker->IsPlaying()) Speaker->SetWorldLocation(Target->HeadLocation());
    if(Target.IsValid() && FVector::Dist2D(GetOwner()->GetActorLocation(),Target->GetActorLocation())>850.f) Close();
    if(bRecording && GetWorld()->GetTimeSeconds()-RecordStartedAt>=15.f) StopRecording(true);
    Requests.RemoveAll([](const auto& R){ return R->GetStatus()==EHttpRequestStatus::Succeeded || R->GetStatus()==EHttpRequestStatus::Failed; });
}
void UCEDialogueComponent::ConsumeReplyStream() {
    if(!ReplyStream || !Target.IsValid()) return;
    TArray<uint8> Bytes; { FScopeLock Guard(&ReplyStream->Lock); Swap(Bytes,ReplyStream->Incoming); }
    for(uint8 Byte:Bytes) {
        if(Byte!='\n') { StreamLine.Add(Byte); continue; }
        StreamLine.Add(0); const FString Line=UTF8_TO_TCHAR(reinterpret_cast<const char*>(StreamLine.GetData())); StreamLine.Empty();
        auto Envelope=Decode(Line); if(!Envelope) continue;
        const TSharedPtr<FJsonObject>* Message=nullptr; FString Delta;
        if(Envelope->TryGetObjectField(TEXT("message"),Message) && (*Message)->TryGetStringField(TEXT("content"),Delta)) StreamJson+=Delta;
        bool Done=false; if(Envelope->TryGetBoolField(TEXT("done"),Done) && Done) {
            auto MessageObject=MakeShared<FJsonObject>(); MessageObject->SetStringField(TEXT("content"),StreamJson); Envelope->SetObjectField(TEXT("message"),MessageObject); StreamEnvelope=Encode(Envelope);
        }
    }
    PreviewReply=PreviewText(StreamJson);
    if(FirstTextSeconds==0 && PreviewReply.Len()>=3) {
        FirstTextSeconds=FPlatformTime::Seconds()-TurnStartedAt;
        UE_LOG(LogTemp,Display,TEXT("CE_DIALOGUE_FIRST_TEXT seconds=%.2f"),FirstTextSeconds);
    }
}
void UCEDialogueComponent::BuildPanel() {
    TWeakObjectPtr<UCEDialogueComponent> Weak(this);
    static const FTextBlockStyle ButtonText=FTextBlockStyle(FCoreStyle::Get().GetWidgetStyle<FTextBlockStyle>(TEXT("ButtonText"))).SetFont(FCoreStyle::GetDefaultFontStyle("Bold",18));
    auto Text=[Weak](auto Getter) { return TAttribute<FText>::CreateLambda([Weak,Getter] { return FText::FromString(Weak.IsValid() ? Getter(Weak.Get()) : FString()); }); };
    auto Root=SNew(SCEChatPanel).Visibility_Lambda([Weak]{return Weak.IsValid() && Weak->bEditing ? EVisibility::Visible : EVisibility::Collapsed;})[
        SNew(SVerticalBox)
        +SVerticalBox::Slot().FillHeight(1)
        +SVerticalBox::Slot().AutoHeight().HAlign(HAlign_Center).Padding(20,16)[
            SNew(SBox).WidthOverride(920)[SNew(SBorder).BorderImage(FCoreStyle::Get().GetBrush(TEXT("WhiteBrush"))).Padding(24).BorderBackgroundColor(FLinearColor(.018f,.029f,.033f,.96f))[
                SNew(SVerticalBox)
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,6)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",14)).ColorAndOpacity(FLinearColor(.78f,.57f,.29f)).Text(FText::FromString(TEXT("CERO ESTRELLAS  /  CONVERSACIÓN")))]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,5)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Bold",24)).ColorAndOpacity(FLinearColor(.95f,.91f,.82f))
                    .Text(Text([](auto* D){ return D->Target.IsValid() ? D->Target->DisplayName : FString(TEXT("CLIENTE")); }))]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,16)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).ColorAndOpacity(FLinearColor(.53f,.67f,.65f)).Text(Text([](auto* D){return D->Target.IsValid() ? TEXT("Su pedido: ")+D->Target->OrderText() : FString();})).AutoWrapText(true)]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,18)[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",24)).ColorAndOpacity(FLinearColor(.94f,.94f,.90f)).AutoWrapText(true)
                    .Text(Text([](auto* D){ return !D->PreviewReply.IsEmpty() && D->bBusy ? D->PreviewReply : D->LastReply.IsEmpty() ? FString(D->bBusy ? TEXT("Un momento...") : TEXT("Te escucha. ¿Qué le dices?")) : D->LastReply; }))]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[SAssignNew(Input,SEditableTextBox).Font(FCoreStyle::GetDefaultFontStyle("Regular",22)).HintText(FText::FromString(TEXT("Habla con el cliente...")))
                    .OnKeyCharHandler_Lambda([](const FGeometry&,const FCharacterEvent&){return FParse::Param(FCommandLine::Get(),TEXT("CeroDialogueTest")) ? FReply::Handled() : FReply::Unhandled();})
                    .Text(Text([](auto* D){return D->Draft;}))
                    .OnTextChanged_Lambda([Weak](const FText& Value){if(Weak.IsValid()) Weak->Draft=Value.ToString();})
                    .ClearKeyboardFocusOnCommit(false)
                    .OnTextCommitted_Lambda([Weak](const FText& Value,ETextCommit::Type Type){if(Weak.IsValid() && Type==ETextCommit::OnEnter) Weak->Submit(Value.ToString());})]
                +SVerticalBox::Slot().AutoHeight().Padding(0,0,0,12)[SNew(SHorizontalBox)
                    +SHorizontalBox::Slot().AutoWidth().Padding(0,0,12,0)[SNew(SButton).ButtonColorAndOpacity(FLinearColor(.72f,.50f,.22f)).ContentPadding(FMargin(15,7)).TextStyle(&ButtonText).Text(FText::FromString(TEXT("Enviar ↵"))).IsEnabled_Lambda([Weak]{return Weak.IsValid() && !Weak->bBusy && !Weak->bRecording;})
                        .OnClicked_Lambda([Weak]{if(Weak.IsValid()) Weak->SubmitDraft(); return FReply::Handled();})]
                    +SHorizontalBox::Slot().AutoWidth().Padding(0,0,12,0)[SNew(SButton).TextStyle(&ButtonText).Text(Text([](auto* D){return FString(D->bRecording ? TEXT("Detener / transcribir") : TEXT("Grabar voz"));}))
                        .IsEnabled_Lambda([Weak]{return Weak.IsValid() && !Weak->bBusy;}).OnClicked_Lambda([Weak]{if(Weak.IsValid()) Weak->ToggleRecording(); return FReply::Handled();})]
                    +SHorizontalBox::Slot().AutoWidth().Padding(0,0,12,0)[SNew(SCheckBox).IsChecked_Lambda([Weak]{return Weak.IsValid() && Weak->bVoiceEnabled ? ECheckBoxState::Checked : ECheckBoxState::Unchecked;})
                        .OnCheckStateChanged_Lambda([Weak](ECheckBoxState V){if(Weak.IsValid()) {Weak->bVoiceEnabled=V==ECheckBoxState::Checked; if(!Weak->bVoiceEnabled) Weak->Speaker->Stop();}})
                        [SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",16)).Text(FText::FromString(TEXT("Escuchar voz")))]]
                    +SHorizontalBox::Slot().AutoWidth().Padding(0,0,12,0)[SNew(SButton).TextStyle(&ButtonText).Text(FText::FromString(TEXT("Seguir jugando"))).OnClicked_Lambda([Weak]{if(Weak.IsValid()) Weak->ResumePlay(); return FReply::Handled();})]
                    +SHorizontalBox::Slot().FillWidth(1).HAlign(HAlign_Right)[SNew(SButton).TextStyle(&ButtonText).Text(FText::FromString(TEXT("Cerrar [Esc]"))).OnClicked_Lambda([Weak]{if(Weak.IsValid()) Weak->Close(); return FReply::Handled();})]
                ]
                +SVerticalBox::Slot().AutoHeight()[SNew(STextBlock).Font(FCoreStyle::GetDefaultFontStyle("Regular",15)).ColorAndOpacity(FLinearColor(.58f,.69f,.67f)).AutoWrapText(true).Text(Text([](auto* D){return D->Status;}))]
            ]]
        ]
    ];
    Root->Owner=this; Panel=Root; GEngine->GameViewport->AddViewportWidgetContent(Panel.ToSharedRef(),50);
}

bool UCEDialogueComponent::ValidateReply(const FString& Json,FString& Text,FString& Emotion,float& Anger,bool& Leave) {
    const auto O=Decode(Json); if(!O || O->Values.Num()!=4) return false;
    FString T,E; double A; bool L;
    if(!O->TryGetStringField(TEXT("texto"),T) || !O->TryGetStringField(TEXT("emocion"),E) || !O->TryGetNumberField(TEXT("cambio_enfado"),A) || !O->TryGetBoolField(TEXT("quiere_irse"),L)) return false;
    T=T.TrimStartAndEnd(); bool Known=false; for(const auto* V:Emotions) Known|=E==V;
    if(T.IsEmpty() || T.Len()>350 || !Known || !FMath::IsFinite(A) || A<-.10001 || A>.30001) return false;
    Text=T; Emotion=E; Anger=float(A); Leave=L; return true;
}
bool UCEDialogueComponent::SetDraft(const FString& Text) {
    if(!Input) return false; Draft=Text;
    // Slate deliberately preserves focused editable text when a bound attribute changes.
    // SetText forces its editable layout to refresh while retaining our draft binding.
    TWeakObjectPtr<UCEDialogueComponent> Weak(this);
    Input->SetText(TAttribute<FText>::CreateLambda([Weak]{return FText::FromString(Weak.IsValid() ? Weak->Draft : FString());}));
    return true;
}
bool UCEDialogueComponent::SubmitDraft() { return Input && Submit(Draft); }
bool UCEDialogueComponent::Submit(const FString& Raw,bool bRetry) {
    if(!Target.IsValid() || Target->bLeaving || bBusy || bRecording) return false;
    const FString Text=Raw.TrimStartAndEnd();
    if(Text.IsEmpty() || Text.Len()>500) { Status=TEXT("Escribe entre 1 y 500 caracteres."); return false; }
    CancelRequests(); if(Speaker) Speaker->Stop(); const int32 Turn=Generation; bBusy=true;
    TurnStartedAt=FPlatformTime::Seconds(); FirstTextSeconds=0; ResponseSeconds=0;
    LastPlayerText=Text; LastReply.Empty(); bLastReplyFromOllama=false;
    Target->DialogueHoldUntil=GetWorld()->GetTimeSeconds()+90.f;
    Status=TEXT("El cliente está pensando..."); Target->ThinkingUntil=GetWorld()->GetTimeSeconds()+90.f;
    auto Context=MakeShared<FJsonObject>();
    // Identity selects the history on the engine side; the order is already in
    // the stable system prompt. Send only meaningful changes, not default zeros.
    if(Target->Anger>.5f) Context->SetStringField(TEXT("animo_actual"),TEXT("molesto"));
    if(Target->HitCount>0) Context->SetNumberField(TEXT("impactos_recibidos"),Target->HitCount);
    if(Target->WetMl>0) Context->SetStringField(TEXT("ropa_empapada_de"),Target->ReceivedLiquid);
    if(Target->bFilming) Context->SetBoolField(TEXT("grabando_desastre_ajeno"),true);
    if(!Target->SpecialRequest.IsEmpty()) Context->SetStringField(TEXT("preferencia_del_pedido"),Target->SpecialRequest);
    if(Target->bDrinkServed) Context->SetBoolField(TEXT("desayuno_servido"),true);
    if(GetWorld()->GetTimeSeconds()<Target->FallenUntil) Context->SetBoolField(TEXT("en_el_suelo"),true);
    TArray<TSharedPtr<FJsonValue>> Incidents;
    for(const auto& Event:Events) if(Event.CustomerId==Target->CustomerId && (Event.Kind==TEXT("customer_hit") || Event.Kind==TEXT("customer_fallen") || Event.Kind==TEXT("customer_slipped") || Event.Kind==TEXT("laptop_destroyed") || Event.Kind==TEXT("pastry_stuck"))) {
        auto O=MakeShared<FJsonObject>(); O->SetStringField(TEXT("accion"),Event.Kind.ToString()); O->SetStringField(TEXT("objeto"),Event.ObjectName); O->SetNumberField(TEXT("gravedad"),Event.Severity); Incidents.Add(MakeShared<FJsonValueObject>(O));
        if(Incidents.Num()>8) Incidents.RemoveAt(0);
    }
    Context->SetArrayField(TEXT("incidentes_recibidos"),Incidents);
    const TCHAR* Personalities[]={TEXT("Inversor cripto engreído, sarcástico; presumes de NFTs."),TEXT("Hipster del café, pedante."),TEXT("Influencer pasivo-agresiva; te importan outfit y seguidores.")};
    auto Body=MakeShared<FJsonObject>(); Body->SetStringField(TEXT("model"),Model); Body->SetBoolField(TEXT("stream"),true); Body->SetBoolField(TEXT("think"),false); Body->SetStringField(TEXT("keep_alive"),TEXT("20m"));
    // Constrain generation as well as validating on the engine side: a typo in
    // a field name must not cause a second full inference and delay the reply.
    // dialogo precedes reaccion in the grammar, so words can appear while the
    // bounded gameplay metadata is still being generated. Apply both atomically.
    Body->SetObjectField(TEXT("format"),Decode(TEXT("{\"type\":\"object\",\"properties\":{\"dialogo\":{\"type\":\"object\",\"properties\":{\"texto\":{\"type\":\"string\",\"minLength\":1,\"maxLength\":100}},\"required\":[\"texto\"],\"additionalProperties\":false},\"reaccion\":{\"type\":\"object\",\"properties\":{\"emocion\":{\"type\":\"string\",\"enum\":[\"neutral\",\"enfadado\",\"humillado\",\"triste\",\"divertido\"]},\"cambio_enfado\":{\"type\":\"number\",\"enum\":[-0.1,0,0.1,0.2,0.3]},\"quiere_irse\":{\"type\":\"boolean\"}},\"required\":[\"emocion\",\"cambio_enfado\",\"quiere_irse\"],\"additionalProperties\":false}},\"required\":[\"dialogo\",\"reaccion\"],\"additionalProperties\":false}")));
    auto Options=MakeShared<FJsonObject>(); Options->SetNumberField(TEXT("num_gpu"),0); Options->SetNumberField(TEXT("num_thread"),6); Options->SetNumberField(TEXT("num_ctx"),2048); Options->SetNumberField(TEXT("num_predict"),128); Options->SetNumberField(TEXT("temperature"),.1); Body->SetObjectField(TEXT("options"),Options);
    // Keep instructions and previous messages stable; changing facts belong to
    // this turn's user message, which is preserved verbatim for prefix reuse.
    const FString System=FString(TEXT("Eres cliente del restaurante Cero Estrellas; hablas con tu camarero, el usuario. "))+Personalities[Target->Archetype()]+TEXT(" Tu pedido: ")+Target->OrderText()+TEXT(". Responde TODAS sus preguntas sobre cualquier tema, recuerda la conversación. Respuesta en español, hasta 100 caracteres, sin añadir preguntas. Hablas en primera persona de TU desayuno y en segunda persona de las cosas del camarero. Los incidentes recibidos los provoca el camarero contra ti; no inventes incidentes. Devuelve solo JSON: {\"dialogo\":{\"texto\":\"respuesta\"},\"reaccion\":{\"emocion\":\"neutral\",\"cambio_enfado\":0,\"quiere_irse\":false}}. emocion: neutral/enfadado/humillado/triste/divertido; cambio_enfado entre -0.1 y 0.3. Disculpas reducen enfado; preguntas normales no lo aumentan.");
    TArray<TSharedPtr<FJsonValue>> Messages; Messages.Add(Message(TEXT("system"),System));
    // A neutral identity example reinforces the roles without planting a spill
    // or an insult that the model might repeat during unrelated questions.
    Messages.Add(Message(TEXT("user"),TEXT("Camarero: ¿Quién eres y dónde estás?")));
    Messages.Add(Message(TEXT("assistant"),TEXT("{\"dialogo\":{\"texto\":\"Soy cliente de Cero Estrellas; tú eres mi camarero.\"},\"reaccion\":{\"emocion\":\"neutral\",\"cambio_enfado\":0,\"quiere_irse\":false}}")));
    auto& Turns=History.FindOrAdd(Target->CustomerId);
    for(const auto& T:Turns) {
        // Past conversation needs words and roles; current facts and the schema
        // are sent once below. Repeating old state/JSON wastes CPU prefill time.
        Messages.Add(Message(TEXT("user"),TEXT("Camarero: ")+T.PlayerText));
        Messages.Add(Message(TEXT("assistant"),T.ReplyText));
    }
    const FString UserMessage=TEXT("[Estado del cliente; incidentes recibidos del camarero]\n")+Encode(Context)+TEXT("\nCamarero: ")+Text;
    Messages.Add(Message(TEXT("user"),UserMessage));
    const FString Perspective=Incidents.IsEmpty() ? FString() : TEXT("Tú eres el cliente que RECIBIÓ los incidentes. El camarero te habla. Si hablas del incidente, contesta como víctima: el camarero actúa, tú reaccionas. No repitas su frase ni hables como si tú fueras él.");
    if(!Perspective.IsEmpty()) Messages.Add(Message(TEXT("system"),Perspective));
    Body->SetArrayField(TEXT("messages"),Messages);
    TestTrace(Encode(Body),TEXT("request"),Turn);
    auto Request=FHttpModule::Get().CreateRequest(); Request->SetURL(TEXT("http://127.0.0.1:11434/api/chat")); Request->SetVerb(TEXT("POST")); Request->SetHeader(TEXT("Content-Type"),TEXT("application/json")); Request->SetTimeout(90); Request->SetActivityTimeout(90); Request->SetContentAsString(Encode(Body));
    ReplyStream=MakeShared<FCEReplyStream,ESPMode::ThreadSafe>(); const auto StreamData=ReplyStream;
    Request->SetDelegateThreadPolicy(EHttpRequestDelegateThreadPolicy::CompleteOnGameThread);
    Request->SetResponseBodyReceiveStreamDelegateV2(FHttpRequestStreamDelegateV2::CreateLambda([StreamData](void* Data,int64& Length) {
        FScopeLock Guard(&StreamData->Lock);
        if(Length<0 || StreamData->Received+Length>128*1024) { Length=0; return; }
        StreamData->Incoming.Append(static_cast<uint8*>(Data),int32(Length)); StreamData->Received+=Length;
    }));
    Request->OnProcessRequestComplete().BindWeakLambda(this,[this,Turn,bRetry,Text](FHttpRequestPtr,FHttpResponsePtr Response,bool OK) {
        if(Generation!=Turn || !Target.IsValid() || Target->bLeaving) return;
        ConsumeReplyStream(); ResponseSeconds=FPlatformTime::Seconds()-TurnStartedAt;
        bBusy=false; Target->ThinkingUntil=0.f;
        if(!OK || !Response || Response->GetResponseCode()!=200) { UE_LOG(LogTemp,Warning,TEXT("CE_DIALOGUE_HTTP_FAILED code=%d connected=%d"),Response ? Response->GetResponseCode() : 0,OK); Status=TEXT("No se ha podido escuchar la respuesta. Prueba a enviar de nuevo."); return; }
        TestTrace(StreamEnvelope,TEXT("response"),Turn);
        const auto Envelope=Decode(StreamEnvelope); const TSharedPtr<FJsonObject>* Msg=nullptr; FString Json;
        FString Answer,Emotion; float Anger=0; bool Leave=false;
        if(!Envelope || !Envelope->TryGetObjectField(TEXT("message"),Msg) || !(*Msg)->TryGetStringField(TEXT("content"),Json) || !ValidateReply(NormalizeProviderReply(Json),Answer,Emotion,Anger,Leave)) {
            FString Reason; if(Envelope) Envelope->TryGetStringField(TEXT("done_reason"),Reason);
            UE_LOG(LogTemp,Warning,TEXT("CE_DIALOGUE_INVALID done_reason=%s chars=%d retry=%d"),*Reason,Json.Len(),bRetry);
            if(!bRetry) {
                const FString RetryText=LastPlayerText; bBusy=true; Status=TEXT("El cliente reformula su respuesta...");
                GetWorld()->GetTimerManager().SetTimerForNextTick(FTimerDelegate::CreateWeakLambda(this,[this,RetryText,Turn] {
                    if(Generation!=Turn || !Target.IsValid()) return; bBusy=false; Submit(RetryText,true);
                }));
                return;
            }
            Status=TEXT("El cliente ha perdido el hilo. Prueba a decirlo de otra forma."); return;
        }
        LastReply=Answer; LastEmotion=Emotion; bLastReplyFromOllama=true; ++ValidReplies;
        if(Emotion==TEXT("neutral") || Emotion==TEXT("divertido")) Anger=FMath::Min(Anger,0.f);
        auto& Turns=History.FindOrAdd(Target->CustomerId); Turns.Add(FCEConversationTurn{Text,Answer,Json});
        int32 HistoryChars=0; for(const auto& T:Turns) HistoryChars+=T.PlayerText.Len()+T.ReplyText.Len();
        // More short exchanges fit without evicting a cached prefix after six
        // replies. Long exchanges still have a bounded prompt and memory cost.
        while(Turns.Num()>1 && (Turns.Num()>12 || HistoryChars>2500)) {
            HistoryChars-=Turns[0].PlayerText.Len()+Turns[0].ReplyText.Len(); Turns.RemoveAt(0);
        }
        if(History.Num()>24) { for(auto It=History.CreateIterator();It;++It) if(It.Key()!=Target->CustomerId) { It.RemoveCurrent(); break; } }
        Status=TEXT("Tu turno. Puedes seguir hablando."); Target->ApplyDialogue(Answer,Emotion,Anger,Leave);
        UE_LOG(LogTemp,Display,TEXT("CE_DIALOGUE_REPLY customer=%s emotion=%s chars=%d anger=%.2f wants_leave=%d first_text=%.2f complete=%.2f"),*Target->CustomerId,*Emotion,Answer.Len(),Target->Anger,Leave,FirstTextSeconds,ResponseSeconds);
        if(bVoiceEnabled && !Target->bLeaving) SpeakReply(Answer,Turn);
    }); Requests.Add(Request);
    if(!Request->ProcessRequest()) { bBusy=false; Target->ThinkingUntil=0.f; Status=TEXT("No se ha podido enviar. Inténtalo de nuevo."); return false; } ResumePlay(); return true;
}
void UCEDialogueComponent::SpeakReply(const FString& Text,int32 Turn) {
    auto Body=MakeShared<FJsonObject>(); Body->SetStringField(TEXT("text"),Text);
    auto R=FHttpModule::Get().CreateRequest(); R->SetURL(TEXT("http://127.0.0.1:11435/speak")); R->SetVerb(TEXT("POST")); R->SetHeader(TEXT("Content-Type"),TEXT("application/json")); R->SetContentAsString(Encode(Body)); R->SetTimeout(30);
    R->OnProcessRequestComplete().BindWeakLambda(this,[this,Turn](FHttpRequestPtr,FHttpResponsePtr Response,bool OK) {
        if(Turn!=Generation || !Target.IsValid() || !bVoiceEnabled || bRecording) return;
        if(!OK || !Response || Response->GetResponseCode()!=200) { Status=TEXT("La voz no está disponible. Puedes leer la respuesta y seguir hablando."); return; }
        auto Data=Response->GetContent(); FWaveModInfo Wave;
        if(Data.Num()>4*1024*1024 || !Wave.ReadWaveInfo(Data.GetData(),Data.Num()) || *Wave.pBitsPerSample!=16 || *Wave.pChannels!=1) { Status=TEXT("No se ha podido reproducir la voz. Puedes seguir leyendo y hablando."); return; }
        Voice=NewObject<USoundWaveProcedural>(this); Voice->SetSampleRate(*Wave.pSamplesPerSec); Voice->NumChannels=1;
        Voice->Duration=float(Wave.SampleDataSize)/(*Wave.pSamplesPerSec*2); Voice->QueueAudio(Wave.SampleDataStart,Wave.SampleDataSize);
        Speaker->SetWorldLocation(Target->HeadLocation()); Speaker->SetPitchMultiplier(Target->Archetype()==0 ? .94f : Target->Archetype()==1 ? 1.02f : 1.12f);
        Speaker->SetSound(Voice); Speaker->Play(); ++PlayedVoices; Target->VoiceStarted(Voice->Duration/Speaker->PitchMultiplier);
        TArray<float> Envelope; const int32 Frames=Wave.SampleDataSize/2, Window=FMath::Max(1,int32(*Wave.pSamplesPerSec/30));
        for(int32 Start=0;Start<Frames;Start+=Window) {
            double Energy=0; int32 Count=0;
            for(int32 i=Start;i<FMath::Min(Frames,Start+Window);++i) { const int16 Sample=int16(uint16(Wave.SampleDataStart[i*2]) | uint16(Wave.SampleDataStart[i*2+1])<<8); const float V=float(Sample)/32768.f; Energy+=V*V; ++Count; }
            Envelope.Add(FMath::Clamp(float(FMath::Sqrt(Energy/FMath::Max(1,Count)))*6.f,0.f,1.f));
        }
        Target->SetSpeechEnvelope(Envelope,Voice->Duration/Speaker->PitchMultiplier);
        Target->Speak(LastReply,Voice->Duration+.5f);
        UE_LOG(LogTemp,Display,TEXT("CE_DIALOGUE_VOICE duration=%.2f"),Voice->Duration);
    }); Requests.Add(R); R->ProcessRequest();
}
void UCEDialogueComponent::ToggleRecording() {
    if(!bEditing) bRecordingOnTheMove=true;
    if(bRecording) { StopRecording(true); return; }
    if(!Target.IsValid() || bBusy) return;
    CancelRequests(); if(Speaker) Speaker->Stop();
    Capture=MakeUnique<Audio::FAudioCapture>(); CaptureData=MakeShared<FCECaptureBuffer,ESPMode::ThreadSafe>();
    Audio::FAudioCaptureDeviceParams Params; Params.PCMAudioEncoding=Audio::EPCMAudioEncoding::FLOATING_POINT_32;
    auto Buffer=CaptureData;
    if(!Capture->OpenAudioCaptureStream(Params,[Buffer](const void* Data,int32 Frames,int32 Channels,int32 Rate,double,bool) {
        FScopeLock Lock(&Buffer->Lock); Buffer->Rate=Rate;
        const auto* Float=static_cast<const float*>(Data); const int32 Count=FMath::Min(Frames,FMath::Max(0,Rate*15-Buffer->Samples.Num()));
        for(int32 i=0;i<Count;++i) { float V=0; for(int32 C=0;C<Channels;++C) V+=Float[i*Channels+C]; V/=FMath::Max(1,Channels); Buffer->Samples.Add(int16(FMath::Clamp(FMath::IsFinite(V)? V : 0.f,-1.f,1.f)*32767)); }
    },1024) || !Capture->StartStream()) { Capture.Reset(); CaptureData.Reset(); Status=TEXT("Micrófono no disponible. Comprueba el dispositivo predeterminado y los permisos de Windows; puedes escribir."); return; }
    bRecording=true; RecordStartedAt=GetWorld()->GetTimeSeconds(); Target->DialogueHoldUntil=RecordStartedAt+45;
    Status=TEXT("Grabando el micrófono (máximo 15 s). Pulsa Detener o V para transcribir.");
}
void UCEDialogueComponent::StopRecording(bool bSend) {
    if(!Capture) return; Capture->StopStream(); Capture->CloseStream(); Capture.Reset(); bRecording=false;
    TArray<uint8> Wav;
    if(bSend && CaptureData) {
        // Keep the buffer alive until its lock is released, including the short-recording return.
        const auto Buffer=CaptureData;
        FScopeLock Lock(&Buffer->Lock); const auto& PCM=Buffer->Samples; const int32 Rate=Buffer->Rate;
        if(PCM.Num()<Rate/4) { Status=TEXT("Grabación demasiado corta. Vuelve a intentarlo."); CaptureData.Reset(); return; }
        Wav.Append(reinterpret_cast<const uint8*>("RIFF"),4); Put32(Wav,36+PCM.Num()*2); Wav.Append(reinterpret_cast<const uint8*>("WAVEfmt "),8);
        Put32(Wav,16); Put16(Wav,1); Put16(Wav,1); Put32(Wav,Rate); Put32(Wav,Rate*2); Put16(Wav,2); Put16(Wav,16);
        Wav.Append(reinterpret_cast<const uint8*>("data"),4); Put32(Wav,PCM.Num()*2); Wav.Append(reinterpret_cast<const uint8*>(PCM.GetData()),PCM.Num()*2);
    }
    CaptureData.Reset(); if(!Wav.IsEmpty()) TranscribeWav(Wav);
}
void UCEDialogueComponent::TranscribeWav(const TArray<uint8>& Wav) {
    if(!Target.IsValid() || bBusy) return; CancelRequests(); bBusy=true; const int32 Turn=Generation;
    Status=TEXT("Pasando tu voz a texto...");
    auto R=FHttpModule::Get().CreateRequest(); R->SetURL(TEXT("http://127.0.0.1:11435/transcribe")); R->SetVerb(TEXT("POST")); R->SetHeader(TEXT("Content-Type"),TEXT("audio/wav")); R->SetContent(Wav); R->SetTimeout(30);
    R->OnProcessRequestComplete().BindWeakLambda(this,[this,Turn](FHttpRequestPtr,FHttpResponsePtr Response,bool OK) {
        if(Turn!=Generation || !Target.IsValid()) return; bBusy=false;
        auto O=OK && Response && Response->GetResponseCode()==200 ? Decode(Response->GetContentAsString()) : nullptr;
        FString Text;
        if(!O || !O->TryGetStringField(TEXT("text"),Text)) { Status=TEXT("No se ha podido entender la grabación. Puedes volver a grabar o escribir."); return; }
        if(Text.TrimStartAndEnd().IsEmpty()) { Status=TEXT("No se ha reconocido voz. Prueba de nuevo o escribe."); return; }
        Transcript=Text.Left(500); SetDraft(Transcript); if(bRecordingOnTheMove) { EditConversation(); bRecordingOnTheMove=false; }
        Status=TEXT("Revisa la transcripción y pulsa Enviar para hablar con el cliente.");
        UE_LOG(LogTemp,Display,TEXT("CE_DIALOGUE_TRANSCRIBED chars=%d"),Transcript.Len());
    }); Requests.Add(R); if(!R->ProcessRequest()) { bBusy=false; Status=TEXT("No se ha podido procesar la grabación. Puedes escribir."); }
}
