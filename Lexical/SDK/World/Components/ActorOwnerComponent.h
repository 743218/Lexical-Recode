#pragma once
#include "IEntityComponent.h"

class ActorOwnerComponent : IEntityComponent {
public:
	class Actor* actor;
};