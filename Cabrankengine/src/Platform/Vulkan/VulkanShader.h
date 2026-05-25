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
		VulkanShader(std::string_view name, std::string_view vertexSrc, std::string_view fragmentSrc);
		~VulkanShader() override;

		// No-op on Vulkan — per-draw data flows through descriptor sets / push constants.
		void bind() const override {}
		void unbind() const override {}

		void setInt(const std::string&, int) override {}
		void setIntArray(const std::string&, uint32_t, int*) override {}
		void setFloat(const std::string&, float) override {}
		void setFloat3(const std::string&, const math::Vector3&) override {}
		void setFloat4(const std::string&, const math::Vector4&) override {}
		void setMat4(const std::string&, const math::Mat4&) override {}

		const std::string& getName() const override {
			return m_Name;
		}

		[[nodiscard]] VkShaderModule getModule() const {
			return m_ShaderModule;
		}

	  private:
		std::string m_Name;
		VkShaderModule m_ShaderModule{ VK_NULL_HANDLE };
		Slang::ComPtr<slang::IGlobalSession> m_SlangGlobalSession;
	};

} // namespace cbk::platform::vk
