#include "PCH.h"

#include "WBSP3Model.h"
#include "WBSP3Def.h"
#include "MFloat.h"

#define FACECUT3_EPSILON 0.01f

#if defined(MODEL_BSP3_BOUNDCHECK_STACK)
#ifdef	PLATFORM_PS2
#warning "There are some defines enabled which cause performance drops."
#else
#endif
#endif

#ifdef	MODEL_BSP3_BOUNDCHECK_STACK
int EnumFaces_Box_i_MaxDepth = 0;
int EnumFaces_Box_i_MaxDepth_LastPeak = 0;
int EnumFaces_All_i_MaxDepth = 0;
int EnumFaces_All_i_MaxDepth_LastPeak = 0;
#endif	// MODEL_BSP3_BOUNDCHECK_STACK

int BSP3Model_CutFace3(CVec3Dfp32* _pVerts, int _nv, const CPlane3Dfp32* _pPlanes, int _np, int& _bClip)
{
	MAUTOSTRIP(BSP3Model_CutFace3, 0);
	_bClip = false;
	if (!_nv) return 0;
	const int MaxVClip = 32;

	CVec3Dfp32 VClip[MaxVClip];

	CVec3Dfp32* pVDest = &VClip[0];
	CVec3Dfp32* pVSrc = _pVerts;

	for(int iPlane = 0; iPlane < _np; iPlane++)
	{
		const CPlane3Dfp32* pP = &_pPlanes[iPlane];
		fp32 VertPDist[32];
		int Side = 0;

		// Calc point-2-plane distance for all vertices.
		for(int v = 0; v < _nv; v++)
		{
			VertPDist[v] = -pP->Distance(pVSrc[v]);
			if (VertPDist[v] < -FACECUT3_EPSILON) Side |= 2;
			else if (VertPDist[v] > FACECUT3_EPSILON) Side |= 1;
			else Side |= 4;

//			if (VertPDist[v] < 0.0f) bBehind = true; else bFront = true;
		}

		// If all points are on one side, return either all or none.
		if ((Side & 3) == 2)
		{
			_bClip = true;
			return 0;
		}
		if ((Side & 3) != 3) continue;

/*		if (!(bFront && bBehind))
		{
			if (bFront) continue;
			_bClip = true;
			return 0;
		}
*/
		_bClip = true;
		int nClip = 0;
		{
			int v = _nv-1;
			for (int v2 = 0; v2 < _nv; v2++)
			{
				if (VertPDist[v] > -FACECUT3_EPSILON)
				{
					pVDest[nClip] = pVSrc[v];
					nClip++;

					if ((VertPDist[v2] < -FACECUT3_EPSILON) && (VertPDist[v] > FACECUT3_EPSILON))
					{
						goto ClipVertex;
/*
						fp32 dvx = (pVSrc[v2].k[0] - pVSrc[v].k[0]);
						fp32 dvy = (pVSrc[v2].k[1] - pVSrc[v].k[1]);
						fp32 dvz = (pVSrc[v2].k[2] - pVSrc[v].k[2]);
						fp32 s = -(dvx*pP->n.k[0] + dvy*pP->n.k[1] + dvz*pP->n.k[2]);
						if (s)
						{
							fp32 sp = VertPDist[v];
							fp32 t = -sp/s;
							pVDest[nClip].k[0] = pVSrc[v].k[0] + dvx * t;
							pVDest[nClip].k[1] = pVSrc[v].k[1] + dvy * t;
							pVDest[nClip].k[2] = pVSrc[v].k[2] + dvz * t;
							nClip++;
						}
*/
					}
				}
				else
				{
					if (VertPDist[v2] > FACECUT3_EPSILON)
					{
ClipVertex:
						fp32 dvx = (pVSrc[v2].k[0] - pVSrc[v].k[0]);
						fp32 dvy = (pVSrc[v2].k[1] - pVSrc[v].k[1]);
						fp32 dvz = (pVSrc[v2].k[2] - pVSrc[v].k[2]);
						fp32 s = -(dvx*pP->n.k[0] + dvy*pP->n.k[1] + dvz*pP->n.k[2]);
						if (s)
						{
							fp32 sp = VertPDist[v];
							fp32 t = -sp/s;
							pVDest[nClip].k[0] = pVSrc[v].k[0] + dvx * t;
							pVDest[nClip].k[1] = pVSrc[v].k[1] + dvy * t;
							pVDest[nClip].k[2] = pVSrc[v].k[2] + dvz * t;
							nClip++;
						}
					}
				}
#ifndef	M_RTM
				if (nClip > MaxVClip-1) Error_static("CutFace", "Too many vertices.");
#endif
				v = v2;
			}
		}

		if (!nClip) return 0;
		_nv = nClip;

		Swap(pVSrc, pVDest);
	}

	// Move if the latest vertices are in the wrong array.
	if (pVSrc != _pVerts) 
#ifdef	PLATFORM_PS2
		PS2WordCopy( _pVerts, pVSrc, _nv * (sizeof(CVec3Dfp32)/4) );
#else
		memcpy(_pVerts, pVSrc, _nv*sizeof(CVec3Dfp32));
#endif
	return _nv;
}

// -------------------------------------------------------------------
//  CBSP3_LinkContext
// -------------------------------------------------------------------
#define LINKBOX_EPSILON 0.001f

CBSP3_LinkContext::CBSP3_LinkContext()
{
	MAUTOSTRIP(CBSP3_LinkContext_ctor, MAUTOSTRIP_VOID);
	m_pLinks = NULL;
	m_pModel = NULL;
	m_pIDLinkMap = NULL;
	m_pPLLinkMap = NULL;
}

void CBSP3_LinkContext::Create(CXR_Model_BSP3* _pModel, int _NumIDs, int _NumLinks, int _nPL, bool _bNeedBoxes)
{
	MAUTOSTRIP(CBSP3_LinkContext_Create, MAUTOSTRIP_VOID);
	m_pModel = _pModel;
	if (_NumIDs > 65535) Error("Create", "Can only handle 64k IDs.");
	m_lIDLinkMap.SetLen(_NumIDs);

	if (_bNeedBoxes)
		m_lIDLinkBox.SetLen(_NumIDs);
	else
		m_lIDLinkBox.Destroy();

	m_lIDFlags.SetLen(_NumIDs);
	FillChar(m_lIDLinkMap.GetBasePtr(), m_lIDLinkMap.ListSize(), 0);
	FillChar(m_lIDFlags.GetBasePtr(), m_lIDFlags.ListSize(), 0);
	m_lPLLinkMap.SetLen(_nPL+1);
	m_iPLInfinite = _nPL;
	FillChar(m_lPLLinkMap.GetBasePtr(), m_lPLLinkMap.ListSize(), 0);

	m_bPLBlock = 0;
	m_lPLBlock.SetLen((m_lPLLinkMap.Len() + 7) >> 3);
	FillChar(m_lPLBlock.GetBasePtr(), m_lPLBlock.ListSize(), 0);

	_NumLinks = (_NumLinks + 31) & ~31;
	m_lLinks.SetLen(_NumLinks);
	m_LinkHeap.Create(_NumLinks, FALSE);	// Non-dynamic heap
	m_LinkHeap.AllocID();					// Reserve ID 0.
	m_LinkHeapMinAvail = m_LinkHeap.MaxAvail();
	m_pLinks = m_lLinks.GetBasePtr();

	m_pIDLinkMap = m_lIDLinkMap.GetBasePtr();
	m_pPLLinkMap = m_lPLLinkMap.GetBasePtr();
}

