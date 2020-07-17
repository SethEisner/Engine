#include "CollisionObject.h"
#include "RigidBody.h" // for RigidBody
#include "BroadCollide.h" // for AABB
#include "NarrowCollide.h" // for OBB
#include "../Gameplay/GameObject.h"
#include <DirectXCollision.h>
#include <D3DCommon.h>
#include "../RenderManager/d3dUtil.h" // for Mesh
#include "../RenderManager/RenderItem.h"
CollisionObject::CollisionObject(GameObject* game_object, Mesh* mesh) : m_game_object(game_object), m_obb_count(mesh->m_submeshes.size()), m_oriented_boxes(new OrientedBoundingBox[m_obb_count]){
	// give the mesh we want to create a collision object for
	assert(mesh); // we need the mesh to already be created before we try to create a collision object for it
	for (size_t i = 0; i != m_obb_count; ++i) {
		using namespace DirectX;
		// Mesh* mesh = game_object->m_mesh;
		Vertex* vertex_buffer = reinterpret_cast<Vertex*>(mesh->m_vertex_buffer_cpu->GetBufferPointer());
		size_t start_index = mesh->m_submeshes[i].m_start_index; // the location of the first index read by the GPU from the index buffer
		size_t vertex_count = mesh->m_submeshes[i].m_vertex_count; // the number of indecis for this submesh
		size_t base_vertex = mesh->m_submeshes[i].m_base_vertex; // a value added to each index before reading a vertex from the vertex buffer
		m_oriented_boxes[i].m_oriented_box.CreateFromPoints(m_oriented_boxes[i].m_oriented_box,
			vertex_count, reinterpret_cast<XMFLOAT3*>(vertex_buffer + base_vertex + start_index), mesh->m_vertex_stride);
		m_oriented_boxes[i].m_transform = mesh->m_submeshes[i].m_transform;
	}

}
CollisionObject::CollisionObject(GameObject* game_object, RigidBody* rigid_body, BoundingBox* aabb, uint32_t obb_count, OrientedBoundingBox* obbs)
	: m_game_object(game_object), m_body(rigid_body), m_box(aabb), m_obb_count(obb_count), m_oriented_boxes(new OrientedBoundingBox[m_obb_count]) {
	memcpy(m_oriented_boxes, obbs, m_obb_count * sizeof(OrientedBoundingBox*)); // copy the pointers into the CollisionObject
}