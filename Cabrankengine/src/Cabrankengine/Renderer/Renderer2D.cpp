#include <pch.h>
#include "Renderer2D.h"

#include <Common/Math/MatrixFactory.h>

#include "GeometryDescriptor.h"
#include "Materials/Texture2DMaterial.h"
#include "RenderCommand.h"
#include "Shader.h"

namespace cbk::rendering {

	using namespace math;

	struct QuadVertex {
		Vector3 Position;
		Vector4 Color;
		Vector2 TexCoord;
		float TexIndex;
		float TilingFactor;
	};

	struct Renderer2DData {
		static const uint32_t MaxQuads = 20000;
		static const uint32_t MaxVertices = MaxQuads * 4;
		static const uint32_t MaxIndices = MaxQuads * 6;
		static const uint32_t MaxTextureSlots = 32;

		Ref<GeometryDescriptor> QuadDesc;
		Ref<Texture2D> WhiteTexture;
		Ref<Texture2DMaterial> QuadMaterial;

		uint32_t QuadIndexCount = 0;
		QuadVertex* QuadVertexBufferBase = nullptr;
		QuadVertex* QuadVertexBufferPtr = nullptr;

		std::array<Ref<Texture2D>, MaxTextureSlots> TextureSlots;
		uint32_t TextureSlotIndex = 1; // Slot 0 = White Texture

		Vector3 QuadVertexPositions[4];

		Renderer2D::Statistics Stats;
	};

	static Renderer2DData s_Data;	

	void Renderer2D::init() {
		CBK_PROFILE_FUNCTION();

		s_Data.QuadVertexBufferBase = new QuadVertex[s_Data.MaxVertices];

		uint32_t* quadIndices = new uint32_t[s_Data.MaxIndices];

		uint32_t offset = 0;
		for (uint32_t i = 0; i < s_Data.MaxIndices; i += 6) {
			quadIndices[i + 0] = offset + 0;
			quadIndices[i + 1] = offset + 1;
			quadIndices[i + 2] = offset + 2;

			quadIndices[i + 3] = offset + 2;
			quadIndices[i + 4] = offset + 3;
			quadIndices[i + 5] = offset + 0;
			offset += 4;
		}

		s_Data.QuadDesc = GeometryDescriptor::create(s_Data.MaxVertices * sizeof(QuadVertex), quadIndices, s_Data.MaxIndices * sizeof(uint32_t), { 
			{ ShaderDataType::Float3, "pos" }, 
			{ ShaderDataType::Float4, "col" }, 
			{ ShaderDataType::Float2, "tex" },
			{ ShaderDataType::Float, "texIndex"},
			{ ShaderDataType::Float, "tilingFactor"}
		});
		delete[] quadIndices;

		s_Data.WhiteTexture = Texture2D::create(TextureSpecification());
		uint32_t whiteTextureData = 0xffffffff;
		s_Data.WhiteTexture->setData(&whiteTextureData, sizeof(uint32_t));

		s_Data.TextureSlots[0] = s_Data.WhiteTexture;

		// Material owns the batch's 32 texture slots and view-projection; the
		// batcher repopulates the slots before each flush via setTextureSlot.
		s_Data.QuadMaterial = Texture2DMaterial::create();

		s_Data.QuadVertexPositions[0] = { -0.5f, -0.5f, 0.0f };
		s_Data.QuadVertexPositions[1] = {  0.5f, -0.5f, 0.0f };
		s_Data.QuadVertexPositions[2] = {  0.5f,  0.5f, 0.0f };
		s_Data.QuadVertexPositions[3] = { -0.5f,  0.5f, 0.0f };
	}

	void Renderer2D::shutdown() {
		CBK_PROFILE_FUNCTION();

		delete[] s_Data.QuadVertexBufferBase;
		s_Data.QuadVertexBufferBase = nullptr;

		// s_Data is a program-scope static; its Ref<>s would otherwise destruct
		// after the Vulkan device/allocator are gone. Release the GPU-backed
		// resources here, while shutdown() still runs with a live device.
		for (auto& slot : s_Data.TextureSlots)
			slot.reset();
		s_Data.QuadMaterial.reset();
		s_Data.WhiteTexture.reset();
		s_Data.QuadDesc.reset();
	}

	void Renderer2D::beginScene(const math::Mat4& viewProjection) {
		CBK_PROFILE_FUNCTION();

		s_Data.QuadMaterial->setViewProjection(viewProjection);

		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;

		s_Data.TextureSlotIndex = 1;
	}

	void Renderer2D::endScene() {
		CBK_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount == 0)
			return;

		uint32_t dataSize = static_cast<uint32_t>(reinterpret_cast<uint8_t*>(s_Data.QuadVertexBufferPtr) - reinterpret_cast<uint8_t*>(s_Data.QuadVertexBufferBase));
		s_Data.QuadDesc->setData(s_Data.QuadVertexBufferBase, dataSize);

