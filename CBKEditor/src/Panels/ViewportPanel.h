#pragma once

#include <Common/Math/Vector2.h>

#include "Panel.h"

namespace cbk::editor {

	// Shows the engine's final rendered frame as an image inside a dock panel.
	// The focus/hover flags and cached size exist for wiring that comes later:
	// routing camera input only while the panel is active, and resizing the
	// render target to match the panel instead of stretching the frame.
	class ViewportPanel : public Panel {
	  public:
		ViewportPanel() : Panel("Viewport") {}

		void onImGuiRender() override;

		[[nodiscard]] bool isFocused() const {
			return m_Focused;
		}

		[[nodiscard]] bool isHovered() const {
			return m_Hovered;
		}

		[[nodiscard]] const math::Vector2& getSize() const {
			return m_Size;
		}

	  private:
		bool m_Focused = false;
		bool m_Hovered = false;
		math::Vector2 m_Size = math::Vector2::Zero;
	};
} // namespace cbk::editor
