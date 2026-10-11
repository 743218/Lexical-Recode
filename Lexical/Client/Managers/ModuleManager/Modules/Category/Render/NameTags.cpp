#include "NameTags.h"

NameTags::NameTags() : Module("NameTags", "Better nametags", Category::RENDER) {
	registerSetting(new EnumSetting("Mode", "NULL", { "None", "Underline", "Outline" }, &renderMode, 2));
	registerSetting(new SliderSetting<int>("Opacity", "NULL", &opacity, 125, 0, 255));
	registerSetting(new BoolSetting("Armor", "Display armor", &displayArmor, true));
	registerSetting(new BoolSetting("Self", "Render nametags for local player", &self, true));
	registerSetting(new BoolSetting("Mobs", "Render nametags for mobs", &mobTags, false));
	registerSetting(new BoolSetting("Items", "Render nametags for items", &itemTags, false));
	registerSetting(new ColorSetting("Rect Color", "NULL", &rectColor, rectColor));
}

void NameTags::Render() {
	if (!Game::canUseMoveKeys())
		return;

	if (Game::clientInstance == nullptr)
		return;

	if (Game::clientInstance->getLevelRenderer() == nullptr)
		return;

	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (localPlayer == nullptr)
		return;

	Level* level = localPlayer->level;
	if (level == nullptr)
		return;

	Vec3<float> origin = MCR::origin;

	NameTagsTransform transform;

	const auto actorList = level->getRuntimeActorList();

	const size_t maxTags = 256;
	size_t rendered = 0;

	std::unordered_map<uint32_t, int> newFrames;
	newFrames.reserve(actorList.size());

	for (const auto& entity : actorList) {
		if (rendered >= maxTags)
			break;

		if (entity == nullptr)
			continue;

		auto* actorType = entity->getActorTypeComponent();
		if (actorType == nullptr)
			continue;

		uint32_t entityId = actorType->id;

		bool isTarget = false;
		if (self && entity == localPlayer)
			isTarget = true;
		else if (itemTags && entityId == 64)
			isTarget = true;
		else if (TargetUtil::isTargetValid(entity, mobTags, false))
			isTarget = true;

		if (!isTarget)
			continue;

		uint32_t uniqueId = entity->getRuntimeIDComponent()->runtimeId.id;

		int frames = 0;
		auto it = targetValidFrames.find(uniqueId);
		if (it != targetValidFrames.end())
			frames = it->second;

		if (frames < REQUIRED_VALID_FRAMES)
			frames++;

		newFrames[uniqueId] = frames;

		const bool canDrawItems = (frames >= REQUIRED_VALID_FRAMES);

		Vec3<float> entityPos = entity->getEyePos();

		if (entityId != 319 && entity->aabbShape != nullptr)
			entityPos.y += entity->aabbShape->height;

		const float renderOffsetY = 0.75f;

		Vec2<float> textPos;
		if (!MCR::worldToScreen(entityPos.add(Vec3<float>(0.f, renderOffsetY, 0.f)), textPos))
			continue;

		float dist = origin.dist(entityPos);
		if (dist <= 0.f)
			continue;

		float textSize = fmax(0.5f, fmin(2.f, 3.f / dist));
		float textPaddingX = 2.f * textSize;
		float textPaddingY = 1.f * textSize;

		std::string entityName = WorldUtil::getEntityNameTags(entity);
		if (entityName.empty())
			continue;

		float textWidth = MCR::getTextWidth(entityName, textSize);
		float textHeight = 9.f * textSize;

		textPos.x -= textWidth / 2.f;
		textPos.y -= textHeight / 2.f;

		Vec4<float> rectPos = Vec4<float>(textPos.x - textPaddingX,
			textPos.y - textPaddingY,
			textPos.x + textWidth + textPaddingX,
			textPos.y + textHeight + textPaddingY);

		bool shouldRender = true;
		if (entity == localPlayer && Game::viewPerspectiveMode == 0)
			shouldRender = false;

		if (!shouldRender)
			continue;

		NameTagRenderData renderData;
		renderData.screenPos = Vec2<float>(textPos.x + (textWidth / 2.f), rectPos.y);
		renderData.text = entityName;
		renderData.scale = textSize;
		transform.renderList.push_back(renderData);

		rendered++;

		if (displayArmor && canDrawItems) {
			float scale = 0.85f * textSize;
			float spacing = 15.f * scale;

			Vec2<float> armorHudPos = Vec2<float>((rectPos.x + rectPos.z) / 2.f, rectPos.y - 17.f * scale);

			ItemStack* armorItemStack[6] = { nullptr, nullptr, nullptr, nullptr, nullptr, nullptr };

			ItemStack* offhandItem = entity->getOffhandSlot();
			if (offhandItem != nullptr && offhandItem->isValid()) {
				armorItemStack[0] = offhandItem;
				armorHudPos.x -= (spacing / 2.f);
			}

			for (int i = 0; i < 4; i++) {
				ItemStack* stack = entity->getArmor(i);
				if (stack != nullptr && stack->isValid()) {
					armorItemStack[i + 1] = stack;
					armorHudPos.x -= (spacing / 2.f);
				}
			}

			ItemStack* holdingItem = entity->getCarriedItem();
			if (holdingItem != nullptr && holdingItem->isValid()) {
				armorItemStack[5] = holdingItem;
				armorHudPos.x -= (spacing / 2.f);
			}

			for (int i = 0; i < 6; i++) {
				if (armorItemStack[i] == nullptr)
					continue;

				uint8_t itemCount = armorItemStack[i]->mCount;
				if (itemCount == 0)
					continue;

				MCR::drawItem(armorHudPos, armorItemStack[i], scale, true);

				if (itemCount > 1) {
					Vec2<float> itemCountPos = Vec2<float>(armorHudPos.x + (17.f * scale), armorHudPos.y + (8.f * scale));
					std::string itemCountText = std::to_string(itemCount);
					itemCountPos.x -= MCR::getTextWidth(itemCountText, scale);
					MCR::drawText(itemCountPos, itemCountText, UIColor(255, 255, 255), scale);
				}

				armorHudPos.x += spacing;
			}
		}
	}

	targetValidFrames = std::move(newFrames);

	transformList.push(std::move(transform));

	while (transformList.size() > 4)
		transformList.pop();
}

