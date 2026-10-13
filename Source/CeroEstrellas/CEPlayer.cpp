#include "CEGameplay.h"
#include "CEKitchen.h"
#include "CEVisual.h"
#include "CEDialogue.h"
#include "Camera/CameraComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/InputComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "GameFramework/PlayerController.h"
#include "PhysicsEngine/PhysicsHandleComponent.h"
#include "Kismet/GameplayStatics.h"
#include "Kismet/KismetSystemLibrary.h"
#include "EngineUtils.h"

ACEPlayer::ACEPlayer()
{
    PrimaryActorTick.bCanEverTick = true;
    GetCapsuleComponent()->InitCapsuleSize(28.f, 88.f);
    Camera = CreateDefaultSubobject<UCameraComponent>(TEXT("FirstPersonCamera"));
    Camera->SetupAttachment(GetCapsuleComponent());
    Camera->SetRelativeLocation(FVector(0,0,66));
    Camera->bUsePawnControlRotation = true;
    Camera->FieldOfView = 85.f;
    Handle = CreateDefaultSubobject<UPhysicsHandleComponent>(TEXT("ElasticGrip"));
    Handle->LinearStiffness = 1700.f;
    Handle->LinearDamping = 120.f;
    Handle->AngularStiffness = 1000.f;
    Handle->AngularDamping = 90.f;
    Handle->InterpolationSpeed = 12.f;
    Dialogue=CreateDefaultSubobject<UCEDialogueComponent>(TEXT("CustomerDialogue"));
    GetCharacterMovement()->MaxWalkSpeed = 230.f;
    GetCharacterMovement()->BrakingDecelerationWalking = 1500.f;
    GetCharacterMovement()->JumpZVelocity = 390.f;
    GetCharacterMovement()->AirControl = .25f;
    bUseControllerRotationYaw = true;
}

void ACEPlayer::BeginPlay()
{
    Super::BeginPlay();
    if (auto* PC = Cast<APlayerController>(GetController()))
    {
        PC->SetControlRotation(FRotator(-13,0,0));
        PC->SetInputMode(FInputModeGameOnly());
        PC->bShowMouseCursor = false;
    }
}

void ACEPlayer::SetupPlayerInputComponent(UInputComponent* Input)
{
    Super::SetupPlayerInputComponent(Input);
    Input->BindAxis(TEXT("MoveForward"), this, &ACEPlayer::Forward);
    Input->BindAxis(TEXT("MoveRight"), this, &ACEPlayer::Right);
    Input->BindAxis(TEXT("Turn"), this, &ACEPlayer::Turn);
    Input->BindAxis(TEXT("LookUp"), this, &ACEPlayer::Look);
    Input->BindAction(TEXT("Grab"), IE_Pressed, this, &ACEPlayer::Grab);
    Input->BindAction(TEXT("Throw"), IE_Pressed, this, &ACEPlayer::Throw);
    Input->BindAction(TEXT("Pour"), IE_Pressed, this, &ACEPlayer::Pour);
    Input->BindAction(TEXT("Pour"), IE_Released, this, &ACEPlayer::StopPour);
    Input->BindAction(TEXT("Interact"), IE_Pressed, this, &ACEPlayer::Interact);
    Input->BindAction(TEXT("Interact"), IE_Released, this, &ACEPlayer::ReleaseInteraction);
    Input->BindAction(TEXT("Taunt"), IE_Pressed, this, &ACEPlayer::Taunt);
    Input->BindAction(TEXT("Dialogue"), IE_Pressed, this, &ACEPlayer::Talk);
    Input->BindAction(TEXT("VoiceDialogue"), IE_Pressed, this, &ACEPlayer::TalkVoice);
    Input->BindAction(TEXT("OrderList"),IE_Pressed,this,&ACEPlayer::ShowOrders);
    Input->BindAction(TEXT("OrderList"),IE_Released,this,&ACEPlayer::HideOrders);
    Input->BindAction(TEXT("Help"),IE_Pressed,this,&ACEPlayer::ToggleHelp);
    Input->BindAction(TEXT("Sprint"), IE_Pressed, this, &ACEPlayer::SprintOn);
    Input->BindAction(TEXT("Sprint"), IE_Released, this, &ACEPlayer::SprintOff);
    Input->BindAction(TEXT("Reset"), IE_Pressed, this, &ACEPlayer::Reset);
    Input->BindAction(TEXT("Jump"), IE_Pressed, this, &ACharacter::Jump);
    Input->BindAction(TEXT("Jump"), IE_Released, this, &ACharacter::StopJumping);
    Input->BindAction(TEXT("Quit"), IE_Pressed, this, &ACEPlayer::Quit);
}

