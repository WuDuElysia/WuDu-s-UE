// AdGuiManager.h
#pragma once
#define NOMINMAX
#include "AdEngine.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_glfw.h"

namespace ade {
    class AdGuiManager {
    public:
        AdGuiManager();
        ~AdGuiManager() = default;

        // 初始化GUI管理器
        void OnInit();
        // 开始GUI帧
        void BeginGui();
        // 结束GUI帧
        void EndGui();
        // 添加GUI函数
        void AddGuiFunction(const std::function<void()>& func);
        // 清理GUI资源
        void OnDestroy();

        // 设置GUI可见性
        void SetVisible(bool visible) { mGuiVisible = visible; }
        // 获取GUI可见性
        bool IsVisible() const { return mGuiVisible; }

    private:
        bool mGuiVisible; // GUI是否可见
        std::vector<std::function<void()>> mGuiFunctions; // 存储所有GUI函数
    };
}