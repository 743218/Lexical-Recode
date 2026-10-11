#include "DevTools.h"
#include "../../Client/Client.h"

DevTools::DevTools() : CommandBase("devtools", "commands for devs", "<devcommand> <optional argument>", { "devtool" }) {

}

bool DevTools::execute(const std::vector<std::string>& args) {
	LocalPlayer& localPlayer = *Game::getLocalPlayer();
	if (args.size() == 0) {
		Game::DisplayClientMessage("%sDevTools </>", MCTF::GRAY);
		Game::DisplayClientMessage("%sblockinfo %s|%s shows info about the block you are looking at", MCTF::GRAY, MCTF::RESET, MCTF::GRAY);
		Game::DisplayClientMessage("%siteminfo %s|%s shows info about the item you are holding", MCTF::GRAY, MCTF::RESET, MCTF::GRAY);
	}
	else {
		Game::DisplayClientMessage("%sDevTools </>", MCTF::GRAY);
		if (args[1] == "iteminfo") {
			if (InventoryUtil::getHeldItemId() == 0) Game::DisplayClientMessage("%sYou are not holding anything", MCTF::GRAY);
			else {
				ItemStack& stack = *InventoryUtil::getItem(InventoryUtil::getSelectedSlot());
				Game::DisplayClientMessage("%sTexture Name: %s%s", MCTF::GRAY, MCTF::WHITE, stack.item.get()->texture_name);
				Game::DisplayClientMessage("%sItem ID: %s%i", MCTF::GRAY, MCTF::WHITE, InventoryUtil::getHeldItemId());
				Game::DisplayClientMessage("%sAttack Damage: %s%f", MCTF::GRAY, MCTF::WHITE, localPlayer.calculateAttackDamage(&localPlayer));
				Game::DisplayClientMessage("%sDurability: %s%i%s/%s%i", MCTF::GRAY, MCTF::WHITE, stack.item.get()->getDamageValue(stack.mUserData), MCTF::GRAY, MCTF::WHITE, stack.item.get()->getMaxDamage());
			}
		}
		else if (args[1] == "blockinfo") {
			HitResult& hitResult = *localPlayer.level->getHitResult();
			const BlockPos& looking = hitResult.blockPos;
			Block& block = *WorldUtil::getBlock(looking);
			Game::DisplayClientMessage("%sBlock Pos: %s[%s%i%s,%s%i%s,%s%i%s]", MCTF::GRAY, MCTF::WHITE, MCTF::GRAY, looking.x, MCTF::WHITE, MCTF::GRAY, looking.y, MCTF::WHITE, MCTF::GRAY, looking.z, MCTF::WHITE);
			Game::DisplayClientMessage("%sBlock ID: %s%i", MCTF::GRAY, MCTF::WHITE, block.blockLegacy->blockId);
			Game::DisplayClientMessage("%sFace: %s%i", MCTF::GRAY, MCTF::WHITE, hitResult.selectedFace);
			Game::DisplayClientMessage("%sBlock Runtime ID: %s%lld", MCTF::GRAY, MCTF::WHITE, block.getRuntimeId());
		}
	}
	return true;
}