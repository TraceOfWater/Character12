//--------------------------------------------------------------------------------------
// By Stars XU Tianchen
//--------------------------------------------------------------------------------------

#pragma once

//#undef D3DCOLOR_ARGB
#include "Core/XUSGResource.h"

//--------------------------------------------------------------------------------------
// Hard Defines for the various structures
//--------------------------------------------------------------------------------------
#define SDKMESH_FILE_VERSION	101
#define MAX_VERTEX_ELEMENTS		32
#define MAX_VERTEX_STREAMS		16
#define MAX_FRAME_NAME			100
#define MAX_MESH_NAME			100
#define MAX_SUBSET_NAME			100
#define MAX_MATERIAL_NAME		100
#define MAX_TEXTURE_NAME		MAX_PATH
#define MAX_MATERIAL_PATH		MAX_PATH
#define INVALID_FRAME			((uint32_t)-1)
#define INVALID_MESH			((uint32_t)-1)
#define INVALID_MATERIAL		((uint32_t)-1)
#define INVALID_SUBSET			((uint32_t)-1)
#define INVALID_ANIMATION_DATA	((uint32_t)-1)
#define INVALID_SAMPLER_SLOT	((uint32_t)-1)
#define ERROR_RESOURCE_VALUE	1

#ifndef SAFE_DELETE
#define SAFE_DELETE(p)			{ if (p) { delete (p); (p) = nullptr; } }
#endif
#ifndef SAFE_DELETE_ARRAY
#define SAFE_DELETE_ARRAY(p)	{ if (p) { delete[] (p); (p) = nullptr; } }
#endif
#ifndef SAFE_RELEASE
#define SAFE_RELEASE(p)			{ if (p) { (p)->Release(); (p) = nullptr; } }
#endif

namespace XUSG
{
	template<typename TYPE> bool IsErrorResource(TYPE data)
	{
		if (static_cast<TYPE>(ERROR_RESOURCE_VALUE) == data) return true;

		return false;
	}

	//--------------------------------------------------------------------------------------
	// Enumerated Types.
	//--------------------------------------------------------------------------------------
	enum SDKMESH_PRIMITIVE_TYPE
	{
		PT_TRIANGLE_LIST = 0,
		PT_TRIANGLE_STRIP,
		PT_LINE_LIST,
		PT_LINE_STRIP,
		PT_POINT_LIST,
		PT_TRIANGLE_LIST_ADJ,
		PT_TRIANGLE_STRIP_ADJ,
		PT_LINE_LIST_ADJ,
		PT_LINE_STRIP_ADJ,
		PT_QUAD_PATCH_LIST,
		PT_TRIANGLE_PATCH_LIST
	};

	enum SDKMESH_INDEX_TYPE
	{
		IT_16BIT = 0,
		IT_32BIT
	};

	enum FRAME_TRANSFORM_TYPE
	{
		FTT_RELATIVE = 0,
		FTT_ABSOLUTE		//This is not currently used but is here to support absolute transformations in the future
	};

	//--------------------------------------------------------------------------------------
	// Structures.  Unions with pointers are forced to 64bit.
	//--------------------------------------------------------------------------------------
#pragma pack(push, 8)

	struct SDKMESH_HEADER
	{
		//Basic Info and sizes
		uint32_t Version;
		uint8_t IsBigEndian;
		uint64_t HeaderSize;
		uint64_t NonBufferDataSize;
		uint64_t BufferDataSize;

		//Stats
		uint32_t NumVertexBuffers;
		uint32_t NumIndexBuffers;
		uint32_t NumMeshes;
		uint32_t NumTotalSubsets;
		uint32_t NumFrames;
		uint32_t NumMaterials;

		//Offsets to Data
		uint64_t VertexStreamHeadersOffset;
		uint64_t IndexStreamHeadersOffset;
		uint64_t MeshDataOffset;
		uint64_t SubsetDataOffset;
		uint64_t FrameDataOffset;
		uint64_t MaterialDataOffset;
	};

	struct SDKMESH_VERTEX_BUFFER_HEADER
	{
		uint64_t NumVertices;
		uint64_t SizeBytes;
		uint64_t StrideBytes;

		struct VertexElement
		{
			uint16_t	Stream;		// Stream index
			uint16_t	Offset;		// Offset in the stream in bytes
			uint8_t		Type;		// Data type
			uint8_t		Method;		// Processing method
			uint8_t		Usage;		// Semantics
			uint8_t		UsageIndex;	// Semantic index
		} Decl[MAX_VERTEX_ELEMENTS];

		union
		{
			uint64_t DataOffset;	// (This also forces the union to 64bits)
			VertexBuffer *pVertexBuffer;
		};
	};

