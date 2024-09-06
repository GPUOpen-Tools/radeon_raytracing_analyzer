//=============================================================================
// Copyright (c) 2021-2024 Advanced Micro Devices, Inc. All rights reserved.
/// @author AMD Developer Tools Team
/// @file
/// @brief
//=============================================================================

cbuffer PerSceneConstantBuffer : register(b0)
{
    float4x4 transform;
    float4x4 world;
    float4   lightPosition;
    float4   camera;  //xyz camera, Z specular
    float    minBvhLevel;
    float    maxBvhLevel;
    int      coloringMode;
    int      selectedInstance;
    int      maxInstanceCount;
    int      maxTriangleCount;
};

cbuffer PerInstanceConstantBuffer : register(b1)
{
    int   instanceCount;
    int   triangleCount;
    int   blasIndex;
    int   maxDepth;
    float averageDepth;
};

cbuffer PerMeshConstantBuffer : register(b2)
{
    float4x4 worldTransform;
    int      instanceIndex;
};

#define COLORING_MODE_TREE_LEVEL (0)
#define COLORING_MODE_BLAS_INSTANCE (1)
#define COLORING_MODE_GEOMETRY_INDEX (2)
#define COLORING_MODE_OPAQUE (3)
#define COLORING_MODE_LIT (4)
#define COLORING_MODE_TECHNICAL (5)
#define COLORING_MODE_AVERAGE_SAH (6)
#define COLORING_MODE_MAX_SAH (7)
#define COLORING_MODE_INSTANCE_COUNT (8)
#define COLORING_MODE_TRIANGLE_COUNT (9)
#define COLORING_MODE_BLAS_MAX_DEPTH (10)
#define COLORING_MODE_BLAS_AVERAGE_DEPTH (11)
#define COLORING_MODE_INSTANCE_INDEX (12)

struct PSInput
{
    float4 position : SV_POSITION;
    float3 normal : NORMAL;
    float4 color : COLOR;
    float4 lightDir : LIGHT_DIR;
};

struct VSInput
{
    float4 position : POSITION;
    float4 normal : NORMAL;
    float4 color : COLOR;
};

PSInput VSMain(VSInput input)
{
    PSInput result;

    float4x4 finalTransform = mul(transform, worldTransform);
    result.position         = mul(finalTransform, input.position);
    result.normal           = mul(finalTransform, input.normal).xyz;
    result.lightDir         = normalize(lightPosition - result.position);
    result.color            = float4(0.0f, 0.0f, 0.0f, 1.0f);
    result.normal           = input.normal.xyz;

    return result;
}

PSInput VSMain_Selected(VSInput input)
{
    PSInput result;

    float4x4 finalTransform = mul(transform, worldTransform);
    result.position         = mul(finalTransform, input.position);
    result.normal           = mul(finalTransform, input.normal).xyz;
    result.lightDir         = normalize(lightPosition - result.position);
    result.color            = float4(1.0f, 1.0f, 1.0f, 1.0f);
    result.normal           = input.normal.xyz;

    return result;
}

float4 PSMain(PSInput input)
    : SV_TARGET
{
    return input.color;
}
