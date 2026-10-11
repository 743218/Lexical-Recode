#pragma once
#include "../../ModuleBase/Module.h"

class HUD : public Module {
private:
	bool showPosition = true;
	bool showDirection = false;
	bool showArmor = true;
	bool showEffects = true;
	bool watermark = true;
	bool ping = true;
	bool fps = false;
	int offset = 15;
	int opacity = 125;
	int spacing = -1;

	struct CachedEffect {
		std::string name;
		std::string duration;
		UIColor color;
	};

	struct CachedData {
		bool valid = false;
		Vec2<float> windowSize;
		Vec2<float> windowSizeScaled;
		float posX = 0.f;
		float posY = 0.f;
		float posZ = 0.f;
		int dimensionId = 0;
		float rotation = 0.f;
		int ping = -1;
		std::vector<CachedEffect> effects;
		std::vector<ItemStack*> armorItems;
	} cachedData;

	inline std::string getEffectTimeLeftStr(MobEffectInstance* mEffectInstance) {
		uint32_t timeLeft = mEffectInstance->mDuration;
		uint32_t timeReal = (uint32_t)(timeLeft / 20);
		std::string m = std::to_string(timeReal / 60);
		std::string s;
		if (timeReal % 60 < 10)
			s += "0";
		s += std::to_string(timeReal % 60);
		return m + ":" + s;
	}
public:
	HUD();
	~HUD();

	void onImGuiRender(ImDrawList* drawList) override;
	void onMCRender(MinecraftUIRenderContext* ctx) override;
};