
#include "PCH.h"

#ifdef PLATFORM_PS3

#include "MSystem_Core.h"
#include "Raster/MRCCore.h"

#include "MContentContext.h"

#include <sysutil/sysutil_common.h>
#include <sysutil/sysutil_sysparam.h>
#include <sysutil/sysutil_savedata.h>

const bool bSaveGameDebugging = false;

#define MEMORY_CONTAINER_SIZE_AUTO		(4 * 1024 * 1024)
#define MEMORY_CONTAINER_SIZE_FIXED		(4 * 1024 * 1024)
#define MEMORY_CONTAINER_SIZE_LIST		(5 * 1024 * 1024)
#define MEMORY_CONTAINER_SIZE_DELETE	(5 * 1024 * 1024)

#define _MAX_(a, b) (((a)>(b))?(a):(b))

#define MEMORY_CONTAINER_SIZE_LARGEST (_MAX_(_MAX_(MEMORY_CONTAINER_SIZE_AUTO, MEMORY_CONTAINER_SIZE_FIXED), _MAX_(MEMORY_CONTAINER_SIZE_LIST, MEMORY_CONTAINER_SIZE_DELETE)))

class CDisplayContextNULL : public CDisplayContext
{
protected:
	MRTC_DECLARE;
public:

	CImage m_Image;

	CDisplayContextNULL()
	{
		m_Image.Create(640, 480, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
	}

	~CDisplayContextNULL()
	{
	}

	virtual CPnt GetScreenSize(){return CPnt(640, 480);};
	virtual CPnt GetMaxWindowSize(){return CPnt(640, 480);};

	virtual void Create()
	{
		CDisplayContext::Create();
	}

	virtual void SetMode(int nr)
	{
	}

	virtual void ModeList_Init()
	{
	}

	virtual int SpawnWindow(int _Flags = 0)
	{
		return 0;
	}

	virtual void DeleteWindow(int _iWnd)
	{
	}

	virtual void SelectWindow(int _iWnd)
	{
	}

	virtual void SetWindowPosition(int _iWnd, CRct _Rct)
	{
	}

	virtual void SetPalette(spCImagePalette _spPal)
	{
	}

	virtual CImage* GetFrameBuffer()
	{
		return &m_Image;
	}

	virtual void ClearFrameBuffer(int _Buffers = (CDC_CLEAR_COLOR | CDC_CLEAR_ZBUFFER | CDC_CLEAR_STENCIL), int _Color = 0)
	{
	}

	int GetMode()
	{
		return 0;
	}


	class CRenderContextNULL : public CRC_Core
	{
	public:

		CDisplayContextNULL *m_pDisplayContext;
		

		void Internal_RenderPolygon(int _nV, const CVec3Dfp32* _pV, const CVec3Dfp32* _pN, const CVec4Dfp32* _pCol = NULL, const CVec4Dfp32* _pSpec = NULL, /*const fp32* _pFog = NULL,*/
									const CVec4Dfp32* _pTV0 = NULL, const CVec4Dfp32* _pTV1 = NULL, const CVec4Dfp32* _pTV2 = NULL, const CVec4Dfp32* _pTV3 = NULL, int _Color = 0xffffffff)
		{
		}

		CRenderContextNULL()
		{
		}

		~CRenderContextNULL()
		{
		}

		void Create(CObj* _pContext, const char* _pParams)
		{
			CRC_Core::Create(_pContext, _pParams);

			MACRO_GetSystem;

			m_Caps_TextureFormats = -1;
			m_Caps_DisplayFormats = -1;
			m_Caps_ZFormats = -1;
			m_Caps_StencilDepth = 8;
			m_Caps_AlphaDepth = 8;
			m_Caps_Flags = -1;

			m_Caps_nMultiTexture = 16;
			m_Caps_nMultiTextureCoords = 8;
			m_Caps_nMultiTextureEnv = 8;

		}

		const char* GetRenderingStatus() { return ""; }
		virtual void Flip_SetInterval(int _nFrames){};

		void Attrib_Set(CRC_Attributes* _pAttrib){}
		void Attrib_SetAbsolute(CRC_Attributes* _pAttrib){}
		void Matrix_SetRender(int _iMode, const CMat4Dfp32* _pMatrix){}

		virtual void Texture_PrecacheFlush(){}
		virtual void Texture_PrecacheBegin( int _Count ){}
		virtual void Texture_PrecacheEnd(){}
		virtual void Texture_Precache(int _TextureID){}

		virtual int Texture_GetBackBufferTextureID() {return 0;}
		virtual int Texture_GetFrontBufferTextureID() {return 0;}
		virtual int Texture_GetZBufferTextureID() {return 0;}
		virtual int Geometry_GetVBSize(int _VBID) {return 0;}

		void Render_IndexedTriangles(uint16* _pTriVertIndices, int _nTriangles){}
		void Render_IndexedTriangleStrip(uint16* _pIndices, int _Len){}
		void Render_IndexedWires(uint16* _pIndices, int _Len){}
		void Render_IndexedPolygon(uint16* _pIndices, int _Len){}
		void Render_IndexedPrimitives(uint16* _pPrimStream, int _StreamLen){}
		void Render_VertexBuffer(int _VBID){}
		void Render_Wire(const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, CPixel32 _Color){}
		void Render_WireStrip(const CVec3Dfp32* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color){}
		void Render_WireLoop(const CVec3Dfp32* _pV, const uint16* _piV, int _nVertices, CPixel32 _Color){}

		virtual void Geometry_PrecacheFlush(){}
		virtual void Geometry_PrecacheBegin( int _Count ){}
		virtual void Geometry_PrecacheEnd(){}
		virtual void Geometry_Precache(int _VBID){}
		virtual CDisplayContext* GetDC(){ return m_pDisplayContext;}

		void Register(CScriptRegisterContext & _RegContext){}

	};

	TPtr<CRenderContextNULL> m_spRenderContext;


	virtual CRenderContext* GetRenderContext(class CRCLock* _pLock)
	{
		if (!m_spRenderContext)
		{
			m_spRenderContext = MNew(CRenderContextNULL);
			m_spRenderContext->m_pDisplayContext = this;
			m_spRenderContext->Create(this, "");

		}

		return m_spRenderContext;
	}

	virtual int Win32_CreateFromWindow(void* _hWnd, int _Flags = 0)
	{
		return 0;
	}

	virtual int Win32_CreateWindow(int _WS, void* _pWndParent, int _Flags = 0)
	{
		return 0;
	}

	virtual void Win32_ProcessMessages()
	{
	}

	virtual void* Win32_GethWnd(int _iWnd = 0)
	{
		return 0;
	}

	virtual void Parser_Modes()
	{
	}

	virtual void Register(CScriptRegisterContext & _RegContext)
	{
		CDisplayContext::Register(_RegContext);
	}

};

MRTC_IMPLEMENT_DYNAMIC_NO_IGNORE(CDisplayContextNULL, CDisplayContext);

class CSaveContentContextPS3 : public CContentContext, public MRTC_Thread
{
public:
	enum
	{
		REQ_TYPE_VOID = 0,

		REQ_TYPE_ENUM_CONTAINERS,
		REQ_TYPE_CONTAINER_CREATE,
		REQ_TYPE_CONTAINER_DELETE,

		REQ_TYPE_CONTAINER_MOUNT,
		REQ_TYPE_CONTAINER_QUERY_SIZE,
		REQ_TYPE_CONTAINER_QUERY_PATH,

