/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#include "Common.h"
#include <iostream>

void fetchGLError()
{
	GLenum error = glGetError();

	if (error != GL_NO_ERROR) {
		switch (error) {
		case GL_INVALID_ENUM:
			std::cerr << "GL: enum argument out of range." << std::endl;
			break;
		case GL_INVALID_VALUE:
			std::cerr << "GL: Numeric argument out of range." << std::endl;
			break;
		case GL_INVALID_OPERATION:
			std::cerr << "GL: Operation illegal in current state." << std::endl;
			break;
		case GL_INVALID_FRAMEBUFFER_OPERATION:
			std::cerr << "GL: Framebuffer object is not complete." << std::endl;
			break;
		case GL_OUT_OF_MEMORY:
			std::cerr << "GL: Not enough memory left to execute command." << std::endl;
			break;
		default:
			std::cerr << "GL: Unknown error." << std::endl;
		}
	}
}

glm::vec3 closestPointOnTriangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& sourcePosition, float& s, float& t)
{
	glm::vec3 edge0 = v2 - v1;
	glm::vec3 edge1 = v3 - v1;
	glm::vec3 v0 = v1 - sourcePosition;

	float a = glm::dot(edge0, edge0);
	float b = glm::dot(edge0, edge1);
	float c = glm::dot(edge1, edge1);
	float d = glm::dot(edge0, v0);
	float e = glm::dot(edge1, v0);

	float det = a*c - b*b;
	s = b*e - c*d;
	t = b*d - a*e;

	if (s + t < det)
	{
		if (s < 0.0f)
		{
			if (t < 0.0f)
			{
				if (d < 0.0f)
				{
					s = glm::clamp(-d / a, 0.0f, 1.0f);
					t = 0.0f;
				}
				else
				{
					s = 0.0f;
					t = glm::clamp(-e / c, 0.0f, 1.0f);
				}
			}
			else
			{
				s = 0.0f;
				t = glm::clamp(-e / c, 0.0f, 1.0f);
			}
		}
		else if (t < 0.0f)
		{
			s = glm::clamp(-d / a, 0.0f, 1.0f);
			t = 0.0f;
		}
		else
		{
			float invDet = 1.0f / det;
			s *= invDet;
			t *= invDet;
		}
	}
	else
	{
		if (s < 0.0f)
		{
			float tmp0 = b + d;
			float tmp1 = c + e;
			if (tmp1 > tmp0)
			{
				float numer = tmp1 - tmp0;
				float denom = a - 2.0f * b + c;
				s = glm::clamp(numer / denom, 0.0f, 1.0f);
				t = 1.0f - s;
			}
			else
			{
				t = glm::clamp(-e / c, 0.0f, 1.0f);
				s = 0.0f;
			}
		}
		else if (t < 0.0f)
		{
			if (a + d > b + e)
			{
				float numer = c + e - b - d;
				float denom = a - 2.0f * b + c;
				s = glm::clamp(numer / denom, 0.0f, 1.0f);
				t = 1.0f - s;
			}
			else
			{
				s = glm::clamp(-e / c, 0.0f, 1.0f);
				t = 0.0f;
			}
		}
		else
		{
			float numer = c + e - b - d;
			float denom = a - 2.0f * b + c;
			s = glm::clamp(numer / denom, 0.0f, 1.0f);
			t = 1.0f - s;
		}
	}

	return v1 + s * edge0 + t * edge1;
}