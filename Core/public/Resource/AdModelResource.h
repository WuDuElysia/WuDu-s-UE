#ifndef AD_MODEL_RESOURCE_H
#define AD_MODEL_RESOURCE_H

#include "AdResource.h"
#include "Graphic/AdVKBuffer.h"
#include <glm/glm.hpp>
#include "AdEngine.h"

namespace WuDu {
	struct ModelVertex {
		glm::vec3 Position;
		glm::vec3 Normal;
		glm::vec2 TexCoord;
		//切线
		glm::vec3 Tangent; 
		//副切线
		glm::vec3 Bitangent;
	};

	struct ModelMesh {
		std::string Name;
		//std::shared_ptr<AdVKBuffer> VertexBuffer;
		//std::shared_ptr<AdVKBuffer> IndexBuffer;
		std::vector<ModelVertex> Vertices;
		std::vector<uint32_t> Indices;
		uint32_t VertexCount;
		uint32_t IndexCount;
		uint32_t MaterialIndex;

	};

	struct ModelMaterial {
		std::string Name;

		// PBR材质属性
		glm::vec3 BaseColor;     // 基础颜色
		float Metallic;          // 金属度
		float Roughness;         // 粗糙度
		float AO;                // 环境光遮蔽
		float Alpha;             // 透明度
		glm::vec3 EmissiveColor; // 自发光颜色
		float EmissiveStrength;  // 自发光强度
		float Anisotropy;        // 各向异性因子(0.0=无各向异性, 1.0=完全各向异性)
		glm::vec3 AnisotropyDirection; // 各向异性方向

		// PBR纹理资源路径
		std::string BaseColorTexturePath;   // 基础颜色纹理
		std::string MetallicTexturePath;    // 金属度纹理
		std::string RoughnessTexturePath;   // 粗糙度纹理
		std::string NormalTexturePath;      // 法线纹理
		std::string AOTexturePath;          // 环境光遮蔽纹理
		std::string AlphaTexturePath;       // 透明度贴图
		std::string EmissiveTexturePath;    // 自发光纹理
	};

	class AdModelResource : public AdResource {
	public:
		AdModelResource(const std::string& modelPath);
		~AdModelResource();

		bool Load() override;
		void Unload() override;

		const std::vector<ModelMesh>& GetMeshes() const { return mMeshes;  }

	private:
		std::string mModelPath;
		std::vector<ModelMesh> mMeshes;
		std::vector<ModelMaterial> mMaterials;
		std::shared_ptr<AdVKDevice> mDevice;
		bool mIsLoaded = false;

	};


}

#endif