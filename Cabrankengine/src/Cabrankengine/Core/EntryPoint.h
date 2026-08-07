#pragma once

// Despite being obvious, this is a good practice to be able to give a clear error message if the platform is not supported.
#if defined(CBK_PLATFORM_WINDOWS) || defined(CBK_PLATFORM_LINUX) || defined(CBK_PLATFORM_MACOS)

// The "extern" keyword is optional here but it is a good practice to use it to clearly show that the function is defined outside of this
// file. This expresion is similar to a forward declaration for functions.
extern cbk::Application* cbk::createApplication();

int main(int argc, char** argv) {
	cbk::Logger::init();

	for (int i = 1; i < argc; ++i) {
		std::string_view arg = argv[i];
		if (arg == "--log-level" && i + 1 < argc) {
			cbk::Logger::setLevel(argv[++i]);
		} else if (arg.starts_with("--log-level=")) {
			cbk::Logger::setLevel(arg.substr(12));
		}
	}

	CBK_CORE_INFO("Initialized logger!");
	CBK_PROFILE_BEGIN_SESSION("Startup", "CabrankengineProfile-Startup.json");

	auto app = cbk::createApplication();
	CBK_PROFILE_END_SESSION();

	CBK_PROFILE_BEGIN_SESSION("Runtime", "CabrankengineProfile-Runtime.json");
	app->Run();
	CBK_PROFILE_END_SESSION();

	CBK_PROFILE_BEGIN_SESSION("Shutdown", "CabrankengineProfile-Shutdown.json");
	delete app;
	CBK_PROFILE_END_SESSION();
}

#else
#error "Cabrankengine only supports Windows and Linux!"
#endif
