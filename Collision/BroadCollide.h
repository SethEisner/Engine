#pragma once
#include "RigidBody.h"
#include "CollisionObject.h"
#include "Contact.h"
#include <DirectXCollision.h>
#include <memory>

class CollisionObject;
class GameObject;
// broad-phase collision detection using a BVH of spheres because it's super fast

// float because DirectX collision volumes store their geometric attributes in floats
static constexpr float four_pi = 2.566370614359172953850573533118;
static constexpr float four_over_three_pi = 4.1887902047863909846168578443727;
struct BoundingSphere {
	DirectX::BoundingSphere m_sphere;
	BoundingSphere(const DirectX::XMFLOAT3& center, float radius);
	BoundingSphere(const BoundingSphere& first, const BoundingSphere& second);
	bool overlaps(const BoundingSphere& other) const;
	float get_growth(const BoundingSphere& other) const; // returns how much the bs would have to grow to incorporate the given bounding sphere , should be percentage growth in surface area
	inline float get_size() const { // size is the volume
		return four_over_three_pi * m_sphere.Radius * m_sphere.Radius * m_sphere.Radius;
	}
};
struct BoundingBox { // AABB
private: // to be private should have an extents and center function
	DirectX::BoundingBox m_box; // the AABB in model space that we transform from so we dont get acculuated errors
	DirectX::BoundingBox m_world_box; // the AABB in world space for comapring
	GameObject* m_game_object = nullptr;
public:
	
	// std::shared_ptr<DirectX::XMFLOAT4X4> m_transform; // same transform as in Mesh and also RigidBody
	DirectX::XMFLOAT4X4* m_transform = nullptr; // get's set when we create the CollisionObject, transform from model space to world space
	// DirectX::XMFLOAT4X4* m_model_transform;
	// DirectX::XMFLOAT4X4* m_world_transform;
	// BoundingBox();
	BoundingBox();
	BoundingBox(GameObject* obj);
	// BoundingBox(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& extents) : m_box(center, extents) {}
	BoundingBox(const BoundingBox& first, const BoundingBox& second);
	inline bool overlaps(const BoundingBox& other) const { // check for intersection in world space
		return this->m_world_box.Intersects(other.m_world_box);
	}
	float get_growth(const BoundingBox& other) const;
	inline float get_size() const { // volume of the box, can determine growth from the volume delta
		return fabsf(8.0f * m_box.Extents.x * m_box.Extents.y * m_box.Extents.z); // extents is distance from center to edge, so need to multiply by 8
	}
	void create_from_points(size_t count, const DirectX::XMFLOAT3* points, size_t stride) {
		m_box.CreateFromPoints(m_box, count, points, stride);
	}
	// create merged is wrong, if we merge two bounding boxes into a single one, we should do it based on their transformed bounding box, and set our transform to be a nullptr
	// the transform we store is from model space to world space, if we merge two bounding boxes, what do we do with the gameobjects? doesnt make sense to merge boundingboxes if they are tied to gameobjects...
	void create_merged(const BoundingBox& other) { // merge this BoundingBox with another bounding box, should be in the same space because we dont change the transform
		// m_box.CreateMerged(this->m_box, this->m_world_box, other.m_world_box);
		// XMStoreFloat4x4(m_transform, XMLoadFloat4x4(&m_game_object->m_transform)); // store the identity matrix for the transfrom as the new box is in world space
		// m_world_box = m_box;
		m_box.CreateMerged(this->m_box, this->m_box, other.m_box);
	}
	// needs to be in ccp file because it uses the CollisionObject pointer
	void transform(); // applied the internal transform, and then the GameObject transform
	void transform(const DirectX::XMFLOAT4X4& M) {
		this->m_box.Transform(this->m_world_box, DirectX::XMLoadFloat4x4(&M));
	}
	void set_game_object(GameObject* obj) {
		m_game_object = obj;
	}
};

// holds two bodies that might be in contact
// struct PotentialContact {
// 	RigidBody* m_body[2];
// };

struct PotentialContact { // a potential contact is a pair of two collisionobjects, use collision objects so it's easy to go get associated gameobject, rigidbody, AABB, and OBBs
	CollisionObject* m_collision_object[2] = { nullptr, nullptr };
	PotentialContact() : m_collision_object{ nullptr, nullptr } {}
	PotentialContact(CollisionObject* first, CollisionObject* second) : m_collision_object{ first, second } {}
};

