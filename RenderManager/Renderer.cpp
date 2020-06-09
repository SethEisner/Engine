#include "Renderer.h"
#include <assert.h>
#include <DirectXColors.h>
#include <DirectXMath.h>
#include "FrameResources.h"
#include "Scene.h"
#include "../Engine.h"

bool Renderer::init() {

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


	// chapter 6 specific items
	ThrowIfFailed(m_command_list->Reset(m_command_list_allocator.Get(), nullptr));
	engine->camera->set_position(0.0f, 2.0f, -200.0f);
	load_textures(); // separate loading from the renderer to use the resource manager
	build_root_signature();
	build_descriptor_heaps();
	build_shaders_and_input_layout();
	//build_constant_buffers();
	build_shape_geometry();
	build_materials();
	build_render_items();
	build_frame_resources();
	build_psos();
	ThrowIfFailed(m_command_list->Close()); // execute the initialization commands
	ID3D12CommandList* cmd_lists[] = { m_command_list.Get() };
	m_command_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists);
	flush_command_queue();
	return true;
}


void Renderer::update() {

	m_curr_frame_resources_index = (m_curr_frame_resources_index + 1) % g_num_frame_resources;
	m_curr_frame_resource = m_frame_resources[m_curr_frame_resources_index];

	// GPU has not finished using this frame resource so we must wait until the GPU is done
	if (m_curr_frame_resource->m_fence != 0 && m_fence->GetCompletedValue() < m_curr_frame_resource->m_fence) {
		HANDLE event_handle = CreateEventEx(nullptr, NULL, false, EVENT_ALL_ACCESS);
		ThrowIfFailed(m_fence->SetEventOnCompletion(m_curr_frame_resource->m_fence, event_handle));
		WaitForSingleObject(event_handle, INFINITE);
		CloseHandle(event_handle);
	}
	animate_materials(*engine->global_timer);
	update_object_cbs(*engine->global_timer);
	update_material_buffer(*engine->global_timer);
	update_main_pass_cb(*engine->global_timer);
}
void Renderer::draw() {
	auto command_list_allocator = m_curr_frame_resource->m_cmd_list_allocator;
	ThrowIfFailed(command_list_allocator->Reset()); // reset the allocator to accept new commands (must be done after GPU is done using the allocator
	// reset command list after it has been added to the command queue
	ThrowIfFailed(m_command_list->Reset(command_list_allocator.Get(), m_psos["opaque"].Get()));


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

	// chapter 6
	ID3D12DescriptorHeap* descriptor_heaps[] = { m_srv_descriptor_heap.Get() };
	m_command_list->SetDescriptorHeaps(_countof(descriptor_heaps), descriptor_heaps);
	m_command_list->SetGraphicsRootSignature(m_root_signature.Get());
	auto pass_cb = m_curr_frame_resource->m_pass_cb->get_resource();
	m_command_list->SetGraphicsRootConstantBufferView(1, pass_cb->GetGPUVirtualAddress());
	auto mat_buffer = m_curr_frame_resource->m_material_buffer->get_resource();
	m_command_list->SetGraphicsRootShaderResourceView(2, mat_buffer->GetGPUVirtualAddress());
	m_command_list->SetGraphicsRootDescriptorTable(3, m_srv_descriptor_heap->GetGPUDescriptorHandleForHeapStart());
	draw_render_items(m_command_list.Get(), m_opaque_render_items);
	m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(current_back_buffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
	ThrowIfFailed(m_command_list->Close());


	ID3D12CommandList* cmd_lists[] = { m_command_list.Get() };
	m_command_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists);
	// swap the back and front buffers
	ThrowIfFailed(m_swap_chain->Present(0, 0));
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
	ThrowIfFailed(m_command_list->Reset(m_command_list_allocator.Get(), nullptr));
	// release resources we will be recreating
	for (size_t i = 0; i != SWAP_CHAIN_BUFFER_COUNT; ++i) {
		m_swap_chain_buffer[i].Reset();
	}
	m_depth_stencil_buffer.Reset();
	// resize the swap chain
	ThrowIfFailed(m_swap_chain->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, engine->window->m_width, engine->window->m_height, m_back_buffer_format, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
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
	m_screen_viewport.Width = static_cast<float>(engine->window->m_width); //m_window->m_width);
	m_screen_viewport.Height = static_cast<float>(engine->window->m_height); //m_window->m_height);
	m_screen_viewport.MinDepth = 0.0f;
	m_screen_viewport.MaxDepth = 1.0f;
	// m_scissor_rect = { 0, 0, static_cast<float>(m_window->get_window_width()), static_cast<float>(m_window->get_window_height()) };
	m_scissor_rect = { 0, 0, static_cast<LONG>(engine->window->m_width), //m_window->m_width), 
		static_cast<LONG>(engine->window->m_height) };//m_window->m_height) };


	// chapter 6
	using namespace DirectX;
	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * pi, engine->window->get_aspect_ratio(), 1.0f, 1000.0f);
	XMStoreFloat4x4(&m_proj, p);
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
	s.Width = engine->window->m_width; //m_window->m_width;
	s.Height = engine->window->m_height; //m_window->m_height;
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

	ThrowIfFailed(m_dxgi_factory->CreateSwapChainForHwnd(m_command_queue.Get(), engine->window->m_handle, &s, NULL, NULL, m_swap_chain.GetAddressOf()));
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
void Renderer::build_descriptor_heaps() {
	/* no texutres to deal with yet
	D3D12_DESCRIPTOR_HEAP_DESC srv_heap_desc = {};
	srv_heap_desc.NumDescriptors = 4;
	srv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	srv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	//srv_heap_desc.NodeMask = 0;
	ThrowIfFailed(m_d3d_device->CreateDescriptorHeap(&srv_heap_desc, IID_PPV_ARGS(&m_srv_descriptor_heap)));

	// fill out the heap with our 4 descriptors
	CD3DX12_CPU_DESCRIPTOR_HANDLE h_desc(m_srv_descriptor_heap->GetCPUDescriptorHandleForHeapStart());
	auto bricks_tex = m_textures["bricks_tex"]->m_resource;
	auto stone_tex = m_textures["stone_tex"]->m_resource;
	auto tile_tex = m_textures["tile_tex"]->m_resource;
	auto crate_tex = m_textures["create_tex"]->m_resource;

	D3D12_SHADER_RESOURCE_VIEW_DESC srv_desc = {};
	srv_desc.Shader4ComponentMapping = D3D12_DEFAULT_SHADER_4_COMPONENT_MAPPING;
	srv_desc.Format = bricks_tex->GetDesc().Format;
	srv_desc.ViewDimension = D3D12_SRV_DIMENSION_TEXTURE2D;
	srv_desc.Texture2D.MostDetailedMip = 0;
	srv_desc.Texture2D.MipLevels = bricks_tex->GetDesc().MipLevels;
	srv_desc.Texture2D.ResourceMinLODClamp = 0.0f;
	m_d3d_device->CreateShaderResourceView(bricks_tex.Get(), &srv_desc, h_desc);

	h_desc.Offset(1, m_cbv_srv_descriptor_size);
	srv_desc.Format = stone_tex->GetDesc().Format;
	srv_desc.Texture2D.MipLevels = stone_tex->GetDesc().MipLevels;
	m_d3d_device->CreateShaderResourceView(stone_tex.Get(), &srv_desc, h_desc);

	h_desc.Offset(1, m_cbv_srv_descriptor_size);
	srv_desc.Format = tile_tex->GetDesc().Format;
	srv_desc.Texture2D.MipLevels = tile_tex->GetDesc().MipLevels;
	m_d3d_device->CreateShaderResourceView(tile_tex.Get(), &srv_desc, h_desc);

	h_desc.Offset(1, m_cbv_srv_descriptor_size);
	srv_desc.Format = crate_tex->GetDesc().Format;
	srv_desc.Texture2D.MipLevels = crate_tex->GetDesc().MipLevels;
	m_d3d_device->CreateShaderResourceView(crate_tex.Get(), &srv_desc, h_desc);
	*/
}
// void Renderer::build_constant_buffers() {
// 	m_object_cb = new UploadBuffer<ObjectConstants>(m_d3d_device.Get(), 1, true);
// 	uint32_t cb_size = calc_constant_buffer_size(sizeof(ObjectConstants));
// 	D3D12_GPU_VIRTUAL_ADDRESS cb_addr = m_object_cb->get_resource()->GetGPUVirtualAddress();
// 	// offset to the ith object constant buffer in the buffer
// 	size_t cb_index = 0;
// 	cb_addr += cb_index * cb_size;
// 	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
// 	cbv_desc.BufferLocation = cb_addr;
// 	cbv_desc.SizeInBytes = cb_size;
// 
// 	m_d3d_device->CreateConstantBufferView(&cbv_desc, m_cbv_heap->GetCPUDescriptorHandleForHeapStart());
// }
void Renderer::build_root_signature() {
	CD3DX12_DESCRIPTOR_RANGE tex_table;
	tex_table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_SRV, 4, 0, 0);
	CD3DX12_ROOT_PARAMETER slot_root_parameter[4];
	slot_root_parameter[0].InitAsConstantBufferView(0);
	slot_root_parameter[1].InitAsConstantBufferView(1);
	slot_root_parameter[2].InitAsShaderResourceView(0, 1);
	slot_root_parameter[3].InitAsDescriptorTable(1, &tex_table, D3D12_SHADER_VISIBILITY_PIXEL);
	
	// array of 6 CD3DX12_STATIC_SAMPLER_DESC
	auto static_samplers = get_static_samplers();
	// the root signature is an array of root parameters, in our case 4 root parameters
	CD3DX12_ROOT_SIGNATURE_DESC root_sig_desc(4, slot_root_parameter, static_samplers.size(), static_samplers.data(), D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	// create a root signature with a single slot that points to a descriptor range consisting of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serialized_root_sig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> error_blob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1, serialized_root_sig.GetAddressOf(), error_blob.GetAddressOf());
	if (error_blob) {
		OutputDebugStringA((char*)error_blob->GetBufferPointer());
	}
	ThrowIfFailed(hr);
	ThrowIfFailed(m_d3d_device->CreateRootSignature(0, serialized_root_sig->GetBufferPointer(), serialized_root_sig->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));
}
void Renderer::build_shaders_and_input_layout() {
	const D3D_SHADER_MACRO alpha_test_defines[] = {
		"ALPHA_TEST", "1", NULL, NULL
	};
	HRESULT hr = S_OK;
	// m_vs_bytecode = compile_shader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	// m_ps_bytecode = compile_shader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");
	// m_shaders["standard_vs"] = compile_shader(L"Shaders\\default.hlsl", nullptr, "VS", "vs_5_1");
	// m_shaders["opaque_ps"] = compile_shader(L"Shaders\\default.hlsl", nullptr, "PS", "ps_5_1");

	m_shaders["standard_vs"] = compile_shader(L"Shaders\\color.hlsl", nullptr, "VS", "vs_5_1");
	m_shaders["opaque_ps"] = compile_shader(L"Shaders\\color.hlsl", nullptr, "PS", "ps_5_1");
	m_input_layout = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		//{"TEXCOORD", 0, DXGI_FORMAT_R32G32_FLOAT, 0, 24, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}
void Renderer::build_shape_geometry() {
	// MeshData for the scene is in Engine->scene 

	// use the MeshData structure to build the Mesh and Submesh structures
	uint32_t vertex_offset = 0;
	uint32_t index_offset = 0;

	SubMesh cat;
	cat.m_index_count = engine->scene->m_mesh->m_indecis.size();
	cat.m_start_index = index_offset;
	cat.m_base_vertex = vertex_offset;


	// for each mesh, we copy the data to an vertices and indices buffer. for now we only have one mesh so we only copy one buffer 
	size_t vertex_count = engine->scene->m_mesh->m_vertices.size();
	size_t index_count = engine->scene->m_mesh->m_indecis.size();
	std::vector<Vertex> vertices(vertex_count); // frameresources Vertex type
	std::vector<uint16_t> indices(index_count);
	size_t vertices_index = 0;
	memcpy(vertices.data(), engine->scene->m_mesh->m_vertices.data(), vertex_count);
	memcpy(indices.data(), engine->scene->m_mesh->m_indecis.data(), index_count);

	const size_t vb_byte_size = vertices.size() * sizeof(Vertex);
	const size_t ib_byte_size = indices.size() * sizeof(uint16_t);

	// create a Mesh that holds the SubMesh and buffers we just created
	Mesh* geo = new Mesh();
	geo->name = "cat_geo";
	ThrowIfFailed(D3DCreateBlob(vb_byte_size, &geo->m_vertex_buffer_cpu));
	CopyMemory(geo->m_vertex_buffer_cpu->GetBufferPointer(), vertices.data(), vb_byte_size);
	ThrowIfFailed(D3DCreateBlob(ib_byte_size, &geo->m_index_buffer_cpu));
	CopyMemory(geo->m_index_buffer_cpu->GetBufferPointer(), indices.data(), ib_byte_size);

	geo->m_vertex_buffer_gpu = create_default_buffer(m_d3d_device.Get(), m_command_list.Get(), vertices.data(), vb_byte_size, geo->m_vertex_buffer_uploader);
	geo->m_index_buffer_gpu = create_default_buffer(m_d3d_device.Get(), m_command_list.Get(), indices.data(), ib_byte_size, geo->m_index_buffer_uploader);

	geo->m_vertex_stride = sizeof(Vertex);
	geo->m_vertex_buffer_size = vb_byte_size;
	geo->m_index_format = DXGI_FORMAT_R16_UINT;
	geo->m_index_buffer_size = ib_byte_size;

	geo->m_draw_args["cat"] = cat;

	// read and build from data that should be in ResourceManager
	// using namespace DirectX;
	// std::array<Vertex, 8> vertices = {
	// 	Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::White) }),
	// 	Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Black) }),
	// 	Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Red) }),
	// 	Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, -1.0f), DirectX::XMFLOAT4(DirectX::Colors::Green) }),
	// 	Vertex({ DirectX::XMFLOAT3(-1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Blue) }),
	// 	Vertex({ DirectX::XMFLOAT3(-1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Yellow) }),
	// 	Vertex({ DirectX::XMFLOAT3(+1.0f, +1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Cyan) }),
	// 	Vertex({ DirectX::XMFLOAT3(+1.0f, -1.0f, +1.0f), DirectX::XMFLOAT4(DirectX::Colors::Magenta) })
	// };
	// std::array<std::uint16_t, 36> indices = {
	// 	// front face
	// 	0, 1, 2,
	// 	0, 2, 3,
	// 	// back face
	// 	4, 6, 5,
	// 	4, 7, 6,
	// 	// left face
	// 	4, 5, 1,
	// 	4, 1, 0,
	// 	// right face
	// 	3, 2, 6,
	// 	3, 6, 7,
	// 	// top face
	// 	1, 5, 6,
	// 	1, 6, 2,
	// 	// bottom face
	// 	4, 0, 3,
	// 	4, 3, 7
	// };
	// const uint32_t vb_size = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
	// const uint32_t ib_size = static_cast<uint32_t>(indices.size() * sizeof(uint16_t));
	// m_box_geo = new Mesh();
	// m_box_geo->hash_name = std::hash<std::string>{}("box_geo");
	// ThrowIfFailed(D3DCreateBlob(vb_size, &m_box_geo->m_vertex_buffer_cpu));
	// CopyMemory(m_box_geo->m_vertex_buffer_cpu->GetBufferPointer(), vertices.data(), vb_size);
	// ThrowIfFailed(D3DCreateBlob(ib_size, &m_box_geo->m_index_buffer_cpu));
	// CopyMemory(m_box_geo->m_index_buffer_cpu->GetBufferPointer(), indices.data(), ib_size);
	// m_box_geo->m_vertex_buffer_gpu = create_default_buffer(m_d3d_device.Get(), m_command_list.Get(), vertices.data(), vb_size, m_box_geo->m_vertex_buffer_uploader);
	// m_box_geo->m_index_buffer_gpu = create_default_buffer(m_d3d_device.Get(), m_command_list.Get(), indices.data(), ib_size, m_box_geo->m_index_buffer_uploader);
	// 
	// m_box_geo->m_vertex_stride = sizeof(Vertex);
	// m_box_geo->m_vertex_buffer_size = vb_size;
	// m_box_geo->m_index_format = DXGI_FORMAT_R16_UINT;
	// m_box_geo->m_index_buffer_size = ib_size;
	// 
	// // only obne mesh now so set indecis into mesh structure buffers to be 0
	// SubMesh submesh;
	// submesh.m_index_count = indices.size();
	// submesh.m_start_index = 0;
	// submesh.m_base_vertex = 0;
	// 
	// m_box_geo->m_draw_args[m_box_geo->hash_name] = submesh;
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
		reinterpret_cast<BYTE*>(m_shaders["opaque_vs"]->GetBufferPointer()),
		m_shaders["opaque_vs"]->GetBufferSize()
	};
	opaque_pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	opaque_pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	opaque_pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	opaque_pso_desc.SampleMask = UINT_MAX;
	opaque_pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	opaque_pso_desc.NumRenderTargets = 1;
	opaque_pso_desc.RTVFormats[0] = m_back_buffer_format;
	opaque_pso_desc.SampleDesc.Count = 1;
	opaque_pso_desc.SampleDesc.Quality = 0;
	opaque_pso_desc.DSVFormat = m_depth_stencil_format;
	ThrowIfFailed(m_d3d_device->CreateGraphicsPipelineState(&opaque_pso_desc, IID_PPV_ARGS(&m_psos["opaque"])));
}
void Renderer::build_frame_resources() {
	for (int i = 0; i < g_num_frame_resources; ++i) { // okay to initialize in a loop because g_num_frame_resources should be around 2 or 3
		FrameResources* temp = new FrameResources(m_d3d_device.Get(), 1, m_render_items.size(), m_materials.size());
		m_frame_resources.push_back(temp);
	}
}
void Renderer::build_materials() {}
void Renderer::build_render_items() {
	RenderItem* cat = new RenderItem();
	XMStoreFloat4x4(&cat->m_world, DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f) * DirectX::XMMatrixTranslation(0.0f, 1.0f, 0.0f));
	XMStoreFloat4x4(&cat->m_tex_transform, DirectX::XMMatrixScaling(1.0f, 1.0f, 1.0f));
	cat->m_obj_cb_index = 0;
	cat->m_mesh = m_geometries["cat_geo"].ge;
}
void Renderer::draw_render_items(ID3D12GraphicsCommandList* cmd_list, const std::vector<RenderItem*>& r_items) { // rename to draw_game_objects
	size_t obj_cb_size = calc_constant_buffer_size(sizeof(ObjectConstants));
	auto object_cb = m_curr_frame_resource->m_object_cb->get_resource();
	for (size_t i = 0; i < r_items.size(); i++) {
		auto ri = r_items[i];
		cmd_list->IASetVertexBuffers(0, 1, &ri->m_mesh->get_vertex_buffer_view());
		cmd_list->IASetIndexBuffer(&ri->m_mesh->get_index_buffer_view());
		cmd_list->IASetPrimitiveTopology(ri->m_primitive_type);
		D3D12_GPU_VIRTUAL_ADDRESS obj_cb_addr = object_cb->GetGPUVirtualAddress() + ri->m_obj_cb_index * obj_cb_size;
		cmd_list->SetGraphicsRootConstantBufferView(0, obj_cb_addr);
		cmd_list->DrawIndexedInstanced(ri->m_index_count, 1, ri->m_start_index_location, ri->m_base_vertex_location, 0);
	}
}
void Renderer::animate_materials(const Timer& t) {}
void Renderer::update_object_cbs(const Timer& t) {
	UploadBuffer<ObjectConstants>* curr_object_cb = m_curr_frame_resource->m_object_cb;
	for (auto& item : m_render_items) { // do for every item we are told to render
		// only need to update the item if the constants have changed
		if (item->m_num_frames_dirty > 0) {
			using namespace DirectX;
			XMMATRIX world = XMLoadFloat4x4(&item->m_world);
			XMMATRIX tex_transform = XMLoadFloat4x4(&item->m_tex_transform);
			ObjectConstants obj_consts;
			XMStoreFloat4x4(&obj_consts.m_world, XMMatrixTranspose(world));
			XMStoreFloat4x4(&obj_consts.m_tex_transform, XMMatrixTranspose(tex_transform));
			obj_consts.m_material_index = item->m_material->m_mat_cb_index;
			curr_object_cb->copy_data(item->m_obj_cb_index, obj_consts);
			item->m_num_frames_dirty--;
		}
	}
}
void Renderer::update_material_buffer(const Timer& t) {
	UploadBuffer<MaterialData>* curr_material_buffer = m_curr_frame_resource->m_material_buffer;
	for (auto& item : m_materials) {
		Material* mat = item.second;
		if (mat->m_num_frames_dirty > 0) {
			using namespace DirectX;
			XMMATRIX mat_transform = XMLoadFloat4x4(&mat->m_mat_transform);
			MaterialData mat_data;
			mat_data.m_diffuse_albedo = mat->m_diffuse_albedo;
			mat_data.m_fresnel_r0 = mat->m_fresnel_r0;
			mat_data.m_roughness = mat->m_roughness;
			XMStoreFloat4x4(&mat_data.m_mat_transform, XMMatrixTranspose(mat_transform));
			mat_data.m_diffuse_map_index = mat->m_diffuse_srv_heap_index;
			curr_material_buffer->copy_data(mat->m_mat_cb_index, mat_data);
			mat->m_num_frames_dirty--;
		}
	}
}
void Renderer::update_main_pass_cb(const Timer& t) {
	using namespace DirectX;
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
	m_main_pass_cb.m_ambient_light = {0.25f, 0.25f, 0.35f, 1.0f};
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
void Renderer::load_textures() {
	// call resource manager
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
