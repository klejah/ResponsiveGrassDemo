/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "WindGenerator.h"
#include <iostream>

float WindGenerator::maxMagnitude = 8.0f;

WindGenerator::WindGenerator(const float _minFrequencyDir, const float _maxFrequencyDir, const float _minFrequencyMag, const float _maxFrequencyMag) : maxFrequencyDir(_maxFrequencyDir), minFrequencyDir(_minFrequencyDir), frequencyDir(0), shiftPeriodDir(0), maxFrequencyMag(_maxFrequencyMag), minFrequencyMag(_minFrequencyMag), frequencyMag(0), shiftPeriodMag(0), newDir(1.0f, 0.0f, 0.0f), newPos(0.0f), magnitude(0.5f), newMagnitude(0.5f), wave(0.0f), type(WindType::VECTOR), parentObject(0), parentObjectOffset(0.0f)
{

}

WindGenerator::~WindGenerator()
{

}

void WindGenerator::update(const double dt)
{
	shiftPeriodDir += dt;
	shiftPeriodMag += dt;
	wave += (float)dt;

	switch (type)
	{
	case WindType::VECTOR:
		if (shiftPeriodDir >= frequencyDir)
		{
			shiftPeriodDir = 0;
			frequencyDir = minFrequencyDir + random() * (maxFrequencyDir - minFrequencyDir);
			float phi = random() * PI_F * 2.0f;
			newDir = glm::normalize(glm::vec3(glm::sin(phi), random() - 0.5f, glm::cos(phi)));
			wind = newDir * newMagnitude;
		}

		if (shiftPeriodMag >= frequencyMag)
		{
			shiftPeriodMag = 0;
			frequencyMag = minFrequencyMag + random() * (maxFrequencyMag - minFrequencyMag);
			newMagnitude = random() * maxMagnitude;
			wind = newDir * newMagnitude;
		}
		break;
	case WindType::POINT:
	case WindType::POINTWITHTANGENTIAL:
		if (parentObject != 0)
		{
			wind = glm::vec3(parentObject->getTransform() * glm::vec4(0.0f, 0.0f, 0.0f, 1.0f)) + parentObjectOffset;
		}
		else
		{
			float d = glm::distance(wind, newPos);
			if (d < 0.05f)
			{
				newPos = glm::vec3(0.0f);
			}
		}
		break;
	}

	windData = glm::vec4(wind, wave);
}

void WindGenerator::resetWind()
{
	wind = glm::vec3(0.0f, 0.0f, 0.0f);
	shiftPeriodDir = FLT_MAX;
	shiftPeriodMag = FLT_MAX;
	wave = 0.0f;
}

const glm::vec4& WindGenerator::getWindData()
{
	return windData;
}