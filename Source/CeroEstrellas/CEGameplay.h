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
class UCEDialogueComponent;
class USkeletalMeshComponent;
class UAudioComponent;
class UMaterialInstanceDynamic;
class UPhysicalAnimationComponent;

UENUM(BlueprintType)
enum class ECEPropKind : uint8 { Cup, Pastry, Tray, Portafilter, Laptop, Pitcher, Chair, Notebook, Flower, Bag, Mobile, Tablet };

UENUM(BlueprintType)
enum class ECEGuestDevice : uint8 { None, Laptop, Mobile, Tablet };

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
    UPROPERTY(BlueprintReadOnly) int32 ShiftPhase = 0;
    UPROPERTY(BlueprintReadOnly) FString ShiftTitle = TEXT("APERTURA / Un desayuno tranquilo");
    UPROPERTY(BlueprintReadOnly) float ShiftDuration = 480.f;
    UPROPERTY(BlueprintReadOnly) bool bShiftFinished = false;
    UPROPERTY(BlueprintReadOnly) int32 StoriesHelped = 0;
    UPROPERTY(BlueprintReadOnly) int32 StoriesRuined = 0;
    UPROPERTY(BlueprintReadOnly) FString LastStoryOutcome;
    UPROPERTY(BlueprintReadOnly) int32 ChainReactions = 0;
    UPROPERTY(BlueprintReadOnly) FString BossCampaign;
    UPROPERTY(BlueprintReadOnly) FString LastMajorVictim;
    FName LastMajorKind;
    float CampaignUntil = 0.f;
    int32 CampaignsCreated = 0;
    int32 StoryEncounters = 0;
    void WarmResources();
    bool HasSound(FName Name) const { return SoundCache.Contains(Name); }
    int32 ResidentResourceCount() const { return ResidentResources.Num(); }
    UPROPERTY() TMap<FString,TObjectPtr<UMaterialInstanceDynamic>> SharedMaterials;
    UPROPERTY(BlueprintReadOnly) int32 FilmedDisasters = 0;
    UPROPERTY(BlueprintReadOnly) FString CustomerSubtitle;
    UPROPERTY(BlueprintReadOnly) float CustomerSubtitleUntil = 0.f;
    UPROPERTY(BlueprintReadOnly) FString SubtitleCustomer;
    void Subtitle(const FString& Id,const FString& Name,const FString& Text,float Duration);
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
    void TickBusiness(bool bForce=false);
    UPROPERTY(BlueprintReadOnly) int32 SpecialEvent = 0;
    UPROPERTY(BlueprintReadOnly) int32 BusinessEvents = 0;
    UPROPERTY(BlueprintReadOnly) FString BusinessTitle;
    float BusinessUntil = 0.f;
    float NextBusinessAt = 150.f;
