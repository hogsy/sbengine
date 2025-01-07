#include "PCH.h"
#include "../WObj_Game/WObj_GameMod.h"
#include "../WObj_Misc/WObj_EffectSystem.h"
#include "../../GameWorld/WClientMod.h"
#include "../../GameWorld/WServerMod.h"
#include "../WObj_Char/WObj_CharDarkling_ClientData.h"
#include "../WObj_Char/WObj_CharShapeshifter.h"
#include "../WObj_Sys/WObj_Trigger.h"
#include "../wrpg/WRPGChar.h"
#include "../wrpg/WRPGItem.h"
#include "../wrpg/WRPGWeapon.h"
#include "../../GameWorld/WFrontEndMod.h"
#include "../../Exe/WGameContextMain.h"
#include "../../Exe_Xenon/Darkness.spa.h"

#define SPAWNCLASS "player"
//#define SPAWNCLASS "ai_bigalien"
//#define SPAWNCLASS "ai_riotguard"
//#define SPAWNCLASS "ai_heavyguard"
//#define SPAWNCLASS "ai_attackdroid"
//#define SPAWNCLASS "ai_exoskeleton"
//#define SPAWNCLASS "ai_screamerdroid"

#define SCOREBOARD_BACKGROUND	0xc0000000
#define SCOREBOARD_BORDER		0xff404040
#define SCOREBOARD_TEXTCOLOR	0xffffffff
#define SCOREBOARD_TEXTSHADOW	0xff000000

#ifdef M_Profile
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_GameDebug, CWObject_GameP4, 0x0100);
#endif
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_GameCampaign, CWObject_GameP4, 0x0100);
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_GameDM, CWObject_GameP4, 0x0100);
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_GameTDM, CWObject_GameDM, 0x0100);
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_GameCTF, CWObject_GameTDM, 0x0100);
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_GameSurvivor, CWObject_GameDM, 0x0100);
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_GameLastHuman, CWObject_GameSurvivor, 0x0100);

//#pragma optimize("", off)
//#pragma inline_depth(0)

#ifdef PLATFORM_DOLPHIN
void GameCube_DestroyStuff();
void GameCube_DestroyStuff()
{
#ifndef M_RTM
	OSReport("GameCube_DestroyStuff()\n");
#endif	

	CDisplayContextDolphin* pDC = CDisplayContextDolphin::Get();
	CRenderContextDolphin* pRC = safe_cast<CRenderContextDolphin>(pDC->GetRenderContext());

	// Destroy all vertexbuffers and textures
	pRC->NukeVBs();
	pRC->NukeTextures();

	// Destroy sounds
	MACRO_GetRegisterObject(CSoundContext_Dolphin, pSound, "SYSTEM.SOUND");
	if (pSound)
		pSound->DestroyAllSounds(false); // not streams

	CAI_Core::FreeStaticAlloations();
}

#elif	defined( PLATFORM_PS2 )

#include "RndrPs2/MDispPS2.h"

void PS2_DestroyStuff();
void PS2_DestroyStuff()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CDisplayContext*pDisplay = pSys->m_spDisplay;
	CDisplayContextPS2* pDC = safe_cast<CDisplayContextPS2>(pDisplay);
	CRenderContextPS2* pRC = safe_cast<CRenderContextPS2>(pDC->GetRenderContext());

	if( pRC )
	{

		pRC->Render_PrecacheFlush();
		// Destroy all vertexbuffers
		pRC->Geometry_PrecacheFlush();
		
		// Destroy all textures
		pRC->Texture_PrecacheFlush();
	}
	
	MACRO_GetRegisterObject(CSoundContext, pSound, "SYSTEM.SOUND");
	if (pSound)
	{
		pSound->Wave_PrecacheFlush();
	}

//	CAI_Core::FreeStaticAlloations();
}
#endif


static const char* GetBindingGUITexture(int _ScanCode)
{
#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
	switch (_ScanCode)
	{
	case SKEY_JOY_AXIS01_POS:
	case SKEY_JOY_AXIS01_NEG:
	case SKEY_JOY_AXIS00_NEG:
	case SKEY_JOY_AXIS00_POS:	return "GUI_Button_L";

	case SKEY_JOY_AXIS03_POS:
	case SKEY_JOY_AXIS03_NEG:
	case SKEY_JOY_AXIS02_NEG:
	case SKEY_JOY_AXIS02_POS:	return "GUI_Button_R";

	case SKEY_JOY_BUTTON00:		return "GUI_Button_A";
	case SKEY_JOY_BUTTON01:		return "GUI_Button_B";
	case SKEY_JOY_BUTTON02:		return "GUI_Button_X";
	case SKEY_JOY_BUTTON03:		return "GUI_Button_Y";
	case SKEY_JOY_BUTTON04:		return "GUI_Button_Start";
	case SKEY_JOY_BUTTON05:		return "GUI_Button_Back";
	case SKEY_JOY_BUTTON06:		return "GUI_Button_RC";
	case SKEY_JOY_BUTTON07:		return "GUI_Button_LC";
	case SKEY_JOY_BUTTON08:		return "GUI_Button_LB";
	case SKEY_JOY_BUTTON09:		return "GUI_Button_RB";
	case SKEY_JOY_BUTTON0A:		return "GUI_Button_LT";
	case SKEY_JOY_BUTTON0B:		return "GUI_Button_RT";

	case SKEY_JOY_POV00:		return "GUI_Button_DUP";
	case SKEY_JOY_POV01:		return "GUI_Button_DRight";
	case SKEY_JOY_POV02:		return "GUI_Button_DDown";
	case SKEY_JOY_POV03:		return "GUI_Button_DLeft";
	}

#else // PC
	switch (_ScanCode)
	{
	case SKEY_JOY_AXIS01_POS:
	case SKEY_JOY_AXIS01_NEG:
	case SKEY_JOY_AXIS00_NEG:
	case SKEY_JOY_AXIS00_POS:	return "GUI_Button_L";

	case SKEY_JOY_AXIS03_POS:
	case SKEY_JOY_AXIS03_NEG:
	case SKEY_JOY_AXIS02_NEG:
	case SKEY_JOY_AXIS02_POS:	return "GUI_Button_R";

	case SKEY_JOY_BUTTON00:		return "GUI_Button_A";
	case SKEY_JOY_BUTTON01:		return "GUI_Button_B";
	case SKEY_JOY_BUTTON02:		return "GUI_Button_X";
	case SKEY_JOY_BUTTON03:		return "GUI_Button_Y";
	case 164:					return "GUI_Button_LB";
	case 165:					return "GUI_Button_RB";
	case 166:					return "GUI_Button_Back";
	case 167:					return "GUI_Button_Start";
//	case -1:					return "GUI_Button_RC";
	case 168:					return "GUI_Button_LC";
	case 137:					return "GUI_Button_RT";
	case 136: 					return "GUI_Button_LT";

	case SKEY_JOY_POV00:		return "GUI_Button_DUP";
	case SKEY_JOY_POV01:		return "GUI_Button_DRight";
	case SKEY_JOY_POV02:		return "GUI_Button_DDown";
	case SKEY_JOY_POV03:		return "GUI_Button_DLeft";
	}
#endif 

	return NULL;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_GameP4
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_GameP4::CWObject_GameP4()
{
	for (int i = 0; i < GAME_NUM_AIPARAM; i++)
		m_lAIParams[i] = 0;

	//Set these values high as default... optimize by setting values in registry
	m_lAIParams[GAME_AIPARAM_NUMPURSUITSALLOWED] = 3; //3 is good optimized value
	m_lAIParams[GAME_AIPARAM_NUMMELEEALLOWED] = 2;	 //2 is good optimized value

	m_MinimumClearanceLevel = 0;
	m_Gamestyle = GAMESTYLE_SNEAK;
	m_DefaultGamestyle = m_Gamestyle;
	
	for (int i = 0; i < NUM_DIFFICULTYLEVELS; i++)
	{
		m_DamageTweak_Melee[i] = 1.0f;
		m_DamageTweak_Ranged[i] = 1.0f;
		m_GlobalDamageTweak_Melee[i] = 1.0f;
		m_GlobalDamageTweak_Ranged[i] = 1.0f;
	}

	m_bLightsOut = false;
	m_bNoHintIcon = false;
	m_bRailForgetEnemies = false;
	
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	
	m_DifficultyLevel = DIFFICULTYLEVEL_NORMAL;	// Normal by default
	m_iRailObject = -1; // doesn't exist
}

void CWObject_GameP4::OnCreate()
{
	CWObject_GameCore::OnCreate();
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	int SpawnMask = -1;

	m_AIResources.Init(m_pWServer);

	// Reset for each level
	m_Gamestyle = GAMESTYLE_SNEAK;
	m_DefaultGamestyle = m_Gamestyle;

	for (int i = 0; i < NUM_DIFFICULTYLEVELS; i++)
	{
		m_DamageTweak_Melee[i] = 1.0f;
		m_DamageTweak_Ranged[i] = 1.0f;
	}

	if (pSys)
	{
		CRegistry* pOptionsRegistry = pSys->GetOptions();
		if (pOptionsRegistry)
		{
			CRegistry* pDifficultyLevel = pOptionsRegistry->Find("GAME_DIFFICULTY");
			if (pDifficultyLevel)
			{
				int32 DifficultyLevel = pDifficultyLevel->GetThisValuei();
				if (DifficultyLevel >= DIFFICULTYLEVEL_EASY && DifficultyLevel <= DIFFICULTYLEVEL_COMMENTARY)
				{
					m_DifficultyLevel = DifficultyLevel;
					SpawnMask = 1<<(DifficultyLevel-1);
				}
				else
				{
					ConOut("No valid difficultysetting selected. Selecting NORMAL");
					pDifficultyLevel->SetThisKey("DIFFICULTYLEVEL", "NORMAL");
					m_DifficultyLevel = DIFFICULTYLEVEL_NORMAL;
					SpawnMask = 2;
				}

				ConOut(CStrF("Difficultylevel %d", DifficultyLevel));
			}
			else
			{
				ConOut("ERROR: Difficulty option net set");
				m_DifficultyLevel = DIFFICULTYLEVEL_NORMAL;
				SpawnMask = 1;
			}
		}

		// Only update difficulty flags (including commentary)
		m_pWServer->m_CurSpawnMask &= ~SERVER_SPAWNFLAGS_DIFFICULTY_MASK;
		m_pWServer->m_CurSpawnMask |= (SpawnMask & SERVER_SPAWNFLAGS_DIFFICULTY_MASK);
	}

	m_StartLevel = m_pWServer->m_WorldName;
}

void CWObject_GameP4::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MSCOPE(CWObject_GameCore::OnEvalKey, GAMECORE);

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	int KeyValuei = KeyValue.Val_int();
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();

	switch (_KeyHash)
	{
	case MHASH4('AI_N','UMPU','RSUI','TS'): // "AI_NUMPURSUITS"
		{
			m_lAIParams[GAME_AIPARAM_NUMPURSUITSALLOWED] = KeyValuei;
			break;
		}
	
	case MHASH3('AI_N','UMME','LEE'): // "AI_NUMMELEE"
		{
			m_lAIParams[GAME_AIPARAM_NUMMELEEALLOWED] = KeyValuei;
			break;
		}

	case MHASH1('AI'): // "AI"
		{
			//Let AI resource handler handle all of these keys
			for (int i = 0; i < _pKey->GetNumChildren(); i++)
			{
				const CRegistry* pReg = _pKey->GetChild(i);
				m_AIResources.OnEvalKey(pReg->GetThisNameHash(), pReg);
			}
			break;
		}

	case MHASH3('DEFA','ULTT','EAMS'): // "DEFAULTTEAMS"
		{
			// Add to default team templates
			CStr Teams = KeyValue;
			CStr Team;
			while ((Team = Teams.GetStrSep(";")) != CStr(""))
			{
				int i;
				for(i = 0; i < m_lDefaultTeams.Len(); i++)
					if(m_lDefaultTeams[i] == Team)
						break;

				if(i == m_lDefaultTeams.Len())
					m_lDefaultTeams.Add(Team);
			}
			break;
		}
	case MHASH7('GLOB','AL_T','WEAK','_DAM','AGE_','MELE','E'): // "GLOBAL_TWEAK_DAMAGE_MELEE"
		{
			if (KeyValuef > 0.0f)
			{
				// This will tweak all difficulty levels
				if (KeyValuef > 0.0f)
				{
					for (int i = 0; i < NUM_DIFFICULTYLEVELS; i++)
					{
						m_GlobalDamageTweak_Melee[i] = KeyValuef;
					}
				}
			}

			// Find individual difficulty settings if any
			for (int i = 0; i < _pKey->GetNumChildren(); i++)
			{
				const CRegistry* pChild = _pKey->GetChild(i);
				if (pChild)
				{
					CStr ChildName = pChild->GetThisName();
					uint32 ChildHash = StringToHash(ChildName);				
					CStr ChildValue = pChild->GetThisValue();
					fp32 ChildValuef = (fp32)ChildValue.Val_fp64();
				
				
					if (ChildHash == MHASH1('EASY'))
					{
						m_GlobalDamageTweak_Melee[DIFFICULTYLEVEL_EASY] = ChildValuef;
					}
					else if (ChildHash == MHASH2('NORM','AL'))
					{
						m_GlobalDamageTweak_Melee[DIFFICULTYLEVEL_NORMAL] = ChildValuef;
					}
					else if (ChildHash == MHASH1('HARD'))
					{
						m_GlobalDamageTweak_Melee[DIFFICULTYLEVEL_HARD] = ChildValuef;
					}
					else if (ChildHash == MHASH3('COMM','ENTA','RY'))
					{
						m_GlobalDamageTweak_Melee[DIFFICULTYLEVEL_COMMENTARY] = ChildValuef;
					}
				}
			}
			break;
		}
	case MHASH7('GLOB','AL_T','WEAK','_DAM','AGE_','RANG','ED'): // "GLOBAL_TWEAK_DAMAGE_RANGED"
		{
			if (KeyValuef > 0.0f)
			{
				// This will tweak all difficulty levels
				if (KeyValuef > 0.0f)
				{
					for (int i = 0; i < NUM_DIFFICULTYLEVELS; i++)
					{
						m_GlobalDamageTweak_Ranged[i] = KeyValuef;
					}
				}
			}

			// Find individual difficulty settings if any
			for (int i = 0; i < _pKey->GetNumChildren(); i++)
			{
				const CRegistry* pChild = _pKey->GetChild(i);
				if (pChild)
				{
					CStr ChildName = pChild->GetThisName();
					uint32 ChildHash = StringToHash(ChildName);				
					CStr ChildValue = pChild->GetThisValue();
					fp32 ChildValuef = (fp32)ChildValue.Val_fp64();
					if (ChildHash == MHASH1('EASY'))
					{
						m_GlobalDamageTweak_Ranged[DIFFICULTYLEVEL_EASY] = ChildValuef;
					}
					else if (ChildHash == MHASH2('NORM','AL'))
					{
						m_GlobalDamageTweak_Ranged[DIFFICULTYLEVEL_NORMAL] = ChildValuef;
					}
					else if (ChildHash == MHASH1('HARD'))
					{
						m_GlobalDamageTweak_Ranged[DIFFICULTYLEVEL_HARD] = ChildValuef;
					}
					else if (ChildHash == MHASH3('COMM','ENTA','RY'))
					{
						m_GlobalDamageTweak_Ranged[DIFFICULTYLEVEL_COMMENTARY] = ChildValuef;
					}
				}
			}
			break;
		}
	case MHASH6('MINI','MUMC','LEAR','ANCE','LEVE','L'): // "MINIMUMCLEARANCELEVEL"
		{
			m_MinimumClearanceLevel = KeyValuei;
			break;
		}

	case MHASH3('LIGH','TSOU','T'): // "LIGHTSOUT"
		{
			m_bLightsOut = KeyValuei != 0;
			break;
		}

	case MHASH3('NOHI','NTIC','ON'): // "NOHINTICON"
		{
			m_bNoHintIcon = true;
			break;
		}

	case MHASH5('RAIL','FORG','ETEN','EMIE','S'): // "RAILFORGETENEMIES"
		{
			m_bRailForgetEnemies = (KeyValuei != 0);
			break;
		}

	case MHASH6('NIGH','TVIS','IONL','IGHT','RANG','E'): // "NIGHTVISIONLIGHTRANGE"
		{
			SetNVRange(KeyValuei, 0);
			break;
		}
	case MHASH3('GAME','STYL','E'): // "GAMESTYLE"
		{
			const CStr Value = KeyValue.UpperCase();
			uint32 ValueHash = StringToHash(Value);

			if (ValueHash == MHASH2('SNEA','K'))
			{
				m_Gamestyle = GAMESTYLE_SNEAK;
			}
			else if (ValueHash == MHASH2('FIGH','T'))
			{
				m_Gamestyle = GAMESTYLE_FIGHT;
			}
			else if (ValueHash == MHASH3('ADVE','NTUR','E'))
			{
				m_Gamestyle = GAMESTYLE_ADVENTURE;
			}
			else if (ValueHash == MHASH1('KILL'))
			{
				m_Gamestyle = GAMESTYLE_KILL;
			}
			m_DefaultGamestyle = m_Gamestyle;
			break;
		}
	case MHASH5('TWEA','K_DA','MAGE','_MEL','EE'): // "TWEAK_DAMAGE_MELEE"
		{	// This will tweak all difficulty levels
			if (KeyValuef > 0.0f)
			{
				for (int i = 0; i < NUM_DIFFICULTYLEVELS; i++)
				{
					m_DamageTweak_Melee[i] = KeyValuef;
				}
			}

			for (int i = 0; i < _pKey->GetNumChildren(); i++)
			{
				const CRegistry* pChild = _pKey->GetChild(i);
				if (pChild)
				{
					CStr ChildName = pChild->GetThisName();
					uint32 ChildHash = StringToHash(ChildName);				
					CStr ChildValue = pChild->GetThisValue();
					fp32 ChildValuef = (fp32)ChildValue.Val_fp64();
					if (ChildHash == MHASH1('EASY'))
					{
						m_DamageTweak_Melee[DIFFICULTYLEVEL_EASY] = ChildValuef;
					}
					else if (ChildHash == MHASH2('NORM','AL'))
					{
						m_DamageTweak_Melee[DIFFICULTYLEVEL_NORMAL] = ChildValuef;
					}
					else if (ChildHash == MHASH1('HARD'))
					{
						m_DamageTweak_Melee[DIFFICULTYLEVEL_HARD] = ChildValuef;
					}
					else if (ChildHash == MHASH3('COMM','ENTA','RY'))
					{
						m_DamageTweak_Melee[DIFFICULTYLEVEL_COMMENTARY] = ChildValuef;
					}
				}
			}
			break;
		}
	case MHASH5('TWEA','K_DA','MAGE','_RAN','GED'): // "TWEAK_DAMAGE_RANGED"
		{	// This will tweak all difficulty levels
			if (KeyValuef > 0.0f)
			{
				for (int i = 0; i < NUM_DIFFICULTYLEVELS; i++)
				{
					m_DamageTweak_Ranged[i] = KeyValuef;
				}
			}

			for (int i = 0; i < _pKey->GetNumChildren(); i++)
			{
				const CRegistry* pChild = _pKey->GetChild(i);
				if (pChild)
				{
					CStr ChildName = pChild->GetThisName();
					uint32 ChildHash = StringToHash(ChildName);				
					CStr ChildValue = pChild->GetThisValue();
					fp32 ChildValuef = (fp32)ChildValue.Val_fp64();
					if (ChildHash == MHASH1('EASY'))
					{
						m_DamageTweak_Ranged[DIFFICULTYLEVEL_EASY] = ChildValuef;
					}
					else if (ChildHash == MHASH2('NORM','AL'))
					{
						m_DamageTweak_Ranged[DIFFICULTYLEVEL_NORMAL] = ChildValuef;
					}
					else if (ChildHash == MHASH1('HARD'))
					{
						m_DamageTweak_Ranged[DIFFICULTYLEVEL_HARD] = ChildValuef;
					}
					else if (ChildHash == MHASH3('COMM','ENTA','RY'))
					{
						m_DamageTweak_Ranged[DIFFICULTYLEVEL_COMMENTARY] = ChildValuef;
					}
				}
			}
			break;
		}
	case MHASH2('AUTO','SAVE'): // "AUTOSAVE"
		{
			m_AutoSave = KeyValue;
			break;
		}

	default:
		{
			CWObject_GameCore::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_GameP4::OnRefresh()
{
	MSCOPESHORT(CWObject_GameP4::OnRefresh);
	CWObject_GameCore::OnRefresh();
	
	m_AIResources.OnRefresh();
	for(int i = 0; i < m_lspPlayers.Len(); i++)
	{
		CWO_Player *pPlayer = Player_Get(i);
		if(pPlayer)
		{
			if(pPlayer->m_Mode == PLAYERMODE_DEAD)
			{
				//CWObject_Character *pObj = (CWObject_Character *)m_pWServer->Object_Get(pPlayer->m_iObject);
				if(/*(!pObj || (pObj->m_PendingCutsceneTick == -1 &&
				   !(pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE))) &&*/
				   (GetClientData()->GetCurScreenFade(m_pWServer->GetGameTick(), 0) & 0xff000000) >= 0xfc000000)
				{
					if ((m_LastLoadTick != -1) && (m_LastSave.CompareNoCase("Quick") == 0) && (m_pWServer->GetGameTick() < (m_LastLoadTick + m_pWServer->GetGameTicksPerSecond() * (3 + 6))))
						m_LastSave = "Checkpoint";

					if(m_LastSave != "")
					{
						ConExecute("deferredscriptgrabscreen(\"loadgame (\\\"" + m_LastSave +  "\\\")\")");
						m_bWaitingForChangeWorldScript = true;
					}
					else
						ConExecute("changemap(\"" + m_StartLevel + "\", 4)"); // Keep resource
					pPlayer->m_Mode = PLAYERMODE_WAITINGFORRESPAWN;

					MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
					CGameContextMod *pGameMod = TDynamicCast<CGameContextMod>(pGame);
					if(pGameMod)
					{
						CWFrontEnd_Mod *pFrontEnd = ((CWFrontEnd_Mod *)((CWFrontEnd *)pGameMod->m_spFrontEnd));
						CStr Past, Present, Future;
						Past = m_DeathSeqPast;
						Present = m_DeathSeqPresent;
						Future = m_DeathSeqFuture;

						int Num = m_DeathSeqPast.GetNumMatches(",") + 1;
						if(Num)
						{
							int Selected = MRTC_RAND() % Num;
							for(int i = 0; i < Selected; i++)
								Past.GetStrSep(",");
						}

						Num = m_DeathSeqPresent.GetNumMatches(",") + 1;
						if(Num)
						{
							int Selected = MRTC_RAND() % Num;
							for(int i = 0; i < Selected; i++)
								Present.GetStrSep(",");
						}

						Num = m_DeathSeqFuture.GetNumMatches(",") + 1;
						if(Num)
						{
							int Selected = MRTC_RAND() % Num;
							for(int i = 0; i < Selected; i++)
								Future.GetStrSep(",");
						}

						pFrontEnd->m_Cube.SetDeathScene(Past.GetStrSep(","), Present.GetStrSep(","), Future.GetStrSep(","), m_DeathSeqSound);
					}
				}
			}
		}
	}
}

aint CWObject_GameP4::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAME_GETAIPARAM:
		{
			if ((_Msg.m_Param0 >= 0) && (_Msg.m_Param0 < GAME_NUM_AIPARAM))
				return m_lAIParams[_Msg.m_Param0];
			else
				return 0;
		}

	case OBJMSG_GAME_GETGAMESTYLE:
		{
			return(m_Gamestyle);
		}
	case OBJMSG_GAME_SETGAMESTYLE:
		{
			if ((_Msg.m_Param0 >= GAMESTYLE_SNEAK)&&(_Msg.m_Param0 >= GAMESTYLE_KILL))
			{
				m_Gamestyle = _Msg.m_Param0;
			}
			else if (_Msg.m_Param0 == GAMESTYLE_RESET)
			{
				m_Gamestyle = m_DefaultGamestyle;
			}
			return(1);
		}

	case OBJMSG_GAME_TWEAK_DAMAGE_MELEE:
		{
			return aint(1024.0f * m_GlobalDamageTweak_Melee[m_DifficultyLevel] * m_DamageTweak_Melee[m_DifficultyLevel]);
		}

	case OBJMSG_GAME_TWEAK_DAMAGE_RANGED:
		{
			return aint(1024.0f * m_GlobalDamageTweak_Ranged[m_DifficultyLevel] * m_DamageTweak_Ranged[m_DifficultyLevel]);
		}

	case OBJMSG_GAME_PAUSEALLAI:
	{
		//Due to ogier message default, this must be counterinituitive
		if (_Msg.m_Param0 == 0)
			m_AIResources.Pause(CAI_ResourceHandler::PAUSE_SCRIPT);
		else
			m_AIResources.Unpause(CAI_ResourceHandler::PAUSE_SCRIPT);
		return 1;
	}

	case OBJMSG_GAME_GETAIRESOURCES:
		return (aint)(&m_AIResources);

	case OBJMSG_GAME_GETMINIMUMCLEARANCELEVEL:
		return m_MinimumClearanceLevel;

	case OBJMSG_GAME_GETLIGHTSOUT:
		return m_bLightsOut;

	case OBJMSG_GAME_SETDIFFICULTYLEVEL:
		m_DifficultyLevel = _Msg.m_Param0;
		return 1;

	case OBJMSG_GAME_GETDIFFICULTYLEVEL:
		return m_DifficultyLevel;

	case OBJMSG_GAMEP4_SETMISSION:
		return SetMission(_Msg.m_Param0, (const char *)_Msg.m_pData);

	case OBJMSG_GAMEP4_COMPLETEMISSION:
		return RemoveMission(_Msg.m_Param0, true);

	case OBJMSG_GAMEP4_FAILMISSION:
		return RemoveMission(_Msg.m_Param0, false);
	
	case OBJMSG_GAMEP4_CHECKPOINT:
		{
			//Move out tension check to it's own if with more precision, kill the tension check in trigger_checkpoint
			if(GetPlayerStatus() & (PLAYER_STATUS_DEAD | /*PLAYER_STATUS_TENSION |*/ PLAYER_STATUS_NOTPLAYERVIEW))
				return 0;

			fp32 Tension = 0;
			CWObject_Message Msg(OBJMSG_CHAR_GETTENSION);
			Msg.m_pData = &Tension;
			Msg.m_DataSize = sizeof(Tension);
			m_pWServer->Message_SendToObject(Msg, Player_GetObjectIndex(0));

			if(Tension > 0.2f && !_Msg.m_Param0)
				return 0;

			if(m_LastSaveTick != -1 && (m_LastSaveTick + m_pWServer->GetGameTicksPerSecond() * 5) > m_pWServer->GetGameTick())
				return 0;

			CStr DeathSeqs = (char *)_Msg.m_pData;
			m_DeathSeqPast = DeathSeqs.GetStrSep("/");
			m_DeathSeqPresent = DeathSeqs.GetStrSep("/");
			m_DeathSeqFuture = DeathSeqs.GetStrSep("/");
			m_DeathSeqSound = DeathSeqs;

			CStr Savename = "Checkpoint";
			m_LastSave = Savename;
			m_pWServer->World_Save(m_LastSave);
			m_LastSaveTick = m_pWServer->GetGameTick();
			return 1;
		}

	case OBJMSG_GAMEP4_SETNVRANGE:
		{
			int Range = _Msg.m_Param0;
			if(Range == 0)
				Range = 512;
			if(_Msg.m_pData)
				SetNVRange(Range, M_AToF((const char *)_Msg.m_pData) / 1000);
			else
				SetNVRange(Range, 0.15f);
			return 1;
		}
	case OBJMSG_GAMEP4_SAVEPRECACHEINFO:
		{
			CWServer_Mod *pServer = safe_cast<CWServer_Mod >(m_pWServer);
			pServer->m_GameState.PrecacheInfo((char *)_Msg.m_pData, _Msg.m_Param0);
			return 1;
		}
	case OBJMSG_GAME_LOADPRECACHEINFO:
		{
			char *_pSt = (char *)_Msg.m_pData;
			switch(_Msg.m_Param0)
			{
				case PRECACHEINFO_RPG:
					CRPG_Object::IncludeRPGClass(_pSt, m_pWServer->GetMapData(), m_pWServer, true);
					break;
			}
			return 1;
		}
	case OBJMSG_GAME_LOADANIMATIONS:
		{
			CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
			if (pServerMod)
				pServerMod->LoadAnimations();
			return 1;
		}
	case OBJSYSMSG_GAME_POSTWORLDLOAD:
		{
			if(_Msg.m_pData)
			{
				m_LastSave = (char *)_Msg.m_pData;
				m_LastLoadTick = m_pWServer->GetGameTick();
			}
			return 1;
		}

	case OBJSYSMSG_GAME_POSTWORLDSAVE:
		{
			if(_Msg.m_pData)
				m_LastSave = (char *)_Msg.m_pData;
			
			// Don't disable checkpoints due to quicksave
			//m_LastSaveTick = m_pWServer->GetGameTick();
			return 1;
		}

	case OBJMSG_RAIL_REGISTER:
		{
			m_iRailObject = _Msg.m_iSender;
			return 1;
		}
	case OBJMSG_RAIL_POWER:
	case OBJMSG_RAIL_NUMBYTYPE:
	case OBJMSG_RAIL_NUMBYNAME:
	case OBJMSG_RAIL_SPAWN:
	case OBJMSG_RAIL_ENEMY:
	case OBJMSG_RAIL_UNFRIENDLY:
	case OBJMSG_RAIL_ATTACK:
		{	
			return m_pWServer->Message_SendToObject(_Msg, m_iRailObject);
		}

	case OBJSYSMSG_PRECACHEMESSAGE:
		{
			if(((CWObject_Message *)_Msg.m_pData)->m_Msg == OBJMSG_RAIL_SPAWN)
				return m_pWServer->Message_SendToObject(_Msg, m_iRailObject);
			break;
		}
	case OBJMSG_GAME_QUITTOMENU:
		if(_Msg.m_Param0 == 1)
		{
			if(m_LastSave != "")
			{
				ConExecute("deferredscriptgrabscreen(\"loadgame (\\\"" + m_LastSave +  "\\\")\")");
				m_bWaitingForChangeWorldScript = true;
			}
			else
				ConExecute("changemap (\"" + m_StartLevel + "\", 4)"); // Keep resource
			return 1;
		}
		return CWObject_GameCore::OnMessage(_Msg);

	case OBJMSG_RAIL_FORGETENEMIES:
		{
			if (m_bRailForgetEnemies)
				return 1;
			else
				return 0;
		}

	case OBJMSG_GAME_MODIFYSPAWNFLAGS:
		{
			bool bSet = (_Msg.m_pData == NULL || CStrBase::CompareNoCase((const char*)_Msg.m_pData, "Set") == 0);

			// set flags instantly
			if (bSet)
				m_pWServer->m_CurSpawnMask |= _Msg.m_Param0;
			else
				m_pWServer->m_CurSpawnMask &= ~_Msg.m_Param0;
		}
		return 1;
	case OBJMSG_GAME_ACHIEVMENT:
		{
			CStr delayed = (char*)_Msg.m_pData;
			if(delayed.Val_int())
				AwardAchievement(_Msg.m_Param0, true);
			else
				m_DelayedAchievments.Add(_Msg.m_Param0);
		}
		return 1;
	}

	return CWObject_GameCore::OnMessage(_Msg);
}

aint CWObject_GameP4::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAMEP4_GETNIGHTVISIONLIGHTRANGE:
		return GetClientData(_pObj)->GetCurNVRange(_pWClient->GetGameTick(),  _pWClient->GetRenderTickFrac());
	case OBJMSG_GAME_RENDERSTATUSBAR:
		OnClientRenderStatusBar(_pObj, _pWClient, (CXR_Engine *)_Msg.m_Param0, (CRC_Util2D *)_Msg.m_Param1, (CVec2Dfp32 *)_Msg.m_pData);
		return 1;
	}

	return CWObject_GameCore::OnClientMessage(_pObj, _pWClient, _Msg);
}

void CWObject_GameP4::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	switch(_Msg.m_MsgType)
	{
	case NETMSG_GAME_DESTROYVOICES:
		{
			TSelection<CSelection::LARGE_BUFFER> Selection;
			_pWClient->Selection_AddAll(Selection);
			const int16 *pSel;
			int nSel = _pWClient->Selection_Get(Selection, &pSel);
			CWObject_Message Msg(OBJMSG_GAMEP4_DESTROYVOICES);
			for(int i = 0; i < nSel; i++)
				_pWClient->ClientMessage_SendToObject(Msg, pSel[i]);
		}
	}
}

