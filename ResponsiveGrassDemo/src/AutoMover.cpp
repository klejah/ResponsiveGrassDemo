/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "AutoMover.h"

#include "glm/gtx/transform.hpp"

AutoMover::AutoMover(SceneObject& obj, const double delay, const double speed, const std::vector<glm::vec3>& points, bool loop) : AutoTransformer(obj, delay), s(speed), p(points), nextPoint(0), curPoint(0), maxTime(1.0f), timeLeft(0.0f), loop(loop), finished(false)
{
	if (points.size() == 0)
	{
		p.push_back(glm::vec3(0.0f, 0.0f, 0.0f));
	}
	rotation = glm::mat4(glm::mat3(obj.getTransform()));
	obj.setPosition(glm::translate(glm::mat4(1.0f), p[0]) * rotation);
}

AutoMover::~AutoMover()
{
}

void AutoMover::update(const Clock& time)
{
	if (enabled && !finished)
	{
		if (delayTime > 0.0)
		{
			delayTime -= time.LastFrameTime();
			return;
		}

		timeLeft -= time.LastFrameTime();
		glm::vec3 pos;
		if (timeLeft <= 0.0)
		{
			pos = p[nextPoint];
			curPoint = nextPoint;
			nextPoint++;
			if (nextPoint == p.size())
			{
				nextPoint = 0;
				if (!loop)
				{
					finished = true;
				}
			}
			double distance = glm::distance(p[curPoint], p[nextPoint]);
			maxTime = distance / s;
			timeLeft = maxTime;
			std::cout << "Point reached" << std::endl;
		}
		else
		{
			pos = glm::mix(p[curPoint], p[nextPoint], (float)(1.0 - (timeLeft / maxTime)));
		}
		
		rotation = glm::mat4(glm::mat3(obj.getTransform()));
		obj.setPosition(glm::translate(glm::mat4(1.0f), pos) * rotation);
	}
}