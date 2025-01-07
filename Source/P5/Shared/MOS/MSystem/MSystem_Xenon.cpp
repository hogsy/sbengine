
#include "PCH.h"

#ifdef PLATFORM_XENON

#include "MSystem_Core.h"
#include "MContentContext.h"

#include <xtl.h>
#include <xbox.h>

// -------------------------------------------------------------------
//  CSaveContentContext_Xenon
// -------------------------------------------------------------------

class CSaveContentContext_Xenon : public CContentContext, MRTC_Thread
{
protected:

	enum
	{
		REQ_TYPE_VOID = 0,

		REQ_TYPE_STORAGE_SELECT,

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
			m_iUser = 0;
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

		uint32 m_iUser;
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

	XCONTENTDEVICEID m_DeviceID;
	XOVERLAPPED m_IODeviceSelect;
	
	int AllocRequestInstance();
	void QueueRequest(CRequest* _pReq);

	int AllocRequest(uint _iUser, int _Type);
	int AllocRequest(uint _iUser, int _Type, CStr _Container);
	int AllocRequest(uint _iUser, int _Type, CStr _Container, CStr _Instance);
	int AllocRequest(uint _iUser, int _Type, CStr _Container, CStr _Instance, void* _pData, uint _DataLen);
	void FreeRequest(int _iRequest);

	const char* Thread_GetName() {return "CSaveContentContext_Xenon";}
public:
	CSaveContentContext_Xenon() : m_pRequestHead(NULL), m_pRequestTail(NULL), m_DeviceID(XCONTENTDEVICE_ANY) {}
	void Create()
	{
		m_DeviceID = 1;
		m_lRequests.SetLen(16);
		Thread_Create();
	}
	void Destroy()
	{
		Thread_Destroy();
	}

	int Thread_Main();

	int StorageSelect(uint _iUser, uint _SaveSize);

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

// -------------------------------------------------------------------
//  CSystemXenon
// -------------------------------------------------------------------

class SYSTEMDLLEXPORT CSystemXenon : public CSystemCore
{
	MRTC_DECLARE;

protected:
	CSaveContentContext_Xenon m_SaveGameContext;
public:

	DECLARE_OPERATOR_NEW


	CSystemXenon();
	virtual ~CSystemXenon();
	virtual void Create(void* this_inst, void* prev_inst, char* _cmdline, 
		 int _cmdshow, const char* _pAppClassName);
	virtual void Destroy();
	virtual void CreateSystems();
	virtual CStr GetModuleName(CStr _Name, bool _bAppendSystem = false);

	virtual void Render(CDisplayContext* _pDC = NULL, int _Context = 0);
	virtual void Refresh();

	virtual int ReadOptions();				// 0 == Corrupt, 1 == Ok, 2 == NoFile
	virtual int WriteOptions();			// 0 == Failed, 1 == Ok, 2 == Corrupt

	virtual void DC_InitList();

	virtual CStr GetUserFolder();
	virtual CStr TranslateSavePath(CStr _Path);

	virtual class CContentContext* GetContentContext(int _iContent);

