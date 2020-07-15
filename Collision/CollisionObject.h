#pragma once
#include "RigidBody.h" // for RigidBody
#include "BroadCollide.h" // for AABB
#include "NarrowCollide.h" // for OBB
#include "../Gameplay/GameObject.h"

struct CollisionObject {
	GameObject* m_game_object = nullptr; // pointer to the gameobject that contains us
	RigidBody* m_body = nullptr;
	BoundingBox* m_box = nullptr;
	const uint32_t m_obb_count;
	OrientedBoundingBox* m_oriented_boxes = nullptr; // stores an array of OBBs of size m_obb_count
	CollisionObject() = delete;
	// must pass the game_object we intend to add the physics object to
	CollisionObject(GameObject* game_object, RigidBody* rigid_body, BoundingBox* aabb, uint32_t obb_count, OrientedBoundingBox* obbs)
		: m_game_object(game_object), m_body(rigid_body), m_box(aabb), m_obb_count(obb_count), m_oriented_boxes(new OrientedBoundingBox[m_obb_count]) {
		memcpy(m_oriented_boxes, obbs, m_obb_count * sizeof(OrientedBoundingBox*)); // copy the pointers into the CollisionObject
	}
	~CollisionObject() {
		delete m_body;
		delete m_box;
		delete[] m_oriented_boxes;
	}
};
enum GameObjectComponents {
	EMPTY = 0,
	HAS_MESH = 1,
	HAS_COLLISION = 2,
	HAS_CAMERA = 4
};
inline GameObjectComponents operator|(GameObjectComponents a, GameObjectComponents b) {
	return static_cast<GameObjectComponents>(static_cast<int>(a) | static_cast<int>(b));
}
inline GameObjectComponents operator&(GameObjectComponents a, GameObjectComponents b) {
	return static_cast<GameObjectComponents>(static_cast<int>(a) & static_cast<int>(b));
}
inline GameObjectComponents operator~(GameObjectComponents a) {
	return static_cast<GameObjectComponents>(~static_cast<int>(a));
}