// change the hierarchy to use an AABB instead of a sphere hierarchy
template<class BoundingVolume>
class BVHNode { // node of the bounding volume hierarchy, hierarchy is a binary tree
public:
	BVHNode* m_children[2]; // if leaf then both children will be NULL
	BoundingVolume m_volume; // pointer to the boundingvolume in the CollisionObject
	CollisionObject* m_collision_object; // only leaf nodes have a rigid body
	// could a leaf node not have a rigid body? scenery doesnt have a rigidbody but does have a BoundingBox
	BVHNode* m_parent; // parent node useful for tree manipulation operations
	// BVHNode(BVHNode* parent, BoundingVolume* volume, CollisionObject* body = nullptr)
	// 	: m_parent(parent), m_volume(volume), m_body(body), m_children({ nullptr, nullptr }) {
	// }
	BVHNode(BVHNode* parent, const BoundingVolume& volume, CollisionObject* obj = nullptr)
		: m_parent(parent), m_volume(volume), m_collision_object(obj), m_children{ nullptr, nullptr } {
	}
	~BVHNode();
	inline bool is_leaf() const {
		return m_collision_object != nullptr;
	}
	inline bool is_root() const {
		return m_parent == nullptr;
	}
	size_t get_potential_contacts(PotentialContact* contacts, size_t limit) const; // build the array of potential contacts
	// void insert(RigidBody* body, const BoundingVolume& volume);
	void insert(CollisionObject* coll_obj); // collision objects have their own BoundingVolume
protected:
	bool overlaps(const BVHNode<BoundingVolume>* other) const; // checks if two nodes overlap, should call the overlaps method of the BoundingVolume
	size_t get_potential_contacts_with(const BVHNode<BoundingVolume>* other, PotentialContact* contacts, size_t limit) const;
	void recalculate_bounding_volume(); // recalculate the bounding volume of a node based on its children
};

template<class BoundingVolume>
bool BVHNode<BoundingVolume>::overlaps(const BVHNode<BoundingVolume>* other) const {
	// BoundingVolume first = *this;
	// BoundingVolume second = *other;
	// if (this->is_leaf()) { // if either node is a leaf node, we need to transform that node by it's transform, and then check the overlap
	// 	this->m_volume.transform(first, DirectX::XMLoadFloat4x4(this->m_transform));
	// }
	// if (other->is_leaf()) {
	// 	other->m_volume.transform(second, DirectX::XMLoadFloat4x4(other->m_transform));
	// }
	// internal nodes should not be transformed, or should have a transform of the identity matrix.
	// this is because, when we create them we want them to encapsulate both children so there isnt a specific transform to use. 
	// also, both of the children will have transforms that we need to encapsulate
	return m_collision_object->m_box->overlaps(*other->m_collision_object->m_box); // call the overlap function for the Bounding volume we use for the hierarchy
}

/*
template<class BoundingVolume>
void BVHNode<BoundingVolume>::insert(RigidBody* body, const BoundingVolume& volume) {
	if (is_leaf()) { // if we are a leaf then we must create two new children and make ourselves an internal node
		// create two new children with ourselves as the parent
		// the first child has our volume and body
		// the second child will have the volume and body we want to insert
		m_children[0] = new BVHNode<BoundingVolume>(this, m_volume, m_body); // copies the transform internal to m_volume
		m_children[1] = new BVHNode<BoundingVolume>(this, volume, body); // copies the transform internal to volume
		this->m_body = nullptr; // set to nullptr so we can be classified as an internal node
		DirectX::XMStoreFloat4x4(this->m_volume.m_transform, DirectX::XMMatrixIdentity());
		recalculate_bounding_volume(); // reclaculate our bounding volume using our children's bounding volumes
	}
	else { // we need to recurse the tree to find the leaf node to insert into
		// need to pick which child to insert the new node into by which one would grow the least
		if (m_children[0]->m_volume.get_growth(volume) < m_children[1]->m_volume.get_growth(volume)) {
			m_children[0]->insert(body, volume);
		}
		else {
			m_children[1]->insert(body, volume);
		}
	}
}
*/
template<class BoundingVolume>
void BVHNode<BoundingVolume>::insert(CollisionObject* coll_obj) {
	if (is_leaf()) { // if we are a leaf then we must create two new children and make ourself the internal node
		// our leaf data becomes the first child and the other child is the object we are inserting
		// then we become the parent for our two children
		m_children[0] = new BVHNode<BoundingVolume>(this, m_volume, m_collision_object); // copy our data to the first child
		m_children[1] = new BVHNode<BoundingVolume>(this, *coll_obj->m_box, coll_obj); // insert the new collision object as our second child
		this->m_collision_object = nullptr; // set the collision object to nullptr as only leaf nodes have collision objects
		DirectX::XMStoreFloat4x4(this->m_volume.m_transform, DirectX::XMMatrixIdentity());
		recalculate_bounding_volume();
	}
	else { // we are an internal node with children so we need to pick the correct children to recurse into to find the best node to insert the collision object
		// the best child to recurse into is the one that will grow the least
		if (m_children[0]->m_volume.get_growth(*coll_obj->m_box) < m_children[1]->m_volume.get_growth(*coll_obj->m_box)) {
			m_children[0]->insert(coll_obj);
		}
		else {
			m_children[1]->insert(coll_obj);
		}
	}
}

