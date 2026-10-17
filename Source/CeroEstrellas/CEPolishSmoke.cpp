#include "CEGameplay.h"
#include "CEKitchen.h"
#include "CECafeLayout.h"
#include "CEDialogue.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/CapsuleComponent.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Components/TextRenderComponent.h"
#include "GameFramework/PlayerController.h"
#include "Camera/CameraComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "UnrealClient.h"
#include "InputKeyEventArgs.h"

void ACEGameMode::PolishSmokeStep()
{
    auto* Player=Cast<ACEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* PC=UGameplayStatics::GetPlayerController(this,0);
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>(); const float Now=GetWorld()->GetTimeSeconds();
    auto Check=[this](const TCHAR* Name,bool Pass) { LoopChecks.Add(Name,Pass); UE_LOG(LogTemp,Display,TEXT("CE_POLISH_CHECK %s=%d"),Name,Pass); };
    auto Shot=[](const TCHAR* Name) {
        FString Folder=FPaths::ProjectDir()/TEXT("Evidence/Entrega07Editor"); FParse::Value(FCommandLine::Get(),TEXT("CeroEvidenceDir="),Folder);
        FScreenshotRequest::RequestScreenshot(Folder/(FString(Name)+TEXT(".png")),true,false);
    };
    auto Aim=[Player,PC](FVector Target) { PC->SetControlRotation((Target-Player->Camera->GetComponentLocation()).Rotation()); };
    auto Key=[PC](FKey K,EInputEvent E) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,E,E==IE_Pressed ? 1.f : 0.f)); };
    if(SmokeStage>=45) { ProfessionalSmokeStep(); return; }
    if(bPolishChecksCompleted) { SmokeStage=44; SmokeNextAt=Now+.1f; return; }
    UE_LOG(LogTemp,Display,TEXT("CE_POLISH_STAGE %d"),SmokeStage);
    switch(SmokeStage++) {
    case 34:
        Player->Dialogue->Close(); Player->Release();
        for(TActorIterator<ACECustomer> It(GetWorld());It;++It) It->Destroy();
        SmokeCustomer=GetWorld()->SpawnActor<ACECustomer>(FVector(460,-100,92),FRotator::ZeroRotator);
        SmokeCustomer->Configure(TEXT("qa07-cafe"),TEXT("HIPSTER PURISTA"),FLinearColor(.38f,.22f,.12f),1);
        SmokeCustomer->State=ECECustomerState::Waiting; SmokeCustomer->bOrderKnown=true; SmokeCustomer->RequestedDrink=ECEDrink::Latte;
        SmokeWitness=GetWorld()->SpawnActor<ACECustomer>(FVector(460,220,92),FRotator::ZeroRotator);
        SmokeWitness->Configure(TEXT("qa07-testigo"),TEXT("CRIPTO-BRO"),FLinearColor(.1f,.23f,.24f),0);
        SmokeWitness->State=ECECustomerState::Waiting; SmokeWitness->bOrderKnown=true; SmokeWitness->SeatSlot=1;
        Check(TEXT("articulated_physics_assets_have_bodies_and_joints"),SmokeCustomer->Rig->GetPhysicsAsset() && SmokeCustomer->Rig->GetPhysicsAsset()->SkeletalBodySetups.Num()>=12 && SmokeCustomer->Rig->GetPhysicsAsset()->ConstraintSetup.Num()>=11);
        Check(TEXT("six_diegetic_order_tickets_exist"),OrderTickets.Num()==CECafeLayout::TableCount);
        SmokeCustomer->Speak(TEXT("Mis bolsillos ya no siguen a mis manos."),3.f);
        Player->SetActorLocation(FVector(260,180,92)); Aim(SmokeCustomer->GetActorLocation()+FVector(0,0,15));
        Key(EKeys::Tab,IE_Pressed); SmokeNextAt=Now+.6f; break;
    case 35:
        if(!LoopChecks.Contains(TEXT("Tab_opens_order_list"))) {
            Check(TEXT("Tab_opens_order_list"),Player->bShowOrderList); Key(EKeys::Tab,IE_Released);
            Shot(TEXT("12_caderas_y_manos_corregidas")); SmokeStage=35; SmokeNextAt=Now+.4f; break;
        }
        // Test station dosing without a violent pickup or a teleport with a full cup.
        Player->bPouring=false; Player->SlipUntil=0.f;
        Player->SetActorLocation(FVector(275,-100,92)); Aim(SmokeCustomer->HeadLocation());
        FVector CupAnchor=Player->Camera->GetComponentLocation()+Player->Camera->GetForwardVector()*88.f+Player->Camera->GetRightVector()*24.f-FVector(0,0,30);
        CupAnchor.Z=FMath::Max(CupAnchor.Z,Player->Camera->GetComponentLocation().Z-18.f);
        SmokeCup=GetWorld()->SpawnActor<ACEProp>(CupAnchor,FRotator(0,PC->GetControlRotation().Yaw,0)); SmokeCup->Configure(ECEPropKind::Cup); SmokeCup->AddLiquid(60,95,0,65);
        SmokeCup->Body->SetPhysicsLinearVelocity(FVector::ZeroVector); SmokeCup->Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
        Player->Hold(SmokeCup);
        for(TActorIterator<ACEStation> It(GetWorld());It;++It) if(It->Kind==ECEStationKind::Sugar || It->Kind==ECEStationKind::Syrup || It->Kind==ECEStationKind::Frother) It->Use(Player);
        SmokeNextAt=Now+1.7f; break;
    case 36:
    {
        if(!LoopChecks.Contains(TEXT("sugar_station_adds_two_grams"))) {
        Check(TEXT("Tab_release_returns_to_clear_game_view"),!Player->bShowOrderList);
        UE_LOG(LogTemp,Display,TEXT("CE_DOSING_FIXTURE sugar=%.3f syrup=%.3f foam=%.3f volume=%.3f rotation=%s"),SmokeCup->SugarGrams,SmokeCup->SyrupMl,SmokeCup->FoamMl,SmokeCup->VolumeMl(),*SmokeCup->GetActorRotation().ToString());
        Check(TEXT("sugar_station_adds_two_grams"),SmokeCup->SugarGrams==2.f);
        Check(TEXT("syrup_pump_adds_six_ml_with_capacity"),SmokeCup->SyrupMl>5.9f && SmokeCup->VolumeMl()<=SmokeCup->CapacityMl());
        Check(TEXT("hot_milk_becomes_visible_foam"),SmokeCup->FoamMl>20.f);
        Player->SetActorLocation(FVector(275,-100,92)); Aim(SmokeCustomer->HeadLocation()); Shot(TEXT("13_latte_espuma_y_extras"));
        SmokeStage=36; SmokeNextAt=Now+.4f; break;
        }
        auto* Wrong=GetWorld()->SpawnActor<ACEProp>(FVector(280,-160,115),FRotator::ZeroRotator); Wrong->Configure(ECEPropKind::Cup); Wrong->AddLiquid(60,95,0,65);
        SmokeCustomer->bWantsFoam=true; SmokeCustomer->bWantsSugar=true; SmokeCustomer->bWantsSyrup=true;
        Check(TEXT("customer_rejects_missing_requested_extras"),!SmokeCustomer->Serve(Wrong)); Wrong->Destroy();
        Check(TEXT("customer_accepts_physically_prepared_extras"),SmokeCustomer->Serve(SmokeCup));
        Check(TEXT("successful_breakfast_builds_customer_trust"),SmokeCustomer->Trust>.3f);
        Player->Dialogue->Open(SmokeCustomer); Player->Dialogue->bVoiceEnabled=false;
        const int32 Before=Player->Dialogue->ServiceReplies;
        Player->Dialogue->Submit(TEXT("qué has pedido"));
        Check(TEXT("common_service_reply_uses_validated_authored_json"),Player->Dialogue->ServiceReplies==Before+1 && !Player->Dialogue->bLastReplyFromOllama && Player->Dialogue->LastReply.Contains(TEXT("Latte")));
        Check(TEXT("common_service_reply_does_not_wait_for_model"),!Player->Dialogue->IsBusy() && Player->Dialogue->ResponseSeconds<.1 && !Player->Dialogue->IsEditing());
        Player->Dialogue->Close();
        SmokeNextAt=Now+.4f; break;
    }
    case 37:
        if(!LoopChecks.Contains(TEXT("suspicious_customer_moves_breakfast_away"))) {
        Player->SetActorLocation(SmokeCustomer->GetActorLocation()+FVector(-140,0,0));
        SmokeCustomer->bSeated=true; SmokeCustomer->State=ECECustomerState::Waiting; SmokeCustomer->Anger=.4f;
        SmokePitcher=GetWorld()->SpawnActor<ACEProp>(Player->GetActorLocation()+FVector(40,0,30),FRotator::ZeroRotator); SmokePitcher->Configure(ECEPropKind::Pitcher); SmokePitcher->AddLiquid(0,120,0,65); Player->Hold(SmokePitcher); Player->bPouring=true;
        UE_LOG(LogTemp,Display,TEXT("CE_DEFENCE_FIXTURE seated=%d physical=%d leaving=%d now=%.2f fallen_until=%.2f npc=%s player=%s cup_held=%d cup_broken=%d"),SmokeCustomer->bSeated,SmokeCustomer->bPhysicalFall,SmokeCustomer->bLeaving,Now,SmokeCustomer->FallenUntil,*SmokeCustomer->GetActorLocation().ToString(),*Player->GetActorLocation().ToString(),SmokeCup->bHeld,SmokeCup->bBroken);
        SmokeCustomer->TickSocial(.7f); Player->bPouring=false; Player->Release();
        Check(TEXT("suspicious_customer_moves_breakfast_away"),SmokeCustomer->DefensiveMoves>0);
        SmokeCustomer->WitnessCount=1; SmokeCustomer->TickSocial(.1f);
        Check(TEXT("witnesses_exchange_contextual_comments"),SmokeCustomer->SocialExchanges>0 && SmokeWitness->SocialExchanges>0);
        Player->SetActorLocation(FVector(235,-250,92)); Aim(SmokeCustomer->HeadLocation()); Shot(TEXT("14_cliente_protege_desayuno"));
        SmokeStage=37; SmokeNextAt=Now+.4f; break;
        }
        SmokeCustomer->Anger=0.f; SmokeCustomer->bSeated=false; SmokeCustomer->Fall(FVector(0,1,0));
        SmokeWitness->Anger=0.f; SmokeWitness->Fall(FVector(0,-1,0));
        {
            auto* Third=GetWorld()->SpawnActor<ACECustomer>(FVector(580,-330,92),FRotator::ZeroRotator);
            Third->Configure(TEXT("qa07-limite"),TEXT("INFLUENCER"),FLinearColor(.5f,.3f,.4f),2); Third->Fall(FVector(1,0,0));
            Check(TEXT("ragdolls_are_limited_to_two_simultaneous_customers"),SmokeCustomer->bPhysicalFall && SmokeWitness->bPhysicalFall && !Third->bPhysicalFall); Third->Destroy();
        }
        LoopPreviousCustomerPosition=SmokeCustomer->Rig->GetSocketLocation(TEXT("pelvis")); SmokeNextAt=Now+.7f; break;
    case 38:
        Check(TEXT("ragdoll_joint_anchors_remain_connected"),SmokeCustomer->IsBodyConsistent() && SmokeWitness->IsBodyConsistent());
        Check(TEXT("physical_fall_disables_standing_capsule_queries"),SmokeCustomer->Collision->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
        Check(TEXT("fall_simulates_real_articulated_customer_body"),SmokeCustomer->bPhysicalFall && SmokeCustomer->Rig->IsSimulatingPhysics(TEXT("pelvis")));
        Check(TEXT("physical_head_collision_keeps_face_above_floor"),SmokeCustomer->HeadLocation().Z>=10.f);
        Check(TEXT("ragdoll_moves_in_response_to_gravity_and_impulse"),FVector::Dist(LoopPreviousCustomerPosition,SmokeCustomer->Rig->GetSocketLocation(TEXT("pelvis")))>5.f);
        Aim(SmokeCustomer->HeadLocation()); Shot(TEXT("15_caida_fisica_real")); SmokeNextAt=Now+2.8f; break;
    case 39:
        Check(TEXT("customer_recovers_from_ragdoll_into_animation"),!SmokeCustomer->bPhysicalFall && !SmokeCustomer->Rig->IsSimulatingPhysics(TEXT("pelvis")));
        SmokeCustomer->PhysicalRecoil(FVector(0,1,0));
        Check(TEXT("small_impact_simulates_head_without_full_ragdoll"),!SmokeCustomer->bPhysicalFall && SmokeCustomer->Rig->IsSimulatingPhysics(TEXT("head")));
        SmokeNextAt=Now+1.1f; break;
    case 40:
    {
        Check(TEXT("head_returns_to_animation_after_short_recoil"),!SmokeCustomer->Rig->IsSimulatingPhysics(TEXT("head")));
        const int32 Prior=Cafe->BusinessEvents; Cafe->ViralPoints=999; Cafe->NextBusinessAt=Now;
        Cafe->TickBusiness(); Check(TEXT("ordinary_service_does_not_start_marketing_event"),Cafe->BusinessEvents==Prior);
        Cafe->ViralPoints=1000; Cafe->TickBusiness();
        Check(TEXT("viral_score_starts_critic_event"),Cafe->SpecialEvent==1 && Cafe->BusinessEvents==Prior+1);
        auto* Visitor=GetWorld()->SpawnActor<ACECustomer>(FVector(620,450,92),FRotator::ZeroRotator);
        Visitor->Configure(TEXT("qa07-critico"),TEXT("CRÍTICO"),FLinearColor(.3f,.3f,.3f),1); Visitor->BeginVisit(2,ECEDrink::Latte,false);
        Check(TEXT("new_arrival_receives_active_business_role"),Visitor->VisitRole==TEXT("CRÍTICO EN INCÓGNITO")); Visitor->Destroy();
        Cafe->TickBusiness(); Check(TEXT("marketing_events_do_not_repeat_on_every_tick"),Cafe->BusinessEvents==Prior+1);
        Cafe->TickBusiness(true); Check(TEXT("aggressive_experience_is_distinct_event"),Cafe->SpecialEvent==2);
        Cafe->TickBusiness(true); Check(TEXT("corporate_tasting_is_distinct_event"),Cafe->SpecialEvent==3);
        Player->SetActorLocation(FVector(130,350,92)); Aim(SmokeCustomer->HeadLocation()); Shot(TEXT("16_eventos_y_comandas"));
        SmokeNextAt=Now+.5f; break;
    }
    case 41:
    {
        Check(TEXT("flat_order_tickets_do_not_cover_customer_faces"),OrderTickets.Num()==CECafeLayout::TableCount && OrderTickets[0]->GetComponentLocation().Z<113.f && OrderTickets[0]->GetUpVector().Z<.1f);
        auto* Asset=SmokeCustomer->Rig->GetPhysicsAsset(); bool Knees=true; int32 Hinges=0;
        for(const auto& Joint:Asset->ConstraintSetup) if(Joint->DefaultInstance.ConstraintBone1.ToString().StartsWith(TEXT("shin"))) { ++Hinges; Knees &= Joint->DefaultInstance.GetAngularSwing1Limit()<=6.f && Joint->DefaultInstance.GetAngularTwistLimit()>50.f; }
        Check(TEXT("knees_use_anatomical_hinge_limits"),Knees && Hinges==2);
        Check(TEXT("non_adjacent_ragdoll_parts_have_self_collision"),Asset->CollisionDisableTable.Num()<30);
        Player->SetActorLocation(TampStation->GetActorLocation()+FVector(0,70,-25));
        auto* TestFilter=GetWorld()->SpawnActor<ACEProp>(TampStation->GetActorLocation()+FVector(0,0,10),FRotator::ZeroRotator); TestFilter->Configure(ECEPropKind::Portafilter); TestFilter->bGroundDose=true; Player->Hold(TestFilter);
        Player->bInteractHeld=false; TampStation->Use(Player); TampStation->Tick(.1f);
        Check(TEXT("releasing_tamper_early_leaves_dose_uncompressed"),!TestFilter->bTamped && TampStation->WorkLeft==0 && TestFilter->Body->IsSimulatingPhysics());
        Player->Hold(TestFilter); Player->bInteractHeld=true; TampStation->Use(Player); TampStation->Tick(.2f);
        Check(TEXT("holding_tamper_builds_measurable_pressure"),TampStation->TampPressure>.3f && !TestFilter->bTamped);
        TampStation->Tick(.4f); Player->bInteractHeld=false;
        Check(TEXT("complete_manual_tamp_returns_filter_to_hand"),TestFilter->bTamped && Player->Held==TestFilter);
        Player->Release(); TestFilter->Destroy();
        EspressoMachine->bDoseReady=true; EspressoMachine->Use(Player); EspressoMachine->Tick(.5f); EspressoMachine->Use(Player);
        Check(TEXT("F_stops_extraction_before_full_recipe_volume"),EspressoMachine->WorkLeft==0 && !EspressoMachine->bDoseReady && EspressoMachine->ExtractedMl>0 && EspressoMachine->ExtractedMl<60);
        const int32 Helped=Cafe->StoriesHelped,Ruined=Cafe->StoriesRuined;
        bool AllStories=true;
        for(int32 Story=0;Story<5;++Story) {
            auto* Guest=GetWorld()->SpawnActor<ACECustomer>(FVector(600,350,92),FRotator::ZeroRotator); Guest->Configure(TEXT("qa08-story"),TEXT("VISITA"),FLinearColor(.4f,.3f,.2f),Story%3,Story);
            Guest->InitStory(Story); Guest->AdvanceStory(); Guest->StoryStageStartedAt=Now-5.f; Guest->State=ECECustomerState::Eating; Guest->StoryConversation(TEXT("Ánimo, suerte: tu poema y tu crítica pueden salir bien.")); Guest->StoryStageStartedAt=Now-9.f; Guest->TickStory(.1f);
            AllStories &= Guest->bStoryResolved && Guest->bStoryHelped && !Guest->StoryOutcome.IsEmpty(); Guest->ResolveStory(false); Guest->Destroy();
            Guest=GetWorld()->SpawnActor<ACECustomer>(FVector(600,350,92),FRotator::ZeroRotator); Guest->Configure(TEXT("qa08-sabotage"),TEXT("VISITA"),FLinearColor(.4f,.3f,.2f),Story%3,Story);
            Guest->InitStory(Story); Guest->React(TEXT("coffee_spill"),TEXT("Camisa"),.4f); AllStories &= Guest->bStoryResolved && Guest->WipeUntil>Now; Guest->Destroy();
        }
        Check(TEXT("five_stories_have_help_and_sabotage_outcomes"),AllStories && Cafe->StoriesHelped==Helped+5 && Cafe->StoriesRuined==Ruined+5);
        const int32 Praise=Cafe->PraiseCount; Cafe->TickRadio(); Check(TEXT("story_outcomes_do_not_spam_the_boss"),Cafe->PraiseCount==Praise);
        Player->SetActorLocation(FVector(-135,-100,92)); Aim(FVector(165,-100,145)); Shot(TEXT("17_mostrador_despejado")); SmokeNextAt=Now+.5f; break;
    }
    case 42:
        for(TActorIterator<ACECustomer> It(GetWorld());It;++It) It->Destroy();
        for(int32 Look=0;Look<9;++Look) {
            auto* Guest=GetWorld()->SpawnActor<ACECustomer>(FVector(220+Look%3*180.f,-390+Look/3*250.f,92),FRotator::ZeroRotator);
            Guest->Configure(FString::Printf(TEXT("qa08-look%d"),Look),TEXT("VISITA"),FLinearColor(.2f,.3f,.3f),Look%3,Look); Guest->InitStory(); Guest->State=ECECustomerState::Waiting; Guest->SeatSlot=-1;
            if(Look==3) SmokeCustomer=Guest;
        }
        Player->SetActorLocation(FVector(70,520,92)); Aim(FVector(450,-130,125)); SmokeNextAt=Now+.5f; break;
    case 43:
    {
        if(!LoopChecks.Contains(TEXT("nine_distinct_guest_profiles_are_visible"))) {
        TSet<FString> Names; for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(It->LookIndex>=0) Names.Add(It->DisplayName);
        Check(TEXT("nine_distinct_guest_profiles_are_visible"),Names.Num()==9); Shot(TEXT("18_clientela_extravagante"));
        Player->Dialogue->Open(SmokeCustomer); Player->Dialogue->bVoiceEnabled=false; Player->Dialogue->Submit(TEXT("cuéntame tu historia"));
        Check(TEXT("story_question_uses_actual_guest_situation"),Player->Dialogue->LastReply==SmokeCustomer->StoryAnswer().Left(100));
        Player->Dialogue->Submit(TEXT("buena suerte")); Check(TEXT("conversation_encouragement_progresses_story"),SmokeCustomer->bStoryHelped);
        Player->Dialogue->Close(); SmokeStage=43; SmokeNextAt=Now+.4f; break;
        }
        if(!IsValid(SmokeWitness) || SmokeWitness->CustomerId!=TEXT("qa08-contact-target")) {
            for(TActorIterator<ACECustomer> It(GetWorld());It;++It) It->Destroy();
            SmokeCustomer=GetWorld()->SpawnActor<ACECustomer>(FVector(590,-50,92),FRotator::ZeroRotator); SmokeCustomer->Configure(TEXT("qa08-contact-source"),TEXT("MOTORISTA"),FLinearColor(.1f,.1f,.1f),0); SmokeCustomer->State=ECECustomerState::Waiting;
            SmokeWitness=GetWorld()->SpawnActor<ACECustomer>(FVector(590,18,92),FRotator::ZeroRotator); SmokeWitness->Configure(TEXT("qa08-contact-target"),TEXT("TESTIGO"),FLinearColor(.2f,.4f,.2f),1); SmokeWitness->State=ECECustomerState::Waiting;
            LoopFirstBossPhrase=Cafe->ChainReactions; SmokeCustomer->Fall(FVector(0,1,0));
            Player->SetActorLocation(FVector(380,-220,92)); Aim(SmokeCustomer->GetActorLocation()); SmokeStage=43; SmokeNextAt=Now+1.1f; break;
        }
        Check(TEXT("actual_ragdoll_contact_can_knock_over_another_guest"),SmokeWitness->PhysicalFalls>0 && Cafe->ChainReactions>LoopFirstBossPhrase);
        Check(TEXT("contact_chain_preserves_connected_bodies"),SmokeCustomer->IsBodyConsistent() && SmokeWitness->IsBodyConsistent());
        Shot(TEXT("19_contacto_y_caida_en_cadena"));
        SmokeStage=45; SmokeNextAt=Now+.4f; break;
    }
    default:
        bPolishChecksCompleted=true;
        SmokeStage=FParse::Param(FCommandLine::Get(),TEXT("CeroQuickSmoke")) ? 44 : 31;
        SmokeNextAt=SmokeStage==31 ? FMath::Max(Now+1.f,241.f) : Now+.3f; break;
    }
}
