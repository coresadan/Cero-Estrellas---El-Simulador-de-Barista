using UnrealBuildTool;
public class CeroEstrellas : ModuleRules
{
    public CeroEstrellas(ReadOnlyTargetRules Target) : base(Target)
    {
        PCHUsage = PCHUsageMode.UseExplicitOrSharedPCHs;
        PublicDependencyModuleNames.AddRange(new [] { "Core", "CoreUObject", "Engine", "InputCore", "PhysicsCore" });
        PrivateDependencyModuleNames.AddRange(new [] { "Json", "JsonUtilities" });
    }
}
