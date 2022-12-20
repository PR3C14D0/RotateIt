#include <iostream>
#include <Windows.h>
#include "Core.h";

using namespace std;

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
bool quit = false;

int WINAPI WinMain(HINSTANCE hInstance, HINSTANCE hPrevInstance, LPSTR lpCmdLine, int nShowCmd) {
	const wchar_t* CLASS_NAME = L"RotateIt";

#ifndef NDEBUG
	AllocConsole();
	FILE* f;
	freopen_s(&f, "CONOUT$", "w", stdout);
#endif

	WNDCLASS wc = { };
	wc.hInstance = hInstance;
	wc.lpszClassName = CLASS_NAME;
	wc.lpfnWndProc = WndProc;

	RegisterClass(&wc);

	HWND hwnd = CreateWindowEx(
		NULL,
		CLASS_NAME,
		L"Rotate It",
		WS_OVERLAPPEDWINDOW,
		CW_USEDEFAULT, CW_USEDEFAULT,
		CW_USEDEFAULT, CW_USEDEFAULT,
		NULL, NULL, hInstance, NULL
	);

	if (hwnd == NULL)
		return 1;
	
	ShowWindow(hwnd, nShowCmd);


	Core* core = new Core(hwnd);

	MSG msg = { };
	while (!quit) {
		if (PeekMessage(&msg, NULL, NULL, NULL, PM_REMOVE)) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
		}
		core->MainLoop();
	}

	core->Cleanup();
	
	return 0;
}

extern IMGUI_IMPL_API LRESULT ImGui_ImplWin32_WndProcHandler(HWND hWnd, UINT msg, WPARAM wParam, LPARAM lParam);

LRESULT CALLBACK WndProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	if (ImGui_ImplWin32_WndProcHandler(hwnd, uMsg, wParam, lParam))
		return 0;

	switch (uMsg) {
	case WM_CLOSE:
		if (MessageBox(hwnd, L"Are you sure you want to close?", L"Confirmation", MB_OKCANCEL) == IDOK)
			DestroyWindow(hwnd);
		return 0;
	case WM_DESTROY:
		quit = true;
		PostQuitMessage(0);
		return 0;
	}

	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}