void CWObject_GameP4::OnClientRenderStatusBar(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D, CVec2Dfp32 *_rect)
{
//	bool bAutoFlush = _pUtil2D->GetAutoFlush();
//	_pUtil2D->SetAutoFlush( false );	
	CWObject_GameCore::OnClientRenderStatusBar(_pObj, _pWClient, _pEngine, _pUtil2D, _rect);

	CClientData *pCD = GetClientData(_pObj);
	int Tick = _pWClient->GetGameTick();

	CRC_Viewport VP;
	_pWClient->Render_GetLastRenderViewport(VP);
	CClipRect Clip(VP.GetViewClip());
	bool bRenderExtraButton = true;
	if(pCD->m_GameMessage_Text.m_Value != "")
	{
		int VHeight = 480;
		int VWidth = TruncToInt(640.0f * VP.GetFOVAspect());

		// Game message
		if(pCD->m_GameMessage_Text.m_Value[0] == '$')
		{
			int Alpha = 0xff000000 - (GetClientData(_pObj)->GetCurScreenFade(_pWClient->GetGameTick(), _pWClient->GetRenderTickFrac()) & 0xff000000);

			CStr Reg = pCD->m_GameMessage_Text.m_Value.Copy(1, 1024);
			int iReg = _pWClient->GetMapData()->GetResourceIndex_Registry(Reg);
			CRegistry *pReg = _pWClient->GetMapData()->GetResource_Registry(iReg);
			if(pReg && pReg->GetNumChildren() == 1)
			{
				pReg = pReg->GetChild(0);

				CFStr St = pReg->GetThisValue();
				fp32 Speed = St.GetStrSep(",").Val_fp64() * _pWClient->GetGameTickTime();
				int LineSize = St.GetStrSep(",").Val_int();
				fp32 TimeBeforeFade = St.GetStrSep(",").Val_fp64() * _pWClient->GetGameTicksPerSecond();
				fp32 FadeDuration = St.GetStrSep(",").Val_fp64() * _pWClient->GetGameTicksPerSecond();
				fp32 Duration = Tick + _pWClient->GetRenderTickFrac() - pCD->m_GameMessage_StartTick;

				_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
				if(Duration > TimeBeforeFade)
				{
					int Alpha = 255;
					if(Duration - TimeBeforeFade < FadeDuration)
						Alpha = int(255 * (Duration - TimeBeforeFade) / FadeDuration);
					_pUtil2D->SetTexture(0);
					_pUtil2D->Rect(Clip, Clip.clip, Alpha << 24);
				}

				fp32 ScrolledPixels = Duration * Speed;
				fp32 LowestLine = ScrolledPixels / LineSize;
				int iLine = TruncToInt(LowestLine);
				fp32 Frac = LowestLine - iLine;

				int Pos = TruncToInt(VHeight - Frac * LineSize);
				while(Pos > -LineSize && iLine >= 0)
				{
					if(iLine < pReg->GetNumChildren())
					{
						int32 TextCol = 0x00808080 | Alpha;
						int32 ShadowCol = 0x00000000 | Alpha;
						CFStr Name = pReg->GetName(iLine);
						if(Name != "$SPACE")
						{
							CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font(Name.GetStrSep(",")));
							CStr St = CStrF("§Z%i%s", Name.GetStrSep(",").Val_int(), pReg->GetValue(iLine).Str());
							if(pFont)
								_pUtil2D->Text_DrawFormatted(Clip, pFont, St, Name.GetStrSep(",").Val_int(), Pos, WSTYLE_TEXT_SHADOW, TextCol, ShadowCol, 0, 640, LineSize, false);
						}
					}

					Pos -= LineSize;
					iLine--;
				}
			}
		}
		else
		{
			const fp32 EnterTicks = 0.3f * _pWClient->GetGameTicksPerSecond();
			int MsgDuration = pCD->m_GameMessage_Duration;
			if(MsgDuration == 0)
				MsgDuration = int(EnterTicks * 2 + 5);
			int StopTick = pCD->m_GameMessage_StartTick + MsgDuration;
			if(StopTick != 0 && StopTick > Tick)
			{
				CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
				CRC_Font *pTitleFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("HEADINGS"));
				if(pFont && pTitleFont)
				{
					fp32 Duration = Tick + _pWClient->GetRenderTickFrac() - pCD->m_GameMessage_StartTick;

					CFStr Text = pCD->m_GameMessage_Text;
					CFStr Title;
					if(Text.Find("|") != -1)
						Title = Text.GetStrSep("|");
					else
						Title = "§LGAMEMSG_INFO";

					int iSurfRes = 0;
					int SurfW = 0;
					int SurfH = 0;
					if(Text.CompareSubStr("$") == 0)
					{
						iSurfRes = Text.GetStrSep(",").Copy(1, 1024).Val_int();
						SurfW = Text.GetStrSep(",").Val_int();
						SurfH = Text.GetStrSep(",").Val_int();
					}
					Text = "§Z18" + Text;

					wchar Buffer[1024];
					Localize_Str(Text.Str(), Buffer, 1024);

					if(Buffer[4] == '#')
					{
						CStr St = Buffer + 5;
						while(St != "")
						{
							int X = St.GetStrSep(",").Val_int();
							int Y = St.GetStrSep(",").Val_int();
							//pFont->GetOriginalSize()

							int Alpha;
							if(Duration < EnterTicks)
								Alpha = int(255 * Duration / EnterTicks);
							else if(Duration < MsgDuration - EnterTicks)
								Alpha = 255;
							else
								Alpha = int(255 * (MsgDuration - Duration) / EnterTicks);
							if(St[0] == '$')
							{
								St = St.Copy(1, 1024);
								iSurfRes = _pWClient->GetMapData()->GetResourceIndex_Surface(St.GetStrSep(",").Ansi());
								int iSurf = _pWClient->GetMapData()->GetResource_SurfaceID(iSurfRes);
								_pUtil2D->SetSurface(iSurf, _pWClient->GetRenderTime());
								int W = St.GetStrSep(",").Val_int();
								int H = St.GetStrSep(",").Val_int();

//								CXW_Surface *pSurf = _pWClient->GetMapData()->GetResource_Surface(iSurfRes);
/*								if(pSurf && pSurf->GetBaseFrame() && pSurf->GetBaseFrame()->m_lTextures.Len() > 0)
								{
									int TextureID = pSurf->GetBaseFrame()->m_lTextures[0].m_TextureID;
									MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
									if(pTC && TextureID > 0)
									{
										pTC->MakeDirty(TextureID);
										CTextureContainer_Video* pTCVideo = TDynamicCast<CTextureContainer_Video>(pTC->GetTextureContainer(TextureID));
										if(pTCVideo)
										{
											int iLocal = pTC->GetLocal(TextureID);
											static bFirst = true;
											if(bFirst)
											{
												pTCVideo->Rewind(iLocal);
												pTCVideo->AutoRestart(iLocal);
												bFirst = false;
											}
										}
									}
								}*/
								if(W > 10000)
								{
									_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / VWidth, fp32(_pUtil2D->GetTextureHeight()) / VHeight);
									W -= 10000;
								}
								else
								{
									_pUtil2D->SetTextureOrigo(Clip, CPnt(X, Y));
									_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / W, fp32(_pUtil2D->GetTextureHeight()) / H);
								}
								_pUtil2D->Rect(Clip, CRct(X, Y, X + W, Y + H), 0x00ffffff | (Alpha << 24));
							}
							else
							{
								CRC_Font *pFont2 = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("PALATINO"));
								if(pFont2)
									pFont = pFont2;
								CStr St2 = "§Z24" + St;
								int32 TextCol = 0x00808080 | (Alpha << 24);
								int32 ShadowCol = 0x00000000 | (Alpha << 24);
								_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
								_pUtil2D->Text_DrawFormatted(Clip, pFont, St2, X, Y, WSTYLE_TEXT_WORDWRAP | WSTYLE_TEXT_SHADOW, TextCol, ShadowCol, 0, (int)(VWidth - X), (int)(VHeight - Y), false);
							}

		/*					if(iSurfRes != 0)
							{
								int iSurf = _pWClient->GetMapData()->GetResource_SurfaceID(iSurfRes);
								_pUtil2D->SetSurface(iSurf, CMTime::CreateFromSeconds(0));
								_pUtil2D->SetTextureOrigo(Clip, CPnt(XPos, _YPos));
								_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / SurfW, fp32(_pUtil2D->GetTextureHeight()) / SurfH);
								_pUtil2D->Rect(Clip, CRct(XPos, _YPos, XPos + SurfW, _YPos + SurfH), 0xff808080);
								Width += SurfW;
							}*/
						}
					}
					else
					{
						SurfW = int((SurfW / _pUtil2D->GetRC()->GetDC()->GetPixelAspect()));

						wchar BufTitle[1024];
						Localize_Str(Title.Str(), BufTitle, 1024);
						CRct Rct = CalcInfoRectWidth(_pUtil2D, BufTitle, pTitleFont, Buffer, pFont, iSurfRes, SurfW, SurfH);

						CVec2Dfp32 coord_scale = _pUtil2D->GetCoordinateScale();
						int XPos;
						int XBasePos = (int)VWidth / 2 - Rct.GetWidth() / 2;
						//if(1.4f < ((fp32)Clip.clip.p1.x / (fp32)Clip.clip.p1.y))
							XBasePos += (int(fp32(Clip.clip.p1.x) / coord_scale.k[0]) - (int)VWidth) / 2; 

						if(Duration < EnterTicks)
							XPos = RoundToInt(VWidth - (VWidth - XBasePos) * (1.0f - Sqr(1.0f - Duration / EnterTicks)));
						else if(Duration < MsgDuration - EnterTicks)
						{
							bRenderExtraButton = false;
							XPos = XBasePos;
						}
						else
							XPos = XBasePos - RoundToInt((Rct.GetWidth() + XBasePos) * Sqr(1.0f - ((MsgDuration - Duration) / EnterTicks)));
						int YPos = RoundToInt( (150.0f / 480.0f) * VHeight );

						Rct.p0.x += XPos;
						Rct.p1.x += XPos;
						Rct.p0.y += YPos;
						Rct.p1.y += YPos;

						DrawInfoRect(_pUtil2D, _pWClient, Rct, BufTitle, pTitleFont, Buffer, pTitleFont, iSurfRes, SurfW, SurfH);
					}
				}
			}	
		}
	}

	int iPlayer = _pWClient->Player_GetLocalObject();
	CWObject_Client *pPlayerObj = _pWClient->Object_Get(iPlayer);
	if(_pObj->m_ClientFlags & GAME_CLIENTFLAGS_PENDINGGAMEMSG && pPlayerObj && CWObject_Character::Char_IsPlayerView(pPlayerObj))
	{
		CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("HEADINGS"));
		if(pFont)
		{
			wchar Buf[1024];
			Localize_Str("§z09§LGAMEMSG_HELP", Buf, 1024);
			int Width = int(pFont->GetWidth(pFont->GetOriginalSize(), Buf));
			int32 TextCol = CPixel32::From_fp32((226/1.7f),(224/1.7f),(220/1.7f),255);
			_pUtil2D->Text_DrawFormatted(Clip, pFont, Buf, 555 - Width, 87, 0, TextCol, 0, 0, Width, 40, false);
		}

		if((_pWClient->GetGameTick() & 0xf) > 7)
		{
			_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
#ifdef PLATFORM_XBOX
			_pUtil2D->DrawTexture(Clip, CPnt(560, 80), "GUI_Button_B_32");
#elif !defined(PLATFORM_CONSOLE)
			_pUtil2D->DrawTexture(Clip, CPnt(560, 80), "GUI_Button_F1_64", 0xff808080, CVec2Dfp32(2.0f, 2.0f));
#endif
		}
	}

	if(pCD->m_GameSurface_Surface != 0 && bRenderExtraButton)
	{
		fp32 HintDur = fp32(_pWClient->GetGameTick() - pCD->m_GameSurface_StartTick) + _pWClient->GetRenderTickFrac();
		bool bOn = _pWClient->GetGameTick() % 25 > 12;
		if(bOn && (pCD->m_GameSurface_Duration == 0 || HintDur < pCD->m_GameSurface_Duration * _pWClient->GetGameTicksPerSecond()))
		{
			CXW_Surface* pSurface = _pWClient->GetMapData()->GetResource_Surface(pCD->m_GameSurface_Surface);
			if (pSurface)
			{
				//
				// Get "BINDINGID" key value from surface, which contains the name of an action identifier, and see if there is a 
				// key binding that matches this action. If so, look up the matching GUI icon for this scancode and render that texture.
				//
				// NOTE: this was earlier done by adding a list of textures to each surface, where the list of textures would b with texture animation and setting the time to the scan code value
				//       (and then have the GUI textures on the correct "time" in the surface), but that has now been replaced by a 
				//       GetBindingGUITexture() which is easier to maintain and can be adjusted for platform differences.
				//

				//fp32 Time = 0;
				int Binding_ScanCode = 0;
				CStr Action;
			#ifdef XW_SURF_THIN
				int iBindingSt = pSurface->m_Keys.GetKeyIndex("BINDINGID");
				if(iBindingSt != -1)
				{
					Action = pSurface->m_Keys.GetKeyValue(iBindingSt);
			#else
				int iBindingSt = pSurface->m_spKeys->GetKeyIndex("BINDINGID");
				if(iBindingSt != -1)
				{
					Action = pSurface->m_spKeys->GetKeyValue(iBindingSt);
			#endif
					MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
					CRegistry *pOptions = pSys ? pSys->GetOptions() : 0;
					if (pOptions)
						Binding_ScanCode = pOptions->GetValue(Action).Val_int();
				}
				const char* pGUITextureName = GetBindingGUITexture(Binding_ScanCode);

				if (iBindingSt != -1 && !pGUITextureName)
				{
					CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("HEADINGS"));
					if(pFont)
					{
						CStr St("§Z14§LBINDING_UNDEF§p0§LMENU_BINDING_" + Action.Copy(7, 1024) + "§pq");
						CPnt TopRight(RoundToInt(Clip.clip.p1.x * 0.70f), RoundToInt(Clip.clip.p1.y * 0.08f));
						_pUtil2D->Text_DrawFormatted(Clip, pFont, St, TopRight.x, TopRight.y, WSTYLE_TEXT_WORDWRAP, 0xffffffff, 0, 0, 100, 1000, false);
					}
				}
				else
				{
					_pUtil2D->SetCoordinateScale(CVec2Dfp32(1.0f, 1.0f));
					//_pUtil2D->SetSurface(pSurface, CMTime::CreateFromSeconds(Time));
					_pUtil2D->SetTexture(pGUITextureName);
					fp32 PixelAspect = _pUtil2D->GetRC()->GetDC()->GetPixelAspect();
					CPnt Size = CPnt(RoundToInt(_pUtil2D->GetTextureWidth() * PixelAspect), _pUtil2D->GetTextureHeight());

#ifdef PLATFORM_CONSOLE
					CPnt TopRight(RoundToInt(Clip.clip.p1.x - (Clip.clip.p1.x * 0.075f) - Size.x), RoundToInt(Clip.clip.p1.y * 0.075f));
#else
					CPnt TopRight(RoundToInt(Clip.clip.p1.x * 0.95f - Size.x), RoundToInt(Clip.clip.p1.y * 0.08f));
#endif
					_pUtil2D->SetTextureScale(1.0f * PixelAspect, 1.0f);
					_pUtil2D->SetTextureOrigo(Clip, TopRight);
					_pUtil2D->Rect(Clip, CRct(TopRight.x, TopRight.y, TopRight.x + Size.x, TopRight.y + Size.y), 0xffffffff);
				}
			}
		}
	}

	if(_rect)
		_pUtil2D->SetCoordinateScale(*_rect);

//	_pUtil2D->Flush();
//	_pUtil2D->SetAutoFlush( bAutoFlush );
}

int CWObject_GameP4::SetMission(int _MissionID, const char *_pSt)
{
	CClientData *pCD = GetClientData();
	int i;
	for(i = 0; i < pCD->m_lMissionID.Len(); i++)
		if(pCD->m_lMissionID[i] == _MissionID)
		{
			if(pCD->m_lMissionDesc[i] == _pSt)
				return 0;

			pCD->m_lMissionDesc[i] = _pSt;
			pCD->m_lMissionDesc.MakeDirty();
			pCD->m_lMissionFlags[i] = 0;
			pCD->m_lMissionFlags.MakeDirty();

			CFStr St = "§LMISSION_MISSIONUPDATED§pq|";
			St += _pSt;
#ifndef M_DEMO
			if(!m_bNoHintIcon)
			{
				ShowGameSurface(m_pWServer->GetMapData()->GetResourceIndex_Surface("GUI_ICON_JOURNAL"), 4);
				m_pWServer->Sound_Global(m_pWServer->GetMapData()->GetResourceIndex_Sound("gui_info"));
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETNEXTGUISCREEN, 0), Player_GetObjectIndex(0));
			}
#endif

			//ShowGameMsg(St, 4);
			return 1;
		}

	if(pCD->m_lMissionID.Len() == MAXMISSIONS - 1)
		Error("CWObject_GameP4", "Too many simultaions running missions");

	if(i == pCD->m_lMissionID.Len())
	{
		pCD->m_lMissionID.Add(_MissionID);
		pCD->m_lMissionID.MakeDirty();
		pCD->m_lMissionDesc.Add(_pSt);
		pCD->m_lMissionDesc.MakeDirty();
		pCD->m_lMissionFlags.Add(0);
		pCD->m_lMissionFlags.MakeDirty();

		CFStr St = "§LMISSION_NEWMISSION§pq|";
#ifndef M_DEMO
		CFStr St2 = _pSt;
		if(St2.Find("_1") == St2.Len() - 2)
			St += St2.Copy(0, St2.Len() - 2);
		else
#endif
			St += _pSt;
#ifndef M_DEMO
		St += "§pq||§LMISSION_NEWMISSION2";
		if(m_bNoHintIcon)
			St += " §LMISSION_NEWMISSION3";
#endif
		ShowGameMsg(St, 4);
		if (Player_GetNum())
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETNEXTGUISCREEN, 0), Player_GetObjectIndex(0));
	}

	return 1;
}

int CWObject_GameP4::RemoveMission(int _MissionID, bool _bCompleted)
{
	CClientData *pCD = GetClientData();
	for(int i = 0; i < pCD->m_lMissionID.Len(); i++)
		if(pCD->m_lMissionID[i] == _MissionID)
		{
			pCD->m_lMissionID.Del(i);
			pCD->m_lMissionID.MakeDirty();
			pCD->m_lMissionDesc.Del(i);
			pCD->m_lMissionID.MakeDirty();
			pCD->m_lMissionFlags.Del(i);
			pCD->m_lMissionID.MakeDirty();
/*			pCD->m_lMissionFlags[i] |= 1;
			pCD->m_lMissionFlags.MakeDirty();*/
			if(_bCompleted)
				ShowGameMsg("§LMISSION_MISSIONCOMPLETED", 4);
			/*else
				ShowGameMsg("§LMISSION_MISSIONFAILED", 4);*/
			return 1;
		}

	return 0;
}

void CWObject_GameP4::Player_OnSpawn(int _iPlayer)
{
	CWO_Player *pPlayer = Player_Get(_iPlayer);
	if(pPlayer)
	{
		CWS_ClientObjInfo* pObjInfo = m_pWServer->Object_GetPerClientData(pPlayer->m_iObject, pPlayer->m_iClient);
		if(pObjInfo)
			pObjInfo->m_ClientFlags |= CWO_CLIENTFLAGS_HIGHPRECISION | CWO_CLIENTFLAGS_MOVEVELOCITY;
		
//		OnSetClientWindow(pPlayer->m_iClient, "");
	}

	m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAMEP4_GETNIGHTVISIONLIGHTRANGE), pPlayer->m_iObject);
}

int CWObject_GameP4::OnClientConnected(int _iClient)
{
	return CWObject_GameCore::OnClientConnected(_iClient);
}

void CWObject_GameP4::OnDeltaLoad(CCFile* _pFile, int _Flags)
{
	uint16 NumAchievments;
	_pFile->ReadLE(NumAchievments);

	m_DelayedAchievments.Clear();
	for(int32 i = 0; i < NumAchievments; i++)
	{
		uint8 AchievmentID;
		_pFile->ReadLE(AchievmentID);
		m_DelayedAchievments.Add(AchievmentID);
	}
	m_AIResources.OnDeltaLoad(_pFile);
	m_DeathSeqPast.Read(_pFile);
	m_DeathSeqPresent.Read(_pFile);
	m_DeathSeqFuture.Read(_pFile);
	m_DeathSeqSound.Read(_pFile);
	CWObject_GameCore::OnDeltaLoad(_pFile, _Flags);
}

void CWObject_GameP4::OnDeltaSave(CCFile* _pFile)
{
	uint16 NumAchievments = m_DelayedAchievments.Len();
	_pFile->WriteLE(NumAchievments);
	for(int32 i = 0; i < NumAchievments; i++)
		_pFile->WriteLE(m_DelayedAchievments[i]);
	m_AIResources.OnDeltaSave(_pFile);
	m_DeathSeqPast.Write(_pFile);
	m_DeathSeqPresent.Write(_pFile);
	m_DeathSeqFuture.Write(_pFile);
	m_DeathSeqSound.Write(_pFile);
	CWObject_GameCore::OnDeltaSave(_pFile);
}

int CWObject_GameP4::OnInitWorld()
{
	//Spawn default teams
	if (m_pWServer)
	{
		for (int i = 0; i < m_lDefaultTeams.Len(); i++)
		{
			m_pWServer->Object_Create(m_lDefaultTeams[i]);
		}

//		m_lDefaultTeams.Clear();
	}

	//Make sure no ai is paused
	m_AIResources.UnpauseAll();

	// Clear some AI static allocations
	m_AIResources.CleanStatic();

	//Reset levelkey stuff
	m_MinimumClearanceLevel = 0;
	m_bRailForgetEnemies = false;

	m_LastLoadTick = ~0;

	//Should other levelkey variables be reset as well?
	//m_Gamestyle = GAMESTYLE_SNEAK;
	//m_DefaultGamestyle = m_Gamestyle;
	//...

	return CWObject_GameCore::OnInitWorld();
}


int CWObject_GameP4::OnCloseWorld()
{
	AwardDelayedAchievements();

	// Clear some AI static allocations
	m_AIResources.CleanStatic();
	m_AutoSave = "";

	return CWObject_GameCore::OnCloseWorld();
}

// This function uses alot of stack so it is placed outside in its own scope
void M_NOINLINE CWObject_GameP4::OnSpawnRailHandler()
{
	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pWServer->Selection_AddClass(Selection, "RailHandler");
	
	// get selection
	const int16 *pSel;
	int32 nSel = m_pWServer->Selection_Get(Selection, &pSel);

	if(nSel == 0)
	{
		// Do a selection on the "usable" type which should all be rails
		TSelection<CSelection::LARGE_BUFFER> Selection;
		m_pWServer->Selection_AddClass(Selection, "Rail");

		// get selection
		const int16 *pSel;
		int32 nSel = m_pWServer->Selection_Get(Selection, &pSel);

		if (nSel > 0)
		{
			CMat4Dfp32 Unit;
			Unit.Unit();
			m_pWServer->Object_Create("RailHandler", Unit);
		}
	}

}


void CWObject_GameP4::OnSpawnWorld()
{
	// Clear the global list of dead
	m_AIResources.m_KBGlobals.ms_lDead.Clear();

	OnSpawnRailHandler();

	m_pWServer->GetMapData()->GetResourceIndex_Sound("gui_info");
	m_LastSaveTick = ~0;

	{
		CStr PhoneBook = "phonebook_base";
		CRegistry *pPhone = m_pWServer->Registry_GetLevelKeys("PHONEBOOK");
		if(pPhone)
			PhoneBook = pPhone->GetThisValue();

		// Create object...
		CMat4Dfp32 Pos;
		Pos.CreateTranslation(CVec4Dfp32(0,0,-1000.0f,1.0f));
		// Create phonebook, all address entries will be in here

		m_pWServer->Object_Create(PhoneBook, Pos);
	}

	CWObject_GameCore::OnSpawnWorld();
}

int CWObject_GameP4::OnClientCommand(int _iPlayer, const CCmd* _pCmd)
{
	if (!_pCmd) return 0;

	int Size = _pCmd->m_Size;
	switch(_pCmd->m_Cmd)
	{
	case CONTROL_GUI_NEXT:
		{
			CWO_Player *pPlayer = Player_Get(_iPlayer);
			if(pPlayer && pPlayer->m_iClient != -1)
			{
				CRegistry* pReg = m_pWServer->Registry_GetClient(pPlayer->m_iClient, WCLIENT_REG_GAMESTATE);
				if(pReg)
				{
					CStr Key = 	pReg->GetValue("WINDOW");
					if(Key == "infoscreen")
					{
						//M_TRACEALWAYS(CStrF("%i. GUI_NEXT, Clear window\n", m_pWServer->GetGameTick()));
						OnSetClientWindow(pPlayer->m_iClient, "");
					}
					else
					{
						//M_TRACEALWAYS(CStrF("%i. GUI_NEXT, Don't clear window\n", m_pWServer->GetGameTick()));
					}
				}
			}
			return 1;
		}
	case CONTROL_MENUTRIGGER:
		{
			m_pWServer->Message_SendToTarget(CWObject_Message(OBJMSG_IMPULSE, 1), CFStrF("menutrigger%i", _pCmd->m_Data[0]));
			return 1;
		}

	case CONTROL_UPDATE_CD_AUTOAIMVAL:
	{
		if(_iPlayer != -1)
		{
			CWO_Player *pPlayer = Player_Get(_iPlayer);
			CWObject_CoreData *pTarget = pPlayer ? m_pWServer->Object_GetCD(pPlayer->m_iObject) : NULL;
			bool bChar = (pTarget->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER) != 0;
			if(bChar)
			{
				char cmd[256];
				memcpy(cmd, _pCmd->m_Data, Size);
				cmd[Size] = 0;
				CStr command = cmd;
				CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(pTarget);
				pCD->m_AutoAimVal = command.Val_fp64();
			}
		 }
		
	}

	case CONTROL_CMD:
		{
			int Pos = 0;
			int Cmd = (Size > 0) ? _pCmd->m_Data[Pos++] : -1;
			switch(Cmd)
			{					
			case CMD_SHOWCONTROLLER:
				ShowInfoScreen("GUI_LOAD_01", 0);
				return 1;
			case CMD_LOADLASTSAVE:
				{
					m_PendingPause = -1;
					m_PendingWorldTick = -1;
					if(m_LastSave != "")
					{
						ConExecute("deferredscriptgrabscreen(\"loadgame (\\\"Checkpoint\\\")\")");
						m_bWaitingForChangeWorldScript = true;
					}
					else
						ConExecute("changemap(\"" + m_StartLevel + "\", 4)"); // Keep resource

					int nClient = m_pWServer->Client_GetCount();
					for(int i = 0; i < nClient; i++)
						OnSetClientWindow(i, "");
				}
			}
			return CWObject_GameCore::OnClientCommand(_iPlayer, _pCmd);
		}
	}

	return CWObject_GameCore::OnClientCommand(_iPlayer, _pCmd);
}

void CWObject_GameP4::AwardDelayedAchievements(void)
{
	for(int i = 0; i < m_DelayedAchievments.Len(); i++)
		AwardAchievement(m_DelayedAchievments[i], false);
	m_DelayedAchievments.Clear();
	ConExecute("saveprofile()");
}

void CWObject_GameP4::AwardAchievement(int _ID, bool _bSaveProfile)
{
#ifdef PLATFORM_CONSOLE
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
	if(pGameMod)
	{
		pGameMod->m_pMPHandler->AchievementComplete(_ID);
	}
#endif	
}

int CWObject_GameP4::OnChangeWorld(const char *_pWorld, int _Flags, int _iSender)
{
	if(m_PendingWorld != "")
		return 0;

	AwardDelayedAchievements();

	return CWObject_GameCore::OnChangeWorld(_pWorld, _Flags, _iSender);
}

CStr CWObject_GameP4::GetDefualtSpawnClass()
{
	CRegistry *pTpl = m_pWServer->Registry_GetTemplates();
	CFStr Player = SPAWNCLASS + CFStr("_") + m_pWServer->m_WorldName.Str();
	if(pTpl && pTpl->FindIndex(Player) != -1)
		return Player;

	Player = SPAWNCLASS + CFStr("_base");
	if(pTpl && pTpl->FindIndex(Player) != -1)
		return Player;

	return "spectator";	// SPAWNCLASS makes no sense since CWObject_Player cannot host a player.
}
/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_GameCampaign
|__________________________________________________________________________________________________
\*************************************************************************************************/
int CWObject_GameCampaign::OnInitWorld()
{
	CRPG_Object::m_bPrecacheForPlayerUse = true;
	m_pWServer->GetMapData()->GetResourceIndex_Class(GetDefualtSpawnClass());
	CRPG_Object::m_bPrecacheForPlayerUse = false;

	return CWObject_GameP4::OnInitWorld();
}

int CWObject_GameCampaign::OnClientConnected(int _iClient)
{
	CWObject_GameP4::OnClientConnected(_iClient);

	CMat4Dfp32 Mat;
	int16 Flags = 0;
	CWServer_Mod *pServer = safe_cast<CWServer_Mod >(m_pWServer);

	// Jakob
	int16 iPlayerStartObj = 0;
	if (!pServer || !pServer->m_GameState.GetNextPendingPlayerPos(Mat))
	{
		Mat = GetSpawnPosition(0, m_LastWorldChangeID, &Flags, &iPlayerStartObj);
	}
	
	int PlayerObj = -1;
	int PlayerGUID = -1;
	CStr Class;
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys->GetEnvironment()->GetValuei("FORCESPECTATOR"))
		Class = "spectator";
	else
	{
		Class = pServer ? pServer->m_GameState.GetNextPendingPlayerClass(&PlayerObj, &PlayerGUID, &Flags) : CStr("");
		if(Class != "")
			Flags |= INFOPLAYERSTART_FLAGS_OGR_SPECIALCLASS;
		else
			Class = GetDefualtSpawnClass();
	}

	if (_iClient == 0 && PlayerObj == -1)
	{
		PlayerObj = 2559;
	}

	if (pServer->m_GameState.GetHasInsertedPlayerGUID())
	{
		// Remove cheated GUID
		pServer->m_spGUIDHash->Remove(PlayerObj);
		pServer->m_GameState.ClearInsertedPlayerGUID();
	}

	if(pServer && Flags & (INFOPLAYERSTART_FLAGS_CLEARSAVE | INFOPLAYERSTART_FLAGS_CLEARPLAYER))
		pServer->m_GameState.ClearSaves();

	if(Flags & INFOPLAYERSTART_FLAGS_CLEARMISSIONS)
	{
		GetClientData()->m_lMissionID.Clear();
		GetClientData()->m_lMissionDesc.Clear();
		GetClientData()->m_lMissionFlags.Clear();
	}

	int iObj = Player_Respawn(Player_GetWithClient(_iClient)->m_iPlayer, Mat, Class, Flags, PlayerObj);
	if(iObj == -1)
	{
		ConOutL(CStrF("ERROR: Failed to spawn character %s", Class.Str()));
	}
	else if(pServer)
	{
		if(PlayerGUID != -1)
			pServer->Object_ChangeGUID(iObj, PlayerGUID);
		pServer->m_GameState.ApplyNextPendingPlayer(pServer, iObj, Flags);
	}

#ifndef M_DEMO
	CWServer_Mod *pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
	if(pServerMod->m_GameState.IsFirstLevelVisit(pServerMod))
	{
		if(m_AutoSave != "")
		{
			if(!m_pWServer->World_SaveExist(m_AutoSave))
				m_pWServer->World_Save(m_AutoSave);
			else
				m_pWServer->World_Save("Checkpoint");
			m_LastSave = "Checkpoint";
			m_LastSaveTick = m_pWServer->GetGameTick();
		}
	}
#endif

	m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETNEXTGUISCREEN, 2), iObj);

	OnPlayerEntersGame(iObj, iPlayerStartObj);
	return 1;
}


int CWObject_GameCampaign::OnCharacterKilled(int _iObject, int _iSender)
{
	if(Player_GetNum() > 0)
	{
		CWO_Player *pPlayer = Player_Get(0);
		if(pPlayer && pPlayer->m_iObject == _iObject)
		{
			CWObject_Character *pObj = (CWObject_Character *)m_pWServer->Object_Get(_iObject);
			if(pObj->m_PendingCutsceneTick == -1)
				FadeScreen(0xff000000, 1.5f, 0);
			pPlayer->m_Mode = PLAYERMODE_DEAD;
		}
	}

	return 1;
}

void CWObject_GameCampaign::OnRefresh()
{
	CWObject_GameP4::OnRefresh();
}

#ifdef M_Profile

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_GameDebug
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_GameDebug::CWObject_GameDebug()
{
}

void CWObject_GameDebug::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	CWObject_GameP4::OnEvalKey(_KeyHash, _pKey);
}

void CWObject_GameDebug::OnRefresh()
{
	CWObject_GameP4::OnRefresh();
}

CWObject_GameDebug::CWO_PlayerDebug *CWObject_GameDebug::Player_GetDebug(int _iPlayer)
{
	return (CWO_PlayerDebug *)(CWO_Player *)m_lspPlayers[_iPlayer];
}

int CWObject_GameDebug::OnClientConnected(int _iClient)
{
	CWObject_GameP4::OnClientConnected(_iClient);

	CWServer_Mod *pServer = safe_cast<CWServer_Mod >(m_pWServer);
	int16 Flags = 0;
	int iObj = 0;
	if(m_LastWorldChangeID == "")
	{
		//if(pServer)
		//	pServer->m_GameState.ClearSaves();

		CMat4Dfp32 Mat;
		CStr Class = m_pWServer->Registry_GetGame()->GetValue("QUICK_CHARACTER");
		if(Class == "")
		{
			if(pServer->m_GameState.GetNextPendingPlayerPos(Mat))
			{
				CWO_PlayerDebug *pPlayer = safe_cast<CWO_PlayerDebug >(Player_GetWithClient(_iClient));
				if(pPlayer && pPlayer->m_Class == "auto")
					Class = GetDefualtSpawnClass();
				else if(pPlayer && pPlayer->m_Class != "")
					Class = pPlayer->m_Class;
				else
				{
					Class = pServer->m_GameState.GetNextPendingPlayerClass();
					if(Class != "")
						Flags |= INFOPLAYERSTART_FLAGS_OGR_SPECIALCLASS;
					else
						Class = GetDefualtSpawnClass();
				}
			}
			else
			{
				OnSetClientWindow(_iClient, "selectclass");
				Class = "spectator";
//				Class = GetDefualtSpawnClass();
				Mat = GetSpawnPosition(0, NULL, &Flags);
			}
		}
		else
		{
			Mat = GetSpawnPosition(0, NULL, &Flags);
			if(Class.CompareNoCase("Manual") == 0)
			{
				OnSetClientWindow(_iClient, "selectclass");
				Class = "spectator";
			}
			else if(Class.CompareNoCase("Auto") == 0)
			{
				Class = GetDefualtSpawnClass();
			}
			else if(Class.CompareNoCase("NoClip") == 0)
			{
				Class = GetDefualtSpawnClass();
				Flags |= INFOPLAYERSTART_FLAGS_OGR_NOCLIP;
			}
		}

		CStr StartPos = m_pWServer->Registry_GetGame()->GetValue("QUICK_STARTPOS");
		if(StartPos != "")
		{
			Mat.ParseString(StartPos);
			Mat.k[3][2] -= 56; // Compensate for "head pos"...
		}

		iObj = Player_Respawn(Player_GetWithClient(_iClient)->m_iPlayer, Mat, Class, Flags);
	}
	else
	{
		CWO_PlayerDebug *pPlayer = (CWO_PlayerDebug *)Player_GetWithClient(_iClient);
		if(pPlayer)
		{
			CMat4Dfp32 Mat = GetSpawnPosition(0, m_LastWorldChangeID, &Flags);
			CStr Class = pPlayer ? pPlayer->m_Class : CStr("");
			if(Class == "")
				Class = pServer ? pServer->m_GameState.GetNextPendingPlayerClass() : CStr("");
			if(Class == "auto" || Class == "")
				Class = GetDefualtSpawnClass();

			if(pServer && Flags & (INFOPLAYERSTART_FLAGS_CLEARSAVE | INFOPLAYERSTART_FLAGS_CLEARPLAYER))
				pServer->m_GameState.ClearSaves();

			if(Flags & INFOPLAYERSTART_FLAGS_CLEARMISSIONS)
			{
				GetClientData()->m_lMissionID.Clear();
				GetClientData()->m_lMissionDesc.Clear();
				GetClientData()->m_lMissionFlags.Clear();
			}

			iObj = Player_Respawn(pPlayer->m_iPlayer, Mat, Class, 0);
		}
	}

	if(pServer && iObj > 0)
	{
		if(pServer)
			pServer->m_GameState.ApplyNextPendingPlayer(pServer, iObj, Flags);
	}

	OnPlayerEntersGame(iObj, 0);

	m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETNEXTGUISCREEN, 2), iObj);

	return 1;
}

