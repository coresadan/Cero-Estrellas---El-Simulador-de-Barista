#include "CEGameplay.h"
#include "CEVisual.h"
#include "Components/SkeletalMeshComponent.h"
#include "CEBodyAnimation.h"
#include "CEGuestMotion.h"
#include "PhysicsEngine/PhysicalAnimationComponent.h"
#include "EngineUtils.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/SkeletalMesh.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "Kismet/GameplayStatics.h"
#include "Sound/SoundBase.h"
#include "Sound/SoundAttenuation.h"
#include "Sound/SoundConcurrency.h"

bool ACECustomer::BuildRig(const FLinearColor& Outfit)
{
    const TCHAR* Names[]={TEXT("SK_Crypto"),TEXT("SK_Hipster"),TEXT("SK_Influencer")};
    const FString Path=FString::Printf(TEXT("/Game/Prototype/Characters/%s.%s"),Names[Personality],Names[Personality]);
    auto* Mesh=LoadObject<USkeletalMesh>(nullptr,*Path); if(!Mesh || !Mesh->GetSkeleton()) return false;
    VisualRoot=NewObject<USceneComponent>(this); AddInstanceComponent(VisualRoot);
    VisualRoot->SetupAttachment(Collision); VisualRoot->RegisterComponent();
    Rig=NewObject<USkeletalMeshComponent>(this); AddInstanceComponent(Rig);
    Rig->SetupAttachment(VisualRoot); Rig->SetRelativeLocation(FVector(0,0,-88));
    Rig->SetSkinnedAssetAndUpdate(Mesh); Rig->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Rig->SetAnimInstanceClass(UCEBodyAnimation::StaticClass());
    Rig->VisibilityBasedAnimTickOption=EVisibilityBasedAnimTickOption::AlwaysTickPoseAndRefreshBones;
    Rig->RegisterComponent(); Rig->RefreshBoneTransforms();
    Rig->SetNotifyRigidBodyCollision(true); Rig->OnComponentHit.AddDynamic(this,&ACECustomer::RagdollContact);
    PhysicalDriver=NewObject<UPhysicalAnimationComponent>(this); AddInstanceComponent(PhysicalDriver);
    PhysicalDriver->RegisterComponent(); PhysicalDriver->SetSkeletalMeshComponent(Rig);
    const FLinearColor Skins[]={FLinearColor(.64f,.40f,.27f),FLinearColor(.78f,.56f,.39f),FLinearColor(.42f,.23f,.14f),FLinearColor(.60f,.36f,.21f),FLinearColor(.28f,.14f,.085f),FLinearColor(.87f,.65f,.49f),FLinearColor(.54f,.29f,.17f),FLinearColor(.75f,.53f,.38f),FLinearColor(.36f,.19f,.12f)};
    const FLinearColor Outfits[]={Outfit,FLinearColor(.33f,.19f,.095f),FLinearColor(.65f,.25f,.40f),FLinearColor(.12f,.025f,.16f),FLinearColor(.83f,.84f,.69f),FLinearColor(.62f,.035f,.12f),FLinearColor(.07f,.085f,.09f),FLinearColor(.28f,.72f,.08f),FLinearColor(.055f,.09f,.38f)};
    const FLinearColor Hairs[]={FLinearColor(.033f,.022f,.018f),FLinearColor(.1f,.045f,.017f),FLinearColor(.23f,.10f,.03f),FLinearColor(.5f,.02f,.35f),FLinearColor(.08f,.05f,.025f),FLinearColor(.035f,.02f,.015f),FLinearColor(.015f,.015f,.02f),FLinearColor(.55f,.58f,.57f),FLinearColor(.16f,.045f,.02f)};
    const int32 Look=LookIndex<0 ? Personality : LookIndex%9;
    const FLinearColor Skin=Skins[Look],Cloth=LookIndex<0 ? Outfit : Outfits[Look];
    for(int32 i=0;i<Mesh->GetMaterials().Num();++i) {
        const FString Slot=Mesh->GetMaterials()[i].MaterialSlotName.ToString();
        const bool bSkin=Slot==TEXT("Skin"), bCloth=Slot==TEXT("Clothing"), bPants=Slot==TEXT("Trousers");
        auto* Mat=CEVisual::Surface(this,bSkin ? Skin : bCloth ? Cloth : bPants ? FLinearColor(.055f,.065f,.077f) : Hairs[Look],bSkin ? TEXT("M_Skin") : bCloth || bPants ? TEXT("M_Fabric") : TEXT("M_Surface"));
        Rig->SetMaterial(i,Mat); if(bCloth) ClothingMaterial=Mat;
    }
    const auto& Ref=Mesh->GetRefSkeleton();
    for(int32 i=0;i<Ref.GetNum();++i) {
        const FName Name=Ref.GetBoneName(i); const int32 Parent=Ref.GetParentIndex(i);
        RestBones.Add(Name,Parent<0 ? Ref.GetRefBonePose()[i] : Ref.GetRefBonePose()[i]*RestBones.FindRef(Ref.GetBoneName(Parent)));
    }
    HeadPivot=NewObject<USceneComponent>(this); AddInstanceComponent(HeadPivot);
    HeadPivot->SetupAttachment(VisualRoot); HeadPivot->RegisterComponent();
    auto Face=[this](const TCHAR* Shape,FVector Position,FVector Scale,FLinearColor Color) {
        auto* Part=CEVisual::Part(this,Shape,FVector::ZeroVector,Scale,Color);
        Part->AttachToComponent(HeadPivot,FAttachmentTransformRules::KeepRelativeTransform); Part->SetRelativeLocation(Position); return Part;
    };
    for(int32 i=0;i<2;++i) {
        const float Side=i==0 ? -1.f : 1.f;
        Eyes[i]=Face(TEXT("Sphere"),FVector(-11.3f,Side*4.8f,3),FVector(.021f,.043f,.025f),FLinearColor(.83f,.81f,.75f));
        Pupils[i]=Face(TEXT("Sphere"),FVector(-12.5f,Side*4.8f,3),FVector(.011f,.018f,.020f),FLinearColor(.075f,.045f,.025f));
        auto* Glint=Face(TEXT("Sphere"),FVector(-13.1f,Side*4.8f-.3f,3.5f),FVector(.005f,.008f,.008f),FLinearColor(.98f,.97f,.93f));
        Glint->AttachToComponent(Pupils[i],FAttachmentTransformRules::KeepWorldTransform);
        Brows[i]=Face(TEXT("Cube"),FVector(-11.8f,Side*4.8f,6.8f),FVector(.012f,.055f,.012f),FLinearColor(.04f,.026f,.02f));
        auto* Ear=Face(TEXT("Sphere"),FVector(0,Side*11.8f,0),FVector(.041f,.026f,.055f),Skin);
        Ear->SetMaterial(0,CEVisual::Surface(this,Skin,TEXT("M_Skin")));
    }
    Mouth=Face(TEXT("Sphere"),FVector(-11.9f,0,-7),FVector(.01f,.056f,.008f),FLinearColor(.19f,.055f,.045f));
    if(LookIndex<0 ? Personality==0 : Look==0 || Look==6 || Look==7) Face(TEXT("SM_Glasses"),FVector(1,0,-.5f),FVector(.74f),FLinearColor(.04f,.065f,.06f));
    if(LookIndex<0 ? Personality==1 : Look==1) {
        Face(TEXT("Cylinder"),FVector(0,0,16),FVector(.32f,.29f,.025f),FLinearColor(.18f,.095f,.035f));
        Face(TEXT("Cylinder"),FVector(0,0,20),FVector(.23f,.22f,.08f),FLinearColor(.24f,.14f,.06f));
        Face(TEXT("Sphere"),FVector(-10.4f,0,-9),FVector(.022f,.15f,.045f),FLinearColor(.09f,.04f,.016f));
    }
    if(Look==3 && LookIndex>=0) {
        for(int32 i=-2;i<=2;++i) Face(TEXT("Cone"),FVector(i*3.f,0,24),FVector(.07f,.06f,.24f),Hairs[Look]);
        for(float Side:{-1.f,1.f}) Face(TEXT("Sphere"),FVector(0,Side*13,0),FVector(.024f),FLinearColor(.75f,.66f,.30f));
    }
    if(Look==4 && LookIndex>=0) {
        Face(TEXT("Cylinder"),FVector(0,0,18),FVector(.36f,.33f,.08f),FLinearColor(.88f,.91f,.79f));
        Face(TEXT("Sphere"),FVector(0,0,22),FVector(.30f,.28f,.16f),FLinearColor(.88f,.91f,.79f));
        Face(TEXT("Cube"),FVector(-14,0,8),FVector(.03f,.16f,.03f),FLinearColor(.75f,.43f,.07f));
    }
    if(Look==5 && LookIndex>=0) for(int32 i=-2;i<=2;++i) {
        Face(TEXT("Cone"),FVector(-5,i*4.f,21+FMath::Abs(i)*-1.f),FVector(.04f,.04f,.09f),FLinearColor(.85f,.57f,.17f));
        Face(TEXT("Sphere"),FVector(0,i*3.f,19),FVector(.025f),FLinearColor(.95f,.4f,.65f));
    }
    if(Look==6 && LookIndex>=0) {
        Face(TEXT("Sphere"),FVector(2,0,15),FVector(.30f,.30f,.13f),FLinearColor(.06f,.07f,.08f));
        Face(TEXT("Cube"),FVector(-10,0,14),FVector(.09f,.20f,.025f),FLinearColor(.28f,.27f,.25f));
    }
    if(Look==7 && LookIndex>=0) {
        for(float Side:{-1.f,1.f}) Face(TEXT("Sphere"),FVector(4,Side*11,18),FVector(.13f),Hairs[Look]);
        Face(TEXT("Sphere"),FVector(-10.5f,0,-6),FVector(.01f,.07f,.011f),FLinearColor(.48f,.025f,.23f));
    }
    if(Look==8 && LookIndex>=0) {
        Face(TEXT("Cylinder"),FVector(0,0,17),FVector(.36f,.32f,.025f),FLinearColor(.04f,.025f,.07f));
        Face(TEXT("Cylinder"),FVector(0,0,27),FVector(.23f,.22f,.19f),FLinearColor(.08f,.045f,.14f));
    }
    // Tailored details use bone sockets and therefore follow animated and
    // physical bodies; they cannot drag trouser vertices back into the arms.
    for(float Side:{-1.f,1.f}) {
        auto* Lapel=CEVisual::Part(this,TEXT("Cube"),FVector::ZeroVector,FVector(.018f,.06f,.14f),Cloth*.65f);
        Lapel->AttachToComponent(Rig,FAttachmentTransformRules::KeepRelativeTransform,TEXT("chest"));
        Lapel->SetRelativeRotation(RestBones.FindRef(TEXT("chest")).GetRotation().Inverse().Rotator());
        Lapel->SetRelativeLocation(RestBones.FindRef(TEXT("chest")).GetRotation().Inverse().RotateVector(FVector(-12,Side*5.f,-4)));
    }
    Phone=CEVisual::Part(this,TEXT("Cube"),FVector::ZeroVector,FVector(.018f,.065f,.13f),FLinearColor(.055f,.04f,.06f));
    Phone->AttachToComponent(VisualRoot,FAttachmentTransformRules::KeepRelativeTransform); Phone->SetVisibility(false);
    UE_LOG(LogTemp,Display,TEXT("CE_CUSTOMER_RIG mesh=%s bones=%d head=%s"),*Path,Rig->GetNumBones(),*RestBones.FindRef(TEXT("head")).GetLocation().ToString());
    return true;
}

