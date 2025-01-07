#include "PCH.h"

#include "WBSP2Model.h"
#include "WBSP2Def.h"
#include "MFloat.h"

//#pragma optimize("",off)
//#pragma inline_depth(0)

int aShiftMulTab[32] = {M_Bit(0), M_Bit(1), M_Bit(2), M_Bit(3), M_Bit(4), M_Bit(5), M_Bit(6), M_Bit(7), M_Bit(8), M_Bit(9), M_Bit(10), M_Bit(11), M_Bit(12), M_Bit(13), M_Bit(14), M_Bit(15), M_Bit(16), M_Bit(17), M_Bit(18), M_Bit(19), M_Bit(20), M_Bit(21), M_Bit(22), M_Bit(23), M_Bit(24), M_Bit(25), M_Bit(26), M_Bit(27), M_Bit(28), M_Bit(29), M_Bit(30), M_Bit(31)};

// -------------------------------------------------------------------
// CBSP2_SLCIBContainer
// -------------------------------------------------------------------
CBSP2_SLCIBContainer::CBSP2_SLCIBContainer()
{
	m_pLD = NULL;
	m_pModel = NULL;
}

CBSP2_SLCIBContainer::~CBSP2_SLCIBContainer()
{
	if (m_pLD)
	{
		TAP<CBSP2_SLCIBInfo> pIBI = m_pLD->m_lSLCIBI;
		for(int i = 0; i < pIBI.Len(); i++)
		{
			if (pIBI[i].m_IBID)
			{
				m_pVBCtx->FreeID(pIBI[i].m_IBID);
				pIBI[i].m_IBID = 0;
			}
		}
	}
}

void CBSP2_SLCIBContainer::Create(class CBSP2_LightData* _pLD, CXR_Model_BSP2* _pModel)
{
	m_pLD = _pLD;
	m_pModel = _pModel;

	TAP<CBSP2_SLCIBInfo> pIBI = m_pLD->m_lSLCIBI;
	for(int i = 0; i < pIBI.Len(); i++)
	{
		pIBI[i].m_IBID = m_pVBCtx->AllocID(m_iVBC, i);
	}
}

int CBSP2_SLCIBContainer::GetNumLocal()
{
	return m_pLD->m_lSLCIBI.Len();
}

int CBSP2_SLCIBContainer::GetID(int _iLocal)
{
	return m_pLD->m_lSLCIBI[_iLocal].m_IBID;
}

CFStr CBSP2_SLCIBContainer::GetName(int _iLocal)
{
	return CFStrF("BSP2:%s:SLCIB:%04d", m_pLD->m_FileName.Str(), _iLocal);
}

void CBSP2_SLCIBContainer::Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags)
{
	if (m_lTempPrim.Len())
	{
		M_TRACEALWAYS("(CBSP2_SLCIBContainer::Get) Temp data not released, iLocal %d\n", _iLocal);
	}
	const CBSP2_SLCIBInfo& IBI = m_pLD->m_lSLCIBI[_iLocal];
	m_lTempPrim.SetLen(IBI.m_nPrim);

	uint iPrim = 0;
	for(int i = 0; i < IBI.m_nSLC; i++)
	{
		const CBSP2_SLCluster& SLC = m_pLD->m_lSLC[IBI.m_iSLC + i];
		int nPrim = m_pModel->RenderTesselatePrim_Triangles(&m_pLD->m_liSLCFaces[SLC.m_iiFaces], SLC.m_nFaces, &m_lTempPrim[iPrim]);
		if (nPrim + iPrim > m_lTempPrim.Len())
			Error("Get", CStrF("Primitives written outside buffer. (%d + %d > %d)", nPrim, iPrim, m_lTempPrim.Len()));
		iPrim += nPrim;
	}

	M_ASSERT(iPrim == IBI.m_nPrim, "Primitive count missmatch.");

//	FillChar(m_lTempPrim.GetBasePtr(), m_lTempPrim.ListSize(), 0);

	_VB.Clear();
	_VB.m_nPrim = IBI.m_nPrim/3;
	_VB.m_PrimType = CRC_RIP_TRIANGLES;
	_VB.m_piPrim = m_lTempPrim.GetBasePtr();
}

void CBSP2_SLCIBContainer::Release(int _iLocal)
{
	m_lTempPrim.Destroy();
}

// -------------------------------------------------------------------
//  CBSP2_LinkContext
// -------------------------------------------------------------------
#define LINKBOX_EPSILON 0.001f

CBSP2_LinkContext::CBSP2_LinkContext()
{
	MAUTOSTRIP(CBSP2_LinkContext_ctor, MAUTOSTRIP_VOID);
	m_pLinks = NULL;
	m_pModel = NULL;
	m_pIDLinkMap = NULL;
	m_pPLLinkMap = NULL;
}

