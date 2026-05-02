#pragma once

#include "Scene.h"

namespace cbk::scene {

	class SceneSerializer {
	  public:
		static void        serialize(const Scene& scene, std::string_view path);
		static Scene       deserialize(std::string_view path);
		static std::string serializeToString(const Scene& scene);
		static Scene       deserializeFromString(std::string_view json);
	};
} // namespace cbk::scene