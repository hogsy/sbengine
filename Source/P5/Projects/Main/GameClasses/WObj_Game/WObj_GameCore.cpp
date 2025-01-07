#include "PCH.h"
#include "WObj_GameCore.h"
#include "../../GameWorld/WClientMod.h"
#include "../../GameWorld/WServerMod.h"
#include "../WRPG/WRPGCore.h"
#include "../WObj_Char.h"
#include "../WObj_AI/AI_Knowledgebase.h"
#include "../WObj_Misc/WObj_Room.h"
#include "../WObj_Misc/WObj_ScenePoint.h"
#include "../WObj_Misc/WObj_Shell.h"
#include "../WObj_Misc/WObj_Object.h"
#include "../WAG2_ClientData_Game.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PhysCluster.h"
#include "../../../../Shared/MOS/Classes/GameContext/WGameContext.h"  //TEMP!

#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PhysCluster.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_GameCore, CWObject_Game, 0x0100);

bool CWObject_GameCore::m_sbRenderSkipText = false;

//Get hash without spaces in front and back
uint32 CleanHash(const char * _pStr)
{
	char lBuf[128];
	uint8 Ln = CStrBase::StrLen(_pStr);

	if( Ln >= 128 ) return StringToHash(_pStr);

	uint8 iFirst = 0;
	while( _pStr[iFirst] == ' ' ) iFirst++;

	if( iFirst == Ln ) return 0;

	memcpy(lBuf,_pStr+iFirst,Ln+1-iFirst);
	Ln -= iFirst;

	uint8 iLast = Ln-1;
	while( lBuf[iLast] == ' ' ) iLast--;
	lBuf[iLast+1] = '\0';
	return StringToHash(lBuf);
}

//Parse 'angles' stuff
M_FORCEINLINE CMat4Dfp32 ParseRotationKey(const CStr & _Str)
{
	CMat4Dfp32 Mat;
	CVec3Dfp32 v; v.ParseString(_Str);
	v *= (1.0f/360.0f);
	v.CreateMatrixFromAngles(0, Mat);
	return Mat;
}

class CRagDollBpt
{
public:

	class CBody
	{
	public:
		uint32		m_iType:8;
		uint32		m_iBone:24;
		CMat4Dfp32	m_Transform,m_InvTransform;
		CVec3Dfp32	m_Dim;
	};

	class CConstraint
	{
	public:
		uint8		m_iType;
		CMat4Dfp32	m_Transform;
		CVec4Dfp32	m_Param;
		uint16		m_iSub1,m_iSub2;
	};

	TThinArray<CBody>		m_lBodies;
	TThinArray<CConstraint>	m_lConstraints;

	uint32				m_Hash;

	uint32 Parse(const CRegistry * _pReg)
	{
		//Get number of constraints or bodies
		uint32 nB = 0,nC = 0;
		uint i;
		for(i = 0;i < _pReg->GetNumChildren();i++)
		{
			uint32 Hs = _pReg->GetChild(i)->GetThisNameHash();

			if( Hs == MHASH3('PRIM','ITIV','E') ) nB++;
			else if( Hs == MHASH2('JOIN','T') ) nC++;
		}

		uint32 Ret = MHASH2('DEFA','ULT');

#ifndef M_RTM

		//Error check
		if( !nB )
		{
			ConOutL(CStrF("ERROR: Rigid body %s has no bodies!",_pReg->GetThisName().Str()));
			return 0;
		}
		if( !nC )
			ConOutL(CStrF("WARNING: Rigid body %s has no constraints!",_pReg->GetThisName().Str()));

#endif

		m_lBodies.SetLen(nB);
		m_lConstraints.SetLen(nC);
		TThinArray<uint32>		lBodyHash;
		TThinArray<uint32>		lConstraintHash;
		lBodyHash.SetLen(nB);
		lConstraintHash.SetLen(nC * 2);

		nB = 0;
		nC = 0;

		for(i = 0;i < _pReg->GetNumChildren();i++)
		{
			const CRegistry *pReg= _pReg->GetChild(i);

			switch( pReg->GetThisNameHash() )
			{

			case MHASH1('NAME'):
				Ret = StringToHash(_pReg->GetThisValue());
				break;

			case MHASH3('PRIM','ITIV','E'):
				{
					CBody &Bd = m_lBodies[nB];
					Bd.m_Transform.Unit();
					CVec3Dfp32 Mov(0);

					for(uint j = 0;j < pReg->GetNumChildren();j++)
					{
						const CRegistry * pChild = pReg->GetChild(j);
						CStr Val = pChild->GetThisValue();

						switch( pChild->GetThisNameHash() )
						{

						case MHASH3('TARG','ETNA','ME'):
							lBodyHash[nB] = CleanHash(Val);
							break;

						case MHASH1('TYPE'):
							Bd.m_iType = Val.Val_int();
							break;

						case MHASH2('LENG','TH'):
							Bd.m_Dim.k[0] = Val.Val_fp64();
							break;

						case MHASH2('RADI','US'):
							Bd.m_Dim.k[1] = Val.Val_fp64();
							break;

						case MHASH2('BONE','ID'):
							Bd.m_iBone = Val.Val_int();
							break;

						case MHASH2('ORIG','IN'):
							Mov.ParseString(Val);
							break;

						case MHASH3('DIME','NSIO','NS'):
							Bd.m_Dim.ParseString(Val);
							break;

						case MHASH2('ANGL','ES'):
						case MHASH2('ROTA','TION'):
							Bd.m_Transform = ParseRotationKey(Val);
							break;

						}
					}

					Bd.m_Transform.GetRow(3) = Mov;
					nB++;
					/*
					CBody &Bd = m_lBodies[nB];
					lBodyHash[nB] = CleanHash(Val.GetStrSep(","));
					nB++;

					Bd.m_iType = OBJECT_PRIMTYPE_CAPSULE;
					Bd.m_iBone = Val.GetStrSep(",").Val_int();
					
					Bd.m_Transform.k[0][3] = Bd.m_Transform.k[1][3] = Bd.m_Transform.k[2][3] = 0;
					Bd.m_Transform.k[3][3] = 1.0f;

					//Points
					CVec3Dfp32 Pt1,Pt2;
					Pt1.k[0] = Val.GetStrSep(",").Val_fp64();
					Pt1.k[1] = Val.GetStrSep(",").Val_fp64();
					Pt1.k[2] = Val.GetStrSep(",").Val_fp64();
					Pt2.k[0] = Val.GetStrSep(",").Val_fp64();
					Pt2.k[1] = Val.GetStrSep(",").Val_fp64();
					Pt2.k[2] = Val.GetStrSep(",").Val_fp64();
					Bd.m_Transform.GetRow(3) = Pt1;//(Pt1 + Pt2) * 0.5f;
					Bd.m_Transform.GetRow(0) = Pt2.Normalize();//(Pt2 - Pt1);

					//Complete matrix
					CVec3Dfp32 Up,Right;
					if( Abs(Bd.m_Transform.k[0][2]) > 0.99f )
						Right = Pt2 / CVec3Dfp32(0.0f,1.0f,0.0f);
					else
						Right = Pt2 / CVec3Dfp32(0.0f,0.0f,1.0f);
					Up = Pt2 / Right;
					Bd.m_Transform.GetRow(1) = Right.Normalize();
					Bd.m_Transform.GetRow(2) = Up.Normalize();

					//Length - radius
					Bd.m_Dim.k[0] = Val.GetStrSep(",").Val_fp64();
					Bd.m_Dim.k[1] = Val.GetStrSep(",").Val_fp64();
					*/
				}
				break;

			case MHASH2('JOIN','T'):
				{
					CConstraint &Cn = m_lConstraints[nC];
					Cn.m_Transform.Unit();
					CVec3Dfp32 Mov(0);
					bool bHasMin = false;

					for(uint j = 0;j < pReg->GetNumChildren();j++)
					{
						const CRegistry * pChild = pReg->GetChild(j);
						CStr Val = pChild->GetThisValue();

						switch( pChild->GetThisNameHash() )
						{

						case MHASH2('TARG','ET1'):
							lConstraintHash[nC * 2] = CleanHash(Val);
							break;

						case MHASH2('TARG','ET2'):
							lConstraintHash[nC * 2 + 1] = CleanHash(Val);
							break;

						case MHASH1('TYPE'):
							Cn.m_iType = Val.Val_int();
							break;

						case MHASH2('LENG','TH'):
							Cn.m_Param.k[0] = Val.Val_fp64();
							break;

						case MHASH2('MAXA','NGLE'):
							Cn.m_Param.k[1] = Val.Val_fp64();
							break;

						case MHASH2('MINA','NGLE'):
							Cn.m_Param.k[2] = Val.Val_fp64();
							bHasMin = true;
							break;

						case MHASH2('ORIG','IN'):
							Mov.ParseString(Val);
							break;

						case MHASH2('ANGL','ES'):
						case MHASH2('ROTA','TION'):
							Cn.m_Transform = ParseRotationKey(Val);
							break;
						}
					}

					if( Cn.m_iType == 1 )
						Cn.m_iType = CWD_ConstraintDescriptor::BALL;
					else
						Cn.m_iType = (bHasMin) ? CWD_ConstraintDescriptor::HINGE2 : CWD_ConstraintDescriptor::HINGE;
					Cn.m_Transform.GetRow(3) = Mov;
					nC++;
				}
				break;
/*
			case MHASH3('BALL','JOIN','T'):
				{
					CConstraint &Cn = m_lConstraints[nC];
					
					Cn.m_iType = CWD_ConstraintDescriptor::BALL;
					lConstraintHash[nC * 2] = CleanHash(Val.GetStrSep(","));
					lConstraintHash[nC * 2+1] = CleanHash(Val.GetStrSep(","));
					nC++;

					//Position
					Cn.m_Transform.k[3][0] = Val.GetStrSep(",").Val_fp64();
					Cn.m_Transform.k[3][1] = Val.GetStrSep(",").Val_fp64();
					Cn.m_Transform.k[3][2] = Val.GetStrSep(",").Val_fp64();

					//MaxAngle
					Cn.m_Param.k[0] = Val.GetStrSep(",").Val_fp64();
				}
				break;

			case MHASH3('HING','EJOI','NT'):
				{
					CConstraint &Cn = m_lConstraints[nC];

					Cn.m_iType = CWD_ConstraintDescriptor::HINGE;
					lConstraintHash[nC * 2] = CleanHash(Val.GetStrSep(","));
					lConstraintHash[nC * 2+1] = CleanHash(Val.GetStrSep(","));
					nC++;

					//Position
					Cn.m_Transform.k[3][0] = Val.GetStrSep(",").Val_fp64();
					Cn.m_Transform.k[3][1] = Val.GetStrSep(",").Val_fp64();
					Cn.m_Transform.k[3][2] = Val.GetStrSep(",").Val_fp64();

					//Axis
					Cn.m_Transform.k[2][0] = Val.GetStrSep(",").Val_fp64();
					Cn.m_Transform.k[2][1] = Val.GetStrSep(",").Val_fp64();
					Cn.m_Transform.k[2][2] = Val.GetStrSep(",").Val_fp64();
					Cn.m_Transform.GetRow(2).Normalize();

					//AngleAxis
					Cn.m_Transform.k[0][0] = Val.GetStrSep(",").Val_fp64();
					Cn.m_Transform.k[0][1] = Val.GetStrSep(",").Val_fp64();
					Cn.m_Transform.k[0][2] = Val.GetStrSep(",").Val_fp64();
					Cn.m_Transform.GetRow(0).Normalize();

					//Length - MaxAngle - MinAngle(optional)
					Cn.m_Param.k[0] = Val.GetStrSep(",").Val_fp64();
					Cn.m_Param.k[1] = Val.GetStrSep(",").Val_fp64();

					if( Val.Len() != 0 )
					{
						Cn.m_iType = CWD_ConstraintDescriptor::HINGE2;
						Cn.m_Param.k[2] = Val.GetStrSep(",").Val_fp64();
					}
				}
				break;
				*/
			}
		}

		//Invert matrices
		for(i = 0;i < nB;i++)
		{
			CBody &Bd = m_lBodies[i];
			Bd.m_Transform.InverseOrthogonal(Bd.m_InvTransform);
		}

		//All bodies and constraints created, time to connect them
		for(i = 0;i < nC;i++)
		{
			CConstraint &Cn = m_lConstraints[i];
			
			//Find bodies
			uint32 *pHash = lConstraintHash.GetBasePtr() + i*2;
			uint16 iBodies[2] = {0xFFFF,0xFFFF};
			for(uint j = 0;j < 2;j++)
			{
				for(uint k = 0;k < nB;k++)
				{
					if( pHash[j] == lBodyHash[k] )
					{
						iBodies[j] = k;
						break;
					}
				}
			}
			
#ifndef M_RTM
			//Constraint invalid?
			if( (iBodies[0] == 0xFFFF) || (iBodies[1] == 0xFFFF) )
			{
				Cn.m_iSub1 = 0;
				Cn.m_iSub2 = 0;
				LogFile(CStrF("ERROR: Constraint %d in ragdoll %s has invalid body IDs",i,
					_pReg->GetThisName().Str()));
				continue;
			}
#endif

			Cn.m_iSub1 = iBodies[0];
			Cn.m_iSub2 = iBodies[1];
		}

		return Ret;
	}
};

CWObject_GameCore::CWObject_GameCore()
{
	m_PendingWorldTick = -1;
	m_PendingPause = -1;
	m_iDummyPlayer = -1;
	m_LastWorldOffset = 0;
	m_LastWorldPos.Unit();
	m_bWaitingForChangeWorldScript = false;

	m_pScenePointManager = NULL;
	m_pRoomManager = DNew(CWO_RoomManager) CWO_RoomManager;
	m_pShellManager = DNew(CWO_ShellManager) CWO_ShellManager;
	m_pSoundGroupManager = DNew(CWO_SoundGroupManager) CWO_SoundGroupManager;
}

CWObject_GameCore::~CWObject_GameCore()
{
	m_pScenePointManager = NULL;

	delete m_pRoomManager;
	m_pRoomManager = NULL;

	delete m_pShellManager;
	m_pShellManager = NULL;

	delete m_pSoundGroupManager;
	m_pSoundGroupManager = NULL;
}


CWObject_GameCore::CClientData *CWObject_GameCore::GetClientData(CWObject_CoreData* _pObj)
{
	if(!_pObj->m_lspClientObj[CLIENTOBJ_CLIENTDATA])
	{
		_pObj->m_lspClientObj[CLIENTOBJ_CLIENTDATA] = MNew(CClientData);
		if(!_pObj->m_lspClientObj[CLIENTOBJ_CLIENTDATA])
			Error_static("CWObject_GameCore::GetClientData", "Unable to create client objects.");
	}
	
	return (CWObject_GameCore::CClientData*)(CReferenceCount*)_pObj->m_lspClientObj[CLIENTOBJ_CLIENTDATA];
}

CWObject_GameCore::CClientData *CWObject_GameCore::GetClientData()
{
	return GetClientData(this);
}

const CWObject_GameCore::CClientData *CWObject_GameCore::GetClientData(const CWObject_CoreData* _pObj)
{
	return GetClientData(const_cast<CWObject_CoreData *>(_pObj));
}

int CWObject_GameCore::ReleaseControls(int _iClient)
{
	m_pWServer->NetMsg_SendToClient(CNetMsg(WPACKET_RELEASECONTROLS), _iClient);
	return 0;
}

void CWObject_GameCore::ReleaseControlsAll()
{
	for(int i = 0; i < Player_GetNum(); i++)
	{
		int iClient = Player_GetClient(i);
		if(iClient >= 0)
			ReleaseControls(iClient);
	}
}

int CWObject_GameCore::OnSetClientWindow(int _iClient, const char *_pWnd)
{
	//M_TRACEALWAYS(CStrF("%i. OnSetClientWindow: %s\n", m_pWServer->GetGameTick(), _pWnd));
	CRegistry* pReg = m_pWServer->Registry_GetClient(_iClient, WCLIENT_REG_GAMESTATE);
	if(pReg)
	{
		CFStr Wnd = _pWnd;
		if(Wnd != "")
			ReleaseControls(_iClient);
#ifdef M_DEMO_XBOX
		if(_pWnd != "mission_journal")
			pReg->SetValue("WINDOW", _pWnd);
#else
		pReg->SetValue("WINDOW", _pWnd);
#endif
		return 1;
	}
	else
		return 0;
}

void CWObject_GameCore::SetMusic(const char *_pMusic)
{
	CFStr Music = _pMusic;
	GetClientData()->m_liMusic.Clear();
	GetClientData()->m_liMusicVolumeModifier.Clear();
	while(Music != "")
	{
		GetClientData()->m_liMusic.Add(m_pWServer->GetMapData()->GetResourceIndex_Sound(Music.GetStrSep(",")));
		GetClientData()->m_liMusicVolumeModifier.Add(-1.0f);
	}
	GetClientData()->m_liMusic.MakeDirty();
	GetClientData()->m_liMusicVolumeModifier.MakeDirty();
}

