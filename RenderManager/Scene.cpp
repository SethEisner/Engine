#include "Scene.h"
#include "../Engine.h"
#include "../ResourceManager/ResourceManager.h"

// Scene::Scene() : m_items(new std::vector<RenderItem>()) {}\
// Scene::~Scene() {
// 	delete m_items;
// }
Scene::Scene() : m_name(""), m_hash_name(0) {}//, m_object(new Object()) {}//m_mesh(new MeshData()), m_material(new Material()) {} //, m_material(new Material()) {}
Scene::~Scene() {
	//delete m_material;
	//delete m_object;
	delete m_root;
}

void Scene::process_node(Node* current_node, Node* parent, aiNode* ai_node, const aiScene* scene) {
	// fill out the current_node members from the aiNode
	current_node->m_parent = parent;
	current_node->m_children_count = ai_node->mNumChildren;
	current_node->m_children = new Node[current_node->m_children_count];
	aiMatrix4x4 temp = ai_node->mTransformation.Transpose(); // transpose because we are using directx
	DirectX::XMMATRIX temp_transform = DirectX::XMLoadFloat4x4(&DirectX::XMFLOAT4X4(temp.a1, temp.a2, temp.a3, temp.a4,
															 temp.b1, temp.b2, temp.b3, temp.b4,
															 temp.c1, temp.c2, temp.c3, temp.c4,
															 temp.d1, temp.d2, temp.d3, temp.d4));
	// make the node transform be the multiplication of the current node transform and the parent node so it is in world space
	DirectX::XMMATRIX parent_transform = DirectX::XMMatrixIdentity();
	if(parent) {
		parent_transform = parent->m_node_transform;
	}
	current_node->m_node_transform = DirectX::XMMatrixMultiply(temp_transform, parent_transform);
	//current_node->m_node_transform = DirectX::XMMatrixMultiply(temp_transform, DirectX::XMLoadFloat4x4(&parent->m_node_transform));
	// create an object that holds zero or more meshes
	current_node->m_object = new Object(ai_node->mNumMeshes);
	for (size_t i = 0; i != current_node->m_object->m_mesh_count; ++i) {
		uint32_t scene_mesh_index = ai_node->mMeshes[i]; // each mesh has an index into the scene structure array of meshes
		// process the mesh...
		current_node->m_object->m_sub_meshes[i].init(scene->mMeshes[scene_mesh_index]);
		//current_node->m_object[i].init(scene->mMeshes[i]);
	}
	for (size_t i = 0; i != current_node->m_children_count; ++i) {
		process_node((current_node->m_children + i), current_node, ai_node->mChildren[i], scene);
		//m_object->m_sub_meshes[i].init(scene->mMeshes[i]);
	}
	// and then call this function recursively on the child nodes
	return;
}

bool Scene::init() {
	// use the resource manager to load the items into memory
	// then build each RenderItem and add it to the vector
	std::string name = "Sword";
	std::string zip_file = name + ".zip";
	engine->resource_manager->load_resource(zip_file);
	while (!engine->resource_manager->resource_loaded(zip_file)) { // wait until the zipfile is loaded
		//std::this_thread::sleep_for(std::chrono::milliseconds(1));
	}
	const aiScene* scene = engine->resource_manager->get_scene_pointer(zip_file + "/" + name + ".dae");

	// m_object = new Object(scene->mNumMeshes);
	// 
	// for (size_t i = 0; i != m_object->m_mesh_count; ++i) { // init every mesh in the scene to the object
	// 	m_object->m_sub_meshes[i].init(scene->mMeshes[i]);
	// }

	m_root = new Node();
	process_node(m_root, nullptr, scene->mRootNode, scene);

	// should eventually read in a file that has all this data in a nice format. maybe a json file callend scene
	// build the textures in the scene, or tell the renderer about them
	m_texture_count = 1;
	m_textures = new Texture[m_texture_count]; // make an array of one texture, just use the color texture for now
	m_textures[0].m_name = "base_color";
	m_textures[0].m_filename = zip_file + "/Sting_Base_Color.dds"; // use the name to look up into the resource manager and get the data pointer
	// do just the color now as that will be the simplest to implement in the shader

	
	return true;
}
void Scene::update() {
	// call the update method on each render item
}
void Scene::shutdown() {
	// tell the resource manager that we no longer need all the resources loaded
}