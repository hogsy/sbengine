#include "PCH.h"

#include "WBSPModel.h"
#include "WBSPDef.h"
//#include "MFloat.h"

#define XW_PORTAL_SEARCHED 128
#define XW_PORTAL_PATH 64

int CXR_Model_BSP::IntPath_GetPortalLeaf(const CVec3Dfp32& _v)
{
//LogFile(CStr("IntPath_GetPortalLeaf"));
	int iNode = 1;
	CBSP_Node* pN = m_lNodes.GetBasePtr();
	while(iNode)
	{
//LogFile(CStrF("        iNode %d, Flg %.8x", iNode, pN[iNode].m_Flags));
		if (pN[iNode].m_Flags & XW_NODE_PORTAL)
			if (pN[iNode].m_Flags & XW_NODE_NAVIGATION) break;
//		if (pN[iNode].IsLeaf()) return 0;
		fp32 d = m_lPlanes[pN[iNode].m_iPlane].Distance(_v);
		iNode = (d >= 0.0f) ? pN[iNode].m_iNodeFront : pN[iNode].m_iNodeBack;
	}
//LogFile(CStrF("        iNode %d, m_iNavigationLeaf %d", iNode, pN[iNode].m_iNavigationLeaf));

	return pN[iNode].m_iNavigationLeaf;
}

/*fp32 LineDistance(const CVec3Dfp32& _a0, const CVec3Dfp32& _a1, const CVec3Dfp32& _b0, const CVec3Dfp32& _b1)
{
	CVec3Dfp32 dA;
	a1.Sub(a0, dA);
	
}*/

fp32 BoxDistance(const CVec3Dfp32& v, const CBox3Dfp32 _Box)
{
	CVec3Dfp32 dV;
	for(int i = 0; i < 3; i++)
	{
		if (v.k[i] < _Box.m_Min.k[i])
			dV.k[i] = _Box.m_Min.k[i] - v.k[i];
		else if (v.k[i] > _Box.m_Max.k[i])
			dV.k[i] = v.k[i] - _Box.m_Max.k[i];
		else
			dV.k[i] = 0;
	}

	return dV.Length();
}


void GetMidPoint(const CVec3Dfp32* _pV, const uint32* _piV, int _nV, CVec3Dfp32& _Dst)
{
	CVec3Dfp32 V(0);
	for(int v = 0; v < _nV; v++)
		V += _pV[_piV[v]];
	V.Scale(1.0f / fp32(_nV), _Dst);
}

fp32 GetFenceDistance(const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, const CVec3Dfp32* _pV, const uint32* _piV, int _nV)
{
	CPlane3Dfp32 Plane;
	Plane.Create(_pV[_piV[0]], _pV[_piV[1]], _pV[_piV[2]]);

	fp32 d = _FP32_MAX;

	if (Plane.Distance(_v0) > 0.001f)
	{
		if (Plane.Distance(_v1) < -0.001f)
		{
			int iv1 = _piV[_nV-1];
			int v;
			for(v = 0; v < _nV; v++)
			{
				int iv2 = _piV[v];
				CPlane3Dfp32 P;
				P.Create(_pV[iv1], _pV[iv2], _v0);
				if (P.Distance(_v1) > 0.0f) break;
				int iv1 = iv2;
			}
			if (v == _nV)
			{
				ConOutL("Hit portal 0.");
				return 0;
			}
		}
	}
	else
	{
		if (Plane.Distance(_v1) > 0.001f)
		{
			int iv1 = _piV[_nV-1];
			int v;
			for(v = 0; v < _nV; v++)
			{
				int iv2 = _piV[v];
				CPlane3Dfp32 P;
				P.Create(_pV[iv1], _pV[iv2], _v0);
				if (P.Distance(_v1) < 0.0f) break;
				int iv1 = iv2;
			}
			if (v == _nV)
			{
				ConOutL("Hit portal 1.");
				return 0;
			}
		}
	}

/*	int iv1 = _piV[_nV-1];
	for(int v = 0; v < _nV; v++)
	{
		int iv2 = _piV[v];
		fp32 EdgeDist = CVec3Dfp32::Point2LineDistance(V, _pV[iv1], _pV[iv2]);
		if (EdgeDist < d) d = EdgeDist;
		int iv1 = iv2;
	}*/

//	return d;
	CVec3Dfp32 V(0);
	for(int v = 0; v < _nV; v++)
		V += _pV[_piV[v]];
	V *= (1.0f / fp32(_nV));

	return CVec3Dfp32::Point2LineDistance(V, _v0, _v1);
}

