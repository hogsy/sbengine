#include "PCH.h"

#include "MDisplay.h"
#include "../MSystem.h"

int spCDC_VideoMode::Compare(const spCDC_VideoMode& _Elem) const
{
	int Windowed = 0;
	if ((*this)->m_Format.GetMemModel() & IMAGE_MEM_WINDOWED)
		Windowed--;
	if (_Elem->m_Format.GetMemModel() & IMAGE_MEM_WINDOWED)
		Windowed++;
	if (Windowed)
		return Windowed;

	if ((*this)->m_Format.GetWidth() < _Elem->m_Format.GetWidth())
		return -1;
	if ((*this)->m_Format.GetWidth() > _Elem->m_Format.GetWidth())
		return 1;
	if ((*this)->m_Format.GetHeight() < _Elem->m_Format.GetHeight())
		return -1;
	if ((*this)->m_Format.GetHeight() > _Elem->m_Format.GetHeight())
		return 1;
	if ((*this)->m_Format.GetFormat() < _Elem->m_Format.GetFormat())
		return -1;
	if ((*this)->m_Format.GetFormat() > _Elem->m_Format.GetFormat())
		return 1;
	if ((*this)->m_RefreshRate < _Elem->m_RefreshRate)
		return -1;
	if ((*this)->m_RefreshRate > _Elem->m_RefreshRate)
		return 1;

	return 0;
}



CDisplayContext::CDisplayContext()
{
	MAUTOSTRIP(CDisplayContext_ctor, MAUTOSTRIP_VOID);
/*	m_MouseMove = CPnt(0, 0);
	m_MousePos = CPnt(0, 0);
	m_MouseSensitivity = 1.0f;
	m_MouseWheel = 0;
	m_WheelSensitivity = 1.0f;
	m_MouseButtons = 0;*/
	m_DisplayModeNr = -1;
	m_bPendingScreenshot = 0;
	m_bProgressiveScan = false;
	m_iRefreshRate = 0;
	m_fScreenAspect = 1.0f;
	m_fPixelAspect = 1.0f;
	m_bIsWideSreen = false;

	m_pOwnerThread	= MRTC_SystemInfo::OS_GetThreadID();
}

CDisplayContext::~CDisplayContext()
{
	MAUTOSTRIP(CDisplayContext_dtor, MAUTOSTRIP_VOID);
}

void CDisplayContext::Create()
{
	MAUTOSTRIP(CDisplayContext_Create, MAUTOSTRIP_VOID);
}

bool CDisplayContext::SetOwner(void* _pNewOwner)
{
	MAUTOSTRIP(CDisplayContext_SetOwner, MAUTOSTRIP_VOID);
	// Only current owner can change owner of context
	if(m_pOwnerThread != MRTC_SystemInfo::OS_GetThreadID())
		return false;
	m_pOwnerThread	= _pNewOwner;
	return true;
}

void* CDisplayContext::GetOwner()
{
	return m_pOwnerThread;
}

bool CDisplayContext::IsValid()
{
	return true;
}

void CDisplayContext::SetMode(int nr)
{
	MAUTOSTRIP(CDisplayContext_SetMode, MAUTOSTRIP_VOID);
	int Mode = GetMode();
	if (Mode < 0) return;
	MACRO_GetRegisterObject(CInputContext, pInput, "SYSTEM.INPUT");
//	if (pInput) pInput->SetMouseArea(m_lspModes[Mode]->GetRect());

//	if (g_pOS && g_pOS->m_spInput)
//		g_pOS->m_spInput->SetMouseArea(m_lspModes[Mode]->GetRect());
}

int CDisplayContext::GetMode()
{
	MAUTOSTRIP(CDisplayContext_GetMode, 0);
	return m_DisplayModeNr;
}

bool CDisplayContext::IsFullScreen()
{
	MAUTOSTRIP(CDisplayContext_IsFullScreen, false);
	if (m_DisplayModeNr < 0) return FALSE;
	return ((m_lspModes[m_DisplayModeNr]->m_Format.GetMemModel() & IMAGE_MEM_WINDOWED) == 0);
};