private:
    void Praise(FName Kind);
    TSet<FString> Departures;
    TArray<int32> UnusedPhrases;
    float LastScoreAt = 0.f;
    TSet<FString> RecentVictims;
    int32 EventSerial = 0;
    int32 PhraseSerial = 0;
    TMap<FString,float> ComboEvents;
    TMap<FName,float> SoundTimes;
    UPROPERTY() TObjectPtr<UAudioComponent> Ambience;
    UPROPERTY() TArray<TObjectPtr<UObject>> ResidentResources;
    UPROPERTY() TMap<FName,TObjectPtr<USoundBase>> SoundCache;
    UPROPERTY() TArray<TObjectPtr<UAudioComponent>> RoomAmbience;
    int32 AnnouncedPhase = 0;
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
    bool WasThrown() const { return bThrown; }
    bool Pour();
    float VolumeMl() const { return EspressoMl+MilkMl+WaterMl; }
    float AddLiquid(float Espresso, float Milk, float Water, float Degrees);
    void UpdateLiquidVisual();
    void SpillConfirmed(float Amount);
    bool IsContainer() const { return Kind==ECEPropKind::Cup || Kind==ECEPropKind::Pitcher; }
    bool IsDevice() const { return Kind==ECEPropKind::Laptop || Kind==ECEPropKind::Mobile || Kind==ECEPropKind::Tablet; }
    void DamageDevice();
    void SetDeviceCarried(bool bCarried);
    float CapacityMl() const { return Kind==ECEPropKind::Pitcher ? 350.f : 180.f; }
    FVector PourTarget = FVector::ZeroVector;
    bool bAimedPour = false;
    UPROPERTY(BlueprintReadOnly) float EspressoMl = 0.f;
    UPROPERTY(BlueprintReadOnly) float MilkMl = 0.f;
    UPROPERTY(BlueprintReadOnly) float WaterMl = 0.f;
    UPROPERTY(BlueprintReadOnly) float Temperature = 20.f;
    UPROPERTY(BlueprintReadOnly) float FoamMl = 0.f;
    UPROPERTY(BlueprintReadOnly) float SugarGrams = 0.f;
    UPROPERTY(BlueprintReadOnly) float SyrupMl = 0.f;
    UPROPERTY(BlueprintReadOnly) bool bGroundDose = false;
    UPROPERTY(BlueprintReadOnly) bool bTamped = false;
    UPROPERTY(BlueprintReadOnly) bool bBroken = false;
    UPROPERTY(VisibleAnywhere, BlueprintReadOnly) TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY(BlueprintReadOnly) ECEPropKind Kind = ECEPropKind::Cup;
    UPROPERTY(BlueprintReadOnly) FString OwnerCustomerId;
    UPROPERTY(BlueprintReadOnly) FString DisplayName;
    UPROPERTY(BlueprintReadOnly) bool bHasCoffee = false;
    bool bHeld = false;
    bool bPourRequested = false;
    bool bDelivered = false;
    bool bStuck = false;
    float SurfaceTilt = 0.f;
    void AnimateLiquidSurface(float Delta);
private:
    UFUNCTION() void Hit(UPrimitiveComponent* HitComponent, AActor* OtherActor, UPrimitiveComponent* OtherComponent, FVector NormalImpulse, const FHitResult& HitResult);
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Coffee;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Foam;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> DeviceScreen;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ClosedCover;
    TArray<TObjectPtr<UStaticMeshComponent>> Details;
    bool bThrown = false;
    float LastHitAt = -100.f;
    float DropAccumulator = 0.f;
    float SpillSinceEvent = 0.f;
    float LastSpillEvent = -100.f;
    FVector PreviousVelocity = FVector::ZeroVector;
    FVector2D Slosh = FVector2D::ZeroVector;
};

