#pragma once

#include <string>
#include <utility>

#include <imgui.h>

namespace cbk::editor {

	// Base class for editor dock panels. Each panel owns a single ImGui window and is
	// driven once per frame by EditorLayer, after the dockspace host has been submitted.
	//
	// Panels open their window through begin()/end() rather than ImGui::Begin/End so the
	// base can keep them inside the dockspace: a panel dragged onto another split docks
	// there normally, but one released over empty space is pulled back to the dockspace
	// root on the next frame instead of being left floating.
	class Panel {
	  public:
		explicit Panel(std::string name) : m_Name(std::move(name)) {}
		virtual ~Panel() = default;

		virtual void onImGuiRender() = 0;

		virtual void reset() = 0;

		[[nodiscard]] const std::string& getName() const {
			return m_Name;
		}

		// Dockspace node undocked panels are snapped back into. Set once by EditorLayer.
		static void setDockId(ImGuiID dockId) {
			s_DockId = dockId;
		}

	  protected:
		bool begin(ImGuiWindowFlags flags = 0) {
			// Merging into a tab bar or splitting off a new node both leave the window
			// briefly "undocked" while ImGui's drag-and-drop machinery finishes reparenting
			// it — a merge usually lands the same frame the mouse releases, a new split can
			// take a few more. Snapping back the instant we see !m_Docked races that (and
			// forcing it on the exact merge frame trips DockNodeAddWindow's assert, since
			// both operations then target the same window at once). So only a panel that
			// has stayed undocked, with the mouse up, for several consecutive frames is
			// treated as genuinely dropped on empty space and pulled back into the dockspace.
			if (ImGui::IsMouseDown(ImGuiMouseButton_Left))
				m_UndockedFrames = 0; // still mid-drag (or an unrelated click) — don't count yet
			else
				m_UndockedFrames = m_Docked ? 0 : m_UndockedFrames + 1;

			constexpr int settleFrames = 5;
			if (m_UndockedFrames >= settleFrames && s_DockId != 0)
				ImGui::SetNextWindowDockID(s_DockId);

			const bool visible = ImGui::Begin(m_Name.c_str(), nullptr, flags);
			m_Docked = ImGui::IsWindowDocked();
			return visible;
		}

		void end() {
			ImGui::End();
		}

		std::string m_Name;

	  private:
		static inline ImGuiID s_DockId = 0;

		bool m_Docked = true;
		int m_UndockedFrames = 0;
	};
} // namespace cbk::editor
