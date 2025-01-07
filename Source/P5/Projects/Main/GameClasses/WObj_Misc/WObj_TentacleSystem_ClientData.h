/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Client data for CWObject_TentacleSystem.

	Author:			Anton Ragnarsson, Olle Rosenquist, Patrik Willbo

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWO_TentacleSystem_ClientData

	Comments:

	History:		
		050517:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WObj_TentacleSystem_ClientData_h__
#define __WObj_TentacleSystem_ClientData_h__

#include "../../Shared/MOS/Classes/GameWorld/WObjects/WObj_AutoVar.h"

class CWObject_TentacleSystem;
class CWObject_CreepingDark;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| enums
|__________________________________________________________________________________________________
\*************************************************************************************************/
enum 
{
	// Tentacle arms
	TENTACLE_DEMONARM = 0,						// Jackies tentacle and demonhead buddies
	TENTACLE_DEMONHEAD1,
	TENTACLE_DEMONHEAD2,
	TENTACLE_SCREENSTUFF,						// Tentacles in view when heads are taken out
	TENTACLE_CREEPINGDARK_SQUID,				// Tentacles out from back in creepingdark mode
	TENTACLE_MaxTentacles,

	NUM_POWERSTREAKS = 2,

	// Tentacle system tasks
	TENTACLE_TASK_IDLE			= 0,			// Do nothing
	TENTACLE_TASK_GRABHOLD,						// Grab & hold object in front of the player
	TENTACLE_TASK_GETNOTHING,					// Just send the arm out and bring it back again..
	TENTACLE_TASK_BREAKOBJECT,					// Reach object, damage it and return
	TENTACLE_TASK_WIGGLE,						// Do nothing, just wiggle in player view (temp)
	TENTACLE_TASK_GRABHOLD2,
	TENTACLE_TASK_TRAIL,
	TENTACLE_TASK_BOREDOM,						// Damn we're getting bored.
	TENTACLE_TASK_GRABCHAROBJECT,				// Grab an equipped object from target character
	TENTACLE_TASK_PUSHOBJECT,					// Push/throw/flip object
	TENTACLE_TASK_DEVOURTARGET,					// Devour a target
	
	// Tentacle system states
	TENTACLE_STATE_OFF			= 0,			// Inactive
	TENTACLE_STATE_IDLE,						// Doing nothing
	TENTACLE_STATE_WIGGLE,						// Wiggling in view
	TENTACLE_STATE_INTERESTED,					// Same as wiggle, but looking at something special
	TENTACLE_STATE_REACHOBJECT,					// Moving towards target
	TENTACLE_STATE_REACHPOSITION,				// Just move towards point (do nothing..)
	TENTACLE_STATE_GRABOBJECT,					// Holding object, staying in view
	TENTACLE_STATE_RETURN,						// Returning to master
	TENTACLE_STATE_RETURNLOW,					// Returning to master, moving at ground level
	TENTACLE_STATE_TRAIL,						// Returning to master, moving at ground level
	TENTACLE_STATE_WIGGLE2,						// Special for the "tentacle screen stuff"
	
	// Specials for tentacles
	TENTACLE_DEVOUR_ROTTRACK_HEART					= -9,

	// Tentacle client data flags
	TENTACLE_CD_FLAGS_NORENDER						= M_Bit(0),	// Breaks out of rendering

	// Special constants
	TENTACLE_SPEED_SNAPTOTARGET 					= 1000,			// Snaps tentacle to target
    TENTACLE_LENGTH_DEVOURMAX						= 164,			// 5 meters from target max!

	// Tentacle sounds
	TENTACLE_SOUND_REACH							= 0,
	TENTACLE_SOUND_GRAB,
	TENTACLE_SOUND_HOLD,
	TENTACLE_SOUND_RETURN,
	TENTACLE_SOUND_DEVOUR,
	TENTACLE_SOUND_BREATH_SOFT_DEMONHEAD1,
	TENTACLE_SOUND_BREATH_SOFT_DEMONHEAD2,
	TENTACLE_SOUND_BREATH_HARD_DEMONHEAD1,
	TENTACLE_SOUND_BREATH_HARD_DEMONHEAD2,
	TENTACLE_NumSounds,

	// Demon Arm target selection
	TENTACLE_SELECTION_CHARACTER					= M_Bit(0),
	TENTACLE_SELECTION_CORPSE						= M_Bit(1),
	TENTACLE_SELECTION_OBJECT						= M_Bit(2),
	TENTACLE_SELECTION_LAMP							= M_Bit(3),
	TENTACLE_SELECTION_CHARACTERITEM				= M_Bit(4),
	TENTACLE_SELECTION_SWINGDOOR					= M_Bit(5),
	TENTACLE_SELECTION_ALL							= ~0,

