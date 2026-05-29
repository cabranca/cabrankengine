#pragma once

#include "Scene.h"

namespace cbk::scene {

	class SceneSerializer {
	  public:
		static void        serialize(const Scene& scene, std::string_view path);
		static Scene       deserialize(std::string_view path);
		static std::string serializeToString(const Scene& scene);
		// Note: the 'id' field on each entity in the JSON is read-only metadata
		// for inspector/debug use. Deserialization assigns fresh IDs via the Registry.
		static Scene       deserializeFromString(std::string_view json);
	};
} // namespace cbk::scene