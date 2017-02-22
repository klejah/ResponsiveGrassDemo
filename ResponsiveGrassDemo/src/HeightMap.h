/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef HEIGHTMAP_H
#define HEIGHTMAP_H

#include "Texture2D.h"
#include "Camera.h"

class HeightMap : public Texture2D
{
public:
	HeightMap(const std::string& fileName, const std::string& fileName_normal, const float heightScale);
	~HeightMap();

	Texture2D* normalMap;
	float heightScale;

	static HeightMap* GenerateHeightMap(const unsigned int width, const unsigned int height, const unsigned int method, const unsigned int iterations, const float heightScale, const std::string& filename, const std::string& filename_normal, const float normal_smoothness = 0.5f);
private:
	HeightMap();
};

#endif