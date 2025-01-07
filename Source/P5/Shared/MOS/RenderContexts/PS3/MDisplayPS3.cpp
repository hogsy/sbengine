#include "PCH.h"

#include "MDisplayPS3.h"
#include "../../MOS.h"

CDisplayContextPS3 CDisplayContextPS3::ms_This;

#ifdef GL_FUNCTIMING

DIdsListLinkDA_List(CGLFunctionTimer, m_Link) g_GlobalTimers = {0, 0};
CGLGlobalTiming g_GlobalTiming;
CGLFunctionTimer::CGLFunctionTimer(const char *_pName)
{
	m_pName = _pName;
	if (!g_GlobalTimers.m_Link.m_pNextPtr)
		g_GlobalTimers.Construct();

	g_GlobalTimers.Insert(this);
}
void CGLGlobalTiming::Reset()
{
	if (m_bPendingTrace < 0)
	{
		if (!(++m_bPendingTrace))
		{
			TraceTimes();
		}
	}

	m_GlobalTimer.Reset();
	m_GlobalCalls = 0;

	DIdsListLinkD_Iter(CGLFunctionTimer, m_Link) Iter = g_GlobalTimers;

	while (Iter)
	{
		Iter->m_Calls = 0;
		Iter->m_Timer.Reset();
		++Iter;
	}
}
void CGLGlobalTiming::TraceTimes()
{
	g_GlobalTimers.MergeSort<CSortTrace>();
	DIdsListLinkD_Iter(CGLFunctionTimer, m_Link) Iter = g_GlobalTimers;

	M_TRACEALWAYS("GlobalTimer %f ms, %d calls\n", m_GlobalTimer.Scale(1000.0f).GetTime(), m_GlobalCalls);

	while (Iter)
	{
		CGLFunctionTimer *pTimer = Iter;
		if (pTimer->m_Calls)
		{
			if (pTimer->m_Timer.GetTime() < 0.001f)
			{
				M_TRACEALWAYS("%5.1f micro s, %6d calls: %s\n", pTimer->m_Timer.Scale(1000000.0f).GetTime(), (int)pTimer->m_Calls, pTimer->m_pName);
				LogFile(CStrF("%5.1f micro s, %6d calls: %s\n", pTimer->m_Timer.Scale(1000000.0f).GetTime(), (int)pTimer->m_Calls, pTimer->m_pName));
			}
			else
			{
				M_TRACEALWAYS("%5.1f ms     , %6d calls: %s\n", pTimer->m_Timer.Scale(1000.0f).GetTime(), (int)pTimer->m_Calls, pTimer->m_pName);
				LogFile(CStrF("%5.1f ms     , %6d calls: %s\n", pTimer->m_Timer.Scale(1000.0f).GetTime(), (int)pTimer->m_Calls, pTimer->m_pName));
			}
		}
		++Iter;
	}
}
#endif



// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CDisplayContextPS3, CDisplayContext);


