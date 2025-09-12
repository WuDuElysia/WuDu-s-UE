#include"Window/AdGlfwWindow.h"
#include"Adlog.h"
#include"glfw/glfw3.h"
#include"glfw/glfw3native.h"
#include"Event/AdEventAdaper.h"

// 命名空间ade开始
namespace ade {
	/**
	 * AdGlfwWindow类的构造函数
	 * @param width 窗口的宽度
	 * @param height 窗口的高度
	 * @param title 窗口的标题
	 */
	AdGlfwWindow::AdGlfwWindow(uint32_t width, uint32_t height, const char* title) {
		// 初始化GLFW库，如果失败则记录日志并返回
		if (!glfwInit()) {
			LOG_T("faild to init glfw!!!");
			return;
		}

		// 设置GLFW窗口提示，禁用API
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

		// 创建GLFW窗口，如果失败则记录日志并返回
		mGLFWWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
		if (!mGLFWWindow) {
			LOG_T("faild to create glfw window!!!");
			return;
		}
	

		// 获取主显示器
		GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
		// 如果主显示器存在
		if (primaryMonitor) {
			// 定义变量以存储显示器的工作区域位置和大小
			int xPos, yPos, workWidth, workHeight;
			// 获取主显示器的工作区域位置和大小
			glfwGetMonitorWorkarea(primaryMonitor, &xPos, &yPos, &workWidth, &workHeight);
			// 将窗口位置设置为工作区域中央
			glfwSetWindowPos(mGLFWWindow, workWidth / 2 - width / 2, workHeight / 2 - height / 2);
		}

		// 使当前窗口的上下文成为当前线程的主上下文
		glfwMakeContextCurrent(mGLFWWindow);

		// 显示窗口
		glfwShowWindow(mGLFWWindow);

		EventAdapter::Initialize(mGLFWWindow);

	}

	GLFWwindow* AdGlfwWindow::GetGLFWWindow() const
	{
		return mGLFWWindow;
	}
	/**
	 * AdGlfwWindow类的析构函数
	 * 销毁GLFW窗口并终止GLFW库
	 */
	AdGlfwWindow::~AdGlfwWindow() {
		glfwDestroyWindow(mGLFWWindow);
		glfwTerminate();
		LOG_T("The application running end");
	}

	/**
	 * 检查窗口是否应该关闭
	 * @return 如果窗口应该关闭则返回true，否则返回false
	 */
	bool AdGlfwWindow::ShouldClose() {
		return glfwWindowShouldClose(mGLFWWindow);
	}

	/**
	 * 处理窗口事件
	 */
	void AdGlfwWindow::PollEvents() {
		glfwPollEvents();
	}

	/**
	 * 交换窗口缓冲区
	 */
	void AdGlfwWindow::SwapBuffer() {
		glfwSwapBuffers(mGLFWWindow);
	}

}