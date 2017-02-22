/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "HeightMap.h"
#include <fstream>
#include <iostream>

#include "Shader.h"
#include "OpenGLState.h"
#include "ImageProcess.h"

HeightMap::HeightMap(const std::string& fileName, const std::string& fileName_normal, const float heightScale) : HeightMap()
{
	this->heightScale = heightScale;

	std::ifstream ifile(fileName.c_str());
	if (ifile.good())
	{
		PNGOutput png = loadPNGFile(fileName, false);

		if (png.info_ptr == 0)
		{
			std::cout << "ERROR HEIGHTMAP: PNG info is null." << std::endl;
		}
		else
		{
			unsigned int width = png.info_ptr->width;
			unsigned int height = png.info_ptr->height;

			glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

			if (png.info_ptr->channels == 3)
			{
				unsigned char* data = new unsigned char[width * height * 2];

				for (unsigned int i = 0; i < width*height*2; i+=2)
				{
					data[i] = png.image_data[i * 3];
					data[i + 1] = (unsigned char)heightScale;
				}

				BufferImage(width, height, GL_UNSIGNED_BYTE, data);

				delete data;

				std::ifstream ifileN(fileName_normal.c_str());
				if (ifileN.good())
				{
					PNGOutput png = loadPNGFile(fileName_normal, false);

					if (png.info_ptr == 0)
					{
						std::cout << "ERROR HEIGHTMAP_NORMAL: PNG info is null." << std::endl;
					}
					else
					{
						unsigned int width = png.info_ptr->width;
						unsigned int height = png.info_ptr->height;

						glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

						if (png.info_ptr->channels == 3)
						{
							normalMap = new Texture2D(GL_RGB, GL_RGB, false, false, width, height, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR, GL_UNSIGNED_BYTE, 0);
							normalMap->BufferImage(width, height, GL_UNSIGNED_BYTE, png.image_data);
						}
						else
						{
							std::cout << "Error HEIGHTMAP_NORMAL: No Channel count of 3." << std::endl;
						}
					}
				}
			}
			else
			{
				std::cout << "Error HEIGHTMAP: No Channel count of 3." << std::endl;
			}
		}
	}
}

HeightMap::~HeightMap()
{
	
}

HeightMap::HeightMap() : Texture2D(GL_RG32F, GL_RG, false, false, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR), normalMap(0), heightScale(0)
{
}

