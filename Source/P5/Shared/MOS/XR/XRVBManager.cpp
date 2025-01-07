#include "PCH.h"

// #include "../RndrGL/MRndrGL.h"

#include "XRVBManager.h"
#include "XRClass.h"
#include "XRVBContext.h"
#include "../MSystem/Raster/MRCCore.h"

#ifdef PLATFORM_DOLPHIN
#include "../MRndrDolphin/RenderContext.h"
#endif

#define VBM_OVERWRITE_DEBUGGING 0

#if VBM_OVERWRITE_DEBUGGING
const int MatchBufferSize = 16;
static char aMatchBuffer[MatchBufferSize];
#endif

void CXR_VBMScope::CVBM_SortableVector::Create(uint32 _MaxVB)
{
	m_nMax	= _MaxVB;
}

void CXR_VBMScope::CVBM_SortableVector::Add(CXR_VBManager* _pVBM, CXR_VBPtr _pVBP)
{
	if(!m_lpVBP)
	{
		m_lpVBP = (CXR_VBPtr*)_pVBM->Alloc(sizeof(CXR_VBPtr) * m_nMax);
		if(!m_lpVBP)
			return;
	}

	if(m_iCurrent >= m_nMax)
	{
		// Only write this once so it won't spam
		if(m_iCurrent == m_nMax)
			ConOutL(CStrF("WARNING: Reached maximum VBCount %d", m_nMax));
		return;
	}

	m_lpVBP[m_iCurrent++]	= _pVBP;
}

static int QSortCompare_CXR_VBPtr(const void* _pElem1, const void* _pElem2)
{
	const CXR_VBPtr* pE1 = (const CXR_VBPtr*)_pElem1;
	const CXR_VBPtr* pE2 = (const CXR_VBPtr*)_pElem2;

	return pE1->Compare(*pE2);
}

void CXR_VBMScope::CVBM_SortableVector::Sort()
{
	if(m_iCurrent > 1)
	{
		qsort(m_lpVBP, m_iCurrent, sizeof(CXR_VBPtr), QSortCompare_CXR_VBPtr);
	}
}

CXR_VBPtr& CXR_VBMScope::CVBM_SortableVector::operator [] (int _iVB)
{
	M_ASSERT(_iVB >= 0 && _iVB < m_iCurrent, "Out of range");
	return m_lpVBP[_iVB];
}

int CXR_VBMScope::CVBM_SortableVector::Len() const
{
	return m_iCurrent;
}

void CXR_VBMScope::Create(uint32 _MaxVB)
{
	m_lpVB.Create(_MaxVB);
}


#define	THREAD_ERROR_CHECKING	1

int CXR_VBPtr::Compare(const CXR_VBPtr& _pVB) const
{
	if (m_pVB->m_Priority < _pVB.m_pVB->m_Priority)
		return -1;
	if (m_pVB->m_Priority > _pVB.m_pVB->m_Priority)
		return 1;

	{
		if (m_pVB->m_pAttrib != _pVB.m_pVB->m_pAttrib)
		{
			int FlagsThis = m_pVB->m_Flags;
			XR_COMPAREATTRIBUTE_INT(FlagsThis, _pVB.m_pVB->m_Flags);

			if (FlagsThis & CXR_VBFLAGS_VIRTUALATTRIBUTES)
			{
				XR_COMPAREATTRIBUTE_INT(m_pVB->m_pVAttrib->m_Class, _pVB.m_pVB->m_pVAttrib->m_Class);
				int Comp = m_pVB->m_pVAttrib->OnCompare(_pVB.m_pVB->m_pVAttrib);
				if (Comp)
					return Comp;
			}
			else
			{
				int Comp = m_pVB->m_pAttrib->Compare(*_pVB.m_pVB->m_pAttrib);
				if (Comp)
					return Comp;
/*				else
				{
					const_cast<CXR_VBPtr&>(_pVB).m_pVB->m_pAttrib = m_pVB->m_pAttrib;
				}*/
			}
		}
	}

#ifdef M_Profile
	// Error checking
/*	if ((!(m_pVB->m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN))) || BitCount(m_pVB->m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) != 1)
	{
		M_BREAKPOINT;
	}		
	if ((!(_pVB.m_pVB->m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN))) || BitCount(_pVB.m_pVB->m_Flags & (CXR_VBFLAGS_VBIDCHAIN | CXR_VBFLAGS_VBCHAIN)) != 1)
	{
		M_BREAKPOINT;
	}*/
#endif
/*	if (m_pVB->m_Flags & CXR_VBFLAGS_VBIDCHAIN)
	{
		if (_pVB.m_pVB->m_Flags & CXR_VBFLAGS_VBIDCHAIN)
		{
			CXR_VBIDChain *pChain0 = m_pVB->GetVBIDChain();
			CXR_VBIDChain *pChain1 = _pVB.m_pVB->GetVBIDChain();

			if (pChain0->m_VBID < pChain1->m_VBID)
				return -1;
			if (pChain0->m_VBID > pChain1->m_VBID)
				return 1;                
		}
		else
		{
			return -1;
		}
	}
	else
	{
		if (_pVB.m_pVB->m_Flags & CXR_VBFLAGS_VBCHAIN)
		{
			return 0;
		}
		else
		{
			return 1;
		}
	}*/

/*		if (m_pVB->m_pMatrixPaletteArgs && !_pVB.m_pVB->m_pMatrixPaletteArgs)
		return 1;
	if (!m_pVB->m_pMatrixPaletteArgs && _pVB.m_pVB->m_pMatrixPaletteArgs)
		return -1;

	if (m_pVB->m_pMatrixPaletteArgs && _pVB.m_pVB->m_pMatrixPaletteArgs)
	{
		if (m_pVB->m_pMatrixPaletteArgs->m_piMatrices < _pVB.m_pVB->m_pMatrixPaletteArgs->m_piMatrices)
			return -1;
		if (m_pVB->m_pMatrixPaletteArgs->m_piMatrices > _pVB.m_pVB->m_pMatrixPaletteArgs->m_piMatrices)
			return 1;
		if (m_pVB->m_pMatrixPaletteArgs->m_pMatrices < _pVB.m_pVB->m_pMatrixPaletteArgs->m_pMatrices)
			return -1;
		if (m_pVB->m_pMatrixPaletteArgs->m_pMatrices > _pVB.m_pVB->m_pMatrixPaletteArgs->m_pMatrices)
			return 1;
	}
//		if ((uint32)m_pVB->m_pMatrixPaletteArgs < (uint32)_pVB.m_pVB->m_pMatrixPaletteArgs)
//			return -1;
//		if ((uint32)m_pVB->m_pMatrixPaletteArgs > (uint32)_pVB.m_pVB->m_pMatrixPaletteArgs)
//			return 1;

	for(int m = 0; m < CRC_MATRIXSTACKS; m++)
	{
		if (m_pVB->m_pTransform[m] != _pVB.m_pVB->m_pTransform[m])
		{
			if (m_pVB->m_pTransform[m] && !_pVB.m_pVB->m_pTransform[m])
				return 1;
			if (!m_pVB->m_pTransform[m] && _pVB.m_pVB->m_pTransform[m])
				return -1;
			if (!m_pVB->m_pTransform[m] && !_pVB.m_pVB->m_pTransform[m])
				continue;

			int Cmp = memcmp(m_pVB->m_pTransform[m], _pVB.m_pVB->m_pTransform[m], sizeof(CMat4Dfp4));
			if (Cmp)
				return Cmp;
		}
	}*/

	return 0;
}



	MRTC_IMPLEMENT_DYNAMIC(CXR_VBManager, CReferenceCount);
	MRTC_IMPLEMENT_DYNAMIC(CXR_VBMContainer, CReferenceCount);

// -------------------------------------------------------------------
//  CXR_VBManager
// -------------------------------------------------------------------
CXR_VBManager::CXR_VBManager()
{
	MAUTOSTRIP(CXR_VBManager_ctor, MAUTOSTRIP_VOID);
	m_AllocPos = 0;
	m_pHeapHW = NULL;
	m_HeapSizeHW = 0;
	m_AllocPosHW = 0;
	m_bOutOfMemory = false;
	m_pCurrentRC = NULL;
	m_pOwner	= 0;
	m_iVPStack	= 0;
	m_iScopeStack	= -1;
	m_iClipStack	 = 0;
	m_MaxVB	= 16384;
#ifdef PLATFORM_DOLPHIN
	m_nToken = -1;
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	m_RecordUsage = 0;
	gf_RDSendRegisterHeap((mint)this, "VBManager");
#endif

	MACRO_GetRegisterObject(CXR_VBContext, pVBCtx, "SYSTEM.VBCONTEXT");
	m_pVBCtx	= pVBCtx;

	m_pActiveScope = NULL;
	m_pHeap = NULL;
	m_HeapSize = 0;
}

CXR_VBManager::~CXR_VBManager()
{
	if (m_pHeap)
	{
		MRTC_SystemInfo::OS_FreeGPU(m_pHeap);
		m_pHeap = NULL;
	}
	

}

void CXR_VBManager::Create(int _HeapSize, int _MaxVB)
{
	if (m_pHeap)
	{
		MRTC_SystemInfo::OS_FreeGPU(m_pHeap);
		m_pHeap = NULL;
	}

	MAUTOSTRIP(CXR_VBManager_Create, MAUTOSTRIP_VOID);

	m_pHeap = (uint8 *)MRTC_SystemInfo::OS_AllocGPU(_HeapSize, true);
	m_HeapSize = _HeapSize;

	m_State	= CXR_VBMANAGER_STATE_PREBEGIN;

	m_AllocPos = 0;
#ifdef M_Profile
	m_AllocPosMax = 0;
#endif
	m_MaxVB	= _MaxVB;

	m_lVertexUseMap.SetLen(2048);
	FillChar(m_lVertexUseMap.GetBasePtr(), m_lVertexUseMap.ListSize(), 0);
}

#ifdef M_Profile
int CXR_VBManager::GetMinAvail()
{
	return m_HeapSize - m_AllocPosMax;
}
#endif

uint8* CXR_VBManager::GetVertexUseMap(int _nV)
{
	MAUTOSTRIP(CXR_VBManager_GetVertexUseMap, NULL);
	if (m_lVertexUseMap.Len()*8 >= _nV) return m_lVertexUseMap.GetBasePtr();

	m_lVertexUseMap.SetLen(_nV*2 >> 3);
	FillChar(m_lVertexUseMap.GetBasePtr(), m_lVertexUseMap.ListSize(), 0);
	return m_lVertexUseMap.GetBasePtr();
}

int CXR_VBManager::GetHeap()
{
	MAUTOSTRIP(CXR_VBManager_GetHeap_2, 0);
	return m_HeapSize;
}

int CXR_VBManager::GetAvail()
{
	MAUTOSTRIP(CXR_VBManager_GetAvail_2, 0);
	return GetHeap() - GetAllocPos();
}

int CXR_VBManager::GetAllocPos()
{
	MAUTOSTRIP(CXR_VBManager_GetAvail_2, 0);
	return m_AllocPos;
}

bool CXR_VBManager::OutOfMemory()
{
	MAUTOSTRIP(CXR_VBManager_OutOfMemory, false);
	return m_bOutOfMemory;
}

void* CXR_VBManager::Alloc_Open()
{
	MAUTOSTRIP(CXR_VBManager_Alloc_Open, NULL);

#if	THREAD_ERROR_CHECKING
	if(m_State != CXR_VBMANAGER_STATE_RENDERING)
		Error("CXR_VBManager::Alloc_Open", "CXR_VBManager::Alloc_Open called outside of Begin/End");
#endif	// THREAD_ERROR_CHECKING

	M_ASSERT((void*)m_AllocLock.m_ThreadID == MRTC_SystemInfo::OS_GetThreadID(), "Must take m_AllocLock before calling Alloc_Open");

#if VBM_OVERWRITE_DEBUGGING
	if(memcmp(m_pHeap + m_AllocPos, aMatchBuffer, sizeof(aMatchBuffer)))
	{
		M_BREAKPOINT;
	}
#endif //VBM_OVERWRITE_DEBUGGING

	int AllocPos = m_AllocPos;

	if (AllocPos >= m_HeapSize)
	{
		return NULL;
	}
	else
		return m_pHeap + AllocPos;
}

