/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "FontRenderer.h"
#include "Common.h"
#include "OpenGLState.h"

#include <glm\gtc\matrix_transform.hpp>

#include "ft2build.h"
#include FT_FREETYPE_H

#include <iostream>

#define FIRSTCHARCODE 32
#define BUFFERLENGTH 10

FontRenderer::FontRenderer(const std::string& fontpath, const unsigned int _fontSize, const unsigned int _screenWidth, const unsigned int _screenHeight) : fontShader(SHADERPATH + "/Font/fontShader"),
	fontTexture(GL_R8, GL_RED, false, false, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR), charSize(_fontSize)
{
	FT_Library library;

	FT_Error error = FT_Init_FreeType(&library);
	if (error)
	{
		std::cout << "Init freetype failed!\n";
		return;
	}

	FT_Face face;
	error = FT_New_Face(library, fontpath.c_str(), 0, &face);
	if (error == FT_Err_Unknown_File_Format)
	{
		std::cout << "File format not supported!\n";
		return;
	}
	else if (error)
	{
		std::cout << "Error while loading Font file!\n";
		return;
	}

	error = FT_Set_Char_Size(face, 0, 16 * 64, 96, 96);
	if (error)
	{
		std::cout << "Error while setting char size!\n";
		return;
	}

	error = FT_Set_Pixel_Sizes(face, charSize, 0);

	unsigned char * buffer = new unsigned char[_fontSize * _fontSize];

	fontTexture.bind(0);
	fontTexture.BufferImage((unsigned int)(_fontSize)* 16, (unsigned int)(_fontSize)* 16, GL_UNSIGNED_BYTE, 0);

	for (short charcodeCounter = 0; charcodeCounter < 256; charcodeCounter++)
	{
		error = FT_Load_Char(face, charcodeCounter + FIRSTCHARCODE, FT_LOAD_RENDER);
		if (error)
		{
			std::cout << "Error while loading char " << charcodeCounter + FIRSTCHARCODE << "!\n";
			return;
		}

		charLeft[charcodeCounter] = face->glyph->bitmap_left;
		charTop[charcodeCounter] = face->glyph->bitmap_top;
		advanceLeft[charcodeCounter] = face->glyph->advance.x >> 6;
		advanceTop[charcodeCounter] = face->glyph->advance.y >> 6;

		int textureY = charcodeCounter / 16;
		int textureX = charcodeCounter % 16;

		for (unsigned int i = 0; i<_fontSize * _fontSize; i++)
		{
			buffer[i] = 0;
		}

		unsigned int bmpWidth = glm::min((unsigned int)face->glyph->bitmap.width, _fontSize);
		unsigned int bmpHeight = glm::min((unsigned int)face->glyph->bitmap.rows, _fontSize);

		for (unsigned int y = 0; y < bmpHeight; y++)
		{
			for (unsigned int x = 0; x < bmpWidth; x++)
			{
				buffer[x + _fontSize * (_fontSize - bmpHeight + y)] = face->glyph->bitmap.buffer[x + face->glyph->bitmap.width * (face->glyph->bitmap.rows - y - 1)];
			}
		}

		fontTexture.BufferSubImage(textureX * _fontSize, textureY * _fontSize, _fontSize, _fontSize, GL_UNSIGNED_BYTE, buffer);
	}

	delete[] buffer;

	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_R, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);

	projectionMatrix = glm::ortho(0.0f, (float)_screenWidth, (float)_screenHeight, 0.0f, 0.0f, 100.0f);

	glGenBuffers(1, &vbo);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, BUFFERLENGTH * sizeof(CharacterData), 0, GL_STREAM_DRAW);

	glGenVertexArrays(1, &vao);

	glBindVertexArray(vao);

	GLint posindex = glGetAttribLocation(fontShader.getHandle(), "position");
	GLint charindex = glGetAttribLocation(fontShader.getHandle(), "charIndex");
	glEnableVertexAttribArray(posindex);
	glVertexAttribPointer(posindex, 2, GL_FLOAT, GL_FALSE, sizeof(CharacterData), (GLvoid*)0);
	glEnableVertexAttribArray(charindex);
	glVertexAttribIPointer(charindex, 1, GL_UNSIGNED_INT, sizeof(CharacterData), (GLvoid*)(2 * sizeof(float)));

	glBindVertexArray(0);
}

FontRenderer::~FontRenderer()
{
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
}

void FontRenderer::RenderString(const std::string& text, const glm::vec2& position, const glm::vec4& color)
{
	auto& oglState = OpenGLState::Instance();
	oglState.disable(GL_DEPTH_TEST);
	oglState.enable(GL_BLEND);
	oglState.disable(GL_CULL_FACE);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	fontTexture.bind(0);

	fontShader.bind();

	fontShader.setUniform("color", color);
	fontShader.setUniform("projectionMatrix", projectionMatrix);
	fontShader.setUniform("charSize", (float)charSize);
	fontShader.setUniform("glyphTexture", (GLint)0);

	glBindVertexArray(vao);

	float l = (float)position.x;
	float t = (float)position.y;

	for (size_t rangeStart = 0; rangeStart < text.size(); rangeStart += BUFFERLENGTH)
	{
		size_t range = BUFFERLENGTH;

		if (range > text.size() - rangeStart)
			range = text.size() - rangeStart;

		glBindBuffer(GL_ARRAY_BUFFER, vbo);

		CharacterData* data = (CharacterData*)glMapBufferRange(GL_ARRAY_BUFFER, 0, range * sizeof(CharacterData), GL_MAP_WRITE_BIT | GL_MAP_INVALIDATE_RANGE_BIT);

		for (size_t i = 0; i < range; i++)
		{
			unsigned int charIndex = text.c_str()[rangeStart + i] - FIRSTCHARCODE;

			if (charIndex < 0 || charIndex > 256)
			{
				std::cout << "Error character " << (char)charIndex << "!";
				return;
			}

			float thisleft = l + charLeft[charIndex];
			float thistop = t - charTop[charIndex];

			data[i].positionX = thisleft;
			data[i].positionY = thistop;
			data[i].charIndex = charIndex;

			l += advanceLeft[charIndex];
		}

		if (!glUnmapBuffer(GL_ARRAY_BUFFER))
		{
			std::cout << "Failed to unmap array buffer!\n";
		}

		glDrawArrays(GL_POINTS, 0, (GLsizei)range);

		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	fontShader.unbind();

	glBindVertexArray(0);

	oglState.enable(GL_DEPTH_TEST);
	oglState.disable(GL_BLEND);
	oglState.enable(GL_CULL_FACE);
}