#include "GameObject.h"
#include "../Collision/RigidBody.h"

void GameObject::add_mesh(Mesh* mesh) {
	m_mesh = mesh;
	m_components = m_components | HAS_MESH;
}
Mesh* GameObject::remove_mesh() {
	Mesh* ret = m_mesh;
	m_mesh = nullptr;
	m_components = m_components & ~HAS_MESH;
	return ret;
}
void GameObject::add_collision_object(CollisionObject* collision_obj) {
	m_collision_object = collision_obj;
	m_components = m_components | HAS_COLLISION;
}
CollisionObject* GameObject::remove_collision_object() {
	CollisionObject* ret = m_collision_object;
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
	// translate the floor down by the duration every frame. should move pretty quickly
	// using namespace DirectX;
	// DirectX::XMFLOAT3 dur_vec = { 0.0f, static_cast<float>(-duration/2.0), 0.0f };
	// XMStoreFloat3(&m_position, DirectX::XMLoadFloat3(&m_position) + DirectX::XMLoadFloat3(&dur_vec));
	// XMStoreFloat4x4(&m_transform, XMMatrixTranslation(m_position.x, m_position.y, m_position.z));
	m_mesh->m_transform = m_collision_object->m_body->get_transform();
}