	virtual void Parser_SysShowMemory();
	virtual void Parser_MemoryDebug(int _Flags);
	virtual void Parser_TestMemory();
	virtual void ExitProcess();
};

// -------------------------------------------------------------------
//  CSystemXenon
// -------------------------------------------------------------------

// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CSystemXenon, CSystemCore);


IMPLEMENT_OPERATOR_NEW(CSystemXenon);



CSystemXenon::CSystemXenon()
{
}

CSystemXenon::~CSystemXenon()
{
//	Destroy();
}

CStr CSystemXenon::GetModuleName(CStr _Name, bool _bSystemPath)
{
	if (_bSystemPath && m_DefaultSystem.Len())
		return _Name + "_" + m_DefaultSystem + ".dll";
	else
		return _Name + ".dll";

}

void CSystemXenon::Create(void* this_inst, void* prev_inst, char* _cmdline, 
	 int _cmdshow, const char* _pAppClassName)
{
	MSCOPE(CSystemXenon::Create, SYSTEMXENON);

//	LogFile("(CSystemXenon::Create)");

/*	for(int i = 0; i < 20; i++)
		Sleep(1000);*/

	// Change directory to that of the executable.
	CStr CommandLine; 

	m_ExePath = "D:\\";
	unsigned long iLaunchDataType = 0;

	LogFile("(CSystemXenon::Create) Path is set to: " + m_ExePath);
/*
#ifndef	M_RTM
#ifndef PLATFORM_XENON
XNetStartupParams xnsp;
	memset( &xnsp, 0, sizeof( xnsp ) );
	xnsp.cfgSizeOfStruct = sizeof( XNetStartupParams );
	xnsp.cfgFlags = XNET_STARTUP_BYPASS_SECURITY;
	INT err = XNetStartup( &xnsp );
#endif	// M_RTM
#endif
*/

	CSystemCore::Create(_cmdline, _pAppClassName);

//	LogFile("(CSystemXenon::Create) Done");
}

CStr CSystemXenon::GetUserFolder()
{
	return "CACHE:\\";
}

CStr CSystemXenon::TranslateSavePath(CStr _Path) 
{
	MACRO_GetSystem;
	CStr SavePath = pSys->GetRegistry()->GetValue("SAVEPATH");
	int ExePathLen = m_ExePath.Len();
	if (SavePath != "" && m_ExePath.CompareNoCase(_Path.Left(ExePathLen)) == 0)
	{
		return SavePath + _Path.RightFrom(ExePathLen);
	}
	return _Path;
}

void CSystemXenon::Destroy()
{
	MSCOPE(CSystemXenon::Destroy, SYSTEMXENON);
	
	LogFile("(CSystemXenon::Destroy)");
	M_TRY
	{ 
		DC_Set(-1); 
	} 
	M_CATCH(
	catch (CCException) 
	{
	};
	)
	CSystemCore::Destroy();
	LogFile("(CSystemXenon::Destroy) Done");
}

void CSystemXenon::CreateSystems()
{
	MSCOPE(CSystemXenon::CreateSystems, SYSTEMXENON);
	
	CSystemCore::CreateSystems();
}

void CSystemXenon::Render(CDisplayContext* _pDC, int _Context)
{
	MSCOPE(CSystemXenon::Render, SYSTEMXENON);
	
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
				// FIXME!!! The CSS_MSG_WIN32_CREATEDWINDOW message should take care of this, but
				// m_ioMainWindow isn't set when it is called...
			}
		}
//		ConOutL(CStrF("hWnd %d", m_spWinInfo->m_hMainWnd));
	}
	if (!_pDC) return;
//	if (m_iMainWindow <= 0) return;
//	_pDC->SelectWindow(m_iMainWindow);
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
			M_TRY
			{ 
				m_spApp->OnRender(_pDC, spImg, Clip, _Context);
			}
			M_CATCH(
			catch(CCExceptionGraphicsHAL) 
			{
				Error("Render", "Render failed. (1)"); 
			}
			catch(CCException)
			{ 
				throw;
//				Error("Render", "Render failed. (2)"); 
			};
			)
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
};

void CSystemXenon::Refresh()
{
	MSCOPE(CSystemXenon::Refresh, SYSTEMXENON);
	
	CSystemCore::Refresh();

	if (m_RefreshRecursion) return;
	m_RefreshRecursion++;
	M_TRY
	{
		if (m_spDisplay != NULL)
		{
/*			if (m_iMainWindow >= 0) 
				m_spWinInfo->m_hMainWnd = m_spDisplay->Win32_GethWnd(m_iMainWindow);
			else
				m_spWinInfo->m_hMainWnd = NULL;*/
		}
	}
	M_CATCH(
	catch(CCException)
	{
		m_RefreshRecursion--;
		throw;
	}
	)
	m_RefreshRecursion--;
}

int CSystemXenon::ReadOptions()
{
	// 0 == Corrupt, 1 == Ok, 2 == NoFile

	// Option registry added to the environment
	if (CDiskUtil::FileExists(m_OptionsFilename))
	{
		M_TRY
		{
			bool bCorruptFile = false;
			if(!bCorruptFile)
			{	
				m_spRegistry->ReadRegistryDir("OPT", m_OptionsFilename);
				
				spCRegistry spEnv2 = m_spRegistry->Find("OPT");
				if (!spEnv2) Error("CreateSystems", "Internal error. (1)");
			}
			else
				return 0;
		}
		M_CATCH(
		catch(CCException)
		{
			return 0;
		}
		)
	}
	else
	{
		ConOutL("(CSystemCore::CreateSystems) " + m_OptionsFilename + " doesn't exist.");
		return 2;
	}

	return 1;
}

