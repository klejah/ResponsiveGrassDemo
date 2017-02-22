/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "GLClock.h"

GLClock::GLClock() : hasEverStarted(false), hasEverStopped(false), isRunning(false)
{
	glGenQueries(2, queries);
}

GLClock::~GLClock()
{
	glDeleteQueries(2, queries);
}

void GLClock::Start()
{
	glQueryCounter(queries[0], GL_TIMESTAMP);
	hasEverStarted = true;
	isRunning = true;
}

void GLClock::Stop()
{
	glQueryCounter(queries[1], GL_TIMESTAMP);
	hasEverStopped = true;
	isRunning = false;
}

double GLClock::Time()
{
	if (hasEverStarted && hasEverStopped && !isRunning)
	{
		GLint stopTimerAvailable = 0;
		while (!stopTimerAvailable)
			glGetQueryObjectiv(queries[1], GL_QUERY_RESULT_AVAILABLE, &stopTimerAvailable);


		glGetQueryObjectui64v(queries[0], GL_QUERY_RESULT, &timer);
		glGetQueryObjectui64v(queries[1], GL_QUERY_RESULT, &timer2);

		return ((double)(timer2 - timer) / 1000000.0);
	}
	else
	{
		return 0.0f;
	}
}