/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "SpherePackedObject.h"
#include "glm/gtx/transform.hpp"
#include "OpenGLState.h"
#include "PhysXController.h"
#include <fstream>
#include <sstream>

Shader* SpherePackedObject::sphereShader = 0;
GLuint SpherePackedObject::sphereVAO = 0;
GLuint SpherePackedObject::sphereVBO = 0;

SpherePackedObject::SpherePackedObject(Shader* drawShader, std::string& filename, glm::vec3& scale, const bool generatePhysicalObject, const bool isStatic, const glm::mat4& position, const unsigned int maxIterations, const float staticFriction, const float dynamicFriction, const float restitution) : SceneObject(drawShader, 0, isStatic, position, glm::vec4(1.0f)), spheres()
{
	if (sphereShader == 0)
	{
		sphereShader = new Shader(SHADERPATH + "debugSphereShader");
	}
	if (sphereVAO == 0)
	{
		glGenVertexArrays(1, &sphereVAO);
		glGenBuffers(1, &sphereVBO);

		glm::vec3 pos = glm::vec3(0.0f, 0.0f, 0.0f);

		glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3), &pos, GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);

		glBindVertexArray(sphereVAO);
		glBindBuffer(GL_ARRAY_BUFFER, sphereVBO);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, (GLsizei)0, (GLvoid*)0);
		glBindVertexArray(0);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	//Check for file
	std::string name = filename + std::to_string(maxIterations) + "_0";
	std::replace(name.begin(), name.end(), '.', '_');
	std::replace(name.begin(), name.end(), '\\', '_');
	std::replace(name.begin(), name.end(), ':', '_');
	std::replace(name.begin(), name.end(), '/', '_');
	name = GENERATEDFILESPATH + name;
	std::ifstream f(name.c_str());

	bool spheresLoaded = false;
	if (f.good())
	{
		//Load from file
		std::string line;

		glm::vec3 inputScale;
		//Get Scale
		{
			std::getline(f, line);
			std::string::size_type sz0, sz1, sz2;
			inputScale.x = std::stof(line, &sz0);
			inputScale.y = std::stof(line.substr(sz0), &sz1);
			inputScale.z = std::stof(line.substr(sz0 + sz1), &sz2);
		}

		//Just works for uniform scale or equal scale
		if ((inputScale.x == inputScale.y && inputScale.x != inputScale.z && scale.x == scale.y && scale.x == scale.z) || (scale.x == inputScale.x && scale.y == inputScale.y && scale.z == inputScale.z))
		{
			glm::vec4 sizeScaling = glm::vec4(scale.x / inputScale.x);
			while (std::getline(f, line))
			{
				std::string::size_type sz0, sz1, sz2;
				float x = std::stof(line, &sz0);
				float y = std::stof(line.substr(sz0), &sz1);
				float z = std::stof(line.substr(sz0 + sz1), &sz2);
				float r = std::stof(line.substr(sz0 + sz1 + sz2));
				spheres.push_back(glm::vec4(x, y, z, r) * sizeScaling);
			}
			spheresLoaded = true;
			f.close();
		}
		else
		{
			//TODO try loading perhaps
			while (f.good())
			{
				name[name.size() - 1]++;
				f.close();
				f = std::ifstream(name.c_str());
			}
			f.close();
		}
	}	
	
	SceneObjectGeometry* geo = new SceneObjectGeometry(filename, SceneObjectGeometry::BasicGeometry::SPHERE, scale, !spheresLoaded, false, 0, false, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f);
	this->attachGeometry(geo);

	if (!spheresLoaded)
	{
		//Pack spheres
		spheres = SpherePacker::packSpheres(geo->getFaceList(), maxIterations, true);

		//Save to file
		std::ofstream writeStream(name);
		if (writeStream.is_open())
		{
			writeStream << std::to_string(scale.x) << " " << std::to_string(scale.y) << " " << std::to_string(scale.z) << "\n";
			for (unsigned int i = 0; i < spheres.size(); i++)
			{
				glm::vec4 sphere = spheres[i];
				writeStream << std::to_string(sphere.x) << " " << std::to_string(sphere.y) << " " << std::to_string(sphere.z) << " " << std::to_string(sphere.w) << "\n";
			}
			writeStream.close();
		}
		else
		{
			std::cout << "Could not open file to write sphere info. Filename = " << name << std::endl;
		}
	}

	if (generatePhysicalObject)
	{
		PhysXController& physX = PhysXController::Instance();

		std::vector<physx::PxSphereGeometry> sphereGeometries;
		physicalObject = physX.createEmptyActor(PhysXController::MatToPxTransform(position), isStatic);
		PxMaterial* material = physX.createMaterial(staticFriction, dynamicFriction, restitution);

		GeometryArgs g(GeometryArgs::GeometryType::SPHERE);
		//glm::vec3 mean = geo->min + (geo->max - geo->min) * 0.5f;
		for (unsigned int i = 0; i < spheres.size(); i++)
		{
			if (g.numericArgs.size() == 0)
			{
				g.addNumericArg((PxReal)spheres[i].w);
			}
			else
			{
				g.numericArgs[0] = (PxReal)spheres[i].w;
			}
			PxGeometry* geom = physX.createGeometry(g);
			PxShape* addedShape = physicalObject->createShape(*geom, *material);
			glm::vec3 pos = spheres[i].xyz;
			//glm::vec3 pos = mean - (spheres[i].xyz - mean);
			addedShape->setLocalPose(PhysXController::MatToPxTransform(glm::translate(glm::mat4(1.0f), pos)));
		}
		if (!isStatic)
		{
			PxRigidBodyExt::updateMassAndInertia(*((physx::PxRigidBody*)physicalObject), (physx::PxReal)1.0f);
		}

		physX.addActorToScene(physicalObject);
	}
}

SpherePackedObject::~SpherePackedObject()
{
	delete this->geometry;
	this->geometry = 0;
}

void SpherePackedObject::draw(const Camera& cam)
{
	if (drawObject)
	{
		SceneObject::draw(cam);
	}
	else
	{
		unsigned int amountSpheres = spheres.size();

		OpenGLState::Instance().setPatchVertices(1);

		sphereShader->bind();
		glBindVertexArray(sphereVAO);
		for (unsigned int i = 0; i < amountSpheres; i++)
		{
			glm::vec4 sphere = spheres[i];

			glm::mat4 posMat = modelMatrix * glm::translate(glm::mat4(1.0f), glm::vec3(sphere));
			glm::mat4 mvpMatrix = cam.viewProjectionMatrix * posMat;

			sphereShader->setUniform("r", sphere.w);
			sphereShader->setUniform("mvpMatrix", mvpMatrix);
			sphereShader->setUniform("uniColor", glm::vec4((float)i / (float)amountSpheres, 1.0f, 1.0f, 1.0f));

			glDrawArrays(GL_PATCHES, 0, 1);
		}

		glBindVertexArray(0);
		sphereShader->unbind();
	}
}