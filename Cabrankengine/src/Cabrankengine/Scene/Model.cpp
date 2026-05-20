#include <pch.h>
#include "Model.h"

#include <fstream>

#include <Cabrankengine/Renderer/Texture.h>

#include <Common/BinaryFormats.h>

namespace cbk::scene {

	using cbk::common::MeshHeader;
	using cbk::common::ModelHeader;
	using cbk::common::PropertyEntry;
	using cbk::common::TextureEntry;

	Model::Model(const std::string& path, const Ref<rendering::Material>& material) : m_Material(material) {
		std::error_code ec;
		if (!std::filesystem::exists(path, ec)) {
			CBK_CORE_ERROR("Cannot find model file {0} - Error: {1}", path, ec.message());
			return;
		}

		std::ifstream file(path, std::ios::binary);
		if (!file) {
			CBK_CORE_ERROR("Cannot open model file {0}", path);
			return;
		}

		ModelHeader header;
		if (!file.read(reinterpret_cast<char*>(&header), sizeof(ModelHeader))) {
			CBK_CORE_ERROR("Cannot read model header from {0}", path);
			return;
		}
		if (header.magic != 0x43424B4D) {
			CBK_CORE_ERROR("Invalid model file {0} - .cbkm expected!", path);
			return;
		}

		std::string directory = path.substr(0, path.find_last_of('/'));

		for (uint32_t i = 0; i < header.numTextures; i++) {
			TextureEntry entry;
			if (!file.read(reinterpret_cast<char*>(&entry), sizeof(TextureEntry))) {
				CBK_CORE_ERROR("Failed to read texture entry {0} from {1}", i, path);
				return;
			}

			std::string texPath(entry.pathLength, '\0');
			if (!file.read(texPath.data(), entry.pathLength)) {
				CBK_CORE_ERROR("Failed to read texture path {0} from {1}", i, path);
				return;
			}

			auto texture = rendering::Texture2D::create(directory + "/" + texPath);
			m_Material->applyTexture(entry.type, texture);
		}

		for (uint32_t i = 0; i < header.numProperties; i++) {
			PropertyEntry prop;
			if (!file.read(reinterpret_cast<char*>(&prop), sizeof(PropertyEntry))) {
				CBK_CORE_ERROR("Failed to read property entry {0} from {1}", i, path);
				return;
			}
			m_Material->applyProperty(prop.key, prop.value);
		}

		for (uint32_t i = 0; i < header.numMeshes; i++) {
			MeshHeader mh;
			if (!file.read(reinterpret_cast<char*>(&mh), sizeof(MeshHeader))) {
				CBK_CORE_ERROR("Failed to read mesh header {0} from {1}", i, path);
				return;
			}

			std::vector<cbk::common::Vertex> rawVertices(mh.numVertices);
			if (!file.read(reinterpret_cast<char*>(rawVertices.data()), mh.numVertices * sizeof(cbk::common::Vertex))) {
				CBK_CORE_ERROR("Failed to read mesh vertices {0} from {1}", i, path);
				return;
			}

			std::vector<uint32_t> indices(mh.numIndices);
			if (!file.read(reinterpret_cast<char*>(indices.data()), mh.numIndices * sizeof(uint32_t))) {
				CBK_CORE_ERROR("Failed to read mesh indices {0} from {1}", i, path);
				return;
			}

			std::vector<rendering::Vertex> vertices;
			vertices.reserve(mh.numVertices);
			for (const auto& rv : rawVertices) {
				vertices.push_back({ { rv.px, rv.py, rv.pz },
				                     { rv.nx, rv.ny, rv.nz },
				                     { rv.tx, rv.ty },
				                     { rv.tanx, rv.tany, rv.tanz } });
			}

			m_Meshes.emplace_back(std::move(vertices), std::move(indices), m_Material);
		}
	}

	void Model::draw(const math::Mat4& transform) {
		for (auto& mesh : m_Meshes)
			mesh.draw(transform);
	}

	Ref<Model> Model::create(const std::string& path, const Ref<rendering::Material>& material) {
		return createRef<Model>(path, material);
	}

} // namespace cbk::scene
