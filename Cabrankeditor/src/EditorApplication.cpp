#include <Cabrankengine.h>

// --- Entry Point ---
#include "Cabrankengine/Core/EntryPoint.h"

using namespace cbk;

class EditorLayer : public Layer {
  public:
	EditorLayer() : Layer("Example") {
		
	}

	void onUpdate(Timestep delta) override {
		CBK_PROFILE_FUNCTION();
	}

	void onImGuiRender() override {
		CBK_PROFILE_FUNCTION();
	}
};

class Editor : public Application {
  public:
	Editor() {
		pushLayer(new EditorLayer());
	}
	~Editor() {}
};

Application* cbk::createApplication() {
	return new Editor();
}
