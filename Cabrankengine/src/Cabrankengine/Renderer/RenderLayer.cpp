#include <pch.h>
#include "RenderLayer.h"

#include <Cabrankengine/ECS/Components.h>

#include "Renderer.h"
#include "Renderer2D.h"
#include "RenderCommand.h"
#include "TextRenderer.h"

namespace cbk::rendering {

	using namespace ecs;
	using namespace scene;

	RenderLayer::RenderLayer() {
		CBK_CORE_ASSERT(!s_Instance, "RenderLayer already exists!");
		s_Instance = this;
	}

	void RenderLayer::onAttach() {
		if (m_Scene)
			loadRegistry();
	}

	void RenderLayer::onUpdate(Timestep dt) {
		RenderCommand::setClearColor(m_Scene->getMetadata().BackgroundColor);
		RenderCommand::clear();

		m_CameraControllerSystem->update(*m_Scene->getRegistry(), dt);
		m_CameraSystem->update(*m_Scene->getRegistry(), dt);

		Renderer2D::beginScene(m_CameraSystem->getViewProjectionMatrix());
		m_SpriteRenderSystem->update(*m_Scene->getRegistry(), dt);
		Renderer2D::endScene();

		LightEnvironment lights;
		for (auto e : m_DirLightSystem->getEntities()) {
			auto dl = m_Scene->getRegistry()->getComponent<ecs::CDirectionalLight>(e).value();
			lights.DirLight = { dl->Direction, dl->Radiance };
			break;
		}
		for (auto e : m_PointLightSystem->getEntities()) {
			auto transform = m_Scene->getRegistry()->getComponent<ecs::CTransform>(e).value();
			auto pl = m_Scene->getRegistry()->getComponent<ecs::CPointLight>(e).value();
			lights.PointLights.push_back({ transform->Position, pl->Radiance, pl->Constant, pl->Linear, pl->Quadratic });
		}

		Renderer::beginScene(m_CameraSystem->getViewProjectionMatrix(), m_CameraSystem->getCameraWorldPosition(), lights);
		m_PhongRenderSystem->update(*m_Scene->getRegistry(), dt);
		m_PBRRenderSystem->update(*m_Scene->getRegistry(), dt);
		Renderer::endScene();

		TextRenderer::beginScene(m_CameraSystem->getViewProjectionMatrix());
		m_TextRenderSystem->update(*m_Scene->getRegistry(), dt);
		TextRenderer::endScene();
	}

	void RenderLayer::onEvent(Event& event) {
		EventDispatcher dispatcher(event);
		dispatcher.dispatch<WindowResizeEvent>(BIND_EVENT_FN(&RenderLayer::onWindowResize, this));
	}

	void RenderLayer::setScene(Scene* scene) {
		s_Instance->m_Scene = scene;
	}

	void RenderLayer::loadRegistry() {
		m_Scene->getRegistry()->registerComponent<CTransform>();
		m_Scene->getRegistry()->registerComponent<CCamera>();
		m_Scene->getRegistry()->registerComponent<CCameraController>();
		m_Scene->getRegistry()->registerComponent<CDirectionalLight>();
		m_Scene->getRegistry()->registerComponent<CPointLight>();
		m_Scene->getRegistry()->registerComponent<CSprite>();
		m_Scene->getRegistry()->registerComponent<CPhongModel>();
		m_Scene->getRegistry()->registerComponent<CPBRModel>();
		m_Scene->getRegistry()->registerComponent<CText>();

		m_CameraSystem = m_Scene->getRegistry()->registerSystem<CameraSystem>();
		m_CameraControllerSystem = m_Scene->getRegistry()->registerSystem<CameraControllerSystem>();
		m_DirLightSystem = m_Scene->getRegistry()->registerSystem<DirectionalLightSystem>();
		m_PointLightSystem = m_Scene->getRegistry()->registerSystem<PointLightSystem>();
		m_SpriteRenderSystem = m_Scene->getRegistry()->registerSystem<SpriteRenderSystem>();
		m_PhongRenderSystem = m_Scene->getRegistry()->registerSystem<PhongRenderSystem>();
		m_PBRRenderSystem = m_Scene->getRegistry()->registerSystem<PBRRenderSystem>();
		m_TextRenderSystem = m_Scene->getRegistry()->registerSystem<TextRenderSystem>();

		Signature sig;
		sig.set(m_Scene->getRegistry()->getComponentType<CTransform>());
		sig.set(m_Scene->getRegistry()->getComponentType<CCamera>());
		m_Scene->getRegistry()->setSystemSignature<CameraSystem>(sig);

		sig.reset();
		sig.set(m_Scene->getRegistry()->getComponentType<CTransform>());
		sig.set(m_Scene->getRegistry()->getComponentType<CCameraController>());
		m_Scene->getRegistry()->setSystemSignature<CameraControllerSystem>(sig);

		sig.reset();
		sig.set(m_Scene->getRegistry()->getComponentType<CDirectionalLight>());
		m_Scene->getRegistry()->setSystemSignature<DirectionalLightSystem>(sig);

		sig.reset();
		sig.set(m_Scene->getRegistry()->getComponentType<CTransform>());
		sig.set(m_Scene->getRegistry()->getComponentType<CPointLight>());
		m_Scene->getRegistry()->setSystemSignature<PointLightSystem>(sig);

		sig.reset();
		sig.set(m_Scene->getRegistry()->getComponentType<CTransform>());
		sig.set(m_Scene->getRegistry()->getComponentType<CSprite>());
		m_Scene->getRegistry()->setSystemSignature<SpriteRenderSystem>(sig);

		sig.reset();
		sig.set(m_Scene->getRegistry()->getComponentType<CTransform>());
		sig.set(m_Scene->getRegistry()->getComponentType<CPhongModel>());
		m_Scene->getRegistry()->setSystemSignature<PhongRenderSystem>(sig);

		sig.reset();
		sig.set(m_Scene->getRegistry()->getComponentType<CTransform>());
		sig.set(m_Scene->getRegistry()->getComponentType<CPBRModel>());
		m_Scene->getRegistry()->setSystemSignature<PBRRenderSystem>(sig);

		sig.reset();
		sig.set(m_Scene->getRegistry()->getComponentType<CTransform>());
		sig.set(m_Scene->getRegistry()->getComponentType<CText>());
		m_Scene->getRegistry()->setSystemSignature<TextRenderSystem>(sig);
	}

	bool RenderLayer::onWindowResize(WindowResizeEvent& event) {
		return m_CameraSystem->onWindowResize(event);
	}
} // namespace cbk::rendering
