#pragma once

#include <Cabrankengine/Scene/Scene.h>

#include "OutlinerPanel.h"

namespace cbk::editor {

	// Shows the components of the entity selected in the Outliner. No data wired yet.
	class DetailsPanel : public Panel {
	  public:
		explicit DetailsPanel(const Ref<OutlinerPanel>& outliner);

		void onImGuiRender() override;

		void reset() override;

	  private:
		scene::Scene& m_Scene;
		ecs::Entity m_SelectedEntity = ecs::k_InvalidEntity;
	};
} // namespace cbk::editor
