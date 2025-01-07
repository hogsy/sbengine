#ifndef _INC_XRVBManager
#define _INC_XRVBManager

#include "XRVertexBuffer.h"


// -------------------------------------------------------------------
//  CXR_VBManager
// -------------------------------------------------------------------
#define XR_VBMLINKEDLIST

#ifndef XR_VBMLINKEDLIST
class CXR_VBPtr
{
public:
	CXR_VertexBuffer* m_pVB;
	fp32 m_Priority;
	DLinkDS_Link(CXR_VBPtr, m_Link);

	CXR_VBPtr()
	{
		m_pVB = NULL;
		m_Priority = 0.0f;
	}

	CXR_VBPtr(CXR_VertexBuffer* _pVB)
	{
		m_pVB = _pVB;
		m_Priority = _pVB->m_Priority;
	}

	int Compare(const CXR_VBPtr& _pVB) const;
};
#endif

enum
{
	VBM_SHOWTIME = 1,

	VBM_HWALLOCALIGN = 32,
};

enum
{
	VBMSCOPE_FL_NEEDSORT	= M_Bit(0),
	VBMSCOPE_FL_RENDERWIRE	= M_Bit(1),

	VBMCONTEXT_ENGINE	= 0,
	VBMCONTEXT_RENDER	= 1,

	CXR_VBMCONTEXT_COUNT	= 2,
};

enum
{
	CXR_VBMANAGER_STATE_PREBEGIN	= 0,
	CXR_VBMANAGER_STATE_RENDERING	= 1,
	CXR_VBMANAGER_STATE_POSTBEGIN	= 2,
};


enum
{
	CXR_CLIPMAXPLANES			= 6,
	CXR_CLIP_STACK_MAX			= 8,
	CXR_CLIP_HEAP_MAX			= 32,		// NOTE: uint8 CXR_VertexBuffer::m_iClip

	CXR_VIEWPORT_STACK_MAX		= 8,
	CXR_VIEWPORT_HEAP_MAX		= 256,		// NOTE: uint8 CXR_VertexBuffer::m_iVP

	CXR_SORTSCOPES_MAX			= 32,
};

class CXR_ClipStackEntry
{
public:
	CPlane3Dfp32 m_lPlanes[CXR_CLIPMAXPLANES];
	int m_nPlanes;

	CXR_ClipStackEntry();
	void Create(const CPlane3Dfp32* _pPlanes, int _nPlanes, const CMat4Dfp32* _pTransform = NULL);
	void Copy(const CXR_ClipStackEntry& _Src, const CMat4Dfp32* _pTransform = NULL);
};

class CXR_VBMScope
{
public:
#ifndef XR_VBMLINKEDLIST
	class CVBM_SortableVector
	{
	protected:
		CXR_VBPtr*	m_lpVBP;
		uint32 m_nMax;
		uint32 m_iCurrent;

	public:
		CVBM_SortableVector() : m_lpVBP(NULL),m_nMax(0),m_iCurrent(0) {}

		void Create(uint32 _MaxVB);

		void Add(CXR_VBManager* _pVBM, CXR_VBPtr _pVBP);
		void Sort();
		CXR_VBPtr* GetBasePtr()
		{
			return m_lpVBP;
		}

		CXR_VBPtr& operator [] (int _iVB);

		int Len() const;
	};
#endif

#ifdef XR_VBMLINKEDLIST
	class M_ALIGN(128) CBucketContainer
	{
	public:
		enum 
		{
			ENumBuckets0 = 512,
			ENumBuckets1 = 512,
			ENumBuckets = ENumBuckets0 + ENumBuckets1

		};

		class CContainer
		{
		public:
			DLinkDA_List(CXR_VertexBuffer, m_Link) m_VBs;
			DLinkDA_Link(CContainer, m_Link);
		};

		DLinkDA_List(CContainer, m_Link) m_UsedContainers;
		typedef DLinkD_Iter(CContainer, m_Link) CContaireIter;
		CContainer m_Containers[ENumBuckets0 + ENumBuckets1];
		//		 m_VBs;
	};

	CBucketContainer *m_pContainer;

