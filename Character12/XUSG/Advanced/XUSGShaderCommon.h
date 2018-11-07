//--------------------------------------------------------------------------------------
// By Stars XU Tianchen
//--------------------------------------------------------------------------------------

#pragma once

#ifndef	BASIC_SHADER_IDS
#define	BASIC_SHADER_IDS

// Vertex shaders
enum VertexShader : uint8_t
{
	VS_SCREEN_QUAD,
	VS_BASE_PASS,
	VS_DEPTH,
	VS_SKINNING,
	VS_REFLECT,
	VS_BOUND
};

// Pixel shaders
enum PixelShader : uint8_t
{
	PS_DEFERRED_SHADE,
	PS_DEPTH,
	PS_OCCLUSION,

	PS_BASE_PASS,
	PS_DEPTH_MODEL,
	PS_OCCLUSION_MODEL,
	PS_REFLECT,
	PS_BOUND,

	PS_SS_REFLECT,
	PS_SKY,
	PS_WATER,
	PS_RESAMPLE,

	PS_POST_PROC,
	PS_TONE_MAP,
	PS_TEMPORAL_AA,
	PS_UNSHARP,
	PS_FXAA
};

// Compute shaders
enum ComputeShader : uint8_t
{
	CS_SKINNING,
	CS_LUM_ADAPT
};

#endif
