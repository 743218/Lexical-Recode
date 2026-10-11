#include "WorldUtil.h"
#include "../StringUtil.h"

BlockLegacy* WorldUtil::getBlockLegacy(const BlockPos& blockPos) {
	return getBlock(blockPos)->blockLegacy;
}

bool WorldUtil::isContainer(const BlockPos& blockPos) {
	return getBlockLegacy(blockPos)->blockId == 130 || getBlockLegacy(blockPos)->blockId == 54 || getBlockLegacy(blockPos)->blockId == 146 || getBlockLegacy(blockPos)->blockId == 145;
}

Block* WorldUtil::getBlock(const BlockPos& blockPos) {
	return Game::clientInstance->getRegion()->getBlock(blockPos);
}

bool WorldUtil::canBuildOn(const BlockPos& blockPos) {
	ClientInstance* ci = Game::clientInstance;
	if (!ci) return false;
	return getBlock(blockPos)->blockLegacy->canBeBuiltOver(ci->getRegion(), blockPos);
}

float WorldUtil::distanceToPoint(const Vec3<float>& pos, const Vec3<float>& point) {
	return pos.dist(point);
}

float WorldUtil::distanceToEntity(const Vec3<float>& pos, Actor* entity) {
	return distanceToPoint(pos, entity->getPos());
}

float WorldUtil::distanceToBlock(const Vec3<float>& pos, const BlockPos& blockPos) {
	return distanceToPoint(pos, blockPos.CastTo<float>().add(Vec3<float>(0.5f, 0.5f, 0.5f)));
}

float WorldUtil::getSeenPercent(const Vec3<float>& pos, const AABB& aabb) {
	float seenPercent = Game::clientInstance->getRegion()->getSeenPercent(pos, aabb);
	if (seenPercent > 1.f) return 1.f; //fixes weird bug when player is phased in the z axis making exposure output more than 1
	return seenPercent;
}

float WorldUtil::getSeenPercent(const Vec3<float>& pos, Actor* actor) {
	return getSeenPercent(pos, actor->aabbShape->aabb);
}

float WorldUtil::getExposure(const Vec3<float>& pos, const AABB& aabb, const int& ignoredBlock) {
	const BlockPos& blockPos = pos.CastTo<int>();
	Block* block = WorldUtil::getBlock(blockPos);
	if (block->blockLegacy->blockId != ignoredBlock) return WorldUtil::getSeenPercent(pos, aabb);
	else {
		const Vec3<float>& min = aabb.lower;
		if (pos.y > min.y) {
			if (WorldUtil::getBlock(pos.sub(0.f, 1.f, 0.f).CastTo<int>())->blockLegacy->isSolid()) return 0.f;
		}
	}
	return 0.f;
}

bool WorldUtil::rayCast(const Vec3<float>& start, const Vec3<float>& end, const int& ignoredBlock) {
	Vec3<float> current = Vec3<float>(start.x, start.y, start.z);
	Vec3<float> direction = end.sub(start).normalize();

	const float stepX = sign(direction.x);
	const float stepY = sign(direction.y);
	const float stepZ = sign(direction.z);

	float tMaxX = boundary(start.x, direction.x);
	float tMaxY = boundary(start.y, direction.y);
	float tMaxZ = boundary(start.z, direction.z);

	const float tDeltaX = direction.x == 0 ? 0 : stepX / direction.z;
	const float tDeltaY = direction.y == 0 ? 0 : stepY / direction.y;
	const float tDeltaZ = direction.z == 0 ? 0 : stepZ / direction.z;

	const float radius = start.dist(end);

	while (true) {
		Block* block = WorldUtil::getBlock(current.CastTo<int>()); 
		if (block->blockLegacy->isSolid()) {
			if (current == start && block->blockLegacy->blockId != ignoredBlock) return true;
			if (current != start) return true;
		}

		if (tMaxX < tMaxY && tMaxX < tMaxZ) {
			if (tMaxX > radius) break;
			current.x += stepX;
			tMaxX += tDeltaX;
		}
		else if (tMaxY < tMaxZ) {
			if (tMaxY > radius) break;
			current.y += stepY;
			tMaxY += tDeltaY;
		}
		else {
			if (tMaxZ > radius) break;
			current.z += stepZ;
			tMaxZ += tDeltaZ;
		}
	}
	return false;
}

float WorldUtil::sign(const float f) {
	if (f > 0) return 1;
	if (f < 0) return -1;
	return 0.f;
}

float WorldUtil::boundary(float start, float distance) {
	if (distance == 0) return std::numeric_limits<float>::infinity();
	if (distance < 0) {
		start = -start;
		distance = -distance;
		if (floor(start) == start) return 0;
	}
	return (1 - (start - floor(start))) / distance;
}

std::string WorldUtil::getEntityNameTags(Actor* entity) {
	uint32_t entityId = entity->getActorTypeComponent()->id;

	switch (entityId) {
	case 319: {
		// Player
		return StringUtil::sanitize(*entity->getNameTag());
		break;
	}
	case 64: {
		// Item
		ItemStack* stack = (ItemStack*)((uintptr_t)entity + 0x448);

		if (!stack->isValid())
			return std::string("No item");

		std::stringstream textbuild;

		if (stack->mCount > 1)
			textbuild << std::to_string(stack->mCount) << "x ";

		bool wasSpace = true;
		std::string name = *(std::string*)(stack->item.get() + 0xD0);
		for (int i = 0; i < name.size(); i++) {
			if (wasSpace) {
				name[i] = toupper(name[i]);
				wasSpace = false;
			}

			if (name[i] == '_') {
				wasSpace = true;
				name[i] = ' ';
			}
		}

		textbuild << name;

		return textbuild.str();
		break;
	}
	default: {
		std::string name = entity->getEntityLocNameString();
		name.erase(0, 7);	// erase "entity."
		bool wasSpace = true;
		for (int i = 0; i < name.size(); i++) {
			if (wasSpace) {
				name[i] = toupper(name[i]);
				wasSpace = false;
			}

			if (name[i] == '_') {
				wasSpace = true;
				name[i] = ' ';
			}

			if (name[i] == '.') {
				name.erase(i, 5);	// erase ".name"
				break;
			}
		}
		return name;
		break;
	}
	}

	return "NULL";
}