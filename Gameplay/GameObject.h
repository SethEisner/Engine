#pragma once

#include "../RenderManager/d3dUtil.h" // for Mesh
#include "../Collision/CollisionObject.h" // for CollisionObject
#include "../RenderManager/Camera.h" // for the camera
#include <vector>


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
// only holds data pointers, should not need any member functions
// should never need to change the pointers it holds, but can use the pointers to change the objects they point to (e.g. to update a bounding box)
class Mesh;
class CollisionObject;
class Camera;
// remove functions return the pointer we set to null so we dont have a lost memory
// DO THE ADD AND REMOVE AS TEMPLATED TYPES, CAN USE DECL TYPE TO COMPARE
// game object has the ownership of the components, and thus controls their lifetime
// the add functions assume ownership of the passed pointer
// the remove functions relinquish ownership of the passed pointer
struct GameObject {
	Mesh* m_mesh = nullptr;
	CollisionObject* m_collision_object = nullptr;
	Camera* m_camera = nullptr; // may not want to have a camera in the game object
	GameObjectComponents m_components = EMPTY;
	void add_mesh(Mesh* mesh);
	Mesh* remove_mesh();
	void add_collision_object(CollisionObject* collision_obj);
	CollisionObject* remove_collision_object();
	void add_camera(Camera* camera);
	Camera* remove_camera();
	GameObject() = delete;
	~GameObject() {
		delete m_camera;
		delete m_collision_object;
		delete m_mesh;
	}
};

// is the player a gameobject, or does the player contain a gameobejct
// I think the player is a gameobject