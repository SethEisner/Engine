#include "Engine.h"


//Engine* engine = new Engine();

Engine::Engine(HINSTANCE hInstance) :
	m_window(new Window(hInstance, L"class name", L"Window name")),
	m_renderer(new Renderer(m_window)) {}
bool Engine::init() {
	// call init functions for all members in the correct order
	if (!m_window->init()) return false;
	try {
		m_renderer->init();
	}
	catch (DxException& e) {
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		exit(e.error_code);
	}
	m_renderer->on_resize();
	return true;
}
void Engine::run() {
	MSG msg = { 0 };
	while (msg.message != WM_QUIT) {
		// if there are window messages then process them
		if (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) >= 0) {
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			//m_input_manager->get_input(msg); // call input manager with the message
		}
		m_renderer->update();
		m_renderer->draw();
	}
}
void Engine::shutdown() {

}
void Engine::update() {
	m_renderer->update();
}
Renderer* Engine::get_renderer() const {
	return m_renderer;
}
Window* Engine::get_window() const {
	if(this) return m_window;
}