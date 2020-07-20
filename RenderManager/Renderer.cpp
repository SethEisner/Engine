#include "Renderer.h"
#include <assert.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include "FrameResources.h"
#include "Scene.h"
#include "../Engine.h"
#include <exception>
#include <queue>
#include <DDSTextureLoader.h>
#include "../ResourceManager/ResourceManager.h"
#include <DDSTextureLoader.h>
#include "DDSTextureLoader12.h"



Microsoft::WRL::ComPtr<ID3D12Device> Renderer::get_device() {
	return m_d3d_device;
}
Microsoft::WRL::ComPtr<ID3D12GraphicsCommandList> Renderer::get_command_list(size_t id) {
	return m_command_lists[id];
}
void Renderer::reset_command_list(size_t id) {
	m_command_lists[id]->Reset(m_command_list_allocators[id].Get(), nullptr); // ThrowIfFailed(m_command_lists[id]->Reset(m_command_list_allocators[id].Get(), nullptr));
}
void Renderer::close_command_list(size_t id) { // WRONG!!! funciton is used now to build the Mesh buffers while creating the scene, 
	// need to ensure that once we start submitting draw calls from other threads that we flush the command queue with every thread's command list
	// right now we would flush the command queue for every thread which is very wasteful
	m_command_lists[id]->Close(); // ThrowIfFailed(m_command_lists[id]->Close()); // execute the initialization commands
	ID3D12CommandList* cmd_lists[] = { m_command_lists[id].Get() };
	m_command_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists);
	flush_command_queue();
}
void Renderer::add_mesh(const Mesh* mesh) {
	m_geometries[mesh->m_mesh_id] = mesh;
	build_descriptor_heaps(mesh); // build the descriptor heap for the mesh
	//build_render_item(mesh);
	m_scene_ready = true;
}
void Renderer::create_and_add_texture(const std::string& name, const std::string& filename, size_t id, Mesh& corresponding_mesh, TextureFlags flag) {// use the thread's command list to create and add the texture 

	//DirectX::ResourceUploadBatch resource_upload(m_d3d_device.Get());
	Texture* texture = new Texture();
	texture->m_name = name;
	texture->m_filename = filename;
	auto temp1 = engine->resource_manager->get_data_pointer(texture->m_filename);
	auto temp2 = engine->resource_manager->get_data_size(texture->m_filename);

	CreateDDSTextureFromMemory12(m_d3d_device.Get(), m_command_lists[id].Get(),
		engine->resource_manager->get_data_pointer(texture->m_filename),
		engine->resource_manager->get_data_size(texture->m_filename), texture->m_resource, texture->m_upload_heap);
	/*
	ThrowIfFailed(CreateDDSTextureFromMemory12(m_d3d_device.Get(), m_command_lists[id].Get(),
		engine->resource_manager->get_data_pointer(texture->m_filename),
		engine->resource_manager->get_data_size(texture->m_filename), texture->m_resource, texture->m_upload_heap)); */
	//m_textures[tex->m_name] = std::move(tex);
	// set the texture map
	int index = get_texture_index(flag);
	assert(m_texture_map[corresponding_mesh.m_mesh_id][index] == nullptr); // assert that we are not overwriting the data
	m_texture_map[corresponding_mesh.m_mesh_id][index] = texture;
	corresponding_mesh.m_textures_used = corresponding_mesh.m_textures_used | flag;

}
bool Renderer::init() {

#if defined(DEBUG) || defined(_DEBUG)
	Microsoft::WRL::ComPtr<ID3D12Debug> debug_controller;
	D3D12GetDebugInterface(IID_PPV_ARGS(debug_controller.GetAddressOf())); // ThrowIfFailed(D3D12GetDebugInterface(IID_PPV_ARGS(debug_controller.GetAddressOf())));
	debug_controller->EnableDebugLayer();
#endif

	CreateDXGIFactory1(IID_PPV_ARGS(m_dxgi_factory.GetAddressOf())); // ThrowIfFailed(CreateDXGIFactory1(IID_PPV_ARGS(m_dxgi_factory.GetAddressOf())));

	// find the hardware device that supports directx12 and store it in m_d3d_device member
	// IID_PPV_ARGS is used  to retrieve an interface pointer
	D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_11_0, IID_PPV_ARGS(m_d3d_device.GetAddressOf())); // ThrowIfFailed(D3D12CreateDevice(nullptr, D3D_FEATURE_LEVEL_12_0, IID_PPV_ARGS(m_d3d_device.GetAddressOf())));
	m_d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()));// ThrowIfFailed(m_d3d_device->CreateFence(0, D3D12_FENCE_FLAG_NONE, IID_PPV_ARGS(m_fence.GetAddressOf()))); // fence is for syncronizing the CPU and GPU, will remove later
	m_rtv_descriptor_size = m_d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_RTV);
	m_dsv_descriptor_size = m_d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_DSV);
	m_cbv_srv_uav_descriptor_size = m_d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);

	// check 4x MSAA quality supprt level for out back buffer format (all dx11 capable devices support 4xMSAA
	D3D12_FEATURE_DATA_MULTISAMPLE_QUALITY_LEVELS ms_quality_levels;
	ms_quality_levels.Format = m_back_buffer_format;
	ms_quality_levels.SampleCount = 4; // 4 because 4x MSAA
	ms_quality_levels.Flags = D3D12_MULTISAMPLE_QUALITY_LEVELS_FLAG_NONE;
	ms_quality_levels.NumQualityLevels = 0;
	m_d3d_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &ms_quality_levels, sizeof(ms_quality_levels)); // ThrowIfFailed(m_d3d_device->CheckFeatureSupport(D3D12_FEATURE_MULTISAMPLE_QUALITY_LEVELS, &ms_quality_levels, sizeof(ms_quality_levels)));
	m_4xMSAA_quality = ms_quality_levels.NumQualityLevels;
	assert(m_4xMSAA_quality > 0);
	create_command_objects();
	create_swap_chain();
	create_rtv_and_dsv_descriptor_heaps(); // for the render target and depth stencil view
	create_dummy_texture();
	m_cbv_srv_descriptor_size = m_d3d_device->GetDescriptorHandleIncrementSize(D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV);
	// m_frame_resources.resize(g_num_frame_resources);
	// camera position can be set anywhere
	engine->camera->set_position(0.0f, 2.0f, -10.0f);
	// build the basic structures the rending pipeline can use to render our objects
	build_root_signature();
	build_shaders_and_input_layout();
	build_psos();
	return true;
}


