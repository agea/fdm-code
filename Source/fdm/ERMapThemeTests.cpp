#include "ERProceduralMap.h"
#include "ERMapTheme.h"
#include "Engine/StaticMesh.h"
#include "Misc/AutomationTest.h"
#if WITH_DEV_AUTOMATION_TESTS
IMPLEMENT_SIMPLE_AUTOMATION_TEST(FERMapThemeTest,"EclipseRecoil.Destruction.SharedMapThemes",EAutomationTestFlags::EditorContext|EAutomationTestFlags::EngineFilter)
bool FERMapThemeTest::RunTest(const FString& Parameters)
{
    TArray<int32> Expected;TSet<int32> Seen;
    for(const TCHAR* Name:{TEXT("CentralBuilding"),TEXT("MiddleEasternAlleys"),TEXT("FireTraining"),TEXT("Rooms")})
    {
        auto* Saved=LoadObject<UERMapTemplate>(nullptr,*FString::Printf(TEXT("/Game/EclipseRecoil/Art/MapTemplates/DA_%s"),Name));
        if(!TestNotNull(TEXT("production recipe"),Saved) || !TestNotNull(TEXT("shared library bound"),Saved->ThemeLibrary.Get()))return false;
        auto* Raw=DuplicateObject<UERMapTemplate>(Saved,GetTransientPackage());Raw->ThemeLibrary=nullptr;
        FERProceduralConfig C;FERBlockRoomConfig B;
        for(int32 Seed=0;Seed<32;++Seed)
        {
            C.Seed=Seed;C.TemplateAsset=Raw;FERProceduralLayout Before,After,Again;
            if(!TestTrue(TEXT("original geometry builds"),ERProcedural::Build(C,B,Before)))return false;
            C.TemplateAsset=Saved;
            if(!TestTrue(TEXT("themed geometry builds"),ERProcedural::Build(C,B,After)))return false;
            Seen.Add(After.ThemeIndex);
            const auto& Palette=Saved->ThemeLibrary->Themes[After.ThemeIndex];
            if(Palette.ShellSteel)
            {
                TestTrue(TEXT("warehouse shell, court walls, frames, ladders and decks use distinct finishes"),
                    Palette.ShellSteel!=Palette.Wall && Palette.LadderSteel!=Palette.Materials[uint8(EERBlockMaterial::Metal)] &&
                    Palette.DeckSteel!=Palette.LadderSteel && Palette.DeckSteel!=Palette.Wall && Palette.DeckSteel!=Palette.ShellSteel);
            }
            if(Expected.Num()<32)Expected.Add(After.ThemeIndex);else TestEqual(TEXT("same seed selects same theme across all templates"),After.ThemeIndex,Expected[Seed]);
            TestEqual(TEXT("panel count unchanged"),Before.Panels.Num(),After.Panels.Num());
            for(int32 I=0;I<Before.Panels.Num();++I)
            {
                const auto& A=Before.Panels[I];const auto& D=After.Panels[I];
                if(!TestTrue(TEXT("construction geometry, calibration, openings and support unchanged"),A.Id==D.Id && A.Material==D.Material && A.Strength==D.Strength && A.Transform.Equals(D.Transform) && A.Size==D.Size && A.Cell==D.Cell && A.Holes==D.Holes && A.bAnchorBottom==D.bAnchorBottom && A.bAnchored==D.bAnchored && A.bIndestructible==D.bIndestructible && A.SkinCRC==D.SkinCRC && A.Skin.Num()==D.Skin.Num()))return false;
                if(Palette.PreserveCargoPaint && D.Material==EERBlockMaterial::Metal && D.Strength>1)
                    TestEqual(TEXT("reinforced cargo keeps its authored paint"),D.Surface,A.Surface);
                else TestTrue(TEXT("finish follows physical material or ladder role"),D.Surface==Palette.Materials[uint8(D.Material)] ||
                    (Palette.LadderSteel && D.Material==EERBlockMaterial::Metal && D.Surface==Palette.LadderSteel));
            }
            TestEqual(TEXT("fixed piece count unchanged"),Before.FixedPieces.Num(),After.FixedPieces.Num());
            for(int32 I=0;I<Before.FixedPieces.Num();++I)
            {
                const auto& A=Before.FixedPieces[I];const auto& D=After.FixedPieces[I];
                TestTrue(TEXT("original mesh, dimensions and collision retained"),A.Transform.Equals(D.Transform) && A.Mesh==D.Mesh && A.Material==D.Material && A.Roof==D.Roof && A.VisualOnly==D.VisualOnly && A.MeshCollision==D.MeshCollision && A.CollisionOnly==D.CollisionOnly);
                if(Palette.LadderSteel && D.PawnBarrier)TestEqual(TEXT("parapets use the ladder/detail finish"),D.Surface,Palette.LadderSteel.Get());
                if(Palette.ShellSteel && D.Mesh && D.Mesh->GetName().Contains(TEXT("SideWallBay")))TestEqual(TEXT("warehouse wall sheets retain a dedicated finish"),D.Surface,Palette.ShellSteel.Get());
            }
            if(Palette.LadderSteel)for(const auto& Ladder:After.Ladders)
            {
                const FVector Rung=Ladder.Frame.TransformPosition({0,0,25});
                const auto* Panel=After.Panels.FindByPredicate([&](const auto& D){return D.Material==EERBlockMaterial::Metal && FVector::DistSquared(D.Transform.TransformPosition(D.Size*.5),Rung)<9;});
                if(TestNotNull(TEXT("physical ladder has a first rung"),Panel))TestEqual(TEXT("ladder rungs get the light galvanized finish"),Panel->Surface,Palette.LadderSteel.Get());
            }
            TestTrue(TEXT("same arena and starts"),Before.ArenaSize==After.ArenaSize && Before.AlphaSpawns.Num()==After.AlphaSpawns.Num() && Before.OmegaSpawns.Num()==After.OmegaSpawns.Num());
            Again=Before;TestTrue(TEXT("cosmetic pass deterministic"),ERProcedural::ApplyMapTheme(C,Again));
            TestTrue(TEXT("repeat theme and ground"),Again.ThemeIndex==After.ThemeIndex && Again.GroundSurface==After.GroundSurface);
            for(int32 I=0;I<After.Panels.Num();++I)TestEqual(TEXT("role finishes deterministic"),Again.Panels[I].Surface,After.Panels[I].Surface);
        }
        auto* Invalid=DuplicateObject<UERMapTemplate>(Saved,GetTransientPackage());Invalid->ThemeLibrary=DuplicateObject<UERMapThemeLibrary>(Saved->ThemeLibrary,GetTransientPackage());
        Invalid->ThemeLibrary->Themes[2].Materials.Reset();C.TemplateAsset=Invalid;
        FERProceduralLayout Prior;Prior.ThemeName=TEXT("unchanged");
        TestFalse(TEXT("incomplete theme library rejected"),ERProcedural::Build(C,B,Prior));
        TestEqual(TEXT("rejection leaves output intact"),Prior.ThemeName,FName(TEXT("unchanged")));
        Invalid->ThemeLibrary=DuplicateObject<UERMapThemeLibrary>(Saved->ThemeLibrary,GetTransientPackage());
        if(Invalid->ThemeLibrary->Themes[2].ShellSteel)
        {Invalid->ThemeLibrary->Themes[2].DeckSteel=nullptr;TestFalse(TEXT("partial steel role palette rejected"),ERProcedural::Build(C,B,Prior));}
    }
    TestEqual(TEXT("all themes sampled"),Seen.Num(),3);
    AddInfo(TEXT("ER_SHARED_MAP_THEMES templates=4 seeds=32 geometry=unchanged deterministic=PASS"));return true;
}
#endif
