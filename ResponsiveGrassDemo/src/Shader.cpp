/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "Shader.h"
#include <iostream>
#include <fstream>

#include "glm\gtc\type_ptr.hpp"

#include "OpenGLState.h"

bool Shader::boundsQueried = false;
GLint Shader::max_atomic_counters = 0;
GLint Shader::max_atomic_counter_buffers = 0;
GLint Shader::max_combined_uniform_components = 0;
GLint Shader::max_shader_storage_blocks = 0;
GLint Shader::max_shared_memory_size = 0;
GLint Shader::max_texture_image_units = 0;
GLint Shader::max_uniform_blocks = 0;
GLint Shader::max_uniform_components = 0;
GLint Shader::max_work_group_count_X = 0;
GLint Shader::max_work_group_count_Y = 0;
GLint Shader::max_work_group_count_Z = 0;
GLint Shader::max_work_group_invocations = 0;
GLint Shader::max_work_group_size_X = 0;
GLint Shader::max_work_group_size_Y = 0;
GLint Shader::max_work_group_size_Z = 0;

void replaceAll(std::string& string, const std::string& symbol, const std::string& replacement) {
	if (symbol.empty())
		return;
	size_t start_pos = 0;
	while ((start_pos = string.find(symbol, start_pos)) != std::string::npos) {
		string.replace(start_pos, symbol.length(), replacement);
		start_pos += replacement.length();
	}
}

Shader::Shader(const std::string& path) : ready(false), tessellationShader(false)
{
	std::vector<std::string> symbols;
	std::vector<std::string> replacements;

	initialize(path, symbols, replacements);
}

Shader::Shader(const std::string& path, const std::vector<std::string>& symbols, const std::vector<std::string>& replacements) : ready(false), tessellationShader(false)
{
	if (symbols.size() != replacements.size())
	{
		std::cout << "ERROR: symbols and replacements have different sizes in shader " << path << std::endl;
	}
	else
	{
		initialize(path, symbols, replacements);
	}
}

