#pragma once
#include "UploadBuffer.h"
#include <d3d12.h>
#include <DirectXMath.h>
#include "d3dUtil.h"
#include "MathHelper.h"

struct ObjectConstants {
	DirectX::XMFLOAT4X4 m_world = MathHelper::identity_4x4();
	DirectX::XMFLOAT4X4 m_tex_transform = MathHelper::identity_4x4();
	uint32_t m_material_index;
	// padding necessary beacuse of hlsl packing rules
	uint32_t m_obj_pad0;
	uint32_t m_obj_pad1;
	uint32_t m_obj_pad2;
};
struct PassConstants {
	DirectX::XMFLOAT4X4 m_view = MathHelper::identity_4x4();
	DirectX::XMFLOAT4X4 m_inv_view = MathHelper::identity_4x4();
	DirectX::XMFLOAT4X4 m_proj = MathHelper::identity_4x4();
	DirectX::XMFLOAT4X4 m_inv_proj = MathHelper::identity_4x4();
	DirectX::XMFLOAT4X4 m_view_proj = MathHelper::identity_4x4();
	DirectX::XMFLOAT4X4 m_inv_view_proj = MathHelper::identity_4x4();
	DirectX::XMFLOAT3 m_eye_pos_w = { 0.0f, 0.0f, 0.0f };
	float m_cb_per_obj_pad_1 = 0.0f;
	DirectX::XMFLOAT2 m_render_target_size = { 0.0f, 0.0f };
	DirectX::XMFLOAT2 m_inv_render_target_size = { 0.0f, 0.0f };
	float m_near_z = 0.0f;
	float m_far_z = 0.0f;
	float m_total_time = 0.0f;
	float m_delta_time = 0.0f;
	DirectX::XMFLOAT4 m_ambient_light = { 0.0f, 0.0f, 0.0f, 1.0f };
	Light m_lights[MAXLIGHTS];
};

struct MaterialData {
	DirectX::XMFLOAT4 m_diffuse_albedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 m_fresnel_r0 = { 0.01f, 0.01f, 0.01f };
	float m_roughness = 0.25f;
	DirectX::XMFLOAT4X4 m_mat_transform = MathHelper::identity_4x4();
	uint32_t m_diffuse_map_index = 0;
	uint32_t m_mat_pad0;
	uint32_t m_mat_pad1;
	uint32_t m_mat_pad2;
};

struct Vertex {
	DirectX::XMFLOAT3 m_pos;
	DirectX::XMFLOAT3 m_normal;
	DirectX::XMFLOAT2 m_text_coord;
};

// stores the resources needed for the cpu to build the command lists for a frame
// keep a circular buffer of frame resources so the cpu can start processing the next frame while the gpu is working
struct FrameResources {
public:
	FrameResources(ID3D12Device* device, uint32_t pass_count, uint32_t object_count, uint32_t material_count);
	FrameResources(const FrameResources& rhs) = delete;
	FrameResources& operator=(const FrameResources& rhs) = delete;
	~FrameResources();
	// every frame neds its own allocator and buffers because we cannot edit these while the gpu is using them
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_cmd_list_allocator;
	UploadBuffer<PassConstants>* m_pass_cb = nullptr;
	UploadBuffer<ObjectConstants>* m_object_cb = nullptr;
	UploadBuffer<MaterialData>* m_material_buffer = nullptr;
	// fence value to mark commands up to this fence point
	// enables the cpu to check if these frame resources are still in use by the gpu
	uint64_t m_fence = 0;
};