int CSystemXenon::WriteOptions()
{
	// // 0 == Failed, 1 == Ok, 2 == Corrupt

	M_TRY
	{
		//spCRegistry spEnv = m_spRegistry->Find("ENV");
		int ClusterSize = 16384;
		{
			CCFile File;
			File.OpenExt(m_OptionsFilename, CFILE_WRITE | CFILE_UNICODE, NO_COMPRESSION, NORMAL_COMPRESSION, 0, -1, ClusterSize);
			m_spRegistry->WriteRegistryDir("OPT", &File);
			File.Close();
		}
	}
	M_CATCH(
	catch(...)	
	{
		LogFile("(CSystemCore::SaveOptions) Error writing options " + m_OptionsFilename);
		CDiskUtil::DelFile(m_OptionsFilename);
		return 0;	
	}
	)
	return 1;
}

/*
class CDisplayContextXDF : public CDisplayContext
{
protected:
	MRTC_DECLARE;
public:

	spCRenderContext m_spRenderContext;
	CImage m_Image;

	CDisplayContextXDF()
	{
		m_Image.Create(640, 480, IMAGE_FORMAT_BGRA8, IMAGE_MEM_IMAGE);
	}

	~CDisplayContextXDF()
	{
	}

	virtual CPnt GetScreenSize(){return CPnt(640, 480);};

	virtual void Create()
	{
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

	virtual CRenderContext* GetRenderContext()
	{
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

MRTC_IMPLEMENT_DYNAMIC(CDisplayContextXDF, CDisplayContext);
*/

#include "Raster/MRCCore.h"

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

	virtual CPnt GetScreenSize() { return CPnt(640, 480); };
	virtual CPnt GetMaxWindowSize() { return CPnt(640, 480); };

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

void CSystemXenon::DC_InitList()
{
	static int Test = (mint)&g_ClassRegCDisplayContextNULL;
	m_lspDC.Add(MNew4(CDisplayContextDesc, "", "Xenon", "Xenon DisplayContext", "CDisplayContextXenon") );
};

void CSystemXenon::Parser_SysShowMemory()
{
	ConOut("(CSystemXenon::Parser_SysShowMemory) Not implemented.");

/*	DWORD ProcessID = GetCurrentProcessId();

	Win32_EnumProcs( Win32_GetProcessMemoryInfo, 0);
*/	

/*	HANDLE hProcess = OpenProcess(PROCESS_ALL_ACCESS, FALSE, ProcessID);
	if (hProcess)
	{

		PROCESS_MEMORY_COUNTERS MemCount;
		FillChar(&MemCount, sizeof(MemCount), 0);

		GetProcessMemoryInfo(hProcess, &MemCount, sizeof(MemCount));

		ConOutL("-------------------------------------------------------------------");
		ConOutL("Process memory counters:");
		ConOutL("-------------------------------------------------------------------");
		ConOutL(CStrF("PageFaults               %d", MemCount.PageFaultCount));
		ConOutL(CStrF("WorkingSetSize           %d, peak %d", MemCount.WorkingSetSize, MemCount.PeakWorkingSetSize));
		ConOutL(CStrF("PageFileUsage            %d, peak %d", MemCount.PagefileUsage, MemCount.PeakPagefileUsage));
		ConOutL(CStrF("QuotaPagedPoolUsage      %d, peak %d", MemCount.QuotaPagedPoolUsage, MemCount.PeakPagefileUsage));
		ConOutL(CStrF("QuotaNonPagedPoolUsage   %d, peak %d", MemCount.QuotaNonPagedPoolUsage, MemCount.PeakQuotaNonPagedPoolUsage));
		ConOutL("-------------------------------------------------------------------");
		CloseHandle(hProcess);
	}
	else
		ConOutL("(CSystemXenon::Parser_SysShowMemory) Invalid process handle.");*/
}

void CSystemXenon::Parser_MemoryDebug(int _Flags)
{
#ifdef MRTC_MEMORYDEBUG
	int Flags = 0;
	if (_Flags & 1) Flags |= _CRTDBG_ALLOC_MEM_DF;
	if (_Flags & 2) Flags |= _CRTDBG_DELAY_FREE_MEM_DF;
	if (_Flags & 4) Flags |= _CRTDBG_LEAK_CHECK_DF;
	_CrtSetDbgFlag(Flags);
#else
	ConOut("This executable is not compiled for memory-debuging.");
#endif
}

