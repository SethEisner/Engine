#include "NarrowCollide.h"
#include <assert.h>
#include <limits>

// should use iterators and stl ADTs for searching

void OrientedBoundingBox::calculate_internals() {
	using namespace DirectX;
	XMStoreFloat4x4(&m_transform, XMMatrixMultiply(XMLoadFloat4x4(&m_offset), XMLoadFloat4x4(&m_body->get_transform())));
}

inline bool IntersectionTests::intersects(const OrientedBoundingBox& first, const OrientedBoundingBox& second) {
	return first.m_oriented_box.Intersects(second.m_oriented_box); // call the DirectX Collision function so it does the hardwork for us
}
static inline float transform_to_axis(const OrientedBoundingBox& box, const DirectX::XMFLOAT3& axis) {
	using namespace DirectX;
	XMVECTOR axis_ = XMLoadFloat3(&axis);
	float x_dot = fabs(XMVectorGetX(XMVector3Dot(axis_, XMLoadFloat3(&box.get_axis(0)))));
	float y_dot = fabsf(XMVectorGetX(XMVector3Dot(axis_, XMLoadFloat3(&box.get_axis(1)))));
	float z_dot = fabsf(XMVectorGetX(XMVector3Dot(axis_, XMLoadFloat3(&box.get_axis(2)))));
	return box.m_oriented_box.Extents.x * x_dot 
			+ box.m_oriented_box.Extents.y * y_dot 
			+ box.m_oriented_box.Extents.z * z_dot;
}
// check if the boxes overlap along the given axis, and returns the amount of overlap
// to_center is the vector between the boxes' center points
static inline float penetration_on_axis(const OrientedBoundingBox& first, const OrientedBoundingBox& second, const DirectX::XMFLOAT3& axis, const DirectX::XMFLOAT3& to_center) {
	// project the half-size of each box onto the axis
	float first_projection = transform_to_axis(first, axis);
	float second_projection = transform_to_axis(second, axis);
	// project their center distance onto the axis
	using namespace DirectX;
	float distance = fabs(XMVectorGetX(XMVector3Dot(XMLoadFloat3(&to_center), XMLoadFloat3(&axis))));
	// return the overlap, positive indicates overlap, negative indicates separation
	return first_projection + second_projection - distance;

}
// returns true if the boxes overlap on the axis
// updates value of smallest penetration and smallest case if the returned penetration is less than the given penetration value
static inline bool try_axis(const OrientedBoundingBox& first, const OrientedBoundingBox& second, DirectX::XMFLOAT3& axis, const DirectX::XMFLOAT3& to_center, size_t index, float& smallest_penetration, int& smallest_case) {
	using namespace DirectX;
	XMVECTOR axis_ = XMLoadFloat3(&axis);
	float magnitude = XMVectorGetX(XMVector3Dot(axis_, axis_)); // dont needs abs because dot with itself will always be positive
	if (magnitude < 0.0001) return true; // axis is bad
	XMStoreFloat3(&axis, XMVector3Normalize(axis_)); // normalize the axis
	float penetration = penetration_on_axis(first, second, axis, to_center);
	if (penetration < 0) return false;
	if (penetration < smallest_penetration) {
		smallest_penetration = penetration;
		smallest_case = index;
	}
	return true;
}
// calculate the best contact point to use for the found contact
static inline DirectX::XMFLOAT3 contact_point(const DirectX::XMFLOAT3& p_one, const DirectX::XMFLOAT3& d_one, float size_one, const DirectX::XMFLOAT3& p_two, const DirectX::XMFLOAT3& d_two, float size_two, bool use_one) {
	// if use_1 is true, and the contact point is outside the edge (in the case of an edge-face contact) the we use 1's midpoint, otherwise we use 2's midpoint
	using namespace DirectX;
	XMVECTOR to_st; // center one, center two
	XMVECTOR d_one_ = XMLoadFloat3(&d_one);
	XMVECTOR p_one_ = XMLoadFloat3(&p_one);
	XMVECTOR d_two_ = XMLoadFloat3(&d_two);
	XMVECTOR p_two_ = XMLoadFloat3(&p_two);
	// float dp_sta_one, dp_sta_two, dp_one_two, sm_one, sm_two; // squared magnitude of one and two
	float sm_one = XMVectorGetX(XMVector3Dot(d_one_, d_one_));
	float sm_two = XMVectorGetX(XMVector3Dot(d_two_, d_two_));
	float dp_one_two = fabsf(XMVectorGetX(XMVector3Dot(d_two_, d_one_)));

	to_st = p_one_ - p_two_;
	float dp_sta_one = fabsf(XMVectorGetX(XMVector3Dot(d_one_, to_st)));
	float dp_sta_two = fabsf(XMVectorGetX(XMVector3Dot(d_two_, to_st)));
	
	float denom = sm_one * sm_two - dp_one_two * dp_one_two;
	// zero denominator indicates parallel lines
	if (fabsf(denom) < 0.0001f) return use_one ? p_one : p_two;
	
	float inv_denom = 1.0f / denom;
	float mua = (dp_one_two * dp_sta_two - sm_two * dp_sta_one) * inv_denom;
	float mub = (sm_one * dp_sta_two - dp_one_two * dp_sta_one) * inv_denom;
	// if either of the edges has the nearest point out of bounds, then the edges aren't crossed and we have an edge-face contact. 
	// our contact point is on the edge, which we know from the use_one parameter
	if (mua > size_one || mua < -size_one || mub > size_two || mub < -size_two) return use_one ? p_one : p_two;
	XMVECTOR c_one, c_two;
	c_one = p_one_ + d_one_ * mua;
	c_two = p_two_ + d_two_ * mub;
	XMFLOAT3 ret;
	XMStoreFloat3(&ret, c_one * 0.5 + c_two * 0.5f);
	return ret;
}
void fill_point_face_box_box(const OrientedBoundingBox& first, const OrientedBoundingBox& second, const DirectX::XMFLOAT3& to_center, CollisionData* data, int best, float penetration) {
	// called when we know that a vertex from box two is in contact with a face on box one
	Contact* contact = data->m_contacts;
	using namespace DirectX;
	assert(best >= 0 && best <= 2); // assert that best is either the x,y, or z axis
	XMVECTOR normal = XMLoadFloat3(&first.get_axis(best)); // normal of the face is in the same direction as the axis the face lies on
	float dot = XMVectorGetX(XMVector3Dot(normal, XMLoadFloat3(&to_center)));
	if (dot > 0) normal *= -1.0f; // determine if the positive or negative face on the axis collided
	// determine which vertex of the second box collided with the face
	XMFLOAT3 vertex = second.m_oriented_box.Extents;
	if (XMVectorGetX(XMVector3Dot(XMLoadFloat3(&second.get_axis(0)), normal)) < 0.0f) vertex.x = -vertex.x;
	if (XMVectorGetX(XMVector3Dot(XMLoadFloat3(&second.get_axis(1)), normal)) < 0.0f) vertex.y = -vertex.y;
	if (XMVectorGetX(XMVector3Dot(XMLoadFloat3(&second.get_axis(2)), normal)) < 0.0f) vertex.z = -vertex.z;
	XMStoreFloat3(&contact->m_contact_normal, normal);
	contact->m_penetration = penetration;
	XMStoreFloat3(&contact->m_contact_point, XMVector3Transform(XMLoadFloat3(&vertex), XMLoadFloat4x4(&second.get_transform()))); // convert the vertex to world space
	contact->set_body_data(first.m_body, second.m_body, data->m_friction, data->m_restitution);
}
uint32_t CollisionDetector::collides(const OrientedBoundingBox& first, const OrientedBoundingBox& second, CollisionData* data) {
	using namespace DirectX;
	XMVECTOR axes[6];
	// XMFLOAT3 second_axes[3];
	for (size_t i = 0; i != 3; i++) { // get x,y,z axes for each box, dont need to store translation
		axes[i] = XMLoadFloat3(&first.get_axis(i));
		axes[i + 3] = XMLoadFloat3(&second.get_axis(i));
		// axes[i] = first.get_axis(i);
		// axes[i + 3] = second.get_axis(i);
	}
	XMFLOAT3 to_center;
	XMStoreFloat3(&to_center, XMLoadFloat3(&second.get_axis(3)) - XMLoadFloat3(&first.get_axis(3))); // get_axis function may be wrong because idk how this caluclates the distace from their centers
	float penetration = std::numeric_limits<float>::max();
	int best = -1; // place to store penetration results of the try_axis function
	XMFLOAT3 axis; // stores the current axis as an XMFLOAT3 we can easily pass to try_axis
	for (size_t i = 0; i != 6; ++i) { // try to find a separating axis using the axis we stored (starting with x,y,z of first, and then x,y,z of second
		XMStoreFloat3(&axis, axes[i]);
		if (!try_axis(first, second, axis, to_center, i, penetration, best)) return 0; // return 0 if we find a sparating axis because we found no collision
	}
	int best_axis = best; // store the best axis so far before we do the cross products
	size_t index = 6;
	XMFLOAT3 cross;
	for (size_t i = 0; i != 3; ++i) {
		for (size_t j = 3; j != 6; ++j) { // start from 3 so we can start from the base data of the second box
			XMStoreFloat3(&cross, XMVector3Cross(axes[i], axes[j])); 
			if (!try_axis(first, second, cross, to_center, index, penetration, best)) return 0;
		}
	}
	assert(best >= 0); // we should only get to this point if we could not find a separating axis, and this best should be a non-negative number
	// by this point we know there is a collision, and we know which axis gave the smallest penetration
	// we can deal with it in different ways depending on which axis it was`
	switch (best) {
	case 0:
	case 1:
	case 2: // a vertex of the second box intersected a face on the first box
		fill_point_face_box_box(first, second, to_center, data, best, penetration);
		data->add_contacts(1);
		return 1;
	case 3:
	case 4:
	case 5: // a vertex of the first box intersected a face on the second box
		// function assumes that the vertex that collided is on the second paramter,
		// we must flip the to_center vector if we swap the box pointers
		fill_point_face_box_box(second, first, XMFLOAT3(-to_center.x, -to_center.y, -to_center.z), data, best, penetration);
		data->add_contacts(1);
		return 1;
	default: // collision resulted in edge-edge contact so we need to find out which two axis collided
		best -= 6; // best is now [0-9]
		size_t first_axis_index = best / 3;
		size_t second_axis_index = best % 3;
		// XMVECTOR first_axis = axes[first_axis_index]; // determine along which axis the colliding edges are parallel
		// XMVECTOR second_axis = axes[3 + second_axis_index];
		XMVECTOR normal = XMVector3Cross(axes[first_axis_index], axes[3 + second_axis_index]); // collsion normal must be perpendicular to both edges
		normal = XMVector3Normalize(normal); 
		if (XMVectorGetX(XMVector3Dot(normal, XMLoadFloat3(&to_center))) > 0.0f) normal *= -1.0f;
		// determine the two edges from the axis
		// find the point in the center of each parallel edge. the component in the point in the direction of the collision axis is zero (because midpoint) 
		// and we determine which of the extremes in each of the other axes is closest
		// use float array so we can index into it with the values we obtained from best because we cant index into an XMFLOAT3
		float point_on_first_edge[3] = { first.m_oriented_box.Extents.x, first.m_oriented_box.Extents.y, first.m_oriented_box.Extents.z };
		float point_on_second_edge[3] = { second.m_oriented_box.Extents.x, second.m_oriented_box.Extents.y, second.m_oriented_box.Extents.z };
		float first_half_size = point_on_first_edge[first_axis_index]; // store for later
		float second_half_size = point_on_second_edge[second_axis_index];
		point_on_first_edge[first_axis_index] = 0.0f;
		point_on_second_edge[second_axis_index] = 0.0f;
		for (size_t i = 0; i != 3; ++i) {
			if (i != first_axis_index && XMVectorGetX(XMVector3Dot(axes[i], normal)) > 0.0f) point_on_first_edge[i] *= -1.0f;
			if (i != second_axis_index && XMVectorGetX(XMVector3Dot(axes[3 + i], normal)) < 0.0f) point_on_second_edge[i] *= -1.0f;
		}
		// convert the points to world coordinates
		XMFLOAT3 first_point;
		XMFLOAT3 second_point;
		XMStoreFloat3(&first_point,  XMVector3Transform(XMLoadFloat3(&XMFLOAT3(point_on_first_edge)), XMLoadFloat4x4(&first.get_transform())));
		XMStoreFloat3(&second_point, XMVector3Transform(XMLoadFloat3(&XMFLOAT3(point_on_second_edge)), XMLoadFloat4x4(&second.get_transform())));
		// find out which point is closest approach of the two line segments
		float first_extents[3] = { first.m_oriented_box.Extents.x, first.m_oriented_box.Extents.y, first.m_oriented_box.Extents.z };
		float second_extents[3] = { second.m_oriented_box.Extents.x, second.m_oriented_box.Extents.y, second.m_oriented_box.Extents.z };
		XMFLOAT3 first_edge;
		XMStoreFloat3(&first_edge, axes[first_axis_index]);
		XMFLOAT3 second_edge;
		XMStoreFloat3(&second_edge, axes[3 + second_axis_index]);
		XMFLOAT3 vertex = contact_point(first_point, first_edge, first_half_size, second_point, second_edge, second_half_size, best_axis > 2);

		Contact* contact = data->m_contacts;
		contact->m_penetration = penetration;
		XMStoreFloat3(&contact->m_contact_normal, normal); // axis is perpendicular to both edges so we can use it as the contact normal
		contact->m_contact_point = vertex;
		contact->set_body_data(first.m_body, second.m_body, data->m_friction, data->m_restitution);
		data->add_contacts(1);
		return 1;
	}
	assert(false); // should not get here because of the default case
}