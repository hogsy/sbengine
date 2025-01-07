/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_CreepingDark.h
					
	Author:			Olle Rosenquist
					
	Copyright:		Copyright O3 Games AB 2004
					
	Contents:		
					
	Comments:		
					
	History:		
		041110:		Created File
\*____________________________________________________________________________________________*/

#ifndef _INC_WOBJ_CREEPINGDARK
#define _INC_WOBJ_CREEPINGDARK

#include "../WObj_Sys/WObj_Physical.h"
#include "../WObj_Messages.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_Hook.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AutoVar.h"
#include "WObj_TentacleSystem_Clientdata.h"

#define CWObject_CreepingDarkParent CWObject
#define CREEPINGDARK_MAX_CAMERATILT 0.18f

enum
{
	OBJMSG_CREEPINGDARK_TURNLEFT = OBJMSGBASE_MISC_CREEPINGDARK,
	OBJMSG_CREEPINGDARK_TURNRIGHT,
	OBJMSG_CREEPINGDARK_BACKTRACK,
	OBJMSG_CREEPINGDARK_SETSTARTINGCHARACTER,
	OBJMSG_CREEPINGDARK_DOATTACK,
	OBJMSG_CREEPINGDARK_DEVOUR,
	OBJMSG_CREEPINGDARK_GETDARKNESSCOST,
	OBJMSG_CREEPINGDARK_JUMP,

	CREEPINGDARK_MOVEMODE_UNDEFINED = 0,
	CREEPINGDARK_MOVEMODE_CREEPING = 1,
	CREEPINGDARK_MOVEMODE_BACKTRACKING = 2,
	CREEPINGDARK_MOVEMODE_ATTACKING = 3,
	CREEPINGDARK_MOVEMODE_DEVOUR = 4,

	CREEPINGDARK_CHECKPOINTORIENTATION_POSX = 0,
	CREEPINGDARK_CHECKPOINTORIENTATION_NEGX,
	CREEPINGDARK_CHECKPOINTORIENTATION_POSY,
	CREEPINGDARK_CHECKPOINTORIENTATION_NEGY,
	CREEPINGDARK_CHECKPOINTORIENTATION_POSZ,
	CREEPINGDARK_CHECKPOINTORIENTATION_NEGZ,

	CREEPINGDARK_COPYFLAGS_RENDERHEAD		= 1 << 0,
	CREEPINGDARK_COPYFLAGS_REACHEDEND		= 1 << 1,
	CREEPINGDARK_COPYFLAGS_FIRSTCOPY		= 1 << 2,
	CREEPINGDARK_COPYFLAGS_COPYSTARTTICK	= 1 << 3,
	CREEPINGDARK_COPYFLAGS_COPYENDTICK		= 1 << 4,
	CREEPINGDARK_COPYFLAGS_COPYGRAVITY		= 1 << 5,
	CREEPINGDARK_COPYFLAGS_COPYSTARTCHAR	= 1 << 6,
	CREEPINGDARK_COPYFLAGS_LOCKEDROT		= 1 << 7,


	CREEPINGDARK_MOVEFLAGS_AUTOPULLUP			= 1 << 0,
	CREEPINGDARK_MOVEFLAGS_IS_ROTATING			= 1 << 1,
	CREEPINGDARK_MOVEFLAGS_STEEP_SLOPE_AHEAD	= 1 << 2,
	CREEPINGDARK_MOVEFLAGS_FAKEGRAV				= 1 << 3,

	CREEPINGDARK_CHECKPOINT_LORES				= 1 << 0,	// Low resolution checkpoint
	CREEPINGDARK_CHECKPOINT_HIRES				= 1 << 1,	// High resolution checkpoint (Used for rendering)

	CREEPINGDARK_SELECTION_CHARACTER			= M_Bit(0),
	CREEPINGDARK_SELECTION_LAMP					= M_Bit(1),
	CREEPINGDARK_SELECTION_OBJECT				= M_Bit(2),
	CREEPINGDARK_SELECTION_TIGGER				= M_Bit(3),
	CREEPINGDARK_SELECTION_GLASS				= M_Bit(4),

	CREEPINGDARK_NETMSG_SOUND	= 1,

	CREEPINGDARK_SOUND_AT = 0,
	CREEPINGDARK_SOUND_ON,
};

