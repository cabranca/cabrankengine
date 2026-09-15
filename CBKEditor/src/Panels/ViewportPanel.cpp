#include "ViewportPanel.h"

#include <imgui.h>

#include <Cabrankengine/Renderer/RenderCommand.h>

namespace cbk::editor {

	namespace {
		constexpr float k_TargetAspect = 16.f / 9.f;
	}

	void ViewportPanel::onImGuiRender() {
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));
		begin();

		m_Focused = ImGui::IsWindowFocused();
		m_Hovered = ImGui::IsWindowHovered();

		// Fit the largest 16:9 rect inside the panel and centre it; the leftover
		// strips are just the window background showing through (letterboxing).
		const ImVec2 avail = ImGui::GetContentRegionAvail();
		ImVec2 image = { avail.x, avail.x / k_TargetAspect };
		if (image.y > avail.y)
			image = { avail.y * k_TargetAspect, avail.y };
		m_Size = { image.x, image.y };

		const ImVec2 origin = ImGui::GetCursorPos();
		ImGui::SetCursorPos({ origin.x + (avail.x - image.x) * 0.5f, origin.y + (avail.y - image.y) * 0.5f });

		// Backends that render straight to the backbuffer expose no texture to sample here.
		const auto finalFrame = static_cast<ImTextureID>(rendering::RenderCommand::getFinalFrame());
		if (finalFrame != ImTextureID_Invalid)
			ImGui::Image(finalFrame, image);

		end();
		ImGui::PopStyleVar();
	}

	void ViewportPanel::reset() {}
} // namespace cbk::editor
