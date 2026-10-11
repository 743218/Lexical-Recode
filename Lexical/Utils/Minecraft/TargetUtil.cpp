#include "TargetUtil.h"


bool TargetUtil::isTargetValid(Actor* target, bool isMob, bool friendCheck, float rangeCheck) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();

	if (localPlayer == nullptr)
		return false;

	if (target == nullptr)
		return false;

	if (target == localPlayer)
		return false;

	if (!target->isAlive())
		return false;

	ActorTypeComponent* actorTypeComponent = target->getActorTypeComponent();
	if (actorTypeComponent == nullptr)
		return false;
	uint32_t entityId = actorTypeComponent->id;

	if (!isMob) {
		if (entityId != 319 && entityId != 307) // 319 = player, 307 = npc
			return false;

		if (target->getNameTag()->size() == 0)
			return false;
	}
	else {
		static std::vector<uint32_t> bannedEntityId = {
			64,					// entity.item.name
			69,					// entity.xp_orb.name
			4194372,			// entity.xp_bottle.name
			4194390,			// entity.splash_potion.name
			4194405,			// entity.lingering_potion.name
			95,					// entity.area_effect_cloud.name
			12582992,			// entity.arrow.name
			4194385,			// entity.snowball.name
			4194447,			// entity.wind_charge_projectile.name
			4194393,			// entity.wither_skull.name
			4194395,			// entity.wither_skull_dangerous.name
			72,					// entity.fireworks_rocket.name
			4194391,			// entity.ender_pearl.name
			77,					// entity.fishing_hook.name
			12582985,			// entity.thrown_trident.name
			66,					// entity.falling_block.name
			65,					// entity.tnt.name
		};

		for (uint32_t i : bannedEntityId) {
			if (entityId == i)
				return false;
		}
	}

	if ((localPlayer->getPos().dist(target->getPos())) > rangeCheck) return false;
	if (friendCheck && FriendManager::isInList(*target->getNameTag())) return false;
	return true;
}

AABB TargetUtil::makeAABB(float width, float height, const Vec3<float>& lowerPos) {
	AABB out;
	out.lower = lowerPos.sub(Vec3<float>(width / 2.f, 1.6f, width / 2.f));
	out.upper = out.lower.add(Vec3<float>(width, height, width));
	return out;
}