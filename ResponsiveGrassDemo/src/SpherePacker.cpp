/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "SpherePacker.h"
#include <iostream>

/**
 * Helper Functions
**/

void GenerateVoxelData(const unsigned int dMin, const unsigned int dMax, const unsigned int height, const unsigned int width, const glm::vec3& gridSize, const glm::vec3& min, const std::vector<Geometry::TriangleFace>& faces, float* voxelData)
{
	const glm::vec3 half = glm::vec3(0.5f, 0.5f, 0.5f);
	for (unsigned int d = dMin; d < dMax; d++)
	{
		for (unsigned int h = 0; h < height; h++)
		{
			for (unsigned int w = 0; w < width; w++)
			{
				glm::vec3 gPos = (glm::vec3(w, h, d) + half) * gridSize + min;

				glm::vec3 closestPoint;
				float nearestDist = FLT_MAX;
				for (unsigned int f = 0; f < faces.size(); f++)
				{
					auto face = faces[f];
					float s, t;
					glm::vec3 cp = closestPointOnTriangle(face.vertices[0].position, face.vertices[1].position, face.vertices[2].position, gPos, s, t);
					float dist = glm::distance(gPos, cp);
					if (dist < nearestDist)
					{
						closestPoint = cp;
						nearestDist = dist;
					}
				}

				voxelData[d * height * width * 3 + h * width * 3 + w * 3 + 0] = closestPoint.x;
				voxelData[d * height * width * 3 + h * width * 3 + w * 3 + 1] = closestPoint.y;
				voxelData[d * height * width * 3 + h * width * 3 + w * 3 + 2] = closestPoint.z;
			}
		}
	}
}

inline unsigned int getIndex(const unsigned int d, const unsigned int h, const unsigned int w, const unsigned int i, const unsigned int width, const unsigned int height)
{
	return d * height * width * 3 + h * width * 3 + w * 3 + i;
}

void UpdateVoxelData(const unsigned int dMin, const unsigned int dMax, const unsigned int height, const unsigned int width, const glm::vec3& gridSize, const glm::vec3& min, const std::vector<glm::vec4>& newSpheres, float* voxelData)
{
	const glm::vec3 half = glm::vec3(0.5f, 0.5f, 0.5f);
	for (unsigned int d = dMin; d < dMax; d++)
	{
		for (unsigned int h = 0; h < height; h++)
		{
			for (unsigned int w = 0; w < width; w++)
			{
				glm::vec3 gPos = (glm::vec3(w, h, d) + half) * gridSize + min;

				glm::vec3 voxelNearest = glm::vec3(voxelData[d * height * width * 3 + h * width * 3 + w * 3 + 0],
					voxelData[d * height * width * 3 + h * width * 3 + w * 3 + 1],
					voxelData[d * height * width * 3 + h * width * 3 + w * 3 + 2]);

				float nearestDist = glm::distance(gPos, voxelNearest);
				bool updated = false;
				for each(glm::vec4 sphere in newSpheres)
				{
					glm::vec3 spherePos = glm::vec3(sphere);
					float dist = glm::distance(gPos, spherePos) - sphere.w;
					if (dist < nearestDist)
					{
						nearestDist = dist;
						glm::vec3 dir = spherePos - gPos;
						dir = glm::normalize(dir);
						voxelNearest = gPos + dir * nearestDist;
						updated = true;
					}
				}

				if (updated)
				{
					//voxelData[getIndex(d, h, w, 0, width, height)] = voxelNearest[0];
					//voxelData[getIndex(d, h, w, 1, width, height)] = voxelNearest[1];
					//voxelData[getIndex(d, h, w, 2, width, height)] = voxelNearest[2];
					voxelData[d * height * width * 3 + h * width * 3 + w * 3 + 0] = voxelNearest[0];
					voxelData[d * height * width * 3 + h * width * 3 + w * 3 + 1] = voxelNearest[1];
					voxelData[d * height * width * 3 + h * width * 3 + w * 3 + 2] = voxelNearest[2];
				}
			}
		}
	}
}

