/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef ASSIMPIMPORTER_H
#define ASSIMPIMPORTER_H

#include <string>
#include <vector>
#include <iostream>
#include "Common.h"
#include "assimp/cimport.h"
#include "assimp/scene.h"
#include "assimp/postprocess.h"

class AssimpImporter 
{
public:
	static inline glm::mat4 aiMatrixToGlmMatrix(aiMatrix4x4 mat)
	{
		return glm::mat4(mat.a1, mat.a2, mat.a3, mat.a4, mat.b1, mat.b2, mat.b3, mat.b4, mat.c1, mat.c2, mat.c3, mat.c4, mat.d1, mat.d2, mat.d3, mat.d4);
	}

	static inline glm::mat3 aiMatrixToGlmMatrix(aiMatrix3x3 mat)
	{
		return glm::mat3(mat.a1, mat.a2, mat.a3, mat.b1, mat.b2, mat.b3, mat.c1, mat.c2, mat.c3);
	}

	struct ImportModel
	{
		std::string name;
		glm::mat4 transform;
		std::vector<glm::vec3> position;
		std::vector<glm::vec3> normal;
		std::vector<glm::vec3> tangent;
		std::vector<glm::vec3> bitangent;
		std::vector<glm::vec2> uv;
		std::vector<unsigned int> index;

		std::vector<ImportModel*> children;
	};

	static ImportModel* importModel(const std::string filename) 
	{
		const aiScene* scene = aiImportFile(filename.c_str(), aiProcess_Triangulate | aiProcess_FindInvalidData | aiProcess_GenSmoothNormals | aiProcess_ImproveCacheLocality | aiProcess_CalcTangentSpace | aiProcess_JoinIdenticalVertices | aiProcess_FindDegenerates | aiProcess_SortByPType);

		if (scene == 0)
		{
			std::cout << "Import for file " << filename << " failed!" << std::endl;
			return 0;
		}

		const aiNode* root = scene->mRootNode;
		
		return importNode(root, scene);
	}

	static ImportModel* importNode(const aiNode* node, const aiScene* scene)
	{
		bool children = false;
		ImportModel* m = new ImportModel();
		m->name = std::string(node->mName.C_Str());
		for (unsigned int i = 0; i < node->mNumChildren; i++)
		{
			children = true;
			ImportModel* cIM = importNode(node->mChildren[i], scene);
			if (cIM != 0)
			{
				m->children.push_back(cIM);
			}
		}

		if (node->mNumMeshes == 0 && !children)
		{
			//no mesh and no children -> discard
			delete m;
			return 0;
		}

		//Mesh
		if (node->mNumMeshes > 0)
		{
			if (node->mNumMeshes > 1)
			{
				std::cout << "WARNING: Node " << node->mName.C_Str() << " has more than one mesh! Only the first one will be imported." << std::endl;
			}

			for (unsigned int i = 0; i < glm::min(node->mNumMeshes, 1u); i++)
			{
				const aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
				for (unsigned int j = 0; j < mesh->mNumVertices; j++)
				{
					aiVector3D v = mesh->mVertices[j];
					m->position.push_back(glm::vec3(v.x, v.y, v.z));

					if (mesh->HasNormals())
					{
						aiVector3D n = mesh->mNormals[j];
						m->normal.push_back(glm::vec3(n.x,n.y,n.z));
					}

					if (mesh->HasTangentsAndBitangents())
					{
						aiVector3D t = mesh->mTangents[j];
						aiVector3D bt = mesh->mBitangents[j];
						m->tangent.push_back(glm::vec3(t.x, t.y, t.z));
						m->bitangent.push_back(glm::vec3(bt.x, bt.y, bt.z));
					}

					if (mesh->HasTextureCoords(0))
					{
						aiVector3D u = mesh->mTextureCoords[0][j];
						m->uv.push_back(glm::vec2(u.x,u.y));
					}
				}

				for (unsigned int k = 0; k < mesh->mNumFaces; k++)
				{
					aiFace face = mesh->mFaces[k];
					for (unsigned int l = 0; l < face.mNumIndices; l++)
					{
						m->index.push_back(face.mIndices[l]);
					}
				}
			}
		}

		//just transform
		else
		{
			m->transform = aiMatrixToGlmMatrix(node->mTransformation);
		}

		return m;
	}
};

#endif