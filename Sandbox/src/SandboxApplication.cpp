#include <imgui.h>

#include <Cabrankengine.h>

// --- Entry Point ---
#include "Cabrankengine/Core/EntryPoint.h"

#include "Sandbox2D.h"

using namespace cbk;
using namespace cbk::ecs;
using namespace cbk::math;
using namespace cbk::rendering;
using namespace cbk::scene;
using namespace cbk::scene::arch;

class ExampleLayer : public Layer {
  public:
	ExampleLayer() : Layer("Example") {
		auto* reg = Application::get().getRegistry();

		// Sponza is authored in centimeters — scale down to meters
		// PBRModelArch sponza{ "assets/models/sponza/Sponza.cbkm" };
		// sponza.transform().Scale = Vector3(0.1f);
		// PBRModelArch curtains{ "assets/models/sponza_curtains/Curtains.cbkm" };

		PhongModelArch backpack{ "assets/models/backpack/backpack.cbkm" };

		// Start camera inside the main hall, looking down the corridor
		CameraControllerArch camera(ProjectionType::Perspective);
		camera.transform().Position = { 0.F, 1.8F, 0.F };
		camera.camera().Far = 200.F;           // Sponza is deep; default 100 clips it
		camera.controller().FreeFlight = true; // fly freely along the look direction

		// Key light — created manually so onImGuiRender can edit it
		m_SunEntity = reg->createEntity();
		reg->addComponent<CDirectionalLight>(m_SunEntity,
		                                     CDirectionalLight{ .Direction = { 1.F, -1.F, -1.F }, .Radiance = { 2.F, 2.F, 2.F } });
		m_PointLight = reg->createEntity();
		reg->addComponent(m_PointLight, CTransform{ .Position = { 0.F, 0.F, 2.F } });
		reg->addComponent(m_PointLight, CPointLight{ .Radiance = { 5.F, 0.F, 0.F } });
	}

	void onUpdate(Timestep delta) override {}

	void onImGuiRender() override {
		auto* reg = Application::get().getRegistry();
		auto* dirLight = reg->getComponent<CDirectionalLight>(m_SunEntity).value();

		auto* pointLightTrans = reg->getComponent<CTransform>(m_PointLight).value();
		auto* pointLight = reg->getComponent<CPointLight>(m_PointLight).value();

		ImGui::Begin("Lights");
		ImGui::DragFloat3("Directional Light Direction", &(dirLight->Direction.x));
		ImGui::ColorEdit3("Directional Light Position", &(dirLight->Radiance.x), ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
		ImGui::Separator();
		ImGui::InputFloat3("Point Light Position", &(pointLightTrans->Position.x));
		ImGui::ColorEdit3("Point Light Position", &(pointLight->Radiance.x), ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
		ImGui::End();
	}

  private:
	Entity m_SunEntity = k_InvalidEntity;
	Entity m_PointLight = k_InvalidEntity;
};

class Sandbox : public Application {
  public:
	Sandbox() {
		pushLayer(createScope<ExampleLayer>());
		// pushLayer(createScope<Sandbox2D>());
	}
	~Sandbox() {}
};

Application* cbk::createApplication() {
	return new Sandbox();
}