int CXR_Model_BSP::IntPath_FindPortal(int _iPL, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1)
{
	CBSP_PortalLeaf* pPL = &m_lPortalLeaves[_iPL];
	CVec3Dfp32 dV;
	_v1.Sub(_v0, dV);

	const CVec3Dfp32* pV = m_lVertices.GetBasePtr();

	fp32 BestDist = _FP32_MAX;
	int BestPortal = -1;

	for(int i = 0; i < pPL->m_nPortals; i++)
	{
		int iP = m_liPortals[pPL->m_iiPortals + i];
		const CBSP_Portal* pP = &m_lPortals[iP];
		const CPlane3Dfp32* pPlane = &m_lPlanes[pP->m_iPlane];
		int bFrontThis = (pP->m_iNodeFront == _iPL);
		fp32 Dot = pPlane->n * dV;
		if (!bFrontThis) Dot = -Dot;
		if (Dot < 0.0f)
		{
			fp32 Dist = GetFenceDistance(_v0, _v1, m_lVertices.GetBasePtr(), &m_liVertices[pP->m_iiVertices], pP->m_nVertices);
			if (Dist < BestDist)
			{
				BestDist = Dist;
				BestPortal = iP;
			}
		}
	}

	return BestPortal;
}

int CXR_Model_BSP::Path_GetNeighbourPortal(int _iPL1, int _iPL2)
{
	CBSP_PortalLeaf* pPL = &m_lPortalLeaves[_iPL1];
	for(int p = 0; p < pPL->m_nPortals; p++)
	{
		int iP = m_liPortals[p + pPL->m_iiPortals];
		CBSP_Portal* pP = &m_lPortals[iP];
		if (m_lNodes[pP->m_iNodeFront].m_iNavigationLeaf == _iPL1)
		{
			if (m_lNodes[pP->m_iNodeBack].m_iNavigationLeaf == _iPL2) return iP;
		}
		else
		{
			if (m_lNodes[pP->m_iNodeFront].m_iNavigationLeaf == _iPL2) return iP;
		}
	}
	return -1;
}

/*class CPathFindContext
{
public:*/
	bool Path_bComplete = false;
	uint16* Path_piPL = NULL;
	int Path_niPL = 0;
	int Path_MaxPL = 0;
	int Path_iPLDestination = 0;
	int Path_MaxEnterFlg = 0;
	int Path_MaxExitFlg = 0;

	int Path_NodeCounter = 0;
	int Path_PortalCounter = 0;
//}

