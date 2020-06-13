#pragma once
#include <vector>
#include "RenderItem.h"
#include "Object.h"
#include <assimp/scene.h>

class Engine;

class Scene {
public:
	Scene();
	~Scene();
	bool init();
	void update();
	void shutdown();
	void process_node(Node* current_node, Node* parent, aiNode* node, const aiScene* scene);
	std::string m_name;
	size_t m_hash_name;
	//std::vector<RenderItem>* m_items; // RenderItem contains the Mesh pointer and Material Pointer
	// start with a single mesh and material
	// MeshData* m_mesh;
	// Material* m_material;
	// eventually have a scene graph
	// eventually have a BVH
	//Object* m_object; // keep this for now so we compile
	Texture* m_textures; // stores the textures that we build from 
	Node* m_root;
	uint32_t m_texture_count;
};