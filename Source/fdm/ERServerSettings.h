#pragma once
#include "CoreMinimal.h"

namespace ERServer
{
    constexpr int32 DefaultMaxPlayers = 16;
    // Human capacity includes the listen host. No fixed upper player cap;
    // the numeric entry accepts any positive int32 (the slider offers 1..64).
    inline int32 ValidMaxPlayers(int32 Value) { return FMath::Max(1, Value); }
}
