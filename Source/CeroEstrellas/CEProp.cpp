#include "CEGameplay.h"
#include "CEKitchen.h"
#include "CEVisual.h"
#include "Components/StaticMeshComponent.h"
#include "PhysicsEngine/BodyInstance.h"
#include "PhysicalMaterials/PhysicalMaterial.h"
#include "Materials/MaterialInstanceDynamic.h"
#include "EngineUtils.h"
#include "Components/SkeletalMeshComponent.h"

ACEProp::ACEProp()
{
    PrimaryActorTick.bCanEverTick=true;
    Body=CreateDefaultSubobject<UStaticMeshComponent>(TEXT("PhysicsBody")); SetRootComponent(Body);
    Body->SetCollisionProfileName(TEXT("PhysicsActor")); Body->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
    Body->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block); Body->SetNotifyRigidBodyCollision(true);
    Body->BodyInstance.bUseCCD=true; Body->OnComponentHit.AddDynamic(this,&ACEProp::Hit);
}
void ACEProp::Configure(ECEPropKind InKind)
{
    Kind=InKind;
    const bool bCup=Kind==ECEPropKind::Cup, bFilter=Kind==ECEPropKind::Portafilter, bTray=Kind==ECEPropKind::Tray, bLaptop=Kind==ECEPropKind::Laptop, bPitcher=Kind==ECEPropKind::Pitcher, bChair=Kind==ECEPropKind::Chair;
    const TCHAR* Shape=(bCup || bPitcher || bFilter) ? TEXT("Cylinder") : (bTray || bLaptop || bChair) ? TEXT("Cube") : TEXT("Sphere");
    UStaticMesh* Mesh=nullptr;
    const TCHAR* Authored=bCup ? TEXT("SM_Cup") : bPitcher ? TEXT("SM_Pitcher") : bFilter ? TEXT("SM_Filter") : Kind==ECEPropKind::Pastry ? TEXT("SM_Croissant") : bTray || bChair || bLaptop ? TEXT("SM_BevelCube") : nullptr;
    if(Authored) Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Game/Prototype/Meshes/%s.%s"),Authored,Authored));
    if(!Mesh) Mesh=LoadObject<UStaticMesh>(nullptr,*FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),Shape,Shape));
    Body->SetStaticMesh(Mesh);
    Body->SetWorldScale3D(bCup || bFilter ? FVector(1.f) : bPitcher ? FVector(1.15f,1.15f,1.15f) : bChair ? FVector(.52f,.55f,.07f) : bTray ? FVector(1.1f,.7f,.035f) : bLaptop ? FVector(.42f,.32f,.035f) : FVector(.32f,.18f,.12f));
    const FLinearColor Color=bCup ? FLinearColor(.82f,.78f,.67f) : bTray ? FLinearColor(.06f,.25f,.22f) : bChair ? FLinearColor(.025f,.19f,.16f) : bPitcher ? FLinearColor(.65f,.68f,.67f) : bFilter || bLaptop ? FLinearColor(.23f,.27f,.28f) : FLinearColor(.8f,.35f,.07f);
    Body->SetMaterial(0,CEVisual::Material(this,Color,false,bFilter || bLaptop || bPitcher));
    if(bCup) Body->SetMaterial(0,CEVisual::Surface(this,Color,TEXT("M_Ceramic")));
    if(Kind==ECEPropKind::Pastry) Body->SetMaterial(0,CEVisual::Surface(this,FLinearColor(.78f,.34f,.075f),TEXT("M_Pastry")));
    auto* Physical=NewObject<UPhysicalMaterial>(this);
    Physical->Friction=bTray ? .55f : .36f; Physical->Restitution=Kind==ECEPropKind::Pastry ? .68f : .12f;
    Body->SetPhysMaterialOverride(Physical); Body->SetSimulatePhysics(true);
    Body->SetMassOverrideInKg(NAME_None,bCup ? .35f : bPitcher ? .65f : bChair ? 3.5f : bTray ? 1.1f : bFilter ? .45f : bLaptop ? 1.4f : .14f,true);
    Body->SetLinearDamping(.15f); Body->SetAngularDamping(.45f);
    if(bChair) { Body->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Ignore); Body->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block); }
    DisplayName=bPitcher ? TEXT("Jarra de leche") : bChair ? TEXT("Silla del cliente") : bCup ? TEXT("Taza vacía") : bTray ? TEXT("Bandeja") : bFilter ? TEXT("Portafiltro vacío") : bLaptop ? TEXT("Portátil del cliente") : TEXT("Cruasán recién hecho");
    auto Add=[this](const TCHAR* S,FVector L,FVector Scale,FLinearColor C)
    {
        auto* Part=CEVisual::Part(this,S,L/Body->GetComponentScale(),Scale/Body->GetComponentScale(),C); Details.Add(Part); return Part;
    };
    if(bCup || bPitcher)
    {
        Coffee=Add(TEXT("Cylinder"),FVector(0,0,-9),FVector(.185f,.185f,.012f),FLinearColor(.08f,.023f,.01f));
        Coffee->SetMaterial(0,CEVisual::Surface(this,FLinearColor(.08f,.023f,.01f),TEXT("M_Ceramic")));
        Coffee->SetVisibility(false);
        Foam=Add(TEXT("Sphere"),FVector(0,0,10),FVector(.17f,.17f,.035f),FLinearColor(.93f,.87f,.70f));
        Foam->SetMaterial(0,CEVisual::Surface(this,FLinearColor(.93f,.87f,.70f),TEXT("M_Fabric")));
        Foam->SetVisibility(false);
        // Hollow handles and the milk spout are now part of the authored shell.
    }
    if(bChair)
    {
        auto* Cushion=Add(TEXT("SM_BevelCube"),FVector(0,0,5),FVector(.50f,.51f,.035f),Color);
        Cushion->SetMaterial(0,CEVisual::Surface(this,Color,TEXT("M_Fabric")));
        auto* Back=Add(TEXT("SM_SeatBack"),FVector(24,0,41),FVector(.08f,.55f,.79f),Color);
        Back->SetCollisionProfileName(TEXT("PhysicsActor")); Back->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Ignore);
        Back->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block); Back->WeldTo(Body,NAME_None,true);
        auto* Padding=Add(TEXT("SM_SeatBack"),FVector(19,0,41),FVector(.025f,.48f,.68f),Color);
        Padding->SetMaterial(0,CEVisual::Surface(this,Color,TEXT("M_Fabric")));
        for(int32 X:{-1,1}) for(int32 Y:{-1,1})
        {
            auto* Leg=Add(TEXT("Cube"),FVector(X*20.f,Y*22.f,-24),FVector(.045f,.045f,.48f),FLinearColor(.012f,.065f,.055f));
            Leg->SetCollisionProfileName(TEXT("PhysicsActor")); Leg->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Ignore);
            Leg->SetCollisionResponseToChannel(ECC_Pawn,ECR_Block); Leg->WeldTo(Body,NAME_None,true);
            Add(TEXT("Cube"),FVector(X*20.f,Y*22.f,-44),FVector(.047f,.047f,.07f),FLinearColor(.42f,.30f,.14f));
        }
        Body->SetCenterOfMass(FVector(0,0,-12));
    }
    if(bTray)
    {
        Add(TEXT("SM_TrayRim"),FVector::ZeroVector,FVector(1),FLinearColor(.025f,.1f,.09f));
        for(int32 Side:{-1,1})
        {
            auto* Edge=Add(TEXT("Cube"),FVector(0,Side*34.f,3),FVector(1.1f,.025f,.07f),FLinearColor(.025f,.1f,.09f));
            auto* End=Add(TEXT("Cube"),FVector(Side*54.f,0,3),FVector(.025f,.7f,.07f),FLinearColor(.025f,.1f,.09f));
            for(auto* Rim:{Edge,End})
            {
                Rim->SetVisibility(false);
                Rim->SetCollisionProfileName(TEXT("PhysicsActor")); Rim->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
                Rim->WeldTo(Body,NAME_None,true);
            }
        }
    }
    if(bFilter)
    {
        Add(TEXT("SM_FilterGrip"),FVector::ZeroVector,FVector(1),FLinearColor(.02f,.025f,.025f));
        Coffee=Add(TEXT("Cylinder"),FVector(0,0,3.1f),FVector(.17f,.17f,.01f),FLinearColor(.055f,.022f,.009f));
        Coffee->SetVisibility(false);
    }
    if(bLaptop)
    {
        auto* Lid=Add(TEXT("Cube"),FVector(17,0,13),FVector(.025f,.32f,.27f),Color); Lid->SetRelativeRotation(FRotator(0,0,-12));
        auto* Screen=Add(TEXT("Cube"),FVector(15.5f,0,13),FVector(.007f,.28f,.23f),FLinearColor(.025f,.10f,.12f));
        Screen->SetMaterial(0,CEVisual::Surface(this,FLinearColor::White,TEXT("M_Screen")));
        for(int32 Row=0;Row<4;++Row) Add(TEXT("Cube"),FVector(-8+Row*5,0,2),FVector(.02f,.25f,.009f),FLinearColor(.025f,.03f,.03f));
    }
    // Pastry folds and crescent silhouette are baked into SM_Croissant.
}
float ACEProp::AddLiquid(float Espresso,float Milk,float Water,float Degrees)
{
    if(!IsContainer() || bBroken) return 0.f;
    const float Amount=Espresso+Milk+Water, Old=VolumeMl();
    if(Amount<=0.f) return 0.f;
    const float Added=FMath::Min(Amount,FMath::Max(0.f,CapacityMl()-Old));
    const float Ratio=Added/Amount;
    EspressoMl+=Espresso*Ratio; MilkMl+=Milk*Ratio; WaterMl+=Water*Ratio;
    Temperature=(Temperature*Old+Degrees*Added)/FMath::Max(1.f,Old+Added);
    UpdateLiquidVisual(); return Added;
}
void ACEProp::UpdateLiquidVisual()
{
    bHasCoffee=VolumeMl()>.2f;
    if(Kind==ECEPropKind::Portafilter)
    {
        if(Coffee) Coffee->SetVisibility(bGroundDose);
        DisplayName=bGroundDose ? bTamped ? TEXT("Portafiltro prensado") : TEXT("Portafiltro dosificado / falta prensar") : TEXT("Portafiltro vacío"); return;
    }
    if(!IsContainer() || !Coffee) return;
    Coffee->SetVisibility(bHasCoffee && !bBroken);
    if(Foam) {
        Foam->SetVisibility(bHasCoffee && FoamMl>.5f && !bBroken);
        const float Z=-9.f+VolumeMl()/CapacityMl()*19.f;
        Foam->SetRelativeLocation(FVector(0,0,Z+FoamMl*.07f)/Body->GetComponentScale());
        Foam->SetRelativeScale3D(FVector(.17f,.17f,.012f+FoamMl*.0018f)/Body->GetComponentScale());
    }
    Coffee->SetRelativeLocation(FVector(0,0,-9.f+VolumeMl()/CapacityMl()*19.f)/Body->GetComponentScale());
    const float MilkRatio=MilkMl/FMath::Max(1.f,VolumeMl());
    if(auto* Mat=Cast<UMaterialInstanceDynamic>(Coffee->GetMaterial(0))) Mat->SetVectorParameterValue(TEXT("Tint"),FMath::Lerp(FLinearColor(.055f,.015f,.005f),FLinearColor(.87f,.76f,.51f),MilkRatio));
    DisplayName=Kind==ECEPropKind::Pitcher ? (Temperature>=55.f ? TEXT("Jarra de leche caliente") : TEXT("Jarra de leche fría")) : !bHasCoffee ? TEXT("Taza vacía") : MilkMl>30.f ? TEXT("Café con leche") : WaterMl>50.f ? TEXT("Americano") : EspressoMl>5.f ? TEXT("Espresso") : TEXT("Agua caliente");
}
void ACEProp::MarkThrown()
{
    bThrown=true; GetWorld()->GetSubsystem<UCECafeSubsystem>()->Observe(TEXT("object_thrown"),TEXT(""),DisplayName,.15f);
}
void ACEProp::SpillConfirmed(float Amount)
{
    SpillSinceEvent+=Amount;
    const float Now=GetWorld()->GetTimeSeconds();
    if(SpillSinceEvent>=25.f && Now-LastSpillEvent>4.f)
    {
        GetWorld()->GetSubsystem<UCECafeSubsystem>()->Observe(TEXT("coffee_spill"),TEXT(""),DisplayName,FMath::Clamp(SpillSinceEvent/300.f,.1f,.5f));
        LastSpillEvent=Now; SpillSinceEvent=0.f;
    }
}
bool ACEProp::Pour()
{
    if(!IsContainer() || !bHasCoffee || bBroken) return false;
    bPourRequested=true; return true;
}
void ACEProp::Tick(float DeltaSeconds)
{
    Super::Tick(DeltaSeconds);
    if(IsContainer() && bHasCoffee && !bBroken)
    {
        Temperature=FMath::Max(20.f,Temperature-DeltaSeconds*.08f);
        const float Tilt=1.f-FMath::Clamp(GetActorUpVector().Z,0.f,1.f);
        if(Tilt>.24f || bPourRequested)
        {
            auto* Liquid=GetWorld()->GetSubsystem<UCECafeSubsystem>()->Liquids.Get();
            DropAccumulator+=DeltaSeconds;
            if(Liquid && DropAccumulator>=.025f)
            {
                const float Amount=FMath::Min(VolumeMl(),DropAccumulator*(bPourRequested ? 140.f : 150.f*Tilt));
                const float Ratio=Amount/FMath::Max(.01f,VolumeMl());
                const FVector Down=FVector::VectorPlaneProject(FVector::DownVector,GetActorUpVector()).GetSafeNormal();
                const FVector Lip=GetActorLocation()+GetActorUpVector()*10.f+Down*10.f;
                FVector Flow=Down*85.f+Body->GetPhysicsLinearVelocity()*.35f+FVector(0,0,-45);
                if(bAimedPour && bPourRequested)
                {
                    const float Flight=FMath::Sqrt(FMath::Max(10.f,Lip.Z-PourTarget.Z)*2.f/980.f);
                    Flow=(PourTarget-Lip)/FMath::Max(.12f,Flight); Flow.Z+=490.f*Flight;
                }
                if(Liquid->Emit(Lip,Flow,EspressoMl*Ratio,MilkMl*Ratio,WaterMl*Ratio,Temperature,this))
                {
                    FoamMl*=1.f-Ratio; SugarGrams*=1.f-Ratio; SyrupMl*=1.f-Ratio;
                    EspressoMl*=1.f-Ratio; MilkMl*=1.f-Ratio; WaterMl*=1.f-Ratio; UpdateLiquidVisual();
                }
                DropAccumulator=0.f;
            }
        }
        else DropAccumulator=0.f;
    }
    if(bBroken && Kind==ECEPropKind::Laptop && Details.Num()>1)
    {
        if(auto* Mat=Cast<UMaterialInstanceDynamic>(Details[1]->GetMaterial(0))) Mat->SetVectorParameterValue(TEXT("Tint"),FLinearColor(.01f,.01f,.01f));
    }
    if(GetActorLocation().Z< -200.f) Destroy();
}
void ACEProp::Hit(UPrimitiveComponent* HitComponent,AActor* OtherActor,UPrimitiveComponent* OtherComponent,FVector NormalImpulse,const FHitResult& HitResult)
{
    const float Now=GetWorld()->GetTimeSeconds();
    if(Now-LastHitAt<.5f) return;
    auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>();
    if(Kind==ECEPropKind::Chair && NormalImpulse.Size()>220.f)
    {
        LastHitAt=Now; Cafe->Sound(TEXT("Thump"),HitResult.ImpactPoint,.4f);
        for(TActorIterator<ACECustomer> It(GetWorld());It;++It)
            if(!It->bLeaving && FVector::Dist2D(It->GetActorLocation(),GetActorLocation())<90.f && Now>It->FallenUntil)
                It->Fall((It->GetActorLocation()-GetActorLocation()).GetSafeNormal2D());
    }
    if(bThrown)
    {
        if(auto* Customer=Cast<ACECustomer>(OtherActor))
        {
            LastHitAt=Now; Customer->React(TEXT("customer_hit"),DisplayName,Kind==ECEPropKind::Pastry ? .85f : .9f); Cafe->Sound(TEXT("Thump"),HitResult.ImpactPoint); bThrown=false;
            if(Kind==ECEPropKind::Pastry && Customer->Rig) {
                Body->SetSimulatePhysics(false); Body->SetCollisionEnabled(ECollisionEnabled::QueryOnly); bStuck=true;
                AttachToComponent(Customer->Rig,FAttachmentTransformRules::KeepWorldTransform,TEXT("head"));
                SetActorLocation(Customer->HeadLocation()+Customer->GetActorQuat().RotateVector(FVector(-16,0,0)));
                SetLifeSpan(8.f); Cafe->Observe(TEXT("pastry_stuck"),Customer->CustomerId,TEXT("Cruasán pegado en la cara"),.3f);
            }
        }
        if(Kind==ECEPropKind::Cup && NormalImpulse.Size()>200.f && !bHeld)
        {
            LastHitAt=Now; bBroken=true; bThrown=false; DisplayName=TEXT("Taza rota");
            Cafe->Observe(TEXT("ceramic_broken"),TEXT(""),DisplayName,.5f); Cafe->Sound(TEXT("Ceramic"),GetActorLocation());
            if(VolumeMl()>0.f && Cafe->Liquids)
            {
                Cafe->Liquids->Emit(GetActorLocation()+FVector(0,0,5),FVector(30,0,-40),EspressoMl,MilkMl,WaterMl,Temperature,this);
                EspressoMl=MilkMl=WaterMl=0.f; UpdateLiquidVisual();
            }
            Body->SetSimulatePhysics(false); Body->SetVisibility(false); Body->SetCollisionEnabled(ECollisionEnabled::NoCollision);
            for(auto Detail:Details) Detail->SetVisibility(false);
            int32 Count=0; for(TActorIterator<AActor> It(GetWorld());It;++It) if(It->ActorHasTag(TEXT("CeramicShard"))) ++Count;
            for(int32 i=0;i<6 && Count+i<36;++i)
            {
                auto* Shard=GetWorld()->SpawnActor<AActor>(); Shard->Tags.Add(TEXT("CeramicShard"));
                auto* Piece=CEVisual::Part(Shard,TEXT("Cube"),FVector::ZeroVector,FVector(.045f,.06f,.022f),FLinearColor(.82f,.78f,.67f));
                Shard->SetActorLocationAndRotation(GetActorLocation()+FVector(FMath::RandRange(-9.f,9.f),FMath::RandRange(-9.f,9.f),i*2.f),FRotator(FMath::RandRange(0.f,90.f),i*60.f,25.f));
                Piece->SetCollisionProfileName(TEXT("PhysicsActor")); Piece->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
                Piece->SetSimulatePhysics(true); Piece->SetMassOverrideInKg(NAME_None,.025f);
                Piece->SetPhysicsLinearVelocity(FVector(FMath::RandRange(-150.f,150.f),FMath::RandRange(-150.f,150.f),FMath::RandRange(80.f,180.f)));
                Shard->SetLifeSpan(20.f);
            }
            SetLifeSpan(2.f);
        }
    }
}
