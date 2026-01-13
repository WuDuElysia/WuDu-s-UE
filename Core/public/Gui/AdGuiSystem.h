// AdGuiSystem.h - GUI系统的接口头
#pragma once
#define NOMINMAX
#include "AdEngine.h"
#include "imgui/imgui.h"
#include "AdApplication.h"
#include "AdGuiRenderer.h"
#include "AdGuiEventHandler.h"
#include "AdGuiManager.h"
#include "AdSceneEditor.h"

namespace WuDu {
	class AdGuiSystem {
	public:
		AdGuiSystem() : mGuiVisible(true) {}
		virtual ~AdGuiSystem() = default;

		// 初始化GUI系统
		void OnInit();
		// 渲染GUI
		void OnRender();
		// 销毁资源
		void OnDestroy();
		// 渲染前准备
		void OnBeforeRender();

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
			AdUnlitMaterial* defaultMaterial
		);
		
		// 添加场景编辑器UI
		void AddSceneEditor();

		// 获取系统组件
		AdGuiRenderer* GetRenderer() { return &mRenderer; }
		AdGuiEventHandler* GetEventHandler() { return &mEventHandler; }
		AdGuiManager* GetManager() { return &mManager; }
		AdSceneEditor* GetSceneEditor() { return &mSceneEditor; }

	private:
		// 系统组件
		AdGuiRenderer mRenderer;        // 渲染器
		AdGuiEventHandler mEventHandler; // 事件处理器
		AdGuiManager mManager;          // GUI管理器
		AdSceneEditor mSceneEditor;     // 场景编辑器

		// 系统状态
		bool mGuiVisible;               // GUI是否可见
	};
}