	// Breath flags
	TENTACLE_BREATH_MODEL0							= M_Bit(0),
	TENTACLE_BREATH_MODEL1							= M_Bit(1),
	TENTACLE_BREATH_MODEL2							= M_Bit(2),

	// Don't change any of the AG2 values unless you plan to update the Graph's

	// Same as weapontype (for now, doesn't matter really..), don
	TENTACLE_AG2_TOKEN_HUGIN						= 0,
	TENTACLE_AG2_TOKEN_MUNIN,
	TENTACLE_AG2_TOKEN_GUITENTACLE,
	TENTACLE_AG2_TOKEN_TENTACLESQUID,
	TENTACLE_AG2_TOKEN_HEART,
	TENTACLE_AG2_TOKEN_NUMTOKENS,

	TENTACLE_AG2_IMPULSETYPE_DEMONHEAD				= 3,
	TENTACLE_AG2_IMPULSETYPE_GUITENTACLE			= 4,
	TENTACLE_AG2_IMPULSETYPE_TENTACLESQUID			= 5,
	TENTACLE_AG2_IMPULSETYPE_HEART					= 6,
	TENTACLE_AG2_IMPULSEVALUE_HUGIN					= 8,
	TENTACLE_AG2_IMPULSEVALUE_MUNIN					= 9,
	TENTACLE_AG2_IMPULSEVALUE_LOOK					= 10,
	TENTACLE_AG2_IMPULSEVALUE_LOOKRETURN			= 11,
	TENTACLE_AG2_IMPULSEVALUE_HURT					= 12,
	TENTACLE_AG2_IMPULSEVALUE_DEVOUR				= 13,
	TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKSNAKE		= 14,
	TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKATTACK	= 15,
	TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKRETURN	= 16,
	//TENTACLE_AG2_IMPULSEVALUE__					= 17,
	//TENTACLE_AG2_IMPULSEVALUE___					= 18,
	//TENTACLE_AG2_IMPULSEVALUE____					= 19,
	TENTACLE_AG2_IMPULSEVALUE_BOREDOM_SLEEP			= 20,
	TENTACLE_AG2_IMPULSEVALUE_IDLE					= 21, // TENTACLE_AG2_IMPULSEVALUE_BOREDOMSTOP = 21,
	TENTACLE_AG2_IMPULSEVALUE_BOREDOM_FIGHT			= 22,
	TENTACLE_AG2_IMPULSEVALUE_STARTGROWLVIOLENT		= 23,
	TENTACLE_AG2_IMPULSEVALUE_ENDGROWLVIOLENT		= 24,
	TENTACLE_AG2_IMPULSEVALUE_BOREDOM_TTPLAYER		= 25,
	//TENTACLE_AG2_IMPULSEVALUE_____				= 26,
	//TENTACLE_AG2_IMPULSEVALUE______				= 27,
	//TENTACLE_AG2_IMPULSEVALUE_______				= 28,
	TENTACLE_AG2_IMPULSEVALUE_GROWL					= 29,

	TENTACLE_AG2_IMPULSEVALUE_GUITENTACLE_START		= 0,
	TENTACLE_AG2_IMPULSEVALUE_GUITENTACLE_END		= 1,
	TENTACLE_AG2_IMPULSEVALUE_GUITENTACLE			= 10,
	
	TENTACLE_AG2_IMPULSEVALUE_TENTACLESQUID_CROUCH	= 0,
	TENTACLE_AG2_IMPULSEVALUE_TENTACLESQUID_STAND	= 1,
	TENTACLE_AG2_IMPULSEVALUE_TENTACLESQUID			= 10,
	
	// Start creeping dark values at 100
	TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKNUDGE			= 100,
	TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKMH			= 101,
	TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARK_ATTACKCROUCH = 102,
	TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARK_ATTACKHIGH	= 103,
	TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARK_SNAKE2		= 104,
	TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARK_DEVOUR		= 105,

	// Start heart values at 200
	TENTACLE_AG2_IMPULSEVALUE_HEART						= 200,
	TENTACLE_AG2_IMPULSEVALUE_HEART_DEVOUR_01			= 201,

	TENTACLE_CAMERA_BONE	= 46,
	