void CWObject_GameCore::ShowGameMsg(const char *_pSt, fp32 _Duration, bool _bFromQueue)
{
	if (!_pSt)
	{
		ConOutL("§cf8WARNING: Show game message with empty message!");
		return;
	}

	int Duration = m_pWServer->GetGameTick() - GetClientData()->m_GameMessage_StartTick;
	bool bWait = (Duration >= 0 && Duration < Max((int)GetClientData()->m_GameMessage_Duration, (int)6));
	if(!_bFromQueue && m_lGameMessages.Len() > 0)
		bWait = true;
	CWO_Player *pPlayer = Player_GetNum() ? Player_Get(0) : NULL;
	if(pPlayer || bWait)
	{
//		CWObject *pObj = pPlayer ? m_pWServer->Object_Get(pPlayer->m_iObject) : NULL;
		if(bWait /*|| (pObj && !CWObject_Character::Char_IsPlayerView(pObj) && _pSt[0] != '$')*/)
		{
			wchar Buf[1024];
			Localize_Str(_pSt, Buf, 1024);
			if(Buf[0] != '#')
			{
				m_lGameMessages.Add((char *)_pSt);
				if(_Duration == 0)
					_Duration = 100000;
				m_lGameMessages_Duration.Add(-_Duration);
				return;
			}
		}
	}

	//_Duration = 0;
	GetClientData()->m_GameMessage_Text.m_Value = _pSt;
	GetClientData()->m_GameMessage_Text.MakeDirty();
	GetClientData()->m_GameMessage_StartTick = m_pWServer->GetGameTick();
	GetClientData()->m_GameMessage_Duration = RoundToInt(_Duration * m_pWServer->GetGameTicksPerSecond());
	if(_Duration == 0 && _pSt[0] != '$')
		m_PendingPause = m_pWServer->GetGameTick() + 6;
}

void CWObject_GameCore::ShowGameSurface(int _iSurface, fp32 _Duration)
{
	GetClientData()->m_GameSurface_Surface = _iSurface;
	GetClientData()->m_GameSurface_StartTick = m_pWServer->GetGameTick();
	GetClientData()->m_GameSurface_Duration = RoundToInt(_Duration * m_pWServer->GetGameTicksPerSecond());
}

void CWObject_GameCore::ShowGameSurfaceOverlay(const char* _pSt, fp32 _Duration)
{
	// No reason to update anything except stop rendering
	if (!_pSt)
	{
		GetClientData()->m_GameSurfaceOverlay_Surface = 0;
		return;
	}

	CStr Key(_pSt);
	GetClientData()->m_GameSurfaceOverlay_StartTick = m_pWServer->GetGameTick();
	GetClientData()->m_GameSurfaceOverlay_Duration = TruncToInt(m_pWServer->GetGameTicksPerSecond() * _Duration);
	GetClientData()->m_GameSurfaceOverlay_Surface = m_pWServer->GetMapData()->GetResourceIndex_Surface(Key.GetStrSep(","));
	if (Key.Len() > 0)
	{
		GetClientData()->m_GameSurfaceOverlay_Type = Key.GetStrSep(",").Val_int();
		uint16 Width = (uint16)Key.GetStrSep(",").Val_int();
		uint16 Height = (uint16)Key.GetStrSep(",").Val_int();
		GetClientData()->m_GameSurfaceOverlay_WH = (uint32(Width) << 16) | uint32(Height);
	}
	else
	{
		GetClientData()->m_GameSurfaceOverlay_Type = 0;
		GetClientData()->m_GameSurfaceOverlay_WH = 0;
	}
}

void CWObject_GameCore::ShowInfoScreen(const char *_pSt, int _Type)
{
	if(_Type == 0)
	{
/*		ConExecute(CStr("showinfoscreen(\"%s\")", _pSt));
		int nClient = m_pWServer->Client_GetCount();
		for(int i = 0; i < nClient; i++)
			OnSetClientWindow(i, "infoscreen");*/
//#ifdef PLATFORM_CONSOLE
		ConExecute(CStrF("cg_rootmenu(\"%s\")", _pSt));
//#endif
	}
	else
	{
		GetClientData()->m_InfoScreen_StartTick = m_pWServer->GetGameTick();
		if(_pSt != NULL)
		{
			GetClientData()->m_InfoScreen_Surface = m_pWServer->GetMapData()->GetResourceIndex_Surface(_pSt);
			GetClientData()->m_InfoScreen_Type = 0;
		}
		else
			GetClientData()->m_InfoScreen_Type = 1;
	}
}

CStr CWObject_GameCore::GetClientWindow(int _iClient)
{
	CRegistry* pReg = m_pWServer->Registry_GetClient(_iClient, WCLIENT_REG_GAMESTATE);
	if(pReg)
		return pReg->GetValue("WINDOW");
	else
		return "";
}

CWObject_GameCore::CClientData::CClientData()
{
	m_FirstTension0Tick = -1;
	m_bRenderWorld = true;
	m_VolumeMultiplier = 1.0f;

	m_MaxCaptures = 3;
	m_StartTick = 0;
	m_MaxTime = 300;
	m_MaxScore = 20;
	m_GameOver = false;
}

uint32 CWObject_GameCore::CClientData::GetCurScreenFade(int32 _GameTick, fp32 _TickFraction)
{
	if(m_ScreenFade_StartTick == 0)
		return m_ScreenFade_DestColor;
	
	fp32 Tick = fp32(_GameTick - int(m_ScreenFade_StartTick)) + _TickFraction;
	if(Tick > m_ScreenFade_Duration)
		return m_ScreenFade_DestColor;
	
	if(Tick < 0)
		return m_ScreenFade_SrcColor;
	
	return CPixel32(m_ScreenFade_DestColor).AlphaBlendRGBA(CPixel32(m_ScreenFade_SrcColor), int(255 * Tick / fp32(m_ScreenFade_Duration)));
}

uint8 CWObject_GameCore::CClientData::GetCurSoundFade(int32 _GameTick, fp32 _TickFraction)
{
	if(m_SoundFade_StartTick == 0)
		return m_SoundFade_Dest;
	
	fp32 Tick = fp32(_GameTick - m_SoundFade_StartTick) + _TickFraction;
	if(Tick > m_SoundFade_Duration)
		return m_SoundFade_Dest;
	
	if(Tick < 0)
		return m_SoundFade_Src;
	
//	return (m_SoundFade_Dest - m_SoundFade_Src) + m_SoundFade_Src * Tick / fp32(m_SoundFade_Duration);

	int Dest = m_SoundFade_Dest;
	int Src = m_SoundFade_Src;
	fp32 Duration = m_SoundFade_Duration;
	return uint8(Src + (Dest - Src) * Tick / Duration);
}

uint16 CWObject_GameCore::CClientData::GetCurNVRange(int32 _GameTick, fp32 _TickFraction)
{
	if(m_NVRange_StartTick == 0)
		return m_NVRange_Dest;

	fp32 Tick = (_GameTick - m_NVRange_StartTick) + _TickFraction;
	if(Tick > m_NVRange_Duration)
		return m_NVRange_Dest;

	if(Tick < 0)
		return m_NVRange_Src;

	int Dest = m_NVRange_Dest;
	int Src = m_NVRange_Src;
	fp32 Duration = m_NVRange_Duration;
	return uint16(Src + (Dest - Src) * Tick / Duration);
}

int CWObject_GameCore::FadeScreen(int _DestColor, fp32 _Duration, int _ExtraTickBeforeStart)
{
	GetClientData()->m_ScreenFade_SrcColor = GetClientData()->GetCurScreenFade(m_pWServer->GetGameTick(), 0);
	GetClientData()->m_ScreenFade_DestColor = (uint32)_DestColor;
	GetClientData()->m_ScreenFade_Duration = RoundToInt(_Duration * m_pWServer->GetGameTicksPerSecond());
	GetClientData()->m_ScreenFade_StartTick = m_pWServer->GetGameTick() + _ExtraTickBeforeStart;
	
	m_DirtyMask |= GetClientData()->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
	return 1;
}

int CWObject_GameCore::FadeSound(int _Dest, fp32 _Duration, int _ExtraTickBeforeStart)
{
	GetClientData()->m_SoundFade_Src = GetClientData()->GetCurSoundFade(m_pWServer->GetGameTick(), 0);
	GetClientData()->m_SoundFade_Dest = (uint32)_Dest;
	GetClientData()->m_SoundFade_Duration = RoundToInt(_Duration * m_pWServer->GetGameTicksPerSecond());
	GetClientData()->m_SoundFade_StartTick = m_pWServer->GetGameTick() + _ExtraTickBeforeStart;
	
	m_DirtyMask |= GetClientData()->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
	return 1;
}

int CWObject_GameCore::SetNVRange(uint16 _Range, fp32 _Duration, int _ExtraTickBeforeStart)
{
	CClientData *pData = GetClientData();
	pData->m_NVRange_Src = GetClientData()->GetCurNVRange(m_pWServer->GetGameTick(), 0);
	pData->m_NVRange_Dest = _Range;
	pData->m_NVRange_Duration = RoundToInt(_Duration * m_pWServer->GetGameTicksPerSecond());
	pData->m_NVRange_StartTick = m_pWServer->GetGameTick() + _ExtraTickBeforeStart;

	m_DirtyMask |= GetClientData()->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
	return 1;
}


void CWObject_GameCore::SetPicmipOffsets(const char* _pPicmipOffsets)
{
	// This is not a nice way to do things. The server should "suggest" picmips to the clients,
	// And the client should run the commands														<-- (that's actually some old text from the SetPicmips() method, which wasn't used and is now replaced with this temporary thing)

	//
	// NOTE: added this to as a quick way of getting NY3_Mansion to load...   final game should not need this!  /anton
	//

	MACRO_GetSystemEnvironment(pEnv);

	// Disable 'rs_preload_textures' to prevent render context from reloading existing textures
	int Old = pEnv->GetValuei("rs_preload_textures", -1);
	pEnv->SetValuei("rs_preload_textures", 0); 

	CFStr Offsets = _pPicmipOffsets;
	for (uint iGroup = 0; !Offsets.IsEmpty(); iGroup++)
	{
		CFStr OffsetKey = CFStrF("R_PICMIP_OFFSET%d", iGroup);
		CFStr PicmipKey = CFStrF("R_PICMIP%d", iGroup);

		int NewPicmipOffset = Offsets.GetStrSep(",").Val_int();
		int CurrPicmipOffset = pEnv->GetValuei(OffsetKey, 0);
		int CurrPicmip = pEnv->GetValuei(PicmipKey, 1);
		int NewPicmip = CurrPicmip - CurrPicmipOffset + NewPicmipOffset;

		if (NewPicmipOffset != 0)
			pEnv->SetValuei(OffsetKey, NewPicmipOffset);
		else
			pEnv->DeleteKey(OffsetKey);

		pEnv->SetValuei(PicmipKey, NewPicmip);
		ConExecute( CFStrF("r_picmip(%i,%i)", iGroup, NewPicmip) );
	}

	// Restore 'rs_preload_textures'
	if (Old == -1)
		pEnv->DeleteKey("rs_preload_textures");
	else
		pEnv->SetValuei("rs_preload_textures", Old);
}


int CWObject_GameCore::OnChangeWorld(const char *_pWorld, int _Flags, int _iSender)
{
	if(m_PendingWorld != "")
		return 0;

	if(GetPlayerStatus() & PLAYER_STATUS_DEAD)
		return 0;

	CWObject *pChar = Player_GetObject(0);
	if(pChar)
		m_LastWorldPos = pChar->GetPositionMatrix();
	else
		m_LastWorldPos.Unit();

	CWObject *pObj = m_pWServer->Object_Get(_iSender);

	m_LastWorldChangeID = _pWorld;
	CStr World = m_LastWorldChangeID.GetStrSep(":");
	if(m_LastWorldChangeID == "" && pObj)
		m_LastWorldChangeID = pObj->GetName();

	m_LastWorldOffset = 0;
	if(pObj && pObj->GetPhysState().m_nPrim && pObj->GetPhysState().m_Prim[0].m_iPhysModel > 0)
	{
		CXR_Model *pModel = m_pWServer->GetMapData()->GetResource_Model(pObj->GetPhysState().m_Prim[0].m_iPhysModel);
		if(pModel)
		{
			CVec3Dfp32 RefPos = pObj->GetPosition();
			CBox3Dfp32 Bound;
			pModel->GetBound_Box(Bound);
			RefPos.k[2] += Bound.m_Min[2];
			m_LastWorldOffset = CVec3Dfp32::GetRow(m_LastWorldPos, 3) - RefPos;
		}
	}

	if(World.GetFilenameExtenstion() == "")
		World += ".xw";
	
	if(_Flags & 1)
	{

		m_PendingWorld = World;
		m_PendingWorldTick = m_pWServer->GetGameTick() + 5;
		ConExecute("hintload()");
		//FadeScreen(0xff000000, 0.15f);
		//FadeSound(0, 0.15f);
	}
	else
	{
#ifdef PLATFORM_CONSOLE
		m_pWServer->World_ChangeFromGame(World, SERVER_CHANGEWORLD_KEEPGAME | SERVER_CHANGEWORLD_MIGRATION);
#else
		m_pWServer->World_ChangeFromGame(World, SERVER_CHANGEWORLD_KEEPGAME | SERVER_CHANGEWORLD_MIGRATION | SERVER_CHANGEWORLD_KEEPRESOURCES);
#endif
		m_bWaitingForChangeWorldScript = true;
	}
	return 1;
}

void CWObject_GameCore::OnCreate()
{
	MSCOPE(CWObject_GameCore::OnCreate, GAMECORE);
	
	CWObject_Game::OnCreate();

	SetNVRange(512, 0);
	m_pWServer->Game_Register(m_iObject);

	// Check if GameName is PB (riddick)
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	if (pGame && pGame->m_GameName.CompareNoCase("PB") == 0)
	{
		CClientData* pCD = GetClientData();
		pCD->m_Flags = pCD->m_Flags | GAME_FLAGS_IS_PBHD;
	}
}

void CWObject_GameCore::OnDestroy()
{
	m_pWServer->Game_Unregister();
}

