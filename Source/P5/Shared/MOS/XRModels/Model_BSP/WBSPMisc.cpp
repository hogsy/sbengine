#include "PCH.h"

#include "WBSPModel.h"
#include "WBSPDef.h"
#include "MFloat.h"


#ifdef PLATFORM_PS2
#include "eeregs.h"
#endif


#define FACECUT2_EPSILON 0.01f


int BSPModel_CutFace2(CVec3Dfp32* _pVerts, int _nv, const CPlane3Dfp32* _pPlanes, int _np, 
	int _bInvertPlanes, CVec2Dfp32* _pTVerts1, CVec2Dfp32* _pTVerts2, CVec2Dfp32* _pTVerts3)
{
	MAUTOSTRIP(BSPModel_CutFace2, 0);

	if (!_nv) return 0;
	const int MaxVClip = 32;

	int bClipTVerts1 = (_pTVerts1 != NULL);
	int bClipTVerts2 = (_pTVerts2 != NULL);
	int bClipTVerts3 = (_pTVerts3 != NULL);

	CVec3Dfp32 VClip[MaxVClip];
	CVec2Dfp32 TVClip1[MaxVClip];
	CVec2Dfp32 TVClip2[MaxVClip];
	CVec2Dfp32 TVClip3[MaxVClip];

	CVec3Dfp32* pVDest = &VClip[0];
	CVec2Dfp32* pTVDest1 = &TVClip1[0];
	CVec2Dfp32* pTVDest2 = &TVClip2[0];
	CVec2Dfp32* pTVDest3 = &TVClip3[0];
	CVec3Dfp32* pVSrc = _pVerts;
	CVec2Dfp32* pTVSrc1 = _pTVerts1;
	CVec2Dfp32* pTVSrc2 = _pTVerts2;
	CVec2Dfp32* pTVSrc3 = _pTVerts3;

	for(int iPlane = 0; iPlane < _np; iPlane++)
	{
		const CPlane3Dfp32* pP = &_pPlanes[iPlane];
		fp32 VertPDist[32];
//		fp32 d = -((pP->p.k[0] * pP->n.k[0]) + (pP->p.k[1] * pP->n.k[1]) + (pP->p.k[2] * pP->n.k[2]));
//		fp32 d = pP->d;
		bool bBehind = false;
		bool bFront = false;

		// Calc point-2-plane distance for all vertices.
		for(int v = 0; v < _nv; v++)
		{
			VertPDist[v] = pP->Distance(pVSrc[v]);
			if (_bInvertPlanes) VertPDist[v] = -VertPDist[v];
			if (VertPDist[v] < 0.0f) bBehind = true; else bFront = true;
		}

		// If all points are on one side, return either all or none.
		if (!(bFront && bBehind))
		{
			if (bFront) continue;
			return 0;
		}

		int nClip = 0;
		{
			int v = _nv-1;
			for (int v2 = 0; v2 < _nv; v2++)
			{
				if (VertPDist[v] > -FACECUT2_EPSILON)
				{
					pVDest[nClip] = pVSrc[v];
					if (bClipTVerts1) pTVDest1[nClip] = pTVSrc1[v];
					if (bClipTVerts2) pTVDest2[nClip] = pTVSrc2[v];
					if (bClipTVerts3) pTVDest3[nClip] = pTVSrc3[v];
					nClip++;

					if ((VertPDist[v2] < -FACECUT2_EPSILON) && (VertPDist[v] > FACECUT2_EPSILON))
					{
						fp32 dvx = (pVSrc[v2].k[0] - pVSrc[v].k[0]);
						fp32 dvy = (pVSrc[v2].k[1] - pVSrc[v].k[1]);
						fp32 dvz = (pVSrc[v2].k[2] - pVSrc[v].k[2]);
						fp32 s = dvx*pP->n.k[0] + dvy*pP->n.k[1] + dvz*pP->n.k[2];
						if (_bInvertPlanes) s = -s;
						if (s)
						{
							fp32 sp = VertPDist[v];
							fp32 t = -sp/s;
							pVDest[nClip].k[0] = pVSrc[v].k[0] + dvx * t;
							pVDest[nClip].k[1] = pVSrc[v].k[1] + dvy * t;
							pVDest[nClip].k[2] = pVSrc[v].k[2] + dvz * t;

							if (bClipTVerts1)
							{
								fp32 dtvx = (pTVSrc1[v2].k[0] - pTVSrc1[v].k[0]);
								fp32 dtvy = (pTVSrc1[v2].k[1] - pTVSrc1[v].k[1]);
								pTVDest1[nClip].k[0] = pTVSrc1[v].k[0] + dtvx * t;
								pTVDest1[nClip].k[1] = pTVSrc1[v].k[1] + dtvy * t;
							}
							if (bClipTVerts2)
							{
								fp32 dtvx = (pTVSrc2[v2].k[0] - pTVSrc2[v].k[0]);
								fp32 dtvy = (pTVSrc2[v2].k[1] - pTVSrc2[v].k[1]);
								pTVDest2[nClip].k[0] = pTVSrc2[v].k[0] + dtvx * t;
								pTVDest2[nClip].k[1] = pTVSrc2[v].k[1] + dtvy * t;
							}
							if (bClipTVerts3)
							{
								fp32 dtvx = (pTVSrc3[v2].k[0] - pTVSrc3[v].k[0]);
								fp32 dtvy = (pTVSrc3[v2].k[1] - pTVSrc3[v].k[1]);
								pTVDest3[nClip].k[0] = pTVSrc3[v].k[0] + dtvx * t;
								pTVDest3[nClip].k[1] = pTVSrc3[v].k[1] + dtvy * t;
							}
							nClip++;
						}
					}
				}
				else
				{
					if (VertPDist[v2] > FACECUT2_EPSILON)
					{
						fp32 dvx = (pVSrc[v2].k[0] - pVSrc[v].k[0]);
						fp32 dvy = (pVSrc[v2].k[1] - pVSrc[v].k[1]);
						fp32 dvz = (pVSrc[v2].k[2] - pVSrc[v].k[2]);
						fp32 s = dvx*pP->n.k[0] + dvy*pP->n.k[1] + dvz*pP->n.k[2];
						if (_bInvertPlanes) s = -s;
						if (s)
						{
							fp32 sp = VertPDist[v];
							fp32 t = -sp/s;
							pVDest[nClip].k[0] = pVSrc[v].k[0] + dvx * t;
							pVDest[nClip].k[1] = pVSrc[v].k[1] + dvy * t;
							pVDest[nClip].k[2] = pVSrc[v].k[2] + dvz * t;

							if (bClipTVerts1)
							{
								fp32 dtvx = (pTVSrc1[v2].k[0] - pTVSrc1[v].k[0]);
								fp32 dtvy = (pTVSrc1[v2].k[1] - pTVSrc1[v].k[1]);
								pTVDest1[nClip].k[0] = pTVSrc1[v].k[0] + dtvx * t;
								pTVDest1[nClip].k[1] = pTVSrc1[v].k[1] + dtvy * t;
							}
							if (bClipTVerts2)
							{
								fp32 dtvx = (pTVSrc2[v2].k[0] - pTVSrc2[v].k[0]);
								fp32 dtvy = (pTVSrc2[v2].k[1] - pTVSrc2[v].k[1]);
								pTVDest2[nClip].k[0] = pTVSrc2[v].k[0] + dtvx * t;
								pTVDest2[nClip].k[1] = pTVSrc2[v].k[1] + dtvy * t;
							}
							if (bClipTVerts3)
							{
								fp32 dtvx = (pTVSrc3[v2].k[0] - pTVSrc3[v].k[0]);
								fp32 dtvy = (pTVSrc3[v2].k[1] - pTVSrc3[v].k[1]);
								pTVDest3[nClip].k[0] = pTVSrc3[v].k[0] + dtvx * t;
								pTVDest3[nClip].k[1] = pTVSrc3[v].k[1] + dtvy * t;
							}
							nClip++;
						}
					}
				}

				if (nClip > MaxVClip-1) Error_static("CutFace", "Too many vertices.");
				v = v2;
			}
		}

		if (!nClip) return 0;
		_nv = nClip;

		Swap(pVSrc, pVDest);
		Swap(pTVSrc1, pTVDest1);
		Swap(pTVSrc2, pTVDest2);
		Swap(pTVSrc3, pTVDest3);
	}

	// Move if the latest vertices are in the wrong array.
	if (pVSrc != _pVerts) 
	{
		memcpy(_pVerts, pVSrc, _nv*sizeof(CVec3Dfp32));
		if (bClipTVerts1) memcpy(_pTVerts1, pTVSrc1, _nv*sizeof(CVec2Dfp32));
		if (bClipTVerts2) memcpy(_pTVerts2, pTVSrc2, _nv*sizeof(CVec2Dfp32));
		if (bClipTVerts3) memcpy(_pTVerts3, pTVSrc3, _nv*sizeof(CVec2Dfp32));
	}
	return _nv;
}


