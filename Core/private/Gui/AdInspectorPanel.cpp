#define NOMINMAX
#include "Gui/AdInspectorPanel.h"
#include "Gui/AdEditorContext.h"
#include "Gui/AdComponentRegistry.h"
#include "Gui/AdComponentInfo.h"
#include "Gui/AdPropertyDescriptor.h"
#include "ECS/AdEntity.h"
#include "ECS/AdScene.h"
#include "ECS/Component/Material/AdPBRMaterialComponent.h"
#include "ECS/Component/Light/AdPointLightComponent.h"
#include "ECS/Component/Light/AdDirectionalLightComponent.h"
#include "ECS/Component/Light/AdSpotLightComponent.h"
#include "Render/AdTexture.h"
#include "Render/AdSampler.h"
#include "imgui/imgui.h"
#include <glm/glm.hpp>
#include <filesystem>
#include "AdLog.h"

namespace WuDu {

	// Helper: get the first AdPBRMaterial* from a PBR material component
	static AdPBRMaterial* GetFirstPBRMaterialFromComp(AdPBRMaterialComponent* comp) {
		const auto& meshMaterials = comp->GetMeshMaterials();
		if (meshMaterials.empty()) {
			return nullptr;
		}
		return meshMaterials.begin()->first;
	}

	// PBR texture slot definitions
	struct PBRTextureSlotInfo {
		uint32_t slotId;
		const char* label;
	};

	static const PBRTextureSlotInfo s_PBRTextureSlots[] = {
		{ PBR_MAT_BASE_COLOR,          "Base Color" },
		{ PBR_MAT_NORMAL,              "Normal" },
		{ PBR_MAT_METALLIC_ROUGHNESS,  "Metallic/Roughness" },
		{ PBR_MAT_AO,                  "Ambient Occlusion" },
		{ PBR_MAT_EMISSIVE,            "Emissive" },
	};

	void AdInspectorPanel::OnImGui(AdEditorContext& context) {
		ImGui::Begin("Inspector");

		context.ValidateSelection();

		if (!context.selectedEntity) {
			ImGui::Text("No entity selected");
			ImGui::End();
			return;
		}

		AdEntity* entity = context.selectedEntity;
		entt::entity enttEntity = entity->GetEcsEntity();
		entt::registry& registry = context.scene->GetEcsRegistry();

		auto components = AdComponentRegistry::GetInstance().GetComponentsForEntity(registry, enttEntity);

		for (const AdComponentInfo* info : components) {
			void* componentPtr = info->getComponent(registry, enttEntity);
			if (!componentPtr) continue;

			ImGui::PushID(static_cast<int>(info->typeId));

			ImGuiTreeNodeFlags headerFlags = ImGuiTreeNodeFlags_DefaultOpen;
			bool headerOpen = ImGui::CollapsingHeader(info->displayName.c_str(), headerFlags);

			// Right-click context menu for Remove Component (skip Transform)
			bool removeRequested = false;
			if (info->displayName != "Transform") {
				if (ImGui::BeginPopupContextItem()) {
					if (ImGui::MenuItem("Remove Component")) {
						removeRequested = true;
					}
					ImGui::EndPopup();
				}
			}

			if (headerOpen) {
				DrawComponent(context, *info, componentPtr);

				// Render PBR texture slots after PBR Material component properties
				if (info->displayName == "PBR Material") {
					DrawPBRTextureSlots(context);
				}
			}

			ImGui::PopID();

			if (removeRequested && info->removeComponent) {
				info->removeComponent(registry, enttEntity);
			}
		}

		DrawAddComponentButton(context);

		ImGui::End();
	}

	void AdInspectorPanel::DrawComponent(AdEditorContext& context,
										 const AdComponentInfo& info, void* component) {
		ImGui::PushID(info.typeId);
		for (const auto& prop : info.properties) {
			DrawProperty(prop, component);
		}
		ImGui::PopID();
	}