FVector ACECustomer::HeadLocation() const { return HeadPivot ? HeadPivot->GetComponentLocation() : GetActorLocation()+FVector(0,0,60); }
void ACECustomer::Soak(float Ml,const FString& Liquid) { WetMl=FMath::Min(350.f,WetMl+FMath::Max(0.f,Ml)); ReceivedLiquid=Liquid; }
void ACECustomer::VoiceStarted(float Duration,float Energy) { VoiceBegin=GetWorld()->GetTimeSeconds(); VoiceUntil=VoiceBegin+Duration; VoiceEnergy=Energy; SpeechLevels.Reset(); }
void ACECustomer::SetSpeechEnvelope(const TArray<float>& Levels,float Duration) { VoiceStarted(Duration); SpeechLevels=Levels; }
void ACECustomer::ImmediateVoice(const TCHAR* Cue)
{
    const float Now=GetWorld()->GetTimeSeconds(); if(Now-ImmediateVoiceAt<3.5f) return;
    const FString Name=FString::Printf(TEXT("NPC_%d_%s"),Personality,Cue);
    auto* Wave=LoadObject<USoundBase>(nullptr,*FString::Printf(TEXT("/Game/Prototype/Audio/Customers/%s.%s"),*Name,*Name));
    if(!Wave) return; ImmediateVoiceAt=Now;
    auto* Spatial=NewObject<USoundAttenuation>(this); Spatial->Attenuation.bAttenuate=true; Spatial->Attenuation.bSpatialize=true;
    Spatial->Attenuation.AttenuationShapeExtents=FVector(80); Spatial->Attenuation.FalloffDistance=480.f;
    auto* Limit=LoadObject<USoundConcurrency>(nullptr,TEXT("/Game/Prototype/Audio/SC_Customers.SC_Customers"));
    const float Pitch=Personality==0 ? .94f : Personality==1 ? 1.02f : 1.12f;
    UGameplayStatics::PlaySoundAtLocation(GetWorld(),Wave,HeadLocation(),FRotator::ZeroRotator,.6f,Pitch,0.f,Spatial,Limit,this);
    VoiceStarted(Wave->GetDuration()/Pitch);
}

