// AdFirstPersonCameraComponent.cpp
#include "ECS/Component/AdFirstPersonCameraComponent.h"
#include "AdLog.h"

namespace ade {
	const glm::mat4& AdFirstPersonCameraComponent::GetProjMat() {
		if (mProjMatDirty) {
			// 确保使用正确的 Vulkan 投影矩阵
			mProjMat = glm::perspectiveRH_ZO(
				glm::radians(mFov),
				mAspect,           // 确保这个值正确设置
				mNearPlane,
				mFarPlane
			);
			mProjMatDirty = false;
			UpdateProjectionMatrix();
		}
		return mProjMat;
	}

	void AdFirstPersonCameraComponent::UpdateProjectionMatrix() const {
		mProjMat = glm::perspectiveRH_ZO(
			glm::radians(mFov),
			mAspect,
			mNearPlane,
			mFarPlane
		);
		mProjMatDirty = false;
	}

	const glm::mat4& AdFirstPersonCameraComponent::GetViewMat() {
		if (mViewMatDirty) {

			UpdateViewMatrix();
		}
		return mViewMat;
	}

	void AdFirstPersonCameraComponent::UpdateViewMatrix() const {
		AdEntity* owner = GetOwner();
		if (AdEntity::HasComponent<AdTransformComponent>(owner)) {
			auto& transComp = owner->GetComponent<AdTransformComponent>();

			// 计算相机的前方向向量
			glm::vec3 front;
			front.x = cos(glm::radians(mYaw)) * cos(glm::radians(mPitch));
			front.y = sin(glm::radians(mPitch));
			front.z = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
			front = glm::normalize(front);

			// 计算右方向和上方向向量
			glm::vec3 worldUp(0.0f, 1.0f, 0.0f);
			glm::vec3 right = glm::normalize(glm::cross(front, worldUp));
			glm::vec3 up = glm::normalize(glm::cross(right, front));

			// 计算视图矩阵
			mViewMat = glm::lookAt(transComp.position, transComp.position + front, up);
			mViewMatDirty = false;
		}
	}

	void AdFirstPersonCameraComponent::OnMouseMove(float deltaX, float deltaY) {
		mYaw += deltaX * mSensitivity;
		mPitch -= deltaY * mSensitivity;

		// 限制俯仰角
		if (mPitch > 89.0f) mPitch = 89.0f;
		if (mPitch < -89.0f) mPitch = -89.0f;

		mViewMatDirty = true;
	}

	void AdFirstPersonCameraComponent::OnMouseScroll(float yOffset) {
		mMoveSpeed += yOffset * 0.5f;
		mMoveSpeed = glm::max(0.1f, mMoveSpeed);
	}

	void AdFirstPersonCameraComponent::Update(float deltaTime) {
		AdEntity* owner = GetOwner();
		if (!owner || !owner->HasComponent<AdTransformComponent>()) {
			return;
		}

		auto& transComp = owner->GetComponent<AdTransformComponent>();

		// 计算相机的前、右方向向量
		glm::vec3 front;
		front.x = cos(glm::radians(mYaw)) * cos(glm::radians(mPitch));
		front.y = sin(glm::radians(mPitch));
		front.z = sin(glm::radians(mYaw)) * cos(glm::radians(mPitch));
		front = glm::normalize(front);

		glm::vec3 right = glm::normalize(glm::cross(front, glm::vec3(0.0f, 1.0f, 0.0f)));

		// 根据按键状态移动相机
		float velocity = mMoveSpeed * deltaTime;

		if (mMoveForward) {
			transComp.position += front * velocity;
		}
		if (mMoveBackward) {
			transComp.position -= front * velocity;
		}
		if (mMoveLeft) {
			transComp.position -= right * velocity;
		}
		if (mMoveRight) {
			transComp.position += right * velocity;
		}

		mViewMatDirty = true;
	}
}