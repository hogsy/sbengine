#ifndef __MWINMOVEICONTROLLER_H
#define __MWINMOVEICONTROLLER_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Helper class for controlling movies in windows

	Author:			Hans Andersson

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CMWinMoviewController
\*____________________________________________________________________________________________*/

#include "../../MSystem/Misc/MConsole.h"

class CRC_Util2D;
class CRC_Viewport;

class CMWinMoviewController
{
public:

	CMWinMoviewController()
	{
		m_iRenderMode = 0;
		m_iForegroundRenderMode = 0;
		m_bIsFading = false;
		m_iNextRenderMode = 0;
		m_bIsSkipFading = false;
	}
	
	// ConExecute (helper)
	
	bool bConExecute(CStr _Script)
	{
		MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
		if (!pCon) return false;
		bool bException = false;
		M_TRY
		{
			pCon->ExecuteString(_Script);
		}
		M_CATCH(
		catch(CCException)
		{
			bException = true;
		}
		)
		return !bException;
	}

	// Exit

	void vOutroExit()
	{
		bConExecute(m_OutroScript);
	}

	// Movie attributes

	CMTime	m_StillStartTime;
	int		m_iRenderMode;
	int		m_iForegroundRenderMode;
	CStr	m_OutroScript;
	
	// Fade

	bool	m_bIsFading;
	bool	m_bFinishedFading;
	CStr	m_FadeSoundVideo;
	CMTime	m_StartFadeTime;

	// Skip fading

	bool	m_bIsSkipFading;
	int		m_iNextRenderMode;
	int		m_iSkipFadeMode;

	// Movie functions

	void vFastRewindBINK(CStr sTextureName, bool _bLoop, bool _bLastFrame, bool _bPaused);
	int iGetStatusOfBink(CStr sTextureName, int &CurrentFrame, int &NumFrames);
	void vPause(CStr sTextureName, bool doPause);
	void vCloseVideo(CStr sTextureName);
	void vGetMovieSize(CStr sTextureName, int &width, int &height);
	fp32  GetMovieTime(const char* _pTextureName);

	void SetVolume(CStr sTextureName, fp32 fpVolume);
	void SetFrame(CStr _TextureName, int _Frame);

	// End/begin Fading functions

	void vStartFading(CStr sTextureName, bool bPause);
	void vDrawFading(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, CRC_Viewport *_pViewPort);
	void vDrawFadingUp(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, CRC_Viewport *_pViewPort);

	// Skip fading

	void vFadeDownLastFrameFadeUp(CRC_Util2D* _pRCUtil, const CClipRect &_Clip, const CRct &_Client, CRC_Viewport *_pViewPort);
	void vStartFadeDownLastFrameFadeUp(CStr strTexture, int iNextRenderMode);
};

#endif 