void CBSP3_LinkContext::Insert(int _ID, const CBox3Dfp32& _Box, int _Flags)
{
	MAUTOSTRIP(CBSP3_LinkContext_Insert, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP3_LinkContext::Insert");

	Remove(_ID);

	M_ASSERT(!m_lIDLinkMap[_ID], "!");

	m_iNextLink = 0;
	m_iPrevLink = 0;


	m_lIDLinkBox[_ID] = _Box;
	m_lIDFlags[_ID] = _Flags | 0x80;

	m_CurLinkID = _ID;
	m_pModel->LinkBox(this, 1, _Box);

/*	if (!m_lIDLinkMap[_ID])
	{
		InsertInfinite(_ID, _Flags);
//		ConOut(CStrF("§c0f0NOTE: (CBSP3_LinkContext::Insert) ID %d (%s) was not linked to any PL.", _ID, (char*)_Box.GetString() ));
	}*/

//if (bCheck && pByte[0x04474F13] != 0xfd) M_BREAKPOINT;
}

void CBSP3_LinkContext::InsertWithPVS(int _ID, const CBox3Dfp32& _Box, int _Flags, const uint8* _pPVS)
{
	MAUTOSTRIP(CBSP3_LinkContext_Insert, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP3_LinkContext::Insert");

	Remove(_ID);

	M_ASSERT(!m_lIDLinkMap[_ID], "!");

	m_iNextLink = 0;
	m_iPrevLink = 0;

	m_lIDLinkBox[_ID] = _Box;
	m_lIDFlags[_ID] = _Flags | 0x80;

	m_CurLinkID = _ID;
	m_pModel->LinkBoxWithPVS(this, 1, _Box, _pPVS);

/*	if (!m_lIDLinkMap[_ID])
	{
		InsertInfinite(_ID, _Flags);
//		ConOut(CStrF("§c0f0NOTE: (CBSP3_LinkContext::Insert) ID %d (%s) was not linked to any PL.", _ID, (char*)_Box.GetString() ));
	}*/
}


void CBSP3_LinkContext::Insert(int _ID, int _iPL)
{
	MAUTOSTRIP(CBSP3_LinkContext_Insert_2, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP3_LinkContext::Insert");

	Remove(_ID);
	m_iNextLink = 0;
	m_iPrevLink = 0;
	m_lIDFlags[_ID] = 0x80;

	m_CurLinkID = _ID;
	AddPortalLeaf(_iPL);
}

void CBSP3_LinkContext::InsertBegin(int _ID, int _Flags)
{
	MAUTOSTRIP(CBSP3_LinkContext_InsertBegin, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP3_LinkContext::InsertBegin");

	Remove(_ID);
	m_iNextLink = 0;
	m_iPrevLink = 0;

	m_CurLinkID = _ID;
	m_bPLBlock = 1;

	m_lIDFlags[_ID] = _Flags | 0x80;

	if (m_lIDLinkBox.Len())
	{
		m_lIDLinkBox[_ID].m_Min = 100000000000.0f;
		m_lIDLinkBox[_ID].m_Max = -100000000000.0f;
	}
}

void CBSP3_LinkContext::InsertEnd(int _ID)
{
	MAUTOSTRIP(CBSP3_LinkContext_InsertEnd, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP3_LinkContext::InsertEnd");

	m_bPLBlock = 0;

	// Unblock the PLs we linked
	int iLink = GetLink_ID(_ID);
	for(; iLink; iLink = m_pLinks[iLink].m_iLinkNextPL)
	{
		m_lPLBlock[m_pLinks[iLink].m_iPortalLeaf >> 3] = 0;
	}
}

void CBSP3_LinkContext::InsertInfinite(int _ID, int _Flags)
{
	MAUTOSTRIP(CBSP3_LinkContext_InsertInfinite, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP3_LinkContext::InsertInfinite");
	Remove(_ID);
	m_iNextLink = 0;
	m_iPrevLink = 0;


	m_lIDFlags[_ID] = _Flags | 0x80;

	m_CurLinkID = _ID;

	AddPortalLeaf(m_iPLInfinite);
}

void CBSP3_LinkContext::Remove(int _ID)
{
	MAUTOSTRIP(CBSP3_LinkContext_Remove, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP3_LinkContext::Remove");
//	if (m_pLinks != m_lLinks.GetBasePtr()) M_BREAKPOINT;
	int iLink = m_lIDLinkMap[_ID];
	while(iLink)
	{
		M_ASSERT(m_lLinks.ValidPos(iLink), CStrF("Link out of range. %d/%d", iLink, m_lLinks.Len()));

		m_LinkHeap.FreeID(iLink);
		const CBSP3_Link* pL = &m_pLinks[iLink];

		// relink prev
		if (pL->m_iLinkPrevObject)
			m_pLinks[pL->m_iLinkPrevObject].m_iLinkNextObject = pL->m_iLinkNextObject;
		else
			m_lPLLinkMap[pL->m_iPortalLeaf] = pL->m_iLinkNextObject;

		// relink next
		if (pL->m_iLinkNextObject)
			m_pLinks[pL->m_iLinkNextObject].m_iLinkPrevObject = pL->m_iLinkPrevObject;

		iLink = pL->m_iLinkNextPL;
	}

	m_lIDLinkMap[_ID] = 0;
//	m_lIDFlags[_ID] = 0;
}

int CBSP3_LinkContext::EnumPortalLeafList(uint16* _piPL, int _nPL, uint16* _piRet, int _MaxRet)
{
	MAUTOSTRIP(CBSP3_LinkContext_EnumPortalLeafList, 0);
	int nRet = 0;

	nRet += EnumPortalLeaf(m_iPLInfinite, &_piRet[nRet], _MaxRet-nRet);

	for(int i = 0; i < _nPL; i++)
	{
		int iLink = m_pPLLinkMap[_piPL[i]];
		while(iLink)
		{
			M_ASSERT(m_lLinks.ValidPos(iLink), CStrF("Link out of range. %d/%d", iLink, m_lLinks.Len()));

			int ID = m_pLinks[iLink].m_ID;

			if(!(m_pIDLinkMap[ID] & 0x8000))
			{
				if(nRet >= _MaxRet)
				{
					#ifdef M_RTM
						return nRet;
					#else
						Error("EnumPortalLeafList", CStrF("Out of enumeration space. (%d)", _MaxRet));
					#endif
				}

				_piRet[nRet++] = ID;
				m_pIDLinkMap[ID] |= 0x8000;
			}

			// Next...
			iLink = m_pLinks[iLink].m_iLinkNextObject;
		}
	}
	
	// Untag all that were added.
	for(int i = 0; i < nRet; i++)
		m_pIDLinkMap[_piRet[i]] &= ~0x8000;

	return nRet;
}

int CBSP3_LinkContext::EnumPortalLeaf(int _iPL, uint16* _piRet, int _MaxRet)
{
	MAUTOSTRIP(CBSP3_LinkContext_EnumPortalLeaf, 0);
	int nRet = 0;
	int iLink = m_pPLLinkMap[_iPL];
	while(iLink)
	{
		M_ASSERT(m_lLinks.ValidPos(iLink), CStrF("Link out of range. %d/%d", iLink, m_lLinks.Len()));

#ifndef	M_RTM
		if (nRet >= _MaxRet) Error("EnumBox_r", CStrF("Out of enumeration space. (%d)", _MaxRet));
#endif
		int ID = m_pLinks[iLink].m_ID;

		// If taged, don't add it again.
		if (!(m_pIDLinkMap[ID] & 0x8000))
		{
			// Add to list and tag ID.
			_piRet[nRet++] = m_pLinks[iLink].m_ID;
			m_pIDLinkMap[ID] |= 0x8000;
		}

		// Next...
		iLink = m_pLinks[iLink].m_iLinkNextObject;
	}
	return nRet;
}

int CBSP3_LinkContext::EnumPortalLeafWithClip(int _iPL, uint16* _piRet, int _MaxRet, const CRC_ClipVolume* _pClip)
{
	MAUTOSTRIP(CBSP3_LinkContext_EnumPortalLeafWithClip, 0);
	int nRet = 0;
	int iLink = m_pPLLinkMap[_iPL];

	for(; iLink; iLink = m_pLinks[iLink].m_iLinkNextObject)
	{
		M_ASSERT(m_lLinks.ValidPos(iLink), CStrF("Link out of range. %d/%d", iLink, m_lLinks.Len()));

#ifndef	M_RTM
		if (nRet >= _MaxRet) Error("EnumBox_r", CStrF("Out of enumeration space. (%d)", _MaxRet));
#endif
		int ID = m_pLinks[iLink].m_ID;

		// If taged, don't add it again.
		if (!(m_pIDLinkMap[ID] & 0x8000))
		{
			// Cull
			if (!(m_lIDFlags[ID] & CXR_SCENEGRAPH_NOPORTALPVSCULL))
			{
				const CBox3Dfp32& Box = m_lIDLinkBox[ID];
				for(int p = 0; p < _pClip->m_nPlanes; p++)
					if (_pClip->m_Planes[p].GetBoxMaxDistance(Box.m_Min, Box.m_Max) > 0.0f)
						continue;
			}

			// Add to list and tag ID.
			_piRet[nRet++] = m_pLinks[iLink].m_ID;
			m_pIDLinkMap[ID] |= 0x8000;
		}
	}
	return nRet;
}

int CBSP3_LinkContext::MergeIDs(uint16* _piRet, int _nRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP3_LinkContext_MergeIDs, 0);
	int nRet = _nRet;
	for(int iM = 0; iM < _nMerge; iM++)
	{
		int iElem = _pMergeElem[iM];
//		if (!(m_lIDLinkMap[iElem] & 0x8000))
		if (!(m_pIDLinkMap[iElem] & 0x8000))
		{
#ifndef	M_RTM
			if (nRet >= _MaxRet) Error("EnumVisible", "Out of enumeration space.");
#endif
//			m_lIDLinkMap[iElem] |= 0x8000;
			m_pIDLinkMap[iElem] |= 0x8000;
			_piRet[nRet++] = iElem;
		}
	}
	return nRet;
}

int CBSP3_LinkContext::EnumBox_r(int _iNode, const CBox3Dfp32& _Box, uint16* _piRet, int _MaxRet)
{
	MAUTOSTRIP(CBSP3_LinkContext_EnumBox_r, 0);
	MSCOPESHORT(CBSP3_LinkContext::EnumBox_r); //AR-SCOPE

	int nRet = 0;
	const CBSP3_Node* pN = &m_pModel->m_lNodes[_iNode];
	if( pN->IsStructureLeaf() )
	{
		return EnumPortalLeaf(pN->m_iPortalLeaf, _piRet, _MaxRet);
	}

	if (pN->IsNode())
	{
		int iBack = pN->m_iNodeBack;
		const CPlane3Dfp32* pP = &m_pModel->m_lPlanes[pN->m_iPlane];
		if (!iBack)
		{
//			int iFront = pN->m_iNodeFront;
			if (pP->GetBoxMaxDistance(_Box.m_Min, _Box.m_Max) > MODEL_BSP_EPSILON)
				nRet += EnumBox_r(pN->m_iNodeFront, _Box, &_piRet[nRet], _MaxRet - nRet);
		}
		else
		{
			int iFront = pN->m_iNodeFront;
			if (!iFront)
			{
				if (pP->GetBoxMinDistance(_Box.m_Min, _Box.m_Max) < -MODEL_BSP_EPSILON)
					nRet += EnumBox_r(pN->m_iNodeBack, _Box, &_piRet[nRet], _MaxRet - nRet);
			}
			else
			{
				fp32 MinDist, MaxDist;
				pP->GetBoxMinAndMaxDistance(_Box.m_Min, _Box.m_Max, MinDist, MaxDist );
				if (MaxDist > MODEL_BSP_EPSILON)
					nRet += EnumBox_r(pN->m_iNodeFront, _Box, &_piRet[nRet], _MaxRet - nRet);
				if (MinDist < -MODEL_BSP_EPSILON)
					nRet += EnumBox_r(pN->m_iNodeBack, _Box, &_piRet[nRet], _MaxRet - nRet);
			}
		}
	}

	return nRet;
}

int CBSP3_LinkContext::EnumBox(const CBox3Dfp32& _Box, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP3_LinkContext_EnumBox, 0);
	int nRet = 0;
	nRet += EnumPortalLeaf(m_iPLInfinite, &_piRet[nRet], _MaxRet-nRet);
	nRet += EnumBox_r(1, _Box, &_piRet[nRet], _MaxRet-nRet);

	// Merge with incomming
	if (_nMerge) nRet = MergeIDs(_piRet, nRet, _MaxRet, _pMergeElem, _nMerge);

	// Untag all that were added.
	for(int i = 0; i < nRet; i++)
		m_pIDLinkMap[_piRet[i]] &= ~0x8000;

	return nRet;
}

int CBSP3_LinkContext::EnumVisible(CBSP3_ViewInstance* _pView, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP3_LinkContext_EnumVisible, 0);
	int nRet = 0;
	nRet += EnumPortalLeaf(m_iPLInfinite, &_piRet[nRet], _MaxRet-nRet);
	const uint16* piVisPortalLeaves = _pView->m_liVisPortalLeaves.GetBasePtr();
	for(int iiPL = 0; iiPL < _pView->m_nVisLeaves; iiPL++)
	{
		int iPL = piVisPortalLeaves[iiPL];
		nRet += EnumPortalLeafWithClip(iPL, &_piRet[nRet], _MaxRet-nRet, 
					&_pView->m_lRPortals[_pView->m_liLeafRPortals[iPL]]);
	}

	// Merge with incomming
	if (_nMerge) nRet = MergeIDs(_piRet, nRet, _MaxRet, _pMergeElem, _nMerge);

	// Untag all that were added.
	for(int i = 0; i < nRet; i++)
		m_pIDLinkMap[_piRet[i]] &= ~0x8000;

	return nRet;
}

int CBSP3_LinkContext::EnumPVS(const uint8* _pPVS, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP3_LinkContext_EnumPVS, 0);
	int nRet = 0;
	int PVSLen = m_pModel->SceneGraph_PVSGetLen();
	int nPL = m_pModel->m_nStructureLeaves;

	nRet += EnumPortalLeaf(m_iPLInfinite, &_piRet[nRet], _MaxRet-nRet);

	int iPL = 0;
	int i;
	for(i = 0; i < PVSLen; i++)
		if (_pPVS[i])
		{
			int j = 1;
			while(j != 256)
			{
				if (iPL >= nPL) break;
				if (_pPVS[i] & j)
				{
					nRet += EnumPortalLeaf(iPL, &_piRet[nRet], _MaxRet-nRet);
				}

				j += j;
				iPL++;
			}
			if (j != 256) break;
		}
		else
			iPL += 8;

	// Merge with incomming
	if (_nMerge) nRet = MergeIDs(_piRet, nRet, _MaxRet, _pMergeElem, _nMerge);

	// Untag all that were added.
	for(i = 0; i < nRet; i++)
		m_pIDLinkMap[_piRet[i]] &= ~0x8000;

	return nRet;
}

int CBSP3_LinkContext::EnumIDs(int _iPL, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP3_LinkContext_EnumIDs, 0);
	if (_nMerge)
	{
		// Add IDs in PL.
		int nRet = EnumPortalLeaf(_iPL, _piRet, _MaxRet);

		// Merge with incomming
		nRet = MergeIDs(_piRet, nRet, _MaxRet, _pMergeElem, _nMerge);

		// Untag all that were added.
		for(int i = 0; i < nRet; i++)
			m_pIDLinkMap[_piRet[i]] &= ~0x8000;
		return nRet;
	}
	else
	{
		int nRet = 0;
		int iPL = _iPL;

		int iLink = m_pPLLinkMap[iPL];
		while(iLink)
		{
			M_ASSERT(m_lLinks.ValidPos(iLink), CStrF("Link out of range. %d/%d", iLink, m_lLinks.Len()));
#ifndef	M_RTM
			if (nRet >= _MaxRet) Error("EnumVisible", CStrF("Out of enumeration space. (%d)", _MaxRet));
#endif
			// Add to list
			_piRet[nRet++] = m_pLinks[iLink].m_ID;

			// Next...
			iLink = m_pLinks[iLink].m_iLinkNextObject;
		}
		return nRet;
	}
}

int CBSP3_LinkContext::EnumPortalLeaves(int _ID, uint16* _piRet, int _MaxRet)
{
	MAUTOSTRIP(CBSP3_LinkContext_EnumPortalLeaves, 0);
	int nRet = 0;
	int iLink = m_pIDLinkMap[_ID];
	while(iLink)
	{
		const CBSP3_Link* pL = &m_pLinks[iLink];
#ifndef	M_RTM
		if (nRet >= _MaxRet) Error("EnumPortalLeaves", CStr("Out of enumeration space. (%d)") );
#endif
		_piRet[nRet++] = pL->m_iPortalLeaf;
		iLink = pL->m_iLinkNextPL;
	}
	return nRet;
}

int CBSP3_LinkContext::EnumPortalLeaves_Single(int _ID)
{
	MAUTOSTRIP(CBSP3_LinkContext_EnumPortalLeaves_Single, 0);
	int iLink = m_pIDLinkMap[_ID];
	if (iLink)
		return m_pLinks[iLink].m_iPortalLeaf;
	else
		return -1;
}

void CBSP3_LinkContext::AddPortalLeaf(int _iPL)
{
	MAUTOSTRIP(CBSP3_LinkContext_AddPortalLeaf, MAUTOSTRIP_VOID);
	MSCOPESHORT(CBSP3_LinkContext::AddPortalLeaf); //AR-SCOPE

	if (m_bPLBlock)
	{
		if (m_lPLBlock[_iPL >> 3] & (1 << (_iPL & 7)))
			return;

		m_lPLBlock[_iPL >> 3] |= (1 << (_iPL & 7));
	}


	int iLink = m_iNextLink;
	if (!iLink)
	{
		// Create new link.
		iLink= m_LinkHeap.AllocID();
		m_LinkHeapMinAvail = Min(m_LinkHeapMinAvail, m_LinkHeap.MaxAvail());

		if (iLink < 0)
		{
			//Error("Insert", "Unable to allocate link.");
#ifndef	M_RTM
			// Only report this problem once every second.
			static CMStopTimer g_LastLogTime(1.0f);
			if (g_LastLogTime.Done())
			{
				M_TRACEALWAYS("(CBSP3_LinkContext::AddPortalLeaf) Unable to allocate link.\n");
				g_LastLogTime.Reset(1.0f);
			}
#endif
//			M_TRACEALWAYS("*");
			return;
		}
		
		m_pLinks[iLink].m_iPortalLeaf = 0;
		m_pLinks[iLink].m_iLinkPrevObject = 0;
		m_pLinks[iLink].m_iLinkNextObject = 0;
		m_pLinks[iLink].m_iLinkPrevPL = 0;
		m_pLinks[iLink].m_iLinkNextPL = 0;
	}

	if (m_iPrevLink)
		m_pLinks[m_iPrevLink].m_iLinkNextPL = iLink;
	else
		m_lIDLinkMap[m_CurLinkID] = iLink;

	m_pLinks[iLink].m_iLinkPrevPL = m_iPrevLink;

//ConOut(CStrF("(AddPortalLeaf) PL %d, ID %d, iLink %d", _iPL, m_CurLinkID, iLink) );
	m_pLinks[iLink].m_ID = m_CurLinkID;

	if (m_pLinks[iLink].m_iPortalLeaf != _iPL)
	{
		CBSP3_Link* pL = &m_pLinks[iLink];
		int PrevObj = pL->m_iLinkPrevObject;
		int NextObj = pL->m_iLinkNextObject;

		// unlink prev
		if (PrevObj)
			m_pLinks[PrevObj].m_iLinkNextObject = NextObj;
		else
			m_lPLLinkMap[pL->m_iPortalLeaf] = NextObj;

		// unlink next
		if (NextObj)
			m_pLinks[NextObj].m_iLinkPrevObject = PrevObj;

		// link into new PL chain.
		if (m_lPLLinkMap[_iPL])
			m_pLinks[m_lPLLinkMap[_iPL]].m_iLinkPrevObject = iLink;
		pL->m_iLinkNextObject = m_lPLLinkMap[_iPL];
		pL->m_iLinkPrevObject = 0;
		m_lPLLinkMap[_iPL] = iLink;

		// Set correct PL
		m_pLinks[iLink].m_iPortalLeaf = _iPL;
	}

	m_iPrevLink = iLink;
	m_iNextLink = m_pLinks[iLink].m_iLinkNextPL;
}

void CBSP3_LinkContext::ExpandCurrentBox(const CBox3Dfp32& _Box)
{
	m_lIDLinkBox[m_CurLinkID].Expand(_Box);
}

int CBSP3_LinkContext::GetNumLinkedIDs() const
{
	int nIDs = m_lIDLinkMap.Len();

	int nLinked = 0;
	for(int i = 0; i < nIDs; i++)
		if (m_pIDLinkMap[i])
			nLinked++;

	return nLinked;
}

void CBSP3_LinkContext::TraceContents() const
{
	int nIDs = m_lIDLinkMap.Len();

	for(int i = 0; i < nIDs; i++)
		if (m_pIDLinkMap[i])
		{
			int nLinks = 0;
			int iLink = m_pIDLinkMap[i];
			while(iLink)
			{
				nLinks++;
				iLink = m_pLinks[iLink].m_iLinkNextPL;
			}

			const CBox3Dfp32& Box = m_lIDLinkBox[i];
			int Flags = m_lIDFlags[i];
			M_TRACEALWAYS("         ID: %.3d, Flg %d, Box: (%.2f,%.2f,%.2f - %.2f,%.2f,%.2f), PLs(%d): ", i, Flags, Box.m_Min[0], Box.m_Min[1], Box.m_Min[2], Box.m_Max[0], Box.m_Max[1], Box.m_Max[2], nLinks);

			iLink = m_pIDLinkMap[i];
			while(iLink)
			{
				M_TRACEALWAYS("%.3d, ", m_pLinks[iLink].m_iPortalLeaf);


				iLink = m_pLinks[iLink].m_iLinkNextPL;
			}
			M_TRACEALWAYS("\n");
		}
}


// -------------------------------------------------------------------
//  CXR_Model_BSP3
// -------------------------------------------------------------------
void CXR_Model_BSP3::LinkBox(CBSP3_LinkContext* _pLinkCtx, uint16 _iNode, const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP3_LinkBox, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::LinkBox);
	LinkBox_i( _pLinkCtx, _iNode, _Box );
}

void CXR_Model_BSP3::LinkBox_r(CBSP3_LinkContext* _pLinkCtx, uint16 _iNode, const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP3_LinkBox_r, MAUTOSTRIP_VOID);

	if (!_iNode) return;
	const CBSP3_Node* pN = &m_pNodes[_iNode];
	if (pN->IsStructureLeaf())
	{
		_pLinkCtx->AddPortalLeaf(pN->m_iPortalLeaf);
		return;
	}
	else if (pN->IsNode())
	{
//LogFile(CStrF("Prnt %d, N %d, ChildF %d ChildB %d", pN->m_iNodeParent, _iNode, pN->m_iNodeFront, pN->m_iNodeBack));
		uint16 iBack = pN->m_iNodeBack;
		if (!iBack)
		{
			uint16 iFront = pN->m_iNodeFront;
			if (m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_Box.m_Min, _Box.m_Max) > LINKBOX_EPSILON)
				LinkBox_r(_pLinkCtx, iFront, _Box);
		}
		else
		{
			uint16 iFront = pN->m_iNodeFront;
			if (!iFront)
			{
				if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) < -LINKBOX_EPSILON)
					LinkBox_r(_pLinkCtx, iBack, _Box);
			}
			else
			{
				fp32 MinDist, MaxDist;
				m_pPlanes[pN->m_iPlane].GetBoxMinAndMaxDistance( _Box.m_Min, _Box.m_Max, MinDist, MaxDist );
				if( MaxDist > LINKBOX_EPSILON )
					LinkBox_r(_pLinkCtx, iFront, _Box);
				if( MinDist < -LINKBOX_EPSILON )
					LinkBox_r(_pLinkCtx, iBack, _Box);
			}
		}
	}
}