void CBSP2_LinkContext::Create(CXR_Model_BSP2* _pModel, int _NumIDs, int _NumLinks, int _nPL, bool _bNeedBoxes)
{
	MAUTOSTRIP(CBSP2_LinkContext_Create, MAUTOSTRIP_VOID);
	m_pModel = _pModel;
	if (_NumIDs > 65535)
		Error("Create", "Can only handle 64k IDs.");

	m_MaxID = _NumIDs;
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

void CBSP2_LinkContext::Insert(int _ID, const CBox3Dfp32& _Box, int _Flags)
{
	MAUTOSTRIP(CBSP2_LinkContext_Insert, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP2_LinkContext::Insert");

	Remove(_ID);

	M_ASSERT(!m_lIDLinkMap[_ID], "!");

	m_iNextLink = 0;
	m_iPrevLink = 0;


	m_lIDLinkBox[_ID] = _Box;
	m_lIDFlags[_ID] = _Flags | 0x80;

	m_CurLinkID = _ID;
//	m_pModel->LinkBox_r(this, 1, _Box);
	m_pModel->LinkBox_i(this, 1, _Box);

/*	if (!m_lIDLinkMap[_ID])
	{
		InsertInfinite(_ID, _Flags);
//		ConOut(CStrF("§c0f0NOTE: (CBSP2_LinkContext::Insert) ID %d (%s) was not linked to any PL.", _ID, (char*)_Box.GetString() ));
	}*/

//if (bCheck && pByte[0x04474F13] != 0xfd) M_BREAKPOINT;
}

void CBSP2_LinkContext::InsertWithPVS(int _ID, const CBox3Dfp32& _Box, int _Flags, const uint8* _pPVS)
{
	MAUTOSTRIP(CBSP2_LinkContext_Insert, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP2_LinkContext::Insert");

	Remove(_ID);

	M_ASSERT(!m_lIDLinkMap[_ID], "!");

	m_iNextLink = 0;
	m_iPrevLink = 0;

	m_lIDLinkBox[_ID] = _Box;
	m_lIDFlags[_ID] = _Flags | 0x80;

	m_CurLinkID = _ID;
//	m_pModel->LinkBoxWithPVS_r(this, 1, _Box, _pPVS);
	m_pModel->LinkBoxWithPVS_i(this, 1, _Box, _pPVS);

/*	if (!m_lIDLinkMap[_ID])
	{
		InsertInfinite(_ID, _Flags);
//		ConOut(CStrF("§c0f0NOTE: (CBSP2_LinkContext::Insert) ID %d (%s) was not linked to any PL.", _ID, (char*)_Box.GetString() ));
	}*/
}


void CBSP2_LinkContext::Insert(int _ID, int _iPL)
{
	MAUTOSTRIP(CBSP2_LinkContext_Insert_2, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP2_LinkContext::Insert");

	Remove(_ID);
	m_iNextLink = 0;
	m_iPrevLink = 0;
	m_lIDFlags[_ID] = 0x80;

	m_CurLinkID = _ID;
	AddPortalLeaf(_iPL);
}

void CBSP2_LinkContext::InsertBegin(int _ID, int _Flags)
{
	MAUTOSTRIP(CBSP2_LinkContext_InsertBegin, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP2_LinkContext::InsertBegin");

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

void CBSP2_LinkContext::InsertEnd(int _ID)
{
	MAUTOSTRIP(CBSP2_LinkContext_InsertEnd, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP2_LinkContext::InsertEnd");

	m_bPLBlock = 0;

	// Unblock the PLs we linked
	int iLink = GetLink_ID(_ID);
	for(; iLink; iLink = m_pLinks[iLink].m_iLinkNextPL)
	{
		m_lPLBlock[m_pLinks[iLink].m_iPortalLeaf >> 3] = 0;
	}
}

void CBSP2_LinkContext::InsertInfinite(int _ID, int _Flags)
{
	MAUTOSTRIP(CBSP2_LinkContext_InsertInfinite, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP2_LinkContext::InsertInfinite");
	Remove(_ID);
	m_iNextLink = 0;
	m_iPrevLink = 0;


	m_lIDFlags[_ID] = _Flags | 0x80;

	m_CurLinkID = _ID;

	AddPortalLeaf(m_iPLInfinite);
}

void CBSP2_LinkContext::Remove(int _ID)
{
	MAUTOSTRIP(CBSP2_LinkContext_Remove, MAUTOSTRIP_VOID);
	M_ASSERT(m_pLinks == m_lLinks.GetBasePtr(), "CBSP2_LinkContext::Remove");
//	if (m_pLinks != m_lLinks.GetBasePtr()) M_BREAKPOINT;
	int iLink = m_lIDLinkMap[_ID];
	while(iLink)
	{
		M_ASSERT(m_lLinks.ValidPos(iLink), CStrF("Link out of range. %d/%d", iLink, m_lLinks.Len()));

		m_LinkHeap.FreeID(iLink);
		const CBSP2_Link* pL = &m_pLinks[iLink];

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

int CBSP2_LinkContext::EnumPortalLeafList(uint16* _piPL, int _nPL, uint16* _piRet, int _MaxRet)
{
	MAUTOSTRIP(CBSP2_LinkContext_EnumPortalLeafList, 0);
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

int CBSP2_LinkContext::EnumPortalLeaf(int _iPL, uint16* _piRet, int _MaxRet)
{
	MAUTOSTRIP(CBSP2_LinkContext_EnumPortalLeaf, 0);
	int nRet = 0;
	int iLink = m_pPLLinkMap[_iPL];
	uint16 *M_RESTRICT pIDLinkMap = m_pIDLinkMap;
	CBSP2_Link *M_RESTRICT pLinks = m_pLinks;
	// Is there a point in precaching iLink? We're guaranteed to miss it since we use it instantaly
	// The fucked up size of a link can make an instance straddle a cacheline so we precache 2 lines just to be safe
	M_PRECACHE128(0, &pLinks[iLink]);
	M_PRECACHE128(128, &pLinks[iLink]);
	M_PRECACHE128(0, &pLinks[pLinks[iLink].m_iLinkNextObject]);
	M_PRECACHE128(128, &pLinks[pLinks[iLink].m_iLinkNextObject]);
	while(iLink)
	{
		M_ASSERT(m_lLinks.ValidPos(iLink), CStrF("Link out of range. %d/%d", iLink, m_lLinks.Len()));
	
		int ID = pLinks[iLink].m_ID;

		// If taged, don't add it again.
		if (!(pIDLinkMap[ID] & 0x8000))
		{
			if (nRet >= _MaxRet)
#ifdef M_RTM
				return nRet;
#else
				Error("EnumBox_r", CStrF("Out of enumeration space. (%d)", _MaxRet));
#endif
			// Add to list and tag ID.
			_piRet[nRet++] = pLinks[iLink].m_ID;
			pIDLinkMap[ID] |= 0x8000;
		}

		// Next...
		iLink = pLinks[iLink].m_iLinkNextObject;
		M_PRECACHE128(0, &pLinks[pLinks[iLink].m_iLinkNextObject]);
		M_PRECACHE128(128, &pLinks[pLinks[iLink].m_iLinkNextObject]);
	}
	return nRet;
}

int CBSP2_LinkContext::EnumPortalLeafWithClip(int _iPL, uint16* _piRet, int _MaxRet, const CRC_ClipVolume* _pClip)
{
	MAUTOSTRIP(CBSP2_LinkContext_EnumPortalLeafWithClip, 0);
	int nRet = 0;
	int iLink = m_pPLLinkMap[_iPL];

	for(; iLink; iLink = m_pLinks[iLink].m_iLinkNextObject)
	{
		M_ASSERT(m_lLinks.ValidPos(iLink), CStrF("Link out of range. %d/%d", iLink, m_lLinks.Len()));

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

			if (nRet >= _MaxRet)
#ifdef M_RTM
				return nRet;
#else
				Error("EnumBox_r", CStrF("Out of enumeration space. (%d)", _MaxRet));
#endif
			// Add to list and tag ID.
			_piRet[nRet++] = m_pLinks[iLink].m_ID;
			m_pIDLinkMap[ID] |= 0x8000;
		}
	}
	return nRet;
}

int CBSP2_LinkContext::MergeIDs(uint16* _piRet, int _nRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP2_LinkContext_MergeIDs, 0);
	int nRet = _nRet;
	uint MaxID = m_MaxID;
	for(int iM = 0; iM < _nMerge; iM++)
	{
		uint iElem = _pMergeElem[iM];
		if ((iElem < MaxID) && !(m_pIDLinkMap[iElem] & 0x8000))
		{
			if (nRet >= _MaxRet)
#ifdef M_RTM
				return nRet;
#else
				Error("EnumVisible", "Out of enumeration space.");
#endif
			m_pIDLinkMap[iElem] |= 0x8000;
			_piRet[nRet++] = iElem;
		}
	}
	return nRet;
}

int CBSP2_LinkContext::EnumBox_r(int _iNode, const CBox3Dfp32& _Box, uint16* _piRet, int _MaxRet)
{
	MAUTOSTRIP(CBSP2_LinkContext_EnumBox_r, 0);
	MSCOPESHORT(CBSP2_LinkContext::EnumBox_r); //AR-SCOPE

	int nRet = 0;
	const CBSP2_Node* pN = &m_pModel->m_lNodes[_iNode];
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


int CBSP2_LinkContext::EnumBox(const CBox3Dfp32& _Box, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP2_LinkContext_EnumBox, 0);
	int nRet = 0;
	nRet += EnumPortalLeaf(m_iPLInfinite, &_piRet[nRet], _MaxRet-nRet);

#ifdef BSP_USE_OCTAAABB
	if( m_pModel->m_lOctaAABBNodes.Len() )
	{

		//Create an iterator- since portalleaves are not supplied, it will stop at them
		CBSP_OctaAABBIterator<CXR_Model_BSP2,CBSP2_PortalLeafExt> OctItr(m_pModel,NULL,_Box);

		uint32 iNext = OctItr.Next();
		while( iNext != 0xFFFFFFFF )
		{
			nRet += EnumPortalLeaf(iNext, &_piRet[nRet], _MaxRet-nRet);
			iNext = OctItr.Next();
		}
	}
	else
#endif
	{
		nRet += EnumBox_r(1, _Box, &_piRet[nRet], _MaxRet-nRet);
	}

	// Merge with incomming
	if (_nMerge) nRet = MergeIDs(_piRet, nRet, _MaxRet, _pMergeElem, _nMerge);

	// Untag all that were added.
	for(int i = 0; i < nRet; i++)
		m_pIDLinkMap[_piRet[i]] &= ~0x8000;

	return nRet;
}

int CBSP2_LinkContext::EnumVisible(CBSP2_ViewInstance* _pView, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP2_LinkContext_EnumVisible, 0);
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

int CBSP2_LinkContext::EnumPVS(const uint8* _pPVS, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP2_LinkContext_EnumPVS, 0);
	MSCOPESHORT(CBSP2_LinkContext::EnumPVS);

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

int CBSP2_LinkContext::EnumIDs(int _iPL, uint16* _piRet, int _MaxRet, const uint16* _pMergeElem, int _nMerge)
{
	MAUTOSTRIP(CBSP2_LinkContext_EnumIDs, 0);
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
			if (nRet >= _MaxRet)
#ifdef M_RTM
				return nRet;
#else
				Error("EnumVisible", CStrF("Out of enumeration space. (%d)", _MaxRet));
#endif

			// Add to list
			_piRet[nRet++] = m_pLinks[iLink].m_ID;

			// Next...
			iLink = m_pLinks[iLink].m_iLinkNextObject;
		}
		return nRet;
	}
}

int CBSP2_LinkContext::EnumPortalLeaves(int _ID, uint16* _piRet, int _MaxRet)
{
	MAUTOSTRIP(CBSP2_LinkContext_EnumPortalLeaves, 0);
	int nRet = 0;
	int iLink = m_pIDLinkMap[_ID];
	while(iLink)
	{
		const CBSP2_Link* pL = &m_pLinks[iLink];
		if (nRet >= _MaxRet)
#ifdef M_RTM
			return nRet;
#else
			Error("EnumPortalLeaves", CStr("Out of enumeration space. (%d)") );
#endif
		_piRet[nRet++] = pL->m_iPortalLeaf;
		iLink = pL->m_iLinkNextPL;
	}
	return nRet;
}

int CBSP2_LinkContext::EnumPortalLeaves_Single(int _ID)
{
	MAUTOSTRIP(CBSP2_LinkContext_EnumPortalLeaves_Single, 0);
	int iLink = m_pIDLinkMap[_ID];
	if (iLink)
		return m_pLinks[iLink].m_iPortalLeaf;
	else
		return -1;
}

void CBSP2_LinkContext::AddPortalLeaf(int _iPL)
{
	MAUTOSTRIP(CBSP2_LinkContext_AddPortalLeaf, MAUTOSTRIP_VOID);
	//MSCOPESHORT(CBSP2_LinkContext::AddPortalLeaf); //AR-SCOPE

#ifndef M_RTM
	if ((_iPL == m_iPLInfinite) && !(m_lIDFlags[m_CurLinkID] & CXR_SCENEGRAPH_NOPORTALPVSCULL))
	{
		M_ASSERT(0, "!");
	}
#endif


	if (m_bPLBlock)
	{
		if (m_lPLBlock[_iPL >> 3] & (aShiftMulTab[_iPL & 7]))
			return;

		m_lPLBlock[_iPL >> 3] |= (aShiftMulTab[_iPL & 7]);
	}


	int iLink = m_iNextLink;
	if (!iLink)
	{
		// Create new link.
		int MaxAvail = m_LinkHeap.MaxAvail();	//JK-NOTE: Do MaxAvail call before alloc to prevent a LHS
		iLink = m_LinkHeap.AllocID();
		m_LinkHeapMinAvail = Min(m_LinkHeapMinAvail, MaxAvail - 1);

		if (iLink < 0)
		{
			//Error("Insert", "Unable to allocate link.");

			// Only report this problem once every second.
			static CMStopTimer g_LastLogTime(1.0f);
			if (g_LastLogTime.Done())
			{
				M_TRACEALWAYS("(CBSP2_LinkContext::AddPortalLeaf) Unable to allocate link. (pLinkCtx %.8x, %d, %d)\n", this, m_MaxID, m_lLinks.Len() );
				ConOutL(CStrF("(CBSP2_LinkContext::AddPortalLeaf) Unable to allocate link. (pLinkCtx %.8x, %d, %d)", this, m_MaxID, m_lLinks.Len() ));
				g_LastLogTime.Reset(1.0f);
			}

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
		CBSP2_Link* pL = &m_pLinks[iLink];
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

void CBSP2_LinkContext::ExpandCurrentBox(const CBox3Dfp32& _Box)
{
	m_lIDLinkBox[m_CurLinkID].Expand(_Box);
}

int CBSP2_LinkContext::GetNumLinkedIDs() const
{
	int nIDs = m_lIDLinkMap.Len();

	int nLinked = 0;
	for(int i = 0; i < nIDs; i++)
		if (m_pIDLinkMap[i])
			nLinked++;

	return nLinked;
}

void CBSP2_LinkContext::TraceContents() const
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
//  CXR_Model_BSP2
// -------------------------------------------------------------------
void CXR_Model_BSP2::LinkBox_r(CBSP2_LinkContext* _pLinkCtx, int _iNode, const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_LinkBox_r, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP2::LinkBox_r); //AR-SCOPE

	if (!_iNode) return;
	const CBSP2_Node* pN = &m_pNodes[_iNode];
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

void CXR_Model_BSP2::LinkBox_i(CBSP2_LinkContext* _pLinkCtx, int _iNode, const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_LinkBox_i, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP2::LinkBox_i); //AR-SCOPE

#ifdef BSP_USE_OCTAAABB

	if( m_lOctaAABBNodes.Len() )
	{
		CBSP_OctaAABBIterator<CXR_Model_BSP2,CBSP2_PortalLeafExt> OctItr(this,NULL,_Box);

		uint32 iNext = OctItr.Next();
		while( iNext != 0xFFFFFFFF )
		{
			_pLinkCtx->AddPortalLeaf(iNext);
			iNext = OctItr.Next();
		}

		return;
	}

#endif

	if (!_iNode) return;

	int aWorkStack[256];
	int iWorkPos = 0;
	aWorkStack[iWorkPos++]	= _iNode;
StartOf_LinkBox_i:
	int iWorkNode = aWorkStack[--iWorkPos];

StartOf_LinkBox_i_NoAdd:
	const CBSP2_Node* pN = &m_pNodes[iWorkNode];
	if (pN->IsStructureLeaf())
	{
		_pLinkCtx->AddPortalLeaf(pN->m_iPortalLeaf);
	}
	else if (pN->IsNode())
	{
		int iBack = pN->m_iNodeBack;
		int iFront = pN->m_iNodeFront;
		fp32 MinDist, MaxDist;
		m_pPlanes[pN->m_iPlane].GetBoxMinAndMaxDistance(_Box.m_Min, _Box.m_Max, MinDist, MaxDist);
		if(iFront && iBack)
		{
			if(MaxDist > LINKBOX_EPSILON)
				aWorkStack[iWorkPos++]	= iFront;
			if(MinDist < -LINKBOX_EPSILON)
			{
				iWorkNode = iBack;
				goto StartOf_LinkBox_i_NoAdd;
			}
		}
		else if(iFront)
		{
			if(MaxDist > LINKBOX_EPSILON)
			{
				iWorkNode = iFront;
				goto StartOf_LinkBox_i_NoAdd;
			}
		}
		else
		{
			if(MinDist < -LINKBOX_EPSILON)
			{
				iWorkNode = iBack;
				goto StartOf_LinkBox_i_NoAdd;
			}
		}
	}

	if( iWorkPos > 0 )
		goto StartOf_LinkBox_i;
}

void CXR_Model_BSP2::LinkBoxWithPVS_r(CBSP2_LinkContext* _pLinkCtx, int _iNode, const CBox3Dfp32& _Box, const uint8* _pPVS)
{
	MAUTOSTRIP(CXR_Model_BSP_LinkBoxWithPVS_r, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP2::LinkBoxWithPVS_r); //AR-SCOPE

	if (!_iNode) return;
	const CBSP2_Node* pN = &m_pNodes[_iNode];
	if (pN->IsStructureLeaf())
	{
		int iPL = pN->m_iPortalLeaf;
		if (_pPVS[iPL >> 3] & (aShiftMulTab[iPL & 7]))
		{
			_pLinkCtx->AddPortalLeaf(iPL);

			CBox3Dfp32 BoxAnd(_Box);
			BoxAnd.And(m_pPortalLeaves[iPL].m_BoundBox);

			_pLinkCtx->ExpandCurrentBox(BoxAnd);
		}
		return;
	}

	if (pN->IsNode())
	{
//LogFile(CStrF("Prnt %d, N %d, ChildF %d ChildB %d", pN->m_iNodeParent, _iNode, pN->m_iNodeFront, pN->m_iNodeBack));
		int iBack = pN->m_iNodeBack;
		if (!iBack)
		{
			int iFront = pN->m_iNodeFront;
			if (m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_Box.m_Min, _Box.m_Max) > LINKBOX_EPSILON)
				LinkBoxWithPVS_r(_pLinkCtx, iFront, _Box,  _pPVS);
		}
		else
		{
			int iFront = pN->m_iNodeFront;
			if (!iFront)
			{
				if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) < -LINKBOX_EPSILON)
					LinkBoxWithPVS_r(_pLinkCtx, iBack, _Box, _pPVS);
			}
			else
			{
				if (m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_Box.m_Min, _Box.m_Max) > LINKBOX_EPSILON)
					LinkBoxWithPVS_r(_pLinkCtx, iFront, _Box, _pPVS);
				if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_Box.m_Min, _Box.m_Max) < -LINKBOX_EPSILON)
					LinkBoxWithPVS_r(_pLinkCtx, iBack, _Box, _pPVS);
			}
		}
	}
}

// #pragma optimize("",off)

void CXR_Model_BSP2::LinkBoxWithPVS_i(CBSP2_LinkContext* _pLinkCtx, int _iNode, const CBox3Dfp32& _Box, const uint8* _pPVS)
{
	MAUTOSTRIP(CXR_Model_BSP_LinkBoxWithPVS_i, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP2::LinkBoxWithPVS_i); //AR-SCOPE

#ifdef BSP_USE_OCTAAABB

	if( m_lOctaAABBNodes.Len() )
	{
		/*
		uint16 t_liPL[256];
		uint16 t_nPL = 0;
		CBSP_OctaAABBIterator<CXR_Model_BSP2,CBSP2_PortalLeafExt> OctItr(this,NULL,_Box);

		CBox3Dfp32 Test1,Test2 = _Box;
		bool bHasFixed = false;

		uint32 iNext = OctItr.Next();
		while( iNext != 0xFFFFFFFF )
		{
			if( _pPVS[iNext >> 3] & (aShiftMulTab[iNext & 7]) )
			{
				t_liPL[t_nPL] = iNext;
				t_nPL ++;

				//_pLinkCtx->AddPortalLeaf(iNext);

				CBox3Dfp32 BoxAnd(_Box);
				BoxAnd.And(m_pPortalLeaves[iNext].m_BoundBox);
				
				if( !bHasFixed ) 
				{
					Test1 = BoxAnd;
					bHasFixed = true;
				}
				else Test1.Expand(BoxAnd);
				
				//_pLinkCtx->ExpandCurrentBox(BoxAnd);
			}
			iNext = OctItr.Next();
		}
		//*/

		CBox3Dfp32 TestBox = _Box;
		uint16 liPL[1024];
		uint32 nPL = BSPOctaAABB_EnumPL(this,liPL,1024,TestBox,_pPVS);
		//_pLinkCtx->ExpandCurrentBox(TestBox);

		for(uint i = 0;i < nPL;i++)
		{
			_pLinkCtx->AddPortalLeaf(liPL[i]);

			CBox3Dfp32 BoxAnd(_Box);
			BoxAnd.And(m_pPortalLeaves[liPL[i]].m_BoundBox);
			_pLinkCtx->ExpandCurrentBox(BoxAnd);
		}

		return;
	}

#endif

	if (!_iNode) return;

	CVec3Dfp32 BoxMin = _Box.m_Min;
	CVec3Dfp32 BoxMax = _Box.m_Max;

	int aWorkStack[256];
	uint32 iWorkPos = 0;
	int iWorkNode = _iNode;
	goto StartOf_LinkBoxWithPVS_i_NoAdd;

StartOf_LinkBoxWithPVS_i:
	iWorkNode = aWorkStack[--iWorkPos];

StartOf_LinkBoxWithPVS_i_NoAdd:

	const CBSP2_Node* pN = &m_pNodes[iWorkNode];
	if (pN->IsStructureLeaf())
	{
		int iPL = pN->m_iPortalLeaf;
		if (_pPVS[iPL >> 3] & (aShiftMulTab[iPL & 7]))
		{
			_pLinkCtx->AddPortalLeaf(iPL);

			CBox3Dfp32 BoxAnd(_Box);
			BoxAnd.And(m_pPortalLeaves[iPL].m_BoundBox);

			_pLinkCtx->ExpandCurrentBox(BoxAnd);
		}
	}
	else if (pN->IsNode())
	{
		int iBack = pN->m_iNodeBack;
		int iFront = pN->m_iNodeFront;
		fp32 MinDist, MaxDist;
		m_pPlanes[pN->m_iPlane].GetBoxMinAndMaxDistance(BoxMin, BoxMax, MinDist, MaxDist);

		if(iFront && iBack)
		{
			uint32 Add = 0;
			if(MaxDist > LINKBOX_EPSILON)
			{
				Add = 1;
				aWorkStack[iWorkPos] = iFront;
				iWorkNode = iFront;
			}

			if(MinDist < -LINKBOX_EPSILON)
			{
				iWorkPos += Add;
				iWorkNode = iBack;
				goto StartOf_LinkBoxWithPVS_i_NoAdd;
			}

			if(Add)
				goto StartOf_LinkBoxWithPVS_i_NoAdd;
		}
		else if(iFront)
		{
			if(MaxDist > LINKBOX_EPSILON)
			{
				iWorkNode = iFront;
				goto StartOf_LinkBoxWithPVS_i_NoAdd;
			}
		}
		else
		{
			if(MinDist < -LINKBOX_EPSILON)
			{
				iWorkNode = iBack;
				goto StartOf_LinkBoxWithPVS_i_NoAdd;
			}
		}
	}

	if( iWorkPos > 0 )
		goto StartOf_LinkBoxWithPVS_i;
}


// -------------------------------------------------------------------

void CXR_Model_BSP2::EnumFaces_Sphere_r(CXR_PhysicsContext* _pPhysContext, CBSP2_EnumContext* _pEnumContext, int _iNode) const
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_Sphere_r, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_BSP2::EnumFaces_Sphere_r); //AR-SCOPE

	if (!_iNode) return;
	const CBSP2_Node* pN = &m_lNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		_pEnumContext->m_EnumPL = pN->m_iPortalLeaf;
	}

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;
//		uint8* pFTag = _pEnumContext->m_pFTag;
//		uint32* piFUntag = _pEnumContext->m_piFUntag;

		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_piFaces[iiFaces + f];
			int iFaceIdx = iFace >> 3;
			int iFaceMask = aShiftMulTab[iFace & 7];
			if (_pEnumContext->m_pFTag[iFaceIdx] & iFaceMask) continue;
			if (!_pEnumContext->m_pFTag[iFaceIdx])
			{
				if(_pEnumContext->m_nUntagEnum >= _pEnumContext->m_MaxUntagEnum)
					M_ASSERT(0, "?");
				_pEnumContext->m_piFUntag[_pEnumContext->m_nUntagEnum++] = iFaceIdx;
			}
			_pEnumContext->m_pFTag[iFaceIdx] |= iFaceMask;

			const CBSP2_Face* pF = &m_pFaces[iFace];

			// Check face flag?
			if (_pEnumContext->m_EnumQuality & ENUM_FACEFLAGS && !(_pEnumContext->m_EnumFaceFlags & pF->m_Flags)) continue;

			// Check medium
			if (_pEnumContext->m_EnumQuality & ENUM_MEDIUMFLAGS && !(m_lMediums[pF->m_iBackMedium].m_MediumFlags & _pEnumContext->m_EnumMedium)) continue;

			int iPlane = pF->m_iPlane;
			fp32 Dist = m_pPlanes[iPlane].Distance(_pEnumContext->m_EnumSphere);
			if (Abs(Dist) < _pEnumContext->m_EnumSphereR)
			{
				if (_pEnumContext->m_nEnum < _pEnumContext->m_MaxEnum)
				{
					_pEnumContext->m_piEnum[_pEnumContext->m_nEnum] = iFace;
					if (_pEnumContext->m_piEnumPL) _pEnumContext->m_piEnumPL[_pEnumContext->m_nEnum] = _pEnumContext->m_EnumPL;
					_pEnumContext->m_nEnum++;
				}
				else
				{
					_pEnumContext->m_bEnumError = true;
					return;
				}
			}
		}
	}
	else if (pN->IsNode())
	{
		fp32 Dist = m_pPlanes[pN->m_iPlane].Distance(_pEnumContext->m_EnumSphere);
		if (Dist < _pEnumContext->m_EnumSphereR)
			EnumFaces_Sphere_r(_pPhysContext, _pEnumContext, pN->m_iNodeBack);
		if (Dist > -_pEnumContext->m_EnumSphereR)
			EnumFaces_Sphere_r(_pPhysContext, _pEnumContext, pN->m_iNodeFront);
	}
}

