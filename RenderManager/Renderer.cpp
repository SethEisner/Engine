#include "Renderer.h"
#include <assert.h>
#include <DirectXColors.h>
//#include <DirectXHelpers.h>
#include <d3d12sdklayers.h>

LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
	switch (uMsg)
	{
	case WM_DESTROY:
		PostQuitMessage(0);
		exit(0);
	case WM_PAINT:
	{
		PAINTSTRUCT ps;
		HDC hdc = BeginPaint(hwnd, &ps);

		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));

		EndPaint(hwnd, &ps);
	}
	return 0;

	}
	return DefWindowProc(hwnd, uMsg, wParam, lParam);
}

Renderer::Renderer(HINSTANCE hInstance) : m_window(new Window(hInstance, L"class name", L"Window name")) {

}
Renderer::~Renderer() {

}
bool Renderer::init() {
	//assert(m_window->init()); // if we cant create the window then we can't render to the window
	//init_window();
	if (!m_window->init()) return false;

#if defined(DEBUG) || defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug> debug_controller;
	ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(debug_controller.GetAddressOf())));
	debug_controller->EnableDebugLayer();
#endif

	ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(m_dxgi_factory.GetAddressOf())));

	// find the hardware device that supports directx12 and store it in m_d3d_device member
	// IID_PPV_ARGS is used  to retrieve an interface pointer
	ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(m_d3d_device.GetAddressOf())));
	ThrowIfFailed(m_d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()))); // fence is for syncronizing the CPU and GPU, will remove later
	m_rtv_descriptor_size = m_d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsv_descriptor_size = m_d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cbv_srv_uav_descriptor_size = m_d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// check 4x MSAA quality supprt level for out back buffer format (all dx11 capable devices support 4xMSAA
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms_quality_levels;
	ms_quality_levels.Format = m_back_buffer_format;
	ms_quality_levels.SampleCount = 4; // 4 because 4x MSAA
	ms_quality_levels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	ms_quality_levels.NumQualityLevels = 0;
	ThrowIfFailed(m_d3d_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &ms_quality_levels, sizeof(ms_quality_levels)));
	m_4xMSAA_quality = ms_quality_levels.NumQualityLevels;
	assert(m_4xMSAA_quality > 0);
	create_command_objects();
	create_swap_chain();
	create_rtv_and_dsv_descriptor_heaps();
	return true;
}


