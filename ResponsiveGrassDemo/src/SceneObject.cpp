/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "SceneObject.h"
#include "physx/foundation/PxMat44.h"
#include "glm/gtc/type_ptr.hpp"
#include "PhysXController.h"
#include "OpenGLState.h"
#include "BoundingBox.h"
#include "BoundingSphere.h"

#include "glm/gtc/matrix_transform.hpp"

#include <iostream>

#define GREEDY_ITERATIONS 1000

SceneObject::SceneObject(Shader* _drawShader, SceneObjectGeometry* _geometry, const bool isStatic, const glm::mat4& position, const glm::vec4& tessellationProps, const glm::vec4& textureTileAndOffset, const glm::vec4& heightMapTileAndOffset, const float ambientCoefficient, const float diffuseCoefficient, const float specularCoefficient, const float specularExponent) : drawShader(_drawShader), geometry(0), isStatic(isStatic), bounds(0), physicalObject(0), modelMatrix(position), tessellationProps(tessellationProps), textureTileAndOffset(textureTileAndOffset), heightMapTileAndOffset(heightMapTileAndOffset), ambientCoefficient(ambientCoefficient), diffuseCoefficient(diffuseCoefficient), specularCoefficient(specularCoefficient), specularExponent(specularExponent)
{
	if (_geometry != 0)
	{
		attachGeometry(_geometry);
	}
}

SceneObject::~SceneObject()
{
	if (physicalObject != 0)
	{
		PhysXController::Instance().removeActorFromScene(physicalObject);
		physicalObject->release();
	}
}

void SceneObject::update(const Camera& cam, const Clock& time)
{
	if (physicalObject != 0)
	{
		physx::PxTransform pose = physicalObject->getGlobalPose();
		physx::PxMat44 matrix(pose);
		float* m = (float*)&matrix;
		modelMatrix = glm::make_mat4(m);
	}

	if (bounds != 0)
	{
		visible = bounds->isVisible(cam, modelMatrix);
	}

	for each (SceneObject* child in children)
	{
		child->update(cam, time);
		visible |= child->isVisible();
	}
}

void SceneObject::draw(const Camera& cam)
{
	if (visible)
	{
		if (drawShader != 0)
		{
			drawShader->bind();

			//modelmatrix set by geometry
			drawShader->setUniform("vpMatrix", cam.viewProjectionMatrix);
			drawShader->setUniform("camPos", cam.position);
			drawShader->setUniform("lightDir", LIGHTDIR);

			drawShader->setUniform("nearFar", glm::vec2(cam.near, cam.far));

			drawShader->setUniform("ambientCoefficient", ambientCoefficient);
			drawShader->setUniform("diffuseCoefficient", diffuseCoefficient);
			drawShader->setUniform("specularCoefficient", specularCoefficient);
			drawShader->setUniform("specularExponent", specularExponent);

			drawShader->setUniform("hasNormalMap", (GLboolean)hasNormalMap);
			drawShader->setUniform("hasDiffuseTexture", (GLboolean)hasDiffuseTexture);
			drawShader->setUniform("hasSpecularTexture", (GLboolean)hasSpecularTexture);
			drawShader->setUniform("hasAlphaTexture", (GLboolean)hasAlphaTexture);
			drawShader->setUniform("textureTileAndOffset", textureTileAndOffset);

			if (drawShader->hasTessellationShader())
			{
				drawShader->setUniform("hasHeightMap", (GLboolean)hasHeightMap);
				drawShader->setUniform("minTessLevel", tessellationProps.x);
				drawShader->setUniform("maxTessLevel", tessellationProps.y);
				drawShader->setUniform("maxDistance", tessellationProps.z);
				drawShader->setUniform("minDistance", tessellationProps.w);
				if (hasHeightMap)
				{
					drawShader->setUniform("heightMapTileAndOffset", heightMapTileAndOffset);
				}
			}

			for (unsigned int i = 0; i < textures.size(); i++)
			{
				auto txt = textures[i];
				if (txt.name.find("height") != std::string::npos || drawShader->hasTessellationShader())
				{
					txt.ptr->bind((GLint)i);
					drawShader->setUniform(txt.name, (GLint)i);
				}
			}

			/*if (hasAlphaTexture)
			{
				OpenGLState::Instance().enable(GL_BLEND);
			}*/

			GLenum drawMode = drawShader->hasTessellationShader() ? GL_PATCHES : GL_TRIANGLES;
			if (geometry != 0)
			{
				geometry->draw(drawMode, modelMatrix, drawShader);
			}

			/*if (hasAlphaTexture)
			{
				OpenGLState::Instance().disable(GL_BLEND);
			}*/
		}
	
		for each (SceneObject* child in children)
		{
			child->parentDraw(modelMatrix);
		}

		if (geometry != 0 && geometry->innerSphere != 0)
		{
			//geometry->drawInnerSphere(modelMatrix, cam.viewProjectionMatrix);
		}
	}
}

void SceneObject::parentDraw(glm::mat4& parentMatrix)
{
	if (visible)
	{
		glm::mat4 matrix = modelMatrix * parentMatrix;
		if (drawShader != 0)
		{
			GLenum drawMode = drawShader->hasTessellationShader() ? GL_PATCHES : GL_TRIANGLES;
			if (geometry != 0)
			{
				geometry->draw(drawMode, matrix, drawShader);
			}
		}

		for each (SceneObject* child in children)
		{
			child->parentDraw(matrix);
		}
	}
}

void SceneObject::attachGeometry(SceneObjectGeometry* _geometry)
{
	geometry = _geometry;
	bounds = geometry->getBoundingObject();
	modelMatrix = modelMatrix * geometry->rootTransform;

	if (geometry->physicalGeometry != 0)
	{
		glm::mat4 mMatrix = modelMatrix * geometry->geometryMatrix;
		physx::PxMat44 matrix((float *)&mMatrix);
		if (isStatic)
		{
			physicalObject = PhysXController::Instance().createStatic(physx::PxTransform(matrix), *geometry->physicalGeometry, *geometry->physicalMaterial);
		}
		else
		{
			physicalObject = PhysXController::Instance().createDynamic(physx::PxTransform(matrix), *geometry->physicalGeometry, *geometry->physicalMaterial, physx::PxVec3(0.0f), geometry->physicalAngularDamping, geometry->physicalDensity);
		}
		PhysXController::Instance().addActorToScene(physicalObject);
	}
}

void SceneObject::attachChild(SceneObject* child)
{
	if (child != 0)
	{
		children.push_back(child);
	}
}