void CXR_Model_BSP::IntPath_Build_r(int _iPL, int _iPathPL, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1)
{
	if (_iPathPL >= Path_MaxPL) return;

	Path_NodeCounter++;
	m_lPortalLeaves[_iPL].m_Flags |= XW_PORTAL_SEARCHED;
	m_lPortalLeaves[_iPL].m_iPathNode = _iPathPL;
	Path_piPL[_iPathPL] = _iPL;
//ConOutL(CStrF("(CXR_Model_BSP::IntPath_Build_r) Path[%d] = %d", _iPathPL, _iPL));

/*	int ip = Path_GetNeighbourPortal(_iPL, Path_iPLDestination);
	if (ip >= 0)
	{
		m_lPortalLeaves[Path_iPLDestination].m_iPathNode = _iPathPL+1;
		Path_piPL[_iPathPL+1] = Path_iPLDestination;
		Path_niPL = _iPathPL+2;
		Path_bComplete = true;
		return;
	}*/

	// List with 
	const int nMaxSortPortals = 16;
	uint16 liPortals[nMaxSortPortals];
	fp32 lPortalQuality[nMaxSortPortals];
	int nPortals = 0;

	CBSP_PortalLeaf* pPL = &m_lPortalLeaves[_iPL];
	CVec3Dfp32 dV;
	_v1.Sub(_v0, dV);

	const CVec3Dfp32* pV = m_lVertices.GetBasePtr();

	int i;
	for(i = 0; i < pPL->m_nPortals; i++)
	{
		Path_PortalCounter++;
		int iP = m_liPortals[pPL->m_iiPortals + i];
		/*const */CBSP_Portal* pP = &m_lPortals[iP];
		int iDstNode = (pPL->m_iNode == pP->m_iNodeFront) ? pP->m_iNodeBack : pP->m_iNodeFront;
		int iDstPL = m_lNodes[iDstNode].m_iNavigationLeaf;
		CBSP_PortalLeafExt* pDstPL = &m_lPortalLeaves[iDstPL];
		if (pDstPL->m_Flags & XW_PORTAL_SEARCHED) continue;
	
		int EnterFlags = pP->GetEnterFlags(pPL->m_iNode);
		if (EnterFlags > Path_MaxEnterFlg)
		{
//			LogFile(CStrF("Portal (to) rejected %d, PL %d -> %d, EnterFlg %.4x (%.4x)", iP, _iPL, iDstPL, pP->GetEnterFlags(pDstPL->m_iNode), pP->m_Flags));
			continue;
		}
//LogFile(CStrF("Portal (to) accepted %d, PL %d -> %d, EnterFlg %.4x (%.4x)", iP, _iPL, iDstPL, pP->GetEnterFlags(pDstPL->m_iNode), pP->m_Flags));

		if (pP->GetEnterFlags(pDstPL->m_iNode) > Path_MaxExitFlg)
		{
//			LogFile(CStrF("Portal (from )rejected %d, PL %d -> %d, EnterFlg %.4x (%.4x)", iP, _iPL, iDstPL, pP->GetEnterFlags(pDstPL->m_iNode), pP->m_Flags));
			continue;
		}
//LogFile(CStrF("Portal (from )accepted %d, PL %d -> %d, EnterFlg %.4x (%.4x)", iP, _iPL, iDstPL, pP->GetEnterFlags(pDstPL->m_iNode), pP->m_Flags));

		// Check if this is destination so we can stop looking.
		if (iDstPL == Path_iPLDestination)
		{
			m_lPortalLeaves[Path_iPLDestination].m_iPathNode = _iPathPL+1;
			Path_piPL[_iPathPL+1] = Path_iPLDestination;
			Path_niPL = _iPathPL+2;
			Path_bComplete = true;
			return;
		}

		{
			fp32 Dist = BoxDistance(_v1, pDstPL->m_FaceBoundBox);
			liPortals[nPortals] = iP;
			lPortalQuality[nPortals] = Dist;
			nPortals++;
			if (nPortals == nMaxSortPortals) break;
		}
	}

	// Sort
	for(i = 0; i < nPortals-1; i++)
		for(int j = i+1; j < nPortals; j++)
			if (lPortalQuality[j] < lPortalQuality[i])
			{
				Swap(lPortalQuality[j], lPortalQuality[i]);
				Swap(liPortals[j], liPortals[i]);
			}

	// Recurse
	for(int p = 0; p < nPortals; p++)
	{
		int iP = liPortals[p];
		CBSP_Portal* pP = &m_lPortals[iP];
//		pP->m_Flags |= XW_PORTAL_SEARCHED;
		
		CVec3Dfp32 Mid;
		GetMidPoint(m_lVertices.GetBasePtr(), &m_liVertices[pP->m_iiVertices], pP->m_nVertices, Mid);

		if (m_lNodes[pP->m_iNodeFront].m_iNavigationLeaf == _iPL)
		{
//			ConOutL(CStrF("(CXR_Model_BSP::IntPath_Build_r) Recurse %d => %d", _iPL, m_lNodes[pP->m_iNodeBack].m_iNavigationLeaf));
			IntPath_Build_r(m_lNodes[pP->m_iNodeBack].m_iNavigationLeaf, _iPathPL+1, Mid, _v1);
		}
		else
		{
//			ConOutL(CStrF("(CXR_Model_BSP::IntPath_Build_r) Recurse %d => %d", _iPL, m_lNodes[pP->m_iNodeFront].m_iNavigationLeaf));
			IntPath_Build_r(m_lNodes[pP->m_iNodeFront].m_iNavigationLeaf, _iPathPL+1, Mid, _v1);
		}

		if (Path_bComplete) break;
	}
}

