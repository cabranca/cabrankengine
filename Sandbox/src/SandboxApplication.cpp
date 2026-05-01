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

#ifdef CBK_RENDERER_OPENGL
class ExampleLayer : public Layer {
  public:
	ExampleLayer() : Layer("Example") {
		CameraControllerArch cameraController(ProjectionType::Perspective);

		PBRModelArch gun{};
		gun.transform().Position = { 2.f, -2.f, 2.f };
		gun.transform().Rotation = { -90.f, 0.f, 0.f };
		gun.transform().Scale = Vector3(0.1f);
		gun.model().Model = Model<PBRMaterial>::create("assets/models/gun/Cerberus_LP.cbkm");

		PhongModelArch backpack{};
		backpack.model().Model = Model<PhongMaterial>::create("assets/models/backpack/backpack.cbkm");

		TextArch text{};
		text.transform().Position = { 5.f, 0.f, 0.f };
		text.text().Text = "ALLA LA ESTAN RENDERIZANDO";
		text.text().Color = { 1.f, 1.f, 1.0f, 1.f };

		DirectionalLightArch dirLight{};
		dirLight.light().Direction = { 0.0f, -1.0f, 0.0f };
		dirLight.light().Radiance = { 0.5f, 0.5f, 0.5f };

		PointLightArch lamp{};
		lamp.transform().Position = { 0.0f, 0.0f, 8.0f };
	}

	void onUpdate(Timestep delta) override {
		CBK_PROFILE_FUNCTION();
	}

	void onImGuiRender() override {
		CBK_PROFILE_FUNCTION();
	}
};
#endif

class Sandbox : public Application {
  public:
	Sandbox() {
		pushLayer(new ExampleLayer());
		// pushLayer(new Sandbox2D());
	}
	~Sandbox() {}
};

Application* cbk::createApplication() {
	return new Sandbox();
}
