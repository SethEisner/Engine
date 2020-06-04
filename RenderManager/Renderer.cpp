#include "Renderer.h"
#include <assert.h>
#include <DirectXColors.h>
//#include <DirectXHelpers.h>
#include <d3d12sdklayers.h>
#include "../engine.h"

// LRESULT CALLBACK WindowProc(HWND hwnd, UINT uMsg, WPARAM wParam, LPARAM lParam) {
// 	switch (uMsg)
// 	{
// 	case WM_DESTROY:
// 		PostQuitMessage(0);
// 		exit(0);
// 	case WM_PAINT:
// 	{
// 		PAINTSTRUCT ps;
// 		HDC hdc = BeginPaint(hwnd, &ps);
// 
// 		FillRect(hdc, &ps.rcPaint, (HBRUSH)(COLOR_WINDOW + 1));
// 
// 		EndPaint(hwnd, &ps);
// 	}
// 	return 0;
// 
// 	}
// 	return DefWindowProc(hwnd, uMsg, wParam, lParam);
// }

//Window* Renderer::get_window() const {
//	return m_window;
//}

Renderer::Renderer(Window* window) : m_window(window) {

}
Renderer::~Renderer() {

}
bool Renderer::init() {
	//assert(m_window->init()); // if we cant create the window then we can't render to the window
	//init_window();
	//if (!m_window->init()) return false;

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
	build_descriptor_heaps();
	build_constant_buffers();
	build_root_signature();
	build_shaders_and_input_layout();
	build_box_geometry();
	build_pso();
	ThrowIfFailed(m_command_list->Close()); // execute the initialization commands
	ID3D12CommandList* cmd_lists[] = {m_command_list.Get()};
	m_command_queue->ExecuteCommandLists(_countof(cmd_lists), cmd_lists);
	flush_command_queue();


	return true;
}


