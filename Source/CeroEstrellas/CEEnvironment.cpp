#include "CEKitchen.h"
#include "CEVisual.h"
#include "EngineUtils.h"
#include "Kismet/GameplayStatics.h"

ACECafeDoor::ACECafeDoor()
{
    PrimaryActorTick.bCanEverTick=true;
    Hinge=CreateDefaultSubobject<USceneComponent>(TEXT("DoorHinge")); SetRootComponent(Hinge);
}
void ACECafeDoor::BeginPlay()
{
    Super::BeginPlay();
    CEVisual::Part(this,TEXT("Cube"),FVector(40,0,0),FVector(.78f,.08f,2.38f),FLinearColor(.14f,.32f,.29f));
    CEVisual::Part(this,TEXT("Cube"),FVector(40,-4,33),FVector(.62f,.008f,.75f),FLinearColor(.2f,.45f,.47f));
    CEVisual::Part(this,TEXT("Cube"),FVector(70,-7,-20),FVector(.055f,.07f,.26f),FLinearColor(.65f,.68f,.6f));
}
void ACECafeDoor::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds); const float Now=GetWorld()->GetTimeSeconds();
    bool Nearby=false;
    for(TActorIterator<ACECustomer> It(GetWorld());It;++It) Nearby|=FVector::Dist2D(It->GetActorLocation(),GetActorLocation()+FVector(40,0,0))<170.f;
    if(auto* Player=UGameplayStatics::GetPlayerPawn(this,0)) Nearby|=FVector::Dist2D(Player->GetActorLocation(),GetActorLocation()+FVector(40,0,0))<150.f;
    if(Nearby) HoldUntil=Now+1.5f;
    OpenAngle=FMath::FInterpTo(OpenAngle,Now<HoldUntil ? -88.f : 0.f,DeltaSeconds,4.f);
    Hinge->SetWorldRotation(FRotator(0,OpenAngle,0));
}
