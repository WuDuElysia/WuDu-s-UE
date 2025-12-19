// AdCameraControllerManager.h
#pragma once
#include "ECS/AdEntity.h"
#include "ECS/Component/AdFirstPersonCameraComponent.h"
#include "ECS/Component/AdLookAtCameraComponent.h"
#include "Event/AdInputManager.h"
#include "Event/AdEvent.h"
#include "Event/AdEventAdaper.h"

namespace WuDu {
	class AdCameraControllerManager {
	private:
		AdEntity* m_CameraEntity = nullptr;

		// 使用函数指针而不是类型转换
		std::function<void(float, float)> m_OnMouseMove;
		std::function<void(float)> m_OnMouseScroll;
		std::function<void(float)> m_Update;
		std::function<void(float)> m_SetAspect;
		bool m_MouseDragging = false;
		glm::vec2 m_LastMousePos;
		float m_CameraYaw = 0.0f;
		float m_CameraPitch = 0.0f;
		float m_CameraSensitivity = 0.1f;
		float m_CameraRadius = 2.0f;
		float m_LastTime = 0.0f;

	public:
		AdCameraControllerManager(AdEntity* cameraEntity) : m_CameraEntity(cameraEntity) {
			UpdateActiveController();
			SetupCameraControls();
		}

		void SetupCameraControls() {
			WuDu::InputManager::GetInstance().Subscribe<WuDu::MouseClickEvent>(
				[this](WuDu::MouseClickEvent& event) {
					HandleMouseClick(event);
				}
			);

			WuDu::InputManager::GetInstance().Subscribe<WuDu::MouseMoveEvent>(
				[this](WuDu::MouseMoveEvent& event) {
					HandleMouseMove(event);
				}
			);

			WuDu::InputManager::GetInstance().Subscribe<WuDu::MouseReleaseEvent>(
				[this](WuDu::MouseReleaseEvent& event) {
					HandleMouseRelease(event);
				}
			);

			WuDu::InputManager::GetInstance().Subscribe<WuDu::MouseScrollEvent>(
				[this](WuDu::MouseScrollEvent& event) {
					HandleMouseScroll(event);
				}
			);

			WuDu::InputManager::GetInstance().Subscribe<WuDu::KeyPressEvent>(
				[this](WuDu::KeyPressEvent& event) {
					HandleKeyPress(event);
				}
			);

			WuDu::InputManager::GetInstance().Subscribe<WuDu::KeyReleaseEvent>(
				[this](WuDu::KeyReleaseEvent& event) {
					HandleKeyRelease(event);
				}
			);
		}

		void HandleMouseClick(WuDu::MouseClickEvent& event) {
			if (event.GetButton() == GLFW_MOUSE_BUTTON_RIGHT) {
				m_MouseDragging = true;
				m_LastMousePos = event.GetPosition();  // 这里设置起点
			}
		}

		void HandleMouseRelease(WuDu::MouseReleaseEvent& event) {
			if (event.GetButton() == GLFW_MOUSE_BUTTON_RIGHT) {
				m_MouseDragging = false;
			}
		}

		void HandleMouseMove(WuDu::MouseMoveEvent& event) {
			if (m_MouseDragging) {
				glm::vec2 currentPos = event.GetPosition();
				glm::vec2 delta = currentPos - m_LastMousePos;

				// 调用相机控制器处理鼠标移动
				OnMouseMove(delta.x, delta.y);

				m_LastMousePos = currentPos;
			}
		}

		void HandleMouseScroll(WuDu::MouseScrollEvent& event) {

			float delta = event.GetYOffset();
			OnMouseScroll(delta);

		}

		void HandleKeyPress(WuDu::KeyPressEvent& event) {

			WuDu::AdEntity* camera = GetCameraEntity();
			if (camera && camera->HasComponent<WuDu::AdFirstPersonCameraComponent>()) {
				auto& fpCamera = camera->GetComponent<WuDu::AdFirstPersonCameraComponent>();

				switch (event.GetKey()) {
				case GLFW_KEY_W: fpCamera.SetMoveForward(true); break;
				case GLFW_KEY_S: fpCamera.SetMoveBackward(true); break;
				case GLFW_KEY_A: fpCamera.SetMoveLeft(true); break;
				case GLFW_KEY_D: fpCamera.SetMoveRight(true); break;
				case GLFW_KEY_Q: fpCamera.SetMoveDown(true); break;
				case GLFW_KEY_E: fpCamera.SetMoveUp(true); break;
				}
			}

		}

		void HandleKeyRelease(WuDu::KeyReleaseEvent& event) {

			WuDu::AdEntity* camera = GetCameraEntity();
			if (camera && camera->HasComponent<WuDu::AdFirstPersonCameraComponent>()) {
				auto& fpCamera = camera->GetComponent<WuDu::AdFirstPersonCameraComponent>();

				switch (event.GetKey()) {
				case GLFW_KEY_W: fpCamera.SetMoveForward(false); break;
				case GLFW_KEY_S: fpCamera.SetMoveBackward(false); break;
				case GLFW_KEY_A: fpCamera.SetMoveLeft(false); break;
				case GLFW_KEY_D: fpCamera.SetMoveRight(false); break;
				case GLFW_KEY_Q: fpCamera.SetMoveDown(false); break;
				case GLFW_KEY_E: fpCamera.SetMoveUp(false); break;
				}
			}

		}

		void UpdateActiveController() {
			if (!m_CameraEntity) return;

			// 为不同类型的相机设置对应的函数
			if (m_CameraEntity->HasComponent<AdFirstPersonCameraComponent>()) {
				auto& comp = m_CameraEntity->GetComponent<AdFirstPersonCameraComponent>();
				m_OnMouseMove = [&comp](float dx, float dy) { comp.OnMouseMove(dx, dy); };
				m_OnMouseScroll = [&comp](float y) { comp.OnMouseScroll(y); };
				m_Update = [&comp](float dt) { comp.Update(dt); };
				m_SetAspect = [&comp](float a) { comp.SetAspect(a); };
			}
			else if (m_CameraEntity->HasComponent<AdLookAtCameraComponent>()) {
				auto& comp = m_CameraEntity->GetComponent<AdLookAtCameraComponent>();
				m_OnMouseMove = [&comp](float dx, float dy) { comp.OnMouseMove(dx, dy); };
				m_OnMouseScroll = [&comp](float y) { comp.OnMouseScroll(y); };
				m_Update = [](float dt) { /* LookAt相机可能不需要Update */ };
				m_SetAspect = [&comp](float a) { comp.SetAspect(a); };
			}
			else {
				m_OnMouseMove = nullptr;
				m_OnMouseScroll = nullptr;
				m_Update = nullptr;
				m_SetAspect = nullptr;
			}
		}

		AdEntity* GetCameraEntity() const { return m_CameraEntity; }
		void SetCameraEntity(AdEntity* cameraEntity) {
			m_CameraEntity = cameraEntity;
			UpdateActiveController();
		}

		// 代理方法
		void OnMouseMove(float deltaX, float deltaY) {
			if (m_OnMouseMove) {
				m_OnMouseMove(deltaX, deltaY);
			}
		}

		void OnMouseScroll(float yOffset) {
			if (m_OnMouseScroll) {
				m_OnMouseScroll(yOffset);
			}
		}

		void Update(float deltaTime) {
			if (m_Update) {
				m_Update(deltaTime);
			}
		}

		void SetAspect(float aspect) {
			if (m_SetAspect) {
				m_SetAspect(aspect);
			}
		}
	};
}