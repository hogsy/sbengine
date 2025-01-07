#ifndef __WOBJ_GAMECORE_H
#define __WOBJ_GAMECORE_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_GameCore
					
	Author:			Jens Andersson
					
	Copyright:		
					
	Contents:		
					
	Comments:		
					
	History:		
		020808:		Created File
\*____________________________________________________________________________________________*/

#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Game.h"
#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AutoVar.h"
#include "../WObj_AutoVar_AttachModel.h"
#include "../WRPG/WRPGCore.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CWObject_GameCore
					
	Comments:		GameObject base-class with generic functiality:

					* Animation resource mapper
					* Screen fade support
					* Release controls command
					* Client window wrappers
					* Placeholder Picmip wrappers
					* Placeholder Tranzitsones

	TODO:			* Transitzone support
					* Include and precache cleanup
					* Picmip cleanup
\*____________________________________________________________________*/
class CWO_GameCameraOverlay
{
public:
	CWO_GameCameraOverlay() {}

	int8	m_Type;
	int32	m_StartTick;
	uint16	m_Duration;
	uint16	m_SurfaceID;
	uint32	m_WH;
};

class CWObject_GameCore : public CWObject_Game
{
	MRTC_DECLARE_SERIAL_WOBJECT;

public:
	CWObject_GameCore();
	~CWObject_GameCore();
	enum
	{
		GAME_CLIENTFLAGS_PENDINGGAMEMSG = (1 << CWO_CLIENTFLAGS_USERSHIFT),

		GAME_FLAGS_NOSOUNDVOLUMES = M_Bit(0),
		GAME_FLAGS_NOMUSIC = M_Bit(1),
		GAME_FLAGS_NOSOUNDEFFECTS = M_Bit(2),
		GAME_FLAGS_NOSOUNDAMBIENTS = M_Bit(3),
		GAME_FLAGS_NOSOUNDVOICE = M_Bit(4),
		GAME_FLAGS_IS_PBHD = M_Bit(5),			// set to true for Riddick HD  (ugly? yeah)

		PLAYER_STATUS_DEAD = 1,
		PLAYER_STATUS_TENSION = 2,
		PLAYER_STATUS_NOTPLAYERVIEW = 4,
		PLAYER_STATUS_PENDINGCUTSCENE = 8,

		MAXANIMFILES = 64,
		CLIENTOBJ_CLIENTDATA = 0,
		MAXMUSICRESOURCES = 7,
	};
	
	struct CClientData : public CReferenceCount, public CAutoVarContainer
	{
		AUTOVAR_SETCLASS(CClientData, CAutoVarContainer);

		CAUTOVAR(TAutoVar_StaticArray_(uint16, MAXANIMFILES), m_Anim_ResourceMapper, DIRTYMASK_0_0);
		CAUTOVAR_OP(CAutoVar_uint8, m_Flags, DIRTYMASK_0_1);
		CAUTOVAR_OP(CAutoVar_uint32, m_ScreenFade_StartTick, DIRTYMASK_0_2);
		CAUTOVAR_OP(CAutoVar_uint32, m_ScreenFade_SrcColor, DIRTYMASK_0_2);
		CAUTOVAR_OP(CAutoVar_uint32, m_ScreenFade_DestColor, DIRTYMASK_0_2);
		CAUTOVAR_OP(CAutoVar_int16, m_ScreenFade_Duration, DIRTYMASK_0_2);
		
		CAUTOVAR_OP(CAutoVar_int32, m_TensionReleaseTick, DIRTYMASK_0_6); 
		CAUTOVAR_OP(CAutoVar_int16, m_TensionReleaseDelayDuration, DIRTYMASK_0_6);
		CAUTOVAR_OP(CAutoVar_int32, m_BattleReleaseTick, DIRTYMASK_0_6); 
		CAUTOVAR_OP(CAutoVar_int16, m_BattleReleaseDelayDuration, DIRTYMASK_0_6);

