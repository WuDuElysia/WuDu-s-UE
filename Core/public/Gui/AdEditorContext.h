#pragma once

namespace WuDu {
	class AdScene;
	class AdEntity;

	// 编辑器面板间的共享状态
	struct AdEditorContext {
		AdScene* scene = nullptr;
		AdEntity* selectedEntity = nullptr;

		void SetSelectedEntity(AdEntity* entity) { selectedEntity = entity; }
		void ClearSelection() { selectedEntity = nullptr; }

		// Validate selection: if selectedEntity is non-null but invalid, auto-clear to nullptr.
		// Call this each frame before rendering panels.
		void ValidateSelection();
	};
}