	struct SDKMESH_INDEX_BUFFER_HEADER
	{
		uint64_t NumIndices;
		uint64_t SizeBytes;
		uint32_t IndexType;
		union
		{
			uint64_t DataOffset;	// (This also forces the union to 64bits)
			IndexBuffer *pIndexBuffer;
		};
	};

	struct SDKMESH_MESH
	{
		char Name[MAX_MESH_NAME];
		uint8_t NumVertexBuffers;
		uint32_t VertexBuffers[MAX_VERTEX_STREAMS];
		uint32_t IndexBuffer;
		uint32_t NumSubsets;
		uint32_t NumFrameInfluences; //aka bones

		DirectX::XMFLOAT3 BoundingBoxCenter;
		DirectX::XMFLOAT3 BoundingBoxExtents;

		union
		{
			uint64_t SubsetOffset;			//Offset to list of subsets (This also forces the union to 64bits)
			uint32_t *pSubsets;				//Pointer to list of subsets
		};
		union
		{
			uint64_t FrameInfluenceOffset;	//Offset to list of frame influences (This also forces the union to 64bits)
			uint32_t *pFrameInfluences;		//Pointer to list of frame influences
		};
	};

	struct SDKMESH_SUBSET
	{
		char Name[MAX_SUBSET_NAME];
		uint32_t MaterialID;
		uint32_t PrimitiveType;
		uint64_t IndexStart;
		uint64_t IndexCount;
		uint64_t VertexStart;
		uint64_t VertexCount;
	};

	struct SDKMESH_FRAME
	{
		char Name[MAX_FRAME_NAME];
		uint32_t Mesh;
		uint32_t ParentFrame;
		uint32_t ChildFrame;
		uint32_t SiblingFrame;
		DirectX::XMFLOAT4X4 Matrix;
		uint32_t AnimationDataIndex;		//Used to index which set of keyframes transforms this frame
	};

	struct SDKMESH_MATERIAL
	{
		char Name[MAX_MATERIAL_NAME];

		// Use MaterialInstancePath
		char MaterialInstancePath[MAX_MATERIAL_PATH];

		// Or fall back to d3d8-type materials
		char DiffuseTexture[MAX_TEXTURE_NAME];
		char NormalTexture[MAX_TEXTURE_NAME];
		char SpecularTexture[MAX_TEXTURE_NAME];

		DirectX::XMFLOAT4 Diffuse;
		DirectX::XMFLOAT4 Ambient;
		DirectX::XMFLOAT4 Specular;
		DirectX::XMFLOAT4 Emissive;
		float Power;

		union
		{
			uint64_t Force64_1;			//Force the union to 64bits
			Texture2D *pAlbedo;
		};
		union
		{
			uint64_t Force64_2;			//Force the union to 64bits
			Texture2D *pNormal;
		};
		union
		{
			uint64_t Force64_3;			//Force the union to 64bits
			Texture2D *pSpecular;
		};
		uint64_t Force64_4;				// Force the union to 64bits
		uint64_t Force64_5;				// Force the union to 64bits
		uint64_t Force64_6;				// Force the union to 64bits
	};

	struct SDKANIMATION_FILE_HEADER
	{
		uint32_t	Version;
		uint8_t		IsBigEndian;
		uint32_t	FrameTransformType;
		uint32_t	NumFrames;
		uint32_t	NumAnimationKeys;
		uint32_t	AnimationFPS;
		uint64_t	AnimationDataSize;
		uint64_t	AnimationDataOffset;
	};

	struct SDKANIMATION_DATA
	{
		DirectX::XMFLOAT3 Translation;
		DirectX::XMFLOAT4 Orientation;
		DirectX::XMFLOAT3 Scaling;
	};

	struct SDKANIMATION_FRAME_DATA
	{
		char FrameName[MAX_FRAME_NAME];
		union
		{
			uint64_t DataOffset;
			SDKANIMATION_DATA* pAnimationData;
		};
	};

#pragma pack(pop)

	static_assert(sizeof(SDKMESH_VERTEX_BUFFER_HEADER::VertexElement) == 8, "Vertex element structure size incorrect");
	static_assert(sizeof(SDKMESH_HEADER) == 104, "SDK Mesh structure size incorrect");
	static_assert(sizeof(SDKMESH_VERTEX_BUFFER_HEADER) == 288, "SDK Mesh structure size incorrect");
	static_assert(sizeof(SDKMESH_INDEX_BUFFER_HEADER) == 32, "SDK Mesh structure size incorrect");
	static_assert(sizeof(SDKMESH_MESH) == 224, "SDK Mesh structure size incorrect");
	static_assert(sizeof(SDKMESH_SUBSET) == 144, "SDK Mesh structure size incorrect");
	static_assert(sizeof(SDKMESH_FRAME) == 184, "SDK Mesh structure size incorrect");
	static_assert(sizeof(SDKMESH_MATERIAL) == 1256, "SDK Mesh structure size incorrect");
	static_assert(sizeof(SDKANIMATION_FILE_HEADER) == 40, "SDK Mesh structure size incorrect");
	static_assert(sizeof(SDKANIMATION_DATA) == 40, "SDK Mesh structure size incorrect");
	static_assert(sizeof(SDKANIMATION_FRAME_DATA) == 112, "SDK Mesh structure size incorrect");

#ifndef _CONVERTER_APP_

