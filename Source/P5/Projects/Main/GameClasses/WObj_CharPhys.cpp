#include "PCH.h"

#include "WObj_Char.h"
#include "../GameWorld/WClientMod_Defines.h"
#include "WRPG/WRPGSpell.h"
#include "WObj_Misc/WObj_Model_Ladder.h"
#include "WObj_Misc/WObj_Ledge.h"
#include "WObj_Char/WObj_CharDarkling_ClientData.h"

#include "../../../Shared/Mos/XR/Phys/WPhysPCS_Enable.h"
#include "../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Game.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character movement
					
	Contents:		Char_SetPhysics
					Phys_GetUserAccelleration
					Phys_Move
					PhysUtil_GetMediumAccelleration
					OnPhysicsEvent
					OnIntersectLine
					OnClientPreIntersection
					etc..
\*____________________________________________________________________________________________*/

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CGrabbedObject
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CGrabbedObject::Init(CWorld_PhysState& _WPhysState, uint16 _iObject, const CVec3Dfp32& _Look)
{
	CWObject_CoreData* pObj = _WPhysState.Object_GetCD(_iObject);
	if (pObj)
	{
		if (pObj->GetMass() > 40.0f)
		{
			ConOutL(CStrF("Object too heavy! (%.1f kg, limit is 40 kg)", pObj->GetMass()));
			return;
		}

		m_iTargetObj = _iObject;
		const CMat4Dfp32& ObjMat = pObj->GetPositionMatrix();

		CVec3Dfp32 Center;
		pObj->GetAbsBoundBox()->GetCenter(m_Pid[0].m_LastPos);
		m_Pid[1].m_LastPos = m_Pid[0].m_LastPos + _Look * 16.0f;

		CMat4Dfp32 InvObjMat;
		ObjMat.InverseOrthogonal(InvObjMat);
		for (uint i = 0; i < 2; i++)
		{
			m_Pid[i].m_IntegrateTerm = CVec3Dfp32(0.0f);
			m_Pid[i].m_DeriveTerm = CVec3Dfp32(0.0f);
			m_Pid[i].m_LocalGrabPos = m_Pid[i].m_LastPos;
			m_Pid[i].m_LocalGrabPos *= InvObjMat;
		}
	}
}

void CGrabbedObject::Release()
{
	m_iTargetObj = 0;
}

bool CGrabbedObject::Update(CWorld_Server& _WServer, const CVec3Dfp32& _WantedPos, const CVec3Dfp32& _Look)
{
	CWObject_CoreData* pObj = _WServer.Object_GetCD(m_iTargetObj);
	if (!pObj)
		return false;

	CVec3Dfp32 lApplyPos[2], lGrabPos[2], lForce[2];
	lApplyPos[0] = m_Pid[0].m_LastPos;
	lApplyPos[1] = m_Pid[1].m_LastPos;
	lGrabPos[0] = _WantedPos;
	lGrabPos[1] = _WantedPos + _Look * 16.0f;

	const CMat4Dfp32& ObjMat = pObj->GetPositionMatrix();
	fp32 Mass = pObj->GetMass();

	// Update PID
	for (uint i = 0; i < 2; i++)
	{
		CVec3Dfp32 CurrPos = m_Pid[i].m_LocalGrabPos;
		CurrPos *= ObjMat;

		CVec3Dfp32 Diff = lGrabPos[i] - CurrPos;
		fp32 K = 0.5f * Mass;
		fp32 Ti = 350.0f;
		fp32 Td = 5.0f;
		fp32 N = 10.0f;
		fp32 h = 1.0f;
		m_Pid[i].m_IntegrateTerm += Diff * (K/Ti);
		m_Pid[i].m_DeriveTerm *= (Td / (Td + N*h));
		m_Pid[i].m_DeriveTerm -= (CurrPos - m_Pid[i].m_LastPos) * (K*Td*N/(Td + N*h));
		m_Pid[i].m_LastPos = CurrPos;

		lForce[i] = (Diff*K + m_Pid[i].m_IntegrateTerm + m_Pid[i].m_DeriveTerm);
	}

	// Add forces to adjust position
	CVec3Dfp32 AvgForce = (lForce[0] + lForce[1]) * 0.5f;
	_WServer.Phys_AddForce(m_iTargetObj, AvgForce);
	_WServer.Phys_AddForce(m_iTargetObj, CVec3Dfp32(0, 0, 9.81f*Mass)); // Cancel out gravity

	// Add impulses to adjust rotation
	lForce[0] -= AvgForce;
	lForce[1] -= AvgForce;
	_WServer.Phys_AddImpulse(m_iTargetObj, lApplyPos[0], lForce[0]);
	_WServer.Phys_AddImpulse(m_iTargetObj, lApplyPos[1], lForce[1]);

	// Damp velocity to get rid of spinning
	CVelocityfp32 Vel = pObj->GetVelocity();
	Vel.m_Rot.m_Angle *= 0.98f;
	_WServer.Object_SetVelocity(m_iTargetObj, Vel);

	return true;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Character
|__________________________________________________________________________________________________
\*************************************************************************************************/
#ifndef	M_RTM
#define	ENABLE_PHYSLOG
#endif

#define AIMING_TYPE_MINIGUN 7

// Also defined in XWBSP_NavGrid.cpp
#ifdef PLATFORM_PS2
#define PLAYER_PHYS_COS_MAX_SLOPE 0.69465837045899728665640629942269f	// 46 degrees
#else
#define PLAYER_PHYS_COS_MAX_SLOPE 0.69465837045899728665640629942269f	// 46 degrees
#endif

CVec3Dfp32 CWObject_Character::Char_GetPhysRelPos(CWObject_CoreData* _pObj, int _Type)
{
	MAUTOSTRIP(CWObject_Character_Char_GetPhysRelPos, CVec3Dfp32());
	CVec3Dfp32 Ret(0);
	switch(_Type)
	{
	case PLAYER_PHYS_STAND :
		break;
	case PLAYER_PHYS_CROUCH :
		break;
	case PLAYER_PHYS_DEAD :
		{
/*			CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
			if (!pCD) return 0;
			Ret.k[2] = (uint8) pCD->m_Phys_DeadRadius;*/
		}
		break;
	case PLAYER_PHYS_NOCLIP :
	case PLAYER_PHYS_NOCLIP2 :
		break;
	default:
		{
		}
	}

	return Ret;
}

// tries to relocate position in different directions, and moves the position as little as possible
static bool FindValidPosition(CWorld_Server* _pWServer, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _CurrPosMat, CVec3Dfp32& _NewPos)
{
	CVec3Dfp32 NewPos = _NewPos;
	CMat4Dfp32 NewPosMat = _CurrPosMat;

	fp32 Closest = -_FP32_MAX;
	CVec3Dfp32 BestPos = NewPos;
	for (int i = 0; i < 6; i++)
	{
		NewPosMat.GetRow(3) = NewPos;
		NewPosMat.GetRow(3).k[i/2] += 24.0f * (1-(i&1)*2); // cycle through +x,-x,+y,-y,+z,-z

		// check if offset position is valid
		if (!_pWServer->Phys_IntersectWorld(NULL, _PhysState, NewPosMat, NewPosMat, -1))
		{
			// ok, we found a valid position, now let's see how close we can get
			CCollisionInfo CInfo;
			if (_pWServer->Phys_IntersectWorld(NULL, _PhysState, NewPosMat, _CurrPosMat, -1, &CInfo) && CInfo.m_bIsValid)
			{
				if (CInfo.m_Time > Closest)
				{
					Closest = CInfo.m_Time;
					NewPosMat.GetRow(3).Lerp(_CurrPosMat.GetRow(3), CInfo.m_Time-0.05f, BestPos); // update bestpos
				}
			}
		}
	}
	if (Closest >= 0.0f)
	{
		_NewPos = BestPos + (BestPos - NewPos).SetLength(0.1f);
		return true;
	}
	return false;
}


int CWObject_Character::Char_SetPhysics(CWObject_CoreData* _pObj, CWorld_PhysState* _pPhysState, CWorld_Server* _pWServer, int _Type, int _bTele, bool _bReset)
{
	MAUTOSTRIP(CWObject_Character_Char_SetPhysics, 0);
	const int PhysType = CWObject_Character::Char_GetPhysType(_pObj);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if (!pCD) return 0;
	if (_Type == PhysType && !_bReset && !(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_FORCESETPHYS)) 
		return 1;

	if(_Type == 0)
		return 1;

	MSCOPE(CWObject_Character::Char_SetPhysics, Character);

	bool bIsMP = false;
	if(_pWServer && pCD->m_iPlayer != -1 && _pWServer->Message_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), _pWServer->Game_GetObjectIndex()))
		bIsMP = true;

	// Get old phys state
	CWO_PhysicsState Phys = _pObj->GetPhysState();
	int OldIntersectFlags = Phys.m_ObjectIntersectFlags;

	CVec3Dfp32 CurrentRelPos = Char_GetPhysRelPos(_pObj, PhysType);
	CVec3Dfp32 RelPos = Char_GetPhysRelPos(_pObj, _Type);

	uint32 ObjectFlags = OBJECT_FLAGS_CHARACTER | OBJECT_FLAGS_WORLDTELEPORT | OBJECT_FLAGS_PHYSOBJECT;
	if(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_GHOST || pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOWORLDCOLL)
		ObjectFlags &= ~OBJECT_FLAGS_PHYSOBJECT;

	int32 IntersectionFlags = OBJECT_FLAGS_PLAYERPHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_ANIMPHYS;

	if (pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOPHYSFLAG || (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOANIMPHYS))
		IntersectionFlags &= ~OBJECT_FLAGS_ANIMPHYS;

	if (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOCHARACTERCOLL || (pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOCHARCOLL))
	{
		// If you remove OBJECT_FLAGS_PHYSOBJECT from intersectionflags you can move through doors, 
		// if you remove PLAYERPHYSMODEL you will fall through the world... So remove PHYSOBJECT from 
		// objectflags and you can move through characters but nothing else???
		ObjectFlags &= ~OBJECT_FLAGS_PHYSOBJECT;
		//IntersectionFlags &= ~(/*OBJECT_FLAGS_PLAYERPHYSMODEL |*/ OBJECT_FLAGS_PHYSOBJECT);
	}

	if (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOWORLDCOLL)
		IntersectionFlags = 0;

	if (!(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOPROJECTILECOLL))
		IntersectionFlags |= OBJECT_FLAGS_PROJECTILE;

	if(_Type == PLAYER_PHYS_CROUCH && _pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOCROUCH)
		_Type = PLAYER_PHYS_STAND;

	if ((_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_FORCECROUCH) && (_Type != PLAYER_PHYS_DEAD))
		_Type = PLAYER_PHYS_CROUCH;
	
	if (_Type == PLAYER_PHYS_DEAD)
		IntersectionFlags &= ~(/*OBJECT_FLAGS_PHYSMODEL | */OBJECT_FLAGS_PHYSOBJECT);

	if(pCD->m_iPlayer != -1)
		ObjectFlags |= OBJECT_FLAGS_PLAYER;
	/*else
		ObjectFlags |= OBJECT_FLAGS_USEABLE;*/
		
	int PhysFlags = /*| OBJECT_PHYSFLAGS_PUSHER*/ OBJECT_PHYSFLAGS_SLIDEABLE | 
		/*OBJECT_PHYSFLAGS_FRICTION | OBJECT_PHYSFLAGS_ROTFRICTION_SLIDE | */OBJECT_PHYSFLAGS_PHYSMOVEMENT |
		OBJECT_PHYSFLAGS_OFFSET;

	if(!(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_NONPUSHABLE))
		PhysFlags |= OBJECT_PHYSFLAGS_PUSHABLE;
	
/*	if(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_MOUNTED)
		Phys.m_iExclude = (uint16)pCD->m_iMountedObject;*/

	if ((pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_NOCOLLISIONSUPPORTED) && 
		((pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_NOCOLLISION) || 
		(pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_INVISIBLE)))
	{
		IntersectionFlags &= ~OBJECT_FLAGS_PHYSOBJECT;
		ObjectFlags &= OBJECT_FLAGS_WORLDTELEPORT | OBJECT_FLAGS_PLAYER;
	}

	Phys.m_MediumFlags |= XW_MEDIUM_PLAYERSOLID | XW_MEDIUM_DYNAMICSSOLID;
	if (pCD->m_iPlayer == -1)
		Phys.m_MediumFlags |= XW_MEDIUM_AISOLID;

//	if(bIsMP)
//		IntersectionFlags |= OBJECT_FLAGS_CHARACTER;

	// Don't want any collision at all
	if (pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOCOLLISION)
	{
		ObjectFlags = 0;
		IntersectionFlags = 0;
	}

	//	_pObj->m_PhysFlags = PHYSICAL_FLAGS_MEDIUMACCEL | PHYSICAL_FLAGS_MEDIUMROTATE;
	switch(_Type)
	{
	case PLAYER_PHYS_STAND :
		{
			// Stand
			if (pCD->IsDarkling())
			{
				Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_SPHERE, -1, 16.0f, CVec3Dfp32(0,0,0));
			}
			else
			{
				Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(pCD->m_Phys_Width, pCD->m_Phys_Width, pCD->m_Phys_Height), CVec3Dfp32(0,0,pCD->m_Phys_Height));
			}
//			Phys.m_Prim[1].Create(OBJECT_PRIMTYPE_SPHERE, -1, CVec3Dfp32(14.0f, 0, 0), CVec3Dfp32(0,0,PLAYER_PHYSHEIGHT));
			Phys.m_nPrim = 1;
			Phys.m_ObjectFlags = ObjectFlags;
			if (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_GHOST)
				Phys.m_ObjectIntersectFlags = 0;
			else
				Phys.m_ObjectIntersectFlags = IntersectionFlags;
			Phys.m_PhysFlags = PhysFlags;

			// chech "use_stairs" needed for feet-IK.
			if(_pWServer && pCD->m_iPlayer == -1)
			{
				CWObject_Character *pChar  = safe_cast<CWObject_Character>(_pObj);
				CAI_Core* pAI = pChar->AI_GetAI();
				if (pAI &&
					(pAI->m_CharacterClass != CAI_Core::CLASS_DARKLING) && 
					(pAI->m_UseFlags & CAI_Core::USE_STAIRS) && 
					(!pAI->m_bWallClimb))
				{
					Phys.m_ObjectIntersectFlags |= OBJECT_FLAGS_CREEPING_DARK;
				}	
			}	
			
		}
		break;
	case PLAYER_PHYS_CROUCH :
		{
			// Crouch
			// Same as stand if darkling
			if (pCD->IsDarkling())
			{
				Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_SPHERE, -1, 16.0f, CVec3Dfp32(0,0,0));
			}
			else
			{
				Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(pCD->m_Phys_Width, pCD->m_Phys_Width, pCD->m_Phys_HeightCrouch), CVec3Dfp32(0,0,pCD->m_Phys_HeightCrouch));
			}