void CXR_VBManager::Alloc_Close(int _Size)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_Close, MAUTOSTRIP_VOID);
#if	THREAD_ERROR_CHECKING
	if(m_State != CXR_VBMANAGER_STATE_RENDERING)
		Error("CXR_VBManager::Alloc_Close", "CXR_VBManager::Alloc_Close called outside of Begin/End");
#endif	// THREAD_ERROR_CHECKING

	if (_Size > GetAvail())
	{
		// Make sure Lock is released before throwing exception (memory is probably fucked though)
		Error("Alloc_Close", CStrF("Illegal open allocation. (Size: %d, Avail %d)", _Size, GetAvail()));
	}
#ifdef PLATFORM_PS3
	_Size = (_Size + 15) & ~15;
#else
	_Size = (_Size + 3) & ~3;
#endif
	int AllocPos = m_AllocPos;
	m_AllocPos += _Size;

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	if (m_RecordUsage)
		gf_RDSendHeapAlloc((void *)(mint)AllocPos, _Size, this, gf_RDGetSequence(), 0);
#endif
#ifdef M_Profile
	if (AllocPos + _Size > m_AllocPosMax)
		m_AllocPosMax = AllocPos + _Size;

#endif
#if VBM_OVERWRITE_DEBUGGING
	memcpy(aMatchBuffer, m_pHeap + m_AllocPos, sizeof(aMatchBuffer));
#endif //VBM_OVERWRITE_DEBUGGING


#ifdef M_Profile
	m_Stats_nOpen += _Size;
#endif
}

#ifdef NEVER

int CXR_VBManager::GetHeapHW()
{
	MAUTOSTRIP(CXR_VBManager_GetHeapHW, 0);
	return m_HeapSizeHW;
}

int CXR_VBManager::GetAvailHW()
{
	MAUTOSTRIP(CXR_VBManager_GetAvailHW, 0);
	return GetHeapHW() - m_AllocPosHW;
}

void* CXR_VBManager::Alloc_OpenHW()
{
	MAUTOSTRIP(CXR_VBManager_Alloc_OpenHW, NULL);
	if (!GetAvailHW()) return NULL;
	return &m_pHeapHW[m_AllocPosHW];
}

void CXR_VBManager::Alloc_CloseHW(int _Size)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_CloseHW, MAUTOSTRIP_VOID);
	if (_Size > GetAvailHW())
		Error("Alloc_CloseHW", CStrF("Illegal open allocation. (Size: %d, Avail %d)", _Size, GetAvailHW()));

	m_AllocPosHW += _Size;
	m_AllocPosHW = (m_AllocPosHW + (VBM_HWALLOCALIGN-1)) & (~VBM_HWALLOCALIGN);
}

#endif

void* CXR_VBManager::AllocHW(int _Size)
{
	MAUTOSTRIP(CXR_VBManager_AllocHW, NULL);
	return NULL;
#ifdef NEVER
	if (_Size <= GetAvailHW())
	{
		void* p = &m_pHeapHW[m_AllocPosHW];
		m_AllocPosHW += _Size;
		m_AllocPosHW = (m_AllocPosHW + (VBM_HWALLOCALIGN-1)) & (~(VBM_HWALLOCALIGN-1));
	if (int((uint8*)p) & 0x1f)
		ConOut(CStrF("Unaligned: %.8x", p));
		return p;
	}
	else
	{
		m_bOutOfMemory = true;
		return NULL;
	}
#endif
}


void CXR_VBManager::GetAllocParams(int* _pAllocPos,MRTC_SpinLock* _pLock,uint8** _pHeap,mint* _pHeapSize)
{
	_pAllocPos=&m_AllocPos;
	_pLock=&m_AllocLock;
	_pHeap=&m_pHeap;
	_pHeapSize=&m_HeapSize;
}


void* CXR_VBManager::Alloc(int _Size)
{
	MAUTOSTRIP(CXR_VBManager_Alloc, NULL);
#if	THREAD_ERROR_CHECKING
	if(m_State != CXR_VBMANAGER_STATE_RENDERING)
		Error("CXR_VBManager::Alloc", "CXR_VBManager::Alloc called outside of Begin/End");
#endif	// THREAD_ERROR_CHECKING

	if (!_Size)
		return NULL;

	// Make sure Alloc is threadsafe
	M_LOCK(m_AllocLock);

#if VBM_OVERWRITE_DEBUGGING
	if(memcmp(m_pHeap + m_AllocPos, aMatchBuffer, sizeof(aMatchBuffer)))
	{
		M_BREAKPOINT;
	}
#endif //VBM_OVERWRITE_DEBUGGING
#ifdef PLATFORM_PS3
	_Size = (_Size + 15) & ~15;
#else
	_Size = (_Size + 3) & ~3;
#endif
	int32 AllocPos = m_AllocPos;
	m_AllocPos += _Size;


	if (AllocPos + _Size <= m_HeapSize)
	{
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	if (m_RecordUsage)
		gf_RDSendHeapAlloc((void *)(mint)AllocPos, _Size, this, gf_RDGetSequence(),  0);
#endif
		void* p = m_pHeap + AllocPos;


#ifdef M_Profile
		if (AllocPos + _Size > m_AllocPosMax)
			m_AllocPosMax = AllocPos + _Size;
#endif
#if VBM_OVERWRITE_DEBUGGING
		memcpy(aMatchBuffer, m_pHeap + m_AllocPos, sizeof(aMatchBuffer));
#endif //VBM_OVERWRITE_DEBUGGING
		return p;
	}
	else
	{
		m_bOutOfMemory = true;
		return NULL;
	}
}

void* CXR_VBManager::AllocArray(int _Size)
{
	MAUTOSTRIP(CXR_VBManager_AllocArray, NULL);
#ifdef NEVER
	if (GetAvailHW() >= _Size)
		return AllocHW(_Size);
	else
#endif
		return Alloc(_Size);
}

CXR_VertexBuffer* CXR_VBManager::Alloc_VB()
{
	MAUTOSTRIP(CXR_VBManager_Alloc_VB, NULL);
	CXR_VertexBuffer* pVB = (CXR_VertexBuffer*) Alloc(sizeof(CXR_VertexBuffer));
	if (pVB)
	{
		pVB->Clear();
#ifdef M_Profile
		m_Stats_nVB++;
#endif
	}
	return pVB;
}

CXR_VBIDChain* CXR_VBManager::Alloc_VBIDChain()
{
	CXR_VBIDChain* pChain = (CXR_VBIDChain*) Alloc(sizeof(CXR_VBIDChain));
	if(!pChain)
		return NULL;
	pChain->Clear();
	return pChain;
}

CXR_VBChain* CXR_VBManager::Alloc_VBChain()
{
	CXR_VBChain* pChain = (CXR_VBChain*) Alloc(sizeof(CXR_VBChain));
	if(!pChain)
		return NULL;
	pChain->Clear();
	return pChain;
}

CXR_VertexBuffer_PreRender* CXR_VBManager::Alloc_PreRender(PFN_VERTEXBUFFER_PRERENDER _pFunc, void *_pContext)
{
	CXR_VertexBuffer_PreRender *pPreRender = (CXR_VertexBuffer_PreRender *) Alloc(sizeof(CXR_VertexBuffer_PreRender));
	if(!pPreRender)
		return NULL;
	pPreRender->m_pfnPreRender = _pFunc;
	pPreRender->m_pPreRenderContext = _pContext;
	return pPreRender;
}

CXR_VBChain* CXR_VBManager::Alloc_VBChain(int _Contents, int _nV)
{
	CXR_VBChain* pChain = Alloc_VBChain();
	if (!pChain)
		return NULL;

	if (_Contents & CXR_VB_VERTICES)
	{
		pChain->m_pV = Alloc_V3(_nV);
		if (!pChain->m_pV) 
			return NULL;
		pChain->m_nV = _nV;
	}

	if (_Contents & CXR_VB_COLORS)
	{
		pChain->m_pCol = Alloc_CPixel32(_nV);
		if (!pChain->m_pCol) return NULL;
	}

	if (_Contents & CXR_VB_SPECULAR)
	{
		pChain->m_pSpec = Alloc_CPixel32(_nV);
		if (!pChain->m_pSpec) return NULL;
	}

/*	if (_Contents & CXR_VB_FOG)
	{
		pVB->m_pFog = Alloc_fp4(_nV);
		if (!pVB->m_pFog) return NULL;
	}*/

	if (_Contents & CXR_VB_TVERTICESALL)
	{
		for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
		{
			int Flag = CXR_VB_TVERTICES0 << i;
			if (_Contents & Flag)
			{
				pChain->m_pTV[i] = (fp4*)Alloc_V2(_nV);
				if (!pChain->m_pTV[i]) return NULL;
				pChain->m_nTVComp[i] = 2;
			}
		}
	}
	return pChain;
}

CXR_VertexBuffer* CXR_VBManager::Alloc_VB(int _Contents, int _nV)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_VB_2, NULL);
	CXR_VertexBuffer* pVB = Alloc_VB();
	if (!pVB) return NULL;

	if (_Contents & CXR_VB_ATTRIB)
	{
		pVB->m_pAttrib = Alloc_Attrib();
		if (!pVB->m_pAttrib) return NULL;
		pVB->m_pAttrib->SetDefault();
	}

	if (_Contents & (CXR_VB_VERTICES | CXR_VB_COLORS | CXR_VB_SPECULAR | CXR_VB_TVERTICESALL))
	{
		CXR_VBChain *pChain = Alloc_VBChain(_Contents, _nV);

		if (!pChain)
			return NULL;

		pVB->m_Flags |= CXR_VBFLAGS_VBCHAIN;
		pVB->m_pVBChain = pChain;
	}


	return pVB;
}

CXR_VertexBuffer* CXR_VBManager::Alloc_VBAttrib()
{
	CXR_VertexBuffer* pFirstVB = (CXR_VertexBuffer*)Alloc(sizeof(CXR_VertexBuffer) + sizeof(CRC_Attributes));
	if (!pFirstVB) 
		return NULL;

	pFirstVB->Clear();
	pFirstVB->m_pAttrib = (CRC_Attributes*)(pFirstVB + 1);
	pFirstVB->m_pAttrib->SetDefault();

	return pFirstVB;
}

#if 0
CXR_VertexBuffer* CXR_VBManager::Alloc_VBChainAttrib(int _Len)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_VBChainAttrib, NULL);

	CXR_VertexBuffer* pFirstVB = (CXR_VertexBuffer*)Alloc(sizeof(CXR_VertexBuffer)*_Len + sizeof(CRC_Attributes));
	if (!pFirstVB) return NULL;

	int nVB = 0;
	CXR_VertexBuffer* pPrevVB = NULL;
	CXR_VertexBuffer* pVB = pFirstVB;
	while(nVB < _Len)
	{
		pVB->Clear();
		if( pPrevVB ) pPrevVB->m_pNextVB = pVB;
		pPrevVB = pVB;
		pVB++;
		nVB++;
	}

#ifdef M_Profile
	m_Stats_nAttrib++;
	if (_Len > 1)
	{
		m_Stats_nChains++;
		m_Stats_nChainVBs += _Len;
	}
#endif

	pFirstVB->m_pAttrib = (CRC_Attributes*)pVB;
	pFirstVB->m_pAttrib->SetDefault();
	return pFirstVB;
}
#endif

