#pragma once

#include <cstdint>

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

	struct ModelHeader {
		uint32_t magic = 0x43424B4D; // "CBKM" (Cabrankengine Model)
		uint32_t version = 2;
		uint32_t numMeshes;
		uint32_t numTextures;
		uint32_t numProperties;
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
		None  = 0,
		Phong = 1,
		PBR   = 2,
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
		float px, py, pz;       // position
		float nx, ny, nz;       // normal
		float tx, ty;           // texCoords
		float tanx, tany, tanz; // tangent
	};

	struct MeshHeader {
		uint32_t numVertices;
		uint32_t numIndices;
		// followed by Vertex[numVertices] then uint32_t[numIndices]
	};

} // namespace cbk::common
