#pragma once
#include <string>
#include <wrl/client.h>
#include <stdint.h>
#include <d3d12.h>
#include <unordered_map>
#include <DirectXCollision.h>

// defines a subrange in the geometry buffers in the main Mesh structure
// multiple geometries can be contained in one vertex and index buffer
// this provides the ofsets and data needed to draw a subset of geometry
// probably should change the names of Mesh and SubMesh because mesh can contain the mesh data for multiple different objects
struct SubMesh {
	size_t m_index_count = 0;
	size_t m_start_index = 0;
	size_t m_base_vertex = 0;
	// bounding box for the mesh. change to sphere later
	DirectX::BoundingBox Bounds; // bounding box for the mesh 
};

struct Mesh {
	std::string name; // look up by name
	uint64_t hash_name; // look up by hashed string name
	Microsoft::WRL::ComPtr<ID3DBlob> m_vertex_buffer_cpu = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> m_index_buffer_cpu = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertex_buffer_gpu = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_index_buffer_gpu = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertex_buffer_uploader  = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_index_buffer_uploader = nullptr;
	// buffer meta data
	size_t m_vertex_stride = 0;
	size_t m_vertex_buffer_size = 0;
	DXGI_FORMAT m_index_format = DXGI_FORMAT_R16_UINT; // indecis are unsigned, and 16 bits in size
	size_t m_index_buffer_size = 0;
	// a mesh can contain multiple meshes in one vertex/index buffer
	std::unordered_map<uint64_t, SubMesh> m_draw_args; // replace with hashtable in threadsafe containers
	D3D12_VERTEX_BUFFER_VIEW get_vertex_buffer_view() const {
		D3D12_VERTEX_BUFFER_VIEW vbv;
		vbv.BufferLocation = m_vertex_buffer_gpu->GetGPUVirtualAddress();
		vbv.StrideInBytes = m_vertex_stride;
		vbv.SizeInBytes = m_vertex_buffer_size;
		return vbv;
	}
	D3D12_INDEX_BUFFER_VIEW get_index_buffer_view() const {
		D3D12_INDEX_BUFFER_VIEW ibv;
		ibv.BufferLocation = m_index_buffer_gpu->GetGPUVirtualAddress();
		ibv.Format = m_index_format;
		ibv.SizeInBytes = m_index_buffer_size;
		return ibv;
	}
	void dispose_uploaders() {
		m_vertex_buffer_uploader = nullptr;
		m_index_buffer_uploader = nullptr;
	}
};