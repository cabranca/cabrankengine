#include <pch.h>
#include "OpenGLShader.h"

#include <glad/glad.h>

#include <Common/Math/MatrixFactory.h>

namespace cbk::platform::opengl {

	using namespace math;
	using namespace rendering;

	static GLenum ShaderTypeFromString(const std::string& type) {
		if (type == "vertex")
			return GL_VERTEX_SHADER;
		else if (type == "fragment" || type == "pixel")
			return GL_FRAGMENT_SHADER;
		CBK_CORE_ASSERT(false, "Unknown shader type!");
		return 0;
	}

	OpenGLShader::OpenGLShader(const std::string& filepath) : m_RendererId(0) {
		CBK_PROFILE_FUNCTION();

		std::filesystem::path path(filepath);
		m_Name = path.stem().string();

		std::string shaderSource = readFile(filepath + ".glsl");
		auto shaderSources = preProcess(shaderSource);
		compile(shaderSources);
	}

	OpenGLShader::OpenGLShader(const std::string& name, const std::string& vertexSrc, const std::string& fragmentSrc) 
		: m_RendererId(0), m_Name(name) {
		CBK_PROFILE_FUNCTION();

		std::unordered_map<GLenum, std::string> sources;
		sources[GL_VERTEX_SHADER] = vertexSrc;
		sources[GL_FRAGMENT_SHADER] = fragmentSrc;
		compile(sources);
	}

	OpenGLShader::~OpenGLShader() {
		CBK_PROFILE_FUNCTION();

		glDeleteProgram(m_RendererId);
	}

	void OpenGLShader::bind() const {
		CBK_PROFILE_FUNCTION();

		glUseProgram(m_RendererId);
	}

	void OpenGLShader::unbind() const {
		CBK_PROFILE_FUNCTION();

		glUseProgram(0);
	}

	void OpenGLShader::setInt(const std::string& name, int value) {
		CBK_PROFILE_FUNCTION();

		uploadUniformInt(name, value);
	}

	void OpenGLShader::setIntArray(const std::string& name, uint32_t count, int* values) {
		CBK_PROFILE_FUNCTION();

		uploadUniformIntArray(name, values, count);
	}

	void OpenGLShader::setFloat(const std::string& name, float value) {
		CBK_PROFILE_FUNCTION();

		uploadUniformFloat1(name, value);
	}

	void OpenGLShader::setFloat3(const std::string& name, const Vector3& vector) {
		CBK_PROFILE_FUNCTION();

		uploadUniformFloat3(name, vector);
	}

	void OpenGLShader::setFloat4(const std::string& name, const Vector4& vector) {
		CBK_PROFILE_FUNCTION();

		uploadUniformFloat4(name, vector);
	}

	void OpenGLShader::setMat4(const std::string& name, const Mat4& value) {
		CBK_PROFILE_FUNCTION();

		uploadUniformMat4(name, value);
	}

	void OpenGLShader::uploadUniformInt(const std::string& name, int value) {
		CBK_PROFILE_FUNCTION();

		GLint location = glGetUniformLocation(m_RendererId, name.c_str());
		if (location == -1) {
			CBK_CORE_ERROR("Uniform '{0}' not found in shader!", name);
			return;
		}
		glUniform1i(location, value);
	}

	void OpenGLShader::uploadUniformIntArray(const std::string& name, int* values, uint32_t count) {
		CBK_PROFILE_FUNCTION();

		GLint location = glGetUniformLocation(m_RendererId, name.c_str());
		if (location == -1) {
			CBK_CORE_ERROR("Uniform '{0}' not found in shader!", name);
			return;
		}
		glUniform1iv(location, count, values);
	}

	void OpenGLShader::uploadUniformFloat1(const std::string& name, float value) {
		CBK_PROFILE_FUNCTION();

		GLint location = glGetUniformLocation(m_RendererId, name.c_str());
		if (location == -1) {
			CBK_CORE_ERROR("Uniform '{0}' not found in shader!", name);
			return;
		}
		glUniform1f(location, value);
	}

	void OpenGLShader::uploadUniformFloat2(const std::string& name, const Vector2& values) {
		CBK_PROFILE_FUNCTION();

		GLint location = glGetUniformLocation(m_RendererId, name.c_str());
		if (location == -1) {
			CBK_CORE_ERROR("Uniform '{0}' not found in shader!", name);
			return;
		}
		glUniform2f(location, values.x, values.y);
	}

