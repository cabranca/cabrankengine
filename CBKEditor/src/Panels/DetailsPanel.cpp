#include <pch.h>
#include "DetailsPanel.h"

#include <imgui.h>

#include <Cabrankengine/Core/Application.h>
#include <Cabrankengine/ECS/Components.h>

namespace cbk::editor {

	using namespace ecs;

	void createTransformPanel(CTransform* transform) {
		ImGui::InputFloat3("Position", &transform->Position.x);
		ImGui::InputFloat3("Rotation", &transform->Rotation.x);
		ImGui::InputFloat3("Scale", &transform->Scale.x);
	}

	DetailsPanel::DetailsPanel(const Ref<OutlinerPanel>& outliner) : Panel("Details"), m_Scene(Application::get().getScene()) {
		outliner->setSelectedEntityCallBack([this](ecs::Entity e) { m_SelectedEntity = e; });
	}

	void DetailsPanel::onImGuiRender() {
		begin();

		std::optional<CTransform*> transform = m_Scene.getRegistry()->getComponent<ecs::CTransform>(m_SelectedEntity);
		if (transform)
			createTransformPanel(transform.value());
		end();
	}

	void DetailsPanel::reset() {
		m_SelectedEntity = ecs::k_InvalidEntity;
	}
} // namespace cbk::editor
