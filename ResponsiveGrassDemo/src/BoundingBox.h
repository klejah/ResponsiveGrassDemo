/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef BOUNDINGBOX_H
#define BOUNDINGBOX_H

#include "Common.h"
#include "BoundingObject.h"
#include "Plane.h"
#include "Camera.h"

class BoundingBox : public BoundingObject
{
public:
	BoundingBox();
	BoundingBox(float xMin, float xMax, float yMin, float yMax, float zMin, float zMax);
	~BoundingBox();

	float isVisibleF(const Camera& camera);
	float isVisibleF2(const Camera& camera, const glm::mat4& transform);
	virtual bool isVisible(const Camera& camera, const glm::mat4& transform);
	virtual void inflate(const glm::vec3& value);
	virtual void scale(const glm::vec3& factor);

	glm::vec3 getNearestVertex(const Plane& p) const;

	class TransformedBox
	{
	public:
		TransformedBox();
		TransformedBox(const glm::vec4& location, const glm::vec4& axis1, const glm::vec4& axis2, const glm::vec4& axis3);
		TransformedBox(const TransformedBox& box);

		virtual bool isVisible(const Camera& camera);
		float isVisibleF2(const Camera& camera);
	public:
		glm::vec3 location, axis1, axis2, axis3;
		glm::vec3 normalizedAxis1, normalizedAxis2, normalizedAxis3;
	};

	TransformedBox& transform(const glm::mat4& transform);

	float xMin, xMax, yMin, yMax, zMin, zMax;

public:
	glm::vec4 location;
	glm::vec3 size;

	unsigned short lastFail;

	TransformedBox transformed;
	glm::mat4 lastTransform;
};

#endif