void ACECustomer::AnimateRig(float Delta,bool bWalking)
{
    if(!Rig) return;
    const float Now=GetWorld()->GetTimeSeconds();
    const float Travel=FVector::Dist2D(GetActorLocation(),LastMotionLocation); LastMotionLocation=GetActorLocation();
    if(bWalking && Travel<12.f) WalkPhase+=Travel*.057f;
    PoseWalk=FMath::FInterpTo(PoseWalk,bWalking ? 1.f : 0.f,Delta,6.f);
    const float Gait=FMath::Sin(WalkPhase+SeatSlot*.7f)*PoseWalk;
    if(bPhysicalFall && Now>=FallenUntil) EndPhysicalFall();
    if(bHeadPhysics && !bPhysicalFall && Now>=RecoilUntil) {
        Rig->SetAllBodiesBelowSimulatePhysics(TEXT("head"),false,true);
        Rig->SetAllBodiesBelowPhysicsBlendWeight(TEXT("head"),0.f,false,true);
        Rig->SetCollisionEnabled(ECollisionEnabled::NoCollision); bHeadPhysics=false;
    }
    PoseSeat=bPhysicalFall ? FallSeatBlend : FMath::FInterpTo(PoseSeat,bSeated ? 1.f : 0.f,Delta,5.f);
    PoseFall=FMath::FInterpTo(PoseFall,Now<FallenUntil && !bPhysicalFall ? 1.f : 0.f,Delta,Now<FallenUntil ? 10.f : 3.f);
    PoseGesture=FMath::FInterpTo(PoseGesture,Now<GestureUntil ? 1.f : 0.f,Delta,6.f);
    bFilming=Now<FilmingUntil; bTakingSip=State==ECECustomerState::Eating && FMath::Fmod(Now,5.f)<1.7f && PoseFall<.1f;
    float Yaw=0;
    if(auto* Player=UGameplayStatics::GetPlayerPawn(this,0)) {
        const FVector Local=GetActorTransform().InverseTransformPosition(Player->GetActorLocation());
        if(Local.Size2D()<420.f || Now<ThinkingUntil) Yaw=FMath::Clamp(FMath::UnwindDegrees(Local.Rotation().Yaw-180.f),-48.f,48.f);
    }
    PoseHeadYaw=FMath::FInterpTo(PoseHeadYaw,Yaw,Delta,4.f);
    // Rebuild FK from the immutable reference, never accumulating pose errors.
    PosedBones.Reset();
    const float Breath=FMath::Sin(Now*2.f+SeatSlot)*.45f;
    for(int32 i=0;i<Rig->GetNumBones();++i) {
        const FName Name=Rig->GetBoneName(i), Parent=Rig->GetParentBone(Name);
        const FTransform Rest=RestBones.FindRef(Name);
        FTransform Pose=Parent.IsNone() ? Rest : Rest.GetRelativeTransform(RestBones.FindRef(Parent))*PosedBones.FindRef(Parent);
        const FString Bone=Name.ToString(); float Pitch=0,Roll=0,YawBone=0;
        const float Side=Bone.EndsWith(TEXT("_l")) ? -1.f : 1.f;
        const auto Stride=CEGuestMotion::Stride(WalkPhase+(Side<0 ? PI : 0.f));
        if(Bone==TEXT("root")) {
            Pitch=(bPhysicalFall || RecoverUntil>Now) ? 0.f : PoseFall*78.f; Roll=(bPhysicalFall || RecoverUntil>Now) ? 0.f : PoseFall*15.f;
            const FVector Pivot(0,0,88);
            const FVector FallOffset=Pivot-FRotator(Pitch,0,Roll).Quaternion().RotateVector(Pivot)+FVector(0,0,-PoseFall*65.f);
            Pose.AddToTranslation((bPhysicalFall || RecoverUntil>Now ? FVector::ZeroVector : FallOffset)+FVector(0,0,-PoseSeat*28.f*(1.f-PoseFall)+(bWalking ? FMath::Abs(Gait)*.7f : 0.f)));
        }
        else if(Bone==TEXT("spine")) Pitch=Breath+PoseSeat*5.f+Anger*2.f;
        else if(Bone==TEXT("head")) { YawBone=PoseHeadYaw; Pitch=Now<ThinkingUntil ? -5.f : Now<GestureUntil ? FMath::Sin(Now*4.f)*3.f : 0.f; }
        else if(Bone.StartsWith(TEXT("upperarm"))) {
            Pitch=FMath::Lerp(-4.f-PoseGesture*(23.f+FMath::Sin(Now*5.f+Side)*9.f),Stride.Arm,PoseWalk);
            if(!bPhysicalFall && RecoverUntil<Now) Pitch=FMath::Lerp(Pitch,-98.f+FMath::Sin(Now*12.f+Side)*13.f,PoseFall);
            Roll=Side*(8.f+Anger*8.f);
            if(bFilming && Side<0) { Pitch=-68.f; Roll=-8.f; }
            if(bTakingSip && Side>0) { Pitch=-48.f; Roll=8.f; }
            if(Now<ProtectUntil && Side>0) { Pitch=-65.f; Roll=12.f; }
        } else if(Bone.StartsWith(TEXT("forearm"))) {
            Pitch=-8.f-PoseGesture*38.f;
            if(bFilming && Side<0) Pitch=-38.f;
            if(bTakingSip && Side>0) Pitch=-90.f;
        } else if(Bone.StartsWith(TEXT("thigh"))) Pitch=-PoseSeat*83.f+Stride.Hip*PoseWalk+PoseFall*12.f;
        else if(Bone.StartsWith(TEXT("shin"))) Pitch=PoseSeat*83.f+Stride.Knee*PoseWalk;
        if(Bone.StartsWith(TEXT("foot"))) Pitch=Stride.Ankle*PoseWalk;
        if(Now<WipeUntil && (Bone==TEXT("upperarm_r") || Bone==TEXT("forearm_r"))) { Pitch=Bone==TEXT("upperarm_r") ? -38.f : -65.f; Roll=FMath::Sin(Now*8.f)*8.f; }
        if(Bone==TEXT("chest") && Now<RecoilUntil) Pitch-=12.f*(RecoilUntil-Now);
        const FQuat DeltaRotation=FRotator(Pitch,YawBone,Roll).Quaternion();
        Pose.SetRotation(DeltaRotation*Pose.GetRotation()); Pose.NormalizeRotation();
        PosedBones.Add(Name,Pose);
    }
    if(!bPhysicalFall && RecoverUntil<Now) {
        // Reach towards the desk or the mouth instead of moving the cup
        // independently of the arm. The elbow keeps a consistent bend plane.
        for(int32 SideIndex=0;SideIndex<2;++SideIndex) {
            const TCHAR* S=SideIndex==0 ? TEXT("l") : TEXT("r"); const float Side=SideIndex==0 ? -1.f : 1.f;
            if((!bSeated && !(bTakingSip && SideIndex==1)) || bFilming || Now<WipeUntil || Now<ProtectUntil) continue;
            const FName Shoulder(*FString::Printf(TEXT("upperarm_%s"),S)),Elbow(*FString::Printf(TEXT("forearm_%s"),S)),Hand(*FString::Printf(TEXT("hand_%s"),S));
            const FVector A=PosedBones.FindRef(Shoulder).GetLocation(),B=PosedBones.FindRef(Elbow).GetLocation(),OldEnd=PosedBones.FindRef(Hand).GetLocation();
            FVector Goal(-33.f,Side*25.f,84.f);
            if(bTakingSip && SideIndex==1) Goal=PosedBones.FindRef(TEXT("head")).GetLocation()+FVector(-10,7,-18);
            const float L1=FVector::Dist(A,B),L2=FVector::Dist(B,OldEnd);
            const FVector Axis=(Goal-A).GetSafeNormal(); const float D=FMath::Clamp(FVector::Dist(A,Goal),1.f,L1+L2-.1f);
            Goal=A+Axis*D; const float Along=(L1*L1-L2*L2+D*D)/(2.f*D);
            FVector Bend=FVector::VectorPlaneProject(FVector(0,Side*.3f,-1),Axis).GetSafeNormal();
            const FVector NewB=A+Axis*Along+Bend*FMath::Sqrt(FMath::Max(0.f,L1*L1-Along*Along));
            auto& Upper=PosedBones.FindChecked(Shoulder); Upper.SetRotation(FQuat::FindBetweenVectors(B-A,NewB-A)*Upper.GetRotation());
            auto& Lower=PosedBones.FindChecked(Elbow); Lower.SetLocation(NewB); Lower.SetRotation(FQuat::FindBetweenVectors(OldEnd-B,Goal-NewB)*Lower.GetRotation());
            PosedBones.FindChecked(Hand).SetLocation(Goal);
        }
        // Two-bone leg correction against the floor. Knee plane remains in
        // front of the hip, and contact offsets are eased to avoid foot pops.
        for(int32 SideIndex=0;SideIndex<2;++SideIndex) {
            const TCHAR* S=SideIndex==0 ? TEXT("l") : TEXT("r");
            const FName Hip(*FString::Printf(TEXT("thigh_%s"),S)),Knee(*FString::Printf(TEXT("shin_%s"),S)),Foot(*FString::Printf(TEXT("foot_%s"),S));
            FVector End=PosedBones.FindRef(Foot).GetLocation(); const FVector World=Rig->GetComponentTransform().TransformPosition(End);
            FHitResult Hit; FCollisionQueryParams Query; Query.AddIgnoredActor(this);
            const bool Ground=GetWorld()->LineTraceSingleByChannel(Hit,World+FVector(0,0,18),World-FVector(0,0,30),ECC_WorldStatic,Query);
            const float Correction=Ground ? FMath::Clamp(Hit.ImpactPoint.Z+9.f-World.Z,-8.f,14.f) : 0.f;
            FootGroundOffsets[SideIndex]=FMath::FInterpTo(FootGroundOffsets[SideIndex],Correction,Delta,12.f);
            const FVector A=PosedBones.FindRef(Hip).GetLocation(),B=PosedBones.FindRef(Knee).GetLocation();
            End.Z+=FootGroundOffsets[SideIndex];
            const float L1=FVector::Dist(A,B),L2=FVector::Dist(B,PosedBones.FindRef(Foot).GetLocation());
            const FVector Axis=(End-A).GetSafeNormal(); const float D=FMath::Clamp(FVector::Dist(A,End),1.f,L1+L2-.01f);
            const float Along=(L1*L1-L2*L2+D*D)/(2.f*D);
            FVector Bend=FVector::VectorPlaneProject(FVector(-1,0,0),Axis).GetSafeNormal();
            if(Bend.IsNearlyZero()) Bend=FVector(0,1,0);
            const FVector NewB=A+Axis*Along+Bend*FMath::Sqrt(FMath::Max(0.f,L1*L1-Along*Along));
            auto& H=PosedBones.FindChecked(Hip); H.SetRotation(FQuat::FindBetweenVectors(B-A,NewB-A)*H.GetRotation());
            auto& K=PosedBones.FindChecked(Knee); K.SetLocation(NewB); K.SetRotation(FQuat::FindBetweenVectors(PosedBones.FindRef(Foot).GetLocation()-B,End-NewB)*K.GetRotation());
            auto& F=PosedBones.FindChecked(Foot); F.SetLocation(End);
        }
    }
    if(auto* Animation=Cast<UCEBodyAnimation>(Rig->GetAnimInstance())) {
        Animation->LocalPoseFrame.Reset();
        for(int32 i=0;i<Rig->GetNumBones();++i) {
            const FName Name=Rig->GetBoneName(i),Parent=Rig->GetParentBone(Name);
            FTransform Local=Parent.IsNone() ? PosedBones.FindRef(Name) : PosedBones.FindRef(Name).GetRelativeTransform(PosedBones.FindRef(Parent));
            if(RecoverUntil>Now && RecoveryPose.IsValidIndex(i)) Local.Blend(RecoveryPose[i],Local,1.f-(RecoverUntil-Now)/1.1f);
            Animation->LocalPoseFrame.Add(Local);
        }
    }
    // Face uses the head's displacement from its reference orientation.
    const FTransform Head=Rig->GetSocketTransform(TEXT("head")), RestHead=RestBones.FindRef(TEXT("head"));
    HeadPivot->SetWorldLocation(Head.GetLocation());
    HeadPivot->SetWorldRotation((Head.GetRotation()*RestHead.GetRotation().Inverse()).Rotator());
    if(ServedCup.IsValid() && ServedCup->bDelivered && !ServedCup->bHeld && !ServedCup->bBroken) {
        auto* Cup=ServedCup.Get();
        FVector CupTarget=ServedCupPosition;
        if(bTakingSip) CupTarget=Rig->GetSocketLocation(TEXT("hand_r"))+GetActorRotation().RotateVector(FVector(-15,0,11));
        Cup->SetActorLocation(FMath::VInterpTo(Cup->GetActorLocation(),CupTarget,Delta,5.f));
        Cup->SetActorRotation(FRotator(bTakingSip ? -12.f : 0.f,GetActorRotation().Yaw,0));
        if(bTakingSip && Cup->VolumeMl()>0) { const float Ratio=FMath::Max(0.f,1.f-Delta*4.f/FMath::Max(1.f,Cup->VolumeMl())); Cup->EspressoMl*=Ratio; Cup->MilkMl*=Ratio; Cup->WaterMl*=Ratio; Cup->UpdateLiquidVisual(); }
    }
    const bool Blink=FMath::Fmod(Now+SeatSlot*.71f,4.3f)<.13f;
    const bool Speaking=Now<VoiceUntil;
    const int32 SpeechIndex=FMath::FloorToInt((Now-VoiceBegin)/FMath::Max(.01f,VoiceUntil-VoiceBegin)*SpeechLevels.Num());
    const float Talk=Speaking ? (SpeechLevels.IsValidIndex(SpeechIndex) ? SpeechLevels[SpeechIndex] : FMath::Abs(FMath::Sin(Now*13.f))*VoiceEnergy) : Now<ReactionUntil ? FMath::Abs(FMath::Sin(Now*10.f))*.45f : 0.f;
    for(int32 i=0;i<2;++i) {
        Eyes[i]->SetRelativeScale3D(FVector(.021f,.043f,Blink ? .003f : .025f)); Pupils[i]->SetVisibility(!Blink,true);
        Brows[i]->SetRelativeRotation(FRotator(0,0,(i==0 ? -1.f : 1.f)*(DialogueEmotion==TEXT("triste") ? -16.f : Anger*18.f)));
    }
    Mouth->SetRelativeScale3D(FVector(.01f,.056f,.008f+Talk*.03f));
    if(ClothingMaterial) ClothingMaterial->SetScalarParameterValue(TEXT("Wetness"),FMath::Clamp(WetMl/90.f,0.f,1.f));
    if(Phone) {
        Phone->SetVisibility(bFilming);
        Phone->SetWorldLocation(Rig->GetSocketLocation(TEXT("hand_l")));
        Phone->SetWorldRotation(GetActorRotation()+FRotator(0,PoseHeadYaw,8));
    }
}

