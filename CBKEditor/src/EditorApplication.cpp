#include <pch.h>

#include <imgui.h>
#include <imgui_internal.h>

#include <Cabrankengine.h>

#include "Panels/DetailsPanel.h"
#include "Panels/OutlinerPanel.h"
#include "Panels/Panel.h"
#include "Panels/ViewportPanel.h"

// --- Entry Point ---
#include "Cabrankengine/Core/EntryPoint.h"

using namespace cbk;
using namespace cbk::ecs;
using namespace cbk::editor;
using namespace cbk::math;
using namespace cbk::rendering;
using namespace cbk::scene;
using namespace cbk::scene::arch;

class EditorLayer : public Layer {
  public:
	EditorLayer() : Layer("Editor") {
		Application::get().queueSceneLoad(SceneSerializer::deserialize("scenes/testScene.cbkscn"));

		m_Outliner = createRef<OutlinerPanel>();

		m_Panels.push_back(createRef<ViewportPanel>());
		m_Panels.push_back(m_Outliner);
		m_Panels.push_back(createRef<DetailsPanel>(m_Outliner));
	}

	void onUpdate(Timestep delta) override {}

	void onImGuiRender() override {
		drawDockspace();

		for (auto& panel: m_Panels)
			panel->onImGuiRender();
	}

  private:
	// Full-window host that owns the dockspace node every panel docks into.
	void drawDockspace() {
		const ImGuiViewport* viewport = ImGui::GetMainViewport();
		ImGui::SetNextWindowPos(viewport->WorkPos);
		ImGui::SetNextWindowSize(viewport->WorkSize);
		ImGui::SetNextWindowViewport(viewport->ID);

		ImGui::PushStyleVar(ImGuiStyleVar_WindowRounding, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowBorderSize, 0.f);
		ImGui::PushStyleVar(ImGuiStyleVar_WindowPadding, ImVec2(0.f, 0.f));

		constexpr ImGuiWindowFlags hostFlags =
		    ImGuiWindowFlags_NoDocking | ImGuiWindowFlags_NoTitleBar | ImGuiWindowFlags_NoCollapse | ImGuiWindowFlags_NoResize |
		    ImGuiWindowFlags_NoMove | ImGuiWindowFlags_NoBringToFrontOnFocus | ImGuiWindowFlags_NoNavFocus | ImGuiWindowFlags_MenuBar;

		ImGui::Begin("##EditorDockspaceHost", nullptr, hostFlags);
		ImGui::PopStyleVar(3);

		const ImGuiID dockspaceId = ImGui::GetID("EditorDockspace");
		Panel::setDockId(dockspaceId);

		// First run (no imgui.ini) or an explicit reset: lay the panels out ourselves.
		const bool needsLayout = m_ResetLayout || ImGui::DockBuilderGetNode(dockspaceId) == nullptr;
		ImGui::DockSpace(dockspaceId, ImVec2(0.f, 0.f), ImGuiDockNodeFlags_PassthruCentralNode);

		if (needsLayout) {
			buildDefaultLayout(dockspaceId);
			m_ResetLayout = false;
		}

		if (ImGui::BeginMenuBar()) {
			if (ImGui::BeginMenu("Scene")) {
				if (ImGui::MenuItem("New Scene")) {
					Application::get().queueSceneLoad(scene::Scene());
					for (auto& panel: m_Panels)
						panel->reset();
				}
				if (ImGui::MenuItem("Save Scene")) {
					SceneSerializer::serialize(Application::get().getScene(), "scenes/testScene.cbkscn");
				}
				if (ImGui::MenuItem("Save Scene as...")) {
					char filename[1024];
					FILE *f = popen("zenity --file-selection --save", "r");
					if (f) {
						if (fgets(filename, 1024, f)) {
							filename[strlen(filename)-1] = '\0'; // Zenity sets a \n after the filename in stdout so I have to trim it
							SceneSerializer::serialize(Application::get().getScene(), filename);
						}
						pclose(f);
					}

				}
				if (ImGui::MenuItem("Load Scene")) {
					// Source - https://stackoverflow.com/a/30431988
					// Posted by Ziming Song, modified by community. See post 'Timeline' for change history
					// Retrieved 2026-09-14, License - CC BY-SA 4.0
					char filename[1024];
					FILE *f = popen("zenity --file-selection", "r");
					if (f) {
						if (fgets(filename, 1024, f)) {
							filename[strlen(filename)-1] = '\0'; // Zenity sets a \n after the filename in stdout so I have to trim it
							Application::get().queueSceneLoad(SceneSerializer::deserialize(filename));
							for (auto& panel: m_Panels)
								panel->reset();
						}
						pclose(f);
					}
				}
				ImGui::EndMenu();
			}
			if (ImGui::BeginMenu("Window")) {
				if (ImGui::MenuItem("Reset Layout to default"))
					m_ResetLayout = true;
				ImGui::EndMenu();
			}
			ImGui::EndMenuBar();
		}

		ImGui::End();
	}

	// Outliner on the left, Details on the right, Viewport filling the centre.
	static void buildDefaultLayout(ImGuiID dockspaceId) {
		ImGui::DockBuilderRemoveNode(dockspaceId);
		ImGui::DockBuilderAddNode(dockspaceId, ImGuiDockNodeFlags_DockSpace);
		ImGui::DockBuilderSetNodeSize(dockspaceId, ImGui::GetMainViewport()->WorkSize);

		ImGuiID centre = dockspaceId;
		const ImGuiID left = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Left, 0.20f, nullptr, &centre);
		const ImGuiID right = ImGui::DockBuilderSplitNode(centre, ImGuiDir_Right, 0.25f, nullptr, &centre);

		ImGui::DockBuilderDockWindow("Outliner", left);
		ImGui::DockBuilderDockWindow("Details", right);
		ImGui::DockBuilderDockWindow("Viewport", centre);
		ImGui::DockBuilderFinish(dockspaceId);
	}

	Ref<OutlinerPanel> m_Outliner;
	std::vector<Ref<Panel>> m_Panels;
	bool m_ResetLayout = false;
};

class Editor : public Application {
  public:
	Editor() : Application(true) {
		pushLayer(createScope<EditorLayer>());
	}
};

Application* cbk::createApplication() {
	return new Editor();
}
