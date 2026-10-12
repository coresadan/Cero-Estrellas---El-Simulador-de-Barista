#include "CEGameplay.h"
#include "CEVisual.h"
#include "Components/PoseableMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Components/SceneComponent.h"
#include "Components/CapsuleComponent.h"
#include "Components/TextRenderComponent.h"
#include "Engine/SkeletalMesh.h"
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
    Rig=NewObject<UPoseableMeshComponent>(this); AddInstanceComponent(Rig);
    Rig->SetupAttachment(VisualRoot); Rig->SetRelativeLocation(FVector(0,0,-88));
    Rig->SetSkinnedAssetAndUpdate(Mesh); Rig->SetCollisionEnabled(ECollisionEnabled::NoCollision);
    Rig->RegisterComponent(); Rig->RefreshBoneTransforms();
    const FLinearColor Skin(.64f+Personality*.04f,.40f+Personality*.03f,.27f+Personality*.02f);
    for(int32 i=0;i<Mesh->GetMaterials().Num();++i) {
        const FString Slot=Mesh->GetMaterials()[i].MaterialSlotName.ToString();
        const bool bSkin=Slot==TEXT("Skin"), bCloth=Slot==TEXT("Clothing"), bPants=Slot==TEXT("Trousers");
        auto* Mat=CEVisual::Surface(this,bSkin ? Skin : bCloth ? Outfit : bPants ? FLinearColor(.055f,.065f,.077f) : FLinearColor(.033f,.022f,.018f),bSkin ? TEXT("M_Skin") : bCloth || bPants ? TEXT("M_Fabric") : TEXT("M_Surface"));
        Rig->SetMaterial(i,Mat); if(bCloth) ClothingMaterial=Mat;
    }
    for(int32 i=0;i<Rig->GetNumBones();++i) {
        const FName Name=Rig->GetBoneName(i); RestBones.Add(Name,Rig->GetBoneTransformByName(Name,EBoneSpaces::ComponentSpace));
    }
    HeadPivot=NewObject<USceneComponent>(this); AddInstanceComponent(HeadPivot);
    HeadPivot->SetupAttachment(VisualRoot); HeadPivot->RegisterComponent();
    auto Face=[this](const TCHAR* Shape,FVector Position,FVector Scale,FLinearColor Color) {
        auto* Part=CEVisual::Part(this,Shape,FVector::ZeroVector,Scale,Color);
        Part->AttachToComponent(HeadPivot,FAttachmentTransformRules::KeepRelativeTransform); Part->SetRelativeLocation(Position); return Part;
    };
    for(int32 i=0;i<2;++i) {
        const float Side=i==0 ? -1.f : 1.f;
        Eyes[i]=Face(TEXT("Sphere"),FVector(-11.3f,Side*4.8f,3),FVector(.023f,.050f,.029f),FLinearColor(.83f,.81f,.75f));
        Pupils[i]=Face(TEXT("Sphere"),FVector(-12.5f,Side*4.8f,3),FVector(.012f,.021f,.023f),FLinearColor(.075f,.045f,.025f));
        auto* Glint=Face(TEXT("Sphere"),FVector(-13.1f,Side*4.8f-.3f,3.5f),FVector(.005f,.008f,.008f),FLinearColor(.98f,.97f,.93f));
        Glint->AttachToComponent(Pupils[i],FAttachmentTransformRules::KeepWorldTransform);
        Brows[i]=Face(TEXT("Cube"),FVector(-11.8f,Side*4.8f,6.8f),FVector(.012f,.055f,.012f),FLinearColor(.04f,.026f,.02f));
        auto* Ear=Face(TEXT("Sphere"),FVector(0,Side*11.8f,0),FVector(.041f,.026f,.055f),Skin);
        Ear->SetMaterial(0,CEVisual::Surface(this,Skin,TEXT("M_Skin")));
    }
    Mouth=Face(TEXT("Sphere"),FVector(-11.9f,0,-7),FVector(.01f,.056f,.008f),FLinearColor(.19f,.055f,.045f));
    if(Personality==0) Face(TEXT("SM_Glasses"),FVector(1,0,-.5f),FVector(.74f),FLinearColor(.04f,.065f,.06f));
    if(Personality==1) {
        Face(TEXT("Cylinder"),FVector(0,0,16),FVector(.32f,.29f,.025f),FLinearColor(.18f,.095f,.035f));
        Face(TEXT("Cylinder"),FVector(0,0,20),FVector(.23f,.22f,.08f),FLinearColor(.24f,.14f,.06f));
        Face(TEXT("Sphere"),FVector(-10.4f,0,-9),FVector(.022f,.15f,.045f),FLinearColor(.09f,.04f,.016f));
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
    const float Now=GetWorld()->GetTimeSeconds(), Phase=Now*8.5f+SeatSlot*.7f, Gait=FMath::Sin(Phase);
    PoseSeat=FMath::FInterpTo(PoseSeat,bSeated ? 1.f : 0.f,Delta,5.f);
    PoseFall=FMath::FInterpTo(PoseFall,Now<FallenUntil ? 1.f : 0.f,Delta,Now<FallenUntil ? 10.f : 3.f);
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
        if(Bone==TEXT("root")) {
            Pitch=PoseFall*78.f; Roll=PoseFall*15.f;
            const FVector Pivot(0,0,88);
            const FVector FallOffset=Pivot-FRotator(Pitch,0,Roll).Quaternion().RotateVector(Pivot)+FVector(0,0,-PoseFall*65.f);
            Pose.AddToTranslation(FallOffset+FVector(0,0,-PoseSeat*28.f*(1.f-PoseFall)+(bWalking ? FMath::Abs(Gait)*1.2f : 0.f)));
        }
        else if(Bone==TEXT("spine")) Pitch=Breath+PoseSeat*5.f+Anger*2.f;
        else if(Bone==TEXT("head")) { YawBone=PoseHeadYaw; Pitch=Now<ThinkingUntil ? -5.f : Now<GestureUntil ? FMath::Sin(Now*4.f)*3.f : 0.f; }
        else if(Bone.StartsWith(TEXT("upperarm"))) {
            Pitch=bWalking ? -Gait*Side*23.f : -7.f-PoseGesture*(23.f+FMath::Sin(Now*5.f+Side)*9.f);
            Pitch=FMath::Lerp(Pitch,-98.f+FMath::Sin(Now*12.f+Side)*13.f,PoseFall);
            Roll=Side*(5.f+Anger*11.f+PoseFall*20.f);
            if(bFilming && Side<0) { Pitch=-68.f; Roll=-8.f; }
            if(bTakingSip && Side>0) { Pitch=-48.f; Roll=8.f; }
            if(Now<ProtectUntil && Side>0) { Pitch=-65.f; Roll=12.f; }
        } else if(Bone.StartsWith(TEXT("forearm"))) {
            Pitch=-8.f-PoseGesture*38.f;
            if(bFilming && Side<0) Pitch=-38.f;
            if(bTakingSip && Side>0) Pitch=-90.f;
        } else if(Bone.StartsWith(TEXT("thigh"))) Pitch=-PoseSeat*83.f+(bWalking ? Gait*Side*25.f : 0.f)+PoseFall*12.f;
        else if(Bone.StartsWith(TEXT("shin"))) Pitch=PoseSeat*83.f+(bWalking ? FMath::Max(0.f,-Gait*Side)*28.f : 0.f);
        const FQuat DeltaRotation=FRotator(Pitch,YawBone,Roll).Quaternion();
        Pose.SetRotation(DeltaRotation*Pose.GetRotation()); Pose.NormalizeRotation();
        PosedBones.Add(Name,Pose); Rig->SetBoneTransformByName(Name,Pose,EBoneSpaces::ComponentSpace);
    }
    Rig->RefreshBoneTransforms();
    // Face uses the head's displacement from its reference orientation.
    const FTransform Head=PosedBones.FindRef(TEXT("head")), RestHead=RestBones.FindRef(TEXT("head"));
    HeadPivot->SetRelativeLocation(Head.GetLocation()+FVector(0,0,-88));
    HeadPivot->SetRelativeRotation((Head.GetRotation()*RestHead.GetRotation().Inverse()).Rotator());
    if(ServedCup.IsValid() && ServedCup->bDelivered && !ServedCup->bHeld && !ServedCup->bBroken) {
        auto* Cup=ServedCup.Get();
        FVector CupTarget=ServedCupPosition;
        if(bTakingSip) CupTarget=Rig->GetComponentTransform().TransformPosition(PosedBones.FindRef(TEXT("hand_r")).GetLocation())+FVector(0,0,11);
        Cup->SetActorLocation(FMath::VInterpTo(Cup->GetActorLocation(),CupTarget,Delta,5.f));
        Cup->SetActorRotation(FRotator(bTakingSip ? -12.f : 0.f,GetActorRotation().Yaw,0));
        if(bTakingSip && Cup->VolumeMl()>0) { const float Ratio=FMath::Max(0.f,1.f-Delta*4.f/FMath::Max(1.f,Cup->VolumeMl())); Cup->EspressoMl*=Ratio; Cup->MilkMl*=Ratio; Cup->WaterMl*=Ratio; Cup->UpdateLiquidVisual(); }
    }
    const bool Blink=FMath::Fmod(Now+SeatSlot*.71f,4.3f)<.13f;
    const bool Speaking=Now<VoiceUntil;
    const int32 SpeechIndex=FMath::FloorToInt((Now-VoiceBegin)/FMath::Max(.01f,VoiceUntil-VoiceBegin)*SpeechLevels.Num());
    const float Talk=Speaking ? (SpeechLevels.IsValidIndex(SpeechIndex) ? SpeechLevels[SpeechIndex] : FMath::Abs(FMath::Sin(Now*13.f))*VoiceEnergy) : Now<ReactionUntil ? FMath::Abs(FMath::Sin(Now*10.f))*.45f : 0.f;
    for(int32 i=0;i<2;++i) {
        Eyes[i]->SetRelativeScale3D(FVector(.023f,.05f,Blink ? .003f : .029f)); Pupils[i]->SetVisibility(!Blink,true);
        Brows[i]->SetRelativeRotation(FRotator(0,0,(i==0 ? -1.f : 1.f)*(DialogueEmotion==TEXT("triste") ? -16.f : Anger*18.f)));
    }
    Mouth->SetRelativeScale3D(FVector(.01f,.056f,.008f+Talk*.03f));
    if(ClothingMaterial) ClothingMaterial->SetScalarParameterValue(TEXT("Wetness"),FMath::Clamp(WetMl/90.f,0.f,1.f));
    if(Phone) {
        Phone->SetVisibility(bFilming);
        const FTransform Hand=PosedBones.FindRef(TEXT("hand_l"));
        Phone->SetRelativeLocation(Hand.GetLocation()+FVector(-2,0,-88)); Phone->SetRelativeRotation(FRotator(0,PoseHeadYaw,8));
    }
}
