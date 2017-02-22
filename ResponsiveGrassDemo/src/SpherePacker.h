/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef SPHEREPACKER_H
#define SPHEREPACKER_H

#include "Common.h"
#include "Geometry.h"
#include "Clock.h"
#include "ThreadPool.h"
#include "Shader.h"

#define SPHERE_PACKER_THREAD_COUNT 6

class SpherePacker
{
private:
	Shader* packingShader;
	Clock timer;
	ThreadPool pool;

	SpherePacker() : timer(), packingShader(0), pool(SPHERE_PACKER_THREAD_COUNT)
	{
		packingShader = new Shader(SHADERPATH + "Misc/SpherePacking");
	}

	Shader& getPackingShader()
	{
		return *packingShader;
	}
public:
	~SpherePacker()
	{

	}

	static SpherePacker& getInstance()
	{
		static SpherePacker* instance = 0;
		if (instance == 0)
		{
			instance = new SpherePacker();
		}
		return *instance;
	}

	static std::vector<glm::vec4> packSpheres(const std::vector<Geometry::TriangleFace>& faces, const unsigned int maxIterations, const bool deleteInnerSpheres);
};

#endif