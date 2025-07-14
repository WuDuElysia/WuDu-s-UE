#include"Adlog.h"


#include"spdlog/sinks/stdout_color_sinks.h"
#include"spdlog/async.h"

namespace ade {
	std::shared_ptr<spdlog::logger> Adlog::sLoggerInstance{};
	void Adlog::Init() {
		sLoggerInstance = spdlog::stdout_color_mt<spdlog::async_factory>("async_logger");
		sLoggerInstance->set_level(spdlog::level::trace);
		sLoggerInstance->set_pattern("[%H:%M:%S.%e] [%l] %v");
	}
}