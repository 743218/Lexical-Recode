#include "Arraylist.h"
#include "../../../../../../Utils/TimerUtil.h"
#include "../../../../../../Renderer/ImGuiRenderer.h"
#include "../../../ModuleManager.h"

Arraylist::Arraylist() : Module("Arraylist", "Display module enabled", Category::CLIENT) {
	registerSetting(new BoolSetting("Bottom", "Fat", &bottom, true));
	registerSetting(new BoolSetting("Show Modes", "Show Modes", &showModes, true));
	registerSetting(new BoolSetting("Outline", "NULL", &outline, false));
	registerSetting(new ColorSetting("MainColor", "NULL", &color, color, false));
	registerSetting(new BoolSetting("Rainbow", "Gay", &rainbow, false));
	registerSetting(new EnumSetting("ModeColor", "NULL", { "White", "Gray" }, &modeColorEnum, 0));

	registerSetting(new SliderSetting<int>("Opacity", "NULL", &opacity, 130, 0, 255));
	registerSetting(new SliderSetting<int>("Spacing", "NULL", &spacing, -2, -3, 3));
	registerSetting(new SliderSetting<int>("Offset", "NULL", &offset, 10, 0, 30));
}

bool Arraylist::sortByLength(Module* lhs, Module* rhs) {
	float textSize = 1.f;

	float textWidth1 = ImGuiRenderer::getTextWidth(lhs->getModuleName(), textSize);
	float textWidth2 = ImGuiRenderer::getTextWidth(rhs->getModuleName(), textSize);

	static Arraylist* arralistMod = ModuleManager::getModule<Arraylist>();
	if (arralistMod->showModes) {
		if (lhs->getModeText() != "NULL")
			textWidth1 += ImGuiRenderer::getTextWidth(" " + lhs->getModeText(), textSize);

		if (rhs->getModeText() != "NULL")
			textWidth2 += ImGuiRenderer::getTextWidth(" " + rhs->getModeText(), textSize);
	}

	return (textWidth1 > textWidth2);
}

void Arraylist::onImGuiRender(ImDrawList* drawList) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (localPlayer == nullptr)
		return;

	Vec2<float> windowSize = Game::clientInstance->guiData->windowSizeReal;	//ImGuiRenderer::getWindowSize();
	Vec2<float> windowSizeScaled = Game::clientInstance->guiData->windowSizeScaled;

	float textSize = 1.f;
	float textPaddingX = std::max(1.f, ((float)spacing + 3.f)) * textSize;
	float textPaddingY = (float)spacing * textSize;
	float textHeight = ImGuiRenderer::getTextHeight("", textSize);

	float lineWidth = 1.5f * textSize;
	
	Vec2<float> Pos = Vec2<float>(windowSize.x - (float)offset, (float)offset);
	if (bottom)
		Pos.y = windowSize.y - (textHeight + textPaddingY * 2.f) - (float)offset;

	UIColor mainColor = UIColor(color.r, color.g, color.b);//UIColor(115, 135, 255);
	UIColor modeColor = UIColor(255, 255, 255);
	if (modeColorEnum == 1)
		modeColor = UIColor(175, 175, 175);

	static std::vector<Module*> moduleList = ModuleManager::moduleList;
	std::sort(moduleList.begin(), moduleList.end(), sortByLength);
	int index = 0;
	Vec4<float> lastRect;
	for (auto& mod : moduleList) {
		std::string moduleName = mod->getModuleName();
		std::string modeText = mod->getModeText();
		bool shouldRender = (mod->isEnabled() && mod->isVisible());

		if (!showModes)
			modeText = "NULL";

		if (rainbow)
			mainColor = ColorUtil::getRainbowColor(7.f, 1.f, 1.f, (int)(index * 200 * (bottom ? -1.f : 1.f)));

		mod->arraylistAnim = Math::lerp(mod->arraylistAnim, shouldRender ? 1.05f : -0.1f, ImGuiRenderer::deltaTime * 15.f);

		if (mod->arraylistAnim > 1.f)
			mod->arraylistAnim = 1.f;

		if (mod->arraylistAnim < 0.f)
			mod->arraylistAnim = 0.f;

		if (mod->arraylistAnim > 0.f) {
			float fullTextWidth = ImGuiRenderer::getTextWidth(moduleName, textSize);
			if (modeText != "NULL")
				fullTextWidth += ImGuiRenderer::getTextWidth(" " + modeText, textSize);

			Vec4<float> rectPos = Vec4<float>(Pos.x - textPaddingX - fullTextWidth - textPaddingX,
											  Pos.y,
											  Pos.x,
											  Pos.y + textPaddingY + textHeight + textPaddingY);

			float animRect = (windowSize.x + 25.f - rectPos.x) * (1.f - mod->arraylistAnim);
			rectPos.x += animRect;
			rectPos.z += animRect;

			Vec2<float> textPos = Vec2<float>(rectPos.x + textPaddingX, rectPos.y + textPaddingY);

			ImGuiRenderer::fillRectangle(rectPos, UIColor(0, 0, 0, opacity));
			if (outline) {
				ImGuiRenderer::fillRectangle(Vec4<float>(rectPos.x - lineWidth, rectPos.y, rectPos.x, rectPos.w), mainColor);
				ImGuiRenderer::fillRectangle(Vec4<float>(rectPos.z, rectPos.y, rectPos.z + lineWidth, rectPos.w), mainColor);
				if (index == 0) {
					if (bottom)
						ImGuiRenderer::fillRectangle(Vec4<float>(rectPos.x - lineWidth, rectPos.w, rectPos.z + lineWidth, rectPos.w + lineWidth), mainColor);
					else
						ImGuiRenderer::fillRectangle(Vec4<float>(rectPos.x - lineWidth, rectPos.y - lineWidth, rectPos.z + lineWidth, rectPos.y), mainColor);
				}
				else {
					if (bottom)
						ImGuiRenderer::fillRectangle(Vec4<float>(lastRect.x - lineWidth, rectPos.w - lineWidth, rectPos.x, rectPos.w), mainColor);
					else
						ImGuiRenderer::fillRectangle(Vec4<float>(lastRect.x - lineWidth, rectPos.y, rectPos.x, rectPos.y + lineWidth), mainColor);
				}
			}
			ImGuiRenderer::drawText(textPos, moduleName, mainColor, textSize, true);
			if (modeText != "NULL") {
				textPos.x += ImGuiRenderer::getTextWidth(moduleName + " ", textSize);
				ImGuiRenderer::drawText(textPos, modeText, modeColor, textSize, true);
			}
			lastRect = rectPos;
		}

		float yAnim = mod->arraylistAnim * 1.25f;
		if (yAnim > 1.f)
			yAnim = 1.f;

		Pos.y += (textHeight + textPaddingY * 2.f) * yAnim * (bottom ? -1.f : 1.f);

		if (shouldRender)
			index++;
	}
	if (outline) {
		if (bottom)
			ImGuiRenderer::fillRectangle(Vec4<float>(lastRect.x - lineWidth, lastRect.y - lineWidth, lastRect.z + lineWidth, lastRect.y), mainColor);
		else
			ImGuiRenderer::fillRectangle(Vec4<float>(lastRect.x - lineWidth, lastRect.w, lastRect.z + lineWidth, lastRect.w + lineWidth), mainColor);
	}
}