void Shader::initialize(const std::string& path, const std::vector<std::string>& symbols, const std::vector<std::string>& replacements)
{
	bool vsExists = false, tcsExists = false, tesExists = false, gsExists = false, fsExists = false, csExists = false;
	std::string vertex_shader_source, tess_control_shader_source, tess_eval_shader_source, geometry_shader_source, fragment_shader_source, compute_shader_source;

	//check for shader existance and source
	{
		std::ifstream vsfile((path + ".vs").c_str());
		if (vsfile)
		{
			vertex_shader_source = std::string(std::istreambuf_iterator<char>(vsfile), std::istreambuf_iterator<char>());
			for (unsigned int i = 0; i < symbols.size(); i++)
			{
				replaceAll(vertex_shader_source, symbols[i], replacements[i]);
			}
			vsExists = true;
		}

		std::ifstream tcsfile((path + ".tcs").c_str());
		if (tcsfile)
		{
			tess_control_shader_source = std::string(std::istreambuf_iterator<char>(tcsfile), std::istreambuf_iterator<char>());
			for (unsigned int i = 0; i < symbols.size(); i++)
			{
				replaceAll(tess_control_shader_source, symbols[i], replacements[i]);
			}
			tcsExists = true;
		}

		std::ifstream tesfile((path + ".tes").c_str());
		if (tesfile)
		{
			tess_eval_shader_source = std::string(std::istreambuf_iterator<char>(tesfile), std::istreambuf_iterator<char>());
			for (unsigned int i = 0; i < symbols.size(); i++)
			{
				replaceAll(tess_eval_shader_source, symbols[i], replacements[i]);
			}
			tesExists = true;
		}

		std::ifstream gsfile((path + ".gs").c_str());
		if (gsfile)
		{
			geometry_shader_source = std::string(std::istreambuf_iterator<char>(gsfile), std::istreambuf_iterator<char>());
			for (unsigned int i = 0; i < symbols.size(); i++)
			{
				replaceAll(geometry_shader_source, symbols[i], replacements[i]);
			}
			gsExists = true;
		}

		std::ifstream fsfile((path + ".fs").c_str());
		if (fsfile)
		{
			fragment_shader_source = std::string(std::istreambuf_iterator<char>(fsfile), std::istreambuf_iterator<char>());
			for (unsigned int i = 0; i < symbols.size(); i++)
			{
				replaceAll(fragment_shader_source, symbols[i], replacements[i]);
			}
			fsExists = true;
		}

		std::ifstream csfile((path + ".cs").c_str());
		if (csfile)
		{
			compute_shader_source = std::string(std::istreambuf_iterator<char>(csfile), std::istreambuf_iterator<char>());
			for (unsigned int i = 0; i < symbols.size(); i++)
			{
				replaceAll(compute_shader_source, symbols[i], replacements[i]);
			}
			csExists = true;
		}
	}

	CompileResult vsCR, tcsCR, tesCR, gsCR, fsCR, csCR;
	if (vsExists)
	{
		vsCR = compile(GL_VERTEX_SHADER, vertex_shader_source);
		if (!vsCR.success)
		{
			std::cout << "Error comiling vertex shader with path \"" << path << ".vs\": " << vsCR.error << std::endl;
		}
	}

	if (tcsExists)
	{
		tcsCR = compile(GL_TESS_CONTROL_SHADER, tess_control_shader_source);
		if (!tcsCR.success)
		{
			std::cout << "Error comiling tessellation control shader with path \"" << path << ".tcs\": " << tcsCR.error << std::endl;
		}
	}

	if (tesExists)
	{
		tesCR = compile(GL_TESS_EVALUATION_SHADER, tess_eval_shader_source);
		if (!tesCR.success)
		{
			std::cout << "Error comiling tessellation evaluation shader with path \"" << path << ".tes\": " << tesCR.error << std::endl;
		}
	}

	if (gsExists)
	{
		gsCR = compile(GL_GEOMETRY_SHADER, geometry_shader_source);
		if (!gsCR.success)
		{
			std::cout << "Error comiling geometry shader with path \"" << path << ".gs\": " << gsCR.error << std::endl;
		}
	}

	if (fsExists)
	{
		fsCR = compile(GL_FRAGMENT_SHADER, fragment_shader_source);
		if (!fsCR.success)
		{
			std::cout << "Error comiling fragment shader with path \"" << path << ".fs\": " << fsCR.error << std::endl;
		}
	}

	if (csExists)
	{
		csCR = compile(GL_COMPUTE_SHADER, compute_shader_source);
		if (!csCR.success)
		{
			std::cout << "Error comiling compute shader with path \"" << path << ".cs\": " << csCR.error << std::endl;
		}
	}

	handle = glCreateProgram();

	if (handle == 0)
	{
		std::cout << "Error: Unable to assign handle for shader program" << std::endl;
		ready = false;
		return;
	}

	if (vsExists && vsCR.success)
	{
		glAttachShader(handle, vsCR.handle);
	}

	if (tcsExists && tcsCR.success)
	{
		tessellationShader = true;
		glAttachShader(handle, tcsCR.handle);
	}

	if (tesExists && tesCR.success)
	{
		tessellationShader = true;
		glAttachShader(handle, tesCR.handle);
	}

	if (gsExists && gsCR.success)
	{
		glAttachShader(handle, gsCR.handle);
	}

	if (fsExists && fsCR.success)
	{
		glAttachShader(handle, fsCR.handle);
	}

	if (csExists && csCR.success)
	{
		glAttachShader(handle, csCR.handle);
	}

	glLinkProgram(handle);

	if (vsExists && vsCR.success)
	{
		glDeleteShader(vsCR.handle);
	}

	if (tcsExists && tcsCR.success)
	{
		glDeleteShader(tcsCR.handle);
	}

	if (tesExists && tesCR.success)
	{
		glDeleteShader(tesCR.handle);
	}

	if (gsExists && gsCR.success)
	{
		glDeleteShader(gsCR.handle);
	}

	if (fsExists && fsCR.success)
	{
		glDeleteShader(fsCR.handle);
	}

	if (csExists && csCR.success)
	{
		glDeleteShader(csCR.handle);
	}

	int status;

	glGetProgramiv(handle, GL_LINK_STATUS, &status);

	if (status != GL_TRUE)
	{
		char logBuffer[8096];
		GLsizei length;
		logBuffer[0] = '\0';
		glGetProgramInfoLog(handle, 8096, &length, logBuffer);

		std::cout << "Shader linking failed: " << logBuffer << std::endl;

		glDeleteProgram(handle);
		handle = 0;
		ready = false;
	}
	else
	{
		ready = true;

		type = UNKNOWN;
		if (!csExists)
		{
			type = DRAW;
		}
		else if (!vsExists && !tcsExists && !tesExists && !gsExists && !fsExists)
		{
			type = COMPUTE;
		}

		if (csExists && !boundsQueried)
		{
			queryBounds();
		}

		readUniforms();
	}
}

