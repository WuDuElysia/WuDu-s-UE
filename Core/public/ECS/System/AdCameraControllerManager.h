// AdCameraControllerManager.h
#pragma once
#include "ECS/AdEntity.h"
#include "ECS/Component/AdFirstPersonCameraComponent.h"
#include "ECS/Component/AdLookAtCameraComponent.h"

namespace ade {
	class AdCameraControllerManager {
	private:
		AdEntity* m_CameraEntity = nullptr;

		// ʹ�ú���ָ�����������ת��
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

			// Ϊ��ͬ���͵�������ö�Ӧ�ĺ���
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
				m_Update = [](float dt) { /* LookAt������ܲ���ҪUpdate */ };
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

		// ������
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