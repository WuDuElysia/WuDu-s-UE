// AdGuiEventHandler.h
#pragma once
#define NOMINMAX
#include "AdEngine.h"
#include "Event/AdInputManager.h"
#include "Event/AdEvent.h"
#include "imgui/imgui.h"

namespace WuDu {
    class AdGuiEventHandler {
    public:
        AdGuiEventHandler();
        ~AdGuiEventHandler() = default;

        // 初始化事件处理
        void OnInit();
        // 清理事件处理
        void OnDestroy();
        // 设置GUI控件
        void SetupGuiControls();
        // 检查是否应该处理输入
        bool ProcessInput();

        // 事件处理函数
        void HandleMouseClick(WuDu::MouseClickEvent& event);
        void HandleMouseRelease(WuDu::MouseReleaseEvent& event);
        void HandleMouseMove(WuDu::MouseMoveEvent& event);
        void HandleMouseScroll(WuDu::MouseScrollEvent& event);

    private:
        // 鼠标状态
        bool m_MouseDragging = false;
        glm::vec2 m_LastMousePos;
    };
}