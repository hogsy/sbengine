#include "PCH.h"
#include "WObj_Damage.h"
#include "WObj_System.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Damage
|__________________________________________________________________________________________________
\*************************************************************************************************/

CWO_Damage::CWO_Damage(int _Damage, uint32 _DamageType, fp32 _DeliverDelay)
{
	MAUTOSTRIP(CWO_Damage_ctor, MAUTOSTRIP_VOID);

	m_Damage = _Damage;
	m_DamageType = _DamageType;
	m_DeliverDelay = _DeliverDelay;
}

void CWO_Damage::Parse(const char* _Str, const char** _pTypeStr)
{
	MAUTOSTRIP(CWO_Damage_Parse, MAUTOSTRIP_VOID);

	CFStr Str = _Str;

	m_Damage = Str.GetStrSep(",").Val_int();

	if (Str != "")
	{
		CFStr Str2 = Str.GetStrSep(","); Str2.Trim();
		m_DamageType = (uint32)Str2.TranslateFlags(_pTypeStr);

		if (Str != "")
		{
			m_DeliverDelay = (fp32)Str.GetStrSep(",").Val_fp64();
		}
	}
}

bool CWO_Damage::IsValid()
{
	MAUTOSTRIP(CWO_Damage_IsValid, false);

	return (m_Damage > 0 || m_DamageType != 0);
}

int CWO_Damage::Send(int _iTarget, int _iSender, CWorld_Server* _pWServer, const CVec3Dfp32 &_Pos, const char* _pEffect)
{
	MAUTOSTRIP(CWO_Damage_Send, 0);

	CWO_DamageMsg Damage(m_Damage, m_DamageType, &_Pos, NULL, NULL, NULL, -1, _pEffect);
	return Damage.Send(_iTarget, _iSender, _pWServer);
}

int CWO_Damage::SendExt(int _iTarget, int _iSender, CWorld_Server* _pWServer, CCollisionInfo* _pCInfo, const CVec3Dfp32* _pForce, const CVec3Dfp32* _pSplattDir, const char* _pDamageEffect, fp32 _DamageBoost)
{
	MAUTOSTRIP(CWO_Damage_SendExt, 0);
/*
	fp32 DamageBoost = 1.0f;
	if (m_iSkill != -1)
	{
		int Res = m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETDAMAGEBOOST, m_iSkill), _iSender);
		if(Res != 0)
			DamageBoost = fp32(Res) / 256;
	}
*/
	if (IsValid())
	{
		if (_pCInfo != NULL)
		{
			CVec3Dfp32* pPos = &(_pCInfo->m_Pos);

			CVec3Dfp32 Force;
			CVec3Dfp32* pForce;
			if (_pForce != NULL)
			{
				Force = *_pForce * _DamageBoost;
				pForce = &Force;
			}
			else
				pForce = NULL;

			const CVec3Dfp32* pSplattDir;
			if (_pSplattDir != NULL)
				pSplattDir = _pSplattDir;
			else
				if (pForce != NULL)
					pSplattDir = pForce;
				else
					pSplattDir = NULL;

			int SurfaceType = _pCInfo->m_SurfaceType;

			CWO_DamageMsg Msg(int(m_Damage * _DamageBoost), m_DamageType, pPos, pForce, pSplattDir, _pCInfo, SurfaceType, _pDamageEffect);
			return Msg.Send(_iTarget, _iSender, _pWServer);
		}
		else
		{
			CWO_DamageMsg Msg(int(m_Damage * _DamageBoost), m_DamageType, NULL, NULL, NULL, NULL, 0, _pDamageEffect);
			return Msg.Send(_iTarget, _iSender, _pWServer);
		}
	}
	return 0;
}