void searchForOuterSphere(const unsigned int idx, const float fac, const std::vector<glm::vec4>& spheres, const std::vector<Geometry::TriangleFace>& faces, bool* isOuterArray)
{
	glm::vec4 sphere = spheres[idx];
	for each(Geometry::TriangleFace f in faces)
	{
		float s, t;
		glm::vec3 cp = closestPointOnTriangle(f.vertices[0].position, f.vertices[1].position, f.vertices[2].position, sphere.xyz, s, t);
		float dist = glm::distance(glm::vec3(sphere.xyz), cp);
		if (dist - sphere.w < fac * sphere.w)
		{
			isOuterArray[idx] = true;
			return;
		}
	}
}

bool intersect(const glm::vec4& s, const std::vector<glm::vec4>& ls)
{
	for each(glm::vec4 sphere in ls)
	{
		if (glm::distance(glm::vec3(s), glm::vec3(sphere)) < s.w + sphere.w)
		{
			return true;
		}
	}

	return false;
}

std::vector<glm::vec4> SpherePacker::packSpheres(const std::vector<Geometry::TriangleFace>& faces, const unsigned int maxIterations, const bool deleteInnerSpheres)
{
	std::vector<glm::vec4> spheres;

	Clock& timer = getInstance().timer;

	glm::vec3 min(FLT_MAX, FLT_MAX, FLT_MAX);
	glm::vec3 max(-FLT_MAX, -FLT_MAX, -FLT_MAX);

	timer.Tick();

	//Find range
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		auto face = faces[i];
		for (unsigned int j = 0; j < 3; j++)
		{
			auto vert = face.vertices[j];
			auto p = vert.position;
			min = glm::min(min, p);
			max = glm::max(max, p);
		}
	}

	glm::vec3 mean = min + (max - min) * 0.5f;

	timer.Tick();
	std::cout << "Find range finished in " << timer.LastFrameTime() << " seconds." << std::endl;

	//Create grid
	unsigned int width = 256;
	unsigned int height = 256;
	unsigned int depth = 256;
	//unsigned int width  = 100;
	//unsigned int height = 100;
	//unsigned int depth  = 100;
	GLfloat * voxelData = new GLfloat[depth * height * width * 3];

	glm::vec3 span = max - min;
	glm::vec3 gridSize = span / glm::vec3(width, height, depth);

	timer.Tick();

	ThreadPool& pool = getInstance().pool;

	for (unsigned int t = 0; t < SPHERE_PACKER_THREAD_COUNT; t++)
	{
		unsigned int dMin = (unsigned int)roundf((float)t * ((float)depth / (float)SPHERE_PACKER_THREAD_COUNT));
		unsigned int dMax = (unsigned int)roundf((float)(t + 1) * ((float)depth / (float)SPHERE_PACKER_THREAD_COUNT));
		pool.AddJob([dMin, dMax, height, width, gridSize, min, faces, voxelData](){
			GenerateVoxelData(dMin, dMax, height, width, gridSize, min, faces, voxelData);
		});
	}

	pool.WaitAll();

	timer.Tick();
	std::cout << "Voxel data created in " << timer.LastFrameTime() << " seconds." << std::endl;

	//Create 3D texture
	GLuint gridTex;
	glGenTextures(1, &gridTex);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_3D, gridTex);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_S, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_T, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_WRAP_R, GL_CLAMP);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_3D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexImage3D(GL_TEXTURE_3D, 0, GL_RGB32F, width, height, depth, 0, GL_RGB, GL_FLOAT, voxelData);

	//Sphere Packing
	glm::vec4 * prototypes = new glm::vec4[faces.size()];
	GLuint protoBuffer;
	glGenBuffers(1, &protoBuffer);
	unsigned int amountPrototypes = faces.size();
	//unsigned int amountPrototypes = 1;

	timer.Tick();

	//Use shader programm
	Shader& shader = getInstance().getPackingShader();
	shader.bind();
	shader.setUniform("gridTexture", (GLint)0);
	shader.setUniform("iterations", (GLuint)1000);
	shader.setUniform("invGridSize", glm::vec3(1.0f, 1.0f, 1.0f) / gridSize);
	shader.setUniform("gridMin", min);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, 0, protoBuffer);

	for (unsigned int i = 0; i < maxIterations; i++)
	{
		//Generate 1 Prototype per face
		for (unsigned int f = 0; f < amountPrototypes; f++)
		{
			auto face = faces[f];
			glm::vec3 barycentric = glm::vec3(random(), random(), random());
			barycentric /= barycentric.x + barycentric.y + barycentric.z;
			glm::vec3 p = barycentric.x * face.vertices[0].position + barycentric.y * face.vertices[1].position + barycentric.z * face.vertices[2].position;
			p += -face.faceNormal * glm::min(span.x, glm::min(span.y, span.z)) * 0.001f;
			prototypes[f] = glm::vec4(p, 0.0f);
		}

		//Fill buffer with prototypes
		if (i == 0)
		{
			glBufferStorage(GL_SHADER_STORAGE_BUFFER, amountPrototypes * sizeof(glm::vec4), prototypes, GL_DYNAMIC_STORAGE_BIT | GL_MAP_READ_BIT);
		}
		else
		{
			glBufferSubData(GL_SHADER_STORAGE_BUFFER, 0, amountPrototypes * sizeof(glm::vec4), prototypes);
		}

		shader.dispatch(amountPrototypes, 1, 1);

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);

		//Get Prototypes
		glm::vec4* buf = (glm::vec4*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
		std::vector<glm::vec4> potentialSpheres;
		for (unsigned int f = 0; f < amountPrototypes; f++)
		{
			potentialSpheres.push_back(buf[f]);
		}
		glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);

		//Insert new Spheres and update grid
		std::vector<glm::vec4> newSpheres;
		struct {
			bool operator()(glm::vec4 a, glm::vec4 b)
			{
				return a.w > b.w;
			}
		} compVec4W;
		std::sort(potentialSpheres.begin(), potentialSpheres.end(), compVec4W);
		for (unsigned int s = 0; s < potentialSpheres.size(); s++)
		{
			glm::vec4 sphere = potentialSpheres[s];
			if (isfinite(sphere.w) && !intersect(sphere, spheres))
			{
				newSpheres.push_back(sphere);
				spheres.push_back(sphere);
				//std::cout << "Add sphere at [" << sphere.x << "," << sphere.y << "," << sphere.z << "] with radius " << sphere.w << std::endl;
			}
			/*else if (!isfinite(sphere.w))
			{
			std::cout << "Culled inf sphere" << std::endl;
			}*/
		}

		for (unsigned int t = 0; t < SPHERE_PACKER_THREAD_COUNT; t++)
		{
			unsigned int dMin = (unsigned int)roundf((float)t * ((float)depth / (float)SPHERE_PACKER_THREAD_COUNT));
			unsigned int dMax = (unsigned int)roundf((float)(t + 1) * ((float)depth / (float)SPHERE_PACKER_THREAD_COUNT));
			pool.AddJob([dMin, dMax, height, width, gridSize, min, newSpheres, voxelData](){
				UpdateVoxelData(dMin, dMax, height, width, gridSize, min, newSpheres, voxelData);
			});
		}
		pool.WaitAll();

		glTexSubImage3D(GL_TEXTURE_3D, 0, 0, 0, 0, width, height, depth, GL_RGB, GL_FLOAT, voxelData);
		//std::cout << "Iteration " << i << " finished" << std::endl;
	}

	timer.Tick();
	std::cout << "Iterations finished in " << timer.LastFrameTime() << " seconds." << std::endl;

	delete[] prototypes;
	delete[] voxelData;

	glDeleteBuffers(1, &protoBuffer);
	glDeleteTextures(1, &gridTex);

	if (deleteInnerSpheres)
	{
		timer.Tick();
		bool* isOuterArray = new bool[spheres.size()];
		float fac = 0.5f;
		for (unsigned int i = 0; i < spheres.size(); i++)
		{
			isOuterArray[i] = false;
			pool.AddJob([i, fac, spheres, faces, isOuterArray](){
				searchForOuterSphere(i, fac, spheres, faces, isOuterArray);
			});
		}
		pool.WaitAll();
		for (int i = spheres.size() - 1; i >= 0; i--)
		{
			if (!isOuterArray[i])
			{
				std::cout << "Delete sphere idx = " << i << std::endl;
				spheres.erase(spheres.begin() + i);
			}
		}
		delete[] isOuterArray;
		timer.Tick();
		std::cout << "Delete inner spheres finished in " << timer.LastFrameTime() << " seconds." << std::endl;
	}

	return spheres;
}