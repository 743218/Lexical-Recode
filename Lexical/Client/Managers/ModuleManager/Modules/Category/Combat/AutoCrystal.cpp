#include "AutoCrystal.h"
#include "../../../ModuleManager.h"
#include "KillAura.h"

AutoCrystal::AutoCrystal() : Module("AutoCrystal", "Place and break end crystals automatically.", Category::COMBAT) {
	{
		std::random_device rd;
		rng.seed(((uint64_t)rd() << 32) ^ (uint64_t)rd());
	}

	renderPlacements.reserve(8);
	placementsScratch.reserve(8);
	resultsScratch.reserve(8);
	closeGroupScratch.reserve(8);
	usedScratch.reserve(8);

	registerSetting(new EnumSetting("Switch", "The switching method.", { "None", "Switch", "Silent", "MobEq" }, &itemSwapping, 0));

	registerSetting(new BoolSetting("Self", "Test on yourself.", &selfTest, false));

	registerSetting(new SliderSetting<int>("Place Delay", "Delay before placing.", &placeDelay, 0, 0, 20));

	registerSetting(new SliderSetting<float>("Place Range", "Maximum placement range.", &placeRange, 3.f, 1.f, 6.f));

	registerSetting(new BoolSetting("MultiPlace", "Place multiple crystals per tick.", &multiPlace, false));

	registerSetting(new SliderSetting<int>("MultiPlace Count", "How many crystals to place.", &multiPlaceCount, 2, 2, 5, [this]() -> bool {
		return multiPlace;
		}));

	registerSetting(new BoolSetting("Intersect", "Place crystals whose AABBs intersect each other (server speedup).", &placePackets, false, [this]() -> bool {
		return multiPlace;
		}));

	registerSetting(new SliderSetting<float>("Min Place Damage", "Only place if the crystal deals at least this much damage.", &minPlaceDamage, 0.f, 0.f, 20.f));

	registerSetting(new SliderSetting<int>("Break Delay", "Delay before breaking.", &breakDelay, 0, 0, 20));

	registerSetting(new SliderSetting<int>("Break Packets", "Number of attack packets.", &breakPackets, 1, 1, 10));

	registerSetting(new SliderSetting<float>("Break Range", "Maximum crystal break range.", &breakRange, 7.f, 1.f, 10.f));

	registerSetting(new SliderSetting<float>("Min Break Damage", "Only break if the crystal deals at least this much damage.", &minBreakDamage, 0.f, 0.f, 20.f));

	registerSetting(new BoolSetting("Render", "Render placement info.", &render, false));

	registerSetting(new BoolSetting("Smooth", "Smooth grow/shrink and position animation.", &renderSmooth, false, [this]() -> bool {
		return render;
		}));

	registerSetting(new SliderSetting<float>("Smooth Speed", "Speed of the smooth animation.", &renderSmoothSpeed, 7.f, 1.f, 20.f, [this]() -> bool {
		return render && renderSmooth;
		}));

	registerSetting(new BoolSetting("Box Fill", "Render filled box.", &renderFill, true, [this]() -> bool {
		return render;
		}));

	registerSetting(new BoolSetting("Box Line", "Render box outline.", &renderOutline, true, [this]() -> bool {
		return render;
		}));

	registerSetting(new BoolSetting("Damage Text", "Render damage text.", &renderDamage, true, [this]() -> bool {
		return render;
		}));

	registerSetting(new SliderSetting<float>("Text Scale", "Scale of the damage text.", &renderScale, 1.f, 0.5f, 2.f, [this]() -> bool {
		return render && renderDamage;
		}));

	registerSetting(new ColorSetting("Fill Color", "NULL", &fillColor, fillColor, true, true, [this]() -> bool {
		return render && renderFill;
		}));

	registerSetting(new ColorSetting("Line Color", "NULL", &lineColor, lineColor, true, true, [this]() -> bool {
		return render && renderOutline;
		}));

	registerSetting(new ColorSetting("Damage Color", "NULL", &damageColor, damageColor, true, true, [this]() -> bool {
		return render && renderDamage;
		}));
}

