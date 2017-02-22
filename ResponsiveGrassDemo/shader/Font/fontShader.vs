/**
 * (c) Klemens Jahrmann
 * klemens.jahrmann@net1220.at
 */

#version 430

in vec2 position;
in uint charIndex;

out uint vCharIndex;

void main()
{
	vCharIndex = charIndex;

	gl_Position = vec4(position,0.0f,0.0f);
}