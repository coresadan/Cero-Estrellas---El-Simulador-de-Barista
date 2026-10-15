#include "CEGameplay.h"
#include "CEKitchen.h"
#include "CEDialogue.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "Components/AudioComponent.h"
#include "Camera/CameraComponent.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/FileHelper.h"
#include "Misc/Paths.h"
#include "Misc/Parse.h"
#include "Misc/CommandLine.h"
#include "Serialization/JsonSerializer.h"
#include "UnrealClient.h"

void ACEGameMode::ProfessionalSmokeStep()
{
    auto* Player=Cast<ACEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* PC=UGameplayStatics::GetPlayerController(this,0);
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>(); const float Now=GetWorld()->GetTimeSeconds();
    auto Check=[this](const TCHAR* Name,bool Pass) { LoopChecks.Add(Name,Pass); UE_LOG(LogTemp,Display,TEXT("CE09_CHECK %s=%d"),Name,Pass); };
    FString Folder=FPaths::ProjectDir()/TEXT("Evidence/Entrega09Editor"); FParse::Value(FCommandLine::Get(),TEXT("CeroEvidenceDir="),Folder);
    auto Shot=[&Folder](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(Folder/(FString(Name)+TEXT(".png")),true,false); };
    auto Aim=[&](FVector P) { PC->SetControlRotation((P-Player->Camera->GetComponentLocation()).Rotation()); };
    UE_LOG(LogTemp,Display,TEXT("CE09_STAGE %d"),SmokeStage);
    switch(SmokeStage++) {
    case 45: {
        Player->Release(); Player->Dialogue->Close(); Player->bPouring=false;
        for(TActorIterator<ACECustomer> It(GetWorld());It;++It) It->Destroy();
        NextArrivalAt=0; TickArrivals(); // Test actual arrival placement, before any sabotage.
        auto Report=MakeShared<FJsonObject>(); TArray<TSharedPtr<FJsonValue>> Runs;
        int32 Queries[2]={}; bool Conserved=true;
        for(int32 Run=0;Run<2;++Run) {
            auto* Liquid=GetWorld()->SpawnActor<ACELiquidSystem>(); Liquid->SetActorTickEnabled(false);
            for(int32 i=0;i<32;++i) Liquid->Emit(FVector(-490+(i%8)*3.f,200+(i/8)*3.f,180),FVector(0,0,-30),2,0,0,70);
            const int32 Hz=Run==0 ? 60 : 30; for(int32 Frame=0;Frame<Hz;++Frame) Liquid->Simulate(1.f/Hz);
            Queries[Run]=Liquid->CollisionQueries; Conserved &= FMath::IsNearlyEqual(Liquid->FloorMl,64.f,.01f) && Liquid->InFlightMl()<.01f && Liquid->UnresolvedMl<.01f;
            auto Row=MakeShared<FJsonObject>(); Row->SetNumberField(TEXT("hz"),Hz); Row->SetNumberField(TEXT("queries"),Liquid->CollisionQueries);
            Row->SetNumberField(TEXT("simulation_cpu_ms"),Liquid->SimulationCpuMs); Row->SetNumberField(TEXT("floor_ml"),Liquid->FloorMl);
            Runs.Add(MakeShared<FJsonValueObject>(Row)); Liquid->Destroy();
        }
        Check(TEXT("30hz_liquid_conserves_same_known_volume_as_60hz"),Conserved);
        Check(TEXT("30hz_liquid_reduces_collision_queries_by_at_least_35_percent"),Queries[1]>0 && Queries[1]<Queries[0]*.65f);
        Report->SetArrayField(TEXT("runs"),Runs); Report->SetBoolField(TEXT("conserved"),Conserved);
        Report->SetStringField(TEXT("scope"),TEXT("Same new solver and scene at 60/30 Hz; not a complete old-build benchmark."));
        FString Json; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Json)); FFileHelper::SaveStringToFile(Json,*(Folder/TEXT("liquid_comparison.json")));
        Check(TEXT("continuous_stream_rendering_uses_bounded_pool"),Cafe->Liquids->Streams->GetInstanceCount()==64 && Cafe->Liquids->Drops->GetInstanceCount()==192);
        bool Assets=true; for(const auto* Name:{TEXT("SM09_EspressoShell"),TEXT("SM09_TableTop"),TEXT("SM09_Pendant"),TEXT("SM09_Collar"),TEXT("SM09_GaugeRim"),TEXT("SM09_Knob")}) Assets &= StaticLoadObject(UObject::StaticClass(),nullptr,*FString::Printf(TEXT("/Game/Prototype/Meshes/%s.%s"),Name,Name))!=nullptr;
        Assets &= StaticLoadObject(UObject::StaticClass(),nullptr,TEXT("/Game/Prototype/M09_Foam.M09_Foam"))!=nullptr;
        Check(TEXT("new_hero_geometry_is_loaded_in_playable_world"),Assets && EspressoMachine->Body->GetStaticMesh()->GetName()==TEXT("SM09_EspressoShell"));
        int32 Tables=0,Missing=0; bool TableBounds=true;
        for(TActorIterator<AActor> It(GetWorld());It;++It) {
            TArray<UStaticMeshComponent*> Parts; It->GetComponents(Parts);
            for(auto* Part:Parts) {
                if(!Part->GetStaticMesh()) { ++Missing; continue; }
                if(Part->GetStaticMesh()->GetName()==TEXT("SM09_TableTop")) {
                    ++Tables; const FVector Span=Part->Bounds.BoxExtent*2;
                    TableBounds &= Span.X>120.f && Span.Y>110.f && Span.Z>7.f && Span.Z<10.f && Part->GetMaterial(0)!=nullptr;
                }
            }
        }
        Check(TEXT("all_three_tabletops_are_visible_meshes_with_full_size_and_material"),Tables==3 && TableBounds);
        Check(TEXT("every_world_static_mesh_component_has_a_loaded_mesh"),Missing==0);
        // Validate the actual new arrival's placement. Earlier stages deliberately
        // knock laptops around without necessarily marking them as thrown.
        const FString ArrivalId=FString::Printf(TEXT("cliente-%03d"),CustomerSerial);
        bool Screens=true; int32 Laptops=0;
        for(TActorIterator<ACEProp> It(GetWorld());It;++It) if(It->Kind==ECEPropKind::Laptop && It->OwnerCustomerId==ArrivalId && !It->bBroken && !It->WasThrown()) { ++Laptops; Screens &= FVector::DotProduct(-It->GetActorForwardVector(),FVector::ForwardVector)>.8f; }
        Check(TEXT("laptop_screens_face_seated_customers"),Screens && Laptops==1);
        EspressoMachine->WorkLeft=0; EspressoMachine->ExtractionTargetMl=60; EspressoMachine->AdjustSetting(-1);
        Check(TEXT("espresso_selector_supports_a_real_short_recipe"),EspressoMachine->ExtractionTargetMl==30);
        EspressoMachine->AdjustSetting(1); EspressoMachine->AdjustSetting(1); EspressoMachine->AdjustSetting(1);
        Check(TEXT("espresso_selector_clamps_to_ninety_ml"),EspressoMachine->ExtractionTargetMl==90);
        EspressoMachine->WorkLeft=1; EspressoMachine->AdjustSetting(-1); Check(TEXT("busy_machine_rejects_setting_changes"),EspressoMachine->ExtractionTargetMl==90);
        EspressoMachine->Tick(.3f); Check(TEXT("extraction_drives_visible_pressure_gauge"),EspressoMachine->PressureBars>3.f && EspressoMachine->GaugeNeedle);
        EspressoMachine->WorkLeft=0; EspressoMachine->WorkingSound->Stop(); EspressoMachine->ExtractionTargetMl=60;
        MilkStation->MilkTargetDegrees=65; MilkStation->WorkLeft=0; MilkStation->AdjustSetting(1); Check(TEXT("steam_target_is_adjustable_and_bounded"),MilkStation->MilkTargetDegrees==75); MilkStation->MilkTargetDegrees=65;
        Player->SetActorLocation(FVector(-160,-130,92)); Aim(EspressoMachine->GetActorLocation()+FVector(0,30,10)); Shot(TEXT("20_cafetera_y_materiales"));
        SmokeNextAt=Now+.5f; break;
    }
    case 46: {
        for(TActorIterator<ACEProp> It(GetWorld());It;++It) if(It->Kind==ECEPropKind::Chair && FVector::Dist2D(It->GetActorLocation(),FVector(450,-100,52))<90.f) It->Destroy();
        SmokeCustomer=GetWorld()->SpawnActor<ACECustomer>(FVector(440,-100,92),FRotator::ZeroRotator); SmokeCustomer->Configure(TEXT("qa09-story"),TEXT("VISITA"),FLinearColor(.3f,.2f,.1f),0,3);
        auto* PoseSeat=GetWorld()->SpawnActor<ACEProp>(FVector(450,-100,52),FRotator::ZeroRotator); PoseSeat->Configure(ECEPropKind::Chair); PoseSeat->Body->SetSimulatePhysics(false); SmokeCustomer->Seat=PoseSeat;
        SmokeCustomer->InitStory(0); SmokeCustomer->State=ECECustomerState::Waiting; SmokeCustomer->bOrderKnown=true; SmokeCustomer->bSeated=true;
        SmokeCustomer->bStoryHelped=true; SmokeCustomer->TickStory(.1f); Check(TEXT("story_requires_stages_and_breakfast_before_success"),!SmokeCustomer->bStoryResolved && SmokeCustomer->StoryStage==0);
        SmokeCustomer->StoryStartedAt=Now-16; SmokeCustomer->TickStory(.1f);
        Check(TEXT("interview_has_a_timed_phone_call_stage"),SmokeCustomer->StoryStage==1 && SmokeCustomer->PhoneCallUntil>Now);
        SmokeCustomer->State=ECECustomerState::Eating; SmokeCustomer->StoryStageStartedAt=Now-5.f; SmokeCustomer->TickStory(.1f); Check(TEXT("breakfast_and_support_advance_story_without_skipping_ending"),SmokeCustomer->StoryStage==2 && !SmokeCustomer->bStoryResolved);
        SmokeCustomer->StoryStageStartedAt=Now-9; SmokeCustomer->TickStory(.1f); Check(TEXT("multi_stage_story_resolves_once"),SmokeCustomer->bStoryResolved && SmokeCustomer->StoryStage==3);
        SmokeWitness=GetWorld()->SpawnActor<ACECustomer>(FVector(460,200,92),FRotator::ZeroRotator); SmokeWitness->Configure(TEXT("qa09-partner"),TEXT("VISITA"),FLinearColor(.2f,.3f,.4f),1,7); SmokeWitness->State=ECECustomerState::Waiting;
        SmokeCustomer->PairWith(SmokeWitness); const int32 Encounters=Cafe->StoryEncounters; SmokeCustomer->PairWith(SmokeWitness);
        Check(TEXT("paired_guests_keep_reciprocal_identity_and_single_encounter"),SmokeCustomer->SocialPartnerId==SmokeWitness->CustomerId && SmokeWitness->SocialPartnerId==SmokeCustomer->CustomerId && Cafe->StoryEncounters==Encounters);
        Check(TEXT("guest_height_and_body_shapes_are_visibly_varied"),SmokeCustomer->BodyScale!=SmokeWitness->BodyScale);
        SmokeWitness->InitStory(4); SmokeWitness->StoryConversation(TEXT("Verso: Tu ausencia sabe a café.")); Check(TEXT("player_can_author_the_poem_draft"),SmokeWitness->StoryDraft==TEXT("Tu ausencia sabe a café."));
        SmokeCustomer->State=ECECustomerState::Waiting; SmokeCustomer->PhoneCallUntil=0; SmokeCustomer->DialogueHoldUntil=0; SmokeWitness->DialogueHoldUntil=0;
        SmokeCustomer->TickSocial(.1f); Check(TEXT("peaceful_customers_converse_without_an_incident"),SmokeCustomer->CasualExchanges>0);
        Player->SetActorLocation(FVector(245,-100,92)); Aim(SmokeCustomer->GetActorLocation()+FVector(0,0,43));
        Player->Dialogue->Open(SmokeCustomer); Player->Dialogue->bVoiceEnabled=false; Player->Dialogue->Submit(TEXT("te traigo otra taza")); Player->Dialogue->Submit(TEXT("qué te prometí"));
        Check(TEXT("immediate_dialogue_remembers_service_promises"),Player->Dialogue->LastReply.Contains(TEXT("te traigo otra taza")) && !Player->Dialogue->IsBusy()); Player->Dialogue->Close();
        Shot(TEXT("21_historia_y_clientes")); SmokeNextAt=Now+.5f; break;
    }
    case 47: {
        SmokeCustomer->PhoneCallUntil=0; SmokeCustomer->SocialGestureUntil=0; SmokeCustomer->AnimateRig(.5f,false);
        UE_LOG(LogTemp,Display,TEXT("CE10_POSE seated=%d falling=%d"),SmokeCustomer->bSeated,SmokeCustomer->bPhysicalFall);
        Check(TEXT("seated_hands_remain_separate_and_arm_lengths_are_preserved"),SmokeCustomer->bSeated && SmokeCustomer->HasNaturalArmPose());
        const int32 Played=SmokeCustomer->RecordedLinesPlayed; SmokeCustomer->Speak(TEXT("¡Mis notas! Las necesitaba para hoy."));
        Check(TEXT("authored_voice_recording_matches_the_exact_caption"),SmokeCustomer->RecordedLinesPlayed==Played+1 && SmokeCustomer->LastRecordedLine==TEXT("¡Mis notas! Las necesitaba para hoy."));
        auto* Notes=GetWorld()->SpawnActor<ACEProp>(FVector(385,-134,84),FRotator::ZeroRotator); Notes->Configure(ECEPropKind::Notebook); Notes->OwnerCustomerId=SmokeCustomer->CustomerId;
        SmokeCustomer->bStoryResolved=false; SmokeCustomer->bStoryHelped=false; SmokeCustomer->StoryStage=1;
        Check(TEXT("returning_owned_notes_supports_the_story_through_physical_interaction"),SmokeCustomer->Serve(Notes) && SmokeCustomer->bStoryHelped);
        Notes->OwnerCustomerId=TEXT("different_customer");
        Check(TEXT("wrong_recipient_resolves_story_as_a_physical_misunderstanding"),!SmokeCustomer->Serve(Notes) && SmokeCustomer->bStoryResolved);
        Notes->Destroy();
        Check(TEXT("impact_splashes_are_fixed_size_and_have_no_collision"),Cafe->Liquids->ImpactSpray->GetInstanceCount()==48 && Cafe->Liquids->ImpactSpray->GetCollisionEnabled()==ECollisionEnabled::NoCollision);
        TSet<FString> Bodies; for(const auto* Name:{TEXT("SK_Crypto"),TEXT("SK_Hipster"),TEXT("SK_Influencer"),TEXT("SK_Punk"),TEXT("SK_Astronaut"),TEXT("SK_Diva"),TEXT("SK_Biker"),TEXT("SK_Granny"),TEXT("SK_Magician")})
            if(LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/Prototype/Characters/%s.%s"),Name,Name))) Bodies.Add(Name);
        Check(TEXT("nine_individual_customer_bodies_are_cooked"),Bodies.Num()==9);
        SmokeCustomer->RequestedDrink=ECEDrink::Latte; SmokeCustomer->bWantsPastry=false;
        SmokeCup=GetWorld()->SpawnActor<ACEProp>(FVector(320,-128,94),FRotator::ZeroRotator); SmokeCup->Configure(ECEPropKind::Cup); SmokeCup->AddLiquid(60,95,0,65);
        Check(TEXT("natural_sip_fixture_accepts_real_recipe"),SmokeCustomer->Serve(SmokeCup));
        SmokeCustomer->Fall(FVector(0,1,0)); SmokeCustomer->AnimateRig(.1f,false);
        Check(TEXT("fall_releases_served_cup_into_physics"),!SmokeCup->bDelivered && SmokeCup->Body->IsSimulatingPhysics());
        const int32 Praise=Cafe->PraiseCount; Cafe->BossCooldownUntil=0; Cafe->BossSubtitleUntil=0;
        Cafe->Observe(TEXT("laptop_destroyed"),SmokeWitness->CustomerId,TEXT("Portátil de la cita"),1.f);
        Check(TEXT("major_incident_creates_a_remembered_boss_campaign"),Cafe->PraiseCount==Praise+1 && Cafe->LastMajorVictim==SmokeWitness->CustomerId && !Cafe->BossCampaign.IsEmpty() && Cafe->CampaignUntil>Now);
        Cafe->Observe(TEXT("laptop_destroyed"),SmokeWitness->CustomerId,TEXT("Portátil"),1.f); Check(TEXT("boss_memory_preserves_radio_cooldown"),Cafe->PraiseCount==Praise+1);
        for(int32 i=0;i<3;++i) Cafe->Observe(TEXT("customer_hit"),FString::Printf(TEXT("qa09-side%d"),i),TEXT("Incidente de prueba"),.8f);
        const int32 BeforeStory=Cafe->PraiseCount; const float Cooldown=Cafe->BossCooldownUntil,SubtitleUntil=Cafe->BossSubtitleUntil;
        Cafe->BossCooldownUntil=0; Cafe->BossSubtitleUntil=0; Cafe->Observe(TEXT("story_stage"),SmokeCustomer->CustomerId,TEXT("Entrevista"),0.f);
        Check(TEXT("story_notification_never_triggers_radio_from_prior_disaster_score"),Cafe->PraiseCount==BeforeStory);
        Cafe->BossCooldownUntil=Cooldown; Cafe->BossSubtitleUntil=SubtitleUntil;
        Check(TEXT("voice_and_character_resources_are_resident"),Cafe->ResidentResourceCount()>=45 && Cafe->SharedMaterials.Num()>=4);
        bool Audio=true; for(const auto* N:{TEXT("CafeStreet"),TEXT("CafeRoom"),TEXT("PhoneRing"),TEXT("OperaWarmup")}) Audio &= Cafe->HasSound(FName(N));
        Check(TEXT("new_spatial_ambience_and_story_sounds_are_available"),Audio);
        SmokeNextAt=Now+.8f; break;
    }
    case 48:
        Check(TEXT("scaled_guest_ragdoll_preserves_connected_joints"),SmokeCustomer->IsBodyConsistent());
        Aim(SmokeCustomer->HeadLocation()); Shot(TEXT("22_caida_y_campana"));
        bPolishChecksCompleted=true; SmokeStage=FParse::Param(FCommandLine::Get(),TEXT("CeroQuickSmoke")) ? 44 : 31;
        SmokeNextAt=SmokeStage==31 ? FMath::Max(Now+1.f,241.f) : Now+.4f; break;
    }
}