	// AnimGraph2 Bool properties
	TENTACLE_AG2_PROPERTY_BOOL_ISSERVER				= 0,
	TENTACLE_AG2_PROPERTY_BOOL_ALWAYSTRUE			= 1,
	TENTACLE_AG2_PROPERTY_BOOL_NUMPROPERTIES,

	// AnimGraph2 Int properties
	TENTACLE_AG2_PROPERTY_INT_DIRECTION				= 2,
	//TENTACLE_AG2_PROPERTY_INT__					= 3,
	TENTACLE_AG2_PROPERTY_INT_DEVOURTARGET			= 4,
	TENTACLE_AG2_PROPERTY_INT_NUMPROPERTIES,

	// Direction property values
	TENTACLE_AG2_DIRECTION_UNDEFINED				= 0,
	TENTACLE_AG2_DIRECTION_LEFT						= 1,
	TENTACLE_AG2_DIRECTION_RIGHT					= 2,
	TENTACLE_AG2_DIRECTION_FRONT					= 3,
	TENTACLE_AG2_DIRECTION_BACK						= 4,
	TENTACLE_AG2_DIRECTION_NUM						= 4,
	TENTACLE_AG2_DIRECTION_LEFT_SLOW				= 5,
	TENTACLE_AG2_DIRECTION_RIGHT_SLOW				= 6,

	// Devour target property values
	TENTACLE_AG2_DEVOURTARGET_UNDEFINED					= 0,
	TENTACLE_AG2_DEVOURTARGET_NORMAL_VARIATION_1		= 1,
	TENTACLE_AG2_DEVOURTARGET_NORMAL_VARIATION_2		= 2,

	// AnimGraph2 State Flags
	TENTACLE_AG2_STATEFLAG_HURTACTIVE				= M_Bit(0),	// 0x00000001
	TENTACLE_AG2_STATEFLAG_LOOKACTIVE				= M_Bit(1),	// 0x00000002
	TENTACLE_AG2_STATEFLAG_LOOKMOVEACTIVE			= M_Bit(2),	// 0x00000004
	TENTACLE_AG2_STATEFLAG_DEVOURTARGET				= M_Bit(3),	// 0x00000008
	//TENTACLE_AG2_STATEFLAG__						= M_Bit(4),	// 0x00000010
	TENTACLE_AG2_STATEFLAG_GROWLING					= M_Bit(5),	// 0x00000020
	//TENTACLE_AG2_STATEFLAG___						= M_Bit(6),	// 0x00000040
	TENTACLE_AG2_STATEFLAG_GROWLINGBASE				= M_Bit(7),	// 0x00000080

	// State flag masks, used to filter out if animation is okay to start or not
	TENTACLE_AG2_STATEFLAG_MASK_LOOK				= (TENTACLE_AG2_STATEFLAG_LOOKMOVEACTIVE | TENTACLE_AG2_STATEFLAG_LOOKACTIVE | TENTACLE_AG2_STATEFLAG_HURTACTIVE | TENTACLE_AG2_STATEFLAG_DEVOURTARGET),
	TENTACLE_AG2_STATEFLAG_MASK_IDLE				= (TENTACLE_AG2_STATEFLAG_HURTACTIVE),
	TENTACLE_AG2_STATEFLAG_MASK_GROWL				= (TENTACLE_AG2_STATEFLAG_HURTACTIVE | TENTACLE_AG2_STATEFLAG_LOOKMOVEACTIVE | TENTACLE_AG2_STATEFLAG_LOOKACTIVE),
	TENTACLE_AG2_STATEFLAG_MASK_HURT				= (TENTACLE_AG2_STATEFLAG_DEVOURTARGET),
	TENTACLE_AG2_STATEFLAG_MASK_DEVOUR				= (TENTACLE_AG2_STATEFLAG_DEVOURTARGET),

	// Borrow an unused autovar dirtymask
	TENTACLE_AG2_DIRTYMASK = CAutoVarContainer::DIRTYMASK_2_1,

	TENTACLE_NETMSG_CREATEDECAL						= 1,

	// How power streaks should behave
	POWERSTREAKS_RENDER_MODEL						= 0,
	POWERSTREAKS_RENDER_MODEL_0						= POWERSTREAKS_RENDER_MODEL,
	POWERSTREAKS_RENDER_MODEL_1,
	POWERSTREAKS_RENDER_MODEL_2,
	POWERSTREAKS_RENDER_MODEL_MAX,
	POWERSTREAKS_RENDER_SCREEN,

