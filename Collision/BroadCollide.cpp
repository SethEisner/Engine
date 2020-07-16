#include "BroadCollide.h"

//is there a way to calculate get growth without allocating a BoundingBox
float BoundingBox::get_growth(const BoundingBox& other) const {
	BoundingBox box(*this, other); // create a box that contains both of the boxes
	// entents is half size so need to multiply each extent by 2
	// could probably remove all the 2's and multiply final result by 8
	using namespace DirectX;
	// XMFLOAT3 new_dim = { 2.0f * box.m_box.Extents.x, 2.0f * box.m_box.Extents.y, 2.0f * box.m_box.Extents.z };
	// XMFLOAT3 orig_dim = { 2.0f * this->m_box.Extents.x, 2.0f * this->m_box.Extents.y, 2.0f * this->m_box.Extents.z };
	XMFLOAT3 new_dim = { box.m_box.Extents.x, box.m_box.Extents.y, box.m_box.Extents.z };
	XMFLOAT3 orig_dim = { this->m_box.Extents.x, this->m_box.Extents.y, this->m_box.Extents.z };
	// return the increase in surface area of the new box from our original box
	// length is x, width is y, height is z
	return 8.0f * ((new_dim.x * new_dim.y + new_dim.x * new_dim.z + new_dim.z * new_dim.y) 
				- (orig_dim.x * orig_dim.y + orig_dim.x * orig_dim.z + orig_dim.z * orig_dim.y));
	// 8 is from pulling a 2 out of the inner calculation and a 2*2 from the dimensions
}