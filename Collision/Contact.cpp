#include "Contact.h"
#include <memory>
#include <assert.h>

// Contact implementation
void Contact::set_body_data(RigidBody* first, RigidBody* second, float friction, float restitution) {
	m_body[0] = first;
	m_body[1] = second;
	m_friction = friction;
	m_restitution = restitution;
}
void Contact::calculate_internals(double duration) {
	if (!m_body[0]) swap_bodies(); // the first body in the collision must have a valid pointer, the second can be a nullptr to mark it as unmovable scenery 
	assert(m_body[0]);
	// calculate a set of axis at the contact point, with the X axis being the contact normal
	calculate_contact_basis();
	// store the position of the contact relative to each body in contact space
	using namespace DirectX;
	assert(g_num_bodies == 2);
	for (size_t i = 0; i != g_num_bodies && m_body[i]; ++i) { // calculate the relative position for each non null body in the contact structure
		XMStoreFloat3(&m_relative_contact_position[i], XMLoadFloat3(&m_contact_point) - XMLoadFloat3(&m_body[i]->get_position()));
	}
	// find the relative velocity of the bodies at the contact point
	m_contact_velocity = calculate_local_velocity(0, duration);
	if (m_body[1]) {
		// m_contact_velocity -= calculate_local_velocity(1, duration);
		// negate here because contact normal is negated for the second body, so we need to re-negate it to put it into terms of the first body
		XMStoreFloat3(&m_contact_velocity, XMLoadFloat3(&m_contact_velocity) - XMLoadFloat3(&calculate_local_velocity(1, duration)));
	}
	// calculate the desired change in velocity for the resolution
	calculate_desired_delta_velocity(duration);

}
void Contact::swap_bodies() { // reverses the contact
	using namespace DirectX;
	// reverse the contact normal
	XMStoreFloat3(&m_contact_normal, -XMLoadFloat3(&m_contact_normal));
	std::swap(m_body[0], m_body[1]);
	
}
void Contact::calculate_desired_delta_velocity(double duration) {
	const static float velocity_limit = 0.25f;
	float velocity_from_acc = 0.0f;
	using namespace DirectX;
	const XMVECTOR contact_normal = XMLoadFloat3(&m_contact_normal);
	velocity_from_acc += XMVectorGetX(XMVector3Dot((XMLoadFloat3(&m_body[0]->get_last_frame_acceleration()) * duration), contact_normal));
	if (m_body[1]) velocity_from_acc -= XMVectorGetX(XMVector3Dot((XMLoadFloat3(&m_body[1]->get_last_frame_acceleration()) * duration), contact_normal));
	// if the velocity is very low, set the restitution to zero so the objects dont try to bounce: limits vibration of stacked objects
	float this_restitution = m_restitution;
	if (fabsf(m_contact_velocity.x) < velocity_limit) this_restitution = 0.0f;
	// combine the bounce velocity with the removed accleration velocity
	m_desired_delta_velocity = -m_contact_velocity.x - this_restitution * (m_contact_velocity.x - velocity_from_acc);
}
DirectX::XMFLOAT3 Contact::calculate_local_velocity(uint32_t body_index, double duration) {
	RigidBody* this_body = m_body[body_index];
	using namespace DirectX;
	// XMFLOAT3 velocity = this_body->get_velocity();
	XMVECTOR velocity = XMLoadFloat3(&this_body->get_velocity());
	// convert world space velocity into contact coordinates
	const XMMATRIX world_to_contact = XMMatrixTranspose(XMLoadFloat3x3(&m_contact_to_world));
	XMVECTOR contact_velocity = XMVector3Transform(velocity, world_to_contact);
	// calculate the amount of velocity that is due to forces without reactions (gravity)
	XMVECTOR acc_velocity;
	acc_velocity = XMLoadFloat3(&this_body->get_last_frame_acceleration()) * duration;
	// calculate the velocity in contact coordinates
	acc_velocity = XMVector3Transform(acc_velocity, world_to_contact);
	acc_velocity = XMVectorSetByIndex(acc_velocity, 0.0f, 0); // set x value to zero, because we dont want the velocity from acceleration in the direction of the contact normal
	// add the planar velocities, if there is enough friction they will be removed during velocity resolution
	contact_velocity += acc_velocity;
	XMFLOAT3 ret;
	XMStoreFloat3(&ret, contact_velocity);
	return ret;
}
void Contact::calculate_contact_basis() { // calulates an orthonormal basis for the contact point, based on the friction direction if anisotropic
	// constructs a row matrix to transform from contact space to world space
	// the row matrix form is the inverse of the column matrix form which would go from world space to contact space
	DirectX::XMFLOAT3 contact_tangent[2];

	using namespace DirectX;
	// todo: determine if orthogonal returns a normalized vector or not
	XMStoreFloat3(&contact_tangent[0], XMVector3Normalize(XMVector3Orthogonal(XMLoadFloat3(&m_contact_normal)))); // compute a normalized vector that's orthogonal to the contact normal
	// contact basis is right handed so we cross the normal (x) with the first tangent (y) to get z
	XMStoreFloat3(&contact_tangent[1], XMVector3Normalize(XMVector3Cross(XMLoadFloat3(&m_contact_normal), XMLoadFloat3(&contact_tangent[0]))));
	// copy into the rows to automatically calculate the transpose (inverse) which takes us from contact space to world space
	memcpy(m_contact_to_world.m[0], &m_contact_normal,   3 * sizeof(float));
	memcpy(m_contact_to_world.m[1], &contact_tangent[0], 3 * sizeof(float));
	memcpy(m_contact_to_world.m[2], &contact_tangent[1], 3 * sizeof(float));
}
void apply_impulse(const DirectX::XMFLOAT3& impulse, RigidBody* body, DirectX::XMFLOAT3* velocity_change, DirectX::XMFLOAT3* rotation_change); // apply an impulse to the given body, returning a velocity

