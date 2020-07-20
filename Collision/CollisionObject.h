#pragma once
// #include "RigidBody.h" // for RigidBody
// #include "BroadCollide.h" // for AABB
// #include "NarrowCollide.h" // for OBB
// #include "../Gameplay/GameObject.h"
// #include <DirectXCollision.h>
// #include <D3DCommon.h>
// #include "../RenderManager/d3dUtil.h" // for Mesh
#include <stdint.h>
#include <DirectXMath.h>
#include <DirectXCollision.h>
class BoundingBox;
class RigidBody;
class GameObject;
class Mesh;
class OrientedBoundingBox;
// each submesh in a mesh has it's own OBB, it's smaller so it's faster to calculate
struct CollisionObject {
	GameObject* m_game_object = nullptr; // pointer to the gameobject that contains us
	RigidBody* m_body = nullptr;
	BoundingBox* m_box = nullptr;
	const uint32_t m_obb_count;
	OrientedBoundingBox* m_oriented_boxes = nullptr; // stores an array of OBBs of size m_obb_count
	CollisionObject() = delete;
	// must pass the game_object we intend to add the physics object to
	CollisionObject(GameObject* game_object, RigidBody* rigid_body, BoundingBox* aabb, uint32_t obb_count, OrientedBoundingBox* obbs); /*
		: m_game_object(game_object), m_body(rigid_body), m_box(aabb), m_obb_count(obb_count), m_oriented_boxes(new OrientedBoundingBox[m_obb_count]) {
		memcpy(m_oriented_boxes, obbs, m_obb_count * sizeof(OrientedBoundingBox*)); // copy the pointers into the CollisionObject
	}*/
	// explicit CollisionObject(GameObject* game_object);
	CollisionObject(GameObject* game_object, Mesh* mesh);/* : m_game_object(game_object), m_obb_count(game_object->m_mesh->m_submeshes.size()), m_oriented_boxes(new OrientedBoundingBox[m_obb_count]){
		// give the mesh we want to create a collision object for
		assert(game_object->m_mesh); // we need the mesh to already be created before we try to create a collision object for it
		for (size_t i = 0; i != m_obb_count; ++i) {
			using namespace DirectX;
			// Mesh* mesh = game_object->m_mesh;
			m_oriented_boxes[i].m_oriented_box.CreateFromPoints(m_oriented_boxes[i].m_oriented_box, 
					game_object->m_mesh->m_vertex_count, 
					reinterpret_cast<XMFLOAT3*>(game_object->m_mesh->m_vertex_buffer_cpu->GetBufferPointer()), 
					game_object->m_mesh->m_vertex_stride);
		}
	}*/
	~CollisionObject() {
		delete m_body;
		delete m_box;
		delete[] m_oriented_boxes;
	}
	DirectX::XMFLOAT3 get_position();
};