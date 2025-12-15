#pragma
#include "Resource/AdModelResource.h"
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace WuDu {

	class AdModelLoader {
	public:
		static bool LoadModel(const std::string& filePath,
								std::vector<ModelMesh>& meshes,
								std::vector<ModelMaterial>& materials);

	private:
		//处理assimp节点
		static void ProcessNode(aiNode* node,
							const aiScene* scene,
							std::vector<ModelMesh>& meshes,
							std::vector<ModelMaterial>& materials);

		//处理单个网格 提取顶点数据
		static ModelMesh ProcessMesh(aiMesh* mesh,
			const aiScene* scene,
			const std::vector<ModelMaterial>& materials);

		//处理材质
		static void ProcessMaterials(const aiScene* scene,
			std::vector<ModelMaterial>& materials);

	};

}