int CDisplayContext::PageFlip()
{
	MAUTOSTRIP(CDisplayContext_PageFlip, 0);
	if(m_bPendingScreenshot)
	{
		
		if (m_bPendingScreenshot == 1)
		{
			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
			if(pSys)
			{
				CImage* pImg = GetFrameBuffer();
				if (pImg != NULL)
				{
	#ifdef PLATFORM_XBOX1
	#if defined(M_DEMO_XBOX1)
					CStr TargetPath = "Z:\\ScreenShots";
	#else
					CStr TargetPath = "T:\\ScreenShots";
	#endif
	#else
					CStr TargetPath = pSys->m_ExePath + "ScreenShots";
	#endif

					int i = 0;
					if (CDiskUtil::CreatePath(TargetPath))
					{
						while(CDiskUtil::FileExists(TargetPath + CStrF("\\Screen%.2d.png", i))) i++;
	#ifdef PLATFORM_WIN_PC
						MRTC_SystemInfo::OS_Sleep(500);	// Fixes some driver weirdness with rendering pipeline not beeing flushed before reading back the framebuffer.
	#endif

						spCImage spConv = pImg->Convert(pImg->GetFormat());
						spConv = spConv->Convert(IMAGE_FORMAT_BGR8);
						
						int bFake = pSys->GetEnvironment()->GetValuei("vid_fakeoverbright", 1) || !IsFullScreen();
						if(!bFake)
							// Add image to itself to compensate for overbrightness
							spConv->Blt(spConv->GetClipRect(), *spConv, IMAGE_OPER_ADDSATURATE, CPnt(0,0), 0);

						CStr Filename = TargetPath + CStrF("\\Screen%.2d.png", i);
						spConv->Write(Filename);

	#ifndef PLATFORM_CONSOLE

						// Z-Buffer capture
						CRCLock RCLock;
						CRenderContext* pRC = GetRenderContext(&RCLock);
						if (pRC)
						{
							int h = spConv->GetHeight();
							int w = spConv->GetWidth();

							CImage ZBuffer;
							ZBuffer.Create(w, h, IMAGE_FORMAT_I8, IMAGE_MEM_IMAGE);

							TThinArray<fp32> lDepth;
							lDepth.SetLen(spConv->GetWidth());

							const CRC_Viewport* pVP = pRC->Viewport_Get();	// This might not be the viewport used to render the scene, so z-far/near could be completely off
							fp32 f = pVP->GetBackPlane();
							fp32 n = pVP->GetFrontPlane() * 0.5f;

							MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
							fp32 zmax = 1000.0f;
							if(pSys)
								zmax = pSys->GetEnvironment()->GetValuef("VID_CAPTUREZRANGE", 1000.0f);

	/*						// Calc max z
							{
								zmax = 0;
								for(int y = 0; y < spConv->GetHeight(); y++)
								{
									pRC->ReadDepthPixels(0-w/2, y-h/2, spConv->GetWidth(), 1, lDepth.GetBasePtr() );
									for(int x = 0; x < w; x++)
									{
										fp32 zw = lDepth[x];
										fp32 zd = (zw - 0.5f) * 2.0f;
										fp32 z = -2.0f * f * n / (zd*(f - n) - (f + n));
										zmax = Max(z, zmax);
									}
								}
							}*/

							{
								for(int y = 0; y < spConv->GetHeight(); y++)
								{
									pRC->ReadDepthPixels(0-w/2, y-h/2, spConv->GetWidth(), 1, lDepth.GetBasePtr() );
									for(int x = 0; x < w; x++)
									{
										fp32 zw = lDepth[x];
										fp32 zd = (zw - 0.5f) * 2.0f;
										fp32 z = -2.0f * f * n / (zd*(f - n) - (f + n));

										int zi = RoundToInt(255.0f * z / zmax);
										ZBuffer.SetPixel(ZBuffer.GetClipRect(), CPnt(x, y), CPixel32(zi,zi,zi,255));
									}
								}
							}

							CStr Filename = TargetPath + CStrF("\\Screen%.2d_z.TGA", i);
							ZBuffer.Write(Filename);
						}
	#endif
					}
				}
			}
		}
		--m_bPendingScreenshot;
	}

	return true;
}


