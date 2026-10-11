#include "ClientNameCommand.h"
#include "../../Client/Client.h"

ClientNameCommand::ClientNameCommand() : CommandBase("clientname", "changes the clients name", "<name>", { "clientname", "setname", "name" }) {

}

bool ClientNameCommand::execute(const std::vector<std::string>& args) {
	if (args.size() < 2) return false;
	std::string name = args[1];
	if (args.size() >= 2) {
		for (int i = 2; i < args.size(); i++) {
			name += ' ' + args[i];
		}
	}
	Client::setClientName(name);
	Client::DisplayClientMessage("%sSuccessfully changed client name to %s%s", MCTF::GREEN, MCTF::GOLD, name.c_str());
	return true;
}