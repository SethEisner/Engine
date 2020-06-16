#include "Object.h"
#include <assimp/mesh.h>
// void SubMeshData::init(const aiMesh* mesh) {
// 	int temp = 0;
// 	// assuming only one mesh in the scene (true for cat.dae)
// 	// scene->mMeshes->
// 	// m_vertices.resize(scene)
// 	m_name = mesh->mName.C_Str();
// 
// 	m_material_index = mesh->mMaterialIndex;
// 
// 	switch (mesh->mPrimitiveTypes) {
// 	case aiPrimitiveType_LINE:
// 		m_primitive = D3D11_PRIMITIVE_TOPOLOGY_LINELIST;
// 		break;
// 	case aiPrimitiveType_TRIANGLE:
// 		m_primitive = D3D11_PRIMITIVE_TOPOLOGY_TRIANGLELIST;
// 		break;
// 	default:
// 		assert(false && "mesh contains an unsupported primitive topology");
// 	}
// 
// 	m_vertices.resize(mesh->mNumVertices);
// 	for (size_t i = 0; i != mesh->mNumVertices; ++i) {
// 		m_vertices[i] = VertexData(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z,
// 			mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z,
// 			// 0.0f, 0.0f);
// 			// first numer (0) is the supported nymber of texture coordinates per mesh, should only care about meshes with one set of texture coordinates as of right now (so it's okay to just hardcode the 0) 
// 			mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y); // will need to change the hardcoded 0 to use a texture index we look up
// 	}
// 	// size_t num_indecis = 0;
// 	// for (size_t i = 0; i != mesh->mNumFaces; ++i) { // for each face, add the number of indecis it contains
// 	// 	num_indecis += mesh->mFaces[i].mNumIndices;
// 	// }
// 	//m_indecis.resize(3 * mesh->mNumFaces);
// 	mesh->mNumFaces;
// 	size_t index = 0;
// 	for (size_t i = 0; i != mesh->mNumFaces; ++i) { // for each face
// 		int num_indecis = mesh->mFaces[i].mNumIndices;
// 		aiFace temp_face = mesh->mFaces[i];
// 		for (size_t j = 0; j != num_indecis; ++j) { // add each index for the current face to the array
// 			int temp_index = mesh->mFaces[i].mIndices[j];
// 			m_indecis.push_back(mesh->mFaces[i].mIndices[j]);
// 			// m_indecis.push_back(index++);
// 		}
// 	}
// }