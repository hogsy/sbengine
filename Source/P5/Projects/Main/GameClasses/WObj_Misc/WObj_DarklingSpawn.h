/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_DarklingSpawn.h

	Author:			Olle Rosenquist

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_DarklingSpawn

	Comments:		

	History:		
		0511xx:		Created File
\*____________________________________________________________________________________________*/
#ifndef __WOBJ_DARKLINGSPAWN_H
#define __WOBJ_DARKLINGSPAWN_H

#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Lights.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_System.h"
#define DARKLINGSPAWN_MAX_LIGHTINTENSITY	0.3f
#define DARKLINGSPAWN_CENTER_TO_CHOICE_OFFSET 10.0f


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObj_ObjLightMeter
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObj_ObjLightMeter : public CReferenceCount
{
	enum
	{
		MaxLights			= 8,
		MaxMoveThreshold	= 5,
	};

public:
	//CWObj_LightMeter(CAI_Core* _pAI);
	CWObj_ObjLightMeter();
	~CWObj_ObjLightMeter();
	
	// Initialize the light meters last measure tick and the measure interval
	void Init(CWorld_Server* _pWServer, const fp32& _MeasureInterval);

	// Measure() measures the light intersecting a box at _Pos with dimensions _Width, _Height
	// The _nLights tells us the number of lights to sample this call (-1 will sample all lights)
	// Returns true if all lights have been sampled at least once and false if not
	//fp32 MeasureFakePointLight(const CVec3Dfp32& _BoxPos, const CVec3Dfp32& _MeasurePos, fp32 _Width, fp32 _Height, const CXR_Light& _Light, const bool _TraceLine, const int _iExclude0, const int _iExclude1);
	fp32 MeasureFakePointLight(CWorld_Server* _pWServer, const CVec3Dfp32& _BoxPos, const CVec3Dfp32& _MeasurePos, fp32 _Width, fp32 _Height, const CXR_Light& _Light, const bool _TraceLine, const int _iExclude0, const int _iExclude1);
	bool Measure(CWorld_Server* _pWServer, const bool& _bConsiderLightmap, const CVec3Dfp32& _BoxPos, const CVec3Dfp32& _MeasurePos, fp32 _Width, fp32 _Height, int _nLights, bool _TraceLines, int _iSkipObj);
	//bool Measure(const CVec3Dfp32& _BoxPos,const CVec3Dfp32& _MeasurePos,fp32 _Width,fp32 _Height,int _nLights,bool _bTraceLines,int _iSkipObj);
	
	// Returns the current light intensity (this may be too dark/light if the last Measure() didn't update)
	fp32 GetIntensity(int _iObj);
	fp32 GetLightmapIntensity() {return(m_LightMapIntensity);}
	fp32 GetIntensityDelta() {return(m_LightIntensity - m_PrevLightIntensity);}
	
	// Create a measure in a specific tick
	void MakeMeasure(const int& _MeasureTick);

	// Returns true if the current itensity value is valid ie all lights have been measured for the current position.
	//bool IntensityValid();
	int	m_LastMeasureTick;								// Last time we measured intensity
	int m_MeasureInterval;								// Ticks between each measure
	
protected:
	// Updates the m_LightIntensity and m_bIntensityValid flags
	void UpdateIntensity(const bool& _bConsiderLightmap);

	fp32 m_Slop;
	fp32 m_LightIntensity;								// Light measurement from 0 to 1.0
	fp32 m_PrevLightIntensity;							// Light measurement from 0 to 1.0
	int16 m_iOwnerFlashLight;							// Who is the owner of the flash
	fp32 m_FlashlightIntensity;							// Intensity of flash
	int16 m_nLights;									// Current number of valid lights in m_lpLights
	const CXR_Light *m_lpLights[MaxLights];
	fp32 m_lpLightIntensities[MaxLights];				// Intensities of lights in m_lpLights
	fp32 m_LightMapIntensity;
	bool m_bIntensityValid;								// Have all lights been measured
	CVec3Dfp32 m_Pos;									// Current position
};
typedef TPtr<CWObj_ObjLightMeter>	spCWObj_ObjLightMeter;



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_DarklingSpawn
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum
{
	OBJMSG_DARKLINGSPAWN_DOSPAWNCHECK = OBJMSGBASE_MISC_DARKLINGSPAWN,
	OBJMSG_DARKLINGSPAWN_ISCLOSESPAWN,
	OBJMSG_DARKLINGSPAWN_SENDDARKLINGSPAWN,
	OBJMSG_DARKLINGSPAWN_SENDENTERNEARBY,
	OBJMSG_DARKLINGSPAWN_SENDLEAVENEARBY,
	OBJMSG_DARKLINGSPAWN_SENDENTERACTIVE,
	OBJMSG_DARKLINGSPAWN_SENDLEAVEACTIVE,
	OBJMSG_DARKLINGSPAWN_SENDTURNACTIVE,
	OBJMSG_DARKLINGSPAWN_SENDLEAVENEARBYSOUNDS,
	OBJMSG_DARKLINGSPAWN_SENDDARKLIGHTS,
	OBJMSG_DARKLINGSPAWN_TOUCHEFFECT,
};

