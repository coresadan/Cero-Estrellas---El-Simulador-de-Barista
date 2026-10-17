#include "CEGameplay.h"
#include "CECafeLayout.h"
#include "CEVisual.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SkeletalMeshComponent.h"
#include "EngineUtils.h"

void ACECustomer::PreparePossessions(ECEGuestDevice Device)
{
    if(bPossessionsPrepared) return;
    bPossessionsPrepared=true; VisitDevice=Device;
    auto Carry=[this](ECEPropKind Kind) {
        auto* Item=GetWorld()->SpawnActor<ACEProp>(GetActorLocation(),GetActorRotation());
        Item->Configure(Kind); Item->OwnerCustomerId=CustomerId;
        Item->Tags.Add(FName(*FString::Printf(TEXT("seat%d"),SeatSlot)));
        Item->Body->SetSimulatePhysics(false); Item->SetActorEnableCollision(false);
        Item->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);
        Possessions.Add(Item); return Item;
    };
    if(Device!=ECEGuestDevice::None) {
        PersonalDevice=Carry(Device==ECEGuestDevice::Laptop ? ECEPropKind::Laptop : Device==ECEGuestDevice::Mobile ? ECEPropKind::Mobile : ECEPropKind::Tablet);
        PersonalDevice->SetDeviceCarried(true);
    }
    Carry(StoryId==1 ? ECEPropKind::Flower : StoryId==3 ? ECEPropKind::Bag : ECEPropKind::Notebook);
    TickPossessions(0);
}

void ACECustomer::TickPossessions(float DeltaSeconds)
{
    if(!bPossessionsPrepared) return;
    const FVector Table=CECafeLayout::Table(SeatSlot);
    // A stolen, thrown or damaged object never teleports back to its owner.
    if(bLeaving && bDevicePlaced && IsValid(PersonalDevice) && !bPhysicalFall && !PersonalDevice->bHeld && !PersonalDevice->WasThrown() && !PersonalDevice->bBroken && FVector::Dist2D(PersonalDevice->GetActorLocation(),Table)<110.f) {
        PersonalDevice->Body->SetSimulatePhysics(false); PersonalDevice->SetActorEnableCollision(false);
        PersonalDevice->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform);
        PersonalDevice->SetDeviceCarried(true); bDevicePlaced=false; DevicePlacement=0;
    }
    if(bPhysicalFall) for(auto Weak:Possessions) if(auto* Item=Weak.Get()) if(Item->GetAttachParentActor()==this) {
        Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
        Item->SetActorEnableCollision(true); Item->Body->SetSimulatePhysics(true);
        Item->SetDeviceCarried(false); Item->MarkThrown();
    }
    if(bSeated && !bLeaving && !bPhysicalFall && !bDevicePlaced) DevicePlacement=FMath::Min(1.f,DevicePlacement+DeltaSeconds/1.1f);
    for(auto Weak:Possessions) if(auto* Item=Weak.Get()) {
        if(Item->bHeld || Item->WasThrown() || Item->bBroken) continue;
        if(Item->GetAttachParentActor()!=this) continue;
        const bool IsDevice=Item->IsDevice();
        const FVector CarryLocal=IsDevice ? FVector(-25,0,22) : FVector(-4,27,0);
        const FVector CarryWorld=GetActorTransform().TransformPosition(CarryLocal);
        FVector Destination=CECafeLayout::Device(SeatSlot);
        if(!IsDevice) Destination=Table+FVector(-15,38,Item->Kind==ECEPropKind::Bag ? 20.f : 7.f);
        // Flat handheld screens leave room for the breakfast; laptop lid faces the chair.
        if(IsDevice && Item->Kind!=ECEPropKind::Laptop) Destination+=FVector(9,9,-.7f);
        const float Blend=FMath::SmoothStep(0.f,1.f,DevicePlacement);
        Item->SetActorLocation(FMath::Lerp(CarryWorld,Destination,Blend));
        Item->SetActorRotation(FQuat::Slerp(GetActorQuat(),FRotator(0,180,0).Quaternion(),Blend));
        if(DevicePlacement>=1.f) {
            Item->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
            Item->SetActorEnableCollision(true); Item->Body->SetSimulatePhysics(true);
            Item->Body->SetPhysicsLinearVelocity(FVector::ZeroVector);
            Item->Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
            Item->SetDeviceCarried(false);
        }
    }
    if(DevicePlacement>=1.f && !bDevicePlaced) {
        bDevicePlaced=true; DevicePlacedAt=GetWorld()->GetTimeSeconds();
        UE_LOG(LogTemp,Display,TEXT("CE12_DEVICE_PLACED owner=%s slot=%d kind=%d"),*CustomerId,SeatSlot,int32(VisitDevice));
    }
}