// -------------------------------------------------------------------
//  CBSP_LinkContext
// -------------------------------------------------------------------
#define LINKBOX_EPSILON 0.001f

CBSP_LinkContext::CBSP_LinkContext()
{
	MAUTOSTRIP(CBSP_LinkContext_ctor, MAUTOSTRIP_VOID);
	m_pLinks = NULL;
	m_pModel = NULL;
	m_pIDLinkMap = NULL;
	m_pPLLinkMap = NULL;
}

void CBSP_LinkContext::Create(CXR_Model_BSP* _pModel, int _NumIDs, int _NumLinks)
{
	MAUTOSTRIP(CBSP_LinkContext_Create, MAUTOSTRIP_VOID);
	MSCOPESHORT(CBSP_LinkContext::Create);
	m_pModel = _pModel;
	if (_NumIDs > 65535) Error("Create", "Can only handle 64k IDs.");
	m_lIDLinkMap.SetLen(_NumIDs);
	m_lIDLinkBox.SetLen(_NumIDs);
	m_lIDFlags.SetLen(_NumIDs);
	FillChar(m_lIDLinkMap.GetBasePtr(), m_lIDLinkMap.ListSize(), 0);
	FillChar(m_lIDFlags.GetBasePtr(), m_lIDFlags.ListSize(), 0);
	m_lPLLinkMap.SetLen(m_pModel->m_nStructureLeaves+1);
	m_iPLInfinite = m_pModel->m_nStructureLeaves;
	FillChar(m_lPLLinkMap.GetBasePtr(), m_lPLLinkMap.ListSize(), 0);

	_NumLinks = (_NumLinks + 31) & ~31;
	m_lLinks.SetLen(_NumLinks);
	m_LinkHeap.Create(_NumLinks, FALSE);	// Non-dynamic heap
	m_LinkHeap.AllocID();					// Reserve ID 0.
	m_pLinks = m_lLinks.GetBasePtr();

	m_pIDLinkMap = m_lIDLinkMap.GetBasePtr();
	m_pPLLinkMap = m_lPLLinkMap.GetBasePtr();
}

