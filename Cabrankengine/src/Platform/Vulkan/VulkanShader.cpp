#include <pch.h>
#include "VulkanShader.h"

#include <fstream>
#include <sstream>
#include <vector>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>

#include "VkCheck.h"
#include "VulkanDeviceContext.h"

namespace cbk::platform::vk {

	static std::string slangDiagnosticsToString(ISlangBlob* blob) {
		if (!blob)
			return {};
		return std::string(static_cast<const char*>(blob->getBufferPointer()), blob->getBufferSize());
	}

	VulkanShader::VulkanShader(const std::string& filepath) {
		std::filesystem::path path(filepath);
		m_Name = path.stem().string();

		auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());

		// 1) Read shader source from disk. loadModuleFromSourceString needs the actual code,
		//    not just a path; the previous code passed nullptr as the source blob and silently failed.
		std::ifstream file(filepath + ".slang");
		if (!file.is_open()) {
			CBK_CORE_ERROR("VulkanShader: cannot open {}.slang", filepath);
			return;
		}
		std::stringstream sourceStream;
		sourceStream << file.rdbuf();
		std::string source = sourceStream.str();

		// 2) Build a Slang session targeting SPIR-V 1.4.
		slang::createGlobalSession(m_SlangGlobalSession.writeRef());
		auto slangTargets =
		    std::to_array<slang::TargetDesc>({ { .format = SLANG_SPIRV, .profile = m_SlangGlobalSession->findProfile("spirv_1_4") } });
		auto slangOptions = std::to_array<slang::CompilerOptionEntry>(
		    { { slang::CompilerOptionName::EmitSpirvDirectly, { slang::CompilerOptionValueKind::Int, 1 } } });
		slang::SessionDesc slangSessionDesc{
			.targets = slangTargets.data(),
			.targetCount = SlangInt(slangTargets.size()),
			.defaultMatrixLayoutMode = SLANG_MATRIX_LAYOUT_COLUMN_MAJOR,
			.compilerOptionEntries = slangOptions.data(),
			.compilerOptionEntryCount = static_cast<uint32_t>(slangOptions.size()),
		};

		Slang::ComPtr<slang::ISession> slangSession;
		m_SlangGlobalSession->createSession(slangSessionDesc, slangSession.writeRef());

		// 3) Load + compile the module. Diagnostics surface compilation errors.
		Slang::ComPtr<ISlangBlob> diagnostics;
		Slang::ComPtr<slang::IModule> slangModule{ slangSession->loadModuleFromSourceString(m_Name.c_str(), (filepath + ".slang").c_str(),
			                                                                                source.c_str(), diagnostics.writeRef()) };
		if (!slangModule) {
			CBK_CORE_ERROR("VulkanShader: failed to load slang module from {}.slang: {}", filepath, slangDiagnosticsToString(diagnostics));
			return;
		}

		// 4) Compose module + all defined entry points so the final program contains every stage.
		std::vector<slang::IComponentType*> components{ slangModule };
		std::vector<Slang::ComPtr<slang::IEntryPoint>> entryPoints;
		SlangInt entryPointCount = slangModule->getDefinedEntryPointCount();
		entryPoints.reserve(entryPointCount);
		for (SlangInt i = 0; i < entryPointCount; ++i) {
			Slang::ComPtr<slang::IEntryPoint> ep;
			slangModule->getDefinedEntryPoint(i, ep.writeRef());
			entryPoints.push_back(ep);
			components.push_back(ep);
		}

		Slang::ComPtr<slang::IComponentType> composed;
		diagnostics = nullptr;
		auto slangResult = slangSession->createCompositeComponentType(components.data(), static_cast<SlangInt>(components.size()),
		                                                              composed.writeRef(), diagnostics.writeRef());
		if (SLANG_FAILED(slangResult)) {
			CBK_CORE_ERROR("VulkanShader: composite component type failed for {}.slang: {}", filepath,
			               slangDiagnosticsToString(diagnostics));
			return;
		}

		Slang::ComPtr<slang::IComponentType> linkedProgram;
		diagnostics = nullptr;
		slangResult = composed->link(linkedProgram.writeRef(), diagnostics.writeRef());
		if (SLANG_FAILED(slangResult)) {
			CBK_CORE_ERROR("VulkanShader: link failed for {}.slang: {}", filepath, slangDiagnosticsToString(diagnostics));
			return;
		}

		// 5) Emit SPIR-V for target 0 (the SPIR-V target configured above).
		Slang::ComPtr<ISlangBlob> spirv;
		diagnostics = nullptr;
		slangResult = linkedProgram->getTargetCode(0, spirv.writeRef(), diagnostics.writeRef());
		if (SLANG_FAILED(slangResult) || !spirv || spirv->getBufferSize() == 0) {
			CBK_CORE_ERROR("VulkanShader: SPIR-V emission failed for {}.slang: {}", filepath, slangDiagnosticsToString(diagnostics));
			return;
		}

		VkShaderModuleCreateInfo shaderModuleCI{
			.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
			.codeSize = spirv->getBufferSize(),
			.pCode = reinterpret_cast<const uint32_t*>(spirv->getBufferPointer()),
		};
		VK_CHECK(vkCreateShaderModule(ctx->getLogicalDevice(), &shaderModuleCI, nullptr, &m_ShaderModule));
	}

	VulkanShader::VulkanShader(std::string_view name, std::string_view /*vertexSrc*/, std::string_view /*fragmentSrc*/) : m_Name(name) {
		// TODO: compile from inline source. Not currently used.
	}

	VulkanShader::~VulkanShader() {
		if (m_ShaderModule != VK_NULL_HANDLE) {
			auto ctx = static_cast<VulkanDeviceContext*>(Application::get().getWindow().getContext());
			vkDestroyShaderModule(ctx->getLogicalDevice(), m_ShaderModule, nullptr);
		}
	}

} // namespace cbk::platform::vk
