#ifndef _INC_WBSP2MODEL
#define _INC_WBSP2MODEL

#include "../Model_BSP/XWCommon.h"
#include "../Model_BSP/XWCommon_OctAABB.h"
#include "XW2Common.h"
#include "../../XR/XR.h"
#include "../../XR/Solids/MCSolid.h"
#include "../../XR/Solids/XRIndexedSolid.h"
#include "../../XR/XRVBContainer.h"
#include "../../XR/XRShader.h"

class CXR_VBManager;
class CXR_VertexBuffer;

enum
{
	XR_LOCALVBM_MAXDEFERREDVB = 32,
};

class CXR_LocalVBManager
{
	CXR_VBManager* m_pVBM;
	uint8* m_pLocalHeap;
	uint m_LocalPos;
	uint m_LocalHeapSize;
	uint m_nDeferredVB;
	CXR_VertexBuffer* m_lpDeferredVB[XR_LOCALVBM_MAXDEFERREDVB];

public:
	void Clear()
	{
		m_pVBM = NULL;
		m_pLocalHeap = NULL;
		m_LocalPos = 0;
		m_LocalHeapSize = 0;
	}

	void Begin(CXR_VBManager* _pVBM, uint _LocalHeapSize)
	{
		m_pVBM = _pVBM;
		m_LocalHeapSize = _LocalHeapSize;
		m_LocalPos = 0;
		uint8* pLocalHeap = (uint8*) _pVBM->Alloc(_LocalHeapSize+128);
		if (pLocalHeap)
			pLocalHeap = (uint8*)((((mint)pLocalHeap)+127) & ~127);
		m_pLocalHeap = pLocalHeap;
		m_nDeferredVB = 0;
	}

	void End()
	{
		if (m_nDeferredVB)
			m_pVBM->AddVBArray(m_lpDeferredVB, m_nDeferredVB);
		Clear();
	}

	CXR_LocalVBManager()
	{
		Clear();
	}

	template<uint TAlign>
	M_FORCEINLINE void* AllocAlign(uint _Size)
	{
		if (!m_pLocalHeap)
			return NULL;

		_Size = (_Size+TAlign-1) & ~(TAlign-1);
		uint Pos = (m_LocalPos+TAlign-1) & ~(TAlign-1);
		if (_Size+Pos > m_LocalHeapSize)
		{
			if (_Size > (m_LocalHeapSize >> 3))
				return m_pVBM->Alloc(_Size);

			uint8* pLocalHeap = (uint8*)m_pVBM->Alloc(m_LocalHeapSize+128);
			if (!pLocalHeap)
			{
				m_pLocalHeap = NULL;
				return NULL;
			}

			pLocalHeap = (uint8*)((((mint)pLocalHeap)+127) & ~127);
			m_pLocalHeap = pLocalHeap;
			m_LocalPos = _Size;
			return pLocalHeap;
		}

		uint8* pRet = m_pLocalHeap+Pos;

		{
			uint8*M_RESTRICT pLocalHeap = m_pLocalHeap;
			uint32 OldCache = (Pos + 127) & ~127;
			uint32 NewCache = (Pos + 127 + _Size) & ~127;
			uint32 nLines = (NewCache - OldCache) >> 7;
			if(nLines > 0 && nLines < 8)
			{
				if(nLines & 4)
				{
					uint32 ThisCache = OldCache;
					OldCache += 512;
					M_PREZERO128(ThisCache, pLocalHeap);
					M_PREZERO128(ThisCache + 128, pLocalHeap);
					M_PREZERO128(ThisCache + 256, pLocalHeap);
					M_PREZERO128(ThisCache + 384, pLocalHeap);
				}
				if(nLines & 2)
				{
					uint32 ThisCache = OldCache;
					OldCache += 256;
					M_PREZERO128(ThisCache, pLocalHeap);
					M_PREZERO128(ThisCache + 128, pLocalHeap);
				}
				if(nLines & 1)
				{
					uint32 ThisCache = OldCache;
					OldCache += 128;
					M_PREZERO128(ThisCache, pLocalHeap);
				}
			}
		}

		m_LocalPos = Pos + _Size;
		return pRet;
	}

	M_FORCEINLINE void* Alloc(uint _Size) { return AllocAlign<4>(_Size); };
	M_FORCEINLINE void* AllocAlign16(uint _Size) { return AllocAlign<16>(_Size); };

	M_FORCEINLINE uint16* Alloc_uint16(uint _nElem) { return (uint16*)AllocAlign<4>(_nElem * sizeof(uint16)); };
	M_FORCEINLINE uint32* Alloc_uint32(uint _nElem) { return (uint32*)AllocAlign<4>(_nElem * sizeof(uint32)); };

	M_FORCEINLINE CXR_VertexBuffer* Alloc_VB()
	{
		CXR_VertexBuffer* pVB = (CXR_VertexBuffer*) Alloc(sizeof(CXR_VertexBuffer));
		if (pVB)
		{
#ifdef M_Profile
			pVB->m_Link.Construct();
#endif
			pVB->Clear();
		}
		return pVB;
	}

	M_FORCEINLINE CXR_VertexBuffer* Alloc_VBAttrib()
	{
		CXR_VertexBuffer* pFirstVB = (CXR_VertexBuffer*)Alloc(sizeof(CXR_VertexBuffer) + sizeof(CRC_Attributes));
		if (!pFirstVB) 
			return NULL;

#ifdef M_Profile
		pFirstVB->m_Link.Construct();
#endif
		pFirstVB->Clear();
		pFirstVB->m_pAttrib = (CRC_Attributes*)(pFirstVB + 1);
		pFirstVB->m_pAttrib->SetDefault();
		return pFirstVB;
	}

	M_FORCEINLINE CXR_VertexBuffer* Construct_VB_IDChain(void*M_RESTRICT _pMemVB, void*M_RESTRICT _pMemChain)
	{
		CXR_VertexBuffer*M_RESTRICT pVB = (CXR_VertexBuffer*) _pMemVB;
#ifdef M_Profile
		pVB->m_Link.Construct();
#endif
		pVB->Clear();
		CXR_VBIDChain*M_RESTRICT pChain = (CXR_VBIDChain*)_pMemChain;
		pChain->Clear();
		pVB->SetVBIDChain(pChain);
		return pVB;
	}
	M_FORCEINLINE CXR_VertexBuffer* Alloc_VB_IDChain()
	{
		CXR_VertexBuffer* pVB = (CXR_VertexBuffer*) Alloc(sizeof(CXR_VertexBuffer) + sizeof(CXR_VBIDChain));
		if (!pVB)
			return false;
		Construct_VB_IDChain(pVB, pVB+1);
		return pVB;
	}

	void AddVB(CXR_VertexBuffer* _pVB)
	{
		int nVB = m_nDeferredVB;
		m_lpDeferredVB[nVB] = _pVB;
		nVB++;
		if (nVB == XR_LOCALVBM_MAXDEFERREDVB)
		{
			m_pVBM->AddVBArray(m_lpDeferredVB, nVB);
			nVB = 0;
		}
		m_nDeferredVB = nVB;
	}
};

#define MODEL_BSP_EPSILON 0.001f


#define MODEL_BSP_MAXVIEWS			8

#define MODEL_BSP_MAXFACEVERTICES	32

#define MODEL_BSP_MAXDYNAMICLIGHTS 32	// Limited by 32-bit mask used as per-face dynamic light status.

#define MODEL_BSP_SPECBUMP
//#define MODEL_BSP_SPECBUMPNUM 5

//#define MODEL_BSP_LITFOG

#define MODEL_BSP_MAXDIAMETERCLASSES 16

#define XW_LIGHT_EXT_SHADOWS		1
#define XW_LIGHT_EXT_DYNSHADOWS		2
#define XW_LIGHT_INT_SHADOWS		4
#define XW_LIGHT_INT_DYNSHADOWS		8

#define SBELEM_MAXBRUSHES 4096
#define SBELEM_MAXFACES 16

#define SBELEM_FACE(iElem) ((iElem >> 12) & 0xf)
#define SBELEM_BRUSH(iElem) (iElem & 0xfff)
#define SBELEM(iBrush, iFace) ((iBrush & 0xfff) + (iFace << 12))

vec128 CalcBoxScissor(const class CBSP2_ViewInstance* _pView, vec128 _BoxMin, vec128 _BoxMax);

M_FORCEINLINE void CalcBoxScissor(const CBSP2_ViewInstance* _pView, const CBox3Dfp32& _Box, CScissorRect& _Scissor)
{
	vec128 vboxmin = M_VLd_P3_Slow(&_Box.m_Min);
	vec128 vboxmax = M_VLd_P3_Slow(&_Box.m_Max);

	M_VStAny64(CalcBoxScissor(_pView, vboxmin, vboxmax), &_Scissor);
}


// -------------------------------------------------------------------
//  Texture container designed for lightmaps
// -------------------------------------------------------------------
class CXR_Model_BSP2;

// -------------------------------------------------------------------
#define RPORTAL_MAXPLANES 16

// -------------------------------------------------------------------
//  Link-instances
// -------------------------------------------------------------------
class CBSP2_Link
{
public:
	uint16 m_iPortalLeaf;
	uint16 m_ID;
	uint16 m_iLinkNextPL;
	uint16 m_iLinkPrevPL;
	uint16 m_iLinkNextObject;
	uint16 m_iLinkPrevObject;

	CBSP2_Link()
	{
		m_iPortalLeaf = 0;
		m_ID = 0;
		m_iLinkNextPL = 0;
		m_iLinkPrevPL = 0;
		m_iLinkNextObject = 0;
		m_iLinkPrevObject = 0;
	}
};

// -------------------------------------------------------------------
class CBSP2_ViewInstance;

class CBSP2_LinkContext : public CReferenceCount
{
	TThinArray<CBSP2_Link> m_lLinks;
	CBSP2_Link* m_pLinks;
	CIDHeap m_LinkHeap;
	int m_LinkHeapMinAvail;
	int m_MaxID;

	TThinArray<uint16> m_lIDLinkMap;		// Connects to next/prev-PL chain.
	TThinArray<uint16> m_lPLLinkMap;		// Portal-leaf link-map, connects to next/prev-object chain
	TThinArray<CBox3Dfp32> m_lIDLinkBox;		// Box that was used to link an ID, used for culling when enumerating visibles.
	TThinArray<uint8> m_lIDFlags;

	TThinArray<uint8> m_lPLBlock;

	uint16* m_pIDLinkMap;
	uint16* m_pPLLinkMap;

	uint16 m_bPLBlock;
	uint16 m_iPLInfinite;						// PL-index where 'infinite-elements' are linked.

	CXR_Model_BSP2* m_pModel;

	int m_iNextLink;
	int m_iPrevLink;
	int m_CurLinkID;

public:
	void AddPortalLeaf(int _iNode);			// Only use this after InsertBegin()
	void ExpandCurrentBox(const CBox3Dfp32& _Box);
	const CBox3Dfp32& GetElementBox(int _Elem) { return m_lIDLinkBox[_Elem]; };
	uint16 GetElementFlags(int _Elem) { return m_lIDFlags[_Elem]; };

