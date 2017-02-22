/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef FONT_RENDERER_H
#define FONT_RENDERER_H

#include "Common.h"

#include <string>

#include "Texture2D.h"
#include "Shader.h"

class FontRenderer
{
private:
	struct CharacterData
	{
		float positionX;
		float positionY;
		GLuint charIndex;
	};

	GLuint vbo, vao;
	Texture2D fontTexture;
	Shader fontShader;

	unsigned int charSize;
	int charLeft[256];
	int charTop[256];
	int advanceLeft[256];
	int advanceTop[256];

	glm::mat4 projectionMatrix;

public:
	FontRenderer(const std::string& fontpath, const unsigned int _fontSize, const unsigned int _screenWidth, const unsigned int _screenHeight);
	~FontRenderer();

	void RenderString(const std::string& text, const glm::vec2& position, const glm::vec4& color);
};

#endif