	typedef DLinkD_Iter(CXR_VertexBuffer, m_Link) CVBIterator;
	int m_nVBs;
#else
	CVBM_SortableVector	m_lpVB;
#endif
	int GetNumVBs()
	{
	#ifdef XR_VBMLINKEDLIST
		return m_nVBs;
	#else
		return m_lpVB.Len();
	#endif
	}


	const char* m_pName;
	int	m_Flags;
	class CXR_LightOcclusionInfo* m_pLO;
	int m_nMaxLights;
	int	m_nBufferSkip;


	CXR_VBMScope() 
	{
#ifdef XR_VBMLINKEDLIST
		m_pContainer = NULL;
#endif
	}

	CXR_VBMScope(bool _bSort) : m_pLO(0), m_nMaxLights(0), m_nBufferSkip(0)
	{
		m_Flags	= _bSort?VBMSCOPE_FL_NEEDSORT:0;
#ifdef XR_VBMLINKEDLIST
		m_nVBs = 0;
		m_pContainer = NULL;
//		m_VBs.Construct();
#endif
		m_pName = NULL;
	}

	void Create(uint32 _MaxVB, const char* _pName);

#ifdef XR_VBMLINKEDLIST
	CXR_VBMScope& operator = (CXR_VBMScope &_Src)
	{
		m_pContainer = _Src.m_pContainer;
#if 0
		if (!_Src.m_VBs.IsEmpty())
		{
			m_VBs.Transfer(_Src.m_VBs);
		}
		else
		{
			m_VBs.Construct();
		}
#endif
		m_nVBs = _Src.m_nVBs;
		m_Flags = _Src.m_Flags;
		m_pLO = _Src.m_pLO;
		m_nMaxLights = _Src.m_nMaxLights;
		m_nBufferSkip = _Src.m_nBufferSkip;
		m_pName = _Src.m_pName;
		return *this;
	}
#endif
};

class CXR_VBCallback
{
public:
	static void RenderCallback(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
	{
		((CXR_VBCallback *)_pContext)->Callback(_pRC, _pVBM, _pVB, _pScope, _Flags);
	}

	virtual void Callback(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, CXR_VBMScope* _pScope, int _Flags) pure;
};


class CXR_VBManager : public CReferenceCount
{
	MRTC_DECLARE;


/*
	class CXR_VBMContext
	{
	public:
		TArray<uint8>	m_lHeap;

		int m_AllocPos;	// This is updated in Internal_End

		void Create(int _HeapSize)
		{
			m_lHeap.SetLen(_HeapSize);
			m_State	= CXR_VBMANAGER_STATE_PREBEGIN;
			m_AvailableEvent.Signal();
		}

		void SortScopes()
		{
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
	};
*/
public:
	class CVBAddCollector
	{
	public:
		CVBAddCollector(CXR_VertexBuffer** _pVBs, mint _MaxLen)
		{
			m_iIndex = 0;
			m_pVBs = _pVBs;
			m_MaxLen = _MaxLen;
		}

		mint m_iIndex;
		mint m_MaxLen;
		CXR_VertexBuffer**m_pVBs;
	};
//#define VBM_OVERWRITE_DEBUGGING 1

#if VBM_OVERWRITE_DEBUGGING
	const static int ms_MatchBufferSize = 16;
	uint8 m_MatchBuffer[ms_MatchBufferSize];
#endif

protected:
//	TArray<uint8> m_lHeap;	// This points at a context's array
	uint8 *m_pHeap;
	mint m_HeapSize;

	CXR_VBMScope* m_pActiveScope;
	TStaticArray<int, 8> m_lScopeStack;
	TStaticArray<CXR_VBMScope, 32> m_lSortScopes;


	TMRTC_ThreadLocal<CVBAddCollector *> m_CurrentCollector;

	int	m_iScopeStack;
	int	m_MaxVB;

	int	m_State;
	int m_AllocPos;
#ifdef M_Profile
	int m_AllocPosMax;
#endif
#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	int m_RecordUsage;
#endif

	class CXR_VBContext* m_pVBCtx;

	// Memory potentially provided by render-context. (i.e, by VertexArrayRange)
	uint8* m_pHeapHW;
	int m_HeapSizeHW;
	int m_AllocPosHW;

	TArray<uint8> m_lVertexUseMap;

	bool m_bOutOfMemory;

	int m_nBuffers;
	int m_nVBs;
	CMTime m_TSort;
	CMTime m_TRender;

