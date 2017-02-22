/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef PLANE_H
#define PLANE_H

#include "Common.h"

class Plane
{
public:
	Plane(void);
	~Plane(void);

	enum PlaneSide {
		FRONT,
		BACK,
		RIGHT,
		LEFT,
		TOP,
		BOTTOM
	};

	float getDistance(const glm::vec3& point) const;
	const glm::vec3& getNormal() const;

	void set3Points(glm::vec3 &v1, glm::vec3 &v2, glm::vec3 &v3);
	void setNormalAndPoint(const glm::vec3 & normal, const glm::vec3 & point);
	void setPlaneProps(const glm::vec3 & normal, const float d);

	float d;
	glm::vec3 normal;
};

#endif