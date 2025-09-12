// AdGuiEventHandler.cpp
#include "Gui/AdGuiEventHandler.h"
#include "AdApplication.h"
#include "Event/AdInputManager.h"
#include "Event/AdEvent.h"
#include "GLFW/glfw3.h"

namespace ade {
    AdGuiEventHandler::AdGuiEventHandler() {
    }

    void AdGuiEventHandler::OnInit() {
        SetupGuiControls();
    }

    void AdGuiEventHandler::OnDestroy() {
        //// 取消订阅所有事件
        //InputManager::GetInstance().UnsubscribeAll<ade::MouseClickEvent>(this);
        //InputManager::GetInstance().UnsubscribeAll<ade::MouseMoveEvent>(this);
        //InputManager::GetInstance().UnsubscribeAll<ade::MouseReleaseEvent>(this);
        //InputManager::GetInstance().UnsubscribeAll<ade::MouseScrollEvent>(this);
    }

    void AdGuiEventHandler::SetupGuiControls() {
        InputManager::GetInstance().Subscribe<ade::MouseClickEvent>(
            [this](ade::MouseClickEvent& event) {
                HandleMouseClick(event);
            }
        );

        InputManager::GetInstance().Subscribe<ade::MouseMoveEvent>(
            [this](ade::MouseMoveEvent& event) {
                HandleMouseMove(event);
            }
        );

        InputManager::GetInstance().Subscribe<ade::MouseReleaseEvent>(
            [this](ade::MouseReleaseEvent& event) {
                HandleMouseRelease(event);
            }
        );

        InputManager::GetInstance().Subscribe<ade::MouseScrollEvent>(
            [this](ade::MouseScrollEvent& event) {
                HandleMouseScroll(event);
            }
        );
    }

    bool AdGuiEventHandler::ProcessInput() {
        ImGuiIO& io = ImGui::GetIO();
        return io.WantCaptureMouse || io.WantCaptureKeyboard;
    }

    void AdGuiEventHandler::HandleMouseClick(ade::MouseClickEvent& event) {
        ImGuiIO& io = ImGui::GetIO();

        // 将鼠标点击事件转换为ImGui的鼠标按钮索引
        int button = -1;
        if (event.GetButton() == GLFW_MOUSE_BUTTON_LEFT) {
            button = 0;
        }
        else if (event.GetButton() == GLFW_MOUSE_BUTTON_RIGHT) {
            button = 1;
        }
        else if (event.GetButton() == GLFW_MOUSE_BUTTON_MIDDLE) {
            button = 2;
        }

        if (button >= 0) {
            // 更新ImGui的鼠标状态
            io.MouseDown[button] = true;

            // 处理拖动开始
            if (button == 0) {  // 左键点击
                m_MouseDragging = true;
                m_LastMousePos = event.GetPosition();
            }
        }
    }

    // 鼠标释放事件处理
    void AdGuiEventHandler::HandleMouseRelease(ade::MouseReleaseEvent& event) {
        ImGuiIO& io = ImGui::GetIO();

        // 将鼠标释放事件转换为ImGui的鼠标按钮索引
        int button = -1;
        if (event.GetButton() == GLFW_MOUSE_BUTTON_LEFT) {
            button = 0;
        }
        else if (event.GetButton() == GLFW_MOUSE_BUTTON_RIGHT) {
            button = 1;
        }
        else if (event.GetButton() == GLFW_MOUSE_BUTTON_MIDDLE) {
            button = 2;
        }

        if (button >= 0) {
            // 更新ImGui的鼠标状态
            io.MouseDown[button] = false;

            // 处理拖动结束
            if (button == 0) {  // 左键释放
                m_MouseDragging = false;
            }
        }
    }

    //鼠标移动事件处理
    void AdGuiEventHandler::HandleMouseMove(ade::MouseMoveEvent& event) {
        ImGuiIO& io = ImGui::GetIO();
        glm::vec2 currentPos = event.GetPosition();

        // 更新ImGui的鼠标位置
        io.MousePos = ImVec2(currentPos.x, currentPos.y);

        // 处理拖动
        if (m_MouseDragging) {
            glm::vec2 delta = currentPos - m_LastMousePos;

            // 更新ImGui的鼠标增量
            io.MouseDelta.x = delta.x;
            io.MouseDelta.y = delta.y;

            // 处理窗口拖动（如果需要）
            // ImGui会自动处理窗口拖动，我们只需提供正确的输入

            m_LastMousePos = currentPos;
        }
        else {
            // 重置鼠标增量
            io.MouseDelta.x = 0.0f;
            io.MouseDelta.y = 0.0f;
        }
    }

    //鼠标滚轮事件处理
    void AdGuiEventHandler::HandleMouseScroll(ade::MouseScrollEvent& event) {
        ImGuiIO& io = ImGui::GetIO();

        // 更新ImGui的鼠标滚轮状态
        io.MouseWheel += event.GetYOffset();
        io.MouseWheelH += event.GetXOffset();  // 水平滚动
    }
}