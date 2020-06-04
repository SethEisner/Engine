#pragma once
#include <d3d12.h>
#include <DirectXMath.h>
#include "../Utilities/Utilities.h"
#include <windows.h>

static DirectX::XMFLOAT4X4 id(
    1.0f, 0.0f, 0.0f, 0.0f,
    0.0f, 1.0f, 0.0f, 0.0f,
    0.0f, 0.0f, 1.0f, 0.0f,
    0.0f, 0.0f, 0.0f, 1.0f);

inline DirectX::XMFLOAT4X4 identity_4x4()
{
    return id;
}

struct Vertex {
	DirectX::XMFLOAT3 m_pos;
	DirectX::XMFLOAT4 m_color;
};

struct ObjectConstants {
    DirectX::XMFLOAT4X4 m_world_view_proj = identity_4x4();
};

inline size_t calc_constant_buffer_size(size_t size) {
    // round up to nearest multiple of 256, and then mask off the remainder to get the exact multiple
    return (size + 255) & ~255;
}

template<typename T>
class UploadBuffer {
public:
    UploadBuffer(ID3D12Device* device, size_t count, bool is_constant_buffer) : m_is_cb(is_constant_buffer) {
        m_element_size = sizeof(T);
        // constant buffers must be multiples of the minimum hardware allocation size (usually 256 bytes) in size due to hardware limitations
        if (m_is_cb) {
            m_element_size = calc_constant_buffer_size(m_element_size);
        }
        ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD), 
                D3D12_HEAP_FLAG_NONE, 
                &CD3DX12_RESOURCE_DESC::Buffer(m_element_size * count), 
                D3D12_RESOURCE_STATE_GENERIC_READ, nullptr, 
                IID_PPV_ARGS(&m_upload_buffer)));
        ThrowIfFailed(m_upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mapped_data)));
    }
    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    ~UploadBuffer() {
        if (m_upload_buffer) {
            m_upload_buffer->Unmap(0, nullptr);
        }
        m_mapped_data = nullptr;
    }
    ID3D12Resource* get_resource() const {
        return m_upload_buffer.Get();
    }
    void copy_data(size_t index, const T& data) {
        memcpy(&m_mapped_data[index * m_element_size], &data, sizeof(T));
    }
private:
    Microsoft::WRL::ComPtr<ID3D12Resource> m_upload_buffer;
    uint8_t* m_mapped_data;
    size_t m_element_size;
    bool m_is_cb;
};