#pragma once
#include "Body.h"
#include "Contact.h"
#include <list>
#include <vector>

// represents an independent simulation of physics. it keeps track of a set of rigid bodies, and prvides the means to update them all
// engine should keep a copy of the world pointer and should build the BVH from the objects in the scene

class World {
	bool m_calculate_iterations; // true if the world should calculate the number of iterations to give the contact resolver at each frame
	// use lists because iterators make this run much faster than a custom linked list
	std::vector<Body*>* m_bodies; // a list of bodies in our scene, should remain a list incase we need to do arbitrary insertions
	ContactResolver m_resolver;
	std::vector<ContactGenerator*>* m_generators; // the list of contact generators, would want multiple contact generators for diffent interactions (floating in water vs walking into a wall)
	Contact* contacts; // holds the array of contacts
	size_t m_max_contacts;
public:
	explicit World(size_t max_contacts, size_t iterations = 0);
	~World();
	size_t generate_contacts(); // calls each contact generator to report their contacts
	void run_physics(double duration); // should do nothing for now
	void start_frame(); // initializes the world for a simulation frame. clears the force and torque acculumators for bodies in the world
};