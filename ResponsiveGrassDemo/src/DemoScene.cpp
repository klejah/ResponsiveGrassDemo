/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "DemoScene.h"

#include "CameraTransforms.h"
#include <glm/gtc/matrix_transform.hpp>
#include "OpenGLState.h"

#include "AutoMover.h"
#include "AutoRotator.h"

#include <iostream>

#include <sys\stat.h>
#include <direct.h>

//Helper Functions declared here, defined at the end of file
void createHeightField(PhysXController& physic, const HeightMap& heightMap, const SceneObject& object, const glm::vec2 objectSizeXZ, PxRigidStatic*& heightFieldActor);
void createHeightField(PhysXController& physic, const std::vector<Geometry::TriangleFace>& faces, const SceneObject& object, const glm::vec2 objectSizeXZ, PxRigidStatic*& heightFieldActor);
std::string IDtoScreenshotName(const unsigned int id, const bool text);
void takeScreenShot(const std::string& path, const std::string& name, const unsigned int width, const unsigned int height);
Skybox* loadSkybox(const std::string& skybox);

#define BALLSIZE 1.5f

#define SCENE 0


DemoScene::DemoScene(GLFWwindow* _window, unsigned int _width, unsigned int _height) : window(_window), width(_width), height(_height), 
	fpsCounter(), time(), windGenerator(), cam(0), physic(PhysXController::Instance()),
	textures(), sceneObjects(), balls(), grassObjects(), spherePackedObjects(), grassFields(), heightMaps(),
	innerSphereList(), colliderList(), 
	ballShader(0), allInOneTessellationShader(0), ballGeometry(0)
{
	windGenerator.push_back(new WindGenerator(5.0f, 8.0f, 1.0f, 2.0f));
	fontRenderer = new FontRenderer(RESSOURCEPATH + "Font/consola.ttf", 12, width, height);

	cam = new Camera(glm::radians(60.0f), (float)width, (float)height, 0.1f, 200.0f, glm::vec3(0.0f, 10.0f, 50.0f));
	cam->rotateHorizontal(glm::radians(0.0f));
	cam->rotateVertical(glm::radians(0.0f));

	//Seek current screenshotid
	std::string name = GENERATEDFILESPATH + "Screenshots/Screenshot000.png";
	screenshotid = 0;
	struct stat buffer;
	while (stat(name.c_str(), &buffer) == 0)
	{
		screenshotid++;
		if (screenshotid < 10)
		{
			name[name.size() - 5] = '0' + (char)screenshotid;
		}
		else if (screenshotid < 100)
		{
			name[name.size() - 5] = '0' + (char)(screenshotid % 10);
			name[name.size() - 6] = '0' + (char)(screenshotid / 10);
		}
		else
		{
			name[name.size() - 5] = '0' + (char)(screenshotid % 10);
			name[name.size() - 6] = '0' + (char)((screenshotid % 100) / 10);
			name[name.size() - 7] = '0' + (char)(screenshotid / 100);
		}
	}
	std::cout << "File " << name << " is availlable." << std::endl;

	ballTextures.push_back(Texture2D::loadTextureFromFile(TEXTUREPATH + "Ball/btex1.png", true, true, false, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));
	ballTextures.push_back(Texture2D::loadTextureFromFile(TEXTUREPATH + "Ball/ftex1.png", true, true, false, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));
	ballTextures.push_back(Texture2D::loadTextureFromFile(TEXTUREPATH + "Ball/ttex1.png", true, true, false, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));
	ballTextures.push_back(Texture2D::loadTextureFromFile(TEXTUREPATH + "Ball/otex1.png", true, true, false, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));

	std::vector<std::string> s;
	std::vector<std::string> r;
	s.push_back(               SHADER_POSITION_STRING);
	r.push_back(std::to_string(SHADER_POSITION_LOCATION));
	s.push_back(               SHADER_NORMAL_STRING);
	r.push_back(std::to_string(SHADER_NORMAL_LOCATION));
	s.push_back(               SHADER_UV_STRING);
	r.push_back(std::to_string(SHADER_UV_LOCATION));
	s.push_back(               SHADER_TANGENT_STRING);
	r.push_back(std::to_string(SHADER_TANGENT_LOCATION));
	s.push_back(               SHADER_BITANGENT_STRING);
	r.push_back(std::to_string(SHADER_BITANGENT_LOCATION));
	ballShader = new Shader(SHADERPATH + "Objects/BallShader", s, r);
	allInOneTessellationShader = new Shader(SHADERPATH + "Objects/AllInOneTessellationShader", s, r);
	ballGeometry.push_back(new SceneObjectGeometry(MODELPATH + "BBall.ply", SceneObjectGeometry::BasicGeometry::SPHERE, glm::vec3(BALLSIZE, BALLSIZE, BALLSIZE), false, true, 0, true, 0.4f, 0.4f, 0.8f, 0.4f, 12.0f));
	ballGeometry.push_back(new SceneObjectGeometry(MODELPATH + "FBall.ply", SceneObjectGeometry::BasicGeometry::SPHERE, glm::vec3(BALLSIZE, BALLSIZE, BALLSIZE), false, true, 0, true, 0.5f, 0.5f, 0.6f, 0.5f, 10.0f));
	ballGeometry.push_back(new SceneObjectGeometry(MODELPATH + "TBall.ply", SceneObjectGeometry::BasicGeometry::SPHERE, glm::vec3(BALLSIZE, BALLSIZE, BALLSIZE), false, true, 0, true, 0.7f, 0.7f, 0.7f, 0.7f, 5.0f));
	ballGeometry.push_back(new SceneObjectGeometry(MODELPATH + "OBall.ply", SceneObjectGeometry::BasicGeometry::SPHERE, glm::vec3(BALLSIZE, BALLSIZE, BALLSIZE), false, true, 0, true, 0.3f, 0.3f, 0.01f, 0.2f, 15.0f));

	//PP VAO Setup
	glGenVertexArrays(1, &ppvao);

	//PP Shader
	postprocessShader = new Shader(SHADERPATH + "Misc/Postprocess");

	//FBO Setup
	glGenFramebuffers(1, &fbo);
	glGenRenderbuffers(1, &fboDepthBuffer);
	glBindRenderbuffer(GL_RENDERBUFFER, fboDepthBuffer);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, MS_SAMPLES, GL_DEPTH_COMPONENT, _width, _height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);
	fboColorTex = new Texture2D(GL_RGB, GL_RGB, false, true, _width, _height, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR, GL_UNSIGNED_BYTE, 0);
	fboDepthTex = new Texture2D(GL_R16, GL_RED, false, true, _width, _height, GL_REPEAT, GL_REPEAT, GL_LINEAR, GL_LINEAR, GL_UNSIGNED_BYTE, 0);

	glBindFramebuffer(GL_FRAMEBUFFER, fbo);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D_MULTISAMPLE, fboColorTex->Handle(), 0);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D_MULTISAMPLE, fboDepthTex->Handle(), 0);
	glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, fboDepthBuffer);
	GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
	if (status != GL_FRAMEBUFFER_COMPLETE)
	{
		std::cout << "Framebuffer cannot be set up" << std::endl;
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	loadScene(SCENE);

	//Grass
	GrassOvermind& over = GrassOvermind::getInstance();
	over.setGravity({ glm::vec4(0.0f, -1.0f, 0.0f, 1.0f), glm::vec4(0.0f, 0.0f, 0.0f, 1.0f), 0.0f });
	over.setMaxDistance(250.0f);
	over.colliderList = &colliderList;
	over.innerSphereList = &innerSphereList;
}

