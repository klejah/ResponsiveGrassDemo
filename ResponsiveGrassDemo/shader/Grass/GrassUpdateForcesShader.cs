/**
 * (c) Klemens Jahrmann
 * klemens.jahrmann@net1220.at
 */

#version 430

layout(std430, binding=POSITION_LOCATION) buffer grassPos { //xyz ground + dirAlpha
    vec4 p[];
};

layout(std430, binding=V1_LOCATION) buffer grassV1 { //xyz v1 + height
    vec4 bv1[];
};

layout(std430, binding=V2_LOCATION) buffer grassV2 { //xyz v2 + width
    vec4 bv2[];
};

layout(std430, binding=ATTR_LOCATION) buffer grassAttr { //xyz bladeUp + bend
    vec4 attr[];
};

layout(std430, binding=DEBUG_LOCATION) buffer grassDebug {
    vec4 debug[];
};

layout(local_size_x=MAX_WORK_GROUP_SIZE_X, local_size_y=1, local_size_z=1) in;

//Pressure Map
layout(binding = 0, rgba32f) uniform image2D pressureMap;
uniform ivec2 pressureMapOffset;
uniform uint pressureMapBlockSize;

//Height Map
uniform bool useHeightMap;
uniform sampler2D heightMap;
uniform vec4 heightMapBounds; //xMin zMin xLength zLength

uniform uint amountBlades;
uniform float dt;
uniform mat4 modelMatrix;
uniform mat4 invModelMatrix;
uniform mat3 invTransModelMatrix;

//forces
uniform uint windType;
uniform vec4 windData;
uniform vec4 gravityVec;
uniform vec4 gravityPoint;
uniform float useGravityPoint;

uniform vec4 sphereCollider[MAX_AMOUNT_SPHERE_COLLIDER];
uniform uint amountSphereCollider;

float invHeight;
vec3 groundPosV2;

vec3 CalculateV1(in vec3 groundPos, in vec3 v2, in vec3 bladeUp, in float height)
{
    vec3 g = groundPosV2 - dot(groundPosV2, bladeUp) * bladeUp;
    float v2ratio = abs(length(g) * invHeight);
    float fac = max(1.0f - v2ratio, 0.05f * max(v2ratio, 1.0f));
    return groundPos + bladeUp * height * fac;
}

void MakePersistentLength(in vec3 groundPos, inout vec3 v1, inout vec3 v2, in float height)
{
    //Persistent length
    vec3 v01 = v1 - groundPos;
    vec3 v12 = v2 - v1;
    float lv01 = length(v01);
    float lv12 = length(v12);

    float L1 = lv01 + lv12;
    float L0 = length(groundPosV2);
    float L = (2.0f * L0 + L1) / 3.0f; //http://steve.hollasch.net/cgindex/curves/cbezarclen.html

    float ldiff = height / L;
    v01 = v01 * ldiff;
    v12 = v12 * ldiff;
    v1 = groundPos + v01;
    v2 = v1 + v12;
}

void EnsureValidV2Pos(inout vec3 v2, in vec3 bladeUp)
{
    v2 += bladeUp * -min(dot(bladeUp, groundPosV2),0.0f);
}

