/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef SHADER_H_
#define SHADER_H_

#include "Common.h"
#include <string>
#include <map>
#include <vector>

class Shader
{
private:
	struct UniformType {
	public:
		GLint location;
		GLint size;
		GLenum type;
	};
	struct CompileResult {
	public:
		GLuint handle;
		bool success;
		std::string error;
	};
	enum ShaderType
	{
		UNKNOWN, DRAW, COMPUTE
	};

	GLuint handle;
	std::map<std::string, UniformType> uniforms;
	bool ready;

	CompileResult compile(GLenum type, const std::string& source) const;
	void readUniforms();

	ShaderType type;
	bool tessellationShader;

	Shader(){}

	void initialize(const std::string& path, const std::vector<std::string>& symbols, const std::vector<std::string>& replacements);

	//Compute Shader things
	static bool boundsQueried;

public:
	Shader(const std::string& path);
	Shader(const std::string& path, const std::vector<std::string>& symbols, const std::vector<std::string>& replacements);
	~Shader();
	
	GLuint getHandle() const;
	void bind() const;
	void unbind() const;

	bool hasTessellationShader() const;

	void setUniform(const std::string& name, const GLuint value) const;
	void setUniform(const std::string& name, const GLboolean value) const;
	void setUniform(const std::string& name, const GLint value) const;
	void setUniform(const std::string& name, const GLint vecX, const GLint vecY) const;
	void setUniform(const std::string& name, const GLfloat value) const;
	void setUniform(const std::string& name, const glm::vec2& value) const;
	void setUniform(const std::string& name, const glm::vec3& value) const;
	void setUniform(const std::string& name, const glm::vec4& value) const;
	void setUniform(const std::string& name, const glm::ivec2& value) const;
	void setUniform(const std::string& name, const glm::ivec3& value) const;
	void setUniform(const std::string& name, const glm::ivec4& value) const;
	void setUniform(const std::string& name, const glm::mat2& value) const;
	void setUniform(const std::string& name, const glm::mat3& value) const;
	void setUniform(const std::string& name, const glm::mat4& value) const;
	void setUniform(const std::string& name, const std::vector<GLuint>& value) const;
	void setUniform(const std::string& name, const std::vector<GLboolean>& value) const;
	void setUniform(const std::string& name, const std::vector<GLint>& value) const;
	void setUniform(const std::string& name, const std::vector<GLfloat>& value) const;
	void setUniform(const std::string& name, const std::vector<glm::vec2>& value) const;
	void setUniform(const std::string& name, const std::vector<glm::vec3>& value) const;
	void setUniform(const std::string& name, const std::vector<glm::vec4>& value) const;
	void setUniform(const std::string& name, const std::vector<glm::mat2>& value) const;
	void setUniform(const std::string& name, const std::vector<glm::mat3>& value) const;
	void setUniform(const std::string& name, const std::vector<glm::mat4>& value) const;

	//Compute Shader things
	void dispatch(const unsigned int wg_x, const unsigned int wg_y, const unsigned int wg_z) const;

	static GLint max_shader_storage_blocks, max_uniform_blocks, max_texture_image_units, max_uniform_components, max_atomic_counters, max_atomic_counter_buffers, max_combined_uniform_components, max_shared_memory_size;
	static GLint max_work_group_invocations, max_work_group_count_X, max_work_group_count_Y, max_work_group_count_Z, max_work_group_size_X, max_work_group_size_Y, max_work_group_size_Z;
	static void queryBounds();
	static void printBounds();
};

#endif