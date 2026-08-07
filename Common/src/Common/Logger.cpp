#include "Logger.h"

#include <spdlog/sinks/stdout_color_sinks.h>

namespace cbk::common {

	std::shared_ptr<spdlog::logger> Logger::m_CoreLogger;
	std::shared_ptr<spdlog::logger> Logger::m_ClientLogger;
	std::shared_ptr<spdlog::logger> Logger::m_AssetConverterLogger;

	void Logger::init() {
		spdlog::set_pattern("´%^[%T] %n: %v%$");

		m_CoreLogger = spdlog::stdout_color_mt("CBK");
		m_CoreLogger->set_level(spdlog::level::info);

		m_ClientLogger = spdlog::stdout_color_mt("APP");
		m_ClientLogger->set_level(spdlog::level::debug);

		m_AssetConverterLogger = spdlog::stdout_color_mt("ASSET CONVERTER");
		m_AssetConverterLogger->set_level(spdlog::level::info);
	}

	void Logger::setLevel(std::string_view level) {
		spdlog::level::level_enum lvl;
		if (level == "trace")
			lvl = spdlog::level::trace;
		else if (level == "debug")
			lvl = spdlog::level::debug;
		else if (level == "info")
			lvl = spdlog::level::info;
		else if (level == "warn")
			lvl = spdlog::level::warn;
		else if (level == "error")
			lvl = spdlog::level::err;
		else if (level == "critical")
			lvl = spdlog::level::critical;
		else if (level == "off")
			lvl = spdlog::level::off;
		else {
			m_CoreLogger->warn("Logger::setLevel: unknown level '{0}', falling back to 'info'", level);
			lvl = spdlog::level::info;
		}

		m_CoreLogger->set_level(lvl);
		m_ClientLogger->set_level(lvl);
		m_AssetConverterLogger->set_level(lvl);
	}

} // namespace cbk::common
