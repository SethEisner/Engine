#include "Window.h"

Window::Window(HINSTANCE hInstance, const wchar_t* class_name, const wchar_t* window_name) 
	: m_window_class({}), m_handle(NULL), m_width(1920), m_height(1080), m_hinstance(hInstance), m_class_name(class_name), m_window_name(window_name) {
	
}
Window::~Window() {} // doesnt need to do anything
bool Window::init() {
	/*
	m_window_class.lpfnWndProc = &WindowProc;
	m_window_class.hInstance = m_hinstance;
	m_window_class.lpszClassName = m_class_name;
	RegisterClass(&m_window_class);
	m_handle = CreateWindowEx(0, m_class_name, m_window_name, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height,
		NULL,       // Parent window    
		NULL,       // Menu
		m_hinstance,  // Instance handle
		this        // Additional application data
	);
	assert(m_handle);
	ShowWindow(m_handle, SW_SHOW);
	UpdateWindow(m_handle);
	return true;
	*/
	
	m_window_class.style = CS_HREDRAW | CS_VREDRAW;
	m_window_class.lpfnWndProc = WindowProc;
	m_window_class.cbClsExtra = 0;
	m_window_class.cbWndExtra = 0;
	m_window_class.hInstance = m_hinstance;
	m_window_class.hIcon = LoadIcon(0, IDI_APPLICATION);
	m_window_class.hCursor = LoadCursor(0, IDC_ARROW);
	m_window_class.hbrBackground = (HBRUSH)GetStockObject(NULL_BRUSH);
	m_window_class.lpszMenuName = 0;
	m_window_class.lpszClassName = m_class_name;
	if (!RegisterClass(&m_window_class)) {
		MessageBox(0, L"RegisterClass failed", 0, 0);
		return false;
	}

	// compute window rectangle dimesnsions based on requested client area dimensions
	RECT R = { 0, 0, m_width, m_height };
	AdjustWindowRect(&R, WS_OVERLAPPEDWINDOW, false);

	m_handle = CreateWindowEx(0, m_class_name, m_window_name, WS_OVERLAPPEDWINDOW, CW_USEDEFAULT, CW_USEDEFAULT, m_width, m_height,
		NULL,       // Parent window    
		NULL,       // Menu
		m_hinstance,  // Instance handle
		0			// Additional application data
	);
	if (!m_handle) {
		MessageBox(0, L"CreateWindow failed", 0, 0);
		return false;
	}
	ShowWindow(m_handle, SW_SHOW);
	UpdateWindow(m_handle);

	RECT r;
	GetWindowRect(m_handle, &r);
	assert(r.right - r.left == m_width);
	assert(r.bottom - r.top == m_height);
	//while (true) {}
	return true;
}

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
// HWND Window::get_handle() {
// 	return m_window_handle;
// }
// 
// size_t Window::get_width() {
// 	return m_width;
// }
// size_t Window::get_height() {
// 	return m_height;
// }