void CWObject_GameCore::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MSCOPE(CWObject_GameCore::OnEvalKey, GAMECORE);

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	switch (_KeyHash)
	{
	case MHASH3('INCL','UDEC','LASS'): // "INCLUDECLASS"
	case MHASH3('LEVE','LKEY','S'): // "LEVELKEYS"
	case MHASH3('TEMP','LATE','S'): // "TEMPLATES"
	case MHASH3('RPGT','EMPL','ATES'): // "RPGTEMPLATES"
	case MHASH3('GAME','CLAS','S'): // "GAMECLASS"
	case MHASH2('WORL','D'): // "WORLD"
		{
			// Ignore
			break;
		}
	case MHASH2('MUSI','C'): // "MUSIC"
		{
			SetMusic(_pKey->GetThisValue());
			break;
		}

	case MHASH3('MUSI','CVOL','UMES'): // "MUSICVOLUMES"
		{
			CFStr Volumes = _pKey->GetThisValue();
			GetClientData()->m_liMusicVolumes.Clear();
			while(Volumes != "")
				GetClientData()->m_liMusicVolumes.Add(int(Volumes.GetStrSep(",").Val_fp64() * 127));
			GetClientData()->m_liMusicVolumes.MakeDirty();
			break;
		}
	case MHASH4('MUSI','CFAD','ESPE','EDS'): // "MUSICFADESPEEDS"
		{
			CFStr FadeSpeeds = _pKey->GetThisValue();
			GetClientData()->m_lfMusicFadeSpeeds.Clear();
			while(FadeSpeeds != "")
				GetClientData()->m_lfMusicFadeSpeeds.Add(FadeSpeeds.GetStrSep(",").Val_fp64());
			GetClientData()->m_lfMusicFadeSpeeds.MakeDirty();
			break;
		}
	case MHASH5('TENS','ION_','RELE','ASED','ELAY'): // "TENSION_RELEASEDEALY"
		{
			CFStr ReleaseDealy = _pKey->GetThisValue();
			GetClientData()->m_TensionReleaseDelayDuration = TruncToInt(ReleaseDealy.GetStrSep(",").Val_fp64() * m_pWServer->GetGameTicksPerSecond());
			GetClientData()->m_BattleReleaseDelayDuration = TruncToInt(ReleaseDealy.GetStrSep(",").Val_fp64() * m_pWServer->GetGameTicksPerSecond());
			
			GetClientData()->m_TensionReleaseDelayDuration.MakeDirty();
			GetClientData()->m_BattleReleaseDelayDuration.MakeDirty();

			break;
		}
	case MHASH4('PREC','ACHE','_SUR','FACE'): // "PRECACHE_SURFACE"
		{
			while(KeyValue != "")
				m_pWServer->GetMapData()->GetResourceIndex_Surface(KeyValue.GetStrSep(","));
			break;
		}
	case MHASH5('PREC','ACHE','_REG','ISTR','Y'): // "PRECACHE_REGISTRY"
		{
			while(KeyValue != "")
				m_pWServer->GetMapData()->GetResourceIndex_Registry(KeyValue.GetStrSep(","));
			break;
		}
	case MHASH4('PREC','ACHE','_FON','T'): // "PRECACHE_FONT"
		{
			while(KeyValue != "")
				m_pWServer->GetMapData()->GetResourceIndex_Font(KeyValue.GetStrSep(","));
			break;
		}

	case MHASH4('DEFA','ULT_','STAR','T'): // "DEFAULT_START"
		m_DefaultSpawn = KeyValue;
		break;

#ifdef PLATFORM_XBOX1
	case MHASH5('DISA','BLEA','UTOV','IEWS','CALE'): // "DISABLEAUTOVIEWSCALE"
		{
			ConExecute("setASR(0)");
			break;
		}
#endif

	case MHASH4('PIC','MIP_','OFF','SET'): // "PICMIP_OFFSET"
		{
			// This is a hack to allow lower texture quality on some levels.
			SetPicmipOffsets(KeyValue);
		}
		break;

	case MHASH2('RAGD','OLLS'):
		{
			//Create temp list
			TThinArray<uint32>	lHash;
			TThinArray<uint32>	liPos;
			uint32 nChildren = _pKey->GetNumChildren();

			lHash.SetLen(nChildren);
			liPos.SetLen(nChildren);
			uint32 *pHash = lHash.GetBasePtr();
			uint32 *piPos = liPos.GetBasePtr();

			//Get hashed values
			uint i;
			uint nLost = 0;
			for(i = 0;i < nChildren;i++)
			{
				if( _pKey->GetChild(i)->GetThisNameHash() != MHASH2('RAGD','OLL') )
				{
					nLost++;
					continue;
				}

				const CRegistry * pReg = _pKey->GetChild(i)->FindChild("name");
				if( pReg )
					pHash[i] = StringToHash(pReg->GetThisValue());
				else
					pHash[i] = MHASH2('DEFA','ULT');
				piPos[i] = i;
			}
			nChildren -= nLost;

			//Shakersort so we can use bisection when finding specific ragdoll
			{
				bool bDone = false;
#ifndef M_RTM
				bool bReport = false;
#endif
				while(!bDone)
				{
					bDone = true;

					for(i = 0;i < nChildren-1;i++) 
					{
#ifndef M_RTM
						if( (pHash[i] == pHash[i+1]) && (!bReport) )
						{
							LogFile(CStrF("ERROR: Ragdoll name/hash conflict: %d(%s) - %d(%s)",
								piPos[i],_pKey->GetChild(piPos[i])->GetThisName().Str(),
								piPos[i+1],_pKey->GetChild(piPos[i+1])->GetThisName().Str()).Str());
						}
#endif
						if(pHash[i] > pHash[i+1])
						{
							Swap(pHash[i],pHash[i+1]);
							Swap(piPos[i],piPos[i+1]);
							bDone = false;
						}
					}

					for(i = nChildren-1;i > 0;i--) if(pHash[i-1] > pHash[i])
						{
							Swap(pHash[i-1],pHash[i]);
							Swap(piPos[i-1],piPos[i]);
							bDone = false;
						}
				}
			}

			//On to the parsing!
			m_lRagDollTypes.SetLen(nChildren);
			for(i = 0;i < nChildren;i++)
			{
				CRagDollBpt &Rd = m_lRagDollTypes[i];
				Rd.m_Hash = pHash[i];
				const CRegistry * pReg = _pKey->GetChild(piPos[i]);
				Rd.Parse(pReg);
			}

		}
		break;

	default:
		{
			if (KeyName.CompareSubStr("AUTO_IMPULSE") == 0)
			{
				int iSlot = atoi(KeyName.Str() + 12);
				m_lAutoImpulse.SetMinLen(iSlot + 1);
				m_lAutoImpulse[iSlot] = KeyValue;
				return;
			}

		break;

			CWObject_Game::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_GameCore::OnFinishEvalKeys()
{
	MSCOPE(CWObject_GameCore::OnFinishEvalKeys, GAMECORE);

	CWObject_Game::OnFinishEvalKeys();
}

int CWObject_GameCore::GetPlayerStatus(int _iPlayer)
{
	CWObject *pObj = m_pWServer->Object_Get(Player_GetObjectIndex(_iPlayer));
	if(pObj)
	{
		CWObject_Character *pChar = TDynamicCast<CWObject_Character>(pObj);
		if(pChar)
		{
			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(pChar);
			if(pCD && pCD->m_iPlayer != -1)
			{
				// Pretty sure this is the player...
				int Status = 0;
				if(CWObject_Character::Char_GetPhysType(pChar) == PLAYER_PHYS_DEAD)
					Status |= PLAYER_STATUS_DEAD;
				if(pCD->m_Tension != 0)
					Status |= PLAYER_STATUS_TENSION;
				if(!CWObject_Character::Char_IsPlayerView(pChar))
					Status |= PLAYER_STATUS_NOTPLAYERVIEW;
				if(pChar->m_PendingCutsceneTick != -1)
					Status |= PLAYER_STATUS_PENDINGCUTSCENE;
				return Status;
			}
		}
	}
	return 0;
}

void CWObject_GameCore::OnRefresh()
{
	MSCOPESHORT(CWObject_GameCore::OnRefresh);
	CWObject_Game::OnRefresh();

	{
		// Make sure that world messages blocked by async-savegame is sent
		CWServer_Mod *pServer = safe_cast<CWServer_Mod >(m_pWServer);
		pServer->m_GameState.RefreshBlockedWorldMessages();
	}

	if(m_PendingWorldTick == m_pWServer->GetGameTick())
	{
		if(!(GetPlayerStatus() & PLAYER_STATUS_DEAD))
		{
#ifdef PLATFORM_CONSOLE
			m_pWServer->World_ChangeFromGame(m_PendingWorld, SERVER_CHANGEWORLD_KEEPGAME | SERVER_CHANGEWORLD_MIGRATION);
#else
			m_pWServer->World_ChangeFromGame(m_PendingWorld, SERVER_CHANGEWORLD_KEEPGAME | SERVER_CHANGEWORLD_MIGRATION | SERVER_CHANGEWORLD_KEEPRESOURCES);
#endif
			m_bWaitingForChangeWorldScript = true;
			m_PendingWorldTick = -1;
			//m_PendingWorld = "";
		}
	}

	CClientData *pCD = GetClientData(this);

	for(int i = 0; i < pCD->m_liMusicVolumeModifier.Len(); i++)
		pCD->m_liMusicVolumeModifier[i] = -1.0f;

	if(m_lGameMessages.Len() && m_lGameMessages_Duration[0] < 0)
	{
		int Duration = m_pWServer->GetGameTick() - GetClientData()->m_GameMessage_StartTick;
		bool bWait = Duration >= 0 && Duration < Max((int)GetClientData()->m_GameMessage_Duration, (int)12);
		CWO_Player *pPlayer = Player_Get(0);
		if(!bWait && pPlayer)
		{
//			int Status = GetPlayerStatus();
			//if(!(Status & (PLAYER_STATUS_NOTPLAYERVIEW | PLAYER_STATUS_DEAD | PLAYER_STATUS_PENDINGCUTSCENE)))
			{
				fp32 Duration = -m_lGameMessages_Duration[0];
				if(Duration == 100000)
					Duration = 0;
				ShowGameMsg(m_lGameMessages[0], Duration, true);

				m_lGameMessages.Del(0);
				m_lGameMessages_Duration.Del(0);
				if(m_lGameMessages.Len() == 0)
					ClientFlags() &= ~GAME_CLIENTFLAGS_PENDINGGAMEMSG;
			}
		}
	}

	if(m_PendingPause != -1 && m_PendingPause == m_pWServer->GetGameTick())
	{
		int Status = GetPlayerStatus();
		if(!(Status & (PLAYER_STATUS_NOTPLAYERVIEW | PLAYER_STATUS_DEAD | PLAYER_STATUS_PENDINGCUTSCENE)) && !m_bWaitingForChangeWorldScript)
		{
//			int nClient = m_pWServer->Client_GetCount();
			for(int i = 0; i < Player_GetNum(); i++)
			{
				CWObject_Character *pChar = TDynamicCast<CWObject_Character>(Player_GetObject(i));
				if(pChar)
				{
					CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(pChar);
					if(pCD)
						pChar->m_Player.m_LastInfoscreenPress = m_pWServer->GetGameTick();
				}
				OnSetClientWindow(Player_GetClient(i), "infoscreen");
			}
		}
		m_PendingPause = -1;
	}

	m_DirtyMask |= pCD->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}

void CWObject_GameCore::IncludeAllPlayers()
{
	MSCOPE(IncludeAllPlayers, GAMECORE);

	CRPG_Object::m_bPrecacheForPlayerUse = true;
	CRegistry *pTemplates = m_pWServer->Registry_GetServer()->Find("TEMPLATES");
	if(pTemplates)
	{
		for(int i = 0; i < pTemplates->GetNumChildren(); i++)
		{
			if(pTemplates->GetName(i).CompareSubStr("PLAYER_") == 0)
			{
				CRPG_Object::m_bPrecacheForPlayerUse = true;
				m_pWServer->GetMapData()->GetResourceIndex_Class(pTemplates->GetName(i));
				CRPG_Object::m_bPrecacheForPlayerUse = false;
			}
		}
	}
	CRPG_Object::m_bPrecacheForPlayerUse = false;
}

void CWObject_GameCore::IncludeAllAIs()
{
	MSCOPE(IncludeAllAIs, GAMECORE);

	CRegistry *pTemplates = m_pWServer->Registry_GetServer()->Find("TEMPLATES");
	if(pTemplates)
	{
		for(int i = 0; i < pTemplates->GetNumChildren(); i++)
		{
			if(pTemplates->GetName(i).CompareSubStr("AI_") == 0)
				m_pWServer->GetMapData()->GetResourceIndex_Class(pTemplates->GetName(i));
		}
	}
}

int CWObject_GameCore::OnInitWorld()
{
	SetPicmipOffsets("0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");
	m_PendingWorld = "";

	//FadeScreen(0x00000000, 2, 4);
	//GetClientData()->m_ScreenFade_SrcColor = 0xff000000;
	GetClientData()->m_ScreenFade_SrcColor = 0x00000000;
	GetClientData()->m_ScreenFade_DestColor = 0x00000000;
	m_sbRenderSkipText = false;

	// Create scenepoint manager
	M_ASSERT(!m_pScenePointManager, "Shouldn't have an old ScenePointManager");
	int iSPM = m_pWServer->Object_Create("ScenePointManager", GetPositionMatrix());
	m_pScenePointManager = safe_cast<CWObject_ScenePointManager>(m_pWServer->Object_Get(iSPM));
	M_ASSERT(m_pScenePointManager, "Could not create ScenePointManager");

	m_pSoundGroupManager->InitWorld(*m_pWServer);

	return CWObject_Game::OnInitWorld();
}
	

int CWObject_GameCore::OnCloseWorld()
{
	SetPicmipOffsets("0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0");

	// Clear the animation mapping list, since we will shortly loose mapdata it references
	GetClientData()->m_Anim_ResourceMapper.SetLen(0);
	GetClientData()->m_Anim_ResourceMapper.MakeDirty();
	CStr StrEmpty;
	for (int i = 0; i < MAXANIMFILES; i++)
		m_Anim_ResourceMapperNames[i] = StrEmpty;

	m_pShellManager->CloseWorld();
	m_pSoundGroupManager->CloseWorld();

	m_lGameMessages.Clear();
	m_lGameMessages_Duration.Clear();
	m_sbRenderSkipText = false;
	m_bWaitingForChangeWorldScript = false;
	m_ClientFlags &= ~GAME_CLIENTFLAGS_PENDINGGAMEMSG;

	m_pScenePointManager = NULL;

	return CWObject_Game::OnCloseWorld();
}

void CWObject_GameCore::OnSpawnWorld()
{
	GetClientData()->m_TensionReleaseDelayDuration = 0;
	GetClientData()->m_BattleReleaseDelayDuration = 0;

	CRegistry *pGame = m_pWServer->Registry_GetLevelKeys("GAME");
	if(pGame)
	{
		for(int i = 0; i < pGame->GetNumChildren(); i++)
		{
			const CRegistry* pReg = pGame->GetChild(i);
			OnEvalKey(pReg->GetThisNameHash(), pReg);
		}
	}

	GetClientData()->m_lMissionID.MakeDirty();
	GetClientData()->m_lMissionDesc.MakeDirty();
	GetClientData()->m_lMissionFlags.MakeDirty();

	{
		// Create dummy player object, to allow animations and other to be tagged correctly
		CMat4Dfp32 Mat;
		Mat.Unit();
		Mat.k[3][2] = -100000;
		if (m_iDummyPlayer != -1)
		{
			m_pWServer->Object_DestroyInstant(m_iDummyPlayer);
			m_iDummyPlayer = -1;
		}

		m_iDummyPlayer = m_pWServer->Object_Create(GetDefualtSpawnClass(), Mat);
		CWObject *pObj = m_pWServer->Object_Get(m_iDummyPlayer);
		if(pObj)
		{
			CRegistry_Dynamic Reg;
			Reg.SetThisKey("PLAYERNR","0");
			pObj->OnEvalKey(Reg.GetThisNameHash(), &Reg);
			pObj->OnSpawnWorld();
			// Wait until after spawnworld2
			//m_pWServer->Object_DestroyInstant(m_iDummyPlayer);
		}
	}

	CWObject_Game::OnSpawnWorld();

	// Init Room manager
	m_pRoomManager->InitWorld(m_pWServer, m_pScenePointManager);

	m_DirtyMask |= GetClientData()->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}

void CWObject_GameCore::OnSpawnWorld2()
{
	CWObject_Game::OnSpawnWorld2();

	// Dummy char onspawnworld2
	CWObject *pObj = m_pWServer->Object_Get(m_iDummyPlayer);
	if(pObj)
		pObj->OnSpawnWorld2();
	// Init ScenePoint manager
	m_pScenePointManager->InitManager();
	m_pScenePointManager->Selection_Clear();
	m_pScenePointManager->Selection_AddAll();
	const TArray<CWO_ScenePoint *> lpScenePoints = m_pScenePointManager->Selection_Get();	
	TArray<uint16> lScenePointUsers;
	
	// Initialize the shell manager
	m_pShellManager->SpawnWorld(this, m_pWServer);

	// Get all characters
	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pWServer->Selection_AddClass(Selection, "CharNPC");
	// m_pWServer->Selection_AddOnFlagsSet(Selection, OBJECT_FLAGS_CHARACTER);
	const int16* pSel = NULL;
	int nSel = m_pWServer->Selection_Get(Selection, &pSel);
	int nUsers = 0;
	int nTeams = 0;
	TArray<CXRAG2_Impulse> lImpulse;
	lImpulse.SetLen(1);
	lImpulse[0].m_ImpulseType = AG2_IMPULSETYPE_BEHAVIOR;
	TArray<CXRAG2_StateConstant> lConstants;
	for (int i = 0; i < lpScenePoints.Len(); i++)
	{
		CWO_ScenePoint* pSP = lpScenePoints[i];
		lImpulse[0].m_ImpulseValue = pSP->GetBehaviour();
		if (!lImpulse[0].m_ImpulseValue) {continue;}

		lScenePointUsers.QuickSetLen(0);

		// First we all individual users if any
		for (int j = 0; j < CWO_ScenePoint::MAXUSERS; j++)
		{
			uint32 NameHash = pSP->GetNthUserName(j);
			if (!NameHash) break;

			TSelection<CSelection::SMALL_BUFFER> Sel2;
			m_pWServer->Selection_AddTarget(Sel2, NameHash);
			uint nElem = Sel2.GetNumElements();
			int16* piElem = Sel2.GetData();

			for (uint k = 0; k < nElem; k++, nUsers++)
				lScenePointUsers.Add(piElem[k]);
		}

		// Now we iterate over all teams and check membership for all 
		for (int j = 0; j < CWO_ScenePoint::MAXUSERS; j++)
		{
			if (pSP->GetNthTeam(j) == 0) {break;}
			nTeams++;
			CWObject_Message IsTeamMember(OBJMSG_TEAM_BELONGTOTEAM,pSP->GetNthTeam(j));
			for (int k = 0; k < nSel; k++)
			{
				if (m_pWServer->Message_SendToObject(IsTeamMember,pSel[k]))
				{
					lScenePointUsers.Add(pSel[k]);
				}
			}
		}

		// Get offsets and secondary behaviours if any
		if ((pSP->GetBehaviour())&&(lScenePointUsers.Len() > 0))
		{	// Just get the first users animgraph
			CWObject* pObj = m_pWServer->Object_Get(lScenePointUsers[0]);
			if (pObj)
			{
				CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pObj);
				if (pCD && pCD->m_AnimGraph2.GetAG2I() && pCD->m_AnimGraph2.GetAG2I()->AcquireAllResourcesFromMapData(m_pWServer->GetMapData()))
				{
					TArray<CXRAG2_StateConstant> lConstants; 
					lConstants.Clear();
					int iBehaviour = pSP->GetBehaviour();
					lConstants.Add(CXRAG2_StateConstant(AG2_CONSTANT_OFFSETX,0.0f)); 
					lConstants.Add(CXRAG2_StateConstant(AG2_CONSTANT_OFFSETY,0.0f)); 
					lConstants.Add(CXRAG2_StateConstant(AG2_CONSTANT_OFFSETZ,0.0f)); 
					lConstants.Add(CXRAG2_StateConstant(AG2_CONSTANT_OTHERBEHAVIOR,0.0f));
					lConstants.Add(CXRAG2_StateConstant(AG2_CONSTANT_REVERSEDIR,0.0f));
					pCD->m_AnimGraph2.GetAG2I()->GetGraphBlockConstants(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR, iBehaviour),lConstants);
					CVec3Dfp32 Offset;
					Offset[0] = lConstants[0].m_Value;
					Offset[1] = lConstants[1].m_Value;
					Offset[2] = lConstants[2].m_Value;
					int iSecondaryBehaviour = int(lConstants[3].m_Value);
					bool bReverseDir = lConstants[4].m_Value > 0.0f ? true : false;
					pSP->SetOffsetAndSecondaryBehaviour(iSecondaryBehaviour,Offset,bReverseDir);
					if (iSecondaryBehaviour)
					{	// Precache secondary behaviour too
						lImpulse.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR,iSecondaryBehaviour));
					}
				}
			}
		}

		if ((nUsers == 0) && (nTeams == 0))
		{	// No users were specified meaning that ALL can use the scenepoint
			for (int j = 0; j < nSel; j++)
			{
				CWObject* pObj = m_pWServer->Object_Get(pSel[j]);
				if (pObj)
				{
					CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pObj);
					if (pCD)
					{
						CWAG2I_Context Context(pObj, m_pWServer, pCD->m_GameTime);
						CWAG2I* pWAGI = pCD->m_AnimGraph2.GetAG2I();
						if (pWAGI)
						{
							pWAGI->TagAnimSetFromImpulses(&Context,m_pWServer->GetMapData(),m_pWServer->m_spWData,lImpulse);
						}
					}
				}
			}
		}
		else
		{	// Users from lScenePointUsers
			for (int j = 0; j < lScenePointUsers.Len(); j++)
			{
				CWObject* pObj = m_pWServer->Object_Get(lScenePointUsers[j]);
				if (pObj)
				{
					CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pObj);
					if (pCD)
					{	
						CWAG2I_Context Context(pObj, m_pWServer, pCD->m_GameTime);
						CWAG2I* pWAGI = pCD->m_AnimGraph2.GetAG2I();
						if (pWAGI)
						{
							pWAGI->TagAnimSetFromImpulses(&Context,m_pWServer->GetMapData(),m_pWServer->m_spWData,lImpulse);
						}
					}
				}
			}
		}

		
	}
	// Destroy dummy player
	if (m_iDummyPlayer != -1)
	{
		m_pWServer->Object_DestroyInstant(m_iDummyPlayer);
		m_iDummyPlayer = -1;
	}

	CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
	if (pServerMod)
		pServerMod->LoadAnimations();
}


