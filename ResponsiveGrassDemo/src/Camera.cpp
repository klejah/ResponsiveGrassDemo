/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "Camera.h"
#include "SceneObject.h"

#include "glm\gtc\matrix_transform.hpp"

Camera::Camera(const float _fov, const float _width, const float _height, const float _near, const float _far, const glm::vec3& _startingPosition)
: position(_startingPosition), viewVector(), horizontalAngle(PI_F), verticalAngle(0.0f), fov(_fov), aspectRatio(_width / _height), near(_near), far(_far), width(_width), height(_height), transformed(true), vAdd(0.0f), hAdd(0.0f), parentObject(0), parentOffset(0.0f), inheritParentRotation(true)
{
	heightNear = glm::tan(fov / 2.0f) * _near;
	heightFar = glm::tan(fov / 2.0f) * _far;
	widthNear = heightNear * aspectRatio;
	widthFar = heightFar * aspectRatio;

	projectionMatrix = glm::perspective(_fov, aspectRatio, _near, _far);
	invProjectionMatrix = glm::inverse(projectionMatrix);
	viewProjectionMatrix = projectionMatrix * viewMatrix;
	invViewProjectionMatrix = glm::inverse(viewProjectionMatrix);
}

Camera::~Camera()
{

}

void Camera::translate(const glm::vec3& _translation)
{
	position += _translation;
	transformed = true;
}

void Camera::rotateHorizontal(const float _angle)
{
	horizontalAngle += _angle;
	transformed = true;
}

void Camera::rotateVertical(const float _angle)
{
	verticalAngle += _angle;
	transformed = true;
}

glm::vec3 Camera::rotateVec(const glm::vec3& _vec)
{
	return glm::mat3(invViewMatrix) * _vec;
}

void Camera::update()
{
	if (parentObject != 0)
	{
		position = glm::vec3(parentObject->getTransform() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) + parentOffset;
		glm::vec3 v = glm::normalize(glm::vec3(parentObject->getTransform() * glm::vec4(0.0f, 0.0f, 1.0f, 0.0f)));
		
		if (inheritParentRotation)
		{
			verticalAngle = glm::asin(v.y) + vAdd;
			glm::vec3 up = glm::vec3(0.0f, 1.0f, 0.0f);
			glm::vec3 front = glm::vec3(0.0f, 0.0f, 1.0f);
			glm::vec3 vProj = glm::normalize(v - up * glm::dot(v, up));
			horizontalAngle = glm::acos(glm::dot(front, vProj)) + hAdd;
		}

		//std::cout << "v = [" << v.x << "," << v.y << "," << v.z << "] VerticalAngle = " << verticalAngle << " HorizontalAngle = " << horizontalAngle << std::endl;
		transformed = true;
	}

	if (transformed)
	{
		viewVector = glm::normalize(glm::vec3(glm::cos(verticalAngle) * glm::sin(horizontalAngle), glm::sin(verticalAngle), glm::cos(verticalAngle) * glm::cos(horizontalAngle)));
		glm::vec3 rightVector = glm::normalize(glm::vec3(glm::sin(horizontalAngle - PI_2_F), 0.0f, glm::cos(horizontalAngle - PI_2_F)));
		glm::vec3 upVector = glm::normalize(glm::vec3(glm::cross(rightVector, viewVector)));

		viewMatrix = glm::lookAt(position, position + viewVector, upVector);
		invViewMatrix = glm::inverse(viewMatrix);
		viewProjectionMatrix = projectionMatrix * viewMatrix;
		invViewProjectionMatrix = glm::inverse(viewProjectionMatrix);

		//Update frustum
		glm::vec3 nc = position + viewVector * near;
		glm::vec3 fc = position + viewVector * far;

		frustumPlanes[Plane::FRONT].setNormalAndPoint(viewVector, nc);
		frustumPlanes[Plane::BACK].setNormalAndPoint(-viewVector, fc);

		glm::vec3 point = nc + rightVector * widthNear;
		glm::vec3 normal = glm::normalize(glm::cross(upVector, glm::normalize(point - position)));
		frustumPlanes[Plane::RIGHT].setNormalAndPoint(normal, point);

		point = nc - rightVector * widthNear;
		normal = glm::normalize(glm::cross(glm::normalize(point - position), upVector));
		frustumPlanes[Plane::LEFT].setNormalAndPoint(normal, point);

		point = nc + upVector * heightNear;
		normal = glm::normalize(glm::cross(glm::normalize(point - position), rightVector));
		frustumPlanes[Plane::TOP].setNormalAndPoint(normal, point);

		point = nc - upVector * heightNear;
		normal = glm::normalize(glm::cross(rightVector, glm::normalize(point - position)));
		frustumPlanes[Plane::BOTTOM].setNormalAndPoint(normal, point);

		transformed = false;
	}
}

void Camera::updateResolution(const float _width, const float _height)
{
	width = _width;
	height = _height;
	aspectRatio = _width / _height;
	heightNear = glm::tan(glm::radians(fov) / 2.0f) * near;
	heightFar = glm::tan(glm::radians(fov) / 2.0f) * far;
	widthNear = heightNear * aspectRatio;
	widthFar = heightFar * aspectRatio;

	projectionMatrix = glm::perspectiveFov(fov, _width, _height, near, far);
	invProjectionMatrix = glm::inverse(projectionMatrix);

	transformed = true;
}