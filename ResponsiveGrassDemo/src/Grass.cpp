/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "Grass.h"

#include <numeric>
#include <algorithm>
#include <functional>
#include <thread>
#include <queue>
#include <fstream>
#include "glm\gtc\matrix_transform.hpp"
#include "BoundingBox.h"
#include "OpenGLState.h"
#include "ThreadPool.h"

#define MAX_AMOUNT_INNER_SPHERES 150
#define MAX_AMOUNT_SPHERE_COLLIDER 50
#define OPTIMAL_TILE_FACTOR 10

#define PARTITIONING_BY_CLUSTERING

#define USE_MANHATTEN_DISTANCE false
#define EVALUATE_MSE

//*******************************************
//*********** Helper functions **************
//*******************************************
#pragma region Helper functions

glm::vec3 interpolate3G2(const glm::vec3& barycentric, const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3)
{
	return v1 * barycentric.x + v2 * barycentric.y + v3 * barycentric.z;
}

bool intersect(const BoundingBox::TransformedBox& box, const glm::vec4& sphere)
{
	glm::vec3 Bmin = box.location - (box.axis1 + box.axis2 + box.axis3);
	glm::vec3 Bmax = box.location + (box.axis1 + box.axis2 + box.axis3);
	float dmin = 0;
	for (unsigned int i = 0; i < 3; i++) {
		if (sphere[i] < Bmin[i])
		{
			float d = sphere[i] - Bmin[i];
			dmin += d*d;
		}
		else
		{
			if (sphere[i] > Bmax[i])
			{
				float d = sphere[i] - Bmax[i];
				dmin += d*d;
			}
		}
	}
	
	return dmin <= sphere.w*sphere.w;
}

bool intersect(const glm::vec4& sphere1, const glm::vec4& sphere2)
{
	glm::vec3 s1 = sphere1.xyz;
	glm::vec3 s2 = sphere2.xyz;
	return glm::distance(s1, s2) <= (sphere1.w + sphere2.w);
}

//Method for Quicksort
unsigned int partition(unsigned int index[], const float values[], const unsigned int low, const unsigned int high)
{
	unsigned int pIndex = low + rand() % ((high - low)+1);
	float pivot = values[index[pIndex]];
	unsigned int helpPivot = index[high];
	index[high] = index[pIndex];
	index[pIndex] = helpPivot;

	unsigned int i = low;
	for (unsigned int j = low; j < high; j++)
	{
		if (values[index[j]] <= pivot)
		{
			unsigned int help = index[i];
			index[i] = index[j];
			index[j] = help;
			i++;
		}
	}

	unsigned int help = index[i];
	index[i] = index[high];
	index[high] = help;

	return i;
}

//Recursive Quicksort method with multithreading
void multi_quicksort(unsigned int index[], const float values[], const unsigned int low, const unsigned int high, const unsigned int depth, const unsigned int maxDepth)
{
	if (low < high)
	{
		unsigned int p = partition(index, values, low, high);

		if (depth <= maxDepth)
		{
			if (p != low && p != high)
			{
				std::thread t1(multi_quicksort, index, values, low, p - 1, depth+1, maxDepth);
				std::thread t2(multi_quicksort, index, values, p + 1, high, depth + 1, maxDepth);
				t1.join();
				t2.join();
			}
			else if (p != low)
			{
				std::thread t1(multi_quicksort, index, values, low, p - 1, depth + 1, maxDepth);
				t1.join();
			}
			else if (p != high)
			{
				std::thread t2(multi_quicksort, index, values, p + 1, high, depth + 1, maxDepth);
				t2.join();
			}
		}
		else
		{
			if (p != low)
			{
				multi_quicksort(index, values, low, p - 1, depth, maxDepth);
			}
			if (p != high)
			{
				multi_quicksort(index, values, p + 1, high, depth, maxDepth);
			}
		}
	}
}

void SortVectors(std::vector<glm::vec4>& main, std::vector<glm::vec4>& other1, std::vector<glm::vec4>& other2, std::vector<glm::vec4>& other3, float values[], const unsigned int minIndexInclusive, const unsigned int maxIndexExclusive)
{
	unsigned int* index = new unsigned int[main.size()];

	for (unsigned int i = 0; i < main.size(); i++)
	{
		index[i] = i;
	}

	unsigned int maxDepth = 5; //TODO make smarter

	std::thread t(multi_quicksort, index, values, minIndexInclusive, maxIndexExclusive - 1, 0, maxDepth);
	t.join();

	std::vector<glm::vec4> main_local(main);
	std::vector<glm::vec4> other1_local(other1);
	std::vector<glm::vec4> other2_local(other2);
	std::vector<glm::vec4> other3_local(other3);
	float* values_local = new float[main.size()];
	std::copy(values, values+main.size(), values_local);

	for (unsigned int i = 0; i < main.size(); i++)
	{
		main[i] = main_local[index[i]];
		other1[i] = other1_local[index[i]];
		other2[i] = other2_local[index[i]];
		other3[i] = other3_local[index[i]];
		values[i] = values_local[index[i]];
	}

	delete[] values_local;
	delete[] index;
}

class Grid3D
{
	struct GridCell
	{
		float xMin, xMax, yMin, yMax, zMin, zMax;
		std::vector<glm::vec3> content;
	};

	std::vector<GridCell> cells;
	float xMin, xMax, yMin, yMax, zMin, zMax, gridSize;
	unsigned int amountCellsX, amountCellsY, amountCellsZ;

public:
	Grid3D(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax, float gridSize) : xMin(xMin), xMax(xMax), yMin(yMin), yMax(yMax), zMin(zMin), zMax(zMax), gridSize(gridSize)
	{
		amountCellsX = glm::max((unsigned int)glm::ceil((xMax - xMin) / gridSize), 1u);
		amountCellsY = glm::max((unsigned int)glm::ceil((yMax - yMin) / gridSize), 1u);
		amountCellsZ = glm::max((unsigned int)glm::ceil((zMax - zMin) / gridSize), 1u);

		cells.reserve(amountCellsX * amountCellsY * amountCellsZ);

		for (unsigned int z = 0; z < amountCellsZ; z++)
		{
			float curZMin = z * gridSize;
			float curZMax = glm::max((z + 1)*gridSize, zMax);
			for (unsigned int y = 0; y < amountCellsY; y++)
			{
				float curYMin = y * gridSize;
				float curYMax = glm::max((y + 1)*gridSize, yMax);
				for (unsigned int x = 0; x < amountCellsX; x++)
				{
					float curXMin = x * gridSize;
					float curXMax = glm::max((x + 1)*gridSize, xMax);

					GridCell cell;
					cell.xMin = curXMin; cell.xMax = curXMax;
					cell.yMin = curYMin; cell.yMax = curYMax;
					cell.zMin = curZMin; cell.zMax = curZMax;
					cells.push_back(cell);
				}
			}
		}
	}

	~Grid3D() {}

	unsigned int getIndex(unsigned int zBucket, unsigned int yBucket, unsigned int xBucket) const
	{
		return zBucket * (amountCellsX * amountCellsY) + yBucket * amountCellsX + xBucket;
	}

	void insert(const glm::vec3& position)
	{
		unsigned int xBucket = glm::min((unsigned int)glm::floor((position.x - xMin) / gridSize), amountCellsX - 1);
		unsigned int yBucket = glm::min((unsigned int)glm::floor((position.y - yMin) / gridSize), amountCellsY - 1);
		unsigned int zBucket = glm::min((unsigned int)glm::floor((position.z - zMin) / gridSize), amountCellsZ - 1);

		unsigned int index = getIndex(zBucket, yBucket, xBucket);

		cells[index].content.push_back(position);
	}

	std::vector<glm::vec3> getCandidates(const glm::vec3& position) const
	{
		unsigned int xBucket = glm::min((unsigned int)glm::floor((position.x - xMin) / gridSize), amountCellsX - 1);
		unsigned int yBucket = glm::min((unsigned int)glm::floor((position.y - yMin) / gridSize), amountCellsY - 1);
		unsigned int zBucket = glm::min((unsigned int)glm::floor((position.z - zMin) / gridSize), amountCellsZ - 1);

		std::vector<glm::vec3> ret;

		{
			unsigned int index = getIndex(zBucket, yBucket, xBucket);
			auto& c = cells[index].content;
			ret.reserve(ret.size() + c.size());
			ret.insert(ret.end(), c.begin(), c.end());
		}
		
		for (int x = -1; x <= 1; x++)
		{
			if (!(x == -1 && xBucket == 0) && !(x == 1 && xBucket == amountCellsX - 1))
			{
				for (int y = -1; y <= 1; y++)
				{
					if (!(y == -1 && yBucket == 0) && !(y == 1 && yBucket == amountCellsY - 1))
					{
						for (int z = -1; z <= 1; z++)
						{
							if (!(z == -1 && zBucket == 0) && !(z == 1 && zBucket == amountCellsZ - 1))
							{
								unsigned int index = getIndex(zBucket + z, yBucket + y, xBucket + x);
								auto& c = cells[index].content;
								if (!c.empty())
								{
									ret.reserve(ret.size() + c.size());
									ret.insert(ret.end(), c.begin(), c.end());
								}
							}
						}
					}
				}
			}
		}

		return ret;
	}
};

std::vector<std::vector<unsigned int>> factorize3(unsigned int n)
{
	std::vector<std::vector<unsigned int>> ret;

	float n_max = glm::ceil(glm::pow((float)n, 1.0f / 3.0f));
	for (unsigned int i = 1; i <= n_max; i++)
	{
		if (n % i == 0)
		{
			unsigned int m = n / i;
			float m_max = glm::ceil(glm::sqrt((float)m));
			for (unsigned int j = i; j <= m_max; j++)
			{
				if (m % j == 0)
				{
					std::vector<unsigned int> out;
					out.push_back(i);
					out.push_back(j);
					out.push_back(m / j);
					std::sort(out.begin(), out.end());
					ret.push_back(out);
				}
			}
		}
	}

	return ret;
}

float manDist(const glm::vec3& p1, const glm::vec3& p2)
{
	glm::vec3 dif = glm::abs(p1 - p2);
	return dif.x + dif.y + dif.z;
}