bool CXR_VBManager::Alloc_VBChainCopy(CXR_VertexBuffer* _pDst, CXR_VertexBuffer* _pSrc)
{
	_pDst->CopyVBChain(_pSrc);
	if (_pDst->IsVBIDChain())
	{
		CXR_VBIDChain *pChain = _pSrc->GetVBIDChain();
		int nVBCount = 0;
		while(pChain)
		{
			nVBCount++;
			pChain = pChain->m_pNextVB;
		}

		uint8* pVBMData = (uint8*)Alloc(sizeof(CXR_VBIDChain) * nVBCount);
		if(!pVBMData)
			return false;
		_pDst->m_pVBChain = NULL;
		CXR_VBIDChain *pLast = NULL;
		pChain = _pSrc->GetVBIDChain();
		while (pChain)
		{
			CXR_VBIDChain* pNewChain = (CXR_VBIDChain*)pVBMData; pVBMData += sizeof(CXR_VBIDChain);
			*pNewChain = *pChain;
			if (pLast)
				pLast->m_pNextVB = pNewChain;
			else
				_pDst->m_pVBChain = pNewChain;
			pLast = pNewChain;
			pNewChain->m_pNextVB = NULL;

			pChain = pChain->m_pNextVB;
		}
	}
	else
	{
		CXR_VBChain *pChain = _pSrc->GetVBChain();
		int nVBCount = 0;
		while(pChain)
		{
			nVBCount++;
			pChain = pChain->m_pNextVB;
		}

		uint8* pVBMData = (uint8*)Alloc(sizeof(CXR_VBChain) * nVBCount);
		if(!pVBMData)
			return false;

		_pDst->m_pVBChain = NULL;
		CXR_VBChain *pLast = NULL;
		pChain = _pSrc->GetVBChain();
		while (pChain)
		{
			CXR_VBChain* pNewChain = (CXR_VBChain*)pVBMData; pVBMData += sizeof(CXR_VBChain);
			*pNewChain = *pChain;
			if (pLast)
				pLast->m_pNextVB = pNewChain;
			else
				_pDst->m_pVBChain = pNewChain;
			pLast = pNewChain;
			pNewChain->m_pNextVB = NULL;

			pChain = pChain->m_pNextVB;
		}
	}
	return true;
}

/*

CXR_VertexBuffer* CXR_VBManager::Alloc_VBChainOnly(int _Len)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_VBChainOnly, NULL);

	CXR_VertexBuffer* pFirstVB = (CXR_VertexBuffer*)Alloc(sizeof(CXR_VertexBuffer)*_Len);
	if (!pFirstVB) return NULL;

	int nVB = 0;
	CXR_VertexBuffer* pPrevVB = NULL;
	CXR_VertexBuffer* pVB = pFirstVB;
	while(nVB < _Len)
	{
		pVB->Clear();
		if( pPrevVB ) pPrevVB->m_pNextVB = pVB;
		pPrevVB = pVB;
		pVB++;
		nVB++;
	}

#ifdef M_Profile
	if (_Len > 1)
	{
		m_Stats_nChains++;
		m_Stats_nChainVBs += _Len;
	}
#endif

	return pFirstVB;
}

CXR_VertexBuffer* CXR_VBManager::Alloc_VBChain(int _Contents, int _Len)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_VBChain, NULL);
#ifndef	M_RTM
	if (_Contents & (CXR_VB_VERTICES | CXR_VB_TVERTICES0 | CXR_VB_TVERTICES1 | CXR_VB_TVERTICES2 | CXR_VB_TVERTICES3 | CXR_VB_COLORS))
		Error("Alloc_VBChain", "Cannot allocate geometry.");
#endif

	CXR_VertexBuffer* pFirstVB = Alloc_VB(_Contents);
	if (!pFirstVB) return NULL;
	int nVB = 1;

	_Contents &= ~CXR_VB_ATTRIB;

	CXR_VertexBuffer* pVB = pFirstVB;
	while(nVB < _Len)
	{
		pVB->m_pNextVB = Alloc_VB(_Contents);
		if (!pVB->m_pNextVB) return NULL;
		pVB = pVB->m_pNextVB;
		nVB++;
	}

#ifdef M_Profile
	if (_Len > 1)
	{
		m_Stats_nChains++;
		m_Stats_nChainVBs += _Len;
	}
#endif

	return pFirstVB;
}
*/

const CMat4Dfp4** CXR_VBManager::Alloc_TextureMatrixArray()
{
	const CMat4Dfp4** pArray = (const CMat4Dfp4**)Alloc(sizeof(const CMat4Dfp4*) * CRC_MAXTEXCOORDS);
	if (!pArray)
		return NULL;
	memset(pArray, 0, sizeof(const CMat4Dfp4*) * CRC_MAXTEXCOORDS);
	return pArray;
}

CRC_Attributes* CXR_VBManager::Alloc_Attrib()
{
	MAUTOSTRIP(CXR_VBManager_Alloc_Attrib, NULL);
#ifdef M_Profile
	m_Stats_nAttrib++;
#endif
	return (CRC_Attributes*) Alloc(sizeof(CRC_Attributes));
}

CRC_Light* CXR_VBManager::Alloc_Lights(int _nLights)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_Lights, NULL);
#ifdef M_Profile
	m_Stats_nLights++;
#endif
	return (CRC_Light*) Alloc(sizeof(CRC_Light) * _nLights);
}

CXR_Light* CXR_VBManager::Alloc_XRLights(int _nLights)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_Lights, NULL);
#ifdef M_Profile
	m_Stats_nXRLights++;
#endif
	return (CXR_Light*) Alloc(sizeof(CXR_Light) * _nLights);
}

CMat4Dfp4* CXR_VBManager::Alloc_M4()
{
	MAUTOSTRIP(CXR_VBManager_Alloc_M4, NULL);
#ifdef M_Profile
	m_Stats_nM4++;
#endif
	return (CMat4Dfp4*) Alloc(sizeof(CMat4Dfp4));
}

CMat4Dfp4* CXR_VBManager::Alloc_M4(const CMat4Dfp4& _Mat)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_M4_2, NULL);
#ifdef M_Profile
	m_Stats_nM4++;
#endif
	CMat4Dfp4* pMat = (CMat4Dfp4*) Alloc(sizeof(CMat4Dfp4));
	if (pMat) *pMat = _Mat;
	return pMat;
}

CMat43fp4* CXR_VBManager::Alloc_M43()
{
#ifdef M_Profile
	m_Stats_nM4++;
#endif
	return (CMat43fp4*) Alloc(sizeof(CMat43fp4));
}

CMat43fp4* CXR_VBManager::Alloc_M43(const CMat43fp4& _Mat)
{
#ifdef M_Profile
	m_Stats_nM4++;
#endif
	CMat43fp4* pMat = (CMat43fp4*) Alloc(sizeof(CMat43fp4));
	if (pMat) *pMat = _Mat;
	return pMat;
}

#ifndef DEFINE_MAT43_IS_MAT4D

CMat4Dfp4* CXR_VBManager::Alloc_M4(const CMat43fp4& _Mat)
{
#ifdef M_Profile
	m_Stats_nM4++;
#endif
	CMat4Dfp4* pMat = (CMat4Dfp4*) Alloc(sizeof(CMat4Dfp4));
	if (pMat) _Mat.Assign4x4(*pMat);
	return pMat;
}

CMat43fp4* CXR_VBManager::Alloc_M43(const CMat4Dfp4& _Mat)
{
#ifdef M_Profile
	m_Stats_nM4++;
#endif
	CMat43fp4* pMat = (CMat43fp4*) Alloc(sizeof(CMat43fp4));
	if (pMat)
	{
		pMat->CreateFrom(_Mat);
	}
	return pMat;
}

#endif

CVec4Dfp4* CXR_VBManager::Alloc_V4(int _nV)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_V4, NULL);
#ifdef M_Profile
	m_Stats_nV4 += _nV;
#endif
	return (CVec4Dfp4*) AllocArray(sizeof(CVec4Dfp4) * _nV);
}

CVec3Dfp4* CXR_VBManager::Alloc_V3(int _nV)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_V3, NULL);
#ifdef M_Profile
	m_Stats_nV3 += _nV;
#endif
	return (CVec3Dfp4*) AllocArray(sizeof(CVec3Dfp4) * _nV);
}

CVec2Dfp4* CXR_VBManager::Alloc_V2(int _nV)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_V2, NULL);
#ifdef M_Profile
	m_Stats_nV2 += _nV;
#endif
	return (CVec2Dfp4*) AllocArray(sizeof(CVec2Dfp4) * _nV);
}

uint16* CXR_VBManager::Alloc_Int16(int _nV)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_Int16, NULL);
#ifdef M_Profile
	m_Stats_nInt16 += _nV;
#endif
	return (uint16*) Alloc(sizeof(uint16) * _nV);
}

uint32* CXR_VBManager::Alloc_Int32(int _nV)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_Int32, NULL);
#ifdef M_Profile
	m_Stats_nInt32 += _nV;
#endif
	return (uint32*) Alloc(sizeof(uint32) * _nV);
}

fp4* CXR_VBManager::Alloc_fp4(int _nV)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_fp4, NULL);
#ifdef M_Profile
	m_Stats_nFp4 += _nV;
#endif
	return (fp4*) Alloc(sizeof(fp4) * _nV);
}

CPixel32* CXR_VBManager::Alloc_CPixel32(int _nV)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_CPixel32, NULL);
#ifdef M_Profile
	m_Stats_nPixel32 += _nV;
#endif
	return (CPixel32*) AllocArray(sizeof(uint32) * _nV);
}

void CXR_VBManager::AddVB(CXR_VertexBuffer* _pVB)
{
	MAUTOSTRIP(CXR_VBManager_AddVB, MAUTOSTRIP_VOID);
//	ConOut(CStrF("(CXR_VBManager::AddVB) %.8x", _pVB));
	if (!_pVB) return;

	M_ASSERT(!_pVB->m_pVBChain || !_pVB->IsVBIDChain() || _pVB->GetVBIDChain()->m_PrimType != 0, "Invalid VB prim type");
	M_ASSERT(!_pVB->m_pVBChain || _pVB->IsVBIDChain() || _pVB->GetVBChain()->m_PrimType != 0, "Invalid VB prim type");

#ifndef M_RTM
	if (_pVB->m_pVBChain)
	{
		int nTri = 0;
		uint16 *pTri = 0;

		if (_pVB->IsVBIDChain())
		{
			if (_pVB->GetVBIDChain()->m_PrimType == CRC_RIP_TRIANGLES)
			{
				nTri = _pVB->GetVBIDChain()->m_nPrim;
				pTri = _pVB->GetVBIDChain()->m_piPrim;
			}
		}
		else
		{
			if (_pVB->GetVBChain()->m_PrimType == CRC_RIP_TRIANGLES)
			{
				nTri = _pVB->GetVBChain()->m_nPrim;
				pTri = _pVB->GetVBChain()->m_piPrim;
			}
		}

		// Check that triangles are correctly added. If you crash here you might have incorrectly added a list wint 65535 indices, the max is 65534
		if (nTri == 0xffff)
		{
			// Recursively call this function
			mint *pList = ((mint *)pTri);
			int nLists = *pList;
			++pList;
			for (int i = 0; i < nLists; ++i)
			{
				int nTri = *pList;
				++pList;
				uint16 *pTriList = *((uint16 **)pList);

				for (int i = 0; i < nTri; ++i)
				{
					uint16 Tri = pTriList[i];
					MRTC_ReferenceSymbol(Tri);
				}
				++pList;
			}
		}
	}
#endif

#ifndef	PLATFORM_PS2
#ifdef _DEBUG
	if (!(_pVB->m_Flags & CXR_VBFLAGS_VIRTUALATTRIBUTES) && _pVB->m_pAttrib)
	{
		if (_pVB->m_pAttrib->m_Flags & CRC_FLAGS_STENCIL)
		{
			M_ASSERT(_pVB->m_pAttrib->m_StencilFrontFunc, "You must set a valid stencil function");
		}
		if (_pVB->m_pAttrib->m_Flags & CRC_FLAGS_ZCOMPARE)
		{
			M_ASSERT(_pVB->m_pAttrib->m_ZCompare, "You must set a valid zcompare function");
		}
	}
#endif	// _DEBUG
#endif	// PLATFORM_PS2

	if (_pVB->IsValid())
	{
		M_LOCK(m_AddLock);

#if	THREAD_ERROR_CHECKING
		if(m_State != CXR_VBMANAGER_STATE_RENDERING)
			Error("CXR_VBManager::AddVB", "CXR_VBManager::AddVB called outside of Begin/End");
#endif	// THREAD_ERROR_CHECKING

		if(m_iScopeStack == -1)
			Error("CXR_VBManager::AddVB", "Must be inside scope for AddVB to be valid");
//		int iActiveScope = m_lScopeStack[m_iScopeStack];

		if(_pVB->m_iVP == 0)
			_pVB->m_iVP = m_liVPStack[m_iVPStack];

		if((_pVB->m_iClip == 0) && !(_pVB->m_Flags & CXR_VBFLAGS_FORCENOCLIP))
			_pVB->m_iClip = m_liClipStack[m_iClipStack];

		m_pActiveScope->m_lpVB.Add(const_cast<CXR_VBManager*>(this), CXR_VBPtr(_pVB));
	}
//Please remove potentially spamming conouts before check-in /AO
//	else
//		ConOutD("Invalid vertex-buffer.");
}