		CAUTOVAR_OP(CAutoVar_uint32, m_SoundFade_StartTick, DIRTYMASK_0_3);
		CAUTOVAR_OP(CAutoVar_uint8, m_SoundFade_Src, DIRTYMASK_0_3);
		CAUTOVAR_OP(CAutoVar_uint8, m_SoundFade_Dest, DIRTYMASK_0_3);
		CAUTOVAR_OP(CAutoVar_int16, m_SoundFade_Duration, DIRTYMASK_0_3);
		CAUTOVAR(CAutoVar_CFStr, m_GameMessage_Text, DIRTYMASK_0_4);
		CAUTOVAR_OP(CAutoVar_uint32, m_GameMessage_StartTick, DIRTYMASK_0_5);
		CAUTOVAR_OP(CAutoVar_uint16, m_GameMessage_Duration, DIRTYMASK_0_5);
		CAUTOVAR_OP(CAutoVar_uint32, m_InfoScreen_StartTick, DIRTYMASK_0_5);
		CAUTOVAR_OP(CAutoVar_uint32, m_InfoScreen_Surface, DIRTYMASK_0_5);
		CAUTOVAR_OP(CAutoVar_uint8, m_InfoScreen_Type, DIRTYMASK_0_5);
		CAUTOVAR(TAutoVar_StaticArray_(uint16, MAXMUSICRESOURCES), m_liMusic, DIRTYMASK_0_6);
		CAUTOVAR(TAutoVar_StaticArray_(uint8, MAXMUSICRESOURCES), m_liMusicVolumes, DIRTYMASK_0_6);
		CAUTOVAR(TAutoVar_StaticArray_(fp32, MAXMUSICRESOURCES*2), m_lfMusicFadeSpeeds, DIRTYMASK_0_6);
		CAUTOVAR(TAutoVar_StaticArray_(fp32, MAXMUSICRESOURCES), m_liMusicVolumeModifier, DIRTYMASK_0_6);
	
		CAUTOVAR_OP(CAutoVar_uint16, m_GameSurface_Surface, DIRTYMASK_1_1);
		CAUTOVAR_OP(CAutoVar_uint32, m_GameSurface_StartTick, DIRTYMASK_1_1);
		CAUTOVAR_OP(CAutoVar_uint16, m_GameSurface_Duration, DIRTYMASK_1_1);

		CAUTOVAR_OP(CAutoVar_uint16, m_GameSurfaceOverlay_Surface, DIRTYMASK_1_1);
		CAUTOVAR_OP(CAutoVar_uint32, m_GameSurfaceOverlay_StartTick, DIRTYMASK_1_1);
		CAUTOVAR_OP(CAutoVar_uint16, m_GameSurfaceOverlay_Duration, DIRTYMASK_1_1);
		CAUTOVAR_OP(CAutoVar_int8,   m_GameSurfaceOverlay_Type, DIRTYMASK_1_1);
		CAUTOVAR_OP(CAutoVar_uint32, m_GameSurfaceOverlay_WH, DIRTYMASK_1_1);
		
		CAUTOVAR(TAutoVar_StaticArray_(int16, 8), m_lPlayerScores, DIRTYMASK_1_2);
		CAUTOVAR(TAutoVar_StaticArray_(int16, 8), m_lPlayerDeaths, DIRTYMASK_1_2);
		CAUTOVAR(TAutoVar_StaticArray_(uint8, 2), m_lCaptures, DIRTYMASK_1_2);
		CAUTOVAR(TAutoVar_StaticArray_(int8, 8), m_lPlayerTeams, DIRTYMASK_1_3);
		
		int32 m_FirstTension0Tick;

		struct CSubtitleInstance
		{
			CSubtitleInstance() {}
			CSubtitleInstance(int _iObject, fp32 _Range)
			{
				m_iObject = _iObject;
				m_RangeSqr = _Range * _Range;
			}

			int m_iObject;
			fp32 m_RangeSqr;
		};

		TArray<CSubtitleInstance> m_lCurSubtitles;
		bool m_bRenderWorld;

		TArray<CStr> m_lPlayerNames;

		struct SMultiplayerMessage 
		{
			uint8	m_Type;	//killed, team kill, uses the corresponding NETMSG_
			CStr	m_Str1;
			CStr	m_Str2;
			uint32	m_StartTick;
			int16	m_Duration;
		};