//			Phys.m_Prim[1].Create(OBJECT_PRIMTYPE_SPHERE, -1, CVec3Dfp32(14.0f, 0, 0), CVec3Dfp32(0,0,16));
			Phys.m_nPrim = 1;
			Phys.m_ObjectFlags = ObjectFlags;
			if (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_GHOST)
				Phys.m_ObjectIntersectFlags = 0;
			else
				Phys.m_ObjectIntersectFlags = IntersectionFlags;
			Phys.m_PhysFlags = PhysFlags;
		}
		break;
	case PLAYER_PHYS_DEAD :
		{
			// Dead, might not be enough place for box, use current box
			/*Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(pCD->m_Phys_Width, pCD->m_Phys_Width, pCD->m_Phys_HeightCrouch), CVec3Dfp32(0,0,pCD->m_Phys_HeightCrouch));
//			Phys.m_Prim[1].Create(OBJECT_PRIMTYPE_SPHERE, -1, CVec3Dfp32(14.0f, 0, 0), CVec3Dfp32(0,0,16));
			Phys.m_nPrim = 1;*/
			ObjectFlags &= OBJECT_FLAGS_WORLDTELEPORT | OBJECT_FLAGS_PLAYER;
			// Make the character "pickupable" and sensitive to corpse trigger
			if(pCD->m_iPlayer == -1)
				ObjectFlags |= OBJECT_FLAGS_PICKUP | OBJECT_FLAGS_TRIGGER;
			Phys.m_ObjectFlags = ObjectFlags;
			if (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_GHOST)
				Phys.m_ObjectIntersectFlags = 0;
			else
				Phys.m_ObjectIntersectFlags = IntersectionFlags;
			Phys.m_PhysFlags = PhysFlags;

/*			Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_SPHERE, -1, CVec3Dfp32(m_Phys_DeadRadius, 0, 0), 0);
			Phys.m_nPrim = 1;
			Phys.m_ObjectFlags = OBJECT_FLAGS_WORLDTELEPORT;
			Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL;
//			Phys.m_PhysFlags = OBJECT_PHYSFLAGS_PUSHABLE | OBJECT_PHYSFLAGS_PUSHER | OBJECT_PHYSFLAGS_SLIDEABLE | OBJECT_PHYSFLAGS_FRICTION | OBJECT_PHYSFLAGS_PHYSMOVEMENT;
			Phys.m_PhysFlags = 
				OBJECT_PHYSFLAGS_ROTATION |
				OBJECT_PHYSFLAGS_PUSHABLE | 
//				OBJECT_PHYSFLAGS_PUSHER | 
				OBJECT_PHYSFLAGS_SLIDEABLE | 
				OBJECT_PHYSFLAGS_FRICTION | 
				OBJECT_PHYSFLAGS_PHYSMOVEMENT |
				OBJECT_PHYSFLAGS_ROTFRICTION_SLIDE |
				OBJECT_PHYSFLAGS_ROTFRICTION_ROT;*/
		}
		break;


	case PLAYER_PHYS_NOCLIP2 :
	case PLAYER_PHYS_NOCLIP :
		{
			// NoClip
			Phys.m_nPrim = 0;
			ObjectFlags &= OBJECT_FLAGS_WORLDTELEPORT | OBJECT_FLAGS_PLAYER;
//			Phys.m_ObjectFlags = ObjectFlags;
			Phys.m_ObjectFlags = OBJECT_FLAGS_WORLDTELEPORT;
			Phys.m_ObjectIntersectFlags = 0;
			Phys.m_PhysFlags = OBJECT_PHYSFLAGS_PHYSMOVEMENT;
			
			if (_Type == PLAYER_PHYS_NOCLIP2)
			{
				Phys.m_ObjectFlags |= OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
				Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
				Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(PLAYER_PHYS_NOCLIP2_SIZE, PLAYER_PHYS_NOCLIP2_SIZE, PLAYER_PHYS_NOCLIP2_SIZE), CVec3Dfp32(0,0,1.66f*32.0f-PLAYER_PHYS_NOCLIP2_SIZE));
				Phys.m_nPrim = 1;
				Phys.m_PhysFlags = 
					OBJECT_PHYSFLAGS_PUSHABLE | 
					OBJECT_PHYSFLAGS_SLIDEABLE | 
					OBJECT_PHYSFLAGS_PHYSMOVEMENT |
					OBJECT_PHYSFLAGS_OFFSET;
			}

//			_pObj->m_PhysFlags = 0;
		}
		break;
	case PLAYER_PHYS_SPECTATOR :
		{
			// NoClip
			Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(pCD->m_Phys_SpectatorSize, pCD->m_Phys_SpectatorSize, pCD->m_Phys_SpectatorSize), CVec3Dfp32(0,0,pCD->m_Phys_SpectatorSize));
			Phys.m_nPrim = 1;
			ObjectFlags &= OBJECT_FLAGS_WORLDTELEPORT | OBJECT_FLAGS_PLAYER;
//			Phys.m_ObjectFlags = ObjectFlags;
			Phys.m_ObjectFlags = OBJECT_FLAGS_WORLDTELEPORT;
			Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PLAYERPHYSMODEL;
			Phys.m_PhysFlags = 
				OBJECT_PHYSFLAGS_PUSHABLE | 
				OBJECT_PHYSFLAGS_SLIDEABLE | 
				OBJECT_PHYSFLAGS_PHYSMOVEMENT |
				OBJECT_PHYSFLAGS_OFFSET;
//			_pObj->m_PhysFlags = 0;
		}
		break;
	default:
		{
			Error_static("CWObject_Character::Char_SetPhysics", "Invalid type.");
		}
	}

	if (pCD->m_iPlayer != -1)
		Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_NODYNAMICSUPDATE;

/*	// Disable character collision for darklings		-- this will make triggers not react to darklings. disabled until a better solution is found /anton
	if (pCD->IsDarkling() && pCD->m_iPlayer == -1)
		Phys.m_Prim[0].m_ObjectFlagsMask = ~OBJECT_FLAGS_CHARACTER;*/

//	if(pCD->m_iPlayer != -1)
//		Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_PUSHER;

	CVec3Dfp32 RelMove;
	RelPos.Sub(CurrentRelPos, RelMove);
	CVec3Dfp32 NewPos(_pObj->GetLocalPosition());
	NewPos += RelMove;
	int Ret = _pPhysState->Object_SetPhysics(_pObj->m_iObject, Phys, NewPos);

/*	if(Ret && pCD->m_iPlayer != -1)
	{
		if (!Phys.m_nPrim)
			M_TRACEALWAYS("(PLAYER_PHYS) No prims: %s\n" );
		else if (Phys.m_Prim[0].m_PrimType == OBJECT_PRIMTYPE_BOX)
			M_TRACEALWAYS("(PLAYER_PHYS) Creating Box: %d, %d, %d\n", Phys.m_Prim[0].m_Dimensions[0], Phys.m_Prim[0].m_Dimensions[1], Phys.m_Prim[0].m_Dimensions[2]);
		else
			M_TRACEALWAYS("(PLAYER_PHYS) Creating other: %d, %d, %d\n", Phys.m_Prim[0].m_Dimensions[0], Phys.m_Prim[0].m_Dimensions[1], Phys.m_Prim[0].m_Dimensions[2] );
	}*/


	if (_pWServer && !Ret && !(OldIntersectFlags & OBJECT_FLAGS_PLAYERPHYSMODEL) && (IntersectionFlags & OBJECT_FLAGS_PLAYERPHYSMODEL))
	{
		// World collision was turned on but char is not in a valid position - try to find one!
		if (FindValidPosition(_pWServer, Phys, _pObj->GetLocalPositionMatrix(), NewPos))
			Ret = _pPhysState->Object_SetPhysics(_pObj->m_iObject, Phys, NewPos); // try again (should work)
	}
	if (_pWServer && !Ret && _bTele)
	{
		// Try to telefrag everything colliding to the physics-state
		TSelection<CSelection::LARGE_BUFFER> Selection;
		_pWServer->Selection_AddIntersection(Selection, _pObj->GetPosition(), Phys);
		_pWServer->Message_SendToSelection(CWObject_Message(OBJMSG_PHYSICS_KILL, 0, 0, _pObj->m_iObject), Selection);

		// Try to set the phys-state again and see if the telefrag improved the situation.
		Ret = _pPhysState->Object_SetPhysics(_pObj->m_iObject, Phys, NewPos);
	}
	if(!Ret && (PhysType == PLAYER_PHYS_DEAD) && _pWServer)
	{
		//_pWServer->Object_Destroy(_pObj->m_iObject);
		CWObject* pObj = _pWServer->Object_Get(_pObj->m_iObject);
		if (pObj)
			pObj->ClientFlags() |= CWO_CLIENTFLAGS_INVISIBLE;
		return 0;
	}
	if (!Ret && !_bTele && PhysType != 0)
	{
		// Try using current bbox width, and use new height, need to set a flag somewhere
		// about resetting the physics when there's space enough for it..
		CVec3Dfp32 Dim = _pObj->GetPhysState().m_Prim[0].GetDim();
		Dim.k[2] = Phys.m_Prim[0].GetDim().k[2];
		Phys.m_Prim[0].SetDim(Dim);
		Phys.m_nPrim = 1;
		Ret = _pPhysState->Object_SetPhysics(_pObj->m_iObject, Phys, NewPos);
		// Force set physics until we can get it done
		pCD->m_Phys_Flags = pCD->m_Phys_Flags | PLAYER_PHYSFLAGS_FORCESETPHYS;
	}
	else if (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_FORCESETPHYS)
	{
		pCD->m_Phys_Flags = pCD->m_Phys_Flags & ~PLAYER_PHYSFLAGS_FORCESETPHYS;
	}
		
	if (Ret)
	{
		Char_SetPhysType(_pObj, _Type);

		int ClientFlags = _pObj->m_ClientFlags & ~(PLAYER_CLIENTFLAGS_SPECTATOR);

		switch(_Type)
		{
		case PLAYER_PHYS_CROUCH :
			{
				// Crouch
			}
			break;
		case PLAYER_PHYS_STAND :
			{
				// Stand
			}
			break;
		case PLAYER_PHYS_DEAD :
			{
				// Dead
				ClientFlags |= PLAYER_CLIENTFLAGS_NOGRAVITY;
			}
			break;
		case PLAYER_PHYS_NOCLIP :
		case PLAYER_PHYS_NOCLIP2 :
			{
				// NoClip
			}
			break;
		case PLAYER_PHYS_SPECTATOR :
			{
				// Spectator
				ClientFlags |= PLAYER_CLIENTFLAGS_SPECTATOR;
			}
			break;

		default:
			{
				Error_static("CWObject_Character::Char_SetPhysics", "Invalid type. (2)");
			}
		}

		_pObj->m_ClientFlags = ClientFlags;
	}

	pCD->m_Phys_IdleTicks = 0;

	// Update impulsestate for animgraph, (update for crouch..)
	CWAG2I_Context AG2Context(_pObj, _pPhysState, CMTime::CreateFromTicks(pCD->m_GameTick,_pPhysState->GetGameTickTime(),pCD->m_PredictFrameFrac));
	pCD->m_AnimGraph2.GetAG2I()->UpdateImpulseState(&AG2Context);

	return Ret;
}

bool CWObject_Character::Phys_ToggleNoClip(int _PlayerPhys)
{
	MAUTOSTRIP(CWObject_Character_Phys_ToggleNoClip, false);
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);

	if (!(m_ClientFlags & PLAYER_CLIENTFLAGS_SPECTATOR))
	{
		// NOCLIP
		bool bSuccess = false;
//		if (Char_CheatsEnabled())
		{
			if (Char_GetPhysType(this) == PLAYER_PHYS_NOCLIP)
			{
				if (!Char_SetPhysics(this, m_pWServer, m_pWServer, PLAYER_PHYS_CROUCH))
					m_pWServer->Game_GetObject()->Player_ConOut(pCD->m_iPlayer, (char*)"Unable to switch from noclip.");
				else
					bSuccess = true;
			}
			else
			{
				if (!Char_SetPhysics(this, m_pWServer, m_pWServer, _PlayerPhys))
					m_pWServer->Game_GetObject()->Player_ConOut(pCD->m_iPlayer, (char*)"Unable to switch to noclip.");
				else
					bSuccess = true;
			}
		}
/*		else
		{
			CWO_Player* pP = m_pWServer->Game_GetObject()->Player_Get(pCD->m_iPlayer);
			if (pP) m_pWServer->Net_ConOut(CStrF("%s %s", (char*)pP->m_Name, "tried to cheat."));
		}*/

		m_pWServer->Game_GetObject()->Player_ConOut(pCD->m_iPlayer, CStrF("Noclip %s", (Char_GetPhysType(this) == PLAYER_PHYS_NOCLIP) ? "On" : "Off"));

#ifndef M_RTM
		// Set/reset noclip speed scaling
		if (Char_GetPhysType(this) == PLAYER_PHYS_NOCLIP)
			pCD->m_ControlMove_Scale = pCD->m_ControlMove_Scale * m_NoClipSpeedFactor;
		else
			pCD->m_ControlMove_Scale = pCD->m_ControlMove_Scale / m_NoClipSpeedFactor;
#endif
		return bSuccess;
	}
	else
		return false;
}

int CWObject_Character::Phys_GetMediumSamplingPoints(CVec3Dfp32* _pRetV, int _MaxV)
{
	MAUTOSTRIP(CWObject_Character_Phys_GetMediumSamplingPoints, 0);
	_pRetV[0] = GetPosition();
	_pRetV[0][2] += 8.0f;
	return 1;
}

void CWObject_Character::Phys_SetForceMove(CWObject_CoreData* _pObj, fp32 _Speed, fp32 _TotalTime)
{
	MAUTOSTRIP(CWObject_Character_Phys_SetForceMove, MAUTOSTRIP_VOID);
	CWO_Character_ClientData *pCD= CWObject_Character::GetClientData(_pObj);
	if (!pCD) return;

	CVec3Dfp32 ControlMove = GetAdjustedMovement(pCD);
	CVec3Dfp32 Move;
	fp32 MoveLenSqr = ControlMove.LengthSqr();
	if (MoveLenSqr > Sqr(0.01f))
	{
		Move = ControlMove * (_Speed * M_InvSqrt(MoveLenSqr));
	}
	else
	{
//		CVec3Dfp32 Dir(1,0,0);
//		Move = Dir * _Speed;
		Move[0]	= _Speed;
		Move[1]	= 0;
		Move[2] = 0;
	}

	Char_SetControlMode(_pObj, PLAYER_CONTROLMODE_FORCEMOVE);
	pCD->m_ControlMode_Param0 = Move[0];
	pCD->m_ControlMode_Param1 = Move[1];
	pCD->m_ControlMode_Param2 = Move[2];
	pCD->m_ControlMode_Param3 = _TotalTime;
}



CVec3Dfp32 CWObject_Character::Phys_GetUserAccelleration(const CSelection& _Selection, CWObject_CoreData* _pObj, CWObject_Character* _pChar, CWorld_PhysState* _pPhysState, 
	const CVec3Dfp32& _Move, const CVec3Dfp32& _Look, uint32 _Press, uint32& _Released, 
	const CXR_MediumDesc& _MediumDesc, int16& _Flags)
{
	// Dead?
//	if (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOMOVE)
//		return 0;
	MAUTOSTRIP(CWObject_Character_Phys_GetUserAccelleration, CVec3Dfp32(0,0,0));
	MSCOPESHORT(CWObject_Character::Phys_GetUserAccelleration);

	_pObj->m_PhysAttrib.m_StepSize = PLAYER_STEPSIZE;

	CVec3Dfp32 VRet(0);

	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if(!pCD)
		return 0;
	if(pCD->m_LiquidTickCountDown>0)
		pCD->m_LiquidTickCountDown = pCD->m_LiquidTickCountDown - 1;
	
	const int PhysType = CWObject_Character::Char_GetPhysType(_pObj);

	CVec3Dfp32 Look;
	CVec3Dfp32 Move;
	if (((_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOMOVE) ||
		(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE)) && PhysType != PLAYER_PHYS_NOCLIP)
	{
		Look = _Look;
		Move = 0;
	}
	else
	{
		Look = _Look;
		Move = _Move;
	}

	// Don't allow player to move while selecting darkness power
	if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_SELECTION)
		Move = CVec3Dfp32(0.0f);

/* --("2K x06 revert": use old controls, i.e. don't lock player)
	// Don't allow player to move if pressing [RB] while using the Demon Arm
	if ((pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DEMONARM) && (pCD->m_Control_Press & CONTROLBITS_BUTTON2))
		Move = CVec3Dfp32(0.0f);
*/	

	/*if (pCD->m_iPlayer != -1 &&
		!(_pObj->m_ClientFlags & (PLAYER_CLIENTFLAGS_GHOST)) &&
		!(PhysType == PLAYER_PHYS_NOCLIP) &&
		(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOGRAVITY) == 0)
	{
		const fp32 MLen = Length2(Move[0], Move[1]);
		fp32 MLenNew = MLen;

		if (MLen < PLAYER_CONTROL_WALKMIN)
		{
			pCD->m_Control_Move_StartTick = 0;
			MLenNew = 0;
		}
		else if(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_STUNNED || 
				pCD->m_Item0_Flags & RPG_ITEM_FLAGS_FORCEWALK ||
				pCD->m_Item1_Flags & RPG_ITEM_FLAGS_FORCEWALK)
			MLenNew = PLAYER_CONTROL_WALK;
		else if (MLen < PLAYER_CONTROL_WALKMAX)
		{
			if(!pCD->m_Control_Move_StartTick)
				pCD->m_Control_Move_StartTick = _pPhysState->GetGameTick();

			if(pCD->m_Control_Move_StartTick + PLAYER_CONTROL_WALKLINEARTICKS > _pPhysState->GetGameTick())
				MLenNew = MLen;
			else
				MLenNew = PLAYER_CONTROL_WALK;
		}
		else if(PhysType == PLAYER_PHYS_CROUCH)
			MLenNew = PLAYER_CONTROL_WALK;
		else
			MLenNew = 1.0f;

		const fp32 MLenScale = (MLen > 0.001f) ? MLenNew / MLen : 0;
		Move[0] *= MLenScale;
		Move[1] *= MLenScale;
	}*/
	// Slow down when crouching
	Move *= PhysType == PLAYER_PHYS_CROUCH ? 0.5f : 1.0f;

	// Release dragged bodies when pressing jump if no clip
	if ((pCD->m_iPlayer != -1) && (PhysType == PLAYER_PHYS_NOCLIP) && (_Press != CONTROLBITS_CROUCH))
	{
		_pPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETGRABBEDBODY,0), _pObj->m_iObject);
	}

	

	CMat4Dfp32 MatLook;
	Look.CreateMatrixFromAngles(0, MatLook);
	_pObj->GetPosition().SetMatrixRow(MatLook, 3);

	CMat4Dfp32 MatWalk;