	CRenderContext* m_pCurrentRC;		// Only valid between Begin()/End()



#ifdef PLATFORM_DOLPHIN
	int m_nToken;
#endif

#ifdef M_Profile
	CXR_VertexBuffer* m_pLastSelectedVB;

	int m_Stats_nVB;
	int m_Stats_nAttrib;
	int m_Stats_nLights;
	int m_Stats_nXRLights;
	int m_Stats_nM4;
	int m_Stats_nV4;
	int m_Stats_nV3;
	int m_Stats_nV2;
	int m_Stats_nInt16;
	int m_Stats_nInt32;
	int m_Stats_nFp32;
	int m_Stats_nPixel32;
	int m_Stats_nOpen;
	int m_Stats_nChains;
	int m_Stats_nChainVBs;
	int m_Stats_nMP;
public:
	int m_Stats_nRenderSurface;
#endif

public:

	// -------------------------------
	// Viewport stack
	CRC_Viewport*	m_lpVPHeap[CXR_VIEWPORT_HEAP_MAX];
	int m_nVPHeap;

	uint8 m_liVPStack[CXR_VIEWPORT_STACK_MAX];
	int m_iVPStack;

	// -------------------------------
	// Clipping stack
	CXR_ClipStackEntry*	m_lpClipHeap[CXR_CLIP_HEAP_MAX];
	int m_nClipHeap;

	uint8 m_liClipStack[CXR_CLIP_STACK_MAX];
	int m_iClipStack;

	// -------------------------------
	void* m_pOwner;

	// Internal functions, these assume locking has been done
	void Internal_Begin(CRenderContext* _pRC, const CRC_Viewport* _pVP);
	void Internal_End();
	void Internal_Render(CXR_VBMScope* _pScope, CRenderContext* _pRC, int _Flags, fp32 _StartPrio, fp32 _EndPrio, int _iVB = 0, int _nVB = 0);
	void Internal_SortScope(CXR_VBMScope* _pScope);
	void Internal_Sort();

	void Internal_Viewport_Set(CRC_Viewport* _pVP, fp32 _Priority);

	int VBE_RenderScope(CXR_VBMScope* _pScope, CPnt _Pos, class CXR_VBEContext& _EC, uint _bSelected);
	int VBE_RenderVBInfo(class CXR_VBEContext& _EC, CPnt _Pos, const CXR_VertexBuffer* _pVB, const CRC_Attributes* _pA);
	void VBE_Render(CRenderContext* _pRC, int _Flags, class CXR_VBEContext& _EC);
	dllvirtual void VBE_GetScopeInfo(class CXR_VBEContext& _EC);
	dllvirtual bool VBE_ProcessKey(class CXR_VBEContext& _EC, CScanKey _Key) const;

public:
	MRTC_SpinLock m_AddLock;			// Must be re-entrant due to a fucking nasty hack in trimesh
//	MRTC_SpinLock m_AllocLock;
//	NThread::CSpinLock m_AddLock;		// Not re-entrant, will deadlock if used incorrectly
	NThread::CSpinLock m_AllocLock;		// Not re-entrant, will deadlock if used incorrectly

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	void RecordFrame()
	{
		gf_RDSendHeapClear(this);
		m_RecordUsage = true;
	}
#endif

	CXR_VBManager();
	~CXR_VBManager();
	dllvirtual void Create(int _HeapSize, int _MaxVB);

	dllvirtual uint8* GetVertexUseMap(int _nV);
	void GetAllocParams(int*& _pAllocPos,NThread::CSpinLock*& _pLock,uint8**& _pHeap,mint*& _pHeapSize);
	void *GetHeapPtr()
	{
		return m_pHeap;
	}
	dllvirtual int GetHeap();
	dllvirtual int GetAvail();
	dllvirtual int GetAllocPos();
#ifdef M_Profile
	dllvirtual int GetMinAvail();
#endif

	dllvirtual void* Alloc_Open();
	dllvirtual void Alloc_Close(int _Size);
	dllvirtual bool OutOfMemory();

	// HW heap alloc was a test and is not in use.
#ifdef NEVER
	dllvirtual int GetHeapHW();
	dllvirtual int GetAvailHW();
	dllvirtual void* Alloc_OpenHW();
	dllvirtual void Alloc_CloseHW(int _Size);
#endif

