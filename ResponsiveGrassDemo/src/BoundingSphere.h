/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef BOUNDINGSPHERE_H
#define BOUNDINGSPHERE_H

#include "BoundingObject.h"
#include "Common.h"

class BoundingSphere : public BoundingObject
{
public:
	BoundingSphere(const float r, const glm::vec3& centerPoint = glm::vec3(0.0f,0.0f,0.0f));
	~BoundingSphere();

	bool isVisible(const Camera& camera, const glm::mat4& transform);
	virtual void inflate(const glm::vec3& value);
	virtual void scale(const glm::vec3& factor);
//private:
	float radius;
	glm::vec3 centerPoint;
};

#endif