//	CVec3Dfp32 WalkDir(0, 0, Look.k[2]);
//	WalkDir.CreateMatrixFromAngles(0, MatWalk);
	MatWalk.Unit();
	MatWalk.M_x_RotZ(Look.k[2]);

	int iControlMode = Char_GetControlMode(_pObj);
	if(pCD->m_AnimGraph2.GetAG2I()->HasOverlayAnim())
		iControlMode = PLAYER_CONTROLMODE_ANIMATION;
//	if(iControlMode == PLAYER_CONTROLMODE_ANIMATION)
//		iControlMode = PLAYER_CONTROLMODE_FREE;

	if ((PhysType == PLAYER_PHYS_NOCLIP) ||
		(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_SPECTATOR) ||
		((_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_GHOST || pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOGRAVITY) && 
		(iControlMode != PLAYER_CONTROLMODE_ACTIONCUTSCENE) &&
		(iControlMode != PLAYER_CONTROLMODE_ANIMATION) && 
		/*(iControlMode != PLAYER_CONTROLMODE_ANIMCONTROLLED) && */
		(iControlMode != PLAYER_CONTROLMODE_LEDGE) &&
		(iControlMode != PLAYER_CONTROLMODE_LEDGE2) &&
		(iControlMode != PLAYER_CONTROLMODE_LADDER)))
	{
		// -------------------------------------------------------------------
		//  Spectator fly-mode
		// -------------------------------------------------------------------
		CVec3Dfp32 dV(0);
		dV += CVec3Dfp32::GetRow(MatLook, 0) * (Move[0] * (fp32)pCD->m_Speed_Forward);
		dV += CVec3Dfp32::GetRow(MatLook, 1) * (Move[1] * (fp32)pCD->m_Speed_SideStep);
		dV += CVec3Dfp32::GetRow(MatLook, 2) * (Move[2] * (fp32)pCD->m_Speed_Up);
//			dV *= Char()->Speed();
//			if ((m_Press & CONTROLBITS_TURBO) && Char_CheatsEnabled()) dV *= 2.0f;

		if (_Press & CONTROLBITS_JUMP) dV.k[2] = Min(dV.k[2] + 8.0f, 8.0f);
		dV *= 2.5f;
		dV.Lerp(_pObj->GetMoveVelocity(), 0.9f, dV);

		VRet = dV - _pObj->GetMoveVelocity();
	}
	else
	{
		switch(iControlMode)
		{
		case PLAYER_CONTROLMODE_ANIMATION:
			return Char_ControlMode_Anim2(_Selection, _pObj, _pPhysState, _Flags);
		
		case PLAYER_CONTROLMODE_FORCEMOVE :
			{
				Move[0] = (fp32)pCD->m_ControlMode_Param0;
				Move[1] = (fp32)pCD->m_ControlMode_Param1;
				Move[2] = (fp32)pCD->m_ControlMode_Param2;
//			ConOutL(CStrF("Move = %s, %s, %s", (char*)Move.GetString(), (char*)_pObj->GetPosition().GetString(), (char*)_pObj->GetLocalPosition().GetString() ));
				goto ControlMode_Free;
			}
			break;

		case PLAYER_CONTROLMODE_FREE : 
			{
ControlMode_Free:
				//Should we switch off gravity?
				if (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOGRAVITY || pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOGRAVITY)
					_Flags |= 1;

				if (!(_MediumDesc.m_MediumFlags & XW_MEDIUM_LIQUID))
				{
					// -------------------------------------------------------------------
					//  Not in liquid
					// -------------------------------------------------------------------

					// Slowdown factor does not affect jumping speed

					CVec3Dfp32 dV(0);
					if(!(PhysType == PLAYER_PHYS_DEAD))
					{
						dV += CVec3Dfp32::GetRow(MatWalk, 0) * (Move[0] * LERP((fp32)pCD->m_Speed_WalkForward,(fp32)pCD->m_Speed_Forward,Abs(Move[0])));
						dV += CVec3Dfp32::GetRow(MatWalk, 1) * (Move[1] * (fp32)pCD->m_Speed_SideStep);
					}
		//				dV *= Char()->Speed();
		//				if ((_Press & CONTROLBITS_TURBO) && Char_CheatsEnabled()) dV *= 2.0f;

					// Check if we're standing on something.
					CCollisionInfo CInfo_1;
					CMat4Dfp32 p = _pObj->GetPositionMatrix();
					CVec3Dfp32::GetMatrixRow(p, 3) += CVec3Dfp32(0,0,-0.02f);


					bool bOnGround;
					{
						// MUPPJOKKO - IMPLEMENT ME! en hel kollisions query enbart för att se om man står på marken eller inte!!
#ifdef	USE_PCS
						CPotColSet pcs;
						{
							MSCOPESHORT(BuildPCS);
							float fBoxMinMax[6];																// bounds of movement
							CMat4Dfp32 DestPos;																	// destination of movement

//							GetMovementBounds( fBoxMinMax, _iObj, pObj->GetLocalPositionMatrix(), p );	// extract [x,y,z,t] bounds
							
							CBox3Dfp32 Box1, Box2;
							_pPhysState->Phys_GetMinMaxBox( _pObj->GetPhysState(), _pObj->GetPositionMatrix(), Box1 );
//							_pPhysState->Phys_GetMinMaxBox( _pObj->GetPhysState(), _pObj->GetPositionMatrix(), Box2 );
							
//							Box1.Expand( Box2 );

							fBoxMinMax[0]	= Box1.m_Min[0];
							fBoxMinMax[1]	= Box1.m_Min[1];
							fBoxMinMax[2]	= Box1.m_Min[2] - 0.02f;
							fBoxMinMax[3]	= Box1.m_Max[0];
							fBoxMinMax[4]	= Box1.m_Max[1];
							fBoxMinMax[5]	= Box1.m_Min[2] + 1.0f;

							pcs.SetBox( fBoxMinMax );
							_pPhysState->Selection_GetArray( &pcs, _iSel, _pObj->GetPhysState(), _pObj->m_iObject, _pObj->GetLocalPositionMatrix(), p );
						}
						// NOTE: Assuming char origin is at the bottom of it's box  -mh
						CWO_PhysicsState PhysState(_pObj->GetPhysState());
						PhysState.m_Prim[0].m_DimZ = 1;
						PhysState.m_Prim[0].m_Offset[2] = 1;
						bOnGround = _pPhysState->Phys_IntersectWorld(&pcs, PhysState, _pObj->GetPositionMatrix(), p, _pObj->m_iObject, &CInfo_1);
#else
						// NOTE: Assuming char origin is at the bottom of it's box  -mh
						CWO_PhysicsState PhysState(_pObj->GetPhysState());
						PhysState.m_Prim[0].m_DimZ = 1;
						PhysState.m_Prim[0].m_Offset[2] = 1;
						bOnGround = _pPhysState->Phys_IntersectWorld(&_Selection, PhysState, _pObj->GetPositionMatrix(), p, _pObj->m_iObject, &CInfo_1);
#endif
					}

					if (!CInfo_1.m_bIsValid)
						bOnGround = false;

					if (!bOnGround)
						_Flags |= 2;

					// Invalidate the material
					///pCD->m_GroundMaterial = 0;

					if ((bOnGround || pCD->m_Phys_nInAirFrames < 3) && !(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOGRAVITY) && !(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOGRAVITY))
					{
						// -------------------------------------------------------------------
						//  Walking
						// -------------------------------------------------------------------

						/*if(CInfo_1.m_pSurface)
							pCD->m_GroundMaterial = (uint8)CInfo_1.m_pSurface->GetBaseFrame()->m_MaterialType;*/

						// We hit something, lets look at the collision plane...
						CVec3Dfp32 Normal;
						Normal = (CInfo_1.m_bIsValid) ? CInfo_1.m_Plane.n : CVec3Dfp32(0,0,1);
						Normal.Normalize();
						fp32 Friction = 1.0f;

						if( CInfo_1.m_bIsValid )
						{
							pCD->m_Phys_iLastGroundObject = CInfo_1.m_iObject;

							if (Normal.k[2] > PLAYER_PHYS_COS_MAX_SLOPE)
							{
								// Slope is ok. Project velocity so that it is parallell to the plane. (this way we get ramp-jumping since the velocity increases with the slope)
								CVec3Dfp32 ProjDir(0,0,1);
								dV.Combine(ProjDir, -(dV*Normal) / (ProjDir*Normal), dV);
							}
							else if (Normal.k[2] < PLAYER_PHYS_COS_MAX_SLOPE)
							{
								// Slope is greater than the accepted 'full-speed' slope, so we scale the influence by the feet.
								if (Normal.k[2] > 0.0f)
									Friction = (2.0f*Clamp01(0.5f - Normal.k[2]));
								else
									Friction = 0;
							}
						}

		//				if (CInfo_1.m_bIsValid)
		//					_pObj->m_PhysAttrib.m_StepSize = PLAYER_STEPSIZE * Clamp01(5.0f * (Normal[2] - 0.8f));

						const fp32 MoveLenFric = Clamp01(0.35f + Length2(Move[0], Move[1]) * 0.65f);
						Friction *= MoveLenFric;

						// Feet on floor
						CVec3Dfp32 dVMoved2;
						if (CInfo_1.m_bIsValid)
						{
							const CWObject_CoreData* pCoreData = _pPhysState->Object_GetCD(CInfo_1.m_iObject);
							if (pCoreData && pCoreData->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER)
							{
								_pObj->GetPosition().Sub(pCoreData->GetPosition(), dVMoved2);
								dVMoved2[2] = 0;
//								dVMoved2.Normalize();
//								dVMoved2 *= 8.0f;
								dVMoved2.SetLength( 8.0f );

							}
							else
							{
								dVMoved2 = CInfo_1.m_Velocity;
							}
						}
						else
						{
							const CWObject_CoreData* pCoreData = _pPhysState->Object_GetCD(pCD->m_Phys_iLastGroundObject);
							if (pCoreData)
							{
								dVMoved2 = pCoreData->GetMoveVelocity();
							}
							else
								dVMoved2 = 0;
						}

//				ConOut("dVMoved2 == " + dVMoved2.GetString());
						CVec3Dfp32 v = _pObj->GetMoveVelocity();
						const fp32 vlenSqr = dV.LengthSqr();
						dV += dVMoved2;
						CVec3Dfp32 dV2(dV - v);

						// This is the acceleration formula we've allways had.
						// VRet += dV2.Normalize() * Min(dV2.Length() / 2.0f, Max(2.6f, vlen / 2.0f));
						const fp32 dV2LenSqr = dV2.LengthSqr();
						if (dV2LenSqr > Sqr(0.001f))
							VRet += dV2.Normalize() * Min(M_Sqrt(dV2LenSqr) * (1.0f / 2.0f), Max(2.6f, M_Sqrt(vlenSqr) * (1.0f / 2.0f)));
						VRet *= Friction;

						if (!bOnGround)
							VRet[2] = 0;
/*						else if ((dV2LenSqr > Sqr(0.001f)) && (Normal.k[2] > PLAYER_PHYS_COS_MAX_SLOPE))
						{
							fp32 Scale = VRet.Length() / Length2(VRet[0], VRet[1]);
							VRet[0] *= Scale;
							VRet[1] *= Scale;
							VRet[2] = 0;
						}*/

#ifdef	ENABLE_PHYSLOG
				//		if (_pChar && _pChar->m_bPhysLog) 
/*							ConOutL(CStrF("V %s, VRet %s, Valid %d, dV2 %s, CInfo.m_Vel %s, CInfo.Normal %s", 
								(char*) _pObj->GetMoveVelocity().GetString(), 
								(char*) VRet.GetString(), 
								CInfo_1.m_bIsValid,
								(char*) dV2.GetString(), 
								(char*) CInfo_1.m_Velocity.GetString(), 
								(char*) CInfo_1.m_Plane.n.GetString()));*/
#endif
						if (CInfo_1.m_bIsValid && (Normal.k[2] > PLAYER_PHYS_COS_MAX_SLOPE))
						{
							if (bOnGround && dVMoved2.LengthSqr() == 0)
								_Flags |= 1;	// Turn off gravity in PhysUtil_Move
							
							if (((_Press & CONTROLBITS_JUMP) & _Released) &&
								(PhysType != PLAYER_PHYS_DEAD) &&										// no jump when dead
							    !(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOMOVE) &&					// no jump in 'nomove' mode
							    !(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE) &&						// no jump in 'immobile' mode
							    (pCD->m_AnimGraph2.GetForcedAimingType() != AIMING_TYPE_MINIGUN) &&		// no jump with minigun
							    !(pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_DIALOGUE) &&			// no jump in dialogue
								!(_Press & CONTROLBITS_BUTTON5))										// no jump while pressing drain button
							{
								// -------------------------------------------------------------------
								//  Jump
								// -------------------------------------------------------------------
								// NOTE: Parts of this code is duplicated in OnPhysicsEvent
								_Released &= ~CONTROLBITS_JUMP;
								VRet.k[2] = 0;

								// Make some noise
								_pPhysState->Phys_Message_SendToObject(
									CWObject_Message(OBJMSG_CHAR_RAISENOISELEVEL,99),_pObj->m_iObject);
								
								// Hmm, ok before we can jump, test if there's a ledge around that
								// we can use....
								/*CWAGI_Context Context(_pObj,_pPhysState,_pPhysState->GetGameTime(),0,false);
								fp32 PickupType = AG_PICKUP_TYPELEDGE;
								CXRAG_ICallbackParams Params(&PickupType,1);
								pCD->Effect_PickupStuff(&Context, &Params);
								if (Char_GetControlMode(_pObj) != PLAYER_CONTROLMODE_LEDGE)*/
								{
									const CVec3Dfp32& CurV = _pObj->GetMoveVelocity();
									fp32 vz = VRet.k[2] - Min(0.0f, CurV[2]);
									float fSpeed_Jump = pCD->m_Speed_Jump/* * Char()->JumpSpeed()*/;
									fp32 JumpV = Max(vz + fSpeed_Jump, fSpeed_Jump);
									_pPhysState->Object_AddVelocity(_pObj->m_iObject, CVec3Dfp32(0,0,JumpV));

									_Flags |= 2;	// ??
//									VRet[0] *= 0.2f;
//									VRet[1] *= 0.2f;
//									VRet[0] += CurV[0] * 0.2f;
//									VRet[1] += CurV[1] * 0.2f;
									VRet[0] = VRet[0] * 0.2f + CurV[0] * 0.2f;
									VRet[1] = VRet[1] * 0.2f + CurV[1] * 0.2f;
	#ifdef	ENABLE_PHYSLOG
									if (_pChar && _pChar->m_bPhysLog)
										ConOutL(CStrF("PostJump speed %s, %f", (char*)VRet.GetString(), CurV[2]));
	#endif

									// Ruin accuracy.
									//pCD->Target_SetAccuracy(0);

	/*								if(pCD->m_bIsServerRefresh)
									{
										// Hack
										CWObject_Character *pObj = (CWObject_Character *)_pObj;
										pObj->PlayDialogue_Hash(PLAYER_DIALOGUE_JUMP);
									}*/

									// Movetoken to jump in animgraph
									_pPhysState->Phys_Message_SendToObject(
										CWObject_Message(OBJMSG_CHAR_SETGRABBEDBODY,0), _pObj->m_iObject);
									CWAG2I_Context AG2Context(_pObj, _pPhysState,pCD->m_GameTime);
									pCD->m_AnimGraph2.SendJumpImpulse(&AG2Context,CXRAG2_Impulse(AG2_IMPULSETYPE_JUMP,AG2_IMPULSEVALUE_JUMP_UP));
								}
							}
							else
							{
								if (VRet.k[2] < 0)
									VRet.k[2] = 0;
		//						if (!_pChar) ConOut(CStrF("No Jump: Press %.8x, Release %.8x, ", _Press, _Released) + Normal.GetString());
							}
						}
					}
					else
					{
						// -------------------------------------------------------------------
						//  Airborne
						// -------------------------------------------------------------------

						// Well, air control isn't quite as simple as one might believe:
						if (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOGRAVITY || pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOGRAVITY)
						{
							//Truly flying
							CVec3Dfp32 dV(0);
							dV += CVec3Dfp32::GetRow(MatLook, 0) * (Move[0] * (fp32)pCD->m_Speed_Forward);
							dV += CVec3Dfp32::GetRow(MatLook, 1) * (Move[1] * (fp32)pCD->m_Speed_SideStep);
							dV += CVec3Dfp32::GetRow(MatLook, 2) * (Move[2] * (fp32)pCD->m_Speed_Up);

							dV *= 2.5f;
							dV.Lerp(_pObj->GetMoveVelocity(), 0.9f, dV);

							VRet = dV - _pObj->GetMoveVelocity();
						}
						else
						{
//							VRet = 0;
//							break;
							
							//Just falling
							CVec3Dfp32 CurrentV = _pPhysState->Object_GetVelocity(_pObj->m_iObject);
							CurrentV[2] = 0;
							dV[2] = 0;
							const fp32 dVLen = dV.Length();
							CVec3Dfp32 VAdd;
							VAdd = 0;
//							const fp32 CurrentVLen = CurrentV.Length();
//							const fp32 dVProjCurrent = (dV*CurrentV) / CurrentVLen;
							fp32 CurrentProjdV = (dVLen > 0.0f) ? (dV*CurrentV) / dVLen : 0;
							if (CurrentProjdV < 0.0f) CurrentProjdV = 0;
							if (CurrentProjdV < dVLen)
							{
								VAdd = dV*((dVLen - CurrentProjdV) / dVLen); // - CurrentV*(CurrentVLen
								const fp32 VAddLen = VAdd.Length();
								VAdd = VAdd.Normalize() * Min(VAddLen * (1.0f/ 6.0f), Max(0.0f, dVLen * (1.0f / 15.0f)));
							}

							VRet = VAdd;
						}
					}
				}
				else
				{
					// -------------------------------------------------------------------
					//  In liquid (swimming)
					// -------------------------------------------------------------------
					CVec3Dfp32 dV(0);
					dV += CVec3Dfp32::GetRow(MatLook, 0) * (Move[0] * (fp32)pCD->m_Speed_Forward);
					dV += CVec3Dfp32::GetRow(MatLook, 1) * (Move[1] * (fp32)pCD->m_Speed_SideStep);
					dV += CVec3Dfp32::GetRow(MatLook, 2) * (Move[2] * (fp32)pCD->m_Speed_Up);
					//	dV *= Char()->Speed();
					// if ((_Press & CONTROLBITS_TURBO) && Char_CheatsEnabled()) dV *= 2.0f;

					// Up?
					if (_Press & CONTROLBITS_JUMP)
						dV.k[2] = Min(dV.k[2] + 8.0f, 8.0f);


					CCollisionInfo CInfo_2;
					CMat4Dfp32 p = _pObj->GetPositionMatrix();
					CVec3Dfp32::GetMatrixRow(p, 3) += CVec3Dfp32(0,0,-8);
					bool bOnGround;
					{
						// MUPPJOKKO - IMPLEMENT ME! en hel kollisions query enbart för att se om man står på marken eller inte!!
						// NOTE: Assuming char origin is at the bottom of it's box  -mh
						CWO_PhysicsState PhysState(_pObj->GetPhysState());
						PhysState.m_Prim[0].m_DimZ = 1;
						PhysState.m_Prim[0].m_Offset[2] = 1;
						bOnGround = _pPhysState->Phys_IntersectWorld(&_Selection, PhysState, _pObj->GetPositionMatrix(), p, _pObj->m_iObject, &CInfo_2);
					}
					if (!CInfo_2.m_bIsValid)
						bOnGround = false;
	
					CVec3Dfp32 v = _pPhysState->Object_GetVelocity(_pObj->m_iObject);
					
/*					int MedType = pCD->m_HeadMediumType; 
					// Swim only if not feet is on solid ground and the head is above surface.
					if(!(!(MedType & XW_MEDIUM_LIQUID) && bOnGround))
					{
						// Force player up
						bool IsDiving = (_Look[1] < -0.05f) && (v.LengthSqr() > Sqr(2.0f));
						if(!IsDiving)
							dV.k[2] = Min(dV.k[2] + 2.0f, 8.0f);
						Char_SetAnimCode(_pObj, PLAYER_ANIM_CODE_SWIMMING);
						pCD->m_LiquidTickCountDown = 5;
					}*/
					dV *= 0.9f;

					CVec3Dfp32 dVMoved2 = _MediumDesc.m_Velocity;
					fp32 vlen = dV.Length() * (1.f / 9.f);
					dV += dVMoved2;
					CVec3Dfp32 dV2(dV - v);
					fp32 Length = dV2.Length();
					dV2 *= 1.f / Length;
					Length *= 0.25f;
					VRet += dV2 * Min(Length, Max(0.0f, vlen));
				}
				int32 TokenFlags = pCD->m_AnimGraph2.GetStateFlagsLo();
				if (TokenFlags & AG2_STATEFLAG_APPLYTURNCORRECTION)
				{
					CWAG2I_Context Context(_pObj, _pPhysState, pCD->m_GameTime);
					CQuatfp32 RotVelocity;
					if (pCD->m_AnimGraph2.GetAG2I()->GetAnimRotVelocity(&Context, RotVelocity,0))
					{
						CAxisRotfp32 RotVelocityAR(RotVelocity);
						/*RotVelocityAR.CreateQuaternion(RotVelocity);
						RotVelocityAR = RotVelocity;*/
						//fp32 RotVel = RotVelocityAR.m_Angle * RotVelocityAR.m_Axis.k[2];
						pCD->m_AnimRotVel = RotVelocityAR.m_Angle * RotVelocityAR.m_Axis.k[2];
					}
				}
			}
			break;

		case PLAYER_CONTROLMODE_LADDER :
			{
				CWObject_Phys_Ladder::GetUserAccelleration(_Selection, _pObj, _pPhysState, VRet, Move, 
					MatLook, _Press, _Released, pCD, _Flags);

				break;
			}

		case PLAYER_CONTROLMODE_HANGRAIL:
			{
				CWObject_Phys_Ladder::GetUserAccelleration(_Selection, _pObj, _pPhysState, VRet, Move, 
					MatLook, _Press, _Released, pCD, _Flags);

				return 0.0f;
			}
		case PLAYER_CONTROLMODE_LEDGE:
			{
				// Should not get in here, controlled from phys_move
				/*CWObject_Ledge::GetUserAccelleration(_iSel, _pObj, _pPhysState, VRet, Move, 
					MatLook, _Press, _Released, pCD, _Flags);*/

				break;
			}
		case PLAYER_CONTROLMODE_CARRYBODY:
			{
				//Should we switch off gravity?
				if (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOGRAVITY || pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOGRAVITY)
					_Flags |= 1;

				if (!(_MediumDesc.m_MediumFlags & XW_MEDIUM_LIQUID))
				{
					// -------------------------------------------------------------------
					//  Not in liquid
					// -------------------------------------------------------------------

					// Slowdown factor does not affect jumping speed
					const fp32 Damping = 0.5f;
					CVec3Dfp32 dV(0);
					if(!(PhysType == PLAYER_PHYS_DEAD))
					{
						/*// Only walk backwards
						if (Move[0] < 0)*/
							dV += CVec3Dfp32::GetRow(MatWalk, 0) * ((Move[0] * Damping) * (fp32)pCD->m_Speed_Forward);
						dV += CVec3Dfp32::GetRow(MatWalk, 1) * ((Move[1] * Damping) * (fp32)pCD->m_Speed_SideStep);
					}
		//				dV *= Char()->Speed();
		//				if ((_Press & CONTROLBITS_TURBO) && Char_CheatsEnabled()) dV *= 2.0f;

					// Check if we're standing on something.
					CCollisionInfo CInfo_3;
					CMat4Dfp32 p = _pObj->GetPositionMatrix();
					CVec3Dfp32::GetMatrixRow(p, 3) += CVec3Dfp32(0,0,-0.02f);
					
					bool bOnGround;
					{
						// MUPPJOKKO - IMPLEMENT ME! en hel kollisions query enbart för att se om man står på marken eller inte!!
						// NOTE: Assuming char origin is at the bottom of it's box  -mh
						CWO_PhysicsState PhysState(_pObj->GetPhysState());
						PhysState.m_Prim[0].m_DimZ = 1;
						PhysState.m_Prim[0].m_Offset[2] = 1;
						bOnGround = _pPhysState->Phys_IntersectWorld(&_Selection, PhysState, _pObj->GetPositionMatrix(), p, _pObj->m_iObject, &CInfo_3);
					}
					if (!CInfo_3.m_bIsValid)
						bOnGround = false;

					if (!bOnGround)
						_Flags |= 2;

					if ((bOnGround || pCD->m_Phys_nInAirFrames < 3) && !(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOGRAVITY) && !(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOGRAVITY))
					{
						// -------------------------------------------------------------------
						//  Walking
						// -------------------------------------------------------------------

						// We hit something, lets look at the collision plane...
						CVec3Dfp32 Normal;
						Normal = (CInfo_3.m_bIsValid) ? CInfo_3.m_Plane.n : CVec3Dfp32(0,0,1);
						Normal.Normalize();
						fp32 Friction = 1.0f;

						if (CInfo_3.m_bIsValid && Normal.k[2] > PLAYER_PHYS_COS_MAX_SLOPE)
						{
							// Slope is ok. Project velocity so that it is parallell to the plane. (this way we get ramp-jumping since the velocity increases with the slope)
							CVec3Dfp32 ProjDir(0,0,1);
							dV.Combine(ProjDir, -(dV*Normal) / (ProjDir*Normal), dV);
						}

						if (CInfo_3.m_bIsValid && (Normal.k[2] < PLAYER_PHYS_COS_MAX_SLOPE))
						{
							// Slope is greater than the accepted 'full-speed' slope, so we scale the influence by the feet.
							if (Normal.k[2] > 0.0f)
								Friction = (2.0f*Clamp01(0.5f - Normal.k[2]));
							else
								Friction = 0;
						}

		//				if (CInfo_3.m_bIsValid)
		//					_pObj->m_PhysAttrib.m_StepSize = PLAYER_STEPSIZE * Clamp01(5.0f * (Normal[2] - 0.8f));

						fp32 MoveLenFric = Clamp01(0.35f + Length2(Move[0], Move[1]) * 0.65f);
						Friction *= MoveLenFric;

						// Feet on floor
						CVec3Dfp32 dVMoved2;
						if (CInfo_3.m_bIsValid)
						{
							CWObject_CoreData* pCoreData = _pPhysState->Object_GetCD(CInfo_3.m_iObject);
							if (pCoreData && pCoreData->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER)
							{
								_pObj->GetPosition().Sub(pCoreData->GetPosition(), dVMoved2);
								dVMoved2[2] = 0;
//								dVMoved2.Normalize();
//								dVMoved2 *= 8.0f;
								dVMoved2.SetLength( 8.0f );

							}
							else
							{
								dVMoved2 = CInfo_3.m_Velocity;
							}
						}
						else
							dVMoved2 = 0;

//				ConOut("dVMoved2 == " + dVMoved2.GetString());
						CVec3Dfp32 v = _pObj->GetMoveVelocity();
						const fp32 vlenSqr = dV.LengthSqr();
						dV += dVMoved2;
						CVec3Dfp32 dV2(dV - v);

						// This is the acceleration formula we've allways had.
						// VRet += dV2.Normalize() * Min(dV2.Length() / 2.0f, Max(2.6f, vlen / 2.0f));
						const fp32 dV2LenSqr = dV2.LengthSqr();
						if (dV2LenSqr > Sqr(0.001f))
							VRet += dV2.Normalize() * Min(M_Sqrt(dV2LenSqr) * (1.0f / 2.0f), Max(2.6f, M_Sqrt(vlenSqr) * (1.0f / 2.0f)));
						VRet *= Friction;

						if (!bOnGround)
							VRet[2] = 0;

#ifndef M_RTM
						if (_pChar && _pChar->m_bPhysLog) 
							ConOutL(CStrF("V %s, VRet %s, Valid %d, CInfo.m_Vel %s, CInfo.Normal %s", 
								(char*) _pObj->GetMoveVelocity().GetString(), 
								(char*) VRet.GetString(), 
								CInfo_3.m_bIsValid,
								(char*) CInfo_3.m_Velocity.GetString(), 
								(char*) CInfo_3.m_Plane.n.GetString()));
#endif

						if (CInfo_3.m_bIsValid && (Normal.k[2] > PLAYER_PHYS_COS_MAX_SLOPE))
						{
							if (bOnGround && dVMoved2.LengthSqr() == 0)
								_Flags |= 1;	// Turn off gravity in PhysUtil_Move
							
							if (VRet.k[2] < 0) VRet.k[2] = 0;
						}
					}
				}
				int32 TokenFlags = pCD->m_AnimGraph2.GetStateFlagsLo();
				if (TokenFlags &  AG2_STATEFLAG_APPLYTURNCORRECTION)
				{
					CWAG2I_Context Context(_pObj, _pPhysState, pCD->m_GameTime);
					CQuatfp32 RotVelocity;
					if (pCD->m_AnimGraph2.GetAG2I()->GetAnimRotVelocity(&Context, RotVelocity,0))
					{
						CAxisRotfp32 RotVelocityAR(RotVelocity);
						//fp32 RotVel = RotVelocityAR.m_Angle * RotVelocityAR.m_Axis.k[2];
						pCD->m_AnimRotVel = RotVelocityAR.m_Angle * RotVelocityAR.m_Axis.k[2];
					}
				}
				break;
			}
		case PLAYER_CONTROLMODE_ACTIONCUTSCENE:
			{
				/*VRet = 0;
				_Flags |= PHYSICAL_FLAGS_MEDIUMACCEL;
				CVec3Dfp32 AnimVelocity = 0;
				//ConOut("ACS");
				// Use the fixedcharmove (test)
				pCD->m_FCharMovement.GetUserAccelleration(_iSel, _pObj, _pPhysState, AnimVelocity, 
					pCD, _Flags, FIXEDCHARMOVE_FLAG_APPLYVELOCITY);
				//_pPhysState->Object_SetVelocity(_pObj->m_iObject, AnimVelocity);
				
				break;*/
				int32 iACS = (int32)pCD->m_ControlMode_Param2;
				if (iACS <= ACTIONCUTSCENE_TYPE_HATCHOUTHORIZONTAL)
				{
					return Char_ControlMode_Anim2(_Selection, _pObj, _pPhysState, _Flags);
				}
				else
				{
					// Use the fixedcharmove (test)
					CVec3Dfp32 dV = 0.0f;
					// Acc formula as in controlmode free
					CVec3Dfp32 v = _pObj->GetMoveVelocity();
					const fp32 vlenSqr = dV.LengthSqr();
					CVec3Dfp32 dV2(dV - v);

					const fp32 dV2LenSqr = dV2.LengthSqr();
					if (dV2LenSqr > Sqr(0.001f))
						VRet = dV2.Normalize() * Min(M_Sqrt(dV2LenSqr) * (1.0f / 2.0f), Max(2.6f, M_Sqrt(vlenSqr) * (1.0f / 2.0f)));
					VRet.k[2] = 0.0f;
				}
				break;
			}
		case PLAYER_CONTROLMODE_FIGHTING:
			{
				// Use animcontrolled if we havn't moved over the other character....
//				bool bGetFCharMove = true;
				int32 iFightChar = (pCD->m_iFightingCharacter != -1 ? pCD->m_iFightingCharacter : 
					(pCD->m_FocusFrameType & SELECTION_MASK_TYPEINVALID) == SELECTION_CHAR ? pCD->m_iFocusFrameObject : -1);
				if (iFightChar == -1)
					iFightChar = (pCD->m_iCloseObject ? pCD->m_iCloseObject : -1);

				// Move towards opponent, if no opponent just stay still
				CVec3Dfp32 dV = 0.0f;
				if (iFightChar != -1)
				{
					CVec3Dfp32 OppPos = _pPhysState->Object_GetPosition(iFightChar);
					const fp32 BorderLineSqr = PLAYER_FIGHT_MINDISTANCE * PLAYER_FIGHT_MINDISTANCE;
					if (((OppPos - _pObj->GetPosition()).LengthSqr() > BorderLineSqr))
					{
						CVec3Dfp32 Dir = OppPos - _pObj->GetPosition();
						Dir.Normalize();

						dV = Dir * 4.0f;
					}
				}
				/*else
				{
					CVec3Dfp32 Dir = CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(),0);
					Dir.k[2] = 0.0f;
					Dir.Normalize();
					dV = Dir*4.0f;
				}*/

				// Use the fixedcharmove (test)
				/*CVec3Dfp32 dV = 0.0f;
				if (bGetFCharMove)
					pCD->m_FCharMovement.GetUserAccelleration(_iSel, _pObj, _pPhysState, dV, 
						pCD, _Flags, 0);*/

				// Acc formula as in controlmode free, should maybe be used on ladders and stuff as well?
				CVec3Dfp32 v = _pObj->GetMoveVelocity();
				const fp32 vlenSqr = dV.LengthSqr();
				CVec3Dfp32 dV2(dV - v);

				const fp32 dV2LenSqr = dV2.LengthSqr();
				if (dV2LenSqr > Sqr(0.001f))
					VRet = dV2.Normalize() * Min(M_Sqrt(dV2LenSqr) * (1.0f / 2.0f), Max(2.6f, M_Sqrt(vlenSqr) * (1.0f / 2.0f)));
				VRet.k[2] = 0.0f;
				//_pPhysState->Object_SetVelocity(_pObj->m_iObject, AnimVelocity);
				break;
			}
		case PLAYER_CONTROLMODE_LEDGE2:
			{
				// This is used while grabbing onto a ledge, since the ledge movement itself
				// is only fixed positions
				// If we we're in the air when grabbing ledge, we're still in the air...

				// Update char dir (won't be updated in OnPumpControl when in ledgemode
				// Set orientation according to the look.
				CWObject_Ledge::GetUserAccelleration(_Selection, _pObj, _pPhysState, VRet,_Move,MatLook, 
					pCD,_Flags,0.0f);
				return 0.0f;
				//break;
			}
		case PLAYER_CONTROLMODE_ANIMSYNC:
			{
				// Ok then, if we have setup a synced animation correctly, RelAnimPos will find 
				// velocity to the "perfect" position
				CWAG2I_Context AG2Context(_pObj, _pPhysState, pCD->m_GameTime);
				CVec3Dfp32 MoveVelocity;
				CQuatfp32 RotVelocity;
				pCD->m_RelAnimPos.GetVelocity(&AG2Context, pCD->m_AnimGraph2.GetAG2I(), MoveVelocity, RotVelocity);
				
				RotVelocity.Normalize();
				CAxisRotfp32 RotVelAR;
				RotVelAR.Create(RotVelocity);
				
				//if (_pPhysState->IsServer())
				{
					_pPhysState->Object_SetVelocity(_pObj->m_iObject, MoveVelocity);
					_pPhysState->Object_SetRotVelocity(_pObj->m_iObject, RotVelAR);
				}

				return 0.0f;
			}
		case PLAYER_CONTROLMODE_LEAN:
			{
				//_Flags |= PHYSICAL_FLAGS_MEDIUMACCEL | PHYSICAL_FLAGS_MEDIUMROTATE;
				// Use controlmode free without any new movement
				Move = 0.0;
				goto ControlMode_Free;
				/*VRet = 0.0f;
				_pPhysState->Object_SetVelocity(_pObj->m_iObject,VRet);*/
				break;
			}
		/*case PLAYER_CONTROLMODE_ANIMCONTROLLED:
			{
				CVec3Dfp32 AnimVelocity = 0;
				VRet = 0;

				_Flags |= 1;

				// Use the fixedcharmove (test)
				pCD->m_FCharMovement.GetUserAccelleration(_iSel, _pObj, _pPhysState, AnimVelocity, 
					pCD, _Flags, FIXEDCHARMOVE_FLAG_APPLYVELOCITY);
				//_pPhysState->Object_SetVelocity(_pObj->m_iObject, AnimVelocity);
				break;
			}*/

		/*case PLAYER_CONTROLMODE_LEAN:
			_Flags |= PHYSICAL_FLAGS_MEDIUMACCEL | PHYSICAL_FLAGS_MEDIUMROTATE;
			VRet = CVec3Dfp32(0,0,0);
			break;*/

		default :
			ConOut(CStrF("WARNING: Invalid control-mode %d", Char_GetControlMode(_pObj)));
		}
	}
	if(Char_GetControlMode(_pObj) != PLAYER_CONTROLMODE_LADDER)
	{
		if(pCD->m_Anim_ClimbTickCounter > 5)
		{
			//_pPhysState->Object_SetVelocity(_pObj->m_iObject, CVec3Dfp32(0, 0, 9));
			
/*			if(pCD->m_bIsServerRefresh)
			{
				// Hack
				CWObject_Character *pObj = (CWObject_Character *)_pObj;
				pObj->PlayDialogue_Hash(PLAYER_DIALOGUE_JUMP);
			}*/
			
//			Char_SetAnimSequence(_pObj, _pPhysState, PLAYER_ANIM_SEQUENCE_JUMP);
			//Char_SetControlMode(_pObj, PLAYER_CONTROLMODE_FREE);
		}
		pCD->m_Anim_ClimbTickCounter = 0;
	}
	if (_MediumDesc.m_MediumFlags & XW_MEDIUM_LIQUID)
	{
		pCD->m_TicksInLiquid = pCD->m_TicksInLiquid + 1;
		// The risk of recieving damage from being under water is growing linearly from 200 ticks.
		// It safe to be under water for 200 ticks. (10 seconds)
		// After that the chance of recieving damage every tick is increasing with 0.2% per tick.
		// Example: After 20 seconds (400 ticks) under water the risk of getting damage that tick is
		// 40%. After 700 ticks (35 seconds) the risk is 100% every frame.
/*
		int Tinl = pCD->m_TicksInLiquid;
		if(Tinl > 1200 && pCD->m_bIsServerRefresh &&
			MRTC_RAND() % 800 < ((fp32)Tinl - 1200.0f))	
		{
			CWO_DamageMsg Msg(1, DAMAGE_SUFFOCATE);
			Msg.Send(_pObj->m_iObject, 0, _pPhysState);
			//						_pPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_PHYSICS_DAMAGE, 1, DAMAGE_SUFFOCATE, -1, 0, 0, 0), _pObj->m_iObject);
		}		
*/
	}
	else
	{
		pCD->m_TicksInLiquid = 0;
	}

	// Well then, somehow we can get a little remnant from animationphysics (rotvel), so lets
	// zero it
	if ((iControlMode != PLAYER_CONTROLMODE_ANIMATION))
	{
		CAxisRotfp32 RotUnit;
		RotUnit.Unit();
		if (_pPhysState->Object_GetRotVelocity(_pObj->m_iObject).m_Angle != 0.0f)
			_pPhysState->Object_SetRotVelocity(_pObj->m_iObject, RotUnit);
	}


	if (!(_Press & CONTROLBITS_JUMP))
		_Released |= CONTROLBITS_JUMP;

	if (PhysType != PLAYER_PHYS_NOCLIP && (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE) &&
		((VRet[0] != 0) || (VRet[1] != 0)))
	{
		//Eliminate horizontal and upwards acceleration
		//Can occur due to fixed char move from fighting etc...
		return CVec3Dfp32(0,0,Max(0.0f, VRet[2]));
	}

	return VRet;
}

