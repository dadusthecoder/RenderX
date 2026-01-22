#include "ProLog/ProLog.h"
#include <algorithm>
#include <iomanip>
#include <sstream>
#include <iostream>

namespace ProLog {

	// ============================================================================
	// ProfilerSession Implementation
	// ============================================================================

	ProfilerSession& ProfilerSession::Get() {
		static ProfilerSession instance;
		return instance;
	}

	ProfilerSession::~ProfilerSession() {
		EndSession();
	}

	void ProfilerSession::BeginSession(const std::string& name, const std::string& filepath) {
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (!m_CurrentSession.empty()) {
			EndSession();
		}

		m_OutputStream.open(filepath);
		if (!m_OutputStream.is_open()) {
			std::cerr << "Failed to open profile output file: " << filepath << std::endl;
			return;
		}

		m_CurrentSession = name;
		m_ProfileCount = 0;
		m_StartTime = std::chrono::high_resolution_clock::now();
		m_Statistics.clear();
		m_Buffer.clear();
		m_Buffer.reserve(m_Config.bufferSize);

		WriteHeader();
	}

	void ProfilerSession::EndSession() {
		std::lock_guard<std::mutex> lock(m_Mutex);

		if (!m_CurrentSession.empty()) {
			FlushBuffer();
			WriteFooter();
			WriteStatisticsFile();
			m_OutputStream.close();
			m_CurrentSession.clear();
			m_ProfileCount = 0;
		}
	}

	void ProfilerSession::WriteProfile(const ProfileResult& result) {
		if (!m_Config.enableProfiling) return;

		std::lock_guard<std::mutex> lock(m_Mutex);

		// Update statistics
		auto& stats = m_Statistics[result.name];
		stats.functionName = result.name;
		stats.callCount++;
		stats.totalDuration += result.duration;
		stats.minDuration = std::min(stats.minDuration, result.duration);
		stats.maxDuration = std::max(stats.maxDuration, result.duration);
		stats.avgDuration = static_cast<double>(stats.totalDuration) / stats.callCount;

		// Buffer the result
		m_Buffer.push_back(result);

		if (m_Config.autoFlush && m_Buffer.size() >= m_Config.bufferSize) {
			FlushBuffer();
		}
	}

	void ProfilerSession::SetConfig(const ProfilerConfig& config) {
		std::lock_guard<std::mutex> lock(m_Mutex);
		m_Config = config;
	}

	std::unordered_map<std::string, ProfileStatistics> ProfilerSession::GetStatistics() {
		std::lock_guard<std::mutex> lock(m_Mutex);
		return m_Statistics;
	}

	void ProfilerSession::PrintStatistics() {
		std::lock_guard<std::mutex> lock(m_Mutex);

		std::vector<ProfileStatistics> sorted;
		for (const auto& pair : m_Statistics) {
			sorted.push_back(pair.second);
		}
		std::sort(sorted.begin(), sorted.end(),
			[](const ProfileStatistics& a, const ProfileStatistics& b) {
				return a.totalDuration > b.totalDuration;
			});

		std::cout << "\n" << std::string(100, '=') << "\n";
		std::cout << "PROFILING STATISTICS\n";
		std::cout << std::string(100, '=') << "\n";
		std::cout << std::left
			<< std::setw(40) << "Function"
			<< std::setw(10) << "Calls"
			<< std::setw(15) << "Total (ms)"
			<< std::setw(15) << "Avg (ms)"
			<< std::setw(15) << "Min (ms)"
			<< std::setw(15) << "Max (ms)" << "\n";
		std::cout << std::string(100, '-') << "\n";

		for (const auto& stat : sorted) {
			std::cout << std::left
				<< std::setw(40) << stat.functionName.substr(0, 39)
				<< std::setw(10) << stat.callCount
				<< std::setw(15) << std::fixed << std::setprecision(3) << (stat.totalDuration / 1000.0)
				<< std::setw(15) << (stat.avgDuration / 1000.0)
				<< std::setw(15) << (stat.minDuration / 1000.0)
				<< std::setw(15) << (stat.maxDuration / 1000.0) << "\n";
		}
		std::cout << std::string(100, '=') << "\n\n";
	}

