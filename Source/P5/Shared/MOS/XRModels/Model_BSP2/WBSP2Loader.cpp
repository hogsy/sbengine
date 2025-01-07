#include "PCH.h"

#include "WBSP2Model.h"
#include "WBSP2Def.h"

#include "MFloat.h"

#include "../../MSystem/Raster/MTextureContainers.h"

// #define IGNORE_LIGHTMAP

// #define MODEL_BSP_CHECKTREE

// -------------------------------------------------------------------
void CXR_Model_BSP2::InitPortalBounds()
{
	MAUTOSTRIP(CXR_Model_BSP_InitPortalBounds, MAUTOSTRIP_VOID);
	CBSP2_PortalExt* pP = m_lPortals.GetBasePtr();
	int np = m_lPortals.Len();
	for(int i = 0; i < np; i++)
	{
		int iiv = pP[i].m_iiVertices;
		int nv = pP[i].m_nVertices;

		CVec3Dfp32 VMin(_FP32_MAX);
		CVec3Dfp32 VMax(-_FP32_MAX);
		for(int v = 0; v < nv; v++)
		{
			int iv = m_liVertices[iiv + v];
			CVec3Dfp32* pV = &m_lVertices[iv];

			for(int i = 0; i < 3; i++)
			{
				if (pV->k[i] < VMin.k[i]) VMin.k[i] = pV->k[i];
				if (pV->k[i] > VMax.k[i]) VMax.k[i] = pV->k[i];
			}
		}

		pP[i].m_BoxMin = VMin;
		pP[i].m_BoxMax = VMax;
	}
}

void CXR_Model_BSP2::ExpandFaceBoundBox_r(int _iNode, CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_ExpandFaceBoundBox_r, MAUTOSTRIP_VOID);
	TAP<CBSP2_Node> lNodes = m_lNodes;
	CBSP2_Node* pN = &lNodes[_iNode];
	if (pN->IsLeaf())
	{
		TAP<CBSP2_Face> lFaces = m_lFaces;
		TAP<uint32> liFaces = m_liFaces;
		TAP<CVec3Dfp32> lVertices = m_lVertices;
		TAP<uint32> liVertices = m_liVertices;

		int iiFaces = pN->m_iiFaces;
		int nFaces = pN->m_nFaces;
		for(int iif = 0; iif < nFaces; iif++)
		{
			const CBSP2_Face* pF = &lFaces[liFaces[iiFaces + iif]];
			for(int v = 0; v < pF->m_nVertices; v++)
			{
				const CVec3Dfp32* pV = &lVertices[liVertices[pF->m_iiVertices + v]];
				for(int i = 0; i < 3; i++)
				{
					if (pV->k[i] < _Box.m_Min.k[i]) _Box.m_Min.k[i] = pV->k[i];
					if (pV->k[i] > _Box.m_Max.k[i]) _Box.m_Max.k[i] = pV->k[i];
				}
			}
		}
	}
	else
	{
		if (pN->m_iNodeFront) ExpandFaceBoundBox_r(pN->m_iNodeFront, _Box);
		if (pN->m_iNodeBack) ExpandFaceBoundBox_r(pN->m_iNodeBack, _Box);
	}
}

int CXR_Model_BSP2::GetFaceList_r(int _iNode, uint32* _pDest, int _Pos, int _MaxFaces, int _iSurf, int _SurfFlags)
{
	MAUTOSTRIP(CXR_Model_BSP_GetFaceList_r, 0);
	const CBSP2_Node* pN = &m_pNodes[_iNode];
	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		if (nFaces > _MaxFaces) Error("GetFaceList_r", "Insufficient space in target array.");
		int iiFaces = pN->m_iiFaces;
		int nAdded = 0;
		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_liFaces[iiFaces + f];

			CBSP2_Face* pF = &m_lFaces[iFace];

			// Check surface
			if ((_iSurf >= 0) && (pF->m_iSurface != _iSurf)) continue;

			// Check surface flags
			CXW_Surface* pS = m_lspSurfaces[pF->m_iSurface];
			if (_SurfFlags != -1 && !(pS->m_Flags & _SurfFlags)) continue;

			int i;
			for(i = 0; i < _Pos+nAdded; i++)
				if (_pDest[i] == iFace) break;

			if (i == _Pos+nAdded)
			{
				_pDest[_Pos + nAdded] = iFace;
				nAdded++;
			}
		}
		return nAdded;
	}
	else
	{
		int Added = 0;
		int iFront = pN->m_iNodeFront;
		int iBack = pN->m_iNodeBack;
		if (iFront)
		{
			int nF = GetFaceList_r(iFront, _pDest, _Pos, _MaxFaces, _iSurf, _SurfFlags);
			_MaxFaces -= nF;
			Added += nF;
		}
		if (iBack)
		{
			int nF = GetFaceList_r(iBack, _pDest, _Pos + Added, _MaxFaces, _iSurf, _SurfFlags);
			_MaxFaces -= nF;
			Added += nF;
		}
		return Added;
	}
}

int CXR_Model_BSP2::GetPLFaceList_r(int _iPL, uint32* _pDest, int _MaxFaces, int _iSurf, int _SurfFlags)
{
	MAUTOSTRIP(CXR_Model_BSP_GetPLFaceList_r, 0);
	const CBSP2_PortalLeafExt& PL = m_lPortalLeaves[_iPL];

	int nFaces = PL.m_nFaces;
	if (nFaces > _MaxFaces) Error("GetFaceList_r", "Insufficient space in target array.");
	int iiFaces = PL.m_iiFaces;
	int nAdded = 0;
	for(int f = 0; f < nFaces; f++)
	{
		int iFace = m_liPLFaces[iiFaces + f];

		CBSP2_Face* pF = &m_lFaces[iFace];

		// Check surface
		if ((_iSurf >= 0) && (pF->m_iSurface != _iSurf)) continue;

		// Check surface flags
		CXW_Surface* pS = m_lspSurfaces[pF->m_iSurface];
		if (_SurfFlags != -1 && !(pS->m_Flags & _SurfFlags)) continue;

		_pDest[nAdded] = iFace;
		nAdded++;
	}
	return nAdded;
}


int CXR_Model_BSP2::CompareFaces_Index(int _iFace1, int _iFace2)
{
	MAUTOSTRIP(CXR_Model_BSP_CompareFaces_Index, 0);
	const CBSP2_Face* pF1 = &m_lFaces[_iFace1];
	const CBSP2_Face* pF2 = &m_lFaces[_iFace2];

#ifndef IGNORE_LIGHTMAP
	int iLMC1 = (pF1->m_Flags & XW_FACE_LIGHTMAP) ? 
		m_lLightMapInfo[pF1->m_iLightInfo].m_iLMC : -1;
	int iLMC2 = (pF2->m_Flags & XW_FACE_LIGHTMAP) ? 
		m_lLightMapInfo[pF2->m_iLightInfo].m_iLMC : -1;

	if (iLMC1 < iLMC2)
		return -1;
	else if (iLMC1 > iLMC2)
		return 1;

#endif
	int iSurf1 = pF1->m_iSurface;
	int iSurf2 = pF2->m_iSurface;

	uint bSurf1AlphaComp = m_lSurfaceShaderParams[iSurf1].m_Flags & XR_SHADERFLAGS_USEZEQUAL;
	uint bSurf2AlphaComp = m_lSurfaceShaderParams[iSurf2].m_Flags & XR_SHADERFLAGS_USEZEQUAL;
	if (bSurf1AlphaComp < bSurf2AlphaComp)
		return -1;
	else if (bSurf1AlphaComp > bSurf2AlphaComp)
		return 1;

	if (iSurf1 < iSurf2)
		return -1;
	else if (iSurf1 > iSurf2)
		return 1;
	else
	{
		{
			int iV1 = m_liVertices[pF1->m_iiVertices];
			int iV2 = m_liVertices[pF2->m_iiVertices];
			if (iV1 < iV2) 
				return -1;
			else if (iV1 > iV2) 
				return 1;
			return 0;
		}
	}
}

int CXR_Model_BSP2::CompareFaces(int _iFace1, int _iFace2)
{
	MAUTOSTRIP(CXR_Model_BSP_CompareFaces, 0);
	const CBSP2_Face* pF1 = &m_lFaces[_iFace1];
	const CBSP2_Face* pF2 = &m_lFaces[_iFace2];

#ifndef IGNORE_LIGHTMAP
	int iLMC1 = (pF1->m_Flags & XW_FACE_LIGHTMAP) ? 
		m_lLightMapInfo[pF1->m_iLightInfo].m_iLMC : -1;
	int iLMC2 = (pF2->m_Flags & XW_FACE_LIGHTMAP) ? 
		m_lLightMapInfo[pF2->m_iLightInfo].m_iLMC : -1;

	if (iLMC1 < iLMC2)
		return -1;
	else if (iLMC1 > iLMC2)
		return 1;
#endif

	int iSurf1 = pF1->m_iSurface;
	int iSurf2 = pF2->m_iSurface;

	uint bSurf1AlphaComp = m_lSurfaceShaderParams[iSurf1].m_Flags & XR_SHADERFLAGS_USEZEQUAL;
	uint bSurf2AlphaComp = m_lSurfaceShaderParams[iSurf2].m_Flags & XR_SHADERFLAGS_USEZEQUAL;
	if (bSurf1AlphaComp < bSurf2AlphaComp)
		return -1;
	else if (bSurf1AlphaComp > bSurf2AlphaComp)
		return 1;

	if (iSurf1 < iSurf2)
		return -1;
	else if (iSurf1 > iSurf2)
		return 1;
	else
	{
		return 0;
	}
}

int CXR_Model_BSP2::CompareFaces(const CBSP2_Face* _pF1, const CBSP2_Face* _pF2)
{
	MAUTOSTRIP(CXR_Model_BSP_CompareFaces_2, 0);
#ifndef IGNORE_LIGHTMAP
	int iLMC1 = (_pF1->m_Flags & XW_FACE_LIGHTMAP) ? 
		m_lLightMapInfo[_pF1->m_iLightInfo].m_iLMC : -1;
	int iLMC2 = (_pF2->m_Flags & XW_FACE_LIGHTMAP) ? 
		m_lLightMapInfo[_pF2->m_iLightInfo].m_iLMC : -1;

	if (iLMC1 < iLMC2)
		return -1;
	else if (iLMC1 > iLMC2)
		return 1;
#endif

	int iSurf1 = _pF1->m_iSurface;
	int iSurf2 = _pF2->m_iSurface;

	uint bSurf1AlphaComp = m_lSurfaceShaderParams[iSurf1].m_Flags & XR_SHADERFLAGS_USEZEQUAL;
	uint bSurf2AlphaComp = m_lSurfaceShaderParams[iSurf2].m_Flags & XR_SHADERFLAGS_USEZEQUAL;
	if (bSurf1AlphaComp < bSurf2AlphaComp)
		return -1;
	else if (bSurf1AlphaComp > bSurf2AlphaComp)
		return 1;

	if (iSurf1 < iSurf2)
		return -1;
	else if (iSurf1 > iSurf2)
		return 1;
	else
	{
		{
			int iV1 = m_liVertices[_pF1->m_iiVertices];
			int iV2 = m_liVertices[_pF2->m_iiVertices];
			if (iV1 < iV2) 
				return -1;
			else if (iV1 > iV2) 
				return 1;
			return 0;
		}
	}
}

class CBSP2_FaceSortElem
{
public:
	CBSP2_Face* m_pFace;
	CXR_Model_BSP2* m_pObj;

	int Compare(const CBSP2_FaceSortElem&) const;
};

int CBSP2_FaceSortElem::Compare(const CBSP2_FaceSortElem& _Elem) const
{
	MAUTOSTRIP(CFaceSortElem_Compare, 0);
	return m_pObj->CompareFaces(m_pFace, _Elem.m_pFace);
}

void CXR_Model_BSP2::SortFaceList(uint32* _pSrc, uint32* _pDest, int _nF)
{
	MAUTOSTRIP(CXR_Model_BSP_SortFaceList, MAUTOSTRIP_VOID);
	if (!_nF) return;

	TArray_Sortable<CBSP2_FaceSortElem> lSort;
	lSort.SetLen(_nF);

	int i;
	for(i = 0; i < _nF; i++)
	{
//		lSort[i].m_iFace = _pSrc[i];
		lSort[i].m_pFace = &m_lFaces[_pSrc[i]];
		lSort[i].m_pObj = this;
	}

	lSort.Sort();

	for(i = 0; i < _nF; i++)
	{
		_pDest[i] = lSort[i].m_pFace - m_lFaces.GetBasePtr();
//		_pDest[i] = lSort[i].m_iFace;
	}
}

// -------------------------------------------------------------------
class CBSP2_FaceVBSortElem
{
public:
	int m_iFace;
	CXR_Model_BSP2* m_pObj;

	int Compare(const CBSP2_FaceVBSortElem&) const;
};

int CBSP2_FaceVBSortElem::Compare(const CBSP2_FaceVBSortElem& _Elem) const
{
	MAUTOSTRIP(CFaceVBSortElem_Compare, 0);
	const CBSP2_Face& F0 = m_pObj->m_lFaces[m_iFace];
	const CBSP2_Face& F1 = _Elem.m_pObj->m_lFaces[_Elem.m_iFace];

	if (F0.m_iVB < F1.m_iVB)
		return -1;
	else if (F0.m_iVB > F1.m_iVB)
		return 1;
	else
		return 0;
}

void CXR_Model_BSP2::VBSortFaceList(uint32* _pSrc, uint32* _pDest, int _nF)
{
	MAUTOSTRIP(CXR_Model_BSP_VBSortFaceList, MAUTOSTRIP_VOID);
	if (!_nF) return;

	TArray_Sortable<CBSP2_FaceVBSortElem> lSort;
	lSort.SetLen(_nF);

	int i;
	for(i = 0; i < _nF; i++)
	{
		lSort[i].m_iFace = _pSrc[i];
		lSort[i].m_pObj = this;
	}

	lSort.Sort();

	if (_pDest)
	{
		for(i = 0; i < _nF; i++)
			_pDest[i] = lSort[i].m_iFace;
	}
	else
	{
		for(i = 0; i < _nF; i++)
			_pSrc[i] = lSort[i].m_iFace;
	}
}

// -------------------------------------------------------------------
class CBSP2_CSBFaceSortElem
{
public:
	int m_iFace;
	int m_iSB;
	CXR_Model_BSP2* m_pObj;

	int Compare(const CBSP2_CSBFaceSortElem&) const;
};

static int GetSurfaceFlagsAllVersions(CXW_Surface* _pSurf);
static int GetSurfaceFlagsAllVersions(CXW_Surface* _pSurf)
{
	MAUTOSTRIP(GetSurfaceFlagsAllVersions, 0);
	int Flags = 0;
	while(_pSurf)
	{
		Flags |= _pSurf->m_Flags;
		_pSurf = _pSurf->m_spNextSurf;
	}

	return Flags;
}

void CXR_Model_BSP2::CountVertexBuffer(uint32* _piFaces, int _nFaces, int& _nVB)
{
	MAUTOSTRIP(CXR_Model_BSP_CountVertexBuffer, MAUTOSTRIP_VOID);
	// Counts the space required by PrepareVertexBuffer

//	int nF = _nFaces;

	// Count vertices
	int nPrim, nVert;
	RenderGetPrimCount(_piFaces, _nFaces, nVert, nPrim);

	if (nVert > 65535)
	{
		int nFaces1 = _nFaces >> 1;
		CountVertexBuffer(_piFaces, nFaces1, _nVB);
		_piFaces += nFaces1;
		CountVertexBuffer(_piFaces, _nFaces - nFaces1, _nVB);
		return;
	}

	_nVB++;
}