Shader::~Shader()
{
	glDeleteProgram(handle);
	handle = 0;
	ready = false;
}

void Shader::readUniforms()
{
	//Uniforms
	GLint uniformCount, maxUniformLength;

	glGetProgramiv(handle, GL_ACTIVE_UNIFORMS, &uniformCount);
	glGetProgramiv(handle, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxUniformLength);

	if (maxUniformLength > 100)
	{
		glDeleteProgram(handle);
		handle = 0;
		ready = false;
		std::cout << "Error: Unable to load uniforms, length too long" << std::endl;
		return;
	}

	GLchar buffer[100];
	GLsizei nameLength;
	GLint size;
	GLenum type;

	for (GLint i = 0; i < uniformCount; i++)
	{
		glGetActiveUniform(handle, i, 100, &nameLength, &size, &type, buffer);

		std::string name(buffer);

		UniformType u;
		u.location = glGetUniformLocation(handle, name.c_str());
		u.size = size;
		u.type = type;
		uniforms[name] = u;
	}
}

Shader::CompileResult Shader::compile(GLenum type, const std::string& source) const
{
	CompileResult cr;

	GLuint shader = glCreateShader(type);

	if (shader == 0)
	{
		cr.handle = 0;
		cr.success = false;
		cr.error = "Unable to assign shader handle";
		return cr;
	}

	const char* src = source.data();
	int length = source.size();

	glShaderSource(shader, 1, &src, &length);
	glCompileShader(shader);

	int status;

	glGetShaderiv(shader, GL_COMPILE_STATUS, &status);

	if (status != GL_TRUE) {
		char logBuffer[8096];
		GLsizei l;
		logBuffer[0] = '\0';
		glGetShaderInfoLog(shader, 8096, &l, logBuffer);

		glDeleteShader(shader);

		cr.handle = 0;
		cr.success = false;
		cr.error = std::string(logBuffer);
		return cr;
	}

	cr.handle = shader;
	cr.success = true;
	cr.error = "";
	return cr;
}

GLuint Shader::getHandle() const
{
	return handle;
}

void Shader::bind() const
{
	if (ready)
	{
		OpenGLState::Instance().bindShader(handle);
	}
	else
	{
		std::cout << "ERROR: Unable to bind shader. Shader not ready!" << std::endl;
	}
}

void Shader::unbind() const
{
	OpenGLState::Instance().unbindShader();
}

bool Shader::hasTessellationShader() const
{
	return tessellationShader;
}