void CXR_Model_BSP3::LinkBox_i(CBSP3_LinkContext* _pLinkCtx, uint16 _iNode, const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP3_LinkBox_i, MAUTOSTRIP_VOID);

	if (!_iNode) return;

	uint16 aWorkingSet[256];
	int WorkingStack = 0;
	
	aWorkingSet[WorkingStack++]	= _iNode;
StartOf_LinkBox_i:
	uint16 WorkingNode = aWorkingSet[--WorkingStack];

StartOf_LinkBox_i_NoAdd:
	const CBSP3_Node* pN = &m_pNodes[WorkingNode];

	if (pN->IsStructureLeaf())
	{
		_pLinkCtx->AddPortalLeaf(pN->m_iPortalLeaf);
	}
	else if (pN->IsNode())
	{
//LogFile(CStrF("Prnt %d, N %d, ChildF %d ChildB %d", pN->m_iNodeParent, _iNode, pN->m_iNodeFront, pN->m_iNodeBack));
		uint16 iBack = pN->m_iNodeBack;
		if (!iBack)
		{
			uint16 iFront = pN->m_iNodeFront;
			if (iFront && m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_Box.m_Min, _Box.m_Max) > LINKBOX_EPSILON)
			{
				WorkingNode = iFront;
				goto StartOf_LinkBox_i_NoAdd;
			}
		}
		else
		{
			uint16 iFront = pN->m_iNodeFront;
			if (!iFront)
			{
				if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) < -LINKBOX_EPSILON)
				{
					WorkingNode = iBack;
					goto StartOf_LinkBox_i_NoAdd;
				}
			}
			else
			{
				fp32 MinDist, MaxDist;
				m_pPlanes[pN->m_iPlane].GetBoxMinAndMaxDistance( _Box.m_Min, _Box.m_Max, MinDist, MaxDist );
				if( MaxDist > LINKBOX_EPSILON )
					aWorkingSet[WorkingStack++]	= iFront;

				if( MinDist < -LINKBOX_EPSILON )
				{
					WorkingNode = iBack;
					goto StartOf_LinkBox_i_NoAdd;
				}
			}
		}
	}
	
	if( WorkingStack > 0 )
		goto StartOf_LinkBox_i;
}

void CXR_Model_BSP3::LinkBoxWithPVS(CBSP3_LinkContext* _pLinkCtx, uint16 _iNode, const CBox3Dfp32& _Box, const uint8* _pPVS)
{
	MAUTOSTRIP(CXR_Model_BSP3_LinkBoxWithPVS, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::LinkBoxWithPVS);
	LinkBoxWithPVS_r( _pLinkCtx, _iNode, _Box, _pPVS );
}