	TENTACLE_DEVOURBLEND_TIME_DEFAULT				= 0,
	TENTACLE_DEVOURBLEND_TIME_FORWARD_BACKWARD,
	TENTACLE_DEVOURBLEND_TIME_HEART,
	TENTACLE_DEVOURBLEND_TIME_HEADHEARTOFFSET,
	TENTACLE_DEVOURBLEND_TIME_HOLDING,
};



#define TENTACLE_BASE_BOREDOM			8.0f
#define TENTACLE_RAND_BOREDOM			2.0f


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CSpline_Vec3Dfp32
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CSpline_Vec3Dfp32
{
public:
	// Input data
	struct Point
	{
		CVec3Dfp32 m_Pos;
		CVec3Dfp32 m_Tangent;
	};

	// Temp data
	struct Segment
	{
		fp32 m_TanScale;		// scale modifier for tangents  (TanOut = p0.tangent * scale,  TanIn = p1.tangent * scale)
		fp32 m_SegLen;			// length of segment
		fp32 m_InvSegLen;
		struct CachedPos
		{
			fp32 t;		// t on segment
			fp32 len;	// length to next point
			fp32 sum;	// length from start of segment
		};
		enum { MaxCachePoints = 16 };
		CachedPos m_Cache[MaxCachePoints];
	};
	struct SplinePos
	{
		fp32 t;				// [0..nSegments]  (index and fraction)
		fp32 len;			// length to previous point
		CMat4Dfp32 mat;		// position (and rotation)
	};

	TStaticArray<Point, 16>   m_lPoints;
	TStaticArray<Segment, 15>  m_lSegments;
	fp32 m_Length, m_MaxT;
	CMat4Dfp32 m_EndMat;
	uint8 m_nCachePoints;

	CSpline_Vec3Dfp32();
	void AddPoint(const CMat4Dfp32& _Pos, const CVec3Dfp32& _Tangent);
	bool IsEmpty() const;
	void Finalize(uint8 _nCachePoints = Segment::MaxCachePoints / 2);
	
	void RemoveLastPoint();
	void ModifyLastPoint(const CVec3Dfp32& _Pos, const CVec3Dfp32& _Tangent);

	void CalcPos(fp32 _Time, CVec3Dfp32& _Result) const;			// Evaluate point on segment
	void CalcRot(fp32 _Time, CMat4Dfp32& _Result, const CVec3Dfp32& _RefUp) const;
	void CalcMat(fp32 _Time, CMat4Dfp32& _Result, const CVec3Dfp32& _RefUp) const;
	void FindPos(fp32 _Distance, SplinePos& _Result) const;
	void FindPos(fp32 _Distance, SplinePos& _Result, const CVec3Dfp32& _RefUp) const;
	fp32 GetDistance(fp32 _Time) const;
};

typedef CSpline_Vec3Dfp32 CSpline_Tentacle;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTentacleArnAttachModel
|__________________________________________________________________________________________________
\*************************************************************************************************/
template <uint _NumModels> class TTentacleArmAttachModel
{
protected:
	uint8	m_iAttachNode;
	uint16	m_iModel[_NumModels];
	
	TPtr<CXR_ModelInstance> m_lspModelInstances[_NumModels];


	void UpdateModelInstance(CWorld_Client* _pWClient, CWObject_CoreData* _pObj, CXR_Model* _pModel, TPtr<CXR_ModelInstance> &_spInstance)
	{
		_spInstance = _pModel->CreateModelInstance();
		if (_spInstance)
			_spInstance->Create(_pModel, CXR_ModelInstanceContext(_pWClient->GetGameTick(), _pWClient->GetGameTickTime(), _pObj, _pWClient));
	}


public:
	TTentacleArmAttachModel()
		: m_iAttachNode(0)
	{
		for (uint i = 0; i < _NumModels; i++)
		{
			m_iModel[i];
			m_lspModelInstances[i] = NULL;
		}
	}

	virtual void Pack(uint8*& _pD) const
	{
		TAutoVar_Pack(m_iAttachNode, _pD);
		for (uint i = 0; i < _NumModels; i++)
			TAutoVar_Pack(m_iModel[i], _pD);
	}

	virtual void Unpack(const uint8*& _pD)
	{
		TAutoVar_Unpack(m_iAttachNode, _pD);
		for (uint i = 0; i < _NumModels; i++)
			TAutoVar_Unpack(m_iModel[i], _pD);
	}

	void UpdateModelInstances(CWorld_PhysState* _pWPhysState, CWObject_CoreData* _pObj)
	{
		CWorld_Client* pWClient = safe_cast<CWorld_Client>(_pWPhysState);
		if (pWClient)
		{
			for (uint i = 0; i < _NumModels; i++)
			{
				if (!m_lspModelInstances[i])
				{
					CXR_Model* pModel = _pWPhysState->GetMapData()->GetResource_Model(m_iModel[i]);
					if (pModel)
						UpdateModelInstance(pWClient, _pObj, pModel, m_lspModelInstances[i]);
				}
			}
		}
	}

	M_INLINE CXR_ModelInstance* GetModelInstance(uint _iModelSlot)
	{
		M_ASSERT(_iModelSlot < _NumModels, "TTentacleArmAttachModel::GetModelInstance: Out of range!\n");
		CXR_ModelInstance* pModelInstance = (CXR_ModelInstance*)m_lspModelInstances[_iModelSlot];
		return pModelInstance;
	}

	M_INLINE const uint16&	GetModel(uint _i) const	{ M_ASSERT(_i < _NumModels, "TTentacleArmAttachModel::GetModel: Out of range!\n"); return m_iModel[_i]; }
	M_INLINE uint16&		GetModel(uint _i)		{ M_ASSERT(_i < _NumModels, "TTentacleArmAttachModel::GetModel: Out of range!\n"); return m_iModel[_i]; }
	M_INLINE const uint8&	GetAttachNode() const	{ return m_iAttachNode; }
	M_INLINE uint8&			GetAttachNode()			{ return m_iAttachNode; }
};

