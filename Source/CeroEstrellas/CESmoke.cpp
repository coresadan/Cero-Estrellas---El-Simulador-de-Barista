#include "CEGameplay.h"
#include "CEKitchen.h"
#include "CEBossLines.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Engine/Texture2D.h"
#include "Materials/MaterialInterface.h"
#include "GameFramework/PlayerController.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/Paths.h"
#include "Misc/FileHelper.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/EngineVersion.h"
#include "HAL/FileManager.h"
#include "HAL/PlatformMisc.h"
#include "UnrealClient.h"
#include "Dom/JsonObject.h"
#include "Serialization/JsonSerializer.h"
#include "Serialization/JsonWriter.h"
#include "Sound/SoundBase.h"
#include "InputKeyEventArgs.h"
#include "EngineUtils.h"
#include "CEDialogue.h"
#include "Components/SkeletalMeshComponent.h"
#include "Engine/SkeletalMesh.h"

void ACEGameMode::SmokeStep() { if(bDialogueSmoke) DialogueSmokeStep(); else if((SmokeStage>=34 && SmokeStage<=43) || (SmokeStage>=45 && SmokeStage<=48)) PolishSmokeStep(); else LoopSmokeStep(); }
void ACEGameMode::LoopSmokeStep()
{
    auto* PC=UGameplayStatics::GetPlayerController(this,0);
    auto* Player=PC ? Cast<ACEPlayer>(PC->GetPawn()) : nullptr;
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>();
    if(!Player || !Cafe->Liquids) { FPlatformMisc::RequestExitWithStatus(false,1); bSmoke=false; return; }
    const float Now=GetWorld()->GetTimeSeconds();
    auto Aim=[PC,Player](FVector Target) { PC->SetControlRotation((Target-Player->Camera->GetComponentLocation()).Rotation()); };
    auto Key=[PC](FKey Button,EInputEvent Event) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(Button,Event,Event==IE_Pressed ? 1.f : 0.f)); };
    auto Tap=[Key](FKey Button) { Key(Button,IE_Pressed); Key(Button,IE_Released); };
    auto Check=[this](const TCHAR* Name,bool Passed)
    {
        LoopChecks.Add(Name,Passed); UE_LOG(LogTemp,Display,TEXT("CE_LOOP_CHECK %s=%d"),Name,Passed);
    };
    FString Evidence=FPaths::ProjectDir()/TEXT("Evidence/LoopPlayable"); FParse::Value(FCommandLine::Get(),TEXT("CeroEvidenceDir="),Evidence);
    auto Shot=[Evidence](const TCHAR* Name) { IFileManager::Get().MakeDirectory(*Evidence,true); FScreenshotRequest::RequestScreenshot(Evidence/(FString(Name)+TEXT(".png")),true,false); };
    UE_LOG(LogTemp,Display,TEXT("CE_LOOP_STAGE %d time=%.2f"),SmokeStage,Now);
    switch(SmokeStage++)
    {
    case 0:
        Check(TEXT("empty_clean_stock"),SmokeCup->VolumeMl()==0.f);
        Check(TEXT("no_intro_radio"),Cafe->PraiseCount==0 && Cafe->BossSubtitle.IsEmpty());
        LoopPreviousCustomerPosition=SmokeCustomer->GetActorLocation();
        Cafe->Observe(TEXT("coffee_spill"),TEXT(""),TEXT("Small spill"),.1f);
        Player->SetActorLocation(FVector(-42,-195,92)); Aim(Grinder->GetActorLocation());
        Shot(TEXT("01_turno_abierto")); SmokeNextAt=Now+.7f; break;
    case 1:
        for(TActorIterator<ACECafeDoor> It(GetWorld());It;++It) Check(TEXT("arrival_opens_door"),FMath::Abs(It->OpenAngle)>20.f);
        Tap(EKeys::F); SmokeNextAt=Now+.7f; break;
    case 2:
        Check(TEXT("F_dispenses_portafilter"),Player->Held && Player->Held->Kind==ECEPropKind::Portafilter);
        Tap(EKeys::F); SmokeNextAt=Now+2.1f; break;
    case 3:
        if(!bSmokeGrab)
        {
        Check(TEXT("grinder_doses_coffee"),Player->Held && Player->Held->bGroundDose && Grinder->DosesLeft==11);
        EspressoMachine->Use(Player);
        Check(TEXT("uncompressed_dose_rejected"),Player->Held && !EspressoMachine->bDoseReady);
        Aim(TampStation->GetActorLocation()); Key(EKeys::F,IE_Pressed);
        bSmokeGrab=true; SmokeStage=3; SmokeNextAt=Now+.7f; break;
        }
        Key(EKeys::F,IE_Released);
        Check(TEXT("F_tamps_ground_dose"),Player->Held && Player->Held->bTamped);
        Player->SetActorLocation(FVector(-155,-195,92)); Aim(EspressoMachine->GetActorLocation()+FVector(0,32,12));
        SmokeNextAt=Now+.7f; break;
    case 4:
        Tap(EKeys::F); SmokeNextAt=Now+.7f; break;
    case 5:
        Check(TEXT("F_locks_portafilter"),EspressoMachine->bDoseReady && !Player->Held);
        CupSupply->Use(Player); SmokeCup=Player->Held;
        Check(TEXT("cup_stock_replenishes"),SmokeCup && SmokeCup->VolumeMl()==0.f && CupSupply->Dispensed==1);
        Tap(EKeys::F); SmokeNextAt=Now+.7f; break;
    case 6:
        Check(TEXT("F_places_cup"),!Player->Held && EspressoMachine->CupUnderSpout()==SmokeCup);
        Tap(EKeys::F); SmokeNextAt=Now+.8f; break;
    case 7:
        if(EspressoMachine->WorkLeft>0.f) { Shot(TEXT("02_maquina_completa")); SmokeStage=7; SmokeNextAt=Now+4.f; break; }
        Check(TEXT("brew_fills_hot_espresso"),SmokeCup->EspressoMl>=50.f && SmokeCup->EspressoMl<=62.f && SmokeCup->Temperature>80.f);
        Check(TEXT("small_incidents_stay_silent"),Cafe->PraiseCount==0);
        Check(TEXT("customer_arrives_by_walking"),FVector::Dist(SmokeCustomer->GetActorLocation(),LoopPreviousCustomerPosition)>250.f);
        if(SmokeCustomer->State!=ECECustomerState::Ordering && Now<30.f) { SmokeStage=7; SmokeNextAt=Now+.5f; break; }
        Player->SetActorLocation(FVector(-15,-100,92)); Aim(SmokeCustomer->GetActorLocation()+FVector(0,0,50));
        SmokeNextAt=Now+.6f; break;
    case 8:
        Tap(EKeys::F); SmokeNextAt=Now+.6f; break;
    case 9:
        Check(TEXT("F_takes_breakfast_order"),SmokeCustomer->bOrderKnown && Cafe->OrdersTaken==1);
        CupSupply->Use(Player); SmokeTray=Player->Held; // Keep second cup until its milk cycle finishes.
        SmokeTray->AddLiquid(60,0,0,93); Player->Release(); SmokeTray->Body->SetSimulatePhysics(false);
        SmokeTray->SetActorLocationAndRotation(FVector(-110,450,13),FRotator::ZeroRotator);
        Player->SetActorLocation(FVector(-265,-195,92)); Aim(MilkStation->GetActorLocation());
        MilkStation->Use(Player); SmokePitcher=Player->Held;
        Check(TEXT("milk_stock_is_cold_physical_pitcher"),SmokePitcher && SmokePitcher->Kind==ECEPropKind::Pitcher && SmokePitcher->MilkMl==120.f && SmokePitcher->Temperature<15.f);
        MilkStation->Use(Player); MilkStation->Use(Player); SmokeNextAt=Now+3.5f; break;
    case 10:
        if(!LoopChecks.Contains(TEXT("steam_heats_existing_milk_without_adding_volume")))
        {
            Check(TEXT("steam_heats_existing_milk_without_adding_volume"),SmokePitcher->Temperature>=60.f && SmokePitcher->MilkMl==120.f);
            Player->SetActorLocation(FVector(-210,450,92));
            SmokePitcher->SetActorLocationAndRotation(FVector(-120,450,110),FRotator::ZeroRotator,false,nullptr,ETeleportType::TeleportPhysics);
            Player->Hold(SmokePitcher); Aim(SmokeTray->GetActorLocation()+FVector(0,0,10));
            SmokeStage=10; SmokeNextAt=Now+.6f; break;
        }
        if(!bSmokeSpill)
        { Key(EKeys::RightMouseButton,IE_Pressed); bSmokeSpill=true; SmokeStage=10; SmokeNextAt=Now+.4f; break; }
        if(SmokePitcher->VolumeMl()>1.f && Now<50.f) { Shot(TEXT("02b_jarra_verter_a_mano")); SmokeStage=10; SmokeNextAt=Now+.7f; break; }
        Key(EKeys::RightMouseButton,IE_Released); Player->Release();
        UE_LOG(LogTemp,Display,TEXT("CE_MILK_TRANSFER cup_milk=%.2f cup_total=%.2f cup_temp=%.2f jar=%.2f"),SmokeTray->MilkMl,SmokeTray->VolumeMl(),SmokeTray->Temperature,SmokePitcher->VolumeMl());
        Check(TEXT("manual_pitcher_pour_builds_hot_latte"),SmokeTray->MilkMl>=90.f && SmokeTray->VolumeMl()<=180.01f && SmokeTray->Temperature>65.f);
        if(SmokeCustomer->State!=ECECustomerState::Waiting && Now<50.f) { SmokeStage=10; SmokeNextAt=Now+.5f; break; }
        Player->SetActorLocation(FVector(235,-100,92)); Aim(SmokeCustomer->GetActorLocation()+FVector(0,0,45));
        Player->Hold(SmokeCup); SmokeNextAt=Now+.7f; break;
    case 11:
        Tap(EKeys::F); Shot(TEXT("03_pedido_y_clientes")); SmokeNextAt=Now+.7f; break;
    case 12:
    {
        Check(TEXT("F_serves_correct_order"),Cafe->OrdersServed==1 && SmokeCustomer->bDrinkServed);
        auto* TestCustomer=GetWorld()->SpawnActor<ACECustomer>(FVector(460,220,92),FRotator::ZeroRotator);
        TestCustomer->Configure(TEXT("qa-desayuno"),TEXT("PRUEBA DESAYUNO"),FLinearColor(.4f,.2f,.1f),1);
        TestCustomer->bOrderKnown=true; TestCustomer->State=ECECustomerState::Waiting; TestCustomer->RequestedDrink=ECEDrink::Latte; TestCustomer->bWantsPastry=true; TestCustomer->SeatSlot=1;
        auto* Wrong=GetWorld()->SpawnActor<ACEProp>(FVector(0,400,100),FRotator::ZeroRotator); Wrong->Configure(ECEPropKind::Cup);
        Check(TEXT("wrong_recipe_rejected"),!TestCustomer->Serve(Wrong) && Cafe->WrongOrders==1);
        Check(TEXT("latte_accepted"),TestCustomer->Serve(SmokeTray));
        Check(TEXT("breakfast_requires_pastry"),Cafe->OrdersServed==1);
        auto* Food=GetWorld()->SpawnActor<ACEProp>(FVector(0,400,100),FRotator::ZeroRotator); Food->Configure(ECEPropKind::Pastry);
        Check(TEXT("pastry_completes_breakfast"),TestCustomer->Serve(Food) && Cafe->OrdersServed==2);
        TestCustomer->Destroy(); Wrong->Destroy();
        CupSupply->Use(Player); Player->Held->AddLiquid(60,0,0,93); WaterStation->Use(Player); WaterStation->Use(Player);
        auto* WetCustomer=GetWorld()->SpawnActor<ACECustomer>(FVector(0,500,92),FRotator::ZeroRotator);
        WetCustomer->Configure(TEXT("qa-wet"),TEXT("CLIENTE"),FLinearColor(.45f,.2f,.3f),2); WetCustomer->State=ECECustomerState::Waiting;
        Cafe->Liquids->Emit(FVector(0,500,225),FVector(0,0,-40),30,0,0,90);
        auto* BreakCup=GetWorld()->SpawnActor<ACEProp>(FVector(0,-450,220),FRotator::ZeroRotator); BreakCup->Configure(ECEPropKind::Cup);
        BreakCup->MarkThrown(); BreakCup->Body->SetPhysicsLinearVelocity(FVector(0,0,-900));
        Player->SetActorLocation(FVector(-210,300,92)); PC->SetControlRotation(FRotator(-28,0,0));
        CupSupply->Use(Player); SmokeCup=Player->Held; SmokeCup->AddLiquid(180,0,0,93);
        SmokeNextAt=Now+.7f; break;
    }
    case 13:
        Key(EKeys::RightMouseButton,IE_Pressed); SmokeNextAt=Now+.6f; break;
    case 14:
        if(SmokeCup->VolumeMl()>1.f) { Shot(TEXT("04_liquido_y_charcos")); SmokeStage=14; SmokeNextAt=Now+1.8f; break; }
        Check(TEXT("hot_water_builds_americano"),WaterStation->WorkingCup && WaterStation->WorkingCup->WaterMl>=90.f && WaterStation->WorkingCup->EspressoMl>=59.f);
        Check(TEXT("liquid_physically_soaks_customer"),Cafe->Liquids->SoakedMl>=29.f);
        for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(It->CustomerId==TEXT("qa-wet")) It->Destroy();
        {
            int32 Shards=0; for(TActorIterator<AActor> It(GetWorld());It;++It) if(It->ActorHasTag(TEXT("CeramicShard"))) ++Shards;
            Check(TEXT("thrown_cup_breaks_into_physics_shards"),Shards>0);
        }
        Check(TEXT("right_click_pours_continuously"),SmokeCup->VolumeMl()<1.f && Cafe->Liquids->FloorMl>60.f);
        Check(TEXT("ordinary_service_and_spills_quiet"),Cafe->PraiseCount==0);
        Key(EKeys::RightMouseButton,IE_Released); Player->Release();
        LoopPreviousCustomerPosition=Cafe->Liquids->LargestFloorPuddle()-FVector(80,0,0); LoopPreviousCustomerPosition.Z=92.f;
        Player->SetActorLocation(LoopPreviousCustomerPosition); PC->SetControlRotation(FRotator(0,0,0));
        Key(EKeys::LeftShift,IE_Pressed); Key(EKeys::W,IE_Pressed);
        SmokeTray=GetWorld()->SpawnActor<ACEProp>(FVector(-110,450,13),FRotator::ZeroRotator); SmokeTray->Configure(ECEPropKind::Cup); SmokeTray->Body->SetSimulatePhysics(false);
        Cafe->Liquids->Emit(FVector(-110,450,100),FVector(0,0,-40),30,0,0,90);
        SmokeNextAt=Now+.8f; break;
    case 15:
        if(!LoopChecks.Contains(TEXT("running_over_puddle_slips")))
        {
            Check(TEXT("running_over_puddle_slips"),Cafe->Liquids->SlipCount>0);
            Check(TEXT("keyboard_movement_and_sprint"),Player->GetActorLocation().X-LoopPreviousCustomerPosition.X>50.f && Player->GetCharacterMovement()->MaxWalkSpeed>400.f);
            Key(EKeys::W,IE_Released); Key(EKeys::LeftShift,IE_Released); Player->GetCharacterMovement()->StopMovementImmediately();
            Player->SetActorLocation(FVector(-210,300,92)); PC->SetControlRotation(FRotator(-52,20,0));
            Shot(TEXT("04b_charco_persistente")); SmokeStage=15; SmokeNextAt=Now+.5f; break;
        }
        Check(TEXT("ballistic_liquid_fills_other_cup"),FMath::IsNearlyEqual(SmokeTray->EspressoMl,30.f,.1f));
        Check(TEXT("cup_capacity_conserves_volume"),FMath::IsNearlyEqual(SmokeTray->AddLiquid(300,0,0,90),150.f,.1f) && SmokeTray->VolumeMl()<=180.01f);
        Player->SetActorLocation(FVector(30,350,92));
        SmokeCustomer=GetWorld()->SpawnActor<ACECustomer>(FVector(230,350,92),FRotator::ZeroRotator);
        SmokeCustomer->Configure(TEXT("qa-desastre"),TEXT("CLIENTE"),FLinearColor(.1f,.3f,.3f),0); SmokeCustomer->State=ECECustomerState::Waiting;
        SmokePastry=GetWorld()->SpawnActor<ACEProp>(FVector(120,350,140),FRotator::ZeroRotator); SmokePastry->Configure(ECEPropKind::Pastry); Player->Hold(SmokePastry);
        Aim(SmokeCustomer->GetActorLocation()+FVector(0,0,50)); SmokeNextAt=Now+.7f; break;
    case 16:
        Tap(EKeys::LeftMouseButton); SmokeNextAt=Now+1.3f; break;
    case 17:
        Check(TEXT("mouse_throw_hits_customer"),!Player->Held && SmokeCustomer->HitCount>0 && SmokeCustomer->bLeaving);
        Check(TEXT("thrown_pastry_sticks_to_head"),SmokePastry->bStuck && SmokePastry->GetAttachParentActor()==SmokeCustomer);
        Check(TEXT("radio_waits_for_major_exit"),Cafe->PraiseCount==0);
        Shot(TEXT("05_reaccion_y_gestos")); SmokeNextAt=Now+12.f; break;
    case 18:
        Check(TEXT("confirmed_flight_triggers_boss"),Cafe->DepartedCustomers>=1 && Cafe->PraiseCount==1);
        LoopFirstBossPhrase=Cafe->LastBossPhrase;
        Cafe->Observe(TEXT("laptop_destroyed"),TEXT("qa"),TEXT("Laptop"),1.f);
        Check(TEXT("boss_cooldown_discards_backlog"),Cafe->PraiseCount==1);
        Player->SetActorLocation(FVector(100,450,92)); PC->SetControlRotation(FRotator(-7,-35,0));
        TraySupply->Use(Player); Check(TEXT("tray_stock_replenishes"),Player->Held && Player->Held->Kind==ECEPropKind::Tray && TraySupply->Dispensed==1);
        Shot(TEXT("06_desastre_y_jefa")); Player->Release();
        SmokeNextAt=Cafe->BossCooldownUntil+1.f; break;
    case 19:
        Check(TEXT("silence_after_cooldown_without_incident"),Cafe->PraiseCount==1);
        Cafe->Observe(TEXT("laptop_destroyed"),TEXT("qa-second"),TEXT("Laptop"),1.f);
        Check(TEXT("new_major_event_uses_different_line"),Cafe->PraiseCount==2 && Cafe->LastBossPhrase!=LoopFirstBossPhrase);
        SmokeNextAt=Cafe->BossSubtitleUntil+1.f; break;
    case 20:
    {
        Player->Release();
        SmokeTray=GetWorld()->SpawnActor<ACEProp>(FVector(300,350,52),FRotator::ZeroRotator); SmokeTray->Configure(ECEPropKind::Chair);
        SmokeCustomer=GetWorld()->SpawnActor<ACECustomer>(FVector(265,350,92),FRotator::ZeroRotator);
        SmokeCustomer->Configure(TEXT("qa-silla"),TEXT("CLIENTE SENTADO"),FLinearColor(.2f,.3f,.3f),1);
        SmokeCustomer->State=ECECustomerState::Waiting;
        SmokeWitness=GetWorld()->SpawnActor<ACECustomer>(FVector(390,350,92),FRotator::ZeroRotator);
        SmokeWitness->Configure(TEXT("qa-testigo"),TEXT("TESTIGO"),FLinearColor(.5f,.3f,.4f),2); SmokeWitness->State=ECECustomerState::Waiting;
        Player->SetActorLocation(FVector(350,470,92)); Aim(SmokeTray->GetActorLocation()+FVector(24,0,41));
        SmokeNextAt=Now+1.f; break;
    }
    case 21:
        Check(TEXT("customer_sits_on_physical_chair"),SmokeCustomer->bSeated && SmokeCustomer->Seat==SmokeTray);
        Tap(EKeys::E);
        Tap(EKeys::LeftMouseButton); SmokeNextAt=Now+1.f; break;
    case 22:
    {
        Check(TEXT("removing_chair_makes_customer_fall"),!SmokeCustomer->bSeated && SmokeCustomer->FallenUntil>Now && Cafe->Falls>=1);
        Check(TEXT("fallen_customer_head_remains_above_floor"),SmokeCustomer->HeadLocation().Z>=10.f);
        Check(TEXT("E_grabs_and_throws_physics_chair"),!Player->Held && FVector::Dist2D(SmokeTray->GetActorLocation(),FVector(300,350,52))>65.f);
        Check(TEXT("witness_reacts_to_nearby_disaster"),SmokeWitness->WitnessCount>0);
        Check(TEXT("influencer_records_witnessed_disaster"),SmokeWitness->bFilming && Cafe->FilmedDisasters>0);
        Check(TEXT("puddles_use_irregular_lobes"),Cafe->Liquids->Pools->GetInstanceCount()==160 && Cafe->Liquids->VisiblePuddleLobes()>=5);
        Check(TEXT("continuous_drops_stay_bounded"),Cafe->Liquids->Drops->GetInstanceCount()==192);
        const int32 Before=Cafe->ViralPoints;
        Cafe->Observe(TEXT("ceramic_broken"),TEXT("qa-combo"),TEXT("Test"),.4f);
        const int32 First=Cafe->ViralPoints;
        Cafe->Observe(TEXT("ceramic_broken"),TEXT("qa-combo"),TEXT("Test"),.4f);
        Check(TEXT("repeated_incident_does_not_farm_combo"),First>Before && Cafe->ViralPoints==First);
        Cafe->Observe(TEXT("coffee_spill"),TEXT("qa-combo"),TEXT("Test"),.2f);
        Cafe->Observe(TEXT("laptop_destroyed"),TEXT("qa-combo"),TEXT("Test"),1.f);
        Check(TEXT("different_disasters_chain_viral_combo"),Cafe->Combo>=3 && Cafe->BestCombo>=3);
        Check(TEXT("corporate_challenges_progress"),Cafe->ChallengeStep>=2 && Cafe->ShiftLevel>=1);
        Player->SetActorLocation(FVector(230,500,92)); Aim(SmokeCustomer->GetActorLocation()+FVector(20,0,10));
        Shot(TEXT("07_silla_caida_y_testigo")); SmokeNextAt=Now+4.f; break;
    }
    case 23:
        Check(TEXT("fallen_customer_recovers"),Now>SmokeCustomer->FallenUntil);
        if(!LoopChecks.Contains(TEXT("witness_keeps_filming_during_recovery"))) {
            Check(TEXT("witness_keeps_filming_during_recovery"),SmokeWitness->bFilming);
            Player->SetActorLocation(FVector(210,500,92)); Aim(SmokeWitness->HeadLocation()); Shot(TEXT("07b_influencer_graba"));
            SmokeStage=23; SmokeNextAt=Now+.4f; break;
        }
        Player->SetActorLocation(FVector(-140,300,92)); PC->SetControlRotation(FRotator::ZeroRotator);
        Key(EKeys::SpaceBar,IE_Pressed); SmokeNextAt=Now+.2f; break;
    case 24:
        Check(TEXT("space_jumps_in_first_person"),Player->GetCharacterMovement()->IsFalling() && Player->GetVelocity().Z>0.f);
        Key(EKeys::SpaceBar,IE_Released);
        SmokeNextAt=Now+1.f; break;
    case 25:
    {
        Check(TEXT("first_person_has_two_visible_gripping_hands"),Player->VisibleHands()==2);
        bool Rigs=true; for(const TCHAR* Name:{TEXT("SK_Crypto"),TEXT("SK_Hipster"),TEXT("SK_Influencer")})
            Rigs&=LoadObject<USkeletalMesh>(nullptr,*FString::Printf(TEXT("/Game/Prototype/Characters/%s.%s"),Name,Name))!=nullptr;
        Check(TEXT("three_original_skeletal_customers_are_cooked"),Rigs);
        Check(TEXT("customer_uses_weighted_eighteen_bone_body"),SmokeCustomer->Rig && SmokeCustomer->Rig->GetNumBones()>=18);
        Check(TEXT("steam_uses_bounded_twenty_four_instance_pool"),Cafe->Liquids->Steam->GetInstanceCount()==24);
        Player->SetActorLocation(FVector(130,350,92)); Aim(SmokeCustomer->HeadLocation());
        Player->Dialogue->Open(SmokeCustomer); Player->Dialogue->SetDraft(TEXT("Borrador conservado al seguir trabajando")); Player->Dialogue->ResumePlay();
        Check(TEXT("conversation_can_leave_editor_without_cancelling"),Player->Dialogue->IsOpen() && !Player->Dialogue->IsEditing() && !PC->bShowMouseCursor);
        LoopPreviousCustomerPosition=Player->GetActorLocation(); Key(EKeys::W,IE_Pressed);
        SmokeNextAt=Now+.5f; break;
    }
    case 26:
        Key(EKeys::W,IE_Released);
        Check(TEXT("player_moves_while_conversation_remains_open"),FVector::Dist2D(Player->GetActorLocation(),LoopPreviousCustomerPosition)>20.f && Player->Dialogue->IsOpen());
        Player->Dialogue->EditConversation();
        Check(TEXT("return_to_dialogue_preserves_draft"),Player->Dialogue->Draft==TEXT("Borrador conservado al seguir trabajando") && Player->Dialogue->IsEditing());
        Player->Dialogue->Close();
        SmokeCustomer->Soak(80); SmokeWitness->Witness(SmokeCustomer);
        Player->SetActorLocation(FVector(185,500,92)); Aim(SmokeCustomer->HeadLocation());
        Shot(TEXT("08_personajes_y_manos")); SmokeNextAt=Now+.6f; break;
    case 27:
        Check(TEXT("wetness_is_stored_on_customer"),SmokeCustomer->WetMl>=80.f);
        // A physics event drives the chain; the dialogue model is not involved.
        SmokeCustomer->FallenUntil=0; SmokeCustomer->Anger=0; SmokeCustomer->bLeaving=false;
        SmokePastry=GetWorld()->SpawnActor<ACEProp>(SmokeCustomer->GetActorLocation()+FVector(50,0,-5),FRotator::ZeroRotator); SmokePastry->Configure(ECEPropKind::Pastry);
        LoopPreviousCustomerPosition=SmokePastry->GetActorLocation();
        SmokeCustomer->Fall(FVector(1,0,0));
        Check(TEXT("physics_does_not_need_AI_to_react"),SmokeCustomer->FallenUntil>Now && !Player->Dialogue->IsBusy());
        Check(TEXT("eight_minute_shift_has_reached_rush_phase"),Cafe->ShiftDuration==480.f && Cafe->ShiftPhase>=1);
        SmokeWitness->WitnessCount=0;
        SmokeNextAt=Now+1.f; break;
    case 28:
        Check(TEXT("fall_launches_loose_objects_into_chain"),Cafe->ChainReactions>0 && SmokeWitness->HitCount>0);
        SmokeWitness->Witness(SmokeCustomer); SmokeNextAt=Now+.4f; break;
    case 29:
        Check(TEXT("secondary_physics_impact_reaches_other_customer"),SmokeWitness->HitCount>0 && SmokeWitness->bLeaving);
        Player->SetActorLocation(FVector(190,500,92)); Aim(SmokeWitness->HeadLocation());
        Shot(TEXT("09_directo_del_desastre")); SmokeNextAt=Now+.8f; break;
    case 30:
        SmokeStage=34; SmokeNextAt=Now+1.f; break;
    case 31:
    {
        Check(TEXT("shift_escalates_into_viral_phase_on_real_clock"),Cafe->ShiftPhase==2);
        auto* Demanding=GetWorld()->SpawnActor<ACECustomer>(FVector(220,455,92),FRotator::ZeroRotator);
        Demanding->Configure(TEXT("qa-exigente"),TEXT("CLIENTE EXIGENTE"),FLinearColor(.2f,.3f,.3f),0);
        Demanding->BeginVisit(0,ECEDrink::Latte,true);
        Check(TEXT("later_customers_have_visible_serving_requirements"),!Demanding->SpecialRequest.IsEmpty() && (Demanding->MinServingTemperature>=60.f || Demanding->MinServingVolume>=170.f));
        Demanding->Destroy(); Player->SetActorLocation(FVector(130,350,92)); PC->SetControlRotation(FRotator(-12,0,0));
        Shot(TEXT("10_turno_viral")); SmokeNextAt=421.f; break;
    }
    case 32:
        Check(TEXT("shift_enters_final_minute_on_real_clock"),Cafe->ShiftPhase==3 && !Cafe->bShiftFinished);
        SmokeNextAt=481.f; break;
    case 33:
        Check(TEXT("full_eight_minute_shift_finishes_without_game_over"),Cafe->bShiftFinished && IsValid(Player) && Cafe->Feedback.Contains(TEXT("Despido denegado")));
        {
            const FString Conclusion=Cafe->Feedback;
            Cafe->Notify(TEXT("HISTORIA / Un cliente acaba de resolver su entrevista."));
            Check(TEXT("shift_conclusion_survives_customer_story_notifications"),Cafe->Feedback==Conclusion && Conclusion.Contains(TEXT("Despido denegado")));
        }
        Shot(TEXT("11_despido_denegado")); SmokeNextAt=Now+1.f; break;
    default:
    {
        bool bVoices=true; for(int32 i=0;i<CEBoss::Lines().Num();++i)
        {
            auto* Voice=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Prototype/Audio/Boss_%d.Boss_%d"),i,i)); bVoices=bVoices && Voice && Voice->GetDuration()>0.f;
        }
        Check(TEXT("all_21_boss_recordings_available"),bVoices);
        bool bTextures=true;
        for(const TCHAR* Set:{TEXT("Wood"),TEXT("Stone"),TEXT("Fabric"),TEXT("Metal"),TEXT("Skin"),TEXT("Paint")})
            for(const TCHAR* Map:{TEXT("A"),TEXT("N"),TEXT("ORM")}) {
                const FString Name=FString(Set)+TEXT("_")+Map;
                auto* Texture=LoadObject<UTexture2D>(nullptr,*FString::Printf(TEXT("/Game/Prototype/Textures/%s.%s"),*Name,*Name));
                bTextures&=Texture && Texture->GetSizeX()==512 && Texture->GetSizeY()==512;
            }
        auto* ScreenTexture=LoadObject<UTexture2D>(nullptr,TEXT("/Game/Prototype/Textures/Screen_A.Screen_A"));
        bTextures&=ScreenTexture && ScreenTexture->GetSizeX()==512 && ScreenTexture->GetSizeY()==512;
        Check(TEXT("all_19_PBR_and_screen_textures_available"),bTextures);
        bool bMaterials=true;
        for(const TCHAR* Name:{TEXT("M_Surface"),TEXT("M_Unlit"),TEXT("M_Liquid"),TEXT("M_Metal"),TEXT("M_Wood"),TEXT("M_Ceramic"),TEXT("M_Fabric"),TEXT("M_Skin"),TEXT("M_Pastry"),TEXT("M_Stone"),TEXT("M_Screen")})
            bMaterials&=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Prototype/%s.%s"),Name,Name))!=nullptr;
        Check(TEXT("all_11_materials_available"),bMaterials);
        bool Passed=true; auto Report=MakeShared<FJsonObject>();
        for(const auto& Pair:LoopChecks) { Report->SetBoolField(Pair.Key,Pair.Value); Passed=Passed && Pair.Value; }
        Report->SetBoolField(TEXT("passed"),Passed); Report->SetStringField(TEXT("engine"),FEngineVersion::Current().ToString());
        Report->SetStringField(TEXT("version"),TEXT("0.10.0")); Report->SetBoolField(TEXT("human_playthrough"),false);
        Report->SetBoolField(TEXT("full_shift_exercised"),!FParse::Param(FCommandLine::Get(),TEXT("CeroQuickSmoke")));
        Report->SetBoolField(TEXT("dialogue_not_exercised_in_this_scenario"),true); Report->SetNumberField(TEXT("floor_liquid_ml"),Cafe->Liquids->FloorMl);
        Report->SetNumberField(TEXT("liquid_collision_queries"),Cafe->Liquids->CollisionQueries); Report->SetNumberField(TEXT("liquid_simulation_steps"),Cafe->Liquids->SimulationSteps);
        Report->SetNumberField(TEXT("liquid_simulation_cpu_ms"),Cafe->Liquids->SimulationCpuMs); Report->SetNumberField(TEXT("liquid_peak_packets"),Cafe->Liquids->PeakActiveDrops);
        FrameSamples.Sort(); if(!FrameSamples.IsEmpty()) {
            Report->SetNumberField(TEXT("frame_samples"),FrameSamples.Num());
            Report->SetNumberField(TEXT("frame_p50_ms"),FrameSamples[FrameSamples.Num()/2]);
            Report->SetNumberField(TEXT("frame_p95_ms"),FrameSamples[FMath::Min(FrameSamples.Num()-1,FMath::FloorToInt(FrameSamples.Num()*.95f))]);
            Report->SetNumberField(TEXT("frame_max_ms"),FrameSamples.Last());
        }
        FString Output; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Output)); FFileHelper::SaveStringToFile(Output,*(Evidence/TEXT("smoke_result.json")));
        UE_LOG(LogTemp,Display,TEXT("CE_LOOP_RESULT passed=%d checks=%d"),Passed,LoopChecks.Num());
        bSmoke=false; FPlatformMisc::RequestExitWithStatus(false,Passed ? 0 : 1); break;
    }
    }
}
