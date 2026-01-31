#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h> // allows logging custom structs via operator<<

namespace Rx {

	class Log {
	public:
		static void Init();
		static void Shutdown();
		static std::shared_ptr<spdlog::logger>& GetCoreLogger();
		static std::shared_ptr<spdlog::logger>& GetClientLogger();
	private:
		static std::shared_ptr<spdlog::logger> s_CoreLogger;
		static std::shared_ptr<spdlog::logger> s_ClientLogger;
	};

}

#ifdef  RENDERX_DEBUG
#define RENDERX_LOG_INIT() ::Rx::Log::Init()
#define RENDERX_TRACE(...) ::Rx::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define RENDERX_INFO(...) ::Rx::Log::GetCoreLogger()->info(__VA_ARGS__)
#define RENDERX_WARN(...) ::Rx::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define RENDERX_ERROR(...) ::Rx::Log::GetCoreLogger()->error(__VA_ARGS__)
#define RENDERX_CRITICAL(...) ::Rx::Log::GetCoreLogger()->critical(__VA_ARGS__)
#else 
#define RENDERX_LOG_INIT()
#define RENDERX_TRACE(...)
#define RENDERX_INFO(...) 
#define RENDERX_WARN(...) 
#define RENDERX_ERROR(...)
#define RENDERX_CRITICAL(...)
#endif


#define CLIENT_TRACE(...) ::Rx::Log::GetClientLogger()->trace(__VA_ARGS__)
#define CLIENT_INFO(...) ::Rx::Log::GetClientLogger()->info(__VA_ARGS__)
#define CLIENT_WARN(...) ::Rx::Log::GetClientLogger()->warn(__VA_ARGS__)
#define CLIENT_ERROR(...) ::Rx::Log::GetClientLogger()->error(__VA_ARGS__)
#define CLIENT_CRITICAL(...) ::Rx::Log::GetClientLogger()->critical(__VA_ARGS__)
