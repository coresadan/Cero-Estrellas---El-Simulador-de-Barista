#pragma once

#include "CoreMinimal.h"
#include "GameFramework/Character.h"
#include "GameFramework/GameModeBase.h"
#include "GameFramework/HUD.h"
#include "Subsystems/WorldSubsystem.h"
#include "CEGameplay.generated.h"

class UCameraComponent;
class UPhysicsHandleComponent;
class UStaticMeshComponent;
class UTextRenderComponent;
class UCapsuleComponent;
class USoundBase;
class ACEStation;
class ACELiquidSystem;

UENUM(BlueprintType)
enum class ECEPropKind : uint8 { Cup, Pastry, Tray, Portafilter, Laptop, Pitcher, Chair };

UENUM(BlueprintType)
enum class ECECustomerState : uint8 { Arriving, Ordering, WalkingToTable, Waiting, Eating, Leaving };

UENUM(BlueprintType)
enum class ECEDrink : uint8 { Espresso, Americano, Latte };

USTRUCT(BlueprintType)
struct FCEChaosEvent
{
    GENERATED_BODY()
    UPROPERTY(BlueprintReadOnly) FString EventId;
    UPROPERTY(BlueprintReadOnly) FName Kind;
    UPROPERTY(BlueprintReadOnly) FString CustomerId;
    UPROPERTY(BlueprintReadOnly) FString ObjectName;
    UPROPERTY(BlueprintReadOnly) float Severity = 0.f;
};

DECLARE_DYNAMIC_MULTICAST_DELEGATE_OneParam(FCEChaosObserved, const FCEChaosEvent&, Event);

// Gameplay emits facts here. The future dialogue pipeline can subscribe without owning physics.
UCLASS()
class CEROESTRELLAS_API UCECafeSubsystem : public UWorldSubsystem
{
    GENERATED_BODY()
public:
    UPROPERTY(BlueprintAssignable) FCEChaosObserved OnChaosObserved;
    UPROPERTY(BlueprintReadOnly) int32 ChaosCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 PraiseCount = 0;
    UPROPERTY(BlueprintReadOnly) int32 DepartedCustomers = 0;
    UPROPERTY(BlueprintReadOnly) FString BossSubtitle;
    UPROPERTY(BlueprintReadOnly) FString LastIncident;
    UPROPERTY(BlueprintReadOnly) float BossSubtitleUntil = 0.f;
    UPROPERTY(BlueprintReadOnly) int32 OrdersTaken = 0;
    UPROPERTY(BlueprintReadOnly) int32 OrdersServed = 0;
    UPROPERTY(BlueprintReadOnly) int32 WrongOrders = 0;
    UPROPERTY(BlueprintReadOnly) float DisasterScore = 0.f;
    UPROPERTY(BlueprintReadOnly) int32 Combo = 0;
    UPROPERTY(BlueprintReadOnly) int32 BestCombo = 0;
    UPROPERTY(BlueprintReadOnly) int32 ViralPoints = 0;
    UPROPERTY(BlueprintReadOnly) int32 ShiftLevel = 1;
    UPROPERTY(BlueprintReadOnly) float ComboUntil = 0.f;
    UPROPERTY(BlueprintReadOnly) int32 Falls = 0;
    UPROPERTY(BlueprintReadOnly) FString Challenge = TEXT("Sirve 2 desayunos para financiar tu sabotaje");
    int32 ChallengeStep = 0;
    UPROPERTY(BlueprintReadOnly) FString Feedback;
    UPROPERTY(BlueprintReadOnly) float FeedbackUntil = 0.f;
    UPROPERTY() TObjectPtr<ACELiquidSystem> Liquids;
    void Notify(const FString& Text);
    void Sound(const TCHAR* Name, FVector Location, float Volume=.65f);
    int32 LastBossPhrase = -1;
    float BossCooldownUntil = 0.f;
    UFUNCTION(BlueprintCallable) void Observe(FName Kind, const FString& CustomerId, const FString& ObjectName, float Severity);
    void ConfirmDeparture(const FString& CustomerId, bool bDisaster=true);
    void TickRadio();
private:
    void Praise(FName Kind);
    TSet<FString> Departures;
    TArray<int32> UnusedPhrases;
    float LastScoreAt = 0.f;
    TSet<FString> RecentVictims;
    int32 EventSerial = 0;
    int32 PhraseSerial = 0;
    TMap<FString,float> ComboEvents;
};