void AutoCrystal::onEnable() {
	placeTimer = 0;
	breakTimer = 0;
	pauseTicks = 0;
	renderPlacements.clear();
	placementsScratch.clear();
	resultsScratch.clear();
	closeGroupScratch.clear();
	usedScratch.clear();
}

void AutoCrystal::onDisable() {
	renderPlacements.clear();
	placementsScratch.clear();
	resultsScratch.clear();
	closeGroupScratch.clear();
	usedScratch.clear();
}

void AutoCrystal::switchTo(int slot) const {
	if (itemSwapping == 0) return;
	if (slot < 0 || slot > 8) return;
	bool mobEq = itemSwapping == 3;
	if (mobEq) InventoryUtil::sendMobEquipment(slot);
	InventoryUtil::switchSlot(slot);
}

void AutoCrystal::switchBack(int oldSlot) const {
	if (itemSwapping <= 1) return;
	if (oldSlot < 0 || oldSlot > 8) return;
	InventoryUtil::switchSlot(oldSlot);
}

BlockPos AutoCrystal::getIntegerPosition(Actor* actor) const {
	if (!actor) return BlockPos(0, 0, 0);
	return actor->getPos().floor().CastTo<int>();
}

float AutoCrystal::computeDamageAt(const Vec3<float>& crystalPos, Actor* actor, float exposure) {
	if (!actor) return 0.f;
	return AutoCrystal::computeDamage(crystalPos, actor, exposure);
}

float AutoCrystal::computeDamageTo(const Vec3<float>& crystalPos, Actor* actor) {
	if (!actor) return 0.f;

	AABBShapeComponent* aabbShape = actor->aabbShape;
	if (!aabbShape) return 0.f;

	AABB aabb = aabbShape->aabb;
	aabb.lower.x -= 0.1f;
	aabb.lower.z -= 0.1f;
	aabb.upper.x += 0.1f;
	aabb.upper.z += 0.1f;

	const float exposure = WorldUtil::getSeenPercent(crystalPos, aabb);
	return AutoCrystal::computeDamage(crystalPos, actor, exposure);
}

Actor* AutoCrystal::findTarget(LocalPlayer* localPlayer) {
	if (!localPlayer || !localPlayer->level) return nullptr;
	if (selfTest) return localPlayer;

	Actor* target = nullptr;
	float closestDistance = FLT_MAX;

	for (Actor* actor : localPlayer->level->getRuntimeActorList()) {
		if (!actor || actor == localPlayer) continue;
		if (!TargetUtil::isTargetValid(actor, false, true, 10.f)) continue;

		float distance = localPlayer->getPos().dist(actor->getPos());
		if (distance < closestDistance) {
			closestDistance = distance;
			target = actor;
		}
	}

	return target;
}

bool AutoCrystal::crystalBlocked(const AABB& crystalAABB) const {
	LocalPlayer* lp = Game::getLocalPlayer();
	if (!lp || !lp->level) return false;

	const bool skipEndCrystals = true;
	const Vec3<float> center = crystalAABB.getCenter();

	for (Actor* actor : lp->level->getRuntimeActorList()) {
		if (!actor) continue;
		if (!actor->aabbShape) continue;
		if (actor->getPos().dist(center) > 3.f) continue;

		auto* tc = actor->getActorTypeComponent();
		if (!tc) continue;
		if (skipEndCrystals && tc->id == 71) continue;

		AABB box = actor->aabbShape->aabb;
		if (tc->id == 319) {
			box.lower.x -= 0.1f; box.lower.z -= 0.1f;
			box.upper.x += 0.1f; box.upper.z += 0.1f;
		}

		if (crystalAABB.intersects(box)) return true;
	}
	return false;
}

