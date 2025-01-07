#ifndef _INC_WBSPMODEL
#define _INC_WBSPMODEL

#include "XWCommon.h"
#include "../../XR/XR.h"
#include "../../XR/Solids/MCSolid.h"
#include "../../XR/XRVBContainer.h"

class CXR_VBManager;
class CXR_VertexBuffer;
struct CBSP_BoxInterleaved;

#define MODEL_BSP_EPSILON 0.001f


#define MODEL_BSP_MAXVIEWS			8

#define MODEL_BSP_MAXFACEVERTICES	32

#define MODEL_BSP_MAXDYNAMICLIGHTS 32	// Limited by 32-bit mask used as per-face dynamic light status.

#define MODEL_BSP_SPECBUMP
//#define MODEL_BSP_SPECBUMPNUM 5

//#define MODEL_BSP_LITFOG


#define XW_LIGHT_EXT_SHADOWS		1
#define XW_LIGHT_EXT_DYNSHADOWS		2
#define XW_LIGHT_INT_SHADOWS		4
#define XW_LIGHT_INT_DYNSHADOWS		8

#define SBELEM_MAXBRUSHES 4096
#define SBELEM_MAXFACES 16

#define SBELEM_FACE(iElem) ((iElem >> 12) & 0xf)
#define SBELEM_BRUSH(iElem) (iElem & 0xfff)
#define SBELEM(iBrush, iFace) ((iBrush & 0xfff) + (iFace << 12))

// -------------------------------------------------------------------
int BSPModel_CutFace2(CVec3Dfp32* _pVerts, int _nv, const CPlane3Dfp32* _pPlanes, int _np, 
	int _bInvertPlanes, CVec2Dfp32* _pTVerts1 = NULL, CVec2Dfp32* _pTVerts2 = NULL, CVec2Dfp32* _pTVerts3 = NULL);

int BSPModel_CutFace3(CVec3Dfp32* _pVerts, int _nv, const CPlane3Dfp32* _pPlanes, int _np, int& _bClip);

// -------------------------------------------------------------------
class CBSP_Face : public CBSP_CoreFace
{
public:
//	uint32 m_iTVertices;	// Texture coordinate start index
//	uint32 m_iLMTVertices;	// Lightmap texture coordinate start index
	uint16 m_iiVBVertices;
//	uint16 m_nVBVertices
	int16 m_iVB;

	uint32 m_nPhysV : 8;
	uint32 m_iiPhysV : 24;

	CBSP_Face()
	{
//		m_iTVertices = 0;
//		m_iLMTVertices = 0;
		m_iiVBVertices = 0;
//		m_nVBVertices = 0;
		m_iVB = -1;
		m_nPhysV = 0;
		m_iiPhysV = 0;
	}
};

// -------------------------------------------------------------------
//  Texture container designed for lightmaps
// -------------------------------------------------------------------
class CXW_LMCluster_Ext : public CXW_LMCluster
{
public:
	int m_TextureID;

	CXW_LMCluster_Ext()
	{
		m_TextureID = 0;
	}

/*	void AddLightID(int _ID)
	{
		CXR_LMC_LightID* pIDs = m_lLightIDs.GetBasePtr();
		int nIDs = m_lLightIDs.Len();
		for(int i = 0; i < nIDs; i++)
			if (pIDs[i].m_LightID == _ID) return;
		m_lLightIDs.Add(CXR_LMC_LightID(_ID));
	}*/
};

typedef TPtr<CXW_LMCluster_Ext> spCXW_LMCluster_Ext;

// -------------------------------------------------------------------
class CXR_Model_BSP;

class CTextureContainer_LMC : public CTextureContainer
{
public:
	CXR_Model_BSP* m_pModel;

	CTextureContainer_LMC();
	~CTextureContainer_LMC();
	void Create(CXR_Model_BSP* _pModel);

	void PrepareFrame(spCXR_WorldLightState _spCurrentWLS);

	void BuildLMI(int _iLMI, int _nLMI, CImage* _pImg, CClipRect& _Clip);

	// Overrides
	virtual int GetNumLocal();
	virtual int GetTextureID(int _iLocal);
	virtual int GetTextureDesc(int _iLocal, CImage* _pTargetImg, int& _Ret_nMipmaps);
	virtual void GetTextureProperties(int _iLocal, CTC_TextureProperties&);
	virtual void BuildInto(int _iLocal, CImage** _ppImg, int _nMipmaps, int _ConvertType = IMAGE_CONVERT_RGB, int _iStartMip = 0, uint32 _BuildFlags = 0);
};

typedef TPtr<CTextureContainer_LMC> spCTextureContainer_LMC;

// -------------------------------------------------------------------
#define RPORTAL_MAXPLANES 16

// -------------------------------------------------------------------
//  Link-instances
// -------------------------------------------------------------------
class CBSP_Link
{
public:
	uint16 m_iPortalLeaf;
	uint16 m_ID;
	uint16 m_iLinkNextPL;
	uint16 m_iLinkPrevPL;
	uint16 m_iLinkNextObject;
	uint16 m_iLinkPrevObject;

	CBSP_Link()
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
class CBSPModel_ViewInstance;

class CBSP_LinkContext : public CReferenceCount
{
	TThinArray<CBSP_Link> m_lLinks;
	CBSP_Link* m_pLinks;
	CIDHeap m_LinkHeap;

	TThinArray<uint16> m_lIDLinkMap;		// Connects to next/prev-PL chain.
	TThinArray<uint16> m_lPLLinkMap;		// Portal-leaf link-map, connects to next/prev-object chain
	TThinArray<CBox3Dfp32> m_lIDLinkBox;		// Box that was used to link an ID, used for culling when enumerating visibles.
	TThinArray<uint8> m_lIDFlags;

	uint16* m_pIDLinkMap;
	uint16* m_pPLLinkMap;

	int m_iPLInfinite;						// PL-index where 'infinite-elements' are linked.

	CXR_Model_BSP* m_pModel;

	int m_iNextLink;
	int m_iPrevLink;
	int m_CurLinkID;

	void AddPortalLeaf(int _iNode);

public:

	DECLARE_OPERATOR_NEW


