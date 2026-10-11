#include "CEGameplay.h"
#include "CEVisual.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "EngineUtils.h"

ACECustomer::ACECustomer()
{
    PrimaryActorTick.bCanEverTick=true;
    Collision=CreateDefaultSubobject<UCapsuleComponent>(TEXT("CustomerCollision")); Collision->InitCapsuleSize(29.f,88.f);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); Collision->SetCollisionObjectType(ECC_WorldDynamic);
    Collision->SetCollisionResponseToAllChannels(ECR_Block); SetRootComponent(Collision);
    Bubble=CreateDefaultSubobject<UTextRenderComponent>(TEXT("SpeechBubble")); Bubble->SetupAttachment(Collision);
    Bubble->SetRelativeLocation(FVector(0,0,98)); Bubble->SetHorizontalAlignment(EHTA_Center);
    Bubble->SetWorldSize(8.f); Bubble->SetTextRenderColor(FColor(255,229,183));
}
void ACECustomer::Configure(const FString& Id,const FString& Name,const FLinearColor& Outfit,int32 Archetype)
{
    CustomerId=Id; Personality=Archetype; StartPosition=GetActorLocation();
    VisualRoot=NewObject<USceneComponent>(this); AddInstanceComponent(VisualRoot);
    VisualRoot->SetupAttachment(Collision); VisualRoot->RegisterComponent();
    const FLinearColor Skin(.65f+.06f*Archetype,.40f+.04f*Archetype,.25f+.035f*Archetype), Dark(.025f,.035f,.038f);
    auto Pivot=[this](FVector Position)
    {
        auto* P=NewObject<USceneComponent>(this); AddInstanceComponent(P); P->SetupAttachment(VisualRoot); P->SetRelativeLocation(Position); P->RegisterComponent(); return P;
    };
    auto Part=[this](USceneComponent* Parent,const TCHAR* Shape,FVector P,FVector Scale,FLinearColor Color)
    {
        auto* Mesh=CEVisual::Part(this,Shape,FVector::ZeroVector,Scale,Color);
        Mesh->AttachToComponent(Parent,FAttachmentTransformRules::KeepRelativeTransform); Mesh->SetRelativeLocation(P); return Mesh;
    };
    CEVisual::Part(this,TEXT("SM_Torso"),FVector(0,0,12),FVector(.42f,.51f,.59f),Outfit);
    CEVisual::Part(this,TEXT("Sphere"),FVector(0,0,-18),FVector(.38f,.42f,.22f),Dark);
    for(int32 Button=0;Button<3;++Button) CEVisual::Part(this,TEXT("Sphere"),FVector(-17.5f,0,8+Button*9),FVector(.018f,.018f,.018f),FLinearColor(.78f,.7f,.55f));
    CEVisual::Part(this,TEXT("Cylinder"),FVector(0,0,42),FVector(.14f,.14f,.15f),Skin);
    HeadPivot=Pivot(FVector(0,0,64));
    Part(HeadPivot,TEXT("SM_Head"),FVector::ZeroVector,FVector(.35f,.31f,.43f),Skin);
    Part(HeadPivot,TEXT("Sphere"),FVector(4,0,15),FVector(.34f,.33f,.16f),Dark);
    Part(HeadPivot,TEXT("Sphere"),FVector(-18,0,0),FVector(.07f,.08f,.10f),Skin*.86f);
    for(int32 Side=0;Side<2;++Side)
    {
        const float Sign=Side==0 ? -1.f : 1.f;
        Part(HeadPivot,TEXT("Sphere"),FVector(0,Sign*16,0),FVector(.07f,.045f,.10f),Skin);
        auto* Eye=Part(HeadPivot,TEXT("Sphere"),FVector(-15.1f,Sign*6.7f,5),FVector(.038f,.065f,.055f),FLinearColor(.9f,.89f,.83f)); Eyes[Side]=Eye;
        Pupils[Side]=Part(HeadPivot,TEXT("Sphere"),FVector(-17,Sign*6.7f,5),FVector(.017f,.025f,.033f),Dark);
        Brows[Side]=Part(HeadPivot,TEXT("Cube"),FVector(-16,Sign*6.7f,10),FVector(.015f,.07f,.016f),Dark);
        ArmPivots[Side]=Pivot(FVector(0,Sign*26,31));
        Part(ArmPivots[Side],TEXT("Sphere"),FVector(0,0,-13),FVector(.14f,.14f,.34f),Outfit);
        Part(ArmPivots[Side],TEXT("Cylinder"),FVector(-1,0,-34),FVector(.10f,.10f,.23f),Skin);
        Part(ArmPivots[Side],TEXT("Sphere"),FVector(-1,0,-48),FVector(.10f,.085f,.13f),Skin);
        LegPivots[Side]=Pivot(FVector(0,Sign*12,-22));
        Part(LegPivots[Side],TEXT("Cylinder"),FVector(0,0,-14),FVector(.15f,.16f,.29f),Dark);
        KneePivots[Side]=Pivot(FVector::ZeroVector); KneePivots[Side]->AttachToComponent(LegPivots[Side],FAttachmentTransformRules::KeepRelativeTransform);
        KneePivots[Side]->SetRelativeLocation(FVector(0,0,-28));
        Part(KneePivots[Side],TEXT("Sphere"),FVector::ZeroVector,FVector(.16f,.16f,.16f),Dark);
        Part(KneePivots[Side],TEXT("Cylinder"),FVector(0,0,-14),FVector(.135f,.14f,.29f),Dark);
        Part(KneePivots[Side],TEXT("Sphere"),FVector(-6,0,-35),FVector(.31f,.17f,.13f),FLinearColor(.08f,.045f,.028f));
    }
    Mouth=Part(HeadPivot,TEXT("Sphere"),FVector(-16,0,-9),FVector(.018f,.10f,.018f),FLinearColor(.22f,.075f,.045f));
    if(Archetype==0)
    {
        for(int32 Side:{-1,1}) Part(HeadPivot,TEXT("Cube"),FVector(-17.5f,Side*7.f,5),FVector(.015f,.095f,.055f),FLinearColor(.12f,.18f,.18f));
        CEVisual::Part(this,TEXT("Cube"),FVector(-22,0,18),FVector(.02f,.07f,.28f),FLinearColor(.75f,.18f,.08f));
    }
    if(Archetype==1)
    {
        Part(HeadPivot,TEXT("Cylinder"),FVector(0,0,25),FVector(.45f,.42f,.045f),FLinearColor(.12f,.055f,.023f));
        Part(HeadPivot,TEXT("Cylinder"),FVector(0,0,30),FVector(.32f,.31f,.10f),FLinearColor(.2f,.1f,.04f));
        Part(HeadPivot,TEXT("Sphere"),FVector(-10,0,-15),FVector(.15f,.25f,.19f),FLinearColor(.12f,.05f,.02f));
    }
    if(Archetype==2)
    {
        Part(HeadPivot,TEXT("Sphere"),FVector(14,0,-8),FVector(.25f,.39f,.51f),Dark);
        Part(ArmPivots[0],TEXT("Cube"),FVector(-3,0,-47),FVector(.025f,.09f,.17f),FLinearColor(.12f,.08f,.16f));
    }
    TArray<UStaticMeshComponent*> Meshes; GetComponents(Meshes);
    for(auto* Mesh:Meshes) if(Mesh->GetAttachParent()==Collision) Mesh->AttachToComponent(VisualRoot,FAttachmentTransformRules::KeepRelativeTransform);
    Bubble->SetText(FText::FromString(Name));
}
void ACECustomer::BeginVisit(int32 Slot,ECEDrink Drink,bool bPastry)
{
    SeatSlot=Slot; RequestedDrink=Drink; bWantsPastry=bPastry; Patience=110.f+Slot*12;
    const float Ys[]={-100.f,220.f,-405.f};
    Route={FVector(620,455,92),FVector(185,455,92),FVector(185,Ys[Slot],92)}; RouteIndex=0; State=ECECustomerState::Arriving;
    Speak(TEXT("Buenos días.\nNecesito desayunar."),4.f);
}
FString ACECustomer::OrderText() const
{
    const TCHAR* Drinks[]={TEXT("Espresso · 60 ml"),TEXT("Americano · café + agua"),TEXT("Latte · café + leche")};
    return FString(Drinks[static_cast<int32>(RequestedDrink)])+(bWantsPastry ? TEXT(" + cruasán") : TEXT(""));
}
void ACECustomer::Speak(const FString& Text,float Duration)
{
    Reaction=Text; Bubble->SetText(FText::FromString(Text)); ReactionUntil=GetWorld()->GetTimeSeconds()+Duration;
    Bubble->SetTextRenderColor(Anger>.4f ? FColor(255,140,90) : FColor(255,231,179)); GestureUntil=GetWorld()->GetTimeSeconds()+2.f;
}
void ACECustomer::TakeOrder()
{
    if(bLeaving) return;
    if(State!=ECECustomerState::Ordering)
    {
        if(bOrderKnown) Speak(OrderText(),5.f); return;
    }
    bOrderKnown=true; ++GetWorld()->GetSubsystem<UCECafeSubsystem>()->OrdersTaken;
    Speak(OrderText()+TEXT("\nY que esté caliente, por favor."),7.f);
    GetWorld()->GetSubsystem<UCECafeSubsystem>()->Notify(TEXT("Pedido anotado: ")+OrderText());
    GetWorld()->GetSubsystem<UCECafeSubsystem>()->Sound(TEXT("Order"),GetActorLocation());
    const float Ys[]={-100.f,220.f,-405.f};
    Route={FVector(185,455,92),FVector(620,455,92),FVector(620,Ys[SeatSlot],92),FVector(460,Ys[SeatSlot],92)};
    RouteIndex=0; State=ECECustomerState::WalkingToTable;
}
bool ACECustomer::Serve(ACEProp* Item)
{
    if(!Item || bLeaving || !bOrderKnown || State==ECECustomerState::Eating) return false;
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>();
    if(State!=ECECustomerState::Waiting) { Cafe->Notify(TEXT("El cliente se dirige a su mesa. Entrégale allí el desayuno.")); return false; }
    bool bAccepted=false;
    if(Item->Kind==ECEPropKind::Cup && !bDrinkServed)
    {
        const bool bHot=Item->Temperature>45.f;
        const bool bRecipe=RequestedDrink==ECEDrink::Espresso ? Item->EspressoMl>=45.f && Item->MilkMl<15.f && Item->WaterMl<25.f : RequestedDrink==ECEDrink::Americano ? Item->EspressoMl>=40.f && Item->WaterMl>=65.f && Item->MilkMl<15.f : Item->EspressoMl>=40.f && Item->MilkMl>=65.f;
        if(!bRecipe || !bHot || Item->bBroken)
        {
            ++Cafe->WrongOrders; Anger+=.3f; Speak(!bHot ? TEXT("¿Me traes esto frío?\n¿Es una prueba de paciencia?") : TEXT("No es lo que he pedido.\nQuiero hablar con la encargada."));
            Cafe->Notify(TEXT("Pedido incorrecto: revisa receta, cantidad y temperatura."));
            if(Anger>=1.f) Leave(true); return false;
        }
        bDrinkServed=true; bAccepted=true;
    }
    if(Item->Kind==ECEPropKind::Pastry && bWantsPastry && !bPastryServed) { bPastryServed=true; bAccepted=true; }
    if(!bAccepted) { Speak(TEXT("Eso no es mi pedido.")); return false; }
    if(auto* Player=Cast<ACEPlayer>(UGameplayStatics::GetPlayerPawn(this,0))) if(Player->Held==Item) Player->Release();
    Item->bDelivered=true; Item->Body->SetSimulatePhysics(false);
    const float Ys[]={-100.f,220.f,-405.f};
    Item->SetActorLocation(FVector(320, Ys[SeatSlot]+(Item->Kind==ECEPropKind::Cup ? -28.f : 25.f),Item->Kind==ECEPropKind::Cup ? 94.f : 88.f));
    Item->SetActorRotation(FRotator::ZeroRotator); Item->SetLifeSpan(18.f);
    if(bDrinkServed && (!bWantsPastry || bPastryServed))
    {
        ++Cafe->OrdersServed; State=ECECustomerState::Eating; EatUntil=GetWorld()->GetTimeSeconds()+14.f;
        Route.Reset(); Speak(TEXT("Gracias. Era tan difícil\nhacerlo bien, ¿verdad?")); Cafe->Sound(TEXT("Order"),GetActorLocation()); Cafe->Notify(TEXT("Desayuno servido. Otro día sin poder irte a casa."));
    }
    else Speak(TEXT("Bien. Ahora falta\nel resto del desayuno."));
    return true;
}
void ACECustomer::Taunt()
{
    if(bLeaving) return;
    Anger+=.18f;
    const TCHAR* Responses[]={TEXT("¿Quieres que te despidan?\nTe compro el local y lo gestiono."),TEXT("Mi paciencia está en mínimos.\n¡Voy a liquidar tu contrato!"),TEXT("¡Mi abogado acepta cripto!\nTú no aceptas ni pedidos."),TEXT("Tu desidia carece\nde una narrativa coherente."),TEXT("La amargura debe estar\nen el grano, no en el barista."),TEXT("Esta experiencia no merece\nni una nota de cata."),TEXT("Eso ha quedado grabado.\n¡Me debes una disculpa!"),TEXT("¿Sabes cuántos seguidores tengo?\n¡Hasta mi café tiene patrocinador!"),TEXT("La cancelación combina\ncon mi outfit. Prepárate.")};
    Speak(Responses[Personality*3+(TauntSerial++%3)]); if(Anger>=1.f) Leave(true);
}
void ACECustomer::React(FName Kind,const FString& ObjectName,float Severity)
{
    if(bLeaving) return;
    Anger+=Kind==TEXT("coffee_spill") ? Severity : Severity*1.3f;
    const float Now=GetWorld()->GetTimeSeconds();
    if(Now<NextReactionAt && Kind==TEXT("coffee_spill")) { if(Anger>=1.f) Leave(true); return; }
    NextReactionAt=Now+.8f; ++HitCount;
    GetWorld()->GetSubsystem<UCECafeSubsystem>()->Observe(TEXT("customer_hit"),CustomerId,ObjectName,Severity);
    for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(*It!=this && FVector::Dist2D(It->GetActorLocation(),GetActorLocation())<650.f) It->Witness(this);
    const TCHAR* Lines[]={TEXT("¡Mis inversiones!\n¡Esto va a salir muy caro!"),TEXT("¿Es una performance?\n¡He dicho sin agresiones!"),TEXT("¡Mi ropa! ¡Mi directo!\n¡Me has arruinado el día!")};
    Speak(Kind==TEXT("customer_slipped") ? TEXT("¡Me resbalo!\n¡Poned una señal!") : Lines[Personality],5.f);
    if(Severity>=.75f && Kind!=TEXT("coffee_spill")) Fall(GetActorForwardVector());
    if(Kind==TEXT("laptop_destroyed") || Anger>=1.f) Leave(true);
    UE_LOG(LogTemp,Display,TEXT("CE_CUSTOMER_REACT id=%s hits=%d anger=%.2f leaving=%d"),*CustomerId,HitCount,Anger,bLeaving);
}
void ACECustomer::Witness(ACECustomer* Victim)
{
    const float Now=GetWorld()->GetTimeSeconds();
    if(!Victim || bLeaving || Now<NextWitnessAt) return;
    FHitResult Sight; FCollisionQueryParams Q; Q.AddIgnoredActor(this); Q.AddIgnoredActor(Victim);
    if(GetWorld()->LineTraceSingleByChannel(Sight,GetActorLocation()+FVector(0,0,bSeated ? 37.f : 65.f),Victim->GetActorLocation()+FVector(0,0,65),ECC_Visibility,Q))
    {
        PendingVictim=Victim; if(WitnessRetryUntil==0.f) WitnessRetryUntil=Now+2.f;
        return;
    }
    PendingVictim.Reset(); WitnessRetryUntil=0.f;
    NextWitnessAt=Now+8.f; ++WitnessCount; Anger=FMath::Min(.8f,Anger+.08f);
    const TCHAR* Lines[]={TEXT("Voy a vender acciones de este local.\n¡Esto es una burbuja de café!"),TEXT("La violencia del servicio\ninterpela al espectador."),TEXT("¡No cortéis! Esto se hace viral.\nHashtag: desayuno traumático.")};
    Speak(Lines[(Personality+WitnessCount-1)%3],4.f);
}
void ACECustomer::Fall(const FVector& Direction)
{
    const float Now=GetWorld()->GetTimeSeconds(); if(Now<FallenUntil+1.f) return;
    bSeated=false; FallenUntil=Now+3.f; LastSlipAt=Now;
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>(); ++Cafe->Falls;
    Anger+=.25f; Speak(TEXT("¡EL SUELO NO FORMABA\nPARTE DEL DESAYUNO!"),3.f);
    Cafe->Observe(TEXT("customer_fallen"),CustomerId,TEXT("Caída aparatosa"),.55f); Cafe->Sound(TEXT("Thump"),GetActorLocation());
    for(TActorIterator<ACEProp> It(GetWorld());It;++It)
        if(!It->bHeld && It->Body->IsSimulatingPhysics() && FVector::Dist(It->GetActorLocation(),GetActorLocation())<150.f)
            It->Body->AddImpulse(Direction.GetSafeNormal2D()*160.f+FVector(0,0,80),NAME_None,true);
    for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(*It!=this && FVector::Dist2D(It->GetActorLocation(),GetActorLocation())<650.f) It->Witness(this);
    if(Anger>=1.f) Leave(true);
}
void ACECustomer::Leave(bool bDisaster)
{
    if(bLeaving) return;
    bSeated=false;
    bLeaving=true; bDisasterExit=bDisaster; State=ECECustomerState::Leaving; ExitStartAt=GetWorld()->GetTimeSeconds()+(bDisaster ? 2.f : 1.f);
    if(GetActorLocation().X<250.f) Route={FVector(185,455,92),FVector(620,455,92),FVector(620,660,92)};
    else Route={FVector(620,GetActorLocation().Y,92),FVector(620,660,92)};
    RouteIndex=0;
    if(bDisaster) Speak(TEXT("¡Me voy! ¡Esto es intolerable!\n¡Que te despidan!"),6.f);
}
void ACECustomer::Animate(float DeltaSeconds,bool bWalking)
{
    const float Now=GetWorld()->GetTimeSeconds(), Gait=FMath::Sin(Now*9.f), Talk=Now<ReactionUntil ? FMath::Abs(FMath::Sin(Now*12.f)) : 0.f;
    for(int32 i=0;i<2;++i)
    {
        const float Side=i==0 ? -1.f : 1.f;
        ArmPivots[i]->SetRelativeRotation(FRotator(bWalking ? -Gait*Side*24.f : Now<GestureUntil ? -35.f-FMath::Sin(Now*5)*20.f : -3.f,0,Anger>.5f ? Side*18.f : Side*4.f));
        LegPivots[i]->SetRelativeRotation(FRotator(bSeated ? -78.f : bWalking ? Gait*Side*26.f : 0.f,0,0));
        KneePivots[i]->SetRelativeRotation(FRotator(bSeated ? 78.f : bWalking ? FMath::Max(0.f,-Gait*Side)*22.f : 0.f,0,0));
        const bool bBlink=FMath::Fmod(Now+SeatSlot*.71f,4.3f)<.13f;
        Eyes[i]->SetRelativeScale3D(FVector(.038f,.065f,bBlink ? .008f : .055f));
        Pupils[i]->SetVisibility(!bBlink);
        Brows[i]->SetRelativeRotation(FRotator(0,0,Side*Anger*20.f));
    }
    Mouth->SetRelativeScale3D(FVector(.018f,.10f,.018f+Talk*.042f));
    if(auto* Player=UGameplayStatics::GetPlayerPawn(this,0))
    {
        const FVector Local=GetActorTransform().InverseTransformPosition(Player->GetActorLocation());
        const float Yaw=FMath::Clamp(FMath::UnwindDegrees(Local.Rotation().Yaw-180.f),-45.f,45.f);
        HeadPivot->SetRelativeRotation(FMath::RInterpTo(HeadPivot->GetRelativeRotation(),FRotator(Anger>.5f ? FMath::Sin(Now*9.f)*4.f : 0.f,Yaw,0),DeltaSeconds,4.f));
    }
    HeadPivot->SetRelativeLocation(FVector(0,0,64.f+FMath::Sin(Now*2.f)*.7f+(bWalking ? FMath::Abs(Gait)*1.5f : 0.f)));
    const bool bFallen=Now<FallenUntil;
    VisualRoot->SetRelativeRotation(FMath::RInterpTo(VisualRoot->GetRelativeRotation(),bFallen ? FRotator(78,0,18) : FRotator::ZeroRotator,DeltaSeconds,bFallen ? 9.f : 3.f));
    VisualRoot->SetRelativeLocation(FMath::VInterpTo(VisualRoot->GetRelativeLocation(),FVector(0,0,bFallen ? -63.f : bSeated ? -28.f : 0.f),DeltaSeconds,8.f));
    if(bFallen) for(int32 i=0;i<2;++i) ArmPivots[i]->SetRelativeRotation(FRotator(-100.f+FMath::Sin(Now*18.f+i)*25.f,0,(i==0 ? -1.f : 1.f)*45.f));
    if(auto* PC=UGameplayStatics::GetPlayerController(this,0))
    {
        FVector View; FRotator Rotation; PC->GetPlayerViewPoint(View,Rotation);
        Bubble->SetWorldRotation(FRotator(0,(View-Bubble->GetComponentLocation()).Rotation().Yaw,0));
    }
}
void ACECustomer::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Now=GetWorld()->GetTimeSeconds(); bool bWalking=false;
    if(PendingVictim.IsValid())
    {
        if(Now>=WitnessRetryUntil) { PendingVictim.Reset(); WitnessRetryUntil=0.f; }
        else if(Now>=NextWitnessAttemptAt) { NextWitnessAttemptAt=Now+.2f; Witness(PendingVictim.Get()); }
    }
    if(State==ECECustomerState::Ordering || State==ECECustomerState::Waiting || State==ECECustomerState::WalkingToTable)
    {
        Patience=FMath::Max(0.f,Patience-DeltaSeconds);
        if(Patience<40.f && Now>NextComplaintAt) { NextComplaintAt=Now+18.f; Speak(TEXT("¿Mi desayuno viene andando?\nTengo cosas importantes que hacer.")); }
        if(Patience<=0.f) { Anger=.7f; Speak(TEXT("No pienso esperar más.")); Leave(false); }
    }
    if(State==ECECustomerState::Eating && Now>=EatUntil) { Speak(TEXT("Bueno, me voy.\nHasta otro día.")); Leave(false); }
    if(bSeated && (!IsValid(Seat) || FVector::Dist2D(Seat->GetActorLocation(),GetActorLocation())>65.f || Seat->GetActorUpVector().Z<.55f)) Fall(FVector(1,0,0));
    if(State==ECECustomerState::Waiting && !Seat && Now>FallenUntil)
    {
        for(TActorIterator<ACEProp> It(GetWorld());It;++It)
            if(It->Kind==ECEPropKind::Chair && FVector::Dist2D(It->GetActorLocation(),GetActorLocation())<85.f && It->GetActorUpVector().Z>.7f)
            { Seat=*It; bSeated=true; SetActorLocation(FVector(It->GetActorLocation().X-10.f,It->GetActorLocation().Y,92.f)); break; }
    }
    if(RouteIndex<Route.Num() && Now>=FallenUntil && (!bLeaving || Now>=ExitStartAt))
    {
        const FVector Delta=Route[RouteIndex]-GetActorLocation();
        if(Delta.Size2D()<8.f)
        {
            ++RouteIndex;
            if(RouteIndex>=Route.Num())
            {
                SetActorRotation(FRotator::ZeroRotator);
                if(State==ECECustomerState::Arriving) { State=ECECustomerState::Ordering; Speak(TEXT("¿Me tomas nota?\nF para escuchar el pedido."),6.f); }
                else if(State==ECECustomerState::WalkingToTable) State=ECECustomerState::Waiting;
                else if(bLeaving) { GetWorld()->GetSubsystem<UCECafeSubsystem>()->ConfirmDeparture(CustomerId,bDisasterExit); Destroy(); return; }
            }
        }
        else
        {
            bWalking=true;
            const float Speed=bDisasterExit ? 185.f : 145.f;
            FHitResult Obstacle;
            SetActorLocation(GetActorLocation()+Delta.GetSafeNormal()*FMath::Min(Speed*DeltaSeconds,Delta.Size()),true,&Obstacle);
            if(Obstacle.bBlockingHit)
            {
                // Slide along furniture and other customers instead of walking through them.
                FVector Slide=FVector::VectorPlaneProject(Delta.GetSafeNormal(),Obstacle.Normal);
                if(Slide.SizeSquared()<.05f) Slide=FVector::CrossProduct(Delta.GetSafeNormal2D(),FVector::UpVector)*((GetTypeHash(CustomerId)%2)==0 ? 1.f : -1.f);
                SetActorLocation(GetActorLocation()+Slide*Speed*DeltaSeconds,true);
            }
            SetActorRotation(FMath::RInterpTo(GetActorRotation(),FRotator(0,Delta.Rotation().Yaw-180.f,0),DeltaSeconds,7.f));
        }
    }
    if(Now>ReactionUntil)
    {
        Bubble->SetText(FText::FromString(State==ECECustomerState::Ordering ? TEXT("[ F ] TOMAR PEDIDO") : bOrderKnown && State!=ECECustomerState::Leaving ? FString::Printf(TEXT("MESA %d\n%s"),SeatSlot+1,*OrderText()) : TEXT("")));
    }
    Animate(DeltaSeconds,bWalking);
}
