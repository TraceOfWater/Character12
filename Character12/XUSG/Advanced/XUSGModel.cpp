//--------------------------------------------------------------------------------------
// By Stars XU Tianchen
//--------------------------------------------------------------------------------------

#include "XUSGModel.h"

using namespace std;
using namespace DirectX;
using namespace XUSG;
using namespace XUSG::Graphics;

Model::Model(const Device &device, const GraphicsCommandList &commandList) :
	m_device(device),
	m_commandList(commandList),
	m_temporalIndex(0)
{
}

Model::~Model()
{
}

void Model::Init(const InputLayout &inputLayout, const shared_ptr<SDKMesh> &mesh,
	const shared_ptr<Shader::Pool> &shaderPool, const shared_ptr<Pipeline::Pool> &pipelinePool,
	const shared_ptr <DescriptorTablePool> &descriptorTablePool)
{
	// Set shader group and states
	m_shaderPool = shaderPool;
	m_pipelinePool = pipelinePool;
	m_descriptorTablePool = descriptorTablePool;

	// Get SDKMesh
	m_mesh = mesh;

	// Create variable slots
	createConstantBuffers();
	createPipelineLayout();
	createPipelines(inputLayout);
	createDescriptorTables();
}

void Model::FrameMove()
{
#if	TEMPORAL
	m_temporalIndex = !m_temporalIndex;
#endif
}

void Model::SetMatrices(CXMMATRIX world, CXMMATRIX viewProj, FXMMATRIX *pShadow, bool isTemporal)
{
	// Set World-View-Proj matrix
	const auto worldViewProj = XMMatrixMultiply(world, viewProj);

	// Update constant buffers
	const auto pCBData = reinterpret_cast<CBMatrices*>(m_cbMatrices.Map());
	pCBData->WorldViewProj = XMMatrixTranspose(worldViewProj);
	pCBData->World = XMMatrixTranspose(world);
	pCBData->Normal = XMMatrixInverse(nullptr, world);
	//pCBData->Normal = XMMatrixTranspose(&, &pCBData->Normal);	// transpose once
	//pCBData->Normal = XMMatrixTranspose(&, &pCBData->Normal);	// transpose twice
	
	if (pShadow)
	{
		const auto shadow = XMMatrixMultiply(world, *pShadow);
		pCBData->ShadowProj = XMMatrixTranspose(shadow);

		auto &cbData = *reinterpret_cast<XMMATRIX*>(m_cbShadowMatrix.Map());
		cbData = pCBData->ShadowProj;
	}

#if	TEMPORAL
	if (isTemporal)
	{
		XMStoreFloat4x4(&m_worldViewProjs[m_temporalIndex], worldViewProj);
		const auto worldViewProjPrev = XMLoadFloat4x4(&m_worldViewProjs[!m_temporalIndex]);
		pCBDataMatrices->WorldViewProjPrev = XMMatrixTranspose(worldViewProjPrev);
	}
#endif
}

void Model::SetPipelineState(SubsetFlag subsetFlags)
{
	assert(!(subsetFlags & SUBSET_FULL));

	if (subsetFlags & SUBSET_ALPHA_TEST)
		m_commandList->SetPipelineState(m_pipelines[OPAQUE_TWO_SIDE].Get());
	else if (subsetFlags & SUBSET_ALPHA)
		m_commandList->SetPipelineState(m_pipelines[ALPHA_TWO_SIDE].Get());
	else if (subsetFlags & SUBSET_REFLECTED)
		m_commandList->SetPipelineState(m_pipelines[REFLECTED].Get());
	else m_commandList->SetPipelineState(m_pipelines[OPAQUE_FRONT].Get());
}

void Model::SetPipelineState(PipelineIndex pipeline)
{
	m_commandList->SetPipelineState(m_pipelines[pipeline].Get());
}

void Model::Render(SubsetFlag subsetFlags, bool isShadow, bool reset)
{
	DescriptorPool::InterfaceType* heaps[] =
	{
		m_descriptorTablePool->GetCbvSrvUavPool().Get(),
		m_descriptorTablePool->GetSamplerPool().Get()
	};
	m_commandList->SetDescriptorHeaps(_countof(heaps), heaps);
	m_commandList->SetGraphicsRootSignature(m_pipelineLayout.Get());
	m_commandList->SetGraphicsRootDescriptorTable(MATRICES,
		*m_cbvTables[isShadow ? CBV_SHADOW_MATRIX : CBV_MATRICES]);
	m_commandList->SetGraphicsRootDescriptorTable(SAMPLERS, *m_samplerTable);

	const auto numMeshes = m_mesh->GetNumMeshes();
	for (auto m = 0u; m < numMeshes; ++m)
	{
		// Set IA parameters
		const auto vertexBuffer = m_mesh->GetVertexBuffer(m, 0);
		m_commandList->IASetVertexBuffers(0, 1, &vertexBuffer->GetVBV());

		// Render mesh
		render(m, subsetFlags, reset);
	}

	// Clear out the vb bindings for the next pass
	if (reset) m_commandList->IASetVertexBuffers(0, 1, nullptr);
}