void CXR_Model_BSP2::PrepareVertexBuffer(uint32* _piFaces, int _nFaces, int& _iVB)
{
	MAUTOSTRIP(CXR_Model_BSP_PrepareVertexBuffer, MAUTOSTRIP_VOID);
	MSCOPESHORT(PrepareVertexBuffer);

	// Builds vertex-buffer creation data. (used for CreateVertexBuffer later)

	int nF = _nFaces;

	// Count vertices
	int nPrim, nVert;
	RenderGetPrimCount(_piFaces, _nFaces, nVert, nPrim);

//	int nBrushVert = nVert;

	if (nVert > 65535)
	{
		int nFaces1 = _nFaces >> 1;
		PrepareVertexBuffer(_piFaces, nFaces1, _iVB);
		_piFaces += nFaces1;
		PrepareVertexBuffer(_piFaces, _nFaces - nFaces1, _iVB);
		return;
	}
	M_ASSERT(nPrim < 0xffff, "Too many primitives.");


	// Create

	int iLMC = -1;
	int iSurf = 0;
	int bLightVertices = false;

	if (_nFaces)
	{
#ifndef IGNORE_LIGHTMAP
		if (m_spLMTC != NULL && m_lLMDimensions.Len() && m_lLMTextureIDs.Len())
		{
			iLMC = (m_lFaces[_piFaces[0]].m_Flags & XW_FACE_LIGHTMAP) ? 
					m_lLightMapInfo[m_lFaces[_piFaces[0]].m_iLightInfo].m_iLMC : -1;
		}
#endif
		iSurf = m_lFaces[_piFaces[0]].m_iSurface;
		if (m_lFaces[_piFaces[0]].m_Flags & XW_FACE_LIGHTVERTICES)
			bLightVertices = true;
	}
	else
		Error("PrepareVertexBuffer", "No data.");

	// Search for queue with same surface and lightmap
	uint q;
	for(q = 0; q < m_lSurfQueues.Len(); q++)
		if (m_lSurfQueues[q].m_iSurface == iSurf &&
			m_lSurfQueues[q].m_iLMTexture == iLMC) break;

	uint iSurfQueue = 0;
	if (q < m_lSurfQueues.Len())
		iSurfQueue = q;
	else
	{
		// Didn't find a queue, so we have to add a new one.
		CBSP2_SurfaceQueue Q;
		Q.m_iSurface = iSurf;
		Q.m_iLMTexture = iLMC;
		Q.m_SurfaceFlags = m_lspSurfaces[iSurf]->m_Flags;
		iSurfQueue = m_lSurfQueues.Add(Q);
	}

//	int iVB = m_lspVB.Add(spCBSP2_VertexBuffer(NULL));
	m_lspVB[_iVB] = NULL;

	// Set VB index on all faces
	for(int i = 0; i < _nFaces; i++)
		m_lFaces[_piFaces[i]].m_iVB = _iVB;



	int Components = 1;
	int SurfFlagsAll = GetSurfaceFlagsAllVersions(m_lspSurfaces[iSurf]);

	if (!(m_lspSurfaces[iSurf]->m_Flags & XW_SURFFLAGS_INVISIBLE))
		Components |= 1;
	if (iLMC >= 0)
		Components |= 2 + 32;
	if (bLightVertices)
		Components |= 4;
	if (SurfFlagsAll & XW_SURFFLAGS_NEEDNORMALS)
		Components |= 8;

//	if (SurfFlagsAll & XW_SURFFLAGS_NEEDTANGENTS ||
//		m_lspSurfaces[iSurf]->GetBaseFrame()->GetBumpMap() != NULL)
		Components |= 16;	// Always generate tangents

#if defined(PLATFORM_DOLPHIN) || defined(PLATFORM_PS2)
	if (!m_lPortalLeaves.Len())
		Components |= 8; // need normals for dynamic objects
#else
	Components |= 8; // need normals for dynamic light
#endif

	{
		MSCOPESHORT(VBInfo);

		CBSP2_VBInfo& VBInfo = m_lVBInfo[_iVB];

		VBInfo.m_liFaces.SetLen(_nFaces);
		VBInfo.m_Components = Components;
		VBInfo.m_MaxPrim = 1;
		VBInfo.m_iSurfQueue = iSurfQueue;
		VBInfo.m_nVertices = nVert;
		VBInfo.m_iSurface = iSurf;
		memcpy(VBInfo.m_liFaces.GetBasePtr(), _piFaces, VBInfo.m_liFaces.ListSize());
	}

//LogFile(CStrF("VB %d, MaxPrim %d, MaxVert %d, BrushVert %d", m_lVBInfo.Len()-1, nPrim, nVert, nBrushVert));

	// Init face VB vertex indices.
	{
/*		TThinArray<uint32> liFaceVert;
		liFaceVert.SetLen(nF);

		Tesselate(_piFaces, nF, nBrushVert, NULL, NULL, NULL, NULL, NULL, NULL, NULL, liFaceVert.GetBasePtr(), m_lspSurfaces[iSurf]);

		// Set vertex indices on all faces
		for(int f = 0; f < nF; f++)
		{
			m_lFaces[_piFaces[f]].m_iiVBVertices = liFaceVert[f];
			m_lFaces[_piFaces[f]].m_nVBVertices = m_lFaces[_piFaces[f]].m_nVertices;
		}
*/
		// Set vertex indices on all faces
		int iV = 0;
		CBox3Dfp32 BoundBox(CVec3Dfp32(_FP32_MAX), CVec3Dfp32(-_FP32_MAX));
		TAP_RCNRTM<CVec3Dfp32> pV = m_lVertices;
		TAP_RCNRTM<uint32> piV = m_liVertices;
		for(int f = 0; f < nF; f++)
		{
			CBSP2_Face& Face = m_lFaces[_piFaces[f]];
			Face.m_iiVBVertices = iV;
			//Face.m_nVBVertices = Face.m_nVertices;
			iV += Face.m_nVertices;
			for (int i = 0; i < Face.m_nVertices; ++i)
			{
				BoundBox.Expand(pV[piV[Face.m_iiVertices + i]]);
			}
		}
		fp32 Snap = 1.0 / 8.0;
		BoundBox.m_Min.Snap(Snap, 0.0f);
		BoundBox.m_Min -= CVec3Dfp32(1.0f);
		BoundBox.m_Max.Snap(Snap, 0.0f);
		BoundBox.m_Max += CVec3Dfp32(1.0f);

		m_lVBOffsets[_iVB] = BoundBox.m_Min;
		CVec3Dfp32 Scale = (BoundBox.m_Max - BoundBox.m_Min);
		m_MasterVBScale.k[0] = Max(m_MasterVBScale.k[0], Scale.k[0]);
		m_MasterVBScale.k[1] = Max(m_MasterVBScale.k[1], Scale.k[1]);
		m_MasterVBScale.k[2] = Max(m_MasterVBScale.k[2], Scale.k[2]);
	}

	_iVB++;
}

void CXR_Model_BSP2::CreateVertexBuffer(int _iVB)
{
	MAUTOSTRIP(CXR_Model_BSP_CreateVertexBuffer, MAUTOSTRIP_VOID);
	if (m_lspVB[_iVB] != NULL)
		return;

	CBSP2_VBInfo& VBCI = m_lVBInfo[_iVB];

	spCBSP2_VertexBuffer spVB = MNew(CBSP2_VertexBuffer);
	if (!spVB) MemError("CreateVertexBuffer");
	m_lspVB[_iVB] = spVB;

	spVB->Create(VBCI.m_nVertices, VBCI.m_Components);

	// Create the vertex data
	Tesselate(VBCI.m_liFaces.GetBasePtr(), VBCI.m_liFaces.Len(), VBCI.m_nVertices, 
		spVB->m_lV.GetBasePtr(), spVB->m_lTV1.GetBasePtr(), spVB->m_lTV2.GetBasePtr(), spVB->m_lCol.GetBasePtr(), spVB->m_lN.GetBasePtr(), spVB->m_lTangU.GetBasePtr(), spVB->m_lTangV.GetBasePtr(), spVB->m_lLMScale.GetBasePtr(), NULL, m_lspSurfaces[VBCI.m_iSurface]);
}

CBSP2_VertexBuffer* CXR_Model_BSP2::GetVertexBuffer(int _iVB)
{
	MAUTOSTRIP(CXR_Model_BSP_GetVertexBuffer, NULL);
	CBSP2_VertexBuffer* pBSPVB = m_lspVB[_iVB];
	if (!pBSPVB)
	{
		CreateVertexBuffer(_iVB);
		return m_lspVB[_iVB];
	}
	else
		return pBSPVB;
}

/*
static int IsPointColinear(const CVec3Dfp32* pV, uint32* piV, int nV, int iV)
{
	MAUTOSTRIP(IsPointColinear, 0);
	int iv0 = (iV <= 0) ? nV-1 : iV-1;
	int iv1 = iV;
	int iv2 = (iV >= nV-1) ? 0 : iV+1;

	CVec3Dfp32 E0;
	CVec3Dfp32 E1;
	pV[piV[iv1]].Sub(pV[piV[iv0]], E0);
	pV[piV[iv2]].Sub(pV[piV[iv1]], E1);
	E0.Normalize();
	E1.Normalize();
	if ((Abs(1.0f - E0*E1) < MODEL_BSP_EPSILON) &&
		((E0/E1).Length() < MODEL_BSP_EPSILON)) return 1;

	return 0;
}
*/
static int IsPointColinear(CVec3Dfp32 _N, const CVec3Dfp32* pV, uint32* piV, int nV, int iV)
{
	MAUTOSTRIP(IsPointColinear, 0);
	int iv0 = (iV <= 0) ? nV-1 : iV-1;
	int iv1 = iV;
	int iv2 = (iV >= nV-1) ? 0 : iV+1;

	CVec3Dfp32 E0;
	pV[piV[iv1]].Sub(pV[piV[iv0]], E0);

	CPlane3Dfp32 Plane;
	Plane.n = _N/E0;
	Plane.n.Normalize();
	Plane.d = -(Plane.n * pV[piV[iv1]]);

	fp32 Dist = Plane.Distance(pV[piV[iv2]]);

	if(M_Fabs(Dist) < MODEL_BSP_EPSILON)
		return 1;
	return 0;
}

void CXR_Model_BSP2::CreatePhysVertIndices()
{
	MAUTOSTRIP(CXR_Model_BSP_CreatePhysVertIndices, MAUTOSTRIP_VOID);
	MSCOPE(CreatePhysVertIndices, XR_BSPMODEL);

	TArrayAlign<uint32,16> liV;
	liV.SetGrow(m_lFaces.Len()*2+32);

	uint32 liVTemp[128];

	const CVec3Dfp32* pV = m_lVertices.GetBasePtr();
	int iVBase = m_liVertices.Len();
	for(int i = 0; i < m_lFaces.Len(); i++)
	{
		CBSP2_Face* pF = &m_lFaces[i];
		int nv = pF->m_nVertices;
		int iiv = pF->m_iiVertices;

		pF->m_nPhysV = 0;
		for(int v = 0; v < nv; v++)
		{
			if (!IsPointColinear(m_lPlanes[pF->m_iPlane].n, pV, &m_liVertices[iiv], nv, v))
			{
				liVTemp[pF->m_nPhysV] = m_liVertices[iiv + v];
				pF->m_nPhysV++;
			}
		}

		if (pF->m_nPhysV == pF->m_nVertices)
			pF->m_iiPhysV = pF->m_iiVertices;
		else
		{
			pF->m_iiPhysV = iVBase + liV.Len();
			for(int i = 0; i < pF->m_nPhysV; i++)
				liV.Add(liVTemp[i]);
		}


//		LogFile(CStrF("   V %d -> %d,   Gain  %d", nv, pF->m_nPhysV, nv - pF->m_nPhysV));
	}

	m_liVertices.Add(&liV);
	m_liVertices.OptimizeMemory();
	InitializeListPtrs();
}

void CXR_Model_BSP2::CreateFaceDiameters()
{
	MAUTOSTRIP(CXR_Model_BSP_CreateFaceDiameters, MAUTOSTRIP_VOID);
	MSCOPE(CreateFaceDiameters, XR_BSPMODEL);

	const CVec3Dfp32* pV = m_lVertices.GetBasePtr();
//	int iVBase = m_liVertices.Len();
	for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
	{
		CBSP2_Face* pF = &m_lFaces[iFace];
		int nv = pF->m_nPhysV;
		int iiv = pF->m_iiPhysV;

		const uint32* piV = &m_liVertices[iiv];

		// Bubble-bobble diameter calc
		fp32 MaxDSqr = 0.0f;

		for(int i = 0; i < nv-1; i++)
			for(int j = i+1; j < nv; j++)
			{
				fp32 DistSqr = pV[piV[i]].DistanceSqr(pV[piV[j]]);
				if (DistSqr > MaxDSqr)
					MaxDSqr = DistSqr;
			}

		fp32 Diam = M_Sqrt(MaxDSqr) - 0.00001f;

		int iDiam = 0;
		while(ms_lDiameterClasses[iDiam] < Diam && iDiam < MODEL_BSP_MAXDIAMETERCLASSES-1)
			iDiam++;

		pF->m_iDiameterClass = iDiam;
	}
}

void CXR_Model_BSP2::CreateLightDataSVArrays(CBSP2_LightData_Ext* _pLD)
{
	// Must be run after SplitSLCOnVBIndex
	_pLD->m_lSVThin.SetLen(_pLD->m_lSV.Len());
	_pLD->m_lSVBoundBoxSV.SetLen(_pLD->m_lSV.Len()+1);
	TAP<CBSP2_ShadowVolume> pSV = _pLD->m_lSV;
	TAP<CBSP2_ShadowVolume_Thin> pSVThin = _pLD->m_lSVThin;
	TAP<CBox3Dfp32> lSVBoundBoxSV = _pLD->m_lSVBoundBoxSV;

	for(int i = 0; i < pSV.Len(); i++)
	{
		pSVThin[i].Create(pSV[i]);
		lSVBoundBoxSV[i] = pSV[i].m_BoundBoxSV;
	}
}

class CBSP2_SQSortElem
{
public:
	CBSP2_ShaderQueueElement* m_pSQ;

	int Compare(const CBSP2_SQSortElem& _E) const
	{
		if (m_pSQ->m_iLight < _E.m_pSQ->m_iLight)
			return -1;
		else if (m_pSQ->m_iLight > _E.m_pSQ->m_iLight)
			return 1;
		else if (m_pSQ->m_iSurface < _E.m_pSQ->m_iSurface)
			return -1;
		else if (m_pSQ->m_iSurface > _E.m_pSQ->m_iSurface)
			return 1;
		else
			return 0;
	}
};

