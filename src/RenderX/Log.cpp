#include "Log.h"
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Lgt {

	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	std::shared_ptr<spdlog::logger>& Log::GetCoreLogger() {
		return s_CoreLogger;
	}
	std::shared_ptr<spdlog::logger>& Log::GetClientLogger() {
		return s_ClientLogger;
	}

	void Log::Init() {
		spdlog::set_pattern("%^[%T] %n: %v%$");

		auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		console_sink->set_level(spdlog::level::trace);

		auto file_sink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("RenderX.log", true);
		file_sink->set_level(spdlog::level::trace);

		s_CoreLogger = std::make_shared<spdlog::logger>("RNDRX", spdlog::sinks_init_list{ console_sink, file_sink });
		spdlog::register_logger(s_CoreLogger);
		s_CoreLogger->set_level(spdlog::level::trace);
		s_CoreLogger->flush_on(spdlog::level::trace);

		s_ClientLogger = std::make_shared<spdlog::logger>("APP", spdlog::sinks_init_list{ console_sink, file_sink });
		spdlog::register_logger(s_ClientLogger);
		s_ClientLogger->set_level(spdlog::level::trace);
		s_ClientLogger->flush_on(spdlog::level::trace);
	}

	void Log::Shutdown() {
		s_ClientLogger.reset();
		s_CoreLogger.reset();
		spdlog::shutdown();
	}

}
