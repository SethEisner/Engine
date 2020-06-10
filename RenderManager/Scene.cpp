#include "Scene.h"
#include "../Engine.h"
#include "../ResourceManager/ResourceManager.h"

// Scene::Scene() : m_items(new std::vector<RenderItem>()) {}\
// Scene::~Scene() {
// 	delete m_items;
// }
Scene::Scene() : m_name(""), m_hash_name(0), m_mesh(new MeshData()) {} //, m_material(new Material()) {}
Scene::~Scene() {
	//delete m_material;
	delete m_mesh;
}
bool Scene::init() {
	// use the resource manager to load the items into memory
	// then build each RenderItem and add it to the vector
	std::string temp = "cat";
	engine->resource_manager->load_resource(temp + ".zip");
	while (!engine->resource_manager->resource_loaded(temp + ".zip")) {
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	const aiScene* scene = engine->resource_manager->get_scene_pointer(temp + ".zip/" + temp + ".dae");
	if (scene == nullptr) return false;

	m_mesh->init(scene->mMeshes[0]);

	return true;
}
void Scene::update() {
	// call the update method on each render item
}
void Scene::shutdown() {
	// tell the resource manager that we no longer need all the resources loaded
}