	void ProfilerSession::WriteHeader() {
		m_OutputStream << "{\"otherData\": {\"sessionName\":\"" << m_CurrentSession
			<< "\"},\"traceEvents\":[";
		m_OutputStream.flush();
	}

	void ProfilerSession::WriteFooter() {
		m_OutputStream << "]}";
		m_OutputStream.flush();
	}

	void ProfilerSession::FlushBuffer() {
		for (const auto& result : m_Buffer) {
			if (m_ProfileCount++ > 0)
				m_OutputStream << ",";

			std::string name = result.name;
			std::replace(name.begin(), name.end(), '"', '\'');

			m_OutputStream << "{";
			m_OutputStream << "\"cat\":\"" << result.category << "\",";
			m_OutputStream << "\"dur\":" << result.duration << ',';
			m_OutputStream << "\"name\":\"" << name << "\",";
			m_OutputStream << "\"ph\":\"X\",";
			m_OutputStream << "\"pid\":0,";
			m_OutputStream << "\"tid\":" << result.threadID << ",";
			m_OutputStream << "\"ts\":" << result.start;

			// Add metadata if present
			if (!result.metadata.empty()) {
				m_OutputStream << ",\"args\":{";
				bool first = true;
				for (const auto& [key, value] : result.metadata) {
					if (!first) m_OutputStream << ",";
					m_OutputStream << "\"" << key << "\":\"" << value << "\"";
					first = false;
				}
				m_OutputStream << "}";
			}

			m_OutputStream << "}";
		}
		m_OutputStream.flush();
		m_Buffer.clear();
	}

	void ProfilerSession::WriteStatisticsFile() {
		std::string statsPath = "profile_statistics.txt";
		std::ofstream statsFile(statsPath);

		if (!statsFile.is_open()) {
			std::cerr << "Failed to open statistics file: " << statsPath << std::endl;
			return;
		}

		std::vector<ProfileStatistics> sorted;
		for (const auto& pair : m_Statistics) {
			sorted.push_back(pair.second);
		}
		std::sort(sorted.begin(), sorted.end(),
			[](const ProfileStatistics& a, const ProfileStatistics& b) {
				return a.totalDuration > b.totalDuration;
			});

		auto sessionDuration = std::chrono::duration_cast<std::chrono::milliseconds>(
			std::chrono::high_resolution_clock::now() - m_StartTime).count();

		statsFile << "===========================================\n";
		statsFile << "PROFILING STATISTICS - " << m_CurrentSession << "\n";
		statsFile << "Session Duration: " << sessionDuration << " ms\n";
		statsFile << "===========================================\n\n";

		statsFile << std::left
			<< std::setw(40) << "Function"
			<< std::setw(10) << "Calls"
			<< std::setw(15) << "Total (ms)"
			<< std::setw(15) << "Avg (ms)"
			<< std::setw(15) << "Min (ms)"
			<< std::setw(15) << "Max (ms)"
			<< std::setw(10) << "% Time" << "\n";
		statsFile << std::string(120, '-') << "\n";

		long long totalTime = 0;
		for (const auto& stat : sorted) {
			totalTime += stat.totalDuration;
		}

		for (const auto& stat : sorted) {
			double percentage = totalTime > 0 ? (static_cast<double>(stat.totalDuration) / totalTime * 100.0) : 0.0;
			statsFile << std::left
				<< std::setw(40) << stat.functionName.substr(0, 39)
				<< std::setw(10) << stat.callCount
				<< std::setw(15) << std::fixed << std::setprecision(3) << (stat.totalDuration / 1000.0)
				<< std::setw(15) << (stat.avgDuration / 1000.0)
				<< std::setw(15) << (stat.minDuration / 1000.0)
				<< std::setw(15) << (stat.maxDuration / 1000.0)
				<< std::setw(10) << std::setprecision(1) << percentage << "%\n";
		}
		statsFile << "===========================================\n";
		statsFile.close();
	}

