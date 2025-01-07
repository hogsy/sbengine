#include "PCH.h"

#include "WBSPModel.h"
#include "WBSPDef.h"

#include "MFloat.h"

#include "../../MSystem/Raster/MTextureContainers.h"


// #define MODEL_BSP_CHECKTREE

// -------------------------------------------------------------------
void CXR_Model_BSP::InitPortalBounds()
{
	MAUTOSTRIP(CXR_Model_BSP_InitPortalBounds, MAUTOSTRIP_VOID);
	CBSP_PortalExt* pP = m_lPortals.GetBasePtr();
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

void CXR_Model_BSP::ExpandFaceBoundBox_r(int _iNode, CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_ExpandFaceBoundBox_r, MAUTOSTRIP_VOID);
	CBSP_Node* pN = &m_lNodes[_iNode];
	if (pN->IsLeaf())
	{
		int iiFaces = pN->m_iiFaces;
		int nFaces = pN->m_nFaces;
		for(int iif = 0; iif < nFaces; iif++)
		{
			const CBSP_Face* pF = &m_lFaces[m_liFaces[iiFaces + iif]];
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

int CXR_Model_BSP::GetFaceList_r(int _iNode, uint32* _pDest, int _Pos, int _MaxFaces, int _iSurf, int _SurfFlags)
{
	MAUTOSTRIP(CXR_Model_BSP_GetFaceList_r, 0);
	CBSP_Node* pN = &m_pNodes[_iNode];
	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		if (nFaces > _MaxFaces) Error("GetFaceList_r", "Insufficient space in target array.");
		int iiFaces = pN->m_iiFaces;
		int nAdded = 0;
		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_liFaces[iiFaces + f];

			CBSP_Face* pF = &m_lFaces[iFace];

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

int CXR_Model_BSP::GetPLFaceList_r(int _iPL, uint32* _pDest, int _MaxFaces, int _iSurf, int _SurfFlags)
{
	MAUTOSTRIP(CXR_Model_BSP_GetPLFaceList_r, 0);
	const CBSP_PortalLeafExt& PL = m_lPortalLeaves[_iPL];

	int nFaces = PL.m_nFaces;
	if (nFaces > _MaxFaces) Error("GetFaceList_r", "Insufficient space in target array.");
	int iiFaces = PL.m_iiFaces;
	int nAdded = 0;
	for(int f = 0; f < nFaces; f++)
	{
		int iFace = m_liPLFaces[iiFaces + f];

		CBSP_Face* pF = &m_lFaces[iFace];

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


int CXR_Model_BSP::CompareFaces_Index(int _iFace1, int _iFace2)
{
	MAUTOSTRIP(CXR_Model_BSP_CompareFaces_Index, 0);
	const CBSP_Face* pF1 = &m_lFaces[_iFace1];
	const CBSP_Face* pF2 = &m_lFaces[_iFace2];

	int iSurf1 = pF1->m_iSurface;
	int iSurf2 = pF2->m_iSurface;
	if (iSurf1 < iSurf2)
		return -1;
	else if (iSurf1 > iSurf2)
		return 1;
	else
	{
		int iLMC1 = (pF1->m_Flags & XW_FACE_LIGHTMAP) ? 
			m_lLightMapInfo[pF1->m_iLightInfo].m_iLMC : -1;
		int iLMC2 = (pF2->m_Flags & XW_FACE_LIGHTMAP) ? 
			m_lLightMapInfo[pF2->m_iLightInfo].m_iLMC : -1;

		if (iLMC1 < iLMC2)
			return -1;
		else if (iLMC1 > iLMC2)
			return 1;
		else
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

int CXR_Model_BSP::CompareFaces(int _iFace1, int _iFace2)
{
	MAUTOSTRIP(CXR_Model_BSP_CompareFaces, 0);
	const CBSP_Face* pF1 = &m_lFaces[_iFace1];
	const CBSP_Face* pF2 = &m_lFaces[_iFace2];

	int iSurf1 = pF1->m_iSurface;
	int iSurf2 = pF2->m_iSurface;
	if (iSurf1 < iSurf2)
		return -1;
	else if (iSurf1 > iSurf2)
		return 1;
	else
	{
		int iLMC1 = (pF1->m_Flags & XW_FACE_LIGHTMAP) ? 
			m_lLightMapInfo[pF1->m_iLightInfo].m_iLMC : -1;
		int iLMC2 = (pF2->m_Flags & XW_FACE_LIGHTMAP) ? 
			m_lLightMapInfo[pF2->m_iLightInfo].m_iLMC : -1;

		if (iLMC1 < iLMC2)
			return -1;
		else if (iLMC1 > iLMC2)
			return 1;
		else
			return 0;
	}
}

int CXR_Model_BSP::CompareFaces(const CBSP_Face* _pF1, const CBSP_Face* _pF2)
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
		int iLMC1 = (_pF1->m_Flags & XW_FACE_LIGHTMAP) ? 
			m_lLightMapInfo[_pF1->m_iLightInfo].m_iLMC : -1;
		int iLMC2 = (_pF2->m_Flags & XW_FACE_LIGHTMAP) ? 
			m_lLightMapInfo[_pF2->m_iLightInfo].m_iLMC : -1;

		if (iLMC1 < iLMC2)
			return -1;
		else if (iLMC1 > iLMC2)
			return 1;
		else
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

class CFaceSortElem
{
public:
	CBSP_Face* m_pFace;
	CXR_Model_BSP* m_pObj;

	int Compare(const CFaceSortElem&) const;
};

int CFaceSortElem::Compare(const CFaceSortElem& _Elem) const
{
	MAUTOSTRIP(CFaceSortElem_Compare, 0);
	return m_pObj->CompareFaces(m_pFace, _Elem.m_pFace);
}

void CXR_Model_BSP::SortFaceList(uint32* _pSrc, uint32* _pDest, int _nF)
{
	MAUTOSTRIP(CXR_Model_BSP_SortFaceList, MAUTOSTRIP_VOID);
	if (!_nF) return;

	TArray_Sortable<CFaceSortElem> lSort;
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
class CFaceVBSortElem
{
public:
	int m_iFace;
	CXR_Model_BSP* m_pObj;

	int Compare(const CFaceVBSortElem&) const;
};

int CFaceVBSortElem::Compare(const CFaceVBSortElem& _Elem) const
{
	MAUTOSTRIP(CFaceVBSortElem_Compare, 0);
	const CBSP_Face& F0 = m_pObj->m_lFaces[m_iFace];
	const CBSP_Face& F1 = _Elem.m_pObj->m_lFaces[_Elem.m_iFace];

	if (F0.m_iVB < F1.m_iVB)
		return -1;
	else if (F0.m_iVB > F1.m_iVB)
		return 1;
	else
		return 0;
}

void CXR_Model_BSP::VBSortFaceList(uint32* _pSrc, uint32* _pDest, int _nF)
{
	MAUTOSTRIP(CXR_Model_BSP_VBSortFaceList, MAUTOSTRIP_VOID);
	if (!_nF) return;

	TArray_Sortable<CFaceVBSortElem> lSort;
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
int CXR_Model_BSP::CompareSurface(int _iFace, int _SBID)
{
	MAUTOSTRIP(CXR_Model_BSP_CompareSurface, 0);
	int iSB = SBELEM_BRUSH(_SBID);
	int iSBFace = SBELEM_FACE(_SBID);
	const CBSP_Face* pF1 = &m_lFaces[_iFace];
	CSplineBrush_Face* pF2 = &m_lspSplineBrushes[iSB]->m_lFaces[iSBFace];

	int iSurf1 = pF1->m_iSurface;
	int iSurf2 = pF2->m_iSurface;

	if (iSurf1 < iSurf2)
		return -1;
	else if (iSurf1 > iSurf2)
		return 1;
	else
	{
		int iLMC1 = (pF1->m_Flags & XW_FACE_LIGHTMAP) ? 
			m_lLightMapInfo[pF1->m_iLightInfo].m_iLMC : -1;
		int iLMC2 = (pF2->m_Flags & XW_FACE_LIGHTMAP) ? 
			m_lLightMapInfo[pF2->m_iLightInfo].m_iLMC : -1;
		if (iLMC1 < iLMC2)
			return -1;
		else if (iLMC1 > iLMC2)
			return 1;
		else
			return 0;
	}
}

// -------------------------------------------------------------------
int CXR_Model_BSP::CompareSBFaces(int _iSB1, int _iFace1, int _iSB2, int _iFace2)
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
		int iLMC1 = (pF1->m_Flags & XW_FACE_LIGHTMAP) ? 
			m_lLightMapInfo[pF1->m_iLightInfo].m_iLMC : -1;
		int iLMC2 = (pF2->m_Flags & XW_FACE_LIGHTMAP) ? 
			m_lLightMapInfo[pF2->m_iLightInfo].m_iLMC : -1;

		if (iLMC1 < iLMC2)
			return -1;
		else if (iLMC1 > iLMC2)
			return 1;
		else
			return 0;
	}
}

int CXR_Model_BSP::CompareSBIDs(int _SBID1, int _SBID2)
{
	MAUTOSTRIP(CXR_Model_BSP_CompareSBIDs, 0);
	return CompareSBFaces(SBELEM_BRUSH(_SBID1), SBELEM_FACE(_SBID1), SBELEM_BRUSH(_SBID2), SBELEM_FACE(_SBID2));
}


class CSBFaceSortElem
{
public:
	int m_iFace;
	int m_iSB;
	CXR_Model_BSP* m_pObj;

	int Compare(const CSBFaceSortElem&) const;
};

int CSBFaceSortElem::Compare(const CSBFaceSortElem& _Elem) const
{
	MAUTOSTRIP(CSBFaceSortElem_Compare, 0);
	return m_pObj->CompareSBFaces(m_iSB, m_iFace, _Elem.m_iSB, _Elem.m_iFace);
}

void CXR_Model_BSP::SortSBFaceList(uint32* _pSrc, uint32* _pDest, int _nF)
{
	MAUTOSTRIP(CXR_Model_BSP_SortSBFaceList, MAUTOSTRIP_VOID);
	TArray_Sortable<CSBFaceSortElem> lSort;
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

void CXR_Model_BSP::CreateSBTess(CBSP_VertexBuffer* _pVB, int _iV, int _iP, CSplineBrush* _pSB, int _iFace)
{
	MAUTOSTRIP(CXR_Model_BSP_CreateSBTess, MAUTOSTRIP_VOID);
	const int MaxVerts = 1024;
	CVec2Dfp32 STArray[MaxVerts];

	CVec3Dfp32* pV = &_pVB->m_lV[_iV];
	int nS, nT;
	_pSB->Tesselate(_iFace, MaxVerts, pV, STArray, nS, nT);
	CSplineBrush_Face* pF = &_pSB->m_lFaces[_iFace];
	int nV = nS*nT;

	if (_iV + nV > _pVB->m_lV.Len())
		Error("CreateSBTess", CStrF("Internal error. (%d+%d > %d)(%dx%d)", _iV, nV, _pVB->m_lV.Len(), nS, nT));

	// Transform UVs.
	if (_pVB->m_lTV2.Len())
		pF->EvalUVLM(nV, pV, STArray, &_pVB->m_lTV1[_iV], &_pVB->m_lTV2[_iV]);
	else
		pF->EvalUV(nV, pV, STArray, &_pVB->m_lTV1[_iV]);

	if (_pVB->m_lCol.Len())
		for(int i = 0; i < nV; i++)
			_pVB->m_lCol[_iV+i] = 0xffffffff;

	if (_pVB->m_lN.Len() && _pVB->m_lTangU.Len() && _pVB->m_lTangV.Len())
		_pSB->EvalTangentSpaceArrays(_iFace, nV, STArray, &_pVB->m_lN[_iV], &_pVB->m_lTangU[_iV], &_pVB->m_lTangV[_iV]);
	else if (_pVB->m_lN.Len())
		_pSB->EvalNormal(_iFace, nV, STArray, &_pVB->m_lN[_iV], true);

	pF->m_iVBBaseVertex = _iV;

/*
	pF->m_iPrim = m_lSBPrim.Len();

	{
		for(int s = 0; s < nS-1; s++)
		{
			m_lSBPrim.Add(CRC_RIP_TRISTRIP + ((nT*2+2) << 8));
			m_lSBPrim.Add(nT*2);
			for(int t = 0; t < nT; t++)
			{
				m_lSBPrim.Add(_iV + (s+1)*nT + t);
				m_lSBPrim.Add(_iV + s*nT + t);
			}
		}
	}

	pF->m_nPrim = m_lSBPrim.Len() - pF->m_iPrim;*/
//LogFile(CStrF("    SB Tess  iPrim %d, nPrim %d", pF->m_iPrim, pF->m_nPrim));
}

void CXR_Model_BSP::CreateSBPrim(int _iV, CSplineBrush* _pSB, int _iFace, int _iSBPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_CreateSBPrim, MAUTOSTRIP_VOID);
//	const int MaxVerts = 1024;
//	CVec2Dfp32 STArray[MaxVerts];

	int nS, nT;
	int nPrim, nV;
	_pSB->GetTesselationSpaceExt(_iFace, nV, nPrim, nS, nT);
	CSplineBrush_Face* pF = &_pSB->m_lFaces[_iFace];

	pF->m_iVBBaseVertex = _iV;
	pF->m_iPrim = _iSBPrim;

	uint16* pSBPrim = &m_lSBPrim[_iSBPrim];
	int nP = 0;
	{
		for(int s = 0; s < nS-1; s++)
		{
			pSBPrim[nP++] = CRC_RIP_TRISTRIP + ((nT*2+2) << 8);
			pSBPrim[nP++] = nT*2;
			for(int t = 0; t < nT; t++)
			{
				pSBPrim[nP++] = _iV + (s+1)*nT + t;
				pSBPrim[nP++] = _iV + s*nT + t;
			}
		}
	}

	pF->m_nPrim = nP;
	M_ASSERT(nP == nPrim, CStrF("%d != %d", nP, nPrim));
	M_ASSERT(_iSBPrim + nPrim <= m_lSBPrim.Len(), "!");

//LogFile(CStrF("    SB Tess  iPrim %d, nPrim %d", pF->m_iPrim, pF->m_nPrim));
}

int GetSurfaceFlagsAllVersions(CXW_Surface* _pSurf);
int GetSurfaceFlagsAllVersions(CXW_Surface* _pSurf)
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

void CXR_Model_BSP::CountVertexBuffer(uint32* _piFaces, int _nFaces, uint32* _pSBFaces, int _nSBFaces, int& _nVB, int& _nSBPrim)
{
	MAUTOSTRIP(CXR_Model_BSP_CountVertexBuffer, MAUTOSTRIP_VOID);
	// Counts the space required by PrepareVertexBuffer

//	int nF = _nFaces;

	// Count vertices
	int nPrim, nVert;
	RenderGetPrimCount(_piFaces, _nFaces, nVert, nPrim);

//	int iSBStartVert = nVert;
	int nSBPrim = 0;
	{
		for(int f = 0; f < _nSBFaces; f++)
		{
			int nV, nP;
			m_lspSplineBrushes[SBELEM_BRUSH(_pSBFaces[f])]->InitTesselation(m_VBSBTessLevel);
			m_lspSplineBrushes[SBELEM_BRUSH(_pSBFaces[f])]->GetTesselationSpace(SBELEM_FACE(_pSBFaces[f]), nV, nP);
			nSBPrim += nP;
			nVert += nV;
		}
	}


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

void CXR_Model_BSP::PrepareVertexBuffer(uint32* _piFaces, int _nFaces, uint32* _pSBFaces, int _nSBFaces, int& _iVB, int& _iSBPrim)
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
	{
		for(int f = 0; f < _nSBFaces; f++)
		{
			int nV, nP;
//			m_lspSplineBrushes[SBELEM_BRUSH(_pSBFaces[f])]->InitMinTesselation();
			m_lspSplineBrushes[SBELEM_BRUSH(_pSBFaces[f])]->InitTesselation(m_VBSBTessLevel);
			m_lspSplineBrushes[SBELEM_BRUSH(_pSBFaces[f])]->GetTesselationSpace(SBELEM_FACE(_pSBFaces[f]), nV, nP);
//LogFile(CStrF("%.8x, Prim %d, iVert %d, nV %d", _pSBFaces[f], nPrim, nVert, nV));
			nPrim += nP;
			nVert += nV;

		}
	}


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

	int iLMC = -1;
	int iSurf = 0;
	int bLightVertices = false;

	if (_nFaces)
	{
		iLMC = (m_lFaces[_piFaces[0]].m_Flags & XW_FACE_LIGHTMAP) ? 
				m_lLightMapInfo[m_lFaces[_piFaces[0]].m_iLightInfo].m_iLMC : -1;
		iSurf = m_lFaces[_piFaces[0]].m_iSurface;
		if (m_lFaces[_piFaces[0]].m_Flags & XW_FACE_LIGHTVERTICES)
			bLightVertices = true;
	}
	else if(_nSBFaces)
	{
		CSplineBrush_Face* pF = &m_lspSplineBrushes[SBELEM_BRUSH(_pSBFaces[0])]->m_lFaces[SBELEM_FACE(_pSBFaces[0])];

		iLMC = (pF->m_Flags & XW_FACE_LIGHTMAP) ? m_lLightMapInfo[pF->m_iLightInfo].m_iLMC : -1;
		iSurf = pF->m_iSurface;
	}
	else
		Error("PrepareVertexBuffer", "No data.");

	// Search for queue with same surface and lightmap
	int q;
	for(q = 0; q < m_lVBQueues.Len(); q++)
		if (m_lVBQueues[q].m_iSurface == iSurf &&
			m_lVBQueues[q].m_LMTextureID == iLMC) break;

	int iVBQueue = 0;
	if (q < m_lVBQueues.Len())
		iVBQueue = q;
	else
	{
		// Didn't find a queue, so we have to add a new one.
		CBSP_VBQueue Q;
		Q.m_iSurface = iSurf;
		Q.m_LMTextureID = iLMC;
		iVBQueue = m_lVBQueues.Add(Q);
	}

//	int iVB = m_lspVB.Add(spCBSP_VertexBuffer(NULL));
	m_lspVB[_iVB] = NULL;

	// Set VB index on all faces
	for(int i = 0; i < _nFaces; i++)
		m_lFaces[_piFaces[i]].m_iVB = _iVB;



	int Components = 1;
	int SurfFlagsAll = GetSurfaceFlagsAllVersions(m_lspSurfaces[iSurf]);

	if (!(m_lspSurfaces[iSurf]->m_Flags & XW_SURFFLAGS_INVISIBLE))
		Components |= 1;
	if (iLMC >= 0)
		Components |= 2;
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

		spCBSP_VBInfo spVBCI = MNew(CBSP_VBInfo);
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
			CBSP_Face& Face = m_lFaces[_piFaces[f]];
			Face.m_iiVBVertices = iV;
			//Face.m_nVBVertices = Face.m_nVertices;
			iV += Face.m_nVertices;
		}
	}

	// Create SB primitives
	int iV = iSBStartVert;

	if (_nSBFaces)
	{
//		m_lSBPrim.SetGrow(200000);

		for(int f = 0; f < _nSBFaces; f++)
		{
			int iSB = SBELEM_BRUSH(_pSBFaces[f]);
			int iFace = SBELEM_FACE(_pSBFaces[f]);
			CSplineBrush* pSB = m_lspSplineBrushes[iSB];
			int nV, nP;
			pSB->GetTesselationSpace(iFace, nV, nP);

			CreateSBPrim(iV, pSB, iFace, _iSBPrim);
			pSB->m_lFaces[iFace].m_iVB = _iVB;

			_iSBPrim += nP;
			iV += nV;
		}
	}

	if (iV != nVert)
		Error("PrepareVertexBuffer", CStrF("Internal error. (%d != %d)", iV, nVert));

	_iVB++;
}

void CXR_Model_BSP::CreateVertexBuffer(int _iVB)
{
	MAUTOSTRIP(CXR_Model_BSP_CreateVertexBuffer, MAUTOSTRIP_VOID);
	if (m_lspVB[_iVB] != NULL)
		return;

	CBSP_VBInfo& VBCI = *m_lspVBInfo[_iVB];

	spCBSP_VertexBuffer spVB = MNew(CBSP_VertexBuffer);
	if (!spVB) MemError("CreateVertexBuffer");
	m_lspVB[_iVB] = spVB;

	spVB->Create(VBCI.m_nVertices, VBCI.m_Components);

	// Create the vertex data
	Tesselate(VBCI.m_liFaces.GetBasePtr(), VBCI.m_liFaces.Len(), VBCI.m_iSBVertices, 
		spVB->m_lV.GetBasePtr(), spVB->m_lTV1.GetBasePtr(), spVB->m_lTV2.GetBasePtr(), spVB->m_lCol.GetBasePtr(), spVB->m_lN.GetBasePtr(), spVB->m_lTangU.GetBasePtr(), spVB->m_lTangV.GetBasePtr(), NULL, m_lspSurfaces[VBCI.m_iSurface]);

	int iV = VBCI.m_iSBVertices;

	if (VBCI.m_lSBFaces.Len())
	{
		const uint32* pSBFaces = VBCI.m_lSBFaces.GetBasePtr();
		for(int f = 0; f < VBCI.m_lSBFaces.Len(); f++)
		{
			int iSB = SBELEM_BRUSH(pSBFaces[f]);
			int iFace = SBELEM_FACE(pSBFaces[f]);
			CSplineBrush* pSB = m_lspSplineBrushes[iSB];
//			pSB->InitMinTesselation();
			pSB->InitTesselation(m_VBSBTessLevel);
			int nV, nP;
			pSB->GetTesselationSpace(iFace, nV, nP);

//LogFile(CStrF("iVB %d, %.8x, Vert %d+%d / %d", _iVB, pSBFaces[f], iV, nV, VBCI.m_nVertices));

			CreateSBTess(spVB, iV, -1, pSB, iFace);


			iV += nV;
		}
	}

	if (iV != VBCI.m_nVertices)
		Error("CreateVertexBuffer", CStrF("Internal error. (%d != %d)", iV, VBCI.m_nVertices));

//	LogFile(CStrF("    VertexBuffer: Surface %d, LMC %d, Faces %d, SBFaces %d, nV %d, nPrim %d", iSurf ,iLMC, nF, _nSBFaces, nVert, nPrim ));
}

CBSP_VertexBuffer* CXR_Model_BSP::GetVertexBuffer(int _iVB)
{
	MAUTOSTRIP(CXR_Model_BSP_GetVertexBuffer, NULL);
	CBSP_VertexBuffer* pBSPVB = m_lspVB[_iVB];
	if (!pBSPVB)
	{
		CreateVertexBuffer(_iVB);
		return m_lspVB[_iVB];
	}
	else
		return pBSPVB;
}


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

void CXR_Model_BSP::CreatePhysVertIndices()
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
		CBSP_Face* pF = &m_lFaces[i];
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

void CXR_Model_BSP::CountVBFP(uint32* _piFaces, int _nFaces, int& _nVBFP, int& _nFacePrim)
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
#ifdef MODEL_BSP_USEKNITSTRIP
			RenderGetPrimCount_KnitStrip(&_piFaces[iiFace], nFaces, nV, nPrim);
			// LogFile(CStrF("Count1 %d faces, %d prim, %d iiFace, _piFaces %.8x", nFaces, nPrim, iiFace, _piFaces));
#else
			RenderGetPrimCount(&_piFaces[iiFace], nFaces, nV, nPrim);
#endif

			_nFacePrim += nPrim;
			_nVBFP++;
		}

		iiFace = iiFace2;
	}
}

void CXR_Model_BSP::CalcBoundBox(const uint32* _piFaces, int _nFaces, CBox3Dfp32& _BoundBox)
{
	_BoundBox.m_Min = _FP32_MAX;
	_BoundBox.m_Max = -_FP32_MAX;
	const uint32* piV = m_liVertices.GetBasePtr();
	const CVec3Dfp32* pV = m_lVertices.GetBasePtr();
	for(int i = 0; i < _nFaces; i++)
	{
		const CBSP_Face& Face = m_lFaces[_piFaces[i]];
		for(int v = 0; v < Face.m_nVertices; v++)
			_BoundBox.Expand(pV[piV[v + Face.m_iiVertices]]);
	}
}

void CXR_Model_BSP::CreateVBFP(uint32* _piFaces, int _nFaces, int& _iVBFP, int& _iFacePrim)
{
	MAUTOSTRIP(CXR_Model_BSP_CreateVBFP, MAUTOSTRIP_VOID);
//	const int MaxVBFP = 512;
//	CBSP_VBFacePrim lVBFP[MaxVBFP];
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
#ifdef MODEL_BSP_USEKNITSTRIP
			RenderGetPrimCount_KnitStrip(&_piFaces[iiFace], nFaces, nV, nPrim);
			// LogFile(CStrF("Count2 %d faces, %d prim, %d iiFace, piFaces %.8x", nFaces, nPrim, iiFace, _piFaces));
#else
			RenderGetPrimCount(&_piFaces[iiFace], nFaces, nV, nPrim);
#endif

			CBSP_VBFacePrim& VBFP = m_lVBFacePrim[_iVBFP];
			VBFP.m_iPrim = _iFacePrim;
			VBFP.m_iVB = iVB;
			VBFP.m_nPrim = nPrim;

			m_lspVBInfo[iVB]->m_MaxPrim += (2 + sizeof(void *) / 2);

			M_ASSERT(VBFP.m_iPrim + nPrim <= m_lFacePrim.Len(), "!");

			CalcBoundBox(&_piFaces[iiFace], nFaces, VBFP.m_BoundBox);

#ifdef MODEL_BSP_USEKNITSTRIP
			int nP = RenderTesselatePrim_KnitStrip(&_piFaces[iiFace], nFaces, &m_lFacePrim[VBFP.m_iPrim]);
			// LogFile(CStrF("Build %d faces, %d prim", nFaces, nP));

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
		memcpy(&m_lVBFacePrim[m_lVBFacePrim.Len() - nVBFP], lVBFP, nVBFP*sizeof(CBSP_VBFacePrim));
	}*/
}

void CXR_Model_BSP::Create_PostRead()
{
	MAUTOSTRIP(CXR_Model_BSP_Create_PostRead, MAUTOSTRIP_VOID);
	MSCOPE(Create_PostRead, XR_BSPMODEL);
#ifdef MODEL_BSP_CHECKTREE
	if (m_lPortalLeaves.Len())
		if (!CheckTree_r(1))
			Error("ModelChanged", "BSP-Check failed. (1)")
		else
			LogFile("(CXR_Model_BSP::ModelChanged) BSP-Check 1 ok.");
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
	InitLightVolumeInfo();

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
			CBSP_Node* pN = &m_lNodes[i];
			if (pN->IsLeaf())
				LogFile(CStrF("Parent %.3d <- this %.3d ->   -/-  , Flg %.4x, PL %.3d, NL %.3d", pN->m_iNodeParent, i, pN->m_Flags, pN->m_iPortalLeaf, pN->m_iNavigationLeaf));
			else
				LogFile(CStrF("Parent %.3d <- this %.3d -> %.3d/%.3d, Flg %.4x, PL %.3d, NL %.3d", pN->m_iNodeParent, i, pN->m_iNodeFront, pN->m_iNodeBack, pN->m_Flags, pN->m_iPortalLeaf, pN->m_iNavigationLeaf));
		}

		for(i = 0; i < m_lPortalLeaves.Len(); i++)
		{
			CBSP_PortalLeaf* pPL = &m_lPortalLeaves[i];
			LogFile(CStrF("PL %.3d, Flg %.4x, Node %.3d, Medium %.8x", i, pPL->m_Flags, pPL->m_iNode, pPL->m_ContainsMedium));
		}

		for(i = 0; i < m_lPortals.Len(); i++)
		{
			CBSP_PortalExt* pP = &m_lPortals[i];
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
		//		LogFile(CStrF("(CXR_Model_BSP::ModelChanged) nSky %d, iSky %d", nSkyF, niPLF));
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
			CXR_PhysicsContext EnumContext;
			int nF = EnumFaces_All(&EnumContext, 1, liFaces.GetBasePtr(), nEnumFaces);
			EnumFaces_Untag(&EnumContext);

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
						int iLMC = (pF->m_Flags & XW_FACE_LIGHTMAP) ? m_lLightMapInfo[pF->m_iLightInfo].m_iLMC : -1;

						// Search for queue with same surface and lightmap
						int q;
						for(q = 0; q < m_lVBQueues.Len(); q++)
							if (m_lVBQueues[q].m_iSurface == pF->m_iSurface &&
								m_lVBQueues[q].m_LMTextureID == iLMC) break;

						if (q < m_lVBQueues.Len())
							pF->m_iVBQueue = q;
						else
						{
							// Didn't find a queue, so we have to add a new one.
							pF->m_iVBQueue = m_lVBQueues.Len();
							CBSP_VBQueue Q;
							Q.m_iSurface = pF->m_iSurface;
							Q.m_LMTextureID = iLMC;
							m_lVBQueues.Add(Q);
						}
					}
				}
			}

	//		m_lVBQueues.SetLen(iQueue);
	//		LogFile("-------------------------------------------------------------------");
	//		LogFile(CStrF("(CXR_Model_BSP::Read) %d VB Queues.", m_lVBQueues.Len() ));
	//		LogFile(CStrF("(CXR_Model_BSP::Read) %d VBs.", m_lspVB.Len() ));
	//		LogFile(CStrF("(CXR_Model_BSP::Read) %d VB Surface clusters.", m_lVBSurfClusters.Len() ));
	//		LogFile("-------------------------------------------------------------------");

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
//					CBSP_PortalLeafExt& PL = m_lPortalLeaves[iPL];
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
				CBSP_PortalLeafExt& PL = m_lPortalLeaves[iPL];
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

			M_ASSERT(iVBFP == nVBFP, "!");
			M_ASSERT(iFacePrim == nFacePrim, "!");

			// Debug
	/*		for(int iPL = 0; iPL < m_nStructureLeaves; iPL++)
			{
				CBSP_PortalLeafExt& PL = m_lPortalLeaves[iPL];
				LogFile(CStrF("PORTALLEAF %d, iVBFacePrims %d, nVBFacePrims %d", iPL, PL.m_iVBFacePrims, PL.m_nVBFacePrims));

				for(int i = 0; i < PL.m_nVBFacePrims; i++)
				{
					const CBSP_VBFacePrim& VBFP = m_lVBFacePrim[PL.m_iVBFacePrims + i];
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
	for(iPL = 0; iPL < m_lPortalLeaves.Len(); iPL++)
	{
		CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[iPL];
		if (pPL->m_nFaces)
		{
			uint32 iBase = m_lVertices.Len()-1;
			uint32 iMax = 0;
			for(int iiFace = 0; iiFace < pPL->m_nFaces; iiFace++)
			{
				CBSP_Face* pF = &m_lFaces[m_liPLFaces[pPL->m_iiFaces + iiFace]];
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
			m_lspFogBoxes.Add(spCBSP_FogBox(NULL));
			for(int iPL = 0; iPL < m_lPortalLeaves.Len(); iPL++)
			{
				CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[iPL];
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


	// Link spline-brushes to portal-leaves
	if (m_lspSplineBrushes.Len() && m_lPortalLeaves.Len())
	{
		MSCOPE(m_spSBLink, XR_BSPMODEL);

		m_spSBLink = MNew(CBSP_LinkContext);
		if (!m_spSBLink) MemError("ModelChanged");
		m_spSBLink->Create(this, m_lspSplineBrushes.Len(), Min(65532, m_lspSplineBrushes.Len()*10+500));

		for(int iSB = 0; iSB < m_lspSplineBrushes.Len(); iSB++)
		{
			M_TRY
			{
				m_spSBLink->Insert(iSB, m_lspSplineBrushes[iSB]->m_BoundBox, 0);
//				m_spSBLink->Insert(iSB, m_lspSplineBrushes[iSB]->m_BoundVertexBox);
			}
			M_CATCH(
			catch(CCException)
			{
				if (m_lPortalLeaves.Len())
					if (!CheckTree_r(1))
						Error("Read", CStrF("BSP-Check failed after SB %d ok.", iSB))
					else
						LogFile(CStrF("(CXR_Model_BSP::Read) BSP-Check after SB %d ok.", iSB));
				throw;
			}
			)
		}
	}

	// Setup rendering-queues.
	{
		MSCOPE(m_lFaceQueueHeap, XR_BSPMODEL);

		if(!m_lspVB.Len())
		{
			Error("Create_PostRead", "Empty BSP model.");
	/*		m_lspVB.SetLen(1);
			m_lspVB[0] = DNew(CBSP_VertexBuffer) CBSP_VertexBuffer;
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
			m_lFaceQueueHeap.SetLen(m_lFaces.Len());
			int Pos = 0;
			for(int i = 0; i < m_lVBFaceQueues.Len(); i++)
			{
				if (m_lVBFaceQueues[i].m_MaxElem)
				{
					m_lVBFaceQueues[i].m_pQueue = &m_lFaceQueueHeap[Pos];
					Pos += m_lVBFaceQueues[i].m_MaxElem;
				}
			}
			if (Pos != m_lFaceQueueHeap.Len())
				Error("ModelChanged", "Internal error.");
		}
	}


	// Dump SB-linkcontext
/*	{
		LogFile("---------------------------------------------------------");
		CBSP_LinkContext* pLC = m_spSBLink;
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
			CBSP_Node* pN = &m_lNodes[iNode];
			if (pN->IsLeaf())
			{
				int nF = pN->m_nFaces;
				int iiF = pN->m_iiFaces;
				LogFile(CStrF("PRECLEAN NODE %d, nF %d, iiF %d", iNode, nF, iiF));
				for(int i = 0; i < nF; i++)
				{
					CBSP_Face* pF = &m_lFaces[m_liFaces[iiF + i]];
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

void CXR_Model_BSP::InitBound()
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

void CXR_Model_BSP::InitNodePlaneTypes()
{
	MAUTOSTRIP(CXR_Model_BSP_InitNodePlaneTypes, MAUTOSTRIP_VOID);
	int nPlanes = 0;
	int nOrt = 0;

	for(int i = 0; i < m_lNodes.Len(); i++)
	{
		CBSP_Node* pN = &m_lNodes[i];
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

void CXR_Model_BSP::InitLightVolumeInfo()
{
	MAUTOSTRIP(CXR_Model_BSP_InitLightVolumeInfo, MAUTOSTRIP_VOID);
	CBSP_LightVolumeInfo* pLV = m_lLightVolumeInfo.GetBasePtr();

	int nG = 0;
	for(int i = 0; i < m_lLightVolumeInfo.Len(); i++)
	{
		// Set cell-ptr
		pLV[i].m_pCells = &m_lLightCells[pLV[i].m_iCells];

		// If this is the last entry, it must definitely have a NULL next ptr.
		if (i == m_lLightVolumeInfo.Len()-1)
		{
			pLV[i].m_pNext = NULL;
			break;
		}

		// Set pNext accordingly..
		if (nG > 1)
		{
			pLV[i].m_pNext = &pLV[i+1];
		}
		else
		{
			nG = pLV[i].m_nGrids;
			if (nG > 1)
				pLV[i].m_pNext = &pLV[i+1];
			else
				pLV[i].m_pNext = NULL;
		}

		nG--;
	}
}

void CXR_Model_BSP::Clear()
{
	MAUTOSTRIP(CXR_Model_BSP_Clear, MAUTOSTRIP_VOID);
	Error("Clear", "Not implemented.");
}

void CXR_Model_BSP::Create_ViewInstances(int _nInst)
{
	MAUTOSTRIP(CXR_Model_BSP_Create_ViewInstances, MAUTOSTRIP_VOID);
	MSCOPE(Create_ViewInstances, XR_BSPMODEL);

	m_lspViews.Clear();
	m_lspViews.SetLen(_nInst);

	for(int i = 0; i < _nInst; i++)
	{
		spCBSPModel_ViewInstance spV = MNew(CBSPModel_ViewInstance);
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
void CXR_Model_BSP::Create(const char* _pParam, CDataFile* _pDFile, CCFile*, const CBSP_CreateInfo& _CreateInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_Create, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP::Create, XR_BSPMODEL);

	m_spMaster = ((CXR_Model_BSP*)(CXR_Model*)_CreateInfo.m_spMaster);

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
			pFile->ReadLE(m_liVertices.GetBasePtr(), m_liVertices.Len());
		}
	}

	{
		MSCOPE(m_lFaces, XR_BSPMODEL);
		if (!_pDFile->GetNext("FACES")) Error("Read", "No FACES entry.");

		int nFaces = _pDFile->GetUserData();
		m_lFaces.SetLen(nFaces); 
//		for (int iFace=0; iFace < nFaces; iFace++)
//			m_lFaces[iFace].Read(pFile, _pDFile->GetUserData2());

#ifdef PLATFORM_DOLPHIN
		CBSP_Face* pFaces = m_lFaces.GetBasePtr();
		for (int iFace=0; iFace < nFaces; iFace++)
			((CBSP_CoreFace*)&pFaces[iFace])->Read(pFile, 0x0200);

#else
		M_ASSERT(_pDFile->GetUserData2() == XW_FACE_VERSION && 
			sizeof(CBSP_CoreFace) * nFaces == _pDFile->GetEntrySize(), "!");
		CBSP_Face* pFaces = m_lFaces.GetBasePtr();
		for (int iFace=0; iFace < nFaces; iFace++)
		{
			pFile->Read(&pFaces[iFace], sizeof(CBSP_CoreFace));
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
				pFile->Read(liTemp, n);
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
//		pFile->Read(&m_liFaces[0], m_liFaces.ListSize());
//		SwitchArrayLE_uint32(m_liFaces.GetBasePtr(), m_liFaces.Len()); //AR-ADD: fix proper byte ordering for indices
	}

	{
		MSCOPE(m_lNodes, XR_BSPMODEL);
		if (!_pDFile->GetNext("BSPNODES")) Error("Read", "No NODES entry.");

		int nNodes = _pDFile->GetUserData();
		int Ver = _pDFile->GetUserData2();
		m_lNodes.SetLen(nNodes); 
		CBSP_Node* pN = m_lNodes.GetBasePtr();
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
				CBSP_Face* pF = m_lFaces.GetBasePtr();
				int nFaces = m_lFaces.Len();
				for(int i = 0; i < nFaces; i++)
					pF[i].m_iBackMedium = lMediumMapping[pF[i].m_iBackMedium];
			}

			// Remap nodes
			{
				CBSP_Node* pNodes = m_lNodes.GetBasePtr();
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
				CBSP_LightMapInfo* pLightMapInfo = m_lLightMapInfo.GetBasePtr();
	#ifndef CPU_LITTLEENDIAN
				for (int iLMI=0; iLMI < nLightMapInfo; iLMI++)
					pLightMapInfo[iLMI].SwapLE();
	#endif
			}
			else
			{
				CBSP_LightMapInfo* pLightMapInfo = m_lLightMapInfo.GetBasePtr();
				for(int iLMI = 0; iLMI < nLightMapInfo; iLMI++)
					pLightMapInfo[iLMI].Read(pFile, Version);
			}

/*			m_lLightMapInfo.SetLen(_pDFile->GetUserData()); 
			for(int i = 0; i < m_lLightMapInfo.Len(); i++)
				m_lLightMapInfo[i].Read(pFile, 2);*/
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

			spCTextureContainer_VirtualXTC spLMTC = MNew(CTextureContainer_VirtualXTC);
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
			Error("Create", "LIGHTMAPCLUSTERS2 entry not found. Drop XW into XWC to update file format.");

		_pDFile->PopPosition();*/
	}

	// Light volumes
	{
		MSCOPE(LightVolumes, XR_BSPMODEL);

		_pDFile->PushPosition();
		if (_pDFile->GetNext("LIGHTVOLUMEINFO"))
		{
			m_lLightVolumeInfo.SetLen(_pDFile->GetUserData()); 
			for(int i = 0; i < m_lLightVolumeInfo.Len(); i++)
				m_lLightVolumeInfo[i].Read(pFile);
		}
		_pDFile->PopPosition();

		_pDFile->PushPosition();
		if (_pDFile->GetNext("LIGHTVOLUMES"))
		{
			int nLen = _pDFile->GetUserData();
			m_lLightCells.SetLen(nLen); 
#ifdef PLATFORM_DOLPHIN
			for (int i=0; i<nLen; i++)
				m_lLightCells[i].Read(pFile, 0x0100);
#else
			pFile->Read(m_lLightCells.GetBasePtr(), m_lLightCells.ListSize());
#ifndef CPU_LITTLEENDIAN
			for (int i=0; i<nLen; i++)
				m_lLightCells[i].SwapLE();
#endif
#endif
		}
		_pDFile->PopPosition();
	}

	// Light octtree
	{
		MSCOPE(LightOcttree, XR_BSPMODEL);
		m_spLightOcttree = CBSP_LightOcttree::FindAndRead(_pDFile);
	}

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