void CXR_Model_BSP2::EnumFaces_Box_r(CXR_PhysicsContext* _pPhysContext, CBSP2_EnumContext* _pEnumContext, int _iNode) const
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_Box_r, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	const CBSP2_Node* pN = &m_lNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		_pEnumContext->m_EnumPL = pN->m_iPortalLeaf;
	}

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;
		uint8* pFTag = _pEnumContext->m_pFTag;
		uint32* piFUntag = _pEnumContext->m_piFUntag;

		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_piFaces[iiFaces + f];
			int iFaceIdx = iFace >> 3;
			int iFaceMask = aShiftMulTab[iFace & 7];
			if (pFTag[iFaceIdx] & iFaceMask) continue;
			if (!pFTag[iFaceIdx])
			{
				if(_pEnumContext->m_nUntagEnum >= _pEnumContext->m_MaxUntagEnum)
					M_ASSERT(0, "?");
				piFUntag[_pEnumContext->m_nUntagEnum++] = iFaceIdx;
			}
			pFTag[iFaceIdx] |= iFaceMask;

			if (_pPhysContext->m_pWC) 
				__Phys_RenderFace(iFace, _pPhysContext->m_WMat, _pPhysContext->m_pWC, 0xff100020);

			const CBSP2_Face* pF = &m_pFaces[iFace];
			int iPlane = pF->m_iPlane;
			fp32 MinDist = m_pPlanes[iPlane].GetBoxMinDistance(_pEnumContext->m_EnumBox.m_Min, _pEnumContext->m_EnumBox.m_Max);
			if (MinDist > MODEL_BSP_EPSILON) continue;

			// Check face flag?
			if (_pEnumContext->m_EnumQuality & ENUM_FACEFLAGS && !(_pEnumContext->m_EnumFaceFlags & pF->m_Flags)) continue;

			// Check medium
			if (_pEnumContext->m_EnumQuality & ENUM_MEDIUMFLAGS && !(m_lMediums[pF->m_iBackMedium].m_MediumFlags & _pEnumContext->m_EnumMedium)) continue;

