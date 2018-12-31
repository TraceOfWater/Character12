//--------------------------------------------------------------------------------------
// By Stars XU Tianchen
//--------------------------------------------------------------------------------------

#pragma once

#include "XUSGModel.h"
#include "Core/XUSGComputeState.h"

namespace XUSG
{
	class Character :
		public Model
	{
	public:
		struct Vertex
		{
			DirectX::XMFLOAT3	Pos;
			DirectX::XMUINT3	NormTex;
			DirectX::XMUINT4	TanBiNrm;
		};

		struct MeshLink
		{
			std::wstring		szMeshName;
			std::string			szBoneName;
			uint32_t			uBone;
		};

		Character(const Device &device, const CommandList &commandList);
		virtual ~Character();

		bool Init(const InputLayout &inputLayout,
			const std::shared_ptr<SDKMesh> &mesh,
			const std::shared_ptr<ShaderPool> &shaderPool,
			const std::shared_ptr<Graphics::PipelineCache> &graphicsPipelineCache,
			const std::shared_ptr<Compute::PipelineCache> &computePipelineCache,
			const std::shared_ptr<PipelineLayoutCache> &pipelineLayoutCache,
			const std::shared_ptr<DescriptorTableCache> &descriptorTableCache,
			const std::shared_ptr<std::vector<SDKMesh>> &linkedMeshes = nullptr,
			const std::shared_ptr<std::vector<MeshLink>> &meshLinks = nullptr);
		void InitPosition(const DirectX::XMFLOAT4 &posRot);
		void FrameMove(double time);
		void FrameMove(double time, DirectX::CXMMATRIX viewProj, DirectX::FXMMATRIX *pWorld = nullptr,
			DirectX::FXMMATRIX *pShadowView = nullptr, DirectX::FXMMATRIX *pShadows = nullptr,
			uint8_t numShadows = 0, bool isTemporal = true);
		virtual void SetMatrices(DirectX::CXMMATRIX viewProj, DirectX::FXMMATRIX *pWorld = nullptr,
			DirectX::FXMMATRIX *pShadowView = nullptr, DirectX::FXMMATRIX *pShadows = nullptr,
			uint8_t numShadows = 0, bool isTemporal = true);
		void SetSkinningPipeline();
		void Skinning(bool reset = false);
		void RenderTransformed(SubsetFlags subsetFlags = SUBSET_FULL, uint8_t matrixTableIndex = CBV_MATRICES,
			PipelineLayoutIndex layout = NUM_PIPE_LAYOUT);

		const DirectX::XMFLOAT4 &GetPosition() const;
		DirectX::FXMMATRIX GetWorldMatrix() const;

		static std::shared_ptr<SDKMesh> LoadSDKMesh(const Device &device, const std::wstring &meshFileName,
			const std::wstring &animFileName, const TextureCache &textureCache,
			const std::shared_ptr<std::vector<MeshLink>> &meshLinks = nullptr,
			std::vector<SDKMesh> *pLinkedMeshes = nullptr);

	protected:
		enum DescriptorTableSlot : uint8_t
		{
			INPUT,
			OUTPUT,
			HISTORY = PER_OBJECT
		};

		bool createTransformedStates();
		bool createTransformedVBs(VertexBuffer &vertexBuffer);
		bool createBuffers();
		void createPipelineLayouts();
		void createPipelines(const InputLayout &inputLayout, const Format *rtvFormats = nullptr,
			uint32_t numRTVs = 0, Format dsvFormat = Format(0));
		void createDescriptorTables();
		virtual void setLinkedMatrices(uint32_t mesh, DirectX::CXMMATRIX viewProj,
			DirectX::CXMMATRIX world, DirectX::FXMMATRIX *pShadowView,
			DirectX::FXMMATRIX *pShadows, uint8_t numShadows, bool isTemporal);
		void skinning(bool reset);
		void renderTransformed(SubsetFlags subsetFlags, uint8_t matrixTableIndex, PipelineLayoutIndex layout);
		void renderLinked(uint32_t mesh, uint8_t matrixTableIndex, PipelineLayoutIndex layout);
		void setSkeletalMatrices(uint32_t numMeshes);
		void setBoneMatrices(uint32_t mesh);
		void convertToDQ(DirectX::XMFLOAT4 &dqTran, DirectX::CXMVECTOR quat,
			const DirectX::XMFLOAT3 &tran) const;
		DirectX::FXMMATRIX getDualQuat(uint32_t mesh, uint32_t influence) const;

		std::shared_ptr<std::vector<SDKMesh>>	m_linkedMeshes;
		std::shared_ptr<std::vector<MeshLink>>	m_meshLinks;

		std::shared_ptr<Compute::PipelineCache> m_computePipelineCache;

		VertexBuffer m_transformedVBs[FrameCount];
		DirectX::XMFLOAT4X4	m_mWorld;
		DirectX::XMFLOAT4	m_vPosRot;

#if	TEMPORAL
		std::vector<DirectX::XMFLOAT4X4> m_linkedWorldViewProjs[FrameCount];
#endif

		double m_time;

		StructuredBuffer m_boneWorlds[FrameCount];
		std::vector<ConstantBuffer> m_cbLinkedMatrices;
		std::vector<ConstantBuffer> m_cbLinkedShadowMatrices;

		PipelineLayout	m_skinningPipelineLayout;
		Pipeline		m_skinningPipeline;
		std::vector<DescriptorTable> m_srvSkinningTables[FrameCount];
		std::vector<DescriptorTable> m_uavSkinningTables[FrameCount];
#if	TEMPORAL
		std::vector<DescriptorTable> m_srvSkinnedTables[FrameCount];
#endif
	};
}