void CSystemXenon::Parser_TestMemory()
{
#ifdef MRTC_MEMORYDEBUG
	if (!_CrtCheckMemory())
		ConOutL("(CSystemXenon::Parser_TestMemory) Memory check failure.");
	else
		ConOutL("(CSystemXenon::Parser_TestMemory) Memory check ok.");
#else
	ConOut("This executable is not compiled for memory-debuging.");
#endif
}

void CSystemXenon::ExitProcess()
{
	m_bBreakRequested = true;

}

CContentContext* CSystemXenon::GetContentContext(int _iContext)
{
	switch(_iContext)
	{
	case CONTENT_SAVEGAMES:
		return &m_SaveGameContext;
	}

	return NULL;
}

//#pragma optimize("", off)
//#pragma inline_depth(0)

int CSaveContentContext_Xenon::StorageSelect(uint _iUser, uint _SaveSize)
{
//	return AllocRequest(REQ_TYPE_STORAGE_SELECT);
	ULARGE_INTEGER RequiredSpace;
	RequiredSpace.QuadPart = XContentCalculateSize(_SaveSize, 1);
	memset(&m_IODeviceSelect, 0, sizeof(m_IODeviceSelect));
	m_IODeviceSelect.hEvent = CreateEvent(NULL, false, 0, NULL);
	XShowDeviceSelectorUI(_iUser, XCONTENTTYPE_SAVEDGAME, XCONTENTFLAG_NONE, RequiredSpace, &m_DeviceID, &m_IODeviceSelect);

	return 0;
}

int CSaveContentContext_Xenon::EnumerateContainers(uint _iUser)
{
	return AllocRequest(_iUser, REQ_TYPE_ENUM_CONTAINERS);
}

int CSaveContentContext_Xenon::ContainerCreate(uint _iUser, CStr _Container)
{
	return AllocRequest(_iUser, REQ_TYPE_CONTAINER_CREATE, _Container);
}

int CSaveContentContext_Xenon::ContainerDelete(uint _iUser, CStr _Container)
{
	return AllocRequest(_iUser, REQ_TYPE_CONTAINER_DELETE, _Container);
}

int CSaveContentContext_Xenon::ContainerQuerySize(uint _iUser, CStr _Container)
{
	return AllocRequest(_iUser, REQ_TYPE_CONTAINER_QUERY_SIZE, _Container);
}

int CSaveContentContext_Xenon::ContainerMount(uint _iUser, CStr _Container)
{
	return AllocRequest(_iUser, REQ_TYPE_CONTAINER_MOUNT, _Container);
}

int CSaveContentContext_Xenon::ContainerQueryPath(uint _iUser, CStr _Container)
{
	return AllocRequest(_iUser, REQ_TYPE_CONTAINER_QUERY_PATH, _Container);
}

int CSaveContentContext_Xenon::EnumerateInstances(uint _iUser, CStr _Container)
{
	return AllocRequest(_iUser, REQ_TYPE_ENUM_INSTANCES, _Container);
}

int CSaveContentContext_Xenon::InstanceRead(uint _iUser, CStr _Container, CStr _Instance, void* _pDest, uint32 _MaxLen)
{
	return AllocRequest(_iUser, REQ_TYPE_INSTANCE_READ, _Container, _Instance, _pDest, _MaxLen);
}

int CSaveContentContext_Xenon::InstanceWrite(uint _iUser, CStr _Container, CStr _Instance, void* _pSrc, uint32 _Len)
{
	return AllocRequest(_iUser, REQ_TYPE_INSTANCE_WRITE, _Container, _Instance, _pSrc, _Len);
}

int CSaveContentContext_Xenon::InstanceQuerySize(uint _iUser, CStr _Container, CStr _Instance)
{
	return AllocRequest(_iUser, REQ_TYPE_INSTANCE_QUERY_SIZE, _Container, _Instance);
}

int CSaveContentContext_Xenon::InstanceDelete(uint _iUser, CStr _Container, CStr _Instance)
{
	return AllocRequest(_iUser, REQ_TYPE_INSTANCE_DELETE, _Container, _Instance);
}


