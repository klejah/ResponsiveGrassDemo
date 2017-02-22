/**
 * (c) Klemens Jahrmann
 * klemens.jahrmann@net1220.at
 */

#version 430

layout(location = SHADER_POSITION_LOCATION)  in vec3 position;
layout(location = SHADER_NORMAL_LOCATION)    in vec3 normal;
layout(location = SHADER_UV_LOCATION)        in vec2 uv;
layout(location = SHADER_TANGENT_LOCATION)   in vec3 tangent;
layout(location = SHADER_BITANGENT_LOCATION) in vec3 bitangent;

out vec3 vPosition;
out vec3 vNormal;
out vec2 vUv;
out vec3 vTangent;
out vec3 vBitangent;

void main()
{
	vPosition = position;
	vNormal = normal;
	vUv = uv;
	vTangent = tangent;
	vBitangent = bitangent;
}