// Box can't be behind face.
//			fp32 MaxDist = m_pPlanes[iPlane].GetBoxMaxDistance(ms_EnumBox.m_Min, ms_EnumBox.m_Max);
//			if (MaxDist < -MODEL_BSP_EPSILON) continue;

			// Check face bounding box
			if (_pEnumContext->m_EnumQuality & ENUM_HQ)
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

					if (_pEnumContext->m_EnumBox.m_Min[k] > PMax)
						goto Continue;
					if (_pEnumContext->m_EnumBox.m_Max[k] < PMin)
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
				if (!PBox.IsInside(_pEnumContext->m_EnumBox)) continue;
#endif
			}

			if (_pEnumContext->m_nEnum < _pEnumContext->m_MaxEnum)
			{
				_pEnumContext->m_piEnum[_pEnumContext->m_nEnum] = iFace;
				if (_pEnumContext->m_piEnumPL) _pEnumContext->m_piEnumPL[_pEnumContext->m_nEnum] = _pEnumContext->m_EnumPL;
				_pEnumContext->m_nEnum++;
			}
			else
			{
				_pEnumContext->m_bEnumError = true;
				return;
			}
Continue: ;
		}
	}
	else if (pN->IsNode())
	{
		fp32 MinDist = m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_pEnumContext->m_EnumBox.m_Min, _pEnumContext->m_EnumBox.m_Max);
		fp32 MaxDist = m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_pEnumContext->m_EnumBox.m_Min, _pEnumContext->m_EnumBox.m_Max);

		if (MinDist < MODEL_BSP_EPSILON)
			EnumFaces_Box_r(_pPhysContext, _pEnumContext, pN->m_iNodeBack);
		if (MaxDist > -MODEL_BSP_EPSILON)
			EnumFaces_Box_r(_pPhysContext, _pEnumContext, pN->m_iNodeFront);
	}
}

