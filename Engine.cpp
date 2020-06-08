#include "Engine.h"
#include "RenderManager/Renderer.h"
#include "ResourceManager/ResourceManager.h"
#include "InputManager/InputManager.h"
#include "Memory/MemoryManager.h"
#include "RenderManager/Window.h"
#include "RenderManager/Timer.h"
#include "RenderManager/Camera.h"
#include "RenderManager/Scene.h"

Engine* engine = new Engine();

bool Engine::init(HINSTANCE hInstance) {
	// constructors should be called in the order below. added an extra linebreak where order is dependent
	window = new Window(hInstance, L"class name", L"Window name");
	if (!window->init()) return false;

	resource_manager = new ResourceManager();
	global_timer = new Timer();
	input_manager = new InputManager();
	camera = new Camera();

	scene = new Scene();
	if (!scene->init()) return false;

	renderer = new Renderer();	
	try {
		renderer->init();
	}
	catch (DxException& e) {
		MessageBox(nullptr, e.ToString().c_str(), L"HR Failed", MB_OK);
		exit(e.error_code);
	}
	renderer->on_resize();
	return true;
}
void Engine::run() {
	MSG msg = { 0 };
	while (true) {
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
			if (msg.message == WM_QUIT) return;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			switch (msg.message) {
				// add size case
			}
			input_manager->get_input(msg); // call input manager with the message so it can process it
		}
		update();
		renderer->draw();
	}
}
void Engine::shutdown() {
	delete renderer;
	delete scene;
	delete resource_manager;
	delete input_manager;
	delete global_timer;
	delete camera;
	delete window;
	delete engine;
}
void Engine::update() {
	global_timer->tick();
	renderer->update();
}

