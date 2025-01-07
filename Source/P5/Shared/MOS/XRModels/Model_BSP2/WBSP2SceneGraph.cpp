#include "PCH.h"

#include "WBSP2Model.h"
#include "../../Classes/BitString/MBitString.h"

// -------------------------------------------------------------------
// -------------------------------------------------------------------
//  PVS cache management
// -------------------------------------------------------------------
#ifndef XW_PVS_NOCOMPRESS

int CXR_Model_BSP2::SceneGraph_PVSFindSlot(int _iPL)
{
	int* piPVSCachePL = m_liPVSCachePL.GetBasePtr();
	for(int i = 0; i < m_liPVSCachePL.Len(); i++)
		if (piPVSCachePL[i] == _iPL) return i;
	return -1;
}

int CXR_Model_BSP2::SceneGraph_PVSFindFreeSlot()
{
	int* piPVSCachePL = m_lnPVSCacheRef.GetBasePtr();
	for(int i = 0; i < m_lnPVSCacheRef.Len(); i++)
		if (!m_lnPVSCacheRef[i]) return i;
	return -1;
}

#endif

void CXR_Model_BSP2::SceneGraph_PVSInitCache(int _NumCache)
{
	MSCOPE(CXR_Model_BSP2::SceneGraph_PVSInitCache, XR_BSPMODEL);

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

int CXR_Model_BSP2::SceneGraph_PVSGetLen()
{
	return (m_nStructureLeaves >> 3)+2;
}

bool CXR_Model_BSP2::SceneGraph_PVSGet(int _PVSType, const CVec3Dfp32& _Pos, uint8* _pDst)
{
	int iNode = GetPortalLeaf(_Pos);
	if (iNode == -1) return false;

#ifdef XW_PVS_NOCOMPRESS
	if (m_lPVS.Len())
	{
		CBSP2_PortalLeafExt* pPL = &m_lPortalLeaves[m_lNodes[iNode].m_iPortalLeaf];
		int Len = SceneGraph_PVSGetLen();
		memcpy(_pDst, &m_lPVS[pPL->m_iPVS], Len);
	}
	else
		return false;
#else
	CBSP2_PortalLeafExt* pPL = &m_lPortalLeaves[m_lNodes[iNode].m_iPortalLeaf];
	int PVSLen = CBS_Uncompress(&m_lPVS[pPL->m_iPVS], pPL->m_PVSBitLen, _pDst);
	if (((PVSLen+7) >> 3) > SceneGraph_PVSGetLen())
//	if ((PVSLen >> 3) >= SceneGraph_PVSGetLen())
		Error("SceneGraph_PVSGet", "PVS overrun.");
#endif
	return true;
}

const uint8* CXR_Model_BSP2::SceneGraph_PVSLock(int _PVSType, int _iPortalLeaf)
{
	MSCOPE(CXR_Model_BSP2::SceneGraph_PVSLock_1, XR_BSPMODEL);

#ifdef XW_PVS_NOCOMPRESS
	if (m_lPVS.Len())
	{
		if (_iPortalLeaf == -1)
			return NULL;
		CBSP2_PortalLeafExt* pPL = &m_lPortalLeaves[_iPortalLeaf];
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
		CBSP2_PortalLeafExt* pPL = &m_lPortalLeaves[_iPortalLeaf];
		int PVSLen = CBS_Uncompress(&m_lPVS[pPL->m_iPVS], pPL->m_PVSBitLen, m_lPVSCache[iCache].GetBasePtr());
		if (((PVSLen+7) >> 3) > m_lPVSCache[iCache].Len())
			Error("SceneGraph_PVSLock", CStrF("PVS overrun. (%d >> 3 = %d >= %d)", PVSLen, PVSLen >> 3, m_lPVSCache[iCache].Len()) );
//		ConOut(CStrF("PVS Uncompress PL %d, Len %d", _iPortalLeaf, PVSLen));
	}

	m_lnPVSCacheRef[iCache]++;
	return m_lPVSCache[iCache].GetBasePtr();
#endif
}


const uint8* CXR_Model_BSP2::SceneGraph_PVSLock(int _PVSType, const CVec3Dfp32& _Pos)
{
	MSCOPE(CXR_Model_BSP2::SceneGraph_PVSLock_2, XR_BSPMODEL);

	if (!m_lPVS.Len())
		return NULL;

	int iNode = GetPortalLeaf(_Pos);
	if (iNode == -1) return NULL;
	return SceneGraph_PVSLock(_PVSType, m_lNodes[iNode].m_iPortalLeaf);
}

void CXR_Model_BSP2::SceneGraph_PVSRelease(const uint8* _pPVS)
{
	MSCOPE(CXR_Model_BSP2::SceneGraph_PVSRelease, XR_BSPMODEL);

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

int CXR_Model_BSP2::SceneGraph_GetPVSNode(const CVec3Dfp32& _Pos)
{
	return 0;
}


// -------------------------------------------------------------------
spCXR_SceneGraphInstance CXR_Model_BSP2::SceneGraph_CreateInstance()
{
	if (!m_lPortalLeaves.Len())
		Error("SceneGraph_CreateInstance", "Not a structure BSP model.");

	spCXR_SceneGraphInstance spSG = MNew1(CBSP2_SceneGraph,this);
	return spSG;
}

CBSP2_SceneGraph::~CBSP2_SceneGraph()
{
	if (m_Objects.m_LinkHeap.MaxCapacity())
	{
		M_TRACEALWAYS("SceneGraph Report:\n");
		M_TRACEALWAYS("     Elements: %d\n", m_Objects.m_lIDLinkBox.Len() );
		M_TRACEALWAYS("     PortalLeaves: %d\n", m_Objects.m_lPLLinkMap.Len() );
		M_TRACEALWAYS("     MaxLights: %d\n", m_ObjectLights.m_lPLLinkMap.Len() );
		M_TRACEALWAYS("     Objects Min Avail Links: %d / %d, Linked IDs %d\n", m_Objects.m_LinkHeapMinAvail, m_Objects.m_LinkHeap.MaxCapacity(), m_Objects.GetNumLinkedIDs() );
		M_TRACEALWAYS("     ObjectsNoShadow Min Avail Links: %d / %d, Linked IDs %d\n", m_ObjectsNoShadow.m_LinkHeapMinAvail, m_ObjectsNoShadow.m_LinkHeap.MaxCapacity(), m_ObjectsNoShadow.GetNumLinkedIDs() );
		M_TRACEALWAYS("     ObjectLights Min Avail Links: %d / %d, Linked IDs %d\n", m_ObjectLights.m_LinkHeapMinAvail, m_ObjectLights.m_LinkHeap.MaxCapacity(), m_ObjectLights.GetNumLinkedIDs() );
		M_TRACEALWAYS("     Lights Min Avail Links: %d / %d, Linked IDs %d\n", m_Lights.m_LinkHeapMinAvail, m_Lights.m_LinkHeap.MaxCapacity(), m_Lights.GetNumLinkedIDs() );
		M_TRACEALWAYS("     Object links:\n");
//		m_Objects.TraceContents();
	}
}

void CBSP2_SceneGraph::Create(int _ElemHeap, int _LightHeap, int _CreationFlags)
{
	if (_LightHeap > 255)
		Error("Create", "Max dynamic lights is 255.");

	if (!IsPow2(_LightHeap))
		Error("Create", "Max dynamic lights must be power of 2.");

	m_iNextDynamic = 0;
	m_Objects.Create(m_pModel, _ElemHeap, Min(0xffe0, Max(500, _ElemHeap*12)), m_pModel->m_nStructureLeaves, true);
	m_ObjectsNoShadow.Create(m_pModel, _ElemHeap, Min(0xffe0, Max(500, _ElemHeap*4)), m_pModel->m_nStructureLeaves, true);
	m_DeferredObjects.Create(_ElemHeap);

	CBSP2_LightData* pLD = m_pModel->m_spLightData;
	if (pLD)
	{
		int nStatic = pLD->m_lLights.Len();
		m_iFirstDynamic = nStatic;
		m_lLights.SetLen(m_iFirstDynamic + _LightHeap);
		m_Lights.Create(m_pModel, m_lLights.Len(), Min(0xffe0, Max(500, m_lLights.Len()*10 + 2048)), m_pModel->m_nStructureLeaves, true);
		m_lDynamicLightClip.SetLen(_LightHeap);

		for(int i = 0; i < nStatic; i++)
		{
			pLD->m_lLights[i].m_iLight = i;
			pLD->m_lLights[i].m_iLightf = fp32(i);
		}

		CXR_Light* pL = m_lLights.GetBasePtr();
		memcpy(pL, pLD->m_lLights.GetBasePtr(), pLD->m_lLights.ListSize());
		m_MaxDynamic = _LightHeap;
		m_MaxDynamicAnd = m_MaxDynamic-1;

		for(int i = m_iFirstDynamic; i < m_iFirstDynamic + _LightHeap; i++)
		{
			m_lLights[i].m_iLight = i;
			m_lLights[i].m_iLightf = fp32(i);
			m_lLights[i].m_Flags = 0;
		}

		m_LightGUIDMap.Create(m_lLights.Len(), 6);
		m_DeferredLights.Create(m_lLights.Len());

		for(int i = 1; i < nStatic; i++)
		{
//			pL[i].CalcBoundBox();
			pL[i].m_LightGUID = 0xf000 + i;
			pL[i].m_pPVS = 0;
			m_LightGUIDMap.Insert(i, pL[i].m_LightGUID);
			LinkLight(pL[i]);
		}
	}

	m_ObjectLights.Create(m_pModel, _ElemHeap, Min(0xffe0, Max(500, _ElemHeap*1)), m_lLights.Len(), false);

	if (pLD && !(_CreationFlags & CXR_SCENEGRAPH_CREATIONFLAGS_DONTCLEARINTENSITY))
	{
//		CXR_Light* pL = m_lLights.GetBasePtr();
		int nStatic = pLD->m_lLights.Len();
		for(int i = 1; i < nStatic; i++)
		{
//			if (pL[i].m_Range < 768.0f)
				SceneGraph_Light_SetIntensity(i, 0, true);
		}
	}

	SceneGraph_CommitDeferred();
}

void CBSP2_SceneGraph::SceneGraph_CommitDeferred()
{
	M_LOCK(m_Lock);
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

static int FindSV(const CBSP2_LightData* _pLD, int _iPL, int _iLight)
{
	const CBSP2_ShadowVolume* pSV = _pLD->m_lSV.GetBasePtr();

	if (_iPL >= _pLD->m_lnSVPL.Len())
		return 0;

	int iFirstSV = _pLD->m_liSVFirstPL[_iPL];
	int nSV = _pLD->m_lnSVPL[_iPL];

	for(int i = 0; i < nSV; i++)
		if (pSV[iFirstSV + i].m_iLight == _iLight)
			return iFirstSV + i;

	return 0;
}

void CalcShadowBox(const CXR_Light& _Light, const CBox3Dfp32& _Box, CBox3Dfp32& _Shadow);

int CBSP2_SceneGraph::EnumElementLights(CBSP2_LinkContext* _pLinkCtx, uint16 _Elem, uint16* _piLights, int _nMaxLights)
{
	M_LOCK(m_Lock);
	int nLights = 0;

	// Get all lights affecting the object

	CBSP2_LightData* pLD = m_pModel->m_spLightData;

	CBox3Dfp32 Box = _pLinkCtx->GetElementBox(_Elem);

	const CBSP2_Link* pObjLinks = _pLinkCtx->GetLinks();
	uint iLinkPL = _pLinkCtx->GetLink_ID(_Elem);
	for(; iLinkPL; iLinkPL = pObjLinks[iLinkPL].m_iLinkNextPL)
	{
		uint16 iPL = pObjLinks[iLinkPL].m_iPortalLeaf;

		{
			const CBSP2_Link* pLinks = m_Lights.GetLinks();
			const CPlane3Dfp32* pPlanes = pLD->m_lPlanes.GetBasePtr();

			int iLink = m_Lights.GetLink_PL(iPL);
			for(; iLink; iLink = pLinks[iLink].m_iLinkNextObject)
			{
				CXR_Light* pL = &m_lLights[pLinks[iLink].m_ID];

				if(!(pL->m_Flags & CXR_LIGHT_ENABLED))
					continue;

				if(pL->GetIntensitySqrf() < 0.0001f)
					continue;

				// Check if this light has already been confirmed as affecting the object.
				uint16 iLight = pL->m_iLight;
				int i = 0;
				for(; i < nLights; i++)
					if (iLight == _piLights[i])
						break;
				if (i < nLights)
					continue;

				if(Box.IsInside(pL->m_BoundBox))
				{
					int iSV = FindSV(pLD, iPL, pL->m_iLight);
					if (iSV)
					{
						const CBSP2_ShadowVolume& SV = pLD->m_lSV[iSV];
						if (SV.m_nPlaneLightBound)
						{
							M_ASSERT(SV.m_nPlaneLightBound > 3, "!");
							fp32 Dist[4];
							Dist[0] = pPlanes[SV.m_iPlaneLightBound + 0].GetBoxMinDistance(Box.m_Min, Box.m_Max);
							Dist[1] = pPlanes[SV.m_iPlaneLightBound + 1].GetBoxMinDistance(Box.m_Min, Box.m_Max);
							Dist[2] = pPlanes[SV.m_iPlaneLightBound + 2].GetBoxMinDistance(Box.m_Min, Box.m_Max);
							Dist[3] = pPlanes[SV.m_iPlaneLightBound + 3].GetBoxMinDistance(Box.m_Min, Box.m_Max);

							if(Max(Max(Dist[0], Dist[1]), Max(Dist[2], Dist[3])) > -MODEL_BSP_EPSILON)
								continue;

							int p = 4;
							for(; p < SV.m_nPlaneLightBound; p++)
							{
								if (pPlanes[SV.m_iPlaneLightBound + p].GetBoxMinDistance(Box.m_Min, Box.m_Max) > -MODEL_BSP_EPSILON)
									break;
							}

							if (p < SV.m_nPlaneLightBound)
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

void CBSP2_SceneGraph::DeferrLightElements(CBSP2_LinkContext* _pLinkCtx, uint16 _iLight)
{
	// Get all lights affecting the object

	CBSP2_LightData* pLD = m_pModel->m_spLightData;
	const CPlane3Dfp32* pPlanes = pLD->m_lPlanes.GetBasePtr();

	const CBSP2_Link* pLinks = m_Lights.GetLinks();
	int iLinkPL = m_Lights.GetLink_ID(_iLight);

	CXR_Light* pL = &m_lLights[_iLight];

	for(; iLinkPL; iLinkPL = pLinks[iLinkPL].m_iLinkNextPL)
	{
		int iPL = pLinks[iLinkPL].m_iPortalLeaf;

		{
			const CBSP2_Link* pLinkObjs = _pLinkCtx->GetLinks();
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

				if(Box.IsInside(pL->m_BoundBox))
				{
					int iSV = FindSV(pLD, iPL, pL->m_iLight);
					if (iSV)
					{
						const CBSP2_ShadowVolume& SV = pLD->m_lSV[iSV];
						if (SV.m_nPlaneLightBound)
						{
							M_ASSERT(SV.m_nPlaneLightBound > 3, "!");
							fp32 Dist[4];
							Dist[0] = pPlanes[SV.m_iPlaneLightBound + 0].GetBoxMinDistance(Box.m_Min, Box.m_Max);
							Dist[1] = pPlanes[SV.m_iPlaneLightBound + 1].GetBoxMinDistance(Box.m_Min, Box.m_Max);
							Dist[2] = pPlanes[SV.m_iPlaneLightBound + 2].GetBoxMinDistance(Box.m_Min, Box.m_Max);
							Dist[3] = pPlanes[SV.m_iPlaneLightBound + 3].GetBoxMinDistance(Box.m_Min, Box.m_Max);

							if(Max(Max(Dist[0], Dist[1]), Max(Dist[2], Dist[3])) > -MODEL_BSP_EPSILON)
								continue;

							int p = 4;
							for(; p < SV.m_nPlaneLightBound; p++)
							{
								if (pPlanes[SV.m_iPlaneLightBound + p].GetBoxMinDistance(Box.m_Min, Box.m_Max) > -MODEL_BSP_EPSILON)
									break;
							}

							if (p < SV.m_nPlaneLightBound)
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

void CBSP2_SceneGraph::Internal_LinkElement(uint16 _Elem, const CBox3Dfp32& _Box, int _Flags)
{
	if (_Flags & CXR_SCENEGRAPH_SHADOWCASTER)
	{
		m_ObjectsNoShadow.Insert(_Elem, _Box, _Flags);

		const int nMaxLights = 32;
		uint16 liLights[nMaxLights];

		int nLights = EnumElementLights(&m_ObjectsNoShadow, _Elem, liLights, nMaxLights);

		CBox3Dfp32 BoxTotal(_Box);

		m_ObjectLights.InsertBegin(_Elem, _Flags);
		m_Objects.InsertBegin(_Elem, _Flags);

		{
			// Mirror linkage from m_ObjectsNoShadow to m_Objects
			const CBSP2_Link* pLinks = m_ObjectsNoShadow.GetLinks();
			int iLink = m_ObjectsNoShadow.GetLink_ID(_Elem);
			for(; iLink; iLink = pLinks[iLink].m_iLinkNextPL)
			{
				m_Objects.AddPortalLeaf(pLinks[iLink].m_iPortalLeaf);
			}
		}

		{
			// Add each shadow's PLs to object's linkage
			for(int iL = 0; iL < nLights; iL++)
			{
				m_ObjectLights.AddPortalLeaf(liLights[iL]);	// Portal-leaf index == light index here since we're reusing the link class.

				CBox3Dfp32 ShadowBox;
				const CXR_Light& Light = m_lLights[liLights[iL]];
				CalcShadowBox(Light, _Box, ShadowBox);

				if (Light.m_pPVS)
				{
					m_pModel->LinkBoxWithPVS_i(&m_Objects, 1, ShadowBox, Light.m_pPVS);
				}
			}
		}

		m_Objects.InsertEnd(_Elem);
		m_ObjectLights.InsertEnd(_Elem);
	}
	else
	{
		m_Objects.Insert(_Elem, _Box, _Flags);
		m_ObjectsNoShadow.Remove(_Elem);
		m_ObjectLights.Remove(_Elem);
	}
}

void CBSP2_SceneGraph::SceneGraph_LinkElement(uint16 _Elem, const CBox3Dfp32& _Box, int _Flags)
{
	M_LOCK(m_Lock);
	m_ObjectsNoShadow.Remove(_Elem);
	m_Objects.Remove(_Elem);
	m_ObjectLights.Remove(_Elem);
	m_ObjectsNoShadow.SetElementBox(_Elem, _Box);
	m_ObjectsNoShadow.SetElementFlags(_Elem, _Flags);
	m_DeferredObjects.Insert(_Elem);
}

void CBSP2_SceneGraph::SceneGraph_LinkInfiniteElement(uint16 _Elem, int _Flags)
{
	M_LOCK(m_Lock);
	_Flags &= ~CXR_SCENEGRAPH_SHADOWCASTER;
#ifndef M_RTM
	if (_Flags & CXR_SCENEGRAPH_SHADOWCASTER)
	{
		ConOut("§cf80WARNING: (CBSP2_SceneGraph::SceneGraph_LinkInfiniteElement) Link-inifinte objects cannot be shadow casters.");
	}
#endif

	_Flags |= CXR_SCENEGRAPH_NOPORTALPVSCULL;
	m_ObjectLights.Remove(_Elem);
	m_ObjectsNoShadow.InsertInfinite(_Elem, _Flags);
	m_Objects.InsertInfinite(_Elem, _Flags);
	m_DeferredObjects.Remove(_Elem);
}

void CBSP2_SceneGraph::SceneGraph_UnlinkElement(uint16 _Elem)
{
	M_LOCK(m_Lock);
	m_ObjectsNoShadow.Remove(_Elem);
	m_Objects.Remove(_Elem);
	m_DeferredObjects.Remove(_Elem);
}

int CBSP2_SceneGraph::SceneGraph_EnumerateElementNodes(int _Elem, uint32* _piRetNodes, int _MaxRet)
{
	Error("SceneGraph_EnumerateElementNodes", "Not implemented.");
	return 0;
}

int CBSP2_SceneGraph::SceneGraph_EnumerateNodes(const uint32* _piNodes, int _nNodes, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem, int _nMergeElem)
{
	Error("SceneGraph_EnumerateNodes", "Not implemented.");
	return 0;
}

int CBSP2_SceneGraph::SceneGraph_EnumeratePVS(const uint8* _pPVS, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem, int _nMergeElem)
{
	M_LOCK(m_Lock);
	return m_Objects.EnumPVS(_pPVS, _pRetElem, _MaxRet, _pMergeElem, _nMergeElem);
}

int CBSP2_SceneGraph::SceneGraph_EnumerateView(int _iView, uint16* _pRetElem, int _MaxRet, const uint16* _pMergeElem, int _nMergeElem)
{
	M_LOCK(m_Lock);
	return m_Objects.EnumVisible(m_pModel->m_lspViews[_iView], _pRetElem, _MaxRet, _pMergeElem, _nMergeElem);
}

void CBSP2_SceneGraph::UnlinkLight(int _iLight)
{
	const CBSP2_Link* pLinks = m_ObjectLights.GetLinks();
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

	m_lLights[_iLight].m_Flags &= ~CXR_LIGHT_ENABLED;
}

void CBSP2_SceneGraph::LinkLight(CXR_Light& _Light)
{
	if (!_Light.m_pPVS)
		_Light.m_pPVS = const_cast<uint8*>(m_pModel->SceneGraph_PVSLock(0, _Light.GetPosition()));

	if (_Light.m_pPVS)
	{
		m_Lights.InsertWithPVS(_Light.m_iLight, _Light.m_BoundBox, 0, _Light.m_pPVS);
	}
	else
		m_Lights.Insert(_Light.m_iLight, _Light.m_BoundBox, 0);

	m_DeferredLights.Insert(_Light.m_iLight);

	_Light.m_Flags |= CXR_LIGHT_ENABLED;
}

int CBSP2_SceneGraph::GetFreeDynamicLightIndex()
{
	const CXR_Light* pL = &m_lLights[m_iFirstDynamic];

	int iCheck = m_iNextDynamic & m_MaxDynamicAnd;
	int iFirstCheck = iCheck;
	do 
	{
		if (!(pL[iCheck].m_Flags & CXR_LIGHT_ENABLED))
			goto Found;

		iCheck = (iCheck + 1) & m_MaxDynamicAnd;
	}
	while(iCheck != iFirstCheck);

	return -1;

Found:
	m_iNextDynamic = (iCheck + 1) & m_MaxDynamicAnd;

	int iLight = m_iFirstDynamic + iCheck;
	return iLight;
}

int CBSP2_SceneGraph::GetNumFreeDynamicLights()
{
	const CXR_Light* pL = &m_lLights[m_iFirstDynamic];

	int nFree = 0;
	for(int i = 0; i < m_MaxDynamic; i++)
		if (!(pL[i].m_Flags & CXR_LIGHT_ENABLED))
			nFree++;

	return nFree;
}

int CBSP2_SceneGraph::SceneGraph_Light_GetMaxIndex()
{
	M_LOCK(m_Lock);
	return m_lLights.Len();
}

void CBSP2_SceneGraph::SceneGraph_Light_LinkDynamic(const CXR_Light& _Light)
{
	M_LOCK(m_Lock);
//	M_TRACEALWAYS("(this=%.8x) nFree dynamic %d\n", this, GetNumFreeDynamicLights() );

	if(_Light.m_Type == CXR_LIGHTTYPE_SPOT)
	{
		if(_Light.m_SpotWidth == 0 || _Light.m_SpotHeight == 0)
		{
			// No point in linking invalid spot
			return;
		}
	}

	int iLight = m_LightGUIDMap.GetIndex(_Light.m_LightGUID);
	if (iLight < 0)
	{
		iLight = GetFreeDynamicLightIndex();
		if (iLight < 0)
			return;
		m_LightGUIDMap.Insert(iLight, _Light.m_LightGUID);
	}

	CXR_Light& Light = m_lLights[iLight];
	Light = _Light;
	Light.m_iLight = iLight;
	Light.m_iLightf = fp32(iLight);

	if (Light.m_Type == CXR_LIGHTTYPE_SPOT)
	{
		CBSP2_SpotLightClip& Clip = m_lDynamicLightClip[iLight - m_iFirstDynamic];
		Clip.Create(&Light);
		Clip.GetBoundBox(Light.m_BoundBox);
	}
	else
		Light.CalcBoundBoxFast();

	Light.m_pPVS = const_cast<uint8*>(m_pModel->SceneGraph_PVSLock(0, Light.GetPosition()));

	if (m_pModel->View_GetVisibleBoxWithPVS_Box(Light.m_BoundBox, Light.m_pPVS, Light.m_BoundBox))
		LinkLight(Light);
}

void CBSP2_SceneGraph::SceneGraph_Light_AddPrivateDynamic(const CXR_Light& _Light)
{
	M_LOCK(m_Lock);
	// Lights added with this method will not be returned when enumerating lights.
	// Use this function to reserve a light index for rendering the light manually.

	int iLight = m_LightGUIDMap.GetIndex(_Light.m_LightGUID);
	if (iLight < 0)
	{
		iLight = GetFreeDynamicLightIndex();
		if (iLight < 0)
			return;
		m_LightGUIDMap.Insert(iLight, _Light.m_LightGUID);
	}

	CXR_Light& Light = m_lLights[iLight];
	Light = _Light;
	Light.m_iLight = iLight;
	Light.m_iLightf = fp32(iLight);
	Light.m_Flags |= CXR_LIGHT_ENABLED;
	Light.CalcBoundBoxFast();
}

void CBSP2_SceneGraph::SceneGraph_Light_ClearDynamics()
{
	M_LOCK(m_Lock);
	const CXR_Light* pL = &m_lLights[m_iFirstDynamic];

	for(int i = 0; i < m_MaxDynamic; i++)
	{
		if (pL[i].m_Flags & CXR_LIGHT_ENABLED)
		{
			UnlinkLight(m_iFirstDynamic + i);
			m_LightGUIDMap.Remove(m_iFirstDynamic + i);
		}
//		m_Lights.Remove(m_iFirstDynamic + i);
	}
	m_iNextDynamic = 0;

//	m_nDynamic = 0;
}

int CBSP2_SceneGraph::SceneGraph_Light_GetIndex(int _LightGUID)
{
	M_LOCK(m_Lock);
	int iLight = m_LightGUIDMap.GetIndex(_LightGUID);
	if (iLight == -1)
		return 0;
	else
		return iLight;
}

int CBSP2_SceneGraph::SceneGraph_Light_GetGUID(int _iLight)
{
	M_LOCK(m_Lock);
	unsigned int iL = _iLight;
	if (iL >= m_lLights.Len())
		return 0;

	CXR_Light* pL = m_lLights.GetBasePtr();
	return pL[iL].m_LightGUID;
}

void CBSP2_SceneGraph::SceneGraph_Light_Unlink(int _iLight)
{
	M_LOCK(m_Lock);
	M_ASSERT(_iLight >= 0 && _iLight < m_lLights.Len(), "!");

	m_LightGUIDMap.Remove(_iLight);
	m_Lights.Remove(_iLight);

	CXR_Light& Light = m_lLights[_iLight];
	Light.m_Flags &= ~CXR_LIGHT_ENABLED;
}

void CBSP2_SceneGraph::SceneGraph_Light_SetIntensity(int _iLight, const CVec3Dfp32& _Intensity, bool _bIsOff)
{
	M_LOCK(m_Lock);
	M_ASSERT(_iLight >= 0 && _iLight < m_lLights.Len(), "!");

	CXR_Light& Light = m_lLights[_iLight];

	if (_bIsOff)
		Light.SetIntensity(0);
	else
		Light.SetIntensity(_Intensity);

	if (Light.m_Flags & CXR_LIGHT_ENABLED)
	{
		if (_bIsOff)
			UnlinkLight(_iLight);
	}
	else
	{
		if (!_bIsOff)
		{
			UnlinkLight(_iLight);
			LinkLight(Light);
		}
	}

/*	bool bWasOff = Light.m_Intensity.LengthSqr() < Sqr(0.001f);
	Light.SetIntensity(_Intensity);
	bool bIsOff = Light.m_Intensity.LengthSqr() < Sqr(0.001f);

	if ((bWasOff && !bIsOff) || (!bWasOff && bIsOff))
	{
		UnlinkLight(_iLight);
		LinkLight(Light);
	}*/
}

void CBSP2_SceneGraph::SceneGraph_Light_SetRotation(int _iLight, const CMat4Dfp32& _Rotation)
{
	M_LOCK(m_Lock);
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

void CBSP2_SceneGraph::SceneGraph_Light_SetPosition(int _iLight, const CMat4Dfp32& _Pos)
{
	M_LOCK(m_Lock);
	M_ASSERT(_iLight >= 0 && _iLight < m_lLights.Len(), "!");

	if (_iLight < m_iFirstDynamic)
	{
		ConOut(CStrF("§cf80WARNING: Attempting to move static light %d. (Operation ignored)", _iLight));
		return;
	}

	UnlinkLight(_iLight);
	CXR_Light& Light = m_lLights[_iLight];
	Light.m_pPVS = 0;
	Light.m_Pos = _Pos;
	Light.CalcBoundBoxFast();
	LinkLight(Light);
}

void CBSP2_SceneGraph::SceneGraph_Light_SetProjectionMap(int _iLight, int _TextureID, const CMat4Dfp32* _pPos)
{
	M_LOCK(m_Lock);
	M_ASSERT(_iLight >= 0 && _iLight < m_lLights.Len(), "!");

	CXR_Light& Light = m_lLights[_iLight];
	Light.SetProjectionMap(_TextureID, _pPos);
}

void CBSP2_SceneGraph::SceneGraph_Light_Get(int _iLight, CXR_Light& _Light)
{
	M_LOCK(m_Lock);
	M_ASSERT(_iLight >= 0 && _iLight < m_lLights.Len(), "!");
	_Light = m_lLights[_iLight];
}

void CBSP2_SceneGraph::SceneGraph_Light_GetIntensity(int _iLight, CVec3Dfp32& _Intensity)
{
	M_LOCK(m_Lock);
	unsigned int iL = _iLight;
	if (iL >= m_lLights.Len())
	{
		_Intensity = 1.0f;
		return;
	}

	CXR_Light* pL = m_lLights.GetBasePtr();
	if (pL->m_Flags & CXR_LIGHT_ENABLED)
	{
		CVec4Dfp32 Color = pL[iL].GetIntensityv();
		_Intensity[0] = Color[0];
		_Intensity[1] = Color[1];
		_Intensity[2] = Color[2];
	}
	else
		_Intensity = 0;
}

int CBSP2_SceneGraph::SceneGraph_Light_Enum(const uint16* _piPL, int _nPL, const CBox3Dfp32& _Box, const CXR_Light** _lpLights, int _nMaxLights)
{
	M_LOCK(m_Lock);
	uint16 liLights[1024];

	const int MAXRETLIGHTS = 16;
	fp32 lRetLightSort[MAXRETLIGHTS];
	uint16 liRetLights[MAXRETLIGHTS];
	if (_nMaxLights > MAXRETLIGHTS)
	{
		ConOut("WARNING: (CBSP2_SceneGraph::SceneGraph_Light_Enum) Clamping light query to 16 lights. Fix something.");
		_nMaxLights = MAXRETLIGHTS;
	}

	// Enumerate portal leaves lights
	int nLights = m_Lights.EnumPortalLeafList((uint16*)_piPL, _nPL, liLights, 1024);

	CVec3Dfp32 BoxMin = _Box.m_Min;
	CVec3Dfp32 BoxMax = _Box.m_Max;
	// Get the _nMaxLights strongest lights. (0 sqrt, 0 div!)
	int nRetLights = 0;
	for(int i = 0; i < nLights; i++)
	{
		int iLight = liLights[i];
		CXR_Light& Light = m_lLights[iLight];
		if (Light.m_Type == CXR_LIGHTTYPE_SPOT)
		{
			// Cull box against spotlight volume
			CPlane3Dfp32 lP[5];
			CBSP2_SpotLightClip::CalcSpotPlanes(&Light, lP);

			fp32 Dist[5];
			Dist[0] = lP[0].GetBoxMinDistance(BoxMin, BoxMax);
			Dist[1] = lP[1].GetBoxMinDistance(BoxMin, BoxMax);
			Dist[2] = lP[2].GetBoxMinDistance(BoxMin, BoxMax);
			Dist[3] = lP[3].GetBoxMinDistance(BoxMin, BoxMax);
			Dist[4] = lP[4].GetBoxMinDistance(BoxMin, BoxMax);

			if(Max(Max(Dist[0], Dist[1]), Max(Dist[4], Max(Dist[2], Dist[3]))) > 0.0f)
				continue;

//			int p = 0;
//			for(; p < 5; p++)
//			{
//				if (lP[p].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) > 0.0f)
//					break;
//			}
//			if (p < 5)
//				continue;
		}
		else if (Light.m_Type == CXR_LIGHTTYPE_POINT)
		{
		}
		else
			continue;

		// Calc attenuation
		fp32 DistSqr = _Box.GetMinSqrDistance(Light.GetPosition());
		fp32 Attn = Sqr(1.0f- Clamp01(DistSqr*Sqr(Light.m_RangeInv)));
		CVec4Dfp32 LightIntensity = Light.GetIntensityv();
		fp32 Intensity = LightIntensity[0] + LightIntensity[1]*2.0f + LightIntensity[2];
		fp32 AttnIntensity = Attn * Intensity;

		if (AttnIntensity > 0.0001f)
		{
			if (nRetLights < _nMaxLights)
			{
				liRetLights[nRetLights] = iLight;
				nRetLights++;
			}
			else
			{
				// Find the weakest light
				fp32 Worst = _FP32_MAX;
				int iWorst = 0;
				for(int j = 0; j < nRetLights; j++)
				{
					if (lRetLightSort[j] < Worst)
					{
						Worst = lRetLightSort[j];
						iWorst = j;
					}
				}

				// Replace it if weaker than iLight
				if (Worst < AttnIntensity)
				{
					liRetLights[iWorst] = iLight;
					lRetLightSort[iWorst] = AttnIntensity;
				}
			}
		}
	}

	for(int k = 0; k < nRetLights; k++)
	{
		_lpLights[k] = &m_lLights[liRetLights[k]];
	}

	return nRetLights;
}

int CBSP2_SceneGraph::SceneGraph_Light_Enum(const CBox3Dfp32& _Box, const CXR_Light** _lpLights, int _nMaxLights)
{
	M_LOCK(m_Lock);
	uint16 liLights[1024];

	const int MAXRETLIGHTS = 16;
	fp32 lRetLightSort[MAXRETLIGHTS];
	uint16 liRetLights[MAXRETLIGHTS];
	if (_nMaxLights > MAXRETLIGHTS)
	{
		ConOut("WARNING: (CBSP2_SceneGraph::SceneGraph_Light_Enum) Clamping light query to 16 lights. Fix something.");
		_nMaxLights = MAXRETLIGHTS;
	}

	int nLights = m_Lights.EnumBox(_Box, liLights, 1024, NULL, 0);

	CVec3Dfp32 BoxMin = _Box.m_Min;
	CVec3Dfp32 BoxMax = _Box.m_Max;
	// Get the _nMaxLights strongest lights. (0 sqrt, 0 div!)
	int nRetLights = 0;
	for(int i = 0; i < nLights; i++)
	{
		int iLight = liLights[i];
		CXR_Light& Light = m_lLights[iLight];
		if (Light.m_Type == CXR_LIGHTTYPE_SPOT)
		{
			// Cull box against spotlight volume
			CPlane3Dfp32 lP[5];
			CBSP2_SpotLightClip::CalcSpotPlanes(&Light, lP);

			fp32 Dist[5];
			Dist[0] = lP[0].GetBoxMinDistance(BoxMin, BoxMax);
			Dist[1] = lP[1].GetBoxMinDistance(BoxMin, BoxMax);
			Dist[2] = lP[2].GetBoxMinDistance(BoxMin, BoxMax);
			Dist[3] = lP[3].GetBoxMinDistance(BoxMin, BoxMax);
			Dist[4] = lP[4].GetBoxMinDistance(BoxMin, BoxMax);

			if(Max(Max(Dist[0], Dist[1]), Max(Dist[4], Max(Dist[2], Dist[3]))) > 0.0f)
				continue;

//			int p = 0;
//			for(; p < 5; p++)
//			{
//				if (lP[p].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) > 0.0f)
//					break;
//			}
//			if (p < 5)
//				continue;
		}
		else if (Light.m_Type == CXR_LIGHTTYPE_POINT)
		{
		}
		else
			continue;

		// Calc attenuation
		fp32 DistSqr = _Box.GetMinSqrDistance(Light.GetPosition());
		fp32 Attn = Sqr(1.0f- Clamp01(DistSqr*Sqr(Light.m_RangeInv)));
		CVec4Dfp32 LightColor = Light.GetIntensityv();
		fp32 Intensity = LightColor[0] + LightColor[1]*2.0f + LightColor[2];
		fp32 AttnIntensity = Attn * Intensity;

		if (AttnIntensity > 0.0001f)
		{
			if (nRetLights < _nMaxLights)
			{
				liRetLights[nRetLights] = iLight;
				lRetLightSort[nRetLights] = AttnIntensity;
				nRetLights++;
			}
			else
			{
				// Find the weakest light
				fp32 Worst = _FP32_MAX;
				int iWorst = 0;
				for(int j = 0; j < nRetLights; j++)
				{
					if (lRetLightSort[j] < Worst)
					{
						Worst = lRetLightSort[j];
						iWorst = j;
					}
				}

				// Replace it if weaker than iLight
				if (Worst < AttnIntensity)
				{
					liRetLights[iWorst] = iLight;
					lRetLightSort[iWorst] = AttnIntensity;
				}
			}
		}
	}

	for(int k = 0; k < nRetLights; k++)
	{
		_lpLights[k] = &m_lLights[liRetLights[k]];
	}

	return nRetLights;
}

CXR_Light* CBSP2_SceneGraph::GetLight(int _iLight)
{
//	M_LOCK(m_Lock);
	M_ASSERT(_iLight >= 0 && _iLight < m_lLights.Len(), "!");
	return m_lLights.GetBasePtr() + _iLight;
}
