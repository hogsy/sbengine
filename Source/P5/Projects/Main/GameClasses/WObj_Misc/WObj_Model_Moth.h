#ifndef _INC_WOBJ_MODEL_MOTH
#define _INC_WOBJ_MODEL_MOTH

#include "../../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_System.h"
#include "../WObj_Weapons/WObj_Spells.h"
#include "../WObj_Messages.h"

#define CWOBJECT_MODEL_MOTH_SINRAND_TABLESIZE		(2048)
// How many updates between retraces.
#define CWOBJECT_MODEL_MOTH_TRACEDELAY				(5)

#define RPG_ITEMTYPE_MOTH						(0x125)
enum 
{
	OBJMSG_MOTH_SETSHADOW = OBJMSGBASE_MISC_MOTH,
	OBJMSG_MOTH_TOGGLESHADOW,
};

class CWObject_Model_Moth : public CWObject_Ext_Model
{
	MRTC_DECLARE_SERIAL_WOBJECT;
protected:
	enum AIState
	{
		AIState_Flying		= 0x01,
		AIState_Land		= 0x02,
		AIState_Depart		= 0x03,
		AIState_Landed		= 0x04,
		AIState_Smashed		= 0x05,
		AIState_Escape		= 0x06,
	};

	enum
	{
		MOTH_OPTION_NOSHADOW = 1 << 0,
	};

	struct SurfaceInfo
	{
		CVec3Dfp32	m_Pos;
		CVec3Dfp32	m_Normal;
		fp32			m_Distance;
		int			m_iObject;
		bool		m_CanLand;
	};

	// Make butterfly server side (For testing )
	fp32					m_LifeTime;

	CVec3Dfp32			m_OldPos; // Last position where new velocityvector was determined.
	CVec3Dfp32			m_VelocityVector; // Main direction and velocity until next retrace.

	CVec3Dfp32			m_LastPosNoise, m_LastDirNoise; // Only used for lowpass filters.
	CVec3Dfp32			m_LastDir; // Only used for lowpass filters.

	bool				m_Avoid; // Flag that tells whether there exists a delayed avoidvector or not.
	CVec3Dfp32			m_AvoidVector; // Proximity avoid vector delayed from last retrace.

	int					m_TraceDelay; // Delay (updates) until a new retrace is allowed.
	int					m_IntersectDelay; // Delays (updates) until a new intersection probe is allowed.
	int					m_AltitudeDelay; // Delays (TRACEDELAY updates) until a new altitude retrace is allowed.
	int					m_AltitudeDelayValue;
	
	int					m_MaxMovementScans; // How many retraces that are allowed per velocity update.
	fp32					m_TraceProximityRange; // Length of traced ray.
	fp32					m_IntersectProximityRange; // Radius of intersection primitive.
	fp32					m_CloseAvoidFactor; // Surface normal avoid factor, used to avoid close surfaces.
	fp32					m_DistantAvoidFactor; // Surface normal avoid factor, used to avoid distant surfaces (delayed).
	fp32					m_MinSpeed; // Used as preferred min velocity, speed up to reach it.
	fp32					m_MaxSpeed, m_InvMaxSpeed; // Used as max velocity, clamp velocity vector to it.
	fp32					m_MinAcceleration; // Used for noising velocity vector before retracing
	fp32					m_MaxAcceleration; // Used for noising velocity vector before retracing.
	fp32					m_AccelerationSpeedDependency;
	fp32					m_FreespaceAcceleration; // Used to speedup velocity when it's free of obstacles but below MIN_VELOCITY.
	fp32					m_MinDescendSlope; // Used to determine if extra descent acceleration is needed.
	fp32					m_DescendAcceleration; // Used to descent velocity vector, to faster reach landing ground.
	fp32					m_MaxSlope; // Maximum z part of velocity vector.
	fp32					m_FlattenAcceleration; // Used to flatten z part of velocity vector if z slope is to high.
	fp32					m_FlattenFactor; // Used to flatten z part of velocity vector if z slope is to high.
	fp32					m_LandPositionOffset; // Used to offset landing position out from surface along surface normal.
	fp32					m_MaxLandingDistance; // Butterfly can't land on surfaces further than this distance away.
	fp32					m_MaxLandingSlope; // Maximum z part of a landable surface' normal.
	fp32					m_DepartPositionOffset; // Used to offset start fly position from land position.

	bool				m_NoiseMovement; // Wether to add noise to position and direction.
	fp32					m_MovementNoiseTimeScale;
	fp32					m_PositionNoise; // Amount of position noise.
	fp32					m_DirectionNoise; // Amount of direction noise.

	bool				m_FilterDirection; // Whether to apply direction lowpass filter or not.
	fp32					m_DirectionFilterLeak; // Amounth of new direction that gets through lowpass filter.

	bool				m_LimitFacingSlope; // Flag that tell whether to apply facing slope limitation or not.
	fp32					m_MaxFacingSlope; // Only used to limit model facing slope, not the velocity slope.

	fp32					m_MinFlyingDuration;
	fp32					m_MaxFlyingDuration;
	fp32					m_MinLandedDuration;
	fp32					m_MaxLandedDuration;
	fp32					m_MinLandDuration;
	fp32					m_MaxLandDuration;
	fp32					m_MinDepartDuration;
	fp32					m_MaxDepartDuration;

	fp32					m_FlyingFlutterFreq; // How many flutter periods per second while flying.
	fp32					m_MorphFlutterFreq; // How many flutter periods per second while landing or departing.
	fp32					m_LandedFlutterFreq; // How many flutter periods per second while landed.
	fp32					m_LandedWingSpreadDuration;