void CWO_Damage::Merge(CWO_Damage& _DPrimary, CWO_Damage& _DSecondary, CWO_Damage& _DResult)
{
	MAUTOSTRIP(CWO_Damage_Merge, MAUTOSTRIP_VOID);

	_DResult.m_Damage = _DPrimary.m_Damage + _DSecondary.m_Damage;
	_DResult.m_DamageType = _DPrimary.m_DamageType | _DSecondary.m_DamageType;
	_DResult.m_DeliverDelay = _DPrimary.m_DeliverDelay + _DSecondary.m_DeliverDelay;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_DamageMsg
|__________________________________________________________________________________________________
\*************************************************************************************************/

CWO_DamageMsg::CWO_DamageMsg()
{
	MAUTOSTRIP(CWO_DamageMsg_ctor, MAUTOSTRIP_VOID);

	m_Damage = 0;
	m_DamageType = DAMAGETYPE_UNDEFINED;	// DAMAGETYPE_UNDEFINED (0) signifies any damagetype
	m_SurfaceType = -1;	// -1 signifies any hitlocation
	m_StunTicks = -1;

	m_bPositionValid = false;
	m_Position = CVec3Dfp32(0.0f);
	m_bForceValid = false;
	m_Force = CVec3Dfp32(0.0f);
	m_bSplattDirValid = false;
	m_SplattDir = CVec3Dfp32(0.0f,0.0f,1.0f);	// Just to get the vector normalizeable

	m_pCInfo = NULL;
	m_pDamageEffect = NULL;
}

CWO_DamageMsg::CWO_DamageMsg(int _Damage, uint32 _DamageType, const CVec3Dfp32 *_pPos, const CVec3Dfp32 *_pForce, const CVec3Dfp32 *_pSplattDir, CCollisionInfo* _pCInfo, int _SurfaceType, const char* _pDamageEffect, int _StunTicks)
{
	MAUTOSTRIP(CWO_DamageMsg_ctor_2, MAUTOSTRIP_VOID);

	m_Damage = _Damage;
	m_DamageType = _DamageType;
	m_SurfaceType = _SurfaceType;

	if(_pPos)
	{
		m_bPositionValid = true;
		m_Position = *_pPos;
	}
	else
	{
		m_bPositionValid = false;
		m_Position = CVec3Dfp32(0.0f);
	}

	if(_pForce)
	{
		m_bForceValid = true;
		m_Force = *_pForce;
	}
	else
	{
		m_bForceValid = false;
		m_Force = CVec3Dfp32(0.0f);
	}

	if(_pSplattDir)
	{
		m_bSplattDirValid = true;
		m_SplattDir = *_pSplattDir;
	}
	else
	{
		m_bSplattDirValid = false;
		m_SplattDir = CVec3Dfp32(0.0f,0.0f,1.0f);	// Just to get the vector normalizeable
	}

	m_pCInfo = _pCInfo;
	m_pDamageEffect = _pDamageEffect;
	m_StunTicks = _StunTicks;
}

CWObject_Message CWO_DamageMsg::GetMessage(int _iSender)
{
	MAUTOSTRIP(CWO_DamageMsg_GetMessage, CWObject_Message());

	CWObject_Message Msg(OBJMSG_DAMAGE);
	Msg.m_pData = this;
	Msg.m_DataSize = sizeof(*this);
	Msg.m_iSender = _iSender;
	return Msg;
}

int CWO_DamageMsg::Send(int _iTarget, int _iSender, CWorld_PhysState* _pWPhysState)
{
	MAUTOSTRIP(CWO_DamageMsg_Send, 0);

	return _pWPhysState->Phys_Message_SendToObject(GetMessage(_iSender), _iTarget);
}

void CWO_DamageMsg::SendToSelection(const CSelection& _Selection, int _iSender, CWorld_Server* _pWServer)
{
	MAUTOSTRIP(CWO_DamageMsg_SendToSelection, MAUTOSTRIP_VOID);

	_pWServer->Message_SendToSelection(GetMessage(_iSender), _Selection);
}

const CWO_DamageMsg *CWO_DamageMsg::GetSafe(const CWObject_Message &_Msg)
{
	MAUTOSTRIP(CWO_DamageMsg_GetSafe, NULL);

	if(_Msg.m_pData && _Msg.m_DataSize == sizeof(CWO_DamageMsg))
		return (const CWO_DamageMsg *)_Msg.m_pData;

	return NULL;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Shockwave
|__________________________________________________________________________________________________
\*************************************************************************************************/

CWO_Shockwave::CWO_Shockwave(fp32 _ObjectRange, fp32 _CharacterRange, fp32 _Force, int _Damage, uint32 _DamageType, int _DamageDeliveryTicks, uint32 _Mass)
	: m_Damage(_Damage, _DamageType, _DamageDeliveryTicks)
{
	MAUTOSTRIP(CWO_Shockwave_ctor, MAUTOSTRIP_VOID);

	m_Force = _Force;
	m_Mass = _Mass;
	m_ObjectRange = _ObjectRange;
	m_CharacterRange = _CharacterRange;
}

void CWO_Shockwave::Parse(const char* _Str, const char** _pDamageTypeStr)
{
	MAUTOSTRIP(CWO_Shockwave_Parse, MAUTOSTRIP_VOID);

	CFStr Str = _Str;

	m_ObjectRange = Str.GetStrSep(",").Val_int();

	if (Str != "")
	{
		m_Force = Str.GetStrSep(",").Val_int();

		if (Str != "")
		{
			m_Damage.Parse(Str, _pDamageTypeStr);
		}
	}
}

void CWO_Shockwave::Send(const CVec3Dfp32 &_Center, int32 *_pExclude, int _nExclude, int _iSender, CWorld_Server* _pWServer, int _IntersectionFlags)
{
	MAUTOSTRIP(CWO_Shockwave_Send, MAUTOSTRIP_VOID);

	CWO_ShockwaveMsg Msg(_Center, m_ObjectRange, m_CharacterRange, m_Force, m_Damage.m_Damage, m_Damage.m_DamageType, m_Mass);
	Msg.Send(_pExclude, _nExclude, _iSender, _pWServer, _IntersectionFlags);
}

bool CWO_Shockwave::IsValid()
{
	MAUTOSTRIP(CWO_Shockwave_IsValid, false);

	return m_Damage.IsValid() || (m_ObjectRange > 0 && m_CharacterRange > 0 && m_Force > 0);
}

void CWO_Shockwave::Merge(CWO_Shockwave& _DPrimary, CWO_Shockwave& _DSecondary, CWO_Shockwave& _DResult)
{
	MAUTOSTRIP(CWO_Shockwave_Merge, MAUTOSTRIP_VOID);

	// FIXME: Not yet implmented!
	_DResult = _DPrimary;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_ShockwaveMsg
|__________________________________________________________________________________________________
\*************************************************************************************************/

CWO_ShockwaveMsg::CWO_ShockwaveMsg(const CVec3Dfp32 &_Center, fp32 _ObjectRange, fp32 _CharacterRange, fp32 _Force, int _Damage, uint32 _DamageType, int _Mass)
{
	MAUTOSTRIP(CWO_ShockwaveMsg_ctor, MAUTOSTRIP_VOID);

	m_Damage = _Damage;
	m_DamageType = _DamageType;
	m_Center = _Center;
	m_ObjectRange = _ObjectRange;
	m_CharacterRange = _CharacterRange;
	m_Force = _Force;
	m_Mass = _Mass;
}

void CWO_ShockwaveMsg::Send(int32 *_pExclude, int _nExclude, int _iSender, CWorld_Server* _pWServer, int _IntersectionFlags)
{
	MAUTOSTRIP(CWO_ShockwaveMsg_Send, MAUTOSTRIP_VOID);

	CWObject_Message Msg(OBJMSG_RADIALSHOCKWAVE);
	Msg.m_pData = this;
	Msg.m_DataSize = sizeof(*this);
	Msg.m_iSender = _iSender;

	TSelection<CSelection::MEDIUM_BUFFER> Selection;
	int Intersect = _IntersectionFlags;
	Intersect &= ~OBJECT_FLAGS_CHARACTER;
	_pWServer->Selection_AddBoundSphere(Selection, _IntersectionFlags, m_Center, m_ObjectRange);
	for(int i = 0; i < _nExclude; i++)
		_pWServer->Selection_RemoveOnIndex(Selection, _pExclude[i]);
	
	_pWServer->Message_SendToSelection(Msg, Selection);

	Intersect &= ~OBJECT_FLAGS_PHYSOBJECT;
	Intersect |= OBJECT_FLAGS_CHARACTER;
	TSelection<CSelection::MEDIUM_BUFFER> Selection2;
	_pWServer->Selection_AddBoundSphere(Selection2, _IntersectionFlags, m_Center, m_CharacterRange);
	for(int i = 0; i < _nExclude; i++)
		_pWServer->Selection_RemoveOnIndex(Selection2, _pExclude[i]);

	_pWServer->Message_SendToSelection(Msg, Selection2);
}

bool CWO_ShockwaveMsg::GetTracedDamage(int _iObject, const CVec3Dfp32 &_ObjCenter, fp32 _ObjRadius, fp32 _ObjHeight, CWorld_Server *_pWServer, CWO_DamageMsg &_Msg) const
{
	MAUTOSTRIP(CWO_ShockwaveMsg_GetTracedDamage, false);

	fp32 Range = m_ObjectRange;

	CWObject *pObj = _pWServer->Object_Get(_iObject);

	if(pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER)
		Range = m_CharacterRange;

	CCollisionInfo CInfo;
	CInfo.m_CollisionType = CXR_COLLISIONTYPE_PHYSICS;
	CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION);

	if (_pWServer->Phys_IntersectLine(m_Center, _ObjCenter, 0, OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_TRIGGER, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &CInfo))
	{
/*		if (CInfo.m_bIsValid)
		{
			_pWServer->Debug_RenderWire(m_Center, CInfo.m_Pos, 0xFFFF0000, 20.0f);
			_pWServer->Debug_RenderWire(m_Center, _ObjCenter, 0xFFFF00FF, 20.0f);
		}*/

		if (CInfo.m_iObject != _iObject)
		{
			CVec3Dfp32 DistanceVector = _ObjCenter - m_Center;
			fp32 DistanceZ = DistanceVector[2];

			if (M_Fabs(DistanceZ) > (_ObjHeight * 0.5f))
				return false;

			fp32 DistanceXYSqr = CVec2Dfp32(DistanceVector[0], DistanceVector[1]).LengthSqr();
			if (DistanceXYSqr - Sqr(_ObjRadius) > Sqr(Range))
				return false;

		}
	}

	CVec3Dfp32 v = m_Center - _ObjCenter;
	fp32 r = Max(0.0f, Length2(v[0], v[1]) - _ObjRadius);
	fp32 dh = 0;
	if (m_Center[2] < (_ObjCenter[2] - _ObjHeight * 0.5f ))
		dh = (_ObjCenter[2] - _ObjHeight * 0.5f) - m_Center[2];
	else if(m_Center[2] > (_ObjCenter[2] + _ObjHeight * 0.5f))
		dh = m_Center[2] - (_ObjCenter[2] + _ObjHeight * 0.5f);
	
	fp32 d = Length2(dh, r);
	if (d >= Range)
		return 0;

	fp32 Frac = 1.0f - (d / Range);

	_Msg.m_Damage = uint16(m_Damage * Frac);
	_Msg.m_DamageType = m_DamageType;
	_Msg.m_SurfaceType = 0;
	_Msg.m_bForceValid = true;
	_Msg.m_Force = -v.Normalize() * (Frac * m_Force * 0.5f);
	_Msg.m_Force[2] *= 0.2f;

	if (CInfo.m_bIsValid)
	{
		_Msg.m_Position = CInfo.m_Pos;
		_Msg.m_bPositionValid = true;
	}
	else
	{
		_Msg.m_Position = m_Center;
		_Msg.m_bPositionValid = true;
	}
	return true;
}

const CWO_ShockwaveMsg *CWO_ShockwaveMsg::GetSafe(const CWObject_Message &_Msg)
{
	MAUTOSTRIP(CWO_ShockwaveMsg_GetSafe, NULL);

	if(_Msg.m_pData && _Msg.m_DataSize == sizeof(CWO_ShockwaveMsg))
		return (const CWO_ShockwaveMsg *)_Msg.m_pData;
	
	return NULL;
}
