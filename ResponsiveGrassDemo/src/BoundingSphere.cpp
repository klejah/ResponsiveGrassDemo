/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "BoundingSphere.h"

BoundingSphere::BoundingSphere(const float r, const glm::vec3& center) : radius(r), centerPoint(center)
{

}

BoundingSphere::~BoundingSphere()
{

}

bool BoundingSphere::isVisible(const Camera& camera, const glm::mat4& transform)
{
	glm::vec4 center = transform * glm::vec4(centerPoint, 1.0f);

	for (unsigned int i = 0; i < 6; i++)
	{
		auto side = camera.frustumPlanes[i];

		float distance = side.getDistance(glm::vec3(center));

		if (distance + radius < 0.0f)
		{
			return false;
		}
	}
	return true;
}

void BoundingSphere::inflate(const glm::vec3& value)
{
	radius += glm::max(value.x, glm::max(value.y, value.z));
}

void BoundingSphere::scale(const glm::vec3& factor)
{
	radius *= glm::max(factor.x, glm::max(factor.y, factor.z));
}