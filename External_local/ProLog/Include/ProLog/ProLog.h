#pragma once

#if defined(_WIN32) || defined(_WIN64)
#define PROLOG_PLATFORM_WINDOWS
#elif defined(__APPLE__) || defined(__MACH__)
#define PROLOG_PLATFORM_MACOS
#elif defined(__linux__)
#define PROLOG_PLATFORM_LINUX
#else
#define PROLOG_PLATFORM_UNKNOWN
#endif

//------------------------------------------------------------------------------
// EXPORT / IMPORT MACROS
//------------------------------------------------------------------------------

#if defined(PROLOG_STATIC)
#define PROLOG_API
#else
#if defined(PROLOG_PLATFORM_WINDOWS)
#if defined(PROLOG_BUILD_DLL)
#define PROLOG_API __declspec(dllexport)
#else
#define PROLOG_API
#endif
#elif defined(__GNUC__) || defined(__clang__)
#define PROLOG_API __attribute__((visibility("default")))
#else
#define PROLOG_API
#pragma warning Unknown dynamic link import / export semantics.
#endif
#endif


#include <spdlog/spdlog.h>
#include <spdlog/sinks/basic_file_sink.h>
#include <spdlog/sinks/rotating_file_sink.h>
#include <spdlog/sinks/stdout_color_sinks.h>
#include <chrono>
#include <string>
#include <memory>
#include <unordered_map>
#include <mutex>
#include <fstream>
#include <vector>
#include <thread>

namespace ProLog {


	// ============================================================================
	// Forward Declarations
	// ============================================================================
	struct ProfileResult;
	struct ProfileStatistics;
	class ProfilerSession;
	class Timer;
	class Logger;
	class PerformanceMarker;
	// ============================================================================
	// Configuration
	// ============================================================================
	struct PROLOG_API ProfilerConfig {
		bool enableProfiling = true;
		bool enableLogging = true;
		size_t bufferSize = 1000;
		bool autoFlush = false;
		std::string outputFormat = "chrome";
	};

	// ============================================================================
	// Profile Result Structures
	// ============================================================================
	struct PROLOG_API ProfileResult {
		std::string name;
		std::string category;
		long long start;
		long long duration;
		std::thread::id threadID;
		size_t depth;
		std::unordered_map<std::string, std::string> metadata;
	};

	struct PROLOG_API ProfileStatistics {
		std::string functionName;
		size_t callCount = 0;
		long long totalDuration = 0;
		long long minDuration = LLONG_MAX;
		long long maxDuration = 0;
		double avgDuration = 0.0;
	};

	// ============================================================================
	// Profiler Session
	// ============================================================================
	class PROLOG_API ProfilerSession {
	public:
		static ProfilerSession& Get();

		void BeginSession(const std::string& name, const std::string& filepath = "profile_results.json");
		void EndSession();
		void WriteProfile(const ProfileResult& result);
		void SetConfig(const ProfilerConfig& config);
		std::unordered_map<std::string, ProfileStatistics> GetStatistics();
		void PrintStatistics();
	private:
		ProfilerSession() = default;
		~ProfilerSession();
		ProfilerSession(const ProfilerSession&) = delete;
		ProfilerSession& operator=(const ProfilerSession&) = delete;

		void WriteHeader();
		void WriteFooter();
		void FlushBuffer();
		void WriteStatisticsFile();

		std::string m_CurrentSession;
		std::ofstream m_OutputStream;
		int m_ProfileCount = 0;
		std::mutex m_Mutex;
		ProfilerConfig m_Config;
		std::unordered_map<std::string, ProfileStatistics> m_Statistics;
		std::vector<ProfileResult> m_Buffer;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTime;
	};

	// ============================================================================
	// Timer
	// ============================================================================
	class PROLOG_API Timer {
	public:
		Timer(const char* name, const char* category = "function");
		~Timer();

		void Stop();
		void AddMetadata(const std::string& key, const std::string& value);
	private:
		const char* m_Name;
		const char* m_Category;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimepoint;
		bool m_Stopped;
		size_t m_Depth;
		std::unordered_map<std::string, std::string> m_Metadata;
	};