bool AutoCrystal::buildPlacementCache(Actor* actor, PlacementCache& cache) {
	cache.candidates.clear();

	if (!actor || !actor->aabbShape) return false;

	const BlockPos actorPos = getIntegerPosition(actor).sub(0, 1, 0);
	const int r = (int)placeRange;

	constexpr int kYRange = 2;
	const int minY = actorPos.y - kYRange;
	const int maxY = actorPos.y + kYRange;

	const float rangeSq = placeRange * placeRange;
	const Vec3<float> actorPosFloat = actorPos.CastTo<float>();

	AABB targetAabb = actor->aabbShape->aabb;
	targetAabb.lower.x -= 0.1f;
	targetAabb.lower.z -= 0.1f;
	targetAabb.upper.x += 0.1f;
	targetAabb.upper.z += 0.1f;

	cache.candidates.reserve(64);

	for (int x = actorPos.x - r; x <= actorPos.x + r; ++x) {
		float dxBase = (float)x + 0.5f - actorPosFloat.x;
		float dxBaseSq = dxBase * dxBase;
		if (dxBaseSq > rangeSq) continue;

		for (int z = actorPos.z - r; z <= actorPos.z + r; ++z) {
			float dzBase = (float)z + 0.5f - actorPosFloat.z;
			float dxzSq = dxBaseSq + dzBase * dzBase;
			if (dxzSq > rangeSq) continue;

			for (int y = minY; y <= maxY; ++y) {
				float dyBase = (float)y + 1.f - actorPosFloat.y;
				if (dxzSq + dyBase * dyBase > rangeSq) continue;

				Block* block = WorldUtil::getBlock(BlockPos(x, y, z));
				if (!block || !block->blockLegacy) continue;
				if (!canPlaceCrystalOnBlock(block->blockLegacy->blockId)) continue;

				Block* crystalBlock = WorldUtil::getBlock(BlockPos(x, y + 1, z));
				if (!crystalBlock || !crystalBlock->blockLegacy || crystalBlock->blockLegacy->blockId != 0) continue;

				Block* crystalTop = WorldUtil::getBlock(BlockPos(x, y + 2, z));
				if (!crystalTop || !crystalTop->blockLegacy || crystalTop->blockLegacy->blockId != 0) continue;

				AABB crystalAABB;
				crystalAABB.lower = Vec3<float>((float)x, (float)y, (float)z);
				crystalAABB.upper = Vec3<float>((float)x + 1.f, (float)y + 2.f, (float)z + 1.f);

				if (crystalBlocked(crystalAABB)) continue;

				const Vec3<float> crystalPos((float)x + 0.5f, (float)y + 1.f, (float)z + 0.5f);

				const float damage = computeDamageToCached(crystalPos, actor, targetAabb);
				if (damage <= 0.f || damage < minPlaceDamage) continue;

				cache.candidates.push_back({ x, y, z, crystalPos.x, crystalPos.y, crystalPos.z, damage });
			}
		}
	}

	cache.minPlaceDamage = minPlaceDamage;
	cache.placePackets = placePackets;
	cache.count = multiPlace ? multiPlaceCount : 1;
	return true;
}

std::vector<BlockPos> AutoCrystal::selectFromCandidatesThreaded(PlacementCache& cache) {
	resultsScratch.clear();
	if (cache.candidates.empty() || cache.count <= 0) return resultsScratch;

	std::sort(cache.candidates.begin(), cache.candidates.end(),
		[](const CachedCandidate& a, const CachedCandidate& b) noexcept {
			return a.targetDamage > b.targetDamage;
		});

	if (!cache.placePackets) {
		for (const CachedCandidate& candidate : cache.candidates) {
			if ((int)resultsScratch.size() >= cache.count) break;

			bool intersects = false;
			for (const BlockPos& chosen : resultsScratch) {
				if (chosen.y == candidate.y &&
					crystalBoxesIntersect(chosen, BlockPos(candidate.x, candidate.y, candidate.z))) {
					intersects = true;
					break;
				}
			}

			if (!intersects)
				resultsScratch.push_back(BlockPos(candidate.x, candidate.y, candidate.z));
		}
	}
	else {
		usedScratch.assign(cache.candidates.size(), false);

		for (size_t i = 0; i < cache.candidates.size() && (int)resultsScratch.size() < cache.count; ++i) {
			bool intersects = resultsScratch.empty();
			if (!intersects) {
				for (const BlockPos& chosen : resultsScratch) {
					if (crystalBoxesIntersect(chosen, BlockPos(cache.candidates[i].x, cache.candidates[i].y, cache.candidates[i].z))) {
						intersects = true;
						break;
					}
				}
			}

			if (intersects) {
				resultsScratch.push_back(BlockPos(cache.candidates[i].x, cache.candidates[i].y, cache.candidates[i].z));
				usedScratch[i] = true;
			}
		}

		for (size_t i = 0; i < cache.candidates.size() && (int)resultsScratch.size() < cache.count; ++i) {
			if (!usedScratch[i])
				resultsScratch.push_back(BlockPos(cache.candidates[i].x, cache.candidates[i].y, cache.candidates[i].z));
		}
	}

	return resultsScratch;
}

