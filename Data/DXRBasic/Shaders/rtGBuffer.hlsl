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

// Ray Payload (not used for GBuffer)
struct DummyRayPayload
{
    bool dummy;
};
typedef BuiltInTriangleIntersectionAttributes TriAttributes;

// GBuffer textures
RWTexture2D<float4> gWorldPos;
RWTexture2D<float4> gWorldNorm;
RWTexture2D<float4> gDiffuseCol;
RWTexture2D<float4> gSpecCol;

// Environment map texture and sampler
Texture2D<float4>   gEnvMap;
SamplerState        gEnvSampler;

// Const buffer for the RayGen shader
cbuffer RayGenCB
{
    float2 gPixelJitter;
}

[shader("raygeneration")]
void GBufferRayGen()
{
    // Calculate primary ray direction
    float2 pixelPos = (DispatchRaysIndex().xy + gPixelJitter) / DispatchRaysDimensions().xy;
    float2 pixelPosNormalized = float2(2, -2) * pixelPos + float2(-1, 1);
    float3 rayDir = pixelPosNormalized.x * gCamera.cameraU +
                    pixelPosNormalized.y * gCamera.cameraV +
                    gCamera.cameraW;

    // Initialize ray description
    RayDesc ray;
    ray.Origin = gCamera.posW;
    ray.Direction = normalize(rayDir);
    ray.TMin = 0.f;
    ray.TMax = 1e+38f;

    // Initialize ray payload
    DummyRayPayload payload = { false };

    // Trace ray
    TraceRay(gRtScene, RAY_FLAG_CULL_BACK_FACING_TRIANGLES, ~0, 0, hitProgramCount, 0, ray, payload);
    //TraceRay(gRtScene, 0, ~0, 0, hitProgramCount, 0, ray, payload);
}

[shader("miss")]
void PrimaryMiss(inout DummyRayPayload payload)
{
    // Sample the environment map for Diffuse Color
    float2 uv = wsVectorToLatLong(WorldRayDirection());
    gDiffuseCol[DispatchRaysIndex().xy] = gEnvMap.SampleLevel(gEnvSampler, uv, 0.f);
}

[shader("closesthit")]
void PrimaryClosestHit(inout DummyRayPayload payload, TriAttributes attribs)
{
    // Helper functions to calculate vertex and shading data
    VertexOut vertexData = getVertexAttributes(PrimitiveIndex(), attribs);
    ShadingData shadingData = prepareShadingData(vertexData, gMaterial, gCamera.posW, 0);

    // Save resulting values to GBuffer textures
    uint2 launchIndex = DispatchRaysIndex().xy;
    gWorldPos[launchIndex] = float4(shadingData.posW, 1.f);
    gWorldNorm[launchIndex] = float4(shadingData.N, length(shadingData.posW - gCamera.posW));
    gDiffuseCol[launchIndex] = float4(shadingData.diffuse, shadingData.opacity);
    gSpecCol[launchIndex] = float4(shadingData.specular, shadingData.linearRoughness);
}

[shader("anyhit")]
void PrimaryAnyHit(inout DummyRayPayload payload, TriAttributes attribs)
{
    if (alphaTestFails(attribs)) { IgnoreHit(); }
}