// -------------------------------------------------------------------
CDisplayContextPS3::CDisplayContextPS3()
{
	MSCOPE(CDisplayContextPS3::-, RENDER_GL);
	MRTC_AddRef();
	MRTC_AddRef();
	MRTC_AddRef(); // Make sure we aren't deleted

	m_bLog = false;
	m_bVSync = true;
	m_bAntialias = false;
	m_bAddedToConsole = false;
	m_bPendingResetMode = false;
	m_BackBufferFormat = IMAGE_FORMAT_BGRA8;
	m_pMainThread = MRTC_SystemInfo::OS_GetThreadID();

/*	// The following code snippet causes an assert in psgl (glCopyTexSubImage2D call)
	{
		M_BREAKPOINT;

		psglInit(NULL);

		extern int _psglDebugChannel;
		_psglDebugChannel = 1;

		PSGLbufferParameters DeviceParams;
		DeviceParams.width = 1280;
		DeviceParams.height = 720;
		DeviceParams.colorBits = 24;
		DeviceParams.alphaBits = 8;
		DeviceParams.depthBits = 8;
		DeviceParams.stencilBits = 24;
		DeviceParams.deviceType = PSGL_DEVICE_TYPE_AUTO;
		DeviceParams.TVStandard = PSGL_TV_STANDARD_NONE;
		DeviceParams.TVFormat = PSGL_TV_FORMAT_AUTO;
		DeviceParams.bufferingMode = PSGL_BUFFERING_MODE_DOUBLE;
		DeviceParams.antiAliasing = GL_FALSE;
		CDisplayContextPS3::ms_This.m_pDevice = psglCreateDevice(&DeviceParams);

		CRenderContextPS3::ms_This.m_pContext = psglCreateContext();
		psglMakeCurrent(CRenderContextPS3::ms_This.m_pContext, CDisplayContextPS3::ms_This.m_pDevice);

		psglSwap();

		glBindTexture(GL_TEXTURE_2D, 1234);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexImage2D(GL_TEXTURE_2D, 0, GL_ARGB_SCE, 1280, 720, 0, GL_BGRA, GL_UNSIGNED_BYTE, NULL);
		glCopyTexImage2D(GL_TEXTURE_2D, 0, GL_ARGB_SCE, 0, 0, 1280, 720, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindTexture(GL_TEXTURE_2D, 1234);
		glCopyTexSubImage2D(GL_TEXTURE_2D, 0, 0, 360, 0, 360, 640, 360);
		glBindTexture(GL_TEXTURE_2D, 0);
	}
*/
}

CDisplayContextPS3::~CDisplayContextPS3()
{
	MSCOPE(CDisplayContextPS3::~, RENDER_GL);


	if(m_bAddedToConsole)
	{
		MACRO_RemoveSubSystem(this);
		RemoveFromConsole();
	}

}

bool CDisplayContextPS3::SetOwner(void* _pNewOwner)
{
	DebugBreak();

	return true;
}

void ReportPSGL(GLenum _reportEnum, GLuint _reportClassMask, const char* _pString);
void ReportCG(void);

void CDisplayContextPS3::Create()
{
	MSCOPE(CDisplayContextPS3::Create, RENDER_GL);

	if(m_bAddedToConsole)
		return;

	// Call super
	CDisplayContext::Create();

	// Do some proper options here later on
//	GLErr("Create(0)");
	psglInit(NULL);
//	GLErr("Create(1)");

	EnumModes();
	InitSettings();
	AddToConsole();
	m_bAddedToConsole = true;
	CRenderContextPS3::ms_This.Create(this, "");

	{
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (pSys && pSys->GetEnvironment()->Find("GL_LOG"))
			m_bLog = pSys->GetEnvironment()->GetValuei("GL_LOG", 1) != 0;
	}

	psglSetReportFunction(ReportPSGL);
	psglEnableReport(PSGL_REPORT_ALL);

	// Initialize CG compiler
	cgRTCgcInit();

	MACRO_AddSubSystem(this);
}

// -------------------------------------------------------------------

int GetColorBits(int _Format)
{
	switch(_Format)
	{
	default:
		break;
	case IMAGE_FORMAT_RGB10A2_F:
	case IMAGE_FORMAT_BGR10A2: return 30;
	case IMAGE_FORMAT_BGRA4: return 12;
	case IMAGE_FORMAT_BGR5A1:
	case IMAGE_FORMAT_BGR5: return 15;

	case IMAGE_FORMAT_I16:
	case IMAGE_FORMAT_RGB16:
	case IMAGE_FORMAT_B5G6R5: return 16;

	case IMAGE_FORMAT_RGBA8:
	case IMAGE_FORMAT_BGRA8:
	case IMAGE_FORMAT_BGRX8:
	case IMAGE_FORMAT_RGB8:
	case IMAGE_FORMAT_BGR8: return 24;

	case IMAGE_FORMAT_RGBA16_F:
	case IMAGE_FORMAT_RGBA16:
	case IMAGE_FORMAT_BGRA16: return 48;

	case IMAGE_FORMAT_I8A8:
	case IMAGE_FORMAT_I8: return 8;

	case IMAGE_FORMAT_RGBA32_F:
	case IMAGE_FORMAT_RGB32_F: return 96;
	}
	return 0;
}

