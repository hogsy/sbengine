#include "PCH.h"

#include "WObj_Char.h"
#include "WObj_Sys/WObj_Trigger.h"
#include "WRPG/WRPGFist.h"

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character miscellaneous helper functions
\*____________________________________________________________________________________________*/


// =============================================================================
//
// Function:			Retrieves the position in world-coordinates 
//						of an attachpoint
//						
// Comments:			If u want to offset the position before the 
//						transformation with the bonematrix, use the 
//						_Offset parameter.
//
// =============================================================================
bool GetAttachPos(CWObject_CoreData* _pObj, CXR_SkeletonInstance *_pSkelInstance, CXR_Skeleton *_pSkel, int _iRotTrack, int _iAttachPoint, CVec3Dfp32 &_CameraPos, CVec3Dfp32 _Offset)
{
	MAUTOSTRIP(GetAttachPos, false);

	CMat4Dfp32 Pos;
	if ((_iRotTrack >= 0) && (_iRotTrack < _pSkelInstance->m_nBoneTransform))
		_pSkelInstance->GetBoneTransform(_iRotTrack, &Pos);

	CVec3Dfp32 v(0);
	const CXR_SkeletonAttachPoint* pPoint = _pSkel->GetAttachPoint(_iAttachPoint);
	if (pPoint) v = pPoint->m_LocalPos.GetRow(3);
	v += _Offset;
	_CameraPos = v * Pos;
	return true;
}

// =============================================================================
//
// Function:			Returns true if the screen is in Widescreen mode.
//						
// Comments:			None
//
// =============================================================================
bool IsWidescreen()
{
	MAUTOSTRIP(IsWidescreen, false);
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys && pSys->m_spDisplay && pSys->m_spDisplay->IsWidescreen())
		return true;
	return false;
}

// =============================================================================
//
// Function:			Retrieves the current display aspect
//						
// Comments:			None
//
// =============================================================================
fp32 GetScreenAspect()
{
	MAUTOSTRIP(GetScreenAspect, 1.0f);
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys && pSys->m_spDisplay /*&& pSys->m_spDisplay->IsWidescreen()*/)
		return pSys->m_spDisplay->GetPixelAspect(); 
	return 1.0f;
}

bool IsForcedLook(const CWO_Character_ClientData *_pCD)
{
	MAUTOSTRIP(IsForcedLook, false);
	if( !_pCD ) return false;
	return ((_pCD->m_bForceLook != 0) || (_pCD->m_bForceLookOnTarget != 0));
}

// Added by Mondelore.
bool IsAlive(const int _PhysType)
{
	MAUTOSTRIP(IsAlive, false);
	return ((_PhysType == PLAYER_PHYS_DEAD) == 0);
}

// Added by Mondelore.
bool InNoClipMode(const int _PhysType)
{
	MAUTOSTRIP(InNoClipMode, false);
	return ((_PhysType == PLAYER_PHYS_NOCLIP) != 0);
}

// Added by Mondelore.
bool InCutscene(const CWObject_CoreData* _pObj)
{
	MAUTOSTRIP(InCutscene, false);
	bool result = ((_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE) != 0);
	return result;
}

// Added by Mondelore.
bool AssistAim(const CWO_Character_ClientData *_pCD)
{
	MAUTOSTRIP(AssistAim, false);
	if( !_pCD ) return false;
	return ((_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_ASSISTAIM) != 0);
}

// Added by Mondelore.
bool AssistCamera(const CWO_Character_ClientData *_pCD)
{
	MAUTOSTRIP(AssistCamera, false);
	if( !_pCD ) return false;
	return ((_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_ASSISTCAMERA) != 0);
}

// Added by Mondelore.
int	GetCameraMode(const CWO_Character_ClientData *_pCD, const int _PhysType)
{
	MAUTOSTRIP(GetCameraMode, 0);
	if( !_pCD ) return PLAYER_CAMERAMODE_NORMAL;

	if(_PhysType == PLAYER_PHYS_DEAD)
		return PLAYER_CAMERAMODE_NORMAL;

	if (_pCD->m_MaxZoom > 1.0f)
		return PLAYER_CAMERAMODE_FIRSTPERSON;

	return ((_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_CAMERAMODE_MASK) >> PLAYER_EXTRAFLAGS_CAMERAMODE_SHIFT);
}

