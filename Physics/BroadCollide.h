#pragma once
#include "Body.h"
#include "Contact.h"
#include <DirectXCollision.h>
// broad-phase collision detection using a BVH of spheres because it's super fast


static constexpr double four_pi = 2.566370614359172953850573533118;
static constexpr double four_over_three_pi = 4.1887902047863909846168578443727;
struct BoundingSphere {
	DirectX::BoundingSphere m_sphere;
	BoundingSphere(const DirectX::XMFLOAT3& center, float radius);
	BoundingSphere(const BoundingSphere& first, const BoundingSphere& second);
	bool overlaps(const BoundingSphere& other) const;
	float get_growth(const BoundingSphere& other) const; // returns how much the bs would have to grow to incorporate the given bounding sphere , should be percentage growth in surface area
	inline float get_volume() const {
		return four_over_three_pi * m_sphere.Radius * m_sphere.Radius * m_sphere.Radius;
	}
	inline float get_size() const { // calculate the surface area of the sphere
		return four_pi * m_sphere.Radius * m_sphere.Radius;
	}
};
struct BoundingBox {
	DirectX::BoundingBox m_box;
	BoundingBox();
	BoundingBox(const BoundingBox& first, const BoundingBox& second);
	bool overlaps(const BoundingBox& other) const;
	float get_growth(const BoundingBox& other) const;
	inline float get_size() const { // volume of the box, can determine growth from the volume delta
		return m_box.Extents.x * m_box.Extents.y * m_box.Extents.z;
	}
};


// holds two bodies that might be in contact
struct PotentialContact {
	Body* m_body[2];
};



// change the hierarchy to use an AABB instead of a sphere hierarchy
template<class BoundingVolume>
class BVHNode { // node of the bounding volume hierarchy, hierarchy is a binary tree
public:
	BVHNode* m_children[2]; // if leaf then both children will be NULL
	BoundingVolume m_volume; // volume that encompases all children
	Body* m_body; // only leaf nodes have a rigid body
	BVHNode* m_parent; // parent node useful for tree manipulation operations
	BVHNode(BVHNode* parent, const BoundingVolume& volume, Body* bode = nullptr)
		: m_parent(parent), m_volume(volume), m_body(body), m_children({ nullptr, nullptr }) {
	}
	~BVHNode();
	inline bool is_leaf() const {
		return m_body != nullptr;
	}
	size_t get_potential_contacts(PotentialContact* contacts, size_t limit) const; // build the array of potential contacts
	void insert(Body* body, const BoundingVolume& volume);
protected:
	bool overlaps(const BVHNode<BoundingVolume>* other) const; // checks if two nodes overlap, should call the overlaps method of the BoundingVolume
	size_t get_potential_contacts_with(const BVHNode<BoundingVolume>* other, PotentialContact* contacts, size_t limit) const;
	void recalculate_bounding_volume(bool recurse = true); // recalculate the bounding volume of a node based on its children
};

template<class BoundingVolume>
bool BVHNode<BoundingVolume>::overlaps(const BVHNode<BoundingVolume>* other) const {
	return m_volume->overlaps(other->m_volume); // call the overlap function fot the Bounding volume we use for the hierarchy
}

template<class BoundingVolume>
void BVHNode<BoundingVolume>::insert(Body* body, const BoundingVolume& volume) {
	if (is_leaf()) { // if we are a leaf then we must create two new children and make ourselves an internal node
		// create two new children with ourselves as the parent
		// the first child has our volume and body
		// the second child will have the volume and body we want to insert
		m_children[0] = new BNHNode<BoundingVolume>(this, m_volume, m_body);
		m_children[1] = new BVHNode<BoundingVolume>(this, volume, body);
		this->m_body = nullptr; // set to nullptr so we can be classified as an internal node
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
template<class BoundingVolume>
BVHNode<BoundingVolume>::~BVHNode() {
	// delete the BVH from the root node down, the root has no parent so we just process the children and not the sibling
	if (m_parent) {
		BVHNode<BoundingVolume>* sibling;
		// if we are the first child then the second child is our sibling and vice-versa
		if (parent->children[0] == this) sibling = parent->children[1];
		else sibling = parent->children[0];

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
void BVHNode<BoundingVolume>::recalculate_bounding_volume(bool recurse) { // O(log(n)) 
	if (is_leaf()) return;
	// create a new bounding volume that encompases our children
	m_volume = BoundingVolume(children[0]->m_volume, children[1]->m_volume);
	if (parent) m_parent->recalculate_bounding_volume(true); // recalculate the bounding volume up the tree
}

template<class BoundingVolume>
size_t BVHNode<BoundingVolume>::get_potential_contacts(PotentialContact* contacts, size_t limit) const {
	if (is_leaf() || limit == 0) return 0; // return if we dont have any more room for contacts or we are a leaf ( if root is a leaf then we could never have any contacts)
	return children[0]->get_potential_contacts_with(m_children[1], contacts, limit);
}

template<class BoundingVolume>
size_t BVHNode<BoundingVolume>::get_potential_contacts_with(const BVHNode<BoundingVolume>* other, PotentialContact* contacts, size_t limit) const {
	if (!overlaps(other) || limit == 0) return 0; // if nether BVH overlaps then we dont need to proceed further
	if (is_leaf() && other->is_leaf()) { // if we are both leaf nodes then there is one potential contact because the overlap call above us passed
		contacts->m_body[0] = this->m_body;
		contacts->m_body[1] = other->m_body;
		return 1;
	}
	// determine which brach to descend into, if wither is a leaf then descend into the other, otherwise descend into the larger one
	// descend into the larger one incase we pass the limit from it
	if (other->is_leaf() || (!is_leaf() && m_volume->get_size() >= other->m_volume->get_size())) {
		size_t count = m_children[0]->get_potential_contacts_with(other, contacts, limit);
		if (limit > count) {
			return count + m_children[1]->get_potential_contacts_with(other, contacts + count, limit - count);
		}
		else {
			return count;
		}
	}
	else {
		size_t count = get_potential_contacts_with(other->m_children[0], contacts, limit);
		if (limit > count) {
			return count + get_potential_contacts_with(other->m_children[1], contacts + count, limit - count);
		}
		else {
			return count;
		}
	}
}