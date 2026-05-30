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

#ifdef CBK_RENDERER_METAL

class ExampleLayer : public Layer {
  public:
	ExampleLayer() : Layer("Example"), m_CameraController(PerspectiveCamera(PI / 4.f, 16.f / 9.f, 0.1f, 100.f)) {
		m_ShaderLibrary.load("assets/shaders/Triangle");

		// --- Vertex data: position (Float3) + color (Float4) ---
		float vertices[] = {
			// x      y      z       r     g     b     a
			-0.5f, -0.5f, 0.0f, 1.0f, 0.0f, 0.0f, 1.0f, // bottom-left  (red)
			0.5f,  -0.5f, 0.0f, 0.0f, 1.0f, 0.0f, 1.0f, // bottom-right (green)
			0.0f,  0.5f,  0.0f, 0.0f, 0.0f, 1.0f, 1.0f, // top-center   (blue)
		};

		auto vb = VertexBuffer::create(vertices, sizeof(vertices));
		vb->setLayout({ { ShaderDataType::Float3, "a_Position" }, { ShaderDataType::Float4, "a_Color" } });

		uint32_t indices[] = { 0, 1, 2 };
		auto ib = IndexBuffer::create(indices, 3);

		m_VertexArray = VertexArray::create();
		m_VertexArray->addVertexBuffer(vb);
		m_VertexArray->setIndexBuffer(ib);
	}

	void onUpdate(Timestep delta) override {
		RenderCommand::setClearColor({ 0.1f, 0.1f, 0.1f, 1.f });
		RenderCommand::clear();

		auto shader = m_ShaderLibrary.get("Triangle");
		Renderer::submit(shader, m_VertexArray, identityMat());
		Renderer::endScene();
	}

	void onImGuiRender() override {}

  private:
	CameraController m_CameraController;
	ShaderLibrary m_ShaderLibrary;
	Ref<VertexArray> m_VertexArray;
};
#endif

#if defined(CBK_RENDERER_OPENGL) || defined(CBK_RENDERER_VULKAN)
class ExampleLayer : public Layer {
  public:
	ExampleLayer() : Layer("Example") {
		// Application::get().loadScene(Scene(SceneMetadata{ .Name = "Showcase", .BackgroundColor = { 0.05f, 0.05f, 0.05f, 1.f } }));

		auto* reg = Application::get().getRegistry();

		// Sponza is authored in centimeters — scale down to meters
		PBRModelArch sponza{ "assets/models/sponza/Sponza.cbkm" };
		PBRModelArch curtains{ "assets/models/sponza_curtains/Curtains.cbkm" };

		// Start camera inside the main hall, looking down the corridor
		CameraControllerArch camera(ProjectionType::Perspective);
		camera.transform().Position = { 0.f, 1.8f, 0.f };
		camera.camera().Far = 200.f;   // Sponza is deep; default 100 clips it
		camera.controller().FreeFlight = true; // fly freely along the look direction

		// Key light — created manually so onImGuiRender can edit it
		m_SunEntity = reg->createEntity();
		reg->addComponent<CDirectionalLight>(m_SunEntity,
		                                     CDirectionalLight{ .Direction = { 1.f, -1.2f, 0.f }, .Radiance = { 4.f, 3.7f, 3.1f } });

		// // Rim point light — created manually for the same reason
		// m_RimEntity = reg->createEntity();
		// reg->addComponent<CTransform>(m_RimEntity, CTransform{ .Position = { 0.f, 5.f, -4.f } });
		// reg->addComponent<CPointLight>(m_RimEntity, CPointLight{
		//     .Radiance  = { 1.f, 0.5f, 0.2f },
		//     .Linear    = 0.07f,
		//     .Quadratic = 0.017f
		// });
	}

	void onUpdate(Timestep delta) override {
		CBK_PROFILE_FUNCTION();

		if (!m_AnimateSun)
			return;

		m_Time += delta;
		float angle = m_Time * 0.1f;
		if (auto light = Application::get().getRegistry()->getComponent<CDirectionalLight>(m_SunEntity))
			(*light)->Direction = { std::cos(angle), -1.2f, std::sin(angle) };
	}

	void onImGuiRender() override {
		CBK_PROFILE_FUNCTION();

		auto* reg = Application::get().getRegistry();

		ImGui::Begin("Lights");

		if (ImGui::CollapsingHeader("Sun (Directional)", ImGuiTreeNodeFlags_DefaultOpen)) {
			ImGui::Checkbox("Animate", &m_AnimateSun);

			if (auto light = reg->getComponent<CDirectionalLight>(m_SunEntity)) {
				ImGui::DragFloat3("Direction", &(*light)->Direction.x, 0.01f, -1.f, 1.f);
				ImGui::ColorEdit3("Radiance", &(*light)->Radiance.x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
			}
		}

		// if (ImGui::CollapsingHeader("Rim (Point)", ImGuiTreeNodeFlags_DefaultOpen)) {
		// 	if (auto t = reg->getComponent<CTransform>(m_RimEntity))
		// 		ImGui::DragFloat3("Position",  &(*t)->Position.x, 0.1f);
		// 	if (auto pl = reg->getComponent<CPointLight>(m_RimEntity)) {
		// 		ImGui::ColorEdit3("Radiance",  &(*pl)->Radiance.x, ImGuiColorEditFlags_HDR | ImGuiColorEditFlags_Float);
		// 		ImGui::DragFloat("Linear",     &(*pl)->Linear,     0.001f, 0.f, 1.f);
		// 		ImGui::DragFloat("Quadratic",  &(*pl)->Quadratic,  0.001f, 0.f, 1.f);
		// 	}
		// }

		ImGui::End();
	}

  private:
	Entity m_SunEntity = INVALID_ENTITY;
	Entity m_RimEntity = INVALID_ENTITY;
	float m_Time = 0.f;
	bool m_AnimateSun = true;
};
#endif

class Sandbox : public Application {
  public:
	Sandbox() {
		pushLayer(createScope<ExampleLayer>());
		//pushLayer(createScope<Sandbox2D>());
	}
	~Sandbox() {}
};

Application* cbk::createApplication() {
	return new Sandbox();
}
