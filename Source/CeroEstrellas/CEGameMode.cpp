#include "CEGameplay.h"
#include "CEKitchen.h"
#include "CECafeLayout.h"
#include "CEVisual.h"
#include "Components/TextRenderComponent.h"
#include "Components/PointLightComponent.h"
#include "Components/DirectionalLightComponent.h"
#include "Components/SkyLightComponent.h"
#include "Components/RectLightComponent.h"
#include "Engine/RectLight.h"
#include "Engine/PointLight.h"
#include "Engine/DirectionalLight.h"
#include "Engine/SkyLight.h"
#include "Engine/PostProcessVolume.h"
#include "Engine/TextureCube.h"
#include "Engine/Engine.h"
#include "Engine/GameViewportClient.h"
#include "GameFramework/PlayerController.h"
#include "Kismet/GameplayStatics.h"
#include "Misc/CommandLine.h"
#include "Misc/Parse.h"
#include "EngineUtils.h"
#include "Widgets/SWindow.h"
#include "HAL/IConsoleManager.h"
#include "Engine/Light.h"
#include "Components/LightComponent.h"
#include "RHI.h"

ACEGameMode::ACEGameMode()
{
    PrimaryActorTick.bCanEverTick=true;
    DefaultPawnClass=ACEPlayer::StaticClass();
    HUDClass=ACEHUD::StaticClass();
}

void ACEGameMode::BeginPlay()
{
    GetWorld()->GetSubsystem<UCECafeSubsystem>()->WarmResources();
    BuildCafe();
    const bool WantsLumen=FParse::Param(FCommandLine::Get(),TEXT("CeroLumen"));
    const bool UseLumen=WantsLumen && GMaxRHIFeatureLevel>=ERHIFeatureLevel::SM6;
    if(UseLumen) {
        auto Set=[](const TCHAR* Name,int32 Value) { if(auto* C=IConsoleManager::Get().FindConsoleVariable(Name)) C->Set(Value,ECVF_SetByCode); };
        Set(TEXT("r.DynamicGlobalIlluminationMethod"),1); Set(TEXT("r.ReflectionMethod"),1);
        Set(TEXT("r.Lumen.HardwareRayTracing"),0); Set(TEXT("r.Lumen.TraceMeshSDFs"),0);
        Set(TEXT("sg.GlobalIlluminationQuality"),2); Set(TEXT("sg.ReflectionQuality"),2);
        for(TActorIterator<ALight> It(GetWorld());It;++It) if(It->ActorHasTag(TEXT("CEArchitecture"))) It->GetLightComponent()->SetMobility(EComponentMobility::Movable);
        for(TActorIterator<ASkyLight> It(GetWorld());It;++It) It->GetLightComponent()->SetMobility(EComponentMobility::Movable);
    }
    UE_LOG(LogTemp,Display,TEXT("CE12_GRAPHICS profile=%s requested_lumen=%d feature_level=%d"),UseLumen ? TEXT("LumenSoftwareExperimental") : TEXT("BakedDX11"),WantsLumen,int32(GMaxRHIFeatureLevel));
    if(GetWorld()->WorldType==EWorldType::Game && GEngine && GEngine->GameViewport)
        if(auto Window=GEngine->GameViewport->GetWindow()) Window->SetTitle(FText::FromString(TEXT("Cero Estrellas · El Simulador de Barista")));
    Super::BeginPlay();
    GetWorld()->GetSubsystem<UCECafeSubsystem>()->Observe(TEXT("shift_started"),TEXT(""),TEXT(""),0);
    bSmoke=FParse::Param(FCommandLine::Get(),TEXT("CeroSmokeTest"));
    bDialogueSmoke=FParse::Param(FCommandLine::Get(),TEXT("CeroDialogueTest"));
    bSmoke|=bDialogueSmoke;
    bLoopSmoke=bSmoke;
    if(FParse::Param(FCommandLine::Get(),TEXT("CeroPolishOnly"))) SmokeStage=34;
    UE_LOG(LogTemp,Display,TEXT("CE_READY version=0.12.0 smoke=%d"),bSmoke);
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
    const double WallNow=FPlatformTime::Seconds();
    if(bSmoke && LastFrameWallSeconds>0 && GetWorld()->GetTimeSeconds()>10.f && FrameSamples.Num()<36000)
        FrameSamples.Add(float((WallNow-LastFrameWallSeconds)*1000.));
    LastFrameWallSeconds=WallNow;
    GetWorld()->GetSubsystem<UCECafeSubsystem>()->TickRadio();
    TickArrivals();
    TickTickets();
    if (bSmoke && GetWorld()->GetTimeSeconds()>=SmokeNextAt) SmokeStep();
}