void CXR_Model_BSP2::EnumFaces_All_r(CXR_PhysicsContext* _pPhysContext, CBSP2_EnumContext* _pEnumContext, int _iNode) const
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_All_r, MAUTOSTRIP_VOID);

	if (!_iNode) return;
	const CBSP2_Node* pN = &m_lNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		_pEnumContext->m_EnumPL = pN->m_iPortalLeaf;
	}

	if (pN->IsLeaf())
	{
		int nFaces = pN->m_nFaces;
		int iiFaces = pN->m_iiFaces;
		uint8* pFTag = _pEnumContext->m_pFTag;
		uint32* piFUntag = _pEnumContext->m_piFUntag;

		for(int f = 0; f < nFaces; f++)
		{
			int iFace = m_piFaces[iiFaces + f];
			int iFaceIdx = iFace >> 3;
			int iFaceMask = aShiftMulTab[iFace & 7];
			if (pFTag[iFaceIdx] & iFaceMask) continue;
			if (!pFTag[iFaceIdx])
			{
				if(_pEnumContext->m_nUntagEnum >= _pEnumContext->m_MaxUntagEnum)
					M_ASSERT(0, "?");
				piFUntag[_pEnumContext->m_nUntagEnum++] = iFaceIdx;
			}
			pFTag[iFaceIdx] |= iFaceMask;

			if (_pEnumContext->m_nEnum < _pEnumContext->m_MaxEnum)
			{
				_pEnumContext->m_piEnum[_pEnumContext->m_nEnum] = iFace;
				if (_pEnumContext->m_piEnumPL) _pEnumContext->m_piEnumPL[_pEnumContext->m_nEnum] = _pEnumContext->m_EnumPL;
				_pEnumContext->m_nEnum++;
			}
			else
			{
				_pEnumContext->m_bEnumError = true;
				return;
			}
		}
	}
	else if (pN->IsNode())
	{
		EnumFaces_All_r(_pPhysContext, _pEnumContext, pN->m_iNodeBack);
		EnumFaces_All_r(_pPhysContext, _pEnumContext, pN->m_iNodeFront);
	}
}