/*
class CTentacleArmAttachModel : public TTentacleArmAttachModel<1>
{
	typedef TTentacleArmAttachModel<1>	parent;
public:
	CVec3Dfp32 m_Offset;

	CTentacleArmAttachModel();
	virtual void Pack(uint8*& _pD) const;
	virtual void Unpack(const uint8*& _pD);
};


static M_INLINE void TAutoVar_Pack(const CTentacleArmAttachModel& _Var, uint8*& _pD)	{ _Var.Pack(_pD); }
static M_INLINE void TAutoVar_Unpack(CTentacleArmAttachModel& _Var, const uint8*& _pD)	{ _Var.Unpack(_pD); }
*/


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTentacleArmSetup
|__________________________________________________________________________________________________
\*************************************************************************************************/
struct CTentacleArmSetup
{
	TFStr<20> m_Name;
	uint16 m_liTemplateModels[2];

	CXR_SkeletonAttachPoint m_lAttachPoints[6];
	//CVec3Dfp32 m_lTargetAttachPoints[2];
	uint8 m_nAttachPoints;
	//uint8 m_nTargetAttachPoints;
	uint8 m_iAGToken;
	TTentacleArmAttachModel<1>	m_lAttachModels[2];
	TTentacleArmAttachModel<3>	m_BreathModels;
	TTentacleArmAttachModel<1>	m_DrainModel;

	TThinArray<uint16>			m_liSplineBones;

	fp32 m_Extrude;
	fp32 m_Spin;
	fp32 m_Scale;
	bool m_bMirrorAnim;
	bool m_bPhysLinks;

	CTentacleArmSetup();
	void Setup(const CRegistry& _Reg, CWorld_Server& _WServer);
	void Pack(uint8*& _pD) const;
	void Unpack(const uint8*& _pD);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CPIDRegulator
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CPIDRegulator
{
public:
	CVec3Dfp32 m_IntegrateTerm;
	CVec3Dfp32 m_DeriveTerm;
	CVec3Dfp32 m_LastPos;

	void Reset(const CVec3Dfp32& _CurrPos)
	{
		m_IntegrateTerm = 0;
		m_DeriveTerm = 0;
		m_LastPos = _CurrPos;
	}

	CVec3Dfp32 Update(const CVec3Dfp32& _CurrPos, const CVec3Dfp32& _WantedPos, fp32 _Mass);
};


class CObjectGrabber
{
public:
	CPIDRegulator m_Pid[2];
	CVec3Dfp32 m_LocalGrabPos[2];

	CObjectGrabber();
	void Init(const CMat4Dfp32& _CurrTargetMat, const CVec3Dfp32 _lGrabPos[2]);
	void Update(const CMat4Dfp32& _CurrTargetMat, const CVec3Dfp32 _lGrabPos[2], fp32 _Mass, CVec3Dfp32 _lResult[2]);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTentacleArmState
|__________________________________________________________________________________________________
\*************************************************************************************************/
struct CTentacleArmState
{
	// Replicated:
	uint8 m_Task;
	uint8 m_State;
	uint16 m_iTarget;			// dynamic target
	fp32 m_Speed;
	CVec3Dfp32 m_TargetPos;		// static target
	CVec3Dfp32 m_GrabPoint;
	fp32 m_Length;
	int16 m_iRotTrack;
	uint8 m_iTargetOffset;
	int32 m_BreathCtrl;
	int32 m_BloodTick;
	
