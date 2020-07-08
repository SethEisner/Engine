#include "Body.h"
#include <memory>
#include <assert.h>
#include <math.h>
#include <limits>

void Body::calculate_derived_data() { return; }
void Body::integrate(double duration) {
	// if (!m_is_awake) return; // do not integrate sleeping bodies
	using namespace DirectX;
	// calculate linear accleration from force inputs
	XMVECTOR last_frame_acceleration = XMLoadFloat3(&m_acceleration);//  XMVectorSet(m_acceleration.x, m_acceleration.y, m_acceleration.z, 0.0f);
	XMVECTOR force_accum = XMLoadFloat3(&m_force_accum);
	XMVECTOR inverse_mass = XMVectorSet(m_inverse_mass, m_inverse_mass, m_inverse_mass, 0.0f);
	XMVECTOR velocity = XMLoadFloat3(&m_velocity);
	XMVECTOR position = XMVectorSet(m_position.x, m_position.y, m_position.z, 1.0f); // position can be translated so w is 1
	//m_last_frame_accleration = m_acceleration;
	last_frame_acceleration = last_frame_acceleration + (force_accum * inverse_mass);
	// adjust velocity, update linear velocity from both accleration and impulse
	velocity = velocity + (last_frame_acceleration * duration);
	// impose drag
	velocity = velocity * pow(m_linear_damping, duration);
	// update linear position
	position = position + (velocity * duration);

	calculate_derived_data();
	clear_accumulators();

	XMStoreFloat3(&m_last_frame_accleration, last_frame_acceleration);
	XMStoreFloat3(&m_velocity, velocity);
	XMStoreFloat3(&m_position, position);
	// should never sleep the player because their position and possibly velocity can be updated else where
}
void Body::set_mass(const float mass) {
	assert(mass != 0.0f);
	m_inverse_mass = 1.0f / mass;
}
float Body::get_mass() const{
	if (m_inverse_mass == 0.0f) return std::numeric_limits<float>::infinity();
	return 1.0f / m_inverse_mass;
}
void Body::set_inverse_mass(const float inverse_mass) {
	m_inverse_mass = inverse_mass;
}
float Body::get_inverse_mass() const {
	return m_inverse_mass;
}
bool Body::has_finite_mass() const {
	return m_inverse_mass > 0.0f;
}
void Body::set_linear_damping(const float linear_damping) {
	m_linear_damping = linear_damping;
}
float Body::get_linear_damping() const {
	return m_linear_damping;
}
void Body::set_position(const DirectX::XMFLOAT3& position)
{
	DirectX::XMVectorSet(position.x, position.y, position.z, 1.0f);
}
void Body::set_position(const float x, const float y, const float z) {
	DirectX::XMVectorSet(x, y, z, 1.0f);
}
// fucntions that get an XMVECTOR member should return a XMFLOAT3 or 4
void Body::get_position(DirectX::XMFLOAT3* ret) const {
	*ret = m_position;
}
DirectX::XMFLOAT3 Body::get_position() const {
	return m_position;
}
void Body::get_transform(DirectX::XMFLOAT4X4* transform) const {
	*transform = m_transform;
}
DirectX::XMFLOAT4X4 Body::get_transform() const {
	return m_transform;
}
DirectX::XMFLOAT3 Body::get_point_in_local_space(const DirectX::XMFLOAT3& point) const {
	DirectX::XMFLOAT4 ret;
	DirectX::XMStoreFloat4(&ret, 
		DirectX::XMVector4Transform(DirectX::XMVectorSet(point.x, point.y, point.z, 1.0f), DirectX::XMLoadFloat4x4(&m_inverse_transform)));
	return DirectX::XMFLOAT3(ret.x, ret.y, ret.z);
}
DirectX::XMFLOAT3 Body::get_point_in_world_space(const DirectX::XMFLOAT3& point) const {
	DirectX::XMFLOAT4 ret;
	DirectX::XMStoreFloat4(&ret,
		DirectX::XMVector4Transform(DirectX::XMVectorSet(point.x, point.y, point.z, 1.0f), DirectX::XMLoadFloat4x4(&m_transform)));
	return DirectX::XMFLOAT3(ret.x, ret.y, ret.z);
}
DirectX::XMFLOAT3 Body::get_vector_in_local_space(const DirectX::XMFLOAT3& vector) const {
	DirectX::XMFLOAT3 ret;
	DirectX::XMStoreFloat3(&ret,
		DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&vector), DirectX::XMLoadFloat4x4(&m_inverse_transform)));
	return ret;
}
DirectX::XMFLOAT3 Body::get_vector_in_world_space(const DirectX::XMFLOAT3& vector) const {
	DirectX::XMFLOAT3 ret;
	DirectX::XMStoreFloat3(&ret,
		DirectX::XMVector3Transform(DirectX::XMLoadFloat3(&vector), DirectX::XMLoadFloat4x4(&m_transform)));
	return ret;
}
void Body::set_velocity(const DirectX::XMFLOAT3& velocity) {
	m_velocity = velocity;
}
void Body::set_velocity(float x, float y, float z) {
	m_velocity.x = x;
	m_velocity.y = y;
	m_velocity.z = z;
}
void Body::get_velocity(DirectX::XMFLOAT3* velocity) const {
	*velocity = m_velocity;
}
DirectX::XMFLOAT3 Body::get_velocity() const {
	return m_velocity;
}
void Body::add_velocity(const DirectX::XMFLOAT3& delta_velocity) {
	using namespace DirectX;
	XMVECTOR velocity = XMLoadFloat3(&m_velocity);
	velocity += XMLoadFloat3(&delta_velocity);
	XMStoreFloat3(&m_velocity, velocity);
}
void Body::get_last_frame_acceleration(DirectX::XMFLOAT3* accel) const {
	*accel = m_last_frame_accleration;
}
DirectX::XMFLOAT3 Body::get_last_frame_acceleration() const {
	return m_last_frame_accleration;
}
void Body::clear_accumulators() {
	m_force_accum.x = 0.0f;
	m_force_accum.y = 0.0f;
	m_force_accum.z = 0.0f;
	// dont have forces so we only need to set the force accumulator to zero
}
void Body::add_force(const DirectX::XMFLOAT3& force)
{
	using namespace DirectX;
	XMVECTOR force_accum = XMLoadFloat3(&m_force_accum);
	force_accum += XMLoadFloat3(&force);
	XMStoreFloat3(&m_force_accum, force_accum);
}
// adding a force at a point does the same thing as add force because we do not calculate rotations for Bodys (yet)
// if we want rotations then adding force at a point will apply a torque
// void Body::add_force_at_body_point(const DirectX::XMFLOAT3& force, const DirectX::XMFLOAT3& point) {
// 	// Convert to coordinates relative to center of mass.
// 	using namespace DirectX;
// 	XMFLOAT3 pt = get_point_in_world_space(point);
// 	add_force_at_point(force, pt);
// }
// void Body::add_force_at_point(const DirectX::XMFLOAT3& force, const DirectX::XMFLOAT3& point) {
// 	using namespace DirectX;
// 	XMVECTOR force_accum = XMLoadFloat3(&m_force_accum);
// 	force_accum += XMLoadFloat3(&force);
// 	XMStoreFloat3(&m_force_accum, force_accum);
// }
void Body::set_acceleration(const DirectX::XMFLOAT3& accel) {
	m_acceleration = accel;
}
void Body::set_acceleration(float x, float y, float z) {
	m_acceleration.x = x;
	m_acceleration.y = y;
	m_acceleration.z = z;
}
void Body::get_acceleration(DirectX::XMFLOAT3* accel) const {
	*accel = m_acceleration;
}
DirectX::XMFLOAT3 Body::get_acceleration() const {
	return m_acceleration;
}