#pragma once

#include <Common/Logger.h>

#ifdef CBK_DEBUG
#define CBK_ENABLE_ASSERTS
#if defined(_WIN32) && defined(_WIN64)
#define CBK_DEBUG_BREAK __debugbreak()
#elif defined(__linux__) || defined(__APPLE__)
#include <signal.h>
#define CBK_DEBUG_BREAK raise(SIGTRAP)
#endif
#endif

#ifdef CBK_ENABLE_ASSERTS
#define CBK_APP_ASSERT(x, ...)                                                                                                             \
	{                                                                                                                                      \
		if (!(x)) {                                                                                                                        \
			CBK_APP_ERROR("Assertion failed: {0}", __VA_ARGS__);                                                                           \
			CBK_DEBUG_BREAK;                                                                                                               \
		}                                                                                                                                  \
	}
#define CBK_CORE_ASSERT(x, ...)                                                                                                            \
	{                                                                                                                                      \
		if (!(x)) {                                                                                                                        \
			CBK_CORE_ERROR("Assertion failed: {0}", __VA_ARGS__);                                                                          \
			CBK_DEBUG_BREAK;                                                                                                               \
		}                                                                                                                                  \
	}
#define CBK_AC_ASSERT(x, ...)                                                                                                              \
	{                                                                                                                                      \
		if (!(x)) {                                                                                                                        \
			CBK_AC_ERROR("Assertion failed: {0}", __VA_ARGS__);                                                                            \
			CBK_DEBUG_BREAK;                                                                                                               \
		}                                                                                                                                  \
	}
#else
#define CBK_APP_ASSERT(x, ...)
#define CBK_CORE_ASSERT(x, ...)
#define CBK_AC_ASSERT(x, ...)
#endif
