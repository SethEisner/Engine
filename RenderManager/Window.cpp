#include "Window.h"

Window::Window(HINSTANCE hInstance, const wchar_t* class_name, const wchar_t* window_name, int nCmdShow) : m_window_class({}), m_window_handle(), m_width(1920), m_height(1080) {
	m_window_class.lpfnWndProc = &WindowProc;
	m_window_class.hInstance = hInstance;
	m_window_class.lpszClassName = class_name;
	RegisterClass(&m_window_class);
	m_window_handle = CreateWindowEx(0, class_name, window_name/*L"Engine"*/, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height,
		NULL,       // Parent window    
		NULL,       // Menu
		hInstance,  // Instance handle
		this        // Additional application data
	);
	assert(m_window_handle);
	ShowWindow(m_window_handle, nCmdShow);
	UpdateWindow(m_window_handle);
}
Window::~Window() {} // doesnt need to do anything

LRESULT CALLBACK Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		exit(0);
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

		EndPaint(hwnd, &ps);
	}
	return 0;

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

float Window::get_aspect_ratio() {
	return static_cast<float>(m_width) / m_height;
}
HWND Window::get_window_handle() {
	return m_window_handle;
}

size_t Window::get_window_width() {
	return m_width;
}
size_t Window::get_window_height() {
	return m_height;
}