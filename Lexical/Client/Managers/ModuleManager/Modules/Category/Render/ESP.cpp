#include "ESP.h"

ESP::ESP() : Module("ESP", "Highlights player/entity", Category::RENDER) {
	registerSetting(new ColorSetting("Color", "NULL", &color, UIColor(255, 255, 255), false));
	registerSetting(new SliderSetting<int>("Alpha", "NULL", &alpha, 40, 0, 255));
	registerSetting(new SliderSetting<int>("LineAlpha", "NULL", &lineAlpha, 135, 0, 255));
	registerSetting(new BoolSetting("Mobs", "NULL", &mobs, false));
	registerSetting(new BoolSetting("Items", "NULL", &items, false));
}

void ESP::onLevelRender() {
	LocalPlayer* localPlayer = Game::getLocalPlayer();
	for (auto& entity : localPlayer->level->getRuntimeActorList()) {
		uint32_t entityId = entity->getActorTypeComponent()->id;
		if (TargetUtil::isTargetValid(entity, mobs) || (items && entityId == 64)) {
			Vec3<float> entityEyePos = entity->getEyePos();
			AABBShapeComponent* entityAABBShape = entity->aabbShape;

			AABB aabbRender;
			float lowerOffsetY = 0.f;
			if (entityId == 319)
				lowerOffsetY = 1.6f;
			if (entity->getActorTypeComponent()->id != 319) {
				aabbRender.lower = entityEyePos.sub(Vec3<float>(entityAABBShape->width / 2.f, lowerOffsetY, entityAABBShape->width / 2.f));
				aabbRender.upper = aabbRender.lower.add(Vec3<float>(entityAABBShape->width, entityAABBShape->height, entityAABBShape->width));
			}
			else {
				aabbRender.lower = entityEyePos.sub(Vec3<float>(0.6f / 2.f, lowerOffsetY, 0.6f / 2.f));
				aabbRender.upper = aabbRender.lower.add(Vec3<float>(0.6f, entityAABBShape->height, 0.6f));
			}
			MCR::drawBox3dFilled(aabbRender, UIColor(color.r, color.g, color.b, alpha), UIColor(color.r, color.g, color.b, lineAlpha));
		}
	}
}