void SceneObject::attachTexture(std::string name, Texture2D* ptr)
{
	if (ptr != 0)
	{
		TextureNamePtr tnp;
		tnp.name = name;
		tnp.ptr = ptr;
		textures.push_back(tnp);

		if (name.find("diffuse", 0) != std::string::npos)
		{
			hasDiffuseTexture = true;
		}
		if (name.find("specular", 0) != std::string::npos)
		{
			hasSpecularTexture = true;
		}
		if (name.find("alpha", 0) != std::string::npos)
		{
			hasAlphaTexture = true;
		}
		if (name.find("normal", 0) != std::string::npos)
		{
			hasNormalMap = true;
		}
		if (name.find("height", 0) != std::string::npos)
		{
			hasHeightMap = true;
		}
	}
}

Texture2D* SceneObject::detachTexture(const std::string& name)
{
	unsigned int i;
	bool found = false;
	for (i = 0; i < textures.size(); i++)
	{
		if (textures[i].name.compare(name) == 0)
		{
			found = true;
			break;
		}
	}

	if (found)
	{
		Texture2D* ptr = textures[i].ptr;
		textures.erase(textures.begin() + i);

		if (name.find("diffuse", 0) != std::string::npos)
		{
			hasDiffuseTexture = false;
		}
		if (name.find("specular", 0) != std::string::npos)
		{
			hasSpecularTexture = false;
		}
		if (name.find("alpha", 0) != std::string::npos)
		{
			hasAlphaTexture = false;
		}
		if (name.find("normal", 0) != std::string::npos)
		{
			hasNormalMap = false;
		}
		if (name.find("height", 0) != std::string::npos)
		{
			hasHeightMap = false;
		}

		return ptr;
	}
	else
	{
		return 0;
	}
}

void SceneObject::setPosition(glm::mat4& position)
{
	modelMatrix = position;
	if (physicalObject != 0)
	{
		physx::PxMat44 matrix((float *)&modelMatrix);
		physicalObject->setGlobalPose(physx::PxTransform(matrix));
	}
}

void SceneObject::applyForce(glm::vec3 force)
{
	if (physicalObject != 0 && !isStatic)
	{
		physx::PxVec3 f(force.x,force.y,force.z);
		PxRigidDynamic* physicalDynamicObject = (PxRigidDynamic*)physicalObject;
		physicalDynamicObject->setLinearVelocity(physicalDynamicObject->getLinearVelocity() + f);
		physicalDynamicObject->setAngularVelocity(physx::PxVec3(f.z, f.y, -f.x));
	}
}

bool SceneObject::isVisible() const 
{
	return visible;
}

bool SceneObject::hasInnerSphere() const
{
	if (geometry != 0)
	{
		return geometry->innerSphere != 0;
	}
	return false;
}

const glm::vec4 SceneObject::getInnerSphereVector() const
{
	auto innerSphere = geometry->innerSphere;
	glm::vec3 p = (modelMatrix * glm::vec4(innerSphere->centerPoint, 1.0f)).xyz;
	return glm::vec4(p, innerSphere->radius);
}

SceneObjectGeometry::BasicGeometry SceneObject::getGeometryType() const
{
	if (geometry == 0)
	{
		return SceneObjectGeometry::BasicGeometry::OTHER;
	}
	return geometry->getGeometryType();
}

const glm::mat4& SceneObject::getTransform() const
{
	return modelMatrix;
}

BoundingObject* SceneObject::getBoundingObject() const
{
	return bounds;
}

const glm::vec3 fallbackGeometryScale = glm::vec3(0.0f);
const glm::vec3& SceneObject::getGeometryScale() const
{
	if (geometry == 0)
	{
		return fallbackGeometryScale;
	}
	return geometry->getGeometryScale();
}

SceneObjectGeometry* SceneObject::getGeometry() const
{
	return geometry;
}

/////////////////////////////////
// SceneObjectGeometry //////////
/////////////////////////////////

unsigned int SceneObjectGeometry::instanceCount = 0;