void Shader::setUniform(const std::string& name, const GLuint value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);

		if (it != uniforms.end() && it->second.type == GL_UNSIGNED_INT)
		{
			glUniform1ui(it->second.location, value);
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const GLboolean value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);

		if (it != uniforms.end() && it->second.type == GL_BOOL)
		{
			glUniform1i(it->second.location, value ? 1 : 0);
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const GLint value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);

		if (it != uniforms.end() && (it->second.type == GL_INT || it->second.type == GL_SAMPLER_1D || it->second.type == GL_SAMPLER_2D || it->second.type == GL_SAMPLER_3D || it->second.type == GL_SAMPLER_2D_MULTISAMPLE || it->second.type == GL_SAMPLER_CUBE))
		{
			glUniform1i(it->second.location, value);
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const GLint vecX, const GLint vecY) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);

		if (it != uniforms.end() && (it->second.type == GL_INT_VEC2))
		{
			glUniform2i(it->second.location, vecX, vecY);
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const GLfloat value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);

		if (it != uniforms.end() && it->second.type == GL_FLOAT)
		{
			glUniform1f(it->second.location, value);
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const glm::vec2& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);

		if (it != uniforms.end() && it->second.type == GL_FLOAT_VEC2)
		{
			glUniform2fv(it->second.location, 1, glm::value_ptr(value));
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const glm::vec3& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);

		if (it != uniforms.end() && it->second.type == GL_FLOAT_VEC3)
		{
			glUniform3fv(it->second.location, 1, glm::value_ptr(value));
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const glm::vec4& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);

		if (it != uniforms.end() && it->second.type == GL_FLOAT_VEC4)
		{
			glUniform4fv(it->second.location, 1, glm::value_ptr(value));
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const glm::ivec2& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);

		if (it != uniforms.end() && it->second.type == GL_INT_VEC2)
		{
			glUniform2iv(it->second.location, 1, glm::value_ptr(value));
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const glm::ivec3& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);

		if (it != uniforms.end() && it->second.type == GL_INT_VEC3)
		{
			glUniform3iv(it->second.location, 1, glm::value_ptr(value));
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const glm::ivec4& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);

		if (it != uniforms.end() && it->second.type == GL_INT_VEC4)
		{
			glUniform4iv(it->second.location, 1, glm::value_ptr(value));
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const glm::mat2& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);

		if (it != uniforms.end() && it->second.type == GL_FLOAT_MAT2)
		{
			glUniformMatrix2fv(it->second.location, 1, GL_FALSE, glm::value_ptr(value));
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const glm::mat3& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);

		if (it != uniforms.end() && it->second.type == GL_FLOAT_MAT3)
		{
			glUniformMatrix3fv(it->second.location, 1, GL_FALSE, glm::value_ptr(value));
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const glm::mat4& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);

		if (it != uniforms.end() && it->second.type == GL_FLOAT_MAT4)
		{
			glUniformMatrix4fv(it->second.location, 1, GL_FALSE, glm::value_ptr(value));
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const std::vector<GLuint>& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);
		if (it != uniforms.end() && it->second.type == GL_UNSIGNED_INT)
		{
			if ((int)value.size() <= it->second.size)
			{
				glUniform1uiv(it->second.location, value.size(), value.data());
			}
			else if (DEBUG)
			{
				std::cout << "Uniform " << name << " too big." << std::endl;
			}
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const std::vector<GLboolean>& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);
		if (it != uniforms.end() && it->second.type == GL_BOOL)
		{
			if ((int)value.size() <= it->second.size)
			{
				std::vector<GLuint> values;

				for each (GLboolean b in value)
				{
					values.push_back(b ? 1 : 0);
				}

				glUniform1uiv(it->second.location, value.size(), values.data());
			}
			else if (DEBUG)
			{
				std::cout << "Uniform " << name << " too big." << std::endl;
			}
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const std::vector<GLint>& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);
		if (it != uniforms.end() && it->second.type == GL_INT)
		{
			if ((int)value.size() <= it->second.size)
			{
				glUniform1iv(it->second.location, value.size(), value.data());
			}
			else if (DEBUG)
			{
				std::cout << "Uniform " << name << " too big." << std::endl;
			}
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const std::vector<GLfloat>& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);
		if (it != uniforms.end() && it->second.type == GL_FLOAT)
		{
			if ((int)value.size() <= it->second.size)
			{
				glUniform1fv(it->second.location, value.size(), value.data());
			}
			else if (DEBUG)
			{
				std::cout << "Uniform " << name << " too big." << std::endl;
			}
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const std::vector<glm::vec2>& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);
		if (it != uniforms.end() && it->second.type == GL_FLOAT_VEC2)
		{
			if ((int)value.size() <= it->second.size)
			{
				glUniform2fv(it->second.location, value.size(), (const GLfloat*)value.data());
			}
			else if (DEBUG)
			{
				std::cout << "Uniform " << name << " too big." << std::endl;
			}
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const std::vector<glm::vec3>& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);
		if (it != uniforms.end() && it->second.type == GL_FLOAT_VEC3)
		{
			if ((int)value.size() <= it->second.size)
			{
				glUniform3fv(it->second.location, value.size(), (const GLfloat*)value.data());
			}
			else if (DEBUG)
			{
				std::cout << "Uniform " << name << " too big." << std::endl;
			}
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const std::vector<glm::vec4>& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);
		if (it != uniforms.end() && it->second.type == GL_FLOAT_VEC4)
		{
			if ((int)value.size() <= it->second.size)
			{
				glUniform4fv(it->second.location, value.size(), (const GLfloat*)value.data());
			}
			else if (DEBUG)
			{
				std::cout << "Uniform " << name << " too big." << std::endl;
			}
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const std::vector<glm::mat2>& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);
		if (it != uniforms.end() && it->second.type == GL_FLOAT_MAT2)
		{
			if ((int)value.size() <= it->second.size)
			{
				glUniformMatrix2fv(it->second.location, value.size(), GL_FALSE, (const GLfloat*)value.data());
			}
			else if (DEBUG)
			{
				std::cout << "Uniform " << name << " too big." << std::endl;
			}
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const std::vector<glm::mat3>& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);
		if (it != uniforms.end() && it->second.type == GL_FLOAT_MAT3)
		{
			if ((int)value.size() <= it->second.size)
			{
				glUniformMatrix3fv(it->second.location, value.size(), GL_FALSE, (const GLfloat *)value.data());
			}
			else if (DEBUG)
			{
				std::cout << "Uniform " << name << " too big." << std::endl;
			}
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::setUniform(const std::string& name, const std::vector<glm::mat4>& value) const
{
	if (ready)
	{
		std::map<std::string, UniformType>::const_iterator it = uniforms.find(name);
		if (it != uniforms.end() && it->second.type == GL_FLOAT_MAT4)
		{
			if ((int)value.size() <= it->second.size)
			{
				glUniformMatrix4fv(it->second.location, value.size(), GL_FALSE, (const GLfloat*)value.data());
			}
			else if (DEBUG)
			{
				std::cout << "Uniform " << name << " too big." << std::endl;
			}
		}
		else if (DEBUG)
		{
			std::cout << "Uniform " << name << " not set." << std::endl;
		}
	}
	else
	{
		std::cout << "ERROR: Unable to set uniform. Shader not ready!" << std::endl;
	}
}

void Shader::queryBounds()
{
	if (!boundsQueried)
	{
		glGetIntegerv(GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS, &max_shader_storage_blocks);
		glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_BLOCKS, &max_uniform_blocks);
		glGetIntegerv(GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS, &max_texture_image_units);
		glGetIntegerv(GL_MAX_COMPUTE_UNIFORM_COMPONENTS, &max_uniform_components);
		glGetIntegerv(GL_MAX_COMPUTE_ATOMIC_COUNTERS, &max_atomic_counters);
		glGetIntegerv(GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS, &max_atomic_counter_buffers);
		glGetIntegerv(GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS, &max_combined_uniform_components);
		glGetIntegerv(GL_MAX_COMPUTE_SHARED_MEMORY_SIZE, &max_shared_memory_size);

		glGetIntegerv(GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS, &max_work_group_invocations);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 0, &max_work_group_count_X);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 1, &max_work_group_count_Y);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_COUNT, 2, &max_work_group_count_Z);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 0, &max_work_group_size_X);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 1, &max_work_group_size_Y);
		glGetIntegeri_v(GL_MAX_COMPUTE_WORK_GROUP_SIZE, 2, &max_work_group_size_Z);

		boundsQueried = true;
	}
}

