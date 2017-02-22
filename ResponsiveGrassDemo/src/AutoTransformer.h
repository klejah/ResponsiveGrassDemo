/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef AUTO_TRANSFORMER_H
#define AUTO_TRANSFORMER_H

#include "Common.h"
#include "Clock.h"
#include "SceneObject.h"

class AutoTransformer
{
public:
	SceneObject& obj;
	bool enabled;
	double delayTime;

	AutoTransformer(SceneObject& object, const double delay) : obj(object), enabled(false), delayTime(delay) {}
	~AutoTransformer(){}

	virtual void update(const Clock& time) = 0;
};

#endif