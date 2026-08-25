#include <pch.h>
#include "Renderer.h"

#include <Cabrankengine/Scene/DefaultLibrary.h>

#include "Materials/Material.h"
#include "Renderer2D.h"
#include "RenderCommand.h"
#include "StorageBuffer.h"
#include "UniformBuffer.h"
#include "TextRenderer.h"

namespace cbk::rendering {

	using namespace math;
	using namespace scene;

	// Estructura auxiliar para la luz (32 bytes total)
	struct DirectionalLightData {
		math::Vector3 direction; // 12 bytes
		float _Pad0 = 0.0f;      // 4 bytes de relleno (Total: 16)

		math::Vector3 radiance; // 12 bytes
		float _Pad1 = 0.0f;     // 4 bytes de relleno (Total: 16)
	};

	struct AltSceneData {
		Mat4 ViewProjectionMatrix;     // 64 bytes
		DirectionalLightData DirLight; // 32 bytes (Offset 64)
		math::Vector3 CameraPosition;  // 12 bytes (Offset 96) -> Reemplaza al uniform viewPos
		float _Pad2 = 0.0f;            // 4 bytes (Offset 108 -> Total 112 bytes)
	};

	static Ref<UniformBuffer> s_SSceneUbo;
	static AltSceneData s_AltSceneData;

	// GLSL std430 alignment rules:
	// vec4 = 16 bytes. scalar = 4 bytes.
	// Structs align to largest member (16 bytes).
	struct PointLightGPU {
		float position[4]; // x, y, z, padding
		float radiance[4]; // r, g, b, padding (o intensidad)
		float constant;
		float linear;
		float quadratic;
		float padding; // Para completar 48 bytes (múltiplo de 16)
	};

	struct LightBufferHeader {
		int count;
		int padding[3]; // Alinear a 16 bytes antes del array
	};

	// Upper bound on point lights uploaded per frame. The SSBO is sized for this
	// many lights at init() and reused; uploadLightEnvironment() asserts the scene
	// stays under the cap. Bump this value if you need more — the only cost is
	// fixed GPU memory: sizeof(LightBufferHeader) + N * sizeof(PointLightGPU).
	static constexpr uint32_t k_MaxPointLights = 64;

	void Renderer::init(const Window& window) {
		CBK_PROFILE_FUNCTION();

		RenderCommand::init(window);
		//DefaultLibrary::init();
		// Scene UBO must exist before Renderer2D/TextRenderer init, because the
		// Vulkan materials they create pull the scene descriptor set layout from it
		// at pipeline-layout construction time. The light SSBO is consumed the same
		// way by VulkanPBRMaterial, so it must also be alive before that point.
		//s_SceneData->lightSSBO = StorageBuffer::create(sizeof(LightBufferHeader) + k_MaxPointLights * sizeof(PointLightGPU));
		//Renderer2D::init();
		//TextRenderer::init();
	}

	void Renderer::shutdown() {
		//DefaultLibrary::shutdown();
		ShaderLibrary::shutdown();
		s_SSceneUbo.reset();
		if (s_SceneData)
			s_SceneData->lightSSBO.reset();
		//TextRenderer::shutdown();
		//Renderer2D::shutdown();

		RenderCommand::shutdown();
	}

	void Renderer::beginScene(const math::Mat4& viewProjectionMatrix, const math::Vector3& cameraWorldPosition,
	                          const math::Vector3& direction, const math::Vector3& radiance) {
		//s_SceneData->lightEnvironment = environment;

		//uploadLightEnvironment();
		RenderCommand::beginScene(viewProjectionMatrix, cameraWorldPosition, direction, radiance);
	}

	void Renderer::endScene() {}

	void Renderer::submit(const Ref<Material>& material, const Ref<GeometryDescriptor>& desc, const Mat4& transform) {
		//material->getShader()->setMat4("u_Model", transform);
		RenderCommand::drawIndexed(material, desc, transform, desc->getIndexCount());
	}

	Ref<UniformBuffer> Renderer::getSceneUBO() {
		return s_SSceneUbo;
	}

	Ref<StorageBuffer> Renderer::getLightSSBO() {
		return s_SceneData->lightSSBO;
	}

	const void* Renderer::sceneUniformData() {
		return &s_AltSceneData;
	}

	uint32_t Renderer::sceneUniformSize() {
		return sizeof(AltSceneData);
	}

	void Renderer::onWindowResize(uint32_t width, uint32_t height) {
		RenderCommand::setViewport(0, 0, width, height);
	}

	void Renderer::uploadLightEnvironment() {
		// size_t count = s_SceneData->lightEnvironment.PointLights.size();
		// CBK_CORE_ASSERT(count <= k_MaxPointLights, "Point-light count exceeds k_MaxPointLights — bump the cap or cull lights upstream");
		// LightBufferHeader header;
		// header.count = static_cast<int>(count);

		// std::vector<uint8_t> bufferData;
		// bufferData.resize(sizeof(LightBufferHeader) + count * sizeof(PointLightGPU));

		// memcpy(bufferData.data(), &header, sizeof(LightBufferHeader));

		// PointLightGPU* currentLight = reinterpret_cast<PointLightGPU*>(bufferData.data() + sizeof(LightBufferHeader));

		// for (const auto& light: s_SceneData->lightEnvironment.PointLights) {
		// 	currentLight->position[0] = light.position.x;
		// 	currentLight->position[1] = light.position.y;
		// 	currentLight->position[2] = light.position.z;
		// 	currentLight->position[3] = 1.f;

		// 	currentLight->radiance[0] = light.radiance.x;
		// 	currentLight->radiance[1] = light.radiance.y;
		// 	currentLight->radiance[2] = light.radiance.z;
		// 	currentLight->radiance[3] = 0.f;

		// 	currentLight->constant = light.constant;
		// 	currentLight->linear = light.linear;
		// 	currentLight->quadratic = light.quadratic;

		// 	currentLight++;
		// }

		// if (s_SceneData->lightSSBO) {
		// 	s_SceneData->lightSSBO->setData(bufferData.data(), static_cast<uint32_t>(bufferData.size()));
		// 	s_SceneData->lightSSBO->bind(0);
		// }
	}
} // namespace cbk::rendering