	// ============================================================================
	// Timer Implementation
	// ============================================================================
	static thread_local size_t s_CurrentDepth = 0;

	Timer::Timer(const char* name, const char* category)
		: m_Name(name), m_Category(category), m_Stopped(false), m_Depth(s_CurrentDepth++) {
		m_StartTimepoint = std::chrono::high_resolution_clock::now();
	}

	Timer::~Timer() {
		if (!m_Stopped)
			Stop();
		s_CurrentDepth--;
	}

	void Timer::Stop() {
		auto endTimepoint = std::chrono::high_resolution_clock::now();

		long long start = std::chrono::time_point_cast<std::chrono::microseconds>(
			m_StartTimepoint).time_since_epoch().count();
		long long end = std::chrono::time_point_cast<std::chrono::microseconds>(
			endTimepoint).time_since_epoch().count();

		ProfileResult result;
		result.name = m_Name;
		result.category = m_Category;
		result.start = start;
		result.duration = end - start;
		result.threadID = std::this_thread::get_id();
		result.depth = m_Depth;
		result.metadata = m_Metadata;

		ProfilerSession::Get().WriteProfile(result);

		m_Stopped = true;
	}

	void Timer::AddMetadata(const std::string& key, const std::string& value) {
		m_Metadata[key] = value;
	}

	// ============================================================================
	// Logger Implementation
	// ============================================================================

	Logger& Logger::Get() {
		static Logger instance;
		return instance;
	}

	void Logger::Init(const std::string& logFile, size_t maxSize, size_t maxFiles) {
		try {
			auto console_sink = std::make_shared<spdlog::sinks::stdout_color_sink_mt>();
			console_sink->set_level(spdlog::level::trace);
			console_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%^%=8l%$] [thread %t] %v");

			auto file_sink = std::make_shared<spdlog::sinks::rotating_file_sink_mt>(
				logFile, maxSize, maxFiles);
			file_sink->set_level(spdlog::level::trace);
			file_sink->set_pattern("[%Y-%m-%d %H:%M:%S.%e] [%l] [thread %t] [%s:%#] %v");

			std::vector<spdlog::sink_ptr> sinks{ console_sink, file_sink };
			m_Logger = std::make_shared<spdlog::logger>("APP", sinks.begin(), sinks.end());
			m_Logger->set_level(spdlog::level::trace);
			m_Logger->flush_on(spdlog::level::warn);

			spdlog::register_logger(m_Logger);

			m_Logger->info("Logging system initialized");
		}
		catch (const spdlog::spdlog_ex& ex) {
			std::cerr << "Log initialization failed: " << ex.what() << std::endl;
		}
	}

	std::shared_ptr<spdlog::logger>& Logger::GetLogger() {
		return m_Logger;
	}

	void Logger::SetLevel(spdlog::level::level_enum level) {
		if (m_Logger) {
			m_Logger->set_level(level);
		}
	}

	void Logger::Flush() {
		if (m_Logger) {
			m_Logger->flush();
		}
	}

	// ============================================================================
	// PerformanceMarker Implementation
	// ============================================================================

	std::unordered_map<std::string, std::unique_ptr<Timer>> PerformanceMarker::s_ActiveMarkers;
	std::mutex PerformanceMarker::s_Mutex;

	void PerformanceMarker::BeginEvent(const std::string& name, const std::string& category) {
		std::lock_guard<std::mutex> lock(s_Mutex);
		auto timer = std::make_unique<Timer>(name.c_str(), category.c_str());
		s_ActiveMarkers[name] = std::move(timer);
	}

	void PerformanceMarker::EndEvent(const std::string& name) {
		std::lock_guard<std::mutex> lock(s_Mutex);
		auto it = s_ActiveMarkers.find(name);
		if (it != s_ActiveMarkers.end()) {
			it->second->Stop();
			s_ActiveMarkers.erase(it);
		}
	}

} // namespace Profiler