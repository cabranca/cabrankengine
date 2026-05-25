#include <pch.h>
#include "TextRenderer.h"

#include <ft2build.h>
#include FT_FREETYPE_H

#include "GeometryDescriptor.h"
#include "Materials/TextMaterial.h"
#include "RenderCommand.h"
#include "Shader.h"
#include "Texture.h"

namespace cbk::rendering {

	using namespace math;

	struct TextVertex {
		Vector3 position;
		Vector4 color;
		Vector2 texCoord;
		float texIndex;
	};

	struct TextRendererData {
		static const uint32_t maxQuads = 20000;
		static const uint32_t maxVertices = maxQuads * 4;
		static const uint32_t maxIndices = maxQuads * 6;
		static const uint32_t maxTextureSlots = 32;

		Ref<GeometryDescriptor> textVertexDesc;
		Ref<TextMaterial> textMaterial;

		uint32_t quadIndexCount = 0;
		TextVertex* textVertexBufferBase = nullptr;
		TextVertex* textVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, maxTextureSlots> textureSlots;
		uint32_t textureSlotIndex = 0;
	};

	static TextRendererData s_Data;

	void TextRenderer::init() {
		s_Data.textVertexBufferBase = new TextVertex[s_Data.maxVertices];
		uint32_t* quadIndices = new uint32_t[s_Data.maxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data.maxIndices; i += 6) {
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;

			offset += 4;
		}

		s_Data.textVertexDesc =
		    GeometryDescriptor::create(s_Data.maxVertices * sizeof(TextVertex), quadIndices, s_Data.maxIndices * sizeof(uint32_t),
		                               { { ShaderDataType::Float3, "pos" },
		                                 { ShaderDataType::Float4, "color" },
		                                 { ShaderDataType::Float2, "texCoord" },
		                                 { ShaderDataType::Float, "texIndex" } });
		delete[] quadIndices;

		loadFont("assets/fonts/ocraext.ttf", 20);

		// Material owns the batch's 32 texture slots and view-projection; the
		// batcher repopulates the slots before each flush via setTextureSlot.
		s_Data.textMaterial = TextMaterial::create();
	}

	void TextRenderer::shutdown() {
		delete[] s_Data.textVertexBufferBase;
		s_Data.textVertexBufferBase = nullptr;

		// s_Data and s_Characters are program-scope statics; their Ref<>s would
		// otherwise destruct after the Vulkan device/allocator are gone. Release the
		// GPU-backed resources (glyph textures included) while the device is live.
		for (auto& slot : s_Data.textureSlots)
			slot.reset();
		s_Data.textMaterial.reset();
		s_Data.textVertexDesc.reset();
		s_Characters.clear();
	}

	void TextRenderer::beginScene(const math::Mat4& viewProjection) {
		s_Data.textMaterial->setViewProjection(viewProjection);
		startBatch();
	}

	void TextRenderer::endScene() {
		flush();
	}

	void TextRenderer::flush() {
		if (s_Data.quadIndexCount == 0)
			return;

		uint32_t dataSize = (uint32_t)((uint8_t*)s_Data.textVertexBufferPtr - (uint8_t*)s_Data.textVertexBufferBase);
		s_Data.textVertexDesc->setData(s_Data.textVertexBufferBase, dataSize);

		for (uint32_t i = 0; i < s_Data.textureSlotIndex; i++)
			s_Data.textMaterial->setTextureSlot(i, s_Data.textureSlots[i]);

		// Vulkan: bind() is a no-op and drawIndexed records the descriptor set.
		// OpenGL: bind() binds the shader + texture units, drawIndexed issues the draw.
		s_Data.textMaterial->bind();
		RenderCommand::drawIndexed(s_Data.textMaterial, s_Data.textVertexDesc, math::identityMat(), s_Data.quadIndexCount);
	}

