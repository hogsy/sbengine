#ifndef __WDATARES_MISCMEDIA_H
#define __WDATARES_MISCMEDIA_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Misc resources classes

	Author:			Magnus Högdahl

	Maintainer:		Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWRes_Surface
					CWRes_XFC
\*____________________________________________________________________________________________*/

#include "WData.h"
#include "../Render/MRenderUtil.h"

// -------------------------------------------------------------------
//  SURFACE
// -------------------------------------------------------------------
class CWRes_Surface : public CWResource
{
	MRTC_DECLARE;

	int m_SurfaceID;

public:
	CWRes_Surface();
	virtual CXW_Surface* GetSurface();
	virtual int GetSurfaceID() { return m_SurfaceID; }
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
	virtual void OnLoad();
	virtual void OnPrecache(class CXR_Engine* _pEngine);
};

// -------------------------------------------------------------------
//  XFC
// -------------------------------------------------------------------
class CWRes_XFC : public CWResource
{
	MRTC_DECLARE;

	spCRC_Font m_spFont;

public:
	CWRes_XFC();
	virtual CRC_Font* GetFont();
	virtual bool Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass);
};


#endif // _INC_WDATARES_MISCMEDIA
