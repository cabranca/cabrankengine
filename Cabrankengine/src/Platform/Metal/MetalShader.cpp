#include <pch.h>
#include "MetalShader.h"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include <Platform/Metal/MetalDeviceContext.h>

namespace cbk::platform::metal {

	using namespace rendering;

	MetalShader::MetalShader(const std::string& filepath) {
		CBK_PROFILE_FUNCTION();

		std::filesystem::path path(filepath);
		m_Name = path.stem().string();

		std::string source = readFile(filepath + ".metal");
		compileLibrary(source);
	}

	MetalShader::MetalShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc) : m_Name(name) {
		CBK_PROFILE_FUNCTION();

		// Metal keeps both stages in a single source string, so concatenate.
		std::string source = vertexSrc + "\n" + fragmentSrc;
		compileLibrary(source);
	}

	MetalShader::~MetalShader() {
		CBK_PROFILE_FUNCTION();

		if (m_VertexFunction)
			m_VertexFunction->release();
		if (m_FragmentFunction)
			m_FragmentFunction->release();
		if (m_Library)
			m_Library->release();
	}

	std::string MetalShader::readFile(const std::string& filepath) {
		CBK_PROFILE_FUNCTION();

		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in) {
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&result[0], result.size());
			in.close();
		} else {
			CBK_CORE_ERROR("Could not open shader file: {0}", filepath);
		}
		return result;
	}

	void MetalShader::compileLibrary(const std::string& source) {
		const auto& window = Application::get().getWindow();
		MetalDeviceContext* context = static_cast<MetalDeviceContext*>(window.getContext());
		MTL::Device* device = context->getDevice();

		NS::Error* error = nullptr;
		NS::String* nsSource = NS::String::string(source.c_str(), NS::StringEncoding::UTF8StringEncoding);
		m_Library = device->newLibrary(nsSource, nullptr, &error);
		nsSource->release();

		if (!m_Library) {
			CBK_CORE_ERROR("MetalShader '{0}' compilation failed: {1}", m_Name,
			               error ? error->localizedDescription()->utf8String() : "unknown error");
			return;
		}

		// Convention: every .metal shader exposes vertex_main and fragment_main.
		m_VertexFunction = m_Library->newFunction(NS::String::string("vertex_main", NS::UTF8StringEncoding));
		m_FragmentFunction = m_Library->newFunction(NS::String::string("fragment_main", NS::UTF8StringEncoding));

		if (!m_VertexFunction || !m_FragmentFunction)
			CBK_CORE_ERROR("MetalShader '{0}': could not find 'vertex_main' or 'fragment_main'.", m_Name);
	}
} // namespace cbk::platform::metal