void CBSP_LinkContext::Insert(int _ID, const CBox3Dfp32& _Box, int _Flags)
{
	MAUTOSTRIP(CBSP_LinkContext_Insert, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP_LinkContext::Insert");

/*	int iFirstLink = m_lIDLinkMap[_ID];
	if (iFirstLink)
	{
		const CBSP_Link* pL = &m_pLinks[iFirstLink];
		if (!pL->m_iLinkNextPL)
		{
			// This ID is only linked to a sole PL, we can then traverse the BSP from the PL up 
			// to the root and see if the _Box still within the PL volume.

			const CBSP_Node* pNodes = m_pModel->m_lNodes.GetBasePtr();
			const CPlane3Dfp32* pPlanes = m_pModel->m_lPlanes.GetBasePtr();

			int nSplits = 0;
			int iNode = m_pModel->m_lPortalLeaves[pL->m_iPortalLeaf].m_iNode;
			while(pNodes[iNode].m_iNodeParent)
			{
				const CBSP_Node* pN = &pNodes[iNode];
				int iParent = pN->m_iNodeParent;
				const CBSP_Node* pNP = &pNodes[iParent];
				if (pNP->m_iNodeFront == iNode)
				{
					if (pPlanes[pNP->m_iPlane].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) < -LINKBOX_EPSILON) break;
				}
				else if (pNP->m_iNodeBack == iNode)
				{
					if (pPlanes[pNP->m_iPlane].GetBoxMaxDistance(_Box.m_Min, _Box.m_Max) > LINKBOX_EPSILON) break;
				}
				else
					Error("Insert", CStrF("Broken BSP-tree at node %d, children %d, %d, proposed child %d", iParent, pNP->m_iNodeFront, pNP->m_iNodeBack, iNode));


				nSplits++;
				iNode = iParent;
			}

			if (!pNodes[iNode].m_iNodeParent)
			{
//				ConOut(CStrF("No insert. (depth %d), ", nSplits) + _Box.GetString() );
				return;
			}
//			else
//				ConOut(CStrF("Must insert. (depth %d), ", nSplits) + _Box.GetString() );
		}
	}
*/
	Remove(_ID);

	M_ASSERT(!m_lIDLinkMap[_ID], "!");

	m_iNextLink = 0;
	m_iPrevLink = 0;

/*	if (!iLink)
	{
		// Create new link.
		iLink= m_LinkHeap.AllocID();
		if (iLink < 0) Error("Insert", "Unable to allocate link.");

		m_pLinks[iLink].m_iPortalLeaf = 0;
		m_pLinks[iLink].m_iLinkPrevObject = 0;
		m_pLinks[iLink].m_iLinkNextObject = 0;
		m_pLinks[iLink].m_iLinkPrevPL = 0;
		m_pLinks[iLink].m_iLinkNextPL = 0;
	}*/


/*	m_iNextLink = m_lIDLinkMap[_ID];
	if (!m_iNextLink)
	{
		int iLink = m_lIDLinkMap[_ID] = m_LinkHeap.AllocID();
		if (iLink < 0) Error("Insert", "Unable to allocate link.");

		m_pLinks[iLink].m_iPortalLeaf = 0;
		m_pLinks[iLink].m_ID = _ID;
		m_pLinks[iLink].m_iLinkPrevObject = 0;
		m_pLinks[iLink].m_iLinkNextObject = 0;
		m_pLinks[iLink].m_iLinkPrevPL = 0;
		m_pLinks[iLink].m_iLinkNextPL = 0;
		m_iNextLink = iLink;
	}*/
//if (bCheck && pByte[0x04474F13] != 0xfd) M_BREAKPOINT;

/*LogFile(CStrF("(CBSP_LinkContext::Insert) ID %d, Box %s, %s", _ID,
		(const char*) _Box.m_Min.GetString(),
		(const char*) _Box.m_Max.GetString()) );*/

	m_lIDLinkBox[_ID] = _Box;
	m_lIDFlags[_ID] = _Flags;

	m_CurLinkID = _ID;
	m_pModel->LinkBox_r(this, 1, _Box);

	if (!m_lIDLinkMap[_ID])
	{
		InsertInfinite(_ID, _Flags);
//		ConOut(CStrF("§c0f0NOTE: (CBSP_LinkContext::Insert) ID %d (%s) was not linked to any PL.", _ID, (char*)_Box.GetString() ));
	}

//if (bCheck && pByte[0x04474F13] != 0xfd) M_BREAKPOINT;
}

void CBSP_LinkContext::Insert(int _ID, int _iPL)
{
	MAUTOSTRIP(CBSP_LinkContext_Insert_2, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP_LinkContext::Insert");

	Remove(_ID);
	m_iNextLink = 0;
	m_iPrevLink = 0;

/*	m_iNextLink = m_lIDLinkMap[_ID];
	m_iPrevLink = 0;
	if (!m_iNextLink)
	{
		int iLink = m_lIDLinkMap[_ID] = m_LinkHeap.AllocID();
		if (iLink < 0) Error("Insert", "Unable to allocate link.");

		m_pLinks[iLink].m_iPortalLeaf = 0;
		m_pLinks[iLink].m_ID = _ID;
		m_pLinks[iLink].m_iLinkPrevObject = 0;
		m_pLinks[iLink].m_iLinkNextObject = 0;
		m_pLinks[iLink].m_iLinkPrevPL = 0;
		m_pLinks[iLink].m_iLinkNextPL = 0;
		m_iNextLink = iLink;
	}*/

	m_CurLinkID = _ID;
	AddPortalLeaf(_iPL);
}

void CBSP_LinkContext::InsertInfinite(int _ID, int _Flags)
{
	MAUTOSTRIP(CBSP_LinkContext_InsertInfinite, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP_LinkContext::InsertInfinite");
//	if (m_pLinks != m_lLinks.GetBasePtr()) M_BREAKPOINT;
	Remove(_ID);
	m_iNextLink = 0;
	m_iPrevLink = 0;

/*	m_iNextLink = m_lIDLinkMap[_ID];
	m_iPrevLink = 0;
	if (!m_iNextLink)
	{
		int iLink = m_lIDLinkMap[_ID] = m_LinkHeap.AllocID();
		if (iLink < 0) Error("Insert", "Unable to allocate link.");

		m_pLinks[iLink].m_iPortalLeaf = 0;
		m_pLinks[iLink].m_ID = _ID;
		m_pLinks[iLink].m_iLinkPrevObject = 0;
		m_pLinks[iLink].m_iLinkNextObject = 0;
		m_pLinks[iLink].m_iLinkPrevPL = 0;
		m_pLinks[iLink].m_iLinkNextPL = 0;
		m_iNextLink = iLink;
	}*/

/*LogFile(CStrF("(CBSP_LinkContext::Insert) ID %d, Box %s, %s", _ID,
		(const char*) _Box.m_Min.GetString(),
		(const char*) _Box.m_Max.GetString()) );*/

	m_lIDFlags[_ID] = _Flags;

	m_CurLinkID = _ID;

	AddPortalLeaf(m_iPLInfinite);
}

void CBSP_LinkContext::Remove(int _ID)
{
	MAUTOSTRIP(CBSP_LinkContext_Remove, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP_LinkContext::Remove");
//	if (m_pLinks != m_lLinks.GetBasePtr()) M_BREAKPOINT;
	int iLink = m_lIDLinkMap[_ID];
	while(iLink)
	{
		M_ASSERT(m_lLinks.ValidPos(iLink), CStrF("Link out of range. %d/%d", iLink, m_lLinks.Len()));

		m_LinkHeap.FreeID(iLink);
		const CBSP_Link* pL = &m_pLinks[iLink];

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
}

int CBSP_LinkContext::EnumPortalLeaf(int _iPL, uint16* _piRet, int _MaxRet)
{
	MAUTOSTRIP(CBSP_LinkContext_EnumPortalLeaf, 0);
	int nRet = 0;
	int iLink = m_pPLLinkMap[_iPL];
	while(iLink)
	{
		M_ASSERT(m_lLinks.ValidPos(iLink), CStrF("Link out of range. %d/%d", iLink, m_lLinks.Len()));

		if (nRet >= _MaxRet) Error("EnumBox_r", CStrF("Out of enumeration space. (%d)", _MaxRet));
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

int CBSP_LinkContext::EnumPortalLeafWithClip(int _iPL, uint16* _piRet, int _MaxRet, const CRC_ClipVolume* _pClip)
{
	MAUTOSTRIP(CBSP_LinkContext_EnumPortalLeafWithClip, 0);
	int nRet = 0;
	int iLink = m_pPLLinkMap[_iPL];

	for(; iLink; iLink = m_pLinks[iLink].m_iLinkNextObject)
	{
		M_ASSERT(m_lLinks.ValidPos(iLink), CStrF("Link out of range. %d/%d", iLink, m_lLinks.Len()));

		if (nRet >= _MaxRet) Error("EnumBox_r", CStrF("Out of enumeration space. (%d)", _MaxRet));
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

int CBSP_LinkContext::MergeIDs(uint16* _piRet, int _nRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP_LinkContext_MergeIDs, 0);
	int nRet = _nRet;
	for(int iM = 0; iM < _nMerge; iM++)
	{
		int iElem = _pMergeElem[iM];
//		if (!(m_lIDLinkMap[iElem] & 0x8000))
		if (!(m_pIDLinkMap[iElem] & 0x8000))
		{
			if (nRet >= _MaxRet) Error("EnumVisible", "Out of enumeration space.");
//			m_lIDLinkMap[iElem] |= 0x8000;
			m_pIDLinkMap[iElem] |= 0x8000;
			_piRet[nRet++] = iElem;
		}
	}
	return nRet;
}

int CBSP_LinkContext::EnumBox_r(int _iNode, const CBox3Dfp32& _Box, uint16* _piRet, int _MaxRet)
{
	MAUTOSTRIP(CBSP_LinkContext_EnumBox_r, 0);
	MSCOPESHORT(CBSP_LinkContext::EnumBox_r); //AR-SCOPE

	int nRet = 0;
	const CBSP_Node* pN = &m_pModel->m_lNodes[_iNode];
	if ((pN->m_Flags & (XW_NODE_PORTAL | XW_NODE_STRUCTURE)) == (XW_NODE_PORTAL | XW_NODE_STRUCTURE))
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
				if (pP->GetBoxMaxDistance(_Box.m_Min, _Box.m_Max) > MODEL_BSP_EPSILON)
					nRet += EnumBox_r(pN->m_iNodeFront, _Box, &_piRet[nRet], _MaxRet - nRet);
				if (pP->GetBoxMinDistance(_Box.m_Min, _Box.m_Max) < -MODEL_BSP_EPSILON)
					nRet += EnumBox_r(pN->m_iNodeBack, _Box, &_piRet[nRet], _MaxRet - nRet);
			}
		}
	}

	return nRet;
}

int CBSP_LinkContext::EnumBox(const CBox3Dfp32& _Box, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP_LinkContext_EnumBox, 0);
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

int CBSP_LinkContext::EnumVisible(CBSPModel_ViewInstance* _pView, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP_LinkContext_EnumVisible, 0);
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

int CBSP_LinkContext::EnumPVS(const uint8* _pPVS, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP_LinkContext_EnumPVS, 0);
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

int CBSP_LinkContext::EnumIDs(int _iPL, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP_LinkContext_EnumIDs, 0);
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
			if (nRet >= _MaxRet) Error("EnumVisible", CStrF("Out of enumeration space. (%d)", _MaxRet));

			// Add to list
			_piRet[nRet++] = m_pLinks[iLink].m_ID;

			// Next...
			iLink = m_pLinks[iLink].m_iLinkNextObject;
		}
		return nRet;
	}
}

int CBSP_LinkContext::EnumPortalLeaves(int _ID, uint16* _piRet, int _MaxRet)
{
	MAUTOSTRIP(CBSP_LinkContext_EnumPortalLeaves, 0);
	int nRet = 0;
	int iLink = m_pIDLinkMap[_ID];
	while(iLink)
	{
		const CBSP_Link* pL = &m_pLinks[iLink];
		if (nRet >= _MaxRet) Error("EnumPortalLeaves", CStr("Out of enumeration space. (%d)") );
		_piRet[nRet++] = pL->m_iPortalLeaf;
		iLink = pL->m_iLinkNextPL;
	}
	return nRet;
}

int CBSP_LinkContext::EnumPortalLeaves_Single(int _ID)
{
	MAUTOSTRIP(CBSP_LinkContext_EnumPortalLeaves_Single, 0);
	int iLink = m_pIDLinkMap[_ID];
	if (iLink)
		return m_pLinks[iLink].m_iPortalLeaf;
	else
		return -1;
}

void CBSP_LinkContext::AddPortalLeaf(int _iPL)
{
	MAUTOSTRIP(CBSP_LinkContext_AddPortalLeaf, MAUTOSTRIP_VOID);
	MSCOPESHORT(CBSP_LinkContext::AddPortalLeaf); //AR-SCOPE

	int iLink = m_iNextLink;
	if (!iLink)
	{
		// Create new link.
		iLink= m_LinkHeap.AllocID();
		if (iLink < 0)
		{
			//Error("Insert", "Unable to allocate link.");

			// Only report this problem once every second.
#ifndef	M_RTM
			static CMStopTimer g_LastLogTime(1.0f);
			if (g_LastLogTime.Done())
			{
				M_TRACEALWAYS("(CBSP_LinkContext::AddPortalLeaf) Unable to allocate link.\n");
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
		CBSP_Link* pL = &m_pLinks[iLink];
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

// -------------------------------------------------------------------
//  CXR_Model_BSP
// -------------------------------------------------------------------
void CXR_Model_BSP::LinkBox_r(CBSP_LinkContext* _pLinkCtx, int _iNode, const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_LinkBox_r, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP::LinkBox_r); //AR-SCOPE

	if (!_iNode) return;
	CBSP_Node* pN = &m_pNodes[_iNode];
	if (pN->IsStructureLeaf())
	{
		_pLinkCtx->AddPortalLeaf(pN->m_iPortalLeaf);
		return;
	}

	if (pN->IsNode())
	{
/*		int iBack = pN->m_iNodeBack;
		if (!iBack)
		{
			int iFront = pN->m_iNodeFront;
			if (m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_Box.m_Min, _Box.m_Max) > -MODEL_BSP_EPSILON)
				LinkBox_r(_pLinkCtx, iFront, _Box);
		}
		else
		{
			int iFront = pN->m_iNodeFront;
			if (!iFront)
			{
				if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) < MODEL_BSP_EPSILON)
					LinkBox_r(_pLinkCtx, iBack, _Box);
			}
			else
			{
				if (m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_Box.m_Min, _Box.m_Max) > -MODEL_BSP_EPSILON)
					LinkBox_r(_pLinkCtx, iFront, _Box);
				if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) < MODEL_BSP_EPSILON)
					LinkBox_r(_pLinkCtx, iBack, _Box);
			}
		}*/

//LogFile(CStrF("Prnt %d, N %d, ChildF %d ChildB %d", pN->m_iNodeParent, _iNode, pN->m_iNodeFront, pN->m_iNodeBack));
		int iBack = pN->m_iNodeBack;
		if (!iBack)
		{
			int iFront = pN->m_iNodeFront;
			if (m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_Box.m_Min, _Box.m_Max) > LINKBOX_EPSILON)
				LinkBox_r(_pLinkCtx, iFront, _Box);
		}
		else
		{
			int iFront = pN->m_iNodeFront;
			if (!iFront)
			{
				if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) < -LINKBOX_EPSILON)
					LinkBox_r(_pLinkCtx, iBack, _Box);
			}
			else
			{
				if (m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_Box.m_Min, _Box.m_Max) > LINKBOX_EPSILON)
					LinkBox_r(_pLinkCtx, iFront, _Box);
				if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) < -LINKBOX_EPSILON)
					LinkBox_r(_pLinkCtx, iBack, _Box);
			}
		}
	}
}

