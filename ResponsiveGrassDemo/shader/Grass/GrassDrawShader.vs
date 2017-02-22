/**
 * (c) Klemens Jahrmann
 * klemens.jahrmann@net1220.at
 */

#version 430

uniform mat4 modelMatrix;
uniform bool useHeightMap;
uniform sampler2D heightMap;
uniform vec4 heightMapBounds; //xMin zMin xLength zLength

layout(location = POSITION_LOCATION) in vec4 position;
layout(location = V1_LOCATION) in vec4 v1;
layout(location = V2_LOCATION) in vec4 v2;
layout(location = DEBUG_LOCATION) in vec4 debug;

out vec4 vV1;
out vec4 vV2;
out vec4 vDebug;
out vec3 vBladeDir;
out vec3 vBladeUp;

void main()
{
	vec4 pos = modelMatrix * vec4(position.xyz,1.0f);
	vV1 = vec4((modelMatrix * vec4(v1.xyz,1.0f)).xyz, v1.w);
	vV2 = vec4((modelMatrix * vec4(v2.xyz,1.0f)).xyz, v2.w);

	vBladeUp = normalize(vV1.xyz - pos.xyz);

	if(useHeightMap)
	{
		vec2 heightMapRead = textureLod(heightMap, clamp((pos.xz - heightMapBounds.xy) / heightMapBounds.zw, 0.0f, 1.0f), 0.0f).xy;
		float height = heightMapRead.x * heightMapRead.y;
		pos.xyz += vBladeUp * height;
		vV1.xyz += vBladeUp * height;
		vV2.xyz += vBladeUp * height;
	}

	gl_Position = pos;
	vDebug = debug;

	float dir = position.w;
	float sd = sin(dir);
	float cd = cos(dir);
	vec3 tmp = normalize(vec3(sd, sd + cd, cd));
    vBladeDir = normalize(cross(vBladeUp, tmp));
}