class CWObject_DarklingSpawn : public CWObject_Model
{
	typedef CWObject_Model CWObject_DarklingSpawn_Parent;
	typedef TArray<CStr> SpawnStrArray;
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	enum
	{
		// Data0 - Bit 0-11  = Flags
		DARKLINGSPAWN_FLAGS_DISABLED = M_Bit(0),
		DARKLINGSPAWN_FLAGS_DISABLEONUSE = M_Bit(1),
		DARKLINGSPAWN_FLAGS_DISABLERENDER = M_Bit(2),
		DARKLINGSPAWN_FLAGS_WAITSPAWNED = M_Bit(3),
		DARKLINGSPAWN_FLAGS_CREATEWALLMARK = M_Bit(4),	// Recreate wallmark / Force new wallmark creation
		DARKLINGSPAWN_FLAGS_DESTROYWALLMARK = M_Bit(5),	// Destroy wallmark
		DARKLINGSPAWN_FLAGS_GOTCHARACTERINFO = M_Bit(6),	// If this flag isn't set, refresh until we know if we're gonna be disabled or not
		DARKLINGSPAWN_FLAGS_SHOULDBEENABLED = M_Bit(7),	// If this flag is set this spawnpoint should be set to enabled when character gets the darklingtype
		DARKLINGSPAWN_FLAGS_ALWAYSENABLED = M_Bit(8),	// If this flag is set this spawnpoint can always be used
		DARKLINGSPAWN_FLAGS_INACTIVE = M_Bit(9),			// If this flag is set this spawnpoint can always be used
		DARKLINGSPAWN_FLAGS_USELIGHTMAPS = M_Bit(10),	// Ignore lightmap values
		DARKLINGSPAWN_FLAGS_LAMPSMUSTBEDESTROYED = M_Bit(11),	// All linked lights must be destroyed before being active
		
		// Effectmode masks
		DARKLINGSPAWN_MODE_NEARBY = M_Bit(0),
		DARKLINGSPAWN_MODE_ACTIVE = M_Bit(1),

		// Data0 - Bit 12-19 = Wallmark Tolerance
		DARKLINGSPAWN_WALLMARK_TOLERANCEMASK = 0xFF,

		// Data0 - Bit 20-31 = Wallmark Size
		DARKLINGSPAWN_WALLMARK_SIZEMASK = 0xFFF,

		// Data2
		DARKLINGSPAWN_MODEL_USEVARIATION = M_Bit(0),

		// Data5
		DARKLINGSPAWN_FLAGS2_BERSERKER = M_Bit(0),
		DARKLINGSPAWN_FLAGS2_LIGHTKILLER = M_Bit(1),  
		DARKLINGSPAWN_FLAGS2_KAMIKAZE = M_Bit(2),
		DARKLINGSPAWN_FLAGS2_GUNNER = M_Bit(3),

		DARKLINGSPAWN_SPAWNSTRING_SLOT_LINKEDLAMPS = 0,
		DARKLINGSPAWN_SPAWNSTRING_SLOT_DARKLINGEFFECTS,
		DARKLINGSPAWN_SPAWNSTRING_SLOT_PREFABOBJECTS,
		DARKLINGSPAWN_SPAWNSTRING_SLOT_NUMSPAWNSTRINGS,
	};

	class CSimpleMessageDT : public CWO_SimpleMessage
	{
	public:
		uint m_TypeMask;

		CSimpleMessageDT()
			: CWO_SimpleMessage()
			, m_TypeMask(~0) { }

		CSimpleMessageDT(const CSimpleMessageDT& _x)
			: CWO_SimpleMessage(_x)
			, m_TypeMask(_x.m_TypeMask)
		{ }

		CSimpleMessageDT& operator= (const CSimpleMessageDT& _x)
		{
			CWO_SimpleMessage::operator=(_x);
			m_TypeMask = _x;
			return *this;
		}

		void Parse(CStr _St, CWorld_Server* _pWServer)
		{
			m_TypeMask = ~0;
			if (_St.CompareSubStr("0x") != 0)
			{
				static const char* s_lpTypes[] = { "B", "L", "K", "G", NULL };
				m_TypeMask = _St.GetStrSep(";").TranslateFlags(s_lpTypes);
			}
			CWO_SimpleMessage::Parse(_St, _pWServer);
		}
	};

protected:
	// Spawned darklings, is cleared in onspawnworld
	static TArray<int32> m_slSpawnedDarklings;	
	
