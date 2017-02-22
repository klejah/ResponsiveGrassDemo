/**
 * (c) Klemens Jahrmann
 * klemens.jahrmann@net1220.at
 */

#version 430

uniform sampler2D diffuseTexture;

uniform float ambientCoefficient;
uniform float diffuseCoefficient;
uniform float specularCoefficient;
uniform float specularHardness;

uniform vec3 camPos;
uniform vec3 lightDirection;
uniform vec3 lightColor;
uniform bool useDebugColor;
uniform bool useFlare;
uniform bool usePositionColor;

uniform vec2 nearFar;

in vec4 tePosition;
in vec3 teNormal;
in vec2 teUV;
in vec4 teDebug;

out vec4 out_color[2];

void main()
{
	vec4 color = texture(diffuseTexture, teUV);

	//color *= teUV.y + 0.2f; //pseudo AO

    if(usePositionColor)
    {
        float fac = tePosition.w;
        color.r += fract(fac * 10.0f) * 0.08f;
        color.b += fract(fac * 100.0f) * 0.08f;
        color *= clamp(fac, 0.9f,1.1f);
    }

	vec3 normal = teNormal;
		
	vec3 viewRay = camPos - tePosition.xyz;
    float viewDist = length(viewRay);
    viewRay = viewRay / viewDist;
    viewDist = (viewDist - nearFar.x) / (nearFar.y - nearFar.x);
	vec3 lightRay = normalize(-lightDirection);

	vec3 halfVec = normalize((lightRay + viewRay) / 2.0f);

	float NdL = clamp(dot(normal, lightRay),0.0f,1.0f);
	float NdH = clamp(dot(normal, halfVec),0.0f,1.0f);

	vec3 aCol = max(ambientCoefficient * color.rgb,0.0f);
	vec3 dCol = max(diffuseCoefficient * NdL * color.rgb,0.0f);
	vec3 sCol = max(specularCoefficient * pow(NdH,specularHardness) * color.rgb,0.0f) * teUV.y;
	
	aCol *= lightColor;
	dCol *= lightColor;
	sCol *= lightColor;
	
	if(!useDebugColor)
	{
		if(!useFlare)
		{
			out_color[0] = vec4(aCol + dCol + sCol, 1.0f);
		}
		else
		{
			out_color[0] = vec4(aCol + dCol + sCol, 1.0f);
			vec3 overflow = max(out_color[0].rgb - vec3(0.8f),0.0f) * 0.5f;
			out_color[0].r += overflow.g + overflow.b;
			out_color[0].g += overflow.r + overflow.b;
			out_color[0].b += (overflow.r + overflow.g) * 0.85f;
		}
	}
	else
	{
		out_color[0] = vec4(teDebug.rgb,1.0);
		//out_color[0] = vec4(abs(teNormal.rgb),1.0);
		//out_color[0] = vec4(teDebug.a,1.0f - teDebug.a,0.0f,1.0f);
	}

    out_color[1] = vec4(viewDist);
}