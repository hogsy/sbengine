#include "PCH.h"

#include "WBSPModel.h"
#include "../../Classes/BitString/MBitString.h"

// -------------------------------------------------------------------
//  PVS cache management
// -------------------------------------------------------------------
#ifndef XW_PVS_NOCOMPRESS

int CXR_Model_BSP::SceneGraph_PVSFindSlot(int _iPL)
{
	int* piPVSCachePL = m_liPVSCachePL.GetBasePtr();
	for(int i = 0; i < m_liPVSCachePL.Len(); i++)
		if (piPVSCachePL[i] == _iPL) return i;
	return -1;
}

int CXR_Model_BSP::SceneGraph_PVSFindFreeSlot()
{
	int* piPVSCachePL = m_lnPVSCacheRef.GetBasePtr();
	for(int i = 0; i < m_lnPVSCacheRef.Len(); i++)
		if (!m_lnPVSCacheRef[i]) return i;
	return -1;
}

#endif

void CXR_Model_BSP::SceneGraph_PVSInitCache(int _NumCache)
{
	MSCOPE(CXR_Model_BSP::SceneGraph_PVSInitCache, XR_BSPMODEL);

#ifndef XW_PVS_NOCOMPRESS
	m_lPVSCache.SetLen(_NumCache);
	m_liPVSCachePL.SetLen(_NumCache);
	m_lnPVSCacheRef.SetLen(_NumCache);
	for(int i = 0; i < _NumCache; i++)
	{
		m_lPVSCache[i].SetLen(SceneGraph_PVSGetLen()+1);
		m_liPVSCachePL[i] = -1;
		m_lnPVSCacheRef[i] = 0;
	}
#endif
}

int CXR_Model_BSP::SceneGraph_PVSGetLen()
{
	return (m_nStructureLeaves >> 3)+2;
}

bool CXR_Model_BSP::SceneGraph_PVSGet(int _PVSType, const CVec3Dfp32& _Pos, uint8* _pDst)
{
	int iNode = GetPortalLeaf(_Pos);
	if (iNode == -1) return false;

#ifdef XW_PVS_NOCOMPRESS
	if (m_lPVS.Len())
	{
		CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[m_lNodes[iNode].m_iPortalLeaf];
		int Len = SceneGraph_PVSGetLen();
		memcpy(_pDst, &m_lPVS[pPL->m_iPVS], Len);
	}
	else
		return false;
#else
	CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[m_lNodes[iNode].m_iPortalLeaf];
	int PVSLen = CBS_Uncompress(&m_lPVS[pPL->m_iPVS], pPL->m_PVSBitLen, _pDst);
	if (((PVSLen+7) >> 3) > SceneGraph_PVSGetLen())
//	if ((PVSLen >> 3) >= SceneGraph_PVSGetLen())
		Error("SceneGraph_PVSGet", "PVS overrun.");
#endif
	return true;
}

const uint8* CXR_Model_BSP::SceneGraph_PVSLock(int _PVSType, int _iPortalLeaf)
{
	MSCOPE(CXR_Model_BSP::SceneGraph_PVSLock, XR_BSPMODEL);

#ifdef XW_PVS_NOCOMPRESS
	if (m_lPVS.Len())
	{
		CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[_iPortalLeaf];
		return &m_lPVS[pPL->m_iPVS];
	}
	else
		return NULL;

#else

	int iCache = SceneGraph_PVSFindSlot(_iPortalLeaf);
	if (iCache == -1) iCache = SceneGraph_PVSFindFreeSlot();
	if (iCache == -1) Error("SceneGraph_PVSGet", "Out of cache-lines.");

	if (m_liPVSCachePL[iCache] != _iPortalLeaf)
	{
		m_liPVSCachePL[iCache] = _iPortalLeaf;
		CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[_iPortalLeaf];
		int PVSLen = CBS_Uncompress(&m_lPVS[pPL->m_iPVS], pPL->m_PVSBitLen, m_lPVSCache[iCache].GetBasePtr());
		if (((PVSLen+7) >> 3) > m_lPVSCache[iCache].Len())
			Error("SceneGraph_PVSLock", CStrF("PVS overrun. (%d >> 3 = %d >= %d)", PVSLen, PVSLen >> 3, m_lPVSCache[iCache].Len()) );
//		ConOut(CStrF("PVS Uncompress PL %d, Len %d", _iPortalLeaf, PVSLen));
	}

	m_lnPVSCacheRef[iCache]++;
	return m_lPVSCache[iCache].GetBasePtr();
#endif
}


const uint8* CXR_Model_BSP::SceneGraph_PVSLock(int _PVSType, const CVec3Dfp32& _Pos)
{
	MSCOPE(CXR_Model_BSP::SceneGraph_PVSLock, XR_BSPMODEL);

	int iNode = GetPortalLeaf(_Pos);
	if (iNode == -1) return false;
	return SceneGraph_PVSLock(_PVSType, m_lNodes[iNode].m_iPortalLeaf);
}

