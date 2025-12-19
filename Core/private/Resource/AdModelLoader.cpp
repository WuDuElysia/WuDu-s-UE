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
	ModelMesh AdModelLoader::ProcessMesh(aiMesh* mesh, const aiScene* scene,
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

		//提取索引数据
		for (unsigned int i = 0; i < mesh->mNumFaces; i++) {
			aiFace face = mesh->mFaces[i];
			for (unsigned int j = 0; j < face.mNumIndices; j++) {
				result.Indices.push_back(face.mIndices[j]);
			}
		}

		return result;
	}

	//处理材质
	void AdModelLoader::ProcessMaterials(const aiScene* scene,
							std::vector<ModelMaterial>& materials) {
		materials.reserve(scene->mNumMaterials);

		for (unsigned int i = 0; i < scene->mNumMaterials; i++) {
			aiMaterial* material = scene->mMaterials[i];
			ModelMaterial mat;

			//提取PBR基础颜色
			aiColor3D color(0.0f, 0.0f, 0.0f);
			if (material->Get(AI_MATKEY_BASE_COLOR, color) == AI_SUCCESS) {
				mat.BaseColor = glm::vec3(color.r, color.g, color.b);
			} else {
				mat.BaseColor = glm::vec3(1.0f, 1.0f, 1.0f);
			}

			//提取透明度
			float alpha = 1.0f;
			if (material->Get(AI_MATKEY_OPACITY, alpha) == AI_SUCCESS) {
				mat.Alpha = alpha;
			} else {
				mat.Alpha = 1.0f;
			}

			//提取自发光颜色
			aiColor3D emissiveColor(0.0f, 0.0f, 0.0f);
			if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissiveColor) == AI_SUCCESS) {
				mat.EmissiveColor = glm::vec3(emissiveColor.r, emissiveColor.g, emissiveColor.b);
			} else {
				mat.EmissiveColor = glm::vec3(0.0f, 0.0f, 0.0f);
			}

			//提取PBR金属度
			float metallic = 0.0f;
			if (material->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == AI_SUCCESS) {
				mat.Metallic = metallic;
			} else if (material->Get("$mat.metallic", 0, 0, metallic) == AI_SUCCESS) {
				mat.Metallic = metallic;
			} else {
				mat.Metallic = 0.0f;
			}
			
			//提取PBR粗糙度
			float roughness = 0.5f;
			if (material->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == AI_SUCCESS) {
				mat.Roughness = roughness;
			} else if (material->Get("$mat.roughness", 0, 0, roughness) == AI_SUCCESS) {
				mat.Roughness = roughness;
			} else {
				mat.Roughness = 0.0f;
			}
			
			//提取环境光遮蔽
			float ao = 1.0f;
			mat.AO = ao;
			
			//提取自发光强度
			float emissiveStrength = 1.0f;

			aiColor3D emissive;
			if (material->Get(AI_MATKEY_COLOR_EMISSIVE, emissive) == AI_SUCCESS) {
				emissiveStrength = glm::length(glm::vec3(emissive.r, emissive.g, emissive.b));
			}
			mat.EmissiveStrength = emissiveStrength;
			
			//设置各向异性默认值
			mat.Anisotropy = 0.0f;           // 默认无各向异性
			mat.AnisotropyDirection = glm::vec3(1.0f, 0.0f, 0.0f); // 默认各向异性方向
			


			//提取PBR纹理路径
			aiString texturePath;
			
			//基础颜色纹理
			if (material->GetTexture(aiTextureType_BASE_COLOR, 0, &texturePath) == AI_SUCCESS) {
				mat.BaseColorTexturePath = texturePath.C_Str();
			}

			//法线纹理
			if (material->GetTexture(aiTextureType_NORMALS, 0, &texturePath) == AI_SUCCESS) {
				mat.NormalTexturePath = texturePath.C_Str();
			}

			//金属度纹理
			if (material->GetTexture(aiTextureType_METALNESS, 0, &texturePath) == AI_SUCCESS) {
				mat.MetallicTexturePath = texturePath.C_Str();
			}

			//粗糙度纹理
			if (material->GetTexture(aiTextureType_DIFFUSE_ROUGHNESS, 0, &texturePath) == AI_SUCCESS) {
				mat.RoughnessTexturePath = texturePath.C_Str();
			} else if (material->GetTexture(aiTextureType_SHININESS, 0, &texturePath) == AI_SUCCESS) {
				// 使用光泽度纹理作为粗糙度纹理的回退
				mat.RoughnessTexturePath = texturePath.C_Str();
			}

			//环境光遮蔽纹理
			if (material->GetTexture(aiTextureType_AMBIENT_OCCLUSION, 0, &texturePath) == AI_SUCCESS) {
				mat.AOTexturePath = texturePath.C_Str();
			}



			//透明度贴图
			if (material->GetTexture(aiTextureType_OPACITY, 0, &texturePath) == AI_SUCCESS) {
				mat.AlphaTexturePath = texturePath.C_Str();
			}

			//自发光纹理
			if (material->GetTexture(aiTextureType_EMISSIVE, 0, &texturePath) == AI_SUCCESS) {
				mat.EmissiveTexturePath = texturePath.C_Str();
			}


			materials.push_back(mat);

		}
	}
}