bool ACECustomer::StartPhysicalFall(const FVector& Direction)
{
    if(!Rig || !Rig->GetPhysicsAsset()) return false;
    int32 Active=0; for(TActorIterator<ACECustomer> It(GetWorld());It;++It) if(It->bPhysicalFall) ++Active;
    if(Active>=2) return false;
    if(PhysicalDriver) PhysicalDriver->SetStrengthMultiplyer(0.f);
    // During ragdoll, interaction and liquid traces must hit the actual body,
    // never an invisible upright capsule where the customer used to stand.
    FallSeatBlend=PoseSeat; FallOrigin=GetActorLocation();
    Collision->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Rig->SetCollisionProfileName(TEXT("PhysicsActor")); Rig->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
    Rig->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Block);
    Rig->SetCollisionResponseToChannel(ECC_Visibility,ECR_Block);
    Rig->SetSimulatePhysics(true);
    Rig->SetAllBodiesSimulatePhysics(true); Rig->SetAllBodiesPhysicsBlendWeight(1.f);
    Rig->SetAllBodiesNotifyRigidBodyCollision(true);
    Rig->WakeAllRigidBodies(); Rig->SetAllPhysicsLinearVelocity(Direction.GetSafeNormal2D()*105.f+FVector(0,0,12),false);
    bPhysicalFall=true; ++PhysicalFalls;
    UE_LOG(LogTemp,Display,TEXT("CE_RAGDOLL_BEGIN id=%s active=%d"),*CustomerId,Active+1); return true;
}

