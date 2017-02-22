/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "Texture2D.h"
#include <iostream>
#include <fstream>

Texture2D::Texture2D(const GLenum _internalFormat, const GLenum _format, const bool _generateMipMaps, const bool _enableMultisampling, const GLint _wrapX, const GLint _wrapY,
	const GLint _minFilter, const GLint _magFilter) : internalFormat(_internalFormat), format(_format), generateMipmaps(_generateMipMaps), enableMultisampling(_enableMultisampling), width(0), height(0)
{
	glGenTextures(1, &handle);

	if (enableMultisampling)
	{
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, handle);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MIN_FILTER, _minFilter);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_MAG_FILTER, _magFilter);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_S, _wrapX);
		glTexParameteri(GL_TEXTURE_2D_MULTISAMPLE, GL_TEXTURE_WRAP_T, _wrapY);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, handle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _magFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _wrapX);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _wrapY);
	}
}

Texture2D::Texture2D(const GLenum _internalFormat, const GLenum _format, const bool _generateMipMaps, const bool _enableMultisampling, const unsigned int _width, const unsigned int _height, const GLint _wrapX, const GLint _wrapY,
	const GLint _minFilter, const GLint _magFilter, const GLenum _dataType, GLvoid* _data) : internalFormat(_internalFormat), format(_format), generateMipmaps(_generateMipMaps), enableMultisampling(_enableMultisampling), width(_width), height(_height)
{
	glGenTextures(1, &handle);

	if (enableMultisampling)
	{
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, handle);
		glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, MS_SAMPLES, internalFormat, width, height, GL_TRUE);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, handle);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, _minFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, _magFilter);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, _wrapX);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, _wrapY);
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, width, height, 0, format, _dataType, _data);
		if (generateMipmaps)
		{
			glGenerateMipmap(GL_TEXTURE_2D);
		}
	}
}

Texture2D::~Texture2D()
{
	glDeleteTextures(1, &handle);
}

void Texture2D::BufferImage(const unsigned int _width, const unsigned int _height, GLenum _dataType, GLvoid* _data)
{
	width = _width;
	height = _height;
	if (enableMultisampling)
	{
		glTexImage2D(GL_TEXTURE_2D_MULTISAMPLE, 0, internalFormat, _width, _height, 0, format, _dataType, _data);
		
		if (generateMipmaps)
			glGenerateMipmap(GL_TEXTURE_2D_MULTISAMPLE);
	}
	else
	{
		glTexImage2D(GL_TEXTURE_2D, 0, internalFormat, _width, _height, 0, format, _dataType, _data);

		if (generateMipmaps)
			glGenerateMipmap(GL_TEXTURE_2D);
	}
}

void Texture2D::BufferSubImage(unsigned int _left, unsigned int _top, unsigned int _width, unsigned int _height, GLenum _type, GLvoid* _data)
{
	if (enableMultisampling)
	{
		glTexSubImage2D(GL_TEXTURE_2D_MULTISAMPLE, 0, _left, _top, _width, _height, format, _type, _data);

		if (generateMipmaps)
			glGenerateMipmap(GL_TEXTURE_2D_MULTISAMPLE);
	}
	else
	{
		glTexSubImage2D(GL_TEXTURE_2D, 0, _left, _top, _width, _height, format, _type, _data);

		if (generateMipmaps)
			glGenerateMipmap(GL_TEXTURE_2D);
	}
}

void Texture2D::bind(const GLint textureUnit) const
{
	glActiveTexture(GL_TEXTURE0 + textureUnit);
	if (enableMultisampling)
	{
		glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, handle);
	}
	else
	{
		glBindTexture(GL_TEXTURE_2D, handle);
	}
}

GLuint Texture2D::Handle() const
{
	return handle;
}