// -------------------------------------------------------------------
static int ms_EnumPL = 0;
uint32* CXR_Model_BSP::ms_piEnum = NULL;
uint16* CXR_Model_BSP::ms_piEnumPL = NULL;
int CXR_Model_BSP::ms_nEnum = 0;
int CXR_Model_BSP::ms_nEnumUntag = 0;
int CXR_Model_BSP::ms_MaxEnum = 0;
int CXR_Model_BSP::ms_EnumQuality = 0;
int CXR_Model_BSP::ms_EnumMedium = 0;
int CXR_Model_BSP::ms_EnumFaceFlags = 0;
CBox3Dfp32 CXR_Model_BSP::ms_EnumBox;
CVec3Dfp32 CXR_Model_BSP::ms_EnumSphere;
fp32 CXR_Model_BSP::ms_EnumSphereR = 0;
bool CXR_Model_BSP::ms_EnumError = false;

CPotColSet *CXR_Model_BSP::ms_pcs;
int CXR_Model_BSP::ms_iObj;
uint8 CXR_Model_BSP::ms_bIN1N2Flags;


void CXR_Model_BSP::EnumFaces_Sphere_r(CXR_PhysicsContext* _pPhysContext, int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_Sphere_r, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP::EnumFaces_Sphere_r); //AR-SCOPE

	if (!_iNode) return;
	CBSP_Node* pN = &m_lNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		ms_EnumPL = pN->m_iPortalLeaf;
	}

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;
		uint8* pFTag = m_lFaceTagList.GetBasePtr();
		uint32* piFUntag = m_lFaceUntagList.GetBasePtr();
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

			CBSP_Face* pF = &m_pFaces[iFace];

			// Check face flag?
			if (ms_EnumQuality & ENUM_FACEFLAGS && !(ms_EnumFaceFlags & pF->m_Flags)) continue;

			// Check medium
			if (ms_EnumQuality & ENUM_MEDIUMFLAGS && !(m_lMediums[pF->m_iBackMedium].m_MediumFlags & ms_EnumMedium)) continue;

			int iPlane = pF->m_iPlane;
			fp32 Dist = m_pPlanes[iPlane].Distance(ms_EnumSphere);
			if (Abs(Dist) < ms_EnumSphereR)
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
			EnumFaces_Sphere_r(_pPhysContext, pN->m_iNodeBack);
		if (Dist > -ms_EnumSphereR)
			EnumFaces_Sphere_r(_pPhysContext, pN->m_iNodeFront);
	}
}

