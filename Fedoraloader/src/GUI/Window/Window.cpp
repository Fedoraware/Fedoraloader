#include "Window.h"

#include <ImGui/imgui.h>
#include <ImGui/imgui_impl_win32.h>
#include <ImGui/imgui_impl_dx11.h>

#include <d3d11.h>
#include <stdexcept>

// Window
WNDCLASSEX g_WindowClass;
HWND g_WindowHandle;
UINT g_ResizeWidth = 0, g_ResizeHeight = 0;
POINTS g_DragPos = {};
constexpr int WINDOW_WIDTH = 400, WINDOW_HEIGHT = 250;

// DirectX
ID3D11Device* g_Device = nullptr;
ID3D11DeviceContext* g_DeviceContext = nullptr;
IDXGISwapChain* g_SwapChain = nullptr;
ID3D11RenderTargetView* g_RenderTargetView = nullptr;

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT WINAPI WndProc(HWND hWnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	// Pass messages to ImGui
	if (ImGui_ImplWin32_WndProcHandler(hWnd, uMsg, wParam, lParam))
	{
		return true;
	}

	switch (uMsg)
	{
	case WM_SIZE:
		if (wParam != SIZE_MINIMIZED)
		{
			g_ResizeWidth = LOWORD(lParam);
			g_ResizeHeight = HIWORD(lParam);
		}
		return false;

	case WM_CLOSE:
		Window::IsRunning = false;
		DestroyWindow(hWnd);
		return false;

	case WM_DESTROY:
		PostQuitMessage(0);
		return false;

	case WM_LBUTTONDOWN:
		g_DragPos = MAKEPOINTS(lParam);
		SetCapture(g_WindowHandle);
		return false;

	case WM_LBUTTONUP:
		ReleaseCapture();
		return false;

	case WM_MOUSEMOVE:
		if (wParam == MK_LBUTTON)
		{
			const auto points = MAKEPOINTS(lParam);
			RECT rect = {};

			GetWindowRect(g_WindowHandle, &rect);

			rect.left += points.x - g_DragPos.x;
			rect.top += points.y - g_DragPos.y;

			SetWindowPos(
				g_WindowHandle,
					HWND_TOPMOST,
					rect.left,
					rect.top,
					0, 0,
					SWP_SHOWWINDOW | SWP_NOSIZE | SWP_NOZORDER
				);
		}
		return 0;
	}

	return DefWindowProc(hWnd, uMsg, wParam, lParam);
}

/* DirectX Backend */
void CreateRenderTarget()
{
	ID3D11Texture2D* pBackBuffer;
	g_SwapChain->GetBuffer(0, IID_PPV_ARGS(&pBackBuffer));
	g_Device->CreateRenderTargetView(pBackBuffer, nullptr, &g_RenderTargetView);
	pBackBuffer->Release();
}

bool CreateDeviceD3D(HWND hWnd)
{
	// Setup swap chain
	DXGI_SWAP_CHAIN_DESC sd;
	ZeroMemory(&sd, sizeof(sd));
	sd.BufferCount = 2;
	sd.BufferDesc.Width = 0;
	sd.BufferDesc.Height = 0;
	sd.BufferDesc.Format = DXGI_FORMAT_R8G8B8A8_UNORM;
	sd.BufferDesc.RefreshRate.Numerator = 60;
	sd.BufferDesc.RefreshRate.Denominator = 1;
	sd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	sd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	sd.OutputWindow = hWnd;
	sd.SampleDesc.Count = 1;
	sd.SampleDesc.Quality = 0;
	sd.Windowed = TRUE;
	sd.SwapEffect = DXGI_SWAP_EFFECT_DISCARD;

	constexpr UINT createDeviceFlags = 0;
	//createDeviceFlags |= D3D11_CREATE_DEVICE_DEBUG;
	D3D_FEATURE_LEVEL featureLevel;
	constexpr D3D_FEATURE_LEVEL featureLevelArray[2] = {D3D_FEATURE_LEVEL_11_0, D3D_FEATURE_LEVEL_10_0,};
	HRESULT res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_HARDWARE, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_SwapChain, &g_Device, &featureLevel,
	                                            &g_DeviceContext);

	if (res == DXGI_ERROR_UNSUPPORTED) // Try high-performance WARP software driver if hardware is not available.
	{
		res = D3D11CreateDeviceAndSwapChain(nullptr, D3D_DRIVER_TYPE_WARP, nullptr, createDeviceFlags, featureLevelArray, 2, D3D11_SDK_VERSION, &sd, &g_SwapChain, &g_Device, &featureLevel, &g_DeviceContext);
	}

	if (res != S_OK)
	{
		return false;
	}

	CreateRenderTarget();
	return true;
}

void CleanupRenderTarget()
{
	if (!g_RenderTargetView) { return; }

	g_RenderTargetView->Release();
	g_RenderTargetView = nullptr;
}

