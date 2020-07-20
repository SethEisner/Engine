#pragma once
#include <windows.h>
#include <wrl/client.h>
#include <d3d12.h>
#include <assert.h>
#include <string>
#include "d3dx12.h"
#include <d3d12.h>
#include <d3dcompiler.h>
#include <comdef.h>
// #include <unordered_map>
#include <DirectXMath.h>
#include <DirectXCollision.h>
#include "MathHelper.h"
#include "../Gameplay/GameObject.h"
//#include "FrameResources.h"


//contains structures that are needed on the CPU side, and helper functions

extern const int g_num_frame_resources;

inline void assert_if_failed(HRESULT hr) {
	// #if defined(_DEBUG)
	// 	assert(!FAILED(hr));
	// #endif
	if (FAILED(hr)) {
		_com_error err(hr);
		const std::wstring msg = err.ErrorMessage();
		const std::wstring outputMsg = L" error: " + msg;
		MessageBox(0, outputMsg.c_str(), 0, 0);
		assert(false);
	}
}
inline std::wstring AnsiToWString(const std::string& str)
{
	WCHAR buffer[512];
	MultiByteToWideChar(CP_ACP, 0, str.c_str(), -1, buffer, 512);
	return std::wstring(buffer);
}
class DxException {
public:
	DxException() = default;
	DxException(HRESULT hr, const std::wstring& function_name, const std::wstring& filename, int line_number);
	~DxException() = default;
	std::wstring ToString() const;
	HRESULT error_code = S_OK;
	std::wstring function_name;
	std::wstring filename;
	int line_number = -1;
};