UCLASS()
class CEROESTRELLAS_API ACEProp : public AActor
{
    GENERATED_BODY()
public:
    ACEProp();
    void Configure(ECEPropKind InKind);
    virtual void Tick(float DeltaSeconds) override;
    void MarkThrown();
    bool Pour();
    float VolumeMl() const { return EspressoMl+MilkMl+WaterMl; }
    float AddLiquid(float Espresso, float Milk, float Water, float Degrees);
    void UpdateLiquidVisual();
    void SpillConfirmed(float Amount);
    bool IsContainer() const { return Kind==ECEPropKind::Cup || Kind==ECEPropKind::Pitcher; }
    float CapacityMl() const { return Kind==ECEPropKind::Pitcher ? 350.f : 180.f; }
    FVector PourTarget = FVector::ZeroVector;
    bool bAimedPour = false;
    UPROPERTY(BlueprintReadOnly) float EspressoMl = 0.f;
    UPROPERTY(BlueprintReadOnly) float MilkMl = 0.f;
    UPROPERTY(BlueprintReadOnly) float WaterMl = 0.f;
    UPROPERTY(BlueprintReadOnly) float Temperature = 20.f;
    UPROPERTY(BlueprintReadOnly) bool bGroundDose = false;
    UPROPERTY(BlueprintReadOnly) bool bTamped = false;
    UPROPERTY(BlueprintReadOnly) bool bBroken = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(BlueprintReadOnly) ECEPropKind Kind = ECEPropKind::Cup;
    UPROPERTY(BlueprintReadOnly) FString DisplayName;
    UPROPERTY(BlueprintReadOnly) bool bHasCoffee = false;
    bool bHeld = false;
    bool bPourRequested = false;
    bool bDelivered = false;
private:
    UFUNCTION() void Hit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& HitResult);
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Coffee;
    TArray<TObjectPtr<UStaticMeshComponent>> Details;
    bool bThrown = false;
    float LastHitAt = -100.f;
    float DropAccumulator = 0.f;
    float SpillSinceEvent = 0.f;
    float LastSpillEvent = -100.f;
};

UCLASS()
class CEROESTRELLAS_API ACECustomer : public AActor
{
    GENERATED_BODY()
public:
    ACECustomer();
    void Configure(const FString& Id, const FString& Name, const FLinearColor& Outfit, int32 Archetype);
    virtual void Tick(float DeltaSeconds) override;
    void React(FName Kind, const FString& ObjectName, float Severity);
    void TakeOrder();
    bool Serve(ACEProp* Item);
    void Taunt();
    void BeginVisit(int32 Slot, ECEDrink Drink, bool bPastry);
    FString OrderText() const;
    void Speak(const FString& Text, float Duration=5.f);
    void Fall(const FVector& Direction);
    void Witness(ACECustomer* Victim);
    UPROPERTY(BlueprintReadOnly) bool bSeated = false;
    UPROPERTY(BlueprintReadOnly) float FallenUntil = 0.f;
    UPROPERTY(BlueprintReadOnly) int32 WitnessCount = 0;
    UPROPERTY() TObjectPtr<ACEProp> Seat;
    UPROPERTY(BlueprintReadOnly) ECECustomerState State = ECECustomerState::Arriving;
    UPROPERTY(BlueprintReadOnly) ECEDrink RequestedDrink = ECEDrink::Espresso;
    UPROPERTY(BlueprintReadOnly) bool bWantsPastry = false;
    UPROPERTY(BlueprintReadOnly) bool bOrderKnown = false;
    UPROPERTY(BlueprintReadOnly) bool bDrinkServed = false;
    UPROPERTY(BlueprintReadOnly) bool bPastryServed = false;
    UPROPERTY(BlueprintReadOnly) float Patience = 110.f;
    UPROPERTY(BlueprintReadOnly) float Anger = 0.f;
    int32 SeatSlot = 0;
    float LastSlipAt = -100.f;
    UPROPERTY(BlueprintReadOnly) FString CustomerId;
    UPROPERTY(BlueprintReadOnly) FString Reaction;
    UPROPERTY(BlueprintReadOnly) bool bLeaving = false;
    UPROPERTY(BlueprintReadOnly) int32 HitCount = 0;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCapsuleComponent> Collision;
private:
    UPROPERTY() TObjectPtr<UTextRenderComponent> Bubble;
    int32 Personality = 0;
    float ExitStartAt = 0.f;
    float ReactionUntil = 0.f;
    int32 ExitWaypoint = 0;
    FVector StartPosition;
    TArray<FVector> Route;
    int32 RouteIndex = 0;
    float EatUntil = 0.f;
    float NextComplaintAt = 0.f;
    float GestureUntil = 0.f;
    bool bDisasterExit = false;
    float NextReactionAt = 0.f;
    float NextWitnessAt = 0.f;
    int32 TauntSerial = 0;
    TWeakObjectPtr<ACECustomer> PendingVictim;
    float WitnessRetryUntil = 0.f;
    float NextWitnessAttemptAt = 0.f;
    UPROPERTY() TObjectPtr<USceneComponent> VisualRoot;
    UPROPERTY() TObjectPtr<USceneComponent> HeadPivot;
    UPROPERTY() TObjectPtr<USceneComponent> ArmPivots[2];
    UPROPERTY() TObjectPtr<USceneComponent> LegPivots[2];
    UPROPERTY() TObjectPtr<USceneComponent> KneePivots[2];
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Pupils[2];
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Eyes[2];
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Brows[2];
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Mouth;
    void Leave(bool bDisaster);
    void Animate(float DeltaSeconds, bool bWalking);
};

