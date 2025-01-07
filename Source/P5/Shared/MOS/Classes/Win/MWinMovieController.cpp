#include "PCH.h"
#include "MWinMovieController.h"
#include "../../MSystem/MSystem.h"
#include "../../MSystem/Raster/MTexture.h"
#include "../../MSystem/Raster/MTextureContainers.h"
#include "../../Classes/Win/MWinGrph.h"

// vFastRewindBINK

void CMWinMoviewController::vFastRewindBINK(CStr sTextureName, bool _bLoop, bool _bLastFrame, bool _bPaused)
{
	MAUTOSTRIP(CMWinMoviewController_vFastRewindBINK, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	
	if (!pTC)
	{
		m_iRenderMode = 0;
		return;
	}

	if (_bLastFrame)
	{
		vCloseVideo(sTextureName);
	}
	
	// Set properties
	
	int _TextureID = pTC->GetTextureID(sTextureName);
	if(_TextureID > 0)
	{
		CTextureContainer_Video* pTCVideo = safe_cast<CTextureContainer_Video>(pTC->GetTextureContainer(_TextureID));
		if(pTCVideo)
		{
			int iLocal = pTC->GetLocal(_TextureID);
			
			pTCVideo->Pause(iLocal,_bPaused);
			
			if (!_bLastFrame)
				pTCVideo->Rewind(iLocal);
			else
				pTCVideo->MoveToLastFrame(iLocal);

			pTCVideo->AutoRestart(iLocal,_bLoop);

			SetVolume(sTextureName, 1.0);
		}
	}
}

// iGetStatusOfBink

int CMWinMoviewController::iGetStatusOfBink(CStr sTextureName, int &CurrentFrame, int &NumFrames)
{
	MAUTOSTRIP(CMWinMoviewController_iGetStatusOfBink, 0);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	
	if (!pTC)
	{
		m_iRenderMode = 0;
		return 0;
	}
	
	// Set properties
	
	int _TextureID = pTC->GetTextureID(sTextureName);
	if(_TextureID > 0)
	{
		CTextureContainer_Video* pTCVideo = safe_cast<CTextureContainer_Video>(pTC->GetTextureContainer(_TextureID));
		
		if(pTCVideo)
		{
			int iLocal = pTC->GetLocal(_TextureID);

			CurrentFrame = pTCVideo->GetFrame(iLocal);
			NumFrames = pTCVideo->GetNumFrames(iLocal);
			
			if (pTCVideo->IsOnLastFrame(iLocal))
			{
				return 1;
			}

			return 0;
		}
	}
	
	return 1;
}

void CMWinMoviewController::vPause(CStr sTextureName, bool doPause)
{
	MAUTOSTRIP(CMWinMoviewController_vPause, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	
	if (!pTC)
	{
		m_iRenderMode = 0;
		return;
	}
	
	// Set properties
	
	int _TextureID = pTC->GetTextureID(sTextureName);
	if(_TextureID > 0)
	{
		CTextureContainer_Video* pTCVideo = safe_cast<CTextureContainer_Video>(pTC->GetTextureContainer(_TextureID));
		if(pTCVideo)
		{
			int iLocal = pTC->GetLocal(_TextureID);
			
			pTCVideo->Pause(iLocal,doPause);
		}
	}
}

void CMWinMoviewController::vCloseVideo(CStr sTextureName)
{
	MAUTOSTRIP(CMWinMoviewController_vCloseVideo, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	
	if (!pTC)
	{
		m_iRenderMode = 0;
		return;
	}
/*
	// Disable
	return;
*/
	// Set properties
	int _TextureID = pTC->GetTextureID(sTextureName);
	if (_TextureID > 0)
	{
		CTextureContainer_Video* pTCVideo = safe_cast<CTextureContainer_Video>(pTC->GetTextureContainer(_TextureID));
		if(pTCVideo)
		{
			int iLocal = pTC->GetLocal(_TextureID);
			pTCVideo->CloseVideo(iLocal);
		}
	}
}

void CMWinMoviewController::SetVolume(CStr sTextureName, fp32 fpVolume)
{
	MAUTOSTRIP(CMWinMoviewController_SetVolume, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	
	if (!pTC)
	{
		m_iRenderMode = 0;
		return;
	}
	
	// Set properties
	
	int _TextureID = pTC->GetTextureID(sTextureName);
	if(_TextureID > 0)
	{
		CTextureContainer_Video* pTCVideo = safe_cast<CTextureContainer_Video>(pTC->GetTextureContainer(_TextureID));
		if(pTCVideo)
		{
			int iLocal = pTC->GetLocal(_TextureID);
			
			pTCVideo->SetVolume(iLocal, fpVolume);
		}
	}
}


void CMWinMoviewController::SetFrame(CStr _TextureName, int _Frame)
{
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (pTC)
	{
		int TextureID = pTC->GetTextureID(_TextureName);
		if (TextureID > 0)
		{
			CTextureContainer_Video* pTCVideo = safe_cast<CTextureContainer_Video>(pTC->GetTextureContainer(TextureID));
			if (pTCVideo)
			{
				int iLocal = pTC->GetLocal(TextureID);
				pTCVideo->SetFrame(iLocal, _Frame);
			}
		}
	}
}


void CMWinMoviewController::vStartFading(CStr sTextureName, bool bPause)
{
	MAUTOSTRIP(CMWinMoviewController_vStartFading, MAUTOSTRIP_VOID);
//	m_StartFadeTime = CMTime::GetCPU();
	m_StartFadeTime.Snapshot();
	m_bFinishedFading = false;

	if (sTextureName.Len())
	{
		m_FadeSoundVideo = sTextureName;
	}

	m_bIsFading = true;
}

void CMWinMoviewController::vDrawFading(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, CRC_Viewport *_pViewPort)
{
	MAUTOSTRIP(CMWinMoviewController_vDrawFading, MAUTOSTRIP_VOID);
	const fp32 fpFadeTime = 0.4f;

	CMTime TCurrent;
	TCurrent.Snapshot();
	fp32 fpTimeElapsed = (TCurrent - m_StartFadeTime).GetTime();

	fp32 fpBlend = (fpTimeElapsed/fpFadeTime) * 255.0f;

	if (fpBlend > 255.0f)
	{
		if (!m_bFinishedFading && m_FadeSoundVideo.Len())
		{
			vCloseVideo(m_FadeSoundVideo);
		}

		m_bFinishedFading = true;
		fpBlend = 255.0f;
	}

	if (m_FadeSoundVideo.Len())
		SetVolume(m_FadeSoundVideo, 1.0f - fpBlend/255.0f);

	int iBlend = (int)fpBlend;

	int pix32 = CPixel32(0,0,0,iBlend);
	
	if (_pViewPort) 
	{
		CVec2Dfp32 border = 0;
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");

//		const fp32 Border = pSys->GetEnvironment()->GetValuef("VID_BORDERY");

		CRC_Viewport vp = *_pViewPort;
		CRct ViewRect(0, 0, vp.GetViewRect().GetWidth(), vp.GetViewRect().GetHeight());//(0, 60, 640, 420);
		
		vp.SetAspectRatio(1.0f);
		vp.SetFOV(90);

		if (_pRCUtil->GetVBM()->Viewport_Push(&vp))
		{
			CRC_Util2D TempUtil;
			TempUtil.Begin(_pRCUtil->GetRC(), &vp, _pRCUtil->GetVBM());
			TempUtil.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			TempUtil.SetTexture(0);
			TempUtil.Rect( ViewRect, ViewRect, pix32);
			TempUtil.End();
			
			_pRCUtil->GetVBM()->Viewport_Pop();
		}
	}
	else
	{
		_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		_pRCUtil->SetTexture(0);
		_pRCUtil->Rect( _Clip, CRct(0, 0, 640, 480), pix32);
	}
}

void CMWinMoviewController::vDrawFadingUp(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, CRC_Viewport *_pViewPort)
{
	MAUTOSTRIP(CMWinMoviewController_vDrawFadingUp, MAUTOSTRIP_VOID);

	const fp32 fpFadeTime = 0.4;

	CMTime TCurrent;
	TCurrent.Snapshot();
	fp32 fpTimeElapsed = (TCurrent - m_StartFadeTime).GetTime();

	fp32 fpBlend = 255.0f - (fpTimeElapsed/fpFadeTime) * 255.0f;

	if (fpBlend < 0.0f)
	{
		m_bFinishedFading = true;
		fpBlend = 0.0f;
	}

	int iBlend = (int)fpBlend;

	int pix32 = CPixel32(0,0,0,iBlend);
	
	if (_pViewPort) 
	{
		CVec2Dfp32 border = 0;
		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");

//		const fp32 Border = pSys->GetEnvironment()->GetValuef("VID_BORDERY");

		CRC_Viewport vp = *_pViewPort;
		CRct ViewRect(0, 0, vp.GetViewRect().GetWidth(), vp.GetViewRect().GetHeight());//(0, 60, 640, 420);
		
		vp.SetAspectRatio(1.0f);
		vp.SetFOV(90);

		if (_pRCUtil->GetVBM()->Viewport_Push(&vp))
		{
			CRC_Util2D TempUtil;
			TempUtil.Begin(_pRCUtil->GetRC(), &vp, _pRCUtil->GetVBM());
			TempUtil.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			TempUtil.SetTexture(0);
			TempUtil.Rect(ViewRect, ViewRect, pix32);
			TempUtil.End();
			
			_pRCUtil->GetVBM()->Viewport_Pop();
		}
	}
	else
	{
		_pRCUtil->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		_pRCUtil->SetTexture(0);
		_pRCUtil->Rect(_Clip, CRct(0, 0, 640, 480), pix32);
	}
}

void CMWinMoviewController::vFadeDownLastFrameFadeUp(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, CRC_Viewport *_pViewPort)
{
	MAUTOSTRIP(CMWinMoviewController_vFadeDownLastFrameFadeUp, MAUTOSTRIP_VOID);
	if (m_iSkipFadeMode == 0)
	{
		vDrawFading(_pRCUtil, _Clip, _Client, _pViewPort);

		if (m_bFinishedFading)
		{
			m_iSkipFadeMode = 1;
			m_bFinishedFading = false;
			m_bIsFading = false;
//			m_StartFadeTime = CMTime::GetCPU();
			m_StartFadeTime.Snapshot();
			vFastRewindBINK(m_FadeSoundVideo,false,true,false);
		}	
	}

	if (m_iSkipFadeMode == 1)
	{
		vDrawFadingUp(_pRCUtil, _Clip, _Client, _pViewPort);

		if (m_bFinishedFading)
		{
			m_iRenderMode = m_iNextRenderMode;
			m_bFinishedFading = false;
			m_bIsFading = false;
			m_bIsSkipFading = false;
		}
	}
}


void CMWinMoviewController::vStartFadeDownLastFrameFadeUp(CStr strTexture, int iNextRenderMode)
{
	MAUTOSTRIP(CMWinMoviewController_vStartFadeDownLastFrameFadeUp, MAUTOSTRIP_VOID);
	if (m_bIsSkipFading)
		return;
	
	m_iNextRenderMode = iNextRenderMode;

//	m_StartFadeTime = CMTime::GetCPU();
	m_StartFadeTime.Snapshot();

	m_iSkipFadeMode = 0;

	m_FadeSoundVideo = strTexture;

	m_bFinishedFading = false;
	m_bIsFading = false;
	m_bIsSkipFading = true;
}


void CMWinMoviewController::vGetMovieSize(CStr sTextureName, int &width, int &height)
{
	MAUTOSTRIP(CMWinMoviewController_vGetMovieSize, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	
	if (!pTC)
	{
		m_iRenderMode = 0;
		return;
	}

	// Set properties
	
	int _TextureID = pTC->GetTextureID(sTextureName);
	if(_TextureID > 0)
	{
		CTextureContainer_Video* pTCVideo = safe_cast<CTextureContainer_Video>(pTC->GetTextureContainer(_TextureID));
		if(pTCVideo)
		{
			int iLocal = pTC->GetLocal(_TextureID);
			
			width = pTCVideo->GetWidth(iLocal);
			height = pTCVideo->GetHeight(iLocal);
		}
	}
}


fp32 CMWinMoviewController::GetMovieTime(const char* _pTextureName)
{
	MAUTOSTRIP(CMWinMoviewController_vGetMovieSize, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	
	if(!pTC)
		return 0;

	// Set properties
	
	int _TextureID = pTC->GetTextureID(_pTextureName);
	if(_TextureID <= 0)
		return 0;

	CTextureContainer_Video* pTCVideo = safe_cast<CTextureContainer_Video>(pTC->GetTextureContainer(_TextureID));
	if(!pTCVideo)
		return 0;

	int iLocal = pTC->GetLocal(_TextureID);
	return pTCVideo->GetTime(iLocal);
};

