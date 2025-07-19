#include<iostream>
#include"Adlog.h"
#include"AdWindow.h"
#include"AdGraphicContext.h"

int main() {
	ade::Adlog::Init();
	LOG_T("{0},{1},{2}", __FUNCTION__, 1, 0.14f, true);

	std::unique_ptr<ade::AdWindow> window = ade::AdWindow::Create(800, 600, "sandbox");
	std::unique_ptr<ade::AdGraphicContext> graphicContext = ade::AdGraphicContext::Create(window.get());
	while (!window->ShouldClose()) {
		window->PollEvents();
		window->SwapBuffer();
	}
	return EXIT_SUCCESS;
}