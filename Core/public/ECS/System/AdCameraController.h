// CameraController.h
#pragma once
#include "Event/AdEvent.h"

namespace ade {
        class AdCameraController {
        public:
                virtual ~AdCameraController() = default;

                // ������ƽӿ�
                virtual void OnMouseMove(float deltaX, float deltaY) = 0;
                virtual void OnMouseScroll(float yOffset) = 0;
                virtual void OnMouseClick(int button) = 0;
                virtual void OnMouseRelease(int button) = 0;
                virtual void OnKeyPress(int key) = 0;
                virtual void OnKeyRelease(int key) = 0;
                virtual void Update(float deltaTime) = 0;

                virtual const glm::mat4& GetProjMat() = 0;
                virtual const glm::mat4& GetViewMat() = 0;

                // �����������
                virtual void SetAspect(float aspect) = 0;
                virtual void SetMoveSpeed(float speed) { /* Ĭ�Ͽ�ʵ�� */ }
        };
}