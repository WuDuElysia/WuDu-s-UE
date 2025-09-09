#include "ECS/Component/AdLookAtCameraComponent.h"
#include "ECS/Component/AdTransformComponent.h"

namespace ade {
	/**
	 * @brief ��ȡͶӰ����
	 * @details ����������ӳ��ǡ���߱ȡ���ƽ���Զƽ���������͸��ͶӰ���󣬲���Y����з�ת����ӦOpenGL����ϵ
	 * @return ���ؼ���õ���ͶӰ����ĳ�������
	 */
	const glm::mat4& AdLookAtCameraComponent::GetProjMat() {
		mProjMat = glm::perspective(glm::radians(mFov), mAspect, mNearPlane, mFarPlane);
		mProjMat[1][1] *= -1.f;
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
}