void ACEGameMode::BuildCafe(bool bAuthoring)
{
    const FLinearColor Teal(.065f,.23f,.19f), Deep(.025f,.078f,.073f), Cream(.78f,.75f,.65f), Wood(.29f,.14f,.06f), Orange(.70f,.31f,.12f), Metal(.22f,.28f,.25f);
    TMap<FName,AActor*> FixedSet;
    if(!bAuthoring) for(TActorIterator<AActor> It(GetWorld());It;++It)
        if(It->ActorHasTag(TEXT("CEArchitecture"))) for(const FName Tag:It->Tags)
            if(Tag.ToString().StartsWith(TEXT("CE12_"))) FixedSet.Add(Tag,*It);
    int32 BlockIndex=0,SignIndex=0;
    auto Block=[this,bAuthoring,&FixedSet,&BlockIndex](FVector Location,FVector Size,FLinearColor Color,bool bCollision=true,const TCHAR* Shape=TEXT("Cube"),bool bUnlit=false)
    {
        const FName Key(*FString::Printf(TEXT("CE12_Block_%04d"),BlockIndex++));
        if(auto* Existing=FixedSet.FindRef(Key)) return Existing;
        auto* Actor=GetWorld()->SpawnActor<AActor>(Location,FRotator::ZeroRotator);
        if(bAuthoring) { Actor->Tags.Add(TEXT("CEArchitecture")); Actor->Tags.Add(Key); }
        auto* Mesh=CEVisual::Part(Actor,Shape,FVector::ZeroVector,Size/100.f,Color,bUnlit);
        Mesh->SetCastShadow(!bUnlit);
        Actor->SetActorLocation(Location);
        if (bCollision)
        {
            Mesh->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
            Mesh->SetCollisionObjectType(ECC_WorldStatic);
            Mesh->SetCollisionResponseToAllChannels(ECR_Block);
        }
        return Actor;
    };
    auto Sign=[this,bAuthoring,&FixedSet,&SignIndex](FString Text,FVector Location,float Size,FColor Color,FRotator Rotation=FRotator(0,180,0))
    {
        const FName Key(*FString::Printf(TEXT("CE12_Sign_%04d"),SignIndex++));
        if(auto* Existing=FixedSet.FindRef(Key)) return Cast<UTextRenderComponent>(Existing->GetRootComponent());
        auto* Actor=GetWorld()->SpawnActor<AActor>(Location,Rotation);
        if(bAuthoring) { Actor->Tags.Add(TEXT("CEArchitecture")); Actor->Tags.Add(Key); }
        auto* Component=NewObject<UTextRenderComponent>(Actor);
        Actor->SetRootComponent(Component);
        Actor->AddInstanceComponent(Component);
        Component->SetText(FText::FromString(Text));
        Component->SetWorldSize(Size);
        Component->SetHorizontalAlignment(EHTA_Center);
        Component->SetVerticalAlignment(EVRTA_TextTop);
        Component->SetTextRenderColor(Color);
        Component->RegisterComponent();
        Actor->SetActorLocationAndRotation(Location,Rotation);
        return Component;
    };
    // Compact L-shaped work area. All geometry is inexpensive, intentionally stylised.
    Block(FVector(455,0,-10),FVector(2050,1200,20),Cream);
    for (int32 X=-5;X<=14;++X)
        for (int32 Y=-5;Y<=5;++Y)
        {
            const FLinearColor TileColor=((X+Y)%2==0) ? FLinearColor(.29f,.34f,.32f) : FLinearColor(.68f,.65f,.57f);
            auto* Tile=Block(FVector(X*100,Y*100,.3f),FVector(98,98,.5f),TileColor,false);
            Cast<UStaticMeshComponent>(Tile->GetRootComponent())->SetMaterial(0,CEVisual::Surface(Tile,TileColor,TEXT("M_Stone")));
        }
    Block(FVector(-570,0,170),FVector(20,1200,340),Teal);
    Block(FVector(1480,0,170),FVector(20,1200,340),Teal);
    Block(FVector(455,-600,170),FVector(2050,20,340),Teal);
    // Leave a real opening: swept customer capsules must cross the doorway.
    Block(FVector(-2.5f,600,60),FVector(1135,20,120),Teal);
    Block(FVector(1082.5f,600,60),FVector(815,20,120),Teal);
    Block(FVector(455,600,320),FVector(2050,20,40),Teal);
    Block(FVector(455,0,350),FVector(2050,1200,20),FLinearColor(.62f,.57f,.46f));
    // Wainscoting and brass trim give the large walls depth without extra simulation.
    Block(FVector(1466,0,54),FVector(6,1180,108),Deep,false);
    Block(FVector(1462,0,111),FVector(3,1180,3),FLinearColor(.42f,.30f,.14f),false);
    for(int32 Y=-540;Y<=540;Y+=90) Block(FVector(1461,Y,54),FVector(3,2,97),Teal,false);
    Block(FVector(455,-586,54),FVector(2010,6,108),Deep,false);
    Block(FVector(455,-582,111),FVector(2010,3,3),FLinearColor(.42f,.30f,.14f),false);
    for (int32 X=-500;X<=1250;X+=250)
    {
        Block(FVector(X,598,205),FVector(9,18,190),Deep);
        if(X!=500)
        {
            Block(FVector(X+120,607,208),FVector(231,8,177),FLinearColor(.31f,.58f,.64f),false,TEXT("Cube"),true);
            // Low-cost street silhouettes sit in front of the opaque sky panel.
            // They give the windows a view rather than a featureless luminous wall.
            for(int32 Building=0;Building<3;++Building) {
                const float BX=X+40.f+Building*78.f, Height=65.f+((X/250+Building+6)%3)*35.f;
                Block(FVector(BX,601,119+Height/2),FVector(68,3,Height),FLinearColor(.13f+.025f*Building,.23f,.26f),false,TEXT("Cube"),true);
                Block(FVector(BX,599,120+Height),FVector(73,2,4),FLinearColor(.10f,.16f,.18f),false,TEXT("Cube"),true);
                for(int32 Row=0;Row<3;++Row) for(int32 Column=0;Column<2;++Column)
                    if(25.f+Row*25.f<Height-8.f) Block(FVector(BX-16+Column*32,598,139+Row*25),FVector(12,1,13),FLinearColor(.38f,.48f,.46f),false,TEXT("Cube"),true);
            }
            Block(FVector(X+120,594,207),FVector(5,8,177),Deep,false);
            Block(FVector(X+120,594,208),FVector(231,8,5),Deep,false);
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
    if(!bAuthoring) GetWorld()->SpawnActor<ACECafeDoor>(FVector(580,573,135),FRotator::ZeroRotator);
    Sign(TEXT("SALIDA"),FVector(620,562,270),18,FColor(175,231,160),FRotator(0,-90,0));
    // Backbar, menu, shelves and espresso machine.
    Block(FVector(-380,-150,104),FVector(110,650,10),Wood);
    Block(FVector(-380,-150,52),FVector(105,650,100),Deep);
    for (int32 Shelf=0;Shelf<2;++Shelf)
    {
        Block(FVector(-545,-170,170+Shelf*80),FVector(30,620,6),Wood);
        for (int32 i=0;i<9;++i)
            Block(FVector(-532,-420+i*62,190+Shelf*80),FVector(17,17,34),i%3==0 ? Orange : Cream,false,TEXT("SM_Bottle"));
    }
    Block(FVector(45,-65,50),FVector(90,430,100),Teal);
    Block(FVector(45,-65,104),FVector(110,455,9),Wood);
    Block(FVector(-105,-330,50),FVector(390,90,100),Teal);
    Block(FVector(-105,-330,104),FVector(410,110,9),Wood);
    for(int32 y=-260;y<=140;y+=50) Block(FVector(-1,y,51),FVector(2,3,91),FLinearColor(.08f,.32f,.25f),false);
    // Ribbed timber, recessed toe kick and a brass edge make the counter read
    // as furniture. These details don't add obstacles to the work area.
    for(int32 y=-266;y<=136;y+=18) Block(FVector(92,y,53),FVector(4,12,82),Wood,false);
    Block(FVector(93,-65,10),FVector(6,428,12),Deep,false);
    Block(FVector(99,-65,102),FVector(2,450,2),FLinearColor(.42f,.30f,.14f),false);
    auto Station=[this,bAuthoring](ECEStationKind Kind,FVector Position)
    {
        if(bAuthoring) return static_cast<ACEStation*>(nullptr);
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
    if(!bAuthoring) {
        auto* Liquids=GetWorld()->SpawnActor<ACELiquidSystem>();
        GetWorld()->GetSubsystem<UCECafeSubsystem>()->Liquids=Liquids;
    }
    // Syrup bottles and a bakery stand, accessible from the barista's side.
    Station(ECEStationKind::Pastries,FVector(45,95,121));
    Station(ECEStationKind::Sugar,FVector(-380,-95,122));
    Station(ECEStationKind::Syrup,FVector(-380,130,122));
    Station(ECEStationKind::Frother,FVector(-380,-420,123));
    for(int32 i=0;i<CECafeLayout::TableCount;++i) {
        const FVector P(-15-(i/3)*42.f,-235+(i%3)*75.f,110.7f);
        Block(P,FVector(36,62,.7f),Cream,false);
        OrderTickets.Add(Sign(TEXT("COMANDA"),P+FVector(-15,0,.5f),2.7f,FColor(45,56,49),FRotator(90,180,0)));
    }
    Block(FVector(-145,-384,215),FVector(195,3,57),Deep,false);
    Sign(TEXT("PREPARACIÓN\n1 MOLER  2 PRENSAR  3 EXTRAER\nLATTE: JARRA / VAPOR / VERTER"),FVector(-145,-381,236),6.5f,FColor(232,216,182),FRotator(0,90,0));
    // Main wall graphic and corporate slogans.
    Block(FVector(1467,-70,260),FVector(5,610,150),Deep,false);
    Sign(TEXT("CERO ESTRELLAS"),FVector(1462,-70,286),40,FColor(255,207,134));
    Sign(TEXT("CAFÉ DE ESPECIALIDAD  /  PACIENCIA LIMITADA"),FVector(1462,-70,228),10,FColor(173,192,179));
    Sign(TEXT("EL CLIENTE SIEMPRE TIENE LA CULPA"),FVector(250,-581,310),10,FColor(158,205,185),FRotator(0,90,0));
    Block(FVector(250,-587,230),FVector(310,5,140),Deep,false);
    Sign(TEXT("CARTA DEL DÍA\nEspresso existencial .... 3,50\nLatte de resignación .... 4,00\nDespido ........ AGOTADO"),FVector(250,-582,274),14,FColor(238,209,157),FRotator(0,90,0));
    Sign(TEXT("SONRÍE. ES OBLIGATORIO."),FVector(-557,300,230),17,FColor(255,193,106),FRotator::ZeroRotator);
    // Framed coffee graphics, clock and small shelf details complete the room.
    for(float Y:{-475.f,420.f}) {
        Block(FVector(1463,Y,245),FVector(4,121,125),Wood,false);
        Block(FVector(1460,Y,245),FVector(2,111,115),Cream,false);
        auto* Bean=Block(FVector(1458,Y,250),FVector(3,27,43),Orange,false,TEXT("Sphere"));
        Bean->SetActorRotation(FRotator(0,0,-24));
        Block(FVector(1455,Y,250),FVector(1,2,24),Wood,false);
        Sign(TEXT("CAFÉ DE ORIGEN"),FVector(1455,Y,211),7,FColor(72,62,45));
        Sign(TEXT("CERO PRISAS"),FVector(1455,Y,197),5,FColor(105,94,74));
    }
    auto* Clock=Block(FVector(-554,120,252),FVector(49,49,4),Deep,false,TEXT("Cylinder"));
    Clock->SetActorRotation(FRotator(90,0,0));
    auto* Face=Block(FVector(-550,120,252),FVector(43,43,2),Cream,false,TEXT("Cylinder"));
    Face->SetActorRotation(FRotator(90,0,0));
    for(int32 Hour=0;Hour<12;++Hour) {
        const float Angle=Hour*PI/6;
        auto* Mark=Block(FVector(-548,120+FMath::Sin(Angle)*18,252+FMath::Cos(Angle)*18),FVector(1,1.5f,4),Deep,false);
        Mark->SetActorRotation(FRotator(0,0,-Hour*30.f));
    }
    Block(FVector(-546,120,260),FVector(1,1.5f,17),Deep,false);
    Block(FVector(-545,126,252),FVector(1,13,1.5f),Deep,false);
    // Seating and the three approved archetypes. Visuals and reactions are placeholders, not LLM output.
    for(int32 i=0;i<CECafeLayout::TableCount;++i)
    {
        const FVector T=CECafeLayout::Table(i);
        auto* Mat=Block(T+FVector(20,0,4.5f),FVector(62,87,.4f),FLinearColor(.19f,.28f,.25f),false);
        Cast<UStaticMeshComponent>(Mat->GetRootComponent())->SetMaterial(0,CEVisual::Surface(Mat,FLinearColor(.19f,.28f,.25f),TEXT("M_Fabric")));
        auto* Saucer=Block(T+FVector(30,-28,5),FVector(31,31,1.2f),Cream,false,TEXT("SM09_Saucer"));
        Cast<UStaticMeshComponent>(Saucer->GetRootComponent())->SetMaterial(0,CEVisual::Surface(Saucer,Cream,TEXT("M_Ceramic")));
        Block(T,FVector(125,115,8),Wood,true,TEXT("SM09_TableTop"));
        Block(T+FVector(0,0,-40),FVector(12,12,75),Deep);
        Block(T+FVector(0,0,-74),FVector(65,65,8),Deep);
        Block(T+FVector(31,34,5.3f),FVector(19,13,.7f),Cream,false);
        Block(T+FVector(31,38,6),FVector(23,1.4f,1),Metal,false);
        Block(T+FVector(-61,0,15),FVector(3,44,20),Deep,false);
        Sign(FString::Printf(TEXT("MESA %d"),i+1),T+FVector(-63,0,21),6,FColor(255,219,154));
        if(!bAuthoring) {
        auto* Chair=GetWorld()->SpawnActor<ACEProp>(CECafeLayout::Chair(i),FRotator::ZeroRotator);
        Chair->Configure(ECEPropKind::Chair); Chair->Tags.Add(FName(*FString::Printf(TEXT("chair%d"),i)));
        }
    }
    // Loose cups on a real physical tray; no parenting or simulated fluid.
    auto SpawnProp=[this,bAuthoring](ECEPropKind Kind,FVector Location)
    {
        if(bAuthoring) return static_cast<ACEProp*>(nullptr);
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
    // Side salon: low divider, service sideboard and loose crockery for chains.
    Block(FVector(780,-270,40),FVector(32,240,80),Teal);
    Block(FVector(780,-270,83),FVector(40,250,6),Wood);
    for(float Y:{-350.f,-190.f}) {
        Block(FVector(780,Y,101),FVector(29,29,32),Orange,false,TEXT("SM_Pot"));
        for(int32 i=0;i<4;++i) Block(FVector(780+FMath::Sin(i*2.f)*9,Y,132+i*4),FVector(13,15,37),Teal,false,TEXT("SM_Leaf"));
    }
    Block(FVector(1400,-390,47),FVector(100,270,94),Deep);
    Block(FVector(1400,-390,98),FVector(112,284,9),Wood);
    Sign(TEXT("SALÓN / SIN REUNIONES PRODUCTIVAS"),FVector(1050,-580,252),15,FColor(238,209,157),FRotator(0,90,0));
    Sign(TEXT("DEVUELVE LA VAJILLA. O HAZ HISTORIA."),FVector(1460,-390,183),9,FColor(238,209,157));
    if(!bAuthoring) {
        SpawnProp(ECEPropKind::Tray,FVector(1385,-440,108));
        for(int32 i=0;i<4;++i) SpawnProp(ECEPropKind::Cup,FVector(1370+(i%2)*31,-430+(i/2)*45,123));
        auto* Jug=SpawnProp(ECEPropKind::Pitcher,FVector(1390,-295,118)); Jug->AddLiquid(0,120,0,12);
    }
    // Planters and poster accents.
    for (FVector P : {FVector(1405,430,0),FVector(-480,470,0)})
    {
        Block(P+FVector(0,0,30),FVector(55,55,60),Orange,true,TEXT("SM_Pot"));
        for(int32 i=0;i<9;++i)
        {
            auto* Leaf=Block(P+FVector(FMath::Sin(i*1.2f)*20,FMath::Cos(i*1.2f)*20,78+i*4),FVector(17,24,64),i%2 ? Teal : FLinearColor(.07f,.27f,.12f),false,TEXT("SM_Leaf"));
            Leaf->SetActorRotation(FRotator(12+i*3,i*47.f,18));
        }
    }
    if(!FixedSet.IsEmpty()) {
        UE_LOG(LogTemp,Display,TEXT("CE12_FIXED_SET reused=%d runtime_duplicate_decor=0"),FixedSet.Num());
        return;
    }
    auto TagLight=[](AActor* Actor) { Actor->Tags.Add(TEXT("CEArchitecture")); Actor->Tags.Add(TEXT("CE12_Lighting")); };
    auto* Sun=GetWorld()->SpawnActor<ADirectionalLight>(FVector(0,0,300),FRotator(-38,-90,0)); TagLight(Sun);
    Sun->GetLightComponent()->SetIntensity(2.8f);
    Sun->GetLightComponent()->SetLightColor(FLinearColor(1.f,.91f,.76f));
    auto* Sky=GetWorld()->SpawnActor<ASkyLight>(); TagLight(Sky);
    Sky->GetLightComponent()->SourceType=SLS_SpecifiedCubemap;
    Sky->GetLightComponent()->SetCubemap(LoadObject<UTextureCube>(nullptr,TEXT("/Game/Prototype/T_CafeReflection.T_CafeReflection")));
    Sky->GetLightComponent()->SetIntensity(.65f);
    auto* Grade=GetWorld()->SpawnActor<APostProcessVolume>(); TagLight(Grade); Grade->bUnbound=true;
    auto& PP=Grade->Settings;
    PP.bOverride_AutoExposureBias=true; PP.AutoExposureBias=.35f;
    PP.bOverride_ColorSaturation=true; PP.ColorSaturation=FVector4(.95f,.95f,.95f,1);
    PP.bOverride_AmbientOcclusionIntensity=true; PP.AmbientOcclusionIntensity=.45f;
    PP.bOverride_AmbientOcclusionRadius=true; PP.AmbientOcclusionRadius=35.f;
    PP.bOverride_BloomIntensity=true; PP.BloomIntensity=.12f;
    PP.bOverride_ColorContrast=true; PP.ColorContrast=FVector4(1.01f,1.01f,1.01f,1);
    PP.bOverride_ScreenSpaceReflectionQuality=true; PP.ScreenSpaceReflectionQuality=70.f;
    PP.bOverride_ScreenSpaceReflectionMaxRoughness=true; PP.ScreenSpaceReflectionMaxRoughness=.65f;
    for(FVector P : {FVector(-180,-120,300),FVector(330,-250,300),FVector(330,200,300),FVector(-400,350,300),FVector(1050,-250,300),FVector(1050,210,300)})
    {
        auto* Light=GetWorld()->SpawnActor<APointLight>(P,FRotator::ZeroRotator); TagLight(Light);
        auto* Point=Cast<UPointLightComponent>(Light->GetLightComponent());
        Point->SetIntensity(4600.f); Point->SetAttenuationRadius(620.f);
        Point->SetLightColor(FLinearColor(1.f,.85f,.65f)); Point->SetCastShadows(true);
        Point->SetSourceRadius(24.f); Point->SetSoftSourceRadius(20.f);
        Block(P+FVector(0,0,11),FVector(55,55,24),Deep,false,TEXT("SM09_Pendant"));
        Block(P,FVector(32,32,3),FLinearColor(1.6f,1.13f,.65f),false,TEXT("Cylinder"),true);
        Block(P+FVector(0,0,30),FVector(3,3,38),Deep,false,TEXT("SM_Bottle"));
    }
    auto Rect=[&](FVector P,FRotator R,float Width,float Height,float Intensity,FLinearColor Color) {
        auto* Light=GetWorld()->SpawnActor<ARectLight>(P,R); TagLight(Light);
        auto* C=Cast<URectLightComponent>(Light->GetLightComponent());
        C->SetSourceWidth(Width); C->SetSourceHeight(Height); C->SetIntensity(Intensity);
        C->SetAttenuationRadius(1150.f); C->SetLightColor(Color); C->SetCastShadows(true);
    };
    Rect(FVector(80,570,215),FRotator(0,-90,0),400,150,4200,FLinearColor(.82f,.9f,1.f));
    Rect(FVector(470,570,215),FRotator(0,-90,0),350,150,3600,FLinearColor(.82f,.9f,1.f));
    Rect(FVector(1100,570,215),FRotator(0,-90,0),470,150,4200,FLinearColor(.82f,.9f,1.f));
    Rect(FVector(-145,-350,285),FRotator(-45,90,0),240,65,2600,FLinearColor(1.f,.86f,.67f));
    UE_LOG(LogTemp,Display,TEXT("CE_CAFE_BUILT version=0.12 authoring=%d"),bAuthoring);
}

void ACEGameMode::TickArrivals()
{
    const float Now=GetWorld()->GetTimeSeconds(); if(Now<NextArrivalAt) return;
    // The door fits one person. Finish departures before admitting another visit.
    for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(It->bLeaving || FVector::Dist2D(It->GetActorLocation(),FVector(620,650,92))<100.f) { NextArrivalAt=Now+1.f; return; }
    bool Taken[CECafeLayout::TableCount]={};
    for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(It->SeatSlot>=0 && It->SeatSlot<CECafeLayout::TableCount) Taken[It->SeatSlot]=true;
    int32 Slot=INDEX_NONE; for(int32 i=0;i<CECafeLayout::TableCount;++i) if(!Taken[i]) { Slot=i; break; }
    if(Slot==INDEX_NONE) { NextArrivalAt=Now+3.f; return; }
    auto* ActiveCafe=GetWorld()->GetSubsystem<UCECafeSubsystem>();
    const int32 SequentialLook=CustomerSerial++%9;
    const int32 Look=Now<ActiveCafe->CampaignUntil && CustomerSerial%2==0 ? (ActiveCafe->LastMajorKind==TEXT("laptop_destroyed") ? 0 : 2) : SequentialLook;
    const int32 Archetype=Look%3;
    const FString Names[]={TEXT("CRIPTO-BRO"),TEXT("HIPSTER PURISTA"),TEXT("INFLUENCER")};
    const FLinearColor Colors[]={FLinearColor(.09f,.22f,.25f),FLinearColor(.38f,.22f,.12f),FLinearColor(.53f,.28f,.35f)};
    auto* Customer=GetWorld()->SpawnActor<ACECustomer>(FVector(620,650,92),FRotator::ZeroRotator);
    Customer->Configure(FString::Printf(TEXT("cliente-%03d"),CustomerSerial),Names[Archetype],Colors[Archetype],Archetype,Look);
    Customer->BeginVisit(Slot,static_cast<ECEDrink>((CustomerSerial-1)%3),(CustomerSerial%2)==0);
    if(PendingCompanion.IsValid() && !PendingCompanion->bLeaving) { PendingCompanion->PairWith(Customer); PendingCompanion.Reset(); }
    else if(Customer->StoryId==1 || Look==7) { Customer->SocialGroup=TEXT("reserva-")+Customer->CustomerId; PendingCompanion=Customer; }
    if(!Customer->VisitRole.IsEmpty()) Customer->DisplayName+=TEXT(" / ")+Customer->VisitRole;
    if(!IsValid(SmokeCustomer)) SmokeCustomer=Customer;
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>(); Cafe->Sound(TEXT("Door"),FVector(620,560,160));
    Cafe->Notify(TEXT("Ha entrado un cliente. Acércate y pulsa F para tomar su pedido."));
    // Possessions enter with their owner and only reach the table after seating.
    Customer->PreparePossessions(static_cast<ECEGuestDevice>((CustomerSerial-1)%4==0 ? 1 : (CustomerSerial-1)%4==1 ? 2 : (CustomerSerial-1)%4==2 ? 3 : 0));
    NextArrivalAt=Now+(PendingCompanion.IsValid() ? 4.f : FMath::Max(10.f,18.f-(Cafe->ShiftLevel-1)*1.5f));
    UE_LOG(LogTemp,Display,TEXT("CE_ARRIVAL id=%s slot=%d order=%s"),*Customer->CustomerId,Slot,*Customer->OrderText());
}

void ACEGameMode::TickTickets()
{
    const float Now=GetWorld()->GetTimeSeconds(); if(Now<NextTicketAt) return; NextTicketAt=Now+.3f;
    for(int32 i=0;i<OrderTickets.Num();++i) {
        FString Text=FString::Printf(TEXT("MESA %d\nSIN PEDIDO"),i+1);
        for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(It->SeatSlot==i && !It->bLeaving && It->bOrderKnown) {
            Text=FString::Printf(TEXT("MESA %d\n%s\n%s"),i+1,*It->OrderText().Replace(TEXT(" · "),TEXT("\n")),It->State==ECECustomerState::Eating ? TEXT("SERVIDO") : *It->SpecialRequest);
            break;
        }
        OrderTickets[i]->SetText(FText::FromString(Text));
    }
}
