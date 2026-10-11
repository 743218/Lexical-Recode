#pragma once
#include "CommandBase.h"

class AutoMapartCommand : public CommandBase {
public:
	AutoMapartCommand();
	virtual bool execute(const std::vector<std::string>& args);
};