#pragma once
#include "d3dUtil.h"

class Camera {
public:
	Camera();
	~Camera() = default;
	// camera position
	DirectX::XMVECTOR get_position() const;
	DirectX::XMFLOAT3 get_position3f() const;
	void set_position(float x, float y, float z);
	void set_position(const DirectX::XMFLOAT3& v);
	// camera basis vectors
	DirectX::XMVECTOR get_right() const;
	DirectX::XMFLOAT3 get_right3f() const;
	DirectX::XMVECTOR get_up() const;
	DirectX::XMFLOAT3 get_up3f() const;
	DirectX::XMVECTOR get_look() const;
	DirectX::XMFLOAT3 get_look3f() const;
	// get frustum properties
	float get_near_z() const;
	float get_far_z() const;
	float get_aspect() const;
	float get_fov_y() const;
	float get_fov_x() const;
	// get near and far plane dimensions in view space coordinates
	float get_near_window_width() const;
	float get_near_window_height() const;
	float get_far_window_width() const;
	float get_far_window_height() const;
	// create frustum
	void set_lens(float fov_y, float aspect, float z_near, float z_far);
	void look_at(DirectX::FXMVECTOR pos, DirectX::FXMVECTOR target, DirectX::FXMVECTOR world_up);
	void look_at(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& target, const DirectX::XMFLOAT3& world_up);
	// get view/proj matrices
	DirectX::XMMATRIX get_view() const; //XMMATRIX is packed into XMVECTORs, XMFLOAT4X4 is not packed into XMVECTORs (but is 4 separate floats)
	DirectX::XMMATRIX get_proj() const;
	DirectX::XMFLOAT4X4 get_view4x4() const;
	DirectX::XMFLOAT4X4 get_proj4x4() const;
	// set strafe/walk the camera a distance d
	void strafe(float d);
	void walk(float d);
	// function to rebuild the view matrix after the camera moves
	void update_view_matrix();
private:
	// camera coodinate system defined relative to world space
	DirectX::XMFLOAT3 m_pos = { 0.0f, 0.0f, 0.0f }; // default position is world origin
	DirectX::XMFLOAT3 m_right = { 1.0f, 0.0f, 0.0f };
	DirectX::XMFLOAT3 m_up = { 0.0f, 1.0f, 0.0f }; // up is pos y and right is pos x so view matrix uses a LH coordinate system (allows z to increase away from the camera)
	DirectX::XMFLOAT3 m_look = { 0.0f, 0.0f, 1.0f };
	// frustum properties
	float m_near_z = 0.0f;
	float m_far_z = 0.0f;
	float m_aspect = 0.0f; // can get this from the window structure
	float m_fov_y = 0.0f;
	float m_near_window_height = 0.0f; // can get the width using the aspect ratio
	float m_far_window_height = 0.0f; 
	bool m_view_dirty = true;
	// current view and projection matrices
	DirectX::XMFLOAT4X4 m_view = MathHelper::identity_4x4();
	DirectX::XMFLOAT4X4 m_proj = MathHelper::identity_4x4();
};