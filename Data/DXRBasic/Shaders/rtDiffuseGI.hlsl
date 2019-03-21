/**********************************************************************************************************************
# Copyright (c) 2018, NVIDIA CORPORATION. All rights reserved.
#
# Redistribution and use in source and binary forms, with or without modification, are permitted provided that the
# following conditions are met:
#  * Redistributions of code must retain the copyright notice, this list of conditions and the following disclaimer.
#  * Neither the name of NVIDIA CORPORATION nor the names of its contributors may be used to endorse or promote products
#    derived from this software without specific prior written permission.
#
# THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS ``AS IS'' AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
# LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR A PARTICULAR PURPOSE ARE DISCLAIMED.  IN NO EVENT
# SHALL THE COPYRIGHT OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL, SPECIAL, EXEMPLARY, OR
# CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA,
# OR PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF LIABILITY, WHETHER IN CONTRACT, STRICT
# LIABILITY, OR TORT (INCLUDING NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS SOFTWARE, EVEN IF
# ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
**********************************************************************************************************************/

// Falcor imports
#include "HostDeviceSharedMacros.h"
import Raytracing;
import ShaderCommon;
import Shading;
import Lights;

// Include utility functions from NVIDIA
#include "rtShaderUtils.hlsli"

// Shadow ray payload
struct ShadowRayPayload
{
    bool visible;
};
bool ShadowRayGen(float3 origin, float3 dir, float minT, float maxT);

// GI Ray Payload
struct GIRayPayload
{
    float3 color;
    uint rndSeed;
};
float3 GIRayGen(float3 origin, float3 dir, float minT, float maxT);

typedef BuiltInTriangleIntersectionAttributes TriAttributes;

// GBuffer textures
Texture2D<float4> gWorldPos;
Texture2D<float4> gWorldNorm;
Texture2D<float4> gDiffuseCol;
Texture2D<float4> gSpecCol;

// Output texture
RWTexture2D<float4> gOutput;

// Environment map texture and sampler
Texture2D<float4> gEnvMap;
SamplerState gEnvSampler;

// Const buffer for the RayGen shader
cbuffer RayGenCB
{
    float   gMinT;
    uint    gFrameCount;
    bool    gShadows;
    bool    gGI;
}

[shader("raygeneration")]
void DiffuseGIRayGen()
{
    uint2 launchIndex = DispatchRaysIndex().xy;

    // Get GBuffer data
    float4 worldPos = gWorldPos[launchIndex];
    float4 worldNorm = gWorldNorm[launchIndex];
    float4 diffuseCol = gDiffuseCol[launchIndex];
    float4 specCol = gSpecCol[launchIndex];

    float3 pixelColor;

    if (worldPos.w == 0.f)
    {
        pixelColor = diffuseCol.rgb;
    }
    else
    {
        uint randSeed = initRand(launchIndex.x + DispatchRaysDimensions().x * launchIndex.y, gFrameCount, 16);
        int randomLight = min(int(nextRand(randSeed) * gLightsCount), gLightsCount - 1);

        float lightDist;
        float3 lightIntensity;
        float3 dirToLight;
        getLightData(randomLight, worldPos.xyz, dirToLight, lightIntensity, lightDist);

        // Lambertian (diffuse) color
        float NdotL = saturate(dot(worldNorm.xyz, dirToLight));
        float3 diffuseTerm = diffuseCol.rgb * NdotL * M_1_PI;

        // Specular highlight
        float3 viewDir = normalize(gCamera.posW - worldPos.xyz);
        float3 reflectedDir = reflect(-dirToLight, worldNorm.xyz);
        float RdotV = saturate(dot(reflectedDir, viewDir));
        float3 specTerm = specCol.rgb * pow(RdotV, 32.0f);

        pixelColor = lightIntensity * (diffuseTerm + specTerm);
        
        // Shadow ray
        if (gShadows)
        {
            pixelColor *= (int)ShadowRayGen(worldPos.xyz, dirToLight, gMinT, lightDist);
        }

        if (gGI)
        {
            float3 indirectDir = getCosHemisphereSample(randSeed, worldNorm.xyz);


        }
    }

    gOutput[launchIndex] = float4(pixelColor, 1.f);

    //// Calculate primary ray direction
    //float2 pixelPos = (DispatchRaysIndex().xy + gPixelJitter) / DispatchRaysDimensions().xy;
    //float2 pixelPosNormalized = float2(2, -2) * pixelPos + float2(-1, 1);
    //float3 rayDir = pixelPosNormalized.x * gCamera.cameraU +
    //                pixelPosNormalized.y * gCamera.cameraV +
    //                gCamera.cameraW;

    //// Initialize ray description
    //RayDesc ray;
    //ray.Origin = gCamera.posW;
    //ray.Direction = normalize(rayDir);
    //ray.TMin = 0.f;
    //ray.TMax = 1e+38f;

    //// Initialize ray payload
    //DummyRayPayload payload = { false };

    //// Trace ray
    //TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, hitProgramCount, 0, ray, payload);
}


bool ShadowRayGen(float3 origin, float3 dir, float minT, float maxT)
{
    RayDesc shadowRay;
    shadowRay.Origin = origin;
    shadowRay.Direction = dir;
    shadowRay.TMin = minT;
    shadowRay.TMax = maxT;

    ShadowRayPayload payload = { false };

    TraceRay(gRtScene, RAY_FLAG_ACCEPT_FIRST_HIT_AND_END_SEARCH, ~0, 0, hitProgramCount, 0, shadowRay, payload);

    return payload.visible;
}

[shader("miss")]
void ShadowMiss(inout ShadowRayPayload payload)
{
    payload.visible = true;
    // Sample the environment map for Diffuse Color
    // float2 uv = wsVectorToLatLong(WorldRayDirection());
    // gDiffuseCol[DispatchRaysIndex().xy] = gEnvMap.SampleLevel(gEnvSampler, uv, 0.f);
}

[shader("anyhit")]
void ShadowHit(inout ShadowRayPayload payload, TriAttributes attribs)
{
    if (alphaTestFails(attribs)) { IgnoreHit(); }
    //// Helper functions to calculate vertex and shading data
    //VertexOut vertexData = getVertexAttributes(PrimitiveIndex(), attribs);
    //ShadingData shadingData = prepareShadingData(vertexData, gMaterial, gCamera.posW, 0);

    //// Save resulting values to GBuffer textures
    //uint2 launchIndex = DispatchRaysIndex().xy;
    //gWorldPos[launchIndex] = float4(shadingData.posW, 1.f);
    //gWorldNorm[launchIndex] = float4(shadingData.N, length(shadingData.posW - gCamera.posW));
    //gDiffuseCol[launchIndex] = float4(shadingData.diffuse, shadingData.opacity);
    //gSpecCol[launchIndex] = float4(shadingData.specular, shadingData.linearRoughness);
}