	CBSP_LinkContext();
	void Create(CXR_Model_BSP* _pModel, int _NumIDs, int _NumLinks);
	void Insert(int _ID, const CBox3Dfp32& _Box, int _Flags);
	void Insert(int _ID, int _iPL);
	void InsertInfinite(int _ID, int _Flags);
	void Remove(int _ID);

protected:
	int MergeIDs(uint16* _piRet, int _nRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge);
	int EnumPortalLeaf(int _iPL, uint16* _piRet, int _MaxRet);
	int EnumPortalLeafWithClip(int _iPL, uint16* _piRet, int _MaxRet, const CRC_ClipVolume* _pClip);
	int EnumBox_r(int _iNode, const CBox3Dfp32& _Box, uint16* _piRet, int _MaxRet);
public:
	int EnumBox(const CBox3Dfp32& _Box, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge);
	int EnumVisible(CBSPModel_ViewInstance* _pView, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge);
	int EnumPVS(const uint8* _pPVS, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem,  int _nMerge);
	int EnumIDs(int _iPL, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge);

	int EnumPortalLeaves(int _ID, uint16* _piRet, int _MaxRet);
	int EnumPortalLeaves_Single(int _ID);

	friend class CXR_Model_BSP;
};

typedef TPtr<CBSP_LinkContext> spCBSP_LinkContext;

// -------------------------------------------------------------------
class CBSP_SceneGraph : public CXR_SceneGraphInstance
{
public:
	class CXR_Model_BSP* m_pModel;
	CBSP_LinkContext m_Objects;

	CBSP_SceneGraph(class CXR_Model_BSP* _pModel) : m_pModel(_pModel) {};
	void Create(int _ElemHeap, int _LightHeap, int _CreateFlags);

	virtual void SceneGraph_LinkElement(uint16 _Elem, const CBox3Dfp32& _Box, int _Flags);
	virtual void SceneGraph_LinkInfiniteElement(uint16 _Elem, int _Flags);
	virtual void SceneGraph_UnlinkElement(uint16 _Elem);
	virtual int SceneGraph_EnumerateElementNodes(int _Elem, uint32* _piRetNodes, int _MaxRet);

	virtual int SceneGraph_EnumerateNodes(const uint32* _piNodes, int _nNodes, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem = NULL, int _nMergeElem = 0);
	virtual int SceneGraph_EnumeratePVS(const uint8* _pPVS, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem = NULL, int _nMergeElem = 0);
	virtual int SceneGraph_EnumerateView(int _iView, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem = NULL, int _nMergeElem = 0);

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

	virtual int SceneGraph_Light_Enum(const CBox3Dfp32& _Box, const CXR_Light** _lpLights, int _nMaxLights);
	virtual int SceneGraph_Light_Enum(const uint16* _piPL, int _nPL, const CBox3Dfp32& _Box, const CXR_Light** _lpLights, int _nMaxLights);
};

typedef TPtr<CBSP_SceneGraph> spCBSP_SceneGraph;

// -------------------------------------------------------------------
//  View-instances
// -------------------------------------------------------------------
class CBSPModel_ViewInstance : public CReferenceCount
{
public:

	DECLARE_OPERATOR_NEW


	// Data used during rendering of a specific view.

	TThinArray<uint8> m_lPortalIDStates;		// Bit-array

	// RPortals
	TThinArray<uint16> m_liLeafRPortals;		// Index to RPortal for each leaf.
	TThinArray<uint16> m_liRPortalNext;		// Index to next RPortal for each m_lRPortals.
	TArray<CRC_ClipVolume> m_lRPortals;	// RPortals
	uint16* m_piRPortalNext;					// Ptr to m_liRPortalNext
	CRC_ClipVolume* m_pRPortals;				// Ptr to m_lRPortals
	int m_MaxRPortals;
	int m_nRPortals;							// No. of RPortals. The list has fixed length.

	// Leaf-list
	const uint8* m_pPVS;
	TThinArray<uint16> m_liVisPortalLeaves;		// List of portal-leaves in rendering order
	int m_MaxVisLeaves;
	int m_nVisLeaves;							// No. of leaves. The list has fixed length.

	int m_CurrentView;							// View#
	CRC_ClipVolume m_CurClipVolume;
	CVec3Dfp32 m_CurLocalVP;						// Viewpoint in local crd.system.
	CMat4Dfp32 m_CurVMat;						// VMat
	CMat4Dfp32 m_CurWMat;						// WMat
	CMat4Dfp32 m_CurVMatInv;						// VMat Inverse

	CPlane3Dfp32 m_LocalBackPlane;				// Backplane in model-space
	CPlane3Dfp32 m_LocalFrontPlane;				// Frontplane in model-space
	fp32 m_BackPlaneInv;							// Backplane distance reciprocal.
	fp32 m_ViewBackPlane;						// Viewport backplane.

//	CXR_SceneGraphInstance* m_pSceneGraphInstance;

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

	CBSPModel_ViewInstance()
	{
//		m_pSceneGraphInstance = NULL;
		Clear();
	}
};

typedef TPtr<CBSPModel_ViewInstance> spCBSPModel_ViewInstance;

// -------------------------------------------------------------------
class CXR_FogState;

// -------------------------------------------------------------------
class CBSP_PortalExt : public CBSP_Portal
{
public:
	CVec3Dfp32 m_BoxMin;
	CVec3Dfp32 m_BoxMax;
};

class CBSP_PortalLeafExt : public CBSP_PortalLeaf
{
public:
	CBox3Dfp32 m_FaceBoundBox;
	uint16 m_nFaces;			// Index and count into m_liPLFaces
	uint16 m_nRefFaces;			// Index and count into m_liPLFaces
	uint16 m_nSkyFaces;			// Index and count into m_liPLFaces
	uint16 m_nVBFacePrims;
	uint32 m_iiFaces;
	uint32 m_iiRefFaces;
	uint32 m_iiSkyFaces;
	uint32 m_iVBFacePrims;
	uint32 m_iBaseVertex;
	uint32 m_nVertices;
	uint32 m_iPathNode;
	uint32 m_iFogBox;

	uint32 m_DynLightMask;
	fp32 m_BasePriority;			// Vertexbuffer base priority.

	CBSP_PortalLeafExt()
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
		m_iVBFacePrims = 0;
		m_nVBFacePrims = 0;

		m_DynLightMask = 0;
		m_BasePriority = 0;
	}
};