	// Not replicated:
	CVec3Dfp32 m_Pos;
	CMat4Dfp32 m_Attach1_Cache;
	CObjectGrabber m_Grabber;
	CVec3Dfp32 m_Dir;
	CVec3Dfp32 m_LastTargetPos, m_CurrTargetPos; // for blending towards TargetPos
	CVec3Dfp32 m_CurrAttachDir;  // current direction at "attach" point for demon heads
	fp32 m_QueueSpeed;
	uint8 m_QueueTask;
	uint8 m_QueueState;
	bool m_bHostileNearby;
//	CPIDRegulator m_PhysArm_Pid0, m_PhysArm_Pid1; 
//	TStaticArray<uint16, 100>* m_plBones;

	CTentacleArmState* m_pControlArm;	// Let me control another arm
	void PassOnTarget();				// Passes target onto selected control arms

	// Task / State queueing and switching helpers
	void UpdateRunQueue();
	void UpdateTaskQueue();
	void UpdateTaskQueue(uint8 _Task, uint8 _State, fp32 _Speed);
	void UpdateTask(uint8 _Task);
	void UpdateTask(uint8 _Task, fp32 _Speed);
	void UpdateTask(uint8 _Task, uint8 _State, fp32 _Speed);
	void UpdateState(uint8 _State, fp32 _Speed);

	CTentacleArmState();
	void Pack(uint8*& _pD) const;
	void Unpack(const uint8*& _pD);

	// Task tests
	M_INLINE bool IsTask_Boredom() const			{ return m_Task == TENTACLE_TASK_BOREDOM;				}
	M_INLINE bool IsTask_Wiggle() const				{ return m_Task == TENTACLE_TASK_WIGGLE;				}
	M_INLINE bool IsTask_DevourTarget() const		{ return m_Task == TENTACLE_TASK_DEVOURTARGET;			}

