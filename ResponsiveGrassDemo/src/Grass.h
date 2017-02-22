/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef GRASS_H
#define GRASS_H

#include <vector>
#include "Common.h"
#include "BoundingBox.h"
#include "Texture2D.h"
#include "SceneObject.h"
#include "Geometry.h"
#include "HeightMap.h"
#include "WindGenerator.h"
#include "GLClock.h"

#pragma region GrassPatch
enum BladeShape { QUAD, TRIANGLE, QUADRATIC, QUADRATIC3D, QUADRATIC3DMINW, THRESHTRIANGLEMINW, DANDELION };

#pragma region BladeShapeMethods
static std::string toString(BladeShape g)
{
	switch (g)
	{
	case QUAD: return "Quad";
	case TRIANGLE: return "Triangle";
	case QUADRATIC: return "Quadratic";
	case QUADRATIC3D: return "Quadratic with 3D shape";
	case QUADRATIC3DMINW: return "Quadratic with 3D shape and min width";
	case THRESHTRIANGLEMINW: return "Threshold with triangle and min width";
	case DANDELION: return "Dandelion";
	}
	return "No string added for this form";
}
#pragma endregion

class GrassPatch
{
public:
	enum GrassBufferEnum
	{
		POSITION, V1, V2, DEBUGOUT, ATTR, INDEX, INDIRECT, ATOMIC_COUNTER, AMOUNT_BUFFER
	};

private:
	struct IndirectBufferStruct
	{
		GLuint count;
		GLuint primCount;
		GLuint firstIndex;
		GLuint baseVertex;
		GLuint baseIndex;
	};

public:
	GLuint grassBuffer[GrassBufferEnum::AMOUNT_BUFFER];
	GLuint grassVAO;

	GLClock timeForce, timeVis, timeDraw;

	unsigned int amountBlades;
	BladeShape bladeShape;
public:
	GrassPatch(const std::vector<glm::vec4>& pos, const std::vector<glm::vec4>& v1, const std::vector<glm::vec4>& v2, const std::vector<glm::vec4>& attr, const std::vector<glm::vec4>& debug, const BladeShape = THRESHTRIANGLEMINW);
	~GrassPatch();

	void updateForce(const Shader& shader);
	void updateVisibility(const Shader& shader, const Shader& copyBuffer);
	void draw(const Shader& shader);

	unsigned int fetchBladesDrawn();
	double fetchTimeForce();
	double fetchTimeVis();
	double fetchTimeDraw();
};
#pragma endregion

#pragma region Grass
enum GrassDistribution
{
	UNIFORM, CLUSTER
};

enum GrassSpacialDistribution
{
	FACE_RANDOM, FACE_AREA
};
	
struct GrassGravity
{
	glm::vec4 gravityVector;
	glm::vec4 gravityPoint;
	float gravityPointAlpha;
};

struct GrassPatchInfo
{
	GrassPatch* patch;
	glm::ivec2 pressureMapOffset;
	BoundingBox* bounds;
	glm::mat4 modelMatrix;
	glm::vec4 tessellationProps;
	bool visible;
	bool forceVisible;
};

struct GrassCreateBladeParams
{
	float density;
	GrassDistribution distribution;
	float clusterPercentage;
	GrassSpacialDistribution spacialDistribution;
	float bladeMinHeight;
	float bladeMaxHeight;
	float bladeMinWidth;
	float bladeMaxWidth;
	float bladeMinBend;
	float bladeMaxBend;
	BladeShape shape;
	glm::vec4 tessellationProps;
};

class GrassOvermind;

class Grass
{
private:
	void ProcessPatch(GrassPatchInfo& patch, const Camera& cam) const;
	void UpdatePatchForce(const GrassPatchInfo& patch) const;
	void UpdatePatchVisibility(const GrassPatchInfo& patch) const;
	void DrawPatch(const GrassPatchInfo& patch) const;

	void DistributeFaceRandom(const GrassCreateBladeParams& p, std::vector <Geometry::TriangleFace>& faces);
	void DistributeFaceArea(const GrassCreateBladeParams& p, std::vector <Geometry::TriangleFace>& faces);
	void GeneratePatches(std::vector<glm::vec4>& bladePositions, std::vector<glm::vec4>& bladeV1, std::vector<glm::vec4>& bladeV2, std::vector<glm::vec4>& bladeAttr, const BladeShape shape, const glm::vec4& tessellationProps);

