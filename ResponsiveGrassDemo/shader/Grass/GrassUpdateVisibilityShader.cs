/**
 * (c) Klemens Jahrmann
 * klemens.jahrmann@net1220.at
 */

#version 430

layout(std430, binding=POSITION_LOCATION) buffer grassPos { //xyz ground + dirAlpha
    vec4 p[];
};

layout(std430, binding=V1_LOCATION) buffer grassV1 { //xyz v1 + height
    vec4 v1[];
};

layout(std430, binding=V2_LOCATION) buffer grassV2 { //xyz v2 + width
    vec4 v2[];
};

layout(std430, binding=ATTR_LOCATION) buffer grassAttr { //xyz bladeUp + bend
    vec4 attr[];
};

layout(std430, binding=DEBUG_LOCATION) buffer grassDebug {
    vec4 debug[];
};

layout(std430, binding=INDEX_LOCATION) buffer index {
    uint ind[];
};

layout(std430, binding=ATOMIC_COUNTER_LOCATION) buffer indirectBuffer {
    uint indirect[];
};

layout(local_size_x=MAX_WORK_GROUP_SIZE_X, local_size_y=1, local_size_z=1) in;

//layout(binding=ATOMIC_COUNTER_LOCATION, offset=0) uniform atomic_uint visibleBladeCount;

//Height Map
uniform bool useHeightMap;
uniform sampler2D heightMap;
uniform vec4 heightMapBounds; //xMin zMin xLength zLength

//Inner Sphere
uniform uint innerSphereAmount;
uniform vec4 innerSphere[MAX_AMOUNT_INNER_SPHERES];

//Depth Texture
uniform bool doDepthBufferCulling;
uniform sampler2DMS depthTexture;
uniform vec2 widthHeight;

//Misc
uniform uint amountBlades;
uniform mat4 modelMatrix;
uniform mat3 invTransModelMatrix;
uniform mat4 vpMatrix;
uniform vec2 nearFar;
uniform vec3 camPos;
uniform float maxDistance;
uniform bool doDepthCulling;
uniform float depthCullLevel;
uniform bool doVFC;
uniform bool doOrientationCulling;

