#include "Camera.h"
#include "../Utilities/Utilities.h"
#include "../Engine.h"
#include "../InputManager/InputManager.h"
using namespace DirectX;

Camera::Camera() {
	// create default lens with an fov of 90, an aspect ratio of 1.0f, a near plane at 1.0f,a nd a far plane at 1000.0f
	set_lens(radians(90.0f), 1.0f, 1.0f, 1000.0f);
	//update_view_matrix();
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
	//assert(!m_view_dirty);
	return XMLoadFloat4x4(&m_view);
}
XMMATRIX Camera::get_proj() const {
	return XMLoadFloat4x4(&m_proj);
}
XMFLOAT4X4 Camera::get_view4x4() const {
	//assert(!m_view_dirty);
	return m_view;
}
XMFLOAT4X4 Camera::get_proj4x4() const {
	return m_proj;
}
// given radians
void Camera::pitch(float angle) { // rotate about the right vector
	// rotates the up and look vector about the right vector
	XMMATRIX rotation = XMMatrixRotationAxis(XMLoadFloat3(&m_right), angle);
	XMStoreFloat3(&m_up, XMVector3TransformNormal(XMLoadFloat3(&m_up), rotation));
	XMStoreFloat3(&m_look, XMVector3TransformNormal(XMLoadFloat3(&m_look), rotation));
	//XMStoreFloat4x4(&m_rotation, rotation);
	m_view_dirty = true;
}
void Camera::yaw(float angle) {
	// rotates the basis vectors about the y-axis
	XMMATRIX rotation = XMMatrixRotationY(angle);

	XMStoreFloat3(&m_right, XMVector3Transform(XMLoadFloat3(&m_right), rotation));
	XMStoreFloat3(&m_up, XMVector3Transform(XMLoadFloat3(&m_up), rotation));
	XMStoreFloat3(&m_look, XMVector3Transform(XMLoadFloat3(&m_look), rotation));

	m_view_dirty = true;
}
// strafe left and right along the right vector
void Camera::strafe(float d) { // d is direction (+d is right, -d is left)
	// m_pos += d * m_right; kinematic equation
	XMVECTOR scalar = XMVectorReplicate(d);
	XMVECTOR right = XMLoadFloat3(&m_right);
	XMVECTOR pos = XMLoadFloat3(&m_pos);
	// scalar * right + pos
	XMStoreFloat3(&m_pos, XMVectorMultiplyAdd(scalar, right, pos));
	//XMStoreFloat3(&m_look, XMVectorAdd(pos, XMLoadFloat3(&m_look)));
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
	/*if (m_view_dirty) {
		XMVECTOR right = XMLoadFloat3(&m_right);
		XMVECTOR up = XMLoadFloat3(&m_up);
		XMVECTOR look = XMLoadFloat3(&m_look);
		XMVECTOR pos = XMLoadFloat3(&m_pos);

		look = XMVector3Normalize(look);
		up = XMVector3Normalize(XMVector3Cross(look, right));
		right = XMVector3Cross(up, look);
		
		// negate because LH coordinate system
		float x = XMVectorGetX(XMVector3Dot(pos, right));
		float y = XMVectorGetX(XMVector3Dot(pos, up));
		float z = XMVectorGetZ(XMVector3Dot(pos, look));

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
	}*/
}
void Camera::update(float delta_t) {
	// if (engine->input_manager->mouse_pos_changed()) {
	// 	int x = engine->input_manager->get_mouse_x();
	// 	int y = engine->input_manager->get_mouse_y();
	// 	int prev_x = engine->input_manager->get_mouse_prev_x();
	// 	int prev_y = engine->input_manager->get_mouse_prev_y();
	// 	float dx = XMConvertToRadians(0.125f * static_cast<float>(x - prev_x));
	// 	float dy = XMConvertToRadians(0.125f * static_cast<float>(y - prev_y));
	// 	std::string temp = std::to_string(dx) + " " + std::to_string(dy) + "\n";
	// 	OutputDebugStringA(temp.c_str());
	// 	yaw(dx);
	// 	pitch(dy);
	// 	//engine->input_manager->update_mouse_state();
	// }
	
	//if (engine->input_manager->mouse_pos_changed()) {
		m_yaw = engine->input_manager->get_mouse_delta_x() * 0.001f;
		//m_yaw = 0.0f;
		m_pitch = engine->input_manager->get_mouse_delta_y() * 0.001f;

		XMMATRIX rotation = XMMatrixRotationRollPitchYaw(-m_pitch, m_yaw, 0.0f);
		XMVECTOR target = XMVector3Normalize(XMVector3TransformCoord(m_default_forward, rotation));
		// XMFLOAT4 temp;
		// XMStoreFloat4(&temp, target);
		// std::string debug = "";
		// debug += std::to_string(temp.x) + " " + std::to_string(temp.y) + " " + std::to_string(temp.z) + " " + std::to_string(temp.w) + "\n";
		// OutputDebugStringA(debug.c_str());
		// rotate the basis about the up vector
		XMMATRIX rotate_y = XMMatrixRotationY(m_yaw);
		XMStoreFloat3(&m_right, XMVector3TransformCoord(m_default_right, rotate_y));
		XMStoreFloat3(&m_up, XMVector3TransformCoord(XMLoadFloat3(&m_up), rotate_y));
		XMStoreFloat3(&m_look, XMVector3TransformCoord(m_default_forward, rotate_y));

		// update position here later
		if (engine->input_manager->is_held(HASH("forward")) || engine->input_manager->is_pressed(HASH("forward"))) {
			walk(delta_t*10.0f);
		}
		if (engine->input_manager->is_held(HASH("backward")) || engine->input_manager->is_pressed(HASH("backward"))) {
			walk(-delta_t * 10.0f);
		}
		if (engine->input_manager->is_held(HASH("right")) || engine->input_manager->is_pressed(HASH("right"))) {
			strafe(delta_t * 10.0f);
		}
		if (engine->input_manager->is_held(HASH("left")) || engine->input_manager->is_pressed(HASH("left"))) {
			strafe(-delta_t * 10.0f);
		}


		// update the view matrix to be our rotation
		XMStoreFloat4x4(&m_view, XMMatrixLookToLH(XMLoadFloat3(&m_pos), target, XMLoadFloat3(&m_up)));
		m_view_dirty = false;
	//}

}