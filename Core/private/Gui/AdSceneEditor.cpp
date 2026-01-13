// AdSceneEditor.cpp
#include "Gui/AdSceneEditor.h"
#include "AdApplication.h"
#include "Graphic/AdVKRenderPass.h"
#include "Window/AdGlfwWindow.h"

namespace WuDu {
	AdSceneEditor::AdSceneEditor() {
	}

	void AdSceneEditor::SetResources(
		AdScene* scene,
		AdEntity* activeCamera,
		AdMesh* cubeMesh,
		AdUnlitMaterial* defaultMaterial
	) {
		mScene = scene;
		mActiveCamera = activeCamera;
		mCubeMesh = cubeMesh;
		mDefaultMaterial = defaultMaterial;
	}

	void AdSceneEditor::AddSceneEditor() {
		// 添加场景编辑器的GUI函数
		ImGui::Begin("Scene Hierarchy");
		ShowSceneHierarchy();
		ImGui::End();

		ImGui::Begin("Properties");
		ShowTransformEditor();
		ImGui::End();

		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0, 0));
		ImGui::Begin("Viewport", nullptr, ImGuiWindowFlags_NoScrollbar);
		HandleSceneViewport();
		ImGui::End();
		ImGui::PopStyleVar();
	}

	void AdSceneEditor::ShowSceneHierarchy() {
		if (!mScene) {
			ImGui::Text("No scene set");
			return;
		}

		if (ImGui::Button("Add Cube")) {
			// 创建新实体
			AdEntity* cube = mScene->CreateEntity("Cube");

			// 添加材质组件并关联网格和材质
			auto& materialComp = cube->AddComponent<AdUnlitMaterialComponent>();
			materialComp.AddMesh(mCubeMesh, mDefaultMaterial);

			// 选中新创建的实体
			SelectEntity(cube);
		}

		ImGui::Separator();

		// 显示场景中所有实体
		entt::registry& reg = mScene->GetEcsRegistry();
		auto view = reg.view<AdTransformComponent>();

		// 如果没有符合条件的实体，则直接返回
		if (std::distance(view.begin(), view.end()) == 0) {
			return;
		}

		// 遍历实体
		for (auto entity : view) {
			// 将entt::entity转换为AdEntity*
			AdEntity* adEntity = mScene->GetEntity(entity);
			if (!adEntity) continue;

			// 获取实体名称
			std::string name = adEntity->GetName();

			ImGui::PushID(static_cast<int>(entity));
			bool isSelected = (adEntity == mSelectedEntity);
			if (ImGui::Selectable(name.c_str(), &isSelected)) {
				SelectEntity(adEntity);
			}
			ImGui::PopID();
		}
	}

	void AdSceneEditor::ShowTransformEditor() {
		if (!mSelectedEntity) {
			ImGui::Text("No entity selected");
			return;
		}

		// 检查实体是否有变换组件
		if (!mSelectedEntity->HasComponent<AdTransformComponent>()) {
			ImGui::Text("Selected entity has no transform");
			return;
		}

		auto& transform = mSelectedEntity->GetComponent<AdTransformComponent>();

		// 位置编辑
		ImGui::Text("Position");
		ImGui::DragFloat3("##Position", &transform.position[0], 0.1f);

		// 旋转编辑
		ImGui::Text("Rotation (degrees)");
		ImGui::DragFloat3("##Rotation", &transform.rotation[0], 0.5f, -360.0f, 360.0f);

		// 缩放编辑
		ImGui::Text("Scale");
		ImGui::DragFloat3("##Scale", &transform.scale[0], 0.01f, 0.01f);
	}

	void AdSceneEditor::HandleSceneViewport() {
		ImGuiIO& io = ImGui::GetIO();
		ImVec2 viewportPanelSize = ImGui::GetContentRegionAvail();

		// 记录视口尺寸
		if (viewportPanelSize.x > 0 && viewportPanelSize.y > 0) {
			mViewportSize = { viewportPanelSize.x, viewportPanelSize.y };
		}

		// 处理视口交互
		ImGui::SetItemAllowOverlap();
		if (ImGui::IsItemHovered()) {
			// 处理鼠标点击选择
			if (ImGui::IsMouseClicked(0) && !io.WantCaptureMouse) {
				SelectEntityAtMousePosition(io.MousePos.x, io.MousePos.y);
			}

			// 处理拖动变换
			if (mSelectedEntity && ImGui::IsMouseDown(0) && !io.WantCaptureMouse) {
				HandleTransformDrag(io.MousePos);
			}
		}
	}

	void AdSceneEditor::SelectEntity(AdEntity* entity) {
		mSelectedEntity = entity;
		if (entity) {
			// 选中时记录初始鼠标位置，用于拖动
			ImGuiIO& io = ImGui::GetIO();
			mLastMousePos = { io.MousePos.x, io.MousePos.y };
		}
	}

	void AdSceneEditor::SelectEntityAtMousePosition(float mouseX, float mouseY) {
		if (!mActiveCamera || !mScene) return;

		auto& cameraTransform = mActiveCamera->GetComponent<AdTransformComponent>();
		auto& cameraComp = mActiveCamera->GetComponent<AdFirstPersonCameraComponent>();

		// 1. 将屏幕坐标转换为NDC坐标 (-1到1)
		float ndcX = (mouseX / mViewportSize.x) * 2.0f - 1.0f;
		float ndcY = 1.0f - (mouseY / mViewportSize.y) * 2.0f; // 翻转Y轴

		// 2. 创建射线（从摄像机位置到屏幕点）
		glm::vec3 rayOrigin;
		glm::vec3 rayDirection;

		// 3. 使用摄像机信息计算射线
		cameraComp.Unproject(ndcX, ndcY, rayOrigin, rayDirection);

		// 4. 射线与场景中实体的碰撞检测
		float closestDistance = std::numeric_limits<float>::max();
		AdEntity* closestEntity = nullptr;

		entt::registry& reg = mScene->GetEcsRegistry();
		auto view = reg.view<AdTransformComponent, AdUnlitMaterialComponent>();

		view.each([&](auto entity, AdTransformComponent& transform, AdUnlitMaterialComponent& materialComp) {
			// 将entt::entity转换为AdEntity*
			AdEntity* adEntity = mScene->GetEntity(entity);
			if (!adEntity) return;

			float distance = RayIntersectsCube(rayOrigin, rayDirection, transform);

			if (distance > 0 && distance < closestDistance) {
				closestDistance = distance;
				closestEntity = adEntity;
			}
			});

		mSelectedEntity = closestEntity;
	}

	void AdSceneEditor::HandleTransformDrag(const ImVec2& currentMousePos) {
		if (!mSelectedEntity || !mActiveCamera) return;

		auto& transform = mSelectedEntity->GetComponent<AdTransformComponent>();
		auto& cameraTransform = mActiveCamera->GetComponent<AdTransformComponent>();

		// 计算鼠标移动量
		glm::vec2 delta = { currentMousePos.x - mLastMousePos.x, currentMousePos.y - mLastMousePos.y };

		switch (mCurrentTransformMode) {
		case TransformMode::Translate: {
			// 基于摄像机方向计算移动方向
			glm::vec3 forward = cameraTransform.GetTransform() * glm::vec4(0, 0, -1, 0);
			glm::vec3 right = cameraTransform.GetTransform() * glm::vec4(1, 0, 0, 0);

			// 计算移动量（考虑摄像机距离）
			float distance = glm::distance(transform.position, cameraTransform.position);
			float sensitivity = 0.01f * distance;

			transform.position += right * (delta.x * sensitivity);
			transform.position += glm::vec3(0, 1, 0) * (delta.y * sensitivity);
			break;
		}
		case TransformMode::Rotate: {
			// 绕Y轴旋转（简化版）
			transform.rotation.y += delta.x * 0.5f;
			transform.rotation.x += delta.y * 0.5f;
			// 限制X轴旋转范围，避免万向锁
			transform.rotation.x = glm::clamp(transform.rotation.x, -89.0f, 89.0f);
			break;
		}
		case TransformMode::Scale: {
			// 统一缩放
			float scaleDelta = (delta.x - delta.y) * 0.01f;
			transform.scale += glm::vec3(scaleDelta);
			// 确保缩放不为负
			transform.scale = glm::max(transform.scale, glm::vec3(0.01f));
			break;
		}
		}

		mLastMousePos = glm::vec2(currentMousePos.x, currentMousePos.y);
	}

	float AdSceneEditor::RayIntersectsCube(const glm::vec3& rayOrigin,
		const glm::vec3& rayDirection,
		const AdTransformComponent& transform) {
		// 简化的射线-立方体相交检测
		// 实际应用中应该使用更精确的算法

		// 1. 将射线转换到立方体的局部空间
		glm::mat4 worldToModel = glm::inverse(transform.GetTransform());
		glm::vec4 localRayOrigin = worldToModel * glm::vec4(rayOrigin, 1.0f);
		glm::vec4 localRayDir = worldToModel * glm::vec4(rayDirection, 0.0f);

		// 2. 检测与单位立方体的相交
		float tMin = -std::numeric_limits<float>::max();
		float tMax = std::numeric_limits<float>::max();

		// 检查X轴
		if (std::abs(localRayDir.x) > 1e-6) {
			float t1 = (-0.5f - localRayOrigin.x) / localRayDir.x;
			float t2 = (0.5f - localRayOrigin.x) / localRayDir.x;
			tMin = std::max(tMin, std::min(t1, t2));
			tMax = std::min(tMax, std::max(t1, t2));
		}

		// 检查Y轴
		if (std::abs(localRayDir.y) > 1e-6) {
			float t1 = (-0.5f - localRayOrigin.y) / localRayDir.y;
			float t2 = (0.5f - localRayOrigin.y) / localRayDir.y;
			tMin = std::max(tMin, std::min(t1, t2));
			tMax = std::min(tMax, std::max(t1, t2));
		}

		// 检查Z轴
		if (std::abs(localRayDir.z) > 1e-6) {
			float t1 = (-0.5f - localRayOrigin.z) / localRayDir.z;
			float t2 = (0.5f - localRayOrigin.z) / localRayDir.z;
			tMin = std::max(tMin, std::min(t1, t2));
			tMax = std::min(tMax, std::max(t1, t2));
		}

		if (tMax >= std::max(0.0f, tMin)) {
			return tMin > 0 ? tMin : tMax;
		}

		return -1.0f; // 无相交
	}
}