void CleanupDeviceD3D()
{
	CleanupRenderTarget();

	if (g_SwapChain)
	{
		g_SwapChain->Release();
		g_SwapChain = nullptr;
	}
	if (g_DeviceContext)
	{
		g_DeviceContext->Release();
		g_DeviceContext = nullptr;
	}
	if (g_Device)
	{
		g_Device->Release();
		g_Device = nullptr;
	}
}

/* ImGui */
void SetupImGui(HWND hWnd)
{
	// Setup Dear ImGui context
	IMGUI_CHECKVERSION();
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();
	io.IniFilename = nullptr;

	io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard; // Enable Keyboard Controls
	io.ConfigFlags |= ImGuiConfigFlags_NavEnableGamepad; // Enable Gamepad Controls

	// Setup Dear ImGui style
	ImGui::StyleColorsDark();
	//ImGui::StyleColorsLight();

	// Setup Platform/Renderer backends
	ImGui_ImplWin32_Init(hWnd);
	ImGui_ImplDX11_Init(g_Device, g_DeviceContext);
}

/* Global functions */
void Window::Create()
{
	// Initialize the window
	g_WindowClass = {
		.cbSize = sizeof g_WindowClass,
		.style = CS_HREDRAW | CS_VREDRAW,
		.lpfnWndProc = WndProc,
		.cbClsExtra = 0,
		.cbWndExtra = 0,
		.hInstance = GetModuleHandle(nullptr),
		.hIcon = nullptr,
		.hCursor = LoadCursor(nullptr, IDC_ARROW),
		.hbrBackground = CreateSolidBrush(RGB(0, 0, 0)),
		.lpszMenuName = nullptr,
		.lpszClassName = "fl001",
		.hIconSm = nullptr
	};

	if (!RegisterClassEx(&g_WindowClass))
	{
		throw std::runtime_error("Failed to register window class");
	}

	// Create the window
	g_WindowHandle = CreateWindowEx(
		0,
		g_WindowClass.lpszClassName,
		"Fedoraloader",
		WS_POPUP,
		200, 200,
		WINDOW_WIDTH, WINDOW_HEIGHT,
		nullptr,
		nullptr,
		g_WindowClass.hInstance,
		nullptr
	);

	if (g_WindowHandle == nullptr || g_WindowHandle == INVALID_HANDLE_VALUE)
	{
		throw std::runtime_error("Failed to create window");
	}

	// Init D3D
	if (!CreateDeviceD3D(g_WindowHandle))
	{
		DestroyWindow(g_WindowHandle);
		UnregisterClass(g_WindowClass.lpszClassName, g_WindowClass.hInstance);
		CleanupDeviceD3D();
		throw std::runtime_error("Failed to create D3D device");
	}

	// Init ImGui
	SetupImGui(g_WindowHandle);

	// Show the window
	ShowWindow(g_WindowHandle, SW_SHOWDEFAULT);
	UpdateWindow(g_WindowHandle);
}

void Window::Destroy()
{
	// Cleanup ImGui / D3D
	ImGui_ImplDX11_Shutdown();
	ImGui_ImplWin32_Shutdown();
	ImGui::DestroyContext();

	CleanupDeviceD3D();

	// Destroy Window
	DestroyWindow(g_WindowHandle);
	UnregisterClass(g_WindowClass.lpszClassName, g_WindowClass.hInstance);
}

void Window::BeginFrame()
{
	// Handle window messages
	MSG windowMsg;
	while (PeekMessage(&windowMsg, g_WindowHandle, 0, 0, PM_REMOVE))
	{
		TranslateMessage(&windowMsg);
		DispatchMessage(&windowMsg);

		if (windowMsg.message == WM_QUIT)
		{
			IsRunning = false;
			return;
		}
	}

	// Handle window resize
	if (g_ResizeWidth != 0 && g_ResizeHeight != 0)
	{
		CleanupRenderTarget();
		g_SwapChain->ResizeBuffers(0, g_ResizeWidth, g_ResizeHeight, DXGI_FORMAT_UNKNOWN, 0);
		g_ResizeWidth = g_ResizeHeight = 0;
		CreateRenderTarget();
	}

	// Start the Dear ImGui frame
	ImGui_ImplDX11_NewFrame();
	ImGui_ImplWin32_NewFrame();
	ImGui::NewFrame();
}

void Window::EndFrame()
{
	// Rendering
	ImGui::Render();
	constexpr float clearColor[4] = { 0.f, 0.f, 0.f, 0.f };
	g_DeviceContext->OMSetRenderTargets(1, &g_RenderTargetView, nullptr);
	g_DeviceContext->ClearRenderTargetView(g_RenderTargetView, clearColor);
	ImGui_ImplDX11_RenderDrawData(ImGui::GetDrawData());

	g_SwapChain->Present(1, 0); // Present with vsync
	//g_SwapChain->Present(0, 0); // Present without vsync
}