	void AdInspectorPanel::DrawProperty(const AdPropertyDescriptor& prop, void* component) {
		std::string label = prop.displayLabel + "##" + prop.name;

		switch (prop.type) {
			case PropertyType::Float: {
				float value;
				prop.getter(component, &value);
				float minVal = prop.minValue.value_or(0.0f);
				float maxVal = prop.maxValue.value_or(0.0f);
				if (ImGui::DragFloat(label.c_str(), &value, prop.dragSpeed, minVal, maxVal)) {
					prop.setter(component, &value);
				}
				break;
			}
			case PropertyType::Int: {
				int value;
				prop.getter(component, &value);
				int minVal = static_cast<int>(prop.minValue.value_or(0.0f));
				int maxVal = static_cast<int>(prop.maxValue.value_or(0.0f));
				if (ImGui::DragInt(label.c_str(), &value, prop.dragSpeed, minVal, maxVal)) {
					prop.setter(component, &value);
				}
				break;
			}
			case PropertyType::Bool: {
				bool value;
				prop.getter(component, &value);
				if (ImGui::Checkbox(label.c_str(), &value)) {
					prop.setter(component, &value);
				}
				break;
			}
			case PropertyType::Vec2: {
				glm::vec2 value;
				prop.getter(component, &value);
				float minVal = prop.minValue.value_or(0.0f);
				float maxVal = prop.maxValue.value_or(0.0f);
				if (ImGui::DragFloat2(label.c_str(), &value.x, prop.dragSpeed, minVal, maxVal)) {
					prop.setter(component, &value);
				}
				break;
			}
			case PropertyType::Vec3: {
				glm::vec3 value;
				prop.getter(component, &value);
				float minVal = prop.minValue.value_or(0.0f);
				float maxVal = prop.maxValue.value_or(0.0f);
				if (ImGui::DragFloat3(label.c_str(), &value.x, prop.dragSpeed, minVal, maxVal)) {
					prop.setter(component, &value);
				}
				break;
			}
			case PropertyType::Vec4: {
				glm::vec4 value;
				prop.getter(component, &value);
				float minVal = prop.minValue.value_or(0.0f);
				float maxVal = prop.maxValue.value_or(0.0f);
				if (ImGui::DragFloat4(label.c_str(), &value.x, prop.dragSpeed, minVal, maxVal)) {
					prop.setter(component, &value);
				}
				break;
			}
			case PropertyType::Color3: {
				glm::vec3 value;
				prop.getter(component, &value);
				if (ImGui::ColorEdit3(label.c_str(), &value.x)) {
					prop.setter(component, &value);
				}
				break;
			}
			case PropertyType::Color4: {
				glm::vec4 value;
				prop.getter(component, &value);
				if (ImGui::ColorEdit4(label.c_str(), &value.x)) {
					prop.setter(component, &value);
				}
				break;
			}
			case PropertyType::Enum: {
				int currentIndex;
				prop.getter(component, &currentIndex);
				// Build null-separated string for ImGui::Combo
				std::string comboItems;
				for (const auto& option : prop.enumOptions) {
					comboItems += option;
					comboItems += '\0';
				}
				comboItems += '\0';
				if (ImGui::Combo(label.c_str(), &currentIndex, comboItems.c_str())) {
					prop.setter(component, &currentIndex);
				}
				break;
			}
		}
	}

