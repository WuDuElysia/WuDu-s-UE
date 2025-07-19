#ifndef AD_GLFW_WINDOW_H
#define AD_GLFW_WINDOW_H

#include"AdWindow.h"
#include"GLFW/glfw3.h"

namespace ade {
	class AdGlfwWindow : public AdWindow {
	public:
		AdGlfwWindow() = delete;
		AdGlfwWindow(uint32_t width, uint32_t height, const char* title);
		~AdGlfwWindow() override;
		bool ShouldClose() override;
		void PollEvents() override;
		void SwapBuffer() override;
		GLFWwindow* GetGLFWWindow() const;
	private:
		GLFWwindow* mGLFWWindow;
	};
}

#endif