		TArray<SMultiplayerMessage> m_lMultiplayerMessages;

		struct SMultiplayerMessageImportant
		{
			uint8	m_Type;	//captured flag etc, uses the corresponding NETMSG_
			CStr	m_Str1;
			uint8	m_Team;
			uint32	m_StartTick;
			int16	m_Duration;
		};

		TArray<SMultiplayerMessageImportant> m_lMultiplayerMessagesImportant;

		// Move these to GameP4
		#define MAXMISSIONS 30
		CAUTOVAR(TAutoVar_StaticArray_(uint16, MAXMISSIONS), m_lMissionID, DIRTYMASK_1_4);
		CAUTOVAR(TAutoVar_StaticArray_(TFStr<48>, MAXMISSIONS), m_lMissionDesc, DIRTYMASK_1_4);
		CAUTOVAR(TAutoVar_StaticArray_(uint16, MAXMISSIONS), m_lMissionFlags, DIRTYMASK_1_4);
		CAUTOVAR_OP(CAutoVar_uint32, m_NVRange_StartTick, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_uint16, m_NVRange_Src, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_uint16, m_NVRange_Dest, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_int16, m_NVRange_Duration, DIRTYMASK_1_5);
		CAUTOVAR_OP(CAutoVar_fp32, m_VolumeMultiplier, DIRTYMASK_1_1);

		//These will be forced to dirty every time a new client connects
		CAUTOVAR_OP(CAutoVar_int8,	m_GameOver, DIRTYMASK_2_0);
		CAUTOVAR_OP(CAutoVar_int8,	m_MaxScore, DIRTYMASK_2_0);
		CAUTOVAR_OP(CAutoVar_int32,	m_MaxTime,	DIRTYMASK_2_0);
		CAUTOVAR_OP(CAutoVar_int32,	m_StartTick, DIRTYMASK_2_0);
		CAUTOVAR_OP(CAutoVar_int8, m_MaxCaptures, DIRTYMASK_2_0);
		CAUTOVAR_OP(CAutoVar_int32,	m_WarmUpCountDownTick, DIRTYMASK_2_0);
		CAUTOVAR_OP(CAutoVar_uint8, m_WarmUpMode, DIRTYMASK_2_0);
		CAUTOVAR(CAutoVar_CFStr, m_LastRoundWinner, DIRTYMASK_2_1);
		CAUTOVAR(CAutoVar_CFStr, m_GameModeName, DIRTYMASK_2_1);

		AUTOVAR_PACK_BEGIN
		AUTOVAR_PACK_VAR(m_Anim_ResourceMapper)
		AUTOVAR_PACK_VAR(m_Flags)
		AUTOVAR_PACK_VAR(m_ScreenFade_SrcColor)
		AUTOVAR_PACK_VAR(m_ScreenFade_DestColor)
		AUTOVAR_PACK_VAR(m_ScreenFade_StartTick)
		AUTOVAR_PACK_VAR(m_ScreenFade_Duration)
		AUTOVAR_PACK_VAR(m_SoundFade_StartTick)
		AUTOVAR_PACK_VAR(m_SoundFade_Src)
		AUTOVAR_PACK_VAR(m_SoundFade_Dest)
		AUTOVAR_PACK_VAR(m_SoundFade_Duration)
		
		AUTOVAR_PACK_VAR(m_TensionReleaseTick)
		AUTOVAR_PACK_VAR(m_TensionReleaseDelayDuration)
		AUTOVAR_PACK_VAR(m_BattleReleaseTick)
		AUTOVAR_PACK_VAR(m_BattleReleaseDelayDuration)

