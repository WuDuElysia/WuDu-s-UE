#ifndef AD_MODEL_RESOURCE_H
#define AD_MODEL_RESOURCE_H

#include "AdResource.h"
#include "Graphic/AdVKBuffer.h"
#include <glm/glm.hpp>

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
		std::shared_ptr<AdVKBuffer> VertexBuffer;
		std::shared_ptr<AdVKBuffer> IndexBuffer;
		uint32_t VertexCount;
		uint32_t IndexCount;
		uint32_t MaterialIndex;

	};

	struct ModelMaterial {
		std::string Name;
		//漫反射颜色
		glm::vec3 DifuseColor;
		//高光颜色
		glm::vec3 SpecularColor;
		//光泽度
		float Shininess;
		std::string DiffuseTexturePath;
		std::string NormalTexturePath;
		std::string SpecularTexturePath;
	};

	class AdModelResource : public AdResource {
	public:
		AdModelResource(const std::string& modelPath);
		~AdModelResource();

		bool Load() override;
		void Unload() override;

		const std::vector<ModelMesh>& GetMeshes() const { return mMeshs;  }

	private:
		std::vector<ModelMesh> mMeshs;
		std::vector<ModelMaterial> mMaterials;
		std::shared_ptr<AdVKDevice> mDevice;

	};


}

#endif