void CXR_Model_BSP::EnumFaces_Box_r(CXR_PhysicsContext* _pPhysContext, int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_Box_r, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	CBSP_Node* pN = &m_lNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		ms_EnumPL = pN->m_iPortalLeaf;
	}

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;

		uint8* pFTag = m_lFaceTagList.GetBasePtr();
		uint32* piFUntag = m_lFaceUntagList.GetBasePtr();

		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_piFaces[iiFaces + f];
			
			if (pFTag[iFace >> 3] & (1 << (iFace & 7)))
				continue;
				
			if (!pFTag[iFace >> 3])
				piFUntag[ms_nEnumUntag++] = iFace;
			
			pFTag[iFace >> 3] |= 1 << (iFace & 7);

			CBSP_Face* pF = &m_pFaces[iFace];
			
			int iPlane = pF->m_iPlane;
			fp32 MinDist = m_pPlanes[iPlane].GetBoxMinDistance( ms_EnumBox.m_Min, ms_EnumBox.m_Max );

			if( MinDist > MODEL_BSP_EPSILON )
				continue;

			// Check face flag?
			if (ms_EnumQuality & ENUM_FACEFLAGS && !(ms_EnumFaceFlags & pF->m_Flags))
				continue;

			// Check medium
			if (ms_EnumQuality & ENUM_MEDIUMFLAGS && !(m_lMediums[pF->m_iBackMedium].m_MediumFlags & ms_EnumMedium))
				continue;

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
			}

			M_ASSERT( ms_nEnum < ms_MaxEnum, "EnumFaces_Box_r" );

			ms_piEnum[ms_nEnum] = iFace;

			if (ms_piEnumPL)
				ms_piEnumPL[ms_nEnum] = ms_EnumPL;

			ms_nEnum++;

