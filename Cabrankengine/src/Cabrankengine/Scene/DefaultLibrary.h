#pragma once

#include <Cabrankengine/Renderer/Texture.h>
#include <Cabrankengine/Renderer/Shader.h>
#include <Cabrankengine/Renderer/GeometryDescriptor.h>

namespace cbk::scene {

	class DefaultLibrary {
	  public:
		static void init();
		static void shutdown();

		static Ref<rendering::Texture2D> getWhiteTexture();
		static Ref<rendering::Texture2D> getBlackTexture();
		static Ref<rendering::Texture2D> getFlatNormalTexture();
		static Ref<rendering::Shader> getErrorShader();
		static Ref<rendering::GeometryDescriptor> getCube();
		static Ref<rendering::GeometryDescriptor> getQuad();
		static Ref<rendering::GeometryDescriptor> getSphere();

	  private:
		inline static rendering::TextureSpecification s_Specs = { 1, 1, rendering::ImageFormat::RGBA8 };
		inline static Ref<rendering::Texture2D> s_WhiteTexture = nullptr;
		inline static Ref<rendering::Texture2D> s_BlackTexture = nullptr;
		inline static Ref<rendering::Texture2D> s_FlatNormalTexture = nullptr;

		inline static Ref<rendering::Shader> s_ErrorShader = nullptr;

		inline static Ref<rendering::GeometryDescriptor> s_CubeDesc = nullptr;
		inline static Ref<rendering::GeometryDescriptor> s_QuadDesc = nullptr;
		inline static Ref<rendering::GeometryDescriptor> s_SphereDesc = nullptr;

		static void setupBasicShaders();
		static void setupTexture(Ref<rendering::Texture2D>& tex, uint32_t data);
		static void setupQuad();
		static void setupCube();
		static void setupSphere();
	};
} // namespace cbk::scene