void createBoxGeometry(std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<glm::vec2>& uv, std::vector<GLuint>& index, const glm::vec3& scale)
{
	//Front
	positions.push_back(glm::vec3(-0.5f,  0.5f, -0.5f) * scale); //0
	positions.push_back(glm::vec3(-0.5f, -0.5f, -0.5f) * scale); //1
	positions.push_back(glm::vec3(0.5f, -0.5f, -0.5f) * scale); //2
	positions.push_back(glm::vec3(0.5f, 0.5f, -0.5f) * scale); //3
	normals.push_back(glm::vec3(0.0f, 0.0f, -1.0f));
	normals.push_back(glm::vec3(0.0f, 0.0f, -1.0f));
	normals.push_back(glm::vec3(0.0f, 0.0f, -1.0f));
	normals.push_back(glm::vec3(0.0f, 0.0f, -1.0f));
	uv.push_back(glm::vec2(0.0f, 1.0f));
	uv.push_back(glm::vec2(0.0f, 0.0f));
	uv.push_back(glm::vec2(1.0f, 0.0f));
	uv.push_back(glm::vec2(1.0f, 1.0f));
	index.push_back(0);
	index.push_back(1);
	index.push_back(2);
	index.push_back(0);
	index.push_back(2);
	index.push_back(3);
	//Left
	positions.push_back(glm::vec3(-0.5f, 0.5f, 0.5f) * scale); //4
	positions.push_back(glm::vec3(-0.5f, -0.5f, 0.5f) * scale); //5
	positions.push_back(glm::vec3(-0.5f, -0.5f, -0.5f) * scale); //6
	positions.push_back(glm::vec3(-0.5f, 0.5f, -0.5f) * scale); //7
	normals.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
	normals.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
	normals.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
	normals.push_back(glm::vec3(-1.0f, 0.0f, 0.0f));
	uv.push_back(glm::vec2(0.0f, 1.0f));
	uv.push_back(glm::vec2(0.0f, 0.0f));
	uv.push_back(glm::vec2(1.0f, 0.0f));
	uv.push_back(glm::vec2(1.0f, 1.0f));
	index.push_back(4);
	index.push_back(5);
	index.push_back(6);
	index.push_back(4);
	index.push_back(6);
	index.push_back(7);
	//Right
	positions.push_back(glm::vec3(0.5f, 0.5f, -0.5f) * scale); //8
	positions.push_back(glm::vec3(0.5f, -0.5f, -0.5f) * scale); //9
	positions.push_back(glm::vec3(0.5f, -0.5f, 0.5f) * scale); //10
	positions.push_back(glm::vec3(0.5f, 0.5f, 0.5f) * scale); //11
	normals.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
	normals.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
	normals.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
	normals.push_back(glm::vec3(1.0f, 0.0f, 0.0f));
	uv.push_back(glm::vec2(0.0f, 1.0f));
	uv.push_back(glm::vec2(0.0f, 0.0f));
	uv.push_back(glm::vec2(1.0f, 0.0f));
	uv.push_back(glm::vec2(1.0f, 1.0f));
	index.push_back(8);
	index.push_back(9);
	index.push_back(10);
	index.push_back(8);
	index.push_back(10);
	index.push_back(11);
	//Back
	positions.push_back(glm::vec3(0.5f, 0.5f, 0.5f) * scale); //12
	positions.push_back(glm::vec3(0.5f, -0.5f, 0.5f) * scale); //13
	positions.push_back(glm::vec3(-0.5f, -0.5f, 0.5f) * scale); //14
	positions.push_back(glm::vec3(-0.5f, 0.5f, 0.5f) * scale); //15
	normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
	normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
	normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
	normals.push_back(glm::vec3(0.0f, 0.0f, 1.0f));
	uv.push_back(glm::vec2(0.0f, 1.0f));
	uv.push_back(glm::vec2(0.0f, 0.0f));
	uv.push_back(glm::vec2(1.0f, 0.0f));
	uv.push_back(glm::vec2(1.0f, 1.0f));
	index.push_back(12);
	index.push_back(13);
	index.push_back(14);
	index.push_back(12);
	index.push_back(14);
	index.push_back(15);
	//TOP
	positions.push_back(glm::vec3(-0.5f, 0.5f, 0.5f) * scale); //16
	positions.push_back(glm::vec3(-0.5f, 0.5f, -0.5f) * scale); //17
	positions.push_back(glm::vec3(0.5f, 0.5f, -0.5f) * scale); //18
	positions.push_back(glm::vec3(0.5f, 0.5f, 0.5f) * scale); //19
	normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	normals.push_back(glm::vec3(0.0f, 1.0f, 0.0f));
	uv.push_back(glm::vec2(0.0f, 1.0f));
	uv.push_back(glm::vec2(0.0f, 0.0f));
	uv.push_back(glm::vec2(1.0f, 0.0f));
	uv.push_back(glm::vec2(1.0f, 1.0f));
	index.push_back(16);
	index.push_back(17);
	index.push_back(18);
	index.push_back(16);
	index.push_back(18);
	index.push_back(19);
	//BOT
	positions.push_back(glm::vec3(-0.5f, -0.5f, -0.5f) * scale); //20
	positions.push_back(glm::vec3(-0.5f, -0.5f, 0.5f) * scale); //21
	positions.push_back(glm::vec3(0.5f, -0.5f, -0.5f) * scale); //22
	positions.push_back(glm::vec3(0.5f, -0.5f, 0.5f) * scale); //23
	normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
	normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
	normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
	normals.push_back(glm::vec3(0.0f, -1.0f, 0.0f));
	uv.push_back(glm::vec2(0.0f, 1.0f));
	uv.push_back(glm::vec2(0.0f, 0.0f));
	uv.push_back(glm::vec2(1.0f, 0.0f));
	uv.push_back(glm::vec2(1.0f, 1.0f));
	index.push_back(20);
	index.push_back(21);
	index.push_back(22);
	index.push_back(20);
	index.push_back(22);
	index.push_back(23);
}

glm::vec2 calcSphereUV(const glm::vec3& pos) //Miller cylindrical projection
{
	//float l = glm::atan(pos.y, pos.x);
	//float p = glm::atan(pos.z, pos.x);
	//return glm::vec2((l + PI_F) / (2.0f * PI_F), glm::clamp(glm::asinh((glm::tan(p)) + 5.0f) / 10.0f,0.0f,1.0f));
	glm::vec3 p = glm::normalize(-pos);
	return glm::vec2(0.5 + (glm::atan(p.z, p.x)) / (2.0f * PI_F), 0.5 - (glm::asin(p.y)) / (PI_F));
}

void createSphereGeometry(std::vector<glm::vec3>& positions, std::vector<glm::vec3>& normals, std::vector<glm::vec2>& uv, std::vector<GLuint>& index, const glm::vec3& scale)
{
	float t = (1.0f + sqrtf(5.0f)) / 2.0f;
	positions.push_back(glm::normalize(glm::vec3(-1.0f, t, 0.0f)) * scale);
	positions.push_back(glm::normalize(glm::vec3( 1.0f, t, 0.0f)) * scale);
	positions.push_back(glm::normalize(glm::vec3(-1.0f,-t, 0.0f)) * scale);
	positions.push_back(glm::normalize(glm::vec3( 1.0f,-t, 0.0f)) * scale);
	positions.push_back(glm::normalize(glm::vec3( 0.0f,-1.0f, t)) * scale);
	positions.push_back(glm::normalize(glm::vec3( 0.0f, 1.0f, t)) * scale);
	positions.push_back(glm::normalize(glm::vec3( 0.0f,-1.0f,-t)) * scale);
	positions.push_back(glm::normalize(glm::vec3( 0.0f, 1.0f,-t)) * scale);
	positions.push_back(glm::normalize(glm::vec3( t, 0.0f,-1.0f)) * scale);
	positions.push_back(glm::normalize(glm::vec3( t, 0.0f, 1.0f)) * scale);
	positions.push_back(glm::normalize(glm::vec3(-t, 0.0f,-1.0f)) * scale);
	positions.push_back(glm::normalize(glm::vec3(-t, 0.0f, 1.0f)) * scale);

	normals.push_back(glm::normalize(glm::vec3(-1.0f, t, 0.0f)));
	normals.push_back(glm::normalize(glm::vec3(1.0f, t, 0.0f)));
	normals.push_back(glm::normalize(glm::vec3(-1.0f, -t, 0.0f)));
	normals.push_back(glm::normalize(glm::vec3(1.0f, -t, 0.0f)));
	normals.push_back(glm::normalize(glm::vec3(0.0f, -1.0f, t)));
	normals.push_back(glm::normalize(glm::vec3(0.0f, 1.0f, t)));
	normals.push_back(glm::normalize(glm::vec3(0.0f, -1.0f, -t)));
	normals.push_back(glm::normalize(glm::vec3(0.0f, 1.0f, -t)));
	normals.push_back(glm::normalize(glm::vec3(t, 0.0f, -1.0f)));
	normals.push_back(glm::normalize(glm::vec3(t, 0.0f, 1.0f)));
	normals.push_back(glm::normalize(glm::vec3(-t, 0.0f, -1.0f)));
	normals.push_back(glm::normalize(glm::vec3(-t, 0.0f, 1.0f)));

	for each (glm::vec3 pos in positions)
	{
		uv.push_back(calcSphereUV(pos));
	}

	index.push_back(0);
	index.push_back(11);
	index.push_back(5);
	index.push_back(0);
	index.push_back(5);
	index.push_back(1);
	index.push_back(0);
	index.push_back(1);
	index.push_back(7);
	index.push_back(0);
	index.push_back(7);
	index.push_back(10);
	index.push_back(0);
	index.push_back(10);
	index.push_back(11);
	index.push_back(1);
	index.push_back(5);
	index.push_back(9);
	index.push_back(5);
	index.push_back(11);
	index.push_back(4);
	index.push_back(11);
	index.push_back(10);
	index.push_back(2);
	index.push_back(10);
	index.push_back(7);
	index.push_back(6);
	index.push_back(7);
	index.push_back(1);
	index.push_back(8);
	index.push_back(3);
	index.push_back(9);
	index.push_back(4);
	index.push_back(3);
	index.push_back(4);
	index.push_back(2);
	index.push_back(3);
	index.push_back(2);
	index.push_back(6);
	index.push_back(3);
	index.push_back(6);
	index.push_back(8);
	index.push_back(3);
	index.push_back(8);
	index.push_back(9);
	index.push_back(4);
	index.push_back(9);
	index.push_back(5);
	index.push_back(2);
	index.push_back(4);
	index.push_back(11);
	index.push_back(6);
	index.push_back(2);
	index.push_back(10);
	index.push_back(8);
	index.push_back(6);
	index.push_back(7);
	index.push_back(9);
	index.push_back(8);
	index.push_back(1);
}

