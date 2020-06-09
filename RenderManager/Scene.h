#pragma once
#include <vector>
#include "RenderItem.h"
#include "Geometry.h"

class Engine;

class Scene {
public:
	Scene();
	~Scene();
	bool init();
	void update();
	void shutdown();
	std::string m_name;
	size_t m_hash_name;
	//std::vector<RenderItem>* m_items; // RenderItem contains the Mesh pointer and Material Pointer
	// start with a single mesh and material
	MeshData* m_mesh;
	//Material* m_material;
	// eventually have a scene graph
	// eventually have a BVH
};