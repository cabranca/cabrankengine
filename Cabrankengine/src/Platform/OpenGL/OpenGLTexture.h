#pragma once

#include <glad/glad.h>

#include <Cabrankengine/Renderer/Texture.h>

namespace cbk::platform::opengl {

	class OpenGLTexture2D : public rendering::Texture2D {
	  public:
		OpenGLTexture2D(const rendering::TextureSpecification& specification);
		OpenGLTexture2D(const std::string& path, bool sRGB = false);
		OpenGLTexture2D(const FT_Face& face);
		virtual ~OpenGLTexture2D();

		// Returns the specification of the texture, which includes its width, height, format, and mip generation status.
		virtual const rendering::TextureSpecification& getSpecification() const override {
			return m_Specification;
		}

		// Returns the texture width
		virtual uint32_t getWidth() const override {
			return m_Width;
		}

		// Returns the texture height
		virtual uint32_t getHeight() const override {
			return m_Height;
		}

		// Returns the texture's renderer ID, which is used by the graphics API to identify the texture.
		virtual uint64_t getRendererID() const override {
			return m_RendererID;
		}

		// Returns the path to the texture file.
		virtual const std::string& getPath() const override {
			return m_Path;
		}

		// Sets the texture data from a raw data pointer and size.
		virtual void setData(void* data, uint32_t size) override;

		// Binds the texture to a specific slot in the graphics API, allowing it to be used in rendering.
		virtual void bind(uint32_t slot = 0) const override;

		// Returns whether the texture is loaded successfully.
		virtual bool isLoaded() const override {
			return m_IsLoaded;
		}

		// Returns whether both textures are equal based on their specifications and renderer IDs.
		virtual bool operator==(const Texture& other) const override {
			return m_RendererID == other.getRendererID();
		}

	  private:
		rendering::TextureSpecification
		    m_Specification; // Specification of the texture, including width, height, format, and mip generation status

		std::string m_Path;         // Path to the texture file, used for loading the texture from disk
		bool m_IsLoaded = false;    // Flag indicating whether the texture has been loaded successfully
		uint32_t m_Width, m_Height; // Width and height of the texture, used for rendering and mip generation
		uint32_t m_RendererID;      // Renderer ID of the texture, used by the graphics API to identify the texture
		GLenum m_InternalFormat,
		    m_DataFormat; // Internal format and data format of the texture, used by the graphics API for texture creation and binding
	};

} // namespace cbk::platform::opengl
