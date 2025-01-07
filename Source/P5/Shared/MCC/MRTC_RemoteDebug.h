
#ifndef DInc_MRTC_RemoteDebug_h
#define DInc_MRTC_RemoteDebug_h

enum
{
	 ERemoteDebug_Init
	,ERemoteDebug_AllocPhysical
	,ERemoteDebug_FreePhysical
	,ERemoteDebug_AllocHeap
	,ERemoteDebug_FreeHeap
	,ERemoteDebug_RegisterPhysicalHeap
	,ERemoteDebug_RegisterHeap
	,ERemoteDebug_ProgramInfo
	,ERemoteDebug_GraphicsBlock
	,ERemoteDebug_NextFrame
	,ERemoteDebug_NextServerTick
	,ERemoteDebug_RunTimeClassRegister
	,ERemoteDebug_RunTimeClassCreate
	,ERemoteDebug_RunTimeClassDestroy
	,ERemoteDebug_RunTimeClassConstructor
	,ERemoteDebug_RunTimeClassDestructor
	,ERemoteDebug_Exception
	,ERemoteDebug_Trace
	,ERemoteDebug_LogFile
	,ERemoteDebug_UnhandeledException
	,ERemoteDebug_ClearHeap
	,ERemoteDebug_KeepAlive
	,ERemoteDebug_DllLoad
	,ERemoteDebug_DllUnload
	,ERemoteDebug_ProfileStackTrace
};

enum
{
	 ERemoteDebugClient_KeepAlive
	,ERemoteDebugClient_EnableFlags
	,ERemoteDebugClient_ChannelData
};


enum ERDEnableFlag
{
	 ERDEnableFlag_Enable = DBit(0)
	,ERDEnableFlag_Scope = DBit(1)
	,ERDEnableFlag_Category = DBit(2)
	,ERDEnableFlag_StackTrace = DBit(3)
	,ERDEnableFlag_PhysicalMemory = DBit(4)
	,ERDEnableFlag_HeapMemory = DBit(5)
	,ERDEnableFlag_SendCRTAllocs = DBit(6)
	,ERDEnableFlag_CObjAllocations = DBit(7)
	,ERDEnableFlag_RuntimeClasses = DBit(8)
	,ERDEnableFlag_RuntimeClassAllocations = DBit(9)
	,ERDEnableFlag_Exceptions = DBit(10)
	,ERDEnableFlag_LogFile = DBit(11)
	,ERDEnableFlag_Trace = DBit(12)
	,ERDEnableFlag_Profiling = DBit(13)
};

class MRTC_RemoteDebugChannel
{
public:
	virtual void SendPacket(const void *_pData, mint _Size) pure;
	virtual const void *GetHeadPacketData(mint &_Size) pure;
	virtual void PopPacket() pure;
	virtual void Delete() pure;
};


#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTCATEGORY
enum
{
	 DRDMaxCategoryMaxStack = 128
	,DRDMaxCategoryMaxStackSize = sizeof(MRTC_RemoteDebugCategory) * DRDMaxCategoryMaxStack
	
};
#else
enum
{
	 DRDMaxCategoryMaxStack = 0
	,DRDMaxCategoryMaxStackSize = 0
};
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER_SUPPORTSCOPE
enum
{
 	 DRDMaxScopeMaxStack = 128
	,DRDMaxScopeMaxStackSize = sizeof(MRTC_RemoteDebugScope) * DRDMaxScopeMaxStack
};
#else
enum
{
 	 DRDMaxScopeMaxStack = 0
	,DRDMaxScopeMaxStackSize = 0
};
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER

enum
{
	 DRDMaxSendStackSize = DRDMaxScopeMaxStackSize + DRDMaxCategoryMaxStackSize + 8192
};


class MRTC_RemoteDebug
{
public:

	class MRTC_RemoteDebugInternal *m_pInternalData;
	class MRTC_RemoteDebugInternal *GetInternal();

	int m_EnableFlags;

	void SendData(uint32 _Message, const void *_pData, mint _DataSize, bool _bAttachStackTrace, bool _bSendCategoryScope, mint _Ebp = 0xffffffff);
	void SendDataOnlyBuffer(uint32 _Message, const void *_pData, mint _DataSize);
	void SendDataRaw(uint32 _Message, const void *_pData, mint _DataSize);

	void Flush();

	void Create(int _Port);

	uint64 GetSequence();
	void Destroy();
	void ModuleInit();
	void ModuleFinish();

	MRTC_RemoteDebugChannel *CreateDebugChannel(int _iChannel);

};

#ifdef PLATFORM_XBOX
#define MRTC_ENABLE_REMOTEDEBUGGER_STATIC
#endif

#ifdef MRTC_ENABLE_REMOTEDEBUGGER_STATIC
extern MRTC_RemoteDebug gs_RemoteDebugger;
#	define MRTC_GetRD() (&gs_RemoteDebugger)
#else
#	define MRTC_GetRD() MRTC_GetObjectManager()->GetRemoteDebugger()
#endif

M_INLINE static uint64 gf_RDGetSequence()
{
#ifndef MRTC_ENABLE_REMOTEDEBUGGER_STATIC
	if (!MRTC_GetObjectManager() || !MRTC_GetObjectManager()->GetRemoteDebugger() || !MRTC_SystemInfo::MRTC_GetSystemInfo().m_pInternalData)
		return 0;
#endif

	return MRTC_GetRD()->GetSequence();
}
void gf_RDSendRegisterPhysicalHeap(mint _Heap, const char *_pName);
void gf_RDSendRegisterHeap(mint _Heap, const char *_pName);
void gf_RDSendPhysicalAlloc(void *_pData, mint _Size, mint _Heap, uint64 _Sequence, uint32 _Type);
void gf_RDSendHeapAlloc(void *_pData, mint _Size, void *_pHeap, uint64 _Sequence, uint32 _Type);
void gf_RDSendHeapFree(void *_pData, void *_pHeap, uint64 _Sequence);
void gf_RDSendPhysicalFree(void *_pData, mint _Heap, uint64 _Sequence);
void gf_RDSendHeapClear(void *_pHeap);

#else

enum
{
	 DRDMaxSendStackSize = 0
};

#define gf_RDSendRegisterPhysicalHeap(_Heap, _pName) ((void )0)
#define gf_RDSendRegisterHeap(_Heap, _pName) ((void )0)
#define gf_RDSendPhysicalAlloc(_pData, _Size, _Heap, _Sequence, _Type) ((void )0)
#define gf_RDSendHeapAlloc(_pData, _Size, _pHeap, _Sequence, _Type) ((void )0)
#define gf_RDSendHeapFree(_pData, _pHeap, _Sequence) ((void )0)
#define gf_RDSendPhysicalFree(_pData, _Heap, _Sequence) ((void )0)
#define gf_RDSendHeapClear(_Heap) ((void )0)



#endif

#endif // DInc_MRTC_RemoteDebug_h