Continue: ;

		}
	}
	else if (pN->IsNode())
	{
		fp32 MinDist = m_pPlanes[pN->m_iPlane].GetBoxMinDistance(ms_EnumBox.m_Min, ms_EnumBox.m_Max);
		fp32 MaxDist = m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(ms_EnumBox.m_Min, ms_EnumBox.m_Max);

		if (MinDist < MODEL_BSP_EPSILON)
			EnumFaces_Box_r(_pPhysContext, pN->m_iNodeBack);
		if (MaxDist > -MODEL_BSP_EPSILON)
			EnumFaces_Box_r(_pPhysContext, pN->m_iNodeFront);
	}
}

void CXR_Model_BSP::EnumFaces_All_r(CXR_PhysicsContext* _pPhysContext, int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_All_r, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP::EnumFaces_All_r); //AR-SCOPE

	if (!_iNode) return;
	CBSP_Node* pN = &m_lNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		ms_EnumPL = pN->m_iPortalLeaf;
	}

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;
		uint8* pFTag = m_lFaceTagList.GetBasePtr();
		uint32* piFUntag = m_lFaceUntagList.GetBasePtr();
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
		EnumFaces_All_r(_pPhysContext, pN->m_iNodeBack);
		EnumFaces_All_r(_pPhysContext, pN->m_iNodeFront);
	}
}

int CXR_Model_BSP::EnumFaces_Sphere(CXR_PhysicsContext* _pPhysContext, int _iNode, uint32* _piFaces, int _MaxFaces, int _Quality, int _Medium, int _FaceFlags, const CVec3Dfp32& _Origin, fp32 _Radius, uint16* _piFacePL)
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_Sphere, 0);
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

	EnumFaces_Sphere_r(_pPhysContext, _iNode);
	return ms_nEnum;
}

int CXR_Model_BSP::EnumFaces_Box(CXR_PhysicsContext* _pPhysContext, int _iNode, uint32* _piFaces, int _MaxFaces, int _Quality, int _Medium, int _FaceFlags, const CBox3Dfp32& _Box, uint16* _piFacePL)
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_Box, 0);
	MSCOPESHORT(CXR_Model_BSP::EnumFaces_Box);

	ms_EnumBox = _Box;
	ms_piEnum = _piFaces;
	ms_piEnumPL = _piFacePL;
	ms_MaxEnum = _MaxFaces;
	ms_nEnum = 0;
	ms_EnumQuality = _Quality;
	ms_EnumMedium = _Medium;
	ms_EnumFaceFlags = _FaceFlags;
	ms_EnumError = 0;
	ms_EnumPL = 0;
	EnumFaces_Box_r(_pPhysContext, _iNode);
	EnumFaces_Untag(_pPhysContext);

	return ms_nEnum;
}

int CXR_Model_BSP::EnumFaces_All(CXR_PhysicsContext* _pPhysContext, int _iNode, uint32* _piFaces, int _MaxFaces)
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_All, 0);
	ms_piEnum = _piFaces;
	ms_piEnumPL = NULL;
	ms_MaxEnum = _MaxFaces;
	ms_nEnum = 0;
	ms_EnumQuality = 0;
	ms_EnumMedium = 0;
	ms_EnumFaceFlags = 0;
	ms_EnumError = 0;
	ms_EnumPL = 0;
//EnumFaces_Untag();
	EnumFaces_All_r(_pPhysContext, _iNode);
	return ms_nEnum;
}


void CXR_Model_BSP::EnumFaces_Untag(CXR_PhysicsContext* _pPhysContext)
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_Untag, MAUTOSTRIP_VOID);
	const uint32* piF = m_lFaceUntagList.GetBasePtr();
	uint8* pFTag = m_lFaceTagList.GetBasePtr();
	for(int i = 0; i < ms_nEnumUntag; i++)
	{
		int iFace = piF[i];
		pFTag[iFace >> 3] = 0;
//		ConOut(CStrF("    Removed face %d  (%d)", iFace, i));
	}
	ms_nEnumUntag = 0;
}

/*void CXR_Model_BSP::EnumFaces_UntagExact(const uint32* _piFaces, int _nFaces)
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_UntagExact, NULL);
	uint8* pFTag = m_lFaceTagList.GetBasePtr();
	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = _piFaces[i];
		pFTag[iFace >> 3] &= ~(1 << (iFace & 7));
	}
}*/

// -------------------------------------------------------------------
int CXR_Model_BSP::CountFaces_r(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_CountFaces_r, 0);
	MSCOPESHORT(CXR_Model_BSP::CountFaces_r); //AR-SCOPE

	CBSP_Node* pN = &m_lNodes[_iNode];
	if (pN->IsLeaf())
		return pN->m_nFaces;
	else
		return CountFaces_r(pN->m_iNodeFront) + CountFaces_r(pN->m_iNodeBack);
}

void CXR_Model_BSP::InitFaces()
{
	MAUTOSTRIP(CXR_Model_BSP_InitFaces, MAUTOSTRIP_VOID);
}

aint CXR_Model_BSP::GetParam(int _Param)
{
	MAUTOSTRIP(CXR_Model_BSP_GetParam, 0);
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
	
	default :
		return CXR_Model::GetParam(_Param);
	}
}

void CXR_Model_BSP::SetParam(int _Param, aint _Value)
{
	MAUTOSTRIP(CXR_Model_BSP_SetParam, MAUTOSTRIP_VOID);
	switch(_Param)
	{
	case MODEL_BSP_PARAM_GLOBALFLAGS :
		GetGlobalEnable() = _Value;

	default :
		CXR_Model::SetParam(_Param, _Value);
	}
}