void CXR_Model_BSP3::LinkBoxWithPVS_r(CBSP3_LinkContext* _pLinkCtx, uint16 _iNode, const CBox3Dfp32& _Box, const uint8* _pPVS)
{
	MAUTOSTRIP(CXR_Model_BSP3_LinkBoxWithPVS_r, MAUTOSTRIP_VOID);

	if (!_iNode) return;
	const CBSP3_Node* pN = &m_pNodes[_iNode];
	if (pN->IsStructureLeaf())
	{
		int iPL = pN->m_iPortalLeaf;
		if (_pPVS[iPL >> 3] & (1 << (iPL & 7)))
		{
			_pLinkCtx->AddPortalLeaf(iPL);

			CBox3Dfp32 BoxAnd(_Box);
			BoxAnd.And(m_pPortalLeaves[iPL].m_BoundBox);

			_pLinkCtx->ExpandCurrentBox(BoxAnd);
		}
		return;
	}
	else if (pN->IsNode())
	{
//LogFile(CStrF("Prnt %d, N %d, ChildF %d ChildB %d", pN->m_iNodeParent, _iNode, pN->m_iNodeFront, pN->m_iNodeBack));
		uint16 iBack = pN->m_iNodeBack;
		if (!iBack)
		{
			uint16 iFront = pN->m_iNodeFront;
			if (m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_Box.m_Min, _Box.m_Max) > LINKBOX_EPSILON)
				LinkBoxWithPVS_r(_pLinkCtx, iFront, _Box,  _pPVS);
		}
		else
		{
			uint16 iFront = pN->m_iNodeFront;
			if (!iFront)
			{
				if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) < -LINKBOX_EPSILON)
					LinkBoxWithPVS_r(_pLinkCtx, iBack, _Box, _pPVS);
			}
			else
			{
				fp32 MinDist, MaxDist;
				m_pPlanes[pN->m_iPlane].GetBoxMinAndMaxDistance( _Box.m_Min, _Box.m_Max, MinDist, MaxDist );
				if( MaxDist > LINKBOX_EPSILON )
					LinkBoxWithPVS_r(_pLinkCtx, iFront, _Box, _pPVS);
				if( MinDist < -LINKBOX_EPSILON )
					LinkBoxWithPVS_r(_pLinkCtx, iBack, _Box, _pPVS);
			}
		}
	}
}



// -------------------------------------------------------------------
//static int ms_EnumPL = 0;
uint32* CXR_Model_BSP3::ms_piEnum = NULL;
uint16* CXR_Model_BSP3::ms_piEnumPL = NULL;
int CXR_Model_BSP3::ms_nEnum = 0;
int CXR_Model_BSP3::ms_nEnumUntag = 0;
int CXR_Model_BSP3::ms_MaxEnum = 0;
int CXR_Model_BSP3::ms_EnumQuality = 0;
int CXR_Model_BSP3::ms_EnumMedium = 0;
int CXR_Model_BSP3::ms_EnumFaceFlags = 0;
CBox3Dfp32 CXR_Model_BSP3::ms_EnumBox;
CVec3Dfp32 CXR_Model_BSP3::ms_EnumSphere;
fp32 CXR_Model_BSP3::ms_EnumSphereR = 0;
bool CXR_Model_BSP3::ms_EnumError = false;

CPotColSet *CXR_Model_BSP3::ms_pcs;
int CXR_Model_BSP3::ms_iObj;
uint8 CXR_Model_BSP3::ms_bIN1N2Flags;

/*
void CXR_Model_BSP3::EnumFaces_Sphere_r(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnumFaces_Sphere_r, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::EnumFaces_Sphere_r); //AR-SCOPE

	if (!_iNode) return;
	const CBSP3_Node* pN = &m_lNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		ms_EnumPL = pN->m_iPortalLeaf;
	}

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;
		uint8* pFTag = m_lFaceTagList.GetBasePtr();
		uint16* piFUntag = m_lFaceUntagList.GetBasePtr();
		if (m_lFaceTagList.Len()*8 < m_lFaces.Len() ||
			m_lFaceUntagList.Len() < m_lFaces.Len())
			Error("EnumFaces_Sphere_r", "Tag array not initialized.");
		int MaxFaceUntag = m_lFaceUntagList.Len();

		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_piFaces[iiFaces + f];
			if (pFTag[iFace >> 3] & (1 << (iFace & 7))) continue;
			if (!pFTag[iFace >> 3])
			{
				if (ms_nEnumUntag >= MaxFaceUntag)
					M_ASSERT(0, "?");
//					Error("EnumFaces_Sphere_r", CStrF("Out of range. %d, %d, %d, %d", m_lFaces.Len(), pN->m_nFaces, ms_nEnumUntag, MaxFaceUntag));
				piFUntag[ms_nEnumUntag++] = iFace;
			}
			pFTag[iFace >> 3] |= 1 << (iFace & 7);

			CBSP3_Face* pF = &m_pFaces[iFace];

			// Check face flag?
			if (ms_EnumQuality & ENUM_FACEFLAGS && !(ms_EnumFaceFlags & pF->m_Flags)) continue;

			// Check medium
			if (ms_EnumQuality & ENUM_MEDIUMFLAGS && !(m_lMediums[pF->m_iBackMedium].m_MediumFlags & ms_EnumMedium)) continue;

			int iPlane = pF->m_iPlane;
			fp32 Dist = m_pPlanes[iPlane].Distance(ms_EnumSphere);
			if (M_Fabs(Dist) < ms_EnumSphereR)
			{
				if (ms_nEnum < ms_MaxEnum)
				{
					ms_piEnum[ms_nEnum] = iFace;
					if (ms_piEnumPL) ms_piEnumPL[ms_nEnum] = ms_EnumPL;
					ms_nEnum++;
				}
				else
				{
					ms_EnumError = true;
					return;
				}
			}
		}
	}
	else if (pN->IsNode())
	{
		fp32 Dist = m_pPlanes[pN->m_iPlane].Distance(ms_EnumSphere);
		if (Dist < ms_EnumSphereR)
			EnumFaces_Sphere_r(pN->m_iNodeBack);
		if (Dist > -ms_EnumSphereR)
			EnumFaces_Sphere_r(pN->m_iNodeFront);
	}
}
*/
void CXR_Model_BSP3::EnumFaces_Box_i(CXR_PhysicsContext* _pPhysContext, int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnumFaces_Box_i, MAUTOSTRIP_VOID);

	if (!_iNode) return;

	uint8* pFTag = m_lFaceTagList.GetBasePtr();
	uint16* piFUntag = m_lFaceUntagList.GetBasePtr();
#ifndef	M_RTM
	if (m_lFaceTagList.Len()*8 < m_lFaces.Len() ||
		m_lFaceUntagList.Len() < m_lFaces.Len())
		Error("EnumFaces_Box_i", "Tag array not initialized.");
#endif	// M_RTM
	int MaxFaceUntag = m_lFaceUntagList.Len();
	const CXR_MediumDesc* pMediums = m_lMediums.GetBasePtr();

	uint16 EnumPL = 0;
	uint16 aWorkingSet[64];
	int WorkingStack = 0;
	aWorkingSet[WorkingStack++]	= _iNode;

StartOf_EnumFaces_Box_i:
#ifdef	MODEL_BSP3_BOUNDCHECK_STACK
	EnumFaces_Box_i_MaxDepth = Max( EnumFaces_Box_i_MaxDepth, WorkingStack );
#endif	// MODEL_BSP3_BOUNDCHECK_STACK
	uint16 WorkingNode = aWorkingSet[--WorkingStack];

StartOf_EnumFaces_Box_i_NoAdd:
	const CBSP3_Node* pN = &m_lNodes[WorkingNode];

	if (pN->IsStructureLeaf())
	{
		EnumPL = pN->m_iPortalLeaf;
	}

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;

		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_piFaces[iiFaces + f];
			{
				int iFaceIdx = iFace >> 3;
				uint8 iFaceMask = 1 << (iFace & 0x7);
				if (pFTag[iFaceIdx] & iFaceMask) continue;
				if (!pFTag[iFaceIdx])
				{
					if (ms_nEnumUntag >= MaxFaceUntag)
						M_ASSERT(0, "?");
					piFUntag[ms_nEnumUntag++] = iFace;
				}
				pFTag[iFaceIdx] |= iFaceMask;
			}

#ifdef	BSP3_MODEL_PHYS_RENDER
			if (_pPhysContext->m_pWC) 
				__Phys_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xff100020);
#endif	// BSP3_MODEL_PHYS_RENDER

			const CBSP3_Face* pF = &m_pFaces[iFace];
			int iPlane = pF->m_iPlane;
			fp32 MinDist = m_pPlanes[iPlane].GetBoxMinDistance(ms_EnumBox.m_Min, ms_EnumBox.m_Max);
			if (MinDist > MODEL_BSP_EPSILON) continue;

			// Check face flag?
			if (ms_EnumQuality & ENUM_FACEFLAGS && !(ms_EnumFaceFlags & pF->m_Flags)) continue;

			// Check medium
			if (ms_EnumQuality & ENUM_MEDIUMFLAGS && !(pMediums[pF->m_iBackMedium].m_MediumFlags & ms_EnumMedium)) continue;

