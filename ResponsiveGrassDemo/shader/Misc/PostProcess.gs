/**
 * (c) Klemens Jahrmann
 * klemens.jahrmann@net1220.at
 */

#version 420

layout(points) in;
layout(triangle_strip, max_vertices = 3) out;

out vec2 uv;

void main()
{
	uv = vec2(0.0f, 0.0f);
	gl_Position = vec4(-1.0f, -1.0f, 0.0f, 1.0f);
	EmitVertex();
	uv = vec2(2.0f, 0.0f);
	gl_Position = vec4(3.0f, -1.0f, 0.0f, 1.0f);
	EmitVertex();
	uv = vec2(0.0f, 2.0f);
	gl_Position = vec4(-1.0f, 3.0f, 0.0f, 1.0f);
	EmitVertex();
	EndPrimitive();
}