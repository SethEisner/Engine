#include "Window.h"
#include "../Engine.h"

static bool window_initialized = false;

static LRESULT CALLBACK MainWindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam)
{
	auto window = engine->window;
	if (window) return window->WindowProc(hwnd, uMsg, wParam, lParam);
}

Window::Window(HINSTANCE hInstance, const wchar_t* class_name, const wchar_t* window_name)
	: m_window_class({}), m_handle(NULL), m_width(1920), m_height(1080), m_hinstance(hInstance), m_class_name(class_name), m_window_name(window_name) {
}
Window::~Window() {} // doesnt need to do anything
bool Window::init() {

	m_window_class.style = CS_HREDRAW | CS_VREDRAW;
	m_window_class.lpfnWndProc = MainWindowProc;
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
	SetCapture(m_handle);
	m_mouse_captured = true;
	//ClipCursor(&R);
	m_left = R.left;
	m_top = R.top;
	GetWindowRect(m_handle, &R);
	m_screen_x = R.left;
	m_screen_y = R.top;
	window_initialized = true;
	return true;
}

LRESULT Window::WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg) {
	case WM_DESTROY: {
		PostQuitMessage(0);
		exit(0);
	}

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

float Window::get_aspect_ratio() {
	return static_cast<float>(m_width) / m_height;
}
void Window::toggle_mouse_capture() {
	if (m_mouse_captured) ReleaseCapture();
	else SetCapture(m_handle);
	m_mouse_captured = !m_mouse_captured;
}
void Window::update(float delta) {
	static size_t frame_count = 0;
	static float time_elapsed = 0.0f;
	frame_count++;
	time_elapsed += delta;
	if (time_elapsed >= 1.0f){ // 
		float fps = frame_count;
		float mspf = 1000.0f / fps;
		std::wstring fps_str = std::to_wstring(fps);
		std::wstring mspf_str = std::to_wstring(mspf);
		std::wstring window_text = L"    fps: " + fps_str + L"   mspf: " + mspf_str;

		SetWindowTextW(m_handle, window_text.c_str());

		// Reset for next average.
		frame_count = 0;
		time_elapsed = 0.0f;
	}
	engine->camera->set_lens(get_aspect_ratio());
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