// Box can't be behind face.
//			fp32 MaxDist = m_pPlanes[iPlane].GetBoxMaxDistance(ms_EnumBox.m_Min, ms_EnumBox.m_Max);
//			if (MaxDist < -MODEL_BSP_EPSILON) continue;

			// Check face bounding box
			if (ms_EnumQuality & ENUM_HQ)
			{
				int nv = pF->m_nVertices;
				int iiv = pF->m_iiVertices;

				{
					CVec3Dfp32 PMin = m_pVertices[m_piVertices[iiv]];
					CVec3Dfp32 PMax = PMin;
					
					for( int v = 1; v < nv; v++ )
					{
						const CVec3Dfp32& V = m_pVertices[m_piVertices[iiv + v]];
						PMin.k[0]	= Min( PMin.k[0], V.k[0] );
						PMax.k[0]	= Max( PMax.k[0], V.k[0] );
						PMin.k[1]	= Min( PMin.k[1], V.k[1] );
						PMax.k[1]	= Max( PMax.k[1], V.k[1] );
						PMin.k[2]	= Min( PMin.k[2], V.k[2] );
						PMax.k[2]	= Max( PMax.k[2], V.k[2] );
					}
					if (ms_EnumBox.m_Min.k[0] > PMax.k[0] || ms_EnumBox.m_Max.k[0] < PMin.k[0] ||
						ms_EnumBox.m_Min.k[1] > PMax.k[1] || ms_EnumBox.m_Max.k[1] < PMin.k[1] ||
						ms_EnumBox.m_Min.k[2] > PMax.k[2] || ms_EnumBox.m_Max.k[2] < PMin.k[2])
						continue;
				}
			}

			if (ms_nEnum < ms_MaxEnum)
			{
				ms_piEnum[ms_nEnum] = iFace;
				if (ms_piEnumPL) ms_piEnumPL[ms_nEnum] = EnumPL;
				ms_nEnum++;
//				ConOut(CStrF("    Added face %d  (%d)", iFace, ms_nEnum-1));
			}
			else
			{
				ms_EnumError = true;
				return;
			}
		}
	}
	else if (pN->IsNode())
	{
		fp32 MinDist, MaxDist;
		m_pPlanes[pN->m_iPlane].GetBoxMinAndMaxDistance(ms_EnumBox.m_Min, ms_EnumBox.m_Max, MinDist, MaxDist);
		
		uint16 iNodeBack = pN->m_iNodeBack;
		uint16 iNodeFront = pN->m_iNodeFront;

		// Traverse back first (so we're compatible with the recursive version)
		if (iNodeFront > 0 && MaxDist > -MODEL_BSP_EPSILON)
			aWorkingSet[WorkingStack++]	= iNodeFront;
		if (iNodeBack > 0 && MinDist < MODEL_BSP_EPSILON)
		{
			WorkingNode = iNodeBack;
			goto StartOf_EnumFaces_Box_i_NoAdd;
		}
	}
	
	if( WorkingStack > 0 )
		goto StartOf_EnumFaces_Box_i;

	return;
}
/*
void CXR_Model_BSP3::EnumFaces_Box_r(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnumFaces_Box_r, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	const CBSP3_Node* pN = &m_lNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		ms_EnumPL = pN->m_iPortalLeaf;
	}

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;
		uint8* pFTag = m_lFaceTagList.GetBasePtr();
		uint16* piFUntag = m_lFaceUntagList.GetBasePtr();
		if (m_lFaceTagList.Len()*8 < m_lFaces.Len() ||
			m_lFaceUntagList.Len() < m_lFaces.Len())
			Error("EnumFaces_Sphere_r", "Tag array not initialized.");
		int MaxFaceUntag = m_lFaceUntagList.Len();

		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_piFaces[iiFaces + f];
			if (pFTag[iFace >> 3] & (1 << (iFace & 7))) continue;
			if (!pFTag[iFace >> 3])
			{
				if (ms_nEnumUntag >= MaxFaceUntag)
					M_ASSERT(0, "?");
//					Error("EnumFaces_Box_r", "Out of range.");
				piFUntag[ms_nEnumUntag++] = iFace;
			}
			pFTag[iFace >> 3] |= 1 << (iFace & 7);

			if (ms_pPhysWC) 
				__Phys_RenderFace(iFace, ms_PhysWMat, ms_pPhysWC, 0xff100020);

			CBSP3_Face* pF = &m_pFaces[iFace];
			int iPlane = pF->m_iPlane;
			fp32 MinDist = m_pPlanes[iPlane].GetBoxMinDistance(ms_EnumBox.m_Min, ms_EnumBox.m_Max);
			if (MinDist > MODEL_BSP_EPSILON) continue;

			// Check face flag?
			if (ms_EnumQuality & ENUM_FACEFLAGS && !(ms_EnumFaceFlags & pF->m_Flags)) continue;

			// Check medium
			if (ms_EnumQuality & ENUM_MEDIUMFLAGS && !(m_lMediums[pF->m_iBackMedium].m_MediumFlags & ms_EnumMedium)) continue;

// Box can't be behind face.
//			fp32 MaxDist = m_pPlanes[iPlane].GetBoxMaxDistance(ms_EnumBox.m_Min, ms_EnumBox.m_Max);
//			if (MaxDist < -MODEL_BSP_EPSILON) continue;

			// Check face bounding box
			if (ms_EnumQuality & ENUM_HQ)
			{
				int nv = pF->m_nVertices;
				int iiv = pF->m_iiVertices;

				for(int k = 0; k < 3; k++)
				{
					fp32 PMin = _FP32_MAX;
					fp32 PMax = -_FP32_MAX;
					for(int v = 0; v < nv; v++)
					{
						const CVec3Dfp32& V = m_pVertices[m_piVertices[iiv + v]];
						if (V.k[k] < PMin) PMin = V.k[k];
						if (V.k[k] > PMax) PMax = V.k[k];
					}

					if (ms_EnumBox.m_Min[k] > PMax)
						goto Continue;
					if (ms_EnumBox.m_Max[k] < PMin)
						goto Continue;
				}

#ifdef NEVER
				// Get bound-box for polygon.
				CBox3Dfp32 PBox;
				PBox.m_Min = _FP32_MAX;
				PBox.m_Max = -_FP32_MAX;
				for(int v = 0; v < nv; v++)
				{
					const CVec3Dfp32& V = m_pVertices[m_piVertices[iiv + v]];
					for(int k = 0; k < 3; k++)
					{
						if (V.k[k] < PBox.m_Min.k[k]) PBox.m_Min.k[k] = V.k[k];
						if (V.k[k] > PBox.m_Max.k[k]) PBox.m_Max.k[k] = V.k[k];
					}
				}
				if (!PBox.IsInside(ms_EnumBox)) continue;
#endif
			}

			if (ms_nEnum < ms_MaxEnum)
			{
				ms_piEnum[ms_nEnum] = iFace;
				if (ms_piEnumPL) ms_piEnumPL[ms_nEnum] = ms_EnumPL;
				ms_nEnum++;
//				ConOut(CStrF("    Added face %d  (%d)", iFace, ms_nEnum-1));
			}
			else
			{
				ms_EnumError = true;
				return;
			}
Continue: ;
		}
	}
	else if (pN->IsNode())
	{
		fp32 MinDist, MaxDist;
		m_pPlanes[pN->m_iPlane].GetBoxMinAndMaxDistance(ms_EnumBox.m_Min, ms_EnumBox.m_Max, MinDist, MaxDist);

		if (MinDist < MODEL_BSP_EPSILON)
			EnumFaces_Box_r(pN->m_iNodeBack);
		if (MaxDist > -MODEL_BSP_EPSILON)
			EnumFaces_Box_r(pN->m_iNodeFront);
	}
}
*/
void CXR_Model_BSP3::EnumFaces_All_i(CXR_PhysicsContext* _pPhysContext, int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnumFaces_All_i, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::EnumFaces_All_i);

	if (!_iNode) return;
	
	uint16 EnumPL = 0;
	uint16 aWorkingSet[256];
	int WorkingStack = 0;
	aWorkingSet[WorkingStack++]	= _iNode;
	
StartOf_EnumFaces_All_i:
#ifdef	MODEL_BSP3_BOUNDCHECK_STACK
	EnumFaces_All_i_MaxDepth = Max( EnumFaces_All_i_MaxDepth, WorkingStack );
#endif	// MODEL_BSP3_BOUNDCHECK_STACK
	uint16 WorkingNode = aWorkingSet[--WorkingStack];
	
	const CBSP3_Node* pN = &m_lNodes[WorkingNode];

	if (pN->IsStructureLeaf())
	{
		EnumPL = pN->m_iPortalLeaf;
	}

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;
		uint8* pFTag = m_lFaceTagList.GetBasePtr();
		uint16* piFUntag = m_lFaceUntagList.GetBasePtr();
#ifndef	M_RTM
		if (m_lFaceTagList.Len()*8 < m_lFaces.Len() ||
			m_lFaceUntagList.Len() < m_lFaces.Len())
			Error("EnumFaces_Sphere_r", "Tag array not initialized.");
#endif
		int MaxFaceUntag = m_lFaceUntagList.Len();

		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_piFaces[iiFaces + f];
			if (pFTag[iFace >> 3] & (1 << (iFace & 7))) continue;
			if (!pFTag[iFace >> 3])
			{
				if (ms_nEnumUntag >= MaxFaceUntag)
					M_ASSERT(0, "?");
//					Error("EnumFaces_Box_r", "Out of range.");
				piFUntag[ms_nEnumUntag++] = iFace;
			}
			pFTag[iFace >> 3] |= 1 << (iFace & 7);

			if (ms_nEnum < ms_MaxEnum)
			{
				ms_piEnum[ms_nEnum] = iFace;
				if (ms_piEnumPL) ms_piEnumPL[ms_nEnum] = EnumPL;
				ms_nEnum++;
			}
			else
			{
				ms_EnumError = true;
				return;
			}
		}
	}
	else if (pN->IsNode())
	{
		aWorkingSet[WorkingStack++]	= pN->m_iNodeFront;
		aWorkingSet[WorkingStack++]	= pN->m_iNodeBack;
	}
	
	if( WorkingStack > 0 )
		goto StartOf_EnumFaces_All_i;
}
/*
void CXR_Model_BSP3::EnumFaces_All_r(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnumFaces_All_r, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::EnumFaces_All_r); //AR-SCOPE

	if (!_iNode) return;
	CBSP3_Node* pN = &m_lNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		ms_EnumPL = pN->m_iPortalLeaf;
	}

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;
		uint8* pFTag = m_lFaceTagList.GetBasePtr();
		uint16* piFUntag = m_lFaceUntagList.GetBasePtr();
		if (m_lFaceTagList.Len()*8 < m_lFaces.Len() ||
			m_lFaceUntagList.Len() < m_lFaces.Len())
			Error("EnumFaces_Sphere_r", "Tag array not initialized.");
		int MaxFaceUntag = m_lFaceUntagList.Len();

		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_piFaces[iiFaces + f];
			if (pFTag[iFace >> 3] & (1 << (iFace & 7))) continue;
			if (!pFTag[iFace >> 3])
			{
				if (ms_nEnumUntag >= MaxFaceUntag)
					M_ASSERT(0, "?");
//					Error("EnumFaces_Box_r", "Out of range.");
				piFUntag[ms_nEnumUntag++] = iFace;
			}
			pFTag[iFace >> 3] |= 1 << (iFace & 7);

			if (ms_nEnum < ms_MaxEnum)
			{
				ms_piEnum[ms_nEnum] = iFace;
				if (ms_piEnumPL) ms_piEnumPL[ms_nEnum] = ms_EnumPL;
				ms_nEnum++;
			}
			else
			{
				ms_EnumError = true;
				return;
			}
		}
	}
	else if (pN->IsNode())
	{
		EnumFaces_All_r(pN->m_iNodeBack);
		EnumFaces_All_r(pN->m_iNodeFront);
	}
}
*/
/*
int CXR_Model_BSP3::EnumFaces_Sphere(int _iNode, uint16* _piFaces, int _MaxFaces, int _Quality, int _Medium, int _FaceFlags, const CVec3Dfp32& _Origin, fp32 _Radius, uint16* _piFacePL)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnumFaces_Sphere, 0);
	ms_EnumSphere = _Origin;
	ms_EnumSphereR = _Radius;
	ms_piEnum = _piFaces;
	ms_piEnumPL = _piFacePL;
	ms_MaxEnum = _MaxFaces;
	ms_nEnum = 0;
	ms_EnumQuality = _Quality;
	ms_EnumMedium = _Medium;
	ms_EnumFaceFlags = _FaceFlags;
	ms_EnumError = 0;
	ms_EnumPL = 0;

	EnumFaces_Sphere_r(_iNode);
	return ms_nEnum;
}
*/
int CXR_Model_BSP3::EnumFaces_Box(CXR_PhysicsContext* _pPhysContext, int _iNode, uint32* _piFaces, int _MaxFaces, int _Quality, int _Medium, int _FaceFlags, const CBox3Dfp32& _Box, uint16* _piFacePL)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnumFaces_Box, 0);
	MSCOPESHORT(CXR_Model_BSP3::EnumFaces_Box);

	ms_EnumBox = _Box;
	ms_piEnum = _piFaces;
	ms_piEnumPL = _piFacePL;
	ms_MaxEnum = _MaxFaces;
	ms_nEnum = 0;
	ms_EnumQuality = _Quality;
	ms_EnumMedium = _Medium;
	ms_EnumFaceFlags = _FaceFlags;
	ms_EnumError = 0;
//	ms_EnumPL = 0;
//EnumFaces_Untag();
//	EnumFaces_Box_r(_iNode);
	EnumFaces_Box_i(_pPhysContext, _iNode);
#ifdef	MODEL_BSP3_BOUNDCHECK_STACK
	if( EnumFaces_Box_i_MaxDepth != EnumFaces_Box_i_MaxDepth_LastPeak )
	{
		EnumFaces_Box_i_MaxDepth_LastPeak	= EnumFaces_Box_i_MaxDepth;
		scePrintf( "EnumFaces_Box_i::New PeakStack %d\n", EnumFaces_Box_i_MaxDepth_LastPeak );
	}
#endif
	return ms_nEnum;
}