int CWObject_GameDebug::OnInitWorld()
{
//	IncludeAllPlayers();

	// This will be requested on the client by select-char window
	m_pWServer->GetMapData()->GetResourceIndex_Registry("Registry\\Sv");

	CRPG_Object::m_bPrecacheForPlayerUse = true;
	m_pWServer->GetMapData()->GetResourceIndex_Class(GetDefualtSpawnClass());
	CRPG_Object::m_bPrecacheForPlayerUse = false;

	for(int i = 0; i < m_lspPlayers.Len(); i++)
	{
		CWO_PlayerDebug *pPlayer = Player_GetDebug(i);
		if(pPlayer)
			pPlayer->m_Mode = 0;
	}

	m_bNoHintIcon = false;

	return CWObject_GameP4::OnInitWorld();
}

aint CWObject_GameDebug::OnMessage(const CWObject_Message& _Msg)
{
	return CWObject_GameP4::OnMessage(_Msg);
}

int CWObject_GameDebug::OnClientCommand(int _iPlayer, const CCmd* _pCmd)
{
	int Size = _pCmd->m_Size;
	switch(_pCmd->m_Cmd)
	{
	case CONTROL_JOINGAME:
		{
			char Class[256];
			memcpy(Class, _pCmd->m_Data, Size);
			Class[Size] = 0;
			if(GetDefualtSpawnClass().CompareNoCase(Class) == 0)
				Player_GetDebug(_iPlayer)->m_Class = "auto";
			else
				Player_GetDebug(_iPlayer)->m_Class = Class;

			int State = m_pWServer->GetMapData()->GetState();
			m_pWServer->GetMapData()->SetState(State & ~WMAPDATA_STATE_NOCREATE);
			int16 Flags;
			CMat4Dfp32 Mat = GetSpawnPosition(0, NULL, &Flags);
			Player_Respawn(_iPlayer, Mat, Class, Flags);

			m_pWServer->GetMapData()->SetState(State);

			CWO_Player *pPlayer = Player_Get(_iPlayer);
			if(pPlayer)
				OnSetClientWindow(pPlayer->m_iClient, "");
			return true;
		}
	}
	return CWObject_GameP4::OnClientCommand(_iPlayer, _pCmd);
}

spCWO_Player CWObject_GameDebug::CreatePlayerObject()
{
	CWO_PlayerDebug *pP = MNew(CWO_PlayerDebug);
	spCWO_Player spP = (CWO_Player *)pP;
	if (!spP) MemError("CreateChar");

	return spP;
}

int CWObject_GameDebug::OnCharacterKilled(int _iObject, int _iSender)
{
	if(Player_GetNum() > 0)
	{
		CWO_Player *pPlayer = Player_Get(0);
		if(pPlayer && pPlayer->m_iObject == _iObject)
		{
			CWObject_Character *pObj = (CWObject_Character *)m_pWServer->Object_Get(_iObject);
			if(pObj->m_PendingCutsceneTick == -1)
				FadeScreen(0xff000000, 6, 4);
			pPlayer->m_Mode = PLAYERMODE_DEAD;
		}
	}

	return 1;
}

#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_GameDM
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_GameDM::CWObject_GameDM()
{
	M_TRACE("CWObject_GameDM::CWObject_GameDM\n");

	m_GameMode = 0;
	m_bDoOnce = true;

	CClientData *pCD = GetClientData();
	pCD->m_WarmUpMode = MP_WARMUP_MODE_PREGAME;
	pCD->m_WarmUpCountDownTick = -1;
	m_nNumPlayersNeeded = 2;

#if defined(PLATFORM_CONSOLE)
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
	if(pGameMod)
	{
		pCD->m_MaxTime = pGameMod->m_pMPHandler->m_TimeLimit;
		pCD->m_MaxScore = pGameMod->m_pMPHandler->m_ScoreLimit;
		pCD->m_GameModeName.m_Value = CStrF("§Z20 %s",pGameMod->m_pMPHandler->GetGameModeName().Str());
	}
#else
	pCD->m_MaxScore = 20;
	pCD->m_MaxTime = 600;
	pCD->m_GameModeName.m_Value = "§Z20 §LMENU_SHAPESHIFTER - §LMENU_DM";
#endif
	pCD->m_GameModeName.MakeDirty();

	pCD->m_GameOver	= 0;
	m_GameOverTick	= 0;
}

int CWObject_GameDM::OnInitWorld()
{
	M_TRACE("CWObject_GameDM::CWObject_GameDM\n");

	ConExecute("mp_getgamemode()");

	return parent::OnInitWorld();
}

void CWObject_GameDM::OnSpawnWorld()
{
	CMat4Dfp32 Mat;
	Mat.Unit();
	Mat.k[3][2] = -100000;
	for(int i = 0; i < m_liDummyDarklings.Len(); i++)
		m_pWServer->Object_DestroyInstant(m_liDummyDarklings[i]);

	m_liDummyDarklings.Clear();

	static const char *Darklings[] = { "multiplayer_Darkling_05", "multiplayer_Darkling_06", "multiplayer_Darkling_Lightkiller", "multiplayer_Darkling_Gunner", "multiplayer_Darkling_Kamikaze", NULL };

	for(int i = 0; i < 5; i++)
	{
		int iDummy = 0;
		iDummy = m_pWServer->Object_Create(Darklings[i], Mat);
		CWObject *pObj = m_pWServer->Object_Get(iDummy);
		if(pObj)
		{
			CRegistry_Dynamic Reg;
			Reg.SetThisKey("PLAYERNR","0");
			pObj->OnEvalKey(Reg.GetThisNameHash(), &Reg);
			pObj->OnSpawnWorld();
			m_liDummyDarklings.Add(iDummy);
		}
	}

	return parent::OnSpawnWorld();
}

void CWObject_GameDM::OnSpawnWorld2()
{
	parent::OnSpawnWorld2();

	for(int i = 0; i < m_liDummyDarklings.Len(); i++)
	{
		CWObject *pObj = m_pWServer->Object_Get(m_liDummyDarklings[i]);
		pObj->OnSpawnWorld2();
		m_pWServer->Object_DestroyInstant(m_liDummyDarklings[i]);
	}
	m_liDummyDarklings.Clear();

	if(m_pWServer)
	{
		CClientData *pCD = GetClientData();
		pCD->m_StartTick = m_pWServer->GetGameTick();
		pCD->m_GameOver = 0;
	}
}

CWObject_GameDM::CWO_PlayerDM *CWObject_GameDM::Player_GetDM(int _iPlayer)
{
	return (CWO_PlayerDM *)(CWO_Player *)m_lspPlayers[_iPlayer];
}

int CWObject_GameDM::OnClientConnected(int _iClient)
{
	if (!CWObject_GameP4::OnClientConnected(_iClient))
		return 0;

	M_TRACE("CWObject_GameDM::OnClientConnected\n");

	CMat4Dfp32 Mat;
	CWServer_Mod* pServer = safe_cast<CWServer_Mod>(m_pWServer);
	M_ASSERT(pServer, "No server!");

	Mat = GetSpawnPosition();

	int PlayerObj = -1;
	int PlayerGUID = -1;
	CFStr Class;
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys->GetEnvironment()->GetValuei("FORCESPECTATOR"))
	{
		Class = "Spectator";
	}
	else
	{
		Class = "Spectator";
		
		//Open window to select model
/*		if(m_GameMode == MP_MODE_SHAPESHIFTER)
			OnSetClientWindow(_iClient, "multiplayer_selectcharacter_shapeshifters");
		else
			OnSetClientWindow(_iClient, "multiplayer_selectcharacter_darklings");*/
	}

	int16 Flags = 0;
	int iObj = Player_Respawn(Player_GetWithClient(_iClient)->m_iPlayer, Mat, Class, Flags, PlayerObj);
	if (iObj == -1)
	{
		ConOutL(CStrF("ERROR: Failed to spawn character %s", Class.Str()));
		return 0;
	}
 
	if (PlayerGUID != -1)
		pServer->Object_ChangeGUID(iObj, PlayerGUID);

	CClientData *pCD = GetClientData();

	bool bFoundEmptyPlayer = false;
	CWO_PlayerDM *pPlayerClient = Player_GetDM(_iClient);
	for(int i = 0; i < pCD->m_lPlayerNames.Len(); i++)
	{	
		CWO_PlayerDM *pPlayer = Player_GetDM(i);
		if(pPlayer)
		{
			CNetMsg Msg2;
			Msg2.AddInt8(i);
			Msg2.AddStrAny(pPlayer->m_Name);
			CNetMsg Msg(WPACKET_OBJNETMSG);
			Msg.AddInt8(NETMSG_GAME_SETNAME);
			Msg.AddInt16(m_iObject);
			Msg.AddInt16(m_iClass);
			if (Msg.AddData((void*)Msg2.m_Data, Msg2.m_MsgSize))
				m_pWServer->Net_PutMsgEx(pPlayerClient->m_iPlayer, Msg, -1, WCLIENT_STATE_INGAME);

			if(!pPlayer->m_Name.Len())
				bFoundEmptyPlayer = true;
		}
	}

	if(!bFoundEmptyPlayer)
	{
		pCD->m_lPlayerScores.Add(0);
		pCD->m_lPlayerDeaths.Add(0);
		CStr EmptyName;
		pCD->m_lPlayerNames.Add(EmptyName);
	}

	pCD->m_lPlayerScores.MakeDirty();	//We must make it dirty so that the new player will get what we have
	pPlayerClient->m_Name = " "; 
	pPlayerClient->m_bReady = true;

	while(pCD->m_lPlayerNames.Len() <= _iClient)
	{
		CStr EmptyName;
		pCD->m_lPlayerNames.Add(EmptyName);
	}

	while(pCD->m_lPlayerScores.Len() <= _iClient)
		pCD->m_lPlayerScores.Add(0);

	while(pCD->m_lPlayerDeaths.Len() <= _iClient)
		pCD->m_lPlayerDeaths.Add(0);

	pCD->m_MaxScore.MakeDirty();
	pCD->m_liMusic.MakeDirty();

	OnPlayerEntersGame(iObj, 0);

	return 1;
}

void CWObject_GameDM::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	CWObject_GameP4::OnIncludeClass(_pWData, _pWServer);

	_pWData->GetResourceIndex_Class("multiplayer_Darkling_05");
	_pWData->GetResourceIndex_Class("multiplayer_Darkling_06");
	_pWData->GetResourceIndex_Class("multiplayer_Darkling_Lightkiller");
	_pWData->GetResourceIndex_Class("multiplayer_Mob_01");
	_pWData->GetResourceIndex_Class("multiplayer_Mob_02");
	_pWData->GetResourceIndex_Class("multiplayer_Mob_03");
	_pWData->GetResourceIndex_Class("multiplayer_Mob_04");
	_pWData->GetResourceIndex_Class("multiplayer_Mob_05");
	_pWData->GetResourceIndex_Class("multiplayer_Mob_06");
	_pWData->GetResourceIndex_Class("multiplayer_Mob_07");
	_pWData->GetResourceIndex_Class("multiplayer_Mob_08");
	_pWData->GetResourceIndex_Class("multiplayer_Swat_01");
	_pWData->GetResourceIndex_Class("multiplayer_Swat_02");
	_pWData->GetResourceIndex_Class("multiplayer_Swat_03");
	_pWData->GetResourceIndex_Class("multiplayer_Swat_04");
	_pWData->GetResourceIndex_Class("multiplayer_Swat_05");
	_pWData->GetResourceIndex_Class("multiplayer_Swat_06");
	_pWData->GetResourceIndex_Class("multiplayer_Swat_07");
	_pWData->GetResourceIndex_Class("multiplayer_Cop_01");
	_pWData->GetResourceIndex_Class("multiplayer_Cop_02");
	_pWData->GetResourceIndex_Class("multiplayer_Cop_03");
	_pWData->GetResourceIndex_Class("mp_pickup_boost_red");
	_pWData->GetResourceIndex_Class("mp_pickup_boost_green");
	_pWData->GetResourceIndex_Class("mp_pickup_boost_blue");
	_pWData->GetResourceIndex_Class("mp_pickup_boost_yellow");
	_pWData->GetResourceIndex_Class("mp_pickup_boost_pink");
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_250");	//Darkness damage
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_251");	//Darkness shield
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_252");	//Darkness health
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_125");	//It's a draw
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_128");	//You win!
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_129");	//You loose!
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_139");	//You have taken the lead
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_141");	//You have lost the lead
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_143");	//You are tied for the lead
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_200");	//Standby
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_201");	//Game begins in
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_202");	//3
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_203");	//2
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_204");	//1
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_205");	//Go
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_206");	//Game ends in
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_207");	//20
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_208");	//10
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_209");	//5
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_210");	//4
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_211");	//Game over
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_262");	//Death Frenzy
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_263");	//Killing Spree
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_264");	//Murder Ballad
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_265");	//Trigger Happy
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_266");	//Dark Massacre
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_267");	//Bloodthirsty
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_268");	//Killing Machine
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_269");	//Serial Killer
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_270");	//Jack the Ripper!
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_272");	//Killing Streak

	_pWData->GetResourceIndex_Class("object_mp_health_small");
	_pWData->GetResourceIndex_Class("object_mp_SMG");
	_pWData->GetResourceIndex_Class("object_mp_automatic");
	_pWData->GetResourceIndex_Class("object_mp_ShgRiot");
	_pWData->GetResourceIndex_Class("object_mp_health_boost");
	_pWData->GetResourceIndex_Class("object_mp_speed_boost");
	_pWData->GetResourceIndex_Class("object_mp_damage_boost");
	_pWData->GetResourceIndex_Class("object_mp_shield");
}

int CWObject_GameDM::OnClientCommand(int _iPlayer, const CCmd* _pCmd)
{
	int Size = _pCmd->m_Size;
	switch(_pCmd->m_Cmd)
	{
	case CONTROL_JOINGAME:
		{
			char Class[256];
			memcpy(Class, _pCmd->m_Data, Size);
			Class[Size] = 0;

			CWO_PlayerDM *pPlayer = Player_GetDM(_iPlayer);
			pPlayer->m_ClassHuman = Class;
			pPlayer->m_bJoined = true;

			int State = m_pWServer->GetMapData()->GetState();
			m_pWServer->GetMapData()->SetState(State & ~WMAPDATA_STATE_NOCREATE);	
			CMat4Dfp32 Mat = GetSpawnPosition();
			int16 Flags = 0;
			Player_Respawn(_iPlayer, Mat, pPlayer->m_ClassHuman, Flags);

			CWObject *pObj = m_pWServer->Object_Get(pPlayer->m_iObject);
			CWObject_CharShapeshifter *pChar = safe_cast<CWObject_CharShapeshifter>(pObj);
			CWO_CharShapeshifter_ClientData &CD = pChar->GetClientData();
			CD.m_iDarklingModel = m_pWServer->GetMapData()->GetResourceIndex_Model(pPlayer->m_ModelDarkling.Str());
			m_pWServer->GetMapData()->SetState(State);

			if(m_GameMode == MP_MODE_DARKLINGS)
			{
				CWObject_Message Msg;
				Msg.m_Msg = OBJMSG_GAME_TOGGLEDARKLINGHUMAN;
				Msg.m_iSender = pPlayer->m_iPlayer;
				Msg.m_Param0 = 1;
				m_pWServer->Message_SendToObject(Msg, m_iObject);
			}
			else
				pChar->RemoveAllDarklingsWeapons();

			OnSetClientWindow(pPlayer->m_iClient, "");

			CNetMsg Msg2;
			Msg2.m_MsgType = NETMSG_GAME_JOIN;
			Msg2.AddStrAny(pPlayer->m_Name);
			m_pWServer->NetMsg_SendToObject(Msg2, m_iObject);
			
			return true;
		}

	case CONTROL_SETNAME:
		{
			char Name[256];
			memcpy(Name, _pCmd->m_Data, Size);
			Name[Size] = 0;

			if(_iPlayer != -1 && _iPlayer < Player_GetNum())
			{
				CWO_PlayerDM *pPlayer = Player_GetDM(_iPlayer);
				pPlayer->m_Name = Name;

				CClientData *pCD = GetClientData();

//				while(pCD->m_lPlayerNames.Len() <= _iPlayer)
//					pCD->m_lPlayerNames.Add("");

				pCD->m_lPlayerNames[_iPlayer] = pPlayer->m_Name;

				CNetMsg Msg2(NETMSG_GAME_SETNAME);
				Msg2.AddInt8(_iPlayer);
				Msg2.AddStrAny(pPlayer->m_Name);
				m_pWServer->NetMsg_SendToObject(Msg2, m_iObject);
			}
			return true;
		}

	case CONTROL_SETDARKLINGNAME:
		{
			char Darkling[256];
			memcpy(Darkling, _pCmd->m_Data, Size);
			Darkling[Size] = 0;

			if(_iPlayer != -1 && _iPlayer < Player_GetNum())
			{
				CWO_PlayerDM *pPlayer = Player_GetDM(_iPlayer);
				pPlayer->m_ModelDarkling = CStrF("Characters\\Darklings\\%s", Darkling);
			}
			return true;
		}

	case CONTROL_SETNAME_PLAYERID:
		{
			if(_iPlayer < Player_GetNum() && _iPlayer != -1)
			{
				CWO_PlayerDM *pPlayer = Player_GetDM(_iPlayer);
				memcpy(&pPlayer->m_player_id, _pCmd->m_Data, Size);
				M_ASSERT(pPlayer->m_player_id != 0, "Invalid player id");
#if defined(PLATFORM_CONSOLE)
				MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
				CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
				for(int i = 0; i < pGameMod->m_pMPHandler->GetNumPlayers(); i++)
				{
					if(pGameMod->m_pMPHandler->GetPlayerID(i) == pPlayer->m_player_id)
					{
						CStr name = pGameMod->m_pMPHandler->GetPlayerName(i);
						pPlayer->m_Name = name;

						CClientData *pCD = GetClientData();
						pCD->m_lPlayerNames[_iPlayer] = pPlayer->m_Name;

						CNetMsg Msg2(NETMSG_GAME_SETNAME);
						Msg2.AddInt8(_iPlayer);
						Msg2.AddStrAny(pPlayer->m_Name);				
						m_pWServer->NetMsg_SendToObject(Msg2, m_iObject);
					}
				}
				if(pGameMod->m_pMPHandler->GetPlayerID(-1) == pPlayer->m_player_id)
				{
					CStr name = pGameMod->m_pMPHandler->GetPlayerName(-1);
					pPlayer->m_Name = name;

					CClientData *pCD = GetClientData();
					pCD->m_lPlayerNames[_iPlayer] = pPlayer->m_Name;

					CNetMsg Msg2(NETMSG_GAME_SETNAME);
					Msg2.AddInt8(_iPlayer);
					Msg2.AddStrAny(pPlayer->m_Name);				
					m_pWServer->NetMsg_SendToObject(Msg2, m_iObject);
				}
#endif
			}
		}
	}
	return CWObject_GameP4::OnClientCommand(_iPlayer, _pCmd);
}

int CWObject_GameDM::OnCharacterKilled(int _iObject, int _iSender)
{
	M_TRACE("CWObject_GameDM::OnCharacterKilled, iObject=%d, iSender=%d\n", _iObject, _iSender);

	uint nPlayers = Player_GetNum();
	uint other_player = ~0;
	for(uint j = 0; j < nPlayers; j++)
	{
		CWO_Player* pPlayer = Player_Get(j);
		if (pPlayer && pPlayer->m_iObject == _iSender)
		{
			other_player = j;
			break;
		}
	}

	for (uint i = 0; i < nPlayers; i++)
	{
		CWO_Player* pPlayer = Player_Get(i);
		if (pPlayer && pPlayer->m_iObject == _iObject)
		{
			CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(_iObject));
			CWO_CharShapeshifter_ClientData& CD = CWObject_CharShapeshifter::GetClientData(pPlayerObj);

			if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_DAMAGE)
			{
				CWObject_PickupInfo PickupInfo;
				PickupInfo.m_TemplateName = "object_mp_damage_boost";
				PickupInfo.m_Pos = pPlayerObj->GetPosition();
				PickupInfo.m_SpawnTick = m_pWServer->GetGameTick();
				PickupInfo.m_TicksLeft = CD.m_DamageBoostDuration - (m_pWServer->GetGameTick() - CD.m_DamageBoostStartTick);
				m_lPickups.Add(PickupInfo);

				int iDrainSoundInOut = m_pWServer->GetMapData()->GetResourceIndex_Sound("gam_drk_crp02");
				int iDrainLoopSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_Drk_crp03");

				if(iDrainSoundInOut)
					m_pWServer->Sound_At(pPlayerObj->GetPosition(), iDrainSoundInOut, WCLIENT_ATTENUATION_3D);

				if(iDrainLoopSound)
					m_pWServer->Sound_Off(pPlayerObj->m_iObject, iDrainLoopSound);

				CWObject_Message MsgDrain(OBJMSG_EFFECTSYSTEM_SETFADE, 2);
				m_pWServer->Message_SendToObject(MsgDrain, pPlayerObj->m_iDarkness_Drain_2);
			}
			if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_SPEED)
			{
				CWObject_PickupInfo PickupInfo;
				PickupInfo.m_TemplateName = "object_mp_speed_boost";
				PickupInfo.m_Pos = pPlayerObj->GetPosition();
				PickupInfo.m_SpawnTick = m_pWServer->GetGameTick();
				PickupInfo.m_TicksLeft = CD.m_SpeedBoostDuration - (m_pWServer->GetGameTick() - CD.m_SpeedBoostStartTick);
				m_lPickups.Add(PickupInfo);
			}
			if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_INVISIBILITY)
			{
				CWObject_PickupInfo PickupInfo;
				PickupInfo.m_TemplateName = "object_mp_invisibility";
				PickupInfo.m_Pos = pPlayerObj->GetPosition();
				PickupInfo.m_SpawnTick = m_pWServer->GetGameTick();
				PickupInfo.m_TicksLeft = CD.m_InvisibiltyBoostDuration - (m_pWServer->GetGameTick() - CD.m_InvisibiltyBoostStartTick);
				m_lPickups.Add(PickupInfo);

				pPlayerObj->ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
			}

			pPlayer->m_Mode = PLAYERMODE_DEAD;

			m_pWServer->Net_ClientCommand(i, "mp_setmultiplayerstatus(1)");

			CWO_PlayerDM *pPlayerKiller, *pPlayerKilled;
			pPlayerKilled = NULL;
			pPlayerKiller = NULL;

			pPlayerKilled = Player_GetDM(i);
			pPlayerKilled->m_DeathTick = m_pWServer->GetGameTick();
			pPlayerKilled->m_KillsInARow = 0;
			CClientData *pCD = GetClientData();

			if(!pCD->m_WarmUpMode)
			{
				if(other_player != ~0)
				{
					pPlayerKiller = Player_GetDM(other_player);
					GivePoint(other_player, 1);
					AddKill(other_player);
				}
				else
				{	//suicide
//					pPlayerKilled->m_Score--;
//					pCD->m_lPlayerScores[i] = pPlayerKilled->m_Score;
					GivePoint(pPlayerKilled->m_iPlayer, -1);
				}
				if(pCD->m_MaxScore && pPlayerKiller && pPlayerKiller->m_Score >= pCD->m_MaxScore)
				{	//We have a winner, end game
					EndGame();
				}
				pCD->m_lPlayerScores.MakeDirty();
				pPlayerKilled->m_Deaths++;
				pCD->m_lPlayerDeaths[i] = pPlayerKilled->m_Deaths;
				pCD->m_lPlayerDeaths.MakeDirty();
#if defined(PLATFORM_CONSOLE)
				MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
				CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

				if(other_player == ~0)
				{	//suicide
					pGameMod->m_pMPHandler->StatisticsSetScore(pPlayerKilled->m_Score, pPlayerKilled->m_player_id);
					pGameMod->m_pMPHandler->StatisticsSetDeaths(pPlayerKilled->m_Deaths, pPlayerKilled->m_player_id);
				}
				else
				{	//kill
					pGameMod->m_pMPHandler->StatisticsSetScore(pPlayerKiller->m_Score, pPlayerKiller->m_player_id);
					pGameMod->m_pMPHandler->StatisticsSetDeaths(pPlayerKilled->m_Deaths, pPlayerKilled->m_player_id);
				}
#endif
				CNetMsg Msg2;
				if(other_player == ~0)
				{
					Msg2.m_MsgType = NETMSG_GAME_PLAYERSUICIDE;
					Msg2.AddStrAny(pPlayerKilled->m_Name);
				}
				else
				{
					Msg2.m_MsgType = NETMSG_GAME_PLAYERKILL;
					Msg2.AddStrAny(pPlayerKilled->m_Name);
					Msg2.AddStrAny(pPlayerKiller->m_Name);
				}
				m_pWServer->NetMsg_SendToObject(Msg2, m_iObject);
			}

			return 1;
		}
	}

	return 0;
}

int CWObject_GameDM::GetHighestScore()
{
	int HighestScore = -999;
	for(int i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerDM* pPlayer = Player_GetDM(i);
		if(pPlayer)
		{
			if(pPlayer->m_Score > HighestScore)
				HighestScore = pPlayer->m_Score;
		}
	}
	return HighestScore;
}

void CWObject_GameDM::GivePoint(int _iPlayer, int _nPoints)
{
	CClientData *pCD = GetClientData();
	CWO_PlayerDM* pPlayer = Player_GetDM(_iPlayer);

	int Score = GetHighestScore();
	bool bTiedOrLeading = Score == pPlayer->m_Score;
	pPlayer->m_Score += _nPoints;
	int ScoreNew = GetHighestScore();
	if(Score < ScoreNew)
	{	//Taken the lead, all players with Score lost the lead (was tied) if no tied players player was already in the lead
		bool bWasTied = false;
		for(int i = 0; i < Player_GetNum(); i++)
		{
			CWO_PlayerDM* pPlayerOther = Player_GetDM(i);
			if(pPlayerOther)
			{
				if(pPlayerOther->m_Score == Score)
				{
					int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_141");
					if(iSound)
						m_pWServer->Sound_Global(iSound, 1.0f, pPlayerOther->m_iPlayer);
					bWasTied = true;
				}
			}
		}
		if(bWasTied)
		{
			int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_139");
			if(iSound)
				m_pWServer->Sound_Global(iSound, 1.0f, pPlayer->m_iPlayer);
		}
	}
	else if(Score == ScoreNew)
	{
		if(pPlayer->m_Score == Score)
		{
			for(int i = 0; i < Player_GetNum(); i++)
			{
				CWO_PlayerDM* pPlayerOther = Player_GetDM(i);
				if(pPlayerOther && pPlayerOther->m_Score == Score)
				{
					int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_143");
					if(iSound)
						m_pWServer->Sound_Global(iSound, 1.0f, pPlayerOther->m_iPlayer);
				}
			}
		}
		else if(_nPoints < 0)
		{
			if(pPlayer->m_Score - _nPoints == Score)
			{
				if(Score == ScoreNew)	//Highest score didn't change, player lost the lead and someone else got it
				{
					int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_141");
					if(iSound)
						m_pWServer->Sound_Global(iSound, 1.0f, pPlayer->m_iPlayer);

					int nNum = 0;
					int iIndex = -1;
					for(int i = 0; i < Player_GetNum(); i++)
					{
						CWO_PlayerDM* pPlayerOther = Player_GetDM(i);
						if(pPlayerOther && pPlayerOther->m_Score == Score)
						{
							nNum++;
							iIndex = pPlayerOther->m_iPlayer;
						}
					}
					if(nNum == 1)
					{
						int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_139");
						if(iSound)
							m_pWServer->Sound_Global(iSound, 1.0f, iIndex);
					}
				}
			}
			else
			{	//Highest score got lowered, we could possible be tied now
				int nNum = 0;
				for(int i = 0; i < Player_GetNum(); i++)
				{
					CWO_PlayerDM* pPlayerOther = Player_GetDM(i);
					if(pPlayerOther && pPlayerOther->m_Score == ScoreNew)
						nNum++;
				}
				if(nNum > 1)
				{	//We are tied
					for(int i = 0; i < Player_GetNum(); i++)
					{
						CWO_PlayerDM* pPlayerOther = Player_GetDM(i);
						if(pPlayerOther && pPlayerOther->m_Score == ScoreNew)
						{
							int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_143");
							if(iSound)
								m_pWServer->Sound_Global(iSound, 1.0f, pPlayerOther->m_iPlayer);
						}
					}
				}
			}
		}
	}

	pCD->m_lPlayerScores[_iPlayer] = pPlayer->m_Score;
}

CStr CWObject_GameDM::GetDefualtSpawnClass()
{
	return "Spectator";
}

void CWObject_GameDM::CheckForWinner()
{

}

aint CWObject_GameDM::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_GAME_CANPICKUPSPAWN:
		if(_Msg.m_Param0 & CWObject_Trigger_Pickup::PICKUP_SPAWNFLAG_DM)
			return 1;
		return 0;
	case OBJMSG_CHAR_CANDARKLINGSUSEWEAPONS:
		{
			if(m_GameMode == MP_MODE_DARKLINGS)
				return 1;
			return 0;
		}
		break;

	case OBJMSG_GAME_GETPLAYER:
		{
			for(int i = 0; i < Player_GetNum(); i++)
			{
				CWO_PlayerDM* pPlayer = Player_GetDM(i);
				if(pPlayer && pPlayer->m_iObject == _Msg.m_Param0)
					return (aint)pPlayer;
			}
			return (aint)0;
		}
		break;

	case OBJMSG_GAME_GETPLAYERWITHINDEX:
		{
			return (aint)Player_GetDM(_Msg.m_Param0);
		}
		break;

	case OBJMSG_CHAR_PICKUP:
		OnPickUp(_Msg.m_iSender, _Msg.m_Param0, _Msg.m_Param1);
		return 1;

	case OBJMSG_GAME_TWEAK_DAMAGE_RANGED:
		for(int i = 0; i < Player_GetNum(); i++)
		{
			CWO_PlayerDM* pPlayer = Player_GetDM(i);
			if(pPlayer)
			{
				if(pPlayer->m_iObject == _Msg.m_Param0)
				{	//This is the sender, if he has quad, increase dmg
					CWObject_CharShapeshifter* pPlayerObj = TDynamicCast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(pPlayer->m_iObject));
					if(pPlayerObj)
					{
						CWO_CharShapeshifter_ClientData& CD = pPlayerObj->GetClientData(pPlayerObj);
						if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_DAMAGE)
							return (aint)(1024 * CD.m_DamageBoostMultiplier);
					}
				}
			}
		}
		return 1024; 

	case OBJMSG_GAME_GETISMP:
		return 1;

	case OBJMSG_GAME_TOGGLEDARKLINGHUMAN:
		ToggleDarklingHuman(_Msg.m_iSender, _Msg.m_Param0 ? true : false);
		return 1;

	case OBJMSG_GAME_RESPAWN:
		{
			CWO_PlayerDM *pPlayer= Player_GetDM(_Msg.m_iSender);
			uint32 compare = pPlayer->m_DeathTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * 1.0f);
			bool bForce = _Msg.m_Param0 ? true : false;
			if(compare > m_pWServer->GetGameTick() && !bForce)
				return 1;
			if(pPlayer->m_Mode == PLAYERMODE_DEAD || bForce)
			{
				CMat4Dfp32 Mat = GetSpawnPosition();
				int16 Flags = 0;

				Player_Respawn(pPlayer->m_iPlayer, Mat, pPlayer->m_ClassHuman, Flags);
				pPlayer->m_Mode = 0;
				if(m_GameMode != MP_MODE_DARKLINGS)
				{
					pPlayer->m_bIsHuman = true;
					m_pWServer->Net_ClientCommand(pPlayer->m_iPlayer, "cl_darknessvision(0)");
				}
				else
				{
					CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(pPlayer->m_iObject));
					CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pPlayerObj);

					CWObject_Message Msg;
					Msg.m_Msg = OBJMSG_GAME_TOGGLEDARKLINGHUMAN;
					Msg.m_iSender = pPlayer->m_iPlayer;
					Msg.m_Param0 = 1;
					m_pWServer->Message_SendToObject(Msg, m_iObject);
				}

				m_pWServer->Net_ClientCommand(_Msg.m_iSender, "mp_setmultiplayerstatus(0)");
			}
		}
		return 1;

	case OBJMSG_GAME_SETGAMEMODE:
		m_GameMode = _Msg.m_Param0;
		return 1;
		
	case OBJMSG_GAME_GETTENSION:
		{
			for(int i = 0; i < Player_GetNum(); i++)
			{
				CWO_PlayerDM* pPlayer = Player_GetDM(i);
				if(pPlayer && pPlayer->m_iObject == _Msg.m_Param0)
				{
					int Tension = 127;
					CWObject *pObj = m_pWServer->Object_Get(pPlayer->m_iObject);
					CWObject_CharShapeshifter *pChar = safe_cast<CWObject_CharShapeshifter>(pObj);
					CWO_Character_ClientData &CD = pChar->GetClientData();

					int LastDamage = pChar->Char()->m_LastDamageTick;	
					if(LastDamage + TruncToInt(10.0f * m_pWServer->GetGameTicksPerSecond()) > m_pWServer->GetGameTick())
						Tension = 255;

					return Tension;
				}
			}
		}
		return 0;

