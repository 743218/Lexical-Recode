#include "ClickGUI.h"
#include "../../../../../../Utils/TimerUtil.h"
#include "../../../../../../Libs/json.hpp"
#include "../../../../../../Renderer/ImGuiRenderer.h"
#include "../../../ModuleManager.h"

ClickGUI::ClickGUI() : Module("ClickGUI", "Display all modules", Category::CLIENT, VK_INSERT) {
	registerSetting(new ColorSetting("Color", "NULL", &mainColor, mainColor));
	registerSetting(new SliderSetting<float>("Blur", "Background blur intensity", &blurStrength, 4.f, 0.f, 20.f));
	registerSetting(new BoolSetting("Description", "Show Description", &showDescription, true));
}

ClickGUI::~ClickGUI() {
	for (auto& window : windowList) {
		delete window;
	}
	windowList.clear();
}

ClickGUI::ClickWindow::ClickWindow(std::string windowName, Vec2<float> startPos, Category c) {
	this->name = windowName;
	this->category = c;
	this->pos = startPos;
	this->extended = true;

	for (auto& mod : ModuleManager::moduleList) {
		if (mod->getCategory() == c) {
			this->moduleList.push_back(mod);
		}
	}

	std::sort(this->moduleList.begin(), this->moduleList.end(), [](Module* lhs, Module* rhs) {
		return lhs->getModuleName() < rhs->getModuleName();
	});
}

void ClickGUI::onDisable() {
	Game::clientInstance->grabMouse();
	
	isLeftClickDown = false;
	isRightClickDown = false;
	isHoldingLeftClick = false;
	isHoldingRightClick = false;

	draggingWindowPtr = nullptr;

	capturingKbSettingPtr = nullptr;
	draggingSliderSettingPtr = nullptr;

	openAnim = 0.0f;
}

void ClickGUI::onEnable() {
	Game::clientInstance->releasebMouse();
	openAnim = 0.0f;
}

bool ClickGUI::isVisible() {
	return false;
}

void ClickGUI::onKeyUpdate(int key, bool isDown) {
	if (!isEnabled()) {
		if (key == getKeybind() && isDown) {
			setEnabled(true);
		}
	}
	else {
		if (isDown) {
			if (key < 192) {
				if (capturingKbSettingPtr != nullptr) {
					if (key != VK_ESCAPE)
						*capturingKbSettingPtr->value = key;
					capturingKbSettingPtr = nullptr;
					return;
				}
			}
			if (key == getKeybind() || key == VK_ESCAPE) {
				setEnabled(false);
			}
		}

		if (key == VK_SHIFT)
			isHoldingShift = isDown;
	}
}

void ClickGUI::onMouseUpdate(Vec2<float> mousePosA, char mouseButton, char isDown) {

	//MouseButtons
	//0 = mouse move
	//1 = left click
	//2 = right click
	//3 = middle click
	//4 = scroll   (isDown: 120 (SCROLL UP) and -120 (SCROLL DOWN))

	switch (mouseButton) {
	case 0:
		mousePos = mousePosA;
		break;
	case 1:
		isLeftClickDown = isDown;
		isHoldingLeftClick = isDown;
		break;
	case 2:
		isRightClickDown = isDown;
		isHoldingRightClick = isDown;
		break;
	case 4:
		float moveVec = (isDown < 0) ? -15.f : 15.f;
		for (auto& window : windowList) {
			if (window == draggingWindowPtr)
				continue;

			if (!isHoldingShift)
				window->pos.y += moveVec;
			else
				window->pos.x += moveVec;
		}
		break;
	}

	if (draggingWindowPtr != nullptr) {
		if (!isHoldingLeftClick)
			draggingWindowPtr = nullptr;
	}

	if (capturingKbSettingPtr != nullptr) {
		if (isRightClickDown) {
			*capturingKbSettingPtr->value = 0;
			capturingKbSettingPtr = nullptr;
			isRightClickDown = false;
		}
	}

	if (draggingSliderSettingPtr != nullptr) {
		if (!isHoldingLeftClick)
			draggingSliderSettingPtr = nullptr;
	}
}

void ClickGUI::InitClickGUI() {
	setEnabled(false);

	Vec2<float> startPos = Vec2<float>(25.f, 35.f);
	windowList.push_back(new ClickWindow("Combat", startPos, Category::COMBAT));
	startPos.x += 265.f;
	windowList.push_back(new ClickWindow("Movement", startPos, Category::MOVEMENT));
	startPos.x += 265.f;
	windowList.push_back(new ClickWindow("Render", startPos, Category::RENDER));
	startPos.x += 265.f;
	windowList.push_back(new ClickWindow("Player", startPos, Category::PLAYER));
	startPos.x += 265.f;
	windowList.push_back(new ClickWindow("World", startPos, Category::WORLD));
	startPos.x += 265.f;
	windowList.push_back(new ClickWindow("Misc", startPos, Category::MISC));
	startPos.x += 265.f;
	windowList.push_back(new ClickWindow("Client", startPos, Category::CLIENT));

	initialized = true;
}

