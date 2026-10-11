#pragma once
#include "../../ModuleBase/Module.h"

class AutoCrystal : public Module {
private:
	struct CachedCandidate {
		int x, y, z;
		float cx, cy, cz;
		float targetDamage;
	};

	struct PlacementCache {
		std::vector<CachedCandidate> candidates;

		float minPlaceDamage = 0.f;
		bool  placePackets = false;
		int   count = 1;
	};

	int itemSwapping = 0;
	bool selfTest = false;

	int placeDelay = 0;

	float placeRange = 3.f;
	bool multiPlace = false;
	int multiPlaceCount = 2;
	bool placePackets = false;
	float minPlaceDamage = 0.f;

	int breakDelay = 0;
	int breakPackets = 1;
	float breakRange = 7.f;
	float minBreakDamage = 0.f;

	int placeTimer = 0;
	int breakTimer = 0;
	int pauseTicks = 0;

	bool render = false;
	bool renderFill = true;
	bool renderOutline = true;
	bool renderDamage = true;
	bool renderSmooth = false;
	float renderScale = 1.f;
	float renderSmoothSpeed = 7.f;
	float renderGrow = 0.f;

	UIColor fillColor = UIColor(255, 0, 255, 80);
	UIColor lineColor = UIColor(255, 0, 255, 255);
	UIColor damageColor = UIColor(255, 255, 255, 255);

	struct PlaceRenderData {
		AABB box;
		Vec3<float> center;
		float damage = 0.f;
	};

	std::vector<PlaceRenderData> renderPlacements;
	std::vector<PlaceRenderData> smoothBoxes;
	std::vector<Vec3<float>> smoothCenters;

	std::mt19937 rng;

	std::vector<BlockPos> placementsScratch;
	std::vector<BlockPos> resultsScratch;
	std::vector<size_t>   closeGroupScratch;
	std::vector<bool>     usedScratch;

	void switchTo(int slot) const;
	void switchBack(int oldSlot) const;
	void drawPlacementBox(const Vec3<float>& center, float scale);

	BlockPos getIntegerPosition(Actor* actor) const;
	BlockPos getBestPlacementPos(Actor* actor, float range);
	std::vector<BlockPos> getBestPlacementPositions(Actor* actor, float range, int count);
	bool crystalBlocked(const AABB& crystalAABB) const;

	bool buildPlacementCache(Actor* actor, PlacementCache& cache);
	std::vector<BlockPos> selectFromCandidatesThreaded(PlacementCache& cache);
	std::vector<BlockPos> getBestPlacementPositionsSingle(Actor* actor, float range, int count);

	float computeDamageAt(const Vec3<float>& crystalPos, Actor* actor, float exposure);
	float computeDamageTo(const Vec3<float>& crystalPos, Actor* actor);

	Actor* findTarget(LocalPlayer* localPlayer);

	static inline bool canPlaceCrystalOnBlock(int id) {
		return id == 7 || id == 49;
	}

	static inline bool crystalBoxesIntersect(const BlockPos& a, const BlockPos& b) {
		const int dx = a.x - b.x; if (dx > 1 || dx < -1) return false;
		const int dy = a.y - b.y; if (dy > 1 || dy < -1) return false;
		const int dz = a.z - b.z; if (dz > 1 || dz < -1) return false;
		return true;
	}

public:
	AutoCrystal();

	static inline float computeDamage(const Vec3<float>& position, Actor* actor, float exposure) {
		if (!actor) return 0.f;
		constexpr float explosionSize = 12.f;

		AABBShapeComponent* aabbShape = actor->aabbShape;
		if (!aabbShape) return 0.f;

		const AABB& aabb = aabbShape->aabb;
		const float cx = (aabb.lower.x + aabb.upper.x) * 0.5f;
		const float cz = (aabb.lower.z + aabb.upper.z) * 0.5f;

		const float px = cx - position.x;
		const float py = aabb.lower.y - position.y;
		const float pz = cz - position.z;
		const float distance = sqrtf(px * px + py * py + pz * pz) / explosionSize;
		if (distance > 1.f) return 0.f;

		const float impact = (1.f - distance) * exposure;
		float damage = (float)std::max(
			(int)(((impact * impact + impact) * 0.5f) * (8.f * explosionSize) + 1.f),
			0
		);

		uint8_t armorPoints = 0;
		uint8_t epf = 0;

		for (int i = 0; i < 4; ++i) {
			ItemStack* armor = actor->getArmor(i);
			if (!armor || !armor->isValid()) continue;

			Item* item = armor->item.get();
			if (!item) continue;

			armorPoints += item->getArmorValue();
			epf += 5;
		}

		damage *= 1.f - armorPoints * 0.04f;
		damage *= 1.f - std::min(
			std::ceilf(std::min((float)epf, 25.f) * 0.5f),
			20.f
		) * 0.04f;

		return damage > 0.f ? damage : 0.f;
	}

	static inline float computeDamageToCached(const Vec3<float>& crystalPos, Actor* actor, const AABB& inflatedAabb) {
		if (!actor) return 0.f;
		const float exposure = WorldUtil::getSeenPercent(crystalPos, inflatedAabb);
		return computeDamage(crystalPos, actor, exposure);
	}

	void onEnable() override;
	void onDisable() override;
	void onNormalTick(LocalPlayer* localPlayer) override;
	void onLevelRender() override;
	void onMCRender(MinecraftUIRenderContext* renderCtx) override;
};