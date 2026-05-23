#include "ModelConverter.h"
#include "TextureConverter.h"

#include <algorithm>
#include <filesystem>
#include <fstream>
#include <string>
#include <vector>

#include <Common/Logger.h>
#include <assimp/Importer.hpp>
#include <assimp/scene.h>
#include <assimp/postprocess.h>

namespace cbk::ac {

	struct CollectedMesh {
		std::vector<ModelConverter::Vertex> vertices;
		std::vector<uint32_t> indices;
		uint32_t materialIndex = 0;
	};

	struct TextureRef {
		ModelConverter::TextureType type;
		std::string relativePath; // relative to model directory, with .cbkt extension
	};

	struct PropertyRef {
		uint32_t key;
		float value;
	};

	// One material's worth of textures + scalar properties.
	struct CollectedMaterial {
		std::vector<TextureRef> textures;
		std::vector<PropertyRef> properties;
	};

	static void collectMeshes(aiNode* node, const aiScene* scene, std::vector<CollectedMesh>& meshes) {
		for (unsigned int i = 0; i < node->mNumMeshes; i++) {
			aiMesh* aiM = scene->mMeshes[node->mMeshes[i]];
			CollectedMesh cm;
			cm.materialIndex = aiM->mMaterialIndex;

			for (unsigned int v = 0; v < aiM->mNumVertices; v++) {
				ModelConverter::Vertex vert{};
				vert.position = { aiM->mVertices[v].x, aiM->mVertices[v].y, aiM->mVertices[v].z };

				if (aiM->HasNormals())
					vert.normal = { aiM->mNormals[v].x, aiM->mNormals[v].y, aiM->mNormals[v].z };

				if (aiM->mTextureCoords[0])
					vert.texCoords = { aiM->mTextureCoords[0][v].x, aiM->mTextureCoords[0][v].y };

				if (aiM->HasTangentsAndBitangents())
					vert.tangent = { aiM->mTangents[v].x, aiM->mTangents[v].y, aiM->mTangents[v].z };

				cm.vertices.push_back(vert);
			}

			for (unsigned int f = 0; f < aiM->mNumFaces; f++) {
				aiFace& face = aiM->mFaces[f];
				for (unsigned int j = 0; j < face.mNumIndices; j++)
					cm.indices.push_back(face.mIndices[j]);
			}

			meshes.push_back(std::move(cm));
		}

		for (unsigned int i = 0; i < node->mNumChildren; i++)
			collectMeshes(node->mChildren[i], scene, meshes);
	}