aint CWObject_GameCore::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAME_GETANIMFROMHANDLE:
		return aint((CXR_Anim_SequenceData*)GetAnimFromHandle(this, m_pWServer, _Msg.m_Param0));

	case OBJMSG_GAME_RESOLVEANIMHANDLE:
		return ResolveAnimHandle((char *)_Msg.m_pData);

	case OBJMSG_GAME_GETSCREENFADE:
		return GetClientData()->GetCurScreenFade(m_pWServer->GetGameTick(), 0);
		
	case OBJMSG_GAME_GETSOUNDFADE:
		return GetClientData()->GetCurSoundFade(m_pWServer->GetGameTick(), 0);

	case OBJMSG_GAME_GETSCENEPOINTMANAGER:
		return (aint)m_pScenePointManager;

	case OBJMSG_GAME_GETROOMMANAGER:
		return (aint)m_pRoomManager;

	case OBJMSG_GAME_GETSHELLMANAGER:
		return (aint)m_pShellManager;

	case OBJMSG_GAME_GETSOUNDGROUPMANAGER:
		return (aint)m_pSoundGroupManager;

	case OBJMSG_GAME_ADDSCENEPOINT:
		return m_pScenePointManager->AddScenePoint(*(CWO_ScenePoint *)_Msg.m_Param0,*(CWO_ScenePointInitBlock *)_Msg.m_Param1);

	case OBJMSG_BROKEN_LIGHT:
		{	// Inform all bots that param0 light was broken
			// As there is only one(?) player in PB we send the broken light msg to him
			// When the game object(this) gets a proper ai manager it will of course handle it
			// in a more appropriate fashion.
			if (m_lspPlayers.Len() > 0)
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_BROKEN_LIGHT,_Msg.m_Param0), m_lspPlayers[0]->m_iObject);
			return 1;
		};
//	case OBJMSG_GAME_ENDROUND:
//		return EndRound(_Msg.m_Param0, _Msg.m_Param0, _Msg.m_pData, _Msg.m_DataSize);

	case OBJMSG_GAME_FADESCREEN:
		{
			int Status = GetPlayerStatus(0);
			if(Status & PLAYER_STATUS_DEAD)
				return 0;

			int Col = 0;
			if(_Msg.m_pData)
				Col = CFStr((char *)_Msg.m_pData).Val_int() & 0xffffff;
			if(_Msg.m_Param0 < 0)
				return FadeScreen(Col, -fp32(_Msg.m_Param0) / 1000);
			else
				return FadeScreen(Col | 0xff000000, fp32(_Msg.m_Param0) / 1000);
		}

	case OBJMSG_GAME_FADESOUND:
		if(_Msg.m_Param0 < 0)
		{
			M_TRACEALWAYS( "Fading up volume\n" );
			return FadeSound(0, -fp32(_Msg.m_Param0) / 1000);
		}
		else
		{
			M_TRACEALWAYS( "Fading down volume\n" );
			return FadeSound(255, fp32(_Msg.m_Param0) / 1000);
		}

	case OBJMSG_GAME_PLAY2DSOUND:
		{
			if (_Msg.m_pData) // Format:  "<SOUND>/[<SYNCOBJ>]"
			{
				CFStr SyncObj = (const char *)_Msg.m_pData;
				CFStr Sound = SyncObj.GetStrSep("/");

				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(Sound);
				if (iSound > 0)
				{
					uint32 GroupID = SyncObj.StrHash(); 
					m_pWServer->Sound_GlobalCutscene(iSound, 1.0f, -1, _Msg.m_Param0 * (1.0f / 1000.0f), GroupID);
					return 1;
				}
			}
			return 0;
		}

	case OBJMSG_GAME_CHARACTERKILLED:
		return OnCharacterKilled(_Msg.m_Param0, _Msg.m_iSender);

	case OBJMSG_GAME_QUITTOMENU:
		if(_Msg.m_Param0 == 0)
		{
			ConExecute("deferredscript(\"disconnect()\")");
			ConExecute("deferredscript(\"cg_rootmenu (\\\"mainmenu\\\")\")");
		}
		else if(_Msg.m_Param0 == 1)
		{
			//ConExecute("deferredscript(\"sv_restart\")");
			ConExecute("loadgame(\"checkpoint\")");
		}
		else if(_Msg.m_Param0 == 2)
		{
			ConExecute("deferredscript(\"sv_restart()\")");
		}
		return 1;

	case OBJMSG_GAME_PIPEMESSAGETOWORLD:
		{
			CWServer_Mod* pServer = safe_cast<CWServer_Mod >(m_pWServer);
			M_ASSERT(pServer != NULL && _Msg.m_pData != NULL && _Msg.m_Param0 != 0, "!");
			const char* pLevelAndTarget = (const char *)_Msg.m_Param0;					// the level to send message to  (level:target)
			const CWObject_Message& Msg2 = *(const CWObject_Message *)_Msg.m_pData;		// the actual message to send

			// 'ModifySpawnFlags' needs to be handled separately
			if (Msg2.m_Msg == OBJMSG_GAME_MODIFYSPAWNFLAGS)
			{
				CFStr Target = pLevelAndTarget;
				CFStr Level = Target.GetStrSep(":");
				if (Target.CompareNoCase("$game") == 0)
				{
					if (m_pWServer->m_WorldName.CompareNoCase(Level) == 0)
					{ // if sending to the currently active level, set flags instantly...
						return OnMessage(Msg2);
					}
					else
					{ // otherwise, modify the savegame for corresponding level  (NOTE: only modifies local flags - it's not valid to set global spawnflags on another level)
						bool bSet = (Msg2.m_pData == NULL || CStrBase::CompareNoCase((const char*)Msg2.m_pData, "Set") == 0);
						pServer->m_GameState.ModifyLocalSpawnFlags(Level, bSet, Msg2.m_Param0);
						return 1;
					}
				}
			}

			return pServer->m_GameState.SendMessageToLevel(pLevelAndTarget, Msg2);
		}
		break;

	case OBJMSG_GAME_RESET:
		{
			CWServer_Mod* pWServer = safe_cast<CWServer_Mod >(m_pWServer);
			M_ASSERTHANDLER(pWServer, "No ServerMod?!", return 0);

			const char* pLevel = (const char*)_Msg.m_pData;
			if (m_pWServer->m_WorldName.CompareNoCase(pLevel) == 0)
				ConOutL("§cf8WARNING: Sending level reset message to the currently played level will not work!");
			pWServer->m_GameState.ResetSaveInfo(pLevel);
			return 0;
		}

	case OBJMSG_GAME_GETCLIENTDATA:
		return aint(GetClientData());

	case OBJMSG_GAME_SHOWGAMEMSG:
		{
			fp32 Duration = fp32(_Msg.m_Param0) / 1000;
			//if(Duration != 0.0f || (_Msg.m_Reason & 1) || ((char *)_Msg.m_pData)[0] == '$')
				ShowGameMsg((char *)_Msg.m_pData, Duration);
			/*else
			{
				const char *pSt = (const char *)_Msg.m_pData;
				for(int i = 0; i < m_lGameMessages.Len(); i++)
					if(m_lGameMessages[i] == pSt)
						return 0;
				m_lGameMessages.Add(pSt);
				m_lGameMessages_Duration.Add(Duration);
				ClientFlags() |= GAME_CLIENTFLAGS_PENDINGGAMEMSG;
			}*/
		}
		return 1;

	case OBJMSG_GAME_SHOWGAMESURFACE:
		{
			fp32 Duration = fp32(_Msg.m_Param0) / 1000;
			if(_Msg.m_pData == 0 || *((const char *)_Msg.m_pData) == 0)
				ShowGameSurface(NULL, Duration);
			else
				ShowGameSurface(m_pWServer->GetMapData()->GetResourceIndex_Surface((const char *)_Msg.m_pData), Duration);
			return 1;
		}

	case OBJMSG_GAME_SHOWGAMESURFACEOVERLAY:
		{
			fp32 Duration = fp32(_Msg.m_Param0) / 1000;
			if(_Msg.m_pData == 0 || *((const char*)_Msg.m_pData) == 0)
				ShowGameSurfaceOverlay(NULL, Duration);
			else
				ShowGameSurfaceOverlay((char *)_Msg.m_pData, Duration);
			return 1;
		}

	case OBJMSG_GAME_REMOVEGAMEMSG:
		{
			for(int i = 0; i < m_lGameMessages.Len(); i++)
			{
				if(m_lGameMessages[i] == (const char *)_Msg.m_pData)
				{
					m_lGameMessages.Del(i);
					m_lGameMessages_Duration.Del(i);
					if(m_lGameMessages.Len() == 0)
						ClientFlags() &= ~GAME_CLIENTFLAGS_PENDINGGAMEMSG;
					return 1;
				}
			}
		}
		return 0;

	case OBJMSG_GAME_SHOWPENDINGGAMEMSG:
			if(m_lGameMessages.Len())
			{
				if(GetClientData()->m_GameMessage_StartTick + GetClientData()->m_GameMessage_Duration <= m_pWServer->GetGameTick())
				{
					ShowGameMsg(m_lGameMessages[0], m_lGameMessages_Duration[0]);
					m_lGameMessages.Del(0);
					m_lGameMessages_Duration.Del(0);
					if(m_lGameMessages.Len() == 0)
						ClientFlags() &= ~GAME_CLIENTFLAGS_PENDINGGAMEMSG;
				}
				return 1;
			}
			return 0;
	case OBJMSG_GAME_LEVELEXITS:
		{
			CFStr World = CFStr("Worlds\\") + (const char *)_Msg.m_pData + ".xw";
			CStr Path = m_pWServer->GetMapData()->ResolveFileName(World);
			return CDiskUtil::FileExists(Path);
		}

#ifndef M_RTM
	case OBJMSG_GAME_DEBUGMSG:
		ConOutL((char *)_Msg.m_pData);
		return 1;
#endif

	case OBJMSG_GAME_SHOWINFOSCREEN:
		ShowInfoScreen((char *)_Msg.m_pData, _Msg.m_Param0);
		return 1;

	// Scenepoint message (TODO: these could be sent directly to scenepoint manager instead...)
	case OBJMSG_GAME_SCENEPOINT_SPAWN:
	case OBJMSG_GAME_SCENEPOINT_UNSPAWN:
	case OBJMSG_GAME_SCENEPOINT_ACTIVATE:
	case OBJMSG_GAME_SCENEPOINT_RAISEPRIO:
	case OBJMSG_GAME_SCENEPOINT_RESTOREPRIO:
		{
			if (m_pScenePointManager)
			{
				m_pScenePointManager->Selection_Clear();
				m_pScenePointManager->Selection_AddByName( (const char*)_Msg.m_pData );
				m_pScenePointManager->Message_SendToSelection(_Msg);
				return 1;
			}
		}
		return 0;

	case OBJMSG_CHAR_RESPAWN:
		{
			CWObject *pObj = m_pWServer->Object_Get(_Msg.m_iSender);
			CStr Class = (const char *)_Msg.m_pData;
			if(_Msg.m_pData == NULL)
			{
				Class = GetDefualtSpawnClass();
				if(Class == "")
					return 0;
			}
			int Param = pObj->m_Param;
			int iObj = Player_Respawn(_Msg.m_Param0, pObj->GetPositionMatrix(), Class, _Msg.m_pData ? 0x2000 : 0);
			pObj = m_pWServer->Object_Get(iObj);
			if(pObj)
				pObj->m_Param = Param;
			return iObj;
		}
		return 1;

	case OBJMSG_GAMECORE_GETDEFAULTSPAWNCLASS:
		{
			CStr* pStr = (CStr*)_Msg.m_pData;
			if (pStr)
			{
				*pStr = GetDefualtSpawnClass();
				return 1;
			}
			return 0;
		}

	case OBJMSG_GAME_MODIFYFLAGS:
		GetClientData()->m_Flags = (GetClientData()->m_Flags | _Msg.m_Param0) & (~_Msg.m_Param1);
		return 1;

	case OBJMSG_GAME_SETVOLUMEMULTIPLIER:
		{
			// Set volumemultiplier
			GetClientData()->m_VolumeMultiplier = _Msg.m_VecParam0[0];
			return 1;
		}

	case OBJMSG_GAME_CONEXECUTE:
		return ConExecute((char *)_Msg.m_pData);

	case OBJMSG_GAME_SETMUSIC:
		if(_Msg.m_Param0 >= 0 && _Msg.m_Param0 < GetClientData()->m_liMusic.Len())
		{
			int Res = GetClientData()->m_liMusic[_Msg.m_Param0];
			CFStr Volume = (char *)_Msg.m_pData;
			CFStr Music = Volume.GetStrSep(",");
			if(Music != "")
				GetClientData()->m_liMusic[_Msg.m_Param0] = m_pWServer->GetMapData()->GetResourceIndex_Sound(Music);
//			else if(_Msg.m_Param1 != 0)
//				GetClientData()->m_liMusic[_Msg.m_Param0] = _Msg.m_Param1;	//m_Param1 is the activators object index
			else
				GetClientData()->m_liMusic[_Msg.m_Param0] = 0;
			if(Volume != "")
			{
				GetClientData()->m_liMusicVolumes[_Msg.m_Param0] = uint8(Volume.Val_fp64() * 127);
				GetClientData()->m_liMusicVolumes.MakeDirty();
			}
			GetClientData()->m_liMusic.MakeDirty();
			return Res;
		}
		return 0;

	case OBJSYSMSG_PRECACHEMESSAGE:
		{
			if(_Msg.m_DataSize == sizeof(CWObject_Message))
			{
				CWObject_Message *pMsg = (CWObject_Message *)_Msg.m_pData;
				switch(pMsg->m_Msg)
				{
				case OBJMSG_GAME_PLAY2DSOUND:
					{
						if (pMsg->m_pData)
						{
							CFStr Sound = CFStr((const char*)pMsg->m_pData).GetStrSep("/");
							if (!Sound.IsEmpty())
								return m_pWServer->GetMapData()->GetResourceIndex_Sound(Sound) > 0;
						}
						break;
					}
				case OBJMSG_GAME_SHOWINFOSCREEN:
					if(pMsg->m_Param0 == 1 && pMsg->m_pData)
						// Fading surface
						return m_pWServer->GetMapData()->GetResourceIndex_Surface((const char *)pMsg->m_pData) > 0;

				case OBJMSG_GAME_SETMUSIC:
					{
						char *pSound = (char *)pMsg->m_pData;
						if(pSound && (*pSound != 0))
							return m_pWServer->GetMapData()->GetResourceIndex_Sound(pSound) > 0;
						break;
					}
				case OBJMSG_GAME_SHOWGAMESURFACE:
					{
						char *pSurf = (char *)pMsg->m_pData;
						if(pSurf && (*pSurf != 0))
							return m_pWServer->GetMapData()->GetResourceIndex_Surface(pSurf) > 0;
						break;
					}

				case OBJMSG_GAME_SHOWGAMESURFACEOVERLAY:
					{
						char* pSurfaceName = (char*)pMsg->m_pData;
						if(pSurfaceName && (*pSurfaceName != 0))
							return m_pWServer->GetMapData()->GetResourceIndex_Surface(CStr(pSurfaceName).GetStrSep(",")) > 0;
						break;
					}
				case OBJMSG_GAME_SHOWGAMEMSG:
					{
						const char* pStr = (const char*)pMsg->m_pData;
						if (pStr && pStr[0] && pStr[0] != '$')
						{
							if (strchr(pStr, '|'))
								pStr = strchr(pStr, '|') + 1;

							wchar Buffer[1024];
							::Localize_Str(pStr, Buffer, 1024);
							if (Buffer[0] == '#')
							{
								CStr St = Buffer + 1;
								while (!St.IsEmpty())
								{
									St.GetStrSep(","); // X
									St.GetStrSep(","); // Y
									if (St[0] == '$')
									{
										St = St.Del(0,1);
										m_pWServer->GetMapData()->GetResourceIndex_Surface(St.GetStrSep(",").Ansi());
										St.GetStrSep(","); // W
										St.GetStrSep(","); // H
									}
								}
							}
						}
					}
					break;
				}
			}
			break;
		}
	}

	return CWObject_Game::OnMessage(_Msg);
}

