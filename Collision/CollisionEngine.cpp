#include "CollisionEngine.h"
//#include <cstdlib>
#include <algorithm>
#include <math.h>

CollisionEngine::CollisionEngine(size_t max_contacts, size_t iterations) :
		m_calculate_iterations(iterations == 0),
		m_max_contacts(max_contacts),
		m_collision_objects(new std::vector<CollisionObject*>()),
		m_resolver(new ContactResolver(iterations)),
		m_collision_data(new CollisionData(m_max_contacts)),
		// m_potential_contacts(new std::vector<PotentialCollision>()),
	m_potential_contacts(new PotentialContact[m_max_contacts]),
		m_bvh(nullptr) {// new BVHNode<BoundingBox>(nullptr, std::move(BoundingBox()) ,nullptr)) {
	m_collision_data->reset(m_max_contacts);
	m_collision_data->m_friction = 0.9f;
	m_collision_data->m_restitution = 0.3f;
	m_collision_data->m_tolerance = 0.1f;
}

void CollisionEngine::start_frame() {
	for (auto& objs : *m_collision_objects) { // reset the forces on each body and calculate the necessary data
		if (objs->m_body) objs->m_body->clear_accumulators();
		// objs->m_body->calculate_derived_data();
	}
	
}
size_t CollisionEngine::broad_phase() { // apply the transforms 
	 return m_bvh->get_potential_contacts(m_potential_contacts, m_max_contacts);
}
void CollisionEngine::narrow_phase(size_t num_contacts) {
	m_collision_data->reset(m_max_contacts); // reset the contacts
	// for every potential collision
	for (auto curr = m_potential_contacts; curr != m_potential_contacts + num_contacts; ++curr) {
		auto first = curr->m_collision_object[0];
		auto second = curr->m_collision_object[1];
		for (size_t i = 0; i != first->m_obb_count; ++i) {
			for (size_t j = 0; j != second->m_obb_count; ++j) {
				if (!m_collision_data->has_more_contacts()) return;
				CollisionDetector::collides(first->m_oriented_boxes[i], second->m_oriented_boxes[j], m_collision_data); // element wise check to array for overlap
			}
		}
	}
	// m_collion data holds all the collisions for this frame
}
void CollisionEngine::generate_contacts() {
	// perform broad phase collision detection with the bouding volume hierarchy
	size_t potential_contact_count = m_bvh->get_potential_contacts(m_potential_contacts, m_max_contacts); // build the array of potential contacts
	// perform narrow phase collision on every potential contact
	for (auto curr = m_potential_contacts; curr != m_potential_contacts + potential_contact_count; ++curr) {
		auto first = curr->m_collision_object[0];
		auto second = curr->m_collision_object[1];
		// check for collision between every pair of OBBs between the two collision objects
		for (size_t i = 0; i != first->m_obb_count; ++i) {
			for (size_t j = 0; j != second->m_obb_count; ++j) {
				if (!m_collision_data->has_more_contacts()) return; // we have run out of contacts we are allowed to generate
				CollisionDetector::collides(first->m_oriented_boxes[i], second->m_oriented_boxes[j], m_collision_data); // element wise check to array for overlap
			}
		}
	}
}
void CollisionEngine::run_frame(double duration) {
	// integrate all the objects
	for (auto& objs : *m_collision_objects) {
		if (objs->m_body) objs->m_body->integrate(duration);
		objs->m_box->transform(); // update m_transformed_box so we can get the correct result from broad_phase collision
		for (size_t i = 0; i != objs->m_obb_count; ++i) {
			objs->m_oriented_boxes[i].calculate_internals();
		}
	}
	m_collision_data->reset(m_max_contacts); // reset the contacts for this frame
	// generate the contacts for this frame
	generate_contacts(); // data is in m_collision_data
	// resolve the generated contacts
	if (m_calculate_iterations) m_resolver->set_iterations(m_collision_data->m_contact_count * 4); // the number of iterations we perform is dependent on the nbumber of contacts
	m_resolver->resolve_contacts(m_collision_data->m_contact_base, m_collision_data->m_contact, duration);
	for (auto& objs : *m_collision_objects) { // update all rigidbodies everyframe because we integrated all the bodies
		objs->m_body->update();
	}
}
void CollisionEngine::update(double duration) {
	start_frame();
	run_frame(duration);
}
void CollisionEngine::add_object(CollisionObject* coll_obj) {
	m_collision_objects->push_back(coll_obj);
	if(m_bvh) {
		m_bvh->insert(coll_obj);
		return;
	}
	m_bvh = new BVHNode<BoundingBox>(nullptr, *coll_obj->m_box, coll_obj);
}
void CollisionEngine::remove_object(CollisionObject* coll_obj) {
	// probably okay for this to be slower, because we should rarely be removing collision objects, as collision objects are tied to gameobjects
	// m_collision_objects->erase(find(m_collision_objects->begin(), m_collision_objects->end(), coll_obj));
}