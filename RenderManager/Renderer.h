#pragma once
#include <stdint.h>
#include <wrl/client.h>
#include <dxgi1_4.h>
#include <dxgi.h>
#include <d3d12.h>
#include "../d3dx12.h"
#include "Window.h"
#include "../Utilities/Utilities.h"
#include <windows.h>

// contains a command queue, heaps, vector of render items that's in the potentially visible set
// eventually contains a struct that details the graphics options sin use and a way to change them
//
// #pragma comment(lib,"d3dcompiler.lib")
// #pragma comment(lib, "D3D12.lib")
// #pragma comment(lib, "dxgi.lib")

class Renderer {
public:
	Renderer() = delete;
	explicit Renderer(HINSTANCE hInstance);
	~Renderer();
	bool init_window();
	bool init();
	void update();
	void draw();
	void shutdown();
	void on_resize();
private:
	void create_command_objects();
	void create_swap_chain();
	void create_rtv_and_dsv_descriptor_heaps();
	void flush_command_queue();
	ID3D12Resource* current_back_buffer() const;
	D3D12_CPU_DESCRIPTOR_HANDLE current_back_buffer_view() const;
	D3D12_CPU_DESCRIPTOR_HANDLE depth_stencil_view() const;
	bool m_4xMSAA = true;
	uint32_t m_4xMSAA_quality = 0;
	// dxgi is directx graphics infrastructure
	Microsoft::WRL::ComPtr<IDXGIFactory4> m_dxgi_factory; // factory is for generating dxgi objects
	//IDXGIFactory* m_dxgi_factory;
	Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swap_chain;
	//Microsoft::WRL::ComPtr<IDXGISwapChain1> m_swap_chain1;
	//IDXGISwapChain* m_swap_chain;
	Microsoft::WRL::ComPtr<ID3D12Device> m_d3d_device;
	Microsoft::WRL::ComPtr<ID3D12Fence> m_fence;
	size_t m_current_fence = 0;
	Microsoft::WRL::ComPtr<ID3D12CommandQueue> m_command_queue;
	//ID3D12CommandQueue* m_command_queue;
	Microsoft::WRL::ComPtr<ID3D12CommandAllocator> m_command_list_allocator;
	Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> m_command_list;
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
	Window* m_window;
};