	void TextRenderer::drawText(const std::string& text, math::Vector3 position, float scale, math::Vector4 color) {
		float x = position.x;
		float y = position.y;

		for (char c: text) {
			if (s_Data.quadIndexCount >= s_Data.maxIndices)
				nextBatch();

			Character ch = s_Characters.at(c);

			float xpos = x + ch.bearing.x * scale;
			float ypos = y + (s_Characters['H'].bearing.y - ch.bearing.y) * scale;

			float w = ch.size.x * scale;
			float h = ch.size.y * scale;

			float textureIndex = -1.f;

			for (uint32_t i = 0; i < s_Data.textureSlotIndex; i++) {
				if (s_Data.textureSlots[i] == ch.texture) {
					textureIndex = (float)i;
					break;
				}
			}

			if (textureIndex == -1.f) {
				if (s_Data.textureSlotIndex >= s_Data.maxTextureSlots)
					nextBatch();

				textureIndex = (float)s_Data.textureSlotIndex;
				s_Data.textureSlots[s_Data.textureSlotIndex] = ch.texture;
				s_Data.textureSlotIndex++;
			}

			s_Data.textVertexBufferPtr->position = { xpos, ypos + h, position.z };
			s_Data.textVertexBufferPtr->color = color;
			s_Data.textVertexBufferPtr->texCoord = { 0.0f, 0.0f };
			s_Data.textVertexBufferPtr->texIndex = textureIndex;
			s_Data.textVertexBufferPtr++;

			s_Data.textVertexBufferPtr->position = { xpos + w, ypos + h, position.z };
			s_Data.textVertexBufferPtr->color = color;
			s_Data.textVertexBufferPtr->texCoord = { 1.0f, 0.0f };
			s_Data.textVertexBufferPtr->texIndex = textureIndex;
			s_Data.textVertexBufferPtr++;

			s_Data.textVertexBufferPtr->position = { xpos + w, ypos, position.z };
			s_Data.textVertexBufferPtr->color = color;
			s_Data.textVertexBufferPtr->texCoord = { 1.0f, 1.0f };
			s_Data.textVertexBufferPtr->texIndex = textureIndex;
			s_Data.textVertexBufferPtr++;

			s_Data.textVertexBufferPtr->position = { xpos, ypos, position.z };
			s_Data.textVertexBufferPtr->color = color;
			s_Data.textVertexBufferPtr->texCoord = { 0.0f, 1.0f };
			s_Data.textVertexBufferPtr->texIndex = textureIndex;
			s_Data.textVertexBufferPtr++;

			s_Data.quadIndexCount += 6;

			x += (ch.advance >> 6) * scale;
		}
	}

	void TextRenderer::loadFont(const std::string& font, unsigned int fontSize) {
		s_Characters.clear();

		FT_Library ft;
		if (FT_Init_FreeType(&ft))
			CBK_CORE_ERROR("ERROR::FREETYPE: Could not init FreeType Library");

		FT_Face face;
		if (FT_New_Face(ft, font.c_str(), 0, &face))
			CBK_CORE_ERROR("ERROR::FREETYPE: Failed to load font");

		FT_Set_Pixel_Sizes(face, 0, fontSize);

		for (unsigned char c = 0; c < 128; c++) {
			if (FT_Load_Char(face, c, FT_LOAD_RENDER)) {
				CBK_CORE_ERROR("ERROR::FREETYTPE: Failed to load Glyph");
				continue;
			}

			// A texture for each letter. Ideally we'd use a TextureAtlas
			auto charTexture = Texture2D::create(face);

			Character character = { charTexture, Vector2(face->glyph->bitmap.width, face->glyph->bitmap.rows),
				                    Vector2(face->glyph->bitmap_left, face->glyph->bitmap_top),
				                    static_cast<unsigned int>(face->glyph->advance.x) };
			s_Characters.emplace(c, character);
		}

		FT_Done_Face(face);
		FT_Done_FreeType(ft);
	}

	void TextRenderer::startBatch() {
		s_Data.quadIndexCount = 0;
		s_Data.textVertexBufferPtr = s_Data.textVertexBufferBase;
		s_Data.textureSlotIndex = 0;
	}

	void TextRenderer::nextBatch() {
		flush();
		startBatch();
	}
} // namespace cbk::rendering