/*	case OBJSYSMSG_GAME_INITWORLD:
#if defined(PLATFORM_XENON)
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

		pGameMod->m_pMPHandler->LoadingDone();
#endif
		return parent::OnMessage(_Msg);*/
	}

	return parent::OnMessage(_Msg);
}

void CWObject_GameDM::OnPickUp(int _iPlayer, int _iObject, int _TicksLeft)
{
	CClientData *pCD = GetClientData();
	if(pCD->m_WarmUpMode)
		return;
	CWObject_Trigger_Pickup *pTriggerObject = safe_cast<CWObject_Trigger_Pickup>(m_pWServer->Object_Get(_iObject));

	uint nPlayers = Player_GetNum();
	uint iPlayerIndex = 0;
	for(uint i = 0; i < nPlayers; i++)
	{
		CWO_Player* pPlayer = Player_Get(i);
		if (pPlayer && pPlayer->m_iObject == _iPlayer)
		{
			iPlayerIndex = i;
			break;
		}
	}

	CWO_PlayerDM *pPlayer = Player_GetDM(iPlayerIndex);
	if(!pTriggerObject->GetTemplateName())
		return;
	
	CWObject *pObj = m_pWServer->Object_Get(_iPlayer);

	switch(pTriggerObject->m_Type) 
	{
	case CWObject_Trigger_Pickup::PICKUP_HEALTH:
		{
			//increase health
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_ADDHEALTH, pTriggerObject->m_Param0), pPlayer->m_iObject);
			m_pWServer->Object_Create("mp_pickup_boost_red", pTriggerObject->GetPositionMatrix(), 0);
			break;
		}

	case CWObject_Trigger_Pickup::PICKUP_AMMO:
		{
			CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(pPlayer->m_iObject));
			CRPG_Object_Inventory* pWeaponInv = pPlayerObj->Char()->GetInventory(AG2_ITEMSLOT_WEAPONS);
			for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
			{
				CRPG_Object_Item* pWeapon = pWeaponInv->GetItemByIndex(i);
				if(pWeapon->m_iItemType == pTriggerObject->m_Param0)
					((CRPG_Object_Weapon *)pWeapon)->AddMagazine(1);
			}
			break;
		}
	case CWObject_Trigger_Pickup::PICKUP_WEAPON:
		{
			if (m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_CANPICKUPITEM, pTriggerObject->m_spDummyObject->m_iItemType), _iPlayer))
			{
				CWObject_Character *pChar = safe_cast<CWObject_Character>(pObj);
				CRPG_Object_Inventory* pWeaponInv = pChar->Char()->GetInventory(AG2_ITEMSLOT_WEAPONS);

				bool bAddedAsAmmo = false;
				for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
				{
					CRPG_Object_Item* pWeapon = pWeaponInv->GetItemByIndex(i);
					if(pWeapon->m_iItemType == pTriggerObject->m_spDummyObject->m_iItemType)
					{	//We already have weapon, add ammo instead
						bAddedAsAmmo = true;
						((CRPG_Object_Weapon *)pWeapon)->AddMagazine(1);
					}
				}

				if(!bAddedAsAmmo)
				{
					CRPG_Object_Item* pCurItem = pWeaponInv->FindFinalSelectedItem();
					uint16 bDontAutoEquip = 1;
					if(pCurItem && pCurItem->m_iItemType == 1)
						bDontAutoEquip = 0;

					CWObject_Message Msg(OBJMSG_RPG_AVAILABLEPICKUP, (pTriggerObject->m_spDummyObject->m_iItemType), aint((CRPG_Object_Item *)(CReferenceCount *)pTriggerObject->m_spDummyObject));
					Msg.m_Reason = bDontAutoEquip; //Don't autoequip this item. 
					Msg.m_pData = (char *)pTriggerObject->m_WeaponTemplate;
					Msg.m_DataSize = pTriggerObject->m_WeaponTemplate.Len();
					Msg.m_iSender = m_iObject;

					m_pWServer->Message_SendToObject(Msg, _iPlayer);
				}
			}
			break;
		}
	case CWObject_Trigger_Pickup::PICKUP_SPEED_BOOST:
		{
			CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(pObj);
			CWO_CharShapeshifter_ClientData& CD = pPlayerObj->GetClientData();
			CD.m_ActiveBoosts = CD.m_ActiveBoosts | MP_BOOST_ACTIVE_SPEED;
			CD.m_SpeedBoostMultiplier = pTriggerObject->m_Multiplier;
			CD.m_SpeedBoostStartTick = m_pWServer->GetGameTick();
			CD.m_SpeedBoostDuration = RoundToInt(pTriggerObject->m_Duration * m_pWServer->GetGameTicksPerSecond() - _TicksLeft);

			CD.m_Speed_Forward = CD.m_Speed_Forward * CD.m_SpeedBoostMultiplier;
			CD.m_Speed_SideStep = CD.m_Speed_SideStep * CD.m_SpeedBoostMultiplier;
			CD.m_Speed_Up = CD.m_Speed_Up * CD.m_SpeedBoostMultiplier;
			CD.m_Speed_Jump = CD.m_Speed_Jump * CD.m_SpeedBoostMultiplier;

			m_pWServer->Object_Create("mp_pickup_boost_pink", pTriggerObject->GetPositionMatrix(), 0);

			break;
		}
	case CWObject_Trigger_Pickup::PICKUP_DAMAGE_BOOST:
		{
			CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(pObj);
			CWO_CharShapeshifter_ClientData& CD = pPlayerObj->GetClientData();
			CD.m_ActiveBoosts = CD.m_ActiveBoosts | MP_BOOST_ACTIVE_DAMAGE;
			CD.m_DamageBoostMultiplier = pTriggerObject->m_Multiplier;
			CD.m_DamageBoostStartTick = m_pWServer->GetGameTick();
			CD.m_DamageBoostDuration = RoundToInt(pTriggerObject->m_Duration * m_pWServer->GetGameTicksPerSecond() - _TicksLeft);

			int iDrainSoundInOut = m_pWServer->GetMapData()->GetResourceIndex_Sound("gam_drk_crp01");
			int iDrainLoopSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_Drk_crp03");
			int iDamagePickup = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_250");
			if (iDrainSoundInOut)
				m_pWServer->Sound_At(pPlayerObj->GetPosition(), iDrainSoundInOut, WCLIENT_ATTENUATION_3D);

			if(iDrainLoopSound)
				m_pWServer->Sound_On(pPlayerObj->m_iObject, iDrainLoopSound, WCLIENT_ATTENUATION_FORCE_3D_LOOP);

			if(iDamagePickup)
				m_pWServer->Sound_Global(iDamagePickup, 1.0f, iPlayerIndex);

			CWObject_Message MsgDrain(OBJMSG_EFFECTSYSTEM_SETFADE, 1);
			if(iDrainLoopSound)
				m_pWServer->Message_SendToObject(MsgDrain, pPlayerObj->m_iDarkness_Drain_2);

			m_pWServer->Object_Create("mp_pickup_boost_blue", pTriggerObject->GetPositionMatrix(), 0);
			break;
		}
	case CWObject_Trigger_Pickup::PICKUP_HEALTH_BOOST:
		{
			CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(pObj);
			CWO_CharShapeshifter_ClientData& CD = pPlayerObj->GetClientData();
			if(!(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_HEALTH))
			{
				int MaxHealth = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETMAXHEALTH), pPlayer->m_iObject);
				int CurHealth = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETHEALTH), pPlayer->m_iObject);

				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETMAXHEALTH, pTriggerObject->m_Param0 + MaxHealth), pPlayer->m_iObject);
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETHEALTH, pTriggerObject->m_Param0 + CurHealth), pPlayer->m_iObject);
				
				CD.m_ActiveBoosts = CD.m_ActiveBoosts | MP_BOOST_ACTIVE_HEALTH;
				CD.m_HealthBoostAmount = pTriggerObject->m_Param0;
				CD.m_HealthBoostStartTick = m_pWServer->GetGameTick();
				CD.m_HealthBoostDuration = RoundToInt(pTriggerObject->m_Duration * m_pWServer->GetGameTicksPerSecond());
			}
			else
			{
				int CurHealth = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETHEALTH), pPlayer->m_iObject);
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETHEALTH, pTriggerObject->m_Param0 + CurHealth), pPlayer->m_iObject);
				CD.m_HealthBoostStartTick = m_pWServer->GetGameTick();
			}

			int iHealthPickup = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_252");
			if(iHealthPickup)
				m_pWServer->Sound_Global(iHealthPickup, 1.0f, iPlayerIndex);

			m_pWServer->Object_Create("mp_pickup_boost_red", pTriggerObject->GetPositionMatrix(), 0);

			break;
		}
	case CWObject_Trigger_Pickup::PICKUP_SHIELD:
		{
			CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(pObj);
			CWO_CharShapeshifter_ClientData& CD = pPlayerObj->GetClientData();

			CD.m_ActiveBoosts = CD.m_ActiveBoosts | MP_BOOST_ACTIVE_SHIELD;
			CD.m_ShieldBoostStartTick = m_pWServer->GetGameTick();
			CD.m_ShieldBoostDuration = RoundToInt(pTriggerObject->m_Duration * m_pWServer->GetGameTicksPerSecond());
			m_pWServer->Object_Create("mp_pickup_boost_green", pTriggerObject->GetPositionMatrix(), 0);
			int iShieldPickup = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_251");
			if(iShieldPickup)
				m_pWServer->Sound_Global(iShieldPickup, 1.0f, iPlayerIndex);
			break;
		}
	case CWObject_Trigger_Pickup::PICKUP_INVISIBILITY:
		{
			CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(pObj);
			CWO_CharShapeshifter_ClientData& CD = pPlayerObj->GetClientData();
			CD.m_ActiveBoosts = CD.m_ActiveBoosts | MP_BOOST_ACTIVE_INVISIBILITY;
			CD.m_InvisibiltyBoostStartTick = m_pWServer->GetGameTick();
			CD.m_InvisibiltyBoostDuration = RoundToInt(pTriggerObject->m_Duration * m_pWServer->GetGameTicksPerSecond() - _TicksLeft);

			pPlayerObj->ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;
			m_pWServer->Object_Create("mp_pickup_boost_yellow", pTriggerObject->GetPositionMatrix(), 0);
			break;
		}
	}

	m_pWServer->Sound_At(pObj->GetPosition(), pTriggerObject->m_iSound, WCLIENT_ATTENUATION_3D);

	if(!_TicksLeft)
	{
		CWObject_PickupInfo PickupInfo;
		PickupInfo.m_TemplateName = pTriggerObject->GetTemplateName();
		PickupInfo.m_Pos = pTriggerObject->GetPosition();
		PickupInfo.m_SpawnTick = m_pWServer->GetGameTick() + (uint32)(pTriggerObject->m_SpawnTime * m_pWServer->GetGameTicksPerSecond());
		PickupInfo.m_TicksLeft = 0;
		m_lPickups.Add(PickupInfo);
	}

	m_pWServer->Object_Destroy(_iObject);
}


void CWObject_GameDM::ToggleDarklingHuman(int16 _iSender, bool _bForce)
{
	CClientData *pCD = GetClientData();
	if((m_GameMode != MP_MODE_SHAPESHIFTER || pCD->m_GameOver) && !_bForce)
		return;

	CWO_PlayerDM* pPlayer = Player_GetDM(_iSender);
	if (pPlayer && pPlayer->m_Mode != PLAYERMODE_DEAD)
	{
		CMat4Dfp32 Mat;
		int16 Flags = 0;
		CWServer_Mod* pServer = safe_cast<CWServer_Mod>(m_pWServer);
		M_ASSERT(pServer, "No server!");

		CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(pPlayer->m_iObject));
		CWO_CharShapeshifter_ClientData& CD = CWObject_CharShapeshifter::GetClientData(pPlayerObj);
		pPlayerObj->ToggleDarklingHuman(_bForce);
		pPlayer->m_bIsHuman = CD.m_IsHuman ? true : false;
	}
}

spCWO_Player CWObject_GameDM::CreatePlayerObject()
{
	CWO_PlayerDM *pP = MNew(CWO_PlayerDM);
	spCWO_Player spP = (CWO_Player *)pP;
	if (!spP) MemError("CreateChar");

	return spP;
}

aint CWObject_GameDM::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAMEMOD_RENDERMULTIPLAYERSTATUS:
		OnClientRenderMultiplayerStatus(_pObj, _pWClient, (CXR_Engine *)_Msg.m_Param0, (CRC_Util2D *)_Msg.m_Param1);
		return 1;

	case OBJMSG_GAME_RENDERSTATUSBAR:
		OnClientRenderStatusBar(_pObj, _pWClient, (CXR_Engine *)_Msg.m_Param0, (CRC_Util2D *)_Msg.m_Param1, (CVec2Dfp32 *)_Msg.m_pData);
		return 1;

	case OBJMSG_GAME_GETISMP:
		return 1;
	}

	return parent::OnClientMessage(_pObj, _pWClient, _Msg);
}

void CWObject_GameDM::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	switch(_Msg.m_MsgType)
	{
	case NETMSG_GAME_PLAYERKILL:
		{
			int p = 0;
			
			CClientData *pCD = GetClientData(_pObj);
			CClientData::SMultiplayerMessage MMsg;
			MMsg.m_Type = NETMSG_GAME_PLAYERKILL;
			MMsg.m_Str1 = _Msg.GetStrAny(p);
			MMsg.m_Str2 = _Msg.GetStrAny(p);
			MMsg.m_StartTick = _pWClient->GetGameTick();
			MMsg.m_Duration = RoundToInt(3.0f * _pWClient->GetGameTicksPerSecond());
			while(pCD->m_lMultiplayerMessages.Len() >= 4)
				pCD->m_lMultiplayerMessages.Del(0);
			pCD->m_lMultiplayerMessages.Add(MMsg);
		}
		return;

	case NETMSG_GAME_PLAYERSUICIDE:
		{
			int p = 0;

			CClientData *pCD = GetClientData(_pObj);
			CClientData::SMultiplayerMessage MMsg;
			MMsg.m_Type = NETMSG_GAME_PLAYERSUICIDE;
			MMsg.m_Str1 = _Msg.GetStrAny(p);
			MMsg.m_StartTick = _pWClient->GetGameTick();
			MMsg.m_Duration = RoundToInt(3.0f * _pWClient->GetGameTicksPerSecond());
			while(pCD->m_lMultiplayerMessages.Len() >= 4)
				pCD->m_lMultiplayerMessages.Del(0);
			pCD->m_lMultiplayerMessages.Add(MMsg);
		}
		return;

	case NETMSG_GAME_PLAYER_DISCONNECT:
		{
			int p = 0;

			CClientData *pCD = GetClientData(_pObj);
			CClientData::SMultiplayerMessage MMsg;
			MMsg.m_Type = NETMSG_GAME_PLAYER_DISCONNECT;
			MMsg.m_Str1 = _Msg.GetStrAny(p);
			uint8 iPlayer = _Msg.GetInt8(p);
			int Reason = _Msg.GetInt32(p);

//			0 : Server shutdown
//			1 : Client quit
//			2 : Client timeout

			switch(Reason) 
			{
			case 0:
#if defined(PLATFORM_XENON)
				MMsg.m_Str2 = WTEXT("§LMP_DISCONNECT_SERVER_QUIT");	
#else
				MMsg.m_Str2 = "§LMP_DISCONNECT_SERVER_QUIT";	
#endif
				break;
			case 1:
#if defined(PLATFORM_XENON)
				MMsg.m_Str2 = WTEXT("§LMP_DISCONNECT_SERVER_TIMEOUT");	
#else
				MMsg.m_Str2 = "§LMP_DISCONNECT_SERVER_TIMEOUT";	
#endif
				break;
			case 2:
#if defined(PLATFORM_XENON)
				MMsg.m_Str2 = WTEXT("§LMP_DISCONNECT_QUIT");	
#else
				MMsg.m_Str2 = "§LMP_DISCONNECT_QUIT";	
#endif
				break;
			}

			MMsg.m_StartTick = _pWClient->GetGameTick();
			MMsg.m_Duration = RoundToInt(3.0f * _pWClient->GetGameTicksPerSecond());
			while(pCD->m_lMultiplayerMessages.Len() >= 4)
				pCD->m_lMultiplayerMessages.Del(0);
			pCD->m_lMultiplayerMessages.Add(MMsg);

			if(pCD->m_lPlayerNames.Len() > iPlayer)
				pCD->m_lPlayerNames[iPlayer] = "";
			if(pCD->m_lPlayerScores.Len() > iPlayer)
				pCD->m_lPlayerScores[iPlayer] = 0;
			if(pCD->m_lPlayerDeaths.Len() > iPlayer)
				pCD->m_lPlayerDeaths[iPlayer] = 0;
			pCD->m_lPlayerScores.MakeDirty();
		}
		return;

	case NETMSG_GAME_SETNAME:
		{
			int p = 0;

			CClientData *pCD = GetClientData(_pObj);
			uint8 iPlayer = _Msg.GetInt8(p);
			CStr name;
			name = _Msg.GetStrAny(p);
			//This is because players can connect and then join the game in different orders, but we need the order to match
			//so if some players are slow to join, we fill them in as empty names while we wait on them
			CStr EmptyName;
			while(iPlayer >= pCD->m_lPlayerNames.Len())
				pCD->m_lPlayerNames.Add(EmptyName);

			while(iPlayer >= pCD->m_lPlayerScores.Len())
				pCD->m_lPlayerScores.Add(0);

			while(iPlayer >= pCD->m_lPlayerDeaths.Len())
				pCD->m_lPlayerDeaths.Add(0);

			pCD->m_lPlayerNames[iPlayer] = name;
		}
		return;
		
	case NETMSG_GAME_JOIN:
		{
			int p = 0;

			CClientData *pCD = GetClientData(_pObj);
			CClientData::SMultiplayerMessageImportant MMsg;
			MMsg.m_Type = NETMSG_GAME_JOIN;
			MMsg.m_Str1 = _Msg.GetStrAny(p);
			MMsg.m_StartTick = _pWClient->GetGameTick();
			MMsg.m_Duration = RoundToInt(2.0f * _pWClient->GetGameTicksPerSecond());
			while(pCD->m_lMultiplayerMessagesImportant.Len() >= 4)
				pCD->m_lMultiplayerMessagesImportant.Del(0);
			pCD->m_lMultiplayerMessagesImportant.Add(MMsg);
		}
		break;

	case NETMSG_GAME_JOIN_TEAM:
		{
			int p = 0;

			CClientData *pCD = GetClientData(_pObj);
			CClientData::SMultiplayerMessageImportant MMsg;
			MMsg.m_Type = NETMSG_GAME_JOIN_TEAM;
			MMsg.m_Str1 = _Msg.GetStrAny(p);
			MMsg.m_StartTick = _pWClient->GetGameTick();
			MMsg.m_Duration = RoundToInt(2.0f * _pWClient->GetGameTicksPerSecond());
			while(pCD->m_lMultiplayerMessagesImportant.Len() >= 4)
				pCD->m_lMultiplayerMessagesImportant.Del(0);
			pCD->m_lMultiplayerMessagesImportant.Add(MMsg);
		}
		break;
	case NETMSG_GAME_ACHIEVEMENT:
		{
			int p = 0;
			int8 Achievement = _Msg.GetInt8(p);
			AwardAchievement(Achievement, true);
		}
		break;
	}
	return parent::OnClientNetMsg(_pObj, _pWClient, _Msg);
}

void CWObject_GameDM::AddKill(int _iPlayer)
{
	CWO_PlayerDM *pPlayer = Player_GetDM(_iPlayer);
	pPlayer->m_KillsInARow++;
	int iSound = 0;
	switch(pPlayer->m_KillsInARow)
	{
	case 4:
		if(pPlayer->m_bIsHuman)
			iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_272");
		else
			iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_267");
		break;
	case 8:
		{
			if(pPlayer->m_bIsHuman)
				iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_263");
			else
				iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_266");
			CWObject *pObj = m_pWServer->Object_Get(pPlayer->m_iObject);
			CNetMsg Msg(WPACKET_OBJNETMSG);
			Msg.AddInt8(NETMSG_GAME_ACHIEVEMENT);
			Msg.AddInt16(m_iObject);
			Msg.AddInt16(m_iClass);
			Msg.AddInt8(ACHIEVEMENT_KILLING_STREAK);
			m_pWServer->NetMsg_SendToClient(Msg, pPlayer->m_iPlayer);
		}
		break;
	case 12:
		if(pPlayer->m_bIsHuman)
			iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_265");
		else
			iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_262");
	    break;
	case 16:
		if(pPlayer->m_bIsHuman)
			iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_268");
		else
			iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_264");
	    break;
	case 20:
		if(pPlayer->m_bIsHuman)
			iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_269");
		else
			iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_270");
	    break;
	}
	if(iSound)
		m_pWServer->Sound_Global(iSound);
}

void CWObject_GameDM::OnRefresh()
{
	CClientData *pCD = GetClientData();
	//TODO
	//Fix this better
	if(m_bDoOnce)
	{
#if defined(PLATFORM_CONSOLE)
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

		pGameMod->m_pMPHandler->LoadingDone();
#endif
		m_bDoOnce = false;		
	}

	parent::OnRefresh();

	for(int i = 0; i < m_lPickups.Len(); i++)
	{
		if(m_lPickups[i].m_SpawnTick <= m_pWServer->GetGameTick())
		{
			CMat4Dfp32 Mat;
			Mat.Unit();
			Mat.GetRow(3) = m_lPickups[i].m_Pos;
			//if we fail to spawn it, try again next frame
			int iObj = m_pWServer->Object_Create(m_lPickups[i].m_TemplateName, Mat);
			if(iObj == -1)
				continue;
			if(m_lPickups[i].m_TicksLeft)
			{
				CWObject_Message Msg(OBJMSG_TRIGGER_PICKUP_SETTICKSLEFT, m_lPickups[i].m_TicksLeft);
				m_pWServer->Message_SendToObject(Msg, iObj);
			}
			m_lPickups.Del(i);
			return;
		}
	}

	for(int i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerDM *pPlayer = Player_GetDM(i);
		if(pPlayer)
		{
			CWObject_CharShapeshifter* pPlayerObj = TDynamicCast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(pPlayer->m_iObject));
			if(pPlayerObj) 
			{
				CWO_CharShapeshifter_ClientData& CD = pPlayerObj->GetClientData();
				if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_SPEED)
				{
					if(m_pWServer->GetGameTick() > CD.m_SpeedBoostStartTick + CD.m_SpeedBoostDuration)
					{
						CD.m_ActiveBoosts = CD.m_ActiveBoosts & ~MP_BOOST_ACTIVE_SPEED;
						pPlayerObj->ResetSpeeds();
					}
				}

				if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_DAMAGE)
				{
					if(m_pWServer->GetGameTick() > CD.m_DamageBoostStartTick + CD.m_DamageBoostDuration)
					{
						CD.m_ActiveBoosts = CD.m_ActiveBoosts & ~MP_BOOST_ACTIVE_DAMAGE;

						int iDrainSoundInOut = m_pWServer->GetMapData()->GetResourceIndex_Sound("gam_drk_crp02");
						int iDrainLoopSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_Drk_crp03");

						if(iDrainSoundInOut)
							m_pWServer->Sound_At(pPlayerObj->GetPosition(), iDrainSoundInOut, WCLIENT_ATTENUATION_3D);

						if(iDrainLoopSound)
							m_pWServer->Sound_Off(pPlayerObj->m_iObject, iDrainLoopSound);
						
						CWObject_Message MsgDrain(OBJMSG_EFFECTSYSTEM_SETFADE, 2);
						m_pWServer->Message_SendToObject(MsgDrain, pPlayerObj->m_iDarkness_Drain_2);
					}
				}

				if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_HEALTH)
				{
					if(m_pWServer->GetGameTick() > CD.m_HealthBoostStartTick + CD.m_HealthBoostDuration)
					{
						CD.m_ActiveBoosts = CD.m_ActiveBoosts & ~MP_BOOST_ACTIVE_HEALTH;
						int CurHealth = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETHEALTH), pPlayer->m_iObject);
						int MaxHealth = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETMAXHEALTH), pPlayer->m_iObject);
						m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETMAXHEALTH, MaxHealth - CD.m_HealthBoostAmount), pPlayer->m_iObject);
						MaxHealth = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETMAXHEALTH), pPlayer->m_iObject);
						if(CurHealth > MaxHealth)
 							m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETHEALTH, MaxHealth), pPlayer->m_iObject);
					}
				}

				if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_SHIELD)
				{
					if(m_pWServer->GetGameTick() > CD.m_ShieldBoostStartTick + CD.m_ShieldBoostDuration)
						CD.m_ActiveBoosts = CD.m_ActiveBoosts & ~MP_BOOST_ACTIVE_SHIELD;
				}

				if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_INVISIBILITY)
				{
					if(m_pWServer->GetGameTick() > CD.m_InvisibiltyBoostStartTick + CD.m_InvisibiltyBoostDuration)
					{
						CD.m_ActiveBoosts = CD.m_ActiveBoosts & ~MP_BOOST_ACTIVE_INVISIBILITY;
						pPlayerObj->ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
					}
				}
			}
			else if(pPlayer->m_bReady && pPlayer->m_ClassHuman.IsEmpty() && pPlayer->m_ModelDarkling.IsEmpty() && !(m_pWServer->GetGameTick() % 15))
			{
				CWObject* pObj = m_pWServer->Object_Get(pPlayer->m_iObject);
				if(pObj)	//Player is still a spectator
				{
					CNetMsg Msg(WPACKET_OBJNETMSG);
					Msg.AddInt8(PLAYER_NETMSG_MP_GETTEMPLATES);
					Msg.AddInt16(pObj->m_iObject);
					Msg.AddInt16(pObj->m_iClass);
					m_pWServer->NetMsg_SendToClient(Msg, pPlayer->m_iPlayer);
				}
			}
			if(pPlayer->m_Mode == PLAYERMODE_DEAD)
			{
				if(m_pWServer->GetGameTick() > pPlayer->m_DeathTick + 15.0f * m_pWServer->GetGameTicksPerSecond())
				{
					CWObject_Message Msg(OBJMSG_GAME_RESPAWN);
					Msg.m_iSender = pPlayer->m_iPlayer;
					m_pWServer->Message_SendToObject(Msg, m_iObject);
				}
			}
		}
	}

	if(pCD->m_WarmUpMode == MP_WARMUP_MODE_PREGAME)
	{
		int NumPlayersJoined = 0;
		for(int i = 0; i < Player_GetNum(); i++)
		{
			CWO_PlayerDM *pPlayer = Player_GetDM(i);
			if(pPlayer && pPlayer->m_bJoined)
				NumPlayersJoined++;
		}
		if(m_nNumPlayersNeeded <= NumPlayersJoined)
		{
			if(pCD->m_WarmUpCountDownTick == -1)
			{
				pCD->m_WarmUpCountDownTick = m_pWServer->GetGameTick();
				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_201");
				if(iSound)
					m_pWServer->Sound_Global(iSound);
			}
			else
			{
				int Ticks = RoundToInt(WARMUPCOUNTDOWN * m_pWServer->GetGameTicksPerSecond()) - (m_pWServer->GetGameTick() - pCD->m_WarmUpCountDownTick);
				if(Ticks == RoundToInt(3.0f * m_pWServer->GetGameTicksPerSecond()))
				{
					int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_202");
					if(iSound)
						m_pWServer->Sound_Global(iSound);
				}
				else if(Ticks == RoundToInt(2.0f * m_pWServer->GetGameTicksPerSecond()))
				{
					int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_203");
					if(iSound)
						m_pWServer->Sound_Global(iSound);
				}
				else if(Ticks == RoundToInt(1.0f * m_pWServer->GetGameTicksPerSecond()))
				{
					int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_204");
					if(iSound)
						m_pWServer->Sound_Global(iSound);
				}
			}

			if(pCD->m_WarmUpCountDownTick != -1 && (m_pWServer->GetGameTick() - pCD->m_WarmUpCountDownTick) * m_pWServer->GetGameTickTime() > WARMUPCOUNTDOWN)
			{
				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_205");
				if(iSound)
					m_pWServer->Sound_Global(iSound);
				pCD->m_WarmUpMode = MP_WARMUP_MODE_NONE;
				pCD->m_StartTick = m_pWServer->GetGameTick();
				RespawnAll();
			}
		}
		else
			pCD->m_WarmUpCountDownTick = -1;
	}
	else if(pCD->m_WarmUpMode == MP_WARMUP_MODE_BETWEEN_ROUNDS)
	{
		if(pCD->m_WarmUpCountDownTick != -1 && (m_pWServer->GetGameTick() - pCD->m_WarmUpCountDownTick) * m_pWServer->GetGameTickTime() > WARMUPCOUNTDOWN)
		{
			pCD->m_WarmUpMode = MP_WARMUP_MODE_NONE;
			RespawnAll();
			pCD->m_WarmUpCountDownTick = -1;
		}
	}
	else
	{
		int NumPlayersJoined = 0;
		for(int i = 0; i < Player_GetNum(); i++)
		{
			CWO_PlayerDM *pPlayer = Player_GetDM(i);
			if(pPlayer && pPlayer->m_bJoined)
				NumPlayersJoined++;
		}
		if(NumPlayersJoined < m_nNumPlayersNeeded)
			pCD->m_WarmUpMode = MP_WARMUP_MODE_PREGAME;
	}

	if(pCD->m_WarmUpMode == MP_WARMUP_MODE_NONE)
	{
		int Ticks = RoundToInt(pCD->m_MaxTime * m_pWServer->GetGameTicksPerSecond()) - (m_pWServer->GetGameTick() - pCD->m_StartTick);
		int GameEndsSoon = RoundToInt(23.0f * m_pWServer->GetGameTicksPerSecond());
		if(Ticks <= GameEndsSoon)
		{
			if(Ticks == GameEndsSoon)
			{
				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_206");
				if(iSound)
					m_pWServer->Sound_Global(iSound);
			}
			else if(Ticks == RoundToInt(21.0f * m_pWServer->GetGameTicksPerSecond()))
			{
				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_207");
				if(iSound)
					m_pWServer->Sound_Global(iSound);
			}
			else if(Ticks == RoundToInt(11.0f * m_pWServer->GetGameTicksPerSecond()))
			{
				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_208");
				if(iSound)
					m_pWServer->Sound_Global(iSound);
			}
			else if(Ticks == RoundToInt(6.0f * m_pWServer->GetGameTicksPerSecond()))
			{
				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_209");
				if(iSound)
					m_pWServer->Sound_Global(iSound);
			}
			else if(Ticks == RoundToInt(5.0f * m_pWServer->GetGameTicksPerSecond()))
			{
				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_210");
				if(iSound)
					m_pWServer->Sound_Global(iSound);
			}
			else if(Ticks == RoundToInt(4.0f * m_pWServer->GetGameTicksPerSecond()))
			{
				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_202");
				if(iSound)
					m_pWServer->Sound_Global(iSound);
			}
			else if(Ticks == RoundToInt(3.0f * m_pWServer->GetGameTicksPerSecond()))
			{
				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_203");
				if(iSound)
					m_pWServer->Sound_Global(iSound);
			}
			else if(Ticks == RoundToInt(2.0f * m_pWServer->GetGameTicksPerSecond()))
			{
				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_204");
				if(iSound)
					m_pWServer->Sound_Global(iSound);
			}
		}
	}

	if(!pCD->m_WarmUpMode && !pCD->m_GameOver && ((pCD->m_MaxTime - 1.0f) * m_pWServer->GetGameTicksPerSecond() + pCD->m_StartTick) < m_pWServer->GetGameTick())
	{
		EndGame();
		int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_211");
		if(iSound)
			m_pWServer->Sound_Global(iSound);
	}
	if(pCD->m_GameOver)
	{
		if(m_pWServer->GetGameTick() - m_GameOverTick > (int)(15.0f * m_pWServer->GetGameTicksPerSecond()))
		{
			for(int i = 0; i < Player_GetNum(); i++)
				m_pWServer->Net_ClientCommand(i, "mp_setmultiplayerstatus(0)");
#if defined(PLATFORM_CONSOLE)
			MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
			CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

			pGameMod->m_pMPHandler->PrepareMatchOver();
#endif
		}
	}
}

void CWObject_GameDM::RespawnAll(bool _bAlreadySpawned)
{
	for(int i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerDM *pPlayer = Player_GetDM(i);
		if(pPlayer && pPlayer->m_bJoined)
		{
			CWObject_CharShapeshifter* pPlayerObj = TDynamicCast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(pPlayer->m_iObject));
			if(pPlayerObj) 
			{
				CWO_CharShapeshifter_ClientData& CD = pPlayerObj->GetClientData();
				if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_SPEED)
				{
					CD.m_ActiveBoosts = CD.m_ActiveBoosts & ~MP_BOOST_ACTIVE_SPEED;
					pPlayerObj->ResetSpeeds();
				}

				if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_DAMAGE)
				{
					CD.m_ActiveBoosts = CD.m_ActiveBoosts & ~MP_BOOST_ACTIVE_DAMAGE;

					CWObject_Message MsgDrain(OBJMSG_EFFECTSYSTEM_SETFADE, 2);
					m_pWServer->Message_SendToObject(MsgDrain, pPlayerObj->m_iDarkness_Drain_2);
				}

				if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_HEALTH)
				{
					CD.m_ActiveBoosts = CD.m_ActiveBoosts & ~MP_BOOST_ACTIVE_HEALTH;
					int CurHealth = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETHEALTH), pPlayer->m_iObject);
					int MaxHealth = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETMAXHEALTH), pPlayer->m_iObject);
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETMAXHEALTH, MaxHealth - CD.m_HealthBoostAmount), pPlayer->m_iObject);
					MaxHealth = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETMAXHEALTH), pPlayer->m_iObject);
					if(CurHealth > MaxHealth)
						m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETHEALTH, MaxHealth), pPlayer->m_iObject);
				}

				if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_SHIELD)
				{
					CD.m_ActiveBoosts = CD.m_ActiveBoosts & ~MP_BOOST_ACTIVE_SHIELD;
				}

				if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_INVISIBILITY)
				{
					CD.m_ActiveBoosts = CD.m_ActiveBoosts & ~MP_BOOST_ACTIVE_INVISIBILITY;
					pPlayerObj->ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
				}
			}
			if(!_bAlreadySpawned)
			{	
				CMat4Dfp32 Mat = GetSpawnPosition();
				int16 Flags = 0;
				Player_Respawn(i, Mat, pPlayer->m_ClassHuman, Flags);
			}
		}
	}
}

