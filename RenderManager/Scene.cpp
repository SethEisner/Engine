#include "Scene.h"
#include "../Engine.h"
#include "../ResourceManager/ResourceManager.h"
#include "Renderer.h"
#include "../Gameplay/GameObject.h"
#include "../Collision/CollisionEngine.h"
#include "../Collision/CollisionObject.h"

// Scene::Scene() : m_items(new std::vector<RenderItem>()) {}\
// Scene::~Scene() {
// 	delete m_items;
// }
Scene::Scene() : m_name(""), m_hash_name(0) {}//, m_object(new Object()) {}//m_mesh(new MeshData()), m_material(new Material()) {} //, m_material(new Material()) {}
Scene::~Scene() {
	//delete m_material;
	//delete m_object;
	//delete m_root;
}
static size_t mesh_counter = 0;

DirectX::XMFLOAT4X4 assimp_matrix_to_directx(aiNode* ai_node) {
	aiMatrix4x4 temp = ai_node->mTransformation;// .Transpose(); // DirectX uses column matrices
	// shouldnt need transpose here because we use the convert to left handed flag
	return DirectX::XMFLOAT4X4(temp.a1, temp.a2, temp.a3, temp.a4,
		temp.b1, temp.b2, temp.b3, temp.b4,
		temp.c1, temp.c2, temp.c3, temp.c4,
		temp.d1, temp.d2, temp.d3, temp.d4);
	// make the node transform be the multiplication of the current node transform and the parent node so it is in world space
}

