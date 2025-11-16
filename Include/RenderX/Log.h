#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h> // allows logging custom structs via operator<<

namespace Lgt {

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

#define RENDERX_LOG_INIT() ::Lgt ::Log::Init()
#define RENDERX_TRACE(...) ::Lgt ::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define RENDERX_INFO(...) ::Lgt ::Log::GetCoreLogger()->info(__VA_ARGS__)
#define RENDERX_WARN(...) ::Lgt ::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define RENDERX_ERROR(...) ::Lgt ::Log::GetCoreLogger()->error(__VA_ARGS__)
#define RENDERX_CRITICAL(...) ::Lgt ::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define CLIENT_TRACE(...) ::Lgt ::Log::GetClientLogger()->trace(__VA_ARGS__)
#define CLIENT_INFO(...) ::Lgt ::Log::GetClientLogger()->info(__VA_ARGS__)
#define CLIENT_WARN(...) ::Lgt ::Log::GetClientLogger()->warn(__VA_ARGS__)
#define CLIENT_ERROR(...) ::Lgt ::Log::GetClientLogger()->error(__VA_ARGS__)
#define CLIENT_CRITICAL(...) ::Lgt ::Log::GetClientLogger()->critical(__VA_ARGS__)