		AUTOVAR_PACK_VAR(m_GameMessage_Text)
		AUTOVAR_PACK_VAR(m_GameMessage_StartTick)
		AUTOVAR_PACK_VAR(m_GameMessage_Duration)
		AUTOVAR_PACK_VAR(m_GameSurface_Surface)
		AUTOVAR_PACK_VAR(m_GameSurface_StartTick)
		AUTOVAR_PACK_VAR(m_GameSurface_Duration)
		AUTOVAR_PACK_VAR(m_GameSurfaceOverlay_Surface)
		AUTOVAR_PACK_VAR(m_GameSurfaceOverlay_StartTick)
		AUTOVAR_PACK_VAR(m_GameSurfaceOverlay_Duration)
		AUTOVAR_PACK_VAR(m_GameSurfaceOverlay_Type)
		AUTOVAR_PACK_VAR(m_GameSurfaceOverlay_WH)
		AUTOVAR_PACK_VAR(m_InfoScreen_StartTick)
		AUTOVAR_PACK_VAR(m_InfoScreen_Surface)
		AUTOVAR_PACK_VAR(m_InfoScreen_Type)
		AUTOVAR_PACK_VAR(m_liMusic)
		AUTOVAR_PACK_VAR(m_liMusicVolumes)
		AUTOVAR_PACK_VAR(m_lfMusicFadeSpeeds)
		AUTOVAR_PACK_VAR(m_liMusicVolumeModifier)
		AUTOVAR_PACK_VAR(m_lPlayerScores)
		AUTOVAR_PACK_VAR(m_lPlayerDeaths)
		AUTOVAR_PACK_VAR(m_lCaptures)
//		AUTOVAR_PACK_VAR(m_lPlayerNames)
		AUTOVAR_PACK_VAR(m_lPlayerTeams)
		AUTOVAR_PACK_VAR(m_lMissionID)
		AUTOVAR_PACK_VAR(m_lMissionDesc)
		AUTOVAR_PACK_VAR(m_lMissionFlags)
		AUTOVAR_PACK_VAR(m_NVRange_StartTick)
		AUTOVAR_PACK_VAR(m_NVRange_Src)
		AUTOVAR_PACK_VAR(m_NVRange_Dest)
		AUTOVAR_PACK_VAR(m_NVRange_Duration)
		AUTOVAR_PACK_VAR(m_VolumeMultiplier)
		AUTOVAR_PACK_VAR(m_GameOver)
		AUTOVAR_PACK_VAR(m_MaxScore)
		AUTOVAR_PACK_VAR(m_MaxTime)
		AUTOVAR_PACK_VAR(m_StartTick)
		AUTOVAR_PACK_VAR(m_WarmUpCountDownTick)
		AUTOVAR_PACK_VAR(m_WarmUpMode)
		AUTOVAR_PACK_VAR(m_LastRoundWinner)
		AUTOVAR_PACK_VAR(m_MaxCaptures)
		AUTOVAR_PACK_VAR(m_GameModeName)
		AUTOVAR_PACK_END
			
		CClientData();
		uint32 GetCurScreenFade(int32 _GameTick, fp32 _TickFraction);
		uint8 GetCurSoundFade(int32 _GameTick, fp32 _TickFraction);
		uint16 GetCurNVRange(int32 _GameTick, fp32 _TickFraction);
	};

protected:
	static const CClientData *GetClientData(const CWObject_CoreData* _pObj);
	static CClientData *GetClientData(CWObject_CoreData* _pObj);
	CClientData *GetClientData();

	CStr m_Anim_ResourceMapperNames[MAXANIMFILES];
	CFStr m_LastWorldChangeID;
	CMat4Dfp32 m_LastWorldPos;
	CVec3Dfp32 m_LastWorldOffset;

	CFStr m_PendingWorld;
	int m_PendingWorldTick;
	int m_PendingPause;
	bool m_bWaitingForChangeWorldScript;
	CStr m_DefaultSpawn;					// name of default info_player_start object
	TArray<CStr> m_lAutoImpulse;			// list of comma-separated strings containing objects to impulse when level starts

	TArray<CStr> m_lGameMessages;
	TArray<fp32> m_lGameMessages_Duration;

	class CWObject_ScenePointManager* m_pScenePointManager;
	class CWO_RoomManager*       m_pRoomManager; 
	class CWO_ShellManager*      m_pShellManager;
	class CWO_SoundGroupManager* m_pSoundGroupManager;

	TArray<class CRagDollBpt>		m_lRagDollTypes;

protected:

	virtual void OnCreate();
	virtual void OnDestroy();

	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	virtual void OnRefresh();
	static void OnIncludeClass(CMapData*, CWorld_Server *_pWServer);
	static void OnClientPrecacheClass(CWorld_Client* _pWClient, CXR_Engine* _pEngine);
	
	int ReleaseControls(int _iClient);
	void ReleaseControlsAll();

	virtual int OnSetClientWindow(int _iClient, const char *_pWnd);
	CStr GetClientWindow(int _iClient);

	void SetMusic(const char *_pMusic);

	void ShowGameMsg(const char *_pSt, fp32 _Duration, bool _bFromQueue = false);
	void ShowGameSurface(int _iSurface, fp32 _Duration);
	void ShowInfoScreen(const char *_pSt, int _Type);
	void ShowGameSurfaceOverlay(const char* _pSt, fp32 _Duration);

	int FadeScreen(int _DstColor, fp32 _Duration, int _ExtraTickBeforeStart = 0);
	int FadeSound(int _Dest, fp32 _Duration, int _ExtraTickBeforeStart = 0);
	void SetPicmipOffsets(const char* _pPicmipOffsets);

	int SetNVRange(uint16 Range, fp32 _Duration, int _ExtraTickBeforeStart = 0);
	
	virtual int OnChangeWorld(const char *_pWorld, int _Flags, int _iSender);
	virtual int OnInitWorld();
	virtual int OnCloseWorld();
	virtual void OnSpawnWorld();
	virtual void OnSpawnWorld2();

	virtual int OnCharacterKilled(int _iObject, int _iSender) { return 1; }
	int GetPlayerStatus(int _iPlayer = 0);

	void IncludeAllPlayers();
	void IncludeAllAIs();

	int OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const;
	static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void OnClientRenderStatusBar(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D, CVec2Dfp32 *_rect = NULL);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	
	CMat4Dfp32 GetSpawnPosition(int _Flags = 0, const char *_pName = NULL, int16 *_pRetFlags = NULL, int16 *_pRetObj = NULL);
	virtual CStr GetDefualtSpawnClass() { return ""; }
	void OnPlayerEntersGame(int _iPlayerObj, int _iPlayerStartObj);
	
	void SendForceEndTimeleapMessage();

	int ResolveAnimHandle(const char *_pSequence);
	static spCXR_Anim_SequenceData GetAnimFromHandle(CWObject_CoreData *_pObj, CWorld_PhysState* _pWPhysState, int _Handle);

	virtual void OnDeltaSave(CCFile *_pFile);
	virtual void OnDeltaLoad(CCFile *_pFile, int _Flags);
	virtual void OnFinishDeltaLoad();
	static void OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);
	static void OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags);

public:
	static CRct CalcInfoRectWidth(CRC_Util2D *_pUtil2D, wchar *_pTitle = NULL, CRC_Font *_pTitleFont = NULL, wchar *_pText = NULL, CRC_Font *_pFont = NULL, int _iSurfRes = 0, int _SurfaceWidth = 0, int _SurfaceHeight = 0);
	static void DrawInfoRect(CRC_Util2D *_pUtil2D, CWorld_Client *_pWClient, const CRct &_Rct, const wchar *_pTitle = NULL, CRC_Font *_pTitleFont = NULL, const wchar *_pText = NULL, CRC_Font *_pFont = NULL, int _iSurfRes = 0, int _SurfaceWidth = 0, int _SurfaceHeight = 0, int _TitleExtraHeight = 0, int _TextCol = 0, int _Color1 = 0, int _Color2 = 0);
	static void DrawFocusFrame(CRC_Util2D *_pUtil2D, CWorld_Client *_pWClient);

	enum { NameHashDefualt = MHASH2('DEFA','ULT') };
	static bool CreateRagdoll(const CWObject_GameCore *_pGame, CWorld_Server *_pServer,class CWPhys_Cluster *_pPC,
		uint8 *_piMat,uint32 _iObj,TThinArray<CMat4Dfp32> &_lInvMat,uint32 _NameHash = NameHashDefualt);

	static bool m_sbRenderSkipText;
	int32 m_iDummyPlayer;
};

#endif
