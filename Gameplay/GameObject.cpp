#include "GameObject.h"
#include "../Collision/RigidBody.h"


void GameObject::calculate_transform() {
	using namespace DirectX;
	// doesnt make sense to rotate about anything but the world origin
	XMStoreFloat4x4(&m_transform, XMMatrixAffineTransformation(XMLoadFloat3(&m_scale), XMVectorZero(), XMLoadFloat4(&m_rotation), XMLoadFloat3(&m_position)));
}
void GameObject::add_mesh(Mesh* mesh) {
	m_mesh = mesh;
	mesh->m_game_object = this;
	m_components = m_components | HAS_MESH;
}
Mesh* GameObject::remove_mesh() {
	Mesh* ret = m_mesh;
	m_mesh->m_game_object = nullptr;
	m_mesh = nullptr;
	m_components = m_components & ~HAS_MESH;
	return ret;
}
void GameObject::add_collision_object(CollisionObject* collision_obj) {
	m_collision_object = collision_obj;
	m_collision_object->m_game_object = this;
	m_components = m_components | HAS_COLLISION;
}
CollisionObject* GameObject::remove_collision_object() {
	CollisionObject* ret = m_collision_object;
	m_collision_object->m_game_object = nullptr;
	m_collision_object = nullptr;
	m_components = m_components & ~HAS_COLLISION;
	return ret;
}
void GameObject::update(double duration) {
	calculate_transform();
}