	// Allocation
	dllvirtual void* Alloc(int _Size, uint _bCacheZeroAll = false);
	dllvirtual void* AllocHW(int _Size);
	dllvirtual void* AllocArray(int _Size);										// Tries to allocate in HW heap if possible.
	dllvirtual CXR_VertexBuffer* Alloc_VB();
	dllvirtual CXR_VertexBuffer* Alloc_VBAttrib();
	dllvirtual CXR_VertexBuffer* Alloc_VB(int _Contents, int _nV = 0);
	dllvirtual CXR_VBChain* Alloc_VBChain(int _Contents, int _nV = 0);
	dllvirtual CXR_VBIDChain* Alloc_VBIDChain();
	dllvirtual CXR_VBChain* Alloc_VBChain();
	dllvirtual CXR_VertexBuffer_PreRender* Alloc_PreRender(PFN_VERTEXBUFFER_PRERENDER _pFunc, void *_pContext);
	dllvirtual const CMat4Dfp32** Alloc_TextureMatrixArray();
	dllvirtual CRC_MatrixPalette* Alloc_MP();

	dllvirtual CMat4Dfp32* Alloc_M4_Proj2D(fp32 _Z);
	dllvirtual CMat4Dfp32* Alloc_M4_Proj2DRelBackPlane(fp32 _BackPlaneOfs = -16.0f);
	dllvirtual CMat4Dfp32* Alloc_M4_Proj2D(CRC_Viewport* _pVP, fp32 _Z);
	dllvirtual CMat4Dfp32* Alloc_M4_Proj2DRelBackPlane(CRC_Viewport* _pVP, fp32 _BackPlaneOfs = -16.0f);

	dllvirtual bool Alloc_VBChainCopy(CXR_VertexBuffer* _pDst, CXR_VertexBufferGeometry* _pSrc);					// Only allocates chain. no attrib. :No geometry can be alloced with Alloc_VBChain
//	dllvirtual CXR_VertexBuffer* Alloc_VBChain(int _Contents, int _Len = 1);		// No geometry can be alloced with Alloc_VBChain
//	dllvirtual CXR_VertexBuffer* Alloc_VBChainOnly(CXR_VertexBuffer* _pSrc);					// Only allocates chain. no attrib. :No geometry can be alloced with Alloc_VBChain
//	dllvirtual CXR_VertexBuffer* Alloc_VBChainAttrib(int _Len = 1);					// Allocates chain and attrib :No geometry can be alloced with Alloc_VBChain
	dllvirtual CRC_Attributes* Alloc_Attrib();
	dllvirtual CRC_Attributes* Alloc_Attrib(int _nCount);
	dllvirtual class CXR_Light* Alloc_XRLights(int _nCount);
	dllvirtual CRC_Light* Alloc_Lights(int _nLights);
	dllvirtual CMat4Dfp32* Alloc_M4();
	dllvirtual CMat4Dfp32* Alloc_M4(const CMat4Dfp32& _Mat);
	dllvirtual CMat43fp32* Alloc_M43();
	dllvirtual CMat43fp32* Alloc_M43(const CMat43fp32& _Mat);
	dllvirtual CMat4Dfp32* Alloc_M4(const CMat43fp32& _Mat);
	dllvirtual CMat43fp32* Alloc_M43(const CMat4Dfp32& _Mat);

	dllvirtual CVec4Dfp32* Alloc_V4(int _nV);
	dllvirtual CVec3Dfp32* Alloc_V3(int _nV);
	dllvirtual CVec2Dfp32* Alloc_V2(int _nV);
	dllvirtual uint16* Alloc_Int16(int _nV);
	dllvirtual uint32* Alloc_Int32(int _nV);
	dllvirtual fp32* Alloc_fp32(int _nV);
	dllvirtual CPixel32* Alloc_CPixel32(int _nV);
	dllvirtual char* Alloc_Str(const char* _pStr);

