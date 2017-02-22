/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "Camera.h"

inline void panLeft(Camera& cam, float speed, float dt)
{
	cam.translate(cam.rotateVec(glm::vec3(-speed * dt, 0.0f, 0.0f)));
}

inline void panRight(Camera& cam, float speed, float dt)
{
	cam.translate(cam.rotateVec(glm::vec3(speed * dt, 0.0f, 0.0f)));
}

inline void panUp(Camera& cam, float speed, float dt)
{
	cam.translate(cam.rotateVec(glm::vec3(0.0f, speed * dt, 0.0f)));
}

inline void panDown(Camera& cam, float speed, float dt)
{
	cam.translate(cam.rotateVec(glm::vec3(0.0f, -speed * dt, 0.0f)));
}

inline void panFront(Camera& cam, float speed, float dt)
{
	cam.translate(cam.rotateVec(glm::vec3(0.0f, 0.0f, -speed * dt)));
}

inline void panBack(Camera& cam, float speed, float dt)
{
	cam.translate(cam.rotateVec(glm::vec3(0.0f, 0.0f, speed * dt)));
}

inline void rotateLeft(Camera& cam, float speed, float dt)
{
	cam.rotateHorizontal(speed * dt);
}

inline void rotateRight(Camera& cam, float speed, float dt)
{
	cam.rotateHorizontal(-speed * dt);
}

inline void lookUp(Camera& cam, float speed, float dt)
{
	cam.rotateVertical(speed * dt);
}

inline void lookDown(Camera& cam, float speed, float dt)
{
	cam.rotateVertical(-speed * dt);
}