DemoScene::~DemoScene()
{

}

void DemoScene::execute()
{
	time.Tick();
	
	glClearColor(0.1f, 0.1f, 0.1f, 0.0f);

	double physicsTime = 0.0;
	double physicsTimestep = 1.0 / 60.0;
	double animTime = 0.0;

	while (true)
	{
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		glViewport(0, 0, width, height);
		glDrawBuffer(GL_COLOR_ATTACHMENT0);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
		glDrawBuffer(GL_COLOR_ATTACHMENT1);
		glClear(GL_COLOR_BUFFER_BIT);
		glClearColor(0.1f, 0.1f, 0.1f, 0.0f);
		GLenum buffer[2] = {GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1};
		glDrawBuffers(2, buffer);


		innerSphereList.clear();
		colliderList.clear();

		time.Tick();

		//update
		fpsCounter.update(time.LastFrameTime());
		cam->update();

		if (physicsTime >= physicsTimestep)
		{
			physic.update(physicsTime);
			physicsTime = 0.0;
		}
		else
		{
			physicsTime += time.LastFrameTime();
		}

		//std::cout << "=========== UPDATE ================" << std::endl;

		for (unsigned int i = 0; i < windGenerator.size(); i++)
		{
			windGenerator[i]->update(time.LastFrameTime());
		}

		for (unsigned int i = 0; i < transformer.size(); i++)
		{
			transformer[i]->update(time);
		}

		unsigned int visibleObjects = 0;

		for each (SceneObject* obj in sceneObjects)
		{
			obj->update(*cam,time);

			if (obj->isVisible() && obj->hasInnerSphere())
			{
				innerSphereList.push_back(obj->getInnerSphereVector());
			}

			if (obj->isVisible())
			{
				visibleObjects++;
			}
		}

		for each (SceneObject* obj in balls)
		{
			obj->update(*cam, time);

			if (obj->isVisible() && obj->hasInnerSphere())
			{
				innerSphereList.push_back(obj->getInnerSphereVector());
			}

			if (obj->isVisible())
			{
				visibleObjects++;
			}

			colliderList.push_back(glm::vec4(glm::vec3(obj->getTransform() * glm::vec4(0.0f,0.0f,0.0f,1.0f)), obj->getGeometryScale().x));
		}

		for each (GrassObject* obj in grassObjects)
		{
			obj->update(*cam, time);

			if (obj->isVisible() && obj->hasInnerSphere())
			{
				innerSphereList.push_back(obj->getInnerSphereVector());
			}
			if (obj->isVisible())
			{
				visibleObjects++;
			}
		}

		for each(SpherePackedObject* obj in spherePackedObjects)
		{
			obj->update(*cam, time);
			if (obj->isVisible())
			{
				visibleObjects++;
			}

			glm::mat4 mm = obj->getTransform();
			bool maxfound = false;
			for each(glm::vec4 s in obj->spheres)
			{
				if (s.w > 0.5f)
				{
					glm::vec4 pos = mm * glm::vec4(s.xyz, 1.0f);
					colliderList.push_back(glm::vec4(pos.xyz, s.w));
					if (!maxfound)
					{
						innerSphereList.push_back(glm::vec4(pos.xyz, s.w));
						maxfound = true;
					}
				}
			}
		}

		//draw
		if (OpenGLState::Instance().isWireframe())
		{
			OpenGLState::Instance().toggleWireframe();
			GLenum wirebuffer[2] = { GL_NONE, GL_COLOR_ATTACHMENT1 };
			glDrawBuffers(2, wirebuffer);
			for each (SceneObject* obj in sceneObjects)	{ obj->draw(*cam); }
			for each (SceneObject* obj in balls) { obj->draw(*cam); }
			for each (GrassObject* obj in grassObjects)	{ obj->draw(*cam); }
			for each (SpherePackedObject* obj in spherePackedObjects)	{ obj->draw(*cam); }

			OpenGLState::Instance().toggleWireframe();
			glDrawBuffer(GL_COLOR_ATTACHMENT0);
			glClear(GL_DEPTH_BUFFER_BIT);
		}

		for each (SceneObject* obj in sceneObjects)
		{
			obj->draw(*cam);
		}

		for each (SceneObject* obj in balls)
		{
			obj->draw(*cam);
		}

		for each (GrassObject* obj in grassObjects)
		{
			obj->draw(*cam);
		}

		for each (SpherePackedObject* obj in spherePackedObjects)
		{
			obj->draw(*cam);
		}

		///////////////////////////////////////////////////////
		///////////////////////////////////////////////////////
		//glDrawBuffer(GL_COLOR_ATTACHMENT0);
		for each(Grass* g in grassFields)
		{
			g->Draw((float)time.LastFrameTime(), *cam);
		}

		for each (GrassObject* obj in grassObjects)
		{
			obj->drawGrass(*cam, time);
		}
		///////////////////////////////////////////////////////
		///////////////////////////////////////////////////////

		if (skybox && drawSkybox)
		{
			bool toggle = false;
			if (!(SCENE == 3 && OpenGLState::Instance().isWireframe()))
			{
				skybox->render(*cam);
			}
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glViewport(0, 0, width, height);
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		glDrawBuffer(GL_BACK);

		if (showDepth)
		{
			glBindFramebuffer(GL_READ_FRAMEBUFFER, fbo);
			glReadBuffer(GL_COLOR_ATTACHMENT1);
			glBindFramebuffer(GL_DRAW_FRAMEBUFFER, 0);
			glBlitFramebuffer(0, 0, width, height, 0, 0, width, height, GL_COLOR_BUFFER_BIT, GL_LINEAR);
			glBindFramebuffer(GL_READ_FRAMEBUFFER, 0);
		}
		else
		{
			postprocessShader->bind();
			glBindVertexArray(ppvao);

			bool wireToggled = false;
			if (OpenGLState::Instance().isWireframe())
			{
				OpenGLState::Instance().toggleWireframe();
				wireToggled = true;
			}
			
			fboColorTex->bind(0);
			postprocessShader->setUniform("sourceTexture", (GLint)0);
			postprocessShader->setUniform("screenSize", glm::vec2(cam->width, cam->height));
			glDrawArrays(GL_POINTS, 0, 1);
			glBindVertexArray(0);
			postprocessShader->unbind();

			if (wireToggled)
			{
				OpenGLState::Instance().toggleWireframe();
			}
		}
		
		unsigned int fps = fpsCounter.FPS();
		double frameTime = time.LastFrameTime();

		if (doScreenshot)
		{
			takeScreenShot(GENERATEDFILESPATH + "Screenshots", IDtoScreenshotName(screenshotid, false), width, height);
		}

		if (drawFont)
		{
			bool wireToggled = false;
			if (OpenGLState::Instance().isWireframe())
			{
				OpenGLState::Instance().toggleWireframe();
				wireToggled = true;
			}
			fontRenderer->RenderString("FPS:" + std::to_string(fps), glm::vec2(0, 12), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			fontRenderer->RenderString("Frametime:" + std::to_string(frameTime), glm::vec2(0, 26), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			fontRenderer->RenderString("Visible Objects: " + std::to_string(visibleObjects), glm::vec2(0, 40), glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));

			if (wireToggled)
			{
				OpenGLState::Instance().toggleWireframe();
			}

			if (doScreenshot)
			{
				takeScreenShot(GENERATEDFILESPATH + "Screenshots", IDtoScreenshotName(screenshotid, true), width, height);
			}
		}

		if (doScreenshot)
		{
			screenshotid++;
			doScreenshot = false;
		}

		glfwSwapBuffers(window);
		glfwPollEvents();
		checkKeys(window);

		if (glfwWindowShouldClose(window))
		{
			break;
		}

		fetchGLError();
	}
}

const float speed = 1.0f;

void DemoScene::checkKeys(GLFWwindow* window)
{
	float s = speed;
	if (glfwGetKey(window, GLFW_KEY_LEFT_SHIFT) == GLFW_PRESS)
	{
		s *= 10.0f;
	}
	if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS)
	{
		panFront(*cam, s, (float)time.LastFrameTime());
	}
	if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS)
	{
		panBack(*cam, s, (float)time.LastFrameTime());
	}
	if (glfwGetKey(window, GLFW_KEY_LEFT) == GLFW_PRESS)
	{
		panLeft(*cam, s, (float)time.LastFrameTime());
	}
	if (glfwGetKey(window, GLFW_KEY_RIGHT) == GLFW_PRESS)
	{
		panRight(*cam, s, (float)time.LastFrameTime());
	}
	if (glfwGetKey(window, GLFW_KEY_UP) == GLFW_PRESS)
	{
		panUp(*cam, s, (float)time.LastFrameTime());
	}
	if (glfwGetKey(window, GLFW_KEY_DOWN) == GLFW_PRESS)
	{
		panDown(*cam, s, (float)time.LastFrameTime());
	}
	if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS)
	{
		rotateLeft(*cam, s, (float)time.LastFrameTime());
	}
	if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS)
	{
		rotateRight(*cam, s, (float)time.LastFrameTime());
	}
	if (glfwGetKey(window, GLFW_KEY_PAGE_UP) == GLFW_PRESS)
	{
		lookUp(*cam, s, (float)time.LastFrameTime());
	}
	if (glfwGetKey(window, GLFW_KEY_PAGE_DOWN) == GLFW_PRESS)
	{
		lookDown(*cam, s, (float)time.LastFrameTime());
	}
	if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS)
	{
		power += (float)time.LastFrameTime() * s;
	}

	if (spherePackedObjects.size() > 0)
	{
		if (glfwGetKey(window, GLFW_KEY_1) == GLFW_PRESS)
		{
			spherePackedObjects[0]->setPosition(glm::translate(spherePackedObjects[0]->getTransform(), glm::vec3(s, 0.0f, 0.0f) * (float)time.LastFrameTime()));
		}
		if (glfwGetKey(window, GLFW_KEY_2) == GLFW_PRESS)
		{
			spherePackedObjects[0]->setPosition(glm::translate(spherePackedObjects[0]->getTransform(), glm::vec3(-s, 0.0f, 0.0f) * (float)time.LastFrameTime()));
		}
		if (glfwGetKey(window, GLFW_KEY_3) == GLFW_PRESS)
		{
			spherePackedObjects[0]->setPosition(glm::translate(spherePackedObjects[0]->getTransform(), glm::vec3(0.0f, s, 0.0f) * (float)time.LastFrameTime()));
		}
		if (glfwGetKey(window, GLFW_KEY_4) == GLFW_PRESS)
		{
			spherePackedObjects[0]->setPosition(glm::translate(spherePackedObjects[0]->getTransform(), glm::vec3(0.0f, -s, 0.0f) * (float)time.LastFrameTime()));
		}
		if (glfwGetKey(window, GLFW_KEY_5) == GLFW_PRESS)
		{
			spherePackedObjects[0]->setPosition(glm::translate(spherePackedObjects[0]->getTransform(), glm::vec3(0.0f, 0.0f, s) * (float)time.LastFrameTime()));
		}
		if (glfwGetKey(window, GLFW_KEY_6) == GLFW_PRESS)
		{
			spherePackedObjects[0]->setPosition(glm::translate(spherePackedObjects[0]->getTransform(), glm::vec3(0.0f, 0.0f, -s) * (float)time.LastFrameTime()));
		}
	}
}