class CBSP_VBFacePrim
{
public:
	uint16 m_iVB;
	uint32 m_iPrim;
	uint32 m_nPrim;
	CBox3Dfp32 m_BoundBox;
};

// -------------------------------------------------------------------
class CBSP_VBInfo : public CReferenceCount
{
public:
	TThinArray<uint32> m_liFaces;
	TThinArray<uint32> m_lSBFaces;
	unsigned int m_Components : 8;
	unsigned int m_bNoRelease : 1;
	unsigned int m_iVBQueue : 16;
	unsigned int m_nVertices : 16;
	unsigned int m_iSBVertices : 16;
	unsigned int m_iSurface : 16;
	int m_VBID;

	// Primitive buffer with space for all possible primitives using this buffer, allocated in VBM
	int m_MaxPrim;
	uint16* m_pPrim;
	int m_nPrim;

	CBSP_VBInfo()
	{
		m_Components = 0;
		m_bNoRelease = 0;
		m_iVBQueue = 0;
		m_VBID = -1;
		m_MaxPrim = 0;
		m_pPrim = NULL;
		m_nPrim = 0;
	}

	bool AllocPrimitives(CXR_VBManager* _pVBM)
	{
		m_nPrim = 0;
		m_pPrim = _pVBM->Alloc_Int16(m_MaxPrim);
		if (!m_pPrim) return false;
		return true;
	}

	void Clear()
	{
		m_pPrim = NULL;
		m_nPrim = 0;
	}
};

typedef TPtr<CBSP_VBInfo> spCBSP_VBInfo;

// -------------------------------------------------------------------
class CBSP_VertexBuffer : public CReferenceCount
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

	CBSP_VertexBuffer()
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
	}
};

typedef TPtr<CBSP_VertexBuffer> spCBSP_VertexBuffer;

// -------------------------------------------------------------------
/*
class CBSP_VBSurfaceCluster
{
public:
	int m_iSurface;
	int m_iiFaces;
	int m_nFaces;
	int m_iVBBaseVertex;
	int m_nVBVertices;
	int m_iLMC;
	int m_iVBQueue;
//	TArray<uint16> m_lPrim;

	CBSP_VBSurfaceCluster()
	{
		m_iSurface = 0;
		m_iiFaces = 0;
		m_nFaces = 0;
		m_iVBBaseVertex = 0;
		m_nVBVertices = 0;
		m_iLMC = -1;
		m_iVBQueue = 0;
	}
};
*/
// -------------------------------------------------------------------

class CBSP_VBChain
{
public:
	mint m_Chain : (sizeof(void *) * 8 - 1);
	mint m_bVBIds : 1;

	CBSP_VBChain()
	{
		Clear();
	}

	void Clear()
	{
		m_Chain = NULL;
		m_bVBIds = 0;
	}

	void SetToVB(CXR_VertexBuffer *_pVB)
	{
		M_ASSERT(!_pVB->m_pVBChain, "Must be so");
		if (m_bVBIds)
			_pVB->m_Flags |= CXR_VBFLAGS_VBIDCHAIN;
		else
			_pVB->m_Flags |= CXR_VBFLAGS_VBCHAIN;

		_pVB->m_pVBChain = (void *)(m_Chain << 1);
		
	}

	void AddChain(CXR_VBChain *_pChain)
	{
		if (!m_Chain)
		{
			m_bVBIds = false;			
		}
		else
		{
			M_ASSERT(!m_bVBIds, "Wrong type");
			CXR_VBChain *pLastChain = _pChain;

			while (pLastChain->m_pNextVB)
				pLastChain = pLastChain->m_pNextVB;

			pLastChain->m_pNextVB = GetVBChain();
		}
		m_Chain = ((mint)_pChain >> 1);
	}

	void AddChain(CXR_VBIDChain *_pChain)
	{
		if (!m_Chain)
		{
			m_bVBIds = true;			
		}
		else
		{
			M_ASSERT(m_bVBIds, "Wrong type");
			CXR_VBIDChain *pLastChain = _pChain;

			while (pLastChain->m_pNextVB)
				pLastChain = pLastChain->m_pNextVB;

			pLastChain->m_pNextVB = GetVBIDChain();
		}
		m_Chain = ((mint)_pChain >> 1);
	}

	CXR_VBChain *GetVBChain()
	{
		return (CXR_VBChain *)(m_Chain << 1);
	}

	CXR_VBIDChain *GetVBIDChain()
	{
		return (CXR_VBIDChain *)(m_Chain << 1);
	}

};


class CBSP_VBQueue
{
public:
	int m_iSurface;
	int m_LMTextureID;
	CBSP_VBChain m_VBQueue;

	CBSP_VBQueue()
	{
		m_iSurface = 0;
		m_LMTextureID = 0;
		m_VBQueue.Clear();
	}

	void AddChain(CBSP_VBChain *_pVB);
	void AddChain(CXR_VBIDChain *_pVB);
	void AddChain(CXR_VBChain *_pVB);
	void AddFromVB(CXR_VertexBuffer *_pVB);
	void Clear();
};

class CVBFaceQueue
{
public:
	uint32* m_pQueue;
	uint32 m_nElem;
	uint32 m_MaxElem;
	uint32 m_DynamicLightMask;

	CVBFaceQueue()
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
/*class CXR_LightSphere : public CReferenceCount
{
public:
	TArray<uint8> m_lColorMap;
	CVec3Dint m_Size;
	CVec3Dint m_Modulo;
	int m_Radius;

	CXR_LightSphere();
	void Create(int _Size);
	CPixel32 GetColor(const CVec3Dfp32& _v);

};*/

/*
class CClipBox
{
public:
	CVec3int m_Offset;
	CBox3int m_Clip;
};

class CImage3 : public CImage
{
protected:
	int mDepth;
	int mModuloZ;

public:
	inline int GetDepth() { return mDepth; };

	void Create(int _x, int _h, int _z, int _Fmt, int _MemModel);
	void Blt(const CClipBox& _Clip, const CImage3& _Img, const CVec3int& _Pos, int _Mode);
}
*/

class CBSP_FogBox : public CReferenceCount
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

	CBSP_FogBox();
	void Create(const CBox3Dfp32& _Box, const CVec3Dfp32& _BoxSize);
	void Create(const CBSP_LightVolumeInfo& _LightVol);

