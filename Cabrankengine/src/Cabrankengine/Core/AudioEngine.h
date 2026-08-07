#pragma once

#include <string>

namespace cbk {

	class AudioEngine {
	  public:
		static void init();
		static void playAudio(const std::string& path, bool looped);
	};
} // namespace cbk