int CSaveContentContext_Xenon::PeekRequestData(int _iRequest, CContentQueryData* _pRet)
{
	if(_iRequest < 0 || _iRequest >= m_lRequests.Len())
		return CONTENT_ERR_INVALID_HANDLE;

	M_LOCK(m_RequestLock);
	CRequest* pReq = &m_lRequests[_iRequest];
	if(pReq->m_Type == REQ_TYPE_VOID)
		return CONTENT_ERR_INVALID_HANDLE;

	if(_pRet)
		_pRet->m_Data = pReq->m_ResultSize;

	return pReq->m_ResultError;
}

int CSaveContentContext_Xenon::GetRequestData(int _iRequest, CContentQueryData* _pRet)
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

int CSaveContentContext_Xenon::GetRequestEnumeration(int _iRequest, TArray<CStr>& _lData)
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

int CSaveContentContext_Xenon::GetRequestStatus(int _iRequest)
{
	if(_iRequest < 0 || _iRequest >= m_lRequests.Len())
		return CONTENT_ERR_INVALID_HANDLE;

	M_LOCK(m_RequestLock);
	CRequest* pReq = &m_lRequests[_iRequest];
	if(pReq->m_Type == REQ_TYPE_VOID)
		return CONTENT_ERR_INVALID_HANDLE;

	return pReq->m_ResultError;
}

int CSaveContentContext_Xenon::BlockOnRequest(int _iRequest)
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

