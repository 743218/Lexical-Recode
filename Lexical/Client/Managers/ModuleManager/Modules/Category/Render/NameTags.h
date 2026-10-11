#pragma once
#include "../../ModuleBase/Module.h"
#include <queue>

struct NameTagRenderData {
	Vec2<float> screenPos;
	std::string text;
	float scale;
};

struct NameTagsTransform {
	std::vector<NameTagRenderData> renderList;
};

class NameTags : public Module {
private:
	int renderMode = 2;
	int opacity = 125;
	bool displayArmor = true;
	bool self = true;
	bool mobTags = false;
	bool itemTags = false;
	UIColor rectColor = UIColor(255, 255, 255, 255);
	std::queue<NameTagsTransform> transformList;
public:
	NameTags();
	void Render();
	void RenderD2D();
};

//Old NameTags
/*
#pragma once
#include "../../ModuleBase/Module.h"

class NameTags : public Module {
private:
	int renderMode = 2;
	int opacity = 125;
	bool self = true;
	bool mobTags = false;
	bool itemTags = false;

	bool invalidChar(char c);
	std::string sanitize(std::string text);
	std::string getEntityNameTags(Actor* entity);
public:
	NameTags();
	void Render();
};
*/