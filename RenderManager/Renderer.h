#pragma once
#include <stdint.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <dxgi.h>
#include <d3d12.h>
#include "d3dx12.h"
#include "Window.h"
#include "../Utilities/Utilities.h"
#include <windows.h>
//#include "RenderTypes.h"
#include <vector>
#include <array>
#include <unordered_map>
//#include "Mesh.h"
#include "MathHelper.h"
#include "UploadBuffer.h"
#include "d3dUtil.h"
#include "FrameResources.h"
#include "Camera.h"
#include "Timer.h"
#include "RenderItem.h"
#include "DDSTextureLoader.h"
#include "ResourceUploadBatch.h"
#include <queue>

class Engine;
// contains a command queue, heaps, vector of render items that's in the potentially visible set
// eventually contains a struct that details the graphics options sin use and a way to change them
//
// #pragma comment(lib,"d3dcompiler.lib")
// #pragma comment(lib, "D3D12.lib")
// #pragma comment(lib, "dxgi.lib")




static enum class RenderLayers {
	opaque = 0
};

class Renderer {
public:
	Renderer() = default;
	~Renderer() = default;
	//bool init_window();
	bool init();
	void update();
	void draw();
	void shutdown();
	void on_resize();
	//Window* get_window() const;
	Microsoft::WRL::ComPtr<ID3D12Device> get_device();
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> get_command_list(size_t id);
	void reset_command_list(size_t id);
	void close_command_list(size_t id);
	void add_mesh(const Mesh* mesh);
	void create_and_add_texture(const std::string& name, const std::string& filename, size_t id, Mesh& corresponding_mesh, TextureFlags flag); // needs to edit the mesh
private:
	void create_command_objects();
	void create_swap_chain();
	void create_rtv_and_dsv_descriptor_heaps();
	void create_dummy_texture();
	void flush_command_queue();
	ID3D12Resource* current_back_buffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE current_back_buffer_view() const;
	D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_view() const;
	void build_descriptor_heaps(const Mesh* mesh);
	//void build_constant_buffers();
	void build_root_signature();
	void build_shaders_and_input_layout();
	void build_psos();
	// void build_frame_resources();
	void build_materials();
	void build_render_items();
	void draw_render_items(ID3D12GraphicsCommandList*, const std::vector<RenderItem>&);
	void animate_materials(const Timer&);
	void update_object_cbs(const Timer&);
	void update_material_buffer(const Timer&);
	void update_main_pass_cb(const Timer&);
	// void load_textures();
	std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> get_static_samplers();

	bool m_4xMSAA = true;
	uint32_t m_4xMSAA_quality = 0;
	// dxgi is directx graphics infrastructure
	Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgi_factory; // factory is for generating dxgi objects
	Microsoft::WRL::ComPtr<IDXGISwapChain> m_swap_chain;
	Microsoft::WRL::ComPtr<ID3D12Device> m_d3d_device;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	size_t m_current_fence = 0;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_command_queue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_command_list_allocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_command_list;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_command_lists[1]; // publically facing command lists that other threads can access using their job system thread id
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_command_list_allocators[1];
	size_t m_num_command_lists = 1;
	static const size_t SWAP_CHAIN_BUFFER_COUNT = 2;
	size_t m_current_back_buffer = 0;
	Microsoft::WRL::ComPtr<ID3D12Resource> m_swap_chain_buffer[SWAP_CHAIN_BUFFER_COUNT];
	Microsoft::WRL::ComPtr<ID3D12Resource> m_depth_stencil_buffer;
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_rtv_heap; // heap to store the render target view
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_dsv_heap; // heap to store the depth stencil view
	D3D12_VIEWPORT m_screen_viewport; // screen viewport structure
	D3D12_RECT m_scissor_rect; // scissor rectangel structure
	size_t m_rtv_descriptor_size;// = 0; // set in init function
	size_t m_dsv_descriptor_size;// = 0;
	size_t m_cbv_srv_uav_descriptor_size;// = 0;
	D3D_DRIVER_TYPE m_d3d_driver_type = D3D_DRIVER_TYPE_HARDWARE; 
	DXGI_FORMAT m_back_buffer_format = DXGI_FORMAT_R8G8B8A8_UNORM;
	DXGI_FORMAT	m_depth_stencil_format = DXGI_FORMAT_D24_UNORM_S8_UINT; // 24 bit depth, 8 bit stencil
	//Window* m_window;

	//Chapter 6 specific items
	Microsoft::WRL::ComPtr<ID3D12RootSignature> m_root_signature = nullptr;;
	//Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_cbv_heap = nullptr;
	UploadBuffer<ObjectConstants>* m_object_cb = nullptr;
	Mesh* m_box_geo = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> m_vs_bytecode = nullptr; // compiled shader code for the vertex shader
	Microsoft::WRL::ComPtr<ID3DBlob> m_ps_bytecode = nullptr; // compiled shader code for the pixel shader
	std::vector<D3D12_INPUT_ELEMENT_DESC> m_input_layout;
	Microsoft::WRL::ComPtr<ID3D12PipelineState> m_pso = nullptr;
	
	// chapter 15 specific items
	std::queue<FrameResources*> m_frame_resources; // the frame resources ring buffer
	// std::queue<std::unique_ptr<FrameResources>> m_frame_resources;
	// std::vector<std::unique_ptr<FrameResources>> m_frame_resources;
	// std::vector<FrameResources*> m_frame_resources;
	FrameResources* m_curr_frame_resource = nullptr; // the frame resoures structure for the current frame
	int m_curr_frame_resources_index = -1; // start at -1 because we add to it immediately (allows us to start from 0)
	uint32_t m_cbv_srv_descriptor_size = 0;
	// Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> m_srv_descriptor_heap = nullptr;
	std::unordered_map<size_t, const Mesh*> m_geometries; // map from mesh id to the mesh pointer we have stored for it
	std::unordered_map<std::string, Material*> m_materials;
	// std::unordered_map<std::string, Texture*> m_textures; // textures store the material data
	std::unordered_map<std::string, Microsoft::WRL::ComPtr<ID3DBlob>> m_shaders;
	std::unordered_map<RenderLayers, Microsoft::WRL::ComPtr<ID3D12PipelineState>> m_psos;
	std::vector<RenderItem> m_render_items; // all render items for the frame
	std::vector<RenderItem> m_opaque_render_items; // render items for a pso
	PassConstants m_main_pass_cb;
	Camera m_camera; // move to engine?
	// directx12 texture specific
	DirectX::ResourceUploadBatch* m_upload_batch = {};

	// texture system members
	std::unordered_map<size_t, std::array<Texture*, NUM_TEXTURES>> m_texture_map; // map from mesh pointer to the array of textures it uses, use the Mesh pointer to get the texture flags and set the constants
	// std::unordered_map<size_t, std::vector<std::pair<TextureFlags, Texture*>>> m_texture_map; // map from mesh id to the contiguous vector of tectures it uses, where each texture has it's corresponding type
	// will use the flags in the shader code to get a 0 or 1 which controls whether or not we use the result of the calculation
	std::unordered_map<size_t, Microsoft::WRL::ComPtr<ID3D12DescriptorHeap>> m_descriptor_heap_map; // map from the mesh to the descritor heaps for the textures it uses
	Texture* m_dummy_texture = nullptr;
	bool m_added_textures = false;
	bool m_scene_ready = false;
	bool m_created_frame_resources = false;
};