void ACECustomer::EndPhysicalFall()
{
    if(!bPhysicalFall || !Rig) return;
    RecoveryPose.Reset();
    for(int32 i=0;i<Rig->GetNumBones();++i) {
        const FName Name=Rig->GetBoneName(i),Parent=Rig->GetParentBone(Name);
        FTransform Pose=Rig->GetSocketTransform(Name,RTS_Component);
        if(!Parent.IsNone()) Pose=Pose.GetRelativeTransform(Rig->GetSocketTransform(Parent,RTS_Component));
        RecoveryPose.Add(Pose);
    }
    Rig->SetAllBodiesSimulatePhysics(false); Rig->SetAllBodiesPhysicsBlendWeight(0.f);
    Rig->SetSimulatePhysics(false);
    Rig->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Rig->AttachToComponent(VisualRoot,FAttachmentTransformRules::SnapToTargetNotIncludingScale);
    Rig->SetRelativeLocationAndRotation(FVector(0,0,-88),FRotator::ZeroRotator);
    Collision->SetCollisionEnabled(ECollisionEnabled::QueryAndPhysics);
    bPhysicalFall=false; RecoverUntil=GetWorld()->GetTimeSeconds()+1.1f;
    UE_LOG(LogTemp,Display,TEXT("CE_RAGDOLL_END id=%s"),*CustomerId);
}

