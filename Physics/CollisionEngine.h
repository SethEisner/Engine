#pragma once
#include "Body.h"
#include "Contact.h"
#include <vector>
#include <array>

// represents an independent simulation of physics. it keeps track of a set of rigid bodies, and prvides the means to update them all
// engine should keep a copy of the CollisionEngine pointer and should build the BVH from the objects in the scene
// should be renamed to CollisionEngine
class CollisionEngine {
	bool m_calculate_iterations; // true if the CollisionEngine should calculate the number of iterations to give the contact resolver at each frame
	const size_t m_max_contacts;
	// use lists because iterators make this run much faster than a custom linked list
	std::vector<Body*>* m_bodies; // a list of bodies in our scene, should remain a list incase we need to do arbitrary insertions
	ContactResolver* m_resolver;
	std::vector<ContactGenerator*>* m_contact_generators; // the list of contact generators, would want multiple contact generators for diffent interactions (floating in water vs walking into a wall)
	Contact* m_contacts; // holds the array of contacts
public:
	explicit CollisionEngine(size_t max_contacts, size_t iterations = 0);
	~CollisionEngine();
	size_t generate_contacts(); // calls each contact generator to report their contacts
	void run_physics(double duration); // 
	void start_frame(); // initializes the CollisionEngine for a simulation frame. clears the force and torque acculumators for bodies in the CollisionEngine
};