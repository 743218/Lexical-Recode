#pragma once
#include "../SDK/Render/BaseActorRenderContext.h"
#include "../SDK/Render/MinecraftUIRenderContext.h"
#include "../SDK/Render/ScreenContext.h"
#include "../SDK/Render/Tessellator.h"
#include "../SDK/Render/MeshHelpers.h"

#include "../Utils/Maths.h"
#include "../Utils/ColorUtil.h"

namespace MCR {

	struct Render3dData {
		Vec3<float> pos;
		UIColor color;
		Render3dData(const Vec3<float>& _pos, const UIColor& _color) {
			pos = _pos;
			color = _color;
		}
	};

	struct BoxRotation {
		Vec3<float> axis;
		float angle;
		BoxRotation(const Vec3<float>& _axis, float _angle) {
			axis = _axis;
			angle = _angle;
		}
	};
	extern BaseActorRenderContext baseActorRenderCtx;
	extern MinecraftUIRenderContext* renderCtx;
	extern ScreenContext* screenCtx;
	extern ShaderColor* currentColor;
	extern Tessellator* tessellator;
	extern mce::MaterialPtr* uiMaterial;
	extern mce::MaterialPtr* blendMaterial;
	extern Font* mcFont;
	extern Vec3<float> origin;
	extern float deltaTime;
	extern std::vector<Render3dData> quad3dRenderList;
	extern std::vector<Render3dData> line3dRenderList;

	void onRenderScreen(MinecraftUIRenderContext* ctx);
	void onRenderWorld(LevelRenderer* levelRenderer, ScreenContext* screenContext);
	void setColor(const mce::Color& color);
	void setColor(const UIColor& color);
	void drawText(const Vec2<float>& textPos, const std::string& textStr, const UIColor& color, float textSize = 1.f);
	float getTextWidth(const std::string& textStr, float textSize = 1.f);
	float getTextHeight(float textSize = 1.f);
	void drawQuad(const Vec2<float>& p1, const Vec2<float>& p2, const Vec2<float>& p3, const Vec2<float>& p4, const UIColor& color);
	void fillRectangle(const Vec4<float>& rect, const UIColor& color);
	void drawItem(const Vec2<float>& pos, ItemStack* itemStack, float scale, bool showDurabilityBar);
	void drawBox3dFilled(const AABB& aabb, const UIColor& color, const UIColor& lineColor, float scale = 1.0f, BoxRotation rotation = BoxRotation(Vec3<float>(1.f, 1.f, 1.f), 0.f));
	void drawTextInWorld(const Vec3<float>& pos, const std::string& textStr, const UIColor& color, float textSize = 1.f);
	bool worldToScreen(const Vec3<float>& pos, Vec2<float>& out);
}