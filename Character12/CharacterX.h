//*********************************************************
//
// Copyright (c) Microsoft. All rights reserved.
// This code is licensed under the MIT License (MIT).
// THIS CODE IS PROVIDED *AS IS* WITHOUT WARRANTY OF
// ANY KIND, EITHER EXPRESS OR IMPLIED, INCLUDING ANY
// IMPLIED WARRANTIES OF FITNESS FOR A PARTICULAR
// PURPOSE, MERCHANTABILITY, OR NON-INFRINGEMENT.
//
//*********************************************************

#pragma once

#include "StepTimer.h"
#include "Core/XUSGGraphicsState.h"
#include "Core/XUSGResource.h"
#include "Advanced/XUSGCharacter.h"

using namespace DirectX;

// Note that while ComPtr is used to manage the lifetime of resources on the CPU,
// it has no understanding of the lifetime of resources on the GPU. Apps must account
// for the GPU lifetime of resources to avoid destroying objects that may still be
// referenced by the GPU.
// An example of this can be found in the class method: OnDestroy().
using Microsoft::WRL::ComPtr;

class CharacterX : public DXFramework
{
public:
	CharacterX(uint32_t width, uint32_t height, std::wstring name);

	virtual void OnInit();
	virtual void OnUpdate();
	virtual void OnRender();
	virtual void OnDestroy();

private:
	static const uint32_t FrameCount = 2;
	static const uint32_t TextureWidth = 1024;
	static const uint32_t TextureHeight = 1024;
	static const uint32_t TexturePixelSize = 4;	// The number of bytes used to represent a pixel in the texture.

	struct Vertex
	{
		XMFLOAT3 position;
		XMFLOAT2 uv;
	};

	std::shared_ptr<XUSG::Shader::Pool>				m_shaderPool;
	std::shared_ptr<XUSG::Graphics::Pipeline::Pool>	m_pipelinePool;
	std::shared_ptr<XUSG::DescriptorTablePool>		m_descriptorTablePool;

	// Pipeline objects.
	XUSG::InputLayout 				m_inputLayout;
	CD3DX12_VIEWPORT				m_viewport;
	CD3DX12_RECT					m_scissorRect;

	ComPtr<IDXGISwapChain3>			m_swapChain;
	ComPtr<ID3D12CommandAllocator>	m_commandAllocators[FrameCount];
	ComPtr<ID3D12CommandQueue>		m_commandQueue;

	XUSG::Device m_device;
	XUSG::Resource m_renderTargets[FrameCount];
	XUSG::GraphicsCommandList m_commandList;

	// App resources.
	std::unique_ptr<XUSG::Character> m_character;
	XUSG::DescriptorPool	m_rtvPool;
	XUSG::RenderTargetTable	m_rtvTables[FrameCount];
	XUSG::DepthStencil		m_depth;
	XMFLOAT4X4				m_proj;

	// Synchronization objects.
	uint32_t m_frameIndex;
	HANDLE m_fenceEvent;
	ComPtr<ID3D12Fence> m_fence;
	uint64_t m_fenceValues[FrameCount];

	// Application state
	StepTimer m_timer;

	void LoadPipeline();
	void LoadAssets();
	std::vector<uint8_t> GenerateTextureData(uint32_t subDivLevel);
	void PopulateCommandList();
	void WaitForGpu();
	void MoveToNextFrame();
	double CalculateFrameStats(float *fTimeStep = nullptr);
};