void DemoScene::key_callback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	if (key == GLFW_KEY_PRINT_SCREEN && action == GLFW_PRESS)
	{
		doScreenshot = true;
	}

	if (key == GLFW_KEY_N && action == GLFW_PRESS)
	{
		GrassOvermind::getInstance().setUseDebugColor(!GrassOvermind::getInstance().getUseDebugColor());
	}

	if (key == GLFW_KEY_B && action == GLFW_PRESS)
	{
		GrassOvermind::getInstance().setUseFlare(!GrassOvermind::getInstance().getUseFlare());
	}

	if (key == GLFW_KEY_V && action == GLFW_PRESS)
	{
		bool dbc = GrassOvermind::getInstance().getDepthBufferCulling();
		GrassOvermind::getInstance().setDepthBufferCulling(!dbc);
		std::cout << "DepthBufferCulling is now " << (dbc ? "false" : "true") << std::endl;
	}

	if (key == GLFW_KEY_C && action == GLFW_PRESS)
	{
		bool coll = GrassOvermind::getInstance().getCollisionDetection();
		GrassOvermind::getInstance().setCollisionDetection(!coll);
		std::cout << "Collision detection is now " << (coll ? "deactive" : "active") << std::endl;
	}

	if (key == GLFW_KEY_X && action == GLFW_PRESS)
	{
		switch (windGenerator[0]->getWindType())
		{
		case WindType::VECTOR:
			windGenerator[0]->setWindType(WindType::POINT);
			windGenerator[0]->resetWind();
			std::cout << "Wind type is now directional" << std::endl;
			break;
		case WindType::POINT:
			windGenerator[0]->setWindType(WindType::POINTWITHTANGENTIAL);
			std::cout << "Wind type is now area" << std::endl;
			break;
		case WindType::POINTWITHTANGENTIAL:
			windGenerator[0]->setWindType(WindType::VECTOR);
			windGenerator[0]->resetWind();
			std::cout << "Wind type is now rotating area" << std::endl;
			break;
		}
	}

	if (key == GLFW_KEY_Y && action == GLFW_PRESS)
	{
		drawSkybox = !drawSkybox;
		if (!drawSkybox)
			std::cout << "Skybox disabled" << std::endl;
		else
			std::cout << "Skybox enabled" << std::endl;
	}

	if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
	{
		glm::mat4 position(glm::translate(glm::mat4(), cam->position + cam->viewVector * BALLSIZE * 1.5f));
		SceneObject* so = 0;

		if (ballGeometry.size() > 1)
		{
			int chosen = rand() % ballGeometry.size();

			so = new SceneObject(allInOneTessellationShader, ballGeometry[chosen], false, position, glm::vec4(1.0f, 1.0f, 1.0f, 1.0f));
			if (ballTextures.size() == ballGeometry.size())
			{
				so->attachTexture("diffuseTexture", ballTextures[chosen]);
			}
		}
		else
		{
			so = new SceneObject(ballShader, ballGeometry[0], false, position, glm::vec4(2.0f, 5.0f, 10.0f, 5.0f));
		}

		so->applyForce(cam->viewVector * 2.0f * power);
		power = 1.0f;
		balls.push_back(so);
	}

	if (key == GLFW_KEY_E && action == GLFW_PRESS)
	{
		GrassOvermind::getInstance().setUsePositionColor(!GrassOvermind::getInstance().getUsePositionColor());
	}

	if (key == GLFW_KEY_R && action == GLFW_PRESS)
	{
		GrassOvermind::getInstance().setInnerSphereCulling(!GrassOvermind::getInstance().getInnerSphereCulling());
	}

	if (key == GLFW_KEY_T && action == GLFW_PRESS)
	{
		drawFont = !drawFont;
	}

	if (key == GLFW_KEY_Z && action == GLFW_PRESS)
	{
		showDepth = !showDepth;
	}

	if (key == GLFW_KEY_TAB && action == GLFW_PRESS)
	{
		for each(SpherePackedObject* s in spherePackedObjects)
		{
			s->drawObject = !s->drawObject;
		}
	}

	if (key == GLFW_KEY_ENTER && action == GLFW_PRESS)
	{
		for (unsigned int i = 0; i < transformer.size(); i++)
		{
			transformer[i]->enabled = true;
		}
		animationStarted = true;
	}

	if (key == GLFW_KEY_KP_ADD && action == GLFW_PRESS && mods == GLFW_MOD_SHIFT)
	{
		float dcl = GrassOvermind::getInstance().getDepthCullLevel() + 1.0f;
		GrassOvermind::getInstance().setDepthCullLevel(dcl);
		std::cout << "DepthCullLevel is now " << dcl << std::endl;
	}

	if (key == GLFW_KEY_KP_SUBTRACT && action == GLFW_PRESS && mods == GLFW_MOD_SHIFT)
	{
		float dcl = GrassOvermind::getInstance().getDepthCullLevel() - 1.0f;
		GrassOvermind::getInstance().setDepthCullLevel(dcl);
		std::cout << "DepthCullLevel is now " << dcl << std::endl;
	}

	if (key == GLFW_KEY_KP_ADD && action == GLFW_PRESS && mods != GLFW_MOD_SHIFT)
	{
		WindGenerator::maxMagnitude = WindGenerator::maxMagnitude + 1.0f;
		std::cout << "Wind magnitude is now " << WindGenerator::maxMagnitude << std::endl;
	}

	if (key == GLFW_KEY_KP_SUBTRACT && action == GLFW_PRESS && mods != GLFW_MOD_SHIFT)
	{
		WindGenerator::maxMagnitude = glm::max(WindGenerator::maxMagnitude - 1.0f, 0.1f);
		std::cout << "Wind magnitude is now " << WindGenerator::maxMagnitude << std::endl;
	}

	if (key == GLFW_KEY_G && action == GLFW_PRESS)
	{
		bool ddc = GrassOvermind::getInstance().getDepthCulling();
		GrassOvermind::getInstance().setDepthCulling(!ddc);
		std::cout << "Depth culling " << (ddc ? "disabled" : "enabled") << std::endl;
	}

	if (key == GLFW_KEY_H && action == GLFW_PRESS)
	{
		bool dvfc = GrassOvermind::getInstance().getViewFrustumCulling();
		GrassOvermind::getInstance().setViewFrustumCulling(!dvfc);
		std::cout << "View frustum culling " << (dvfc ? "disabled" : "enabled") << std::endl;
	}

	if (key == GLFW_KEY_F && action == GLFW_PRESS)
	{
		bool doc = GrassOvermind::getInstance().getOrientationCulling();
		GrassOvermind::getInstance().setOrientationCulling(!doc);
		std::cout << "Orientation culling " << (doc ? "disabled" : "enabled") << std::endl;
	}

	if (key == GLFW_KEY_Q && action == GLFW_PRESS)
	{
		OpenGLState::Instance().toggleWireframe();
	}
}

