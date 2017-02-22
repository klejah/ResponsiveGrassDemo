/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "Skybox.h"

#include "Texture2D.h"

Skybox::Skybox(const std::string& filename)
{
	glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

	//Load shader
	shader = new Shader(SHADERPATH + "/Misc/Skybox");

	//Load texture
	std::vector<Texture2D::PNGOutput> out;
	out.reserve(6);
	out.push_back(Texture2D::loadPNGFile(filename + "xpos.png", true));
	out.push_back(Texture2D::loadPNGFile(filename + "xneg.png", true));
	out.push_back(Texture2D::loadPNGFile(filename + "ypos.png", true));
	out.push_back(Texture2D::loadPNGFile(filename + "yneg.png", true));
	out.push_back(Texture2D::loadPNGFile(filename + "zpos.png", true));
	out.push_back(Texture2D::loadPNGFile(filename + "zneg.png", true));

	glGenTextures(1, &cubetex);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);

	for (GLuint i = 0; i < (GLuint)out.size(); i++)
	{
		if (out[i].info_ptr->channels == 3)
		{
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGB, out[i].info_ptr->width, out[i].info_ptr->height, 0, GL_RGB, GL_UNSIGNED_BYTE, out[i].image_data
				);
		}
		else
		{
			glTexImage2D(
				GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
				0, GL_RGBA, out[i].info_ptr->width, out[i].info_ptr->height, 0, GL_RGBA, GL_UNSIGNED_BYTE, out[i].image_data
				);
		}
	}

	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	glBindTexture(GL_TEXTURE_CUBE_MAP, 0);

	for (unsigned int i = 0; i < out.size(); i++)
	{
		delete out[i].image_data;
	}

	//Generate vbo vao
	std::vector<glm::vec2> quadverts;
	quadverts.reserve(4);
	quadverts.push_back(glm::vec2(-1.0f, -1.0f));
	quadverts.push_back(glm::vec2( 1.0f, -1.0f));
	quadverts.push_back(glm::vec2( 1.0f,  1.0f));
	quadverts.push_back(glm::vec2(-1.0f,  1.0f));
	std::vector<GLuint> indices;
	indices.reserve(6);
	indices.push_back(0);
	indices.push_back(1);
	indices.push_back(3);
	indices.push_back(1);
	indices.push_back(2);
	indices.push_back(3);

	glGenBuffers(1, &vbo);
	glGenBuffers(1, &ind);
	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, 4 * sizeof(glm::vec2), quadverts.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 2, GL_FLOAT, GL_FALSE, 0, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ind);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, 6 * sizeof(GLuint), indices.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);

}

Skybox::~Skybox()
{
	glDeleteTextures(1, &cubetex);
	glDeleteBuffers(1, &ind);
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

void Skybox::render(const Camera& cam)
{
	glDepthMask(GL_FALSE);

	shader->bind();

	shader->setUniform("invProjectionMatrix", cam.invProjectionMatrix);
	shader->setUniform("viewMatrix", cam.viewMatrix);

	glBindVertexArray(vao);

	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubetex);

	shader->setUniform("cubeTex", (GLint)0);

	glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, 0);
	
	glBindVertexArray(0);

	shader->unbind();

	glDepthMask(GL_TRUE);
}