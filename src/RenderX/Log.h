#pragma once
#include <memory>
#include <spdlog/spdlog.h>
#include <spdlog/fmt/ostr.h> // allows logging custom structs via operator<<

namespace RenderX {

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

#define RENDERX_LOG_INIT() ::RenderX ::Log::Init()
#define RENDERX_TRACE(...) ::RenderX ::Log::GetCoreLogger()->trace(__VA_ARGS__)
#define RENDERX_INFO(...) ::RenderX ::Log::GetCoreLogger()->info(__VA_ARGS__)
#define RENDERX_WARN(...) ::RenderX ::Log::GetCoreLogger()->warn(__VA_ARGS__)
#define RENDERX_ERROR(...) ::RenderX ::Log::GetCoreLogger()->error(__VA_ARGS__)
#define RENDERX_CRITICAL(...) ::RenderX ::Log::GetCoreLogger()->critical(__VA_ARGS__)

#define CLIENT_TRACE(...) ::RenderX ::Log::GetClientLogger()->trace(__VA_ARGS__)
#define CLIENT_INFO(...) ::RenderX ::Log::GetClientLogger()->info(__VA_ARGS__)
#define CLIENT_WARN(...) ::RenderX ::Log::GetClientLogger()->warn(__VA_ARGS__)
#define CLIENT_ERROR(...) ::RenderX ::Log::GetClientLogger()->error(__VA_ARGS__)
#define CLIENT_CRITICAL(...) ::RenderX ::Log::GetClientLogger()->critical(__VA_ARGS__)
