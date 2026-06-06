#pragma once

#include <memory>

#include <spdlog/spdlog.h>

namespace cbk::common {

	// Logger manages the logging system for the engine, client, and asset converter.
	// Wrapper around spdlog to provide a simple interface for logging messages.
	class Logger {
	  public:
		// Sets the loggers and the patterns
		static void init();

		// Sets the level on all engine loggers. Accepts "trace", "debug", "info", "warn", "error", "critical", "off".
		// Unknown values fall back to "info" and emit a warning.
		static void setLevel(std::string_view level);

		// Returns the logger for the engine app
		[[nodiscard]] static std::shared_ptr<spdlog::logger>& getCoreLogger() {
			return m_CoreLogger;
		}

		// Returns the logger for the client (game) app
		[[nodiscard]] static std::shared_ptr<spdlog::logger>& getClientLogger() {
			return m_ClientLogger;
		}

		// Returns the logger for the asset converter
		[[nodiscard]] static std::shared_ptr<spdlog::logger>& getAssetConverterLogger() {
			return m_AssetConverterLogger;
		}

	  private:
		static std::shared_ptr<spdlog::logger> m_CoreLogger;
		static std::shared_ptr<spdlog::logger> m_ClientLogger;
		static std::shared_ptr<spdlog::logger> m_AssetConverterLogger;
	};

} // namespace cbk::common

// Core log macros
#define CBK_CORE_TRACE(...) ::cbk::common::Logger::getCoreLogger()->trace(__VA_ARGS__)
#define CBK_CORE_DEBUG(...) ::cbk::common::Logger::getCoreLogger()->debug(__VA_ARGS__)
#define CBK_CORE_INFO(...) ::cbk::common::Logger::getCoreLogger()->info(__VA_ARGS__)
#define CBK_CORE_WARN(...) ::cbk::common::Logger::getCoreLogger()->warn(__VA_ARGS__)
#define CBK_CORE_ERROR(...) ::cbk::common::Logger::getCoreLogger()->error(__VA_ARGS__)
#define CBK_CORE_FATAL(...) ::cbk::common::Logger::getCoreLogger()->critical(__VA_ARGS__)

// Client log macros
#define CBK_APP_TRACE(...) ::cbk::common::Logger::getClientLogger()->trace(__VA_ARGS__)
#define CBK_APP_DEBUG(...) ::cbk::common::Logger::getClientLogger()->debug(__VA_ARGS__)
#define CBK_APP_INFO(...) ::cbk::common::Logger::getClientLogger()->info(__VA_ARGS__)
#define CBK_APP_WARN(...) ::cbk::common::Logger::getClientLogger()->warn(__VA_ARGS__)
#define CBK_APP_ERROR(...) ::cbk::common::Logger::getClientLogger()->error(__VA_ARGS__)
#define CBK_APP_FATAL(...) ::cbk::common::Logger::getClientLogger()->critical(__VA_ARGS__)

// Asset Converter log macros
#define CBK_AC_TRACE(...) ::cbk::common::Logger::getAssetConverterLogger()->trace(__VA_ARGS__)
#define CBK_AC_DEBUG(...) ::cbk::common::Logger::getAssetConverterLogger()->debug(__VA_ARGS__)
#define CBK_AC_INFO(...) ::cbk::common::Logger::getAssetConverterLogger()->info(__VA_ARGS__)
#define CBK_AC_WARN(...) ::cbk::common::Logger::getAssetConverterLogger()->warn(__VA_ARGS__)
#define CBK_AC_ERROR(...) ::cbk::common::Logger::getAssetConverterLogger()->error(__VA_ARGS__)
#define CBK_AC_FATAL(...) ::cbk::common::Logger::getAssetConverterLogger()->critical(__VA_ARGS__)
