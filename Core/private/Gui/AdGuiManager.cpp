// AdGuiManager.cpp
#include "Gui/AdGuiManager.h"
#include "AdApplication.h"
#include "Graphic/AdVKRenderPass.h"
#include "Render/AdRenderContext.h"
#include <Window/AdGlfwWindow.h>

namespace ade {
    AdGuiManager::AdGuiManager() : mGuiVisible(true) {
    }

    void AdGuiManager::OnInit() {
        // 初始化ImGui上下文
        IMGUI_CHECKVERSION();
        ImGui::CreateContext();
        ImGuiIO& io = ImGui::GetIO();
        io.ConfigFlags |= ImGuiConfigFlags_NavEnableKeyboard;
        io.ConfigFlags |= ImGuiConfigFlags_DockingEnable;
        io.ConfigFlags |= ImGuiConfigFlags_ViewportsEnable; // 允许窗口脱离主窗口
        io.ConfigViewportsNoAutoMerge = true;  // 防止窗口自动合并
        io.ConfigViewportsNoTaskBarIcon = false;

        // 使用窗口实际大小
        ade::AdApplication* appCxt = AdApplication::GetAppContext()->app;
        AdGlfwWindow* window = static_cast<AdGlfwWindow*>(appCxt->GetWindow());
        
        // 获取交换链和设置显示大小
        ade::AdRenderContext* renderCxt = AdApplication::GetAppContext()->renderCxt;
        ade::AdVKSwapchain* swapchain = renderCxt->GetSwapchain();
        io.DisplaySize = ImVec2(static_cast<float>(swapchain->GetWidth()), static_cast<float>(swapchain->GetHeight()));
        
        // 设置ImGui样式
        ImGui::StyleColorsDark();
        
        // 初始化GLFW后端
        ImGui_ImplGlfw_InitForVulkan(window->GetGLFWWindow(), true);
    }

    void AdGuiManager::BeginGui() {
        // 开始新的ImGui帧
        ImGui_ImplVulkan_NewFrame();      // Vulkan后端先调用
        ImGui_ImplGlfw_NewFrame();        // GLFW后端后调用
        ImGui::NewFrame();

        // 执行所有注册的GUI函数
        for (auto& guiFunc : mGuiFunctions) {
            guiFunc();
        }
    }

    void AdGuiManager::EndGui() {
        // 结束ImGui帧并准备渲染
        ImGui::Render();
    }

    void AdGuiManager::AddGuiFunction(const std::function<void()>& func) {
        // 添加GUI函数到列表中
        mGuiFunctions.push_back(func);
    }

    void AdGuiManager::OnDestroy() {
        // 关闭ImGui GLFW后端
        ImGui_ImplGlfw_Shutdown();

        // 清空GUI函数列表
        mGuiFunctions.clear();

        // 最后销毁ImGui上下文
        ImGui::DestroyContext();
    }
}