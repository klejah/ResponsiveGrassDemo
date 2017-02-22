/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef FPSCOUNTER_H
#define FPSCOUNTER_H

class FPSCounter
{
private:
	double time;
	unsigned int fps, curAmountFrames;
public:
	FPSCounter();
	~FPSCounter();

	void update(double dt);

	unsigned int FPS() const;
};

#endif