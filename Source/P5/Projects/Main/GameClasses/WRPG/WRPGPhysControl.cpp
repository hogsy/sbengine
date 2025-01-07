/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WRPGPhysControl
					
	Author:			Anton Ragnarsson
					
	Copyright:		
					
	Contents:		CRPG_Object_PhysControl
					
	History:		
		050613:		Created File
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WRPGWeapon.h"
#include "WRPGChar.h"
#include "../WObj_Char.h"


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CRPG_Object_PhysControl

	Comments:		Used for physics debugging
\*____________________________________________________________________*/
class CRPG_Object_PhysControl : public CRPG_Object_Weapon
{
	MRTC_DECLARE;

	bool m_bActive;
	int  m_iOwner;
	int  m_iPhysObject;
	fp32  m_Distance;
	CVec3Dfp32 m_GrabPos; // Position of 'grab' point (in object space)

	CVec3Dfp32 m_LastDirection, m_LastCharacterPos, m_LastObjectPos;
	CVec3Dfp32 m_PidIntegrateTerm, m_PidDeriveTerm;

public:
	bool m_bTriggerPressed;

	virtual void OnCreate();
	virtual bool DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout = true);

	virtual bool Activate(const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, int _Input);
	virtual bool Deactivate(const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, int _Input);
	virtual bool OnProcess(CRPG_Object* _pRoot, int _iObject);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Implementation
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_DYNAMIC(CRPG_Object_PhysControl, CRPG_Object_Weapon);


void CRPG_Object_PhysControl::OnCreate()
{
	CRPG_Object_Weapon::OnCreate();

	m_bActive = false;
	m_Flags |= RPG_ITEM_FLAGS_LASERBEAM;
}


bool CRPG_Object_PhysControl::DrawAmmo(int _Num, const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, bool _bApplyFireTimeout)
{
	return true;
}

bool CRPG_Object_PhysControl::Activate(const CMat4Dfp32& _Mat, CRPG_Object* _pRoot, int _iObject, int _Input)
{
	if (!m_bActive)
	{
		CWObject_Character* pChar = CWObject_Character::IsCharacter(_iObject, m_pWServer);
		if (!pChar)
			return false;

		CMat4Dfp32 Mat;
		pChar->AI_GetHeadMat(Mat);

//		uint32 nObjectFlags = OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
		CVec3Dfp32 StartPos = Mat.GetRow(3);
		m_LastCharacterPos = StartPos;
		m_LastDirection = Mat.GetRow(0);
		m_PidIntegrateTerm = CVec3Dfp32(0,0,0);
		m_PidDeriveTerm = CVec3Dfp32(0,0,0);
		CVec3Dfp32 EndPos = StartPos + Mat.GetRow(0) * 2000.0f;

		CCollisionInfo CInfo;
		bool bHit = m_pWServer->Phys_IntersectLine(StartPos, EndPos, 
			OBJECT_FLAGS_PROJECTILE, OBJECT_FLAGS_PHYSOBJECT/* | OBJECT_FLAGS_PHYSMODEL*/, 
			XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS, &CInfo, _iObject);
		if (bHit && CInfo.m_bIsValid)
		{
			CWObject* pObj = m_pWServer->Object_Get(CInfo.m_iObject);
			if (pObj && pObj->m_pRigidBody2)
			{
				m_bActive = true;
				m_iPhysObject = pObj->m_iObject;
				m_Distance = (pObj->GetPosition() - StartPos).Length();

				if (m_Distance > 7*32.0)
					m_Distance = 7*32.0;

				m_LastObjectPos = pObj->GetPosition();
	//			m_Distance = (CInfo.m_Pos - StartPos).Length();

				m_GrabPos = CInfo.m_LocalPos;

				M_TRACE("CRPG_Object_PhysControl::Activate, _iObject = %d (%s)\n", 	_iObject, m_pWServer->Object_GetName(_iObject));
				M_TRACE(" - Hit object %d (%s)\n", m_iPhysObject, m_pWServer->Object_GetName(m_iPhysObject));
			}
		}
	}
	return true;
}


bool CRPG_Object_PhysControl::Deactivate(const CMat4Dfp32 &_Mat, CRPG_Object* _pRoot, int _iObject, int _Input)
{
	if (m_bActive)
	{
		//CWObject_CoreData *pCoreData = m_pWServer->Object_GetCD(m_iPhysObject);
		//pCoreData->m_pRigidBody->SetActive(true);

		m_bActive = false;
		m_iPhysObject = 0;
		M_TRACE("CRPG_Object_PhysControl::Deactivate\n");
	}
	return true;
}


bool CRPG_Object_PhysControl::OnProcess(CRPG_Object* _pRoot, int _iObject)
{
	if (m_bActive)
	{
		CWObject_CoreData* pObj = m_pWServer->Object_GetCD(m_iPhysObject);
		if (!pObj)
		{
			m_iPhysObject = 0;
			m_bActive = false;
			return true;
		}

		CWObject_Character* pChar = CWObject_Character::IsCharacter(_iObject, m_pWServer);
		if (pChar)
		{
			CMat4Dfp32 Mat;
			pChar->AI_GetHeadMat(Mat);
			CVec3Dfp32 Dir = Mat.GetRow(0);
			CVec3Dfp32 Pos = Mat.GetRow(3);

			if ((Dir - m_LastDirection).Length() < 0.05f)
				Dir = m_LastDirection;
			else
				m_LastDirection = Dir;

			if ((Pos - m_LastCharacterPos).Length() < 1.0f)
				Pos = m_LastCharacterPos;
			else
				m_LastCharacterPos = Pos;

			const CMat4Dfp32& ObjPosMat = pObj->GetPositionMatrix();

			CVec3Dfp32 Offset = m_GrabPos;
			Offset.MultiplyMatrix3x3(ObjPosMat);

			CVec3Dfp32 NewPos = Pos + Dir * m_Distance - Offset;
			CVec3Dfp32 OldPos = ObjPosMat.GetRow(3);
			CVec3Dfp32 Diff = NewPos - OldPos;

			fp32 Mass = pObj->GetMass();

			fp32 K = 2.0f * Mass;
			fp32 Ti = 350.0f;
			fp32 Td = 5.0f;
			fp32 N = 10.0f;
			fp32 h = 1.0f;
			m_PidIntegrateTerm = m_PidIntegrateTerm + Diff*(K/Ti);
			m_PidDeriveTerm = m_PidDeriveTerm*(Td / (Td + N*h)) - (OldPos - m_LastObjectPos)*(K*Td*N/(Td + N*h));

			m_pWServer->Phys_AddForce(m_iPhysObject, CVec3Dfp32(0.0f, 0.0f, 9.8f*Mass)); // Cancel out gravity
			m_pWServer->Phys_AddForce(m_iPhysObject, Diff*K + m_PidIntegrateTerm + m_PidDeriveTerm);
			//m_pWServer->Phys_AddImpulse(m_iPhysObject, OldPos, Diff*20, 0.2, 1.0);

			m_LastObjectPos = OldPos;
		}
	}
	return true;
}

