#include <pch.h>
#include "Renderer.h"

#include "Lights.h"
#include "Materials/Material.h"
#include "Renderer2D.h"
#include "RenderCommand.h"
#include "StorageBuffer.h"
#include "TextRenderer.h"
#include "UniformBuffer.h"
#include "VertexArray.h"


namespace cbk::rendering {

	using namespace math;

	// Estructura auxiliar para la luz (32 bytes total)
	struct directionalLightData {
		math::Vector3 direction; // 12 bytes
		float _Pad0 = 0.0f;      // 4 bytes de relleno (Total: 16)

		math::Vector3 radiance; // 12 bytes
		float _Pad1 = 0.0f;     // 4 bytes de relleno (Total: 16)
	};

	struct AltSceneData {
		Mat4 ViewProjectionMatrix; // 64 bytes
		directionalLightData DirLight;   // 32 bytes (Offset 64)
		math::Vector3 CameraPosition;    // 12 bytes (Offset 96) -> Reemplaza al uniform viewPos
		float _Pad2 = 0.0f;              // 4 bytes (Offset 108 -> Total 112 bytes)
	};

	static Ref<UniformBuffer> s_SceneUBO;
	static AltSceneData s_AltSceneData;

	// GLSL std430 alignment rules:
	// vec4 = 16 bytes. scalar = 4 bytes.
	// Structs align to largest member (16 bytes).
	struct PointLightGPU {
		float position[4];  // x, y, z, padding
		float radiance[4];   // r, g, b, padding (o intensidad)
		float constant;
		float linear;
		float quadratic;
		float padding;      // Para completar 48 bytes (múltiplo de 16)
	};

	struct LightBufferHeader {
		int count;
		int padding[3];     // Alinear a 16 bytes antes del array
	};

	void Renderer::init() {
		CBK_PROFILE_FUNCTION();

		RenderCommand::init();
		Renderer2D::init();
		TextRenderer::init();
		s_SceneData->lightSSBO = StorageBuffer::create(sizeof(LightBufferHeader) + sizeof(PointLightGPU));
		s_SceneUBO = UniformBuffer::create(sizeof(AltSceneData), 0);
	}

	void Renderer::shutdown() {
		Renderer2D::shutdown();
	}

	void Renderer::beginScene(const math::Mat4& viewProjectionMatrix, const math::Vector3& cameraWorldPosition,
	                          const LightEnvironment& environment) {
		s_AltSceneData.ViewProjectionMatrix = viewProjectionMatrix;
		s_AltSceneData.DirLight.direction = environment.DirLight.direction;
		s_AltSceneData.DirLight.radiance = environment.DirLight.radiance;
		s_AltSceneData.CameraPosition = cameraWorldPosition;
		s_SceneUBO->setData(&s_AltSceneData,
		                    sizeof(AltSceneData)); // TODO: if this is the same size as in the initialization, why pass it again?
		s_SceneData->lightEnvironment = environment;

		uploadLightEnvironment();
	}

	void Renderer::endScene() {
		// endFrame() is now called once per frame in the application loop,
		// after all rendering subsystems (Renderer, Renderer2D, etc.) are done.
	}

	void Renderer::submit(const Ref<Shader>& shader, const Ref<VertexArray>& vertexArray, const Mat4& transform) {
		shader->bind();
		shader->setMat4("u_Model", transform);
		RenderCommand::drawIndexed(vertexArray);
	}

	void Renderer::submit(const Ref<Material>& material, const Ref<VertexArray>& vertexArray, const Mat4& transform) {
		material->bind();
		material->getShader()->setMat4("u_Model", transform);
		RenderCommand::drawIndexed(vertexArray);
	}

	void Renderer::onWindowResize(uint32_t width, uint32_t height) {
		RenderCommand::setViewport(0, 0, width, height);
	}
	
	void Renderer::uploadLightEnvironment() {
		size_t count = s_SceneData->lightEnvironment.PointLights.size();
		LightBufferHeader header;
		header.count = static_cast<int>(count);

		std::vector<uint8_t> bufferData;
		bufferData.resize(sizeof(LightBufferHeader) + count * sizeof(PointLightGPU));

		memcpy(bufferData.data(), &header, sizeof(LightBufferHeader));

		PointLightGPU* currentLight = reinterpret_cast<PointLightGPU*>(bufferData.data() + sizeof(LightBufferHeader));

		for (const auto& light : s_SceneData->lightEnvironment.PointLights) {
			currentLight->position[0] = light.position.x;
			currentLight->position[1] = light.position.y;
			currentLight->position[2] = light.position.z;
			currentLight->position[3] = 1.f;
			
			currentLight->radiance[0] = light.radiance.x;
			currentLight->radiance[1] = light.radiance.y;
			currentLight->radiance[2] = light.radiance.z;
			currentLight->radiance[3] = 0.f;

			currentLight->constant = light.constant;
			currentLight->linear = light.linear;
			currentLight->quadratic = light.quadratic;

			currentLight++;
		}

		if (s_SceneData->lightSSBO) {
			s_SceneData->lightSSBO->setData(bufferData.data(), bufferData.size());
			s_SceneData->lightSSBO->bind(0);
		}
	}
} // namespace cbk::rendering
