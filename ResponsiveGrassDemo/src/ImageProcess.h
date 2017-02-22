/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef IMAGEPROCESS_H
#define IMAGEPROCESS_H

#include "Common.h"
#include "Shader.h"
#include "Texture2D.h"

#include <map>

class ImageProcess
{
public:
	static ImageProcess& Instance();

	~ImageProcess();

	void convolute(const Texture2D& texture, const std::string& textureFormat, const std::vector<glm::vec4>& filter, const unsigned char tileWidth, const unsigned char tileHeight, const unsigned char filterWidth, const unsigned char filterHeight, const glm::ivec4& imageBounds);
	void convolute(const Texture2D& texture_in, const std::string& textureFormat_in, const Texture2D& texture_out, const std::string& textureFormat_out, const std::vector<glm::vec4>& filter, const unsigned char tileWidth, const unsigned char tileHeight, const unsigned char filterWidth, const unsigned char filterHeight, const glm::ivec4& imageBounds);
	void convoluteSeparable(const Texture2D& texture, const std::string& textureFormat, const std::vector<glm::vec4>& filterRow, const std::vector<glm::vec4>& filterCol, const unsigned char tileWidth, const unsigned char tileHeight, const unsigned char filterWidth, const unsigned char filterHeight, const glm::ivec4& imageBounds);
	void convoluteSeparable(const Texture2D& texture_in, const std::string& textureFormat_in, const Texture2D& texture_out, const std::string& textureFormat_out, const std::vector<glm::vec4>& filterRow, const std::vector<glm::vec4>& filterCol, const unsigned char tileWidth, const unsigned char tileHeight, const unsigned char filterWidth, const unsigned char filterHeight, const glm::ivec4& imageBounds);
	void cleanUpGL();
private:
	ImageProcess();

	static ImageProcess* instance;

	std::map<unsigned long, Shader*> convoluteShader;
	std::map<std::string, unsigned char> formatCharMap;
	std::map<std::string, GLenum> formatEnumMap;

	const unsigned long getHash(const bool sep, const unsigned char a, const unsigned char b, const unsigned char c, const unsigned char d, const unsigned char fmt1, const unsigned char fmt2) const
	{
		unsigned long hash = (unsigned int)a;
		hash <<= 8;
		hash |= (unsigned int)b;
		hash <<= 8;
		hash |= (unsigned int)c;
		hash <<= 8;
		hash |= (unsigned int)d;
		hash <<= 8;
		hash |= (unsigned int)fmt1;
		hash <<= 8;
		hash |= (unsigned int)fmt2;
		hash <<= 1;
		hash |= (sep ? 1u : 0u);

		return hash;
	}

	Shader* addShader(const bool separable, const unsigned long hash, const unsigned char tileWidth, const unsigned char tileHeight, const unsigned char filterWidth, const unsigned char filterHeight, const std::string& textureFmt1, const std::string& textureFmt2);
	Shader* getShader(const bool separable, const unsigned char tileWidth, const unsigned char tileHeight, const unsigned char filterWidth, const unsigned char filterHeight, const std::string& textureFmt1, const std::string& textureFmt2);
};

#endif