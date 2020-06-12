#pragma once
//#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
#include "d3dutil.h"
// could make vertex data contain contiguous arrays
class VertexData {
public:
	VertexData() = default;
	VertexData(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& norm, const DirectX::XMFLOAT3& tangent, const DirectX::XMFLOAT2& tex_coord) :
		m_position(pos), m_normal(norm), /*m_tangent(tangent), */m_tex_coord(tex_coord) {}
	VertexData( float px, float py, float pz,
				float nx, float ny, float nz,
				/*float tx, float ty, float tz,*/
				float u, float v) :
		m_position(px, py, pz), m_normal(nx, ny, nz), /*m_tangent(tx, ty, tz), */m_tex_coord (u, v) {}
	DirectX::XMFLOAT3 m_position;
	DirectX::XMFLOAT3 m_normal;
	//DirectX::XMFLOAT3 m_tangent;
	DirectX::XMFLOAT2 m_tex_coord;
};
class SubMeshData {
public:
	SubMeshData() : m_vertices(std::vector<VertexData>()), m_indecis(std::vector<uint16_t>()) {}
	std::string m_name;
	std::vector<VertexData> m_vertices;
	std::vector<uint16_t> m_indecis;
	uint32_t m_material_index;
	D3D_PRIMITIVE_TOPOLOGY m_primitive;
	// transform for a certain mesh is relative to the space of the parent
	DirectX::XMFLOAT4X4 m_mesh_transform; // transformation matrix for the node.
	void init(const aiMesh* mesh);
};


class Object { // keep it as object, should rename later
public:
	SubMeshData* m_sub_meshes = nullptr;
	//MaterialConstants* m_materials = nullptr;
	const uint32_t m_mesh_count;
	//const uint32_t m_material_count;
	Object() = delete;
	// Object(uint32_t mesh_count, uint32_t material_count) : m_mesh_count(mesh_count), m_material_count(material_count) {
	// 	m_sub_meshes = new SubMeshData[m_mesh_count];
	// 	m_materials = new MaterialConstants[m_material_count];
	// }
	// ~Object() {
	// 	if (m_sub_meshes > 0) delete m_sub_meshes;
	// 	if (m_materials > 0) delete m_materials;
	// }
	Object(uint32_t mesh_count) : m_mesh_count(mesh_count){
		m_sub_meshes = new SubMeshData[m_mesh_count];

	}
	~Object() {
		if (m_sub_meshes > 0) delete m_sub_meshes;
	}
};
// scene has a node hierarchy
class Node {
public:
	Node* m_parent;
	Node* m_children; // child nodes within the node
	uint32_t m_children_count; // number of children at each node
	DirectX::XMMATRIX m_node_transform; // world transform for the node
	Object* m_object; // meshes within the node each node contains 
	// uint32_t m_objects_count; // number of objects within the node
};