void Renderer::update() {
	// build the view matrix
	float x = m_radius * sinf(m_phi) * cosf(m_theta);
	float y = m_radius * sinf(m_phi) * sinf(m_theta);
	float z = m_radius * cosf(m_phi);
	using namespace DirectX;
	XMVECTOR pos = XMVectorSet(x, y, z, 1.0f);
	XMVECTOR target = XMVectorZero();
	XMVECTOR up = XMVectorSet(0.0f, 1.0f, 0.0f, 0.0f);
	XMMATRIX view = XMMatrixLookAtLH(pos, target, up);
	XMStoreFloat4x4(&m_view, view);

	XMMATRIX world = XMLoadFloat4x4(&m_world);
	XMMATRIX proj = XMLoadFloat4x4(&m_proj);
	XMMATRIX world_view_proj = world * view * proj;
	ObjectConstants obj_consts;
	XMStoreFloat4x4(&obj_consts.m_world_view_proj, XMMatrixTranspose(world_view_proj));
	m_object_cb->copy_data(0, obj_consts);
}
void Renderer::draw() {
	ThrowIfFailed(m_command_list_allocator->Reset()); // reset the allocator to accept new commands (must be done after GPU is done using the allocator
	// reset command list after it has been added to the command queue
	ThrowIfFailed(m_command_list->Reset(m_command_list_allocator.Get(), nullptr));
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
	ID3D12DescriptorHeap* descriptor_heaps[] = { m_cbv_heap.Get() };
	m_command_list->SetDescriptorHeaps(_countof(descriptor_heaps), descriptor_heaps);
	m_command_list->SetGraphicsRootSignature(m_root_signature.Get());
	m_command_list->IASetVertexBuffers(0, 1, &m_box_geo->get_vertex_buffer_view());
	m_command_list->IASetIndexBuffer(&m_box_geo->get_index_buffer_view());
	m_command_list->IASetPrimitiveTopology(D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST);
	m_command_list->SetGraphicsRootDescriptorTable(0, m_cbv_heap->GetGPUDescriptorHandleForHeapStart());
	m_command_list->SetPipelineState(m_pso.Get());
	m_command_list->DrawIndexedInstanced(m_box_geo->m_draw_args[m_box_geo->hash_name].m_index_count, 1, 0, 0, 0);


	// indicate a state transition on the resource usage
	m_command_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(current_back_buffer(), D3D12_RESOURCE_STATE_RENDER_TARGET, D3D12_RESOURCE_STATE_PRESENT));
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
	ThrowIfFailed(m_swap_chain->ResizeBuffers(SWAP_CHAIN_BUFFER_COUNT, m_window->m_width, //m_window->m_width
	m_window->m_height, //m_window->m_height
	m_back_buffer_format, DXGI_SWAP_CHAIN_FLAG_ALLOW_MODE_SWITCH));
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
	depth_stencil_desc.Width = m_window->m_width;// m_window->m_width;
	depth_stencil_desc.Height = m_window->m_height;// m_window->m_height; //m_window->m_height;
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
	m_screen_viewport.Width = static_cast<float>(m_window->m_width); //m_window->m_width);
	m_screen_viewport.Height = static_cast<float>(m_window->m_height); //m_window->m_height);
	m_screen_viewport.MinDepth = 0.0f;
	m_screen_viewport.MaxDepth = 1.0f;
	// m_scissor_rect = { 0, 0, static_cast<float>(m_window->get_window_width()), static_cast<float>(m_window->get_window_height()) };
	m_scissor_rect = { 0, 0, static_cast<LONG>(m_window->m_width), //m_window->m_width), 
		static_cast<LONG>(m_window->m_height) };//m_window->m_height) };


	// chapter 6
	using namespace DirectX;
	XMMATRIX p = XMMatrixPerspectiveFovLH(0.25f * pi, m_window->get_aspect_ratio(), 1.0f, 1000.0f);
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
	s.Width = m_window->m_width; //m_window->m_width;
	s.Height = m_window->m_height; //m_window->m_height;
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
	D3D12_DESCRIPTOR_HEAP_DESC cbv_heap_desc;
	cbv_heap_desc.NumDescriptors = 1;
	cbv_heap_desc.Type = D3D12_DESCRIPTOR_HEAP_TYPE_CBV_SRV_UAV;
	cbv_heap_desc.Flags = D3D12_DESCRIPTOR_HEAP_FLAG_SHADER_VISIBLE;
	cbv_heap_desc.NodeMask = 0;
	ThrowIfFailed(m_d3d_device->CreateDescriptorHeap(&cbv_heap_desc, IID_PPV_ARGS(&m_cbv_heap)));
}
void Renderer::build_constant_buffers() {
	m_object_cb = new UploadBuffer<ObjectConstants>(m_d3d_device.Get(), 1, true);
	uint32_t cb_size = calc_constant_buffer_size(sizeof(ObjectConstants));
	D3D12_GPU_VIRTUAL_ADDRESS cb_addr = m_object_cb->get_resource()->GetGPUVirtualAddress();
	// offset to the ith object constant buffer in the buffer
	size_t cb_index = 0;
	cb_addr += cb_index * cb_size;
	D3D12_CONSTANT_BUFFER_VIEW_DESC cbv_desc;
	cbv_desc.BufferLocation = cb_addr;
	cbv_desc.SizeInBytes = cb_size;

	m_d3d_device->CreateConstantBufferView(&cbv_desc, m_cbv_heap->GetCPUDescriptorHandleForHeapStart());
}
void Renderer::build_root_signature() {
	// the root signature defines the resources the shader programs expect
	// root signature is analagous to a function signature
	// root parameter can be a table, root descriptor, or root constants
	CD3DX12_ROOT_PARAMETER slot_root_parameter[1];
	// create a single descriptor table of CBVs (will be more sophisticated later)
	CD3DX12_DESCRIPTOR_RANGE cbv_table;
	cbv_table.Init(D3D12_DESCRIPTOR_RANGE_TYPE_CBV, 1, 0);
	slot_root_parameter[0].InitAsDescriptorTable(1, &cbv_table);
	// a root signature is an array of root parameters
	CD3DX12_ROOT_SIGNATURE_DESC root_sig_desc(1, slot_root_parameter, 0, nullptr, D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
	// create a root signature with a single slot that points to a descriptor range consisting of a single constant buffer
	Microsoft::WRL::ComPtr<ID3DBlob> serialized_root_sig = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> error_blob = nullptr;
	HRESULT hr = D3D12SerializeRootSignature(&root_sig_desc, D3D_ROOT_SIGNATURE_VERSION_1, serialized_root_sig.GetAddressOf(), error_blob.GetAddressOf());
	if (error_blob) {
		OutputDebugStringA((char*)error_blob->GetBufferPointer());
	}
	ThrowIfFailed(hr);
	// create the root signaure
	ThrowIfFailed(m_d3d_device->CreateRootSignature(0, serialized_root_sig->GetBufferPointer(), serialized_root_sig->GetBufferSize(), IID_PPV_ARGS(&m_root_signature)));
}
void Renderer::build_shaders_and_input_layout() {
	HRESULT hr = S_OK;
	m_vs_bytecode = compile_shader(L"C:\\Users\\Seth Eisner\\source\\repos\\Engine\\Shaders\\color.hlsl", nullptr, "VS", "vs_5_0");
	m_ps_bytecode = compile_shader(L"C:\\Users\\Seth Eisner\\source\\repos\\Engine\\Shaders\\color.hlsl", nullptr, "PS", "ps_5_0");
	m_input_layout = {
		{"POSITION", 0, DXGI_FORMAT_R32G32B32_FLOAT, 0, 0, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0},
		{"COLOR", 0, DXGI_FORMAT_R32G32B32A32_FLOAT, 0, 12, D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0}
	};
}
void Renderer::build_box_geometry() {
	using namespace DirectX;
	std::array<Vertex, 8> vertices = {
		Vertex({ XMFLOAT3(-1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::White) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Black) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, -1.0f), XMFLOAT4(Colors::Red) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, -1.0f), XMFLOAT4(Colors::Green) }),
		Vertex({ XMFLOAT3(-1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Blue) }),
		Vertex({ XMFLOAT3(-1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Yellow) }),
		Vertex({ XMFLOAT3(+1.0f, +1.0f, +1.0f), XMFLOAT4(Colors::Cyan) }),
		Vertex({ XMFLOAT3(+1.0f, -1.0f, +1.0f), XMFLOAT4(Colors::Magenta) })
	};
	std::array<std::uint16_t, 36> indices = {
		// front face
		0, 1, 2,
		0, 2, 3,
		// back face
		4, 6, 5,
		4, 7, 6,
		// left face
		4, 5, 1,
		4, 1, 0,
		// right face
		3, 2, 6,
		3, 6, 7,
		// top face
		1, 5, 6,
		1, 6, 2,
		// bottom face
		4, 0, 3,
		4, 3, 7
	};
	const uint32_t vb_size = static_cast<uint32_t>(vertices.size() * sizeof(Vertex));
	const uint32_t ib_size = static_cast<uint32_t>(indices.size() * sizeof(uint16_t));
	m_box_geo = new Mesh();
	m_box_geo->hash_name = std::hash<std::string>{}("box_geo");
	ThrowIfFailed(D3DCreateBlob(vb_size, &m_box_geo->m_vertex_buffer_cpu));
	CopyMemory(m_box_geo->m_vertex_buffer_cpu->GetBufferPointer(), vertices.data(), vb_size);
	ThrowIfFailed(D3DCreateBlob(ib_size, &m_box_geo->m_index_buffer_cpu));
	CopyMemory(m_box_geo->m_index_buffer_cpu->GetBufferPointer(), indices.data(), ib_size);
	m_box_geo->m_vertex_buffer_gpu = create_default_buffer(m_d3d_device.Get(), m_command_list.Get(), vertices.data(), vb_size, m_box_geo->m_vertex_buffer_uploader);
	m_box_geo->m_index_buffer_gpu = create_default_buffer(m_d3d_device.Get(), m_command_list.Get(), indices.data(), ib_size, m_box_geo->m_index_buffer_uploader);

	m_box_geo->m_vertex_stride = sizeof(Vertex);
	m_box_geo->m_vertex_buffer_size = vb_size;
	m_box_geo->m_index_format = DXGI_FORMAT_R16_UINT;
	m_box_geo->m_index_buffer_size = ib_size;

	// only obne mesh now so set indecis into mesh structure buffers to be 0
	SubMesh submesh;
	submesh.m_index_count = indices.size();
	submesh.m_start_index = 0;
	submesh.m_base_vertex = 0;

	m_box_geo->m_draw_args[m_box_geo->hash_name] = submesh;
}

