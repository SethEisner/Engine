#include "Scene.h"
#include "../Engine.h"
#include "../ResourceManager/ResourceManager.h"

Scene::Scene() : m_items(new std::vector<RenderItem>()) {}\
Scene::~Scene() {
	delete m_items;
}
bool Scene::init() {
	// use the resource manager to load the items into memory
	// then build each RenderItem and add it to the vector

	engine->resource_manager->load_resource("cat.zip");
	while (!engine->resource_manager->resource_loaded("cat.zip")) {
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	const aiScene* cat = engine->resource_manager->get_scene_pointer("cat.zip/cat.dae");
	return true;
}
void Scene::update() {
	// call the update method on each render item
}
void Scene::shutdown() {
	// tell the resource manager that we no longer need all the resources loaded
}