#include "PCH.h"

#include "WBSP3Model.h"
#include "WBSP3Def.h"

#include "MFloat.h"

#include "../../MSystem/Raster/MTextureContainers.h"

#ifdef	PLATFORM_PS2
#include "../../RndrPS2/MDispPS2.h"
#include "../../RndrPS2/MRndrPS2.h"
#endif


// #define MODEL_BSP_CHECKTREE

// -------------------------------------------------------------------
void CXR_Model_BSP3::InitPortalBounds()
{
	MAUTOSTRIP(CXR_Model_BSP_InitPortalBounds, MAUTOSTRIP_VOID);
	CBSP3_PortalExt* pP = m_lPortals.GetBasePtr();
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

void CXR_Model_BSP3::ExpandFaceBoundBox_r(int _iNode, CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_ExpandFaceBoundBox_r, MAUTOSTRIP_VOID);
	CBSP3_Node* pN = &m_lNodes[_iNode];
	if (pN->IsLeaf())
	{
		int iiFaces = pN->m_iiFaces;
		int nFaces = pN->m_nFaces;
		for(int iif = 0; iif < nFaces; iif++)
		{
			const CBSP3_Face* pF = &m_lFaces[m_liFaces[iiFaces + iif]];
			for(int v = 0; v < pF->m_nVertices; v++)
			{
				const CVec3Dfp32* pV = &m_lVertices[m_liVertices[pF->m_iiVertices + v]];
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

int CXR_Model_BSP3::GetFaceList_r(int _iNode, uint32* _pDest, int _Pos, int _MaxFaces, int _iSurf, int _SurfFlags)
{
	MAUTOSTRIP(CXR_Model_BSP_GetFaceList_r, 0);
	CBSP3_Node* pN = &m_pNodes[_iNode];
	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		if (nFaces > _MaxFaces) Error("GetFaceList_r", "Insufficient space in target array.");
		int iiFaces = pN->m_iiFaces;
		int nAdded = 0;
		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_liFaces[iiFaces + f];

			CBSP3_Face* pF = &m_lFaces[iFace];

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

int CXR_Model_BSP3::GetPLFaceList_r(int _iPL, uint32* _pDest, int _MaxFaces, int _iSurf, int _SurfFlags)
{
	MAUTOSTRIP(CXR_Model_BSP_GetPLFaceList_r, 0);
	const CBSP3_PortalLeafExt& PL = m_lPortalLeaves[_iPL];

	int nFaces = PL.m_nFaces;
	if (nFaces > _MaxFaces) Error("GetFaceList_r", "Insufficient space in target array.");
	int iiFaces = PL.m_iiFaces;
	int nAdded = 0;
	for(int f = 0; f < nFaces; f++)
	{
		int iFace = m_liPLFaces[iiFaces + f];

		CBSP3_Face* pF = &m_lFaces[iFace];

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


int CXR_Model_BSP3::CompareFaces_Index(int _iFace1, int _iFace2)
{
	MAUTOSTRIP(CXR_Model_BSP_CompareFaces_Index, 0);
	const CBSP3_Face* pF1 = &m_lFaces[_iFace1];
	const CBSP3_Face* pF2 = &m_lFaces[_iFace2];

	int iSurf1 = pF1->m_iSurface;
	int iSurf2 = pF2->m_iSurface;
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

int CXR_Model_BSP3::CompareFaces(int _iFace1, int _iFace2)
{
	MAUTOSTRIP(CXR_Model_BSP_CompareFaces, 0);
	const CBSP3_Face* pF1 = &m_lFaces[_iFace1];
	const CBSP3_Face* pF2 = &m_lFaces[_iFace2];

	int iSurf1 = pF1->m_iSurface;
	int iSurf2 = pF2->m_iSurface;
	if (iSurf1 < iSurf2)
		return -1;
	else if (iSurf1 > iSurf2)
		return 1;
	else
	{
		return 0;
	}
}

int CXR_Model_BSP3::CompareFaces(const CBSP3_Face* _pF1, const CBSP3_Face* _pF2)
{
	MAUTOSTRIP(CXR_Model_BSP_CompareFaces_2, 0);
	int iSurf1 = _pF1->m_iSurface;
	int iSurf2 = _pF2->m_iSurface;
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

class CBSP3_FaceSortElem
{
public:
	CBSP3_Face* m_pFace;
	CXR_Model_BSP3* m_pObj;

	int Compare(const CBSP3_FaceSortElem&) const;
};

int CBSP3_FaceSortElem::Compare(const CBSP3_FaceSortElem& _Elem) const
{
	MAUTOSTRIP(CFaceSortElem_Compare, 0);
	return m_pObj->CompareFaces(m_pFace, _Elem.m_pFace);
}

void CXR_Model_BSP3::SortFaceList(uint32* _pSrc, uint32* _pDest, int _nF)
{
	MAUTOSTRIP(CXR_Model_BSP_SortFaceList, MAUTOSTRIP_VOID);
	if (!_nF) return;

	TArray_Sortable<CBSP3_FaceSortElem> lSort;
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
class CBSP3_FaceVBSortElem
{
public:
	int m_iFace;
	CXR_Model_BSP3* m_pObj;

	int Compare(const CBSP3_FaceVBSortElem&) const;
};

int CBSP3_FaceVBSortElem::Compare(const CBSP3_FaceVBSortElem& _Elem) const
{
	MAUTOSTRIP(CFaceVBSortElem_Compare, 0);
	const CBSP3_Face& F0 = m_pObj->m_lFaces[m_iFace];
	const CBSP3_Face& F1 = _Elem.m_pObj->m_lFaces[_Elem.m_iFace];

	if (F0.m_iVB < F1.m_iVB)
		return -1;
	else if (F0.m_iVB > F1.m_iVB)
		return 1;
	else
		return 0;
}

void CXR_Model_BSP3::VBSortFaceList(uint32* _pSrc, uint32* _pDest, int _nF)
{
	MAUTOSTRIP(CXR_Model_BSP_VBSortFaceList, MAUTOSTRIP_VOID);
	if (!_nF) return;

	TArray_Sortable<CBSP3_FaceVBSortElem> lSort;
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
int CXR_Model_BSP3::CompareSurface(int _iFace, int _SBID)
{
	MAUTOSTRIP(CXR_Model_BSP_CompareSurface, 0);
	int iSB = SBELEM_BRUSH(_SBID);
	int iSBFace = SBELEM_FACE(_SBID);
	const CBSP3_Face* pF1 = &m_lFaces[_iFace];
	CSplineBrush_Face* pF2 = &m_lspSplineBrushes[iSB]->m_lFaces[iSBFace];

	int iSurf1 = pF1->m_iSurface;
	int iSurf2 = pF2->m_iSurface;

	if (iSurf1 < iSurf2)
		return -1;
	else if (iSurf1 > iSurf2)
		return 1;
	else
	{
		return 0;
	}
}

// -------------------------------------------------------------------
int CXR_Model_BSP3::CompareSBFaces(int _iSB1, int _iFace1, int _iSB2, int _iFace2)
{
	MAUTOSTRIP(CXR_Model_BSP_CompareSBFaces, 0);
	CSplineBrush_Face* pF1 = &m_lspSplineBrushes[_iSB1]->m_lFaces[_iFace1];
	CSplineBrush_Face* pF2 = &m_lspSplineBrushes[_iSB2]->m_lFaces[_iFace2];

	int iSurf1 = pF1->m_iSurface;
	int iSurf2 = pF2->m_iSurface;
	if (iSurf1 < iSurf2)
		return -1;
	else if (iSurf1 > iSurf2)
		return 1;
	else
	{
		return 0;
	}
}

int CXR_Model_BSP3::CompareSBIDs(int _SBID1, int _SBID2)
{
	MAUTOSTRIP(CXR_Model_BSP_CompareSBIDs, 0);
	return CompareSBFaces(SBELEM_BRUSH(_SBID1), SBELEM_FACE(_SBID1), SBELEM_BRUSH(_SBID2), SBELEM_FACE(_SBID2));
}


class CBSP3_CSBFaceSortElem
{
public:
	int m_iFace;
	int m_iSB;
	CXR_Model_BSP3* m_pObj;

	int Compare(const CBSP3_CSBFaceSortElem&) const;
};

int CBSP3_CSBFaceSortElem::Compare(const CBSP3_CSBFaceSortElem& _Elem) const
{
	MAUTOSTRIP(CSBFaceSortElem_Compare, 0);
	return m_pObj->CompareSBFaces(m_iSB, m_iFace, _Elem.m_iSB, _Elem.m_iFace);
}

void CXR_Model_BSP3::SortSBFaceList(uint32* _pSrc, uint32* _pDest, int _nF)
{
	MAUTOSTRIP(CXR_Model_BSP_SortSBFaceList, MAUTOSTRIP_VOID);
	TArray_Sortable<CBSP3_CSBFaceSortElem> lSort;
	lSort.SetLen(_nF);

	int i;
	for(i = 0; i < _nF; i++)
	{
		lSort[i].m_iFace = SBELEM_FACE(_pSrc[i]);
		lSort[i].m_iSB = SBELEM_BRUSH(_pSrc[i]);
		lSort[i].m_pObj = this;
	}

	lSort.Sort();

	for(i = 0; i < _nF; i++)
	{
		_pDest[i] = SBELEM(lSort[i].m_iSB, lSort[i].m_iFace);
	}
}

void CXR_Model_BSP3::CreateSBTess(CBSP3_VertexBuffer* _pVB, int _iV, int _iP, class CSplineBrush* _pSB, int _iFace)
{
	MAUTOSTRIP(CXR_Model_BSP_CreateSBTess, MAUTOSTRIP_VOID);
}

void CXR_Model_BSP3::CreateSBPrim(int _iV, class CSplineBrush* _pSB, int _iFace, int _iSBPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_CreateSBPrim, MAUTOSTRIP_VOID);
}

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

void CXR_Model_BSP3::CountVertexBuffer(uint32* _piFaces, int _nFaces, uint32* _pSBFaces, int _nSBFaces, int& _nVB, int& _nSBPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_CountVertexBuffer, MAUTOSTRIP_VOID);
	// Counts the space required by PrepareVertexBuffer

//	int nF = _nFaces;

	// Count vertices
	int nPrim, nVert;
	RenderGetPrimCount(_piFaces, _nFaces, nVert, nPrim);

//	int iSBStartVert = nVert;
	int nSBPrim = 0;


	if (nVert > 65535)
	{
		if (_nSBFaces)
		{
			CountVertexBuffer(_piFaces, _nFaces, NULL, 0, _nVB, _nSBPrim);
			CountVertexBuffer(NULL, 0, _pSBFaces, _nSBFaces, _nVB, _nSBPrim);
		}
		else
		{
			int nFaces1 = _nFaces >> 1;
			int nSBFaces1 = _nSBFaces >> 1;
			CountVertexBuffer(_piFaces, nFaces1, _pSBFaces, nSBFaces1, _nVB, _nSBPrim);
			_piFaces += nFaces1;
			_pSBFaces += nSBFaces1;
			CountVertexBuffer(_piFaces, _nFaces - nFaces1, _pSBFaces, _nSBFaces - nSBFaces1, _nVB, _nSBPrim);
		}
		return;
	}

	_nSBPrim += nSBPrim;
	_nVB++;
}

void CXR_Model_BSP3::PrepareVertexBuffer(uint32* _piFaces, int _nFaces, uint32* _pSBFaces, int _nSBFaces, int& _iVB, int& _iSBPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_PrepareVertexBuffer, MAUTOSTRIP_VOID);
	MSCOPESHORT(PrepareVertexBuffer);

	// Builds vertex-buffer creation data. (used for CreateVertexBuffer later)

	int nF = _nFaces;

	// Count vertices
	int nPrim, nVert;
	RenderGetPrimCount(_piFaces, _nFaces, nVert, nPrim);

//	int nBrushVert = nVert;

	int iSBStartVert = nVert;

	if (nVert > 65535)
	{
		if (_nSBFaces)
		{
			PrepareVertexBuffer(_piFaces, _nFaces, NULL, 0, _iVB, _iSBPrim);
			PrepareVertexBuffer(NULL, 0, _pSBFaces, _nSBFaces, _iVB, _iSBPrim);
		}
		else
		{
			int nFaces1 = _nFaces >> 1;
			int nSBFaces1 = _nSBFaces >> 1;
			PrepareVertexBuffer(_piFaces, nFaces1, _pSBFaces, nSBFaces1, _iVB, _iSBPrim);
			_piFaces += nFaces1;
			_pSBFaces += nSBFaces1;
			PrepareVertexBuffer(_piFaces, _nFaces - nFaces1, _pSBFaces, _nSBFaces - nSBFaces1, _iVB, _iSBPrim);
		}
		return;
	}


	// Create

	int iSurf = 0;
	int bLightVertices = false;

	if (_nFaces)
	{
		iSurf = m_lFaces[_piFaces[0]].m_iSurface;
		if (m_lFaces[_piFaces[0]].m_Flags & XW_FACE_LIGHTVERTICES)
			bLightVertices = true;
	}
	else if(_nSBFaces)
	{
		CSplineBrush_Face* pF = &m_lspSplineBrushes[SBELEM_BRUSH(_pSBFaces[0])]->m_lFaces[SBELEM_FACE(_pSBFaces[0])];

		iSurf = pF->m_iSurface;
	}
	else
		Error("PrepareVertexBuffer", "No data.");

	// Search for queue with same surface and lightmap
	int q;
	for(q = 0; q < m_lVBQueues.Len(); q++)
		if (m_lVBQueues[q].m_iSurface == iSurf ) break;

	int iVBQueue = 0;
	if (q < m_lVBQueues.Len())
		iVBQueue = q;
	else
	{
		// Didn't find a queue, so we have to add a new one.
		CBSP3_VBQueue Q;
		Q.m_iSurface = iSurf;
		iVBQueue = m_lVBQueues.Add(Q);
	}

//	int iVB = m_lspVB.Add(spCBSP3_VertexBuffer(NULL));
	m_lspVB[_iVB] = NULL;

	// Set VB index on all faces
	for(int i = 0; i < _nFaces; i++)
		m_lFaces[_piFaces[i]].m_iVB = _iVB;



	int Components = 1;
	int SurfFlagsAll = GetSurfaceFlagsAllVersions(m_lspSurfaces[iSurf]);

	if (!(m_lspSurfaces[iSurf]->m_Flags & XW_SURFFLAGS_INVISIBLE))
		Components |= 1;
	if (bLightVertices)
		Components |= 4;
	if (SurfFlagsAll & XW_SURFFLAGS_NEEDNORMALS)
		Components |= 8;

	if (SurfFlagsAll & XW_SURFFLAGS_NEEDTANGENTS ||
		m_lspSurfaces[iSurf]->GetBaseFrame()->GetBumpMap() != NULL)
		Components |= 16;

#if defined(PLATFORM_DOLPHIN) || defined(PLATFORM_PS2)
	if (!m_lPortalLeaves.Len())
		Components |= 8; // need normals for dynamic objects
#else
	Components |= 8; // need normals for dynamic light
#endif

	{
		MSCOPESHORT(VBInfo);

		spCBSP3_VBInfo spVBCI = MNew(CBSP3_VBInfo);
		spVBCI->m_liFaces.SetLen(_nFaces);
		spVBCI->m_lSBFaces.SetLen(_nSBFaces);
		spVBCI->m_Components = Components;
	//	spVBCI->m_MaxPrim = nPrim+1;
		spVBCI->m_MaxPrim = 1 + _nSBFaces*4;
		spVBCI->m_iVBQueue = iVBQueue;
		spVBCI->m_nVertices = nVert;
		spVBCI->m_iSBVertices = iSBStartVert;
		spVBCI->m_iSurface = iSurf;
		memcpy(spVBCI->m_liFaces.GetBasePtr(), _piFaces, spVBCI->m_liFaces.ListSize());
		memcpy(spVBCI->m_lSBFaces.GetBasePtr(), _pSBFaces, spVBCI->m_lSBFaces.ListSize());
		m_lspVBInfo[_iVB] = spVBCI;
	}
//	m_lspVBInfo.Add(spVBCI);

//LogFile(CStrF("VB %d, MaxPrim %d, MaxVert %d, BrushVert %d", m_lspVBInfo.Len()-1, nPrim, nVert, nBrushVert));

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
		for(int f = 0; f < nF; f++)
		{
			CBSP3_Face& Face = m_lFaces[_piFaces[f]];
			Face.m_iiVBVertices = iV;
			//Face.m_nVBVertices = Face.m_nVertices;
			iV += Face.m_nVertices;
		}
	}

	// Create SB primitives
	int iV = iSBStartVert;

	if (iV != nVert)
		Error("PrepareVertexBuffer", CStrF("Internal error. (%d != %d)", iV, nVert));

	_iVB++;
}

void CXR_Model_BSP3::CreateVertexBuffer(int _iVB)
{
	MAUTOSTRIP(CXR_Model_BSP_CreateVertexBuffer, MAUTOSTRIP_VOID);
	if (m_lspVB[_iVB] != NULL)
		return;

	CBSP3_VBInfo& VBCI = *m_lspVBInfo[_iVB];

	spCBSP3_VertexBuffer spVB = MNew(CBSP3_VertexBuffer);
	if (!spVB) MemError("CreateVertexBuffer");
	m_lspVB[_iVB] = spVB;

	spVB->Create(VBCI.m_nVertices, VBCI.m_Components);

	// Create the vertex data
	Tesselate(VBCI.m_liFaces.GetBasePtr(), VBCI.m_liFaces.Len(), VBCI.m_iSBVertices, 
		spVB->m_lV.GetBasePtr(), spVB->m_lTV1.GetBasePtr(), spVB->m_lTV2.GetBasePtr(), spVB->m_lCol.GetBasePtr(), spVB->m_lN.GetBasePtr(), spVB->m_lTangU.GetBasePtr(), spVB->m_lTangV.GetBasePtr(), NULL, m_lspSurfaces[VBCI.m_iSurface]);

	int iV = VBCI.m_iSBVertices;

	if (iV != VBCI.m_nVertices)
		Error("CreateVertexBuffer", CStrF("Internal error. (%d != %d)", iV, VBCI.m_nVertices));

//	LogFile(CStrF("    VertexBuffer: Surface %d, LMC %d, Faces %d, SBFaces %d, nV %d, nPrim %d", iSurf ,iLMC, nF, _nSBFaces, nVert, nPrim ));
}

CBSP3_VertexBuffer* CXR_Model_BSP3::GetVertexBuffer(int _iVB)
{
	MAUTOSTRIP(CXR_Model_BSP_GetVertexBuffer, NULL);
	CBSP3_VertexBuffer* pBSPVB = m_lspVB[_iVB];
	if (!pBSPVB)
	{
		CreateVertexBuffer(_iVB);
		return m_lspVB[_iVB];
	}
	else
		return pBSPVB;
}


static int IsPointColinear(const CVec3Dfp32* pV, const uint32* piV, int nV, int iV)
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
	if ((M_Fabs(1.0f - E0*E1) < MODEL_BSP_EPSILON) &&
		((E0/E1).Length() < MODEL_BSP_EPSILON)) return 1;

	return 0;
}

void CXR_Model_BSP3::CreatePhysVertIndices()
{
	MAUTOSTRIP(CXR_Model_BSP_CreatePhysVertIndices, MAUTOSTRIP_VOID);
	MSCOPE(CreatePhysVertIndices, XR_BSPMODEL);

	TArray<uint32> liV;
	liV.SetGrow(m_lFaces.Len()*2+32);

	uint32 liVTemp[128];

	const CVec3Dfp32* pV = m_lVertices.GetBasePtr();
	int iVBase = m_liVertices.Len();
	for(int i = 0; i < m_lFaces.Len(); i++)
	{
		CBSP3_Face* pF = &m_lFaces[i];
		int nv = pF->m_nVertices;
		int iiv = pF->m_iiVertices;

		pF->m_nPhysV = 0;
		for(int v = 0; v < nv; v++)
		{
			if (!IsPointColinear(pV, &m_liVertices[iiv], nv, v))
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

void CXR_Model_BSP3::CountVBFP(uint32* _piFaces, int _nFaces, int& _nVBFP, int& _nFacePrim)
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
			!(m_lspSurfaces[m_lspVBInfo[iVB]->m_iSurface]->m_Flags & (XW_SURFFLAGS_REFLECTION | XW_SURFFLAGS_SKYVIS | XW_SURFFLAGS_INVISIBLE)))
		{
			int nFaces = iiFace2 - iiFace;
			int nV = 0;
			int nPrim = 0;
#ifdef MODEL_BSP3_USEKNITSTRIP
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

void CXR_Model_BSP3::CreateVBFP(uint32* _piFaces, int _nFaces, int& _iVBFP, int& _iFacePrim)
{
	MAUTOSTRIP(CXR_Model_BSP_CreateVBFP, MAUTOSTRIP_VOID);
//	const int MaxVBFP = 512;
//	CBSP3_VBFacePrim lVBFP[MaxVBFP];
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
			!(m_lspSurfaces[m_lspVBInfo[iVB]->m_iSurface]->m_Flags & (XW_SURFFLAGS_REFLECTION | XW_SURFFLAGS_SKYVIS | XW_SURFFLAGS_INVISIBLE)))
		{
			int nFaces = iiFace2 - iiFace;
			int nV = 0;
			int nPrim = 0;
#ifdef MODEL_BSP3_USEKNITSTRIP
			RenderGetPrimCount_KnitStrip(&_piFaces[iiFace], nFaces, nV, nPrim);
#else
			RenderGetPrimCount(&_piFaces[iiFace], nFaces, nV, nPrim);
#endif

			CBSP3_VBFacePrim& VBFP = m_lVBFacePrim[_iVBFP];
			VBFP.m_iPrim = _iFacePrim;
			VBFP.m_iVB = iVB;
			VBFP.m_nPrim = nPrim;
			VBFP.m_VBID	= 0;

			m_lspVBInfo[iVB]->m_MaxPrim += (2 + sizeof(void *) / 2);

			M_ASSERT(VBFP.m_iPrim + nPrim <= m_lFacePrim.Len(), "!");

#ifdef MODEL_BSP3_USEKNITSTRIP
			RenderTesselatePrim_KnitStrip(&_piFaces[iiFace], nFaces, &m_lFacePrim[VBFP.m_iPrim]);
#else
			RenderTesselatePrim(&_piFaces[iiFace], nFaces, &m_lFacePrim[VBFP.m_iPrim]);
#endif

			//Added by Anton - must calculate the boundbox
			const uint32* piF = _piFaces + iiFace;
			for (int i=0; i<nFaces; i++)
			{
				const CBSP3_Face& F = m_lFaces[ *(piF++) ];
				const uint32* piV = &m_liVertices[F.m_iiVertices];
				int nv = F.m_nVertices;
				for (int v=0; v<nv; v++)
				{
					const CVec3Dfp32& V = m_lVertices[piV[v]];
					VBFP.m_BoundBox.Expand(V);
				}
			}

			_iFacePrim += nPrim;

			_iVBFP++;
		}

		iiFace = iiFace2;
	}

/*	if (nVBFP)
	{
		m_lVBFacePrim.SetLen(m_lVBFacePrim.Len() + nVBFP);
		memcpy(&m_lVBFacePrim[m_lVBFacePrim.Len() - nVBFP], lVBFP, nVBFP*sizeof(CBSP3_VBFacePrim));
	}*/
}

void CXR_Model_BSP3::Create_PostRead()
{
	MAUTOSTRIP(CXR_Model_BSP_Create_PostRead, MAUTOSTRIP_VOID);
	MSCOPE(Create_PostRead, XR_BSPMODEL);
#ifdef MODEL_BSP_CHECKTREE
	if (m_lPortalLeaves.Len())
		if (!CheckTree_r(1))
			Error("ModelChanged", "BSP-Check failed. (1)")
		else
			LogFile("(CXR_Model_BSP3::ModelChanged) BSP-Check 1 ok.");
#endif

	MACRO_GetSystemEnvironment(pEnv);
	if (pEnv)
	{
		m_VBSBTessLevel = Clamp01(pEnv->GetValuef("XR_SPLINETESSLEVEL", 0.5f));
	}

		
	if (!m_spMaster)
	{
		if (m_lPortalLeaves.Len())
			Create_ViewInstances(MODEL_BSP_MAXVIEWS);
		else
			Create_ViewInstances(1);
	}

	InitBound();
	InitNodePlaneTypes();

	CreatePhysVertIndices();

	{
		MSCOPE(TagLists, XR_BSPMODEL);
		if (!m_spMaster)
		{
			m_lFaceLightMask.SetLen(m_lFaces.Len());
			FillChar(&m_lFaceLightMask[0], m_lFaceLightMask.ListSize(), 0);
			m_lLightElemTagList.SetLen(Min(MODEL_BSP_MAXDYNAMICLIGHTFACES, m_lFaces.Len()) );
			m_lFaceTagList.SetLen((m_lFaces.Len()+31) >> 3);
			FillChar(m_lFaceTagList.GetBasePtr(), m_lFaceTagList.ListSize(), 0);
			m_lFaceUntagList.SetLen(m_lFaces.Len());
			m_nTagFaces = 0;
		}
		else
		{
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
		}
	}

	// Init backwards-references from portal-leaves to nodes.
	m_nStructureLeaves = 0;
	for(int iNode = 0; iNode < m_lNodes.Len(); iNode++)
		if (m_lNodes[iNode].m_Flags & XW_NODE_PORTAL)
		{
			if (m_lNodes[iNode].m_Flags & XW_NODE_STRUCTURE)
			{
				int iPL = m_lNodes[iNode].m_iPortalLeaf;
				m_lPortalLeaves[iPL].m_iNode = iNode;
				m_nStructureLeaves = MaxMT(iPL+1, m_nStructureLeaves);
			}
//			if (m_lNodes[iNode].m_Flags & XW_NODE_NAVIGATION)
//				m_lPortalLeaves[m_lNodes[iNode].m_iNavigationLeaf].m_iNode = iNode;
		}


	// Dump nodes and portal-leaves to logfile.
/*	{
		for(int i = 0; i < m_lNodes.Len(); i++)
		{
			CBSP3_Node* pN = &m_lNodes[i];
			if (pN->IsLeaf())
				LogFile(CStrF("Parent %.3d <- this %.3d ->   -/-  , Flg %.4x, PL %.3d, NL %.3d", pN->m_iNodeParent, i, pN->m_Flags, pN->m_iPortalLeaf, pN->m_iNavigationLeaf));
			else
				LogFile(CStrF("Parent %.3d <- this %.3d -> %.3d/%.3d, Flg %.4x, PL %.3d, NL %.3d", pN->m_iNodeParent, i, pN->m_iNodeFront, pN->m_iNodeBack, pN->m_Flags, pN->m_iPortalLeaf, pN->m_iNavigationLeaf));
		}

		for(i = 0; i < m_lPortalLeaves.Len(); i++)
		{
			CBSP3_PortalLeaf* pPL = &m_lPortalLeaves[i];
			LogFile(CStrF("PL %.3d, Flg %.4x, Node %.3d, Medium %.8x", i, pPL->m_Flags, pPL->m_iNode, pPL->m_ContainsMedium));
		}

		for(i = 0; i < m_lPortals.Len(); i++)
		{
			CBSP3_PortalExt* pP = &m_lPortals[i];
			LogFile(CStrF("P %.3d, ID %d, Flg %.4x, NodeF %.3d, NodeB %.3d", i, pP->m_PortalID, pP->m_Flags, pP->m_iNodeFront, pP->m_iNodeBack));
		}
	}
*/


	// Init portal-leaves face bounding-box.
	int iPL;
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
	}

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
				for(iPL = 0; iPL < m_lPortalLeaves.Len(); iPL++)
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
		//		LogFile(CStrF("(CXR_Model_BSP3::ModelChanged) nSky %d, iSky %d", nSkyF, niPLF));
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

		// Create vertex buffers
		{
			MSCOPE(CreateVBs, XR_BSPMODEL);

			// -------------------------------------------------------------------
			// Get faces
			CXR_PhysicsContext PhysContext;
			int nF = EnumFaces_All(&PhysContext, 1, liFaces.GetBasePtr(), nEnumFaces);
			EnumFaces_Untag(&PhysContext);

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
			// Get spline brush faces
			TArray<uint32> lSBFaces;
			{
				int nSBFaces = 0;
				int iSB;
				for(iSB = 0; iSB < m_lspSplineBrushes.Len(); iSB++)
					nSBFaces += m_lspSplineBrushes[iSB]->m_lFaces.Len();

				lSBFaces.SetLen(nSBFaces);
				int nSBFaces2 = 0;
				uint32* pSBFaces = lSBFaces.GetBasePtr();

				M_ASSERT(m_lspSplineBrushes.Len() <= SBELEM_MAXBRUSHES, "!");

				for(iSB = 0; iSB < m_lspSplineBrushes.Len(); iSB++)
				{
					CSplineBrush* pSB = m_lspSplineBrushes[iSB];
					M_ASSERT(pSB->m_lFaces.Len() <= SBELEM_MAXFACES, "!");
					for(int f = 0; f < pSB->m_lFaces.Len(); f++)
						pSBFaces[nSBFaces2++] = SBELEM(iSB, f);
				}

				M_ASSERT(nSBFaces == nSBFaces2, "!");
				SortSBFaceList(lSBFaces.GetBasePtr(), lSBFaces.GetBasePtr(), lSBFaces.Len());
			}

			// -------------------------------------------------------------------
			// Count vertex buffer stuff
			int nSBPrim = 0;
			int nVB = 0;
			{
				MSCOPESHORT(CountVertexBuffer);

				int nF2 = lSBFaces.Len();
				int k2 = 0;
				int k = 0;
				while(k < nF || k2 < nF2)
				{
					int i = k;
					int i2 = k2;

					if ((k < nF) && ((k2 == nF2) || CompareSurface(piFaces[k], lSBFaces[k2]) <= 0))
					{
						while(i < nF && !CompareFaces(piFaces[k], piFaces[i])) i++;
						if ((k2 < nF2) && CompareSurface(piFaces[k], lSBFaces[k2]) == 0)
							while(i2 < nF2 && !CompareSBIDs(lSBFaces[k2], lSBFaces[i2])) i2++;
					}
					else
					{
						while(i2 < nF2 && !CompareSBIDs(lSBFaces[k2], lSBFaces[i2])) i2++;
					}

					if (i2-k2)
						CountVertexBuffer(&piFaces[k], i - k, &lSBFaces[k2], i2 - k2, nVB, nSBPrim);
					else
						CountVertexBuffer(&piFaces[k], i - k, NULL, 0, nVB, nSBPrim);

					k = i;
					k2 = i2;
				}
			}

			// -------------------------------------------------------------------
			// Prepare vertex buffers
			m_lspVB.SetLen(nVB);
			m_lspVBInfo.SetLen(nVB);
			m_lSBPrim.SetLen(nSBPrim);
			m_lVBQueues.SetGrow(m_lspVB.Len());

			int iSBPrim = 0;
			int iVB = 0;
			{
				MSCOPESHORT(PrepareVertexBuffer);

				int nF2 = lSBFaces.Len();
				int k2 = 0;
				int k = 0;
				while(k < nF || k2 < nF2)
				{
					int i = k;
					int i2 = k2;

					if ((k < nF) && ((k2 == nF2) || CompareSurface(piFaces[k], lSBFaces[k2]) <= 0))
					{
						while(i < nF && !CompareFaces(piFaces[k], piFaces[i])) i++;
						if ((k2 < nF2) && CompareSurface(piFaces[k], lSBFaces[k2]) == 0)
							while(i2 < nF2 && !CompareSBIDs(lSBFaces[k2], lSBFaces[i2])) i2++;
					}
					else
					{
						while(i2 < nF2 && !CompareSBIDs(lSBFaces[k2], lSBFaces[i2])) i2++;
					}

					if (i2-k2)
						PrepareVertexBuffer(&piFaces[k], i - k, &lSBFaces[k2], i2 - k2, iVB, iSBPrim);
					else
						PrepareVertexBuffer(&piFaces[k], i - k, NULL, 0, iVB, iSBPrim);

					k = i;
					k2 = i2;
				}
			}

			M_ASSERT(iVB == nVB, "!");
			M_ASSERT(iSBPrim == nSBPrim, "!");

//			m_lSBPrim.OptimizeMemory();

			// -------------------------------------------------------------------
			// Assign VB queue index to all spline-brush faces.
			{
				MSCOPESHORT(VBQueueIndex);

				for(int iSB = 0; iSB < m_lspSplineBrushes.Len(); iSB++)
				{
					CSplineBrush* pSB = m_lspSplineBrushes[iSB];
					for(int f = 0; f < pSB->m_lFaces.Len(); f++)
					{
						CSplineBrush_Face* pF = &pSB->m_lFaces[f];

						// Search for queue with same surface and lightmap
						int q;
						for(q = 0; q < m_lVBQueues.Len(); q++)
							if (m_lVBQueues[q].m_iSurface == pF->m_iSurface ) break;

						if (q < m_lVBQueues.Len())
							pF->m_iVBQueue = q;
						else
						{
							// Didn't find a queue, so we have to add a new one.
							pF->m_iVBQueue = m_lVBQueues.Len();
							CBSP3_VBQueue Q;
							Q.m_iSurface = pF->m_iSurface;
							m_lVBQueues.Add(Q);
						}
					}
				}
			}

	//		m_lVBQueues.SetLen(iQueue);
	//		LogFile("-------------------------------------------------------------------");
	//		LogFile(CStrF("(CXR_Model_BSP3::Read) %d VB Queues.", m_lVBQueues.Len() ));
	//		LogFile(CStrF("(CXR_Model_BSP3::Read) %d VBs.", m_lspVB.Len() ));
	//		LogFile(CStrF("(CXR_Model_BSP3::Read) %d VB Surface clusters.", m_lVBSurfClusters.Len() ));
	//		LogFile("-------------------------------------------------------------------");

			m_iFirstLightVB	= 0;
			m_iFirstFaceVB	= 0;
			int nLightVB = 0;
			if( m_spLightData )
			{
				CBSP3_LightData* pLD = m_spLightData;
				nLightVB = pLD->GetSVCount();
				if( nLightVB > 0 )
					m_iFirstLightVB	= nVB;
			}

			m_iFirstFaceVB	= nVB + nLightVB;

			{
				MSCOPESHORT(VB_AllocVBID);
				VB_AllocVBID();
			}
		}
		
		// Build portal-leaf VBFacePrims
		if (m_lPortalLeaves.Len())
		{
			MSCOPE(CreateVBFP, XR_BSPMODEL);

			int nVBFP = 0;
			int nFacePrim = 0;
			{
				for(int iPL = 0; iPL < m_nStructureLeaves; iPL++)
				{
//					CBSP3_PortalLeafExt& PL = m_lPortalLeaves[iPL];
					int nFaces = GetPLFaceList_r(iPL, liFaces.GetBasePtr(), nEnumFaces, -1, -1);
					if (nFaces)
					{
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
				CBSP3_PortalLeafExt& PL = m_lPortalLeaves[iPL];
				int nFaces = GetPLFaceList_r(iPL, liFaces.GetBasePtr(), nEnumFaces, -1, -1);
				if (nFaces)
				{
					PL.m_iVBFacePrims = iVBFP;
					VBSortFaceList(liFaces.GetBasePtr(), NULL, nFaces);
					CreateVBFP(liFaces.GetBasePtr(), nFaces, iVBFP, iFacePrim);
					PL.m_nVBFacePrims = iVBFP - PL.m_iVBFacePrims;
					if (!PL.m_nVBFacePrims)
						PL.m_iVBFacePrims = 0;
				}
			}

			for( int i = 0; i < nVBFP; i++ )
			{
				m_lVBFacePrim[i].m_VBID	= m_pVBCtx->AllocID( m_iVBC, m_iFirstFaceVB + i );
			}

			M_ASSERT(iVBFP == nVBFP, "!");
			M_ASSERT(iFacePrim == nFacePrim, "!");

			// Debug
	/*		for(int iPL = 0; iPL < m_nStructureLeaves; iPL++)
			{
				CBSP3_PortalLeafExt& PL = m_lPortalLeaves[iPL];
				LogFile(CStrF("PORTALLEAF %d, iVBFacePrims %d, nVBFacePrims %d", iPL, PL.m_iVBFacePrims, PL.m_nVBFacePrims));

				for(int i = 0; i < PL.m_nVBFacePrims; i++)
				{
					const CBSP3_VBFacePrim& VBFP = m_lVBFacePrim[PL.m_iVBFacePrims + i];
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

				for( int i = 0; i < nVBFP; i++ )
				{
					m_lVBFacePrim[i].m_VBID	= m_pVBCtx->AllocID( m_iVBC, m_iFirstFaceVB + i );
				}
			}
		}
	}

	// Init iBaseVertex & nVertices
	uint32 MaxPLVerts = 0;
	for(iPL = 0; iPL < m_lPortalLeaves.Len(); iPL++)
	{
		CBSP3_PortalLeafExt* pPL = &m_lPortalLeaves[iPL];
		if (pPL->m_nFaces)
		{
			uint32 iBase = m_lVertices.Len()-1;
			uint32 iMax = 0;
			for(int iiFace = 0; iiFace < pPL->m_nFaces; iiFace++)
			{
				CBSP3_Face* pF = &m_lFaces[m_liPLFaces[pPL->m_iiFaces + iiFace]];
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
			m_lspFogBoxes.Add(spCBSP3_FogBox(NULL));
			for(int iPL = 0; iPL < m_lPortalLeaves.Len(); iPL++)
			{
				CBSP3_PortalLeafExt* pPL = &m_lPortalLeaves[iPL];
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

/*
	// Link spline-brushes to portal-leaves
	if (m_lspSplineBrushes.Len() && m_lPortalLeaves.Len())
	{
		MSCOPE(m_spSBLink, XR_BSPMODEL);

		m_spSBLink = DNew(CBSP3_LinkContext) CBSP3_LinkContext;
		if (!m_spSBLink) MemError("ModelChanged");
		m_spSBLink->Create(this, m_lspSplineBrushes.Len(), Min(65532, m_lspSplineBrushes.Len()*10+500), m_nStructureLeaves);

		for(int iSB = 0; iSB < m_lspSplineBrushes.Len(); iSB++)
		{
			try
			{
				m_spSBLink->Insert(iSB, m_lspSplineBrushes[iSB]->m_BoundBox, 0);
//				m_spSBLink->Insert(iSB, m_lspSplineBrushes[iSB]->m_BoundVertexBox);
			}
			catch(CCException)
			{
				if (m_lPortalLeaves.Len())
					if (!CheckTree_r(1))
						Error("Read", CStrF("BSP-Check failed after SB %d ok.", iSB))
					else
						LogFile(CStrF("(CXR_Model_BSP3::Read) BSP-Check after SB %d ok.", iSB));
				throw;
			}
		}
	}
*/
	// Dump SB-linkcontext
/*	{
		LogFile("---------------------------------------------------------");
		CBSP3_LinkContext* pLC = m_spSBLink;
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
			CBSP3_Node* pN = &m_lNodes[iNode];
			if (pN->IsLeaf())
			{
				int nF = pN->m_nFaces;
				int iiF = pN->m_iiFaces;
				LogFile(CStrF("PRECLEAN NODE %d, nF %d, iiF %d", iNode, nF, iiF));
				for(int i = 0; i < nF; i++)
				{
					CBSP3_Face* pF = &m_lFaces[m_liFaces[iiF + i]];
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
		

void CXR_Model_BSP3::LightDataSanityCheck()
{
	CBSP3_LightData* pLD = m_spLightData;
	if (!pLD)
		return;

	LogFile("(CXR_Model_BSP3::LightDataSanityCheck) Begin...");

	bool bErr = false;

	// Check m_lSV
	{
		int nSV = pLD->GetSVCount();
		for(int i = 0; i < nSV; i++)
		{
			const CBSP3_ShadowVolume& SV = pLD->m_plSV[i];

			CHECK_LD(SV.m_iPL, >=, m_lPortalLeaves.Len(), "");
			CHECK_LD(SV.m_iLight, >=, pLD->m_LightsCount, "");
//			CHECK_LD(SV.m_iVBase+SV.m_nVertices, >, pLD->m_lSVVertices.Len(), "");
//			CHECK_LD(SV.m_iTriBase+SV.m_nTriangles*3, >, pLD->m_liSVPrim.Len(), "");
//			CHECK_LD(SV.m_iLMC+SV.m_nLMC, >, pLD->m_lSLC.Len(), "");


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

		uint16 m_iSLC;				// Index of first Surface/Light cluster (CBSP3_SLCluster)
		uint16 m_nSLC;
			if (*/
		}
	}

	if (bErr)
		Error("LightDataSanityCheck", "Light data is insane!");

	LogFile("(CXR_Model_BSP3::LightDataSanityCheck) Done.");
}

void CXR_Model_BSP3::InitBound()
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

	for(int iSB = 0; iSB < m_lspSplineBrushes.Len(); iSB++)
		m_BoundBox.Expand(m_lspSplineBrushes[iSB]->m_BoundBox);


	m_BoundBox.m_Min -= CVec3Dfp32(0.0001f);
	m_BoundBox.m_Max += CVec3Dfp32(0.0001f);
}

void CXR_Model_BSP3::InitNodePlaneTypes()
{
	MAUTOSTRIP(CXR_Model_BSP_InitNodePlaneTypes, MAUTOSTRIP_VOID);
	int nPlanes = 0;
	int nOrt = 0;

	for(int i = 0; i < m_lNodes.Len(); i++)
	{
		CBSP3_Node* pN = &m_lNodes[i];
		if (pN->IsNode())
		{
			nPlanes++;

			int iPlane = pN->m_iPlane;
			CPlane3Dfp32* pP = &m_lPlanes[iPlane];

			int iType = 0;
			for(int k = 0; k < 3; k++)
			{
				if (M_Fabs(pP->n.k[k] - 1.0f) < 0.00000001f)
					iType = k+1;
				else if (M_Fabs(pP->n.k[k] + 1.0f) < 0.00000001f)
					iType = k+1+4;
			}
			if (iType) nOrt++;

			pN->m_Flags |= iType << XW_NODE_PLANETYPESHIFT;
		}
	}

//	if (m_lFaces.Len() > 500)
//		ConOut(CStrF("%d of %d planes were axis-aligned.", nOrt, nPlanes));
}

void CXR_Model_BSP3::Clear()
{
	MAUTOSTRIP(CXR_Model_BSP_Clear, MAUTOSTRIP_VOID);
	Error("Clear", "Not implemented.");
}

void CXR_Model_BSP3::Create_ViewInstances(int _nInst)
{
	MAUTOSTRIP(CXR_Model_BSP_Create_ViewInstances, MAUTOSTRIP_VOID);
	MSCOPE(Create_ViewInstances, XR_BSPMODEL);

	m_lspViews.Clear();
	m_lspViews.SetLen(_nInst);

	for(int i = 0; i < _nInst; i++)
	{
		spCBSP3_ViewInstance spV = MNew(CBSP3_ViewInstance);
		if (spV == NULL) MemError("Create");
		spV->m_lPortalIDStates.SetLen(XW_MAXUSERPORTALS >> 3);

		m_lspViews[i] = spV;
		View_Reset(i);
	}
	m_pView = NULL;
	m_iView = 0;
}


// -------------------------------------------------------------------
//  Read/Write
// -------------------------------------------------------------------
void CXR_Model_BSP3::Create(const char* _pParam, CDataFile* _pDFile, CCFile*, const CBSP_CreateInfo& _CreateInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_Create, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP3::Create, XR_BSPMODEL);

	m_spMaster = ((CXR_Model_BSP3*)(CXR_Model*)_CreateInfo.m_spMaster);

	if (m_spMaster != NULL)
	{
		m_lspViews = m_spMaster->m_lspViews;
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
		m_pNodes	= m_lNodes.GetBasePtr();
	}

	{
		MSCOPE(m_liVertices, XR_BSPMODEL);
		if (!_pDFile->GetNext("VERTEXINDICES")) Error("Read", "No VERTEXINDICES entry.");
		int Len = _pDFile->GetUserData();
		m_liVertices.SetLen(Len); 

		if( 0x0123 == XWVersion )
		{
			// Indices stored as 32-bit
			uint32* piV = m_liVertices.GetBasePtr();
			int nRead = 0;
			while(nRead < Len)
			{
				uint16 liTemp[256];
				uint16 n = Min(256, Len-nRead);
				pFile->ReadLE(liTemp, n);
				for(int i = 0; i < n; i++)
				{
					piV[nRead + i] = liTemp[i];
				}
				nRead += n;
			}
		}
		else
		{
			pFile->ReadLE( m_liVertices.GetBasePtr(), m_liVertices.Len() );
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

#ifdef PLATFORM_DOLPHIN
		CBSP3_Face* pFaces = m_lFaces.GetBasePtr();
		for (int iFace=0; iFace < nFaces; iFace++)
			((CBSP3_CoreFace*)&pFaces[iFace])->Read(pFile, 0x0200);

#else
		M_ASSERT(_pDFile->GetUserData2() == XW_FACE_VERSION && 
			sizeof(CBSP3_CoreFace) * nFaces == _pDFile->GetEntrySize(), "!");
		CBSP3_Face* pFaces = m_lFaces.GetBasePtr();
		for (int iFace=0; iFace < nFaces; iFace++)
		{
			pFile->Read(&pFaces[iFace], sizeof(CBSP3_CoreFace));
#ifndef CPU_LITTLEENDIAN
			pFaces[iFace].SwapLE();
#endif
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
				pFile->ReadLE(liTemp, n);
				for(int i = 0; i < n; i++)
				{
					piF[nRead + i] = liTemp[i];
				}
				nRead += n;
			}
		}
		else
		{
			pFile->ReadLE( m_liFaces.GetBasePtr(), m_liFaces.Len() );
		}
//		pFile->Read(&m_liFaces[0], m_liFaces.ListSize());
//		SwitchArrayLE_uint32(m_liFaces.GetBasePtr(), m_liFaces.Len()); //AR-ADD: fix proper byte ordering for indices
	}

// URGENTFIXME: Omtimize loading
/*
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
	_pDFile->PushPosition();
	if (_pDFile->GetNext("EDGES"))
	{
		m_lEdges.SetLen(_pDFile->GetUserData());
		for(int i = 0; i < m_lEdges.Len(); i++)
			m_lEdges[i].Read(pFile);
	}
	_pDFile->PopPosition();

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
*/

	{
		MSCOPE(m_lNodes, XR_BSPMODEL);
		if (!_pDFile->GetNext("BSPNODES")) Error("Read", "No NODES entry.");

		int nNodes = _pDFile->GetUserData();
		int Ver = _pDFile->GetUserData2();
		m_lNodes.SetLen(nNodes); 
		CBSP3_Node* pN = m_lNodes.GetBasePtr();
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
	}

	{
		MSCOPE(m_lMediums, XR_BSPMODEL);
		if (!_pDFile->GetNext("MEDIUMS")) Error("Read", "No MEDIUMS entry.");
		m_lMediums.SetLen(_pDFile->GetUserData()); 
		{ for(int iMedium = 0; iMedium < m_lMediums.Len(); iMedium++) m_lMediums[iMedium].Read(pFile); }

		if (m_spMaster != NULL)
		{
			TThinArray<uint16> lMediumMapping;
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
				CBSP3_Face* pF = m_lFaces.GetBasePtr();
				int nFaces = m_lFaces.Len();
				for(int i = 0; i < nFaces; i++)
					pF[i].m_iBackMedium = lMediumMapping[pF[i].m_iBackMedium];
			}

			// Remap nodes
			{
				CBSP3_Node* pNodes = m_lNodes.GetBasePtr();
				int nNodes = m_lNodes.Len();
				for(int i = 0; i < nNodes; i++)
				{
					if (pNodes[i].IsLeaf())
						pNodes[i].m_iMedium = lMediumMapping[pNodes[i].m_iMedium];
				}
			}

			m_lMediums = m_spMaster->m_lMediums;
		}
	}

	
//	static int g_SplineBrushMem = 0;
//	static int g_nSplineBrushes = 0;
//	int dMem = MRTC_MemAvail();

	// Splinebrushes
/*
	{
		MSCOPE(SplineBrushes, XR_BSPMODEL);

		_pDFile->PushPosition();
		if (_pDFile->GetNext("SPLINEBRUSHES"))
		{
			int nSB = _pDFile->GetUserData();
			if (!_pDFile->GetSubDir()) Error("Read", "Invalid SPLINEBRUSHES entry.");
			m_lspSplineBrushes.SetLen(nSB);

			int iSB = 0;
			while(_pDFile->GetNext("SPLINEBRUSH"))
			{
				if (!_pDFile->GetSubDir()) Error("Read", "Invalid SPLINEBRUSH entry.");
				spCSplineBrush spSB = MNew(CSplineBrush);
				spSB->Read(_pDFile);
				m_lspSplineBrushes[iSB] = spSB;
				_pDFile->GetParent();
				iSB++;
//				g_nSplineBrushes++;
			}
			m_lspSplineBrushes.SetLen(iSB);

			_pDFile->GetParent();
		}
		_pDFile->PopPosition();
	}
*/
//	dMem -= MRTC_MemAvail();
//	g_SplineBrushMem += dMem;
//	LogFile(CStrF("%d SBRUSH SO FAR %d bytes, %d b/sbrush", g_nSplineBrushes, g_SplineBrushMem, g_SplineBrushMem / g_nSplineBrushes));

	// Portal-leaves
	_pDFile->PushPosition();
	if (_pDFile->GetNext("PORTALLEAVES2"))
	{
		MSCOPE(PortalLeaves2, XR_BSPMODEL);

		m_lPortalLeaves.SetLen(_pDFile->GetUserData());
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
			for(int iPL = 0; iPL < m_lPortalLeaves.Len(); iPL++)
				m_lPortalLeaves[iPL].Read(pFile, false);
		}
		_pDFile->PopPosition();
	}

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

	// Light normal-indices
	_pDFile->PushPosition();
	if (_pDFile->GetNext("LIGHTNORMALINDX"))
	{
		MSCOPE(m_liLightNormals, XR_BSPMODEL);

		m_liLightNormals.SetLen(_pDFile->GetUserData()); 
		pFile->ReadLE(m_liLightNormals.GetBasePtr(), m_liLightNormals.Len());
	}
	_pDFile->PopPosition();
/*
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
			m_lLightMapInfo.SetLen(nLightMapInfo); 
			M_ASSERT(m_lLightMapInfo.ListSize() == _pDFile->GetEntrySize(), "!");
			pFile->Read(m_lLightMapInfo.GetBasePtr(), m_lLightMapInfo.ListSize());
			CBSP3_LightMapInfo* pLightMapInfo = m_lLightMapInfo.GetBasePtr();
			for (int iLMI=0; iLMI < nLightMapInfo; iLMI++)
				pLightMapInfo[iLMI].SwapLE();
		}
		_pDFile->PopPosition();
	}

	if (m_lLightMapInfo.Len())
	{
		MSCOPE(LightMaps, XR_BSPMODEL);

		// LIGHTMAPS
		_pDFile->PushPosition();
		if (_pDFile->GetNext("LIGHTMAPS3"))
		{
			if (!_pDFile->GetSubDir())
				Error("Create", "Invalid LIGHTMAPS3 entry.");

			spCTextureContainer_VirtualXTC spLMTC = DNew(CTextureContainer_VirtualXTC) CTextureContainer_VirtualXTC;
			spLMTC->Create(_pDFile);
			m_spLMTC = spLMTC;

			m_lLMDimensions.SetLen(m_spLMTC->GetNumLocal());
			for(int i = 0; i < m_spLMTC->GetNumLocal(); i++)
			{
				CImage Desc;
				int nMipMaps;
				m_spLMTC->GetTextureDesc(i, &Desc, nMipMaps);
				m_lLMDimensions[i] = CPnt(Desc.GetWidth(), Desc.GetHeight());
			}
			
			_pDFile->GetParent();
		}
		_pDFile->PopPosition();

		if (!m_spLMTC)
		{
			Error("Create", "LIGHTMAPS3 not found. Legacy format?");
		}

	}
*/

	// Light vertices
	_pDFile->PushPosition();
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
	_pDFile->PopPosition();

	// LIGHT DATA
	m_spLightData = CBSP3_LightData::FindAndRead(_pDFile);
	if( m_spLightData )
	{
		MSCOPE(m_spLightData, XR_BSPMODEL);
		// Fuxored, Lightmap data lies independantly from other lightdata
		m_spLightData->ReadLightmap(_pDFile);
	}

	LightDataSanityCheck();

#ifdef MODEL_BSP_CHECKTREE
	if (m_lPortalLeaves.Len())
		if (!CheckTree_r(1))
			Error("Read", "BSP-Check failed. (1)")
		else
			LogFile("(CXR_Model_BSP3::Read) BSP-Check 1 ok.");
#endif

/*
	{
		if (m_lLightMapInfo.Len() > 0)
		{
			if (!m_spLMTC)
				Error("Create", "No lightmap TC. Legacy format?");
		}
		else if (m_spMaster != NULL)
		{
			// Share lightmap data from master
			m_spLMTC = m_spMaster->m_spLMTC;
			m_lLMDimensions = m_spMaster->m_lLMDimensions;
			m_lLightMapInfo = m_spMaster->m_lLightMapInfo;
			m_spLightMapContainer = m_spMaster->m_spLightMapContainer;
		}
	}
*/
//	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
//	if (!pTC) Error("Read", "No texture-context available.");

	// Init surfaces
	{
		MSCOPE(InitTextures, XR_BSPMODEL);
		for(int iSurf = 0; iSurf < m_lspSurfaces.Len(); iSurf++)
		{
			CXW_Surface* pS = m_lspSurfaces[iSurf];
			pS->InitTextures(false);	// Don't report failures.
		}
	}

//	if (m_lPortals.Len())
//		ConOutL(CStrF("(CXR_Model_BSP3::Read) %d texture coordinates created. (%d bytes)", m_lTVertices.Len(), m_lTVertices.ListSize() ));


	// Init spline-brush UV-coord scales.
/*
	{
		MSCOPE(SplineBrushinit, XR_BSPMODEL);
		for(int iSB = 0; iSB < m_lspSplineBrushes.Len(); iSB++)
		{
			CSplineBrush* pSB = m_lspSplineBrushes[iSB];
	//		pSB->m_lspFaceLightMaps.Clear();
			for(int iFace = 0; iFace < pSB->m_lFaces.Len(); iFace++)
			{
				CSplineBrush_Face* pF = &pSB->m_lFaces[iFace];

				CXW_Surface* pSurf = m_lspSurfaces[pF->m_iSurface];
				pF->m_pSurface = pSurf;
				
				CVec3Dfp32 RefDim = pSurf->GetTextureMappingReferenceDimensions();

				CVec2Dfp32 UVScale(1.0f / RefDim[0], 1.0f / RefDim[1]);
				pF->ScaleMapping(UVScale);

			}

			// URGENTFIXME: This needs to be done so that compiled spline-faces gets correct UV-coords.
			pSB->ModelChanged(true);
		}
	}
*/
	LightDataSanityCheck();

	InitializeListPtrs();
	Create_PostRead();
	InitializeListPtrs();

	LightDataSanityCheck();
}
