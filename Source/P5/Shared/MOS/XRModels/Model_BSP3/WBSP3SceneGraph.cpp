#include "PCH.h"

#include "WBSP3Model.h"
#include "../../Classes/BitString/MBitString.h"

// -------------------------------------------------------------------
// -------------------------------------------------------------------
//  PVS cache management
// -------------------------------------------------------------------
#ifndef XW_PVS_NOCOMPRESS

int CXR_Model_BSP3::SceneGraph_PVSFindSlot(int _iPL)
{
	int* piPVSCachePL = m_liPVSCachePL.GetBasePtr();
	for(int i = 0; i < m_liPVSCachePL.Len(); i++)
		if (piPVSCachePL[i] == _iPL) return i;
	return -1;
}

int CXR_Model_BSP3::SceneGraph_PVSFindFreeSlot()
{
	int* piPVSCachePL = m_lnPVSCacheRef.GetBasePtr();
	for(int i = 0; i < m_lnPVSCacheRef.Len(); i++)
		if (!m_lnPVSCacheRef[i]) return i;
	return -1;
}

#endif

void CXR_Model_BSP3::SceneGraph_PVSInitCache(int _NumCache)
{
	MSCOPE(CXR_Model_BSP3::SceneGraph_PVSInitCache, XR_BSPMODEL);

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

int CXR_Model_BSP3::SceneGraph_PVSGetLen()
{
	return (m_nStructureLeaves >> 3)+2;
}

bool CXR_Model_BSP3::SceneGraph_PVSGet(int _PVSType, const CVec3Dfp32& _Pos, uint8* _pDst)
{
	int iNode = GetPortalLeaf(_Pos);
	if (iNode == -1) return false;

#ifdef XW_PVS_NOCOMPRESS
	if (m_lPVS.Len())
	{
		CBSP3_PortalLeafExt* pPL = &m_lPortalLeaves[m_lNodes[iNode].m_iPortalLeaf];
		int Len = SceneGraph_PVSGetLen();
		memcpy(_pDst, &m_lPVS[pPL->m_iPVS], Len);
	}
	else
		return false;
#else
	CBSP3_PortalLeafExt* pPL = &m_lPortalLeaves[m_lNodes[iNode].m_iPortalLeaf];
	int PVSLen = CBS_Uncompress(&m_lPVS[pPL->m_iPVS], pPL->m_PVSBitLen, _pDst);
	if (((PVSLen+7) >> 3) > SceneGraph_PVSGetLen())
//	if ((PVSLen >> 3) >= SceneGraph_PVSGetLen())
		Error("SceneGraph_PVSGet", "PVS overrun.");
#endif
	return true;
}

const uint8* CXR_Model_BSP3::SceneGraph_PVSLock(int _PVSType, int _iPortalLeaf)
{
	MSCOPE(CXR_Model_BSP3::SceneGraph_PVSLock, XR_BSPMODEL);

#ifdef XW_PVS_NOCOMPRESS
	if (m_lPVS.Len())
	{
		CBSP3_PortalLeafExt* pPL = &m_lPortalLeaves[_iPortalLeaf];
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
		CBSP3_PortalLeafExt* pPL = &m_lPortalLeaves[_iPortalLeaf];
		int PVSLen = CBS_Uncompress(&m_lPVS[pPL->m_iPVS], pPL->m_PVSBitLen, m_lPVSCache[iCache].GetBasePtr());
		if (((PVSLen+7) >> 3) > m_lPVSCache[iCache].Len())
			Error("SceneGraph_PVSLock", CStrF("PVS overrun. (%d >> 3 = %d >= %d)", PVSLen, PVSLen >> 3, m_lPVSCache[iCache].Len()) );
//		ConOut(CStrF("PVS Uncompress PL %d, Len %d", _iPortalLeaf, PVSLen));
	}

	m_lnPVSCacheRef[iCache]++;
	return m_lPVSCache[iCache].GetBasePtr();
#endif
}


const uint8* CXR_Model_BSP3::SceneGraph_PVSLock(int _PVSType, const CVec3Dfp32& _Pos)
{
	MSCOPE(CXR_Model_BSP3::SceneGraph_PVSLock, XR_BSPMODEL);

	if (!m_lPVS.Len())
		return NULL;

	int iNode = GetPortalLeaf(_Pos);
	if (iNode == -1) return NULL;
	return SceneGraph_PVSLock(_PVSType, m_lNodes[iNode].m_iPortalLeaf);
}

void CXR_Model_BSP3::SceneGraph_PVSRelease(const uint8* _pPVS)
{
	MSCOPE(CXR_Model_BSP3::SceneGraph_PVSRelease, XR_BSPMODEL);

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

int CXR_Model_BSP3::SceneGraph_GetPVSNode(const CVec3Dfp32& _Pos)
{
	return 0;
}


// -------------------------------------------------------------------
spCXR_SceneGraphInstance CXR_Model_BSP3::SceneGraph_CreateInstance()
{
	MSCOPE(CXR_Model_BSP3::SceneGraph_CreateInstance, XR_BSPMODEL);
	if (!m_lPortalLeaves.Len())
		Error("SceneGraph_CreateInstance", "Not a structure BSP model.");

	spCXR_SceneGraphInstance spSG = MNew1(CBSP3_SceneGraph, this);
	return spSG;
}

void CBSP3_SceneGraph::Create(int _ElemHeap, int _LightHeap, int _CreateFlags)
{
	MSCOPE(CBSP3_SceneGraph::Create, XR_BSPMODEL);
	if (_LightHeap > 255)
		Error("Create", "Max dynamic lights is 255.");

	m_Objects.Create(m_pModel, _ElemHeap, Min(0xffe0, Max(500, _ElemHeap*6)), m_pModel->m_nStructureLeaves, true);
	m_ObjectsNoShadow.Create(m_pModel, _ElemHeap, Min(0xffe0, Max(500, _ElemHeap*1)), m_pModel->m_nStructureLeaves, true);
	m_DeferredObjects.Create(_ElemHeap);

	CBSP3_LightData* pLD = m_pModel->m_spLightData;
	if (pLD)
	{
		CXR_Light* plLights = pLD->GetLights();
		int nStatic = pLD->m_LightsCount;
		m_iFirstDynamic = nStatic;
		m_lLights.SetLen(m_iFirstDynamic + _LightHeap);
		m_Lights.Create(m_pModel, m_lLights.Len(), Min(65532, Max(500, m_lLights.Len()*10)), m_pModel->m_nStructureLeaves, true);
		m_lDynamicLightClip.SetLen(_LightHeap);

		for(uint i = 0; i < nStatic; i++)
		{
			pLD->m_lLights[i].m_iLight = i;
			pLD->m_lLights[i].m_iLightf = fp32(i);
		}

		CXR_Light* pL = m_lLights.GetBasePtr();
		memcpy(pL, plLights, sizeof(CXR_Light)*pLD->m_LightsCount);
		m_nDynamic = 0;
		m_MaxDynamic = _LightHeap;

		m_LightGUIDMap.Create(m_lLights.Len(), 6);
		m_DeferredLights.Create(m_lLights.Len());

		for(int i = 1; i < nStatic; i++)
		{
//			pL[i].CalcBoundBox();
			pL[i].m_LightGUID = 0xf000 + i;
			m_LightGUIDMap.Insert(i, pL[i].m_LightGUID);
			LinkLight(pL[i]);
		}
	}

	m_ObjectLights.Create(m_pModel, _ElemHeap, Min(65532, Max(500, _ElemHeap*1)), m_lLights.Len(), false);

	SceneGraph_CommitDeferred();
}

void CBSP3_SceneGraph::SceneGraph_CommitDeferred()
{
	MSCOPESHORT( CBSP3_SceneGraph::SceneGraph_CommitDeferred );
	// Deferr any elements the deferred lights touch
	{
		const CIndexPool16::CHashIDInfo* pLinks = m_DeferredLights.GetLinks();
		int iLink = m_DeferredLights.GetFirst();

		for(; iLink != -1; iLink = pLinks[iLink].m_iNext)
		{
			int iLight = pLinks[iLink].m_iElem;
			DeferrLightElements(&m_ObjectsNoShadow, iLight);
		}
	}

	// Link all deferred elements
	{
		const CIndexPool16::CHashIDInfo* pLinks = m_DeferredObjects.GetLinks();
		int iLink = m_DeferredObjects.GetFirst();

		for(; iLink != -1; iLink = pLinks[iLink].m_iNext)
		{
			int iElem = pLinks[iLink].m_iElem;
			Internal_LinkElement(iElem, m_ObjectsNoShadow.GetElementBox(iElem), m_ObjectsNoShadow.GetElementFlags(iElem));
		}
	}

	m_DeferredLights.RemoveAll();
	m_DeferredObjects.RemoveAll();
}

static inline int FindSV(const CBSP3_LightData* _pLD, int _iPL, int _iLight)
{
	const CBSP3_ShadowVolume* pSV = _pLD->m_plSV;

	if (_iPL >= _pLD->m_lnSVPL.Len())
		return 0;

	int iFirstSV = _pLD->m_liSVFirstPL[_iPL];
	int nSV = _pLD->m_lnSVPL[_iPL];

	for(int i = 0; i < nSV; i++)
		if (pSV[iFirstSV + i].m_iLight == _iLight)
			return iFirstSV + i;

	return 0;
}

int CBSP3_SceneGraph::EnumElementLights(const CBSP3_LinkContext* _pLinkCtx, uint16 _Elem, uint16* _piLights, int _nMaxLights) const
{
	int nLights = 0;

	// Get all lights affecting the object

	const CBSP3_LightData* pLD = m_pModel->m_spLightData;
	const CBSP3_Link* pLinks = m_Lights.GetLinks();
	const CPlane3Dfp32* pPlanes = pLD->m_plPlanes;

	const CBox3Dfp32& Box = _pLinkCtx->GetElementBox(_Elem);

	const CBSP3_Link* pObjLinks = _pLinkCtx->GetLinks();
	int iLinkPL = _pLinkCtx->GetLink_ID(_Elem);
	for(; iLinkPL; iLinkPL = pObjLinks[iLinkPL].m_iLinkNextPL)
	{
		int iPL = pObjLinks[iLinkPL].m_iPortalLeaf;

		{
			int iLink = m_Lights.GetLink_PL(iPL);
			for(; iLink; iLink = pLinks[iLink].m_iLinkNextObject)
			{
				const CXR_Light* pL = &m_lLights[pLinks[iLink].m_ID];

				// Check if this light has already been confirmed as affecting the object.
				int iLight = pL->m_iLight;
				int i = 0;
				for(; i < nLights; i++)
					if (iLight == _piLights[i])
						break;
				if (i < nLights)
					continue;

				if ((pL->GetIntensitySqrf() > 0.0001f) &&
					pL->m_BoundBox.m_Min[0] < Box.m_Max[0] &&
					pL->m_BoundBox.m_Min[1] < Box.m_Max[1] &&
					pL->m_BoundBox.m_Min[2] < Box.m_Max[2] &&
					pL->m_BoundBox.m_Max[0] > Box.m_Min[0] &&
					pL->m_BoundBox.m_Max[1] > Box.m_Min[1] &&
					pL->m_BoundBox.m_Max[2] > Box.m_Min[2])
				{
					int iSV = FindSV(pLD, iPL, pL->m_iLight);
					if (iSV)
					{
						const CBSP3_ShadowVolume& SV = pLD->m_plSV[iSV];
						if (SV.m_nPlaneLightBound)
						{
							int p = 0;
							for(; p < SV.m_nPlaneLightBound; p++)
							{
								if (pPlanes[SV.m_iPlaneLightBound + p].GetBoxMinDistance(Box.m_Min, Box.m_Max) > -MODEL_BSP_EPSILON)
									break;
							}

							if (p != SV.m_nPlaneLightBound)
								continue;
						}
						else
						{
							if (!Box.IsInside(SV.m_BoundBoxLight))
								continue;
						}

						// Add light
						if (nLights < _nMaxLights)
							_piLights[nLights++] = iLight;

					}
					else
					{
						if (pL->m_iLight >= m_iFirstDynamic)
						{
							if (pL->m_Type != CXR_LIGHTTYPE_SPOT || 
								m_lDynamicLightClip[pL->m_iLight-m_iFirstDynamic].IntersectBox(Box))
							{
								// Add light
								if (nLights < _nMaxLights)
									_piLights[nLights++] = iLight;
							}
						}
					}
				}
			}
		}
	}

	return nLights;
}

void CBSP3_SceneGraph::DeferrLightElements(CBSP3_LinkContext* _pLinkCtx, uint16 _iLight)
{
	MSCOPESHORT( CBSP3_SceneGraph::DeferrLightElements );
	// Get all lights affecting the object

	const CBSP3_LightData* pLD = m_pModel->m_spLightData;
	const CPlane3Dfp32* pPlanes = pLD->m_plPlanes;

	const CBSP3_Link* pLinks = m_Lights.GetLinks();
	int iLinkPL = m_Lights.GetLink_ID(_iLight);

	const CXR_Light* pL = &m_lLights[_iLight];

	for(; iLinkPL; iLinkPL = pLinks[iLinkPL].m_iLinkNextPL)
	{
		int iPL = pLinks[iLinkPL].m_iPortalLeaf;

		{
			const CBSP3_Link* pLinkObjs = _pLinkCtx->GetLinks();
			int iLinkObj = _pLinkCtx->GetLink_PL(iPL);

			for(; iLinkObj; iLinkObj = pLinkObjs[iLinkObj].m_iLinkNextObject)
			{
				int iElem = pLinkObjs[iLinkObj].m_ID;
				if (!(_pLinkCtx->GetElementFlags(iElem) & CXR_SCENEGRAPH_SHADOWCASTER))
					continue;

				const CBox3Dfp32& Box = _pLinkCtx->GetElementBox(iElem);

				// Check if this light has already been confirmed as affecting the element.
/*				int i = 0;
				for(; i < nElem; i++)
					if (iElem == _piElem[i])
						break;
				if (i < nElem)
					continue;*/

				if (pL->m_BoundBox.m_Min[0] < Box.m_Max[0] &&
					pL->m_BoundBox.m_Min[1] < Box.m_Max[1] &&
					pL->m_BoundBox.m_Min[2] < Box.m_Max[2] &&
					pL->m_BoundBox.m_Max[0] > Box.m_Min[0] &&
					pL->m_BoundBox.m_Max[1] > Box.m_Min[1] &&
					pL->m_BoundBox.m_Max[2] > Box.m_Min[2])
				{
					int iSV = FindSV(pLD, iPL, pL->m_iLight);
					if (iSV)
					{
						const CBSP3_ShadowVolume& SV = pLD->m_plSV[iSV];
						if (SV.m_nPlaneLightBound)
						{
							int p = 0;
							for(; p < SV.m_nPlaneLightBound; p++)
							{
								if (pPlanes[SV.m_iPlaneLightBound + p].GetBoxMinDistance(Box.m_Min, Box.m_Max) > -MODEL_BSP_EPSILON)
									break;
							}

							if (p != SV.m_nPlaneLightBound)
								continue;
						}
						else
						{
							if (!Box.IsInside(SV.m_BoundBoxLight))
								continue;
						}

						// Add elemenet
						m_DeferredObjects.Insert(iElem);
//						if (nElem < _nMaxElem)
//							_piElem[nElem++] = iElem;

					}
					else
					{
						if (pL->m_iLight >= m_iFirstDynamic)
						{
							if (pL->m_Type != CXR_LIGHTTYPE_SPOT || 
								m_lDynamicLightClip[pL->m_iLight-m_iFirstDynamic].IntersectBox(Box))
							{
								// Add elemenet
								m_DeferredObjects.Insert(iElem);
//								if (nElem < _nMaxElem)
//									_piElem[nElem++] = iElem;
							}
						}
					}
				}
			}
		}
	}
}

void CBSP3_SceneGraph::Internal_LinkElement(uint16 _Elem, const CBox3Dfp32& _Box, int _Flags)
{
	if (_Flags & CXR_SCENEGRAPH_SHADOWCASTER)
	{
		m_Objects.Insert(_Elem, _Box, _Flags);

		const int nMaxLights = 32;
		uint16 liLights[nMaxLights];

		int nLights = EnumElementLights(&m_Objects, _Elem, liLights, nMaxLights);

		CBox3Dfp32 BoxTotal(_Box);

		m_ObjectLights.InsertBegin(_Elem, _Flags);

		if (nLights)
		{
			for(int iL = 0; iL < nLights; iL++)
			{
				m_ObjectLights.AddPortalLeaf(liLights[iL]);	// Portal-leaf index == light index here since we're reusing the link class.


			}
		}
		m_ObjectLights.InsertEnd(_Elem);
	}
	else
	{
		m_Objects.Insert(_Elem, _Box, _Flags);
		m_ObjectsNoShadow.Remove(_Elem);
		m_ObjectLights.Remove(_Elem);
	}
}

void CBSP3_SceneGraph::SceneGraph_LinkElement(uint16 _Elem, const CBox3Dfp32& _Box, int _Flags)
{
	m_ObjectsNoShadow.Remove(_Elem);
	m_Objects.Remove(_Elem);
	m_ObjectLights.Remove(_Elem);
	m_ObjectsNoShadow.SetElementBox(_Elem, _Box);
	m_ObjectsNoShadow.SetElementFlags(_Elem, _Flags);
	m_DeferredObjects.Insert(_Elem);
}

void CBSP3_SceneGraph::SceneGraph_LinkInfiniteElement(uint16 _Elem, int _Flags)
{
	_Flags &= ~CXR_SCENEGRAPH_SHADOWCASTER;
#ifndef M_RTM
	if (_Flags & CXR_SCENEGRAPH_SHADOWCASTER)
	{
		ConOut("§cf80WARNING: (CBSP3_SceneGraph::SceneGraph_LinkInfiniteElement) Link-inifinte objects cannot be shadow casters.");
	}
#endif

	_Flags |= CXR_SCENEGRAPH_NOPORTALPVSCULL;
	m_ObjectLights.Remove(_Elem);
	m_ObjectsNoShadow.InsertInfinite(_Elem, _Flags);
	m_Objects.InsertInfinite(_Elem, _Flags);
	m_DeferredObjects.Remove(_Elem);
}

void CBSP3_SceneGraph::SceneGraph_UnlinkElement(uint16 _Elem)
{
	m_ObjectsNoShadow.Remove(_Elem);
	m_Objects.Remove(_Elem);
	m_DeferredObjects.Remove(_Elem);
}

int CBSP3_SceneGraph::SceneGraph_EnumerateElementNodes(int _Elem, uint32* _piRetNodes, int _MaxRet)
{
	Error("SceneGraph_EnumerateElementNodes", "Not implemented.");
	return 0;
}

int CBSP3_SceneGraph::SceneGraph_EnumerateNodes(const uint32* _piNodes, int _nNodes, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem, int _nMergeElem)
{
	Error("SceneGraph_EnumerateNodes", "Not implemented.");
	return 0;
}

int CBSP3_SceneGraph::SceneGraph_EnumeratePVS(const uint8* _pPVS, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem, int _nMergeElem)
{
	return m_Objects.EnumPVS(_pPVS, _pRetElem, _MaxRet, _pMergeElem, _nMergeElem);
}

int CBSP3_SceneGraph::SceneGraph_EnumerateView(int _iView, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem, int _nMergeElem)
{
	return m_Objects.EnumVisible(m_pModel->m_lspViews[_iView], _pRetElem, _MaxRet, _pMergeElem, _nMergeElem);
}

void CBSP3_SceneGraph::UnlinkLight(int _iLight)
{
	const CBSP3_Link* pLinks = m_ObjectLights.GetLinks();
	int iLink = m_ObjectLights.GetLink_PL(_iLight);

	while(iLink)
	{
		int iObj = pLinks[iLink].m_ID;
		
		m_ObjectsNoShadow.Remove(iObj);
		m_Objects.Remove(iObj);
		m_DeferredObjects.Insert(iObj);

		iLink = pLinks[iLink].m_iLinkNextObject;
		m_ObjectLights.Remove(iObj);
	}

	m_Lights.Remove(_iLight);
}

void CBSP3_SceneGraph::LinkLight(CXR_Light& _Light)
{
	const uint8* pPVS = m_pModel->SceneGraph_PVSLock(0, _Light.GetPosition());
	if (pPVS)
	{
		m_Lights.InsertWithPVS(_Light.m_iLight, _Light.m_BoundBox, 0, pPVS);
		m_pModel->SceneGraph_PVSRelease(pPVS);
	}
	else
		m_Lights.Insert(_Light.m_iLight, _Light.m_BoundBox, 0);

	m_DeferredLights.Insert(_Light.m_iLight);
}

int CBSP3_SceneGraph::SceneGraph_Light_GetMaxIndex()
{
	return m_lLights.Len();
}

void CBSP3_SceneGraph::SceneGraph_Light_LinkDynamic(const CXR_Light& _Light)
{
	int iLight = m_LightGUIDMap.GetIndex(_Light.m_LightGUID);
	if (iLight < 0)
	{
		if (m_nDynamic >= m_MaxDynamic)
			return;

		iLight = m_iFirstDynamic + m_nDynamic;
		m_nDynamic++;

		m_LightGUIDMap.Insert(iLight, _Light.m_LightGUID);
	}

	CXR_Light& Light = m_lLights[iLight];
	Light = _Light;
	Light.m_iLight = iLight;
	Light.m_iLightf = fp32(iLight);
	if (Light.m_Type == CXR_LIGHTTYPE_SPOT)
	{
		CBSP3_SpotLightClip& Clip = m_lDynamicLightClip[iLight - m_iFirstDynamic];
		Clip.Create(&Light);
		Clip.GetBoundBox(Light.m_BoundBox);
	}
	else
		Light.CalcBoundBoxFast();

	const uint8* pPVS = m_pModel->SceneGraph_PVSLock(0, _Light.GetPosition());

	if (m_pModel->View_GetVisibleBoxWithPVS_Box(Light.m_BoundBox, pPVS, Light.m_BoundBox))
		LinkLight(Light);

	if (pPVS)
		m_pModel->SceneGraph_PVSRelease(pPVS);
}

void CBSP3_SceneGraph::SceneGraph_Light_AddPrivateDynamic(const CXR_Light& _Light)
{
	// Lights added with this method will not be returned when enumerating lights.
	// Use this function to reserve a light index for rendering the light manually.

	int iLight = m_LightGUIDMap.GetIndex(_Light.m_LightGUID);
	if (iLight < 0)
	{
		if (m_nDynamic >= m_MaxDynamic)
			return;

		iLight = m_iFirstDynamic + m_nDynamic;
		m_nDynamic++;

		m_LightGUIDMap.Insert(iLight, _Light.m_LightGUID);
	}

	CXR_Light& Light = m_lLights[iLight];
	Light = _Light;
	Light.m_iLight = iLight;
	Light.m_iLightf = fp32(iLight);
	Light.CalcBoundBoxFast();
}

void CBSP3_SceneGraph::SceneGraph_Light_ClearDynamics()
{
	for(int i = 0; i < m_nDynamic; i++)
	{
		UnlinkLight(m_iFirstDynamic + i);
		m_LightGUIDMap.Remove(m_iFirstDynamic + i);
//		m_Lights.Remove(m_iFirstDynamic + i);
	}

	m_nDynamic = 0;
}

int CBSP3_SceneGraph::SceneGraph_Light_GetIndex(int _LightGUID)
{
	int iLight = m_LightGUIDMap.GetIndex(_LightGUID);
	if (iLight == -1)
		return 0;
	else
		return iLight;
}

int CBSP3_SceneGraph::SceneGraph_Light_GetGUID(int _iLight)
{
	unsigned int iL = _iLight;
	if (iL >= m_lLights.Len())
		return 0;

	CXR_Light* pL = m_lLights.GetBasePtr();
	return pL[iL].m_LightGUID;
}

void CBSP3_SceneGraph::SceneGraph_Light_Unlink(int _iLight)
{
	M_ASSERT(_iLight >= 0 && _iLight < m_lLights.Len(), "!");

	m_LightGUIDMap.Remove(_iLight);
	m_Lights.Remove(_iLight);
}

void CBSP3_SceneGraph::SceneGraph_Light_SetIntensity(int _iLight, const CVec3Dfp32& _Intensity, bool _bIsOff)
{
	M_ASSERT(_iLight >= 0 && _iLight < m_lLights.Len(), "!");

	CXR_Light& Light = m_lLights[_iLight];
	Light.SetIntensity(_Intensity);
}

void CBSP3_SceneGraph::SceneGraph_Light_SetRotation(int _iLight, const CMat4Dfp32& _Rotation)
{
	M_ASSERT(_iLight >= 0 && _iLight < m_lLights.Len(), "!");

	CXR_Light& Light = m_lLights[_iLight];

	if (_iLight < m_iFirstDynamic && Light.m_Type != CXR_LIGHTTYPE_POINT)
	{
		ConOut(CStrF("§cf80WARNING: Attempting to rotate non-point static light %d. (Operation ignored)", _iLight));
		return;
	}

	CVec3Dfp32::GetRow(Light.m_Pos, 0) = CVec3Dfp32::GetRow(_Rotation, 0);
	CVec3Dfp32::GetRow(Light.m_Pos, 1) = CVec3Dfp32::GetRow(_Rotation, 1);
	CVec3Dfp32::GetRow(Light.m_Pos, 2) = CVec3Dfp32::GetRow(_Rotation, 2);

	// Only link if dynamic (rotating a static point light doesn't change it's linkage.)
	if (_iLight >= m_iFirstDynamic)
	{
		Light.CalcBoundBoxFast();
		LinkLight(Light);
	}
}

void CBSP3_SceneGraph::SceneGraph_Light_SetPosition(int _iLight, const CMat4Dfp32& _Pos)
{
	M_ASSERT(_iLight >= 0 && _iLight < m_lLights.Len(), "!");

	if (_iLight < m_iFirstDynamic)
	{
		ConOut(CStrF("§cf80WARNING: Attempting to move static light %d. (Operation ignored)", _iLight));
		return;
	}

	UnlinkLight(_iLight);
	CXR_Light& Light = m_lLights[_iLight];
	Light.m_Pos = _Pos;
	Light.CalcBoundBoxFast();
	LinkLight(Light);
}

void CBSP3_SceneGraph::SceneGraph_Light_SetProjectionMap(int _iLight, int _TextureID, const CMat4Dfp32* _pPos)
{
	M_ASSERT(_iLight >= 0 && _iLight < m_lLights.Len(), "!");

	CXR_Light& Light = m_lLights[_iLight];
	Light.SetProjectionMap(_TextureID, _pPos);
}

void CBSP3_SceneGraph::SceneGraph_Light_Get(int _iLight, CXR_Light& _Light)
{
	M_ASSERT(_iLight >= 0 && _iLight < m_lLights.Len(), "!");
	_Light = m_lLights[_iLight];
}

void CBSP3_SceneGraph::SceneGraph_Light_GetIntensity(int _iLight, CVec3Dfp32& _Intensity)
{
	unsigned int iL = _iLight;
	if (iL >= m_lLights.Len())
	{
		_Intensity = 1.0f;
		return;
	}

	CXR_Light* pL = m_lLights.GetBasePtr();
	if (pL->m_Flags & CXR_LIGHT_ENABLED)
	{
		CVec4Dfp32 LightColor = pL[iL].GetIntensityv();
		_Intensity[0] = LightColor[0];
		_Intensity[1] = LightColor[1];
		_Intensity[2] = LightColor[2];
	}
	else
		_Intensity = 0;
}

int CBSP3_SceneGraph::SceneGraph_Light_Enum(const CBox3Dfp32& _Box, const CXR_Light** _lpLights, int _nMaxLights)
{
	uint16 liLights[1024];

	int nLights = m_Lights.EnumBox(_Box, liLights, 1024, NULL, 0);
	if (nLights > _nMaxLights)
		nLights = _nMaxLights;

	for(int i = 0; i < nLights; i++)
		_lpLights[i] = &m_lLights[liLights[i]];

	return nLights;
}

int CBSP3_SceneGraph::SceneGraph_Light_Enum(const uint16* _piPL, int _nPL, const CBox3Dfp32& _Box, const CXR_Light** _lpLights, int _nMaxLights)
{
	uint16 liLights[1024];

	int nLights = m_Lights.EnumPortalLeafList((uint16*)_piPL, _nPL, liLights, 1024);
	if (nLights > _nMaxLights)
		nLights = _nMaxLights;

	for(int i = 0; i < nLights; i++)
		_lpLights[i] = &m_lLights[liLights[i]];

	return nLights;
}

