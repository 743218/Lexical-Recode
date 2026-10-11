#include "AutoMapartCommand.h"
#include "../../Client/Client.h"
#include "../../Utils/FileUtil.h"

AutoMapartCommand::AutoMapartCommand() : CommandBase("automapart", "extra stuff with auto mapart", { "mapart" }) {

}

bool AutoMapartCommand::execute(const std::vector<std::string>& args) {
	static AutoMapart* mapart = ModuleManager::getModule<AutoMapart>();
	if (args.size() == 0) {
		Game::DisplayClientMessage("%sAutoMapart", MCTF::GRAY);
		Game::DisplayClientMessage("%sreset %s|%s Resets the start pos", MCTF::GRAY, MCTF::RESET, MCTF::GRAY);
		Game::DisplayClientMessage("%sload %s|%s Loads a new mapart", MCTF::GRAY, MCTF::RESET, MCTF::GRAY);
	}
	else {
		if (args[1] == "reset") {
			mapart->startPos = BlockPos(0, -32000, 0);
			mapart->mapArt.clear();
			mapart->image.clear();
			Game::DisplayClientMessage("AutoMapart: %sSuccessfully cleared AutoMapart data.", MCTF::GREEN);
		}
		else if (args[1] == "load") {
			if (args.size() < 2) Game::DisplayClientMessage("AutoMapart: %sPlease enter an image name", MCTF::GRAY);
			else {
				std::string filePath = FileUtil::getClientPath() + "Maparts\\" + args[2] + ".png";
				if (FileUtil::doesFilePathExist(filePath)) {
					mapart->mapartName = args[2];
					mapart->image.clear();
					mapart->mapArt.clear();
					Game::DisplayClientMessage("AutoMapart: %sSuccessfully loaded image %s%s%s.", MCTF::GRAY, MCTF::WHITE, (args[2] + ".png").c_str(), MCTF::GRAY);
				}
				else Game::DisplayClientMessage("AutoMapart: %sImage with the name of %s%s%s does not exist", MCTF::GRAY, MCTF::WHITE, (args[2] + ".png").c_str(), MCTF::GRAY);
			}
		}
	}
	return true;
}