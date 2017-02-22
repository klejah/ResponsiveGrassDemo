/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "ImageProcess.h"

ImageProcess* ImageProcess::instance = 0;

ImageProcess::ImageProcess() : convoluteShader(), formatCharMap(), formatEnumMap()
{
	unsigned char i = 0;
	formatCharMap.insert(std::pair<std::string, unsigned char>("rgba32f", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rgba16f", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rg32f", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rg16f", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("r11f_g11f_b10f", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("r32f", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("r16f", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rgba16", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rgb10_a2", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rgba8", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rg16", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rg8", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("r16", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("r8", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rgba16_snorm", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rgba8_snorm", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rg16_snorm", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rg8_snorm", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("r16_snorm", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("r8_snorm", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rgba32i", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rgba16i", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rgba8i", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rg32i", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rg16i", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rg8i", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("r32i", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("r16i", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("r8i", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rgba32ui", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rgba16ui", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rgb10_a2ui", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rgba8ui", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rg32ui", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rg16ui", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("rg8ui", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("r32ui", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("r16ui", i++));
	formatCharMap.insert(std::pair<std::string, unsigned char>("r8ui", i++));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rgba32f", GL_RGBA32F));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rgba16f", GL_RGBA16F));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rg32f", GL_RG32F));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rg16f", GL_RG16F));
	formatEnumMap.insert(std::pair<std::string, GLenum>("r11f_g11f_b10f", GL_R11F_G11F_B10F));
	formatEnumMap.insert(std::pair<std::string, GLenum>("r32f", GL_R32F));
	formatEnumMap.insert(std::pair<std::string, GLenum>("r16f", GL_R16F));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rgba16", GL_RGBA16));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rgb10_a2", GL_RGB10_A2));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rgba8", GL_RGBA8));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rg16", GL_RG16));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rg8", GL_RG8));
	formatEnumMap.insert(std::pair<std::string, GLenum>("r16", GL_R16));
	formatEnumMap.insert(std::pair<std::string, GLenum>("r8", GL_R8));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rgba16_snorm", GL_RGBA16_SNORM));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rgba8_snorm", GL_RGBA8_SNORM));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rg16_snorm", GL_RG16_SNORM));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rg8_snorm", GL_RG8_SNORM));
	formatEnumMap.insert(std::pair<std::string, GLenum>("r16_snorm", GL_R16_SNORM));
	formatEnumMap.insert(std::pair<std::string, GLenum>("r8_snorm", GL_R8_SNORM));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rgba32i", GL_RGBA32I));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rgba16i", GL_RGBA16I));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rgba8i", GL_RGBA8I));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rg32i", GL_RG32I));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rg16i", GL_RG16I));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rg8i", GL_RG8I));
	formatEnumMap.insert(std::pair<std::string, GLenum>("r32i", GL_R32I));
	formatEnumMap.insert(std::pair<std::string, GLenum>("r16i", GL_R16I));
	formatEnumMap.insert(std::pair<std::string, GLenum>("r8i", GL_R8I));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rgba32ui", GL_RGBA32UI));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rgba16ui", GL_RGBA16UI));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rgb10_a2ui", GL_RGB10_A2UI));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rgba8ui", GL_RGBA8UI));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rg32ui", GL_RG32UI));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rg16ui", GL_RG16UI));
	formatEnumMap.insert(std::pair<std::string, GLenum>("rg8ui", GL_RG8UI));
	formatEnumMap.insert(std::pair<std::string, GLenum>("r32ui", GL_R32UI));
	formatEnumMap.insert(std::pair<std::string, GLenum>("r16ui", GL_R16UI));
	formatEnumMap.insert(std::pair<std::string, GLenum>("r8ui", GL_R8UI));
}

ImageProcess::~ImageProcess()
{

}

ImageProcess& ImageProcess::Instance()
{
	if (instance == 0)
	{
		instance = new ImageProcess();
	}
	return *instance;
}

