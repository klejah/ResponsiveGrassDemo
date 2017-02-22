/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef SKYBOX_H
#define SKYBOX_H

#include "Common.h"
#include "Shader.h"
#include "Camera.h"

class Skybox
{
public:
	Skybox(const std::string& filename);
	~Skybox();

	void render(const Camera& cam);

private:
	Shader* shader;
	GLuint cubetex;
	GLuint vbo, ind, vao;
};

#endif