void main()
{
    uint id = gl_GlobalInvocationID.x; //for grass blade

    if(id < amountBlades)
    {
        //debug[id] = vec4(0.0f,0.0f,1.0f,1.0f);

        float dirAlpha = p[id].w;
        float height = bv1[id].w;
        invHeight = 1.0f / height;
        float width = bv2[id].w;
        float bendingFac = attr[id].w;
        vec3 groundPos = (modelMatrix * vec4(p[id].xyz,1.0f)).xyz;

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

        float mapHeight = 0.0f;
        if(useHeightMap)
        {
            vec2 heightMapRead = textureLod(heightMap, clamp((groundPos.xz - heightMapBounds.xy) / heightMapBounds.zw, 0.0f, 1.0f), 0.0f).xy;
            mapHeight = heightMapRead.x * heightMapRead.y;
            groundPos += bladeUp * mapHeight;
        }

        vec3 idleV2 = groundPos + bladeUp * height;

        float mdt = min(dt, 1.0f);

        //read pressure map
        ivec2 pressureMapLookup = pressureMapOffset * ivec2(pressureMapBlockSize) + ivec2(id % pressureMapBlockSize, id / pressureMapBlockSize);
        vec4 oldPressure = imageLoad(pressureMap, pressureMapLookup);
        float collisionForce = max(oldPressure.w - (1.0f - bendingFac) * 0.5f * mdt, 0.0f); //!!CARE!! 0.5f is some random constant 

        //apply old pressure
        vec3 v2 = idleV2 + oldPressure.xyz;
        groundPosV2 = v2 - groundPos;

        //gravity
        const float h = height;// * 0.4f;
        vec3 grav = normalize(gravityVec.xyz) * gravityVec.w * (1.0f - useGravityPoint) + normalize(gravityPoint.xyz - v2) * gravityPoint.w * useGravityPoint; //towards mass center
        //float sign = step(-0.01f, dot(groundPosV2, bladeFront)) * 2.0f - 1.0f;
        float sign = step(-0.01f, dot(normalize(grav), bladeFront)) * 2.0f - 1.0f;
        grav += sign * bladeFront * h * (gravityVec.w * (1.0f - useGravityPoint) + gravityPoint.w * useGravityPoint) * 0.25f; //also a bit forward !!CARE!! 0.25f is some random constant
        grav = grav * h * bendingFac * mdt; //apply bending fac and dt

        //wind
        vec3 w = vec3(0.0f,0.0f,0.0f);
        float windageHeight = abs(dot(groundPosV2, bladeUp)) * invHeight;
        float wDebug = 0.0f;
        //TODO perhaps use subroutines
        switch(windType)
        {
        case 0:
            {
                float windageDir = 1.0f - abs(dot(normalize(windData.xyz), normalize(groundPosV2)));
                float windPos = 1.0f - max((cos((groundPos.x + groundPos.z) * 0.75f + windData.w) + sin((groundPos.x + groundPos.y) * 0.5f + windData.w) + sin((groundPos.y + groundPos.z) * 0.25f + windData.w)) / 3.0f, 0.0f);
                w = windData.xyz * windageDir * windageHeight * windPos * windPos * bendingFac * mdt; //!!CARE!! windPos^2 is just random
                wDebug = windPos * windPos;
            }
            break;
        case 1:
            {
                vec3 windDir = groundPos - windData.xyz;
                float windDist = length(windDir);
                windDir /= windDist;
                float windageDir = 1.0f;// - abs(dot(windDir, normalize(groundPosV2)));
                windDir *= 100.0f; //!!CARE!! 6.0f is some random const
                float windAtten = max(1.0f - log2(windDist * 0.2f + 1.0f) * 0.25f, 0.0f);
                float windPos = 1.0f - max(sin(windDist * 0.4f - windData.w * 4.0f), 0.0f); //!!CARE 2.0f is some random const
                w = windDir * windageDir * windAtten * windageHeight * windPos * bendingFac * mdt;
                wDebug = windPos * windAtten;
            }
            break;
        case 2:
            {
                vec3 windDir = groundPos - windData.xyz;
                float windDist = length(windDir);
                windDir /= windDist;
                vec3 windTangent = normalize(cross(windDir, bladeUp)) * 6.0f; //!!CARE!! 6.0f is some random const
                float windageDir = 1.0f - abs(dot(windDir, normalize(groundPosV2)));
                windDir *= 40.0f; //!!CARE!! 4.0f is some random const
                float windAtten = max(1.0f - log2(windDist * 0.5f + 1.0f) * 0.25f, 0.0f);
                float windPos = sin(windDist * 0.1f - windData.w * 1.5f); //!!CARE 3.0f is some random const
                windPos = windPos * windPos * windPos;
                //windDir = windDir * (step(0.0f,windPos) * 2.0f - 1.0f);
                windDir += windTangent * (1.0f - windAtten * windAtten) * 10.0f;
                w = windDir * windageDir * windAtten * windageHeight * /*abs(*/windPos/*)*/ * bendingFac * mdt;
                wDebug = windPos * windAtten;
            }
            break;
        }

        //stiffness
        vec3 stiffness = (idleV2 - v2) * (1.0f - bendingFac * 0.25f) * max(1.0f - collisionForce, 0.1f) * mdt; //!!!CARE!!! 0.1f is some random constant and 0.25f also

        //apply new forces
        v2 += grav + w + stiffness;
        groundPosV2 = v2 - groundPos;

        //Ensure valid V2 Pos -> not under ground plane
        EnsureValidV2Pos(v2, bladeUp);

        //Calculate v1
        vec3 v1 = CalculateV1(groundPos, v2, bladeUp, height);

        //Grass length correction
        MakePersistentLength(groundPos, v1, v2, height);

        //Collision detection
        //Collision with SphereColliders
        bool dataDirty = false;
        for(uint colli = 0; colli < amountSphereCollider; colli++)
        {
            float r = sphereCollider[colli].w;
            vec3 cPos = sphereCollider[colli].xyz;

            float d1 = distance(groundPos, cPos) - r;

            //Check for possible collsion
            if(d1 < height)
            {
                vec3 v2cPos = cPos - v2;
                float l = length(v2cPos);
                float d2 = l - r;
                
                //Case 1: v2 in sphere => move v2 to the nearest border (TODO: with respect to the ground position)
                if(d2 < 0)
                {
                    vec3 collVec = (v2cPos / l) * d2;
                    collisionForce += dot(collVec,collVec);
                    v2 += collVec;
                    dataDirty = true;
                } 
                
                //Case 2: Curve in sphere
                vec3 halfPoint = groundPos * 0.25f + 0.5f * v1 + 0.25f * v2;
                vec3 halfVec = normalize(halfPoint - groundPos);
                vec3 halfPointCPos = cPos - halfPoint;
                float lh = length(halfPointCPos);
                float dHalf = lh - r;
                
                if(dHalf < 0)
                {
                    vec3 collVec = (halfPointCPos / lh) * dHalf * 4.0f;
                    collisionForce += dot(collVec,collVec);
                    v2 += collVec;
                    dataDirty = true;
                }
            }
            else
            {
                continue;
            }
        }

        //Set v1 and correct grass length if collision happened
        if(dataDirty)
        {
            groundPosV2 = v2 - groundPos;

            //Ensure valid V2 Pos -> not under ground plane
            EnsureValidV2Pos(v2, bladeUp);            

            //Calculate v1
            v1 = CalculateV1(groundPos, v2, bladeUp, height);

            //Grass length correction
            MakePersistentLength(groundPos, v1, v2, height);
        }
    
        //Save v1 and v2 to buffers and update pressure map
        vec3 pressure = v2 - idleV2;
        bv1[id].xyz = (invModelMatrix * vec4(v1 - bladeUp * mapHeight,1.0f)).xyz;
        bv2[id].xyz = (invModelMatrix * vec4(v2 - bladeUp * mapHeight,1.0f)).xyz;
        imageStore(pressureMap, pressureMapLookup, vec4(pressure,collisionForce));

        //debug[id] = vec4(oldPressure);
        //float wtmp = (width - 0.02f) / 0.04f;
        //debug[id] = vec4(wtmp, wtmp, wtmp, 1.0f);
        //debug[id] = vec4(wDebug,0.0f,0.0f,1.0f);
        //debug[id] = vec4(float(dataDirty),0.0f,0.0f,1.0f);
        //debug[id] = vec4(float(amountSphereCollider) / float(MAX_AMOUNT_SPHERE_COLLIDER),amountSphereCollider,0.0f,1.0f);
        //debug[id] = vec4(1.0f,0.0f,1.0f,1.0f);
        //debug[id] = vec4(bladeUp * 0.5f + 0.5f,1.0f);
        //debug[id] = vec4(((grav / mdt) / max(abs(grav.x), max(abs(grav.y),abs(grav.z)))) * 0.5f + 0.5f, wDebug);
        //debug[id].yz = vec2(pressureMapOffset) / vec2(5.0f, 7.0f);
        //float ccode = (pressureMapOffset.y * 5 + pressureMapOffset.x) - 0.5;// / 31.0f;
        //debug[id].xyz = ccode.xxx;
        /*if(pressureMapOffset.x == 0 && pressureMapOffset.y == 0)
        {
            debug[id] = vec4(1.0f, 0.0f, 0.0f, 1.0f);
        }
        else if(pressureMapOffset.x == 1 && pressureMapOffset.y == 0)
        {
            debug[id] = vec4(1.0f, 1.0f, 0.0f, 1.0f);
        }
        else if(pressureMapOffset.x == 2 && pressureMapOffset.y == 0)
        {
            debug[id] = vec4(1.0f, 0.0f, 1.0f, 1.0f);
        }
        else if(pressureMapOffset.x == 3 && pressureMapOffset.y == 0)
        {
            debug[id] = vec4(1.0f, 1.0f, 1.0f, 1.0f);
        }
        else if(pressureMapOffset.x == 0 && pressureMapOffset.y == 1)
        {
            debug[id] = vec4(0.0f, 0.0f, 1.0f, 1.0f);
        }
        else if(pressureMapOffset.x == 1 && pressureMapOffset.y == 1)
        {
            debug[id] = vec4(0.0f, 1.0f, 1.0f, 1.0f);
        }
        else if(pressureMapOffset.x == 2 && pressureMapOffset.y == 1)
        {
            debug[id] = vec4(0.5f, 0.5f, 1.0f, 1.0f);
        }
        else if(pressureMapOffset.x == 3 && pressureMapOffset.y == 1)
        {
            debug[id] = vec4(0.0f, 1.0f, 0.0f, 1.0f);
        }
        else if(pressureMapOffset.x == 0 && pressureMapOffset.y == 2)
        {
            debug[id] = vec4(0.5f, 1.0f, 0.5f, 1.0f);
        }
        else if(pressureMapOffset.x == 1 && pressureMapOffset.y == 2)
        {
            debug[id] = vec4(0.75f, 1.0f, 0.25f, 1.0f);
        }
        else if(pressureMapOffset.x == 2 && pressureMapOffset.y == 2)
        {
            debug[id] = vec4(0.25f, 1.0f, 0.75f, 1.0f);
        }*/
    }
}