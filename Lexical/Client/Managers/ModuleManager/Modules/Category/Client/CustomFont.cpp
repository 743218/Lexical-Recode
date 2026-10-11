#include "CustomFont.h"

CustomFont::CustomFont() : Module("Fonts", "Font of Client", Category::CLIENT) {
	fontEnumSetting = (EnumSetting*)registerSetting(new EnumSetting("Font", "NULL", { "ProductSans", "Mojangles", "Comfortaa", "Poppins", "Verdana", "Tahoma", "Bahnschrift" }, &fontMode, 0));
	registerSetting(new SliderSetting<int>("FontSize", "NULL", &fontSize, 30, 25, 40));
	registerSetting(new BoolSetting("Shadow", "NULL", &shadow, false));
}

bool CustomFont::isEnabled() {
	return true;
}

bool CustomFont::isVisible() {
	return false;
}

std::string CustomFont::getSelectedFont() {
	return fontEnumSetting->enumList[fontMode];
}