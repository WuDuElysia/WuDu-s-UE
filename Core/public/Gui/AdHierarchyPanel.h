// AdHierarchyPanel.h
#pragma once

namespace WuDu {
	struct AdEditorContext;
	class AdNode;

	class AdHierarchyPanel {
	public:
		void OnImGui(AdEditorContext& context);

	private:
		void DrawNodeTree(AdEditorContext& context, AdNode* node);
		void DrawContextMenu(AdEditorContext& context);
	};
}