bool CWObject_Character::Phys_ControlAffectsMovement(const CControlFrame& _Msg, int _Pos, const CWObject_CoreData* _pObj, const CWO_Character_ClientData* _pCD)
{
	MSCOPESHORT(CWObject_Character::Phys_ControlAffectsMovement);
	MAUTOSTRIP(CWObject_Character_Phys_ControlAffectsMovement, false);
	int Ctrl;
	int Size;
	int dTime;
//	int ID = _Msg.m_Data[_Pos+3];
	_Msg.GetCmd(_Pos, Ctrl, Size, dTime);
	if (_Pos + Size > _Msg.m_MsgSize) return false;

	switch(Ctrl)
	{
		case CONTROL_STATEBITS:
			{
				if (Size == 4)
					return _Msg.GetInt32(_Pos) != _pCD->m_Control_Press;
				else
					return false;
			}

		case CONTROL_LOOK:
			if (Size == 6)
			{
				if (_pCD->m_Phys_nInAirFrames > 2)
					return false;

				CVec3Dfp32 Look = _pCD->m_Control_Look;

				CVec3Dfp32 dLook = _Msg.GetVecInt16_Max32768(_Pos);
//				ConOutL("(CWObject_Character::Char_ProcessControl) dLook " + dLook.GetString());
				dLook *= ( 1.0f / 65536 );

		//		m_Look[0] += dLook[0];
				Look[1] = Min(PLAYER_CAMERAMAXTILT, Max(-PLAYER_CAMERAMAXTILT, Look[1] + dLook[1]));
				Look[2] += dLook[2];

				return (_pCD->m_Control_Look != Look) != 0;
			}
			else
				return false;
		
		case CONTROL_MOVE:
			{
				CVec3Dfp32 Move;
				if (Size == 6)
					Move = _Msg.GetVecInt16_Max32768(_Pos) * (1.0f / 256.0f);
				else
					Move = 0;
			
				return (_pCD->m_Control_Move != Move) != 0;
			}

		case CONTROL_CMD:
			{
				int Cmd = _Msg.GetInt8(_Pos);
				return Cmd == 127;
			}

		default :
			return false;
	}
}