int CXR_Model_BSP2::EnumFaces_Sphere(CXR_PhysicsContext* _pPhysContext, CBSP2_EnumContext* _pEnumContext, int _iNode) const
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_Sphere, 0);

	if( m_lOctaAABBNodes.Len() )
	{
		BSPOctaAABB_EnumFaces<CXR_Model_BSP2,CBSP2_EnumContext,CBSP2_PortalLeafExt,CBSP2_Face>
			(this,_pPhysContext,m_lPortalLeaves.GetBasePtr(),_pEnumContext,true);
	}
	else
		EnumFaces_Sphere_r(_pPhysContext, _pEnumContext, _iNode);
	return _pEnumContext->m_nEnum;
}

int CXR_Model_BSP2::EnumFaces_Box(CXR_PhysicsContext* _pPhysContext, CBSP2_EnumContext* _pEnumContext, int _iNode) const
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_Box, 0);

	if( m_lOctaAABBNodes.Len() )
	{
		BSPOctaAABB_EnumFaces<CXR_Model_BSP2,CBSP2_EnumContext,CBSP2_PortalLeafExt,CBSP2_Face>
			(this,_pPhysContext,m_lPortalLeaves.GetBasePtr(),_pEnumContext,false);	
	}
	else 
		EnumFaces_Box_r(_pPhysContext, _pEnumContext, _iNode);
	return _pEnumContext->m_nEnum;
}

int CXR_Model_BSP2::EnumFaces_All(CXR_PhysicsContext* _pPhysContext, CBSP2_EnumContext* _pEnumContext, int _iNode) const
{
	MAUTOSTRIP(CXR_Model_BSP_EnumFaces_All, 0);
	MSCOPESHORT(CXR_Model_BSP2::EnumFaces_All);

	EnumFaces_All_r(_pPhysContext, _pEnumContext, _iNode);
	return _pEnumContext->m_nEnum;
}


// -------------------------------------------------------------------
int CXR_Model_BSP2::CountFaces_r(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_CountFaces_r, 0);
	MSCOPESHORT(CXR_Model_BSP2::CountFaces_r); //AR-SCOPE

	CBSP2_Node* pN = &m_lNodes[_iNode];
	if (pN->IsLeaf())
		return pN->m_nFaces;
	else
		return CountFaces_r(pN->m_iNodeFront) + CountFaces_r(pN->m_iNodeBack);
}

void CXR_Model_BSP2::InitFaces()
{
	MAUTOSTRIP(CXR_Model_BSP_InitFaces, MAUTOSTRIP_VOID);
}

aint CXR_Model_BSP2::GetParam(int _Param)
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

	case CXR_MODEL_PARAM_ISSHADOWCASTER :
		{
			for(int i = 0; i < m_lspSurfaces.Len(); i++)
			{
				CXW_Surface* pSurf = m_lspSurfaces[i];
				if (!(pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE) &&
					!(pSurf->GetBaseFrame()->m_Medium.m_MediumFlags & XW_MEDIUM_SEETHROUGH))
					return 1;
			}
			return 0;
		}

	case CXR_MODEL_PARAM_LIGHTVOLUME :
		{
			return (aint)(CXR_LightVolume*)m_spLightFieldOcttree;
		}

	default :
		return CXR_Model::GetParam(_Param);
	}
}

void CXR_Model_BSP2::SetParam(int _Param, aint _Value)
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

int CXR_Model_BSP2::GetParamfv(int _Param, fp32* _pRetValues)
{
	MAUTOSTRIP(CXR_Model_BSP_GetParamfv, 0);
	return CXR_Model::GetParamfv(_Param, _pRetValues);
}

void CXR_Model_BSP2::SetParamfv(int _Param, const fp32* _pValues)
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

fp32 CXR_Model_BSP2::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_BSP_GetBound_Sphere, 0.0f);
	return m_BoundRadius;
}

void CXR_Model_BSP2::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_BSP_GetBound_Box, MAUTOSTRIP_VOID);
	_Box = m_BoundBox;
}

void CXR_Model_BSP2::GetBound_Box(CBox3Dfp32& _Box, int _Mask, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_BSP_GetBound_Box, MAUTOSTRIP_VOID);
	CXR_IndexedSolidContainer32 *pSolidContainer = m_spIndexedSolids;
	if (pSolidContainer == NULL)
	{
		_Box = m_BoundBox;
	}
	else
	{
		vec128 invalid = M_VNegOne_i32();
		vec128 BoxMin = M_VScalar(1000000000.0f);
		vec128 BoxMax = M_VScalar(-1000000000.0f);
		vec128 zero = M_VZero();
		vec128 one = M_VOne_u32();
		vec128 PhysGroupAndMask = M_VScalar_u32(0x0000ffff);
		vec128 InputMask = M_VLdScalar_i32(_Mask);

		TAP<const CXR_IndexedSolid32> pSolids = pSolidContainer->m_lSolids;
		TAP<const CXR_MediumDesc> pMediums = pSolidContainer->m_lMediums;

		int nSolids = 0;

		for (int i = 0; i < pSolids.Len(); i++)
		{
			uint iMedium = pSolids[i].m_iMedium;
			vec128 PhysGroupMask = M_VShl_u32(one, M_VAnd(M_VLdScalar_i16(pMediums[iMedium].m_iPhysGroup), PhysGroupAndMask));

			// selmask is zero if the requested bit is set
			vec128 maskmask = M_VAnd(PhysGroupMask, InputMask);
			vec128 selmask = M_VCmpEqMsk_u32(maskmask, zero);

			vec128 BBoxMin, BBoxMax;
			M_VLdU_V3x2_Slow(&pSolids[i].m_BoundBox, BBoxMin, BBoxMax);
			BoxMin = M_VSelMsk(selmask, BoxMin, M_VMin(BoxMin, BBoxMin));
			BoxMax = M_VSelMsk(selmask, BoxMax, M_VMax(BoxMax, BBoxMax));

			// Make sure invalid is updated properly
			invalid = M_VSelMsk(selmask, invalid, M_VSelMsk(invalid, zero, invalid));
		}
		BoxMin = M_VSelMsk(invalid, zero, BoxMin);
		BoxMax = M_VSelMsk(invalid, zero, BoxMax);
		M_VStU_V3x2_Slow(BoxMin, BoxMax, &_Box);
	}
}

