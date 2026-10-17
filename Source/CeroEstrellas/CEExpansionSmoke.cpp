#include "CEGameplay.h"
#include "CECafeLayout.h"
#include "CEKitchen.h"
#include "CEDialogue.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/PlayerController.h"
#include "InputKeyEventArgs.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "Misc/Paths.h"
#include "UnrealClient.h"

void ACEGameMode::ExpansionSmokeStep()
{
    auto* Player=Cast<ACEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
    auto* PC=UGameplayStatics::GetPlayerController(this,0);
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>(); const float Now=GetWorld()->GetTimeSeconds();
    auto Check=[this](const TCHAR* Name,bool Pass) { LoopChecks.Add(Name,Pass); UE_LOG(LogTemp,Display,TEXT("CE12_CHECK %s=%d"),Name,Pass); };
    FString Folder=FPaths::ProjectDir()/TEXT("Evidence/Entrega12Editor"); FParse::Value(FCommandLine::Get(),TEXT("CeroEvidenceDir="),Folder);
    auto Shot=[&](const TCHAR* Name) { FScreenshotRequest::RequestScreenshot(Folder/(FString(Name)+TEXT(".png")),true,false); };
    auto Aim=[&](FVector P) { PC->SetControlRotation((P-Player->Camera->GetComponentLocation()).Rotation()); };
    auto Visitor=[&](int32 Slot)->ACECustomer* { for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(It->CustomerId==FString::Printf(TEXT("qa12-%d"),Slot)) return *It; return nullptr; };
    auto Key=[&](FKey K,EInputEvent Event) { PC->InputKey(FInputKeyEventArgs::CreateSimulated(K,Event,Event==IE_Pressed ? 1.f : 0.f)); };
    UE_LOG(LogTemp,Display,TEXT("CE12_STAGE %d"),SmokeStage);
    switch(SmokeStage++) {
    case 50: {
        Player->Release(); Player->bPouring=false; Player->Dialogue->Close();
        for(TActorIterator<ACECustomer> It(GetWorld());It;++It) It->Destroy();
        for(TActorIterator<ACEStation> It(GetWorld());It;++It) { It->WorkLeft=0; It->WorkingCup=nullptr; It->Filter=nullptr; }
        for(TActorIterator<ACEProp> It(GetWorld());It;++It) It->Destroy();
        // Restore physical seating after the preceding sabotage scenarios.
        for(int32 Slot=0;Slot<CECafeLayout::TableCount;++Slot) {
            auto* Chair=GetWorld()->SpawnActor<ACEProp>(CECafeLayout::Chair(Slot),FRotator::ZeroRotator); Chair->Configure(ECEPropKind::Chair); Chair->Tags.Add(FName(*FString::Printf(TEXT("chair%d"),Slot)));
            auto* Guest=GetWorld()->SpawnActor<ACECustomer>(FVector(165,CECafeLayout::OrderY(Slot),92),FRotator::ZeroRotator);
            Guest->Configure(FString::Printf(TEXT("qa12-%d"),Slot),TEXT("CLIENTE DEL SALÓN"),FLinearColor(.3f,.4f,.3f),Slot%3,Slot);
            Guest->BeginVisit(Slot,ECEDrink::Espresso,false);
            const ECEGuestDevice Types[]={ECEGuestDevice::Laptop,ECEGuestDevice::Mobile,ECEGuestDevice::Tablet,ECEGuestDevice::None,ECEGuestDevice::Laptop,ECEGuestDevice::Mobile};
            Guest->PreparePossessions(Types[Slot]); Guest->State=ECECustomerState::Ordering; Guest->TakeOrder();
            Guest->DialogueHoldUntil=Now+Slot*1.5f;
        }
        bool Carried=true; int32 DeviceCount=0;
        for(TActorIterator<ACEProp> It(GetWorld());It;++It) if(It->IsDevice()) { ++DeviceCount; Carried &= It->GetAttachParentActor()!=nullptr && !It->Body->IsSimulatingPhysics(); }
        Check(TEXT("tables_start_without_devices_while_visitors_carry_them"),Carried && DeviceCount==5);
        NextArrivalAt=Now+1000; ExpansionDeadline=Now+65;
        Player->SetActorLocation(FVector(780,345,92)); Aim(FVector(780,-210,120)); Shot(TEXT("24_salon_ampliado"));
        SmokeNextAt=Now+.5f; break;
    }
    case 51: {
        bool Ready=true;
        for(int32 Slot=0;Slot<CECafeLayout::TableCount;++Slot) { auto* G=Visitor(Slot); Ready &= G && G->bSeated && G->bDevicePlaced && FVector::Dist2D(G->GetActorLocation(),CECafeLayout::Table(Slot))<130.f; }
        if(!Ready && Now<ExpansionDeadline) { SmokeStage=51; SmokeNextAt=Now+.5f; break; }
        Check(TEXT("six_visitors_reach_their_own_physical_seats_and_unpack"),Ready);
        TSet<ECEGuestDevice> Types; bool Owned=true,Screens=true,Physics=true; int32 Count=0;
        for(int32 Slot=0;Slot<CECafeLayout::TableCount;++Slot) if(auto* G=Visitor(Slot)) {
            Types.Add(G->VisitDevice);
            if(auto* D=G->PersonalDevice.Get()) {
                ++Count; Owned &= D->OwnerCustomerId==G->CustomerId && !D->GetAttachParentActor() && FVector::Dist2D(D->GetActorLocation(),CECafeLayout::Table(Slot))<100.f;
                Physics &= D->Body->IsSimulatingPhysics();
                if(D->Kind==ECEPropKind::Laptop) Screens &= FVector::DotProduct(-D->GetActorForwardVector(),FVector::ForwardVector)>.8f;
            } else Owned &= G->VisitDevice==ECEGuestDevice::None;
        }
        Check(TEXT("laptop_mobile_tablet_and_no_device_are_all_represented"),Types.Num()==4 && Count==5);
        Check(TEXT("unpacked_devices_keep_their_actual_owner_and_table"),Owned);
        Check(TEXT("unpacked_devices_are_independent_physics_objects"),Physics);
        Check(TEXT("laptop_screens_face_their_owners_after_unpacking"),Screens);
        const int32 Serial=CustomerSerial; NextArrivalAt=0; TickArrivals();
        Check(TEXT("seventh_visit_waits_when_all_six_tables_are_taken"),CustomerSerial==Serial); NextArrivalAt=Now+1000;
        Player->SetActorLocation(FVector(780,470,92)); Aim(FVector(780,-160,105)); Shot(TEXT("25_clientes_y_dispositivos"));
        SmokeNextAt=Now+.5f; break;
    }
    case 52: {
        bool Served=true;
        for(int32 Slot=0;Slot<CECafeLayout::TableCount;++Slot) if(auto* G=Visitor(Slot)) {
            G->bWantsFoam=G->bWantsSugar=G->bWantsSyrup=false; G->MinServingVolume=0; G->MinServingTemperature=45;
            auto* Cup=GetWorld()->SpawnActor<ACEProp>(CECafeLayout::Food(Slot,true),FRotator::ZeroRotator); Cup->Configure(ECEPropKind::Cup); Cup->AddLiquid(60,0,0,93);
            Served &= G->Serve(Cup) && FVector::Dist(Cup->GetActorLocation(),CECafeLayout::Food(Slot,true))<1.f;
        } else Served=false;
        Check(TEXT("breakfast_can_be_served_at_all_six_actual_tables"),Served);
        Key(EKeys::Tab,IE_Pressed);
        SmokeNextAt=Now+.6f; break;
    }
    case 53: {
        auto* Hud=Cast<ACEHUD>(PC->GetHUD()); Check(TEXT("modern_hud_is_mounted_and_Tab_opens_orders"),Hud && Hud->HasModernOverlay() && Player->bShowOrderList);
        Shot(TEXT("26_hud_seis_comandas"));
        Key(EKeys::Tab,IE_Released);
        auto* G=Visitor(1); auto* D=G ? G->PersonalDevice.Get() : nullptr;
        if(D) { Player->SetActorLocation(FVector(340,355,92)); Aim(D->GetActorLocation()); Key(EKeys::E,IE_Pressed); Key(EKeys::E,IE_Released); }
        SmokeNextAt=Now+.7f; break;
    }
    case 54: {
        auto* MobileOwner=Visitor(1); auto* TabletOwner=Visitor(2);
        Check(TEXT("player_can_grab_customer_mobile_without_owner_snapping_it_back"),MobileOwner && Player->Held && Player->Held==MobileOwner->PersonalDevice && !Player->Held->GetAttachParentActor());
        auto* Other=Visitor(0);
        Check(TEXT("device_cannot_be_returned_to_the_wrong_customer"),Other && Player->Held && MobileOwner && !Other->Serve(Player->Held) && Player->Held==MobileOwner->PersonalDevice);
        Shot(TEXT("27_movil_en_mano"));
        if(TabletOwner && IsValid(TabletOwner->PersonalDevice)) Cafe->Liquids->Emit(TabletOwner->PersonalDevice->GetActorLocation()+FVector(0,0,65),FVector(0,0,-40),0,0,6,70);
        SmokeNextAt=Now+1.5f; break;
    }
    case 55: {
        auto* G=Visitor(2);
        Check(TEXT("physical_liquid_breaks_tablet_and_reaches_its_owner"),G && IsValid(G->PersonalDevice) && G->PersonalDevice->bBroken && G->HitCount>0);
        bool ClearScreen=G && IsValid(G->PersonalDevice);
        if(ClearScreen) {
            TArray<UStaticMeshComponent*> Parts; G->PersonalDevice->GetComponents(Parts);
            for(auto* Part:Parts) ClearScreen &= !Part->ComponentHasTag(TEXT("LiquidStain"));
        }
        Check(TEXT("wet_tablet_remains_visible_without_oversized_generic_prop_stain"),ClearScreen);
        auto* MobileOwner=Visitor(1);
        const bool Returned=MobileOwner && Player->Held && MobileOwner->Serve(Player->Held);
        Check(TEXT("stolen_mobile_can_be_returned_to_its_actual_owner"),Returned && !Player->Held && IsValid(MobileOwner->PersonalDevice) && MobileOwner->PersonalDevice->Body->IsSimulatingPhysics());
        if(MobileOwner) { MobileOwner->Anger=1.1f; MobileOwner->React(TEXT("belonging_damaged"),TEXT("Reclamación de prueba"),0.f); }
        Player->Release();
        if(G && IsValid(G->PersonalDevice)) { Player->SetActorLocation(FVector(410,-285,92)); Aim(G->PersonalDevice->GetActorLocation()); }
        Shot(TEXT("28_tablet_mojada"));
        SmokeNextAt=Now+.6f; break;
    }
    case 56: {
        auto* G=Visitor(1);
        Check(TEXT("departing_customer_carries_its_recovered_device"),G && G->bLeaving && IsValid(G->PersonalDevice) && G->PersonalDevice->GetAttachParentActor()==G && !G->PersonalDevice->Body->IsSimulatingPhysics() && FVector::Dist(G->PersonalDevice->GetActorLocation(),G->GetActorLocation())<70.f);
        bPolishChecksCompleted=true; SmokeStage=FParse::Param(FCommandLine::Get(),TEXT("CeroQuickSmoke")) ? 44 : 31;
        SmokeNextAt=SmokeStage==31 ? FMath::Max(Now+1.f,241.f) : Now+.5f; break;
    }
    }
}