	// !!! _pVB and _pAttrib must be a pointer to m_lHeap unless they're guaranteed to be valid over several frames !!!

#ifdef M_VBDEBUG
	dllvirtual void AddVBImp(CXR_VertexBuffer* _pVB, const char *_pFile, int _Line);
	dllvirtual void AddVBImp(const CXR_VertexBuffer& _VB, const char *_pFile, int _Line);
#else
	dllvirtual void AddVB(CXR_VertexBuffer* _pVB);
	dllvirtual void AddVB(const CXR_VertexBuffer& _VB);
#endif
	dllvirtual void AddVBArray(CXR_VertexBuffer** _lpVB, uint _nVB);
	dllvirtual bool AddCallback(PFN_VERTEXBUFFER_PRERENDER _pfnCallback, void* _pContext, fp32 _Priority, uint32 _VBEColor = 0xff404000);

	template<typename t_CCallback>
	t_CCallback* AddCallbackClass(fp32 _Priority)
	{
		t_CCallback* pData = (t_CCallback*)Alloc(sizeof(t_CCallback));
		if (!pData)
			return NULL;

		new ((void *)pData) t_CCallback;

		if (!AddCallback(t_CCallback::RenderCallback, pData, _Priority))
			return NULL;

		return pData;

	}

	dllvirtual bool AddRenderOptions(fp32 _Priority, uint32 _Options, uint32 _Format = 0xffffffff);
	dllvirtual bool AddRenderRange(fp32 _Priority, fp32 _StartRenderPrio, fp32 _EndRenderPrio);
	dllvirtual bool AddClearRenderTarget(fp32 _Priority, int _WhatToClear, CPixel32 _ColorClearTo, fp32 _ZClearTo, int _StencilClearTo, CRct _ClearArea = CRct(-1,-1,-1,-1));
	dllvirtual bool AddCopyToTexture(fp32 _Priority, CRct _SrcRect, CPnt _Dst, uint16 _TextureID, bint _bContinueTiling, uint16 _Slice = 0, uint32 _iMRT = 0);

	dllvirtual bool AddRenderTargetEnableMasks(fp32 _Priority, uint32 _And);

	dllvirtual bool AddNextClearParams(fp32 _Priority, int _WhatToClear, CPixel32 _ColorClearTo, fp32 _ZClearTo, int _StencilClearTo, CRct _ClearArea = CRct(-1,-1,-1,-1));

	dllvirtual bool AddSetRenderTarget(fp32 _Priority, const CRC_RenderTargetDesc &_Desc);

#if 0
	// These won't work properly with scopes

	mint GetCurrentVB();
	mint GetNextVB(mint _pVB);
	CXR_VertexBuffer* GetVB(mint _hVB);
#endif


	dllvirtual void SetVBAddCollector(CVBAddCollector *_pCollector);

protected:

	void Sort();

public:

	dllvirtual void Begin(CRenderContext* _pRC, const CRC_Viewport* _pVP);// , int _nBuckets = 0, fp32 _BucketPriorityStart = 0.0f, fp32 _BucketPriorityEnd = 0.0f
	dllvirtual void End();
//	dllvirtual void Render(CRenderContext* _pRC, int _Flags, int _nBufferSkip = 0, class CXR_ViewClipInterface* _pViewClip = NULL);
	dllvirtual void Render(CRenderContext* _pRC, int _Flags, class CXR_VBEContext* _pEC = NULL);
	dllvirtual bool IsRendering();

	dllvirtual void Flush(CRenderContext* _pRC, int _Flags,  int _nBufferSkip = 0, class CXR_ViewClipInterface* _pViewClip = NULL);

	dllvirtual void ScopeBegin(const char* _pName, bool _bNeedSort, int _MaxVB = 0);	// 0 means use default value
	dllvirtual void ScopeEnd();
	dllvirtual void ScopeSetLightOcclusionMask(class CXR_LightOcclusionInfo* _pLO, int _nMaxLights);
	dllvirtual void ScopeSetBufferskip(int _nBufferskip);
	dllvirtual void ScopeSetFlags(int _Flags);
	dllvirtual void ScopeClearFlags(int _Flags);

	dllvirtual CStr GetInfoString();

	dllvirtual void SetOwner(void* _pOwner);

	// Viewport
	dllvirtual CRC_Viewport* Viewport_Get();
	dllvirtual CRC_Viewport* Viewport_Get(int _Index);
	dllvirtual int Viewport_Add(const CRC_Viewport* _pVP);					// 0 == Out of VB heap
	dllvirtual bool Viewport_Push(const CRC_Viewport* _pVP);				// false == Out of VB heap, don't Viewport_Pop()
	dllvirtual void Viewport_Pop();
	dllvirtual int Viewport_GetCurrentIndex();