		REQ_TYPE_ENUM_INSTANCES,
		REQ_TYPE_INSTANCE_QUERY_SIZE,
		REQ_TYPE_INSTANCE_DELETE,
		REQ_TYPE_INSTANCE_READ,
		REQ_TYPE_INSTANCE_WRITE,

		REQ_TYPE_MAX,
	};

	class CRequest
	{
	public:
		CRequest() : m_Type(REQ_TYPE_VOID), m_pNext(NULL)
		{
		}

		void Reset()
		{
			m_Container.Clear();
			m_Instance.Clear();
			m_pDestArg = NULL;
			m_LenDestArg = 0;

			m_ResultError = CONTENT_ERR_PENDING;
			m_ResultSize = 0;
			m_lResultEnum.Destroy();
		}

		int m_Type;
		CRequest* m_pNext;

		CStr m_Container;
		CStr m_Instance;
		void* m_pDestArg;
		uint32 m_LenDestArg;

		uint32 m_ResultError;
		uint32 m_ResultSize;
		TArray<CStr> m_lResultEnum;

		NThread::CEventAutoReset m_Done;
	};

	MRTC_CriticalSection m_RequestLock;
	TThinArray<CRequest> m_lRequests;
	CRequest* m_pRequestHead;
	CRequest* m_pRequestTail;
	NThread::CEventAutoResetReportable m_RequestAvail;
	sys_memory_container_t m_MemoryContainer;
	
	int AllocRequestInstance();
	void QueueRequest(CRequest* _pReq);

	int AllocRequest(int _Type);
	int AllocRequest(int _Type, CStr _Container);
	int AllocRequest(int _Type, CStr _Container, CStr _Instance);
	int AllocRequest(int _Type, CStr _Container, CStr _Instance, void* _pData, uint _DataLen);
	void FreeRequest(int _iRequest);

	const char* Thread_GetName() {return "CSaveContentContextPS3";}
public:
	CSaveContentContextPS3() : m_pRequestHead(NULL), m_pRequestTail(NULL) {}
	int Thread_Main();

	void Create();
	void Destroy()
	{
		Thread_Destroy();
	}

	int StorageSelect(uint _iUser, uint _SaveSize) {return -1;}

	int EnumerateContainers(uint _iUser);
	int ContainerCreate(uint _iUser, CStr _Container);
	int ContainerDelete(uint _iUser, CStr _Container);

	int ContainerMount(uint _iUser, CStr _Container);
	int ContainerQuerySize(uint _iUser, CStr _Container);
	int ContainerQueryPath(uint _iUser, CStr _Container);

	int EnumerateInstances(uint _iUser, CStr _Container);
	int InstanceQuerySize(uint _iUser, CStr _Container, CStr _Instance);
	int InstanceDelete(uint _iUser, CStr _Container, CStr _Instance);
	int InstanceRead(uint _iUser, CStr _Container, CStr _Instance, void* _pDest, uint32 _MaxLen);
	int InstanceWrite(uint _iUser, CStr _Container, CStr _Instance, void* _pSrc, uint32 _Len);

	int GetRequestStatus(int _iRequest);
	int BlockOnRequest(int _iRequest);

	int PeekRequestData(int _iRequest, CContentQueryData* _pRet);
	int GetRequestData(int _iRequest, CContentQueryData* _pRet);
	int GetRequestEnumeration(int _iRequest, TArray<CStr>& _lData);
};

class CSystemPS3 : public CSystemCore
{
	MRTC_DECLARE;

	CSaveContentContextPS3 m_SaveGameContext;
public:

	virtual void Create(void* this_inst, void* prev_inst, char* cmdline, int _cmdshow, const char* _pAppClassName)
	{
		CSystemCore::Create(cmdline, _pAppClassName);
	}


	virtual void DC_InitList()
	{
//		m_lspDC.Add(MNew4(CDisplayContextDesc, "", "PS3", "PS3 DisplayContext", "CDisplayContextPS3") );
		m_lspDC.Add(MNew4(CDisplayContextDesc, "", "NULL", "NULL DisplayContext", "CDisplayContextNULL") );
	}

	virtual void Render(CDisplayContext* _pDC, int _Context)
	{
		MSCOPE(CSystemWin32::Render, SYSTEMWIN32);
		
		if (IsRendering()) return; // Error("Render", "Render-recusion.");
		if (!m_spApp) return;
		if (!_pDC || (_pDC == m_spDisplay))
		{
			_pDC = m_spDisplay;
			if (!_pDC) return;

			if (m_iMainWindow < 0)
			{
				m_iMainWindow = m_spDisplay->SpawnWindow();

				if(m_iMainWindow >= 0)
				{
				}
			}
		}
		if (!_pDC) return;
		if (_pDC->GetMode() < 0) return;

		// Return false if displaycontext isn't valid (not created yet, re-creating, etc.)
		if(!_pDC->IsValid())
			return;

		m_bIsRendering = true;
		M_TRY
		{
			CImage *spImg = _pDC->GetFrameBuffer();
			if (spImg != NULL)
			{
				CClipRect Clip = spImg->GetClipRect();

				m_spApp->OnRender(_pDC, spImg, Clip, _Context);
			};
		}
		M_CATCH(
		catch(CCException)
		{
			m_bIsRendering = false;
			throw;
		};
		)
		m_bIsRendering = false;
	}

	CStr GetModuleName(CStr _Name, bool _bSystemPath)
	{
		return _Name + ".dll";
	}

	CContentContext* GetContentContext(int _Context)
	{
		switch(_Context)
		{
		case CONTENT_SAVEGAMES:
			return &m_SaveGameContext;
		}
		return NULL;
	}
};


MRTC_IMPLEMENT_DYNAMIC(CSystemPS3, CSystemCore);

void CSaveContentContextPS3::FreeRequest(int _iRequest)
{
	M_LOCK(m_RequestLock);
	m_lRequests[_iRequest].m_Type = REQ_TYPE_VOID;
	m_lRequests[_iRequest].Reset();
}

int CSaveContentContextPS3::AllocRequestInstance()
{
	M_LOCK(m_RequestLock);
	TAP_RCD<CRequest> pRequests(m_lRequests);
	for(int i = 0; i < pRequests.Len(); i++)
	{
		if(pRequests[i].m_Type == REQ_TYPE_VOID)
		{
			pRequests[i].Reset();
			return i;
		}
	}

	return -1;
}

void CSaveContentContextPS3::QueueRequest(CRequest* _pReq)
{
	{
		M_LOCK(m_RequestLock);
		if(m_pRequestHead)
			m_pRequestTail->m_pNext = _pReq;
		else
			m_pRequestHead = _pReq;
		m_pRequestTail = _pReq;
	}
	m_RequestAvail.Signal();
}

int CSaveContentContextPS3::AllocRequest(int _Type)
{
	int Request = AllocRequestInstance();
	if(Request >= 0)
	{
		CRequest* pReq = &m_lRequests[Request];
		pReq->m_Type = _Type;
		QueueRequest(pReq);
	}

	return Request;
}

int CSaveContentContextPS3::AllocRequest(int _Type, CStr _Container)
{
	int Request = AllocRequestInstance();
	if(Request >= 0)
	{
		CRequest* pReq = &m_lRequests[Request];
		pReq->m_Type = _Type;
		pReq->m_Container = _Container;
		QueueRequest(pReq);
	}

	return Request;
}

int CSaveContentContextPS3::AllocRequest(int _Type, CStr _Container, CStr _Instance)
{
	int Request = AllocRequestInstance();
	if(Request >= 0)
	{
		CRequest* pReq = &m_lRequests[Request];
		pReq->m_Type = _Type;
		pReq->m_Container = _Container;
		pReq->m_Instance = _Instance;
		QueueRequest(pReq);
	}

	return Request;
}

int CSaveContentContextPS3::AllocRequest(int _Type, CStr _Container, CStr _Instance, void* _pDest, uint _DestLen)
{
	int Request = AllocRequestInstance();
	if(Request >= 0)
	{
		CRequest* pReq = &m_lRequests[Request];
		pReq->m_Type = _Type;
		pReq->m_Container = _Container;
		pReq->m_Instance = _Instance;
		pReq->m_pDestArg = _pDest;
		pReq->m_LenDestArg = _DestLen;
		QueueRequest(pReq);
	}

	return Request;
}

#define NEWDATA_NAME_LIST		"Create New Game Data"
#define MAX_LISTSAVEDATA_NUM 5

class CSaveQueueItem
{
public:
	CFStr m_Filename;
	void* m_pData;
	uint32 m_DataSize;
	uint32 m_Type;
};

CMTime ReqStart;
class CSaveQueue
{
public:
	CSaveQueue() : m_pRequest(NULL) {}
	void Create()
	{
		m_lThumbnail = CDiskUtil::ReadFileToArray("Thumbnail.png", CFILE_READ | CFILE_BINARY);
	}
	void CreateQueue(CFStr _TitleName, CFStr _DirectoryName, uint _Len)
	{
		m_TitleName = _TitleName;
		m_DirectoryName = _DirectoryName.UpperCase();
		m_lQueue.SetLen(_Len);
		m_nItems = _Len;
		m_iCurrentItems = 0;
		M_TRACEALWAYS("CreateQueue('%s', '%s', %d)\n", m_TitleName.GetStr(), m_DirectoryName.GetStr(), m_nItems);
	}

