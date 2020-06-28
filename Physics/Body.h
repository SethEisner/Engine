#pragma once
#include <DirectXMath.h>

// dont program the physics calculations yet, when/if I do we can have a bool for 
// the player / rotatable, which prevents the body from rotating, can
// contact should use a pair of body pointers, doing it this way allows us to use RTTI and a single contact type

class Body {
public:
	virtual void calculate_derived_data(); // a rigid body will need to calculate different values
	virtual void integrate(float); // virtual because a rigidbody would integrate differently
	void set_mass(const float);
	float get_mass() const;
	void set_inverse_mass(const float);
	float get_inverse_mass() const;
	bool has_finite_mass() const; // if inverse mass is not 0 then we have finite mass
	void set_linear_damping(const float);
	float get_linear_damping() const;
	void set_position(const DirectX::XMFLOAT3& pos);
	void set_position(const float x, const float y, const float z);
	void get_position(DirectX::XMFLOAT3*) const;
	DirectX::XMFLOAT3 get_position() const;
	void get_transform(DirectX::XMFLOAT4X4& transform) const;
	DirectX::XMFLOAT4X4 get_transform() const;
	DirectX::XMFLOAT3 get_point_in_local_space(const DirectX::XMFLOAT3& pos) const;
	DirectX::XMFLOAT3 get_point_in_world_space(const DirectX::XMFLOAT3& pos) const;
	DirectX::XMFLOAT3 get_vector_in_local_space(const DirectX::XMFLOAT3& pos) const;
	DirectX::XMFLOAT3 get_vector_in_world_space(const DirectX::XMFLOAT3& pos) const;
	void set_velocity(const DirectX::XMFLOAT3& velocity);
	void set_velocity(const float x, const float y, const float z);
	void get_velocity(DirectX::XMFLOAT3* velocity) const;
	DirectX::XMFLOAT3 get_velocity() const;
	void add_velocity(const DirectX::XMFLOAT3& delta);
	void get_last_frame_acceleration(DirectX::XMFLOAT3* linearAcceleration) const; // useful for coherency caching?
	DirectX::XMFLOAT3 get_last_frame_acceleration() const;
	void clear_accumulators();
	void add_force(const DirectX::XMFLOAT3& force);
	void add_force_at_point(const DirectX::XMFLOAT3& force, const DirectX::XMFLOAT3& point); // point is in world space
	void add_force_at_body_point(const DirectX::XMFLOAT3& force, const DirectX::XMFLOAT3& point); // point is in body space
	void set_acceleration(const DirectX::XMFLOAT3& accel);
	void set_acceleration(const float x, const float y, const float z);
	void get_acceleration(DirectX::XMFLOAT3* accel) const;
	DirectX::XMFLOAT3 get_acceleration() const;

	// should have the virtual functions needed by the rigid body but they should be inlined and do nothing so that the contact code can work with the parent or child class
	
protected:
	DirectX::XMFLOAT4X4 m_transform; // convert from body space to world space;
	float m_inverse_mass; // holds the inverse mass of the body
	float m_linear_damping; // holds the amound of dampening applied to linear motion, for the player we want high damping so we dont get pushed super far
	DirectX::XMFLOAT3 m_position; // holds the position of the body
	DirectX::XMFLOAT3 m_velocity; // holds the linear velocity of the body
	
	// optimizations that may not be needed yet
	// bool m_is_awake; // can sleep the body so it doesnt get updated by the integration nor can it collide with the world
	// bool m_can_sleep; // mark whether the body is allowed to fall asleep

};
class RigidBody : protected Body { // allows for rotation

};