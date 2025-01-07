
#include "PCH.h"

#include "WData.h"
#include "WDataRes_Core.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWResource
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWResource::CWResource()
{
	MAUTOSTRIP(CWResource_ctor, MAUTOSTRIP_VOID);
	m_Flags = 0;
	m_iRcClass = 0;
	m_iRc = 0;
//	m_TouchTime = 0;
	m_MemUsed = 0;
}

bool CWResource::Create(CWorldData* _pWData, const char* _pName, CMapData* _pMapData, int _iRcClass)
{
	MAUTOSTRIP(CWResource_Create, false);
#ifndef	M_RTM
	if( _pName && strlen( _pName ) > (WPACKET_MAXPACKETSIZE-4) )
	{
		CStr Msg = CStrF( "CWResource::Create Resource name is to long %d (max is %d) '%s'", strlen( _pName ), (WPACKET_MAXPACKETSIZE-4), _pName );
		ConOut( Msg );
		LogFile( Msg );
		m_Name.Capture( _pName, (WPACKET_MAXPACKETSIZE-4) );
		m_iRcClass = _iRcClass;
		m_MemUsed = 0;

		return true;
	}
#endif
	m_Name = _pName;
	m_iRcClass = _iRcClass;
	m_MemUsed = 0;
	return true;
}

void CWResource::OnLoad()
{
	MAUTOSTRIP(CWResource_OnLoad, MAUTOSTRIP_VOID);
}

void CWResource::OnUnload()
{
	MAUTOSTRIP(CWResource_OnUnload, MAUTOSTRIP_VOID);
}

void CWResource::OnPrecache(class CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CWResource_OnPrecache, MAUTOSTRIP_VOID);
}

void CWResource::OnPostPrecache(class CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CWResource_OnPrecache, MAUTOSTRIP_VOID);
}

void CWResource::OnRefresh()
{
	MAUTOSTRIP(CWResource_OnRefresh, MAUTOSTRIP_VOID);
}

void CWResource::OnHibernate()
{
	MAUTOSTRIP(CWResource_OnHibernate, MAUTOSTRIP_VOID);
}

void CWResource::OnRegisterMapData(class CWorldData* _pWData, class CMapData* _pMapData)
{
	MAUTOSTRIP(CWResource_OnRegisterMapData, MAUTOSTRIP_VOID);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWorldData
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT(CWorldData, CConsoleClient);