void CWObject_GameCore::SendForceEndTimeleapMessage()
{
	for(int i = 0; i < m_lspPlayers.Len(); i++)
		if(m_lspPlayers[i])
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_FORCEENDTIMELEAP), m_lspPlayers[i]->m_iObject);
}

void CWObject_GameCore::OnIncludeClass(CMapData *_pWData, CWorld_Server *_pWServer)
{
	// This stuff automatically includes resources from various templates and register-defintions.
	CRegistry *pInclude = _pWData->m_spWData->GetGameReg()->Find("SERVER\\INCLUDECLASS");
	if(pInclude)
	{
//		CRegistry *pTemplates = _pWData->m_spWData->GetGameReg()->Find("SERVER\\TEMPLATES");
		
		CRegistry *pClasses = pInclude->FindChild("CLASSES");
		if(pClasses)
		{
			int nClasses = pClasses->GetNumChildren();
			for(int c = 0; c < nClasses; c++)
			{
				CRegistry *pChild = pClasses->GetChild(c);
				CStr st = pChild->GetThisName().LowerCase();
				//				ConOutL("§C999Loading §C974" + st + "§C999...");
				_pWData->GetResourceIndex_Class(pChild->GetThisName());
			}
		}
		
		CRegistry *pRPGClasses = pInclude->FindChild("RPGTEMPLATES");
		if(pRPGClasses)
		{
			int nClasses = pRPGClasses->GetNumChildren();
			for(int c = 0; c < nClasses; c++)
			{
				CRegistry *pChild = pRPGClasses->GetChild(c);
				CRPG_Object::IncludeRPGClass(pChild->GetThisName(), _pWData, _pWServer);
			}
		}
		
		CRegistry *pCustom = pInclude->FindChild("CUSTOMMODELS");
		if(pCustom)
		{
			int nModels = pCustom->GetNumChildren();
			for(int m = 0; m < nModels; m++)
			{
				CRegistry *pChild = pCustom->GetChild(m);
				_pWData->GetResourceIndex_Model(pChild->GetThisName());
			}
		}
		
		CRegistry *pModels = pInclude->FindChild("MODELS");
		if(pModels)
		{
			int nModels = pModels->GetNumChildren();
			for(int m = 0; m < nModels; m++)
			{
				CRegistry *pChild = pModels->GetChild(m);
				_pWData->GetResourceIndex_Model(pChild->GetThisName());
			}
		}
		
		CRegistry *pAnims = pInclude->FindChild("ANIMS");
		if(pAnims)
		{
			int nAnims = pAnims->GetNumChildren();
			for(int m = 0; m < nAnims; m++)
			{
				CRegistry *pChild = pAnims->GetChild(m);
				_pWData->GetResourceIndex_Anim(pChild->GetThisName());
			}
		}
		
		CRegistry *pSounds = pInclude->FindChild("SOUNDS");
		if(pSounds)
		{
			int nSounds = pSounds->GetNumChildren();
			for(int m = 0; m < nSounds; m++)
			{
				CRegistry *pChild = pSounds->GetChild(m);
				_pWData->GetResourceIndex_Sound(pChild->GetThisName());
			}
		}
		
		CRegistry *pFonts = pInclude->FindChild("FONTS");
		if(pFonts)
		{
			int nFonts = pFonts->GetNumChildren();
			for(int m = 0; m < nFonts; m++)
			{
				CRegistry *pChild = pFonts->GetChild(m);
				_pWData->GetResourceIndex_Font(pChild->GetThisName());
			}
		}
	}

	CWO_ShellManager::OnIncludeClass(_pWData, _pWServer);
}

void CWObject_GameCore::OnClientPrecacheClass(CWorld_Client* _pWClient, CXR_Engine* _pEngine)
{
	CWObject_Game::OnClientPrecacheClass(_pWClient, _pEngine);

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if(pTC)
	{
		pTC->SetTextureParam(pTC->GetTextureID("GUI_TRIANGLE"), CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
#ifdef PLATFORM_XBOX
		pTC->SetTextureParam(pTC->GetTextureID("GUI_Button_B_32"), CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
#elif !defined(PLATFORM_CONSOLE)
		pTC->SetTextureParam(pTC->GetTextureID("GUI_Button_F1_64"), CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
#endif
		pTC->SetTextureParam(pTC->GetTextureID("GUI_ICON_JOURNAL"), CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	}
}

int CWObject_GameCore::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const
{
	const CClientData *pCD = GetClientData(this);
	int Flags = 0;
	if(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
		Flags = CWO_CLIENTUPDATE_AUTOVAR;
	uint8* pD = _pData;
	pD += CWObject_Game::OnCreateClientUpdate(_iClient, _pClObjInfo, _pObj, _pData, Flags);
	if (pD - _pData == 0)
		return pD - _pData;
	
	pCD->AutoVar_Pack(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT, pD, m_pWServer->GetMapData());
	
	return pD - _pData;
}

int CWObject_GameCore::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	MSCOPESHORT(CWObject_GameCore::OnClientUpdate);
	const uint8* pD = &_pData[CWObject_Game::OnClientUpdate(_pObj, _pWClient, _pData, _Flags)];
	if (_pObj->m_iClass == 0 || pD - _pData == 0) return pD - _pData;
	
	if(_pObj->m_bAutoVarDirty)
		GetClientData(_pObj)->AutoVar_Unpack(pD, _pWClient->GetMapData());

	return (uint8*)pD - _pData;
}

aint CWObject_GameCore::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAME_GETSHELLMANAGER:
		return (aint)((CWObject_GameCore*)_pObj)->m_pShellManager;

	case OBJMSG_GAME_RENDERSTATUSBAR:
		OnClientRenderStatusBar(_pObj, _pWClient, (CXR_Engine *)_Msg.m_Param0, (CRC_Util2D *)_Msg.m_Param1, (CVec2Dfp32 *)_Msg.m_pData);
		return 1;
		
	case OBJMSG_GAME_GETANIMFROMHANDLE:
		return aint((CXR_Anim_SequenceData*)GetAnimFromHandle(_pObj, _pWClient, _Msg.m_Param0));
		
	case OBJMSG_GAME_GETSCREENFADE:
		{
			int Res = GetClientData(_pObj)->GetCurScreenFade(_pWClient->GetGameTick(), _pWClient->GetRenderTickFrac());
			if(_Msg.m_Param0 != 0)
				*(int *)_Msg.m_Param0 = m_sbRenderSkipText;
			return Res;
		}
		
	case OBJMSG_GAME_GETGAMESTATE:
		return GetGameState(_pObj);

	case OBJMSG_GAME_GETCLIENTDATA:
		return aint(GetClientData(_pObj));
	case OBJMSG_GAME_SETRENDERWORLD:
		{
			GetClientData(_pObj)->m_bRenderWorld = _Msg.m_Param0 != 0;
			return 1;
		}

	case OBJMSG_GAME_NEVERRENDERWORLD:
		{
			if(GetClientData(_pObj)->m_bRenderWorld)
			{
				if((GetClientData(_pObj)->GetCurScreenFade(_pWClient->GetGameTick(), _pWClient->GetRenderTickFrac()) & 0xff000000) == 0xff000000)
					return 1;
				else
					return 0;
			}
			else
				return 1;
		}

	case OBJMSG_GAME_ADDSUBTITLE:
		{
			CClientData *pCD = GetClientData(_pObj);
			for(int i = 0; i < pCD->m_lCurSubtitles.Len(); i++)
				if(pCD->m_lCurSubtitles[i].m_iObject == _Msg.m_iSender)
				{
					if(_Msg.m_Param0 == 0)
						pCD->m_lCurSubtitles.Del(i);
					else
						pCD->m_lCurSubtitles[i].m_RangeSqr = _Msg.m_Param0 * _Msg.m_Param0;
					return 0;
				}
			
			if(_Msg.m_Param0 != 0)
				pCD->m_lCurSubtitles.Add(CClientData::CSubtitleInstance(_Msg.m_iSender, _Msg.m_Param0));
			return 1;
		}

	case OBJMSG_GAME_GETOVERLAYSURFACE:
		{
			CClientData* pCD = GetClientData(_pObj);
			if(pCD && _Msg.m_pData)
			{
				CWO_GameCameraOverlay& GameCameraOverlay = *(CWO_GameCameraOverlay*)_Msg.m_pData;
				
				GameCameraOverlay.m_Duration = pCD->m_GameSurfaceOverlay_Duration;
				GameCameraOverlay.m_StartTick = pCD->m_GameSurfaceOverlay_StartTick;
				GameCameraOverlay.m_SurfaceID = pCD->m_GameSurfaceOverlay_Surface;
				GameCameraOverlay.m_Type = pCD->m_GameSurfaceOverlay_Type;
				GameCameraOverlay.m_WH = pCD->m_GameSurfaceOverlay_WH;
				return 1;
			}

			return 0;
		}
	}

	return CWObject::OnClientMessage(_pObj, _pWClient, _Msg);
}

int CompareSubtitleType(int _Type1, int _Type2)
{
	if(_Type1 == _Type2)
		return 0;
	
	if(_Type1 == -1)
		return 1;

	switch(_Type1)
	{
	case SUBTITLE_TYPE_IDLE: return 1;
	case SUBTITLE_TYPE_INTERACTIVE: if(_Type2 == SUBTITLE_TYPE_CUTSCENE) return 1; return -1;
	case SUBTITLE_TYPE_AI: if(_Type2 == SUBTITLE_TYPE_IDLE) return -1; return 1;
	case SUBTITLE_TYPE_CUTSCENE: return -1;
	case SUBTITLE_TYPE_CUTSCENEKEEPENDANIM: return -1;
	}
	return 0;
}

void CWObject_GameCore::OnClientRenderStatusBar(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D, CVec2Dfp32 *_rect)
{
	MSCOPESHORT(CWObject_GameCore::OnClientRenderStatusBar);
	CMat4Dfp32 Mat;
	_pWClient->Render_GetLastRenderCamera(Mat);
	
	CClientData *pCD = GetClientData(_pObj);

	CDialogueInstance *pNewestItem = NULL;
	int iNewestObject = 0;
	int NewestParam = 0;
	int NewestType = -1;
	const fp32 FadeoutDistance = 64;
	fp32 NewestDistanceSqr = 0;
	fp32 NewestMaxDistanceSqr = 0;
	
	const CMTime& Time = _pWClient->GetGameTime();

	CWObject_Client *pPlayer = _pWClient->Object_Get(_pWClient->Player_GetLocalObject());
	bool bCutscene = (pPlayer ? pPlayer->m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE : 0) != 0;
	bool bDialogue = (pPlayer ? pPlayer->m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE : 0) != 0;


	// Allow player to render statusbar
	int iPlayer = _pWClient->Player_GetLocalObject();
	if(iPlayer >= 0)
		_pWClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_RENDERSTATUSBAR, aint(_pEngine), aint(_pUtil2D)), iPlayer);

//	int nSub = 0;
	for(int i = pCD->m_lCurSubtitles.Len() - 1; i >= 0; i--)
	{
		CClientData::CSubtitleInstance *pSI = &pCD->m_lCurSubtitles[i];
		int Param = 0;
		CDialogueInstance *pItem = (CDialogueInstance *)_pWClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_REQUESTSUBTITLE, aint(&Param)), pSI->m_iObject);
		if(pItem != NULL && pItem->m_Choice != "")
		{
			pNewestItem = pItem;
			iNewestObject = pSI->m_iObject;
			NewestDistanceSqr = 0;
			NewestMaxDistanceSqr = 10000;
			NewestParam = Param;
			NewestType = pItem->m_SubtitleFlags & SUBTITLE_TYPE_MASK;
			break;
		}
		if (pItem && pItem->m_Subtitle != "" && (Time - pItem->m_StartTime).GetTime() <= pItem->m_SampleLength)
		{
			int Type = pItem->m_SubtitleFlags & SUBTITLE_TYPE_MASK;
			if((!bCutscene || Type == SUBTITLE_TYPE_CUTSCENE || Type == SUBTITLE_TYPE_CUTSCENEKEEPENDANIM) &&
			   (!bDialogue || Type == SUBTITLE_TYPE_INTERACTIVE))
			{
				CWObject_Client *pObj = _pWClient->Object_Get(pSI->m_iObject);
				if(pObj)
				{
					fp32 DistanceSqr = (pObj->GetPosition() - CVec3Dfp32::GetRow(Mat, 3)).LengthSqr();
					fp32 MaxDistanceSqr = pSI->m_RangeSqr - (FadeoutDistance * FadeoutDistance);
					if(DistanceSqr < MaxDistanceSqr)
					{
						int TypeComp = CompareSubtitleType(NewestType, pItem->m_SubtitleFlags & SUBTITLE_TYPE_MASK);
						if(TypeComp > 0 || (TypeComp == 0 && DistanceSqr < NewestDistanceSqr))
						{
							pNewestItem = pItem;
							iNewestObject = pSI->m_iObject;
							NewestDistanceSqr = DistanceSqr;
							NewestMaxDistanceSqr = MaxDistanceSqr;
							NewestParam = Param;
							NewestType = pItem->m_SubtitleFlags & SUBTITLE_TYPE_MASK;
						}
					}
				}
			}
		}
		else
			pCD->m_lCurSubtitles.Del(i);
	}
	
	if(pNewestItem)
	{
		fp32 NewestFade = 0;
		fp32 Distance = M_Sqrt(NewestDistanceSqr);
		fp32 MaxDistance = M_Sqrt(NewestMaxDistanceSqr);
		if(MaxDistance - Distance > FadeoutDistance)
			NewestFade = 1.0f;
		else
			NewestFade = (MaxDistance - Distance) / FadeoutDistance;
		NewestFade *= 1.0f - (fp32(GetClientData(_pObj)->GetCurScreenFade(_pWClient->GetGameTick(), _pWClient->GetRenderTickFrac()) >> 24) / 255);
		CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
		if(pFont)
		{
			CClipRect Clip(0, 0, 640, 480);
			CRct Rect = _pUtil2D->GetVBM()->Viewport_Get()->GetViewRect();

			fp32 aspect = ((fp32)Rect.p1.x / (fp32)Rect.p1.y);
			int ExtraHeight = 5;
			int YStart = 400;
			if(aspect > 1.4f)
			{
				CVec2Dfp32 scale(Rect.GetWidth() / 640.0f, Rect.GetHeight() / 480.0f);
				_pUtil2D->SetFontScale(1.0f + scale.k[0] - scale.k[1], 1.0f + scale.k[0] - scale.k[1]);
				ExtraHeight = 10;
				YStart = 390;
			}

			int Alpha = RoundToInt(NewestFade* 255) << 24;
			int Col = pNewestItem->m_SubtitleColor | Alpha;
			int ShadowCol = Alpha;
			int Border = 150;
			int XStart = Border / 2;
			Clip.clip.p0.x += Border / 2;
			Clip.clip.p1.x -= Border / 2;
//			const int ChoiceHeight = 30;
//			const int YStart = 355;

			_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

//			int Flags = pNewestItem->m_Flags;
/*			if(pNewestItem->m_Choice != "")
			{
				CStr Choice = pNewestItem->m_Choice;
				int iItem = pNewestItem->m_iDialogueItem & 0xff;
				int i;
				for(i = 0; i < iItem - 1; i++)
					Choice.GetStrSep(";");

				Choice = Choice.Ansi();
				CStr lChoice[3];
				if(iItem > 0)
					lChoice[0] = Choice.GetStrSep(";");
				lChoice[1] = Choice.GetStrSep(";");
				lChoice[2] = Choice.GetStrSep(";");

				int Width = pFont->GetWidth(pFont->GetOriginalSize(), CStrF("§Z%i", pNewestItem->m_SubtitleSize) + lChoice[1]) + 8 + 4;
				DrawInfoRect(_pUtil2D, _pWClient, CRct(XStart - 3, YStart+ChoiceHeight-3, XStart + Width - 3, YStart+ChoiceHeight+22));
				for(i = 0; i < 3; i++)
				{
					CStr St = lChoice[i];
					int Col = 0xff808080;
					if(St != "")
					{
						if(St.Ansi()[0] == '$')
						{
							St = St.Copy(1, 1024);
							Col = 0xff603030;
						}

						CStr Text = CStrF("§Z%i", pNewestItem->m_SubtitleSize) + St;
						_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, XStart, YStart + i * ChoiceHeight, 0, ShadowCol, ShadowCol, ShadowCol, Clip.GetWidth()-Border, Clip.GetHeight(), true);
						_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, XStart, YStart + i * ChoiceHeight, 0, Col, Col, ShadowCol, Clip.GetWidth()-Border, Clip.GetHeight(), false);
					}
				}
			}
			else */			if(pNewestItem->m_Subtitle != "")
			{
				CStr Subtitle = pNewestItem->m_Subtitle;

				/* -- This code will append the character's name in front of the subtitle text
				CWObject_CoreData* pObj = _pWClient->Object_GetCD(iNewestObject);
				if (pObj && CWObject_Character::IsCharacter(pObj))
				{
					CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pObj);
					if (pCD && pCD->m_CharName != "")
						Subtitle = (CFStr)pCD->m_CharName + ": " + Subtitle;
				}*/

				CStr Text = CStrF("§Z%i", pNewestItem->m_SubtitleSize) + Subtitle;
				char moo = '§';	// GCC workaround
				if (pNewestItem->m_Subtitle.Ansi().Str()[0] == moo)
					Text += CStrF("§p0%i§pq", NewestParam);

				_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, XStart, YStart, WSTYLE_TEXT_WORDWRAP, ShadowCol, ShadowCol, ShadowCol, Clip.GetWidth(), Clip.GetHeight(), true, ExtraHeight);
				_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, XStart, YStart, WSTYLE_TEXT_WORDWRAP, Col, Col, ShadowCol, Clip.GetWidth(), Clip.GetHeight(), false, ExtraHeight);
				_pUtil2D->SetFontScale(1.0f, 1.0f);
			}
		}
	}

