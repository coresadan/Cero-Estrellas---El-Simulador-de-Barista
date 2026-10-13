#include "CECharacterAssets.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "UObject/Package.h"
#include "PhysicsEngine/PhysicsAsset.h"
#include "PhysicsEngine/SkeletalBodySetup.h"
#include "PhysicsEngine/PhysicsConstraintTemplate.h"
#include "PhysicsEngine/AggregateGeom.h"

bool UCECharacterAssets::PrepareSkeleton(USkeletalMesh* Mesh)
{
#if WITH_EDITOR
    if(!Mesh) return false;
    USkeleton* Skeleton=Mesh->GetSkeleton();
    if(!Skeleton) {
        Skeleton=NewObject<USkeleton>(Mesh->GetOutermost(),FName(*(Mesh->GetName()+TEXT("_Bones"))),RF_Public|RF_Standalone);
        if(!Skeleton->MergeAllBonesToBoneTree(Mesh)) return false;
        Mesh->SetSkeleton(Skeleton); Mesh->MarkPackageDirty();
    }
    return Skeleton && Skeleton->GetReferenceSkeleton().GetNum()>=18;
#else
    return false;
#endif
}

bool UCECharacterAssets::PreparePhysics(USkeletalMesh* Mesh)
{
#if WITH_EDITOR
    if(!Mesh || !PrepareSkeleton(Mesh)) return false;
    auto* Asset=Mesh->GetPhysicsAsset();
    if(!Asset) Asset=NewObject<UPhysicsAsset>(Mesh->GetOutermost(),FName(*(Mesh->GetName()+TEXT("_Physics"))),RF_Public|RF_Standalone);
    Asset->SkeletalBodySetups.Reset(); Asset->ConstraintSetup.Reset(); Asset->CollisionDisableTable.Reset();
    const auto& Ref=Mesh->GetRefSkeleton(); TArray<FTransform> Component;
    for(int32 i=0;i<Ref.GetNum();++i) {
        const int32 Parent=Ref.GetParentIndex(i);
        Component.Add(Parent<0 ? Ref.GetRefBonePose()[i] : Ref.GetRefBonePose()[i]*Component[Parent]);
    }
    TMap<FName,int32> BodyIndices;
    for(int32 i=0;i<Ref.GetNum();++i) {
        const FName Name=Ref.GetBoneName(i); const FString N=Name.ToString();
        if(N==TEXT("root") || N==TEXT("neck") || N.StartsWith(TEXT("CE_Rig"))) continue;
        auto* Body=NewObject<USkeletalBodySetup>(Asset,NAME_None,RF_Transactional);
        Body->BoneName=Name; Body->PhysicsType=PhysType_Default;
        Body->DefaultInstance.LinearDamping=.35f; Body->DefaultInstance.AngularDamping=3.f; Body->DefaultInstance.bUseCCD=true;
        const float Mass=N==TEXT("pelvis") ? 12.f : N==TEXT("chest") ? 10.f : N==TEXT("spine") ? 8.f : N==TEXT("head") ? 4.f : N.StartsWith(TEXT("thigh")) ? 6.f : N.StartsWith(TEXT("shin")) ? 3.f : N.StartsWith(TEXT("upperarm")) ? 2.f : N.StartsWith(TEXT("forearm")) ? 1.3f : .8f;
        Body->DefaultInstance.SetMassOverride(Mass,true);
        Body->DefaultInstance.SetOverrideIterationCounts(true);
        Body->DefaultInstance.PositionSolverIterationCount=12; Body->DefaultInstance.VelocitySolverIterationCount=4;
        Body->DefaultInstance.SetMaxDepenetrationVelocity(150.f);
        FVector Start=Component[i].GetLocation(), End=Start+FVector(0,0,12);
        for(int32 j=i+1;j<Ref.GetNum();++j) if(Ref.GetParentIndex(j)==i) { End=Component[j].GetLocation(); break; }
        if(N==TEXT("chest")) End=Start+FVector(0,0,10);
        if(N==TEXT("pelvis")) End=Start+FVector(0,0,4);
        if(N==TEXT("head")) { Start+=FVector(0,0,-4); End=Start+FVector(0,0,8); }
        if(N.StartsWith(TEXT("hand"))) End=Start+FVector(0,0,-7);
        if(N.StartsWith(TEXT("foot"))) End=Start+FVector(-12,0,-4);
        const float Radius=N==TEXT("head") ? 13.f : N==TEXT("pelvis") ? 10.f : N==TEXT("spine") || N==TEXT("chest") ? 11.5f : N.StartsWith(TEXT("thigh")) ? 5.5f : N.StartsWith(TEXT("upperarm")) ? 4.2f : 3.2f;
        FKSphylElem Shape; Shape.Radius=Radius; Shape.Length=FMath::Max(0.f,FVector::Distance(Start,End)-2.f*Radius);
        Shape.Center=Component[i].InverseTransformPosition((Start+End)*.5f);
        Shape.Rotation=(Component[i].GetRotation().Inverse()*FQuat::FindBetweenNormals(FVector::UpVector,(End-Start).GetSafeNormal())).Rotator();
        Body->AggGeom.SphylElems.Add(Shape); Body->CollisionTraceFlag=CTF_UseSimpleAsComplex;
        BodyIndices.Add(Name,Asset->SkeletalBodySetups.Num()); Asset->SkeletalBodySetups.Add(Body);
        Body->CreatePhysicsMeshes();
    }
    for(int32 i=0;i<Ref.GetNum();++i) if(BodyIndices.Contains(Ref.GetBoneName(i))) {
        int32 Parent=Ref.GetParentIndex(i);
        while(Parent>=0 && !BodyIndices.Contains(Ref.GetBoneName(Parent))) Parent=Ref.GetParentIndex(Parent);
        if(Parent<0) continue;
        auto* Joint=NewObject<UPhysicsConstraintTemplate>(Asset,NAME_None,RF_Transactional);
        auto& C=Joint->DefaultInstance; C.JointName=Ref.GetBoneName(i); C.ConstraintBone1=Ref.GetBoneName(i); C.ConstraintBone2=Ref.GetBoneName(Parent);
        const FString Bone=Ref.GetBoneName(i).ToString();
        const bool Hinge=Bone.StartsWith(TEXT("shin")) || Bone.StartsWith(TEXT("forearm"));
        // Twist X is aligned to the body's Y flexion axis, with one-sided
        // knee/elbow limits. A knee is not a shoulder ball joint.
        FTransform Anchor(Hinge ? FRotator(0,90,0).Quaternion() : FQuat::Identity,Component[i].GetLocation());
        C.SetRefFrame(EConstraintFrame::Frame1,Anchor.GetRelativeTransform(Component[i]));
        C.SetRefFrame(EConstraintFrame::Frame2,Anchor.GetRelativeTransform(Component[Parent]));
        C.SetLinearXLimit(LCM_Locked,0); C.SetLinearYLimit(LCM_Locked,0); C.SetLinearZLimit(LCM_Locked,0);
        if(Hinge) {
            const bool Knee=Bone.StartsWith(TEXT("shin"));
            C.SetAngularSwing1Limit(ACM_Limited,5); C.SetAngularSwing2Limit(ACM_Limited,5);
            C.SetAngularTwistLimit(ACM_Limited,Knee ? 62.f : 65.f);
            C.AngularRotationOffset=FRotator(0,0,Knee ? 58.f : -60.f);
        } else if(Bone==TEXT("spine") || Bone==TEXT("chest")) {
            C.SetAngularSwing1Limit(ACM_Limited,15); C.SetAngularSwing2Limit(ACM_Limited,20); C.SetAngularTwistLimit(ACM_Limited,12);
        } else if(Bone==TEXT("head")) {
            C.SetAngularSwing1Limit(ACM_Limited,35); C.SetAngularSwing2Limit(ACM_Limited,28); C.SetAngularTwistLimit(ACM_Limited,25);
        } else if(Bone.StartsWith(TEXT("hand")) || Bone.StartsWith(TEXT("foot"))) {
            C.SetAngularSwing1Limit(ACM_Limited,18); C.SetAngularSwing2Limit(ACM_Limited,25); C.SetAngularTwistLimit(ACM_Limited,12);
        } else {
            C.SetAngularSwing1Limit(ACM_Limited,65); C.SetAngularSwing2Limit(ACM_Limited,95); C.SetAngularTwistLimit(ACM_Limited,25);
        }
        C.EnableMassConditioning(); C.SetProjectionParams(true,.12f,.08f,5.f,15.f);
        C.SetDisableCollision(true); Joint->SetDefaultProfile(C); Asset->ConstraintSetup.Add(Joint);
    }
    // Adjacent bodies overlap at joints; the rest collide to keep limbs
    // outside the torso and prevent the paper-folded silhouette.
    for(int32 a=0;a<Asset->SkeletalBodySetups.Num();++a) for(int32 b=a+1;b<Asset->SkeletalBodySetups.Num();++b) {
        const FName A=Asset->SkeletalBodySetups[a]->BoneName,B=Asset->SkeletalBodySetups[b]->BoneName;
        bool Adjacent=false;
        for(const auto& Joint:Asset->ConstraintSetup) if((Joint->DefaultInstance.ConstraintBone1==A && Joint->DefaultInstance.ConstraintBone2==B) || (Joint->DefaultInstance.ConstraintBone1==B && Joint->DefaultInstance.ConstraintBone2==A)) Adjacent=true;
        if(Adjacent) Asset->DisableCollision(a,b);
    }
    Asset->UpdateBodySetupIndexMap(); Asset->UpdateBoundsBodiesArray(); Asset->MarkPackageDirty();
    Mesh->SetPhysicsAsset(Asset); Mesh->MarkPackageDirty();
    return Asset->SkeletalBodySetups.Num()>=12 && Asset->ConstraintSetup.Num()>=11;
#else
    return false;
#endif
}
