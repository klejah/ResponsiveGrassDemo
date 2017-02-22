/**
 * (c) Klemens Jahrmann
 * klemens.jahrmann@net1220.at
 */

#version 430

uniform mat4 invProjectionMatrix;
uniform mat4 viewMatrix;

in vec2 pos;

smooth out vec3 eyeDir;

void main()
{
	vec4 aPos = vec4(pos, 0.999999f, 1.0f);
	mat3 invViewMatrix = transpose(mat3(viewMatrix));
	vec3 unprojected = (invProjectionMatrix * aPos).xyz;
	eyeDir = invViewMatrix * unprojected;

	gl_Position = aPos;
}