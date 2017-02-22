/**
 * (c) Klemens Jahrmann
 * klemens.jahrmann@net1220.at
 */

#version 430

layout(points) in;
layout(triangle_strip, max_vertices = 6) out;

uniform mat4 projectionMatrix;
uniform float charSize;

in uint vCharIndex[];

out vec2 texCoord;

const float texSize = 1.0f / 16.0f;

void main()
{
	vec2 pos = gl_in[0].gl_Position.xy;
	vec2 texBasis = vec2(float(mod(vCharIndex[0], 16)) / 16.0f, float(vCharIndex[0] / 16) / 16.0f);

	gl_Position = projectionMatrix * vec4(pos, -1.0f, 1.0f);
	texCoord = vec2(texBasis.x, texBasis.y + texSize);
	EmitVertex();

	gl_Position = projectionMatrix * vec4(pos.x + charSize, pos.y + charSize, -1.0f, 1.0f);
	texCoord = vec2(texBasis.x + texSize, texBasis.y);
	EmitVertex();

	gl_Position = projectionMatrix * vec4(pos.x + charSize, pos.y, -1.0f, 1.0f);
	texCoord = vec2(texBasis.x + texSize, texBasis.y + texSize);
	EmitVertex();

	EndPrimitive();

	gl_Position = projectionMatrix * vec4(pos, -1.0f, 1.0f);
	texCoord = vec2(texBasis.x, texBasis.y + texSize);
	EmitVertex();

	gl_Position = projectionMatrix * vec4(pos.x, pos.y + charSize, -1.0f, 1.0f);
	texCoord = vec2(texBasis.x, texBasis.y);
	EmitVertex();

	gl_Position = projectionMatrix * vec4(pos.x + charSize, pos.y + charSize, -1.0f, 1.0f);
	texCoord = vec2(texBasis.x + texSize, texBasis.y);
	EmitVertex();

	EndPrimitive();
}