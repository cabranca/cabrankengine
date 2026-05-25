#pragma once

namespace cbk {
	
	class Layer;

	// LayerStack is a class that manages a stack of layers and overlays.
	// It allows pushing and popping layers and overlays, maintaining the order of the update and event handling.
	class LayerStack {
		public:
			LayerStack();
			~LayerStack();

			// Pushes a layer to the stack on top of the other layers (always under the overlays)
			void pushLayer(Scope<Layer> layer);

			// Pushes a layer to the stack on top of the other overlays
			void pushOverlay(Scope<Layer> overlay);

			// Pops the top layer from the stack
			void popLayer(Layer* layer);

			// Pops the top overlay from the stack
			void popOverlay(Layer* overlay);

			// Getter for the iterator pointing to the beginning of the layer stack
			std::vector<Scope<Layer>>::iterator begin() { return m_Layers.begin(); }

			// Getter for the iterator pointing to the end of the layer stack
			std::vector<Scope<Layer>>::iterator end() { return m_Layers.end(); }

		private:
			std::vector<Scope<Layer>> m_Layers; // Actual structure for the layers
			unsigned int m_LayerInsertIndex; // Custom iterator for the layers. The idea is to always push a new layer on top of other layers but under every overlay. For the overlays, push them always on top of everything
	};
}