int CSaveContentContext_Xenon::Thread_Main()
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

			switch(pReq->m_Type)
			{
			default:
				pReq->m_ResultError = CONTENT_ERR_UNSUPPORTED;
				break;

			case REQ_TYPE_STORAGE_SELECT:
				{
					ULARGE_INTEGER RequiredSpace;
					RequiredSpace.QuadPart = XContentCalculateSize(1024 * 1024, 16);
					memset(&m_IODeviceSelect, 0, sizeof(m_IODeviceSelect));
					m_IODeviceSelect.hEvent = CreateEvent(NULL, false, 0, NULL);
					XShowDeviceSelectorUI(pReq->m_iUser, XCONTENTTYPE_SAVEDGAME, XCONTENTFLAG_NONE, RequiredSpace, &m_DeviceID, &m_IODeviceSelect);
					break;
				}

			case REQ_TYPE_ENUM_CONTAINERS:
				{
					HANDLE hEnum = 0;
					int ResultError = CONTENT_ERR_GENERIC_WRITE;
					do
					{
						if(XContentCreateEnumerator(pReq->m_iUser, m_DeviceID, XCONTENTTYPE_SAVEDGAME, XCONTENTFLAG_ENUM_EXCLUDECOMMON, 1, NULL, &hEnum) != ERROR_SUCCESS)
							break;

						XCONTENT_DATA XContent;
						DWORD nItems = 0;
						char aBuf[43];
						aBuf[42] = 0;
						while(XEnumerate(hEnum, &XContent, sizeof(XContent), &nItems, NULL) == ERROR_SUCCESS)
						{
							memcpy(aBuf, XContent.szFileName, 42);
							pReq->m_lResultEnum.Add(CStrF("%d:%s", XContent.DeviceID, aBuf));
						}

						CloseHandle(hEnum);
						ResultError = CONTENT_OK;
					} while(0);

					pReq->m_ResultError = ResultError;
					break;
				}

			case REQ_TYPE_CONTAINER_CREATE:
				{
					int ResultError = CONTENT_ERR_GENERIC_WRITE;
					do
					{
						XCONTENTDEVICEID DeviceID = m_DeviceID;
						CFStr Container = pReq->m_Container;
						int Sep = Container.Find(":");
						if(Sep >= 0)
						{
							DeviceID = pReq->m_Container.LeftTo(Sep).Val_int();
							Container = pReq->m_Container.RightFrom(Sep + 1);
						}

						XCONTENT_DATA XContent = {0};
						XContent.DeviceID = DeviceID;
						XContent.dwContentType = XCONTENTTYPE_SAVEDGAME;
						CStr DisplayName = pReq->m_Container.Unicode().LeftTo(127);

						memcpy(XContent.szDisplayName, DisplayName.GetStrW(), (DisplayName.Len() + 1) * 2);
						strncpy(XContent.szFileName, Container, 42);

						DWORD Err = XContentCreate(pReq->m_iUser, "savedrive", &XContent, XCONTENTFLAG_CREATENEW, NULL, NULL, NULL);
						if(Err != ERROR_SUCCESS)
							break;

						XContentClose("savedrive", NULL);
						ResultError = CONTENT_OK;
					} while(0);
					pReq->m_ResultError = ResultError;
					break;
				}

			case REQ_TYPE_CONTAINER_DELETE:
				{
					int ResultError = CONTENT_ERR_GENERIC_WRITE;
					do
					{
						XCONTENTDEVICEID DeviceID = m_DeviceID;
						CFStr Container = pReq->m_Container;
						int Sep = Container.Find(":");
						if(Sep >= 0)
						{
							DeviceID = pReq->m_Container.LeftTo(Sep).Val_int();
							Container = pReq->m_Container.RightFrom(Sep + 1);
						}
						XCONTENT_DATA XContent = {0};
						XContent.DeviceID = DeviceID;
						XContent.dwContentType = XCONTENTTYPE_SAVEDGAME;
//						CStr DisplayName = pReq->m_Container.Unicode().LeftTo(127);

//						memcpy(XContent.szDisplayName, DisplayName.GetStrW(), (DisplayName.Len() + 1) * 2);
						strncpy(XContent.szFileName, Container, 42);


						DWORD Err = XContentDelete(pReq->m_iUser, &XContent, NULL);
						if(Err != ERROR_SUCCESS)
							break;

						ResultError = CONTENT_OK;
					} while(0);
					pReq->m_ResultError = ResultError;
					break;
				}

			case REQ_TYPE_ENUM_INSTANCES:
				{
					int ResultError = CONTENT_ERR_GENERIC_WRITE;
					do
					{
						XCONTENTDEVICEID DeviceID = m_DeviceID;
						CFStr Container = pReq->m_Container;
						int Sep = Container.Find(":");
						if(Sep >= 0)
						{
							DeviceID = pReq->m_Container.LeftTo(Sep).Val_int();
							Container = pReq->m_Container.RightFrom(Sep + 1);
						}
						XCONTENT_DATA XContent = {0};
						XContent.DeviceID = DeviceID;
						XContent.dwContentType = XCONTENTTYPE_SAVEDGAME;
						CStr DisplayName = pReq->m_Container.Unicode().LeftTo(127);

						memcpy(XContent.szDisplayName, DisplayName.GetStrW(), (DisplayName.Len() + 1) * 2);
						strncpy(XContent.szFileName, Container, 42);

						DWORD Err = XContentCreate(pReq->m_iUser, "savedrive", &XContent, XCONTENTFLAG_OPENEXISTING, NULL, NULL, NULL);
						if(Err != ERROR_SUCCESS)
							break;

						{
							CDirectoryNode Dir;
							Dir.ReadDirectory(CStrF("%s:\\*", "savedrive"));
							int nf = Dir.GetFileCount();
							for(int i = 0; i < nf; i++)
							{
								CDir_FileRec* pF = Dir.GetFileRec(i);
								if(!Dir.IsDirectory(i))
								{
									pReq->m_lResultEnum.Add(pF->m_Name);
								}
							}
						}

						XContentClose("savedrive", NULL);
						ResultError = CONTENT_OK;
					} while(0);
					pReq->m_ResultError = ResultError;
					break;
				}

			case REQ_TYPE_INSTANCE_WRITE:
				{
					int ResultError = CONTENT_ERR_GENERIC_WRITE;
					do
					{
						XCONTENTDEVICEID DeviceID = m_DeviceID;
						CFStr Container = pReq->m_Container;
						int Sep = Container.Find(":");
						if(Sep >= 0)
						{
							DeviceID = pReq->m_Container.LeftTo(Sep).Val_int();
							Container = pReq->m_Container.RightFrom(Sep + 1);
						}
						XCONTENT_DATA XContent = {0};
						XContent.DeviceID = DeviceID;
						XContent.dwContentType = XCONTENTTYPE_SAVEDGAME;
						CStr DisplayName = pReq->m_Container.Unicode().LeftTo(127);

						memcpy(XContent.szDisplayName, DisplayName.GetStrW(), (DisplayName.Len() + 1) * 2);
						strncpy(XContent.szFileName, Container, 42);

						DWORD Err = XContentCreate(pReq->m_iUser, "savedrive", &XContent, XCONTENTFLAG_OPENALWAYS, NULL, NULL, NULL);
						if(Err != ERROR_SUCCESS)
							break;

						{
							CCFile File;
							File.Open(CStrF("%s:\\%s", "savedrive", pReq->m_Instance.GetStr()), CFILE_WRITE | CFILE_BINARY | CFILE_TRUNC | CFILE_NODEFERCLOSE);
							File.Write(pReq->m_pDestArg, pReq->m_LenDestArg);
							File.Close();
						}

						XContentClose("savedrive", NULL);
						ResultError = CONTENT_OK;
					} while(0);
					pReq->m_ResultError = ResultError;
					break;
				}

			case REQ_TYPE_INSTANCE_READ:
				{
					int ResultError = CONTENT_ERR_GENERIC_WRITE;
					do
					{
						XCONTENTDEVICEID DeviceID = m_DeviceID;
						CFStr Container = pReq->m_Container;
						int Sep = Container.Find(":");
						if(Sep >= 0)
						{
							DeviceID = pReq->m_Container.LeftTo(Sep).Val_int();
							Container = pReq->m_Container.RightFrom(Sep + 1);
						}
						XCONTENT_DATA XContent = {0};
						XContent.DeviceID = DeviceID;
						XContent.dwContentType = XCONTENTTYPE_SAVEDGAME;
						CStr DisplayName = pReq->m_Container.Unicode().LeftTo(127);

						memcpy(XContent.szDisplayName, DisplayName.GetStrW(), (DisplayName.Len() + 1) * 2);
						strncpy(XContent.szFileName, Container, 42);

						DWORD Err = XContentCreate(pReq->m_iUser, "savedrive", &XContent, XCONTENTFLAG_OPENEXISTING, NULL, NULL, NULL);
						if(Err != ERROR_SUCCESS)
							break;

						do 
						{
							CStr FileName = CStrF("%s:\\%s", "savedrive", pReq->m_Instance.GetStr());
							if(!CDiskUtil::FileExists(FileName))
								break;

							{
								CCFile File;
								File.Open(FileName, CFILE_READ | CFILE_BINARY | CFILE_NODEFERCLOSE);
								int ReadData = Min(pReq->m_LenDestArg, (uint32)File.Length());
								File.Read(pReq->m_pDestArg, ReadData);
								File.Close();
								pReq->m_ResultSize = ReadData;
							}
						} while(0);

						XContentClose("savedrive", NULL);
						ResultError = CONTENT_OK;
					} while(0);
					pReq->m_ResultError = ResultError;
					break;
				}

			case REQ_TYPE_INSTANCE_QUERY_SIZE:
				{
					int ResultError = CONTENT_ERR_GENERIC_WRITE;
					do
					{
						XCONTENTDEVICEID DeviceID = m_DeviceID;
						CFStr Container = pReq->m_Container;
						int Sep = Container.Find(":");
						if(Sep >= 0)
						{
							DeviceID = pReq->m_Container.LeftTo(Sep).Val_int();
							Container = pReq->m_Container.RightFrom(Sep + 1);
						}
						XCONTENT_DATA XContent = {0};
						XContent.DeviceID = DeviceID;
						XContent.dwContentType = XCONTENTTYPE_SAVEDGAME;
						CStr DisplayName = pReq->m_Container.Unicode().LeftTo(127);

						memcpy(XContent.szDisplayName, DisplayName.GetStrW(), (DisplayName.Len() + 1) * 2);
						strncpy(XContent.szFileName, Container, 42);

						DWORD Err = XContentCreate(pReq->m_iUser, "savedrive", &XContent, XCONTENTFLAG_OPENEXISTING, NULL, NULL, NULL);
						if(Err != ERROR_SUCCESS)
							break;

						do 
						{
							CStr FileName = CStrF("%s:\\%s", "savedrive", pReq->m_Instance.GetStr());
							if(!CDiskUtil::FileExists(FileName))
								break;

							CCFile File;
							File.Open(FileName, CFILE_READ | CFILE_BINARY | CFILE_NODEFERCLOSE);
							pReq->m_ResultSize = File.Length();
						} while(0);

						XContentClose("savedrive", NULL);
						ResultError = CONTENT_OK;
					} while(0);
					pReq->m_ResultError = ResultError;
					break;
				}
			case REQ_TYPE_INSTANCE_DELETE:
				{
					int ResultError = CONTENT_ERR_GENERIC_WRITE;
					do
					{
						XCONTENTDEVICEID DeviceID = m_DeviceID;
						CFStr Container = pReq->m_Container;
						int Sep = Container.Find(":");
						if(Sep >= 0)
						{
							DeviceID = pReq->m_Container.LeftTo(Sep).Val_int();
							Container = pReq->m_Container.RightFrom(Sep + 1);
						}
						XCONTENT_DATA XContent = {0};
						XContent.DeviceID = DeviceID;
						XContent.dwContentType = XCONTENTTYPE_SAVEDGAME;
						CStr DisplayName = pReq->m_Container.Unicode().LeftTo(127);

						memcpy(XContent.szDisplayName, DisplayName.GetStrW(), (DisplayName.Len() + 1) * 2);
						strncpy(XContent.szFileName, Container, 42);

						DWORD Err = XContentCreate(pReq->m_iUser, "savedrive", &XContent, XCONTENTFLAG_OPENEXISTING, NULL, NULL, NULL);
						if(Err != ERROR_SUCCESS)
							break;

						do 
						{
							CStr FileName = CStrF("%s:\\%s", "savedrive", pReq->m_Instance.GetStr());
							if(!CDiskUtil::FileExists(FileName))
								break;

							if(!CDiskUtil::DelFile(FileName))
								break;
						} while(0);

						XContentClose("savedrive", NULL);
						ResultError = CONTENT_OK;
					} while(0);
					pReq->m_ResultError = ResultError;
					break;
				}
			}

			// Make sure data is written before we signal that it is available
			M_EXPORTBARRIER;
			pReq->m_Done.Signal();

		} while(1);

	}

	return 0;
}

