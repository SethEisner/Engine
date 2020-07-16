#include "CollisionEngine.h"
//#include <cstdlib>
#include <algorithm>
#include <math.h>

CollisionEngine::CollisionEngine(size_t max_contacts, size_t iterations) :
	m_calculate_iterations(iterations == 0),
	m_max_contacts(max_contacts),
	// m_max_potential_contacts(max_contacts * 2), // allows for false positives
	m_collision_objects(new std::vector<CollisionObject*>()),
	m_resolver(new ContactResolver(iterations)),
	m_potential_contacts(new std::vector<PotentialCollision>()) {}
	//m_potential_contacts_count(0)

void CollisionEngine::start_frame() {
	for (auto& objs : *m_collision_objects) { // reset the forces on each body and calculate the necessary data
		objs->m_body->clear_accumulators();
		objs->m_body->calculate_derived_data();
	}
	
}
void CollisionEngine::broad_phase() {
	m_potential_contacts->clear();
	for (auto i = m_collision_objects->begin(); i != m_collision_objects->end(); ++i) {
		for (auto j = i + 1; j != m_collision_objects->end(); ++j) {
			if ((*i)->m_box->overlaps(*(*j)->m_box)) {
				m_potential_contacts->emplace_back(PotentialCollision({ (*i), (*j) }));
			}
		}
	}
	return;
}
void CollisionEngine::narrow_phase() {
	m_collision_data.reset(m_max_contacts);
	m_collision_data.m_friction = 0.9f;
	m_collision_data.m_restitution = 0.6f;
	m_collision_data.m_tolerance = 0.1f;
	// for every potential collision
	for (auto curr = m_potential_contacts->begin(); curr != m_potential_contacts->end(); ++curr) {
		auto first = curr->m_collision_object[0];
		auto second = curr->m_collision_object[1];
		for (size_t i = 0; i != first->m_obb_count; ++i) {
			for (size_t j = 0; i != second->m_obb_count; ++j) {
				if (!m_collision_data.has_more_contacts()) return;
				CollisionDetector::collides(first->m_oriented_boxes[i], second->m_oriented_boxes[j], &m_collision_data);
			}
		}
	}
	// m_collion data holds all the collisions for this frame
}
size_t CollisionEngine::generate_contacts() {
	// build the array of potential contacts, and set the m_potential_contacts_count member variable
	broad_phase(); // perform broad phase
	narrow_phase(); // perform narrow phase
	return m_collision_data.m_contact_count;
}
void CollisionEngine::run_frame(double duration) {
	// integrate all the objects
	for (auto& objs : *m_collision_objects) {
		objs->m_body->integrate(duration);
	}
	// generate the contacts for this frame
	size_t num_contacts = generate_contacts();
	// resolve the generated contacts
	if (m_calculate_iterations) m_resolver->set_iterations(num_contacts * 4); // the number of iterations we perform is dependent on the nbumber of contacts
	m_resolver->resolve_contacts(m_collision_data.m_contacts, num_contacts, duration);
}
void CollisionEngine::update(double duration) {
	start_frame();
	run_frame(duration);
}
void CollisionEngine::add_object(CollisionObject* coll_obj) {
	m_collision_objects->push_back(coll_obj);
}
void CollisionEngine::remove_object(CollisionObject* coll_obj) {
	// probably okay for this to be slower, because we should rarely be removing collision objects, as collision objects are tied to gameobjects
	m_collision_objects->erase(find(m_collision_objects->begin(), m_collision_objects->end(), coll_obj));
}