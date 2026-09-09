#pragma once

#include <pch.h>

#include <nlohmann/json.hpp>

namespace cbk {

	class Config {
	  public:
		static nlohmann::json load(const std::string& path) {
			std::ifstream file(path);
			if (!file.is_open()) {
				nlohmann::json defaults = { { "window", { { "title", "Cabrankengine" }, { "width", 1600 }, { "height", 900 } } } };

				std::ofstream out(path);
				if (out.is_open()) {
					out << defaults.dump(4);
					out.close();
					CBK_CORE_WARN("[Config] File not found, creating new one in {0}", path);
				} else {
					CBK_CORE_ERROR("[Config] File not found, failed to create a new on in {0}", path);
				}

				return defaults;
			}

			nlohmann::json j;
			try {
				file >> j;
			} catch (std::exception& e) {
				CBK_CORE_ERROR("[Config] Failed reading {0} - Error message: {1}", path, e.what());
				return { { "window", { { "title", "Cabrankengine" }, { "width", 1600 }, { "height", 900 } } } };
			}
			return j;
		}
	};
} // namespace cbk