	void SetElementBox(int _Elem, const CBox3Dfp32&  _Box) { m_lIDLinkBox[_Elem] = _Box; };
	void SetElementFlags(int _Elem, int _Flags) { m_lIDFlags[_Elem] = _Flags; };


	DECLARE_OPERATOR_NEW


	CBSP2_LinkContext();
	void Create(CXR_Model_BSP2* _pModel, int _NumIDs, int _NumLinks, int _nPL, bool _bNeedBoxes);
	void Insert(int _ID, const CBox3Dfp32& _Box, int _Flags);
	void InsertWithPVS(int _ID, const CBox3Dfp32& _Box, int _Flags, const uint8* _pPVS);
	void Insert(int _ID, int _iPL);
	void InsertBegin(int _ID, int _Flags);
	void InsertEnd(int _ID);
	void InsertInfinite(int _ID, int _Flags);
	void Remove(int _ID);

protected:
	int MergeIDs(uint16* _piRet, int _nRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge);
	int EnumPortalLeaf(int _iPL, uint16* _piRet, int _MaxRet);
	int EnumPortalLeafWithClip(int _iPL, uint16* _piRet, int _MaxRet, const CRC_ClipVolume* _pClip);
	int EnumBox_r(int _iNode, const CBox3Dfp32& _Box, uint16* _piRet, int _MaxRet);
public:
	int EnumBox(const CBox3Dfp32& _Box, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge);
	int EnumVisible(CBSP2_ViewInstance* _pView, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge);
	int EnumPVS(const uint8* _pPVS, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem,  int _nMerge);
	int EnumIDs(int _iPL, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge);

	int EnumPortalLeafList(uint16* _piPL, int _nPL, uint16* _piRet, int _MaxRet);

	int EnumPortalLeaves(int _ID, uint16* _piRet, int _MaxRet);
	int EnumPortalLeaves_Single(int _ID);

	const CBSP2_Link* GetLinks() { return m_pLinks; };
	int GetLink_PL(int _iPL) { return m_pPLLinkMap[_iPL]; };
	int GetLink_ID(int _ID) { return m_pIDLinkMap[_ID]; };

	int GetNumLinkedIDs() const;
	void TraceContents() const;

	friend class CXR_Model_BSP2;
	friend class CBSP2_SceneGraph;
};

typedef TPtr<CBSP2_LinkContext> spCBSP2_LinkContext;

// -------------------------------------------------------------------
class CBSP2_SpotLightClip
{
public:
	CPlane3Dfp32 m_lPlanes[5];
	CVec3Dfp32 m_lV[5];

	static void CalcSpotPlanes(const CXR_Light* _pL, CPlane3Dfp32* _pP);
	static void CalcSpotVerticesAndPlanes(const CXR_Light* _pL, CVec3Dfp32* _pV, CPlane3Dfp32* _pP);
	void Create(const CXR_Light* _pL);

	int IntersectBox(const CBox3Dfp32& _Box) const;
	void GetBoundBox(CBox3Dfp32& _Box) const;
};

class CBSP2_SceneGraph : public CXR_SceneGraphInstance
{
public:
	// YOU HAVE TO TAKE THE m_Lock CRITICALSECTION BEFORE MESSING WITH THESE OUTSIDE OF CBSP2_SceneGraph
	// protected functions are not threadsafe, they rely on public functions for threadsafety
	class CXR_Model_BSP2* m_pModel;
	CBSP2_LinkContext m_Objects;
	CBSP2_LinkContext m_Lights;
	CBSP2_LinkContext m_ObjectLights;
	CBSP2_LinkContext m_ObjectsNoShadow;

	CIndexPool16 m_DeferredObjects;
	CIndexPool16 m_DeferredLights;

	TThinArray<CXR_Light> m_lLights;
	TThinArray<CBSP2_SpotLightClip> m_lDynamicLightClip;
	uint16 m_iFirstDynamic;
//	uint8 m_nDynamic;
	uint8 m_iNextDynamic;
	uint8 m_MaxDynamic;
	uint8 m_MaxDynamicAnd;

	MRTC_CriticalSection m_Lock;

	CBSP2_SceneGraph(class CXR_Model_BSP2* _pModel) : m_pModel(_pModel) {};
	~CBSP2_SceneGraph();
	void Create(int _ElemHeap, int _LightHeap, int _CreationFlags);

	int EnumElementLights(CBSP2_LinkContext* _pLinkCtx, uint16 _Elem, uint16* _piLights, int _nMaxLights);

	virtual void SceneGraph_CommitDeferred();

	virtual void SceneGraph_LinkElement(uint16 _Elem, const CBox3Dfp32& _Box, int _Flags);
	virtual void SceneGraph_LinkInfiniteElement(uint16 _Elem, int _Flags);
	virtual void SceneGraph_UnlinkElement(uint16 _Elem);
	virtual int SceneGraph_EnumerateElementNodes(int _Elem, uint32* _piRetNodes, int _MaxRet);

	virtual int SceneGraph_EnumerateNodes(const uint32* _piNodes, int _nNodes, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem = NULL, int _nMergeElem = 0);
	virtual int SceneGraph_EnumeratePVS(const uint8* _pPVS, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem = NULL, int _nMergeElem = 0);
	virtual int SceneGraph_EnumerateView(int _iView, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem = NULL, int _nMergeElem = 0);

protected:
	void Internal_LinkElement(uint16 _Elem, const CBox3Dfp32& _Box, int _Flags);
	void DeferrLightElements(CBSP2_LinkContext* _pLinkCtx, uint16 _iLight);

	class CBSP2_LightGUIDEntry
	{
	public:
		uint16 m_LightGUID;
	};

	CMap16 m_LightGUIDMap;

	void UnlinkLight(int _iLight);
	void LinkLight(CXR_Light& _Light);
	int GetFreeDynamicLightIndex();
	int GetNumFreeDynamicLights();

public:
	virtual int SceneGraph_Light_GetMaxIndex();
	virtual void SceneGraph_Light_LinkDynamic(const CXR_Light& _Light);							// NOTE: You MUST set m_LightGUID to a unique value! Otherwise you might overwrite other lights.
	virtual void SceneGraph_Light_AddPrivateDynamic(const CXR_Light& _Light);					// NOTE: You MUST set m_LightGUID to a unique value! Otherwise you might overwrite other lights.
	virtual void SceneGraph_Light_ClearDynamics();												// Remove all dynamic lights from the SGI. Typically only called by system code.
	virtual int SceneGraph_Light_GetIndex(int _LightGUID);										// Convert LightGUID to iLight, if return == 0 _LightGUID doesn't exist.
	virtual int SceneGraph_Light_GetGUID(int _iLight);
	virtual void SceneGraph_Light_Unlink(int _iLight);

	virtual void SceneGraph_Light_SetIntensity(int _iLight, const CVec3Dfp32& _Intensity, bool _bIsOff);
	virtual void SceneGraph_Light_SetRotation(int _iLight, const CMat4Dfp32& _Rotation);
	virtual void SceneGraph_Light_SetPosition(int _iLight, const CMat4Dfp32& _Pos);
	virtual void SceneGraph_Light_SetProjectionMap(int _iLight, int _TextureID, const CMat4Dfp32* _Pos);
	virtual void SceneGraph_Light_Get(int _iLight, CXR_Light& _Light);
	virtual void SceneGraph_Light_GetIntensity(int _iLight, CVec3Dfp32& _Intensity);

	virtual int SceneGraph_Light_Enum(const CBox3Dfp32& _Box, const CXR_Light** _lpLights, int _nMaxLights);
	virtual int SceneGraph_Light_Enum(const uint16* _piPL, int _nPL, const CBox3Dfp32& _Box, const CXR_Light** _lpLights, int _nMaxLights);

	CXR_Light* GetLight(int _iLight);
};

typedef TPtr<CBSP2_SceneGraph> spCBSP2_SceneGraph;


// -------------------------------------------------------------------
//  View-instances
// -------------------------------------------------------------------
class CBSP2_ViewInstance : public CReferenceCount
{
public:

	DECLARE_OPERATOR_NEW


	// Data used during rendering of a specific view.

	TThinArray<uint8> m_lPortalIDStates;		// Bit-array

	// RPortals
	TThinArray<uint16> m_liLeafRPortals;		// Index to RPortal for each leaf.
	TThinArray<uint16> m_liRPortalNext;			// Index to next RPortal for each m_lRPortals.
	TArray<CRC_ClipVolume> m_lRPortals;			// RPortals
//	TThinArray<CRect2Duint16> m_lPortalScissor;
	TThinArray<CScissorRect> m_lPortalScissor;
	uint16* m_piRPortalNext;					// Ptr to m_liRPortalNext
	CRC_ClipVolume* m_pRPortals;				// Ptr to m_lRPortals
	int m_MaxRPortals;
	int m_nRPortals;							// No. of RPortals. The list has fixed length.

	// Leaf-list
	const uint8* m_pPVS;
	TThinArray<uint16> m_liVisPortalLeaves;		// List of portal-leaves in rendering order
	int m_MaxVisLeaves;
	int m_nVisLeaves;							// No. of leaves. The list has fixed length.

	CRct m_VPRect;
	int m_VPWidth;
	int m_VPHeight;
	CMat4Dfp32 m_VPVMat;							// VMat
	CVec4Dfp32 m_VPScaleVec;					// 2D projection
	CVec4Dfp32 m_VPMidVec;
	CVec4Dint32 m_VPRectMinVec;
	CVec4Dint32 m_VPRectMaxVec;

	CPlane3Dfp32 m_LocalBackPlane;				// Backplane in model-space
	CPlane3Dfp32 m_LocalFrontPlane;				// Frontplane in model-space
	fp32 m_BackPlaneInv;							// Backplane distance reciprocal.
	fp32 m_ViewBackPlane;						// Viewport backplane.

	CBSP2_SceneGraph* m_pSceneGraph;
	TThinArrayAlign<CXR_LightOcclusionInfo, 16> m_lLightOcclusion;
	TThinArray<uint16> m_liLightsUsed;
	TThinArray<uint8> m_lActiveSV;

	int InPVS(int _iPL) const
	{
		if (!m_pPVS) return true;
		return m_pPVS[_iPL >> 3] & (1 << (_iPL & 7));
	}

	int PortalOpen(int _PortalID) const
	{
		int Pos = _PortalID >> 3;
		if (!m_lPortalIDStates.ValidPos(Pos)) return 1;
		return m_lPortalIDStates.GetBasePtr()[Pos] & (1 << (_PortalID & 7));
	}

	void Clear()
	{
		m_pPVS = NULL;
		m_MaxRPortals = 0;
		m_nRPortals = 0;
		m_MaxVisLeaves = 0;
		m_nVisLeaves = 0;
	}

	CBSP2_ViewInstance()
	{
		m_pSceneGraph = NULL;
		Clear();
	}
};

typedef TPtr<CBSP2_ViewInstance> spCBSP2_ViewInstance;