#ifdef MODEL_BSP_CHECKTREE
	if (m_lPortalLeaves.Len())
		if (!CheckTree_r(1))
			Error("Read", "BSP-Check failed. (1)")
		else
			LogFile("(CXR_Model_BSP::Read) BSP-Check 1 ok.");
#endif

	{
		if (m_lLightMapInfo.Len() > 0)
		{
			if (!m_spLMTC)
				Error("Create", "No lightmap TC. Legacy format?");

			// Create lightmap texture container, unless we already got one
/*			if (!m_spLMTC)
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

#ifdef NEVER
	{
		MSCOPE(m_lTVertices, XR_BSPMODEL);
		m_lTVertices.SetLen(nTC);
		int iTC = 0;
		for(int iFace = 0; iFace < m_lFaces.Len(); iFace++)
		{
			CBSP_Face* pFace = &m_lFaces[iFace];
			if (!(pFace->m_Flags & XW_FACE_VISIBLE)) continue;

			int nv = pFace->m_nVertices;
			int iiv = pFace->m_iiVertices;
			if (iiv + nv > m_liVertices.Len())
				Error("Read", CStrF("Face %d, Vertex-index-index out of range. (%d+%d > %d)", iFace, iiv, nv, m_liVertices.Len()));
			uint32* piV = &m_liVertices[0];
			CVec3Dfp32* pV = &m_lVertices[0];
			
			CXW_Surface* pSurf = m_lspSurfaces[pFace->m_iSurface];
			CVec3Dfp32 ReferenceDim = pSurf->GetTextureMappingReferenceDimensions();

			int iMapping = pFace->m_iMapping;
			fp32 TxtWidthInv = 1.0f / ReferenceDim[0];
			fp32 TxtHeightInv = 1.0f / ReferenceDim[1];
			CVec3Dfp32 UVec = m_lMappings[iMapping].m_U;
			CVec3Dfp32 VVec = m_lMappings[iMapping].m_V;
			fp32 UVecLenSqrInv = 1.0f/(UVec*UVec);
			fp32 VVecLenSqrInv = 1.0f/(VVec*VVec);
			fp32 UOffset = m_lMappings[iMapping].m_UOffset;
			fp32 VOffset = m_lMappings[iMapping].m_VOffset;

			fp32 MidPixelAdjust = 0.5f;
			fp32 MidPixelAdjustLM;
			fp32 TxtWidthInvLM;
			fp32 TxtHeightInvLM;
			fp32 UOffsetLM;
			fp32 VOffsetLM;
			int LMWidth = 0;
			int LMHeight = 0;
			int bUseLM = (pFace->m_Flags & XW_FACE_LIGHTMAP);
			const CXW_LMCluster* pLMC = NULL;
			const CBSP_LightMapInfo* pLMI = NULL;
			if (bUseLM)
			{
				int iLM = pFace->m_iLightInfo;
				pLMI = &m_lLightMapInfo[iLM];
				int iLMC = m_lLightMapInfo[iLM].m_iLMC;

				CImage LMRef;
				m_LightMapContainer.GetLightMap(iLM, LMRef);
				LMWidth = LMRef.GetWidth();
				LMHeight= LMRef.GetHeight();

				fp32 LMScale = pLMI->m_Scale;
				UOffsetLM = pLMI->m_OffsetU;
				VOffsetLM = pLMI->m_OffsetV;
				TxtWidthInvLM = 1.0f / (LMScale * m_lLMDimensions[iLMC].x);
				TxtHeightInvLM = 1.0f / (LMScale * m_lLMDimensions[iLMC].y);
				MidPixelAdjustLM = MidPixelAdjust*LMScale;
			}

			pFace->m_iTVertices = iTC;
			if (bUseLM) pFace->m_iLMTVertices = iTC + nv;

			CVec2Dfp32 TMin(_FP32_MAX);
			CVec2Dfp32 TMax(-_FP32_MAX);
			CVec2Dfp32 TLMMin(_FP32_MAX);
			CVec2Dfp32 TLMMax(-_FP32_MAX);
			int v;
			for(v = 0; v < nv; v++)
			{
				CVec3Dfp32* pS = &m_lVertices[m_liVertices[iiv+v]];
				CVec2Dfp32 TV;
				CVec2Dfp32 TVLM;

				fp32 UProj = (pS->k[0]*UVec.k[0] + pS->k[1]*UVec.k[1] + pS->k[2]*UVec.k[2])*UVecLenSqrInv;
				fp32 VProj = (pS->k[0]*VVec.k[0] + pS->k[1]*VVec.k[1] + pS->k[2]*VVec.k[2])*VVecLenSqrInv;
				TV.k[0] = (UProj + UOffset/* + MidPixelAdjust*/) * TxtWidthInv;
				TV.k[1] = (VProj + VOffset/* + MidPixelAdjust*/) * TxtHeightInv;
				if (TV.k[0] > TMax.k[0]) TMax.k[0] = TV.k[0];
				if (TV.k[1] > TMax.k[1]) TMax.k[1] = TV.k[1];
				if (TV.k[0] < TMin.k[0]) TMin.k[0] = TV.k[0];
				if (TV.k[1] < TMin.k[1]) TMin.k[1] = TV.k[1];
				m_lTVertices[iTC + v] = TV;

				if (bUseLM)
				{
					TVLM.k[0] = (UProj + UOffset + UOffsetLM + MidPixelAdjustLM) / fp32(pLMI->m_Scale);
					TVLM.k[1] = (VProj + VOffset + VOffsetLM + MidPixelAdjustLM) / fp32(pLMI->m_Scale);
					if (TVLM.k[0] > TLMMax.k[0]) TLMMax.k[0] = TVLM.k[0];
					if (TVLM.k[1] > TLMMax.k[1]) TLMMax.k[1] = TVLM.k[1];
					if (TVLM.k[0] < TLMMin.k[0]) TLMMin.k[0] = TVLM.k[0];
					if (TVLM.k[1] < TLMMin.k[1]) TLMMin.k[1] = TVLM.k[1];
					m_lTVertices[iTC + nv + v] = TVLM;
				}

	//LogFile(CStrF("Params[0] = %f", pSurf->m_lBumpMaps[0].m_Params[0]));
			}

			iTC += nv;
			if (bUseLM) iTC += nv;

			CVec2Dfp32 TMid = (TMin + TMax) * 0.5f;
			TMid.k[0] = RoundToInt(TMid.k[0]/16.0f)*16.0f;
			TMid.k[1] = RoundToInt(TMid.k[1]/16.0f)*16.0f;
			for(v = 0; v < nv; v++)
			{
				int iTC = pFace->m_iTVertices + v;
				m_lTVertices[iTC] -= TMid;
			}

			if (bUseLM)
			{
				CVec2Dfp32 TLMMid = (TLMMin + TLMMax) * 0.5f;
				TLMMid.k[0] = RoundToInt(TLMMid.k[0]);
				TLMMid.k[1] = RoundToInt(TLMMid.k[1]);
				for(v = 0; v < nv; v++)
				{
					int iTC = pFace->m_iLMTVertices + v;
	//				m_lTVertices[iTC] -= TLMMid;
			if (m_lTVertices[iTC][0] < 0.0f || m_lTVertices[iTC][1] < 0.0f ||
				m_lTVertices[iTC][0] > LMWidth || m_lTVertices[iTC][1] > LMHeight)
			{
				LogFile(CStrF("Face %d, v %d, %s, (%d,%d), %dx%d", iFace, v, (char*)m_lTVertices[iTC].GetString(), pLMI->m_LMCOffsetU, pLMI->m_LMCOffsetV, LMWidth, LMHeight));
			}

					m_lTVertices[iTC][0] = (m_lTVertices[iTC][0] + (fp32)pLMI->m_LMCOffsetU) / fp32(pLMC->m_Width);
					m_lTVertices[iTC][1] = (m_lTVertices[iTC][1] + (fp32)pLMI->m_LMCOffsetV) / fp32(pLMC->m_Height);
				}
			}
		}
	}