int CXR_Model_BSP3::EnumFaces_All(CXR_PhysicsContext* _pPhysContext, int _iNode, uint32* _piFaces, int _MaxFaces)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnumFaces_All, 0);
	ms_piEnum = _piFaces;
	ms_piEnumPL = NULL;
	ms_MaxEnum = _MaxFaces;
	ms_nEnum = 0;
	ms_EnumQuality = 0;
	ms_EnumMedium = 0;
	ms_EnumFaceFlags = 0;
	ms_EnumError = 0;
//	ms_EnumPL = 0;
//EnumFaces_Untag();
//	EnumFaces_All_r(_iNode);
	EnumFaces_All_i(_pPhysContext, _iNode);
#ifdef	MODEL_BSP3_BOUNDCHECK_STACK
	if( EnumFaces_All_i_MaxDepth != EnumFaces_All_i_MaxDepth_LastPeak )
	{
		EnumFaces_All_i_MaxDepth_LastPeak	= EnumFaces_All_i_MaxDepth;
		scePrintf( "EnumFaces_All_i::New PeakStack %d\n", EnumFaces_All_i_MaxDepth_LastPeak );
	}
#endif
	return ms_nEnum;
}


void CXR_Model_BSP3::EnumFaces_Untag(CXR_PhysicsContext* _pPhysContext)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnumFaces_Untag, MAUTOSTRIP_VOID);
	const uint16* piF = m_lFaceUntagList.GetBasePtr();
	uint8* pFTag = m_lFaceTagList.GetBasePtr();
	const int nEnumUntag = ms_nEnumUntag;
	for(int i = 0; i < nEnumUntag; i++)
	{
		int iFace = piF[i];
		pFTag[iFace >> 3] = 0;
	}
	ms_nEnumUntag = 0;
}

/*void CXR_Model_BSP3::EnumFaces_UntagExact(const uint32* _piFaces, int _nFaces)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnumFaces_UntagExact, NULL);
	uint8* pFTag = m_lFaceTagList.GetBasePtr();
	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = _piFaces[i];
		pFTag[iFace >> 3] &= ~(1 << (iFace & 7));
	}
}*/

// -------------------------------------------------------------------
int CXR_Model_BSP3::CountFaces_r(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP3_CountFaces_r, 0);
	MSCOPESHORT(CXR_Model_BSP3::CountFaces_r); //AR-SCOPE

	CBSP3_Node* pN = &m_lNodes[_iNode];
	if (pN->IsLeaf())
		return pN->m_nFaces;
	else
		return CountFaces_r(pN->m_iNodeFront) + CountFaces_r(pN->m_iNodeBack);
}

void CXR_Model_BSP3::InitFaces()
{
	MAUTOSTRIP(CXR_Model_BSP3_InitFaces, MAUTOSTRIP_VOID);
}

aint CXR_Model_BSP3::GetParam(int _Param)
{
	MAUTOSTRIP(CXR_Model_BSP3_GetParam, 0);
	switch(_Param)
	{
	case MODEL_BSP_PARAM_GLOBALFLAGS :
		return GetGlobalEnable();

#ifndef PLATFORM_CONSOLE
	case MODEL_PARAM_NUMTRIANGLES :
		return m_lFaces.Len();		// Not quite right, but...

	case MODEL_PARAM_NUMVERTICES :
		return m_lVertices.Len();	// Not quite right either, but...
#endif

	case CXR_MODEL_PARAM_TIMEMODE:
		return CXR_MODEL_TIMEMODE_CONTROLLED;
	
	case CXR_MODEL_PARAM_ISSHADOWCASTER :
		{
//			for(int i = 0; i < m_lspSurfaces.Len(); i++)
//			{
//				CXW_Surface* pSurf = m_lspSurfaces[i];
//				if (!(pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE) &&
//					!(pSurf->GetBaseFrame()->m_Medium.m_MediumFlags & XW_MEDIUM_SEETHROUGH))
//					return 1;
//			}
			return 0;
		}

	default :
		return CXR_Model::GetParam(_Param);
	}
}

void CXR_Model_BSP3::SetParam(int _Param, aint _Value)
{
	MAUTOSTRIP(CXR_Model_BSP3_SetParam, MAUTOSTRIP_VOID);
	switch(_Param)
	{
	case MODEL_BSP_PARAM_GLOBALFLAGS :
		GetGlobalEnable() = _Value;

	default :
		CXR_Model::SetParam(_Param, _Value);
	}
}

int CXR_Model_BSP3::GetParamfv(int _Param, fp32* _pRetValues)
{
	MAUTOSTRIP(CXR_Model_BSP3_GetParamfv, 0);
	return CXR_Model::GetParamfv(_Param, _pRetValues);
}

void CXR_Model_BSP3::SetParamfv(int _Param, const fp32* _pValues)
{
	MAUTOSTRIP(CXR_Model_BSP3_SetParamfv, MAUTOSTRIP_VOID);
	switch(_Param)
	{
	case CXR_MODEL_PARAM_TEXTUREPARAM :
		{
			for(int i = 0; i < m_lspSurfaces.Len(); i++)
				m_lspSurfaces[i]->SetTextureParam((int)_pValues[0], (int)_pValues[1]);
		}
		break;

	default :
		CXR_PhysicsModel::SetParamfv(_Param, _pValues);
	}
}

fp32 CXR_Model_BSP3::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_BSP3_GetBound_Sphere, 0.0f);
	return m_BoundRadius;
}

void CXR_Model_BSP3::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_BSP3_GetBound_Box, MAUTOSTRIP_VOID);
	_Box = m_BoundBox;
}

int CXR_Model_BSP3::NumFacesInTree(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP3_NumFacesInTree, 0);
	if (!_iNode) return 0;
	if (m_pNodes[_iNode].IsLeaf()) return m_pNodes[_iNode].m_nFaces;

	return NumFacesInTree(m_pNodes[_iNode].m_iNodeFront) + NumFacesInTree(m_pNodes[_iNode].m_iNodeBack);
}

int CXR_Model_BSP3::GetNumEnabledLeaves(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP3_GetNumEnabledLeaves, 0);
	if (!_iNode) return 0;
	if (!(m_pNodes[_iNode].m_Flags & XW_NODE_ENABLE)) return 0;

	if (m_pNodes[_iNode].IsNode())
		return GetNumEnabledLeaves(m_pNodes[_iNode].m_iNodeFront) + GetNumEnabledLeaves(m_pNodes[_iNode].m_iNodeBack);
	else
		return 1;
}

int CXR_Model_BSP3::GetFirstSplitNode_Sphere(const CVec3Dfp32& _Pos, fp32 _Radius)
{
	MAUTOSTRIP(CXR_Model_BSP3_GetFirstSplitNode_Sphere, 0);
	int iNode = 1;
	while(1)
	{
		int iPlane = m_pNodes[iNode].m_iPlane;
		if (!iPlane) return iNode;
		CPlane3Dfp32* pP = &m_pPlanes[iPlane];

		fp32 d = pP->Distance(_Pos);
		if ((d < _Radius) && (d > -_Radius)) return iNode;
		iNode = (d > 0) ? m_pNodes[iNode].m_iNodeFront : m_pNodes[iNode].m_iNodeBack;
		if (!iNode) return 0;
	}
}

int CXR_Model_BSP3::GetFirstSplitNode_Box(const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax)
{
	MAUTOSTRIP(CXR_Model_BSP3_GetFirstSplitNode_Box, 0);
	Error("GetFirstSplitNode_Box", "FIXME! I'm broken!");
	int iNode = 1;

	int SideMask = 0;
	do
	{
		int iPlane = m_pNodes[iNode].m_iPlane;
		if (!iPlane) return iNode;
		CPlane3Dfp32* pP = &m_pPlanes[iPlane];

		fp32 d = -pP->d;

		fp32 xp0 = pP->n.k[0] * _BoxMin.k[0];
		fp32 xp1 = pP->n.k[0] * _BoxMax.k[0];
		fp32 yp0 = pP->n.k[1] * _BoxMin.k[1];
		fp32 yp1 = pP->n.k[1] * _BoxMax.k[1];
		fp32 zp0 = pP->n.k[2] * _BoxMin.k[2];
		fp32 zp1 = pP->n.k[2] * _BoxMax.k[2];

		SideMask |= (xp0 + yp0 + zp0 > d) ? 1 : 2;
		SideMask |= (xp0 + yp0 + zp1 > d) ? 1 : 2;
		if (SideMask == 3) break;
		SideMask |= (xp0 + yp1 + zp0 > d) ? 1 : 2;
		if (SideMask == 3) break;
		SideMask |= (xp0 + yp1 + zp1 > d) ? 1 : 2;
		if (SideMask == 3) break;

		SideMask |= (xp1 + yp0 + zp0 > d) ? 1 : 2;
		if (SideMask == 3) break;
		SideMask |= (xp1 + yp0 + zp1 > d) ? 1 : 2;
		if (SideMask == 3) break;
		SideMask |= (xp1 + yp1 + zp0 > d) ? 1 : 2;
		if (SideMask == 3) break;
		SideMask |= (xp1 + yp1 + zp1 > d) ? 1 : 2;
		if (SideMask == 3) break;

		iNode = (SideMask & 1) ? m_pNodes[iNode].m_iNodeFront : m_pNodes[iNode].m_iNodeBack;
	}
	while(1);

	return iNode;
}

int CXR_Model_BSP3::GetFirstSplitNode_Polyhedron(const CVec3Dfp32* _pV, int _nVertices)
{
	MAUTOSTRIP(CXR_Model_BSP3_GetFirstSplitNode_Polyhedron, 0);
	int iNode = 1;
	do
	{
		int iPlane = m_pNodes[iNode].m_iPlane;
		if (!iPlane) return iNode;
		CPlane3Dfp32* pP = &m_pPlanes[iPlane];

		int SideMask = 0;
		for(int v = 0; v < _nVertices; v++)
		{
			fp32 d = pP->Distance(_pV[v]);
			if (d > 0.001f)
				SideMask |= 1;
			else if (d < -0.001f)
				SideMask |= 2;
//			SideMask |= (pP->Distance(_pV[v]) > 0.0f) ? 1 : 2;
			if (SideMask == 3) return iNode;
		}

		iNode = (SideMask & 1) ? m_pNodes[iNode].m_iNodeFront : m_pNodes[iNode].m_iNodeBack;
	}
	while(1);
}

int CXR_Model_BSP3::Structure_GetFirstSplitNode_Polyhedron(const CVec3Dfp32* _pV, int _nVertices)
{
	MAUTOSTRIP(CXR_Model_BSP3_Structure_GetFirstSplitNode_Polyhedron, 0);
	int iNode = 1;
	do
	{
		CBSP3_Node* pN = &m_pNodes[iNode];
		if ((pN->m_Flags & (XW_NODE_PORTAL | XW_NODE_STRUCTURE)) == (XW_NODE_PORTAL | XW_NODE_STRUCTURE)) return iNode;
		int iPlane = m_pNodes[iNode].m_iPlane;
		if (!iPlane) return iNode;
		CPlane3Dfp32* pP = &m_pPlanes[iPlane];

		int SideMask = 0;
		for(int v = 0; v < _nVertices; v++)
		{
			fp32 d = pP->Distance(_pV[v]);
			if (d > 0.001f)
				SideMask |= 1;
			else if (d < -0.001f)
				SideMask |= 2;
			if (SideMask == 3) return iNode;
		}

		iNode = (SideMask & 1) ? m_pNodes[iNode].m_iNodeFront : m_pNodes[iNode].m_iNodeBack;
	}
	while(1);
}

