/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			Client data for CWObject_CharShapeshifter.

Author:			Roger Mattsson

Copyright:		2005 Starbreeze Studios AB

Contents:		CWObject_CharShapeshifter_ClientData

Comments:

History:		
051117:		Created file
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_CharShapeshifter_ClientData.h"
#include "../../GameWorld/WClientMod_Defines.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Charhapeshifter_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT(CWO_CharShapeshifter_ClientData, CWO_CharShapeshifter_ClientData::parent_darkling);

CWO_CharShapeshifter_ClientData::CWO_CharShapeshifter_ClientData()
{

}


void CWO_CharShapeshifter_ClientData::Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	parent_darkling::Clear(_pObj, _pWPhysState);

	m_IsHuman = 1;
	m_LastTransformTick = 0;

	m_ActiveBoosts = 0;

	m_SpeedBoostStartTick = 0;
	m_SpeedBoostDuration = 0;
	m_SpeedBoostMultiplier = 0.0f;

	m_DamageBoostStartTick = 0;
	m_DamageBoostDuration = 0;
	m_DamageBoostMultiplier = 1.0f;

	m_HealthBoostStartTick = 0;
	m_HealthBoostDuration = 0;
	m_HealthBoostAmount = 0;

	m_ShieldBoostStartTick = 0;
	m_ShieldBoostDuration = 0;

	m_InvisibiltyBoostStartTick = 0;
	m_InvisibiltyBoostDuration = 0;
	m_iDarklingModel = 0;
}


void CWO_CharShapeshifter_ClientData::Copy(const CWO_Player_ClientData& _CD)
{
	const CWO_CharShapeshifter_ClientData& CD = *safe_cast<const CWO_CharShapeshifter_ClientData>(&_CD);
	m_LastTransformTick = CD.m_LastTransformTick;

	m_ActiveBoosts = CD.m_ActiveBoosts;
	m_SpeedBoostStartTick = CD.m_SpeedBoostStartTick;
	m_SpeedBoostDuration = CD.m_SpeedBoostDuration;
	m_SpeedBoostMultiplier = CD.m_SpeedBoostMultiplier;
	m_DamageBoostStartTick = CD.m_DamageBoostStartTick;
	m_DamageBoostDuration = CD.m_DamageBoostDuration;
	m_DamageBoostMultiplier = CD.m_DamageBoostMultiplier;
	m_HealthBoostStartTick = CD.m_HealthBoostStartTick;
	m_HealthBoostDuration = CD.m_HealthBoostDuration;
	m_HealthBoostAmount = CD.m_HealthBoostAmount;
	m_ShieldBoostStartTick = CD.m_ShieldBoostStartTick;
	m_ShieldBoostDuration = CD.m_ShieldBoostDuration;
	m_InvisibiltyBoostStartTick = CD.m_InvisibiltyBoostStartTick;
	m_InvisibiltyBoostDuration = CD.m_InvisibiltyBoostDuration;
	m_iDarklingModel = CD.m_iDarklingModel;

	if(m_IsHuman)
		parent_human::Copy(_CD);
	else
		parent_darkling::Copy(_CD);
}


void CWO_CharShapeshifter_ClientData::Char_UpdateLook(fp32 _FrameFrac)
{
	if(m_IsHuman)
		parent_human::Char_UpdateLook(_FrameFrac);
	else
		parent_darkling::Char_UpdateLook(_FrameFrac);
}


void CWO_CharShapeshifter_ClientData::Char_ProcessControl_Look(const CVec3Dfp32& _dLook)
{
	if(m_IsHuman)
		parent_human::Char_ProcessControl_Look(_dLook);
	else
		parent_darkling::Char_ProcessControl_Look(_dLook);
}


void CWO_CharShapeshifter_ClientData::Char_ProcessControl_Move(const CVec3Dfp32& _Move)
{
	if(m_IsHuman)
		parent_human::Char_ProcessControl_Move(_Move);
	else
		parent_darkling::Char_ProcessControl_Move(_Move);
}


void CWO_CharShapeshifter_ClientData::Phys_Move(const CSelection& _Selection, fp32 _dTime, const CVec3Dfp32& _UserVel, bool _bPredicted)
{
	if(m_IsHuman)
		parent_human::Phys_Move(_Selection, _dTime, _UserVel, _bPredicted);
	else
		parent_darkling::Phys_Move(_Selection, _dTime, _UserVel, _bPredicted);
}


int CWO_CharShapeshifter_ClientData::OnClientPredictControlFrame(CWorld_Client* _pWClient, CWObject_Client* pPredict, CControlFrame& _Ctrl, bool _bFullFrame, bool _bLog)
{
	if(m_IsHuman)
		return parent_human::OnClientPredictControlFrame(_pWClient, pPredict, _Ctrl, _bFullFrame, _bLog);
	else
		return parent_darkling::OnClientPredictControlFrame(_pWClient, pPredict, _Ctrl, _bFullFrame, _bLog);
}


void CWO_CharShapeshifter_ClientData::OnRefresh()
{
	parent_darkling::OnRefresh();

	if (m_iPlayer != -1)
	{
		// Player darklings can only climb while pressing the left shoulder button (secondary fire)
		if (m_Control_Press & CONTROLBITS_SECONDARY)
		{
			m_Flags = m_Flags | DARKLING_FLAGS_CAN_CLIMB;
		}
		else
		{
			m_Flags = m_Flags & ~DARKLING_FLAGS_CAN_CLIMB;
			m_Flags = m_Flags & ~DARKLING_FLAGS_STEEP_SLOPE_AHEAD;
			SetGravity(CVec3Dfp32(0,0,-1));
		}
	}
}


CWO_CharDarkling_ClientData* CWO_CharShapeshifter_ClientData::IsDarkling()
{
	return m_IsHuman ? NULL : this;
}