	// Clipping
	dllvirtual CXR_ClipStackEntry* Clip_Get(int _iClip);
	dllvirtual int Clip_Add(const CPlane3Dfp32* _pPlanes, int _nPlanes, const CMat4Dfp32* _pTransform = NULL);		// 0 == Out of VB heap
	dllvirtual bool Clip_Push(const CPlane3Dfp32* _pPlanes, int _nPlanes, const CMat4Dfp32* _pTransform = NULL);		// false == Out of VB heap, don't Clip_Pop()
	dllvirtual void Clip_Pop();

	// Debug
	dllvirtual void IncBufferSkip(int _nStep);
	dllvirtual void DecBufferSkip(int _nStep);

#ifdef PLATFORM_PS2
	void PrefetchTexture( CRenderContext* _pRC, int i, int _nBufferSkip );
#endif

	dllvirtual bint ConvertVertexBuildBuffer(CRC_VertexBuffer &_Dest, const CRC_BuildVertexBuffer&_Source);

	dllvirtual void RenderBox(const CMat4Dfp32& _Transform, const CBox3Dfp32& _Box, CPixel32 _Color);
	dllvirtual void RenderWire(const CMat4Dfp32& _Transform, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CPixel32 _Color);
	dllvirtual void RenderBox(const CBox3Dfp32& _Box, CPixel32 _Color);
	dllvirtual void RenderWire(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, CPixel32 _Color);
	dllvirtual void RenderWires(const CMat4Dfp32& _Transform, const CVec3Dfp32* _pV, int _nVertices, CPixel32 _Color, bool bLoop);
	dllvirtual void RenderWires(const CMat4Dfp32& _Transform, const CVec3Dfp32* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color, bool bLoop);
	dllvirtual void RenderWires(const CMat4Dfp32& _Transform, const CVec3Dfp32* _pV, const uint32* _piV, int _nVertices, CPixel32 _Color, bool bLoop);
	dllvirtual void RenderWires(const CMat4Dfp32& _Transform, const CVec3Dfp32* _pV, const uint32* _piV, int _nVertices, const CPixel32* _pColors, bool _bLoop);

	dllvirtual bool InsertTexGen(CRC_Attributes* _pA, int _nTexGen, const int* _pTexGenModes, const int* _pTexGenComp, const int* _piTexGenChannel, fp32** _pTexGenAttr);

	class CXR_VBManager_RenderRange
	{
	public:
		fp32 m_StartPrio;
		fp32 m_EndPrio;

		static void RenderCallback(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
		{
			MSCOPESHORT(InitiateTexturePrecache);

			CXR_VBManager_RenderRange* pData = (CXR_VBManager_RenderRange*)_pContext;

			_pRC->Viewport_Push();
			_pVBM->Internal_Render(_pScope, _pRC, _Flags, pData->m_StartPrio, pData->m_EndPrio);
			_pRC->Viewport_Pop();
		}
	};
};

#ifdef M_VBDEBUG
#define AddVB(_x) AddVBImp(_x, __FILE__, __LINE__)
#endif

typedef TPtr<CXR_VBManager> spCXR_VBManager;

// -------------------------------------------------------------------
//  CXR_VBMContainer
// -------------------------------------------------------------------

#if 1
class CXR_VBMContainer : public CReferenceCount
{
	MRTC_DECLARE;
protected:
	class CVBM
	{
	public:
		DLinkD_Link(CVBM, m_Link);
		CXR_VBManager *m_pVBM;
		spCXR_VBManager m_spVBM;
	};

	DLinkD_List(CVBM, m_Link) m_Available;
	DLinkD_List(CVBM, m_Link) m_Dirty;
	DLinkD_List(CVBM, m_Link) m_Free;
	int m_nAvailable;
	int m_bBlockUntilFree;

	TThinArray<CVBM> m_lVBM;
	MRTC_CriticalSection m_Lock;

	NThread::CEventAutoReset m_AvailableEvent;
	NThread::CEventAutoReset m_DirtyEvent;

public:
	CXR_VBMContainer() {}

