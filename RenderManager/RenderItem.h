#pragma once
#include "MathHelper.h"
#include "d3dUtil.h"
#include <d3d12.h>
#include "FrameResources.h"

// stores parameters needed to draw an object
struct RenderItem {
	RenderItem() = default;
	RenderItem(const RenderItem& rhs) = default;
	RenderItem& operator= (const RenderItem&) = default;
	DirectX::XMFLOAT4X4 m_world = MathHelper::identity_4x4();
	DirectX::XMFLOAT4X4 m_tex_transform = MathHelper::identity_4x4();
	int m_num_frames_dirty = g_num_frame_resources;
	int m_textures_used = 0;
	uint32_t m_obj_cb_index = -1; // index into the contant buffer array
	Material* m_material = nullptr;
	Mesh* m_mesh; // pointer to the geometry buffers that store all the beometry
	// curently the Mesh structure has buffers that can hold multiple submeshes
	// the submesh structure stores the index range for that spsecific submesh
	D3D12_PRIMITIVE_TOPOLOGY m_primitive_type = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
	// draw indexed instanced parameters
	uint32_t m_index_count = 0;
	uint32_t m_start_index_location = 0;
	int m_base_vertex_location = 0;
};