void Renderer::update() {
	// nothing to update yet
}
void Renderer::draw() {
	ThrowIfFailed(m_command_list_allocator->Reset()); // reset the allocator to accept new commands (must be done after GPU is done using the allocator
	// reset command list after it has been added to the command queue
	ThrowIfFailed(m_command_list->Reset(m_command_list_allocator.Get(), nullptr));
	// indicate a state transition on the resource usage
	m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(current_back_buffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	// set the viewport and scissor rectangle. must be reset whenever the command list is reset
	m_command_list->RSSetViewports(1, &m_screen_viewport);
	m_command_list->RSSetScissorRects(1, &m_scissor_rect);
	// clear the back buffer and the depth buffer
	m_command_list->ClearRenderTargetView(current_back_buffer_view(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	m_command_list->ClearDepthStencilView(depth_stencil_view(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	// specify the buffers we are going to render to
	m_command_list->OMSetRenderTargets(1, &current_back_buffer_view(), true, &depth_stencil_view());
	// indicate a state transition on the resource usage
	m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(current_back_buffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	// done recording commands
	ThrowIfFailed(m_command_list->Close());
	// add the command list to the queue for execution
	ID3D12CommandList* cmd_lists[] = { m_command_list.Get() };
	m_command_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists);
	// swap the back and front buffers
	ThrowIfFailed(m_swap_chain->Present(0, 0));
	m_current_back_buffer = (m_current_back_buffer + 1) % SWAP_CHAIN_BUFFER_COUNT;
	// wait until the commands are finished executing
	flush_command_queue();

}
void Renderer::shutdown() {

}
void Renderer::on_resize() {
	assert(m_d3d_device);
	assert(m_swap_chain);
	assert(m_command_list_allocator);
	flush_command_queue();
	ThrowIfFailed(m_command_list->Reset(m_command_list_allocator.Get(), nullptr));
	// release resources we will be recreating
	for (size_t i = 0; i != SWAP_CHAIN_BUFFER_COUNT; ++i) {
		m_swap_chain_buffer[i].Reset();
	}
	m_depth_stencil_buffer.Reset();
	// resize the swap chain
	ThrowIfFailed(m_swap_chain->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, m_window->m_width, m_window->m_height, m_back_buffer_format, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
	m_current_back_buffer = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_heap_handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart());
	for (size_t i = 0; i != SWAP_CHAIN_BUFFER_COUNT; ++i) {
		ThrowIfFailed(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&m_swap_chain_buffer[i])));
		m_d3d_device->CreateRenderTargetView(m_swap_chain_buffer[i].Get(), nullptr, rtv_heap_handle);
		rtv_heap_handle.Offset(1, m_rtv_descriptor_size);
	}
	// create the depth/stencil buffer and view (with the new size)
	D3D12_RESOURCE_DESC depth_stencil_desc;
	depth_stencil_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depth_stencil_desc.Alignment = 0;
	depth_stencil_desc.Width =m_window->m_width;
	depth_stencil_desc.Height = m_window->m_height;
	depth_stencil_desc.DepthOrArraySize = 1;
	depth_stencil_desc.MipLevels = 1;
	depth_stencil_desc.Format = DXGI_FORMAT_R24G8_TYPELESS;
	depth_stencil_desc.SampleDesc.Count = 1; // m_4xMSAA ? 4 : 1;
	depth_stencil_desc.SampleDesc.Quality = 0; //  m_4xMSAA ? (m_4xMSAA_quality - 1) : 0;
	depth_stencil_desc.Layout = D3D12_TEXTURE_LAYOUT_UNKNOWN;
	depth_stencil_desc.Flags = D3D12_RESOURCE_FLAG_ALLOW_DEPTH_STENCIL;
	D3D12_CLEAR_VALUE opt_clear;
	opt_clear.Format = m_depth_stencil_format;
	opt_clear.DepthStencil.Depth = 1.0f;
	opt_clear.DepthStencil.Stencil = 0;
	ThrowIfFailed(m_d3d_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE, 
			&depth_stencil_desc, D3D12_RESOURCE_STATE_COMMON, &opt_clear, IID_PPV_ARGS(m_depth_stencil_buffer.GetAddressOf())));

	// create descriptor to mip level 0 of entire resource using the format of the resource
	D3D12_DEPTH_STENCIL_VIEW_DESC dsv_desc;
	dsv_desc.Flags = D3D12_DSV_FLAG_NONE;
	dsv_desc.ViewDimension = D3D12_DSV_DIMENSION_TEXTURE2D;
	dsv_desc.Format = m_depth_stencil_format;
	dsv_desc.Texture2D.MipSlice = 0;
	m_d3d_device->CreateDepthStencilView(m_depth_stencil_buffer.Get(), &dsv_desc, depth_stencil_view());

	// transition the resource from its initial state to be used as a depth buffer
	m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(m_depth_stencil_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_DEPTH_WRITE));
	// execute the resize commands
	ThrowIfFailed(m_command_list->Close());
	ID3D12CommandList* cmd_lists[] = { m_command_list.Get() };
	m_command_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists);
	flush_command_queue();
	m_screen_viewport.TopLeftX = 0;
	m_screen_viewport.TopLeftY = 0;
	m_screen_viewport.Width = static_cast<float>(m_window->m_width);
	m_screen_viewport.Height = static_cast<float>(m_window->m_height);
	m_screen_viewport.MinDepth = 0.0f;
	m_screen_viewport.MaxDepth = 1.0f;
	// m_scissor_rect = { 0, 0, static_cast<float>(engine->get_window()->get_window_width()), static_cast<float>(engine->get_window()->get_window_height()) };
	m_scissor_rect = { 0, 0, static_cast<LONG>(m_window->m_width), static_cast<LONG>(m_window->m_height) };
}
void Renderer::create_command_objects() {
	D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
	command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	ThrowIfFailed(m_d3d_device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(m_command_queue.GetAddressOf())));
	ThrowIfFailed(m_d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_command_list_allocator.GetAddressOf())));
	ThrowIfFailed(m_d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_list_allocator.Get(), nullptr, IID_PPV_ARGS(m_command_list.GetAddressOf())));
	m_command_list->Close();
}
void Renderer::create_swap_chain() {
	m_swap_chain.Reset();
	DXGI_SWAP_CHAIN_DESC1 s;
	s.Width = m_window->m_width;
	s.Height = m_window->m_height;
	s.Format = m_back_buffer_format;
	s.Stereo = false;
	s.SampleDesc.Count = 1;
	s.SampleDesc.Quality = 0;
	s.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	s.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	s.Scaling = DXGI_SCALING_NONE;
	s.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	s.AlphaMode = DXGI_ALPHA_MODE_IGNORE;
	s.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;


	ThrowIfFailed(m_dxgi_factory->CreateSwapChainForHwnd(m_command_queue.Get(), m_window->m_handle, &s, NULL, NULL, m_swap_chain.GetAddressOf()));

	// DXGI_SWAP_CHAIN_DESC swap_chain_desc;
	// ZeroMemory(&swap_chain_desc, sizeof(DXGI_SWAP_CHAIN_DESC));
	// swap_chain_desc.BufferDesc.Width = m_window->m_width;
	// swap_chain_desc.BufferDesc.Height = m_window->m_height;
	// swap_chain_desc.BufferDesc.RefreshRate.Numerator = 60;
	// swap_chain_desc.BufferDesc.RefreshRate.Denominator = 1;
	// swap_chain_desc.BufferDesc.Format = m_back_buffer_format;
	// swap_chain_desc.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	// swap_chain_desc.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	// // need to create MSAA render target and explitiyly resolve to the DXGI back buffer per presentation
	// swap_chain_desc.SampleDesc.Count = 1; // m_4xMSAA ? 4 : 1; // no support for MSAA swap chains
	// swap_chain_desc.SampleDesc.Quality = 0; // m_4xMSAA ? (m_4xMSAA_quality - 1) : 0;
	// swap_chain_desc.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT; // SET THE SWAP BUFFER TO BE THE RENDER TARGET OUTPUT
	// swap_chain_desc.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	// swap_chain_desc.OutputWindow = m_window->m_handle;// m_window.get_handle();
	// swap_chain_desc.Windowed = true;
	// swap_chain_desc.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	// swap_chain_desc.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	// 
	// // set fullscreen description to null to make a windowed swap chain
	// assert(&swap_chain_desc != NULL);
	// assert(m_command_queue != NULL);
	// 
	// assert(m_command_queue.Get() != NULL);
	// assert(m_swap_chain.GetAddressOf() != NULL);//m_swap_chain.GetAddressOf() != NULL);
	// //ThrowIfFailed(m_dxgi_factory->CreateSwapChainForHwnd(m_command_queue.Get(), m_window.get_handle(), &scd1, NULL, NULL, m_swap_chain.GetAddressOf()));
	// // ThrowIfFailed(m_dxgi_factory->CreateSwapChain(m_command_queue.Get(), &swap_chain_desc, m_swap_chain.GetAddressOf()));//m_swap_chain.GetAddressOf()));
	// //ThrowIfFailed(m_dxgi_factory->CreateSwapChain(m_command_queue.Get(), &swap_chain_desc, m_swap_chain.GetAddressOf()));//m_swap_chain.GetAddressOf()));
	// ThrowIfFailed(m_dxgi_factory->CreateSwapChain(m_command_queue.Get(), &swap_chain_desc, m_swap_chain.GetAddressOf()));
	//m_dxgi_factory->CreateSwapChain(m_command_queue.Get(), &swap_chain_desc, m_swap_chain.GetAddressOf());
	//ThrowIfFailed(m_dxgi_factory->CreateSwapChain(m_d3d_device.Get(), &swap_chain_desc, m_swap_chain.GetAddressOf()));
}
void Renderer::create_rtv_and_dsv_descriptor_heaps() {
	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc;
	rtv_heap_desc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
	rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtv_heap_desc.NodeMask = 0;
	ThrowIfFailed(m_d3d_device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(m_rtv_heap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc;
	dsv_heap_desc.NumDescriptors = 1;
	dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsv_heap_desc.NodeMask = 0;
	ThrowIfFailed(m_d3d_device->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(m_dsv_heap.GetAddressOf())));
}
void Renderer::flush_command_queue() {
	m_current_fence++;
	// add a fence to the command queue, and wait until the fence is reached. waits for the command queue to become empty
	ThrowIfFailed(m_command_queue->Signal(m_fence.Get(), m_current_fence));
	if (m_fence->GetCompletedValue() < m_current_fence) {
		HANDLE event_handle = CreateEventEx(nullptr, NULL, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_current_fence, event_handle));
		WaitForSingleObject(event_handle, INFINITE);
		CloseHandle(event_handle);
	}
}
ID3D12Resource* Renderer::current_back_buffer() const {
	return m_swap_chain_buffer[m_current_back_buffer].Get();
}
D3D12_CPU_DESCRIPTOR_HANDLE Renderer::current_back_buffer_view() const {
	return CD3DX12_CPU_DESCRIPTOR_HANDLE(m_rtv_heap->GetCPUDescriptorHandleForHeapStart(), m_current_back_buffer, m_rtv_descriptor_size);
}
D3D12_CPU_DESCRIPTOR_HANDLE Renderer::depth_stencil_view() const {
	return m_dsv_heap->GetCPUDescriptorHandleForHeapStart();
}