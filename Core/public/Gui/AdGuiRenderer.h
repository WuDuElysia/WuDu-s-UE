// AdGuiRenderer.h
#pragma once
#define NOMINMAX
#include "AdEngine.h"
#include "Graphic/AdVKDescriptorSet.h"
#include "Graphic/AdVKCommandBuffer.h"
#include "Render/AdRenderTarget.h"
#include "Render/AdRenderer.h"
#include "Graphic/AdVKPipeline.h"
#include "imgui/imgui.h"
#include "imgui/backends/imgui_impl_vulkan.h"
#include "imgui/backends/imgui_impl_glfw.h"

namespace ade {
    class AdGuiRenderer {
    public:
        AdGuiRenderer();
        ~AdGuiRenderer() = default;

        // 初始化渲染资源
        void OnInit();
        // 渲染ImGui内容
        void OnRender();
        // 清理渲染资源
        void OnDestroy();
        // 重建资源（窗口大小变化时调用）
        void RebuildResources();
        // 创建GUI渲染通道
        void CreateGUIRenderPass();

        // 获取渲染器
        std::shared_ptr<AdRenderer> GetRenderer() const { return mGUIRenderer; }
        // 获取渲染目标
        std::shared_ptr<AdRenderTarget> GetRenderTarget() const { return mGUIRenderTarget; }
        // 获取渲染通道
        std::shared_ptr<AdVKRenderPass> GetRenderPass() const { return mGUIRenderPass; }
	//获取描述符池
	std::shared_ptr<AdVKDescriptorPool> GetDescriptorPool() const { return mImGuiDescriptorPool; }

    private:
        std::shared_ptr<AdVKDescriptorPool> mImGuiDescriptorPool;
        std::shared_ptr<AdVKRenderPass> mGUIRenderPass;    // GUI专用渲染通道
        std::shared_ptr<AdRenderTarget> mGUIRenderTarget;   // GUI专用渲染目标
        std::shared_ptr<AdRenderer> mGUIRenderer;          // GUI渲染器
        std::vector<VkCommandBuffer> mGUICmdBuffers;        // GUI命令缓冲区数组
    };
}