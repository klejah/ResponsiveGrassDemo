/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef CAMERA_H
#define CAMERA_H

#include "Common.h"
#include "Plane.h"

class SceneObject;

class Camera
{
public:
	glm::vec3 position, viewVector;
	float horizontalAngle, verticalAngle;
	float hAdd, vAdd;
	Plane frustumPlanes[6];
	float fov, aspectRatio, near, far, widthNear, heightNear, widthFar, heightFar, width, height;
	bool transformed;
	SceneObject* parentObject;
	glm::vec3 parentOffset;
	bool inheritParentRotation;
public:
	glm::mat4 viewMatrix, invViewMatrix, projectionMatrix, invProjectionMatrix, viewProjectionMatrix, invViewProjectionMatrix;

	Camera(const float _fov, const float _width, const float _height, const float _near, const float _far, const glm::vec3& _startingPosition);
	~Camera();

	void translate(const glm::vec3& _translation);
	void rotateHorizontal(const float _angle);
	void rotateVertical(const float _angle);
	glm::vec3 rotateVec(const glm::vec3& _vec);

	void update();

	void updateResolution(const float _width, const float _height);
};

#endif