void DemoScene::loadScene(unsigned int id)
{
	switch (id)
	{
	case 0:
		{
			  skybox = loadSkybox("hills");
			  physx::PxMaterial* groundMat = PhysXController::Instance().createMaterial(0.5f, 0.5f, 0.6f);
			  PhysXController::Instance().addStaticPlane(0.0f, 1.0f, 0.0f, 2.5f, groundMat);

			  sceneObjects.push_back(new SceneObject(allInOneTessellationShader, new SceneObjectGeometry(MODELPATH + "Scene0/testPlane.obj", SceneObjectGeometry::BasicGeometry::BOX, glm::vec3(160.0f, 160.0f, 160.0f), false, false, 0, false, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f), true, glm::translate(glm::mat4(), glm::vec3(0.0f, -2.5f, 0.0f)), glm::vec4(10.0f, 50.0f, 100.0f, 30.0f), glm::vec4(20, 20, 0, 0), glm::vec4(1, -1, 0, 1), 0.5f, 0.5f, 0.3f, 100.0f));
			  sceneObjects[sceneObjects.size() - 1]->attachTexture("diffuseTexture", Texture2D::loadTextureFromFile(TEXTUREPATH + "Ground/Soil.png", true, true, false, GL_REPEAT, GL_REPEAT, GL_LINEAR_MIPMAP_LINEAR, GL_LINEAR));

			  glm::vec2 dimension = sceneObjects[sceneObjects.size() - 1]->getGeometryScale().xz * 0.5f;

			  std::vector<Geometry::TriangleFace> faces;
			  faces.push_back(Geometry::TriangleFace({ glm::vec3(dimension.x, 0.0f, dimension.y), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
			  { glm::vec3(dimension.x, 0.0f, -dimension.y), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
			  { glm::vec3(-dimension.x, 0.0f, dimension.y), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) }));
			  faces.push_back(Geometry::TriangleFace({ glm::vec3(-dimension.x, 0.0f, -dimension.y), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
			  { glm::vec3(-dimension.x, 0.0f, dimension.y), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) },
			  { glm::vec3(dimension.x, 0.0f, -dimension.y), glm::vec3(0.0f, 1.0f, 0.0f), glm::vec3(1.0f, 0.0f, 0.0f), glm::vec3(0.0f, 0.0f, 1.0f) }));

			  std::vector<GrassCreateBladeParams> grassParams;
			  GrassCreateBladeParams grassParam1;
			  grassParam1.density = 50.0f;
			  grassParam1.distribution = GrassDistribution::UNIFORM;
			  grassParam1.clusterPercentage = 0.2f;
			  grassParam1.spacialDistribution = GrassSpacialDistribution::FACE_RANDOM;
			  grassParam1.bladeMinHeight = 1.3f;
			  grassParam1.bladeMaxHeight = 2.5f;
			  grassParam1.bladeMinWidth = 0.1f;
			  grassParam1.bladeMaxWidth = 0.14f;
			  grassParam1.bladeMinBend = 0.5f;
			  grassParam1.bladeMaxBend = 0.7f;
			  grassParam1.shape = BladeShape::THRESHTRIANGLEMINW;
			  grassParam1.tessellationProps = glm::vec4(3.0f, 5.0f, 15.0f, 5.0f);
			  grassParams.push_back(grassParam1);

			  grassFields.push_back(new Grass(grassParams, faces));
			  grassFields[grassFields.size() - 1]->parentObject = sceneObjects[sceneObjects.size() - 1];
			  grassFields[grassFields.size() - 1]->wind.push_back(windGenerator[0]);
			  grassFields[grassFields.size() - 1]->depthTexture = fboDepthTex;

			  SceneObjectGeometry* text = new SceneObjectGeometry(MODELPATH + "Scene0/Text1.ply", SceneObjectGeometry::BasicGeometry::SPHERE, glm::vec3(10.0f), true, false, 0, false, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
			  std::vector<GrassCreateBladeParams> params;
			  GrassCreateBladeParams param;
			  param.density = 60.0f;
			  param.distribution = GrassDistribution::UNIFORM;
			  param.clusterPercentage = 0.6f;
			  param.spacialDistribution = GrassSpacialDistribution::FACE_AREA;
			  param.bladeMinHeight = 0.6f;
			  param.bladeMaxHeight = 0.8f;
			  param.bladeMinWidth = 0.06f;
			  param.bladeMaxWidth = 0.08f;
			  param.bladeMinBend = 0.25f;
			  param.bladeMaxBend = 0.3f;
			  param.shape = BladeShape::QUADRATIC3DMINW;
			  param.tessellationProps = glm::vec4(3.0f, 5.0f, 15.0f, 5.0f);
			  params.push_back(param);
			  grassFields.push_back(new Grass(params, text->getFaceList()));
			  grassFields[grassFields.size() - 1]->wind.push_back(windGenerator[0]);
			  grassFields[grassFields.size() - 1]->depthTexture = fboDepthTex;
			  grassFields[grassFields.size() - 1]->useLocalGravity = true;
			  grassFields[grassFields.size() - 1]->localGravity = { glm::vec4(0.0f, 0.0f, -1.0f, 2.0f), glm::vec4(0.0f), 0.0f };
			  grassFields[grassFields.size() - 1]->modelMatrix = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 18.0f, -60.0f));
		}
		break;
	}
}

