/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef WINDGENERATOR_H
#define WINDGENERATOR_H

#include "Common.h"
#include "SceneObject.h"

enum WindType 
{
	VECTOR = 0, POINT = 1, POINTWITHTANGENTIAL = 2
};

class WindGenerator
{
public:
	WindGenerator(const float minFrequencyDir, const float maxFrequencyDir, const float minFrequencyMag, const float maxFrequencyMag);
	~WindGenerator();

	void update(const double dt);
	const glm::vec4& getWindData();
	void setWindType(WindType newType) { type = newType; }
	void resetWind();
	WindType getWindType() { return type; }

	static float maxMagnitude;
	SceneObject* parentObject;
	glm::vec3 parentObjectOffset;
private:
	float minFrequencyDir, maxFrequencyDir, frequencyDir, minFrequencyMag, maxFrequencyMag, frequencyMag;
	double shiftPeriodDir, shiftPeriodMag;

	glm::vec3 newDir, wind, newPos;
	float magnitude, newMagnitude;
	float wave;
	glm::vec4 windData;
	WindType type;
};

#endif