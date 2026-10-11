#include "CEGameplay.h"
#include "CEKitchen.h"
#include "CEVisual.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Engine/PointLight.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "EngineUtils.h"

ACEGameMode::ACEGameMode()
{
    PrimaryActorTick.bCanEverTick=true;
    DefaultPawnClass=ACEPlayer::StaticClass();
    HUDClass=ACEHUD::StaticClass();
}

void ACEGameMode::BeginPlay()
{
    BuildCafe();
    Super::BeginPlay();
    GetWorld()->GetSubsystem<UCECafeSubsystem>()->Observe(TEXT("shift_started"),TEXT(""),TEXT(""),0);
    bSmoke=FParse::Param(FCommandLine::Get(),TEXT("CeroSmokeTest"));
    bLoopSmoke=bSmoke;
    UE_LOG(LogTemp,Display,TEXT("CE_READY version=0.3.0 smoke=%d"),bSmoke);
}

void ACEGameMode::Tick(float DeltaSeconds)
{
    if (bSmoke && GEngine && GEngine->GameViewport)
    {
        // Injected controller events still work; real window input cannot disrupt the scenario.
        GEngine->GameViewport->SetIgnoreInput(true);
        if (!bSmokeInputReady)
        {
            if (auto* PC=UGameplayStatics::GetPlayerController(this,0))
            {
                PC->FlushPressedKeys();
                bSmokeInputReady=true;
            }
        }
    }
    Super::Tick(DeltaSeconds);
    GetWorld()->GetSubsystem<UCECafeSubsystem>()->TickRadio();
    TickArrivals();
    if (bSmoke && GetWorld()->GetTimeSeconds()>=SmokeNextAt) SmokeStep();
}