void ACECustomer::PhysicalRecoil(const FVector& Direction)
{
    RecoilUntil=GetWorld()->GetTimeSeconds()+.75f;
    if(bPhysicalFall || !Rig || !PhysicalDriver || !Rig->GetPhysicsAsset()) return;
    FPhysicalAnimationData Drive; Drive.bIsLocalSimulation=true;
    Drive.OrientationStrength=1600.f; Drive.AngularVelocityStrength=160.f;
    Rig->SetCollisionProfileName(TEXT("PhysicsActor")); Rig->SetCollisionResponseToChannel(ECC_Pawn,ECR_Ignore);
    Rig->SetCollisionResponseToChannel(ECC_WorldDynamic,ECR_Ignore);
    PhysicalDriver->SetStrengthMultiplyer(1.f);
    PhysicalDriver->ApplyPhysicalAnimationSettingsBelow(TEXT("head"),Drive,true);
    Rig->SetAllBodiesBelowSimulatePhysics(TEXT("head"),true,true);
    Rig->SetAllBodiesBelowPhysicsBlendWeight(TEXT("head"),1.f,false,true);
    Rig->AddImpulse(Direction.GetSafeNormal()*35.f,TEXT("head"),true); bHeadPhysics=true;
}

bool ACECustomer::IsBodyConsistent(float MaxJointError) const {
    if(!Rig || !Rig->GetPhysicsAsset()) return false;
    for(const auto& Joint:Rig->GetPhysicsAsset()->ConstraintSetup) {
        const auto& C=Joint->DefaultInstance;
        const FVector A=Rig->GetSocketTransform(C.ConstraintBone1).TransformPosition(C.GetRefFrame(EConstraintFrame::Frame1).GetLocation());
        const FVector B=Rig->GetSocketTransform(C.ConstraintBone2).TransformPosition(C.GetRefFrame(EConstraintFrame::Frame2).GetLocation());
        if(A.ContainsNaN() || B.ContainsNaN() || FVector::Distance(A,B)>MaxJointError) return false;
    }
    return true;
}

void ACECustomer::RagdollContact(UPrimitiveComponent* HitComponent,AActor* OtherActor,UPrimitiveComponent* OtherComponent,FVector Impulse,const FHitResult& Hit) {
    const float Now=GetWorld()->GetTimeSeconds(); auto* Other=Cast<ACECustomer>(OtherActor);
    if(bPhysicalFall && Other) UE_LOG(LogTemp,Display,TEXT("CE_RAGDOLL_CONTACT source=%s target=%s impulse=%.1f"),*CustomerId,*Other->CustomerId,Impulse.Size());
    if(!bPhysicalFall || !Other || Other==this || Other->bPhysicalFall || Other->bLeaving || Now-LastRagdollContactAt<6.f || Impulse.Size()<700.f) return;
    LastRagdollContactAt=Now; auto* Cafe=GetWorld()->GetSubsystem<UCECafeSubsystem>();
    ++Cafe->ChainReactions; Cafe->Observe(TEXT("chain_reaction"),Other->CustomerId,TEXT("Contacto con cliente que cae"),.45f);
    Other->Fall((Other->GetActorLocation()-GetActorLocation()).GetSafeNormal2D());
}