int CXR_Model_BSP2::NumFacesInTree(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_NumFacesInTree, 0);
	if (!_iNode) return 0;
	if (m_pNodes[_iNode].IsLeaf()) return m_pNodes[_iNode].m_nFaces;

	return NumFacesInTree(m_pNodes[_iNode].m_iNodeFront) + NumFacesInTree(m_pNodes[_iNode].m_iNodeBack);
}

int CXR_Model_BSP2::GetNumEnabledLeaves(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_GetNumEnabledLeaves, 0);
	if (!_iNode) return 0;
	if (!(m_pNodes[_iNode].m_Flags & XW_NODE_ENABLE)) return 0;

	if (m_pNodes[_iNode].IsNode())
		return GetNumEnabledLeaves(m_pNodes[_iNode].m_iNodeFront) + GetNumEnabledLeaves(m_pNodes[_iNode].m_iNodeBack);
	else
		return 1;
}

int CXR_Model_BSP2::GetFirstSplitNode_Sphere(const CVec3Dfp32& _Pos, fp32 _Radius)
{
	MAUTOSTRIP(CXR_Model_BSP_GetFirstSplitNode_Sphere, 0);
	int iNode = 1;
	while(1)
	{
		int iPlane = m_pNodes[iNode].m_iPlane;
		if (!iPlane) return iNode;
		const CPlane3Dfp32* pP = &m_pPlanes[iPlane];

		fp32 d = pP->Distance(_Pos);
		if ((d < _Radius) && (d > -_Radius)) return iNode;
		iNode = (d > 0) ? m_pNodes[iNode].m_iNodeFront : m_pNodes[iNode].m_iNodeBack;
		if (!iNode) return 0;
	}
}

