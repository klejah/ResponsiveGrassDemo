/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef BOUNDINGOBJECT_H
#define BOUNDINGOBJECT_H

#include "Camera.h"
#include "Common.h"

class BoundingObject
{
public:
	BoundingObject(){}
	~BoundingObject(){}

	virtual bool isVisible(const Camera& cam, const glm::mat4& transform) = 0;
	virtual void inflate(const glm::vec3& value) = 0;
	virtual void scale(const glm::vec3& factor) = 0;
};

#endif