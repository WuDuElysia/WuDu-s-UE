// AdFirstPersonCameraComponent.h
#pragma once
#include "ECS/AdComponent.h"
#include "ECS/Component/AdTransformComponent.h"
#include "ECS/System/AdCameraController.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace ade {
	class AdFirstPersonCameraComponent : public AdComponent, public AdCameraController {
	private:
		// �������
		float mFov = 65.f;
		float mAspect = 1.f;
		float mNearPlane = 0.3f;
		float mFarPlane = 1000.f;

		// �������
		mutable glm::mat4 mProjMat{ 1.f };
		mutable glm::mat4 mViewMat{ 1.f };
		mutable bool mProjMatDirty = true;
		mutable bool mViewMatDirty = true;

		// ������Ʋ���
		float mYaw = -90.0f;    // ƫ����
		float mPitch = 0.0f;    // ������
		float mSensitivity = 0.2f;
		float mMoveSpeed = 3.0f;

		// �ƶ�״̬
		bool mMoveForward = false;
		bool mMoveBackward = false;
		bool mMoveLeft = false;
		bool mMoveRight = false;

		float mRadius{ 4.f };

	public:
		AdFirstPersonCameraComponent() = default;

		// ͶӰ�������
		void SetFov(float fov) { mFov = fov; mProjMatDirty = true; }
		void SetAspect(float aspect)override { mAspect = aspect; mProjMatDirty = true; }
		void SetNearPlane(float nearPlane) { mNearPlane = nearPlane; mProjMatDirty = true; }
		void SetFarPlane(float farPlane) { mFarPlane = farPlane; mProjMatDirty = true; }
		const glm::mat4& GetProjMat()override;

		// ��ͼ�������
		const glm::mat4& GetViewMat()override;

		// �������
		void OnMouseClick(int button)override {};
		void OnMouseRelease(int button)override {};
		void OnKeyPress(int key)override {};
		void OnKeyRelease(int key)override {};
		void OnMouseMove(float deltaX, float deltaY) override;
		void OnMouseScroll(float yOffset)override;

		// �ƶ�����
		void SetMoveForward(bool pressed) { mMoveForward = pressed; }
		void SetMoveBackward(bool pressed) { mMoveBackward = pressed; }
		void SetMoveLeft(bool pressed) { mMoveLeft = pressed; }
		void SetMoveRight(bool pressed) { mMoveRight = pressed; }


		// ����
		void Update(float deltaTime)override;

		// ��ȡ�������
		float GetYaw() const { return mYaw; }
		float GetPitch() const { return mPitch; }
		void SetSensitivity(float sensitivity) { mSensitivity = sensitivity; }
		void SetMoveSpeed(float speed)override { mMoveSpeed = speed; }

	private:
		void UpdateViewMatrix() const;
		void UpdateProjectionMatrix() const;
	};
}