void ACEPlayer::Forward(float Value) { AddMovementInput(GetActorForwardVector(), Value); }
void ACEPlayer::Right(float Value) { AddMovementInput(GetActorRightVector(), Value); }
void ACEPlayer::Turn(float Value) { AddControllerYawInput(Value); }
void ACEPlayer::Look(float Value) { AddControllerPitchInput(Value); }
void ACEPlayer::SprintOn() { GetCharacterMovement()->MaxWalkSpeed = 440.f; }
void ACEPlayer::SprintOff() { GetCharacterMovement()->MaxWalkSpeed = 230.f; }
void ACEPlayer::StopPour() { bPouring = false; if(Held) Held->bPourRequested=false; }
void ACEPlayer::Reset() { UGameplayStatics::OpenLevel(this, TEXT("Cafeteria")); }
void ACEPlayer::Quit() { if(Dialogue->IsOpen()) { Dialogue->Close(); return; } UKismetSystemLibrary::QuitGame(this, Cast<APlayerController>(GetController()), EQuitPreference::Quit, false); }
void ACEPlayer::Talk() { if(Dialogue->IsOpen() && !Dialogue->IsEditing()) { Dialogue->EditConversation(); return; } if(auto* Customer=Cast<ACECustomer>(FocusActor(400.f))) Dialogue->Open(Customer); else GetWorld()->GetSubsystem<UCECafeSubsystem>()->Notify(TEXT("Mira a un cliente y pulsa T para Dialogar.")); }
void ACEPlayer::TalkVoice() { if(!Dialogue->IsOpen()) { auto* Customer=Cast<ACECustomer>(FocusActor(400.f)); if(!Customer || !Dialogue->Open(Customer)) return; Dialogue->ResumePlay(); } Dialogue->ToggleRecording(); }

ACEProp* ACEPlayer::FocusProp() const
{
    auto* Prop=Cast<ACEProp>(FocusActor());
    return Prop && !Prop->bBroken ? Prop : nullptr;
}

AActor* ACEPlayer::FocusActor(float Range) const
{
    FHitResult Hit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    if(Held) Params.AddIgnoredActor(Held);
    FVector ViewLocation=Camera->GetComponentLocation();
    FRotator ViewRotation=Camera->GetComponentRotation();
    if (GetController()) GetController()->GetPlayerViewPoint(ViewLocation,ViewRotation);
    GetWorld()->LineTraceSingleByChannel(Hit, ViewLocation, ViewLocation + ViewRotation.Vector()*Range, ECC_Visibility, Params);
    return Hit.GetActor();
}

void ACEPlayer::Release()
{
    Handle->ReleaseComponent();
    if (Held) { Held->bHeld = false; Held->bPourRequested=false; Held->bAimedPour=false; }
    Held = nullptr;
    bPouring = false;
}

void ACEPlayer::Grab()
{
    if (Held) { Release(); return; }
    auto* Prop = FocusProp();
    if (!Prop)
    {
        UE_LOG(LogTemp,Verbose,TEXT("CE_GRAB no_focus camera=%s rotation=%s"),*Camera->GetComponentLocation().ToString(),*Camera->GetComponentRotation().ToString());
        return;
    }
    Hold(Prop);
}