	void Clear();
	void AddLight(const CXR_Light* _pL);
	void SetAlphaFromBrightness();
	void Filter();

	CPixel32 GetFog(const CVec3Dfp32& _v, int _Alpha) const;
	CPixel32 GetFog_BoxSpace(const CVec3Dfp32& _v, int _Alpha) const;

	void GetFogBoxVectors(const CVec3Dfp32& _p0, const CVec3Dfp32& _v, CVec3Dfp32& _fp0, CVec3Dfp32& _fv) const;
};

typedef TPtr<CBSP_FogBox> spCBSP_FogBox;

// -------------------------------------------------------------------
//  Wallmark structures
// -------------------------------------------------------------------
class CBSP_Wallmark : public CXR_WallmarkDesc
{
public:
	uint16 m_iV;
	uint16 m_nV;
};

class CBSP_WallmarkContext : public CReferenceCount
{
public:
	TThinArray<CVec3Dfp32> m_lV;
	TThinArray<CVec3Dfp32> m_lN;
	TThinArray<CVec2Dfp32> m_lTV;
	TThinArray<CVec2Dfp32> m_lTV2;
	TThinArray<CPixel32> m_lCol;
	int m_nV;
	int m_iVTail;
	int m_iVHead;

	CXR_SurfaceContext* m_pSC;

	TPtr<TQueue<CBSP_Wallmark> > m_spqWM;
	TPtr<CBSP_LinkContext> m_spLink;
//	TPtr<CIDHeap> m_spWMAlloc;

	void Clear();

	void FreeWallmark();
	int AddWallmark(const CBSP_Wallmark& _WM);
	int MaxPut() const;
	int AddVertices(int _nV);

	CBSP_WallmarkContext();
	void Create(CXR_Model_BSP* _pModel, int _nV);
};

typedef TPtr<CBSP_WallmarkContext> spCBSP_WallmarkContext;

// -------------------------------------------------------------------
class CXR_Model_BSP;

class CXR_Model_BSP : 
	public CXR_SceneGraphInterface, 
	public CXR_ViewClipInterface, 
	public CXR_PhysicsModel, 