class CBSP2_ViewInstanceData
{
public:
	// 16 byte aligned stuff first
	CMat4Dfp32 m_CurVMat;						// VMat
	CMat4Dfp32 m_CurWMat;						// WMat
	CMat4Dfp32 m_CurVMatInv;						// VMat Inverse
	CVec3Dfp32 m_CurLocalVP;						// Viewpoint in local crd.system.
	CRC_ClipVolume m_CurClipVolume;

//	int m_CurrentView;							// View#
};

// -------------------------------------------------------------------
class CXR_FogState;

// -------------------------------------------------------------------
class CBSP2_PortalExt : public CBSP2_Portal
{
public:
	CVec3Dfp32 m_BoxMin;
	CVec3Dfp32 m_BoxMax;
};

class CBSP2_PortalLeafExt : public CBSP2_PortalLeaf
{
public:
	CBox3Dfp32 m_FaceBoundBox;
	uint16 m_nFaces;			// Index and count into m_liPLFaces
	uint16 m_nRefFaces;			// Index and count into m_liPLFaces
	uint16 m_nSkyFaces;			// Index and count into m_liPLFaces
	uint32 m_iiFaces;
	uint32 m_iiRefFaces;
	uint32 m_iiSkyFaces;
	uint32 m_iBaseVertex;
	uint32 m_nVertices;
	uint32 m_iPathNode;
	uint32 m_iFogBox;

	fp32 m_BasePriority;			// Vertexbuffer base priority.

	CBSP2_PortalLeafExt()
	{
		m_FaceBoundBox.m_Min = 0;
		m_FaceBoundBox.m_Max = 0;
		m_nFaces = 0;
		m_iiFaces = 0;
		m_nRefFaces = 0;
		m_iiRefFaces = 0;
		m_iBaseVertex = 0;
		m_nVertices = 0;
		m_iPathNode = 0;
		m_iFogBox = 0;

		m_BasePriority = 0;
	}
};

class CBSP2_PortalLeaf_VBFP
{
public:
	uint32 m_nVBFacePrims;
	uint32 m_iVBFacePrims;

	CBSP2_PortalLeaf_VBFP()
	{
		m_iVBFacePrims = 0;
		m_nVBFacePrims = 0;
	}
};

class CBSP2_VBFacePrim
{
public:
	uint16 m_iVB;
	uint16 m_nPrim;
	uint32 m_iPrim;
//	CBox3Dfp32 m_BoundBox;
};

// -------------------------------------------------------------------
class CBSP2_VBInfo
{
public:
	TThinArray<uint32> m_liFaces;
	uint16 m_VBID;
	uint16 m_iSurfQueue;
	uint16 m_nVertices;
	uint16 m_iSurface;
	uint32 m_Components : 8;
	uint32 m_bNoRelease : 1;

	// Primitive buffer with space for all possible primitives using this buffer, allocated in VBM
	int m_MaxPrim;

	CBSP2_VBInfo()
	{
		m_Components = 0;
		m_bNoRelease = 0;
		m_iSurfQueue = 0;
		m_VBID = ~0;
		m_MaxPrim = 0;
	}

};

class CBSP2_VBPrimQueue
{
public:
	uint16 m_iSurfQueue;
	int m_MaxPrim;		// Duplicate from CBSP2_VBInfo
	uint16* m_pPrim;
	int m_nPrim;

/*	bool AllocPrimitives(CXR_VBManager* _pVBM)
	{
		m_nPrim = 0;
		m_pPrim = _pVBM->Alloc_Int16(m_MaxPrim);
		if (!m_pPrim)
			return false;
		return true;
	}

	bool AllocPrimitives(CXR_LocalVBManager* _pVBM)
	{
		m_nPrim = 0;
		m_pPrim = (uint16*)_pVBM->Alloc(2*m_MaxPrim);
		if (!m_pPrim)
			return false;
		return true;
	}*/

	void Create(const CBSP2_VBInfo& _VBI)
	{
		m_iSurfQueue = _VBI.m_iSurfQueue;
		m_MaxPrim = _VBI.m_MaxPrim;
		m_pPrim = NULL;
		m_nPrim = 0;
	}

	void ClearPrimitives()
	{
		m_pPrim = NULL;
		m_nPrim = 0;
	}
};

// -------------------------------------------------------------------
class CBSP2_LightVBInfo
{
public:
	uint16 m_iLight;
	uint16 m_iVB;

	// Primitive buffer with space for all possible primitives using this buffer, allocated in VBM
	uint32 m_MaxPrim;
	uint16* m_pPrim;
	uint16 m_nPrim;
	uint16 m_iSurface;

	CBSP2_LightVBInfo();
};

// -------------------------------------------------------------------
class CBSP2_VertexBuffer : public CReferenceCount
{
public:

	DECLARE_OPERATOR_NEW


	TThinArray<CVec3Dfp32> m_lV;
	TThinArray<CVec3Dfp32> m_lN;
	TThinArray<CVec3Dfp32> m_lTangU;
	TThinArray<CVec3Dfp32> m_lTangV;
	TThinArray<CVec2Dfp32> m_lTV1;
	TThinArray<CVec2Dfp32> m_lTV2;
	TThinArray<CPixel32> m_lCol;
	TThinArray<fp32> m_lLMScale;

	CBSP2_VertexBuffer()
	{
	}

	void Create(int _nV, int _Components = -1)
	{
		m_lV.SetLen(_nV);
		if (_Components & 1) m_lTV1.SetLen(_nV);
		if (_Components & 2) m_lTV2.SetLen(_nV);
		if (_Components & 4) m_lCol.SetLen(_nV);
		if (_Components & 8) m_lN.SetLen(_nV);
		if (_Components & 16) m_lTangU.SetLen(_nV);
		if (_Components & 16) m_lTangV.SetLen(_nV);
		if (_Components & 32) m_lLMScale.SetLen(_nV);
	}
};

typedef TPtr<CBSP2_VertexBuffer> spCBSP2_VertexBuffer;

// -------------------------------------------------------------------
class CBSP2_SurfaceQueue
{
public:
	uint16 m_iSurface;
	int16 m_iLMTexture;
	uint32 m_SurfaceFlags;

	CBSP2_SurfaceQueue()
	{
		m_iSurface = 0;
		m_iLMTexture = -1;
		m_SurfaceFlags = 0;
	}
};

class CBSP2_SurfaceQueueData
{
public:
	uint16 m_iSurface;
	int16 m_iLMTexture;
	uint32 m_SurfaceFlags;
	CBSP2_VBChain m_VBChain;

	M_FORCEINLINE void Create(const CBSP2_SurfaceQueue& _SQ)
	{
		m_iSurface = _SQ.m_iSurface;
		m_iLMTexture = _SQ.m_iLMTexture;
		m_SurfaceFlags = _SQ.m_SurfaceFlags;
	}

	M_FORCEINLINE void AddChain(CBSP2_VBChain *_pVB)
	{
		m_VBChain.AddChain(_pVB->m_pChain);
	}
	M_FORCEINLINE void AddChain(CXR_VBIDChain *_pVB)
	{
		m_VBChain.AddChain(_pVB);
	}
	M_FORCEINLINE void Clear()
	{
		m_VBChain.Clear();
	}
};

class CBSP2_VBFaceQueueData
{
public:
//	uint32	m_Offset;
	uint32	m_MaxElem;
	CBSP2_VBFaceQueueData()
	{
//		m_Offset = 0;
		m_MaxElem = 0;
	}
};

class CBSP2_VBFaceQueue
{
public:
	uint32* m_pQueue;
	uint32 m_nElem;
	uint32 m_MaxElem;
	uint32 m_DynamicLightMask;

	CBSP2_VBFaceQueue()
	{
		m_pQueue = NULL;
		m_nElem = 0;
		m_MaxElem = 0;
		m_DynamicLightMask = 0;
	}

	void AddElem(uint32 _Elem)
	{
		if (m_nElem >= m_MaxElem) return;
		m_pQueue[m_nElem++] = _Elem;
	}
};

// -------------------------------------------------------------------
class CBSP2_ShadowVolume_Thin
{
public:
	uint16 m_iPL;
	uint16 m_iLight;
	uint16 m_iSLC;				// Index of first Surface/Light cluster (CBSP2_SLCluster)
	uint16 m_nSLC;

	void Create(const CBSP2_ShadowVolume& _SV)
	{
		m_iPL = _SV.m_iPL;
		m_iLight = _SV.m_iLight;
		m_iSLC = _SV.m_iSLC;
		m_nSLC = _SV.m_nSLC;
	}
};

class CBSP2_LightData_Ext : public CBSP2_LightData
{
public:
	TThinArray<CBSP2_ShadowVolume_Thin> m_lSVThin;
	TThinArray<CBox3Dfp32> m_lSVBoundBoxSV;
};

typedef TPtr<CBSP2_LightData_Ext> spCBSP2_LightData_Ext;

// -------------------------------------------------------------------
class CBSP2_FogBox : public CReferenceCount
{
public:
	TThinArray<CPixel32> m_lBox;
	CVec3Dfp32 m_BoxOrigin;
	CVec3Dfp32 m_BoxScale;
	CVec3Dfp32 m_BoxScaleInv;
	uint32 m_MaxIndex;
	uint32 m_BoxX;
	uint32 m_BoxY;
	uint32 m_BoxZ;
	int m_BoxModY;
	int m_BoxModZ;
	int m_Flags;
	int m_UpdateCount;

	inline int GetIndex(int x, int y, int z) const { return x + y*m_BoxModY + z*m_BoxModZ; };

	CBSP2_FogBox();
	void Create(const CBox3Dfp32& _Box, const CVec3Dfp32& _BoxSize);
//	void Create(const CBSP2_LightVolumeInfo& _LightVol);

	void Clear();
	void AddLight(const CXR_Light* _pL);
	void SetAlphaFromBrightness();
	void Filter();

	CPixel32 GetFog(const CVec3Dfp32& _v, int _Alpha) const;
	CPixel32 GetFog_BoxSpace(const CVec3Dfp32& _v, int _Alpha) const;

	void GetFogBoxVectors(const CVec3Dfp32& _p0, const CVec3Dfp32& _v, CVec3Dfp32& _fp0, CVec3Dfp32& _fv) const;
};

typedef TPtr<CBSP2_FogBox> spCBSP2_FogBox;

// -------------------------------------------------------------------
//  Wallmark structures
// -------------------------------------------------------------------
class CBSP2_Wallmark : public CXR_WallmarkDesc
{
public:
	uint16 m_iV;
	uint16 m_nV;
	CVec3Dfp32 m_Pos;
};

class CBSP2_WallmarkContext : public CReferenceCount
{
public:
	TThinArray<CVec3Dfp32> m_lV;
	TThinArray<CVec3Dfp32> m_lN;
	TThinArray<CVec2Dfp32> m_lTV;
	TThinArray<CVec2Dfp32> m_lTV2;
	TThinArray<CVec3Dfp32> m_lTangU;
	TThinArray<CVec3Dfp32> m_lTangV;
	TThinArray<CPixel32> m_lCol;
	int m_nV;
	int m_iVTail;
	int m_iVHead;

