#include "PCH.h"

// #include "../RndrGL/MRndrGL.h"

#include "XRVBManager.h"
#include "XRClass.h"
#include "XRVBContext.h"
#include "../MSystem/Raster/MRCCore.h"

#include "../Classes/Win/MWinGrph.h"
#include "../Classes/Render/MRenderVPGen.h"
#include "XRVBEContext.h"

#ifdef PLATFORM_DOLPHIN
#include "../MRndrDolphin/RenderContext.h"
#endif

class CXR_VBManager2 :public CXR_VBManager
{
public:

	void CheckVBList()
	{
		int nScopes = m_lSortScopes.Len();
		CXR_VBMScope* pScopes = m_lSortScopes.GetBasePtr();
		for(int iScope = 0; iScope < nScopes; iScope++)
		{
			CXR_VBMScope* pScope = &pScopes[iScope];

			int nVbs = 0;
			if (pScope->m_pContainer)
			{
				CXR_VBMScope::CBucketContainer::CContaireIter BucketIter = pScope->m_pContainer->m_UsedContainers;
				while (BucketIter)
				{

					DLinkD_Iter(CXR_VertexBuffer, m_Link) Iter = BucketIter->m_VBs;
					while(Iter)
					{
						++nVbs;
						++Iter;
					}
					++BucketIter;
				}
			}

			if (nVbs != pScope->m_nVBs)
				M_BREAKPOINT;

			int bbla = 0;
		}

	}
};


#ifndef XR_VBMLINKEDLIST
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
#endif


#ifndef XR_VBMLINKEDLIST

//#define XR_VBMSTDQSORT 
#ifdef XR_VBMSTDQSORT

static int QSortCompare_CXR_VBPtr(const void* _pElem1, const void* _pElem2)
{
	const CXR_VBPtr* pE1 = (const CXR_VBPtr*)_pElem1;
	const CXR_VBPtr* pE2 = (const CXR_VBPtr*)_pElem2;

	return pE1->Compare(*pE2);
}
#else
class CVBPtrSortClass
{
public:
	static int Compare(void * _pContext, const CXR_VBPtr &pE1, const CXR_VBPtr &pE2)
	{
	return pE1.Compare(pE2);
	}

};
#endif

void CXR_VBMScope::CVBM_SortableVector::Sort()
{
	if(m_iCurrent > 1)
	{
		TAP_RCNRTM<CXR_VBPtr> Tap;
		Tap.m_pArray = m_lpVBP;
		Tap.m_Len = m_iCurrent;
#ifdef XR_VBMSTDQSORT
		qsort(m_lpVBP, m_iCurrent, sizeof(CXR_VBPtr), QSortCompare_CXR_VBPtr);
#else
		TArray<CXR_VBPtr>::Private_QSortHash_r<CVBPtrSortClass, void >(Tap, 0, m_iCurrent - 1, NULL);
#endif
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

#endif

void CXR_VBMScope::Create(uint32 _MaxVB, const char* _pName)
{
#ifdef XR_VBMLINKEDLIST
//	m_VBs.Construct();
#else
	m_lpVB.Create(_MaxVB);
#endif
	m_pName = _pName;
}

#ifndef XR_VBMLINKEDLIST
#define	THREAD_ERROR_CHECKING	1

int CXR_VBPtr::Compare(const CXR_VBPtr& _pVB) const
{
	if (m_Priority < _pVB.m_Priority)
		return -1;
	if (m_Priority > _pVB.m_Priority)
		return 1;

	const CXR_VertexBuffer *pThisVB = m_pVB;
	const CXR_VertexBuffer *pThatVB = _pVB.m_pVB;

	{
		if (pThisVB->m_pAttrib != pThatVB->m_pAttrib)
		{
			int FlagsThis = pThisVB->m_Flags;
			XR_COMPAREATTRIBUTE_INT(FlagsThis, pThatVB->m_Flags);

			if (FlagsThis & CXR_VBFLAGS_VIRTUALATTRIBUTES)
			{
				XR_COMPAREATTRIBUTE_INT(pThisVB->m_pVAttrib->m_Class, pThatVB->m_pVAttrib->m_Class);
				int Comp = pThisVB->m_pVAttrib->OnCompare(pThatVB->m_pVAttrib);
				if (Comp)
					return Comp;
			}
			else
			{
				int Comp = pThisVB->m_pAttrib->Compare(*pThatVB->m_pAttrib);
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

			int Cmp = memcmp(m_pVB->m_pTransform[m], _pVB.m_pVB->m_pTransform[m], sizeof(CMat4Dfp32));
			if (Cmp)
				return Cmp;
		}
	}*/

	return 0;
}


#endif

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
	gf_RDSendRegisterHeap((mint)this, "VBManager", 0, 0);
#endif

	MACRO_GetRegisterObject(CXR_VBContext, pVBCtx, "SYSTEM.VBCONTEXT");
	m_pVBCtx	= pVBCtx;

	m_pActiveScope = NULL;
	m_pHeap = NULL;
	m_HeapSize = 0;

#ifdef M_Profile
	m_pLastSelectedVB = NULL;
#endif
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
	gf_RDSendRegisterHeap((mint)this, "VBManager", (mint)m_pHeap, (mint)m_pHeap + _HeapSize);

	m_State	= CXR_VBMANAGER_STATE_PREBEGIN;

	m_AllocPos = 0;
	if (mint(m_pHeap) & 15)
		m_AllocPos+=16-mint(m_pHeap) & 15;

#ifdef M_Profile
	m_AllocPosMax = 0;
#endif
	m_MaxVB	= _MaxVB;

	m_lVertexUseMap.SetLen(2048);
	FillChar(m_lVertexUseMap.GetBasePtr(), m_lVertexUseMap.ListSize(), 0);

	CRC_VPFormat::InitRegsNeeded();
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
	return Max(0, GetHeap() - GetAllocPos());
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

//	M_ASSERT((void*)m_AllocLock.m_ThreadID == MRTC_SystemInfo::OS_GetThreadID(), "Must take m_AllocLock before calling Alloc_Open");

#if VBM_OVERWRITE_DEBUGGING
	if(memcmp(m_pHeap + m_AllocPos, m_MatchBuffer, sizeof(m_MatchBuffer)))
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

	_Size = (_Size + 15) & ~15;

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
	memcpy(m_MatchBuffer, m_pHeap + m_AllocPos, sizeof(m_MatchBuffer));
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


void CXR_VBManager::GetAllocParams(int*& _pAllocPos,NThread::CSpinLock*& _pLock,uint8**& _pHeap,mint*& _pHeapSize)
{
	_pAllocPos=&m_AllocPos;
	_pLock=&m_AllocLock;
	_pHeap=&m_pHeap;
	_pHeapSize=&m_HeapSize;
}


void* CXR_VBManager::Alloc(int _Size, uint _bCacheZeroAll)
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
	if(memcmp(m_pHeap + m_AllocPos, m_MatchBuffer, sizeof(m_MatchBuffer)))
	{
		M_BREAKPOINT;
	}
#endif //VBM_OVERWRITE_DEBUGGING

	_Size = (_Size + 15) & ~15;

	int32 AllocPos = m_AllocPos;


	if (AllocPos + _Size <= m_HeapSize)
	{
		uint32 OldCache = (m_AllocPos + 127) & ~127;
		uint32 NewCache = ((m_AllocPos + 127) + _Size) & ~127;
		uint32 nLines = (NewCache - OldCache) >> 7;
		if(nLines > 0)
		{
			void*M_RESTRICT pHeap = m_pHeap;
			if (_bCacheZeroAll)
			{
				while(nLines > 8)
				{
					uint32 ThisCache = OldCache;
					OldCache += 0x400;
					M_PREZERO128(ThisCache, pHeap);
					M_PREZERO128(ThisCache + 0x080, pHeap);
					M_PREZERO128(ThisCache + 0x100, pHeap);
					M_PREZERO128(ThisCache + 0x180, pHeap);
					M_PREZERO128(ThisCache + 0x200, pHeap);
					M_PREZERO128(ThisCache + 0x280, pHeap);
					M_PREZERO128(ThisCache + 0x300, pHeap);
					M_PREZERO128(ThisCache + 0x380, pHeap);
					nLines -= 8;
				}
			}

			if(nLines & 4)
			{
				uint32 ThisCache = OldCache;
				OldCache += 512;
				M_PREZERO128(ThisCache, pHeap);
				M_PREZERO128(ThisCache + 128, pHeap);
				M_PREZERO128(ThisCache + 256, pHeap);
				M_PREZERO128(ThisCache + 384, pHeap);
			}
			if(nLines & 2)
			{
				uint32 ThisCache = OldCache;
				OldCache += 256;
				M_PREZERO128(ThisCache, pHeap);
				M_PREZERO128(ThisCache + 128, pHeap);
			}
			if(nLines & 1)
			{
				uint32 ThisCache = OldCache;
				OldCache += 128;
				M_PREZERO128(ThisCache, pHeap);
			}
		}

		m_AllocPos = AllocPos + _Size;
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
		memcpy(m_MatchBuffer, m_pHeap + m_AllocPos, sizeof(m_MatchBuffer));
#endif //VBM_OVERWRITE_DEBUGGING
		return p;
	}
	else
	{
		M_TRACEALWAYS("Out of VB memory! (tried to allocated: %d)\n", _Size);
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
#ifdef M_Profile
		pVB->m_Link.Construct();
#endif
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
	pPreRender->Create(_pContext, _pFunc);
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
		pVB->m_pFog = Alloc_fp32(_nV);
		if (!pVB->m_pFog) return NULL;
	}*/

	if (_Contents & CXR_VB_TVERTICESALL)
	{
		for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
		{
			int Flag = CXR_VB_TVERTICES0 << i;
			if (_Contents & Flag)
			{
				pChain->m_pTV[i] = (fp32*)Alloc_V2(_nV);
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
	CRC_Attributes*M_RESTRICT pA = (CRC_Attributes*)Alloc(sizeof(CXR_VertexBuffer) + sizeof(CRC_Attributes));
	if (!pA) 
		return NULL;
	CXR_VertexBuffer*M_RESTRICT pVB = (CXR_VertexBuffer*)(pA+1);

	pA->SetDefault();
#ifdef M_Profile
	pVB->m_Link.Construct();
#endif
	pVB->Clear();
	pVB->m_pAttrib = pA;

	return pVB;
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
#ifdef M_Profile
		pVB->m_Link.Construct();
#endif
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

bool CXR_VBManager::Alloc_VBChainCopy(CXR_VertexBuffer* _pDst, CXR_VertexBufferGeometry* _pSrc)
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

const CMat4Dfp32** CXR_VBManager::Alloc_TextureMatrixArray()
{
	const CMat4Dfp32** pArray = (const CMat4Dfp32**)Alloc(sizeof(const CMat4Dfp32*) * CRC_MAXTEXCOORDS);
	if (!pArray)
		return NULL;
	memset(pArray, 0, sizeof(const CMat4Dfp32*) * CRC_MAXTEXCOORDS);
	return pArray;
}

CMat4Dfp32* CXR_VBManager::Alloc_M4_Proj2D(fp32 _Z)
{
	CMat4Dfp32* pM = Alloc_M4();
	if (!pM)
		return NULL;
	Viewport_Get()->Get2DMatrix(*pM, _Z);
	return pM;
}

CMat4Dfp32* CXR_VBManager::Alloc_M4_Proj2DRelBackPlane(fp32 _BackPlaneOfs)
{
	CMat4Dfp32* pM = Alloc_M4();
	if (!pM)
		return NULL;
	Viewport_Get()->Get2DMatrix_RelBackPlane(*pM, _BackPlaneOfs);
	return pM;
}

CMat4Dfp32* CXR_VBManager::Alloc_M4_Proj2D(CRC_Viewport* _pVP, fp32 _Z)
{
	CMat4Dfp32* pM = Alloc_M4();
	if (!pM)
		return NULL;
	_pVP->Get2DMatrix(*pM, _Z);
	return pM;
}

CMat4Dfp32* CXR_VBManager::Alloc_M4_Proj2DRelBackPlane(CRC_Viewport* _pVP, fp32 _BackPlaneOfs)
{
	CMat4Dfp32* pM = Alloc_M4();
	if (!pM)
		return NULL;
	_pVP->Get2DMatrix_RelBackPlane(*pM, _BackPlaneOfs);
	return pM;
}

CRC_Attributes* CXR_VBManager::Alloc_Attrib()
{
	MAUTOSTRIP(CXR_VBManager_Alloc_Attrib, NULL);
#ifdef M_Profile
	m_Stats_nAttrib++;
#endif
	return (CRC_Attributes*) Alloc(sizeof(CRC_Attributes));
}

CRC_Attributes* CXR_VBManager::Alloc_Attrib(int _nCount)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_Attrib, NULL);
#ifdef M_Profile
	m_Stats_nAttrib += _nCount;
#endif
	return (CRC_Attributes*) Alloc(sizeof(CRC_Attributes) * _nCount);
}

CRC_MatrixPalette* CXR_VBManager::Alloc_MP()
{
	MAUTOSTRIP(CXR_VBManager_Alloc_Lights, NULL);
#ifdef M_Profile
	m_Stats_nMP++;
#endif
	void* pMem = Alloc(sizeof(CRC_MatrixPalette));
	CRC_MatrixPalette* pMP = NULL;
	if(pMem)
		pMP = new (pMem) CRC_MatrixPalette;
	return pMP;
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

CMat4Dfp32* CXR_VBManager::Alloc_M4()
{
	MAUTOSTRIP(CXR_VBManager_Alloc_M4, NULL);
#ifdef M_Profile
	m_Stats_nM4++;
#endif
	return (CMat4Dfp32*) Alloc(sizeof(CMat4Dfp32));
}

CMat4Dfp32* CXR_VBManager::Alloc_M4(const CMat4Dfp32& _Mat)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_M4_2, NULL);
#ifdef M_Profile
	m_Stats_nM4++;
#endif
	CMat4Dfp32* pMat = (CMat4Dfp32*) Alloc(sizeof(CMat4Dfp32));
	if (pMat) *pMat = _Mat;
	return pMat;
}


CMat43fp32* CXR_VBManager::Alloc_M43()
{
#ifdef M_Profile
	m_Stats_nM4++;
#endif
	return (CMat43fp32*) Alloc(sizeof(CMat43fp32));
}

CMat43fp32* CXR_VBManager::Alloc_M43(const CMat43fp32& _Mat)
{
#ifdef M_Profile
	m_Stats_nM4++;
#endif
	CMat43fp32* pMat = (CMat43fp32*) Alloc(sizeof(CMat43fp32));
	if (pMat) *pMat = _Mat;
	return pMat;
}

CMat4Dfp32* CXR_VBManager::Alloc_M4(const CMat43fp32& _Mat)
{
#ifdef M_Profile
	m_Stats_nM4++;
#endif
	CMat4Dfp32* pMat = (CMat4Dfp32*) Alloc(sizeof(CMat4Dfp32));
	if (pMat) _Mat.Assign4x4(*pMat);
	return pMat;
}

CMat43fp32* CXR_VBManager::Alloc_M43(const CMat4Dfp32& _Mat)
{
#ifdef M_Profile
	m_Stats_nM4++;
#endif
	CMat43fp32* pMat = (CMat43fp32*) Alloc(sizeof(CMat43fp32));
	if (pMat)
	{
		pMat->CreateFrom(_Mat);
	}
	return pMat;
}

CVec4Dfp32* CXR_VBManager::Alloc_V4(int _nV)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_V4, NULL);
#ifdef M_Profile
	m_Stats_nV4 += _nV;
#endif
	return (CVec4Dfp32*) AllocArray(sizeof(CVec4Dfp32) * _nV);
}

CVec3Dfp32* CXR_VBManager::Alloc_V3(int _nV)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_V3, NULL);
#ifdef M_Profile
	m_Stats_nV3 += _nV;
