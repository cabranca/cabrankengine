#include <pch.h>
#include "OutlinerPanel.h"

#include <imgui.h>

#include <Cabrankengine/Core/Application.h>

namespace cbk::editor {

	void OutlinerPanel::onImGuiRender() {
		begin();

		const auto& scene = Application::get().getScene();
		const auto& entities = scene.getAllEntities();

		for (uint32_t i = 0; i < entities.size(); i++) {
			if (ImGui::Selectable(scene.getEntityName(entities[i]).data(), i == m_SelectedEntity)) {
				m_SelectedEntity = i;
				for (const auto& callback: m_Callbacks)
					callback(m_SelectedEntity);
			}
		}

		end();
	}

	void OutlinerPanel::setSelectedEntityCallBack(const std::function<void(ecs::Entity)>& callback) {
		m_Callbacks.push_back(callback);
	}
} // namespace cbk::editor