void CXR_Model_BSP2::SortShaderQueues(CBSP2_LightData_Ext* _pLD)
{
	TAP<CBSP2_ShaderQueueElement> lSQ = _pLD->m_lShaderQueue;

	TArray_Sortable<CBSP2_SQSortElem> lSort;
	lSort.SetLen(lSQ.Len());

	uint i;
	for(i = 0; i < lSQ.Len(); i++)
		lSort[i].m_pSQ = &lSQ[i];
	lSort.Sort();

	TThinArray<CBSP2_ShaderQueueElement> lSQNew;
	TThinArray<uint32> liSQRemap;
	lSQNew.SetLen(lSQ.Len());
	liSQRemap.SetLen(lSQ.Len());
	for(i = 0; i < lSQ.Len(); i++)
	{
		uint j = lSort[i].m_pSQ - lSQ.GetBasePtr();
		liSQRemap[j] = i;
		lSQNew[i] = lSQ[j];
	}

	TAP_RCD<CBSP2_SLCluster> lSLC = _pLD->m_lSLC;
	for(i = 0; i < lSLC.Len(); i++)
	{
		lSLC[i].m_iSQ = liSQRemap[lSLC[i].m_iSQ];
	}

	_pLD->m_lShaderQueue.ListSwap(lSQNew);
	lSQ = _pLD->m_lShaderQueue;

/*	M_TRACEALWAYS("ShaderQueue.Len() = %d\n", lSQ.Len());

	for(int iSQ = 0; iSQ < lSQ.Len(); iSQ++)
	{
		M_TRACEALWAYS("   SQ %d = (%.3d, %.3d)\n", lSQ[iSQ].m_iLight, lSQ[iSQ].m_iSurface);
	}*/
}


void CXR_Model_BSP2::CountVBFP(uint32* _piFaces, int _nFaces, int& _nVBFP, int& _nFacePrim)
{
	MAUTOSTRIP(CXR_Model_BSP_CountVBFP, MAUTOSTRIP_VOID);
//	VBSortFaceList(_piFaces, NULL, _nFaces);
	int iiFace = 0;
	while(iiFace < _nFaces)
	{
		int iVB = m_lFaces[_piFaces[iiFace]].m_iVB;
		int iiFace2 = iiFace;
		while(iiFace2 < _nFaces && m_lFaces[_piFaces[iiFace2]].m_iVB == iVB)
			iiFace2++;

		if (iVB >= 0 && 
			!(m_lspSurfaces[m_lVBInfo[iVB].m_iSurface]->m_Flags & (XW_SURFFLAGS_REFLECTION | XW_SURFFLAGS_SKYVIS | XW_SURFFLAGS_INVISIBLE)))
		{
			int nFaces = iiFace2 - iiFace;
			int nV = 0;
			int nPrim = 0;
#ifdef MODEL_BSP_USEKNITSTRIP
			RenderGetPrimCount_KnitStrip(&_piFaces[iiFace], nFaces, nV, nPrim);
#else
			RenderGetPrimCount(&_piFaces[iiFace], nFaces, nV, nPrim);
#endif

			_nFacePrim += nPrim;
			_nVBFP++;
		}

		iiFace = iiFace2;
	}
}

void CXR_Model_BSP2::CreateVBFP(uint32* _piFaces, int _nFaces, int& _iVBFP, int& _iFacePrim)
{
	MAUTOSTRIP(CXR_Model_BSP_CreateVBFP, MAUTOSTRIP_VOID);
//	const int MaxVBFP = 512;
//	CBSP2_VBFacePrim lVBFP[MaxVBFP];
//	int nVBFP = 0;

//	VBSortFaceList(_piFaces, NULL, _nFaces);
	int iiFace = 0;
	while(iiFace < _nFaces)
	{
		int iVB = m_lFaces[_piFaces[iiFace]].m_iVB;
		int iiFace2 = iiFace;
		while(iiFace2 < _nFaces && m_lFaces[_piFaces[iiFace2]].m_iVB == iVB)
			iiFace2++;

		if (iVB >= 0 && 
			!(m_lspSurfaces[m_lVBInfo[iVB].m_iSurface]->m_Flags & (XW_SURFFLAGS_REFLECTION | XW_SURFFLAGS_SKYVIS | XW_SURFFLAGS_INVISIBLE)))
		{
			int nFaces = iiFace2 - iiFace;
			int nV = 0;
			int nPrim = 0;
#ifdef MODEL_BSP_USEKNITSTRIP
			RenderGetPrimCount_KnitStrip(&_piFaces[iiFace], nFaces, nV, nPrim);
#else
			RenderGetPrimCount(&_piFaces[iiFace], nFaces, nV, nPrim);
#endif

			CBSP2_VBFacePrim& VBFP = m_lVBFacePrim[_iVBFP];
			VBFP.m_iPrim = _iFacePrim;
			VBFP.m_iVB = iVB;
			M_ASSERT(nPrim <= 0xffff, "Too many primitives.");
			VBFP.m_nPrim = nPrim;

			m_lVBInfo[iVB].m_MaxPrim += (2 + sizeof(void *) / 2);

			M_ASSERT(VBFP.m_iPrim + nPrim <= m_lFacePrim.Len(), "!");

#ifdef MODEL_BSP_USEKNITSTRIP
			RenderTesselatePrim_KnitStrip(&_piFaces[iiFace], nFaces, &m_lFacePrim[VBFP.m_iPrim]);
#else
			RenderTesselatePrim(&_piFaces[iiFace], nFaces, &m_lFacePrim[VBFP.m_iPrim]);
#endif
			_iFacePrim += nPrim;

			_iVBFP++;
		}

		iiFace = iiFace2;
	}

/*	if (nVBFP)
	{
		m_lVBFacePrim.SetLen(m_lVBFacePrim.Len() + nVBFP);
		memcpy(&m_lVBFacePrim[m_lVBFacePrim.Len() - nVBFP], lVBFP, nVBFP*sizeof(CBSP2_VBFacePrim));
	}*/
}

static void LogFace(CXR_Model_BSP2* _pThis, int _iFace)
{
	TAP_RCD<CBSP2_Face> pF = _pThis->m_lFaces;
	TAP_RCD<uint32> piV = _pThis->m_liVertices;
	TAP_RCD<CVec3Dfp32> pV = _pThis->m_lVertices;

	const CBSP2_Face& F = pF[_iFace];
	CStr s = CStrF("    Face %d, iVB %d, ", _iFace, F.m_iVB);

	for(int v = 0; v < F.m_nVertices; v++)
	{
		s += pV[piV[F.m_iiVertices + v]].GetString();
	}

	LogFile(s);
}

void CXR_Model_BSP2::SplitSLCOnVBIndex()
{
	// Must be run before CountSLCVB() and after CountVertexBuffer/PrepareVertexBuffer

	CBSP2_LightData* pLD = m_spLightData;
	if (!pLD)
		return;

	CMTime T; T.Start();

	int nSLC = pLD->m_lSLC.Len();
	int nSLCNew = 0;
	TAP_RCD<CBSP2_Face> pFaces = m_lFaces;

	// Two-pass coffee..  first count how much coffee is needed
	{
		TAP<CBSP2_SLCluster> pSLC = pLD->m_lSLC;
		TAP<uint32> piSLCFace = pLD->m_liSLCFaces;

		for(int iSLC = 0; iSLC < pSLC.Len(); iSLC++)
		{
			const CBSP2_SLCluster& SLC = pSLC[iSLC];
			int nF =  SLC.m_nFaces;
			int iiF = SLC.m_iiFaces;
			int iVB = pFaces[piSLCFace[iiF]].m_iVB;
			nSLCNew++;
			for(int f = 1; f < nF; f++)
			{
				int iF = piSLCFace[iiF + f];
				if (pFaces[iF].m_iVB != iVB)
				{
					iVB = pFaces[iF].m_iVB;
					nSLCNew++;
				}
			}
		}
	}

	XW_LIGHTDATATHINARRAY<CBSP2_SLCluster> lSLCNew;
	lSLCNew.SetLen(nSLCNew);

	TThinArray<int> liSLCRemap;
	TThinArray<int> lnSLCRemap;
	liSLCRemap.SetLen(nSLC);
	lnSLCRemap.SetLen(nSLC);

	// Brew the coffee
	{
		TAP<CBSP2_SLCluster> pSLC = pLD->m_lSLC;
		TAP<CBSP2_SLCluster> pSLCNew = lSLCNew;
		TAP<uint32> piSLCFaces = pLD->m_liSLCFaces;

		int iSLCNew = 0;
		for(int iSLC = 0; iSLC < pSLC.Len(); iSLC++)
		{
			const CBSP2_SLCluster& SLC = pSLC[iSLC];
			int nF =  SLC.m_nFaces;
			int iiF = SLC.m_iiFaces;
			int iVB = pFaces[piSLCFaces[iiF]].m_iVB;
			int nFNew = 1;
			int iiFBegin = iiF;
			liSLCRemap[iSLC] = iSLCNew;
			lnSLCRemap[iSLC] = 0;
			for(int f = 1; f < nF; f++)
			{
				int iF = piSLCFaces[iiF + f];
				if (pFaces[iF].m_iVB != iVB)
				{
					// Create SLC
					CBSP2_SLCluster& SLCNew = pSLCNew[iSLCNew];
					SLCNew.m_iSurface = SLC.m_iSurface;
					SLCNew.m_iLight = SLC.m_iLight;
					SLCNew.m_iSQ = SLC.m_iSQ;
//					SLCNew.m_BoundBox = SLC.m_BoundBox;
					SLCNew.m_iiFaces = iiFBegin;
					SLCNew.m_nFaces = nFNew;
					GetFaceListBoundBox(&piSLCFaces[iiFBegin], nFNew, SLCNew.m_BoundBox);

					lnSLCRemap[iSLC]++;

					iSLCNew++;
					nFNew = 0;
					iiFBegin = iiF + f;
					iVB = pFaces[iF].m_iVB;
				}
				nFNew++;
			}
			// Create SLC
			CBSP2_SLCluster& SLCNew = pSLCNew[iSLCNew];
			SLCNew.m_iSurface = SLC.m_iSurface;
			SLCNew.m_iLight = SLC.m_iLight;
			SLCNew.m_iSQ = SLC.m_iSQ;
//			SLCNew.m_BoundBox = SLC.m_BoundBox;
			SLCNew.m_iiFaces = iiFBegin;
			SLCNew.m_nFaces = nFNew;
			GetFaceListBoundBox(&piSLCFaces[iiFBegin], nFNew, SLCNew.m_BoundBox);

			lnSLCRemap[iSLC]++;

			iSLCNew++;
		}
	}

	// Remap SLC indices in "shadow volume" structure
	{
		TAP<CBSP2_ShadowVolume> pSV = pLD->m_lSV;
		for(int iSV = 0; iSV < pSV.Len(); iSV++)
		{
			CBSP2_ShadowVolume& SV = pSV[iSV];
			int nSLC = SV.m_nSLC;
			int iSLC = SV.m_iSLC;
			if (nSLC)
			{
				SV.m_iSLC = liSLCRemap[SV.m_iSLC];
				int nSLCNew = 0;
				for(int i = 0; i < nSLC; i++)
					nSLCNew += lnSLCRemap[iSLC+i];
				SV.m_nSLC = nSLCNew;
			}
		}
	}

	lSLCNew.ListSwap(pLD->m_lSLC);
	lSLCNew.Destroy();
	liSLCRemap.Destroy();
	lnSLCRemap.Destroy();

	T.Stop();

//	M_TRACEALWAYS("SplitSLCOnVBIndex: nSLCNew %d, nSLCOld %d, Rebuild time %f ms\n", nSLCNew, nSLC, T.GetTime() * 1000.0f);
}

void CXR_Model_BSP2::CountSLCVB()
{
	// This must be run before AllocVB()

	CBSP2_LightData* pLD = m_spLightData;
	if (!pLD)
		return;

	int nSLC = pLD->m_lSLC.Len();
	if (!nSLC)
		return;

	pLD->m_lSLCVBI.SetLen(nSLC);
	TAP<CBSP2_SLCluster> pSLC = pLD->m_lSLC;
	TAP<CBSP2_SLCVBInfo> pSLCVBI = pLD->m_lSLCVBI;
	TAP_RCD<CBSP2_Face> pFaces = m_lFaces;
	uint32* piSLCFaces = pLD->m_liSLCFaces.GetBasePtr();

	uint16 lnIBSLC[1024];
	uint16 liIBSLC[1024];
	uint16 lnIBPrim[1024];
	uint iIB = 0;
	uint iPrim = 0;
	uint nIBSLC = 0;

	liIBSLC[iIB] = 0;

	for(int i = 0; i < pSLCVBI.Len(); i++)
	{
		const CBSP2_SLCluster& SLC = pSLC[i];
		CBSP2_SLCVBInfo& SLCVBI = pSLCVBI[i];
		int nV, nPrim;
		RenderGetPrimCount_Triangles(&piSLCFaces[SLC.m_iiFaces], SLC.m_nFaces, nV, nPrim);

		uint iVB = pFaces[piSLCFaces[SLC.m_iiFaces]].m_iVB;
#ifndef M_Profile
		{
			for(int f = 0; f < SLC.m_nFaces; f++)
			{
				LogFace(this, piSLCFaces[SLC.m_iiFaces+f]);

				int iVBTest = pFaces[piSLCFaces[SLC.m_iiFaces+f]].m_iVB;
				if (iVBTest != iVB)
				{
					M_TRACEALWAYS("ERROR: Face %d in SLC %d does not use the intended VB, %d != %d\n", piSLCFaces[SLC.m_iiFaces+f], i, iVBTest, iVB);
					M_ASSERT(0, "(CXR_Model_BSP2::CountSLCVB) VB inconsistency.");
				}
			}
		}
#endif

		if (nPrim >= 0xffff)
		{
			Error("CountSLCVB", CStrF("Too many SLC primitives. (%d)", nPrim));
		}
		if (nPrim + iPrim >= 0xffff)
		{
			lnIBSLC[iIB] = nIBSLC;
			lnIBPrim[iIB] = iPrim;
			iIB++;
			if (iIB >= 1024)
				Error("CountSLCVB", "Out of temporary stack space.");
			liIBSLC[iIB] = i;
			iPrim = 0;
			nIBSLC = 0;
		}

		SLCVBI.m_VBID = ~0;
		SLCVBI.m_iPrimTri = iPrim;
		SLCVBI.m_nPrimTri = nPrim / 3;
		SLCVBI.m_IBID = iIB;
		iPrim += nPrim;
		nIBSLC++;
	}
	lnIBSLC[iIB] = nIBSLC;
	lnIBPrim[iIB] = iPrim;
	int nIB = iIB+1;

	{
		pLD->m_lSLCIBI.SetLen(nIB);
		TAP<CBSP2_SLCIBInfo> pIBI = pLD->m_lSLCIBI;
		for(int i = 0; i < pIBI.Len(); i++)
		{
			pIBI[i].m_IBID = 0;
			pIBI[i].m_nPrim = lnIBPrim[i];
			pIBI[i].m_iSLC = liIBSLC[i];
			pIBI[i].m_nSLC = lnIBSLC[i];
		}
	}
}

