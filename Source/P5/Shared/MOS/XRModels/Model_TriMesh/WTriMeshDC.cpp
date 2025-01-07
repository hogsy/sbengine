
#include "PCH.h"
#include "WTriMeshDC.h"

MRTC_IMPLEMENT_DYNAMIC(CXR_TriangleMeshDecalContainer, CReferenceCount);

void CXR_TriangleMeshDecalContainer::LinkDecal(int _iDecal)
{
	CXR_TMDC_Decal* pD = m_lDecals.GetBasePtr();
	int iFirst = m_GUIDMap.GetIndex(pD[_iDecal].m_GUID);
	if (iFirst == -1)
	{
		// No decals exist for this GUID, so just add to GUID map.
		m_GUIDMap.Insert(_iDecal, pD[_iDecal].m_GUID);
		pD[_iDecal].m_iDecalNext = 0xffff;
		pD[_iDecal].m_iDecalPrev = 0xffff;
	}
	else
	{
		// Insert the decal after the first decal in the chain for it's GUID.
		pD[_iDecal].m_iDecalNext = pD[iFirst].m_iDecalNext;
		pD[_iDecal].m_iDecalPrev = iFirst;
		if (pD[iFirst].m_iDecalNext != 0xffff)
			pD[pD[iFirst].m_iDecalNext].m_iDecalPrev = _iDecal;
		pD[iFirst].m_iDecalNext = _iDecal;
	}
}

void CXR_TriangleMeshDecalContainer::UnlinkDecal(int _iDecal)
{
	CXR_TMDC_Decal* pD = m_lDecals.GetBasePtr();
	if (pD[_iDecal].m_iDecalPrev == 0xffff)
	{
		// This decal is first in the chain for it's GUID, so unlink it and link the second in the chain. (if there is a second)
		m_GUIDMap.Remove(_iDecal);
		if (pD[_iDecal].m_iDecalNext != 0xffff)
		{
			m_GUIDMap.Insert(pD[_iDecal].m_iDecalNext, pD[_iDecal].m_GUID);
			pD[pD[_iDecal].m_iDecalNext].m_iDecalPrev = 0xffff;
		}
	}
	else
	{
		pD[pD[_iDecal].m_iDecalPrev].m_iDecalNext = pD[_iDecal].m_iDecalNext;
		if (pD[_iDecal].m_iDecalNext != 0xffff)
		{
			pD[pD[_iDecal].m_iDecalNext].m_iDecalPrev = pD[_iDecal].m_iDecalPrev;
		}
	}

	pD[_iDecal].m_iDecalPrev = 0xffff;
	pD[_iDecal].m_iDecalNext = 0xffff;
}

void CXR_TriangleMeshDecalContainer::FreeDecal()
{
	if (m_iDecalHead == m_iDecalTail)
	{
		m_iIndexTail = 0;
		m_iIndexHead = 0;
		return;
	}

	UnlinkDecal(m_iDecalTail);

	m_iDecalTail++;
	if (m_iDecalTail >= m_nDecals)
		m_iDecalTail = 0;

	if (m_iDecalHead == m_iDecalTail)
	{
		m_iIndexTail = 0;
		m_iIndexHead = 0;
	}
	else
	{
		m_iIndexTail = m_lDecals[m_iDecalTail].m_iIndices;
	}
}

int CXR_TriangleMeshDecalContainer::MaxIndexPut() const
{
	if (m_iIndexHead == m_iIndexTail)
		return m_nIndices-1;
	else
		return Max(0, ((2*m_nIndices - (m_iIndexHead - m_iIndexTail)) % m_nIndices) -1);
}

int CXR_TriangleMeshDecalContainer::MaxDecalPut() const
{
	if (m_iDecalHead == m_iDecalTail)
		return m_nDecals-1;
	else
		return Max(0, ((2*m_nDecals - (m_iDecalHead - m_iDecalTail)) % m_nDecals) -1);
}

