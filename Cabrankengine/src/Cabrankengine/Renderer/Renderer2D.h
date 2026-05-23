#pragma once

#include <Common/Math/Mat4.h>

#include "Texture.h"

namespace cbk::rendering {

	class Renderer2D {
		public:
			static void init();

			static void shutdown();

			static void beginScene(const math::Mat4& viewProjection);

			static void endScene();

			static void flush();

			static void drawQuad(const math::Vector2& position, const math::Vector2& size, const math::Vector4& color);
			static void drawQuad(const math::Vector3& position, const math::Vector2& size, const math::Vector4& color);
			static void drawQuad(const math::Vector2& position, const math::Vector2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const math::Vector4& tintColor = math::Vector4(1.0f));
			static void drawQuad(const math::Vector3& position, const math::Vector2& size, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const math::Vector4& tintColor = math::Vector4(1.0f));

			static void drawRotatedQuad(const math::Vector2& position, const math::Vector2& size, float rotation, const math::Vector4& color);
			static void drawRotatedQuad(const math::Vector3& position, const math::Vector2& size, float rotation, const math::Vector4& color);
			static void drawRotatedQuad(const math::Vector2& position, const math::Vector2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const math::Vector4& tintColor = math::Vector4(1.0f));
			static void drawRotatedQuad(const math::Vector3& position, const math::Vector2& size, float rotation, const Ref<Texture2D>& texture, float tilingFactor = 1.0f, const math::Vector4& tintColor = math::Vector4(1.0f));
	
			struct Statistics {
				uint32_t DrawCalls = 0;
				uint32_t QuadCount = 0;

				uint32_t getTotalVertexCount() { return QuadCount * 4; }
				uint32_t getTotalIndexCount() { return QuadCount * 6; }
			};

			static Statistics getStats();
			static void resetStats();

		private:
			static void flushAndReset();
	};
}