void CXR_Model_BSP3::EnableTreeFromNode_Sphere(int _iNode, const CVec3Dfp32& _Pos, fp32 _Radius)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnableTreeFromNode_Sphere, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	m_pNodes[_iNode].m_Flags |= XW_NODE_ENABLE;

	int iPlane = m_pNodes[_iNode].m_iPlane;
	if (!iPlane) return;
	CPlane3Dfp32* pP = &m_pPlanes[iPlane];
	fp32 d = pP->Distance(_Pos);
	if (d > -_Radius) 
		EnableTreeFromNode_Sphere(m_pNodes[_iNode].m_iNodeFront, _Pos, _Radius);
	if (d < _Radius) 
		EnableTreeFromNode_Sphere(m_pNodes[_iNode].m_iNodeBack, _Pos, _Radius);
}

void CXR_Model_BSP3::EnableTreeFromNode_Box(int _iNode, const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnableTreeFromNode_Box, MAUTOSTRIP_VOID);
	Error("EnableTreeFromNode_Box", "FIXME! I'm broken!");

	if (!_iNode) return;
	m_pNodes[_iNode].m_Flags |= XW_NODE_ENABLE;

	int iPlane = m_pNodes[_iNode].m_iPlane;
	if (!iPlane) return;
	CPlane3Dfp32* pP = &m_pPlanes[iPlane];

	fp32 d = -pP->d;

	fp32 xp0 = pP->n.k[0] * _BoxMin.k[0];
	fp32 xp1 = pP->n.k[0] * _BoxMax.k[0];
	fp32 yp0 = pP->n.k[1] * _BoxMin.k[1];
	fp32 yp1 = pP->n.k[1] * _BoxMax.k[1];
	fp32 zp0 = pP->n.k[2] * _BoxMin.k[2];
	fp32 zp1 = pP->n.k[2] * _BoxMax.k[2];

	int SideMask = 0;
	do
	{
		SideMask |= (xp0 + yp0 + zp0 > d) ? 1 : 2;
		SideMask |= (xp0 + yp0 + zp1 > d) ? 1 : 2;
		if (SideMask == 3) continue;
		SideMask |= (xp0 + yp1 + zp0 > d) ? 1 : 2;
		if (SideMask == 3) continue;
		SideMask |= (xp0 + yp1 + zp1 > d) ? 1 : 2;
		if (SideMask == 3) continue;

		SideMask |= (xp1 + yp0 + zp0 > d) ? 1 : 2;
		if (SideMask == 3) continue;
		SideMask |= (xp1 + yp0 + zp1 > d) ? 1 : 2;
		if (SideMask == 3) continue;
		SideMask |= (xp1 + yp1 + zp0 > d) ? 1 : 2;
		if (SideMask == 3) continue;
		SideMask |= (xp1 + yp1 + zp1 > d) ? 1 : 2;
		if (SideMask == 3) continue;
	}
	while(0);

	if (SideMask & 1) EnableTreeFromNode_Box(m_pNodes[_iNode].m_iNodeFront, _BoxMin, _BoxMax);
	if (SideMask & 2) EnableTreeFromNode_Box(m_pNodes[_iNode].m_iNodeBack, _BoxMin, _BoxMax);
}

void CXR_Model_BSP3::EnableTreeFromNode_Polyhedron(int _iNode, const CVec3Dfp32* _pV, int _nVertices)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnableTreeFromNode_Polyhedron, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	m_pNodes[_iNode].m_Flags |= XW_NODE_ENABLE;

	int iPlane = m_pNodes[_iNode].m_iPlane;
	if (!iPlane) return;
	CPlane3Dfp32* pP = &m_pPlanes[iPlane];

	int SideMask = 0;
	for(int v = 0; v < _nVertices; v++)
	{
		fp32 d = pP->Distance(_pV[v]);
		if (d > 0.001f)
			SideMask |= 1;
		else if (d < -0.001f)
			SideMask |= 2;
//		SideMask |= (pP->Distance(_pV[v]) > 0.0f) ? 1 : 2;
		if (SideMask == 3) break;
	}

	if (SideMask & 1) EnableTreeFromNode_Polyhedron(m_pNodes[_iNode].m_iNodeFront, _pV, _nVertices);
	if (SideMask & 2) EnableTreeFromNode_Polyhedron(m_pNodes[_iNode].m_iNodeBack, _pV, _nVertices);
}

void CXR_Model_BSP3::EnableTreeFromNode(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnableTreeFromNode, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::EnableTreeFromNode);
	if (!_iNode) return;
	while ((_iNode) && !(m_pNodes[_iNode].m_Flags & XW_NODE_ENABLE))
	{
		m_pNodes[_iNode].m_Flags |= XW_NODE_ENABLE;
		_iNode = m_pNodes[_iNode].m_iNodeParent;
	}
}

void CXR_Model_BSP3::EnableTreeFlagFromNode(int _iNode, int _Flag)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnableTreeFlagFromNode, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	while ((_iNode) && !(m_pNodes[_iNode].m_Flags & _Flag))
	{
		m_pNodes[_iNode].m_Flags |= _Flag;
		_iNode = m_pNodes[_iNode].m_iNodeParent;
	}
}

void CXR_Model_BSP3::EnableWithPortalFloodFill(int _iNode, int _EnableFlag, int _MediumFlag)
{
	MAUTOSTRIP(CXR_Model_BSP3_EnableWithPortalFloodFill, MAUTOSTRIP_VOID);
	CBSP3_Node* pN = &m_pNodes[_iNode];
	if (pN->m_Flags & _EnableFlag) return;

	if (!pN->IsStructureLeaf())
	{
		ConOut(CStrF("(CXR_Model_BSP3::EnableWithPortalFloodFill) Node %d was not a structure-leaf.", _iNode));
		return;
	}
	const CBSP3_PortalLeaf* pPL = &m_lPortalLeaves[pN->m_iPortalLeaf];
	if (!(pPL->m_ContainsMedium & _MediumFlag)) return;

	pN->m_Flags |= _EnableFlag;

	for(int p = 0; p < pPL->m_nPortals; p++)
	{
		int iP = m_liPortals[pPL->m_iiPortals + p];
		const CBSP3_Portal* pP = &m_pPortals[iP];
		EnableWithPortalFloodFill((pP->m_iNodeFront == _iNode) ? pP->m_iNodeBack : pP->m_iNodeFront, _EnableFlag, _MediumFlag);
	}
}

void CXR_Model_BSP3::DisableWithPortalFloodFill(int _iNode, int _EnableFlag)
{
	MAUTOSTRIP(CXR_Model_BSP3_DisableWithPortalFloodFill, MAUTOSTRIP_VOID);
	CBSP3_Node* pN = &m_pNodes[_iNode];
	if (!(pN->m_Flags & _EnableFlag)) return;

	if (!pN->IsStructureLeaf())
	{
		ConOut(CStrF("(CXR_Model_BSP3::EnableWithPortalFloodFill) Node %d was not a structure-leaf.", _iNode));
		return;
	}
	const CBSP3_PortalLeaf* pPL = &m_lPortalLeaves[pN->m_iPortalLeaf];
	pN->m_Flags &= ~_EnableFlag;

	for(int p = 0; p < pPL->m_nPortals; p++)
	{
		int iP = m_liPortals[pPL->m_iiPortals + p];
		const CBSP3_Portal* pP = &m_pPortals[iP];
		DisableWithPortalFloodFill((pP->m_iNodeFront == _iNode) ? pP->m_iNodeBack : pP->m_iNodeFront, _EnableFlag);
	}
}

void CXR_Model_BSP3::Structure_EnableTreeFromNode_Polyhedron(int _iNode, const CVec3Dfp32* _pV, int _nVertices)
{
	MAUTOSTRIP(CXR_Model_BSP3_Structure_EnableTreeFromNode_Polyhedron, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	CBSP3_Node* pN = &m_pNodes[_iNode];
	pN->m_Flags |= XW_NODE_ENABLE;

	if ((pN->m_Flags & (XW_NODE_PORTAL | XW_NODE_STRUCTURE)) == (XW_NODE_PORTAL | XW_NODE_STRUCTURE)) return;
	int iPlane = m_pNodes[_iNode].m_iPlane;
	if (!iPlane) return;
	CPlane3Dfp32* pP = &m_pPlanes[iPlane];

	int SideMask = 0;
	for(int v = 0; v < _nVertices; v++)
	{
		fp32 d = pP->Distance(_pV[v]);
		if (d > 0.001f)
			SideMask |= 1;
		else if (d < -0.001f)
			SideMask |= 2;
		if (SideMask == 3) break;
	}

	if (SideMask & 1) Structure_EnableTreeFromNode_Polyhedron(pN->m_iNodeFront, _pV, _nVertices);
	if (SideMask & 2) Structure_EnableTreeFromNode_Polyhedron(pN->m_iNodeBack, _pV, _nVertices);
}

void CXR_Model_BSP3::InitBoundNodesFromEnabledNodes(int _iNode, int _iNodeLastSplit, int _iNodeLastSplitSide)
{
	MAUTOSTRIP(CXR_Model_BSP3_InitBoundNodesFromEnabledNodes, MAUTOSTRIP_VOID);
	if (m_pNodes[_iNode].IsLeaf())
	{
		if (_iNodeLastSplit)
		{
			if (_iNodeLastSplitSide == 1)
				m_pNodes[_iNodeLastSplit].m_Bound.m_iBoundNodeFront = _iNode;
			else
				m_pNodes[_iNodeLastSplit].m_Bound.m_iBoundNodeBack = _iNode;
		}
		return;
	}

	int iFront = m_pNodes[_iNode].m_iNodeFront;
	int iBack = m_pNodes[_iNode].m_iNodeBack;

	if ((iFront) && !(m_pNodes[iFront].m_Flags & XW_NODE_ENABLE))
	{
		InitBoundNodesFromEnabledNodes(iBack, _iNodeLastSplit, _iNodeLastSplitSide);
	}
	else if ((iBack) && !(m_pNodes[iBack].m_Flags & XW_NODE_ENABLE))
	{
		InitBoundNodesFromEnabledNodes(iFront, _iNodeLastSplit, _iNodeLastSplitSide);
	}
	else
	{
		if (_iNodeLastSplit)
		{
			if (_iNodeLastSplitSide == 1)
				m_pNodes[_iNodeLastSplit].m_Bound.m_iBoundNodeFront = _iNode;
			else
				m_pNodes[_iNodeLastSplit].m_Bound.m_iBoundNodeBack = _iNode;
		}

		InitBoundNodesFromEnabledNodes(iFront, _iNode, 1);
		InitBoundNodesFromEnabledNodes(iBack, _iNode, 2);
	}
}

void CXR_Model_BSP3::Structure_InitBoundNodesFromEnabledNodes(int _iNode, int _iNodeLastSplit, int _iNodeLastSplitSide)
{
	MAUTOSTRIP(CXR_Model_BSP3_Structure_InitBoundNodesFromEnabledNodes, MAUTOSTRIP_VOID);
	CBSP3_Node* pN = &m_pNodes[_iNode];

//	if (!(pN->m_Flags & XW_NODE_STRUCTURE)) return;
	if (!_iNode || ((pN->m_Flags & (XW_NODE_PORTAL | XW_NODE_STRUCTURE)) == (XW_NODE_PORTAL | XW_NODE_STRUCTURE)))
	{
		if (_iNodeLastSplit)
		{
			if (_iNodeLastSplitSide == 1)
				m_pNodes[_iNodeLastSplit].m_Bound.m_iBoundNodeFront = _iNode;
			else
				m_pNodes[_iNodeLastSplit].m_Bound.m_iBoundNodeBack = _iNode;
		}
		return;
	}

	int iFront = pN->m_iNodeFront;
	int iBack = pN->m_iNodeBack;

	if ((iFront) && !(m_pNodes[iFront].m_Flags & XW_NODE_ENABLE))
	{
		if (!iBack) return;
		Structure_InitBoundNodesFromEnabledNodes(iBack, _iNodeLastSplit, _iNodeLastSplitSide);
	}
	else if ((iBack) && !(m_pNodes[iBack].m_Flags & XW_NODE_ENABLE))
	{
		Structure_InitBoundNodesFromEnabledNodes(iFront, _iNodeLastSplit, _iNodeLastSplitSide);
	}
	else
	{
		if (_iNodeLastSplit)
		{
			if (_iNodeLastSplitSide == 1)
				m_pNodes[_iNodeLastSplit].m_Bound.m_iBoundNodeFront = _iNode;
			else
				m_pNodes[_iNodeLastSplit].m_Bound.m_iBoundNodeBack = _iNode;
		}

		Structure_InitBoundNodesFromEnabledNodes(iFront, _iNode, 1);
		Structure_InitBoundNodesFromEnabledNodes(iBack, _iNode, 2);
	}
}

void CXR_Model_BSP3::DisableTree(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP3_DisableTree, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::DisableTree);
	if (!_iNode) return;
	if (m_pNodes[_iNode].m_Flags & XW_NODE_ENABLE)
	{
		m_pNodes[_iNode].m_Flags &= (~XW_NODE_ENABLE);
		if (m_pNodes[_iNode].m_iPlane)
		{
			DisableTree(m_pNodes[_iNode].m_iNodeFront);
			DisableTree(m_pNodes[_iNode].m_iNodeBack);
		}
	}
}

