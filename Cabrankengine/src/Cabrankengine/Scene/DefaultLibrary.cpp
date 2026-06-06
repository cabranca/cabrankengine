#include <pch.h>
#include "DefaultLibrary.h"

namespace cbk::scene {

	using namespace rendering;

	void DefaultLibrary::init() {
		setupTexture(s_WhiteTexture, 0xffffffff);
        setupTexture(s_BlackTexture, 0xff000000);
        setupTexture(s_FlatNormalTexture, 0xffff8080);

		setupQuad();
        setupCube();
        setupSphere();

        setupBasicShaders();
	}

	void DefaultLibrary::shutdown() {
		s_WhiteTexture.reset();
		s_BlackTexture.reset();
		s_FlatNormalTexture.reset();
		s_ErrorShader.reset();
		s_CubeDesc.reset();
		s_QuadDesc.reset();
		s_SphereDesc.reset();
	}

	Ref<rendering::Texture2D> DefaultLibrary::getWhiteTexture() {
		return s_WhiteTexture;
	}

	Ref<rendering::Texture2D> DefaultLibrary::getBlackTexture() {
		return s_BlackTexture;
	}

	Ref<rendering::Texture2D> DefaultLibrary::getFlatNormalTexture() {
		return s_FlatNormalTexture;
	}

	Ref<rendering::Shader> DefaultLibrary::getErrorShader() {
		return s_ErrorShader;
	}

	Ref<rendering::GeometryDescriptor> DefaultLibrary::getCube() {
		return s_CubeDesc;
	}

	Ref<rendering::GeometryDescriptor> DefaultLibrary::getQuad() {
		return s_QuadDesc;
	}

	Ref<rendering::GeometryDescriptor> DefaultLibrary::getSphere() {
		return s_SphereDesc;
	}

	void DefaultLibrary::setupBasicShaders() {
		ShaderLibrary::load("assets/shaders/Error");
		s_ErrorShader = ShaderLibrary::get("Error");
		ShaderLibrary::load("assets/shaders/Phong");
		ShaderLibrary::load("assets/shaders/Texture");
		ShaderLibrary::load("assets/shaders/Text");
#ifndef CBK_OPENGL_ES
		ShaderLibrary::load("assets/shaders/PBR");
#endif
	}

	void DefaultLibrary::setupTexture(Ref<Texture2D>& tex, uint32_t data) {
        tex = Texture2D::create(s_Specs);
		tex->setData(&data, sizeof(uint32_t));
    }

	void DefaultLibrary::setupQuad() {
		std::array<float, (3 + 2) * 4> quadVertices = { -0.5f, -0.5f, 0.0f, 0.0f, 0.0f, 0.5f,  -0.5f, 0.0f, 1.0f, 0.0f,
			                                            0.5f,  0.5f,  0.0f, 1.0f, 1.0f, -0.5f, 0.5f,  0.0f, 0.0f, 1.0f };

		std::array<uint32_t, 6> quadIndices = { 0, 1, 2, 2, 3, 0 };

		s_QuadDesc = GeometryDescriptor::create(quadVertices.data(), quadVertices.size() * sizeof(float), quadIndices.data(),
		                                        quadIndices.size() * sizeof(uint32_t),
		                                        { { ShaderDataType::Float3, "a_Position" }, { ShaderDataType::Float2, "a_TexCoords" } });
	}