int CWObject_GameDM::Player_Respawn(int _iPlayer, const CMat4Dfp32& _Pos, const char* _pClassName, int _Param1, int _iObj)
{
	CWO_PlayerDM *pPlayer = Player_GetDM(_iPlayer);
	CWObject_CharShapeshifter* pPlayerObj = TDynamicCast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(pPlayer->m_iObject));
	int iDarklingModel = -1;
	if(pPlayerObj)
	{
		CWO_CharShapeshifter_ClientData& CD = pPlayerObj->GetClientData();
		iDarklingModel = CD.m_iDarklingModel;
	}

	int Ret = parent::Player_Respawn(_iPlayer, _Pos, _pClassName, _Param1, _iObj);

	if(pPlayerObj)
	{
		pPlayerObj = TDynamicCast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(pPlayer->m_iObject));
		if(pPlayerObj)
		{
			CNetMsg Msg(PLAYER_NETMSG_RESET_POSTANIMSYSTEM); 
			m_pWServer->NetMsg_SendToObject(Msg, pPlayerObj->m_iObject); 

			pPlayerObj->EquipSomething();
			CWO_CharShapeshifter_ClientData& CD2 = pPlayerObj->GetClientData();
			CD2.m_iDarklingModel = iDarklingModel;
			CD2.m_PostAnimSystem.ResetValues();
		}
	}

	return Ret;
}

int CWObject_GameDM::OnRemoveClient(int _iClient, int _Reason)
{
	if(_iClient == -1)
	{
		ConOutL("§cf80WARNING: Dropping client with index -1");
		return 1;
	}

	CNetMsg Msg2(NETMSG_GAME_PLAYER_DISCONNECT);
	CWO_PlayerDM *pPlayer = Player_GetDM(_iClient);
	if(!pPlayer)
	{
		ConOutL(CStrF("§cf80WARNING: Failed to get client %i in OnRemoveClient", _iClient));
		return 1;
	}

#if defined(PLATFORM_CONSOLE)
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

	pGameMod->m_pMPHandler->ClientDropped(-1, pPlayer->m_player_id);
#endif

	Msg2.AddStrAny(pPlayer->m_Name);
	Msg2.AddInt8(_iClient);
	Msg2.AddInt32(_Reason);
	m_pWServer->NetMsg_SendToObject(Msg2, m_iObject);

	CClientData *pCD = GetClientData();

	if(pCD->m_lPlayerNames.Len() > _iClient)
		pCD->m_lPlayerNames[_iClient] = "";
	if(pCD->m_lPlayerScores.Len() > _iClient)
	{
		pCD->m_lPlayerScores[_iClient] = 0;
		pCD->m_lPlayerDeaths[_iClient] = 0;
	}
	pCD->m_lPlayerScores.MakeDirty();
	
	return CWObject_Game::OnRemoveClient(_iClient, _Reason);
}

CMat4Dfp32 CWObject_GameDM::GetSpawnPosition(int _Team)
{
	CMat4Dfp32 Pos;
	Pos.Unit();

	TSelection<CSelection::LARGE_BUFFER> Selection;
	const int16* pSel;
	int nSel = 0;

	m_pWServer->Selection_AddClass(Selection, "info_player_start");
	nSel = m_pWServer->Selection_Get(Selection, &pSel);

	if(nSel > 0)
	{
		TArray<int16>	Indices;

		CWObject *pFound = NULL;
		for(int i = 0; i < nSel; i++)
		{
			CWObject *pObj = m_pWServer->Object_Get(pSel[i]);
			int team = pObj->m_Data[0];
			if(_Team == -1)
				Indices.Add(pSel[i]);
			else if(_Team == team)
				Indices.Add(pSel[i]);
		}

		if(!Indices.Len())
		{
			Pos = m_pWServer->Object_GetPositionMatrix(pSel[MRTC_RAND() % nSel]);
			ConOutL(CStrF("§cf80WARNING: No player start positions on map with team %i", _Team));
			return Pos;
		}

		pFound = m_pWServer->Object_Get(Indices[MRTC_RAND() % Indices.Len()]);

		Pos = pFound->GetPositionMatrix();
	}
	else
	{
		Pos.Unit();
		ConOutL("§cf80WARNING: No player start positions on map.");
	}

	return Pos;
}

void CWObject_GameDM::EndGame()
{
	CClientData *pCD = GetClientData();
	pCD->m_GameOver = true;
	m_GameOverTick = m_pWServer->GetGameTick();
	int HighestScore = -1;
	int nNum = 0;
	for(uint i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerDM* pPlayer = Player_GetDM(i);
		if(pPlayer)
		{
			if(pPlayer->m_Score == HighestScore)
				nNum++;
			if(pPlayer->m_Score > HighestScore)
			{
				HighestScore = pPlayer->m_Score;
				nNum++;
			}
			m_pWServer->Net_ClientCommand(i, "mp_setmultiplayerstatus(1)");
			
			if(pPlayer->m_iObject != -1)
			{
				CWObject *pObj = m_pWServer->Object_Get(pPlayer->m_iObject);
				CMat4Dfp32 Mat = pObj->GetPositionMatrix();
				Player_Respawn(i, Mat, "Spectator", 0, -1);
			}
		}
	}

	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

	for(uint i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerDM* pPlayer = Player_GetDM(i);
		if(pPlayer)
		{
			if(HighestScore == pPlayer->m_Score)
			{
				int iSound = 0;
				if(nNum == 1)
					iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_128");
				else
					iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_125");
				if(iSound)
					m_pWServer->Sound_Global(iSound, 1.0f, i);
				
				CNetMsg Msg(WPACKET_OBJNETMSG);
				Msg.AddInt8(NETMSG_GAME_ACHIEVEMENT);
				Msg.AddInt16(m_iObject);
				Msg.AddInt16(m_iClass);
				Msg.AddInt8(ACHIEVEMENT_FIRST_MP_VICTORY);
				m_pWServer->NetMsg_SendToClient(Msg, pPlayer->m_iPlayer);
			}
			else
			{
				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_129");
				if(iSound)
					m_pWServer->Sound_Global(iSound, 1.0f, i);
			}
		}
	}
}

static void DrawPlayerListBackGround(CRC_Util2D *_pUtil2D, CRct _rect)
{
	CClipRect Clip(0, 0, 640, 480);
//	_pUtil2D->Rect(Clip, _rect, SCOREBOARD_BACKGROUND);
	CPnt p0[4], p1[4];
	p0[0] = _rect.p0;
	p1[0].x = _rect.p1.x;
	p1[0].y = _rect.p0.y;

	p0[1] = p1[0];
	p1[1] = _rect.p1;

	p0[2] = p1[1];
	p1[2].x = _rect.p0.x;
	p1[2].y = _rect.p1.y;

	p0[3] = p1[2];
	p1[3] = _rect.p0;

	_pUtil2D->Line(Clip, p0[0], p1[0], SCOREBOARD_BORDER);
	_pUtil2D->Line(Clip, p0[1], p1[1], SCOREBOARD_BORDER);
	_pUtil2D->Line(Clip, p0[2], p1[2], SCOREBOARD_BORDER);
	_pUtil2D->Line(Clip, p0[3], p1[3], SCOREBOARD_BORDER);
};

void CWObject_GameDM::OnClientRenderMultiplayerStatus(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D)
{
	CClientData *pCD = GetClientData(_pObj);

	if(pCD->m_lPlayerNames.Len() == 0)
		return;

	TThinArray<SPlayerInfo> PlayerInfo;
	PlayerInfo.SetLen(pCD->m_lPlayerNames.Len());
	memset(PlayerInfo.GetBasePtr(), 0, PlayerInfo.ListSize());

	uint8 NumPlayerInfo = 0;
	for(int i = 0; i < pCD->m_lPlayerNames.Len(); i++)
	{
		if(pCD->m_lPlayerNames[i] != "")
		{	
			PlayerInfo[NumPlayerInfo].m_Name = pCD->m_lPlayerNames[i];
			if(pCD->m_lPlayerScores.Len() > i)
				PlayerInfo[NumPlayerInfo].m_SortKey = pCD->m_lPlayerScores[i] + 32768;
			else
				PlayerInfo[NumPlayerInfo].m_SortKey = 32768;
			if(pCD->m_lPlayerDeaths.Len() > i)
				PlayerInfo[NumPlayerInfo].m_Deaths = pCD->m_lPlayerDeaths[i];
			else
				PlayerInfo[NumPlayerInfo].m_Deaths = 0;
			NumPlayerInfo++;
		}
	}

	RadixSort(PlayerInfo.GetBasePtr(), NumPlayerInfo);

	CClipRect Clip(0, 0, 640, 480);
	_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
	if(!pFont)
		return;

	CRct rect;
	rect.p0.x = 100;
	rect.p0.y = 90;
	rect.p1.x = 500;
	rect.p1.y = 300;
	_pUtil2D->Rect(Clip, rect, SCOREBOARD_BACKGROUND);

	_pUtil2D->Text_DrawFormatted(Clip, pFont, pCD->m_GameModeName.m_Value, -20, 100, WSTYLE_TEXT_SHADOW | WSTYLE_TEXT_CENTER, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_TIME_LEFT"), 250, 125, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_NAME"), 105, 140, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_SCORE"), 305, 140, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_DEATHS"), 375, 140, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_PING"), 445, 140, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

	DrawPlayerListBackGround(_pUtil2D, CRct(105, 155, 495, 295));
		
	CStr St;
	int8 minuter = (int)(pCD->m_MaxTime - ((_pWClient->GetGameTick() - pCD->m_StartTick) / _pWClient->GetGameTicksPerSecond())) / 60;
	if(minuter < 0)
		minuter = 0;
	int8 sekunder = (int)(pCD->m_MaxTime - ((_pWClient->GetGameTick() - pCD->m_StartTick) / _pWClient->GetGameTicksPerSecond())) % 60;
	if(sekunder < 0)
		sekunder = 0;
	if(sekunder < 10)
		St = CStrF("§Z10%i:0%i", minuter, sekunder);
	else
		St = CStrF("§Z10%i:%i", minuter, sekunder);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 320, 125, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

	int row = 0;

	for(int8 i = NumPlayerInfo - 1; i > -1; i--)
	{
		CStr Name = PlayerInfo[i].m_Name;
		int Score = PlayerInfo[i].m_SortKey - 32768;
		int deaths = PlayerInfo[i].m_Deaths;
		int ping = 0;

		if(Name.IsUnicode())
			St = CStrF(WTEXT("§Z10 %s"), Name.StrW());
		else
			St = CStrF("§Z10 %s", Name.Str());

		_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 110, 160 + row * 10, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

		if(Score < 10)
			St = CStrF("§Z10   %i", Score);
		else if(Score < 100)
			St = CStrF("§Z10  %i", Score);
		else
			St = CStrF("§Z10 %i", Score);
		_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 305, 160 + row * 10, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

		if(deaths < 10)
			St = CStrF("§Z10   %i", deaths);
		else if(deaths < 100)
			St = CStrF("§Z10  %i", deaths);
		else
			St = CStrF("§Z10 %i", deaths);
		_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 375, 160 + row * 10, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

		if(ping < 10)
			St = CStrF("§Z10   %i", ping);
		else if(ping < 100)
			St = CStrF("§Z10  %i", ping);
		else
			St = CStrF("§Z10 %i", ping);
		_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 445, 160 + row * 10, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

		row++;
	}
}

CStr CWObject_GameDM::GetStatusBarText(int _iText, CClientData* _pCD)
{
	CStr St;
	switch(_pCD->m_lMultiplayerMessages[_iText].m_Type) 
	{
	case NETMSG_GAME_PLAYERKILL:
		{
			if(_pCD->m_lMultiplayerMessages[_iText].m_Str1.IsUnicode())
				St = CStrF(WTEXT("§Z10 %s §LMP_WAS_KILLED_BY§pq %s"), _pCD->m_lMultiplayerMessages[_iText].m_Str1.StrW(), _pCD->m_lMultiplayerMessages[_iText].m_Str2.StrW());
			else
				St = CStrF("§Z10 %s §LMP_WAS_KILLED_BY§pq %s", _pCD->m_lMultiplayerMessages[_iText].m_Str1.Str(), _pCD->m_lMultiplayerMessages[_iText].m_Str2.Str());
		}
		break;
	case NETMSG_GAME_PLAYERSUICIDE:
		{
			if(_pCD->m_lMultiplayerMessages[_iText].m_Str1.IsUnicode())
				St = CStrF(WTEXT("§Z10 %s §LMP_SUICIDE"), _pCD->m_lMultiplayerMessages[_iText].m_Str1.StrW());
			else
				St = CStrF("§Z10 %s §LMP_SUICIDE", _pCD->m_lMultiplayerMessages[_iText].m_Str1.Str());
		}
		break;
	case NETMSG_GAME_PLAYER_DISCONNECT:
		{
			if(_pCD->m_lMultiplayerMessages[_iText].m_Str1.IsUnicode())
				St = CStrF(WTEXT("§Z10 %s %s"), _pCD->m_lMultiplayerMessages[_iText].m_Str1.StrW(), _pCD->m_lMultiplayerMessages[_iText].m_Str2.StrW());
			else
				St = CStrF("§Z10 %s %s", _pCD->m_lMultiplayerMessages[_iText].m_Str1.Str(), _pCD->m_lMultiplayerMessages[_iText].m_Str2.Str());
		}
		break;
	case NETMSG_GAME_TEAMKILL:
		{
			if(_pCD->m_lMultiplayerMessages[_iText].m_Str1.IsUnicode())
				St = CStrF(WTEXT("§Z10 %s §LMP_WAS_TEAMKILLED_BY§pq %s"), _pCD->m_lMultiplayerMessages[_iText].m_Str1.StrW(), _pCD->m_lMultiplayerMessages[_iText].m_Str2.StrW());
			else
				St = CStrF("§Z10 %s §LMP_WAS_TEAMKILLED_BY§pq %s", _pCD->m_lMultiplayerMessages[_iText].m_Str1.Str(), _pCD->m_lMultiplayerMessages[_iText].m_Str2.Str());
		}
		break;
	}
	return St;
}

void CWObject_GameDM::GetStatusBarTextImportant(int _iText, CClientData* _pCD, CStr &_Title, CStr &_Text)
{
	switch(_pCD->m_lMultiplayerMessagesImportant[_iText].m_Type) 
	{	
	case NETMSG_GAME_JOIN:
		_Title = "§LGAMEMSG_INFO";
		if(_pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.IsUnicode())
			_Text = CStrF(WTEXT("%s §LMP_HAS_JOINED_GAME"), _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.StrW());
		else
			_Text = CStrF("%s §LMP_HAS_JOINED_GAME", _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.Str());
		break;
	case NETMSG_GAME_JOIN_TEAM:
		_Title = "§LGAMEMSG_INFO";
		if(_pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.IsUnicode())
			_Text = CStrF(WTEXT("%s §LMP_HAS_JOINED_TEAM"), _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.StrW());
		else
			_Text = CStrF("%s §LMP_HAS_JOINED_TEAM", _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.Str());
		break;
	case NETMSG_GAME_FLAGSTOLEN:
		_Title = "§LMP_FLAG_STOLEN_TITLE";
		if(_pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.IsUnicode())
			_Text = CStrF(WTEXT("§LMP_FLAG_STOLEN_TEXT§pq %s"), _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.StrW());
		else
			_Text = CStrF("§LMP_FLAG_STOLEN_TEXT§pq %s", _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.Str());
		break;
	case NETMSG_GAME_FLAGRETURNED:
		_Title = "§LMP_FLAG_RETURNED_TITLE";
		if(_pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.IsUnicode())
			_Text = CStrF(WTEXT("§LMP_FLAG_RETURNED_TEXT§pq %s"), _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.StrW());
		else
			_Text = CStrF("§LMP_FLAG_RETURNED_TEXT§pq %s", _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.Str());
		break;
	case NETMSG_GAME_FLAGCAPTURED:
		_Title = "§LMP_FLAG_CAPTURED_TITLE";
		if(_pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.IsUnicode())
			_Text = CStrF(WTEXT("§LMP_FLAG_CAPTURED_TEXT§pq %s"), _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.StrW());
		else
			_Text = CStrF("§LMP_FLAG_CAPTURED_TEXT§pq %s", _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.Str());
		break;					
	case NETMSG_GAME_ENEMY_FLAGSTOLEN:
		_Title = "§LMP_FLAG_ENEMY_STOLEN_TITLE";
		if(_pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.IsUnicode())
			_Text = CStrF(WTEXT("§LMP_FLAG_ENEMY_STOLEN_TEXT§pq %s"), _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.StrW());
		else
			_Text = CStrF("§LMP_FLAG_ENEMY_STOLEN_TEXT§pq %s", _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.Str());
		break;
	case NETMSG_GAME_ENEMY_FLAGRETURNED:
		_Title = "§LMP_FLAG_ENEMY_RETURNED_TITLE";
		if(_pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.IsUnicode())
			_Text = CStrF(WTEXT("§LMP_FLAG_ENEMY_RETURNED_TEXT§pq %s"), _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.StrW());
		else
			_Text = CStrF("§LMP_FLAG_ENEMY_RETURNED_TEXT§pq %s", _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.Str());
		break;
	case NETMSG_GAME_ENEMY_FLAGCAPTURED:
		_Title = "§LMP_FLAG_ENEMY_CAPTURED_TITLE";
		if(_pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.IsUnicode())
			_Text = CStrF(WTEXT("§LMP_FLAG_ENEMY_CAPTURED_TEXT§pq %s"), _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.StrW());
		else
			_Text = CStrF("§LMP_FLAG_ENEMY_CAPTURED_TEXT§pq %s", _pCD->m_lMultiplayerMessagesImportant[_iText].m_Str1.Str());
	}
}

void CWObject_GameDM::OnClientRenderStatusBar(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D, CVec2Dfp32 *_rect)
{
	CClientData *pCD = GetClientData(_pObj);

	if(_rect)
		_pUtil2D->SetCoordinateScale(*_rect);

	if(pCD)
	{
		int row = 0;
		CClipRect Clip(0, 0, 640, 480);
		_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

		CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
		if(!pFont)
			return;

		for(uint8 i = 0; i < pCD->m_lMultiplayerMessages.Len(); i++)
		{
			CStr St = GetStatusBarText(i, pCD);
#ifdef PLATFORM_CONSOLE
			_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 50, 80 + row * 10, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 400, 12, false);
#else
			_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 10, 10 + row * 10, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 400, 12, false);
#endif
			row++;
		}

		for(uint8 i = 0; i < pCD->m_lMultiplayerMessages.Len(); i++)
		{
			if(pCD->m_lMultiplayerMessages[i].m_StartTick + pCD->m_lMultiplayerMessages[i].m_Duration <= _pWClient->GetGameTick())
			{
				pCD->m_lMultiplayerMessages.Del(i);
				break;
			}
		}

		if(pCD->m_WarmUpMode == MP_WARMUP_MODE_PREGAME)
		{
			if(pCD->m_WarmUpCountDownTick == -1)
				_pUtil2D->Text_DrawFormatted(Clip, pFont, "§Z32 §LMP_WARMUP", 110, 200, WSTYLE_TEXT_SHADOW | WSTYLE_TEXT_CENTER, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 400, 32, false);
			else
			{
				int Secs = TruncToInt((WARMUPCOUNTDOWN + 1.0f) - ((_pWClient->GetGameTick() - pCD->m_WarmUpCountDownTick) * _pWClient->GetGameTickTime()));
				if(Secs)
					_pUtil2D->Text_DrawFormatted(Clip, pFont, CStrF("§Z32 %i", Secs), 110, 200, WSTYLE_TEXT_SHADOW | WSTYLE_TEXT_CENTER, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 400, 32, false);
			}
		}
		else if(pCD->m_WarmUpMode == MP_WARMUP_MODE_BETWEEN_ROUNDS)
		{
			CStr Text;
			if(pCD->m_LastRoundWinner.m_Value.IsUnicode())
				Text = CStrF(WTEXT("§Z32 §LMP_LAST_ROUND_WINNER %s"), pCD->m_LastRoundWinner.m_Value.StrW());
			else
				Text = CStrF("§Z32 §LMP_LAST_ROUND_WINNER %s", pCD->m_LastRoundWinner.m_Value.Str());
			_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, 110, 200, WSTYLE_TEXT_SHADOW | WSTYLE_TEXT_CENTER, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 400, 32, false);
			int Secs = TruncToInt((WARMUPCOUNTDOWN + 1.0f) - ((_pWClient->GetGameTick() - pCD->m_WarmUpCountDownTick) * _pWClient->GetGameTickTime()));
			if(Secs)
				_pUtil2D->Text_DrawFormatted(Clip, pFont, CStrF("§Z32 %i", Secs), 110, 200, WSTYLE_TEXT_SHADOW | WSTYLE_TEXT_CENTER, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 400, 32, false);
		}

		int Tick = _pWClient->GetGameTick();
		for(uint8 i = 0; i < pCD->m_lMultiplayerMessagesImportant.Len(); i++)
		{
			const fp32 EnterTicks = 0.3f * _pWClient->GetGameTicksPerSecond();
			int MsgDuration = pCD->m_lMultiplayerMessagesImportant[i].m_Duration;
			if(MsgDuration == 0)
				MsgDuration = int(EnterTicks * 2 + 5);
			int StopTick = pCD->m_lMultiplayerMessagesImportant[i].m_StartTick + MsgDuration;
			if(StopTick != 0 && StopTick > Tick)
			{
				CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
				CRC_Font *pTitleFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("HEADINGS"));
				if(pFont && pTitleFont)
				{
					fp32 Duration = Tick + _pWClient->GetRenderTickFrac() - pCD->m_lMultiplayerMessagesImportant[i].m_StartTick;

					CStr Text, Title;
					GetStatusBarTextImportant(i, pCD, Title, Text);

					int iSurfRes = 0;
					int SurfW = 0;
					int SurfH = 0;
					if(Text.CompareSubStr("$") == 0)
					{
						iSurfRes = Text.GetStrSep(",").Copy(1, 1024).Val_int();
						SurfW = Text.GetStrSep(",").Val_int();
						SurfH = Text.GetStrSep(",").Val_int();
					}
					Text = WTEXT("§Z18") + Text;

					wchar Buffer[1024];
					Localize_Str(Text.StrW(), Buffer, 1024);

					if(Buffer[4] == '#')
					{
						CStr St = Buffer + 5;
						while(St != "")
						{
							int X = St.GetStrSep(",").Val_int();
							int Y = St.GetStrSep(",").Val_int();
							//pFont->GetOriginalSize()

							int Alpha;
							if(Duration < EnterTicks)
								Alpha = int(255 * Duration / EnterTicks);
							else if(Duration < MsgDuration - EnterTicks)
								Alpha = 255;
							else
								Alpha = int(255 * (MsgDuration - Duration) / EnterTicks);
							if(St[0] == '$')
							{
								St = St.Copy(1, 1024);
								iSurfRes = _pWClient->GetMapData()->GetResourceIndex_Surface(St.GetStrSep(",").Ansi());
								int iSurf = _pWClient->GetMapData()->GetResource_SurfaceID(iSurfRes);
								_pUtil2D->SetSurface(iSurf, CMTime::CreateFromSeconds(Duration));
								int W = St.GetStrSep(",").Val_int();
								int H = St.GetStrSep(",").Val_int();

								if(W > 10000)
								{
									_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / 640, fp32(_pUtil2D->GetTextureHeight()) / 480);
									W -= 10000;
								}
								else
								{
									_pUtil2D->SetTextureOrigo(Clip, CPnt(X, Y));
									_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / W, fp32(_pUtil2D->GetTextureHeight()) / H);
								}
								_pUtil2D->Rect(Clip, CRct(X, Y, X + W, Y + H), 0x00ffffff | (Alpha << 24));
							}
							else
							{
								CRC_Font *pFont2 = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("PALATINO"));
								if(pFont2)
									pFont = pFont2;
								CStr St2 = "§Z24" + St;
								_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
								_pUtil2D->Text_DrawFormatted(Clip, pFont, St2, X, Y, WSTYLE_TEXT_WORDWRAP | WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640 - X, 480 - Y, false);
							}
						}
					}
					else
					{
						SurfW = int((SurfW / _pUtil2D->GetRC()->GetDC()->GetPixelAspect()));

						wchar BufTitle[1024];
						Localize_Str(Title.Str(), BufTitle, 1024);
						CRct Rct = CalcInfoRectWidth(_pUtil2D, BufTitle, pTitleFont, Buffer, pFont, iSurfRes, SurfW, SurfH);

						int XPos;
						int XBasePos = 640 / 2 - Rct.GetWidth() / 2;
						if(Duration < EnterTicks)
							XPos = 640 - RoundToInt((640 - XBasePos) * (1.0f - Sqr(1.0f - Duration / EnterTicks)));
						else if(Duration < MsgDuration - EnterTicks)
							XPos = XBasePos;
						else
							XPos = XBasePos - RoundToInt((Rct.GetWidth() + XBasePos) * Sqr(1.0f - ((MsgDuration - Duration) / EnterTicks)));
						int YPos = 150;

						Rct.p0.x += XPos;
						Rct.p1.x += XPos;
						Rct.p0.y += YPos;
						Rct.p1.y += YPos;

						DrawInfoRect(_pUtil2D, _pWClient, Rct, BufTitle, pTitleFont, Buffer, pFont, iSurfRes, SurfW, SurfH);
					}
				}
			}
		}
		
		for(uint8 i = 0; i < pCD->m_lMultiplayerMessagesImportant.Len(); i++)
		{
			if(pCD->m_lMultiplayerMessagesImportant[i].m_StartTick + pCD->m_lMultiplayerMessagesImportant[i].m_Duration <= _pWClient->GetGameTick())
			{
				pCD->m_lMultiplayerMessagesImportant.Del(i);
				break;
			}
		}
	}

	return parent::OnClientRenderStatusBar(_pObj, _pWClient, _pEngine, _pUtil2D, _rect);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_GameTDM
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_GameTDM::CWObject_GameTDM()
{
	M_TRACE("CWObject_GameTDM::CWObject_GameTDM\n");

	CClientData *pCD = GetClientData();
#if defined(PLATFORM_CONSOLE)
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
	if(pGameMod)
	{
		pCD->m_MaxTime = pGameMod->m_pMPHandler->m_TimeLimit;
		pCD->m_MaxScore = pGameMod->m_pMPHandler->m_ScoreLimit;
		pCD->m_GameModeName.m_Value = pGameMod->m_pMPHandler->GetGameModeName();
	}
#else
	pCD->m_MaxScore = 100;
	pCD->m_MaxTime = 600;
	pCD->m_GameModeName.m_Value = "§Z20 §LMENU_SHAPESHIFTER - §LMENU_TDM";
#endif
	pCD->m_GameModeName.MakeDirty();

	m_bDoOnce = true;
	m_nNumPlayersNeeded = 2;
	pCD->m_WarmUpMode = MP_WARMUP_MODE_PREGAME;
	pCD->m_WarmUpCountDownTick = -1;
}

CWObject_GameTDM::CWO_PlayerTDM *CWObject_GameTDM::Player_GetTDM(int _iPlayer)
{
	return (CWO_PlayerTDM *)(CWO_Player *)m_lspPlayers[_iPlayer];
}

void CWObject_GameTDM::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	parent::OnIncludeClass(_pWData, _pWServer);

	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_126");	//Your team wins
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_127");	//The Enemy team wins
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_134");	//Humans wins!
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_135");	//Darklings wins!
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_140");	//Your team has taken the lead
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_142");	//Your team has lost the lead
}

int CWObject_GameTDM::OnClientConnected(int _iClient)
{
	if (!CWObject_GameP4::OnClientConnected(_iClient))
		return 0;

	CMat4Dfp32 Mat;
	int16 Flags = 0;
	CWServer_Mod* pServer = safe_cast<CWServer_Mod>(m_pWServer);
	M_ASSERT(pServer, "No server!");

	Mat = GetSpawnPosition();

	int PlayerObj = -1;
	int PlayerGUID = -1;
	CFStr Class;
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys->GetEnvironment()->GetValuei("FORCESPECTATOR"))
	{
		Class = "Spectator";
	}
	else
	{
		Class = "Spectator";

		//Open window to select team
		OnSetClientWindow(_iClient, "multiplayer_selectteam");
	}

	int iObj = Player_Respawn(Player_GetWithClient(_iClient)->m_iPlayer, Mat, Class, Flags, PlayerObj);
	if (iObj == -1)
	{
		ConOutL(CStrF("ERROR: Failed to spawn character %s", Class.Str()));
		return 0;
	}

	if (PlayerGUID != -1)
		pServer->Object_ChangeGUID(iObj, PlayerGUID);

	M_TRACE("CWObject_GameTDM::OnClientConnected");

	CClientData *pCD = GetClientData();
	bool bFoundEmptyPlayer = false;
	CWO_PlayerDM *pPlayerClient = Player_GetDM(_iClient);
	for(int i = 0; i < pCD->m_lPlayerNames.Len(); i++)
	{	
		CWO_PlayerDM *pPlayer = Player_GetDM(i);
		if(pPlayer)
		{
			CNetMsg Msg2;
			Msg2.AddInt8(i);
			Msg2.AddStrAny(pPlayer->m_Name);
			CNetMsg Msg(WPACKET_OBJNETMSG);
			Msg.AddInt8(NETMSG_GAME_SETNAME);
			Msg.AddInt16(m_iObject);
			Msg.AddInt16(m_iClass);
			if (Msg.AddData((void*)Msg2.m_Data, Msg2.m_MsgSize))
				m_pWServer->Net_PutMsgEx(pPlayerClient->m_iPlayer, Msg, -1, WCLIENT_STATE_INGAME);

			if(!pPlayer->m_Name.Len())
				bFoundEmptyPlayer = true;
		}
	}

	if(!bFoundEmptyPlayer)
	{
		pCD->m_lPlayerScores.Add(0);
		pCD->m_lPlayerDeaths.Add(0);
		pCD->m_lPlayerTeams.Add(-1);
		CStr EmptyName;
		pCD->m_lPlayerNames.Add(EmptyName);
	}

	pCD->m_lPlayerTeams[_iClient] = -1;
	pCD->m_lPlayerScores.MakeDirty();	//We must make it dirty so that the new player will get what we have
	pCD->m_lPlayerTeams.MakeDirty();	//Dirty it so new player gets it, this could be prettier
	pPlayerClient->m_Name = " ";

	while(pCD->m_lPlayerNames.Len() <= _iClient)
	{
		CStr EmptyName;
		pCD->m_lPlayerNames.Add(EmptyName);
	}

	while(pCD->m_lPlayerScores.Len() <= _iClient)
		pCD->m_lPlayerScores.Add(0);

	while(pCD->m_lPlayerDeaths.Len() <= _iClient)
		pCD->m_lPlayerDeaths.Add(0);

	while(pCD->m_lPlayerTeams.Len() <= _iClient)
		pCD->m_lPlayerTeams.Add(0);

	pCD->m_MaxScore.MakeDirty();
	pCD->m_liMusic.MakeDirty();

	OnPlayerEntersGame(iObj, 0);

	return 1;
}


aint CWObject_GameTDM::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAMEMOD_RENDERMULTIPLAYERSTATUS:
		OnClientRenderMultiplayerStatus(_pObj, _pWClient, (CXR_Engine *)_Msg.m_Param0, (CRC_Util2D *)_Msg.m_Param1);
		return 1;

	case OBJMSG_GAME_RENDERSTATUSBAR:
		OnClientRenderStatusBar(_pObj, _pWClient, (CXR_Engine *)_Msg.m_Param0, (CRC_Util2D *)_Msg.m_Param1, (CVec2Dfp32 *)_Msg.m_pData);
		return 1;
	}

	return parent::OnClientMessage(_pObj, _pWClient, _Msg);
}

void CWObject_GameTDM::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	switch(_Msg.m_MsgType)
	{
	case NETMSG_GAME_TEAMKILL:
		{
			int p = 0;

			CClientData *pCD = GetClientData(_pObj);
			CClientData::SMultiplayerMessage MMsg;
			MMsg.m_Type = NETMSG_GAME_TEAMKILL;
			MMsg.m_Str1 = _Msg.GetStrAny(p);
			MMsg.m_Str2 = _Msg.GetStrAny(p);
			MMsg.m_StartTick = _pWClient->GetGameTick();
			MMsg.m_Duration = RoundToInt(3.0f * _pWClient->GetGameTicksPerSecond());
			while(pCD->m_lMultiplayerMessages.Len() >= 4)
				pCD->m_lMultiplayerMessages.Del(0);
			pCD->m_lMultiplayerMessages.Add(MMsg);
		}
		return;

	case NETMSG_GAME_SETNAME:
		{
			parent::OnClientNetMsg(_pObj, _pWClient, _Msg);
			int p = 0;
			uint8 iPlayer = _Msg.GetInt8(p);
			CClientData *pCD = GetClientData(_pObj);
			while(iPlayer >= pCD->m_lPlayerTeams.Len())
				pCD->m_lPlayerTeams.Add(0);
		}
		return;
	}
	return parent::OnClientNetMsg(_pObj, _pWClient, _Msg);
}

