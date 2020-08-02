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
	CollisionObject(GameObject* game_object, RigidBody* rigid_body, BoundingBox* aabb, uint32_t obb_count, OrientedBoundingBox* obbs);
	CollisionObject(GameObject* game_object, Mesh* mesh);
	// inline float get_friction() const;
	// inline float get_restitution() const;
	~CollisionObject() {
		delete m_body;
		delete m_box;
		delete[] m_oriented_boxes;
	}
	// float m_friction;
	// float m_restitution;
	void add_rigid_body();
	DirectX::XMFLOAT3 get_position();
	bool get_awake();
};