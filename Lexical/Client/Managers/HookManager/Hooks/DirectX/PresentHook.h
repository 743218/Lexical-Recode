#pragma once
#include "../FuncHook.h"
#include "../../../../../Renderer/D2D.h"
#include "../../../../../Renderer/ImGuiRenderer.h"
#include "../../../../Client.h"
#include <Windows.h>

class PresentHook : public FuncHook {
private:
	using present_t = HRESULT(__thiscall*)(IDXGISwapChain3*, UINT, UINT);
	static inline present_t oPresent;

	static HRESULT presentCallback(IDXGISwapChain3* swapChain, UINT syncInterval, UINT flags) {

		if (!Client::isInitialized())
			return oPresent(swapChain, syncInterval, flags);

		// Initial and Update DeltaTime
		{
			static bool initDeltaTime = false;
			static std::chrono::steady_clock::time_point lastTime;
			static std::chrono::steady_clock::time_point currentTime;

			currentTime = std::chrono::steady_clock::now();

			if (initDeltaTime)
				D2D::deltaTime = std::chrono::duration_cast<std::chrono::milliseconds>(currentTime - lastTime).count() / 1000.0f;

			lastTime = currentTime;
			initDeltaTime = true;
		}

		static HWND window = (HWND)FindWindowA(nullptr, (LPCSTR)"Minecraft");

		ID3D12Device* d3d12Device = nullptr;
		ID3D11Device* d3d11Device = nullptr;

		if (SUCCEEDED(swapChain->GetDevice(IID_PPV_ARGS(&d3d12Device)))) {
			static_cast<ID3D12Device5*>(d3d12Device)->RemoveDevice();
			return oPresent(swapChain, syncInterval, flags);
		}
		else if (SUCCEEDED(swapChain->GetDevice(IID_PPV_ARGS(&d3d11Device)))) {

			// D2D pass (HUD, ESP, eject screen, ...)
			D2D::NewFrame(swapChain, d3d11Device, (float)GetDpiForWindow(window));
			D2D::Render();
			D2D::EndFrame();

			// Eject may have been triggered inside D2D::Render (Ctrl+L hold):
			// skip the ImGui pass, hooks are already disabled.
			if (!Client::isInitialized()) {
				d3d11Device->Release();
				return oPresent(swapChain, syncInterval, flags);
			}

			// ImGui pass (ClickGUI, notifications, onImGuiRender)
			ID3D11DeviceContext* d3d11Context = nullptr;
			ID3D11Texture2D* backBufferTexture = nullptr;
			IDXGISurface* dxgiBackBuffer = nullptr;
			ID3D11RenderTargetView* renderTargetView = nullptr;

			d3d11Device->GetImmediateContext(&d3d11Context);
			swapChain->GetBuffer(0, IID_PPV_ARGS(&backBufferTexture));
			swapChain->GetBuffer(0, IID_PPV_ARGS(&dxgiBackBuffer));
			d3d11Device->CreateRenderTargetView(backBufferTexture, NULL, &renderTargetView);

			if (!ImGuiRenderer::initialized) {
				ImGuiRenderer::Init(window, d3d11Device, d3d11Context);
			}

			if (ImGuiRenderer::initialized && Game::clientInstance != nullptr && Game::clientInstance->guiData != nullptr) {
				ImGuiRenderer::NewFrame(d3d11Device, dxgiBackBuffer, (float)GetDpiForWindow(window), Game::clientInstance->guiData->windowSizeReal);
				if (backBufferTexture != nullptr) {
					backBufferTexture->Release();
					backBufferTexture = nullptr;
				}
				ImGuiRenderer::Render();
				ImGuiRenderer::EndFrame(d3d11Context, renderTargetView);
				// ImFX::EndFrame() consumed our dxgiBackBuffer reference (it Releases the
				// surface ImFX::NewFrame was given): ownership transferred, do not release below.
				dxgiBackBuffer = nullptr;
				d3d11Context->Flush();
			}

			if (backBufferTexture != nullptr)
				backBufferTexture->Release();
			if (dxgiBackBuffer != nullptr)
				dxgiBackBuffer->Release();
			if (renderTargetView != nullptr)
				renderTargetView->Release();
			if (d3d11Context != nullptr)
				d3d11Context->Release();

			d3d11Device->Release();
		}

		return oPresent(swapChain, syncInterval, flags);
	}
public:
	PresentHook() {
		OriginFunc = (void*)&oPresent;
		func = (void*)&presentCallback;
	}
};
