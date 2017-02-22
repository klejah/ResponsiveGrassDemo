/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "Plane.h"

Plane::Plane(void)
{
}


Plane::~Plane(void)
{
}

float Plane::getDistance(const glm::vec3  &point) const
{
	return glm::dot(normal, point) + d;
}

const glm::vec3& Plane::getNormal() const
{
	return normal;
}

void Plane::setPlaneProps(const glm::vec3& _normal, const float _d)
{
	normal = _normal;
	d = _d;
}

void Plane::set3Points(glm::vec3 &v1, glm::vec3 &v2, glm::vec3 &v3)
{
	glm::vec3 aux1, aux2;

	aux1 = glm::normalize(v1 - v2);
	aux2 = glm::normalize(v3 - v2);

	normal = glm::normalize(glm::cross(aux2, aux1));
	d = -glm::dot(normal, v2);
}

void Plane::setNormalAndPoint(const glm::vec3& _normal, const glm::vec3& point)
{
	normal = _normal;
	d = -glm::dot(_normal, point);
}