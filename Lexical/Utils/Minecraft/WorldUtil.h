#pragma once
#include "../../SDK/Game.h"

namespace WorldUtil {
	Block* getBlock(const BlockPos& blockPos);
	BlockLegacy* getBlockLegacy(const BlockPos& blockPos);
	bool isContainer(const BlockPos& blockPos);
	bool canBuildOn(const BlockPos& blockPos);
	float distanceToPoint(const Vec3<float>& pos, const Vec3<float>& point);
	float distanceToEntity(const Vec3<float>& pos, Actor* entity);
	float distanceToBlock(const Vec3<float>& pos, const BlockPos& blockPos);
	float getSeenPercent(const Vec3<float>& pos, const AABB& aabb);
	float getSeenPercent(const Vec3<float>& pos, Actor* actor);
	float getExposure(const Vec3<float>& pos, const AABB& aabb, const int& ignoredBlock);
	bool rayCast(const Vec3<float>& start, const Vec3<float>& end, const int& ignoredBlock);
	float sign(const float f);
	float boundary(float start, float distance);
	std::string getEntityNameTags(Actor* entity);
}