//#define CHAR_TELEPORTWATCH

void CWObject_Character::Phys_Move(const CSelection& _Selection, CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, CWorld_PhysState* _pPhysState, fp32 _dTime, const CVec3Dfp32& _UserVel, bool _bPredicted)
{
	MAUTOSTRIP(CWObject_Character_Phys_Move, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Character::Phys_Move);
	if (_pObj->IsDestroyed()) return;

	const int PhysType = CWObject_Character::Char_GetPhysType(_pObj);

	if (PhysType != PLAYER_PHYS_NOCLIP && _pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE)
	{
		//Since we can get velocity from other objects such as triggers etc
		//we reset any non-downwards velocity (assumed to be gravity) here
		CVec3Dfp32 Vel = _pObj->GetMoveVelocity();
		Vel[0] = 0.0f;
		Vel[1] = 0.0f;
		if (Vel[2] > 0.0f)
			Vel[2] = 0.0f;
		_pPhysState->Object_SetVelocity(_pObj->m_iObject, Vel);

#ifndef M_RTM
		bool bHaveParent = (_pPhysState->IsServer() && (((CWObject*)_pObj)->GetParent() > 0));
		CVec3Dfp32 Pos = _pObj->GetPosition();
		CVec3Dfp32 LastPos = _pObj->GetLastPosition();
		if (!bHaveParent && 
			((Pos[0] != LastPos[0]) ||
			(Pos[1] != LastPos[1])) &&
			(LastPos != CVec3Dfp32(0))) //Lastpos is origin at first refresh...This is unsafe, but so is this fix :/
		{
			//Damn! Some movement have slipped through... Perhaps some code forcing position instead of using velocity
			//Force position back to last position
			ConOutL("Aargh! The fucker is moving even though he should be immobile!");
		}
#endif
	}


//	_pCD->ControlSnapshot();

#ifndef M_RTM
	int bAGIEnable = true;
	if (_pCD->m_bIsServerRefresh)
	{
		CWorld_Server* pServer = (CWorld_Server*)_pPhysState;
		bAGIEnable = pServer->Registry_GetServer()->GetValuei("CHAR_AGI", 1);
	}
#else
	int bAGIEnable = true;
#endif

	const CMTime GameTime = _pCD->m_GameTime;
	bool bNoControlMove = (_pCD->m_Control_Move.LengthSqr() < 0.001f &&
		_pCD->m_Control_Press == 0);

	bool bAGRefreshEnabled = !((_pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_CANDISABLEREFRESH) && _pCD->m_AnimGraph2.GetPropertyBool(PROPERTY_BOOL_DISABLEREFRESH)
		&& bNoControlMove && (_pCD->m_AnimGraph2.GetImportantAGEvent() > 1));

	/*if (_pCD->m_iPlayer != 0)
	{
		const CWAGI_Token* pToken = _pCD->m_AnimGraph.m_AGI.GetTokenFromID(AG_TOKEN_MAIN);
		int32 StateIndex = (pToken ? pToken->GetStateIndex() : -1);
		if (!bAGRefreshEnabled)
			ConOut(CStrF("Woot, iObj: %d, iState: %d, disabled refresh: %d AGDR: %d NCM: %d IAGE: %d",_pObj->m_iObject, StateIndex, _pCD->GetStateFlagsLo(), _pCD->GetDisableRefresh(),bNoControlMove, _pCD->GetImportantAGEvent()));
		else
			ConOut(CStrF("Crap, iObj: %d, iState: %d, no disabled refresh: %d AGDR: %d NCM: %d IAGE: %d",_pObj->m_iObject, StateIndex, _pCD->GetStateFlagsLo(), _pCD->GetDisableRefresh(), bNoControlMove, _pCD->GetImportantAGEvent()));
	}*/

	// AGMerge
	if (_pCD != NULL && bAGIEnable)
	{
		if (_pCD->m_bIsServerRefresh || (_pCD->m_bIsClientRefresh && (((CWorld_Client*)_pPhysState)->Player_GetLocalObject() == _pObj->m_iObject)))
		{
			// Don't want to refresh with at time less than or equal last refresh...
			bool bOk = true;
			fp32 TimeSpan = _dTime * _pPhysState->GetGameTickTime();
			/*if (_pCD->m_AGRefreshLastTick == _pCD->m_GameTick)
				bOk = (TimeSpan > _pCD->m_AGRefreshLastFrac);
			if (bOk)
			{
				_pCD->m_AGRefreshLastTick = _pCD->m_GameTick;
				_pCD->m_AGRefreshLastFrac = TimeSpan;
			}*/
			if (!InNoClipMode(PhysType) && bOk && bAGRefreshEnabled && !_pCD->m_AnimGraph2.GetAG2I()->GetDisableAll())
			{
				CWAG2I_Context AG2Context(_pObj, _pPhysState, GameTime, TimeSpan);
				CWAG2I* pAG2I = _pCD->m_AnimGraph2.GetAG2I();
				if (pAG2I)
					pAG2I->Refresh(&AG2Context);
			}
		}
	}

	/*if (_pObj->m_iObject == 18)
		ConOutL(CStrF("%s: PlayerPos: %s", (_pPhysState->IsServer() ? "S":"C"),_pObj->GetPosition().GetString().GetStr()));*/

	CMovePhysInfo MPInfo;
	if (_bPredicted)
		MPInfo.m_Flags = SERVER_MOVEPHYSINFOFLAGS_NOEXTERNALEFFECTS;

	bool bForceRun = _pCD->m_RelAnimPos.IsMoving() || (_pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_APPLYTURNCORRECTION);
	
	/*if (Char_GetControlMode(_pObj) == PLAYER_CONTROLMODE_LEDGE2)
		bForceRun = true;*/
	/*if (bForceRun)
		_pCD->m_RelAnimPos.DoEffects(_pObj);*/
	// Hmm, do we really need this check, it takes an awful lot of time to perform
	// Should be able to do it better somehow...? (flag in states with anim velocity, some
	// extra work but will save alot of time.....?)
	// Right now we have the same check as ag refresh, if the AI says it is idle,
	// don't check it if we have a residue anim velocity, movevelocity will still be 
	// large and will be refreshed until idle....
	// Should also only be used if in animation controlmode
	const int32 ControlMode = Char_GetControlMode(_pObj);
	bForceRun = bForceRun || _pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOGRAVITY ||
		!(_pCD->m_Phys_IdleTicks > PLAYER_PHYS_IDLETICKS &&
		_pObj->GetMoveVelocity().LengthSqr() < 0.001f && bNoControlMove);

	// Don't check for anim velocity unless absolutely neccessary
	if (!bForceRun && ((ControlMode == PLAYER_CONTROLMODE_ANIMATION) || 
		(ControlMode == PLAYER_CONTROLMODE_LEDGE2) || (ControlMode == PLAYER_CONTROLMODE_LADDER)))
	{
		CWAG2I_Context AG2Context(_pObj, _pPhysState, GameTime);
		if (bAGRefreshEnabled && (_pCD->m_AnimGraph2.m_PhysImpulse.LengthSqr() > 0.01f || 
			_pCD->m_AnimGraph2.GetAG2I()->HasAnimVelocity(&AG2Context,0)))
			bForceRun = true;
	}

	if (bForceRun)
	{
		CVec3Dfp32 PrevPos = _pObj->GetLastPosition();

		// Do *not* allow immobile characters to move... (ugly patch)
		if (PhysType != PLAYER_PHYS_NOCLIP && _pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE)
		{
			CVelocityfp32 Zero; Zero.Unit();
			_pPhysState->Object_SetVelocity(_pObj->m_iObject, Zero);
		}

		_pPhysState->Object_MovePhysical(&_Selection, _pObj->m_iObject, _dTime, &MPInfo);

		// AndersO: PrevPos is the position at the start of the previous tick, so the distance calculated 
		// below is in fact the movement last tick plus the movement we just performed. Is this intentional?
		if (_pObj->GetPosition().DistanceSqr(PrevPos) > Sqr(128) && PrevPos != CVec3Dfp32(0))
		{
			// If we've teleported we'd better update the look in case we rotated.
			_pCD->m_Control_Look_Wanted = GetLook(_pObj->GetPositionMatrix());

#ifdef CHAR_TELEPORTWATCH
			ConOutL("-------------------------------------------------------------------");
			ConOutL(CStrF("WARNING: Teleported from %s to %s", PrevPos.GetString().Str(), _pObj->GetPosition().GetString().Str()));
			ConOutL(_pObj->Dump(_pPhysState->GetMapData(), -1));
			ConOutL(_pObj->GetMoveVelocity().GetString());
			ConOutL("-------------------------------------------------------------------");

			if (_pPhysState->IsServer())
			{
				CWObject* pObj = (CWObject*)_pObj;
				M_TRACE("T: %d - [Char %s, %d], WARNING: Teleported from %s to %s\n", _pPhysState->GetGameTick(), 
					pObj->GetName(), pObj->m_iObject, PrevPos.GetString().Str(), pObj->GetPosition().GetString().Str());

				if (pObj->GetPosition().k[2] < -100000.0f)
				{
					ConOutL(CStrF("WARNING: Destroying character %s (%d) because of crazy position (%s)", 
						pObj->GetName(), pObj->m_iObject, pObj->GetPosition().GetString().Str()));
					((CWorld_Server*)_pPhysState)->Object_Destroy(pObj->m_iObject);
				}
			}
#endif
		}

		// Auto tilt
		if ((_pCD->m_Control_AutoTiltCountDown == 0) && 
			(_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_ASSISTCAMERA) &&
			!IsForcedLook(_pCD))//(_pCD->m_bForceLookOnTarget == false))
		{
			if (/*(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_THIRDPERSON) &&*/
				!(PhysType == PLAYER_PHYS_NOCLIP) &&
				!(_pCD->m_CrosshairInfo & PLAYER_CROSSHAIR_VISIBLE) &&
				AssistCamera(_pCD))
			{
				fp32 TiltSpeed = Min(2.0f, _pObj->GetMoveVelocity().Length()) * 0.00125f;
				fp32 Tilt = _pCD->m_Control_Look_Wanted[1];

				if (Tilt < (PLAYER_AUTOTILT_ANGLE + 0.005f))//0.0f)
					Tilt = Tilt+TiltSpeed*_dTime;
				else if (Tilt > (PLAYER_AUTOTILT_ANGLE - 0.005f))//0.0f)
					Tilt = Tilt-TiltSpeed*_dTime;

				_pCD->m_Control_Look_Wanted[1] = Tilt;
				

				_pCD->Char_UpdateLook(_pCD->m_PredictFrameFrac + _dTime);
			}
		}


		// Epsilon value increased by kma
		if (_pObj->GetMoveVelocity().LengthSqr() < 0.001f && 
			(_pObj->GetPosition().DistanceSqr(PrevPos) < Sqr(0.01f)) &&
			 _pCD->m_iPlayer == -1 &&
			!_pCD->m_Phys_bInAir)
			_pCD->m_Phys_IdleTicks++;
		else
			_pCD->m_Phys_IdleTicks = 0;
		
		if (_pObj->IsDestroyed()) return;
	}

	if (_bPredicted && MPInfo.m_StepUpResult > 0.0f)
	{
		_pCD->m_Cam_BobMove.k[2] -= MPInfo.m_StepUpResult;
		_pCD->m_Cam_LastBobMove.k[2] -= MPInfo.m_StepUpResult;
		_pCD->m_StepUpDamper -= MPInfo.m_StepUpResult;
		_pCD->m_LastStepUpDamper -= MPInfo.m_StepUpResult;

//		ConOutL(CStrF("(CD %.8x) Adding %f to bob-z => %f", _pCD, MPInfo.m_StepUpResult, _pCD->m_Cam_BobMove.k[2]));
	}

	_pCD->m_GameTime += CMTime::CreateFromSeconds(_dTime * _pPhysState->GetGameTickTime());

	// Apply rotation velocity (only cosmetic)
	int bGotTurn = _pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_APPLYTURNCORRECTION;
	if (bGotTurn)
	{
		_pCD->m_TurnCorrectionLastAngle = _pCD->m_TurnCorrectionTargetAngle;
		fp32 TargetAngle = _pCD->m_TurnCorrectionTargetAngle + _pCD->m_AnimRotVel * _dTime;// * SERVER_TIMEPERFRAME;

		// Find max offset angle
		fp32 MaxOffset = ((fp32)(_pCD->m_AnimGraph2.GetMaxBodyOffset())) / 360.0f;
		if (TargetAngle < -MaxOffset)
			_pCD->m_TurnCorrectionTargetAngle = -MaxOffset;
		else if (TargetAngle > MaxOffset)
			_pCD->m_TurnCorrectionTargetAngle = MaxOffset;
		else
			_pCD->m_TurnCorrectionTargetAngle = TargetAngle;
	}
/*	if (!(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOLOOK))
	{
	CMat4Dfp32 Mat;
		_pCD->m_Control_Look.CreateMatrixFromAngles(0, Mat);
		_pPhysState->Object_SetRotation(_pObj->m_iObject, Mat);
	}*/

//	PhysUtil_Move(const CSelection& _Selection, CWObject_CoreData* _pObj, CWorld_PhysState* _pPhysState, const CPhysUtilParams& _Params, fp32 _dTime, const CVec3Dfp32& _UserVel, const CVec3Dfp32* _pMediumV, const CXR_MediumDesc* _pMediums, int _nMediums)
}