#endif
	return (CVec3Dfp32*) AllocArray(sizeof(CVec3Dfp32) * _nV);
}

CVec2Dfp32* CXR_VBManager::Alloc_V2(int _nV)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_V2, NULL);
#ifdef M_Profile
	m_Stats_nV2 += _nV;
#endif
	return (CVec2Dfp32*) AllocArray(sizeof(CVec2Dfp32) * _nV);
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

fp32* CXR_VBManager::Alloc_fp32(int _nV)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_fp32, NULL);
#ifdef M_Profile
	m_Stats_nFp32 += _nV;
#endif
	return (fp32*) Alloc(sizeof(fp32) * _nV);
}

CPixel32* CXR_VBManager::Alloc_CPixel32(int _nV)
{
	MAUTOSTRIP(CXR_VBManager_Alloc_CPixel32, NULL);
#ifdef M_Profile
	m_Stats_nPixel32 += _nV;
#endif
	return (CPixel32*) AllocArray(sizeof(uint32) * _nV);
}

char* CXR_VBManager::Alloc_Str(const char* _pStr)
{
	uint Len = CStrBase::StrLen(_pStr);
	char* pStr = (char*) Alloc(Len+1);
	if (!pStr)
		return NULL;

	strcpy(pStr, _pStr);
	return pStr;
}



#ifndef M_RTM
static void CheckVB(CXR_VertexBuffer* _pVB)
{
	uint Flags = _pVB->m_Flags;
	if (Flags & CXR_VBFLAGS_VBIDCHAIN)
	{
		CXR_VBIDChain* pChain = _pVB->GetVBIDChain();
		M_ASSERT((Flags & (CXR_VBFLAGS_VBCHAIN | CXR_VBFLAGS_VBIDCHAIN)) == CXR_VBFLAGS_VBIDCHAIN, "");

		while (pChain)
		{
			if (pChain->m_VBID)
			{
				if (pChain->m_PrimType == CRC_RIP_VBID)
				{
				}
				else
				{
					switch (pChain->m_PrimType)
					{
					case CRC_RIP_STREAM:
						{
							CRCPrimStreamIterator StreamIterate(pChain->m_piPrim, pChain->m_nPrim);				
							while (StreamIterate.IsValid())
							{
								switch(StreamIterate.GetCurrentType())
								{
								case CRC_RIP_TRIFAN :
								case CRC_RIP_TRISTRIP :
								case CRC_RIP_TRIANGLES :
									break;
								default:
					M_ASSERT(false, CStrF("CheckVB) Invalid primitive steam"));
									break;
								}
								StreamIterate.Next();
							}
							break;
						}
					case CRC_RIP_TRIANGLES:
					case CRC_RIP_WIRES:
							break;
					default:
						{
							M_ASSERT(false, CStrF("CheckVB) Invalid primitive type: %d", pChain->m_PrimType));
							break;
						}
					}
				}
			}
			else
			{
				M_ASSERT(0, "Dumma dig");
			}
			pChain = pChain->m_pNextVB;
		}
	}
	else if (Flags & CXR_VBFLAGS_VBCHAIN)
	{
		M_ASSERT((Flags & (CXR_VBFLAGS_VBCHAIN | CXR_VBFLAGS_VBIDCHAIN)) == CXR_VBFLAGS_VBCHAIN, "");

		CXR_VBChain* pChain = _pVB->GetVBChain();
		while (pChain)
		{
			switch(pChain->m_PrimType)
			{
			case CRC_RIP_STREAM:
				{
					CRCPrimStreamIterator StreamIterate(pChain->m_piPrim, pChain->m_nPrim);				
					while (StreamIterate.IsValid())
					{
						switch(StreamIterate.GetCurrentType())
						{
						case CRC_RIP_TRIFAN :
						case CRC_RIP_TRISTRIP :
						case CRC_RIP_TRIANGLES :
							break;
						default:
					M_ASSERT(false, CStrF("CheckVB) Invalid primitive steam"));
							break;
						}
						StreamIterate.Next();
					}
				}
				break;
			case CRC_RIP_TRIANGLES:
			case CRC_RIP_WIRES:
				break;

			default:
				{
					M_ASSERT(false, CStrF("CheckVB) Invalid primitive type: %d", pChain->m_PrimType));
//					M_BREAKPOINT;
					break;
				}
			}
			pChain = pChain->m_pNextVB;
		}
	}
}
#endif


#ifdef M_VBDEBUG
void CXR_VBManager::AddVBImp(CXR_VertexBuffer* _pVB, const char *_pFile, int _Line)
#else
void CXR_VBManager::AddVB(CXR_VertexBuffer* _pVB)
#endif
{

	MAUTOSTRIP(CXR_VBManager_AddVB, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_VBManager::AddVB);
//	ConOut(CStrF("(CXR_VBManager::AddVB) %.8x", _pVB));

	M_ASSERT(_pVB, "AddVB() was called with NULL vertexbuffer!");
	if (!_pVB) 
		return;
#ifdef M_Profile
	if ((uint8 *)_pVB < (uint8 *)m_pHeap || (uint8 *)_pVB > (uint8 *)m_pHeap + m_HeapSize)
	{
		M_TRACEALWAYS("(CXR_VBManager::AddVB) pVB (%.8x) is outside heap (%.8x - %.8x)\n", _pVB, m_pHeap, m_pHeap + m_HeapSize);
		M_BREAKPOINT; // VB must reside in heap
	}

	if (m_pLastSelectedVB == _pVB)
	{
		vec128 dummy; M_VSt(M_VZero(), &dummy);
		M_IMPORTBARRIER;	// Just do something that we can breakpoint on.
	}
#endif

#ifndef M_RTM
	static bool bCheck = false;
	if (bCheck)
		CheckVB(_pVB);
#endif

#ifdef M_VBDEBUG
	_pVB->m_pFile = _pFile;
	_pVB->m_Line = _Line;
#endif

	M_ASSERT(!_pVB->m_pVBChain || !_pVB->IsVBIDChain() || _pVB->GetVBIDChain()->m_PrimType != 0, "Invalid VB prim type");
	M_ASSERT(!_pVB->m_pVBChain || _pVB->IsVBIDChain() || _pVB->GetVBChain()->m_PrimType != 0, "Invalid VB prim type");

#ifndef M_RTM
	if (bCheck && _pVB->m_pVBChain)
	{
		MSCOPESHORT(CheckVB2);

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
		M_ASSERT((mint(_pVB->m_pAttrib->m_pTexGenAttr) & 0x0f) == 0, "TexGen attributes must be aligned.");
	}
#endif	// _DEBUG
#endif	// PLATFORM_PS2

	fp32 Prio = _pVB->m_Priority + 100.0f;
	_pVB->m_Priority = Prio;
	uint32 PrioInt = TruncToInt(Prio);

#ifdef _DEBUG
	M_ASSERT(_pVB->IsValid(), "Broken VB");	
#endif

	{

		M_LOCK(m_AddLock);

//		((CXR_VBManager2*)this)->CheckVBList();


		// Check priority
#ifdef _DEBUG 
		M_ASSERT(_pVB->m_Priority > -100.0, "Invalid priorty");
#endif

#if	THREAD_ERROR_CHECKING
		if(m_State != CXR_VBMANAGER_STATE_RENDERING)
			Error("CXR_VBManager::AddVB", "CXR_VBManager::AddVB called outside of Begin/End");
#endif	// THREAD_ERROR_CHECKING

		if(m_iScopeStack == -1)
			Error("CXR_VBManager::AddVB", "Must be inside scope for AddVB to be valid");
//		int iActiveScope = m_lScopeStack[m_iScopeStack];

#ifdef CPU_LITTLEENDIAN
		uint16 VPClip = _pVB->m_iVPClip;

		if (_pVB->m_iVP == 0)
			VPClip = (VPClip & 0xFF00) | (m_liVPStack[m_iVPStack]);

		if((_pVB->m_iClip == 0) && !(_pVB->m_Flags & CXR_VBFLAGS_FORCENOCLIP))
			VPClip = (VPClip & 0x00FF) | (m_liClipStack[m_iClipStack] << 8);

		_pVB->m_iVPClip = VPClip;
#else
		uint16 VPClip = _pVB->m_iVPClip;

		if (_pVB->m_iVP == 0)
			VPClip = (VPClip & 0x00FF) | m_liVPStack[m_iVPStack] << 8;

		if((_pVB->m_iClip == 0) && !(_pVB->m_Flags & CXR_VBFLAGS_FORCENOCLIP))
			VPClip = (VPClip & 0xFF00) | m_liClipStack[m_iClipStack];

		_pVB->m_iVPClip = VPClip;

#endif

		if (m_CurrentCollector.Allocated())
		{
			CVBAddCollector *pCollector = *m_CurrentCollector;
			if (pCollector)
			{
				int iIndex = pCollector->m_iIndex;
				if (iIndex < pCollector->m_MaxLen)
				{
					pCollector->m_pVBs[iIndex] = _pVB;
					pCollector->m_iIndex = iIndex + 1;
				}
			}
		}

#ifdef XR_VBMLINKEDLIST

#if 0
		m_pActiveScope->m_VBs.UnsafeInsert(_pVB);
#else
		CXR_VBMScope::CBucketContainer *pContainer = m_pActiveScope->m_pContainer;
		if (!pContainer)
		{
#if 0

			mint Data = (mint)Alloc(sizeof(CXR_VBMScope::CBucketContainer));
			m_pActiveScope->m_pContainer = pContainer = (CXR_VBMScope::CBucketContainer *)Data;
			for (int i = 0; i < CXR_VBMScope::CBucketContainer::ENumBuckets; ++i)
			{
				pContainer->m_Containers[i].m_VBs.m_Link.Construct();
			}
#else

			mint Data = (mint)Alloc(sizeof(CXR_VBMScope::CBucketContainer) + 127);
			if (!Data)
				return;
			Data = (Data + 127) & (~127);
			m_pActiveScope->m_pContainer = pContainer = (CXR_VBMScope::CBucketContainer *)Data;
			
			mint nClear = sizeof(CXR_VBMScope::CBucketContainer);
			mint nClear4 = nClear & (~511);
			for (int i = 0; i < nClear4; i += 128*4)
			{
				M_ZERO128(i, pContainer);
				M_ZERO128(i+128, pContainer);
				M_ZERO128(i+256, pContainer);
				M_ZERO128(i+384, pContainer);
			}
			for (int i = nClear4; i < nClear; i += 128)
			{
				M_ZERO128(i, pContainer);
			}
#endif
			
			pContainer->m_UsedContainers.Construct();
		}

		if (pContainer)
		{
			uint32 iBucket = 0;
			if (m_pActiveScope->m_Flags & VBMSCOPE_FL_NEEDSORT)
			{
				iBucket = PrioInt;
				if (iBucket >= 2048)
				{
					iBucket = MinMT(MaxMT(iBucket - (2048), 0), CXR_VBMScope::CBucketContainer::ENumBuckets1 - 1) + CXR_VBMScope::CBucketContainer::ENumBuckets0;
				}
				else
				{
					iBucket = MinMT(MaxMT(iBucket, 0), (uint32)CXR_VBMScope::CBucketContainer::ENumBuckets0 - 1);
				}
			}

			CXR_VBMScope::CBucketContainer::CContainer &Container = pContainer->m_Containers[iBucket];
			if (!Container.m_VBs.m_Link.IsInList())
			{
				Container.m_VBs.Construct();
				pContainer->m_UsedContainers.UnsafeInsert(Container);
			}
			++m_pActiveScope->m_nVBs;
#ifdef M_Profile
			if (_pVB->m_Link.IsInList())
				M_BREAKPOINT; // You cannot add a VB twice
#endif
			Container.m_VBs.UnsafeInsert(_pVB);
//			((CXR_VBManager2*)this)->CheckVBList();
//			int blaha = 0;

		}

#endif
#else
		m_pActiveScope->m_lpVB.Add(const_cast<CXR_VBManager*>(this), CXR_VBPtr(_pVB));
#endif
	}
//Please remove potentially spamming conouts before check-in /AO
//	else
//		ConOutD("Invalid vertex-buffer.");
}