	void Create(int _nCount, int _nHeapSize, int _MaxVB)
	{
		M_LOCK(m_Lock);
		m_lVBM.SetLen(_nCount);
		m_nAvailable = _nCount;
		m_bBlockUntilFree = false;
		for(int i = 0; i < _nCount; i++)
		{
			MRTC_SAFECREATEOBJECT(spVBM, "CXR_VBManager", CXR_VBManager);
			m_lVBM[i].m_spVBM = spVBM;
			m_lVBM[i].m_pVBM = spVBM;
			spVBM->Create(_nHeapSize, _MaxVB);
			m_Available.Insert(m_lVBM[i]);
		}

		//m_AvailableEvent.SetSignaled();
		//m_DirtyEvent.ResetSignaled();
		//m_AllDoneEvent.SetSignaled();
	}

	CXR_VBManager* GetAvailVBM(fp64 _Timeout = 0.0)
	{
		M_LOCK(m_Lock);
		while (Volatile(m_bBlockUntilFree))
		{
			M_UNLOCK(m_Lock);
			MRTC_SystemInfo::OS_Sleep(10);
		}
		m_AvailableEvent.TryWait();
		CVBM* pVBM = m_Available.Pop();
		if(!pVBM)
		{
			if(_Timeout == 0.0) 
				return NULL;
			{
				M_UNLOCK(m_Lock);
				m_AvailableEvent.WaitTimeout(_Timeout);
			}

			pVBM = m_Available.Pop();
			if (!pVBM)
				return NULL;
		}

		--m_nAvailable;
		m_Free.Insert(pVBM);
		return pVBM->m_pVBM;
	}

	void BlockUntilAllVBMFree()
	{
		Volatile(m_bBlockUntilFree) = 1;
		while (1)
		{
			{
				M_LOCK(m_Lock);
				if (m_nAvailable == m_lVBM.Len())
					break;
			}
			MRTC_SystemInfo::OS_Sleep(10);
		}
		Volatile(m_bBlockUntilFree) = 0;
	}


	bool WaitForVBMAdd(fp64 _Timeout)
	{
		M_LOCK(m_Lock);
		while (Volatile(m_bBlockUntilFree))
		{
			M_UNLOCK(m_Lock);
			MRTC_SystemInfo::OS_Sleep(10);
		}
		m_AvailableEvent.TryWait();
		{
			M_UNLOCK(m_Lock);
			return !m_AvailableEvent.WaitTimeout(_Timeout);
		}
	}

	
	int GetNumAvailVBM() const
	{
		return m_nAvailable;
	}


	bool BlockUntilSingleVBMFree(fp64 _Timeout = 0.0)
	{
		M_LOCK(m_Lock);
		while (Volatile(m_bBlockUntilFree))
		{
			M_UNLOCK(m_Lock);
			MRTC_SystemInfo::OS_Sleep(10);
		}
		m_AvailableEvent.TryWait();
		if( !m_Available.IsEmpty() ) return true;
		
		if(_Timeout == 0.0) return false;

		{
			M_UNLOCK(m_Lock);
			m_AvailableEvent.WaitTimeout(_Timeout);
		}

		return !m_Available.IsEmpty();
	}

	CXR_VBManager* GetDirtyVBM(fp64 _Timeout = 0.0)
	{
		M_LOCK(m_Lock);
		m_DirtyEvent.TryWait();
		CVBM* pVBM = m_Dirty.Pop();
		if(!pVBM)
		{
			if(_Timeout == 0.0) 
				return NULL;
			{
				M_UNLOCK(m_Lock);
				m_DirtyEvent.WaitTimeout(_Timeout);
			}

			pVBM = m_Dirty.Pop();
			if (!pVBM)
				return NULL;
		}
		m_Free.Insert(pVBM);
		return pVBM->m_pVBM;
	}

	void Clean()
	{
		M_LOCK(m_Lock);
		CXR_VBManager* pVBM = 0;
		while((pVBM = GetDirtyVBM()))
		{
			pVBM->SetOwner(MRTC_SystemInfo::OS_GetThreadID());
			pVBM->End();
			AddAvailVBM(pVBM);
		}
	}

	void AddDirtyVBM(CXR_VBManager* _pVBM)
	{
		M_LOCK(m_Lock);
		CVBM *pFree = m_Free.Pop();
		M_ASSERT(pFree, "");
		pFree->m_pVBM = _pVBM;
		m_Dirty.Insert(pFree);
		m_DirtyEvent.Signal();
	}