//	public CXR_PathInterface, 
	public CXR_FogInterface,
	public CXR_WallmarkInterface,
	public CXR_PortalSurfaceRender,
	public CXR_VBContainer
{
public:

	DECLARE_OPERATOR_NEW

	MRTC_DECLARE;

	friend class CXR_Material_LightMaps;

public:
	void LinkBox_r(CBSP_LinkContext* _pLinkCtx, int _iNode, const CBox3Dfp32& _Box);

	TPtr<CXR_Model_BSP> m_spMaster;						// Pointer to master-model, if any.
														// The master-model is the world. Entities share
														// data with the master-model in order to save memory.

	// Settings
	static int ms_Enable;

	static int ms_LastMedium;
	static fp32 ms_BasePriority_Opaque;
	static fp32 ms_BasePriority_Transparent;

	int16 m_nStructureLeaves;

	// Const.
	TThinArray<CPlane3Dfp32> m_lPlanes;
	TThinArray<CVec3Dfp32> m_lVertices;
	TArray<uint32> m_liVertices;
	TThinArray<CBSP_Face> m_lFaces;
	TThinArray<uint32> m_liFaces;
	TThinArray<CBSP_Node> m_lNodes;
	TThinArray<CBSP_PortalLeafExt> m_lPortalLeaves;
	TThinArray<CBSP_PortalExt> m_lPortals;
	TThinArray<CBSP_FogPortalFace> m_lFogPortalFaces;
	TThinArray<CBSP_FogPortal> m_lFogPortals;
	TThinArray<uint8> m_lPVS;

	TThinArray<uint16> m_liPortals;
	TThinArray<uint32> m_liPLFaces;

	TThinArray<spCSplineBrush> m_lspSplineBrushes;
	spCBSP_LinkContext m_spSBLink;						// Portal-leaf linkage for spline-brushes.

	TArray<uint16> m_lSBPrim;

//	TArray<CBSP_Leaf> m_lLeaves;
	TThinArray<CXR_PlaneMapping> m_lMappings;
	TArray<spCXW_Surface> m_lspSurfaces;
	TArray<CXR_MediumDesc> m_lMediums;

	// Generated data
	TArray<spCBSP_VBInfo> m_lspVBInfo;					// m_lspVBInfo and m_lspVB are parallell arrays.
	TArray<spCBSP_VertexBuffer> m_lspVB;
	TArray<CBSP_VBQueue> m_lVBQueues;
	fp32 m_VBSBTessLevel;

	TThinArray<CBSP_VBFacePrim> m_lVBFacePrim;
	TThinArray<uint16> m_lFacePrim;

	// -------------------------------------------------------------------
	// Light

		// LightMaps
		TArray<CBSP_LightMapInfo> m_lLightMapInfo;
		TPtr<CBSP_LightMapContainer> m_spLightMapContainer;

		spCTextureContainer m_spLMTC;
		TArray<CPnt> m_lLMDimensions;

		// Lightvolumes
		TThinArray<CBSP_LightVolumeInfo> m_lLightVolumeInfo;
		TThinArray<CXR_LightGridPoint> m_lLightCells;

		TPtr<CBSP_LightOcttree> m_spLightOcttree;

		// LightVertices
		TThinArray<CBSP_LightVerticesInfo> m_lLightVerticesInfo;
		TThinArray<int32> m_lLightVertices;

		// Smoothing normals
		TThinArray<uint32> m_liLightNormals;

	// -------------------------------------------------------------------
	TArray<uint32> m_lFaceLightMask;
	TArray<uint32> m_lLightElemTagList;
	TArray<uint8> m_lFaceTagList;
	TArray<uint32> m_lFaceUntagList;
	int m_nTagFaces;

	// Pointers into the lists above.
	CPlane3Dfp32* m_pPlanes;
	CVec3Dfp32* m_pVertices;
	uint32* m_piVertices;
	CBSP_Face* m_pFaces;
	uint32* m_piFaces;
	CBSP_Node* m_pNodes;
	int8* m_pVVertMask;
	uint16* m_piVVertTag;
	CBSP_PortalLeafExt* m_pPortalLeaves;
	CBSP_PortalExt* m_pPortals;
	uint16* m_piPortals;

	TArray<CPixel32> m_lVertexFog;
	TArray<spCBSP_FogBox> m_lspFogBoxes;

	CBSP_LightVerticesInfo* m_pLightVerticesInfo;
	int32* m_pLightVertices;

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
	TThinArray<spCBSPModel_ViewInstance> m_lspViews;
	int m_iView;
	CBSPModel_ViewInstance* m_pView;
	int m_CurrentView;

	// Face-queue
	TThinArray<uint32> m_lFaceQueueHeap;
	TThinArray<CVBFaceQueue> m_lVBFaceQueues;


	// Timing & statistics
#ifdef M_Profile

	static int ms_nPortal0;
	static int ms_nPortal1;
	static int ms_nPortal2;
	static int ms_nPortal3;

	static int ms_nFacesChecked;
	static int ms_nFacesFront;
	static int ms_nFacesVisible;
	static int ms_nSurfSwitch;

	static CMTime ms_TimeSurf;
	static CMTime ms_TimeBrushVB;
	static CMTime ms_TimeSplineVB;
	static CMTime ms_TimeCull;
	static CMTime ms_TimeBuild;

	static CMTime ms_Time;
	static CMTime ms_TimePortals;
	static CMTime ms_TimePortalAnd;
	static CMTime ms_TimePortalOr;
	static CMTime ms_TimePortal0;
	static CMTime ms_TimePortal1;
	static CMTime ms_TimePortal2;
	static CMTime ms_TimePortal3;
	static CMTime ms_TimeRenderLeafList;
	static CMTime ms_TimeRenderFaceQueue;
	static CMTime ms_TimeRenderPoly1;
	static CMTime ms_TimeRenderPoly2;
#endif

	TThinArray<uint32> m_liRefFaces;		// Faces with reflections or portals.


protected:
	fp32 m_BoundRadius;
	CBox3Dfp32 m_BoundBox;

	// Temp. storage
	static CVec2Dfp32 ms_PortalOrMin;
	static CVec2Dfp32 ms_PortalOrMax;

	static CXR_VBManager* ms_pVBM;
	static CMat4Dfp32* ms_pVBMatrixM2V;
	static CMat4Dfp32* ms_pVBMatrixM2W;
	static CMat4Dfp32* ms_pVBMatrixW2V;

	static CXR_Engine* ms_pCurrentEngine;
	static CXR_FogState* ms_pSceneFog;
	static CXR_RenderInfo ms_RenderInfo;
	static int ms_CurrentTLEnable;
	static int ms_bCurrentDynLightCullBackFace;
	static int ms_CurrentRCCaps;

	static int ms_SurfOptions;
	static int ms_SurfCaps;
	
	static CRC_Attributes* ms_Render_lpAttribDynLight[MODEL_BSP_MAXDYNAMICLIGHTS];
	static CRC_Attributes* ms_Render_lpAttribDynLight_DestAlphaExpAttn[MODEL_BSP_MAXDYNAMICLIGHTS];

	static int ms_TextureID_AttenuationExp;
#if	!defined(PLATFORM_PS2 ) && !defined(PLATFORM_DOLPHIN)
	static int ms_TextureID_Normalize;
	static int ms_TextureID_CubeNoise;
	static int ms_TextureID_Specular[8];
#endif
	static int ms_TextureID_DynLightProj;


	spCXR_WorldLightState m_spCurrentWLS;
	static spCXR_WorldLightState m_spTempWLS;

	void ExpandFaceBoundBox_r(int _iNode, CBox3Dfp32& _Box);
	int GetFaceList_r(int _iNode, uint32* _pDest, int _Pos, int _MaxFaces, int _iSurf = -1, int _SurfFlags = -1);
	int GetPLFaceList_r(int _iPL, uint32* _pDest, int _MaxFaces, int _iSurf = -1, int _SurfFlags = -1);
public:
	int CompareFaces(const CBSP_Face* _pF1, const CBSP_Face* _pF2);
	int CompareFaces(int _iFace1, int _iFace2);
	int CompareFaces_Index(int _iFace1, int _iFace2);
	int CompareSurface(int _iFace, int _SBID);
	int CompareSBFaces(int _iSB1, int _iFace1, int _iSB2, int _iFace2);
	int CompareSBIDs(int _SBID1, int _SBID2);
protected:
	void SortFaceList(uint32* _pSrc, uint32* _pDest, int _nF);
	void SortSBFaceList(uint32* _pSrc, uint32* _pDest, int _nF);
	void VBSortFaceList(uint32* _pSrc, uint32* _pDest, int _nF);

	void CreateSBTess(CBSP_VertexBuffer* _pVB, int _iV, int _iP, CSplineBrush* _pSB, int _iFace);
	void CreateSBPrim(int _iV, CSplineBrush* _pSB, int _iFace, int _iSBPrim);

	void CountVertexBuffer(uint32* _piFaces, int _nFaces, uint32* _pSBFaces, int _nSBFaces, int& _nVB, int& _nSBPrim);
	void PrepareVertexBuffer(uint32* _piFaces, int _nFaces, uint32* _pSBFaces, int _nSBFaces, int& _iVB, int& _iSBPrim);
	void CreateVertexBuffer(int _iVB);

	CBSP_VertexBuffer* GetVertexBuffer(int _iVB);
	void CreatePhysVertIndices();

	void CountVBFP(uint32* _piFaces, int _nFaces, int& _nVBFP, int& _nFacePrim);
	void CalcBoundBox(const uint32* _piFaces, int _nFaces, CBox3Dfp32& _BoundBox);
	void CreateVBFP(uint32* _piFaces, int _nFaces, int& _iVBFP, int& _iFacePrim);
	
	void InitBound();
	void InitPortalBounds();
	void InitNodePlaneTypes();
	void InitLightVolumeInfo();

	// -------------------------------------------------------------------
	// Face enumeration.
	static uint32* ms_piEnum;
	static uint16* ms_piEnumPL;
	static int ms_nEnum;
	static int ms_MaxEnum;
	static int ms_EnumQuality;
	static int ms_EnumMedium;
	static int ms_EnumFaceFlags;
	static CBox3Dfp32 ms_EnumBox;
	static CVec3Dfp32 ms_EnumSphere;
	static fp32 ms_EnumSphereR;
	static bool ms_EnumError;
	static int ms_nEnumUntag;
	static int ms_iObj;
	static CPotColSet *ms_pcs;
	static uint8 ms_bIN1N2Flags;

	enum
	{
		ENUM_HQ = 1,
		ENUM_FACEFLAGS = 2,
		ENUM_MEDIUMFLAGS = 4,
	};

	void EnumFaces_Sphere_r(CXR_PhysicsContext* _pPhysContext, int _iNode);
	void EnumFaces_Box_r(CXR_PhysicsContext* _pPhysContext, int _iNode);
	void EnumFaces_All_r(CXR_PhysicsContext* _pPhysContext, int _iNode);

	int EnumFaces_Sphere(CXR_PhysicsContext* _pPhysContext, int _iNode, uint32* _piFaces, int _MaxFaces, int _Quality, int _Medium, int _FaceFlags, const CVec3Dfp32& _Origin, fp32 _Radius, uint16* _piFacePL = NULL);
	int EnumFaces_Box(CXR_PhysicsContext* _pPhysContext, int _iNode, uint32* _piFaces, int _MaxFaces, int _Quality, int _Medium, int _FaceFlags, const CBox3Dfp32& _Box, uint16* _piFacePL = NULL);
	int EnumFaces_All(CXR_PhysicsContext* _pPhysContext, int _iNode, uint32* _piFaces, int _MaxFaces);
	void EnumFaces_Untag(CXR_PhysicsContext* _pPhysContext);
	int EnumError() { return ms_EnumError; };

	// -------------------------------------------------------------------
	int CountFaces_r(int _iNode);
	void InitFaces();

	int Light_TraceLine(int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1);
	int Light_TraceLine_Bounded(int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1);

	// -------------------------------------------------------------------
	//  Fog
	static CBox3Dfp32 ms_Fog_LastBox;

	void Fog_InitFogBox(int _iPL);
	void Fog_AddLightVolume(CBSP_FogBox* _pBox, int _iLightVolume);
	void Fog_BuildFogBox(int _iPL);

	void Fog_RenderPortal(int _iFirstNode, int _iPL, int _ip, int _iStartNode);
	void Fog_RenderPortal_HW(int _iFirstNode, int _iPL, int _ip, int _iStartNode);
	int Fog_GetVisiblePortals(int _iPL, uint32* _piPortals, int _MaxPortals);
	void Fog_RenderPortals(int _iPL, uint32* _piPortals, int _nP);
	bool Fog_SetVertices(CXR_VertexBuffer* pVB, const uint32* _piFaces, int _nFaces, const uint32* _piFaceVerts, int _iPLVBase);
	bool Fog_SetAttrib(CRC_Attributes* _pVB, CBSP_PortalLeafExt* _pPL, const CVec3Dfp32& _LocalEye);

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
	void RenderPortalFence(int _iPortal, CPixel32 _Color);
	void RenderRPortal(const CRC_ClipVolume* _pPortal, CPixel32 _Color);
	void RenderNodePortals(int _iNode);

	void GetLightVertices(int _iFace, CPixel32* _pLVerts, CVec4Dfp32* _pDynBumpDir = NULL);

	// Temporary variables
	static const CXR_AnimState* m_pCurAnimState;
	static CXR_MediumDesc* m_pCurMedium;
	static CXW_Surface* m_pCurSurface;
	static CXW_SurfaceKeyFrame* m_pCurSurfaceKey;
	static CXW_SurfaceKeyFrame m_TmpSurfKeyFrame;

	void GetFaceBoundBox(int _iFace, CBox3Dfp32& _Box);

	// Override from CXR_PortalSurfaceRender
	virtual void RenderPortalSurface(CXR_Engine* _pEngine, void* _pSurfContext, const CVec3Dfp32* _pVertices, int _nVertices, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);

	void RenderSplineBrushFace_VB(CSplineBrush* _pBrush, int _iFace, int _iPL);
	void RenderSplineBrushFace(CSplineBrush* _pBrush, int _iFace, bool _bUseCompiled, int _iPL = 0);
	void RenderSplineBrushes(bool _bNoCull = false);
	void RenderSplineBrushes(int _iPL);

	void RenderGetPrimCount(const uint32* _piFaces, int _nFaces, int& _nV, int& _nPrim);
	void RenderGetPrimCount_KnitStrip(const uint32* _piFaces, int _nFaces, int& _nV, int& _nPrim);
	void Tesselate(const uint32* _piFaces, int _nFaces, int _nV, CVec3Dfp32* _pV, CVec2Dfp32* _pTV1, CVec2Dfp32* _pTV2, CPixel32* _pCol, CVec3Dfp32* _pN, CVec3Dfp32* _pTangU, CVec3Dfp32* _pTangV, uint32* _piFaceVertices, CXW_Surface* _pSurf);
	int RenderTesselatePrim(const uint32* _piFaces, int _nFaces, uint16* _pPrim);
	int RenderTesselatePrim_KnitStrip(const uint32* _piFaces, int _nFaces, uint16* _pPrim);
	bool RenderTesselateVBPrim(const uint32* _piFaces, int _nFaces, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB);
	bool RenderTesselate(const uint32* _piFaces, int _nFaces, int _TessFlags, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, uint32* _piFaceVertices);
	void RenderSurface(CMTime _AnimTime, CXW_Surface* _pSurf, CXW_SurfaceKeyFrame* _pSurfKey, 
		CXR_VertexBuffer* _pVB, int _LightMapTextureID, bool _bAddLight);

	void VB_CopyGeometry(int _iVB, CBSP_VBChain* _pVB);

	void VB_RenderFaces(int _iVB, const uint32* _piFaces, int _nFaces, CXW_Surface* _pSurf, bool _bAddLight);
	void VB_RenderFaces(int _iVB, const uint32* _piFaces, int _nFaces, CXW_Surface* _pSurf);

	void VB_ClearFaceQueues();
	void VB_RenderFaceQueues();

	void VB_ClearQueues();
	void VB_RenderQueues();

	void VB_Flush();
	void VB_AddPrimitives(int _iVB, const uint16* _pPrim, int _nPrim);
	void VB_Tesselate(int _iVB, const uint32* _piFaces, int _nFaces);

	void VB_AllocVBID();
	void VB_FreeVBID();

	// -------------------------------------------------------------------
	// CXR_VBContainer overrides
	virtual int GetNumLocal();
	virtual int GetID(int _iLocal);
	virtual void Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags);
	virtual void Release(int _iLocal);

	// -------------------------------------------------------------------
