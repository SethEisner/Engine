#pragma once
#include <assert.h>
#include <windows.h>

class Engine;

class Window {
public:
	Window(HINSTANCE hInstance, const wchar_t* class_name, const wchar_t* window_name);
	~Window();// {} // probably doesnt need to do anything
	bool init();
	//void on_resize(); //function that gets called when the window is resized
	float get_aspect_ratio();
	void toggle_mouse_capture();
	void update(float);
	LRESULT WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	//HWND get_handle();
	//size_t get_width();
	//size_t get_height();
	WNDCLASS m_window_class;
	HWND m_handle;
	size_t m_width;
	size_t m_height;
	HINSTANCE m_hinstance;
	const wchar_t* m_class_name;
	const wchar_t* m_window_name;
	float m_left;
	float m_top;
	float m_screen_x;
	float m_screen_y;
	bool m_mouse_captured;
	//bool m_window_initialized = false;
};