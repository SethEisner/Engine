#pragma once
#include "Contact.h"
#include <DirectXCollision.h>
// contains narrow phase collision detection code. used to return contacts between pairs of primitives (OBBs)
// intersection tests use SAT to check if two objects intersect
// collision tests use the intersection tests as an early out and build the contacts

class IntersectionTests;
class CollisionDetector;

class OrientedBoundingBox { // base class for primitive to detect collisions against
public:
	friend class IntersectionTests;
	friend class CollisionDetector;
	DirectX::BoundingOrientedBox m_oriented_box;
	RigidBody* m_body; // the rigid body of the primitive
	// offset is probably not needed at all, as the ribidbody should be in Mesh space
	// DirectX::XMFLOAT4X4 m_offset; // the offset of this primitive from the given rigid body
	DirectX::XMFLOAT4X4 m_transform; // transform from submesh (vertices) to mesh space. same as submesh transform
	//the resultant transform of the primitive. calculated by combining the offset of the primitive with the transform of the rigid body;
	void calculate_internals();
	DirectX::XMFLOAT3 get_axis(uint32_t index) const {
		// get's a row of the transformation matrix, return a row because if we want the translation of the transformation matrix, that would be the last row of the column matrix
		return DirectX::XMFLOAT3(m_transform.m[index][0], m_transform.m[index][1], m_transform.m[index][2]);
	}
	const DirectX::XMFLOAT4X4& get_transform() const {
		return m_transform;
	}
	void get_corners(DirectX::XMFLOAT3* corners) {
		m_oriented_box.GetCorners(corners); // get the corners of the OBB we are wrapping
	}
// protected:
	
};

// struct OrientedBoundingBox : public CollisionPrimitive {
// 	DirectX::BoundingOrientedBox m_oriented_box; // use an oriented bounding box for narrow phase collision
// };

class IntersectionTests {
public:
	static bool intersects(const OrientedBoundingBox& first, const OrientedBoundingBox& second); // only determine if two OBBs intersect
};

// structure that contains data the collision detector will use to build the contacts
struct CollisionData {
	// should probably use a vector of Contact* so we can use iterators...
	Contact* m_contact_base; // holds the base of the collision data
	Contact* m_contacts; // holds the pointer to the next contact struct we can write into
	int m_contacts_left;
	uint32_t m_contact_count; // the number of contacts in the contact array
	float m_friction; // should probably not use friction for now
	float m_restitution; // 
	float m_tolerance; // tolerance of a collision, allows us to mark nearly colliding objects as colliding so there isnt a large interpenetration on the next frame
	inline bool has_more_contacts() {
		return m_contacts_left > 0;
	}
	void reset(uint32_t max_contacts) {
		m_contacts_left = max_contacts;
		m_contact_count = 0;
		m_contacts = m_contact_base;
	}
	void add_contacts(uint32_t count) {
		m_contacts_left -= count;
		m_contact_count += count;
		m_contacts += count; // move our pointer into the array forward by the number of contacts we added
	}
};

struct CollisionDetector {
	// does a collision test between two oriented boxes and 
	static uint32_t collides(const OrientedBoundingBox& first, const OrientedBoundingBox& second, CollisionData* data);
};