void ClickGUI::RenderImGui() {
	if (!initialized)
		return;

	// Note: text size/height scale with the CustomFont size modifier,
	// but window widths stay fixed (fontPercent = 1.f below).
	if (Game::canUseMoveKeys())
		Game::clientInstance->releasebMouse();

	static Vec2<float> oldMousePos = mousePos;
	mouseDelta = mousePos.sub(oldMousePos);
	oldMousePos = mousePos;

	Vec2<float> screenSize = Game::clientInstance->guiData->windowSizeReal;	//ImGuiRenderer::getWindowSize();
	float deltaTime = ImGuiRenderer::deltaTime;

	float textSize = 1.f;
	float textPaddingX = 3.f * textSize;
	float textPaddingY = 0.f * textSize;
	float textHeight = ImGuiRenderer::getTextHeight("", textSize);

	std::string descriptionText = "NULL";

	//UIColor mainColor = UIColor(115, 135, 255, 125);	// 135

	//openAnim = Math::lerp(openAnim, 1.f, deltaTime * 2.5f);
	openAnim += deltaTime * 2.f;
	if (openAnim > 1.f)
		openAnim = 1.f;

	// Background screen
	if (blurStrength > 0.1f)
		ImGuiRenderer::addBlur(Vec4<float>(0.f, 0.f, screenSize.x, screenSize.y), blurStrength * openAnim);
	ImGuiRenderer::fillRectangle(Vec4<float>(0.f, 0.f, screenSize.x, screenSize.y), UIColor(0, 0, 0, (int)(110 * openAnim)));

	// Render every single window
	for (auto& window : windowList) {
		// Logic for window 
		if (window->pos.x > screenSize.x) window->pos.x = screenSize.x - 265.f;
		if (window == draggingWindowPtr) {
			window->pos = window->pos.add(mouseDelta);
		}

		// Header RectPos (fixed layout, ignores CustomFont size modifier)
		float fontPercent = 1.f;

		Vec4<float> hRectPos = Vec4<float>(window->pos.x,
										   window->pos.y,
										   window->pos.x + (int)(250.f * fontPercent) + (textPaddingX * 2.f),
										   window->pos.y + textHeight + (textPaddingY * 2.f));

		Vec2<float> hTextPos = Vec2<float>(hRectPos.x + textPaddingX, hRectPos.y + textPaddingY);

		if (hRectPos.contains(mousePos)) {
			if (isLeftClickDown) {
				draggingWindowPtr = window;
				isLeftClickDown = false;
			}
			else if (isRightClickDown) {
				window->extended = !window->extended;
				isRightClickDown = false;
			}
		}

		updateSelectedAnimRect(hRectPos, window->selectedAnim);

		ImGuiRenderer::fillRectangle(hRectPos, mainColor);
		ImGuiRenderer::drawText(hTextPos, window->name, UIColor(255, 255, 255), textSize);

		ImGuiRenderer::fillRectangle(hRectPos, UIColor(255, 255, 255, (int)(25 * window->selectedAnim)));

		if (window->extended) {
			float moduleSpace = 2.f * textSize * fontPercent;
			float settingSpace = 2.f * textSize * fontPercent;
			float yHeight = 0.f;
			yHeight += moduleSpace;

			for (auto& mod : window->moduleList) {
				yHeight += textHeight + (textPaddingY * 2.f);

				if (mod->extended) {
					yHeight += settingSpace;
					for (auto& setting : mod->getSettingList()) {

						if (setting->dependOn.has_value()) {
							// The setting didn't pass the check so continue
							if (!setting->dependOn.value()())
								continue;
						}

						if (setting->type == SettingType::COLOR_S) {
							ColorSetting* colorSetting = static_cast<ColorSetting*>(setting);
							yHeight += textHeight + (textPaddingY * 2.f);
							if (colorSetting->extended) {
								yHeight += settingSpace;
								for (auto& slider : colorSetting->colorSliders) {
									yHeight += textHeight + (textPaddingY * 2.f) + settingSpace;
								}

								if (colorSetting->showSynced) {
									yHeight += textHeight + (textPaddingY * 2.f) + settingSpace;
								}

								yHeight -= settingSpace;
							}
						}
						else if (setting->type == SettingType::VEC3_S) {
							Vec3SettingBase* vecSetting = static_cast<Vec3SettingBase*>(setting);
							yHeight += textHeight + (textPaddingY * 2.f);
							if (vecSetting->extended) {
								yHeight += settingSpace;
								for (int i = 0; i < 3; i++) {
									yHeight += textHeight + (textPaddingY * 2.f) + settingSpace;
								}
								yHeight -= settingSpace;
							}
						}
						else yHeight += textHeight + (textPaddingY * 2.f);
						yHeight += settingSpace;
					}
				}

				yHeight += moduleSpace;
			}

			// Window background rectPos
			float wbgPaddingX = 2.f * textSize * fontPercent;
			Vec4<float> wbgRectPos = Vec4<float>(hRectPos.x + wbgPaddingX, hRectPos.w, hRectPos.z - wbgPaddingX, hRectPos.w + yHeight);
			ImGuiRenderer::fillRectangle(wbgRectPos, UIColor(0, 0, 0, 75));

			float yOffset = hRectPos.w + moduleSpace;
			for (auto& mod : window->moduleList) {
				float modPaddingX = wbgPaddingX + (2.f * textSize * fontPercent);
				Vec4<float> mRectPos = Vec4<float>(hRectPos.x + modPaddingX,
												   yOffset,
												   hRectPos.z - modPaddingX,
												   yOffset + textHeight + (textPaddingY * 2.f));

				Vec2<float> mTextPos = Vec2<float>(mRectPos.x + textPaddingX, mRectPos.y + textPaddingY);

				if (mRectPos.contains(mousePos)) {
					descriptionText = mod->getDescription();
					if (isLeftClickDown) {
						mod->toggle();
						isLeftClickDown = false;
					}
					else if (isRightClickDown) {
						mod->extended = !mod->extended;
						isRightClickDown = false;
					}
				}

				updateSelectedAnimRect(mRectPos, mod->selectedAnim);

				ImGuiRenderer::fillRectangle(mRectPos, mod->isEnabled() ? mainColor : UIColor(0, 0, 0, 35));
				ImGuiRenderer::drawText(mTextPos, mod->getModuleName(), mod->isEnabled() ? UIColor(255, 255, 255) : ColorUtil::lerp(UIColor(175, 175, 175), UIColor(255, 255, 255), mod->selectedAnim), textSize);
				ImGuiRenderer::fillRectangle(mRectPos, UIColor(255, 255, 255, (int)(25 * mod->selectedAnim)));

				yOffset += textHeight + (textPaddingY * 2.f);

				if (mod->extended) {
					yOffset += settingSpace;
					float startY = yOffset;
					float settingPaddingX = 7.5f * textSize * fontPercent;
					float settingPaddingZ = 3.5f * textSize * fontPercent;

					for (auto& setting : mod->getSettingList()) {

						if (setting->dependOn.has_value()) {
							// The setting didn't pass the check so continue
							if (!setting->dependOn.value()())
								continue;
						}

						std::string settingName = setting->name;

						Vec4<float> sRectPos = Vec4<float>(mRectPos.x + settingPaddingX,
														   yOffset,
														   mRectPos.z - settingPaddingZ,
														   yOffset + textHeight + (textPaddingY * 2.f));

						Vec2<float> sTextPos = Vec2<float>(sRectPos.x + textPaddingX, sRectPos.y + textPaddingY);

						if (sRectPos.contains(mousePos)) {
							descriptionText = setting->description;
						}

						updateSelectedAnimRect(sRectPos, setting->selectedAnim);

						switch (setting->type) {
						case SettingType::BOOL_S: {
							BoolSetting* boolSetting = static_cast<BoolSetting*>(setting);
							bool& boolValue = (*boolSetting->value);

							if (sRectPos.contains(mousePos)) {
								if (isLeftClickDown) {
									boolValue = !boolValue;
									isLeftClickDown = false;
								}
							}

							if (boolValue)
								ImGuiRenderer::fillRectangle(sRectPos, mainColor);

							ImGuiRenderer::drawText(sTextPos, settingName, UIColor(255, 255, 255), textSize);

							yOffset += textHeight + (textPaddingY * 2.f);
							break;
						}
						case SettingType::KEYBIND_S: {
							KeybindSetting* keybindSetting = static_cast<KeybindSetting*>(setting);
							int& keyValue = (*keybindSetting->value);

							if (sRectPos.contains(mousePos)) {
								if (isLeftClickDown) {
									if (capturingKbSettingPtr == keybindSetting)
										capturingKbSettingPtr = nullptr;
									else
										capturingKbSettingPtr = keybindSetting;
									
									isLeftClickDown = false;
								}
							}

							std::string keybindName;
							if (setting == capturingKbSettingPtr) {
								keybindName = "...";
							}
							else {
								if (keyValue != 0)
									keybindName = KeyNames[keyValue];
								else
									keybindName = "None";
							}

							Vec2<float> keybindTextPos = Vec2<float>(sRectPos.z - textPaddingX - ImGuiRenderer::getTextWidth(keybindName, textSize), sTextPos.y);

							ImGuiRenderer::drawText(sTextPos, settingName + ":", UIColor(255, 255, 255), textSize);
							ImGuiRenderer::drawText(keybindTextPos, keybindName, UIColor(255, 255, 255), textSize);

							yOffset += textHeight + (textPaddingY * 2.f);
							break;
						}
						case SettingType::ENUM_S: {
							EnumSetting* enumSetting = static_cast<EnumSetting*>(setting);
							int& enumValue = (*enumSetting->value);

							if (sRectPos.contains(mousePos)) {
								if (isLeftClickDown) {
									enumValue++;
									if (enumValue > enumSetting->enumList.size() - 1)
										enumValue = 0;
									isLeftClickDown = false;
								}
								else if (isRightClickDown) {
									enumValue--;
									if (enumValue < 0)
										enumValue = (int)enumSetting->enumList.size() - 1;
									isRightClickDown = false;
								}
							}

							std::string modeName = enumSetting->enumList[enumValue];
							Vec2<float> modeTextPos = Vec2<float>(sRectPos.z - textPaddingX - ImGuiRenderer::getTextWidth(modeName, textSize), sTextPos.y);

							ImGuiRenderer::drawText(sTextPos, settingName + ":", UIColor(255, 255, 255), textSize);
							ImGuiRenderer::drawText(modeTextPos, modeName, UIColor(255, 255, 255), textSize);

							yOffset += textHeight + (textPaddingY * 2.f);
							break;
						}
						case SettingType::COLOR_S: {
							ColorSetting* colorSetting = static_cast<ColorSetting*>(setting);
							
							if (sRectPos.contains(mousePos)) {
								if (isRightClickDown) {
									colorSetting->extended = !colorSetting->extended;
									isRightClickDown = false;
								}
							}

							float colorBoxSize = std::round(textHeight / 1.5f);
							float colorBoxPaddingX = textPaddingX + (2.f * textSize);
							Vec4<float> colorBoxRect = Vec4<float>(sRectPos.z - colorBoxPaddingX - colorBoxSize,
															    (sRectPos.y + sRectPos.w - colorBoxSize) / 2.f,
																sRectPos.z - colorBoxPaddingX,
																(sRectPos.y + sRectPos.w + colorBoxSize) / 2.f);

							ImGuiRenderer::drawText(sTextPos, settingName + ":", UIColor(255, 255, 255), textSize);
							ImGuiRenderer::fillRectangle(colorBoxRect, (*colorSetting->colorPtr));

							yOffset += textHeight + (textPaddingY * 2.f);

							if (colorSetting->extended) {
								yOffset += settingSpace;

								float colorStartX = sRectPos.x;
								float colorStartY = yOffset;

								for (auto& slider : colorSetting->colorSliders) {

									Vec4<float> colorSliderRect = Vec4<float>(sRectPos.x + settingPaddingX,
																			  yOffset,
																			  sRectPos.z - settingPaddingZ,
																			  yOffset + textHeight + (textPaddingY * 2.f));

									Vec2<float> colorSliderTextPos = Vec2<float>(colorSliderRect.x + textPaddingX, colorSliderRect.y + textPaddingY);

									updateSelectedAnimRect(colorSliderRect, slider->selectedAnim);

									if (colorSliderRect.contains(mousePos)) {
										if (isLeftClickDown) {
											draggingSliderSettingPtr = slider;
											isLeftClickDown = false;
										}
									}

									uint8_t& value = (*slider->valuePtr);
									float minValue = (float)slider->minValue;
									float maxValue = (float)slider->maxValue;

									if (draggingSliderSettingPtr == slider) {
										float draggingPercent = (mousePos.x - colorSliderRect.x) / (colorSliderRect.z - colorSliderRect.x);

										if (draggingPercent > 1.f)
											draggingPercent = 1.f;
										if (draggingPercent < 0.f)
											draggingPercent = 0.f;

										value = (int)minValue + (int)std::round((maxValue - minValue) * draggingPercent);
									}

									float valuePercent = (value - minValue) / (maxValue - minValue);

									if (valuePercent > 1.f)
										valuePercent = 1.f;
									if (valuePercent < 0.f)
										valuePercent = 0.f;

									Vec4<float> valueRectPos = Vec4<float>(colorSliderRect.x, colorSliderRect.y, colorSliderRect.x + (colorSliderRect.z - colorSliderRect.x) * valuePercent, colorSliderRect.w);

									char valueText[10];
									sprintf_s(valueText, 10, "%i", (int)value);
									std::string valueTextStr(valueText);

									Vec2<float> valueTextPos = Vec2<float>(colorSliderRect.z - textPaddingX - ImGuiRenderer::getTextWidth(valueTextStr, textSize, (draggingSliderSettingPtr != slider)), colorSliderTextPos.y);

									ImGuiRenderer::fillRectangle(valueRectPos, mainColor);	
									ImGuiRenderer::drawText(colorSliderTextPos, slider->name + ":", UIColor(255, 255, 255), textSize);
									ImGuiRenderer::drawText(valueTextPos, valueTextStr, UIColor(255, 255, 255), textSize, (draggingSliderSettingPtr != slider));

									ImGuiRenderer::fillRectangle(colorSliderRect, UIColor(255, 255, 255, (int)(25 * slider->selectedAnim)));

									yOffset += textHeight + (textPaddingY * 2.f) + settingSpace;
								}
								if (colorSetting->showSynced) {
									bool& boolValue = (colorSetting->colorSynced);
									Vec4<float> syncRectPos = Vec4<float>(sRectPos.x + settingPaddingX,
										yOffset,
										sRectPos.z - settingPaddingZ,
										yOffset + textHeight + (textPaddingY * 2.f));

									Vec2<float> syncTextPos = Vec2<float>(syncRectPos.x + textPaddingX, syncRectPos.y + textPaddingY);
									updateSelectedAnimRect(syncRectPos, colorSetting->syncSelectedAnim);
									if (syncRectPos.contains(mousePos)) {
										if (isLeftClickDown) {
											boolValue = !boolValue;
											isLeftClickDown = false;
										}
									}
									if (boolValue) ImGuiRenderer::fillRectangle(syncRectPos, mainColor);
									ImGuiRenderer::fillRectangle(syncRectPos, UIColor(255, 255, 255, (int)(25 * colorSetting->syncSelectedAnim)));
									ImGuiRenderer::drawText(syncTextPos, "Synced", UIColor(255, 255, 255), textSize);
									yOffset += textHeight + (textPaddingY * 2.f) + settingSpace;
								}
								yOffset -= settingSpace;
								
								float colorEndY = yOffset;
								float colorSLineWidth = 4.f * textSize * fontPercent;
								float colorSLinePaddingX = 1.f * textSize;
								Vec4<float> colorSLineRect = Vec4<float>(colorStartX + colorSLinePaddingX,
																		 colorStartY,
																		 colorStartX + colorSLinePaddingX + colorSLineWidth,
																		 colorEndY);

								ImGuiRenderer::fillRectangle(colorSLineRect, mainColor);
							}
							break;
						}
						case SettingType::VEC3_S: {
							Vec3SettingBase* vecSetting = static_cast<Vec3SettingBase*>(setting);
							if (sRectPos.contains(mousePos)) {
								if (isRightClickDown) {
									vecSetting->extended = !vecSetting->extended;
									isRightClickDown = false;
								}
							}
							ImGuiRenderer::drawText(sTextPos, settingName, UIColor(255, 255, 255), textSize);
							yOffset += textHeight + (textPaddingY * 2.f);
							if (vecSetting->extended) {
								yOffset += settingSpace;
								const float startX = sRectPos.x;
								const float startY = yOffset;
								if (vecSetting->valueType == Vec3Type::FLOAT_T) {
									Vec3Setting<float>* vec = static_cast<Vec3Setting<float>*>(vecSetting);
									for (auto& slider : vec->vecSliders) {
										Vec4<float> sliderRect = Vec4<float>(sRectPos.x + settingPaddingX, yOffset, sRectPos.z - settingPaddingZ, yOffset + textHeight + (textPaddingY * 2.f));
										const Vec2<float>& sliderTextPos = Vec2<float>(sliderRect.x + textPaddingX, sliderRect.y + textPaddingY);
										updateSelectedAnimRect(sliderRect, slider->selectedAnim);
										if (sliderRect.contains(mousePos)) {
											if (isLeftClickDown) {
												draggingSliderSettingPtr = slider;
												isLeftClickDown = false;
											}
										}
										float& value = (*slider->valuePtr);
										float minValue = (float)slider->minValue;
										float maxValue = (float)slider->maxValue;
										if (draggingSliderSettingPtr == slider) {
											float draggingPercent = (mousePos.x - sRectPos.x) / (sRectPos.z - sRectPos.x);
											if (draggingPercent > 1.f) draggingPercent = 1.f;
											if (draggingPercent < 0.f) draggingPercent = 0.f;
											value = minValue + (maxValue - minValue) * draggingPercent;
										}
										float valuePercent = (value - minValue) / (maxValue - minValue);
										if (valuePercent > 1.f) valuePercent = 1.f;
										if (valuePercent < 0.f) valuePercent = 0.f;
										Vec4<float> valueRectPos = Vec4<float>(sliderRect.x, sliderRect.y, sliderRect.x + (sliderRect.z - sliderRect.x) * valuePercent, sliderRect.w);
										char valueText[10];
										sprintf_s(valueText, 10, "%.2f", value);
										std::string valueTextStr(valueText);
										Vec2<float> valueTextPos = Vec2<float>(sliderRect.z - textPaddingX - ImGuiRenderer::getTextWidth(valueTextStr, textSize, (draggingSliderSettingPtr != slider)), sliderRect.y);
										ImGuiRenderer::fillRectangle(valueRectPos, mainColor);
										ImGuiRenderer::drawText(sliderTextPos, slider->name + ":", UIColor(255, 255, 255), textSize);
										ImGuiRenderer::drawText(valueTextPos, valueTextStr, UIColor(255, 255, 255), textSize, (draggingSliderSettingPtr != slider));
										ImGuiRenderer::fillRectangle(sliderRect, UIColor(255, 255, 255, (int)(25 * slider->selectedAnim)));
										yOffset += textHeight + (textPaddingY * 2.f) + settingSpace;
									}
									yOffset -= settingSpace;

									const float EndY = yOffset;
									const float SLineWidth = 4.f * textSize * fontPercent;
									const float SLinePaddingX = 1.f * textSize;
									const Vec4<float>& SLineRect = Vec4<float>(startX + SLinePaddingX,
										startY,
										startX + SLinePaddingX + SLineWidth,
										EndY);

									ImGuiRenderer::fillRectangle(SLineRect, mainColor);
								}
								else if (vecSetting->valueType == Vec3Type::INT_T) {
									Vec3Setting<int>* vec = static_cast<Vec3Setting<int>*>(vecSetting);
									for (auto& slider : vec->vecSliders) {
										Vec4<float> sliderRect = Vec4<float>(sRectPos.x + settingPaddingX, yOffset, sRectPos.z - settingPaddingZ, yOffset + textHeight + (textPaddingY * 2.f));
										const Vec2<float>& sliderTextPos = Vec2<float>(sliderRect.x + textPaddingX, sliderRect.y + textPaddingY);

										updateSelectedAnimRect(sliderRect, slider->selectedAnim);

										if (sliderRect.contains(mousePos)) {
											if (isLeftClickDown) {
												draggingSliderSettingPtr = slider;
												isLeftClickDown = false;
											}
										}

										int& value = (*slider->valuePtr);
										float minValue = (float)slider->minValue;
										float maxValue = (float)slider->maxValue;

										if (draggingSliderSettingPtr == slider) {
											float draggingPercent = (mousePos.x - sliderRect.x) / (sliderRect.z - sliderRect.x);
											if (draggingPercent > 1.f) draggingPercent = 1.f;
											if (draggingPercent < 0.f) draggingPercent = 0.f;
											value = (int)minValue + (int)std::round((maxValue - minValue) * draggingPercent);
										}

										float valuePercent = (value - minValue) / (maxValue - minValue);

										if (valuePercent > 1.f) valuePercent = 1.f;
										if (valuePercent < 0.f) valuePercent = 0.f;

										Vec4<float> valueRectPos = Vec4<float>(sliderRect.x, sliderRect.y, sliderRect.x + (sliderRect.z - sliderRect.x) * valuePercent, sliderRect.w);

										char valueText[10];
										sprintf_s(valueText, 10, "%.i", (int)value);
										std::string valueTextStr(valueText);

										Vec2<float> valueTextPos = Vec2<float>(sliderRect.z - textPaddingX - ImGuiRenderer::getTextWidth(valueTextStr, textSize, (draggingSliderSettingPtr != slider)), sliderRect.y);

										ImGuiRenderer::fillRectangle(valueRectPos, mainColor);
										ImGuiRenderer::drawText(sliderTextPos, slider->name + ":", UIColor(255, 255, 255), textSize);
										ImGuiRenderer::drawText(valueTextPos, valueTextStr, UIColor(255, 255, 255), textSize, (draggingSliderSettingPtr != slider));

										ImGuiRenderer::fillRectangle(sliderRect, UIColor(255, 255, 255, (int)(25 * slider->selectedAnim)));

										yOffset += textHeight + (textPaddingY * 2.f) + settingSpace;
									}
									yOffset -= settingSpace;

									const float EndY = yOffset;
									const float SLineWidth = 4.f * textSize * fontPercent;
									const float SLinePaddingX = 1.f * textSize;
									Vec4<float> SLineRect = Vec4<float>(startX + SLinePaddingX,
										startY,
										startX + SLinePaddingX + SLineWidth,
										EndY);

									ImGuiRenderer::fillRectangle(SLineRect, mainColor);
								}
							}
							break;
						}
						case SettingType::SLIDER_S: {
							SliderSettingBase* sliderSettingBase = static_cast<SliderSettingBase*>(setting);

							if (sRectPos.contains(mousePos)) {
								if (isLeftClickDown) {
									draggingSliderSettingPtr = sliderSettingBase;
									isLeftClickDown = false;
								}
							}

							if (sliderSettingBase->valueType == ValueType::INT_T) {
								SliderSetting<int>* intSlider = static_cast<SliderSetting<int>*>(sliderSettingBase);
								int& value = (*intSlider->valuePtr);
								float minValue = (float)intSlider->minValue;
								float maxValue = (float)intSlider->maxValue;

								if (draggingSliderSettingPtr == sliderSettingBase) {
									float draggingPercent = (mousePos.x - sRectPos.x) / (sRectPos.z - sRectPos.x);

									if (draggingPercent > 1.f)
										draggingPercent = 1.f;
									if (draggingPercent < 0.f)
										draggingPercent = 0.f;

									value = (int)minValue + (int)std::round((maxValue - minValue) * draggingPercent);
								}

								float valuePercent = (value - minValue) / (maxValue - minValue);

								if (valuePercent > 1.f)
									valuePercent = 1.f;
								if (valuePercent < 0.f)
									valuePercent = 0.f;

								Vec4<float> valueRectPos = Vec4<float>(sRectPos.x, sRectPos.y, sRectPos.x + (sRectPos.z - sRectPos.x) * valuePercent, sRectPos.w);

								char valueText[10];
								sprintf_s(valueText, 10, "%i", value);
								std::string valueTextStr(valueText);

								Vec2<float> valueTextPos = Vec2<float>(sRectPos.z - textPaddingX - ImGuiRenderer::getTextWidth(valueTextStr, textSize, (draggingSliderSettingPtr != sliderSettingBase)), sTextPos.y);

								ImGuiRenderer::fillRectangle(valueRectPos, mainColor);
								ImGuiRenderer::drawText(sTextPos, settingName + ":", UIColor(255, 255, 255), textSize);
								ImGuiRenderer::drawText(valueTextPos, valueTextStr, UIColor(255, 255, 255), textSize, (draggingSliderSettingPtr != sliderSettingBase));
							}
							else if (sliderSettingBase->valueType == ValueType::FLOAT_T) {
								SliderSetting<float>* floatSlider = static_cast<SliderSetting<float>*>(sliderSettingBase);
								float& value = (*floatSlider->valuePtr);
								float minValue = floatSlider->minValue;
								float maxValue = floatSlider->maxValue;

								if (draggingSliderSettingPtr == sliderSettingBase) {
									float draggingPercent = (mousePos.x - sRectPos.x) / (sRectPos.z - sRectPos.x);

									if (draggingPercent > 1.f)
										draggingPercent = 1.f;
									if (draggingPercent < 0.f)
										draggingPercent = 0.f;

									value = minValue + (maxValue - minValue) * draggingPercent;
								}

								float valuePercent = (value - minValue) / (maxValue - minValue);

								if (valuePercent > 1.f)
									valuePercent = 1.f;
								if (valuePercent < 0.f)
									valuePercent = 0.f;

								Vec4<float> valueRectPos = Vec4<float>(sRectPos.x, sRectPos.y, sRectPos.x + (sRectPos.z - sRectPos.x) * valuePercent, sRectPos.w);

								char valueText[10];
								sprintf_s(valueText, 10, "%.2f", value);
								std::string valueTextStr(valueText);

								Vec2<float> valueTextPos = Vec2<float>(sRectPos.z - textPaddingX - ImGuiRenderer::getTextWidth(valueTextStr, textSize, (draggingSliderSettingPtr != sliderSettingBase)), sTextPos.y);

								ImGuiRenderer::fillRectangle(valueRectPos, mainColor);
								ImGuiRenderer::drawText(sTextPos, settingName + ":", UIColor(255, 255, 255), textSize);
								ImGuiRenderer::drawText(valueTextPos, valueTextStr, UIColor(255, 255, 255), textSize, (draggingSliderSettingPtr != sliderSettingBase));
							}

							yOffset += textHeight + (textPaddingY * 2.f);
							break;
						}
						}
						ImGuiRenderer::fillRectangle(sRectPos, UIColor(255, 255, 255, (int)(25 * setting->selectedAnim)));
						yOffset += settingSpace;
					}
					float endY = yOffset - settingSpace;

					float sLineWidth = 4.f * textSize * fontPercent;
					float sLinePaddingX = 1.f * textSize;
					Vec4<float> sLineRect = Vec4<float>(mRectPos.x + sLinePaddingX,
														startY,
														mRectPos.x + sLinePaddingX + sLineWidth,
														endY);

					ImGuiRenderer::fillRectangle(sLineRect, mainColor);
				}

				yOffset += moduleSpace;
			}
		}
	}

	if (showDescription && descriptionText != "NULL" && draggingWindowPtr == nullptr && draggingSliderSettingPtr == nullptr) {
		Vec2<float> mousePadding = Vec2<float>(15.f, 15.f);
		Vec4<float> rectPos = Vec4<float>(mousePos.x + mousePadding.x,
										  mousePos.y + mousePadding.y,
										  mousePos.x + mousePadding.x + ImGuiRenderer::getTextWidth(descriptionText, 0.8f) + 2.f * 2.f,
										  mousePos.y + mousePadding.y + ImGuiRenderer::getTextHeight(descriptionText, 0.8f));

		Vec2<float> textPos = Vec2<float>(rectPos.x, rectPos.y).add(Vec2<float>(2.f, 0.f));
		ImGuiRenderer::fillRectangle(rectPos, UIColor(0, 0, 0, 125));
		ImGuiRenderer::drawText(textPos, descriptionText, UIColor(255, 255, 255), 0.8f);

	}

	isLeftClickDown = false;
	isRightClickDown = false;
}

