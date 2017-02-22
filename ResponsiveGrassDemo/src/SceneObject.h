/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef SCENE_OBJECT_H
#define SCENE_OBJECT_H

#include "Common.h"
#include "physx/PxRigidDynamic.h"
#include "Shader.h"
#include "Camera.h"
#include "Texture2D.h"
#include "BoundingObject.h"
#include "BoundingSphere.h"
#include "AssimpImporter.h"
#include "Geometry.h"
#include "Clock.h"
#include <vector>

class SceneObject;

class SceneObjectGeometry
{
	friend class SceneObject;
public:
	enum BasicGeometry {
		SPHERE,BOX, OTHER
	};

	physx::PxGeometry* physicalGeometry;
	physx::PxMaterial* physicalMaterial;
	physx::PxReal physicalAngularDamping;
	physx::PxReal physicalDensity;

	glm::mat4 geometryMatrix, rootTransform;

	bool hasNormals = false, hasUVs = false, hasTangentsAndBitangents = false;

	SceneObjectGeometry(BasicGeometry type, const glm::vec3& scale, const bool calculateInnerSphere, const unsigned int innerSphereMethod, const bool generatePhysicalObject, const float staticFriction, const float dynamicFriction, const float restitution, const float angularDamping, const float density);
	SceneObjectGeometry(std::string filename, BasicGeometry boundingType, const glm::vec3& scale, const bool generateFaceList, const bool calculateInnerSphere, const unsigned int innerSphereMethod, const bool generatePhysicalObject, const float staticFriction, const float dynamicFriction, const float restitution, const float angularDamping, const float density);
	SceneObjectGeometry(const AssimpImporter::ImportModel* geometryModel, const BasicGeometry boundingType, const glm::vec3& scale, const bool generateFaceList, const bool calculateInnerSphere, const unsigned int innerSphereMethod, const bool generatePhysicalObject, const float staticFriction, const float dynamicFriction, const float restitution, const float angularDamping, const float density);
	~SceneObjectGeometry();

	void draw(GLenum drawMode, glm::mat4& modelMatrix, Shader* drawShader);
	BoundingObject* getBoundingObject() const;
	BasicGeometry getGeometryType() const;
	const glm::vec3& getGeometryScale() const;
	void calculateInnerSphereGreedy(const std::vector<unsigned int> index, const std::vector<glm::vec3> position, const unsigned int method, const unsigned int n = 100);
	std::vector<Geometry::TriangleFace>& getFaceList() { return faceList; }

	glm::vec3 min, max;
private:
	SceneObjectGeometry(const AssimpImporter::ImportModel* geometryModel, const glm::vec3& scale);

	static unsigned int instanceCount;

	GLuint vaoHandle;
	std::vector<GLuint> vboHandle;
	bool hasIndizes;
	GLsizei count;
	BoundingObject* boundingObject;
	BoundingSphere* innerSphere;
	BasicGeometry geometryType;
	glm::vec3 geometryScale;
	std::vector<SceneObjectGeometry*> childGeometries;
	std::vector<Geometry::TriangleFace> faceList;
	void uploadDataToGraphicsCard(const std::vector<unsigned int>* index, const std::vector<glm::vec3>* position, const std::vector<glm::vec3>* normal, const std::vector<glm::vec2>* uv, const std::vector<glm::vec3>* tangent, const std::vector<glm::vec3>* bitangent, const glm::vec3& scale);
};

class SceneObject
{
public:
	struct TextureNamePtr
	{
		std::string name;
		Texture2D* ptr;
	};

	SceneObject(Shader* drawShader, SceneObjectGeometry* geometry, const bool isStatic, const glm::mat4& position, const glm::vec4& tessellationProps, const glm::vec4& textureTileAndOffset = glm::vec4(1, 1, 0, 0), const glm::vec4& heightMapTileAndOffset = glm::vec4(1, 1, 0, 0), const float ambientCoefficient = 0.2f, const float diffuseCoefficient = 0.7f, const float specularCoefficient = 0.5f, const float specularExponent = 10.0f);
	~SceneObject();

	virtual void update(const Camera& cam, const Clock& time);
	virtual void draw(const Camera& cam);

	void attachGeometry(SceneObjectGeometry* geometry);
	void attachChild(SceneObject* child);
	void attachTexture(std::string name, Texture2D* ptr);
	Texture2D* detachTexture(const std::string& name);
	void setPosition(glm::mat4& position);
	void applyForce(glm::vec3 force);

	bool isVisible() const;
	bool hasInnerSphere() const;
	SceneObjectGeometry::BasicGeometry getGeometryType() const;
	const glm::mat4& getTransform() const;
	const glm::vec3& getGeometryScale() const;
	const glm::vec4 getInnerSphereVector() const;
	BoundingObject* getBoundingObject() const;
	SceneObjectGeometry* getGeometry() const;

protected:
	BoundingObject* bounds;
	physx::PxRigidActor* physicalObject;
	SceneObjectGeometry* geometry;
	std::vector<SceneObject*> children;
	Shader* drawShader;
	std::vector<TextureNamePtr> textures;
	glm::mat4 modelMatrix;
	bool visible;
	bool isStatic;

	glm::vec4 tessellationProps;
	glm::vec4 textureTileAndOffset;
	glm::vec4 heightMapTileAndOffset;
	float ambientCoefficient;
	float diffuseCoefficient;
	float specularCoefficient;
	float specularExponent;

	bool hasDiffuseTexture = false, hasSpecularTexture = false, hasNormalMap = false, hasAlphaTexture = false, hasHeightMap = false;

	virtual void parentDraw(glm::mat4& parentMatrix);
};

#endif