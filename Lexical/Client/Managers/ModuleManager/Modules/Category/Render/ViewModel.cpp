#include "ViewModel.h"

ViewModel::ViewModel() : Module("ViewModel", "Custom item view model", Category::RENDER) {
	registerSetting(new BoolSetting("Reset", "Reset all settings to default", &Reset, false));
	registerSetting(new BoolSetting("Fancy Eat", "No, this isnt no eat bob", &fancyEat, false));

	registerSetting(new Vec3Setting<float>("M-Hand Translate", "NULL", &mhTrans, mhTrans, Vec3<float>(-2.f, -2.f, -2.f), Vec3<float>(2.f, 2.f, 2.f)));
	registerSetting(new Vec3Setting<float>("M-Hand Scale", "NULL", &mhScale, mhScale, Vec3<float>(-2.f, -2.f, -2.f), Vec3<float>(2.f, 2.f, 2.f)));

	registerSetting(new Vec3Setting<float>("O-Hand Translate", "NULL", &ohTrans, ohTrans, Vec3<float>(-2.f, -2.f, -2.f), Vec3<float>(2.f, 2.f, 2.f)));
	registerSetting(new Vec3Setting<float>("O-Hand Scale", "NULL", &ohScale, ohScale, Vec3<float>(-2.f, -2.f, -2.f), Vec3<float>(2.f, 2.f, 2.f)));
}

void ViewModel::onClientTick() {
	if (Reset) {
		mhTrans = Vec3<float>(0.f, 0.f, 0.f);
		mhScale = Vec3<float>(1.f, 1.f, 1.f);

		ohTrans = Vec3<float>(0.f, 0.f, 0.f);
		ohScale = Vec3<float>(1.f, 1.f, 1.f);

		Reset = false;
	}
}