	CXR_SurfaceContext* m_pSC;

	TPtr<TQueue<CBSP2_Wallmark> > m_spqWM;
	TPtr<CBSP2_LinkContext> m_spLink;
//	TPtr<CIDHeap> m_spWMAlloc;

	void Clear();

	void FreeWallmark();
	int AddWallmark(const CBSP2_Wallmark& _WM);
	int MaxPut() const;
	int AddVertices(int _nV);

	CBSP2_WallmarkContext();
	void Create(CXR_Model_BSP2* _pModel, int _nV);
};

typedef TPtr<CBSP2_WallmarkContext> spCBSP2_WallmarkContext;


// -------------------------------------------------------------------
class CBSP2_WorldData : public CReferenceCount
{
public:

};

class CBSP2_EnumContext
{
public:
	// Enumeration tagging data
	TThinArray<uint8>	m_lFaceTagList;
	TThinArray<uint32>	m_lFaceUntagList;
	TThinArray<uint8>	m_lEdgeTag;
	uint8* m_pFTag;
	uint32*	m_piFUntag;

	// Face enumeration data
	uint32* m_piEnum;
	int	m_nEnum;
	int	m_MaxEnum;
	int m_MaxUntagEnum;
	int	m_nUntagEnum;
	// PL enumeration data
	uint16* m_piEnumPL;
	int	m_nEnumPL;
	int	m_MaxEnumPL;
	bool m_bEnumError;
	int	m_EnumPL;		// Temp variable

	// Enumeration parameters
	int	m_EnumQuality;
	int	m_EnumMedium;
	int	m_EnumFaceFlags;
	CBox3Dfp32 m_EnumBox;
	CVec3Dfp32 m_EnumSphere;
	fp32 m_EnumSphereR;

	// PCS enumeration data
	class CPotColSet* m_pPCS;
	int	m_iObj;
	int	m_bIN1N2Flags;
	CBox3Dfp32*	m_pTransformedBox;
	class CBSP2_BoxInterleaved* m_pTransformedBoxInterleaved;
	CVec3Dfp32 m_TransformedCenter;

	void Untag()
	{
		for(int i = 0; i < m_nUntagEnum; i++)
		{
			m_pFTag[m_piFUntag[i]]	= 0;
		}
		m_nUntagEnum	= 0;
	}

	void SetupPCSEnum(CPotColSet* _pPCS, int _iObj, int _bIN1N2Flags, const CVec3Dfp32& _Center, CBox3Dfp32* _pTransformedBox, class CBSP2_BoxInterleaved* _pTransformedBoxInterleaved)
	{
		m_pPCS	= _pPCS;
		m_iObj	= _iObj;
		m_bIN1N2Flags	= _bIN1N2Flags;
		m_pTransformedBox	= _pTransformedBox;
		m_pTransformedBoxInterleaved	= _pTransformedBoxInterleaved;
		m_TransformedCenter	= _Center;
	}

	void SetupBoxEnum(const CBox3Dfp32& _Box)
	{
		m_EnumBox	= _Box;
	}

	void SetupSphereEnum(const CVec3Dfp32& _Center, fp32 _Radius)
	{
		m_EnumSphere	= _Center;
		m_EnumSphereR	= _Radius;
	}

	void SetupFaceEnum(uint32* _piEnum, int _MaxEnum)
	{
		m_piEnum	= _piEnum;
		m_MaxEnum	= _MaxEnum;
		m_nEnum		= 0;
	}

	void SetupPLEnum(uint16* _piEnum, int _MaxEnum)
	{
		m_piEnumPL	= _piEnum;
		m_MaxEnumPL	= _MaxEnum;
		m_nEnumPL		= 0;
	}

	void CreateEdges(int _nMaxEdges)
	{
		int lDataSize = (_nMaxEdges + 31) >> 3;
		if(m_lEdgeTag.Len() < lDataSize)
		{
			m_lEdgeTag.SetLen(lDataSize);
			FillChar(m_lEdgeTag.GetBasePtr(), m_lEdgeTag.ListSize(), 0);
		}
	}

	void Create(int _nMaxFaces, int _EnumQuality, int _EnumMedium = -1, int _EnumFaceFlags = -1)
	{
		int lDataSize = (_nMaxFaces + 31) >> 3;
		if(m_lFaceTagList.Len() < lDataSize)
		{
			m_lFaceTagList.SetLen(lDataSize);
			m_lFaceUntagList.SetLen(lDataSize);
			FillChar(m_lFaceTagList.GetBasePtr(), m_lFaceTagList.ListSize(), 0);

			m_pFTag	= m_lFaceTagList.GetBasePtr();
			m_piFUntag	= m_lFaceUntagList.GetBasePtr();

			m_MaxUntagEnum	= lDataSize;
		}

		m_bEnumError	= false;
		m_EnumQuality	= _EnumQuality;
		m_EnumMedium	= _EnumMedium;
		m_EnumFaceFlags	= _EnumFaceFlags;
		m_nEnum			= 0;
		m_MaxEnum		= 0;
		m_nEnumPL		= 0;
		m_MaxEnumPL		= 0;
		m_nUntagEnum	= 0;

		m_piEnum	= 0;
		m_piEnumPL	= 0;
		m_pPCS		= 0;
	}
	CBSP2_EnumContext() : m_pFTag(0), m_piFUntag(0), m_MaxUntagEnum(0)
	{
	}
};


class CBSP2_DynamicLightContext
{
public:
	const uint8* m_Light_pPVS;
	bool m_Light_CurrentEnableShadows;
	bool m_Light_CurrentFrontFaceShadow;
	fp32 m_Light_CurrentFrontFaceShadowMul;

	fp32 m_Light_CurrentRange;
	CVec3Dfp32 m_Light_CurrentPos;
	CBox3Dfp32 m_Light_CurrentBoundBox;

	CVec3Dfp32 m_Light_lCurrentSpotVertices[5];
	CPlane3Dfp32 m_Light_lCurrentSpotPlanes[5];

	CRC_ClipVolume* m_Light_pCurrentClip;
	CBox3Dfp32 m_Light_CurrentShadedBound;
};

// -------------------------------------------------------------------
class CBSP2_View_Params
{
public:
	CBSP2_View_Params()
	{
		Clear();
	}
	void Clear()
	{
		m_pSceneFog = NULL;
		m_pClipVolume = NULL;
		m_pRenderInfo = NULL;
		m_iClosestRPortal = 0x7fffffff;
		m_HighestPrior = -10000000;
		m_pCurrentEngine = NULL;

		m_VisVBox	= CBox3Dfp32(CVec3Dfp32(0), CVec3Dfp32(0));

		m_v0	= CVec3Dfp32(0);
		m_Radius	= 0;

		m_pViewData = NULL;
	}
	CXR_FogState*	m_pSceneFog;
	CRC_ClipVolume* m_pClipVolume;
	CXR_RenderInfo* m_pRenderInfo;
	int				m_iClosestRPortal;
	fp32				m_HighestPrior;
	CXR_Engine*		m_pCurrentEngine;

	CVec2Dfp32 m_PortalOrMin;
	CVec2Dfp32 m_PortalOrMax;

	CBox3Dfp32		m_VisVBox;

	CVec3Dfp32		m_v0;
	fp32				m_Radius;

	CBSP2_ViewInstanceData* m_pViewData;

	void SetupForBox(CXR_FogState* _pSceneFog, const CBox3Dfp32& _VisVBox, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo, CXR_Engine* _pCurrentEngine)
	{
		m_pSceneFog = _pSceneFog;
		m_pClipVolume	= _pClipVolume;
		m_pRenderInfo	= _pRenderInfo;
		m_pCurrentEngine	= _pCurrentEngine;

		m_VisVBox	= _VisVBox;
	}

	void SetupForSphere(CXR_FogState* _pSceneFog, const CVec3Dfp32& _v0, fp32 _Radius, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo, CXR_Engine* _pCurrentEngine)
	{
		m_pSceneFog = _pSceneFog;
		m_pClipVolume	= _pClipVolume;
		m_pRenderInfo	= _pRenderInfo;
		m_pCurrentEngine	= _pCurrentEngine;

		m_v0	= _v0;
		m_Radius	= _Radius;
	}
};

class CBSP2_RenderParams
{
public:
	CXR_Engine*	m_pCurrentEngine;
	const CXR_AnimState* m_pCurAnimState;
	CXR_FogState* m_pSceneFog;
	uint32 m_CurOnRenderFlags;
	CXR_RenderInfo m_RenderInfo;
	CMTime m_EngineTime;
	uint32 m_UnifiedAmbience;

	fp32 m_BasePriority_Opaque;
	fp32 m_BasePriority_Transparent;
	CXR_VBManager* m_pVBM;
	CXR_MediumDesc* m_pCurMedium;

	CMat4Dfp32* m_pVBMatrixM2V;
	CMat4Dfp32* m_pVBMatrixM2W;
	CMat4Dfp32* m_pVBMatrixW2V;
	const CMat4Dfp32* m_pWVelMat;
	CBSP2_ViewInstanceData* m_pViewData;

	CBSP2_VBFaceQueue* m_pVBFaceQueue;
	uint32* m_pFaceQueueHeap;

	CBSP2_SurfaceQueueData* m_pSurfQueues;

	TAP_RCD<CBSP2_VBChain> m_pShaderQueueVBChain;

	TAP_RCD<CBSP2_VBPrimQueue> m_pVBPrimQueue;

	CXW_SurfaceKeyFrame* m_plTmpSurfKeyFrame[ANIMSTATE_NUMSURF];

	uint32 m_CurrentIsPortalWorld : 1;
	//	uint32 m_CurrentTLEnable : 1;
	uint32 m_CurrentRCCaps;

	CXR_LightInfo m_lRenderLightInfo[MODEL_BSP_MAXDYNAMICLIGHTS];
	CXR_Light* m_lRenderLight;

	CXR_LocalVBManager m_LocalVBM;

	uint8 M_ALIGN(16) m_RenderLightData[sizeof(CXR_Light) * MODEL_BSP_MAXDYNAMICLIGHTS];

	uint8 M_ALIGN(16) m_plTmpSurfKeyFrameData[sizeof(CXW_SurfaceKeyFrame) * ANIMSTATE_NUMSURF];


	CBSP2_RenderParams() : 
		m_RenderInfo(NULL), 
		m_lRenderLight((CXR_Light*)m_RenderLightData)
	{
		Clear();
	}
	~CBSP2_RenderParams()
	{
		for(int i = 0; i < ANIMSTATE_NUMSURF; i++)
		{
			if(m_plTmpSurfKeyFrame[i])
			{
				// Run destructor on created temp objects
				m_plTmpSurfKeyFrame[i]->CXW_SurfaceKeyFrame::~CXW_SurfaceKeyFrame();
				m_plTmpSurfKeyFrame[i] = NULL;
			}
		}
	}

