#pragma once
#include "../../ModuleBase/Module.h"
#include <queue>
#include <vector>
#include <string>

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
	std::unordered_map<uint32_t, int> targetValidFrames;
	static constexpr int REQUIRED_VALID_FRAMES = 200;

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