SceneObjectGeometry::SceneObjectGeometry(const BasicGeometry type, const glm::vec3& scale, const bool calculateInnerSphere, const unsigned int innerSphereMethod, const bool generatePhysicalObject, const float staticFriction, const float dynamicFriction, const float restitution, const float angularDamping, const float density) : geometryType(type), geometryScale(scale), boundingObject(0), innerSphere(0), physicalGeometry(0), physicalMaterial(0), geometryMatrix(1.0f), rootTransform(1.0f), vaoHandle(0)
{
	instanceCount++;

	std::vector<glm::vec3> positions;
	std::vector<glm::vec3> normals;
	std::vector<glm::vec2> uv;
	std::vector<GLuint> index;

	switch (type)
	{
	case BasicGeometry::BOX:
		{
			createBoxGeometry(positions, normals, uv, index, scale);
			if (generatePhysicalObject)
			{
				GeometryArgs args(GeometryArgs::GeometryType::BOX);
				args.addNumericArg(scale.x * 0.5f);
				args.addNumericArg(scale.y * 0.5f);
				args.addNumericArg(scale.z * 0.5f);
				physicalGeometry = PhysXController::Instance().createGeometry(args);
			}
			boundingObject = new BoundingBox(-0.5f * scale.x,0.5f * scale.x, -0.5f * scale.y, 0.5f * scale.y, -0.5f * scale.z, 0.5f * scale.z);
			break;
		}
	case BasicGeometry::SPHERE:
		{
			createSphereGeometry(positions, normals, uv, index, scale);
			if (generatePhysicalObject)
			{
				GeometryArgs args(GeometryArgs::GeometryType::SPHERE);
				args.addNumericArg(glm::max(scale.x, glm::max(scale.y, scale.z)));
				physicalGeometry = PhysXController::Instance().createGeometry(args);
			}
			boundingObject = new BoundingSphere(glm::max(scale.x, glm::max(scale.y, scale.z)));
			break;
		}
	case BasicGeometry::OTHER:
		//TODO
		break;
	}

	if (generatePhysicalObject)
	{
		physicalMaterial = PhysXController::Instance().createMaterial(staticFriction, dynamicFriction, restitution);
		physicalAngularDamping = angularDamping;
		physicalDensity = density;
	}

	if (calculateInnerSphere)
	{
		calculateInnerSphereGreedy(index, positions, innerSphereMethod, 100);
	}

	uploadDataToGraphicsCard(&index, &positions, &normals, &uv, 0, 0, glm::vec3(1.0f));
}

void getMinMaxAxes(const AssimpImporter::ImportModel* model, glm::vec3& min, glm::vec3& max)
{
	min = glm::vec3(FLT_MAX, FLT_MAX, FLT_MAX);
	max = glm::vec3(FLT_MIN, FLT_MIN, FLT_MIN);

	std::vector<const AssimpImporter::ImportModel*> models;
	std::vector<glm::mat4> modelMatrices;
	models.push_back(model);
	modelMatrices.push_back(model->transform);
	for (unsigned int i = 0; i<models.size(); i++)
	{
		const AssimpImporter::ImportModel* m = models[i];
		glm::mat4 modelMatrix = modelMatrices[i];
		if (m->position.size() > 0)
		{
			for (unsigned int p = 0; p < m->position.size(); p++)
			{
				auto pos = modelMatrix * glm::vec4(m->position[p], 1.0f);
				if (pos.x < min.x)
				{
					min.x = pos.x;
				}
				if (pos.x > max.x)
				{
					max.x = pos.x;
				}
				if (pos.y < min.y)
				{
					min.y = pos.y;
				}
				if (pos.y > max.y)
				{
					max.y = pos.y;
				}
				if (pos.z < min.z)
				{
					min.z = pos.z;
				}
				if (pos.z > max.z)
				{
					max.z = pos.z;
				}
			}
		}

		if (m->children.size() > 0)
		{
			for (unsigned int c = 0; c < m->children.size(); c++)
			{
				models.push_back(m->children[c]);
				modelMatrices.push_back(m->children[c]->transform * modelMatrix);
			}
		}
	}
}

void getIndizesPositions(const AssimpImporter::ImportModel* model, const glm::vec3& scale, const glm::mat4& geometryMatrix, std::vector<unsigned int>& indizes, std::vector<glm::vec3>& positions)
{
	std::vector<const AssimpImporter::ImportModel*> models;
	std::vector<glm::mat4> modelMatrices;
	models.push_back(model);
	modelMatrices.push_back(model->transform * geometryMatrix);
	unsigned int indexOffset = 0;
	for (unsigned int i = 0; i<models.size(); i++)
	{
		indexOffset = indizes.size();
		const AssimpImporter::ImportModel* m = models[i];
		glm::mat4 modelMatrix = modelMatrices[i];
		if (m->position.size() > 0)
		{
			for (unsigned int p = 0; p < m->position.size(); p++)
			{
				auto pos = modelMatrix * glm::vec4(m->position[p] * scale, 1.0f);
				positions.push_back(pos.xyz);
			}

			if (m->index.size() > 0)
			{
				for (unsigned int ind = 0; ind<m->index.size(); ind++)
				{
					indizes.push_back(m->index[ind] + indexOffset);
				}
			}
		}

		if (m->children.size() > 0)
		{
			for (unsigned int c = 0; c < m->children.size(); c++)
			{
				models.push_back(m->children[c]);
				modelMatrices.push_back(m->children[c]->transform * modelMatrix);
			}
		}
	}
}