std::vector<BlockPos> AutoCrystal::getBestPlacementPositionsSingle(Actor* actor, float range, int count) {
	PlacementCache cache;
	if (!buildPlacementCache(actor, cache)) return {};
	cache.count = count;
	return selectFromCandidatesThreaded(cache);
}

std::vector<BlockPos> AutoCrystal::getBestPlacementPositions(Actor* actor, float range, int count) {
	return getBestPlacementPositionsSingle(actor, range, count);
}

BlockPos AutoCrystal::getBestPlacementPos(Actor* actor, float range) {
	if (!actor) return BlockPos(0, -999, 0);

	AABBShapeComponent* aabbShape = actor->aabbShape;
	if (!aabbShape) return BlockPos(0, -999, 0);

	const BlockPos actorPos = getIntegerPosition(actor).sub(0, 1, 0);
	const int r = (int)range;

	constexpr int kYRange = 2;
	const int minX = actorPos.x - r;
	const int maxX = actorPos.x + r;
	const int minY = actorPos.y - kYRange;
	const int maxY = actorPos.y + kYRange;
	const int minZ = actorPos.z - r;
	const int maxZ = actorPos.z + r;

	const float rangeSq = range * range;
	const Vec3<float> actorPosFloat = actorPos.CastTo<float>();

	AABB targetAabb = aabbShape->aabb;
	targetAabb.lower.x -= 0.1f;
	targetAabb.lower.z -= 0.1f;
	targetAabb.upper.x += 0.1f;
	targetAabb.upper.z += 0.1f;

	BlockPos bestPos(0, -999, 0);
	float bestDamage = 0.f;

	for (int x = minX; x <= maxX; ++x) {
		float dxBase = (float)x + 0.5f - actorPosFloat.x;
		float dxBaseSq = dxBase * dxBase;
		if (dxBaseSq > rangeSq) continue;

		for (int z = minZ; z <= maxZ; ++z) {
			float dzBase = (float)z + 0.5f - actorPosFloat.z;
			float dxzSq = dxBaseSq + dzBase * dzBase;
			if (dxzSq > rangeSq) continue;

			for (int y = minY; y <= maxY; ++y) {
				float dyBase = (float)y + 1.f - actorPosFloat.y;
				if (dxzSq + dyBase * dyBase > rangeSq) continue;

				Block* block = WorldUtil::getBlock(BlockPos(x, y, z));
				if (!block || !block->blockLegacy) continue;
				if (!canPlaceCrystalOnBlock(block->blockLegacy->blockId)) continue;

				Block* crystalBlock = WorldUtil::getBlock(BlockPos(x, y + 1, z));
				if (!crystalBlock || !crystalBlock->blockLegacy || crystalBlock->blockLegacy->blockId != 0) continue;

				Block* crystalTop = WorldUtil::getBlock(BlockPos(x, y + 2, z));
				if (!crystalTop || !crystalTop->blockLegacy || crystalTop->blockLegacy->blockId != 0) continue;

				AABB crystalAABB;
				crystalAABB.lower = Vec3<float>((float)x, (float)y, (float)z);
				crystalAABB.upper = Vec3<float>((float)x + 1.f, (float)y + 2.f, (float)z + 1.f);

				if (crystalBlocked(crystalAABB)) continue;

				const Vec3<float> crystalPos((float)x + 0.5f, (float)y + 1.f, (float)z + 0.5f);

				const float damage = computeDamageToCached(crystalPos, actor, targetAabb);
				if (damage <= bestDamage || damage < minPlaceDamage) continue;

				bestDamage = damage;
				bestPos = BlockPos(x, y, z);
			}
		}
	}

	return bestPos;
}

