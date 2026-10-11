#pragma once
#include <d3d11.h>
#include <dxgi1_4.h>

#include <string>

#include "../Utils/Maths.h"
#include "../Utils/ColorUtil.h"

struct ImDrawList;
struct ImFont;

namespace ImGuiRenderer {
	extern float deltaTime;
	extern float fontSize;
	extern bool initialized;

	bool Init(HWND window, ID3D11Device* device, ID3D11DeviceContext* context);
	void NewFrame(ID3D11Device* device, IDXGISurface* backBuffer, float dpi, const Vec2<float>& displaySize);
	void Render();
	void EndFrame(ID3D11DeviceContext* context, ID3D11RenderTargetView* renderTargetView);
	void Cleanup();
	void OnResize();

	Vec2<float> getWindowSize();
	float getGuiScale();
	float getBaseFontSize();

	void setDrawList(ImDrawList* drawList);
	void drawText(const Vec2<float>& textPos, const std::string& textStr, const UIColor& color, float textSize = 1.f, bool /*storeTextLayout*/ = true);
	float getTextWidth(const std::string& textStr, float textSize = 1.f, bool /*storeTextLayout*/ = true);
	float getTextHeight(const std::string& textStr, float textSize = 1.f, bool /*storeTextLayout*/ = true);
	void drawLine(const Vec2<float>& startPos, const Vec2<float>& endPos, const UIColor& color, float width = 1.f);
	void drawRectangle(const Vec4<float>& rect, const UIColor& color, float width = 1.f);
	void fillRectangle(const Vec4<float>& rect, const UIColor& color, float rounding = 0.f);
	void drawCircle(const Vec2<float>& centerPos, const UIColor& color, float radius, float width = 1.f);
	void fillCircle(const Vec2<float>& centerPos, const UIColor& color, float radius);
	void addBlur(const Vec4<float>& rect, float strength, bool /*flush*/ = true);
}