	CXW_SurfaceKeyFrame* CreateTempSurfKeyFrame(int _iTemp)
	{
		M_ASSERT(_iTemp >= 0 && _iTemp < ANIMSTATE_NUMSURF, "!");
		if(m_plTmpSurfKeyFrame[_iTemp] == NULL)
		{
			m_plTmpSurfKeyFrame[_iTemp] = new (m_plTmpSurfKeyFrameData +(_iTemp * sizeof(CXW_SurfaceKeyFrame))) CXW_SurfaceKeyFrame;
		}

		return m_plTmpSurfKeyFrame[_iTemp];
	}
	void Clear()
	{
		m_pCurrentEngine = NULL;
		m_pCurAnimState = NULL;
		m_pSceneFog = NULL;
		m_CurOnRenderFlags = 0;
		m_RenderInfo.Clear(NULL);
		m_EngineTime.Reset();
		m_UnifiedAmbience = 0xff000000;

		m_CurrentIsPortalWorld = false;
//		m_CurrentTLEnable	= false;
		m_CurrentRCCaps = 0;

		m_BasePriority_Opaque = CXR_VBPRIORITY_MODEL_OPAQUE;
		m_BasePriority_Transparent = CXR_VBPRIORITY_MODEL_TRANSPARENT;

		m_pVBM = NULL;
		m_pCurMedium = NULL;

		m_pVBMatrixM2V = NULL;
		m_pVBMatrixM2W = NULL;
		m_pVBMatrixW2V = NULL;
		m_pWVelMat = NULL;

		m_pViewData = NULL;

		m_pVBFaceQueue = NULL;
		m_pFaceQueueHeap = NULL;

		m_pSurfQueues = NULL;

		m_pShaderQueueVBChain.Set(NULL, 0);

		for(int i = 0; i < ANIMSTATE_NUMSURF; i++)
			m_plTmpSurfKeyFrame[i]	= NULL;
	}
};
// -------------------------------------------------------------------
class CXR_Model_BSP2;

class CXR_Model_BSP2 : 
	public CXR_SceneGraphInterface, 
	public CXR_ViewClipInterface, 
	public CXR_PhysicsModel, 
	public CXR_FogInterface,
	public CXR_WallmarkInterface,
	public CXR_PortalSurfaceRender,
	public CXR_VBContainer
{
public:

	DECLARE_OPERATOR_NEW

	MRTC_DECLARE;

	friend class CBSP2_SLCIBContainer;

public:
	
	enum
	{
		ENUM_HQ = 1,
		ENUM_FACEFLAGS = 2,
		ENUM_MEDIUMFLAGS = 4,
	};

	void LinkBox_r(CBSP2_LinkContext* _pLinkCtx, int _iNode, const CBox3Dfp32& _Box);
	void LinkBox_i(CBSP2_LinkContext* _pLinkCtx, int _iNode, const CBox3Dfp32& _Box);
	void LinkBoxWithPVS_r(CBSP2_LinkContext* _pLinkCtx, int _iNode, const CBox3Dfp32& _Box, const uint8* _pPVS);
	void LinkBoxWithPVS_i(CBSP2_LinkContext* _pLinkCtx, int _iNode, const CBox3Dfp32& _Box, const uint8* _pPVS);

	TPtr<CXR_Model_BSP2> m_spMaster;						// Pointer to master-model, if any.
														// The master-model is the world. Entities share
														// data with the master-model in order to save memory.
	uint16 m_iSubModel;
	uint16 m_nSubModels;

	mutable TPtr<TObjectPool<CBSP2_EnumContext> >	m_spEnumContextPool;
	// Settings
	static int ms_Enable;

	static CRC_Attributes m_RenderZBuffer;
	static CRC_Attributes m_RenderZBufferCullCW;

	static fp32 ms_lDiameterClasses[MODEL_BSP_MAXDIAMETERCLASSES];

	int16 m_nStructureLeaves;

	spCXR_IndexedSolidContainer32 m_spIndexedSolids;

	// Const.
	TThinArray<CPlane3Dfp32> m_lPlanes;
	TThinArrayAlign<CVec3Dfp32,16> m_lVertices;
	TArrayAlign<uint32,16> m_liVertices;
	TThinArrayAlign<CBSP2_Face,16> m_lFaces;
#ifdef M_Profile
	TThinArray<uint8> m_lFaceLights;
#endif
	TThinArray<uint32> m_liFaces;
	TThinArray<CBSP2_Node> m_lNodes;
	TThinArray<CBSP2_PortalLeafExt> m_lPortalLeaves;
	TThinArray<CBSP2_PortalLeaf_VBFP> m_lPortalLeaves_VBFP;
	TThinArray<CBSP2_PortalExt> m_lPortals;
	TThinArray<CBSP2_FogPortalFace> m_lFogPortalFaces;
	TThinArray<CBSP2_FogPortal> m_lFogPortals;
	TThinArray<uint8> m_lPVS;

	TThinArrayAlign<CBSP_Edge,16> m_lEdges;
	TThinArrayAlign<CBSP_EdgeFaces,16> m_lEdgeFaces;
	TThinArrayAlign<uint32,16> m_liEdges;

	TThinArray<uint16> m_liPortals;
	TThinArray<uint32> m_liPLFaces;

	TThinArray<CXR_PlaneMapping> m_lMappings;
	TArray<spCXW_Surface> m_lspSurfaces;
	TArray<CXR_MediumDesc> m_lMediums;

	// Generated data
	TArray<CBSP2_VBInfo> m_lVBInfo;					// m_lVBInfo and m_lspVB are parallel arrays.
	TArray<spCBSP2_VertexBuffer> m_lspVB;

	CVec3Dfp32 m_MasterVBScale;
	TArray<CVec3Dfp32> m_lVBOffsets;

	TThinArray<CBSP2_LightVBInfo> m_lLightVBInfo;
	TArray<CBSP2_SurfaceQueue> m_lSurfQueues;
	TArray<CXR_SurfaceShaderParams> m_lSurfaceShaderParams;

	TThinArray<CBSP2_VBFacePrim> m_lVBFacePrim;
	TThinArray<uint16> m_lFacePrim;

	// -------------------------------------------------------------------
	// Light

		// Unified lighting data
		spCBSP2_LightData_Ext m_spLightData;

		// LightMaps
		TArray<CBSP2_LightMapInfo> m_lLightMapInfo;
		TPtr<CBSP2_LightMapContainer> m_spLightMapContainer;

		spCTextureContainer m_spLMTC;
		TArray<CPnt> m_lLMDimensions;
		TArray<uint16> m_lLMTextureIDs;

		// Smoothing normals
		TThinArray<uint32> m_liLightNormals;

		// Light field octtree
		TPtr<CBSP2_LightFieldOcttree> m_spLightFieldOcttree;

	// -------------------------------------------------------------------
	// Pointers into the lists above.
	CPlane3Dfp32* m_pPlanes;
	CVec3Dfp32* m_pVertices;
	uint32* m_piVertices;
	CBSP2_Face* m_pFaces;
	uint32* m_piFaces;
	CBSP2_Node* m_pNodes;
	int8* m_pVVertMask;
	uint16* m_piVVertTag;
	CBSP2_PortalLeafExt* m_pPortalLeaves;
	CBSP2_PortalExt* m_pPortals;
	uint16* m_piPortals;

	TArray<CPixel32> m_lVertexFog;
	TArray<spCBSP2_FogBox> m_lspFogBoxes;

	// Fog vertex tagging
	TArray<int8> m_lFogTags;
	TArray<uint32> m_liFogTags;
	int m_nFogTags;
	int m_MaxFogTags;

	// View vertex tagging
	TThinArray<int8> m_lVVertMask;
	TThinArray<uint16> m_liVVertTag;
	int m_nVVertTag;
	int m_MaxVVertTag;

	// List of view-instances.
	TThinArray<spCBSP2_ViewInstance> m_lspViews;
	TThinArray<CBSP2_ViewInstanceData> m_lViewData;
	int m_iView;
	CBSP2_ViewInstance* m_pWorldView;				// read-write data (used by world)
	const CBSP2_ViewInstance* m_pView;				// read-only data (used by all models)
	const CBSP2_ViewInstanceData* m_pViewClipData;	// read-only data (used by all models)
//	int m_CurrentView;

	// Face-queue
//	TThinArray<uint32> m_lFaceQueueHeap;
//	TThinArray<CBSP2_VBFaceQueue> m_lVBFaceQueues;
	uint32 m_nFaceQueueHeapSize;
	TThinArray<CBSP2_VBFaceQueueData>	m_lVBFaceQueues;


	// Timing & statistics
#ifdef M_Profile
	mutable int m_nPortal0;
	mutable int m_nPortal1;
	mutable int m_nPortal2;
	mutable int m_nPortal3;

	mutable int m_nFacesChecked;
	mutable int m_nFacesFront;
	mutable int m_nFacesVisible;
	mutable int m_nSurfSwitch;

	mutable CMTime m_TimeSurf;
	mutable CMTime m_TimeBrushVB;
	mutable CMTime m_TimeCull;
	mutable CMTime m_TimeBuild;

	mutable CMTime m_Time;
	mutable CMTime m_TimePortals;
	mutable CMTime m_TimePortalAnd;
	mutable CMTime m_TimePortalOr;
	mutable CMTime m_TimePortal0;
	mutable CMTime m_TimePortal1;
	mutable CMTime m_TimePortal2;
	mutable CMTime m_TimePortal3;
	mutable CMTime m_TimeRenderLeafList;
	mutable CMTime m_TimeRenderFaceQueue;
	mutable CMTime m_TimeRenderPoly1;
	mutable CMTime m_TimeRenderPoly2;
#endif

	TThinArray<uint32> m_liRefFaces;		// Faces with reflections or portals.

	CBox3Dfp32		m_OctaAABBBox;
	CBSP_OctaAABBList m_lOctaAABBNodes;
	CMat4Dfp32		m_OctaAABBToU16;
	CMat4Dfp32		m_OctaAABBFromU16;
	vec128			m_OctaAABBScale;
	vec128			m_OctaAABBInvScale;
	vec128			m_OctaAABBTranslate;

protected:
	fp32 m_BoundRadius;
	CBox3Dfp32 m_BoundBox;

	void ExpandFaceBoundBox_r(int _iNode, CBox3Dfp32& _Box);
	int GetFaceList_r(int _iNode, uint32* _pDest, int _Pos, int _MaxFaces, int _iSurf = -1, int _SurfFlags = -1);
	int GetPLFaceList_r(int _iPL, uint32* _pDest, int _MaxFaces, int _iSurf = -1, int _SurfFlags = -1);
public:
	int CompareFaces(const CBSP2_Face* _pF1, const CBSP2_Face* _pF2);
	int CompareFaces(int _iFace1, int _iFace2);
	int CompareFaces_Index(int _iFace1, int _iFace2);
protected:
	void SortFaceList(uint32* _pSrc, uint32* _pDest, int _nF);
	void VBSortFaceList(uint32* _pSrc, uint32* _pDest, int _nF);

	void CountVertexBuffer(uint32* _piFaces, int _nFaces, int& _nVB);
	void PrepareVertexBuffer(uint32* _piFaces, int _nFaces, int& _iVB);
	void CreateVertexBuffer(int _iVB);

	CBSP2_VertexBuffer* GetVertexBuffer(int _iVB);
	void CreatePhysVertIndices();
	void CreateFaceDiameters();
	void CreateLightDataSVArrays(CBSP2_LightData_Ext* _pLD);
	void SortShaderQueues(CBSP2_LightData_Ext* _pLD);

	void CountVBFP(uint32* _piFaces, int _nFaces, int& _nVBFP, int& _nFacePrim);
	void CreateVBFP(uint32* _piFaces, int _nFaces, int& _iVBFP, int& _iFacePrim);

	void SplitSLCOnVBIndex();
	void CountSLCVB();
	void PrepareSLCVB();

	void LightDataSanityCheck();
	void LightDataFixup();

	void GetFaceList_DebugSlow_r(uint _iNode, TArray<uint>& _liFaces);
	void PortalLeafSanityCheck(const char* _pLocation);

	void CreateSurfaceShaderParams();
	
	void InitBound();
	void InitPortalBounds();
	void InitNodePlaneTypes();

	// -------------------------------------------------------------------
	// Face enumeration.

	void EnumFaces_Sphere_r(CXR_PhysicsContext* _pPhysContext, CBSP2_EnumContext* _pEnumContext, int _iNode) const;
	void EnumFaces_Box_r(CXR_PhysicsContext* _pPhysContext, CBSP2_EnumContext* _pEnumContext, int _iNode) const;
	void EnumFaces_All_r(CXR_PhysicsContext* _pPhysContext, CBSP2_EnumContext* _pEnumContext, int _iNode) const;

	int EnumFaces_Sphere(CXR_PhysicsContext* _pPhysContext, CBSP2_EnumContext* _pEnumContext, int _iNode) const;
	int EnumFaces_Box(CXR_PhysicsContext* _pPhysContext, CBSP2_EnumContext* _pEnumContext, int _iNode) const;
	int EnumFaces_All(CXR_PhysicsContext* _pPhysContext, CBSP2_EnumContext* _pEnumContext, int _iNode) const;

	// -------------------------------------------------------------------
	int CountFaces_r(int _iNode);
	void InitFaces();

	// -------------------------------------------------------------------
	//  Fog

	void Fog_InitFogBox(int _iPL);
	void Fog_AddLightVolume(CBSP2_FogBox* _pBox, int _iLightVolume);
	void Fog_BuildFogBox(int _iPL);

	void Fog_RenderPortal(CBSP2_RenderParams* _pRenderParams, int _iFirstNode, int _iPL, int _ip, int _iStartNode);
	void Fog_RenderPortal_HW(CBSP2_RenderParams* _pRenderParams, int _iFirstNode, int _iPL, int _ip, int _iStartNode);
	int Fog_GetVisiblePortals(CBSP2_RenderParams* _pRenderParams, int _iPL, uint32* _piPortals, int _MaxPortals);
	void Fog_RenderPortals(int _iPL, uint32* _piPortals, int _nP);
	bool Fog_SetVertices(CBSP2_RenderParams* _pRenderParams, CXR_VertexBuffer* pVB, const uint32* _piFaces, int _nFaces, const uint32* _piFaceVerts, int _iPLVBase);
	bool Fog_SetAttrib(CBSP2_RenderParams* _pRenderParams, CRC_Attributes* _pVB, CBSP2_PortalLeafExt* _pPL, const CVec3Dfp32& _LocalEye);

	CPixel32 Fog_EvalFogBox(int _iPL, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MaxAlpha);
	CPixel32 Fog_TracePortalLeaf(int _iFirstNode, int _iDestPL, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MaxAlpha);
	CPixel32 Fog_TraceLine_FogBox(int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MaxAlpha);
	CPixel32 Fog_TraceLine_Lit(int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MaxAlpha);
	CPixel32 Fog_TraceLine(int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MaxAlpha);

	void Fog_TraceVertices(const uint32* _piFaces, int _nFaces, int _iPL);
	void Fog_TraceVertices(const uint32* _piFaces, int _nFaces);

	void Fog_EnableFogPL_r(int _iNode, const CBox3Dfp32& _Box);
	void Fog_DisableFogPL_r(int _iNode, const CBox3Dfp32& _Box);

	// -------------------------------------------------------------------
	// Overrides from CXR_FogInterface
	virtual CXR_FogInterface* Fog_GetInterface() { return this; };
	virtual void Fog_Trace(int _hAccelerator, const CVec3Dfp32& _POV, const CVec3Dfp32* _pV, int _nV, CPixel32* _pFog);
	virtual int Fog_InitTraceBound(const CBox3Dfp32& _Box);	// Returns a hAccelerator, 0 => No fog.
	virtual void Fog_ReleaseTraceBound(int _hAccelerator);

	// Debug rendering.
	void RenderPortalFence(CBSP2_RenderParams* _pRenderParams, int _iPortal, CPixel32 _Color);
	void RenderRPortal(CBSP2_RenderParams* _pRenderParams, const CRC_ClipVolume* _pPortal, CPixel32 _Color);
	void RenderNodePortals(CBSP2_RenderParams* _pRenderParams, int _iNode);

	void GetFaceBoundBox(int _iFace, CBox3Dfp32& _Box);
	void GetFaceListBoundBox(const uint32* _piFaces, int _nFaces, CBox3Dfp32& _Box);

	// Override from CXR_PortalSurfaceRender
	virtual void RenderPortalSurface(CXR_Engine* _pEngine, void* _pSurfContext, const CVec3Dfp32* _pVertices, int _nVertices, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);

	void RenderGetPrimCount(const uint32* _piFaces, int _nFaces, int& _nV, int& _nPrim);
	void RenderGetPrimCount_Triangles(const uint32* _piFaces, int _nFaces, int& _nV, int& _nPrim);
	void RenderGetPrimCount_KnitStrip(const uint32* _piFaces, int _nFaces, int& _nV, int& _nPrim);
	void Tesselate(const uint32* _piFaces, int _nFaces, int _nV, CVec3Dfp32* _pV, CVec2Dfp32* _pTV1, CVec2Dfp32* _pTV2, CPixel32* _pCol, CVec3Dfp32* _pN, CVec3Dfp32* _pTangU, CVec3Dfp32* _pTangV, fp32* _pLMScale, uint32* _piFaceVertices, CXW_Surface* _pSurf);
	int RenderTesselatePrim(const uint32* _piFaces, int _nFaces, uint16* _pPrim);
	int RenderTesselatePrim_Triangles(const uint32* _piFaces, int _nFaces, uint16* _pPrim);
	int RenderTesselatePrim_KnitStrip(const uint32* _piFaces, int _nFaces, uint16* _pPrim);
	bool RenderTesselateVBPrim(const uint32* _piFaces, int _nFaces, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB);
	bool RenderTesselate(const uint32* _piFaces, int _nFaces, int _TessFlags, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, uint32* _piFaceVertices);

	void VB_CopyGeometry(CBSP2_RenderParams* _pRenderParams, int _iVB, CBSP2_VBChain* _pVB);

	void VB_RenderFaces(CBSP2_RenderParams* _pRenderParams, int _iVB, const uint32* _piFaces, int _nFaces, CXW_Surface* _pSurf, bool _bAddLight);
	void VB_RenderFaces(int _iVB, const uint32* _piFaces, int _nFaces, CXW_Surface* _pSurf);

	void VB_ClearFaceQueues(CBSP2_RenderParams* _pRenderParams);
	void VB_RenderFaceQueues(CBSP2_RenderParams* _pRenderParams);

	void VB_ClearQueues(CBSP2_RenderParams* _pRenderParams);
	void VB_RenderQueues(CBSP2_RenderParams* _pRenderParams);

	void VB_Flush(CBSP2_RenderParams* _pRenderParams);
	void VB_AddPrimitives(CBSP2_RenderParams* _pRenderParams, int _iVB, const uint16* _pPrim, int _nPrim);
	void VB_AddPrimitives_LocalVBM(CBSP2_RenderParams* _pRenderParams, int _iVB, const uint16* _pPrim, int _nPrim);
	void VB_Tesselate(CBSP2_RenderParams* _pRenderParams, int _iVB, const uint32* _piFaces, int _nFaces);

	void VB_AllocVBID();
	void VB_FreeVBID();

	// -------------------------------------------------------------------
	// CXR_VBContainer overrides
	CFStr GetName(int _iLocal);
	virtual int GetNumLocal();
	virtual int GetID(int _iLocal);
	virtual void Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags);
	virtual void Release(int _iLocal);

	// -------------------------------------------------------------------
