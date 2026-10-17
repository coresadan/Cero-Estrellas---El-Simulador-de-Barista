#include "CEKitchen.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Kismet/GameplayStatics.h"
#include "GameFramework/CharacterMovementComponent.h"
#include "EngineUtils.h"
#include "CEVisual.h"
#include "HAL/PlatformTime.h"
#include "ProfilingDebugging/CpuProfilerTrace.h"

ACELiquidSystem::ACELiquidSystem()
{
    PrimaryActorTick.bCanEverTick=true;
    auto* Root=CreateDefaultSubobject<USceneComponent>(TEXT("LiquidRoot")); SetRootComponent(Root);
    Drops=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("BallisticDrops")); Drops->SetupAttachment(Root);
    Streams=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ContinuousStreams")); Streams->SetupAttachment(Root);
    Pools=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("PersistentPuddles")); Pools->SetupAttachment(Root);
    Steam=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("SteamPuffs")); Steam->SetupAttachment(Root);
    ImpactSpray=CreateDefaultSubobject<UInstancedStaticMeshComponent>(TEXT("ImpactSpray")); ImpactSpray->SetupAttachment(Root);
    for(auto* Mesh : {Drops.Get(),Streams.Get(),Pools.Get(),Steam.Get(),ImpactSpray.Get()})
    {
        Mesh->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        Mesh->SetCastShadow(false); Mesh->NumCustomDataFloats=3;
    }
}
void ACELiquidSystem::BeginPlay()
{
    Super::BeginPlay();
    Drops->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Pools->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Game/Prototype/Meshes/SM11_Puddle.SM11_Puddle")));
    auto* Mat=LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Prototype/M_Liquid.M_Liquid"));
    Drops->SetMaterial(0,Mat); Pools->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Prototype/M11_Puddle.M11_Puddle")));
    ImpactSpray->SetStaticMesh(Drops->GetStaticMesh()); ImpactSpray->SetMaterial(0,Mat);
    ImpactParticles.SetNum(48); for(int32 i=0;i<48;++i) ImpactSpray->AddInstance(FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector));
    Particles.SetNum(192);
    Streams->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Cylinder.Cylinder"))); Streams->SetMaterial(0,Mat);
    Flows.SetNum(8); for(int32 i=0;i<64;++i) Streams->AddInstance(FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector));
    Steam->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,TEXT("/Engine/BasicShapes/Sphere.Sphere")));
    Steam->SetMaterial(0,LoadObject<UMaterialInterface>(nullptr,TEXT("/Game/Prototype/M_Steam.M_Steam")));
    Puffs.SetNum(24); for(int32 i=0;i<24;++i) Steam->AddInstance(FTransform(FQuat::Identity,FVector::ZeroVector,FVector::ZeroVector));
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
        Drop.Position=Origin; Drop.PreviousPosition=Origin; Drop.Velocity=Velocity; Drop.Age=0.f;
        Drop.Espresso=Espresso; Drop.Milk=Milk; Drop.Water=Water; Drop.Temperature=Degrees;
        const float Fraction=Source && Source->IsContainer() ? (Espresso+Milk+Water)/FMath::Max(.01f,Source->VolumeMl()) : 0.f;
        Drop.Foam=Source ? Source->FoamMl*Fraction : 0.f;
        Drop.Sugar=Source ? Source->SugarGrams*Fraction : 0.f;
        Drop.Syrup=Source ? Source->SyrupMl*Fraction : 0.f;
        Drop.Color=FMath::Lerp(FLinearColor(.09f,.025f,.008f),FLinearColor(.88f,.81f,.61f),FMath::Clamp(Milk/(Espresso+Milk+Water),0.f,1.f));
        if(Espresso<.01f && Milk<.01f) Drop.Color=FLinearColor(.35f,.65f,.7f);
        Drop.Source=Source; Drop.bAlive=true; Cursor=(Index+1)%Particles.Num();
        Drops->SetCustomDataValue(Index,0,Drop.Color.R,false); Drops->SetCustomDataValue(Index,1,Drop.Color.G,false); Drops->SetCustomDataValue(Index,2,Drop.Color.B,false);
        EmittedMl+=Espresso+Milk+Water; PeakActiveDrops=FMath::Max(PeakActiveDrops,ActiveDrops());
        if(Source) {
            const float Now=GetWorld()->GetTimeSeconds(); FFlow* Flow=nullptr;
            for(auto& F:Flows) if(F.Source.Get()==Source) { Flow=&F; break; }
            if(!Flow) for(auto& F:Flows) if(F.Until<Now) { Flow=&F; break; }
            if(Flow) {
                if(Flow->Until<Now || Now>=Flow->NextPredictAt) {
                    Flow->NextPredictAt=Now+.25f; Flow->Flight=.56f;
                    FCollisionQueryParams Query; Query.AddIgnoredActor(this); Query.AddIgnoredActor(Source);
                    FVector Previous=Origin;
                    for(int32 PredictionIndex=1;PredictionIndex<=8;++PredictionIndex) {
                        const float T=PredictionIndex*.07f; const FVector Next=Origin+Velocity*T+FVector(0,0,-490.f*T*T);
                        FHitResult Hit; ++CollisionQueries;
                        if(GetWorld()->LineTraceSingleByChannel(Hit,Previous,Next,ECC_Visibility,Query)) {
                            Flow->Flight=FMath::Max(.025f,T-.07f+.07f*Hit.Time); break;
                        }
                        Previous=Next;
                    }
                }
                Flow->Source=Source; Flow->Origin=Origin; Flow->Velocity=Velocity; Flow->Until=Now+.12f;
                const int32 Base=int32(Flow-Flows.GetData())*8;
                for(int32 i=Base;i<Base+8;++i) { Streams->SetCustomDataValue(i,0,Drop.Color.R,false); Streams->SetCustomDataValue(i,1,Drop.Color.G,false); Streams->SetCustomDataValue(i,2,Drop.Color.B,false); }
            }
        }
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
    for(auto& Flow:Flows) if(Drop.Source.IsValid() && Flow.Source==Drop.Source) Flow.Flight=FMath::Clamp(Drop.Age,.06f,.7f);
    if(auto* Cup=Cast<ACEProp>(Hit.GetActor()))
    {
        if(Cup->IsContainer() && !Cup->bBroken && Cup->GetActorUpVector().Z>.7f && Drop.Velocity.Z<0.f && Hit.ImpactPoint.Z>=Cup->GetActorLocation().Z+7.f)
        {
            const float Added=Cup->AddLiquid(Drop.Espresso,Drop.Milk,Drop.Water,Drop.Temperature);
            const float Accepted=Added/FMath::Max(.01f,Volume);
            Cup->FoamMl+=Drop.Foam*Accepted; Cup->SugarGrams+=Drop.Sugar*Accepted; Cup->SyrupMl+=Drop.Syrup*Accepted;
            Cup->UpdateLiquidVisual();
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
        if(Cup->IsDevice() && Volume>1.f && !Cup->bBroken) Cup->DamageDevice();
        if(!Cup->OwnerCustomerId.IsEmpty() && Cup->Kind==ECEPropKind::Notebook && !Cup->bBroken && Volume>1.f) {
            Cup->bBroken=true; Cup->DisplayName=TEXT("Cuaderno empapado");
            for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(It->CustomerId==Cup->OwnerCustomerId) {
                It->React(TEXT("belonging_damaged"),TEXT("Notas empapadas"),.4f);
                It->Speak(TEXT("¡Mis notas! Ahora mi historia se ha disuelto en café."),5.f); break;
            }
        }
    }
    Spray(Hit.ImpactPoint,Hit.ImpactNormal,Drop.Color);
    if(auto* Customer=Cast<ACECustomer>(Hit.GetActor()))
    {
        SoakedMl+=Volume;
        const FString LiquidName=Drop.Espresso>.01f ? TEXT("café") : Drop.Milk>.01f ? TEXT("leche") : TEXT("agua");
        Customer->Soak(Volume,LiquidName);
        Customer->React(TEXT("coffee_spill"),LiquidName+(Drop.Temperature>45.f ? TEXT(" caliente") : TEXT(" fría")),Volume*.012f);
    }
    else if(auto* MovingProp=Cast<ACEProp>(Hit.GetActor())) {
        UnresolvedMl+=Volume;
        // The generic prop stain is 85 cm wide. Small electronics already show
        // damage through their screen and the impact spray; don't bury them in it.
        if(!MovingProp->IsDevice()) CEVisual::Stain(GetWorld(),Hit.ImpactPoint+Hit.ImpactNormal*.8f,Hit.ImpactNormal.Z>.65f,MovingProp);
    }
    else if(Hit.ImpactNormal.Z>.65f) Splash(Hit.ImpactPoint,Drop.Color,Volume);
    else { UnresolvedMl+=Volume; CEVisual::Stain(GetWorld(),Hit.ImpactPoint+Hit.ImpactNormal*.8f,false,Hit.GetActor()); }
    if(Drop.Source.IsValid()) Drop.Source->SpillConfirmed(Volume);
    // One subtle splash per impact group, not one sound per drop.
    if(FMath::RandRange(0,5)==0) Cafe->Sound(TEXT("Splash"),Hit.ImpactPoint,.3f);
}
float ACELiquidSystem::InFlightMl() const { float Ml=0; for(const auto& D:Particles) if(D.bAlive) Ml+=D.Espresso+D.Milk+D.Water; return Ml; }
void ACELiquidSystem::Spray(FVector Position,FVector Normal,FLinearColor Color) {
    const float Now=GetWorld()->GetTimeSeconds(); if(Now<NextImpactSprayAt || ImpactParticles.IsEmpty()) return;
    NextImpactSprayAt=Now+.08f;
    for(int32 i=0;i<6;++i) {
        const int32 Index=ImpactCursor++%ImpactParticles.Num(); auto& P=ImpactParticles[Index];
        P.Position=Position+Normal*1.5f; P.Velocity=Normal*FMath::RandRange(25.f,65.f)+FMath::VRand()*45.f; P.Age=0;
        ImpactSpray->SetCustomDataValue(Index,0,Color.R,false); ImpactSpray->SetCustomDataValue(Index,1,Color.G,false); ImpactSpray->SetCustomDataValue(Index,2,Color.B,false);
    }
}
void ACELiquidSystem::Simulate(float Step)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(CE_Liquid_Collision);
    const double Start=FPlatformTime::Seconds(); ++SimulationSteps;
    for(auto& D:Particles) {
        if(!D.bAlive) continue;
        D.PreviousPosition=D.Position; D.Age+=Step; D.Velocity.Z-=980.f*Step; D.Position+=D.Velocity*Step;
        FHitResult Hit; FCollisionQueryParams Q; Q.AddIgnoredActor(this); if(D.Source.IsValid()) Q.AddIgnoredActor(D.Source.Get());
        ++CollisionQueries;
        if(GetWorld()->LineTraceSingleByChannel(Hit,D.PreviousPosition,D.Position,ECC_Visibility,Q)) { Impact(D,Hit); D.bAlive=false; }
        else if(D.Age>3.f || D.Position.Z< -30.f) { UnresolvedMl+=D.Espresso+D.Milk+D.Water; D.bAlive=false; }
    }
    SimulationCpuMs+=(FPlatformTime::Seconds()-Start)*1000.;
}
void ACELiquidSystem::Tick(float DeltaSeconds)
{
    TRACE_CPUPROFILER_EVENT_SCOPE(CE_Liquid_Tick);
    Super::Tick(DeltaSeconds);
    const float SteamNow=GetWorld()->GetTimeSeconds();
    bool SprayDirty=false;
    for(int32 i=0;i<ImpactParticles.Num();++i) {
        auto& P=ImpactParticles[i]; if(P.Age>=1.f) continue;
        P.Age+=DeltaSeconds; P.Velocity.Z-=490.f*DeltaSeconds; P.Position+=P.Velocity*DeltaSeconds;
        const float Size=.012f*FMath::Max(0.f,1.f-P.Age/.35f);
        ImpactSpray->UpdateInstanceTransform(i,FTransform(FQuat::Identity,P.Position,FVector(Size,Size,Size*1.5f)),true,false); SprayDirty=true;
        if(P.Age>=.35f) P.Age=1.f;
    }
    if(SprayDirty) ImpactSpray->MarkRenderStateDirty();
    SimulationAccumulator+=FMath::Min(DeltaSeconds,.133333f);
    constexpr float Step=1.f/30.f;
    while(SimulationAccumulator>=Step) { Simulate(Step); SimulationAccumulator-=Step; }
    bool DropsDirty=false;
    for(int32 i=0;i<Particles.Num();++i) {
        const auto& D=Particles[i]; if(!D.bAlive && D.Age==0.f) continue;
        const FVector P=FMath::Lerp(D.PreviousPosition,D.Position,SimulationAccumulator/Step);
        bool StreamVisible=false; if(D.Source.IsValid()) for(const auto& F:Flows) if(F.Source==D.Source && F.Until>SteamNow) { StreamVisible=true; break; }
        const float Width=FMath::Clamp(FMath::Sqrt(D.Espresso+D.Milk+D.Water)*.007f,.008f,.025f);
        const FVector Scale=D.bAlive && !StreamVisible ? FVector(Width,Width,FMath::Clamp(D.Velocity.Size()*.00034f,.045f,.30f)) : FVector::ZeroVector;
        Drops->UpdateInstanceTransform(i,FTransform(FQuat::FindBetweenNormals(FVector::UpVector,D.Velocity.GetSafeNormal()),P-D.Velocity*.009f,Scale),true,false);
        if(!D.bAlive) Particles[i].Age=0; DropsDirty=true;
    }
    if(DropsDirty) Drops->MarkRenderStateDirty();
    bool StreamsDirty=false;
    for(int32 f=0;f<Flows.Num();++f) {
        auto& Flow=Flows[f]; if(Flow.Until==0.f) continue; StreamsDirty=true; const bool Visible=Flow.Until>SteamNow && Flow.Source.IsValid();
        for(int32 j=0;j<8;++j) {
            const float A=Flow.Flight*j/8.f,B=Flow.Flight*(j+1)/8.f;
            const FVector P=Flow.Origin+Flow.Velocity*A+FVector(0,0,-490*A*A),Q=Flow.Origin+Flow.Velocity*B+FVector(0,0,-490*B*B);
            Streams->UpdateInstanceTransform(f*8+j,FTransform(FQuat::FindBetweenNormals(FVector::UpVector,(Q-P).GetSafeNormal()),(P+Q)*.5f,Visible ? FVector((Flow.Source.IsValid() && Flow.Source->IsContainer() ? .010f : .006f)*(1.f+.06f*FMath::Sin(SteamNow*25.f+j)),(Flow.Source.IsValid() && Flow.Source->IsContainer() ? .010f : .006f),(Q-P).Size()/100.f+.003f) : FVector::ZeroVector),true,false);
        }
        if(!Visible) Flow.Until=0.f;
    }
    if(StreamsDirty) Streams->MarkRenderStateDirty();
    if(SteamNow>=RegistryNextAt) {
        RegistryNextAt=SteamNow+1.f; HotContainers.Reset();
        for(TActorIterator<ACEProp> It(GetWorld());It;++It) if(It->IsContainer()) HotContainers.Add(*It);
    }
    if(SteamNow>=NextSteamAt) {
        NextSteamAt=SteamNow+.22f; int32 Count=0;
        for(const auto& Weak:HotContainers) if(auto* Prop=Weak.Get()) if(Prop->VolumeMl()>10.f && Prop->Temperature>55.f && !Prop->bBroken && Count<6) {
            auto& Puff=Puffs[SteamCursor]; Puff.Position=Prop->GetActorLocation()+FVector(0,0,11); Puff.Age=0; SteamCursor=(SteamCursor+1)%Puffs.Num(); ++Count;
        }
    }
    bool SteamDirty=false;
    for(int32 i=0;i<Puffs.Num();++i) {
        auto& Puff=Puffs[i]; if(Puff.Age>1.3f) continue;
        Puff.Age+=DeltaSeconds; Puff.Position+=FVector(FMath::Sin(SteamNow+i)*3.f,2.f,18.f)*DeltaSeconds;
        const float Size=Puff.Age<1.3f ? .035f+Puff.Age*.075f : 0.f;
        Steam->SetCustomDataValue(i,0,FMath::Sin(FMath::Clamp(Puff.Age/1.3f,0.f,1.f)*PI),false);
        Steam->UpdateInstanceTransform(i,FTransform(FQuat::Identity,Puff.Position,FVector(Size,Size,Size*1.6f)),true,false); SteamDirty=true;
    }
    if(SteamDirty) Steam->MarkRenderStateDirty();
    const float Now=GetWorld()->GetTimeSeconds();
    if(Now<NextSlipAt) return; NextSlipAt=Now+.1f;
    for(auto& Pool:Puddles)
    {
        if(Pool.Until<Now)
        {
            if(Pool.Until<0.f) continue;
            Pool.Until=-1.f;
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
