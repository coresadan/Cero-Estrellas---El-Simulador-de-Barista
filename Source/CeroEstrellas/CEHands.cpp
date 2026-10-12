#include "CEGameplay.h"
#include "CEDialogue.h"
#include "CEVisual.h"
#include "Camera/CameraComponent.h"
#include "Components/StaticMeshComponent.h"
#include "GameFramework/CharacterMovementComponent.h"

int32 ACEPlayer::VisibleHands() const { int32 N=0; for(auto Hand:Hands) if(Hand && Hand->IsVisible()) ++N; return N; }
void ACEPlayer::SetHighlight(AActor* Actor)
{
    if(Highlighted.Get()==Actor) return;
    if(Highlighted.IsValid()) { TArray<UPrimitiveComponent*> Parts; Highlighted->GetComponents(Parts); for(auto* P:Parts) P->SetRenderCustomDepth(false); }
    Highlighted=Actor;
    if(Actor) { TArray<UPrimitiveComponent*> Parts; Actor->GetComponents(Parts); for(auto* P:Parts) { P->SetCustomDepthStencilValue(1); P->SetRenderCustomDepth(true); } }
}
void ACEPlayer::AnimateHands(float Delta)
{
    const float Now=GetWorld()->GetTimeSeconds();
    for(int32 i=0;i<2;++i) {
        const float Side=i==0 ? -1.f : 1.f;
        if(!Hands[i]) {
            Hands[i]=CEVisual::Part(this,TEXT("SM_GripHand"),FVector::ZeroVector,FVector(1.f),FLinearColor(.64f,.40f,.26f));
            Hands[i]->SetMaterial(0,CEVisual::Surface(this,FLinearColor(.64f,.40f,.26f),TEXT("M_Skin")));
            Forearms[i]=CEVisual::Part(this,TEXT("SM_Forearm"),FVector::ZeroVector,FVector(.075f,.075f,.27f),FLinearColor(.64f,.40f,.26f));
            Sleeves[i]=CEVisual::Part(this,TEXT("SM_Sleeve"),FVector::ZeroVector,FVector(.10f,.12f,.30f),FLinearColor(.045f,.12f,.13f));
            for(auto* P:{Hands[i].Get(),Forearms[i].Get(),Sleeves[i].Get()}) { P->SetCastShadow(false); P->SetOnlyOwnerSee(true); P->SetRenderCustomDepth(false); }
        }
        // Use this frame's controller view: camera component rotation updates
        // later and otherwise makes the arms jump when looking down quickly.
        const FTransform View(GetControlRotation(),Camera->GetComponentLocation());
        FVector Wrist=View.TransformPosition(FVector(48.f,Side*20.f,-31.f)); FQuat Grip=View.GetRotation();
        const bool bTwoHands=Held && (Held->Kind==ECEPropKind::Tray || Held->Kind==ECEPropKind::Chair || Held->Kind==ECEPropKind::Laptop);
        if(IsValid(Held) && (i==1 || bTwoHands)) {
            FVector Local;
            if(Held->Kind==ECEPropKind::Tray) Local=FVector(-18,Side*34,-1);
            else if(Held->Kind==ECEPropKind::Chair) Local=FVector(8,Side*25,35);
            else if(Held->Kind==ECEPropKind::Laptop) Local=FVector(-6,Side*16,0);
            else if(Held->Kind==ECEPropKind::Portafilter) Local=FVector(22,0,0);
            else if(Held->IsContainer()) Local=FVector(15,0,1);
            else Local=FVector(0,0,0);
            Wrist=Held->GetActorLocation()+Held->GetActorQuat().RotateVector(Local); Grip=Held->GetActorQuat();
        } else if(Now<ActionUntil) Wrist=View.TransformPosition(FVector(52,Side*14,-20));
        const FVector Shoulder=View.TransformPosition(FVector(2,Side*18,-35));
        const FVector Bend=View.TransformVector(FVector(-2,Side*12,-25));
        // Stable elbow pole gives a continuous arm; wrists follow real body lag.
        const FVector Elbow=Shoulder+(Wrist-Shoulder)*.45f+Bend;
        auto Segment=[](UStaticMeshComponent* Piece,FVector A,FVector B,float Width) {
            const FVector Axis=B-A; Piece->SetWorldLocation((A+B)*.5f); Piece->SetWorldRotation(FQuat::FindBetweenNormals(FVector::UpVector,Axis.GetSafeNormal()));
            Piece->SetWorldScale3D(FVector(Width,Width,Axis.Size()/100.f));
        };
        Segment(Sleeves[i],Shoulder,Elbow,.105f); Segment(Forearms[i],Elbow,Wrist,.077f);
        Hands[i]->SetWorldLocation(Wrist-(Wrist-Elbow).GetSafeNormal()*2.2f); Hands[i]->SetWorldRotation(Grip); Hands[i]->SetWorldScale3D(FVector(1.f,Side,1.f));
        const bool Visible=!Dialogue->IsEditing();
        Hands[i]->SetVisibility(Visible); Forearms[i]->SetVisibility(Visible); Sleeves[i]->SetVisibility(Visible);
    }
    SetHighlight(Dialogue->IsEditing() ? nullptr : FocusActor(250.f));
}
