#pragma once
#include "d3dx12.h"
#include <d3d12.h>
#include "d3dutil.h"

template<typename T>
class UploadBuffer {
public:
    UploadBuffer(ID3D12Device* device, size_t count, bool is_constant_buffer) : m_is_cb(is_constant_buffer) {
        m_element_size = sizeof(T);
        // constant buffers must be multiples of the minimum hardware allocation size (usually 256 bytes) in size due to hardware limitations
        if (m_is_cb) {
            m_element_size = calc_constant_buffer_size(m_element_size);
        }
        device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(m_element_size * count),
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&m_upload_buffer));
        /*
        ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
            D3D12_HEAP_FLAG_NONE,
            &CD3DX12_RESOURCE_DESC::Buffer(m_element_size * count),
            D3D12_RESOURCE_STATE_GENERIC_READ, nullptr,
            IID_PPV_ARGS(&m_upload_buffer))); */
        m_upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mapped_data)); // ThrowIfFailed(m_upload_buffer->Map(0, nullptr, reinterpret_cast<void**>(&m_mapped_data)));
    }
    UploadBuffer(const UploadBuffer& rhs) = delete;
    UploadBuffer& operator=(const UploadBuffer& rhs) = delete;
    ~UploadBuffer() {
        if (m_upload_buffer) {
            m_upload_buffer->Unmap(0, nullptr);
            // m_upload_buffer->Release();
            // m_upload_buffer.ReleaseAndGetAddressOf();
            
        }
        // delete m_mapped_data;
        m_mapped_data = nullptr; // unmapped the data so that should have freed it on the other end
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