void ModerateQuat(CQuatfp32& q, const CQuatfp32& newq, CQuatfp32& qprim, int a)
{
	MAUTOSTRIP(ModerateQuat, MAUTOSTRIP_VOID);
	const fp32 Scale = 2048.0f;
	for(int i = 0; i < 4; i++)
	{
		int x = (int)(q.k[i]*Scale);
		int xprim = (int)(qprim.k[i]*Scale);
		Moderate(x, (int)(newq.k[i]*Scale), xprim, a);
		q.k[i] = fp32(x) / Scale;
		qprim.k[i] = fp32(xprim) / Scale;
	}

	q.Normalize();
}

void ModerateVec3f(CVec3Dfp32& q, const CVec3Dfp32& newq, CVec3Dfp32& qprim, int a)
{
	MAUTOSTRIP(ModerateVec3f, MAUTOSTRIP_VOID);
	const fp32 Scale = 512.0f;
	for(int i = 0; i < 3; i++)
	{
		int x = (int)(q.k[i]*Scale);
		int xprim = (int)(qprim.k[i]*Scale);
		Moderate(x, (int)(newq.k[i]*Scale), xprim, a);
		q.k[i] = fp32(x) / Scale;
		qprim.k[i] = fp32(xprim) / Scale;
	}
}

void ModerateVec2f(CVec2Dfp32& q, const CVec2Dfp32& newq, CVec2Dfp32& qprim, int a)
{
	MAUTOSTRIP(ModerateVec2f, MAUTOSTRIP_VOID);
	const fp32 Scale = 512.0f;
	for(int i = 0; i < 2; i++)
	{
		int x = (int)(q.k[i]*Scale);
		int xprim = (int)(qprim.k[i]*Scale);
		Moderate(x, (int)(newq.k[i]*Scale), xprim, a);
		q.k[i] = fp32(x) / Scale;
		qprim.k[i] = fp32(xprim) / Scale;
	}
}



// How much is already adjusted in the animation
fp32 GetAnimPhysAldreadyAdjusted(int32 _AnimPhysMoveType)
{
	switch (_AnimPhysMoveType)
	{
	case ANIMPHYSMOVETYPE_BACKWARD:
		{
			return 0.5f;
		}
	case ANIMPHYSMOVETYPE_LEFT:
		{
			return -0.25f;
		}
	case ANIMPHYSMOVETYPE_RIGHT:
		{
			return 0.25f;
		}
	default:
		return 0.0f;
	}
}

extern fp32 AngleAdjust(fp32 _AngleA, fp32 _AngleB);
void AdjustTurnCorrection(CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, int32 _MoveType, CWorld_PhysState* _pWPhys)
{
	#define PLAYER_TURN_CHANGESPEED (320.0f)

	fp32 WantedAngle;
	if (_MoveType == 0)
	{
		// Just moderate towards current targetangle
		//WantedAngle = _pCD->m_TurnCorrectionTargetAngle;
		_pCD->m_TurnCorrectionLastAngle = _pCD->m_TurnCorrectionTargetAngle;
		return;
	}
	else if (_MoveType != ANIMPHYSMOVETYPE_RESET)
	{

		fp32 MoveAngleUnitControl;
		fp32 MoveAngleUnit;
		MoveAngleUnitControl = _pCD->m_AnimGraph2.GetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITCONTROL);
		MoveAngleUnit = _pCD->m_AnimGraph2.GetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNIT);

		const int32 ControlMode = CWObject_Character::Char_GetControlMode(_pObj);
		switch (ControlMode)
		{
		case PLAYER_CONTROLMODE_ANIMATION:
			{
				WantedAngle = MoveAngleUnitControl;
				break;
			}
		case PLAYER_CONTROLMODE_CARRYBODY:
		case PLAYER_CONTROLMODE_FREE:
			{
				WantedAngle = MoveAngleUnit;
				break;
			}
		default:
			{
				WantedAngle = 0.0f;
				break;
			}
		}
	}
	else
	{
		WantedAngle = 0.0f;
	}

	// Should turn wanted angle towards the closest one as well!!!!
	WantedAngle = ((WantedAngle < 0.5f) ? WantedAngle : (WantedAngle - 1.0f));
	WantedAngle -= GetAnimPhysAldreadyAdjusted(_MoveType);
	WantedAngle = MACRO_ADJUSTEDANGLE(WantedAngle);
	WantedAngle = ((WantedAngle < 0.5f) ? WantedAngle : (WantedAngle - 1.0f));