//	if (!bCutscene)
//		DrawFocusFrame(_pUtil2D,_pWClient);
}

void CWObject_GameCore::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT( CWObject_GameCore::OnClientRefresh );
	CWObject_Game::OnClientRefresh(_pObj, _pWClient);

	CClientData *pCD = GetClientData(_pObj);

	bool bIsPBHD = (pCD->m_Flags & GAME_FLAGS_IS_PBHD) != 0;		// needed for some riddick-specific code... remove when code has branched

	// Update tension
	CWO_CameraEffects NVInfo;
	CWObject_Message Msg(OBJMSG_CHAR_GETNVINTENSITY);
	Msg.m_pData = &NVInfo;
	_pWClient->ClientMessage_SendToObject(Msg, _pWClient->Player_GetLocalObject());

	fp32 VolumeMusic = pCD->m_VolumeMultiplier;
	fp32 VolumeAmbient = pCD->m_VolumeMultiplier;
	fp32 VolumeEffects = pCD->m_VolumeMultiplier;
	fp32 VolumeVoices = pCD->m_VolumeMultiplier;
	fp32 VolumeCutscene = pCD->m_VolumeMultiplier;
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys && pSys->GetOptions())
	{
		VolumeMusic *= pSys->GetOptions()->GetValuef("SND_VOLUMEMUSIC", 0.7f);
		fp32 Sfx = pSys->GetOptions()->GetValuef("SND_VOLUMESFX", 0.7f);
		VolumeAmbient *= Sfx;
		VolumeEffects *= Sfx;
		VolumeVoices *= Sfx;
	}

	if(pCD->m_Flags & GAME_FLAGS_NOMUSIC)
		VolumeMusic = 0;
	if(pCD->m_Flags & GAME_FLAGS_NOSOUNDEFFECTS)
		VolumeEffects = 0;
	if(pCD->m_Flags & GAME_FLAGS_NOSOUNDAMBIENTS)
		VolumeAmbient = 0;
	if(pCD->m_Flags & GAME_FLAGS_NOSOUNDVOICE)
		VolumeVoices = 0;
	
	bool bTweakSoundvolumes = false;
	CWObject_Client *pChar = _pWClient->Object_Get(_pWClient->Player_GetLocalObject());
	if(pChar)
	{
		CWO_Character_ClientData *pCDChar = CWObject_Character::GetClientData(pChar);

		const fp32 Hurt = (fp32)LERP(pCDChar->m_HitEffect_LastTime, pCDChar->m_HitEffect_Time, pCDChar->m_PredictFrameFrac);
		bTweakSoundvolumes = (Hurt > 0.0001f) || pCDChar->m_DarknessVoiceEffect || pCDChar->m_3PI_LightState != THIRDPERSONINTERACTIVE_LIGHT_STATE_OFF;
		if(bTweakSoundvolumes)
		{
			fp32 SoundTweak = (fp32(pCDChar->m_Health.m_Value) / fp32(pCDChar->m_MaxHealth.m_Value)) * 0.5f + 0.5f;
			SoundTweak = Max(SoundTweak, 0.50f);
			if(pCDChar->m_DarknessVoiceEffect)
				SoundTweak = 0.75f;
			VolumeMusic *= SoundTweak;

			// fade ambientsound just like the dialoguelamp
			fp32 AmbientFadeVal = 0.65f;
			if(pCDChar->m_3PI_LightState == THIRDPERSONINTERACTIVE_LIGHT_STATE_ON ||
				pCDChar->m_3PI_LightState == THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_IN)
			{
				int32 Diff = _pWClient->GetGameTick() - pCDChar->m_3PI_LightFadeStart;
				fp32 FadeDuration = 2.5f * _pWClient->GetGameTicksPerSecond();
				AmbientFadeVal = ((fp32)Diff / FadeDuration);
				AmbientFadeVal = Min(AmbientFadeVal, 1.0f);
				AmbientFadeVal = Max(AmbientFadeVal, 0.0f);
				AmbientFadeVal = 1.0f - AmbientFadeVal;
				AmbientFadeVal *= 0.65f;
			}
			else if(pCDChar->m_3PI_LightState == THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_OUT)
			{
				int32 Diff = _pWClient->GetGameTick() - pCDChar->m_3PI_LightFadeStart;
				fp32 FadeDuration = 2.5f * _pWClient->GetGameTicksPerSecond();
				AmbientFadeVal = ((fp32)Diff / FadeDuration);
				AmbientFadeVal = Min(AmbientFadeVal, 1.0f);
				AmbientFadeVal = Max(AmbientFadeVal, 0.0f);
				AmbientFadeVal *= 0.65f;
			}
						
			VolumeAmbient *= SoundTweak * (0.35f + AmbientFadeVal);
			VolumeEffects *= SoundTweak;
			VolumeVoices *= SoundTweak;
		}
	}

	fp32 lFadeSpeeds[MAXMUSICRESOURCES*2];
	fp32 lVolumesMusic[MAXMUSICRESOURCES];
	memset(lVolumesMusic, 0, sizeof(lVolumesMusic));
	if (!bIsPBHD)
	{
		// Game specific logic
		if(pCD->m_liMusicVolumes.Len())
		{
			lVolumesMusic[0] = (VolumeAmbient * pCD->m_liMusicVolumes[0]) / 127;
			if(pCD->m_liMusicVolumeModifier[0] != -1.0f)
				lVolumesMusic[0] = pCD->m_liMusicVolumeModifier[0];

			int iTrack = 0;
			if(NVInfo.m_Tension > 0.9f)
			{
				pCD->m_BattleReleaseTick = _pWClient->GetGameTick() + pCD->m_BattleReleaseDelayDuration;
				pCD->m_BattleReleaseTick.MakeDirty();

				iTrack = 3;
			}
			else if(NVInfo.m_Tension > 0.05f || _pWClient->GetGameTick() < pCD->m_BattleReleaseTick)
			{
				pCD->m_TensionReleaseTick = _pWClient->GetGameTick() + pCD->m_TensionReleaseDelayDuration;
				pCD->m_TensionReleaseTick.MakeDirty();

				if(_pWClient->GetGameTick() < pCD->m_BattleReleaseTick)
					iTrack = 3;
				else
					iTrack = 2;
			}
			else
			{
				if(_pWClient->GetGameTick() < pCD->m_TensionReleaseTick)
					iTrack = 2;
				else
					iTrack = 1;
			}

			bool bDarknessMode = (NVInfo.m_Nightvision >= 0.5f);
			if(NVInfo.m_CreepingDark > 0.1f)
			{
				lVolumesMusic[0] = 0.0f;	
				VolumeEffects *= 0.4f;		
				bDarknessMode = true;
			}

			if (bDarknessMode)
			{ // Change music to darkness mode
				iTrack += 3;
			}

			if(pCD->m_liMusicVolumes.Len() > iTrack)
				lVolumesMusic[iTrack] = (VolumeMusic * pCD->m_liMusicVolumes[iTrack]) / 127;
			else
				lVolumesMusic[iTrack] = VolumeMusic;

			if(pCD->m_liMusicVolumeModifier[iTrack] != -1.0f)
				lVolumesMusic[iTrack] = pCD->m_liMusicVolumeModifier[iTrack];
		}

		if(pCD->m_lfMusicFadeSpeeds.Len())
		{
			for(int i = 0; i < pCD->m_lfMusicFadeSpeeds.Len(); i++)
			lFadeSpeeds[i] = pCD->m_lfMusicFadeSpeeds[i];
		}
	}
	else // bIsPBHD
	{
		lVolumesMusic[0] = VolumeMusic;
		lVolumesMusic[1] = 0.0f;
		lVolumesMusic[2] = 0.0f;
		lVolumesMusic[3] = 0.0f;
		if (NVInfo.m_Tension > 0.05f)
			lVolumesMusic[3] = 1.0f;
		else if (NVInfo.m_Crouch > 0.05f)
			lVolumesMusic[2] = 1.0f;
		else
			lVolumesMusic[1] = 1.0f;

		for (uint i = 0; i < MAXMUSICRESOURCES*2; i++)
			lFadeSpeeds[i] = 0.25f;
	}

	int nMusicResources = pCD->m_liMusic.Len();
	int lMusic[MAXMUSICRESOURCES];
	for(int i = 0; i < nMusicResources; i++)
		lMusic[i] = pCD->m_liMusic[i];

	_pWClient->Sound_MultiStream_Play(lMusic, nMusicResources);
	_pWClient->Sound_MultiStream_Volumes(lVolumesMusic, nMusicResources, lFadeSpeeds);

	VolumeCutscene = VolumeVoices;
	if(bTweakSoundvolumes)
		VolumeCutscene = pCD->m_VolumeMultiplier;

	CWorld_ClientCore *pCore = safe_cast<CWorld_ClientCore >(_pWClient);
	if(pCore)
	{
		_pWClient->Sound_SetSoundChannelVolume(pCore->m_hChannels[WCLIENT_CHANNEL_CUTSCENE], VolumeCutscene);
		_pWClient->Sound_SetSoundChannelVolume(pCore->m_hChannels[WCLIENT_CHANNEL_SFX], VolumeEffects);
		_pWClient->Sound_SetSoundChannelVolume(pCore->m_hChannels[WCLIENT_CHANNEL_SFXLOOPING], VolumeEffects);
		_pWClient->Sound_SetSoundChannelVolume(pCore->m_hChannels[WCLIENT_CHANNEL_AMBIENT], VolumeAmbient);
		_pWClient->Sound_SetSoundChannelVolume(pCore->m_hChannels[WCLIENT_CHANNEL_VOICE], VolumeVoices);
	}

}

spCXR_Anim_SequenceData CWObject_GameCore::GetAnimFromHandle(CWObject_CoreData *_pObj, CWorld_PhysState* _pWPhysState, int _Handle)
{
	CClientData *pCD = GetClientData(_pObj);

	// Index 0 reserverd for AnimList
	int Index = (_Handle >> 8) - 1;
	if(Index < 0 || Index >= pCD->m_Anim_ResourceMapper.Len())
		return NULL;

	CXR_Anim_Base *pAnim = _pWPhysState->GetMapData()->GetResource_Anim(pCD->m_Anim_ResourceMapper[Index]);
	spCXR_Anim_SequenceData spSeq = NULL;
	if(!pAnim)
	{
#ifndef M_RTM
		const char *pName = "";
		CWObject_GameCore *pGame = TDynamicCast<CWObject_GameCore >(_pObj);
		if(pGame)
			pName = pGame->m_Anim_ResourceMapperNames[Index].Str();

		ConOutL(CStrF("§cf80WARNING: Could not get animation resource %i (Name %s  Handle 0x%x)", pCD->m_Anim_ResourceMapper[Index], pName, _Handle));
#endif
		return NULL;
	}

	spSeq = pAnim->GetSequence(_Handle & 255);
#ifndef M_RTM
	if(!spSeq)
	{
		const char *pName = "";
		CWObject_GameCore *pGame = TDynamicCast<CWObject_GameCore >(_pObj);
		if(pGame)
			pName = pGame->m_Anim_ResourceMapperNames[Index].Str();

		ConOutL(CStrF("Anim resource %i did not contain sequence (Name %s  Handle 0x%X)", pCD->m_Anim_ResourceMapper[Index], pName, _Handle & 255));
	}
#endif
	
	return spSeq;
}

int CWObject_GameCore::ResolveAnimHandle(const char *_pSequence)
{
	CFStr St = _pSequence;
	CFStr Name = St.GetStrSep(":");
	int iSeq = St.Val_int();
	char *pName = Name;

	if(Name == "")
		return -1;

	if(iSeq > 255)
		Error("CWObject_GameCore::ResolveAnimHandle", "Animation sequence indexes must be lower than 256");

	if(Name.CompareNoCase("ANIMLIST") == 0)
		return iSeq;

	CClientData *pCD = GetClientData();
	for(int i = 0; i < pCD->m_Anim_ResourceMapper.Len(); i++)
	{
		if(m_Anim_ResourceMapperNames[i].CompareNoCase(Name) == 0)
			return ((i + 1) << 8) | iSeq;
	}

	if(pCD->m_Anim_ResourceMapper.Len() == MAXANIMFILES)
		Error("CWObject_GameCore::ResolveAnimHandle", "Too many animation-files");

	int iResource = m_pWServer->GetMapData()->GetResourceIndex_Anim(pName);
	if(iResource <= 0)
		return -1;

	int Index = pCD->m_Anim_ResourceMapper.Add(iResource);
	pCD->m_Anim_ResourceMapper.MakeDirty();

	m_Anim_ResourceMapperNames[Index] = Name;

	// Index 0 reserverd for AnimList
	return ((Index + 1) << 8) | iSeq;
}

CMat4Dfp32 CWObject_GameCore::GetSpawnPosition(int _Flags, const char *_pName, int16 *_pRetFlags, int16 *_pRetObj)
{
	CMat4Dfp32 Pos;
	Pos.Unit();

	TSelection<CSelection::LARGE_BUFFER> Selection;
	const int16* pSel;
	int nSel = 0;
	if(_pName != NULL)
	{
		m_pWServer->Selection_AddTarget(Selection, _pName);
		nSel = m_pWServer->Selection_Get(Selection, &pSel);
	}

	if(nSel == 0)
	{
		if (!m_DefaultSpawn.IsEmpty())
		{
			m_pWServer->Selection_AddTarget(Selection, m_DefaultSpawn.Str());
			m_pWServer->Selection_RemoveOnNotClass(Selection, "info_player_start");
		}
		else
		{
			m_pWServer->Selection_AddClass(Selection, "info_player_start");
		}
		nSel = m_pWServer->Selection_Get(Selection, &pSel);
	}
	
	if(nSel > 0)
	{
		CWObject *pFound = NULL;
//		CWObject *pDefault = NULL;
		for(int i = 0; i < nSel; i++)
		{
			CWObject *pObj = m_pWServer->Object_Get(pSel[i]);
			if(pObj->m_iAnim0 & INFOPLAYERSTART_FLAGS_DEFAULTSTART)
			{
				pFound = pObj;
				*_pRetObj = pSel[i];
				break;
			}
		}
		if(!pFound)
			pFound = m_pWServer->Object_Get(pSel[MRTC_RAND() % nSel]);

		if(pFound)
		{
			if(_pRetFlags)
				*_pRetFlags = pFound->m_iAnim0;
			Pos = pFound->GetPositionMatrix();
			Pos.k[3][2] += 0.01f;

			if(m_LastWorldChangeID != "")
			{
				if(!(pFound->m_iAnim0 & INFOPLAYERSTART_FLAGS_RESETDIRECTION))
				{
					CVec3Dfp32::GetRow(m_LastWorldPos, 0).SetRow(Pos, 0);
					CVec3Dfp32::GetRow(m_LastWorldPos, 1).SetRow(Pos, 1);
					CVec3Dfp32::GetRow(m_LastWorldPos, 2).SetRow(Pos, 2);
				}
				if(pFound->m_iAnim0 & INFOPLAYERSTART_FLAGS_RELATIVEPOS)
					CVec3Dfp32::GetRow(Pos, 3) += m_LastWorldOffset;
			}
		}
	}
	else
	{
		Pos.Unit();
		*_pRetObj = 0;
		ConOutL("§cf80WARNING: No player start positions on map.");
	}

	return Pos;
}