void AutoCrystal::onNormalTick(LocalPlayer* localPlayer) {
	if (!localPlayer || !localPlayer->level) return;

	if (localPlayer->getItemUseDuration() > 0) return;

	if (auto packetMine = ModuleManager::getModule<PacketMine>()) {
		if (packetMine->isEnabled()) {
			if (packetMine->getBreakPos() != BlockPos(0, 0, 0)) pauseTicks = 2;
			if (pauseTicks > 0) { pauseTicks--; return; }
		}
	}

	const int oldSlot = InventoryUtil::getSelectedSlot();

	int attackSlot = PlayerUtil::selectedSlotServerSide;
	if (attackSlot = -1) InventoryUtil::sendMobEquipment(InventoryUtil::getSelectedSlot()); // this should sync?
	
	if (auto ka = ModuleManager::getModule<KillAura>()) {
		if (ka->isEnabled() && ka->getWeaponSlot() != -1)
			attackSlot = ka->getWeaponSlot();
	}

	Actor* target = findTarget(localPlayer);

	if (breakDelay != 0 && breakTimer < breakDelay) {
		breakTimer++;
	}
	else {
		Actor* closestCrystal = nullptr;
		float closestDistance = breakRange;

		const Vec3<float> localPos = localPlayer->getPos();

		for (Actor* actor : localPlayer->level->getRuntimeActorList()) {
			if (!actor) continue;

			auto typeComp = actor->getActorTypeComponent();
			if (!typeComp || typeComp->id != 71) continue;

			if (!TargetUtil::isTargetValid(actor, true, false, breakRange))
				continue;

			const float distance = localPos.dist(actor->getPos());
			if (distance >= closestDistance) continue;

			const Vec3<float> crystalPos = actor->getPos();

			if (minBreakDamage > 0.f) {
				if (!target) continue;
				if (computeDamageTo(crystalPos, target) < minBreakDamage) continue;
			}

			closestDistance = distance;
			closestCrystal = actor;
		}

		if (closestCrystal) {
			InventoryUtil::sendMobEquipment(attackSlot);

			auto runtimeComp = closestCrystal->getRuntimeIDComponent();
			if (runtimeComp) {
				const uint64_t runId = runtimeComp->runtimeId.id;
				auto dimComp = localPlayer->getDimensionTypeComponent();
				const bool checkRuntime = dimComp && dimComp->type == 2;

				std::vector<uint64_t> validRuntimeIds;
				if (checkRuntime) {
					validRuntimeIds.reserve(16);
					for (Actor* actor : localPlayer->level->getRuntimeActorList()) {
						if (!actor) continue;
						auto typeComp = actor->getActorTypeComponent();
						if (!typeComp || typeComp->id != 71) continue;
						auto actorRuntimeComp = actor->getRuntimeIDComponent();
						if (!actorRuntimeComp) continue;
						validRuntimeIds.push_back(actorRuntimeComp->runtimeId.id);
					}
				}

				for (int i = -1; i < breakPackets; i++) {
					const uint64_t attackRuntimeId = runId + i;

					if (checkRuntime) {
						bool found = false;
						for (uint64_t id : validRuntimeIds) {
							if (id == attackRuntimeId) { found = true; break; }
						}
						if (!found) continue;
					}

					closestCrystal->setStatusFlag(ActorFlags::Invisible, true);
					closestCrystal->setStatusFlag(ActorFlags::HiddenWhenInvisible, true);

					auto attackTransac = ItemUseOnActorInventoryTransaction::make_unique();
					attackTransac->actorRuntimeId = attackRuntimeId;
					attackTransac->actionType = ItemUseOnActorInventoryTransaction::ActionType::Attack;
					attackTransac->slot = attackSlot;
					attackTransac->itemInHand = NetworkItemStackDescriptor(InventoryUtil::getItem(attackSlot));
					attackTransac->playerPos = localPos;
					attackTransac->clickPos = Vec3<float>(
						Math::randomFloat(0.5f, 0.5f),
						Math::randomFloat(0.5f, 0.5f),
						Math::randomFloat(0.5f, 0.5f)
					);

					InventoryTransactionPacket packet(std::move(attackTransac));
					localPlayer->level->getPacketSender()->send(&packet);
				}
			}
		}

		breakTimer = 0;
	}

	placementsScratch.clear();

	if ((render || target) && target) {
		if (multiPlace) {
			const std::vector<BlockPos>& res = getBestPlacementPositions(target, placeRange, multiPlaceCount);
			placementsScratch.assign(res.begin(), res.end());
		}
		else {
			BlockPos bestPos = getBestPlacementPos(target, placeRange);
			if (bestPos.y != -999)
				placementsScratch.push_back(bestPos);
		}
	}

	if (render) {
		renderPlacements.clear();
		const size_t n = placementsScratch.size();
		renderPlacements.reserve(n);

		if (target && target->aabbShape) {
			AABB targetAabb = target->aabbShape->aabb;
			targetAabb.lower.x -= 0.1f;
			targetAabb.lower.z -= 0.1f;
			targetAabb.upper.x += 0.1f;
			targetAabb.upper.z += 0.1f;

			for (const BlockPos& bestPos : placementsScratch) {
				PlaceRenderData data;
				data.box.lower = Vec3<float>((float)bestPos.x, (float)bestPos.y, (float)bestPos.z);
				data.box.upper = Vec3<float>((float)bestPos.x + 1.f, (float)bestPos.y + 1.f, (float)bestPos.z + 1.f);
				data.center = Vec3<float>(bestPos.x + 0.5f, bestPos.y + 0.5f, bestPos.z + 0.5f);

				const Vec3<float> crystalPos(bestPos.x + 0.5f, bestPos.y + 1.f, bestPos.z + 0.5f);
				const float exposure = WorldUtil::getSeenPercent(crystalPos, targetAabb);
				data.damage = AutoCrystal::computeDamage(crystalPos, target, exposure);

				renderPlacements.push_back(data);
			}
		}
	}

	const bool delayReady = placeDelay == 0 || placeTimer >= placeDelay;

	if (!delayReady) {
		placeTimer++;
	}
	else {
		const int crystalSlot = InventoryUtil::findItemSlotInHotbar(758);

		if (crystalSlot != -1) {
			if (!placementsScratch.empty()) {
				switchTo(crystalSlot);

				const Vec3<float> localPos = localPlayer->getPos();

				for (BlockPos& bestPos : placementsScratch) {
					Block* block = WorldUtil::getBlock(bestPos);
					if (!block) continue;

					auto runtimeId = block->getRuntimeId();
					if (!runtimeId) continue;

					auto itemUseInvTransac = ItemUseInventoryTransaction::make_unique();
					itemUseInvTransac->actionType = ItemUseInventoryTransaction::ActionType::Place;
					itemUseInvTransac->blockPos = bestPos;
					itemUseInvTransac->targetBlockRuntimeId = *runtimeId;
					itemUseInvTransac->face = 0;
					itemUseInvTransac->slot = crystalSlot;
					itemUseInvTransac->itemInHand = NetworkItemStackDescriptor(InventoryUtil::getItem(crystalSlot));
					itemUseInvTransac->playerPos = localPos;
					itemUseInvTransac->clickPos = Vec3<float>(0.5f, 0.5f, 0.5f);

					InventoryTransactionPacket packet(std::move(itemUseInvTransac));
					localPlayer->level->getPacketSender()->send(&packet);
				}
			}

			placeTimer = 0;
		}
	}

	switchBack(oldSlot);
}

