// AdCameraControllerManager.h
#pragma once
#include "ECS/AdEntity.h"
#include "ECS/Component/AdFirstPersonCameraComponent.h"
#include "ECS/Component/AdLookAtCameraComponent.h"

namespace ade {
	class AdCameraControllerManager {
	private:
		AdEntity* m_CameraEntity = nullptr;

		// 使用函数指针而不是类型转换
		std::function<void(float, float)> m_OnMouseMove;
		std::function<void(float)> m_OnMouseScroll;
		std::function<void(float)> m_Update;
		std::function<void(float)> m_SetAspect;

	public:
		AdCameraControllerManager(AdEntity* cameraEntity) : m_CameraEntity(cameraEntity) {
			UpdateActiveController();
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