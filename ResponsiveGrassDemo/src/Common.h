/**
* (c) Klemens Jahrmann
* klemens.jahrmann@net1220.at
*/

#ifndef COMMON_H
#define COMMON_H

#define SHADERPATH std::string("../shader/")
#define RESSOURCEPATH std::string("../ressources/")
#define TEXTUREPATH std::string(RESSOURCEPATH) + "Textures/"
#define MODELPATH std::string(RESSOURCEPATH) + "Models/"
#define GENERATEDFILESPATH std::string(RESSOURCEPATH) + "Generated/"

#define DEBUG true

#include "GL\glew.h"
#define GLM_FORCE_RADIANS
#define GLM_SWIZZLE
#include "glm\glm.hpp"

#define MS_SAMPLES 8

#define LIGHTDIR glm::vec3(0.4, -1.0f, 0.2)

#define PI 3.1415926535897932384626433832795
#define PI_2 1.5707963267948966192313216916398
#define PI_F 3.1415926535897932384626433832795f
#define PI_2_F 1.5707963267948966192313216916398f

#define SHADER_POSITION_LOCATION 0
#define SHADER_POSITION_STRING std::string("SHADER_POSITION_LOCATION")
#define SHADER_NORMAL_LOCATION 1
#define SHADER_NORMAL_STRING std::string("SHADER_NORMAL_LOCATION")
#define SHADER_UV_LOCATION 2
#define SHADER_UV_STRING std::string("SHADER_UV_LOCATION")
#define SHADER_TANGENT_LOCATION 3
#define SHADER_TANGENT_STRING std::string("SHADER_TANGENT_LOCATION")
#define SHADER_BITANGENT_LOCATION 4
#define SHADER_BITANGENT_STRING std::string("SHADER_BITANGENT_LOCATION")

void fetchGLError();

glm::vec3 closestPointOnTriangle(const glm::vec3& v1, const glm::vec3& v2, const glm::vec3& v3, const glm::vec3& sourcePosition, float& s, float& t);

inline float random()
{
	return ((float)rand() / (RAND_MAX));
}

#endif