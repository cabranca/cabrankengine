#pragma once

#include <algorithm>
#include <chrono>
#include <filesystem>
#include <fstream>
#include <string>
#include <thread>

namespace cbk {

	struct ProfileResult {
		std::string Name;
		long long Start, End;
		uint32_t ThreadID;
	};

	struct InstrumentationSession {
		std::string Name;
	};

	class Instrumentor {
	  private:
		InstrumentationSession* m_CurrentSession;
		std::ofstream m_OutputStream;
		int m_ProfileCount;

	  public:
		Instrumentor() : m_CurrentSession(nullptr), m_ProfileCount(0) {}

		void BeginSession(const std::string& name, const std::string& filepath = "results.json") {
			std::string profilingDirString = "profiling";
			// Extrae la carpeta del filepath
			std::filesystem::path profilingDir(profilingDirString);

			// Crea la carpeta si no existe
			if (!profilingDir.empty() && !std::filesystem::exists(profilingDir)) {
				std::filesystem::create_directories(profilingDir);
			}

			m_OutputStream.open(profilingDirString + "/" + filepath);
			WriteHeader();
			m_CurrentSession = new InstrumentationSession{ name };
		}

		void EndSession() {
			WriteFooter();
			m_OutputStream.close();
			delete m_CurrentSession;
			m_CurrentSession = nullptr;
			m_ProfileCount = 0;
		}

		void WriteProfile(const ProfileResult& result) {
			if (m_ProfileCount++ > 0)
				m_OutputStream << ",";

			std::string name = result.Name;
			std::replace(name.begin(), name.end(), '"', '\'');

			m_OutputStream << "{";
			m_OutputStream << "\"cat\":\"function\",";
			m_OutputStream << "\"dur\":" << (result.End - result.Start) << ',';
			m_OutputStream << "\"name\":\"" << name << "\",";
			m_OutputStream << "\"ph\":\"X\",";
			m_OutputStream << "\"pid\":0,";
			m_OutputStream << "\"tid\":" << result.ThreadID << ",";
			m_OutputStream << "\"ts\":" << result.Start;
			m_OutputStream << "}";

			m_OutputStream.flush();
		}

		void WriteHeader() {
			m_OutputStream << "{\"otherData\": {},\"traceEvents\":[";
			m_OutputStream.flush();
		}

		void WriteFooter() {
			m_OutputStream << "]}";
			m_OutputStream.flush();
		}

		[[nodiscard]] static Instrumentor& Get() {
			static Instrumentor instance;
			return instance;
		}
	};

	class InstrumentationTimer {
	  public:
		InstrumentationTimer(const char* name) : m_Name(name), m_Stopped(false) {
			m_StartTimepoint = std::chrono::high_resolution_clock::now();
		}

		~InstrumentationTimer() {
			if (!m_Stopped)
				Stop();
		}

		void Stop() {
			auto endTimepoint = std::chrono::high_resolution_clock::now();

			long long start = std::chrono::time_point_cast<std::chrono::microseconds>(m_StartTimepoint).time_since_epoch().count();
			long long end = std::chrono::time_point_cast<std::chrono::microseconds>(endTimepoint).time_since_epoch().count();

			uint32_t threadID = static_cast<uint32_t>(std::hash<std::thread::id>{}(std::this_thread::get_id()) & 0xFFFFFFFF);
			Instrumentor::Get().WriteProfile({ m_Name, start, end, threadID });

			m_Stopped = true;
		}

	  private:
		const char* m_Name;
		std::chrono::time_point<std::chrono::high_resolution_clock> m_StartTimepoint;
		bool m_Stopped;
	};
} // namespace cbk

#define CBK_PROFILE 1
#if CBK_PROFILE
#define CBK_PROFILE_BEGIN_SESSION(name, filepath) ::cbk::Instrumentor::Get().BeginSession(name, filepath)
#define CBK_PROFILE_END_SESSION() ::cbk::Instrumentor::Get().EndSession()
#define CBK_PROFILE_SCOPE(name) ::cbk::InstrumentationTimer timer##__LINE__(name);
#if defined(__GNUC__)
#define CBK_PROFILE_FUNCTION() CBK_PROFILE_SCOPE(__PRETTY_FUNCTION__)
#elif (defined(__FUNCSIG__) || (_MSC_VER))
#define CBK_PROFILE_FUNCTION() CBK_PROFILE_SCOPE(__FUNCSIG__)
#endif
#else
#define CBK_PROFILE_BEGIN_SESSION(name, filepath)
#define CBK_PROFILE_END_SESSION()
#define CBK_PROFILE_SCOPE(name)
#define CBK_PROFILE_FUNCTION()
#endif