	//--------------------------------------------------------------------------------------
	// SDKMesh class. This class reads the sdkmesh file format for use by the samples
	//--------------------------------------------------------------------------------------
	class SDKMesh
	{
	public:
		SDKMesh() noexcept;
		virtual ~SDKMesh();

		virtual HRESULT Create(_In_ const Device &device, _In_z_ const wchar_t *szFileName);
		virtual HRESULT Create(_In_ const Device &device, uint8_t *pData, size_t DataBytes, _In_ bool bCopyStatic = false);
		virtual HRESULT LoadAnimation(_In_z_ const wchar_t *szFileName);
		virtual void ReleaseUploadResources();
		virtual void Destroy();

		//Frame manipulation
		void TransformBindPose(_In_ DirectX::CXMMATRIX world);
		void TransformMesh(_In_ DirectX::CXMMATRIX world, _In_ double fTime);

#if 0
		//Direct3D 11 Rendering
		virtual void Render(_In_ ID3D11DeviceContext* pd3dDeviceContext,
			_In_ UINT iDiffuseSlot = INVALID_SAMPLER_SLOT,
			_In_ UINT iNormalSlot = INVALID_SAMPLER_SLOT,
			_In_ UINT iSpecularSlot = INVALID_SAMPLER_SLOT);
		virtual void RenderAdjacent(_In_ ID3D11DeviceContext* pd3dDeviceContext,
			_In_ UINT iDiffuseSlot = INVALID_SAMPLER_SLOT,
			_In_ UINT iNormalSlot = INVALID_SAMPLER_SLOT,
			_In_ UINT iSpecularSlot = INVALID_SAMPLER_SLOT);
#endif

		// Helpers (Graphics API specific)
		static PrimitiveTopology GetPrimitiveType(_In_ SDKMESH_PRIMITIVE_TYPE PrimType);
		Format GetIBFormat(_In_ uint32_t iMesh) const;

		VertexBuffer		*GetVertexBuffer(_In_ uint32_t iMesh, _In_ uint32_t iVB) const;
		IndexBuffer			*GetIndexBuffer(_In_ uint32_t iMesh) const;
		SDKMESH_INDEX_TYPE	GetIndexType(_In_ uint32_t iMesh) const;

		IndexBuffer			*GetAdjIndexBuffer(_In_ uint32_t iMesh) const;

		// Helpers (general)
		const char			*GetMeshPathA() const;
		const wchar_t		*GetMeshPathW() const;
		uint32_t			GetNumMeshes() const;
		uint32_t			GetNumMaterials() const;
		uint32_t			GetNumVBs() const;
		uint32_t			GetNumIBs() const;

		VertexBuffer		*GetVB11At(_In_ uint32_t iVB) const;
		IndexBuffer			*GetIB11At(_In_ uint32_t iIB) const;

		uint8_t				*GetRawVerticesAt(_In_ uint32_t iVB) const;
		uint8_t				*GetRawIndicesAt(_In_ uint32_t iIB) const;

		SDKMESH_MATERIAL	*GetMaterial(_In_ uint32_t iMaterial) const;
		SDKMESH_MESH		*GetMesh(_In_ uint32_t iMesh) const;
		uint32_t			GetNumSubsets(_In_ uint32_t iMesh) const;
		SDKMESH_SUBSET		*GetSubset(_In_ uint32_t iMesh, _In_ uint32_t iSubset) const;
		uint32_t			GetVertexStride(_In_ uint32_t iMesh, _In_ uint32_t iVB) const;
		uint32_t			GetNumFrames() const;
		SDKMESH_FRAME		*GetFrame(_In_ uint32_t iFrame) const;
		SDKMESH_FRAME		*FindFrame(_In_z_ const char *pszName) const;
		uint64_t			GetNumVertices(_In_ uint32_t iMesh, _In_ uint32_t iVB) const;
		uint64_t			GetNumIndices(_In_ uint32_t iMesh) const;
		DirectX::XMVECTOR	GetMeshBBoxCenter(_In_ uint32_t iMesh) const;
		DirectX::XMVECTOR	GetMeshBBoxExtents(_In_ uint32_t iMesh) const;
		uint32_t			GetOutstandingResources() const;
		uint32_t			GetOutstandingBufferResources() const;
		bool				CheckLoadDone();
		bool				IsLoaded() const;
		bool				IsLoading() const;
		void				SetLoading(_In_ bool bLoading);
		bool				HadLoadingError() const;