void main()
{
    uint id = gl_GlobalInvocationID.x; //for grass blade

    if(id == 0)
    {
        atomicExchange(indirect[0], 0);
    }

    barrier();

    if(id < amountBlades)
    {
        float dirAlpha = p[id].w;
        vec3 pos = (modelMatrix * vec4(p[id].xyz,1.0f)).xyz;
        vec3 wV1 = (modelMatrix * vec4(v1[id].xyz,1.0f)).xyz;
        vec3 wV2 = (modelMatrix * vec4(v2[id].xyz,1.0f)).xyz;

        //direction of the blade
        vec3 bladeUp = attr[id].xyz;
        float sd = sin(dirAlpha);
        float cd = cos(dirAlpha);
        vec3 tmp = normalize(vec3(sd, sd + cd, cd)); //arbitrary vector for finding normal vector
        vec3 bladeDir = normalize(cross(bladeUp, tmp));
        vec3 bladeFront = normalize(cross(bladeUp, bladeDir));

        bladeUp = normalize(invTransModelMatrix * bladeUp);
        bladeDir = normalize(invTransModelMatrix * bladeDir);
        bladeFront = normalize(invTransModelMatrix * bladeFront);

        if(useHeightMap)
        {
            vec2 heightMapRead = textureLod(heightMap, clamp((pos.xz - heightMapBounds.xy) / heightMapBounds.zw, 0.0f, 1.0f), 0.0f).xy;
            float mapHeight = heightMapRead.x * heightMapRead.y;
            pos += bladeUp * mapHeight;
            wV1 += bladeUp * mapHeight;
            wV2 += bladeUp * mapHeight;
        }

        vec3 midPoint = 0.25f * pos + 0.5f * wV1 + 0.25f * wV2;

        //Visibility Check
        vec3 camDir = pos - camPos;
        vec3 camDirProj = camDir - dot(camDir, bladeUp) * bladeUp;
        float distance = length(camDir);
        vec3 camDirNorm = camDir / distance;

        //Orientation
        if(doOrientationCulling && abs(dot(camDirNorm, bladeDir)) >= 0.9f)
        {
            return;
        }

        //View Frustum Culling
        vec4 pNDC = vpMatrix * vec4(pos,1.0f);
        vec4 midNDC = vpMatrix * vec4(midPoint,1.0f);
        vec4 v2NDC = vpMatrix * vec4(wV2,1.0f);
        if(doVFC)
        {
            //Add tolerance
            float tolerance = 0.5f;
            vec4 pNDCTol = pNDC;
            vec4 midNDCTol = midNDC;
            vec4 v2NDCTol = v2NDC;
            pNDCTol.w += tolerance;
            midNDCTol.w += tolerance;
            v2NDCTol.w += tolerance;
            float nearTol = nearFar.x - 2.0f * tolerance;
            float farTol = nearFar.y + 2.0f * tolerance;

            if(!(
                pNDCTol.x > -pNDCTol.w && pNDCTol.x < pNDCTol.w && 
		        pNDCTol.y > -pNDCTol.w && pNDCTol.y < pNDCTol.w &&
                pNDCTol.z > -pNDCTol.w && pNDCTol.z < pNDCTol.w || 
		        //pNDCTol.w > nearTol && pNDCTol.w < farTol || 
                midNDCTol.x > -midNDCTol.w && midNDCTol.x < midNDCTol.w &&
                midNDCTol.y > -midNDCTol.w && midNDCTol.y < midNDCTol.w &&
                midNDCTol.z > -midNDCTol.w && midNDCTol.z < midNDCTol.w ||
                //midNDCTol.w > nearTol && midNDCTol.w < farTol ||
                v2NDCTol.x > -v2NDCTol.w && v2NDCTol.x < v2NDCTol.w && 
                v2NDCTol.y > -v2NDCTol.w && v2NDCTol.y < v2NDCTol.w &&
                v2NDCTol.y > -v2NDCTol.w && v2NDCTol.y < v2NDCTol.w))
                //v2NDCTol.w > nearTol && v2NDCTol.w < farTol))
            {
                return;
            }
        }

        //Depth culling
        if(doDepthCulling)
        {
            float projDistance = length(camDirProj);
            uint value = uint(ceil(max((1.0f - projDistance / maxDistance),0.0f) * depthCullLevel));
        
            if(mod(id,uint(depthCullLevel)) >= value)
            {
                return;
            }

            //debug[id] = vec4(float(value) / depthCullLevel, 1.0f - float(value) / depthCullLevel, 0.0f, 1.0f);
        }

        vec3 camDirMid = midPoint - camPos;
        float distanceMid = length(camDirMid);
        vec3 camDirV2 = wV2 - camPos;
        float distanceV2 = length(camDirV2);

        //Inner Sphere culling
        if(innerSphereAmount > 0)
        {
            vec3 camDirMidNorm = camDirMid / distanceMid;
            vec3 camDirV2Norm = camDirV2 / distanceV2;
            for(uint i = 0; i < innerSphereAmount; i++)
            {
                vec3 sCenter = innerSphere[i].xyz;
                float r = innerSphere[i].w;

                vec3 camPosSCenter = camPos - sCenter;

                //Check if sphere is in front of blade
                float dotP = dot(camDirNorm, -camPosSCenter);
                float dotMid = dot(camDirMidNorm, -camPosSCenter);
                float dotV2 = dot(camDirV2Norm, -camPosSCenter);
                if(dotP < 0.0f || dotMid < 0.0f || dotV2 < 0.0f || dotP > distance || dotMid > distanceMid || dotV2 > distanceV2)
                {
                    continue;
                }

                //http://mathworld.wolfram.com/Point-LineDistance3-Dimensional.html (|x2 - x1| is in this case allways 1 since vector is normalized)
                float dP = length(cross(camDirNorm, camPosSCenter));
                float dMid = length(cross(camDirMidNorm, camPosSCenter));
                float dV2 = length(cross(camDirV2Norm, camPosSCenter));

                if(dP <= r && dMid <= r && dV2 <= r)
                {
                    return;
                }
            }
        }

        //Depth buffer culling
        if(doDepthBufferCulling)
        {
            vec4 p_begin_NDC = vpMatrix * vec4(0.81f * pos + 0.18f * wV1 + 0.01 * wV2,1.0f);
            //vec2 uvP = (pNDC.xy / pNDC.w) * 0.5f + 0.5f;
            vec2 uvP = (p_begin_NDC.xy / p_begin_NDC.w) * 0.5f + 0.5f;
            vec2 uvMid = (midNDC.xy / midNDC.w) * 0.5f + 0.5f;
            vec2 uvV2 = (v2NDC.xy / v2NDC.w) * 0.5f + 0.5f;
            const float depthP   = texelFetch(depthTexture, ivec2(uvP   * widthHeight), 1).x;
            const float depthMid = texelFetch(depthTexture, ivec2(uvMid * widthHeight), 2).x;
            const float depthV2  = texelFetch(depthTexture, ivec2(uvV2  * widthHeight), 3).x;
            const float nearFarRange = 1.0f / (nearFar.y - nearFar.x);
            //float tol = 0.1f;
            //float tol = 0.05f;
            float tol = 0.01f;
            float nearPlusTol = nearFar.x;
            //depthP =  (depthP+tol)  * nearFarRange + nearPlusTol;
            //depthMid = (depthMid+tol) * nearFarRange + nearPlusTol;
            //depthV2 = (depthV2+tol) * nearFarRange + nearPlusTol;
            const float dLD = (distance - nearFar.x) * nearFarRange;
            const float mLD = (distanceMid - nearFar.x) * nearFarRange;
            const float vLD = (distanceV2 - nearFar.x) * nearFarRange;
            //debug[id].y = depthP-distance;
            //debug[id].z = (distance - nearFar.x) / nearFarRange;
            debug[id].xyz = vec3(dLD - depthP, mLD - depthMid, vLD - depthV2);
            //if(depthP < distance && depthMid < distanceMid && depthV2 < distanceV2)
            if(depthP + tol < dLD && depthMid + tol < mLD && depthV2 + tol < vLD)
            {
                return;
            }
        }

        //uint index = atomicCounterIncrement(visibleBladeCount);
        uint index = atomicAdd(indirect[0], 1);
        ind[index] = id;
    }
}