int CXR_Model_BSP2::GetFirstSplitNode_Box(const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax)
{
	MAUTOSTRIP(CXR_Model_BSP_GetFirstSplitNode_Box, 0);
	Error("GetFirstSplitNode_Box", "FIXME! I'm broken!");
	int iNode = 1;

	int SideMask = 0;
	do
	{
		int iPlane = m_pNodes[iNode].m_iPlane;
		if (!iPlane) return iNode;
		const CPlane3Dfp32* pP = &m_pPlanes[iPlane];

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

int CXR_Model_BSP2::GetFirstSplitNode_Polyhedron(const CVec3Dfp32* _pV, int _nVertices)
{
	MAUTOSTRIP(CXR_Model_BSP_GetFirstSplitNode_Polyhedron, 0);
	int iNode = 1;
	do
	{
		int iPlane = m_pNodes[iNode].m_iPlane;
		if (!iPlane) return iNode;
		const CPlane3Dfp32* pP = &m_pPlanes[iPlane];

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

int CXR_Model_BSP2::Structure_GetFirstSplitNode_Polyhedron(const CVec3Dfp32* _pV, int _nVertices)
{
	MAUTOSTRIP(CXR_Model_BSP_Structure_GetFirstSplitNode_Polyhedron, 0);
	int iNode = 1;
	do
	{
		const CBSP2_Node* pN = &m_pNodes[iNode];
		if ((pN->m_Flags & (XW_NODE_PORTAL | XW_NODE_STRUCTURE)) == (XW_NODE_PORTAL | XW_NODE_STRUCTURE)) return iNode;
		int iPlane = m_pNodes[iNode].m_iPlane;
		if (!iPlane) return iNode;
		const CPlane3Dfp32* pP = &m_pPlanes[iPlane];

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

void CXR_Model_BSP2::EnableTreeFromNode_Sphere(int _iNode, const CVec3Dfp32& _Pos, fp32 _Radius)
{
	MAUTOSTRIP(CXR_Model_BSP_EnableTreeFromNode_Sphere, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	m_pNodes[_iNode].m_Flags |= XW_NODE_ENABLE;

	int iPlane = m_pNodes[_iNode].m_iPlane;
	if (!iPlane) return;
	const CPlane3Dfp32* pP = &m_pPlanes[iPlane];
	fp32 d = pP->Distance(_Pos);
	if (d > -_Radius) 
		EnableTreeFromNode_Sphere(m_pNodes[_iNode].m_iNodeFront, _Pos, _Radius);
	if (d < _Radius) 
		EnableTreeFromNode_Sphere(m_pNodes[_iNode].m_iNodeBack, _Pos, _Radius);
}

void CXR_Model_BSP2::EnableTreeFromNode_Box(int _iNode, const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax)
{
	MAUTOSTRIP(CXR_Model_BSP_EnableTreeFromNode_Box, MAUTOSTRIP_VOID);
	Error("EnableTreeFromNode_Box", "FIXME! I'm broken!");

	if (!_iNode) return;
	m_pNodes[_iNode].m_Flags |= XW_NODE_ENABLE;

	int iPlane = m_pNodes[_iNode].m_iPlane;
	if (!iPlane) return;
	const CPlane3Dfp32* pP = &m_pPlanes[iPlane];

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

void CXR_Model_BSP2::EnableTreeFromNode_Polyhedron(int _iNode, const CVec3Dfp32* _pV, int _nVertices)
{
	MAUTOSTRIP(CXR_Model_BSP_EnableTreeFromNode_Polyhedron, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	m_pNodes[_iNode].m_Flags |= XW_NODE_ENABLE;

	int iPlane = m_pNodes[_iNode].m_iPlane;
	if (!iPlane) return;
	const CPlane3Dfp32* pP = &m_pPlanes[iPlane];

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

void CXR_Model_BSP2::EnableTreeFromNode(int _iNode)
{
	MAUTOSTRIP(CXR_Model_BSP_EnableTreeFromNode, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	while ((_iNode) && !(m_pNodes[_iNode].m_Flags & XW_NODE_ENABLE))
	{
		m_pNodes[_iNode].m_Flags |= XW_NODE_ENABLE;
		_iNode = m_pNodes[_iNode].m_iNodeParent;
	}
}

void CXR_Model_BSP2::EnableTreeFlagFromNode(int _iNode, int _Flag)
{
	MAUTOSTRIP(CXR_Model_BSP_EnableTreeFlagFromNode, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	while ((_iNode) && !(m_pNodes[_iNode].m_Flags & _Flag))
	{
		m_pNodes[_iNode].m_Flags |= _Flag;
		_iNode = m_pNodes[_iNode].m_iNodeParent;
	}
}

void CXR_Model_BSP2::EnableWithPortalFloodFill(int _iNode, int _EnableFlag, int _MediumFlag)
{
	MAUTOSTRIP(CXR_Model_BSP_EnableWithPortalFloodFill, MAUTOSTRIP_VOID);
	CBSP2_Node* pN = &m_pNodes[_iNode];
	if (pN->m_Flags & _EnableFlag) return;

	if (!pN->IsStructureLeaf())
	{
		ConOut(CStrF("(CXR_Model_BSP2::EnableWithPortalFloodFill) Node %d was not a structure-leaf.", _iNode));
		return;
	}
	const CBSP2_PortalLeaf* pPL = &m_lPortalLeaves[pN->m_iPortalLeaf];
	if (!(pPL->m_ContainsMedium & _MediumFlag)) return;

	pN->m_Flags |= _EnableFlag;

	for(int p = 0; p < pPL->m_nPortals; p++)
	{
		int iP = m_liPortals[pPL->m_iiPortals + p];
		const CBSP2_Portal* pP = &m_pPortals[iP];
		EnableWithPortalFloodFill((pP->m_iNodeFront == _iNode) ? pP->m_iNodeBack : pP->m_iNodeFront, _EnableFlag, _MediumFlag);
	}
}

void CXR_Model_BSP2::DisableWithPortalFloodFill(int _iNode, int _EnableFlag)
{
	MAUTOSTRIP(CXR_Model_BSP_DisableWithPortalFloodFill, MAUTOSTRIP_VOID);
	CBSP2_Node* pN = &m_pNodes[_iNode];
	if (!(pN->m_Flags & _EnableFlag)) return;

	if (!pN->IsStructureLeaf())
	{
		ConOut(CStrF("(CXR_Model_BSP2::EnableWithPortalFloodFill) Node %d was not a structure-leaf.", _iNode));
		return;
	}
	const CBSP2_PortalLeaf* pPL = &m_lPortalLeaves[pN->m_iPortalLeaf];
	pN->m_Flags &= ~_EnableFlag;

	for(int p = 0; p < pPL->m_nPortals; p++)
	{
		int iP = m_liPortals[pPL->m_iiPortals + p];
		const CBSP2_Portal* pP = &m_pPortals[iP];
		DisableWithPortalFloodFill((pP->m_iNodeFront == _iNode) ? pP->m_iNodeBack : pP->m_iNodeFront, _EnableFlag);
	}
}

void CXR_Model_BSP2::Structure_EnableTreeFromNode_Polyhedron(int _iNode, const CVec3Dfp32* _pV, int _nVertices)
{
	MAUTOSTRIP(CXR_Model_BSP_Structure_EnableTreeFromNode_Polyhedron, MAUTOSTRIP_VOID);
	if (!_iNode) return;
	CBSP2_Node* pN = &m_pNodes[_iNode];
	pN->m_Flags |= XW_NODE_ENABLE;

	if ((pN->m_Flags & (XW_NODE_PORTAL | XW_NODE_STRUCTURE)) == (XW_NODE_PORTAL | XW_NODE_STRUCTURE)) return;
	int iPlane = m_pNodes[_iNode].m_iPlane;
	if (!iPlane) return;
	const CPlane3Dfp32* pP = &m_pPlanes[iPlane];

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
/*
void CXR_Model_BSP2::InitBoundNodesFromEnabledNodes(int _iNode, int _iNodeLastSplit, int _iNodeLastSplitSide)
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

void CXR_Model_BSP2::Structure_InitBoundNodesFromEnabledNodes(int _iNode, int _iNodeLastSplit, int _iNodeLastSplitSide)
{
	MAUTOSTRIP(CXR_Model_BSP_Structure_InitBoundNodesFromEnabledNodes, MAUTOSTRIP_VOID);
	CBSP2_Node* pN = &m_pNodes[_iNode];

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
*/
void CXR_Model_BSP2::DisableTree(int _iNode)
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

void CXR_Model_BSP2::DisableTreeFlag(int _iNode, int _Flag)
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

bool CXR_Model_BSP2::CheckTree_r(int _iNode, int _iParent, int _PortalDone)
{
	MAUTOSTRIP(CXR_Model_BSP_CheckTree_r, false);
	MSCOPESHORT(CXR_Model_BSP2::CheckTree_r); //AR-SCOPE

	if (!_iNode) return true;
	CBSP2_Node* pN = &m_lNodes[_iNode];

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
/*		if (pN->m_Flags & XW_NODE_NAVIGATION)
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
int CXR_Model_BSP2::GetMedium(const CVec3Dfp32& _v0)
{
	MAUTOSTRIP(CXR_Model_BSP_Phys_GetMedium, 0);

	CVec3Dfp32 v0;
	int iLeaf = GetLeaf(_v0);
	if (!iLeaf) return XW_MEDIUM_SOLID;
	return m_lMediums[m_lNodes[iLeaf].m_iMedium].m_MediumFlags;
}

int CXR_Model_BSP2::GetLeaf(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CXR_Model_BSP_GetLeaf, 0);
	int iNode = 1;
	const CBSP2_Node* plNodes = m_lNodes.GetBasePtr();
	const CPlane3Dfp32* plPlanes = m_lPlanes.GetBasePtr();
	while(plNodes[iNode].IsNode())
	{
		int iPlane = plNodes[iNode].m_iPlane;
		int VSide = plPlanes[iPlane].GetPlaneSide(_v);
		iNode = (VSide >= 0) ? plNodes[iNode].m_iNodeFront : plNodes[iNode].m_iNodeBack;
	}
	return iNode;
}

int CXR_Model_BSP2::GetPortalLeaf(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CXR_Model_BSP_GetPortalLeaf, 0);
	int iNode = 1;
	const CBSP2_Node* plNodes = m_lNodes.GetBasePtr();
	const CPlane3Dfp32* plPlanes = m_lPlanes.GetBasePtr();
	while(!(plNodes[iNode].m_Flags & XW_NODE_PORTAL))
	{
		if (plNodes[iNode].IsLeaf()) return -1;
		int iPlane = plNodes[iNode].m_iPlane;
		int VSide = plPlanes[iPlane].GetPlaneSide(_v);
		iNode = (VSide >= 0) ? plNodes[iNode].m_iNodeFront : plNodes[iNode].m_iNodeBack;
	}
	return iNode;
}

int CXR_Model_BSP2::GetStructurePortalLeaf(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CXR_Model_BSP_GetStructurePortalLeaf, 0);
	int iNode = 1;
	const CBSP2_Node* plNodes = m_lNodes.GetBasePtr();
	const CPlane3Dfp32* plPlanes = m_lPlanes.GetBasePtr();
	while(iNode)
	{
		const CBSP2_Node* pN = &plNodes[iNode];
		if (pN->IsStructureLeaf()) return pN->m_iPortalLeaf;
		if (pN->IsLeaf()) return -1;
		int iPlane = pN->m_iPlane;
		int VSide = plPlanes[iPlane].GetPlaneSide(_v);
		iNode = (VSide >= 0) ? pN->m_iNodeFront : pN->m_iNodeBack;
	}
	return -1;
}

/*int CXR_Model_BSP2::GetNavigationPortalLeaf(const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CXR_Model_BSP_GetNavigationPortalLeaf, NULL);
	int iNode = 1;
	while(iNode)
	{
		const CBSP2_Node* pN = &m_lNodes[iNode];
		if (pN->IsNavigationLeaf()) return pN->m_iNavigationLeaf;
		if (pN->IsLeaf()) return -1;
		int iPlane = pN->m_iPlane;
		int VSide = m_lPlanes[iPlane].GetPlaneSide(_v);
		iNode = (VSide >= 0) ? pN->m_iNodeFront : pN->m_iNodeBack;
	}
	return -1;
}*/

CStr CXR_Model_BSP2::GetInfo()
{
	MAUTOSTRIP(CXR_Model_BSP_GetInfo, CStr());
#ifdef M_Profile
	return CStr(TString("All: ", m_Time) + TString(",  Leaves: ", m_TimeRenderLeafList-m_TimeRenderPoly1) + TString(",  Queue: ", m_TimeRenderFaceQueue-m_TimeRenderPoly2));
#else
	return "Profiling turned off";
#endif
}

void CXR_Model_BSP2::InitializeListPtrs()
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
	m_nVVertTag = 0;
	m_nFogTags = 0;

//	m_pLightVerticesInfo = (m_lLightVerticesInfo.Len()) ? &m_lLightVerticesInfo[0] : NULL;
//	m_pLightVertices = (m_lLightVertices.Len()) ? &m_lLightVertices[0] : NULL;
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