int CXR_Model_BSP::GetParamfv(int _Param, fp32* _pRetValues)
{
	MAUTOSTRIP(CXR_Model_BSP_GetParamfv, 0);
	return CXR_Model::GetParamfv(_Param, _pRetValues);
}

void CXR_Model_BSP::SetParamfv(int _Param, const fp32* _pValues)
{
	MAUTOSTRIP(CXR_Model_BSP_SetParamfv, MAUTOSTRIP_VOID);
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

fp32 CXR_Model_BSP::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_BSP_GetBound_Sphere, 0.0f);
	return m_BoundRadius;
}

void CXR_Model_BSP::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_BSP_GetBound_Box, MAUTOSTRIP_VOID);
	_Box = m_BoundBox;
}

int CXR_Model_BSP::NumFacesInTree(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_NumFacesInTree, 0);
	if (!_iNode) return 0;
	if (m_pNodes[_iNode].IsLeaf()) return m_pNodes[_iNode].m_nFaces;

	return NumFacesInTree(m_pNodes[_iNode].m_iNodeFront) + NumFacesInTree(m_pNodes[_iNode].m_iNodeBack);
}

int CXR_Model_BSP::GetNumEnabledLeaves(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_GetNumEnabledLeaves, 0);
	if (!_iNode) return 0;
	if (!(m_pNodes[_iNode].m_Flags & XW_NODE_ENABLE)) return 0;

	if (m_pNodes[_iNode].IsNode())
		return GetNumEnabledLeaves(m_pNodes[_iNode].m_iNodeFront) + GetNumEnabledLeaves(m_pNodes[_iNode].m_iNodeBack);
	else
		return 1;
}

