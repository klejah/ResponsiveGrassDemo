/**
 * (c) Klemens Jahrmann
 * klemens.jahrmann@net1220.at
 */

#version 420

uniform sampler2DMS sourceTexture;
uniform vec2 screenSize;

in vec2 uv;

out vec4 out_color;

void main()
{
    ivec2 lookup = ivec2(screenSize * uv);
    vec3 col = texelFetch(sourceTexture, lookup, gl_SampleID).rgb;
    out_color = vec4(col, 1.0f);
}