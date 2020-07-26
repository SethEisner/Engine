#include "CollisionObject.h"
#include "RigidBody.h" // for RigidBody
#include "BroadCollide.h" // for AABB
#include "NarrowCollide.h" // for OBB
#include "../Gameplay/GameObject.h"
#include <DirectXCollision.h>
#include <D3DCommon.h>
#include "../RenderManager/d3dUtil.h" // for Mesh
#include "../RenderManager/RenderItem.h"
CollisionObject::CollisionObject(GameObject* game_object, Mesh* mesh) : 
		m_game_object(game_object), 
		m_body(nullptr), 
		m_obb_count(mesh->m_submeshes.size()), 
		m_oriented_boxes(new OrientedBoundingBox[m_obb_count]), 
		m_box(new BoundingBox(game_object)) {
	// give the mesh we want to create a collision object for
	assert(mesh); // we need the mesh to already be created before we try to create a collision object for it
	
	// create the OBBs
	for (size_t i = 0; i != m_obb_count; ++i) {
		// Mesh* mesh = game_object->m_mesh;
		Vertex* vertex_buffer = reinterpret_cast<Vertex*>(mesh->m_vertex_buffer_cpu->GetBufferPointer());
		size_t start_index = mesh->m_submeshes[i].m_start_index; // the location of the first index read by the GPU from the index buffer
		size_t vertex_count = mesh->m_submeshes[i].m_vertex_count; // the number of indecis for this submesh
		size_t base_vertex = mesh->m_submeshes[i].m_base_vertex; // a value added to each index before reading a vertex from the vertex buffer
		m_oriented_boxes[i].create_from_points(vertex_count, vertex_buffer, base_vertex, start_index, mesh->m_vertex_stride);
		// m_oriented_boxes[i].m_transform = mesh->m_submeshes[i].m_transform;
		m_oriented_boxes[i].m_body = nullptr;
		m_oriented_boxes[i].set_transform_pointers(game_object, mesh, (mesh->m_submeshes.data() + i));
		m_oriented_boxes[i].calculate_transform();
	}

	// create the AABB
	DirectX::XMFLOAT3 corners[8];
	assert(m_obb_count >= 1);
	m_oriented_boxes[0].get_corners(corners); // get's the corners in submodel space
	for (size_t i = 0; i != 8; ++i) { // transform each corner into m_box's/AABB's space
		// DirectX::XMStoreFloat3(corners + i, DirectX::XMVector3Transform(XMLoadFloat3(corners + i), XMLoadFloat4x4(&m_oriented_boxes[0].get_model_transform())));
		DirectX::XMStoreFloat3(corners + i, DirectX::XMVector3Transform(XMLoadFloat3(corners + i), XMLoadFloat4x4(&mesh->m_submeshes[0].m_transform)));
	}
	m_box->create_from_points(8, corners, sizeof(DirectX::XMFLOAT3));
	for (size_t obb_index = 1; obb_index < m_obb_count; ++obb_index) { // for the other. use < because it seems safer here
		m_oriented_boxes[obb_index].get_corners(corners); // get the corners of the OBB
		for (size_t j = 0; j != 8; ++j) { // transform each corner into m_box's/AABB's space
			// DirectX::XMStoreFloat3(corners + j, DirectX::XMVector3Transform(XMLoadFloat3(corners + j), XMLoadFloat4x4(&m_oriented_boxes[obb_index].m_transform)));
			DirectX::XMStoreFloat3(corners + j, DirectX::XMVector3Transform(XMLoadFloat3(corners + j), XMLoadFloat4x4(&mesh->m_submeshes[obb_index].m_transform)));
		}
		BoundingBox temp; // bounding boxes are super cheap so this shouldnt be a problem, alsoonly 8 points so that should be super fast too
		temp.create_from_points(8, corners, sizeof(DirectX::XMFLOAT3));
		m_box->create_merged(temp); // would technically be faster if we didnt add them one by one, but this is simple enough for now
	}
	m_box->m_transform = &mesh->m_transform;

	// the rigidbody is created from the gameobject
}
CollisionObject::CollisionObject(GameObject* game_object, RigidBody* rigid_body, BoundingBox* aabb, uint32_t obb_count, OrientedBoundingBox* obbs)
	: m_game_object(game_object), m_body(rigid_body), m_box(aabb), m_obb_count(obb_count), m_oriented_boxes(new OrientedBoundingBox[m_obb_count]) {
	memcpy(m_oriented_boxes, obbs, m_obb_count * sizeof(OrientedBoundingBox*)); // copy the pointers into the CollisionObject
}
DirectX::XMFLOAT3 CollisionObject::get_position() {
	return m_body->get_position();
}
void CollisionObject::add_rigid_body() {
	if (m_body) return;
	m_body = new RigidBody(m_game_object);
	for (size_t i = 0; i != m_obb_count; ++i) {
		m_oriented_boxes[i].m_body = m_body;
	}
}