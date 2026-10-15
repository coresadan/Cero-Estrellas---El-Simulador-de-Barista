#pragma once
#include "CoreMinimal.h"
#include "CEGameplay.h"
#include "CEKitchen.generated.h"

class UInstancedStaticMeshComponent;
class UAudioComponent;

UCLASS()
class CEROESTRELLAS_API ACECafeDoor : public AActor
{
    GENERATED_BODY()
public:
    ACECafeDoor();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    float OpenAngle = 0.f;
private:
    UPROPERTY() TObjectPtr<USceneComponent> Hinge;
    float HoldUntil = 0.f;
};

UENUM()
enum class ECEStationKind : uint8 { Grinder, Espresso, Milk, Water, Cups, Trays, Pastries, Bin, Tamper, Sugar, Syrup, Frother };

// Stations expose contextual interaction; physical containers stay independent actors.
UCLASS()
class CEROESTRELLAS_API ACEStation : public AActor
{
    GENERATED_BODY()
public:
    ACEStation();
    void Configure(ECEStationKind InKind);
    virtual void Tick(float DeltaSeconds) override;
    void Use(ACEPlayer* Player);
    FString Prompt(const ACEPlayer* Player) const;
    FString Status() const;
    FVector CupPosition() const;
    ACEProp* CupUnderSpout() const;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> Body;
    UPROPERTY() TObjectPtr<UTextRenderComponent> Screen;
    UPROPERTY() TObjectPtr<UAudioComponent> WorkingSound;
    UPROPERTY() TObjectPtr<ACEProp> Filter;
    UPROPERTY() TObjectPtr<ACEProp> WorkingCup;
    ECEStationKind Kind = ECEStationKind::Espresso;
    float WorkLeft = 0.f;
    bool bDoseReady = false;
    int32 DosesLeft = 12;
    int32 Dispensed = 0;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> MovingTool;
    float ToolRestZ = 0.f;
    float LockLeft = 0.f;
    float TampPressure = 0.f;
    float ExtractedMl = 0.f;
    float ExtractionTargetMl = 60.f;
    float MilkTargetDegrees = 65.f;
    float PressureBars = 0.f;
    void AdjustSetting(int32 Direction);
    UPROPERTY() TObjectPtr<UStaticMeshComponent> GaugeNeedle;
    UPROPERTY() TObjectPtr<UStaticMeshComponent> ControlKnob;
private:
    float TransferAccumulator = 0.f;
    float ScreenNextAt = 0.f;
    void PlaceCup(ACEPlayer* Player);
};

struct FCELiquidDrop
{
    FVector Position = FVector::ZeroVector;
    FVector PreviousPosition = FVector::ZeroVector;
    FVector Velocity = FVector::ZeroVector;
    FLinearColor Color;
    float Age = 0.f;
    float Espresso = 0.f;
    float Milk = 0.f;
    float Water = 0.f;
    float Temperature = 20.f;
    float Foam = 0.f;
    float Sugar = 0.f;
    float Syrup = 0.f;
    bool bAlive = false;
    TWeakObjectPtr<ACEProp> Source;
};

struct FCEPuddle
{
    FVector Position;
    float Ml = 0.f;
    float Until = 0.f;
    float Radius = 0.f;
    FLinearColor Color;
    int32 Instance = 0;
};

// Fixed pools: ballistic drops, collision and conserved volume, without a fluid grid.
UCLASS()
class CEROESTRELLAS_API ACELiquidSystem : public AActor
{
    GENERATED_BODY()
public:
    ACELiquidSystem();
    virtual void BeginPlay() override;
    virtual void Tick(float DeltaSeconds) override;
    bool Emit(FVector Origin, FVector Velocity, float Espresso, float Milk, float Water, float Degrees, ACEProp* Source=nullptr);
    void Splash(FVector Position, FLinearColor Color, float Ml);
    int32 ActiveDrops() const;
    FVector LargestFloorPuddle() const;
    int32 VisiblePuddleLobes() const { return Puddles.Num()*5; }
    float FloorMl = 0.f;
    float TransferredMl = 0.f;
    float SoakedMl = 0.f;
    int32 SlipCount = 0;
    int32 CollisionQueries = 0;
    int32 SimulationSteps = 0;
    int32 PeakActiveDrops = 0;
    double SimulationCpuMs = 0;
    float EmittedMl = 0.f;
    float UnresolvedMl = 0.f;
    float InFlightMl() const;
    void Simulate(float Step);
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Drops;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Streams;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Pools;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> Steam;
    UPROPERTY() TObjectPtr<UInstancedStaticMeshComponent> ImpactSpray;
private:
    struct FImpactParticle { FVector Position=FVector::ZeroVector,Velocity=FVector::ZeroVector; float Age=1.f; };
    TArray<FImpactParticle> ImpactParticles;
    int32 ImpactCursor=0;
    float NextImpactSprayAt=0;
    void Spray(FVector Position,FVector Normal,FLinearColor Color);
    struct FSteamPuff { FVector Position=FVector::ZeroVector; float Age=10.f; };
    TArray<FSteamPuff> Puffs;
    float NextSteamAt = 0.f;
    int32 SteamCursor = 0;
    TArray<FCELiquidDrop> Particles;
    TArray<FCEPuddle> Puddles;
    struct FFlow { TWeakObjectPtr<ACEProp> Source; FVector Origin=FVector::ZeroVector,Velocity=FVector::ZeroVector; float Until=0,Flight=.45f,NextPredictAt=0; };
    TArray<FFlow> Flows;
    TArray<TWeakObjectPtr<ACEProp>> HotContainers;
    float RegistryNextAt = 0;
    float SimulationAccumulator = 0;
    int32 Cursor = 0;
    float NextSlipAt = 0.f;
    float PlayerSlipUntil = 0.f;
    void Impact(FCELiquidDrop& Drop, const FHitResult& Hit);
};
