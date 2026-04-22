#define NOMINMAX
#include "ECS/AdEntity.h"
#include "Gui/AdEditorContext.h"

#include <filesystem>
#include "Resource/AdModelResource.h"
#include "Resource/AdResourceManager.h"
#include "Render/AdMesh.h"
#include "ECS/Component/Material/AdPBRMaterialComponent.h"
#include "Adlog.h"

namespace WuDu {
	void AdEditorContext::ValidateSelection() {
		if (selectedEntity && !selectedEntity->IsValid()) {
			selectedEntity = nullptr;
		}
	}

	AdEntity* AdEditorContext::CreateModelEntity(const std::string& modelPath) {
		// 1. 检查文件是否存在
		if (!std::filesystem::exists(modelPath)) {
			LOG_E("Model file not found: {0}", modelPath);
			return nullptr;
		}

		// 2. 通过 ResourceManager 加载（自动缓存）
		auto modelRes = AdResourceManager::GetInstance()->Load<AdModelResource>(modelPath);
		if (!modelRes) {
			LOG_E("Failed to load model: {0}", modelPath);
			return nullptr;
		}

		// 3. 检查是否有网格
		const auto& meshes = modelRes->GetMeshes();
		if (meshes.empty()) {
			LOG_W("Model contains zero meshes: {0}", modelPath);
			return nullptr;
		}

		// 4. 提取文件名（不含扩展名）作为实体名
		std::string entityName = std::filesystem::path(modelPath).stem().string();

		// 5. 创建实体
		AdEntity* entity = scene->CreateEntity(entityName);

		// 6. 创建默认 PBR 材质
		AdPBRMaterial* defaultMat = AdMaterialFactory::GetInstance()->CreateMaterial<AdPBRMaterial>();

		// 7. 添加 PBR 材质组件并注册所有网格
		auto& matComp = entity->AddComponent<AdPBRMaterialComponent>();
		for (const auto& modelMesh : meshes) {
			auto gpuMesh = std::make_shared<AdMesh>(modelMesh.Vertices, modelMesh.Indices);
			matComp.AddMesh(gpuMesh.get(), defaultMat);
			mOwnedMeshes.push_back(gpuMesh);
		}

		// 8. 选中新实体
		SetSelectedEntity(entity);

		return entity;
	}
}
