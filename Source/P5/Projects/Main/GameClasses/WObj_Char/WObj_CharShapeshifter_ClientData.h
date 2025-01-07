/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			Client data for CWObject_CharShapeshifter.

Author:			Roger Mattsson

Copyright:		2005 Starbreeze Studios AB

Contents:		CWObject_CharShapeshifter_ClientData

Comments:

History:		
051117:		Created file
\*____________________________________________________________________________________________*/
#ifndef __WObj_CharShapeshifter_ClientData_h__
#define __WObj_Charhapeshifter_ClientData_h__

#include "WObj_CharDarkling_ClientData.h"

enum
{
	MP_BOOST_ACTIVE_NONE = M_Bit(0),
	MP_BOOST_ACTIVE_SPEED = M_Bit(1),
	MP_BOOST_ACTIVE_DAMAGE = M_Bit(2),
	MP_BOOST_ACTIVE_HEALTH = M_Bit(3),
	MP_BOOST_ACTIVE_SHIELD = M_Bit(4),	//Blocks one darkling jump attack
	MP_BOOST_ACTIVE_INVISIBILITY = M_Bit(5),
};

class CWO_CharShapeshifter_ClientData : public CWO_CharDarkling_ClientData
{
	typedef CWO_CharDarkling_ClientData parent_darkling;
	typedef CWO_Character_ClientData parent_human;
	MRTC_DECLARE;
	AUTOVAR_SETCLASS(CWO_CharShapeshifter_ClientData, parent_darkling);

public:
	CAUTOVAR_OP(CAutoVar_int8,		m_IsHuman,	DIRTYMASK_0_3);
	CAUTOVAR_OP(CAutoVar_uint8,		m_ActiveBoosts,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint32,	m_SpeedBoostStartTick,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint32,	m_SpeedBoostDuration,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_fp32,		m_SpeedBoostMultiplier,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint32,	m_DamageBoostStartTick,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint32,	m_DamageBoostDuration,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_fp32,		m_DamageBoostMultiplier,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint32,	m_HealthBoostStartTick,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint32,	m_HealthBoostDuration,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint32,	m_HealthBoostAmount,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint32,	m_ShieldBoostStartTick,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint32,	m_ShieldBoostDuration,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint32,	m_InvisibiltyBoostStartTick,	DIRTYMASK_0_4);
	CAUTOVAR_OP(CAutoVar_uint32,	m_InvisibiltyBoostDuration,	DIRTYMASK_0_4);
	
	AUTOVAR_PACK_BEGIN
	AUTOVAR_PACK_VAR(m_IsHuman)
	AUTOVAR_PACK_VAR(m_ActiveBoosts)
	AUTOVAR_PACK_VAR(m_SpeedBoostStartTick)
	AUTOVAR_PACK_VAR(m_SpeedBoostDuration)
	AUTOVAR_PACK_VAR(m_SpeedBoostMultiplier)
	AUTOVAR_PACK_VAR(m_DamageBoostStartTick)
	AUTOVAR_PACK_VAR(m_DamageBoostDuration)
	AUTOVAR_PACK_VAR(m_DamageBoostMultiplier)
	AUTOVAR_PACK_VAR(m_HealthBoostStartTick)
	AUTOVAR_PACK_VAR(m_HealthBoostDuration)
	AUTOVAR_PACK_VAR(m_HealthBoostAmount)
	AUTOVAR_PACK_VAR(m_ShieldBoostStartTick)
	AUTOVAR_PACK_VAR(m_ShieldBoostDuration)
	AUTOVAR_PACK_VAR(m_InvisibiltyBoostStartTick)
	AUTOVAR_PACK_VAR(m_InvisibiltyBoostDuration)
	AUTOVAR_PACK_END

	uint32	m_LastTransformTick;
	int16	m_iDarklingModel;

public:
	CWO_CharShapeshifter_ClientData();
	void Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);

	virtual void Copy(const CWO_Player_ClientData& _CD);

	virtual void Char_UpdateLook(fp32 _FrameFrac);
	virtual void Char_ProcessControl_Look(const CVec3Dfp32& _dLook);
	virtual void Char_ProcessControl_Move(const CVec3Dfp32& _Move);
	virtual void Phys_Move(const CSelection& _Selection, fp32 _dTime, const CVec3Dfp32& _UserVel, bool _bPredicted);
	virtual int OnClientPredictControlFrame(CWorld_Client* _pWClient, CWObject_Client* pPredict, CControlFrame& _Ctrl, bool _bFullFrame, bool _bLog);

	virtual void OnRefresh();
	virtual CWO_CharDarkling_ClientData* IsDarkling();

	int16	m_iHumanModel[CWO_NUMMODELINDICES];
};

#endif // __WObj_Charhapeshifter_ClientData_h__
