// AdGuiSystem.h - GUI系统的接口头
#pragma once
#define NOMINMAX
#include "AdEngine.h"
#include "imgui/imgui.h"
#include "imgui/imgui_internal.h"
#include "AdApplication.h"
#include "AdGuiRenderer.h"
#include "AdGuiEventHandler.h"
#include "AdGuiManager.h"
#include "AdSceneEditor.h"
#include "AdEditorContext.h"
#include "AdHierarchyPanel.h"
#include "AdInspectorPanel.h"
#include "AdResourceBrowserPanel.h"

namespace WuDu {
	class AdGuiSystem {
	public:
		AdGuiSystem() : mGuiVisible(true) {}
		virtual ~AdGuiSystem() = default;

		// 初始化GUI系统
		void OnInit();
		// 渲染GUI（旧版，独立acquire/present，会闪烁）
		void OnRender();
		// 渲染GUI（新版，录制命令缓冲区，由外部统一提交，不闪烁）
		VkCommandBuffer OnRenderGui(int32_t imageIndex);
		// 销毁资源
		void OnDestroy();
		// 渲染前准备
		void OnBeforeRender();

		// 在渲染循环外部处理延迟纹理加载请求
		void ProcessPendingTextureLoads();

		// 委托给AdGuiManager的方法
		void BeginGui();
		void EndGui();
		void AddGuiFunction(const std::function<void()>& func);
		// 委托给AdGuiRenderer的方法
		void RebuildResources();
		// 委托给AdGuiEventHandler的方法
		bool ProcessInput();

		// 设置场景编辑器资源
		void SetResources(
			AdScene* scene,
			AdEntity* activeCamera,
			AdMesh* cubeMesh,
			AdMaterial* defaultMaterial
		);
		
		// 添加场景编辑器UI
		void AddSceneEditor();

		// 获取系统组件
		AdGuiRenderer* GetRenderer() { return &mRenderer; }
		AdGuiEventHandler* GetEventHandler() { return &mEventHandler; }
		AdGuiManager* GetManager() { return &mManager; }
		AdSceneEditor* GetSceneEditor() { return &mSceneEditor; }
		AdEditorContext* GetEditorContext() { return &mEditorContext; }

	private:
		// 设置全屏 Dockspace 和主菜单栏
		void SetupDockspace();

		// 系统组件
		AdGuiRenderer mRenderer;        // 渲染器
		AdGuiEventHandler mEventHandler; // 事件处理器
		AdGuiManager mManager;          // GUI管理器
		AdSceneEditor mSceneEditor;     // 场景编辑器

		// 编辑器面板系统
		AdEditorContext mEditorContext;              // 编辑器共享上下文
		AdHierarchyPanel mHierarchyPanel;            // 场景层级面板
		AdInspectorPanel mInspectorPanel;            // 检查器面板
		AdResourceBrowserPanel mResourceBrowserPanel; // 资源浏览器面板

		// 系统状态
		bool mGuiVisible;               // GUI是否可见
		bool mFirstTimeDockLayout = true; // 首次布局标志
	};
}