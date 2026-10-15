#pragma once

#include "CoreMinimal.h"
#include "CEGameplay.h"
#include "Engine/World.h"
#include "Components/StaticMeshComponent.h"
#include "Materials/MaterialInstanceDynamic.h"

namespace CEVisual
{
    inline UMaterialInstanceDynamic* Surface(UObject* Owner,FLinearColor Color,const TCHAR* Name)
    {
        auto* Base=LoadObject<UMaterialInterface>(nullptr,*FString::Printf(TEXT("/Game/Prototype/%s.%s"),Name,Name));
        auto* World=Owner ? Owner->GetWorld() : nullptr; auto* Cafe=World ? World->GetSubsystem<UCECafeSubsystem>() : nullptr;
        const bool Shared=FString(Name)!=TEXT("M_Fabric") && FString(Name)!=TEXT("M_Ceramic") && FString(Name)!=TEXT("M_Screen");
        const FString Key=FString(Name)+Color.ToString();
        if(Shared && Cafe) if(auto* Cached=Cafe->SharedMaterials.FindRef(Key).Get()) return Cached;
        auto* Result=UMaterialInstanceDynamic::Create(Base,Shared && Cafe ? static_cast<UObject*>(Cafe) : Owner);
        if(Shared && Cafe && Result) Cafe->SharedMaterials.Add(Key,Result);
        if(Result) Result->SetVectorParameterValue(TEXT("Tint"),Color);
        return Result;
    }
    inline UMaterialInstanceDynamic* Material(UObject* Owner, FLinearColor Color, bool bUnlit = false, bool bMetal = false)
    {
        bMetal|=!bUnlit && (Color.Equals(FLinearColor(.55f,.60f,.58f),.005f) || Color.Equals(FLinearColor(.42f,.30f,.14f),.005f));
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
        // Manufactured objects share authored bevelled geometry instead of razor-edged cubes.
        if(FString(Shape)==TEXT("Cube")) Shape=TEXT("SM_BevelCube");
        const bool Authored=FString(Shape).StartsWith(TEXT("SM"));
        const FString MeshPath=Authored ? FString::Printf(TEXT("/Game/Prototype/Meshes/%s.%s"),Shape,Shape) : FString::Printf(TEXT("/Engine/BasicShapes/%s.%s"),Shape,Shape);
        auto* Mesh=LoadObject<UStaticMesh>(nullptr,*MeshPath);
        ensureMsgf(Mesh,TEXT("Cafe mesh missing: %s"),*MeshPath);
        Component->SetStaticMesh(Mesh);
        Component->SetCollisionEnabled(ECollisionEnabled::NoCollision);
        if (Actor->GetRootComponent()) Component->SetupAttachment(Actor->GetRootComponent());
        else Actor->SetRootComponent(Component);
        Component->SetRelativeLocation(Location);
        Component->SetRelativeScale3D(Scale);
        Component->SetMaterial(0, Material(Actor, Color, bUnlit));
        const FString Kind(Shape);
        if(Kind==TEXT("SM_Head") || Kind==TEXT("SM_Hand") || Kind==TEXT("SM_Forearm")) Component->SetMaterial(0,Surface(Actor,Color,TEXT("M_Skin")));
        if(Kind==TEXT("SM_Torso") || Kind==TEXT("SM_Sleeve") || Kind==TEXT("SM_Trouser") || Kind==TEXT("SM_Shin")) Component->SetMaterial(0,Surface(Actor,Color,TEXT("M_Fabric")));
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
