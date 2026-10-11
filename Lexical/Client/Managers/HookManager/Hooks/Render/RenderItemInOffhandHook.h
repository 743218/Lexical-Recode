#pragma once
#include "../FuncHook.h"

class RenderItemInOffhandHook : public FuncHook {
private:
	using func_t = void(__thiscall*)(__int64, BaseActorRenderContext*, Player*, uint8_t);
	static inline func_t oFunc;

	static void ItemInHandRenderer_renderOffhandCallback(__int64 _this, BaseActorRenderContext* a2, Player* a3, uint8_t a4) {
		static ViewModel* viewModelMod = ModuleManager::getModule<ViewModel>();
		if (viewModelMod->isEnabled()) {

			MatrixStack& matStackPtr = a2->screenContext->camera->matrixStack;
			glm::mat4x4 newMat = viewModelMod->oldMatrix;

			newMat = glm::translate<float>(newMat, viewModelMod->ohTrans.toGlm());
			newMat = glm::scale<float>(newMat, viewModelMod->ohScale.toGlm());

			matStackPtr.stack.top() = newMat;
		}

		oFunc(_this, a2, a3, a4);
	}
public:
	RenderItemInOffhandHook() {
		OriginFunc = (void*)&oFunc;
		func = (void*)&ItemInHandRenderer_renderOffhandCallback;
	}
};