int GetAlphaBits(int _Format)
{
	switch(_Format)
	{
	default:
		break;
	case IMAGE_FORMAT_RGB10A2_F:
	case IMAGE_FORMAT_BGR10A2: return 2;

	case IMAGE_FORMAT_BGRA4: return 4;

	case IMAGE_FORMAT_BGR5A1: return 1;

	case IMAGE_FORMAT_RGBA16_F:
	case IMAGE_FORMAT_RGBA16:
	case IMAGE_FORMAT_BGRA16: return 16;

	case IMAGE_FORMAT_RGBA8:
	case IMAGE_FORMAT_BGRA8:
	case IMAGE_FORMAT_I8A8:
	case IMAGE_FORMAT_A8: return 8;

	case IMAGE_FORMAT_RGBA32_F: return 32;
	}
	return 0;
}

int GetDepthBits(int _Format)
{
	switch(_Format)
	{
	default:
		break;
	case IMAGE_FORMAT_Z24S8: return 24;
	}

	return 0;
}

int GetStencilBits(int _Format)
{
	switch(_Format)
	{
	default:
		break;
	case IMAGE_FORMAT_Z24S8: return 8;
	}

	return 0;
}

void CDisplayContextPS3::ResetMode()
{
	if(CDisplayContextPS3::ms_This.m_DisplayModeNr < 0)
		return;

	if(CDisplayContextPS3::ms_This.m_pDevice)
	{
		glFinish();

		// Delete all resources before destroying context
		CRenderContextPS3::ms_This.VB_DeleteAll();
		CRenderContextPS3::ms_This.GL_DestroyAllPrograms();
		CRenderContextPS3::ms_This.GL_DeleteTextures();

		GLErr("ResetMode(0, 1)");
		if(CRenderContextPS3::ms_This.m_CGContext)
			cgDestroyContext(CRenderContextPS3::ms_This.m_CGContext);
		GLErr("ResetMode(0, 2)");
		if(CRenderContextPS3::ms_This.m_pContext)
			psglDestroyContext(CRenderContextPS3::ms_This.m_pContext);
		GLErr("ResetMode(0, 3)");
		psglDestroyDevice(m_pDevice);
		GLErr("ResetMode(0, 4)");
		CRenderContextPS3::ms_This.m_pContext = NULL;
		CDisplayContextPS3::ms_This.m_pDevice = NULL;
	}

//	CDCGL_VideoMode* pMode = (CDCGL_VideoMode*)((CDC_VideoMode*)CDisplayContextPS3::ms_This.m_lspModes[CDisplayContextPS3::ms_This.m_DisplayModeNr]);

//	PSGLbufferParameters DeviceParams;
//	DeviceParams.width = pMode->m_Width;
//	DeviceParams.height = pMode->m_Height;
//	DeviceParams.colorBits = GetColorBits(pMode->m_FrontBufferMode);
//	DeviceParams.alphaBits = GetAlphaBits(pMode->m_FrontBufferMode);
//	DeviceParams.depthBits = GetDepthBits(pMode->m_ZBufferMode);
//	DeviceParams.stencilBits = GetStencilBits(pMode->m_ZBufferMode);
//	DeviceParams.deviceType = PSGL_DEVICE_TYPE_AUTO;
//	DeviceParams.TVStandard = PSGL_TV_STANDARD_NONE;
//	DeviceParams.TVFormat = PSGL_TV_FORMAT_AUTO;
//	DeviceParams.bufferingMode = PSGL_BUFFERING_MODE_DOUBLE;
//	DeviceParams.antiAliasing = m_bAntialias?GL_TRUE:GL_FALSE;
//	CDisplayContextPS3::ms_This.m_pDevice = psglCreateDevice(&DeviceParams);
	CDisplayContextPS3::ms_This.m_pDevice = psglCreateDeviceAuto(GL_ARGB_SCE, GL_DEPTH_COMPONENT24, GL_MULTISAMPLING_NONE_SCE);

	GLuint Width, Height;
	psglGetDeviceDimensions(CDisplayContextPS3::ms_This.m_pDevice, &Width, &Height);

//	GLErr("ResetMode(1)");
#ifdef PLATFORM_PS3
	M_ASSERT(CDisplayContextPS3::ms_This.m_pDevice, "Failed to create psgl device");
#endif

	CRenderContextPS3::ms_This.m_pContext = psglCreateContext();
#ifdef PLATFORM_PS3
	M_ASSERT(CRenderContextPS3::ms_This.m_pContext, "Failed to create psgl context");
#endif
	psglMakeCurrent(CRenderContextPS3::ms_This.m_pContext, CDisplayContextPS3::ms_This.m_pDevice);
//	GLErr("ResetMode(2)");

//	psglLoadShaderLibrary("/app_home/System/GL/PS3_Cache/shaders.bin");

	SetRenderTarget(Width, Height, IMAGE_FORMAT_BGRX8, false);
	CBackbufferContext TempContext;
	TempContext.m_Setup.m_Width = Width;
	TempContext.m_Setup.m_Height = Height;
	TempContext.m_Setup.m_BackBufferFormat = IMAGE_FORMAT_BGRA8;
	TempContext.m_Setup.m_bZBuffer = true;
	TempContext.m_Setup.m_ZStartMem = 0x7fffffff;
	m_CurrentBackbufferContext = TempContext;
	CDisplayContextPS3::ms_This.m_DefaultBackbufferContext = m_CurrentBackbufferContext;
	CDisplayContextPS3::ms_This.m_BackbufferImage.CreateVirtual(Width, Height, IMAGE_FORMAT_BGRA8, IMAGE_MEM_VIRTUAL);

	CRenderContextPS3::ms_This.m_CGContext = cgCreateContext();
	GLErr("ResetMode(3)");
	cgSetErrorCallback(ReportCG);
	GLErr("ResetMode(3)");

	glViewport(0, 0, m_CurrentBackbufferContext.m_Setup.m_Width,m_CurrentBackbufferContext.m_Setup.m_Height);
	glScissor(0, 0, m_CurrentBackbufferContext.m_Setup.m_Width, m_CurrentBackbufferContext.m_Setup.m_Height);
	CRenderContextPS3::ms_This.GL_InitSettings();
	CRenderContextPS3::ms_This.GL_InitTextures();
}

