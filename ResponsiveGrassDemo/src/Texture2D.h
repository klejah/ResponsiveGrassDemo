/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef TEXTURE_2D_H
#define TEXTURE_2D_H

#include "Common.h"
#include <string>
#include "libpng/png.h"
#include "libpng/pngstruct.h"
#include "libpng/pnginfo.h"

class Texture2D
{
private:
	GLuint handle;

	GLenum internalFormat, format;
	bool generateMipmaps;
	bool enableMultisampling;
	unsigned int width, height;

public:
	struct PNGOutput {
		unsigned char *image_data;
		png_infop info_ptr;
	};

	static Texture2D::PNGOutput Texture2D::loadPNGFile(const std::string& fileName, const bool gammaCorrection);

public:
	Texture2D(const GLenum _internalFormat, const GLenum _format, const bool _generateMipMaps, const bool _enableMultisampling, const GLint _wrapX, const GLint _wrapY, const GLint _minFilter, const GLint _magFilter);
	Texture2D(const GLenum _internalFormat, const GLenum _format, const bool _generateMipMaps, const bool _enableMultisampling, const unsigned int _width, const unsigned int _height, const GLint _wrapX, const GLint _wrapY,
		const GLint _minFilter, const GLint _magFilter, const GLenum _dataType, GLvoid* _data);
	virtual ~Texture2D();

	void BufferImage(const unsigned int _width, const unsigned int _height, GLenum _dataType, GLvoid* _data);
	void BufferSubImage(unsigned int _left, unsigned int _top, unsigned int _width, unsigned int _height, GLenum _dataType, GLvoid* _data);

	void bind(const GLint textureUnit) const;

	GLuint Handle() const;

	unsigned int Width() const { return width; }
	unsigned int Height() const { return height; }

	static Texture2D* loadTextureFromFile(const std::string& fileName, const bool gammaCorrection, const bool _generateMipMaps, const bool _enableMultisampling, const GLint _wrapX, const GLint _wrapY, const GLint _minFilter, const GLint _magFilter);
};

#endif