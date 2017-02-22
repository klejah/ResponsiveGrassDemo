/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "GrassObject.h"

GrassObject::GrassObject(const std::string& filename, const SceneObjectGeometry::BasicGeometry type, const glm::vec3& scale, const bool generatePhysicalObject, const float staticFriction, const float dynamicFriction, const float restitution, const float angularDamping, const float physicalDensity, const std::vector<GrassCreateBladeParams>& params, Shader* drawShader, const bool isStatic, glm::mat4& position, const glm::vec4& tessellationProps, const glm::vec4& textureTileAndOffset, const glm::vec4& heightMapTileAndOffset, float ambientCoefficient, float diffuseCoefficient, float specularCoefficient, float specularExponent)
	: SceneObject(drawShader, 0, isStatic, position, tessellationProps, textureTileAndOffset, heightMapTileAndOffset, ambientCoefficient, diffuseCoefficient, specularCoefficient, specularExponent), grass(0), faces()
{
	AssimpImporter::ImportModel* model = AssimpImporter::importModel(filename);
	if (model == 0)
	{
		std::cout << "ERROR GrassObject: Could not load model!" << std::endl;
		return;
	}

	std::vector<AssimpImporter::ImportModel*> models;
	std::vector<glm::mat4> modelMatrices;
	models.push_back(model);
	modelMatrices.push_back(glm::mat4(1.0f));
	float xMin = FLT_MAX;
	float yMin = FLT_MAX;
	float zMin = FLT_MAX;
	float xMax = -FLT_MAX;
	float yMax = -FLT_MAX;
	float zMax = -FLT_MAX;
	for (unsigned int i = 0; i < models.size(); i++)
	{
		AssimpImporter::ImportModel* m = models[i];
		glm::mat4 modMatrix = modelMatrices[i];
		glm::mat3 invTrans = glm::inverse(glm::transpose(glm::mat3(modMatrix)));

		for (unsigned int j = 0; j < m->index.size(); j += 3)
		{
			Geometry::Vertex v1;
			v1.position = glm::vec3(modMatrix * glm::vec4(m->position[m->index[j]] * scale, 1.0f));
			v1.normal = invTrans * m->normal[m->index[j]];
			if (m->tangent.size() > 0)
				v1.tangent = invTrans * m->tangent[m->index[j]];
			if (m->bitangent.size() > 0)
				v1.bitangent = invTrans * m->bitangent[m->index[j]];
			Geometry::Vertex v2;
			v2.position = glm::vec3(modMatrix * glm::vec4(m->position[m->index[j + 1]] * scale, 1.0f));
			v2.normal = invTrans * m->normal[m->index[j + 1]];
			if (m->tangent.size() > 0)
				v2.tangent = invTrans * m->tangent[m->index[j + 1]];
			if (m->bitangent.size() > 0)
				v2.bitangent = invTrans * m->bitangent[m->index[j + 1]];
			Geometry::Vertex v3;
			v3.position = glm::vec3(modMatrix * glm::vec4(m->position[m->index[j + 2]] * scale, 1.0f));
			v3.normal = invTrans * m->normal[m->index[j + 2]];
			if (m->tangent.size() > 0)
				v3.tangent = invTrans * m->tangent[m->index[j + 2]];
			if (m->bitangent.size() > 0)
				v3.bitangent = invTrans * m->bitangent[m->index[j + 2]];

			xMin = glm::min(xMin, glm::min(v1.position.x, glm::min(v2.position.x, v3.position.x))); 
			yMin = glm::min(yMin, glm::min(v1.position.y, glm::min(v2.position.y, v3.position.y)));	
			zMin = glm::min(zMin, glm::min(v1.position.z, glm::min(v2.position.z, v3.position.z))); 
			xMax = glm::max(xMax, glm::max(v1.position.x, glm::max(v2.position.x, v3.position.x)));	
			yMax = glm::max(yMax, glm::max(v1.position.y, glm::max(v2.position.y, v3.position.y))); 
			zMax = glm::max(zMax, glm::max(v1.position.z, glm::max(v2.position.z, v3.position.z)));

			Geometry::TriangleFace f(v1,v2,v3);
			faces.push_back(f);
		}

		for (unsigned int j = 0; j < m->children.size(); j++)
		{
			models.push_back(m->children[j]);
			modelMatrices.push_back(m->children[j]->transform * modMatrix);
		}
	}

	Clock c;
	c.Tick();
	grass = new Grass(params, faces);
	c.Tick();
	std::cout << "Grass generation time: " << std::to_string(c.LastFrameTime()) << std::endl;
	grass->parentObject = this;

	attachGeometry(new SceneObjectGeometry(model, type, scale, false, false, 0, generatePhysicalObject, staticFriction, dynamicFriction, restitution, angularDamping, physicalDensity));

	float maxBladeHeight = -FLT_MAX;
	for each(auto p in params)
	{
		maxBladeHeight = glm::max(maxBladeHeight, p.bladeMaxHeight);
	}

	bounds->inflate(glm::vec3(maxBladeHeight, maxBladeHeight, maxBladeHeight));
}

GrassObject::GrassObject(const GrassObject& other) : SceneObject(other), grass(0), faces(other.faces)
{
	if (other.grass != 0)
	{
		grass = new Grass(*other.grass);
	}
}

GrassObject::GrassObject(const GrassObject& other, const std::vector<GrassCreateBladeParams>& params) : SceneObject(other), grass(0), faces(other.faces)
{
	grass = new Grass(params, faces);
	grass->parentObject = this;

	bounds = geometry->getBoundingObject();

	float maxBladeHeight = -FLT_MAX;
	for each(auto p in params)
	{
		maxBladeHeight = glm::max(maxBladeHeight, p.bladeMaxHeight);
	}

	bounds->inflate(glm::vec3(maxBladeHeight, maxBladeHeight, maxBladeHeight));
}

GrassObject::~GrassObject()
{

}

void GrassObject::update(const Camera& cam, const Clock& time)
{
	SceneObject::update(cam, time);
}

void GrassObject::draw(const Camera& cam)
{
	if (visible)
	{
		SceneObject::draw(cam);
	}
}

void GrassObject::drawGrass(const Camera& cam, const Clock& time)
{
	if (visible)
	{
		grass->Draw((float)time.LastFrameTime(), cam);
	}
}

void GrassObject::parentDraw(glm::mat4& parentMatrix)
{
	if (visible)
	{
		SceneObject::parentDraw(parentMatrix);
	}
}