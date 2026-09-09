#include <imgui.h>

#include <Cabrankengine.h>

// --- Entry Point ---
#include "Cabrankengine/Core/EntryPoint.h"


using namespace cbk;
using namespace cbk::ecs;
using namespace cbk::math;
using namespace cbk::rendering;
using namespace cbk::scene;
using namespace cbk::scene::arch;

class EditorLayer : public Layer {
  public:
	EditorLayer() : Layer("Example") {
		Application::get().loadScene(SceneSerializer::deserialize("testScene.cbkscn"));
	}

	void onUpdate(Timestep delta) override {}

	void onImGuiRender() override {
		// auto* reg = Application::get().getRegistry();
		// auto* dirLight = reg->getComponent<CDirectionalLight>(m_SunEntity).value();

		// auto* pointLightTrans = reg->getComponent<CTransform>(m_PointLight).value();
		// auto* pointLight = reg->getComponent<CPointLight>(m_PointLight).value();

		// ImGui::Begin("Lights");
		// ImGui::DragFloat3("Directional Light Direction", &(dirLight->Direction.x));
		// ImGui::ColorEdit3("Directional Light Position", &(dirLight->Radiance.x), ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
		// ImGui::Separator();
		// ImGui::InputFloat3("Point Light Position", &(pointLightTrans->Position.x));
		// ImGui::ColorEdit3("Point Light Position", &(pointLight->Radiance.x), ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
		// ImGui::End();

	}

  private:
	Entity m_SunEntity = k_InvalidEntity;
	Entity m_PointLight = k_InvalidEntity;
};

class Editor : public Application {
  public:
	Editor() : Application(true) {
		pushLayer(createScope<EditorLayer>());
	}
};

Application* cbk::createApplication() {
	return new Editor();
}