//	void RenderSurfaceCluster(CBSP_VBSurfaceCluster* _pCluster, int _iVB, CBSP_PortalLeafExt* _pPL = NULL);

	void RenderNHF(const uint32* _piFaces, int _nFaces, int _iPL);
	void VB_RenderNHF(int _iVB, const uint16* _piPrim, int _nPrim, CBSP_PortalLeafExt* _pPL);

	void RenderPortalLeaf(int _iPL);
	void RenderLeafList();

	void UntagVertices();
	void UntagFogVertices();

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

	void InitBoundNodesFromEnabledNodes(int _iNode, int _iNodeLastSplit, int _iNodeLastSplitSide);
	void DisableTree(int _iNode);
	void DisableTreeFlag(int _iNode, int _Flag);

	bool CheckTree_r(int _iNode, int _iParent = 0, int _PortalDone = 0);

	int Structure_GetFirstSplitNode_Polyhedron(const CVec3Dfp32* _pV, int _nVertices);
	void Structure_EnableTreeFromNode_Polyhedron(int _iNode, const CVec3Dfp32* _pV, int _nVertices);
	void Structure_InitBoundNodesFromEnabledNodes(int _iNode, int _iNodeLastSplit, int _iNodeLastSplitSide);

	// -------------------------------------------------------------------
	// Portal stuff
	int Portal_And(const CRC_ClipVolume* _pPortal, const CBSP_PortalExt* _pP, bool _bFrontThis, CRC_ClipVolume* _pPDest);
	void Portal_Or(CRC_ClipVolume* _pP1, const CRC_ClipVolume* _pP2);
	void Portal_AddNode(int _iNode, int _iClipRPortal);

	void Portal_OpenClipFront_r(CBSP_Node* pNode, const CVec3Dfp32* _pVPortal, int _nVPortal);
	void Portal_OpenClipBack_r(CBSP_Node* pNode, const CVec3Dfp32* _pVPortal, int _nVPortal);
	void Portal_Open_r(int _iNode, const CVec3Dfp32* _pVPortal, int _nVPortal);
