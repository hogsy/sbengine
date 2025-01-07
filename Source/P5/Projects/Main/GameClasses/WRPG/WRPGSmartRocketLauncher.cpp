#include "pch.h"
#include "WRPGAmmo.h"
#include "WRPGSmartROcketLauncher.h"
#include "../WObj_Weapons/WObj_SmartRocket.h"
/*
MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_SmartRocketLauncher, CRPG_Object_Rifle);

bool CRPG_Object_SmartRocketLauncher::Activate(const CMat43fp32 &_Mat, CRPG_Object *_pRoot, int _iObject, int _Input)
{
	if((m_CurrentSelectedAmmo == RIFLESPAWN_APPENDAGE)
	&& (m_Spawn[RIFLESPAWN_APPENDAGE].m_SummonClass != "projectile_smartrocket"))
	{
		// Error, trying to use wrong type of ammunition for this weapon class
		return false;
	};

	if (!CRPG_Object_Rifle::Activate(_Mat, _pRoot, _iObject, _Input))
		return false;

	return true;
}

bool CRPG_Object_SmartRocketLauncher::OnProcess(CRPG_Object* _pRoot, const CMat43fp32& _Mat, int _iObject)
{
	CRPG_Object_Rifle::OnProcess(_pRoot, _Mat, _iObject);
//	if (!CRPG_Object_Rifle::OnProcess(_pRoot, _Mat, _iObject))
//		return false;

	if (m_SpawnedObjectGUID != 0)
	{
		CVec3Dfp32 Start, Stop;
		Start = CVec3Dfp32::GetMatrixRow(_Mat, 3);
		Stop = Start + (CVec3Dfp32::GetMatrixRow(_Mat, 0) * 1000.0f);

		CCollisionInfo CInfo;
		CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
		int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
		int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
		int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
		int32 iExclude = _iObject;
		bool bHit = m_pWServer->Phys_IntersectLine(Start, Stop, OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude);
		if (bHit)
		{
			CVec3Dfp32 HitPos = CInfo.m_Pos;
			CWObject_Message Msg(OBJMSG_SMARTROCKET_SETNEWTARGET);
			Msg.m_VecParam0 = HitPos;
			if (!m_pWServer->Message_SendToObjectGUID(Msg, m_SpawnedObjectGUID))
			{
				ConOut("Couldn't change direction of rocket");
				m_SpawnedObjectGUID = 0;
			};
		}
	};

	return true;
}

void CRPG_Object_SmartRocketLauncher::OnDeltaLoad(CCFile* _pFile)
{
}

void CRPG_Object_SmartRocketLauncher::OnDeltaSave(CCFile* _pFile)
{
}
*/