//	void RenderSurfaceCluster(CBSP2_VBSurfaceCluster* _pCluster, int _iVB, CBSP2_PortalLeafExt* _pPL = NULL);

	void RenderNHF(CBSP2_RenderParams* _pRenderParams, const uint32* _piFaces, int _nFaces, int _iPL);
	void VB_RenderNHF(CBSP2_RenderParams* _pRenderParams, int _iVB, const uint16* _piPrim, int _nPrim, CBSP2_PortalLeafExt* _pPL);

	bool PrepareShaderQueue(CBSP2_RenderParams* _pRenderParams);
	void RenderShaderQueue(CBSP2_RenderParams* _pRenderParams);
	void Render_SLC(CBSP2_RenderParams*M_RESTRICT _pRenderParams, int _iPL, const CRC_ClipVolume* _pClip, int _iSLC, int _nSLC);
	void Render_SLC2(CBSP2_RenderParams*M_RESTRICT _pRenderParams, int _iPL, const CRC_ClipVolume* _pClip, int _iSLC, int _nSLC);
	void RenderPortalLeaf(CBSP2_RenderParams*M_RESTRICT _pRenderParams, int _iPL);
	void RenderLeafList(CBSP2_RenderParams* _pRenderParams);

	bool VB_AllocateFaceQueues(CBSP2_RenderParams& _RenderParams);
	bool VB_AllocateSurfaceQueues(CBSP2_RenderParams& _RenderParams);
	bool VB_AllocatePrimitiveQueues(CBSP2_RenderParams& _RenderParams);

	void UntagVertices();
	void UntagFogVertices();

	void RenderWire(CBSP2_RenderParams* _pRenderParams, CPixel32 _Color);

#ifdef M_Profile
	void RenderLightColorKeyFaceList(CBSP2_RenderParams * _pRenderParams,uint32 * _pFaces,uint32 _nFaces,
		CRC_Attributes *_pAttr, int16 _iVB, CPixel32 _Color);
	void RenderLightColorKey(CBSP2_RenderParams * _pRenderParams);
	void LightCountSLC(CBSP2_RenderParams * _pRenderParams);
