// Falcor imports
#include "HostDeviceSharedMacros.h"
import Raytracing;
import ShaderCommon;
import Shading;

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

// Environment map texture and sampler
Texture2D<float4>   gEnvMap;
SamplerState        gEnvSampler;

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

}