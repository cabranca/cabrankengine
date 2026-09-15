#pragma once

#include <Cabrankengine/ECS/Common.h>

#include "Panel.h"

namespace cbk::editor {

	// Lists the entities in the active scene. No data wired yet.
	class OutlinerPanel : public Panel {
	  public:
		OutlinerPanel() : Panel("Outliner") {}

		void onImGuiRender() override;

		void reset() override;

		void setSelectedEntityCallBack(const std::function<void(ecs::Entity)>& callback);

	  private:
		ecs::Entity m_SelectedEntity = ecs::k_InvalidEntity;
		std::vector<std::function<void(ecs::Entity)>> m_Callbacks;
	};
} // namespace cbk::editor
