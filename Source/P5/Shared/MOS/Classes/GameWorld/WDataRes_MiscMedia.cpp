
#include "PCH.h"

#include "WDataRes_MiscMedia.h"

MRTC_IMPLEMENT_DYNAMIC(CWRes_Surface, CWResource);
MRTC_IMPLEMENT_DYNAMIC(CWRes_XFC, CWResource);

//#define LOG_SND_INCLUDE

//#define DebugLog(s) LogFile(s)
#define DebugLog(s)

// -------------------------------------------------------------------
//  SURFACE
// -------------------------------------------------------------------
CWRes_Surface::CWRes_Surface()
{
	MAUTOSTRIP(CWRes_Surface_ctor, MAUTOSTRIP_VOID);
	m_SurfaceID = 0;
}

CXW_Surface* CWRes_Surface::GetSurface()
{
	MAUTOSTRIP(CWRes_Surface_GetSurface, NULL);
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	return (pSC) ? pSC->GetSurface(m_SurfaceID) : NULL;
}

bool CWRes_Surface::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_Surface_Create, false);
	MSCOPE(CWRes_Surface::Create, RES_SURFACE);
	
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC) return false;

	int Len = CStrBase::StrLen(_pName);
	if (Len < 5) return false;

	m_SurfaceID = pSC->GetSurfaceID(&_pName[4]);

	return true;
}

void CWRes_Surface::OnLoad()
{
	MAUTOSTRIP(CWRes_Surface_OnLoad, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC) return;

	CXW_Surface *pSurf = pSC->GetSurface(m_SurfaceID);
	if(pSurf)
	{
		pSurf->InitTextures(false);	// Don't report failures.
	}
}

void CWRes_Surface::OnPrecache(class CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CWRes_Surface_OnPrecache, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC) return;

	CXW_Surface *pSurf = pSC->GetSurface(m_SurfaceID);
	if(pSurf)
	{
		pSurf->InitTextures(false);	// Don't report failures.
		pSurf->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	}
}

// -------------------------------------------------------------------
//  XFC
// -------------------------------------------------------------------
CWRes_XFC::CWRes_XFC()
{
	MAUTOSTRIP(CWRes_XFC_ctor, MAUTOSTRIP_VOID);
}

CRC_Font* CWRes_XFC::GetFont()
{
	MAUTOSTRIP(CWRes_XFC_GetFont, NULL);
	return m_spFont;
}

bool CWRes_XFC::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWRes_XFC_Create, false);
	MSCOPE(CWRes_XFC::Create, RES_XFC);
	
	if (!CWResource::Create(_pWData, _pName, _pMapData, _iRcClass)) return false;
//		ConOut("        " + _Name);

	CStr FileName = CStr(_pName).Copy(4, 100000);
	FileName.UpperCase();
	if (FileName.Find(".XFC") < 0) FileName += ".XFC";
	FileName = _pWData->ResolveFileName("FONTS\\" + FileName);
	if (!CDiskUtil::FileExists(FileName)) return false;
	m_spFont = MNew(CRC_Font);
	if (!m_spFont) MemError("Create");
	m_spFont->ReadFromFile(FileName);
	m_Name = _pName;
	return true;
}


