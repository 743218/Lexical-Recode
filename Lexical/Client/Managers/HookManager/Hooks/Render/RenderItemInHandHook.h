#pragma once
#include "../FuncHook.h"

class RenderItemInHandHook : public FuncHook {
private:
	using func_t = void(__thiscall*)(__int64, BaseActorRenderContext*, uint8_t);
	static inline func_t oFunc;

	static void ItemInHandRenderer_renderCallback(__int64 _this, BaseActorRenderContext* a2, uint8_t a3) {
		static ViewModel* viewModelMod = ModuleManager::getModule<ViewModel>();
		viewModelMod->oldMatrix = a2->screenContext->camera->matrixStack.stack.top();
		if (viewModelMod->isEnabled()) {
			
			if (!(Game::isRightClickDown() && Game::getLocalPlayer()->getItemUseDuration() > 0 && viewModelMod->fancyEat)) {
				MatrixStack& matStackPtr = a2->screenContext->camera->matrixStack;
				glm::mat4x4 newMat = viewModelMod->oldMatrix;

				newMat = glm::translate<float>(newMat, viewModelMod->mhTrans.toGlm());
				newMat = glm::scale<float>(newMat, viewModelMod->mhScale.toGlm());

				matStackPtr.stack.top() = newMat;
			}
		}

		oFunc(_this, a2, a3);
	}
public:
	RenderItemInHandHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&ItemInHandRenderer_renderCallback;
	}
};