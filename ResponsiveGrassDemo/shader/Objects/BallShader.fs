/**
 * (c) Klemens Jahrmann
 * klemens.jahrmann@net1220.at
 */

#version 430

#define MAX_SPEC_EXP 150.0f

uniform bool hasUVs;
uniform bool hasNormals;
uniform bool hasTangentsAndBitangents;

uniform bool hasNormalMap;
uniform sampler2D normalMap;
uniform bool hasDiffuseTexture;
uniform sampler2D diffuseTexture;
uniform bool hasSpecularTexture;
uniform sampler2D specularTexture;
uniform bool hasAlphaTexture;
uniform sampler2D alphaTexture;

uniform vec4 textureTileAndOffset;

uniform float ambientCoefficient;
uniform float diffuseCoefficient;
uniform float specularCoefficient;
uniform float specularExponent;

uniform vec2 nearFar;

in vec3 teViewRay;
in vec3 teLightRay;
in vec3 teReflectionRay;

in vec3 teNormal;
in vec2 teUv;
in vec3 teTangent;
in vec3 teBitangent;

layout(location = 0) out vec4 out_color;
layout(location = 1) out vec4 linear_depth;

void main()
{
    linear_depth.x = (length(teViewRay) - nearFar.x) / (nearFar.y - nearFar.x);
    if(hasNormals)
    {
        vec3 normal = normalize(teNormal);
        if(hasUVs && hasDiffuseTexture)
        {
            vec4 color = texture(diffuseTexture, teUv * textureTileAndOffset.xy + textureTileAndOffset.zw);

            vec3 lightRay, reflectionRay, viewRay;
            if(hasTangentsAndBitangents && hasNormalMap)
            {
                normal = normalize(texture(normalMap, teUv * textureTileAndOffset.xy + textureTileAndOffset.zw).xyz * 2.0f - 1.0f);
                vec3 t = normalize(teTangent);
                vec3 b = normalize(teBitangent);
                viewRay.x = dot(teViewRay, t);
                viewRay.y = dot(teViewRay, b);
                viewRay.z = dot(teViewRay, normal);
                lightRay.x = dot(teLightRay, t);
                lightRay.y = dot(teLightRay, b);
                lightRay.z = dot(teLightRay, normal);
                reflectionRay.x = dot(teReflectionRay, t);
                reflectionRay.y = dot(teReflectionRay, b);
                reflectionRay.z = dot(teReflectionRay, normal);
            }
            else
            {
                viewRay = normalize(teViewRay);
                lightRay = normalize(teLightRay);
                reflectionRay = normalize(teReflectionRay);
            }

		    vec4 ambient = ambientCoefficient * color;
		    vec4 diffuse = diffuseCoefficient * max(dot(normal, lightRay),0.0f) * color;

            float exp = specularExponent;
            if(hasSpecularTexture)
            {
                vec4 specTex = texture(specularTexture, teUv * textureTileAndOffset.xy + textureTileAndOffset.zw);
                color = vec4(specTex.xyz, 1.0f);
                exp = specTex.a * MAX_SPEC_EXP;
            }

		    vec4 specular = specularCoefficient * max(pow(dot(reflectionRay, viewRay),exp),0.0f) * color;

            out_color = ambient + diffuse + specular;

            if(hasAlphaTexture)
            {
                out_color.a = texture(alphaTexture, teUv * textureTileAndOffset.xy + textureTileAndOffset.zw).x;
            }
        }
        else
        {
            if(hasUVs && hasNormalMap)
            {
                normal = normalize(texture(normalMap, teUv * textureTileAndOffset.xy + textureTileAndOffset.zw).xyz * 2.0f - 1.0f);
            }
            
            out_color = vec4(normal * 0.5f + 0.5f, 1.0f);
        }
    }
    else if(hasUVs && hasDiffuseTexture)
    {
        out_color = texture(diffuseTexture, teUv * textureTileAndOffset.xy + textureTileAndOffset.zw);

        if(hasAlphaTexture)
        {
            out_color.a = texture(alphaTexture, teUv * textureTileAndOffset.xy + textureTileAndOffset.zw).x;
        }
    }
    else
    {
        out_color = vec4(1.0f,1.0f,1.0f,1.0f);
    }
}