#ifdef M_VBDEBUG
void CXR_VBManager::AddVBImp(const CXR_VertexBuffer& _VB, const char *_pFile, int _Line)
#else
void CXR_VBManager::AddVB(const CXR_VertexBuffer& _VB)
#endif
{
	M_ASSERT(!_VB.m_pVBChain || !_VB.IsVBIDChain() || _VB.GetVBIDChain()->m_PrimType != 0, "Invalid VB prim type");
	M_ASSERT(!_VB.m_pVBChain || _VB.IsVBIDChain() || _VB.GetVBChain()->m_PrimType != 0, "Invalid VB prim type");
	MAUTOSTRIP(CXR_VBManager_AddVB_2, MAUTOSTRIP_VOID);
	CXR_VertexBuffer* pVB = Alloc_VB();
	if (pVB)
	{
		*pVB = _VB;
#ifdef M_VBDEBUG
		AddVBImp(pVB, _pFile, _Line);
#else
		AddVB(pVB);
#endif
	}
}

#define XR_LOCALVBM_MAXDEFERREDVB 32

void CXR_VBManager::AddVBArray(CXR_VertexBuffer** _lpVB, uint _nVB)
{
/*	for(int iVB = 0; iVB < _nVB; iVB++)
	{
		AddVB(_lpVB[iVB]);
	}
	return;*/

	M_LOCK(m_AddLock);

	CVBAddCollector* pCollector = NULL;
	if (m_CurrentCollector.Allocated())
		pCollector = *m_CurrentCollector;

/*	uint32 lPrioInt[XR_LOCALVBM_MAXDEFERREDVB];
	for(int i = 0; i < _nVB; i++)
	{
		M_VLdMem32(&_lpVB[i]->m_Priority);
	}
*/

	for(int iVB = 0; iVB < _nVB; iVB++)
	{
		CXR_VertexBuffer* _pVB = _lpVB[iVB];

#ifdef _DEBUG
		M_ASSERT(_pVB->IsValid(), "Broken VB");	
#endif
		{
			// Check priority
#ifdef _DEBUG 
			M_ASSERT(_pVB->m_Priority > -100.0, "Invalid priorty");
#endif

	#if	THREAD_ERROR_CHECKING
			if(m_State != CXR_VBMANAGER_STATE_RENDERING)
				Error("CXR_VBManager::AddVB", "CXR_VBManager::AddVB called outside of Begin/End");
	#endif	// THREAD_ERROR_CHECKING

			if(m_iScopeStack == -1)
				Error("CXR_VBManager::AddVB", "Must be inside scope for AddVB to be valid");
			//		int iActiveScope = m_lScopeStack[m_iScopeStack];

			fp32 Prio = _pVB->m_Priority + 100.0f;
			_pVB->m_Priority = Prio;
			uint32 PrioInt = TruncToInt(Prio);

	#ifdef CPU_LITTLEENDIAN
			uint16 VPClip = _pVB->m_iVPClip;

			if (_pVB->m_iVP == 0)
				VPClip = (VPClip & 0xFF00) | (m_liVPStack[m_iVPStack]);

			if((_pVB->m_iClip == 0) && !(_pVB->m_Flags & CXR_VBFLAGS_FORCENOCLIP))
				VPClip = (VPClip & 0x00FF) | (m_liClipStack[m_iClipStack] << 8);

			_pVB->m_iVPClip = VPClip;
	#else
			uint16 VPClip = _pVB->m_iVPClip;

			if (_pVB->m_iVP == 0)
				VPClip = (VPClip & 0x00FF) | m_liVPStack[m_iVPStack] << 8;

			if((_pVB->m_iClip == 0) && !(_pVB->m_Flags & CXR_VBFLAGS_FORCENOCLIP))
				VPClip = (VPClip & 0xFF00) | m_liClipStack[m_iClipStack];

			_pVB->m_iVPClip = VPClip;

	#endif

			if (pCollector)
			{
				int iIndex = pCollector->m_iIndex;
				if (iIndex < pCollector->m_MaxLen)
				{
					pCollector->m_pVBs[iIndex] = _pVB;
					pCollector->m_iIndex = iIndex + 1;
				}
			}

	#ifdef XR_VBMLINKEDLIST

	#if 0
			m_pActiveScope->m_VBs.UnsafeInsert(_pVB);
	#else
			CXR_VBMScope::CBucketContainer *pContainer = m_pActiveScope->m_pContainer;
			if (!pContainer)
			{
	#if 0

				mint Data = (mint)Alloc(sizeof(CXR_VBMScope::CBucketContainer));
				m_pActiveScope->m_pContainer = pContainer = (CXR_VBMScope::CBucketContainer *)Data;
				for (int i = 0; i < CXR_VBMScope::CBucketContainer::ENumBuckets; ++i)
				{
					pContainer->m_Containers[i].m_VBs.m_Link.Construct();
				}
	#else

				mint Data = (mint)Alloc(sizeof(CXR_VBMScope::CBucketContainer) + 127);
				if (!Data)
					return;
				Data = (Data + 127) & (~127);
				m_pActiveScope->m_pContainer = pContainer = (CXR_VBMScope::CBucketContainer *)Data;

				mint nClear = sizeof(CXR_VBMScope::CBucketContainer);
				mint nClear4 = nClear & (~511);
				for (int i = 0; i < nClear4; i += 128*4)
				{
					M_ZERO128(i, pContainer);
					M_ZERO128(i+128, pContainer);
					M_ZERO128(i+256, pContainer);
					M_ZERO128(i+384, pContainer);
				}
				for (int i = nClear4; i < nClear; i += 128)
				{
					M_ZERO128(i, pContainer);
				}
	#endif

				pContainer->m_UsedContainers.Construct();
			}

			if (pContainer)
			{
				uint32 iBucket = 0;
				if (m_pActiveScope->m_Flags & VBMSCOPE_FL_NEEDSORT)
				{
					iBucket = PrioInt;
					if (iBucket >= 2048)
					{
						iBucket = MinMT(MaxMT(iBucket - (2048), 0), CXR_VBMScope::CBucketContainer::ENumBuckets1 - 1) + CXR_VBMScope::CBucketContainer::ENumBuckets0;
					}
					else
					{
						iBucket = MinMT(MaxMT(iBucket, 0), (uint32)CXR_VBMScope::CBucketContainer::ENumBuckets0 - 1);
					}
				}

				CXR_VBMScope::CBucketContainer::CContainer &Container = pContainer->m_Containers[iBucket];
				if (!Container.m_VBs.m_Link.IsInList())
				{
					Container.m_VBs.Construct();
					pContainer->m_UsedContainers.UnsafeInsert(Container);
				}
				++m_pActiveScope->m_nVBs;
	#ifdef M_Profile
				if (_pVB->m_Link.IsInList())
					M_BREAKPOINT; // You cannot add a VB twice
	#endif
				Container.m_VBs.UnsafeInsert(_pVB);
				//			((CXR_VBManager2*)this)->CheckVBList();
				//			int blaha = 0;

			}

	#endif
	#else
			m_pActiveScope->m_lpVB.Add(const_cast<CXR_VBManager*>(this), CXR_VBPtr(_pVB));
	#endif
		}
	}
}

