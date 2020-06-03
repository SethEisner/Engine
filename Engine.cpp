#include "Engine.h"


//Engine* engine = new Engine();

Engine::Engine(HINSTANCE hInstance) :
	// m_window(new Window(hInstance, L"class name", L"Window name")),
	m_renderer(new Renderer(hInstance)) {}
bool Engine::init() {
	// call init functions for all members in the correct order
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
	bool run_flag = true;
	while (run_flag) {
		m_renderer->draw();
		//run_flag = !run_flag;
	}
}
void Engine::shutdown() {

}