//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////
// Helper Functions
//////////////////////////////////////////////////////////////
//////////////////////////////////////////////////////////////

void createHeightField(PhysXController& physic, const HeightMap& heightMap, const SceneObject& object, const glm::vec2 objectSizeXZ, PxRigidStatic*& heightFieldActor)
{
	physx::PxHeightField* heightField = physic.createHeightField(heightMap);
	physx::PxMaterial* heightFieldMaterial = physic.createMaterial(0.5f, 0.5f, 0.6f);
	physx::PxTransform matrix(PxMat44((float *)&glm::translate(object.getTransform(), glm::vec3(-objectSizeXZ.x * 0.5f, 0.0f, -objectSizeXZ.y * 0.5f))));
	heightFieldActor = physic.createStatic(matrix);
	physx::PxReal rowScale = objectSizeXZ.x / (heightMap.Height());
	physx::PxReal columnScale = objectSizeXZ.y / (heightMap.Width());
	physx::PxReal heightScale = PxMax(heightMap.heightScale / (PxReal)0x7fff, PX_MIN_HEIGHTFIELD_Y_SCALE);
	heightFieldActor->createShape(PxHeightFieldGeometry(heightField, PxMeshGeometryFlags(), heightScale, rowScale, columnScale), *heightFieldMaterial);
	//heightField->release();
}

