#pragma once

#include <Common/Math/Mat4.h>

namespace cbk::rendering {

	class Texture2D;

	/// Holds all state information relevant to a character as loaded using FreeType.
	struct Character {
		Ref<Texture2D> texture; // ID handle of the glyph texture
		math::Vector2 size;     // Size of glyph
		math::Vector2 bearing;  // Offset from baseline to left/top of glyph
		unsigned int advance;   // Horizontal offset to advance to next glyph
	};

	// A renderer class for rendering text displayed by a font loaded using the
	// FreeType library. A single font is loaded, processed into a list of Character
	// items for later rendering.
	class TextRenderer {
	  public:
		static void init();
		static void shutdown();

		static void beginScene(const math::Mat4& viewProjection);
		static void endScene();
		static void flush();

		static void drawText(const std::string& text, math::Vector3 position, float scale, math::Vector4 color);

		static void loadFont(const std::string& font, unsigned int fontSize);

	  private:
		// Holds a list of pre-compiled Characters.
		inline static std::unordered_map<char, Character> s_Characters = {};

		static void startBatch();
		static void nextBatch();
	};
} // namespace cbk::rendering
