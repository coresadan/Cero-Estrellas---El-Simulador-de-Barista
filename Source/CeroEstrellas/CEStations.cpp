#include "CEKitchen.h"
#include "CEVisual.h"
#include "Components/StaticMeshComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/AudioComponent.h"
#include "Sound/SoundBase.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

namespace {
FString StationLabel(const ACEStation& Station)
{
    if(Station.WorkLeft>0 && Station.Kind==ECEStationKind::Espresso)
        return FString::Printf(TEXT("9 BAR  /  %.1f s"),Station.WorkLeft);
    const FString Full=Station.Status(); int32 Newline;
    return Full.FindChar(TEXT('\n'),Newline) ? Full.Left(Newline) : Full;
}
}

ACEStation::ACEStation()
{
    PrimaryActorTick.bCanEverTick=true;
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("StationBody")); SetRootComponent(Body);
    Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); Body->SetCollisionResponseToAllChannels(ECR_Block);
    Body->SetCollisionObjectType(ECC_WorldStatic);
    Screen=CreateDefaultSubobject<UTextRenderComponent>(TEXT("Instructions")); Screen->SetupAttachment(Body);
    Screen->SetWorldSize(3.6f); Screen->SetHorizontalAlignment(EHTA_Center); Screen->SetTextRenderColor(FColor(210,227,209));
    Screen->SetVerticalAlignment(EVRTA_TextCenter);
    WorkingSound=CreateDefaultSubobject<UAudioComponent>(TEXT("ApplianceSound")); WorkingSound->SetupAttachment(Body);
    WorkingSound->bAutoActivate=false; WorkingSound->SetVolumeMultiplier(.35f);
    WorkingSound->bOverrideAttenuation=true;
    WorkingSound->AttenuationOverrides.bAttenuate=true;
    WorkingSound->AttenuationOverrides.bSpatialize=true;
    WorkingSound->AttenuationOverrides.AttenuationShapeExtents=FVector(120.f);
    WorkingSound->AttenuationOverrides.FalloffDistance=700.f;
}
void ACEStation::Configure(ECEStationKind InKind)
{
    Kind=InKind;
    Body->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Prototype/Meshes/SM_BevelCube.SM_BevelCube")));
    const FVector Size=Kind==ECEStationKind::Tamper ? FVector(38,42,12) : Kind==ECEStationKind::Espresso ? FVector(145,65,66) : Kind==ECEStationKind::Grinder ? FVector(38,44,70) : Kind==ECEStationKind::Milk || Kind==ECEStationKind::Water ? FVector(42,48,64) : Kind==ECEStationKind::Trays ? FVector(75,80,30) : Kind==ECEStationKind::Bin ? FVector(50,50,70) : FVector(70,70,25);
    Body->SetWorldScale3D(Size/100.f);
    Body->SetMaterial(0,CEVisual::Material(this,Kind==ECEStationKind::Bin ? FLinearColor(.12f,.18f,.17f) : FLinearColor(.16f,.22f,.22f),false,Kind==ECEStationKind::Espresso || Kind==ECEStationKind::Grinder || Kind==ECEStationKind::Milk || Kind==ECEStationKind::Water));
    auto Add=[this](const TCHAR* S,FVector P,FVector Scale,FLinearColor C,bool bGlow=false)
    {
        return CEVisual::Part(this,S,P/Body->GetComponentScale(),Scale/Body->GetComponentScale(),C,bGlow);
    };
    const FLinearColor Steel(.55f,.6f,.58f), Black(.018f,.025f,.027f), Orange(.91f,.32f,.08f);
    Screen->SetRelativeScale3D(FVector::OneVector/Body->GetComponentScale());
    Screen->SetRelativeLocation(FVector(0,Size.Y/2+1,Size.Z/2-5)/Body->GetComponentScale());
    Screen->SetRelativeRotation(FRotator(0,90,0));
    if(Kind==ECEStationKind::Espresso)
    {
        Add(TEXT("Cube"),FVector(0,34.8f,26),FVector(.89f,.015f,.12f),Black);
        Screen->SetRelativeLocation(FVector(0,36.f,29)/Body->GetComponentScale());
        Add(TEXT("Cube"),FVector(0,34,-9),FVector(1.27f,.02f,.34f),Black);
        Add(TEXT("Cube"),FVector(0,49,-31),FVector(1.4f,.40f,.035f),Steel);
        for(int32 i=-4;i<=4;++i) Add(TEXT("Cube"),FVector(i*14,50,-28),FVector(.01f,.33f,.01f),Black);
        Add(TEXT("Cylinder"),FVector(0,46,7),FVector(.27f,.27f,.12f),Steel);
        Add(TEXT("Cylinder"),FVector(-5,46,-4),FVector(.04f,.04f,.12f),Steel);
        Add(TEXT("Cylinder"),FVector(5,46,-4),FVector(.04f,.04f,.12f),Steel);
        auto* Gauge=Add(TEXT("Cylinder"),FVector(-46,34,11),FVector(.18f,.18f,.02f),FLinearColor(.86f,.81f,.68f)); Gauge->SetRelativeRotation(FRotator(0,0,90));
        auto* Needle=Add(TEXT("Cube"),FVector(-46,35,13),FVector(.01f,.01f,.07f),Orange); Needle->SetRelativeRotation(FRotator(15,0,0));
        for(int32 i=0;i<3;++i) Add(TEXT("Sphere"),FVector(30+i*14,34,12),FVector(.06f,.035f,.06f),i==0 ? Orange : Steel,true);
        Add(TEXT("Cube"),FVector(0,0,36),FVector(1.4f,.58f,.025f),Steel);
        for(int32 Side:{-1,1})
        {
            Add(TEXT("Cube"),FVector(Side*70.f,0,0),FVector(.035f,.64f,.64f),FLinearColor(.035f,.14f,.12f));
            Add(TEXT("Cube"),FVector(Side*61.f,34,0),FVector(.025f,.025f,.57f),Steel);
            Add(TEXT("SM_Wand"),FVector(Side*56.f,37,-6),FVector(1),Steel);
            Add(TEXT("SM_SteamKnob"),FVector(Side*57.f,39,25),FVector(.7f,.7f,1.f),Black)->SetRelativeRotation(FRotator(0,0,90));
        }
        for(int32 i=-5;i<=5;++i) Add(TEXT("Cube"),FVector(i*11.f,-19,37.7f),FVector(.02f,.2f,.012f),Black);
    }
    if(Kind==ECEStationKind::Tamper)
    {
        MovingTool=Add(TEXT("Cylinder"),FVector(0,0,12),FVector(.15f,.15f,.045f),Steel); ToolRestZ=MovingTool->GetRelativeLocation().Z;
        Add(TEXT("Cylinder"),FVector(0,0,20),FVector(.055f,.055f,.15f),Black);
        Add(TEXT("Sphere"),FVector(0,0,29),FVector(.13f,.13f,.10f),FLinearColor(.3f,.12f,.035f));
    }
    if(Kind==ECEStationKind::Grinder)
    {
        Add(TEXT("Cylinder"),FVector(0,0,46),FVector(.30f,.30f,.22f),FLinearColor(.24f,.1f,.025f));
        Add(TEXT("SM_Hopper"),FVector(0,0,27),FVector(.33f,.33f,.23f),Steel);
        Add(TEXT("Cylinder"),FVector(0,0,60),FVector(.32f,.32f,.03f),Black);
        Add(TEXT("Cube"),FVector(0,26,0),FVector(.13f,.18f,.055f),Steel);
        for(int32 Side:{-1,1}) Add(TEXT("Cube"),FVector(Side*9,27,-20),FVector(.025f,.23f,.025f),Steel);
    }
    if(Kind==ECEStationKind::Milk || Kind==ECEStationKind::Water)
    {
        Add(TEXT("Cylinder"),FVector(0,29,2),FVector(.05f,.05f,.40f),Steel);
        Add(TEXT("Cube"),FVector(0,33,-15),FVector(.045f,.16f,.045f),Steel);
        MovingTool=Add(TEXT("Sphere"),FVector(0,26,23),FVector(.12f,.07f,.12f),Kind==ECEStationKind::Milk ? FLinearColor(.84f,.75f,.54f) : Orange);
        Add(TEXT("SM_Wand"),FVector(-12,24,-4),FVector(.8f),Steel);
        Add(TEXT("Cube"),FVector(0,40,-30),FVector(.39f,.29f,.025f),Steel);
    }
    if(Kind==ECEStationKind::Sugar || Kind==ECEStationKind::Syrup || Kind==ECEStationKind::Frother) {
        Add(Kind==ECEStationKind::Syrup ? TEXT("SM_Bottle") : TEXT("Cylinder"),FVector(0,0,20),FVector(.30f,.30f,.36f),Kind==ECEStationKind::Syrup ? FLinearColor(.37f,.14f,.03f) : Steel);
        MovingTool=Add(TEXT("Cylinder"),FVector(0,0,43),FVector(.07f,.07f,.08f),Black); ToolRestZ=MovingTool->GetRelativeLocation().Z;
        Add(TEXT("Cube"),FVector(0,14,44),FVector(.04f,.28f,.03f),Steel);
    }
    if(Kind==ECEStationKind::Cups || Kind==ECEStationKind::Trays || Kind==ECEStationKind::Pastries)
    {
        for(int32 Side:{-1,1}) Add(TEXT("Cube"),FVector(Side*(Size.X/2-2),0,18),FVector(.035f,Size.Y/100.f,.60f),Steel);
        Screen->SetRelativeLocation(FVector(0,Size.Y/2+1,6)/Body->GetComponentScale());
    }
    Screen->SetText(FText::FromString(StationLabel(*this)));
}
FVector ACEStation::CupPosition() const
{
    return GetActorLocation()+FVector(0,Kind==ECEStationKind::Espresso ? 49.f : 40.f,Kind==ECEStationKind::Espresso ? -16.f : -16.f);
}
ACEProp* ACEStation::CupUnderSpout() const
{
    for(TActorIterator<ACEProp> It(GetWorld());It;++It)
        if((It->Kind==ECEPropKind::Cup || (Kind==ECEStationKind::Milk && It->Kind==ECEPropKind::Pitcher)) && !It->bHeld && !It->bBroken && FVector::Dist2D(It->GetActorLocation(),CupPosition())<19.f && FMath::Abs(It->GetActorLocation().Z-CupPosition().Z)<18.f && It->GetActorUpVector().Z>.7f) return *It;
    return nullptr;
}
FString ACEStation::Status() const
{
    if(WorkLeft>0.f) return FString::Printf(TEXT("%s / %.1f s"),Kind==ECEStationKind::Grinder ? TEXT("MOLIENDO") : Kind==ECEStationKind::Espresso ? TEXT("EXTRAYENDO / 9 BAR") : TEXT("CALENTANDO"),WorkLeft);
    switch(Kind)
    {
    case ECEStationKind::Espresso: return bDoseReady ? TEXT("DOSIS LISTA / COLOCA TAZA\nF: EXTRAER 60 ml") : TEXT("GRUPO / 93 C\nINSERTA PORTAFILTRO DOSIFICADO");
    case ECEStationKind::Grinder: return FString::Printf(TEXT("MOLINILLO / %d DOSIS\nF: MOLER EN PORTAFILTRO"),DosesLeft);
    case ECEStationKind::Milk: return TEXT("LANZA DE VAPOR\nJARRA → CALENTAR → VERTER");
    case ECEStationKind::Tamper: return TEXT("PRENSADO / 18 g\nMANTÉN F: COMPACTAR DOSIS");
    case ECEStationKind::Sugar: return TEXT("AZÚCAR / DOSIS 2 g\nF: ENDULZAR TAZA");
    case ECEStationKind::Syrup: return TEXT("SIROPE / DOSIS 6 ml\nF: BOMBEAR EN TAZA");
    case ECEStationKind::Frother: return TEXT("ESPUMADOR\nF: BATIR LECHE CALIENTE");
    case ECEStationKind::Water: return TEXT("AGUA CALIENTE\nF: +100 ml / 85 C");
    case ECEStationKind::Cups: return TEXT("TAZAS LIMPIAS\nF: COGER NUEVA");
    case ECEStationKind::Trays: return TEXT("BANDEJAS\nF: COGER NUEVA");
    case ECEStationKind::Pastries: return TEXT("CRUASANES\nF: COGER NUEVO");
    default: return TEXT("RESIDUOS\nF: TIRAR OBJETO");
    }
}
FString ACEStation::Prompt(const ACEPlayer* Player) const
{
    if(Kind==ECEStationKind::Milk && !Player->Held && WorkLeft<=0.f && !CupUnderSpout()) return TEXT("[ F ] Coger jarra con 120 ml de leche fría");
    if(Kind==ECEStationKind::Milk && Player->Held && Player->Held->Kind==ECEPropKind::Pitcher) return TEXT("[ F ] Colocar jarra bajo la lanza · F otra vez calentar");
    if(WorkLeft>0.f) return Kind==ECEStationKind::Tamper ? TEXT("Mantén F: presión · suelta para interrumpir") : Kind==ECEStationKind::Espresso ? TEXT("F: detener extracción · E: retirar taza") : TEXT("En marcha · E: retirar jarra");
    if(Kind==ECEStationKind::Espresso && Player->Held && Player->Held->Kind==ECEPropKind::Portafilter) return TEXT("[ F ] Acoplar portafiltro / preparar extracción");
    if((Kind==ECEStationKind::Espresso || Kind==ECEStationKind::Milk || Kind==ECEStationKind::Water) && Player->Held && Player->Held->Kind==ECEPropKind::Cup) return TEXT("[ F ] Colocar taza bajo el surtidor");
    if(Kind==ECEStationKind::Grinder && !Player->Held) return DosesLeft==0 ? TEXT("[ F ] Rellenar tolva de granos") : TEXT("[ F ] Coger portafiltro limpio");
    if(Kind==ECEStationKind::Espresso && !bDoseReady && IsValid(Filter)) return TEXT("[ F ] Retirar portafiltro usado · volver a moler");
    FString OneLine=Status(); OneLine.ReplaceInline(TEXT("\n"),TEXT(" · ")); return TEXT("[ F ] ")+OneLine;
}
void ACEStation::PlaceCup(ACEPlayer* Player)
{
    auto* Cup=Player->Held.Get(); Player->Release();
    Cup->Body->SetPhysicsLinearVelocity(FVector::ZeroVector); Cup->Body->SetPhysicsAngularVelocityInDegrees(FVector::ZeroVector);
    Cup->SetActorLocationAndRotation(CupPosition(),FRotator::ZeroRotator, false,nullptr,ETeleportType::TeleportPhysics);
    GetWorld()->GetSubsystem<UCECafeSubsystem>()->Notify(TEXT("Taza colocada. Pulsa F otra vez para abrir el surtidor."));
}
void ACEStation::Use(ACEPlayer* Player)
{
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>();
    if(!Player) return;
    if(WorkLeft>0.f) {
        if(Kind==ECEStationKind::Espresso) { WorkLeft=0; WorkingSound->Stop(); bDoseReady=false; if(IsValid(Filter)) { Filter->bGroundDose=false; Filter->bTamped=false; } Cafe->Notify(FString::Printf(TEXT("Extracción detenida: %.0f ml. Puedes provocar un café corto."),ExtractedMl)); }
        return;
    }
    if(Kind==ECEStationKind::Sugar || Kind==ECEStationKind::Syrup || Kind==ECEStationKind::Frother) {
        auto* Cup=Player->Held.Get();
        if(!Cup || !Cup->IsContainer() || Cup->VolumeMl()<1) { Cafe->Notify(TEXT("Acerca una taza o jarra con líquido y pulsa F.")); return; }
        if(Kind==ECEStationKind::Sugar) { Cup->SugarGrams=FMath::Min(20.f,Cup->SugarGrams+2.f); ++Dispensed; WorkLeft=.35f; Cafe->Notify(TEXT("Azúcar añadido: 2 g.")); }
        else if(Kind==ECEStationKind::Syrup) { const float Added=Cup->AddLiquid(0,0,6,20); Cup->SyrupMl+=Added; ++Dispensed; WorkLeft=.35f; Cafe->Notify(Added>0 ? TEXT("Una pulsación de sirope.") : TEXT("La taza está llena: vierte un poco primero.")); }
        else {
            if(Cup->MilkMl<30 || Cup->Temperature<50) { Cafe->Notify(TEXT("La espuma necesita leche caliente: jarra → vapor → espumador.")); return; }
            WorkingCup=Cup; WorkLeft=1.5f; WorkingSound->SetSound(LoadObject<USoundBase>(nullptr,TEXT("/Game/Prototype/Audio/FX/Steam.Steam"))); WorkingSound->Play();
        }
        Cup->UpdateLiquidVisual(); Cafe->Sound(TEXT("Click"),GetActorLocation()); return;
    }
    if(Kind==ECEStationKind::Tamper)
    {
        if(!Player->Held || Player->Held->Kind!=ECEPropKind::Portafilter || !Player->Held->bGroundDose)
        { Cafe->Notify(TEXT("Muele una dosis y trae el portafiltro al prensador.")); return; }
        Filter=Player->Held; Player->Release(); Filter->Body->SetSimulatePhysics(false);
        Filter->SetActorLocationAndRotation(GetActorLocation()+FVector(0,0,6),FRotator::ZeroRotator);
        Filter->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform); TampPressure=0; WorkLeft=.55f;
        Cafe->Sound(TEXT("Thump"),GetActorLocation(),.3f); Cafe->Notify(TEXT("Prensando la dosis...")); return;
    }
    if(Kind==ECEStationKind::Milk)
    {
        if(Player->Held && Player->Held->Kind==ECEPropKind::Pitcher) { PlaceCup(Player); return; }
        if(Player->Held) { Cafe->Notify(TEXT("La lanza calienta la jarra. Vierte luego la leche en el espresso con clic derecho.")); return; }
        WorkingCup=CupUnderSpout();
        if(WorkingCup && WorkingCup->Kind==ECEPropKind::Pitcher)
        {
            WorkLeft=2.5f; WorkingSound->SetSound(LoadObject<USoundBase>(nullptr,TEXT("/Game/Prototype/Audio/FX/Steam.Steam"))); WorkingSound->Play(); return;
        }
        int32 Count=0; for(TActorIterator<ACEProp> It(GetWorld());It;++It) ++Count;
        if(Count>=48) { Cafe->Notify(TEXT("Retira objetos antes de sacar más jarras.")); return; }
        auto* Jar=GetWorld()->SpawnActor<ACEProp>(CupPosition(),FRotator::ZeroRotator);
        Jar->Configure(ECEPropKind::Pitcher); Jar->AddLiquid(0,120,0,8); Player->Hold(Jar); ++Dispensed; return;
    }
    if(Kind==ECEStationKind::Bin)
    {
        if(Player->Held) { auto* Trash=Player->Held.Get(); Player->Release(); Trash->Destroy(); Cafe->Notify(TEXT("Residuos retirados. Dimisión todavía pendiente.")); } return;
    }
    if(Kind==ECEStationKind::Cups || Kind==ECEStationKind::Trays || Kind==ECEStationKind::Pastries)
    {
        if(Player->Held) { Cafe->Notify(TEXT("Suelta el objeto que llevas antes de coger otro.")); return; }
        int32 Count=0; for(TActorIterator<ACEProp> It(GetWorld());It;++It) ++Count;
        if(Count>=48) { Cafe->Notify(TEXT("Demasiados objetos fuera: lleva algunos al cubo de residuos.")); return; }
        const ECEPropKind PropKind=Kind==ECEStationKind::Cups ? ECEPropKind::Cup : Kind==ECEStationKind::Trays ? ECEPropKind::Tray : ECEPropKind::Pastry;
        auto* Item=GetWorld()->SpawnActor<ACEProp>(GetActorLocation()+FVector(0,65,35),FRotator::ZeroRotator);
        Item->Configure(PropKind); Player->Hold(Item); ++Dispensed; Cafe->Sound(TEXT("Click"),GetActorLocation()); return;
    }
    if(Kind==ECEStationKind::Grinder)
    {
        if(DosesLeft<=0) { DosesLeft=12; Cafe->Notify(TEXT("Tolva recargada: doce dosis. O doce oportunidades de desastre.")); return; }
        if(!Player->Held)
        {
            int32 Count=0; for(TActorIterator<ACEProp> It(GetWorld());It;++It) ++Count;
            if(Count>=48) { Cafe->Notify(TEXT("Retira objetos antes de sacar más portafiltros.")); return; }
            auto* NewFilter=GetWorld()->SpawnActor<ACEProp>(GetActorLocation()+FVector(0,50,0),FRotator::ZeroRotator); NewFilter->Configure(ECEPropKind::Portafilter); Player->Hold(NewFilter); return;
        }
        if(Player->Held->Kind!=ECEPropKind::Portafilter || Player->Held->bGroundDose) { Cafe->Notify(TEXT("Necesitas un portafiltro vacío para moler.")); return; }
        Filter=Player->Held; WorkLeft=1.6f;
        WorkingSound->SetSound(LoadObject<USoundBase>(nullptr,TEXT("/Game/Prototype/Audio/FX/Grinder.Grinder"))); WorkingSound->Play(); return;
    }
    if(Player->Held && Player->Held->Kind==ECEPropKind::Cup) { PlaceCup(Player); return; }
    if(Kind==ECEStationKind::Espresso && Player->Held && Player->Held->Kind==ECEPropKind::Portafilter)
    {
        if(!Player->Held->bGroundDose || !Player->Held->bTamped) { Cafe->Notify(TEXT("Muele y PRENSA la dosis antes de acoplarla. El prensador está delante de la máquina.")); return; }
        if(IsValid(Filter)) { Filter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); Filter->Destroy(); }
        Filter=Player->Held; Player->Release(); Filter->Body->SetSimulatePhysics(false);
        Filter->SetActorLocationAndRotation(GetActorLocation()+FVector(0,44,3),FRotator::ZeroRotator);
        Filter->AttachToActor(this,FAttachmentTransformRules::KeepWorldTransform); bDoseReady=true; LockLeft=.45f; Cafe->Sound(TEXT("Click"),GetActorLocation()); return;
    }
    if(Kind==ECEStationKind::Espresso && !bDoseReady)
    {
        if(IsValid(Filter) && !Player->Held) { Player->Hold(Filter); return; }
        Cafe->Notify(TEXT("Falta una dosis. Molino → portafiltro → grupo de extracción.")); return;
    }
    WorkingCup=CupUnderSpout();
    if(!WorkingCup) { Cafe->Notify(TEXT("No hay taza bajo el surtidor. También puedes extraer directamente al suelo...")); }
    WorkLeft=Kind==ECEStationKind::Espresso ? 3.2f : 2.5f; TransferAccumulator=0.f; ExtractedMl=0.f;
    WorkingSound->SetSound(LoadObject<USoundBase>(nullptr,Kind==ECEStationKind::Espresso ? TEXT("/Game/Prototype/Audio/FX/Brew.Brew") : TEXT("/Game/Prototype/Audio/FX/Steam.Steam"))); WorkingSound->Play();
}
void ACEStation::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Now=GetWorld()->GetTimeSeconds();
    if(LockLeft>0 && IsValid(Filter)) { LockLeft=FMath::Max(0.f,LockLeft-DeltaSeconds); Filter->SetActorRotation(FRotator(0,90.f*(1.f-LockLeft/.45f),0)); }
    if(MovingTool) {
        if(Kind==ECEStationKind::Milk || Kind==ECEStationKind::Water) MovingTool->SetRelativeRotation(FRotator(0,0,WorkLeft>0 ? 90.f : 0.f));
        else { FVector P=MovingTool->GetRelativeLocation(); P.Z=ToolRestZ-(WorkLeft>0 ? FMath::Sin(FMath::Clamp(WorkLeft/.55f,0.f,1.f)*PI)*6.f : 0.f)/Body->GetComponentScale().Z; MovingTool->SetRelativeLocation(P); }
    }
    if(Kind==ECEStationKind::Tamper && WorkLeft>0.f) {
        auto* Player=Cast<ACEPlayer>(UGameplayStatics::GetPlayerPawn(this,0));
        if(!Player || !Player->bInteractHeld || FVector::Dist2D(Player->GetActorLocation(),GetActorLocation())>230.f || !IsValid(Filter) || Filter->bHeld) {
            WorkLeft=0; if(IsValid(Filter) && !Filter->bHeld) { Filter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); Filter->Body->SetSimulatePhysics(true); }
            GetWorld()->GetSubsystem<UCECafeSubsystem>()->Notify(TEXT("Prensado interrumpido. Agarra el filtro y mantén F para completar."));
        }
    }
    if(WorkLeft>0.f)
    {
        const float Step=FMath::Min(DeltaSeconds,WorkLeft); WorkLeft=FMath::Max(0.f,WorkLeft-Step);
        if(Kind==ECEStationKind::Frother) {
            if(IsValid(WorkingCup)) { WorkingCup->FoamMl=FMath::Min(60.f,WorkingCup->FoamMl+Step*22.f); WorkingCup->UpdateLiquidVisual(); }
        }
        else if(Kind==ECEStationKind::Milk)
        {
            if(IsValid(WorkingCup) && CupUnderSpout()==WorkingCup)
            {
                WorkingCup->Temperature=FMath::Min(65.f,WorkingCup->Temperature+Step*25.f);
                if(WorkingCup->Temperature>50.f) WorkingCup->FoamMl=FMath::Min(20.f,WorkingCup->FoamMl+Step*8.f);
                WorkingCup->UpdateLiquidVisual();
            }
        }
        else if(Kind==ECEStationKind::Tamper) { TampPressure=FMath::Clamp(1.f-WorkLeft/.55f,0.f,1.f); }
        else if(Kind==ECEStationKind::Sugar || Kind==ECEStationKind::Syrup) { }
        else if(Kind!=ECEStationKind::Grinder)
        {
            TransferAccumulator+=Step;
            if(TransferAccumulator>=.025f || WorkLeft<=0.f)
            {
                const float Amount=TransferAccumulator*(Kind==ECEStationKind::Espresso ? 60.f/3.2f : 40.f);
                if(Kind==ECEStationKind::Espresso) ExtractedMl+=Amount;
                auto* Liquid=GetWorld()->GetSubsystem<UCECafeSubsystem>()->Liquids.Get();
                const float E=Kind==ECEStationKind::Espresso ? Amount : 0.f, M=Kind==ECEStationKind::Milk ? Amount : 0.f, W=Kind==ECEStationKind::Water ? Amount : 0.f;
                if(Liquid) Liquid->Emit(CupPosition()+FVector(0,0,24),FVector(0,0,-70),E,M,W,Kind==ECEStationKind::Espresso ? 93.f : Kind==ECEStationKind::Milk ? 65.f : 85.f,Kind==ECEStationKind::Espresso ? Filter.Get() : nullptr);
                TransferAccumulator=0.f;
            }
        }
        if(WorkLeft<=0.f)
        {
            WorkingSound->Stop();
            if(Kind==ECEStationKind::Tamper && IsValid(Filter)) { Filter->bTamped=true; Filter->UpdateLiquidVisual();
                Filter->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform);
                if(auto* Player=Cast<ACEPlayer>(UGameplayStatics::GetPlayerPawn(this,0))) { if(!Player->Held) Player->Hold(Filter); else Filter->Body->SetSimulatePhysics(true); } GetWorld()->GetSubsystem<UCECafeSubsystem>()->Notify(TEXT("Dosis prensada. Acopla el portafiltro en el grupo.")); }
            if(Kind==ECEStationKind::Grinder && IsValid(Filter)) { Filter->bGroundDose=true; Filter->bTamped=false; Filter->UpdateLiquidVisual(); --DosesLeft; }
            if(Kind==ECEStationKind::Espresso)
            {
                bDoseReady=false;
                if(IsValid(Filter)) { Filter->bGroundDose=false; Filter->bTamped=false; Filter->UpdateLiquidVisual(); }
            }
            GetWorld()->GetSubsystem<UCECafeSubsystem>()->Sound(TEXT("Click"),GetActorLocation());
        }
    }
    if(Now>=ScreenNextAt) { ScreenNextAt=Now+.2f; Screen->SetText(FText::FromString(StationLabel(*this))); }
}