int CWObject_GameTDM::OnClientCommand(int _iPlayer, const CCmd* _pCmd)
{
	int Size = _pCmd->m_Size;
	switch(_pCmd->m_Cmd)
	{
	case CONTROL_JOINTEAM:
		{
			char team;
			memcpy(&team, _pCmd->m_Data, 1);

			CWO_PlayerTDM *pPlayer = Player_GetTDM(_iPlayer);

			pPlayer->m_bReady = true;
			
			CMat4Dfp32 Mat;
			int16 Flags = 0;
			CWServer_Mod* pServer = safe_cast<CWServer_Mod>(m_pWServer);
			M_ASSERT(pServer, "No server!");

			if(team == '0')
				pPlayer->m_Team = 0;
			else
				pPlayer->m_Team = 1;

			Mat = GetSpawnPosition(pPlayer->m_Team);

#if defined(PLATFORM_CONSOLE)
			MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
			CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

			pGameMod->m_pMPHandler->SetTeam(pPlayer->m_player_id, pPlayer->m_Team);
#endif

			CClientData *pCD = GetClientData();

			pCD->m_lPlayerTeams[_iPlayer] = pPlayer->m_Team;
			pCD->m_lPlayerTeams.MakeDirty();

			CNetMsg Msg2;
			Msg2.m_MsgType = NETMSG_GAME_JOIN_TEAM;
			Msg2.AddStrAny(pPlayer->m_Name);
			m_pWServer->NetMsg_SendToObject(Msg2, m_iObject);

//			OnSetClientWindow(_iPlayer, "");
/*			if(m_GameMode == MP_MODE_SHAPESHIFTER)
				OnSetClientWindow(_iPlayer, "multiplayer_selectcharacter_shapeshifters");
			else if(m_GameMode == MP_MODE_DARKLINGS)
				OnSetClientWindow(_iPlayer, "multiplayer_selectcharacter_darklings");
			else
			{
				if(pPlayer->m_Team)
					OnSetClientWindow(_iPlayer, "multiplayer_selectcharacter_darklings");
				else
					OnSetClientWindow(_iPlayer, "multiplayer_selectcharacter_shapeshifters");
			}*/

			return true;
		}

	case CONTROL_JOINGAME:
		{
			char Class[256];
			memcpy(Class, _pCmd->m_Data, Size);
			Class[Size] = 0;

			CWO_PlayerTDM *pPlayer = Player_GetTDM(_iPlayer);
			pPlayer->m_ClassHuman = Class;
			ValidateHumanModelForTeam(_iPlayer);
			pPlayer->m_bJoined = true;

			int State = m_pWServer->GetMapData()->GetState();
			m_pWServer->GetMapData()->SetState(State & ~WMAPDATA_STATE_NOCREATE);	
			CMat4Dfp32 Mat = GetSpawnPosition(pPlayer->m_Team);
			int16 Flags = 0;
			Player_Respawn(_iPlayer, Mat, pPlayer->m_ClassHuman, Flags);

			CWObject *pObj = m_pWServer->Object_Get(pPlayer->m_iObject);
			CWObject_CharShapeshifter *pChar = safe_cast<CWObject_CharShapeshifter>(pObj);
			CWO_CharShapeshifter_ClientData &CD = pChar->GetClientData();
			CD.m_iDarklingModel = m_pWServer->GetMapData()->GetResourceIndex_Model(pPlayer->m_ModelDarkling.Str());
			m_pWServer->GetMapData()->SetState(State);
			if(m_GameMode == MP_MODE_DARKLINGS || (m_GameMode == MP_MODE_DARKLINGS_VS_HUMANS && pPlayer->m_Team))
			{
				CWObject_Message Msg;
				Msg.m_Msg = OBJMSG_GAME_TOGGLEDARKLINGHUMAN;
				Msg.m_iSender = pPlayer->m_iPlayer;
				Msg.m_Param0 = 1;
				m_pWServer->Message_SendToObject(Msg, m_iObject);
			}
			else
				pChar->RemoveAllDarklingsWeapons();

			OnSetClientWindow(pPlayer->m_iClient, "");

			CNetMsg Msg2;
			Msg2.m_MsgType = NETMSG_GAME_JOIN;
			Msg2.AddStrAny(pPlayer->m_Name);
			m_pWServer->NetMsg_SendToObject(Msg2, m_iObject);

			return true;
		}

	case CONTROL_SETDARKLINGNAME:
		{
			char Darkling[256];
			memcpy(Darkling, _pCmd->m_Data, Size);
			Darkling[Size] = 0;

			if(_iPlayer != -1 && _iPlayer < Player_GetNum())
			{
				CWO_PlayerDM *pPlayer = Player_GetDM(_iPlayer);
				pPlayer->m_ModelDarkling = CStrF("Characters\\Darklings\\%s", Darkling);	
				ValidateDarklingModelforTeam(_iPlayer);
			}
			return true;
		}
	}
	return parent::OnClientCommand(_iPlayer, _pCmd);
}

void CWObject_GameTDM::ValidateHumanModelForTeam(int _iPlayer)
{
	CWO_PlayerTDM *pPlayer = Player_GetTDM(_iPlayer);
	//TEMP
	if(pPlayer->m_Team)
		pPlayer->m_ClassHuman = CStrF("multiplayer_Swat_0%i", (MRTC_RAND() % 2) + 1);
	else
		pPlayer->m_ClassHuman = CStrF("multiplayer_Mob_0%i", (MRTC_RAND() % 3) + 1);
}

void CWObject_GameTDM::ValidateDarklingModelforTeam(int _iPlayer)
{
	CWO_PlayerTDM *pPlayer = Player_GetTDM(_iPlayer);
	//TEMP
	if(pPlayer->m_Team)
		pPlayer->m_ModelDarkling = "Characters\\Darklings\\Darkling_06";
	else
		pPlayer->m_ModelDarkling = "Characters\\Darklings\\Darkling_Lightkiller";
}

void CWObject_GameTDM::RespawnAll(bool _bAlreadySpawned)
{
	for(int i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerTDM *pPlayer = Player_GetTDM(i);
		if(pPlayer && !_bAlreadySpawned)
		{	
			CMat4Dfp32 Mat = GetSpawnPosition(pPlayer->m_Team);
			int16 Flags = 0;
			Player_Respawn(i, Mat, pPlayer->m_ClassHuman, Flags);
			if(m_GameMode == MP_MODE_DARKLINGS || (m_GameMode == MP_MODE_DARKLINGS_VS_HUMANS && pPlayer->m_Team))
			{
				CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(pPlayer->m_iObject));
				CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pPlayerObj);

				CWObject_Message Msg;
				Msg.m_Msg = OBJMSG_GAME_TOGGLEDARKLINGHUMAN;
				Msg.m_iSender = pPlayer->m_iPlayer;
				Msg.m_Param0 = 1;
				m_pWServer->Message_SendToObject(Msg, m_iObject);
			}
		}
	}
	parent::RespawnAll(true);
}

void CWObject_GameTDM::GivePoint(int _iPlayer, int _nPoints)
{
	CClientData *pCD = GetClientData();
	CWO_PlayerTDM* pPlayer = Player_GetTDM(_iPlayer);

	int ScoreTeam1 = 0;
	int ScoreTeam2 = 0;
	int ScoreTeam1New = 0;
	int ScoreTeam2New = 0;
	for(int i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerTDM* pPlayer = Player_GetTDM(i);
		if(pPlayer)
		{
			if(pPlayer->m_Team)
				ScoreTeam2 += pPlayer->m_Score;
			else
				ScoreTeam1 += pPlayer->m_Score;
		}
	}
	
	pPlayer->m_Score += _nPoints;
	
	for(int i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerTDM* pPlayer = Player_GetTDM(i);
		if(pPlayer)
		{
			if(pPlayer->m_Team)
				ScoreTeam2New += pPlayer->m_Score;
			else
				ScoreTeam1New += pPlayer->m_Score;
		}
	}

	if(ScoreTeam1New >= pCD->m_MaxScore || ScoreTeam2New >= pCD->m_MaxScore)
	{
		EndGame();
		return;
	}

	bool bTeam1WasLeading = ScoreTeam1 > ScoreTeam2;
	bool bTeam2WasLeading = ScoreTeam1 < ScoreTeam2;
	bool bTeam1IsLeading = ScoreTeam1New > ScoreTeam2New;
	bool bTeam2IsLeading = ScoreTeam1New < ScoreTeam2New;

	if(!bTeam1WasLeading && bTeam1IsLeading)
	{
		for(int i = 0; i < Player_GetNum(); i++)
		{
			CWO_PlayerTDM* pPlayer = Player_GetTDM(i);
			if(pPlayer)
			{
				if(!pPlayer->m_Team)
				{
					int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_140");
					if(iSound)
						m_pWServer->Sound_Global(iSound);
				}
				else
				{
					int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_142");
					if(iSound)
						m_pWServer->Sound_Global(iSound);
				}
			}
		}
	}
	else if(!bTeam2WasLeading && bTeam2IsLeading)
	{
		for(int i = 0; i < Player_GetNum(); i++)
		{
			CWO_PlayerTDM* pPlayer = Player_GetTDM(i);
			if(pPlayer)
			{
				if(pPlayer->m_Team)
				{
					int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_140");
					if(iSound)
						m_pWServer->Sound_Global(iSound);
				}
				else
				{
					int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_142");
					if(iSound)
						m_pWServer->Sound_Global(iSound);
				}
			}
		}
	}

	pCD->m_lPlayerScores[_iPlayer] = pPlayer->m_Score;
}

int CWObject_GameTDM::OnCharacterKilled(int _iObject, int _iSender)
{
	M_TRACE("CWObject_GameTDM::OnCharacterKilled, iObject=%d, iSender=%d\n", _iObject, _iSender);

	CClientData *pCD = GetClientData();

	uint nPlayers = Player_GetNum();
	uint other_player = ~0;
	for(uint j = 0; j < nPlayers; j++)
	{
		CWO_Player* pPlayer = Player_Get(j);
		if (pPlayer && pPlayer->m_iObject == _iSender)
		{
			other_player = j;
			break;
		}
	}

	for (uint i = 0; i < nPlayers; i++)
	{
		CWO_Player* pPlayer = Player_Get(i);
		if (pPlayer && pPlayer->m_iObject == _iObject)
		{
			CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(_iObject));
			CWO_CharShapeshifter_ClientData& CD = CWObject_CharShapeshifter::GetClientData(pPlayerObj);

			if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_DAMAGE)
			{
				CWObject_PickupInfo PickupInfo;
				PickupInfo.m_TemplateName = "object_mp_damage_boost";
				PickupInfo.m_Pos = pPlayerObj->GetPosition();
				PickupInfo.m_SpawnTick = m_pWServer->GetGameTick();
				PickupInfo.m_TicksLeft = CD.m_DamageBoostDuration - (m_pWServer->GetGameTick() - CD.m_DamageBoostStartTick);
				m_lPickups.Add(PickupInfo);

				int iDrainSoundInOut = m_pWServer->GetMapData()->GetResourceIndex_Sound("gam_drk_crp02");
				int iDrainLoopSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_Drk_crp03");

				if(iDrainSoundInOut)
					m_pWServer->Sound_At(pPlayerObj->GetPosition(), iDrainSoundInOut, WCLIENT_ATTENUATION_3D);

				if(iDrainLoopSound)
					m_pWServer->Sound_Off(pPlayerObj->m_iObject, iDrainLoopSound);

				CWObject_Message MsgDrain(OBJMSG_EFFECTSYSTEM_SETFADE, 2);
				m_pWServer->Message_SendToObject(MsgDrain, pPlayerObj->m_iDarkness_Drain_2);
			}
			if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_SPEED)
			{
				CWObject_PickupInfo PickupInfo;
				PickupInfo.m_TemplateName = "object_mp_speed_boost";
				PickupInfo.m_Pos = pPlayerObj->GetPosition();
				PickupInfo.m_SpawnTick = m_pWServer->GetGameTick();
				PickupInfo.m_TicksLeft = CD.m_SpeedBoostDuration - (m_pWServer->GetGameTick() - CD.m_SpeedBoostStartTick);
				m_lPickups.Add(PickupInfo);
			}
			if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_INVISIBILITY)
			{
				CWObject_PickupInfo PickupInfo;
				PickupInfo.m_TemplateName = "object_mp_invisibility";
				PickupInfo.m_Pos = pPlayerObj->GetPosition();
				PickupInfo.m_SpawnTick = m_pWServer->GetGameTick();
				PickupInfo.m_TicksLeft = CD.m_InvisibiltyBoostDuration - (m_pWServer->GetGameTick() - CD.m_InvisibiltyBoostStartTick);
				m_lPickups.Add(PickupInfo);

				pPlayerObj->ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
			}

			pPlayer->m_Mode = PLAYERMODE_DEAD;
			m_pWServer->Net_ClientCommand(i, "mp_setmultiplayerstatus(1)");

			CWO_PlayerTDM *pPlayerKiller, *pPlayerKilled;
			pPlayerKiller = NULL;
			pPlayerKilled = NULL;
				
			CNetMsg Msg2;
			pPlayerKilled = Player_GetTDM(i);
			pPlayerKilled->m_DeathTick = m_pWServer->GetGameTick();
			pPlayerKilled->m_KillsInARow = 0;
			if(!pCD->m_WarmUpMode)
			{
				if(other_player != ~0)
				{
					pPlayerKiller = Player_GetTDM(other_player);

					if(pPlayerKiller->m_Team == pPlayerKilled->m_Team)
					{
						GivePoint(pPlayerKiller->m_iPlayer, -2);
						Msg2.m_MsgType = NETMSG_GAME_TEAMKILL;
					}
					else
					{
						GivePoint(pPlayerKiller->m_iPlayer, 1);
						Msg2.m_MsgType = NETMSG_GAME_PLAYERKILL;
						AddKill(other_player);
					}
				}
				else
				{	//suicide
					GivePoint(pPlayerKilled->m_iPlayer, -2);
					Msg2.m_MsgType = NETMSG_GAME_PLAYERSUICIDE;
				}

				GetClientData()->m_lPlayerScores.MakeDirty();
				pPlayerKilled->m_Deaths++;
				GetClientData()->m_lPlayerDeaths[i] = pPlayerKilled->m_Deaths;

#if defined(PLATFORM_CONSOLE)
				MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
				CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

				if(other_player == ~0)
				{	//suicide
					pGameMod->m_pMPHandler->StatisticsSetScore(pPlayerKilled->m_Score, pPlayerKilled->m_player_id);
					pGameMod->m_pMPHandler->StatisticsSetDeaths(pPlayerKilled->m_Deaths, pPlayerKilled->m_player_id);
					Msg2.AddStrAny(pPlayerKilled->m_Name);
				}
				else
				{
					pGameMod->m_pMPHandler->StatisticsSetScore(pPlayerKiller->m_Score, pPlayerKiller->m_player_id);
					pGameMod->m_pMPHandler->StatisticsSetDeaths(pPlayerKilled->m_Deaths, pPlayerKilled->m_player_id);

					Msg2.AddStrAny(pPlayerKilled->m_Name);
					Msg2.AddStrAny(pPlayerKiller->m_Name);
				}
#else
				if(other_player == ~0)
				{	//suicide
					Msg2.AddStrAny(pPlayerKilled->m_Name);
				}
				else
				{
					Msg2.AddStrAny(pPlayerKilled->m_Name);
					Msg2.AddStrAny(pPlayerKiller->m_Name);
				}
#endif
			
				m_pWServer->NetMsg_SendToObject(Msg2, m_iObject);
			}
			
			return 1;
		}
	}

	return 0;
}

aint CWObject_GameTDM::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAME_CANPICKUPSPAWN:
		{
			if(_Msg.m_Param0 & CWObject_Trigger_Pickup::PICKUP_SPAWNFLAG_TDM)
			{
				if(m_GameMode == MP_MODE_SHAPESHIFTER)
				{
					if(_Msg.m_Param0 & CWObject_Trigger_Pickup::PICKUP_SPAWNFLAG_SHAPESHIFTER)
						return 1;
				}
				else if(m_GameMode == MP_MODE_DARKLINGS)
				{
					if(_Msg.m_Param0 & CWObject_Trigger_Pickup::PICKUP_SPAWNFLAG_DARKLINGS)
						return 1;
				}
				else
				{
					if(_Msg.m_Param0 & CWObject_Trigger_Pickup::PICKUP_SPAWNFLAG_DVH)
						return 1;
				}
			}
			return 0;
		}
	case OBJMSG_GAME_TOGGLEDARKLINGHUMAN:
		{
			if(!(m_GameMode == MP_MODE_DARKLINGS_VS_HUMANS && !_Msg.m_Param0))
				return parent::OnMessage(_Msg);
		}
		return 1;
	case OBJMSG_GAME_RESPAWN:
		{
			if(m_GameMode == MP_MODE_DARKLINGS_VS_HUMANS)
			{
				CWO_PlayerTDM *pPlayer= Player_GetTDM(_Msg.m_iSender);
				uint32 compare = pPlayer->m_DeathTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * 1.0f);
				bool bForce = _Msg.m_Param0 ? true : false;
				if(compare > m_pWServer->GetGameTick() && !bForce)
					return 1;
				if(pPlayer->m_Mode == PLAYERMODE_DEAD || bForce)
				{
					CMat4Dfp32 Mat = GetSpawnPosition();
					int16 Flags = 0;

					Player_Respawn(pPlayer->m_iPlayer, Mat, pPlayer->m_ClassHuman, Flags);
					pPlayer->m_Mode = 0;
					if(pPlayer->m_Team)
					{
						CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(pPlayer->m_iObject));
						CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pPlayerObj);

						CWObject_Message Msg;
						Msg.m_Msg = OBJMSG_GAME_TOGGLEDARKLINGHUMAN;
						Msg.m_iSender = pPlayer->m_iPlayer;
						Msg.m_Param0 = 1;
						m_pWServer->Message_SendToObject(Msg, m_iObject);
					}
					else
					{
						pPlayer->m_bIsHuman = true;
						m_pWServer->Net_ClientCommand(pPlayer->m_iPlayer, "cl_darknessvision(0)");
					}

					m_pWServer->Net_ClientCommand(_Msg.m_iSender, "mp_setmultiplayerstatus(0)");
				}
			}
			else
				return parent::OnMessage(_Msg);
		}
		return 1;
	}
	return parent::OnMessage(_Msg);
}

void CWObject_GameTDM::EndGame()
{
	CClientData *pCD = GetClientData();
	pCD->m_GameOver = true;
	m_GameOverTick = m_pWServer->GetGameTick();
	int Team1Score = 0;
	int Team2Score = 0;
	for(uint i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerTDM* pPlayer = Player_GetTDM(i);
		if(pPlayer)
		{
			m_pWServer->Net_ClientCommand(i, "mp_setmultiplayerstatus(1)");
			CWObject *pObj = m_pWServer->Object_Get(pPlayer->m_iObject);
			CMat4Dfp32 Mat = pObj->GetPositionMatrix();
			Player_Respawn(i, Mat, "Spectator", 0, -1);
			if(pPlayer->m_Team)
				Team2Score += pPlayer->m_Score;
			else
				Team1Score += pPlayer->m_Score;
		}
	}

	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

	bool bTeam1Won = Team1Score > Team2Score;

	for(uint i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerTDM* pPlayer = Player_GetTDM(i);
		if(pPlayer)
		{
			bool bAwardAchievement = false;
			int iSound = 0;
			if(Team1Score == Team2Score)
				 iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_125");
			else if(m_GameMode != MP_MODE_DARKLINGS_VS_HUMANS)
			{
				if(bTeam1Won)
				{
					if(!pPlayer->m_Team)
						iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_126");
					else
						iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_127");
				}
				else
				{
					if(pPlayer->m_Team)
						iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_126");
					else
						iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_127");
				}

				if((bTeam1Won && !pPlayer->m_Team) || pPlayer->m_Team)
					bAwardAchievement = true;
			}
			else
			{
				if(bTeam1Won)
					iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_134");
				else
					iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_135");
				
				if((bTeam1Won && !pPlayer->m_Team) || pPlayer->m_Team)
					bAwardAchievement = true;
			}
			if(iSound)
				m_pWServer->Sound_Global(iSound, 1.0f, i);

			if(bAwardAchievement)
			{
				CNetMsg Msg(WPACKET_OBJNETMSG);
				Msg.AddInt8(NETMSG_GAME_ACHIEVEMENT);
				Msg.AddInt16(m_iObject);
				Msg.AddInt16(m_iClass);
				Msg.AddInt8(ACHIEVEMENT_FIRST_MP_VICTORY);
				m_pWServer->NetMsg_SendToClient(Msg, pPlayer->m_iPlayer);
			}
		}
	}
}

void CWObject_GameTDM::OnRefresh()
{
	parent::OnRefresh();
	CClientData *pCD = GetClientData();
	uint8 Team1 = 0;
	uint8 Team2 = 0;
	uint8 Team1Player = 0;
	uint8 Team2Player = 0;
	if(pCD->m_WarmUpMode == MP_WARMUP_MODE_NONE)
	{
		for (uint i = 0; i < Player_GetNum(); i++)
		{
			CWO_PlayerTDM* pPlayer = Player_GetTDM(i);
			if (pPlayer)
			{
				if(pPlayer->m_bJoined)
				{
					if(!pPlayer->m_Team)
					{
						Team1++;
						Team1Player = i;
					}
					else
					{
						Team2++;
						Team2Player = i;
					}
				}
				else	//Someone hasn't joined yet, no need to autobalance
					return;
			}
		}
	}
	if(Team1 > Team2 + 1)
	{	//Move one player to team2
		CWO_PlayerTDM* pPlayer = Player_GetTDM(Team1Player);
		pPlayer->m_Team = 1;
		pCD->m_lPlayerTeams[Team1Player] = 1;
		pCD->m_lPlayerTeams.MakeDirty();
		ValidateHumanModelForTeam(Team1Player);
		ValidateDarklingModelforTeam(Team1Player);

		CWObject_Message Msg(OBJMSG_GAME_RESPAWN);
		Msg.m_iSender = Team1Player;
		Msg.m_Param0 = 1;
		m_pWServer->Message_SendToObject(Msg, m_iObject);
	}
	else if(Team2 > Team1 + 1)
	{	//Move one player to team1
		CWO_PlayerTDM* pPlayer = Player_GetTDM(Team2Player);
		pPlayer->m_Team = 0;
		pCD->m_lPlayerTeams[Team2Player] = 0;
		pCD->m_lPlayerTeams.MakeDirty();
		ValidateHumanModelForTeam(Team2Player);
		ValidateDarklingModelforTeam(Team2Player);

		CWObject_Message Msg(OBJMSG_GAME_RESPAWN);
		Msg.m_iSender = Team2Player;
		Msg.m_Param0 = 1;
		m_pWServer->Message_SendToObject(Msg, m_iObject);
	}
}

int CWObject_GameTDM::OnRemoveClient(int _iClient, int _Reason)
{
	if(_iClient == -1)
	{
		ConOutL("§cf80WARNING: Dropping client with index -1");
		return 1;
	}
	CClientData *pCD = GetClientData();
	if(pCD->m_lPlayerTeams[_iClient])
		pCD->m_lPlayerTeams[_iClient] = -1;
	pCD->m_lPlayerTeams.MakeDirty();

	return parent::OnRemoveClient(_iClient, _Reason);
}

void CWObject_GameTDM::OnClientRenderMultiplayerStatus(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D)
{
	CClientData *pCD = GetClientData(_pObj);

	if(pCD->m_lPlayerNames.Len() == 0)
		return;

//	SPlayerInfo *PlayerInfo = DNew(SPlayerInfo) SPlayerInfo[pCD->m_lPlayerNames.Len()];
	TThinArray<SPlayerInfo> PlayerInfo;
	PlayerInfo.SetLen(pCD->m_lPlayerNames.Len());
	memset(PlayerInfo.GetBasePtr(), 0, PlayerInfo.ListSize());

	uint8 NumPlayerInfo = 0;
	int16 Team1 = 0;
	int16 Team2 = 0;
	for(int i = 0; i < pCD->m_lPlayerNames.Len(); i++)
	{
		if(pCD->m_lPlayerNames[i] != "")
		{	
			PlayerInfo[NumPlayerInfo].m_Name = pCD->m_lPlayerNames[i];
			if(pCD->m_lPlayerScores.Len() > i)
				PlayerInfo[NumPlayerInfo].m_SortKey = pCD->m_lPlayerScores[i] + 32768;
			else
				PlayerInfo[NumPlayerInfo].m_SortKey = 32768;
			if(pCD->m_lPlayerDeaths.Len() > i)
				PlayerInfo[NumPlayerInfo].m_Deaths = pCD->m_lPlayerDeaths[i];
			else
				PlayerInfo[NumPlayerInfo].m_Deaths = 0;
			PlayerInfo[NumPlayerInfo].m_Team = pCD->m_lPlayerTeams[i];
			if(PlayerInfo[NumPlayerInfo].m_Team == 0)
				Team1 += pCD->m_lPlayerScores[i];
			else
				Team2 += pCD->m_lPlayerScores[i];
			NumPlayerInfo++;
		}
	}

	RadixSort(PlayerInfo.GetBasePtr(), NumPlayerInfo);

	CClipRect Clip(0, 0, 640, 480);
	_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
	if(!pFont)
		return;

	CRct rect;
	rect.p0.x = 100;
	rect.p0.y = 90;
	rect.p1.x = 500;
	rect.p1.y = 300;
	_pUtil2D->Rect(Clip, rect, SCOREBOARD_BACKGROUND);

	_pUtil2D->Text_DrawFormatted(Clip, pFont, pCD->m_GameModeName.m_Value, -20, 100, WSTYLE_TEXT_SHADOW | WSTYLE_TEXT_CENTER, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_TIME_LEFT"), 250, 125, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z16 §LMP_SCOREBOARD_TOTAL_SCORE"), 225, 140, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStrF("§Z16 %i", Team1), 160, 140, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStrF("§Z16 %i", Team2), 415, 140, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_NAME"), 105, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_SCORE"), 185, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_DEATHS"), 225, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_PING"), 265, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_NAME"), 300, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_SCORE"), 380, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_DEATHS"), 420, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_PING"), 460, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

	DrawPlayerListBackGround(_pUtil2D, CRct(105, 180, 295, 295));
	DrawPlayerListBackGround(_pUtil2D, CRct(295, 180, 490, 295));

	CStr St;
	int8 minuter = (int)(pCD->m_MaxTime - ((_pWClient->GetGameTick() - pCD->m_StartTick) / _pWClient->GetGameTicksPerSecond())) / 60;
	if(minuter < 0)
		minuter = 0;
	int8 sekunder = (int)(pCD->m_MaxTime - ((_pWClient->GetGameTick() - pCD->m_StartTick) / _pWClient->GetGameTicksPerSecond())) % 60;
	if(sekunder < 0)
		sekunder = 0;
	if(sekunder < 10)
		St = CStrF("§Z10%i:0%i", minuter, sekunder);
	else
		St = CStrF("§Z10%i:%i", minuter, sekunder);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 320, 125, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

	int row1 = 0;
	int row2 = 0;
	for(int8 i = NumPlayerInfo - 1; i > -1; i--)
	{
		CStr Name = PlayerInfo[i].m_Name;
		int Score = PlayerInfo[i].m_SortKey - 32768;
		int deaths = PlayerInfo[i].m_Deaths;
		int ping = 0;

		int team = PlayerInfo[i].m_Team;
		int row = 0;
		int offset = 0;
		if(team == 0)
			row = row1;
		else 
		{
			row = row2;
			offset = 195;
		}

		if(Name.IsUnicode())
			St = CStrF(WTEXT("§Z10 %s"), Name.StrW());
		else
			St = CStrF("§Z10 %s", Name.Str());
		_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 110 + offset, 185 + row * 10, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

		if(Score < 10)
			St = CStrF("§Z10   %i", Score);
		else if(Score < 100)
			St = CStrF("§Z10  %i", Score);
		else
			St = CStrF("§Z10 %i", Score);
		_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 185 + offset, 185 + row * 10, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

		if(deaths < 10)
			St = CStrF("§Z10   %i", deaths);
		else if(deaths < 100)
			St = CStrF("§Z10  %i", deaths);
		else
			St = CStrF("§Z10 %i", deaths);
		_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 225 + offset, 185 + row * 10, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

		if(ping < 10)
			St = CStrF("§Z10   %i", ping);
		else if(ping < 100)
			St = CStrF("§Z10  %i", ping);
		else
			St = CStrF("§Z10 %i", ping);
		_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 265 + offset, 185 + row * 10, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

		if(team == 0)
			row1++;
		else 
			row2++;

	}
}

spCWO_Player CWObject_GameTDM::CreatePlayerObject()
{
	CWO_PlayerTDM *pP = MNew(CWO_PlayerTDM);
	spCWO_Player spP = (CWO_Player *)pP;
	if (!spP) MemError("CreateChar");

	return spP;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_GameCTF
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_GameCTF::CWObject_GameCTF()
{
	M_TRACE("CWObject_GameCTF::CWObject_GameCTF\n");

	m_NumCaptures[0] = 0;
	m_NumCaptures[1] = 0;

	CClientData *pCD = GetClientData();
	pCD->m_lCaptures.Add(0);
	pCD->m_lCaptures.Add(0);

	m_bDoOnce = true;
	pCD->m_WarmUpMode = MP_WARMUP_MODE_PREGAME;
	pCD->m_WarmUpCountDownTick = -1;
	m_nNumPlayersNeeded = 2;

#if defined(PLATFORM_CONSOLE)
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
	if(pGameMod)
	{
		pCD->m_MaxTime = pGameMod->m_pMPHandler->m_TimeLimit;
		pCD->m_MaxCaptures = pGameMod->m_pMPHandler->m_CaptureLimit;
		pCD->m_GameModeName.m_Value = CStrF("§Z20 %s",pGameMod->m_pMPHandler->GetGameModeName().Str());
	}
#else
	pCD->m_MaxTime = 600;
	pCD->m_MaxCaptures = 3;
	pCD->m_GameModeName.m_Value = "§Z20 §LMENU_SHAPESHIFTER - §LMENU_CTF";
#endif
	pCD->m_GameModeName.MakeDirty();
}

CWObject_GameCTF::CWO_PlayerCTF *CWObject_GameCTF::Player_GetCTF(int _iPlayer)
{
	return (CWO_PlayerCTF *)(CWO_Player *)m_lspPlayers[_iPlayer];
}

int CWObject_GameCTF::OnClientConnected(int _iClient)
{
	int res = parent::OnClientConnected(_iClient);

	CClientData *pCD = GetClientData();
	pCD->m_MaxCaptures = m_MaxCaptures;

	return res;
}

int CWObject_GameCTF::OnInitWorld()
{
	M_TRACE("CWObject_GameCTF::CWObject_GameCTF\n");

	return parent::OnInitWorld();
}

void CWObject_GameCTF::OnSpawnWorld()
{
	GetFlagSpawnPosition();

	parent::OnSpawnWorld();
}

void CWObject_GameCTF::OnFlagPickedUp(int _iPlayer, int _iObject)
{
	CClientData *pCD = GetClientData();
	if(pCD->m_WarmUpMode)
		return;
	CWObject_Trigger_Pickup *pTriggerObject = safe_cast<CWObject_Trigger_Pickup>(m_pWServer->Object_Get(_iObject));

	uint nPlayers = Player_GetNum();
	uint iPlayerIndex = 0;
	for(uint i = 0; i < nPlayers; i++)
	{
		CWO_Player* pPlayer = Player_Get(i);
		if (pPlayer && pPlayer->m_iObject == _iPlayer)
		{
			iPlayerIndex = i;
			break;
		}
	}

	CWO_PlayerCTF *pPlayer = Player_GetCTF(iPlayerIndex);
	int PlayerTeam = pPlayer->m_Team;
	CWObject *pFlag = m_pWServer->Object_Get(_iObject);
	int FlagTeam = pTriggerObject->m_Param0;

	CNetMsg Msg2;
	Msg2.AddStrAny(pPlayer->m_Name);
	Msg2.AddInt8(PlayerTeam);

	if(PlayerTeam != FlagTeam)
	{	//Enemy flag, pick it up
		pPlayer->m_bHasFlag = true;

		int iFlagIndex = m_pWServer->GetMapData()->GetResourceIndex_Model(pTriggerObject->m_ModelName.Str());

		m_pWServer->Object_Destroy(_iObject);

		for(int i = 0; i < Player_GetNum(); i++)
		{
			CWO_PlayerCTF *pClient = Player_GetCTF(i);
			if(pClient)
			{
				if(pClient->m_Team == PlayerTeam)
				{
					int iEnemyFlagStolen = -1;
					if(pClient->m_iPlayer != i)
						iEnemyFlagStolen = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_100");
					else
						iEnemyFlagStolen = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_101");
					if(iEnemyFlagStolen)
						m_pWServer->Sound_Global(iEnemyFlagStolen, 1.0f, i);

					Msg2.m_MsgType = NETMSG_GAME_ENEMY_FLAGSTOLEN;
				}
				else
				{
					int iFlagStolen = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_110");
					if(iFlagStolen)
						m_pWServer->Sound_Global(iFlagStolen, 1.0f, i);

					Msg2.m_MsgType = NETMSG_GAME_FLAGSTOLEN;
				}

				CNetMsg Msg(WPACKET_OBJNETMSG);
				Msg.AddInt8(Msg2.m_MsgType);
				Msg.AddInt16(m_iObject);
				Msg.AddInt16(m_iClass);
				if (Msg.AddData((void*)Msg2.m_Data, Msg2.m_MsgSize))
				{
					m_pWServer->Net_PutMsgEx(i, Msg, -1, WCLIENT_STATE_INGAME);
				}

				CWObject* pPlayerObj = m_pWServer->Object_Get(_iPlayer);
				if (pPlayerObj)
				{
					CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pPlayerObj);
					pCD->m_iFlagIndex = iFlagIndex;
					pCD->m_iFlagIndex.MakeDirty();
				}
			}
		}

		return;
	}
	else
	{	//Same team, we can't pick this one up, check if player has enemy flag, if so capture
		bool FlagIsAtBase = true;

		CMat4Dfp32 SpawnPos;
		SpawnPos = m_FlagSpawnPos[FlagTeam];
		CWObject *pObj = m_pWServer->Object_Get(_iObject);
		CVec3Dfp32 FlagPos = pObj->GetPosition();

		if(FlagPos.AlmostEqual(SpawnPos.GetRow(3), 3.0f))
			FlagIsAtBase = true;
		else
			FlagIsAtBase = false;

		if(!FlagIsAtBase)
		{
			m_pWServer->Object_SetPosition_World(_iObject, SpawnPos);

			pPlayer->m_Score++;
			GetClientData()->m_lPlayerScores[iPlayerIndex] = pPlayer->m_Score;
			GetClientData()->m_lPlayerScores.MakeDirty();

			for(int i = 0; i < Player_GetNum(); i++)
			{
				CWO_PlayerCTF *pClient = Player_GetCTF(i);

				if(pClient)
				{
					if(pClient->m_Team == PlayerTeam)
					{
						int iFlagReturned = -1;
						if(pClient->m_iPlayer != i)
							iFlagReturned = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_104");
						else
							iFlagReturned = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_105");
						if(iFlagReturned)
							m_pWServer->Sound_Global(iFlagReturned, 1.0f, i);
						Msg2.m_MsgType = NETMSG_GAME_FLAGRETURNED;
					}
					else
					{
						int iEnemyFlagReturned = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_112");
						if(iEnemyFlagReturned)
							m_pWServer->Sound_Global(iEnemyFlagReturned, 1.0f, i);
						Msg2.m_MsgType = NETMSG_GAME_ENEMY_FLAGRETURNED;
					}
		
					CNetMsg Msg(WPACKET_OBJNETMSG);
					Msg.AddInt8(Msg2.m_MsgType);
					Msg.AddInt16(m_iObject);
					Msg.AddInt16(m_iClass);
					if (Msg.AddData((void*)Msg2.m_Data, Msg2.m_MsgSize))
						m_pWServer->Net_PutMsgEx(i, Msg, -1, WCLIENT_STATE_INGAME);
				}
			}
		}
		else if(pPlayer->m_bHasFlag)
		{
			for(int i = 0; i < Player_GetNum(); i++)
			{
				CWO_PlayerCTF *pClient = Player_GetCTF(i);

				if(pClient)
				{
					if(pClient->m_Team == PlayerTeam)
					{
						int iEnemyFlagCaptured = -1;
						if(pClient->m_iPlayer != i)
							iEnemyFlagCaptured = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_106");
						else
							iEnemyFlagCaptured = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_107");
						if(iEnemyFlagCaptured)
							m_pWServer->Sound_Global(iEnemyFlagCaptured, 1.0f, i);
						Msg2.m_MsgType = NETMSG_GAME_ENEMY_FLAGCAPTURED;
					}
					else
					{
						int iFlagCaptured = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_113");
						if(iFlagCaptured)
							m_pWServer->Sound_Global(iFlagCaptured, 1.0f, i);
						Msg2.m_MsgType = NETMSG_GAME_FLAGCAPTURED;
					}
		
					CNetMsg Msg(WPACKET_OBJNETMSG);
					Msg.AddInt8(Msg2.m_MsgType);
					Msg.AddInt16(m_iObject);
					Msg.AddInt16(m_iClass);
					if (Msg.AddData((void*)Msg2.m_Data, Msg2.m_MsgSize))
						m_pWServer->Net_PutMsgEx(i, Msg, -1, WCLIENT_STATE_INGAME);

					CWObject* pPlayerObj = m_pWServer->Object_Get(_iPlayer);
					if (pPlayerObj)
					{
						CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pPlayerObj);
						pCD->m_iFlagIndex = -1;
						pCD->m_iFlagIndex.MakeDirty();
					}
				}
			}

			int Team1Captures = m_NumCaptures[0];
			int Team2Captures = m_NumCaptures[1];
			m_NumCaptures[pPlayer->m_Team]++;
			//Voice for when taking the lead, to much voices going on, disabled atm
/*			bool bGotLead = false;
			int Team = -1;
			if(!(Team1Captures > m_NumCaptures[1]) && m_NumCaptures[0] > m_NumCaptures[1])
			{
				bGotLead = true;
				Team = 0;
			}
			if(!(Team2Captures > m_NumCaptures[0]) && m_NumCaptures[1] > m_NumCaptures[0])
			{
				bGotLead = true;
				Team = 1;
			}

			if(bGotLead)
			{
				for(int ii = 0; ii < Player_GetNum(); ii++)
				{
					CWO_PlayerCTF *pPlayerSound = Player_GetCTF(ii);
					if(pPlayerSound)
					{
						if(pPlayerSound->m_Team == Team)
						{
							int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_140");
							if(iSound)
								m_pWServer->Sound_Global(iSound, 1.0f, ii);
						}
						else
						{
							int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_142");
							if(iSound)
								m_pWServer->Sound_Global(iSound, 1.0f, ii);
						}
					}
				}
			}*/
				

#if defined(PLATFORM_CONSOLE)
			MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
			CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
			pGameMod->m_pMPHandler->StatisticsSetCaptures(m_NumCaptures[pPlayer->m_Team], pPlayer->m_player_id);
#endif

			if(m_MaxCaptures && m_NumCaptures[pPlayer->m_Team] >= m_MaxCaptures)
				EndGame();
			CClientData *pCD = GetClientData();
			pCD->m_lCaptures[pPlayer->m_Team] = m_NumCaptures[pPlayer->m_Team];
			pCD->m_lCaptures.MakeDirty();
			pPlayer->m_Score++;
			pCD->m_lPlayerScores[iPlayerIndex] = pPlayer->m_Score;
			pCD->m_lPlayerScores.MakeDirty();

			pPlayer->m_bHasFlag = false;

			CMat4Dfp32 MatFlag;
			if(pPlayer->m_Team)
				m_pWServer->Object_Create("object_mp_ctf_flag1", m_FlagSpawnPos[0]);
			else
				m_pWServer->Object_Create("object_mp_ctf_flag2", m_FlagSpawnPos[1]);
		}

		return;
	}
}

