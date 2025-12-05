#include "ECS/Component/AdLookAtCameraComponent.h"
#include "ECS/Component/AdTransformComponent.h"

namespace WuDu {
	/**
	 * @brief 获取投影矩阵
	 * @details 根据相机的视场角、宽高比、近平面和远平面参数计算透视投影矩阵，适配Vulkan坐标系
	 * @return 返回计算得到的投影矩阵的常量引用
	 */
	const glm::mat4& AdLookAtCameraComponent::GetProjMat() {
		// 使用适合Vulkan的投影函数
		mProjMat = glm::perspectiveRH_ZO(glm::radians(mFov), mAspect, mNearPlane, mFarPlane);
		// GLM的RH_ZO版本已经适配了Vulkan的坐标系(Y轴向下，深度[0,1])
		return mProjMat;
	}

	/**
	 * @brief 获取视图矩阵
	 * @details 根据相机的目标位置、角度和半径计算相机位置，并生成视图矩阵。
	 *          相机位置通过球坐标转换为笛卡尔坐标计算得出，确保相机始终看向目标点。
	 * @return 返回计算得到的视图矩阵的常量引用
	 */
	const glm::mat4& AdLookAtCameraComponent::GetViewMat() {
		AdEntity* owner = GetOwner();
		if (AdEntity::HasComponent<AdTransformComponent>(owner)) {
			auto& transComp = owner->GetComponent<AdTransformComponent>();
			float yaw = transComp.rotation.x;
			float pitch = transComp.rotation.y;

			// 根据偏航角和俯仰角计算相机方向向量
			glm::vec3 direction;
			direction.x = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
			direction.y = sin(glm::radians(pitch));
			direction.z = cos(glm::radians(pitch)) * cos(glm::radians(yaw));

			// 根据目标位置、方向向量和半径计算相机位置
			transComp.position = mTarget + direction * mRadius;

			// 计算视图矩阵：从相机位置看向目标位置
			mViewMat = glm::lookAt(transComp.position, mTarget, mWorldUp);
		}
		return mViewMat;
	}

	/**
	 * @brief 设置视图矩阵
	 * @param viewMat 要设置的视图矩阵
	 * @todo 该函数尚未实现
	 */
	void AdLookAtCameraComponent::SetViewMat(const glm::mat4& viewMat) {

	}
	// 添加角度控制方法
	void AdLookAtCameraComponent::SetYaw(float yaw) {
		// 更新拥有者的变换组件
		AdEntity* owner = GetOwner();
		if (owner && owner->HasComponent<AdTransformComponent>()) {
			auto& transComp = owner->GetComponent<AdTransformComponent>();
			transComp.rotation.x = yaw;
		}
	}

	void AdLookAtCameraComponent::SetPitch(float pitch) {
		pitch = glm::clamp(pitch, -89.0f, 89.0f); // 限制角度
		AdEntity* owner = GetOwner();
		if (owner && owner->HasComponent<AdTransformComponent>()) {
			auto& transComp = owner->GetComponent<AdTransformComponent>();
			transComp.rotation.y = pitch;
		}
	}

	void AdLookAtCameraComponent::AdjustYaw(float delta) {
		AdEntity* owner = GetOwner();
		if (owner && owner->HasComponent<AdTransformComponent>()) {
			auto& transComp = owner->GetComponent<AdTransformComponent>();
			transComp.rotation.x += delta;
		}
	}

	void AdLookAtCameraComponent::AdjustPitch(float delta) {
		AdEntity* owner = GetOwner();
		if (owner && owner->HasComponent<AdTransformComponent>()) {
			auto& transComp = owner->GetComponent<AdTransformComponent>();
			transComp.rotation.y = glm::clamp(transComp.rotation.y + delta, -89.0f, 89.0f);
		}
	}

	// 获取当前角度
	float AdLookAtCameraComponent::GetYaw() const {
		AdEntity* owner = GetOwner();
		if (owner && owner->HasComponent<AdTransformComponent>()) {
			auto& transComp = owner->GetComponent<AdTransformComponent>();
			return transComp.rotation.x;
		}
		return 0.0f;
	}

	float AdLookAtCameraComponent::GetPitch() const {
		AdEntity* owner = GetOwner();
		if (owner && owner->HasComponent<AdTransformComponent>()) {
			auto& transComp = owner->GetComponent<AdTransformComponent>();
			return transComp.rotation.y;
		}
		return 0.0f;
	}

	// AdLookAtCameraComponent.cpp 中实现接口方法
	void AdLookAtCameraComponent::OnMouseMove(float deltaX, float deltaY) {
		// 使用现有的角度控制逻辑
		float yaw = GetYaw();
		float pitch = GetPitch();

		yaw += deltaX * mSensitivity;
		pitch += deltaY * mSensitivity;

		// 限制俯仰角
		pitch = glm::clamp(pitch, -89.0f, 89.0f);

		// 更新角度
		SetYaw(yaw);
		SetPitch(pitch);
	}

	void AdLookAtCameraComponent::OnMouseScroll(float yOffset) {
		// 实现缩放功能
		mRadius -= yOffset * 0.1f;
		mRadius = glm::max(0.1f, mRadius);
	}

	


}