void Renderer::update() {
	build_render_items();
	if (m_render_items.empty()) return; // if there is nothing to render then there is nothing to update

	m_curr_frame_resources_index = (m_curr_frame_resources_index + 1) % g_num_frame_resources;
	

	// dont want to create new frame_resources every frame, want to reuse as it's slow to allocate the memory in the loop


	// m_curr_frame_resources_index = (m_curr_frame_resources_index + 1) % g_num_frame_resources;
	// m_curr_frame_resource = m_frame_resources[m_curr_frame_resources_index];
	if (m_frame_resources.size() == 3) {
		// GPU has not finished using this frame resource so we must wait until the GPU is done
		if (m_frame_resources.front()->m_fence != 0 && m_fence->GetCompletedValue() < m_frame_resources.front()->m_fence) {
			HANDLE event_handle = CreateEventEx(nullptr, NULL, false, EVENT_ALL_ACCESS);
			m_fence->SetEventOnCompletion(m_frame_resources.front()->m_fence, event_handle); // ThrowIfFailed(m_fence->SetEventOnCompletion(front_frame_resource->m_fence, event_handle));
			WaitForSingleObject(event_handle, INFINITE);
			CloseHandle(event_handle);
		}
		FrameResources* temp = std::move(m_frame_resources.front());
		//delete temp;
		//delete m_frame_resources.front();
		m_frame_resources.pop(); // remove it from the queue
		m_frame_resources.emplace(temp);
		// m_curr_frame_resource = front_frame_resource;
	}
	else {
		m_frame_resources.emplace(new FrameResources(m_d3d_device.Get(), 1, m_render_items.size(), m_materials.size()));
	}
	m_curr_frame_resource = m_frame_resources.front();
	size_t a = sizeof(*m_curr_frame_resource);
	
	animate_materials(*engine->global_timer);
	update_object_cbs(*engine->global_timer);
	update_material_buffer(*engine->global_timer);
	update_main_pass_cb(*engine->global_timer);
}
void Renderer::draw() {
	if (m_render_items.empty()) return; // if there is nothing to render then there is nothing to draw

	auto command_list_allocator = m_curr_frame_resource->m_cmd_list_allocator;
	command_list_allocator->Reset(); // ThrowIfFailed(command_list_allocator->Reset()); // reset the allocator to accept new commands (must be done after GPU is done using the allocator
	// reset command list after it has been added to the command queue
	m_command_list->Reset(command_list_allocator.Get(), m_psos[RenderLayers::opaque/*"opaque"*/].Get()); // ThrowIfFailed(m_command_list->Reset(command_list_allocator.Get(), m_psos["opaque"].Get()));


	// indicate a state transition on the resource usage
	//m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(current_back_buffer(), D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));
	// set the viewport and scissor rectangle. must be reset whenever the command list is reset
	m_command_list->RSSetViewports(1, &m_screen_viewport);
	m_command_list->RSSetScissorRects(1, &m_scissor_rect);

	// chapter 6
	m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(current_back_buffer(),
		D3D12_RESOURCE_STATE_PRESENT, D3D12_RESOURCE_STATE_RENDER_TARGET));

	// clear the back buffer and the depth buffer
	m_command_list->ClearRenderTargetView(current_back_buffer_view(), DirectX::Colors::LightSteelBlue, 0, nullptr);
	m_command_list->ClearDepthStencilView(depth_stencil_view(), D3D12_CLEAR_FLAG_DEPTH | D3D12_CLEAR_FLAG_STENCIL, 1.0f, 0, 0, nullptr);
	// specify the buffers we are going to render to
	m_command_list->OMSetRenderTargets(1, &current_back_buffer_view(), true, &depth_stencil_view());

	// should be render item independent
	m_command_list->SetGraphicsRootSignature(m_root_signature.Get());
	auto pass_cb = m_curr_frame_resource->m_pass_cb->get_resource();
	m_command_list->SetGraphicsRootConstantBufferView(1, pass_cb->GetGPUVirtualAddress());


	draw_render_items(m_command_list.Get(), m_opaque_render_items);
	m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(current_back_buffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	m_command_list->Close(); // ThrowIfFailed(m_command_list->Close());


	ID3D12CommandList* cmd_lists[] = { m_command_list.Get() };
	m_command_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists);
	// swap the back and front buffers
	
	m_swap_chain->Present(0, 0); // ThrowIfFailed(m_swap_chain->Present(0, 0));
	//ThrowIfFailed(m_swap_chain->Present(0, 0));
	m_current_back_buffer = (m_current_back_buffer + 1) % SWAP_CHAIN_BUFFER_COUNT;
	// advance the dence value to mark the commands up to this fence point
	m_curr_frame_resource->m_fence = ++m_current_fence;
	// add an instruction to the command queue to set a new fence point
	// the new fence point is set once the GPU finishes processing all the command prior to this signal
	m_command_queue->Signal(m_fence.Get(), m_current_fence);
}
void Renderer::shutdown() {

}
void Renderer::on_resize() {
	assert(m_d3d_device);
	assert(m_swap_chain);
	assert(m_command_list_allocator);
	flush_command_queue();
	m_command_list->Reset(m_command_list_allocator.Get(), nullptr); // ThrowIfFailed(m_command_list->Reset(m_command_list_allocator.Get(), nullptr));
	// release resources we will be recreating
	for (size_t i = 0; i != SWAP_CHAIN_BUFFER_COUNT; ++i) {
		m_swap_chain_buffer[i].Reset();
	}
	m_depth_stencil_buffer.Reset();
	// resize the swap chain
	m_swap_chain->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, engine->window->m_width, engine->window->m_height, m_back_buffer_format, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH); // ThrowIfFailed(m_swap_chain->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, engine->window->m_width, engine->window->m_height, m_back_buffer_format, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
	m_current_back_buffer = 0;
	CD3DX12_CPU_DESCRIPTOR_HANDLE rtv_heap_handle(m_rtv_heap->GetCPUDescriptorHandleForHeapStart());
	for (size_t i = 0; i != SWAP_CHAIN_BUFFER_COUNT; ++i) {
		m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&m_swap_chain_buffer[i])); // ThrowIfFailed(m_swap_chain->GetBuffer(i, IID_PPV_ARGS(&m_swap_chain_buffer[i])));
		m_d3d_device->CreateRenderTargetView(m_swap_chain_buffer[i].Get(), nullptr, rtv_heap_handle);
		rtv_heap_handle.Offset(1, m_rtv_descriptor_size);
	}
	// create the depth/stencil buffer and view (with the new size)
	D3D12_RESOURCE_DESC depth_stencil_desc;
	depth_stencil_desc.Dimension = D3D12_RESOURCE_DIMENSION_TEXTURE2D;
	depth_stencil_desc.Alignment = 0;
	depth_stencil_desc.Width = engine->window->m_width;// m_window->m_width;
	depth_stencil_desc.Height = engine->window->m_height;// m_window->m_height; //m_window->m_height;
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
	m_d3d_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&depth_stencil_desc, D3D12_RESOURCE_STATE_COMMON, &opt_clear, IID_PPV_ARGS(m_depth_stencil_buffer.GetAddressOf()));
	/*
	ThrowIfFailed(m_d3d_device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT), D3D12_HEAP_FLAG_NONE,
		&depth_stencil_desc, D3D12_RESOURCE_STATE_COMMON, &opt_clear, IID_PPV_ARGS(m_depth_stencil_buffer.GetAddressOf()))); */

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
	m_command_list->Close(); // ThrowIfFailed(m_command_list->Close());
	ID3D12CommandList* cmd_lists[] = { m_command_list.Get() };
	m_command_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists);
	flush_command_queue();
	m_screen_viewport.TopLeftX = 0;
	m_screen_viewport.TopLeftY = 0;
	m_screen_viewport.Width = static_cast<float>(engine->window->m_width); //m_window->m_width);
	m_screen_viewport.Height = static_cast<float>(engine->window->m_height); //m_window->m_height);
	m_screen_viewport.MinDepth = 0.0f;
	m_screen_viewport.MaxDepth = 1.0f;
	// m_scissor_rect = { 0, 0, static_cast<float>(m_window->get_window_width()), static_cast<float>(m_window->get_window_height()) };
	m_scissor_rect = { 0, 0, static_cast<LONG>(engine->window->m_width), //m_window->m_width), 
		static_cast<LONG>(engine->window->m_height) };//m_window->m_height) };
}
void Renderer::create_command_objects() {
	D3D12_COMMAND_QUEUE_DESC command_queue_desc = {};
	command_queue_desc.Type = D3D12_COMMAND_LIST_TYPE_DIRECT;
	command_queue_desc.Flags = D3D12_COMMAND_QUEUE_FLAG_NONE;
	m_d3d_device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(m_command_queue.GetAddressOf())); // ThrowIfFailed(m_d3d_device->CreateCommandQueue(&command_queue_desc, IID_PPV_ARGS(m_command_queue.GetAddressOf())));
	m_d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_command_list_allocator.GetAddressOf())); // ThrowIfFailed(m_d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_command_list_allocator.GetAddressOf())));
	m_d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_list_allocator.Get(), nullptr, IID_PPV_ARGS(m_command_list.GetAddressOf())); // ThrowIfFailed(m_d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_list_allocator.Get(), nullptr, IID_PPV_ARGS(m_command_list.GetAddressOf())));
	for (size_t i = 0; i != m_num_command_lists; ++i) {
		m_d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_command_list_allocators[i].GetAddressOf())); // ThrowIfFailed(m_d3d_device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_command_list_allocators[i].GetAddressOf())));
		m_d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_list_allocators[i].Get(), nullptr, IID_PPV_ARGS(m_command_lists[i].GetAddressOf())); // ThrowIfFailed(m_d3d_device->CreateCommandList(0, D3D12_COMMAND_LIST_TYPE_DIRECT, m_command_list_allocators[i].Get(), nullptr, IID_PPV_ARGS(m_command_lists[i].GetAddressOf())));
		m_command_lists[i]->Close(); // ThrowIfFailed(m_command_lists[i]->Close());
	}
	m_command_list->Close();
}
void Renderer::create_swap_chain() {
	m_swap_chain.Reset();
	DXGI_SWAP_CHAIN_DESC scd;
	scd.BufferCount = SWAP_CHAIN_BUFFER_COUNT;
	scd.BufferDesc.Width = engine->window->m_width;
	scd.BufferDesc.Height = engine->window->m_height;
	scd.BufferDesc.RefreshRate.Numerator = 0;
	scd.BufferDesc.RefreshRate.Denominator = 1;
	scd.BufferDesc.Format = m_back_buffer_format;
	scd.BufferDesc.ScanlineOrdering = DXGI_MODE_SCANLINE_ORDER_UNSPECIFIED;
	scd.BufferDesc.Scaling = DXGI_MODE_SCALING_UNSPECIFIED;
	scd.SampleDesc.Count = 1;
	scd.SampleDesc.Quality = 0;
	scd.BufferUsage = DXGI_USAGE_RENDER_TARGET_OUTPUT;
	scd.OutputWindow = engine->window->m_handle;
	scd.Windowed = true;
	scd.SwapEffect = DXGI_SWAP_EFFECT_FLIP_DISCARD;
	scd.Flags = DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH;
	try {
		ThrowIfFailed(m_dxgi_factory->CreateSwapChain(m_command_queue.Get(), &scd, m_swap_chain.GetAddressOf()));
	}
	catch(DxException e){
		OutputDebugString(e.ToString().c_str());
	}
}
void Renderer::create_rtv_and_dsv_descriptor_heaps() {
	D3D12_DESCRIPTOR_HEAP_DESC rtv_heap_desc;
	rtv_heap_desc.NumDescriptors = SWAP_CHAIN_BUFFER_COUNT;
	rtv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_RTV;
	rtv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	rtv_heap_desc.NodeMask = 0;
	m_d3d_device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(m_rtv_heap.GetAddressOf())); // ThrowIfFailed(m_d3d_device->CreateDescriptorHeap(&rtv_heap_desc, IID_PPV_ARGS(m_rtv_heap.GetAddressOf())));

	D3D12_DESCRIPTOR_HEAP_DESC dsv_heap_desc;
	dsv_heap_desc.NumDescriptors = 1;
	dsv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_DSV;
	dsv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_NONE;
	dsv_heap_desc.NodeMask = 0;
	m_d3d_device->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(m_dsv_heap.GetAddressOf())); // ThrowIfFailed(m_d3d_device->CreateDescriptorHeap(&dsv_heap_desc, IID_PPV_ARGS(m_dsv_heap.GetAddressOf())));
}
void Renderer::create_dummy_texture() {
	std::string name = "default";
	std::string zip_file = name + ".zip";
	std::string filename = zip_file + "/black.dds";
	engine->resource_manager->load_resource(zip_file);
	//engine->resource_manager->load_resource("Sword.zip");
	while (!engine->resource_manager->resource_loaded(zip_file)) {} // wait until the zipfile is loaded (super fast because we 
	m_dummy_texture = new Texture();
	m_dummy_texture->m_name = "dummy_texture";
	m_dummy_texture->m_filename = filename;
	CreateDDSTextureFromMemory12(m_d3d_device.Get(), m_command_list.Get(),
		engine->resource_manager->get_data_pointer(m_dummy_texture->m_filename),
		engine->resource_manager->get_data_size(m_dummy_texture->m_filename), m_dummy_texture->m_resource, m_dummy_texture->m_upload_heap);
}
void Renderer::flush_command_queue() {
	m_current_fence++;
	// add a fence to the command queue, and wait until the fence is reached. waits for the command queue to become empty
	m_command_queue->Signal(m_fence.Get(), m_current_fence);//  ThrowIfFailed(m_command_queue->Signal(m_fence.Get(), m_current_fence));
	if (m_fence->GetCompletedValue() < m_current_fence) {
		HANDLE event_handle = CreateEventEx(nullptr, NULL, false, EVENT_ALL_ACCESS);
		m_fence->SetEventOnCompletion(m_current_fence, event_handle); // ThrowIfFailed(m_fence->SetEventOnCompletion(m_current_fence, event_handle));
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
void Renderer::build_descriptor_heaps(const Mesh* mesh) { // create heaps to hold the textures (maybe dont create them every frame but only for new textures...



	// create a new descritpor heap and map it to the given mesh
	Microsoft::WRL::ComPtr<ID3D12DescriptorHeap> srv_descriptor_heap = nullptr;
	D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
	srv_heap_desc.NumDescriptors = NUM_TEXTURES;
	srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//srv_heap_desc.NodeMask = 0;
	m_d3d_device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&srv_descriptor_heap)); // ThrowIfFailed(m_d3d_device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&srv_descriptor_heap)));

	m_descriptor_heap_map[mesh->m_mesh_id] = std::move(srv_descriptor_heap);

	// fill out the heap with our NUM_TEXTURE descriptors
	CD3DX12_CPU_DESCRIPTOR_HANDLE h_desc(m_descriptor_heap_map[mesh->m_mesh_id]->GetCPUDescriptorHandleForHeapStart());

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D; // only support 2d textures for now
	size_t offset = 0;

	uint32_t supported_textures = static_cast<uint32_t>(mesh->m_textures_used);
	static const uint32_t mask = 0x1;
	Texture* tex = nullptr;
	for (size_t i = 0; i != NUM_TEXTURES; ++i, supported_textures >>= 1){//, offset = 1) { // for each texture type the mesh could support
		// if it is supported, create the view
		Texture* tex = m_dummy_texture;
		h_desc.Offset(offset, m_cbv_srv_descriptor_size);
		if (supported_textures & mask) { // bind the real texture
			tex = m_texture_map[mesh->m_mesh_id][i]; // get the corresponding texture pointer
			
		}
		srv_desc.Format = tex->m_resource->GetDesc().Format;
		srv_desc.Texture2D.MipLevels = tex->m_resource->GetDesc().MipLevels;
		m_d3d_device->CreateShaderResourceView(tex->m_resource.Get(), &srv_desc, h_desc);
		offset = 1;
	}
}
void Renderer::build_root_signature() {
	CD3DX12_DESCRIPTOR_RANGE tex_table;
	tex_table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, NUM_TEXTURES, 0, 0);
	CD3DX12_ROOT_PARAMETER slot_root_parameter[4];
	slot_root_parameter[0].InitAsConstantBufferView(0);
	slot_root_parameter[1].InitAsConstantBufferView(1);
	slot_root_parameter[2].InitAsShaderResourceView(0, 1);
	slot_root_parameter[3].InitAsDescriptorTable(1, &tex_table, D3D12_SHADER_VISIBILITY_PIXEL);
	
	auto static_samplers = get_static_samplers();
	
	CD3DX12_ROOT_SIGNATURE_DESC root_sig_desc(4, slot_root_parameter, static_samplers.size(), static_samplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	Microsoft::WRL::ComPtr<ID3DBlob> serialized_root_sig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> error_blob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1, serialized_root_sig.GetAddressOf(), error_blob.GetAddressOf());
	if (error_blob) {
		OutputDebugStringA((char*)error_blob->GetBufferPointer());
	}
	m_d3d_device->CreateRootSignature(0, serialized_root_sig->GetBufferPointer(), serialized_root_sig->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)); // ThrowIfFailed(m_d3d_device->CreateRootSignature(0, serialized_root_sig->GetBufferPointer(), serialized_root_sig->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));
}
void Renderer::build_shaders_and_input_layout() {
	const D3D_SHADER_MACRO alpha_test_defines[] = {
		"ALPHA_TEST", "1", NULL, NULL
	};
	HRESULT hr = S_OK;
	m_shaders["standard_vs"] = compile_shader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_1");
	m_shaders["opaque_ps"] = compile_shader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_1");
	m_input_layout = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"NORMAL", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"TANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"BITANGENT", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 36, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 48, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}
void Renderer::build_psos() {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC opaque_pso_desc;
	ZeroMemory(&opaque_pso_desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	opaque_pso_desc.InputLayout = { m_input_layout.data(), (uint32_t)m_input_layout.size() };
	opaque_pso_desc.pRootSignature = m_root_signature.Get();
	opaque_pso_desc.VS = {
		reinterpret_cast<BYTE*>(m_shaders["standard_vs"]->GetBufferPointer()),
		m_shaders["standard_vs"]->GetBufferSize()
	};
	opaque_pso_desc.PS = { 
		reinterpret_cast<BYTE*>(m_shaders["opaque_ps"]->GetBufferPointer()),
		m_shaders["opaque_ps"]->GetBufferSize()
	};
	CD3DX12_RASTERIZER_DESC rasterizer_state = {};
	rasterizer_state.FillMode = D3D12_FILL_MODE_SOLID;
	// rasterizer_state.FillMode = D3D12_FILL_MODE_WIREFRAME;
	rasterizer_state.CullMode = D3D12_CULL_MODE_BACK;
	rasterizer_state.FrontCounterClockwise = FALSE;
	rasterizer_state.DepthBias = D3D12_DEFAULT_DEPTH_BIAS;
	rasterizer_state.DepthBiasClamp = D3D12_DEFAULT_DEPTH_BIAS_CLAMP;
	rasterizer_state.SlopeScaledDepthBias = D3D12_DEFAULT_SLOPE_SCALED_DEPTH_BIAS;
	rasterizer_state.DepthClipEnable = TRUE;
	rasterizer_state.MultisampleEnable = TRUE; // FALSE;
	rasterizer_state.AntialiasedLineEnable = FALSE;
	rasterizer_state.ForcedSampleCount = 0;
	rasterizer_state.ConservativeRaster = D3D12_CONSERVATIVE_RASTERIZATION_MODE_OFF;
	
	opaque_pso_desc.RasterizerState = rasterizer_state; // CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT); // rasterizer_state;
	// opaque_pso_desc.RasterizerState = rasterizer_state;
	opaque_pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaque_pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaque_pso_desc.SampleMask = UINT_MAX;
	opaque_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaque_pso_desc.NumRenderTargets = 1;
	opaque_pso_desc.RTVFormats[0] = m_back_buffer_format;
	opaque_pso_desc.SampleDesc.Count = 1;
	opaque_pso_desc.SampleDesc.Quality = 0;
	opaque_pso_desc.DSVFormat = m_depth_stencil_format;
	m_d3d_device->CreateGraphicsPipelineState(&opaque_pso_desc, IID_PPV_ARGS(&m_psos[RenderLayers::opaque/*"opaque"*/])); // ThrowIfFailed(m_d3d_device->CreateGraphicsPipelineState(&opaque_pso_desc, IID_PPV_ARGS(&m_psos["opaque"])));
}
void Renderer::build_materials() {}
void Renderer::build_render_items() { // can tranverse the meshes in the scene, cull them, and add what survives to the renderItem list
	// should use a scene hierarchy, could technically use the BVH we created for the physics
	m_render_items.clear(); // .resize(engine->scene->m_mesh->m_draw_args.size());
	m_opaque_render_items.clear(); // .resize(engine->scene->m_mesh->m_draw_args.size());
	size_t i = 0;
	for (std::pair<size_t, const Mesh*> geometry_pair : m_geometries) { // use the stored geometry pointers, instead of going through the scene. allows us to render only what the Scene has given us
		const Mesh* mesh = geometry_pair.second;
		for (const SubMesh& submesh : mesh->m_submeshes) {
			RenderItem ri = {};
			// go from submesh vertices to mesh, to game object, to world space
			// should do this multiplication in the vertex shader, as that's literally what it's for...
			XMStoreFloat4x4(&ri.m_world, DirectX::XMMatrixMultiply(DirectX::XMMatrixMultiply(XMLoadFloat4x4(&submesh.m_transform), XMLoadFloat4x4(&mesh->m_transform)), XMLoadFloat4x4(&mesh->m_game_object->m_transform)));
			XMStoreFloat4x4(&ri.m_tex_transform, DirectX::XMMatrixIdentity()); // DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f));
			ri.m_textures_used = static_cast<int>(engine->scene->m_floor->m_mesh->m_textures_used);
			ri.m_obj_cb_index = i++;
			ri.m_mesh = geometry_pair.second; // engine->scene->m_floor->m_mesh;
			ri.m_primitive_type = submesh.m_primitive;
			ri.m_index_count = submesh.m_index_count;
			ri.m_start_index_location = submesh.m_start_index;
			ri.m_base_vertex_location = submesh.m_base_vertex;
			m_render_items.emplace_back(std::move(ri));
			m_opaque_render_items.emplace_back(std::move(ri));
		}
	}
}
void Renderer::draw_render_items(ID3D12GraphicsCommandList* cmd_list, const std::vector<RenderItem>& r_items) { // rename to draw_game_objects
	size_t obj_cb_size = calc_constant_buffer_size(sizeof(ObjectConstants));
	auto object_cb = m_curr_frame_resource->m_object_cb->get_resource();
	for (size_t i = 0; i < r_items.size(); i++) { // have a different descriptor heap for each render item so need to set it that way
		ID3D12DescriptorHeap* descriptor_heaps[] = { m_descriptor_heap_map.at(r_items[i].m_mesh->m_mesh_id).Get() };
		m_command_list->SetDescriptorHeaps(_countof(descriptor_heaps), descriptor_heaps);
		m_command_list->SetGraphicsRootDescriptorTable(3, m_descriptor_heap_map.at(r_items[i].m_mesh->m_mesh_id)->GetGPUDescriptorHandleForHeapStart());

		cmd_list->IASetVertexBuffers(0, 1, &r_items[i].m_mesh->get_vertex_buffer_view());
		cmd_list->IASetIndexBuffer(&r_items[i].m_mesh->get_index_buffer_view());
		cmd_list->IASetPrimitiveTopology(r_items[i].m_primitive_type);
		D3D12_GPU_VIRTUAL_ADDRESS obj_cb_addr = object_cb->GetGPUVirtualAddress() + r_items[i].m_obj_cb_index * obj_cb_size;
		cmd_list->SetGraphicsRootConstantBufferView(0, obj_cb_addr);
		cmd_list->DrawIndexedInstanced(r_items[i].m_index_count, 1, r_items[i].m_start_index_location, r_items[i].m_base_vertex_location, 0);
	}
}
void Renderer::animate_materials(const Timer& t) {}
void Renderer::update_object_cbs(const Timer& t) {
	UploadBuffer<ObjectConstants>* curr_object_cb = m_curr_frame_resource->m_object_cb;
	for (auto& item : m_render_items) { // do for every item we are told to render
		// only need to update the item if the constants have changed
		if (item.m_num_frames_dirty > 0) {
			using namespace DirectX;
			XMMATRIX world = XMLoadFloat4x4(&item.m_world);
			XMMATRIX tex_transform = XMLoadFloat4x4(&item.m_tex_transform);
			ObjectConstants obj_consts;
			XMStoreFloat4x4(&obj_consts.m_world, XMMatrixTranspose(world));
			XMStoreFloat4x4(&obj_consts.m_tex_transform, XMMatrixTranspose(tex_transform));
			obj_consts.m_textures_used = item.m_textures_used; // should not change but the memcpy would overwrite it
			//obj_consts.m_material_index = item.m_material->m_mat_cb_index;
			curr_object_cb->copy_data(item.m_obj_cb_index, obj_consts);
			item.m_num_frames_dirty--;
		}
	}
}
void Renderer::update_material_buffer(const Timer& t) {
	// UploadBuffer<MaterialData>* curr_material_buffer = m_curr_frame_resource->m_material_buffer;
	// for (auto& item : m_materials) {
	// 	Material* mat = item.second;
	// 	if (mat->m_num_frames_dirty > 0) {
	// 		using namespace DirectX;
	// 		XMMATRIX mat_transform = XMLoadFloat4x4(&mat->m_mat_transform);
	// 		MaterialData mat_data;
	// 		mat_data.m_diffuse_albedo = mat->m_diffuse_albedo;
	// 		mat_data.m_fresnel_r0 = mat->m_fresnel_r0;
	// 		mat_data.m_roughness = mat->m_roughness;
	// 		XMStoreFloat4x4(&mat_data.m_mat_transform, XMMatrixTranspose(mat_transform));
	// 		mat_data.m_diffuse_map_index = mat->m_diffuse_srv_heap_index;
	// 		curr_material_buffer->copy_data(mat->m_mat_cb_index, mat_data);
	// 		mat->m_num_frames_dirty--;
	// 	}
	// }
}
void Renderer::update_main_pass_cb(const Timer& t) {
	using namespace DirectX;
	//engine->camera->update_view_matrix();
	XMMATRIX view = engine->camera->get_view();
	XMMATRIX proj = engine->camera->get_proj();
	XMMATRIX view_proj = XMMatrixMultiply(view, proj);
	XMMATRIX inv_view = XMMatrixInverse(&XMMatrixDeterminant(view), view);
	XMMATRIX inv_proj = XMMatrixInverse(&XMMatrixDeterminant(proj), proj);
	XMMATRIX inv_view_proj = XMMatrixInverse(&XMMatrixDeterminant(view_proj), view_proj);

	XMStoreFloat4x4(&m_main_pass_cb.m_view, XMMatrixTranspose(view));
	XMStoreFloat4x4(&m_main_pass_cb.m_inv_view, XMMatrixTranspose(inv_view));
	XMStoreFloat4x4(&m_main_pass_cb.m_proj, XMMatrixTranspose(proj));
	XMStoreFloat4x4(&m_main_pass_cb.m_inv_proj, XMMatrixTranspose(inv_proj));
	XMStoreFloat4x4(&m_main_pass_cb.m_view_proj, XMMatrixTranspose(view_proj));
	XMStoreFloat4x4(&m_main_pass_cb.m_inv_view_proj, XMMatrixTranspose(inv_view_proj));

	m_main_pass_cb.m_eye_pos_w = engine->camera->get_position3f();
	m_main_pass_cb.m_render_target_size = XMFLOAT2(engine->window->m_width, engine->window->m_height);
	m_main_pass_cb.m_inv_render_target_size = XMFLOAT2(1.0f/engine->window->m_width, 1.0f/engine->window->m_height);
	m_main_pass_cb.m_near_z = engine->camera->get_near_z();
	m_main_pass_cb.m_far_z = engine->camera->get_far_z();
	m_main_pass_cb.m_total_time = t.total_time();
	m_main_pass_cb.m_delta_time = t.delta_time();
	m_main_pass_cb.m_ambient_light = {0.1f, 0.1f, 0.11f, 1.0f};
	m_main_pass_cb.m_lights[0].m_direction = {0.57735f, -0.57735f, 0.57735f};
	m_main_pass_cb.m_lights[0].m_strength = { 0.8f, 0.8f, 0.8f };
	m_main_pass_cb.m_lights[1].m_direction = { -0.57735f, -0.57735f, 0.57735f };
	m_main_pass_cb.m_lights[1].m_strength = { 0.4f, 0.4f, 0.4f };
	m_main_pass_cb.m_lights[2].m_direction = { 0.0f, -0.707f, -0.707f };
	m_main_pass_cb.m_lights[2].m_strength = { 0.2f, 0.2f, 0.2f };

	// copy data to the gpu
	UploadBuffer<PassConstants>* curr_pass_cb = m_curr_frame_resource->m_pass_cb;
	curr_pass_cb->copy_data(0, m_main_pass_cb);
}
std::array<const CD3DX12_STATIC_SAMPLER_DESC, 6> Renderer::get_static_samplers() {
	const CD3DX12_STATIC_SAMPLER_DESC point_wrap(
		0,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	const CD3DX12_STATIC_SAMPLER_DESC point_clamp(
		1,
		D3D12_FILTER_MIN_MAG_MIP_POINT,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	const CD3DX12_STATIC_SAMPLER_DESC linear_wrap(
		2,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	const CD3DX12_STATIC_SAMPLER_DESC linear_clamp(
		3,
		D3D12_FILTER_MIN_MAG_MIP_LINEAR,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	const CD3DX12_STATIC_SAMPLER_DESC anisotropic_wrap(
		4,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
		D3D12_TEXTURE_ADDRESS_MODE_WRAP, 
		D3D12_TEXTURE_ADDRESS_MODE_WRAP);
	const CD3DX12_STATIC_SAMPLER_DESC anisotropic_clamp(
		5,
		D3D12_FILTER_ANISOTROPIC,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP,
		D3D12_TEXTURE_ADDRESS_MODE_CLAMP);
	return { point_wrap, point_clamp, linear_wrap, linear_clamp, anisotropic_wrap, anisotropic_clamp };
}