aint CWObject_GameCTF::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAMEMOD_RENDERMULTIPLAYERSTATUS:
		OnClientRenderMultiplayerStatus(_pObj, _pWClient, (CXR_Engine *)_Msg.m_Param0, (CRC_Util2D *)_Msg.m_Param1);
		return 1;
	case OBJMSG_GAME_RENDERSTATUSBAR:
		OnClientRenderStatusBar(_pObj, _pWClient, (CXR_Engine *)_Msg.m_Param0, (CRC_Util2D *)_Msg.m_Param1, (CVec2Dfp32 *)_Msg.m_pData);
		return 1;
	}

	return parent::OnClientMessage(_pObj, _pWClient, _Msg);
}

void CWObject_GameCTF::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	switch(_Msg.m_MsgType)
	{
	case NETMSG_GAME_FLAGCAPTURED:
	case NETMSG_GAME_FLAGRETURNED:
	case NETMSG_GAME_FLAGSTOLEN:
	case NETMSG_GAME_ENEMY_FLAGCAPTURED:
	case NETMSG_GAME_ENEMY_FLAGRETURNED:
	case NETMSG_GAME_ENEMY_FLAGSTOLEN:
		{
			int p = 0;

			CClientData *pCD = GetClientData(_pObj);
			CClientData::SMultiplayerMessageImportant MMsg;
			MMsg.m_Type = _Msg.m_MsgType;
			MMsg.m_Str1 = _Msg.GetStrAny(p);
			MMsg.m_Team = _Msg.GetInt8(p);
			MMsg.m_StartTick = _pWClient->GetGameTick();
			MMsg.m_Duration = RoundToInt(2.0f * _pWClient->GetGameTicksPerSecond());

			pCD->m_lMultiplayerMessagesImportant.Add(MMsg);
		}
		return;
	}
	return parent::OnClientNetMsg(_pObj, _pWClient, _Msg);
}

int CWObject_GameCTF::OnCharacterKilled(int _iObject, int _iSender)
{
	M_TRACE("CWObject_GameCTF::OnCharacterKilled, iObject=%d, iSender=%d\n", _iObject, _iSender);

	for (uint i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerCTF* pPlayer = Player_GetCTF(i);
		if (pPlayer && pPlayer->m_iObject == _iObject)
		{
			if(pPlayer->m_bHasFlag)
			{
				pPlayer->m_bHasFlag = false;

				CWObject* pPlayerObj = m_pWServer->Object_Get(_iObject);
				if (pPlayerObj)
				{
					CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pPlayerObj);
					pCD->m_iFlagIndex = -1;
					pCD->m_iFlagIndex.MakeDirty();
				}

				CMat4Dfp32 MatFlag;
				MatFlag.CreateTranslation(pPlayerObj->GetPosition());
				MatFlag.r[3] = M_VAdd(MatFlag.r[3], M_VConst(0,0,42.0f,0));

				int iFlag = -1;
				if(pPlayer->m_Team)
					iFlag = m_pWServer->Object_Create("object_mp_ctf_flag1", MatFlag);
				else
					iFlag = m_pWServer->Object_Create("object_mp_ctf_flag2", MatFlag);

				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_103");
				if(iSound)
					m_pWServer->Sound_Global(iSound, 1.0f, i);
				
				for (uint ii = 0; ii < Player_GetNum(); ii++)
				{
					CWO_PlayerCTF* pPlayerAgain = Player_GetCTF(ii);
					if(pPlayerAgain && pPlayerAgain->m_iPlayer != i)
					{
						if(pPlayerAgain->m_Team == pPlayer->m_Team)
						{
							int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_102");
							if(iSound)
								m_pWServer->Sound_Global(iSound, 1.0f, ii);
						}
						else
						{
							int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_111");
							if(iSound)
								m_pWServer->Sound_Global(iSound, 1.0f, ii);
						}
					}
				}

				if(iFlag != -1)
					m_pWServer->Object_SetVelocity(iFlag, pPlayerObj->GetVelocity());
			}
			break;
		}
	}

	return parent::OnCharacterKilled(_iObject, _iSender);
}

void CWObject_GameCTF::GetFlagSpawnPosition(void)
{
/*	TSelection<CSelection::LARGE_BUFFER> Selection;

	m_pWServer->Selection_AddTarget(Selection, "object_mp_ctf_flag1");
	m_pWServer->Selection_AddTarget(Selection, "object_mp_ctf_flag2");

	const int16* pSel;
	int nSel = m_pWServer->Selection_Get(Selection, &pSel);*/

	m_FlagSpawnPos[0] = m_pWServer->Object_GetPositionMatrix(m_pWServer->Selection_GetSingleTarget("object_mp_ctf_flag1"));
	m_FlagSpawnPos[1] = m_pWServer->Object_GetPositionMatrix(m_pWServer->Selection_GetSingleTarget("object_mp_ctf_flag2"));
}

aint CWObject_GameCTF::OnMessage(const CWObject_Message& _Msg)
{
	CStr St;
	switch (_Msg.m_Msg)
	{
	case OBJMSG_GAME_CANPICKUPSPAWN:
		{
			if(_Msg.m_Param0 & CWObject_Trigger_Pickup::PICKUP_SPAWNFLAG_CTF)
			{
				if(m_GameMode == MP_MODE_SHAPESHIFTER)
				{
					if(_Msg.m_Param0 & CWObject_Trigger_Pickup::PICKUP_SPAWNFLAG_SHAPESHIFTER)
						return 1;
				}
				else if(m_GameMode == MP_MODE_DARKLINGS)
				{
					if(_Msg.m_Param0 & CWObject_Trigger_Pickup::PICKUP_SPAWNFLAG_DARKLINGS)
						return 1;
				}
				else
				{
					if(_Msg.m_Param0 & CWObject_Trigger_Pickup::PICKUP_SPAWNFLAG_DVH)
						return 1;
				}
			}
			return 0;
		}
	case OBJMSG_CHAR_PICKUP:
//		St = (char *)_Msg.m_pData;
		OnPickUp(_Msg.m_iSender, _Msg.m_Param0, _Msg.m_Param1);
		return 1;

	case OBJMSG_GAME_RESPAWN:
		{
			CWO_PlayerTDM *pPlayer= Player_GetTDM(_Msg.m_iSender);
			uint32 compare = pPlayer->m_DeathTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * 1.0f);
			bool bForce = _Msg.m_Param0 ? true : false;
			if(compare > m_pWServer->GetGameTick() && !bForce)
				return 1;
			if(pPlayer->m_Mode == PLAYERMODE_DEAD || bForce)
			{
				CMat4Dfp32 Mat = GetSpawnPosition(pPlayer->m_Team);
				int16 Flags = 0;

				Player_Respawn(pPlayer->m_iPlayer, Mat, pPlayer->m_ClassHuman, Flags);
				pPlayer->m_Mode = 0;
				if(m_GameMode == MP_MODE_DARKLINGS || (m_GameMode == MP_MODE_DARKLINGS_VS_HUMANS && pPlayer->m_Team))
				{
					CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(pPlayer->m_iObject));
					CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pPlayerObj);

					CWObject_Message Msg;
					Msg.m_Msg = OBJMSG_GAME_TOGGLEDARKLINGHUMAN;
					Msg.m_iSender = pPlayer->m_iPlayer;
					Msg.m_Param0 = 1;
					m_pWServer->Message_SendToObject(Msg, m_iObject);
				}
				else
				{
					pPlayer->m_bIsHuman = true;
					m_pWServer->Net_ClientCommand(pPlayer->m_iPlayer, "cl_darknessvision(0)");
				}

				m_pWServer->Net_ClientCommand(_Msg.m_iSender, "mp_setmultiplayerstatus(0)");
			}
		}
		return 1;

	case OBJMSG_GAME_GETTENSION:
		{
			//If a flag is missing we force battle music to all players
			for(int i = 0; i < Player_GetNum(); i++)
			{
				CWO_PlayerCTF* pPlayer = Player_GetCTF(i);
				if(pPlayer && pPlayer->m_bHasFlag)
				{
					return 255;
				}
			}
			break;
		}
	}

	return parent::OnMessage(_Msg);
}

void CWObject_GameCTF::OnRefresh()
{
	parent::OnRefresh();

	bool bFlag1Ok = false;
	bool bFlag2Ok = false;

	for (uint i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerCTF* pPlayer = Player_GetCTF(i);
		if (pPlayer)
		{
			if(pPlayer->m_bHasFlag)
			{
				if(pPlayer->m_Team)
					bFlag1Ok = true;
				else
					bFlag2Ok = true;
			}
		}
	}

	if(!bFlag1Ok)
	{
		int iObj = m_pWServer->Selection_GetSingleTarget("object_mp_ctf_flag1");
		CWObject *pObj = m_pWServer->Object_Get(iObj);
		if(pObj)
		{
			if(pObj->GetPosition().k[2] < -10000.0f)
				m_pWServer->Object_SetPosition_World(iObj, m_FlagSpawnPos[0]);
		}
		else
			m_pWServer->Object_Create("object_mp_ctf_flag1", m_FlagSpawnPos[0]);
	}
	if(!bFlag2Ok)
	{
		int iObj = m_pWServer->Selection_GetSingleTarget("object_mp_ctf_flag2");
		CWObject *pObj = m_pWServer->Object_Get(iObj);
		if(pObj)
		{
			if(pObj->GetPosition().k[2] < -10000.0f)
				m_pWServer->Object_SetPosition_World(iObj, m_FlagSpawnPos[1]);
		}
		else
			m_pWServer->Object_Create("object_mp_ctf_flag1", m_FlagSpawnPos[1]);
	}
}

int CWObject_GameCTF::OnRemoveClient(int _iClient, int _Reason)
{
	CWO_PlayerCTF* pPlayer = Player_GetCTF(_iClient);
	if(pPlayer->m_bHasFlag)
	{
		pPlayer->m_bHasFlag = false;

		CWObject* pPlayerObj = m_pWServer->Object_Get(pPlayer->m_iObject);
		if (pPlayerObj)
		{
			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pPlayerObj);
			pCD->m_iFlagIndex = -1;
			pCD->m_iFlagIndex.MakeDirty();
		}

		CVec3Dfp32 Pos = pPlayerObj->GetPosition();

		CMat4Dfp32 MatFlag;
		MatFlag.CreateTranslation(Pos);
		MatFlag.r[3] = M_VAdd(MatFlag.r[3],M_VConst(0,0,42.0f,0));

		if(pPlayer->m_Team)
			m_pWServer->Object_Create("object_mp_ctf_flag1", MatFlag);
		else
			m_pWServer->Object_Create("object_mp_ctf_flag2", MatFlag);
	}
	
	return parent::OnRemoveClient(_iClient, _Reason);
}

void CWObject_GameCTF::OnPickUp(int _iPlayer, int _iObject, int _TicksLeft)
{
	CWObject_Trigger_Pickup *pTriggerObject = safe_cast<CWObject_Trigger_Pickup>(m_pWServer->Object_Get(_iObject));

	if(pTriggerObject->m_Type == CWObject_Trigger_Pickup::PICKUP_FLAG)
		OnFlagPickedUp(_iPlayer, _iObject);
	else
		parent::OnPickUp(_iPlayer, _iObject, _TicksLeft);
}

void CWObject_GameCTF::ToggleDarklingHuman(int16 _iSender, bool _bForce)
{
	CClientData *pCD = GetClientData();
	if((m_GameMode != MP_MODE_SHAPESHIFTER || pCD->m_GameOver) && !_bForce)
		return;

	CWO_PlayerCTF* pPlayer = Player_GetCTF(_iSender);
	if (pPlayer)
	{
		CMat4Dfp32 Mat;
		int16 Flags = 0;
		CWServer_Mod* pServer = safe_cast<CWServer_Mod>(m_pWServer);
		M_ASSERT(pServer, "No server!");

		if(pPlayer->m_bHasFlag)
		{	//Player have the flag, we need to make sure the flag is rendered
			int iFlagIndex = -1;
			if(!pPlayer->m_Team)
				iFlagIndex = m_pWServer->GetMapData()->GetResourceIndex_Model("Details\\ww1_Flag_Ger_01:ww1_Flag_Am_01");
			else
				iFlagIndex = m_pWServer->GetMapData()->GetResourceIndex_Model("Details\\ww1_Flag_Ger_01");

			CWObject* pPlayerObj = m_pWServer->Object_Get(pPlayer->m_iObject);
			if (pPlayerObj)
			{
				CWO_Character_ClientData* pCDChar = CWObject_Character::GetClientData(pPlayerObj);
				pCDChar->m_iFlagIndex = iFlagIndex;
				pCDChar->m_iFlagIndex.MakeDirty();
			}
		}
	}
	parent::ToggleDarklingHuman(_iSender, _bForce);
}

spCWO_Player CWObject_GameCTF::CreatePlayerObject()
{
	CWO_PlayerCTF *pP = MNew(CWO_PlayerCTF);
	spCWO_Player spP = (CWO_Player *)pP;
	if (!spP) MemError("CreateChar");

	return spP;
}

void CWObject_GameCTF::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	parent::OnIncludeClass(_pWData, _pWServer);

	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_100");	//Your team has the enemy flag
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_101");	//You have the enemy flag
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_102");	//Your team dropped the enemy flag
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_103");	//You dropped the enemy flag
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_104");	//Your team returned the flag
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_105");	//You have returned your flag
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_106");	//Your team captured the enemy flag
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_107");	//You captured the enemy flag
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_110");	//The enemy team has your flag
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_111");	//The enemy team dropped your flag
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_112");	//The enemy team returned their flag
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_113");	//The enemy team captured your flag

	_pWData->GetResourceIndex_Class("object_mp_ctf_flag1");
	_pWData->GetResourceIndex_Class("object_mp_ctf_flag2");
}

void CWObject_GameCTF::GivePoint(int _iPlayer, int _nPoints)
{
	CClientData *pCD = GetClientData();
	CWO_PlayerDM* pPlayer = Player_GetDM(_iPlayer);

	pPlayer->m_Score += _nPoints;

	pCD->m_lPlayerScores[_iPlayer] = pPlayer->m_Score;
}

void CWObject_GameCTF::EndGame()
{
	CClientData *pCD = GetClientData();
	pCD->m_GameOver = true;
	m_GameOverTick = m_pWServer->GetGameTick();
	for(uint i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerDM* pPlayer = Player_GetDM(i);
		if(pPlayer)
		{
			m_pWServer->Net_ClientCommand(i, "mp_setmultiplayerstatus(1)");
			CWObject *pObj = m_pWServer->Object_Get(pPlayer->m_iObject);
			CMat4Dfp32 Mat = pObj->GetPositionMatrix();
			Player_Respawn(i, Mat, "Spectator", 0, -1);
		}
	}

	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

	int WinningTeam = -1;
	if(m_NumCaptures[0] > m_NumCaptures[1])
		WinningTeam = 0;
	else if(m_NumCaptures[1] > m_NumCaptures[0])
		WinningTeam = 1;
	for(uint i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerCTF* pPlayer = Player_GetCTF(i);
		if(pPlayer)
		{
			int iSound = 0;
			bool bAwardAchievement = false;
			if(WinningTeam == -1)
				iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_125");
			else if(m_GameMode != MP_MODE_DARKLINGS_VS_HUMANS)
			{
				if(pPlayer->m_Team == WinningTeam)
				{
					iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_126");
					bAwardAchievement = true;
				}
				else
					iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_127");
			}
			else
			{
				if(!WinningTeam)
					iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_134");
				else
					iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_135");

				if(pPlayer->m_Team == WinningTeam)
					bAwardAchievement = true;
			}
			if(iSound)
				m_pWServer->Sound_Global(iSound, 1.0f, i);

			if(bAwardAchievement)
			{
				CNetMsg Msg(WPACKET_OBJNETMSG);
				Msg.AddInt8(NETMSG_GAME_ACHIEVEMENT);
				Msg.AddInt16(m_iObject);
				Msg.AddInt16(m_iClass);
				Msg.AddInt8(ACHIEVEMENT_FIRST_MP_VICTORY);
				m_pWServer->NetMsg_SendToClient(Msg, pPlayer->m_iPlayer);
			}
		}
	}
}

void CWObject_GameCTF::OnClientRenderMultiplayerStatus(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D)
{
	CClientData *pCD = GetClientData(_pObj);

	if(pCD->m_lPlayerNames.Len() == 0)
		return;

	TThinArray<SPlayerInfo> PlayerInfo;
	PlayerInfo.SetLen(pCD->m_lPlayerNames.Len());
	memset(PlayerInfo.GetBasePtr(), 0, PlayerInfo.ListSize());

	uint8 NumPlayerInfo = 0;
	for(int i = 0; i < pCD->m_lPlayerNames.Len(); i++)
	{
		if(pCD->m_lPlayerNames[i] != "")
		{
			PlayerInfo[NumPlayerInfo].m_Name = pCD->m_lPlayerNames[i];
			if(pCD->m_lPlayerScores.Len() > i)
				PlayerInfo[NumPlayerInfo].m_SortKey = pCD->m_lPlayerScores[i] + 32768;
			else
				PlayerInfo[NumPlayerInfo].m_SortKey = 32768;
			if(pCD->m_lPlayerDeaths.Len() > i)
				PlayerInfo[NumPlayerInfo].m_Deaths = pCD->m_lPlayerDeaths[i];
			else
				PlayerInfo[NumPlayerInfo].m_Deaths = 0;
			PlayerInfo[NumPlayerInfo].m_Team = pCD->m_lPlayerTeams[i];
			NumPlayerInfo++;
		}
	}

	RadixSort(PlayerInfo.GetBasePtr(), NumPlayerInfo);

	CClipRect Clip(0, 0, 640, 480);
	_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
	if(!pFont)
		return;

	CRct rect;
	rect.p0.x = 100;
	rect.p0.y = 90;
	rect.p1.x = 500;
	rect.p1.y = 300;
	_pUtil2D->Rect(Clip, rect, SCOREBOARD_BACKGROUND);

	_pUtil2D->Text_DrawFormatted(Clip, pFont, pCD->m_GameModeName.m_Value, -20, 100, WSTYLE_TEXT_SHADOW | WSTYLE_TEXT_CENTER, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_TIME_LEFT"), 250, 125, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z16 §LMP_SCOREBOARD_FLAGS_CAPTURED"), 225, 140, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStrF("§Z16 %i", pCD->m_lCaptures[0]), 160, 140, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStrF("§Z16 %i", pCD->m_lCaptures[1]), 415, 140, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_NAME"), 105, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_SCORE"), 185, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_DEATHS"), 225, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_PING"), 265, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_NAME"), 300, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_SCORE"), 380, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_DEATHS"), 420, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, CStr("§Z10 §LMP_SCOREBOARD_PING"), 460, 165, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

	DrawPlayerListBackGround(_pUtil2D, CRct(105, 180, 295, 295));
	DrawPlayerListBackGround(_pUtil2D, CRct(295, 180, 490, 295));
	
	CStr St;
	int8 minuter = (int)(pCD->m_MaxTime - ((_pWClient->GetGameTick() - pCD->m_StartTick) / _pWClient->GetGameTicksPerSecond())) / 60;
	if(minuter < 0)
		minuter = 0;
	int8 sekunder = (int)(pCD->m_MaxTime - ((_pWClient->GetGameTick() - pCD->m_StartTick) / _pWClient->GetGameTicksPerSecond())) % 60;
	if(sekunder < 0)
		sekunder = 0;
	if(sekunder < 10)
		St = CStrF("§Z10%i:0%i", minuter, sekunder);
	else
		St = CStrF("§Z10%i:%i", minuter, sekunder);
	_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 320, 125, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

	int row1 = 0;
	int row2 = 0;
	for(int8 i = NumPlayerInfo - 1; i > -1; i--)
	{
		CStr Name = PlayerInfo[i].m_Name;
		int Score = PlayerInfo[i].m_SortKey - 32768;
		int deaths = PlayerInfo[i].m_Deaths;
		int ping = 0;

		int team = PlayerInfo[i].m_Team;
		int row = 0;
		int offset = 0;
		if(team == 0)
			row = row1;
		else 
		{
			row = row2;
			offset = 195;
		}

		if(Name.IsUnicode())
			St = CStrF(WTEXT("§Z10 %s"), Name.StrW());
		else
			St = CStrF("§Z10 %s", Name.Str());

		_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 110 + offset, 185 + row * 10, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

		if(Score < 10)
			St = CStrF("§Z10   %i", Score);
		else if(Score < 100)
			St = CStrF("§Z10  %i", Score);
		else
			St = CStrF("§Z10 %i", Score);
		_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 185 + offset, 185 + row * 10, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

		if(deaths < 10)
			St = CStrF("§Z10   %i", deaths);
		else if(deaths < 100)
			St = CStrF("§Z10  %i", deaths);
		else
			St = CStrF("§Z10 %i", deaths);
		_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 225 + offset, 185 + row * 10, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

		if(ping < 10)
			St = CStrF("§Z10   %i", ping);
		else if(ping < 100)
			St = CStrF("§Z10  %i", ping);
		else
			St = CStrF("§Z10 %i", ping);
		_pUtil2D->Text_DrawFormatted(Clip, pFont, St, 265 + offset, 185 + row * 10, WSTYLE_TEXT_SHADOW, SCOREBOARD_TEXTCOLOR, SCOREBOARD_TEXTSHADOW, 0, 640, 12, false);

		if(team == 0)
			row1++;
		else 
			row2++;

	}
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_GameSurvivor
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_GameSurvivor::CWObject_GameSurvivor()
{
	M_TRACE("CWObject_GameSurvivor::CWObject_GameSurvivor\n");

	CClientData *pCD = GetClientData();
#if defined(PLATFORM_CONSOLE)
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
	if(pGameMod)
	{
		pCD->m_MaxTime = pGameMod->m_pMPHandler->m_TimeLimit;
		pCD->m_MaxScore = pGameMod->m_pMPHandler->m_ScoreLimit;
		pCD->m_GameModeName.m_Value = pGameMod->m_pMPHandler->GetGameModeName();
	}
#else
	pCD->m_MaxScore = 20;
	pCD->m_MaxTime = 600;
	pCD->m_GameModeName.m_Value = "§Z20 §LMENU_SURVIVOR";
#endif
	pCD->m_GameModeName.MakeDirty();

	m_nNumPlayersNeeded = 3;
	pCD->m_WarmUpMode = MP_WARMUP_MODE_PREGAME;
	pCD->m_WarmUpCountDownTick = -1;
	m_bDoOnce = true;
	m_iNextToBeDarkling = -1;
}

CWObject_GameSurvivor::CWO_PlayerSurvivor *CWObject_GameSurvivor::Player_GetSurvivor(int _iPlayer)
{
	return (CWO_PlayerSurvivor *)(CWO_Player *)m_lspPlayers[_iPlayer];
}

void CWObject_GameSurvivor::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	parent::OnIncludeClass(_pWData, _pWServer);

	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_216");	//6 humans left
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_217");	//5 humans left
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_218");	//4 humans left
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_219");	//3 humans left
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_220");	//2 humans left
	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_221");	//1 humans left
}

aint CWObject_GameSurvivor::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAME_CANPICKUPSPAWN:
		{
			if(_Msg.m_Param0 & CWObject_Trigger_Pickup::PICKUP_SPAWNFLAG_SURVIVOR)
				return 1;
			return 0;
		}
	case OBJMSG_CHAR_PICKUP:
		{
			uint nPlayers = Player_GetNum();
			uint iPlayerIndex = 0;
			for(uint i = 0; i < nPlayers; i++)
			{
				CWO_Player* pPlayer = Player_Get(i);
				if (pPlayer && pPlayer->m_iObject == _Msg.m_iSender)
				{
					iPlayerIndex = i;
					break;
				}
			}

			CWO_PlayerSurvivor *pPlayer= Player_GetSurvivor(iPlayerIndex);
			if(pPlayer->m_bIsHuman)
				OnPickUp(_Msg.m_iSender, _Msg.m_Param0, _Msg.m_Param1);
		}
		return 1;
	case OBJMSG_GAME_TOGGLEDARKLINGHUMAN:
		{
			if(_Msg.m_Param0)
				return parent::OnMessage(_Msg);
		}
		return 1;
	case OBJMSG_GAME_RESPAWN:
		{
			CWO_PlayerSurvivor *pPlayer= Player_GetSurvivor(_Msg.m_iSender);
			uint32 compare = pPlayer->m_DeathTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * 1.0f);
			bool bForce = _Msg.m_Param0 ? true : false;
			if(compare > m_pWServer->GetGameTick() && !bForce)
				return 1;
			if(pPlayer->m_Mode == PLAYERMODE_DEAD || bForce)
			{
				CMat4Dfp32 Mat = GetSpawnPosition();
				int16 Flags = 0;

				Player_Respawn(pPlayer->m_iPlayer, Mat, pPlayer->m_ClassHuman, Flags);
				pPlayer->m_Mode = 0;
				if(pPlayer->m_SpawnAsDarkling)
				{
					CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(pPlayer->m_iObject));
					CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pPlayerObj);

					CWObject_Message Msg;
					Msg.m_Msg = OBJMSG_GAME_TOGGLEDARKLINGHUMAN;
					Msg.m_iSender = pPlayer->m_iPlayer;
					Msg.m_Param0 = 1;
					m_pWServer->Message_SendToObject(Msg, m_iObject);
				}
				else
				{
					pPlayer->m_bIsHuman = true;
					m_pWServer->Net_ClientCommand(pPlayer->m_iPlayer, "cl_darknessvision(0)");
				}

				m_pWServer->Net_ClientCommand(_Msg.m_iSender, "mp_setmultiplayerstatus(0)");
			}
		}
		return 1;
	}
	return parent::OnMessage(_Msg);
}

int CWObject_GameSurvivor::OnClientConnected(int _iClient)
{
	if (!CWObject_GameP4::OnClientConnected(_iClient))
		return 0;

	CMat4Dfp32 Mat;
	int16 Flags = 0;
	CWServer_Mod* pServer = safe_cast<CWServer_Mod>(m_pWServer);
	M_ASSERT(pServer, "No server!");

	Mat = GetSpawnPosition();

	int PlayerObj = -1;
	int PlayerGUID = -1;
	CFStr Class;
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys->GetEnvironment()->GetValuei("FORCESPECTATOR"))
	{
		Class = "Spectator";
	}
	else
	{
		Class = "Spectator";

		//Open window to select team
//		OnSetClientWindow(_iClient, "multiplayer_selectcharacter_shapeshifters");
	}

	int iObj = Player_Respawn(Player_GetWithClient(_iClient)->m_iPlayer, Mat, Class, Flags, PlayerObj);
	if (iObj == -1)
	{
		ConOutL(CStrF("ERROR: Failed to spawn character %s", Class.Str()));
		return 0;
	}

	if (PlayerGUID != -1)
		pServer->Object_ChangeGUID(iObj, PlayerGUID);

	M_TRACE("CWObject_GameSurvivor::OnClientConnected");

	CClientData *pCD = GetClientData();
	bool bFoundEmptyPlayer = false;
	CWO_PlayerSurvivor *pPlayerClient = Player_GetSurvivor(_iClient);
	if(Player_GetNum() == 1)
		pPlayerClient->m_SpawnAsDarkling = true;	//First player to join
	for(int i = 0; i < pCD->m_lPlayerNames.Len(); i++)
	{	
		CWO_PlayerDM *pPlayer = Player_GetDM(i);
		if(pPlayer)
		{
			CNetMsg Msg2;
			Msg2.AddInt8(i);
			Msg2.AddStrAny(pPlayer->m_Name);
			CNetMsg Msg(WPACKET_OBJNETMSG);
			Msg.AddInt8(NETMSG_GAME_SETNAME);
			Msg.AddInt16(m_iObject);
			Msg.AddInt16(m_iClass);
			if (Msg.AddData((void*)Msg2.m_Data, Msg2.m_MsgSize))
				m_pWServer->Net_PutMsgEx(pPlayerClient->m_iPlayer, Msg, -1, WCLIENT_STATE_INGAME);

			if(!pPlayer->m_Name.Len())
				bFoundEmptyPlayer = true;
		}
	}

	if(!bFoundEmptyPlayer)
	{
		pCD->m_lPlayerScores.Add(0);
		pCD->m_lPlayerDeaths.Add(0);
		CStr EmptyName;
		pCD->m_lPlayerNames.Add(EmptyName);
	}

	pCD->m_lPlayerScores.MakeDirty();	//We must make it dirty so that the new player will get what we have
	pPlayerClient->m_Name = " ";

	while(pCD->m_lPlayerNames.Len() <= _iClient)
	{
		CStr EmptyName;
		pCD->m_lPlayerNames.Add(EmptyName);
	}

	while(pCD->m_lPlayerScores.Len() <= _iClient)
		pCD->m_lPlayerScores.Add(0);

	while(pCD->m_lPlayerDeaths.Len() <= _iClient)
		pCD->m_lPlayerDeaths.Add(0);

	pCD->m_MaxScore.MakeDirty();

	OnPlayerEntersGame(iObj, 0);

	return 1;
}

aint CWObject_GameSurvivor::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAMEMOD_RENDERMULTIPLAYERSTATUS:
		OnClientRenderMultiplayerStatus(_pObj, _pWClient, (CXR_Engine *)_Msg.m_Param0, (CRC_Util2D *)_Msg.m_Param1);
		return 1;

	case OBJMSG_GAME_RENDERSTATUSBAR:
		OnClientRenderStatusBar(_pObj, _pWClient, (CXR_Engine *)_Msg.m_Param0, (CRC_Util2D *)_Msg.m_Param1, (CVec2Dfp32 *)_Msg.m_pData);
		return 1;
	}

	return parent::OnClientMessage(_pObj, _pWClient, _Msg);
}

