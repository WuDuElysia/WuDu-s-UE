#include<iostream>
#include"Adlog.h"
#include"AdWindow.h"
#include"AdGraphicContext.h"
#include"Graphic/AdVKGraphicContext.h"
#include"Graphic/AdVKDevice.h"
#include"Graphic/AdVKSwapchain.h"

int main() {
	ade::Adlog::Init();
	LOG_T("{0},{1},{2}", __FUNCTION__, 1, 0.14f, true);

	std::unique_ptr<ade::AdWindow> window = ade::AdWindow::Create(800, 600, "sandbox");
	std::unique_ptr<ade::AdGraphicContext> graphicContext = ade::AdGraphicContext::Create(window.get());
	auto vkgraaphicContext = dynamic_cast<ade::AdVKGraphicContext*>(graphicContext.get());
	std::unique_ptr<ade::AdVKDevice> vkdevice = std::make_unique<ade::AdVKDevice>(vkgraaphicContext, 2, 2);
	std::unique_ptr<ade::AdVKSwapchain> vkswapchain = std::make_unique<ade::AdVKSwapchain>(vkgraaphicContext,vkdevice.get());

	while (!window->ShouldClose()) {
		window->PollEvents();
		window->SwapBuffer();
	}
	return EXIT_SUCCESS;
	
}