void ClickGUI::updateSelectedAnimRect(Vec4<float>& rect, float& anim) {
	bool shouldUp = rect.contains(mousePos);

	if (draggingWindowPtr != nullptr)
		shouldUp = false;

	if (draggingSliderSettingPtr != nullptr) {
		if (&draggingSliderSettingPtr->selectedAnim != &anim)
			shouldUp = false;
		else
			shouldUp = true;
	}

	//anim += ImGuiRenderer::deltaTime * (shouldUp ? 5.f : -2.f);
	if (shouldUp)
		anim = 1.f;
	else
		anim -= ImGuiRenderer::deltaTime * 2.f;

	if (anim > 1.f)
		anim = 1.f;
	if (anim < 0.f)
		anim = 0.f;
}

using json = nlohmann::json;

void ClickGUI::onLoadConfig(void* confVoid) {
	json* config = reinterpret_cast<json*>(confVoid);
	std::string modName = this->getModuleName();
	try {
		if (config->contains(modName)) {
			json obj = config->at(getModuleName());
			if (obj.is_null()) return;

			for (Setting* setting : this->getSettingList()) {
				if (obj.contains(setting->name)) {
					if (setting->type == SettingType::KEYBIND_S) {
						KeybindSetting* keySetting = (KeybindSetting*)setting;
						*keySetting->value = obj[keySetting->name].get<int>();
					}
					else if (setting->type == SettingType::BOOL_S) {
						BoolSetting* boolSetting = (BoolSetting*)setting;
						*boolSetting->value = obj[boolSetting->name].get<bool>();
					}
					else if (setting->type == SettingType::ENUM_S) {
						EnumSetting* enumSetting = (EnumSetting*)setting;
						*enumSetting->value = obj[enumSetting->name].get<int>();
					}
					else if (setting->type == SettingType::COLOR_S) {
						ColorSetting* colorSetting = (ColorSetting*)setting;
						json colorObj = obj[colorSetting->name];
						if (colorObj.contains("Red")) colorSetting->colorPtr->r = colorObj["Red"].get<int>();
						if (colorObj.contains("Green")) colorSetting->colorPtr->g = colorObj["Green"].get<int>();
						if (colorObj.contains("Blue")) colorSetting->colorPtr->b = colorObj["Blue"].get<int>();
						if (colorObj.contains("Alpha")) colorSetting->colorPtr->a = colorObj["Alpha"].get<int>();
						if (colorObj.contains("Synced")) colorSetting->colorSynced = colorObj["Synced"].get<bool>();
					}
					else if (setting->type == SettingType::SLIDER_S) {
						SliderSettingBase* sliderSetting = (SliderSettingBase*)setting;
						if (sliderSetting->valueType == ValueType::INT_T) {
							SliderSetting<int>* intSlider = (SliderSetting<int>*)sliderSetting;
							*intSlider->valuePtr = obj[intSlider->name].get<int>();
						}
						else if (sliderSetting->valueType == ValueType::FLOAT_T) {
							SliderSetting<float>* floatSlider = (SliderSetting<float>*)sliderSetting;
							*floatSlider->valuePtr = obj[floatSlider->name].get<float>();
						}
					}
				}
			}

			for (auto& window : windowList) {
				std::string windowName = window->name;

				if (obj.contains(windowName)) {
					json confValue = obj.at(windowName);
					if (confValue.is_null())
						continue;

					if (confValue.contains("isExtended")) {
						window->extended = confValue["isExtended"].get<bool>();
					}

					if (confValue.contains("pos")) {
						window->pos.x = confValue["pos"]["x"].get<float>();
						window->pos.y = confValue["pos"]["y"].get<float>();
					}
				}
			}
		}
	}
	catch (const std::exception& e) {

	}
}

