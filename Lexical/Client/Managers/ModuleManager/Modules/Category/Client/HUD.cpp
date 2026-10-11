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

void HUD::onImGuiRender(ImDrawList* drawList) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (localPlayer == nullptr)
		return;
	static Colors* colorMod = ModuleManager::getModule<Colors>();
	Vec2<float> windowSize = Game::clientInstance->guiData->windowSizeReal;	//ImGuiRenderer::getWindowSize();
	Vec2<float> windowSizeScaled = Game::clientInstance->guiData->windowSizeScaled;

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
		Vec3<float> lpPos = localPlayer->getPos();
		int dimensionId = localPlayer->getDimensionTypeComponent()->type;
		if (dimensionId == 0) {
			// Overworld
			char netherPosText[50];
			sprintf_s(netherPosText, 50, "Nether: %.1f, %.1f, %.1f", (lpPos.x / 8.f), (lpPos.y - 1.6f), (lpPos.z / 8.f));
			ImGuiRenderer::fillRectangle(Vec4<float>(posX, posY, posX + ImGuiRenderer::getTextWidth(std::string(netherPosText), textSize, false) + textPaddingX * 2.f, posY + textHeight + textPaddingY * 2.f), UIColor(0, 0, 0, opacity));
			ImGuiRenderer::drawText(Vec2<float>(posX + textPaddingX, posY + textPaddingY), std::string(netherPosText), UIColor(255, 255, 255), textSize, false);
			posY -= textHeight + (textPaddingY * 2.f);
		}
		else if (dimensionId == 1) {
			// Nether
			char overworldPosText[50];
			sprintf_s(overworldPosText, 50, "Overworld: %.1f, %.1f, %.1f", (lpPos.x * 8.f), (lpPos.y - 1.6f), (lpPos.z * 8.f));
			ImGuiRenderer::fillRectangle(Vec4<float>(posX, posY, posX + ImGuiRenderer::getTextWidth(std::string(overworldPosText), textSize, false) + textPaddingX * 2.f, posY + textHeight + textPaddingY * 2.f), UIColor(0, 0, 0, opacity));
			ImGuiRenderer::drawText(Vec2<float>(posX + textPaddingX, posY + textPaddingY), std::string(overworldPosText), UIColor(255, 255, 255), textSize, false);
			posY -= textHeight + (textPaddingY * 2.f);
		}

		char posText[50];
		sprintf_s(posText, 50, "Position: %.1f, %.1f, %.1f", lpPos.x, (lpPos.y - 1.6f), lpPos.z);
		ImGuiRenderer::fillRectangle(Vec4<float>(posX, posY, posX + ImGuiRenderer::getTextWidth(std::string(posText), textSize, false) + textPaddingX * 2.f, posY + textHeight + textPaddingY * 2.f), UIColor(0, 0, 0, opacity));
		ImGuiRenderer::drawText(Vec2<float>(posX + textPaddingX, posY + textPaddingY), std::string(posText), UIColor(255, 255, 255), textSize, false);
		posY -= textHeight + (textPaddingY * 2.f);
	}
	
	if (showDirection) {
		float rotation = localPlayer->rotation->presentRot.y;
		if (rotation < 0) {
			rotation += 360.0;
		}
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
		const int fps = 1 / ImGuiRenderer::deltaTime;
		const std::string fpsText = "FPS: " + std::to_string(fps);
		ImGuiRenderer::fillRectangle(Vec4<float>(posX, posY, posX + ImGuiRenderer::getTextWidth(fpsText, textSize, false) + textPaddingX * 2.f, posY + textHeight + textPaddingY * 2.f), UIColor(0, 0, 0, opacity));
		ImGuiRenderer::drawText(Vec2<float>(posX + textPaddingX, posY + textPaddingY), fpsText, UIColor(255, 255, 255), textSize, false);
		posY -= textHeight + (textPaddingY * 2.f);
	}

	if (ping) {
		int ping = -1;
		RaknetConnector* rakNet = localPlayer->level->getPacketSender()->networkSystem->remoteConnector->rakNet;
		if (rakNet != nullptr) {
			RakPeer* rakPeer = rakNet->peer;
			if (rakPeer != nullptr) {
				ping = rakPeer->getPing();
			}
		}
		const std::string pingText = "Ping: " + std::to_string(ping) + "ms";
		ImGuiRenderer::fillRectangle(Vec4<float>(posX, posY, posX + ImGuiRenderer::getTextWidth(pingText, textSize, false) + textPaddingX * 2.f, posY + textHeight + textPaddingY * 2.f), UIColor(0, 0, 0, opacity));
		ImGuiRenderer::drawText(Vec2<float>(posX + textPaddingX, posY + textPaddingY), pingText, UIColor(255, 255, 255), textSize, false);
		posY -= textHeight + (textPaddingY * 2.f);
	}

	if (showEffects) {
		static Arraylist* arrayListMod = ModuleManager::getModule<Arraylist>();

		Vec2<float> EffectPos = Vec2<float>(windowSize.x - (float)offset, (float)offset);
		if (!arrayListMod->bottom)
			EffectPos.y = windowSize.y - (textHeight + textPaddingY * 2.f) - (float)offset;

		for (uint32_t effectId = 1; effectId < 37; effectId++) {
			MobEffect* mEffect = MobEffect::getById(effectId);
			if (mEffect == nullptr)
				continue;
			if (localPlayer->hasEffect(mEffect)) {
				MobEffectInstance* mEffectInstance = localPlayer->getEffect(mEffect);
				if (mEffectInstance == nullptr)
					continue;
				std::string text1 = mEffectInstance->getDisplayName() + " ";
				std::string text2 = getEffectTimeLeftStr(mEffectInstance);
				std::string fullText = text1 + text2;

				Vec4<float> eRectPos = Vec4<float>(EffectPos.x - ImGuiRenderer::getTextWidth(fullText, textSize, false) - (textPaddingX * 2.f),
												   EffectPos.y,
												   EffectPos.x,
												   EffectPos.y + textHeight + (textPaddingY * 2.f));

				Vec2<float> eTextPos = Vec2<float>(eRectPos.x + textPaddingX, eRectPos.y + textPaddingY);
				Vec2<float> eDurationTextPos = Vec2<float>(eTextPos.x + ImGuiRenderer::getTextWidth(text1, textSize, true), eTextPos.y);

				ImGuiRenderer::fillRectangle(eRectPos, UIColor(0, 0, 0, opacity));
				ImGuiRenderer::drawText(eTextPos, text1, mEffect->color, textSize, true);
				ImGuiRenderer::drawText(eDurationTextPos, text2, UIColor(255, 255, 255), textSize, false);

				EffectPos.y -= (textHeight + (textPaddingY * 2.f)) * (arrayListMod->bottom ? -1.f : 1.f);
			}
		}
	}
}

void HUD::onMCRender(MinecraftUIRenderContext* ctx) {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (localPlayer == nullptr)
		return;

	if (!Game::canUseMoveKeys())
		return;

	Vec2<float> windowSizeScaled = Game::clientInstance->guiData->windowSizeScaled;

	if (showArmor) {
		static float constexpr scale = 1.f;
		static float constexpr spacing = 18.f * scale;
		Vec2<float> armorHudPos = Vec2<float>(windowSizeScaled.x / 2.f + 50.f, windowSizeScaled.y - 58.f);

		ItemStack* armorItemStack[4] = { nullptr, nullptr, nullptr, nullptr };

		for (int i = 0; i < 4; i++) {
			ItemStack* stack = localPlayer->getArmor(i);
			if (stack->isValid()) {
				armorItemStack[i] = stack;
				armorHudPos.x -= (spacing / 2.f);
			}
		}

		for (int i = 0; i < 4; i++) {
			if (armorItemStack[i] != nullptr) {
				MCR::drawItem(armorHudPos, armorItemStack[i], scale, true);
				armorHudPos.x += spacing;
			}
		}
	}
}