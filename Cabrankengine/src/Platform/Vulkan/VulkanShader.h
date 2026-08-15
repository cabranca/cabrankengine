#pragma once

#include <volk/volk.h>
#include "slang/slang.h"
#include "slang/slang-com-ptr.h"

#include <Cabrankengine/Renderer/Shader.h>

namespace cbk::platform::vk {

	// Minimal shader resource: SPIR-V module + entry-point name. Pipeline state
	// (descriptor layouts, push ranges, vertex format, render state) lives on
	// the concrete Vulkan material that uses this shader.
	class VulkanShader : public rendering::Shader {
	  public:
		VulkanShader(const std::string& filepath);
		~VulkanShader() override;

		const std::string& getName() const override {
			return m_Name;
		}

		[[nodiscard]] VkShaderModule getModule() const {
			return m_ShaderModule;
		}

	  private:
		VkDevice m_Device = VK_NULL_HANDLE; // NON-OWNING
		std::string m_Name;
		VkShaderModule m_ShaderModule{ VK_NULL_HANDLE };
		Slang::ComPtr<slang::IGlobalSession> m_SlangGlobalSession;
	};

} // namespace cbk::platform::vk
