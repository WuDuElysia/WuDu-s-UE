#include "ECS/Component/AdLookAtCameraComponent.h"
#include "ECS/Component/AdTransformComponent.h"

namespace ade {
	/**
	 * @brief 获取投影矩阵
	 * @details 根据相机的视场角、宽高比、近平面和远平面参数计算透视投影矩阵，并对Y轴进行翻转以适应OpenGL坐标系
	 * @return 返回计算得到的投影矩阵的常量引用
	 */
	const glm::mat4& AdLookAtCameraComponent::GetProjMat() {
		mProjMat = glm::perspective(glm::radians(mFov), mAspect, mNearPlane, mFarPlane);
		mProjMat[1][1] *= -1.f;
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
}