//	fp32 WantedCached = WantedAngle;
	WantedAngle *= 100.0f;
	fp32 TargetAngle = _pCD->m_TurnCorrectionTargetAngle * 100.0f;
	//ConOut(CStrF("WantedAngle: %f",WantedAngle));
	_pCD->m_TurnCorrectionLastAngle = _pCD->m_TurnCorrectionTargetAngle;
	fp32 AngleChange = _pCD->m_TurnCorrectionAngleChange * 100.0f;
	Moderatef(TargetAngle, WantedAngle, AngleChange,  (int)(PLAYER_TURN_CHANGESPEED * 20.0f * _pWPhys->GetGameTickTime()));
	_pCD->m_TurnCorrectionTargetAngle = TargetAngle / 100.0f;
	_pCD->m_TurnCorrectionAngleChange = AngleChange / 100.0f;

	TargetAngle = _pCD->m_TurnCorrectionTargetAngle;
	// Find max offset angle
	fp32 MaxOffset = ((fp32)(_pCD->m_AnimGraph2.GetMaxBodyOffset())) / 360.0f;
	if (TargetAngle < -MaxOffset)
		_pCD->m_TurnCorrectionTargetAngle = -MaxOffset;
	else if (TargetAngle > MaxOffset)
		_pCD->m_TurnCorrectionTargetAngle = MaxOffset;
	else
		_pCD->m_TurnCorrectionTargetAngle = TargetAngle;

	// New test
	/*CVec3Dfp32 Look = CWObject_Character::GetLook(CVec3Dfp32::GetMatrixRow(_pObj->GetPositionMatrix(),0));

	WantedAngle = WantedCached + _pCD->m_Control_Look[2];//Look[2];
	WantedAngle = MACRO_ADJUSTEDANGLE(WantedAngle);
	WantedAngle += AngleAdjust(_pCD->m_TargetBodyAngleZ, WantedAngle);
	WantedAngle *= 100.0f;
	_pCD->m_LastBodyAngleZ = _pCD->m_TargetBodyAngleZ;
	AngleChange = _pCD->m_BodyAngleZChange * 100.0f;
	TargetAngle = _pCD->m_TargetBodyAngleZ * 100.0f;
	Moderatef(TargetAngle, WantedAngle, AngleChange, 
		PLAYER_TURN_CHANGESPEED);
	_pCD->m_TargetBodyAngleZ = TargetAngle / 100.0f;
	_pCD->m_BodyAngleZChange = AngleChange / 100.0f;*/
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Character
|__________________________________________________________________________________________________
\*************************************************************************************************/
CRPG_Object_Char *CWObject_Character::Char()
{
	return (CRPG_Object_Char *)(CRPG_Object *)m_spRPGObject;
}

// -------------------------------------------------------------------
CVec3Dfp32 CWObject_Character::GetLook(const CVec3Dfp32& _VFwd)
{
	MAUTOSTRIP(CWObject_Character_GetLook, CVec3Dfp32());
	CVec3Dfp32 Look2;
	Look2[0] = 0;

	//Handle straight up/down look (AO)
	if ((_VFwd[0] == 0.0f) && (_VFwd[1] == 0.0f))
		Look2[2] = 0.0f;
	else
		Look2[2] = 1.0f - CVec3Dfp32::AngleFromVector(_VFwd[0], _VFwd[1]);

	fp32 l = Length2(_VFwd[0], _VFwd[1]);
	Look2[1] = CVec3Dfp32::AngleFromVector(l, _VFwd[2]);
	if (Look2[1] > 0.5f) Look2[1] -= 1.0f;

	return Look2;
}

CVec3Dfp32 CWObject_Character::GetLook(const CMat4Dfp32& _Mat)
{
	MAUTOSTRIP(CWObject_Character_GetLook_2, CVec3Dfp32());
	CVec3Dfp32 VFwd = CVec3Dfp32::GetMatrixRow(_Mat, 0);
	return GetLook(VFwd);
}

// -------------------------------------------------------------------
void CWObject_Character::Char_SetGameTickDiff(int _Diff)
{
	MAUTOSTRIP(CWObject_Character_Char_SetGameTickDiff, MAUTOSTRIP_VOID);
	// we use bits 0..7 in m_Data[PLAYER_DATAINDEX_CONTROL]
//	if (!TDynamicCast<CWObject_Client>(_pObj))
//		ConOutL(CStrF("CWObject_Character::Char_SetGameTickDiff %i", _Diff));
	if(Char_GetGameTickDiff(this) != _Diff)
	{
		Data(PLAYER_DATAINDEX_CONTROL) &= 0xffffff00;
		Data(PLAYER_DATAINDEX_CONTROL) |= (_Diff) & 0xff;
	}
}