void NameTags::RenderD2D() {
	if (!Game::canUseMoveKeys())
		return;

	if (Game::clientInstance == nullptr)
		return;

	if (Game::clientInstance->guiData == nullptr)
		return;

	LocalPlayer* localPlayer = Game::getLocalPlayer();
	if (localPlayer == nullptr)
		return;

	Vec2<float> windowSize = Game::clientInstance->guiData->windowSizeReal;
	Vec2<float> windowScaled = Game::clientInstance->guiData->windowSizeScaled;

	if (windowScaled.x <= 0.f || windowScaled.y <= 0.f)
		return;

	float scaleX = windowSize.x / windowScaled.x;
	float scaleY = windowSize.y / windowScaled.y;

	NameTagsTransform currentTransform;
	if (!transformList.empty()) {
		currentTransform = transformList.front();
		transformList.pop();
	}

	if (currentTransform.renderList.empty())
		return;

	for (const auto& renderData : currentTransform.renderList) {
		float textSize = renderData.scale;
		if (textSize <= 0.f)
			continue;

		const std::string& entityName = renderData.text;
		if (entityName.empty())
			continue;

		float textWidth = D2D::getTextWidth(entityName, textSize);
		float textHeight = D2D::getTextHeight(entityName, textSize);

		float textPaddingX = 3.f * textSize;
		float textPaddingY = 0.f * textSize;

		Vec2<float> renderPos;
		renderPos.x = renderData.screenPos.x * scaleX;
		renderPos.y = renderData.screenPos.y * scaleY;

		Vec4<float> rectPos = Vec4<float>(renderPos.x - (textWidth / 2.f) - textPaddingX,
			renderPos.y,
			renderPos.x + (textWidth / 2.f) + textPaddingX,
			renderPos.y + textHeight + (textPaddingY * 2.f));

		Vec4<float> underlineRect = Vec4<float>(rectPos.x, rectPos.w, rectPos.z, rectPos.w + 2.f * textSize);

		Vec2<float> textPos = Vec2<float>(rectPos.x + textPaddingX, rectPos.y + textPaddingY);

		D2D::fillRectangle(rectPos, UIColor(0, 0, 0, opacity));

		if (renderMode == 1)
			D2D::fillRectangle(underlineRect, rectColor);
		else if (renderMode == 2)
			D2D::drawRectangle(rectPos, rectColor, 1.f * textSize);

		if (FriendManager::isInList(entityName))
			D2D::drawText(textPos, entityName, UIColor(0, 255, 255), textSize);
		else
			D2D::drawText(textPos, entityName, UIColor(255, 255, 255), textSize);
	}
}