		// Animation
		uint32_t			GetNumInfluences(_In_ uint32_t iMesh) const;
		DirectX::XMMATRIX	GetMeshInfluenceMatrix(_In_ uint32_t iMesh, _In_ uint32_t iInfluence) const;
		uint32_t			GetAnimationKeyFromTime(_In_ double fTime) const;
		DirectX::XMMATRIX	GetWorldMatrix(_In_ uint32_t iFrameIndex) const;
		DirectX::XMMATRIX	GetInfluenceMatrix(_In_ uint32_t iFrameIndex) const;
		bool				GetAnimationProperties(_Out_ uint32_t *pNumKeys, _Out_ float* pFrameTime) const;

	protected:
		void loadMaterials(_In_reads_(NumMaterials) SDKMESH_MATERIAL *pMaterials, _In_ uint32_t NumMaterials);

		HRESULT createVertexBuffer(_In_ SDKMESH_VERTEX_BUFFER_HEADER *pHeader, _In_reads_(pHeader->SizeBytes) void *pVertices);
		HRESULT createIndexBuffer(_In_ SDKMESH_INDEX_BUFFER_HEADER *pHeader, _In_reads_(pHeader->SizeBytes) void *pIndices);

		virtual HRESULT createFromFile(_In_opt_ const Device &device, _In_z_ const wchar_t *szFileName);
		virtual HRESULT createFromMemory(_In_opt_ const Device &device, _In_reads_(DataBytes) uint8_t *pData,
			_In_ size_t DataBytes, _In_ bool bCopyStatic);

		// Frame manipulation
		void transformBindPoseFrame(_In_ uint32_t iFrame, _In_ DirectX::CXMMATRIX parentWorld);
		void transformFrame(_In_ uint32_t iFrame, _In_ DirectX::CXMMATRIX parentWorld, _In_ double fTime);
		void transformFrameAbsolute(_In_ uint32_t iFrame, _In_ double fTime);

#if 0
		//Direct3D 11 rendering helpers
		void RenderMesh(_In_ UINT iMesh,
			_In_ bool bAdjacent,
			_In_ ID3D11DeviceContext* pd3dDeviceContext,
			_In_ UINT iDiffuseSlot,
			_In_ UINT iNormalSlot,
			_In_ UINT iSpecularSlot);
			void RenderFrame(_In_ UINT iFrame,
			_In_ bool bAdjacent,
			_In_ ID3D11DeviceContext* pd3dDeviceContext,
			_In_ UINT iDiffuseSlot,
			_In_ UINT iNormalSlot,
			_In_ UINT iSpecularSlot);
#endif

		// These are the pointers to the two chunks of data loaded in from the mesh file
		uint8_t *m_pStaticMeshData;
		uint8_t *m_pHeapData;
		uint8_t *m_pAnimationData;
		uint8_t **m_ppVertices;
		uint8_t **m_ppIndices;

		// Keep track of the path
		wchar_t	m_strPathW[MAX_PATH];
		char	m_strPath[MAX_PATH];

		// General mesh info
		SDKMESH_HEADER					*m_pMeshHeader;
		SDKMESH_VERTEX_BUFFER_HEADER	*m_pVertexBufferArray;
		SDKMESH_INDEX_BUFFER_HEADER		*m_pIndexBufferArray;
		SDKMESH_MESH					*m_pMeshArray;
		SDKMESH_SUBSET					*m_pSubsetArray;
		SDKMESH_FRAME					*m_pFrameArray;
		SDKMESH_MATERIAL				*m_pMaterialArray;

		// Adjacency information (not part of the m_pStaticMeshData, so it must be created and destroyed separately )
		SDKMESH_INDEX_BUFFER_HEADER		*m_pAdjacencyIndexBufferArray;

		// Animation
		SDKANIMATION_FILE_HEADER		*m_pAnimationHeader;
		SDKANIMATION_FRAME_DATA			*m_pAnimationFrameData;
		DirectX::XMFLOAT4X4				*m_pBindPoseFrameMatrices;
		DirectX::XMFLOAT4X4				*m_pTransformedFrameMatrices;
		DirectX::XMFLOAT4X4				*m_pWorldPoseFrameMatrices;

		std::vector<Resource>			m_resourceUploads;

	private:
		uint32_t m_numOutstandingResources;
		bool m_isLoading;
		//uint8_t *m_pBufferData;
		HANDLE m_filehandle;
		//HANDLE m_hFileMappingObject;
		//std::vector<uint8_t*> m_mappedPointers;
		Device m_device;
		GraphicsCommandList m_commandList;
	};

#endif
}
