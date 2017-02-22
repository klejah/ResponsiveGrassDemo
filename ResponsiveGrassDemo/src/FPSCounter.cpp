/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "FPSCounter.h"

FPSCounter::FPSCounter() : time(0.0), fps(0), curAmountFrames(0)
{

}

FPSCounter::~FPSCounter()
{

}

void FPSCounter::update(double dt)
{
	curAmountFrames++;
	time += dt;

	if (time > 1.0)
	{
		fps = curAmountFrames;
		curAmountFrames = 0;
		time = 0.0;
	}
}

unsigned int FPSCounter::FPS() const
{
	return fps;
}