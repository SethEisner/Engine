#pragma once
#include "RigidBody.h"
#include "CollisionObject.h"
#include "../Gameplay/GameObject.h"
#include "Contact.h"
#include <DirectXCollision.h>
#include <memory>
#include <algorithm>

class CollisionObject;
class GameObject;
// broad-phase collision detection using a BVH of spheres because it's super fast

// float because DirectX collision volumes store their geometric attributes in floats
// static constexpr float four_pi = 2.566370614359172953850573533118;
// static constexpr float four_over_three_pi = 4.1887902047863909846168578443727;
// struct BoundingSphere {
// 	DirectX::BoundingSphere m_sphere;
// 	BoundingSphere(const DirectX::XMFLOAT3& center, float radius);
// 	BoundingSphere(const BoundingSphere& first, const BoundingSphere& second);
// 	bool overlaps(const BoundingSphere& other) const;
// 	float get_growth(const BoundingSphere& other) const; // returns how much the bs would have to grow to incorporate the given bounding sphere , should be percentage growth in surface area
// 	inline float get_size() const { // size is the volume
// 		return four_over_three_pi * m_sphere.Radius * m_sphere.Radius * m_sphere.Radius;
// 	}
// };
struct BoundingBox { // AABB
private: // to be private should have an extents and center function
	DirectX::BoundingBox m_box; // the AABB in model space that we transform from so we dont get acculuated errors
	GameObject* m_game_object = nullptr;
public:
	DirectX::BoundingBox m_world_box; // the AABB in world space for comapring
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
		// return this->m_world_box.Intersects(other.m_world_box);
		if (this->m_world_box.Intersects(other.m_world_box)) {
			return true;
		}
		return false;

	}
	bool intersects(const DirectX::XMVECTOR& origin, const DirectX::XMVECTOR& direction, float& distance) const {
		// return true if we found an intersection with the ray and the intersection at all and return the distance of the intersections
		return m_world_box.Intersects(origin, direction, distance);
	}
	bool intersects(const DirectX::XMVECTOR& origin, const DirectX::XMVECTOR& direction, float min_dist, float max_dist) const {
		// return true if we found an intersection with the ray and the intersection within the distance range provided
		float distance = 0.0f;
		return (m_world_box.Intersects(origin, direction, distance) && distance <= max_dist && distance >= min_dist);
	}
	float get_growth(const BoundingBox& other) const; // the change in surface area after combining this and other
	inline float calculate_surface_area() const {
		float surface_area = 8.0f * m_box.Extents.x * m_box.Extents.y + m_box.Extents.x * m_box.Extents.z + m_box.Extents.y * m_box.Extents.z;
		assert(surface_area > 0.0f);
		return surface_area;
	}
	// inline float get_surface_area() const { // surface area of the box
	// 	// m_surface_area = 8.0f * m_box.Extents.x * m_box.Extents.y + m_box.Extents.x * m_box.Extents.z + m_box.Extents.y * m_box.Extents.z;
	// 	assert(m_surface_area > 0.0f);
	// 	return m_surface_area;
	// 	// volume return fabsf(8.0f * m_box.Extents.x * m_box.Extents.y * m_box.Extents.z); // extents is distance from center to edge, so need to multiply by 8
	// }
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

struct PotentialContact { // a potential contact is a pair of two collisionobjects, use collision objects so it's easy to go get associated gameobject, rigidbody, AABB, and OBBs
	CollisionObject* m_collision_object[2] = { nullptr, nullptr };
	PotentialContact() : m_collision_object{ nullptr, nullptr } {}
	PotentialContact(CollisionObject* first, CollisionObject* second) : m_collision_object{ first, second } {}
};

