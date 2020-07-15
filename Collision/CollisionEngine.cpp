#include "CollisionEngine.h"
//#include <cstdlib>
#include <algorithm>
#include <math.h>

CollisionEngine::CollisionEngine(size_t max_contacts, size_t iterations) :
	m_calculate_iterations(iterations == 0),
	m_max_contacts(max_contacts),
	m_max_potential_contacts(max_contacts * 2), // allow for 
	m_collision_objects(new std::vector<CollisionObject*>()),
	m_resolver(new ContactResolver(iterations)),
	m_potential_contacts(new PotentialCollision[m_max_potential_contacts]),
	m_potential_contacts_count(0) {
}
void CollisionEngine::start_frame() {
	for (auto& objs : *m_collision_objects) { // reset the forces on each body and calculate the necessary data
		objs->m_body->clear_accumulators();
		objs->m_body->calculate_derived_data();
	}
}
size_t CollisionEngine::generate_contacts() {
	m_potential_contacts_count = 0;
	// build the array of potential contacts, and set the m_potential_contacts_count member variable
	// traverse the array of potential contacts until we run out of contacts,
	//		pairwise compare the OBBs of each collision object to determine if actual overlap, use NarrowCollide.cpp // have an overlap function
	//		if there's a collision, NarrowCollide.cpp uses our CollisionData member and fills out the structure
	// we return the number of collisions we found


	// use the vector of collision objects, and store what we find into the 
	// int64_t limit = m_max_contacts;
	// Contact* next_contact = m_contacts;
	// for (auto& contact_generator : *m_contact_generators) {
	// 	size_t num_contacts = contact_generator->add_contact(next_contact, limit);
	// 	limit -= num_contacts;
	// 	next_contact += num_contacts;
	// 	if (limit <= 0) break; // run out of room in our contact array, for performace reasons we stop here
	// }
	// return m_max_contacts - limit;
}
void CollisionEngine::run_physics(double duration) {
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
void CollisionEngine::add_object(CollisionObject* coll_obj) {

}
void CollisionEngine::remove_object(CollisionObject* coll_obj) {

}