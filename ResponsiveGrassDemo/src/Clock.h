/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef CLOCK_H
#define CLOCK_H

#include <ctime>

class Clock
{
private:
	long long startTime, lastTickTime, tickTime, timerFrequency;
	double absoluteTime, lastFrameTime;
public:
	Clock();
	~Clock();
	void Tick();

	bool hasFixedTime = false;
	double fixedFrameTime = 1.0 / 60.0;

	double AbsoluteTime() const;
	double LastFrameTime() const;
};

#endif