#include <pch.h>
#include "RenderLayer.h"

#include <Cabrankengine/ECS/Components.h>

#include "Renderer.h"
#include "Renderer2D.h"
#include "RenderCommand.h"
#include "TextRenderer.h"

namespace cbk::rendering {

	using namespace ecs;
	using namespace math;
	using namespace scene;

	RenderLayer::RenderLayer() {
		CBK_CORE_ASSERT(!s_Instance, "RenderLayer already exists!");
		s_Instance = this;
	}

	void RenderLayer::onAttach() {}

	void RenderLayer::onUpdate(Timestep dt) {
		RenderCommand::setClearColor(m_Scene->getMetadata().BackgroundColor);
		RenderCommand::clear();

		Mat4    vp;
		Vector3 camPos;
		if (m_EditorMode) {
			m_EditorCamera.update(dt);
			vp     = m_EditorCamera.getViewProjectionMatrix();
			camPos = m_EditorCamera.getWorldPosition();
		} else {
			m_CameraControllerSystem->update(*m_Scene->getRegistry(), dt);
			m_CameraSystem->update(*m_Scene->getRegistry(), dt);
			vp     = m_CameraSystem->getViewProjectionMatrix();
			camPos = m_CameraSystem->getCameraWorldPosition();
		}

		Renderer2D::beginScene(vp);
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

		Renderer::beginScene(vp, camPos, lights);
		m_PhongRenderSystem->update(*m_Scene->getRegistry(), dt);
		m_PBRRenderSystem->update(*m_Scene->getRegistry(), dt);
		Renderer::endScene();

		TextRenderer::beginScene(vp);
		m_TextRenderSystem->update(*m_Scene->getRegistry(), dt);
		TextRenderer::endScene();
	}

	void RenderLayer::onEvent(Event& event) {
		EventDispatcher dispatcher(event);
		dispatcher.dispatch<WindowResizeEvent>(BIND_EVENT_FN(&RenderLayer::onWindowResize, this));
	}

	void RenderLayer::setScene(Scene* scene) {
		s_Instance->m_Scene = scene;
		s_Instance->loadRegistry();
	}

	void RenderLayer::setEditorMode(bool editorMode) {
		s_Instance->m_EditorMode = editorMode;
	}

	void RenderLayer::loadRegistry() {
		ecs::initDefaultSystems(*m_Scene->getRegistry());

		m_CameraSystem = m_Scene->getRegistry()->getSystem<CameraSystem>();
		m_CameraControllerSystem = m_Scene->getRegistry()->getSystem<CameraControllerSystem>();
		m_DirLightSystem = m_Scene->getRegistry()->getSystem<DirectionalLightSystem>();
		m_PointLightSystem = m_Scene->getRegistry()->getSystem<PointLightSystem>();
		m_SpriteRenderSystem = m_Scene->getRegistry()->getSystem<SpriteRenderSystem>();
		m_PhongRenderSystem = m_Scene->getRegistry()->getSystem<PhongRenderSystem>();
		m_PBRRenderSystem = m_Scene->getRegistry()->getSystem<PBRRenderSystem>();
		m_TextRenderSystem = m_Scene->getRegistry()->getSystem<TextRenderSystem>();

		m_Scene->getRegistry()->rebuildSystemMembership();
	}

	bool RenderLayer::onWindowResize(WindowResizeEvent& event) {
		if (m_EditorMode) {
			m_EditorCamera.onWindowResize(static_cast<float>(event.getWidth()) / event.getHeight());
			return false;
		}
		return m_CameraSystem->onWindowResize(event);
	}
} // namespace cbk::rendering