void ACEGameMode::BuildCafe()
{
    const FLinearColor Teal(.025f,.19f,.16f), Deep(.012f,.065f,.055f), Cream(.84f,.74f,.54f), Wood(.29f,.14f,.06f), Orange(.9f,.28f,.07f), Metal(.22f,.28f,.25f);
    auto Block=[this](FVector Location,FVector Size,FLinearColor Color,bool bCollision=true,const TCHAR* Shape=TEXT("Cube"),bool bUnlit=false)
    {
        auto* Actor=GetWorld()->SpawnActor<AActor>(Location,FRotator::ZeroRotator);
        auto* Mesh=CEVisual::Part(Actor,Shape,FVector::ZeroVector,Size/100.f,Color,bUnlit);
        Actor->SetActorLocation(Location);
        if (bCollision)
        {
            Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Mesh->SetCollisionObjectType(ECC_WorldStatic);
            Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        }
        return Actor;
    };
    auto Sign=[this](FString Text,FVector Location,float Size,FColor Color,FRotator Rotation=FRotator(0,180,0))
    {
        auto* Actor=GetWorld()->SpawnActor<AActor>(Location,Rotation);
        auto* Component=NewObject<UTextRenderComponent>(Actor);
        Actor->SetRootComponent(Component);
        Actor->AddInstanceComponent(Component);
        Component->SetText(FText::FromString(Text));
        Component->SetWorldSize(Size);
        Component->SetHorizontalAlignment(EHTA_Center);
        Component->SetTextRenderColor(Color);
        Component->RegisterComponent();
        Actor->SetActorLocationAndRotation(Location,Rotation);
    };
    // Compact L-shaped work area. All geometry is inexpensive, intentionally stylised.
    Block(FVector(120,0,-10),FVector(1380,1200,20),Cream);
    for (int32 X=-5;X<=7;++X)
        for (int32 Y=-5;Y<=5;++Y)
            Block(FVector(X*100,Y*100,.3f),FVector(98,98,.5f),((X+Y)%2==0) ? FLinearColor(.35f,.43f,.34f) : FLinearColor(.68f,.65f,.49f),false);
    Block(FVector(-570,0,170),FVector(20,1200,340),Teal);
    Block(FVector(800,0,170),FVector(20,1200,340),Teal);
    Block(FVector(120,-600,170),FVector(1380,20,340),Teal);
    // Leave a real opening: swept customer capsules must cross the doorway.
    Block(FVector(-2.5f,600,60),FVector(1135,20,120),Teal);
    Block(FVector(742.5f,600,60),FVector(135,20,120),Teal);
    Block(FVector(120,600,320),FVector(1380,20,40),Teal);
    Block(FVector(120,0,350),FVector(1380,1200,20),FLinearColor(.19f,.22f,.15f));
    for (int32 X=-500;X<=750;X+=250)
    {
        Block(FVector(X,598,205),FVector(9,18,190),Deep);
        if(X!=500)
        {
            Block(FVector(X+120,607,208),FVector(231,8,177),FLinearColor(.31f,.58f,.64f),false,TEXT("Cube"),true);
            Block(FVector(X+100,609,170),FVector(80,5,45),FLinearColor(.23f,.36f,.36f),false,TEXT("Cube"),true);
        }
    }
    // Door is a recognisable destination, with a physical frame and an exit sign.
    Block(FVector(563,581,135),FVector(14,14,270),Deep);
    Block(FVector(677,581,135),FVector(14,14,270),Deep);
    Block(FVector(620,581,273),FVector(128,14,14),Deep);
    Block(FVector(620,670,-5),FVector(220,160,10),Cream);
    Block(FVector(520,670,160),FVector(20,160,320),Deep);
    Block(FVector(720,670,160),FVector(20,160,320),Deep);
    Block(FVector(620,750,160),FVector(220,20,320),FLinearColor(.15f,.32f,.37f));
    Block(FVector(620,670,350),FVector(220,160,20),Deep);
    GetWorld()->SpawnActor<ACECafeDoor>(FVector(580,573,135),FRotator::ZeroRotator);
    Sign(TEXT("SALIDA"),FVector(620,562,270),18,FColor(175,231,160),FRotator(0,-90,0));
    // Backbar, menu, shelves and espresso machine.
    Block(FVector(-380,-150,104),FVector(110,650,10),Wood);
    Block(FVector(-380,-150,52),FVector(105,650,100),Deep);
    for (int32 Shelf=0;Shelf<2;++Shelf)
    {
        Block(FVector(-545,-170,170+Shelf*80),FVector(30,620,6),Wood);
        for (int32 i=0;i<9;++i)
            Block(FVector(-532,-420+i*62,190+Shelf*80),FVector(17,17,34),i%3==0 ? Orange : Cream,false,TEXT("Cylinder"));
    }
    Block(FVector(45,-65,50),FVector(90,430,100),Teal);
    Block(FVector(45,-65,104),FVector(110,455,9),Wood);
    Block(FVector(-105,-330,50),FVector(390,90,100),Teal);
    Block(FVector(-105,-330,104),FVector(410,110,9),Wood);
    for(int32 y=-260;y<=140;y+=50) Block(FVector(-1,y,51),FVector(2,3,91),FLinearColor(.08f,.32f,.25f),false);
    auto Station=[this](ECEStationKind Kind,FVector Position)
    {
        auto* S=GetWorld()->SpawnActor<ACEStation>(Position,FRotator::ZeroRotator); S->Configure(Kind); return S;
    };
    EspressoMachine=Station(ECEStationKind::Espresso,FVector(-155,-330,145));
    Grinder=Station(ECEStationKind::Grinder,FVector(-42,-330,146));
    MilkStation=Station(ECEStationKind::Milk,FVector(-265,-330,141));
    WaterStation=Station(ECEStationKind::Water,FVector(-345,-330,141));
    CupSupply=Station(ECEStationKind::Cups,FVector(-380,-195,123));
    Block(FVector(-50,-262,104),FVector(70,38,9),Wood);
    TampStation=Station(ECEStationKind::Tamper,FVector(-50,-260,117));
    TraySupply=Station(ECEStationKind::Trays,FVector(-380,30,127));
    Station(ECEStationKind::Bin,FVector(-270,300,38));
    auto* Liquids=GetWorld()->SpawnActor<ACELiquidSystem>();
    GetWorld()->GetSubsystem<UCECafeSubsystem>()->Liquids=Liquids;
    // Syrup bottles and a bakery stand, accessible from the barista's side.
    Station(ECEStationKind::Pastries,FVector(45,95,121));
    Sign(TEXT("PREPARACIÓN\n1 MOLER  2 PRENSAR  3 EXTRAER\nLATTE: JARRA → VAPOR → VERTER"),FVector(-145,-382,226),8,FColor(255,213,162),FRotator(0,90,0));
    // Main wall graphic and corporate slogans.
    Block(FVector(787,-70,260),FVector(5,610,150),Deep,false);
    Sign(TEXT("CERO ESTRELLAS"),FVector(782,-70,286),40,FColor(255,207,134));
    Sign(TEXT("EL CLIENTE SIEMPRE TIENE LA CULPA"),FVector(250,-581,310),10,FColor(158,205,185),FRotator(0,90,0));
    Block(FVector(250,-587,230),FVector(310,5,140),Deep,false);
    Sign(TEXT("CARTA DEL DÍA\nEspresso existencial .... 3,50\nLatte de resignación .... 4,00\nDespido ........ AGOTADO"),FVector(250,-582,274),14,FColor(238,209,157),FRotator(0,90,0));
    Sign(TEXT("SONRÍE. ES OBLIGATORIO."),FVector(-557,300,230),17,FColor(255,193,106),FRotator::ZeroRotator);
    // Seating and the three approved archetypes. Visuals and reactions are placeholders, not LLM output.
    const float Ys[]={-100.f,220.f,-405.f};
    for(int32 i=0;i<3;++i)
    {
        Block(FVector(355,Ys[i],78),FVector(125,115,8),Wood);
        Block(FVector(355,Ys[i],38),FVector(12,12,75),Deep);
        Block(FVector(355,Ys[i],4),FVector(65,65,8),Deep);
        Block(FVector(294,Ys[i],93),FVector(3,44,20),Deep,false);
        Sign(FString::Printf(TEXT("MESA %d"),i+1),FVector(292,Ys[i],99),6,FColor(255,219,154));
        auto* Chair=GetWorld()->SpawnActor<ACEProp>(FVector(495,Ys[i],52),FRotator::ZeroRotator);
        Chair->Configure(ECEPropKind::Chair); Chair->Tags.Add(FName(*FString::Printf(TEXT("chair%d"),i)));
    }
    // Loose cups on a real physical tray; no parenting or simulated fluid.
    auto SpawnProp=[this](ECEPropKind Kind,FVector Location)
    {
        auto* Prop=GetWorld()->SpawnActor<ACEProp>(Location,FRotator::ZeroRotator);
        Prop->Configure(Kind);
        return Prop;
    };
    SmokeTray=SpawnProp(ECEPropKind::Tray,FVector(45,-145,112));
    SmokeCup=SpawnProp(ECEPropKind::Cup,FVector(45,-145,134));
    SpawnProp(ECEPropKind::Cup,FVector(45,-170,134));
    SpawnProp(ECEPropKind::Cup,FVector(45,-55,123));
    SmokePastry=SpawnProp(ECEPropKind::Pastry,FVector(45,14,118));
    SpawnProp(ECEPropKind::Pastry,FVector(40,90,151));
    SpawnProp(ECEPropKind::Pastry,FVector(48,115,151));
    for(int32 i=0;i<4;++i) SpawnProp(ECEPropKind::Tray,FVector(-380,30,155+i*10));
    for(int32 i=0;i<6;++i) SpawnProp(ECEPropKind::Cup,FVector(-393+(i%2)*27,-212+(i/2)*24,151));
    SpawnProp(ECEPropKind::Portafilter,FVector(-38,-287,120));
    // Planters and poster accents.
    for (FVector P : {FVector(710,440,0),FVector(-480,470,0)})
    {
        Block(P+FVector(0,0,30),FVector(55,55,60),Orange,true,TEXT("Cylinder"));
        for(int32 i=0;i<5;++i) Block(P+FVector(FMath::Sin(i*1.2f)*20,FMath::Cos(i*1.2f)*20,80+i*8),FVector(18,18,70),Teal,false,TEXT("Sphere"));
    }
    auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,300),FRotator(-55,-35,0));
    Sun->GetLightComponent()->SetIntensity(2.8f);
    Sun->GetLightComponent()->SetLightColor(FLinearColor(1.f,.88f,.66f));
    auto* Sky=GetWorld()->SpawnActor<ASkyLight>();
    Sky->GetLightComponent()->SetIntensity(.7f);
    for(FVector P : {FVector(-180,-120,300),FVector(330,-250,300),FVector(330,200,300),FVector(-400,350,300)})
    {
        auto* Light=GetWorld()->SpawnActor<APointLight>(P,FRotator::ZeroRotator);
        auto* Point=Cast<UPointLightComponent>(Light->GetLightComponent());
        Point->SetIntensity(4500.f);
        Point->SetAttenuationRadius(650.f);
        Point->SetLightColor(FLinearColor(1.f,.82f,.54f));
        Point->SetCastShadows(false);
        Block(P+FVector(0,0,11),FVector(55,55,15),Orange,false,TEXT("Cone"));
        Block(P,FVector(32,32,3),FLinearColor(1.f,.66f,.28f),false,TEXT("Cylinder"),true);
        Block(P+FVector(0,0,30),FVector(3,3,38),Deep,false,TEXT("Cylinder"));
    }
    UE_LOG(LogTemp,Display,TEXT("CE_CAFE_BUILT version=0.3 stations=9 replenishable_stock=1"));
}

