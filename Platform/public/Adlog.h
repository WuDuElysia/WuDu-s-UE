#ifndef ADLOG_H
#define ADLOG_H

#include"AdEngine.h"
#define SPDLOG_ACTIVE_LEVEL SPDLOG_LEVEL_TRACE
 #include"spdlog/spdlog.h"


namespace ade {
	class Adlog {
	public:
		Adlog() = delete;
		Adlog(const Adlog&) = delete;
		Adlog& operator=(const Adlog&) = delete;
		static void Init();

		static spdlog::logger*  GetLoggerInstance() {
			assert(sLoggerInstance != nullptr && "Logger instance is not initialized!");
			return sLoggerInstance.get();
		}
	private:
		static std::shared_ptr<spdlog::logger> sLoggerInstance;
	};

#define AD_LOG_LOGGER_CALL(adLog, level, ...)\
        (adLog)->log(spdlog::source_loc{__FILE__, __LINE__, SPDLOG_FUNCTION}, level, __VA_ARGS__)

#define LOG_T(...) AD_LOG_LOGGER_CALL(ade::Adlog::GetLoggerInstance(), spdlog::level::trace, __VA_ARGS__)
#define LOG_D(...) AD_LOG_LOGGER_CALL(ade::Adlog::GetLoggerInstance(), spdlog::level::debug, __VA_ARGS__)
#define LOG_I(...) AD_LOG_LOGGER_CALL(ade::Adlog::GetLoggerInstance(), spdlog::level::info, __VA_ARGS__)
#define LOG_W(...) AD_LOG_LOGGER_CALL(ade::Adlog::GetLoggerInstance(), spdlog::level::warn, __VA_ARGS__)
#define LOG_E(...) AD_LOG_LOGGER_CALL(ade::Adlog::GetLoggerInstance(), spdlog::level::err, __VA_ARGS__)
}

#endif