#endif

//	if (m_lPortals.Len())
//		ConOutL(CStrF("(CXR_Model_BSP::Read) %d texture coordinates created. (%d bytes)", m_lTVertices.Len(), m_lTVertices.ListSize() ));


	// Init spline-brush UV-coord scales.
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

				if (pF->m_Flags & XW_FACE_LIGHTMAP && m_spLMTC != NULL)
				{
					CBSP_LightMapInfo& LMI = m_lLightMapInfo[pF->m_iLightInfo];
					fp32 TxtW = m_lLMDimensions[LMI.m_iLMC].x;
					fp32 TxtH = m_lLMDimensions[LMI.m_iLMC].y;

	/*				LogFile(CStrF("SB %d, Face %d, iLMI %d, LMC %d, Dim %dx%d, Ofs %d,%d",
						iSB, iFace, pF->m_iLightInfo, LMI.m_iLMC, 
						LMI.m_LMCWidthHalf, LMI.m_LMCHeightHalf, 
						LMI.m_LMCOffsetXHalf, LMI.m_LMCOffsetYHalf));
	*/
					pF->m_LightMapping[0][0] = fp32(LMI.m_LMCWidthHalf*2 - 1) / TxtW;
					pF->m_LightMapping[0][1] = fp32(LMI.m_LMCHeightHalf*2 - 1) / TxtH;
					pF->m_LightMapping[1][0] = fp32(LMI.m_LMCOffsetXHalf*2 + 0.5f) / TxtW;
					pF->m_LightMapping[1][1] = fp32(LMI.m_LMCOffsetYHalf*2 + 0.5f) / TxtH;
				}
			}

			// URGENTFIXME: This needs to be done so that compiled spline-faces gets correct UV-coords.
			pSB->ModelChanged(true);
		}
	}

	InitializeListPtrs();
	Create_PostRead();
	InitializeListPtrs();
}