	void AdInspectorPanel::DrawPBRTextureSlots(AdEditorContext& context) {
		if (!context.selectedEntity) return;

		entt::entity enttEntity = context.selectedEntity->GetEcsEntity();
		entt::registry& registry = context.scene->GetEcsRegistry();

		if (!registry.all_of<AdPBRMaterialComponent>(enttEntity)) return;

		// Skip PBR texture slots for light entities to prevent freeze
		if (registry.any_of<AdPointLightComponent, AdDirectionalLightComponent, AdSpotLightComponent>(enttEntity)) return;

		auto& comp = registry.get<AdPBRMaterialComponent>(enttEntity);
		AdPBRMaterial* material = GetFirstPBRMaterialFromComp(&comp);
		if (!material) {
			ImGui::TextDisabled("No PBR material instance");
			return;
		}

		// Auto-clear error after 3 seconds
		if (!mTextureLoadError.empty() && (ImGui::GetTime() - mErrorTimestamp) > 3.0) {
			mTextureLoadError.clear();
			mErrorSlotId = -1;
		}

		ImGui::Separator();
		ImGui::Text("Texture Slots");
		ImGui::Spacing();

		for (const auto& slot : s_PBRTextureSlots) {
			ImGui::PushID(static_cast<int>(slot.slotId));

			const TextureView* texView = material->GetTextureView(slot.slotId);
			bool hasTexture = texView && texView->texture;

			// 整个槽位区域作为一个组，用 Group 包裹方便做拖拽目标
			ImGui::BeginGroup();

			// 槽位标签
			ImGui::Text("%s:", slot.label);
			ImGui::SameLine(120.0f);

			if (hasTexture) {
				// 显示纹理文件名
				auto pathIt = mLoadedTexturePaths.find(slot.slotId);
				if (pathIt != mLoadedTexturePaths.end()) {
					std::string fileName = std::filesystem::path(pathIt->second).filename().string();
					ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "%s", fileName.c_str());
				} else {
					ImGui::TextColored(ImVec4(0.4f, 0.8f, 0.4f, 1.0f), "Assigned");
				}

				// Enable/disable checkbox
				ImGui::SameLine();
				bool enabled = texView->bEnable;
				if (ImGui::Checkbox("##enable", &enabled)) {
					material->UpdateTextureViewEnable(slot.slotId, enabled);
				}

				// Clear button
				ImGui::SameLine();
				if (ImGui::SmallButton("Clear")) {
					material->SetTextureView(slot.slotId, nullptr, nullptr);
					mLoadedTextures.erase(slot.slotId);
					mLoadedTexturePaths.erase(slot.slotId);
				}
			} else {
				// 没有纹理时显示一个可视化的拖拽目标按钮
				ImGui::Button("Drag texture here##slot", ImVec2(ImGui::GetContentRegionAvail().x, 24.0f));
			}

			ImGui::EndGroup();

			// 整个 Group 区域作为拖拽目标
			if (ImGui::BeginDragDropTarget()) {
				if (const ImGuiPayload* payload = ImGui::AcceptDragDropPayload("TEXTURE_PATH")) {
					std::string texturePath(static_cast<const char*>(payload->Data), payload->DataSize - 1);
					LOG_D("[DragDrop] Accepted texture drag-drop: slotId={0}, path=\"{1}\", payloadSize={2}", slot.slotId, texturePath, payload->DataSize);
					mPendingTextureLoads.push_back({ slot.slotId, texturePath });
					LOG_D("[DragDrop] Queued pending load. Queue size now: {0}", mPendingTextureLoads.size());
				}
				ImGui::EndDragDropTarget();
			}

			// Show error text for this slot
			if (mErrorSlotId == static_cast<int>(slot.slotId) && !mTextureLoadError.empty()) {
				ImGui::TextColored(ImVec4(1.0f, 0.2f, 0.2f, 1.0f), "%s", mTextureLoadError.c_str());
			}

			ImGui::PopID();
		}
	}

	void AdInspectorPanel::ProcessPendingTextureLoads(AdEditorContext& context) {
		if (mPendingTextureLoads.empty()) return;

		LOG_D("[ProcessPending] === Start processing {0} pending texture load(s) ===", mPendingTextureLoads.size());

		if (!context.selectedEntity) {
			LOG_W("[ProcessPending] No selected entity, clearing queue");
			mPendingTextureLoads.clear();
			return;
		}

		entt::entity enttEntity = context.selectedEntity->GetEcsEntity();
		entt::registry& registry = context.scene->GetEcsRegistry();
		LOG_D("[ProcessPending] Selected entity ECS id: {0}", static_cast<uint32_t>(enttEntity));

		if (!registry.all_of<AdPBRMaterialComponent>(enttEntity)) {
			LOG_W("[ProcessPending] Entity has no AdPBRMaterialComponent, clearing queue");
			mPendingTextureLoads.clear();
			return;
		}

		auto& comp = registry.get<AdPBRMaterialComponent>(enttEntity);
		AdPBRMaterial* material = GetFirstPBRMaterialFromComp(&comp);
		if (!material) {
			LOG_W("[ProcessPending] GetFirstPBRMaterialFromComp returned nullptr, clearing queue");
			mPendingTextureLoads.clear();
			return;
		}
		LOG_D("[ProcessPending] Got material ptr: {0}", (void*)material);

		for (const auto& request : mPendingTextureLoads) {
			LOG_D("[ProcessPending] Processing: slotId={0}, path=\"{1}\"", request.slotId, request.texturePath);

			if (!mDefaultSampler) {
				LOG_D("[ProcessPending] Creating default sampler...");
				mDefaultSampler = std::make_shared<AdSampler>();
				LOG_D("[ProcessPending] Default sampler created: {0}", (void*)mDefaultSampler.get());
			}

			LOG_D("[ProcessPending] Creating AdTexture from \"{0}\"...", request.texturePath);
			auto texture = std::make_shared<AdTexture>(request.texturePath);
			LOG_D("[ProcessPending] AdTexture created. GetImage()={0}", (void*)texture->GetImage());

			if (texture->GetImage()) {
				LOG_D("[ProcessPending] Texture loaded OK. Updating mLoadedTextures and calling SetTextureView(slotId={0})...", request.slotId);
				mLoadedTextures[request.slotId] = texture;
				mLoadedTexturePaths[request.slotId] = request.texturePath;
				material->SetTextureView(request.slotId, texture.get(), mDefaultSampler.get());
				LOG_D("[ProcessPending] SetTextureView done for slotId={0}", request.slotId);
			} else {
				mTextureLoadError = "Failed to load: " + std::filesystem::path(request.texturePath).filename().string();
				mErrorTimestamp = static_cast<float>(ImGui::GetTime());
				mErrorSlotId = static_cast<int>(request.slotId);
				LOG_E("[ProcessPending] Texture load FAILED for slotId={0}, path=\"{1}\"", request.slotId, request.texturePath);
			}
		}

		mPendingTextureLoads.clear();
		LOG_D("[ProcessPending] === Done. Queue cleared ===");
	}

	void AdInspectorPanel::DrawAddComponentButton(AdEditorContext& context) {
		if (!context.selectedEntity) return;

		entt::entity enttEntity = context.selectedEntity->GetEcsEntity();
		entt::registry& registry = context.scene->GetEcsRegistry();

		ImGui::Separator();

		float availWidth = ImGui::GetContentRegionAvail().x;
		if (ImGui::Button("Add Component", ImVec2(availWidth, 0))) {
			ImGui::OpenPopup("AddComponentPopup");
		}

		if (ImGui::BeginPopup("AddComponentPopup")) {
			auto missing = AdComponentRegistry::GetInstance().GetMissingComponentsForEntity(registry, enttEntity);
			if (missing.empty()) {
				ImGui::TextDisabled("All components added");
			} else {
				for (const AdComponentInfo* info : missing) {
					if (ImGui::MenuItem(info->displayName.c_str())) {
						if (info->addComponent) {
							info->addComponent(registry, enttEntity);
						}
					}
				}
			}
			ImGui::EndPopup();
		}
	}

}
