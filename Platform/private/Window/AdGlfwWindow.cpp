#include"Window/AdGlfwWindow.h"
#include"Adlog.h"
#include"glfw/glfw3.h"
#include"glfw/glfw3native.h"
#include"Event/AdEventAdaper.h"

// �����ռ�ade��ʼ
namespace ade {
	/**
	 * AdGlfwWindow��Ĺ��캯��
	 * @param width ���ڵĿ��
	 * @param height ���ڵĸ߶�
	 * @param title ���ڵı���
	 */
	AdGlfwWindow::AdGlfwWindow(uint32_t width, uint32_t height, const char* title) {
		// ��ʼ��GLFW�⣬���ʧ�����¼��־������
		if (!glfwInit()) {
			LOG_T("faild to init glfw!!!");
			return;
		}

		// ����GLFW������ʾ������API
		glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
		glfwWindowHint(GLFW_VISIBLE, GL_FALSE);

		// ����GLFW���ڣ����ʧ�����¼��־������
		mGLFWWindow = glfwCreateWindow(width, height, title, nullptr, nullptr);
		if (!mGLFWWindow) {
			LOG_T("faild to create glfw window!!!");
			return;
		}
	

		// ��ȡ����ʾ��
		GLFWmonitor* primaryMonitor = glfwGetPrimaryMonitor();
		// �������ʾ������
		if (primaryMonitor) {
			// ��������Դ洢��ʾ���Ĺ�������λ�úʹ�С
			int xPos, yPos, workWidth, workHeight;
			// ��ȡ����ʾ���Ĺ�������λ�úʹ�С
			glfwGetMonitorWorkarea(primaryMonitor, &xPos, &yPos, &workWidth, &workHeight);
			// ������λ������Ϊ������������
			glfwSetWindowPos(mGLFWWindow, workWidth / 2 - width / 2, workHeight / 2 - height / 2);
		}

		// ʹ��ǰ���ڵ������ĳ�Ϊ��ǰ�̵߳���������
		glfwMakeContextCurrent(mGLFWWindow);

		// ��ʾ����
		glfwShowWindow(mGLFWWindow);

		EventAdapter::Initialize(mGLFWWindow);

	}

	GLFWwindow* AdGlfwWindow::GetGLFWWindow() const
	{
		return mGLFWWindow;
	}
	/**
	 * AdGlfwWindow�����������
	 * ����GLFW���ڲ���ֹGLFW��
	 */
	AdGlfwWindow::~AdGlfwWindow() {
		glfwDestroyWindow(mGLFWWindow);
		glfwTerminate();
		LOG_T("The application running end");
	}

	/**
	 * ��鴰���Ƿ�Ӧ�ùر�
	 * @return �������Ӧ�ùر��򷵻�true�����򷵻�false
	 */
	bool AdGlfwWindow::ShouldClose() {
		return glfwWindowShouldClose(mGLFWWindow);
	}

	/**
	 * �������¼�
	 */
	void AdGlfwWindow::PollEvents() {
		glfwPollEvents();
	}

	/**
	 * �������ڻ�����
	 */
	void AdGlfwWindow::SwapBuffer() {
		glfwSwapBuffers(mGLFWWindow);
	}

}