template<class BoundingVolume>
BVHNode<BoundingVolume>::~BVHNode() {
	// delete the BVH from the root node down, the root has no parent so we just process the children and not the sibling
	if (m_parent) {
		BVHNode<BoundingVolume>* sibling;
		// if we are the first child then the second child is our sibling and vice-versa
		if (m_parent->children[0] == this) sibling = m_parent->children[1];
		else sibling = m_parent->children[0];

		// our sibling needs to become our parent
		// write its data to our parent
		m_parent->m_volume = sibling->m_volume;
		m_parent->m_body = sibling->m_body;
		m_parent->m_children[0] = sibling->m_children[0];
		m_parent->m_children[1] = sibling->m_children[1];

		// delete the sibling from the hierarchy
		sibling->m_parent = nullptr;
		sibling->m_body = nullptr;
		sibling->m_children[0] = nullptr;
		sibling->m_children[1] = nullptr;
		delete sibling;

		m_parent->recalculate_bounding_volume();
	}
	if (m_children[0]) {
		m_children[0]->m_parent = nullptr;
		delete m_children[0];
	}
	if (m_children[1]) {
		m_children[1]->m_parent = nullptr;
		delete m_children[1];
	}
}
// recurse is not used...
template<class BoundingVolume>
void BVHNode<BoundingVolume>::recalculate_bounding_volume() { // O(log(n)) 
	if (is_leaf()) return;
	// create a new bounding volume that encompases our children
	m_volume = BoundingVolume(m_children[0]->m_volume, m_children[1]->m_volume);
	if (m_parent) m_parent->recalculate_bounding_volume(); // recalculate the bounding volume up the tree
}

template<class BoundingVolume>
size_t BVHNode<BoundingVolume>::get_potential_contacts(PotentialContact* contacts, size_t limit) const {
	if (is_leaf() || limit == 0) return 0; // return if we dont have any more room for contacts or we are a leaf ( if root is a leaf then we could never have any contacts)
	return m_children[0]->get_potential_contacts_with(m_children[1], contacts, limit);
}

template<class BoundingVolume>
size_t BVHNode<BoundingVolume>::get_potential_contacts_with(const BVHNode<BoundingVolume>* other, PotentialContact* contacts, size_t limit) const {
	if (!overlaps(other) || limit == 0) return 0; // if nether BVH overlaps then we dont need to proceed further
	if (is_leaf() && other->is_leaf()) { // if we are both leaf nodes then there is one potential contact because the overlap call above us passed
		contacts->m_collision_object[0] =  this->m_collision_object;
		contacts->m_collision_object[1] = other->m_collision_object;
		return 1;
	}
	// determine which brach to descend into, if either is a leaf then descend into the other, otherwise descend into the larger one
	// descend into the larger one incase we pass the limit from it
	if (other->is_leaf() || (!is_leaf() && m_volume.get_size() >= other->m_volume.get_size())) {
		size_t count = m_children[0]->get_potential_contacts_with(other, contacts, limit);
		if (limit > count) return count + m_children[1]->get_potential_contacts_with(other, contacts + count, limit - count);
		return count;
	}
	else {
		size_t count = get_potential_contacts_with(other->m_children[0], contacts, limit);
		if (limit > count) return count + get_potential_contacts_with(other->m_children[1], contacts + count, limit - count);
		return count;
	}
}