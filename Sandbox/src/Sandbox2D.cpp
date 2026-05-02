#include "Sandbox2D.h"

#include <imgui.h>

using namespace cbk;
using namespace cbk::ecs;
using namespace cbk::math;
using namespace cbk::rendering;
using namespace cbk::scene;
using namespace cbk::scene::arch;

Sandbox2D::Sandbox2D() : Layer("Sandbox2D") {}

void Sandbox2D::onAttach() {
	CBK_PROFILE_FUNCTION();

	CameraArch camera(ProjectionType::Orthographic);
	camera.camera().Far = 1.f;
	camera.camera().Near = -1.f;

	SpriteArch box{"assets/textures/container2.cbkt"};
	box.transform().Scale = Vector3(500.f, 500.f, 0.f);
}

void Sandbox2D::onDetach() {
	CBK_PROFILE_FUNCTION();
}

void Sandbox2D::onUpdate(cbk::Timestep delta) {
	CBK_PROFILE_FUNCTION();

	{ CBK_PROFILE_SCOPE("Renderer Draw"); }
}

void Sandbox2D::onImGuiRender() {
	CBK_PROFILE_FUNCTION();
}

void Sandbox2D::onEvent(cbk::Event& e) {
	CBK_PROFILE_FUNCTION();
}