// Doesn't differ much from tentacle systems CSpline class with a few exceptions here and there
class CSpline_RenderTrail
{
public:
	struct Point
	{
		CVec3Dfp32 m_Pos;		// Start value
		CVec3Dfp32 m_TangentOut;	// Out tangent
		CVec3Dfp32 m_TangentIn;
	};
	struct Segment
	{
		fp32 m_SegLen;	// length of segment
		fp32 m_InvSegLen;
		struct CachedPos
		{
			fp32 t;		// t on segment
			fp32 len;	// length to next point
			fp32 sum;	// length from start of segment
		};
		enum { MaxCachePoints = 8 };
		CachedPos m_Cache[MaxCachePoints];
	};
	TThinArray<Point>		m_lPoints;
	TThinArray<Segment>		m_lSegments;
	
	// Finalized tangent vectors
	TThinArray<CVec3Dfp32>	m_lTangentOut;
	TThinArray<CVec3Dfp32>	m_lTangentIn;
	fp32 m_Length, m_MaxT;
	CMat4Dfp32 m_EndMat;
	uint8 m_nCachePoints;
	uint8 m_nTempPoints;

	CSpline_RenderTrail();
	void Create(const int& _nPoints);
	void AddPoint(const CMat4Dfp32& _Pos, const CVec3Dfp32& _Tangent);
	void AddTempPoint(const CMat4Dfp32& _Pos, const CVec3Dfp32& _Tangent);
	void PreFinalize();
	void Finalize(uint8 _nCachePoints = Segment::MaxCachePoints);
	void Render(CWireContainer& _WC);

	struct SplinePos
	{
		fp32 t;				// [0..nSegments]  (index and fraction)
		fp32 len;			// length to previous point
		CMat4Dfp32 mat;		// position (and rotation)
	};
	void CalcPos(fp32 _Time, CVec3Dfp32& _Result) const;			// Evaluate point on segment
	void CalcRot(fp32 _Time, CMat4Dfp32& _Result, const CVec3Dfp32& _RefUp) const;
	void CalcMat(fp32 _Time, CMat4Dfp32& _Result, const CVec3Dfp32& _RefUp) const;
	void FindPos(fp32 _Distance, SplinePos& _Result) const;
	void FindPos(fp32 _Distance, SplinePos& _Result, const CVec3Dfp32& _RefUp) const;
};

class CSpline_Creep : public CSpline_Vec3Dfp32
{
public:
	void Clear();
};

