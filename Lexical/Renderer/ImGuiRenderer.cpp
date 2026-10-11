#include "ImGuiRenderer.h"

#include "../Libs/imgui/imgui.h"
#include "../Libs/imgui/imgui_impl_dx11.h"
#include "../Libs/imgui/imgui_impl_win32.h"
#include "../Libs/imfx/imfx.h"

#include "../Assets/Fonts/Fonts.h"

#include "../Client/Client.h"
#include "../SDK/Game.h"

float ImGuiRenderer::deltaTime = 0.016f;
float ImGuiRenderer::fontSize = 22.f;
bool ImGuiRenderer::initialized = false;

static ImDrawList* currentDrawList = nullptr;

static ImU32 toImU32(const UIColor& color) {
	return IM_COL32(color.r, color.g, color.b, color.a);
}

static ImVec2 toImVec2(const Vec2<float>& pos) {
	return ImVec2(pos.x, pos.y);
}

bool ImGuiRenderer::Init(HWND window, ID3D11Device* device, ID3D11DeviceContext* context) {
	if (initialized)
		return true;

	// Same font loading as Radulasso (DirectXHook::presentCallback)
	if (ImGui::GetCurrentContext() == nullptr) {
		ImGui::CreateContext();
	}

	ImGuiIO& io = ImGui::GetIO();
	ImGui::StyleColorsDark();

	// Our TTF blobs are static arrays: ImGui must NOT take ownership of them,
	// otherwise DestroyContext() calls free() on static memory (heap corruption on eject).
	ImFontConfig fontCfg;
	fontCfg.FontDataOwnedByAtlas = false;

	io.Fonts->AddFontFromMemoryTTF((void*)ProductSansRegular, (int)ProductSansRegularSize, 50.f, &fontCfg);
	io.Fonts->AddFontFromMemoryTTF((void*)Mojangles, (int)MojanglesSize, 200.f, &fontCfg);
	io.Fonts->AddFontFromMemoryTTF((void*)Comfortaa, (int)ComfortaaSize, 200.f, &fontCfg);
	io.Fonts->AddFontFromMemoryTTF((void*)Poppins, (int)PoppinsSize, 50.f, &fontCfg);
	io.Fonts->AddFontFromMemoryTTF((void*)Verdana, (int)VerdanaSize, 50.f, &fontCfg);
	io.Fonts->AddFontFromMemoryTTF((void*)Tahoma, (int)TahomaSize, 50.f, &fontCfg);
	io.Fonts->AddFontFromMemoryTTF((void*)Bahnschrift, (int)BahnschriftSize, 50.f, &fontCfg);
	io.IniFilename = NULL;
	io.FontDefault = io.Fonts->Fonts[0];

	ImGui_ImplWin32_Init(window);
	ImGui_ImplDX11_Init(device, context);

	initialized = true;
	return true;
}

void ImGuiRenderer::NewFrame(ID3D11Device* device, IDXGISurface* backBuffer, float dpi, const Vec2<float>& displaySize) {
	if (!initialized)
		return;

	ImFX::NewFrame(device, backBuffer, dpi);
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame(ImVec2(displaySize.x, displaySize.y));
	ImGui::NewFrame();
}

void ImGuiRenderer::Render() {
	if (!initialized)
		return;

	ImDrawList* drawList = ImGui::GetBackgroundDrawList();
	currentDrawList = drawList;

	ImGuiIO& io = ImGui::GetIO();
	deltaTime = io.DeltaTime;

	// Font selection follows the CustomFont module (same 6 fonts as Radulasso)
	static CustomFont* customFontMod = nullptr;
	if (customFontMod == nullptr)
		customFontMod = ModuleManager::getModule<CustomFont>();

	if (customFontMod != nullptr && !io.Fonts->Fonts.empty()) {
		int selected = customFontMod->fontMode;
		if (selected < 0 || selected >= (int)io.Fonts->Fonts.size())
			selected = 0;
		io.FontDefault = io.Fonts->Fonts[selected];

		// Per-font base sizes, same as Radulasso's DirectXHook::handle,
		// scaled by the CustomFont size modifier (25 = unscaled)
		float sizeModifier = (float)customFontMod->fontSize / 25.f;
		fontSize = getBaseFontSize() * sizeModifier;
	}

	ModuleManager::onImGuiRender(drawList);
	NotificationManager::RenderImGui();

	{
		static ClickGUI* clickGuiMod = nullptr;
		if (clickGuiMod == nullptr)
			clickGuiMod = ModuleManager::getModule<ClickGUI>();
		if (clickGuiMod != nullptr && clickGuiMod->isEnabled())
			clickGuiMod->RenderImGui();
	}
}

void ImGuiRenderer::EndFrame(ID3D11DeviceContext* context, ID3D11RenderTargetView* renderTargetView) {
	if (!initialized)
		return;

	ImGui::EndFrame();
	ImGui::Render();

	context->OMSetRenderTargets(1, &renderTargetView, NULL);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());
	ImFX::EndFrame();

	currentDrawList = nullptr;
}

