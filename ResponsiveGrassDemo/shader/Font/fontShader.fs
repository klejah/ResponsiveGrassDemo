/**
 * (c) Klemens Jahrmann
 * klemens.jahrmann@net1220.at
 */

#version 430

uniform sampler2D glyphTexture;
uniform vec4 color;

in vec2 texCoord;
out vec4 out_color;

void main()
{
	out_color = vec4(color.rgb, texture2D(glyphTexture, texCoord).r);
}