int CXR_Model_BSP::GetFirstSplitNode_Sphere(const CVec3Dfp32& _Pos, fp32 _Radius)
{
	MAUTOSTRIP(CXR_Model_BSP_GetFirstSplitNode_Sphere, 0);
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

int CXR_Model_BSP::GetFirstSplitNode_Box(const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax)
{
	MAUTOSTRIP(CXR_Model_BSP_GetFirstSplitNode_Box, 0);
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

int CXR_Model_BSP::GetFirstSplitNode_Polyhedron(const CVec3Dfp32* _pV, int _nVertices)
{
	MAUTOSTRIP(CXR_Model_BSP_GetFirstSplitNode_Polyhedron, 0);
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

int CXR_Model_BSP::Structure_GetFirstSplitNode_Polyhedron(const CVec3Dfp32* _pV, int _nVertices)
{
	MAUTOSTRIP(CXR_Model_BSP_Structure_GetFirstSplitNode_Polyhedron, 0);
	int iNode = 1;
	do
	{
		CBSP_Node* pN = &m_pNodes[iNode];
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

void CXR_Model_BSP::EnableTreeFromNode_Sphere(int _iNode, const CVec3Dfp32& _Pos, fp32 _Radius)
{
	MAUTOSTRIP(CXR_Model_BSP_EnableTreeFromNode_Sphere, MAUTOSTRIP_VOID);
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

void CXR_Model_BSP::EnableTreeFromNode_Box(int _iNode, const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax)
{
	MAUTOSTRIP(CXR_Model_BSP_EnableTreeFromNode_Box, MAUTOSTRIP_VOID);
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

void CXR_Model_BSP::EnableTreeFromNode_Polyhedron(int _iNode, const CVec3Dfp32* _pV, int _nVertices)
{
	MAUTOSTRIP(CXR_Model_BSP_EnableTreeFromNode_Polyhedron, MAUTOSTRIP_VOID);
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

void CXR_Model_BSP::EnableTreeFromNode(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_EnableTreeFromNode, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	while ((_iNode) && !(m_pNodes[_iNode].m_Flags & XW_NODE_ENABLE))
	{
		m_pNodes[_iNode].m_Flags |= XW_NODE_ENABLE;
		_iNode = m_pNodes[_iNode].m_iNodeParent;
	}
}

void CXR_Model_BSP::EnableTreeFlagFromNode(int _iNode, int _Flag)
{
	MAUTOSTRIP(CXR_Model_BSP_EnableTreeFlagFromNode, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	while ((_iNode) && !(m_pNodes[_iNode].m_Flags & _Flag))
	{
		m_pNodes[_iNode].m_Flags |= _Flag;
		_iNode = m_pNodes[_iNode].m_iNodeParent;
	}
}

void CXR_Model_BSP::EnableWithPortalFloodFill(int _iNode, int _EnableFlag, int _MediumFlag)
{
	MAUTOSTRIP(CXR_Model_BSP_EnableWithPortalFloodFill, MAUTOSTRIP_VOID);
	CBSP_Node* pN = &m_pNodes[_iNode];
	if (pN->m_Flags & _EnableFlag) return;

	if (!pN->IsStructureLeaf())
	{
		ConOut(CStrF("(CXR_Model_BSP::EnableWithPortalFloodFill) Node %d was not a structure-leaf.", _iNode));
		return;
	}
	const CBSP_PortalLeaf* pPL = &m_lPortalLeaves[pN->m_iPortalLeaf];
	if (!(pPL->m_ContainsMedium & _MediumFlag)) return;

	pN->m_Flags |= _EnableFlag;

	for(int p = 0; p < pPL->m_nPortals; p++)
	{
		int iP = m_liPortals[pPL->m_iiPortals + p];
		const CBSP_Portal* pP = &m_pPortals[iP];
		EnableWithPortalFloodFill((pP->m_iNodeFront == _iNode) ? pP->m_iNodeBack : pP->m_iNodeFront, _EnableFlag, _MediumFlag);
	}
}

void CXR_Model_BSP::DisableWithPortalFloodFill(int _iNode, int _EnableFlag)
{
	MAUTOSTRIP(CXR_Model_BSP_DisableWithPortalFloodFill, MAUTOSTRIP_VOID);
	CBSP_Node* pN = &m_pNodes[_iNode];
	if (!(pN->m_Flags & _EnableFlag)) return;

	if (!pN->IsStructureLeaf())
	{
		ConOut(CStrF("(CXR_Model_BSP::EnableWithPortalFloodFill) Node %d was not a structure-leaf.", _iNode));
		return;
	}
	const CBSP_PortalLeaf* pPL = &m_lPortalLeaves[pN->m_iPortalLeaf];
	pN->m_Flags &= ~_EnableFlag;

	for(int p = 0; p < pPL->m_nPortals; p++)
	{
		int iP = m_liPortals[pPL->m_iiPortals + p];
		const CBSP_Portal* pP = &m_pPortals[iP];
		DisableWithPortalFloodFill((pP->m_iNodeFront == _iNode) ? pP->m_iNodeBack : pP->m_iNodeFront, _EnableFlag);
	}
}

void CXR_Model_BSP::Structure_EnableTreeFromNode_Polyhedron(int _iNode, const CVec3Dfp32* _pV, int _nVertices)
{
	MAUTOSTRIP(CXR_Model_BSP_Structure_EnableTreeFromNode_Polyhedron, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	CBSP_Node* pN = &m_pNodes[_iNode];
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

void CXR_Model_BSP::InitBoundNodesFromEnabledNodes(int _iNode, int _iNodeLastSplit, int _iNodeLastSplitSide)
{
	MAUTOSTRIP(CXR_Model_BSP_InitBoundNodesFromEnabledNodes, MAUTOSTRIP_VOID);
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

void CXR_Model_BSP::Structure_InitBoundNodesFromEnabledNodes(int _iNode, int _iNodeLastSplit, int _iNodeLastSplitSide)
{
	MAUTOSTRIP(CXR_Model_BSP_Structure_InitBoundNodesFromEnabledNodes, MAUTOSTRIP_VOID);
	CBSP_Node* pN = &m_pNodes[_iNode];

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

void CXR_Model_BSP::DisableTree(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_DisableTree, MAUTOSTRIP_VOID);
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

void CXR_Model_BSP::DisableTreeFlag(int _iNode, int _Flag)
{
	MAUTOSTRIP(CXR_Model_BSP_DisableTreeFlag, MAUTOSTRIP_VOID);
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

bool CXR_Model_BSP::CheckTree_r(int _iNode, int _iParent, int _PortalDone)
{
	MAUTOSTRIP(CXR_Model_BSP_CheckTree_r, false);
	MSCOPESHORT(CXR_Model_BSP::CheckTree_r); //AR-SCOPE

	if (!_iNode) return true;
	CBSP_Node* pN = &m_lNodes[_iNode];

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
/*
		if (pN->m_Flags & XW_NODE_NAVIGATION)
		{
//LogFile(CStrF("Node %d, Nav.Leaf %d, Medium %.8x", _iNode, pN->m_iNavigationLeaf, m_lPortalLeaves[pN->m_iNavigationLeaf].m_ContainsMedium));
			if (m_spCapture)
			{
				spCSolid spLS = CreateLeafSolid(_iNode);
				if ((spLS->m_BoundMax - spLS->m_BoundMin).Length() < 1000000.0)
				{
					CMat4Dfp32 Unit; Unit.Unit();
					CPixel32 Color(Random*16+16, Random*16+16, Random*16+16);
					spLS->RenderSolidNoCull(m_spCapture, Unit, Unit, Color, 0);
					spLS->RenderEdges(m_spCapture, 0xffffffff);
				}
			}

			if (_PortalDone & 2)
				LogFile(CStrF("(CheckTree) More than one navigation-leaf in branch. (iNode %d)", _iNode));
			_PortalDone |= 2;
		}
*/
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

// -------------------------------------------------------------------
int CXR_Model_BSP::GetLeaf(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CXR_Model_BSP_GetLeaf, 0);
	int iNode = 1;
	while(m_lNodes[iNode].IsNode())
	{
		int iPlane = m_lNodes[iNode].m_iPlane;
		int VSide = m_lPlanes[iPlane].GetPlaneSide(_v);
		iNode = (VSide >= 0) ? m_lNodes[iNode].m_iNodeFront : m_lNodes[iNode].m_iNodeBack;
	}
	return iNode;
}

int CXR_Model_BSP::GetPortalLeaf(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CXR_Model_BSP_GetPortalLeaf, 0);
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

int CXR_Model_BSP::GetStructurePortalLeaf(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CXR_Model_BSP_GetStructurePortalLeaf, 0);
	int iNode = 1;
	while(iNode)
	{
		const CBSP_Node* pN = &m_lNodes[iNode];
		if (pN->IsStructureLeaf()) return pN->m_iPortalLeaf;
		if (pN->IsLeaf()) return -1;
		int iPlane = pN->m_iPlane;
		int VSide = m_lPlanes[iPlane].GetPlaneSide(_v);
		iNode = (VSide >= 0) ? pN->m_iNodeFront : pN->m_iNodeBack;
	}
	return -1;
}

/*int CXR_Model_BSP::GetNavigationPortalLeaf(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CXR_Model_BSP_GetNavigationPortalLeaf, NULL);
	int iNode = 1;
	while(iNode)
	{
		const CBSP_Node* pN = &m_lNodes[iNode];
		if (pN->IsNavigationLeaf()) return pN->m_iNavigationLeaf;
		if (pN->IsLeaf()) return -1;
		int iPlane = pN->m_iPlane;
		int VSide = m_lPlanes[iPlane].GetPlaneSide(_v);
		iNode = (VSide >= 0) ? pN->m_iNodeFront : pN->m_iNodeBack;
	}
	return -1;
}*/

CStr CXR_Model_BSP::GetInfo()
{
	MAUTOSTRIP(CXR_Model_BSP_GetInfo, CStr());
#ifdef M_Profile
	return CStr(TString("All: ", ms_Time) + TString(",  Leaves: ", ms_TimeRenderLeafList-ms_TimeRenderPoly1) + TString(",  Queue: ", ms_TimeRenderFaceQueue-ms_TimeRenderPoly2));
#endif
	return "";
}

void CXR_Model_BSP::InitializeListPtrs()
{
	MAUTOSTRIP(CXR_Model_BSP_InitializeListPtrs, MAUTOSTRIP_VOID);
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