	void OpenGLShader::uploadUniformFloat3(const std::string& name, const Vector3& values) {
		CBK_PROFILE_FUNCTION();

		GLint location = glGetUniformLocation(m_RendererId, name.c_str());
		if (location == -1) {
			CBK_CORE_ERROR("Uniform '{0}' not found in shader!", name);
			return;
		}
		glUniform3f(location, values.x, values.y, values.z);
	}

	void OpenGLShader::uploadUniformFloat4(const std::string& name, const Vector4& values) {
		CBK_PROFILE_FUNCTION();

		GLint location = glGetUniformLocation(m_RendererId, name.c_str());
		if (location == -1) {
			CBK_CORE_ERROR("Uniform '{0}' not found in shader!", name);
			return;
		}
		glUniform4f(location, values.x, values.y, values.z, values.w);
	}

	void OpenGLShader::uploadUniformMat4(const std::string& name, const Mat4& matrix) {
		CBK_PROFILE_FUNCTION();

		GLint location = glGetUniformLocation(m_RendererId, name.c_str());
		if (location == -1) {
			CBK_CORE_ERROR("Uniform {0} not found in shader!", name);
			return;
		}
		glUniformMatrix4fv(location, 1, GL_FALSE, reinterpret_cast<const float*>(matrix.elements.data()));
	}

	std::string OpenGLShader::readFile(const std::string& filepath) {
		CBK_PROFILE_FUNCTION();

		std::string result;
		std::ifstream in(filepath, std::ios::in | std::ios::binary);
		if (in) {
			in.seekg(0, std::ios::end);
			result.resize(in.tellg());
			in.seekg(0, std::ios::beg);
			in.read(&result[0], result.size());
			in.close();
		}
		else {
			CBK_CORE_ERROR("Could not open shader file: {0}", filepath);
		}
		return result;
	}

	std::unordered_map<GLenum, std::string> OpenGLShader::preProcess(const std::string& source) {
		CBK_PROFILE_FUNCTION();

		std::unordered_map<GLenum, std::string> shaderSources;
		const char* typeToken = "#type";
		size_t typeTokenLength = strlen(typeToken);
		size_t pos = source.find(typeToken, 0);
		while (pos != std::string::npos) {
			size_t eol = source.find_first_of("\r\n", pos);
			CBK_CORE_ASSERT(eol != std::string::npos, "Syntax error in shader file!");
			size_t begin = pos + typeTokenLength + 1;
			std::string type = source.substr(begin, eol - begin);
			CBK_CORE_ASSERT(ShaderTypeFromString(type), "Invalid shader type specified!");
			size_t nextLinePos = source.find_first_not_of("\r\n", eol);
			pos = source.find(typeToken, nextLinePos);
			shaderSources[ShaderTypeFromString(type)] = source.substr(nextLinePos, pos - nextLinePos);
		}
		return shaderSources;
	}

	void OpenGLShader::compile(const std::unordered_map<GLenum, std::string>& shaderSources) {
		CBK_PROFILE_FUNCTION();

		GLuint program = glCreateProgram();
		CBK_CORE_ASSERT(shaderSources.size() <= 2, "OpenGL only supports 2 shader types (vertex and fragment) at the moment!");
		std::array<GLenum, 2> shaderIds;
		int shaderIndex = 0;
		for (const auto& [type, source] : shaderSources) {
			GLenum shaderId = glCreateShader(type);
			const char* src = source.c_str();
			glShaderSource(shaderId, 1, &src, nullptr);
			glCompileShader(shaderId);
			int isCompiled = 0;
			glGetShaderiv(shaderId, GL_COMPILE_STATUS, &isCompiled);
			if (isCompiled == GL_FALSE) {
				int maxLength = 0;
				glGetShaderiv(shaderId, GL_INFO_LOG_LENGTH, &maxLength);
				std::vector<char> infoLog(maxLength);
				glGetShaderInfoLog(shaderId, maxLength, &maxLength, infoLog.data());
				glDeleteShader(shaderId);
				CBK_CORE_ERROR("Shader compilation error: {0}", infoLog.data());
				return;
			}
			shaderIds[shaderIndex++] = shaderId;
		}

		for (uint32_t shaderId : shaderIds) {
			glAttachShader(program, shaderId);
		}
		glLinkProgram(program);
		for (uint32_t shaderId : shaderIds) {
			glDetachShader(program, shaderId);
			glDeleteShader(shaderId);
		}

		m_RendererId = program;
	}
}