void Scene::process_node(const DirectX::XMMATRIX& parent_transform, aiNode* ai_node, const aiScene* scene, Mesh* mesh) {
	// only need to set the transform and the primitive type of submesh using the mesh index
	DirectX::XMMATRIX temp_transform = DirectX::XMLoadFloat4x4(&assimp_matrix_to_directx(ai_node));
	// make the node transform be the multiplication of the current node transform and the parent node so it is in world space
	// DirectX::XMMATRIX transform = DirectX::XMMatrixMultiply(temp_transform, parent_transform);
	DirectX::XMMATRIX transform = DirectX::XMMatrixMultiply(parent_transform, temp_transform);

	// if (ai_node->mNumMeshes == 0) { // the node has no meshes, so we should skip this
	// 	if (ai_node->mNumChildren == 0) return; // does this mesh do anything? can get here if the node is for a camera, should be okay to ignore for now
	// 	for (size_t i = 0; i != ai_node->mNumChildren; ++i) { // process all the child node and give our transform because we are their parent
	// 		process_node(transform, ai_node->mChildren[i], scene, mesh);
	// 		return;
	// 	}
	// 	return;
	// }

	D3D_PRIMITIVE_TOPOLOGY primitive;

	// for each mesh in our node, we need to set the primitive type and the transform we calculated
	for (size_t i = 0; i != ai_node->mNumMeshes; ++i) { // possible that we reference the same mesh more than once... which results in the transform being messed up
		// for each ai_mesh in the node, create a new SubMesh and add it to the draw_args
		SubMesh sub_mesh = {};

		uint32_t scene_mesh_index = ai_node->mMeshes[i];

		sub_mesh.m_index_count = mesh->m_submesh_helper[scene_mesh_index].m_index_count;
		sub_mesh.m_vertex_count = mesh->m_submesh_helper[scene_mesh_index].m_vertex_count;
		sub_mesh.m_base_vertex = mesh->m_submesh_helper[scene_mesh_index].m_base_vertex;
		sub_mesh.m_start_index = mesh->m_submesh_helper[scene_mesh_index].m_start_index;
		DirectX::XMStoreFloat4x4(&sub_mesh.m_transform, transform);
		// DirectX::XMStoreFloat4x4(&sub_mesh.m_transform, XMMatrixTranspose(transform));
		switch (scene->mMeshes[scene_mesh_index]->mPrimitiveTypes) {
		case aiPrimitiveType_LINE:
			primitive = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
			break;
		case aiPrimitiveType_TRIANGLE:
			primitive = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
			break;
		default:
			assert(false && "mesh contains an unsupported primitive topology");
		}
		sub_mesh.m_primitive = primitive;

		std::string name = "";
		std::string node_name(ai_node->mName.C_Str());
		std::string mesh_name(scene->mMeshes[scene_mesh_index]->mName.C_Str());
		name = node_name + "_" + mesh_name;
		mesh->m_submeshes.emplace_back(sub_mesh); // copies the sub_mesh into the unordered map
	}
	for (size_t i = 0; i != ai_node->mNumChildren; ++i) { // process all the child node and give our transform because we are their parent
		process_node(transform, ai_node->mChildren[i], scene, mesh);
	}
}
void Scene::create_mesh(const aiScene* scene, Mesh* mesh) {
	mesh->m_mesh_id = mesh_counter++;
	std::vector<Vertex> vertices;
	std::vector<uint16_t> indices;

	size_t base_vertex = vertices.size(); // size is zero because we have not added anything
	size_t start_index = indices.size();
	for (size_t i = 0; i != scene->mNumMeshes; ++i) { // for each aiMesh
		size_t num_vertices = 0;
		for (size_t j = 0; j != scene->mMeshes[i]->mNumVertices; ++j) { // for each vertex in an aiMesh
			Vertex temp;
			float pos[3] = { scene->mMeshes[i]->mVertices[j].x, scene->mMeshes[i]->mVertices[j].y, scene->mMeshes[i]->mVertices[j].z };
			float norm[3] = { scene->mMeshes[i]->mNormals[j].x, scene->mMeshes[i]->mNormals[j].y, scene->mMeshes[i]->mNormals[j].z };
			float tex[2] = { scene->mMeshes[i]->mTextureCoords[0][j].x, scene->mMeshes[i]->mTextureCoords[0][j].y };
			float tang[3] = { scene->mMeshes[i]->mTangents[j].x, scene->mMeshes[i]->mTangents[j].y, scene->mMeshes[i]->mTangents[j].z };
			float bitang[3] = { scene->mMeshes[i]->mBitangents[j].x, scene->mMeshes[i]->mBitangents[j].y, scene->mMeshes[i]->mBitangents[j].z };
			temp.m_pos = DirectX::XMFLOAT3(pos);
			temp.m_normal = DirectX::XMFLOAT3(norm);
			temp.m_tangent = DirectX::XMFLOAT3(tang);
			temp.m_bitangent = DirectX::XMFLOAT3(bitang);
			temp.m_text_coord = DirectX::XMFLOAT2(tex);

			num_vertices++;
			vertices.push_back(std::move(temp));
		}
		size_t num_indices = 0;
		for (size_t j = 0; j != scene->mMeshes[i]->mNumFaces; ++j) { // for each face in an aiMesh
			for (size_t k = 0; k != scene->mMeshes[i]->mFaces[j].mNumIndices; ++k) { // for each index in the face
				indices.push_back(scene->mMeshes[i]->mFaces[j].mIndices[k]);
				num_indices++;
			}
		}
		SubMeshBufferData temp_sub_mesh = { };
		temp_sub_mesh.m_index_count = num_indices;
		temp_sub_mesh.m_vertex_count = num_vertices;
		temp_sub_mesh.m_base_vertex = base_vertex;
		temp_sub_mesh.m_start_index = start_index;
		// will set the primitive topology and the transform whhile traversing the node hierarchy
		//mesh->m_sub_mesh_helper[i] = temp_sub_mesh;
		mesh->m_submesh_helper.emplace_back(temp_sub_mesh); // can emplace back, because i will be the index that it is placed at, making it a much faster map
		base_vertex = vertices.size();
		start_index = indices.size();
	}
	mesh->m_vertex_count = vertices.size();
	mesh->m_index_count = indices.size();
	XMStoreFloat4x4(&mesh->m_transform, DirectX::XMMatrixIdentity());
	// mesh->m_transform = assimp_matrix_to_directx(scene->mRootNode);
	const size_t vb_byte_size = vertices.size() * sizeof(Vertex);
	const size_t ib_byte_size = indices.size() * sizeof(uint16_t);

	engine->renderer->reset_command_list(0);

	ThrowIfFailed(D3DCreateBlob(vb_byte_size, &mesh->m_vertex_buffer_cpu));
	CopyMemory(mesh->m_vertex_buffer_cpu->GetBufferPointer(), vertices.data(), vb_byte_size);
	ThrowIfFailed(D3DCreateBlob(ib_byte_size, &mesh->m_index_buffer_cpu));
	CopyMemory(mesh->m_index_buffer_cpu->GetBufferPointer(), indices.data(), ib_byte_size);
	mesh->m_vertex_buffer_gpu = create_default_buffer(engine->renderer->get_device().Get(), engine->renderer->get_command_list(0).Get(), vertices.data(), vb_byte_size, mesh->m_vertex_buffer_uploader);
	mesh->m_index_buffer_gpu = create_default_buffer(engine->renderer->get_device().Get(), engine->renderer->get_command_list(0).Get(), indices.data(), ib_byte_size, mesh->m_index_buffer_uploader);
	

	engine->renderer->close_command_list(0);

	mesh->m_vertex_stride = sizeof(Vertex);
	mesh->m_vertex_buffer_size = vb_byte_size;
	mesh->m_index_format = DXGI_FORMAT_R16_UINT;
	mesh->m_index_buffer_size = ib_byte_size;
	// for (size_t i = 0; i != scene->mRootNode->mNumChildren; ++i) { // for each child of the root node...
	// 	process_node(DirectX::XMMatrixIdentity(), scene->mRootNode->mChildren[i], scene, m_floor->m_mesh); // build the submeshes based on how assimp has them, because we need their unique transforms
	// }
	process_node(XMLoadFloat4x4(&mesh->m_transform), scene->mRootNode, scene, mesh);
}
bool Scene::init() {
	// use the resource manager to load the items into memory
	// then build each RenderItem and add it to the vector
	//std::string name = "Sword";
	std::string name = "floor";
	std::string zip_file = name + ".zip";
	engine->resource_manager->load_resource(zip_file);
	std::string crate = "crate";
	std::string zip_file1 = "crate.zip";
	engine->resource_manager->load_resource(zip_file1);
	//engine->resource_manager->load_resource("Sword.zip");
	while (!engine->resource_manager->resource_loaded(zip_file)) {}
	while (!engine->resource_manager->resource_loaded(zip_file1)) {}
	const aiScene* scene;
	
	scene = engine->resource_manager->get_scene_pointer(zip_file + "/" + name + ".dae");

	// m_object = new Object(scene->mNumMeshes);
	// 
	// for (size_t i = 0; i != m_object->m_mesh_count; ++i) { // init every mesh in the scene to the object
	// 	m_object->m_sub_meshes[i].init(scene->mMeshes[i]);
	// }

	m_floor = new GameObject();
	// XMStoreFloat4x4(&m_floor->m_transform, DirectX::XMMatrixIdentity());
	m_floor->m_position = { 0.0f, -1.0f, 1.0f };
	m_floor->add_mesh(new Mesh());
	
	create_mesh(scene, m_floor->m_mesh);
	


	// m_root = new Node();

	

	// should eventually read in a file that has all this data in a nice format. maybe a json file callend scene
	// build the textures in the scene, or tell the renderer about them

	// replace 0 with the thread id to index into the array of command lists
	engine->renderer->reset_command_list(0);
	// std::string tex_name = "checker";
	// std::string filename = "floor.zip/checkerboard.dds";
	// engine->renderer->create_and_add_texture(tex_name, filename, 0, *m_mesh, TextureFlags::COLOR);
	std::string tex_name = "base_color";
	std::string filename = zip_file + "/checkerboard.dds";
	engine->renderer->create_and_add_texture(tex_name, filename, 0, m_floor->m_mesh, TextureFlags::COLOR);


	// tex_name = "sword_normal";
	// filename = zip_file + "/Sting_Normal.dds";
	// engine->renderer->create_and_add_texture(tex_name, filename, 0, *m_mesh, TextureFlags::NORMAL);
	// 
	// tex_name = "sword_roughness";
	// filename = zip_file + "/Sting_Roughness.dds";
	// engine->renderer->create_and_add_texture(tex_name, filename, 0, *m_mesh, TextureFlags::ROUGHNESS);
	// 
	// tex_name = "sword_metalness";
	// filename = zip_file + "/Sting_Roughness.dds";
	// engine->renderer->create_and_add_texture(tex_name, filename, 0, *m_mesh, TextureFlags::METALLIC);
	// 
	// tex_name = "sword_height";
	// filename = zip_file + "/Sting_Height.dds";
	// engine->renderer->create_and_add_texture(tex_name, filename, 0, *m_mesh, TextureFlags::HEIGHT);
	
	engine->renderer->close_command_list(0);


	engine->renderer->add_mesh(m_floor->m_mesh); // add mesh at the end because we bind the textures it uses in this function
	// CollisionObject* coll_obj = new CollisionObject(m_floor, m_floor->m_mesh);
	m_floor->add_collision_object(new CollisionObject(m_floor, m_floor->m_mesh));
	m_floor->m_collision_object->m_body->set_inverse_mass(0.0f); // set the inverse mass of the floor to be zero so it has infinite mass
	// m_floor->m_collision_object->m_body->set_mass(1.0f);
	m_floor->m_collision_object->m_body->set_acceleration({ 0.0f, 0.0f, 0.0f });
	m_floor->m_collision_object->m_body->set_linear_damping(1.0f);
	m_floor->m_collision_object->m_body->set_velocity({ 0.0f,0.0f,0.0f });
	m_floor->m_collision_object->m_body->set_position(m_floor->m_position); // set the position of the rigid body to be that of the game object
	// m_floor->m_collision_object->m_body->set_transform(&m_floor->m_mesh->m_transform); //rigidbody should have same transform as mesh as it takes us from Model space to world space
	engine->collision_engine->add_object(m_floor->m_collision_object);

	
	engine->renderer->reset_command_list(0);
	
	
	m_crate = new GameObject();
	// XMStoreFloat4x4(&m_crate->m_transform, DirectX::XMMatrixIdentity());
	m_crate->m_position = { 0.0f, 5.0f, 0.0f };
	m_crate->add_mesh(new Mesh());
	while (!engine->resource_manager->resource_loaded(zip_file1)) {}
	scene = engine->resource_manager->get_scene_pointer(zip_file1 + "/" + crate + ".dae");
	create_mesh(scene, m_crate->m_mesh);
	// std::string filename = "crate.zip/crate_diffuse.dds";
	engine->renderer->create_and_add_texture("crate_color", "floor.zip/checkerboard.dds" /*"crate.zip/crate_diffuse.dds"*/, 0, m_crate->m_mesh, TextureFlags::COLOR);
	engine->renderer->close_command_list(0);
	engine->renderer->add_mesh(m_crate->m_mesh);
	m_crate->add_collision_object(new CollisionObject(m_crate, m_crate->m_mesh));
	m_crate->m_collision_object->m_body->set_mass(1.0f);
	m_crate->m_collision_object->m_body->set_acceleration({ 0.0f, 0.0f, 0.0f });
	m_crate->m_collision_object->m_body->set_linear_damping(1.0f);
	m_crate->m_collision_object->m_body->set_velocity({ 0.0f,0.0f,0.0f });
	m_crate->m_collision_object->m_body->set_position(m_crate->m_position);
	engine->collision_engine->add_object(m_crate->m_collision_object);
	
	return true;
}
void Scene::update(double duration) {
	// call the update method on each GameObject
	m_floor->update(duration);
	m_crate->update(duration);
}
void Scene::shutdown() {
	// tell the resource manager that we no longer need all the resources loaded
}