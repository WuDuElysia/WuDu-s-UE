#include "AdApplication.h"
#include "AdLog.h"
#include "Render/AdRenderContext.h"
#include "ECS/AdEntity.h"
#include "Event/AdInputManager.h"

namespace WuDu {
	AdAppContext AdApplication::sAppContext{};

	/**
	 * @brief 启动应用程序，初始化日志系统、窗口、渲染上下文等组件
	 * @param argc 命令行参数数量
	 * @param argv 命令行参数数组
	 */
	void AdApplication::Start(int argc, char** argv) {
		Adlog::Init(); // 初始化日志系统

		ParseArgs(argc, argv); // 解析命令行参数
		OnConfiguration(&mAppSettings); // 配置应用程序设置

		// 创建窗口和渲染上下文
		mWindow = AdWindow::Create(mAppSettings.width, mAppSettings.height, mAppSettings.title);
		mRenderContext = std::make_shared<AdRenderContext>(mWindow.get());

		// 设置全局应用上下文
		sAppContext.app = this;
		sAppContext.renderCxt = mRenderContext.get();

		OnInit(); // 调用初始化回调
		LoadScene(); // 加载场景

		mStartTimePoint = std::chrono::steady_clock::now(); // 记录开始时间点
	}

	/**
	 * @brief 停止应用程序并卸载场景、执行销毁操作
	 */
	void AdApplication::Stop() {
		UnLoadScene(); // 卸载当前场景
		OnDestroy(); // 执行销毁回调
	}

	/**
	 * @brief 应用程序主循环，处理事件、更新逻辑和渲染
	 */
	void AdApplication::MainLoop() {
		mLastTimePoint = std::chrono::steady_clock::now(); // 记录上一帧时间点
		while (!mWindow->ShouldClose()) { // 循环直到窗口关闭
			mWindow->PollEvents(); // 处理窗口事件
			
			// 计算帧间隔时间
			float deltaTime = std::chrono::duration<float>(std::chrono::steady_clock::now() - mLastTimePoint).count();
			mLastTimePoint = std::chrono::steady_clock::now();
			mFrameIndex++; // 帧计数器递增

			if (!bPause) { // 如果未暂停，更新游戏逻辑
				OnUpdate(deltaTime);
			}
			OnRender(); // 执行渲染操作

			mWindow->SwapBuffer(); // 交换窗口显示缓冲
		}
	}

	/**
	 * @brief 解析命令行参数，当前为未实现
	 * @param argc 命令行参数数量
	 * @param argv 命令行参数数组
	 */
	void AdApplication::ParseArgs(int argc, char** argv) {

	}

	/**
	 * @brief 加载指定路径的场景文件
	 * @param filePath 场景文件路径，当前未使用
	 * @return 是否加载成功
	 */
	bool AdApplication::LoadScene(const std::string& filePath) {
		if (mScene) { // 如果已有场景，先卸载
			UnLoadScene();
		}
		mScene = std::make_unique<AdScene>(); // 创建新场景
		OnSceneInit(mScene.get()); // 初始化场景回调
		sAppContext.scene = mScene.get(); // 设置全局上下文的场景指针
		return true;
	}

	/**
	 * @brief 卸载当前加载的场景
	 */
	void AdApplication::UnLoadScene() {
		if (mScene) { // 如果存在场景
			OnSceneDestroy(mScene.get()); // 执行场景销毁回调
			mScene.reset(); // 释放场景资源
			sAppContext.scene = nullptr; // 清除全局上下文的场景指针
		}
	}
}