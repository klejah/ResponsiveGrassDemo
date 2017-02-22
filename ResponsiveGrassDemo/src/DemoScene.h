/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef DemoScene_H
#define DemoScene_H

#include "Common.h"
#include "GLFW\glfw3.h"
#include "Clock.h"

#include "Camera.h"
#include "FontRenderer.h"
#include "FPSCounter.h"
#include "PhysXController.h"
#include "SceneObject.h"
#include "Shader.h"
#include "SpherePackedObject.h"
#include "Skybox.h"

#include "Grass.h"
#include "WindGenerator.h"
#include "GrassObject.h"
#include "HeightMap.h"
#include "AutoTransformer.h"
#include <vector>

class DemoScene
{
private:
	Camera* cam;
	FontRenderer* fontRenderer;
	FPSCounter fpsCounter;

	GLFWwindow* window;
	unsigned int width, height;
	Clock time;

	std::vector<WindGenerator*> windGenerator;
	PhysXController& physic;

	std::vector<Texture2D*> textures;
	std::vector<Texture2D*> ballTextures;
	std::vector<SceneObject*> sceneObjects;
	std::vector<SceneObject*> balls;
	std::vector<GrassObject*> grassObjects;
	std::vector<Grass*> grassFields;
	std::vector<SpherePackedObject*> spherePackedObjects;
	std::vector<glm::vec4> innerSphereList;
	std::vector<glm::vec4> colliderList;
	std::vector<AutoTransformer*> transformer;
	bool animationStarted = false;
	std::vector<HeightMap*> heightMaps;
	unsigned int heightMapObjectIndex = -1;
	physx::PxRigidStatic* heightFieldActor = 0;

	Shader* postprocessShader;
	Shader* ballShader;
	Shader* allInOneTessellationShader;
	std::vector<SceneObjectGeometry*> ballGeometry;

	bool drawFont = true;
	bool showDepth = false;
	
	float power = 1.0f;

	bool doScreenshot = false;
	unsigned int screenshotid = 0;

	GLuint ppvao;
	GLuint fbo;
	GLuint fboDepthBuffer;
	Texture2D* fboColorTex;
	Texture2D* fboDepthTex;
	Skybox* skybox = 0;
	bool drawSkybox = true;

	void loadScene(unsigned int id);
public:
	DemoScene(GLFWwindow* _window, unsigned int _width, unsigned int _height);
	~DemoScene();

	virtual void execute();
	virtual void checkKeys(GLFWwindow* window);
	virtual void key_callback(GLFWwindow* window, int key, int scancode, int action, int mods);

	void reshape(unsigned int width, unsigned int heigth);
};

#endif