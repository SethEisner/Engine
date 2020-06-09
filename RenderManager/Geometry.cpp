#include "Geometry.h"
#include <assimp/mesh.h>
void MeshData::init(const aiMesh* mesh) {
	int temp = 0;
	// assuming only one mesh in the scene (true for cat.dae)
	// scene->mMeshes->
	// m_vertices.resize(scene)
	m_vertices.resize(mesh->mNumVertices);
	for (size_t i = 0; i != mesh->mNumVertices; ++i) {
		m_vertices[i] = VertexData(mesh->mVertices[i].x, mesh->mVertices[i].y, mesh->mVertices[i].z, 
								   mesh->mNormals[i].x, mesh->mNormals[i].y, mesh->mNormals[i].z, 
								   mesh->mTextureCoords[0][i].x, mesh->mTextureCoords[0][i].y); // will need to change the hardcoded 0 to use a texture index we look up
	}
	// size_t num_indecis = 0;
	// for (size_t i = 0; i != mesh->mNumFaces; ++i) { // for each face, add the number of indecis it contains
	// 	num_indecis += mesh->mFaces[i].mNumIndices;
	// }
	//m_indecis.resize(num_indecis);
	//size_t index = 0;
	for (size_t i = 0; i != mesh->mNumFaces; ++i) { // for each face
		int num_indecis = mesh->mFaces[i].mNumIndices;
		aiFace temp_face = mesh->mFaces[i];
		for (size_t j = 0; j != num_indecis; ++j) { // add each index for the current face to the array
			int temp_index = mesh->mFaces[i].mIndices[j];
			m_indecis.push_back(mesh->mFaces[i].mIndices[j]);
		}
	}
}