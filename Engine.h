#pragma once
#include "RenderManager/Renderer.h"
#include "InputManager/InputManager.h"
#include "Memory/MemoryManager.h"

//class Renderer;

class Engine {
public:
	Engine() = delete;
	explicit Engine(HINSTANCE hInstance);
	~Engine() = default;
	bool init();
	void run(); // calls the main game loop
	void shutdown(); // called before the destructor
	//void on_resize(); // function that gets called when the window is resized. calls the on_resize function for the Window and Renderer objects
private:
	// maybe turn into static globals like we currently have for the render manager
	Renderer* m_renderer;
	// InputManager* m_input_manager;
	// jobsystem has no object, just a namespace
	// MemoryManager* m_memory_manager;
};

extern Engine* engine;