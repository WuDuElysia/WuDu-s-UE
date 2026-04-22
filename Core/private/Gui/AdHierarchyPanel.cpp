#define NOMINMAX
#include "Gui/AdHierarchyPanel.h"
#include "Gui/AdEditorContext.h"
#include "ECS/AdEntity.h"
#include "ECS/AdScene.h"
#include "imgui/imgui.h"

namespace WuDu {
	void AdHierarchyPanel::OnImGui(AdEditorContext& context) {
		ImGui::Begin("Scene Hierarchy");

		if (!context.scene) {
			ImGui::Text("No scene loaded");
			ImGui::End();
			return;
		}

		AdNode* rootNode = context.scene->GetRootNode();
		if (rootNode) {
			for (auto* child : rootNode->GetChildren()) {
				DrawNodeTree(context, child);
			}
		}

		// Deselect when clicking on empty space (only if not clicking on an item)
		if (ImGui::IsMouseClicked(0) && ImGui::IsWindowHovered() && !ImGui::IsAnyItemHovered()) {
			context.ClearSelection();
		}

		DrawContextMenu(context);

		ImGui::End();
	}

	void AdHierarchyPanel::DrawNodeTree(AdEditorContext& context, AdNode* node) {
		if (!node) return;

		AdEntity* entity = dynamic_cast<AdEntity*>(node);
		bool isSelected = (entity && context.selectedEntity == entity);

		ImGuiTreeNodeFlags flags = ImGuiTreeNodeFlags_OpenOnArrow | ImGuiTreeNodeFlags_SpanAvailWidth;
		if (isSelected) {
			flags |= ImGuiTreeNodeFlags_Selected;
		}
		if (!node->HasChildren()) {
			flags |= ImGuiTreeNodeFlags_Leaf | ImGuiTreeNodeFlags_NoTreePushOnOpen;
		}

		bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)(uint32_t)node->GetId(), flags, "%s", node->GetName().c_str());

		if (ImGui::IsItemClicked() && entity) {
			context.SetSelectedEntity(entity);
		}

		if (node->HasChildren() && opened) {
			for (auto* child : node->GetChildren()) {
				DrawNodeTree(context, child);
			}
			ImGui::TreePop();
		}
	}

	void AdHierarchyPanel::DrawContextMenu(AdEditorContext& context) {
		if (ImGui::BeginPopupContextWindow("HierarchyContextMenu", ImGuiPopupFlags_NoOpenOverExistingPopup | ImGuiPopupFlags_MouseButtonRight)) {
			if (ImGui::MenuItem("Create Empty Entity")) {
				context.scene->CreateEntity("New Entity");
			}

			if (context.selectedEntity && context.selectedEntity->IsValid()) {
				if (ImGui::MenuItem("Delete Entity")) {
					context.scene->DestroyEntity(context.selectedEntity);
					context.ClearSelection();
				}
			} else {
				ImGui::BeginDisabled();
				ImGui::MenuItem("Delete Entity");
				ImGui::EndDisabled();
			}

			ImGui::EndPopup();
		}
	}
}
