#pragma once
#include "../../ModuleBase/Module.h"

class AutoCrystal : public Module {
private:
	struct Fade {
	public:
		float deltaTime;
		Vec3<float> fadePos;
		float duration;
		Fade(const Vec3<float>& pos, const float& _dur) {
			duration = _dur;
			fadePos = pos;
			deltaTime = 0.f;
		}
	};
	class CrystalData {
	public:
		float localDamage;
		float targetDamage;
	protected:
		static inline const float computeExplosionDamage(const Vec3<float>& position, Actor* actor, const float& exposure) {
			static constexpr uint8_t explosionSize = 12;
			const AABB& entityAABB = actor->aabbShape->aabb;
			const Vec3<float>& center = entityAABB.getCenter();
			const Vec3<float>& feetPos = Vec3<float>(center.x, entityAABB.lower.y, center.z);
			const float distance = feetPos.dist(position) / explosionSize;
			if (distance <= 1.f) {
				const float impact = (1.f - distance) * exposure;
				const uint16_t damage = std::max((int)(((impact * impact + impact) / 2) * 8 * explosionSize + 1), 0);
				uint8_t armorPoints = 0;
				uint8_t epf = 0;
				for (int i = 0; i < 4; i++) {
					const ItemStack* armor = actor->getArmor(i);
					if (armor->isValid()) {
						armorPoints += armor->item.get()->getArmorValue();
						epf += 5;
					}
				}
				static constexpr uint8_t maxEpf = 25;
				float finalDamage = damage;
				finalDamage -= finalDamage * armorPoints * 0.04f;
				finalDamage -= finalDamage * std::min(std::ceilf(std::min(epf, maxEpf) * 0.5f), 20.f) * 0.04f;
				return std::max(0.f, finalDamage);
			}
			return 0.0f;
		}
	};
	class Place : public CrystalData {
	public:
		BlockPos placePos;
		Actor* actor;
		Place(LocalPlayer* localPlayer, const BlockPos& _blockPos, Actor* _actor, const float& exposure) {
			this->placePos = _blockPos;
			this->targetDamage = computeExplosionDamage(_blockPos.CastTo<float>().add(0.5f, 1.f, 0.5f), _actor, exposure);
			this->localDamage = computeExplosionDamage(_blockPos.CastTo<float>().add(0.5f, 1.f, 0.5f), localPlayer, exposure);
			this->actor = _actor;
		}
	};
	class Break : public CrystalData {
	public:
		Actor* endCrystal;
		Break(LocalPlayer* localPlayer, Actor* _endCrystal, Actor* _actor, const float& exposure) {
			const Vec3<float>& crystalPos = _endCrystal->getPos();
			this->endCrystal = _endCrystal;
			this->targetDamage = computeExplosionDamage(crystalPos, _actor, exposure);
			this->localDamage = computeExplosionDamage(crystalPos, localPlayer, exposure);
		}
	};
	class Terrain : public CrystalData {
	public:
		BlockPos placePos;
		Terrain(const BlockPos& _blockPos, Actor* _actor, const float& exposure) {
			this->placePos = _blockPos;
			this->targetDamage = computeExplosionDamage(_blockPos.CastTo<float>().add(0.5f, 1.f, 0.5f), _actor, exposure);
		}
	};
public:
	std::vector<Break> breakList; 
	std::vector<Place> placeList;
	std::vector<Terrain> terrainList;
	std::vector<Actor*> entityList;
	std::vector<Actor*> targetList;
	std::vector<Fade> fadeList;
public:
	uint64_t highestID = -1;
	bool check = false;
private:
	bool place = true;
	bool explode = true;
	bool packetPlace = false;

	int breakDelay = 0;
	int placeDelay = 0;

	float targetRange = 12.f;
	float placeRange = 6.f;
	float breakRange = 6.f;
	float placeProximity = 12.f;

	float enemyDamage = 4.f;
	float selfDamage = 20.f;

	int priority = 0;

	bool multiTask = true;
	bool java = false;
	bool rotate = false;
	bool phasePlace = false;
	int wasteAmount = 1;
	bool self = false;

	int switchMode = 0;
	bool alternate = false;
	bool idPredict = false;
	bool preventKick = false;
	int idPackets = 4;
	int idDelay = 0;
	float idDamage = 4.f;

	bool obsidian = false;
	float minTerrainDamage = 6.f;
	int terrainProximity = 6;
	int blocksPerTick = 4;
	int terrainDelay = 0;
	bool packetTerrain = false;
	bool airplace = false;

	bool render = true;
	bool lerp = false;
	bool dmgText = true;
	bool flat = false;
	bool fade = false;
	float fadeDuration = 1.f;
	float lerpSpeed = 10.f;
	float lineWidth = 0.3f;
	UIColor damageColor = UIColor(255, 255, 255, 255);
	UIColor lineColor = UIColor(255, 255, 255, 60);
	UIColor color = UIColor(255, 255, 255, 60);

	uint8_t oldSlot = -1;
protected:
	float getExposure(const Vec3<float>& blockPos, Actor* actor) const;
	static bool sortByDamage(CrystalData d1, CrystalData d2);
	bool isPlaceValid(const BlockPos& blockPos, Actor* actor);
	void generatePlacement(LocalPlayer* localPlayer, Actor* actor);
	void getCrystalList(LocalPlayer* localPlayer, Actor* actor);
	void attackCrystal(GameMode* gm, LocalPlayer* localPlayer);
	void placeCrystal(GameMode* gm, LocalPlayer* localPlayer);
	bool isTerrainValid(const BlockPos& blockPos, Actor* actor);
	void placeObsidian(GameMode* gm);
	void getTerrainList(Actor* actor);
	void onTerrain(LocalPlayer* localPlayer, Actor* actor);
	bool isInList(const BlockPos& blockPos);
public:
	AutoCrystal();
	virtual void onNormalTick(LocalPlayer* localPlayer) override;
	virtual void onLevelRender() override;
	virtual void onMCRender(MinecraftUIRenderContext* renderCtx) override;
	virtual void onSendPacket(Packet* packet, bool& cancel) override;
	virtual void onDisable() override;
};