class CWObject_CreepingDark : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	class CCheckPoint
	{
	public:
		CVec3Dfp32 m_Position;
		// Orientation of "up" axis (needs to be angles instead)
		//uint8 m_Orientation;
		CVec3Dfp32 m_Normal;
		uint8 m_Flags;
	
		CCheckPoint() : m_Flags(CREEPINGDARK_CHECKPOINT_LORES) {};
		CCheckPoint(const CVec3Dfp32& _Position, CVec2Dfp32 _Orientation, const uint8& _Flags = CREEPINGDARK_CHECKPOINT_LORES);
		CCheckPoint(const CVec3Dfp32& _Position, CVec3Dfp32 _Normal, const uint8& _Flags = CREEPINGDARK_CHECKPOINT_HIRES);
		CVec3Dfp32 GetNormal() const;
		CVec2Dfp32 GetOrientation() const;
	};

	// Need this info on client if we are to make a smooth camera
	class CCreepingDarkClientData : public CReferenceCount, public CAutoVarContainer
	{
		AUTOVAR_SETCLASS(CCreepingDarkClientData, CAutoVarContainer);
	public:
		// New test
		uint8		m_MoveFlags;
		uint32		m_LastGravityTick;
		CVec3Dfp32	m_Gravity;
		CVec3Dfp32	m_Gravity_Estimated;
		CVec3Dfp32	m_SteepSlopeNormal;
		CVec2Dfp32	m_LookSinceGravChange;
		CVec3Dfp32	m_CornerRef;

		int32		m_Phys_nInAirFrames;

		//CQuatfp32 m_qCurrentOrientation;
		//CQuatfp32 m_OldLook;
		//CQuatfp32 m_qCurrentCamera;
		//CQuatfp32 m_qPrevCamera;
		// Camera's max moves with this in mind ("up constraint")
		//CVec3Dfp32 m_CurrentGravity;
		//CQuatfp32 m_qCurrentGravity;
		//CQuatfp32 m_qPrevGravity;
		// Number of checkpoints copied to client...(see if this works so 
		// we don't have to copy all checkpoints every time...)
		int32 m_StartTick;
		int32 m_EndTick;
		int32 m_LastContactTick;
		int32 m_JumpStartTick;
		fp32 m_MoveSpeedTarget;
		fp32 m_MoveSpeed;
		fp32 m_SoftBlock;
		int16 m_iStartingCharacter;
		//bool m_bCopyOrientation : 1;
		bool m_bOnGround;
		bool m_bInWater;
		bool m_bCopyFirst : 1;
		bool m_bCopyEnd : 1;
		bool m_bPrevNoGround : 1;
		bool m_bBackTrackReachedEnd : 1;
		bool m_bCopyStartChar;
		bool m_bRenderHead : 1;
		bool m_bLockedRot : 1;
		bool m_bCopyGravity : 1;
		bool m_bCanClimb : 1;
		bool m_bJumping:1;
		bool m_bNoClimbBlock:1;
				
		CMat4Dfp32 m_LastCamera;

		CSpline_RenderTrail	m_TrailSpline;

		// Backtracker
		CAUTOVAR_OP(CAutoVar_CMTime, m_BackTrackStart, DIRTYMASK_0_0);
		CAUTOVAR_OP(CAutoVar_int8, m_MoveMode, DIRTYMASK_0_1);
		CAUTOVAR_OP(CAutoVar_int32, m_AttackStartTime, DIRTYMASK_0_1);
		CAUTOVAR_OP(CAutoVar_uint16,m_iAutoAimTarget,DIRTYMASK_0_2);
		CAUTOVAR_OP(CAutoVar_CVec3Dfp32, m_NeckPos, DIRTYMASK_0_1);
		//CAUTOVAR_OP(CAutoVar_int32, int32, m_CurrentCheckpoint, DIRTYMASK_1_0);
		AUTOVAR_PACK_BEGIN
		AUTOVAR_PACK_VAR(m_BackTrackStart)
		AUTOVAR_PACK_VAR(m_MoveMode)
		AUTOVAR_PACK_VAR(m_AttackStartTime)
		AUTOVAR_PACK_VAR(m_iAutoAimTarget)
		AUTOVAR_PACK_VAR(m_NeckPos)

		//AUTOVAR_PACK_VAR(m_CurrentCheckpoint)
		AUTOVAR_PACK_END

		// This data is NOT valid on server!!
		TThinArray<CVec3Dfp32>	m_lCheckPointsPos;
		TThinArray<CVec3Dfp32>	m_lCheckPointsN;

		// This data is NOT valid on client!!
		uint8 m_nLastCheckPoints;
		//uint8 m_DirtyFlags;

		CCreepingDarkClientData()
		{
			Clear();
		}

		void Clear()
		{
			m_JumpStartTick = 0;
			m_LastGravityTick = 0;
			m_bOnGround = true;
			m_bInWater = false;
			m_MoveFlags = 0;
			m_Gravity = CVec3Dfp32(0.0f,0.0f,-1.0f);
			m_SteepSlopeNormal = CVec3Dfp32(0.0f,0.0f,1.0f);;
			m_Gravity_Estimated = CVec3Dfp32(0.0f,0.0f,1.0f);
			m_CornerRef = CVec3Dfp32(0.0f,0.0f,0.0f);
			m_Phys_nInAirFrames = 0;
			m_SoftBlock = 1.0f;

			//m_CurrentGravity = 0.0f;
			//m_bCopyOrientation = 0;
			m_bCopyFirst = 0;
			m_bCopyEnd = 0;
			m_bBackTrackReachedEnd = 0;
			m_bPrevNoGround = 0;
			m_bCopyStartChar = 0;
			m_bRenderHead = 0;
			m_StartTick = 0;
			m_EndTick = 0;
			m_LastContactTick = 0;
			m_iStartingCharacter = 0;
			m_MoveSpeedTarget = 0.0f;
			m_MoveSpeed = 0.0f;
			m_bCopyGravity = 0;
			m_BackTrackStart.m_Value.Reset();
			m_MoveMode = CREEPINGDARK_MOVEMODE_CREEPING;
			m_nLastCheckPoints = 0;
			m_bCanClimb = false;
			m_LastCamera.Unit();
			m_bJumping = false;
			m_bNoClimbBlock = false;
		}

		void GetOrientation(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CMat4Dfp32& _Mat, bool _bTilt = true);
		void SetOrientation(CWO_Character_ClientData* _pCD, const CVec3Dfp32& _dLook);
		void UpdateOrientation(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, fp32 _dTime);
		void UpdateOrientation(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, const CMat4Dfp32& _NewMat);

		int OnCreateClientUpdate(uint8* _pData);
		int OnClientUpdate(const uint8* _pData);
		// Resets camera to current orientation

		void SetGravity(const CVec3Dfp32& _NewGravity, CWorld_PhysState* _pWPhys, bool bFakeGrav = false, bool _bSnapToGround = false);

		void NewProcessLook(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, const CVec3Dfp32& _dLook);
		void NewCreate();
		void NewGetAcc(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CSelection& _Selection, const CVec3Dfp32& _ControlMove, CVec3Dfp32& _Acc);
		void NewPhys_OnImpact(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, const CCollisionInfo& _CInfo);
		void NewPhys_Move(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CSelection& _Selection, fp32 _dTime, const CVec3Dfp32& _UserVel, bool _bPredicted);

		void Phys_Move(CWorld_PhysState* _pWPhysState, CWObject_CoreData* _pCreep, fp32 _dTime);
		void GetAcc(CWorld_PhysState* _pWPhysState, CWObject_CoreData* _pCreep, CMat4Dfp32& _AccMat);
		bool CheckOnGround(CWorld_PhysState* _pWPhysState, CWObject_CoreData* _pCreep);
		void SnapToGround(CWorld_PhysState* _pWPhysState, CWObject_CoreData* _pCreep, fp32 _dTime);
	};

