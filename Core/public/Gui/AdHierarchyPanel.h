// AdHierarchyPanel.h
#pragma once
#include <string>

namespace WuDu {
	struct AdEditorContext;
	class AdNode;
	class AdEntity;

	class AdHierarchyPanel {
	public:
		void OnImGui(AdEditorContext& context);

	private:
		void DrawNodeTree(AdEditorContext& context, AdNode* node);
		void DrawContextMenu(AdEditorContext& context);

		// 重命名状态
		AdEntity* mRenamingEntity = nullptr;
		char mRenameBuffer[256] = {};
	};
}
