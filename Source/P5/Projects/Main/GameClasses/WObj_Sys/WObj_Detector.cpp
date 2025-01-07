#include "PCH.h"

#if !defined(M_DISABLE_TODELETE) || 1

#include "WObj_Detector.h"
#include "../WObj_Char.h"
#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_SimpleMessage.h"




CWObject_Detector_Camera::CWObject_Detector_Camera()
{
	m_FOV = 45.0f;
	m_RangeNear = 64.0f;
	m_RangeFar = 288.0f;
	m_bReactOnPlayer = true;
	m_bReactOnNPCs = false;
}

void CWObject_Detector_Camera::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	// Keys att registrera
	// - Ska reagera på spelaren?
	// - Ska reagera på NPC's?
	// - Inom vilket avstånd kameran ska se objekt
	// - Inom vilket FOV kameran ska se objekt
	
	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
	
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();

	switch (_KeyHash)
	{
	case MHASH1('FOV'): // "FOV"
		{
			m_FOV = KeyValuef;
			break;
		}

	case MHASH3('RANG','E_NE','AR'): // "RANGE_NEAR"
		{
			m_RangeNear = KeyValuef;
			break;
		}

	case MHASH3('RANG','E_FA','R'): // "RANGE_FAR"
		{
			m_RangeFar = KeyValuef;
			break;
		}

	case MHASH4('DETE','CTOR','_FLA','GS'): // "DETECTOR_FLAGS"
		{
			static const char *FlagsStr[] = { "ReactOnPlayer", "ReactOnNPCs", NULL };
			int Flags = KeyValue.TranslateFlags(FlagsStr);

			if ((Flags & 1) == 1)
				m_bReactOnPlayer = true;
			else
				m_bReactOnPlayer = false;

			if ((Flags & 2) == 2)
				m_bReactOnNPCs = true;
			else
				m_bReactOnNPCs = false;
			break;
		}

	default:
		{
			if (KeyName.CompareSubStr("MESSAGE") == 0)
			{
				uint iSlot = atoi(KeyName.Str() + 7);
				m_lMessages.SetMinLen(iSlot + 1);
				m_lMessages[iSlot].Parse(KeyValue, m_pWServer);
			}
			else
				CWObject_Engine_Path::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}

void CWObject_Detector_Camera::OnFinishEvalKeys()
{
	CWObject_Engine_Path::OnFinishEvalKeys();
	m_ClientFlags &= ~CWO_CLIENTFLAGS_NOREFRESH;

	if (!m_bReactOnPlayer && !m_bReactOnNPCs)
		ConOutL(CStr("WARNING: Detector_Camera is setup to ignore both Players and NPC's"));
}

void CWObject_Detector_Camera::OnSpawnWorld()
{
	CWObject_Engine_Path::OnSpawnWorld();

	for(int i = 0; i < m_lMessages.Len(); i++)
		m_lMessages[i].SendPrecache(m_iObject, m_pWServer);
}

void CWObject_Detector_Camera::OnRefresh()
{
	CWObject_Engine_Path::OnRefresh();

	CVec3Dfp32 CameraRot = CVec3Dfp32::GetMatrixRow(GetRenderMatrix(m_pWServer, m_pWServer->GetGameTime(), m_pWServer->GetGameTick(), 0), 0);
	fp32 CamAngle = 1.0f - CVec3Dfp32::AngleFromVector(CameraRot.k[0], CameraRot.k[1]);
	fp32 RangeNear = m_RangeNear*m_RangeNear;
	fp32 RangeFar = m_RangeFar*m_RangeFar;

	// Only select objects within radius
	// Note: Selectoin "characters" means that for instance a rock which is thrown
	// infront of the camera won't be detected.
	const int16* pSelectionList = NULL;
	int nSel, c;
	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pWServer->Selection_AddOnFlagsSet(Selection, OBJECT_FLAGS_CHARACTER);
	nSel = m_pWServer->Selection_Get(Selection, &pSelectionList);

	for (c=0; c<nSel; c++)
	{
		CWObject_Character* pChar = safe_cast<CWObject_Character>(m_pWServer->Object_Get(pSelectionList[c]));
		if (!pChar)
			continue;

		CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(pChar);
		if (pCD)
		{
			if((pCD->m_iPlayer != -1)
			&& (!m_bReactOnPlayer))
				continue;	// This character is player controlled and the camera should not react to player

			if((pCD->m_iPlayer == -1)
			&& (!m_bReactOnNPCs))
				continue;	// This character is an NPC and the camera should not react to NPC's
		}

		CVec3Dfp32 CharacterPos = pChar->GetPosition();
		CVec3Dfp32 CameraPos = CameraPos.GetMatrixRow(GetPositionMatrix(), 3);
		CVec3Dfp32 CharCamPos = CharacterPos - CameraPos;

		// Check so that the distance between the character and camera is ok
		CVec3Dfp32 CharCamPos2D = CharCamPos;
		CharCamPos2D.k[2] = 0.0f;

		float Dist = CharCamPos2D.LengthSqr();
		if((Dist > RangeNear)
		&& (Dist < RangeFar))
		{
			fp32 CharCamAngle = 1.0f - CVec3Dfp32::AngleFromVector(CharCamPos.k[0], CharCamPos.k[1]);
			fp32 FovAngle = CamAngle - CharCamAngle;

			if (M_Fabs(FovAngle) < ((m_FOV * (0.5f*1.0f/360.0f))))
			{
				// Object is within correct range
				const CBox3Dfp32* pOBB = pChar->GetAbsBoundBox();
				CVec3Dfp32 RandRange = pOBB->m_Max - pOBB->m_Min;
				CVec3Dfp32 RandVal;

				for (int i=0; i<3; i++)
				{
					RandRange.k[i] *= 0.5f;
					RandVal.k[i] = MRTC_RAND() % int(RandRange.k[i]);
					RandVal.k[i] += pOBB->m_Min.k[i] + (RandRange.k[i] * 0.5f);
				}


				// Do a linetrace to determine if the character is visible
				CVec3Dfp32 Start, Stop;
				CCollisionInfo CInfo;
				int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
				int32 ObjectFlags = OBJECT_FLAGS_WORLD;// | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
				int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;

				CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
				CInfo.SetReturnValues(0);

				m_pWServer->Debug_RenderWire(CameraPos, RandVal, 0xffff2020, 1.0f);
				if (!m_pWServer->Phys_IntersectLine(CameraPos, RandVal, OwnFlags, ObjectFlags, MediumFlags, &CInfo, pChar->m_iObject))
				{
					// No collision, the character is visible
					ConOut(CStrF("Object %i In FOV", pChar->m_iObject));

					for(int i = 0; i < m_lMessages.Len(); i++)
						m_lMessages[i].SendMessage(m_iObject, pChar->m_iObject, m_pWServer);
				}
			}
		}
	}
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Detector_Camera, CWObject_Engine_Path, 0x0100);


MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Detector_Camera_SecurityLow, CWObject_Detector_Camera, 0x0100);
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Detector_Camera_SecurityNormal, CWObject_Detector_Camera, 0x0100);
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Detector_Camera_SecurityHigh, CWObject_Detector_Camera, 0x0100);

#endif