void createHeightField(PhysXController& physic, const std::vector<Geometry::TriangleFace>& faces, const SceneObject& object, const glm::vec2 objectSizeXZ, PxRigidStatic*& heightFieldActor)
{
	float maxHeight = 0.0f;
	for each(auto f in faces)
	{
		for (unsigned int i = 0; i < 3; i++)
		{
			maxHeight = glm::max(maxHeight, f.vertices[i].position.y);
		}
	}

	physx::PxHeightField* heightField = physic.createHeightField(faces, maxHeight);
	physx::PxMaterial* heightFieldMaterial = physic.createMaterial(0.5f, 0.5f, 0.6f);
	physx::PxTransform matrix(PxMat44((float *)&glm::translate(object.getTransform(), glm::vec3(-objectSizeXZ.x * 0.5f, 0.0f, -objectSizeXZ.y * 0.5f))));
	heightFieldActor = physic.createStatic(matrix);
	physx::PxReal rowScale = objectSizeXZ.x / (float)(heightField->getNbRows());
	physx::PxReal columnScale = objectSizeXZ.y / (float)(heightField->getNbColumns());
	physx::PxReal heightScale = PxMax(maxHeight / (PxReal)0x7fff, PX_MIN_HEIGHTFIELD_Y_SCALE);
	heightFieldActor->createShape(PxHeightFieldGeometry(heightField, PxMeshGeometryFlags(), heightScale, rowScale, columnScale), *heightFieldMaterial);
	//heightField->release();
}