void generateFaceListFromInputModel(std::vector<Geometry::TriangleFace>& faceList, const AssimpImporter::ImportModel* model, const glm::vec3& scale, const glm::mat4& geometryMatrix)
{
	std::vector<const AssimpImporter::ImportModel*> models;
	std::vector<glm::mat4> modelMatrices;
	models.push_back(model);
	modelMatrices.push_back(model->transform * geometryMatrix);
	for (unsigned int i = 0; i<models.size(); i++)
	{
		const AssimpImporter::ImportModel* m = models[i];
		glm::mat4 modelMatrix = modelMatrices[i];
		if (m->position.size() > 0)
		{
			if (m->index.size() > 0)
			{
				for (unsigned int ind = 0; ind<m->index.size(); ind+=3)
				{
					glm::vec3 p1 = glm::vec3(modelMatrix * glm::vec4(m->position[m->index[ind + 0]] * scale, 1.0f));
					glm::vec3 p2 = glm::vec3(modelMatrix * glm::vec4(m->position[m->index[ind + 1]] * scale, 1.0f));
					glm::vec3 p3 = glm::vec3(modelMatrix * glm::vec4(m->position[m->index[ind + 2]] * scale, 1.0f));
					if (m->tangent.size() > 0)
					{
						Geometry::Vertex v1 = { p1, m->normal[m->index[ind + 0]], m->tangent[m->index[ind + 0]], m->bitangent[m->index[ind + 0]] };
						Geometry::Vertex v2 = { p2, m->normal[m->index[ind + 1]], m->tangent[m->index[ind + 1]], m->bitangent[m->index[ind + 1]] };
						Geometry::Vertex v3 = { p3, m->normal[m->index[ind + 2]], m->tangent[m->index[ind + 2]], m->bitangent[m->index[ind + 2]] };
						Geometry::TriangleFace face(v1, v2, v3);
						faceList.push_back(face);
					}
					else
					{
						Geometry::Vertex v1 = { p1, m->normal[m->index[ind + 0]], glm::vec3(1.0f, 0.0f, 0.0f), glm::normalize(glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), m->normal[m->index[ind + 0]])) };
						Geometry::Vertex v2 = { p2, m->normal[m->index[ind + 1]], glm::vec3(1.0f, 0.0f, 0.0f), glm::normalize(glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), m->normal[m->index[ind + 1]])) };
						Geometry::Vertex v3 = { p3, m->normal[m->index[ind + 2]], glm::vec3(1.0f, 0.0f, 0.0f), glm::normalize(glm::cross(glm::vec3(1.0f, 0.0f, 0.0f), m->normal[m->index[ind + 2]])) };
						Geometry::TriangleFace face(v1, v2, v3);
						faceList.push_back(face);
					}
				}
			}
		}

		if (m->children.size() > 0)
		{
			for (unsigned int c = 0; c < m->children.size(); c++)
			{
				models.push_back(m->children[c]);
				modelMatrices.push_back(m->children[c]->transform * modelMatrix);
			}
		}
	}
}

SceneObjectGeometry::SceneObjectGeometry(const std::string filename, const BasicGeometry boundingType, const glm::vec3& scale, const bool generateFaceList, const bool calculateInnerSphere, const unsigned int innerSphereMethod, const bool generatePhysicalObject, const float staticFriction, const float dynamicFriction, const float restitution, const float angularDamping, const float density) : SceneObjectGeometry(AssimpImporter::importModel(filename), boundingType, scale, generateFaceList, calculateInnerSphere, innerSphereMethod, generatePhysicalObject, staticFriction, dynamicFriction, restitution, angularDamping, density)
{
}

SceneObjectGeometry::SceneObjectGeometry(const AssimpImporter::ImportModel* geometryModel, const BasicGeometry boundingType, const glm::vec3& scale, const bool generateFaceList, const bool calculateInnerSphere, const unsigned int innerSphereMethod, const bool generatePhysicalObject, const float staticFriction, const float dynamicFriction, const float restitution, const float angularDamping, const float density) : geometryType(BasicGeometry::OTHER), geometryScale(), boundingObject(0), innerSphere(0), physicalGeometry(0), physicalMaterial(0), geometryMatrix(1.0f), vaoHandle(0), faceList()
{
	instanceCount++;

	if (geometryModel == 0)
	{
		std::cout << "SceneObjectGeometry could not be loaded. Model is Null." << std::endl;
		throw std::runtime_error("Invalid SceneObjectGeometry. Model is Null");
		return;
	}

	//Get Min-Max Axes
	getMinMaxAxes(geometryModel, min, max);
	min = min * scale;
	max = max * scale;

	switch (boundingType)
	{
	case BOX:
	{
				float xWidth = max.x - min.x;
				float yWidth = max.y - min.y;
				float zWidth = max.z - min.z;
				geometryScale = glm::vec3(xWidth, yWidth, zWidth);
				if (generatePhysicalObject)
				{
					GeometryArgs args(GeometryArgs::GeometryType::BOX);
					args.addNumericArg(xWidth * 0.5f);
					args.addNumericArg(yWidth * 0.5f);
					args.addNumericArg(zWidth * 0.5f);
					physicalGeometry = PhysXController::Instance().createGeometry(args);
				}
				//boundingObject = new BoundingBox(-0.5f * xWidth, 0.5f * xWidth, -0.5f * yWidth, 0.5f * yWidth, -0.5f * zWidth, 0.5f * zWidth);
				boundingObject = new BoundingBox(min.x, max.x, min.y, max.y, min.z, max.z);
				glm::vec3 mean = min + (max - min) * 0.5f;
				geometryMatrix = glm::translate(glm::mat4(1.0f), -mean);
	}
		break;
	case SPHERE:
	{
				   float xWidth = max.x - min.x;
				   float yWidth = max.y - min.y;
				   float zWidth = max.z - min.z;
				   float maxWidth = glm::max(xWidth, glm::max(yWidth, zWidth));
				   geometryScale = glm::vec3(maxWidth * 0.5f, maxWidth * 0.5f, maxWidth * 0.5f);
				   if (generatePhysicalObject)
				   {
					   GeometryArgs args(GeometryArgs::GeometryType::SPHERE);
					   args.addNumericArg(maxWidth * 0.5f);
					   physicalGeometry = PhysXController::Instance().createGeometry(args);
				   }
				   glm::vec3 mean = min + (max - min) * 0.5f;
				   boundingObject = new BoundingSphere(glm::sqrt(maxWidth * maxWidth * 0.5f), mean);
				   geometryMatrix = glm::translate(glm::mat4(1.0f), -mean);
	}
		break;
	case OTHER:
		//TODO
		break;
	}
	if (generatePhysicalObject)
	{
		physicalMaterial = PhysXController::Instance().createMaterial(staticFriction, dynamicFriction, restitution);
		physicalAngularDamping = angularDamping;
		physicalDensity = density;
	}

	if (calculateInnerSphere)
	{
		std::vector<unsigned int> indizes;
		std::vector<glm::vec3> positions;
		getIndizesPositions(geometryModel, scale, geometryMatrix, indizes, positions);

		calculateInnerSphereGreedy(indizes, positions, innerSphereMethod, GREEDY_ITERATIONS);
	}

	if (generateFaceList)
	{
		if (generatePhysicalObject)
		{
			generateFaceListFromInputModel(faceList, geometryModel, scale, geometryMatrix);
		}
		else
		{
			generateFaceListFromInputModel(faceList, geometryModel, scale, glm::mat4(1.0f));
		}
	}

	//Evaluate import model
	rootTransform = geometryModel->transform;

	if (geometryModel->position.size() != 0)
	{
		uploadDataToGraphicsCard(
			(geometryModel->index.size() != 0) ? &geometryModel->index : 0,
			(geometryModel->position.size() != 0) ? &geometryModel->position : 0,
			(geometryModel->normal.size() != 0) ? &geometryModel->normal : 0,
			(geometryModel->uv.size() != 0) ? &geometryModel->uv : 0,
			(geometryModel->tangent.size() != 0) ? &geometryModel->tangent : 0,
			(geometryModel->bitangent.size() != 0) ? &geometryModel->bitangent : 0,
			scale);
	}
	else
	{
		vaoHandle = 0;
	}

	for (unsigned int i = 0; i < geometryModel->children.size(); i++)
	{
		childGeometries.push_back(new SceneObjectGeometry(geometryModel->children[i], scale));
	}

	//Delete ImportModel
	std::vector<const AssimpImporter::ImportModel*> models;
	models.push_back(geometryModel);
	for (unsigned int i = 0; i < geometryModel->children.size(); i++)
	{
		const AssimpImporter::ImportModel* m = models[i];
		if (m->children.size() > 0)
		{
			for (unsigned int c = 0; c < m->children.size(); c++)
			{
				models.push_back(m->children[c]);
			}
		}
	}
	for (std::vector< const AssimpImporter::ImportModel* >::iterator it = models.begin(); it != models.end(); ++it)
	{
		delete (*it);
	}
	models.clear();
}

