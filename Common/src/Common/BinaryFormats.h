#pragma once

#include <cstdint>
#include <type_traits>

#include <Common/Math/Vector2.h>
#include <Common/Math/Vector3.h>

namespace cbk::common {

	// --- Texture binary format (.cbkt) ---

	struct TextureHeader {
		uint32_t magic = 0x43424B54; // "CBKT" (Cabrankengine Texture)
		uint32_t version = 1;
		uint32_t width, height, channels;
		uint32_t compressedSize;
		uint32_t uncompressedSize;
	};

	// --- Model binary format (.cbkm) ---

	// CBKM v3 layout:
	//   ModelHeader
	//   numMaterials x { MaterialHeader, TextureEntry[]+paths, PropertyEntry[] }
	//   numMeshes    x { MeshHeader, Vertex[], uint32_t[] }
	struct ModelHeader {
		uint32_t magic = 0x43424B4D; // "CBKM" (Cabrankengine Model)
		uint32_t version = 3;
		uint32_t numMeshes;
		uint32_t numMaterials;
	};

	enum class TextureType : uint32_t {
		Diffuse = 1,        // Diffuse / Albedo (BaseColor)
		Specular = 2,
		Normal = 3,
		MetalRoughness = 4, // packed: B=metal, G=roughness
		AO = 5              // Ambient Occlusion
	};

	// Tags a model component so the loader knows which material factory to invoke.
	// Stored alongside the model path in CModel; serialized to JSON.
	enum class MaterialKind : uint32_t {
		PBR   = 0,
		Phong = 1,
	};

	struct TextureEntry {
		TextureType type;
		uint32_t pathLength;
		// followed by char[pathLength]
	};

	// Property keys:
	// 1 = Shininess, 2 = Metalness, 3 = Roughness
	// 4 = BaseColorR, 5 = BaseColorG, 6 = BaseColorB
	struct PropertyEntry {
		uint32_t key;
		float value;
	};

	struct Vertex {
		math::Vector3 position;
		math::Vector3 normal;
		math::Vector2 texCoords;
		math::Vector3 tangent;
	};

	static_assert(sizeof(Vertex) == 44, "Vertex must be tightly packed (3+3+2+3 floats = 44 bytes) for binary IO");
	static_assert(std::is_standard_layout_v<Vertex>, "Vertex must be standard-layout for binary IO");

	// One material's payload. Followed by numTextures TextureEntry records
	// (each trailed by char[pathLength]) then numProperties PropertyEntry records.
	struct MaterialHeader {
		uint32_t numTextures;
		uint32_t numProperties;
	};

	struct MeshHeader {
		uint32_t numVertices;
		uint32_t numIndices;
		uint32_t materialIndex; // index into the model's material table
		// followed by Vertex[numVertices] then uint32_t[numIndices]
	};

} // namespace cbk::common