void CDisplayContextPS3::SetMode(int _Mode)
{
	MSCOPE(CDisplayContextPS3::SetMode, RENDER_PS3);
	if (m_DisplayModeNr != _Mode)
	{
		m_DisplayModeNr = _Mode;
		if (m_pMainThread == MRTC_SystemInfo::OS_GetThreadID())
			ResetMode();
		else
			m_bPendingResetMode = true;
	}
};


CDisplayContextPS3::CBackbufferContext CDisplayContextPS3::SetRenderTarget(int _Width, int _Height, int _ColorBufferFormat, bool _bRetainZBuffer)
{
	CBackbufferContext OldContext = m_CurrentBackbufferContext;
	CBackbufferContext NewContext;

	NewContext.m_Setup.m_Width = _Width;
	NewContext.m_Setup.m_Height = _Height;
	NewContext.m_Setup.m_BackBufferFormat = GL_ARGB_SCE;
	NewContext.m_Setup.m_bZBuffer = (_bRetainZBuffer == false);

	RestoreRenderTarget(NewContext);
	return OldContext;
}

void CDisplayContextPS3::RestoreRenderTarget(CBackbufferContext& _Context)
{
	return;
	if(!memcmp(&_Context.m_Setup, &m_CurrentBackbufferContext.m_Setup, sizeof(CBackbufferContext::CSetup)))
		return;

	GLuint fid;
	glGenFramebuffersOES(1, &fid);
//	GLErr("RestoreRenderTarget(0, 1)");
	glBindFramebufferOES(GL_FRAMEBUFFER_OES, fid);
//	GLErr("RestoreRenderTarget(0, 2)");

	GLuint rid;
	glGenRenderbuffersOES(1, &rid);
//	GLErr("RestoreRenderTarget(1)");
	glBindRenderbufferOES(GL_RENDERBUFFER_OES, rid);
//	GLErr("RestoreRenderTarget(2)");
	glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_ARGB_SCE, _Context.m_Setup.m_Width, _Context.m_Setup.m_Height);
