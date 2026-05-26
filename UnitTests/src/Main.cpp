#define CATCH_CONFIG_RUNNER
#include <catch.hpp>

#include <vector>
#include <string_view>

#include <Cabrankengine/Core/Logger.h>

int main(int argc, char* argv[]) {
	cbk::Logger::init();

	// Strip --log-level from argv before passing to Catch so it doesn't reject the unknown flag.
	std::vector<char*> filtered;
	filtered.reserve(argc);
	filtered.push_back(argv[0]);
	for (int i = 1; i < argc; ++i) {
		std::string_view arg = argv[i];
		if (arg == "--log-level" && i + 1 < argc) {
			cbk::Logger::setLevel(argv[++i]);
		} else if (arg.starts_with("--log-level=")) {
			cbk::Logger::setLevel(arg.substr(12));
		} else {
			filtered.push_back(argv[i]);
		}
	}

	return Catch::Session().run(static_cast<int>(filtered.size()), filtered.data());
}