int CWObject_Character::Char_GetGameTickDiff(CWObject_CoreData *_pObj)
{
	MAUTOSTRIP(CWObject_Character_Char_GetGameTickDiff, 0);
	// we use bits 0..7 in m_Data[PLAYER_DATAINDEX_CONTROL]
	int8 Diff = (_pObj->m_Data[PLAYER_DATAINDEX_CONTROL] & 0xff);
	return Diff;
}

void CWObject_Character::Char_SetControlMode(CWObject_CoreData *_pObj, int _Mode)
{
	MAUTOSTRIP(CWObject_Character_Char_SetControlMode, MAUTOSTRIP_VOID);
	// we use bits 8..11 in m_Data[PLAYER_DATAINDEX_CONTROL]
	_pObj->m_Data[PLAYER_DATAINDEX_CONTROL] &= 0xfffff0ff;
	_pObj->m_Data[PLAYER_DATAINDEX_CONTROL] |= ((_Mode) & 0xf) << 8;
}

int CWObject_Character::Char_GetControlMode(const CWObject_CoreData *_pObj)
{
	MAUTOSTRIP(CWObject_Character_Char_GetControlMode, 0);
	// we use bits 8..11 in m_Data[PLAYER_DATAINDEX_CONTROL]
	return (_pObj->m_Data[PLAYER_DATAINDEX_CONTROL] & 0x00000f00) >> 8;
}

void CWObject_Character::Char_SetPhysType(CWObject_CoreData *_pObj, int _PhysType)
{
	MAUTOSTRIP(CWObject_Character_Char_SetPhysType, MAUTOSTRIP_VOID);
	// we use bits 12..15 in m_Data[PLAYER_DATAINDEX_CONTROL]
	_pObj->m_Data[PLAYER_DATAINDEX_CONTROL] &= 0xffff0fff;
	_pObj->m_Data[PLAYER_DATAINDEX_CONTROL] |= ((_PhysType) & 0xf) << 12;
}

int CWObject_Character::Char_GetPhysType(const CWObject_CoreData *_pObj)
{
	MAUTOSTRIP(CWObject_Character_Char_GetPhysType, 0);
	if( !_pObj ) return 0;
	// we use bits 12..15 in m_Data[PLAYER_DATAINDEX_CONTROL]
	return (_pObj->m_Data[PLAYER_DATAINDEX_CONTROL] & 0x0000f000) >> 12;
}


bool CWObject_Character::IsImmune()
{
	MAUTOSTRIP(CWObject_Character_IsImmune, 0);
	const CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	M_ASSERT(pCD, "No client data!");
	return (pCD->m_Phys_Flags & PLAYER_PHYSFLAGS_IMMUNE) != 0;
};

bool CWObject_Character::Char_IsClimbing()
{
	MAUTOSTRIP(CWObject_Character_Char_IsClimbing, false);
	return Char_GetControlMode(this) == PLAYER_CONTROLMODE_LADDER;
}

bool CWObject_Character::Char_IsSwimming()
{
	MAUTOSTRIP(CWObject_Character_Char_IsSwimming, false);
	const CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	return pCD->m_LiquidTickCountDown > 0;
}

bool CWObject_Character::Char_IsFighting(CWObject* pObj)
{
	return (Char_GetControlMode(pObj) == PLAYER_CONTROLMODE_FIGHTING);
}

bool CWObject_Character::Char_IsFighting(CWO_Character_ClientData* _pCD)
{
	if (_pCD)
		return (_pCD->m_iFightingCharacter != -1);

	return false;
}

void CWObject_Character::CharGetFightCharacter(CWObject_CoreData* _pObj, CWO_Character_ClientData* _pCD, CWorld_PhysState* _pWPhys, int32& _iBest, fp32 _SelectRadius)
{
	int8 Type;
	int32 iCloseSel = -1;
	bool bCurrentNoPrio = false;
	Char_FindStuff(_pWPhys, _pObj, _pCD, _iBest, iCloseSel, Type, bCurrentNoPrio, 0, _SelectRadius, OBJECT_FLAGS_CHARACTER);
}

CStr CWObject_Character::Dump(CMapData* _pWData, int _DumpFlags)
{
	MAUTOSTRIP(CWObject_Character_Dump, CStr());
	const CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(this);
	return CStrF("%s, Player %d, PhysType %d", CWObject_RPG::Dump(_pWData, _DumpFlags).Str(), (int)pCD->m_iPlayer, Char_GetPhysType(this) );
}