void CDisplayContext::SetScreenRatio(int _Width, int _Height, fp32 _PixelAspect)
{
	// Standard resolution 
	int width	= _Width;
	int height	= _Height;

	fp32 PixelAspect = _PixelAspect;
	fp32 ScreenAspect = (((double)width / (double)height) * (3.0/4.0)) * PixelAspect;// / ((4.0/3.0) * (ScreenAspect)));
		
	SetScreenAspect(ScreenAspect);
}

void CDisplayContext::CaptureScreenshot()
{
	MAUTOSTRIP(CDisplayContext_CaptureScreenshot, MAUTOSTRIP_VOID);
#ifdef PLATFORM_XENON
	m_bPendingScreenshot = 4;
#else
	m_bPendingScreenshot = 1;
#endif
}

void CDisplayContext::ModeList_DeleteFormat(int _Format)
{
	MAUTOSTRIP(CDisplayContext_ModeList_DeleteFormat, MAUTOSTRIP_VOID);
	for (int i = m_lspModes.Len()-1; i >= 0; i--)
		if (m_lspModes[i]->m_Format.GetFormat() == _Format) m_lspModes.Del(i);
};

int CDisplayContext::ModeList_Find(int width, int height, int format, int _RefreshRate)
{
	MAUTOSTRIP(CDisplayContext_ModeList_Find, 0);
	int l = m_lspModes.Len();
	int iBestMode = -1;
	int BestMatch = 0x7fffffff;
	for (int i = 0; i < l; i++)
	{
		int Match = abs(m_lspModes[i]->m_Format.GetWidth() - width) * 4 + abs(m_lspModes[i]->m_Format.GetHeight() - height) * 3 + abs(m_lspModes[i]->m_RefreshRate - _RefreshRate);
		if (!(m_lspModes[i]->m_Format.GetMemModel() & IMAGE_MEM_WINDOWED) 
			&& (m_lspModes[i]->m_Format.GetFormat() & format)
			&& Match < BestMatch
			)
		{
			BestMatch = Match;
			iBestMode = i;
		}
	}
	return iBestMode;
};

int CDisplayContext::ModeList_GetNumModes()
{
	MAUTOSTRIP(CDisplayContext_ModeList_GetNumModes, 0);
	return m_lspModes.Len();
}

const CDC_VideoMode& CDisplayContext::ModeList_GetDesc(int _iVMode)
{
	MAUTOSTRIP(CDisplayContext_ModeList_GetDesc, CDC_VideoMode());
/*	CImage* pImg = m_lspModes[_iVMode];
	_VMode.CreateVirtual(pImg->GetWidth(), pImg->GetHeight(), pImg->GetFormat(), pImg->GetMemModel());

	if (pImg->GetMemModel() & IMAGE_MEM_WINDOWED)
		return CStrF("Windowed %dbit", CImage::Format2BPP(pImg->GetFormat()) );
	else
		return CStrF("%dx%d %dbit", pImg->GetWidth(), pImg->GetHeight(), CImage::Format2BPP(pImg->GetFormat()) );*/

    if (m_lspModes.ValidPos(_iVMode))
	{
		return *m_lspModes[_iVMode];
	}
	else
	{
		static CDC_VideoMode DefaultRet;
		DefaultRet.m_Format.CreateVirtual(640, 480, IMAGE_FORMAT_BGRA8, 0);
		return DefaultRet;
	}
}

void CDisplayContext::OnRefresh(int _Context)
{
	MAUTOSTRIP(CDisplayContext_OnRefresh, MAUTOSTRIP_VOID);
//	Win32_ProcessMessages();
}

void CDisplayContext::OnBusy(int _Context)
{
	MAUTOSTRIP(CDisplayContext_OnBusy, MAUTOSTRIP_VOID);
//	Win32_ProcessMessages();
}