Texture2D* Texture2D::loadTextureFromFile(const std::string& fileName, const bool gammaCorrection, const bool _generateMipMaps, const bool _enableMultisampling, const GLint _wrapX, const GLint _wrapY, const GLint _minFilter, const GLint _magFilter)
{
	std::ifstream ifile(fileName.c_str());
	if (ifile.good())
	{
		PNGOutput png = loadPNGFile(fileName, gammaCorrection);

		if (png.info_ptr == 0)
		{
			std::cout << "ERROR: PNG info is null." << std::endl;
			return 0;
		}

		unsigned int width = png.info_ptr->width;
		unsigned int height = png.info_ptr->height;

		glPixelStorei(GL_UNPACK_ALIGNMENT, 1);

		if (png.info_ptr->channels == 4)
		{
			Texture2D* ptr = new Texture2D(gammaCorrection ? GL_SRGB_ALPHA : GL_RGBA, GL_RGBA, _generateMipMaps, _enableMultisampling, width, height, _wrapX, _wrapY, _minFilter, _magFilter, GL_UNSIGNED_BYTE, png.image_data);
			delete png.image_data;
			return ptr;
		}
		else
		{
			if (png.info_ptr->channels == 3)
			{
				Texture2D* ptr = new Texture2D(gammaCorrection ? GL_SRGB : GL_RGB, GL_RGB, _generateMipMaps, _enableMultisampling, width, height, _wrapX, _wrapY, _minFilter, _magFilter, GL_UNSIGNED_BYTE, png.image_data);
				delete png.image_data;
				return ptr;
			}
			else
			{
				std::cout << "Error: Unknown Channel count." << std::endl;
				return 0;
			}
		}
	}
	else
	{
		std::cout << "ERROR: Texture file not found. " << fileName << std::endl;
		return 0;
	}
}

Texture2D::PNGOutput Texture2D::loadPNGFile(const std::string& fileName, const bool gammaCorrection)
{
	FILE* _file = fopen(fileName.c_str(), "rb");

	Texture2D::PNGOutput out = Texture2D::PNGOutput();

	//Check png signature
	unsigned char header[8];
	fread(header, 1, 8, _file);

	if (!png_check_sig(header, 8))
	{
		std::cout << "Png signature check failed." << std::endl;
		return out;
	}

	png_structp png_ptr = png_create_read_struct(PNG_LIBPNG_VER_STRING, NULL, NULL, NULL);
	if (!png_ptr)
	{
		std::cout << "PNG out of memory" << std::endl;
		return out;
	}

	png_infop info_ptr = png_create_info_struct(png_ptr);
	if (!info_ptr)
	{
		png_destroy_read_struct(&png_ptr, NULL, NULL);
		std::cout << "PNG out of memory" << std::endl;
		return out;
	}

	if (setjmp(png_jmpbuf(png_ptr)))
	{
		png_destroy_read_struct(&png_ptr, &info_ptr, NULL);
		std::cout << "PNG: setjmp failed" << std::endl;
		return out;
	}

	png_init_io(png_ptr, _file);
	png_set_sig_bytes(png_ptr, 8);
	png_read_info(png_ptr, info_ptr);

	png_uint_32 height, width;
	int bit_depth, color_type;

	png_get_IHDR(png_ptr, info_ptr, &width, &height, &bit_depth, &color_type,
		NULL, NULL, NULL);

	double gamma = 0.0, screengamma = 2.2;

	char* screengammastr;
	if ((screengammastr = getenv("SCREEN_GAMMA")) != NULL)
		screengamma = atof(screengammastr);

	if (!gammaCorrection || !png_get_gAMA(png_ptr, info_ptr, &gamma))
		gamma = 1.0 / screengamma;

	png_set_gamma(png_ptr, screengamma, gamma);

	if (color_type == PNG_COLOR_TYPE_PALETTE)
		png_set_palette_to_rgb(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY && bit_depth < 8)
		png_set_expand_gray_1_2_4_to_8(png_ptr);
	if (png_get_valid(png_ptr, info_ptr, PNG_INFO_tRNS))
		png_set_tRNS_to_alpha(png_ptr);
	if (bit_depth == 16)
		png_set_strip_16(png_ptr);
	if (color_type == PNG_COLOR_TYPE_GRAY ||
		color_type == PNG_COLOR_TYPE_GRAY_ALPHA)
		png_set_gray_to_rgb(png_ptr);

	png_read_update_info(png_ptr, info_ptr);

	//Read
	png_size_t rowbytes = png_get_rowbytes(png_ptr, info_ptr);
	int channels = (int)png_get_channels(png_ptr, info_ptr);

	unsigned char *image_data;
	png_bytepp row_pointers = 0;

	if ((image_data = (unsigned char*)malloc(rowbytes*height)) == 0) {
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		return out;
	}
	if ((row_pointers = (png_bytepp)malloc(height*sizeof(png_bytep))) == 0) {
		png_destroy_read_struct(&png_ptr, &info_ptr, 0);
		free(image_data);
		image_data = nullptr;
		return out;
	}

	for (unsigned int i = 0; i < height; i++)
		row_pointers[i] = image_data + (height - i - 1) * rowbytes;

	png_read_image(png_ptr, row_pointers);

	free(row_pointers);

	png_read_end(png_ptr, NULL);

	out.image_data = image_data;
	out.info_ptr = info_ptr;
	return out;
}