SceneObjectGeometry::SceneObjectGeometry(const AssimpImporter::ImportModel* geometryModel, const glm::vec3& scale) : geometryType(BasicGeometry::OTHER), geometryScale(1.0f), geometryMatrix(1.0f), innerSphere(0), hasNormals(false), hasUVs(false), hasTangentsAndBitangents(false)
{
	instanceCount++;
	//Evaluate import model
	geometryMatrix = geometryModel->transform;

	if (geometryModel->position.size() != 0)
	{
		uploadDataToGraphicsCard(
			(geometryModel->index.size() != 0) ? &geometryModel->index : 0,
			(geometryModel->position.size() != 0) ? &geometryModel->position : 0,
			(geometryModel->normal.size() != 0) ? &geometryModel->normal : 0,
			(geometryModel->uv.size() != 0) ? &geometryModel->uv : 0,
			(geometryModel->tangent.size() != 0) ? &geometryModel->tangent : 0,
			(geometryModel->bitangent.size() != 0) ? &geometryModel->bitangent : 0,
			scale);
	}
	else
	{
		vaoHandle = 0;
	}

	for (unsigned int i = 0; i < geometryModel->children.size(); i++)
	{
		childGeometries.push_back(new SceneObjectGeometry(geometryModel->children[i], scale));
	}
}

SceneObjectGeometry::~SceneObjectGeometry()
{
	glDeleteVertexArrays(1, &vaoHandle);
	if (vboHandle.size() > 0)
	{
		glDeleteBuffers(vboHandle.size(), &vboHandle[0]);
	}

	instanceCount--;

	if (innerSphere != 0)
	{
		delete innerSphere;
	}

	if (physicalGeometry != 0)
	{
		delete physicalGeometry;
	}

	if (physicalMaterial != 0)
	{
		physicalMaterial->release();
	}

	if (boundingObject != 0)
	{
		delete boundingObject;
	}

	for each(SceneObjectGeometry* child in childGeometries)
	{
		delete child;
	}
}