void Model::LoadSDKMesh(const Device &device, const wstring &meshFileName, shared_ptr<SDKMesh> &mesh)
{
	// Load the mesh
	mesh = make_shared<SDKMesh>();

	ThrowIfFailed(mesh->Create(device, meshFileName.c_str()));

	//mesh->ClassifyMatType();
}

void Model::InitLayout(Pipeline::Pool &pipelinePool, InputLayout &inputLayout)
{
	// Define vertex data layout for post-transformed objects
	const auto offset = 0xffffffff;
	InputElementTable inputElementDescs =
	{
		{ "POSITION",	0, DXGI_FORMAT_R32G32B32_FLOAT,		0, 0,		D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "NORMAL",		0, DXGI_FORMAT_R16G16B16A16_FLOAT,	0, offset,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TEXCOORD",	0, DXGI_FORMAT_R16G16_FLOAT,		0, offset,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "TANGENT",	0, DXGI_FORMAT_R16G16B16A16_FLOAT,	0, offset,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 },
		{ "BINORMAL",	0, DXGI_FORMAT_R16G16B16A16_FLOAT,	0, offset,	D3D12_INPUT_CLASSIFICATION_PER_VERTEX_DATA, 0 }
	};

	inputLayout = pipelinePool.CreateInputLayout(inputElementDescs);
}

void Model::SetShadowMap(const GraphicsCommandList &commandList, const DescriptorTable &shadowTable)
{
	commandList->SetGraphicsRootDescriptorTable(SHADOW_MAP, *shadowTable);
}

void Model::createConstantBuffers()
{
	m_cbMatrices.Create(m_device, 512 * 128, sizeof(CBMatrices));
	m_cbShadowMatrix.Create(m_device, 256 * 128, sizeof(XMFLOAT4));
}

void Model::createPipelineLayout()
{
	auto cbMatrices = 0u;
	auto txDiffuse = 0u;
	auto txNormal = txDiffuse + 1;
	auto txShadow = txNormal + 1;
	auto smpAnisoWrap = 0u;
	auto smpLinearCmp = smpAnisoWrap + 2;

	// Get constant buffer slots
	auto desc = D3D12_SHADER_INPUT_BIND_DESC();
	auto reflector = m_shaderPool->GetReflector(Shader::Stage::VS, VS_BASE_PASS);
	if (reflector)
	{
		const auto hr = reflector->GetResourceBindingDescByName("cbMatrices", &desc);
		if (SUCCEEDED(hr)) cbMatrices = desc.BindPoint;
	}

	reflector = m_shaderPool->GetReflector(Shader::Stage::PS, PS_BASE_PASS);
	if (reflector)
	{
		// Get shader resource slots
		auto hr = reflector->GetResourceBindingDescByName("g_txAlbedo", &desc);
		if (SUCCEEDED(hr)) txDiffuse = desc.BindPoint;
		hr = reflector->GetResourceBindingDescByName("g_txNormal", &desc);
		if (SUCCEEDED(hr)) txNormal = desc.BindPoint;
		hr = reflector->GetResourceBindingDescByName("g_txShadow", &desc);
		if (SUCCEEDED(hr)) txShadow = desc.BindPoint;

		// Get sampler slots
		hr = reflector->GetResourceBindingDescByName("g_smpLinear", &desc);
		if (SUCCEEDED(hr)) smpAnisoWrap = desc.BindPoint;
		hr = reflector->GetResourceBindingDescByName("g_smpCmpLinear", &desc);
		if (SUCCEEDED(hr)) smpLinearCmp = desc.BindPoint;
	}

	// Get pipeline layout
	Util::PipelineLayout utilPipelineLayout;
	// Constant buffers
	utilPipelineLayout.SetRange(MATRICES, DescriptorType::CBV, 1, cbMatrices);
	utilPipelineLayout.SetShaderStage(MATRICES, Shader::Stage::VS);

	// Textures (material and shadow)
	if (txNormal == txDiffuse + 1)
		utilPipelineLayout.SetRange(MATERIAL, DescriptorType::SRV, 2, txDiffuse);
	else
	{
		utilPipelineLayout.SetRange(MATERIAL, DescriptorType::SRV, 1, txDiffuse);
		utilPipelineLayout.SetRange(MATERIAL, DescriptorType::SRV, 1, txNormal);
	}
	utilPipelineLayout.SetShaderStage(MATERIAL, Shader::Stage::PS);

	utilPipelineLayout.SetRange(SHADOW_MAP, DescriptorType::SRV, 1, txShadow);
	utilPipelineLayout.SetShaderStage(SHADOW_MAP, Shader::Stage::PS);

	// Samplers
	if (smpLinearCmp == smpAnisoWrap + 2)
		utilPipelineLayout.SetRange(SAMPLERS, DescriptorType::SAMPLER, 3, smpAnisoWrap);
	else
	{
		utilPipelineLayout.SetRange(SAMPLERS, DescriptorType::SAMPLER, 2, smpAnisoWrap);
		utilPipelineLayout.SetRange(SAMPLERS, DescriptorType::SAMPLER, 1, smpLinearCmp);
	}
	utilPipelineLayout.SetShaderStage(SAMPLERS, Shader::Stage::PS);

	m_pipelineLayout = utilPipelineLayout.GetPipelineLayout(*m_pipelinePool,
		D3D12_ROOT_SIGNATURE_FLAG_ALLOW_INPUT_ASSEMBLER_INPUT_LAYOUT);
}

void Model::createPipelines(const InputLayout &inputLayout, const Format *rtvFormats,
	uint32_t numRTVs, Format dsvFormat)
{
	const Format defaultRtvFormats[] =
	{
		DXGI_FORMAT_B8G8R8A8_UNORM,
		DXGI_FORMAT_R10G10B10A2_UNORM,
		DXGI_FORMAT_B8G8R8A8_UNORM,
		DXGI_FORMAT_R16G16_FLOAT
	};

	rtvFormats = rtvFormats ? rtvFormats : defaultRtvFormats;
	numRTVs = numRTVs > 0 ? numRTVs : _countof(defaultRtvFormats);

	// Get opaque pipeline
	Graphics::State state;
	state.IASetInputLayout(inputLayout);
	state.SetPipelineLayout(m_pipelineLayout);
	state.SetShader(Shader::Stage::VS, m_shaderPool->GetShader(Shader::Stage::VS, VS_BASE_PASS));
	state.SetShader(Shader::Stage::PS, m_shaderPool->GetShader(Shader::Stage::PS, PS_BASE_PASS));
	//state.DSSetState(Graphics::DepthStencilPreset::DEPTH_STENCIL_NONE, *m_pipelinePool);
	state.IASetPrimitiveTopologyType(D3D12_PRIMITIVE_TOPOLOGY_TYPE_TRIANGLE);
	state.OMSetRTVFormats(rtvFormats, numRTVs);
	state.OMSetNumRenderTargets(numRTVs);
	state.OMSetDSVFormat(dsvFormat ? dsvFormat : DXGI_FORMAT_D24_UNORM_S8_UINT);
	m_pipelines[OPAQUE_FRONT] = state.GetPipeline(*m_pipelinePool);

	// Get alpha-test pipeline
	state.RSSetState(Graphics::RasterizerPreset::CULL_NONE, *m_pipelinePool);
	m_pipelines[OPAQUE_TWO_SIDE] = state.GetPipeline(*m_pipelinePool);

	// Get transparent pipeline
	state.RSSetState(Graphics::RasterizerPreset::CULL_NONE, *m_pipelinePool);
	state.OMSetBlendState(BlendPreset::AUTO_NON_PREMUL, *m_pipelinePool);
	m_pipelines[ALPHA_TWO_SIDE] = state.GetPipeline(*m_pipelinePool);

	// Get reflected pipeline
	state.RSSetState(Graphics::RasterizerPreset::CULL_FRONT, *m_pipelinePool);
	state.OMSetBlendState(BlendPreset::DEFAULT_OPAQUE, *m_pipelinePool);
	m_pipelines[REFLECTED] = state.GetPipeline(*m_pipelinePool);
}

void Model::createDescriptorTables()
{
	Util::DescriptorTable cbMatricesTable;
	cbMatricesTable.SetDescriptors(0, 1, &m_cbMatrices.GetCBV());
	m_cbvTables[CBV_MATRICES] = cbMatricesTable.GetCbvSrvUavTable(*m_descriptorTablePool);

	Util::DescriptorTable cbShadowTable;
	cbShadowTable.SetDescriptors(0, 1, &m_cbMatrices.GetCBV());
	m_cbvTables[CBV_SHADOW_MATRIX] = cbShadowTable.GetCbvSrvUavTable(*m_descriptorTablePool);

	Util::DescriptorTable samplerTable;
	const SamplerPreset::Type samplers[] =
	{ SamplerPreset::ANISOTROPIC_WRAP, SamplerPreset::POINT_WRAP, SamplerPreset::LINEAR_LESS_EQUAL };
	samplerTable.SetSamplers(0, _countof(samplers), samplers, *m_descriptorTablePool);
	m_samplerTable = samplerTable.GetSamplerTable(*m_descriptorTablePool);
	
	// Materials
	const auto numMaterials = m_mesh->GetNumMaterials();
	m_srvTables.resize(numMaterials);
	for (auto m = 0u; m < numMaterials; ++m)
	{
		const auto pMaterial = m_mesh->GetMaterial(m);

		if (pMaterial)
		{
			Util::DescriptorTable srvTable;
			const Descriptor srvs[] = { pMaterial->pAlbedo->GetSRV(), pMaterial->pNormal->GetSRV() };
			srvTable.SetDescriptors(0, _countof(srvs), srvs);
			m_srvTables[m] = samplerTable.GetCbvSrvUavTable(*m_descriptorTablePool);
		}
		else m_srvTables[m] = nullptr;
	}
}

void Model::render(uint32_t mesh, SubsetFlag subsetFlags, bool reset)
{
	// Set IA parameters
	m_commandList->IASetIndexBuffer(&m_mesh->GetIndexBuffer(mesh)->GetIBV());

	// Opaque subsets
	if (subsetFlags & SUBSET_OPAQUE)	
	{
		// Set pipeline state
		if (reset) SetPipelineState(SubsetFlag(~SUBSET_ALPHA & subsetFlags));

		const auto numSubsets = m_mesh->GetNumSubsets(mesh, SUBSET_OPAQUE);
		for (auto subset = 0u; subset < numSubsets; ++subset)
		{
			// Get subset
			const auto pSubset = m_mesh->GetSubset(mesh, subset, SUBSET_OPAQUE);
			const auto primType = m_mesh->GetPrimitiveType(SDKMESH_PRIMITIVE_TYPE(pSubset->PrimitiveType));
			m_commandList->IASetPrimitiveTopology(primType);

			// Set material
			if (m_mesh->GetMaterial(pSubset->MaterialID) && m_srvTables[pSubset->MaterialID])
				m_commandList->SetGraphicsRootDescriptorTable(MATERIAL, *m_srvTables[pSubset->MaterialID]);

			// Draw
			m_commandList->DrawIndexedInstanced(static_cast<uint32_t>(pSubset->IndexCount), 1,
				static_cast<uint32_t>(pSubset->IndexStart), static_cast<int32_t>(pSubset->VertexStart), 0);
		}
	}

	// Transparent subsets
	if (subsetFlags & SUBSET_ALPHA)
	{
		// Set pipeline state
		if (reset) SetPipelineState(SubsetFlag(~SUBSET_OPAQUE & subsetFlags));

		const auto numSubsets = m_mesh->GetNumSubsets(mesh, SUBSET_ALPHA);
		for (auto subset = 0u; subset < numSubsets; ++subset)
		{
			// Get subset
			const auto pSubset = m_mesh->GetSubset(mesh, subset, SUBSET_ALPHA);
			const auto primType = m_mesh->GetPrimitiveType(SDKMESH_PRIMITIVE_TYPE(pSubset->PrimitiveType));
			m_commandList->IASetPrimitiveTopology(primType);

			// Set material
			if (m_mesh->GetMaterial(pSubset->MaterialID) && m_srvTables[pSubset->MaterialID])
				m_commandList->SetGraphicsRootDescriptorTable(MATERIAL, *m_srvTables[pSubset->MaterialID]);

			// Draw
			m_commandList->DrawIndexedInstanced(static_cast<uint32_t>(pSubset->IndexCount), 1,
				static_cast<uint32_t>(pSubset->IndexStart), static_cast<int32_t>(pSubset->VertexStart), 0);
		}
	}
}
