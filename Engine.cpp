#include "Engine.h"
#include "RenderManager/Renderer.h"
#include "ResourceManager/ResourceManager.h"
#include "InputManager/InputManager.h"
#include "Memory/MemoryManager.h"
#include "RenderManager/Window.h"
#include "RenderManager/Timer.h"
#include "RenderManager/Camera.h"
#include "RenderManager/Scene.h"
#include "Collision/CollisionEngine.h"

Engine* engine = new Engine();

bool Engine::init(HINSTANCE hInstance) {
	// constructors should be called in the order below. added an extra linebreak where order is dependent
	window = new Window(hInstance, L"class name", L"Window name");
	if (!window->init()) return false;

	resource_manager = new ResourceManager();
	global_timer = new Timer();
	input_manager = new InputManager();
	input_manager->init();
	camera = new Camera();
	// camera position can be set anywhere
	camera->set_position(0.0f, 3.0f, -10.0f);

	renderer = new Renderer();
	collision_engine = new CollisionEngine();
	scene = new Scene();

	renderer->init();
	renderer->on_resize();
	scene->init();

	global_timer->tick();
	return true;
}
void Engine::run() {
	MSG msg = { 0 };
	while (true) {
		input_manager->update(); // update the input manager states
		while (PeekMessage(&msg, NULL, 0, 0, PM_REMOVE) > 0) {
			if (msg.message == WM_QUIT) return;
			TranslateMessage(&msg);
			DispatchMessage(&msg);
			if (msg.message == WM_KEYDOWN && msg.wParam == VK_ESCAPE) {
				engine->window->toggle_mouse_capture();
				global_timer->toggle();
			}
			input_manager->get_input(msg); // call input manager with the message so it can process it
		}
		if (global_timer->running()) {
			
			update();
			renderer->draw();
		}
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
	if (input_manager->is_released(HASH("jump"))) {
		OutputDebugStringA("released\n");
	}
	if (input_manager->is_held(HASH("jump"))) {
		OutputDebugStringA("held\n");
	}
	if (input_manager->is_pressed(HASH("jump"))) {
		OutputDebugStringA("pressed\n");
	}
	if (input_manager->is_unheld(HASH("jump"))) {
		OutputDebugStringA("unheld\n");
	}
	global_timer->tick();
	double duration = global_timer->delta_time();
	window->update(duration);
	camera->update(duration);
	scene->update(duration);
	collision_engine->update(duration);
	renderer->update();
}