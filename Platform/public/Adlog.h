#ifndef ADLOG_H
#define ADLOG_H

#include"AdEngine.h"
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
#include "spdlog/spdlog.h"


namespace ade {
	class Adlog {
	public:
		static void Init();

		static spdlog::logger*  GetLoggerInstance() {
			assert(sLoggerInstance != nullptr && "Logger instance is not initialized!");
			return sLoggerInstance.get();
		}
	private:
		static std::shared_ptr<spdlog::logger> sLoggerInstance;
	};

#define LOG_T(...) SPDLOG_LOGGER_TRACE(ade::Adlog::GetLoggerInstance(), __VA_ARGS__)
}

#endif