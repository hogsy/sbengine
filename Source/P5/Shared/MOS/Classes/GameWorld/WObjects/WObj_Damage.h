#ifndef __WOBJ_DAMAGE_H
#define __WOBJ_DAMAGE_H

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Game damage helpers and messages

	Author:			Jens Andersson

	Copyright:		Starbreeze Studios 2003

	Contents:		CWO_Damage
					CWO_DamageMsg
					CWO_Shockwave
					CWO_ShockwaveMsg
\*____________________________________________________________________________________________*/


class CWObject_Message;
class CWorld_Server;
class CWorld_PhysState;
class CCollisionInfo;
//-------------------------------------------------------------------

#include "MMath.h"

//static void SendToSelection(CVec3Dfp32 _Center, fp32 _Radius, CWObject_Message& _Msg, CWorld_Server* _pWServer, int _iExclude = 0);

//-------------------------------------------------------------------
//- CDamageExt ------------------------------------------------------
//-------------------------------------------------------------------
/*
class CWO_DamageExt
{
public:
	// Convert to CDamage
	int m_Damage;
	uint32 m_DamageType;

	// Convert to CShockwave
	fp32 m_SplashRadius;
	fp32 m_SplashForce;
	int m_SplashDamage;

	int m_SurfaceType;

	const CVec3Dfp32 *m_pPosition;
	const CVec3Dfp32 *m_pForce;
	const CVec3Dfp32 *m_pSplatterDirection;

public:
	CWO_DamageExt() { Clear(); };
	CWO_DamageExt(class CWO_Damage& _Damage);
	CWO_DamageExt(class CWO_Shockwave& _Shockwave);
	CWO_DamageExt(const class CWObject_Message &_Msg);

	void Clear();

	int Send(int _iTarget, int _iExcludeFromShockwave, int _iSender, CWorld_Server *_pWServer);

	int GetTracedSplashDamage(int _iObject, const CVec3Dfp32 &_Center, fp32 _Radius, fp32 _Height, CWorld_Server *_pWServer, CVec3Dfp32 *_pVelocity = NULL) const;

	static CWO_DamageExt* GetSafe(const CWObject_Message &_Msg);
};
*/
//-------------------------------------------------------------------
//- CDamage ---------------------------------------------------------
//-------------------------------------------------------------------

// Damage message return codes
enum
{
	DAMAGE_RETURN_NORMAL = 1,
	DAMAGE_RETURN_RICOCHET = 3,			// Used by projectiles
	DAMAGE_RETURN_PASSTHROUGH = 4,		// Used by projectiles
};

class CWO_Damage
{
public:
	int	m_Damage;
	uint32	m_DamageType;
	fp32	m_DeliverDelay;

	CWO_Damage(int _Damage = 0, uint32 _DamageType = 0, fp32 _DeliverDelay = 0.0f);
	void Parse(const char* _Str, const char** _pTypeStr);
	int Send(int _iTarget, int _iSender, CWorld_Server* _pWServer, const CVec3Dfp32 &_Pos, const char* _pEffect = NULL);
	int SendExt(int _iTarget, int _iSender, CWorld_Server* _pWServer, CCollisionInfo* _pCInfo = NULL, const CVec3Dfp32* _pForce = NULL, const CVec3Dfp32* _pSplattDir = NULL, const char* _pDamageEffect = NULL, fp32 _DamageBoost = 1.0f);

	bool IsValid();

	static void Merge(CWO_Damage& _Primary, CWO_Damage& _Secondary, CWO_Damage& _Result);

};

class CWO_DamageMsg
{
public:
	uint16 m_Damage;
	uint32 m_DamageType;
	int8 m_SurfaceType;
	int8 m_StunTicks;

	int m_bPositionValid : 1;
	int m_bForceValid : 1;
	int m_bSplattDirValid : 1;
	CVec3Dfp32 m_Position;
	CVec3Dfp32 m_Force;
	CVec3Dfp32 m_SplattDir;

	const char* m_pDamageEffect;
	CCollisionInfo* m_pCInfo;

	CWO_DamageMsg();
	CWO_DamageMsg(int _Damage, uint32 _DamageType = 0, const CVec3Dfp32 *_pPos = NULL, const CVec3Dfp32 *_pForce = NULL, const CVec3Dfp32 *_pSplattDir = NULL, CCollisionInfo* _pCInfo = NULL, int _SurfaceType = -1, const char* _pDamageEffect = NULL, int _StunTicks = -1);

	CWObject_Message GetMessage(int _iSender);
	
	int Send(int _iTarget, int _iSender, CWorld_PhysState* _pWPhysState);
	void SendToSelection(const CSelection& _Selection, int _iSender, CWorld_Server* _pWServer);

	static const CWO_DamageMsg *GetSafe(const CWObject_Message &_Msg);

};

//-------------------------------------------------------------------
//- CShockwave ------------------------------------------------------
//-------------------------------------------------------------------

class CWO_Shockwave
{
public:
	fp32 m_ObjectRange;
	fp32 m_CharacterRange;
	fp32 m_Force;
	uint32 m_Mass;
	CWO_Damage m_Damage;

	CWO_Shockwave(fp32 _ObjectRange = 0, fp32 _CharacterRange = 0, fp32 _Force = 0, int _Damage = 0, uint32 _DamageType = 0, int _DamageDeliveryTicks = 0, uint32 _Mass = 50);
	void Parse(const char* _Str, const char** _pDamageTypeStr);
	void Send(const CVec3Dfp32 &_Center, int32 *_pExclude, int _nExclude, int _iSender, CWorld_Server* _pWServer, int _IntersectionFlags = OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_TRIGGER);

	bool IsValid();
	
	static void Merge(CWO_Shockwave& _Primary, CWO_Shockwave& _Secondary, CWO_Shockwave& _Result);

};

class CWO_ShockwaveMsg
{
public:
	CVec3Dfp32 m_Center;
	int m_Damage;
	uint32 m_DamageType;
	uint32 m_Mass;
	fp32 m_ObjectRange;
	fp32 m_CharacterRange;
	fp32 m_Force;

	CWO_ShockwaveMsg() {}
	CWO_ShockwaveMsg(const CVec3Dfp32 &_Center, fp32 _ObjectRange, fp32 _CharacterRange, fp32 _Force, int _Damage = 0, uint32 _DamageType = 0, int _Mass = 50);
	
	void Send(int32 *_pExclude, int _nExclude, int _iSender, CWorld_Server* _pWServer, int _IntersectionFlags = OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_TRIGGER);
	bool GetTracedDamage(int _iObject, const CVec3Dfp32 &_ObjCenter, fp32 _ObjRadius, fp32 _ObjHeight, CWorld_Server *_pWServer, CWO_DamageMsg &_Msg) const;

	static const CWO_ShockwaveMsg *GetSafe(const CWObject_Message &_Msg);
};

//-------------------------------------------------------------------

#endif /* WObj_Damage_h */
