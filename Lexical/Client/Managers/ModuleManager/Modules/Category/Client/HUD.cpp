#include "HUD.h"
#include "../../../../../../Renderer/ImGuiRenderer.h"
#include "../../../ModuleManager.h"
#include "../../Client/Client.h"

HUD::HUD() : Module("HUD", "Displays HUD Elements", Category::CLIENT) {
	registerSetting(new BoolSetting("Frames", "NULL", &fps, true));
	registerSetting(new BoolSetting("Position", "NULL", &showPosition, true));
	registerSetting(new BoolSetting("Direction", "NULL", &showDirection, false));
	registerSetting(new BoolSetting("Armor", "NULL", &showArmor, true));
	registerSetting(new BoolSetting("Effects", "NULL", &showEffects, true));
	registerSetting(new BoolSetting("Watermark", "NULL", &watermark, true));
	registerSetting(new BoolSetting("Ping", "NULL", &ping, true));
	registerSetting(new SliderSetting<int>("Opacity", "NULL", &opacity, 130, 0, 255));
	registerSetting(new SliderSetting<int>("Spacing", "NULL", &spacing, -2, -3, 3));
	registerSetting(new SliderSetting<int>("Offset", "NULL", &offset, 10, 0, 30));
}

HUD::~HUD() {
}

void HUD::onMCRender(MinecraftUIRenderContext* ctx) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (localPlayer == nullptr) {
		cachedData.valid = false;
		return;
	}

	if (!Game::canUseMoveKeys()) {
		cachedData.valid = false;
		return;
	}

	cachedData.valid = true;
	cachedData.windowSize = Game::clientInstance->guiData->windowSizeReal;
	cachedData.windowSizeScaled = Game::clientInstance->guiData->windowSizeScaled;

	Vec3<float> lpPos = localPlayer->getPos();
	cachedData.posX = lpPos.x;
	cachedData.posY = lpPos.y - 1.6f;
	cachedData.posZ = lpPos.z;

	cachedData.dimensionId = localPlayer->getDimensionTypeComponent()->type;

	cachedData.rotation = localPlayer->rotation->presentRot.y;
	if (cachedData.rotation < 0) {
		cachedData.rotation += 360.0f;
	}

	cachedData.ping = -1;
	RaknetConnector* rakNet = localPlayer->level->getPacketSender()->networkSystem->remoteConnector->rakNet;
	if (rakNet != nullptr) {
		RakPeer* rakPeer = rakNet->peer;
		if (rakPeer != nullptr) {
			cachedData.ping = rakPeer->getPing();
		}
	}

	cachedData.effects.clear();
	for (uint32_t effectId = 1; effectId < 37; effectId++) {
		MobEffect* mEffect = MobEffect::getById(effectId);
		if (mEffect == nullptr)
			continue;
		if (localPlayer->hasEffect(mEffect)) {
			MobEffectInstance* mEffectInstance = localPlayer->getEffect(mEffect);
			if (mEffectInstance == nullptr)
				continue;
			CachedEffect ce;
			ce.name = mEffectInstance->getDisplayName();
			ce.duration = getEffectTimeLeftStr(mEffectInstance);
			ce.color = mEffect->color;
			cachedData.effects.push_back(ce);
		}
	}

	cachedData.armorItems.clear();
	for (int i = 0; i < 4; i++) {
		ItemStack* stack = localPlayer->getArmor(i);
		if (stack->isValid()) {
			cachedData.armorItems.push_back(stack);
		}
	}

	if (showArmor) {
		static float constexpr scale = 1.f;
		static float constexpr spacing = 18.f * scale;
		Vec2<float> armorHudPos = Vec2<float>(cachedData.windowSizeScaled.x / 2.f + 50.f, cachedData.windowSizeScaled.y - 58.f);

		for (size_t i = 0; i < cachedData.armorItems.size(); i++) {
			armorHudPos.x -= (spacing / 2.f);
		}

		for (size_t i = 0; i < cachedData.armorItems.size(); i++) {
			MCR::drawItem(armorHudPos, cachedData.armorItems[i], scale, true);
			armorHudPos.x += spacing;
		}
	}
}

