#include <pch.h>
#include "LayerStack.h"

#include "Layer.h"

namespace cbk {

	LayerStack::LayerStack() : m_LayerInsertIndex(0) {}

	LayerStack::~LayerStack() {
		for (Scope<Layer>& layer: m_Layers)
			layer->onDetach();
	}

	void LayerStack::pushLayer(Scope<Layer> layer) {
		layer->onAttach();
		m_Layers.emplace(m_Layers.begin() + m_LayerInsertIndex++, std::move(layer));
	}

	void LayerStack::pushOverlay(Scope<Layer> overlay) {
		overlay->onAttach();
		m_Layers.emplace_back(std::move(overlay));
	}

	void LayerStack::popLayer(Layer* layer) {
		auto it = m_Layers.begin();
		while (it != m_Layers.end()) {
			if (it->get() == layer)
				break;
			it++;
		}
		if (it == m_Layers.end())
			return;

		m_Layers.erase(it);
		m_LayerInsertIndex--;
	}

	void LayerStack::popOverlay(Layer* overlay) {
		auto it = m_Layers.begin();
		while (it != m_Layers.end()) {
			if (it->get() == overlay)
				break;
			it++;
		}
		if (it == m_Layers.end())
			return;

		m_Layers.erase(it);
	}
} // namespace cbk
