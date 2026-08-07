#include "Sandbox2D.h"

#include <imgui.h>

#include <cmath>
#include <vector>

using namespace cbk;
using namespace cbk::ecs;
using namespace cbk::math;
using namespace cbk::rendering;
using namespace cbk::scene;
using namespace cbk::scene::arch;

namespace {
	constexpr int k_GridX = 100;
	constexpr int k_GridY = 100;
	constexpr float k_QuadSize = 14.f;
	constexpr float k_QuadSpacing = 16.f;

	struct QuadInstance {
		Entity Id;
		float PhaseOffset; // staggers rotation/color per quad
	};

	std::vector<QuadInstance> s_Quads;
	Ref<Texture2D> s_SharedTexture;
	float s_Time = 0.f;
} // namespace

Sandbox2D::Sandbox2D() : Layer("Sandbox2D") {}

void Sandbox2D::onAttach() {
	CBK_PROFILE_FUNCTION();

	Application::get().getWindow().setVSync(false);

	CameraArch camera(ProjectionType::Orthographic);
	camera.camera().Far = 1.f;
	camera.camera().Near = -1.f;
	camera.camera().OrthoSize = (k_GridY + 4) * k_QuadSpacing;

	// One texture, shared by every sprite — this is what lets the SpriteRenderSystem
	// emit a single batched draw call instead of one per quad.
	s_SharedTexture = Texture2D::create("assets/textures/awesomeface.cbkt");

	auto* reg = Application::get().getRegistry();
	auto& scene = Application::get().getScene();

	s_Quads.reserve(k_GridX * k_GridY);

	const float originX = -(k_GridX - 1) * k_QuadSpacing * 0.5f;
	const float originY = -(k_GridY - 1) * k_QuadSpacing * 0.5f;

	for (int y = 0; y < k_GridY; ++y) {
		for (int x = 0; x < k_GridX; ++x) {
			Entity e = scene.createEntity("Quad");
			reg->addComponent(e, CTransform{
			                         .Position = { originX + x * k_QuadSpacing, originY + y * k_QuadSpacing, 0.f },
			                         .Scale = { k_QuadSize, k_QuadSize, 0.f },
			                     });
			reg->addComponent(e, CSprite{
			                         .Path = "assets/textures/container2.cbkt",
			                         .Texture = s_SharedTexture,
			                         .Tint = Vector4(1.f),
			                     });

			const float phase = (static_cast<float>(x) + static_cast<float>(y)) * 6.f;
			s_Quads.push_back({ .Id = e, .PhaseOffset = phase });
		}
	}
}

void Sandbox2D::onDetach() {
	CBK_PROFILE_FUNCTION();

	auto* reg = Application::get().getRegistry();
	for (const auto& q: s_Quads)
		reg->destroyEntity(q.Id);

	s_Quads.clear();
	s_SharedTexture.reset();
}

void Sandbox2D::onUpdate(cbk::Timestep delta) {
	CBK_PROFILE_FUNCTION();

	s_Time += delta;

	auto* reg = Application::get().getRegistry();

	for (const auto& q: s_Quads) {
		auto transform = reg->getComponent<CTransform>(q.Id).value();
		auto sprite = reg->getComponent<CSprite>(q.Id).value();

		// Rotation.z is fed straight into MatrixFactory::rotateZ, which interprets
		// its argument as DEGREES — so this gives 30°/sec spin staggered per quad.
		transform->Rotation.z = s_Time * 30.f + q.PhaseOffset;

		const float t = s_Time + q.PhaseOffset * (k_Pi / 180.f);
		sprite->Tint = Vector4(0.5f + 0.5f * std::sin(t),
		                       0.5f + 0.5f * std::sin(t + 2.094f), // +120°
		                       0.5f + 0.5f * std::sin(t + 4.188f), // +240°
		                       1.f);
	}
}

void Sandbox2D::onImGuiRender() {
	CBK_PROFILE_FUNCTION();

	const auto stats = Renderer2D::getStats();

	ImGui::Begin("Stats");
	ImGui::Text("FPS:        %.1f", ImGui::GetIO().Framerate);
	ImGui::Text("Frame time: %.3f ms", 1000.f / ImGui::GetIO().Framerate);
	ImGui::Separator();
	ImGui::Text("Quads:      %u", stats.QuadCount);
	ImGui::Text("Draw calls: %u", stats.DrawCalls);
	ImGui::End();

	Renderer2D::resetStats();
}

void Sandbox2D::onEvent(cbk::Event& e) {
	CBK_PROFILE_FUNCTION();
}
