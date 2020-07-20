#pragma once
#include <Windows.h>
#include "RenderManager/d3dUtil.h"
// #include "RenderManager/Renderer.h"
// #include "InputManager/InputManager.h"
// #include "Memory/MemoryManager.h"
// #include "RenderManager/Window.h"
// #include "RenderManager/Timer.h"

class Renderer;
class ResourceManager;
class Window;
class InputManager;
class Timer;
class Camera;
class Scene;
class CollisionEngine;

//namespace Engine {
class Engine {
public:
	// bool init(HINSTANCE hInstance);
	// void run();
	// void update();
	// void shutdown();
	Engine() = default;
	~Engine() = default;
	bool init(HINSTANCE hInstance);
	void run();
	void shutdown();
	void update();

	Window* window;
	InputManager* input_manager;
	ResourceManager* resource_manager;
	Renderer* renderer;
	Timer* global_timer;
	Camera* camera;
	Scene* scene;
	CollisionEngine* collision_engine;
};

extern Engine* engine;
// Window* Engine::window;
// InputManager* Engine::input_manager;
// Renderer* Engine::renderer;
// Timer* Engine::global_timer;
