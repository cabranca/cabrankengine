#pragma once

#include <volk/volk.h>
#include <Common/Logger.h>
#include <cstdlib>

#define VK_CHECK(expr)                                                                                                                     \
	do {                                                                                                                                   \
		if (VkResult _vkr = (expr); _vkr != VK_SUCCESS) {                                                                                  \
			CBK_CORE_FATAL("Vulkan call failed in {}: {} returned {}", __func__, #expr, static_cast<int>(_vkr));                           \
			std::abort();                                                                                                                  \
		}                                                                                                                                  \
	} while (0)