void ImGuiRenderer::Cleanup() {
	if (!initialized)
		return;

	ImFX::CleanupFX();
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	currentDrawList = nullptr;
	initialized = false;
}

void ImGuiRenderer::OnResize() {
	ImFX::CleanupFX();
}

Vec2<float> ImGuiRenderer::getWindowSize() {
	if (Game::clientInstance != nullptr && Game::clientInstance->guiData != nullptr)
		return Game::clientInstance->guiData->windowSizeReal;
	return Vec2<float>(0.f, 0.f);
}

float ImGuiRenderer::getGuiScale() {
	RECT desktopRect;
	const HWND desktop = GetDesktopWindow();
	GetClientRect(desktop, &desktopRect);

	Vec2<float> windowSize = getWindowSize();
	Vec2<float> baseSize((float)desktopRect.right, (float)desktopRect.bottom);
	if (baseSize.x <= 0.f || baseSize.y <= 0.f)
		return 1.f;

	return std::min(windowSize.x / baseSize.x, windowSize.y / baseSize.y);
}

float ImGuiRenderer::getBaseFontSize() {
	// Unscaled per-font base size (CustomFont size modifier not applied)
	return 22.f;
}

void ImGuiRenderer::setDrawList(ImDrawList* drawList) {
	currentDrawList = drawList;
}

void ImGuiRenderer::drawText(const Vec2<float>& textPos, const std::string& textStr, const UIColor& color, float textSize, bool) {
	if (currentDrawList == nullptr)
		return;

	float guiScale = getGuiScale();
	float scaledSize = (fontSize * textSize) * guiScale;

	static CustomFont* customFontMod = nullptr;
	if (customFontMod == nullptr)
		customFontMod = ModuleManager::getModule<CustomFont>();

	if (customFontMod != nullptr && customFontMod->shadow) {
		currentDrawList->AddText(nullptr, scaledSize, toImVec2(Vec2<float>(textPos.x + 1.1f * guiScale, textPos.y + 1.1f * guiScale)), toImU32(UIColor(0, 0, 0, color.a)), textStr.c_str());
	}

	currentDrawList->AddText(nullptr, scaledSize, toImVec2(textPos), toImU32(color), textStr.c_str());
}

float ImGuiRenderer::getTextWidth(const std::string& textStr, float textSize, bool) {
	float guiScale = getGuiScale();
	float scaledSize = (fontSize * textSize) * guiScale;

	ImFont* font = ImGui::GetFont();
	if (font == nullptr)
		return 0.f;

	return font->CalcTextSizeA(scaledSize, FLT_MAX, -1.f, textStr.c_str()).x;
}

float ImGuiRenderer::getTextHeight(const std::string& textStr, float textSize, bool) {
	float guiScale = getGuiScale();
	float scaledSize = (fontSize * textSize) * guiScale;

	ImFont* font = ImGui::GetFont();
	if (font == nullptr)
		return 0.f;

	const char* text = textStr.empty() ? "A" : textStr.c_str();
	return font->CalcTextSizeA(scaledSize, FLT_MAX, -1.f, text).y;
}

void ImGuiRenderer::drawLine(const Vec2<float>& startPos, const Vec2<float>& endPos, const UIColor& color, float width) {
	if (currentDrawList == nullptr)
		return;
	currentDrawList->AddLine(toImVec2(startPos), toImVec2(endPos), toImU32(color), width);
}

void ImGuiRenderer::drawRectangle(const Vec4<float>& rect, const UIColor& color, float width) {
	if (currentDrawList == nullptr)
		return;
	currentDrawList->AddRect(ImVec2(rect.x, rect.y), ImVec2(rect.z, rect.w), toImU32(color), 0.f, 0, width);
}

void ImGuiRenderer::fillRectangle(const Vec4<float>& rect, const UIColor& color, float rounding) {
	if (currentDrawList == nullptr)
		return;
	currentDrawList->AddRectFilled(ImVec2(rect.x, rect.y), ImVec2(rect.z, rect.w), toImU32(color), rounding * getGuiScale(), 0);
}

void ImGuiRenderer::drawCircle(const Vec2<float>& centerPos, const UIColor& color, float radius, float width) {
	if (currentDrawList == nullptr)
		return;
	currentDrawList->AddCircle(toImVec2(centerPos), radius, toImU32(color), 0, width);
}

void ImGuiRenderer::fillCircle(const Vec2<float>& centerPos, const UIColor& color, float radius) {
	if (currentDrawList == nullptr)
		return;
	currentDrawList->AddCircleFilled(toImVec2(centerPos), radius, toImU32(color), 0);
}

void ImGuiRenderer::addBlur(const Vec4<float>& rect, float strength, bool) {
	if (currentDrawList == nullptr)
		return;
	ImFX::Begin(currentDrawList);
	ImFX::AddBlur(strength, ImVec4(rect.x, rect.y, rect.z, rect.w));
	ImFX::End();
}