int CXR_Model_BSP::IntPath_GetLowestPathIndexPortalLeaf(int _iPL)
{
	CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[_iPL];
	int iMinPath = pPL->m_iPathNode;
	int iMinPL = -1;
	for(int p = 0; p < pPL->m_nPortals; p++)
	{
		int iP = m_liPortals[pPL->m_iiPortals + p];
		CBSP_Portal* pP = &m_lPortals[iP];
		int iPLOther =  
			(m_lNodes[pP->m_iNodeFront].m_iNavigationLeaf == _iPL) ? 
			m_lNodes[pP->m_iNodeBack].m_iNavigationLeaf : 
			m_lNodes[pP->m_iNodeFront].m_iNavigationLeaf;
		CBSP_PortalLeafExt* pPLOther = &m_lPortalLeaves[iPLOther];
		if (pPLOther->m_Flags & XW_PORTAL_PATH)
			if ((pPLOther->m_iPathNode < iMinPath) &&
				(pP->GetEnterFlags(pPLOther->m_iNode) <= Path_MaxEnterFlg) &&
				(pP->GetEnterFlags(pPL->m_iNode) <= Path_MaxExitFlg))
			{
				iMinPath = pPLOther->m_iPathNode;
				iMinPL = iPLOther;
			}
	}
	return iMinPL;
}

int CXR_Model_BSP::IntPath_GetHighestPathIndexPortalLeaf(int _iPL)
{
	CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[_iPL];
	int iMaxPath = pPL->m_iPathNode;
	int iMaxPL = -1;
	for(int p = 0; p < pPL->m_nPortals; p++)
	{
		int iP = m_liPortals[pPL->m_iiPortals + p];
		CBSP_Portal* pP = &m_lPortals[iP];
		int iPLOther =  
			(m_lNodes[pP->m_iNodeFront].m_iNavigationLeaf == _iPL) ? 
			m_lNodes[pP->m_iNodeBack].m_iNavigationLeaf : 
			m_lNodes[pP->m_iNodeFront].m_iNavigationLeaf;
		CBSP_PortalLeafExt* pPLOther = &m_lPortalLeaves[iPLOther];
		if (pPLOther->m_Flags & XW_PORTAL_PATH)
			if ((pPLOther->m_iPathNode > iMaxPath) &&
				(pP->GetEnterFlags(pPL->m_iNode) <= Path_MaxEnterFlg) &&
				(pP->GetEnterFlags(pPLOther->m_iNode) <= Path_MaxExitFlg))
			{
				iMaxPath = pPLOther->m_iPathNode;
				iMaxPL = iPLOther;
			}
	}
	return iMaxPL;
}

int CXR_Model_BSP::IntPath_Optimize(uint16* _piPL, int _niPL)
{
	int j = 0;
	for(int i = 0; i < _niPL; i++)
	{
		_piPL[j] = _piPL[i];
		j++;
		int iPL = IntPath_GetHighestPathIndexPortalLeaf(_piPL[i]);
		if (iPL >= 0)
			while((_piPL[i+1] != iPL) && (i < _niPL-1)) i++;
	}
	return j;
}

int CXR_Model_BSP::Path_Create(const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _Flags, uint16* _piPL, int _MaxPL)
{
	ConOutL("(CXR_Model_BSP::Path_Create) Src " + _v0.GetString() + ", Dst " + _v1.GetString());

	// Dump nodes & portalleaves.
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
	}*/


	// Get source leaf
	int iPL0 = IntPath_GetPortalLeaf(_v0);
	if (!iPL0)
	{
		ConOutL("(CXR_Model_BSP::Path_Create) No source PL.");
		return false;
	}

	// Get destination leaf
	int iPL1 = IntPath_GetPortalLeaf(_v1);
	if (!iPL1)
	{
		ConOutL("(CXR_Model_BSP::Path_Create) No destination PL.");
		return false;
	}

	// Same leaf?
	if (iPL0 == iPL1)
	{
		ConOutL("(CXR_Model_BSP::Path_Create) No source == destination PL.");
		return false;
	}