UCLASS()
class CEROESTRELLAS_API ACEPlayer : public ACharacter
{
    GENERATED_BODY()
public:
    ACEPlayer();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    virtual void SetupPlayerInputComponent(UInputComponent* Input) override;
    UFUNCTION(BlueprintCallable) void Grab();
    UFUNCTION(BlueprintCallable) void Throw();
    UFUNCTION(BlueprintCallable) void Pour();
    UFUNCTION(BlueprintCallable) void Interact();
    UFUNCTION(BlueprintCallable) void Taunt();
    AActor* FocusActor(float Range=300.f) const;
    void Hold(ACEProp* Prop);
    void Release();
    ACEProp* FocusProp() const;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPhysicsHandleComponent> Handle;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<ACEProp> Held;
    bool bPouring = false;
    float SlipUntil = 0.f;
private:
    float FootstepAt = 0.f;
    void Forward(float Value);
    void Right(float Value);
    void Turn(float Value);
    void Look(float Value);
    void SprintOn();
    void SprintOff();
    void StopPour();
    void Reset();
    void Quit();
};

UCLASS()
class CEROESTRELLAS_API ACEHUD : public AHUD
{
    GENERATED_BODY()
public:
    virtual void DrawHUD() override;
private:
    void Label(const FString& Text, float X, float Y, float Scale, FLinearColor Color);
    void Wrapped(const FString& Text, float X, float Y, float Width, float Scale, FLinearColor Color);
};

UCLASS()
class CEROESTRELLAS_API ACEGameMode : public AGameModeBase
{
    GENERATED_BODY()
public:
    ACEGameMode();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    void TickArrivals();
    bool bLoopSmoke = false;
private:
    void BuildCafe();
    void SmokeStep();
    void LoopSmokeStep();
    bool bSmoke = false;
    bool bSmokeInputReady = false;
    int32 SmokeStage = 0;
    float SmokeNextAt = 4.f;
    bool bSmokeGrab = false;
    bool bSmokeThrow = false;
    bool bSmokeSpill = false;
    bool bSmokeTray = false;
    bool bSmokeMovement = false;
    bool bSmokeSprint = false;
    float SmokeMovementStartY = 0.f;
    UPROPERTY() TObjectPtr<ACEProp> SmokeCup;
    UPROPERTY() TObjectPtr<ACEProp> SmokePastry;
    UPROPERTY() TObjectPtr<ACEProp> SmokeTray;
    UPROPERTY() TObjectPtr<ACECustomer> SmokeCustomer;
    UPROPERTY() TObjectPtr<ACEStation> EspressoMachine;
    UPROPERTY() TObjectPtr<ACEStation> Grinder;
    UPROPERTY() TObjectPtr<ACEStation> MilkStation;
    UPROPERTY() TObjectPtr<ACEStation> WaterStation;
    UPROPERTY() TObjectPtr<ACEStation> CupSupply;
    UPROPERTY() TObjectPtr<ACEStation> TraySupply;
    UPROPERTY() TObjectPtr<ACEStation> TampStation;
    UPROPERTY() TObjectPtr<ACEProp> SmokePitcher;
    UPROPERTY() TObjectPtr<ACECustomer> SmokeWitness;
    float NextArrivalAt = 4.f;
    int32 CustomerSerial = 0;
    TMap<FString,bool> LoopChecks;
    FVector LoopPreviousCustomerPosition;
    int32 LoopFirstBossPhrase = -1;
};