UCLASS()
class CEROESTRELLAS_API ACECustomer : public AActor
{
    GENERATED_BODY()
public:
    ACECustomer();
    void Configure(const FString& Id, const FString& Name, const FLinearColor& Outfit, int32 Archetype, int32 Look=-1);
    virtual void Tick(float DeltaSeconds) override;
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    void React(FName Kind, const FString& ObjectName, float Severity);
    void TakeOrder();
    bool Serve(ACEProp* Item);
    void Taunt();
    void BeginVisit(int32 Slot, ECEDrink Drink, bool bPastry);
    void PreparePossessions(ECEGuestDevice Device);
    void TickPossessions(float DeltaSeconds);
    UPROPERTY(BlueprintReadOnly) ECEGuestDevice VisitDevice=ECEGuestDevice::None;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<ACEProp> PersonalDevice;
    UPROPERTY(BlueprintReadOnly) bool bDevicePlaced=false;
    UPROPERTY(BlueprintReadOnly) bool bPossessionsPrepared=false;
    float DevicePlacement=0.f;
    float DevicePlacedAt=-1.f;
    TArray<TWeakObjectPtr<ACEProp>> Possessions;
    FString OrderText() const;
    void Speak(const FString& Text, float Duration=5.f);
    int32 RecordedLinesPlayed=0;
    FString LastRecordedLine;
    void Fall(const FVector& Direction);
    void Witness(ACECustomer* Victim);
    void ApplyDialogue(const FString& Text,const FString& Emotion,float AngerChange,bool bWantsToLeave);
    bool BuildRig(const FLinearColor& Outfit);
    void AnimateRig(float DeltaSeconds,bool bWalking);
    void Soak(float Ml,const FString& Liquid=TEXT("café"));
    void VoiceStarted(float Duration,float Energy=1.f);
    void SetSpeechEnvelope(const TArray<float>& Levels,float Duration);
    void ImmediateVoice(const TCHAR* Cue);
    FVector HeadLocation() const;
    UPROPERTY(VisibleAnywhere) TObjectPtr<USkeletalMeshComponent> Rig;
    bool StartPhysicalFall(const FVector& Direction);
    void EndPhysicalFall();
    void PhysicalRecoil(const FVector& Direction);
    UPROPERTY(BlueprintReadOnly) bool bPhysicalFall = false;
    UPROPERTY(BlueprintReadOnly) int32 PhysicalFalls = 0;
    UPROPERTY(BlueprintReadOnly) FString VisitRole;
    UPROPERTY(BlueprintReadOnly) float Trust = 0.f;
    UPROPERTY(BlueprintReadOnly) int32 DefensiveMoves = 0;
    UPROPERTY(BlueprintReadOnly) int32 SocialExchanges = 0;
    UPROPERTY(BlueprintReadOnly) bool bWantsFoam = false;
    UPROPERTY(BlueprintReadOnly) bool bWantsSugar = false;
    UPROPERTY(BlueprintReadOnly) bool bWantsSyrup = false;
    void TickSocial(float DeltaSeconds);
    void InitStory(int32 ForceStory=-1);
    void TickStory(float DeltaSeconds);
    void StoryConversation(const FString& Text);
    void ResolveStory(bool bSuccess);
    FString StoryAnswer() const;
    FString GuestPersonality() const;
    UPROPERTY(BlueprintReadOnly) int32 LookIndex = -1;
    UPROPERTY(BlueprintReadOnly) int32 StoryId = -1;
    UPROPERTY(BlueprintReadOnly) FString StoryTitle;
    UPROPERTY(BlueprintReadOnly) FString StorySetup;
    UPROPERTY(BlueprintReadOnly) FString StoryObjective;
    UPROPERTY(BlueprintReadOnly) FString StoryOutcome;
    UPROPERTY(BlueprintReadOnly) bool bStoryResolved = false;
    UPROPERTY(BlueprintReadOnly) bool bStoryHelped = false;
    UPROPERTY(BlueprintReadOnly) float WipeUntil = 0.f;
    UPROPERTY(BlueprintReadOnly) int32 SeatAdjustments = 0;
    float NextSeatAdjustAt = 0.f;
    float StoryStartedAt = 0.f;
    UPROPERTY(BlueprintReadOnly) int32 StoryStage = 0;
    UPROPERTY(BlueprintReadOnly) FString StoryBeat;
    UPROPERTY(BlueprintReadOnly) FString StoryDraft;
    UPROPERTY(BlueprintReadOnly) FString SocialPartnerId;
    UPROPERTY(BlueprintReadOnly) FString SocialGroup;
    float StoryStageStartedAt = 0.f;
    float SocialGestureUntil = 0.f;
    float PhoneCallUntil = 0.f;
    void PairWith(ACECustomer* Other);
    void AdvanceStory();
    UPROPERTY(BlueprintReadOnly) FString RememberedPromise;
    FVector BodyScale = FVector::OneVector;
    int32 CasualExchanges = 0;
    float GestureAttention = 0.f;
    bool IsBodyConsistent(float MaxJointError=12.f) const;
    bool HasNaturalArmPose() const;
    UPROPERTY(BlueprintReadOnly) float WetMl = 0.f;
    UPROPERTY(BlueprintReadOnly) FString ReceivedLiquid;
    UPROPERTY(BlueprintReadOnly) bool bFilming = false;
    UPROPERTY(BlueprintReadOnly) FString SpecialRequest;
    float MinServingTemperature = 45.f;
    float MinServingVolume = 0.f;
    float ThinkingUntil = 0.f;
    float VoiceUntil = 0.f;
    int32 Archetype() const { return Personality; }
    FString DisplayName;
    FString DialogueEmotion;
    float DialogueHoldUntil = 0.f;
    float DialogueDepartureAt = 0.f;
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
    UFUNCTION() void RagdollContact(UPrimitiveComponent* HitComponent,AActor* OtherActor,UPrimitiveComponent* OtherComponent,FVector Impulse,const FHitResult& Hit);
    float LastRagdollContactAt = -100.f;
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
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Phone;
    UPROPERTY() TObjectPtr<UAudioComponent> AuthoredSpeaker;
    UPROPERTY() TObjectPtr<UMaterialInstanceDynamic> ClothingMaterial;
    TMap<FName,FTransform> RestBones;
    TMap<FName,FTransform> PosedBones;
    float PoseSeat = 0.f;
    float PoseFall = 0.f;
    float PoseGesture = 0.f;
    float PoseHeadYaw = 0.f;
    float WalkPhase = 0.f;
    float PoseWalk = 0.f;
    FVector LastMotionLocation = FVector::ZeroVector;
    float FilmingUntil = 0.f;
    float LastChainAt = -100.f;
    bool bTakingSip = false;
    float SipBlend = 0.f;
    float WipeBlend = 0.f;
    float PhoneBlend = 0.f;
    float ProtectBlend = 0.f;
    float FaceTalk = 0.f;
    float NextGroundCheckAt = 0.f;
    float VoiceEnergy = 1.f;
    float VoiceBegin = 0.f;
    TArray<float> SpeechLevels;
    TWeakObjectPtr<ACEProp> ServedCup;
    FVector ServedCupPosition;
    float ImmediateVoiceAt = -100.f;
    float ProtectUntil = 0.f;
    float NextProtectAt = 0.f;
    float NextSocialAt = 0.f;
    float NextDefendAt = 0.f;
    float Suspicion = 0.f;
    float RecoilUntil = 0.f;
    float RecoverUntil = 0.f;
    bool bHeadPhysics = false;
    UPROPERTY() TObjectPtr<UPhysicalAnimationComponent> PhysicalDriver;
    TArray<FTransform> RecoveryPose;
    float FootGroundOffsets[2] = {0.f,0.f};
    float FallSeatBlend = 0.f;
    FVector FallOrigin;
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
    void Talk();
    void TalkVoice();
    void Hold(ACEProp* Prop);
    void Release();
    ACEProp* FocusProp() const;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCameraComponent> Camera;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UPhysicsHandleComponent> Handle;
    UPROPERTY(VisibleAnywhere) TObjectPtr<UCEDialogueComponent> Dialogue;
    UPROPERTY(BlueprintReadOnly) TObjectPtr<ACEProp> Held;
    bool bPouring = false;
    bool bInteractHeld = false;
    void ReleaseInteraction() { bInteractHeld=false; }
    bool bShowOrderList = false;
    bool bShowHelp = false;
    void ShowOrders() { bShowOrderList=true; }
    void HideOrders() { bShowOrderList=false; }
    void ToggleHelp() { bShowHelp=!bShowHelp; }
    float SlipUntil = 0.f;
    int32 VisibleHands() const;
    void AnimateHands(float DeltaSeconds);
    void AdjustApplianceUp();
    void AdjustApplianceDown();
private:
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Hands[2];
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Forearms[2];
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Sleeves[2];
    float ActionUntil = 0.f;
    TWeakObjectPtr<AActor> Highlighted;
    void SetHighlight(AActor* Actor);
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
    virtual void EndPlay(const EEndPlayReason::Type Reason) override;
    bool HasModernOverlay() const { return Overlay.IsValid(); }
private:
    TSharedPtr<struct FCEHUDView> ViewState;
    TSharedPtr<class SWidget> Overlay;
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
    void TickTickets();
    // Author the fixed set into the editor map; gameplay actors remain runtime-owned.
    UFUNCTION(BlueprintCallable,Category="CeroEstrellas|Authoring")
    void BuildCafe(bool bAuthoring=false);
    bool bLoopSmoke = false;
private:
    void SmokeStep();
    void LoopSmokeStep();
    void DialogueSmokeStep();
    void PolishSmokeStep();
    void ProfessionalSmokeStep();
    void ExpansionSmokeStep();
    float ExpansionDeadline=0;
    bool bPolishChecksCompleted = false;
    bool bDialogueSmoke = false;
    bool bSmokeMicrophoneAvailable = false;
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
    TWeakObjectPtr<ACECustomer> PendingCompanion;
    TMap<FString,bool> LoopChecks;
    FVector LoopPreviousCustomerPosition;
    int32 LoopFirstBossPhrase = -1;
    TArray<float> FrameSamples;
    double LastFrameWallSeconds=0;
    UPROPERTY() TArray<TObjectPtr<UTextRenderComponent>> OrderTickets;
    float NextTicketAt = 0.f;
};