		flush();
	}

	void Renderer2D::flush() {
		CBK_PROFILE_FUNCTION();

		for (uint32_t i = 0; i < s_Data.TextureSlotIndex; i++)
			s_Data.QuadMaterial->setTextureSlot(i, s_Data.TextureSlots[i]);

		RenderCommand::drawIndexed(s_Data.QuadMaterial, s_Data.QuadDesc, math::identityMat(), s_Data.QuadIndexCount);

		s_Data.Stats.DrawCalls++;
	}

	void Renderer2D::flushAndReset() {
		endScene();
		
		s_Data.QuadIndexCount = 0;
		s_Data.QuadVertexBufferPtr = s_Data.QuadVertexBufferBase;
		s_Data.TextureSlotIndex = 1;
	}

	void Renderer2D::drawQuad(const Vector2& position, const Vector2& size, const Vector4& color) {
		drawQuad(Vector3(position.x, position.y, 0.0f), size, color);
	}

	void Renderer2D::drawQuad(const Vector3& position, const Vector2& size, const Vector4& color) {
		CBK_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			flushAndReset();

		constexpr float texIndex = 0.f;
		constexpr float tilingFactor = 1.f;

		Mat4 transform = math::scaleXYZ({ size.x, size.y, 1.f }) * math::translation(position);

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[0] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(0.0f, 0.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[1] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(1.0f, 0.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[2] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(1.0f, 1.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[3] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(0.0f, 1.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::drawQuad(const Vector2& position, const Vector2& size, const Ref<Texture2D>& texture, float tilingFactor, const Vector4& tintColor) {
		drawQuad(Vector3(position.x, position.y, 0.0f), size, texture, tilingFactor, tintColor);
	}

	void Renderer2D::drawQuad(const Vector3& position, const Vector2& size, const Ref<Texture2D>& texture, float tilingFactor, const Vector4& tintColor) {
		CBK_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			flushAndReset();

		constexpr Vector4 color(1.f);

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++) {
			if (*s_Data.TextureSlots[i].get() == *texture.get()) {
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.f) {
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		Mat4 transform = math::scaleXYZ({ size.x, size.y, 1.f }) * math::translation(position);

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[0] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(0.0f, 0.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[1] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(1.0f, 0.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[2] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(1.0f, 1.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[3] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(0.0f, 1.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::drawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Vector4& color) {
		drawRotatedQuad(Vector3(position.x, position.y, 0.0f), size, rotation, color);
	}

	void Renderer2D::drawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const Vector4& color) {
		CBK_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			flushAndReset();

		constexpr float texIndex = 0.f;
		constexpr float tilingFactor = 1.f;

		Mat4 transform = math::scaleXYZ({size.x, size.y, 1.f}) * rotateZ(rotation) * math::translation(position);

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[0] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(0.0f, 0.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[1] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(1.0f, 0.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[2] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(1.0f, 1.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[3] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(0.0f, 1.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = texIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	void Renderer2D::drawRotatedQuad(const Vector2& position, const Vector2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const Vector4& tintColor) {
		drawRotatedQuad(Vector3(position.x, position.y, 0.0f), size, rotation, texture, tilingFactor, tintColor);
	}

	void Renderer2D::drawRotatedQuad(const Vector3& position, const Vector2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor, const Vector4& tintColor) {
		CBK_PROFILE_FUNCTION();

		if (s_Data.QuadIndexCount >= Renderer2DData::MaxIndices)
			flushAndReset();

		constexpr Vector4 color(1.f);

		float textureIndex = 0.0f;
		for (uint32_t i = 1; i < s_Data.TextureSlotIndex; i++) {
			if (*s_Data.TextureSlots[i].get() == *texture.get()) {
				textureIndex = (float)i;
				break;
			}
		}

		if (textureIndex == 0.f) {
			textureIndex = (float)s_Data.TextureSlotIndex;
			s_Data.TextureSlots[s_Data.TextureSlotIndex] = texture;
			s_Data.TextureSlotIndex++;
		}

		Mat4 transform = math::scaleXYZ({ size.x, size.y, 1.f }) * rotateZ(rotation) * math::translation(position);

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[0] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(0.0f, 0.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[1] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(1.0f, 0.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[2] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(1.0f, 1.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadVertexBufferPtr->Position = s_Data.QuadVertexPositions[3] * transform;
		s_Data.QuadVertexBufferPtr->Color = color;
		s_Data.QuadVertexBufferPtr->TexCoord = Vector2(0.0f, 1.0f);
		s_Data.QuadVertexBufferPtr->TexIndex = textureIndex;
		s_Data.QuadVertexBufferPtr->TilingFactor = tilingFactor;
		s_Data.QuadVertexBufferPtr++;

		s_Data.QuadIndexCount += 6;

		s_Data.Stats.QuadCount++;
	}

	Renderer2D::Statistics Renderer2D::getStats()
	{
		return s_Data.Stats;
	}

	void Renderer2D::resetStats()
	{
		memset(&s_Data.Stats, 0, sizeof(Statistics));
	}
}