	//int m_DarklingSpawnFlags;
	TArray<CSimpleMessageDT> m_lMsg_OnDarklingSpawn;		// Messages sent on spawn
	TArray<CWO_SimpleMessage> m_lMsg_OnEnterNearby;			// Messages sent every time player enters range of an inactive spawn point
	TArray<CWO_SimpleMessage> m_lMsg_OnLeaveNearby;			// Messages sent every time player leaves range of an inactive spawn point
	TArray<CWO_SimpleMessage> m_lMsg_OnEnterActive;			// Messages sent every time player enters range of an active spawn point
	TArray<CWO_SimpleMessage> m_lMsg_OnLeaveActive;			// Messages sent every time player leaves range of an active spawn point
	TArray<CWO_SimpleMessage> m_lMsg_OnTurnActive;			// Messages sent every time player goes from inactive to active
	struct sPrefabIndices
	{
		int16 m_iObject;
		int16 m_iPrefab;
	};
	TArray<struct sPrefabIndices>		m_lDarklingSpawnEffects;		// Darklingspawn effects, contains nearby/active/turnactive models
	/*TArray<int16>			m_lDarklingEffectsNearby;		// Effects that's used every time player enters range of an inactive spawn point
	TArray<int16>			m_lDarklingEffectsActive;		// Effects that's used every time player enters range of an active spawn point
	TArray<int16>			m_lDarklingEffectsTurnActive;	// Effects that's used every time player goes from inactive to active*/
	TArray<sPrefabIndices>	m_liLinkedLamps;		// Lamps that we're linked with
	TThinArray<SpawnStrArray> m_lSpawnStrings;		// Strings that are used during spawn phase
	//CWO_Light_MessageData	m_LightFlicker;			// What flicker linked lights should have
	spCWObj_ObjLightMeter	m_spLightMeter;			// Light meter checking objects light intensity
	spCRegistry				m_spDarklingExtraKeys;	// Extra keys for darkling spawn...
	CStr					m_SpawnEffect;			// Effect played on spawn itself
	// class CWObject_Room*	m_pCurrentRoom;			// Which room this spawn point is in (if any..)
//	CStr					m_DarklingSpawnType;	// Type of darkling to spawn
	int32					m_DarklingSpawnTypeChoice;	// Berserker, Gunner, Lightkiller or Gunner
	int32					m_DarklingSpawnTypeDefault;	// The default spawn type choice; this is used if spawning due to script
	CStr					m_DarklingSpawnName;	// Name of the darkling to spawn

	fp32						m_UsableRadius;			// In what range this darklingspawnpoint can be used
	fp32						m_LastIntensity;		// Last checkde intensity
	
	fp32						m_LightThreshold;		// How dark it has to be for spawn to work
	int32					m_TickNoRefresh;		// Which tick to switch of no refresh
	int32					m_TickDestroyWallmark;	// Which tick to destroy our wallmark if any
	int32					m_TickSpawnDarkling;	// Which tick we should actually spawn the darkling
	int16					m_iSoundActive;			// Looping sound played when spawnpoint is active
	int16					m_iSoundTurnActive;		// Sound played when spawnpoint turns active
	int16					m_iSoundNearby;			// Looping sound played when spawnpoint is in "nearby" mode
	int16					m_iPlayerInRange;		// Player that's in range
	uint8					m_ModeFlags;			// Spawnmode flags
	bool					m_bCanSpawn;
	int32					m_iDarkling;			

	fp32 MeasureLightIntensity(bool _bForce = false);
	void PrecacheDarkling(const CStr& _Darkling);
	bool CheckCanPlayerSpawn(int32 _iPlayer = -1);
	void SetEnabled(bool _bEnabled);
	void UpdateSpawnEffects(bool _bOn);
	bool CheckPrefabLights();
	// Causes darkling to be spawned next refresh
	void SyncDarklingSpawn(int32 _DarklingSpawnTypeChoice);
public:
	virtual void OnCreate();
	virtual void OnDestroy();
	virtual void OnRefresh();
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server*);
	static void OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer);
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
	virtual void OnSpawnWorld();
	virtual void OnSpawnWorld2();
	virtual aint OnMessage(const CWObject_Message &_Msg);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine*, const CMat4Dfp32& _ParentMat);
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	static void GetSpawnEffectDirections(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, int _iObj, CVec3Dfp32 &_Forward, CVec3Dfp32 &_Right);
//	CStr GetDarklingSpawnType() { return m_DarklingSpawnType; }

	//virtual void OnDeltaSave(CCFile* _pFile);
	//virtual void OnDeltaLoad(CCFile* _pFile, int _Flags);
};

// Just a normal model that can be disabled
class CWObject_DarklingSpawnEffect : public CWObject_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;
	typedef TArray<CStr> SpawnStrArray;