#endif

	// -------------------------------------------------------------------
	// BSP-Bounding functions
	int GetNumEnabledLeaves(int _iNode);
	int GetFirstSplitNode_Sphere(const CVec3Dfp32& _Pos, fp32 _Radius);
	int GetFirstSplitNode_Box(const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax);
	int GetFirstSplitNode_Polyhedron(const CVec3Dfp32* _pV, int _nVertices);
	void EnableTreeFromNode_Sphere(int _iNode, const CVec3Dfp32& _Pos, fp32 _Radius);
	void EnableTreeFromNode_Box(int _iNode, const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax);
	void EnableTreeFromNode_Polyhedron(int _iNode, const CVec3Dfp32* _pV, int _nVertices);
	void EnableTreeFromNode(int _iNode);
	void EnableTreeFlagFromNode(int _iNode, int _Flag);

	void EnableWithPortalFloodFill(int _iNode, int _EnableFlag, int _MediumFlag);
	void DisableWithPortalFloodFill(int _iNode, int _EnableFlag);

//	void InitBoundNodesFromEnabledNodes(int _iNode, int _iNodeLastSplit, int _iNodeLastSplitSide);
	void DisableTree(int _iNode);
	void DisableTreeFlag(int _iNode, int _Flag);

	bool CheckTree_r(int _iNode, int _iParent = 0, int _PortalDone = 0);

	int Structure_GetFirstSplitNode_Polyhedron(const CVec3Dfp32* _pV, int _nVertices);
	void Structure_EnableTreeFromNode_Polyhedron(int _iNode, const CVec3Dfp32* _pV, int _nVertices);
//	void Structure_InitBoundNodesFromEnabledNodes(int _iNode, int _iNodeLastSplit, int _iNodeLastSplitSide);

	// -------------------------------------------------------------------
	// Portal stuff
	int Portal_And(const CRC_ClipVolume* _pPortal, const CBSP2_PortalExt* _pP, bool _bFrontThis, CRC_ClipVolume* _pPDest, CXR_FogState* _pSceneFog) const;
	void Portal_Or(CRC_ClipVolume* _pP1, const CRC_ClipVolume* _pP2, CBSP2_View_Params* _pViewParam) const;
	void Portal_AddNode(CBSP2_View_Params* _pViewParams, int _iNode, int _iClipRPortal);

	void Portal_OpenClipFront_r(CBSP2_View_Params* _pViewParams, const CBSP2_Node* pNode, const CVec3Dfp32* _pVPortal, int _nVPortal);
	void Portal_OpenClipBack_r(CBSP2_View_Params* _pViewParams, const CBSP2_Node* pNode, const CVec3Dfp32* _pVPortal, int _nVPortal);
	void Portal_Open_r(CBSP2_View_Params* _pViewParams, int _iNode, const CVec3Dfp32* _pVPortal, int _nVPortal);
//	int Portal_GetFirstOpen(const CVec3Dfp32& _v);
	int Portal_GetFirstOpen_r(int _iNode, const CVec3Dfp32& _v);

	// -------------------------------------------------------------------
	// Dynamic lighting
	bool Light_IntersectSphere_CheckFace(const CBSP2_Face*, const CVec3Dfp32& _Pos, fp32 _Radius);

	bool Light_RenderShading(CBSP2_RenderParams* _pRenderParams, CXR_VertexBuffer* _pVB, const CXR_Light* _pL, CXW_Surface* _pSurf);
	void Light_RenderShading_FaceQueues(CBSP2_RenderParams* _pRenderParams, const CXR_Light* _pL, const CMat4Dfp32* _pWMat, const CMat4Dfp32* _pVMat);
	CXR_VertexBuffer* Light_CreateShadowVolume(CBSP2_RenderParams* _pRenderParams, CBSP2_DynamicLightContext* _pDynLightContext, const CXR_Light& _Light, const uint32* _piFaces, int _nFaces);
	void Light_BuildShadowFaces_Point(CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, const uint32* _piFaces, int _nFaces);
	void Light_BuildShadowFaces_Point(CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, int _iNode);
	void Light_BuildShadowFaces_Spot(CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, const uint32* _piFaces, int _nFaces);
	void Light_BuildShadowFaces_Spot(CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, int _iNode);
	void Light_BuildFaceQueuesAndShadowFaces_Point(CBSP2_RenderParams* _pRenderParams, CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, const uint32* _piFaces, int _nFaces);
	void Light_BuildFaceQueuesAndShadowFaces_Point(CBSP2_RenderParams* _pRenderParams, CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, int _iNode);
	void Light_BuildFaceQueuesAndShadowFaces_Spot(CBSP2_RenderParams* _pRenderParams, CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, const uint32* _piFaces, int _nFaces);
	void Light_BuildFaceQueuesAndShadowFaces_Spot(CBSP2_RenderParams* _pRenderParams, CBSP2_EnumContext* _pEnumContext, CBSP2_DynamicLightContext* _pDynLightContext, int _iNode);
	int Light_FaceIntersectSpot(CBSP2_DynamicLightContext* _pDynLightContext, const CBSP2_Face* _pF, CBox3Dfp32& _FaceBox) const;
	int Light_FaceIntersectOmni(CBSP2_DynamicLightContext* _pDynLightContext, const CBSP2_Face* _pF, CBox3Dfp32& _FaceBox) const;
	CXR_VertexBuffer* Light_CreateFullShadowVolume(CBSP2_RenderParams* _pRenderParams, const CXR_Light& _Light);
	void Light_RenderDynamicLight(CBSP2_RenderParams* _pRenderParams, int _iDynamic, bool _bNoShading, const CMat4Dfp32* _pWMat, const CMat4Dfp32* _pVMat);

	int NumFacesInTree(int _iNode);
	int GetMedium(const CVec3Dfp32& _v0);
	int GetLeaf(const CVec3Dfp32& _v);
public:
	int GetPortalLeaf(const CVec3Dfp32& _v);	// This stupid returns a node index, not a portal-leaf index.
protected:
	int GetStructurePortalLeaf(const CVec3Dfp32& _v);
//	int GetNavigationPortalLeaf(const CVec3Dfp32& _v);

	virtual void Clear();
	void InitializeListPtrs();

public:
	CXR_Model_BSP2();
	~CXR_Model_BSP2();
	virtual void Create(const char* _pParam, CDataFile* _pDFile, CCFile* _pFile, const CBSP_CreateInfo& _CreateInfo);
	void Create_PhongMap();
	void Create_ViewInstances(int _nInst);
	void CreatePortalLeafNodeIndices_r(TAP<CBSP2_Node> _lNodes, uint _iNode);
	void CreatePortalLeafNodeIndices();
	void Create_PostRead();
	static void CleanStatic(); // Called when chaning levels.

	// Overrides
	virtual aint GetParam(int _Param);
	virtual void SetParam(int _Param, aint _Value);
	virtual int GetParamfv(int _Param, fp32* _pRetValues);
	virtual void SetParamfv(int _Param, const fp32* _pValues);

	virtual int GetModelClass() { return CXR_MODEL_CLASS_BSP2; };
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, int _Mask, const CXR_AnimState* _pAnimState = NULL);

	virtual void PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
			const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
	virtual void OnRender2(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _WVelMat, int _Flags);
	virtual void OnPrecache(CXR_Engine* _pEngine, int _iVariation);
	virtual void OnRefreshSurfaces();

	CStr GetInfo();

	// -------------------------------------------------------------------
	// Scenegraph internal functions
#ifndef XW_PVS_NOCOMPRESS
	int SceneGraph_PVSFindSlot(int _iPL);
	int SceneGraph_PVSFindFreeSlot();
#endif
	const uint8* SceneGraph_PVSLock(int _PVSType, int _iPortalLeaf);

protected:
#ifndef XW_PVS_NOCOMPRESS
	TArray<TArray<uint8> > m_lPVSCache;
	TArray<int> m_liPVSCachePL;
	TArray<int> m_lnPVSCacheRef;
#endif

public:
	// -------------------------------------------------------------------
	// Scenegraph overrides
	virtual CXR_SceneGraphInterface* SceneGraph_GetInterface() { return m_lPortalLeaves.Len() ? this : NULL; };

	virtual void SceneGraph_PVSInitCache(int _NumCached);
	virtual int SceneGraph_PVSGetLen();
	virtual bool SceneGraph_PVSGet(int _PVSType, const CVec3Dfp32& _Pos, uint8* _pDst);
	virtual const uint8* SceneGraph_PVSLock(int _PVSType, const CVec3Dfp32& _Pos);
	virtual void SceneGraph_PVSRelease(const uint8* _pPVS);
	virtual int SceneGraph_GetPVSNode(const CVec3Dfp32& _Pos);

	virtual spCXR_SceneGraphInstance SceneGraph_CreateInstance();