#pragma endregion

//*******************************************
//*************** Grass ********************
//*******************************************
#pragma region Grass

Shader * Grass::updateForceShader = 0;
Shader * Grass::updateVisibilityShader = 0;
Shader * Grass::copyBufferShader = 0;
Shader * Grass::drawShader = 0;
Texture2D * Grass::diffuseTexture = 0;
Texture2D * Grass::pressureMap = 0;
unsigned int Grass::maxAmountBlades = 0;
unsigned int Grass::pressureMapBlockSize = 0;
unsigned int Grass::amountGrassInstances = 0;

void Grass::UpdatePressureMap()
{
	unsigned int patchCount = GrassOvermind::getInstance().getAmountOfGrassPatches();

	pressureMapBlockSize = (unsigned int)ceilf(sqrtf((float)maxAmountBlades));
	unsigned int rowCount = (unsigned int)floorf(sqrtf((float)patchCount));
	unsigned int colCount = (unsigned int)ceilf((float)patchCount / rowCount);

	pressureMap->bind(0);
	pressureMap->BufferImage(colCount * pressureMapBlockSize, rowCount * pressureMapBlockSize, GL_BYTE, 0);
	glClearTexImage(pressureMap->Handle(), 0, GL_RGBA, GL_BYTE, 0);

	GrassOvermind::getInstance().NotifyPressureMapUpdated();
}

void Grass::NotifyPressureMapUpdated(unsigned int id, unsigned int count)
{
	unsigned int rowCount = (unsigned int)floorf(sqrtf((float)count));
	unsigned int colCount = (unsigned int)ceilf((float)count / rowCount);

	for (unsigned int i = 0; i < patches.size(); i++)
	{
		patches[i].pressureMapOffset = glm::ivec2((id + i) % colCount, (id + i) / colCount);
	}
}