protected:
	class CDarklingSpawnEffectClientData : public CReferenceCount
	{
	public:
		int32 m_StartTick[3];
		int32 m_EndTick[3];
		int32 m_Offset[3];
		int32 m_RealData;
		CDarklingSpawnEffectClientData()
		{
			m_RealData = 0;
			for (int32 i = 0; i < 3; i++)
			{
				m_StartTick[i] = 0;
				m_EndTick[i] = 0;
				m_Offset[i] = 0;
			}
		}
	};
	class CSoundHelper
	{
	public:
		CVec3Dfp32 m_Offset;
		int m_iRealSound[2];

		int m_RandomSound_iSound[CWObject_Sound::MAXRANDOMSOUNDS];
		int m_RandomSound_MaxTick[CWObject_Sound::MAXRANDOMSOUNDS];
		int m_RandomSound_MinTick[CWObject_Sound::MAXRANDOMSOUNDS];
		int m_RandomSound_NextTick[CWObject_Sound::MAXRANDOMSOUNDS];
		bool m_bWaitSpawn;
		bool m_bRandom;

		CSoundHelper();
		void Init(CWObject_Sound* _pSound, CWorld_Server* _pWServer, CWObject_DarklingSpawnEffect* _pEffect);
		void Spawn(CWorld_Server* _pWServer, CWObject_DarklingSpawnEffect* _pEffect);
		void UnSpawn(CWorld_Server* _pWServer, CWObject_DarklingSpawnEffect* _pEffect);
		void OnRefresh(CWorld_Server* _pWServer, CWObject_DarklingSpawnEffect* _pEffect);
	};
	TArray<CSoundHelper> m_lSounds;
	TArray<CWO_SimpleMessage> m_lMsg_OnDarklingSpawn;		// Messages sent on spawn
	TArray<CWO_SimpleMessage> m_lMsg_OnEnterNearby;			// Messages sent every time player enters range of an inactive spawn point
	TArray<CWO_SimpleMessage> m_lMsg_OnLeaveNearby;			// Messages sent every time player leaves range of an inactive spawn point
	TArray<CWO_SimpleMessage> m_lMsg_OnEnterActive;			// Messages sent every time player enters range of an active spawn point
	TArray<CWO_SimpleMessage> m_lMsg_OnLeaveActive;			// Messages sent every time player leaves range of an active spawn point
	TArray<CWO_SimpleMessage> m_lMsg_OnTurnActive;			// Messages sent every time player goes from inactive to active
	TArray<int16>			m_liLinkedLampsDark;			// Lamps that we're linked with that should be on when lamp is turned off
	TArray<int16>			m_liLinkedLampsTurnActive;		// Lamps that we're linked with that should flash for a when turning active
	TThinArray<SpawnStrArray> m_lSpawnStrings;				// Strings that are used during spawn phase
	int32 m_TurnActiveTick;
	int32 m_DurationNearby;
	int32 m_DurationActive;
	int32 m_DurationTurnActive;
	int32 m_Flags;
	int16 m_NumSoundActive;
	int16 m_NumSoundTurnActive;
	int16 m_NumSoundNearby;
	int16 m_iSoundActive;
	int16 m_iSoundTurnActive;
	int16 m_iSoundNearby;
	bool m_bGotTouched;
public:
	enum
	{
		DARKLINGSPAWNEFFECT_FLAG_NEARBYENABLED = M_Bit(0),
		DARKLINGSPAWNEFFECT_FLAG_ACTIVEENABLED = M_Bit(1),
		DARKLINGSPAWNEFFECT_FLAG_TURNACTIVEENABLED = M_Bit(2),
		DARKLINGSPAWNEFFECT_FLAG_WAITSPAWN		= M_Bit(3),

		DARKLINGSPAWNEFFECT_IMPULSE_DISABLENEARBY = -1,
		DARKLINGSPAWNEFFECT_IMPULSE_ENABLENEARBY = 1,
		DARKLINGSPAWNEFFECT_IMPULSE_DISABLEACTIVE = -2,
		DARKLINGSPAWNEFFECT_IMPULSE_ENABLEACTIVE = 2,
		DARKLINGSPAWNEFFECT_IMPULSE_DISABLETURNACTIVE = -3,
		DARKLINGSPAWNEFFECT_IMPULSE_ENABLETURNACTIVE = 3,
	};
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnCreate();
	virtual void OnSpawnWorld();
	virtual void OnSpawnWorld2();
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message &_Msg);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWorld, CXR_Engine*, const CMat4Dfp32& _ParentMat);
	static CDarklingSpawnEffectClientData* GetClientData(CWObject_CoreData* _pObj);
	static const CDarklingSpawnEffectClientData* GetClientData(const CWObject_CoreData* _pObj);
	//int OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const;
	static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);
};
#endif