	// Collects the textures of a single material. convertedFiles is a model-wide
	// set so the same source image is only written to .cbkt once, even when shared.
	static void collectMaterialTextures(aiMaterial* mat, const std::string& modelDir,
	                                     std::vector<TextureRef>& textures,
	                                     std::vector<std::string>& convertedFiles) {
		auto convertOnce = [&](const std::string& srcPath) {
			if (std::ranges::find(convertedFiles, srcPath) != convertedFiles.end())
				return;
			convertedFiles.push_back(srcPath);
			TextureConverter::convert(srcPath);
		};

		std::vector<std::string> seen; // dedup within this material

		auto tryAdd = [&](aiTextureType aiType, ModelConverter::TextureType cbkType) {
			for (unsigned int i = 0; i < mat->GetTextureCount(aiType); i++) {
				aiString str;
				mat->GetTexture(aiType, i, &str);
				std::string filename(str.C_Str());

				// Normalize Windows backslashes
				std::ranges::replace(filename, '\\', '/');

				if (std::ranges::find(seen, filename) != seen.end())
					continue;
				seen.push_back(filename);

				convertOnce(modelDir + "/" + filename);

				std::filesystem::path rel(filename);
				rel.replace_extension(".cbkt");

				textures.push_back({ cbkType, rel.string() });
			}
		};

		using TT = ModelConverter::TextureType;
		// Phong texture types
		tryAdd(aiTextureType_DIFFUSE, TT::Diffuse);
		tryAdd(aiTextureType_SPECULAR, TT::Specular);
		// PBR texture types
		tryAdd(aiTextureType_BASE_COLOR, TT::Diffuse);
		tryAdd(aiTextureType_NORMALS, TT::Normal);
		tryAdd(aiTextureType_METALNESS, TT::MetalRoughness);
		tryAdd(aiTextureType_AMBIENT_OCCLUSION, TT::AO);

		const bool hasNormal = mat->GetTextureCount(aiTextureType_NORMALS) > 0;
		const bool hasMetalRough = mat->GetTextureCount(aiTextureType_METALNESS) > 0;
		const bool hasAO = mat->GetTextureCount(aiTextureType_AMBIENT_OCCLUSION) > 0;

		// PBR discovery heuristic: if we found a Diffuse/Albedo but no PBR textures,
		// try to find companion textures by naming convention (_N, _M, _R, _AO).
		if (textures.empty() || hasNormal || hasMetalRough || hasAO)
			return;

		std::string diffuseRel;
		for (const auto& tex : textures) {
			if (tex.type == TT::Diffuse) { diffuseRel = tex.relativePath; break; }
		}
		if (diffuseRel.empty())
			return;

		// Convert .cbkt back to original extension to find the base
		std::filesystem::path diffPath(diffuseRel);
		diffPath.replace_extension(""); // remove .cbkt
		std::string stem = diffPath.string();

		// Strip the _A suffix to get the base prefix
		std::string basePrefix;
		if (stem.size() >= 2 && stem.substr(stem.size() - 2) == "_A")
			basePrefix = stem.substr(0, stem.size() - 2);
		if (basePrefix.empty())
			return;

		auto findTexture = [&](const std::string& suffix) -> std::string {
			for (const char* ext : { ".tga", ".png", ".jpg", ".jpeg", ".TGA", ".PNG", ".JPG" }) {
				std::string candidate = modelDir + "/" + basePrefix + suffix + ext;
				if (std::filesystem::exists(candidate))
					return candidate;
			}
			return "";
		};

		// Normal map
		std::string normalPath = findTexture("_N");
		if (!normalPath.empty()) {
			convertOnce(normalPath);
			std::filesystem::path rel(basePrefix + "_N");
			rel.replace_extension(".cbkt");
			textures.push_back({ TT::Normal, rel.string() });
			CBK_AC_INFO("Discovered Normal: {}", normalPath);
		}

		// Metalness + Roughness -> pack into combined MetalRough
		std::string metalPath = findTexture("_M");
		std::string roughPath = findTexture("_R");
		if (!metalPath.empty() && !roughPath.empty()) {
			std::filesystem::path outRel(basePrefix + "_MR");
			outRel.replace_extension(".cbkt");
			std::string outPath = modelDir + "/" + outRel.string();
			if (std::ranges::find(convertedFiles, outPath) == convertedFiles.end()) {
				convertedFiles.push_back(outPath);
				TextureConverter::packMetalRough(metalPath, roughPath, outPath);
			}
			textures.push_back({ TT::MetalRoughness, outRel.string() });
			CBK_AC_INFO("Discovered Metal+Rough: {} + {}", metalPath, roughPath);
		}

		// AO
		std::string aoPath = findTexture("_AO");
		if (!aoPath.empty()) {
			convertOnce(aoPath);
			std::filesystem::path rel(basePrefix + "_AO");
			rel.replace_extension(".cbkt");
			textures.push_back({ TT::AO, rel.string() });
			CBK_AC_INFO("Discovered AO: {}", aoPath);
		}
	}

	static void collectMaterialProperties(aiMaterial* mat, std::vector<PropertyRef>& properties) {
		float shininess;
		if (mat->Get(AI_MATKEY_SHININESS, shininess) == aiReturn_SUCCESS)
			properties.push_back({ 1, shininess });

		float metallic;
		if (mat->Get(AI_MATKEY_METALLIC_FACTOR, metallic) == aiReturn_SUCCESS)
			properties.push_back({ 2, metallic });

		float roughness;
		if (mat->Get(AI_MATKEY_ROUGHNESS_FACTOR, roughness) == aiReturn_SUCCESS)
			properties.push_back({ 3, roughness });

		aiColor4D baseColor;
		if (mat->Get(AI_MATKEY_BASE_COLOR, baseColor) == aiReturn_SUCCESS) {
			properties.push_back({ 4, baseColor.r });
			properties.push_back({ 5, baseColor.g });
			properties.push_back({ 6, baseColor.b });
		}
	}

