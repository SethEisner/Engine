#include "CollisionEngine.h"
//#include <cstdlib>
#include <algorithm>
#include <math.h>

CollisionEngine::CollisionEngine(size_t max_contacts, size_t iterations) :
	m_calculate_iterations(iterations == 0), 
	m_max_contacts(max_contacts), 
	m_bodies(new std::vector<Body*>()), 
	m_resolver(new ContactResolver(iterations)), 
	m_contact_generators(new std::vector<ContactGenerator*>()), 
	m_contacts(new Contact[m_max_contacts]) {}
CollisionEngine::~CollisionEngine() {
	delete m_bodies;
	delete m_contact_generators;
	delete[] m_contacts;
}
void CollisionEngine::start_frame() {
	for (auto& body : *m_bodies) { // reset the forces on each body and calculate the necessary data
		body->clear_accumulators();
		body->calculate_derived_data();
	}
}
size_t CollisionEngine::generate_contacts() {
	int64_t limit = m_max_contacts;
	Contact* next_contact = m_contacts;
	for (auto& contact_generator : *m_contact_generators) {
		size_t num_contacts = contact_generator->add_contact(next_contact, limit);
		limit -= num_contacts;
		next_contact += num_contacts;
		if (limit <= 0) break; // run out of room in our contact array, for performace reasons we stop here
	}
	return m_max_contacts - limit;
}
void CollisionEngine::run_physics(double duration) {
	// integrate all the objects
	for (auto& body : *m_bodies) {
		body->integrate(duration);
	}
	// generate the contacts for this frame
	size_t num_contacts = generate_contacts();
	// resolve the generated contacts
	if (m_calculate_iterations) m_resolver->set_iterations(std::min(static_cast<size_t>(10000), num_contacts * 4)); // the number of iterations we perform is dependent on the nbumber of contacts
	m_resolver->resolve_contacts(m_contacts, num_contacts, duration);
}