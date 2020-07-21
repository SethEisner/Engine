#pragma once
#include <vector>
#include "RenderItem.h"
// #include "Object.h"
#include <assimp/scene.h>
#include "../Gameplay/GameObject.h"
class Engine;

class Scene {
public:
	Scene();
	~Scene();
	bool init();
	void update(double duration);
	void shutdown();
	void process_node(const DirectX::XMMATRIX& parent_transform, aiNode* node, const aiScene* scene, Mesh* mesh);
	void create_mesh(const aiScene* scene, Mesh* mesh);
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
	//Mesh* m_mesh; // eventually have more than one mesh for the scene (turn into a vector for easy adding)
	GameObject* m_floor;
	GameObject* m_crate;
	uint32_t m_mesh_count = 1; // assume only one mesh for now
	// Node* not needed, build the scene hierarchy a different way
	// Node* m_root; // contains the SubMeshes we use to index into the Mesh 
	uint32_t m_texture_count;
};