void AutoCrystal::drawPlacementBox(const Vec3<float>& center, float scale) {
	const float half = 0.5f * scale;

	AABB box;
	box.lower = Vec3<float>(center.x - half, center.y - half, center.z - half);
	box.upper = Vec3<float>(center.x + half, center.y + half, center.z + half);

	if (renderFill && renderOutline)
		MCR::drawBox3dFilled(box, fillColor, lineColor);
	else if (renderFill)
		MCR::drawBox3dFilled(box, fillColor, UIColor(lineColor.r, lineColor.g, lineColor.b, 0));
	else if (renderOutline)
		MCR::drawBox3dFilled(box, UIColor(fillColor.r, fillColor.g, fillColor.b, 0), lineColor);
}

void AutoCrystal::onLevelRender() {
	if (!render) return;

	if (!renderSmooth) {
		if (!renderPlacements.empty())
			drawPlacementBox(renderPlacements[0].center, 1.f);
		return;
	}

	const float t = std::min(1.f, MCR::deltaTime * renderSmoothSpeed);
	const bool active = !renderPlacements.empty();

	if (active) {
		smoothBoxes.clear();
		smoothBoxes.push_back(renderPlacements[0]);

		if (smoothCenters.empty())
			smoothCenters.push_back(smoothBoxes[0].center);
	}

	const float target = active ? 1.f : 0.f;
	renderGrow += (target - renderGrow) * t;

	if (!active && renderGrow <= 0.001f) {
		renderGrow = 0.f;
		smoothBoxes.clear();
		smoothCenters.clear();
	}

	if (!smoothBoxes.empty() && !smoothCenters.empty()) {
		smoothCenters[0] = smoothCenters[0].lerp(smoothBoxes[0].center, t, t, t);
		drawPlacementBox(smoothCenters[0], renderGrow);
	}
}

