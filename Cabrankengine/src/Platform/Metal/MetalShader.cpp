#include <pch.h>
#include "MetalShader.h"

#include <Foundation/Foundation.hpp>
#include <Metal/Metal.hpp>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/Core/Window.h>
#include <Common/Math/MatrixFactory.h>
#include <Platform/Metal/MetalContext.h>
#include <Platform/Metal/MetalRendererAPI.h>

namespace cbk::platform::metal {

	using namespace math;
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

		// En Metal solemos tener todo en un solo string/archivo,
		// así que concatenamos o asumimos que 'vertexSrc' tiene todo si es un string raw.
		std::string source = vertexSrc + "\n" + fragmentSrc;
		compileLibrary(source);
	}

	MetalShader::~MetalShader() {
		CBK_PROFILE_FUNCTION();

		if (m_PipelineState)
			m_PipelineState->release();
		if (m_VertexFunction)
			m_VertexFunction->release();
		if (m_FragmentFunction)
			m_FragmentFunction->release();
		if (m_Library)
			m_Library->release();
	}

	void MetalShader::bind() const {
		CBK_PROFILE_FUNCTION();
		// Tell the API to use this shader — PSO creation is deferred until drawIndexed()
		// where the vertex descriptor is available from the VertexArray.
		MetalRendererAPI::SetCurrentShader(const_cast<MetalShader*>(this));
	}

	void MetalShader::unbind() const {}

	// ---- Uniform name → Metal buffer index mapping ----
	// Metal doesn't have named uniforms like OpenGL. Instead, data is set on
	// numbered buffer indices ([[buffer(N)]]). This map bridges the two worlds.
	// TODO: Replace with Metal shader reflection for automatic discovery.
	uint32_t MetalShader::getBufferIndex(const std::string& name) const {
		static const std::unordered_map<std::string, uint32_t> s_NameToIndex = {
			{ "u_ViewProjection", 30 },
			{ "u_Model",          31 },
		};

		auto it = s_NameToIndex.find(name);
		if (it != s_NameToIndex.end())
			return it->second;

		CBK_CORE_WARN("MetalShader: Unknown uniform name '{}', defaulting to buffer index 30", name);
		return 30;
	}

	void MetalShader::setInt(const std::string& name, int value) {
		CBK_PROFILE_FUNCTION();
		uint32_t idx = getBufferIndex(name);
		auto& buf = m_PendingVertexBytes[idx];
		buf.resize(sizeof(int));
		memcpy(buf.data(), &value, sizeof(int));
	}

	void MetalShader::setIntArray(const std::string& name, uint32_t count, int* values) {
		CBK_PROFILE_FUNCTION();
		// In Metal, sampler indices are not needed — textures are bound directly by slot.
		// This is intentionally a no-op for the "u_Textures" pattern from OpenGL.
	}

	void MetalShader::setFloat(const std::string& name, float value) {
		CBK_PROFILE_FUNCTION();
		uint32_t idx = getBufferIndex(name);
		auto& buf = m_PendingVertexBytes[idx];
		buf.resize(sizeof(float));
		memcpy(buf.data(), &value, sizeof(float));
	}

	void MetalShader::setFloat3(const std::string& name, const Vector3& vector) {
		CBK_PROFILE_FUNCTION();
		uint32_t idx = getBufferIndex(name);
		auto& buf = m_PendingVertexBytes[idx];
		// Metal expects float3 to be 16-byte aligned (same as float4)
		buf.resize(sizeof(float) * 4, 0);
		memcpy(buf.data(), &vector, sizeof(float) * 3);
	}

	void MetalShader::setFloat4(const std::string& name, const Vector4& vector) {
		CBK_PROFILE_FUNCTION();
		uint32_t idx = getBufferIndex(name);
		auto& buf = m_PendingVertexBytes[idx];
		buf.resize(sizeof(float) * 4);
		memcpy(buf.data(), &vector, sizeof(float) * 4);
	}

	void MetalShader::setMat4(const std::string& name, const Mat4& value) {
		CBK_PROFILE_FUNCTION();
		uint32_t idx = getBufferIndex(name);
		auto& buf = m_PendingVertexBytes[idx];
		buf.resize(sizeof(float) * 16);

		// Our Mat4 is row-major with mat[row][col] layout. Metal's float4x4 is
		// column-major with mat[col][row]. When we memcpy the raw floats, Metal
		// reads each row as a column — the same trick that works with OpenGL.
		// No transpose needed.
		memcpy(buf.data(), &value, sizeof(float) * 16);
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
		// 1. Obtener Device
		const auto& window = Application::get().getWindow();
		MetalContext* context = static_cast<MetalContext*>(window.getContext());
		MTL::Device* device = context->getDevice();

		NS::Error* error = nullptr;

		// 2. Crear Librería (Compilar Shader)
		NS::String* nsSource = NS::String::string(source.c_str(), NS::StringEncoding::UTF8StringEncoding);
		m_Library = device->newLibrary(nsSource, nullptr, &error);

		if (!m_Library) {
			CBK_CORE_ERROR("Error shader: {0}", error->localizedDescription()->utf8String());
			return;
		}

		// 3. Obtener funciones por nombre (Convención: vertex_main y fragment_main)
		// TODO: Podrías parsear el archivo para buscar anotaciones o pasarlo por parámetro
		m_VertexFunction = m_Library->newFunction(NS::String::string("vertex_main", NS::UTF8StringEncoding));
		m_FragmentFunction = m_Library->newFunction(NS::String::string("fragment_main", NS::UTF8StringEncoding));

		if (!m_VertexFunction || !m_FragmentFunction) {
			CBK_CORE_ERROR("Error: Could not find 'vertex_main' or 'fragment_main' in shader.");
			return;
		}

		nsSource->release();
	}

	void MetalShader::ensurePipelineState(MTL::VertexDescriptor* vertexDescriptor) {
		if (m_PipelineState) return; // already created (simplistic, improve later)

		auto* context = static_cast<MetalContext*>(Application::get().getWindow().getContext());
		MTL::Device* device = context->getDevice();

		MTL::RenderPipelineDescriptor* desc = MTL::RenderPipelineDescriptor::alloc()->init();
		desc->setVertexFunction(m_VertexFunction);
		desc->setFragmentFunction(m_FragmentFunction);
		desc->setVertexDescriptor(vertexDescriptor);  // <-- from the vertex array!
		desc->colorAttachments()->object(0)->setPixelFormat(MTL::PixelFormatBGRA8Unorm);

		NS::Error* error = nullptr;
		m_PipelineState = device->newRenderPipelineState(desc, &error);
		
		if (!m_PipelineState)
			CBK_CORE_ERROR("Error pipeline: {}", error->localizedDescription()->utf8String());

		desc->release();
	}
} // namespace cbk::platform::metal
