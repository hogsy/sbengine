
/*
------------------------------------------------------------------------------------------------
Name:		MDisplay.cpp/h
Purpose:	Display context
Creation:	9703??

Contents:
class				CDisplayContext				9703??  -				Abstract CDisplayContext

------------------------------------------------------------------------------------------------
*/
#ifndef _INC_MOS_Display
#define _INC_MOS_Display

#include "MRender.h"

enum
{
	CDC_CLEAR_COLOR		=1,
	CDC_CLEAR_ZBUFFER	=2,
	CDC_CLEAR_STENCIL	=4,

	CDC_WINFLAGS_SINGLEBUFFER =1,
};


#define SCREENASPECT_NORMAL		1.f	
#define SCREENASPECT_WIDE		1.5f
#define SCREENASPECT_REALWIDE	16.f/9.f

// -------------------------------------------------------------------
//  CDisplayContext prototype
// -------------------------------------------------------------------

class CDC_VideoMode : public CReferenceCount
{
public:
	CImage m_Format;
	int m_RefreshRate;

	CDC_VideoMode()
	{
		m_RefreshRate = 0;
	}
};

class spCDC_VideoMode : public TPtr<CDC_VideoMode>
{
public:
	spCDC_VideoMode() : TPtr<CDC_VideoMode>() { }
	spCDC_VideoMode(const TPtr<CDC_VideoMode>& _x) : TPtr<CDC_VideoMode>(_x) { }

	int Compare(const spCDC_VideoMode& _Elem) const;
};

class CRCLock
{
public:
	MRTC_MutualWriteManyRead* m_pLock;

	CRCLock() : m_pLock(0) {}
	~CRCLock()
	{
		if(m_pLock)
		{
			m_pLock->Unlock();
		}
	}
};

class SYSTEMDLLEXPORT CDisplayContext : public CConsoleClient, public CSubSystem
{
protected:
	TList_SortableVector<spCDC_VideoMode> m_lspModes;		// This is a pointer list because CImage doesn't have operator=

	
	int		m_DisplayModeNr;
	fp4		m_fScreenAspect;	// Added by Talbot
	fp4		m_fPixelAspect;	// Added by Talbot
	int		m_iRefreshRate;			// Added by Talbot
	int		m_bPendingScreenshot;
	bool	m_bProgressiveScan;		// Added by Talbot
	bool	m_bIsWideSreen;		// Added by Talbot

	void* m_pOwnerThread;

public:
	CDisplayContext();
	~CDisplayContext();
	virtual void Create();

	virtual bool IsValid();

	virtual bool SetOwner(void* _pNewOwner);
	virtual void* GetOwner();

	virtual CPnt GetScreenSize() pure;

	virtual void SetMode(int nr) pure;
	virtual int GetMode();
	virtual bool IsFullScreen();
	
	virtual bool IsWidescreen()				{ return (m_bIsWideSreen);}			// Added by Talbot
	virtual void SetWidescreen(bool _bWide)				{ m_bIsWideSreen = _bWide;}			// Added by Talbot

	virtual void SetScreenAspect(fp4 _aspect)	{ m_fScreenAspect = _aspect;}								// Added by Talbot
	virtual fp4  GetScreenAspect()		{ return m_fScreenAspect;	}								// Added by Talbot
	virtual void SetPixelAspect(fp4 _aspect)	{ m_fPixelAspect = _aspect;}								// Added by Talbot
	virtual fp4  GetPixelAspect()		{ return m_fPixelAspect;	}								// Added by Talbot
	
	virtual bool IsProgressive()			{return m_bProgressiveScan;}									// Added by Talbot
	virtual void SetProgressive(bool _scan) { m_bProgressiveScan = _scan;}									// Added by Talbot

	virtual int  GetRefreshRate()			{ return m_iRefreshRate;}										// Added by Talbot
	virtual void SetRefreshRate(int _rate)	{ if(_rate != 60) _rate = 50; m_iRefreshRate = _rate;}			// Added by Talbot
	
	virtual void SetScreenRatio(int _Width, int _Height, fp4 _PixelAspect);

	virtual int PageFlip();
	virtual void Update() {}
	virtual void ModeList_Init() pure;
	virtual int ModeList_Find(int width, int height, int format, int _RefreshRate = -1);
	virtual void ModeList_DeleteFormat(int _Format);
	virtual int ModeList_GetNumModes();
	virtual const CDC_VideoMode& ModeList_GetDesc(int _iVMode);
	
	virtual void CaptureScreenshot();

	virtual int SpawnWindow(int _Flags = 0) pure;							// Returns window-index
	virtual void DeleteWindow(int _iWnd) pure;							// Returns window-index
	virtual void SelectWindow(int _iWnd) pure;
	virtual void SetWindowPosition(int _iWnd, CRct _Rct) pure;

	virtual void SetPalette(spCImagePalette _spPal) pure;
	virtual CImage* GetFrameBuffer() pure;
	virtual void ClearFrameBuffer(int _Buffers = (CDC_CLEAR_COLOR | CDC_CLEAR_ZBUFFER | CDC_CLEAR_STENCIL), int _Color = 0) pure;

	virtual CRenderContext* GetRenderContext(class CRCLock* _pLock) pure;

#ifdef PLATFORM_WIN_PC
	virtual int Win32_CreateFromWindow(void* _hWnd, int _Flags = 0) pure;				// Returns window-index
	virtual int Win32_CreateWindow(int _WS, void* _pWndParent, int _Flags = 0) pure;		// Returns window-index
	virtual void Win32_ProcessMessages() pure;
	virtual void* Win32_GethWnd(int _iWnd = 0) pure;
//	virtual void Win32_SetMessageCallback(CReferenceCount) pure;
#endif

	// CSubSystems overrides:
	virtual void OnRefresh(int _Context);
	virtual void OnBusy(int _Context);
};



typedef TPtr<CDisplayContext> spCDisplayContext;

/*
CSplitterWnd
    CView : CMWnd
        CView3D : 
            CMWnd->CreateFromHandle



*/



// -------------------------------------------------------------------
/*
class DCDesc : public CReferenceCount
{
public:
	CStr Name;
	CStr Desc;
//	void construct(CDisplayContext* pConstr, CConsole* _pCon);
	spCDisplayContext (*pConstr)(CConsole*);
};

typedef TPtr<DCDesc> spDCDesc;
*/

#endif // _INC_MOS_Display