//	int Portal_GetFirstOpen(const CVec3Dfp32& _v);
	int Portal_GetFirstOpen_r(int _iNode, const CVec3Dfp32& _v);

	// -------------------------------------------------------------------
	// Dynamic lighting
	void Light_TagDynamics();
	void Light_UnTagDynamics();
	void Light_TagSplineBrushes(int _iPL, const CVec3Dfp32& _Pos, fp32 _Range, int _Mask);
	void Light_FlagSplineBrush(int _iSB, const CVec3Dfp32& _Pos, fp32 _Range, int _Mask);
	bool Light_IntersectSphere_CheckFace(int _iFace, const CVec3Dfp32& _Pos, fp32 _Radius);
	void Light_FlagTree(int _iNode, const CVec3Dfp32& _Pos, fp32 _Range, int _Mask);

	void Light_ProjectDynamic(int _iFace, CXR_Light* _pL, CXR_VertexBuffer* _pVB, int _iV);
	bool Light_ProjectDynamic(int _nFaces, const uint32* _piFaces, CXR_Light* _pLight, CXR_VertexBuffer* _pVB);

	void Light_SetAttenuation(CRC_Attributes* _pA, CXR_Light* _pL, int _iTxtChnBase) const;
	CRC_Attributes* Light_GetAttributes_NV20_ExpAttn_NdotL(int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf);
	CRC_Attributes* Light_GetAttributes_NV20_ExpAttn_NdotL_DestAlpha(int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf);
	CRC_Attributes* Light_GetAttributes_NV20_ExpAttn_NMapdotL(int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf, int _TextureIDBump);
	CRC_Attributes* Light_GetAttributes_NV20_ExpAttn_NMapdotH(int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf, int _TextureIDBump);
	CRC_Attributes* Light_GetAttributes_NV20_ExpAttn_NMapHSpaceCubeMap(int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf, int _TextureIDBump, int _TextureIDSpec);
	CRC_Attributes* Light_GetAttributes_NV20_DestAlphaExpAttn(int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf, int _TextureIDBump);
	bool Light_ProjectDynamic_NV20(CXR_VertexBuffer* _pVB, int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf);
	bool Light_ProjectDynamic_MultiTexture(CXR_VertexBuffer* _pVB, int _iDynamic, CXR_Light* _pL, CXW_Surface* _pSurf);

	void Light_RenderDynamic(int _iFace, int _iDynLight);
	static void Light_ProjectDynamic(CXR_Light* _pLight, const CVec3Dfp32& _UVec, const CVec3Dfp32& _VVec, const CPlane3Dfp32& _Plane, int _nV, const CVec3Dfp32* _pV, CVec2Dfp32* _pDestTV, CPixel32* _pDestCol);

	int NumFacesInTree(int _iNode);
	int GetLeaf(const CVec3Dfp32& _v);
	int GetPortalLeaf(const CVec3Dfp32& _v);	// This stupid returns a node index, not a portal-leaf index.

	int GetStructurePortalLeaf(const CVec3Dfp32& _v);
//	int GetNavigationPortalLeaf(const CVec3Dfp32& _v);

	virtual void Clear();
	void InitializeListPtrs();

public:
	CXR_Model_BSP();
	~CXR_Model_BSP();
	virtual void Create(const char* _pParam, CDataFile* _pDFile, CCFile* _pFile, const CBSP_CreateInfo& _CreateInfo);
	void Create_PhongMap();
	void Create_ViewInstances(int _nInst);
	void Create_PostRead();

	// Overrides
	virtual aint GetParam(int _Param);
	virtual void SetParam(int _Param, aint _Value);
	virtual int GetParamfv(int _Param, fp32* _pRetValues);
	virtual void SetParamfv(int _Param, const fp32* _pValues);

	virtual int GetModelClass() { return CXR_MODEL_CLASS_BSP1; };
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState = NULL);

	virtual void PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
			const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
	virtual void OnPrecache(CXR_Engine* _pEngine, int _iVariation);

	CStr GetInfo();

