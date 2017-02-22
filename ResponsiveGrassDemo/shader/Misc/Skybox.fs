/**
 * (c) Klemens Jahrmann
 * klemens.jahrmann@net1220.at
 */

#version 430

uniform samplerCube cubeTex;

smooth in vec3 eyeDir;

out vec4 out_color[2];

void main()
{
    out_color[0] = texture(cubeTex, eyeDir);
    out_color[1] = vec4(1.0f);
}