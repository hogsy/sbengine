/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Client data for CWObject_CharDarkling.

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_CharDarkling_ClientData

	Comments:

	History:		
		050311:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WObj_CharDarkling_ClientData_h__
#define __WObj_CharDarkling_ClientData_h__

#include "WObj_CharPlayer_ClientData.h"

enum
{
	DARKLING_FLAGS_AUTOPULLUP			= M_Bit(0),
	DARKLING_FLAGS_IS_ROTATING			= M_Bit(1),
	DARKLING_FLAGS_STEEP_SLOPE_AHEAD	= M_Bit(2),	// Character is about to walk over a convex corner
//	DARKLING_FLAGS_DOJUMP				= M_Bit(3),	// Character will perform a jump
	DARKLING_FLAGS_JUMP_MATRIX			= M_Bit(4),	// m_JumpDestination is valid
	DARKLING_FLAGS_JUMP_DIRECTION		= M_Bit(5),	// m_JumpDestination contains jump direction
	DARKLING_FLAGS_CAN_CLIMB			= M_Bit(6),
//	DARKLING_FLAGS_FULLSTOP				= M_Bit(7),	// Character will do a full stop (get rid of all veloctity)
	DARKLING_FLAGS_WORLDSPACE			= M_Bit(8),	// All movement input are interpreted as world-space vectors
	DARKLING_FLAGS_BURNING				= M_Bit(9),	// Burning by staying in the light
	DARKLING_FLAGS_JUMPTOAIR			= M_Bit(10),	// Jumping to air scenepoint
	DARKLING_FLAGS_LIGHTNING			= M_Bit(11), // Lightkiller darkling zapping lights

	DARKLING_STATE_NORMAL				= 0,
	DARKLING_STATE_JUMP_INIT			= 1,
	DARKLING_STATE_JUMP_START			= 2,
	DARKLING_STATE_JUMP_FLY				= 3,
	DARKLING_STATE_JUMP_LAND			= 4,

	DARKLING_CAMERA_RIGHT				= 0,	//Camera is to the right of the darkling
	DARKLING_CAMERA_LEFT				= 1,	//to the left
	DARKLING_CAMERA_RIGHT_TO_LEFT		= 2,	//Camera is moving to the left side of the darkling
	DARKLING_CAMERA_LEFT_TO_RIGHT		= 3,	//to the right side

	DARKLING_NUM_EXTRAINSTANCE			= 3,
};


class CWO_CharDarkling_ClientData : public CWO_CharPlayer_ClientData
{
	typedef CWO_CharPlayer_ClientData parent;
	MRTC_DECLARE;
	AUTOVAR_SETCLASS(CWO_CharDarkling_ClientData, parent);

#define DARKLING_CAMERA_SWITCH_TIME 0.5f

public:
	CVec3Dfp32 m_JumpDir;	// World space jump direction

	CMat4Dfp32 m_MatLook;	// Position matrix in the Gravity-plane
	CMat4Dfp32 m_MatBody;	// Position matrix in the UpVector-plane
	CVec3Dfp32 m_CurrMovement;

	int		m_iScenePointTarget;
	int		m_SoundStartTick;

	fp32	m_TotalLeftTurn;
	fp32	m_TotalRightTurn;
	uint32	m_CameraStartTick;
	int8	m_CameraStatus;
	uint8	m_SparkSound;			// Client only,

	TPtr<CXR_ModelInstance>	m_spExtraModelInstance[DARKLING_NUM_EXTRAINSTANCE];

	CAUTOVAR_OP(CAutoVar_uint16,		m_Flags,				DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint8,			m_State,				DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_CVec3Dfp32,	m_Gravity,				DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_CVec3Dfp32,	m_UpVector,				DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_CVec3Dfp32,	m_Gravity_Estimated,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_CVec3Dfp32,    m_CornerRef,			DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint32, 		m_JumpTick,				DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_CMat4Dfp32,	m_JumpDestination,		DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint16,		m_GravityFreeMaxTicks,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_CAxisRotfp32,	m_Jump_dRot,			DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_int32,			m_LightHurtTick,		DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_CVec3Dfp32,	m_LightningPos,			DIRTYMASK_0_5);
	CAUTOVAR_OP(CAutoVar_int32,			m_LightningTick,		DIRTYMASK_0_5);
	CAUTOVAR_OP(CAutoVar_int8,			m_Spawning,				DIRTYMASK_2_0);
	CAUTOVAR_OP(CAutoVar_int32,			m_iEyeTrailModel,		DIRTYMASK_2_1);
	CAUTOVAR_OP(CAutoVar_int32,			m_DK_Special,			DIRTYMASK_2_1);
	
	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_Flags)
	AUTOVAR_PACK_VAR(m_State)
	AUTOVAR_PACK_VAR(m_Gravity)
	AUTOVAR_PACK_VAR(m_Gravity_Estimated)
	AUTOVAR_PACK_VAR(m_UpVector)
	AUTOVAR_PACK_VAR(m_JumpTick)
	AUTOVAR_PACK_VAR(m_JumpDestination)
	AUTOVAR_PACK_VAR(m_GravityFreeMaxTicks)
	AUTOVAR_PACK_VAR(m_Jump_dRot)
	AUTOVAR_PACK_VAR(m_LightHurtTick)
	AUTOVAR_PACK_VAR(m_LightningPos)
	AUTOVAR_PACK_VAR(m_LightningTick)
	AUTOVAR_PACK_VAR(m_Spawning)
	AUTOVAR_PACK_VAR(m_iEyeTrailModel)
	AUTOVAR_PACK_VAR(m_DK_Special)
	AUTOVAR_PACK_END

public:
	CWO_CharDarkling_ClientData();

	void Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	virtual void Copy(const CWO_Player_ClientData& _CD);

	virtual CWO_CharDarkling_ClientData* IsDarkling() { return this; }

	virtual void Char_UpdateLook(fp32 _FrameFrac);
	virtual void Char_ProcessControl_Look(const CVec3Dfp32& _dLook);
	virtual void Char_ProcessControl_Move(const CVec3Dfp32& _Move);
	virtual void Phys_Move(const CSelection& _Selection, fp32 _dTime, const CVec3Dfp32& _UserVel, bool _bPredicted);
	virtual bool Char_IsSpawning();

	virtual int OnClientPredictControlFrame(CWorld_Client* _pWClient, CWObject_Client* pPredict, CControlFrame& _Ctrl, bool _bFullFrame, bool _bLog);
	virtual void OnRefresh();

	void SetGravity(const CVec3Dfp32& _NewGravity);
	void GetCamera(CMat4Dfp32& _Result, fp32 _IPTime, CMat4Dfp32* _pResultWorld = NULL);
	void Camera_Offset(CMat4Dfp32& _Camera, fp32 _IPTime);
	void Phys_GetAcceleration(CMat4Dfp32* _pMat, fp32 _dTime);
	void Phys_OnImpact(CWorld_PhysState* _pWPhys, CWObject_CoreData* _pObj, const CCollisionInfo& _CInfo);

	void UpdateMatrices();
	bool CheckOnGround();
	bool PerformJump();
	void SnapToGround();

	CXR_ModelInstance* GetModelInstance(CXR_Model* _pModel, const uint8& _iSlot);
};



// Debug defines
#define DEBUG_COLOR_RED   0xffff0000
#define DEBUG_COLOR_GREEN 0xff00ff00
#define DEBUG_COLOR_BLUE  0xff0000ff

#define DO_NOT 1?(void)0:



#endif // __WObj_CharDarkling_ClientData_h__