std::string IDtoScreenshotName(const unsigned int id, const bool text)
{
	std::string ret = "Screenshot";

	if (id < 10)
	{
		ret += "00" + std::to_string(id);
	}
	else if (id < 100)
	{
		ret += "0" + std::to_string(id);
	}
	else
	{
		ret += std::to_string(id);
	}

	if (text)
	{
		ret += "_text";
	}

	ret += ".png";

	return ret;
}

void takeScreenShot(const std::string& path, const std::string& name, const unsigned int width, const unsigned int height)
{
	uint8_t *pixels = new uint8_t[width * height * 3];
	// copy pixels from screen
	glReadBuffer(GL_BACK);
	glPixelStorei(GL_PACK_ALIGNMENT, 1);
	glReadPixels(0, 0, width, height, GL_RGB, GL_UNSIGNED_BYTE, (GLvoid *)pixels);

	std::string file = path + "/" + name;
	png_structp png = png_create_write_struct(PNG_LIBPNG_VER_STRING, nullptr, nullptr, nullptr);
	if (!png)
	{
		std::cout << "Unable to create write struct" << std::endl;
	}

	png_infop info = png_create_info_struct(png);
	if (!info) {
		png_destroy_write_struct(&png, &info);
		std::cout << "Unable to create info struct" << std::endl;
	}

	FILE *fp = fopen(file.c_str(), "wb");
	if (!fp) {
		png_destroy_write_struct(&png, &info);
		std::cout << "Unable to open file" << std::endl;
	}

	png_init_io(png, fp);
	unsigned int depth = 8;
	png_set_IHDR(png, info, width, height, depth, PNG_COLOR_TYPE_RGB, PNG_INTERLACE_NONE,
		PNG_COMPRESSION_TYPE_BASE, PNG_FILTER_TYPE_BASE);
	png_colorp palette = (png_colorp)png_malloc(png, PNG_MAX_PALETTE_LENGTH * sizeof(png_color));
	if (!palette) {
		fclose(fp);
		png_destroy_write_struct(&png, &info);
		std::cout << "Unable to create palette" << std::endl;
	}
	png_set_PLTE(png, info, palette, PNG_MAX_PALETTE_LENGTH);
	png_write_info(png, info);
	png_set_packing(png);

	png_bytepp rows = (png_bytepp)png_malloc(png, height * sizeof(png_bytep));
	for (unsigned int i = 0; i < height; ++i)
		rows[i] = (png_bytep)(pixels + (height - i - 1) * width * 3);

	png_write_image(png, rows);
	png_write_end(png, info);
	png_free(png, palette);
	png_destroy_write_struct(&png, &info);

	fclose(fp);
	delete[] rows;
	delete[] pixels;

	std::cout << "Saved file " << name << std::endl;
}