// change the hierarchy to use an AABB instead of a sphere hierarchy
template<class BoundingVolume>
class BVHNode { // node of the bounding volume hierarchy, hierarchy is a binary tree
public:
	BoundingVolume m_volume; // pointer to the boundingvolume in the CollisionObject
	BVHNode* m_children[2]; // if leaf then both children will be NULL
	CollisionObject* m_collision_object; // only leaf nodes have a rigid body
	// could a leaf node not have a rigid body? scenery doesnt have a rigidbody but does have a BoundingBox
	BVHNode* m_parent; // parent node useful for tree manipulation operations
	uint32_t m_height; // the height of the node
	float m_sah_cost; // the surface area of m_volume
	// BVHNode(BVHNode* parent, BoundingVolume* volume, CollisionObject* body = nullptr)
	// 	: m_parent(parent), m_volume(volume), m_body(body), m_children({ nullptr, nullptr }) {
	// }
	BVHNode(BVHNode* parent, const BoundingVolume& volume, float surface_area, CollisionObject* obj = nullptr)
		: m_volume(volume), m_children{ nullptr, nullptr }, m_collision_object(obj), m_parent(parent), m_height(0), m_sah_cost(surface_area) {
	}
	~BVHNode();
	inline bool is_leaf() const {
		return m_collision_object != nullptr;
	}
	inline bool is_root() const {
		return m_parent == nullptr;
	}
	size_t get_potential_contacts(PotentialContact* contacts, size_t limit) const; // build the array of potential contacts
	void get_potential_intersections(std::vector<CollisionObject*>*, const CollisionObject* ourself,  const DirectX::XMVECTOR& origin, const DirectX::XMVECTOR& direction) const;
	// void insert(RigidBody* body, const BoundingVolume& volume);
	void insert(CollisionObject* coll_obj); // collision objects have their own BoundingVolume
	void update();
protected:
	bool overlaps(const BVHNode<BoundingVolume>* other) const; // checks if two nodes overlap, should call the overlaps method of the BoundingVolume
	size_t get_potential_contacts_with(const BVHNode<BoundingVolume>* other, PotentialContact* contacts, size_t limit) const;
	void recalculate_bounding_volume(); // recalculate the bounding volume of a node based on its children
};

