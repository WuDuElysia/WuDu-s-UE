#include "ECS/Component/AdLookAtCameraComponent.h"
#include "ECS/Component/AdTransformComponent.h"

namespace ade {
	/**
	 * @brief ��ȡͶӰ����
	 * @details ����������ӳ��ǡ���߱ȡ���ƽ���Զƽ���������͸��ͶӰ��������Vulkan����ϵ
	 * @return ���ؼ���õ���ͶӰ����ĳ�������
	 */
	const glm::mat4& AdLookAtCameraComponent::GetProjMat() {
		// ʹ���ʺ�Vulkan��ͶӰ����
		mProjMat = glm::perspectiveRH_ZO(glm::radians(mFov), mAspect, mNearPlane, mFarPlane);
		// GLM��RH_ZO�汾�Ѿ�������Vulkan������ϵ(Y�����£����[0,1])
		return mProjMat;
	}

	/**
	 * @brief ��ȡ��ͼ����
	 * @details ���������Ŀ��λ�á��ǶȺͰ뾶�������λ�ã���������ͼ����
	 *          ���λ��ͨ��������ת��Ϊ�ѿ����������ó���ȷ�����ʼ�տ���Ŀ��㡣
	 * @return ���ؼ���õ�����ͼ����ĳ�������
	 */
	const glm::mat4& AdLookAtCameraComponent::GetViewMat() {
		AdEntity* owner = GetOwner();
		if (AdEntity::HasComponent<AdTransformComponent>(owner)) {
			auto& transComp = owner->GetComponent<AdTransformComponent>();
			float yaw = transComp.rotation.x;
			float pitch = transComp.rotation.y;

			// ����ƫ���Ǻ͸����Ǽ��������������
			glm::vec3 direction;
			direction.x = cos(glm::radians(pitch)) * sin(glm::radians(yaw));
			direction.y = sin(glm::radians(pitch));
			direction.z = cos(glm::radians(pitch)) * cos(glm::radians(yaw));

			// ����Ŀ��λ�á����������Ͱ뾶�������λ��
			transComp.position = mTarget + direction * mRadius;

			// ������ͼ���󣺴����λ�ÿ���Ŀ��λ��
			mViewMat = glm::lookAt(transComp.position, mTarget, mWorldUp);
		}
		return mViewMat;
	}

	/**
	 * @brief ������ͼ����
	 * @param viewMat Ҫ���õ���ͼ����
	 * @todo �ú�����δʵ��
	 */
	void AdLookAtCameraComponent::SetViewMat(const glm::mat4& viewMat) {

	}
	// ��ӽǶȿ��Ʒ���
	void AdLookAtCameraComponent::SetYaw(float yaw) {
		// ����ӵ���ߵı任���
		AdEntity* owner = GetOwner();
		if (owner && owner->HasComponent<AdTransformComponent>()) {
			auto& transComp = owner->GetComponent<AdTransformComponent>();
			transComp.rotation.x = yaw;
		}
	}

	void AdLookAtCameraComponent::SetPitch(float pitch) {
		pitch = glm::clamp(pitch, -89.0f, 89.0f); // ���ƽǶ�
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

	// ��ȡ��ǰ�Ƕ�
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

	// AdLookAtCameraComponent.cpp ��ʵ�ֽӿڷ���
	void AdLookAtCameraComponent::OnMouseMove(float deltaX, float deltaY) {
		// ʹ�����еĽǶȿ����߼�
		float yaw = GetYaw();
		float pitch = GetPitch();

		yaw += deltaX * mSensitivity;
		pitch += deltaY * mSensitivity;

		// ���Ƹ�����
		pitch = glm::clamp(pitch, -89.0f, 89.0f);

		// ���½Ƕ�
		SetYaw(yaw);
		SetPitch(pitch);
	}

	void AdLookAtCameraComponent::OnMouseScroll(float yOffset) {
		// ʵ�����Ź���
		mRadius -= yOffset * 0.1f;
		mRadius = glm::max(0.1f, mRadius);
	}

	


}