bool IntersectModel(int _iModel, const CMat4Dfp32& _WMat, const CXR_AnimState& _AnimState, CWorld_PhysState* _pWPhysState, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(IntersectModel, false);
	CXR_Model* pModel = _pWPhysState->GetMapData()->GetResource_Model(_iModel);
	if (!pModel) return false;
	CXR_PhysicsModel* pPhys = pModel->Phys_GetInterface();
	if (!pPhys) return false;

	CXR_PhysicsContext PhysContext(_WMat, &_AnimState, _pWPhysState->Debug_GetWireContainer());
	pPhys->Phys_Init(&PhysContext);
	return pPhys->Phys_IntersectLine(&PhysContext, _p0, _p1, _MediumFlags, _pCollisionInfo);
}

bool IntersectModel(CXR_Model* _pModel, const CMat4Dfp32& _WMat, const CXR_AnimState& _AnimState, CWorld_PhysState* _pWPhysState, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CCollisionInfo* _pCollisionInfo);
bool IntersectModel(CXR_Model* _pModel, const CMat4Dfp32& _WMat, const CXR_AnimState& _AnimState, CWorld_PhysState* _pWPhysState, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(IntersectModel_2, false);
	if (!_pModel) return false;
	CXR_PhysicsModel* pPhys = _pModel->Phys_GetInterface();
	if (!pPhys) return false;

	CXR_PhysicsContext PhysContext(_WMat, &_AnimState, _pWPhysState->Debug_GetWireContainer());
	pPhys->Phys_Init(&PhysContext);
	return pPhys->Phys_IntersectLine(&PhysContext, _p0, _p1, _MediumFlags, _pCollisionInfo);
}

CVec3Dfp32 PhysUtil_GetMediumAcceleration(const CSelection& _Selection, CWObject_CoreData* _pObj, CWorld_PhysState* _pPhysState, const CPhysUtilParams& _Params, const CVec3Dfp32* _pMediumV, const CXR_MediumDesc* _pMediums, int _nMediums);
CVec3Dfp32 PhysUtil_GetMediumAcceleration(const CSelection& _Selection, CWObject_CoreData* _pObj, CWorld_PhysState* _pPhysState, const CPhysUtilParams& _Params, const CVec3Dfp32* _pMediumV, const CXR_MediumDesc* _pMediums, int _nMediums)
{
	MAUTOSTRIP(PhysUtil_GetMediumAcceleration, CVec3Dfp32());
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if(!pCD)
		return 0;

//TODO: Remove this - velocities should be [units / second] instead of [units / tick]
#define PHYSSTATE_CONVERTFROM20HZ(x) ((x) * 20.0f * _pPhysState->GetGameTickTime())
	CVec3Dfp32 Gravity(0, 0, PHYSSTATE_CONVERTFROM20HZ(PHYSSTATE_CONVERTFROM20HZ(-16.0f/8.0f)));

 	if((pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_NOGRAVITY) || 
		pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_NOGRAVITY || 
		_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_SPECTATOR)
		Gravity = 0;

	int Flags = _Params.m_Flags;

//	if(!(pCD->m_Phys_Flags & (PLAYER_PHYSFLAGS_NOMEDIUMMOVEMENT | PLAYER_PHYSFLAGS_IMMOBILE)))
//		_pPhysState->Object_AddVelocity(_pObj->m_iObject, _UserVel /** _dTime*/);

	CVec3Dfp32 MediumVel = 0;
	fp32 nMediumInv = 1.0f / fp32(_nMediums);
	CMat4Dfp32 Rot;
	Rot.Unit();
	int nRot = 0;

	fp32 AvgThickness = 0.0f;

	if (_Params.m_PhysFlags & PHYSICAL_FLAGS_MEDIUMACCEL)
	{
		for(int v = 0; v < _nMediums; v++)
		{
			if(!(pCD->m_Phys_Flags & (PLAYER_PHYSFLAGS_NOMEDIUMMOVEMENT | PLAYER_PHYSFLAGS_IMMOBILE)))
			{
				// If you fix Medium velocity. Fully implement PLAYER_PHYSFLAGS_NOMEDIUMMOVEMENT eller nått
//			CVec3Dfp32 objv = _pPhysState->Object_GetVelocity(_pObj->m_iObject);
//			fp32 dmlen = (_pMediums[v].m_Velocity - objv).Length();	// Speed difference between the medium and the object.

//			_pPhysState->Object_AccelerateTo(_pObj->m_iObject, _pMediums[v].m_Velocity, CVec3Dfp32(_pMediums[v].m_Thickness) * _Params.m_MediumResist * dmlen * nMediumInv);

/*			CVec3Dfp32 objv = _pPhysState->Object_GetVelocity(_pObj->m_iObject);
			CVec3Dfp32 dVel;
			_pMediums[v].m_Velocity.Sub(objv, dVel);
			fp32 dmlen = dVel.Length();
			dVel *= Clamp01(_pMediums[v].m_Thickness * nMediumInv * _Params.m_MediumResist * dmlen);
			MediumVel += dVel;
*/
			}

			AvgThickness += _pMediums[v].m_Thickness;
			CVec3Dfp32 Vel(Gravity * (_Params.m_Density - _pMediums[v].m_Density) * nMediumInv);
			MediumVel += Vel;


			CVec3Dfp32 vTP;
			CVec3Dfp32 Axis;
			_pObj->GetPosition().Sub(_pMediumV[v], vTP);
	//if (nMediumSmp)
	//	ConOut(CStrF("        Medium %d, ", Mediums[v].m_MediumFlags) + vTP.GetString());
			Vel.CrossProd(vTP, Axis);
			fp32 MomentSqr = Axis.LengthSqr();
			if (MomentSqr > 0.001f)
			{	
				fp32 Moment = M_Sqrt(MomentSqr);
				Axis *= 1.0f / Moment;

				CMat4Dfp32 dRot, NewRot;
				Moment *= 0.005f;
				Axis.CreateAxisRotateMatrix(-Moment, dRot);
				for(int i = 0; i < 3; i++)
					for(int j = 0; j < 3; j++)
						Rot.k[i][j] += dRot.k[i][j];
				nRot++;

	//ConOut(CStrF("(CWObject_Physical::OnRefresh) Moment %f, ", Moment) + Axis.GetString() );
			}
		}

		if (Flags & 1) 
			if (MediumVel.k[2] < 0.0f) MediumVel.k[2] = 0;

/*ConOutL(CStrF("Gravity (%f) %s, Vel %s", _dTime, (char*)MediumVel.GetString(), (char*)_pObj->GetMoveVelocity().GetString() ));*/

//		_pPhysState->Object_AddVelocity(_pObj->m_iObject, MediumVel * _dTime);
		AvgThickness *= nMediumInv;
	}

	return MediumVel;
}