void Shader::printBounds()
{
	if (boundsQueried)
	{
		std::cout << "----------------------------------------------------\nCompute Shader Bounds:\n----------------------------------------------------" << std::endl;
		std::cout << "GL_MAX_COMPUTE_SHADER_STORAGE_BLOCKS: " << std::to_string(max_shader_storage_blocks) << std::endl;
		std::cout << "GL_MAX_COMPUTE_UNIFORM_BLOCKS: " << std::to_string(max_uniform_blocks) << std::endl;
		std::cout << "GL_MAX_COMPUTE_TEXTURE_IMAGE_UNITS: " << std::to_string(max_texture_image_units) << std::endl;
		std::cout << "GL_MAX_COMPUTE_UNIFORM_COMPONENTS: " << std::to_string(max_uniform_components) << std::endl;
		std::cout << "GL_MAX_COMPUTE_ATOMIC_COUNTERS: " << std::to_string(max_atomic_counters) << std::endl;
		std::cout << "GL_MAX_COMPUTE_ATOMIC_COUNTER_BUFFERS: " << std::to_string(max_atomic_counter_buffers) << std::endl;
		std::cout << "GL_MAX_COMBINED_COMPUTE_UNIFORM_COMPONENTS: " << std::to_string(max_combined_uniform_components) << std::endl;
		std::cout << "GL_MAX_COMPUTE_SHARED_MEMORY_SIZE: " << std::to_string(max_shared_memory_size) << std::endl;
		std::cout << std::endl;
		std::cout << "GL_MAX_COMPUTE_WORK_GROUP_INVOCATIONS: " << std::to_string(max_work_group_invocations) << std::endl;
		std::cout << "GL_MAX_COMPUTE_WORK_GROUP_COUNT_X: " << std::to_string(max_work_group_count_X) << std::endl;
		std::cout << "GL_MAX_COMPUTE_WORK_GROUP_COUNT_Y: " << std::to_string(max_work_group_count_Y) << std::endl;
		std::cout << "GL_MAX_COMPUTE_WORK_GROUP_COUNT_Z: " << std::to_string(max_work_group_count_Z) << std::endl;
		std::cout << "GL_MAX_COMPUTE_WORK_GROUP_SIZE_X: " << std::to_string(max_work_group_size_X) << std::endl;
		std::cout << "GL_MAX_COMPUTE_WORK_GROUP_SIZE_Y: " << std::to_string(max_work_group_size_Y) << std::endl;
		std::cout << "GL_MAX_COMPUTE_WORK_GROUP_SIZE_Z: " << std::to_string(max_work_group_size_Z) << std::endl;
		std::cout << "----------------------------------------------------" << std::endl;
	}
}

void Shader::dispatch(const unsigned int wg_x, const unsigned int wg_y, const unsigned int wg_z) const
{
	if (ready)
	{
		if (type != COMPUTE)
		{
			std::cout << "ERROR: Unable to dispatch compute shader. Shader is not recognized as compute shader!" << std::endl;
		}
		else
		{
			if (wg_x > (unsigned int)max_work_group_count_X || wg_y > (unsigned int)max_work_group_count_Y || wg_z > (unsigned int)max_work_group_count_Z)
			{
				std::cout << "ERROR: Can not dispatch compute shader, too many work groups assigned." << std::endl;
			}
			else
			{
				glDispatchCompute(wg_x, wg_y, wg_z);
			}
		}
	}
	else
	{
		std::cout << "ERROR: Unable to dispatch compute shader. Shader not ready!" << std::endl;
	}
}