bool CXR_VBManager::AddCallback(PFN_VERTEXBUFFER_PRERENDER _pfnCallback, void* _pContext, fp32 _Priority, uint32 _VBEColor)
{
	CXR_VertexBuffer *pVB = Alloc_VB();
	if (!pVB)
		return false;

	CXR_VertexBuffer_PreRender* pPreRender = (CXR_VertexBuffer_PreRender*) Alloc(sizeof(CXR_VertexBuffer_PreRender));
	if (!pPreRender)
		return false;

	pPreRender->Create(_pContext, _pfnCallback);

	pVB->m_pPreRender = pPreRender;
	pVB->m_Priority = _Priority;
	pVB->m_Flags |= CXR_VBFLAGS_PRERENDER;
	pVB->SetVBEColor(_VBEColor);

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

bool CXR_VBManager::AddRenderOptions(fp32 _Priority, uint32 _Options, uint32 _Format)
{
	CXR_VBManager_RenderOptions* pData = (CXR_VBManager_RenderOptions*)Alloc(sizeof(CXR_VBManager_RenderOptions));
	if (!pData)
		return false;

	pData->m_Options = _Options;
	pData->m_Format = _Format;
	return AddCallback(CXR_VBManager_RenderOptions::RenderCallback, pData, _Priority);
}

bool CXR_VBManager::AddRenderRange(fp32 _Priority, fp32 _StartRenderPrio, fp32 _EndRenderPrio)
{
	CXR_VBManager_RenderRange* pData = (CXR_VBManager_RenderRange*)Alloc(sizeof(CXR_VBManager_RenderRange));
	if (!pData)
		return false;

	pData->m_StartPrio = _StartRenderPrio;
	pData->m_EndPrio = _EndRenderPrio;
	return AddCallback(CXR_VBManager_RenderRange::RenderCallback, pData, _Priority, 0xff400000);
}

#include "XRVBUtil.h"

bool CXR_VBManager::AddClearRenderTarget(fp32 _Priority, int _WhatToClear, CPixel32 _ColorClearTo, fp32 _ZClearTo, int _StencilClearTo, CRct _ClearArea)
{
	CXR_PreRenderData_RenderTarget_Clear *pData = (CXR_PreRenderData_RenderTarget_Clear*)Alloc(sizeof(CXR_PreRenderData_RenderTarget_Clear));
	if (!pData)
		return false;

	pData->m_ClearArea = _ClearArea;
	pData->m_WhatToClear = _WhatToClear;
	pData->m_ColorClearTo = _ColorClearTo;
	pData->m_ZClearTo = _ZClearTo;
	pData->m_StencilClearTo = _StencilClearTo;

	return AddCallback(CXR_PreRenderData_RenderTarget_Clear::ClearRenderTarget, pData, _Priority, 0xffffc000);
}

bool CXR_VBManager::AddCopyToTexture(fp32 _Priority, CRct _SrcRect, CPnt _Dst, uint16 _TextureID, bint _bContinueTiling, uint16 _Slice, uint32 _iMRT)
{
	CXR_PreRenderData_RenderTarget_CopyToTexture1 *pData = (CXR_PreRenderData_RenderTarget_CopyToTexture1*)Alloc(sizeof(CXR_PreRenderData_RenderTarget_CopyToTexture1));
	if (!pData)
		return false;

	pData->m_SrcRect = _SrcRect;
	pData->m_Dst = _Dst;
	pData->m_bContinueTiling = _bContinueTiling;
	pData->m_TextureID = _TextureID;
	pData->m_Slice = _Slice;
	pData->m_iMRT = _iMRT;
	
	return AddCallback(CXR_PreRenderData_RenderTarget_CopyToTexture1::RenderTarget_CopyToTexture, pData, _Priority, 0xffffff00);
}

bool CXR_VBManager::AddNextClearParams(fp32 _Priority, int _WhatToClear, CPixel32 _ColorClearTo, fp32 _ZClearTo, int _StencilClearTo, CRct _ClearArea)
{
	CXR_PreRenderData_RenderTarget_Clear *pData = (CXR_PreRenderData_RenderTarget_Clear*)Alloc(sizeof(CXR_PreRenderData_RenderTarget_Clear));
	if (!pData)
		return false;

	pData->m_ClearArea = _ClearArea;
	pData->m_WhatToClear = _WhatToClear;
	pData->m_ColorClearTo = _ColorClearTo;
	pData->m_ZClearTo = _ZClearTo;
	pData->m_StencilClearTo = _StencilClearTo;

	return AddCallback(CXR_PreRenderData_RenderTarget_Clear::SetNextClearParams, pData, _Priority);
}

class CXR_PreRenderData_RenderTarget_SetRenderTarget
{
public:

	CRC_RenderTargetDesc m_Desc;
	
	static void Callback(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
	{
		CXR_PreRenderData_RenderTarget_SetRenderTarget* pData = (CXR_PreRenderData_RenderTarget_SetRenderTarget*)_pContext;

		_pRC->RenderTarget_SetRenderTarget(pData->m_Desc);
	}
};

bool CXR_VBManager::AddSetRenderTarget(fp32 _Priority, const CRC_RenderTargetDesc &_Desc)
{
	CXR_PreRenderData_RenderTarget_SetRenderTarget *pData = (CXR_PreRenderData_RenderTarget_SetRenderTarget*)Alloc(sizeof(CXR_PreRenderData_RenderTarget_SetRenderTarget));
	if (!pData)
		return false;

	pData->m_Desc = _Desc;

	return AddCallback(CXR_PreRenderData_RenderTarget_SetRenderTarget::Callback, pData, _Priority, 0xffff0000);

}

class CXR_PreRenderData_RenderTarget_SetEnableMasks
{
public:
	uint32 m_And;
	
	static void Callback(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
	{
		CXR_PreRenderData_RenderTarget_SetEnableMasks* pData = (CXR_PreRenderData_RenderTarget_SetEnableMasks*)_pContext;

		_pRC->RenderTarget_SetEnableMasks(pData->m_And);
	}
};

bool CXR_VBManager::AddRenderTargetEnableMasks(fp32 _Priority, uint32 _And)
{
	CXR_PreRenderData_RenderTarget_SetEnableMasks *pData = (CXR_PreRenderData_RenderTarget_SetEnableMasks*)Alloc(sizeof(CXR_PreRenderData_RenderTarget_SetEnableMasks));
	if (!pData)
		return false;

	pData->m_And = _And;

	return AddCallback(CXR_PreRenderData_RenderTarget_SetEnableMasks::Callback, pData, _Priority);
}

#if 0

CXR_VertexBuffer* CXR_VBManager::GetVB(mint _hVB)
{
#ifdef XR_VBMLINKEDLIST
	if(m_iScopeStack == -1)
		Error("CXR_VBManager::GetVB", "Must be inside scope for GetVB to be valid");
//	int iActiveScope = m_lScopeStack[m_iScopeStack];
	return (CXR_VertexBuffer*)_hVB;
#else
	if(m_iScopeStack == -1)
		Error("CXR_VBManager::GetVB", "Must be inside scope for GetVB to be valid");
//	int iActiveScope = m_lScopeStack[m_iScopeStack];
	return m_pActiveScope->m_lpVB[_hVB].m_pVB;
#endif
}

mint CXR_VBManager::GetCurrentVB()
{
#ifdef XR_VBMLINKEDLIST
	if(m_iScopeStack == -1)
		Error("CXR_VBManager::GetVB", "Must be inside scope for GetVB to be valid");
	return (mint)m_pActiveScope->m_VBs.GetLast();
#else
	if(m_iScopeStack == -1)
		Error("CXR_VBManager::GetVB", "Must be inside scope for GetVB to be valid");
	int Len = m_pActiveScope->m_lpVB.Len();
	if (Len)
		return Len-1;
	else
		return 0;
#endif
}

mint CXR_VBManager::GetNextVB(mint _pVB)
{
#ifdef XR_VBMLINKEDLIST
	if (_pVB)
	{
		CXR_VertexBuffer* pNext = DLinkDA_List(CXR_VertexBuffer, m_Link)::GetNext((CXR_VertexBuffer *)_pVB);
		return (mint)pNext;
	}
	else
	{
		if(m_iScopeStack == -1)
			Error("CXR_VBManager::GetVB", "Must be inside scope for GetVB to be valid");
	//	int iActiveScope = m_lScopeStack[m_iScopeStack];
		return (mint)m_pActiveScope->m_VBs.GetFirst();
	}
#else
	if (_pVB)
	{
		int Len = m_pActiveScope->m_lpVB.Len();
		mint Ret = _pVB + 1;
		if (Ret >= Len)
			return 0;
		return Ret;
	}
	return 0;
#endif
}

#endif

void CXR_VBManager::SetVBAddCollector(CVBAddCollector *_pCollector)
{
	CVBAddCollector ** ppCollector = m_CurrentCollector.Get();
	*ppCollector = _pCollector;
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
	m_nVBs = 0;
	m_iScopeStack	= -1;

	m_iVPStack	= 0;
	m_nVPHeap = 1;
#if VBM_OVERWRITE_DEBUGGING
	memcpy(m_MatchBuffer, m_pHeap, sizeof(m_MatchBuffer));
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
	m_Stats_nFp32 = 0;
	m_Stats_nPixel32 = 0;
	m_Stats_nOpen = 0;
	m_Stats_nChains = 0;
	m_Stats_nChainVBs = 0;
	m_Stats_nRenderSurface = 0;
	m_Stats_nMP = 0;
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

#ifdef XR_VBMLINKEDLIST
class CVBSort
{
public:

	M_INLINE static int Compare(void *_pContext, const CXR_VertexBuffer *_VB0, const CXR_VertexBuffer *_VB1)
	{
		const CXR_VertexBuffer * M_RESTRICT pThisVB = _VB0;
		const CXR_VertexBuffer * M_RESTRICT pThatVB = _VB1;

		uint32 Prio0 = (uint32 &)pThisVB->m_Priority;
		uint32 Prio1 = (uint32 &)pThatVB->m_Priority;

/*		if ((Prio0 & 0x80000000))
		{
			if (!(Prio1 & 0x80000000))
				return -1;

			if (Prio0 > Prio1)
				return -1;
			else if (Prio0 < Prio1)
				return 1;
		}
		else if ((Prio1 & 0x80000000))
			return 1;
*/
		if (Prio0 < Prio1)
			return -1;
		else if (Prio0 > Prio1)
			return 1;

		/*
		if (pThisVB->m_Priority < pThatVB->m_Priority)
			return -1;
		if (pThisVB->m_Priority > pThatVB->m_Priority)
			return 1;
			*/


		{
			if (pThisVB->m_pAttrib != pThatVB->m_pAttrib)
			{
				int FlagsThis = pThisVB->m_Flags;
				XR_COMPAREATTRIBUTE_INT(FlagsThis, pThatVB->m_Flags);

				if (FlagsThis & CXR_VBFLAGS_VIRTUALATTRIBUTES)
				{
					XR_COMPAREATTRIBUTE_INT(pThisVB->m_pVAttrib->m_Class, pThatVB->m_pVAttrib->m_Class);
					int Comp = pThisVB->m_pVAttrib->OnCompare(pThatVB->m_pVAttrib);
					if (Comp)
						return Comp;
				}
				else
				{
					int Comp = pThisVB->m_pAttrib->Compare(*pThatVB->m_pAttrib);
					if (Comp)
						return Comp;
	/*				else
					{
						const_cast<CXR_VBPtr&>(_pVB).m_pVB->m_pAttrib = m_pVB->m_pAttrib;
					}*/
				}
			}
		}
		return 0;
	}

};

class CBucketContainerSort
{
public:

	M_INLINE static int Compare(void *_pContext, const CXR_VBMScope::CBucketContainer::CContainer *_VB0, const CXR_VBMScope::CBucketContainer::CContainer *_VB1)
	{
		if (_VB0 > _VB1)
			return 1;
		else if (_VB0 < _VB1)
			return -1;
		return 0;
	}

};

#endif

void CXR_VBManager::Internal_SortScope(CXR_VBMScope* _pScope)
{
	if (!_pScope->m_pContainer)
		return;

#ifdef M_Profile
	static bint bTraceDistribution = 0;
#endif

#ifdef XR_VBMLINKEDLIST
#if 0
	_pScope->m_VBs.MergeSort<CVBSort>();
#else
	_pScope->m_pContainer->m_UsedContainers.MergeSort<CBucketContainerSort>();

	{
		CXR_VBMScope::CBucketContainer::CContaireIter Iter = _pScope->m_pContainer->m_UsedContainers;
		while (Iter)
		{
#ifdef M_Profile
			if (bTraceDistribution)
			{
				CXR_VBMScope::CVBIterator VBIter = Iter->m_VBs;

				int nVBs = 0;

				while (VBIter)
				{
					M_TRACEALWAYS("%f\n", VBIter->m_Priority);
					++nVBs;
					++VBIter;
				}
				M_TRACEALWAYS("Distribution: %d\n", nVBs);
			}
#endif

			Iter->m_VBs.MergeSort<CVBSort>();

			++Iter;
		}
	}

#endif
#else
	_pScope->m_lpVB.Sort();
#endif

#ifdef M_Profile
	bTraceDistribution = false;
#endif
}

void CXR_VBManager::Internal_Sort()
{
#ifdef XR_VBMLINKEDLIST
	return;
#endif

	MAUTOSTRIP(CXR_VBManager_Internal_Sort, MAUTOSTRIP_VOID);
	int nCount = m_lSortScopes.Len();

	for(int i = 0; i < nCount; i++)
	{
		CXR_VBMScope& Scope = m_lSortScopes[i];
		if(Scope.m_Flags & VBMSCOPE_FL_NEEDSORT)
		{
			Internal_SortScope(&Scope);
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

	m_TSort.Reset();
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
		m_Stats_nM4*sizeof(CMat4Dfp32) +
		m_Stats_nV4*sizeof(CVec4Dfp32) +
		m_Stats_nV3*sizeof(CVec3Dfp32) +
		m_Stats_nV2*sizeof(CVec2Dfp32) +
		m_Stats_nInt16*sizeof(int16) +
		m_Stats_nInt32*sizeof(int32) +
		m_Stats_nFp32*sizeof(fp32) +
		m_Stats_nPixel32*sizeof(CPixel32) + 
		m_Stats_nOpen;

	return CStrF("Acc %d,VB %d(%d),Attr %d(%d),Light %d(%d)/%d(%d),M4 %d(%d),V4 %d(%d),V3 %d(%d),V2 %d(%d),int16 %d(%d),int32 %d(%d),fp32 %d(%d),P32 %d(%d),Open %d, Chains %d, VBC %d",
		Accounted,
		m_Stats_nVB, m_Stats_nVB*sizeof(CXR_VertexBuffer), 
		m_Stats_nAttrib, m_Stats_nAttrib*sizeof(CRC_Attributes), 
		m_Stats_nLights, m_Stats_nLights*sizeof(CRC_Light),
		m_Stats_nXRLights, m_Stats_nXRLights*sizeof(CXR_Light),
		m_Stats_nM4, m_Stats_nM4*sizeof(CMat4Dfp32), 
		m_Stats_nV4, m_Stats_nV4*sizeof(CVec4Dfp32), 
		m_Stats_nV3, m_Stats_nV3*sizeof(CVec3Dfp32), 
		m_Stats_nV2, m_Stats_nV2*sizeof(CVec2Dfp32), 
		m_Stats_nInt16, m_Stats_nInt16*sizeof(int16), 
		m_Stats_nInt32, m_Stats_nInt32*sizeof(int32), 
		m_Stats_nFp32, m_Stats_nFp32*sizeof(fp32),
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
	LogFile(CStrF("Txt0 %d, Txt1 %d, Flags %.4x, SrcF %d, DestF %d, ZComp %d, AComp %d, ARef %d, FCol %.8x, FS %f, FE %f, FD %f", 
		_pA->m_TextureID[0], _pA->m_TextureID[1], 
		_pA->m_Flags, _pA->m_SourceBlend, _pA->m_DestBlend,
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

void CXR_VBManager::Render(CRenderContext* _pRC, int _Flags, CXR_VBEContext* _pEC)
{
	MAUTOSTRIP(CXR_VBManager_Render, MAUTOSTRIP_VOID);
	MSCOPE(CXR_VBManager::Render, XR_VBM);

#if	THREAD_ERROR_CHECKING
	if(m_State != CXR_VBMANAGER_STATE_POSTBEGIN)
		Error("CXR_VBManager::Render", "CXR_VBManager::Render can only be called after a CXR_VBManager::End call");
#endif	// THREAD_ERROR_CHECKING

	{
		TMeasureResetProfile(m_TRender);
		int nScopes = m_lSortScopes.Len();
		CXR_VBMScope* pScopes = m_lSortScopes.GetBasePtr();
		_pRC->Viewport_Push();
		_pRC->Clip_Push();

		fp32 LastScopePrio = _FP32_MAX;
		if (_pEC)
		{
			_Flags |= 8;	// Enable VB profiling
			if (_pEC->m_bRenderToSelection)
			{
				nScopes = Max(0, Min(_pEC->m_iScope+1, nScopes));
			}
			if (_pEC->m_bShowGPUTime)
				_Flags |= 16;
		}

		for(int iScope = 0; iScope < nScopes; iScope++)
		{
			CXR_VBMScope* pScope = &pScopes[iScope];
			int nVB = pScope->m_nVBs;
			if(_pEC && _pEC->m_bRenderToSelection)
			{
				nVB = (iScope == nScopes-1)?Max(1, Min(_pEC->m_iVB + 1, nVB)):nVB;
			}
			if(_Flags & 6)
			{
				if(((_Flags & 2) && (pScope->m_Flags & VBMSCOPE_FL_RENDERWIRE)))
					Internal_Render(pScope, _pRC, _Flags & ~4, -_FP32_MAX, _FP32_MAX, 0, nVB);
				else if(((_Flags & 4) && !(pScope->m_Flags & VBMSCOPE_FL_RENDERWIRE)))
					Internal_Render(pScope, _pRC, _Flags & ~4, -_FP32_MAX, _FP32_MAX, 0, nVB);
			}
			else
				Internal_Render(pScope, _pRC, _Flags, -_FP32_MAX, _FP32_MAX, 0, nVB);
		}
		_pRC->Clip_Pop();
		_pRC->Viewport_Pop();
	}


	if (_pEC)
	{
		if(_pRC->Caps_Flags() & CRC_CAPS_FLAGS_MRT)
		{
			// Restore MRT so shaders will work
			CRC_RenderTargetDesc MRT;
			MRT.Clear();
			_pRC->RenderTarget_SetRenderTarget(MRT);
		}
		VBE_Render(_pRC, _Flags, *_pEC);
	}

#if defined(M_Profile)
	// Log VB out-of-memory.
	if (OutOfMemory())
	{
		M_TRACEALWAYS("Vertex buffer out of memory. (Heap %d, VBs %d)\n", GetHeap(), m_nVBs );
		ConOut(CStrF("Vertex buffer out of memory. (Heap %d, VBs %d)", GetHeap(), m_nVBs ));
	}

	int AllocPos = m_AllocPos;
	// Log if sorting took more than 1ms.
	if (_Flags & VBM_SHOWTIME)
	{
		ConOut(CStrF("Sort %fms, Render %fms, %d VBs, Buffers, %d, RS %d, Heap %d, %d", 
		m_TSort.GetTime()*1000.0, m_TRender.GetTime()*1000.0, m_nVBs, 
		m_nBuffers, m_Stats_nRenderSurface, AllocPos, m_AllocPosHW));
	}

	/*
	static fp64 TSortWorst = 0;
	static int WorstLength = 0;

	fp64 SortTime = m_TSort.GetTimefp64()*1000.0;

	if (SortTime > TSortWorst)
	{
		TSortWorst = SortTime;
		WorstLength = m_nVBs;
	}

	static int iTest = 0; 

	++iTest;
	if ((iTest % 100) == 0)
		M_TRACEALWAYS("Sort Time %f(%d) %f(%d)\n", TSortWorst, WorstLength, SortTime, m_nVBs);
	*/

	if (_Flags & 4)
		ConOut( CStrF("VBs %d,Bufs %d,RS %d,Heap %d/%d,Mem:%s\n", m_nVBs, m_nBuffers, m_Stats_nRenderSurface, AllocPos, m_HeapSize, GetInfoString().Str()) );
	if (_Flags & 16)
		M_TRACEALWAYS("VBs %d,Bufs %d,RS %d,Heap %d/%d,Mem:%s\n", m_nVBs, m_nBuffers, m_Stats_nRenderSurface, AllocPos, m_HeapSize, GetInfoString().Str() );
#endif

}


int CXR_VBManager::VBE_RenderScope(CXR_VBMScope* _pScope, CPnt _Pos, CXR_VBEContext& _EC, uint _bSelected)
{
	uint MaxHeight = 10;

	CRC_Util2D* pUtil2D = _EC.m_pUtil2D;
	CRC_Font* pF = _EC.m_pFont;
	CClipRect cr(CRct(0,0,4095,4095), CPnt(0,0));

	pUtil2D->SetPriorityValues(1000.0f, 0.0001f);
	CFStr ScopeText = CFStrF("Scope '%s', Flg %d, nVB %d", _pScope->m_pName, _pScope->m_Flags, _pScope->GetNumVBs());
	pUtil2D->Text(cr, pF, _Pos.x, _Pos.y, ScopeText.Str(), 0xff80ff80);

	uint MaxWidth = RoundToInt(pF->GetWidth(pF->GetOriginalSize(), ScopeText.Str()));

	const int nVBPerLine = 512;
	const int nMaxLines = nVBPerLine*4 + 100;

	CVec2Dfp32 M_ALIGN(8) lp0[nMaxLines];
	CVec2Dfp32 M_ALIGN(8) lp1[nMaxLines];
	CPixel32 lCol0[nMaxLines];
	CPixel32 lCol1[nMaxLines];

	CXR_VertexBuffer* pCurrentVB = NULL;
	CXR_VBMScope::CBucketContainer::CContaireIter BucketIter;
	BucketIter = _pScope->m_pContainer->m_UsedContainers;
	if (BucketIter)
		pCurrentVB = BucketIter->m_VBs.GetFirst();

	CVec2Dfp32 posf(_Pos.x, _Pos.y);
	posf[1] += 8.0f;
	int iVB = 0;

	while(BucketIter && pCurrentVB)
	{
		uint nVBDrawn = 0;
		uint nLines = 0;
		fp32 xf = 0.0f;
		while(nVBDrawn < nVBPerLine)
		{
			fp32 yf = 0.0f;
			if ((nVBDrawn & 63) == 0)
			{
				lp0[nLines].Set(posf[0] + xf, posf[1]-3.0f);
				lp1[nLines].Set(posf[0] + xf, posf[1]+0.0f);
				lCol0[nLines] = 0xff606060;
				lCol1[nLines] = 0xff202020;
				nLines++;
			}

#if defined(M_Profile)
			fp32 T = ClampRange(pCurrentVB->m_Time.GetTime()*1000000.0f / 10.0f, 10.0f);

			lp0[nLines].Set(posf[0] + xf, posf[1]+10.0f-T);
			lp1[nLines].Set(posf[0] + xf, posf[1]+10.0f);
			lCol0[nLines] = 0xffffc000;
			lCol1[nLines] = 0xff800000;
			nLines++;

			lp0[nLines].Set(posf[0] + xf, posf[1]+11.0f);
			lp1[nLines].Set(posf[0] + xf, posf[1]+15.0f);
			lCol0[nLines] = pCurrentVB->m_VBEColor;
			lCol1[nLines] = pCurrentVB->m_VBEColor;
			nLines++;

			if (_EC.m_bShowGPUTime)
			{
				fp32 T = ClampRange(pCurrentVB->m_GPUTime.GetTime() * 1000.0f * 10.0f, 20.0f);
				lp0[nLines].Set(posf[0] + xf, posf[1]+40.0f-T);
				lp1[nLines].Set(posf[0] + xf, posf[1]+40.0f);
				lCol0[nLines] = 0xff00c0ff;
				lCol1[nLines] = 0xff000080;
				nLines++;
				yf += 20.0f;
			}
#endif

			lp0[nLines].Set(posf[0] + xf, posf[1]+16.0f);
			lp1[nLines].Set(posf[0] + xf, posf[1]+19.0f);
			lCol0[nLines] = pCurrentVB->m_Color;
			lCol1[nLines] = pCurrentVB->m_Color;
			nLines++;
			yf += 20.0f;

			if (_bSelected && (iVB == _EC.m_iVB))
			{
				// Render selection indicator
				CMTime T = CMTime::GetCPU();
				int color = RoundToInt(192.0f + 63.0f*M_Sin(6.0f * T.GetTimeModulus(_PI)));
				CPixel32 SelColor(color, color, color, 255);

				lp0[nLines].Set(posf[0] + xf, posf[1]+yf+0.0f); lp1[nLines].Set(posf[0] + xf, posf[1]+yf+4.0f);
				lCol0[nLines] = SelColor; lCol1[nLines] = SelColor;
				nLines++;
				lp0[nLines].Set(posf[0] + xf, posf[1]+yf+1.0f); lp1[nLines].Set(posf[0] + xf - 3.0f, posf[1]+yf+4.0f);
				lCol0[nLines] = SelColor; lCol1[nLines] = SelColor;
				nLines++;
				lp0[nLines].Set(posf[0] + xf, posf[1]+yf+0.0f); lp1[nLines].Set(posf[0] + xf + 4.0f, posf[1]+yf+4.0f);
				lCol0[nLines] = SelColor; lCol1[nLines] = SelColor;
				nLines++;

				// Copy VB+Attributes to EC
#ifdef M_Profile
				m_pLastSelectedVB = pCurrentVB;
#endif
				_EC.m_pCurrentVB = pCurrentVB;
				_EC.m_CurrentVB = *pCurrentVB;
				_EC.m_CurrentVB.m_Link.Construct();
				if (pCurrentVB->m_Flags & CXR_VBFLAGS_VIRTUALATTRIBUTES)
				{
					pCurrentVB->m_pVAttrib->OnSetAttributes(&_EC.m_CurrentAttr, NULL);
				}
				else
				{
					if (pCurrentVB->m_pAttrib)
						_EC.m_CurrentAttr = *pCurrentVB->m_pAttrib;
					else
						_EC.m_CurrentAttr.Clear();
				}
			}


			iVB++;
			xf += 1.0f;
			nVBDrawn++;

			pCurrentVB = DLinkDA_List(CXR_VertexBuffer, m_Link)::GetNext(pCurrentVB);
			if (!pCurrentVB)
			{
				++BucketIter;
				if (BucketIter)
					pCurrentVB = BucketIter->m_VBs.GetFirst();
				if (!pCurrentVB)
					break;
			}
		}
		pUtil2D->Lines(cr, lp0, lp1, lCol0, lCol1, nLines);

		MaxHeight += 26 + uint(_EC.m_bShowGPUTime)*20;
		MaxWidth = Max(MaxWidth, nVBDrawn);

		posf[1] += 26.0f;
		if (_EC.m_bShowGPUTime)
			posf[1] += 20.0f; 
	}

	pUtil2D->SetPriorityValues(0.0f, 0.0001f);
	pUtil2D->Rect(cr, CRct(_Pos.x, _Pos.y, _Pos.x+MaxWidth+2, _Pos.y+MaxHeight), 0x30000000, NULL);

//	bool CRC_Font::Write(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const wchar* _pStr, 
//		const CVec2Dfp32& _Size, CPixel32 _Color, const CVec2Dfp32& _MinLimit, const CVec2Dfp32& _MaxLimit)

	return MaxHeight;
}

int CXR_VBManager::VBE_RenderVBInfo(CXR_VBEContext& _EC, CPnt _Pos, const CXR_VertexBuffer* _pVB, const CRC_Attributes* _pA)
{
	int DisplayW = _EC.m_GUIRect.GetWidth();
	int DisplayH = _EC.m_GUIRect.GetHeight();

	CRC_Util2D* pUtil2D = _EC.m_pUtil2D;
	CRC_Font* pF = _EC.m_pFont;
	CClipRect cr(CRct(0,0,4095,4095), CPnt(0,0));
	pUtil2D->SetPriorityValues(1000.0f, 0.0001f);

	CRC_Attributes* pAText = Alloc_Attrib();
	if (!pAText)
		return 0;
	pAText->SetDefault();
	pAText->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZCOMPARE);
	pAText->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pUtil2D->SetAttrib(pAText);

	int y = 0;

	const CXR_VertexBuffer* pVB = _pVB;
	CFStr Text;

#ifdef M_Profile
	if (_EC.m_bShowGPUTime)
		Text = CFStrF("Selected VB, Time %f us, GPU %f ms", pVB->m_Time.GetTime() * 1000000.0f, pVB->m_GPUTime.GetTime() * 1000.0f);
	else
		Text = CFStrF("Selected VB, Time %f us", pVB->m_Time.GetTime() * 1000000.0f);
#else
	Text = CFStrF("Selected VB");
#endif
	pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
	Text = CFStrF("Prio %f, Flg %.4x, iL %d, iVP %d, iC %d, Col %.8x", pVB->m_Priority-100.0f, pVB->m_Flags, pVB->m_iLight, pVB->m_iVP, pVB->m_iClip, pVB->m_Color);
	pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
	if (pVB->m_pPreRender)
	{
		if (pVB->m_pPreRender->m_pfnPreRender == CXR_PreRenderData_RenderTarget_CopyToTexture1::RenderTarget_CopyToTexture)
		{
			Text = CFStrF("PreRender: RenderTarget_CopyToTexture, pContext %.8x", pVB->m_pPreRender->m_pPreRenderContext);
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;

			CXR_PreRenderData_RenderTarget_CopyToTexture1* pData = (CXR_PreRenderData_RenderTarget_CopyToTexture1*)pVB->m_pPreRender->m_pPreRenderContext;
			Text = CFStrF("    Area %d, %d, %d, %d, Dest %d, %d", 
				pData->m_SrcRect.p0.x, pData->m_SrcRect.p0.y, pData->m_SrcRect.p1.x, pData->m_SrcRect.p1.y, pData->m_Dst.x, pData->m_Dst.y);
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
			Text = CFStrF("    TxtID %d, Slice %d, iMRT %d, ContinueTiling %d", pData->m_TextureID, pData->m_Slice, pData->m_iMRT, pData->m_bContinueTiling);
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		}
		else if (pVB->m_pPreRender->m_pfnPreRender == CXR_PreRenderData_RenderTarget_Clear::ClearRenderTarget)
		{
			Text = CFStrF("PreRender: RenderTarget_Clear, pContext %.8x", pVB->m_pPreRender->m_pPreRenderContext);
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;

			CXR_PreRenderData_RenderTarget_Clear* pData = (CXR_PreRenderData_RenderTarget_Clear*)pVB->m_pPreRender->m_pPreRenderContext;
			Text = CFStrF("    Clear %.2x, Area %d, %d, %d, %d, Color %.8x, Z %f, Stencil %.2x", 
				pData->m_WhatToClear,
				pData->m_ClearArea.p0.x, pData->m_ClearArea.p0.y, pData->m_ClearArea.p1.x, pData->m_ClearArea.p1.y,
				(uint32)pData->m_ColorClearTo, pData->m_ZClearTo, pData->m_StencilClearTo);
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		}
		else if (pVB->m_pPreRender->m_pfnPreRender == CXR_PreRenderData_RenderTarget_SetRenderTarget::Callback)
		{
			Text = CFStrF("PreRender: RenderTarget_SetRenderTarget, pContext %.8x", pVB->m_pPreRender->m_pPreRenderContext);
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;

			CXR_PreRenderData_RenderTarget_SetRenderTarget* pData = (CXR_PreRenderData_RenderTarget_SetRenderTarget*)pVB->m_pPreRender->m_pPreRenderContext;

			Text = CFStrF("  Options %.4x, ClearBuffers %.2x, Stencil %.2x, Depth %f",
				pData->m_Desc.m_Options,
				pData->m_Desc.m_ClearBuffers,
				pData->m_Desc.m_ClearValueStencil,
				pData->m_Desc.m_ClearValueZ);
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;

			Text = CFStrF("  ClearRect %d, %d, %d, %d",
				pData->m_Desc.m_ClearRect.p0.x, pData->m_Desc.m_ClearRect.p0.y, pData->m_Desc.m_ClearRect.p1.x, pData->m_Desc.m_ClearRect.p1.y);
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;

			Text = CFStrF("  ResolveRect %d, %d, %d, %d",
				pData->m_Desc.m_ResolveRect.p0.x, pData->m_Desc.m_ResolveRect.p0.y, pData->m_Desc.m_ResolveRect.p1.x, pData->m_Desc.m_ResolveRect.p1.y);
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;

			for(uint iC = 0; iC < 4; iC++)
			{
				Text = CFStrF("    TxtID %5d, Slice %d, Fmt %.8x, Clear %s", 
					pData->m_Desc.m_lColorTextureID[iC],
					pData->m_Desc.m_lSlice[iC],
					pData->m_Desc.m_lTextureFormats[iC],
					pData->m_Desc.m_lColorClearColor[iC].GetString().Str());
				pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
			}
		}
		else if (pVB->m_pPreRender->m_pfnPreRender == CXR_PreRenderData_RenderTarget_SetEnableMasks::Callback)
		{
			Text = CFStrF("PreRender: RenderTarget_SetEnableMasks, pContext %.8x", pVB->m_pPreRender->m_pPreRenderContext);
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;

			CXR_PreRenderData_RenderTarget_SetEnableMasks* pData = (CXR_PreRenderData_RenderTarget_SetEnableMasks*)pVB->m_pPreRender->m_pPreRenderContext;
			Text = CFStrF("    EnableMasks %.8x", pData->m_And);
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		}
		else
		{
			CFStr Text = CFStrF("PreRender: pfn %.8x, pContext %.8x", pVB->m_pPreRender->m_pfnPreRender, pVB->m_pPreRender->m_pPreRenderContext);
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		}
	}
	else
	{
		CFStr Text = CFStrF("PreRender: n/a");
		pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
	}

	if (pVB->m_pTransform)
	{
		Text = CFStrF("Transform: %s", pVB->m_pTransform->GetString().Str());
		pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
	}
	else
	{
		CFStr Text = CFStrF("Transform: n/a");
		pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
	}

	if(_EC.m_bShowVBChain)
	{
		if(pVB->IsVBIDChain())
		{
			CXR_VBIDChain* pChain = pVB->GetVBIDChain();
			while(pChain)
			{
				CFStr Text = CFStrF("VBID: %d, PrimType %d, piPrim %.8x, Len %d", pChain->m_VBID, pChain->m_PrimType, pChain->m_piPrim, pChain->m_nPrim);
				pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
				pChain = pChain->m_pNextVB;
			}
		}
		else if(pVB->IsVBChain())
		{
			CXR_VBChain* pChain = pVB->GetVBChain();
			while(pChain)
			{
				CFStr Text = CFStrF("VB Size %d", pChain->m_nV);
				pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
				Text = CFStrF("  Prm Type %d, Size %d, %.8x", pChain->m_PrimType, pChain->m_nPrim, pChain->m_piPrim);
				pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
				Text = CFStrF("  Pos %.8x", pChain->m_pV);
				pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
				if(pChain->m_pN)
				{
					Text = CFStrF("  Nrm %.8x", pChain->m_pN);
					pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
				}
				if(pChain->m_pCol)
				{
					Text = CFStrF("  Col %.8x", pChain->m_pCol);
					pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
				}
				if(pChain->m_pSpec)
				{
					Text = CFStrF("  Spc %.8x", pChain->m_pSpec);
					pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
				}
				if(pChain->m_nMWComp > 0)
				{
					Text = CFStrF("  MWC %.8x", pChain->m_nMWComp);
					pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
					Text = CFStrF("  MI %.8x", pChain->m_pMI);
					pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
					Text = CFStrF("  MW %.8x", pChain->m_pMW);
					pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
				}
				for(int iT = 0; iT < CRC_MAXTEXCOORDS; iT++)
				{
					if(pChain->m_pTV[iT])
					{
						Text = CFStrF("  TC%d Size %d, %.8x", iT, pChain->m_nTVComp[iT], pChain->m_pTV[iT]);
						pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
					}
				}
				pChain = pChain->m_pNextVB;
			}
		}
	}
	else
	{
		CFStr Text = CFStrF("%s",pVB->IsVBIDChain()?"VBIDChain":(pVB->IsVBChain()?"VBChain":"Invalid Chain"));
		pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
	}
	_Pos.y += 8;

	bool bPrintAttr = true;
	if (pVB->m_Flags & CXR_VBFLAGS_VIRTUALATTRIBUTES)
	{
		CFStr Text = CFStrF("Virtual Attribute, Class %d", pVB->m_pVAttrib->m_Class);
		pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
	}
	else
	{
		if (pVB->m_pAttrib)
		{
			CFStr Text = CFStrF("Attribute");
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		}
		else
		{
			CFStr Text = CFStrF("No attributes");
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
			bPrintAttr = false;
		}
	}

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("VBE_RenderVBInfo", "No texture-context in system.");

	if (bPrintAttr)
	{
		const CRC_Attributes* pA = _pA;
		Text = CFStrF("Flags %.8x, Blend %d,%d", pA->m_Flags, pA->m_SourceBlend, pA->m_DestBlend);
		pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		Text = CFStrF("ZComp %d, AComp %d, ARef %.2x", pA->m_ZCompare, pA->m_AlphaCompare, pA->m_AlphaRef);
		pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		Text = CFStrF("Poly offset %f, scale %f", pA->m_PolygonOffsetUnits, pA->m_PolygonOffsetScale);
		pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		Text = CFStrF("Stencil front %d, %d, %d, %d", pA->m_StencilFrontFunc, pA->m_StencilFrontOpFail, pA->m_StencilFrontOpZFail, pA->m_StencilFrontOpZPass);
		pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		Text = CFStrF("Stencil Back %d, %d, %d, %d", pA->m_StencilBackFunc, pA->m_StencilBackOpFail, pA->m_StencilBackOpZFail, pA->m_StencilBackOpZPass);
		pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		Text = CFStrF("Stencil Ref %.2x, And %.2x, Mask %.2x", pA->m_StencilRef, pA->m_StencilFuncAnd, pA->m_StencilWriteMask);
		pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		Text = CFStrF("Scissor %d, %d, %d, %d", pA->m_Scissor.GetMinX(), pA->m_Scissor.GetMinY(), pA->m_Scissor.GetMaxX(), pA->m_Scissor.GetMaxY());
		pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		Text = CFStrF("Fog %.8x, %f - %f, %f", (uint32)pA->m_FogColor, pA->m_FogStart, pA->m_FogEnd, pA->m_FogDensity);
		pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		Text = CFStrF("TexEnv %.4x, %.4x, %.4x, %.4x", pA->m_TexEnvMode[0], pA->m_TexEnvMode[1], pA->m_TexEnvMode[2], pA->m_TexEnvMode[3]);
		pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		y += 8;

		CRC_ExtAttributes* pEA = pA->m_pExtAttrib;
		if (pEA)
		{
			if (pEA->m_AttribType >= CRC_ATTRIBTYPE_FP14 && pEA->m_AttribType <= CRC_ATTRIBTYPE_FP30)
			{
				CRC_ExtAttributes_FragmentProgram14* pFP = (CRC_ExtAttributes_FragmentProgram14*)pEA;
				Text = CFStrF("ExtAttr: Type %d (FP), %s, Ptr %.8x", pFP->m_AttribType, pFP->m_pProgramName, pFP);
				pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
				CVec4Dfp32* pP = pFP->m_pParams;
				for(int iP = 0; iP < pFP->m_nParams; iP++)
				{
					Text = CFStrF("    %d : (%f  %f  %f  %f)", iP, pP[iP][0], pP[iP][1], pP[iP][2], pP[iP][3]);
					pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
				}
			}
			else
			{
				Text = CFStrF("ExtAttr: Type %d, Ptr %.8x", pEA->m_AttribType, pEA);
				pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
			}
		}
		else
		{
			Text = CFStrF("ExtAttr: n/a");
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
		}

		y += 8;

		int iTex = 0;
		CVec4Dfp32* pTexGenAttr = (CVec4Dfp32*)pA->m_pTexGenAttr;
		for(iTex = 0; iTex < CRC_MAXTEXCOORDS; iTex++)
		{
			uint TexGen = pA->m_lTexGenMode[iTex];
			if (TexGen < CRC_TEXGENMODE_MAX)
			{
				Text = CFStrF("%d : iCrd %d, TexGen %d", iTex, pA->m_iTexCoordSet[iTex], TexGen);
				pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80);
				if (pTexGenAttr)
				{
					Text = "";
					int nP = CRC_VPFormat::ms_lnTexGenParams[TexGen];
					for(int i = 0; i < nP; i++)
					{
						Text += CFStrF("(%f  %f  %f  %f), ", pTexGenAttr->k[0], pTexGenAttr->k[1], pTexGenAttr->k[2], pTexGenAttr->k[3]);
						pTexGenAttr++;
					}
					pUtil2D->Text(cr, pF, _Pos.x+100, _Pos.y+y, Text.Str(), 0xff80ff80);
				}
			}
			else
			{
				Text = CFStrF("%d : iCrd %d, TexGen %d (INVALID!)", iTex, pA->m_iTexCoordSet[iTex], TexGen);
				pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80);
				pTexGenAttr = NULL;
			}
			y += 8;
		}

		y += 8;

		int nTxtShow = 0;

		for(iTex = 0; iTex < CRC_MAXTEXTURES; iTex++)
		{
			int TxtID = pA->m_TextureID[iTex];
			Text = CFStrF("%d : TexID %d", iTex, TxtID);
			pUtil2D->Text(cr, pF, _Pos.x, _Pos.y+y, Text.Str(), 0xff80ff80);

			int TxtFlags = pTC->GetTextureFlags(TxtID);
			if (TxtFlags & CTC_TXTIDFLAGS_ALLOCATED)
			{
				CTC_TextureProperties TxtProp;
				CImage TxtDesc;
				int nMip;
				pTC->GetTextureDesc(TxtID, &TxtDesc, nMip);
				pTC->GetTextureProperties(TxtID, TxtProp);
				CStr TxtName = pTC->GetName(TxtID);

				Text = CFStrF("%dx%d, Fmt %.8x, Mem %.8x, nMip %d, Flg %.8x, '%s'", TxtDesc.GetWidth(), TxtDesc.GetHeight(), TxtDesc.GetFormat(), TxtDesc.GetMemModel(), nMip, TxtProp.m_Flags, TxtName.Str());
				pUtil2D->Text(cr, pF, _Pos.x+100, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;

				// Render texture thumbnail
				if (_EC.m_bShowTextures)
				{
					const fp32 ThumbnailSize = 128.0f;
					const int ThumbnailSpacingX = 128+4;
					const int ThumbnailSpacingY = 128+4+10;
					int nThumbsPerCol = DisplayH / ThumbnailSpacingY;
					int iCol = nTxtShow / nThumbsPerCol;
					int iRow = nTxtShow % nThumbsPerCol;

					CRC_Attributes* pAT0 = Alloc_Attrib();
					if (!pAT0)
						return y;
					pAT0->SetDefault();
					pAT0->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZCOMPARE);
					pAT0->Attrib_TextureID(0, TxtID);
					CRC_Attributes* pAT1 = Alloc_Attrib();
					if (!pAT1)
						return y;
					pAT1->SetDefault();
					pAT1->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZCOMPARE);
					pAT1->Attrib_TexEnvMode(0, CRC_TEXENVMODE_RGB_SOURCE1_ALPHA);
					pAT1->Attrib_TextureID(0, TxtID);

					CPnt tpos(_EC.m_GUIRect.p1.x - 2*(ThumbnailSpacingX + ThumbnailSpacingX*iCol), _EC.m_GUIRect.p0.y + ThumbnailSpacingY*iRow);
					Text = CFStrF("TexID %d, %s", TxtID, TxtName.Str());
					pUtil2D->Text(cr, pF, tpos.x, tpos.y, Text.Str(), 0xff80ff80);
					tpos.y += 8;

					CRC_Attributes* pAPrev = pUtil2D->GetAttrib();

					pUtil2D->SetTexture(TxtID);
					pUtil2D->SetAttrib(pAT0);
					pUtil2D->SetTextureOrigo(cr, tpos);

					fp32 w = TxtDesc.GetWidth();
					fp32 h = TxtDesc.GetHeight();
					fp32 MaxSize = Max(w, h);

					if (TxtProp.m_Flags & (CTC_TEXTUREFLAGS_CUBEMAP | CTC_TEXTUREFLAGS_CUBEMAPCHAIN))
					{
						CMat4Dfp32* pMat2D = Alloc_M4_Proj2DRelBackPlane();
						CRC_Attributes* pA = Alloc_Attrib();
						if (!pMat2D || !pA)
							return y;
						pA->SetDefault();
						pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);
						pA->Attrib_TextureID(0, TxtID);
						CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(this, pMat2D, CRect2Duint16(tpos.x, tpos.y, tpos.x+128, tpos.y+128), 0xffffffff, 0.0f, pA);
						if (!pVB)
							return y;
						static fp32 lTVCubeFace0[4][3] = 
						{
							{ 0.2f, -1, -1 },
							{ 0.2f, 1, -1 },
							{ 0.2f, 1, 1 },
							{ 0.2f, -1, 1 }
						};
						pVB->Geometry_TVertexArray((CVec3Dfp32*)lTVCubeFace0, 0);
						AddVB(pVB);
					}
					else
					{
						pUtil2D->SetTextureScale(MaxSize/ThumbnailSize, MaxSize/ThumbnailSize);
						pUtil2D->Rect(cr, CRct(tpos, CPnt(tpos.x + RoundToInt(w/MaxSize*ThumbnailSize), tpos.y + RoundToInt(h/MaxSize*ThumbnailSize))), 0xffffffff);
						tpos.x += ThumbnailSpacingX;
						pUtil2D->SetAttrib(pAT1);
						pUtil2D->SetTextureOrigo(cr, tpos);
						pUtil2D->Rect(cr, CRct(tpos, CPnt(tpos.x + RoundToInt(w/MaxSize*ThumbnailSize), tpos.y + RoundToInt(h/MaxSize*ThumbnailSize))), 0xffffffff);
					}
					pUtil2D->SetTexture(0);

					nTxtShow++;

					pUtil2D->SetAttrib(pAText);
				}
			}
			else
			{
				Text = CFStrF("Invalid ID");
				pUtil2D->Text(cr, pF, _Pos.x+100, _Pos.y+y, Text.Str(), 0xff80ff80); y += 8;
			}
		}
	}

	return y;
}

void CXR_VBManager::VBE_Render(CRenderContext* _pRC, int _Flags, CXR_VBEContext& _EC)
{
	if (!_EC.m_pFont || !_EC.m_pUtil2D)
		return;

	int iScopeVBE = m_lSortScopes.Len();
	int nScopes = iScopeVBE;
	ScopeBegin("VBE", true, 1024);
	while(1)
	{
		CRC_Util2D* pUtil2D = _EC.m_pUtil2D;
		CRC_Font* pF = _EC.m_pFont;
		CClipRect cr(CRct(0,0,4095,4095), CPnt(0,0));
		{
			// Initialise util2d
			uint AllocSize = sizeof(CRC_Attributes)*1;
			uint8* pVBMData = (uint8*) Alloc(AllocSize);
			if (!pVBMData)
				break;
			CRC_Attributes* pA0 = (CRC_Attributes*)pVBMData; pVBMData += sizeof(CRC_Attributes)*1;
			pA0->SetDefault();
			pA0->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZCOMPARE);
			pA0->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			pUtil2D->Begin(NULL, Viewport_Get(), this);
			pUtil2D->SetAttrib(pA0);
		}

/*		{
			CPnt p0 = _EC.m_GUIRect.p0;
			CPnt p1 = _EC.m_GUIRect.p0 + CPnt(_EC.m_GUIRect.p1.x - _EC.m_GUIRect.p0.x, 0);
			CPnt p2 = _EC.m_GUIRect.p0 + CPnt(0, _EC.m_GUIRect.p1.y - _EC.m_GUIRect.p0.y);
			CPnt p3 = _EC.m_GUIRect.p1;
			pUtil2D->Line(cr, p0, p0 + CPnt(32, 0), 0xffffffff);
			pUtil2D->Line(cr, p0, p0 + CPnt(0, 32), 0xffffffff);

			pUtil2D->Line(cr, p1, p1 - CPnt(32, 0), 0xffffffff);
			pUtil2D->Line(cr, p1, p1 + CPnt(0, 32), 0xffffffff);

			pUtil2D->Line(cr, p2, p2 + CPnt(32, 0), 0xffffffff);
			pUtil2D->Line(cr, p2, p2 - CPnt(0, 32), 0xffffffff);

			pUtil2D->Line(cr, p3, p3 - CPnt(32, 0), 0xffffffff);
			pUtil2D->Line(cr, p3, p3 - CPnt(0, 32), 0xffffffff);
		}*/

		CPnt Pos = _EC.m_GUIRect.p0;
		CFStr Text = CFStrF("Vertex Buffer Explorer, %s, Current scope %d, Current VB %d", 
			(_EC.m_bInputEnable) ? "Input Enabled" : "Input Disabled",
			_EC.m_iScope, _EC.m_iVB);
		pUtil2D->Text(cr, pF, Pos.x, Pos.y, Text.Str(), 0xff80ff80); Pos.y += 10;

		_EC.ClearVB();
		for(int iScope = 0; iScope < nScopes; iScope++)
		{
			int h = VBE_RenderScope(&m_lSortScopes[iScope], Pos, _EC, _EC.m_iScope == iScope);
			Pos.y += h+2;
		}

		Pos.y += 10;
		VBE_RenderVBInfo(_EC, Pos, &_EC.m_CurrentVB, &_EC.m_CurrentAttr);

		// Render selected vertex buffer
		if (_EC.m_bShowSelection && _EC.m_pCurrentVB && _EC.m_pCurrentVB->m_pVBChain)
		{
			CXR_VertexBuffer* pVBSrc = _EC.m_pCurrentVB;
			CRC_Attributes* pA = Alloc_Attrib();
			if (!pA)
				break;
			pA->SetDefault();
			pA->Attrib_Disable(CRC_FLAGS_CULL);
			pA->Attrib_TexGen(0, CRC_TEXGENMODE_VOID, 0);
			pA->Attrib_TexGen(1, CRC_TEXGENMODE_VOID, 0);
			pA->Attrib_TexGen(2, CRC_TEXGENMODE_VOID, 0);
			pA->Attrib_TexGen(3, CRC_TEXGENMODE_VOID, 0);
			switch(_EC.m_bShowWithZCompare)
			{
			case 1 : pA->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL); break;
			case 2 : pA->Attrib_ZCompare(CRC_COMPARE_EQUAL); break;
			default : pA->Attrib_ZCompare(CRC_COMPARE_ALWAYS); break;
			}
			pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			CXR_VertexBuffer* pVB = Alloc_VB();
			if (!pVB)
				break;
			pVB->Clear();
			pVB->m_pAttrib = pA;
			pVB->m_Priority = -20.0f;
			CMTime T = CMTime::GetCPU();
			int color = RoundToInt(192.0f + 63.0f*M_Sin(4.0f * T.GetTimeModulus(_PI)));
			pVB->m_Color = CPixel32(color>>1, color, color>>1, color>>1);
			pVB->m_pTransform = pVBSrc->m_pTransform;
			pVB->m_pMatrixPaletteArgs = pVBSrc->m_pMatrixPaletteArgs;
			pVB->CopyVBChain(pVBSrc);
			pVB->m_iVP = pVBSrc->m_iVP;
			if (_EC.m_bShowWithClip)
				pVB->m_iClip = pVBSrc->m_iClip;
			AddVB(pVB);
		}

		break;
	}
	ScopeEnd();

	CXR_VBMScope* pScopeVBE = &m_lSortScopes[iScopeVBE];
	Internal_SortScope(pScopeVBE);
	Internal_Render(pScopeVBE, _pRC, _Flags, -_FP32_MAX, _FP32_MAX);

	m_lSortScopes.SetLen(iScopeVBE);
}

void CXR_VBManager::VBE_GetScopeInfo(CXR_VBEContext& _EC)
{
	_EC.m_lScopeInfo[0].m_nVB = 0;	// Clear scope 0 in case there are none.

	int nScopes = Min(m_lSortScopes.Len(), (int)VBE_MAXSCOPES);
	for(int iScope = 0; iScope < nScopes; iScope++)
	{
		CXR_VBMScope& Scope = m_lSortScopes[iScope];
		_EC.m_lScopeInfo[iScope].m_nVB = Scope.GetNumVBs();
	}

	_EC.m_nScopes = nScopes;
	_EC.m_iScope = Max(0, Min(_EC.m_iScope, _EC.m_nScopes-1));
	_EC.m_iVB = Max(0, Min(_EC.m_iVB, _EC.m_lScopeInfo[_EC.m_iScope].m_nVB-1));
}

bool CXR_VBManager::VBE_ProcessKey(CXR_VBEContext& _EC, CScanKey _Key) const
{
	return _EC.ProcessKey(_Key);
}

#ifdef M_Profile
	#define VBM_PROFILEVB_START if (bProfileVB) pVB->m_Time.Start();
	#define VBM_PROFILEVB_STOP if (bProfileVB) pVB->m_Time.Stop();
	#define VBM_PROFILEVBGPU_NEXT if (bProfileVBGPU) _pRC->PerfQuery_Next();
#else
	#define VBM_PROFILEVB_START
	#define VBM_PROFILEVB_STOP
	#define VBM_PROFILEVBGPU_NEXT
#endif

// Internal_Render assumes m_AddLock has been taken
void CXR_VBManager::Internal_Render(CXR_VBMScope* _pScope, CRenderContext* _pRC, int _Flags, fp32 _StartPrio, fp32 _EndPrio, int _iVB, int _nVB)
{
//	((CXR_VBManager2*)this)->CheckVBList();
	MAUTOSTRIP(CXR_VBManager_Internal_Render, MAUTOSTRIP_VOID);

	if(_nVB == 0)
		_nVB = Max(0, _pScope->m_nVBs - _iVB);

	CRC_Viewport** lpVPHeap = m_lpVPHeap;

	bool bProfileVB = (_Flags & 8) != 0;
	bool bProfileVBGPU = (_Flags & 16) != 0;

	void* pPerfQueryMem = NULL;
	if (bProfileVBGPU)
	{
		pPerfQueryMem = Alloc(Max(sizeof(CMTime), _pRC->PerfQuery_GetStride()) * _pScope->GetNumVBs());
		if (!pPerfQueryMem)
			bProfileVBGPU = false;
	}

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

		const CMat4Dfp32* pLastMat[CRC_MATRIXSTACKS];
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

#ifdef XR_VBMLINKEDLIST
		// Sort containers
		CXR_VBMScope::CBucketContainer::CContaireIter BucketIter;
		bint bNeedSort = (_pScope->m_Flags & VBMSCOPE_FL_NEEDSORT) != 0;

		if (bNeedSort)
		{
			TMeasureProfile(m_TSort);
			_pScope->m_pContainer->m_UsedContainers.MergeSort<CBucketContainerSort>();
			BucketIter = _pScope->m_pContainer->m_UsedContainers;
			if (BucketIter)
				BucketIter->m_VBs.MergeSort<CVBSort>();
		}
		else
		{
			BucketIter = _pScope->m_pContainer->m_UsedContainers;
		}

		CXR_VertexBuffer *pCurrentVB = NULL;
		if (BucketIter)
			pCurrentVB = BucketIter->m_VBs.GetFirst();
#else
		CXR_VBPtr* lpVB = _pScope->m_lpVB.GetBasePtr();
#endif
		const int Length = _pScope->GetNumVBs();
		int Count = Length - _pScope->m_nBufferSkip;

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

		bint bFirst = false;

		_StartPrio = Max(_StartPrio + 100.0f, 0.0f);
		if (_EndPrio < 1000000000.0f)
			_EndPrio += 100.0f;

		// -------------------------------------------------------------------
		// VB Loop
		// -------------------------------------------------------------------
		if (pPerfQueryMem)
			_pRC->PerfQuery_Begin(Count, pPerfQueryMem);

		int iVB = 0;
		for(int i = 0; i < Count; i++)
		{
			++m_nVBs;
#ifdef XR_VBMLINKEDLIST
			if (bFirst)
			{
				pCurrentVB = DLinkDA_List(CXR_VertexBuffer, m_Link)::GetNext(pCurrentVB);
				if (!pCurrentVB)
				{
					++BucketIter;
					if (!BucketIter)
					{
						ConOut("ERROR: Internal very serious error in VBManager");
						M_TRACEALWAYS("ERROR: Internal very serious error in VBManager\n");
						break;					
					}
					if (bNeedSort)
					{
						TMeasureProfile(m_TSort);
						BucketIter->m_VBs.MergeSort<CVBSort>();
					}
					pCurrentVB = BucketIter->m_VBs.GetFirst();
				}
			}
			bFirst = true;
			CXR_VertexBuffer* pVB = pCurrentVB;
#else
			CXR_VertexBuffer* pVB = lpVB[i].m_pVB;
#endif
			{
				int i = iVB++;
				if(i < _iVB)
					continue;
				if(i >= (_iVB + _nVB))
					break;
			}

			if ((uint32 &)pVB->m_Priority < (uint32 &)_StartPrio)
				continue;
			if ((uint32 &)pVB->m_Priority > (uint32 &)_EndPrio)
				break;

			VBM_PROFILEVB_START

			// Run pre-render on all vertex-buffers with the same priority. 
			// Since the order should not matter we can execute all of them at once, giving the nice side effect that flare depth-tests are performed in sequence.
/*			if (i == LastPreRender)
			{
				fp32 Prior = pVB->m_Priority;
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

//			_pRC->Geometry_Color(pVB->m_Color);
/*
			const CVpuShadowInfo& VpuShadowInfo=pVB->GetVpuShadowInfo();
			if (VpuShadowInfo.m_TaskId!=0xffffffff)
			{
				MRTC_ThreadPoolManager::VPU_BlockOnTask(VpuShadowInfo.m_TaskId);
				pVB->Render_IndexedTriangles(*VpuShadowInfo.m_ppTriVertIndicies,*VpuShadowInfo.m_pnTris);
				CVpuShadowInfo si;
				si.m_TaskId=0xffffffff;
				si.m_ppTriVertIndicies=0;
				si.m_pnTris=0;
				pVB->SetVpuShadowInfo(si);
			}
*/
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

			_pRC->Geometry_Color(pVB->m_Color);

			do
			{
				if (!pVB->m_pVBChain) // PreRender only buffer
				{
					pLastVB = NULL;
					pLastMat[0] = (CMat4Dfp32*)1;
					VBM_PROFILEVB_STOP;
					VBM_PROFILEVBGPU_NEXT;
					break;
				}
				// Check if the buffer has any primitives at all. (PreRender() on flares will set nPrim to zero if the flare is invisible.)
				if (!pVB->IsVBIDChain() )
				{
					if( !pVB->GetVBChain() || !pVB->GetVBChain()->m_nPrim )
					{
						pLastVB = NULL;
						pLastMat[0] = (CMat4Dfp32*)1;
						VBM_PROFILEVB_STOP;
						VBM_PROFILEVBGPU_NEXT;
						break;
					}
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

									if(!pA->m_Scissor.IsValid())
									{
										pA->m_Scissor.SetRect(0, 0);
										bNullScissor = 1;
									}
	/*
									if (pA->m_Scissor.m_Min[0] >= pA->m_Scissor.m_Max[0] ||
										pA->m_Scissor.m_Min[1] >= pA->m_Scissor.m_Max[1])
									{
										pA->m_Scissor.m_Min[0] = 0;
										pA->m_Scissor.m_Min[1] = 0;
										pA->m_Scissor.m_Max[0] = 0;
										pA->m_Scissor.m_Max[1] = 0;
										bNullScissor = 1;
									}
	*/
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
					VBM_PROFILEVB_STOP;
					VBM_PROFILEVBGPU_NEXT;

					break;
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
			while(0);

			if (pVB->m_pPreRender && pVB->m_pPreRender->m_pfnPostRender)
			{
#ifdef _DEBUG						 
				// Check validity of function
				//				*((mint *)(pVB->m_pPreRender->m_pfnPreRender)) = *((mint *)(pVB->m_pPreRender->m_pfnPreRender));
#ifdef PLATFORM_WIN_PC
				if (IsBadCodePtr((FARPROC)pVB->m_pPreRender->m_pfnPostRender))
					M_BREAKPOINT;
#endif
#endif
				if (!(_Flags & 2))
				{
					pVB->m_pPreRender->m_pfnPostRender(_pRC, this, pVB, pVB->m_pPreRender->m_pPreRenderContext, _pScope, _Flags);
				}
			}

			VBM_PROFILEVB_STOP;
			VBM_PROFILEVBGPU_NEXT;
		}

#ifdef M_Profile
		// -------------------------------------------------------------------
		// Move perf query data to vertex buffers
		if (_Flags & 16)
		{
			int nQueries = 0;
			if (pPerfQueryMem)
				nQueries = _pRC->PerfQuery_End();

			CMTime* pPerfQuery = (CMTime*)pPerfQueryMem;

			CXR_VBMScope::CBucketContainer::CContaireIter BucketIter;
			BucketIter = _pScope->m_pContainer->m_UsedContainers;
			if (BucketIter)
				pCurrentVB = BucketIter->m_VBs.GetFirst();

			for(int i = 0; i < Count; i++)
			{
				++m_nVBs;
#ifdef XR_VBMLINKEDLIST
				CXR_VertexBuffer* pVB = pCurrentVB;
#else
				CXR_VertexBuffer* pVB = lpVB[i].m_pVB;
#endif
				if (i >= nQueries || !pPerfQuery)
					pVB->m_GPUTime.Reset();
				else
					pVB->m_GPUTime = pPerfQuery[i];

#ifdef XR_VBMLINKEDLIST
				pCurrentVB = DLinkDA_List(CXR_VertexBuffer, m_Link)::GetNext(pCurrentVB);
				if (!pCurrentVB)
				{
					++BucketIter;
					if (BucketIter)
						pCurrentVB = BucketIter->m_VBs.GetFirst();
					if (!pCurrentVB)
						break;
				}
#endif
			}
		}
#endif
		// -------------------------------------------------------------------

		_pRC->Geometry_Clear();
		_pRC->Matrix_Pop();
		_pRC->Attrib_Pop();

	#ifdef PLATFORM_DOLPHIN
		m_nToken = pRenderDolphin->InsertToken();
		pRenderDolphin->DisableScratchpad(false);
	#endif
	}

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

void CXR_VBManager::ScopeBegin(const char* _pName, bool _bNeedSort, int _MaxVB)
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
		NewScope.Create(m_MaxVB, _pName);
	else
		NewScope.Create(_MaxVB, _pName);

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
#ifdef XR_VBMLINKEDLIST
	if(!SortScope.m_pContainer)
#else
	if(SortScope.m_lpVB.Len() == 0 )
#endif
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

int CXR_VBManager::Clip_Add(const CPlane3Dfp32* _pPlanes, int _nPlanes, const CMat4Dfp32* _pTransform)
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

bool CXR_VBManager::Clip_Push(const CPlane3Dfp32* _pPlanes, int _nPlanes, const CMat4Dfp32* _pTransform)
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

void CXR_VBManager::RenderBox(const CBox3Dfp32& _Box, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_VBManager_RenderBox, MAUTOTRIP_VOID);
	CXR_VertexBuffer* pVB = Alloc_VBAttrib();
	if(!pVB)
		return;
	if(!pVB->AllocVBChain(this, false))
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
		int iThis = i;
		int iNext = (i + 3) & 3;

		pChain->m_piPrim[i*2 + 0]	= iThis;
		pChain->m_piPrim[i*2 + 1]	= iNext;

		pChain->m_piPrim[8 + i*2 + 0]	= iThis + 4;
		pChain->m_piPrim[8 + i*2 + 1]	= iNext + 4;

		pChain->m_piPrim[16 + i*2 + 0]	= iThis;
		pChain->m_piPrim[16 + i*2 + 1]	= iThis + 4;
	}

	AddVB(pVB);
}

void CXR_VBManager::RenderBox(const CMat4Dfp32& _Transform, const CBox3Dfp32& _Box, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_VBManager_RenderBox2, MAUTOTRIP_VOID);
	CXR_VertexBuffer* pVB = Alloc_VBAttrib();
	if(!pVB)
		return;
	if(!pVB->AllocVBChain(this, false))
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

	CVec3Dfp32 Vertices[8];
	_Box.GetVertices(Vertices);

	for(int i = 0; i < 8; i++)
	{
		Vertices[i].MultiplyMatrix(_Transform, pChain->m_pV[i]);
	}

	int NextMapping[4] = {1, 3, 0, 2};

	for(int i = 0; i < 4; i++)
	{
		int iThis = i;
		int iNext = NextMapping[i];

		pChain->m_piPrim[i*2 + 0]	= iThis;
		pChain->m_piPrim[i*2 + 1]	= iNext;

		pChain->m_piPrim[8 + i*2 + 0]	= iThis + 4;
		pChain->m_piPrim[8 + i*2 + 1]	= iNext + 4;

		pChain->m_piPrim[16 + i*2 + 0]	= iThis;
		pChain->m_piPrim[16 + i*2 + 1]	= iThis + 4;
	}

	AddVB(pVB);
}

void CXR_VBManager::RenderWire(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CPixel32 _Color)
{
	CXR_VertexBuffer* pVB = Alloc_VBAttrib();
	if(!pVB)
		return;
	if(!pVB->AllocVBChain(this, false))
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

void CXR_VBManager::RenderWire(const CMat4Dfp32& _Transform, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_VBManager_RenderWire, MAUTOTRIP_VOID);
	CVec3Dfp32 p0 = _p0 * _Transform;
	CVec3Dfp32 p1 = _p1 * _Transform;
	RenderWire(p0, p1, _Color);
}

void CXR_VBManager::RenderWires(const CMat4Dfp32& _Transform, const CVec3Dfp32* _pV, int _nVertices, CPixel32 _Color, bool _bLoop)
{
	CXR_VertexBuffer* pVB = Alloc_VBAttrib();
	if(!pVB)
		return;
	if(!pVB->AllocVBChain(this, false))
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
	memcpy(pChain->m_pV, _pV, _nVertices * sizeof(CVec3Dfp32));

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

void CXR_VBManager::RenderWires(const CMat4Dfp32& _Transform, const CVec3Dfp32* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color, bool _bLoop)
{
	CXR_VertexBuffer* pVB = Alloc_VBAttrib();
	if(!pVB)
		return;
	if(!pVB->AllocVBChain(this, false))
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

void CXR_VBManager::RenderWires(const CMat4Dfp32& _Transform, const CVec3Dfp32* _pV, const uint32* _piV, int _nVertices, CPixel32 _Color, bool _bLoop)
{
	CXR_VertexBuffer* pVB = Alloc_VBAttrib();
	if(!pVB)
		return;
	if(!pVB->AllocVBChain(this, false))
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

void CXR_VBManager::RenderWires(const CMat4Dfp32& _Transform, const CVec3Dfp32* _pV, const uint32* _piV, int _nVertices, const CPixel32* _pColors, bool _bLoop)
{
	CXR_VertexBuffer* pVB = Alloc_VBAttrib();
	if(!pVB)
		return;
	if(!pVB->AllocVBChain(this, false))
		return;
	pVB->m_pTransform	= Alloc_M4(_Transform);
	if(!pVB->m_pTransform)
		return;
	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pVB->m_Priority		= 10000000.0f;
	CXR_VBChain* pChain = pVB->GetVBChain();
	pChain->m_pV		= Alloc_V3(_nVertices);
	pChain->m_pCol		= Alloc_CPixel32(_nVertices);
	if (!pChain->m_pV || !pChain->m_pCol)
		return;
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
		pChain->m_pCol[i] = _pColors[_piV[i]];
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



bool CXR_VBManager::InsertTexGen(CRC_Attributes* _pA, int _nTexGen, const int* _pTexGenModes, const int* _pTexGenComp, const int* _piTexGenChannel, fp32** _pTexGenAttr)
{
	if (_pA->m_pTexGenAttr)
	{
		fp32* lpTexGenAttr[CRC_MAXTEXCOORDS];
		int lTexGenSize[CRC_MAXTEXCOORDS];
		int lNewTexGenAttrOffset[CRC_MAXTEXCOORDS];	// Offset in fp32s

		fp32* pCurrent = _pA->m_pTexGenAttr;

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

		fp32* pTexGenAttr = Alloc_fp32(pCurrent - (fp32*)NULL);
		if(!pTexGenAttr)
			return false;

		fp32* pTexGenAttrOld = _pA->m_pTexGenAttr;
		{
			for(int i = 0; i < CRC_MAXTEXCOORDS; i++)
			{
				if (lTexGenSize[i])
				{
					memcpy(pTexGenAttr + lNewTexGenAttrOffset[i], lpTexGenAttr[i], lTexGenSize[i]*sizeof(fp32));
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
		fp32* pTexGenAttr = Alloc_fp32(TexGenAttrSize);
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

void CXR_ClipStackEntry::Create(const CPlane3Dfp32* _pPlanes, int _nPlanes, const CMat4Dfp32* _pTransform)
{
	M_ASSERT(_nPlanes <= CXR_CLIPMAXPLANES, "!");
	m_nPlanes = _nPlanes;
	if (_pTransform)
	{
		for(int i = 0; i < _nPlanes; i++)
		{
			CPlane3Dfp32 P = _pPlanes[i];
			P.Transform(*_pTransform);
			m_lPlanes[i] = P;
		}
	}
	else
		memcpy(&m_lPlanes, _pPlanes, sizeof(CPlane3Dfp32) * _nPlanes);
}

void CXR_ClipStackEntry::Copy(const CXR_ClipStackEntry& _Src, const CMat4Dfp32* _pTransform)
{
	MAUTOSTRIP(CXR_ClipStackEntry_Copy, MAUTOSTRIP_VOID);
	Create(_Src.m_lPlanes+0, _Src.m_nPlanes, _pTransform);
}