void Renderer::build_pso() {
	D3D12_GRAPHICS_PIPELINE_STATE_DESC pso_desc;
	ZeroMemory(&pso_desc, sizeof(D3D12_GRAPHICS_PIPELINE_STATE_DESC));
	pso_desc.InputLayout = { m_input_layout.data(), (uint32_t)m_input_layout.size() };
	pso_desc.pRootSignature = m_root_signature.Get();
	pso_desc.VS = {
		reinterpret_cast<BYTE*>(m_vs_bytecode->GetBufferPointer()),
		m_vs_bytecode->GetBufferSize()
	};
	pso_desc.PS = {
		reinterpret_cast<BYTE*>(m_ps_bytecode->GetBufferPointer()),
		m_ps_bytecode->GetBufferSize()
	};
	pso_desc.RasterizerState = CD3DX12_RASTERIZER_DESC(D3D12_DEFAULT);
	pso_desc.BlendState = CD3DX12_BLEND_DESC(D3D12_DEFAULT);
	pso_desc.DepthStencilState = CD3DX12_DEPTH_STENCIL_DESC(D3D12_DEFAULT);
	pso_desc.SampleMask = UINT_MAX;
	pso_desc.PrimitiveTopologyType = D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE;
	pso_desc.NumRenderTargets = 1;
	pso_desc.RTVFormats[0] = m_back_buffer_format;
	pso_desc.SampleDesc.Count = 1;
	pso_desc.SampleDesc.Quality = 0;
	pso_desc.DSVFormat = m_depth_stencil_format;
	ThrowIfFailed(m_d3d_device->CreateGraphicsPipelineState(&pso_desc, IID_PPV_ARGS(&m_pso)));
}