	static Shader * updateForceShader;
	static Shader * updateVisibilityShader;
	static Shader * copyBufferShader;
	static Shader * drawShader;

public:
	static Texture2D * diffuseTexture;
	Texture2D * altDiffuseTexture;
	static Texture2D * pressureMap;
	static unsigned int maxAmountBlades;
	static unsigned int pressureMapBlockSize;
	static unsigned int amountGrassInstances;

	static void UpdatePressureMap();
	void NotifyPressureMapUpdated(unsigned int id, unsigned int count);

public:
	std::vector<GrassPatchInfo> patches;

	glm::mat4 modelMatrix = glm::mat4(1.0f);
	bool visible = true;
	BoundingObject* boundingObject = 0;

	GrassGravity localGravity;
	bool useLocalGravity = false;
	std::vector<WindGenerator*> wind;

	HeightMap* heightMap = 0;
	glm::vec4 heightMapBounds = glm::vec4(0.0f); //xMin zMin xLength zLength

	Texture2D* depthTexture = 0;

	SceneObject* parentObject = 0;

	GrassOvermind* overmind;

public:
	Grass();
	Grass(const std::vector<GrassCreateBladeParams>& params, std::vector<Geometry::TriangleFace>& faces);
	Grass(const Grass& other);
	~Grass();

	void Initialize(const std::vector<GrassCreateBladeParams>& params, std::vector<Geometry::TriangleFace>& faces);
	void Draw(const float dt, const Camera& cam);
};
#pragma endregion

#pragma region GrassOvermind
class GrassOvermind
{
public:
	static GrassOvermind& getInstance()
	{
		return instance;
	}
	~GrassOvermind();

	void addGrassInstance(Grass& g);
	void removeGrassInstance(Grass* g);
	unsigned int getAmountOfGrassPatches() const { return amountPatches; }

	void NotifyPressureMapUpdated();
	void NotifyGrassInstanceUpdated();
	void registerColliderList(std::vector<glm::vec4>& colliderList);
	void registerInnerSphereList(std::vector<glm::vec4>& _innerSphereList);
	void setUseDebugColor(const bool value);
	void setUseFlare(const bool value);
	void setUsePositionColor(const bool value);
	void setInnerSphereCulling(const bool value);
	void setDepthBufferCulling(const bool value);
	void setOrientationCulling(const bool value);
	void setDepthCulling(const bool value);
	void setViewFrustumCulling(const bool value);
	void setCollisionDetection(const bool value);
	void setMaxDistance(const float value);
	void setDepthCullLevel(const float value);
	void setGravity(const GrassGravity value);
	inline bool getUseDebugColor() const { return useDebugColor; }
	inline bool getUseFlare() const { return useFlare; }
	inline bool getUsePositionColor() const { return usePositionColor; }
	inline bool getInnerSphereCulling() const { return doInnerSphereCulling; }
	inline bool getDepthBufferCulling() const { return doDepthBufferCulling; }
	inline bool getOrientationCulling() const { return doOrientationCulling; }
	inline bool getDepthCulling() const { return doDepthCulling; }
	inline bool getViewFrustumCulling() const { return doViewFrustumCulling; }
	inline bool getCollisionDetection() const { return doCollisionDetection; }
	inline float getMaxDistance() const { return maxDistance; }
	inline float getDepthCullLevel() const { return depthCullLevel; }
	inline GrassGravity getGravity() const { return gravity; }

	std::vector<glm::vec4>* colliderList;
	std::vector<glm::vec4>* innerSphereList;

private:
	static GrassOvermind instance;

	GrassOvermind();

	struct GrassInstancePatchInfo
	{
		Grass* grassInstance;
		unsigned int amountPatches;
	};

	std::vector<GrassInstancePatchInfo> grassPatches;

	GrassGravity gravity;

	unsigned int amountPatches = 0;

	bool useDebugColor = false;
	bool useFlare = true;
	bool usePositionColor = true;
	bool doInnerSphereCulling = true;
	bool doDepthBufferCulling = true;
	bool doDepthCulling = true;
	bool doViewFrustumCulling = true;
	bool doCollisionDetection = true;
	bool doOrientationCulling = true;
	float maxDistance = 100.0f;
	float depthCullLevel = 100.0f;
};
#pragma endregion

#endif