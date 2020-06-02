#include "Engine.h"


//Engine* engine = new Engine();

Engine::Engine(HINSTANCE hInstance, int nCmdShow) :
	m_window(new Window(hInstance, L"class name", L"Window name", nCmdShow)),
	m_renderer(new Renderer()) {}
bool Engine::init() {
	// call init functions for all members in the correct order
	return m_renderer->init();
	//return true;
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
Window* Engine::get_window() {
	return m_window;
}