protected:

	// -------------------------------------------------------------------
	// Scenegraph internal functions
	int SceneGraph_PVSFindSlot(int _iPL);
	int SceneGraph_PVSFindFreeSlot();
	const uint8* SceneGraph_PVSLock(int _PVSType, int _iPortalLeaf);

	TArray<TArray<uint8> > m_lPVSCache;
	TArray<int> m_liPVSCachePL;
	TArray<int> m_lnPVSCacheRef;

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

/*	virtual int SceneGraph_CreateContext(int _ElemHeap);
	virtual void SceneGraph_Destroy(int _hSG);
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

	void __Phys_RenderFace(int _iFace, const CMat4Dfp32& _WMat, CWireContainer* _pWC, int _Col);

	static bool __Phys_IntersectSphere_Polygon(const CVec3Dfp32* _pV, const uint32* _piV, int _nV,
		const CPlane3Dfp32& _Plane, const CVec3Dfp32& _Pos, fp32 _Radius, CCollisionInfo* _pCollisionInfo);

	bool __Phys_IntersectSphere_CheckFace(int _iFace, const CVec3Dfp32& _Pos, fp32 _Radius, CCollisionInfo* _pCollisionInfo);
	bool __Phys_IntersectSphere_r(int _iNode, const CVec3Dfp32& _Pos, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo);

	bool __Phys_IntersectSphere_Triangle(const CVec3Dfp32& _V0, const CVec3Dfp32& _V1, const CVec3Dfp32& _V2, const CVec3Dfp32& _Pos, fp32 _Radius, CCollisionInfo* _pCollisionInfo);
	bool __Phys_IntersectSphere_SB(const CVec3Dfp32& _Pos, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo);

	bool __Phys_IntersectOBB_CheckFace(int _iFace, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, CCollisionInfo* _pCollisionInfo);
	bool __Phys_IntersectOBB(CXR_PhysicsContext* _pPhysContext, int _iNode, const CPhysOBB& _BoxStart, const CPhysOBB& _Box, const CBox3Dfp32 _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo);
	bool __Phys_IntersectOBB_SB(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxStart, const CPhysOBB& _BoxDest, const CBox3Dfp32 _Bound, int _MediumFlags, CCollisionInfo* _pCollisionInfo);

	bool __Phys_TraceLine_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, class CPhysLineContext* _pLineContext, CCollisionInfo* _pCollisionInfo);
	bool __Phys_TraceLine_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, class CPhysLineContext* _pLineContext);

	int __Phys_GetCombinedMediumFlags_r(CXR_PhysicsContext* _pPhysContext, int _iNode, const CBox3Dfp32* _pBox);
	
	void CollectPCS_r(CXR_PhysicsContext* _pPhysContext, int _iNode);
	void CollectPCS_IsLeaf(CXR_PhysicsContext* _pPhysContext, const CBSP_Node* _pNode);
	void CollectPCS_IsStructureLeaf(CXR_PhysicsContext* _pPhysContext, int _iPL);

public:
	// -------------------------------------------------------------------
	// Physics overrides
	virtual void Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _v0, fp32& _Radius);
	virtual void Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _Box);

	virtual void Phys_Init(CXR_PhysicsContext* _pPhysContext);
	virtual bool Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, CCollisionInfo* _pCollisionInfo = NULL);

	virtual void CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet *_pcs, const int _iObj, const int _MediumFlags );

	virtual int Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0);
	virtual void Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium);
	virtual int Phys_GetCombinedMediumFlags(CXR_PhysicsContext* _pPhysContext, const CBox3Dfp32& _Box);

	// -------------------------------------------------------------------
	// Visibility internal functions
private:
	static CVec3Dfp32 ms_VisVMin;
	static CVec3Dfp32 ms_VisVMax;
	static CRC_ClipVolume* ms_pVisClipVolume;
	static CXR_RenderInfo* ms_pVisRenderInfo;
	static int ms_iClosestRPortal;
	CXR_Engine* m_Vis_pEngine;
	static CXR_FogState* ms_Vis_pSceneFog;

	bool View_GetClip_Sphere_r(int _iNode, const CVec3Dfp32& _v0, fp32 _Radius);
	bool View_GetClip_Box_r(int _iNode);

	// -------------------------------------------------------------------
	// Visibility overrides
public:
	virtual void View_Reset(int _iView);
	virtual void View_SetState(int _iView, int _State, int _Value);

	virtual void View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_ViewClipInterface* _pViewClip,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);
	virtual void View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CVec3Dfp32* _pVPortal, int _nVPortal,
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);
	virtual void View_SetCurrent(int _iView, CXR_SceneGraphInstance* _pSceneGraphInstance);
	virtual bool View_GetClip_Sphere(CVec3Dfp32 _v0, fp32 _Radius, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo);
	virtual bool View_GetClip_Box(CVec3Dfp32 _min, CVec3Dfp32 _max, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo);

	virtual CXR_ViewClipInterface* View_GetInterface() { return this; };

//	virtual void View_SetPortalState(int _iPortal, int State);

	// -------------------------------------------------------------------
protected:
	TPtr<CBSP_WallmarkContext> m_spWMCTemp;
	TPtr<CBSP_WallmarkContext> m_spWMC;
	TPtr<CBSP_WallmarkContext> m_spWMCStatic;

	virtual void Wallmark_Render(CBSP_WallmarkContext* _pWMC, const CBSP_Wallmark* _pWM, CMTime _Time);
	virtual void Wallmark_Render(CBSP_WallmarkContext* _pWMC, int _iPL);
	bool Wallmark_Create(CBSP_WallmarkContext* _pWMC, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _iPL, const CVec3Dfp32* _pV, int _nV, int _GUID);
	virtual int Wallmark_Create(CBSP_WallmarkContext* _pWMC, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material);

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

typedef TPtr<CXR_Model_BSP> spCXR_Model_BSP;



///////////////////////////////////////////////////////////////////////////////
struct CBSP_BoxInterleaved
{
	fp32 m_MinX, m_MaxX;
	fp32 m_MinY, m_MaxY;
	fp32 m_MinZ, m_MaxZ;

	CBSP_BoxInterleaved() {}

	explicit CBSP_BoxInterleaved(const CBox3Dfp32& _Box)
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
                                   const CBSP_BoxInterleaved& _Box, 
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

#endif // _INC_WBSPMODEL