void CXR_Model_BSP::SceneGraph_PVSRelease(const uint8* _pPVS)
{
	MSCOPE(CXR_Model_BSP::SceneGraph_PVSRelease, XR_BSPMODEL);

#ifndef XW_PVS_NOCOMPRESS
	if (!_pPVS) return;
	for(int i = 0; i < m_lPVSCache.Len(); i++)
		if (m_lPVSCache[i].GetBasePtr() == _pPVS)
		{
			if (!m_lnPVSCacheRef[i])
				Error("SceneGraph_PVSRelease", "Cache-line does not have any references.");
			m_lnPVSCacheRef[i]--;
			return;
		}

	Error("SceneGraph_PVSRelease", "Invalid PVS pointer.");
#endif
}

int CXR_Model_BSP::SceneGraph_GetPVSNode(const CVec3Dfp32& _Pos)
{
	return 0;
}

// -------------------------------------------------------------------
spCXR_SceneGraphInstance CXR_Model_BSP::SceneGraph_CreateInstance()
{
	if (!m_lPortalLeaves.Len())
		Error("SceneGraph_CreateInstance", "Not a structure BSP model.");

	spCXR_SceneGraphInstance spSG = MNew1(CBSP_SceneGraph, this);
	return spSG;
}

void CBSP_SceneGraph::Create(int _ElemHeap, int _LightHeap, int _CreateFlags)
{
	m_Objects.Create(m_pModel, _ElemHeap, Min(65532, Max(500, _ElemHeap*3)));
}

void CBSP_SceneGraph::SceneGraph_LinkElement(uint16 _Elem, const CBox3Dfp32& _Box, int _Flags)
{
	m_Objects.Insert(_Elem, _Box, _Flags);
}

void CBSP_SceneGraph::SceneGraph_LinkInfiniteElement(uint16 _Elem, int _Flags)
{
	m_Objects.InsertInfinite(_Elem, _Flags);
}

void CBSP_SceneGraph::SceneGraph_UnlinkElement(uint16 _Elem)
{
	m_Objects.Remove(_Elem);
}

int CBSP_SceneGraph::SceneGraph_EnumerateElementNodes(int _Elem, uint32* _piRetNodes, int _MaxRet)
{
	Error("SceneGraph_EnumerateElementNodes", "Not implemented.");
	return 0;
}

int CBSP_SceneGraph::SceneGraph_EnumerateNodes(const uint32* _piNodes, int _nNodes, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem, int _nMergeElem)
{
	Error("SceneGraph_EnumerateNodes", "Not implemented.");
	return 0;
}

int CBSP_SceneGraph::SceneGraph_EnumeratePVS(const uint8* _pPVS, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem, int _nMergeElem)
{
	return m_Objects.EnumPVS(_pPVS, _pRetElem, _MaxRet, _pMergeElem, _nMergeElem);
}

int CBSP_SceneGraph::SceneGraph_EnumerateView(int _iView, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem, int _nMergeElem)
{
	return m_Objects.EnumVisible(m_pModel->m_lspViews[_iView], _pRetElem, _MaxRet, _pMergeElem, _nMergeElem);
}


void CBSP_SceneGraph::SceneGraph_Light_LinkDynamic(const CXR_Light& _Light)
{
}

void CBSP_SceneGraph::SceneGraph_Light_ClearDynamics()
{
}

void CBSP_SceneGraph::SceneGraph_Light_AddPrivateDynamic(const CXR_Light& _Light)
{
}

int CBSP_SceneGraph::SceneGraph_Light_GetIndex(int _LightGUID)
{
	return 0;
}

int CBSP_SceneGraph::SceneGraph_Light_GetGUID(int _iLight)
{
	return 0;
}


void CBSP_SceneGraph::SceneGraph_Light_Unlink(int _iLight)
{
}

void CBSP_SceneGraph::SceneGraph_Light_SetIntensity(int _iLight, const CVec3Dfp32& _Intensity, bool _bIsOff)
{
}

void CBSP_SceneGraph::SceneGraph_Light_SetRotation(int _iLight, const CMat4Dfp32& _Rotation)
{
}

void CBSP_SceneGraph::SceneGraph_Light_SetPosition(int _iLight, const CMat4Dfp32& _Pos)
{
}

void CBSP_SceneGraph::SceneGraph_Light_SetProjectionMap(int _iLight, int _TextureID, const CMat4Dfp32* _pPos)
{
}

void CBSP_SceneGraph::SceneGraph_Light_Get(int _iLight, CXR_Light& _Light)
{
	_Light = CXR_Light();
}

int CBSP_SceneGraph::SceneGraph_Light_Enum(const CBox3Dfp32& _Box, const CXR_Light** _lpLights, int _nMaxLights)
{
	return 0;
}

int CBSP_SceneGraph::SceneGraph_Light_Enum(const uint16* _piPL, int _nPL, const CBox3Dfp32& _Box, const CXR_Light** _lpLights, int _nMaxLights)
{
	return 0;
}