Skybox* loadSkybox(const std::string& skybox)
{
	return new Skybox(TEXTUREPATH + "Skybox/" + skybox + "/");
}

void DemoScene::reshape(unsigned int _width, unsigned int _height)
{
	width = _width;
	height = _height;

	if (fontRenderer != 0)
		delete fontRenderer;
	fontRenderer = new FontRenderer(RESSOURCEPATH + "Font/consola.ttf", 12, width, height);

	glBindRenderbuffer(GL_RENDERBUFFER, fboDepthBuffer);
	glRenderbufferStorageMultisample(GL_RENDERBUFFER, MS_SAMPLES, GL_DEPTH_COMPONENT, _width, _height);
	glBindRenderbuffer(GL_RENDERBUFFER, 0);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, fboColorTex->Handle());
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, MS_SAMPLES, GL_RGB, width, height, GL_TRUE);

	glBindTexture(GL_TEXTURE_2D_MULTISAMPLE, fboDepthTex->Handle());
	glTexImage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, MS_SAMPLES, GL_R16, width, height, GL_TRUE);

	Camera* cam_new = new Camera(glm::radians(60.0f), (float)width, (float)height, 0.1f, 200.0f, cam->position);
	cam_new->horizontalAngle = cam->horizontalAngle;
	cam_new->verticalAngle = cam->verticalAngle;
	delete cam;
	cam = cam_new;
}