	// ============================================================================
	// Logger
	// ============================================================================
	class PROLOG_API Logger {
	public:
		static Logger& Get();

		void Init(const std::string& logFile = "application.log",
			size_t maxSize = 1024 * 1024 * 5,
			size_t maxFiles = 3);

		std::shared_ptr<spdlog::logger>& GetLogger();
		void SetLevel(spdlog::level::level_enum level);
		void Flush();
	private:
		Logger() = default;
		Logger(const Logger&) = delete;
		Logger& operator=(const Logger&) = delete;

		std::shared_ptr<spdlog::logger> m_Logger;
	};

	// ============================================================================
	// Performance Marker
	// ============================================================================
	class PROLOG_API PerformanceMarker {
	public:
		static void BeginEvent(const std::string& name, const std::string& category = "marker");
		static void EndEvent(const std::string& name);
	private:
		static std::unordered_map<std::string, std::unique_ptr<Timer>> s_ActiveMarkers;
		static std::mutex s_Mutex;
	};

	static void SetConfig(ProfilerConfig config) {
		ProLog::ProfilerSession::Get().SetConfig(config);
	}

} // namespace Profiler

// ============================================================================
// Feature Toggles
// ============================================================================
#define ENABLE_PROFILING 0 // set to 0 to disable profiling
#define ENABLE_LOGGING 0  // set to 0 to disable logging
// ============================================================================
// Profiling Macros
// ============================================================================

#if ENABLE_PROFILING

#define PROFILE_SCOPE(name) ProLog::Timer timer##__LINE__(name)
#define PROFILE_SCOPE_CAT(name, category) ProLog::Timer timer##__LINE__(name, category)
#define PROFILE_FUNCTION() PROFILE_SCOPE(__FUNCTION__)
#define PROFILE_FUNCTION_CAT(category) PROFILE_SCOPE_CAT(__FUNCTION__, category)

#define PROFILE_BEGIN(name) ProLog::PerformanceMarker::BeginEvent(name)
#define PROFILE_END(name) ProLog::PerformanceMarker::EndEvent(name)

#define PROFILE_START_SESSION(name, filepath) \
	ProLog::ProfilerSession::Get().BeginSession(name, filepath)

#define PROFILE_END_SESSION() \
	ProLog::ProfilerSession::Get().EndSession()

#define PROFILE_PRINT_STATS() \
	ProLog::ProfilerSession::Get().PrintStatistics()

#else

// Compile out completely (zero overhead)
#define PROFILE_SCOPE(name)
#define PROFILE_SCOPE_CAT(name, category)
#define PROFILE_FUNCTION()
#define PROFILE_FUNCTION_CAT(category)

#define PROFILE_BEGIN(name)
#define PROFILE_END(name)

#define PROFILE_START_SESSION(name, filepath)
#define PROFILE_END_SESSION()
#define PROFILE_PRINT_STATS()

#endif
// ============================================================================
// Logging Macros
// ============================================================================

#if ENABLE_LOGGING

#define LOG_TRACE(...) ::ProLog::Logger::Get().GetLogger()->trace(__VA_ARGS__)
#define LOG_DEBUG(...) ::ProLog::Logger::Get().GetLogger()->debug(__VA_ARGS__)
#define LOG_INFO(...) ::ProLog::Logger::Get().GetLogger()->info(__VA_ARGS__)
#define LOG_WARN(...) ::ProLog::Logger::Get().GetLogger()->warn(__VA_ARGS__)
#define LOG_ERROR(...) ::ProLog::Logger::Get().GetLogger()->error(__VA_ARGS__)
#define LOG_CRITICAL(...) ::ProLog::Logger::Get().GetLogger()->critical(__VA_ARGS__)

#define LOG_IF(condition, level, ...)            \
	do {                                         \
		if (condition) LOG_##level(__VA_ARGS__); \
	} while (0)

#else

// Compile out logging completely
#define LOG_TRACE(...)
#define LOG_DEBUG(...)
#define LOG_INFO(...)
#define LOG_WARN(...)
#define LOG_ERROR(...)
#define LOG_CRITICAL(...)

#define LOG_IF(condition, level, ...)

#endif