void SceneObjectGeometry::uploadDataToGraphicsCard(const std::vector<unsigned int>* index, const std::vector<glm::vec3>* position, const std::vector<glm::vec3>* normal, const std::vector<glm::vec2>* uv, const std::vector<glm::vec3>* tangent, const std::vector<glm::vec3>* bitangent, const glm::vec3& scale)
{
	if (position == 0)
	{
		//Empty geometry
		vaoHandle = 0;
		return;
	}

	if (!(tangent != 0) != !(bitangent != 0)) //XOR
	{
		//Invalid geometry
		std::cout << "Invalid Geometry: Tangents and bitangents must both be Null or not Null!" << std::endl;
		vaoHandle = 0;
		return;
	}

	if (normal == 0 && tangent != 0)
	{
		//Invalid geometry
		std::cout << "Invalid Geometry: No normals but tangents defined!" << std::endl;
		vaoHandle = 0;
		return;
	}

	glGenVertexArrays(1, &vaoHandle);

	unsigned int vbos = (unsigned int)(index != 0) + (unsigned int)(position != 0) + (unsigned int)(normal != 0) + (unsigned int)(uv != 0) + (unsigned int)(tangent != 0) + (unsigned int)(bitangent != 0);

	vboHandle.resize(vbos);
	glGenBuffers(vbos, vboHandle.data());

	unsigned int curVbo = 0;

	hasIndizes = index != 0;
	count = hasIndizes ? index->size() : position->size();

	//Buffer data
	if (position != 0)
	{
		std::vector<glm::vec3> scaledPositions;
		scaledPositions.reserve(position->size());
		for (unsigned int i = 0; i < position->size(); i++)
		{
			scaledPositions.push_back((*position)[i] * scale);
		}

		glBindBuffer(GL_ARRAY_BUFFER, vboHandle[curVbo]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * scaledPositions.size(), scaledPositions.data(), GL_STATIC_DRAW);
		curVbo++;
	}
	
	if (normal != 0)
	{
		hasNormals = true;
		glBindBuffer(GL_ARRAY_BUFFER, vboHandle[curVbo]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * normal->size(), normal->data(), GL_STATIC_DRAW);
		curVbo++;
	}
	
	if (uv != 0)
	{
		hasUVs = true;
		glBindBuffer(GL_ARRAY_BUFFER, vboHandle[curVbo]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec2) * uv->size(), uv->data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		curVbo++;
	}

	if (tangent != 0)
	{
		hasTangentsAndBitangents = true;
		glBindBuffer(GL_ARRAY_BUFFER, vboHandle[curVbo]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * tangent->size(), tangent->data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		curVbo++;
	}
	
	if (bitangent != 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboHandle[curVbo]);
		glBufferData(GL_ARRAY_BUFFER, sizeof(glm::vec3) * bitangent->size(), bitangent->data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ARRAY_BUFFER, 0);
		curVbo++;
	}

	if (index != 0)
	{
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboHandle[curVbo]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(GLuint)* index->size(), index->data(), GL_STATIC_DRAW);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
	}

	//Prepare VAO
	glBindVertexArray(vaoHandle);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, vboHandle[curVbo]);

	curVbo = 0;

	if (position != 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboHandle[curVbo]);
		glEnableVertexAttribArray(SHADER_POSITION_LOCATION);
		glVertexAttribPointer(SHADER_POSITION_LOCATION, 3, GL_FLOAT, GL_FALSE, (GLsizei)0, (GLvoid*)0);
		curVbo++;
	}

	if (normal != 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboHandle[curVbo]);
		glEnableVertexAttribArray(SHADER_NORMAL_LOCATION);
		glVertexAttribPointer(SHADER_NORMAL_LOCATION, 3, GL_FLOAT, GL_FALSE, (GLsizei)0, (GLvoid*)0);
		curVbo++;
	}
	
	if (uv != 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboHandle[curVbo]);
		glEnableVertexAttribArray(SHADER_UV_LOCATION);
		glVertexAttribPointer(SHADER_UV_LOCATION, 2, GL_FLOAT, GL_FALSE, (GLsizei)0, (GLvoid*)0);
		curVbo++;
	}

	if (tangent != 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboHandle[curVbo]);
		glEnableVertexAttribArray(SHADER_TANGENT_LOCATION);
		glVertexAttribPointer(SHADER_TANGENT_LOCATION, 3, GL_FLOAT, GL_FALSE, (GLsizei)0, (GLvoid*)0);
		curVbo++;
	}

	if (bitangent != 0)
	{
		glBindBuffer(GL_ARRAY_BUFFER, vboHandle[curVbo]);
		glEnableVertexAttribArray(SHADER_BITANGENT_LOCATION);
		glVertexAttribPointer(SHADER_BITANGENT_LOCATION, 3, GL_FLOAT, GL_FALSE, (GLsizei)0, (GLvoid*)0);
		curVbo++;
	}

	glBindVertexArray(0);
	glBindBuffer(GL_ARRAY_BUFFER, 0);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, 0);
}

void SceneObjectGeometry::draw(GLenum drawMode, glm::mat4& modelMatrix, Shader* drawShader)
{
	glm::mat4 matrix = modelMatrix;// *geometryMatrix;
	if (physicalGeometry == 0)
	{
		matrix = modelMatrix;
	}	

	if (vaoHandle != 0)
	{
		drawShader->setUniform("modelMatrix", matrix);
		drawShader->setUniform("hasNormals", (GLboolean)hasNormals);
		drawShader->setUniform("hasUVs", (GLboolean)hasUVs);
		drawShader->setUniform("hasTangentsAndBitangents", (GLboolean)hasTangentsAndBitangents);

		glBindVertexArray(vaoHandle);

		if (drawMode == GL_PATCHES)
		{
			//glPatchParameteri(GL_PATCH_VERTICES, 3);
			OpenGLState::Instance().setPatchVertices(3);
		}

		glDrawElements(drawMode, count, GL_UNSIGNED_INT, 0);

		glBindVertexArray(0);
	}

	for each (SceneObjectGeometry* c in childGeometries)
	{
		c->draw(drawMode, matrix, drawShader);
	}
}

struct TriangleFace
{
	glm::vec3 vertices[3];
	glm::vec3 normal;
};

struct PointSide
{
	glm::dvec3 point;
	double side;
};