int CWObject_GameSurvivor::OnClientCommand(int _iPlayer, const CCmd* _pCmd)
{
	int Size = _pCmd->m_Size;
	switch(_pCmd->m_Cmd)
	{
	case CONTROL_JOINGAME:
		{
			char Class[256];
			memcpy(Class, _pCmd->m_Data, Size);
			Class[Size] = 0;

			CWO_PlayerSurvivor *pPlayer = Player_GetSurvivor(_iPlayer);
			pPlayer->m_ClassHuman = Class;
			pPlayer->m_bJoined = true;

			int State = m_pWServer->GetMapData()->GetState();
			m_pWServer->GetMapData()->SetState(State & ~WMAPDATA_STATE_NOCREATE);	
			CMat4Dfp32 Mat = GetSpawnPosition();
			int16 Flags = 0;
			Player_Respawn(_iPlayer, Mat, pPlayer->m_ClassHuman, Flags);

			CWObject *pObj = m_pWServer->Object_Get(pPlayer->m_iObject);
			CWObject_CharShapeshifter *pChar = safe_cast<CWObject_CharShapeshifter>(pObj);
			CWO_CharShapeshifter_ClientData &CD = pChar->GetClientData();
			CD.m_iDarklingModel = m_pWServer->GetMapData()->GetResourceIndex_Model(pPlayer->m_ModelDarkling.Str());
			m_pWServer->GetMapData()->SetState(State);

			if(pPlayer->m_SpawnAsDarkling)
			{
				CWObject_Message Msg;
				Msg.m_Msg = OBJMSG_GAME_TOGGLEDARKLINGHUMAN;
				Msg.m_iSender = pPlayer->m_iPlayer;
				Msg.m_Param0 = 1;
				m_pWServer->Message_SendToObject(Msg, m_iObject);
			}
			else
				pChar->RemoveAllDarklingsWeapons();

			OnSetClientWindow(pPlayer->m_iClient, "");

			CNetMsg Msg2;
			Msg2.m_MsgType = NETMSG_GAME_JOIN;
			Msg2.AddStrAny(pPlayer->m_Name);
			m_pWServer->NetMsg_SendToObject(Msg2, m_iObject);

			return true;
		}
	}
	return parent::OnClientCommand(_iPlayer, _pCmd);
}

int CWObject_GameSurvivor::OnCharacterKilled(int _iObject, int _iSender)
{
	M_TRACE("CWObject_GameSurvivor::OnCharacterKilled, iObject=%d, iSender=%d\n", _iObject, _iSender);

	uint nPlayers = Player_GetNum();
	uint other_player = ~0;
	for(uint j = 0; j < nPlayers; j++)
	{
		CWO_Player* pPlayer = Player_Get(j);
		if (pPlayer && pPlayer->m_iObject == _iSender)
		{
			other_player = j;
			break;
		}
	}

	for (uint i = 0; i < nPlayers; i++)
	{
		CWO_Player* pPlayer = Player_Get(i);
		if (pPlayer && pPlayer->m_iObject == _iObject)
		{
			CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(_iObject));
			CWO_CharShapeshifter_ClientData& CD = CWObject_CharShapeshifter::GetClientData(pPlayerObj);

			if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_DAMAGE)
			{
				CWObject_PickupInfo PickupInfo;
				PickupInfo.m_TemplateName = "object_mp_damage_boost";
				PickupInfo.m_Pos = pPlayerObj->GetPosition();
				PickupInfo.m_SpawnTick = m_pWServer->GetGameTick();
				PickupInfo.m_TicksLeft = CD.m_DamageBoostDuration - (m_pWServer->GetGameTick() - CD.m_DamageBoostStartTick);
				m_lPickups.Add(PickupInfo);

				int iDrainSoundInOut = m_pWServer->GetMapData()->GetResourceIndex_Sound("gam_drk_crp02");
				int iDrainLoopSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_Drk_crp03");

				if(iDrainSoundInOut)
					m_pWServer->Sound_At(pPlayerObj->GetPosition(), iDrainSoundInOut, WCLIENT_ATTENUATION_3D);

				if(iDrainLoopSound)
					m_pWServer->Sound_Off(pPlayerObj->m_iObject, iDrainLoopSound);

				CWObject_Message MsgDrain(OBJMSG_EFFECTSYSTEM_SETFADE, 2);
				m_pWServer->Message_SendToObject(MsgDrain, pPlayerObj->m_iDarkness_Drain_2);
			}
			if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_SPEED)
			{
				CWObject_PickupInfo PickupInfo;
				PickupInfo.m_TemplateName = "object_mp_speed_boost";
				PickupInfo.m_Pos = pPlayerObj->GetPosition();
				PickupInfo.m_SpawnTick = m_pWServer->GetGameTick();
				PickupInfo.m_TicksLeft = CD.m_SpeedBoostDuration - (m_pWServer->GetGameTick() - CD.m_SpeedBoostStartTick);
				m_lPickups.Add(PickupInfo);
			}
			if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_INVISIBILITY)
			{
				CWObject_PickupInfo PickupInfo;
				PickupInfo.m_TemplateName = "object_mp_invisibility";
				PickupInfo.m_Pos = pPlayerObj->GetPosition();
				PickupInfo.m_SpawnTick = m_pWServer->GetGameTick();
				PickupInfo.m_TicksLeft = CD.m_InvisibiltyBoostDuration - (m_pWServer->GetGameTick() - CD.m_InvisibiltyBoostStartTick);
				m_lPickups.Add(PickupInfo);

				pPlayerObj->ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
			}

			pPlayer->m_Mode = PLAYERMODE_DEAD;

			CWO_PlayerSurvivor *pPlayerKiller, *pPlayerKilled;
			pPlayerKiller = NULL;
			pPlayerKilled = NULL;

			CNetMsg Msg2;
			pPlayerKilled = Player_GetSurvivor(i);
			pPlayerKilled->m_DeathTick = m_pWServer->GetGameTick();
			pPlayerKilled->m_KillsInARow = 0;
			if(other_player != ~0)
			{
				pPlayerKiller = Player_GetSurvivor(other_player);

				if(!pPlayerKiller->m_bIsHuman && pPlayerKilled->m_bIsHuman)
				{
//					pPlayerKiller->m_Score++;
					AddKill(other_player);
					Msg2.m_MsgType = NETMSG_GAME_PLAYERKILL;
				}
				GetClientData()->m_lPlayerScores[other_player] = pPlayerKiller->m_Score;
			}
			else
			{	//suicide
//				pPlayerKilled->m_Score--;
				Msg2.m_MsgType = NETMSG_GAME_PLAYERSUICIDE;
				GetClientData()->m_lPlayerScores[i] = pPlayerKilled->m_Score;
			}

			GetClientData()->m_lPlayerScores.MakeDirty();
			pPlayerKilled->m_Deaths++;
			GetClientData()->m_lPlayerDeaths[i] = pPlayerKilled->m_Deaths;

#if defined(PLATFORM_CONSOLE)
			MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
			CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

			if(other_player == ~0)
			{	//suicide
//				pGameMod->m_pMPHandler->StatisticsSetScore(pPlayerKilled->m_Score, pPlayerKilled->m_player_id);
//				pGameMod->m_pMPHandler->StatisticsSetDeaths(pPlayerKilled->m_Deaths, pPlayerKilled->m_player_id);
				Msg2.AddStrAny(pPlayerKilled->m_Name);
			}
			else
			{
//				pGameMod->m_pMPHandler->StatisticsSetScore(pPlayerKiller->m_Score, pPlayerKiller->m_player_id);
//				pGameMod->m_pMPHandler->StatisticsSetDeaths(pPlayerKilled->m_Deaths, pPlayerKilled->m_player_id);

				Msg2.AddStrAny(pPlayerKilled->m_Name);
				Msg2.AddStrAny(pPlayerKiller->m_Name);
			}
#else
			if(other_player == ~0)
			{	//suicide
				Msg2.AddStrAny(pPlayerKilled->m_Name);
			}
			else
			{
				Msg2.AddStrAny(pPlayerKilled->m_Name);
				Msg2.AddStrAny(pPlayerKiller->m_Name);
			}
#endif

			m_pWServer->NetMsg_SendToObject(Msg2, m_iObject);
			m_pWServer->Net_ClientCommand(i, "mp_setmultiplayerstatus(1)");

			if(pPlayerKilled->m_bIsHuman)
			{	//One human less
				int nHumans = 0;
				for (uint ii = 0; ii < nPlayers; ii++)
				{
					CWO_PlayerSurvivor* pPlayerLeft = Player_GetSurvivor(ii);
					if (pPlayerLeft && pPlayerLeft->m_bIsHuman)
						nHumans++;
				}
				nHumans--;
				int iSound = 0;
				switch(nHumans)
				{
				case 6:
					iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_216");
					break;
				case 5:
					iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_217");
					break;
				case 4:
					iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_218");
				    break;
				case 3:
					iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_219");
				    break;
				case 2:
					iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_220");
					break;
				case 1:
					iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_221");
					break;
				}
								
				if(iSound)
					m_pWServer->Sound_Global(iSound);
			}

			pPlayerKilled->m_SpawnAsDarkling = true;
			if(m_iNextToBeDarkling == -1)
				m_iNextToBeDarkling = i;

			CheckForWinner();
			return 1;
		}
	}

	return 0;
}

int	CWObject_GameSurvivor::OnRemoveClient(int _iClient, int _Reason)
{
	CheckForWinner();
	return parent::OnRemoveClient(_iClient, _Reason);
}

void CWObject_GameSurvivor::CheckForWinner(void)
{
	CClientData *pCD = GetClientData();
	if(pCD->m_WarmUpMode != MP_WARMUP_MODE_NONE)
		return;
	int HumansLeft = 0;
	int iHuman = 0;
	for(int i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerSurvivor *pPlayer = Player_GetSurvivor(i);
		if(pPlayer && pPlayer->m_bIsHuman && !pPlayer->m_SpawnAsDarkling)
		{
			HumansLeft++;
			iHuman = i;
		}
	}

	if(HumansLeft == 1)
	{
		CWO_PlayerSurvivor *pPlayer = Player_GetSurvivor(iHuman);
		pPlayer->m_Score++;
#if defined(PLATFORM_CONSOLE)
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
		CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
		pGameMod->m_pMPHandler->StatisticsSetScore(pPlayer->m_Score, pPlayer->m_player_id);
#endif
		//RespawnAll();
		pCD->m_WarmUpCountDownTick = m_pWServer->GetGameTick();
		pCD->m_WarmUpMode = MP_WARMUP_MODE_BETWEEN_ROUNDS;
		pCD->m_LastRoundWinner.m_Value = pPlayer->m_Name;
		pCD->m_LastRoundWinner.MakeDirty();
	}
	else if(HumansLeft == 0)
	{	//Last human disconnected, or two humans died same tick?
		RespawnAll();
	}
}

void CWObject_GameSurvivor::RespawnAll(bool _bAlreadySpawned)
{
	for(int i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerSurvivor *pPlayer = Player_GetSurvivor(i);
		if(pPlayer && pPlayer->m_bJoined)
		{
			CMat4Dfp32 Mat = GetSpawnPosition();
			int16 Flags = 0;
			Player_Respawn(i, Mat, pPlayer->m_ClassHuman, Flags);
			pPlayer->m_SpawnAsDarkling = false;
		}
	}

	CWO_PlayerSurvivor *pPlayer = NULL;
	if(m_iNextToBeDarkling != -1)
		pPlayer = Player_GetSurvivor(m_iNextToBeDarkling);
	if(!pPlayer)
		pPlayer = Player_GetSurvivor(rand() % Player_GetNum());

	CWObject_Message Msg;
	Msg.m_Msg = OBJMSG_GAME_TOGGLEDARKLINGHUMAN;
	Msg.m_iSender = pPlayer->m_iPlayer;
	Msg.m_Param0 = 1;
	m_pWServer->Message_SendToObject(Msg, m_iObject);
	pPlayer->m_SpawnAsDarkling = true;

	m_iNextToBeDarkling = -1;

	parent::RespawnAll(true);
}

void CWObject_GameSurvivor::ToggleDarklingHuman(int16 _iSender, bool _bForce)
{
	if(_bForce)
		parent::ToggleDarklingHuman(_iSender, _bForce);
}

spCWO_Player CWObject_GameSurvivor::CreatePlayerObject()
{
	CWO_PlayerSurvivor *pP = MNew(CWO_PlayerSurvivor);
	spCWO_Player spP = (CWO_Player *)pP;
	if (!spP) MemError("CreateChar");

	return spP;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_GameLastHuman
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWObject_GameLastHuman::CWObject_GameLastHuman()
{
	M_TRACE("CWObject_GameLastHuman::CWObject_GameLastHuman\n");

	CClientData *pCD = GetClientData();
#if defined(PLATFORM_CONSOLE)
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
	if(pGameMod)
	{
		pCD->m_MaxTime = pGameMod->m_pMPHandler->m_TimeLimit;
		pCD->m_MaxScore = pGameMod->m_pMPHandler->m_ScoreLimit;
		pCD->m_GameModeName.m_Value = pGameMod->m_pMPHandler->GetGameModeName();
	}
#else
	pCD->m_MaxScore = 20;
	pCD->m_MaxTime = 600;
	pCD->m_GameModeName.m_Value = "§Z20 §LMENU_LASTHUMAN";
#endif
	pCD->m_GameModeName.MakeDirty();

	m_nNumPlayersNeeded = 2; 
	pCD->m_WarmUpMode = MP_WARMUP_MODE_PREGAME;
	pCD->m_WarmUpCountDownTick = -1;
	m_bDoOnce = true;
	m_iNextToBeHuman = -1;
	m_WeaponTemplate = "weapon_mp_SMG";
}

void CWObject_GameLastHuman::OnSpawnWorld()
{
	parent::OnSpawnWorld();

	spCRPG_Object spObj = CRPG_Object::CreateObject(m_WeaponTemplate, m_pWServer);
	m_spDummyObject = (CRPG_Object_Item *)(CRPG_Object *)spObj;
}

CWObject_GameLastHuman::CWO_PlayerLastHuman *CWObject_GameLastHuman::Player_GetLastHuman(int _iPlayer)
{
	return (CWO_PlayerLastHuman *)(CWO_Player *)m_lspPlayers[_iPlayer];
}


void CWObject_GameLastHuman::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	parent::OnIncludeClass(_pWData, _pWServer);

	_pWData->GetResourceIndex_Sound("D_Global_mpvoice_214");	//You are the Last Man standing
}

aint CWObject_GameLastHuman::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GAME_CANPICKUPSPAWN:
		{
			if(_Msg.m_Param0 & CWObject_Trigger_Pickup::PICKUP_SPAWNFLAG_LASTHUMAN)
				return 1;
			return 0;
		}
	case OBJMSG_GAME_TOGGLEDARKLINGHUMAN:
		{
			parent::OnMessage(_Msg);
			CWO_PlayerLastHuman *pPlayer = Player_GetLastHuman(_Msg.m_iSender);
			if(pPlayer && pPlayer->m_bIsHuman && _Msg.m_Param0)
				GiveHumanWeapons(_Msg.m_iSender);
		}
		return 1;

	case OBJMSG_GAME_GETTENSION:
		{
			//LastHuman always gets battle music
			for(int i = 0; i < Player_GetNum(); i++)
			{
				CWO_PlayerLastHuman* pPlayer = Player_GetLastHuman(i);
				if(pPlayer && pPlayer->m_iObject == _Msg.m_Param0 && pPlayer->m_bIsHuman)
				{
					return 255;
				}
			}
			break;
		}
	}
	return parent::OnMessage(_Msg);
}

int CWObject_GameLastHuman::OnClientCommand(int _iPlayer, const CCmd* _pCmd)
{
	int Size = _pCmd->m_Size;
	int Ret = parent::OnClientCommand(_iPlayer, _pCmd);
	switch(_pCmd->m_Cmd)
	{
	case CONTROL_JOINGAME:
		{
			CWO_PlayerSurvivor *pPlayer = Player_GetSurvivor(_iPlayer);
			if(pPlayer->m_bIsHuman)
			{
				GiveHumanWeapons(_iPlayer);
			}
			else
			{
				CWObject_Message Msg;
				Msg.m_Msg = OBJMSG_GAME_TOGGLEDARKLINGHUMAN;
				Msg.m_iSender = pPlayer->m_iPlayer;
				Msg.m_Param0 = 1;
				m_pWServer->Message_SendToObject(Msg, m_iObject);
			}
		}
		break;
	}
	return Ret;
}

int CWObject_GameLastHuman::OnClientConnected(int _iClient)
{
	if (!CWObject_GameP4::OnClientConnected(_iClient))
		return 0;

	CMat4Dfp32 Mat;
	int16 Flags = 0;
	CWServer_Mod* pServer = safe_cast<CWServer_Mod>(m_pWServer);
	M_ASSERT(pServer, "No server!");

	Mat = GetSpawnPosition();

	int PlayerObj = -1;
	int PlayerGUID = -1;
	CFStr Class;
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys->GetEnvironment()->GetValuei("FORCESPECTATOR"))
	{
		Class = "Spectator";
	}
	else
	{
		Class = "Spectator";

		//Open window to select team
//		OnSetClientWindow(_iClient, "multiplayer_selectcharacter_shapeshifters");
	}

	int iObj = Player_Respawn(Player_GetWithClient(_iClient)->m_iPlayer, Mat, Class, Flags, PlayerObj);
	if (iObj == -1)
	{
		ConOutL(CStrF("ERROR: Failed to spawn character %s", Class.Str()));
		return 0;
	}

	if (PlayerGUID != -1)
		pServer->Object_ChangeGUID(iObj, PlayerGUID);

	M_TRACE("CWObject_GameLastHuman::OnClientConnected");

	CClientData *pCD = GetClientData();
	bool bFoundEmptyPlayer = false;
	CWO_PlayerSurvivor *pPlayerClient = Player_GetSurvivor(_iClient);
	if(Player_GetNum() != 1)
		pPlayerClient->m_SpawnAsDarkling = true;

	for(int i = 0; i < pCD->m_lPlayerNames.Len(); i++)
	{	
		CWO_PlayerDM *pPlayer = Player_GetDM(i);
		if(pPlayer)
		{
			CNetMsg Msg2;
			Msg2.AddInt8(i);
			Msg2.AddStrAny(pPlayer->m_Name);
			CNetMsg Msg(WPACKET_OBJNETMSG);
			Msg.AddInt8(NETMSG_GAME_SETNAME);
			Msg.AddInt16(m_iObject);
			Msg.AddInt16(m_iClass);
			if (Msg.AddData((void*)Msg2.m_Data, Msg2.m_MsgSize))
				m_pWServer->Net_PutMsgEx(pPlayerClient->m_iPlayer, Msg, -1, WCLIENT_STATE_INGAME);

			if(!pPlayer->m_Name.Len())
				bFoundEmptyPlayer = true;
		}
	}

	if(!bFoundEmptyPlayer)
	{
		pCD->m_lPlayerScores.Add(0);
		pCD->m_lPlayerDeaths.Add(0);
		CStr EmptyName;
		pCD->m_lPlayerNames.Add(EmptyName);
	}

	pCD->m_lPlayerScores.MakeDirty();	//We must make it dirty so that the new player will get what we have
	pPlayerClient->m_Name = " ";

	while(pCD->m_lPlayerNames.Len() <= _iClient)
	{
		CStr EmptyName;
		pCD->m_lPlayerNames.Add(EmptyName);
	}

	while(pCD->m_lPlayerScores.Len() <= _iClient)
		pCD->m_lPlayerScores.Add(0);

	while(pCD->m_lPlayerDeaths.Len() <= _iClient)
		pCD->m_lPlayerDeaths.Add(0);

	pCD->m_MaxScore.MakeDirty();
	pCD->m_liMusic.MakeDirty();

	OnPlayerEntersGame(iObj, 0);

	return 1;
}


int CWObject_GameLastHuman::OnCharacterKilled(int _iObject, int _iSender)
{
	M_TRACE("CWObject_GameLastHuman::OnCharacterKilled, iObject=%d, iSender=%d\n", _iObject, _iSender);

	uint nPlayers = Player_GetNum();
	uint other_player = ~0;
	for(uint j = 0; j < nPlayers; j++)
	{
		CWO_Player* pPlayer = Player_Get(j);
		if (pPlayer && pPlayer->m_iObject == _iSender)
		{
			other_player = j;
			break;
		}
	}

	for (uint i = 0; i < nPlayers; i++)
	{
		CWO_Player* pPlayer = Player_Get(i);
		if (pPlayer && pPlayer->m_iObject == _iObject)
		{
			CWObject_CharShapeshifter* pPlayerObj = safe_cast<CWObject_CharShapeshifter>(m_pWServer->Object_Get(_iObject));
			CWO_CharShapeshifter_ClientData& CD = CWObject_CharShapeshifter::GetClientData(pPlayerObj);

			if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_DAMAGE)
			{
				CWObject_PickupInfo PickupInfo;
				PickupInfo.m_TemplateName = "object_mp_damage_boost";
				PickupInfo.m_Pos = pPlayerObj->GetPosition();
				PickupInfo.m_SpawnTick = m_pWServer->GetGameTick();
				PickupInfo.m_TicksLeft = CD.m_DamageBoostDuration - (m_pWServer->GetGameTick() - CD.m_DamageBoostStartTick);
				m_lPickups.Add(PickupInfo);

				int iDrainSoundInOut = m_pWServer->GetMapData()->GetResourceIndex_Sound("gam_drk_crp02");
				int iDrainLoopSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("Gam_Drk_crp03");

				if(iDrainSoundInOut)
					m_pWServer->Sound_At(pPlayerObj->GetPosition(), iDrainSoundInOut, WCLIENT_ATTENUATION_3D);

				if(iDrainLoopSound)
					m_pWServer->Sound_Off(pPlayerObj->m_iObject, iDrainLoopSound);

				CWObject_Message MsgDrain(OBJMSG_EFFECTSYSTEM_SETFADE, 2);
				m_pWServer->Message_SendToObject(MsgDrain, pPlayerObj->m_iDarkness_Drain_2);
			}
			if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_SPEED)
			{
				CWObject_PickupInfo PickupInfo;
				PickupInfo.m_TemplateName = "object_mp_speed_boost";
				PickupInfo.m_Pos = pPlayerObj->GetPosition();
				PickupInfo.m_SpawnTick = m_pWServer->GetGameTick();
				PickupInfo.m_TicksLeft = CD.m_SpeedBoostDuration - (m_pWServer->GetGameTick() - CD.m_SpeedBoostStartTick);
				m_lPickups.Add(PickupInfo);
			}
			if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_INVISIBILITY)
			{
				CWObject_PickupInfo PickupInfo;
				PickupInfo.m_TemplateName = "object_mp_invisibility";
				PickupInfo.m_Pos = pPlayerObj->GetPosition();
				PickupInfo.m_SpawnTick = m_pWServer->GetGameTick();
				PickupInfo.m_TicksLeft = CD.m_InvisibiltyBoostDuration - (m_pWServer->GetGameTick() - CD.m_InvisibiltyBoostStartTick);
				m_lPickups.Add(PickupInfo);

				pPlayerObj->ClientFlags() &= ~CWO_CLIENTFLAGS_INVISIBLE;
			}

			pPlayer->m_Mode = PLAYERMODE_DEAD;

			CWO_PlayerSurvivor *pPlayerKiller, *pPlayerKilled;
			pPlayerKiller = NULL;
			pPlayerKilled = NULL;

			CNetMsg Msg2;
			pPlayerKilled = Player_GetSurvivor(i);
			pPlayerKilled->m_DeathTick = m_pWServer->GetGameTick();
			pPlayerKilled->m_KillsInARow = 0;
			if(other_player != ~0)
			{
				pPlayerKiller = Player_GetSurvivor(other_player);

				if(!pPlayerKiller->m_bIsHuman && pPlayerKilled->m_bIsHuman || pPlayerKiller->m_bIsHuman && !pPlayerKilled->m_bIsHuman)
				{
					GivePoint(other_player, 1);
					AddKill(other_player);
					Msg2.m_MsgType = NETMSG_GAME_PLAYERKILL;
				}
			}
			else
			{	//suicide
				//				pPlayerKilled->m_Score--;
				Msg2.m_MsgType = NETMSG_GAME_PLAYERSUICIDE;
				GetClientData()->m_lPlayerScores[i] = pPlayerKilled->m_Score;
			}

			GetClientData()->m_lPlayerScores.MakeDirty();
			pPlayerKilled->m_Deaths++;
			GetClientData()->m_lPlayerDeaths[i] = pPlayerKilled->m_Deaths;

#if defined(PLATFORM_CONSOLE)
			MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
			CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

			if(other_player == ~0)
			{	//suicide
				//				pGameMod->m_pMPHandler->StatisticsSetScore(pPlayerKilled->m_Score, pPlayerKilled->m_player_id);
				//				pGameMod->m_pMPHandler->StatisticsSetDeaths(pPlayerKilled->m_Deaths, pPlayerKilled->m_player_id);
				Msg2.AddStrAny(pPlayerKilled->m_Name);
			}
			else
			{
				pGameMod->m_pMPHandler->StatisticsSetScore(pPlayerKiller->m_Score, pPlayerKiller->m_player_id);
				pGameMod->m_pMPHandler->StatisticsSetDeaths(pPlayerKilled->m_Deaths, pPlayerKilled->m_player_id);

				Msg2.AddStrAny(pPlayerKilled->m_Name);
				Msg2.AddStrAny(pPlayerKiller->m_Name);
			}
#else
			if(other_player == ~0)
			{	//suicide
				Msg2.AddStrAny(pPlayerKilled->m_Name);
			}
			else
			{
				Msg2.AddStrAny(pPlayerKilled->m_Name);
				Msg2.AddStrAny(pPlayerKiller->m_Name);
			}
#endif

			m_pWServer->NetMsg_SendToObject(Msg2, m_iObject);
			m_pWServer->Net_ClientCommand(i, "mp_setmultiplayerstatus(1)");

			pPlayerKilled->m_SpawnAsDarkling = true;

			if(pPlayerKiller && !pPlayerKiller->m_bIsHuman && pPlayerKilled->m_bIsHuman)
			{
				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("D_Global_mpvoice_214");
				if(iSound)
					m_pWServer->Sound_Global(iSound, 1.0f, pPlayerKiller->m_iPlayer);

				pPlayerKiller->m_SpawnAsDarkling = false;
				CWObject_Message Msg;
				Msg.m_Msg = OBJMSG_GAME_TOGGLEDARKLINGHUMAN;
				Msg.m_iSender = pPlayerKiller->m_iPlayer;
				Msg.m_Param0 = 1;
				m_pWServer->Message_SendToObject(Msg, m_iObject);

				if(m_iNextToBeHuman == -1)
					m_iNextToBeHuman = i;
			}

			CheckForWinner();
			return 1;
		}
	}

	return 0;
}

void CWObject_GameLastHuman::GiveHumanWeapons(int _iPlayer)
{
	CWO_PlayerSurvivor *pPlayer = Player_GetSurvivor(_iPlayer);
	if(pPlayer->m_bIsHuman && pPlayer->m_bJoined)
	{	//Make sure player has enough ammo
		int iNumWeapons = 0;
		CWObject *pObj = m_pWServer->Object_Get(pPlayer->m_iObject);
		if(pObj)
		{
			CWObject_Character *pChar = safe_cast<CWObject_Character>(pObj);
			CRPG_Object_Inventory* pWeaponInv = pChar->Char()->GetInventory(AG2_ITEMSLOT_WEAPONS);

			pWeaponInv->RemoveItemByIdentifier(pPlayer->m_iObject, 2, true);

			uint16 bDontAutoEquip = 0;

			CWObject_Message Msg(OBJMSG_RPG_AVAILABLEPICKUP, (m_spDummyObject->m_iItemType), aint((CRPG_Object_Item *)(CReferenceCount *)m_spDummyObject));
			Msg.m_Reason = bDontAutoEquip; 
			Msg.m_pData = (char *)m_WeaponTemplate;
			Msg.m_DataSize = m_WeaponTemplate.Len();
			Msg.m_iSender = m_iObject;

			m_pWServer->Message_SendToObject(Msg, pPlayer->m_iObject);
			m_pWServer->Message_SendToObject(Msg, pPlayer->m_iObject);
			m_pWServer->Message_SendToObject(Msg, pPlayer->m_iObject);
			m_pWServer->Message_SendToObject(Msg, pPlayer->m_iObject);

			CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pChar);
			CRPG_Object_Item* pNextItem = pWeaponInv->GetItemByIndex(pWeaponInv->GetNumItems() - 1);
			if (pNextItem)
			{
				CWAG2I_Context AGContext(this, m_pWServer, pCD->m_GameTime);
				pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_WEAPONIDENTIFIER, pNextItem->m_Identifier);
				if (pCD->m_AnimGraph2.SendAttackImpulse(&AGContext, CXRAG2_Impulse(AG2_IMPULSETYPE_ITEMACTION, AG2_IMPULSEVALUE_ITEMACTION_UNEQUIP)))
					pCD->m_AnimGraph2.SetPropertyBool(PROPERTY_BOOL_WEAPONSELECTQUEUED, true);
				pCD->m_WeaponUnequipTick = 0;
			}
		}
	}
}

void CWObject_GameLastHuman::OnRefresh(void)
{
	parent::OnRefresh();
	bool bhuman = false;
	for(int i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerSurvivor *pPlayer = Player_GetSurvivor(i);
		if(pPlayer && pPlayer->m_bIsHuman && !pPlayer->m_SpawnAsDarkling && pPlayer->m_bJoined)
		{	//Make sure player has enough ammo
			int iNumWeapons = 0;
			CWObject *pObj = m_pWServer->Object_Get(pPlayer->m_iObject);
			if(pObj)
			{
				CWObject_Character *pChar = safe_cast<CWObject_Character>(pObj);
				CRPG_Object_Inventory* pWeaponInv = pChar->Char()->GetInventory(AG2_ITEMSLOT_WEAPONS);

				for(int i = 0; i < pWeaponInv->GetNumItems(); i++)
				{
					CRPG_Object_Item* pWeapon = pWeaponInv->GetItemByIndex(i);
					if(pWeapon->m_iItemType == 1)
						iNumWeapons++;
				}

				if(iNumWeapons < 5)
				{
					CRPG_Object_Item* pCurItem = pWeaponInv->FindFinalSelectedItem();
					uint16 bDontAutoEquip = 1;
					if(pCurItem && pCurItem->m_iItemType == 1)
						bDontAutoEquip = 0;

					CWObject_Message Msg(OBJMSG_RPG_AVAILABLEPICKUP, (m_spDummyObject->m_iItemType), aint((CRPG_Object_Item *)(CReferenceCount *)m_spDummyObject));
					Msg.m_Reason = bDontAutoEquip; //Don't autoequip this item. 
					Msg.m_pData = (char *)m_WeaponTemplate;
					Msg.m_DataSize = m_WeaponTemplate.Len();
					Msg.m_iSender = m_iObject;

					m_pWServer->Message_SendToObject(Msg, pPlayer->m_iObject);
				}
			}
		}
		if(pPlayer && pPlayer->m_bIsHuman && !pPlayer->m_SpawnAsDarkling)
			bhuman = true;
	}

	if(!bhuman)
	{	//No human player, we must have one
		for(int i = 0; i < Player_GetNum(); i++)
		{
			CWO_PlayerSurvivor *pPlayer = Player_GetSurvivor(i);
			if(pPlayer && pPlayer->m_bJoined)
			{
				pPlayer->m_SpawnAsDarkling = false;
				CWObject_Message Msg;
				Msg.m_Msg = OBJMSG_GAME_TOGGLEDARKLINGHUMAN;
				Msg.m_iSender = pPlayer->m_iPlayer;
				Msg.m_Param0 = 1;
				m_pWServer->Message_SendToObject(Msg, m_iObject);

				GiveHumanWeapons(i);
				break;
			}
		}
	}
}

void CWObject_GameLastHuman::CheckForWinner(void)
{

}

void CWObject_GameLastHuman::RespawnAll(bool _bAlreadySpawned)
{
	int iHuman = -1;
	if(m_iNextToBeHuman != -1)
		iHuman = m_iNextToBeHuman;
	if(iHuman == -1)
		iHuman = rand() % Player_GetNum();
	for(int i = 0; i < Player_GetNum(); i++)
	{
		CWO_PlayerSurvivor *pPlayer = Player_GetSurvivor(i);
		if(pPlayer && pPlayer->m_bJoined)
		{
			CMat4Dfp32 Mat = GetSpawnPosition();
			int16 Flags = 0;
			Player_Respawn(i, Mat, pPlayer->m_ClassHuman, Flags);

			if(iHuman != i)
			{
				CWObject_Message Msg;
				Msg.m_Msg = OBJMSG_GAME_TOGGLEDARKLINGHUMAN;
				Msg.m_iSender = pPlayer->m_iPlayer;
				Msg.m_Param0 = 1;
				m_pWServer->Message_SendToObject(Msg, m_iObject);
				pPlayer->m_SpawnAsDarkling = true;
			}
			else
			{
				pPlayer->m_bIsHuman = true;
				pPlayer->m_SpawnAsDarkling = false;
				GiveHumanWeapons(i);
			}
		}
	}

	m_iNextToBeHuman = -1;

	CWObject_GameDM::RespawnAll(true);	
}

spCWO_Player CWObject_GameLastHuman::CreatePlayerObject()
{
	CWO_PlayerLastHuman *pP = MNew(CWO_PlayerLastHuman);
	spCWO_Player spP = (CWO_Player *)pP;
	if (!spP) MemError("CreateChar");

	return spP;
}

void CWObject_GameLastHuman::OnPickUp(int _iPlayer, int _iObject, int _TicksLeft)
{
	CWObject_Trigger_Pickup *pTriggerObject = safe_cast<CWObject_Trigger_Pickup>(m_pWServer->Object_Get(_iObject));
	if(pTriggerObject->m_Type == CWObject_Trigger_Pickup::PICKUP_WEAPON)
		return;
	parent::OnPickUp(_iPlayer, _iObject, _TicksLeft);
}