	bool				m_CorrectAltitude; // Flag that tell whether to adjust towards wanted altitude or not.
	fp32					m_WantedAltitude; // Altitude to ajust towards.
	fp32					m_AltitudeAcceleration; // Altitude adjustment acceleration.

	bool				m_ApplyFlutterAscention;
	fp32					m_FlutterAscention;


	fp32					m_WingStateTime; // State/Time of wing flutter.
	fp32					m_WingStateFreq; // Current wing flutter rate.

	AIState				m_AIState; // The current state of the butterfly AI.
	fp32					m_StateTime; // Amount of elapsed time of current AIState.
	fp32					m_WantedStateTime; // For how long the current AIState is wanted to hold.

	fp32					m_ModelSize;
	
	fp32					m_TimeScale;

	CVec3Dfp32			m_LandPos; // Landing position
	CVec3Dfp32			m_LandDir; // Landing position
	CVec3Dfp32			m_LandNormal; // Normal of surface at landing position.

	int32				m_Randseed;

	static	fp32			ms_SinRandTable[CWOBJECT_MODEL_MOTH_SINRAND_TABLESIZE];
	static	bool		ms_SinRandTableInitialized;
	uint8				m_Options;

	static	void CreateSinRandTable();
	static	fp32	GetSinRandTable(fp32 _Time, fp32 _Timescale);
	static	fp32	GetSinRandTableSlope(fp32 _Time, fp32 _Timescale);
	inline fp32 GetRand(){ return MFloat_GetRand(m_Randseed++); }

	CVec3Dfp32 InterpolateVector(const CVec3Dfp32& _V1, const CVec3Dfp32& _V2, const fp32& _Scale);

	void SetAIState(AIState _State);
	bool TraceRay(CVec3Dfp32 _RayOrigin, CVec3Dfp32 _RayDir, fp32 _RayLength, SurfaceInfo& _SurfaceInfo);
	bool CheckIntersection();
	void SetTransformation(CVec3Dfp32 _Pos, CVec3Dfp32 _Dir, CVec3Dfp32 _Up);
	//void UpdateWingParams();
	static fp32 GetWingAnimTime(fp32 _WingStateTime, fp32 _WingStateFreq, fp32 _TimeScale, 
						CWObject_Model_Moth::AIState _AIState, fp32 _LandedWingSpreadDuration,
						fp32 _Interpolated);
	fp32 GetWingAnimTime(fp32 _Interpolated);
	void MorphLandedState(fp32 _Morph, CVec3Dfp32& _Pos, CVec3Dfp32& _Dir, CVec3Dfp32& _Up);

	void SetDefaultValues();

	void SetUserData(int32 _Data, int _Position);
	void SetUserData(fp32 _Data, int _Position);
	void SetUserData(AIState _AIState, int _Position);
	void GetUserData(int32& _Data, int _Position);
	void GetUserData(fp32& _Data, int _Position);
	void GetUserData(AIState& _Data, int _Position);
	static void GetUserData(CWObject_CoreData* _pObj, int32& _Data, int _Position);
	static void GetUserData(CWObject_CoreData* _pObj, fp32& _Data, int _Position);
	static void GetUserData(CWObject_CoreData* _pObj, AIState& _Data, int _Position);
public:
	CWObject_Model_Moth();
	virtual void OnCreate();
	virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	static void OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server*);
	virtual void OnFinishEvalKeys();
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	static aint OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);
	virtual void UpdateVisibility(int *_lpExtraModels = NULL, int _nExtraModels = 0); // Recreates the VisBox from m_iModel, also modifies the ShadowCasterFlag.
};


// INLINE FUNCTIONS
inline void CWObject_Model_Moth::SetUserData(int32 _Data, int _Position)
{
	m_Data[_Position] = _Data;
}

inline void CWObject_Model_Moth::SetUserData(fp32 _Data, int _Position)
{
	m_Data[_Position] = *(int32*)&_Data;
}

inline void CWObject_Model_Moth::SetUserData(CWObject_Model_Moth::AIState _Data, int _Position)
{
	m_Data[_Position] = *(int32*)&_Data;
}

inline void CWObject_Model_Moth::GetUserData(int32& _Data, int _Position)
{
	_Data = m_Data[_Position];
}

inline void CWObject_Model_Moth::GetUserData(fp32& _Data, int _Position)
{
	_Data = *(fp32*)&m_Data[_Position];
}

inline void CWObject_Model_Moth::GetUserData(CWObject_Model_Moth::AIState& _Data, int _Position)
{
	_Data = *(CWObject_Model_Moth::AIState*)&m_Data[_Position];
}

inline fp32 CWObject_Model_Moth::GetWingAnimTime(fp32 _Interpolated)
{
	return GetWingAnimTime(m_WingStateTime, m_WingStateFreq, m_TimeScale, m_AIState,
						   m_LandedWingSpreadDuration, _Interpolated);
}

inline void CWObject_Model_Moth::GetUserData(CWObject_CoreData* _pObj, int32& _Data, int _Position)
{
	_Data = _pObj->m_Data[_Position];
}

inline void CWObject_Model_Moth::GetUserData(CWObject_CoreData* _pObj, fp32& _Data, int _Position)
{
	_Data = *(fp32*)&_pObj->m_Data[_Position];
}

inline void CWObject_Model_Moth::GetUserData(CWObject_CoreData* _pObj, CWObject_Model_Moth::AIState& _Data, int _Position)
{
	_Data = *(CWObject_Model_Moth::AIState*)&_pObj->m_Data[_Position];
}

#endif
