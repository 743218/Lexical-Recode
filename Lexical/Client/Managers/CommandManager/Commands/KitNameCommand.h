#pragma once
#include "CommandBase.h"

class KitNameCommand : public CommandBase {
public:
	KitNameCommand() : CommandBase("kitname", "Edits AutoKit kitname for nether and overworld", "<nether/overworld> <name>", { "kn", "autokit" }) {

	}
public:
	virtual bool execute(const std::vector<std::string>& args);
};