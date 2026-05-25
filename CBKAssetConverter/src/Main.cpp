#include <algorithm>
#include <cstdint>
#include <filesystem>
#include <string>

#include <Common/Logger.h>

#include "ModelConverter.h"
#include "TextureConverter.h"

int main(int argc, char** argv) {
	cbk::common::Logger::init();

	const char* file = nullptr;
	uint32_t maxTex = 0; // 0 = no texture size cap

	for (int i = 1; i < argc; i++) {
		std::string arg = argv[i];
		if (arg == "--max-tex" && i + 1 < argc)
			maxTex = static_cast<uint32_t>(std::stoul(argv[++i]));
		else if (!file)
			file = argv[i];
	}

	if (!file) {
		CBK_AC_ERROR("Usage: CBKAssetConverter <file> [--max-tex <N>]");
		return -1;
	}

	cbk::ac::TextureConverter::setMaxDimension(maxTex);

	std::string ext = std::filesystem::path(file).extension().string();
	std::ranges::transform(ext, ext.begin(), ::tolower);

	if (ext == ".obj" || ext == ".fbx" || ext == ".gltf" || ext == ".dae")
		cbk::ac::ModelConverter::convert(file);
	else if (ext == ".png" || ext == ".jpg" || ext == ".jpeg" || ext == ".tga" || ext == ".bmp" || ext == ".hdr")
		cbk::ac::TextureConverter::convert(file);
	else
		CBK_AC_ERROR("Unsupported file format: {}", ext);

	return 0;
}
