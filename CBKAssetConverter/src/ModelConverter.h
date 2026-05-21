#pragma once

#include <string_view>

#include <Common/BinaryFormats.h>

namespace cbk::ac {

	class ModelConverter {
	  public:
		static void convert(std::string_view path);

		using Vertex = cbk::common::Vertex;
		using TextureType = cbk::common::TextureType;

	  private:
		using ModelHeader = cbk::common::ModelHeader;
		using MaterialHeader = cbk::common::MaterialHeader;
		using TextureEntry = cbk::common::TextureEntry;
		using PropertyEntry = cbk::common::PropertyEntry;
		using MeshHeader = cbk::common::MeshHeader;
	};
} // namespace cbk::ac
