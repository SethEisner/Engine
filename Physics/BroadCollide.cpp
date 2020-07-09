#include "BroadCollide.h"

BoundingBox::BoundingBox(const DirectX::XMFLOAT3& center, const DirectX::XMFLOAT3& extents) : m_box(center, extents) {}
BoundingBox::BoundingBox(const BoundingBox& first, const BoundingBox& second) : m_box() {
	m_box.CreateMerged(m_box, first.m_box, second.m_box); // because we call member function of this->m_box and use it to edit this->m_box, this may not work...
}
inline bool BoundingBox::overlaps(const BoundingBox& other) const {
	return m_box.Intersects(other.m_box);
}
// find a way to calculate get growth without allocating a BoundingBox
float BoundingBox::get_growth(const BoundingBox& other) const {
	BoundingBox box(*this, other); // create a box that contains both of the boxes
	float new_dim[3] = { box.m_box.Extents.x, box.m_box.Extents.y, box.m_box.Extents.z };
	float orig_dim[3] = { this->m_box.Extents.x, this->m_box.Extents.y, this->m_box.Extents.z };
	enum {
		length = 0,
		width = 1,
		height = 2
	};
	// return the increase in surface area of the new box from our original box
	return 2 * ((new_dim[length] * new_dim[width] + new_dim[length] * new_dim[height] + new_dim[height] * new_dim[width]) 
				- (orig_dim[length] * orig_dim[width] + orig_dim[length] * orig_dim[height] + orig_dim[height] * orig_dim[width]));
}