	void AddAvailVBM(CXR_VBManager* _pVBM)
	{
		M_LOCK(m_Lock);
		CVBM *pFree = m_Free.Pop();
		M_ASSERT(pFree, "");
		pFree->m_pVBM = _pVBM;
		m_Available.Insert(pFree);
		++m_nAvailable;
		m_AvailableEvent.Signal();
	}
};

typedef TPtr<CXR_VBMContainer> spCXR_VBMContainer;
#else
class CXR_VBMContainer : public CReferenceCount
{
	MRTC_DECLARE;
protected:
	TArray<spCXR_VBManager> m_lspVBM;
	TArray<CXR_VBManager*> m_lpDirtyVBM;
	TArray<CXR_VBManager*> m_lpAvailVBM;
	MRTC_CriticalSection m_Lock;

	NThread::CEvent m_AllDoneEvent;
	NThread::CEvent m_AvailableEvent;
	NThread::CEvent m_DirtyEvent;
public:
	CXR_VBMContainer() {}

	void Create(int _nCount, int _nHeapSize, int _MaxVB)
	{
		M_LOCK(m_Lock);
		m_lspVBM.SetLen(_nCount);
		for(int i = 0; i < _nCount; i++)
		{
			MRTC_SAFECREATEOBJECT(spVBM, "CXR_VBManager", CXR_VBManager);
			m_lspVBM[i] = spVBM;
			m_lspVBM[i]->Create(_nHeapSize, _MaxVB);
			m_lpAvailVBM.Add(m_lspVBM[i]);
		}

		m_AvailableEvent.SetSignaled();
		m_DirtyEvent.ResetSignaled();
		m_AllDoneEvent.SetSignaled();
	}

	CXR_VBManager* GetAvailVBM(fp64 _Timeout = 0.0)
	{
		M_LOCK(m_Lock);
		if(!m_AvailableEvent.TryWait())
		{
			if(_Timeout == 0.0) return NULL;

			M_UNLOCK(m_Lock);
			if(!m_AvailableEvent.WaitTimeout(_Timeout))
				return NULL;
		}

		m_AllDoneEvent.ResetSignaled();

		CXR_VBManager* pVBM = m_lpAvailVBM[0];
		m_lpAvailVBM.Del(0);
		if(m_lpAvailVBM.Len() == 0)
			m_AvailableEvent.ResetSignaled();

		return pVBM;
	}

	void BlockUntilAllVBMFree()
	{
		m_AllDoneEvent.Wait();
	}

	CXR_VBManager* GetDirtyVBM(fp64 _Timeout = 0.0)
	{
		M_LOCK(m_Lock);
		if(!m_DirtyEvent.TryWait())
		{
			if(_Timeout == 0.0) return NULL;

			M_UNLOCK(m_Lock);
			if(!m_DirtyEvent.WaitTimeout(_Timeout))
				return NULL;
		}

		CXR_VBManager* pVBM = m_lpDirtyVBM[0];
		m_lpDirtyVBM.Del(0);
		if(m_lpDirtyVBM.Len() == 0)
			m_DirtyEvent.ResetSignaled();

		return pVBM;
	}

	void Clean()
	{
		M_LOCK(m_Lock);
		CXR_VBManager* pVBM = 0;
		while((pVBM = GetDirtyVBM()))
		{
			pVBM->SetOwner(MRTC_SystemInfo::OS_GetThreadID());
			pVBM->End();
			AddAvailVBM(pVBM);
		}
	}

	void AddDirtyVBM(CXR_VBManager* _pVBM)
	{
		M_LOCK(m_Lock);
		m_lpDirtyVBM.Add(_pVBM);
		if(!m_DirtyEvent.TryWait())
			m_DirtyEvent.SetSignaled();
	}

	void AddAvailVBM(CXR_VBManager* _pVBM)
	{
		M_LOCK(m_Lock);
		m_lpAvailVBM.Add(_pVBM);
		if(!m_AvailableEvent.TryWait())
			m_AvailableEvent.SetSignaled();

		if(m_lpAvailVBM.Len() == m_lspVBM.Len())
			m_AllDoneEvent.SetSignaled();
	}
};

typedef TPtr<CXR_VBMContainer> spCXR_VBMContainer;

#endif


#endif //_INC_XRVBManager