void CXR_VBManager::AddVB(const CXR_VertexBuffer& _VB)
{
	M_ASSERT(!_VB.m_pVBChain || !_VB.IsVBIDChain() || _VB.GetVBIDChain()->m_PrimType != 0, "Invalid VB prim type");
	M_ASSERT(!_VB.m_pVBChain || _VB.IsVBIDChain() || _VB.GetVBChain()->m_PrimType != 0, "Invalid VB prim type");
	MAUTOSTRIP(CXR_VBManager_AddVB_2, MAUTOSTRIP_VOID);
	CXR_VertexBuffer* pVB = Alloc_VB();
	if (pVB)
	{
		*pVB = _VB;
		AddVB(pVB);
	}
}

bool CXR_VBManager::AddCallback(PFN_VERTEXBUFFER_PRERENDER _pfnCallback, void* _pContext, fp4 _Priority)
{
	CXR_VertexBuffer *pVB = Alloc_VB();
	if (!pVB)
		return false;

	CXR_VertexBuffer_PreRender* pPreRender = (CXR_VertexBuffer_PreRender*) Alloc(sizeof(CXR_VertexBuffer_PreRender));
	if (!pPreRender)
		return false;

	pPreRender->m_pfnPreRender = _pfnCallback;
	pPreRender->m_pPreRenderContext = _pContext;

	pVB->m_pPreRender = pPreRender;
	pVB->m_Priority = _Priority;
	pVB->m_Flags |= CXR_VBFLAGS_PRERENDER;

	AddVB(pVB);
	return true;
}

class CXR_VBManager_RenderOptions
{
public:
	uint32 m_Options;
	uint32 m_Format;
	