void CXR_Model_BSP3::DisableTreeFlag(int _iNode, int _Flag)
{
	MAUTOSTRIP(CXR_Model_BSP3_DisableTreeFlag, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP3::DisableTreeFlag);
	if (!_iNode) return;
	if (m_pNodes[_iNode].m_Flags & _Flag)
	{
		m_pNodes[_iNode].m_Flags &= (~_Flag);
		if (m_pNodes[_iNode].m_iPlane)
		{
			DisableTreeFlag(m_pNodes[_iNode].m_iNodeFront, _Flag);
			DisableTreeFlag(m_pNodes[_iNode].m_iNodeBack, _Flag);
		}
	}
}
/*
bool CXR_Model_BSP3::CheckTree_r(int _iNode, int _iParent, int _PortalDone)
{
	MAUTOSTRIP(CXR_Model_BSP3_CheckTree_r, false);
	MSCOPESHORT(CXR_Model_BSP3::CheckTree_r); //AR-SCOPE

	if (!_iNode) return true;
	CBSP3_Node* pN = &m_lNodes[_iNode];

	pN->m_Flags |= XW_NODE_ENABLE;

	if (pN->m_iNodeParent != _iParent)
	{
		LogFile(CStrF("(CheckTree) Node %d, parent-node missmatch, parent %d", _iNode, pN->m_iNodeParent));
		if (_iParent)
			LogFile(CStrF("(CheckTree) Node %d, ChildF %d, ChildB", _iParent, m_lNodes[_iParent].m_iNodeFront, m_lNodes[_iParent].m_iNodeBack));
		return false;
	}

	if (pN->m_Flags & XW_NODE_PORTAL)
	{
		if (pN->m_Flags & XW_NODE_NAVIGATION)
		{
//LogFile(CStrF("Node %d, Nav.Leaf %d, Medium %.8x", _iNode, pN->m_iNavigationLeaf, m_lPortalLeaves[pN->m_iNavigationLeaf].m_ContainsMedium));

			if (_PortalDone & 2)
				LogFile(CStrF("(CheckTree) More than one navigation-leaf in branch. (iNode %d)", _iNode));
			_PortalDone |= 2;
		}

		if (pN->m_Flags & XW_NODE_STRUCTURE)
		{
//	LogFile(CStrF("Node %d, Struc.Leaf %d", _iNode, pN->m_iPortalLeaf));
			if (_PortalDone & 1)
				LogFile(CStrF("(CheckTree) More than one structure-leaf in branch. (iNode %d)", _iNode));
			_PortalDone |= 1;
		}
	}

	if (pN->IsLeaf())
	{
//		if (pN->m_Medium & XW_MEDIUM_SKY)
//				LogFile("CheckTree_r: SKY-Medium.");

		if (!(_PortalDone & 1))
			LogFile(CStrF("(CheckTree) No structure-leaf in branch. (iNode %d)", _iNode));
		if (!(_PortalDone & 1))
			LogFile(CStrF("(CheckTree) No navigation-leaf in branch. (iNode %d)", _iNode));
pN->m_Flags &= ~XW_NODE_ENABLE;
		return true;
	}
	else
	{
//if (_iNode == 168)
//LogFile(CStrF("Prnt %d, N %d, ChildF %d ChildB %d", pN->m_iNodeParent, _iNode, pN->m_iNodeFront, pN->m_iNodeBack));
		bool bOk = true;
		if (pN->m_iNodeFront)
			if (m_lNodes[pN->m_iNodeFront].m_Flags & XW_NODE_ENABLE)
			{
				LogFile(CStrF("(CheckTree) Node %d front-links to ancestor node %d", _iNode, pN->m_iNodeFront));
				bOk = false;
			}
		if (pN->m_iNodeBack)
			if (m_lNodes[pN->m_iNodeBack].m_Flags & XW_NODE_ENABLE)
			{
				LogFile(CStrF("(CheckTree) Node %d Back-links to ancestor node %d", _iNode, pN->m_iNodeBack));
				bOk = false;
			}

		bOk &= CheckTree_r(pN->m_iNodeFront, _iNode, _PortalDone);
		bOk &= CheckTree_r(pN->m_iNodeBack, _iNode, _PortalDone);
pN->m_Flags &= ~XW_NODE_ENABLE;
		return bOk;
	}

pN->m_Flags &= ~XW_NODE_ENABLE;
}
*/
// -------------------------------------------------------------------
int CXR_Model_BSP3::GetLeaf(const CVec3Dfp32& _v) const
{
	MAUTOSTRIP(CXR_Model_BSP3_GetLeaf, 0);
	int iNode = 1;
	while(m_lNodes[iNode].IsNode())
	{
		int iPlane = m_lNodes[iNode].m_iPlane;
		int VSide = m_lPlanes[iPlane].GetPlaneSide(_v);
		iNode = (VSide >= 0) ? m_lNodes[iNode].m_iNodeFront : m_lNodes[iNode].m_iNodeBack;
	}
	return iNode;
}

int CXR_Model_BSP3::GetPortalLeaf(const CVec3Dfp32& _v) const
{
	MAUTOSTRIP(CXR_Model_BSP3_GetPortalLeaf, 0);
	MSCOPESHORT(CXR_Model_BSP3::GetPortalLeaf);
	int iNode = 1;
	while(!(m_lNodes[iNode].m_Flags & XW_NODE_PORTAL))
	{
		if (m_lNodes[iNode].IsLeaf()) return -1;
		int iPlane = m_lNodes[iNode].m_iPlane;
		int VSide = m_lPlanes[iPlane].GetPlaneSide(_v);
		iNode = (VSide >= 0) ? m_lNodes[iNode].m_iNodeFront : m_lNodes[iNode].m_iNodeBack;
	}
	return iNode;
}

int CXR_Model_BSP3::GetStructurePortalLeaf(const CVec3Dfp32& _v) const
{
	MAUTOSTRIP(CXR_Model_BSP3_GetStructurePortalLeaf, 0);
	MSCOPESHORT(CXR_Model_BSP3::GetStructurePortalLeaf);
	int iNode = 1;
	while(iNode)
	{
		const CBSP3_Node* pN = &m_lNodes[iNode];
		if (pN->IsStructureLeaf()) return pN->m_iPortalLeaf;
		if (pN->IsLeaf()) return -1;
		int iPlane = pN->m_iPlane;
		int VSide = m_lPlanes[iPlane].GetPlaneSide(_v);
		iNode = (VSide >= 0) ? pN->m_iNodeFront : pN->m_iNodeBack;
	}
	return -1;
}

/*int CXR_Model_BSP3::GetNavigationPortalLeaf(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CXR_Model_BSP3_GetNavigationPortalLeaf, NULL);
	int iNode = 1;
	while(iNode)
	{
		const CBSP3_Node* pN = &m_lNodes[iNode];
		if (pN->IsNavigationLeaf()) return pN->m_iNavigationLeaf;
		if (pN->IsLeaf()) return -1;
		int iPlane = pN->m_iPlane;
		int VSide = m_lPlanes[iPlane].GetPlaneSide(_v);
		iNode = (VSide >= 0) ? pN->m_iNodeFront : pN->m_iNodeBack;
	}
	return -1;
}*/

CStr CXR_Model_BSP3::GetInfo()
{
	MAUTOSTRIP(CXR_Model_BSP3_GetInfo, CStr());
#ifdef M_Profile
	return CStr(TString("All: ", ms_Time) + TString(",  Leaves: ", ms_TimeRenderLeafList-ms_TimeRenderPoly1) + TString(",  Queue: ", ms_TimeRenderFaceQueue-ms_TimeRenderPoly2));
#else
	return "Profiling turned off";
#endif
}

void CXR_Model_BSP3::InitializeListPtrs()
{
	MAUTOSTRIP(CXR_Model_BSP3_InitializeListPtrs, MAUTOSTRIP_VOID);
	m_pPlanes = (m_lPlanes.Len()) ? &m_lPlanes[0] : NULL;
	m_pVertices = (m_lVertices.Len()) ? &m_lVertices[0] : NULL;
	m_piVertices = (m_liVertices.Len()) ? &m_liVertices[0] : NULL;
	m_pFaces = (m_lFaces.Len()) ? &m_lFaces[0] : NULL;
	m_piFaces = (m_liFaces.Len()) ? &m_liFaces[0] : NULL;
	m_pNodes = (m_lNodes.Len()) ? &m_lNodes[0] : NULL;
//	m_pLeaves = (m_lLeaves.Len()) ? &m_lLeaves[0] : NULL;
//	m_pVVertices = (m_lVVertices.Len()) ? &m_lVVertices[0] : NULL;
	m_pVVertMask = (m_lVVertMask.Len()) ? &m_lVVertMask[0] : NULL;
	m_piVVertTag = m_liVVertTag.GetBasePtr();
	m_nFogTags = 0;

	m_piVVertTag = m_liVVertTag.GetBasePtr();
	m_pLightVerticesInfo = (m_lLightVerticesInfo.Len()) ? &m_lLightVerticesInfo[0] : NULL;
	m_pLightVertices = (m_lLightVertices.Len()) ? &m_lLightVertices[0] : NULL;
//	if (m_spLMMat!=NULL) m_pLMInfo = m_spLMMat->m_lLMInfo.GetBasePtr();
//	m_pVisLists = (m_lVisLists.Len()) ? &m_lVisLists[0] : NULL;
//	m_pVisData = (m_lVisData.Len()) ? &m_lVisData[0] : NULL;
	m_pPortalLeaves = m_lPortalLeaves.GetBasePtr();
	m_pPortals = (m_lPortals.Len()) ? &m_lPortals[0] : NULL;
	m_piPortals = (m_liPortals.Len()) ? &m_liPortals[0] : NULL;

	// Initialize face queue
/*	m_iFaceQueue = 0;
	m_FaceQueueLen = Min(6144, m_lFaces.Len());
	m_liFaceQueue.SetLen(m_FaceQueueLen);*/
//	m_liLMFaces.SetLen(m_FaceQueueLen);
//	m_liLVFaces.SetLen(m_FaceQueueLen);
//	m_piFaceQueue = &m_liFaceQueue[0];
}

