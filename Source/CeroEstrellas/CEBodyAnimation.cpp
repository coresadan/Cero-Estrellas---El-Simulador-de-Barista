#include "CEBodyAnimation.h"
#include "Animation/AnimInstanceProxy.h"
#include "Animation/AnimNodeBase.h"

struct FCEBodyProxy final : FAnimInstanceProxy
{
    explicit FCEBodyProxy(UAnimInstance* Instance) : FAnimInstanceProxy(Instance) {}
    TArray<FTransform> Frame;
    virtual void PreUpdate(UAnimInstance* Instance,float Delta) override
    {
        FAnimInstanceProxy::PreUpdate(Instance,Delta);
        Frame=CastChecked<UCEBodyAnimation>(Instance)->LocalPoseFrame;
    }
    virtual bool Evaluate(FPoseContext& Output) override
    {
        Output.ResetToRefPose();
        for(FCompactPoseBoneIndex Bone : Output.Pose.ForEachBoneIndex()) {
            const int32 MeshIndex=Output.Pose.GetBoneContainer().MakeMeshPoseIndex(Bone).GetInt();
            if(Frame.IsValidIndex(MeshIndex)) Output.Pose[Bone]=Frame[MeshIndex];
        }
        return true;
    }
};
FAnimInstanceProxy* UCEBodyAnimation::CreateAnimInstanceProxy() { return new FCEBodyProxy(this); }
void UCEBodyAnimation::DestroyAnimInstanceProxy(FAnimInstanceProxy* Proxy) { delete Proxy; }