HeightMap* HeightMap::GenerateHeightMap(const unsigned int width, const unsigned int height, const unsigned int method, const unsigned int iterations, const float heightScale, const std::string& filename, const std::string& filename_normal, const float normal_smoothness)
{
	//Init OpenGL State
	OpenGLState::Instance().enable(GL_BLEND);
	glBlendFunc(GL_ONE, GL_ONE);
	OpenGLState::Instance().disable(GL_CULL_FACE);
	OpenGLState::Instance().disable(GL_DEPTH_TEST);

	//Save viewport
	GLint viewport[4];
	glGetIntegerv(GL_VIEWPORT, viewport);

	//Init Map
	HeightMap* map = new HeightMap();
	map->heightScale = heightScale;
	map->bind(0);
	map->BufferImage(width, height, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	//Init Draw Texture
	Texture2D tex(GL_R32F, GL_RED, false, false, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR);
	tex.bind(0);
	tex.BufferImage(width, height, GL_UNSIGNED_BYTE, 0);
	glBindTexture(GL_TEXTURE_2D, 0);

	GLuint vao, vbo;
	glGenVertexArrays(1, &vao);
	glGenBuffers(1, &vbo);

	//Init VBO
	std::vector<glm::vec2> randomNumbers;
	randomNumbers.reserve(iterations);

	for (unsigned int i = 0; i < iterations; i++)
	{
		glm::vec2 rnd = glm::vec2(random() * 2.0f * PI_F, random() * 2.0f - 1.0f);
		randomNumbers.push_back(rnd);
	}

	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	glBufferData(GL_ARRAY_BUFFER, iterations * sizeof(glm::vec2), randomNumbers.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//Init FBO
	GLuint fbo;
	glGenFramebuffers(1, &fbo);
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, tex.Handle(), 0);

	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Framebuffer cannot be set up" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
		glDeleteFramebuffers(1, &fbo);
		return 0;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Init Shader
	Shader heightMapCreateShader(SHADERPATH + "Misc/CreateHeightmap");

	//Init VAO
	glBindVertexArray(vao);
	glBindBuffer(GL_ARRAY_BUFFER, vbo);
	GLint loc = glGetAttribLocation(heightMapCreateShader.getHandle(), "randomNumber");
	glEnableVertexAttribArray(loc);
	glVertexAttribPointer(loc, 2, GL_FLOAT, GL_FALSE, (GLsizei)0, (GLvoid*)0);
	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	//Draw
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);	

	heightMapCreateShader.bind();
	heightMapCreateShader.setUniform("displacement", 0.01f);
	heightMapCreateShader.setUniform("method", method);

	glBindVertexArray(vao);
	glDrawArrays(GL_POINTS, 0, randomNumbers.size());
	glBindVertexArray(0);

	heightMapCreateShader.unbind();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	
	//Get Max Value
	float maxValue = FLT_MIN;
	float minValue = FLT_MAX;
	GLfloat* pixelsMax = new GLfloat[width * height];
	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, width, height, GL_RED, GL_FLOAT, (GLvoid *)pixelsMax);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	for (unsigned int i = 0; i < height; i++)
	{
		for (unsigned int j = 0; j < width; j++)
		{
			float value = pixelsMax[i * width + j];
			if (value > maxValue)
			{
				maxValue = value;
			}
			if (value < minValue)
			{
				minValue = value;
			}
		}
	}

	delete[] pixelsMax;

	//Copy Texture
	GLuint vaoC;
	glGenVertexArrays(1, &vaoC);
	Shader cpyShader(SHADERPATH + "Misc/BlitNormalizeTextureAndAddScale");
	GLuint fboC;
	glGenFramebuffers(1, &fboC);
	glBindFramebuffer(GL_FRAMEBUFFER, fboC);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, map->Handle(), 0);

	GLenum statusC = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (statusC != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Framebuffer cannot be set up" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
		glDeleteFramebuffers(1, &fbo);
		glDeleteVertexArrays(1, &vaoC);
		glDeleteFramebuffers(1, &fboC);
		return 0;
	}

	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	cpyShader.bind();

	tex.bind(0);
	cpyShader.setUniform("sourceTexture", (GLint)0);
	cpyShader.setUniform("normalizeFac", 1.0f / (maxValue-minValue));
	cpyShader.setUniform("addValue", -minValue);
	cpyShader.setUniform("heightScale", heightScale);

	glBindVertexArray(vaoC);
	glDrawArrays(GL_POINTS, 0, 1);
	glBindVertexArray(0);

	cpyShader.unbind();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	//Generate Normal Map
	map->normalMap = new Texture2D(GL_RGB, GL_RGB, false, false, width, height, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR, GL_UNSIGNED_BYTE, 0);

	Texture2D gX(GL_R32F, GL_RED, false, false, width, height, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR, GL_UNSIGNED_BYTE, 0);
	Texture2D gY(GL_R32F, GL_RED, false, false, width, height, GL_CLAMP_TO_EDGE, GL_CLAMP_TO_EDGE, GL_LINEAR, GL_LINEAR, GL_UNSIGNED_BYTE, 0);

	auto ip = ImageProcess::Instance();
	std::vector<glm::vec4> row, col;
	row.push_back(glm::vec4(-1));
	row.push_back(glm::vec4(-2));
	row.push_back(glm::vec4(0));
	row.push_back(glm::vec4(2));
	row.push_back(glm::vec4(1));
	col.push_back(glm::vec4(1));
	col.push_back(glm::vec4(4));
	col.push_back(glm::vec4(6));
	col.push_back(glm::vec4(4));
	col.push_back(glm::vec4(1));
	ip.convoluteSeparable(*map, "rg32f", gX, "r32f", row, col, (unsigned char)32, (unsigned char)32, (unsigned char)5, (unsigned char)5, glm::ivec4(0, 0, width, height));
	ip.convoluteSeparable(*map, "rg32f", gY, "r32f", col, row, (unsigned char)32, (unsigned char)32, (unsigned char)5, (unsigned char)5, glm::ivec4(0, 0, width, height));

	GLuint vaoNM;
	glGenVertexArrays(1, &vaoNM);
	Shader nmShader(SHADERPATH + "Misc/NormalMapFromDerivatives");
	GLuint fboNM;
	glGenFramebuffers(1, &fboNM);
	glBindFramebuffer(GL_FRAMEBUFFER, fboNM);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, map->normalMap->Handle(), 0);

	GLenum statusNM = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (statusNM != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Framebuffer cannot be set up" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glDeleteBuffers(1, &vbo);
		glDeleteVertexArrays(1, &vao);
		glDeleteFramebuffers(1, &fbo);
		glDeleteVertexArrays(1, &vaoC);
		glDeleteFramebuffers(1, &fboC);
		glDeleteVertexArrays(1, &vaoNM);
		glDeleteFramebuffers(1, &fboNM);
		delete map->normalMap;
		map->normalMap = 0;
		return 0;
	}

	glDrawBuffer(GL_COLOR_ATTACHMENT0);
	glViewport(0, 0, width, height);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	nmShader.bind();
	
	gX.bind(0);
	nmShader.setUniform("gX", (GLint)0);
	gY.bind(1);
	nmShader.setUniform("gY", (GLint)1);
	nmShader.setUniform("smoothness", normal_smoothness);

	glBindVertexArray(vaoNM);
	glDrawArrays(GL_POINTS, 0, 1);
	glBindVertexArray(0);

	nmShader.unbind();

	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	///////////////////////////////////////////////////////////////////////////////
	/// SAVE FILES ////////////////////////////////////////////////////////////////
	///////////////////////////////////////////////////////////////////////////////

	if (filename.length() > 0)
	{
		uint8_t *pixels = new uint8_t[width * height * 3];
		// copy pixels from screen
		glBindFramebuffer(GL_FRAMEBUFFER, fboC);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, width, height, GL_RG, GL_UNSIGNED_BYTE, (GLvoid *)pixels);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		unsigned int pixelPos = width * height * 2 - 2;
		for (unsigned int i = width * height * 3 - 1; i > 0; i--)
		{
			if (i % 3 == 0)
			{
				pixels[i] = (uint8_t)glm::max(((int)pixels[pixelPos] * 2 - 255), 0);
				pixelPos -= 2;
			}
			else
			{
				pixels[i] = (uint8_t)0;
			}
		}

		std::string file = TEXTUREPATH + filename;
		png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!png)
		{
			std::cout << "Unable to create write struct" << std::endl;
		}

		png_infop info = png_create_info_struct(png);
		if (!info) {
			png_destroy_write_struct(&png, &info);
			std::cout << "Unable to create info struct" << std::endl;
		}

		FILE *fp = fopen(file.c_str(), "wb");
		if (!fp) {
			png_destroy_write_struct(&png, &info);
			std::cout << "Unable to open file" << std::endl;
		}

		png_init_io(png, fp);
		unsigned int depth = 8;
		png_set_IHDR(png, info, width, height, depth, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_colorp palette = (png_colorp)png_malloc(png, PNG_MAX_PALETTE_LENGTH * sizeof(png_color));
		if (!palette) {
			fclose(fp);
			png_destroy_write_struct(&png, &info);
			std::cout << "Unable to create palette" << std::endl;
		}
		png_set_PLTE(png, info, palette, PNG_MAX_PALETTE_LENGTH);
		png_write_info(png, info);
		png_set_packing(png);

		png_bytepp rows = (png_bytepp)png_malloc(png, height * sizeof(png_bytep));
		for (unsigned int i = 0; i < height; ++i)
			rows[i] = (png_bytep)(pixels + (height - i - 1) * width * 3);

		png_write_image(png, rows);
		png_write_end(png, info);
		png_free(png, palette);
		png_destroy_write_struct(&png, &info);

		fclose(fp);
		delete[] rows;
		delete[] pixels;
	}

	if (filename_normal.length() > 0)
	{
		uint8_t *pixels = new uint8_t[width * height * 3];
		// copy pixels from screen
		glBindFramebuffer(GL_FRAMEBUFFER, fboNM);
		glPixelStorei(GL_PACK_ALIGNMENT, 1);
		glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)pixels);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);

		std::string file = TEXTUREPATH + filename_normal;
		png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
		if (!png)
		{
			std::cout << "Unable to create write struct" << std::endl;
		}

		png_infop info = png_create_info_struct(png);
		if (!info) {
			png_destroy_write_struct(&png, &info);
			std::cout << "Unable to create info struct" << std::endl;
		}

		FILE *fp = fopen(file.c_str(), "wb");
		if (!fp) {
			png_destroy_write_struct(&png, &info);
			std::cout << "Unable to open file" << std::endl;
		}

		png_init_io(png, fp);
		unsigned int depth = 8;
		png_set_IHDR(png, info, width, height, depth, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
			PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
		png_colorp palette = (png_colorp)png_malloc(png, PNG_MAX_PALETTE_LENGTH * sizeof(png_color));
		if (!palette) {
			fclose(fp);
			png_destroy_write_struct(&png, &info);
			std::cout << "Unable to create palette" << std::endl;
		}
		png_set_PLTE(png, info, palette, PNG_MAX_PALETTE_LENGTH);
		png_write_info(png, info);
		png_set_packing(png);

		png_bytepp rows = (png_bytepp)png_malloc(png, height * sizeof(png_bytep));
		for (unsigned int i = 0; i < height; ++i)
			rows[i] = (png_bytep)(pixels + (height - i - 1) * width * 3);

		png_write_image(png, rows);
		png_write_end(png, info);
		png_free(png, palette);
		png_destroy_write_struct(&png, &info);

		fclose(fp);
		delete[] rows;
		delete[] pixels;
	}

	//Cleanup
	glDeleteBuffers(1, &vbo);
	glDeleteVertexArrays(1, &vao);
	glDeleteFramebuffers(1, &fbo);
	glDeleteVertexArrays(1, &vaoC);
	glDeleteFramebuffers(1, &fboC);
	glDeleteVertexArrays(1, &vaoNM);
	glDeleteFramebuffers(1, &fboNM);

	glViewport(viewport[0], viewport[1], viewport[2], viewport[3]);
	glDrawBuffer(GL_BACK);

	OpenGLState::Instance().disable(GL_BLEND);
	OpenGLState::Instance().enable(GL_CULL_FACE);
	OpenGLState::Instance().enable(GL_DEPTH_TEST);

	return map;
}