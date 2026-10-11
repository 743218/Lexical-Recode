#include "PlayerUtil.h"
#include "../../Client/Managers/ModuleManager/ModuleManager.h"
#include "WorldUtil.h"

uint32_t PlayerUtil::selectedSlotServerSide = 0;

bool PlayerUtil::canPlaceBlock(const BlockPos& blockPos, bool airPlace) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	BlockSource* region = Game::clientInstance->getRegion();
	Block* block = region->getBlock(blockPos);
	BlockLegacy* blockLegacy = block->blockLegacy;

	if (blockLegacy->canBeBuiltOver(region, blockPos)) {

		if (airPlace) {
			return true;
		}

		BlockPos blockToPlace = blockPos;

		static BlockPos checklist[6] = { BlockPos(0, -1, 0),
										 BlockPos(0, 1, 0),
										 BlockPos(0, 0, -1),
										 BlockPos(0, 0, 1),
										 BlockPos(-1, 0, 0),
										 BlockPos(1, 0, 0) };

		for (const BlockPos& current : checklist) {
			BlockPos calc = blockToPlace.sub(current);
			if (!region->getBlock(calc)->blockLegacy->canBeBuiltOver(region, calc) && !WorldUtil::isContainer(calc)) {
				return true;
			}
		}
	}
	return false;
}

bool PlayerUtil::tryPlaceBlock(const BlockPos& blockPos, bool airPlace, bool packetPlace) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	BlockSource* region = Game::clientInstance->getRegion();
	Block* block = region->getBlock(blockPos);
	BlockLegacy* blockLegacy = block->blockLegacy;

	if (blockLegacy->canBeBuiltOver(region, blockPos)) {

		BlockPos blockToPlace = blockPos;

		static BlockPos checklist[6] = { BlockPos(0, -1, 0),
										 BlockPos(0, 1, 0),
										 BlockPos(0, 0, -1),
										 BlockPos(0, 0, 1),
										 BlockPos(-1, 0, 0),
										 BlockPos(1, 0, 0) };

		bool foundCandidate = false;
		uint8_t i = 0;

		for (const BlockPos& current : checklist) {
			BlockPos calc = blockToPlace.sub(current);
			if (!region->getBlock(calc)->blockLegacy->canBeBuiltOver(region, calc) && !WorldUtil::isContainer(calc) && region->getBlock(calc)->blockLegacy->blockId != 143) {
				foundCandidate = true;
				blockToPlace = calc;
				break;
			}
			i++;
		}

		if (airPlace && !foundCandidate) {
			foundCandidate = true;
			blockToPlace = blockPos;
			i = 0;
		}

		if (foundCandidate) {
			if (!packetPlace) {
				localPlayer->gameMode->buildBlock(blockToPlace, i, false);
			}
			return true;
		}
	}
	return false;
}

bool PlayerUtil::tryPlaceBlockSupport(const BlockPos& pos, int proximity, bool airplace, bool packetPlace) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (PlayerUtil::tryPlaceBlock(pos)) return true;
	std::vector<BlockPos> checkList;
	BlockPos nearestBlock;
	if (checkList.empty()) {
		for (int x = -proximity; x <= proximity; x++) {
			for (int y = -proximity; y <= proximity; y++) {
				for (int z = -proximity; z <= proximity; z++) {
					checkList.emplace_back(pos.x + x, pos.y + y, pos.z + z);
				}
			}
		}
	}

	std::sort(checkList.begin(), checkList.end(), [&](const BlockPos& start, const BlockPos& end) {
		return pos.dist(start) < pos.dist(end);
		});

	std::vector<BlockPos>::iterator it = std::find_if(checkList.begin(), checkList.end(), [](const BlockPos& current) {
		return !WorldUtil::canBuildOn(current);
		});

	if (it != checkList.end()) nearestBlock = *it;
	else return false;

	std::vector<BlockPos> offset;

	for (int x = -1; x <= 1; x++) {
		for (int y = -1; y <= 1; y++) {
			for (int z = -1; z <= 1; z++) {
				offset.emplace_back(nearestBlock.x + x, nearestBlock.y + y, nearestBlock.z + z);
			}
		}
	}

	std::sort(offset.begin(), offset.end(), [&](const BlockPos& start, const BlockPos& end) {
		return pos.dist(start) < pos.dist(end);
	});
	for (const BlockPos& off : offset) {
		if (PlayerUtil::canPlaceBlock(off)) {
			if (PlayerUtil::tryPlaceBlock(off)) {
				localPlayer->swing();
				break;
			}
		}
	}
	return true;
}

bool MoveUtil::isMoving() {
	if (Game::isKeyDown('W') || Game::isKeyDown('A') || Game::isKeyDown('S') || Game::isKeyDown('D')) return true;
	return false;
}

Vec2<float> MoveUtil::getMotion(const float& speed) {
	float playerYaw = Game::getLocalPlayer()->rotation->presentRot.y;
	bool w = Game::isKeyDown('W');
	bool a = Game::isKeyDown('A');
	bool s = Game::isKeyDown('S');
	bool d = Game::isKeyDown('D');
	if (w) {
		if (!a && !d) playerYaw += 90.0f;
		if (a) playerYaw += 45.0f;
		else if (d) playerYaw += 135.0f;
	}
	else if (s) {
		if (!a && !d) playerYaw -= 90.0f;
		if (a) playerYaw -= 45.0f;
		else if (d) playerYaw -= 135.0f;
	}
	else {
		if (!a && d) playerYaw += 180.0f;
	}
	float calcYaw = playerYaw * 0.01745329251f; //0.01745329251f = PI / 180
	return Vec2<float>(cos(calcYaw) * speed, sin(calcYaw) * speed);
}

float MoveUtil::getAdjustedYaw() {
	float yaw = Game::getLocalPlayer()->rotation->presentRot.y;
	if (Game::isKeyDown('W') && Game::isKeyDown('S')) return yaw;
	else if (Game::isKeyDown('W') && Game::isKeyDown('D') && !Game::isKeyDown('A')) yaw += 45.f;
	else if (Game::isKeyDown('W') && Game::isKeyDown('A') && !Game::isKeyDown('D')) yaw -= 45.f;
	else if (Game::isKeyDown('S') && Game::isKeyDown('D') && !Game::isKeyDown('A')) yaw += 135.f;
	else if (Game::isKeyDown('S') && Game::isKeyDown('A') && !Game::isKeyDown('D')) yaw -= 135.f;
	else if (Game::isKeyDown('S')) yaw += 180.f;
	else if (Game::isKeyDown('D') && !Game::isKeyDown('A')) yaw += 90.f;
	else if (Game::isKeyDown('A') && !Game::isKeyDown('D')) yaw -= 90.f;
	if (yaw >= 180) yaw -= 360.f;
	return yaw;
}

void MoveUtil::setSpeed(const float& speed) {
	Vec2<float> motion = getMotion(speed);
	if (!isMoving()) {
		motion.x = 0.f;
		motion.y = 0.f;
	}
	Game::getLocalPlayer()->stateVector->velocity.x = motion.x;
	Game::getLocalPlayer()->stateVector->velocity.z = motion.y;
}

bool PlayerUtil::isEating(LocalPlayer* localPlayer) {
	return localPlayer != nullptr && localPlayer->getItemUseDuration() > 0;
}

bool PlayerUtil::isPacketMining() {
	auto packetMine = ModuleManager::getModule<PacketMine>();
	return packetMine != nullptr && packetMine->isEnabled() && packetMine->getBreakPos() != BlockPos(0, 0, 0);
}