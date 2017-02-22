/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef GRASSOBJECT_H
#define GRASSOBJECT_H

#include "SceneObject.h"
#include "Grass.h"

class GrassObject : public SceneObject
{
public:
	GrassObject(const std::string& filename, const SceneObjectGeometry::BasicGeometry type, const glm::vec3& scale, const bool generatePhysicalObject, const float staticFriction, const float dynamicFriction, const float restitution, const float angularDamping, const float physicalDensity, const std::vector<GrassCreateBladeParams>& params, Shader* drawShader, const bool isStatic, glm::mat4& position, const glm::vec4& tessellationProps, const glm::vec4& textureTileAndOffset = glm::vec4(1, 1, 0, 0), const glm::vec4& heightMapTileAndOffset = glm::vec4(1, 1, 0, 0), float ambientCoefficient = 0.2f, float diffuseCoefficient = 0.7f, float specularCoefficient = 0.5f, float specularExponent = 10.0f);
	GrassObject(const GrassObject& other); //Copy
	GrassObject(const GrassObject& other, const std::vector<GrassCreateBladeParams>& params); //Copy geometry with new grass
	~GrassObject();

	void update(const Camera& cam, const Clock& time) override;
	void draw(const Camera& cam) override;
	void drawGrass(const Camera& cam, const Clock& time);

	Grass* grass;

private:
	std::vector<Geometry::TriangleFace> faces;
	void parentDraw(glm::mat4& parentMatrix) override;
};

#endif