CVec3Dfp32 CWObject_Character::GetTargetFightPos(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, fp32 _Offset)
{
	// From an offset find a suitable target position
	if (!_pObj)
		return CVec3Dfp32(0);

	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);

	CVec3Dfp32 Position = _pObj->GetPosition();
	fp32 Offset = Clamp01(_Offset);

	int iFightingChar = pCD->m_iFightingCharacter;
	if (iFightingChar == -1)
		return Position;

	CVec3Dfp32 Direction = _pWPhysState->Object_GetPosition(iFightingChar) - Position;
	fp32 Length = Direction.Length();
	Direction = Direction / Length;
	fp32 DeltaLength = Length - PLAYER_FIGHTING_DISTANCE;

	return (Position + Direction * Offset * DeltaLength);
}

void CWObject_Character::UpdateGroundMaterial(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if(!pCD)
		return;

	CVec3Dfp32 Start = _pObj->GetPosition() + CVec3Dfp32(0, 0, 1);
	CVec3Dfp32 Stop = _pObj->GetPosition() - CVec3Dfp32(0, 0, 10);
	CCollisionInfo CInfo;
	CInfo.SetReturnValues(CXR_COLLISIONRETURNVALUE_SURFACE);
	if(_pWPhysState->Phys_IntersectLine(Start, Stop, 0, OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_PHYSMODEL, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS | XW_MEDIUM_CAMERASOLID, &CInfo, _pObj->m_iObject))
	{
		int iMaterial = CInfo.m_SurfaceType;
		if(iMaterial == 0 && CInfo.m_pSurface && CInfo.m_pSurface->GetBaseFrame())
			iMaterial = CInfo.m_pSurface->GetBaseFrame()->m_MaterialType;

		if(iMaterial != 0)
			pCD->m_GroundMaterial = iMaterial;
	}
}


void CCompletedMissions::Clear()
{
	m_lCompletedMissions.Clear();
}

void CCompletedMissions::AddMission(CStr _Str)
{
	_Str.MakeUpperCase();
	uint32 Hash = StringToHash(_Str);
	int iPos = GetInsertPosition(Hash);

	if (iPos != -1)
		m_lCompletedMissions.Insert(iPos,Hash);
}

bool CCompletedMissions::MissionExists(CStr _Str)
{
	_Str.MakeUpperCase();
	uint32 Hash = StringToHash(_Str);
	return GetInsertPosition(Hash) == -1;
}

void CCompletedMissions::Write(CCFile* _pFile) const
{
	TAP_RCD<const uint32> pMissions = m_lCompletedMissions;
	int16 Len = pMissions.Len();
	_pFile->WriteLE(Len);
	for (int16 i = 0; i < Len; i++)
	{
		_pFile->WriteLE(pMissions[i]);
	}
}

void CCompletedMissions::Read(CCFile* _pFile)
{
	int16 Len = 0;
	_pFile->ReadLE(Len);
	if (Len > 0)
	{
		m_lCompletedMissions.SetLen(Len);
		TAP_RCD<uint32> pMissions = m_lCompletedMissions;
		for (int16 i = 0; i < Len; i++)
		{
			_pFile->ReadLE(pMissions[i]);
		}
	}
}

int CCompletedMissions::GetInsertPosition(uint32 _Hash)
{
	// Find position to insert into
	TAP_RCD<const uint32> pMissions = m_lCompletedMissions;
	int32 iSearchStart = 0;
	int32 iSearchEnd = pMissions.Len() - 1;
	while (iSearchEnd >= iSearchStart)
	{
		int32 Middle = (iSearchEnd - iSearchStart);
		if (Middle > 1)
		{
			Middle = Middle / 2;
			uint32 CompHash = pMissions[iSearchStart + Middle];
			if (CompHash == _Hash)
			{
				// Already have this item in the list so just break
				return -1;
			}
			else if (CompHash < _Hash)
			{
				// Given hash is larger than middle
				iSearchStart += Middle + 1;
			}
			else
			{
				// Given identifier is less than middle
				iSearchEnd = iSearchStart + Middle - 1;
			}
		}
		else
		{
			// Only 1 - 2 left
			uint32 CompHash = pMissions[iSearchStart];
			if (iSearchStart == iSearchEnd)
			{
				if (CompHash == _Hash)
					return -1;
				
				// If we're adding check where, if larger add after
				if (CompHash < _Hash)
					iSearchStart++;
				break;
			}
			if (CompHash == _Hash)
			{
				return -1;
			}
			// Check if less than searchstart, then insert there
			if (CompHash > _Hash)
				break;
			if (pMissions[iSearchEnd] == _Hash)
				return -1;
			// Add after end
			iSearchStart = pMissions[iSearchEnd] < _Hash ? iSearchEnd + 1 : iSearchEnd;
			break;
		}
	}

	return iSearchStart;
}