void HUD::onImGuiRender(ImDrawList* drawList) {
	if (!cachedData.valid)
		return;

	static Colors* colorMod = ModuleManager::getModule<Colors>();
	Vec2<float> windowSize = cachedData.windowSize;

	float textSize = 1.f;
	float textPaddingX = std::max(1.f, ((float)spacing + 3.f)) * textSize;
	float textPaddingY = (float)spacing * textSize;
	float textHeight = ImGuiRenderer::getTextHeight("", textSize);
	float posX = 0.f + (float)offset;
	float posY = windowSize.y - (textHeight + textPaddingY * 2.f) - (float)offset;

	if (watermark) {
		ImGuiRenderer::drawText(Vec2<float>(5, 5), Client::getClientName(), UIColor(colorMod->getColor().r, colorMod->getColor().g, colorMod->getColor().b, 255), 1.f);
	}

	if (showPosition) {
		if (cachedData.dimensionId == 0) {
			char netherPosText[50];
			sprintf_s(netherPosText, 50, "Nether: %.1f, %.1f, %.1f", (cachedData.posX / 8.f), cachedData.posY, (cachedData.posZ / 8.f));
			ImGuiRenderer::fillRectangle(Vec4<float>(posX, posY, posX + ImGuiRenderer::getTextWidth(std::string(netherPosText), textSize, false) + textPaddingX * 2.f, posY + textHeight + textPaddingY * 2.f), UIColor(0, 0, 0, opacity));
			ImGuiRenderer::drawText(Vec2<float>(posX + textPaddingX, posY + textPaddingY), std::string(netherPosText), UIColor(255, 255, 255), textSize, false);
			posY -= textHeight + (textPaddingY * 2.f);
		}
		else if (cachedData.dimensionId == 1) {
			char overworldPosText[50];
			sprintf_s(overworldPosText, 50, "Overworld: %.1f, %.1f, %.1f", (cachedData.posX * 8.f), cachedData.posY, (cachedData.posZ * 8.f));
			ImGuiRenderer::fillRectangle(Vec4<float>(posX, posY, posX + ImGuiRenderer::getTextWidth(std::string(overworldPosText), textSize, false) + textPaddingX * 2.f, posY + textHeight + textPaddingY * 2.f), UIColor(0, 0, 0, opacity));
			ImGuiRenderer::drawText(Vec2<float>(posX + textPaddingX, posY + textPaddingY), std::string(overworldPosText), UIColor(255, 255, 255), textSize, false);
			posY -= textHeight + (textPaddingY * 2.f);
		}

		char posText[50];
		sprintf_s(posText, 50, "Position: %.1f, %.1f, %.1f", cachedData.posX, cachedData.posY, cachedData.posZ);
		ImGuiRenderer::fillRectangle(Vec4<float>(posX, posY, posX + ImGuiRenderer::getTextWidth(std::string(posText), textSize, false) + textPaddingX * 2.f, posY + textHeight + textPaddingY * 2.f), UIColor(0, 0, 0, opacity));
		ImGuiRenderer::drawText(Vec2<float>(posX + textPaddingX, posY + textPaddingY), std::string(posText), UIColor(255, 255, 255), textSize, false);
		posY -= textHeight + (textPaddingY * 2.f);
	}

	if (showDirection) {
		float rotation = cachedData.rotation;
		std::string directionText;
		if ((0 <= rotation && rotation < 45) || (315 <= rotation && rotation < 360)) {
			directionText = "South (+Z)";
		}
		else if (45 <= rotation && rotation < 135) {
			directionText = "West (-X)";
		}
		else if (135 <= rotation && rotation < 225) {
			directionText = "North (-Z)";
		}
		else if (225 <= rotation && rotation < 315) {
			directionText = "East (+X)";
		}
		else {
			directionText = "NULL";
		}
		char finalText[25];
		sprintf_s(finalText, 25, "Direction: %s", directionText.c_str());

		ImGuiRenderer::fillRectangle(Vec4<float>(posX, posY, posX + ImGuiRenderer::getTextWidth(std::string(finalText), textSize) + textPaddingX * 2.f, posY + textHeight + textPaddingY * 2.f), UIColor(0, 0, 0, opacity));
		ImGuiRenderer::drawText(Vec2<float>(posX + textPaddingX, posY + textPaddingY), std::string(finalText), UIColor(255, 255, 255), textSize);

		posY -= textHeight + (textPaddingY * 2.f);
	}

	if (fps) {
		static int frameCount = 0;
		static float elapsed = 0.f;
		static float cachedFps = 0.f;
		static float displayedFps = 0.f;

		frameCount++;
		elapsed += ImGuiRenderer::deltaTime;
		if (elapsed >= 0.5f) {
			cachedFps = frameCount / elapsed;
			frameCount = 0;
			elapsed = 0.f;

			if (displayedFps == 0.f) {
				displayedFps = cachedFps;
			}
			else if (cachedFps >= displayedFps) {
				displayedFps = cachedFps;
			}
			else if (displayedFps - cachedFps > 2.f) {
				displayedFps = cachedFps;
			}
		}

		const std::string fpsText = "FPS: " + std::to_string((int)(displayedFps + 0.5f));
		ImGuiRenderer::fillRectangle(Vec4<float>(posX, posY, posX + ImGuiRenderer::getTextWidth(fpsText, textSize, false) + textPaddingX * 2.f, posY + textHeight + textPaddingY * 2.f), UIColor(0, 0, 0, opacity));
		ImGuiRenderer::drawText(Vec2<float>(posX + textPaddingX, posY + textPaddingY), fpsText, UIColor(255, 255, 255), textSize, false);
		posY -= textHeight + (textPaddingY * 2.f);
	}

	if (ping) {
		const std::string pingText = "Ping: " + std::to_string(cachedData.ping) + "ms";
		ImGuiRenderer::fillRectangle(Vec4<float>(posX, posY, posX + ImGuiRenderer::getTextWidth(pingText, textSize, false) + textPaddingX * 2.f, posY + textHeight + textPaddingY * 2.f), UIColor(0, 0, 0, opacity));
		ImGuiRenderer::drawText(Vec2<float>(posX + textPaddingX, posY + textPaddingY), pingText, UIColor(255, 255, 255), textSize, false);
		posY -= textHeight + (textPaddingY * 2.f);
	}

	if (showEffects) {
		static Arraylist* arrayListMod = ModuleManager::getModule<Arraylist>();

		Vec2<float> EffectPos = Vec2<float>(windowSize.x - (float)offset, (float)offset);
		if (!arrayListMod->bottom)
			EffectPos.y = windowSize.y - (textHeight + textPaddingY * 2.f) - (float)offset;

		for (const auto& effect : cachedData.effects) {
			std::string text1 = effect.name + " ";
			std::string text2 = effect.duration;
			std::string fullText = text1 + text2;

			Vec4<float> eRectPos = Vec4<float>(EffectPos.x - ImGuiRenderer::getTextWidth(fullText, textSize, false) - (textPaddingX * 2.f),
				EffectPos.y,
				EffectPos.x,
				EffectPos.y + textHeight + (textPaddingY * 2.f));

			Vec2<float> eTextPos = Vec2<float>(eRectPos.x + textPaddingX, eRectPos.y + textPaddingY);
			Vec2<float> eDurationTextPos = Vec2<float>(eTextPos.x + ImGuiRenderer::getTextWidth(text1, textSize, true), eTextPos.y);

			ImGuiRenderer::fillRectangle(eRectPos, UIColor(0, 0, 0, opacity));
			ImGuiRenderer::drawText(eTextPos, text1, effect.color, textSize, true);
			ImGuiRenderer::drawText(eDurationTextPos, text2, UIColor(255, 255, 255), textSize, false);

			EffectPos.y -= (textHeight + (textPaddingY * 2.f)) * (arrayListMod->bottom ? -1.f : 1.f);
		}
	}
}