int CXR_TriangleMeshDecalContainer::AddIndices(const uint16* _piIndices, int _nIndices)
{
	if ((m_iIndexHead + _nIndices > m_nIndices))
	{
		while(m_iIndexTail > m_iIndexHead)
			FreeDecal();

		if (m_iIndexTail == 0) 
			FreeDecal();

		m_iIndexHead = 0;
	}

	while(MaxIndexPut() < _nIndices+1)
		FreeDecal();

	int iv = m_iIndexHead;
	m_iIndexHead += _nIndices;
	if (m_iIndexHead >= m_nIndices)
		m_iIndexHead = 0;
	//ConOutL(CStrF("AddVerts %d, %d, %d, %d", iv, _nV, m_iVHead, m_iVTail));

	if (iv + _nIndices > m_nIndices)
		Error("AddIndices", CStrF("Internal error _nV %d, iV %d, nV %d, H %d, T %d", _nIndices, iv, m_nIndices, m_iIndexHead, m_iIndexTail));

	memcpy(&m_lIndices[iv], _piIndices, _nIndices*2);

	return iv;
}


CXR_TriangleMeshDecalContainer::CXR_TriangleMeshDecalContainer()
{
	m_nIndices = 0;
	m_nDecals = 0;
	m_iIndexHead = 0;
	m_iIndexTail = 0;
	m_iDecalHead = 0;
	m_iDecalTail = 0;
	m_bFreeze = false;
}

void CXR_TriangleMeshDecalContainer::Create(uint16 _IndexHeap, int _DecalHeap)
{
	m_iIndexHead = 0;
	m_iIndexTail = 0;
	m_iDecalHead = 0;
	m_iDecalTail = 0;
	m_bFreeze = false;

	m_lIndices.SetLen(_IndexHeap);
	m_lDecals.SetLen(_DecalHeap);
#ifdef _DEBUG
	FillChar(m_lDecals.GetBasePtr(), m_lDecals.ListSize(), 0xdd);
#endif

	m_GUIDMap.Create(_DecalHeap, 8);

	m_nIndices = _IndexHeap;
	m_nDecals = _DecalHeap;
}

void CXR_TriangleMeshDecalContainer::Freeze(bool _bFreeze)
{
	m_bFreeze = _bFreeze;
}

void CXR_TriangleMeshDecalContainer::AddDecal(uint16 _GUID, const CXR_TMDC_Decal& _Decal, const uint16* _piIndices, int _nIndices)
{
	if (m_bFreeze)
	{
		ConOutL(CStrF("(CXR_TriangleMeshDecalContainer::AddDecal) AddDecal called during freeze. (GUID %d)", _GUID));
		return;
	}

	if (MaxDecalPut() < 1)
		FreeDecal();

	int iIndices = AddIndices(_piIndices, _nIndices);
	if (iIndices == -1)
		return;

	m_lDecals[m_iDecalHead] = _Decal;
	m_lDecals[m_iDecalHead].m_GUID = _GUID;
	m_lDecals[m_iDecalHead].m_iIndices = iIndices;
	m_lDecals[m_iDecalHead].m_nIndices = _nIndices;

	LinkDecal(m_iDecalHead);

	m_iDecalHead++;
	if (m_iDecalHead >= m_nDecals)
		m_iDecalHead = 0;
}

const CXR_TMDC_Decal* CXR_TriangleMeshDecalContainer::GetDecalFirst(uint16 _GUID) const
{
	int iDecal = m_GUIDMap.GetIndex(_GUID);
	if (iDecal == -1)
		return NULL;
	else
		return &m_lDecals[iDecal];
}

const CXR_TMDC_Decal* CXR_TriangleMeshDecalContainer::GetDecalNext(const CXR_TMDC_Decal* _pDecal) const
{
	if (_pDecal->m_iDecalNext == 0xffff)
		return NULL;
	else
		return &m_lDecals[_pDecal->m_iDecalNext];
}