int CWObject_Character::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWObject_Character_OnPhysicsEvent, 0);
	switch(_Event)
	{
	case CWO_PHYSEVENT_IMPACTOTHER :
		{
			// Kill idle counter if something is interacting with this object.
			int Ret = SERVER_PHYS_DEFAULTHANDLER;
			CWO_Character_ClientData *pCD = GetClientData(_pObj);
			if (pCD) 
				pCD->m_Phys_IdleTicks = 0;
			return Ret;
		}
		break;

	// -------------------------------------------------------------------
	case CWO_PHYSEVENT_IMPACT :
		{
			int Ret = SERVER_PHYS_DEFAULTHANDLER;

			CWO_Character_ClientData *pCD = GetClientData(_pObj);
			if (!pCD) return Ret;

			const int PhysType = CWObject_Character::Char_GetPhysType(_pObj);

			if (_pCollisionInfo && _pCollisionInfo->m_bIsValid)
			{
				// -------------------------------------------------------------------
				// Check if _pObj is the server object and if it is run the server preintersection handler.
				//JK-FIX: Do this some other way! Don't check with dynamic_cast.
				CWObject_Character* pSvObj = TDynamicCast<CWObject_Character>(_pObj);
				if (pSvObj)
					pSvObj->Physics_Preintersection(_pCollisionInfo->m_iObject, _pCollisionInfo);
				
				// -------------------------------------------------------------------
				if (Char_GetControlMode(_pObj) == PLAYER_CONTROLMODE_FREE)
				{
					// Kneel
					{
						CVec3Dfp32 Vel = _pObj->GetMoveVelocity();
						const fp32 ImpactVel = -(Vel * _pCollisionInfo->m_Plane.n) / _pCollisionInfo->m_Plane.n.Length();

						if (ImpactVel > PLAYER_MIN_KNEEL_VELOCITY)
						{
							if (_pCollisionInfo->m_Plane.n.k[2] > PLAYER_PHYS_COS_MAX_SLOPE)
							{
								const fp32 t = Clamp01((ImpactVel - PLAYER_MIN_KNEEL_VELOCITY) / (PLAYER_MAX_KNEEL_VELOCITY - PLAYER_MIN_KNEEL_VELOCITY));
								pCD->m_Cam_Kneel = -(PLAYER_MIN_KNEEL_HEIGHT + t*(PLAYER_MAX_KNEEL_HEIGHT-PLAYER_MIN_KNEEL_HEIGHT));
							}
						}
						
						// Ok then, check if we landed on top of a guard, if so kill him 
						// (or stun him)
						// Maybe play some animation?

						/*if (_pPhysState->Phys_Message_SendToObject(
							CWObject_Message(OBJMSG_CHAR_CANBEDROPKILLED, 0,0, _pObj->m_iObject),
							_pCollisionInfo->m_iObject))
						{
							if (ImpactVel > PLAYER_MIN_DROPKILL_VELOCITY)
							{
								// Ok then, the thing we dropped on can be "drop killed", and the 
								// impact velocity is enough so kill him....
								
								// New test, make synced animation
								CWObject_CoreData* pTarget = _pPhysState->Object_GetCD(_pCollisionInfo->m_iObject);
								CWO_Character_ClientData* pCDTarget = (pTarget ? CWObject_Character::GetClientData(pTarget) : NULL);
								pCD->m_RelAnimPos.MakeSynchedAnim(_pPhysState,RELATIVEANIMPOS_MOVE_SNEAK_DROPKILL,pCD,_pObj,pCDTarget,pTarget,0.0f);
								pCD->m_RelAnimPos.MakeDirty();
								pCDTarget->m_RelAnimPos.MakeSynchedAnim(_pPhysState,RELATIVEANIMPOS_MOVE_SNEAK_DROPKILLED,pCDTarget, pTarget, pCD, _pObj, 0.0f, 0.15f);
								pCDTarget->m_RelAnimPos.MakeDirty();
							}
							else
							{
								// The impact velocity wasn't enough to kill him, maybe just stun 
								// him a little?
							}
						}*/
					}

					// Check if we are pushing against a wall, if so check for a ledge nearby to use....
					// First check that we are pushing against a wall like plane (abs(z)~0.05f
					// Also make we're not playing any effect animations
					if (!(pCD->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_HURTACTIVE) &&
						!(pCD->m_AnimGraph2.GetStateFlagsLoToken2() & (AG2_STATEFLAG_EFFECTOKENACTIVE | AG2_STATEFLAG_HURTACTIVE)) && 
						_pPhysState->IsServer() && pCD->m_Phys_bInAir
						&& !IsCharacter(_pCollisionInfo->m_iObject,_pPhysState))
					{
						// Depending on how the impacted surface looks like try to find either
						// hangrail or ledge
						if (_pCollisionInfo->m_Plane.n.k[2] < -0.9f)
						{
							int32 iSel = -1;
							int32 iCloseSel = -1;
							int8 SelType;
							bool bCurrentNoPrio = false;
							CWObject_Character::Char_FindStuff(_pPhysState,_pObj,pCD,iSel,
								iCloseSel, SelType,bCurrentNoPrio);

							if ((iSel != -1) && (SelType & SELECTION_MASK_TYPE) == SELECTION_HANGRAIL)
							{
								CWObject_Character::Char_ActivateStuff(_pPhysState,_pObj,pCD,
									iSel,SelType);
							}
						}
						else if (Abs(_pCollisionInfo->m_Plane.n.k[2]) < 0.05f)
						{
							// Remove "z dir"
							CVec3Dfp32 CharDir = CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(),0);
							CharDir.k[2] = 0.0f;
							CharDir.Normalize();
							// Hmm, what position do we have on the collision?
							CVec3Dfp32 DirToWall = _pCollisionInfo->m_Pos - _pObj->GetPosition();
							DirToWall.k[2] = 0.0f;
							DirToWall.Normalize();

							if ((CharDir * DirToWall) > 0.95f)
							{
								// Increase ledge counter
								#define LEDGEPOINTSPERHIT 3
								#define LEDGEPOINTSNEEDED 30
								pCD->m_LedgeClimbOnCounter += LEDGEPOINTSPERHIT;
								// Grab ledge if we have pressed against the wall, or directly if we're in the air
								//if ((pCD->m_LedgeClimbOnCounter >= LEDGEPOINTSNEEDED) || pCD->m_Phys_bInAir)
								{
									// Reset climbon counter
									pCD->m_LedgeClimbOnCounter = 0;
									// Going to find 
									//ConOut(CStrF("Hit a wall, trying to find ledge. Normal: %s",_pCollisionInfo->m_Plane.n.GetString().GetStr()));
									fp32 Range = 60.0f;//LEDGE_DEFAULTACTIVATIONRANGE;
									fp32 Width = (fp32)pCD->m_Phys_Width * 2.0f;
									int iBestLedge,BestLedgeType;
									fp32 LedgePos;
									int iLedge = CWObject_Ledge::FindBestLedge(_pPhysState, 
										_pObj->GetPosition(), _pObj->m_iObject, CharDir, Range, pCD->m_Phys_bInAir != 0, Width,iBestLedge, 
										BestLedgeType, LedgePos);
									
									if (iLedge != -1)
									{
										// Found a ledge, now grab it
										CWObject_Message LedgeMsg(OBJMSG_LEDGE_GRABLEDGE,BestLedgeType,iBestLedge,
											0,0,CVec3Dfp32(LedgePos,0,0),0,_pObj);
										_pPhysState->Phys_Message_SendToObject(LedgeMsg,iLedge);
										_pPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_ACTIVATE,0,0,
											_pObj->m_iObject), iLedge);
									}
								}
							}
						}
					}

					// Check if we've landed on something that will reset the 'air-timer', m_Phys_nInAirFrames
					if (_pCollisionInfo->m_Plane.n.k[2] > PLAYER_PHYS_COS_MAX_SLOPE)
					{
						// Send land impulse to the animgraph
						CWAG2I_Context AG2Context(_pObj, _pPhysState, pCD->m_GameTime);
						pCD->m_AnimGraph2.SendJumpImpulse(&AG2Context,CXRAG2_Impulse(AG2_IMPULSETYPE_JUMP,AG2_IMPULSEVALUE_JUMP_LAND));
						pCD->m_Phys_nInAirFrames = 0;
						pCD->m_Phys_iLastGroundObject = _pCollisionInfo->m_iObject;

						CVec3Dfp32 V = _pObj->GetMoveVelocity();
						if (V.LengthSqr() < Sqr(2.0f))
							V[2] = 0;
						_pPhysState->Object_SetVelocity(_pObj->m_iObject, V);

						CMat4Dfp32* pAccel = _pMat;
						if (pAccel)
						{
							pAccel->k[3][2] = 0;

						}
					}
					else if(_pCollisionInfo->m_Plane.n.k[2] >= 0.001f)
					{
						CVec3Dfp32 V = _pObj->GetMoveVelocity();
//					ConOut(CStrF("Slope V = %s", V.GetString().Str()));

						CVec3Dfp32 ProjN(_pCollisionInfo->m_Plane.n.k[0], _pCollisionInfo->m_Plane.n.k[1], 0);
						ProjN.Normalize();

						if (-(ProjN * V) < Sqr(2.0f))
						{

							if (V * ProjN < 0.0f)
							{
								if (V[2] > 0.0f)
									V[2] = 0;
								
								CVec3Dfp32 VNProj, VNew;
								if (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE)
								{
									VNew = V;
								}
								else
								{
									V.Project(ProjN, VNProj);
									V.Sub(VNProj, VNew);
								}

								_pPhysState->Object_SetVelocity(_pObj->m_iObject, VNew);
							}

							CMat4Dfp32* pAccel = _pMat;
							if (pAccel)
							{
								CVec3Dfp32& Accel = CVec3Dfp32::GetRow(*pAccel, 3);
								if (Accel * ProjN < 0.0f)
								{
									CVec3Dfp32 ANProj;
									if (Accel[2] > 0.0f)
										Accel[2] = 0;
									Accel.Project(ProjN, ANProj);
									Accel.Sub(ANProj, Accel);
//									int a = 1;
								}
							}
						}
					}
					else if(_pCollisionInfo->m_Plane.n.k[2] > -0.001f)
					{
						CVec3Dfp32 V = _pObj->GetMoveVelocity();
//					ConOut(CStrF("Wall V = %s", V.GetString().Str()));

						if (V[2] > 0.0f && V[2] < 2.0f)
						{
							if (V[2] > 0.0f)
								V[2] = 0;
							_pPhysState->Object_SetVelocity(_pObj->m_iObject, V);

							CMat4Dfp32* pAccel = _pMat;
							if (pAccel)
							{
								CVec3Dfp32& Accel = CVec3Dfp32::GetRow(*pAccel, 3);
								if (Accel[2] > 0.0f)
									Accel[2] = 0;
							}
						}
					}

					
					// Check if we have hit something above us, a horizontal ladder perhaps....
					// Only grab on if "grab" is pressed (secondary button)
					/*CVec3Dfp32 LadderNormal;
					CWObject_Message Msg(OBJMSG_LADDER_GETNORMAL, 0,0,_pObj->m_iObject, 0, 0, 
						0, &LadderNormal, sizeof(CVec3Dfp32));
					
					if (_pPhysState->Phys_Message_SendToObject(Msg, _pCollisionInfo->m_iObject))
					{
						// Ok hit a ladder of some sort, now determine what type of ladder it is
						int Type = _pPhysState->Phys_Message_SendToObject(
							CWObject_Message(OBJMSG_LADDER_GETTYPE, _pObj->m_iObject),
							_pCollisionInfo->m_iObject);

						// Set ladder object id at param3
						*(int32*)&(pCD->m_ControlMode_Param3) = _pCollisionInfo->m_iObject;

						switch (Type)
						{
						case CWOBJECT_LADDER_TYPE_HANGRAIL:
							{
								// Surface is a "horizontal" ladder...?
								// Only enter hangrail control mode if pressing "grab" button
								if (pCD->m_Control_Press & CONTROLBITS_SECONDARY)
								{
									Char_SetControlMode(_pObj, PLAYER_CONTROLMODE_HANGRAIL);
									CVec3Dfp32 Pos1, Pos2;
									int iLadder = _pCollisionInfo->m_iObject;

									_pPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION1,
										0,0,_pObj->m_iObject, 0,0,0,&Pos1, sizeof(CVec3Dfp32)), iLadder);
									_pPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION2,
										0,0,_pObj->m_iObject, 0,0,0,&Pos2, sizeof(CVec3Dfp32)), iLadder);

									CVec3Dfp32 ObjPos = _pObj->GetPosition();

									CVec3Dfp32 LadderDir = Pos2 - Pos1;
									LadderDir.Normalize();

									// Check which direction we are heading
									CVec3Dfp32 Direction;
									CVec3Dfp32 Look = pCD->m_Control_Look;
									CMat4Dfp32 MatLook;

									Look.CreateMatrixFromAngles(0, MatLook);
									Direction = CVec3Dfp32::GetRow(MatLook, 0);

									if (Direction * LadderDir < 0.0f)
									{
										LadderDir = -LadderDir;
									}

									pCD->m_ControlMode_Param0 = LadderDir.k[0];
									pCD->m_ControlMode_Param1 = LadderDir.k[1];
									pCD->m_ControlMode_Param2 = LadderDir.k[2];
				
									// Abort current animation...
									Char_SetAnimSequence(_pObj, _pPhysState, 0);

									// Make the character use the ladder camera
									_pPhysState->Phys_Message_SendToObject(
										CWObject_Message(OBJMSG_ACTIONCUTSCENE_ACTIVATE,0,0,
										_pObj->m_iObject), iLadder);
								}
								
								break;
							}
						case CWOBJECT_LADDER_TYPE_LADDER:
							{
								// Make sure the character is "close" to the ladder before making
								// him climb
								CVec3Dfp32 Pos1, Pos2;
								int iLadder = _pCollisionInfo->m_iObject;

								_pPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION1,
									0,0,_pObj->m_iObject, 0,0,0,&Pos1, sizeof(CVec3Dfp32)), iLadder);
								_pPhysState->Phys_Message_SendToObject(CWObject_Message(OBJMSG_LADDER_GETPOSITION2,
									0,0,_pObj->m_iObject, 0,0,0,&Pos2, sizeof(CVec3Dfp32)), iLadder);

								CVec3Dfp32 ObjPos = _pObj->GetPosition();
								CVec3Dfp32 Direction, LadderDir;

								Direction = Pos1 - ObjPos;
								Direction.Normalize();
								
								LadderDir = Pos2 - Pos1;
								LadderDir.Normalize();

								CVec3Dfp32 PointOnLadder = Pos1 + LadderDir * ((ObjPos - Pos1) * LadderDir);
								int Len = (PointOnLadder - ObjPos).Length();

								// Make sure we are sufficiently close to the ladder and facing it 
								// from the right direction
								if ((Len < 25.0f) && (Direction * LadderNormal < 0.0f) && (CVec3Dfp32::GetRow(_pObj->GetPositionMatrix(), 0) * LadderNormal < 0.0f))
								{
									// Abort current animation...
									Char_SetControlMode(_pObj, PLAYER_CONTROLMODE_LADDER);
									Char_SetAnimSequence(_pObj, _pPhysState, 0);
									
									// Make the character use the ladder camera
									_pPhysState->Phys_Message_SendToObject(
										CWObject_Message(OBJMSG_ACTIONCUTSCENE_ACTIVATE,0,0,
										_pObj->m_iObject), iLadder);


									pCD->m_ControlMode_Param0 = _pCollisionInfo->m_Plane.n[0];
									pCD->m_ControlMode_Param1 = _pCollisionInfo->m_Plane.n[1];
									pCD->m_ControlMode_Param2 = _pCollisionInfo->m_Plane.n[2];
								}

								break;
							}
						default:
							break;
						}
					}*/
					/*if (_pCollisionInfo->m_pSurface && 
						(_pCollisionInfo->m_pSurface->GetBaseFrame()->m_User[0] == 1) && 
						(pCD->m_iPlayer != -1) && (_pCollisionInfo->m_Plane.n.k[2] < 0.0f) &&
						(pCD->m_Control_Press & CONTROLBITS_SECONDARY))
					{
						// Surface is a "horizontal" ladder...?
						Char_SetControlMode(_pObj, PLAYER_CONTROLMODE_HANGRAIL);

						pCD->m_ControlMode_Param0 = _pCollisionInfo->m_Plane.n[0];
						pCD->m_ControlMode_Param1 = _pCollisionInfo->m_Plane.n[1];
						pCD->m_ControlMode_Param2 = _pCollisionInfo->m_Plane.n[2];
						pCD->m_ControlMode_Param3 = _pCollisionInfo->m_Plane.d;
						// Abort jump?!?! REMOVE !!!
						Char_SetAnimSequence(_pObj, _pPhysState, 0);
					}
					else if (_pCollisionInfo->m_pSurface && 
						(_pCollisionInfo->m_pSurface->GetBaseFrame()->m_User[0] == 1) && 
						(pCD->m_iPlayer != -1) && 
						(Abs(CVec3Dfp32(1,1,0) * _pCollisionInfo->m_Plane.n) > 0.75f))
					{
						
						Char_SetControlMode(_pObj, PLAYER_CONTROLMODE_LADDER);

						pCD->m_ControlMode_Param0 = _pCollisionInfo->m_Plane.n[0];
						pCD->m_ControlMode_Param1 = _pCollisionInfo->m_Plane.n[1];
						pCD->m_ControlMode_Param2 = _pCollisionInfo->m_Plane.n[2];
						pCD->m_ControlMode_Param3 = _pCollisionInfo->m_Plane.d;
					}*/

				}

				// -------------------------------------------------------------------
				// Jump
				if (((pCD->m_Control_Press & CONTROLBITS_JUMP) & pCD->m_Control_Released) &&
					(PhysType != PLAYER_PHYS_DEAD) &&													// no jump when dead
					!(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOMOVE) &&								// no jump in 'nomove' mode
					!(pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMOBILE) &&									// no jump in 'immobile' mode
					(Char_GetControlMode(_pObj) == PLAYER_CONTROLMODE_FREE) &&							// only jump in 'free' mode (normal)
					_pCollisionInfo->m_bIsValid &&														// only jump if on valid surface
					(_pCollisionInfo->m_Plane.n.k[2] > PLAYER_PHYS_COS_MAX_SLOPE) &&					// only jump on ground
					(pCD->m_AnimGraph2.GetForcedAimingType() != AIMING_TYPE_MINIGUN) &&					// no jump with minigun
					!(pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_DIALOGUE) &&						// no jump in dialogue
					!(pCD->m_Control_Press & CONTROLBITS_BUTTON5))										// no jump while pressing drain button
				{
					// NOTE: Parts of this code is duplicated in Phys_GetUserAccelleration
					_pPhysState->Phys_Message_SendToObject(
 						CWObject_Message(OBJMSG_CHAR_SETGRABBEDBODY,0), _pObj->m_iObject);

					CWAG2I_Context AG2Context(_pObj, _pPhysState,pCD->m_GameTime);
					pCD->m_AnimGraph2.SendJumpImpulse(&AG2Context,CXRAG2_Impulse(AG2_IMPULSETYPE_JUMP,AG2_IMPULSEVALUE_JUMP_UP));

					pCD->m_Control_Released &= ~CONTROLBITS_JUMP;

					const CVec3Dfp32& CurV = _pObj->GetMoveVelocity();
					const fp32 vz = Min(0.0f, CurV[2]);
					const fp32 fSpeed_Jump = pCD->m_Speed_Jump/* * Char()->JumpSpeed()*/;
					const fp32 JumpV = Max(-vz + fSpeed_Jump, fSpeed_Jump);
					// ConOut(CStrF("Jump AddVel %f", JumpV));
					_pPhysState->Object_AddVelocity(_pObj->m_iObject, CVec3Dfp32(0,0,JumpV));

					// Ruin accuracy.
					//pCD->Target_SetAccuracy(0);
/*
					// Anim junk
					int StanceBase = Char_GetAnimStance(_pObj);
					if(StanceBase == 0xfff)
						StanceBase = 0;
//					Char_SetAnimSequence(_pObj, _pPhysState, StanceBase + PLAYER_ANIM_SEQUENCE_JUMP);
*/
					Ret = SERVER_PHYS_HANDLED;
				}
			}

			return Ret;
		}
		break;

	// -------------------------------------------------------------------
	case CWO_PHYSEVENT_GETACCELERATION :
		{
			if (!_pMat) return SERVER_PHYS_DEFAULTHANDLER;
			_pMat->Unit();

			CWO_Character_ClientData *pCD = GetClientData(_pObj);
			if (!pCD) return SERVER_PHYS_DEFAULTHANDLER;

			//AR-PCS-HACK
			if (_pObjOther)
				pCD->m_pPhysPCS = (CPotColSet*)_pObjOther;

			// Get current medium

			const int MaxSamples = 3;
			int nMediums = 3;
			CXR_MediumDesc Mediums[MaxSamples];
			CVec3Dfp32 MediumV[MaxSamples];

			MediumV[0] = _pObj->GetPosition();
			MediumV[0].k[2] = Min(fp32(_pObj->GetAbsBoundBox()->m_Min[2] + pCD->m_SwimHeight), _pObj->GetAbsBoundBox()->m_Max[2]);
			MediumV[1] = _pObj->GetPosition();
			MediumV[1].k[2] = _pObj->GetAbsBoundBox()->m_Min[2];
			MediumV[2] = _pObj->GetPosition();
			MediumV[2].k[2] = _pObj->GetAbsBoundBox()->m_Max[2]+15;

			if (pCD->m_PhysSelection!=NULL)
				_pPhysState->Phys_GetMediums(*pCD->m_PhysSelection, MediumV, nMediums, Mediums);
			else
				_pPhysState->Phys_GetMediums(MediumV, nMediums, Mediums);

			/*pCD->m_PreviousFeetMediumType = pCD->m_FeetMediumType;
			pCD->m_FeetMediumType = Mediums[1].m_MediumFlags;
			pCD->m_PreviousHeadMediumType = pCD->m_HeadMediumType;
			pCD->m_HeadMediumType = Mediums[2].m_MediumFlags;*/
			
			const int PhysType = CWObject_Character::Char_GetPhysType(_pObj);
			CPhysUtilParams Params;
			if(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_GHOST ||
				PhysType == PLAYER_PHYS_NOCLIP)
				Params.m_PhysFlags = 0;
			else
				Params.m_PhysFlags = PHYSICAL_FLAGS_MEDIUMACCEL | PHYSICAL_FLAGS_MEDIUMROTATE;

			CVec3Dfp32::GetRow(*_pMat, 3) = 
				Phys_GetUserAccelleration(*pCD->m_PhysSelection, _pObj, NULL, _pPhysState, 
					GetAdjustedMovement(pCD), pCD->m_Control_Look, 
					pCD->m_Control_Press, pCD->m_Control_Released, 
					Mediums[0], Params.m_Flags);

			// Convert from 20hz
			CVec3Dfp32::GetRow(*_pMat, 3) *= 20.0f * _pPhysState->GetGameTickTime();

			pCD->m_Phys_bInAir = (Params.m_Flags & 2) ? true : false;
			if (!pCD->m_Phys_bInAir)
				pCD->m_Phys_nInAirFrames = 0;

			CVec3Dfp32 Accel = PhysUtil_GetMediumAcceleration(*pCD->m_PhysSelection, _pObj, _pPhysState, Params, MediumV, Mediums, 1);

			CVec3Dfp32::GetRow(*_pMat, 3) *= 2.0f;
			CVec3Dfp32::GetRow(*_pMat, 3) += Accel;

			//AR-PCS-HACK
			pCD->m_pPhysPCS = NULL;

#ifdef WADDITIONALCAMERACONTROL
			CMat4Dfp32 MatLookAdd;
			CVec3Dfp32 LookAdd = 0.0f;
			LookAdd[1] = pCD->m_AdditionalCameraControlY;
			LookAdd[2] = pCD->m_AdditionalCameraControlZ;

			LookAdd.CreateMatrixFromAngles(0, MatLookAdd);
			CVec3Dfp32 dVAdd = 0.0f;
			dVAdd += CVec3Dfp32::GetRow(MatLookAdd, 0) * (pCD->m_Control_MoveAdditional.m_Value[0] * (fp32)pCD->m_Speed_Forward);
			dVAdd += CVec3Dfp32::GetRow(MatLookAdd, 1) * (pCD->m_Control_MoveAdditional.m_Value[1] * (fp32)pCD->m_Speed_SideStep);
			dVAdd += CVec3Dfp32::GetRow(MatLookAdd, 2) * (pCD->m_Control_MoveAdditional.m_Value[2] * (fp32)pCD->m_Speed_Up);
			pCD->m_AdditionalCameraPos = pCD->m_AdditionalCameraPos.m_Value + dVAdd * _dTime;
#endif

//ConOut(CStrF("(CWObject_Character::OnPhysicsEvent) Accel %s", (char*) CVec3Dfp32::GetRow(*_pMat, 3).GetString() ));

			return SERVER_PHYS_DEFAULTHANDLER;
		}
		break;

	case CWO_PHYSEVENT_DYNAMICS_COLLISION:
		{
			if (_pPhysState->IsServer() && _pObjOther)
			{
				CWObject_Character* pThis = safe_cast<CWObject_Character>(_pObj);
				CWO_Character_ClientData* pCD = GetClientData(_pObj);
				CWObject* pObj = safe_cast<CWObject>(_pObjOther);

				bool bIsPlayer = (pCD->m_iPlayer >= 0); // don't let the player character take physics damage... (temp x06?)
				int iOtherObj = pObj->m_iOwner ? pObj->m_iOwner : pObj->m_iObject;
				if (!bIsPlayer && (iOtherObj != pThis->m_iObject) && !(pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_CHARACTER))
				{
					const CVec3Dfp32& CollPos = _pCollisionInfo->m_Pos;

					// If we're burning, try to set other object on fire
					if (pCD->m_Burnable.m_Value.IsBurning())
					{
						CWO_DamageMsg FireDmg(0, DAMAGETYPE_FIRE, &CollPos);
						FireDmg.Send(iOtherObj, pThis->m_iObject, _pPhysState);
					}

					// Receive damage?
				/*	const CVec3Dfp32& OtherVel = _pCollisionInfo->m_LocalNodePos.GetRow(0);   // reuse hack
					fp32 RelVelSign = _pCollisionInfo->m_Velocity.k[2];
					CVec3Dfp32 Center;
					pThis->GetAbsBoundBox()->GetCenter(Center);
					CVec3Dfp32 RelPos = Center - CollPos;
					RelPos.Normalize();
					CVec3Dfp32 RelVel = OtherVel - pThis->GetMoveVelocity();
					fp32 ImpactScale1 = RelPos * RelVel;
					fp32 ImpactScale2 = OtherVel * RelVel;
					M_TRACE("Coll - ImpactScale:%.2f, ImpactScale2: %.2f\n", ImpactScale1, ImpactScale2);
					if (ImpactScale1 > 0.0f && ImpactScale2 > 0.0f)
					{
						fp32 Mass = pObj->GetMass();
						fp32 Damage = M_Sqrt(Mass) * 0.05f * ImpactScale1;
						M_TRACE("- Damage: %.2f\n", Damage);
				*/
					CVelocityfp32 Vel = pObj->GetVelocity();
					fp32 Amount = _pCollisionInfo->m_Velocity.k[0];
					fp32 Mass = _pObjOther->GetMass();
					fp32 Damage = Amount * M_Sqrt(Mass) * 0.05f;
					//M_TRACE("[Char %d, %s], rigid body collision! (OtherObj: %d, Amount: %.1f, Mass: %.1f, Damage: %.1f)\n", pThis->m_iObject, pThis->GetName(), iOtherObj, Amount, Mass, Damage);
					if (Damage > 2.0f && Amount > 2.0f && (Vel.m_Move.LengthSqr() > 5.0f || Vel.m_Rot.m_Angle > 0.001f))
					{
						//M_TRACE("[Char %d, %s], rigid body collision! (OtherObj: %d, Amount: %.1f, Mass: %.1f, Damage: %.1f)\n", pThis->m_iObject, pThis->GetName(), iOtherObj, Amount, Mass, Damage);
						if (Damage > 2.0f)
						{
							CVec3Dfp32 c1, c2;
							pThis->GetAbsBoundBox()->GetCenter(c1);
							pObj->GetAbsBoundBox()->GetCenter(c2);
							fp32 AbsVel = Vel.m_Move.Length();
							CVec3Dfp32 Force = Vel.m_Move + (c1 - c2).SetLength(AbsVel); // just a mix between object vel direction and object-to-char direction
							Force.SetLength( AbsVel * 1.0f ); // tweak!

							CWO_DamageMsg Msg(int(Damage), DAMAGETYPE_PHYSICS, &CollPos, &Force);
							pThis->Physics_Damage(Msg, iOtherObj);
						}
					}
				}
			}
		}
		return 0;

	default:
		return CWObject_RPG::OnPhysicsEvent(_pObj, _pObjOther, _pPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);
	}
	return SERVER_PHYS_DEFAULTHANDLER;
}