void ACEPlayer::Hold(ACEProp* Prop)
{
    if(!Prop || Prop->bBroken) return;
    if(Prop->bDelivered)
    {
        Prop->bDelivered=false; Prop->SetLifeSpan(0.f); Prop->Body->SetSimulatePhysics(true);
        GetWorld()->GetSubsystem<UCECafeSubsystem>()->Notify(TEXT("Desayuno confiscado. Otra oportunidad para empeorar el servicio."));
    }
    if(Held) Release();
    if(Prop->bStuck) { Prop->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); Prop->bStuck=false; Prop->Body->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics); Prop->Body->SetSimulatePhysics(true); Prop->SetLifeSpan(0.f); }
    if(auto* Station=Cast<ACEStation>(Prop->GetAttachParentActor()))
    {
        if(Station->WorkLeft>0.f) return;
        Station->Filter=nullptr; Station->bDoseReady=false;
        Prop->DetachFromActor(FDetachmentTransformRules::KeepWorldTransform); Prop->Body->SetSimulatePhysics(true);
    }
    Held = Prop;
    Held->bHeld = true;
    const FVector Grip = Held->Body->GetComponentLocation();
    Handle->GrabComponentAtLocationWithRotation(Held->Body, NAME_None, Grip, Held->GetActorRotation());
    UE_LOG(LogTemp, Display, TEXT("CE_GRAB name=%s"), *Held->DisplayName);
}

void ACEPlayer::Throw()
{
    ActionUntil=GetWorld()->GetTimeSeconds()+.28f;
    if (!Held)
    {
        if(auto* Prop=Cast<ACEProp>(FocusActor(230.f)))
        {
            Prop->Body->AddImpulse(Camera->GetForwardVector()*550.f,NAME_None,true);
            GetWorld()->GetSubsystem<UCECafeSubsystem>()->Sound(TEXT("Thump"),Prop->GetActorLocation(),.4f);
        }
        else if(auto* Customer=Cast<ACECustomer>(FocusActor(210.f))) Customer->Fall(Camera->GetForwardVector());
        return;
    }
    ACEProp* Prop = Held;
    Release();
    Prop->MarkThrown();
    FHitResult AimHit;
    FCollisionQueryParams Params;
    Params.AddIgnoredActor(this);
    Params.AddIgnoredActor(Prop);
    const FVector AimEnd=Camera->GetComponentLocation()+Camera->GetForwardVector()*1000.f;
    const bool bHit=GetWorld()->LineTraceSingleByChannel(AimHit,Camera->GetComponentLocation(),AimEnd,ECC_Visibility,Params);
    const FVector Delta=(bHit ? AimHit.ImpactPoint : AimEnd)-Prop->GetActorLocation();
    const float Speed=1150.f;
    FVector Velocity=Delta.GetSafeNormal()*Speed;
    Velocity.Z+=FMath::Abs(GetWorld()->GetGravityZ())*.5f*(Delta.Size()/Speed);
    Prop->Body->SetPhysicsLinearVelocity(Velocity);
    Prop->Body->AddAngularImpulseInDegrees(FVector(0,450.f,60.f), NAME_None, true);
}

void ACEPlayer::Pour()
{
    if (Held && Held->IsContainer()) { bPouring = true; Held->Pour(); }
}

