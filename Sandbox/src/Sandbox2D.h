#pragma once

#include <Cabrankengine.h>

class Sandbox2D : public cbk::Layer {
  public:
	Sandbox2D();

	virtual void onAttach() override;
	virtual void onDetach() override;

	virtual void onUpdate(cbk::Timestep delta) override;
	virtual void onImGuiRender() override;
	virtual void onEvent(cbk::Event& e) override;
};