void ACEGameMode::TickArrivals()
{
    const float Now=GetWorld()->GetTimeSeconds(); if(Now<NextArrivalAt) return;
    bool Taken[3]={false,false,false};
    for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(It->SeatSlot>=0 && It->SeatSlot<3) Taken[It->SeatSlot]=true;
    int32 Slot=INDEX_NONE; for(int32 i=0;i<3;++i) if(!Taken[i]) { Slot=i; break; }
    if(Slot==INDEX_NONE) { NextArrivalAt=Now+3.f; return; }
    const int32 Archetype=CustomerSerial++%3;
    const FString Names[]={TEXT("CRIPTO-BRO"),TEXT("HIPSTER PURISTA"),TEXT("INFLUENCER")};
    const FLinearColor Colors[]={FLinearColor(.09f,.22f,.25f),FLinearColor(.38f,.22f,.12f),FLinearColor(.53f,.28f,.35f)};
    auto* Customer=GetWorld()->SpawnActor<ACECustomer>(FVector(620,650,92),FRotator::ZeroRotator);
    Customer->Configure(FString::Printf(TEXT("cliente-%03d"),CustomerSerial),Names[Archetype],Colors[Archetype],Archetype);
    Customer->BeginVisit(Slot,static_cast<ECEDrink>((CustomerSerial-1)%3),(CustomerSerial%2)==0);
    if(!IsValid(SmokeCustomer)) SmokeCustomer=Customer;
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>(); Cafe->Sound(TEXT("Door"),FVector(620,560,160));
    Cafe->Notify(TEXT("Ha entrado un cliente. Acércate y pulsa F para tomar su pedido."));
    // Replace the table's used laptop instead of growing the physics actor count forever.
    const float Ys[]={-100.f,220.f,-405.f};
    for(TActorIterator<ACEProp> It(GetWorld());It;++It) if(It->Kind==ECEPropKind::Laptop && It->Tags.Contains(FName(*FString::Printf(TEXT("seat%d"),Slot)))) It->Destroy();
    auto* Laptop=GetWorld()->SpawnActor<ACEProp>(FVector(355,Ys[Slot],85),FRotator::ZeroRotator); Laptop->Configure(ECEPropKind::Laptop);
    Laptop->Tags.Add(FName(*FString::Printf(TEXT("seat%d"),Slot)));
    NextArrivalAt=Now+FMath::Max(10.f,18.f-(Cafe->ShiftLevel-1)*1.5f);
    UE_LOG(LogTemp,Display,TEXT("CE_ARRIVAL id=%s slot=%d order=%s"),*Customer->CustomerId,Slot,*Customer->OrderText());
}