ConOutL(CStrF("(CXR_Model_BSP::Path_Create) iPL1 %d, iPL2 %d", iPL0, iPL1));

	Path_NodeCounter = 0;
	Path_PortalCounter = 0;

	Path_bComplete = false;
	m_lPortalLeaves[iPL0].m_iPathNode = 0;
	Path_piPL = _piPL;
	Path_niPL = 0;
	Path_MaxPL = _MaxPL;
	Path_iPLDestination = iPL1;

	Path_MaxEnterFlg = XW_PORTAL_ENTERJUMP48;
	Path_MaxExitFlg = XW_PORTAL_ENTERJUMP96;

	IntPath_Build_r(iPL0, 0, _v0, _v1);

	{
		CBSP_PortalLeafExt* pPL = m_lPortalLeaves.GetBasePtr();
		for(int p = 0; p < Path_niPL; p++)
			pPL[Path_piPL[p]].m_Flags |= XW_PORTAL_PATH;
	}

ConOutL(CStrF("(CXR_Model_BSP::Path_Create) bComplete %d, Len %d, RecurseCount %d, PortalCount %d", Path_bComplete, Path_niPL, Path_NodeCounter, Path_PortalCounter));
	int nSearch1 = IntPath_Optimize(Path_piPL, Path_niPL);
	bool bComplete1 = Path_bComplete;

ConOutL(CStrF("(CXR_Model_BSP::Path_Create) Optimized len %d", Path_niPL));

	{
		CBSP_PortalLeafExt* pPL = m_lPortalLeaves.GetBasePtr();
		int nPL = m_lPortalLeaves.Len();
		for(int p = 0; p < nPL; p++)
			pPL[p].m_Flags &= ~(XW_PORTAL_SEARCHED | XW_PORTAL_PATH);
	}

	Path_NodeCounter = 0;
	Path_PortalCounter = 0;

	Path_bComplete = false;
	m_lPortalLeaves[iPL1].m_iPathNode = 0;
	uint16 BwPath[512];	
	Path_piPL = BwPath;
	Path_niPL = 0;
	Path_MaxPL = 512;
	Path_iPLDestination = iPL0;
	IntPath_Build_r(iPL1, 0, _v1, _v0);

	{
		CBSP_PortalLeafExt* pPL = m_lPortalLeaves.GetBasePtr();
		for(int p = 0; p < Path_niPL; p++)
			pPL[Path_piPL[p]].m_Flags |= XW_PORTAL_PATH;
	}

ConOutL(CStrF("(CXR_Model_BSP::Path_Create) bComplete %d, Len %d, RecurseCount %d, PortalCount %d", Path_bComplete, Path_niPL, Path_NodeCounter, Path_PortalCounter));
	int nSearch2 = IntPath_Optimize(Path_piPL, Path_niPL);

	{
		CBSP_PortalLeafExt* pPL = m_lPortalLeaves.GetBasePtr();
		int nPL = m_lPortalLeaves.Len();
		for(int p = 0; p < nPL; p++)
			pPL[p].m_Flags &= ~(XW_PORTAL_SEARCHED | XW_PORTAL_PATH);
	}

ConOutL(CStrF("(CXR_Model_BSP::Path_Create) Optimized len %d", Path_niPL));
	if (Path_bComplete)
	{
		if (bComplete1)
		{
			if (nSearch2 < nSearch1)
			{
				nSearch1 = nSearch2;
				for(int i = 0; i < nSearch2; i++)
					_piPL[nSearch2-i-1] = Path_piPL[i];
			}
		}
		else
		{
			nSearch1 = nSearch2;
			memcpy(_piPL, Path_piPL, nSearch2*sizeof(BwPath[0]));
			Path_bComplete = true;
		}
	}

	if (!Path_bComplete) nSearch1 = 0;
	return nSearch1;
}

void CXR_Model_BSP::Path_GetPortalLeaf(int _iPortalLeaf, CXR_PathPortalLeaf& _Leaf)
{
	CBSP_PortalLeaf* pPL = &m_lPortalLeaves[_iPortalLeaf];
	_Leaf.m_pPortalVertices = m_lVertices.GetBasePtr();
	_Leaf.m_piPortalVertices = m_liVertices.GetBasePtr();
	_Leaf.m_nPortals = pPL->m_nPortals;
	_Leaf.m_piPortals = &m_liPortals[pPL->m_iiPortals];
}

CXR_IndexedPortal* CXR_Model_BSP::Path_GetPortal(int _iPortal)
{
	return &m_lPortals[_iPortal];
}