	static void RenderCallback(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
	{
		MSCOPESHORT(InitiateTexturePrecache);
		CXR_VBManager_RenderOptions* pData = (CXR_VBManager_RenderOptions*)_pContext;
		_pRC->Render_SetRenderOptions(pData->m_Options, pData->m_Format);
	}
};

bool CXR_VBManager::AddRenderOptions(fp4 _Priority, uint32 _Options, uint32 _Format)
{
	CXR_VBManager_RenderOptions* pData = (CXR_VBManager_RenderOptions*)Alloc(sizeof(CXR_VBManager_RenderOptions));
	if (!pData)
		return false;

	pData->m_Options = _Options;
	pData->m_Format = _Format;
	return AddCallback(CXR_VBManager_RenderOptions::RenderCallback, pData, _Priority);
}

bool CXR_VBManager::AddRenderRange(fp4 _Priority, fp4 _StartRenderPrio, fp4 _EndRenderPrio)
{
	CXR_VBManager_RenderRange* pData = (CXR_VBManager_RenderRange*)Alloc(sizeof(CXR_VBManager_RenderRange));
	if (!pData)
		return false;

	pData->m_StartPrio = _StartRenderPrio;
	pData->m_EndPrio = _EndRenderPrio;
	return AddCallback(CXR_VBManager_RenderRange::RenderCallback, pData, _Priority);
}

bool CXR_VBManager::AddClearRenderTarget(fp4 _Priority, int _WhatToClear, CPixel32 _ColorClearTo, fp4 _ZClearTo, int _StencilClearTo, CRct _ClearArea)
{
	return false;
}

bool CXR_VBManager::AddCopyToTexture(fp4 _Priority, CRct _SrcRect, CPnt _Dst, uint16 _TextureID, bint _bContinueTiling, uint16 _Slice)
{
	return false;
}

bool CXR_VBManager::AddNextClearParams(fp4 _Priority, int _WhatToClear, CPixel32 _ColorClearTo, fp4 _ZClearTo, int _StencilClearTo, CRct _ClearArea)
{
	return false;
}


int CXR_VBManager::GetNextVBIndex()
{
	if(m_iScopeStack == -1)
		Error("CXR_VBManager::GetNextVBIndex", "Must be inside scope for GetNextVBIndex to be valid");
//	int iActiveScope = m_lScopeStack[m_iScopeStack];
	return m_pActiveScope->m_lpVB.Len();
}

CXR_VertexBuffer* CXR_VBManager::GetVB(int _iVB)
{
	if(m_iScopeStack == -1)
		Error("CXR_VBManager::GetVB", "Must be inside scope for GetVB to be valid");
//	int iActiveScope = m_lScopeStack[m_iScopeStack];
	return m_pActiveScope->m_lpVB[_iVB].m_pVB;
}

// Internal_Begin assumes m_AddLock and m_AllocLock has been taken
void CXR_VBManager::Internal_Begin(CRenderContext* _pRC, const CRC_Viewport* _pVP)
{
	MAUTOSTRIP(CXR_VBManager_Internal_Begin, MAUTOSTRIP_VOID);
#ifdef PLATFORM_DOLPHIN
	if (m_nToken >= 0)
	{
		CRenderContextDolphin* pRC = safe_cast<CRenderContextDolphin>(_pRC);
		pRC->WaitForToken(m_nToken);
		GXInvalidateVtxCache();
		m_nToken = -1;
	}
#endif

#if	THREAD_ERROR_CHECKING
	if((m_State != CXR_VBMANAGER_STATE_PREBEGIN) && (m_State != CXR_VBMANAGER_STATE_POSTBEGIN))
		Error("CXR_VBManager::Internal_Begin", "CXR_VBManager::Internal_Begin called without CXR_VBManager::End being called");
#endif	// THREAD_ERROR_CHECKING
	m_State	= CXR_VBMANAGER_STATE_RENDERING;

	m_lSortScopes.Clear();

	m_AllocPos = 0;
	m_bOutOfMemory = false;
	m_nBuffers = 0;
	m_iScopeStack	= -1;

	m_iVPStack	= 0;
	m_nVPHeap = 1;
#if VBM_OVERWRITE_DEBUGGING
	memcpy(aMatchBuffer, m_pHeap, sizeof(aMatchBuffer));
#endif //VBM_OVERWRITE_DEBUGGING
	m_lpVPHeap[m_nVPHeap]	= (CRC_Viewport*)Alloc(sizeof(CRC_Viewport));
	*m_lpVPHeap[m_nVPHeap]	= *_pVP;
	m_liVPStack[m_iVPStack]	= m_nVPHeap;
	m_nVPHeap++;

	memset(&m_lpClipHeap, 0, sizeof(m_lpClipHeap));
	m_lpClipHeap[0] = NULL;
	m_nClipHeap = 1;
	m_iClipStack = 0;
	m_liClipStack[0] = 0;

	m_pCurrentRC = _pRC;

#ifdef M_Profile
	m_Stats_nVB = 0;
	m_Stats_nAttrib = 0;
	m_Stats_nLights = 0;
	m_Stats_nXRLights = 0;
	m_Stats_nM4 = 0;
	m_Stats_nV4 = 0;
	m_Stats_nV3 = 0;
	m_Stats_nV2 = 0;
	m_Stats_nInt16 = 0;
	m_Stats_nInt32 = 0;
	m_Stats_nFp4 = 0;
	m_Stats_nPixel32 = 0;
	m_Stats_nOpen = 0;
	m_Stats_nChains = 0;
	m_Stats_nChainVBs = 0;
	m_Stats_nRenderSurface = 0;
#endif

/*	CRenderContextGL* pRCGL = TDynamicCast<CRenderContextGL>(_pRC);
	if (pRCGL)
	{
		m_pHeapHW = (uint8*)pRCGL->VB_GetArrayHeap(1024*1024);
		m_HeapSizeHW = (m_pHeapHW) ? 1024*1024 : 0;
		m_AllocPosHW = 0;
	}*/
}

void CXR_VBManager::Begin(CRenderContext* _pRC, const CRC_Viewport* _pVP)
{
	MAUTOSTRIP(CXR_VBManager_Begin, MAUTOSTRIP_VOID);
#if	THREAD_ERROR_CHECKING
	if(MRTC_SystemInfo::OS_GetThreadID() != m_pOwner)
		Error("CXR_VBManager::Begin", "Begin call by other thread than owner of VB manager");
#endif	// THREAD_ERROR_CHECKING

	{
		M_LOCK(m_AddLock);
		Internal_Begin(_pRC, _pVP);
	}
}

// Internal_Sort assumes m_AddLock has been taken
void CXR_VBManager::Internal_Sort()
{
	MAUTOSTRIP(CXR_VBManager_Internal_Sort, MAUTOSTRIP_VOID);
	int nCount = m_lSortScopes.Len();
	for(int i = 0; i < nCount; i++)
	{
		CXR_VBMScope& Scope = m_lSortScopes[i];
		if(Scope.m_Flags & VBMSCOPE_FL_NEEDSORT)
		{
			Scope.m_lpVB.Sort();
		}
	}
}

void CXR_VBManager::Sort()
{
	MAUTOSTRIP(CXR_VBManager_Sort, MAUTOSTRIP_VOID);
	MSCOPE(CXR_VBManager::Sort, XR_VBM);
	M_LOCK(m_AddLock);
	Internal_Sort();
}

// Internal_End assumes m_AddLock has been taken
void CXR_VBManager::Internal_End()
{
	MAUTOSTRIP(CXR_VBManager_Internal_End, MAUTOSTRIP_VOID);

	m_State++;
#if	THREAD_ERROR_CHECKING
	if(m_State != CXR_VBMANAGER_STATE_POSTBEGIN)
		Error("CXR_VBManager::End", "CXR_VBManager::End without CXR_VBManager::Begin called before");
#endif	// THREAD_ERROR_CHECKING

	if(m_lScopeStack.Len() != 0)
	{
		Error("CXR_VBMAnager::End", "You have to close all scopes before calling end");
	}

	TMeasureResetProfile(m_TSort);
	Internal_Sort();
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	m_RecordUsage = false;
#endif

	m_pCurrentRC = NULL;

//	ConOut(CStrF("VBs %d, Sort %fms", m_lpVB.Len(), Time/GetCPUFrequency()*1000.0));
}

void CXR_VBManager::End()
{
	MAUTOSTRIP(CXR_VBManager_End, MAUTOSTRIP_VOID);
	MSCOPE(CXR_VBManager::End, XR_VBM);
#if	THREAD_ERROR_CHECKING
	if(MRTC_SystemInfo::OS_GetThreadID() != m_pOwner)
		Error("CXR_VBManager::End", "CXR_VBManager::End call by other thread than owner of VB manager");
#endif	// THREAD_ERROR_CHECKING

	M_LOCK(m_AddLock);
	Internal_End();
}

CStr CXR_VBManager::GetInfoString()
{
	MAUTOSTRIP(CXR_VBManager_GetInfoString, CStr());
#ifdef M_Profile
	int Accounted = 
		m_Stats_nVB*sizeof(CXR_VertexBuffer) +
		m_Stats_nAttrib*sizeof(CRC_Attributes) +
		m_Stats_nLights*sizeof(CRC_Light) +
		m_Stats_nXRLights*sizeof(CXR_Light) +
		m_Stats_nM4*sizeof(CMat4Dfp4) +
		m_Stats_nV4*sizeof(CVec4Dfp4) +
		m_Stats_nV3*sizeof(CVec3Dfp4) +
		m_Stats_nV2*sizeof(CVec2Dfp4) +
		m_Stats_nInt16*sizeof(int16) +
		m_Stats_nInt32*sizeof(int32) +
		m_Stats_nFp4*sizeof(fp4) +
		m_Stats_nPixel32*sizeof(CPixel32) + 
		m_Stats_nOpen;

	return CStrF("Acc %d,VB %d(%d),Attr %d(%d),Light %d(%d)/%d(%d),M4 %d(%d),V4 %d(%d),V3 %d(%d),V2 %d(%d),int16 %d(%d),int32 %d(%d),fp4 %d(%d),P32 %d(%d),Open %d, Chains %d, VBC %d",
		Accounted,
		m_Stats_nVB, m_Stats_nVB*sizeof(CXR_VertexBuffer), 
		m_Stats_nAttrib, m_Stats_nAttrib*sizeof(CRC_Attributes), 
		m_Stats_nLights, m_Stats_nLights*sizeof(CRC_Light),
		m_Stats_nXRLights, m_Stats_nXRLights*sizeof(CXR_Light),
		m_Stats_nM4, m_Stats_nM4*sizeof(CMat4Dfp4), 
		m_Stats_nV4, m_Stats_nV4*sizeof(CVec4Dfp4), 
		m_Stats_nV3, m_Stats_nV3*sizeof(CVec3Dfp4), 
		m_Stats_nV2, m_Stats_nV2*sizeof(CVec2Dfp4), 
		m_Stats_nInt16, m_Stats_nInt16*sizeof(int16), 
		m_Stats_nInt32, m_Stats_nInt32*sizeof(int32), 
		m_Stats_nFp4, m_Stats_nFp4*sizeof(fp4),
		m_Stats_nPixel32, m_Stats_nPixel32*sizeof(CPixel32),
		m_Stats_nOpen,
		m_Stats_nChains, m_Stats_nChainVBs );
#else
	return CStr("No info in RTM mode.");
#endif
}


// #define CXR_VB_LOGATTRIBUTES

#ifdef CXR_VB_LOGATTRIBUTES
static void LogAttribute(const CRC_Attributes* _pA)
{
	MAUTOSTRIP(LogAttribute, MAUTOSTRIP_VOID);
	LogFile(CStrF("Txt0 %d, Txt1 %d, Flags %.4x, Raster %d, SrcF %d, DestF %d, ZComp %d, AComp %d, ARef %d, FCol %.8x, FS %f, FE %f, FD %f", 
		_pA->m_TextureID[0], _pA->m_TextureID[1], 
		_pA->m_Flags, _pA->m_RasterMode, _pA->m_SourceBlend, _pA->m_DestBlend,
		_pA->m_ZCompare, _pA->m_AlphaCompare, _pA->m_AlphaRef,
		_pA->m_FogColor, _pA->m_FogStart, _pA->m_FogEnd, _pA->m_FogDensity));
}


static int g_LogWait = 0;
#endif

#ifdef _DEBUG						 
#ifdef PLATFORM_WIN_PC
#include <windows.h>
#endif
#endif

void CXR_VBManager::Flush(CRenderContext* _pRC, int _Flags, int _nBufferSkip, CXR_ViewClipInterface* _pViewClip)
{
}

void CXR_VBManager::Render(CRenderContext* _pRC, int _Flags)
{
	MAUTOSTRIP(CXR_VBManager_Render, MAUTOSTRIP_VOID);
	MSCOPE(CXR_VBManager::Render, XR_VBM);

#if	THREAD_ERROR_CHECKING
	if(m_State != CXR_VBMANAGER_STATE_POSTBEGIN)
		Error("CXR_VBManager::Render", "CXR_VBManager::Render can only be called after a CXR_VBManager::End call");
#endif	// THREAD_ERROR_CHECKING

	{
		int nScopes = m_lSortScopes.Len();
		CXR_VBMScope* pScopes = m_lSortScopes.GetBasePtr();
		_pRC->Viewport_Push();
		_pRC->Clip_Push();
		for(int iScope = 0; iScope < nScopes; iScope++)
		{
			CXR_VBMScope* pScope = &pScopes[iScope];
			if(_Flags & 6)
			{
				if(((_Flags & 2) && (pScope->m_Flags & VBMSCOPE_FL_RENDERWIRE)))
					Internal_Render(pScope, _pRC, _Flags & ~4, -_FP4_MAX, _FP4_MAX);
				else if(((_Flags & 4) && !(pScope->m_Flags & VBMSCOPE_FL_RENDERWIRE)))
					Internal_Render(pScope, _pRC, _Flags & ~4, -_FP4_MAX, _FP4_MAX);
			}
			else
				Internal_Render(pScope, _pRC, _Flags, -_FP4_MAX, _FP4_MAX);
		}
		_pRC->Clip_Pop();
		_pRC->Viewport_Pop();
	}
}


// Internal_Render assumes m_AddLock has been taken
void CXR_VBManager::Internal_Render(CXR_VBMScope* _pScope, CRenderContext* _pRC, int _Flags, fp4 _StartPrio, fp4 _EndPrio)
{
	MAUTOSTRIP(CXR_VBManager_Internal_Render, MAUTOSTRIP_VOID);

	CRC_Viewport** lpVPHeap = m_lpVPHeap;

	TProfileDef(TRender);
	#ifdef	M_Profile
		int nVert = 0;
		int nUnique = 0;
	#endif

	{
		TMeasureProfile(TRender);

	#ifdef PLATFORM_DOLPHIN
		CRenderContextDolphin* pRenderDolphin = safe_cast<CRenderContextDolphin>(_pRC);
		pRenderDolphin->DisableScratchpad(true);
	#endif	

		_pRC->Geometry_Clear();
		_pRC->Attrib_Push();
		_pRC->Matrix_Push();
		CXR_VertexBuffer* pLastVB = NULL;
		CRC_Attributes TempVAttrib;

		const CMat4Dfp4* pLastMat[CRC_MATRIXSTACKS];
		memset(pLastMat, 0, sizeof(pLastMat));
		const CRC_MatrixPalette* pLastMatrixPalette = NULL;
		uint iLastVP = 0;
		uint iLastClip = ~0;

	//	FillChar(pLastAttr, sizeof(pLastAttr), 0);

	#ifdef CXR_VB_LOGATTRIBUTES	
		if (!g_LogWait) 
			g_LogWait = 10;
		else
			g_LogWait--;
		if (!g_LogWait) LogFile("----------------------------------------------------------------");
	#endif

//		int LastPreRender = 0;

		CXR_VBPtr* lpVB = _pScope->m_lpVB.GetBasePtr();
		const int Length = _pScope->m_lpVB.Len();
		const int Count = Length - _pScope->m_nBufferSkip;

		int nMaxLight = _pScope->m_nMaxLights;
		CXR_LightOcclusionInfo* pLO = _pScope->m_pLO;

#ifndef	PLATFORM_PS2
/*		if (pLO)
		{
			for(int i = 0; i < Count; i++)
			{
				CXR_VertexBuffer* pVB = lpVB[i].m_pVB;
				if (!(pVB->m_Flags & CXR_VBFLAGS_LIGHTSCISSOR))
					continue;
				CRC_Attributes* pA = pVB->m_pAttrib;
				if (!pA)
					continue;

				unsigned int iLight = pVB->m_iLight;
				if (iLight > nMaxLight)
					continue;

				if (pA->m_Flags & CRC_FLAGS_SCISSOR)
					pA->m_Scissor.And(pLO[iLight].m_ScissorShaded);
				else
				{
					pA->m_Flags |= CRC_FLAGS_SCISSOR;
					pA->m_Scissor = pLO[iLight].m_ScissorShaded;
				}

				pA->m_Scissor.And(pLO[iLight].m_ScissorVisible);

				if (pA->m_Scissor.m_Min[0] >= pA->m_Scissor.m_Max[0] ||
					pA->m_Scissor.m_Min[1] >= pA->m_Scissor.m_Max[1])
				{
					pA->m_Scissor.m_Min[0] = 0;
					pA->m_Scissor.m_Min[1] = 0;
					pA->m_Scissor.m_Max[0] = 0;
					pA->m_Scissor.m_Max[1] = 0;
				}
			}
		}*/
#endif

		CRC_Attributes* pRCAttrib = (_Flags & 2)?NULL:_pRC->Attrib_Begin();

		int bNullScissor = 0;

//		CRC_VBIDInfo* pVBIDInfo = _pRC->VB_GetVBIDInfo();

		for(int i = 0; i < Count; i++)
		{
			CXR_VertexBuffer* pVB = lpVB[i].m_pVB;
			if (pVB->m_Priority < _StartPrio)
				continue;
			if (pVB->m_Priority > _EndPrio)
				break;

			// Run pre-render on all vertex-buffers with the same priority. 
			// Since the order should not matter we can execute all of them at once, giving the nice side effect that flare depth-tests are performed in sequence.
/*			if (i == LastPreRender)
			{
				fp4 Prior = pVB->m_Priority;
				while(LastPreRender < Length)
				{
					CXR_VertexBuffer* pVB2 = lpVB[LastPreRender].m_pVB;
					if (pVB2->m_Priority != Prior) break;
					if (pVB2->m_pPreRender)
					{
#ifdef _DEBUG						 
						// Check validity of function
						uint32 Test = *((uint32 *)(pVB2->m_pPreRender->m_pfnPreRender));
#endif
						pVB2->m_pPreRender->m_pfnPreRender(_pRC, this, pVB2, pVB2->m_pPreRender->m_pPreRenderContext);
					}
					LastPreRender++;
				}
			}*/

			_pRC->Geometry_Color(pVB->m_Color);

			if (pVB->m_pPreRender)
			{
#ifdef _DEBUG						 
				// Check validity of function
//				*((mint *)(pVB->m_pPreRender->m_pfnPreRender)) = *((mint *)(pVB->m_pPreRender->m_pfnPreRender));
#ifdef PLATFORM_WIN_PC
				if (IsBadCodePtr((FARPROC)pVB->m_pPreRender->m_pfnPreRender))
					M_BREAKPOINT;
#endif
#endif
				if (!(_Flags & 2))
				{
					if (pLastMatrixPalette)
					{
						_pRC->Matrix_SetPalette(NULL);
						pLastMatrixPalette = NULL;
					}

					pVB->m_pPreRender->m_pfnPreRender(_pRC, this, pVB, pVB->m_pPreRender->m_pPreRenderContext, _pScope, _Flags);
				}
			}

			if (!pVB->m_pVBChain) // PreRender only buffer
				continue;
			// Check if the buffer has any primitives at all. (PreRender() on flares will set nPrim to zero if the flare is invisible.)
			if (!pVB->IsVBIDChain() )
			{
				if( !pVB->GetVBChain() || !pVB->GetVBChain()->m_nPrim )
					continue;
			}
/*
			else
			{
				CXR_VBIDChain* pChain = pVB->GetVBIDChain();
				while(pChain)
				{
					// If VBID isn't fresh then it needs some work
					if ((pVBIDInfo[pChain->m_VBID].m_Fresh & 1) == 0)
					{
						// If vbid is allocated and not fresh then precache it
						if (m_pVBCtx->VB_GetFlags(pChain->m_VBID) & CXR_VBFLAGS_ALLOCATED)
							_pRC->Geometry_Precache(pChain->m_VBID);
						else if (pVBIDInfo[pChain->m_VBID].m_Fresh & 2)
						{
							// This VBID has been loaded by renderer and should be released once rendering is finished
						}
					}
					pChain	= pChain->m_pNextVB;
				}
			}
*/
			{
				// Check if attributes changed
				if (!pLastVB || pVB->m_pAttrib != pLastVB->m_pAttrib)
				{
//					if (pVB->m_pAttrib->Compare(*pLastAttr))
					{
	#ifdef	M_Profile
						nUnique++;
	#endif
						if (!(_Flags & 2))
						{
//							_pRC->m_bAttribsHasChangedForSure = true;
//							pVB->SetAttrib(_pRC);
							int AttrChg = 0;
							if (pVB->m_Flags & CXR_VBFLAGS_VIRTUALATTRIBUTES)
							{
								if (pLastVB && (pLastVB->m_Flags & CXR_VBFLAGS_VIRTUALATTRIBUTES))
									AttrChg = pVB->m_pVAttrib->OnSetAttributes(pRCAttrib, pLastVB->m_pVAttrib);
								else
									AttrChg = pVB->m_pVAttrib->OnSetAttributes(pRCAttrib, NULL);
							}
							else
							{
								*pRCAttrib = *pVB->m_pAttrib;
//								pVB->SetAttrib(_pRC);
								AttrChg = -1;
							}

							bNullScissor = 0;

							while(pLO)
							{
								if (!(pVB->m_Flags & CXR_VBFLAGS_LIGHTSCISSOR))
									break;
								CRC_Attributes* pA = pRCAttrib;

								unsigned int iLight = pVB->m_iLight;
								if (iLight > nMaxLight)
									break;

								if (pA->m_Flags & CRC_FLAGS_SCISSOR)
									pA->m_Scissor.And(pLO[iLight].m_ScissorShaded);
								else
								{
									pA->m_Flags |= CRC_FLAGS_SCISSOR;
									AttrChg |= CRC_ATTRCHG_FLAGS;
									pA->m_Scissor = pLO[iLight].m_ScissorShaded;
								}

								pA->m_Scissor.And(pLO[iLight].m_ScissorVisible);

								if (pA->m_Scissor.m_Min[0] >= pA->m_Scissor.m_Max[0] ||
									pA->m_Scissor.m_Min[1] >= pA->m_Scissor.m_Max[1])
								{
									pA->m_Scissor.m_Min[0] = 0;
									pA->m_Scissor.m_Min[1] = 0;
									pA->m_Scissor.m_Max[0] = 0;
									pA->m_Scissor.m_Max[1] = 0;
									bNullScissor = 1;
								}

								AttrChg |= CRC_ATTRCHG_SCISSOR;
								break;
							}

							_pRC->Attrib_End(AttrChg);
						}
						else
						{
							bNullScissor = 0;
							CRC_Attributes* pA = pVB->m_pAttrib;
							if (pVB->m_Flags & CXR_VBFLAGS_VIRTUALATTRIBUTES)
							{
								pVB->m_pVAttrib->OnSetAttributes(&TempVAttrib, NULL);
								pA = &TempVAttrib;
							}


							if (pA->m_Flags & CRC_FLAGS_SCISSOR)
								_pRC->Attrib_Enable(CRC_FLAGS_SCISSOR);
							else
								_pRC->Attrib_Disable(CRC_FLAGS_SCISSOR);

#ifndef	PLATFORM_PS2
							_pRC->Attrib_Scissor(pA->m_Scissor);
#endif	// PLATFORM_PS2
							if (pA->m_lTexGenMode[0] == CRC_TEXGENMODE_SHADOWVOLUME || pA->m_lTexGenMode[0] == CRC_TEXGENMODE_SHADOWVOLUME2)
							{
								_pRC->Attrib_TexGen(0, pA->m_lTexGenMode[0], CRC_TEXGENCOMP_ALL);
								_pRC->Attrib_TexGenAttr(pA->m_pTexGenAttr);
							}
							else
							{
								_pRC->Attrib_TexGen(0, CRC_TEXGENMODE_TEXCOORD, CRC_TEXGENCOMP_ALL);
								_pRC->Attrib_TexGenAttr(NULL);
							}
						}
	#ifdef CXR_VB_LOGATTRIBUTES	
						if (!g_LogWait) LogAttribute(pRCAttrib);
	#endif
					}

				}
			}

			pLastVB = pVB;
			if (bNullScissor)
			{
				continue;
			}

			// Check if matrix state changed
			do
			{
				if (pVB->m_pTransform != pLastMat[0])
				{
					pVB->SetMatrix(_pRC);
					break;
				}

				if (pVB->m_pMatrixPaletteArgs != pLastMatrixPalette)
				{
					pVB->SetMatrix(_pRC);
					break;
				}

				if (pVB->m_pTextureTransform)
				{
					for(int m = 0; m < CRC_MAXTEXCOORDS; m++)
					{
						if (pVB->m_pTextureTransform[m] != pLastMat[1+m])
						{
							pVB->SetMatrix(_pRC);
							break;
						}
					}
				}
				else
				{
					for(int m = 0; m < CRC_MAXTEXCOORDS; m++)
					{
						if (pLastMat[1+m] != 0)
						{
							pVB->SetMatrix(_pRC);
							break;
						}
					}
				}
			}
			while (0);

			if(pVB->m_iVP != iLastVP)
			{
				_pRC->Viewport_Set(lpVPHeap[pVB->m_iVP]);
				iLastVP	= pVB->m_iVP;
			}

			if(pVB->m_iClip != iLastClip)
			{
				CXR_ClipStackEntry* pClip = m_lpClipHeap[pVB->m_iClip];
				if (pClip)
					_pRC->Clip_Set(pClip->m_lPlanes, pClip->m_nPlanes);
				else
					_pRC->Clip_Clear();
				iLastClip	= pVB->m_iClip;
			}

	//pVB->SetMatrix(_pRC);

			// Render
	//		m_nBuffers += m_lpVB[i].m_pVB->RenderGeometry(_pRC);
			m_nBuffers += pVB->RenderGeometry(_pRC);

	#ifdef	M_Profile
			// Count vertices
	//		nVert += m_lpVB[i].m_pVB->m_nV;
//			nVert += pVB->m_nV;
	#endif

			// Set last
			pLastMat[0] = pVB->m_pTransform;
			if (pVB->m_pTextureTransform)
			{
				for(int m = 0; m < CRC_MAXTEXCOORDS; m++)
					pLastMat[1+m] = pVB->m_pTextureTransform[m];
			}
			else
			{
				for(int m = 0; m < CRC_MAXTEXCOORDS; m++)
					pLastMat[1+m] = NULL;
			}
			pLastMatrixPalette = pVB->m_pMatrixPaletteArgs;
		}
		_pRC->Geometry_Clear();
		_pRC->Matrix_Pop();
		_pRC->Attrib_Pop();

	#ifdef PLATFORM_DOLPHIN
		m_nToken = pRenderDolphin->InsertToken();
		pRenderDolphin->DisableScratchpad(false);
	#endif
	}

#ifdef M_Profile
	// Log VB out-of-memory.
	if (OutOfMemory())
	{
		M_TRACEALWAYS("Vertex buffer out of memory. (Heap %d, VBs %d)\n", GetHeap(), _pScope->m_lpVB.Len() );
		ConOut(CStrF("Vertex buffer out of memory. (Heap %d, VBs %d)", GetHeap(), _pScope->m_lpVB.Len() ));
	}

	int AllocPos = m_AllocPos;
	// Log if sorting took more than 1ms.
	if (_Flags & VBM_SHOWTIME)
		ConOut(CStrF("Sort %fms, Render %fms, %d/%d VBs, %d Buffers, %d Unique attr, %d, RS %d, Vert %d, Heap %d, %d", 
		m_TSort.GetTime()*1000.0, TRender.GetTime()*1000.0, Max(0, _pScope->m_lpVB.Len()-_pScope->m_nBufferSkip), _pScope->m_lpVB.Len(), 
		m_nBuffers, nUnique, m_Stats_nRenderSurface, nVert, AllocPos, m_AllocPosHW));

	if (_Flags & 4)
		ConOut( CStrF("VBs %d,Bufs %d,RS %d,Heap %d/%d,Mem:%s\n", _pScope->m_lpVB.Len(), m_nBuffers, m_Stats_nRenderSurface, AllocPos, m_HeapSize, GetInfoString().Str()) );
	if (_Flags & 8)
		M_TRACEALWAYS("VBs %d,Bufs %d,RS %d,Heap %d/%d,Mem:%s\n", _pScope->m_lpVB.Len(), m_nBuffers, m_Stats_nRenderSurface, AllocPos, m_HeapSize, GetInfoString().Str() );
#endif

}


void CXR_VBManager::IncBufferSkip(int _nStep)
{
	MAUTOSTRIP(CXR_VBManager_IncBufferSkip, MAUTOSTRIP_VOID);
	ConOutL("(CXR_VBManager::IncBufferSkip) Obsolete function.");
//	m_nBufferSkip += _nStep;
}

void CXR_VBManager::DecBufferSkip(int _nStep)
{
	MAUTOSTRIP(CXR_VBManager_DecBufferSkip, MAUTOSTRIP_VOID);
	ConOutL("(CXR_VBManager::DecBufferSkip) Obsolete function.");
//	m_nBufferSkip -= _nStep;
//	if (m_nBufferSkip < 0) m_nBufferSkip = 0;
}

void CXR_VBManager::SetOwner(void* _pOwner)
{
	m_pOwner	= _pOwner;
}

void CXR_VBManager::ScopeBegin(bool _bNeedSort, int _MaxVB)
{
#if	THREAD_ERROR_CHECKING
	if(MRTC_SystemInfo::OS_GetThreadID() != m_pOwner)
		Error("CXR_VBManager::ScopeBegin", "Begin call by other thread than owner of VB manager");

	if(m_State != CXR_VBMANAGER_STATE_RENDERING)
		Error("CXR_VBManager::ScopeBegin", "CXR_VBManager::ScopeBegin called outside of Begin/End");
#endif	// THREAD_ERROR_CHECKING

	M_LOCK(m_AddLock);
	CXR_VBMScope NewScope(_bNeedSort);
	if(_MaxVB == 0)
		NewScope.Create(m_MaxVB);
	else
		NewScope.Create(_MaxVB);

	// Inherit flags and properties from parent scope
	if(m_lScopeStack.Len() > 0)
	{
		int iiParent = m_lScopeStack.Len() - 1;
		int iParentScope = m_lScopeStack[iiParent];

		const CXR_VBMScope& ParentScope = m_lSortScopes[iParentScope];
		NewScope.m_Flags	|= ParentScope.m_Flags & ~VBMSCOPE_FL_NEEDSORT;
		NewScope.m_nBufferSkip	= ParentScope.m_nBufferSkip;
		NewScope.m_pLO	= ParentScope.m_pLO;
		NewScope.m_nMaxLights	= ParentScope.m_nMaxLights;
	}

	// Next active scope index
	int iActiveScope = m_lSortScopes.Add(NewScope);
	m_pActiveScope = &m_lSortScopes[iActiveScope];

	m_iScopeStack = m_lScopeStack.Add(iActiveScope);
}

void CXR_VBManager::ScopeEnd()
{
#if	THREAD_ERROR_CHECKING
	if(MRTC_SystemInfo::OS_GetThreadID() != m_pOwner)
		Error("CXR_VBManager::ScopeEnd", "Begin call by other thread than owner of VB manager");

	if(m_State != CXR_VBMANAGER_STATE_RENDERING)
		Error("CXR_VBManager::ScopeEnd", "CXR_VBManager::ScopeEnd called outside of Begin/End");
#endif	// THREAD_ERROR_CHECKING

	M_LOCK(m_AddLock);
	int iActiveScope = m_lScopeStack[m_iScopeStack];

	CXR_VBMScope& SortScope = m_lSortScopes[iActiveScope];
	if(SortScope.m_lpVB.Len() == 0 )
	{
		m_lSortScopes.Del(iActiveScope);
	}

	m_lScopeStack.Del(m_iScopeStack);
	m_iScopeStack	= m_lScopeStack.Len() - 1;

	if(m_iScopeStack >= 0)
	{
		int iActiveScope = m_lScopeStack[m_iScopeStack];
		m_pActiveScope	= &m_lSortScopes[iActiveScope];
	}
	else
		m_pActiveScope	= NULL;
}

void CXR_VBManager::ScopeSetFlags(int _Flags)
{
#if	THREAD_ERROR_CHECKING
	if(MRTC_SystemInfo::OS_GetThreadID() != m_pOwner)
		Error("CXR_VBManager::ScopeSetFlags", "ScopeSetFlags call by other thread than owner of VB manager");
#endif	// THREAD_ERROR_CHECKING

	M_LOCK(m_AddLock);
	m_pActiveScope->m_Flags	|= _Flags;
}

void CXR_VBManager::ScopeClearFlags(int _Flags)
{
#if	THREAD_ERROR_CHECKING
	if(MRTC_SystemInfo::OS_GetThreadID() != m_pOwner)
		Error("CXR_VBManager:ScopeClearFlags", "ScopeClearFlags call by other thread than owner of VB manager");
#endif	// THREAD_ERROR_CHECKING

	M_LOCK(m_AddLock);
	m_pActiveScope->m_Flags	&= ~_Flags;
}

void CXR_VBManager::ScopeSetBufferskip(int _nBufferSkip)
{
#if	THREAD_ERROR_CHECKING
	if(MRTC_SystemInfo::OS_GetThreadID() != m_pOwner)
		Error("CXR_VBManager::ScopeSetBufferskip", "ScopeSetBufferskip call by other thread than owner of VB manager");
#endif	// THREAD_ERROR_CHECKING

	M_LOCK(m_AddLock);
	m_pActiveScope->m_nBufferSkip	= _nBufferSkip;
}

void CXR_VBManager::ScopeSetLightOcclusionMask(CXR_LightOcclusionInfo* _pLO, int _nMaxLight)
{
#if	THREAD_ERROR_CHECKING
	if(MRTC_SystemInfo::OS_GetThreadID() != m_pOwner)
		Error("CXR_VBManager::ScopeSetLightOcclusionMask", "ScopeSetViewclip call by other thread than owner of VB manager");
#endif	// THREAD_ERROR_CHECKING

	M_LOCK(m_AddLock);
	int nSize = sizeof(CXR_LightOcclusionInfo) * _nMaxLight;
	CXR_LightOcclusionInfo* pLO = (CXR_LightOcclusionInfo*)Alloc(nSize);
	if(pLO)
	{
		memcpy(pLO, _pLO, nSize);
		m_pActiveScope->m_pLO = pLO;
		m_pActiveScope->m_nMaxLights = _nMaxLight;
	}
}

CRC_Viewport* CXR_VBManager::Viewport_Get()
{
#ifndef	M_RTM
	if(m_iVPStack < 0)
		Error("CXR_VBManager::Viewport_Get", "No viewport to get");
#endif
	int iVP = m_liVPStack[m_iVPStack];
	return m_lpVPHeap[iVP];
}

CRC_Viewport* CXR_VBManager::Viewport_Get(int _iVP)
{
	return m_lpVPHeap[_iVP];
}

void CXR_VBManager::Viewport_Pop()
{
#if	THREAD_ERROR_CHECKING
	if(MRTC_SystemInfo::OS_GetThreadID() != m_pOwner)
		Error("CXR_VBManager::Viewport_Pop", "Viewport_Pop call by other thread than owner of VB manager");
#endif	// THREAD_ERROR_CHECKING

#ifndef	M_RTM
	if(m_iVPStack <= 0)
		Error("CXR_VBManager::Viewport_Pop", "No viewport entries to pop");
#endif
	m_iVPStack--;
}

bool CXR_VBManager::Viewport_Push(const CRC_Viewport* _pVP)
{
#if	THREAD_ERROR_CHECKING
	if(MRTC_SystemInfo::OS_GetThreadID() != m_pOwner)
		Error("CXR_VBManager::Viewport_Push", "Viewport_Push call by other thread than owner of VB manager");
#endif	// THREAD_ERROR_CHECKING

	int iVP = m_nVPHeap++;
	if(m_nVPHeap >= CXR_VIEWPORT_HEAP_MAX)
		Error("CXR_VBManager::Viewport_Push", "Viewport heap overflow");
	m_lpVPHeap[iVP]	= (CRC_Viewport*)Alloc(sizeof(CRC_Viewport));
	if(m_lpVPHeap[iVP] == 0)
		return false;
	*m_lpVPHeap[iVP]	= *_pVP;

	if(m_iVPStack >= (CXR_VIEWPORT_STACK_MAX - 1))
		Error("CXR_VBManager::Viewport_Push", "Viewport stack overflow");
	m_liVPStack[++m_iVPStack]	= iVP;

	return true;
}

int CXR_VBManager::Viewport_GetCurrentIndex()
{
#if	THREAD_ERROR_CHECKING
	if(MRTC_SystemInfo::OS_GetThreadID() != m_pOwner)
		Error("CXR_VBManager::Viewport_GetCurrentIndex", "Viewport_GetCurrentIndex call by other thread than owner of VB manager");
#endif	// THREAD_ERROR_CHECKING

#ifndef	M_RTM
	if(m_iVPStack < 0)
		Error("CXR_VBManager::Viewport_Get", "No viewport to get current index for");
#endif
	return m_liVPStack[m_iVPStack];
}

int CXR_VBManager::Viewport_Add(const CRC_Viewport* _pVP)
{
	CRC_Viewport* pVP	= (CRC_Viewport*)Alloc(sizeof(CRC_Viewport));
	if(pVP == 0)
		return 0;

	M_LOCK(m_AddLock);
	if(m_nVPHeap >= CXR_VIEWPORT_HEAP_MAX)
		Error("CXR_VBManager::Viewport_Add", "Viewport heap overflow");

	*pVP	= *_pVP;
	m_lpVPHeap[m_nVPHeap]	= pVP;
	m_nVPHeap++;
	return m_nVPHeap - 1;
}

CXR_ClipStackEntry* CXR_VBManager::Clip_Get(int _iClip)
{
	return m_lpClipHeap[_iClip];
}

int CXR_VBManager::Clip_Add(const CPlane3Dfp4* _pPlanes, int _nPlanes, const CMat4Dfp4* _pTransform)
{
	CXR_ClipStackEntry* pClip	= (CXR_ClipStackEntry*)Alloc(sizeof(CXR_ClipStackEntry));
	if(!pClip)
		return 0;

	M_LOCK(m_AddLock);
	if(m_nClipHeap >= CXR_CLIP_HEAP_MAX)
		Error("CXR_VBManager::Clip_Add", "Clip_Add heap overflow");

	pClip->Create(_pPlanes, _nPlanes, _pTransform);
	m_lpClipHeap[m_nClipHeap] = pClip;
	m_nClipHeap++;
	return m_nClipHeap - 1;
}

bool CXR_VBManager::Clip_Push(const CPlane3Dfp4* _pPlanes, int _nPlanes, const CMat4Dfp4* _pTransform)
{
#if	THREAD_ERROR_CHECKING
	if(MRTC_SystemInfo::OS_GetThreadID() != m_pOwner)
		Error("CXR_VBManager::Clip_Push", "Clip_Push call by other thread than owner of VB manager");
#endif	// THREAD_ERROR_CHECKING

	CXR_ClipStackEntry* pClip	= (CXR_ClipStackEntry*)Alloc(sizeof(CXR_ClipStackEntry));
	if(!pClip)
		return 0;
	pClip->Create(_pPlanes, _nPlanes, _pTransform);

	M_LOCK(m_AddLock);

	int iClip = m_nClipHeap++;
	if(m_nClipHeap >= CXR_CLIP_HEAP_MAX)
		Error("CXR_VBManager::Clip_Push", "Clip_Push heap overflow");
	if(m_iClipStack >= (CXR_CLIP_STACK_MAX - 1))
		Error("CXR_VBManager::Clip_Push", "Clip_Push stack overflow");

	m_lpClipHeap[iClip] = pClip;
	m_liClipStack[++m_iClipStack]	= iClip;
	return true;
}

void CXR_VBManager::Clip_Pop()
{
#if	THREAD_ERROR_CHECKING
	if(MRTC_SystemInfo::OS_GetThreadID() != m_pOwner)
		Error("CXR_VBManager::Clip_Pop", "Clip_Pop call by other thread than owner of VB manager");
#endif	// THREAD_ERROR_CHECKING

#ifndef	M_RTM
	if(m_iClipStack <= 0)
		Error("CXR_VBManager::Clip_Pop", "No clip entries to pop");
#endif
	m_iClipStack--;
}

bint CXR_VBManager::ConvertVertexBuildBuffer(CRC_VertexBuffer  &_Dest, const CRC_BuildVertexBuffer&_Source)
{
//NOTheuntoehu;
	mint Size = _Source.CRC_VertexBuffer_GetSize();
	void *pData = Alloc(Size);
	if (!pData)
		return false;
	_Source.CRC_VertexBuffer_ConvertTo(pData, _Dest);
	return true;
}

bool CXR_VBManager::IsRendering()
{
	return m_State == CXR_VBMANAGER_STATE_RENDERING;
}

void CXR_VBManager::RenderBox(const CBox3Dfp4& _Box, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_VBManager_RenderBox, MAUTOTRIP_VOID);
	CXR_VertexBuffer* pVB = Alloc_VBAttrib();
	if(!pVB)
		return;
	if(!pVB->SetVBChain(this, false))
		return;
	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pVB->m_Color	= _Color;
	pVB->m_Priority	= 10000000.0f;
	CXR_VBChain* pChain = pVB->GetVBChain();
	pChain->m_pV	= Alloc_V3(8);
	pChain->m_nV	= 8;
	pChain->m_piPrim	= (uint16*)Alloc(sizeof(uint16) * 12 * 2);
	pChain->m_nPrim	= 24;
	pChain->m_PrimType	= CRC_RIP_WIRES;
	if(!pChain->m_pV || !pChain->m_piPrim)
		return;

