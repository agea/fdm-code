#pragma once

#include "CoreMinimal.h"
#include "Fonts/CompositeFont.h"
#include "Fonts/SlateFontInfo.h"
#include "Misc/Paths.h"

// One font source for menu titles, performance overlay and the in-game HUD.
namespace ERInterface
{
inline FSlateFontInfo Font(int32 Size)
{
    static const auto Typeface = MakeShared<FCompositeFont>(FName(TEXT("SemiBold")),
        FPaths::ProjectDir() / TEXT("Branding/Fonts/Teko/Fonts/Teko-SemiBold.ttf"),
        EFontHinting::Default, EFontLoadingPolicy::LazyLoad);
    return FSlateFontInfo(Typeface, Size);
}
}
