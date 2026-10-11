#pragma once

#include "CoreMinimal.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace CEVisual
{
    inline UMaterialInstanceDynamic* Material(UObject* Owner, FLinearColor Color, bool bUnlit = false, bool bMetal = false)
    {
        const bool bWood=!bUnlit && !bMetal && Color.Equals(FLinearColor(.29f,.14f,.06f),.005f);
        auto* Base = LoadObject<UMaterialInterface>(nullptr, bUnlit ? TEXT("/Game/Prototype/M_Unlit.M_Unlit") : bMetal ? TEXT("/Game/Prototype/M_Metal.M_Metal") : bWood ? TEXT("/Game/Prototype/M_Wood.M_Wood") : TEXT("/Game/Prototype/M_Surface.M_Surface"));
        auto* Result = UMaterialInstanceDynamic::Create(Base, Owner);
        if (Result) Result->SetVectorParameterValue(TEXT("Tint"), Color);
        return Result;
    }

    inline UStaticMeshComponent* Part(AActor* Actor, const TCHAR* Shape, FVector Location, FVector Scale, FLinearColor Color, bool bUnlit = false)
    {
        auto* Component = NewObject<UStaticMeshComponent>(Actor);
        Actor->AddInstanceComponent(Component);
        Component->SetMobility(EComponentMobility::Movable);
        const FString MeshPath=FString(Shape).StartsWith(TEXT("SM_")) ? FString::Printf(TEXT("/Game/Prototype/Meshes/%s.%s"),Shape,Shape) : FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),Shape,Shape);
        Component->SetStaticMesh(LoadObject<UStaticMesh>(nullptr,*MeshPath));
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (Actor->GetRootComponent()) Component->SetupAttachment(Actor->GetRootComponent());
        else Actor->SetRootComponent(Component);
        Component->SetRelativeLocation(Location);
        Component->SetRelativeScale3D(Scale);
        Component->SetMaterial(0, Material(Actor, Color, bUnlit));
        Component->RegisterComponent();
        return Component;
    }

    inline void Stain(UWorld* World, FVector Location, bool bFloor = true, AActor* Parent = nullptr)
    {
        const FVector Scale=bFloor ? FVector(.85f,.7f,.008f) : FVector(.015f,.30f,.3f);
        if (Parent)
        {
            TArray<UStaticMeshComponent*> Existing;
            Parent->GetComponents(Existing);
            int32 Count=0; for(auto* Mesh:Existing) if(Mesh->ComponentHasTag(TEXT("LiquidStain"))) ++Count;
            if(Count>=4) return;
            // Owned components follow the target and disappear with it on customer exit.
            auto* Stain=Part(Parent,bFloor ? TEXT("Cylinder") : TEXT("Sphere"),Parent->GetActorTransform().InverseTransformPosition(Location),Scale/Parent->GetActorScale3D(),FLinearColor(.09f,.027f,.012f));
            Stain->ComponentTags.Add(TEXT("LiquidStain"));
            return;
        }
        auto* Actor = World->SpawnActor<AActor>(Location, FRotator::ZeroRotator);
        Part(Actor, bFloor ? TEXT("Cylinder") : TEXT("Sphere"), FVector::ZeroVector,
            Scale, FLinearColor(.09f, .027f, .012f));
        Actor->SetActorLocation(Location);
        Actor->SetLifeSpan(75.f);
    }
}
