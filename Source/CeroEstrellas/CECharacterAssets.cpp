#include "CECharacterAssets.h"
#include "Engine/SkeletalMesh.h"
#include "Animation/Skeleton.h"
#include "UObject/Package.h"

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
