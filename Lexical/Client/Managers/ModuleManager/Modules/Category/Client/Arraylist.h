#pragma once
#include "../../ModuleBase/Module.h"

class Arraylist : public Module {
public:
	bool bottom = true;
private:
	bool showModes = true;
	UIColor color = UIColor(115, 135, 255);
	bool rainbow = false;
	int modeColorEnum = 0;
	bool outline = false;
	int offset = 15;
	int opacity = 125;
	int spacing = -1;
	static bool sortByLength(Module* lhs, Module* rhs);
public:
	Arraylist();

	void onImGuiRender(ImDrawList* drawList) override;
};