void CSaveContentContext_Xenon::FreeRequest(int _iRequest)
{
	M_LOCK(m_RequestLock);
	m_lRequests[_iRequest].m_Type = REQ_TYPE_VOID;
	m_lRequests[_iRequest].Reset();
}

int CSaveContentContext_Xenon::AllocRequestInstance()
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

void CSaveContentContext_Xenon::QueueRequest(CRequest* _pReq)
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

int CSaveContentContext_Xenon::AllocRequest(uint _iUser, int _Type)
{
	int Request = AllocRequestInstance();
	if(Request >= 0)
	{
		CRequest* pReq = &m_lRequests[Request];
		pReq->m_iUser = _iUser;
		pReq->m_Type = _Type;
		QueueRequest(pReq);
	}

	return Request;
}

int CSaveContentContext_Xenon::AllocRequest(uint _iUser, int _Type, CStr _Container)
{
	int Request = AllocRequestInstance();
	if(Request >= 0)
	{
		CRequest* pReq = &m_lRequests[Request];
		pReq->m_iUser = _iUser;
		pReq->m_Type = _Type;
		pReq->m_Container = _Container;
		QueueRequest(pReq);
	}

	return Request;
}

int CSaveContentContext_Xenon::AllocRequest(uint _iUser, int _Type, CStr _Container, CStr _Instance)
{
	int Request = AllocRequestInstance();
	if(Request >= 0)
	{
		CRequest* pReq = &m_lRequests[Request];
		pReq->m_iUser = _iUser;
		pReq->m_Type = _Type;
		pReq->m_Container = _Container;
		pReq->m_Instance = _Instance;
		QueueRequest(pReq);
	}

	return Request;
}

int CSaveContentContext_Xenon::AllocRequest(uint _iUser, int _Type, CStr _Container, CStr _Instance, void* _pDest, uint _DestLen)
{
	int Request = AllocRequestInstance();
	if(Request >= 0)
	{
		CRequest* pReq = &m_lRequests[Request];
		pReq->m_iUser = _iUser;
		pReq->m_Type = _Type;
		pReq->m_Container = _Container;
		pReq->m_Instance = _Instance;
		pReq->m_pDestArg = _pDest;
		pReq->m_LenDestArg = _DestLen;
		QueueRequest(pReq);
	}

	return Request;
}

#endif
