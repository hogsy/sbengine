/*
------------------------------------------------------------------------------------------------
Name:		MDispGL.cpp/h
Purpose:	Display context
Creation:	9703??

Contents:
class				CDisplayContextGL				9703??  -				CDisplayContext for OpenGL

------------------------------------------------------------------------------------------------
*/
#ifndef _INC_MOS_DispGL
#define _INC_MOS_DispGL

#include "MGLGlobalFunctions.h"
#include "../../MSystem/MSystem.h"
#include "MRenderPS3_Image.h"
#include "MRenderPS3_Context.h"


// -------------------------------------------------------------------
//  CDisplayContextGL
// -------------------------------------------------------------------

// -------------------------------------------------------------------
class CDCGL_VideoMode : public CDC_VideoMode
{
public:
	int16 m_Width;
	int16 m_Height;
	uint32 m_BackBufferMode;
	uint32 m_ZBufferMode;
	uint32 m_FrontBufferMode;
};

typedef TPtr<CDCGL_VideoMode> spCDCGL_VideoMode;

// -------------------------------------------------------------------
class CRenderContextPS3;
class CDisplayContextPS3 : public CDisplayContext
{
	friend class CRenderContextPS3;
	MRTC_DECLARE;

public:
	static CDisplayContextPS3 ms_This;
	void* m_pMainThread;
protected:
	class CBackbufferContext
	{
	public:
		class CSetup
		{
		public:
			CSetup()
			{
				memset(this, 0, sizeof(*this));
				m_ZStartMem = -1;
			}
			bint m_bZBuffer;
			int m_BackBufferFormat;
			int m_Width;
			int m_Height;
			int m_ZStartMem;
		};
		CBackbufferContext() : m_FBO(0)
		{
		}

		CSetup m_Setup;
		uint32 m_FBO;
	};

	// -------------------------------------------------------------------
	PSGLdevice* m_pDevice;

	virtual CPnt GetScreenSize()
	{
		return CPnt(m_CurrentBackbufferContext.m_Setup.m_Width, m_CurrentBackbufferContext.m_Setup.m_Height);
	};


	uint32 m_bLog:1;
	uint32 m_bVSync:1;
	uint32 m_bAntialias:1;
	uint32 m_bAddedToConsole:1;
	uint32 m_bPendingResetMode:1;
	uint32 m_BackBufferFormat;
	CBackbufferContext m_CurrentBackbufferContext;
	CBackbufferContext m_DefaultBackbufferContext;
	CImagePS3 m_BackbufferImage;

	void ResetMode();
	CBackbufferContext SetRenderTarget(int _Width, int _Height, int _ColorBufferFormat, bool _bRetainZBuffer);
	void RestoreRenderTarget(CBackbufferContext& _Context);
public:

	// DisplayContext overrides
	CDisplayContextPS3();
	~CDisplayContextPS3();
	virtual void Create();
	virtual bool SetOwner(void* _pNewOwner);

	virtual void SetMode(int nr);
	virtual int PageFlip();

	virtual CImage* GetFrameBuffer();
	virtual void ClearFrameBuffer(int _Buffers = (CDC_CLEAR_COLOR | CDC_CLEAR_ZBUFFER | CDC_CLEAR_STENCIL), int _Color = 0);

	virtual CRenderContext* GetRenderContext(class CRCLock* _pLock);

	// Scriptstuff
	virtual void Con_SetDataPath(CStr _DataPath);
	void Con_r_antialias(int _Antialias);
	void Con_r_backbufferformat(int _BackbufferFormat);
	virtual void Parser_Modes();
	virtual void Parser_VSync(int _VSync);
	void Register(CScriptRegisterContext &_RegContext);

	// CSubSystems overrides:
	virtual void OnRefresh(int _Context);
	virtual void OnBusy(int _Context);

	void EnumModes();
	void InitSettings();

	int SpawnWindow(int _Flags){return 0;}
	void DeleteWindow(int _iWnd){}
	void SelectWindow(int _iWnd){}
	void SetWindowPosition(int _iWnd, CRct _Rct){}
	void SetPalette(spCImagePalette _spPal){}
	void ModeList_Init(){}

#ifdef PLATFORM_WIN_PC
	bool m_bLogUsage;
	int Win32_CreateFromWindow(void* _hWnd, int _Flags = 0){return 0;}
	int Win32_CreateWindow(int _WS, void* _pWndParent, int _Flags = 0){return 0;}
	void Win32_ProcessMessages(){}
	void* Win32_GethWnd(int _iWnd = 0){return 0;}
#endif
};

#endif // _INC_MOS_DispGL