void ImageProcess::convolute(const Texture2D& texture, const std::string& textureFormat, const std::vector<glm::vec4>& filter, const unsigned char tileWidth, const unsigned char tileHeight, const unsigned char filterWidth, const unsigned char filterHeight, const glm::ivec4& imageBounds)
{
	Shader* shader = getShader(false, tileWidth, tileHeight, filterWidth, filterHeight, textureFormat, textureFormat);
	
	shader->bind();
	shader->setUniform("weight[0]", filter);
	shader->setUniform("imageBounds", imageBounds);
	shader->setUniform("separateOutputImage", (GLboolean)false);

	GLenum fmt = GL_RGBA16;
	auto fm = formatEnumMap.find(textureFormat);
	if (fm != formatEnumMap.end())
	{
		fmt = fm->second;
	}
	glBindImageTexture(0, texture.Handle(), 0, GL_FALSE, 0, GL_READ_WRITE, fmt);

	shader->dispatch(1 + texture.Width() / tileWidth, 1 + texture.Height() / tileHeight, 1);

	shader->unbind();
}

void ImageProcess::convolute(const Texture2D& texture_in, const std::string& textureFormat_in, const Texture2D& texture_out, const std::string& textureFormat_out, const std::vector<glm::vec4>& filter, const unsigned char tileWidth, const unsigned char tileHeight, const unsigned char filterWidth, const unsigned char filterHeight, const glm::ivec4& imageBounds)
{
	Shader* shader = getShader(false, tileWidth, tileHeight, filterWidth, filterHeight, textureFormat_in, textureFormat_out);

	shader->bind();
	shader->setUniform("weight[0]", filter);
	shader->setUniform("imageBounds", imageBounds);
	shader->setUniform("separateOutputImage", (GLboolean)true);

	GLenum fmt_in = GL_RGBA16;
	GLenum fmt_out = GL_RGBA16;
	auto fm_in = formatEnumMap.find(textureFormat_in);
	if (fm_in != formatEnumMap.end())
	{
		fmt_in = fm_in->second;
	}
	auto fm_out = formatEnumMap.find(textureFormat_out);
	if (fm_out != formatEnumMap.end())
	{
		fmt_out = fm_out->second;
	}
	glBindImageTexture(0, texture_in.Handle(), 0, GL_FALSE, 0, GL_READ_ONLY, fmt_in);
	glBindImageTexture(1, texture_out.Handle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, fmt_out);

	shader->dispatch(1 + texture_in.Width() / tileWidth, 1 + texture_in.Height() / tileHeight, 1);

	shader->unbind();
}

void ImageProcess::convoluteSeparable(const Texture2D& texture, const std::string& textureFormat, const std::vector<glm::vec4>& filterRow, const std::vector<glm::vec4>& filterCol, const unsigned char tileWidth, const unsigned char tileHeight, const unsigned char filterWidth, const unsigned char filterHeight, const glm::ivec4& imageBounds)
{
	Shader* shader = getShader(true, tileWidth, tileHeight, filterWidth, filterHeight, textureFormat, textureFormat);

	shader->bind();
	shader->setUniform("rowWeight[0]", filterRow);
	shader->setUniform("colWeight[0]", filterCol);
	shader->setUniform("imageBounds", imageBounds);
	shader->setUniform("separateOutputImage", (GLboolean)false);

	GLenum fmt = GL_RGBA16;
	auto fm = formatEnumMap.find(textureFormat);
	if (fm != formatEnumMap.end())
	{
		fmt = fm->second;
	}
	glBindImageTexture(0, texture.Handle(), 0, GL_FALSE, 0, GL_READ_WRITE, fmt);

	shader->dispatch(1 + texture.Width() / tileWidth, 1 + texture.Height() / tileHeight, 1);

	shader->unbind();
}

