#include <pch.h>
#include "AudioEngine.h"

namespace cbk {

	void AudioEngine::init() {
		CBK_PROFILE_FUNCTION();
		CBK_CORE_WARN("AudioEngine::init() — no audio backend linked");
	}

	void AudioEngine::playAudio(const std::string& path, bool looped) {
		CBK_PROFILE_FUNCTION();
	}
} // namespace cbk