void ACEPlayer::Interact()
{
    bInteractHeld=true;
    ActionUntil=GetWorld()->GetTimeSeconds()+.35f;
    AActor* Target=FocusActor(330.f);
    auto* Station=Cast<ACEStation>(Target);
    if(!Station && Target) Station=Cast<ACEStation>(Target->GetAttachParentActor());
    if(Station) { Station->Use(this); return; }
    auto* Customer=Cast<ACECustomer>(Target);
    if(!Customer) return;
    if(!Held) { Customer->TakeOrder(); return; }
    if(Held->Kind==ECEPropKind::Tray)
    {
        TArray<ACEProp*> Contents;
        for(TActorIterator<ACEProp> It(GetWorld());It;++It)
        {
            const FVector Local=Held->GetActorQuat().UnrotateVector(It->GetActorLocation()-Held->GetActorLocation());
            if(*It!=Held && !It->bDelivered && FMath::Abs(Local.X)<65.f && FMath::Abs(Local.Y)<45.f && Local.Z>0.f && Local.Z<60.f) Contents.Add(*It);
        }
        for(auto* Item:Contents) Customer->Serve(Item);
        if(Contents.IsEmpty()) GetWorld()->GetSubsystem<UCECafeSubsystem>()->Notify(TEXT("La bandeja está vacía. Coloca el desayuno encima."));
    }
    else Customer->Serve(Held);
}
void ACEPlayer::Taunt()
{
    if(auto* Customer=Cast<ACECustomer>(FocusActor(360.f)))
    {
        GetWorld()->GetSubsystem<UCECafeSubsystem>()->Notify(TEXT("TÚ: Estoy buscando mi despido, no tu aprobación.")); Customer->Taunt();
    }
}

void ACEPlayer::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    AnimateHands(DeltaSeconds);
    const float Now=GetWorld()->GetTimeSeconds(), Speed=GetVelocity().Size2D();
    const float Bob=GetCharacterMovement()->IsMovingOnGround() ? FMath::Sin(Now*(Speed>300.f ? 12.f : 8.f))*FMath::Min(Speed/180.f,1.6f) : 0.f;
    Camera->SetRelativeLocation(FVector(0,0,66.f+Bob));
    Camera->SetFieldOfView(FMath::FInterpTo(Camera->FieldOfView,Speed>300.f ? 91.f : 85.f,DeltaSeconds,3.f));
    if(Speed>80.f && GetCharacterMovement()->IsMovingOnGround() && Now>=FootstepAt)
    { FootstepAt=Now+(Speed>300.f ? .28f : .43f); GetWorld()->GetSubsystem<UCECafeSubsystem>()->Sound(TEXT("Footstep"),GetActorLocation(),.12f); }
    if(!IsValid(Held)) { Held=nullptr; return; }
    if(bPouring && Held->IsContainer())
    {
        auto* Receiver=Cast<ACEProp>(FocusActor(280.f));
        Held->bAimedPour=Receiver && Receiver->IsContainer() && Receiver->GetActorUpVector().Z>.7f;
        if(Held->bAimedPour) Held->PourTarget=Receiver->GetActorLocation()+FVector(0,0,13.f);
    }
    FVector Target = Camera->GetComponentLocation() + Camera->GetForwardVector()*(Held->Kind==ECEPropKind::Tray ? 108.f : 88.f) + Camera->GetRightVector()*(Held->Kind==ECEPropKind::Tray ? 0.f : 24.f) - FVector(0,0,30.f);
    Target.Z=FMath::Max(Target.Z,Camera->GetComponentLocation().Z-(Held->Kind==ECEPropKind::Tray ? 33.f : 18.f));
    FRotator Rotation(0, GetControlRotation().Yaw, 0);
    if (bPouring) Rotation.Roll = 120.f;
    // Running deliberately tilts the tray, letting loose cups move and spill through Chaos.
    if (Held->Kind == ECEPropKind::Tray)
    {
        Rotation.Roll = FMath::Sin(GetWorld()->GetTimeSeconds()*9.f) * FMath::Clamp(Speed/16.f, 0.f, 28.f);
        Rotation.Pitch = FMath::Sin(GetWorld()->GetTimeSeconds()*6.f)*FMath::Clamp(Speed/30.f,0.f,16.f);
    }
    if(Now<SlipUntil) Rotation.Roll+=FMath::Sin(Now*17.f)*24.f;
    if(bPouring && Held->bAimedPour)
    {
        Target=Held->PourTarget+Camera->GetRightVector()*20.f+FVector(0,0,42);
        Rotation.Roll=70.f;
    }
    Handle->SetTargetLocationAndRotation(Target, Rotation);
}