	void DestroyQueue()
	{
		m_lQueue.Destroy();
		m_nItems = 0;
		m_iCurrentItems = 0;
	}

	void ResetQueue()
	{
		m_iCurrentItems = 0;
	}

	void InsertQueueItem(uint _iPos, CFStr _Filename, void* _pData, uint _DataSize, uint _Type)
	{
		CSaveQueueItem& Item = m_lQueue[_iPos];
		Item.m_Filename = _Filename.UpperCase();
		Item.m_pData = _pData;
		Item.m_DataSize = _DataSize;
		Item.m_Type = _Type;
	}

	void SetRequest(CSaveContentContextPS3::CRequest* _pRequest)
	{
		m_pRequest = _pRequest;
	}

	CSaveContentContextPS3::CRequest* m_pRequest;
	CFStr m_TitleName;
	CFStr m_DirectoryName;
	TThinArray<CSaveQueueItem> m_lQueue;
	uint m_nItems;
	uint m_iCurrentItems;
	TArray<uint8> m_lThumbnail;
};

static CSaveQueue s_SaveQueue;

static void dumpCellSaveDataListGet( CellSaveDataListGet *get )
{
	M_TRACEALWAYS("Dump CellSaveDataListGet in CellSaveDataListCallback--------------------\n" );
	M_TRACEALWAYS("\tget->dirNum : %d\n", get->dirNum );
	M_TRACEALWAYS("\tget->dirListNum: %d\n", get->dirListNum );

	for( unsigned int i = 0; i < get->dirListNum; i++ ) {
		M_TRACEALWAYS("\t%4d  DIRNAME: %s\t\tPARAM: %s\n", i, get->dirList[i].dirName, get->dirList[i].listParam );
	}
}

static void dumpCellSaveDataListSet( CellSaveDataListSet *set )
{
	M_TRACEALWAYS("Dump CellSaveDataListSet in CellSaveDataListCallback--------------------\n" );
	M_TRACEALWAYS("\tset->focusPosition : %d\n", set->focusPosition);
	M_TRACEALWAYS("\tset->focusDirName : %s\n", set->focusDirName);
	M_TRACEALWAYS("\tset->fixedListNum : %d\n", set->fixedListNum);
	for(int i = 0; i < set->fixedListNum; i++)
	{
		M_TRACEALWAYS("\t\tset->fixedList[%d].dirName : %s\n", i, set->fixedList[i].dirName);
		M_TRACEALWAYS("\t\tset->fixedList[%d].listParam : %s\n", i, set->fixedList[i].listParam);
	}
	M_TRACEALWAYS("\tset->newData : 0x%.8X\n", set->newData);
	if(set->newData)
	{
		M_TRACEALWAYS("\t\tset->newData->iconPosition : %d\n", set->newData->iconPosition);
		M_TRACEALWAYS("\t\tset->newData->dirName : %s\n", set->newData->dirName);
		M_TRACEALWAYS("\t\tset->newData->icon : 0x%.8X\n", set->newData->icon);
		if(set->newData->icon)
		{
			M_TRACEALWAYS("\t\t\tset->newData->icon->title : %s\n", set->newData->icon->title);
			M_TRACEALWAYS("\t\t\tset->newData->icon->iconBufSize : %d\n", set->newData->icon->iconBufSize);
			M_TRACEALWAYS("\t\t\tset->newData->icon->iconBuf : 0x%.8X\n", set->newData->icon->iconBuf);
		}
	}
}

static void dumpCellSaveDataStatSet(CellSaveDataStatSet* _pSet)
{
	M_TRACEALWAYS("Dump CellSaveDataStatSet in CellSaveDataStatCallback--------------------\n" );
	M_TRACEALWAYS("\tset->setParam: 0x%.8X\n", _pSet->setParam );
	if(_pSet->setParam)
	{
		M_TRACEALWAYS("\t\tset->setParam->title: %s\n", _pSet->setParam->title );
		M_TRACEALWAYS("\t\tset->setParam->subTitle: %s\n", _pSet->setParam->subTitle );
		M_TRACEALWAYS("\t\tset->setParam->detail: %s\n", _pSet->setParam->detail );
		M_TRACEALWAYS("\t\tset->setParam->attribute: %d\n", _pSet->setParam->attribute );
		M_TRACEALWAYS("\t\tset->setParam->parentalLevel: %d\n", _pSet->setParam->parentalLevel );
		M_TRACEALWAYS("\t\tset->setParam->listParam: %s\n", _pSet->setParam->listParam );
	}
	M_TRACEALWAYS("\tset->reCreateMode: %d\n", _pSet->reCreateMode );
}

static void dumpCellSaveDataStatGet(CellSaveDataStatGet* _pGet)
{
	M_TRACEALWAYS("Dump CellSaveDataStatGet in CellSaveDataStatCallback--------------------\n" );
	M_TRACEALWAYS("\tget->dir.dirName : %s\n", _pGet->dir.dirName );
	M_TRACEALWAYS("\tget->isNewData: %d\n", _pGet->isNewData );
	M_TRACEALWAYS("\tget->hddFreeSizeKB 0x%x\n", _pGet->hddFreeSizeKB);
	M_TRACEALWAYS("\tget->sizeKB : 0x%x\n", _pGet->sizeKB);
	M_TRACEALWAYS("\tget->sysSizeKB : 0x%x\n", _pGet->sysSizeKB);
	M_TRACEALWAYS("\tget->bind : %d\n", _pGet->bind);
	M_TRACEALWAYS("\tget->dir : %s  %lld  %lld  %lld\n", _pGet->dir.dirName, _pGet->dir.st_atime, _pGet->dir.st_ctime, _pGet->dir.st_mtime );
	M_TRACEALWAYS("\tget->fileListNum: %d\n", _pGet->fileListNum );

	for( unsigned int i = 0; i < _pGet->fileListNum; i++ ) {
		M_TRACEALWAYS("\t%3d  FILENAME: %s   type : %d  size : %lld  atime : %lld mtime : %lld ctime : %lld\n", i,
			_pGet->fileList[i].fileName,
			_pGet->fileList[i].fileType,
			_pGet->fileList[i].st_size,
			_pGet->fileList[i].st_atime,
			_pGet->fileList[i].st_mtime,
			_pGet->fileList[i].st_ctime );
	}
	M_TRACEALWAYS("\tget->fileNum: %d\n", _pGet->fileNum );
	M_TRACEALWAYS("\n" );
	M_TRACEALWAYS("\tPARAM.SFO:TITLE: %s\n", _pGet->getParam.title );
	M_TRACEALWAYS("\tPARAM.SFO:SUB_TITLE: %s\n", _pGet->getParam.subTitle );
	M_TRACEALWAYS("\tPARAM.SFO:DETAIL: %s\n", _pGet->getParam.detail );
	M_TRACEALWAYS("\tPARAM.SFO:ATTRIBUTE: %d\n", _pGet->getParam.attribute );
	M_TRACEALWAYS("\tPARAM.SFO:PARENTAL_LEVEL: %d\n", _pGet->getParam.parentalLevel );
	M_TRACEALWAYS("\tPARAM.SFO:LIST_PARAM: %s\n", _pGet->getParam.listParam );
	M_TRACEALWAYS("\n" );
}

static void dumpCellSaveDataFileGet( CellSaveDataFileGet *get )
{
	M_TRACEALWAYS("Dump dumpCellSaveDataFileGet in CellSaveDataFileCallback--------------------\n" );
	M_TRACEALWAYS("\tget->dirNum : %d\n", get->excSize );
}

#define PRODUCTCODE "SBZE12345"

static void cb_data_list( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataListSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_list() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataListGet(get);

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_list() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}

static void cb_enum_instances_data_list_load( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataListSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_enum_instances_data_list_load() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataListGet(get);

	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_enum_instances_data_list_load() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}

static void cb_enum_containers_data_list_load( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataListSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_enum_containers_data_list_load() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataListGet(get);

	result->result = CELL_SAVEDATA_CBRESULT_OK_LAST;
	if(s_SaveQueue.m_pRequest != NULL)
	{
		if(get->dirListNum > 0)
		{
			for(int i = 0; i< get->dirListNum; i++)
				s_SaveQueue.m_pRequest->m_lResultEnum.Add(get->dirList[i].dirName + sizeof(PRODUCTCODE));
		}
	}

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_enum_containers_data_list_load() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}

static void cb_data_list_save( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataListSet *set )
{
	static CellSaveDataListNewData newData;
	static CellSaveDataNewDataIcon newDataIcon;
	unsigned int thumb_size;

	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_list_save() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataListGet(get);

	set->fixedList = get->dirList;
	set->fixedListNum = get->dirListNum;
	set->focusPosition = CELL_SAVEDATA_FOCUSPOS_LISTHEAD;
	set->focusDirName = NULL;
	set->reserved = NULL;
	set->newData = NULL;

	if( get->dirListNum < MAX_LISTSAVEDATA_NUM )
	{
		set->newData = &newData;			
		newData.dirName = s_SaveQueue.m_DirectoryName.GetStr();
		newData.iconPosition = CELL_SAVEDATA_ICONPOS_HEAD;
		newData.reserved = NULL;

		newData.icon = &newDataIcon;
		newDataIcon.title = s_SaveQueue.m_TitleName.GetStr();

		newDataIcon.iconBuf = s_SaveQueue.m_lThumbnail.GetBasePtr();
		newDataIcon.iconBufSize = s_SaveQueue.m_lThumbnail.Len();
		newDataIcon.reserved = NULL;
	}

	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;

	if(bSaveGameDebugging) dumpCellSaveDataListSet(set);

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_list_save() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}
static void cb_data_list_load( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataListSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_list_load() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataListGet(get);

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_list_load() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}
static void cb_data_fixed_save( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataFixedSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_fixed_save() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataListGet(get);

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_fixed_save() end\n");
}
static void cb_data_fixed_load( CellSaveDataCBResult *result, CellSaveDataListGet *get, CellSaveDataFixedSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_fixed_load() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataListGet(get);

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_fixed_load() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}

static void cb_delete_data_status_list_save( CellSaveDataCBResult *result, CellSaveDataStatGet *get, CellSaveDataStatSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_delete_data_status_list_save() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataStatGet(get);

	set->reCreateMode = CELL_SAVEDATA_RECREATE_NO;
	set->setParam = &get->getParam;
	set->reserved = NULL;

	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;
	result->reserved = NULL;

	if(bSaveGameDebugging) dumpCellSaveDataStatSet(set);

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_delete_data_status_list_save() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}
static void cb_query_size_data_status_list_save( CellSaveDataCBResult *result, CellSaveDataStatGet *get, CellSaveDataStatSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_query_size_data_status_list_save() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataStatGet(get);

	set->reCreateMode = CELL_SAVEDATA_RECREATE_NO;
	set->setParam = &get->getParam;
	set->reserved = NULL;

	if(!get->isNewData)
	{
		for(int j = 0; j < get->fileListNum; j++)
		{
			if(get->fileList[j].fileType == CELL_SAVEDATA_FILETYPE_NORMALFILE)
			{
				if(s_SaveQueue.m_lQueue[0].m_Filename.CompareNoCase(get->fileList[j].fileName) == 0)
				{
					s_SaveQueue.m_pRequest->m_ResultSize = get->fileList[j].st_size;
					break;
				}
			}
		}
	}

	result->result = CELL_SAVEDATA_CBRESULT_OK_LAST;
	result->reserved = NULL;

	if(bSaveGameDebugging) dumpCellSaveDataStatSet(set);

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_query_size_data_status_list_save() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}
static void cb_enum_instances_data_status_list_save( CellSaveDataCBResult *result, CellSaveDataStatGet *get, CellSaveDataStatSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_enum_instances_data_status_list_save() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataStatGet(get);

	set->reCreateMode = CELL_SAVEDATA_RECREATE_NO;
	set->setParam = &get->getParam;
	set->reserved = NULL;

	if(!get->isNewData)
	{
		for(int j = 0; j < get->fileListNum; j++)
		{
			if(get->fileList[j].fileType == CELL_SAVEDATA_FILETYPE_NORMALFILE)
				s_SaveQueue.m_pRequest->m_lResultEnum.Add(get->fileList[j].fileName);
		}
	}

	result->result = CELL_SAVEDATA_CBRESULT_OK_LAST;
	result->reserved = NULL;

	if(bSaveGameDebugging) dumpCellSaveDataStatSet(set);

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_enum_instances_data_status_list_save() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}
static void cb_data_status_list_save( CellSaveDataCBResult *result, CellSaveDataStatGet *get, CellSaveDataStatSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_status_list_save() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataStatGet(get);

	set->reCreateMode = CELL_SAVEDATA_RECREATE_NO;
	set->setParam = &get->getParam;
	set->reserved = NULL;

	if(get->isNewData)
	{
		strcpy(set->setParam->title, "The Darkness Saved Game");
		strcpy(set->setParam->subTitle, s_SaveQueue.m_DirectoryName.GetStr() + sizeof(PRODUCTCODE));
		strcpy(set->setParam->detail, "Hip detail description");

		set->setParam->attribute = CELL_SAVEDATA_ATTR_NORMAL;
		set->setParam->parentalLevel = 1;
		strcpy( set->setParam->listParam, "MOOMIN" );

		uint RequiredSize = 0;
		for(int i = 0; i < s_SaveQueue.m_nItems; i++)
			RequiredSize += s_SaveQueue.m_lQueue[i].m_DataSize;

		if(get->hddFreeSizeKB < RequiredSize)
		{
			result->errNeedSizeKB = RequiredSize;
			result->result = CELL_SAVEDATA_CBRESULT_ERR_NOSPACE;
			return;
		}
	}
	else
	{
		for(int i = 0; i < s_SaveQueue.m_nItems; i++)
		{
			for(int j = 0; j < get->fileListNum; j++)
			{
				if(!strcmp(get->fileList[j].fileName, s_SaveQueue.m_lQueue[i].m_Filename.GetStr()))
				{
					// Do size-diffrence check
				}
			}
		}
	}

	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;
	result->reserved = NULL;

	if(bSaveGameDebugging) dumpCellSaveDataStatSet(set);

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_status_list_save() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}

static void cb_data_status_list_load( CellSaveDataCBResult *result, CellSaveDataStatGet *get, CellSaveDataStatSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_status_list_load() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataStatGet(get);

	int foundMustExistData = 0;

	if(get->isNewData)
	{
		M_TRACEALWAYS("Data %s is not found\n", get->dir.dirName );
		result->result = CELL_SAVEDATA_CBRESULT_ERR_NODATA;
		return;
	}
	else
	{
		M_TRACEALWAYS("-- %s : %s - %d -- fileListNum=[%d] fileNum=[%d]\n", __FILE__, __FUNCTION__, __LINE__ , get->fileListNum, get->fileNum);
		if ( get->fileListNum < get->fileNum )
		{
			result->result = CELL_SAVEDATA_CBRESULT_ERR_BROKEN;
			return;
		}
	}


	set->reCreateMode = CELL_SAVEDATA_RECREATE_NO;
	set->setParam = NULL;
	set->reserved = NULL;


	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;
	result->reserved = NULL;


	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_status_list_load() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}
static void cb_data_status_save( CellSaveDataCBResult *result, CellSaveDataStatGet *get, CellSaveDataStatSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_status_save() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataStatGet(get);

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_status_save() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}
static void cb_data_status_load( CellSaveDataCBResult *result, CellSaveDataStatGet *get, CellSaveDataStatSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_status_load() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataStatGet(get);

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_data_status_load() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}
static void cb_file_operation_save( CellSaveDataCBResult *result, CellSaveDataFileGet *get, CellSaveDataFileSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_file_operation_save() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataFileGet(get);

	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;

	if(s_SaveQueue.m_iCurrentItems < s_SaveQueue.m_nItems)
	{
		M_TRACEALWAYS("--- Saving '%s'...\n", s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_Filename.GetStr());
		result->progressBarInc = 100/s_SaveQueue.m_nItems;
		set->fileOperation = CELL_SAVEDATA_FILEOP_WRITE;
		set->reserved = NULL;

		set->fileType = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_Type;
		//set->secureFileId

		set->fileName = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_Filename.GetStr();

		set->fileOffset = 0;
		set->fileSize = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_DataSize;
		set->fileBufSize = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_DataSize;
		set->fileBuf = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_pData;
		s_SaveQueue.m_iCurrentItems++;
	}
	else
		result->result = CELL_SAVEDATA_CBRESULT_OK_LAST;

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_file_operation_save() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}
static void cb_file_operation_delete( CellSaveDataCBResult *result, CellSaveDataFileGet *get, CellSaveDataFileSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_file_operation_save() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataFileGet(get);

	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;

	if(s_SaveQueue.m_iCurrentItems < s_SaveQueue.m_nItems)
	{
		M_TRACEALWAYS("--- Deleting '%s'...\n", s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_Filename.GetStr());
		result->progressBarInc = 100/s_SaveQueue.m_nItems;
		set->fileOperation = CELL_SAVEDATA_FILEOP_DELETE;
		set->reserved = NULL;

		set->fileType = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_Type;
		//set->secureFileId

		set->fileName = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_Filename.GetStr();

		set->fileOffset = 0;
		set->fileSize = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_DataSize;
		set->fileBufSize = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_DataSize;
		set->fileBuf = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_pData;
		s_SaveQueue.m_iCurrentItems++;
	}
	else
		result->result = CELL_SAVEDATA_CBRESULT_OK_LAST;

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_file_operation_save() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}
static void cb_file_operation_load( CellSaveDataCBResult *result, CellSaveDataFileGet *get, CellSaveDataFileSet *set )
{
	if(bSaveGameDebugging) M_TRACEALWAYS("Entry TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
	if(bSaveGameDebugging) M_TRACEALWAYS("cb_file_operation_load() start\n");
	if(bSaveGameDebugging) dumpCellSaveDataFileGet(get);

	result->result = CELL_SAVEDATA_CBRESULT_OK_NEXT;

	if(s_SaveQueue.m_iCurrentItems < s_SaveQueue.m_nItems)
	{
		if(bSaveGameDebugging) M_TRACEALWAYS("--- Loading '%s'...\n", s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_Filename.GetStr());
		result->progressBarInc = 100/s_SaveQueue.m_nItems;
		set->fileOperation = CELL_SAVEDATA_FILEOP_READ;
		set->reserved = NULL;

		set->fileType = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_Type;
		//set->secureFileId

		set->fileName = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_Filename.GetStr();

		set->fileOffset = 0;
		set->fileSize = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_DataSize;
		set->fileBufSize = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_DataSize;
		set->fileBuf = s_SaveQueue.m_lQueue[s_SaveQueue.m_iCurrentItems].m_pData;
		s_SaveQueue.m_iCurrentItems++;
	}
	else
		result->result = CELL_SAVEDATA_CBRESULT_OK_LAST;

	if(bSaveGameDebugging) M_TRACEALWAYS("cb_file_operation_load() end\n");
	if(bSaveGameDebugging) M_TRACEALWAYS("Exit TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
}

int CSaveContentContextPS3::Thread_Main()
{
	m_QuitEvent.ReportTo(&m_RequestAvail);
	while(!Thread_IsTerminating())
	{
		m_RequestAvail.Wait();
		if(Thread_IsTerminating())
			break;

		do 
		{
			// Find a request to service
			CRequest* pReq = NULL;
			{
				M_LOCK(m_RequestLock);
				if(m_pRequestHead == NULL)
					break;

				pReq = m_pRequestHead;
				m_pRequestHead = pReq->m_pNext;
				if(m_pRequestHead == NULL)
					m_pRequestTail = NULL;
				pReq->m_pNext = NULL;
			}

			if(bSaveGameDebugging) ReqStart = CMTime::GetCPU();
			if(bSaveGameDebugging) M_TRACEALWAYS("Req %d - '%s', '%s'\n", pReq->m_Type, pReq->m_Container.GetStr(), pReq->m_Instance.GetStr());
			if(bSaveGameDebugging) M_TRACEALWAYS("ReqStart TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
			switch(pReq->m_Type)
			{
			default:
				pReq->m_ResultError = CONTENT_ERR_UNSUPPORTED;
				break;

			case REQ_TYPE_ENUM_CONTAINERS:
				{
					CellSaveDataSetList SetList;
					CellSaveDataSetBuf SetBuf;

					SetList.sortType = CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME;
					SetList.sortOrder = CELL_SAVEDATA_SORTORDER_ASCENT;
					SetList.dirNamePrefix = PRODUCTCODE;
					SetList.reserved = 0;

					SetBuf.dirListMax = 10;
					SetBuf.fileListMax = 40;
					memset(SetBuf.reserved, 0, sizeof(SetBuf.reserved));
					SetBuf.bufSize = Max( 10 * sizeof(CellSaveDataDirList), 40 * sizeof(CellSaveDataFileStat) );
					TThinArray<uint8> lBuf; lBuf.SetLen(SetBuf.bufSize);
					SetBuf.buf = lBuf.GetBasePtr();

					int32 ResultError = CONTENT_ERR_GENERIC_READ;
					if(bSaveGameDebugging) M_TRACEALWAYS("Starting enumeration\n");
					s_SaveQueue.SetRequest(pReq);
					do 
					{
						if(bSaveGameDebugging) M_TRACEALWAYS("-- calling cellSaveDataListLoad, TimeStamp  %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
						if(cellSaveDataListLoad(CELL_SAVEDATA_VERSION_CURRENT, &SetList, &SetBuf, cb_enum_containers_data_list_load, cb_data_status_list_load, cb_file_operation_load, m_MemoryContainer))
							break;
						ResultError = CONTENT_OK;
					} while(0);
					s_SaveQueue.SetRequest(NULL);
					if(bSaveGameDebugging) M_TRACEALWAYS("Enumeration completed\n");

					pReq->m_ResultError = ResultError;
					break;
				}

			case REQ_TYPE_CONTAINER_CREATE:
				{
					CellSaveDataSetList SetList;
					CellSaveDataSetBuf SetBuf;

					SetList.sortType = CELL_SAVEDATA_SORTTYPE_MODIFIEDTIME;
					SetList.sortOrder = CELL_SAVEDATA_SORTORDER_ASCENT;
					SetList.dirNamePrefix = PRODUCTCODE;
					SetList.reserved = 0;

					SetBuf.dirListMax = 10;
					SetBuf.fileListMax = 40;
					memset(SetBuf.reserved, 0, sizeof(SetBuf.reserved));
					SetBuf.bufSize = Max( 10 * sizeof(CellSaveDataDirList), 40 * sizeof(CellSaveDataFileStat) );
					TThinArray<uint8> lBuf; lBuf.SetLen(SetBuf.bufSize);
					SetBuf.buf = lBuf.GetBasePtr();

					TArray<uint8> lIcon0 = CDiskUtil::ReadFileToArray("icon0.png", CFILE_READ | CFILE_BINARY);
					TArray<uint8> lIcon1 = CDiskUtil::ReadFileToArray("icon1.pam", CFILE_READ | CFILE_BINARY);
					TArray<uint8> lPic1 = CDiskUtil::ReadFileToArray("pic1.png", CFILE_READ | CFILE_BINARY);


					int32 ResultError = CONTENT_ERR_GENERIC_WRITE;
					s_SaveQueue.CreateQueue(pReq->m_Container.GetStr(), CFStrF("%s-%s", PRODUCTCODE, pReq->m_Container.GetStr()), 1);
					do
					{
						{
							s_SaveQueue.InsertQueueItem(0, "ICON0.PNG", lIcon0.GetBasePtr(), lIcon0.ListSize(), CELL_SAVEDATA_FILETYPE_CONTENT_ICON0);
							if(bSaveGameDebugging) M_TRACEALWAYS("-- calling cellSaveDataAutoSave, TimeStamp  %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
							if(cellSaveDataAutoSave(CELL_SAVEDATA_VERSION_CURRENT, CFStrF("%s-%s", PRODUCTCODE, pReq->m_Container.GetStr()).GetStr(), CELL_SAVEDATA_ERRDIALOG_NONE, &SetBuf, cb_data_status_list_save, cb_file_operation_save, m_MemoryContainer))
								break;
						}
						{
							s_SaveQueue.ResetQueue();
							s_SaveQueue.InsertQueueItem(0, "ICON1.PAM", lIcon1.GetBasePtr(), lIcon1.ListSize(), CELL_SAVEDATA_FILETYPE_CONTENT_ICON1);
							if(bSaveGameDebugging) M_TRACEALWAYS("-- calling cellSaveDataAutoSave, TimeStamp  %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
							if(cellSaveDataAutoSave(CELL_SAVEDATA_VERSION_CURRENT, CFStrF("%s-%s", PRODUCTCODE, pReq->m_Container.GetStr()).GetStr(), CELL_SAVEDATA_ERRDIALOG_NONE, &SetBuf, cb_data_status_list_save, cb_file_operation_save, m_MemoryContainer))
								break;
						}
						{
							s_SaveQueue.ResetQueue();
							s_SaveQueue.InsertQueueItem(0, "PIC1.PNG", lPic1.GetBasePtr(), lPic1.ListSize(), CELL_SAVEDATA_FILETYPE_CONTENT_PIC1);
							if(bSaveGameDebugging) M_TRACEALWAYS("-- calling cellSaveDataAutoSave, TimeStamp  %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
							if(cellSaveDataAutoSave(CELL_SAVEDATA_VERSION_CURRENT, CFStrF("%s-%s", PRODUCTCODE, pReq->m_Container.GetStr()).GetStr(), CELL_SAVEDATA_ERRDIALOG_NONE, &SetBuf, cb_data_status_list_save, cb_file_operation_save, m_MemoryContainer))
								break;
						}
						ResultError = CONTENT_OK;
					} while(0);
					s_SaveQueue.DestroyQueue();
					pReq->m_ResultError = ResultError;
					break;
				}

			case REQ_TYPE_CONTAINER_DELETE:
				{
					if(cellSaveDataDelete(m_MemoryContainer))
					{
						pReq->m_ResultError = CONTENT_ERR_GENERIC_WRITE;
					}
					else
						pReq->m_ResultError = CONTENT_OK;
					break;
				}

			case REQ_TYPE_INSTANCE_WRITE:
				{
					CellSaveDataSetBuf SetBuf;

					SetBuf.dirListMax = 10;
					SetBuf.fileListMax = 40;
					memset(SetBuf.reserved, 0, sizeof(SetBuf.reserved));
					SetBuf.bufSize = Max( 10 * sizeof(CellSaveDataDirList), 40 * sizeof(CellSaveDataFileStat) );
					TThinArray<uint8> lBuf; lBuf.SetLen(SetBuf.bufSize);
					SetBuf.buf = lBuf.GetBasePtr();

					int32 ResultError = CONTENT_ERR_GENERIC_WRITE;
					s_SaveQueue.CreateQueue(pReq->m_Container.GetStr(), CFStrF("%s-%s", PRODUCTCODE, pReq->m_Container.GetStr()), 1);
					do 
					{
						s_SaveQueue.InsertQueueItem(0, pReq->m_Instance.GetStr(), pReq->m_pDestArg, pReq->m_LenDestArg, CELL_SAVEDATA_FILETYPE_NORMALFILE);
						if(bSaveGameDebugging) M_TRACEALWAYS("-- calling cellSaveDataAutoSave, TimeStamp  %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
						if(cellSaveDataAutoSave(CELL_SAVEDATA_VERSION_CURRENT, CFStrF("%s-%s", PRODUCTCODE, pReq->m_Container.GetStr()).GetStr(), CELL_SAVEDATA_ERRDIALOG_NONE, &SetBuf, cb_data_status_list_save, cb_file_operation_save, m_MemoryContainer))
							break;
						ResultError = CONTENT_OK;
					} while(0);
					s_SaveQueue.DestroyQueue();

					pReq->m_ResultError = ResultError;
					break;
				}

			case REQ_TYPE_INSTANCE_READ:
				{
					CellSaveDataSetBuf SetBuf;

					SetBuf.dirListMax = 10;
					SetBuf.fileListMax = 40;
					memset(SetBuf.reserved, 0, sizeof(SetBuf.reserved));
					SetBuf.bufSize = Max( 10 * sizeof(CellSaveDataDirList), 40 * sizeof(CellSaveDataFileStat) );
					TThinArray<uint8> lBuf; lBuf.SetLen(SetBuf.bufSize);
					SetBuf.buf = lBuf.GetBasePtr();

					int32 ResultError = CONTENT_ERR_GENERIC_READ;
					s_SaveQueue.CreateQueue(pReq->m_Container.GetStr(), CFStrF("%s-%s", PRODUCTCODE, pReq->m_Container.GetStr()), 1);
					do 
					{
						s_SaveQueue.InsertQueueItem(0, pReq->m_Instance.GetStr(), pReq->m_pDestArg, pReq->m_LenDestArg, CELL_SAVEDATA_FILETYPE_NORMALFILE);
						if(bSaveGameDebugging) M_TRACEALWAYS("-- calling cellSaveDataAutoLoad, TimeStamp  %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
						if(cellSaveDataAutoLoad(CELL_SAVEDATA_VERSION_CURRENT, CFStrF("%s-%s", PRODUCTCODE, pReq->m_Container.GetStr()).GetStr(), CELL_SAVEDATA_ERRDIALOG_NONE, &SetBuf, cb_data_status_list_load, cb_file_operation_load, m_MemoryContainer))
							break;
						ResultError = CONTENT_OK;
					} while(0);
					s_SaveQueue.DestroyQueue();

					pReq->m_ResultError = ResultError;
					break;
				}

			case REQ_TYPE_ENUM_INSTANCES:
				{
					CellSaveDataSetBuf SetBuf;

					SetBuf.dirListMax = 10;
					SetBuf.fileListMax = 40;
					memset(SetBuf.reserved, 0, sizeof(SetBuf.reserved));
					SetBuf.bufSize = Max( 10 * sizeof(CellSaveDataDirList), 40 * sizeof(CellSaveDataFileStat) );
					TThinArray<uint8> lBuf; lBuf.SetLen(SetBuf.bufSize);
					SetBuf.buf = lBuf.GetBasePtr();

					int32 ResultError = CONTENT_ERR_GENERIC_READ;
					if(bSaveGameDebugging) M_TRACEALWAYS("Starting enumeration\n");
					s_SaveQueue.SetRequest(pReq);
					do 
					{
						if(bSaveGameDebugging) M_TRACEALWAYS("-- calling cellSaveDataAutoSave, TimeStamp  %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
						if(cellSaveDataAutoSave(CELL_SAVEDATA_VERSION_CURRENT, CFStrF("%s-%s", PRODUCTCODE, pReq->m_Container.GetStr()).GetStr(), CELL_SAVEDATA_ERRDIALOG_NONE, &SetBuf, cb_enum_instances_data_status_list_save, cb_file_operation_save, m_MemoryContainer))
							break;
						ResultError = CONTENT_OK;
					} while(0);
					s_SaveQueue.SetRequest(NULL);
					if(bSaveGameDebugging) M_TRACEALWAYS("Enumeration completed\n");

					pReq->m_ResultError = ResultError;
					break;
				}

			case REQ_TYPE_INSTANCE_QUERY_SIZE:
				{
					CellSaveDataSetBuf SetBuf;

					SetBuf.dirListMax = 10;
					SetBuf.fileListMax = 40;
					memset(SetBuf.reserved, 0, sizeof(SetBuf.reserved));
					SetBuf.bufSize = Max( 10 * sizeof(CellSaveDataDirList), 40 * sizeof(CellSaveDataFileStat) );
					TThinArray<uint8> lBuf; lBuf.SetLen(SetBuf.bufSize);
					SetBuf.buf = lBuf.GetBasePtr();

					int32 ResultError = CONTENT_ERR_GENERIC_READ;
					if(bSaveGameDebugging) M_TRACEALWAYS("Starting enumeration\n");
					s_SaveQueue.SetRequest(pReq);
					s_SaveQueue.CreateQueue(pReq->m_Container.GetStr(), CFStrF("%s-%s", PRODUCTCODE, pReq->m_Container.GetStr()), 1);
					do 
					{
						s_SaveQueue.InsertQueueItem(0, pReq->m_Instance.GetStr(), NULL, 0, CELL_SAVEDATA_FILETYPE_NORMALFILE);
						if(bSaveGameDebugging) M_TRACEALWAYS("-- calling cellSaveDataAutoSave, TimeStamp  %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
						if(cellSaveDataAutoSave(CELL_SAVEDATA_VERSION_CURRENT, CFStrF("%s-%s", PRODUCTCODE, pReq->m_Container.GetStr()).GetStr(), CELL_SAVEDATA_ERRDIALOG_NONE, &SetBuf, cb_query_size_data_status_list_save, cb_file_operation_save, m_MemoryContainer))
							break;
						ResultError = CONTENT_OK;
					} while(0);
					s_SaveQueue.DestroyQueue();
					s_SaveQueue.SetRequest(NULL);
					if(bSaveGameDebugging) M_TRACEALWAYS("Enumeration completed\n");

					pReq->m_ResultError = ResultError;
					break;
				}

			case REQ_TYPE_INSTANCE_DELETE:
				{
					CellSaveDataSetBuf SetBuf;

					SetBuf.dirListMax = 10;
					SetBuf.fileListMax = 40;
					memset(SetBuf.reserved, 0, sizeof(SetBuf.reserved));
					SetBuf.bufSize = Max( 10 * sizeof(CellSaveDataDirList), 40 * sizeof(CellSaveDataFileStat) );
					TThinArray<uint8> lBuf; lBuf.SetLen(SetBuf.bufSize);
					SetBuf.buf = lBuf.GetBasePtr();

					int32 ResultError = CONTENT_ERR_GENERIC_READ;
					if(bSaveGameDebugging) M_TRACEALWAYS("Starting enumeration\n");
					s_SaveQueue.SetRequest(pReq);
					s_SaveQueue.CreateQueue(pReq->m_Container.GetStr(), CFStrF("%s-%s", PRODUCTCODE, pReq->m_Container.GetStr()), 1);
					do 
					{
						s_SaveQueue.InsertQueueItem(0, pReq->m_Instance.GetStr(), NULL, 0, CELL_SAVEDATA_FILETYPE_NORMALFILE);
						if(bSaveGameDebugging) M_TRACEALWAYS("-- calling cellSaveDataAutoSave, TimeStamp  %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());
						if(cellSaveDataAutoSave(CELL_SAVEDATA_VERSION_CURRENT, CFStrF("%s-%s", PRODUCTCODE, pReq->m_Container.GetStr()).GetStr(), CELL_SAVEDATA_ERRDIALOG_NONE, &SetBuf, cb_delete_data_status_list_save, cb_file_operation_delete, m_MemoryContainer))
							break;
						ResultError = CONTENT_OK;
					} while(0);
					s_SaveQueue.DestroyQueue();
					s_SaveQueue.SetRequest(NULL);
					if(bSaveGameDebugging) M_TRACEALWAYS("Enumeration completed\n");

					pReq->m_ResultError = ResultError;
					break;
				}
			}
			if(bSaveGameDebugging) M_TRACEALWAYS("ReqFinished TimeStamp %.3f\n", (CMTime::GetCPU() - ReqStart).GetTime());

			// Make sure data is written before we signal that it is available
			M_EXPORTBARRIER;
			pReq->m_Done.Signal();

		} while(1);

	}

	return 0;
}

static void sysutil_savedata_callback( uint64_t status, uint64_t param, void * userdata )
{
	CSaveContentContextPS3* pSC = (CSaveContentContextPS3*)userdata;
	(void)param ;

	switch(status)
	{
	case CELL_SYSUTIL_REQUEST_EXITGAME:
		M_TRACEALWAYS("sysutil savedata receive CELL_SYSUTIL_REQUEST_EXITGAME \n");
		break;

	case CELL_SYSUTIL_DRAWING_BEGIN:
		M_TRACEALWAYS("sysutil savedata receive CELL_SYSUTIL_DRAWING_BEGIN \n");
		break;

	case CELL_SYSUTIL_DRAWING_END:
		M_TRACEALWAYS("sysutil savedata receive CELL_SYSUTIL_DRAWING_END \n");
		break;

	default:
		M_TRACEALWAYS("sysutil savedata receive unknown status: 0x%llx\n", status);
		break;
	}
}

void CSaveContentContextPS3::Create()
{
	m_lRequests.SetLen(16);
	if( sys_memory_container_create( &m_MemoryContainer, MEMORY_CONTAINER_SIZE_LARGEST) != 0 )
		M_BREAKPOINT;
	cellSysutilRegisterCallback(0, sysutil_savedata_callback, this);
	s_SaveQueue.Create();
	Thread_Create();
}

int CSaveContentContextPS3::EnumerateContainers(uint _iUser)
{
	return AllocRequest(REQ_TYPE_ENUM_CONTAINERS);
}
int CSaveContentContextPS3::ContainerCreate(uint _iUser, CStr _Container)
{
	return AllocRequest(REQ_TYPE_CONTAINER_CREATE, _Container);
}
int CSaveContentContextPS3::ContainerDelete(uint _iUser, CStr _Container)
{
	return AllocRequest(REQ_TYPE_CONTAINER_DELETE, _Container);
}

int CSaveContentContextPS3::ContainerMount(uint _iUser, CStr _Container)
{
	return AllocRequest(REQ_TYPE_CONTAINER_MOUNT, _Container);
}
int CSaveContentContextPS3::ContainerQuerySize(uint _iUser, CStr _Container)
{
	return AllocRequest(REQ_TYPE_CONTAINER_QUERY_SIZE, _Container);
}
int CSaveContentContextPS3::ContainerQueryPath(uint _iUser, CStr _Container)
{
	return AllocRequest(REQ_TYPE_CONTAINER_QUERY_PATH, _Container);
}

int CSaveContentContextPS3::EnumerateInstances(uint _iUser, CStr _Container)
{
	return AllocRequest(REQ_TYPE_ENUM_INSTANCES, _Container);
}
int CSaveContentContextPS3::InstanceQuerySize(uint _iUser, CStr _Container, CStr _Instance)
{
	return AllocRequest(REQ_TYPE_INSTANCE_QUERY_SIZE, _Container, _Instance);
}
int CSaveContentContextPS3::InstanceDelete(uint _iUser, CStr _Container, CStr _Instance)
{
	return AllocRequest(REQ_TYPE_INSTANCE_DELETE, _Container, _Instance);
}
int CSaveContentContextPS3::InstanceRead(uint _iUser, CStr _Container, CStr _Instance, void* _pDest, uint32 _MaxLen)
{
	return AllocRequest(REQ_TYPE_INSTANCE_READ, _Container, _Instance, _pDest, _MaxLen);
}
int CSaveContentContextPS3::InstanceWrite(uint _iUser, CStr _Container, CStr _Instance, void* _pSrc, uint32 _Len)
{
	return AllocRequest(REQ_TYPE_INSTANCE_WRITE, _Container, _Instance, _pSrc, _Len);
}

int CSaveContentContextPS3::GetRequestStatus(int _iRequest)
{
	if(_iRequest < 0 || _iRequest >= m_lRequests.Len())
		return CONTENT_ERR_INVALID_HANDLE;

	M_LOCK(m_RequestLock);
	CRequest* pReq = &m_lRequests[_iRequest];
	if(pReq->m_Type == REQ_TYPE_VOID)
		return CONTENT_ERR_INVALID_HANDLE;

	return pReq->m_ResultError;
}
int CSaveContentContextPS3::BlockOnRequest(int _iRequest)
{
	if(_iRequest < 0 || _iRequest >= m_lRequests.Len())
		return CONTENT_ERR_INVALID_HANDLE;

	do
	{
		CRequest* pReq = NULL;
		{
			M_LOCK(m_RequestLock);
			pReq = &m_lRequests[_iRequest];
			if(pReq->m_Type == REQ_TYPE_VOID)
				return CONTENT_ERR_INVALID_HANDLE;

			if(pReq->m_ResultError != CONTENT_ERR_PENDING)
				return pReq->m_ResultError;
		}

		// If we got here then the handle is proper, wait for it to be done
		pReq->m_Done.Wait();
	} while(1);

	return CONTENT_ERR_UNKNOWN;
}

int CSaveContentContextPS3::PeekRequestData(int _iRequest, CContentQueryData* _pRet)
{
	if(_iRequest < 0 || _iRequest >= m_lRequests.Len())
		return CONTENT_ERR_INVALID_HANDLE;

	M_LOCK(m_RequestLock);
	CRequest* pReq = &m_lRequests[_iRequest];
	if(pReq->m_Type == REQ_TYPE_VOID)
		return CONTENT_ERR_INVALID_HANDLE;

	int ResultError = pReq->m_ResultError;
	if(_pRet)
	{
		_pRet->m_Data = pReq->m_ResultSize;
	}

	return ResultError;
}
int CSaveContentContextPS3::GetRequestData(int _iRequest, CContentQueryData* _pRet)
{
	if(_iRequest < 0 || _iRequest >= m_lRequests.Len())
		return CONTENT_ERR_INVALID_HANDLE;

	M_LOCK(m_RequestLock);
	CRequest* pReq = &m_lRequests[_iRequest];
	if(pReq->m_Type == REQ_TYPE_VOID)
		return CONTENT_ERR_INVALID_HANDLE;

	int ResultError = pReq->m_ResultError;
	if(_pRet)
	{
		_pRet->m_Data = pReq->m_ResultSize;
	}

	FreeRequest(_iRequest);

	return ResultError;
}
int CSaveContentContextPS3::GetRequestEnumeration(int _iRequest, TArray<CStr>& _lData)
{
	if(_iRequest < 0 || _iRequest >= m_lRequests.Len())
		return CONTENT_ERR_INVALID_HANDLE;

	M_LOCK(m_RequestLock);
	CRequest* pReq = &m_lRequests[_iRequest];
	if(pReq->m_Type == REQ_TYPE_VOID)
		return CONTENT_ERR_INVALID_HANDLE;

	if(pReq->m_Type != REQ_TYPE_ENUM_CONTAINERS && pReq->m_Type != REQ_TYPE_ENUM_INSTANCES)
		return CONTENT_ERR_UNSUPPORTED;

	if(pReq->m_ResultError != CONTENT_OK)
		return pReq->m_ResultError;

	_lData = pReq->m_lResultEnum;

	FreeRequest(_iRequest);

	return CONTENT_OK;
}


#endif