//	GLErr("RestoreRenderTarget(3)");
	glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_COLOR_ATTACHMENT0_EXT, GL_RENDERBUFFER_OES, rid);
//	GLErr("RestoreRenderTarget(4)");
	if(_Context.m_Setup.m_bZBuffer)
	{
		GLuint zid;
		glGenRenderbuffersOES(1, &zid);
		glBindRenderbufferOES(GL_RENDERBUFFER_OES, zid);
//		GLErr("RestoreRenderTarget(5)");
		glRenderbufferStorageOES(GL_RENDERBUFFER_OES, GL_DEPTH_COMPONENT, _Context.m_Setup.m_Width, _Context.m_Setup.m_Height);
//		GLErr("RestoreRenderTarget(6)");
		glFramebufferRenderbufferOES(GL_FRAMEBUFFER_OES, GL_DEPTH_ATTACHMENT_OES, GL_RENDERBUFFER_OES, zid);
//		GLErr("RestoreRenderTarget(7)");
	}
	glViewport(0, 0, _Context.m_Setup.m_Width, _Context.m_Setup.m_Height);
	glScissor(0, 0, _Context.m_Setup.m_Width, _Context.m_Setup.m_Height);
	GLErr("RestoreRenderTarget(8)");
	_Context.m_FBO = fid;
	m_CurrentBackbufferContext = _Context;
}

// -------------------------------------------------------------------
int CDisplayContextPS3::PageFlip()
{
	MSCOPE(CDisplayContextPS3::PageFlip, RENDER_GL);
	
	GLErr("PageFlip(0)");
	CDisplayContext::PageFlip();
	//TODO: Remove this glFinish calls
//	glFinish();
//	GLErr("PageFlip(1)");

	CMTime PreSwap;
	PreSwap.Snapshot();

	psglSwap();
	CMTime PostSwap;
	PostSwap.Snapshot();
	CRenderContextPS3::ms_This.m_Stats.m_BlockTime.AddData((PostSwap - PreSwap).GetTime());
	GLErr("PageFlip(2)");

	if(m_bPendingResetMode)
	{
		ResetMode();
		m_bPendingResetMode = false;
	}

	return 0;
};

// -------------------------------------------------------------------
void CDisplayContextPS3::EnumModes()
{
	MSCOPE(CDisplayContextPS3::EnumModes, RENDER_GL);

	m_lspModes.Destroy();

	const int aWidth[]  = { 720, 720, 1280, 1920, 0 };
	const int aHeight[] = { 480, 576, 720,  1080 };
//	const uint32 aBackFormats[] = {IMAGE_FORMAT_BGRX8, IMAGE_FORMAT_RGBA16_F, IMAGE_FORMAT_RGBA32_F, 0 };
	const uint32 aBackFormats[] = {IMAGE_FORMAT_BGRX8, 0 };

	for(int iSize = 0; aWidth[iSize]; iSize++)
	{
		for(int iBack = 0; aBackFormats[iBack]; iBack++)
		{
			CDCGL_VideoMode* pMode = MNew(CDCGL_VideoMode);
			pMode->m_Width				= aWidth[iSize];
			pMode->m_Height				= aHeight[iSize];
			pMode->m_ZBufferMode		= IMAGE_FORMAT_Z24S8;
			pMode->m_FrontBufferMode	= IMAGE_FORMAT_BGRX8;
			pMode->m_BackBufferMode		= aBackFormats[iBack];
			pMode->m_Format.CreateVirtual(pMode->m_Width, pMode->m_Height, aBackFormats[iBack], 0);
			pMode->m_RefreshRate = 60;
			m_lspModes.Add(TPtr<CDC_VideoMode>(pMode));
		}
	}

	SetScreenAspect(4.0f/3.0f);

}

void CDisplayContextPS3::InitSettings()
{
	MACRO_GetSystemEnvironment(pEnv);

	m_bVSync = pEnv->GetValuei("VID_VSYNC", 1);
	m_bAntialias = pEnv->GetValuei("R_ANTIALIAS", 1) > 1;
	m_BackBufferFormat = pEnv->GetValuei("R_BACKBUFFERFORMAT", IMAGE_FORMAT_BGRX8);
}