bool CWObject_Character::OnIntersectLine(CWO_OnIntersectLineContext& _Context, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWObject_Character_OnIntersectLine, false);
	int CollisionType = (_pCollisionInfo) ? _pCollisionInfo->m_CollisionType : CXR_COLLISIONTYPE_PHYSICS;
	CWObject_CoreData* _pObj = _Context.m_pObj;
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if (!pCD)
		return false;

	if (_pCollisionInfo == NULL)
	{
		CollisionType = CXR_COLLISIONTYPE_PHYSICS;
	}

	if (CollisionType == CXR_COLLISIONTYPE_PHYSICS)
	{
		return CWObject_RPG::OnIntersectLine(_Context, _pCollisionInfo);
	}
	else
	{
		CWorld_PhysState* _pWPhysState = _Context.m_pPhysState;

/*		if ((_pObj->ClientFlags() & PLAYER_CLIENTFLAGS_BLOCKING) && 
			(CollisionType == CXR_COLLISIONTYPE_PROJECTILE))*/
		if (false)
		{
			CXR_PhysicsModel_Sphere Sphere;
			Sphere.Phys_SetDimensions(120);
			CXR_PhysicsContext PhysContext(_pObj->GetPositionMatrix());
			Sphere.Phys_Init(&PhysContext);

/*
			CXR_VCModelInstance* pObj = &pVC->m_lObjects[iObj];
			if (!pObj->m_pModel) continue;

			CBox3Dfp32 Box;
			pObj->m_pModel->GetBound_Box(Box, &pObj->m_Anim);
			CMat4Dfp32 Move, L2V;
			Move.Unit();
			CVec3Dfp32::GetMatrixRow(pObj->m_Pos, 3).SetMatrixRow(Move, 3);

			pObj->m_Pos.Multiply(pVC->m_W2VMat, L2V);

			{
				CXR_VertexBuffer *pVB = CXR_Util::Create_Box(GetVBM(), L2V, Box, 0xff5050ff);
				if(pVB)
					GetVBM()->AddVB(pVB);
			}

			{
				CXR_VertexBuffer *pVB = CXR_Util::Create_Sphere(GetVBM(), L2V, pObj->m_pModel->GetBound_Sphere(&pObj->m_Anim), 0xffff2020);
				if(pVB)
					GetVBM()->AddVB(pVB);
			}
*/

			CCollisionInfo CInfo;
			if (Sphere.Phys_IntersectLine(&PhysContext, _Context.m_p0, _Context.m_p1, _Context.m_MediumFlags, (_pCollisionInfo) ? &CInfo : NULL))
			{
				if (_pCollisionInfo == NULL) return true;

				CInfo.m_iObject = _pObj->m_iObject;

				if (CInfo.m_bIsValid)
					if ((!_pCollisionInfo->m_bIsValid) || (_pCollisionInfo->m_Time > CInfo.m_Time))
						*_pCollisionInfo = CInfo;

				return true;
			}

			return false;
		}
		else
		{
			CXR_AnimState Anim;
			CXR_Skeleton* pSkel;
			CXR_SkeletonInstance* pSkelInstance;					
			if(!GetEvaluatedPhysSkeleton(_pObj, _Context.m_pPhysState, pSkelInstance, pSkel, Anim))
				return false;

			bool bHit = false;

			CXR_SkeletonInstance* pSkelInst = Anim.m_pSkeletonInst;
			if (pSkelInst)
			{
				if (pCD->m_Item0_Model.IsValid() && !(pCD->m_Item0_Flags & RPG_ITEM_FLAGS_NORENDERMODEL) && (pCD->m_Item0_Flags & RPG_ITEM_FLAGS_EQUIPPED))
				{
					CXR_AnimState ItemAnim;
					CMat4Dfp32 ItemPos;
					CXR_Model* pItemModel;

					if(pCD->m_Item0_Model.GetModel0_RenderInfo(_pWPhysState->GetMapData(), NULL, Anim.m_pSkeletonInst, pSkel, pCD->m_GameTime, 
															   ItemAnim, ItemPos, pItemModel, NULL))
					{
						CCollisionInfo CInfo;
						CInfo.m_CollisionType = CollisionType;
						if (IntersectModel(pItemModel, ItemPos, ItemAnim, _pWPhysState, _Context.m_p0, _Context.m_p1, _Context.m_MediumFlags, (_pCollisionInfo) ? &CInfo : NULL))
						{
							bHit = true;
							if (!_pCollisionInfo)
								return true;
							if (_pCollisionInfo->Improve(CInfo))
							{
								int iNode = pCD->m_Item0_Model.m_iAttachRotTrack;
								CMat4Dfp32 MInv;
								if ((iNode >= 0) && (iNode < pSkelInst->m_nBoneTransform))
								{
									pSkelInst->GetBoneTransform(iNode).InverseOrthogonal(_pCollisionInfo->m_LocalNodePos);
									_pCollisionInfo->m_LocalNode = iNode;								
								}
								else
								{
//									MInv.Unit();
									_pCollisionInfo->m_LocalNode = 0;
								}
							}
						}
					}
				}
				if (pCD->m_Item1_Model.IsValid() && !(pCD->m_Item1_Flags & RPG_ITEM_FLAGS_NORENDERMODEL) && (pCD->m_Item1_Flags & RPG_ITEM_FLAGS_EQUIPPED))
				{
					CXR_AnimState ItemAnim;
					CMat4Dfp32 ItemPos;
					CXR_Model* pItemModel;

					if(pCD->m_Item1_Model.GetModel0_RenderInfo(_pWPhysState->GetMapData(), NULL, Anim.m_pSkeletonInst, pSkel, pCD->m_GameTime, 
															   ItemAnim, ItemPos, pItemModel,NULL))
					{
						CCollisionInfo CInfo;
						CInfo.m_CollisionType = CollisionType;
						if (IntersectModel(pItemModel, ItemPos, ItemAnim, _pWPhysState, _Context.m_p0, _Context.m_p1, _Context.m_MediumFlags, (_pCollisionInfo) ? &CInfo : NULL))
						{
							bHit = true;
							if (!_pCollisionInfo)
								return true;
							if (_pCollisionInfo->Improve(CInfo))
							{
								int iNode = pCD->m_Item1_Model.m_iAttachRotTrack;
								CMat4Dfp32 MInv;
								if ((iNode >= 0) && (iNode < pSkelInst->m_nBoneTransform))
								{
									pSkelInst->GetBoneTransform(iNode).InverseOrthogonal(_pCollisionInfo->m_LocalNodePos);
									_pCollisionInfo->m_LocalNode = iNode;
								}
								else
								{
//									MInv.Unit();
									_pCollisionInfo->m_LocalNode = 0;
								}
							}
						}
					}
				}
			}

			bool bFoundPhysModel = false;
			CCollisionInfo CInfo;
			CInfo.m_CollisionType = CollisionType;
			int16 liModel[] = { _pObj->m_iModel[0], _pObj->m_iModel[1], _pObj->m_iModel[2], NULL };
			if (pCD->m_HeadGibbed != 0)
				liModel[0] = 0;

			for(int i = 0; i < 3; i++)
			{
				if(liModel[i] > 0)
				{
					CXR_Model* pModel = _pWPhysState->GetMapData()->GetResource_Model(liModel[i]);
					if (!pModel)
						continue;

					CXR_PhysicsModel* pPhys = pModel->Phys_GetInterface();
					if (pPhys)
					{
						bFoundPhysModel = true;
						if (IntersectModel(liModel[i], _pObj->GetPositionMatrix(), Anim, _pWPhysState, _Context.m_p0, _Context.m_p1, _Context.m_MediumFlags, (_pCollisionInfo) ? &CInfo : NULL))
						{
							bHit = true;
							if (!_pCollisionInfo)
								return true;
							_pCollisionInfo->Improve(CInfo);
						}
					}
				}
			}
			if (!bFoundPhysModel)
			{
				// If no phys interface (trimesh without solids), fallback to bound check (TODO: Could check multiple times, but this is just a fallback anyway, so...)
				if (CWObject_RPG::OnIntersectLine(_Context, _pCollisionInfo ? &CInfo : NULL))
				{
					bHit = true;
					if (!_pCollisionInfo)
						return true;
					_pCollisionInfo->Improve(CInfo);
				}
			}
			return bHit;
		}
	}
}

int CWObject_Character::OnClientPreIntersection(CWObject_Client* _pObj, CWorld_Client* _pWClient, int _iObject, CCollisionInfo *_pInfo)
{
	MAUTOSTRIP(CWObject_Character_OnClientPreIntersection, 0);
	// Obsolete, all relevant stuff moved to OnPhysicsEvent().

	return SERVER_PHYS_DEFAULTHANDLER;


#ifdef NEVER
	CWObject_CoreData* pObj = _pWClient->Object_Get(_iObject);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);

	if(pCD && pObj && (pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PHYSMODEL))
	{	
		if (_pInfo && _pInfo->m_bIsValid)
		{
			CVec3Dfp32 Vel = _pObj->GetMoveVelocity();
			const fp32 ImpactVel = -(Vel * _pInfo->m_Plane.n) / _pInfo->m_Plane.n.Length();

			if (ImpactVel > PLAYER_FALLVELOCITYSOUND0)
			{
//				ConOutL(CStrF("Frame %d, Vel %f", m_pWServer->GetFrame(), ImpactVel));
/*				int iSnd;
				if (ImpactVel > PLAYER_FALLVELOCITYSOUND2)
					iSnd = m_iSoundLand[2];
				else if (ImpactVel > PLAYER_FALLVELOCITYSOUND1)
					iSnd = m_iSoundLand[1];
				else
					iSnd = m_iSoundLand[0];

				m_FallDmgTimeout = 3;
*/
				// Only accept damage when colliding to ground, not walls or ceiling.
/*				if (_pInfo->m_Plane.n.k[2] > PLAYER_PHYS_COS_MAX_SLOPE)
				{
					if (ImpactVel > PLAYER_FALLDAMAGEHEIGHT)
					{
						int Dmg = Sqr(0.5f*(ImpactVel-PLAYER_FALLDAMAGEHEIGHT));
						if (Dmg)
						{
//							iSnd = m_iSoundLand[2];
							m_pWServer->MessageQueue_SendToObject(CWObject_Message(OBJMSG_PHYSICS_DAMAGE, Sqr(0.5f*(ImpactVel-PLAYER_FALLDAMAGEHEIGHT)), DAMAGE_FALL, -1), m_iObject);
						}
					}
					m_pWServer->Sound_At(GetPosition(), iSnd, 0);
					
				}*/
			}

			if (ImpactVel > PLAYER_MIN_KNEEL_VELOCITY)
			{
				if (_pInfo->m_Plane.n.k[2] > PLAYER_PHYS_COS_MAX_SLOPE)
				{
					fp32 t = (ImpactVel - PLAYER_MIN_KNEEL_VELOCITY) / (PLAYER_MAX_KNEEL_VELOCITY - PLAYER_MIN_KNEEL_VELOCITY);
					t = Clamp01(t);
					pCD->m_Cam_Kneel = -(PLAYER_MIN_KNEEL_HEIGHT + t*(PLAYER_MAX_KNEEL_HEIGHT-PLAYER_MIN_KNEEL_HEIGHT));
				}
			}
		}
	}
	return SERVER_PHYS_CONTINUE;
#endif
}