	for(int i = 0; i < 8; i++)
	{
		pChain->m_pV[i].k[0]	= (i & 1)?_Box.m_Min.k[0] : _Box.m_Max.k[0];
		pChain->m_pV[i].k[1]	= (i & 2)?_Box.m_Min.k[1] : _Box.m_Max.k[1];
		pChain->m_pV[i].k[2]	= (i & 4)?_Box.m_Min.k[2] : _Box.m_Max.k[2];
	}

	for(int i = 0; i < 4; i++)
	{
		pChain->m_piPrim[i*2 + 0]	= i;
		pChain->m_piPrim[i*2 + 1]	= i + 1;

		pChain->m_piPrim[8 + i*2 + 0]	= i + 4;
		pChain->m_piPrim[8 + i*2 + 1]	= i + 4 + 1;

		pChain->m_piPrim[16 + i*2 + 0]	= i;
		pChain->m_piPrim[16 + i*2 + 1]	= i + 4;
	}

	AddVB(pVB);
}

void CXR_VBManager::RenderBox(const CMat4Dfp4& _Transform, const CBox3Dfp4& _Box, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_VBManager_RenderBox2, MAUTOTRIP_VOID);
	CBox3Dfp4 Box;
	Box.m_Min	= _Box.m_Min * _Transform;
	Box.m_Max	= _Box.m_Max * _Transform;
	RenderBox(Box, _Color);
}

