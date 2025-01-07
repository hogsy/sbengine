#include "PCH.h"
#include "WObj_Params.h"
#include "../WRPG/WRPGCore.h"
#include "../WObj_Char.h"
#include "../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Game.h"

//-------------------------------------------------------------------
//- CWO_CameraShake ----------------------------------------------------
//-------------------------------------------------------------------

void CWO_CameraShake::Clear()
{
	MAUTOSTRIP(CWO_CameraShake_Clear, MAUTOSTRIP_VOID);
	m_Range = 0;
	m_Magnitude = 0;
	m_Duration = 0;
	m_Speed = 0;
	m_bNoRumble = false;
}

//-------------------------------------------------------------------

void CWO_CameraShake::Parse(const char* _Str)
{
	MAUTOSTRIP(CWO_CameraShake_Parse, MAUTOSTRIP_VOID);
	CFStr Str = _Str;

	m_Range = Str.GetStrSep(",").Val_fp64();

	if (Str != "")
	{
		m_Magnitude = Str.GetStrSep(",").Val_fp64();

		if (Str != "")
		{
			m_Duration = Str.GetStrSep(",").Val_fp64();

			if (Str != "")
				m_Speed = Str.GetStrSep(",").Val_fp64();
		}
	}
}

//-------------------------------------------------------------------

bool CWO_CameraShake::IsValid()
{
	MAUTOSTRIP(CWO_CameraShake_IsValid, false);
	return ((m_Range > 0) && (m_Magnitude > 0) && (m_Duration > 0) && (m_Speed > 0));
}

//-------------------------------------------------------------------

static void SendToSelection(CVec3Dfp32 _Center, fp32 _Radius, CWObject_Message& _Msg, CWorld_Server* _pWServer, int _iExclude)
{
	MAUTOSTRIP(SendToSelection, MAUTOSTRIP_VOID);
	TSelection<CSelection::LARGE_BUFFER> Selection;
	_pWServer->Selection_AddBoundBox(Selection, OBJECT_FLAGS_PHYSOBJECT, _Center - CVec3Dfp32(_Radius), _Center + CVec3Dfp32(_Radius));
	if (_iExclude != 0)
		_pWServer->Selection_RemoveOnIndex(Selection, _iExclude);
	_pWServer->Message_SendToSelection(_Msg, Selection);
}

static void SendToPlayersAt(CVec3Dfp32 _Center, fp32 _Radius, CWObject_Message& _Msg, CWorld_Server* _pWServer, int _iExclude = 0)
{
	MAUTOSTRIP(SendToPlayersAt, MAUTOSTRIP_VOID);
	//Send to all players within distance
	CWObject * pPlayer;
	fp32 RadiusSqr = Sqr(_Radius);
	CWObject_Game *pGame = _pWServer->Game_GetObject();
	for (int i = 0; i < pGame->Player_GetNum(); i++)
	{
		pPlayer = pGame->Player_GetObject(i);
		if (pPlayer && 
			(pPlayer->m_iObject != _iExclude) &&
			(pPlayer->GetPosition().DistanceSqr(_Center) <= RadiusSqr))
		{
			_pWServer->Message_SendToObject(_Msg, pPlayer->m_iObject);
		}
	}
};

//-------------------------------------------------------------------

void CWO_CameraShake::Send(CVec3Dfp32 _Center, int _iSender, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CWO_CameraShake_Send, MAUTOSTRIP_VOID);
	if (!IsValid())
		return;

	CWObject_Message Msg(OBJMSG_CHAR_SHAKECAMERA);

	Msg.m_pData = this;
	Msg.m_DataSize = sizeof(CWO_CameraShake);
	Msg.m_VecParam0 = _Center;
	Msg.m_iSender = _iSender;
	Msg.m_Param0 = 1;

	//This should always use "SendToPlayersAt" but must currently use "SendToSelection" when possible,
	//so that there's no chance things don't get fucked up
	if (m_Range > 256)
		SendToPlayersAt(_Center, m_Range, Msg, _pWServer);
	else
		SendToSelection(_Center, m_Range, Msg, _pWServer, 0);
}

//-------------------------------------------------------------------

void CWO_CameraShake::Merge(CWO_CameraShake& _DPrimary, CWO_CameraShake& _DSecondary, CWO_CameraShake& _DResult)
{
	MAUTOSTRIP(CWO_CameraShake_Merge, MAUTOSTRIP_VOID);
	// FIXME: Not yet implmented!
	_DResult = _DPrimary;
}

//-------------------------------------------------------------------