Grass::Grass() : overmind(&GrassOvermind::getInstance()), patches(), localGravity(), wind(), modelMatrix(1.0f), altDiffuseTexture(0)
{
	localGravity.gravityVector = glm::vec4(0);
	localGravity.gravityPoint = glm::vec4(0);
	localGravity.gravityPointAlpha = 0.0f;

	amountGrassInstances++;

	overmind->addGrassInstance(*this);

	if (updateForceShader == 0)
	{
		Shader::queryBounds();
		std::vector<std::string> symbols;
		std::vector<std::string> replace;
		symbols.push_back("MAX_WORK_GROUP_SIZE_X");
		replace.push_back(std::to_string(Shader::max_work_group_size_X));
		symbols.push_back("POSITION_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::POSITION));
		symbols.push_back("V1_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::V1));
		symbols.push_back("V2_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::V2));
		symbols.push_back("DEBUG_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::DEBUGOUT));
		symbols.push_back("ATTR_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::ATTR));
		symbols.push_back("MAX_AMOUNT_SPHERE_COLLIDER");
		replace.push_back(std::to_string(MAX_AMOUNT_SPHERE_COLLIDER));
		updateForceShader = new Shader(SHADERPATH + "Grass/GrassUpdateForcesShader", symbols, replace);
	}

	if (updateVisibilityShader == 0)
	{
		std::vector<std::string> symbols;
		std::vector<std::string> replace;
		symbols.push_back("MAX_WORK_GROUP_SIZE_X");
		replace.push_back(std::to_string(Shader::max_work_group_size_X));
		symbols.push_back("MAX_AMOUNT_INNER_SPHERES");
		replace.push_back(std::to_string(MAX_AMOUNT_INNER_SPHERES));
		symbols.push_back("POSITION_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::POSITION));
		symbols.push_back("V1_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::V1));
		symbols.push_back("V2_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::V2));
		symbols.push_back("DEBUG_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::DEBUGOUT));
		symbols.push_back("ATTR_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::ATTR));
		symbols.push_back("INDEX_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::INDEX));
		symbols.push_back("ATOMIC_COUNTER_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::ATOMIC_COUNTER));
		updateVisibilityShader = new Shader(SHADERPATH + "Grass/GrassUpdateVisibilityShader", symbols, replace);
	}

	if (copyBufferShader == 0)
	{
		std::vector<std::string> symbols;
		std::vector<std::string> replace;
		copyBufferShader = new Shader(SHADERPATH + "Grass/GrassCopyBufferShader", symbols, replace);
	}

	if (drawShader == 0)
	{
		std::vector<std::string> symbols;
		std::vector<std::string> replace;
		symbols.push_back("POSITION_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::POSITION));
		symbols.push_back("V1_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::V1));
		symbols.push_back("V2_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::V2));
		symbols.push_back("DEBUG_LOCATION");
		replace.push_back(std::to_string(GrassPatch::GrassBufferEnum::DEBUGOUT));
		drawShader = new Shader(SHADERPATH + "Grass/GrassDrawShader", symbols, replace);
	}

	if (diffuseTexture == 0)
	{
		diffuseTexture = Texture2D::loadTextureFromFile(TEXTUREPATH + "Grass/GrassDiffuse.png", true, true, false, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR);
	}

	if (pressureMap == 0)
	{
		pressureMap = new Texture2D(GL_RGBA32F, GL_RGB, false, false, GL_CLAMP, GL_CLAMP, GL_NEAREST, GL_NEAREST);
	}
}

Grass::Grass(const std::vector<GrassCreateBladeParams>& params, std::vector<Geometry::TriangleFace>& faces) : Grass()
{
	Initialize(params, faces);
}

Grass::Grass(const Grass& other) : overmind(&GrassOvermind::getInstance()), patches(other.patches), modelMatrix(other.modelMatrix), boundingObject(other.boundingObject), 
	localGravity(other.localGravity), useLocalGravity(other.useLocalGravity), wind(other.wind), heightMap(other.heightMap), heightMapBounds(other.heightMapBounds), 
	parentObject(other.parentObject), altDiffuseTexture(other.altDiffuseTexture)
{
	amountGrassInstances++;

	overmind->addGrassInstance(*this);
	UpdatePressureMap();
}

Grass::~Grass()
{
	amountGrassInstances--;
	overmind->removeGrassInstance(this);
	if (amountGrassInstances == 0)
	{
		delete updateForceShader;
		delete updateVisibilityShader;
		delete drawShader;
	}
}

void Grass::Initialize(const std::vector<GrassCreateBladeParams>& params, std::vector<Geometry::TriangleFace>& faces)
{
	std::cout << "Initialize grass on " << faces.size() << " faces" << std::endl;

	for (unsigned int pI = 0; pI < params.size(); pI++)
	{
		GrassCreateBladeParams p = params[pI];

		switch (p.spacialDistribution)
		{
		case GrassSpacialDistribution::FACE_RANDOM:
			DistributeFaceRandom(p, faces);
			break;
		case GrassSpacialDistribution::FACE_AREA:
			DistributeFaceArea(p, faces);
			break;
		}
	}

	overmind->NotifyGrassInstanceUpdated();
	UpdatePressureMap();

	float xMin = FLT_MAX;
	float yMin = FLT_MAX;
	float zMin = FLT_MAX;
	float xMax = -FLT_MAX;
	float yMax = -FLT_MAX;
	float zMax = -FLT_MAX;
	for each(GrassPatchInfo p in patches)
	{
		if (p.bounds != 0)
		{
			if (p.bounds->xMin < xMin) { xMin = p.bounds->xMin;	}
			if (p.bounds->yMin < yMin) { yMin = p.bounds->yMin; }
			if (p.bounds->zMin < zMin) { zMin = p.bounds->zMin; }
			if (p.bounds->xMax > xMax) { xMax = p.bounds->xMax; }
			if (p.bounds->yMax > yMax) { yMax = p.bounds->yMax; }
			if (p.bounds->zMax > zMax) { zMax = p.bounds->zMax; }
		}
	}
	if (boundingObject != 0)
	{
		delete boundingObject;
		boundingObject = 0;
	}
	boundingObject = new BoundingBox(xMin, xMax, yMin, yMax, zMin, zMax);
}

void GenerateClusterBlades(const GrassCreateBladeParams& p, const float clusterDistance, unsigned int amountBlades, unsigned int amountCluster, 
	const std::vector<glm::vec3>& clusterPos, const std::vector<glm::vec3>& clusterUp, const std::vector<glm::vec3>& clusterTangent, 
	std::vector<glm::vec4>& bladePositions, std::vector<glm::vec4>& bladeV1, std::vector<glm::vec4>& bladeV2, std::vector<glm::vec4>& bladeAttr)
{
	float innerClusterDistance = clusterDistance * 0.1f;
	unsigned int amountBladesPerCluster = (unsigned int)glm::ceil((float)amountBlades / (float)amountCluster);
	std::cout << "Amount Blades Per Cluster " << amountBladesPerCluster << std::endl;
	unsigned int curAmountBlades = 0;

	for (unsigned int i = 0; i < amountCluster; i++)
	{
		unsigned int bladesToGenerate = 0;
		if (curAmountBlades + amountBladesPerCluster <= amountBlades)
		{
			if (amountBladesPerCluster % 2 != 0 && amountBladesPerCluster > 1)
			{
				bladesToGenerate = amountBladesPerCluster + ((rand() % (int)(amountBladesPerCluster - 1)) - (int)(amountBladesPerCluster / 2));
			}
			else
			{
				bladesToGenerate = amountBladesPerCluster + ((rand() % (int)(amountBladesPerCluster)) - (int)(amountBladesPerCluster / 2));
			}
		}
		else
		{
			bladesToGenerate = amountBlades - curAmountBlades;
		}

		if (bladesToGenerate > 0)
		{
			float curDir = random() * PI_F;
			float dirIncrease = 2.0f * PI_F / (float)bladesToGenerate;

			float clusterMinHeight = p.bladeMinHeight + random() * (p.bladeMaxHeight - p.bladeMinHeight);
			float clusterMaxHeight = p.bladeMinHeight + random() * (p.bladeMaxHeight - p.bladeMinHeight);
			if (clusterMaxHeight < clusterMinHeight)
			{
				float h = clusterMinHeight;
				clusterMinHeight = clusterMaxHeight;
				clusterMaxHeight = h;
			}

			float clusterMinWidth = p.bladeMinWidth + random() * (p.bladeMaxWidth - p.bladeMinWidth);
			float clusterMaxWidth = p.bladeMinWidth + random() * (p.bladeMaxWidth - p.bladeMinWidth);
			if (clusterMaxWidth < clusterMinWidth)
			{
				float h = clusterMinWidth;
				clusterMinWidth = clusterMaxWidth;
				clusterMaxWidth = h;
			}

			float clusterMinBend = p.bladeMinBend + random() * (p.bladeMaxBend - p.bladeMinBend);
			float clusterMaxBend = p.bladeMinBend + random() * (p.bladeMaxBend - p.bladeMinBend);
			if (clusterMaxBend < clusterMinBend)
			{
				float h = clusterMinBend;
				clusterMinBend = clusterMaxBend;
				clusterMaxBend = h;
			}

			for (unsigned int j = 0; j < bladesToGenerate; j++)
			{
				float interpolateFac = 0.5f;
				if (bladesToGenerate != 1)
				{
					interpolateFac = (float)j / (float)(bladesToGenerate - 1);
				}				

				glm::vec3 translateDir = glm::mat3(glm::rotate(glm::mat4(1.0f), curDir, clusterUp[i])) * clusterTangent[i];
				glm::vec3 translateVec = translateDir * innerClusterDistance * glm::max(glm::abs(glm::sin(curDir)),0.1f);

				glm::vec3 bladePos = clusterPos[i] + translateVec;
				float bladeAlpha = (curDir - 0.5f * PI_F < 0) ? (curDir + 1.5f * PI_F) : curDir - 0.5f * PI_F;
				if (bladeAlpha > 2.0f * PI_F)
				{
					bladeAlpha -= 2.0f * PI_F;
				}
				if (bladeAlpha > 2.0f * PI_F)
				{
					std::cout << "DistributeFaceRandom: THERE IS SOMETHING FISHY HERE " << bladeAlpha << " - " << curDir << std::endl;
				}

				float bladeHeight = clusterMinHeight + interpolateFac * (clusterMaxHeight - clusterMinHeight);
				float bladeWidth = clusterMinWidth + interpolateFac * (clusterMaxWidth - clusterMinWidth);
				float bladeBend = clusterMinBend + interpolateFac * (clusterMaxBend - clusterMinBend);

				bladePositions.push_back(glm::vec4(bladePos, bladeAlpha));
				bladeAttr.push_back(glm::vec4(clusterUp[i], bladeBend));
				bladeV1.push_back(glm::vec4(bladePos + clusterUp[i] * bladeHeight, bladeHeight));
				bladeV2.push_back(glm::vec4(bladePos + clusterUp[i] * bladeHeight, bladeWidth));

				curAmountBlades++;

				curDir += dirIncrease;
			}
		}
	}
}

void Grass::DistributeFaceRandom(const GrassCreateBladeParams& p, std::vector <Geometry::TriangleFace>& faces)
{
	std::vector<glm::vec4> bladePositions;
	std::vector<glm::vec4> bladeAttr;
	std::vector<glm::vec4> bladeV1;
	std::vector<glm::vec4> bladeV2;

	float sumArea = 0;
	float xMin = FLT_MAX, xMax = -FLT_MAX, yMin = FLT_MAX, yMax = -FLT_MAX, zMin = FLT_MAX, zMax = -FLT_MAX;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		sumArea += faces[i].area;

		for (unsigned int j = 0; j < 3; j++)
		{
			auto v = faces[i].vertices[j].position;
			xMin = glm::min(xMin, v.x);
			yMin = glm::min(yMin, v.y);
			zMin = glm::min(zMin, v.z);
			xMax = glm::max(xMax, v.x);
			yMax = glm::max(yMax, v.y);
			zMax = glm::max(zMax, v.z);
		}
	}

	unsigned int amountBlades = glm::max((unsigned int)(sumArea * p.density), 1u);
	std::cout << "Random-face distribution generates " << amountBlades << " blades" << std::endl;
	bladePositions.reserve(amountBlades);
	bladeAttr.reserve(amountBlades);
	bladeV1.reserve(amountBlades);
	bladeV2.reserve(amountBlades);

	switch (p.distribution)
	{
	case GrassDistribution::UNIFORM:
		{
			for (unsigned int i = 0; i < amountBlades; i++)
			{
				Geometry::TriangleFace face = faces[rand() % faces.size()];

				glm::vec3 barycentric = glm::vec3(random(), random(), random());
				barycentric /= barycentric.x + barycentric.y + barycentric.z;

				glm::vec3 bladePos = interpolate3G2(barycentric, face.vertices[0].position, face.vertices[1].position, face.vertices[2].position);
				glm::vec3 bladeUp = glm::normalize((face.faceNormal + interpolate3G2(barycentric, face.vertices[0].normal, face.vertices[1].normal, face.vertices[2].normal)) * 0.5f);
			
				float dirAlpha = random() * PI_F * 2.0f;
				float height = p.bladeMinHeight + random() * (p.bladeMaxHeight - p.bladeMinHeight);
				float width = p.bladeMinWidth + random() * (p.bladeMaxWidth - p.bladeMinWidth);
				float bend = p.bladeMinBend + random() * (p.bladeMaxBend - p.bladeMinBend);

				bladePositions.push_back(glm::vec4(bladePos, dirAlpha));
				bladeAttr.push_back(glm::vec4(bladeUp, bend));
				bladeV1.push_back(glm::vec4(bladePos + bladeUp * height, height));
				bladeV2.push_back(glm::vec4(bladePos + bladeUp * height, width));
			}
		}
		break;
	case GrassDistribution::CLUSTER:
		{
			std::vector<glm::vec3> clusterPos;
			std::vector<glm::vec3> clusterUp;
			std::vector<glm::vec3> clusterTangent;

			unsigned int amountCluster = glm::max((unsigned int)(amountBlades * p.clusterPercentage), 1u);

			clusterPos.reserve(amountCluster);
			clusterUp.reserve(amountCluster);
			clusterTangent.reserve(amountCluster);

			float clusterDistance = (glm::sqrt(sumArea) / glm::sqrt((float)amountCluster));
			float curClusterDistance = clusterDistance;

			Grid3D grid(xMin, xMax, yMin, yMax, zMin, zMax, clusterDistance);

			unsigned int curAmountCluster = 0;
			unsigned int invalidCount = 0;
			unsigned int curInvalidCount = 0;
			while (curAmountCluster < amountCluster)
			{
				if (curInvalidCount > amountCluster / 10)
				{
					curClusterDistance *= 0.9f;
					curInvalidCount = 0;
				}

				Geometry::TriangleFace face = faces[rand() % faces.size()];

				glm::vec3 barycentric = glm::vec3(random(), random(), random());
				barycentric /= barycentric.x + barycentric.y + barycentric.z;

				glm::vec3 pos = interpolate3G2(barycentric, face.vertices[0].position, face.vertices[1].position, face.vertices[2].position);
				glm::vec3 up = glm::normalize((face.faceNormal + interpolate3G2(barycentric, face.vertices[0].normal, face.vertices[1].normal, face.vertices[2].normal)) * 0.5f);
				glm::vec3 tan = glm::normalize(face.vertices[1].position - face.vertices[0].position);

				//Check for distances
				bool valid = true;
				auto listAdjacentClusters = grid.getCandidates(pos);
				for each (glm::vec3 p in listAdjacentClusters)
				{
					if (glm::distance(p, pos) < curClusterDistance)
					{
						valid = false;
						break;
					}
				}

				if (valid)
				{
					curAmountCluster++;
					clusterPos.push_back(pos);
					clusterUp.push_back(up);
					clusterTangent.push_back(tan);
					grid.insert(pos);
				}
				else
				{
					invalidCount++;
					curInvalidCount++;
				}
			}

			GenerateClusterBlades(p, clusterDistance, amountBlades, amountCluster, clusterPos, clusterUp, clusterTangent, bladePositions, bladeV1, bladeV2, bladeAttr);
		}
		break;
	}

	GeneratePatches(bladePositions, bladeV1, bladeV2, bladeAttr, p.shape, p.tessellationProps);
}

void Grass::DistributeFaceArea(const GrassCreateBladeParams& p, std::vector <Geometry::TriangleFace>& faces)
{
	std::vector<glm::vec4> bladePositions;
	std::vector<glm::vec4> bladeAttr;
	std::vector<glm::vec4> bladeV1;
	std::vector<glm::vec4> bladeV2;

	float sumArea = 0;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		sumArea += faces[i].area;
	}

	unsigned int amountBlades = glm::max((unsigned int)(sumArea * p.density), 1u);
	std::cout << "Area-face distribution generates " << amountBlades << " blades" << std::endl;
	bladePositions.reserve(amountBlades);
	bladeAttr.reserve(amountBlades);
	bladeV1.reserve(amountBlades);
	bladeV2.reserve(amountBlades);

	switch (p.distribution)
	{
	case GrassDistribution::UNIFORM:
		{
			float sumCurArea = 0.0f;
			unsigned int curAmountBlades = 0;
			std::vector<unsigned int> facesWithoutBlades;
			for (unsigned int i = 0; i < faces.size(); i++)
			{
				sumCurArea += faces[i].area;
				float curBlades = sumCurArea * p.density;
				if (curBlades >= 1.0f)
				{
					for (unsigned int j = 0; j < (unsigned int)curBlades; j++)
					{
						Geometry::TriangleFace face = faces[i];

						glm::vec3 barycentric = glm::vec3(random(), random(), random());
						barycentric /= barycentric.x + barycentric.y + barycentric.z;

						glm::vec3 bladePos = interpolate3G2(barycentric, face.vertices[0].position, face.vertices[1].position, face.vertices[2].position);
						glm::vec3 bladeUp = glm::normalize((face.faceNormal + interpolate3G2(barycentric, face.vertices[0].normal, face.vertices[1].normal, face.vertices[2].normal)) * 0.5f);

						float dirAlpha = random() * PI_F * 2.0f;
						float height = p.bladeMinHeight + random() * (p.bladeMaxHeight - p.bladeMinHeight);
						float width = p.bladeMinWidth + random() * (p.bladeMaxWidth - p.bladeMinWidth);
						float bend = p.bladeMinBend + random() * (p.bladeMaxBend - p.bladeMinBend);

						bladePositions.push_back(glm::vec4(bladePos, dirAlpha));
						bladeAttr.push_back(glm::vec4(bladeUp, bend));
						bladeV1.push_back(glm::vec4(bladePos + bladeUp * height, height));
						bladeV2.push_back(glm::vec4(bladePos + bladeUp * height, width));

						curAmountBlades++;
					}
					sumCurArea -= (float)((unsigned int)curBlades) / p.density;
				}
				else
				{
					facesWithoutBlades.push_back(i);
				}
			}
			//Distribute if there are additional blades
			for (unsigned int i = curAmountBlades; i < amountBlades; i++)
			{
				unsigned int faceIndex = 0;
				if (facesWithoutBlades.size() > 0)
				{
					unsigned int fwobIndex = rand() % facesWithoutBlades.size();
					faceIndex = facesWithoutBlades[fwobIndex];
					facesWithoutBlades.erase(facesWithoutBlades.begin() + fwobIndex);
				}
				else
				{
					faceIndex = rand() % faces.size();
				}
				Geometry::TriangleFace face = faces[faceIndex];

				glm::vec3 barycentric = glm::vec3(random(), random(), random());
				barycentric /= barycentric.x + barycentric.y + barycentric.z;

				glm::vec3 bladePos = interpolate3G2(barycentric, face.vertices[0].position, face.vertices[1].position, face.vertices[2].position);
				glm::vec3 bladeUp = glm::normalize((face.faceNormal + interpolate3G2(barycentric, face.vertices[0].normal, face.vertices[1].normal, face.vertices[2].normal)) * 0.5f);

				float dirAlpha = random() * PI_F * 2.0f;
				float height = p.bladeMinHeight + random() * (p.bladeMaxHeight - p.bladeMinHeight);
				float width = p.bladeMinWidth + random() * (p.bladeMaxWidth - p.bladeMinWidth);
				float bend = p.bladeMinBend + random() * (p.bladeMaxBend - p.bladeMinBend);

				bladePositions.push_back(glm::vec4(bladePos, dirAlpha));
				bladeAttr.push_back(glm::vec4(bladeUp, bend));
				bladeV1.push_back(glm::vec4(bladePos + bladeUp * height, height));
				bladeV2.push_back(glm::vec4(bladePos + bladeUp * height, width));
			}
		}
		break;
	case GrassDistribution::CLUSTER:
		{
			float xMin = FLT_MAX, xMax = -FLT_MAX, yMin = FLT_MAX, yMax = -FLT_MAX, zMin = FLT_MAX, zMax = -FLT_MAX;
			for (unsigned int i = 0; i < faces.size(); i++)
			{
				Geometry::TriangleFace f1 = faces[i];

				for (unsigned int j = 0; j < 3; j++)
				{
					auto v = f1.vertices[j].position;
					xMin = glm::min(xMin, v.x);
					yMin = glm::min(yMin, v.y);
					zMin = glm::min(zMin, v.z);
					xMax = glm::max(xMax, v.x);
					yMax = glm::max(yMax, v.y);
					zMax = glm::max(zMax, v.z);
				}
			}

			std::vector<glm::vec3> clusterPos;
			std::vector<glm::vec3> clusterUp;
			std::vector<glm::vec3> clusterTangent;

			unsigned int amountCluster = glm::max((unsigned int)(amountBlades * p.clusterPercentage), 1u);

			clusterPos.reserve(amountCluster);
			clusterUp.reserve(amountCluster);
			clusterTangent.reserve(amountCluster);

			float clusterDistance = (glm::sqrt(sumArea) / glm::sqrt((float)amountCluster));
			float curClusterDistance = clusterDistance;

			Grid3D grid(xMin, xMax, yMin, yMax, zMin, zMax, clusterDistance);

			unsigned int curAmountCluster = 0;
			unsigned int invalidCount = 0;
			unsigned int curFace = 0;
			unsigned int faceIter = 0;
			unsigned int curFaceIter = 0;
			std::vector<unsigned int> facesWithoutCluster;
			unsigned int * clusterCentersInFace = new unsigned int[faces.size()];
			std::fill(clusterCentersInFace, clusterCentersInFace + faces.size(), 0u);
			while (curAmountCluster < amountCluster)
			{
				if (curFaceIter == 1)
				{
					curClusterDistance *= 0.90f;
					curFaceIter = 0;
				}

				if (faces[curFace].area > clusterDistance)
				{
					//try to generate cluster center
					//if generated try to distribute other cluster
					Geometry::TriangleFace f = faces[curFace];
					unsigned int clusterCentersToCreate = 1;
					float diag = glm::sqrt(2.0f * f.area);
					if (diag > curClusterDistance)
					{
						clusterCentersToCreate = (unsigned int)(glm::ceil(diag / curClusterDistance));
					}
					clusterCentersToCreate = glm::max(clusterCentersToCreate - clusterCentersInFace[curFace], 0u);

					for (unsigned int i = 0; i < clusterCentersToCreate; i++)
					{
						if (curAmountCluster < amountCluster)
						{
							glm::vec3 barycentric = glm::vec3(random(), random(), random());
							barycentric /= barycentric.x + barycentric.y + barycentric.z;

							glm::vec3 pos = interpolate3G2(barycentric, f.vertices[0].position, f.vertices[1].position, f.vertices[2].position);
							glm::vec3 up = glm::normalize((f.faceNormal + interpolate3G2(barycentric, f.vertices[0].normal, f.vertices[1].normal, f.vertices[2].normal)) * 0.5f);
							glm::vec3 tan = glm::normalize(f.vertices[1].position - f.vertices[0].position);

							bool valid = true;
							auto listAdjacentClusters = grid.getCandidates(pos);
							for each (glm::vec3 p in listAdjacentClusters)
							{
								if (glm::distance(p, pos) < curClusterDistance)
								{
									valid = false;
									break;
								}
							}

							if (valid)
							{
								curAmountCluster++;
								clusterPos.push_back(pos);
								clusterUp.push_back(up);
								clusterTangent.push_back(tan);
								grid.insert(pos);

								clusterCentersInFace[curFace]++;
							}
							else
							{
								invalidCount++;
							}
						}
						else
						{
							break;
						}
					}
				}
				else
				{
					facesWithoutCluster.push_back(curFace);
				}
				curFace++;
				if (curFace == faces.size())
				{
					std::cout << "One iteration finished - Current amount of clusters: " << curAmountCluster << " out of " << amountCluster << std::endl;
					faceIter++;
					curFaceIter++;
					curFace = 0;
					//Process Faces without Clusters
					for (unsigned int i = 0; i < facesWithoutCluster.size(); i++)
					{
						//Only process if there is really no cluster in this face (could have been distributed from nearby faces)
						if (clusterCentersInFace[facesWithoutCluster[i]] == 0)
						{
							if (curAmountCluster < amountCluster)
							{
								Geometry::TriangleFace f = faces[facesWithoutCluster[i]];

								glm::vec3 barycentric = glm::vec3(random(), random(), random());
								barycentric /= barycentric.x + barycentric.y + barycentric.z;

								glm::vec3 pos = interpolate3G2(barycentric, f.vertices[0].position, f.vertices[1].position, f.vertices[2].position);
								glm::vec3 up = glm::normalize((f.faceNormal + interpolate3G2(barycentric, f.vertices[0].normal, f.vertices[1].normal, f.vertices[2].normal)) * 0.5f);
								glm::vec3 tan = glm::normalize(f.vertices[1].position - f.vertices[0].position);

								bool valid = true;
								auto listAdjacentClusters = grid.getCandidates(pos);
								for each (glm::vec3 p in listAdjacentClusters)
								{
									if (glm::distance(p, pos) < curClusterDistance)
									{
										valid = false;
										break;
									}
								}

								if (valid)
								{
									curAmountCluster++;
									clusterPos.push_back(pos);
									clusterUp.push_back(up);
									clusterTangent.push_back(tan);
									grid.insert(pos);

									clusterCentersInFace[curFace]++;
								}
								else
								{
									invalidCount++;
								}
							}
							else
							{
								break;
							}
						}
					}
				}
			}

			GenerateClusterBlades(p, clusterDistance, amountBlades, amountCluster, clusterPos, clusterUp, clusterTangent, bladePositions, bladeV1, bladeV2, bladeAttr);

			delete[] clusterCentersInFace;
		}
		break;
	}

	GeneratePatches(bladePositions, bladeV1, bladeV2, bladeAttr, p.shape, p.tessellationProps);
}

#ifdef PARTITIONING_BY_CLUSTERING
struct {
	bool operator()(std::pair<unsigned int, float>& a, std::pair<unsigned int, float>& b)
	{
		return a.second < b.second;
	}
} sortingFunc;

void ProcessCluster(const unsigned int t, const unsigned int amountTiles, const unsigned int bladesPerTile, const bool* clusterSwapped, const std::vector<std::vector<unsigned int>>& clusterIndices, const std::vector<glm::vec4>& bladePositions, const float* clusterCounts, glm::vec3* clusterMeans, glm::vec3** swapCandidates, float** swapCandidatesDist, unsigned int** swapCandidatesIdx)
{
	const std::vector<unsigned int>& ind = clusterIndices[t];
	if (clusterSwapped[t])
	{
		clusterMeans[t] = glm::vec3(0.0f);
		for (unsigned int i = 0; i < ind.size(); i++)
		{
			clusterMeans[t] += glm::vec3(bladePositions[ind[i]].xyz) / clusterCounts[t];
		}
	}

	std::vector<std::pair<unsigned int, float>> swapCandidateHelper;
	swapCandidateHelper.reserve(bladesPerTile + 1);
	for (unsigned int s = 0; s < amountTiles; s++)
	{
		if (clusterSwapped[t] || clusterSwapped[s])
		{
			swapCandidateHelper.clear();
			for (unsigned int i = 0; i < ind.size(); i++)
			{
				if (USE_MANHATTEN_DISTANCE)
				{
					float d1 = manDist(bladePositions[ind[i]].xyz, clusterMeans[s]);
					float d2 = manDist(bladePositions[ind[i]].xyz, clusterMeans[t]);
					float dist = d1*d1 - d2*d2;
					swapCandidateHelper.push_back(std::pair<unsigned int, float>(i, dist));
				}
				else
				{
					glm::vec3 vec = bladePositions[ind[i]].xyz - clusterMeans[s];
					glm::vec3 vec2 = bladePositions[ind[i]].xyz - clusterMeans[t];
					float dist = glm::dot(vec, vec) - glm::dot(vec2, vec2);
					swapCandidateHelper.push_back(std::pair<unsigned int, float>(i, dist));
				}
			}
			std::sort(swapCandidateHelper.begin(), swapCandidateHelper.end(), sortingFunc);

			swapCandidatesIdx[t][s] = ind[swapCandidateHelper[0].first];
			swapCandidates[t][s] = bladePositions[ind[swapCandidateHelper[0].first]].xyz;
			if (USE_MANHATTEN_DISTANCE)
			{
				float d = manDist(bladePositions[ind[swapCandidateHelper[0].first]].xyz, clusterMeans[t]);
				swapCandidatesDist[t][s] = d*d;
			}
			else
			{
				glm::vec3 vec = bladePositions[ind[swapCandidateHelper[0].first]].xyz - clusterMeans[t];
				swapCandidatesDist[t][s] = glm::dot(vec, vec);
			}
		}
	}
}
#endif

void Grass::GeneratePatches(std::vector<glm::vec4>& bladePositions, std::vector<glm::vec4>& bladeV1, std::vector<glm::vec4>& bladeV2, std::vector<glm::vec4>& bladeAttr, const BladeShape shape, const glm::vec4& tessellationProps)
{
	unsigned int amountBlades = bladePositions.size();

	if (amountBlades != bladeV1.size() || amountBlades != bladeV2.size() || amountBlades != bladeAttr.size())
	{
		std::cout << "GeneratePatches: THERE IS SOMETHING FISHY HERE" << std::endl;
	}

	float xMin = FLT_MAX;
	float yMin = FLT_MAX;
	float zMin = FLT_MAX;
	float xMax = FLT_MIN;
	float yMax = FLT_MIN;
	float zMax = FLT_MIN;
	for (unsigned int i = 0; i < bladePositions.size(); i++)
	{
		glm::vec3 pos = bladePositions[i].xyz;
		float height = bladeV1[i].w;

		glm::vec3 min = pos - height;
		glm::vec3 max = pos + height;

		if (min.x < xMin) {	xMin = min.x; }
		if (min.y < yMin) { yMin = min.y; }
		if (min.z < zMin) { zMin = min.z; }
		if (max.x > xMax) {	xMax = max.x; }
		if (max.y > yMax) { yMax = max.y; }
		if (max.z > zMax) { zMax = max.z; }
	}

	//TILING
	if (amountBlades > maxAmountBlades)
	{
		if (amountBlades >= (unsigned int)Shader::max_work_group_size_X * (unsigned int)OPTIMAL_TILE_FACTOR)
		{
			//Make tiles
			unsigned int amountTiles = (unsigned int)glm::ceil(amountBlades / ((float)Shader::max_work_group_size_X * (float)OPTIMAL_TILE_FACTOR));
			std::cout << "Amount blades " << amountBlades << " makes " << amountTiles << " tiles." << std::endl;
			unsigned int bladesPerTile = amountBlades / amountTiles;

#ifdef EVALUATE_MSE
			float meanSquaredError = 0.0f;
#endif

#ifdef PARTITIONING_BY_CLUSTERING
			//Idee von http://statistical-research.com/spatial-clustering-with-equal-sizes/
			int* clusterId = new int[amountBlades];
			std::fill_n(clusterId, amountBlades, -1);

			//Assign initial clusters
			std::vector<std::pair<unsigned int, float>> sortedCandidates;
			sortedCandidates.reserve(amountBlades);
			float xRange = xMax - xMin;
			float yRange = yMax - yMin;
			float zRange = zMax - zMin;
			for (unsigned int i = 0; i < amountBlades; i++)
			{
				if (xRange >= yRange && xRange >= zRange)
					sortedCandidates.push_back(std::pair<unsigned int, float>(i, bladePositions[i].x));
				else if (yRange >= xRange && yRange >= zRange)
					sortedCandidates.push_back(std::pair<unsigned int, float>(i, bladePositions[i].y));
				else 
					sortedCandidates.push_back(std::pair<unsigned int, float>(i, bladePositions[i].z));
			}
			std::sort(sortedCandidates.begin(), sortedCandidates.end(), sortingFunc);
			
			int currentCluster = 0;
			std::vector<std::pair<unsigned int, float>> clusterCandidates;
			clusterCandidates.reserve(amountBlades - 1);
			glm::vec3* clusterMeans = new glm::vec3[amountTiles];
			for (unsigned int i = 0; i < amountBlades && (unsigned int)currentCluster < amountTiles; i++)
			{
				//if (clusterId[i] == -1)
				if (clusterId[sortedCandidates[i].first] == -1)
				{
					//clusterId[i] = currentCluster;
					clusterId[sortedCandidates[i].first] = currentCluster;
					//clusterMeans[currentCluster] += glm::vec3(bladePositions[i].xyz) / (float)(bladesPerTile - 1);
					clusterMeans[currentCluster] += glm::vec3(bladePositions[sortedCandidates[i].first].xyz) / (float)(bladesPerTile - 1);
					clusterCandidates.clear();
					for (unsigned int j = 0; j < amountBlades; j++)
					{
						//if (i != j && clusterId[j] == -1)
						if (i != j && clusterId[sortedCandidates[j].first] == -1)
						{
							if (USE_MANHATTEN_DISTANCE)
							{
								//clusterCandidates.push_back(std::pair<unsigned int, float>(j, manDist(bladePositions[j].xyz, bladePositions[i].xyz)));
								float d = manDist(bladePositions[sortedCandidates[j].first].xyz, bladePositions[sortedCandidates[i].first].xyz);
								clusterCandidates.push_back(std::pair<unsigned int, float>(j, d*d));
							}
							else
							{
								//glm::vec3 vec = bladePositions[j].xyz - bladePositions[i].xyz;
								glm::vec3 vec = bladePositions[sortedCandidates[j].first].xyz - bladePositions[sortedCandidates[i].first].xyz;
								clusterCandidates.push_back(std::pair<unsigned int, float>(j, glm::dot(vec,vec)));
							}
						}
					}
					std::sort(clusterCandidates.begin(), clusterCandidates.end(), sortingFunc);


					for (unsigned int c = 0; c < bladesPerTile-1; c++)
					{
						//clusterId[clusterCandidates[c].first] = currentCluster;
						clusterId[sortedCandidates[clusterCandidates[c].first].first] = currentCluster;
					}
					currentCluster++;
					//std::cout << "Cluster " << currentCluster << " initially finished" << std::endl;
				}
			}

			for (unsigned int rest = 0; rest < amountBlades; rest++)
			{
				if (clusterId[rest] == -1)
				{
					unsigned int minCluster;
					float minDist = FLT_MAX;
					for (unsigned int i = 0; i < amountTiles; i++)
					{
						float dist;
						if (USE_MANHATTEN_DISTANCE)
						{
							dist = manDist(clusterMeans[i], bladePositions[rest].xyz);
						}
						else
						{
							glm::vec3 vec = clusterMeans[i] - bladePositions[rest].xyz;
							dist = glm::dot(vec, vec);
						}
						if (dist < minDist)
						{
							minDist = dist;
							minCluster = i;
						}
					}
					clusterId[rest] = minCluster;
				}
			}

			//Swap between clusters to make the overall result better
			glm::vec3** swapCandidates = new glm::vec3*[amountTiles];
			float** swapCandidatesDist = new float*[amountTiles];
			unsigned int** swapCandidatesIdx = new unsigned int*[amountTiles];
			float* clusterCounts = new float[amountTiles];
			bool* clusterSwapped = new bool[amountTiles];
			std::fill_n(clusterSwapped, amountTiles, true);
			std::vector<std::vector<unsigned int>> clusterIndices;
			clusterIndices.reserve(amountTiles);
			for (unsigned int i = 0; i < amountTiles; i++)
			{
				clusterIndices.push_back(std::vector<unsigned int>());
				clusterIndices[i].reserve(bladesPerTile + 1);
				swapCandidates[i] = new glm::vec3[amountTiles];
				swapCandidatesDist[i] = new float[amountTiles];
				swapCandidatesIdx[i] = new unsigned int[amountTiles];
			}
			for (unsigned int i = 0; i < amountBlades; i++)
			{
				clusterCounts[clusterId[i]]++;
				clusterIndices[clusterId[i]].push_back(i);
			}

			std::ofstream debug(GENERATEDFILESPATH + "Debug.txt");

			unsigned int iteration = 0;
			bool clusterWasSwapped = true;
			std::vector<std::pair<unsigned int, float>> swapCandidateHelper;
			swapCandidateHelper.reserve(bladesPerTile + 1);
			ThreadPool pool = ThreadPool(8);
			Clock whileTimeCount;
			whileTimeCount.Tick();
			while (iteration < 0 && clusterWasSwapped)
			//while (clusterWasSwapped)
			{
				clusterWasSwapped = false;

				//Calculate mean and swap candidate if cluster was swapped
				for (unsigned int t = 0; t < amountTiles; t++)
				{
					pool.AddJob([t, amountTiles, bladesPerTile, clusterSwapped, clusterIndices, bladePositions, clusterCounts, clusterMeans, swapCandidates, swapCandidatesDist, swapCandidatesIdx](){
						ProcessCluster(t, amountTiles, bladesPerTile, clusterSwapped, clusterIndices, bladePositions, clusterCounts, clusterMeans, swapCandidates, swapCandidatesDist, swapCandidatesIdx);
					});
				}
				pool.WaitAll();

				std::fill_n(clusterSwapped, amountTiles, false);

				unsigned int swapCount = 0;
				//Try to swap between clusters
				for (unsigned int i = 0; i < amountTiles - 1; i++)
				{
					if (!clusterSwapped[i])
					{
						for (unsigned int j = i + 1; j < amountTiles; j++)
						{
							if (!clusterSwapped[j])
							{
								const glm::vec3 sc1 = swapCandidates[i][j];
								const glm::vec3 cc1 = clusterMeans[i];
								const float scd1 = swapCandidatesDist[i][j];
								const glm::vec3 sc2 = swapCandidates[j][i];
								const glm::vec3 cc2 = clusterMeans[j];
								const float scd2 = swapCandidatesDist[j][i];
								float swappedValue;
								if (USE_MANHATTEN_DISTANCE)
								{
									float d1 = manDist(sc1, cc2);
									float d2 = manDist(sc2, cc1);
									swappedValue = d1*d1 + d2*d2;
								}
								else
								{
									const glm::vec3 vec1 = sc1 - cc2;
									const glm::vec3 vec2 = sc2 - cc1;
									swappedValue = glm::dot(vec1, vec1) + glm::dot(vec2, vec2);
								}
								if (swappedValue < scd1 + scd2)
								{
									//std::cout << "Swap between Cluster " << i << " and cluster " << j << ": benefit=" << std::to_string(scd1 + scd2 - swappedValue) << " Candidate i=[" << std::to_string(swapCandidates[i][j].x) << ", " << std::to_string(swapCandidates[i][j].y) << ", " << std::to_string(swapCandidates[i][j].z) << "] Candidate j=[" << std::to_string(swapCandidates[j][i].x) << ", " << std::to_string(swapCandidates[j][i].y) << ", " << std::to_string(swapCandidates[j][i].z) << "] " << std::endl;
									//debug << "Swap between Cluster " << i << " and cluster " << j << ": benefit=" << std::to_string(scd1 + scd2 - swappedValue) << " Candidate i=[" << std::to_string(swapCandidates[i][j].x) << ", " << std::to_string(swapCandidates[i][j].y) << ", " << std::to_string(swapCandidates[i][j].z) << "] Candidate j=[" << std::to_string(swapCandidates[j][i].x) << ", " << std::to_string(swapCandidates[j][i].y) << ", " << std::to_string(swapCandidates[j][i].z) << "] " << std::endl;

									//swap
									const unsigned int idxi = swapCandidatesIdx[i][j];
									const unsigned int idxj = swapCandidatesIdx[j][i];
									clusterId[idxi] = j;
									clusterId[idxj] = i;

									clusterIndices[i].erase(std::remove(clusterIndices[i].begin(), clusterIndices[i].end(), idxi), clusterIndices[i].end());
									clusterIndices[j].erase(std::remove(clusterIndices[j].begin(), clusterIndices[j].end(), idxj), clusterIndices[j].end());
									clusterIndices[i].push_back(idxj);
									clusterIndices[j].push_back(idxi);

									clusterSwapped[i] = true;
									clusterSwapped[j] = true;
									clusterWasSwapped = true;
									swapCount++;
									break;
								}
								else
								{
									//debug << "No Swap between Cluster " << i << " and cluster " << j << ": benefit=" << std::to_string(scd1 + scd2 - swappedValue) << " Candidate i=[" << std::to_string(swapCandidates[i][j].x) << ", " << std::to_string(swapCandidates[i][j].y) << ", " << std::to_string(swapCandidates[i][j].z) << "] Candidate j=[" << std::to_string(swapCandidates[j][i].x) << ", " << std::to_string(swapCandidates[j][i].y) << ", " << std::to_string(swapCandidates[j][i].z) << "] " << std::endl;
								}
							}
						}
					}
				}

				iteration++;
				std::cout << "Iteration " << iteration << " finished. There " << (clusterWasSwapped ? "were " + std::to_string(swapCount) + " swaps." : " was no swap.") << std::endl;
				//debug << "Iteration " << iteration << " finished. There was " << (clusterWasSwapped ? "a " : "no ") << "swap." << std::endl;
			}
			whileTimeCount.Tick();
			std::cout << "Finished after " << iteration << " iterations in " << std::to_string(whileTimeCount.LastFrameTime()) << "seconds." << std::endl;

			delete[] clusterMeans;
			for (unsigned int i = 0; i < amountTiles; i++)
			{
				delete[] swapCandidates[i];
				delete[] swapCandidatesDist[i];
			}
			delete[] swapCandidates;
			delete[] clusterCounts;
			delete[] clusterSwapped;
			delete[] swapCandidatesDist;
			delete[] swapCandidatesIdx;
#endif

			std::cout << "Amount Tiles: " << amountTiles << std::endl;

			for (unsigned int i = 0; i < amountTiles; i++)
			{
				std::cout << "Starting Tile: " << i << std::endl;

				std::vector<glm::vec4> tilePosition;
				std::vector<glm::vec4> tileV1;
				std::vector<glm::vec4> tileV2;
				std::vector<glm::vec4> tileAttr;
				std::vector<glm::vec4> tileDebug;

				float tile_xMin = FLT_MAX;
				float tile_yMin = FLT_MAX;
				float tile_zMin = FLT_MAX;
				float tile_xMax = -FLT_MAX;
				float tile_yMax = -FLT_MAX;
				float tile_zMax = -FLT_MAX;

#ifdef PARTITIONING_BY_CLUSTERING
				for (unsigned int b = 0; b < amountBlades; b++)
				{
					if (clusterId[b] == i)
					{
						glm::vec3 pos = bladePositions[b].xyz;
						float height = bladeV1[b].w;

						glm::vec3 min = pos - height;
						glm::vec3 max = pos + height;

						if (min.x < tile_xMin) { tile_xMin = min.x; }
						if (min.y < tile_yMin) { tile_yMin = min.y; }
						if (min.z < tile_zMin) { tile_zMin = min.z; }
						if (max.x > tile_xMax) { tile_xMax = max.x; }
						if (max.y > tile_yMax) { tile_yMax = max.y; }
						if (max.z > tile_zMax) { tile_zMax = max.z; }

						tilePosition.push_back(bladePositions[b]);
						tileV1.push_back(bladeV1[b]);
						tileV2.push_back(bladeV2[b]);
						tileAttr.push_back(bladeAttr[b]);
						tileDebug.push_back(glm::vec4(i / 31.0f, (i % 5) / 4.0f, (i % 7) / 6.0f, 1.0f));
					}
				}
#endif

#ifdef EVALUATE_MSE
				glm::vec3 tile_mean = (glm::vec3(tile_xMin, tile_yMin, tile_zMin) + glm::vec3(tile_xMax, tile_yMax, tile_zMax)) * 0.5f;
				for (unsigned int b = 0; b < tilePosition.size(); b++)
				{
					
					glm::vec3 pos = tilePosition[b].xyz;
					glm::vec3 evec = tile_mean - pos;
					meanSquaredError += glm::dot(evec, evec);
				}
#endif

				GrassPatchInfo p;
				p.modelMatrix = glm::mat4(1.0f);
				p.tessellationProps = tessellationProps;
				p.patch = new GrassPatch(tilePosition, tileV1, tileV2, tileAttr, tileDebug, shape);
				p.bounds = new BoundingBox(tile_xMin, tile_xMax, tile_yMin, tile_yMax, tile_zMin, tile_zMax);

				maxAmountBlades = glm::max(maxAmountBlades, p.patch->amountBlades);

				patches.push_back(p);
			}
			
#ifdef EVALUATE_MSE
			meanSquaredError = meanSquaredError / (float)amountBlades;
			std::cout << "MSE = " << std::to_string(meanSquaredError) << std::endl;
#endif

#ifdef PARTITIONING_BY_CLUSTERING
			delete[] clusterId;
#endif
		}
		else
		{
			//Do not tile
			GrassPatchInfo p;
			p.modelMatrix = glm::mat4(1.0f);
			p.tessellationProps = tessellationProps;
			p.patch = new GrassPatch(bladePositions, bladeV1, bladeV2, bladeAttr, std::vector<glm::vec4>(), shape);
			p.bounds = new BoundingBox(xMin, xMax, yMin, yMax, zMin, zMax);

			maxAmountBlades = glm::max(maxAmountBlades, p.patch->amountBlades);

			patches.push_back(p);
		}
	}
	else
	{
		//Do not tile
		GrassPatchInfo p;
		p.modelMatrix = glm::mat4(1.0f);
		p.tessellationProps = tessellationProps;
		p.patch = new GrassPatch(bladePositions, bladeV1, bladeV2, bladeAttr, std::vector<glm::vec4>(), shape);
		p.bounds = new BoundingBox(xMin, xMax, yMin, yMax, zMin, zMax);

		maxAmountBlades = glm::max(maxAmountBlades, p.patch->amountBlades);

		patches.push_back(p);
	}
}

void Grass::Draw(const float dt, const Camera& cam)
{
	if (parentObject != 0)
	{
		modelMatrix = parentObject->getTransform();
	}

	if (boundingObject != 0)
	{
		visible = boundingObject->isVisible(cam, modelMatrix);
	}
	
	if (visible)
	{
		OpenGLState::Instance().disable(GL_CULL_FACE);
		for (unsigned int i = 0; i<patches.size(); i++)
		{
			ProcessPatch(patches[i], cam);
		}
		////////////////
		//Force update//
		////////////////
		updateForceShader->bind();
		//Pressure Map
		glBindImageTexture(0, pressureMap->Handle(), 0, GL_FALSE, 0, GL_READ_WRITE, GL_RGBA32F);
		updateForceShader->setUniform("pressureMapBlockSize", (unsigned int)ceilf(sqrtf((float)maxAmountBlades)));

		//Height Map
		if (heightMap != 0)
		{
			updateForceShader->setUniform("useHeightMap", (GLboolean)true);
			updateForceShader->setUniform("heightMapBounds", heightMapBounds);
			heightMap->bind(1);
			updateForceShader->setUniform("heightMap", (GLint)1);
		}
		else
		{
			updateForceShader->setUniform("useHeightMap", (GLboolean)false);
		}

		//Forces
		if (wind.size() > 0)
		{
			updateForceShader->setUniform("windType", (GLuint)wind[0]->getWindType());
			updateForceShader->setUniform("windData", wind[0]->getWindData());
			//TODO add multiple wind setting
		}
		else
		{
			updateForceShader->setUniform("windType", (GLuint)99);
			updateForceShader->setUniform("windData", glm::vec4(0.0f));
		}

		if (useLocalGravity)
		{
			updateForceShader->setUniform("gravityVec", localGravity.gravityVector);
			updateForceShader->setUniform("gravityPoint", localGravity.gravityPoint);
			updateForceShader->setUniform("useGravityPoint", localGravity.gravityPointAlpha);
		}
		else
		{
			updateForceShader->setUniform("gravityVec", overmind->getGravity().gravityVector);
			updateForceShader->setUniform("gravityPoint", overmind->getGravity().gravityPoint);
			updateForceShader->setUniform("useGravityPoint", overmind->getGravity().gravityPointAlpha);
		}

		//Misc Settings
		updateForceShader->setUniform("dt", dt);

		for (unsigned int i = 0; i < patches.size(); i++)
		{
			if (patches[i].forceVisible)
			{
				UpdatePatchForce(patches[i]);
			}
		}

		glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
		/////////////////////
		//Visibility update//
		/////////////////////
		updateVisibilityShader->bind();

		//Height Map
		if (heightMap != 0)
		{
			updateVisibilityShader->setUniform("useHeightMap", (GLboolean)true);
			updateVisibilityShader->setUniform("heightMapBounds", heightMapBounds);
			heightMap->bind(1);
			updateVisibilityShader->setUniform("heightMap", (GLint)1);
		}
		else
		{
			updateVisibilityShader->setUniform("useHeightMap", (GLboolean)false);
			updateVisibilityShader->setUniform("heightMap", (GLint)1);
		}

		//Inner Spheres
		if (overmind->getInnerSphereCulling() && overmind->innerSphereList != 0)
		{
			auto list = *(overmind->innerSphereList);
			updateVisibilityShader->setUniform("innerSphereAmount", (GLuint)list.size());
			updateVisibilityShader->setUniform("innerSphere[0]", list);
		}
		else
		{
			updateVisibilityShader->setUniform("innerSphereAmount", (GLuint)0);
		}

		//Depth Texture
		if (depthTexture != 0 && overmind->getDepthBufferCulling())
		{
			depthTexture->bind(0);
			updateVisibilityShader->setUniform("depthTexture", 0);
			updateVisibilityShader->setUniform("doDepthBufferCulling", (GLboolean)true);
			updateVisibilityShader->setUniform("widthHeight", glm::vec2(cam.width, cam.height));
		}
		else
		{
			updateVisibilityShader->setUniform("doDepthBufferCulling", (GLboolean)false);
		}

		//Misc Settings
		updateVisibilityShader->setUniform("vpMatrix", cam.viewProjectionMatrix);
		updateVisibilityShader->setUniform("nearFar", glm::vec2(cam.near, cam.far));
		updateVisibilityShader->setUniform("camPos", cam.position);
		updateVisibilityShader->setUniform("maxDistance", overmind->getMaxDistance());
		updateVisibilityShader->setUniform("doDepthCulling", (GLboolean)overmind->getDepthCulling());
		updateVisibilityShader->setUniform("doVFC", (GLboolean)overmind->getViewFrustumCulling());
		updateVisibilityShader->setUniform("doOrientationCulling", (GLboolean)overmind->getOrientationCulling());
		updateVisibilityShader->setUniform("depthCullLevel", overmind->getDepthCullLevel());

		for (unsigned int i = 0; i<patches.size(); i++)
		{
			if (patches[i].visible)
			{
				UpdatePatchVisibility(patches[i]);
			}
		}

		////////
		//Draw//
		////////
		drawShader->bind();

		//VS Uniforms
		if (heightMap != 0)
		{
			drawShader->setUniform("useHeightMap", (GLboolean)true);
			drawShader->setUniform("heightMapBounds", heightMapBounds);
			heightMap->bind(1);
			drawShader->setUniform("heightMap", (GLint)1);
		}
		else
		{
			drawShader->setUniform("useHeightMap", (GLboolean)false);
		}

		//TCS Uniforms
		drawShader->setUniform("camPos", cam.position);

		//TES Uniforms
		drawShader->setUniform("vpMatrix", cam.viewProjectionMatrix);
		drawShader->setUniform("halfScreenSize", glm::ivec2(glm::vec2(cam.width, cam.height) / 2.0f));

		//FS Uniforms
		if (altDiffuseTexture == 0)
		{
			diffuseTexture->bind(0);
		}
		else
		{
			altDiffuseTexture->bind(0);
		}
		
		drawShader->setUniform("diffuseTexture", (GLint)0);
		
		const float ambient = 1.1f;
		const float diffuse = 0.6f;
		const float specular = 1.0f;

		drawShader->setUniform("ambientCoefficient", ambient);
		drawShader->setUniform("diffuseCoefficient", diffuse);
		drawShader->setUniform("specularCoefficient", specular);
		
		drawShader->setUniform("specularHardness", 600.0f);
		drawShader->setUniform("lightDirection", LIGHTDIR);
		drawShader->setUniform("nearFar", glm::vec2(cam.near, cam.far));
		drawShader->setUniform("lightColor", glm::vec3(1.0f, 1.0f, 1.0f));
		drawShader->setUniform("useDebugColor", (GLboolean)overmind->getUseDebugColor());
		drawShader->setUniform("useFlare", (GLboolean)overmind->getUseFlare());
		drawShader->setUniform("usePositionColor", (GLboolean)overmind->getUsePositionColor());

		for (unsigned int i = 0; i<patches.size(); i++)
		{
			DrawPatch(patches[i]);
		}
		OpenGLState::Instance().enable(GL_CULL_FACE);
	}
}

void Grass::ProcessPatch(GrassPatchInfo& patch, const Camera& cam) const
{
	glm::mat4 patchModelMatrix = modelMatrix * patch.modelMatrix;

	float patchVisible = -1.0f;

	if (patch.bounds != 0)
	{
		if (heightMap == 0)
		{
			patchVisible = patch.bounds->isVisibleF2(cam, patchModelMatrix);
		}
		else
		{
			BoundingBox b(*(patch.bounds));
			b.inflate(glm::vec3(0.0f, heightMap->heightScale, 0.0f));
			patchVisible = b.isVisibleF2(cam, patchModelMatrix);
		}
	}

	if (patchVisible < 0.0f)
	{
		patch.visible = true;
		patch.forceVisible = true;
	}
	else
	{
		patch.visible = false;
		if (patchVisible < 2.0f)
		{
			patch.forceVisible = true;
		}
		else
		{
			patch.forceVisible = false;
		}
	}
}

void Grass::UpdatePatchForce(const GrassPatchInfo& patch) const
{
	glm::mat4 patchModelMatrix = modelMatrix * patch.modelMatrix;
	glm::mat4 invPatchModelMatrix = glm::inverse(patchModelMatrix);
	glm::mat3 invTransPatchModelMatrix = glm::inverse(glm::transpose(glm::mat3(patchModelMatrix)));

	//Pressure Map offset
	updateForceShader->setUniform("pressureMapOffset", patch.pressureMapOffset);

	//Collider
	if (overmind->colliderList != 0 && overmind->getCollisionDetection())
	{
		std::vector<glm::vec4> collider;
		collider.reserve(overmind->colliderList->size());

		auto list = *(overmind->colliderList);
		for each (glm::vec4 coll in list)
		{
			if (patch.bounds != 0)
			{
				if (heightMap == 0)
				{
					if (intersect(patch.bounds->transform(patchModelMatrix), coll))
					{
						collider.push_back(coll);
					}
				}
				else
				{
					BoundingBox b(*(patch.bounds));
					b.inflate(glm::vec3(0.0f, heightMap->heightScale, 0.0f));
					if (intersect(b.transform(patchModelMatrix), coll))
					{
						collider.push_back(coll);
					}
				}
			}
			else
			{
				collider.push_back(coll);
			}
		}

		if (collider.size() > MAX_AMOUNT_SPHERE_COLLIDER)
		{
			collider.resize(MAX_AMOUNT_SPHERE_COLLIDER);
		}

		updateForceShader->setUniform("amountSphereCollider", (GLuint)collider.size());
		updateForceShader->setUniform("sphereCollider[0]", collider);
	}
	else
	{
		updateForceShader->setUniform("amountSphereCollider", (GLuint)0);
	}

	//Misc Settings
	updateForceShader->setUniform("modelMatrix", patchModelMatrix);
	updateForceShader->setUniform("invModelMatrix", invPatchModelMatrix);
	updateForceShader->setUniform("invTransModelMatrix", invTransPatchModelMatrix);

	patch.patch->updateForce(*updateForceShader);
}

void Grass::UpdatePatchVisibility(const GrassPatchInfo& patch) const
{
	glm::mat4 patchModelMatrix = modelMatrix * patch.modelMatrix;
	glm::mat3 invTransPatchModelMatrix = glm::inverse(glm::transpose(glm::mat3(patchModelMatrix)));

	//Misc Settings
	updateVisibilityShader->setUniform("modelMatrix", patchModelMatrix);
	updateVisibilityShader->setUniform("invTransModelMatrix", invTransPatchModelMatrix);	

	patch.patch->updateVisibility(*updateVisibilityShader, *copyBufferShader);
}

void Grass::DrawPatch(const GrassPatchInfo& patch) const
{
	glm::mat4 patchModelMatrix = modelMatrix * patch.modelMatrix;

	//VS Uniforms
	drawShader->setUniform("modelMatrix", patchModelMatrix);

	//TCS Uniforms
	drawShader->setUniform("tessellationProps", patch.tessellationProps);

	//TES Uniforms
	GLuint subroutine = patch.patch->bladeShape;
	glUniformSubroutinesuiv(GL_TESS_EVALUATION_SHADER, 1, &subroutine);

	patch.patch->draw(*drawShader);
}

#pragma endregion

//*******************************************
//************ GrassPatch ******************
//*******************************************
#pragma region GrassPatch
GrassPatch::GrassPatch(const std::vector<glm::vec4>& pos, const std::vector<glm::vec4>& v1, const std::vector<glm::vec4>& v2, const std::vector<glm::vec4>& attr, const std::vector<glm::vec4>& debug, const BladeShape bladeShape) : bladeShape(bladeShape)
{
	amountBlades = pos.size();
	if (v1.size() != amountBlades || v2.size() != amountBlades || attr.size() != amountBlades)
	{
		std::cout << "ERROR GrassPatch: Different Vector sizes!" << std::endl;
	}

	//Additional initializations
	std::vector<glm::vec4> debugLocal;
	if (debug.size() == 0)
	{
		debugLocal = std::vector<glm::vec4>(amountBlades, glm::vec4(0));
	}
	else
	{
		debugLocal = debug;
	}
	std::vector<GLuint> index(amountBlades);
	std::iota(index.begin(), index.end(), 0);
	IndirectBufferStruct indirectBufferEntry = { (GLuint)amountBlades, (GLuint)1, (GLuint)0, (GLuint)0, (GLuint)0 };
	GLuint atomicCounterData = 0;

	//GL stuff
	glGenVertexArrays(1, &grassVAO);
	glGenBuffers(GrassBufferEnum::AMOUNT_BUFFER, grassBuffer);

	glBindVertexArray(grassVAO);

	glBindBuffer(GL_ARRAY_BUFFER, grassBuffer[GrassBufferEnum::POSITION]);
	glBufferData(GL_ARRAY_BUFFER, amountBlades * sizeof(glm::vec4), pos.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(GrassBufferEnum::POSITION);
	glVertexAttribPointer(GrassBufferEnum::POSITION, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, grassBuffer[GrassBufferEnum::V1]);
	glBufferData(GL_ARRAY_BUFFER, amountBlades * sizeof(glm::vec4), v1.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(GrassBufferEnum::V1);
	glVertexAttribPointer(GrassBufferEnum::V1, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, grassBuffer[GrassBufferEnum::V2]);
	glBufferData(GL_ARRAY_BUFFER, amountBlades * sizeof(glm::vec4), v2.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(GrassBufferEnum::V2);
	glVertexAttribPointer(GrassBufferEnum::V2, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, grassBuffer[GrassBufferEnum::ATTR]);
	glBufferData(GL_ARRAY_BUFFER, amountBlades * sizeof(glm::vec4), attr.data(), GL_STATIC_DRAW);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_ARRAY_BUFFER, grassBuffer[GrassBufferEnum::DEBUGOUT]);
	glBufferData(GL_ARRAY_BUFFER, amountBlades * sizeof(glm::vec4), debugLocal.data(), GL_STATIC_DRAW);
	glEnableVertexAttribArray(GrassBufferEnum::DEBUGOUT);
	glVertexAttribPointer(GrassBufferEnum::DEBUGOUT, 4, GL_FLOAT, GL_FALSE, 0, 0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);

	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, grassBuffer[GrassBufferEnum::INDIRECT]);
	glBufferData(GL_DRAW_INDIRECT_BUFFER, sizeof(IndirectBufferStruct), &indirectBufferEntry, GL_STATIC_DRAW);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);

	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, grassBuffer[GrassBufferEnum::ATOMIC_COUNTER]);
	glBufferData(GL_ATOMIC_COUNTER_BUFFER, sizeof(GLuint), &atomicCounterData, GL_DYNAMIC_DRAW);
	glBindBuffer(GL_ATOMIC_COUNTER_BUFFER, 0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, grassBuffer[GrassBufferEnum::INDEX]);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, amountBlades * sizeof(GLuint), index.data(), GL_STATIC_DRAW);

	glBindVertexArray(0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

GrassPatch::~GrassPatch()
{
	glDeleteBuffers(GrassBufferEnum::AMOUNT_BUFFER, grassBuffer);
	glDeleteVertexArrays(1, &grassVAO);

}

void GrassPatch::updateForce(const Shader& shader)
{
	shader.setUniform("amountBlades", amountBlades);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GrassBufferEnum::POSITION, grassBuffer[GrassBufferEnum::POSITION]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GrassBufferEnum::V1, grassBuffer[GrassBufferEnum::V1]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GrassBufferEnum::V2, grassBuffer[GrassBufferEnum::V2]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GrassBufferEnum::ATTR, grassBuffer[GrassBufferEnum::ATTR]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GrassBufferEnum::DEBUGOUT, grassBuffer[GrassBufferEnum::DEBUGOUT]);

	timeForce.Start();
	glDispatchCompute((amountBlades / shader.max_work_group_size_X) + 1, 1, 1);
	timeForce.Stop();
}

void GrassPatch::updateVisibility(const Shader& shader, const Shader& copyBuffer) 
{
	shader.setUniform("amountBlades", amountBlades);

	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GrassBufferEnum::POSITION, grassBuffer[GrassBufferEnum::POSITION]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GrassBufferEnum::V1, grassBuffer[GrassBufferEnum::V1]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GrassBufferEnum::V2, grassBuffer[GrassBufferEnum::V2]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GrassBufferEnum::ATTR, grassBuffer[GrassBufferEnum::ATTR]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GrassBufferEnum::DEBUGOUT, grassBuffer[GrassBufferEnum::DEBUGOUT]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GrassBufferEnum::INDEX, grassBuffer[GrassBufferEnum::INDEX]);
	glBindBufferBase(GL_SHADER_STORAGE_BUFFER, GrassBufferEnum::ATOMIC_COUNTER, grassBuffer[GrassBufferEnum::INDIRECT]);

	timeVis.Start();
	glDispatchCompute((amountBlades / shader.max_work_group_size_X) + 1, 1, 1);
	timeVis.Stop();

	glMemoryBarrier(GL_SHADER_STORAGE_BARRIER_BIT);
}

void GrassPatch::draw(const Shader& shader) 
{
	glBindVertexArray(grassVAO);
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, grassBuffer[GrassBufferEnum::INDIRECT]);

	OpenGLState::Instance().setPatchVertices(1);
	timeDraw.Start();
	glDrawElementsIndirect(GL_PATCHES, GL_UNSIGNED_INT, 0);
	timeDraw.Stop();
	
	glBindBuffer(GL_DRAW_INDIRECT_BUFFER, 0);
	glBindVertexArray(0);
}

unsigned int GrassPatch::fetchBladesDrawn()
{
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, grassBuffer[GrassBufferEnum::INDIRECT]);
	IndirectBufferStruct* ind = (IndirectBufferStruct*)glMapBuffer(GL_SHADER_STORAGE_BUFFER, GL_READ_ONLY);
	unsigned int bDrawn = ind->count;
	glUnmapBuffer(GL_SHADER_STORAGE_BUFFER);
	glBindBuffer(GL_SHADER_STORAGE_BUFFER, 0);
	return bDrawn;
}

double GrassPatch::fetchTimeForce()
{
	return timeForce.Time();
}

double GrassPatch::fetchTimeVis()
{
	return timeVis.Time();
}

double GrassPatch::fetchTimeDraw()
{
	return timeDraw.Time();
}

#pragma endregion

//*******************************************
//*********** GrassOvermind ****************
//*******************************************
#pragma region GrassOvermind

GrassOvermind GrassOvermind::instance = *new GrassOvermind();

GrassOvermind::GrassOvermind() : grassPatches(), colliderList(0), innerSphereList(0), gravity({glm::vec4(0.0f,-1.0f,0.0f,1.0f), glm::vec4(0.0f,0.0f,0.0f,1.0f), 0.0f})
{

}

GrassOvermind::~GrassOvermind()
{

}

void GrassOvermind::addGrassInstance(Grass& g)
{
	GrassInstancePatchInfo patchInfo;
	patchInfo.grassInstance = &g;
	unsigned int patchCount = g.patches.size();
	patchInfo.amountPatches = patchCount;
	amountPatches += patchCount;
	grassPatches.push_back(patchInfo);
}

void GrassOvermind::NotifyGrassInstanceUpdated()
{
	amountPatches = 0;
	for(unsigned int i = 0; i < grassPatches.size(); i++)
	{
		unsigned int patchCount = grassPatches[i].grassInstance->patches.size();
		grassPatches[i].amountPatches = patchCount;
		amountPatches += patchCount;
	}
}

void GrassOvermind::removeGrassInstance(Grass* g)
{
	for (std::vector<GrassInstancePatchInfo>::iterator it = grassPatches.begin(); it < grassPatches.end(); it++)
	{
		if (it->grassInstance == g)
		{
			amountPatches -= it->amountPatches;
			grassPatches.erase(it);
			break;
		}
	}
}

void GrassOvermind::NotifyPressureMapUpdated()
{
	unsigned int id = 0;
	for (unsigned int i = 0; i < grassPatches.size(); i++)
	{
		auto g = grassPatches[i];
		g.grassInstance->NotifyPressureMapUpdated(id, amountPatches);
		id += g.amountPatches;
	}
}

void GrassOvermind::registerColliderList(std::vector<glm::vec4>& _colliderList)
{
	colliderList = &_colliderList;
}

void GrassOvermind::registerInnerSphereList(std::vector<glm::vec4>& _innerSphereList)
{
	innerSphereList = &_innerSphereList;
}

void GrassOvermind::setUseDebugColor(const bool value)
{
	useDebugColor = value;
}

void GrassOvermind::setUseFlare(const bool value)
{
	useFlare = value;
}

void GrassOvermind::setUsePositionColor(const bool value)
{
	usePositionColor = value;
}

void GrassOvermind::setInnerSphereCulling(const bool value)
{
	doInnerSphereCulling = value;
}

void GrassOvermind::setDepthBufferCulling(const bool value)
{
	doDepthBufferCulling = value;
}

void GrassOvermind::setDepthCulling(const bool value)
{
	doDepthCulling = value;
}

void GrassOvermind::setViewFrustumCulling(const bool value)
{
	doViewFrustumCulling = value;
}

void GrassOvermind::setOrientationCulling(const bool value)
{
	doOrientationCulling = value;
}

void GrassOvermind::setCollisionDetection(const bool value)
{
	doCollisionDetection = value;
}

void GrassOvermind::setMaxDistance(const float value)
{
	maxDistance = value;
}

void GrassOvermind::setDepthCullLevel(const float value)
{
	depthCullLevel = value;
}

void GrassOvermind::setGravity(const GrassGravity value)
{
	gravity = value;
}

#pragma endregion