//http://www.blackpawn.com/texts/pointinpoly/ und https://www.ecse.rpi.edu/~wrf/Research/Short_Notes/pt_in_polyhedron.html
bool isPointInPolyhedron(const glm::vec3& point, const std::vector<TriangleFace>& faces, const glm::vec3* certainPointForDirection, const float scale, const unsigned int method)
{
	double s = (double)scale;
	glm::dvec3 pHP = glm::dvec3(point) * s;

	glm::dvec3 ray;
	if (certainPointForDirection != 0)
	{
		ray = glm::normalize(glm::dvec3(*certainPointForDirection) * s - pHP);
	}
	else
	{
		ray = glm::normalize(glm::dvec3(random(), random(), random()));
	}

	double insideNum = 0.0f;

	std::vector<PointSide> trianglePointsFound;

	for each(TriangleFace f in faces)
	{
		glm::dvec3 normal = glm::dvec3(f.normal);
		glm::dvec3 vert1 = glm::dvec3(f.vertices[0]) * s;
		glm::dvec3 vert2 = glm::dvec3(f.vertices[1]) * s;
		glm::dvec3 vert3 = glm::dvec3(f.vertices[2]) * s;

		double d = glm::dot(vert1, normal);
		double dP = glm::dot(pHP, normal);
		double side = glm::sign(d - dP);

		double denom = glm::dot(ray, normal);

		double t;
		glm::dvec3 p;
		if (glm::abs(denom) > 1e-5)
		{
			t = glm::dot(vert1 - pHP, normal) / denom;
			p = pHP + t * ray;
		}
		else
		{
			glm::dvec3 nRay;
			while (glm::abs(denom) < 1e-5)
			{
				nRay = ray + glm::dvec3(random()-0.5, random()-0.5, random()-0.5) * 0.001;
				denom = glm::dot(nRay, normal);
			}
			t = glm::dot(vert1 - pHP, normal) / denom;
			p = pHP + t * nRay;
		}

		bool inside;
		switch (method)
		{
		case 0:
		{
			//Barycentric Method
			glm::dvec3 v0 = vert3 - vert1;
			glm::dvec3 v1 = vert2 - vert1;
			glm::dvec3 v2 = p - vert1;

			double dot00 = glm::dot(v0, v0);
			double dot01 = glm::dot(v0, v1);
			double dot02 = glm::dot(v0, v2);
			double dot11 = glm::dot(v1, v1);
			double dot12 = glm::dot(v1, v2);

			double invDenom = 1.0f / (dot00 * dot11 - dot01 * dot01);

			double u = (dot11 * dot02 - dot01 * dot12) * invDenom;
			double v = (dot00 * dot12 - dot01 * dot02) * invDenom;

			inside = (u >= 0.0) && (v >= 0.0) && (u + v <= 1.0);
		}
		break;
		case 1:
		{
			//Same Side Method
			glm::dvec3 edge1 = vert2 - vert1;
			glm::dvec3 edge2 = vert3 - vert2;
			glm::dvec3 edge3 = vert1 - vert3;
			glm::dvec3 edge_normal_1 = glm::cross(edge1, normal);
			glm::dvec3 edge_normal_2 = glm::cross(edge2, normal);
			glm::dvec3 edge_normal_3 = glm::cross(edge3, normal);
			glm::dvec3 ep1 = p - vert1;
			glm::dvec3 ep2 = p - vert2;
			glm::dvec3 ep3 = p - vert3;

			inside = glm::dot(ep1, edge_normal_1) < 0.0 &&
				glm::dot(ep2, edge_normal_2) < 0.0 &&
				glm::dot(ep3, edge_normal_3) < 0.0;
		}
		break;
		default:
		{
			//Angle Method
			glm::dvec3 angleVec1 = glm::normalize(vert1 - p);
			glm::dvec3 angleVec2 = glm::normalize(vert2 - p);
			glm::dvec3 angleVec3 = glm::normalize(vert3 - p);

			double a = glm::acos(glm::dot(angleVec1, angleVec2));
			double b = glm::acos(glm::dot(angleVec2, angleVec3));
			double c = glm::acos(glm::dot(angleVec3, angleVec1));

			inside = glm::abs(2.0 * PI - (a + b + c)) <= 0.01;
		}
		break;
		}
		
		double epsilon = 0.0001;

		if (!inside)
		{
			//Check if p lies on edge
			glm::dvec3 e1 = glm::normalize(vert2 - vert1);
			glm::dvec3 e2 = glm::normalize(vert3 - vert1);
			glm::dvec3 e3 = glm::normalize(vert3 - vert2);
			glm::dvec3 pe1 = vert1 - p;
			glm::dvec3 pe2 = vert1 - p;
			glm::dvec3 pe3 = vert2 - p;

			double d1 = glm::length(glm::cross(e1, pe1));
			double d2 = glm::length(glm::cross(e2, pe2));
			double d3 = glm::length(glm::cross(e3, pe3));

			if (d1 < epsilon || d2 < epsilon || d3 < epsilon)
			{
				//std::cout << "Point on Edge" << std::endl;
				inside = true;
			}
		}

		if (inside)
		{
			for each (PointSide tpf in trianglePointsFound)
			{
				if (glm::distance(tpf.point, p) < epsilon)
				{
					if (glm::abs(side - tpf.side) < epsilon)
					{
						inside = false;
						//std::cout << "Point already taken into account" << std::endl;
					}
					else
					{
						//std::cout << "Point already taken into account but with different side" << std::endl;
					}
				}
			}
		}

		if (inside)
		{
			PointSide ps;
			ps.point = p;
			ps.side = side;
			trianglePointsFound.push_back(ps);
		}

		if (inside)
		{
			insideNum += side;
		}
	}

	//std::cout << "Is Point in Polyhedron insideNum = " << std::to_string(insideNum) << std::endl;

	return abs(insideNum) > 0.5f; //for arithmetic errors
}

void SceneObjectGeometry::calculateInnerSphereGreedy(const std::vector<unsigned int> index, const std::vector<glm::vec3> position, const unsigned int method, const unsigned int n)
{
	//std::cout << "Calculate InnerSphere" << std::endl;
	std::vector<TriangleFace> faces;
	faces.reserve(index.size() / 3);

	glm::vec3 largestFaceMeanPoint;
	float largestArea = 0.0f;
	float smallestArea = FLT_MAX;
	
	for (unsigned int i = 0; i < index.size(); i += 3)
	{
		TriangleFace f;

		f.vertices[0] = position[index[i + 0]];
		f.vertices[1] = position[index[i + 1]];
		f.vertices[2] = position[index[i + 2]];

		f.normal = glm::normalize(glm::cross(f.vertices[1] - f.vertices[0], f.vertices[2] - f.vertices[0]));

		float faceArea = glm::length(glm::cross(f.vertices[1] - f.vertices[0], f.vertices[2] - f.vertices[0]));
		if (faceArea > largestArea)
		{
			largestArea = faceArea;
			largestFaceMeanPoint = (f.vertices[0] + f.vertices[1] + f.vertices[2]) / 3.0f;
		}
		if (faceArea < smallestArea)
		{
			smallestArea = faceArea;
		}

		faces.push_back(f);
	}

	float bestRadius = 0.0f;
	glm::vec3 bestMeanPoint;
	bool foundInnerSphere = false;

	for (unsigned int iteration = 0; iteration < n; iteration++)
	{
		unsigned int i1 = (unsigned int)(rand() % position.size());
		unsigned int i2 = (unsigned int)(rand() % position.size());

		glm::vec3 meanPoint = (position[i1] + position[i2]) * 0.5f;

		float scale = glm::min(1.0f / smallestArea, 1000.0f);
		if (isPointInPolyhedron(meanPoint, faces, &largestFaceMeanPoint, scale, method))
		{
			foundInnerSphere = true;

			float radius = FLT_MAX;
			for (unsigned int i = 0; i < faces.size(); i++)
			{
				float s, t;
				glm::vec3 point = closestPointOnTriangle(faces[i].vertices[0], faces[i].vertices[1], faces[i].vertices[2], meanPoint, s, t);

				float dist = glm::distance(meanPoint, point);
				if (dist < radius)
				{
					radius = dist;
				}
			}

			if (radius > bestRadius)
			{
				//std::cout << "Better Sphere found at: [" << std::to_string(meanPoint.x) << ", " << std::to_string(meanPoint.y) << ", " << std::to_string(meanPoint.z) << "] with r=" << std::to_string(radius) << std::endl;
				bestRadius = radius;
				bestMeanPoint = meanPoint;
			}
		}
	}

	if (foundInnerSphere)
	{
		innerSphere = new BoundingSphere(bestRadius, bestMeanPoint);
	}
	else
	{
		std::cout << "No inner sphere found" << std::endl;
	}
}

BoundingObject* SceneObjectGeometry::getBoundingObject() const
{
	return boundingObject;
}

SceneObjectGeometry::BasicGeometry SceneObjectGeometry::getGeometryType() const
{
	return geometryType;
}

const glm::vec3& SceneObjectGeometry::getGeometryScale() const
{
	return geometryScale;
}