void ClickGUI::onSaveConfig(void* confVoid) {
	json* currentConfig = reinterpret_cast<json*>(confVoid);
	try {
		json obj;
		for (Setting* setting : this->getSettingList()) {
			if (setting->type == SettingType::KEYBIND_S) {
				KeybindSetting* keySetting = (KeybindSetting*)setting;
				obj[keySetting->name] = *keySetting->value;
			}
			else if (setting->type == SettingType::BOOL_S) {
				BoolSetting* boolSetting = (BoolSetting*)setting;
				obj[boolSetting->name] = *boolSetting->value;
			}
			else if (setting->type == SettingType::ENUM_S) {
				EnumSetting* enumSetting = (EnumSetting*)setting;
				obj[enumSetting->name] = *enumSetting->value;
			}
			else if (setting->type == SettingType::COLOR_S) {
				ColorSetting* colorSetting = (ColorSetting*)setting;
				obj[colorSetting->name]["Red"] = colorSetting->colorPtr->r;
				obj[colorSetting->name]["Green"] = colorSetting->colorPtr->g;
				obj[colorSetting->name]["Blue"] = colorSetting->colorPtr->b;
				obj[colorSetting->name]["Alpha"] = colorSetting->colorPtr->a;
				obj[colorSetting->name]["Synced"] = colorSetting->colorSynced;
			}
			else if (setting->type == SettingType::SLIDER_S) {
				SliderSettingBase* sliderSetting = (SliderSettingBase*)setting;
				if (sliderSetting->valueType == ValueType::INT_T) {
					SliderSetting<int>* intSlider = (SliderSetting<int>*)sliderSetting;
					obj[intSlider->name] = *intSlider->valuePtr;
				}
				else if (sliderSetting->valueType == ValueType::FLOAT_T) {
					SliderSetting<float>* floatSlider = (SliderSetting<float>*)sliderSetting;
					obj[floatSlider->name] = *floatSlider->valuePtr;
				}
			}
		}

		for (auto& window : windowList) {
			obj[window->name]["isExtended"] = window->extended;
			obj[window->name]["pos"]["x"] = window->pos.x;
			obj[window->name]["pos"]["y"] = window->pos.y;
		}
		(*currentConfig)[this->getModuleName()] = obj;
	}
	catch (const std::exception& e) {

	}
}