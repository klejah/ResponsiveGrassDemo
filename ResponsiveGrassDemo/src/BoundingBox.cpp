/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "BoundingBox.h"

BoundingBox::BoundingBox() : xMin(0.0f), xMax(0.0f), yMin(0.0f), yMax(0.0f), zMin(0.0f), zMax(0.0f), lastFail(0), location(0.0f, 0.0f, 0.0f, 1.0f), size(0.0f, 0.0f, 0.0f), lastTransform(0.0f)
{

}

BoundingBox::BoundingBox(float _xMin, float _xMax, float _yMin, float _yMax, float _zMin, float _zMax) : xMin(_xMin), xMax(_xMax), yMin(_yMin), yMax(_yMax), zMin(_zMin),
zMax(_zMax), lastFail(0), location((_xMin + _xMax) / 2.0f, (_yMin + _yMax) / 2.0f, (_zMin + _zMax) / 2.0f, 1.0f), size(_xMax - _xMin, _yMax - _yMin, _zMax - _zMin), lastTransform(0.0f)
{

}

BoundingBox::~BoundingBox()
{

}

float BoundingBox::isVisibleF(const Camera& camera)
{
	glm::vec3 p;
	float dist = 0.0f;

	for (unsigned short i = 0; i<6; i++)
	{
		const Plane& side = camera.frustumPlanes[(lastFail + i) % 6];

		if (side.getNormal().x >= 0.0f)
			p.x = xMax;
		else
			p.x = xMin;

		if (side.getNormal().y >= 0.0f)
			p.y = yMax;
		else
			p.y = yMin;

		if (side.getNormal().z >= 0.0f)
			p.z = zMax;
		else
			p.z = zMin;

		float d = side.getDistance(p);
		if (d < 0.0f)
		{
			lastFail = i;

			return -1.0f;
		}

		if ((lastFail + i) % 6 == Plane::FRONT)
		{
			dist = d;
		}
	}

	return dist;
}

float BoundingBox::isVisibleF2(const Camera& camera, const glm::mat4& _transform)
{
	TransformedBox box = transform(_transform);
	return box.isVisibleF2(camera);
}

bool BoundingBox::isVisible(const Camera& camera, const glm::mat4& _transform)
{
	TransformedBox box = transform(_transform);
	return box.isVisible(camera);
}

void BoundingBox::inflate(const glm::vec3& value)
{
	xMin -= value.x * 0.5f;
	yMin -= value.y * 0.5f;
	zMin -= value.z * 0.5f;
	xMax += value.x * 0.5f;
	yMax += value.y * 0.5f;
	zMax += value.z * 0.5f;
	size = glm::vec3(xMax - xMin, yMax - yMin, zMax - zMin);
}

void BoundingBox::scale(const glm::vec3& factor)
{
	size = size * factor;
	xMin = location.x - size.x * 0.5f;
	xMax = location.x + size.x * 0.5f;
	yMin = location.y - size.y * 0.5f;
	yMax = location.y + size.y * 0.5f;
	zMin = location.z - size.z * 0.5f;
	zMax = location.z + size.z * 0.5f;
}

glm::vec3 BoundingBox::getNearestVertex(const Plane& side) const
{
	glm::vec3 p;
	if (side.getNormal().x >= 0)
		p.x = xMax;
	else
		p.x = xMin;

	if (side.getNormal().y >= 0)
		p.y = yMax;
	else
		p.y = yMin;

	if (side.getNormal().z >= 0)
		p.z = zMax;
	else
		p.z = zMin;
	return p;
}

BoundingBox::TransformedBox& BoundingBox::transform(const glm::mat4& transform)
{
	if (transform != lastTransform)
	{
		lastTransform = transform;
		transformed = TransformedBox(
			transform * location,
			transform * glm::vec4(size.x, 0.0f, 0.0f, 0.0f),
			transform * glm::vec4(0.0f, size.y, 0.0f, 0.0f),
			transform * glm::vec4(0.0f, 0.0f, size.z, 0.0f));
	}
	return transformed;
}

BoundingBox::TransformedBox::TransformedBox()
{}

BoundingBox::TransformedBox::TransformedBox(const glm::vec4& _location, const glm::vec4& _axis1, const glm::vec4& _axis2, const glm::vec4& _axis3) : location(_location.xyz()),
axis1(_axis1.xyz() / 2.0f), axis2(_axis2.xyz() / 2.0f), axis3(_axis3.xyz() / 2.0f),
normalizedAxis1(glm::normalize(_axis1.xyz())), normalizedAxis2(glm::normalize(_axis2.xyz())),
normalizedAxis3(glm::normalize(_axis3.xyz()))
{

}

BoundingBox::TransformedBox::TransformedBox(const BoundingBox::TransformedBox& box) : location(box.location), axis1(box.axis1), axis2(box.axis2), axis3(box.axis3), normalizedAxis1(box.normalizedAxis1),
normalizedAxis2(box.normalizedAxis2), normalizedAxis3(box.normalizedAxis3)
{

}

bool BoundingBox::TransformedBox::isVisible(const Camera& camera)
{
	for (unsigned short i = 0; i<6; i++)
	{
		const Plane& side = camera.frustumPlanes[i];

		glm::vec3 nBoxSpace = glm::vec3(glm::dot(normalizedAxis1, side.getNormal()), glm::dot(normalizedAxis2, side.getNormal()), glm::dot(normalizedAxis3, side.getNormal()));

		glm::vec3 p = location;

		if (nBoxSpace.x >= 0)
			p += axis1;
		else
			p -= axis1;

		if (nBoxSpace.y >= 0)
			p += axis2;
		else
			p -= axis2;

		if (nBoxSpace.z >= 0)
			p += axis3;
		else
			p -= axis3;

		if (side.getDistance(p) < 0.0f)
		{
			return false;
		}
	}

	return true;
}

float BoundingBox::TransformedBox::isVisibleF2(const Camera& camera)
{
	glm::vec3 p;
	float dist = -FLT_MAX;
	bool out = false;

	for (unsigned short i = 0; i<6; i++)
	{
		const Plane& side = camera.frustumPlanes[i];

		glm::vec3 nBoxSpace = glm::vec3(glm::dot(normalizedAxis1, side.getNormal()), glm::dot(normalizedAxis2, side.getNormal()), glm::dot(normalizedAxis3, side.getNormal()));

		glm::vec3 p = location;

		if (nBoxSpace.x >= 0)
			p += axis1;
		else
			p -= axis1;

		if (nBoxSpace.y >= 0)
			p += axis2;
		else
			p -= axis2;

		if (nBoxSpace.z >= 0)
			p += axis3;
		else
			p -= axis3;

		float d = side.getDistance(p);
		if (d < 0.0f)
		{
			dist = glm::max(dist, d);
			out = true;
		}
	}

	if (out)
		return -dist;
	else
		return -1.0f;
}