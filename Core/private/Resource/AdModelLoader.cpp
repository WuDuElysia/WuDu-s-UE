#include "Resource/AdModelLoader.h"

namespace WuDu {

	bool AdModelLoader::LoadModel(const std::string& filePath,
								std::vector<ModelMesh>& meshes,
								std::vector<ModelMaterial>& materials) {
		Assimp::Importer importer;

		//设置Assimp导入选项
		unsigned int importFlags =
			aiProcess_Triangulate |			//将所有图元转换为三角形
			aiProcess_FlipUVs |				//反转纹理坐标y轴
			aiProcess_CalcTangentSpace |		//计算切线和副切线
			aiProcess_GenSmoothNormals |	//生成平滑发现
			aiProcess_JoinIdenticalVertices |	//合并相同顶点
			aiProcess_OptimizeMeshes;			//优化网格

		//导入模型
		const aiScene* scene = importer.ReadFile(filePath,importFlags);

		if (!scene || scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE || !scene->mRootNode) {
			LOG_E("Assimp error: {0}",importer.GetErrorString());
			return false;
		}

		//处理材质
		ProcessMaterials(scene,materials);

		//递归处理所有节点
		ProcessNode(scene->mRootNode, scene, meshes, materials);

		return true;
	}

	void AdModelLoader::ProcessNode(aiNode* node, const aiScene* scene,
		std::vector<ModelMesh>& meshes,
		std::vector<ModelMaterial>& materials) {
		//处理当前节点的所有网格
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh* mesh = scene->mMeshes[node->mMeshes[i]];
			meshes.push_back(ProcessMesh(mesh, scene, materials));
		}

		//递归处理子节点
		for (unsigned int i = 0; i < node->mNumChildren; i++) {
			ProcessNode(node->mChildren[i], scene, meshes, materials);
		}
	}

	//处理单个网格 提取顶点数据
	ModelMesh ProcessMesh(aiMesh* mesh, const aiScene* scene,
						const std::vector<ModelMaterial>& materials) {
		ModelMesh result;
		result.MaterialIndex = mesh->mMaterialIndex;

		//提取顶点数据
		for (unsigned int i = 0; i < mesh->mNumVertices; i++) {
			ModelVertex vertex;

			// 提取位置
			vertex.Position.x = mesh->mVertices[i].x;
			vertex.Position.y = mesh->mVertices[i].y;
			vertex.Position.z = mesh->mVertices[i].z;

			// 提取纹理坐标（如果存在）
			if (mesh->mTextureCoords[0]) {
				vertex.TexCoord.x = mesh->mTextureCoords[0][i].x;
				vertex.TexCoord.y = mesh->mTextureCoords[0][i].y;
			}
			else {
				vertex.TexCoord = glm::vec2(0.0f, 0.0f);
			}

			// 提取法线
			vertex.Normal.x = mesh->mNormals[i].x;
			vertex.Normal.y = mesh->mNormals[i].y;
			vertex.Normal.z = mesh->mNormals[i].z;

			// 提取切线和副切线（如果存在）
			if (mesh->mTangents && mesh->mBitangents) {
				vertex.Tangent.x = mesh->mTangents[i].x;
				vertex.Tangent.y = mesh->mTangents[i].y;
				vertex.Tangent.z = mesh->mTangents[i].z;

				vertex.Bitangent.x = mesh->mBitangents[i].x;
				vertex.Bitangent.y = mesh->mBitangents[i].y;
				vertex.Bitangent.z = mesh->mBitangents[i].z;
			}

			result.Vertices.push_back(vertex);
		}
	}
}