template<class BoundingVolume>
bool BVHNode<BoundingVolume>::overlaps(const BVHNode<BoundingVolume>* other) const {
	// doesnt collide for even 
	return m_volume.overlaps(other->m_volume); // call the overlap function for the Bounding volume we use for the hierarchy
	// return m_collision_object->m_box->overlaps(*other->m_collision_object->m_box);
}
template<class BoundingVolume>
void BVHNode<BoundingVolume>::insert(CollisionObject* coll_obj) {
	if (is_leaf()) { // if we are a leaf then we must create two new children and make ourself the internal node
		// our leaf data becomes the first child and the other child is the object we are inserting
		// then we become the parent for our two children
		m_children[0] = new BVHNode<BoundingVolume>(this, m_volume, m_sah_cost, m_collision_object); // copy our data to the first child
		m_children[1] = new BVHNode<BoundingVolume>(this, *coll_obj->m_box, coll_obj->m_box->calculate_surface_area(), coll_obj); // insert the new collision object as our second child
		this->m_collision_object = nullptr; // set the collision object to nullptr as only leaf nodes have collision objects
		m_height = 1; // we were a leaf with height 0, but now we are an internal node with height 1 (because we have 1 layer of children beneath us)
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
void BVHNode<BoundingVolume>::update() { // called every frame
	// need to recalculate internal node volumes
	// this is the current node, start with this = root 
	if (!is_leaf()) { // if we are an internal node, process our children first and then recalculate our volume once the children have the correct volume 
		m_children[0]->update();
		m_children[1]->update();
		m_volume = BoundingVolume(m_children[0]->m_volume, m_children[1]->m_volume);
		m_sah_cost = m_volume.calculate_surface_area();
		if (m_height >= 2) { // we can optionally perform tree rotations on this node as we have children and grandchildren
			float float_max = std::numeric_limits<float>::max();
			// float rotation_costs[6] = { float_max, float_max, float_max, float_max, float_max, float_max, float_max, float_max };
			// we are guaranteed to have two children, and at least one of our children has two children
			// first four rotations is trying to swap a child with either of its nephew
			uint32_t first_height = m_children[0]->m_height;
			uint32_t second_height = m_children[1]->m_height;
			float growth = 0.0f;
			float current_cost;//  = m_sah_cost;
			BVHNode<BoundingVolume>* first_node = nullptr;
			BVHNode<BoundingVolume>* second_node = nullptr;
			BVHNode<BoundingVolume>* uncle;
			BVHNode<BoundingVolume>* grandchild;
			BVHNode<BoundingVolume>* sibling;
			size_t first_parent_index; // the index the first node is at in its parent's children array
			size_t second_parent_index;// the index the second node is at in its parent's children array
			for (size_t i = 0; i != 2; ++i) { // pick the child we want to look at the grand children on
				if (m_children[i]->m_height == 0) continue; // the child we are looking at doesnt have children
				size_t other_child_index = i + 1 & ~2; // the other child of us is either 0+1 or 1+1 & ~2
				assert(i != other_child_index);
				assert(other_child_index == 0 || other_child_index == 1);
				current_cost = m_children[i]->m_sah_cost;
				uncle = m_children[other_child_index];
				for (size_t j = 0; j != 2; j++) { // for each child of the chosen child
					grandchild = m_children[i]->m_children[j];
					size_t other_grandchild_index = j + 1 & ~2;
					sibling = m_children[i]->m_children[other_grandchild_index];
					growth = grandchild->m_volume.get_growth(uncle->m_volume); // see how much surface area is saved if we swap them with their uncle
					if (growth < current_cost) { // we'd be better of paired with our uncle so we must swap our sibling with our uncle
						current_cost = growth;
						first_node = uncle;
						second_node = sibling;
						first_parent_index = other_child_index;
						second_parent_index = other_grandchild_index;
					}
				}
			}
			// second four rotations is trying to swap one grand child with either of its cousins
			// can only perform these rotation if both of our children are parents
			// swapping granchildren can snable our children to be smaller
			if (first_height > 0 && second_height > 0) {
				BVHNode<BoundingVolume>* cousin;
				current_cost = std::min(m_children[0]->m_sah_cost + m_children[1]->m_sah_cost, current_cost);
				grandchild = m_children[0]->m_children[0];
				sibling = m_children[0]->m_children[1];
				for (size_t i = 0; i != 2; ++i) { // swapping granchildren around would change the cost of both of our children, so the cost we compare it to should be the sum
					size_t other_grandchild_index = i + 1 & ~2; // the other child of us is either 0+1 or 1+1 & ~2
					cousin = m_children[1]->m_children[i];
					growth = sibling->m_volume.get_growth(m_children[1]->m_children[i]->m_volume) + 
							 grandchild->m_volume.get_growth(m_children[1]->m_children[other_grandchild_index]->m_volume);
					// we are granchild here
					if (growth < current_cost) { // swapping with our cousin would produce a lower overall SAH cost for our parent and uncle
						current_cost = growth;
						first_node = grandchild;
						second_node = cousin;
						first_parent_index = 0;
						second_parent_index = i;
					}
				}
			}
			/* rotation steps:
				// swap the appropriate members between each parent's children array, we are exchanging ourself with another node and our parent better know that
				// swap parent pointers
				// recalculate the parents volume only
				// recaluclate the parents surface area
				// reclaculate the height of the parents
			*/
			if (!(first_node && second_node)) return;
			assert(first_node->m_parent != second_node->m_parent); // the nodes must have different parents
			assert(first_node->m_parent->m_children[first_parent_index] == first_node);
			assert(second_node->m_parent->m_children[second_parent_index] == second_node);
			std::swap(first_node->m_parent->m_children[first_parent_index], second_node->m_parent->m_children[second_parent_index]);
			std::swap(first_node->m_parent, second_node->m_parent); // swap our parent pointers to exhange the sub branches
			auto first_parent = first_node->m_parent;
			auto second_parent = second_node->m_parent;
			first_parent->m_volume = BoundingVolume(first_parent->m_children[0]->m_volume, first_parent->m_children[1]->m_volume);
			second_parent->m_volume = BoundingVolume(second_parent->m_children[0]->m_volume, second_parent->m_children[1]->m_volume);
			first_parent->m_sah_cost = first_parent->m_volume.calculate_surface_area();
			second_node->m_parent->m_sah_cost = second_parent->m_volume.calculate_surface_area();
			first_parent->m_height = std::max(first_parent->m_children[0]->m_height, first_parent->m_children[1]->m_height) + 1;
			second_parent->m_height = std::max(second_parent->m_children[0]->m_height, second_parent->m_children[1]->m_height) + 1;
		}
	}
	else{ // if we are a leaf, recalculate the leaf's volume because the collision object has moved
		m_volume = *m_collision_object->m_box;
		// the collision box only translates us so we should not need to recalculate the sah
		m_sah_cost = m_volume.calculate_surface_area();
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
	m_sah_cost = m_volume.calculate_surface_area();
	m_height = std::max(m_children[0]->m_height, m_children[1]->m_height) + 1;
	if (m_parent) m_parent->recalculate_bounding_volume(); // recalculate the bounding volume up the tree
}
template<class BoundingVolume>
size_t BVHNode<BoundingVolume>::get_potential_contacts(PotentialContact* contacts, size_t limit) const {
	if (is_leaf() || limit == 0) return 0; // return if we dont have any more room for contacts or we are a leaf ( if root is a leaf then we could never have any contacts)
	return m_children[0]->get_potential_contacts_with(m_children[1], contacts, limit);
}
template<class BoundingVolume>
size_t BVHNode<BoundingVolume>::get_potential_contacts_with(const BVHNode<BoundingVolume>* other, PotentialContact* contacts, size_t limit) const {
	size_t count = 0;
	if (!is_leaf() && limit > count) { // chekc for contacts between our children
		count += m_children[0]->get_potential_contacts_with(m_children[1], contacts + count, limit - count);
	}
	if (!other->is_leaf() && limit > count) { // check for contacts between the other's children
		count += other->m_children[0]->get_potential_contacts_with(other->m_children[1], contacts + count, limit - count);
	}
	if (limit > 0 && this->overlaps(other)) { // still have space and our subtrees overlap
		if (is_leaf() && other->is_leaf()) { // if we are both leaf nodes then there is one potential contact because the overlap call above us passed
			contacts->m_collision_object[0] = this->m_collision_object;
			contacts->m_collision_object[1] = other->m_collision_object;
			return count + 1; // guranteed to have space for one more contact
		}
		if (other->is_leaf() || (!is_leaf() && m_volume.calculate_surface_area() >= other->m_volume.calculate_surface_area())) {
			if (limit > count) count += m_children[0]->get_potential_contacts_with(other, contacts + count, limit - count);
			if (limit > count) count += m_children[1]->get_potential_contacts_with(other, contacts + count, limit - count);
		}
		else { // other must not be a leaf here, and other is bigger than this volume
			if (limit > count) count += get_potential_contacts_with(other->m_children[0], contacts + count, limit - count);
			if (limit > count) count += get_potential_contacts_with(other->m_children[1], contacts + count, limit - count);
		}
	}
	return count;
}
template<class BoundingVolume>
void BVHNode<BoundingVolume>::get_potential_intersections(std::vector<CollisionObject*>* intersections, const CollisionObject* ourself, const DirectX::XMVECTOR& origin, const DirectX::XMVECTOR& direction) const {
	float distance = 0.0f;
	// the raycast will find an intersection even if we are within the node
	if (!m_volume.intersects(origin, direction, distance)) return; // no intersection was found with the current node so we dont need to do any other work on this branch
	if (is_leaf()) { // have a collision with a leaf node
		// if we are checking for intersection against a leaf node that is not the requestor of the raycast, then add it to the intersections vector
		if (ourself != m_collision_object) intersections->emplace_back(m_collision_object); // add the collision object of this node to the intersections vector
		return;
	}
	// check each child for intersections. if we are not a leaf then our collisionobject is nullptr and we dont need to worry about self intersections
	m_children[0]->get_potential_intersections(intersections, ourself, origin, direction); // check for intersections with the first child
	m_children[1]->get_potential_intersections(intersections, ourself, origin, direction); // check for intersections with the second child
}