#include "CEGameplay.h"
#include "Engine/StaticMesh.h"
#include "CEDialogue.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Components/StaticMeshComponent.h"
#include "Framework/Application/SlateApplication.h"
#include "InputKeyEventArgs.h"
#include "UnrealClient.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "HAL/FileManager.h"
#include "Serialization/JsonSerializer.h"
#include "Dom/JsonObject.h"
#include "EngineUtils.h"

void ACEGameMode::DialogueSmokeStep()
{
    auto* PC=GetWorld()->GetFirstPlayerController(); auto* Player=PC ? Cast<ACEPlayer>(PC->GetPawn()) : nullptr;
    if(!Player) { bSmoke=false; FPlatformMisc::RequestExitWithStatus(false,1); return; }
    auto* D=Player->Dialogue.Get(); auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>();
    const float Now=GetWorld()->GetTimeSeconds();
    FString Evidence; FParse::Value(FCommandLine::Get(),TEXT("CeroEvidenceDir="),Evidence); IFileManager::Get().MakeDirectory(*Evidence,true);
    auto Check=[this](const TCHAR* N,bool P){LoopChecks.Add(N,P); UE_LOG(LogTemp,Display,TEXT("CE_DIALOGUE_CHECK %s=%d"),N,P);};
    auto Shot=[Evidence](const TCHAR* N){ FScreenshotRequest::RequestScreenshot(Evidence/(FString(N)+TEXT(".png")),true,false); };
    auto Wait=[this,Now,D] { if(D->IsBusy()) { SmokeNextAt=Now+.25f; return true; } return false; };
    UE_LOG(LogTemp,Display,TEXT("CE_DIALOGUE_STAGE %d"),SmokeStage);
    switch(SmokeStage)
    {
    case 0:
    {
        // The fixture does not bypass the real HTTP, Slate or customer response path.
        for(TActorIterator<ACECustomer> It(GetWorld());It;++It) It->Destroy();
        SmokeCustomer=GetWorld()->SpawnActor<ACECustomer>(FVector(230,350,92),FRotator::ZeroRotator);
        SmokeCustomer->Configure(TEXT("qa-dialogue"),TEXT("HIPSTER PURISTA"),FLinearColor(.38f,.22f,.12f),1);
        SmokeCustomer->State=ECECustomerState::Ordering; SmokeCustomer->RequestedDrink=ECEDrink::Latte; SmokeCustomer->bWantsPastry=true;
        Player->SetActorLocation(FVector(20,350,92)); PC->SetControlRotation((SmokeCustomer->GetActorLocation()+FVector(0,0,53)-Player->Camera->GetComponentLocation()).Rotation());
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::T,IE_Pressed,1)); PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::T,IE_Released,0));
        ++SmokeStage; SmokeNextAt=Now+.6f; break;
    }
    case 1:
    {
        Check(TEXT("T_opens_dialogue_panel_and_cursor"),D->IsOpen() && PC->bShowMouseCursor);
        Check(TEXT("conversation_holds_customer_patience"),SmokeCustomer->DialogueHoldUntil>Now+30 && SmokeCustomer->Patience>=109);
        FString T,E; float A; bool L;
        Check(TEXT("strict_contract_accepts_valid_json"),D->ValidateReply(TEXT("{\"texto\":\"Hola\",\"emocion\":\"neutral\",\"cambio_enfado\":0,\"quiere_irse\":false}"),T,E,A,L));
        Check(TEXT("contract_rejects_missing_fields"),!D->ValidateReply(TEXT("{\"texto\":\"Hola\"}"),T,E,A,L));
        Check(TEXT("contract_rejects_unknown_emotion"),!D->ValidateReply(TEXT("{\"texto\":\"Hola\",\"emocion\":\"hack\",\"cambio_enfado\":0,\"quiere_irse\":false}"),T,E,A,L));
        Check(TEXT("contract_rejects_excessive_gameplay_delta"),!D->ValidateReply(TEXT("{\"texto\":\"Hola\",\"emocion\":\"neutral\",\"cambio_enfado\":100,\"quiere_irse\":false}"),T,E,A,L));
        Check(TEXT("empty_or_long_input_not_sent"),!D->Submit(TEXT("  ")) && !D->Submit(FString::ChrN(501,TEXT('x'))));
        Check(TEXT("text_field_accepts_spanish_input"),D->SetDraft(TEXT("Buenos días, ¿qué desayuno has pedido?")));
        Check(TEXT("UI_draft_starts_real_Ollama_request"),D->SubmitDraft());
        Check(TEXT("sending_restores_game_control_while_HTTP_is_pending"),D->IsOpen() && D->IsBusy() && !D->IsEditing() && !PC->bShowMouseCursor);
        LoopPreviousCustomerPosition=Player->GetActorLocation();
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::D,IE_Pressed,1));
        Check(TEXT("concurrent_turn_is_rejected"),!D->Submit(TEXT("Otra petición")));
        ++SmokeStage; SmokeNextAt=Now+.3f; break;
    }
    case 2:
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::D,IE_Released,0));
        Check(TEXT("player_can_move_during_real_model_inference"),FVector::Dist2D(Player->GetActorLocation(),LoopPreviousCustomerPosition)>20.f && D->IsOpen());
        if(Wait()) break;
        Check(TEXT("real_Ollama_reply_is_validated_and_displayed"),D->ValidReplies==1 && D->bLastReplyFromOllama && !D->LastReply.IsEmpty() && SmokeCustomer->Reaction==D->LastReply);
        Check(TEXT("reply_knows_the_actual_order"),D->LastReply.Contains(TEXT("latte"),ESearchCase::IgnoreCase) || D->LastReply.Contains(TEXT("leche"),ESearchCase::IgnoreCase));
        Check(TEXT("progressive_text_arrives_before_validated_response"),D->FirstTextSeconds>0 && D->FirstTextSeconds<D->ResponseSeconds);
        Check(TEXT("dialogue_UI_hides_backend_terminology"),!D->Status.Contains(TEXT("Ollama")) && !D->Status.Contains(TEXT("IA local")));
        D->EditConversation(); Shot(TEXT("01_dialogar_respuesta_real")); SmokeStage=14; SmokeNextAt=Now+6.f; break;
    case 14:
        if(Wait()) break;
        Check(TEXT("Piper_audio_reaches_Unreal_playback"),D->PlayedVoices>=1);
        D->SetDraft(TEXT("¿En qué restaurante estás y con quién hablas?")); D->SubmitDraft(); SmokeStage=15; SmokeNextAt=Now+.3f; break;
    case 15:
        if(Wait()) break;
        Check(TEXT("customer_knows_restaurant_and_waiter_roles"),D->LastReply.Contains(TEXT("Cero Estrellas"),ESearchCase::IgnoreCase) && (D->LastReply.Contains(TEXT("camarero"),ESearchCase::IgnoreCase) || D->LastReply.Contains(TEXT("barista"),ESearchCase::IgnoreCase)));
        D->SetDraft(TEXT("No, pides tú. ¿Qué quieres desayunar?")); D->SubmitDraft(); SmokeStage=16; SmokeNextAt=Now+.3f; break;
    case 16:
        if(Wait()) break;
        Check(TEXT("customer_handles_player_correction_without_invented_spill"),(D->LastReply.Contains(TEXT("latte"),ESearchCase::IgnoreCase) || D->LastReply.Contains(TEXT("leche"),ESearchCase::IgnoreCase)) && !D->LastReply.Contains(TEXT("camisa"),ESearchCase::IgnoreCase) && !D->LastReply.Contains(TEXT("te sirvo"),ESearchCase::IgnoreCase));
        D->SetDraft(TEXT("¿Cuál es la capital de Francia?")); D->SubmitDraft(); SmokeStage=17; SmokeNextAt=Now+.3f; break;
    case 17:
        if(Wait()) break;
        Check(TEXT("customer_can_discuss_topics_outside_the_restaurant"),D->LastReply.Contains(TEXT("París"),ESearchCase::IgnoreCase) || D->LastReply.Contains(TEXT("Paris"),ESearchCase::IgnoreCase));
        Shot(TEXT("04_conversacion_libre")); SmokeStage=3; SmokeNextAt=Now+6.f; break;
    case 3:
        if(Wait()) break;
        Check(TEXT("Piper_audio_reaches_Unreal_playback"),D->PlayedVoices>=1);
        D->SetDraft(TEXT("Mi perro se llama Croqueta. Recuerda su nombre, por favor."));
        if(D->SubmitDraft()) { ++SmokeStage; SmokeNextAt=Now+.3f; } break;
    case 4:
        if(Wait()) break;
        D->SetDraft(TEXT("¿Cómo se llama mi perro?")); D->SubmitDraft(); ++SmokeStage; SmokeNextAt=Now+.3f; break;
    case 5:
        if(Wait()) break;
        Check(TEXT("customer_remembers_previous_turns"),D->LastReply.Contains(TEXT("Croqueta"),ESearchCase::IgnoreCase) && !D->LastReply.Contains(TEXT("mi perro"),ESearchCase::IgnoreCase));
        SmokeCustomer->Anger=.65f; SmokeCustomer->HitCount=1; Cafe->Observe(TEXT("customer_hit"),SmokeCustomer->CustomerId,TEXT("Café derramado en la camisa"),.5f);
        D->SetDraft(TEXT("¿Qué líquido te he echado en la camisa? Discúlpame.")); D->SubmitDraft(); ++SmokeStage; SmokeNextAt=Now+.3f; break;
    case 6:
        if(Wait()) break;
        Check(TEXT("dialogue_reacts_to_physical_context"),D->LastReply.Contains(TEXT("café"),ESearchCase::IgnoreCase) || D->LastReply.Contains(TEXT("camisa"),ESearchCase::IgnoreCase) || D->LastReply.Contains(TEXT("mancha"),ESearchCase::IgnoreCase));
        Check(TEXT("customer_does_not_claim_to_have_spilled_on_player"),!D->LastReply.Contains(TEXT("te he echado"),ESearchCase::IgnoreCase) && !D->LastReply.Contains(TEXT("te he derramado"),ESearchCase::IgnoreCase) && !D->LastReply.Contains(TEXT("eché"),ESearchCase::IgnoreCase));
        Check(TEXT("AI_anger_change_stays_within_gameplay_bounds"),SmokeCustomer->Anger>=.549f && SmokeCustomer->Anger<=.951f);
        Check(TEXT("ordinary_dialogue_does_not_trigger_boss"),Cafe->PraiseCount==0);
        D->EditConversation(); Shot(TEXT("02_memoria_y_contexto_fisico")); ++SmokeStage; SmokeNextAt=Now+6.f; break;
    case 7:
    {
        FString WavPath; FParse::Value(FCommandLine::Get(),TEXT("CeroTestWav="),WavPath); TArray<uint8> Wav;
        Check(TEXT("speech_fixture_available"),FFileHelper::LoadFileToArray(Wav,*WavPath)); D->TranscribeWav(Wav);
        ++SmokeStage; SmokeNextAt=Now+.3f; break;
    }
    case 8:
        if(Wait()) break;
        Check(TEXT("Whisper_returns_editable_spanish_transcript"),D->Transcript.Len()>20 && D->Transcript.Contains(TEXT("leche"),ESearchCase::IgnoreCase));
        Check(TEXT("transcript_is_not_sent_without_confirmation"),D->ValidReplies==7);
        Shot(TEXT("03_transcripcion_whisper")); SmokeStage=13; SmokeNextAt=Now+1.f; break;
    case 13:
        if(Wait()) break;
        if(D->SubmitDraft()) { SmokeStage=9; SmokeNextAt=Now+.3f; } break;
    case 9:
        if(Wait()) break;
        Check(TEXT("confirmed_transcript_reaches_Ollama"),D->ValidReplies==8 && D->LastPlayerText==D->Transcript && D->bLastReplyFromOllama);
        PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::V,IE_Pressed,1)); PC->InputKey(FInputKeyEventArgs::CreateSimulated(EKeys::V,IE_Released,0));
        // InputKey queues the action for the next player-input tick. Inspect
        // capture after that tick, before Escape closes the real device.
        ++SmokeStage; SmokeNextAt=Now+.6f; break;
    case 10:
        bSmokeMicrophoneAvailable=D->bRecording;
        Check(TEXT("V_starts_microphone_without_opening_modal_editor"),D->bRecording && !D->IsEditing());
        // Route Escape through Slate after focus loss, not directly through Close().
        FSlateApplication::Get().ClearKeyboardFocus();
        D->bSmokeInjectingEscape=true;
        FSlateApplication::Get().ProcessKeyDownEvent(FKeyEvent(EKeys::Escape,FModifierKeysState(),0,false,0,0));
        D->bSmokeInjectingEscape=false;
        Check(TEXT("Escape_closes_panel_and_recording"),!D->IsOpen() && !D->bRecording && !PC->bShowMouseCursor);
        D->Open(SmokeCustomer);
        D->ToggleRecording(); if(D->bRecording) D->ToggleRecording();
        Check(TEXT("short_microphone_recording_stops_safely"),!D->bRecording && !D->IsBusy());
        D->bVoiceEnabled=false;
        Check(TEXT("reopening_preserves_the_customer_conversation"),!D->LastReply.IsEmpty());
        D->Submit(TEXT("¿Qué quieres beber?")); Cafe->Observe(TEXT("customer_hit"),SmokeCustomer->CustomerId,TEXT("Interrupción durante la respuesta"),.1f);
        Check(TEXT("new_physical_incident_cancels_reply_to_old_context"),!D->IsBusy() && D->ValidReplies==8 && D->PreviewReply.IsEmpty());
        D->Submit(TEXT("¿Qué quieres beber?")); D->Close(); ++SmokeStage; SmokeNextAt=Now+3.f; break;
    case 11:
        Check(TEXT("closing_cancels_async_turn_without_stale_reply"),!D->IsBusy() && D->ValidReplies==8 && D->PreviewReply.IsEmpty());
        D->Open(SmokeCustomer); SmokeCustomer->bLeaving=true; ++SmokeStage; SmokeNextAt=Now+.3f; break;
    case 12:
    {
        Check(TEXT("departing_customer_closes_conversation"),!D->IsOpen());
        const TCHAR* Meshes[]={TEXT("SM_Cup"),TEXT("SM_Pitcher"),TEXT("SM_Croissant"),TEXT("SM_BevelCube"),TEXT("SM_Head"),TEXT("SM_Torso"),TEXT("SM_Sleeve"),TEXT("SM_Forearm"),TEXT("SM_Hand"),TEXT("SM_Trouser"),TEXT("SM_Shin"),TEXT("SM_Shoe"),TEXT("SM_Hair"),TEXT("SM_Glasses"),TEXT("SM_Wand"),TEXT("SM_Bottle"),TEXT("SM_Pot"),TEXT("SM_Leaf"),TEXT("SM_Hopper"),TEXT("SM_SeatBack"),TEXT("SM_TrayRim"),TEXT("SM_Filter"),TEXT("SM_FilterGrip"),TEXT("SM_SteamKnob")};
        bool Loaded=true; for(const auto* N:Meshes) Loaded&=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Prototype/Meshes/%s.%s"),N,N))!=nullptr;
        Check(TEXT("all_24_authored_models_available"),Loaded);
        bool Passed=true; auto Report=MakeShared<FJsonObject>(); for(const auto& P:LoopChecks){Report->SetBoolField(P.Key,P.Value); Passed&=P.Value;}
        Report->SetBoolField(TEXT("passed"),Passed); Report->SetStringField(TEXT("version"),TEXT("0.6.0"));
        Report->SetNumberField(TEXT("validated_live_replies"),D->ValidReplies); Report->SetNumberField(TEXT("played_Piper_voices"),D->PlayedVoices);
        Report->SetBoolField(TEXT("microphone_device_opened"),bSmokeMicrophoneAvailable);
        Report->SetBoolField(TEXT("human_microphone_conversation_verified"),false); Report->SetBoolField(TEXT("human_playthrough"),false);
        FrameSamples.Sort(); if(!FrameSamples.IsEmpty()) {
            Report->SetNumberField(TEXT("frame_samples"),FrameSamples.Num());
            Report->SetNumberField(TEXT("frame_p50_ms"),FrameSamples[FrameSamples.Num()/2]);
            Report->SetNumberField(TEXT("frame_p95_ms"),FrameSamples[FMath::Min(FrameSamples.Num()-1,FMath::FloorToInt(FrameSamples.Num()*.95f))]);
            Report->SetNumberField(TEXT("frame_max_ms"),FrameSamples.Last());
        }
        FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json)); FFileHelper::SaveStringToFile(Json,*(Evidence/TEXT("dialogue_result.json")));
        UE_LOG(LogTemp,Display,TEXT("CE_DIALOGUE_RESULT passed=%d checks=%d"),Passed,LoopChecks.Num());
        bSmoke=false; FPlatformMisc::RequestExitWithStatus(false,Passed ? 0 : 1); break;
    }
    }
}