void CXR_Model_BSP2::PrepareSLCVB()
{
	// This must be run after AllocVB
	CBSP2_LightData* pLD = m_spLightData;
	if (!pLD)
		return;

	TAP<CBSP2_SLCluster> pSLC = pLD->m_lSLC;
	TAP<CBSP2_SLCVBInfo> pSLCVBI = pLD->m_lSLCVBI;
	TAP_RCD<CBSP2_SLCIBInfo> pSLCIBI = pLD->m_lSLCIBI;
	uint32* piSLCFaces = pLD->m_liSLCFaces.GetBasePtr();

	for(int i = 0; i < pSLCVBI.Len(); i++)
	{
		const CBSP2_SLCluster& SLC = pSLC[i];
		CBSP2_SLCVBInfo& SLCVBI = pSLCVBI[i];

		uint iVB = m_lFaces[piSLCFaces[SLC.m_iiFaces]].m_iVB;
		SLCVBI.m_VBID = m_lVBInfo[iVB].m_VBID;
		SLCVBI.m_IBID = pSLCIBI[SLCVBI.m_IBID].m_IBID;
	}
}

void CXR_Model_BSP2::CreatePortalLeafNodeIndices_r(TAP<CBSP2_Node> _lNodes, uint _iNode)
{
	const CBSP2_Node& Node = _lNodes[_iNode];
	if (Node.IsStructureLeaf())
	{
		m_lPortalLeaves[Node.m_iPortalLeaf].m_iNode = _iNode;
		return;
	}

	if(Node.IsNode())
	{
		CreatePortalLeafNodeIndices_r(_lNodes, Node.m_iNodeFront);
		CreatePortalLeafNodeIndices_r(_lNodes, Node.m_iNodeBack);
	}
}

void CXR_Model_BSP2::CreatePortalLeafNodeIndices()
{
	TAP<CBSP2_PortalLeafExt> lPL = m_lPortalLeaves;
	if (!lPL.Len())
		return;

	for(int iPL = 0; iPL < lPL.Len(); iPL++)
		lPL[iPL].m_iNode = 0;

	TAP<CBSP2_Node> lNodes = m_lNodes;
	CreatePortalLeafNodeIndices_r(lNodes, 1);
}


void CXR_Model_BSP2::Create_PostRead()
{
	MAUTOSTRIP(CXR_Model_BSP_Create_PostRead, MAUTOSTRIP_VOID);
	MSCOPE(Create_PostRead, XR_BSPMODEL);
#ifdef MODEL_BSP_CHECKTREE
	if (m_lPortalLeaves.Len())
		if (!CheckTree_r(1))
			Error("ModelChanged", "BSP-Check failed. (1)")
		else
			LogFile("(CXR_Model_BSP2::ModelChanged) BSP-Check 1 ok.");
#endif

	if (!m_spMaster)
	{
		if (m_lPortalLeaves.Len())
			Create_ViewInstances(MODEL_BSP_MAXVIEWS);
		else
			Create_ViewInstances(1);

	}

	PortalLeafSanityCheck("Create_PostRead 1");
	InitBound();
	InitNodePlaneTypes();

	CreatePhysVertIndices();
	CreateFaceDiameters();

	PortalLeafSanityCheck("Create_PostRead 2");
	{
		MSCOPE(TagLists, XR_BSPMODEL);
		if (!m_spMaster)
		{
/*			m_lFaceLightMask.SetLen(m_lFaces.Len());
			FillChar(&m_lFaceLightMask[0], m_lFaceLightMask.ListSize(), 0);
			m_lLightElemTagList.SetLen(Min(MODEL_BSP_MAXDYNAMICLIGHTFACES, m_lFaces.Len()) );
			m_lFaceTagList.SetLen((m_lFaces.Len()+31) >> 3);
			FillChar(m_lFaceTagList.GetBasePtr(), m_lFaceTagList.ListSize(), 0);
			m_lFaceUntagList.SetLen(m_lFaces.Len());
			m_nTagFaces = 0;
*/
			m_spEnumContextPool	= MNew(TObjectPool<CBSP2_EnumContext>);
			m_spEnumContextPool->Create(12);
		}
		else
		{
			m_spEnumContextPool	= m_spMaster->m_spEnumContextPool;
/*
			m_lFaceLightMask = m_spMaster->m_lFaceLightMask;
			m_lLightElemTagList = m_spMaster->m_lLightElemTagList;
			m_lFaceTagList = m_spMaster->m_lFaceTagList;
			m_lFaceUntagList = m_spMaster->m_lFaceUntagList;

			if (m_lFaceLightMask.Len() < m_lFaces.Len())
			{
				m_lFaceLightMask.SetLen(m_lFaces.Len());
				FillChar(&m_lFaceLightMask[0], m_lFaceLightMask.ListSize(), 0);
				m_lLightElemTagList.SetLen(Min(MODEL_BSP_MAXDYNAMICLIGHTFACES, m_lFaces.Len()) );
			}
			if (m_lFaceTagList.Len() < ((m_lFaces.Len()+31) >> 3))
			{
				m_lFaceTagList.SetLen((m_lFaces.Len()+31) >> 3);
				FillChar(m_lFaceTagList.GetBasePtr(), m_lFaceTagList.ListSize(), 0);
			}
			if (m_lFaceUntagList.Len() < m_lFaces.Len())
			{
				m_lFaceUntagList.SetLen(m_lFaces.Len());
			}
			m_nTagFaces = 0;
*/
		}
	}

	// Init backwards-references from portal-leaves to nodes.
	{
		uint nStructureLeaves = 0;
		
		TAP_RCD<CBSP2_Node> lNodes = m_lNodes;
		TAP_RCD<CBSP2_PortalLeafExt> lPL = m_lPortalLeaves;

		for(int iNode = 0; iNode < lNodes.Len(); iNode++)
			if (lNodes[iNode].m_Flags & XW_NODE_PORTAL)
			{
				if (lNodes[iNode].m_Flags & XW_NODE_STRUCTURE)
				{
					int iPL = lNodes[iNode].m_iPortalLeaf;

					if (lPL[iPL].m_iNode != iNode)
						M_TRACEALWAYS("WARNING: something is wrong in the BSP loader (this used to be a very descriptive 'wtf'-assert, which was replaced with at trace...\n");

//					lPL[iPL].m_iNode = iNode;
					nStructureLeaves = MaxMT(iPL+1, nStructureLeaves);
				}
	//			if (m_lNodes[iNode].m_Flags & XW_NODE_NAVIGATION)
	//				m_lPortalLeaves[m_lNodes[iNode].m_iNavigationLeaf].m_iNode = iNode;
			}
		m_nStructureLeaves = nStructureLeaves;
	}


	// Dump nodes and portal-leaves to logfile.
/*	{
		for(int i = 0; i < m_lNodes.Len(); i++)
		{
			CBSP2_Node* pN = &m_lNodes[i];
			if (pN->IsLeaf())
				LogFile(CStrF("Parent %.3d <- this %.3d ->   -/-  , Flg %.4x, PL %.3d, NL %.3d", pN->m_iNodeParent, i, pN->m_Flags, pN->m_iPortalLeaf, pN->m_iNavigationLeaf));
			else
				LogFile(CStrF("Parent %.3d <- this %.3d -> %.3d/%.3d, Flg %.4x, PL %.3d, NL %.3d", pN->m_iNodeParent, i, pN->m_iNodeFront, pN->m_iNodeBack, pN->m_Flags, pN->m_iPortalLeaf, pN->m_iNavigationLeaf));
		}

		for(i = 0; i < m_lPortalLeaves.Len(); i++)
		{
			CBSP2_PortalLeaf* pPL = &m_lPortalLeaves[i];
			LogFile(CStrF("PL %.3d, Flg %.4x, Node %.3d, Medium %.8x", i, pPL->m_Flags, pPL->m_iNode, pPL->m_ContainsMedium));
		}

		for(i = 0; i < m_lPortals.Len(); i++)
		{
			CBSP2_PortalExt* pP = &m_lPortals[i];
			LogFile(CStrF("P %.3d, ID %d, Flg %.4x, NodeF %.3d, NodeB %.3d", i, pP->m_PortalID, pP->m_Flags, pP->m_iNodeFront, pP->m_iNodeBack));
		}
	}
*/


	// Init portal-leaves face bounding-box.
/*	int iPL;
	for(iPL = 0; iPL < m_lPortalLeaves.Len(); iPL++)
	{
		m_lPortalLeaves[iPL].m_FaceBoundBox.m_Min = _FP32_MAX;
		m_lPortalLeaves[iPL].m_FaceBoundBox.m_Max = -_FP32_MAX;
		int iNode = m_lPortalLeaves[iPL].m_iNode;
		if (iNode)
			ExpandFaceBoundBox_r(m_lPortalLeaves[iPL].m_iNode, m_lPortalLeaves[iPL].m_FaceBoundBox);
		else
		{
			m_lPortalLeaves[iPL].m_FaceBoundBox.m_Min = 0;
			m_lPortalLeaves[iPL].m_FaceBoundBox.m_Max = 0;
		}
	}*/

	PortalLeafSanityCheck("Create_PostRead 3");
	{
		MSCOPE(EnumFaces, XR_BSPMODEL);
		int nEnumFaces = m_liFaces.Len();
		TArray<uint32> liFaces;
		liFaces.SetLen(nEnumFaces);

		// Build face-lists for portal-leaves
		{
			MSCOPE(PLFaceLists, XR_BSPMODEL);

			if (!m_lPortalLeaves.Len())
			{
				// This is not a portalized BSP-model

				// Add the reflection-faces to lRefFaces
				int nRefF = GetFaceList_r(1, liFaces.GetBasePtr(), 0, nEnumFaces, -1, XW_SURFFLAGS_REFLECTION);
				m_liRefFaces.SetLen(nRefF);
				SortFaceList(liFaces.GetBasePtr(), m_liRefFaces.GetBasePtr(), nRefF);
			}
			else
			{
				// Count special face occurances so we don't have to use arrays dynamically
				int nRefFaces = 0;
				int nSkyFaces = 0;
				for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
				{
					if (m_lspSurfaces[m_lFaces[iFace].m_iSurface]->m_Flags & XW_SURFFLAGS_REFLECTION)
						nRefFaces++;
					if (m_lspSurfaces[m_lFaces[iFace].m_iSurface]->m_Flags & XW_SURFFLAGS_SKYVIS)
						nSkyFaces++;
				}

				SceneGraph_PVSInitCache(32);
				
//				int iVB = 0;
//				int iQueue = 0;

				m_liPLFaces.SetLen(m_liFaces.Len() + nRefFaces + nSkyFaces);	// These should be of same length if everything's alright.
				int niPLF = 0;
				for(uint iPL = 0; iPL < m_lPortalLeaves.Len(); iPL++)
				{
					// Only gather faces for structure-leaves.
					if (!(m_lPortalLeaves[iPL].m_Flags & XW_PORTALLEAF_STRUCTURE)) continue;

					// Reflection-faces
					{
						int nRefF = GetFaceList_r(m_lPortalLeaves[iPL].m_iNode, liFaces.GetBasePtr(), 0, nEnumFaces, -1, XW_SURFFLAGS_REFLECTION);
						if (nRefF) SortFaceList(liFaces.GetBasePtr(), &m_liPLFaces[niPLF], nRefF);
						m_lPortalLeaves[iPL].m_iiRefFaces = (nRefF) ? niPLF : 0;
						m_lPortalLeaves[iPL].m_nRefFaces = nRefF;
						niPLF += nRefF;
					}

					// Sky-faces
					{
						int nSkyF = GetFaceList_r(m_lPortalLeaves[iPL].m_iNode, liFaces.GetBasePtr(), 0, nEnumFaces, -1, XW_SURFFLAGS_SKYVIS);
						if (nSkyF) SortFaceList(liFaces.GetBasePtr(), &m_liPLFaces[niPLF], nSkyF);
						m_lPortalLeaves[iPL].m_iiSkyFaces = (nSkyF) ? niPLF : 0;
						m_lPortalLeaves[iPL].m_nSkyFaces = nSkyF;
		//		LogFile(CStrF("(CXR_Model_BSP2::ModelChanged) nSky %d, iSky %d", nSkyF, niPLF));
						niPLF += nSkyF;
					}

					int nF = GetFaceList_r(m_lPortalLeaves[iPL].m_iNode, liFaces.GetBasePtr(), 0, nEnumFaces);
					if (nF)
					{
						if (nF + niPLF > m_liPLFaces.Len()) Error("ModelChanged", CStrF("Internal error. (m_liPLFaces overrun. %d+%d > %d, nF %d)", nF, niPLF, m_liPLFaces.Len(), m_liFaces.Len()));
						if (nF) SortFaceList(liFaces.GetBasePtr(), &m_liPLFaces[niPLF], nF);
						m_lPortalLeaves[iPL].m_iiFaces = niPLF;
						m_lPortalLeaves[iPL].m_nFaces = nF;
						niPLF += nF;
					}
					else
					{
						m_lPortalLeaves[iPL].m_iiFaces = 0;
						m_lPortalLeaves[iPL].m_nFaces = 0;
					}
				}

			}
		}

		PortalLeafSanityCheck("Create_PostRead 4");
		// Create vertex buffers
		{
			MSCOPE(CreateVBs, XR_BSPMODEL);

			// -------------------------------------------------------------------
			// Get faces
			TObjectPoolAllocator<CBSP2_EnumContext> EnumAlloc(m_spEnumContextPool);
			CBSP2_EnumContext* pEnumContext = EnumAlloc.GetObject();
			pEnumContext->Create(m_lFaces.Len(), 0);
			pEnumContext->SetupFaceEnum(liFaces.GetBasePtr(), nEnumFaces);
			CXR_PhysicsContext PhysContext;
			int nF = EnumFaces_All(&PhysContext, pEnumContext, 1);
			pEnumContext->Untag();

//			int nF = GetFaceList_r(1, liFaces.GetBasePtr(), 0, nEnumFaces);
//			M_ASSERT(liFaces.ListSize() >= m_liFaces.ListSize(), "!");
//			memcpy(liFaces.GetBasePtr(), m_liFaces.GetBasePtr(), m_liFaces.ListSize());
			uint32* piFaces = liFaces.GetBasePtr();
//			int nF = m_liFaces.Len();

			{
				MSCOPESHORT(SortFaceList);
				SortFaceList(piFaces, piFaces, nF);
			}

			// -------------------------------------------------------------------
			// Count vertex buffer stuff
			int nVB = 0;
			{
				MSCOPESHORT(CountVertexBuffer);

				int k = 0;
				while(k < nF)
				{
					int i = k;

					if (k < nF)
						while(i < nF && !CompareFaces(piFaces[k], piFaces[i])) i++;

					CountVertexBuffer(&piFaces[k], i - k, nVB);

					k = i;
				}
			}

			// -------------------------------------------------------------------
			// Prepare vertex buffers
			m_lspVB.SetLen(nVB);
			m_lVBOffsets.SetLen(nVB);
			m_lVBInfo.SetLen(nVB);
			m_lSurfQueues.SetGrow(m_lspVB.Len());


			int iVB = 0;
			{
				MSCOPESHORT(PrepareVertexBuffer);

				int k = 0;
				while(k < nF)
				{
					int i = k;

					if (k < nF)
					{
						while(i < nF && !CompareFaces(piFaces[k], piFaces[i])) i++;
					}

					PrepareVertexBuffer(&piFaces[k], i - k, iVB);

					k = i;
				}
			}

			M_ASSERT(iVB == nVB, "!");

//			m_lSBPrim.OptimizeMemory();

			// -------------------------------------------------------------------
	//		m_lSurfQueues.SetLen(iQueue);
	//		LogFile("-------------------------------------------------------------------");
	//		LogFile(CStrF("(CXR_Model_BSP2::Read) %d VB Queues.", m_lSurfQueues.Len() ));
	//		LogFile(CStrF("(CXR_Model_BSP2::Read) %d VBs.", m_lspVB.Len() ));
	//		LogFile(CStrF("(CXR_Model_BSP2::Read) %d VB Surface clusters.", m_lVBSurfClusters.Len() ));
	//		LogFile("-------------------------------------------------------------------");

		}

		PortalLeafSanityCheck("Create_PostRead 5");
		// -------------------------------------------------------------------
		// Must be run before CountSLCVB()
		if (m_spLightData != NULL)
		{
			SplitSLCOnVBIndex();
			CreateLightDataSVArrays(m_spLightData);	// Must be run after SplitSLCOnVBIndex
			SortShaderQueues(m_spLightData);
		}
		PortalLeafSanityCheck("Create_PostRead 6");

		// -------------------------------------------------------------------
		// Must be run before VB_AllocVBID
		if (m_spLightData != NULL)
			CountSLCVB();

		// -------------------------------------------------------------------
		{
			MSCOPESHORT(VB_AllocVBID);
			VB_AllocVBID();
		}

		PortalLeafSanityCheck("Create_PostRead 7");
		// -------------------------------------------------------------------
		// Build portal-leaf VBFacePrims
		if (m_lPortalLeaves.Len())
		{
			MSCOPE(CreateVBFP, XR_BSPMODEL);

			int nVBFP = 0;
			int nFacePrim = 0;
			{
				for(int iPL = 0; iPL < m_nStructureLeaves; iPL++)
				{
//					CBSP2_PortalLeafExt& PL = m_lPortalLeaves[iPL];
					int nFaces = GetPLFaceList_r(iPL, liFaces.GetBasePtr(), nEnumFaces, -1, -1);
					if (nFaces)
					{
						CBSP2_PortalLeafExt& PL = m_lPortalLeaves[iPL];
						for(uint iiF = 0; iiF < nFaces; iiF++)
						{
							uint iFace = liFaces[iiF];
							if (m_lspSurfaces[m_lFaces[iFace].m_iSurface]->m_Flags & (XW_SURFFLAGS_REFLECTION || XW_SURFFLAGS_SKYVIS))
								continue;

							CBox3Dfp32 FaceBox;
							GetFaceBoundBox(iFace, FaceBox);
							if ((FaceBox.m_Min[0] < PL.m_BoundBox.m_Min[0]-1.0f) ||
								(FaceBox.m_Min[1] < PL.m_BoundBox.m_Min[1]-1.0f) ||
								(FaceBox.m_Min[2] < PL.m_BoundBox.m_Min[2]-1.0f) ||
								(FaceBox.m_Max[0] > PL.m_BoundBox.m_Max[0]+1.0f) ||
								(FaceBox.m_Max[1] > PL.m_BoundBox.m_Max[1]+1.0f) ||
								(FaceBox.m_Max[2] > PL.m_BoundBox.m_Max[2]+1.0f))
								M_TRACEALWAYS("Face %d outside PL %d bound. %s, %s\n", iFace, iPL, FaceBox.GetString().Str(), PL.m_BoundBox.GetString().Str());

							const uint32* piPLFaces = &m_liPLFaces[PL.m_iiFaces];
							uint nPLFaces = PL.m_nFaces;
							uint nFound = 0;
							for(uint i = 0; i < nPLFaces; i++)
							{
								if (piPLFaces[i] == iFace)
									nFound++;
							}
							if (nFound != 1)
							{
								M_TRACEALWAYS("Face %d found %d times in PL %d\n", iFace, nFound, iPL);
							}
						}




						VBSortFaceList(liFaces.GetBasePtr(), NULL, nFaces);
						CountVBFP(liFaces.GetBasePtr(), nFaces, nVBFP, nFacePrim);
			        }
		        }
			}

			m_lFacePrim.SetLen(nFacePrim);
			m_lVBFacePrim.SetLen(nVBFP);

			int iVBFP = 0;
			int iFacePrim = 0;
			for(int iPL = 0; iPL < m_nStructureLeaves; iPL++)
			{
				CBSP2_PortalLeafExt& PL = m_lPortalLeaves[iPL];
				CBSP2_PortalLeaf_VBFP& PLVBFP = m_lPortalLeaves_VBFP[iPL];
				int nFaces = GetPLFaceList_r(iPL, liFaces.GetBasePtr(), nEnumFaces, -1, -1);
				if (nFaces)
				{
					PLVBFP.m_iVBFacePrims = iVBFP;
					VBSortFaceList(liFaces.GetBasePtr(), NULL, nFaces);
					CreateVBFP(liFaces.GetBasePtr(), nFaces, iVBFP, iFacePrim);
					PLVBFP.m_nVBFacePrims = iVBFP - PLVBFP.m_iVBFacePrims;
					if (!PLVBFP.m_nVBFacePrims)
						PLVBFP.m_iVBFacePrims = 0;
				}
			}

			M_ASSERT(iVBFP == nVBFP, "!");
			M_ASSERT(iFacePrim == nFacePrim, "!");

			// Debug
	/*		for(int iPL = 0; iPL < m_nStructureLeaves; iPL++)
			{
				CBSP2_PortalLeafExt& PL = m_lPortalLeaves[iPL];
				LogFile(CStrF("PORTALLEAF %d, iVBFacePrims %d, nVBFacePrims %d", iPL, PL.m_iVBFacePrims, PL.m_nVBFacePrims));

				for(int i = 0; i < PL.m_nVBFacePrims; i++)
				{
					const CBSP2_VBFacePrim& VBFP = m_lVBFacePrim[PL.m_iVBFacePrims + i];
					LogFile(CStrF("    iVB %d, iPrim %d, nPrim %d", VBFP.m_iVB, VBFP.m_iPrim, VBFP.m_nPrim));
				}
			}*/
		}
		else
		{
			MSCOPE(CreateVBFP, XR_BSPMODEL);

			int nFaces = GetFaceList_r(1, liFaces.GetBasePtr(), 0, nEnumFaces, -1, -1);
			if (nFaces)
			{
				VBSortFaceList(liFaces.GetBasePtr(), NULL, nFaces);
				int nVBFP = 0;
				int nFacePrim = 0;
				CountVBFP(liFaces.GetBasePtr(), nFaces, nVBFP, nFacePrim);
				m_lFacePrim.SetLen(nFacePrim);
				m_lVBFacePrim.SetLen(nVBFP);

				int iVBFP = 0;
				int iFacePrim = 0;
				CreateVBFP(liFaces.GetBasePtr(), nFaces, iVBFP, iFacePrim);

				M_ASSERT(iVBFP == nVBFP, "!");
				M_ASSERT(iFacePrim == nFacePrim, "!");
			}
		}
	}

	// Init iBaseVertex & nVertices
	uint32 MaxPLVerts = 0;
	uint iPL = 0;
	for(iPL = 0; iPL < m_lPortalLeaves.Len(); iPL++)
	{
		CBSP2_PortalLeafExt* pPL = &m_lPortalLeaves[iPL];
		if (pPL->m_nFaces)
		{
			uint32 iBase = m_lVertices.Len()-1;
			uint32 iMax = 0;
			for(int iiFace = 0; iiFace < pPL->m_nFaces; iiFace++)
			{
				CBSP2_Face* pF = &m_lFaces[m_liPLFaces[pPL->m_iiFaces + iiFace]];
				for(int v = 0; v < pF->m_nVertices; v++)
				{
					iBase = Min(iBase, (uint32)m_liVertices[pF->m_iiVertices + v]);
					iMax = Max(iMax, (uint32)m_liVertices[pF->m_iiVertices + v]);
				}
			}

			pPL->m_iBaseVertex = iBase;
			pPL->m_nVertices = iMax - iBase + 1;
		}
		else
		{
			pPL->m_iBaseVertex = 0;
			pPL->m_nVertices = 0;
		}

		MaxPLVerts = Max(MaxPLVerts, pPL->m_nVertices);
//LogFile(CStrF("iBase %d, nV %d, nF %d", pPL->m_iBaseVertex, pPL->m_nVertices, pPL->m_nFaces));
	}

	// Build fog-boxes
	{
		MSCOPE(FogStuff, XR_BSPMODEL);
		{
			m_lspFogBoxes.Clear();
			m_lspFogBoxes.Add(spCBSP2_FogBox(NULL));
			for(int iPL = 0; iPL < m_lPortalLeaves.Len(); iPL++)
			{
				CBSP2_PortalLeafExt* pPL = &m_lPortalLeaves[iPL];
				if (pPL->m_Flags & XW_PORTALLEAF_STRUCTURE)
				{
					if (pPL->m_ContainsMedium & XW_MEDIUM_FOG)
						pPL->m_ContainsMedium |= XW_MEDIUM_CLIPFOG;

//					if ((pPL->m_ContainsMedium & (XW_MEDIUM_FOG | XW_MEDIUM_LITFOG)) == (XW_MEDIUM_FOG | XW_MEDIUM_LITFOG))
//						Fog_InitFogBox(iPL);
				}
			}
		}

		if (!m_lPortalLeaves.Len())
			MaxPLVerts = m_lVertices.Len();

		if (m_spMaster != NULL)
		{
			m_lVertexFog = m_spMaster->m_lVertexFog;
			m_lFogTags = m_spMaster->m_lFogTags;
			m_liFogTags = m_spMaster->m_liFogTags;
		}

		if (m_lVertexFog.Len() < MaxPLVerts)
		{
			m_lVertexFog.SetLen(MaxPLVerts);
			m_lFogTags.SetLen(MaxPLVerts);
			m_liFogTags.SetLen(MaxPLVerts);
			FillChar(m_lFogTags.GetBasePtr(), m_lFogTags.ListSize(), 0);
			FillChar(m_lVertexFog.GetBasePtr(), m_lVertexFog.ListSize(), 0);
		}

		m_MaxFogTags = m_liFogTags.Len();
		m_nFogTags = 0;
	}


	// Setup rendering-queues.
	{
		MSCOPE(m_lFaceQueueHeap, XR_BSPMODEL);

		if(!m_lspVB.Len())
		{
			Error("Create_PostRead", "Empty BSP model.");
	/*		m_lspVB.SetLen(1);
			m_lspVB[0] = DNew(CBSP2_VertexBuffer) CBSP2_VertexBuffer;
			if (!m_lspVB[0])
				MemError("Create_PostRead");
			m_lspVB[0]->Create(0);*/
		}
		m_lVBFaceQueues.SetLen(m_lspVB.Len());

	//	m_lSurfaceQueues.SetLen(m_lspSurfaces.Len());

		// Count occurances of each vertex-buffer
		{
			for(int i = 0; i < m_lFaces.Len(); i++)
				m_lVBFaceQueues[m_lFaces[i].m_iVB].m_MaxElem++;
		}

		// Assign queue-space for each vertex buffer
		{
//			m_lFaceQueueHeap.SetLen(m_lFaces.Len());
			uint Pos = 0;
			for(uint i = 0; i < m_lVBFaceQueues.Len(); i++)
			{
				if (m_lVBFaceQueues[i].m_MaxElem)
				{
//					m_lVBFaceQueues[i].m_Offset = Pos;
					Pos += m_lVBFaceQueues[i].m_MaxElem;
				}
			}
			if (Pos != m_lFaces.Len())
				Error("ModelChanged", "Internal error.");
		}
	}

	// -------------------------------------------------------------------
	// Must be run after VB_AllocVBID
	if (m_spLightData != NULL)
		PrepareSLCVB();

	// Dump SB-linkcontext
/*	{
		LogFile("---------------------------------------------------------");
		CBSP2_LinkContext* pLC = m_spSBLink;
		for(int iPL = 0; iPL < m_lPortalLeaves.Len(); iPL++)
		{
			uint16 IDs[512];
			int nIDs = pLC->EnumIDs(iPL, IDs, 512);
			CStr s = CStrF("iPL %d ", iPL);
			for(int i = 0; i < nIDs; i++);
				s += CStrF(", %d", IDs[i]);
			LogFile(s);
		}

		LogFile("---------------------------------------------------------");
		for(int ID = 0; ID < m_lspSplineBrushes.Len(); ID++)
		{
			uint16 PLs[512];
			int nPL = pLC->EnumPortalLeaves(ID, PLs, 512);
			CStr s = CStrF("ID %d ", ID);
			for(int i = 0; i < nPL; i++);
				s += CStrF(", %d", PLs[i]);
			LogFile(s);
		}
		LogFile("---------------------------------------------------------");
	}*/

	// URGENTFIXME: We shouldn't be logging this unless we're debugging stuff....
/*	if (m_lPortalLeaves.Len())
	{
		for(int iNode = 0; iNode < m_lNodes.Len(); iNode++)
		{
			CBSP2_Node* pN = &m_lNodes[iNode];
			if (pN->IsLeaf())
			{
				int nF = pN->m_nFaces;
				int iiF = pN->m_iiFaces;
				LogFile(CStrF("PRECLEAN NODE %d, nF %d, iiF %d", iNode, nF, iiF));
				for(int i = 0; i < nF; i++)
				{
					CBSP2_Face* pF = &m_lFaces[m_liFaces[iiF + i]];
					CStr s;
					int nv = pF->m_nVertices;
					int iiv = pF->m_iiVertices;
					for(int v = 0; v < nv; v++)
						s += m_lVertices[m_liVertices[iiv + v]].GetFilteredString() + ",    ";
					LogFile(CStrF("       Face %d, nV %d, iiV %d, P %s            %s", m_liFaces[iiF + i], nv, iiv, (char*)m_lPlanes[pF->m_iPlane].GetString(), (char*)s ));
				}
			}
		}
	}*/
}

