/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef AUTO_MOVER_H
#define AUTO_MOVER_H

#include "AutoTransformer.h"
#include <vector>

class AutoMover : public AutoTransformer
{
public:
	double s;
	std::vector<glm::vec3> p;
	unsigned int nextPoint;
	unsigned int curPoint;
	double maxTime;
	double timeLeft;
	glm::mat4 rotation;
	bool loop, finished;

	AutoMover(SceneObject& obj, const double delay, const double speed, const std::vector<glm::vec3>& points, bool loop);
	~AutoMover();
	
	void update(const Clock& time);
};

#endif