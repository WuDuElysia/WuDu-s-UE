// AdCameraControllerManager.h
#pragma once
#include "ECS/AdEntity.h"
#include "ECS/Component/AdFirstPersonCameraComponent.h"
#include "ECS/Component/AdLookAtCameraComponent.h"
#include "Event/AdInputManager.h"
#include "Event/AdEvent.h"
#include "Event/AdEventAdaper.h"

namespace ade {
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
			ade::InputManager::GetInstance().Subscribe<ade::MouseClickEvent>(
				[this](ade::MouseClickEvent& event) {
					HandleMouseClick(event);
				}
			);

			ade::InputManager::GetInstance().Subscribe<ade::MouseMoveEvent>(
				[this](ade::MouseMoveEvent& event) {
					HandleMouseMove(event);
				}
			);

			ade::InputManager::GetInstance().Subscribe<ade::MouseReleaseEvent>(
				[this](ade::MouseReleaseEvent& event) {
					HandleMouseRelease(event);
				}
			);

			ade::InputManager::GetInstance().Subscribe<ade::MouseScrollEvent>(
				[this](ade::MouseScrollEvent& event) {
					HandleMouseScroll(event);
				}
			);

			ade::InputManager::GetInstance().Subscribe<ade::KeyPressEvent>(
				[this](ade::KeyPressEvent& event) {
					HandleKeyPress(event);
				}
			);

			ade::InputManager::GetInstance().Subscribe<ade::KeyReleaseEvent>(
				[this](ade::KeyReleaseEvent& event) {
					HandleKeyRelease(event);
				}
			);
		}

		void HandleMouseClick(ade::MouseClickEvent& event) {
			if (event.GetButton() == GLFW_MOUSE_BUTTON_LEFT) {
				m_MouseDragging = true;
				m_LastMousePos = event.GetPosition();  // 这里设置起点
			}
		}

		void HandleMouseRelease(ade::MouseReleaseEvent& event) {
			if (event.GetButton() == GLFW_MOUSE_BUTTON_LEFT) {
				m_MouseDragging = false;
			}
		}

		void HandleMouseMove(ade::MouseMoveEvent& event) {
			if (m_MouseDragging) {
				glm::vec2 currentPos = event.GetPosition();
				glm::vec2 delta = currentPos - m_LastMousePos;

				// 调用相机控制器处理鼠标移动
				OnMouseMove(delta.x, delta.y);

				m_LastMousePos = currentPos;
			}
		}

		void HandleMouseScroll(ade::MouseScrollEvent& event) {

			float delta = event.GetYOffset();
			OnMouseScroll(delta);

		}

		void HandleKeyPress(ade::KeyPressEvent& event) {

			ade::AdEntity* camera = GetCameraEntity();
			if (camera && camera->HasComponent<ade::AdFirstPersonCameraComponent>()) {
				auto& fpCamera = camera->GetComponent<ade::AdFirstPersonCameraComponent>();

				switch (event.GetKey()) {
				case GLFW_KEY_W: fpCamera.SetMoveForward(true); break;
				case GLFW_KEY_S: fpCamera.SetMoveBackward(true); break;
				case GLFW_KEY_A: fpCamera.SetMoveLeft(true); break;
				case GLFW_KEY_D: fpCamera.SetMoveRight(true); break;
				}
			}

		}

		void HandleKeyRelease(ade::KeyReleaseEvent& event) {

			ade::AdEntity* camera = GetCameraEntity();
			if (camera && camera->HasComponent<ade::AdFirstPersonCameraComponent>()) {
				auto& fpCamera = camera->GetComponent<ade::AdFirstPersonCameraComponent>();

				switch (event.GetKey()) {
				case GLFW_KEY_W: fpCamera.SetMoveForward(false); break;
				case GLFW_KEY_S: fpCamera.SetMoveBackward(false); break;
				case GLFW_KEY_A: fpCamera.SetMoveLeft(false); break;
				case GLFW_KEY_D: fpCamera.SetMoveRight(false); break;
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