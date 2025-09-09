// EventAdapter.h
#pragma once
#include "AdInputManager.h"
#include <GLFW/glfw3.h>
#include "AdEvent.h"

namespace ade {
        class EventAdapter {
        public:
                static void Initialize(GLFWwindow* window) {
                        // 设置GLFW回调函数，将this指针存储到窗口用户指针中
                        glfwSetWindowUserPointer(window, nullptr);

                        // 设置各种回调
                        glfwSetMouseButtonCallback(window, MouseButtonCallback);
                        glfwSetCursorPosCallback(window, CursorPosCallback);
                        glfwSetKeyCallback(window, KeyCallback);
                        glfwSetWindowSizeCallback(window, WindowSizeCallback);
                        // ... 其他回调
                }

        private:
                // GLFW回调函数 - 静态方法
                static void MouseButtonCallback(GLFWwindow* window, int button, int action, int mods) {
                        double xpos, ypos;
                        glfwGetCursorPos(window, &xpos, &ypos);

                        // 获取输入管理器实例并分发事件
                        auto& inputManager = InputManager::GetInstance();

                        if (action == GLFW_PRESS) {
                                inputManager.QueueEvent<MouseClickEvent>(
                                        static_cast<float>(xpos),
                                        static_cast<float>(ypos),
                                        button,
                                        mods
                                );
                        }
                        else if (action == GLFW_RELEASE) {
                                inputManager.QueueEvent<MouseReleaseEvent>(
                                        static_cast<float>(xpos),
                                        static_cast<float>(ypos),
                                        button,
                                        mods
                                );
                        }
                }

                static void CursorPosCallback(GLFWwindow* window, double xpos, double ypos) {
                        static double lastX = xpos;
                        static double lastY = ypos;

                        float deltaX = static_cast<float>(xpos - lastX);
                        float deltaY = static_cast<float>(ypos - lastY);

                        lastX = xpos;
                        lastY = ypos;

                        auto& inputManager = InputManager::GetInstance();
                        inputManager.QueueEvent<MouseMoveEvent>(
                                static_cast<float>(xpos),
                                static_cast<float>(ypos),
                                deltaX,
                                deltaY
                        );
                }

                static void KeyCallback(GLFWwindow* window, int key, int scancode, int action, int mods) {
                        auto& inputManager = InputManager::GetInstance();

                        if (action == GLFW_PRESS) {
                                inputManager.QueueEvent<KeyPressEvent>(key, mods);
                        }
                        else if (action == GLFW_RELEASE) {
                                inputManager.QueueEvent<KeyReleaseEvent>(key, mods);
                        }
                }

                static void WindowSizeCallback(GLFWwindow* window, int width, int height) {
                        auto& inputManager = InputManager::GetInstance();
                        inputManager.QueueEvent<WindowResizeEvent>(width, height);
                }
        };
}