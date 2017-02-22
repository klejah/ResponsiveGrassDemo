/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef GLCLOCK_
#define GLCLOCK_

#include "Common.h"

class GLClock
{
public:
	GLClock();
	~GLClock();

	void Start();
	void Stop();
	double Time();
private:
	GLuint64 timer, timer2;
	GLuint queries[2];
	bool hasEverStarted = false;
	bool hasEverStopped = false;
	bool isRunning = false;
};

#endif