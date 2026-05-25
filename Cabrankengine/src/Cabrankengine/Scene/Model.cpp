#include <pch.h>
#include "Model.h"

#include <fstream>

namespace cbk::scene {

	using cbk::common::MaterialHeader;
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
		if (header.version != 3) {
			CBK_CORE_ERROR("Unsupported .cbkm version {0} in {1} - v3 expected (re-run CBKAssetConverter)", header.version, path);
			return;
		}

		std::string directory = path.substr(0, path.find_last_of('/'));

		// Material table: each entry gets its own Material instance, cloned from
		// the prototype passed in by the archetype.
		std::vector<Ref<rendering::Material>> materials;
		materials.reserve(header.numMaterials);

		// Materials reference overlapping image files; load each GPU texture once.
		std::unordered_map<std::string, Ref<rendering::Texture2D>> textureCache;
		for (uint32_t i = 0; i < header.numMaterials; i++) {
			MaterialHeader matHeader;
			if (!file.read(reinterpret_cast<char*>(&matHeader), sizeof(MaterialHeader))) {
				CBK_CORE_ERROR("Failed to read material header {0} from {1}", i, path);
				return;
			}

			Ref<rendering::Material> material = m_Material->instantiate();
			if (!material)
				material = m_Material;

			for (uint32_t t = 0; t < matHeader.numTextures; t++) {
				TextureEntry entry;
				if (!file.read(reinterpret_cast<char*>(&entry), sizeof(TextureEntry))) {
					CBK_CORE_ERROR("Failed to read texture entry {0} from {1}", t, path);
					return;
				}

				std::string texPath(entry.pathLength, '\0');
				if (!file.read(texPath.data(), entry.pathLength)) {
					CBK_CORE_ERROR("Failed to read texture path {0} from {1}", t, path);
					return;
				}

				std::string fullPath = directory + "/" + texPath;
				auto& texture = textureCache[fullPath];
				if (!texture)
					texture = rendering::Texture2D::create(fullPath);
				material->applyTexture(entry.type, texture);
			}

			for (uint32_t p = 0; p < matHeader.numProperties; p++) {
				PropertyEntry prop;
				if (!file.read(reinterpret_cast<char*>(&prop), sizeof(PropertyEntry))) {
					CBK_CORE_ERROR("Failed to read property entry {0} from {1}", p, path);
					return;
				}
				material->applyProperty(prop.key, prop.value);
			}

			materials.push_back(material);
		}

		for (uint32_t i = 0; i < header.numMeshes; i++) {
			MeshHeader mh;
			if (!file.read(reinterpret_cast<char*>(&mh), sizeof(MeshHeader))) {
				CBK_CORE_ERROR("Failed to read mesh header {0} from {1}", i, path);
				return;
			}

			std::vector<rendering::Vertex> vertices(mh.numVertices);
			if (!file.read(reinterpret_cast<char*>(vertices.data()), mh.numVertices * sizeof(rendering::Vertex))) {
				CBK_CORE_ERROR("Failed to read mesh vertices {0} from {1}", i, path);
				return;
			}

			std::vector<uint32_t> indices(mh.numIndices);
			if (!file.read(reinterpret_cast<char*>(indices.data()), mh.numIndices * sizeof(uint32_t))) {
				CBK_CORE_ERROR("Failed to read mesh indices {0} from {1}", i, path);
				return;
			}

			CBK_CORE_ASSERT(mh.materialIndex < materials.size(), "Invalid material index for model {}!", path);
			
			m_Meshes.emplace_back(std::move(vertices), std::move(indices), materials[mh.materialIndex]);
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
