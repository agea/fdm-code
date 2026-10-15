#include "ERBlockRoom.h"
#include "Components/InstancedStaticMeshComponent.h"
#include "Components/StaticMeshComponent.h"
#include "Engine/World.h"
#include "Engine/DamageEvents.h"
#include "Misc/AutomationTest.h"
#include "Kismet/GameplayStatics.h"
#include <limits>

#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERBlockStorageTest, "EclipseRecoil.Destruction.Storage", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FERBlockStorageTest::RunTest(const FString& Parameters)
{
    auto* World = NewObject<UERBlockWorld>();
    auto* Other = NewObject<UERBlockWorld>();
    auto* Catalog = NewObject<UERBlockMaterialCatalog>();
    FERBlockRoomConfig Config;
    TestTrue(TEXT("Generate"), World->GenerateRoom(Config, Catalog));
    TestTrue(TEXT("Repeat"), Other->GenerateRoom(Config, Catalog));
    TestEqual(TEXT("Four walls with clipped return corners"), World->GetBlocks().Num(), 504);
    for (int32 Index = 0; Index < World->GetBlocks().Num(); ++Index)
    {
        const auto& A = World->GetBlocks()[Index]; const auto& B = Other->GetBlocks()[Index];
        TestTrue(TEXT("Deterministic ID and transform"), A.Id == B.Id && A.LocalTransform.Equals(B.LocalTransform));
        TestTrue(TEXT("Stable lookup"), World->Find(A.Id) == &A);
        // Strict positive overlap is prohibited; touching faces are allowed.
        const FVector ACenter = A.LocalTransform.GetLocation();
        FVector AExtent = A.LocalTransform.GetScale3D() * 50;
        if (A.Id.Wall % 2) Swap(AExtent.X, AExtent.Y);
        for (int32 Next = Index + 1; Next < World->GetBlocks().Num(); ++Next)
        {
            const auto& C = World->GetBlocks()[Next];
            FVector CExtent = C.LocalTransform.GetScale3D() * 50;
            if (C.Id.Wall % 2) Swap(CExtent.X, CExtent.Y);
            const FVector Separation = (ACenter - C.LocalTransform.GetLocation()).GetAbs();
            const FVector Extent = AExtent + CExtent;
            if (Separation.X < Extent.X - .001f && Separation.Y < Extent.Y - .001f && Separation.Z < Extent.Z - .001f)
                AddError(TEXT("Overlapping block volumes"));
        }
    }
    FERBlockId Id;
    for (int32 Wall = 0; Wall < 3; ++Wall)
    {
        Id.Wall = Wall;
        const float Initial = World->Find(Id)->Health;
        TestTrue(TEXT("Damage"), World->Damage(Id, 25));
        TestEqual(TEXT("HP deducted once"), World->Find(Id)->Health, Initial - 25);
        TestTrue(TEXT("Cumulative damage state"), World->Find(Id)->State == EERBlockState::Damaged);
    }
    TestTrue(TEXT("Material resistance order"), World->Find(FERBlockId{0,0,0})->Health < World->Find(FERBlockId{1,0,0})->Health && World->Find(FERBlockId{1,0,0})->Health < World->Find(FERBlockId{2,0,0})->Health);
    Id.Wall = 0;
    TestFalse(TEXT("Negative damage"), World->Damage(Id, -10));
    TestFalse(TEXT("Infinite damage"), World->Damage(Id, std::numeric_limits<float>::infinity()));
    TestTrue(TEXT("Destroy"), World->DestroyBlock(Id));
    TestFalse(TEXT("Repeat destroy"), World->DestroyBlock(Id));
    TestEqual(TEXT("Destroyed counter once"), World->GetDestroyedCount(), 1);
    FERBlockRoomConfig Invalid = Config; Invalid.BlockSize.X = 0;
    TestFalse(TEXT("Reject invalid configuration"), World->GenerateRoom(Invalid, Catalog));
    TestEqual(TEXT("Invalid config preserves storage"), World->GetDestroyedCount(), 1);
    Invalid = Config; Invalid.BlockSize.X = 1; Invalid.BlockSize.Z = 1; Invalid.InteriorSize = 100000;
    TestFalse(TEXT("Reject unbounded allocation"), World->GenerateRoom(Invalid, Catalog));
    Config.InteriorSize = 513; Config.Height = 307;
    TestTrue(TEXT("Nondivisible end caps"), World->GenerateRoom(Config, Catalog));
    TestEqual(TEXT("Regeneration resets damage"), World->GetDestroyedCount(), 0);
    return true;
}

IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERBlockCollisionTest, "EclipseRecoil.Destruction.CollisionAndIdentity", EAutomationTestFlags::EditorContext | EAutomationTestFlags::EngineFilter)
bool FERBlockCollisionTest::RunTest(const FString& Parameters)
{
    UWorld::InitializationValues Values;
    Values.AllowAudioPlayback(false).CreateNavigation(false).CreateAISystem(false).CreateFXSystem(false).EnableTraceCollision(true);
    UWorld* World = UWorld::CreateWorld(EWorldType::Game, false, NAME_None, nullptr, true, ERHIFeatureLevel::Num, &Values);
    if (!TestNotNull(TEXT("Physics test world"), World)) return false;
    AERBlockRoom* Room = World->SpawnActor<AERBlockRoom>();
    if (!TestNotNull(TEXT("Room actor"), Room)) { World->DestroyWorld(false); return false; }
    if (!TestTrue(TEXT("Initial room"), Room->Regenerate())) { World->DestroyWorld(false); return false; }
    TestTrue(TEXT("Initial bidirectional mapping"), Room->ValidateMappings());
    FCollisionQueryParams Params(SCENE_QUERY_STAT(ERBlockCollisionTest), false);
    FHitResult Hit;
    const FVector Inside(0,0,100), Outside(0,-350,100);
    const FCollisionShape Capsule = FCollisionShape::MakeCapsule(34,92.5f);
    TestTrue(TEXT("Full wall stops player capsule"), World->SweepSingleByChannel(Hit, Inside, Outside, FQuat::Identity, ECC_Pawn, Capsule, Params));
    // A point event uses the real engine hit instance, same as authorized weapons.
    TestTrue(TEXT("Point damage wall trace"), World->LineTraceSingleByChannel(Hit, FVector(25,0,162.5f), FVector(25,-350,162.5f), ECC_GameTraceChannel1, Params));
    FERBlockId Target;
    TestTrue(TEXT("Resolve real trace"), Room->ResolveHit(Hit, Target));
    TestEqual(TEXT("Point damage consumed"), UGameplayStatics::ApplyPointDamage(Room, 10, FVector(0,-1,0), Hit, nullptr, nullptr, UDamageType::StaticClass()), 10.f);
    TestEqual(TEXT("Correct logical health"), Room->BlockWorld->Find(Target)->Health, 20.f);
    TArray<FERBlockId> Order;
    for (const auto& Block : Room->BlockWorld->GetBlocks()) Order.Add(Block.Id);
    FRandomStream Random(7919);
    for (int32 Index = Order.Num() - 1; Index > 0; --Index) Order.Swap(Index, Random.RandRange(0, Index));
    for (const auto& Id : Order)
    {
        const auto* Block = Room->BlockWorld->Find(Id);
        const FVector Center = Block->LocalTransform.GetLocation();
        const FVector Normal = Id.Wall == 0 ? FVector(0,-1,0) : Id.Wall == 1 ? FVector(1,0,0) : Id.Wall == 2 ? FVector(0,1,0) : FVector(-1,0,0);
        // Return corner caps touch the perpendicular wall; trace their interior
        // columns only, and verify all end caps through exact transforms/mapping.
        const bool bTrace = !(Id.Wall % 2) || (Id.Column > 0 && Id.Column < 10);
        if (bTrace)
        {
            const bool bHit = World->LineTraceSingleByChannel(Hit, Center - Normal * 40, Center + Normal * 40, ECC_GameTraceChannel1, Params);
            FERBlockId Actual;
            TestTrue(TEXT("Live block collision and correct stable ID after arbitrary previous removals"), bHit && Room->ResolveHit(Hit, Actual) && Actual == Id);
        }
        TestTrue(TEXT("Random removal"), Room->ApplyBlockDamage(Id, 0, true));
        TestTrue(TEXT("Surviving instance IDs/transforms"), Room->ValidateMappings());
        if (bTrace)
            TestFalse(TEXT("Removed block no longer blocks trace"), World->LineTraceSingleByChannel(Hit, Center - Normal * 40, Center + Normal * 40, ECC_GameTraceChannel1, Params));
    }
    TestEqual(TEXT("Entire room destroyed"), Room->GetInstanceCount(), 0);
    TestFalse(TEXT("Cleared wall permits full player capsule"), World->SweepSingleByChannel(Hit, Inside, Outside, FQuat::Identity, ECC_Pawn, Capsule, Params));
    TestTrue(TEXT("Floor still collides"), World->LineTraceSingleByChannel(Hit, FVector(0,0,50), FVector(0,0,-50), ECC_GameTraceChannel1, Params));
    TestTrue(TEXT("Floor is a separate indestructible component"), Hit.GetComponent() == Room->Floor);
    FERBlockId FloorId;
    TestFalse(TEXT("Floor cannot resolve to a block"), Room->ResolveHit(Hit, FloorId));
    for (int32 Repeat = 0; Repeat < 5; ++Repeat)
    {
        TestTrue(TEXT("Regenerate"), Room->Regenerate());
        TestTrue(TEXT("Regenerated mapping"), Room->ValidateMappings());
        TestEqual(TEXT("No duplicate instances"), Room->GetInstanceCount(), 504);
        TArray<UInstancedStaticMeshComponent*> Components;
        Room->GetComponents(Components);
        TestEqual(TEXT("No obsolete or duplicate ISM components"), Components.Num(), Room->GetGroupCount());
    }
    World->DestroyWorld(false);
    return true;
}
#endif
