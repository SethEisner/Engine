#pragma once
#include "Contact.h"
#include <DirectXCollision.h>
// contains narrow phase collision detection code. used to return contacts between pairs of primitives (OBBs)
// intersection tests use SAT to check if two objects intersect
// collision tests use the intersection tests as an early out and build the contacts

class IntersectionTests;
class CollisionDetector;
class RigidBody;
class Mesh;
class SubMesh;
class Vertex;
class OrientedBoundingBox { // base class for primitive to detect collisions against
public:
	friend class IntersectionTests;
	friend class CollisionDetector;
	DirectX::BoundingOrientedBox m_oriented_box;
	DirectX::BoundingOrientedBox m_world_box;
	RigidBody* m_body; // the rigid body of the primitive
	// offset is probably not needed at all, as the ribidbody should be in Mesh space
	// DirectX::XMFLOAT4X4 m_offset; // the offset of this primitive from the given rigid body
	DirectX::XMFLOAT4X4 m_world_transform; // transform from submesh space (what the oriented box is built from) to world space

	//the resultant transform of the primitive. calculated by combining the offset of the primitive with the transform of the rigid body;
	void calculate_internals();
	DirectX::XMFLOAT3 get_axis(uint32_t index) const {
		// get's a row of the transformation matrix, return a row because if we want the translation of the transformation matrix, that would be the last row of the column matrix
		return { m_world_transform.m[index][0], m_world_transform.m[index][1], m_world_transform.m[index][2] };
	}
	const DirectX::XMFLOAT4X4& get_transform() const {
		return m_world_transform;
	}
	void get_corners(DirectX::XMFLOAT3* corners) {
		m_oriented_box.GetCorners(corners); // get the corners of the OBB we are wrapping
	}
	void get_world_corners(DirectX::XMFLOAT3* corners) {
		m_world_box.GetCorners(corners); // get the corners of the OBB we are wrapping
	}
	void transform() { // calculate the oriented box in world space
		m_oriented_box.Transform(m_world_box, XMLoadFloat4x4(&m_world_transform));
	}
	void calculate_transform() {
		// transforms the 
		using namespace DirectX;
		XMStoreFloat4x4(&m_world_transform, XMMatrixMultiply(XMLoadFloat4x4(m_submodel_to_model), XMMatrixMultiply(XMLoadFloat4x4(m_model_to_gameobject), XMLoadFloat4x4(m_gameobject_to_world))));
	}
	const DirectX::XMFLOAT4X4& get_model_transform() {
		return *m_submodel_to_model;
	}
	bool intersects(DirectX::XMVECTOR& origin, DirectX::XMVECTOR& direction, float& distance) const {
		// return true if we found an intersection with the ray and the intersection at all and return the distance of the intersections
		return m_world_box.Intersects(origin, direction, distance);
	}
	bool intersects(DirectX::XMVECTOR& origin, DirectX::XMVECTOR& direction, float min_dist, float max_dist) const {
		// return true if we found an intersection with the ray and the intersection within the distance range provided
		float distance = 0.0f;
		bool intersects = m_world_box.Intersects(origin, direction, distance);
		return (intersects && distance <= max_dist && distance >= min_dist);
		// return (m_world_box.Intersects(origin, direction, distance) && distance <= max_dist && distance >= min_dist);
	}
	void create_from_points(size_t vertex_count, Vertex* vertex_buffer, size_t base_vertex, size_t start_index);
	void set_transform_pointers(GameObject* obj, Mesh* mesh, SubMesh* submesh);
private:
	DirectX::XMFLOAT4X4* m_submodel_to_model; // could combine these two matrices into one because as of now they dont change, only the gameobject one gets changed
	DirectX::XMFLOAT4X4* m_model_to_gameobject;
	DirectX::XMFLOAT4X4* m_gameobject_to_world;
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
	Contact* m_contact; // holds the pointer to the next contact struct we can write into
	int m_contacts_left;
	uint32_t m_contact_count; // the number of contacts in the contact array
	// float m_friction; // should probably not use friction for now
	// float m_restitution; // 
	float m_tolerance; // tolerance of a collision, allows us to mark nearly colliding objects as colliding so there isnt a large interpenetration on the next frame
	explicit CollisionData(size_t max_contacts) : m_contact_base(new Contact[max_contacts]), m_contact(m_contact_base), m_contacts_left(max_contacts), m_contact_count(0) {}
	inline bool has_more_contacts() {
		return m_contacts_left > 0;
	}
	void reset(uint32_t max_contacts) {
		m_contacts_left = max_contacts;
		m_contact_count = 0;
		m_contact = m_contact_base;
	}
	void add_contacts(uint32_t count) {
		m_contacts_left -= count;
		m_contact_count += count;
		m_contact += count; // move our pointer into the array forward by the number of contacts we added
	}
};

struct CollisionDetector {
	// does a collision test between two oriented boxes and 
	static uint32_t collides(const OrientedBoundingBox& first, const OrientedBoundingBox& second, CollisionData* data);
};