void ImageProcess::convoluteSeparable(const Texture2D& texture_in, const std::string& textureFormat_in, const Texture2D& texture_out, const std::string& textureFormat_out, const std::vector<glm::vec4>& filterRow, const std::vector<glm::vec4>& filterCol, const unsigned char tileWidth, const unsigned char tileHeight, const unsigned char filterWidth, const unsigned char filterHeight, const glm::ivec4& imageBounds)
{
	Shader* shader = getShader(true, tileWidth, tileHeight, filterWidth, filterHeight, textureFormat_in, textureFormat_out);

	shader->bind();
	shader->setUniform("rowWeight[0]", filterRow);
	shader->setUniform("colWeight[0]", filterCol);
	shader->setUniform("imageBounds", imageBounds);
	shader->setUniform("separateOutputImage", (GLboolean)true);

	GLenum fmt_in = GL_RGBA16;
	GLenum fmt_out = GL_RGBA16;
	auto fm_in = formatEnumMap.find(textureFormat_in);
	if (fm_in != formatEnumMap.end())
	{
		fmt_in = fm_in->second;
	}
	auto fm_out = formatEnumMap.find(textureFormat_out);
	if (fm_out != formatEnumMap.end())
	{
		fmt_out = fm_out->second;
	}
	glBindImageTexture(0, texture_in.Handle(), 0, GL_FALSE, 0, GL_READ_ONLY, fmt_in);
	glBindImageTexture(1, texture_out.Handle(), 0, GL_FALSE, 0, GL_WRITE_ONLY, fmt_out);

	shader->dispatch(1 + texture_in.Width() / tileWidth, 1 + texture_in.Height() / tileHeight, 1);

	shader->unbind();
}

void ImageProcess::cleanUpGL()
{
	for (std::map<unsigned long, Shader*>::iterator iter = convoluteShader.begin(); iter != convoluteShader.end(); iter++)
	{
		delete iter->second;
		iter->second = 0;
	}
}

Shader* ImageProcess::addShader(const bool separable, const unsigned long hash, const unsigned char tileWidth, const unsigned char tileHeight, const unsigned char filterWidth, const unsigned char filterHeight, const std::string& textureFmt1, const std::string& textureFmt2)
{
	std::string filename;
	if (separable)
	{
		filename = SHADERPATH + "Misc/SeparableConvolution";
	}
	else
	{
		filename = SHADERPATH + "Misc/GeneralConvolution";
	}
	std::vector<std::string> s;
	std::vector<std::string> r;
	s.push_back("TILE_WIDTH");
	r.push_back(std::to_string((unsigned int)tileWidth));
	s.push_back("TILE_HEIGHT");
	r.push_back(std::to_string((unsigned int)tileHeight));
	s.push_back("FILTER_WIDTH");
	r.push_back(std::to_string((unsigned int)filterWidth));
	s.push_back("FILTER_HEIGHT");
	r.push_back(std::to_string((unsigned int)filterHeight));
	s.push_back("FORMAT_IN");
	r.push_back(textureFmt1);
	s.push_back("FORMAT_OUT");
	r.push_back(textureFmt2);

	glm::ivec2 tileSize = glm::ivec2((unsigned int)tileWidth, (unsigned int)tileHeight);
	glm::ivec2 filterOffset = glm::ivec2((unsigned int)filterWidth / 2, (unsigned int)filterHeight / 2);
	glm::ivec2 neighborhoodSize = tileSize + 2 * filterOffset;

	s.push_back("NEIGHBORHOOD_WIDTH");
	r.push_back(std::to_string(neighborhoodSize.x));
	s.push_back("NEIGHBORHOOD_HEIGHT");
	r.push_back(std::to_string(neighborhoodSize.y));

	Shader* shader = new Shader(filename, s, r);

	convoluteShader.insert(std::pair<unsigned long, Shader*>(hash, shader));

	return shader;
}

Shader* ImageProcess::getShader(const bool separable, const unsigned char tileWidth, const unsigned char tileHeight, const unsigned char filterWidth, const unsigned char filterHeight, const std::string& textureFmt1, const std::string& textureFmt2)
{
	Shader* shader = 0;

	unsigned char format1Char = 0;
	unsigned char format2Char = 0;
	auto fm1 = formatCharMap.find(textureFmt1);
	if (fm1 != formatCharMap.end())
	{
		format1Char = fm1->second;
	}
	auto fm2 = formatCharMap.find(textureFmt2);
	if (fm2 != formatCharMap.end())
	{
		format2Char = fm2->second;
	}
	unsigned long hash = getHash(separable, tileWidth, tileHeight, filterWidth, filterHeight, format1Char, format2Char);

	auto mapShader = convoluteShader.find(hash);
	if (mapShader != convoluteShader.end())
	{
		shader = mapShader->second;
	}
	else
	{
		shader = addShader(separable, hash, tileWidth, tileHeight, filterWidth, filterHeight, textureFmt1, textureFmt2);
	}

	return shader;
}