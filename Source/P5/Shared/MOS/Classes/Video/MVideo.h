#ifndef __MVIDEO_H
#define __MVIDEO_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Interface for capturing video

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CVideoFile
\*____________________________________________________________________________________________*/

#if !defined( PLATFORM_PS2 ) && !defined( PLATFORM_DOLPHIN )

#include "MRTC.h"
#include "../../MSystem/Raster/MImage.h"

class CVideoFile : public CReferenceCount
{
public:
	virtual bool Create(CStr _FileName, int _Width, int _Height, fp32 _FrameRate, int _KeyFrame = 1) pure;
	virtual bool AddFrame(CImage* _pImg) pure;
	virtual bool AddAudio(class CWaveform *_pWave) { return false; }
	virtual void Close() pure;
};

typedef TPtr<CVideoFile> spCVideoFile;

spCVideoFile MCreateVideoFile();

#endif

#endif