// dont perform rotation so shouldnt have rotation change
void Contact::apply_velocity_change(DirectX::XMFLOAT3 velocity_change[2]) { // perform an inertia-weighted impulse based resolution of this contact alone
	// get the inverse mass
	using namespace DirectX;
	XMFLOAT3 impulse_contact;
	if (m_friction == 0.0f) {
		impulse_contact = calculate_frictionless_impulse(); // shouldn't need to sent the inverse inertia tensor because we are not rotating
	}
	else {
		impulse_contact = calculate_friction_impulse();
	}
	// convert the impulse into world coordinates
	XMVECTOR impulse = XMVector3Transform(XMLoadFloat3(&impulse_contact), XMLoadFloat3x3(&m_contact_to_world));
	// split the impulse into the linear and rotational components
	// only do linear component for now
	XMStoreFloat3(&velocity_change[0], impulse * m_body[0]->get_inverse_mass());
	m_body[0]->add_velocity(velocity_change[0]);
	if (m_body[1]) {
		XMStoreFloat3(&velocity_change[1], impulse * -m_body[1]->get_inverse_mass()); // negate because impulse is in opposite direction
		m_body[1]->add_velocity(velocity_change[1]);
	}
}
void Contact::apply_position_change(DirectX::XMFLOAT3 linear_change[2], float penetration) {
	float linear_move[g_num_bodies];
	float total_inertia = 0;
	float linear_inertia[g_num_bodies];
	using namespace DirectX;
	for (size_t i = 0; i != g_num_bodies && m_body[i]; ++i) {
		// no rotaional component so only need to worry about linear inertia
		linear_inertia[i] = m_body[i]->get_inverse_mass();
		total_inertia += linear_inertia[i];
	}
	// caluclate and apply the changes for each body
	float sign = 1; // sign for first body is 1 and -1 for the second body
	for (size_t i = 0; i != g_num_bodies && m_body[i]; ++i, sign = -1) {
		linear_move[i] = sign * penetration * (linear_inertia[i] / total_inertia); // objects in a collision move based on their relative mass
		
		XMVECTOR projection = XMLoadFloat3(&m_relative_contact_position[i]);
		const XMVECTOR contact_normal = XMLoadFloat3(&m_contact_normal);
		projection += contact_normal * XMVector3Dot(-XMLoadFloat3(&m_relative_contact_position[i]), contact_normal);
		// velocity change is the linear movement along the contact normal
		XMStoreFloat3(&linear_change[i], contact_normal * linear_move[i]);
		// apply the linear movement
		XMFLOAT3 pos;
		m_body[i]->get_position(&pos);
		XMVECTOR new_pos = XMLoadFloat3(&pos);
		new_pos += contact_normal * linear_move[i]; // faster to recalculate the same linear change that it would be to load the previously calculated value
		XMStoreFloat3(&pos, new_pos);
		m_body[i]->set_position(pos);
		// dont need to change the orientation because we are not calculating rotation
		// may not be necessary because original code only called this for sleeping objects
		m_body[i]->calculate_derived_data(); // function only updates rotational data in the rigidbody
	}
}
inline DirectX::XMFLOAT3 Contact::calculate_frictionless_impulse(/*DirectX::XMFLOAT3X3* inverse_inertia_tensor*/) { // calculates the impulse needed to resolve the contact given that the contact has no friction
	using namespace DirectX;
	XMFLOAT3 impulse_contact = { 0.0f, 0.0f, 0.0f };
	// build a vector that shows the change in velocity in world space coordinates for a unit impulse in the direction of the contact normal
	float delta_velocity = m_body[0]->get_inverse_mass(); // without rotational component, the delta velocity is directly proportional to the inverse mass
	if (m_body[1]) {
		delta_velocity += m_body[1]->get_inverse_mass();
	}
	impulse_contact.x = delta_velocity;
	return impulse_contact;
}
inline DirectX::XMFLOAT3 Contact::calculate_friction_impulse(/*DirectX::XMFLOAT3X3* inverse_inertia_tensor*/) { // impulse to solve the contact die to a non-zero coefficient of friction
	using namespace DirectX;
	XMFLOAT3 impulse_contact = { 0.0f, 0.0f, 0.0f };
	float inverse_mass = m_body[0]->get_inverse_mass();
	if (m_body[1]) inverse_mass += m_body[1]->get_inverse_mass();
	XMMATRIX delta_vel = XMMatrixIdentity();
	delta_vel *= inverse_mass;
	// invert to get the impulse needed per unit velocity
	XMVECTOR determinant;
	XMMATRIX impulse_matrix = XMMatrixInverse(&determinant, delta_vel);
	// find the target velocities to kill
	XMVECTOR velocity_kill = {m_desired_delta_velocity, -m_contact_velocity.y, -m_contact_velocity.z, 0.0f};
	// find the impulse required to kill the target velocities
	XMStoreFloat3(&impulse_contact, XMVector3Transform(velocity_kill, impulse_matrix));
	// check for exceeding friction
	float planar_impulse_sqr = impulse_contact.y * impulse_contact.y + impulse_contact.z * impulse_contact.z;
	if (planar_impulse_sqr > ((impulse_contact.x * m_friction) * (impulse_contact.x * m_friction))) { // overcame static friction
		// exceeding static friction so use dynamic friction
		float planar_impulse = sqrtf(planar_impulse_sqr);
		float inv_planar_impulse = 1.0f / planar_impulse;
		impulse_contact.y *= inv_planar_impulse;
		impulse_contact.z *= inv_planar_impulse;
		XMFLOAT3X3 delta_velocity;
		XMStoreFloat3x3(&delta_velocity, delta_vel);
		// cyclone uses row matrices, but I'm usign column befcause that's what direct x math uses
		impulse_contact.x = delta_velocity.m[0][0] + delta_velocity.m[1][0] * m_friction * impulse_contact.y + delta_velocity.m[2][0] * m_friction * impulse_contact.z;
		impulse_contact.x = m_desired_delta_velocity / impulse_contact.x;
		impulse_contact.y *= m_friction * impulse_contact.x;
		impulse_contact.z *= impulse_contact.y; //isotropic friction so the friction we apply is the same for the y and z direction
	}
	return impulse_contact;
}

