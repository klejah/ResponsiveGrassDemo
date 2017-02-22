/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef OPENGLSTATE_H
#define OPENGLSTATE_H

#include "Common.h"
#include <map>

class OpenGLState
{
public:
	static OpenGLState& Instance();

	void enable(const unsigned int value);
	void disable(const unsigned int value);
	void bindShader(const int name);
	void unbindShader();
	void registerShader(const int handle);
	void toggleWireframe();
	void setPatchVertices(const int value);

	const bool isWireframe() const { return wireFrame; }
private:
	OpenGLState();
	~OpenGLState();

	static OpenGLState* instance;

	std::map<unsigned int, bool> state;
	std::map<int, bool> shader;
	int boundShader;
	bool wireFrame;
	int patchVertices;
};
#endif