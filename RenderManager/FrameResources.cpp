#include "FrameResources.h"

// should be okay to use an initializer list
FrameResources::FrameResources(ID3D12Device* device, uint32_t pass_count, uint32_t object_count, uint32_t material_count) :
	m_pass_cb(new UploadBuffer<PassConstants>(device, pass_count, true)),
	m_object_cb(new UploadBuffer<ObjectConstants>(device, object_count, true))
	//m_material_buffer(new UploadBuffer<MaterialData>(device, material_count, false)) 
{ 
	ThrowIfFailed(device->CreateCommandAllocator(D3D12_COMMAND_LIST_TYPE_DIRECT, IID_PPV_ARGS(m_cmd_list_allocator.GetAddressOf())));
}
FrameResources::~FrameResources() {
	delete m_material_buffer;
	delete m_object_cb;
	delete m_pass_cb;
}