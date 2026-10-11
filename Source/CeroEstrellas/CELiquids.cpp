#include "CEKitchen.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "CEVisual.h"

ACELiquidSystem::ACELiquidSystem()
{
    PrimaryActorTick.bCanEverTick=true;
    auto* Root=CreateDefaultSubobject<USceneComponent>(TEXT("LiquidRoot")); SetRootComponent(Root);
    Drops=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BallisticDrops")); Drops->SetupAttachment(Root);
    Pools=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PersistentPuddles")); Pools->SetupAttachment(Root);
    for(auto* Mesh : {Drops.Get(),Pools.Get()})
    {
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false); Mesh->NumCustomDataFloats=3;
    }
}
void ACELiquidSystem::BeginPlay()
{
    Super::BeginPlay();
    Drops->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Pools->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder")));
    auto* Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Prototype/M_Liquid.M_Liquid"));
    Drops->SetMaterial(0,Mat); Pools->SetMaterial(0,Mat);
    Particles.SetNum(192);
    for(int32 i=0;i<Particles.Num();++i) Drops->AddInstance(FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector));
    for(int32 i=0;i<32*5;++i) Pools->AddInstance(FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector));
}
int32 ACELiquidSystem::ActiveDrops() const
{
    int32 Count=0; for(const auto& Drop:Particles) if(Drop.bAlive) ++Count; return Count;
}
FVector ACELiquidSystem::LargestFloorPuddle() const
{
    const FCEPuddle* Largest=nullptr;
    for(const auto& Puddle:Puddles) if(Puddle.Position.Z<5.f && (!Largest || Puddle.Ml>Largest->Ml)) Largest=&Puddle;
    return Largest ? Largest->Position : FVector::ZeroVector;
}
bool ACELiquidSystem::Emit(FVector Origin, FVector Velocity, float Espresso, float Milk, float Water, float Degrees, ACEProp* Source)
{
    if(Particles.IsEmpty() || Espresso+Milk+Water<=0.f) return false;
    for(int32 n=0;n<Particles.Num();++n)
    {
        const int32 Index=(Cursor+n)%Particles.Num();
        auto& Drop=Particles[Index];
        if(Drop.bAlive) continue;
        Drop.Position=Origin; Drop.Velocity=Velocity; Drop.Age=0.f;
        Drop.Espresso=Espresso; Drop.Milk=Milk; Drop.Water=Water; Drop.Temperature=Degrees;
        Drop.Color=FMath::Lerp(FLinearColor(.09f,.025f,.008f),FLinearColor(.88f,.81f,.61f),FMath::Clamp(Milk/(Espresso+Milk+Water),0.f,1.f));
        if(Espresso<.01f && Milk<.01f) Drop.Color=FLinearColor(.35f,.65f,.7f);
        Drop.Source=Source; Drop.bAlive=true; Cursor=(Index+1)%Particles.Num();
        Drops->SetCustomDataValue(Index,0,Drop.Color.R,false); Drops->SetCustomDataValue(Index,1,Drop.Color.G,false); Drops->SetCustomDataValue(Index,2,Drop.Color.B,false);
        return true;
    }
    return false; // Caller retains volume until an available pool slot exists.
}
void ACELiquidSystem::Splash(FVector Position, FLinearColor Color, float Ml)
{
    if(Ml<=0.f) return;
    FloorMl+=Ml;
    int32 Index=INDEX_NONE;
    for(int32 i=0;i<Puddles.Num();++i) if(FVector::Dist2D(Puddles[i].Position,Position)<65.f && FMath::Abs(Puddles[i].Position.Z-Position.Z)<8.f) { Index=i; break; }
    if(Index==INDEX_NONE)
    {
        if(Puddles.Num()<32)
        {
            Index=Puddles.AddDefaulted(); Puddles[Index].Instance=Index;
        }
        else
        {
            Index=0; for(int32 i=1;i<Puddles.Num();++i) if(Puddles[i].Until<Puddles[Index].Until) Index=i;
            Puddles[Index].Ml=0.f;
        }
        Puddles[Index].Position=Position+FVector(0,0,.65f); Puddles[Index].Color=Color;
    }
    auto& Pool=Puddles[Index]; Pool.Ml+=Ml; Pool.Radius=FMath::Clamp(FMath::Sqrt(Pool.Ml)*4.f,12.f,95.f);
    Pool.Until=GetWorld()->GetTimeSeconds()+180.f;
    const float Scale=Pool.Radius/50.f;
    Pool.Color=(Pool.Color*(Pool.Ml-Ml)+Color*Ml)/FMath::Max(1.f,Pool.Ml);
    for(int32 Lobe=0;Lobe<5;++Lobe)
    {
        const float Angle=(Pool.Instance*1.91f+Lobe*1.37f), Spread=Lobe==0 ? 0.f : Pool.Radius*.42f;
        const FVector Offset(FMath::Cos(Angle)*Spread,FMath::Sin(Angle)*Spread,Lobe*.035f);
        const float LobeScale=Scale*(Lobe==0 ? .85f : .48f+Lobe*.035f);
        const int32 Instance=Pool.Instance*5+Lobe;
        Pools->UpdateInstanceTransform(Instance,FTransform(FRotator(0,FMath::RadiansToDegrees(Angle),0),Pool.Position+Offset,FVector(LobeScale,LobeScale*.8f,.003f)),true,false);
        Pools->SetCustomDataValue(Instance,0,Pool.Color.R,false); Pools->SetCustomDataValue(Instance,1,Pool.Color.G,false); Pools->SetCustomDataValue(Instance,2,Pool.Color.B,false);
    }
    Pools->MarkRenderStateDirty();
}
void ACELiquidSystem::Impact(FCELiquidDrop& Drop, const FHitResult& Hit)
{
    const float Volume=Drop.Espresso+Drop.Milk+Drop.Water;
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>();
    if(auto* Cup=Cast<ACEProp>(Hit.GetActor()))
    {
        if(Cup->IsContainer() && !Cup->bBroken && Cup->GetActorUpVector().Z>.7f && Drop.Velocity.Z<0.f && Hit.ImpactPoint.Z>=Cup->GetActorLocation().Z+7.f)
        {
            const float Added=Cup->AddLiquid(Drop.Espresso,Drop.Milk,Drop.Water,Drop.Temperature);
            TransferredMl+=Added;
            if(Added<Volume-.01f)
            {
                FHitResult Floor; FCollisionQueryParams Q; Q.AddIgnoredActor(Cup);
                const FVector Start=Cup->GetActorLocation()+FVector(24,0,0);
                if(GetWorld()->LineTraceSingleByChannel(Floor,Start,Start-FVector(0,0,600),ECC_Visibility,Q)) Splash(Floor.ImpactPoint,Drop.Color,Volume-Added);
                if(Drop.Source.IsValid()) Drop.Source->SpillConfirmed(Volume-Added);
            }
            return;
        }
        if(Cup->Kind==ECEPropKind::Laptop && Volume>1.f && !Cup->bBroken)
        {
            Cup->bBroken=true; Cup->DisplayName=TEXT("Portátil ahogado");
            Cafe->Observe(TEXT("laptop_destroyed"),TEXT(""),Cup->DisplayName,1.f); Cafe->Sound(TEXT("Electric"),Hit.ImpactPoint);
            for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(FVector::Dist2D(It->GetActorLocation(),Cup->GetActorLocation())<170.f) It->React(TEXT("laptop_destroyed"),Cup->DisplayName,1.f);
        }
    }
    if(auto* Customer=Cast<ACECustomer>(Hit.GetActor()))
    {
        SoakedMl+=Volume;
        Customer->React(TEXT("coffee_spill"),TEXT("Café caliente"),Volume*.012f);
        if(Customer->HitCount==1) CEVisual::Stain(GetWorld(),Customer->GetActorTransform().TransformPosition(FVector(-21.5f,0,15)),false,Customer);
    }
    else if(auto* MovingProp=Cast<ACEProp>(Hit.GetActor())) CEVisual::Stain(GetWorld(),Hit.ImpactPoint+Hit.ImpactNormal*.8f,Hit.ImpactNormal.Z>.65f,MovingProp);
    else if(Hit.ImpactNormal.Z>.65f) Splash(Hit.ImpactPoint,Drop.Color,Volume);
    else CEVisual::Stain(GetWorld(),Hit.ImpactPoint+Hit.ImpactNormal*.8f,false,Hit.GetActor());
    if(Drop.Source.IsValid()) Drop.Source->SpillConfirmed(Volume);
    // One subtle splash per impact group, not one sound per drop.
    if(FMath::RandRange(0,5)==0) Cafe->Sound(TEXT("Splash"),Hit.ImpactPoint,.3f);
}
void ACELiquidSystem::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    for(int32 i=0;i<Particles.Num();++i)
    {
        auto& Drop=Particles[i]; if(!Drop.bAlive) continue;
        Drop.Age+=DeltaSeconds;
        const FVector Old=Drop.Position;
        Drop.Velocity.Z-=980.f*DeltaSeconds; Drop.Position+=Drop.Velocity*DeltaSeconds;
        FHitResult Hit; FCollisionQueryParams Q; Q.AddIgnoredActor(this);
        if(Drop.Source.IsValid()) Q.AddIgnoredActor(Drop.Source.Get());
        if(GetWorld()->LineTraceSingleByChannel(Hit,Old,Drop.Position,ECC_Visibility,Q)) { Impact(Drop,Hit); Drop.bAlive=false; }
        if(Drop.Age>3.f || Drop.Position.Z< -30.f) Drop.bAlive=false;
        const float Amount=Drop.Espresso+Drop.Milk+Drop.Water;
        const FQuat Rotation=FQuat::FindBetweenNormals(FVector::UpVector,Drop.Velocity.GetSafeNormal());
        const float Width=FMath::Clamp(FMath::Sqrt(Amount)*.007f,.008f,.025f);
        // Overlapping segments follow velocity; the falling stream stays visually connected.
        const float Length=FMath::Clamp(Drop.Velocity.Size()*.00034f,.045f,.30f);
        const FVector Scale=Drop.bAlive ? FVector(Width,Width,Length) : FVector::ZeroVector;
        Drops->UpdateInstanceTransform(i,FTransform(Rotation,Drop.Position-Drop.Velocity*.009f,Scale),true,false);
    }
    Drops->MarkRenderStateDirty();
    const float Now=GetWorld()->GetTimeSeconds();
    if(Now<NextSlipAt) return; NextSlipAt=Now+.05f;
    for(auto& Pool:Puddles)
    {
        if(Pool.Until<Now)
        {
            for(int32 Lobe=0;Lobe<5;++Lobe) Pools->UpdateInstanceTransform(Pool.Instance*5+Lobe,FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector),true,true);
            continue;
        }
        if(Pool.Position.Z>5.f || Pool.Ml<45.f) continue;
        for(TActorIterator<ACECustomer> It(GetWorld());It;++It)
        {
            if(!It->bLeaving && Now-It->LastSlipAt>6.f && FVector::Dist2D(It->GetActorLocation(),Pool.Position)<Pool.Radius)
            {
                It->LastSlipAt=Now; ++SlipCount; It->React(TEXT("customer_slipped"),TEXT("Charco de café"),.5f);
                It->Fall(It->GetActorForwardVector());
                GetWorld()->GetSubsystem<UCECafeSubsystem>()->Observe(TEXT("customer_slipped"),It->CustomerId,TEXT("Charco"),.6f);
            }
        }
        if(auto* Player=Cast<ACEPlayer>(UGameplayStatics::GetPlayerPawn(this,0)))
        {
            if(Now>PlayerSlipUntil && FVector::Dist2D(Player->GetActorLocation(),Pool.Position)<Pool.Radius && Player->GetVelocity().Size2D()>280.f)
            {
                PlayerSlipUntil=Now+4.f; ++SlipCount;
                Player->SlipUntil=Now+1.1f;
                Player->GetCharacterMovement()->AddImpulse(Player->GetActorForwardVector()*170.f,true);
                GetWorld()->GetSubsystem<UCECafeSubsystem>()->Notify(TEXT("¡Suelo mojado! Correr aquí tiene consecuencias."));
            }
        }
    }
}
