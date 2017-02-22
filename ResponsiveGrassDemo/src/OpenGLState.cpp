/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "OpenGLState.h"

OpenGLState::OpenGLState() : state(), shader(), wireFrame(false), patchVertices(1)
{
	//Default state for glEnable (not glEnablei)
	state.insert(std::pair<unsigned int, bool>(GL_BLEND,false));
	state.insert(std::pair<unsigned int, bool>(GL_COLOR_LOGIC_OP, false));
	state.insert(std::pair<unsigned int, bool>(GL_CULL_FACE, false));
	state.insert(std::pair<unsigned int, bool>(GL_DEBUG_OUTPUT, false));
	state.insert(std::pair<unsigned int, bool>(GL_DEBUG_OUTPUT_SYNCHRONOUS, false));
	state.insert(std::pair<unsigned int, bool>(GL_DEPTH_CLAMP, false));
	state.insert(std::pair<unsigned int, bool>(GL_DEPTH_TEST, false));
	state.insert(std::pair<unsigned int, bool>(GL_DITHER, true));
	state.insert(std::pair<unsigned int, bool>(GL_FRAMEBUFFER_SRGB, false));
	state.insert(std::pair<unsigned int, bool>(GL_LINE_SMOOTH, false));
	state.insert(std::pair<unsigned int, bool>(GL_MULTISAMPLE, true));
	state.insert(std::pair<unsigned int, bool>(GL_POLYGON_OFFSET_FILL, false));
	state.insert(std::pair<unsigned int, bool>(GL_POLYGON_OFFSET_LINE, false));
	state.insert(std::pair<unsigned int, bool>(GL_POLYGON_OFFSET_POINT, false));
	state.insert(std::pair<unsigned int, bool>(GL_POLYGON_SMOOTH, false));
	state.insert(std::pair<unsigned int, bool>(GL_PRIMITIVE_RESTART, false));
	state.insert(std::pair<unsigned int, bool>(GL_PRIMITIVE_RESTART_FIXED_INDEX, false));
	state.insert(std::pair<unsigned int, bool>(GL_RASTERIZER_DISCARD, false));
	state.insert(std::pair<unsigned int, bool>(GL_SAMPLE_ALPHA_TO_COVERAGE, false));
	state.insert(std::pair<unsigned int, bool>(GL_SAMPLE_ALPHA_TO_ONE, false));
	state.insert(std::pair<unsigned int, bool>(GL_SAMPLE_COVERAGE, false));
	state.insert(std::pair<unsigned int, bool>(GL_SAMPLE_SHADING, false));
	state.insert(std::pair<unsigned int, bool>(GL_SAMPLE_MASK, false));
	state.insert(std::pair<unsigned int, bool>(GL_SCISSOR_TEST, false));
	state.insert(std::pair<unsigned int, bool>(GL_STENCIL_TEST, false));
	state.insert(std::pair<unsigned int, bool>(GL_TEXTURE_CUBE_MAP_SEAMLESS, false));
	state.insert(std::pair<unsigned int, bool>(GL_PROGRAM_POINT_SIZE, false));
	glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	glPatchParameteri(GL_PATCH_VERTICES, 1);
}

OpenGLState::~OpenGLState()
{

}

OpenGLState* OpenGLState::instance = 0;

OpenGLState& OpenGLState::Instance()
{
	if (instance == 0)
	{
		instance = new OpenGLState();
	}
	return *instance;
}

void OpenGLState::enable(const unsigned int value)
{
	if (!state[value])
	{
		state[value] = true;
		glEnable(value);
	}
}

void OpenGLState::disable(const unsigned int value)
{
	if (state[value])
	{
		state[value] = false;
		glDisable(value);
	}
}

void OpenGLState::bindShader(const int handle)
{
	if (boundShader != handle && handle != -1)
	{
		shader[boundShader] = false;
		shader[handle] = true;
		boundShader = handle;
		glUseProgram(handle);
	}
}

void OpenGLState::unbindShader()
{
	if (boundShader != -1)
	{
		shader[boundShader] = false;
		boundShader = -1;
	}
}

void OpenGLState::registerShader(const int handle)
{
	if (shader.find(handle) != shader.end())
	{
		shader.insert(std::pair<int, bool>(handle, false));
	}
}

void OpenGLState::toggleWireframe()
{
	if (wireFrame)
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_FILL);
	}
	else
	{
		glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
	}
	wireFrame = !wireFrame;
}

void OpenGLState::setPatchVertices(const int value)
{
	if (value != patchVertices && value > 0 && value < GL_MAX_PATCH_VERTICES)
	{
		patchVertices = value;
		glPatchParameteri(GL_PATCH_VERTICES, value);
	}
}