void CXR_VBManager::RenderWire(const CVec3Dfp4& _p0, const CVec3Dfp4& _p1, CPixel32 _Color)
{
	CXR_VertexBuffer* pVB = Alloc_VBAttrib();
	if(!pVB)
		return;
	if(!pVB->SetVBChain(this, false))
		return;
	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pVB->m_Color	= _Color;
	pVB->m_Priority	= 10000000.0f;
	CXR_VBChain* pChain = pVB->GetVBChain();
	pChain->m_pV	= Alloc_V3(2);
	pChain->m_nV	= 2;
	pChain->m_piPrim	= (uint16*)Alloc(sizeof(uint16) * 2);
	pChain->m_nPrim	= 2;
	pChain->m_PrimType	= CRC_RIP_WIRES;
	if(!pChain->m_pV || !pChain->m_piPrim)
		return;

	pChain->m_pV[0]	= _p0;
	pChain->m_pV[1]	= _p1;
	pChain->m_piPrim[0]	= 0;
	pChain->m_piPrim[1]	= 1;

	AddVB(pVB);
}

void CXR_VBManager::RenderWire(const CMat4Dfp4& _Transform, const CVec3Dfp4& _p0, const CVec3Dfp4& _p1, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_VBManager_RenderWire, MAUTOTRIP_VOID);
	CVec3Dfp4 p0 = _p0 * _Transform;
	CVec3Dfp4 p1 = _p1 * _Transform;
	RenderWire(p0, p1, _Color);
}

