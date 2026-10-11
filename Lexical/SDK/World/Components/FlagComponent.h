#pragma once
#include "IEntityComponent.h"

template<typename T>
class FlagComponent : IEntityComponent {};

// struct OnGroundFlag; doesnt exist in 1.21.2
struct OnGroundFlagComponent : IEntityComponent {};

struct JumpFromGroundRequestFlag {};
struct PlayerComponentFlag {};
struct HorizontalCollisionFlag {};
struct InWaterFlag {};
struct InLavaFlag {};