void CWObject_GameCore::OnPlayerEntersGame(int _iPlayerObj, int _iPlayerStartObj)
{
	if (_iPlayerStartObj > 0)
	{
		CWObject_Info_Player_Start* pInfoPlayerStartObj = TDynamicCast<CWObject_Info_Player_Start>(m_pWServer->Object_Get(_iPlayerStartObj));
		if (pInfoPlayerStartObj)
		{
			TAP<CWO_SimpleMessage> pMsg = pInfoPlayerStartObj->m_lMsg_OnSpawn;
			for (uint i = 0; i < pMsg.Len(); i++)
				pMsg[i].SendMessage(_iPlayerStartObj, _iPlayerObj, m_pWServer);
		}
	}

	if (m_lspPlayers.Len() == 1)
	{
		// When first player enters game, let NPC:s run their OnSpawn messages
		TSelection<CSelection::MEDIUM_BUFFER> Sel;
		m_pWServer->Selection_AddOnFlagsSet(Sel, OBJECT_FLAGS_CHARACTER);
		const int16* piObj = NULL;
		uint nObj = m_pWServer->Selection_Get(Sel, &piObj);
		for (uint i = 0; i < nObj; i++)
		{
			CWObject* pObj = m_pWServer->Object_Get(piObj[i]);
			if (!pObj) continue;

			CWObject_Character* pChar = safe_cast<CWObject_Character>(pObj);
			pChar->SendOnSpawnMessages();
		}
	}

	// Send impulse to all objects listed in 'm_AutoImpulse'
	TAP<const CStr> pAutoImpulse = m_lAutoImpulse;
	for (uint i = 0; i < pAutoImpulse.Len(); i++)
	{
		CFStr AutoImpulse = pAutoImpulse[i];
		while (!AutoImpulse.IsEmpty())
		{
			CFStr Target = AutoImpulse.GetStrMSep(",; ");
			Target.Trim();
			m_pWServer->Message_SendToTarget( CWObject_Message(OBJMSG_IMPULSE, 1), Target);
			AutoImpulse.Trim();
		}
	}
}


void CWObject_GameCore::OnDeltaSave(CCFile *_pFile)
{
	CClientData *pCD = GetClientData();

	// kma: save missions
	uint16 NumMissions = pCD->m_lMissionID.Len();
	_pFile->WriteLE(NumMissions);
	for(int32 i = 0; i < NumMissions; i++)
	{
		_pFile->WriteLE(pCD->m_lMissionID[i]);
		pCD->m_lMissionDesc[i].Write(_pFile);
		_pFile->WriteLE(pCD->m_lMissionFlags[i]);
	}
	_pFile->WriteLE(m_ClientFlags);
	NumMissions = m_lGameMessages.Len();
	_pFile->WriteLE(NumMissions);
	for (int32 i = 0; i < NumMissions; i++)
	{
		_pFile->Writeln(m_lGameMessages[i]);
		_pFile->WriteLE(m_lGameMessages_Duration[i]);
	}
}

void CWObject_GameCore::OnDeltaLoad(CCFile *_pFile, int _Flags)
{
	CClientData *pCD = GetClientData();
	if(!pCD)
		Error("CWObject_GameCore::OnDeltaSave()", "No client data, bailing");

	// Just to make sure we are not trying to stupid stuff
	m_PendingPause = -1;
	m_PendingWorldTick = -1;

	bool bKeepGame = (_Flags & SERVER_CHANGEWORLD_KEEPGAME) != 0;
	if(!bKeepGame)
	{
		pCD->m_lMissionID.Clear();
		pCD->m_lMissionDesc.Clear();
		pCD->m_lMissionFlags.Clear();
	}

	uint16 NumMissions;
	_pFile->ReadLE(NumMissions);

	for(int32 i = 0; i < NumMissions; i++)
	{
		uint16 MissionID, MissionFlags;
		CStr MissionDesc;

		_pFile->ReadLE(MissionID);
		MissionDesc.Read(_pFile);
		_pFile->ReadLE(MissionFlags);

		if(!bKeepGame)
		{
			pCD->m_lMissionID.Add(MissionID);
			pCD->m_lMissionDesc.Add(MissionDesc);
			pCD->m_lMissionFlags.Add(MissionFlags);
		}
	}

	pCD->m_lMissionID.MakeDirty();
	pCD->m_lMissionDesc.MakeDirty();
	pCD->m_lMissionFlags.MakeDirty();

	int32 ClientFlags;
	_pFile->ReadLE(ClientFlags);
	m_ClientFlags |= ClientFlags;
	_pFile->ReadLE(NumMissions);
	m_lGameMessages.SetLen(NumMissions);
	m_lGameMessages_Duration.SetLen(NumMissions);
	for (int32 i = 0; i < NumMissions; i++)
	{
		m_lGameMessages[i] = _pFile->Readln();
		_pFile->ReadLE(m_lGameMessages_Duration[i]);
	}

	m_DirtyMask |= GetClientData()->AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}


void CWObject_GameCore::OnFinishDeltaLoad()
{
	// (this is called after world has spawned and after all objects have been loaded / deleted by savedata)
	CWObject_Game::OnFinishDeltaLoad();
}


void CWObject_GameCore::OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject_Game::OnClientLoad(_pObj, _pWorld, _pFile, _pWData, _Flags);
	GetClientData(_pObj)->AutoVar_Read(_pFile, _pWData);
}

void CWObject_GameCore::OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject_Game::OnClientSave(_pObj, _pWorld, _pFile, _pWData, _Flags);
	GetClientData(_pObj)->AutoVar_Write(_pFile, _pWData);
}

CRct CWObject_GameCore::CalcInfoRectWidth(CRC_Util2D *_pUtil2D, wchar *_pTitle, CRC_Font *_pTitleFont, wchar *_pText, CRC_Font *_pFont, int _iSurfRes, int _SurfaceWidth, int _SurfaceHeight)
{
	int Width = 500 - _SurfaceWidth;
	int Height = 0;
	if(_pText && _pFont)
	{
		Height = int(_pFont->GetHeight(_pFont->GetOriginalSize(), _pText));

		wchar Lines[12][512];
//		const int MaxLines = 15;
		wchar* lpLines[12];
		for(int i = 0; i < 12; i++)
			lpLines[i] = &Lines[i][0];

		int Len = CStrBase::StrLen(_pText);
		int nLines = _pUtil2D->Text_WordWrap(_pFont, Width - 8, _pText, Len, lpLines, 12);
		if(nLines == 1)
			Width = int(_pFont->GetWidth(_pFont->GetOriginalSize(), _pText) + 8 + 4);
		else
			Height *= nLines;
		Height += 8;
	}
	else
		Width = 0;

	if(_pTitleFont && _pTitle)
		Width = MaxMT(Width, int(_pTitleFont->GetWidth(_pTitleFont->GetOriginalSize(), _pTitle) + 40));

	if(_iSurfRes != 0)
	{
		Width += _SurfaceWidth;
		Height = Max(_SurfaceHeight + 8, Height);
	}

	CRct Rect = _pUtil2D->GetVBM()->Viewport_Get()->GetViewRect();

	return CRct(0, 0, Width, Height);
}

void CWObject_GameCore::DrawInfoRect(CRC_Util2D *_pUtil2D, CWorld_Client *_pWClient, const CRct &_Rct, const wchar *_pTitle, CRC_Font *_pTitleFont, const wchar *_pText, CRC_Font *_pFont, int _iSurfRes, int _SurfaceWidth, int _SurfaceHeight, int _TitleExtraHeight, int _TextCol, int _Color1, int _Color2)
{
	int XPos = _Rct.p0.x;
	int YPos = _Rct.p0.y;
	int Width = _Rct.GetWidth() + 15;
	int Height = _Rct.GetHeight();
	int TitleHeight = 2;
	int TextHeight = 2;
	if(_pFont && _pText)
		TextHeight = int(_pTitleFont->GetHeight(_pFont->GetOriginalSize(), _pText));
	if(_pTitleFont && _pTitle)
		TitleHeight = int(_pTitleFont->GetHeight(_pTitleFont->GetOriginalSize(), _pTitle));
	TitleHeight += _TitleExtraHeight;

	int32 Color = (_Color1 ? CPixel32(_Color1) : CPixel32(0, 0, 0, 150));
	int32 Color2 = (_Color2 ? CPixel32(_Color2) : CPixel32(50, 50, 50, 150));

	_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	_pUtil2D->SetTexture(0);

	wchar Text[1024];
	Localize_Str(_pText, Text, 1023);
	int Len = CStrBase::StrLen(Text);

	wchar Lines[32][512];
	const int MaxLines = 31;
	wchar* lpLines[32];
	for(int i = 0; i < 32; i++) 
		lpLines[i] = &Lines[i][0];

	int nLines = _pUtil2D->Text_WordWrap(_pFont, Width - 8 - _SurfaceWidth, (wchar*) Text, Len, &lpLines[0], 31);

	int ExtraHeight = 7;

	CClipRect Clip(_Rct.p0.x, _Rct.p0.y - TitleHeight - 8, _Rct.p1.x + 15, _Rct.p1.y + nLines * (ExtraHeight + TextHeight));
/*	if (Height > 0)
	{
		_pUtil2D->Rect(Clip, CRct(XPos + 2, YPos, XPos + Width - 2, YPos + Height - 2), Color);
		_pUtil2D->Rect(Clip, CRct(XPos, YPos, XPos + 2, YPos + Height - 2), Color2);
		_pUtil2D->Rect(Clip, CRct(XPos + Width - 2, YPos, XPos + Width , YPos + Height - 2), Color2);
		_pUtil2D->Rect(Clip, CRct(XPos, YPos + Height - 2, XPos + Width, YPos + Height), Color2);
	}
	_pUtil2D->Rect(Clip, CRct(XPos, YPos - TitleHeight - 8, XPos + Width - 35, YPos), Color2);
	_pUtil2D->Rect(Clip, CRct(XPos + Width - 35, YPos - TitleHeight, XPos + Width, YPos), Color2);*/

	_pUtil2D->Rect(Clip, CRct(XPos, YPos - TitleHeight - 8, XPos + Width, YPos), Color2);
	_pUtil2D->Rect(Clip, CRct(XPos, YPos, XPos + Width, YPos + nLines * (ExtraHeight + TextHeight) + 10), Color);
	_pUtil2D->Rect(Clip, CRct(XPos, YPos + 1, XPos + Width, YPos + 3), CPixel32(255, 255, 255, 255));

/*	_pUtil2D->SetTexture("GUI_TRIANGLE");
	CPnt Pnt(XPos + Width - 35, YPos - TitleHeight - 8);
	_pUtil2D->SetTextureOrigo(Clip, Pnt);
	_pUtil2D->Rect(Clip, CRct(Pnt.x, Pnt.y, XPos + Width - 27, YPos - TitleHeight), Color2);
*/
	int32 TextCol = (_TextCol ? CPixel32(_TextCol) : CPixel32(255, 255, 255, 255));
	//int32 ShadowCol = CPixel32((226/8),(224/8),(220/8),255);
	if(_pFont && _pText)
		_pUtil2D->Text_DrawFormatted(Clip, _pFont, _pText, XPos + 15 + _SurfaceWidth, YPos + 10, WSTYLE_TEXT_WORDWRAP, TextCol, 0, 0, Width - 8 - _SurfaceWidth, Height - 8, false, ExtraHeight);
	if(_pTitleFont && _pTitle)
		_pUtil2D->Text_DrawFormatted(Clip, _pTitleFont, _pTitle, XPos, YPos - (TitleHeight + 2 - _TitleExtraHeight), WSTYLE_TEXT_CENTER | WSTYLE_TEXT_WORDWRAP, TextCol, 0, 0, Width - 8, TitleHeight + 6, false);

	//_pUtil2D->Flush();	
	if(_iSurfRes != 0)
	{
		int iSurf = _pWClient->GetMapData()->GetResource_SurfaceID(_iSurfRes);
		_pUtil2D->SetSurface(iSurf, CMTime::CreateFromSeconds(0));
		_pUtil2D->SetTextureOrigo(Clip, CPnt(XPos, YPos));
		_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / _SurfaceWidth, fp32(_pUtil2D->GetTextureHeight()) / _SurfaceHeight);
		_pUtil2D->Rect(Clip, CRct(XPos, YPos, XPos + _SurfaceWidth, YPos + _SurfaceHeight), 0xff808080);
		Width += _SurfaceWidth;
	}
}

void AddTriangle(uint16 _V1, uint16 _V2,uint16 _V3, uint16* _pIndex, uint16& _iIndex, uint16 _iStartVert = 0)
{
	_pIndex[_iIndex + 0] = _iStartVert + _V1;
	_pIndex[_iIndex + 1] = _iStartVert + _V2;
	_pIndex[_iIndex + 2] = _iStartVert + _V3;
	_iIndex += 3;
}

void AddVertex(uint16& _iVertex, CVec3Dfp32* _pVertexPos, const CVec3Dfp32& _Pos, CPixel32* _pVertexColor, int32 _Color, int32 _Alpha)
{
	_pVertexPos[_iVertex] = _Pos;
	if (_pVertexColor)
		_pVertexColor[_iVertex] = _Color | (_Alpha << 24);
	_iVertex++;
}

#define PINCIRCLESEGS 10
#define LINEWIDTH 3.0f
#define PINCIRCLERADIUS 3.5f
bool sbPreCalched = false;
static fp32 slPreCalcedCos[PINCIRCLESEGS+1] = {};
static fp32 slPreCalcedSin[PINCIRCLESEGS+1] = {};

void PrecalculatePinCircleAngles(const fp32& _BorderWidth, const fp32& _Radius)
{
	fp32 HalfBorder = _BorderWidth *(1.0f/2.0f);
	// These values should be precalced, borderwidth/radius as defines...

	fp32 StartAngle = M_ASin(HalfBorder/_Radius);
	fp32 Frag = (_PI2-StartAngle*2.0f) * (1.0f/ ((fp32)PINCIRCLESEGS));
	// Start with -45 degree angle + startpos (the pin stuff)
	StartAngle -= _PI*(3.0f/4.0f);

	for (int32 i = 0; i <= PINCIRCLESEGS; i++)
	{
		fp32 Angle = StartAngle + Frag * i;

		slPreCalcedCos[i] = M_Cos(Angle);
		slPreCalcedSin[i] = M_Sin(Angle);
	}
	sbPreCalched = true;
}

