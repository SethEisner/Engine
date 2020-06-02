#pragma once
#include <assert.h>
#include <windows.h>

class Window {
public:
	static LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam);
	Window(HINSTANCE hInstance, const wchar_t* class_name, const wchar_t* window_name, int nCmdShow);
	~Window();// {} // probably doesnt need to do anything
	void on_resize(); //function that gets called when the window is resized
	float get_aspect_ratio();
	HWND get_window_handle();
	size_t get_window_width();
	size_t get_window_height();
private:
	WNDCLASS m_window_class;
	HWND m_window_handle;
	size_t m_width;
	size_t m_height;
};