void CXR_VBManager::RenderWires(const CMat4Dfp4& _Transform, const CVec3Dfp4* _pV, int _nVertices, CPixel32 _Color, bool _bLoop)
{
	CXR_VertexBuffer* pVB = Alloc_VBAttrib();
	if(!pVB)
		return;
	if(!pVB->SetVBChain(this, false))
		return;
	pVB->m_pTransform	= Alloc_M4(_Transform);
	if(!pVB->m_pTransform)
		return;
	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pVB->m_Color		= _Color;
	pVB->m_Priority		= 10000000.0f;
	CXR_VBChain* pChain = pVB->GetVBChain();
	pChain->m_pV		= Alloc_V3(_nVertices);
	pChain->m_nV		= _nVertices;
	if(_bLoop)
	{
		pChain->m_piPrim	= (uint16*)Alloc(sizeof(uint16) * _nVertices * 2);
		pChain->m_nPrim		= _nVertices * 2;
	}
	else
	{
		pChain->m_piPrim	= (uint16*)Alloc(sizeof(uint16) * _nVertices);
		pChain->m_nPrim		= _nVertices;
	}
	pChain->m_PrimType	= CRC_RIP_WIRES;
	if(!pChain->m_pV || !pChain->m_piPrim)
		return;
	memcpy(pChain->m_pV, _pV, _nVertices * sizeof(CVec3Dfp4));

	if(_bLoop)
	{
		int iLast = _nVertices - 1;
		for(int i = 0; i < _nVertices; i++)
		{
			pChain->m_piPrim[i*2 + 0]	= iLast;
			pChain->m_piPrim[i*2 + 1]	= i;
			iLast	= i;
		}
	}
	else
	{
		for(int i = 0; i < _nVertices; i++)
			pChain->m_piPrim[i]	= i;
	}

	AddVB(pVB);
}

void CXR_VBManager::RenderWires(const CMat4Dfp4& _Transform, const CVec3Dfp4* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color, bool _bLoop)
{
	CXR_VertexBuffer* pVB = Alloc_VBAttrib();
	if(!pVB)
		return;
	if(!pVB->SetVBChain(this, false))
		return;
	pVB->m_pTransform	= Alloc_M4(_Transform);
	if(!pVB->m_pTransform)
		return;
	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pVB->m_Color		= _Color;
	pVB->m_Priority		= 10000000.0f;
	CXR_VBChain* pChain = pVB->GetVBChain();
	pChain->m_pV		= Alloc_V3(_nVertices);
	pChain->m_nV		= _nVertices;
	if(_bLoop)
	{
		pChain->m_piPrim	= (uint16*)Alloc(sizeof(uint16) * _nVertices * 2);
		pChain->m_nPrim		= _nVertices * 2;
	}
	else
	{
		pChain->m_piPrim	= (uint16*)Alloc(sizeof(uint16) * _nVertices);
		pChain->m_nPrim		= _nVertices;
	}
	pChain->m_PrimType	= CRC_RIP_WIRES;
	if(!pChain->m_pV || !pChain->m_piPrim)
		return;
	for(int i = 0; i < _nVertices; i++)
	{
		pChain->m_pV[i]	= _pV[_piV[i]];
	}

	if(_bLoop)
	{
		int iLast = _nVertices - 1;
		for(int i = 0; i < _nVertices; i++)
		{
			pChain->m_piPrim[i*2 + 0]	= iLast;
			pChain->m_piPrim[i*2 + 1]	= i;
			iLast	= i;
		}
	}
	else
	{
		for(int i = 0; i < _nVertices; i++)
			pChain->m_piPrim[i]	= i;
	}

	AddVB(pVB);
}

void CXR_VBManager::RenderWires(const CMat4Dfp4& _Transform, const CVec3Dfp4* _pV, const uint32* _piV, int _nVertices, CPixel32 _Color, bool _bLoop)
{
	CXR_VertexBuffer* pVB = Alloc_VBAttrib();
	if(!pVB)
		return;
	if(!pVB->SetVBChain(this, false))
		return;
	pVB->m_pTransform	= Alloc_M4(_Transform);
	if(!pVB->m_pTransform)
		return;
	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pVB->m_Color		= _Color;
	pVB->m_Priority		= 10000000.0f;
	CXR_VBChain* pChain = pVB->GetVBChain();
	pChain->m_pV		= Alloc_V3(_nVertices);
	pChain->m_nV		= _nVertices;
	if(_bLoop)
	{
		pChain->m_piPrim	= (uint16*)Alloc(sizeof(uint16) * _nVertices * 2);
		pChain->m_nPrim		= _nVertices * 2;
	}
	else
	{
		pChain->m_piPrim	= (uint16*)Alloc(sizeof(uint16) * _nVertices);
		pChain->m_nPrim		= _nVertices;
	}
	pChain->m_PrimType	= CRC_RIP_WIRES;
	if(!pChain->m_pV || !pChain->m_piPrim)
		return;
	for(int i = 0; i < _nVertices; i++)
	{
		pChain->m_pV[i]	= _pV[_piV[i]];
	}

	if(_bLoop)
	{
		int iLast = _nVertices - 1;
		for(int i = 0; i < _nVertices; i++)
		{
			pChain->m_piPrim[i*2 + 0]	= iLast;
			pChain->m_piPrim[i*2 + 1]	= i;
			iLast	= i;
		}
	}
	else
	{
		for(int i = 0; i < _nVertices; i++)
			pChain->m_piPrim[i]	= i;
	}

	AddVB(pVB);
}



bool CXR_VBManager::InsertTexGen(CRC_Attributes* _pA, int _nTexGen, const int* _pTexGenModes, const int* _pTexGenComp, const int* _piTexGenChannel, fp4** _pTexGenAttr)
{
	if (_pA->m_pTexGenAttr)
	{
		fp4* lpTexGenAttr[CRC_MAXTEXCOORDS];
		int lTexGenSize[CRC_MAXTEXCOORDS];
		int lNewTexGenAttrOffset[CRC_MAXTEXCOORDS];	// Offset in fp4s

		fp4* pCurrent = _pA->m_pTexGenAttr;

		{
			for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
			{
				lpTexGenAttr[i] = pCurrent;
				int Size = CRC_Attributes::GetTexGenModeAttribSize(_pA->m_lTexGenMode[i], _pA->GetTexGenComp(i));
				lTexGenSize[i] = Size;
				pCurrent += Size;
			}
		}
		{
			for(int i = 0; i < _nTexGen; i++)
				_pA->Attrib_TexGen(_piTexGenChannel[i], _pTexGenModes[i], _pTexGenComp[i]);
		}

		{
			int Offset = 0;
			for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
			{
				lNewTexGenAttrOffset[i] = Offset;
				Offset += CRC_Attributes::GetTexGenModeAttribSize(_pA->m_lTexGenMode[i], _pA->GetTexGenComp(i));
			}
		}

		fp4* pTexGenAttr = Alloc_fp4(pCurrent - (fp4*)NULL);
		if(!pTexGenAttr)
			return false;

		fp4* pTexGenAttrOld = _pA->m_pTexGenAttr;
		{
			for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
			{
				if (lTexGenSize[i])
				{
					memcpy(pTexGenAttr + lNewTexGenAttrOffset[i], lpTexGenAttr[i], lTexGenSize[i]*sizeof(fp4));
				}
			}
		}

		{
			for(int i = 0; i < _nTexGen; i++)
				_pTexGenAttr[i] = pTexGenAttr + lNewTexGenAttrOffset[_piTexGenChannel[i]];

			_pA->Attrib_TexGenAttr(pTexGenAttr);
		}

		return true;
	}
	else
	{
		int TexGenAttrSize = 0;
		{
			for(int i = 0; i < _nTexGen; i++)
			{
				TexGenAttrSize += CRC_Attributes::GetTexGenModeAttribSize(_pTexGenModes[i], _pTexGenComp[i]);
			}
		}
		fp4* pTexGenAttr = Alloc_fp4(TexGenAttrSize);
		if(!pTexGenAttr)
			return false;

		{
			int TexGenAttrOffset = 0;
			for(int i = 0; i < _nTexGen; i++)
			{
				_pTexGenAttr[i] = pTexGenAttr + TexGenAttrOffset;
				TexGenAttrOffset += CRC_Attributes::GetTexGenModeAttribSize(_pTexGenModes[i], _pTexGenComp[i]);

				_pA->Attrib_TexGen(_piTexGenChannel[i], _pTexGenModes[i], _pTexGenComp[i]);
			}

			_pA->Attrib_TexGenAttr(pTexGenAttr);
		}

		return true;
	}
}


// -------------------------------------------------------------------
//  CXR_ClipStackEntry
// -------------------------------------------------------------------
CXR_ClipStackEntry::CXR_ClipStackEntry()
{
	MAUTOSTRIP(CXR_ClipStackEntry_ctor, MAUTOSTRIP_VOID);
	m_nPlanes = 0;
}

void CXR_ClipStackEntry::Create(const CPlane3Dfp4* _pPlanes, int _nPlanes, const CMat4Dfp4* _pTransform)
{
	M_ASSERT(_nPlanes <= CXR_CLIPMAXPLANES, "!");
	m_nPlanes = _nPlanes;
	if (_pTransform)
	{
		for(int i = 0; i < _nPlanes; i++)
		{
			CPlane3Dfp4 P = _pPlanes[i];
			P.Transform(*_pTransform);
			m_lPlanes[i] = P;
		}
	}
	else
		memcpy(&m_lPlanes, _pPlanes, sizeof(CPlane3Dfp4) * _nPlanes);
}

void CXR_ClipStackEntry::Copy(const CXR_ClipStackEntry& _Src, const CMat4Dfp4* _pTransform)
{
	MAUTOSTRIP(CXR_ClipStackEntry_Copy, MAUTOSTRIP_VOID);
	Create(_Src.m_lPlanes+0, _Src.m_nPlanes, _pTransform);
}

