#pragma once
//#include <d3d12.h>
#include <DirectXMath.h>
#include <vector>
#include <assimp/Importer.hpp>
#include <assimp/mesh.h>
// could make vertex data contain contiguous arrays
class VertexData {
public:
	VertexData() = default;
	VertexData(const DirectX::XMFLOAT3& pos, const DirectX::XMFLOAT3& norm, const DirectX::XMFLOAT3& tangent, const DirectX::XMFLOAT2& tex_coord) :
		m_position(pos), m_normal(norm), /*m_tangent(tangent), */m_tex_coord(tex_coord) {}
	VertexData( float px, float py, float pz,
				float nx, float ny, float nz,
				/*float tx, float ty, float tz,*/
				float u, float v) :
		m_position(px, py, pz), m_normal(nx, ny, nz), /*m_tangent(tx, ty, tz), */m_tex_coord (u, v) {}
	DirectX::XMFLOAT3 m_position;
	DirectX::XMFLOAT3 m_normal;
	//DirectX::XMFLOAT3 m_tangent;
	DirectX::XMFLOAT2 m_tex_coord;
};
class MeshData {
public:
	MeshData() : m_vertices(std::vector<VertexData>()), m_indecis(std::vector<uint16_t>()) {}
	std::vector<VertexData> m_vertices;
	std::vector<uint16_t> m_indecis;
	void init(const aiMesh* mesh);
};