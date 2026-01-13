// AdFirstPersonCameraComponent.h
#pragma once
#include "ECS/AdComponent.h"
#include "ECS/Component/AdTransformComponent.h"
#include "ECS/System/AdCameraController.h"
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>

namespace WuDu {
	class AdFirstPersonCameraComponent : public AdComponent, public AdCameraController {
	private:
		// 相机参数
		float mFov = 65.f;
		float mAspect = 1.f;
		float mNearPlane = 0.3f;
		float mFarPlane = 1000.f;

		// 矩阵缓存
		mutable glm::mat4 mProjMat{ 1.f };
		mutable glm::mat4 mViewMat{ 1.f };
		mutable bool mProjMatDirty = true;
		mutable bool mViewMatDirty = true;

		// 相机姿态参数
		float mYaw = -90.0f;    // 偏航角
		float mPitch = 0.0f;    // 俯仰角
		float mSensitivity = 0.2f;
		float mMoveSpeed = 3.0f;

		// 移动状态
		bool mMoveForward = false;
		bool mMoveBackward = false;
		bool mMoveLeft = false;
		bool mMoveRight = false;
		bool mMoveUp = false;
		bool mMoveDown = false;

		float mRadius{ 4.f };

	public:
		AdFirstPersonCameraComponent() = default;

		// 投影矩阵设置
		void SetFov(float fov) { mFov = fov; mProjMatDirty = true; }
		void SetAspect(float aspect)override { mAspect = aspect; mProjMatDirty = true; }
		void SetNearPlane(float nearPlane) { mNearPlane = nearPlane; mProjMatDirty = true; }
		void SetFarPlane(float farPlane) { mFarPlane = farPlane; mProjMatDirty = true; }
		const glm::mat4& GetProjMat()override;

		// 从屏幕坐标反投影
		void Unproject(float ndcX, float ndcY, glm::vec3& outOrigin, glm::vec3& outDirection) const;

		// 视图矩阵获取
		const glm::mat4& GetViewMat()override;

		// 输入处理
		void OnMouseClick(int button)override {};
		void OnMouseRelease(int button)override {};
		void OnKeyPress(int key)override {};
		void OnKeyRelease(int key)override {};
		void OnMouseMove(float deltaX, float deltaY) override;
		void OnMouseScroll(float yOffset)override;

		// 移动控制
		void SetMoveForward(bool pressed) { mMoveForward = pressed; }
		void SetMoveBackward(bool pressed) { mMoveBackward = pressed; }
		void SetMoveLeft(bool pressed) { mMoveLeft = pressed; }
		void SetMoveRight(bool pressed) { mMoveRight = pressed; }
		void SetMoveUp(bool pressed) { mMoveUp = pressed; }    //E
		void SetMoveDown(bool pressed) { mMoveDown = pressed; }    //Q


		// 更新
		void Update(float deltaTime)override;

		// 获取相机参数
		float GetYaw() const { return mYaw; }
		float GetPitch() const { return mPitch; }
		void SetSensitivity(float sensitivity) { mSensitivity = sensitivity; }
		void SetMoveSpeed(float speed)override { mMoveSpeed = speed; }

	private:
		void UpdateViewMatrix() const;
		void UpdateProjectionMatrix() const;
	};
}