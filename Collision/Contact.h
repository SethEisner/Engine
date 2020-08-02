#pragma once
#include "RigidBody.h"
#include <DirectXMath.h>

class ContactResolver;

// static const size_t g_num_bodies = 2;

class Contact {
	friend class ContactResolver;
public:
	Contact() = default;
	RigidBody* m_body[2] = { nullptr, nullptr }; // pair of bodies involved in the contact
	float m_friction; // lateral friction at contact
	float m_restitution; // the normal resitution coefficient at the contact, affects how much energy is preserved in the collision
	float m_penetration; // the depth of penetration at the contact point;
	DirectX::XMFLOAT3 m_contact_point;
	DirectX::XMFLOAT3 m_contact_normal;
	void set_body_data(RigidBody* first, RigidBody* second);
	void set_body_data(RigidBody* first, RigidBody* second, float friction, float restitution);
protected:
	DirectX::XMFLOAT3X3 m_contact_to_world;
	DirectX::XMFLOAT3 m_contact_velocity;
	DirectX::XMFLOAT3 m_relative_contact_position[2]; // hold the world space position of the contact point relative to the center of each body
	float m_desired_delta_velocity;

	void calculate_internals(double duration);
	void swap_bodies(); // reverses the contact
	void match_awake_state(); // updates the awawke state of tounching rigidbodies, will wake a sleeping rigidbody
	void calculate_desired_delta_velocity(double duration);
	DirectX::XMFLOAT3 calculate_local_velocity(uint32_t body_index, double duration);
	void calculate_contact_basis(); // calulates an orthonormal basis for the contact point, based on the friction direction
	void apply_impulse(const DirectX::XMFLOAT3& impulse, RigidBody* body, DirectX::XMFLOAT3* velocity_change, DirectX::XMFLOAT3* rotation_change); // apply an impulse to the given body, returning a velocity
	void apply_velocity_change(DirectX::XMFLOAT3 velocity_change[2]); // perform an inertia-weighted impulse based resolution of this contact alone
	void apply_position_change(DirectX::XMFLOAT3 linear_change[2], float penetration);
	DirectX::XMFLOAT3 calculate_frictionless_impulse(/*DirectX::XMFLOAT3X3* inverse_inertia_tensor*/); // calculates the impulse needed to resolve the contact given that the contact has no friction
	DirectX::XMFLOAT3 calculate_friction_impulse(/*DirectX::XMFLOAT3X3* inverse_inertia_tensor*/); // impulse to solve the contact die to a non-zero coefficient of friction
};

class ContactResolver {
protected:
	size_t m_max_velocity_iterations;
	size_t m_max_position_iterations;
	float m_velocity_epsilon; // velocities smaller than this are considered zero
	float m_position_epsilon; // penetrations smaller than this value are considered to be not penetrating (usually 0.01 is okay)
public:
	size_t m_velocity_iterations_used;
	size_t m_position_iterations_used;
private:
	bool m_valid_settings;

public:
	ContactResolver(size_t iterations, float velocity_epsilon = 0.01f, float position_epsilon = 0.01f);
	ContactResolver(size_t velocity_iterations, size_t position_iterations, float velocity_epsilon = 0.01f, float position_epsilon = 0.01f);
	inline bool is_valid() {
		return (m_max_velocity_iterations > 0) && (m_max_position_iterations > 0) && (m_position_epsilon >= 0.0f) && (m_velocity_epsilon >= 0.0f);
	}
	void set_iterations(size_t velocity_iterations, size_t position_iterations);
	void set_iterations(size_t iterations);
	void set_epsilon(float velocity_epsilon, float position_epsilon);
	void resolve_contacts(Contact* contact_array, Contact* contact_end, double duration);
protected:
	void preprocess_contacts(Contact* contact_array, Contact* contact_end, double duration); //readies all contacts in the array for processing, ensures internal data is correct and appropriate objects are alive
	void adjust_velocities(Contact* contact_array, Contact* contact_end, double duration); // resolves velocity issues constrained by velocity iterations
	void adjust_positions(Contact* contact_array, Contact* contact_end, double duration); // resolves position issues constrained by velocity iterations
};
// fills the contact structure with the generated contact, the contact pointer should always point to the first available contact in a contact array, 
// where limit is the max number of contacts the array can hold, 
// returns the number of contacts that have been created
// seems like a useless class, and we should instead read the BVH for collisions
class ContactGenerator {
public:
	virtual size_t add_contact(Contact* contact, size_t limit) const = 0;
};