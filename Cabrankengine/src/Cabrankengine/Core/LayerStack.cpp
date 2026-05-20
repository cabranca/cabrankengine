#include <pch.h>
#include "LayerStack.h"

#include "Layer.h"

namespace cbk {

	LayerStack::LayerStack() : m_LayerInsertIndex(0)
	{
	}

	LayerStack::~LayerStack()
	{
		// onDetach releases backend resources (e.g. ImGui's Vulkan objects). It must
		// run before the renderer device is gone — ~LayerStack precedes the Window's
		// destruction, so the device is still alive here.
		for (Layer* layer : m_Layers) {
			layer->onDetach();
			delete layer;
		}
	}

	void LayerStack::pushLayer(Layer* layer)
	{
		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex++, layer);
	}

	void LayerStack::pushOverlay(Layer* overlay)
	{
		m_Layers.emplace_back(overlay);
	}

	void LayerStack::popLayer(Layer* layer)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), layer);
		if (it != m_Layers.end()) {
			m_Layers.erase(it);
			m_LayerInsertIndex--;
		}
	}

	void LayerStack::popOverlay(Layer* overlay)
	{
		auto it = std::find(m_Layers.begin(), m_Layers.end(), overlay);
		if (it != m_Layers.end())
			m_Layers.erase(it);
	}
}
