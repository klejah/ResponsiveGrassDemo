/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef AUTO_ROTATOR_H
#define AUTO_ROTATOR_H

#include "AutoTransformer.h"

class AutoRotator : public AutoTransformer
{
public:
	float s;
	double t;
	double maxTime;
	glm::vec3 axis;

	AutoRotator(SceneObject& obj, const double delay, const float speed, const glm::vec3& rotationAxis);
	~AutoRotator();

	void update(const Clock& time);
};

#endif