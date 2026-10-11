#include "KitNameCommand.h"
#include "../../ModuleManager/ModuleManager.h"

bool KitNameCommand::execute(const std::vector<std::string>& args) {
	if (args.size() < 2) return false;
	static AutoKit* autoKit = ModuleManager::getModule<AutoKit>();
	if (args[1] == "ovw" || args[1] == "overworld" || args[1] == "1") {
		autoKit->firstKit = args[2];
		Game::DisplayClientMessage("[%sAutoKit%s]%sSuccessfully set the overworld kit name to %s%s", MCTF::AQUA, MCTF::WHITE, MCTF::GREEN, MCTF::GOLD, args[2].c_str());
	}
	else if (args[1] == "nether" || args[1] == "2") {
		autoKit->secondKit = args[2];
		Game::DisplayClientMessage("[%sAutoKit%s]%sSuccessfully set the nether kit name to %s%s", MCTF::AQUA, MCTF::WHITE, MCTF::GREEN, MCTF::GOLD, args[2].c_str());
	}
	return true;
}