#ifndef ThrowIfFailed
#define ThrowIfFailed(x)                                              \
{                                                                     \
    HRESULT hr__ = (x);                                               \
    std::wstring wfn = AnsiToWString(__FILE__);          \
    if(FAILED(hr__)) { throw DxException(hr__, L#x, wfn, __LINE__); } \
}
#endif

Microsoft::WRL::ComPtr<ID3DBlob> compile_shader(const std::wstring&, const D3D_SHADER_MACRO*, const std::string&, const std::string&);
Microsoft::WRL::ComPtr<ID3D12Resource> create_default_buffer(ID3D12Device*, ID3D12GraphicsCommandList*, const void*, uint64_t, Microsoft::WRL::ComPtr<ID3D12Resource>&);

inline size_t calc_constant_buffer_size(size_t size) {
	// round up to nearest multiple of 256, and then mask off the remainder to get the exact multiple
	return (size + 255) & ~255;
}


// defines a subrange in the geometry buffers in the main Mesh structure
// multiple geometries can be contained in one vertex and index buffer
// this provides the ofsets and data needed to draw a subset of geometry
// probably should change the names of Mesh and SubMesh because mesh can contain the mesh data for multiple different objects

extern constexpr uint32_t NUM_TEXTURES = 5; // support four different types of textures (none is not a texture)
enum class TextureFlags {
	NONE = 0,
	COLOR = 1,
	NORMAL = 2,
	ROUGHNESS = 4,
	METALLIC = 8,
	HEIGHT = 16
};
inline TextureFlags operator|(TextureFlags a, TextureFlags b) {
	return static_cast<TextureFlags>(static_cast<int>(a) | static_cast<int>(b));
}
inline int get_texture_index(TextureFlags flag) {
	switch (flag) {
	case TextureFlags::COLOR:
		return 0;
	case TextureFlags::NORMAL:
		return 1;
	case TextureFlags::ROUGHNESS:
		return 2;
	case TextureFlags::METALLIC:
		return 3;
	case TextureFlags::HEIGHT:
		return 4;
	default:
		return -1;
	}
}

struct Texture {
	std::string m_name;
	std::string m_filename;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_resource = nullptr; // pointer to the texture resource
	Microsoft::WRL::ComPtr<ID3D12Resource> m_upload_heap = nullptr; // pointer to the upload heap that we put the resource into
};

// eventually give each submesh a bounding box
struct SubMesh {
	size_t m_index_count = 0;
	size_t m_vertex_count = 0;
	size_t m_start_index = 0;
	size_t m_base_vertex = 0;
	D3D_PRIMITIVE_TOPOLOGY m_primitive = D3D11_PRIMITIVE_TOPOLOGY_UNDEFINED;
	DirectX::XMFLOAT4X4 m_transform; // transform from submesh space to Mesh space
	SubMesh& operator=(SubMesh&) = default;
	//SubMesh& operator=(const SubMesh&) = default;
	// bounding box for the mesh. change to sphere later
	//DirectX::BoundingBox Bounds; // bounding box for the mesh 
};
struct SubMeshBufferData {
	size_t m_index_count = 0;
	size_t m_vertex_count = 0;
	size_t m_start_index = 0;
	size_t m_base_vertex = 0;
};

class GameObject;
struct Mesh { // contains the buffers for a single object. combines all the submeshes into buffers (for vertex and index and cpu and gpu)
	//std::string m_name; // look up by name
	size_t m_mesh_id; // unique id of the mesh, that components can use to quickly identify the mesh, used in maps
	//uint64_t hash_name; // look up by hashed string name
	Microsoft::WRL::ComPtr<ID3DBlob> m_vertex_buffer_cpu = nullptr; // contains the Vertex vector we build up
	Microsoft::WRL::ComPtr<ID3DBlob> m_index_buffer_cpu = nullptr; // contains the index buffer we build up for a model
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertex_buffer_gpu = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_index_buffer_gpu = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_vertex_buffer_uploader = nullptr;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_index_buffer_uploader = nullptr;
	// buffer meta data
	TextureFlags m_textures_used = TextureFlags::NONE; // default to not using any textures, set with flags from TextureType
	size_t m_vertex_stride = 0;
	size_t m_vertex_buffer_size = 0; // number of bytes of our vertex buffer/ can divide by the size of a buffer to get the number of vertices
	size_t m_vertex_count = 0;
	DXGI_FORMAT m_index_format = DXGI_FORMAT_R16_UINT; // indecis are unsigned, and 16 bits in size
	size_t m_index_buffer_size = 0;
	size_t m_index_count = 0;
	// a mesh can contain multiple meshes in one vertex/index buffer
	//std::unordered_map<std::string, SubMesh> m_draw_args; // replace with hashtable in threadsafe containers
	std::vector<SubMesh> m_submeshes;
	std::vector<SubMeshBufferData> m_submesh_helper;
	//std::unordered_map<uint32_t, SubMeshBufferData> m_sub_mesh_helper; // map from sub mesh index to the submesh data we create while building the Mesh and traversing the node hierarchy
	// std::vector<SubMesh> m_sub_meshes;
	// DirectX::BoundingBox m_aabb; // axis-aligned bounding box for the mesh for collision detection
	GameObject* m_game_object = nullptr; // the gameobject that owns this mesh
	DirectX::XMFLOAT4X4 m_transform; // transfrom from Mesh space to gameobject space store the identity matrix for now
	bool initialized = false;
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
	// void create_mesh(const std::vector<Vertex>& vertices, const std::vector<Vertex>& indices);
};

struct Light { // ordered this way because of HLSL structure packing rules
	DirectX::XMFLOAT3 m_strength = { 0.5, 0.5, 0.5 };
	float m_falloff_start = 1.0f;
	DirectX::XMFLOAT3 m_direction = { 0.0f, -1.0f, 0.0f };
	float m_falloff_end = 10.0f;
	DirectX::XMFLOAT3 m_position = { 0.0f, 0.0f, 0.0f };
	float m_spot_power = 64.0f;
};
#define MAXLIGHTS 16

struct MaterialConstants {
	DirectX::XMFLOAT4 m_diffuse_albedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 m_fresnel_r0 = { 0.01f, 0.01f, 0.01f };
	float m_roughness = 0.25f;
	// used in texture mapping
	DirectX::XMFLOAT4X4 m_mat_transform = MathHelper::identity_4x4();
};
// simple struct for now
// professional engines use a class hierarchy of materials
struct Material {
	std::string name; // change to hash
	//uint64_t hash_name;
	// index into constant buffer corresponding to this material
	int m_mat_cb_index = -1;
	int m_diffuse_srv_heap_index = -1;
	int m_normal_srv_heap_index = -1;
	// dirty flag indicating the material has changed and we need to update the constant buffer
	int m_num_frames_dirty = g_num_frame_resources;
	// material constant buffer data used for shading (get's copied into the Material Constants structure
	DirectX::XMFLOAT4 m_diffuse_albedo = { 1.0f, 1.0f, 1.0f, 1.0f };
	DirectX::XMFLOAT3 m_fresnel_r0 = { 0.01f, 0.01f, 0.01f };
	float m_roughness = 0.25f;
	DirectX::XMFLOAT4X4 m_mat_transform = MathHelper::identity_4x4();
};



#ifndef ReleaseCom
#define ReleaseCom(x) { if(x){ x->Release(); x = 0;}}
#endif // !ReleaseCom
