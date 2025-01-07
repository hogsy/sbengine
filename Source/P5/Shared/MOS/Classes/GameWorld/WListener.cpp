/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WListener.cpp

	Author:			Anton Ragnarsson

	Copyright:		Copyright Starbreeze AB 2006

	Contents:		CWO_LinkContext

	History:		
		060601:		Created File
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WListener.h"


#ifndef M_RTM
	M_INLINE void DBG_ClearLink(CWO_LinkContext::CDualLink& _Link) { memset(&_Link, ~0, sizeof(CWO_LinkContext::CDualLink)); }
	M_INLINE bool DBG_IsCleared(const CWO_LinkContext::CDualLink& _Link)
	{ 
		const uint8* pData = (const uint8*)&_Link;
		for (uint i = 0; i < sizeof(CWO_LinkContext::CDualLink); i++)
			if (pData[i] != uint8(~0)) return false;
		return true;
	}
#else
	#define DBG_ClearLink(_Link) {}
	#define DBG_IsCleared(_Link) true
#endif



void CWO_LinkContext::Create(uint _nMaxIDs, uint _nMaxLinks)
{
	m_LinkHeap.Create(_nMaxLinks);
	m_LinkHeap.ForceAllocID(0);	// dummy entry (0 = invalid index)
	m_lLinks.SetLen(_nMaxLinks);
	m_pLinks = m_lLinks;
	for (uint i = 0; i < _nMaxLinks; i++)
		DBG_ClearLink(m_pLinks[i]);

	for (uint i = 0; i < 2; i++)
	{
		m_liHash[i].SetLen(_nMaxIDs);
		memset(m_liHash[i].GetBasePtr(), 0, m_liHash[i].ListSize());
		m_piHash[i] = m_liHash[i];
	}
}


void CWO_LinkContext::Destroy()
{
	m_LinkHeap.Create(0);
	m_lLinks.SetLen(0);
	m_liHash[0].SetLen(0);
	m_liHash[1].SetLen(0);
}


void CWO_LinkContext::RemoveLink(uint _iLink)
{
	CDualLink& DL = m_pLinks[_iLink];

	for (uint iDir = 0; iDir < 2; iDir++)
	{
		CLink& L = DL.m_Link[iDir];
		if (L.m_iPrev) m_pLinks[L.m_iPrev].m_Link[iDir].m_iNext = L.m_iNext;
		if (L.m_iNext) m_pLinks[L.m_iNext].m_Link[iDir].m_iPrev = L.m_iPrev;

		uint ID = DL.m_Link[iDir ^ 1].m_ID;
		if (m_piHash[iDir][ID] == _iLink)
		{
			M_ASSERT(L.m_iPrev == 0, "first link can't have prev!");
			m_piHash[iDir][ID] = L.m_iNext;
		}
	}
	DBG_ClearLink(DL);
	m_LinkHeap.FreeID(_iLink);
}


void CWO_LinkContext::RemoveElement(uint _ID)
{
	// remove from both lists
	for (uint iDir = 0; iDir < 2; iDir++)
	{
		uint iCurr = m_piHash[iDir][_ID];
		while (iCurr)
		{
			M_ASSERT(m_pLinks[iCurr].m_Link[iDir ^ 1].m_ID == _ID, "CWO_LinkContext::RemoveElement, internal error 1");
			M_ASSERT(m_piHash[iDir][_ID] == iCurr || iDir == 1, "CWO_LinkContext::RemoveElement, internal error 2");
			uint iNext = m_pLinks[iCurr].m_Link[iDir].m_iNext;
			RemoveLink(iCurr);
			iCurr = iNext;
		}
	}
	M_ASSERT(m_piHash[0][_ID] == 0, "CWO_LinkContext::RemoveElement, internal error 3");
}


uint CWO_LinkContext::InsertNewLink(uint _ID1, uint _ID2, uint16 _UserMask)
{
	uint iLink = m_LinkHeap.AllocID();
	uint aID[2] = { _ID1, _ID2 };
	CDualLink& DL = m_pLinks[iLink];
	M_ASSERT(DBG_IsCleared(DL), "newly allocated link is not marked as unused!");
	DL.m_UserMask = _UserMask;

	for (uint iDir = 0; iDir < 2; iDir++)
	{
		uint ID = aID[iDir];
		DL.m_Link[iDir ^ 1].m_ID = ID;

		uint iFirst = m_piHash[iDir][ID];
		CLink& L = DL.m_Link[iDir];
		L.m_iPrev = 0;
		L.m_iNext = iFirst;
		if (iFirst)
			m_pLinks[iFirst].m_Link[iDir].m_iPrev = iLink;
		m_piHash[iDir][ID] = iLink;
	}
	return iLink;
}


uint CWO_LinkContext::FindLink(uint _ID1, uint _ID2) const
{
	uint iCurr = m_piHash[0][_ID1];
	while (iCurr)
	{
		const CLink& L = m_pLinks[iCurr].m_Link[0];
		M_ASSERT(m_pLinks[iCurr].m_Link[1].m_ID == _ID1, "CWO_LinkContext::FindLink, internal error!");
		if (L.m_ID == _ID2)
			break;
		iCurr = L.m_iNext;
	}
	return iCurr;
}


void CWO_LinkContext::AddOrUpdateLink(uint _ID1, uint _ID2, uint16 _UserMask)
{
	uint iLink = FindLink(_ID1, _ID2);
	if (iLink)
		m_pLinks[iLink].m_UserMask |= _UserMask;
	else
		InsertNewLink(_ID1, _ID2, _UserMask);
}


void CWO_LinkContext::RemoveOrUpdateLink(uint _ID1, uint _ID2, uint16 _UserMaskToRemove)
{
	uint iLink = FindLink(_ID1, _ID2);
	if (iLink)
	{
		CDualLink& DL = m_pLinks[iLink];
		uint NewMask = DL.m_UserMask & ~_UserMaskToRemove;
		if (NewMask)
			DL.m_UserMask = NewMask;
		else
			RemoveLink(iLink);
	}
}


uint CWO_LinkContext::EnumLinks(uint _ID, uint _iDir, uint* _piRet, uint _nMaxRet, uint16 _Mask) const
{
	uint iCurr = m_piHash[_iDir][_ID];
	uint nRet = 0;
	while (iCurr && (nRet < _nMaxRet))
	{
		const CDualLink& DL = m_pLinks[iCurr];
		M_ASSERT(DL.m_Link[_iDir ^ 1].m_ID == _ID, "CWO_LinkContext::EnumLinks, internal error!");
		if (DL.m_UserMask & _Mask)
			_piRet[nRet++] = DL.m_Link[_iDir].m_ID;
		iCurr = DL.m_Link[_iDir].m_iNext;
	}
	return nRet;
}


// Same as above, but for 16-bit indices (for compatibility with CSelection)
uint CWO_LinkContext::EnumLinks(uint _ID, uint _iDir, uint16* _piRet, uint _nMaxRet, uint16 _Mask) const
{
	uint iCurr = m_piHash[_iDir][_ID];
	uint nRet = 0;
	while (iCurr && (nRet < _nMaxRet))
	{
		const CDualLink& DL = m_pLinks[iCurr];
		M_ASSERT(DL.m_Link[_iDir ^ 1].m_ID == _ID, "CWO_LinkContext::EnumLinks, internal error!");
		if (DL.m_UserMask & _Mask)
			_piRet[nRet++] = DL.m_Link[_iDir].m_ID;
		iCurr = DL.m_Link[_iDir].m_iNext;
	}
	return nRet;
}

