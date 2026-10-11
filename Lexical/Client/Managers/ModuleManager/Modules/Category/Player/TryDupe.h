#pragma once

#include "Dupe.h"

// Dupe1: standalone front end for the proven anvildupe2 duplication path
// (Dupe's WorkingDupe mode = internal mode 1 -> activeMode 5).
// Reuses Dupe's full transaction engine with the mode locked and a trimmed UI.
class TryDupe1 final : public Dupe {
public:
    TryDupe1();
};