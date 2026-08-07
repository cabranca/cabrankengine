#pragma once

#include <cstdint>
#include <memory>

#ifdef _WIN32
#ifdef _WIN64
#define CBK_PLATFORM_WINDOWS
#else
#error "Cabrankengine only supports 64-bit Windows!"
#endif
#endif

#ifdef __linux__
#ifdef __x86_64__
#define CBK_PLATFORM_LINUX
#else
#error "Cabrankengine only supports 64-bit Linux!"
#endif
#endif

#ifdef __APPLE__
#include <TargetConditionals.h>
#if TARGET_OS_MAC
#if defined(__x86_64__) || defined(__arm64__)
#define CBK_PLATFORM_MACOS
#define CBK_RENDERER_METAL
#else
#error "Cabrankengine only supports 64-bit macOS!"
#endif
#else
#error "Only macOS supported for Apple devices!"
#endif
#endif

#include <Common/Assertion.h>

// Shifts a bit to the left. Replaced the macro with this to enforce type checking (and to practice)
// The return type is uint32_t to ensure it can be used in bitwise operations. It allows 32 disjoint flags.
constexpr uint32_t BIT(int x) {
	return 1u << x;
}

// Binds a function to an event. This is used to pass member functions as callbacks.
// It uses perfect forwarding to preserve the value category of the function and the event.
template <typename T, typename U>
auto BIND_EVENT_FN(T&& func, U* instance) {
	return [func = std::forward<T>(func), instance](auto&& event) -> decltype(auto) {
		return (instance->*func)(std::forward<decltype(event)>(event));
	};
}

// This usings are useless now but when we want to make our own smart pointers,
// we can just change them here and they will be used everywhere.
namespace cbk {

	template <typename T, typename Deleter = std::default_delete<T>>
	using Scope = std::unique_ptr<T, Deleter>;
	template <typename T, typename... Args>
	Scope<T> createScope(Args&&... args) {
		return std::make_unique<T>(std::forward<Args>(args)...);
	}

	template <typename T>
	using Ref = std::shared_ptr<T>;
	template <typename T, typename... Args>
	Ref<T> createRef(Args&&... args) {
		return std::make_shared<T>(std::forward<Args>(args)...);
	}
} // namespace cbk
