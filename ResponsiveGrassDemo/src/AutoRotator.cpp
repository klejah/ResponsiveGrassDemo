/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "AutoRotator.h"
#include <glm/gtx/transform.hpp>

AutoRotator::AutoRotator(SceneObject& obj, const double delay, const float speed, const glm::vec3& rotationAxis) : AutoTransformer(obj, delay), s(speed), axis(rotationAxis), t(0), maxTime(0)
{
}

AutoRotator::~AutoRotator()
{
}

void AutoRotator::update(const Clock& time)
{
	if (enabled && (maxTime == 0.0 || t < maxTime))
	{
		if (delayTime > 0.0)
		{
			delayTime -= time.LastFrameTime();
			return;
		}

		t += time.LastFrameTime();

		obj.setPosition(glm::rotate(obj.getTransform(), s * (float)time.LastFrameTime(), axis));
	}
}