#define NOMINMAX
#include "Gui/AdHierarchyPanel.h"
#include "Gui/AdEditorContext.h"
#include "ECS/AdEntity.h"
#include "ECS/AdScene.h"
#include "ECS/Component/Material/AdPBRMaterialComponent.h"
#include "Render/AdMesh.h"
#include "Render/AdMaterial.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include <cstring>

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

		// MODEL_PATH drop target: 使用窗口矩形区域注册自定义拖放目标
		ImRect windowRect(ImGui::GetWindowPos(), ImVec2(ImGui::GetWindowPos().x + ImGui::GetWindowSize().x, ImGui::GetWindowPos().y + ImGui::GetWindowSize().y));
		if (ImGui::BeginDragDropTargetCustom(windowRect, ImGui::GetID("##HierarchyDropArea"))) {
			if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("MODEL_PATH")) {
				const char* modelPath = static_cast<const char*>(payload->Data);
				context.CreateModelEntity(std::string(modelPath));
			}
			ImGui::EndDragDropTarget();
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

		// 如果正在重命名这个实体，显示输入框
		if (entity && mRenamingEntity == entity) {
			ImGui::PushID((void*)(uintptr_t)(uint32_t)node->GetId());
			ImGui::SetNextItemWidth(-1);
			if (ImGui::InputText("##rename", mRenameBuffer, sizeof(mRenameBuffer),
				ImGuiInputTextFlags_EnterReturnsTrue | ImGuiInputTextFlags_AutoSelectAll)) {
				// 按回车确认
				if (mRenameBuffer[0] != '\0') {
					entity->SetName(mRenameBuffer);
				}
				mRenamingEntity = nullptr;
			}
			// 失去焦点也确认
			if (!ImGui::IsItemActive() && mRenamingEntity == entity) {
				if (mRenameBuffer[0] != '\0') {
					entity->SetName(mRenameBuffer);
				}
				mRenamingEntity = nullptr;
			}
			// 首次显示时自动聚焦
			if (ImGui::IsItemVisible() && !ImGui::IsItemActive()) {
				ImGui::SetKeyboardFocusHere(-1);
			}
			ImGui::PopID();
			return;
		}

		bool opened = ImGui::TreeNodeEx((void*)(uintptr_t)(uint32_t)node->GetId(), flags, "%s", node->GetName().c_str());

		if (ImGui::IsItemClicked() && entity) {
			context.SetSelectedEntity(entity);
		}

		// 双击进入重命名
		if (ImGui::IsItemHovered() && ImGui::IsMouseDoubleClicked(0) && entity) {
			mRenamingEntity = entity;
			strncpy(mRenameBuffer, entity->GetName().c_str(), sizeof(mRenameBuffer) - 1);
			mRenameBuffer[sizeof(mRenameBuffer) - 1] = '\0';
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
				auto* entity = context.scene->CreateEntity("New Entity");
				context.SetSelectedEntity(entity);
			}

			if (ImGui::MenuItem("Create Cube")) {
				auto* cube = context.scene->CreateEntity("Cube");
				if (context.cubeMesh) {
					// 每个 cube 创建独立的材质实例
					AdPBRMaterial* mat = AdMaterialFactory::GetInstance()->CreateMaterial<AdPBRMaterial>();
					auto& matComp = cube->AddComponent<AdPBRMaterialComponent>();
					matComp.AddMesh(context.cubeMesh, mat);
				}
				context.SetSelectedEntity(cube);
			}

			ImGui::Separator();

			if (context.selectedEntity && context.selectedEntity->IsValid()) {
				if (ImGui::MenuItem("Rename")) {
					mRenamingEntity = context.selectedEntity;
					strncpy(mRenameBuffer, context.selectedEntity->GetName().c_str(), sizeof(mRenameBuffer) - 1);
					mRenameBuffer[sizeof(mRenameBuffer) - 1] = '\0';
				}
				if (ImGui::MenuItem("Delete Entity")) {
					context.scene->DestroyEntity(context.selectedEntity);
					context.ClearSelection();
				}
			} else {
				ImGui::BeginDisabled();
				ImGui::MenuItem("Rename");
				ImGui::MenuItem("Delete Entity");
				ImGui::EndDisabled();
			}

			ImGui::EndPopup();
		}
	}
}