#define CHECK_LD(Var1, Expr, Var2, Msg)	\
	if (Var1 Expr Var2)	\
	{	M_TRACEALWAYS(CStrF("        "#Var1"(%d) "#Expr" "#Var2"(%d) \n"#Msg, Var1, Var2)); bErr = true; };
		

void CXR_Model_BSP2::LightDataSanityCheck()
{
	CBSP2_LightData* pLD = m_spLightData;
	if (!pLD)
		return;

	//LogFile("(CXR_Model_BSP2::LightDataSanityCheck) Begin...");

	bool bErr = false;

#ifndef PLATFORM_CONSOLE
	int bXDF = D_MXDFCREATE;
	int Platform = D_MPLATFORM;

	bool bDelayLoad = false;

	if (bXDF && Platform != 0)
	{
		bDelayLoad = true;
	}
#endif

	// Check m_lSV
	{
		for(int i = 0; i < pLD->m_lSV.Len(); i++)
		{
			const CBSP2_ShadowVolume& SV = pLD->m_lSV[i];

			CHECK_LD(SV.m_iPL, >=, m_lPortalLeaves.Len(), "");
			CHECK_LD(SV.m_iLight, >=, pLD->m_lLights.Len(), "");
#ifndef PLATFORM_CONSOLE
			if (!bDelayLoad)
			{
				CHECK_LD(SV.m_iVBase+SV.m_nVertices, >, pLD->m_lSVVertices.Len(), "");
				CHECK_LD(SV.m_iTriBase+SV.m_nTriangles*3, >, pLD->m_liSVPrim.Len(), "");
			}
#endif
			CHECK_LD(SV.m_iSLC+SV.m_nSLC, >, pLD->m_lSLC.Len(), "");


/*		uint16 m_iPL;
		uint16 m_iLight;
		uint32 m_iVBase;
		uint32 m_iTriBase;
		uint16 m_nVertices;
		uint16 m_nTriangles;

		uint16 m_iSVNextPL;			// Single link list, points to next PL for m_iLight
		uint16 m_iSVNextLight;		// Single link list, points to next light for m_iPL

		CBox3Dfp32 m_BoundBoxSV;
		CBox3Dfp32 m_BoundBoxLight;

		uint16 m_iSLC;				// Index of first Surface/Light cluster (CBSP2_SLCluster)
		uint16 m_nSLC;
			if (*/
		}
	}

	if (bErr)
		Error("LightDataSanityCheck", "Light data is insane!");

	//LogFile("(CXR_Model_BSP2::LightDataSanityCheck) Done.");
}

void CXR_Model_BSP2::GetFaceList_DebugSlow_r(uint _iNode, TArray<uint>& _liFaces)
{
	// NOTE: This one will return duplicate indices if faces are in multiple leaves.

	const CBSP2_Node& Node = m_lNodes[_iNode];

	if (Node.IsLeaf())
	{
		TAP_RCD<uint32> piFaces = m_liFaces;
		int iiF = Node.m_iiFaces;
		int nF = Node.m_nFaces;

		for(int i = 0; i < nF; i++)
		{
			int iFace = piFaces[iiF + i];
			const CBSP2_Face& Face = m_lFaces[iFace];
			if (Face.m_nVertices > 2)
			{
				_liFaces.Add(iFace);
			}
		}
	}

	if (Node.IsNode())
	{
		GetFaceList_DebugSlow_r(Node.m_iNodeBack, _liFaces);
		GetFaceList_DebugSlow_r(Node.m_iNodeFront, _liFaces);
	}
}



void CXR_Model_BSP2::PortalLeafSanityCheck(const char* _pLocation)
{
	if (!m_lPortalLeaves.Len())
		return;
	M_TRACEALWAYS("-------------------------------------------------------------------------\n");
	M_TRACEALWAYS("PortalLeafSanityCheck (%s)\n", _pLocation);

	TArray<uint> liFaces;
	liFaces.SetGrow(m_lFaces.Len());
	for(uint iPL = 0; iPL < m_lPortalLeaves.Len(); iPL++)
	{
		const CBSP2_PortalLeaf& PL = m_lPortalLeaves[iPL];
		liFaces.QuickSetLen(0);
		GetFaceList_DebugSlow_r(PL.m_iNode, liFaces);

		uint nFaces = liFaces.Len();
		for(uint iiF = 0; iiF < nFaces; iiF++)
		{
			uint iFace = liFaces[iiF];
			if (m_lspSurfaces[m_lFaces[iFace].m_iSurface]->m_Flags & (XW_SURFFLAGS_REFLECTION || XW_SURFFLAGS_SKYVIS))
				continue;

			CBox3Dfp32 FaceBox;
			GetFaceBoundBox(iFace, FaceBox);
			if ((FaceBox.m_Min[0] < PL.m_BoundBox.m_Min[0]-1.0f) ||
				(FaceBox.m_Min[1] < PL.m_BoundBox.m_Min[1]-1.0f) ||
				(FaceBox.m_Min[2] < PL.m_BoundBox.m_Min[2]-1.0f) ||
				(FaceBox.m_Max[0] > PL.m_BoundBox.m_Max[0]+1.0f) ||
				(FaceBox.m_Max[1] > PL.m_BoundBox.m_Max[1]+1.0f) ||
				(FaceBox.m_Max[2] > PL.m_BoundBox.m_Max[2]+1.0f))
				M_TRACEALWAYS("Face %d outside PL %d bound. %s, %s\n", iFace, iPL, FaceBox.GetString().Str(), PL.m_BoundBox.GetString().Str());
		}

	}
	M_TRACEALWAYS("-------------------------------------------------------------------------\n");
}

void CXR_Model_BSP2::LightDataFixup()
{
	CBSP2_LightData* pLD = m_spLightData;
	if (!pLD)
		return;

	CBSP2_ShadowVolume* pSV = pLD->m_lSV.GetBasePtr();
	int nSV = pLD->m_lSV.Len();

	for(int iSV = 0; iSV < nSV; iSV++)
	{
		if (pSV[iSV].m_BoundBoxLight.m_Min[0] > pSV[iSV].m_BoundBoxLight.m_Max[0] ||
			pSV[iSV].m_BoundBoxLight.m_Min[1] > pSV[iSV].m_BoundBoxLight.m_Max[1] ||
			pSV[iSV].m_BoundBoxLight.m_Min[2] > pSV[iSV].m_BoundBoxLight.m_Max[2])
		{
			pSV[iSV].m_BoundBoxLight = m_lPortalLeaves[pSV[iSV].m_iPL].m_BoundBox;
		}
	}

}

void CXR_Model_BSP2::CreateSurfaceShaderParams()
{
	TAP<spCXW_Surface> lspSurf = m_lspSurfaces;
	m_lSurfaceShaderParams.SetLen(lspSurf.Len());
	TAP<CXR_SurfaceShaderParams> lSSP = m_lSurfaceShaderParams;

	for(uint iSurf = 0; iSurf < lspSurf.Len(); iSurf++)
	{
		CXW_Surface* pSurf = lspSurf[iSurf];
		if (pSurf->m_Flags  & XW_SURFFLAGS_SHADER)
			lSSP[iSurf].Create(pSurf, pSurf->GetBaseFrame());
		else
			lSSP[iSurf].Clear();
	}
}

void CXR_Model_BSP2::InitBound()
{
	MAUTOSTRIP(CXR_Model_BSP_InitBound, MAUTOSTRIP_VOID);
	CVec3Dfp32* pV = m_lVertices.GetBasePtr();

	// Init bound-radius
	int nv = m_lVertices.Len();
	m_BoundRadius = 0;
	for(int v = 0; v < nv; v++)
	{
		fp32 l = pV[v].Length();
		if (l > m_BoundRadius) m_BoundRadius = l;
	}
	m_BoundRadius += 0.0001f;

	// Init bound-box
	CVec3Dfp32::GetMinBoundBox(pV, m_BoundBox.m_Min, m_BoundBox.m_Max, nv);

	m_BoundBox.m_Min -= CVec3Dfp32(0.0001f);
	m_BoundBox.m_Max += CVec3Dfp32(0.0001f);
}

void CXR_Model_BSP2::InitNodePlaneTypes()
{
	MAUTOSTRIP(CXR_Model_BSP_InitNodePlaneTypes, MAUTOSTRIP_VOID);
	int nPlanes = 0;
	int nOrt = 0;

	for(int i = 0; i < m_lNodes.Len(); i++)
	{
		CBSP2_Node* pN = &m_lNodes[i];
		if (pN->IsNode())
		{
			nPlanes++;

			int iPlane = pN->m_iPlane;
			CPlane3Dfp32* pP = &m_lPlanes[iPlane];

			int iType = 0;
			for(int k = 0; k < 3; k++)
			{
				if (Abs(pP->n.k[k] - 1.0f) < 0.00000001f)
					iType = k+1;
				else if (Abs(pP->n.k[k] + 1.0f) < 0.00000001f)
					iType = k+1+4;
			}
			if (iType) nOrt++;

			pN->m_Flags |= iType << XW_NODE_PLANETYPESHIFT;
		}
	}

//	if (m_lFaces.Len() > 500)
//		ConOut(CStrF("%d of %d planes were axis-aligned.", nOrt, nPlanes));
}

void CXR_Model_BSP2::Clear()
{
	MAUTOSTRIP(CXR_Model_BSP_Clear, MAUTOSTRIP_VOID);
	Error("Clear", "Not implemented.");
}

void CXR_Model_BSP2::Create_ViewInstances(int _nInst)
{
	MAUTOSTRIP(CXR_Model_BSP_Create_ViewInstances, MAUTOSTRIP_VOID);
	MSCOPE(Create_ViewInstances, XR_BSPMODEL);

	m_lspViews.Clear();
	m_lspViews.SetLen(_nInst);
	m_lViewData.SetLen(_nInst);

	for(int i = 0; i < _nInst; i++)
	{
		spCBSP2_ViewInstance spV = MNew(CBSP2_ViewInstance);
		if (spV == NULL) MemError("Create");
		spV->m_lPortalIDStates.SetLen(XW_MAXUSERPORTALS >> 3);

		m_lspViews[i] = spV;
		View_Reset(i);
	}
	m_pView = NULL;
	m_pViewClipData	= NULL;
	m_iView = 0;
}

#ifndef PLATFORM_CONSOLE

static int RenderBSPImage_r(CXR_Model_BSP2* _pModel, CImage* _pImage, int _iNode, int _Depth, int _Column)
{
	const CBSP2_Node& Node = _pModel->m_lNodes[_iNode];
	if (Node.IsLeaf())
	{
		if (!_iNode)
			return 0;

		CPixel32 Color = (_iNode) ? 0xffffff00 : 0xffff0000;
		_pImage->SetPixel(_pImage->GetClipRect(), CPnt(_Column, _Depth), Color);
		return 1;
	}
	else if (Node.IsNode())
	{
		CPixel32 Color = (Node.m_Flags & XW_NODE_STRUCTURE) ? 0xff007f00 : 0xff0000ff;
		if (Node.IsStructureLeaf())
			Color = 0xff00ffff;

		_pImage->SetPixel(_pImage->GetClipRect(), CPnt(_Column, _Depth), Color);

		int nColumns = 0;
		nColumns += RenderBSPImage_r(_pModel, _pImage, Node.m_iNodeFront, _Depth+1, _Column+nColumns);
		nColumns += RenderBSPImage_r(_pModel, _pImage, Node.m_iNodeBack, _Depth+1, _Column+nColumns);
		return nColumns;
	}

	return 0;
}

#endif

// -------------------------------------------------------------------
//  Read/Write
// -------------------------------------------------------------------
void CXR_Model_BSP2::Create(const char* _pParam, CDataFile* _pDFile, CCFile*, const CBSP_CreateInfo& _CreateInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_Create, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP2::Create, XR_BSPMODEL);

	m_MasterVBScale = CVec3Dfp32(0.0f);
	m_spMaster = ((CXR_Model_BSP2*)(CXR_Model*)_CreateInfo.m_spMaster);
	m_iSubModel = _CreateInfo.m_iSubModel;
	m_nSubModels = _CreateInfo.m_nSubModels;

	if (m_spMaster != NULL)
	{
		m_lspViews = m_spMaster->m_lspViews;
		m_lViewData.SetLen(m_lspViews.Len());
	}

	if (!_pDFile) Error("Create", "Datafile expected.");
	CCFile* pFile = _pDFile->GetFile();

	// Check file-version
	if (!_pDFile->GetNext("VERSION"))
		Error("Read", "No VERSION entry.");
	const int XWVersion = _pDFile->GetUserData();
	if (XWVersion > XW_VERSION)
		Error("Read", CStrF("Invalid XW-version. %.4x should be less or equal to %.4x", _pDFile->GetUserData(), XW_VERSION));

	// Read stuff
	if (!_pDFile->GetNext("PLANES")) Error("Read", "No PLANES entry.");
	if (_pDFile->GetEntrySize() / _pDFile->GetUserData() != 16)
	{
		Error("Read", "Ouch!, THAT's an old file! (Old plane def.)");
	}
	else
	{
		MSCOPE(m_lPlanes, XR_BSPMODEL);

		m_lPlanes.SetLen(_pDFile->GetUserData());
		pFile->Read(&m_lPlanes[0], m_lPlanes.ListSize());
		SwitchArrayLE_fp32((fp32*)m_lPlanes.GetBasePtr(), m_lPlanes.Len() * sizeof(CPlane3Dfp32)/sizeof(fp32)); //AR-ADD
		m_pPlanes	= m_lPlanes.GetBasePtr();
	}

	{
		MSCOPE(m_lVertices, XR_BSPMODEL);
		if (!_pDFile->GetNext("VERTICES")) Error("Read", "No VERTICES entry.");
		m_lVertices.SetLen(_pDFile->GetUserData());
		pFile->Read(&m_lVertices[0], m_lVertices.ListSize());
		SwitchArrayLE_fp32((fp32*)m_lVertices.GetBasePtr(), m_lVertices.Len() * sizeof(CVec3Dfp32)/sizeof(fp32)); //AR-ADD
	}

	{
		MSCOPE(m_liVertices, XR_BSPMODEL);
		if (!_pDFile->GetNext("VERTEXINDICES")) Error("Read", "No VERTEXINDICES entry.");
		int Len = _pDFile->GetUserData();
		m_liVertices.SetLen(Len); 
		if( 0x0123 == XWVersion )
		{
			uint32* piV = m_liVertices.GetBasePtr();
			int nRead = 0;
			while(nRead < Len)
			{
				uint16 liTemp[256];
				uint16 n = Min(256, Len-nRead);
				pFile->Read(liTemp, n*2);
				SwitchArrayLE_uint16(liTemp, n);
				for(int i = 0; i < n; i++)
					piV[nRead + i] = liTemp[i];
				nRead += n;
			}
		}
		else
		{
			pFile->ReadLE(m_liVertices.GetBasePtr(), m_liVertices.Len());
		}

//		pFile->Read(&m_liVertices[0], m_liVertices.ListSize());
//		SwitchArrayLE_uint32(m_liVertices.GetBasePtr(), m_liVertices.Len()); //AR-ADD: fix proper byte ordering for indices
	}

	{
		MSCOPE(m_lFaces, XR_BSPMODEL);
		if (!_pDFile->GetNext("FACES")) Error("Read", "No FACES entry.");

		int nFaces = _pDFile->GetUserData();
		m_lFaces.SetLen(nFaces); 
//		for (int iFace=0; iFace < nFaces; iFace++)
//			m_lFaces[iFace].Read(pFile, _pDFile->GetUserData2());

		int Ver = _pDFile->GetUserData2();

		CBSP2_Face* pFaces = m_lFaces.GetBasePtr();
		for (int iFace=0; iFace < nFaces; iFace++)
			((CBSP2_CoreFace*)&pFaces[iFace])->Read(pFile, Ver);

#ifndef M_RTM
		{
			TAP<CBSP2_Face> pF = m_lFaces;
			for (int iFace=0; iFace < nFaces; iFace++)
			{
				if (pF[iFace].m_nVertices > CRC_MAXPOLYGONVERTICES)
				{
					M_TRACE("(CXR_Model_BSP2::Create) Face %d has %d vertices.\n", iFace, pF[iFace].m_nVertices);
					pF[iFace].m_nVertices = CRC_MAXPOLYGONVERTICES;
				}
			}
		}
#endif
	}

	{
		MSCOPE(m_liFaces, XR_BSPMODEL);
		if (!_pDFile->GetNext("FACEINDICES")) Error("Read", "No FACEINDICES entry.");
		int Len = _pDFile->GetUserData();
		m_liFaces.SetLen(Len); 
		if( 0x0123 == XWVersion )
		{
			uint32* piF = m_liFaces.GetBasePtr();
			int nRead = 0;
			while(nRead < Len)
			{
				uint16 liTemp[256];
				uint16 n = Min(256, Len-nRead);
				pFile->Read(liTemp, n*2);
				SwitchArrayLE_uint16(liTemp, n);
				for(int i = 0; i < n; i++)
				{
					piF[nRead + i] = liTemp[i];
				}
				nRead += n;
			}
		}
		else
		{
			pFile->ReadLE(m_liFaces.GetBasePtr(), m_liFaces.Len());
		}
	}

// URGENTFIXME: Omtimize loading

	// Edges indices
	_pDFile->PushPosition();
	if (_pDFile->GetNext("EDGEINDICES"))
	{
		m_liEdges.SetLen(_pDFile->GetUserData());
		for(int i = 0; i < m_liEdges.Len(); i++)
			pFile->ReadLE(m_liEdges[i]);
	}
	_pDFile->PopPosition();

	// Edges
	_pDFile->ReadArrayEntry_NoVer(m_lEdges, "EDGES");

/*	_pDFile->PushPosition();
	if (_pDFile->GetNext("EDGES"))
	{
		m_lEdges.SetLen(_pDFile->GetUserData());
		for(int i = 0; i < m_lEdges.Len(); i++)
			m_lEdges[i].Read(pFile);
	}
	_pDFile->PopPosition();*/

	// EdgeFaces
	_pDFile->PushPosition();
	if (_pDFile->GetNext("EDGEFACES"))
	{
		m_lEdgeFaces.SetLen(_pDFile->GetUserData());
		for(int i = 0; i < m_lEdgeFaces.Len(); i++)
			m_lEdgeFaces[i].Read(pFile);
	}
	_pDFile->PopPosition();

	if (m_lEdges.Len() != m_lEdgeFaces.Len())
		Error("Read", "Inconsistent edge count.");


	{
		MSCOPE(m_lNodes, XR_BSPMODEL);
		if (!_pDFile->GetNext("BSPNODES")) Error("Read", "No NODES entry.");

		int nNodes = _pDFile->GetUserData();
		int Ver = _pDFile->GetUserData2();
		m_lNodes.SetLen(nNodes); 
		CBSP2_Node* pN = m_lNodes.GetBasePtr();
		if (Ver == XW_NODE_VERSION)
		{
			// Fast path if the node version is the latest
			M_ASSERT(m_lNodes.ListSize() == _pDFile->GetEntrySize(), "!");
			pFile->Read(&m_lNodes[0], m_lNodes.ListSize());
#ifndef CPU_LITTLEENDIAN
			for (int iNode=0; iNode < nNodes; iNode++)
				pN[iNode].SwapLE();
#endif
		}
		else
		{
			// Fall-back path
			for (int iNode=0; iNode < nNodes; iNode++)
				pN[iNode].Read(pFile, Ver);
		}

		m_pNodes	= m_lNodes.GetBasePtr();
	}

	{
		MSCOPE(m_lMappings, XR_BSPMODEL);
		if (!_pDFile->GetNext("MAPPINGS")) Error("Read", "No LEAVES entry.");
		m_lMappings.SetLen(_pDFile->GetUserData()); 
		{ for(int iMap = 0; iMap < m_lMappings.Len(); iMap++) m_lMappings[iMap].Read(pFile); }
	}

	{
		MSCOPE(Surfaces, XR_BSPMODEL);
		if (!_pDFile->GetNext("SURFACES")) Error("Read", "No SURFACES entry.");
		CXW_Surface::Read(pFile, m_lspSurfaces, _pDFile->GetUserData());

		if (m_spMaster != NULL)
		{
			TArray<spCXW_Surface> lspMasterSurf = m_spMaster->m_lspSurfaces;

			for(int i = 0; i < m_lspSurfaces.Len(); i++)
			{
				spCXW_Surface spSurf = m_lspSurfaces[i];

				for(int j = 0; j < lspMasterSurf.Len(); j++)
					if (spSurf->m_Name == lspMasterSurf[j]->m_Name)
					{
						m_lspSurfaces[i] = lspMasterSurf[j];
						break;
					}
			}
		}

		// Patch face XW_FACE_SHADOWCASTER flags
		{
			int nFaces = m_lFaces.Len();
			for(int iFace = 0; iFace < nFaces; iFace++)
			{
				CBSP2_Face* pFace = &m_lFaces[iFace];
				if (pFace->m_Flags & XW_FACE_VISIBLE)
				{
					CXW_Surface *pSurf = m_lspSurfaces[pFace->m_iSurface];
					if (!(pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE) &&
						!(pSurf->GetBaseFrame()->m_Medium.m_MediumFlags & XW_MEDIUM_SEETHROUGH))
						pFace->m_Flags |= XW_FACE_SHADOWCASTER;
				}
			}
		}

#ifndef M_RTM
		if(_CreateInfo.m_Flags & CBSP_CreateInfo::FLAGS_MAKEPHYSSURFACESVISIBLE)
		{
			int nSurf = m_lspSurfaces.Len();
			for(int i = 0; i < nSurf; i++)
			{
				CXW_Surface *pSurf = m_lspSurfaces[i];
//				CXW_SurfaceLayer *pLayer = &pSurf->GetBaseFrame()->m_lTextures[0];
				if(pSurf->m_Name.CompareSubStr("PHYS_") == 0)
				{
					pSurf->m_Flags &= ~XW_SURFFLAGS_INVISIBLE;
					//pSurf->m_Flags |= XW_SURFFLAGS_OGRINVISIBLE;
				}
			}

			int nFaces = m_lFaces.Len();
			for(int iFace = 0; iFace < nFaces; iFace++)
			{
				CBSP2_Face* pFace = &m_lFaces[iFace];
				if (!(pFace->m_Flags & XW_FACE_VISIBLE))
				{
					CXW_Surface *pSurf = m_lspSurfaces[pFace->m_iSurface];
					if(pSurf->m_Name.CompareSubStr("PHYS_") == 0)
						pFace->m_Flags |= XW_FACE_VISIBLE;
				}
			}

		}
#endif
	}

	{
		MSCOPE(m_lMediums, XR_BSPMODEL);
		if (!_pDFile->GetNext("MEDIUMS")) Error("Read", "No MEDIUMS entry.");
		m_lMediums.SetLen(_pDFile->GetUserData()); 
		{ for(int iMedium = 0; iMedium < m_lMediums.Len(); iMedium++) m_lMediums[iMedium].Read(pFile); }
	}

	
//	static int g_SplineBrushMem = 0;
//	static int g_nSplineBrushes = 0;
//	int dMem = MRTC_MemAvail();

//	dMem -= MRTC_MemAvail();
//	g_SplineBrushMem += dMem;
//	LogFile(CStrF("%d SBRUSH SO FAR %d bytes, %d b/sbrush", g_nSplineBrushes, g_SplineBrushMem, g_SplineBrushMem / g_nSplineBrushes));

	// Portal-leaves
	_pDFile->PushPosition();
	if (_pDFile->GetNext("PORTALLEAVES2"))
	{
		MSCOPE(PortalLeaves2, XR_BSPMODEL);

		m_lPortalLeaves.SetLen(_pDFile->GetUserData());
		m_lPortalLeaves_VBFP.SetLen(_pDFile->GetUserData());

		for(int iPL = 0; iPL < m_lPortalLeaves.Len(); iPL++)
			m_lPortalLeaves[iPL].Read(pFile, true);
	}
	_pDFile->PopPosition();

	if (!m_lPortalLeaves.Len())
	{
		_pDFile->PushPosition();
		if (_pDFile->GetNext("PORTALLEAVES"))
		{
			MSCOPE(PortalLeaves, XR_BSPMODEL);

			m_lPortalLeaves.SetLen(_pDFile->GetUserData());
			m_lPortalLeaves_VBFP.SetLen(_pDFile->GetUserData());

			for(int iPL = 0; iPL < m_lPortalLeaves.Len(); iPL++)
				m_lPortalLeaves[iPL].Read(pFile, false);
		}
		_pDFile->PopPosition();
	}

	CreatePortalLeafNodeIndices();	// This one just needs m_lPortalLeaves & m_lNodes
	PortalLeafSanityCheck("First");


#ifndef MODEL_BSP_RENDERALL
	_pDFile->PushPosition();
	if (_pDFile->GetNext("PORTALS"))
	{
		MSCOPE(Portals, XR_BSPMODEL);

		// Portals
		m_lPortals.SetLen(_pDFile->GetUserData()); 
		int XWPortalVersion = _pDFile->GetUserData2();
		{ for(int i = 0; i < m_lPortals.Len(); i++) m_lPortals[i].Read(pFile, XWPortalVersion); }

		if (!_pDFile->GetNext("PORTALINDICES")) Error("Read", "No PORTALINDICES entry.");
		m_liPortals.SetLen(_pDFile->GetUserData()); 
		pFile->Read(&m_liPortals[0], m_liPortals.ListSize());
		SwitchArrayLE_uint16(m_liPortals.GetBasePtr(), m_liPortals.Len()); //AR-ADD, byte ordering..
			

		InitPortalBounds();

		_pDFile->PushPosition();
		if (_pDFile->GetNext("PVS"))
		{
			if (_pDFile->GetUserData2() == XW_PVS_VERSION)
			{
				m_lPVS.SetLen(_pDFile->GetUserData()); 
				pFile->Read(m_lPVS.GetBasePtr(), m_lPVS.ListSize()); //AR-NOTE uint8[], no need to swap..
			}
		}
		_pDFile->PopPosition();

		// Fog-Portals
		if (_pDFile->GetNext("FOGPORTALS"))
		{
			m_lFogPortals.SetLen(_pDFile->GetUserData()); 
			for(int i = 0; i < m_lFogPortals.Len(); i++)
				m_lFogPortals[i].Read(pFile);

			if (_pDFile->GetNext("FOGPORTALFACES"))
			{
				m_lFogPortalFaces.SetLen(_pDFile->GetUserData()); 
				for(int i = 0; i < m_lFogPortalFaces.Len(); i++)
					m_lFogPortalFaces[i].Read(pFile);
			}
		}
	}
	_pDFile->PopPosition();
#endif

	// Share mediums and remap indices
	TThinArray<uint16> lMediumMapping;
	if (m_spMaster != NULL)
	{
		lMediumMapping.SetLen(m_lMediums.Len());

		CXR_MediumDesc* pM = m_lMediums.GetBasePtr();
		int nM = m_lMediums.Len();
		CXR_MediumDesc* pMM = m_spMaster->m_lMediums.GetBasePtr();
		int nMM = m_spMaster->m_lMediums.Len();

		for(int i = 0; i < nM; i++)
		{
			int j;
			for(j = 0; j < nMM; j++)
				if (pMM[j].Equal(pM[i]))
				{
					lMediumMapping[i] = j;
					break;
				}

				if (j == nMM)
				{
					// We must insert medium since it doesn't exist in the master model.
					m_spMaster->m_lMediums.Add(pM[i]);
					pMM = m_spMaster->m_lMediums.GetBasePtr();
					nMM = m_spMaster->m_lMediums.Len();
					lMediumMapping.SetLen(m_lMediums.Len());
					lMediumMapping[i] = j;
				}
		}

		m_spMaster->m_lMediums.OptimizeMemory();

		// Remap faces
		{
			CBSP2_Face* pF = m_lFaces.GetBasePtr();
			int nFaces = m_lFaces.Len();
			for(int i = 0; i < nFaces; i++)
				pF[i].m_iBackMedium = lMediumMapping[pF[i].m_iBackMedium];
		}

		// Remap nodes
		{
			CBSP2_Node* pNodes = m_lNodes.GetBasePtr();
			int nNodes = m_lNodes.Len();
			for(int i = 0; i < nNodes; i++)
			{
				if (pNodes[i].IsLeaf())
					pNodes[i].m_iMedium = lMediumMapping[pNodes[i].m_iMedium];
			}
		}
		m_lMediums = m_spMaster->m_lMediums;
	}

	// Solids
	_pDFile->PushPosition();
	if (_pDFile->GetNext("SOLIDS"))
	{
		if (!_pDFile->GetSubDir())
			Error("Read", "Corrupt SOLIDS entry.");
		m_spIndexedSolids = MNew(CXR_IndexedSolidContainer32);
		if (!m_spIndexedSolids)
			MemError("Read");
		m_spIndexedSolids->Read(_pDFile);
		m_spIndexedSolids->Create(m_lspSurfaces, m_lMediums, m_lPlanes);
		_pDFile->GetParent();

		if (m_spMaster)
		{
			// Remap solids
			if (m_spIndexedSolids)
			{
				TAP_RCD<CXR_IndexedSolid32> pS = m_spIndexedSolids->m_lSolids;
				for (int i = 0; i < pS.Len(); i++)
				{
					pS[i].m_iMedium = lMediumMapping[pS[i].m_iMedium];
				}
			}
		}
	}
	_pDFile->PopPosition();


	// Light normal-indices
	_pDFile->PushPosition();
	if (_pDFile->GetNext("LIGHTNORMALINDX"))
	{
		MSCOPE(m_liLightNormals, XR_BSPMODEL);

		m_liLightNormals.SetLen(_pDFile->GetUserData()); 
		pFile->ReadLE(m_liLightNormals.GetBasePtr(), m_liLightNormals.Len());
	}
	_pDFile->PopPosition();

	// LIGHTMAPINFO
	{
		MSCOPE(LightMapInfo, XR_BSPMODEL);
		_pDFile->PushPosition();
		if (_pDFile->GetNext("LIGHTMAPINFO"))
		{
			m_lLightMapInfo.SetLen(_pDFile->GetUserData()); 
			for(int i = 0; i < m_lLightMapInfo.Len(); i++)
				m_lLightMapInfo[i].Read(pFile, 1);
		}
		_pDFile->PopPosition();

		_pDFile->PushPosition();
		if (_pDFile->GetNext("LIGHTMAPINFO2"))
		{
			int nLightMapInfo = _pDFile->GetUserData();
			int Version = _pDFile->GetUserData2();
			if(Version == 0) Version = 2;	// Patch to get old maps working
			m_lLightMapInfo.SetLen(nLightMapInfo); 
			if(Version == XW_LIGHTMAPINFO_VERSION)
			{
				M_ASSERT(m_lLightMapInfo.ListSize() == _pDFile->GetEntrySize(), "!");
				pFile->Read(m_lLightMapInfo.GetBasePtr(), m_lLightMapInfo.ListSize());
				CBSP2_LightMapInfo* pLightMapInfo = m_lLightMapInfo.GetBasePtr();
	#ifndef CPU_LITTLEENDIAN
				for (int iLMI=0; iLMI < nLightMapInfo; iLMI++)
					pLightMapInfo[iLMI].SwapLE();
	#endif
			}
			else
			{
				CBSP2_LightMapInfo* pLightMapInfo = m_lLightMapInfo.GetBasePtr();
				for(int iLMI = 0; iLMI < nLightMapInfo; iLMI++)
					pLightMapInfo[iLMI].Read(pFile, Version);
			}

/*			m_lLightMapInfo.SetLen(_pDFile->GetUserData()); 
			for(int i = 0; i < m_lLightMapInfo.Len(); i++)
				m_lLightMapInfo[i].Read(pFile, 2);*/
/*			{
				for (int iFace=0; iFace < m_lFaces.Len(); iFace++)
				{
					CStr s;
					for(int i = 0; i < 32 && iFace < m_lFaces.Len(); i++, iFace++)
					{
						if (m_lFaces[iFace].m_Flags & XW_FACE_LIGHTMAP)
						{
							CBSP2_LightMapInfo& LMI = m_lLightMapInfo[m_lFaces[iFace].m_iLightInfo];
							s += CStrF("%.6x-%.2x-%.2x, ", m_lFaces[iFace].m_iLightInfo, m_lFaces[iFace].m_nLightInfo, LMI.m_iLMC);
						}
					}
					
					LogFile(s);
				}
			}*/
/*			{
				for (int iFace=0; iFace < m_lFaces.Len(); iFace++)
				{
					const CBSP2_Face* pFace = &m_lFaces[iFace];
					if (m_lFaces[iFace].m_Flags & XW_FACE_LIGHTMAP)
					{
						CBSP2_LightMapInfo& LMI = m_lLightMapInfo[m_lFaces[iFace].m_iLightInfo];
						LogFile(CStrF("Face %d, %d (%s), iLMI %d (%d x %d)", iFace, pFace->m_nVertices, m_lVertices[m_liVertices[pFace->m_iiVertices]].GetString().Str(), pFace->m_iLightInfo, LMI.m_LMCWidthHalf*2, LMI.m_LMCHeightHalf*2));
					}
				}
			}*/
		}
		_pDFile->PopPosition();
	}

	/*{  -- I disabled this spam /anton
		for (int iFace=0; iFace < m_lFaces.Len(); iFace++)
		{
			const CBSP2_Face* pFace = &m_lFaces[iFace];
			{
				LogFile(CStrF("Face %d, %d, %d (%s)", iFace, pFace->m_nVertices, pFace->m_iiVertices, m_lVertices[m_liVertices[pFace->m_iiVertices]].GetString().Str()));
			}
		}
	}*/

	if (m_lLightMapInfo.Len())
	{
		MSCOPE(LightMaps, XR_BSPMODEL);

		// LIGHTMAPS
		_pDFile->PushPosition();
		if (_pDFile->GetNext("LIGHTMAPS3"))
		{
			if (!_pDFile->GetSubDir())
				Error("Create", "Invalid LIGHTMAPS3 entry.");

			_pDFile->PushPosition();
			bool bBaked = true;
			if(!_pDFile->GetNext("BAKED"))
				bBaked = false;
			_pDFile->PopPosition();

			if(bBaked)
			{
				if(_pDFile->GetFile()->GetFileName() != "")
				{
					spCTextureContainer_VirtualXTC spLMTC = MNew(CTextureContainer_VirtualXTC);
					spLMTC->Create(_pDFile);
					m_spLMTC = spLMTC;
				}
#ifndef PLATFORM_CONSOLE
				else
				{
					spCTextureContainer_Plain spLMTC = MNew(CTextureContainer_Plain);
					spLMTC->AddFromXTC(_pDFile);
					m_spLMTC = spLMTC;
				}
#endif
			}
			else
			{
#ifndef PLATFORM_CONSOLE
				// Bake textures when loading so we don't need to support more than 1 codepath
				spCTextureContainer_Plain spUnbakedTC = MNew(CTextureContainer_Plain);
				spCTextureContainer_Plain spLMTC = MNew(CTextureContainer_Plain);
				spUnbakedTC->AddFromXTC(_pDFile);
				for(int i = 0; i < spUnbakedTC->GetNumTextures(); i += 5)
				{
					CImage* lpSourceImages[5];
					spCImage lspDestImages[4];
					CTC_TextureProperties Properties;
					spUnbakedTC->GetTextureProperties(i + 0, Properties);
					lpSourceImages[0] = spUnbakedTC->GetTexture(i + 0, 0, CTC_TEXTUREVERSION_ANY);
					lpSourceImages[1] = spUnbakedTC->GetTexture(i + 1, 0, CTC_TEXTUREVERSION_ANY);
					lpSourceImages[2] = spUnbakedTC->GetTexture(i + 2, 0, CTC_TEXTUREVERSION_ANY);
					lpSourceImages[3] = spUnbakedTC->GetTexture(i + 3, 0, CTC_TEXTUREVERSION_ANY);
					lpSourceImages[4] = spUnbakedTC->GetTexture(i + 4, 0, CTC_TEXTUREVERSION_ANY);
					int h = lpSourceImages[0]->GetHeight(); int w = lpSourceImages[0]->GetWidth();
					lspDestImages[0] = lpSourceImages[0]->Duplicate()->Convert(IMAGE_FORMAT_BGRA8);
					lspDestImages[1] = MNew(CImage); lspDestImages[1]->Create(w, h, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
					lspDestImages[2] = MNew(CImage); lspDestImages[2]->Create(w, h, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
					lspDestImages[3] = MNew(CImage); lspDestImages[3]->Create(w, h, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
					CClipRect cr = lpSourceImages[0]->GetClipRect();
					for(int y = 0; y < h; y++)
						for(int x = 0; x < w; x++)
						{
							CPnt Pos(x, y);
							CPixel32 Pix0 = lpSourceImages[1]->GetPixel(cr, Pos);
							CPixel32 Pix1 = lpSourceImages[2]->GetPixel(cr, Pos);
							CPixel32 Pix2 = lpSourceImages[3]->GetPixel(cr, Pos);
							CPixel32 Pix3 = lpSourceImages[4]->GetPixel(cr, Pos);
							lspDestImages[1]->SetPixel(cr, Pos, CPixel32(Pix0.GetR(), Pix0.GetG(), Pix0.GetB(), Pix3.GetR()));
							lspDestImages[2]->SetPixel(cr, Pos, CPixel32(Pix1.GetR(), Pix1.GetG(), Pix1.GetB(), Pix3.GetG()));
							lspDestImages[3]->SetPixel(cr, Pos, CPixel32(Pix2.GetR(), Pix2.GetG(), Pix2.GetB(), Pix3.GetB()));
						}
					spLMTC->AddTexture(lspDestImages[0], Properties, CStrF("LM%d_0", i / 5, 0));
					spLMTC->AddTexture(lspDestImages[1], Properties, CStrF("LM%d_1", i / 5, 0));
					spLMTC->AddTexture(lspDestImages[2], Properties, CStrF("LM%d_2", i / 5, 0));
					spLMTC->AddTexture(lspDestImages[3], Properties, CStrF("LM%d_3", i / 5, 0));
					spUnbakedTC->ReleaseTexture(i + 0, 0);
					spUnbakedTC->ReleaseTexture(i + 1, 0);
					spUnbakedTC->ReleaseTexture(i + 2, 0);
					spUnbakedTC->ReleaseTexture(i + 3, 0);
					spUnbakedTC->ReleaseTexture(i + 4, 0);
				}
				m_spLMTC	= spLMTC;
#else
				Error_static("CXR_Model_BSP2::Create", "Map uses old lightmap format. Please re-render!");
#endif
			}

			m_lLMDimensions.SetLen(m_spLMTC->GetNumLocal());
			m_lLMTextureIDs.SetLen(m_spLMTC->GetNumLocal());
			for(int i = 0; i < m_spLMTC->GetNumLocal(); i++)
			{
				CImage Desc;
				int nMipMaps;
				m_spLMTC->GetTextureDesc(i, &Desc, nMipMaps);
				m_lLMDimensions[i] = CPnt(Desc.GetWidth(), Desc.GetHeight());
				m_lLMTextureIDs[i] = m_spLMTC->GetTextureID(i);
			}
			
			_pDFile->GetParent();
		}
		_pDFile->PopPosition();

		if (!m_spLMTC)
		{
			Error("Create", "LIGHTMAPS3 not found. Legacy format?");
		}

		// LIGHTMAPCLUSTERS2
/*		_pDFile->PushPosition();
		if (_pDFile->GetNext("LIGHTMAPCLUSTERS2"))
		{
			m_lspLMC.SetLen(_pDFile->GetUserData()); 
			for(int i = 0; i < m_lspLMC.Len(); i++) 
			{
				m_lspLMC[i] = DNew(CXW_LMCluster_Ext) CXW_LMCluster_Ext;
				if (!m_lspLMC[i]) MemError("Read");
				m_lspLMC[i]->Read(pFile);
			}
		}
		else
			Error("Create", "LIGHTMAPCLUSTERS2 entry not found. Drop XW into XWC to update file format.");*

		_pDFile->PopPosition();*/
	}


	// Light field octtree
	{
		MSCOPE(LightFieldOcttree, XR_BSPMODEL);
		m_spLightFieldOcttree = CBSP2_LightFieldOcttree::FindAndRead(_pDFile);
	}

	// Light vertices
/*	_pDFile->PushPosition();
	if (_pDFile->GetNext("LIGHTVERTINFO"))
	{
		MSCOPE(LightVertices, XR_BSPMODEL);

		int nLen = _pDFile->GetUserData();
		m_lLightVerticesInfo.SetLen(nLen); 
		pFile->Read(&m_lLightVerticesInfo[0], m_lLightVerticesInfo.ListSize());
#ifndef CPU_LITTLEENDIAN
		for (int i=0; i<nLen; i++)
			m_lLightVerticesInfo[i].SwapLE();
#endif

		if (!_pDFile->GetNext("LIGHTVERTICES")) Error("Read", "No LIGHTVERTICES entry.");
		m_lLightVertices.SetLen(_pDFile->GetUserData());
		pFile->Read(&m_lLightVertices[0], m_lLightVertices.ListSize());
		SwitchArrayLE_int32(m_lLightVertices.GetBasePtr(), m_lLightVertices.Len());
	}
	_pDFile->PopPosition();*/

	// LIGHT DATA
	m_spLightData = CBSP2_LightData::FindAndRead<CBSP2_LightData_Ext>(_pDFile);

	LightDataSanityCheck();

//	LightDataFixup();

	// OctAABBTree
	_pDFile->PushPosition();
	{
		_pDFile->ReadArrayEntry(m_lOctaAABBNodes,"OCTAAABBTREE",BSP_OCTAAABBTREE_VERSION);
		if( _pDFile->GetNext("OCTAAABBHEADER") )
		{
			m_OctaAABBBox.Read(_pDFile->GetFile());

			CVec4Dfp32 Scale(65535.0f / (m_OctaAABBBox.m_Max[0] - m_OctaAABBBox.m_Min[0]),
				65535.0f / (m_OctaAABBBox.m_Max[1] - m_OctaAABBBox.m_Min[1]),
				65535.0f / (m_OctaAABBBox.m_Max[2] - m_OctaAABBBox.m_Min[2]), 0.0f);
			m_OctaAABBToU16.Unit();
			m_OctaAABBToU16.k[0][0] = Scale.k[0];
			m_OctaAABBToU16.k[1][1] = Scale.k[1];
			m_OctaAABBToU16.k[2][2] = Scale.k[2];
			m_OctaAABBToU16.k[3][0] = -m_OctaAABBBox.m_Min[0] * m_OctaAABBToU16.k[0][0];
			m_OctaAABBToU16.k[3][1] = -m_OctaAABBBox.m_Min[1] * m_OctaAABBToU16.k[1][1];
			m_OctaAABBToU16.k[3][2] = -m_OctaAABBBox.m_Min[2] * m_OctaAABBToU16.k[2][2];
			m_OctaAABBScale = Scale.v;
			m_OctaAABBInvScale = M_VRcp(m_OctaAABBScale);
			m_OctaAABBTranslate = M_VLd_V3_Slow(&m_OctaAABBBox.m_Min);

			m_OctaAABBFromU16.Unit();
			m_OctaAABBFromU16.k[0][0] = (m_OctaAABBBox.m_Max[0] - m_OctaAABBBox.m_Min[0]) / 65535.0f;
			m_OctaAABBFromU16.k[1][1] = (m_OctaAABBBox.m_Max[1] - m_OctaAABBBox.m_Min[1]) / 65535.0f;
			m_OctaAABBFromU16.k[2][2] = (m_OctaAABBBox.m_Max[2] - m_OctaAABBBox.m_Min[2]) / 65535.0f;
			m_OctaAABBFromU16.k[3][0] = m_OctaAABBBox.m_Min[0];
			m_OctaAABBFromU16.k[3][1] = m_OctaAABBBox.m_Min[1];
			m_OctaAABBFromU16.k[3][2] = m_OctaAABBBox.m_Min[2];
		}
	}
	_pDFile->PopPosition();

#ifdef MODEL_BSP_CHECKTREE
	if (m_lPortalLeaves.Len())
		if (!CheckTree_r(1))
			Error("Read", "BSP-Check failed. (1)")
		else
			LogFile("(CXR_Model_BSP2::Read) BSP-Check 1 ok.");
#endif

	{
		if (m_lLightMapInfo.Len() > 0)
		{
			if (!m_spLMTC)
				Error("Create", "No lightmap TC. Legacy format?");

/*			// Create lightmap texture container, unless we already got one
			if (!m_spLMTC)
			{
				MSCOPE(m_spLMTC, XR_BSPMODEL);

				spCTextureContainer_LMC spLMTC = DNew(CTextureContainer_LMC) CTextureContainer_LMC();
				if (spLMTC == NULL) MemError("LoadXW");
				spLMTC->Create(this);
				m_spLMTC = spLMTC;
			}
			else
			{
				for(int i = 0; i < m_lspLMC.Len(); i++)
					m_lspLMC[i]->m_TextureID = m_spLMTC->GetTextureID(i);
			}*/
		}
		else if (m_spMaster != NULL)
		{
			// Share lightmap data from master
			m_spLMTC = m_spMaster->m_spLMTC;
			m_lLMDimensions = m_spMaster->m_lLMDimensions;
			m_lLMTextureIDs = m_spMaster->m_lLMTextureIDs;
			m_lLightMapInfo = m_spMaster->m_lLightMapInfo;
			m_spLightMapContainer = m_spMaster->m_spLightMapContainer;
		}
	}

	// Create texture coordinates...
	int nTC = 0;
	{ 
		for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
		{
			nTC += m_lFaces[iFace].m_nVertices; 
			if (m_lFaces[iFace].m_Flags & XW_FACE_LIGHTMAP)
				nTC += m_lFaces[iFace].m_nVertices; 
		}
	}

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("Read", "No texture-context available.");

	// Init surfaces
	{
		MSCOPE(InitTextures, XR_BSPMODEL);
		for(int iSurf = 0; iSurf < m_lspSurfaces.Len(); iSurf++)
		{
			CXW_Surface* pS = m_lspSurfaces[iSurf];
			pS->InitTextures(false);	// Don't report failures.
		}
	}

	PortalLeafSanityCheck("PostRead1");

	CreateSurfaceShaderParams();

	LightDataSanityCheck();

	InitializeListPtrs();
	Create_PostRead();
	InitializeListPtrs();

	LightDataSanityCheck();

#if defined PLATFORM_CONSOLE && 0
	if (m_lPortalLeaves.Len())
	{
		CImage BSPImage;
		BSPImage.Create(Min(8192, m_lNodes.Len()), 64, IMAGE_FORMAT_BGRX8, IMAGE_MEM_IMAGE);
		BSPImage.Fill(BSPImage.GetClipRect(), 0);

		for(int i = 0; i < 100/4; i++)
			BSPImage.Line(BSPImage.GetClipRect(), CPnt(0, 3+4*i), CPnt(65536, 3+4*i), (i & 1) ? 0xff303030 : 0xff202020);

		RenderBSPImage_r(this, &BSPImage, 1, 0, 0);

		BSPImage.Write(CRct(0,0,-1,-1), CStrF("s:\\BSPImage_%s.tga", pFile->GetFileName().GetFilenameNoExt().Str() ) );
	}
#endif
	PortalLeafSanityCheck("PostRead2");
}