protected:

	// Get Current gravity (towards current surface (wall/ceiling/floor...)
	CSpline_Creep m_AttackPath;
	CVec3Dfp32 m_AttackStartPos;
	CVec3Dfp32 m_AttackStartDir;
	TArray<CCheckPoint> m_lCheckPoints;
	//CMat4Dfp32 m_CurrentOrientation;
	CVec3Dfp32 m_LastCheckPointDir;
	CVec3Dfp32 m_LastHiResCheckPointDir;
	CVec3Dfp32 m_AttackLocalOffset;	//Local space offset that tells how much off we are
	//CVec3Dfp32 m_GravityVector;
	int32 m_CurrentCheckpoint;
	//int32 m_LastDirectionChange;
	//int32 m_LastDownHillChange;
	int32 m_LastChange;
	int32 m_LastHitbelow;
	int32 m_LastAttack;
	uint32 m_LastAutoAimTickCheck;
	uint32 m_DevourTarget;
	uint32 m_DevourAttackStartTick;	
	uint32 m_MovingAgainstWallTicks;
	int16 m_iAttackTarget;
	uint8 m_bBloodSpawned;
	bool m_bLastChangeDown:1;
	bool m_bAttackDamageSent:1;
	bool m_bAttackAnimSent:1;
	bool m_bIsSnaking:1;
	bool m_bPlayExitSound:1;
	CVec3Dfp32 m_LastPosCost;
	fp32 m_LastCost;
	
	int GetDarknessCost();
	bool BackTrackGetPos(CVec3Dfp32& _Pos, int32 _GameTick, fp32 _IPTime = 0.0f);
	void BackTrackStart();
	static bool BackTrackActive(const CCreepingDarkClientData* pCD);
	bool BackTrackGetCamera(CMat4Dfp32& _Cam, int32 _GameTick, fp32 _dTime = 0.0f);

	void DevourStart(void);
	
	//void CheckNewOrientation(CCollisionInfo* _pCollisionInfo);
	//void UpdateOrientation();
	void UpdateLastCheckPointDir(const CVec3Dfp32& _Dir);
	void AddCheckpoint(const CVec3Dfp32& _Normal, const uint8& _Flags = CREEPINGDARK_CHECKPOINT_LORES);
	CCheckPoint* GetLastCheckPointType(const uint8& _Flags = CREEPINGDARK_CHECKPOINT_LORES);
	CCheckPoint* GetLastCheckPoint();
	//void DebugRenderCheckPoints(CWorld_PhysState* _pWPhys);

	bool BBoxHit(const CSelection& _Selection, const CVec3Dfp32& _Pos, const CWO_PhysicsState& _PhysState);

	void CheckTargets(fp32 _Length, fp32 _SphereSize);
	void FindAutoAimTargets(void);
	CVec3Dfp32 GetHeartPos(int32 _iObj);

	static void GetLookQuat(CCreepingDarkClientData* _pCD, const CQuatfp32& _qGravity, CQuatfp32& _qLook, const CVec2Dfp32& _Look, bool _bIncludeY = true);

	//static void GetOrientation(CWObject_CoreData* _pObj, CMat4Dfp32& _Mat);
	void GetMedium(CXR_MediumDesc& Medium, const CVec3Dfp32& MediumV);
	void UpdateVelocity(const CSelection& _Selection, CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, const CVec3Dfp32& _Velocity, const CVec3Dfp32& _Gravity);
	void UpdateVelocityModifiers();
	void GetVelocity(CWObject_CoreData* _pObj, const CVec3Dfp32& _Direction, CVec3Dfp32& _Velocity);

	void MoveCreeping(const CVec3Dfp32& _Velocity);
	void MoveBackTrack();
	void MoveAttack();
	void MoveDevour();

	void MakeAttackPath(CWObject_CoreData* _pObj);
	bool CreateAttack(CWObject_CoreData* _pObj);

	CWObject* SelectTarget(CCollisionInfo* _pCInfo, uint _SelectionMask = CREEPINGDARK_SELECTION_CHARACTER | CREEPINGDARK_SELECTION_LAMP | CREEPINGDARK_SELECTION_OBJECT | CREEPINGDARK_SELECTION_TIGGER, const fp32& _MaxRange = 32.0f);
	
	// Find target
	bool CWObject_CreepingDark::GetCharBonePos(CWObject_CoreData& _Char, int _RotTrack, CMat4Dfp32& _Result);
	void NetMsgSendSound(int32 iSound, int8 _Type = CREEPINGDARK_SOUND_AT, int8 _AttnType = 0);

	bool m_bSendStartSound;

