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
	m_collision_data->m_restitution = 0.2f; // 0.2 makes stacks stable
	m_collision_data->m_tolerance = 0.1f;
}

void CollisionEngine::start_frame(double duration) { // integrate all the objects, and update their internal data for collision detection
	for (auto& objs : *m_collision_objects) { // reset the forces on each body and calculate the necessary data
		if (!objs->get_awake()) continue; // if the object is asleep we can skip it
		// otherwise if it's awake it must have a rigidbody that is awake and we can integrate
		objs->m_body->clear_accumulators();
		objs->m_body->integrate(duration);
		objs->m_body->update();
		objs->m_box->transform();
		for (size_t i = 0; i != objs->m_obb_count; ++i) {
			objs->m_oriented_boxes[i].calculate_internals();
		}
	}
}
size_t CollisionEngine::broad_phase() { // apply the transforms 
	return m_bvh->get_potential_contacts(m_potential_contacts, m_max_contacts);
}
void CollisionEngine::narrow_phase(size_t num_contacts) {
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
}
void CollisionEngine::generate_contacts() {
	size_t contacts = broad_phase();
	return (contacts > 0) ? narrow_phase(contacts) : (void)0;
}
void CollisionEngine::run_frame(double duration) {
	
	m_bvh->update(); // update directly after integrating
	m_collision_data->reset(m_max_contacts); // reset the contacts for this frame
	// generate the contacts for this frame
	generate_contacts(); // data is in m_collision_data
	// resolve the generated contacts
	if (m_collision_data->m_contact_count == 0) return; // no collisions to resolve so we can exit early
	if (m_calculate_iterations) m_resolver->set_iterations(m_collision_data->m_contact_count * 4); // the number of iterations we perform is dependent on the nbumber of contacts
	m_resolver->resolve_contacts(m_collision_data->m_contact_base, m_collision_data->m_contact, duration);
}
void CollisionEngine::update(double duration) {
	start_frame(duration);
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