#include "CEGameplay.h"
#include "CEKitchen.h"
#include "CEBossLines.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/InstancedStaticMeshComponent.h"
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

void ACEGameMode::SmokeStep() { LoopSmokeStep(); }
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
        Aim(TampStation->GetActorLocation()); Tap(EKeys::F);
        bSmokeGrab=true; SmokeStage=3; SmokeNextAt=Now+.7f; break;
        }
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
        Check(TEXT("E_grabs_and_throws_physics_chair"),!Player->Held && FVector::Dist2D(SmokeTray->GetActorLocation(),FVector(300,350,52))>65.f);
        Check(TEXT("witness_reacts_to_nearby_disaster"),SmokeWitness->WitnessCount>0);
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
        Player->SetActorLocation(FVector(-140,300,92)); PC->SetControlRotation(FRotator::ZeroRotator);
        Key(EKeys::SpaceBar,IE_Pressed); SmokeNextAt=Now+.2f; break;
    case 24:
        Check(TEXT("space_jumps_in_first_person"),Player->GetCharacterMovement()->IsFalling() && Player->GetVelocity().Z>0.f);
        Key(EKeys::SpaceBar,IE_Released);
        SmokeNextAt=Now+1.f; break;
    default:
    {
        bool bVoices=true; for(int32 i=0;i<CEBoss::Lines().Num();++i)
        {
            auto* Voice=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Prototype/Audio/Boss_%d.Boss_%d"),i,i)); bVoices=bVoices && Voice && Voice->GetDuration()>0.f;
        }
        Check(TEXT("all_18_boss_recordings_available"),bVoices);
        bool Passed=true; auto Report=MakeShared<FJsonObject>();
        for(const auto& Pair:LoopChecks) { Report->SetBoolField(Pair.Key,Pair.Value); Passed=Passed && Pair.Value; }
        Report->SetBoolField(TEXT("passed"),Passed); Report->SetStringField(TEXT("engine"),FEngineVersion::Current().ToString());
        Report->SetStringField(TEXT("version"),TEXT("0.3.0")); Report->SetBoolField(TEXT("human_playthrough"),false);
        Report->SetBoolField(TEXT("microphone_llm_tts_integrated"),false); Report->SetNumberField(TEXT("floor_liquid_ml"),Cafe->Liquids->FloorMl);
        FString Output; FJsonSerializer::Serialize(Report,TJsonWriterFactory<>::Create(&Output)); FFileHelper::SaveStringToFile(Output,*(Evidence/TEXT("smoke_result.json")));
        UE_LOG(LogTemp,Display,TEXT("CE_LOOP_RESULT passed=%d checks=%d"),Passed,LoopChecks.Num());
        bSmoke=false; FPlatformMisc::RequestExitWithStatus(false,Passed ? 0 : 1); break;
    }
    }
}