	bool IsIdle() const;
	bool IsDevouring() const;
	bool IsCreepingDark() const;
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTentacleAnimGraph
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CTentacleSystemAnimGraph : public CWO_ClientData_AnimGraph2Interface
{
public:
	spCWAG2I        m_spAGI;
	spCWAG2I_Mirror m_spMirror;

	int32 m_lEndTick[TENTACLE_AG2_TOKEN_NUMTOKENS];
	int32 m_lTokenStateFlags[TENTACLE_AG2_TOKEN_NUMTOKENS];
	bool m_bNeedUpdate;
	
	CTentacleSystemAnimGraph();
	virtual const CWAG2I* GetAG2I() const { return m_spAGI; }
	virtual       CWAG2I* GetAG2I()       { return m_spAGI; }

	virtual void SetInitialProperties(const CWAG2I_Context* _pContext);
	virtual void UpdateImpulseState(const CWAG2I_Context* _pContext);

	virtual void AG2_OnEnterState(const CWAG2I_Context* _pContext, CAG2TokenID _TokenID, 
	                              CAG2StateIndex _iState, CAG2AnimGraphID _iAnimGraph, 
	                              CAG2ActionIndex _iEnterAction);

	void Copy(const CTentacleSystemAnimGraph& _CD);
	void Clear();

	// Demon head impulse helper
	bool SendImpulse_DemonHead(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, CAG2ImpulseValue _Value);

	// Token control (helpers)
	bool Token_Check(CAG2TokenID _iToken, uint32 _TokenFlag);

	// End tick control (helpers)
	int  EndTick_Get(CAG2TokenID _iToken);
	void EndTick_Set(CAG2TokenID _iToken, int _GameTick, const CWAG2I_Context* _pContext);
	void EndTick_Set(CAG2TokenID _iToken, int _GameTick);
	bool EndTick_Reached(CAG2TokenID _iToken, int _GameTick, int _Subtract = 0);

	// Animation impulse helpers
	bool Anim_Growl(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, bool _bGrowl);
	bool Anim_Idle(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, bool _bBoredom);
	bool Anim_DevourTarget(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, int _DevourVariation);
	bool Anim_Hurt(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, int _ImpactDirection);
	bool Anim_Look(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, int _LookDirection, fp32 _Duration);

	// Token validation in debug builds
	bool Debug_ValidateToken(CAG2TokenID _iToken, const char* _pString);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_PowerStreaks
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_Model_PowerStreaks : public CXR_Model
{
public:
	CXR_Model_PowerStreaks();

	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_TentacleSystem_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWO_TentacleSystem_ClientData : public CReferenceCount, public CAutoVarContainer
{
	friend class CWObject_TentacleSystem;
	friend class CWObject_CreepingDark;

	CXR_Model_PowerStreaks m_PowerStreaks;

	CWObject_CoreData* m_pObj;
	CWorld_PhysState* m_pWPhysState;

	CMTime m_StartTime;
	CMTime m_LastRender;
	CVec3Dfp32 m_ArmControl;
	CTentacleSystemAnimGraph m_AnimGraph;

//	fp32 m_Retract;
	
	// Value of when tentacles boredom kicks in and how close we are to it doing so.
	int m_TargetBoredom;
	int m_CurrentBoredom;
	fp32 m_PowerFade;

   	// Darkness power variables
	uint m_nDamage;				// amount of damage to give for task 'BREAKOBJECT'
	fp32 m_GrabPower;			// controls how heavy objects the demon arm can lift
	fp32 m_Darkness;

	struct SCollisionFlags
	{
		int32		m_OwnFlags;
		int32		m_ObjectFlags;
		int32		m_MediumFlags;
		int32		m_iExclude;
		CVec3Dfp32	m_Pos1;
		CVec3Dfp32	m_Pos2;

		SCollisionFlags(const CVec3Dfp32& _Pos1, const CVec3Dfp32& _Pos2) 
			: m_OwnFlags(OBJECT_FLAGS_PROJECTILE)
			, m_ObjectFlags(OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER)
			, m_MediumFlags(XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS), m_iExclude(0), m_Pos1(_Pos1), m_Pos2(_Pos2) 
		{ }

		SCollisionFlags(int32 _OwnFlags, int32 _ObjectFlags, int32 _MediumFlags, int32 _iExclude, const CVec3Dfp32& _Pos1, const CVec3Dfp32& _Pos2)
			: m_OwnFlags(_OwnFlags)
			, m_ObjectFlags(_ObjectFlags)
			, m_MediumFlags(_MediumFlags)
			, m_iExclude(_iExclude)
			, m_Pos1(_Pos1)
			, m_Pos2(_Pos2) 
		{ }
	};

public:
	AUTOVAR_SETCLASS(CWO_TentacleSystem_ClientData, CAutoVarContainer);

	typedef TAutoVar_StaticArray<CTentacleArmSetup, TENTACLE_MaxTentacles> CAutoVar_ArmSetupArray;
	typedef TAutoVar_StaticArray<CTentacleArmState, TENTACLE_MaxTentacles> CAutoVar_ArmStateArray;
	typedef TAutoVar_StaticArray<CVec2Dfp32,        TENTACLE_MaxTentacles> CAutoVar_ArmRetractArray;		// (Last,Curr) pair for each arm
	typedef TAutoVar_StaticArray<uint16, TENTACLE_NumSounds> CAutoVar_SoundArray;

	CAUTOVAR_OP(CAutoVar_int16,			m_iOwner,		DIRTYMASK_1_0);
	CAUTOVAR(CAutoVar_SoundArray,		m_liSounds,		DIRTYMASK_1_0);
	CAUTOVAR(CAutoVar_ArmSetupArray,	m_lArmSetup,	DIRTYMASK_1_0);
	CAUTOVAR(CAutoVar_ArmStateArray,	m_lArmState,	DIRTYMASK_0_1);
	CAUTOVAR(CAutoVar_ArmRetractArray,	m_lArmRetract,	DIRTYMASK_0_2);
	CAUTOVAR_OP(CAutoVar_uint8,			m_StateFlags,	DIRTYMASK_0_0);

	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_iOwner)
	AUTOVAR_PACK_VAR(m_liSounds)
	AUTOVAR_PACK_VAR(m_lArmSetup)
	AUTOVAR_PACK_VAR(m_lArmState)
	AUTOVAR_PACK_VAR(m_StateFlags)
	AUTOVAR_PACK_VAR(m_lArmRetract)
	AUTOVAR_PACK_END

protected:
	void GetSpline(uint8 _iArm, CSpline_Tentacle& _Spline, bool _bRendering, CMat4Dfp32* _pLastAttachMat = NULL);

	bool GetCharBonePos(CWObject_CoreData& _Char, int _RotTrack, CVec3Dfp32& _Result, fp32 _IPTime = 1.0f);
	bool GetCharBoneRot(CWObject_CoreData& _Char, int _RotTrack, CMat4Dfp32& _Result, fp32 _IPTime = 1.0f);
	bool GetCharBoneMat(CWObject_CoreData& _Char, int _RotTrack, CMat4Dfp32& _Result, fp32 _IPTime = 1.0f);
	void InitCharRagdoll(CWObject_Character* _pChar);

	CTentacleArmState& GetRandomDemonHead(uint32 _Seed);

	bool DemonArmIntersect(const CVec3Dfp32& _Pos0, const CVec3Dfp32& _Pos1, uint _iObj, CMat4Dfp32* _pRetMat);
	bool CheckCollision(const CVec3Dfp32& _OldPos, CVec3Dfp32& _NewPos, fp32 _Radius, int _iExcludeObj = 0);
	fp32 GetDemonHeadFade(CWorld_Client* _pWClient, CWObject_CoreData* _pOwner, int8* _piPower = NULL, int8* _piLastPower = NULL);

	uint GetBreathFlags(int32 _BreathCtrl);
	fp32 GetDevourBlendTime(uint8 _iAGToken, fp32 _IPTime, uint _ReturnType = TENTACLE_DEVOURBLEND_TIME_DEFAULT);
public:
	CWO_TentacleSystem_ClientData();

	void Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	void OnRefresh();
	void OnRender(CWorld_Client* _pWClient, CXR_Engine* _pEngine);
	void OnRender_AnimateTounge_TEMP(CWorld_Client* _pWClient, CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInst, uint _Seed);

	void UpdateModelInstances();

	bool CanStartCreepingDark(uint8 _iArm, const CVec3Dfp32& _StartPos);
	void SetCreepCamera(const CMat4Dfp32& _CameraMat);

	static void CreateBloodEffect(CWorld_Server* _pServer, int _iObject, const CVec3Dfp32& _Pos, const SCollisionFlags* _pColFlags = NULL, bool _bDecal = false, const char* _pDamageEffect = NULL, const CVec3Dfp32* _pPos = NULL, const CVec3Dfp32* _pSplattDir = NULL, int8 _MinSpawn = 1, int8 _MaxSpawn = 5, int8 _iDecal = -1, int8 _Size = 18);
	//void Devour(const int32& _DemonHead);
	//void SpawnBloodSpurt(const int& _iObject, CVec3Dfp32& _Pos);

	void UpdateDemonHeadCollisions(uint _iArm, const CVec3Dfp32& _AttachPos, const CMat4Dfp32& _EndPos);

	// Server only
	void Server_UpdateArm(CTentacleArmState& _Arm, bool _bAtTarget, const CMat4Dfp32& _EndPos, CTentacleArmSetup& _ArmSetup, fp32 _LengthToTarget);
	void Server_UpdateArmAnimGraph(CTentacleArmState& _Arm, CTentacleArmSetup& _ArmSetup, bool _bAtTarget, fp32 _LengthToTarget);
	void Server_UpdatePhysArm(CTentacleArmState& _Arm, const CSpline_Vec3Dfp32& _Spline);
	void Server_MoveObject(CTentacleArmState& _Arm, const CVec3Dfp32& _NewPos, const CMat4Dfp32& _RefMat);
	void Server_ThrowObject(CTentacleArmState& _Arm, const CVec3Dfp32& _Dir, fp32* _pRigidForce = NULL);
	void Server_CreateDevourBlood(CTentacleArmState& _Arm);
	void Server_UpdateMouthBlood(CWorld_Server* _pWServer, CTentacleArmState& _Arm);
	bool Server_IsControllerIdle(CWObject_Character* _pOwnerChar);

	// Client only
	//void Client_CreateDecal(CTentacleArmState& _Arm);

	CWObject_Character* GetMaster();

	// Misc. utility functions
	bool SendProjectileDamage(CWorld_Server* _pWServer, const CVec3Dfp32& _Pos1, const CVec3Dfp32& _Pos2, const CVec3Dfp32& _Force, int32 _Damage);
};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Tentacle_PhysLink
|
| A lightweight object which is only used for physics simulation.
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CWObject_Tentacle_PhysLink : public CWObject
{
	MRTC_DECLARE_SERIAL_WOBJECT;

	uint16 m_iConstraint;

public:
	static void OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat);

	bool InitPhysics(const CWO_PhysicsPrim& _PhysPrim);
	void ConnectTo(uint16 _iObject);
	void Enable();
	void Disable();
};



#endif // __WObj_TentacleSystem_ClientData_h__
