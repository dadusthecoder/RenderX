#include "Common.h"
#include "Log.h"

#include <spdlog/spdlog.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <spdlog/sinks/basic_file_sink.h>

namespace Rx {

	std::shared_ptr<spdlog::logger> Log::s_CoreLogger;
	std::shared_ptr<spdlog::logger> Log::s_ClientLogger;

	std::shared_ptr<spdlog::logger>& Log::Core()   { return s_CoreLogger; }
	std::shared_ptr<spdlog::logger>& Log::Client() { return s_ClientLogger; }

	void Log::Init()
	{
		auto consoleSink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
		consoleSink->set_level(spdlog::level::info);

		auto fileSink = std::make_shared<spdlog::sinks::basic_file_sink_mt>("RenderX.log", true);
		fileSink->set_level(spdlog::level::trace);

		s_CoreLogger = std::make_shared<spdlog::logger>(
			"RENDERX",
			spdlog::sinks_init_list{ consoleSink, fileSink }
		);

		s_CoreLogger->set_pattern("%^[%T] [%n] %v%$");
		s_CoreLogger->set_level(spdlog::level::trace);
		s_CoreLogger->flush_on(spdlog::level::warn);

		spdlog::register_logger(s_CoreLogger);

		s_ClientLogger = std::make_shared<spdlog::logger>(
			"APP",
			spdlog::sinks_init_list{ consoleSink, fileSink }
		);

		s_ClientLogger->set_pattern("%^[%T] [%n] %v%$");
		s_ClientLogger->set_level(spdlog::level::trace);
		s_ClientLogger->flush_on(spdlog::level::warn);

		spdlog::register_logger(s_ClientLogger);
	}

	void Log::Shutdown()
	{
		if (s_CoreLogger) s_CoreLogger->flush();
		if (s_ClientLogger) s_ClientLogger->flush();

		s_CoreLogger.reset();
		s_ClientLogger.reset();

		spdlog::shutdown();
	}

	void Log::Status(const std::string& msg)
	{
		if (!s_CoreLogger)
			return;

		auto& sinks = s_CoreLogger->sinks();
		if (sinks.empty())
			return;

		spdlog::details::log_msg logMsg;
		logMsg.payload = "\r" + msg;

		sinks[0]->log(logMsg); // console only
		sinks[0]->flush();
	}

} // namespace Rx