	void DefaultLibrary::setupCube() {
        std::array<float, (3+3+2)*24> cubeVertices = {
            // Front Face (+Z)
                  // Pos               // Normal            // UV
             -0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 0.0f ,
              0.5f, -0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 0.0f ,
              0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   1.0f, 1.0f ,
             -0.5f,  0.5f,  0.5f,   0.0f,  0.0f,  1.0f,   0.0f, 1.0f ,

            // Right Face (+X)
              0.5f, -0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 0.0f ,
              0.5f, -0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 0.0f ,
              0.5f,  0.5f, -0.5f,   1.0f,  0.0f,  0.0f,   1.0f, 1.0f ,
              0.5f,  0.5f,  0.5f,   1.0f,  0.0f,  0.0f,   0.0f, 1.0f ,

            // Back Face (-Z)
              0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 0.0f ,
             -0.5f, -0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 0.0f ,
             -0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   1.0f, 1.0f ,
              0.5f,  0.5f, -0.5f,   0.0f,  0.0f, -1.0f,   0.0f, 1.0f ,

            // Left Face (-X)
             -0.5f, -0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 0.0f ,
             -0.5f, -0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 0.0f ,
             -0.5f,  0.5f,  0.5f,  -1.0f,  0.0f,  0.0f,   1.0f, 1.0f ,
             -0.5f,  0.5f, -0.5f,  -1.0f,  0.0f,  0.0f,   0.0f, 1.0f ,

            // Top Face (+Y)
             -0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 0.0f ,
              0.5f,  0.5f,  0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 0.0f ,
              0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   1.0f, 1.0f ,
             -0.5f,  0.5f, -0.5f,   0.0f,  1.0f,  0.0f,   0.0f, 1.0f ,

            // Bottom Face (-Y)
             -0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 0.0f ,
              0.5f, -0.5f, -0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 0.0f ,
              0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   1.0f, 1.0f ,
             -0.5f, -0.5f,  0.5f,   0.0f, -1.0f,  0.0f,   0.0f, 1.0f 
        };

        // Indices (6 caras * 2 triangulos * 3 indices = 36 indices)
        std::array<uint32_t, 36> cubeIndices = {
            0,  1,  2,  2,  3,  0,  // Front
            4,  5,  6,  6,  7,  4,  // Right
            8,  9,  10, 10, 11, 8,  // Back
            12, 13, 14, 14, 15, 12, // Left
            16, 17, 18, 18, 19, 16, // Top
            20, 21, 22, 22, 23, 20  // Bottom
        };

		s_CubeDesc = GeometryDescriptor::create(
		    cubeVertices.data(), cubeVertices.size() * sizeof(float), cubeIndices.data(), cubeIndices.size() * sizeof(uint32_t),
		    { { ShaderDataType::Float3, "a_Position" }, { ShaderDataType::Float3, "a_Normal" }, { ShaderDataType::Float2, "a_TexCoord" } });
	}

	void DefaultLibrary::setupSphere() {
		std::vector<float> vertices;
		std::vector<uint32_t> indices;

		constexpr unsigned int X_SEGMENTS = 64;
		constexpr unsigned int Y_SEGMENTS = 64;

		for (unsigned int x = 0; x <= X_SEGMENTS; x++) {
			for (unsigned int y = 0; y <= Y_SEGMENTS; y++) {
				float xSegment = (float)x / (float)X_SEGMENTS;
				float ySegment = (float)y / (float)Y_SEGMENTS;

				float xPos = cosf(xSegment * 2.f * math::PI) * sinf(ySegment * math::PI);
				float yPos = cosf(ySegment * math::PI);
				float zPos = sinf(xSegment * 2.f * math::PI) * sinf(ySegment * math::PI);

				vertices.push_back(xPos);
				vertices.push_back(yPos);
				vertices.push_back(zPos);

				vertices.push_back(xPos);
				vertices.push_back(yPos);
				vertices.push_back(zPos);

				vertices.push_back(xSegment);
				vertices.push_back(ySegment);
			}
		}

		bool oddRow = false;
		for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
			if (!oddRow) // even rows: y == 0, y == 2; and so on
			{
				for (unsigned int x = 0; x <= X_SEGMENTS; ++x) {
					indices.push_back(y * (X_SEGMENTS + 1) + x);
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				}
			} else {
				for (int x = X_SEGMENTS; x >= 0; --x) {
					indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
					indices.push_back(y * (X_SEGMENTS + 1) + x);
				}
			}
			oddRow = !oddRow;
		}

		// NOTA: El algoritmo de Triangle Strip de arriba es eficiente, pero para tu engine
		// que probablemente usa GL_TRIANGLES puros en RenderCommand::DrawIndexed,
		// necesitamos índices estándar de triángulos.
		// Vamos a limpiar indices y generarlos como TRIANGLES estándar:

		indices.clear(); // Reiniciamos para hacerlo compatible con GL_TRIANGLES genérico
		for (unsigned int y = 0; y < Y_SEGMENTS; ++y) {
			for (unsigned int x = 0; x < X_SEGMENTS; ++x) {
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				indices.push_back(y * (X_SEGMENTS + 1) + x);
				indices.push_back(y * (X_SEGMENTS + 1) + x + 1);

				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x);
				indices.push_back(y * (X_SEGMENTS + 1) + x + 1);
				indices.push_back((y + 1) * (X_SEGMENTS + 1) + x + 1);
			}
		}

		s_CubeDesc = GeometryDescriptor::create(
		    vertices.data(), vertices.size() * sizeof(float), indices.data(), indices.size() * sizeof(uint32_t),
		    { { ShaderDataType::Float3, "a_Position" }, { ShaderDataType::Float3, "a_Normal" }, { ShaderDataType::Float2, "a_TexCoord" } });
	}
} // namespace cbk::scene