// -------------------------------------------------------------------
CImage* CDisplayContextPS3::GetFrameBuffer()
{
	MSCOPE(CDisplayContextPS3::GetFrameBuffer, RENDER_GL);

	return &m_BackbufferImage;
};

void CDisplayContextPS3::ClearFrameBuffer(int _Buffers, int _Color)
{
	MSCOPE(CDisplayContextPS3::ClearFrameBuffer, RENDER_GL);

	CPixel32 Pixel(_Color);
	glClearDepthf(1.0f);
	GLErr("glClearDepthf");
	glClearColor(Pixel.GetR() * (1.0f / 255.0f), Pixel.GetG() * (1.0f / 255.0f), Pixel.GetB() * (1.0f / 255.0f), Pixel.GetA() * (1.0f / 255.0f));
	GLErr("glClearColor");
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	GLErr("ClearFrameBuffer");
};

CRenderContext* CDisplayContextPS3::GetRenderContext(CRCLock* _pLock)
{
	MSCOPE(CDisplayContextPS3::GetRenderContext, RENDER_GL);
	return &CRenderContextPS3::ms_This;
};

// --------------------------------
//  CConsoleClient virtuals.
// --------------------------------
void CDisplayContextPS3::Con_SetDataPath(CStr _DataPath)
{
	DebugBreak();
}

void CDisplayContextPS3::Con_r_antialias(int _Antialias)
{
	bool Antialias = _Antialias > 1;
	if (m_bAntialias != Antialias)
	{
		m_bAntialias = Antialias;
		MACRO_GetSystemEnvironment(pEnv);
		pEnv->SetValuei("R_ANTIALIAS", _Antialias);
		m_bPendingResetMode = true;
	}
}

void CDisplayContextPS3::Con_r_backbufferformat(int _BackbufferFormat)
{
	if (m_BackBufferFormat != _BackbufferFormat)
	{
		m_BackBufferFormat = _BackbufferFormat;
		MACRO_GetSystemEnvironment(pEnv);
		pEnv->SetValuei("R_BACKBUFFERFORMAT", _BackbufferFormat);
		m_bPendingResetMode = true;
	}
}


void CDisplayContextPS3::Parser_Modes()
{
	for (int i = 0; i < m_lspModes.Len(); i++)
		ConOut(CStrF("%d : %s, %d hz", i, m_lspModes[i]->m_Format.IDString().Str(), m_lspModes[i]->m_RefreshRate));
}

void CDisplayContextPS3::Parser_VSync(int _VSync)
{
	m_bVSync = _VSync != 0;
}

void CDisplayContextPS3::Register(CScriptRegisterContext & _RegContext)
{
	CDisplayContext::Register(_RegContext);

//	_RegContext.RegFunction("r_vsync", this, &CDisplayContextPS3::Parser_VSync);
	_RegContext.RegFunction("r_vmodes", this, &CDisplayContextPS3::Parser_Modes);
//	_RegContext.RegFunction("r_antialias", this, &CDisplayContextPS3::Con_r_antialias);
//	_RegContext.RegFunction("r_backbufferformat", this, &CDisplayContextPS3::Con_r_backbufferformat);
};

// -------------------------------------------------------------------

void CDisplayContextPS3::OnRefresh(int _Context)
{
}

void CDisplayContextPS3::OnBusy(int _Context)
{
}

CDisplayContext* gf_CreateDisplayContextPS3Static()
{
#ifndef PLATFORM_PS3
	CRenderContextPS3::ms_This.Render_PrecacheFlush();
#endif
#ifndef PLATFORM_CONSOLE
	MACRO_GetSystem;
	CRenderContextPS3::ms_This.m_bLogUsage = pSys->GetEnvironment()->GetValuei("RESOURCES_LOG") != 0;
#endif

	CDisplayContextPS3::ms_This.m_pMainThread = MRTC_SystemInfo::OS_GetThreadID();

	return &CDisplayContextPS3::ms_This;
}
