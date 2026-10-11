#include "Scaffold.h"

Scaffold::Scaffold() : Module("Scaffold", "Places blocks beneath you", Category::WORLD) {
	registerSetting(new SliderSetting<int>("Extend", "NULL", &extend, extend, 1, 5));
	registerSetting(new SliderSetting<float>("Speed", "Speed of placing blocks", &placeSpeed, placeSpeed, 0.f, 20.f));
	registerSetting(new EnumSetting("Sprint", "NULL", {"Off", "Manual", "Always"}, &sprintType, sprintType));
	registerSetting(new BoolSetting("Lock Y", "Locks the y pos", &sameY, sameY));
	registerSetting(new BoolSetting("Clutch", "Tries to save you from falling to your death, this will waste blocks, but hey atleast ur safe!", &clutch, clutch));
	registerSetting(new SliderSetting<int>("Timer", "20 is default, the more value the faster you will be", &tps, tps, 20, 50));
}

void Scaffold::onEnable() {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (!localPlayer) {
		return;
	}
	yPos = (int)localPlayer->getPos().floor().sub(0, 1, 0).y;
}

void Scaffold::onDisable() {
	Minecraft* mc = Game::clientInstance->minecraft;
	if (!mc) {
		return;
	}
	*mc->minecraftTimer = 20;
}

bool Scaffold::buildBlock(BlockPos pos) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (iPlaceSpeed >= 20 - placeSpeed) {
		if (PlayerUtil::tryPlaceBlock(pos)) {
			localPlayer->swing();
			return true;
		}
		else {
			Vec3<float> realPos = localPlayer->getPos().floor().sub(0, 1, 0);
			if (((WorldUtil::canBuildOn(realPos.CastTo<int>().sub(0, 1, 0)) && !sameY) || (sameY && realPos.y <= yPos - 1)) && clutch) {
				static constexpr int proximity = 5;
				std::vector<BlockPos> checkList;
				BlockPos nearestBlock;
				if (checkList.empty()) {
					for (int x = -proximity; x <= proximity; x++) {
						for (int y = -proximity; y <= proximity; y++) {
							for (int z = -proximity; z <= proximity; z++) {
								checkList.push_back(BlockPos(pos.x + x, pos.y + y, pos.z + z));
							}
						}
					}
				}
				std::sort(checkList.begin(), checkList.end(), [realPos](BlockPos start, BlockPos end) {
					BlockPos belowPos = realPos.CastTo<int>().sub(0, 2, 0);
					return belowPos.dist(start) < belowPos.dist(end); }
				);
				for (BlockPos current : checkList) {
					if (!WorldUtil::canBuildOn(current)) {
						nearestBlock = current;
						break;
					}
				}
				std::vector<BlockPos> offset;
				if (offset.empty()) {
					for (int x = -1; x <= 1; x++) {
						for (int y = -1; y <= 1; y++) {
							for (int z = -1; z <= 1; z++) {
								offset.push_back(BlockPos(nearestBlock.x + x, nearestBlock.y + y, nearestBlock.z + z));
							}
						}
					}
				}
				std::sort(offset.begin(), offset.end(), [realPos](BlockPos start, BlockPos end) {
					BlockPos belowPos = realPos.CastTo<int>().sub(0, 2, 0);
					return belowPos.dist(start) < belowPos.dist(end); }
				);
				for (BlockPos off : offset) {
					if (PlayerUtil::tryPlaceBlock(off) && PlayerUtil::canPlaceBlock(off)) {
						PlayerUtil::tryPlaceBlock(off.sub(0, 1, 0)); // extra checkz
						localPlayer->swing();
						break;
					}
				}
			}
		}
		iPlaceSpeed = 0;
	}
	else {
		iPlaceSpeed++;
	}
	return false;
}

void Scaffold::onNormalTick(LocalPlayer* localPlayer) {
	Minecraft* mc = Game::clientInstance->minecraft;
	if (!mc) {
		return;
	}

	if (sprintType == 0) {
		localPlayer->setSprinting(false);
	}
	else if (sprintType == 2) {
		localPlayer->setSprinting(true);
	}

	Vec3<float> actorPos = localPlayer->getPos().floor().sub(0, 1, 0);
	Vec3<int> velocity = Vec3<int>(MoveUtil::getVelocity().x * 10, 0, MoveUtil::getVelocity().z * 10);

	if (!sameY) {
		yPos = actorPos.y;
	}

	BlockPos blockBelow = BlockPos(actorPos.x, yPos - 1, actorPos.z);
	BlockPos blockPos = blockBelow.add(velocity);

	Scaffold::buildBlock(blockBelow);
	if (MoveUtil::isMoving()) {
		*mc->minecraftTimer = tps;
		Scaffold::buildBlock(blockPos);
		for (int i = 1; i < extend; i++) {
			BlockPos pos = blockBelow;
			pos.x += velocity.x * i;
			pos.z += velocity.z * i;
			Scaffold::buildBlock(pos);
		}
	}
	else {
		*mc->minecraftTimer = 20;
	}
}