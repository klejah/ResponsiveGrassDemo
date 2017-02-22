/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "Clock.h"
#include <Windows.h>

Clock::Clock() : startTime(0), lastTickTime(0), absoluteTime(0.0), lastFrameTime(0.0), tickTime(0)
{
	QueryPerformanceFrequency((LARGE_INTEGER*)&timerFrequency);
	QueryPerformanceCounter((LARGE_INTEGER*)&startTime);
	QueryPerformanceCounter((LARGE_INTEGER*)&lastTickTime);

	//set seed for rand calls
	srand((unsigned int)startTime);
}

Clock::~Clock()
{

}

void Clock::Tick()
{
	lastTickTime = tickTime;
	QueryPerformanceCounter((LARGE_INTEGER*)&tickTime);

	absoluteTime = (double)(tickTime - startTime) / (double)timerFrequency;
	lastFrameTime = (double)(tickTime - lastTickTime) / (double)timerFrequency;
}

double Clock::AbsoluteTime() const
{
	return absoluteTime;
}

double Clock::LastFrameTime() const
{
	if (!hasFixedTime)
		return lastFrameTime;
	return fixedFrameTime;
}