public:
	virtual void OnCreate();

	virtual void OnSpawnWorld();
	static void OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient);
	virtual void OnRefresh();
	virtual aint OnMessage(const CWObject_Message& _Msg);
	static void OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer);

	static bool GetCamera(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, CMat4Dfp32& _Cam, const CVec2Dfp32& _Look, int32 _GameTick, fp32 _dTime);
	static int OnPhysicsEvent(CWObject_CoreData*, CWObject_CoreData*, CWorld_PhysState*, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo = NULL);

	static CCreepingDarkClientData* GetCreepingDarkClientData(CWObject_CoreData* _pObj);
	static const CCreepingDarkClientData* GetCreepingDarkClientData(const CWObject_CoreData* _pObj);
	
	static int32 GetStartTick(CWObject_CoreData* _pObj);
	static int32 GetEndTick(CWObject_CoreData* _pObj);
	static bool BackTrackReachedEnd(CWObject_CoreData* _pObj);
	static bool BackTrackActive(CWObject_CoreData* _pObj);
	static bool Renderhead(CWObject_CoreData* _pObj);
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);

	static void UpdateOrientation(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, fp32 _dTime);
	static void UpdateCameraLook(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CVec2Dfp32& _Look, CVec3Dfp32 _dLook);
	static void CWObject_SetCreepOrientation(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, CVec2Dfp32& _Look);
	//static void CWObject_SetOrientationFromCreep(CWObject_CoreData* _pObj, CVec2Dfp32& _Look);

	// Set [-1<->1] 
	static void SetMoveSpeed(CWorld_PhysState* _pWPhys, int32 _iCreep, fp32 _Speed, fp32 _Look);
	static void SetWallClimb(CWorld_PhysState* _pWPhys, int32 _iCreep, bool _bOn);

	int OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pObj, uint8* _pData, int _Flags) const;
	static int OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags);

	void OnDeltaSave(CCFile* _pFile);
	void OnDeltaLoad(CCFile* _pFile, int _Flags);

	const TArray<CCheckPoint>& GetCheckPoints() const { return m_lCheckPoints; }
	void NewSetPhysics();
	void Jump(void);

	static void OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg);

};

#define CWObject_CreepingDarkEntityParent CWObject_Model
class CWObject_CreepingDarkEntity : public CWObject_CreepingDarkEntityParent
{
	MRTC_DECLARE_SERIAL_WOBJECT;
public:
	/*enum
	{
		CREEPINGDARKENTITY_FLAG_COLLIDE		= M_Bit(0),
		CREEPINGDARKENTITY_FLAG_SLOW		= M_Bit(1),
	};
	int32 m_CreepFlags;*/
	// wee... or something..
	//virtual void OnCreate();
	//virtual void OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey);
	virtual void OnFinishEvalKeys();
};

#endif
