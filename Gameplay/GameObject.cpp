#include "GameObject.h"
#include "../Collision/RigidBody.h"


void GameObject::calculate_transform() {
	using namespace DirectX;
	XMStoreFloat4x4(&m_transform, XMMatrixMultiply(XMMatrixScaling(m_scale.x, m_scale.y, m_scale.z), XMMatrixTranslation(m_position.x, m_position.y, m_position.z)));
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
void GameObject::add_camera(Camera* camera) {
	m_camera = camera;
	m_components = m_components | HAS_CAMERA;
}
Camera* GameObject::remove_camera() {
	Camera* ret = m_camera;
	m_camera = nullptr;
	m_components = m_components & ~HAS_CAMERA;
	return ret;
}
void GameObject::update(double duration) {
	calculate_transform();
}