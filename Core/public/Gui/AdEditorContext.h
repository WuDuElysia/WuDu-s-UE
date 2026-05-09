#pragma once

#include <vector>
#include <memory>
#include <string>

namespace WuDu {
	class AdScene;
	class AdEntity;
	class AdMesh;
	class AdPBRMaterial;

	// 编辑器面板间的共享状态
	struct AdEditorContext {
		AdScene* scene = nullptr;
		AdEntity* selectedEntity = nullptr;

		// 共享的 cube mesh 和默认材质（由 GuiSystem 设置）
		AdMesh* cubeMesh = nullptr;
		AdPBRMaterial* defaultMaterial = nullptr;

		void SetSelectedEntity(AdEntity* entity) { selectedEntity = entity; }
		void ClearSelection() { selectedEntity = nullptr; }

		// Validate selection: if selectedEntity is non-null but invalid, auto-clear to nullptr.
		// Call this each frame before rendering panels.
		void ValidateSelection();

		// 从模型文件路径创建实体，返回创建的实体指针，失败返回 nullptr
		AdEntity* CreateModelEntity(const std::string& modelPath);

	private:
		// 持有拖入模型创建的 GPU 资源，确保生命周期
		std::vector<std::shared_ptr<AdMesh>> mOwnedMeshes;
		std::vector<std::shared_ptr<AdPBRMaterial>> mOwnedMaterials;
	};
}
