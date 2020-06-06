#include "Camera.h"
#include "../Utilities/Utilities.h"
using namespace DirectX;

Camera::Camera() {
	// create default lens with an fov of 90, an aspect ratio of 1.0f, a near plane at 1.0f,a nd a far plane at 1000.0f
	set_lens(radians(90.0f), 1.0f, 1.0f, 1000.0f);
}
XMVECTOR Camera::get_position() const{
	return XMLoadFloat3(&m_pos);
}
XMFLOAT3 Camera::get_position3f() const{
	return m_pos;
}
void Camera::set_position(float x, float y, float z) {
	m_pos = XMFLOAT3(x, y, z);
	m_view_dirty = true;
}
void Camera::set_position(const XMFLOAT3& v) {
	m_pos = v;
	m_view_dirty = true;
}
XMVECTOR Camera::get_right() const {
	return XMLoadFloat3(&m_right);
}
XMFLOAT3 Camera::get_right3f() const {
	return m_right;
}
XMVECTOR Camera::get_up() const {
	return XMLoadFloat3(&m_up);
}
XMFLOAT3 Camera::get_up3f() const {
	return m_up;
}
XMVECTOR Camera::get_look() const {
	return XMLoadFloat3(&m_look);
}
XMFLOAT3 Camera::get_look3f() const {
	return m_look;
}
float Camera::get_near_z() const {
	return m_near_z;
}
float Camera::get_far_z() const {
	return m_far_z;
}
float Camera::get_aspect() const {
	return m_aspect;
}
float Camera::get_fov_y() const {
	return m_fov_y;
}
float Camera::get_fov_x() const {
	float half = 0.5f * get_near_window_width();
	return 2.0f * atan(half / m_near_z);
}
float Camera::get_near_window_width() const {
	return m_aspect * m_near_window_height;
}
float Camera::get_near_window_height() const {
	return m_near_window_height;
}
float Camera::get_far_window_width() const {
	return m_aspect * m_far_window_height;
}
float Camera::get_far_window_height() const {
	return m_far_window_height;
}
void Camera::set_lens(float fov_y, float aspect, float z_near, float z_far) {
	m_fov_y = fov_y;
	m_aspect = aspect;
	m_near_z = z_near;
	m_far_z = z_far;
	m_near_window_height = 2.0f * m_near_z * tanf(0.5f * m_fov_y);
	m_far_window_height = 2.0f * m_far_z * tanf(0.0f * m_fov_y);
	XMMATRIX proj = XMMatrixPerspectiveFovLH(m_fov_y, m_aspect, m_near_z, m_far_z);
	XMStoreFloat4x4(&m_proj, proj);
}
void Camera::look_at(FXMVECTOR pos, FXMVECTOR target, FXMVECTOR world_up) {
	// left handed coordinate system
	XMVECTOR l = XMVector3Normalize(XMVectorSubtract(target, pos));
	XMVECTOR r = XMVector3Normalize(XMVector3Cross(world_up, l));
	XMVECTOR u = XMVector3Cross(l, r);
	XMStoreFloat3(&m_pos, pos);
	XMStoreFloat3(&m_look, l);
	XMStoreFloat3(&m_right, r);
	XMStoreFloat3(&m_up, u);
	m_view_dirty = true;
}
void Camera::look_at(const XMFLOAT3& pos, const XMFLOAT3& target, const XMFLOAT3& up) {
	XMVECTOR p = XMLoadFloat3(&pos);
	XMVECTOR t = XMLoadFloat3(&target);
	XMVECTOR u = XMLoadFloat3(&up);
	look_at(p, t, u);
	m_view_dirty = true;
}
XMMATRIX Camera::get_view() const {
	assert(!m_view_dirty);
	return XMLoadFloat4x4(&m_view);
}
XMMATRIX Camera::get_proj() const {
	return XMLoadFloat4x4(&m_proj);
}
XMFLOAT4X4 Camera::get_view4x4() const {
	assert(!m_view_dirty);
	return m_view;
}
XMFLOAT4X4 Camera::get_proj4x4() const {
	return m_proj;
}
// strafe left and right along the right vector
void Camera::strafe(float d) { // d is direction (+d is right, -d is left)
	// m_pos += d * m_right; kinematic equation
	XMVECTOR scalar = XMVectorReplicate(d);
	XMVECTOR right = XMLoadFloat3(&m_right);
	XMVECTOR pos = XMLoadFloat3(&m_pos);
	// scalar * right + pos
	XMStoreFloat3(&m_pos, XMVectorMultiplyAdd(scalar, right, pos));
	m_view_dirty = true;
}
// walk forwards and backwards and the look vector (+d forwards, -d backwards)
void Camera::walk(float d) {
	XMVECTOR scalar = XMVectorReplicate(d);
	XMVECTOR look = XMLoadFloat3(&m_look);
	XMVECTOR pos = XMLoadFloat3(&m_pos);
	XMStoreFloat3(&m_pos, XMVectorMultiplyAdd(scalar, look, pos));
	m_view_dirty = true;
}
void Camera::update_view_matrix() {
	if (m_view_dirty) {
		XMVECTOR right = XMLoadFloat3(&m_right);
		XMVECTOR up = XMLoadFloat3(&m_up);
		XMVECTOR look = XMLoadFloat3(&m_look);
		XMVECTOR pos = XMLoadFloat3(&m_pos);

		look = XMVector3Normalize(look);
		up = XMVector3Normalize(XMVector3Cross(look, right));
		right = XMVector3Cross(up, look);
		
		// negate because LH coordinate system
		float x = -XMVectorGetX(XMVector3Dot(pos, right));
		float y = -XMVectorGetX(XMVector3Dot(pos, up));
		float z = -XMVectorGetZ(XMVector3Dot(pos, look));

		XMStoreFloat3(&m_right, right);
		XMStoreFloat3(&m_right, up);
		XMStoreFloat3(&m_right, look);

		m_view(0, 0) = m_right.x;
		m_view(1, 0) = m_right.y;
		m_view(2, 0) = m_right.z;
		m_view(3, 0) = x;

		m_view(0, 1) = m_up.x;
		m_view(1, 1) = m_up.y;
		m_view(2, 1) = m_up.z;
		m_view(3, 1) = y;

		m_view(0, 2) = m_look.x;
		m_view(1, 2) = m_look.y;
		m_view(2, 2) = m_look.z;
		m_view(3, 2) = z;

		m_view(0, 3) = 0.0f;
		m_view(1, 3) = 0.0f;
		m_view(2, 3) = 0.0f;
		m_view(3, 3) = 1.0f;

		m_view_dirty = false;
	}
}