/*	virtual void SceneGraph_Destroy(int _hSG);
	virtual void SceneGraph_LinkElement(int _hSG, uint16 _Elem, const CBox3Dfp32& _Box, int _Flags);
	virtual void SceneGraph_LinkInfiniteElement(int _hSG, uint16 _Elem, int _Flags);
	virtual void SceneGraph_UnlinkElement(int _hSG, uint16 _Elem);

	virtual int SceneGraph_EnumerateElementNodes(int _hSG, int _Elem, uint16* _piRetNodes, int _MaxRet);

	virtual int SceneGraph_EnumerateNodes(int _hSG, const uint16* _piNodes, int _nNodes, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem = NULL, int _nMergeElem = 0);
	virtual int SceneGraph_EnumeratePVS(int _hSG, const uint8* _pPVS, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem = NULL, int _nMergeElem = 0);
	virtual int SceneGraph_EnumerateView(int _hSG, int _iView, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem = NULL, int _nMergeElem = 0);

	virtual int SceneGraph_Light_GetFreeDynamic();
	virtual void SceneGraph_Light_LinkDynamic(int _iLight, const CXR_Light& _Light);
	virtual void SceneGraph_Light_Unlink(int _iLight);

	virtual void SceneGraph_Light_SetIntensity(int _iLight, const CVec3Dfp32& _Intensity, bool _bIsOff);
	virtual void SceneGraph_Light_SetRotation(int _iLight, const CMat4Dfp32& _Rotation);
	virtual void SceneGraph_Light_SetPosition(int _iLight, const CMat4Dfp32& _Pos);
*/
protected:
	// -------------------------------------------------------------------
	// Physics internal functions

	void __Phys_RenderFace(int _iFace, const CMat4Dfp32& _WMat, CWireContainer* _pWC, int _Col) const;

	static bool __Phys_IntersectSphere_Polygon(const CVec3Dfp32* _pV, const uint32* _piV, int _nV,
		const CPlane3Dfp32& _Plane, const CVec3Dfp32& _Pos, fp32 _Radius, CCollisionInfo* M_RESTRICT _pCollisionInfo);

	bool __Phys_IntersectSphere_CheckFace(int _iFace, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, CCollisionInfo* M_RESTRICT _pCollisionInfo) const;
	bool __Phys_IntersectSphere_r(CXR_PhysicsContext* M_RESTRICT _pPhysContext, int _iNode, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* M_RESTRICT _pCollisionInfo) const;

	bool __Phys_IntersectSphere_Triangle(const CVec3Dfp32& _V0, const CVec3Dfp32& _V1, const CVec3Dfp32& _V2, const CVec3Dfp32& _Pos, fp32 _Radius, CCollisionInfo* M_RESTRICT _pCollisionInfo) const;

	bool __Phys_IntersectOBB_CheckFace(int _iFace, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, CCollisionInfo* M_RESTRICT _pCollisionInfo) const;
	bool __Phys_IntersectOBB_r(CXR_PhysicsContext* M_RESTRICT _pPhysContext, int _iNode, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, const CBox3Dfp32 _Bound, int _MediumFlags, CCollisionInfo* M_RESTRICT _pCollisionInfo) const;
	bool __Phys_IntersectOBB_i(CXR_PhysicsContext* M_RESTRICT _pPhysContext, int _iNode, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, const CBox3Dfp32& _Bound, int _MediumFlags, CCollisionInfo* M_RESTRICT _pCollisionInfo) const;
	int __Phys_CollideOBB_i(CXR_PhysicsContext* M_RESTRICT _pPhysContext, int _iNode, const CPhysOBB& _Box, const CBox3Dfp32& _Bound, int _MediumFlags, CCollisionInfo* M_RESTRICT _pCollisionInfo, int _nMaxCollisions) const;
	int __Phys_CollideSolid_i(CXR_PhysicsContext* M_RESTRICT _pPhysContext, int _iNode, const CXR_IndexedSolidDesc32& _SolidDescr, int _nVertices, const CMat4Dfp32& _BSP2Transform, const CVec3Dfp32& _Offset, const CBox3Dfp32& _Bound, int _MediumFlags, CCollisionInfo* M_RESTRICT _pCollisionInfo, int _nMaxCollisions) const;

	bool __Phys_TraceLine_r(CXR_PhysicsContext* M_RESTRICT _pPhysContext, int _iNode, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, class CPhysLineContext* M_RESTRICT _pLineContext, CCollisionInfo* M_RESTRICT _pCollisionInfo) const;
	bool __Phys_TraceLine_r(CXR_PhysicsContext* M_RESTRICT _pPhysContext, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, class CPhysLineContext* M_RESTRICT _pLineContext) const;

	int __Phys_GetCombinedMediumFlags_r(CXR_PhysicsContext* M_RESTRICT _pPhysContext, int _iNode, const CBox3Dfp32* M_RESTRICT _pBox) const;
	int __Phys_GetCombinedMediumFlags_i(CXR_PhysicsContext* M_RESTRICT _pPhysContext, int _iNode, const CBox3Dfp32* M_RESTRICT _pBox) const;
	
	void CollectPCS_r(CXR_PhysicsContext* M_RESTRICT _pPhysContext, CBSP2_EnumContext* M_RESTRICT _pEnumContext, int _iNode) const;
	void CollectPCS_IsLeaf(CXR_PhysicsContext* M_RESTRICT _pPhysContext, CBSP2_EnumContext* M_RESTRICT _pEnumContext, const CBSP2_Node* M_RESTRICT _pNode) const;

public:
	// -------------------------------------------------------------------
	// Physics overrides
	virtual void Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _v0, fp32& _Radius);
	virtual void Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _Box);

	virtual void Phys_Init(CXR_PhysicsContext* _pPhysContext);
	virtual bool Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);

	virtual int Phys_CollideBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _Box, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions);
	virtual int Phys_CollideBSP2(CXR_PhysicsContext* _pPhysContext, class CXR_IndexedSolidContainer32 *_pSolidContainer, const CMat4Dfp32& _BSP2Transform, const CVec3Dfp32& _Offset, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _nMaxCollisions);

	virtual void CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet* _pcs, const int _iObj, const int _MediumFlags );

	virtual int Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0);
	virtual void Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium);
	virtual int Phys_GetCombinedMediumFlags(CXR_PhysicsContext* _pPhysContext, const CBox3Dfp32& _Box);

	// -------------------------------------------------------------------
	// Visibility internal functions
private:
	void View_CalcScissors(CBSP2_View_Params* _pViewParams);
	void View_InitLightOcclusion();
	void View_InitPLPriorities(CBSP2_RenderParams* _pRenderParams);

	bool View_GetClip_Sphere_r(int _iNode, CBSP2_View_Params* _pParams) const;

	void View_GetScissorWithPVS_Box_i(int _iNode, const CBox3Dfp32& _VisBox, const uint8* _pPVS, CScissorRect* _pScissor) const;
	void View_GetVisibleBoxWithPVS_Box_r(int _iNode, const CBox3Dfp32* _pBox, const uint8* _pPVS, CBox3Dfp32* _pDestBox) const;


	bool View_GetClip_Box_StructureLeaf(int _iNode, CBSP2_View_Params* _pParams) const;
	bool View_GetClip_Box_i(int _iNode, CBSP2_View_Params* _pParams) const;
//	bool View_GetClip_Box_r(int _iNode, CBSP2_View_Params* _pParams) const;

	MRTC_SpinLock m_OcclusionLock;
	CXR_LightOcclusionInfo* View_Light_GetOcclusionInt(int _iLight);					// Make sure you know what you're doing when you call this function, it is not threadsafe
	virtual const uint16* View_Light_GetVisible(int& _nRetLights);
	virtual CXR_LightOcclusionInfo* View_Light_GetOcclusionArray(int& _nLights);		// Make sure you know what you're doing when you call this function, it is not threadsafe
	virtual CXR_LightOcclusionInfo* View_Light_GetOcclusion(int _iLight);				// Make sure you know what you're doing when you call this function, it is not threadsafe
	virtual void View_Light_ApplyOcclusionArray(int _nLights, const uint16* _piLights, const CXR_LightOcclusionInfo* _pLO);
	virtual void View_Light_ApplyOcclusionArray_ShadowShaded(uint _nLights, const uint16* _piLights, const CScissorRect* _pScissors);
	virtual int View_Light_GetOcclusionSize();

	// -------------------------------------------------------------------
	// Visibility overrides
public:
	virtual void View_Reset(int _iView);
	virtual void View_SetState(int _iView, int _State, int _Value);

	virtual void View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);
	virtual void View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CVec3Dfp32* _pVPortal, int _nVPortal,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);

	void View_InitForLight(int _iView, const CVec3Dfp32& _Pos, CVec3Dfp32* _pVPortal, int _nVPortal, const CXR_Light* _pLight);

	virtual void View_SetCurrent(int _iView, CXR_SceneGraphInstance* _pSceneGraphInstance);
	virtual bool View_GetClip_Sphere(CVec3Dfp32 _v0, fp32 _Radius, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo);
	virtual bool View_GetClip_Box(CVec3Dfp32 _min, CVec3Dfp32 _max, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo);
	virtual void View_GetClip(int _Elem, CXR_RenderInfo* _pRenderInfo);

	virtual CXR_ViewClipInterface* View_GetInterface() { return this; };

	// Considering this method for CXR_ViewClipInterface
	bool View_GetVisibleBoxWithPVS_Box(const CBox3Dfp32& _Box, const uint8* _pPVS, CBox3Dfp32& _VisBox);

//	virtual void View_SetPortalState(int _iPortal, int State);

	// -------------------------------------------------------------------
protected:
	NThread::CSpinLock m_WallmarkLock;
	TPtr<CBSP2_WallmarkContext> m_spWMCTemp;
	TPtr<CBSP2_WallmarkContext> m_spWMC;
	TPtr<CBSP2_WallmarkContext> m_spWMCStatic;

	virtual void Wallmark_Render(CBSP2_RenderParams* _pRenderParams, CBSP2_WallmarkContext* _pWMC, const CBSP2_Wallmark* _pWM, 
		CMTime _Time, CXR_VertexBuffer * _pVB,
		CVec3Dfp32 * _pV,CVec3Dfp32 * _pN, CVec2Dfp32 * _pTV1, CVec2Dfp32 * _pTV2,
		CVec3Dfp32 * _pTanU,CVec3Dfp32 * _pTanV, CPixel32 * _pCol,uint16 * _piV);
	virtual void Wallmark_Render(CBSP2_RenderParams* _pRenderParams, CBSP2_WallmarkContext* _pWMC, int _iPL);
	bool Wallmark_Create(CBSP2_WallmarkContext* _pWMC, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _iPL, const CVec3Dfp32* _pV, int _nV, int _GUID, const CXR_PlaneMapping& _Mapping);
	virtual int Wallmark_Create(CBSP2_WallmarkContext* _pWMC, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material);

	// -------------------------------------------------------------------
	// Wallmark overrides
public:
	virtual CXR_WallmarkInterface* Wallmark_GetInterface();

	virtual int Wallmark_CreateContext(const CXR_WallmarkContextCreateInfo& _CreateInfo);
	virtual void Wallmark_DestroyContext(int _hContext);
	virtual int Wallmark_Create(int _hContext, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material = 0);
	virtual bool Wallmark_Destroy(int _GUID);

	virtual int& GetGlobalEnable() { return ms_Enable; };
};

typedef TPtr<CXR_Model_BSP2> spCXR_Model_BSP2;

///////////////////////////////////////////////////////////////////////////////
class CBSP2_BoxInterleaved
{
public:
	fp32 m_MinX, m_MaxX;
	fp32 m_MinY, m_MaxY;
	fp32 m_MinZ, m_MaxZ;

	CBSP2_BoxInterleaved() {}

	explicit CBSP2_BoxInterleaved(const CBox3Dfp32& _Box)
	{
		Init(_Box);
	}

	void Init(const CBox3Dfp32& _Box)
	{
		m_MinX = _Box.m_Min.k[0];
		m_MinY = _Box.m_Min.k[1];
		m_MinZ = _Box.m_Min.k[2];
		m_MaxX = _Box.m_Max.k[0];
		m_MaxY = _Box.m_Max.k[1];
		m_MaxZ = _Box.m_Max.k[2];
	}
};

static M_INLINE void GetBoxMinMaxDistance(const CPlane3Dfp32& _Plane, 
                                   const CBSP2_BoxInterleaved& _Box, 
                                   fp32& _MinDistance, fp32& _MaxDistance)
{
	register const uint32* pPlaneSign = (uint32*)&_Plane;
	register const fp32* pPlane = (const fp32*)&_Plane;
	register const fp32* pBox = (const fp32*)&_Box;

	register uint32 ix, iy, iz;
	register fp32 xMin, xMax, yMin, yMax, zMin, zMax;

	ix = (pPlaneSign[0]>>31); // check sign of normal.x
	xMin = pBox[ix] * pPlane[0];
	xMax = pBox[1-ix] * pPlane[0];	//1-0=1

	iy = 2+(pPlaneSign[1]>>31); // check sign of normal.y
	yMin = pBox[iy] * pPlane[1];
	yMax = pBox[5-iy] * pPlane[1];	//5-2=3

	iz = 4+(pPlaneSign[2]>>31); // check sign of normal.z
	zMin = pBox[iz] * pPlane[2];
	zMax = pBox[9-iz] * pPlane[2];	//9-4=5

	_MinDistance = xMin + yMin + zMin + pPlane[3];
	_MaxDistance = xMax + yMax + zMax + pPlane[3];
}

///////////////////////////////////////////////////////////////////////////////

#endif // _INC_WBSP2MODEL