void AutoCrystal::onMCRender(MinecraftUIRenderContext* renderCtx) {
	if (!render || !renderDamage) return;

	struct Label {
		Vec3<float> center;
		float damage;
	};

	std::vector<Label> labels;

	if (renderSmooth) {
		if (renderGrow > 0.5f && !smoothBoxes.empty() && !smoothCenters.empty())
			labels.push_back({ smoothCenters[0], smoothBoxes[0].damage });
	}
	else {
		if (!renderPlacements.empty())
			labels.push_back({ renderPlacements[0].center, renderPlacements[0].damage });
	}

	if (labels.empty()) return;

	std::vector<Vec4<float>> usedRects;
	usedRects.reserve(labels.size());

	auto overlaps = [](const Vec4<float>& a, const Vec4<float>& b) {
		return a.x < b.z && a.z > b.x && a.y < b.w && a.w > b.y;
		};

	for (auto& label : labels) {
		Vec2<float> screenPos;
		if (!MCR::worldToScreen(label.center, screenPos)) continue;

		char buffer[32];
		snprintf(buffer, sizeof(buffer), "%.1f", label.damage);

		const float textWidth = MCR::getTextWidth(buffer, renderScale);
		const float textHeight = MCR::getTextHeight(renderScale);

		float x = std::round(screenPos.x - textWidth / 2.f);
		float y = std::round(screenPos.y - textHeight / 2.f);

		Vec4<float> rect(x, y, x + textWidth, y + textHeight);

		for (int attempts = 0; attempts < 24; ++attempts) {
			bool collision = false;

			for (auto& used : usedRects) {
				if (overlaps(rect, used)) {
					collision = true;
					break;
				}
			}

			if (!collision) break;

			y -= textHeight + 1.f;
			rect = Vec4<float>(x, y, x + textWidth, y + textHeight);
		}

		usedRects.push_back(rect);
		MCR::drawText(Vec2<float>(x, y), buffer, damageColor, renderScale);
	}
}
