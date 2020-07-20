#pragma once
#include "RigidBody.h"
#include "Contact.h"
#include "NarrowCollide.h"
#include "BroadCollide.h"
#include <vector>
#include <array>
#include "CollisionObject.h" // contains gameobject and collision object

// eventually replace the PotentialContact structure in broad collide with this, and make the BVH use CollisionObjects in place of RigidBodies
struct PotentialCollision { // a potential contact is a pair of two collisionobjects, use collision objects so it's easy to go get associated gameobject, rigidbody, AABB, and OBBs
	CollisionObject* m_collision_object[2] = { nullptr, nullptr };
	PotentialCollision(CollisionObject* first, CollisionObject* second) {
		m_collision_object[0] = first;
		m_collision_object[1] = second;
	}

};
// represents an independent simulation of physics. it keeps track of a set of rigid bodies, and prvides the means to update them all
// engine should keep a copy of the CollisionEngine pointer and should build the BVH from the objects in the scene
// should be renamed to CollisionEngine
class CollisionEngine {
	bool m_calculate_iterations; // true if the CollisionEngine should calculate the number of iterations to give the contact resolver at each frame
	const size_t m_max_contacts;
	// const size_t m_max_potential_contacts;
	// use lists because iterators make this run much faster than a custom linked list
	// vector of bodies is temporary until we add the BVH, want to get collision working before I add it due to complexity
	// keep as vector until we replace with BVH, spend most time iterating over the CollisionObjects, so that is what should be fast, removing the collision object can be slower because that is done very infrequently
	std::vector<CollisionObject*>* m_collision_objects; // replace with a BVHNode of BoundingBoxes;
	// BVHNode<BoundingBox*>* m_bvh_root; // bvh of BoundingBoxes, pass a BoundingBox pointer so we use the pointer in the actual node instead of the BoundingBox itself, 
	// want to do this because the GameObject has ownership the boundingbox itself, use this pointer to read the up to date info
	ContactResolver* m_resolver;
	CollisionData m_collision_data; // stores the collision data for each frame, including the array of contacts. build by the collisiondetector
	//PotentialCollision* m_potential_contacts; // holds the array of potential contacts
	std::vector<PotentialCollision>* m_potential_contacts; // use a vector because we want an unlimited number of potential contacts 
	// because this is cheap to determine and false positives dont count for anything. also we still have the same number of actual contacts
	// size_t m_potential_contacts_count;
public:
	explicit CollisionEngine(size_t max_contacts = 256, size_t iterations = 0);
	~CollisionEngine() {
		delete m_collision_objects;
		delete m_resolver;
	}
	//size_t generate_contacts(); // calls each contact generator to report their contacts
	void run_frame(double duration); // 
	void start_frame(); // initializes the CollisionEngine for a simulation frame. clears the force and torque acculumators for bodies in the CollisionEngine
	void update(double duration); // app.cpp update
	void broad_phase(); // performs broadphase collision detection
	void narrow_phase(); // performs narrow phase collision detection
	size_t generate_contacts(); // eventually use the BVH, for now do O(n^2) search for collisions of CollisionObjects
	// void update_objects(); // could call a post physics update function. idk what would be in it because we only have RigidBody pointers
	// update_objects function originally called integrate, and calculate
	// just stores pointers, does not control ownership of any of the gameobject members
	void add_object(CollisionObject*); // function to add collision objects to the array of bodies
	void remove_object(CollisionObject*); // function to remove the collision object from the array of bodies 
};