void CWObject_GameCore::DrawFocusFrame(CRC_Util2D *_pUtil2D, CWorld_Client *_pWClient)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys || !pSys->GetOptions()->GetValuei("FOCUSFRAME_ENABLED", 1))
		return;

	if (!sbPreCalched)
		PrecalculatePinCircleAngles(LINEWIDTH,PINCIRCLERADIUS);
	CWObject_Client* pObjPlayer = (CWObject_Client*)_pWClient->Object_GetCD(_pWClient->Player_GetLocalObject());
	CRegistry* pUserReg = _pWClient->Registry_GetUser();
	int bNoPredict = (pUserReg) ? pUserReg->GetValuei("NOPREDICT") : 0;
	if (!bNoPredict)
	{
		CWC_Player* pP = _pWClient->Player_GetLocal();
		if (!pP) return;

		if (pObjPlayer->GetNext() != NULL)
		{
			pObjPlayer = CWObject_Character::Player_GetLastValidPrediction(pObjPlayer);
		}
	}
	CWO_Character_ClientData* pCDPlayer = (pObjPlayer ? CWObject_Character::GetClientData(pObjPlayer) : NULL);
	if (pCDPlayer && (pCDPlayer->m_iFocusFrameObject != 0) && CWObject_Character::Char_IsPlayerViewControlled(pObjPlayer) &&
		!(CWObject_Character::Char_GetPhysType(pObjPlayer) == PLAYER_PHYS_DEAD) &&
		!(safe_cast<CWClient_Mod>(_pWClient))->m_bHideHUD)
	{
		CRC_Font* pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
		CStr UseText = pCDPlayer->m_FocusFrameUseText.m_Value.Str();
		CStr DescText = pCDPlayer->m_FocusFrameDescText.m_Value.Str();
		CClipRect Clip(0, 0, 640, 480);
//		int Alpha = 255 << 24;
//		int Col = 0x00ffffff | Alpha;
//		int ShadowCol = Alpha;

		CStr Temp = pCDPlayer->m_FocusFrameUseText.m_Value;
		const char* pFocusUseText = Temp.Str();
		// If the key doesn't exist, just return
		{
			char moo = '§';	// GCC workaround
			bool bHasLoc = (pFocusUseText && 
				(pFocusUseText[0] == moo && pFocusUseText[1] == 'L'));
			int i = (bHasLoc ? 2 : 0);
			// Remove any extra '§' 
			while(pFocusUseText[i] != '\0')
			{
				if (pFocusUseText[i] == moo)
				{
					Temp = Temp.DelFrom(i);
					pFocusUseText = Temp.Str();
					break;
				}
				i++;
			}

			if ((!bHasLoc && !Localize_KeyExists(pFocusUseText)) ||
				(bHasLoc && !Localize_KeyExists(pFocusUseText+2)))
				return;
		}
		Temp = pCDPlayer->m_FocusFrameUseText;
		pFocusUseText = Temp.Str();

		wchar UseBuffer[1024];
		UseBuffer[0] = 167;//(wchar)'§';
		UseBuffer[1] = 'Z';
		UseBuffer[2] = '2';
		UseBuffer[3] = '0';
		Localize_Str(pFocusUseText, UseBuffer+4, 1024);

		Temp = pCDPlayer->m_FocusFrameDescText;
		const char* pFocusDescText = Temp.Str();
		wchar DescBuffer[1024];
		// Check if description text should be used
		bool bFull = false; //(pFocusDescText[0] != '\0' && DescBuffer[0] != '\0');
		{
			char moo = '§';	// GCC workaround
			bool bHasLoc = (pFocusDescText && 
				(pFocusDescText[0] == moo && pFocusDescText[1] == 'L'));
			DescBuffer[0] = 167;//(wchar)'§';
			DescBuffer[1] = 'Z';
			DescBuffer[2] = '1';
			DescBuffer[3] = '5';
			if (bHasLoc && Localize_KeyExists(pFocusDescText+2))
			{
//				int32 Len = Temp.Len();
				Localize_Str(pFocusDescText, DescBuffer+4, 1020);
				bFull = (DescBuffer[0] != '\0');
			}
			else
			{
				DescBuffer[0] = '\0';
			}
		}
		CVec3Dfp32 ScreenPos(pCDPlayer->m_FocusFramePosX * 640.0f, pCDPlayer->m_FocusFramePosY * 480.0f,0.0f);
		int32 PinSlopeHeight = 32;
		int32 PinSlopeWidth = 34;
		int32 PinWidth = 32;
		int32 PinStartX = (int32)ScreenPos.k[0] - PinSlopeWidth - PinWidth;
		int32 PinStartY = (int32)ScreenPos.k[1] - PinSlopeHeight;

		int Width = int(pFont->GetWidth(18, UseBuffer) + 40);
		int32 Len = CStr::StrLen(DescBuffer);
		int32 Height = 0;
		// Text drawformatted returns nr of lines.... (change prio?)
		{
			Height = int32(pFont->GetHeight(15, DescBuffer));

			wchar Lines[12][512];
//			const int MaxLines = 15;
			wchar* lpLines[12];
			for(int i = 0; i < 12; i++)
				lpLines[i] = &Lines[i][0];

			int nLines = _pUtil2D->Text_WordWrap(pFont, Width - 8, DescBuffer, Len, lpLines, 12);
			Height *= nLines;
			Height += 8;
		}
		//int32 Height = Rect.p1.y - Rect.p0.y;//M_Ceil(((fp32)(Len * 15)) / ((fp32)(Width-12))) * (15.0f) + 6;
		int XStart = PinStartX - Width;//- 65;
		int YStart = PinStartY;

		bool bPin = true;

		//ConOut(CStrF("XStart: %d  YStart: %d", XStart,YStart));
		if (pCDPlayer->m_FocusFrameType & SELECTION_FLAG_INVALID || (XStart > 1000 || XStart < -500 || YStart > 1000 || YStart < -500))
		{
			bPin = false;
			CVec3Dfp32 FocusPos = _pWClient->Object_GetPosition(pCDPlayer->m_iFocusFrameObject);
			CVec3Dfp32 CharHeadPos = pObjPlayer->GetPosition();
			CharHeadPos.k[2] += (CWObject_Character::Char_GetPhysType(pObjPlayer) == PLAYER_PHYS_CROUCH ? 32 : 64);
			CVec3Dfp32 FocusDir = FocusPos - CharHeadPos;
			FocusDir.Normalize();
//			fp32 FwdDot = CVec3Dfp32::GetMatrixRow(pObjPlayer->GetPositionMatrix(),0) * FocusDir;
			fp32 RightDot = CVec3Dfp32::GetMatrixRow(pObjPlayer->GetPositionMatrix(),1) * FocusDir;
			fp32 UpDot = CVec3Dfp32::GetMatrixRow(pObjPlayer->GetPositionMatrix(),2) * FocusDir;

			//if (FwdDot < 0 )
				RightDot = -RightDot;
			XStart = int(320 + 320 * RightDot -Width - PinSlopeWidth - 60);
			if (UpDot)
				UpDot = -UpDot;
			YStart = int(240 + 240 * UpDot);// - Height;
		}
		
		if (XStart < 640 * 0.05f)
		{
			XStart = int(640 * 0.05f);
			bPin = false;
		}
		if ((XStart + Width) > 640*0.95f)
		{
			XStart = int(640*0.95f - Width);
			bPin = false;
		}
		if ((YStart-20) < 480 * 0.05f)
		{
			YStart = int(480 * 0.05f + 20);
			bPin = false;
		}
		if ((YStart + Height) > 480 * 0.95f)
		{
			YStart = int(480*0.95f - Height);
			bPin = false;
		}

		//XStart -= Width ;
		//YStart -= 32;
		int32 TextCol = CPixel32((226/2),(224/2),(220/2),255);
		int32 Color = CPixel32::From_fp32((80/2),(100/2),(115/2),0.60f * 255.0f);
		int32 Color2 = CPixel32::From_fp32((80/2),(100/2),(115/2),0.70f * 255.0f);
		DrawInfoRect(_pUtil2D, _pWClient, CRct(XStart, YStart, XStart + Width, bFull ? YStart + Height : YStart/* + 100*/),UseBuffer,pFont,DescBuffer,pFont,0,0,0,7,TextCol,Color,Color2);

		if (!bPin)
			return;

		// Draw line to object
		//_pUtil2D->
		CXR_VBManager* pVBM = _pUtil2D->GetVBM();
		if (!pVBM)
			return;
		CXR_VertexBuffer* pVB = pVBM->Alloc_VB();
		CVec3Dfp32* pVertexPos = NULL;
		CPixel32* pVertexColor = NULL;
		uint16* pIndex = NULL;
		int32 NumVerts = 6 + PINCIRCLESEGS+1;
		int32 NumTris = 5 + PINCIRCLESEGS+1;
		if (pVB)
		{
			pVertexPos = pVBM->Alloc_V3(NumVerts);
			pVertexColor = pVBM->Alloc_CPixel32(NumVerts);
			pIndex = pVBM->Alloc_Int16(NumTris*3);
			pVB->m_pAttrib = pVBM->Alloc_Attrib();
			if (!pVB->m_pAttrib || !pVertexPos || !pVertexColor || !pIndex || !pVB->AllocVBChain(pVBM, false) )
				return;
			*pVB->m_pAttrib	= *_pUtil2D->GetAttrib();
			pVB->Geometry_VertexArray(pVertexPos, NumVerts, true);
			pVB->Geometry_ColorArray(pVertexColor);
		}

		// Send to middle of screen
		
		int32 LineWidth = int32(LINEWIDTH);
		uint16 iIndex,iVertex;
		iIndex = iVertex = 0;
		int32 AlphaLine = int(0.70f * 255.0f);
		int32 ColorLine = Color;//CPixel32((80/2),(100/2),(115/2));
		/*AddTriangle(0,1,2,pIndex,iIndex);
		AddTriangle(0,2,3,pIndex,iIndex);
		AddVertex(iVertex,pVertexPos,CVec3Dfp32(20,20,0),pVertexColor,ColorLine,AlphaLine);
		AddVertex(iVertex,pVertexPos,CVec3Dfp32(200,20,0),pVertexColor,ColorLine,AlphaLine);
		fp32 A = ((fp32)LineWidth)*(1.0f/_SQRT2);
		AddVertex(iVertex,pVertexPos,CVec3Dfp32(200, 300, 0.0f),pVertexColor,ColorLine,AlphaLine);
		AddVertex(iVertex,pVertexPos,CVec3Dfp32(20,300,0),pVertexColor,ColorLine,AlphaLine);*/

		AddTriangle(0,1,4,pIndex,iIndex);
		AddTriangle(4,1,3,pIndex,iIndex);
		AddTriangle(3,1,2,pIndex,iIndex);
		AddTriangle(2,6,3,pIndex,iIndex);
		AddTriangle(3,6,16,pIndex,iIndex);
		AddVertex(iVertex,pVertexPos,CVec3Dfp32(PinStartX,PinStartY-LineWidth,0),pVertexColor,ColorLine,AlphaLine);
		AddVertex(iVertex,pVertexPos,CVec3Dfp32(PinStartX + PinWidth,PinStartY-LineWidth,0),pVertexColor,ColorLine,AlphaLine);
		fp32 A = ((fp32)LineWidth)*(1.0f/_SQRT2);
		AddVertex(iVertex,pVertexPos,CVec3Dfp32(PinStartX + PinWidth + A, PinStartY - A, 0.0f),pVertexColor,ColorLine,AlphaLine);
		AddVertex(iVertex,pVertexPos,CVec3Dfp32(PinStartX + PinWidth,PinStartY,0),pVertexColor,ColorLine,AlphaLine);
		AddVertex(iVertex,pVertexPos,CVec3Dfp32(PinStartX,PinStartY,0),pVertexColor,ColorLine,AlphaLine);

		// Do circle segs
		CVec3Dfp32 Dir(0.0f,0.0f,0.0f);

		// Add the segment triangles
		int32 CircleSegs = PINCIRCLESEGS;
		uint16 CircleIndexStart = iVertex;
		//fp32 CircleRadius = 5.0f;
		for (int32 i = 0; i < CircleSegs; i++)
		{
			AddTriangle(0, i+1, i+2, pIndex, iIndex, CircleIndexStart);
		}

		// Add a closing triangle
		AddTriangle(0, CircleSegs+1, 1, pIndex, iIndex, CircleIndexStart);

		// Add center vertex
		AddVertex(iVertex, pVertexPos, ScreenPos, pVertexColor, ColorLine, AlphaLine);
		for (int32 i = 0; i <= CircleSegs; i++)
		{
			//fp32 Angle = StartAngle + Frag * i;
			Dir.k[0] = slPreCalcedCos[i];//M_Cos(Angle);
			Dir.k[1] = slPreCalcedSin[i];//M_Sin(Angle);

			CVec3Dfp32 Pos = Dir * PINCIRCLERADIUS + ScreenPos;

			AddVertex(iVertex, pVertexPos, Pos, pVertexColor, ColorLine, AlphaLine);
		}


		CVec3Dfp32::MultiplyMatrix(pVertexPos, pVertexPos, _pUtil2D->GetTransform(), NumVerts);

		/*CMat4Dfp32 Unit;
		Unit.Unit();
		CMat4Dfp32 *pMat = pVBM->Alloc_M4(Unit);
		pVB->Matrix_Set(pMat);*/
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
		pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		pVB->Render_IndexedTriangles(pIndex, NumTris);
		pVBM->AddVB(pVB);
//		pVBM->Flush(_pUtil2D->GetRC(), 0);

		// Get bounding box size and set it to the animstate
		/*CWObject_CoreData* pFocusObject = _pWClient->Object_GetCD(pCDPlayer->m_iFocusFrameObject);
		if (pFocusObject)
		{
			CBox3Dfp32 Box;
			pFocusObject->GetVisBoundBox(Box);
			fp32 Radius = Max(M_Fabs(Min(Box.m_Min.k[0], Min(Box.m_Min.k[1],Box.m_Min.k[2]))),
			Max(Box.m_Max.k[0], Max(Box.m_Max.k[1], Box.m_Max.k[2])));

			CXR_AnimState AnimState;
			AnimState.m_AnimAttr0 = 0.5f;
			AnimState.m_AnimAttr1 = Radius;
			AnimState.m_pContext = pFont;
			AnimState.m_Colors[0] = (int32)pCDPlayer->m_FocusFrameUseText.m_Value.Str();
			AnimState.m_Colors[1] = (int32)pCDPlayer->m_FocusFrameDescText.m_Value.Str();
			// Offset 
			//AnimState.m_SurfAnim0 = (pCD->m_FocusFrameType == SELECTIONISCHAR ? 64 : 0);
			CMat4Dfp32 Pos = _pWClient->Object_GetPositionMatrix(pCDPlayer->m_iFocusFrameObject) ;
			if (pCDPlayer->m_FocusFrameType != SELECTIONISDEADCHAR)
			Pos.k[3][2] += pCDPlayer->m_FocusFrameOffset;//(pCD->m_FocusFrameType == SELECTIONISCHAR ? pCD->m_FocusFrameOffset : 0.0f);
			_pEngine->Render_AddModel(pFocusFrame, Pos, AnimState);
		}*/
	}
	return;
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Function:	 Creates a ragdoll from blueprint

    Parameters:
            _pGame:		"this" GameCore
			_pWServer:	Server object for physobjects
			_pPC:		Physcluster object
			_piMat:		List of Matrix ID:s (per bone)
			_iObj:		Object ID
			_lInvMat:	Inverted object positions
			_NameHash:	Hashed value of ragdoll name

    Returns:    true if successful, false if ragdoll not found
\*____________________________________________________________________*/ 
bool CWObject_GameCore::CreateRagdoll(const CWObject_GameCore *_pGame, CWorld_Server *_pServer,class CWPhys_Cluster *_pPC,uint8 *_piMat,
									  uint32 _iObj,TThinArray<CMat4Dfp32> &_lInvMat,uint32 _NameHash)
{
	//Use binary search to find ragdoll
	TAP<const CRagDollBpt> pRD = _pGame->m_lRagDollTypes;
	uint32 iRD;

	//Error check
	if( pRD.Len() == 0 ) return false;

	{
		uint32 iFirst = 0;
		uint32 iLast = pRD.Len()-1;

		while(iFirst != iLast)
		{
			int32 iSample = (iFirst + iLast) / 2;
			int32 Sample = pRD[iSample].m_Hash;

			if( _NameHash < Sample )
			{
				iLast = Max<int32>(0,iSample-1);
			}
			else if( _NameHash > Sample )
			{
				iFirst = Min<int32>(iSample+1,pRD.Len()-1);
			}
			else
			{
				iRD = iSample;
				break;
			}
		}

		if( iFirst == iLast )
		{
			if( pRD[iFirst].m_Hash == _NameHash )
				iRD = iFirst;
			else
				return false;
		}
	}

	//If we didn't get all the necessary parameters, just return true since the RD was found
	if( (!_pServer) || (!_piMat) || (!_pPC) ) return true;

	//Found ragdoll, create rigid bodies
	const CRagDollBpt &RD = pRD[iRD];
	TAP<const CRagDollBpt::CBody> pB = RD.m_lBodies;
	TAP<const CRagDollBpt::CConstraint> pC = RD.m_lConstraints;
	uint i;

	_pPC->m_lObjects.SetLen(pB.Len());
	_lInvMat.SetLen(pB.Len());

	for(i = 0;i < pB.Len();i++)
	{
		CWPhys_ClusterObject &PCO = _pPC->m_lObjects[i];

		PCO.m_PhysPrim.m_iPhysModel = i;
		PCO.m_PhysPrim.m_PrimType = pB[i].m_iType;
		PCO.m_PhysPrim.SetDim(pB[i].m_Dim);
		PCO.m_Velocity.Unit();
		PCO.m_Transform = pB[i].m_Transform;
		PCO.m_PhysPrim.m_ObjectFlagsMask = pB[i].m_iBone;

		_piMat[pB[i].m_iBone] = i;
		_lInvMat[i] = pB[i].m_InvTransform;
	}

	//Create RB:s
	_pServer->Object_InitRigidBody(_iObj,false);

	//Create constraints
	for(i = 0;i < pC.Len();i++)
	{
		switch( pC[i].m_iType )
		{

		case CWD_ConstraintDescriptor::BALL:
			_pServer->Phys_AddBallJointConstraint(_iObj,_iObj,pC[i].m_Transform.GetRow(3),pC[i].m_Param.k[1],pC[i].m_iSub1,pC[i].m_iSub2);
			break;

		case CWD_ConstraintDescriptor::HINGE:
			_pServer->Phys_AddHingeJointConstraint(_iObj,_iObj,pC[i].m_Transform,pC[i].m_Param.k[0],pC[i].m_Param.k[1],
				pC[i].m_iSub1,pC[i].m_iSub2);
			break;

		case CWD_ConstraintDescriptor::HINGE2:
			_pServer->Phys_AddHingeJointConstraint2(_iObj,_iObj,pC[i].m_Transform,pC[i].m_Param.k[0],pC[i].m_Param.k[1],
				pC[i].m_Param.k[2],pC[i].m_iSub1,pC[i].m_iSub2);

		}
	}

	return true;
}
