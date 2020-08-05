#include "BroadCollide.h"
#include "CollisionObject.h"
#include "../Gameplay/GameObject.h"

BoundingBox::BoundingBox() : m_box(DirectX::BoundingBox()), m_world_box(DirectX::BoundingBox()), m_game_object(nullptr), m_transform(new DirectX::XMFLOAT4X4()) {
	XMStoreFloat4x4(m_transform, DirectX::XMMatrixIdentity());
}
BoundingBox::BoundingBox(GameObject* obj) : m_box(DirectX::BoundingBox()), m_world_box(DirectX::BoundingBox()), m_game_object(obj), m_transform(new DirectX::XMFLOAT4X4()) {
}
// creating a new boundingbox from two existing boundingboxes creates the box in world space, also it doesnt have an associated GameObject
BoundingBox::BoundingBox(const BoundingBox& first, const BoundingBox& second) : m_box(DirectX::BoundingBox()), m_world_box(DirectX::BoundingBox()), m_game_object(nullptr), m_transform(new DirectX::XMFLOAT4X4()) {
	// create the new box from the transformed boxes, and then set the transform of the new box to be the identiy matrix
	m_box.CreateMerged(m_box, first.m_world_box, second.m_world_box);
	m_world_box = m_box;
	DirectX::XMStoreFloat4x4(m_transform, DirectX::XMMatrixIdentity());
	m_world_box = m_box;
}
void BoundingBox::transform() {
	DirectX::XMFLOAT4X4 model = *this->m_transform;
	DirectX::XMFLOAT4X4 game_obj_transform = m_game_object->m_transform;
	this->m_box.Transform(this->m_world_box, DirectX::XMMatrixMultiply(DirectX::XMLoadFloat4x4(this->m_transform), DirectX::XMLoadFloat4x4(&m_game_object->m_transform)));
}

float BoundingBox::get_growth(const BoundingBox& other) const {
	float base_surface_area = m_box.Extents.x * m_box.Extents.y + m_box.Extents.x * m_box.Extents.z + m_box.Extents.y * m_box.Extents.z;
	float extents[3] = { 0.0f, 0.0f, 0.0f };
	float this_min[3] = { m_world_box.Center.x - m_world_box.Extents.x, m_world_box.Center.y - m_world_box.Extents.y, m_world_box.Center.z - m_world_box.Extents.z };
	float this_max[3] = { m_world_box.Center.x + m_world_box.Extents.x, m_world_box.Center.y + m_world_box.Extents.y, m_world_box.Center.z + m_world_box.Extents.z };
	float other_min[3] = { other.m_world_box.Center.x - other.m_world_box.Extents.x, other.m_world_box.Center.y - other.m_world_box.Extents.y, other.m_world_box.Center.z - other.m_world_box.Extents.z };
	float other_max[3] = { other.m_world_box.Center.x + other.m_world_box.Extents.x, other.m_world_box.Center.y + other.m_world_box.Extents.y, other.m_world_box.Center.z + other.m_world_box.Extents.z };
	for (size_t i = 0; i != 3; i++) { // find the min and max for each dimension
		extents[i] = ((this_max[i] > other_max[i] ? this_max[i] : other_max[i]) - 
					  (this_min[i] < other_min[i] ? this_min[i] : other_min[i])) * 0.5f;
	}
	float surface_area = extents[0] * extents[1] + extents[0] * extents[2] + extents[1] * extents[2];
	float delta = 8.0f * (surface_area - base_surface_area);
	return delta;
}