//ContactResolver implementation
ContactResolver::ContactResolver(size_t iterations, float velocity_epsilon, float position_epsilon) {
	set_iterations(iterations, iterations);
	set_epsilon(velocity_epsilon, position_epsilon);
}
ContactResolver::ContactResolver(size_t velocity_iterations, size_t position_iterations, float velocity_epsilon, float position_epsilon) {
	set_iterations(velocity_iterations, position_iterations);
	set_epsilon(velocity_epsilon, position_epsilon);
}
void ContactResolver::set_iterations(size_t iterations) {
	set_iterations(iterations, iterations);
}
void ContactResolver::set_iterations(size_t velocity_iterations, size_t position_iterations) {
	m_max_velocity_iterations = velocity_iterations;
	m_max_position_iterations = position_iterations;
}
void ContactResolver::set_epsilon(float velocity_epsilon, float position_epsilon) {
	m_velocity_epsilon = velocity_epsilon;
	m_position_epsilon = position_epsilon;
}
void ContactResolver::resolve_contacts(Contact* contact_array, size_t contact_count, double duration) {
	// for each contact, preprocess it, resolve interpenetration, calculate and apply exit velocity
	if (contact_count == 0) return; // no contacts found
	if (!this->is_valid()) return; // out of iterations or the epsilons are wrong
	// preprocess the contacts so all the data is there when we start
	preprocess_contacts(contact_array, contact_count, duration);
	// resolve interpenetration of contacting objects
	adjust_positions(contact_array, contact_count, duration);
	// resolve the velocity of the contact
	adjust_velocities(contact_array, contact_count, duration);
}
void ContactResolver::preprocess_contacts(Contact* contact_array, size_t contact_count, double duration) { //readies all contacts in the array for processing, ensures internal data is correct and appropriate objects are alive
	const Contact* end = contact_array + contact_count;
	for (Contact* current = contact_array; current != end; ++current) {
		current->calculate_internals(duration);
	}
}
void ContactResolver::adjust_velocities(Contact* contact_array, size_t contact_count, double duration) { // resolves velocity issues constrained by velocity iterations
	using namespace DirectX;
	XMFLOAT3 velocity_change[g_num_bodies];
	assert(g_num_bodies == 2);
	const Contact* end = contact_array + contact_count;
	for (m_velocity_iterations_used = 0; m_velocity_iterations_used != m_max_velocity_iterations; ++m_velocity_iterations_used) { // keep processing until we run out of allowed iterations
		float max = m_velocity_epsilon; // only look for contacts whose velocity is above the threshold
		Contact* max_contact = nullptr;
		for (Contact* current = contact_array; current != end; ++current) {  // find the contact with the largest contact velocity
			if (current->m_desired_delta_velocity > max) {
				max = current->m_desired_delta_velocity;
				max_contact = current;
			}
		}
		if (!max_contact) return; // return if we didnt find a contact with velocity greater than epsilon
		// perform velocity compinent of collsiion resolution for the biggest collision
		max_contact->apply_velocity_change(velocity_change);
		// the update we performed changed some of the relative closing velocities for contacts involcing either one of our objects, and so need to be recomputed
		for (Contact* current = contact_array; current != end; ++current) { // for each contact
			for (size_t i = 0; i != g_num_bodies && current->m_body[i]; ++i) {// for each body in the contact
				for (size_t j = 0; j != g_num_bodies; ++j) { // check if either body in the current contact is one of the bodies we just updated
					if (current->m_body[i] == max_contact->m_body[j]) { // found matching RigidBody pointers, dont want to run this code if m_body[i] is nullptr, because if both are nullptrs then we cant do anything
						XMStoreFloat3(&current->m_contact_velocity, 
							(i?-1:1) * XMLoadFloat3(&current->m_contact_velocity) + 
							XMVector3Transform(XMLoadFloat3(&velocity_change[j]), XMMatrixTranspose(XMLoadFloat3x3(&current->m_contact_to_world))));
						current->calculate_desired_delta_velocity(duration);
					}
				}
			}
		}
	}
}
void ContactResolver::adjust_positions(Contact* contact_array, size_t contact_count, double duration) { // resolves position issues constrained by velocity iterations
	using namespace DirectX;
	XMFLOAT3 linear_change[g_num_bodies];
	float max_penetration;
	const Contact* end = contact_array + contact_count;
	for (m_position_iterations_used = 0; m_position_iterations_used != m_max_position_iterations; ++m_position_iterations_used) {
		// find the most severe penetration
		max_penetration = m_position_epsilon; // only consider penetrations greater than the epsilon, allows objects to interpenetrate slightly
		Contact* max_contact = nullptr;
		for (Contact* current = contact_array; current != end; ++current) {
			if (current->m_penetration > max_penetration) {
				max_penetration = current->m_penetration; // update the max
				max_contact = current;
			}
		}
		if (!max_contact) return;
		max_contact->apply_position_change(linear_change, max_penetration);
		// resolving penetration may have changed the current body's penetration into other bodies
		for (Contact* current = contact_array; current != end; ++current) {
			for (size_t i = 0; i != g_num_bodies && current->m_body[i]; ++i) {// for each body in the contact
				for (size_t j = 0; j != g_num_bodies; ++j) { // check if either body in the current contact is one of the bodies we just updated
					if (current->m_body[i] == max_contact->m_body[j]) { // found mathcing body pointers
						current->m_penetration += (i?1:-1) * XMVectorGetX(XMVector3Dot(XMLoadFloat3(&linear_change[j]), XMLoadFloat3(&current->m_contact_normal)));
						// the sign of the change is positive if we're dealing with the second body in a contact and negative otherwise 
					}
				}
			}
		}
	}
}