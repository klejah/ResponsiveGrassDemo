/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include <iostream>
#include <sstream>

#include "GL\glew.h"
#include "GLFW\glfw3.h"

#include "Common.h"

#include "DemoScene.h"
#include "OpenGLState.h"

DemoScene * scene = 0;

#define MAKE_FULLSCREEN false

void reshape(GLFWwindow* window, int width, int height)
{
	glViewport(0, 0, (GLsizei)width, (GLsizei)height);
	if(scene != 0)
		scene->reshape(width, height);
}

void window_key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_ESCAPE && action == GLFW_PRESS)
	{
		glfwSetWindowShouldClose(window, GL_TRUE);
	}

	if (scene != 0)
	{
		scene->key_callback(window, key, scancode, action, mods);
	}
}

std::string FormatDebugOutput(GLenum source, GLenum type, GLuint id, GLenum severity, const char* msg) {
	std::stringstream stringStream;
	std::string sourceString;
	std::string typeString;
	std::string severityString;

	switch (source) {
	case GL_DEBUG_CATEGORY_API_ERROR_AMD:
	case GL_DEBUG_SOURCE_API: {
								  sourceString = "API";
								  break;
	}
	case GL_DEBUG_CATEGORY_APPLICATION_AMD:
	case GL_DEBUG_SOURCE_APPLICATION: {
										  sourceString = "Application";
										  break;
	}
	case GL_DEBUG_CATEGORY_WINDOW_SYSTEM_AMD:
	case GL_DEBUG_SOURCE_WINDOW_SYSTEM: {
											sourceString = "Window System";
											break;
	}
	case GL_DEBUG_CATEGORY_SHADER_COMPILER_AMD:
	case GL_DEBUG_SOURCE_SHADER_COMPILER: {
											  sourceString = "Shader Compiler";
											  break;
	}
	case GL_DEBUG_SOURCE_THIRD_PARTY: {
										  sourceString = "Third Party";
										  break;
	}
	case GL_DEBUG_CATEGORY_OTHER_AMD:
	case GL_DEBUG_SOURCE_OTHER: {
									sourceString = "Other";
									break;
	}
	default: {
				 sourceString = "Unknown";
				 break;
	}
	}

	switch (type) {
	case GL_DEBUG_TYPE_ERROR: {
								  typeString = "Error";
								  break;
	}
	case GL_DEBUG_CATEGORY_DEPRECATION_AMD:
	case GL_DEBUG_TYPE_DEPRECATED_BEHAVIOR: {
												typeString = "Deprecated Behavior";
												break;
	}
	case GL_DEBUG_CATEGORY_UNDEFINED_BEHAVIOR_AMD:
	case GL_DEBUG_TYPE_UNDEFINED_BEHAVIOR: {
											   typeString = "Undefined Behavior";
											   break;
	}
	case GL_DEBUG_TYPE_PORTABILITY_ARB: {
											typeString = "Portability";
											break;
	}
	case GL_DEBUG_CATEGORY_PERFORMANCE_AMD:
	case GL_DEBUG_TYPE_PERFORMANCE: {
										typeString = "Performance";
										break;
	}
	case GL_DEBUG_CATEGORY_OTHER_AMD:
	case GL_DEBUG_TYPE_OTHER: {
								  typeString = "Other";
								  break;
	}
	default: {
				 typeString = "Unknown";
				 break;
	}
	}

	switch (severity) {
	case GL_DEBUG_SEVERITY_HIGH: {
									 severityString = "High";
									 break;
	}
	case GL_DEBUG_SEVERITY_MEDIUM: {
									   severityString = "Medium";
									   break;
	}
	case GL_DEBUG_SEVERITY_LOW: {
									severityString = "Low";
									break;
	}
	default: {
				 severityString = "Unknown";
				 break;
	}
	}

	stringStream << "OpenGL Error: " << msg;
	stringStream << " [Source = " << sourceString;
	stringStream << ", Type = " << typeString;
	stringStream << ", Severity = " << severityString;
	stringStream << ", ID = " << id << "]";

	return stringStream.str();
}

void APIENTRY DebugCallback(GLenum source, GLenum type, GLuint id, GLenum severity, GLsizei length, const GLchar* message, const GLvoid* userParam) {
	switch (id)
	{
	case 131185:
		return;
	case 131186:
		return;
	default:
		std::string error = FormatDebugOutput(source, type, id, severity, message);
		std::cout << error << std::endl;
		break;
	}
}

int main(void)
{
	//glfw
	GLFWwindow* window;
	int width, height;

	if (!glfwInit())
	{
		std::cout << "Failed initialize glfw\n";
	}

	glfwWindowHint(GLFW_DEPTH_BITS, 16);
	glfwWindowHint(GLFW_SAMPLES, MS_SAMPLES);

#if _DEBUG
	glfwWindowHint(GLFW_OPENGL_DEBUG_CONTEXT, GL_TRUE);
#endif

	if (MAKE_FULLSCREEN)
	{
		GLFWmonitor* monitor = glfwGetPrimaryMonitor();
		int refreshrate = 60;
		glfwWindowHint(GLFW_REFRESH_RATE, refreshrate);
		window = glfwCreateWindow(1024, 768, "GraphicsTestV2", monitor, NULL);
	}
	else
	{
		window = glfwCreateWindow(1024, 768, "GraphicsTestV2", NULL, NULL);
	}
	if (!window)
	{
		glfwTerminate();
		std::cout << "Failed to setup window\n";
	}

	glfwSetFramebufferSizeCallback(window, reshape);
	glfwSetKeyCallback(window, window_key_callback);

	glfwMakeContextCurrent(window);
	glfwSwapInterval(0);

	glfwGetFramebufferSize(window, &width, &height);
	reshape(window, width, height);

	glfwSetTime(0.0);

	//glew
	glewExperimental = GL_TRUE;
	GLenum error = glewInit();
	if (error != GLEW_OK)
	{
		std::cout << "Failed initialize glew\n";
	}
	glGetError();

	std::string glVersion = std::string((const char*)glGetString(GL_VERSION));
	std::cout << "OpenGL Contex Version " << glVersion << std::endl;

#if _DEBUG
	glDebugMessageCallback((GLDEBUGPROC)DebugCallback, 0);
	OpenGLState::Instance().enable(GL_DEBUG_OUTPUT_SYNCHRONOUS);
#endif

	const GLubyte* glvendor = glGetString(GL_VENDOR);
	const GLubyte* glrenderer = glGetString(GL_RENDERER);
	std::cout << "Vendor: " << glvendor << std::endl;
	std::cout << "Renderer: " << glrenderer << std::endl;

	//Scene
	scene = new DemoScene(window, width, height);

	scene->execute();

	glfwTerminate();

	system("pause");
}