	void ModelConverter::convert(std::string_view path) {
		Assimp::Importer importer;
		// PreTransformVertices bakes the node hierarchy's transforms into the mesh
		// vertices and flattens the scene — collectMeshes copies raw vertices and
		// does not apply aiNode::mTransformation, so without this glTF-style
		// scene-graph models render with wrong placement, orientation and scale.
		const aiScene* scene = importer.ReadFile(
		    path.data(),
		    aiProcess_Triangulate | aiProcess_GenSmoothNormals |
		    aiProcess_FlipUVs | aiProcess_CalcTangentSpace | aiProcess_PreTransformVertices);

		if (!scene || (scene->mFlags & AI_SCENE_FLAGS_INCOMPLETE) || !scene->mRootNode) {
			CBK_AC_ERROR("Failed to load model: {} - {}", path, importer.GetErrorString());
			return;
		}

		std::string modelDir(path.substr(0, path.find_last_of('/')));

		std::vector<CollectedMesh> meshes;
		collectMeshes(scene->mRootNode, scene, meshes);

		// One CollectedMaterial per aiMaterial; meshes index into this table.
		std::vector<CollectedMaterial> materials(scene->mNumMaterials);
		std::vector<std::string> convertedFiles;
		for (unsigned int m = 0; m < scene->mNumMaterials; m++) {
			collectMaterialTextures(scene->mMaterials[m], modelDir, materials[m].textures, convertedFiles);
			collectMaterialProperties(scene->mMaterials[m], materials[m].properties);
		}

		std::filesystem::path outputPath(path);
		outputPath.replace_extension(".cbkm");

		std::ofstream out(outputPath, std::ios::binary);
		if (!out) {
			CBK_AC_ERROR("Failed to create output file: {}", outputPath.string());
			return;
		}

		// Header
		ModelHeader header{
			.numMeshes = static_cast<uint32_t>(meshes.size()),
			.numMaterials = static_cast<uint32_t>(materials.size())
		};
		out.write(reinterpret_cast<const char*>(&header), sizeof(header));

		// Material table
		for (const auto& mat : materials) {
			MaterialHeader mh{
				.numTextures = static_cast<uint32_t>(mat.textures.size()),
				.numProperties = static_cast<uint32_t>(mat.properties.size())
			};
			out.write(reinterpret_cast<const char*>(&mh), sizeof(mh));

			for (const auto& tex : mat.textures) {
				TextureEntry entry{
					.type = tex.type,
					.pathLength = static_cast<uint32_t>(tex.relativePath.size())
				};
				out.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
				out.write(tex.relativePath.data(), entry.pathLength);
			}

			for (const auto& prop : mat.properties) {
				PropertyEntry entry{ .key = prop.key, .value = prop.value };
				out.write(reinterpret_cast<const char*>(&entry), sizeof(entry));
			}
		}

		// Mesh data
		for (const auto& mesh : meshes) {
			MeshHeader mh{
				.numVertices = static_cast<uint32_t>(mesh.vertices.size()),
				.numIndices = static_cast<uint32_t>(mesh.indices.size()),
				.materialIndex = mesh.materialIndex
			};
			out.write(reinterpret_cast<const char*>(&mh), sizeof(mh));
			out.write(reinterpret_cast<const char*>(mesh.vertices.data()),
			          mesh.vertices.size() * sizeof(Vertex));
			out.write(reinterpret_cast<const char*>(mesh.indices.data()),
			          mesh.indices.size() * sizeof(uint32_t));
		}

		CBK_AC_INFO("Converted model: {} -> {} ({} meshes, {} materials)",
		             path, outputPath.string(), meshes.size(), materials.size());
	}
} // namespace cbk::ac
