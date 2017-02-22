/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef GEOMETRY_FACE_H
#define GEOMETRY_FACE_H

#include "Common.h"

namespace Geometry
{
	struct Vertex
	{
		glm::vec3 position;
		glm::vec3 normal;
		glm::vec3 tangent;
		glm::vec3 bitangent;
	};

	class TriangleFace
	{
	public:
		Vertex vertices[3];
		glm::vec3 faceNormal;
		glm::vec3 faceCenterPosition;
		float area;

		TriangleFace(const Vertex v1, const Vertex v2, const Vertex v3) : vertices()
		{
			vertices[0] = v1;
			vertices[1] = v2;
			vertices[2] = v3;
			CalculateProperties();
		}

	public:
		void CalculateProperties()
		{
			faceNormal = glm::cross(vertices[1].position - vertices[0].position, vertices[2].position - vertices[0].position);
			area = glm::length(faceNormal) * 0.5f;
			faceNormal = glm::normalize(faceNormal);
			faceCenterPosition = (vertices[0].position + vertices[1].position + vertices[2].position) / 3.0f;
		}
	};
}

#endif