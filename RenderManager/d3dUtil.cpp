#include "d3dUtil.h"

const int g_num_frame_resources = 3;

DxException::DxException(HRESULT hr, const std::wstring& function_name, const std::wstring& filename, int line_number) :
	error_code(hr),
	function_name(function_name),
	filename(filename),
	line_number(line_number) {}
std::wstring DxException::ToString() const {
	// Get the string description of the error code.
	_com_error err(error_code);
	std::wstring msg = err.ErrorMessage();

	return function_name + L" failed in " + filename + L"; line " + std::to_wstring(line_number) + L"; error: " + msg;
}

Microsoft::WRL::ComPtr<ID3DBlob> compile_shader(const std::wstring& filename, const D3D_SHADER_MACRO* defines, const std::string& entrypoint, const std::string& target) {
	uint32_t compile_flags = 0;
#if defined(_DEBUG)
	compile_flags = D3DCOMPILE_DEBUG | D3DCOMPILE_SKIP_OPTIMIZATION;
#endif
	HRESULT hr = S_OK;
	Microsoft::WRL::ComPtr<ID3DBlob> bytecode = nullptr;
	Microsoft::WRL::ComPtr<ID3DBlob> errors;
	hr = D3DCompileFromFile(filename.c_str(), defines, D3D_COMPILE_STANDARD_FILE_INCLUDE, entrypoint.c_str(), target.c_str(), compile_flags, 0, &bytecode, &errors);
	if (errors) {
		OutputDebugStringA((char*)errors->GetBufferPointer());
	}
	ThrowIfFailed(hr);
	return bytecode;
}

Microsoft::WRL::ComPtr<ID3D12Resource> create_default_buffer(ID3D12Device* device, ID3D12GraphicsCommandList* cmd_list, const void* init_data, uint64_t size, Microsoft::WRL::ComPtr<ID3D12Resource>& upload_buffer) {
	Microsoft::WRL::ComPtr<ID3D12Resource> default_buffer;
	// create the actual default buffer resource
	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_DEFAULT),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_COMMON,
		nullptr,
		IID_PPV_ARGS(default_buffer.GetAddressOf())));
	// create an intermediate upload heap that the cpu can upload to and the gpu can read from
	ThrowIfFailed(device->CreateCommittedResource(&CD3DX12_HEAP_PROPERTIES(D3D12_HEAP_TYPE_UPLOAD),
		D3D12_HEAP_FLAG_NONE,
		&CD3DX12_RESOURCE_DESC::Buffer(size),
		D3D12_RESOURCE_STATE_GENERIC_READ,
		nullptr,
		IID_PPV_ARGS(upload_buffer.GetAddressOf())));
	// describe the data we want to copy into the default buffer
	D3D12_SUBRESOURCE_DATA sub_resource_data = {};
	sub_resource_data.pData = init_data;
	sub_resource_data.RowPitch = size;
	sub_resource_data.SlicePitch = sub_resource_data.RowPitch;
	// schedule to copy the data at the default buffer resource. done in function update_subresouces
	cmd_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(default_buffer.Get(), D3D12_RESOURCE_STATE_COMMON, D3D12_RESOURCE_STATE_COPY_DEST));
	UpdateSubresources<1>(cmd_list, default_buffer.Get(), upload_buffer.Get(), 0, 0, 1, &sub_resource_data); // uses d3dx12.h
	cmd_list->ResourceBarrier(1, &CD3DX12_RESOURCE_BARRIER::Transition(default_buffer.Get(), D3D12_RESOURCE_STATE_COPY_DEST, D3D12_RESOURCE_STATE_GENERIC_READ));
	// need to keep the upload buffer alive until the command list that performs the vopy is executed. can be released after the copy
	return default_buffer;
}