/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef SPHERE_PACKED_OBJECT_H
#define SPHERE_PACKED_OBJECT_H

#include "SceneObject.h"
#include "SpherePacker.h"

class SpherePackedObject : public SceneObject
{
private:
	static Shader* sphereShader;
	static GLuint sphereVAO;
	static GLuint sphereVBO;

public:
	SpherePackedObject(Shader* drawShader, std::string& filename, glm::vec3& scale, const bool generatePhysicalObject, const bool isStatic, const glm::mat4& position, const unsigned int maxIterations, const float staticFriction, const float dynamicFriction, const float restitution);
	~SpherePackedObject();

	std::vector<glm::vec4> spheres;

	void draw(const Camera& cam);

	bool drawObject = true;
};

#endif