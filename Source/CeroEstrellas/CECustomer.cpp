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
    CustomerId=Id; DisplayName=Name; Personality=FMath::Clamp(Archetype,0,2); StartPosition=GetActorLocation();
    if(!BuildRig(Outfit)) UE_LOG(LogTemp,Error,TEXT("CE_RIG_MISSING customer=%s"),*Id);
    Bubble->SetVisibility(false);
}
void ACECustomer::BeginVisit(int32 Slot,ECEDrink Drink,bool bPastry)
{
    SeatSlot=Slot; RequestedDrink=Drink; bWantsPastry=bPastry; Patience=110.f+Slot*12;
    const int32 Phase=GetWorld()->GetSubsystem<UCECafeSubsystem>()->ShiftPhase;
    if(Phase>=1 && GetTypeHash(CustomerId)%2==0) { MinServingTemperature=60.f; SpecialRequest=TEXT("Bien caliente: más de 60 C."); }
    else if(Phase>=2) { MinServingVolume=Drink==ECEDrink::Espresso ? 55.f : 170.f; SpecialRequest=Drink==ECEDrink::Espresso ? TEXT("Espresso largo: al menos 55 ml.") : TEXT("Taza bien llena: al menos 170 ml."); }
    Patience=FMath::Max(70.f,Patience-Phase*10.f);
    const float Ys[]={-100.f,220.f,-405.f};
    // A separate aisle allows later arrivals to pass customers waiting at the counter.
    Route={FVector(620,455,92),FVector(245,455,92),FVector(245,Ys[Slot],92),FVector(165,Ys[Slot],92)}; RouteIndex=0; State=ECECustomerState::Arriving;
    Speak(TEXT("Buenos días.\nNecesito desayunar."),4.f);
}
FString ACECustomer::OrderText() const
{
    const TCHAR* Drinks[]={TEXT("Espresso · 60 ml"),TEXT("Americano · café + agua"),TEXT("Latte · café + leche")};
    return FString(Drinks[static_cast<int32>(RequestedDrink)])+(bWantsPastry ? TEXT(" + cruasán") : TEXT(""));
}
void ACECustomer::Speak(const FString& Text,float Duration)
{
    Reaction=Text; FString Visible=Text;
    GetWorld()->GetSubsystem<UCECafeSubsystem>()->Subtitle(CustomerId,DisplayName,Text,Duration);
    if(Text.Len()>75 && !Text.Contains(TEXT("\n")))
    {
        TArray<FString> Words; Text.ParseIntoArray(Words,TEXT(" "),true); Visible.Empty(); int32 Width=0;
        for(const auto& Word:Words) { if(Width+Word.Len()>36) { Visible+=TEXT("\n"); Width=0; } else if(!Visible.IsEmpty()) { Visible+=TEXT(" "); ++Width; } Visible+=Word; Width+=Word.Len(); }
    }
    Bubble->SetText(FText::FromString(Visible)); ReactionUntil=GetWorld()->GetTimeSeconds()+Duration;
    Bubble->SetTextRenderColor(Anger>.4f ? FColor(255,140,90) : FColor(255,231,179)); GestureUntil=GetWorld()->GetTimeSeconds()+2.f;
}
void ACECustomer::ApplyDialogue(const FString& Text,const FString& Emotion,float AngerChange,bool bWantsToLeave)
{
    if(bLeaving) return;
    DialogueEmotion=Emotion;
    Anger=FMath::Clamp(Anger+FMath::Clamp(AngerChange,-.1f,.3f),0.f,1.2f);
    Speak(Text,10.f);
    // Unreal confirms the departure and the existing boss cooldown; the LLM cannot award points.
    if(bWantsToLeave && Anger>=1.f) DialogueDepartureAt=GetWorld()->GetTimeSeconds()+9.f;
}
void ACECustomer::TakeOrder()
{
    if(bLeaving) return;
    if(State!=ECECustomerState::Ordering)
    {
        if(bOrderKnown) Speak(OrderText(),5.f); return;
    }
    bOrderKnown=true; ++GetWorld()->GetSubsystem<UCECafeSubsystem>()->OrdersTaken;
    Speak(OrderText()+TEXT("\n")+(SpecialRequest.IsEmpty() ? TEXT("Y que esté caliente, por favor.") : SpecialRequest),7.f);
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
        const bool bHot=Item->Temperature>MinServingTemperature && Item->VolumeMl()>=MinServingVolume;
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
    if(Item->Kind==ECEPropKind::Cup) { ServedCup=Item; ServedCupPosition=Item->GetActorLocation(); }
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
    ImmediateVoice(Kind==TEXT("coffee_spill") ? TEXT("spill") : TEXT("hit"));
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
    NextWitnessAt=Now+8.f; ++WitnessCount;
    ImmediateVoice(TEXT("witness"));
    if(Personality==2 && Now>=FilmingUntil) { FilmingUntil=Now+8.f; ++GetWorld()->GetSubsystem<UCECafeSubsystem>()->FilmedDisasters;
        GetWorld()->GetSubsystem<UCECafeSubsystem>()->Observe(TEXT("disaster_filmed"),CustomerId,TEXT("Directo del desayuno"),.35f); }
     Anger=FMath::Min(.8f,Anger+.08f);
    const TCHAR* Lines[]={TEXT("Voy a vender acciones de este local.\n¡Esto es una burbuja de café!"),TEXT("La violencia del servicio\ninterpela al espectador."),TEXT("¡No cortéis! Esto se hace viral.\nHashtag: desayuno traumático.")};
    Speak(Lines[(Personality+WitnessCount-1)%3],4.f);
}
void ACECustomer::Fall(const FVector& Direction)
{
    const float Now=GetWorld()->GetTimeSeconds(); if(Now<FallenUntil+1.f) return;
    bSeated=false; FallenUntil=Now+3.f; LastSlipAt=Now;
    ImmediateVoice(TEXT("fall"));
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>(); ++Cafe->Falls;
    Anger+=.25f; Speak(TEXT("¡EL SUELO NO FORMABA\nPARTE DEL DESAYUNO!"),3.f);
    Cafe->Observe(TEXT("customer_fallen"),CustomerId,TEXT("Caída aparatosa"),.55f); Cafe->Sound(TEXT("Thump"),GetActorLocation());
    for(TActorIterator<ACEProp> It(GetWorld());It;++It)
        if(!It->bHeld && It->Body->IsSimulatingPhysics() && FVector::Dist(It->GetActorLocation(),GetActorLocation())<150.f)
            {
                It->Body->AddImpulse(Direction.GetSafeNormal2D()*260.f+FVector(0,0,130),NAME_None,true);
                It->MarkThrown();
                if(Now-LastChainAt>6.f) { LastChainAt=Now; ++Cafe->ChainReactions; Cafe->Observe(TEXT("chain_reaction"),CustomerId,TEXT("Caída desplaza el desayuno"),.45f); }
            }
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
void ACECustomer::Animate(float DeltaSeconds,bool bWalking) { AnimateRig(DeltaSeconds,bWalking); }
void ACECustomer::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    const float Now=GetWorld()->GetTimeSeconds(); bool bWalking=false;
    if(bSeated && !bLeaving && Anger>.3f && Now>NextProtectAt) if(auto* Player=Cast<ACEPlayer>(UGameplayStatics::GetPlayerPawn(this,0)))
        if(Player->bPouring && FVector::Dist2D(Player->GetActorLocation(),GetActorLocation())<210.f) {
            NextProtectAt=Now+12.f; ProtectUntil=Now+1.2f;
            for(TActorIterator<ACEProp> It(GetWorld());It;++It) if(It->Kind==ECEPropKind::Laptop && !It->bHeld && !It->bBroken && FVector::Dist2D(It->GetActorLocation(),GetActorLocation())<170.f) {
                It->Body->AddImpulse(FVector(0,Personality%2 ? -55.f : 55.f,0),NAME_None,true); Speak(TEXT("¡Aparta eso de mi portátil!"),3.f); break;
            }
        }
    Bubble->SetVisibility(false);
    if(Now<DialogueHoldUntil && !bSeated && !bLeaving)
        if(auto* Player=UGameplayStatics::GetPlayerPawn(this,0)) SetActorRotation(FMath::RInterpTo(GetActorRotation(),FRotator(0,(Player->GetActorLocation()-GetActorLocation()).Rotation().Yaw-180,0),DeltaSeconds,4));
    if(DialogueDepartureAt>0 && Now>=DialogueDepartureAt) { DialogueDepartureAt=0; Leave(true); }
    if(PendingVictim.IsValid())
    {
        if(Now>=WitnessRetryUntil) { PendingVictim.Reset(); WitnessRetryUntil=0.f; }
        else if(Now>=NextWitnessAttemptAt) { NextWitnessAttemptAt=Now+.2f; Witness(PendingVictim.Get()); }
    }
    if(Now>=DialogueHoldUntil && (State==ECECustomerState::Ordering || State==ECECustomerState::Waiting || State==ECECustomerState::WalkingToTable))
    {
        Patience=FMath::Max(0.f,Patience-DeltaSeconds);
        if(Patience<40.f && Now>NextComplaintAt) { NextComplaintAt=Now+18.f; Speak(TEXT("¿Mi desayuno viene andando?\nTengo cosas importantes que hacer.")); }
        if(Patience<=0.f) { Anger=.7f; Speak(TEXT("No pienso esperar más.")); Leave(false); }
    }
    if(State==ECECustomerState::Eating && Now>=EatUntil && Now>=DialogueHoldUntil) { Speak(TEXT("Bueno, me voy.\nHasta otro día.")); Leave(false); }
    if(bSeated && (!IsValid(Seat) || FVector::Dist2D(Seat->GetActorLocation(),GetActorLocation())>65.f || Seat->GetActorUpVector().Z<.55f)) Fall(FVector(1,0,0));
    if(State==ECECustomerState::Waiting && !Seat && Now>FallenUntil)
    {
        for(TActorIterator<ACEProp> It(GetWorld());It;++It)
            if(It->Kind==ECEPropKind::Chair && FVector::Dist2D(It->GetActorLocation(),GetActorLocation())<85.f && It->GetActorUpVector().Z>.7f)
            { Seat=*It; bSeated=true; SetActorLocation(FVector(It->GetActorLocation().X-10.f,It->GetActorLocation().Y,92.f)); break; }
    }
    if(RouteIndex<Route.Num() && Now>=FallenUntil && (bLeaving || Now>=DialogueHoldUntil) && (!bLeaving || Now>=ExitStartAt))
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
            FVector Direction=Delta.GetSafeNormal();
            for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(*It!=this) {
                FVector Away=GetActorLocation()-It->GetActorLocation(); Away.Z=0;
                const float Distance=Away.Size();
                if(Distance<85.f && Distance>1.f && FVector::DotProduct(Direction,Away)<0) {
                    // Pass to the side; repulsion along the path makes opposing
                    // pedestrians reverse forever instead of crossing each other.
                    FVector Side=FVector::CrossProduct(Direction,FVector::UpVector).GetSafeNormal();
                    if(FVector::DotProduct(Side,Away)<-.01f) Side*=-1.f;
                    Direction=(Direction+Side*(85.f-Distance)/18.f).GetSafeNormal();
                }
            }
            FHitResult Obstacle;
            SetActorLocation(GetActorLocation()+Direction*FMath::Min(Speed*DeltaSeconds,Delta.Size()),true,&Obstacle);
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
        // Orders already live in the HUD; reserve world text for actual speech.
        Bubble->SetText(FText::FromString(State==ECECustomerState::Ordering ? TEXT("[ F ] TOMAR PEDIDO") : TEXT("")));
    }
    Animate(DeltaSeconds,bWalking);
}
