#include "PCH.h"

#include "WObj_Char.h"
#include "../../../Shared/MOS/Classes/GameWorld/WDataRes_Anim.h"
#include "../../../Shared/MOS/Classes/GameWorld/Client/WClient_Core.h"
#include "../../../Shared/MOS/Classes/GameWorld/WDataRes_FacialSetup.h"
#include "../../../Shared/MOS/XR/XRCustomModel.h"
#include "WRPG/WRPGChar.h"
#include "WObj_Misc/WObj_Model_Ladder.h"
#include "WObj_Char/WObj_CharDarkling_ClientData.h"
#include "WObj_Char/WObj_CharNPC_ClientData.h"
#include "WObj_Char/WObj_CharBlink.h"
#include "WObj_Misc/WObj_Turret.h"

#include "../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PhysCluster.h"

//#define CLOTH_DISABLE

#if 0
/*
	Diverse implementationer av run-time mapping av ben nummer

	Tanken är att man har ett CWO_Char_AnimBoneMap i clientdata (m_AnimBoneMap)
	och kör m_AnimBoneMap.SetSkeleton(pSkel) först i OnGetAnimState, eller
	andra ställen där man ev behöver den. Jag tror iofs man behöver ha 2st CWO_Char_AnimBoneMap 
	så att man cachar phys skeleton och render skeleton separat, annars kommer SetSkeleton leta
	ben nummer hela tiden. (den ska bara köras vid LOD byten)

	Förrutom pSkelInstance->ApplyScale prylen så verkar det inte behövas speciellt många mappings.
	Går det att lösa scalingen på något annat vis? Om hela skelettet behöver mappas så börjar
	cachingen ta en massa plats och hela ideen blir dålig.

	Torso->Head = 4 mappings
	Eyelid stuff = 4 mapppings
	Root = 0 mappings (Alltid = 1)
	Camera = 1 mappings
	mer?

	Istället för
    if(pSkelInstance->m_nBoneTransform > PLAYER_ROTTRACK_LEYELID)
		MULTMATMP(pSkelInstance->m_pBoneLocalPos[PLAYER_ROTTRACK_LEYELID], Mat);

	så skulle man skriva:
	int iBoneLEyeLid = m_AnimBoneMap.m_liBones[PLAYER_BONEMAP_LEYELID];
	if (iBoneLEyeLid)
		MULTMATMP(pSkelInstance->m_pBoneLocalPos[iBoneLEyeLid], Mat);

	-mh
*/
// -------------------------------------------------------------------
class CWO_Char_AnimBoneMap
{
public:
	void* m_pLastSkel;
	uint8 m_iBoneRLeg;
	uint8 m_iBoneRKnee;
	uint8 m_iBoneRFoot;
	uint8 m_iBoneLLeg;
	uint8 m_iBoneLKnee;
	uint8 m_iBoneLFoot;
	uint8 m_iBoneTorso;
	// etc..

	void SetSkeleton(CXR_Skeleton* _pSkel)
	{
		if ((void*)_pSkel == m_pLastSkel)
			return;

		m_pLastSkel = _pSkel;

		m_iBoneRLeg = _pSkel->FindBone(StringToHash("r_leg"));
		m_iBoneRKnee = _pSkel->FindBone(StringToHash("r_knee"));
		m_iBoneRFoot = _pSkel->FindBone(StringToHash("r_foot"));
		m_iBoneLLeg = _pSkel->FindBone(StringToHash("l_leg"));
		m_iBoneLKnee = _pSkel->FindBone(StringToHash("l_knee"));
		m_iBoneLFoot = _pSkel->FindBone(StringToHash("l_foot"));
		m_iBoneTorso = _pSkel->FindBone(StringToHash("torso"));
	}
};

// -------------------------------------------------------------------
//  v2.0

enum
{
	PLAYER_MAXBONEMAP		= 12,

	PLAYER_BONEMAP_RLEG		= 0,
	PLAYER_BONEMAP_RKNEE,
	PLAYER_BONEMAP_RFOOT,
	PLAYER_BONEMAP_LLEG,
	PLAYER_BONEMAP_LKNEE,
	PLAYER_BONEMAP_LFOOT,
	PLAYER_BONEMAP_TORSO,
};

class CWO_Char_AnimBoneMap			// 16 bytes för 12 bone mappings
{
public:
	void* m_pLastSkel;
	uint8 m_liBones[PLAYER_MAXBONEMAP];

	static bool ms_lBoneHashInit = false;
	static uint32 ms_lBoneHash[PLAYER_MAXBONEMAP];
	static char* ms_lBoneNames[] =
	{
		"r_leg",
		"r_knee",
		"r_foot",
		"l_leg",
		"l_knee",
		"l_foot",
		"torso",
		NULL
	};

	void InitBoneHash()
	{
		if (ms_lBoneHashInit)
			return;

		int iBone = 0;
		while(iBone < PLAYER_MAXBONEMAP && ms_lBoneNames[iBone])
		{
			ms_lBoneHash[iBone] = StringToHash(ms_lBoneNames[iBone]);
			iBone++;
		}

		ms_lBoneHashInit = true;
	}

	void SetSkeleton(CXR_Skeleton* _pSkel)
	{
		if ((void*)_pSkel == m_pLastSkel)
			return;

		m_pLastSkel = _pSkel;
		InitBoneHash();

		for(int i = 0; i < PLAYER_MAXBONEMAP; i++)
		{
			m_liBones = _pSkel->FindBone(ms_lBoneHash[i]);
		}
	}
};

// -------------------------------------------------------------------
// v3.0

// Spara pekare till bonehash array i CWO_Char_AnimBoneMap så att man kan olika mappings
// Är det bra till något?

class CWO_Char_AnimBoneMap				// 20 bytes för 12 bone mappings
{
public:
	void* m_pLastSkel;
	uint32* m_pBoneHash;
	uint8 m_liBones[PLAYER_MAXBONEMAP];

	CWO_Char_AnimBoneMap()
	{
		m_pLastSkel = NULL;
		m_pBoneHash = NULL;
		memset(&m_liBones, 0, sizeof(m_liBones));
	}

	void SetBoneMap(uint32* _pBoneHash)
	{
		m_pLastSkel = NULL;
		m_pBoneHash = _pBoneHash;
	}

	void SetSkeleton(CXR_Skeleton* _pSkel)
	{
		if ((void*)_pSkel == m_pLastSkel)
			return;

		m_pLastSkel = _pSkel;
		M_ASSERT(m_pBoneHash, "!");

		int iiBone = 0;
		while(m_pBoneHash[iiBone] && iiBone < PLAYER_MAXBONEMAP)
		{
			m_liBones[iiBone] = _pSkel->FindBone(m_pBoneHash[iiBone]);
		}
	}
};


int CXR_Skeleton::FindBone(uint32 _Hash)
{
	const uint32* pHash = m_lBoneNameHash.GetBasePtr();
	int nBoneNames = m_lBoneNameHash.Len();

	int Min = 0;
	int Max = nBoneNames-1;
	int CurrentIndex = Max >> 1;

	while (Min <= Max)
	{
		M_ASSERT(CurrentIndex >= 0 && CurrentIndex < nBoneNames, "");

		if (pHash[CurrentIndex] < _Hash)
		{
			if (Min == CurrentIndex)
				return 0;
			Min = CurrentIndex;
			CurrentIndex = ((Max + 1 - Min) >> 1) + Min;
		}
		else if (pHash[CurrentIndex] > _Hash)
		{
			if (Max == CurrentIndex)
				return 0;
			Max = CurrentIndex;
			CurrentIndex = ((Max - Min) >> 1) + Min;
		}
		else
		{
			return m_liBoneNames[CurrentIndex];
		}
	}
	return 0;
}

#endif




//#define SAMUEL_TESTAR
/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character animation
					
	Contents:		OnRefreshAnim
					OnGetAnimState
					Char_EvolveClientEffects
					etc..
\*____________________________________________________________________________________________*/
#define CLIENT_OFFSET 1

#define AIMING_TYPE_RIOTGUARD 5
#define AIMING_TYPE_HEAVYGUARD 6
#define AIMING_TYPE_MINIGUN 7
#define AIMING_TYPE_EXOSKELETON 8

void CWObject_Character::Char_SetAnimCode(CWObject_CoreData *_pObj, int _iCode)
{
	MAUTOSTRIP(CWObject_Character_Char_SetAnimCode, MAUTOSTRIP_VOID);
	//4 bits for AnimCode
//	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	_pObj->m_iAnim0 &= 0xfff0;
	_pObj->m_iAnim0 |= _iCode & 0xf;
}

void CWObject_Character::Char_SetAnimStance(CWObject_CoreData *_pObj, int _iStance)
{
	MAUTOSTRIP(CWObject_Character_Char_SetAnimStance, MAUTOSTRIP_VOID);
//	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	//8+4 bits for Stance-handle
	_pObj->m_iAnim0 &= 0x000f;
	_pObj->m_iAnim0 |= (_iStance & 0xfff) << 4;
}


int CWObject_Character::Char_GetAnimCode(const CWObject_CoreData *_pObj)
{
	MAUTOSTRIP(CWObject_Character_Char_GetAnimCode, 0);
	return _pObj->m_iAnim0 & 0xf;
}

int CWObject_Character::Char_GetAnimStance(const CWObject_CoreData *_pObj)
{
	MAUTOSTRIP(CWObject_Character_Char_GetAnimStance, 0);
	return (_pObj->m_iAnim0 & 0xfff0) >> 4;
}

int CWObject_Character::Char_GetAnimTorsoSequence(const CWObject_CoreData *_pObj)
{
	MAUTOSTRIP(CWObject_Character_Char_GetAnimTorsoSequence, 0);
	return _pObj->m_iAnim2;
}

static fp32 Sinc(fp32 _x)
{
	MAUTOSTRIP(Sinc, 0.0f);
//	return _x;

	return M_Sin((_x - 0.5f)*_PI)*0.5f + 0.5f;

/*	// Test code to remove prediction artifacts
	const fp32 Padding = 0.3;
	if (_x < Padding)
		return 0;
	else if (_x > 1.0f - Padding)
		return 1;
	else
		return (_x-Padding) / (1.0f - Padding * 2);*/
}

#define REVERSE_SCALE 0.5f
#define ANIM_CLIMBHEIGHT 27.0f
int CWObject_Character::Char_GetAnimLayers(CWObject_CoreData* _pObj, const CMat4Dfp32 &_Pos, CWorld_PhysState* _pWPhysState, fp32 _IPTime, CXR_AnimLayer* _pLayers, fp32 _Extrapolate)
{
	MAUTOSTRIP(CWObject_Character_Char_GetAnimLayers, 0);
	MSCOPESHORT(CWObject_Character::Char_GetAnimLayers);
	
	// AGMERGE Begin
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if (pCD == NULL)
		return 0;

	CMTime GameTime = CMTime::CreateFromTicks(pCD->m_GameTick, _pWPhysState->GetGameTickTime(), _IPTime) + CMTime::CreateFromSeconds(_Extrapolate);

	int nLayers = AG2I_MAXANIMLAYERS;
	CWAG2I* pAG2I = pCD->m_AnimGraph2.m_spAG2I;
	CWAG2I_Context AG2IContext(_pObj, _pWPhysState, GameTime);
	pAG2I->GetAnimLayers(&AG2IContext, _pLayers, nLayers, 0);

	//ConOut(CStrF("AG2: NUmanimLayers: %d", nLayers2));

    // Posteffect, adjust time of synced animation layers to player time
	if ((pCD->m_iPlayer == -1) && (nLayers > 0) && _pWPhysState->IsClient())
	{
		// Need a world client for this one
		CWorld_Client* pWClient = safe_cast<CWorld_Client>(_pWPhysState);

		if (pWClient)
		{
			CMTime Diff;
			bool bGotDiff = false;
			for (int32 i = 0; i < nLayers; i++)
			{
				if (_pLayers[i].m_Flags & CXR_ANIMLAYER_TAGSYNC)
				{
					if (!bGotDiff)
					{
						Diff.Reset();
						bool bOtherPredict = false;
						CWObject_Client* pOther = pWClient->Object_Get(pCD->m_iFightingCharacter);
						pOther = Player_GetLastValidPrediction(pOther);
						CWO_Character_ClientData *pCDOther = (pOther ? CWObject_Character::GetClientData(pOther) : NULL);
						if (pCDOther)
						{
							CMTime GameTimeSync = CMTime::CreateFromTicks(pCDOther->m_GameTick, _pWPhysState->GetGameTickTime(), pCDOther->m_PredictFrameFrac) + CMTime::CreateFromSeconds(_Extrapolate);
							Diff = GameTimeSync - GameTime;
							bGotDiff = true;
						}
					}
					_pLayers[i].m_Time += Diff.GetTime() * _pLayers[i].m_Blend;
					M_ASSERT(0x7fc00000 != (uint32&)_pLayers[i].m_Time, "!");
					//ConOutL(CStrF("NP: Diff: %f BlendIn: %f BlendOut: %f Lerp: %f", Diff, _pLayers[i].m_BlendIn, _pLayers[i].m_BlendOut, _pLayers[i].m_Blend));
				}
			}
		}
	}

	return nLayers;
}
/*
static fp32 WrapBodyAngle(fp32 _Angle)
{
	MAUTOSTRIP(WrapBodyAngle, 0.0f);
	_Angle = FRAC(_Angle);
	if (_Angle > 0.5f)
		_Angle -= 1.0f;

	return _Angle;
}
*/
#define PrintBodyAngle(_Mess, _pCD)	if(_pCD->m_bIsServerRefresh)							 \
										ConOut(CStr("SERVER: ")+CStr(_Mess)+CStrF(": %f", _pCD->m_Anim_BodyAngleZ)); \
									else													 \
										ConOut(CStr("CLIENT: ")+CStr(_Mess)+CStrF(": %f", _pCD->m_Anim_BodyAngleZ)); 

#if 0
static void CalculateBodyAngles(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CWO_Character_ClientData* _pCD, int _DebugCalledFrom)
{
	MAUTOSTRIP(CalculateBodyAngles, MAUTOSTRIP_VOID);
	// Save the previous set of bodyangles.
	_pCD->m_Anim_LastBodyAngleX = WrapBodyAngle(_pCD->m_Anim_BodyAngleX);
	_pCD->m_Anim_LastBodyAngleY = WrapBodyAngle(_pCD->m_Anim_BodyAngleY);
	_pCD->m_Anim_LastBodyAngleZ = WrapBodyAngle(_pCD->m_Anim_BodyAngleZ);

	CVec3Dfp32 Look = _pCD->m_Control_Look;
	Look[2] = WrapBodyAngle(FRAC(Look[2]));
	
//	int bCrouch = CWObject_Character::Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH;

	//JK-FIX: Change this! Don't detect with a dynamic cast!
	bool bIsServerSide = TDynamicCast<CWObject_Character>(_pObj) != NULL;

	CVec3Dfp32 Move;
	if (bIsServerSide || _pObj->m_ClientFlags & CWO_CLIENTFLAGS_HIGHPRECISION)
		Move = _pObj->GetMoveVelocity();
	else
		_pObj->GetPosition().Sub(_pObj->GetLastPosition(), Move);

	if (_pCD->m_iPlayer == -1)
	{
		_pCD->m_Anim_ModeratedMove *= 10;
		_pCD->m_Anim_ModeratedMovePrim *= 10;
		Move *= 10;
		ModerateVec3f(_pCD->m_Anim_ModeratedMove, Move, _pCD->m_Anim_ModeratedMovePrim, 200);
		_pCD->m_Anim_ModeratedMove *= 0.1f;
		_pCD->m_Anim_ModeratedMovePrim *= 0.1f;
//		Move *= 0.1f;

		Move = _pCD->m_Anim_ModeratedMove;
	}

	fp32 MoveDir = Length2(Move.k[0], Move.k[1]);
//	fp32 SafeZ = Look[2];
	fp32 BodyTurningSpeed = 0.025f;//189f;
	
	int ControlMode = CWObject_Character::Char_GetControlMode(_pObj);
	int AnimCode = CWObject_Character::Char_GetAnimCode(_pObj);
	bool bIsMoving = (_pObj->GetMoveVelocity().LengthSqr() > Sqr(0.05f));
	bool bIsOnLadder = (ControlMode == PLAYER_CONTROLMODE_LADDER);
	bool bIsFlyer = ((_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_AIRCRAFT) && (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOGRAVITY));
	bool bIsFighting = (ControlMode == PLAYER_CONTROLMODE_FIGHTING);
	bool bIsOnHangRail = (ControlMode == PLAYER_CONTROLMODE_HANGRAIL);
	bool bIsOnLedge = (ControlMode == PLAYER_CONTROLMODE_LEDGE);
	//bool bIsAnimControlled = (ControlMode == PLAYER_CONTROLMODE_ANIMCONTROLLED);
	
	fp32 Anim_BodyAngleZ = _pCD->m_Anim_BodyAngleZ;
	fp32 Anim_BodyAngleZTarget = _pCD->m_Anim_BodyAngleZTarget;
//	fp32 BodyTurningSpeed = 0.03f;
	// If the character isn't moving and isn't on a ladder, swimming nor a flyer. A turn around animation should be played when turning around.
	if(!bIsMoving && !bIsOnLadder && !bIsFlyer && !bIsSwimming && 
		!bIsFighting && !bIsOnHangRail && !bIsOnLedge /*&& !bIsAnimControlled*/)
	{		
		Anim_BodyAngleZ = WrapBodyAngle(Anim_BodyAngleZ);
		fp32 dZ = FRAC(2.0f + Anim_BodyAngleZ - Look[2]);
		fp32 dT = FRAC(2.0f + Anim_BodyAngleZ - Anim_BodyAngleZTarget);

		// The character is currently turning right. Continue!
		if(_pCD->m_Anim_TurningState == PLAYER_ANIM_TURNINGRIGHT)
		{
			// Are we done turning right? 
			if(Abs(Anim_BodyAngleZ - Anim_BodyAngleZTarget)< 0.01f ||
			   Abs(Anim_BodyAngleZ - Look[2])< BodyTurningSpeed)
			{
//				if(IsTurning(_pCD))
//					_pCD->m_Anim_BodySeq = 0;
//				PrintBodyAngle("Stopped turning right", _pCD);				
				_pCD->m_Anim_TurningState = PLAYER_ANIM_NOTTURNING;
				_pCD->m_StraightenUpTick = 0;
				//_pCD->m_ExtraFlags = _pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_NOTURNANIMATION;
			}
			else
			{
				// Oh shit! We have started to turn in the other direction. Let's switch direction!
				if (dZ < 0.5f)
				{
//					PrintBodyAngle("Switching right to left", _pCD);				
					Anim_BodyAngleZTarget = Anim_BodyAngleZ - 0.25f;
					_pCD->m_Anim_TurningState = PLAYER_ANIM_TURNINGLEFT;
//					CXR_Anim_SequenceData *pSeq = CWObject_Character::GetAnimSequence(_pObj, _pWPhysState, PLAYER_ANIM_SEQUENCE_TURNLEFT);
//					if(!bCrouch && pSeq)
//						CWObject_Character::Char_SetAnimSequence(_pObj, _pWPhysState, PLAYER_ANIM_SEQUENCE_TURNLEFT);
				}
				else
				{			
///					if(!bCrouch && !IsTurning(_pCD) && !(_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_NOTURNANIMATION) && dZ < 0.90f)
///						CWObject_Character::Char_SetAnimSequence(_pObj, _pWPhysState, PLAYER_ANIM_SEQUENCE_TURNRIGHT);			
					// We have turned really fast. Got to jump forward to not get outturned!
					if(dT < 0.60f)
					{
//						PrintBodyAngle("Turning right too fast", _pCD);				
						Anim_BodyAngleZ += Max(0.60f - dT, BodyTurningSpeed);									
					}
					// Keep on catching up with the look.
					else
					{
//						PrintBodyAngle("Continue turning right", _pCD);				
						Anim_BodyAngleZ += Min(BodyTurningSpeed, (fp32)Abs(Anim_BodyAngleZTarget - Anim_BodyAngleZ));
					}
				}
			}			
		}
		// The character is currently turning left. Continue!
		else if(_pCD->m_Anim_TurningState == PLAYER_ANIM_TURNINGLEFT)
		{
			// Are we done turning right? 
			if(Abs(Anim_BodyAngleZ - Anim_BodyAngleZTarget)< 0.01f ||
			   Abs(Anim_BodyAngleZ - Look[2])< BodyTurningSpeed)
			{
//				if(IsTurning(_pCD))
//					_pCD->m_Anim_BodySeq = 0;
//				PrintBodyAngle("Stopped turning left", _pCD);				
				_pCD->m_Anim_TurningState = PLAYER_ANIM_NOTTURNING;
				_pCD->m_StraightenUpTick = 0;
				//_pCD->m_ExtraFlags = _pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_NOTURNANIMATION;
			}
			else
			{
				// Oh shit! We have started to turn in the other direction. Let's switch direction!
				if (dZ > 0.5f)
				{
//					PrintBodyAngle("Switching left to right", _pCD);				
					Anim_BodyAngleZTarget = Anim_BodyAngleZ + 0.25f;
					_pCD->m_Anim_TurningState = PLAYER_ANIM_TURNINGRIGHT;
//					CXR_Anim_SequenceData *pSeq = CWObject_Character::GetAnimSequence(_pObj, _pWPhysState, PLAYER_ANIM_SEQUENCE_TURNRIGHT);
//					if(!bCrouch && pSeq)
//						CWObject_Character::Char_SetAnimSequence(_pObj, _pWPhysState, PLAYER_ANIM_SEQUENCE_TURNRIGHT);	
				}
				else
				{
///					if(!bCrouch && !IsTurning(_pCD) && !(_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_NOTURNANIMATION) && dZ > 0.10f)
///						CWObject_Character::Char_SetAnimSequence(_pObj, _pWPhysState, PLAYER_ANIM_SEQUENCE_TURNLEFT);			

					// We have turned really fast. Got to jump forward to not get outturned!
					if(dT > 0.40f)
					{
//						PrintBodyAngle("Turning left too fast", _pCD);				
						Anim_BodyAngleZ -= Max(dT - 0.40f, BodyTurningSpeed);									
					}
					// Keep on catching up with the look.
					else
					{
//						PrintBodyAngle("Continue turning left", _pCD);				
						Anim_BodyAngleZ -= Min(BodyTurningSpeed, (fp32)Abs(Anim_BodyAngleZ - Anim_BodyAngleZTarget));				
					}
				}	
			}
		}
		// We are not turning. Should we turn right?
		else if (dZ > 0.5f && (dZ < 0.75f || (_pCD->m_StraightenUpTick == _pCD->m_GameTick)))
		{
			//Start turning right
//			PrintBodyAngle("Start turning right", _pCD);				
			_pCD->m_Anim_TurningState = PLAYER_ANIM_TURNINGRIGHT;
			Anim_BodyAngleZTarget = Anim_BodyAngleZ + 0.25f;
/*			if(_pCD->m_Anim_BodySeq == 0)
			{
				// Only play an animation if we are not crouching and a turn animation exist!
///				if(!bCrouch && !(_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_NOTURNANIMATION))
///					CWObject_Character::Char_SetAnimSequence(_pObj, _pWPhysState, PLAYER_ANIM_SEQUENCE_TURNRIGHT);	
			}*/
			Anim_BodyAngleZ += Min(BodyTurningSpeed, (fp32)Abs(Anim_BodyAngleZTarget - Anim_BodyAngleZ));				
		}
		// We are not turning. Should we turn left?
		else if((dZ > 0.25f || (_pCD->m_StraightenUpTick == _pCD->m_GameTick)) && dZ < 0.5f)
		{
			//Start turning left
//			PrintBodyAngle("Start turning left", _pCD);				
			_pCD->m_Anim_TurningState = PLAYER_ANIM_TURNINGLEFT;
			Anim_BodyAngleZTarget = Anim_BodyAngleZ - 0.25f;
/*			if(_pCD->m_Anim_BodySeq == 0)
			{
				// Only play an animation if we are not crouching and a turn animation exist!
///				if(!bCrouch && !(_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_NOTURNANIMATION))
///					CWObject_Character::Char_SetAnimSequence(_pObj, _pWPhysState, PLAYER_ANIM_SEQUENCE_TURNLEFT);			
			}*/
			Anim_BodyAngleZ -= Min(BodyTurningSpeed, (fp32)Abs(Anim_BodyAngleZ - Anim_BodyAngleZTarget));
		}
	}
	// We are swmming. Just straight forward catching up with the look.
	else if(bIsSwimming)
	{		
		fp32 dZ = FRAC(2.0f + Anim_BodyAngleZ - Look[2]);
		if (dZ > 0.5f && dZ < 0.85f)
		{			
			Anim_BodyAngleZ += Min(0.10f, 0.85f - dZ);
		}
		else if (dZ > 0.15f && dZ < 0.5f)
		{
			Anim_BodyAngleZ -= Min(0.10f, dZ-0.15f);
		}
	}
	else if (bIsFighting /*|| bIsAnimControlled*/)
	{
		// Make the characters face each other (exactly)
		Anim_BodyAngleZ = Look[2];
	}
	// We are either on a ladder or a flyer. We'll be treated handled on.
	else
	{
/*		if(IsTurning(_pCD))
			_pCD->m_Anim_BodySeq = 0;*/
		_pCD->m_Anim_TurningState = PLAYER_ANIM_NOTTURNING;
		_pCD->m_StraightenUpTick = 0;
		//_pCD->m_ExtraFlags = _pCD->m_ExtraFlags & ~PLAYER_EXTRAFLAGS_NOTURNANIMATION;
	}

	// We're a flying fucker. Let's do some nice flying stuff. (is set by setting PHYS_FLYER 1 in the registry for the character)
	if(bIsFlyer)
	{		
		CMat4Dfp32 LookMat;
		_pCD->m_Anim_LastLook.CreateMatrix(LookMat);
		CVec3Dfp32 LastLook = CVec3Dfp32::GetMatrixRow(LookMat, 0);

		// Calculate banking for the critter.
		Look[2] = WrapBodyAngle(FRAC(Look[2]));

		fp32 LastLookX = WrapBodyAngle(-CVec3Dfp32::AngleFromVector(LastLook[0],LastLook[1]));
		fp32 dZ = FRAC(2.0f + Look[2] - LastLookX);
//		fp32 cp = dZ;
		fp32 WantedBanking;
		if (dZ > 0.5f)
		{			
			WantedBanking = ((1.0f-dZ)*10.0f)>0.24f? 0.24f:((1.0f-dZ)*10.0f);
		}
		else if (dZ < 0.5f)
		{
			WantedBanking = (-dZ*10.0f)<-0.24f?-0.24f:(-dZ*10.0f);
		}

		fp32 dB = FRAC(2.0f + _pCD->m_Anim_BodyAngleX - WantedBanking);
		if (dB > 0.5f)
		{			
			_pCD->m_Anim_BodyAngleX = _pCD->m_Anim_BodyAngleX + Min(0.01f, (float)Abs(WantedBanking-_pCD->m_Anim_BodyAngleX));
		}
		else if (dB < 0.5f)
		{
			_pCD->m_Anim_BodyAngleX = _pCD->m_Anim_BodyAngleX - Min(0.01f, (float)Abs(_pCD->m_Anim_BodyAngleX-WantedBanking));
		}

		// Calculate pitch for the critter.
		Look[1] = WrapBodyAngle(FRAC(Look[1]));
		
		fp32 dP = FRAC(2.0f + _pCD->m_Anim_BodyAngleY - Look[1]);
		if (dP > 0.5f && dP < 0.95f)
		{			
			_pCD->m_Anim_BodyAngleY = _pCD->m_Anim_BodyAngleY + Min(0.05f, 0.95f- dP);
		}
		else if (dP > 0.05f && dP < 0.5f)
		{
			_pCD->m_Anim_BodyAngleY = _pCD->m_Anim_BodyAngleY - Min(0.05f, dP - 0.05f);
		}

		// Calculate the heading of the character. We can either use the look of the character or the movement. This is specified by setting
		// the extre flag PLAYER_EXTRAFLAGS_AIRCRAFT_USELOOK in the characters ClientData. Default is to use movement while the character is moving.
		CVec3Dfp32 Hv;
		Hv = _pObj->GetMoveVelocity();
		if(Hv.LengthSqr() < Sqr(0.5f))
			_pObj->GetPosition().Sub(_pObj->GetLastPosition(), Hv);
	
		fp32 MDir = WrapBodyAngle(-CVec3Dfp32::AngleFromVector(Hv.k[0], Hv.k[1]));
	
		if((_pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_AIRCRAFT_USELOOK) || Hv.LengthSqr() < Sqr(0.5f))
		{
			MDir = Look[2];
			dZ = FRAC(2.0f + Anim_BodyAngleZ - MDir);
			if (dZ > 0.5f)
			{			
				Anim_BodyAngleZ += Min(0.10f, 1.0f-dZ);
			}
			else if (dZ < 0.5f)
			{
				Anim_BodyAngleZ -= Min(0.10f, dZ);
			}
		}
		else
		{
			fp32 dMZ = _pCD->m_MoveDir / 256.0f;
			if (dMZ > 0.26f && dMZ < 0.74f)
				MDir = WrapBodyAngle(FRAC(MDir + 0.5f));
			
			dMZ = FRAC(2.0f + MDir - Anim_BodyAngleZ);
			if(dMZ > 0.25f && dMZ < 0.75f)
			{
				fp32 dLZ = FRAC(2.0f + Look[2] - Anim_BodyAngleZ);
				if(dLZ >= 0.5f)
					Anim_BodyAngleZ -= Min(1.0f - dMZ, 0.10f);
				else
					Anim_BodyAngleZ += Min(dMZ, 0.10f);
			}
			else if (dMZ > 0.5f && dMZ < 1.0f)
				Anim_BodyAngleZ -= Min(1.0f - dMZ, 0.10f);
			else if (dMZ > 0.0f && dMZ < 0.5f)
				Anim_BodyAngleZ += Min(dMZ, 0.10f);
			
		}
	}
	// Seems that we are an ordinary character. So if we are not turning. Start doing some movement code.
	else if (/*!IsTurning(_pCD) &&*/ !bIsFighting /*&& !bIsAnimControlled*/)
	{
		// Are we moving fast enough to handle the direction? (Might be problems otherwise when colliding against walls)
		if(MoveDir > 1.0f)
		{
			Look[2] = WrapBodyAngle(FRAC(Look[2]));
			fp32 MoveAngleZ =  WrapBodyAngle(-(CVec3Dfp32::AngleFromVector(Move.k[0], Move.k[1])));
			
			// Use the specified movement by the user to calculate whether to move backwards or not.
			fp32 dMZ = _pCD->m_MoveDir *(1.0f / 256.0f);
			fp32 dMZ2 = FRAC(2.0f + MoveAngleZ - Look[2]);
			if(_pCD->m_Phys_nInAirFrames >2)
				dMZ = dMZ2;
			if (dMZ > 0.26f && dMZ < 0.74f)
			{
				MoveAngleZ = WrapBodyAngle(FRAC(MoveAngleZ + 0.5f));
			}
			dMZ = FRAC(2.0f + MoveAngleZ - Anim_BodyAngleZ);
			if(dMZ > 0.25f && dMZ < 0.75f)
			{
				fp32 dLZ = FRAC(2.0f + Look[2] - Anim_BodyAngleZ);
				if(dLZ >= 0.5f)
					Anim_BodyAngleZ -= Min(1.0f - dMZ, 0.10f);
				else
					Anim_BodyAngleZ += Min(dMZ, 0.10f);
			}
			else if (dMZ > 0.5f && dMZ < 1.0f)
			{
				Anim_BodyAngleZ -= Min(1.0f - dMZ, 0.10f);
			}
			else if (dMZ > 0.0f && dMZ < 0.5f)
			{
				Anim_BodyAngleZ += Min(dMZ, 0.10f);
			}
		}
		else if(bIsMoving && !bIsFighting /*&& !bIsAnimControlled*/)
		{
			Look[2] = WrapBodyAngle(FRAC(Look[2]));
			
			fp32 dZ = FRAC(2.0f + Anim_BodyAngleZ - Look[2]);
			if (dZ > 0.5f && dZ < 0.85f)
			{			
				Anim_BodyAngleZ += Min(0.10f, 0.85f - dZ);
			}
			else if (dZ > 0.15f && dZ < 0.5f)
			{
				Anim_BodyAngleZ -= Min(0.10f, dZ-0.15f);
			}
		}
	}

	_pCD->m_Anim_BodyAngleX = WrapBodyAngle(_pCD->m_Anim_BodyAngleX);
	_pCD->m_Anim_BodyAngleY = WrapBodyAngle(_pCD->m_Anim_BodyAngleY);
	_pCD->m_Anim_BodyAngleZ = WrapBodyAngle(Anim_BodyAngleZ);
	_pCD->m_Anim_BodyAngleZTarget = WrapBodyAngle(Anim_BodyAngleZTarget);
}
#endif

void CWObject_Character::Client_Anim(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CWO_Character_ClientData* _pCD, int _DebugCalledFrom)
{
	MAUTOSTRIP(CWObject_Character_Client_Anim, MAUTOSTRIP_VOID);
	MSCOPE(CWObject_Character::Client_Anim, CHARACTER);
	int AnimCode = Char_GetAnimCode(_pObj);
	{
		// Find out which IK mode to use
		int OldCur = _pCD->m_PostAnimSystem.m_Anim_iCurIKLookMode;
		int Anim_iCurIKLookMode = OldCur;
		//if(_pCD->m_iPlayer != -1)
			Anim_iCurIKLookMode = AIMING_NORMAL;
		/*else
			Anim_iCurIKLookMode = AIMING_HEAD;*/

		if(_pCD->m_Item0_Flags & RPG_ITEM_FLAGS_AIMING)
		{
			if(_pCD->m_iPlayer != -1)
				Anim_iCurIKLookMode = AIMING_BODY;
			if (_pCD->m_Anim_IdleStance  > 0)
				Anim_iCurIKLookMode = AIMING_BODY;
		}

		int32 iAimingTypeOverride = _pCD->m_AnimGraph2.GetForcedAimingType();
		if (iAimingTypeOverride != -1)
			Anim_iCurIKLookMode = iAimingTypeOverride;

/*		if(Anim_iCurIKLookMode == AIMING_BODY && Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH)
			Anim_iCurIKLookMode = AIMING_CROUCHBODY;*/

		if(OldCur != Anim_iCurIKLookMode && _pCD->m_PostAnimSystem.m_Anim_IKLookBlend == 0)
		{
			_pCD->m_PostAnimSystem.m_Anim_iCurIKLookMode	= Anim_iCurIKLookMode;
			_pCD->m_PostAnimSystem.m_Anim_IKLookBlend = 1.0f;
			_pCD->m_PostAnimSystem.m_Anim_iLastIKLookMode = OldCur;
		}
		else
			_pCD->m_PostAnimSystem.m_Anim_IKLookBlend = Max(_pCD->m_PostAnimSystem.m_Anim_IKLookBlend - PLAYER_ANIMBLENDSPEEDVISUAL, 0.0f);
	}
}

/*
#define MULTMAT( M, M2 )	\
{ CMat4Dfp32 Tmp; M2.Multiply(M, Tmp); M = Tmp; }

#define MULTMATMP( M, M2 )	\
{ CMat43fp32 Tmp; M2.Multiply(M, Tmp); M = Tmp; }

#define APPLYQUAT(Q1, Q2, B, Blend) \
{\
	if(Blend > 0)\
	{\
		if(!B)\
			Q2.Unit();\
		Q1.Interpolate(Q2, Q2, Blend);\
	}\
	else\
		Q2 = Q1;\
	B = true;\
}
*/

void CWObject_Character::RotateBoneAbsolute(const CQuatfp32 &_Q, CXR_SkeletonInstance *_pSkelInstance, int _iRotTrack, int _iParent)
{
	CMat4Dfp32 IParent;
	CMat4Dfp32 Rot, dParent, IdParent, M0, M1, M2;
	_pSkelInstance->GetBoneTransform(_iParent ).InverseOrthogonal(IParent);
	_pSkelInstance->GetBoneTransform(0).Multiply(IParent, dParent);
	dParent.InverseOrthogonal(IdParent);

	CMat4Dfp32 Local = _pSkelInstance->GetBoneLocalPos(_iRotTrack);
	
	// Rotate Quat in Parent space and apply to Local bone
	_Q.CreateMatrix(Rot);
	IdParent.Multiply3x3(Rot, M0);
	Local.Multiply3x3(M0, M1);
	M1.Multiply3x3(dParent, M2);

	CVec3Dfp32::GetRow(M2, 0).SetRow(Local, 0);
	CVec3Dfp32::GetRow(M2, 1).SetRow(Local, 1);
	CVec3Dfp32::GetRow(M2, 2).SetRow(Local, 2);

	_pSkelInstance->SetBoneLocalPos( _iRotTrack, &Local );
}

void CWObject_Character::EvalSkelAndApplyBoneScale(fp32 _Scale, CXR_SkeletonInstance* _pSkelInstance, CXR_Skeleton* _pSkel)
{
	if(_Scale != 1.0f)
	{
		// "Global" scale should only be set on root node
		CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(_pSkelInstance->m_pBoneTransform[0],3);
		CMat43fp32 GlobalScale;
		CMat4Dfp32 Temp;
		GlobalScale.Unit();
		GlobalScale.k[0][0] *= _Scale;
		GlobalScale.k[1][1] *= _Scale;
		GlobalScale.k[2][2] *= _Scale;
		Pos = Pos - Pos * _Scale;
		Pos.SetMatrixRow(GlobalScale,3);
		_pSkelInstance->m_pBoneTransform[0].Multiply(GlobalScale,Temp);
		_pSkel->EvalNode(PLAYER_ROTTRACK_ROOT, &Temp, _pSkelInstance);
	}
	else
		_pSkel->EvalNode(PLAYER_ROTTRACK_ROOT, &_pSkelInstance->m_pBoneTransform[0], _pSkelInstance);
}

class CAnimGraphScopeLock
{
	void *m_pLock;
	NThread::FLock *m_pUnlockFunc;
public:

	template <typename t_Lock>
	class TLocker
	{
	public:
		static void Locker(void *_pLock)
		{
			((t_Lock *)_pLock)->Unlock();
		}
	};

	CAnimGraphScopeLock()
	{
		m_pLock = 0;
		m_pUnlockFunc = 0;
	}

	template <typename t_Lock>
	void Create(t_Lock &_Lock)
	{
		_Lock.Lock();
		m_pLock = &_Lock;
		m_pUnlockFunc = TLocker<t_Lock>::Locker;
	}

	void Destroy()
	{
		if(m_pLock)
		{
			m_pUnlockFunc(m_pLock);
			m_pLock = NULL;
		}
	}

	~CAnimGraphScopeLock()
	{
		if(m_pLock)
			m_pUnlockFunc(m_pLock);
	}
};

extern fp32 AngleAdjust(fp32 _AngleA, fp32 _AngleB);
bool CWObject_Character::OnGetAnimState(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CXR_Skeleton* _pSkel, int _iCacheSlot, const CMat4Dfp32& _Pos, fp32 _IPTime, CXR_AnimState& _Anim, CVec3Dfp32 *_pRetPos, bool _bIgnoreCache, fp32 _ClothScaleOverride, CXR_Anim_TrackMask *_pTrackMask, fp32 _OverrideRagdollHeight)
{
	MAUTOSTRIP(CWObject_Character_OnGetAnimState, false);
	MSCOPE(CWObject_Character::OnGetAnimState, CHARACTER);
	
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if (!pCD) return false;
	CWObject_CoreData* pObj = _pObj;

	CWO_CharDarkling_ClientData* pDarklingCD = pCD->IsDarkling();

	bool bIsPlayer = (pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER) != 0;
	bool bIsDarkling = (pDarklingCD != NULL);

	CMat4Dfp32 MatBody, MatLook;
	int Char_ControlMode = Char_GetControlMode(_pObj);
	
	// Get turn correction angle
	fp32 BodyAngle = 0.0f;
	{
		fp32 LastAngle = pCD->m_TurnCorrectionLastAngle;
		LastAngle += AngleAdjust(LastAngle, pCD->m_TurnCorrectionTargetAngle);
		if (pCD->m_iPlayer == -1 && M_Fabs(LastAngle - pCD->m_TurnCorrectionTargetAngle) > 0.001f)
			BodyAngle = LERP(LastAngle, pCD->m_TurnCorrectionTargetAngle, _IPTime);
		else
			BodyAngle = pCD->m_TurnCorrectionTargetAngle;
	}

	//
	// First, get MatLook & MatBody (either from _Pos or by other means)
	//
	MatLook = _Pos;
	
	if (pCD->m_RenderAttached != 0)
	{
		CWObject_CoreData* pObj = _pWPhysState->Object_GetCD(pCD->m_RenderAttached);
		if (pObj)
		{
			CWObject_Message Msg(OBJMSG_HOOK_GETCURRENTMATRIX, (aint)&MatLook, pCD->m_iAttachJoint);
			if (!_pWPhysState->Phys_Message_SendToObject(Msg, pCD->m_RenderAttached))
				Interpolate2(pObj->GetLastPositionMatrix(), pObj->GetPositionMatrix(), MatLook, _IPTime);
		}
		//_pWPhysState->Debug_RenderMatrix(MatLook, 0.05f, false);
	}
	else if (_pObj->GetParent() && _pObj->m_iParentAttach >= 0)
	{
		int iObjParent = _pObj->GetParent();
		CWObject_Message Msg(OBJMSG_HOOK_GETCURRENTMATRIX, aint(&MatLook), _pObj->m_iParentAttach);
		if (_pWPhysState->Phys_Message_SendToObject(Msg, iObjParent))
			M_VMatMul(_pObj->GetLocalPositionMatrix(), MatLook, MatLook);
	}
	MatBody = MatLook;

	//
	// Then, adjust MatLook & MatBody according to flags etc...
	//
	if ((pCD->m_RenderAttached != 0) && (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_FORCEATTACHMATRIX))
	{
		// Angle error version
		// Modify (will not effect camera hopefully)
		// Get look from opponent and diff with our own...
		if (pCD->m_iMountedObject != 0 && pCD->m_iPlayer == -1)
		{
			if (!(pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_TURRET))
			{
				// Modify (will not effect camera hopefully)
				// Get look from opponent and diff with our own...
				CVec3Dfp32 Body = GetLook(CVec3Dfp32::GetMatrixRow(MatLook,0));
				CWObject_CoreData* pObj = _pWPhysState->Object_GetCD(pCD->m_iMountedObject);
				CWO_Character_ClientData* pCDOther = (pObj && pObj->GetPhysState().m_ObjectFlags & (OBJECT_FLAGS_CHARACTER|OBJECT_FLAGS_PLAYER)) ? GetClientData(pObj) : NULL;
				if (pCDOther)
				{
					if (pCDOther->m_CameraUtil.IsActive(CAMERAUTIL_MODE_MOUNTEDCAM))
					{
						// meh...
						Body += pCDOther->m_CameraUtil.GetMountedAngles(pCDOther->m_PredictFrameFrac);
					}
					else
					{
						Body[2] = pCDOther->m_Control_Look_Wanted[2];
						Body[1] = pCDOther->m_Control_Look_Wanted[1];
					}
				}
				Body.CreateMatrixFromAngles(0, MatLook);
					CVec3Dfp32::GetMatrixRow(MatBody,3).SetMatrixRow(MatLook,3);
			}
		}
		else if (pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_TURRET)
		{
			if (_pWPhysState->Phys_Message_SendToObject(CWObject_Message(CWObject_Turret::OBJMSG_TURRET_GETTURRETFLAGS),pCD->m_iMountedObject) & CWObject_Turret::TURRET_FLAG_NOCAMERA)
			{
				CVec3Dfp32 Body = GetLook(CVec3Dfp32::GetMatrixRow(MatLook,0));
				Body[1] = pCD->m_ActionCutSceneCameraOffsetY;
				Body.CreateMatrixFromAngles(0, MatLook);
				CVec3Dfp32::GetMatrixRow(MatBody,3).SetMatrixRow(MatLook,3);
			}
		}
	}
	else if (bIsDarkling)
	{
		CVec3Dfp32 UpVec = pDarklingCD->m_UpVector;
		if (pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_EXACTDESTPOSITION)
			UpVec = MatLook.GetRow(2);
		MatLook.GetRow(3) -= UpVec * 16.0f; // Move base from center to base position

		// Fixate body to ground
		MatBody.GetRow(2) = UpVec;
		MatBody.RecreateMatrix(2, 0);

		// Apply turn correction
		CMat4Dfp32 TurnCorrection, Tmp;
		CQuatfp32(UpVec, BodyAngle).SetMatrix(TurnCorrection);			// Rotate around gravity Z-axis
		Tmp = MatBody;
		Tmp.Multiply(TurnCorrection, MatBody);
		MatBody.GetRow(3) = MatLook.GetRow(3);
	}
	else if (Char_ControlMode == PLAYER_CONTROLMODE_ANIMATION)
	{
		//ConOut("AnimControlMode");
		if(pCD->m_RenderAttached == 0)
		{
			CVec3Dfp32(0, 0, 1).SetMatrixRow(MatBody, 2);
			MatBody.RecreateMatrix(2,0);
		}

		// Must use some crapor here to separate look/body
		CVec3Dfp32 Body = GetLook(CVec3Dfp32::GetMatrixRow(MatBody,0));
		//CWAGI_Context Context(_pObj, _pWPhysState, 0, 0);
		// Angle error version
		if (Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD)
			BodyAngle = 0.0f;

		// Absolute angle version
		/*fp32 LastAngle = pCD->m_LastBodyAngleZ;
		LastAngle += AngleAdjust(LastAngle, pCD->m_TargetBodyAngleZ);
		fp32 BodyAngle = LERP(LastAngle,pCD->m_TargetBodyAngleZ,_IPTime);
		BodyAngle = MACRO_ADJUSTEDANGLE(BodyAngle);*/

		//ConOut(CStrF("BodyAngle: %f",BodyAngle));
		// Absolute angle version
		//Body.k[2] = BodyAngle;
		if (pCD->m_iMountedObject != 0 && pCD->m_iPlayer == -1)
		{
			if (!(pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_TURRET))
			{
				// Modify (will not effect camera hopefully)
				// Get look from opponent and diff with our own...
				CVec3Dfp32 Body = GetLook(CVec3Dfp32::GetMatrixRow(MatLook,0));
				CWObject_CoreData* pObj = _pWPhysState->Object_GetCD(pCD->m_iMountedObject);
				CWO_Character_ClientData* pCDOther = (pObj && pObj->GetPhysState().m_ObjectFlags & (OBJECT_FLAGS_CHARACTER|OBJECT_FLAGS_PLAYER)) ? GetClientData(pObj) : NULL;
				if (pCDOther)
				{
					if (pCDOther->m_CameraUtil.IsActive(CAMERAUTIL_MODE_MOUNTEDCAM))
					{
						// meh...
						Body += pCDOther->m_CameraUtil.GetMountedAngles(pCDOther->m_PredictFrameFrac);
					}
					else
					{
						Body[2] = pCDOther->m_Control_Look_Wanted[2];
						Body[1] = pCDOther->m_Control_Look_Wanted[1];
					}
				}
				Body.CreateMatrixFromAngles(0, MatLook);
				CVec3Dfp32::GetMatrixRow(MatBody,3).SetMatrixRow(MatLook,3);
			}
		}
		else
		{
			// Angle error version
			Body.k[2] += BodyAngle;
			Body.CreateMatrixFromAngles(2, MatBody);
			CVec3Dfp32::GetMatrixRow(MatLook,3).SetMatrixRow(MatBody,3);
		}

	}
	else
	{
		CVec3Dfp32(0, 0, 1).SetMatrixRow(MatBody, 2);
		MatBody.RecreateMatrix(2, 0);
	
		//MatLook = _pObj->GetPositionMatrix();

		// Must use some crapor here to separate look/body
		CVec3Dfp32 Body = GetLook(CVec3Dfp32::GetMatrixRow(MatBody,0));
		//CWAGI_Context Context(_pObj, _pWPhysState, 0, 0);
		
		// Angle error version

		// Absolute angle version
		/*fp32 LastAngle = pCD->m_LastBodyAngleZ;
		LastAngle += AngleAdjust(LastAngle, pCD->m_TargetBodyAngleZ);
		fp32 BodyAngle = LERP(LastAngle,pCD->m_TargetBodyAngleZ,_IPTime);
		BodyAngle = MACRO_ADJUSTEDANGLE(BodyAngle);*/

		//ConOut(CStrF("BodyAngle: %f",BodyAngle));
	/*	if (pCD->m_iPlayer != -1)
			ConOutL(CStrF("BodyAngle: %f", BodyAngle));*/
		// Absolute angle version
		//Body.k[2] = BodyAngle;
		// Angle error version
		Body.k[2] += BodyAngle;

		fp32 Target,Last;
		Target = pCD->m_TurnCorrectionTargetAngle;
		Last = pCD->m_TurnCorrectionLastAngle;
		Body.CreateMatrixFromAngles(2, MatBody);

		MatBody.GetRow(3) = _Pos.GetRow(3);
		MatLook.GetRow(3) = _Pos.GetRow(3);
	}

	// Morph/move the character from "bounding box" position to the position that should actually
	// be used when in fighting mode
	if (pCD->m_iPlayer != -1)
	{
		// Actioncutscene position
		int32 CameraMode = pCD->m_ActionCutSceneCameraMode;
		int32 ACSType = (int32)pCD->m_ControlMode_Param2;
		int32 Flags = pCD->m_ControlMode_Param4;
		if ((Flags & ACTIONCUTSCENE_FLAGS_USEFLOATINGSTART) && 
			((Char_GetControlMode(_pObj) == PLAYER_CONTROLMODE_ACTIONCUTSCENE) &&
			((ACSType == ACTIONCUTSCENE_TYPE_USEHEALTHSTATION) || (CameraMode & CActionCutsceneCamera::ACS_CAMERAMODE_ACTIVE))))
		{
			CWObject_ActionCutscene::GetStartPosition(MatBody, pCD, _pWPhysState);
			MatLook = MatBody;
		}
	}

	if(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE )
	{		
		MatLook = pCD->m_DialoguePlayerPos;
		MatBody = MatLook;
		CVec3Dfp32(0, 0, 1).SetMatrixRow(MatBody, 2);
		MatBody.RecreateMatrix(2, 0);
	}

	fp32 StepUp = LERP(pCD->m_LastStepUpDamper, pCD->m_StepUpDamper, _IPTime);
	MatBody.k[3][2] += StepUp;
	MatLook.k[3][2] += StepUp;

/*	if(pCD->m_iPlayer != -1)
	{
		CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(MatBody,3);
		CVec3Dfp32 Fwd = CVec3Dfp32::GetMatrixRow(MatBody,0);
		Pos -= Fwd * 16;
		Pos.SetMatrixRow(MatBody,3);
	}
*/
/*	if (_DebugCalledFrom & 2)
	{
		_pWPhysState->Debug_RenderMatrix(MatBody, 3.0, true, 0xff7f7f00, 0xff000000, 0xff000000);
//		_pWPhysState->Debug_RenderMatrix(MatLook, 1.0, true, 0xff007f00, 0xff000000, 0xff000000);
	}*/
	

//	CXR_SkeletonInstance* pSkelInstance = GetClientSkelInstance(pCD, _iCacheSlot);

	CAnimGraphScopeLock SkelLock;

	CXR_SkeletonInstance* pSkelInstance;
	if(_Anim.m_pSkeletonInst)
	{
		pSkelInstance	= _Anim.m_pSkeletonInst;
		_bIgnoreCache	= true;
	}
	else
	{
		SkelLock.Create(pCD->m_SkelInstCacheLock);
		pSkelInstance = pCD->m_lspSkelInst[_iCacheSlot];
		if (_pSkel && !pSkelInstance && (((_iCacheSlot == 0) && (bIsPlayer || pCD->m_iMountedObject != 0)) || (_iCacheSlot != 0)))
		{
			MRTC_SAFECREATEOBJECT(spSkelInst, "CXR_SkeletonInstance", CXR_SkeletonInstance);
			pCD->m_lspSkelInst[_iCacheSlot] = spSkelInst;
			spSkelInst->Create(_pSkel->m_lNodes.Len());
			pSkelInstance = spSkelInst;
			_bIgnoreCache = true;
		}
	}


//	if(pCD->m_LastSkeletonInstanceTime != pCD->m_GameTime || pCD->m_LastSkeletonInstanceIPTime != _IPTime)
	if(_bIgnoreCache || 
		pCD->m_lLastSkeletonInstanceSkel[_iCacheSlot] != _pSkel || 
		pCD->m_lLastSkeletonInstanceTick[_iCacheSlot] != pCD->m_GameTick || 
		pCD->m_lLastSkeletonInstanceIPTime[_iCacheSlot] != _IPTime)
	{
		MSCOPESHORT( CacheMiss );
//		LogFile(CStrF("CACHEMISS iObj %d, _pObj %.8x, IPTime %f, %s, %s", _pObj->m_iObject, _pObj, _IPTime, MatBody.GetString().Str(), MatLook.GetString().Str()));

//		CXR_Anim *pAnim = _pWPhysState->GetMapData()->GetResource_Anim(pObj->m_iAnim2);
		CVec3Dfp32 RetPos = 0;
		CXR_Model *pModel = _pWPhysState->GetMapData()->GetResource_Model(pObj->m_iModel[0]);
		if (pModel)
		{
			CXR_Skeleton* pSkel = _pSkel;

			// Eval skeleton
			if (pSkel && pSkelInstance && pSkelInstance->m_nBoneTransform)
			{
				// Eval animation
				MSCOPE(EvalAnim, CHARACTER);

				{ // Set global move-scale. This is used by scaled trimeshes (";scale=x.xx" in the resource name)
					aint tmp = pModel->GetParam(CXR_MODEL_PARAM_GLOBALSCALE);
					if (tmp != 0)
						pSkelInstance->m_MoveScale = tmp * (1.0f / 1000.0f);
				}

				// Code below is nice to use if you are looking for trashed models
				/*memset(pSkelInstance->m_pBoneLocalPos, 0xfe, pSkelInstance->m_nBoneLocalPos * sizeof(pSkelInstance->m_pBoneLocalPos[0]));
				memset(pSkelInstance->m_pBoneTransform, 0xfe, pSkelInstance->m_nBoneTransform * sizeof(pSkelInstance->m_pBoneTransform[0]));
				memset(pSkelInstance->m_pTracksMove, 0xfe, pSkelInstance->m_nTracksMove * sizeof(pSkelInstance->m_pTracksMove[0]));
				memset(pSkelInstance->m_pTracksRot, 0xfe, pSkelInstance->m_nTracksRot * sizeof(pSkelInstance->m_pTracksRot[0]));*/
							
				fp32 AnimCamBlend = 0.0f;
				int SkeletonType = pCD->m_Aim_SkeletonType;

				if (pCD->m_CharSkeletonScale != 0.0f && (SkeletonType == SKELETONTYPE_NORMAL || SkeletonType == SKELETONTYPE_BUTCHER))
					CWO_AnimUtils::SetSkeletonScale(pSkelInstance, pCD->m_CharSkeletonScale);

				CXR_AnimLayer Layers[32];
				int nLayers = Char_GetAnimLayers(_pObj, MatLook, _pWPhysState, _IPTime, Layers);

				if((SkeletonType == SKELETONTYPE_NORMAL || SkeletonType == SKELETONTYPE_BUTCHER) && _pWPhysState->IsClient())
				{
					CWorld_ClientCore* pWClient = safe_cast<CWorld_ClientCore>(_pWPhysState);
					/*if (_bIgnoreCache)
					{
						static int LastTick = 0;
						static fp32 LastIPTime = 0.0f;
						if (pWClient->GetGameTick() != LastTick && LastIPTime != pWClient->GetRenderTickFrac())
						{
							LastTick = pWClient->GetGameTick();
							LastIPTime = pWClient->GetRenderTickFrac();
							ConOutL(CStrF("NewFrame:!"));
						}
						for (int32 i = 0; i < nLayers; i++)
						{
							ConOutL(CStrF("iObj: %d iLayer: %d LayerTime: %f Base: %d",_pObj->m_iObject,i,Layers[i].m_Time,Layers[i].m_iBlendBaseNode));
						}
					}*/
					{
						int Vocap = pCD->m_AnimGraph2.GetPropertyInt(PROPERTY_INT_VOCAPINFO);
						int Vocap2 = pCD->m_AnimGraph2.GetPropertyInt(PROPERTY_INT_VOCAPINFO2);
						int iHold =  Vocap2 ?  Vocap & Vocap2 & AG2_VOCAP_FLAG_IDLE : Vocap & AG2_VOCAP_FLAG_IDLE;


						CWorld_Client* pDebugRenderClient = NULL;
						if (_iCacheSlot == 0 && _pWPhysState->IsClient())
							pDebugRenderClient = safe_cast<CWorld_Client>(_pWPhysState);

						CWO_AnimUtils::EvalVocapAndLipsync(&pCD->m_VoCap, &pCD->m_LipSync, _pWPhysState->GetMapData(), pWClient->m_spSound,
														   pCD->m_AnimLipSync, ((CWObject_Client *)_pObj)->m_ClientData[0], CMTime::CreateFromTicks(pCD->m_GameTick, pWClient->GetGameTickTime(), _IPTime),
														   iHold, Layers, nLayers, 32, _iCacheSlot != 0, pDebugRenderClient);

						if (pCD->m_DarknessVoiceUse)
						{
							fp32 total = 0.0f;
							int num = 0;
							total = pCD->m_LipSync.GetBlendValues(pWClient->m_spSound, num);
							
							if(num)
								pCD->m_DarknessVoiceEffect = Min(1.0f, total);
						}
					}
				}

				CFacialSetup* pFacialSetup = NULL;
				if ((_iCacheSlot == 0) && (pCD->m_iFacialSetup != 0))
					pFacialSetup = _pWPhysState->GetMapData()->GetResource_FacialSetup(pCD->m_iFacialSetup);

				bool bRagdoll = false; //pCD->m_RagdollClient.IsValid();
				CMat4Dfp32 ObjWMatJunk(MatBody);
				if( pCD->m_RagdollClientData.m_State )
				{
					bRagdoll = true;
					if (pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_EXITRAGDOLL)
					{
						// Blend skeleton instances from ragdoll to current animation...
						// Get current without ragdoll
						CXR_SkeletonInstance SkelInstNormal,SkelInstRagDoll;
						SkelInstNormal.Create(pSkel->m_lNodes.Len());
						SkelInstRagDoll.Create(pSkel->m_lNodes.Len());
						pSkel->EvalAnim(Layers, nLayers, &SkelInstNormal, ObjWMatJunk);
						// Find blend
						fp32 Blend = 0.0f;
						for (int32 i = nLayers - 1; i >= 0; i--)
						{
							if (!Layers[i].m_iBlendBaseNode)
							{
								Blend = Layers[i].m_Blend;
								break;
							}
						}
						//Blend = 0.99f;
						CMat4Dfp32 Temp = MatBody;
						pCD->m_RagdollClientData.EvalAnim(pSkel,&SkelInstRagDoll,&pCD->m_RagdollClientMetaData);
						//pCD->m_RagdollClient.EvalAnim(_IPTime,Layers,nLayers,pSkel,&SkelInstRagDoll,Temp,0);
						SkelInstRagDoll.m_pBoneTransform[0] = MatBody;
						// Calculate ragdoll local positions
						CMat4Dfp32 MatRagDoll;
						MatRagDoll = MatBody;
						const int NumNodes = 23;
						// These are the bones the ragdoll uses
						static uint16 liLocalNodes[NumNodes] = 
						{
							PLAYER_ROTTRACK_ROOT,
							PLAYER_ROTTRACK_SPINE,
							PLAYER_ROTTRACK_TORSO,
							PLAYER_ROTTRACK_SPINE2,
							PLAYER_ROTTRACK_RLEG,
							PLAYER_ROTTRACK_RKNEE,
							PLAYER_ROTTRACK_RFOOT,
							PLAYER_ROTTRACK_LLEG,
							PLAYER_ROTTRACK_LKNEE,
							PLAYER_ROTTRACK_LFOOT,
							PLAYER_ROTTRACK_RSHOULDER,
							PLAYER_ROTTRACK_RARM,
							PLAYER_ROTTRACK_RELBOW,
							PLAYER_ROTTRACK_RHAND,
							PLAYER_ROTTRACK_LSHOULDER,
							PLAYER_ROTTRACK_LARM,
							PLAYER_ROTTRACK_LELBOW,
							PLAYER_ROTTRACK_LHAND,
							PLAYER_ROTTRACK_NECK,
							PLAYER_ROTTRACK_HEAD,
							PLAYER_ROTTRACK_RHANDATTACH,
							PLAYER_ROTTRACK_LHANDATTACH,
							PLAYER_ROTTRACK_CAMERA,
						};
						pSkel->CalculateLocalMatrices(&SkelInstRagDoll, MatRagDoll,liLocalNodes, NumNodes);
						// Blend skeletons
						SkelInstRagDoll.BlendInstance(&SkelInstNormal,pSkelInstance,Blend);
						// Evaluate target skeleton
						CMat4Dfp32 MatBlend;
						CVec3Dfp32 VRes;
						CVec3Dfp32::GetMatrixRow(SkelInstRagDoll.m_pBoneTransform[0], 3).Lerp(CVec3Dfp32::GetMatrixRow(SkelInstNormal.m_pBoneTransform[0], 3), Blend, VRes);

						CQuatfp32 Q1, Q2;
						Q1.Create(SkelInstRagDoll.m_pBoneTransform[0]);
						Q2.Create(SkelInstNormal.m_pBoneTransform[0]);

						CQuatfp32 QRes;
						Q1.Lerp(Q2, Blend, QRes);

						QRes.CreateMatrix(MatBlend);
						VRes.SetMatrixRow(MatBlend, 3);
						pSkel->EvalNode(PLAYER_ROTTRACK_ROOT, &MatBlend, pSkelInstance);
						RetPos = CVec3Dfp32::GetRow(ObjWMatJunk,3);
					}
					else
					{
						RetPos = pCD->m_RagdollClientData.EvalAnim(pSkel,pSkelInstance,&pCD->m_RagdollClientMetaData);
					}
				} 

#ifdef INCLUDE_OLD_RAGDOLL

				else if(bRagdoll)
				{
					MSCOPESHORT(EvalAnim_RagdollEvalAnim);
					if (pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_EXITRAGDOLL)
					{
						// Blend skeleton instances from ragdoll to current animation...
						// Get current without ragdoll
						CXR_SkeletonInstance SkelInstNormal,SkelInstRagDoll;
						SkelInstNormal.Create(pSkel->m_lNodes.Len());
						SkelInstRagDoll.Create(pSkel->m_lNodes.Len());
						pSkel->EvalAnim(Layers, nLayers, &SkelInstNormal, ObjWMatJunk);
						// Find blend
						fp32 Blend = 0.0f;
						for (int32 i = nLayers - 1; i >= 0; i--)
						{
							if (!Layers[i].m_iBlendBaseNode)
							{
								Blend = Layers[i].m_Blend;
								break;
							}
						}
						//Blend = 0.99f;
						CMat4Dfp32 Temp = MatBody;
						pCD->m_RagdollClient.EvalAnim(_IPTime,Layers,nLayers,pSkel,&SkelInstRagDoll,Temp,0);
						SkelInstRagDoll.m_pBoneTransform[0] = MatBody;
						// Calculate ragdoll local positions
						CMat4Dfp32 MatRagDoll;
						MatRagDoll = MatBody;
						const int NumNodes = 23;
						// These are the bones the ragdoll uses
						static uint16 liLocalNodes[NumNodes] = 
						{
							PLAYER_ROTTRACK_ROOT,
							PLAYER_ROTTRACK_SPINE,
							PLAYER_ROTTRACK_TORSO,
							PLAYER_ROTTRACK_SPINE2,
							PLAYER_ROTTRACK_RLEG,
							PLAYER_ROTTRACK_RKNEE,
							PLAYER_ROTTRACK_RFOOT,
							PLAYER_ROTTRACK_LLEG,
							PLAYER_ROTTRACK_LKNEE,
							PLAYER_ROTTRACK_LFOOT,
							PLAYER_ROTTRACK_RSHOULDER,
							PLAYER_ROTTRACK_RARM,
							PLAYER_ROTTRACK_RELBOW,
							PLAYER_ROTTRACK_RHAND,
							PLAYER_ROTTRACK_LSHOULDER,
							PLAYER_ROTTRACK_LARM,
							PLAYER_ROTTRACK_LELBOW,
							PLAYER_ROTTRACK_LHAND,
							PLAYER_ROTTRACK_NECK,
							PLAYER_ROTTRACK_HEAD,
							PLAYER_ROTTRACK_RHANDATTACH,
							PLAYER_ROTTRACK_LHANDATTACH,
							PLAYER_ROTTRACK_CAMERA,
						};
						pSkel->CalculateLocalMatrices(&SkelInstRagDoll, MatRagDoll,liLocalNodes, NumNodes);
						// Blend skeletons
						SkelInstRagDoll.BlendInstance(&SkelInstNormal,pSkelInstance,Blend);
						// Evaluate target skeleton
						CMat4Dfp32 MatBlend;
						CVec3Dfp32 VRes;
						CVec3Dfp32::GetMatrixRow(SkelInstRagDoll.m_pBoneTransform[0], 3).Lerp(CVec3Dfp32::GetMatrixRow(SkelInstNormal.m_pBoneTransform[0], 3), Blend, VRes);

						CQuatfp32 Q1, Q2;
						Q1.Create(SkelInstRagDoll.m_pBoneTransform[0]);
						Q2.Create(SkelInstNormal.m_pBoneTransform[0]);

						CQuatfp32 QRes;
						Q1.Lerp(Q2, Blend, QRes);

						QRes.CreateMatrix(MatBlend);
						VRes.SetMatrixRow(MatBlend, 3);
							pSkel->EvalNode(PLAYER_ROTTRACK_ROOT, &MatBlend, pSkelInstance);
					}
					else
					{
						pCD->m_RagdollClient.EvalAnim(_IPTime,Layers,nLayers,pSkel,pSkelInstance,ObjWMatJunk,0, _OverrideRagdollHeight);
					}

					RetPos = CVec3Dfp32::GetRow(ObjWMatJunk,3);
//						pCD->m_spRagdollClient->Draw(0.05f,CVec3Dfp32(0,0,32));

					/*bool bDevour = false;
						if (bDevour)
						{
							if (pFacialSetup)
							{
								uint nMove = pSkelInstance->m_nTracksMove;
								uint nRot = pSkelInstance->m_nTracksRot;
							memset(pSkelInstance->m_pTracksMove, 0, nMove * sizeof(CVec3Dfp32));
							memset(pSkelInstance->m_pTracksRot, 0, nMove * sizeof(CQuatfp32));
								memset(lFaceData, 0, sizeof(lFaceData));

								lFaceData[37] = 1.0f; // wide open mouth
							}
						}
						else
						{
							// No need to evaluate face data
							pFacialSetup = NULL;
					}*/
				}
#endif // INCLUDE_OLD_RAGDOLL
				else
				{
					MSCOPESHORT(EvalAnim_EvalAnim);
					// Animation camera blendin
					fp32 CamBlendIn = 0.0f;
					fp32 CamBlendOut = 1.0f;
					const fp32 BlendTime = 6.0f;
					if (pCD->m_AnimCamBlendInStart && pCD->m_AnimCamBlendInStart <= pCD->m_GameTick)
					{
						int32 GTDiff = pCD->m_GameTick - pCD->m_AnimCamBlendInStart;
						CamBlendIn = (((fp32)GTDiff) + pCD->m_PredictFrameFrac) * (1.0f / BlendTime);
						CamBlendIn = Max(0.0f,Min(CamBlendIn, 1.0f));
					}
					if (pCD->m_AnimCamBlendOutStart && (pCD->m_AnimCamBlendOutStart > pCD->m_AnimCamBlendInStart))
					{
						int32 GTDiff = pCD->m_GameTick - pCD->m_AnimCamBlendOutStart;
						CamBlendOut = (((fp32)GTDiff) + pCD->m_PredictFrameFrac) * (1.0f / BlendTime);
						CamBlendOut = Max(0.0f,Min(1.0f, CamBlendOut));
						CamBlendOut = 1.0f - CamBlendOut;
					}
					AnimCamBlend = Sinc(CamBlendIn * CamBlendOut);

						/*if (pCD->m_iPlayer != -1 && _pWPhysState->IsClient())
							ConOut(CStrF("Blend: %f BIn: %f BOut: %f BIn: %d BOut: %d Tick: %d",AnimCamBlend,CamBlendIn,CamBlendOut,pCD->m_AnimCamBlendInStart,pCD->m_AnimCamBlendOutStart,pCD->m_GameTick));*/

						/*if (pCD->m_AnimCamBlendInStart || pCD->m_AnimCamBlendOutStart)

					fp32 BlendIn = Min(Layers[i].m_Time / 0.3f,1.0f);
					fp32 BlendOut = Min(1.0f,Layers[i].m_BlendOut);
						AnimCamBlend = Max(Min(BlendIn,BlendOut),AnimCamBlend);*/

					if(nLayers >= 32)
						Error_static("OnGetAnimState", "Too many layers. Stacktrashing...");

					if (pFacialSetup && _pTrackMask)
						_pTrackMask->Or(*CWO_AnimUtils::GetFaceDataTrackMask());

					pSkel->EvalAnim(Layers, nLayers, pSkelInstance, ObjWMatJunk, 0, _pTrackMask);
					RetPos = CVec3Dfp32::GetRow(ObjWMatJunk,3);
				}

				{
					MSCOPESHORT(PostAnimEffects);
					CMat4Dfp32 CameraTrack;
					CMat4Dfp32 AimMat;
					CameraTrack.Unit();
					AimMat.Unit();

					if (AnimCamBlend > 0.0f)
					{
						// Only need to be done if blending
						if (AnimCamBlend < 1.0f)
						{
							CVec3Dfp32 Pos(CVec3Dfp32::GetMatrixRow(pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA],3));
							CQuatfp32 Mek,Mek2,Mek3;
							Mek.Create(pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA]);
							Mek2.Create(MatLook);
							Mek2.Lerp(Mek, AnimCamBlend, Mek3);
							Mek3.CreateMatrix3x3(CameraTrack);
							Pos.SetMatrixRow(CameraTrack,3);
							Pos.SetMatrixRow(AimMat,3);
							CameraTrack.GetRow(0).SetRow(AimMat,0);
							LERP(CameraTrack.GetRow(2),CVec3Dfp32(0.0f,0.0f,1.0f),AnimCamBlend).SetRow(AimMat,2);
							AimMat.RecreateMatrix(2,0);
						}
						else
						{
							CameraTrack = pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA];
							CameraTrack.GetRow(0).SetRow(AimMat,0);
							CVec3Dfp32(0.0f,0.0f,1.0f).SetRow(AimMat,2);
							AimMat.RecreateMatrix(2,0);
							CameraTrack.GetRow(2).SetRow(AimMat,3);
						}
					}
					else
					{
						CameraTrack = MatLook;
						AimMat = MatLook;
					}
				
					if (!bRagdoll && 
						(Char_ControlMode != PLAYER_CONTROLMODE_LADDER)&&
						(Char_ControlMode != PLAYER_CONTROLMODE_ACTIONCUTSCENE)&&
						(Char_ControlMode != PLAYER_CONTROLMODE_LEDGE2) &&
						(Char_ControlMode != PLAYER_CONTROLMODE_HANGRAIL) &&
						(pSkel->m_lNodes.Len() > PLAYER_ROTTRACK_CAMERA) && 
						(pCD->m_Aim_SkeletonType != 0 || pCD->m_ForcedAimingMode != 0))
					{
						if (pCD->m_PostAnimSystem.m_Anim_iCurIKLookMode == 55 || pCD->m_ForcedAimingMode == 55)
						{
							// Use aimmat on root
							pSkelInstance->m_pBoneTransform[0] = AimMat;
						}
						else
						{
							pCD->m_PostAnimSystem.m_ForcedAimingMode = pCD->m_ForcedAimingMode;
							pCD->m_PostAnimSystem.m_Aim_SkeletonType = pCD->m_Aim_SkeletonType;
							CWO_CharDarkling_ClientData* pDarklingCD = pCD->IsDarkling();
							CVec3Dfp32 UpVector = pDarklingCD ? pDarklingCD->m_UpVector : CVec3Dfp32(0.0f,0.0f,1.0f);
							pCD->m_PostAnimSystem.EvalAimIK(pSkelInstance, _IPTime, MatBody, AimMat, UpVector);
						}

						// We don't do this when the char is dead as it may
						// fuck up ragdoll things
						if (Char_GetPhysType(_pObj) != PLAYER_PHYS_DEAD)
						{
							//---------------------------------------------------------------------------
							// IK-stuff

							//if(((_pWPhysState->IsClient() && (_ClothScaleOverride > -2.0f)) || _pWPhysState->IsServer()) && TestScale == 1.0f)
							if(((/*_iCacheSlot == 0 &&*/ (_ClothScaleOverride > -2.0f)) || _pWPhysState->IsServer()))// && TestScale == 1.0f)
							{

								CWO_Character_ClientData *pCDFirst = NULL;
								CWObject_CoreData* pObjFirst = NULL;
								
								bool PVSSaysOK = true;

								if(_pWPhysState->IsClient())
								{
									CWorld_Client* pWClient = safe_cast<CWorld_Client>(_pWPhysState);
									CXR_Engine *pEngine = pWClient->Render_GetEngine();
									if(pEngine && pEngine->GetVCDepth() == 0) // ignore IK in reflections
									{
										pObjFirst = pWClient->Object_GetFirstCopy(_pObj->m_iObject);
										pCDFirst = pObjFirst ? GetClientData(pObjFirst) : NULL;
									}
									
									if(pCDFirst)
									{
										CMTime GameTime = CMTime::CreateFromTicks(pCD->m_GameTick, _pWPhysState->GetGameTickTime(), _IPTime);
										pCDFirst->m_PostAnimSystem.UpdateTimer(&GameTime);
									}
								}
								else
								{
									pObjFirst = _pObj;
									pCDFirst = pCD;
									if((_pWPhysState->GetGameTick() - ((CWObject *)_pObj)->GetLastTickInClientPVS()) > 10)
										PVSSaysOK = false;

									if(pCDFirst)
									{
										CMTime GameTime = CMTime::CreateFromTicks(pCD->m_GameTick, _pWPhysState->GetGameTickTime());
										pCDFirst->m_PostAnimSystem.UpdateTimer(&GameTime);

										// used for detecting stairs
										if(pSkelInstance)
											pCDFirst->m_PostAnimSystem.SetModel0Mat(&pSkelInstance->m_pBoneTransform[0]);
									}
								}

								if(pCDFirst && pObjFirst && PVSSaysOK)
								{
									pCDFirst->m_PostAnimSystem.SetCameraLookMat(&MatLook);
									CWO_CharDarkling_ClientData* pDarklingCD = pCDFirst->IsDarkling();
									if(pDarklingCD && _iCacheSlot == 0)
									{
										EvalSkelAndApplyBoneScale(pCD->m_CharGlobalScale, pSkelInstance, pSkel);
										
										bool bIsWallClimber = (pDarklingCD->m_Flags & DARKLING_FLAGS_CAN_CLIMB) != 0;
										pCDFirst->m_PostAnimSystem.EvalFeetIK(pSkel, pSkelInstance, pObjFirst, pModel, _pWPhysState, CPostAnimSystem::FEET_TYPE_DARKLING_QUADRO, 
											Char_GetPhysType(pObjFirst), bIsWallClimber);
									}
									else
									{
										//-------------------------------------------------------------------------------------------------------------------------
										// This should be moved somewhere..
										if (SkeletonType == SKELETONTYPE_BUTCHER)
										{
											if (pCD->m_AnimGraph2.GetStateFlagsHi() & AG2_STATEFLAGHI_IKSYSTEM_ENABLELEFTHANDIK)
											{
												fp32 Scale = 1.0f - pCD->m_CharSkeletonScale;
												CVec3Dfp32 LeftHandUnscaled;
												LeftHandUnscaled = pSkel->m_lNodes[PLAYER_ROTTRACK_LHANDATTACH].m_LocalCenter;
												LeftHandUnscaled *= pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_LHANDATTACH];
												LeftHandUnscaled -= RetPos;
												LeftHandUnscaled *= (1.0f / Scale);
												LeftHandUnscaled += RetPos;
												// move left hand to its unscaled position
												pCDFirst->m_PostAnimSystem.SetGrabPoint(&LeftHandUnscaled, 0.5f, NULL, 0.0f);
											}
											else
											{
												// release hand ik
												pCDFirst->m_PostAnimSystem.SetGrabPoint(NULL, 0.0f, NULL, 0.0f);
											}
										}
										else if (pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_TURRET)
										{
											CVec3Dfp32 AttachPoints[2];
											CWObject_Message Msg(CWObject_Turret::OBJMSG_TURRET_GETATTACHPOSITIONS);
											Msg.m_pData = AttachPoints;
											if (_pWPhysState->Phys_Message_SendToObject(Msg,pCD->m_iMountedObject))
												pCDFirst->m_PostAnimSystem.SetGrabPoint(&AttachPoints[0], 0.0f, &AttachPoints[1], 0.0f);
											else
												pCDFirst->m_PostAnimSystem.ReleaseGrabPoint(true,true);
										}
										else
										{
											pCDFirst->m_PostAnimSystem.ReleaseGrabPoint(true,true);
										}
										//-------------------------------------------------------------------------------------------------------------------------

										if(_iCacheSlot == 0) // dont do this when asking for physskeleteon
											pCDFirst->m_PostAnimSystem.EvalFeetIK(pSkel, pSkelInstance, pObjFirst, pModel, _pWPhysState, CPostAnimSystem::FEET_TYPE_HUMAN_BIPED, 
												Char_GetPhysType(pObjFirst), false);

										if(!(pCDFirst->m_PostAnimSystem.m_IKFlags & CPostAnimSystem::IKFLAGS_OVERRIDEPOSITION))
										{
											EvalSkelAndApplyBoneScale(pCD->m_CharGlobalScale, pSkelInstance, pSkel);
										}
										
										// bypass animgraphs headlook while in behaviour
										if(pCD->m_iPlayer == -1)
										{
											if (pCDFirst->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_BEHAVIORACTIVE)
											{
												CVec3Dfp32 tester = pCDFirst->m_EyeLookDir;
												if ((pCDFirst->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_BEHAVIOR_IKLOOK) && 
													!(pCDFirst->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_BEHAVIORENTER) && 
													!(pCDFirst->m_AnimGraph2.GetStateFlagsLo() & AG2_STATEFLAG_BEHAVIOREXIT))
												{ 
													pCDFirst->m_PostAnimSystem.SetHeadLookDir(&tester);
												}
												else
												{
													pCDFirst->m_PostAnimSystem.SetHeadLookDir(NULL);
												}
											}
											else
												pCDFirst->m_PostAnimSystem.SetHeadLookDir(NULL);

											pCDFirst->m_PostAnimSystem.UpdateHeadLookDir(pSkel,  pSkelInstance, _pWPhysState);
										}
										
										// no we have an evaluated skeleton?
										if(!pCDFirst->m_PostAnimSystem.EvalHandsForGrab(pSkel, pSkelInstance, _pWPhysState, pCDFirst))
										pCDFirst->m_PostAnimSystem.EvalHandsOnWeapon(pSkel, pSkelInstance, pObjFirst, pModel, _pWPhysState);
									}
								}								
							}
							else
								EvalSkelAndApplyBoneScale(pCD->m_CharGlobalScale, pSkelInstance, pSkel);
					
							
							//--------------------------------------------------------------------------------------------------
							
						}

						if(pSkelInstance->m_nBoneLocalPos > 73)
							CWO_AnimUtils::ApplyRollBone(_pObj, _pSkel, pSkelInstance);

						// Facial
						if (pFacialSetup && _pWPhysState->IsClient() && (pSkelInstance->m_nBoneTransform >= PLAYER_ROTTRACK_LEYE))
						{
							fp32 lFaceData[FACEDATA_MAXCHANNELS];
							pFacialSetup->GetFaceData(pSkelInstance, FACEDATA_BASETRACK, FACEDATA_MAXCHANNELS, lFaceData);

							CWO_AnimUtils::ScaleFace(lFaceData, pCD->m_VoCap.m_DialogueAnimFlags);

							// Blink
							if(!(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_NOBLINK))
							{
								fp32 TimeOffset = _IPTime * _pWPhysState->GetGameTickTime();
								bool bDead = (Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD);
								pCD->m_Blink.Apply(TimeOffset, bDead, lFaceData);
							}

							// Eye movement pass 1 to contract muscles
							CVec3Dfp32 LookPos;
							bool bEyeMovement = false;
							if(pCD->m_iEyeLookObj > 0)
							{
								CWObject_CoreData *pObj = _pWPhysState->Object_GetCD(pCD->m_iEyeLookObj);
								if (pObj)
								{
									int ObjectFlags = pObj->GetPhysState().m_ObjectFlags;
									if(ObjectFlags & OBJECT_FLAGS_PLAYER && Char_IsPlayerView(pObj))
									{
										CWorld_ClientCore *pWClient = safe_cast<CWorld_ClientCore>(_pWPhysState);
										CMat4Dfp32 Camera;
										pWClient->Render_GetLastRenderCamera(Camera);
										LookPos = Camera.GetRow(3);
									}
									else if(ObjectFlags & OBJECT_FLAGS_CHARACTER)
										LookPos = Char_GetMouthPos(pObj);
									else
										pObj->GetAbsBoundBox()->GetCenter(LookPos);
								}
								bEyeMovement = CWO_AnimUtils::SetEyeMuscles_LookAtPosition(LookPos, lFaceData, pSkelInstance, pSkel);
							}
//							else
//								bEyeMovement = CWO_AnimUtils::SetEyeMuscles_LookDirection(MatLook.GetRow(0), lFaceData, pSkelInstance);

							// Eval
							pFacialSetup->Eval(lFaceData, pSkelInstance);

							// Re-evaluate from head joint and up
							{
								// ... but before re-evaluating, make sure head movement isn't lost (copy local transform back to rot/move tracks)
								const CMat4Dfp32& HeadLocalPos = pSkelInstance->m_pBoneLocalPos[PLAYER_ROTTRACK_HEAD];
								pSkelInstance->m_pTracksRot[PLAYER_ROTTRACK_HEAD].Create(HeadLocalPos);
							}
							pSkel->InitEvalNode(PLAYER_ROTTRACK_HEAD, pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_NECK], pSkelInstance);

							// Eye movement pass 2 to force eyes in exact direction
							if(bEyeMovement && pCD->m_iEyeLookObj > 0)
								CWO_AnimUtils::RotateEyeBones_ToLookAt(LookPos, pSkelInstance, pSkel);
						}
							
						if((_Anim.m_Anim0 == 0)/*&& (pCD->m_iPlayer != -1 || pCD->m_iMountedObject != 0) && pSkelInstance->m_nBoneTransform > PLAYER_ROTTRACK_CAMERA)*/)
						{
							pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA] = CameraTrack;
							// 1. Get position from head track  (x = Head.Local * Head.Transform)
							// 2. Set M = Camera.Transform
							// 3. Change M.T so that 'x' is preserved:
							//      x = x*M = x*R + T  -->  T = x - x*R

							CVec3Dfp32 HeadPos = _pSkel->m_lNodes[PLAYER_ROTTRACK_HEAD].m_LocalCenter + CVec3Dfp32(3.5f, 0.0f, 1.75f);// + DownVecLocal;
							HeadPos *= pSkelInstance->GetBoneTransform(PLAYER_ROTTRACK_HEAD);

							CMat4Dfp32& CameraMat = pSkelInstance->m_pBoneTransform[PLAYER_ROTTRACK_CAMERA];
							CVec3Dfp32 x = _pSkel->m_lNodes[PLAYER_ROTTRACK_CAMERA].m_LocalCenter;
							x.MultiplyMatrix3x3(CameraMat);
							CameraMat.GetRow(3) = HeadPos - x; // adjust for different rotation and localcenter
						}
					}
					else if(pSkelInstance->m_nBoneLocalPos > 73)
						CWO_AnimUtils::ApplyRollBone(_pObj, _pSkel, pSkelInstance);
				}
			}
		}

		if(!_bIgnoreCache || (bIsPlayer && (_iCacheSlot == 0)))
		{
			if((_iCacheSlot == 0) && bIsPlayer && _Anim.m_pSkeletonInst)
			{
				CAnimGraphScopeLock SkelLock2;
				SkelLock2.Create(pCD->m_SkelInstCacheLock);
				if(!pCD->m_lspSkelInst[0])
				{
					MRTC_SAFECREATEOBJECT(spSkelInst, "CXR_SkeletonInstance", CXR_SkeletonInstance);
					pCD->m_lspSkelInst[0] = spSkelInst;
					spSkelInst->Create(_pSkel->m_lNodes.Len());
				}
				pSkelInstance->Duplicate(pCD->m_lspSkelInst[0]);
			}
			pCD->m_lLastSkeletonInstanceTick[_iCacheSlot] = pCD->m_GameTick;
			pCD->m_lLastSkeletonInstanceIPTime[_iCacheSlot] = _IPTime;
			pCD->m_lLastSkeletonInstancePos[_iCacheSlot] = RetPos;
			pCD->m_lLastSkeletonInstanceSkel[_iCacheSlot] = _pSkel;
		}
		else if(!_Anim.m_pSkeletonInst)
			pCD->m_lLastSkeletonInstanceTick[_iCacheSlot] = -1;

		if(_pRetPos)
			*_pRetPos = RetPos;
	}
	else
	{
		MSCOPESHORT( CacheHit );
//		LogFile(CStrF("CACHEHIT iObj %d, _pObj %.8x, IPTime %f", _pObj->m_iObject, _pObj, _IPTime));
		if(_pRetPos)
			*_pRetPos = pCD->m_lLastSkeletonInstancePos[_iCacheSlot];
	}

	_Anim.m_pSkeletonInst = pSkelInstance;
	_Anim.m_SurfaceOcclusionMask = 0;
	_Anim.m_SurfaceShadowOcclusionMask = 0;
	_Anim.m_AnimTime0 = pCD->m_GameTime;

	return 1;
}


//TODO: Move this into the skeleton class?  (and make it non-recursive...)
static void EvalMissingNodes_r(uint16 _iNode, int16 _iParent, const CXR_Skeleton& _MainSkel, const CXR_Skeleton& _Other, CXR_SkeletonInstance& _SkelInst)
{
	const CXR_SkeletonNode& Node2 = _Other.m_lNodes[_iNode];
	int iRotSlot = Node2.m_iRotationSlot;

	if (!_MainSkel.m_TrackMask.IsEnabledRot(iRotSlot))
	{
		M_ASSERT(_iParent >= 0, "Don't run this on two skeletons with nothing in common!!");

		const CMat4Dfp32& ParentMat = _SkelInst.m_pBoneTransform[_iParent];
		_Other.InitEvalNode(_iNode, ParentMat, &_SkelInst);
		return;
	}

	TAP<const uint16> piNodes = _Other.m_liNodes;
	for (uint8 i = 0; i < Node2.m_nChildren; i++)
	{
		uint16 iChild = piNodes[Node2.m_iiNodeChildren + i];
		EvalMissingNodes_r(iChild, _iNode, _MainSkel, _Other, _SkelInst);
	}
}


bool CWObject_Character::OnGetAnimState2(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CXR_Model* _lpModels[CWO_NUMMODELINDICES], const CMat4Dfp32& _Pos, fp32 _IPTime, CXR_AnimState& _Anim, CVec3Dfp32* _pRetPos, bool _bIgnoreCache, fp32 _ClothScaleOverride, fp32 _OverrideRagdollHeight)
{
	MSCOPESHORT(CWObject_Character::OnGetAnimState2);
	M_ASSERT(_pWPhysState->IsClient(), "This is client-only, for rendering stuff...");
	M_ASSERT(_lpModels[0], "Invalid main model!");

	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
	M_ASSERTHANDLER(pCD, "Should always exist!", return false);
	M_ASSERTHANDLER(_Anim.m_pSkeletonInst, "Need a render skeleton instance!", return false);

	CXR_SkeletonInstance* pSkelInst = _Anim.m_pSkeletonInst;
	CXR_Skeleton* pSkel = _lpModels[0]->GetSkeleton();
	M_ASSERT(pSkel, "OnGetAnimState2: No skeleton for main model!");

	uint nModels = 1;
	for (uint i = 1; i < CWO_NUMMODELINDICES; i++)
		if (_lpModels[i]) _lpModels[nModels++] = _lpModels[i]; // fill gaps.. (shouldn't be needed)

	// Add cloth from all models
	CXR_Skeleton* lpSkel[CWO_NUMMODELINDICES] = { pSkel };

	CXR_Anim_TrackMask TrackMask;
	TrackMask.Copy(*pSkel->GetTrackMask());

	uint nCloth = pSkel->m_lCloth.Len();
	for (uint i = 1; i < nModels; i++)
	{
		lpSkel[i] = _lpModels[i]->GetSkeleton();
		if (lpSkel[i])
			TrackMask.Or(*lpSkel[i]->GetTrackMask());
	}

	if (!OnGetAnimState(_pObj, _pWPhysState, pSkel, 0, _Pos, _IPTime, _Anim, _pRetPos, _bIgnoreCache, _ClothScaleOverride, &TrackMask, _OverrideRagdollHeight))
		return false;

	// Make sure nodes in extra skeletons are evaluated
	for (uint i = 1; i < nModels; i++)
	{
		if (lpSkel[i])
		{
			nCloth += lpSkel[i]->m_lCloth.Len();
			EvalMissingNodes_r(0, -1, *pSkel, *lpSkel[i], *pSkelInst);
		}
	}

#ifndef CLOTH_DISABLE
	static int8 s_bClothDisable = -1;
	if (s_bClothDisable < 0)
	{
		s_bClothDisable = 0;
		MACRO_GetSystemEnvironment(pEnv);
		if (pEnv)
			s_bClothDisable = pEnv->GetValuei("CLOTH_DISABLE", 0);
	}

	if (!s_bClothDisable && (_ClothScaleOverride != -2.0f))
	{
		CMat4Dfp32 AnimPos = pSkelInst->GetBoneTransform(PLAYER_ROTTRACK_ROOT);

		if (nCloth > 0 && nCloth != pCD->m_lCharacterCloth.Len())
		{
			M_TRACEALWAYS("resizing m_lCharacterCloth\n");
			pCD->m_lCharacterCloth.SetLen(0);
			pCD->m_lCharacterCloth.SetLen(nCloth);
		}

		for (uint iModel = 0, iCloth = 0; iModel < nModels; iModel++)
		{
			CXR_Skeleton* pCurrSkel = lpSkel[iModel];
			if (!pCurrSkel)
				continue;

			uint n = pCurrSkel->m_lCloth.Len();
			for (uint j = 0; j < n; j++, iCloth++)
			{
				CXR_Cloth& Cloth = pCurrSkel->m_lCloth[j]; // can't be const because of lazy initiation :(
				if (Cloth.m_MaxID == -1)
					continue; // ignoring invalid cloth
				M_ASSERTHANDLER(Cloth.m_MaxID < pSkelInst->m_nBoneTransform, "Cloth error!", continue);

				// Check if we need to recreate cloth data
				CXR_SkeletonCloth& ClothInstance = pCD->m_lCharacterCloth[iCloth];
				bool bNeedCreate = !ClothInstance.m_IsCreated;
				bNeedCreate |= (ClothInstance.m_lJointPosition.Len() != (Cloth.m_MaxID+1));

				if (bNeedCreate && !ClothInstance.m_bInvalid)
				{
					M_TRACEALWAYS("recreate cloth %d\n", j);
					ClothInstance.Create(j, pCurrSkel, pSkelInst, _lpModels, nModels);
					ClothInstance.m_LastUpdate.MakeInvalid();
					ClothInstance.m_LastUpdatePosition = AnimPos;
				}

				if (ClothInstance.m_IsCreated)
				{
					// Check if we need to update cloth
					CMTime time = CMTime::CreateFromTicks(pCD->m_GameTick, _pWPhysState->GetGameTickTime(), _IPTime);		
					{
						if (ClothInstance.m_LastUpdate.IsInvalid())
							ClothInstance.m_LastUpdate = time;

						fp32 dt = (time - ClothInstance.m_LastUpdate).GetTime();
						ClothInstance.m_LastUpdate = time;
						dt = Min(dt, 1.0f / 30.0f);

						fp32 ClothBlend = _ClothScaleOverride != -1.0f ? _ClothScaleOverride : pCD->m_AnimGraph2.GetClothAnimScale();

						bool bDoClothSimulation = (ClothBlend < 1.0f) && (dt > 0.0f);
						if (bDoClothSimulation)
						{
							fp32 Threshold = (pCD->m_RenderAttached == 0) ? 16.0f : 64.0f;		// don't want to reset when in the car in ny1_tunnel
							{
								fp32 DistSqr = AnimPos.GetRow(3).DistanceSqr(ClothInstance.m_LastUpdatePosition.GetRow(3));
								fp32 LookDot = AnimPos.GetRow(0) * ClothInstance.m_LastUpdatePosition.GetRow(0);
								DistSqr -= _pObj->GetVelocity().m_Move.LengthSqr(); // compensate for movement
								if (DistSqr > Sqr(Threshold) || LookDot < 0.5f)
								{
									// we moved a lot in just one frame... reset the system
									M_TRACE("Reset cloth (too much movement)\n");
									ClothInstance.ResetCloth(pCurrSkel, pSkelInst);
								}
							}
							ClothInstance.m_LastUpdatePosition = AnimPos;
						}

						fp32 FloorOffset = _pObj->GetPosition()[2] + 0.05f * 32.0f;
						if ((Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD))
							FloorOffset -= 5.75f; // Quick hack, lower floor offset slightly downwards on dead characters

						ClothInstance.SetFloorOffset(FloorOffset);
						ClothInstance.Step(_lpModels, nModels, pCurrSkel, pSkelInst, dt, ClothBlend, pCD->m_AnimGraph2.GetClothSimFreq(), _pWPhysState);
					}
				}
			}
		}
	}
#endif

	return true;
}



void TraceChaseCamera(CWObject_Client* _pObj, CWorld_Client* _pWClient, CMat4Dfp32& _CameraMatrix, fp32& _WantedDistance, fp32& _MaxDistance)
{
	MAUTOSTRIP(TraceChaseCamera, MAUTOSTRIP_VOID);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if (!pCD) return;

	if (CWObject_Character::Char_GetPhysType(_pObj) == PLAYER_PHYS_NOCLIP)
	{
		_WantedDistance = 1.0f;
		_MaxDistance = 1.0f;
		return;
	}

	CVec3Dfp32 Head = CVec3Dfp32::GetMatrixRow(_CameraMatrix, 3);
	CVec3Dfp32 Behind = pCD->m_Camera_CurBehindOffset;
	Behind.MultiplyMatrix3x3(_CameraMatrix);
	fp32 BehindDistance = pCD->m_Camera_CurBehindOffset.Length(); // Behind.Length() would also do.

//	CVec3Dfp32 Fwd = CVec3Dfp32::GetMatrixRow(_CameraMatrix, 2);
//	CVec3Dfp32 Side = CVec3Dfp32::GetMatrixRow(_CameraMatrix, 0);
//	CVec3Dfp32 Up = CVec3Dfp32::GetMatrixRow(_CameraMatrix, 1);
	
	CVec3Dfp32 Fwd, Side, Up;
	Fwd = -(Behind / BehindDistance);
	Side = CVec3Dfp32::GetMatrixRow(_CameraMatrix, 0);

	Up = CVec3Dfp32::GetMatrixRow(_CameraMatrix, 1);
	Fwd.CrossProd(Side, Up);

//	_pWClient->Debug_RenderVertex(Head, 0xF00000F0, 50.0f);

	int objects = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
	int mediums = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID;

	bool bHit;
	CCollisionInfo CInfo;

	_MaxDistance = 1.0f;
	fp32 DistanceHint = _MaxDistance;

	bHit = _pWClient->Phys_IntersectLine(Head, Head + Behind, 0, objects, mediums, &CInfo, _pObj->m_iObject);
	if (bHit && CInfo.m_bIsValid)
	{
		_MaxDistance = Min(CInfo.m_Time, _MaxDistance);
//		_pWClient->Debug_RenderWire(Head, Head + Behind * CInfo.m_Time, 0x40400000, 50.0f);
	}
	else
	{
//		_pWClient->Debug_RenderWire(Head, Head + Behind, 0x40004000, 50.0f);
	}

	int NumRings = 3;
	fp32 CircumStep = _PI2 / 6.0f;

	int NumTraces = 1;

	for (int iRing = 1; (iRing - 1) < NumRings; iRing++)
	{
		fp32 segmentfraction = (fp32)iRing / (fp32)NumRings;
//		fp32 invsqrinv_segmentfraction = 1.0f - (1.0f - segmentfraction) * (1.0f - segmentfraction);
		fp32 a = segmentfraction * _PIHALF;
		fp32 cosa = M_Cos(a);
		fp32 sina = M_Sin(a);
		fp32 circum = sina * _PI2;
		int NumSides = RoundToInt(circum / CircumStep);

		for (int iSide = 0; iSide < NumSides; iSide++)
		{
			fp32 ringfraction = (fp32)iSide / (fp32)NumSides;
			fp32 b = ringfraction * _PI2;
			fp32 cosb = M_Cos(b) * sina;
			fp32 sinb = M_Sin(b) * sina;

			fp32 DistanceScale = 0.4f + 0.6f * Sqr(Sqr(cosa));

			CVec3Dfp32 ray = (-Fwd * cosa + Side * cosb + Up * sinb) * BehindDistance * DistanceScale;

			NumTraces++;

			bHit = _pWClient->Phys_IntersectLine(Head, Head + ray, 0, objects, mediums, &CInfo, _pObj->m_iObject);
			if (bHit && CInfo.m_bIsValid)
			{
				fp32 Weight = 1.0f - (Sqr(segmentfraction));
				fp32 WeightedDistance = LERP(CInfo.m_Time * DistanceScale, 1.0f, Weight);
				DistanceHint = Min(WeightedDistance, DistanceHint);
//				_pWClient->Debug_RenderWire(Head, Head + ray * CInfo.m_Time * DistanceScale, 0x40400000, 50.0f);
			}
			else
			{
//				_pWClient->Debug_RenderWire(Head, Head + ray, 0x40004000, 50.0f);
			}
		}
	
	}

//	ConOut(CStrF("NumChaseCamTraces = %i", NumTraces));

	_WantedDistance = Min(Max(0.0f, DistanceHint), _MaxDistance);
}

void CWObject_Character::Char_EvolveClientEffects(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CWObject_Character_Char_EvolveClientEffects, MAUTOSTRIP_VOID);
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if (!pCD) return;

	MSCOPE(CWObject_Character::Char_EvolveClientEffects, Char_EvolveClientEffectsMem);

	int iModel;
	int MaxNumModels = Min(4, ATTACHMODEL_NUMMODELS);
	if (pCD->m_Item0_Flags & RPG_ITEM_FLAGS_RPGMODELFLAGS)
	{
		for (iModel = 0; iModel < MaxNumModels; iModel++)
			pCD->m_Item0_Model.SetFlags(iModel, pCD->GetItemModelFlag(0, iModel));
	}
	if (pCD->m_Item1_Flags & RPG_ITEM_FLAGS_RPGMODELFLAGS)
	{
		for (iModel = 0; iModel < MaxNumModels; iModel++)
			pCD->m_Item1_Model.SetFlags(iModel, pCD->GetItemModelFlag(1, iModel));
	}
	
	int Char_PhysType = Char_GetPhysType(_pObj);
	bool bLocalPlayer = IsLocalPlayer(_pObj, _pWClient);
	if(bLocalPlayer)
	{
		MSCOPE(LocalPlayer, LocalPlayerMem);

		pCD->m_Cam_LastBob = pCD->m_Cam_Bob;
		pCD->m_Cam_LastBobMove = pCD->m_Cam_BobMove;
		pCD->m_Cam_LastBobWeap = pCD->m_Cam_BobWeap;
		pCD->m_LastStepUpDamper = pCD->m_StepUpDamper;

		Moderatef(pCD->m_StepUpDamper, 0, pCD->m_StepUpDamperPrim, PLAYER_STEPUPDAMPING);
//		fp32 TargetZ = pCD->m_Cam_Kneel + Char_GetCameraPos(_pObj)[2]; // Removed by Mondelore.
		fp32 TargetZ = pCD->m_Cam_Kneel; // Added by Mondelore.
//		fp32 TargetZ = 0; // Added by Mondelore.

		// Camera bobbing.
		if (!(Char_PhysType == PLAYER_PHYS_NOCLIP) &&
			!(Char_PhysType == PLAYER_PHYS_DEAD))
		{
			CWObject_Client* pObj = _pObj;
			CMat4Dfp32 Mat;
			Mat.Unit();
//			fp32 Time = pCD->m_GameTime;
			CMTime Time = CMTime::CreateFromTicks(pCD->m_GameTick, _pWClient->GetGameTickTime(), pCD->m_PredictFrameFrac);

			fp32 fTime = Time.GetTimeModulus(1.0f);

			fp32 SpeedZ = pObj->GetMoveVelocity() * CVec3Dfp32::GetMatrixRow(pObj->GetPositionMatrix(), 0) * 0.1f;
			fp32 SpeedX = pObj->GetMoveVelocity() * CVec3Dfp32::GetMatrixRow(pObj->GetPositionMatrix(), 1) * 0.1f;
			Mat.RotZ_x_M(-SpeedX*0.005f);
			Mat.RotX_x_M(Max(Abs(SpeedX), Abs(SpeedZ))*0.0010f*M_Cos(fTime*(_PI*4.0f)));

			Moderatef(pCD->m_Cam_BobMove[2], TargetZ, pCD->m_Cam_dKneel, PLAYER_KNEELDAMPING);
			pCD->m_Cam_Kneel = Min(0.0f, pCD->m_Cam_Kneel+2.0f);

			fp32 BobV = Max(Abs(SpeedX), Abs(SpeedZ));
			pCD->m_Cam_BobWeap = CVec3Dfp32(0, BobV*0.25f*M_Cos(fTime*(_PI*2.0f)), BobV*0.151f*M_Sin(fTime*(_PI*4.0f)));

			pCD->m_Cam_Bob.Create(Mat);
			pCD->m_Cam_BobValid = true;
		}
		else
		{
			pCD->m_Cam_Bob.Unit();
			pCD->m_Cam_BobMove = CVec3Dfp32(0,0, TargetZ);
			pCD->m_Cam_BobWeap = 0;
		}

		// Camera filter
		if ((Char_PhysType == PLAYER_PHYS_NOCLIP) && _pWClient->GetRecMode() == 2)
		{
			if (!pCD->m_Cam_Valid)
			{
				pCD->m_Cam_Rot.Create(_pObj->GetPositionMatrix());
				pCD->m_Cam_LastRot = pCD->m_Cam_Rot;
				pCD->m_Cam_LastPos = _pObj->GetPosition();
				pCD->m_Cam_Pos = _pObj->GetPosition();
				pCD->m_Cam_dRot.k[0] = 0;
				pCD->m_Cam_dRot.k[1] = 0;
				pCD->m_Cam_dRot.k[2] = 0;
				pCD->m_Cam_dRot.k[3] = 0;
				pCD->m_Cam_dPos = 0;
				pCD->m_Cam_Valid = true;
			}

			pCD->m_Cam_LastRot = pCD->m_Cam_Rot;
			pCD->m_Cam_LastPos = pCD->m_Cam_Pos;
			CQuatfp32 Q;
			Q.Create(_pObj->GetPositionMatrix());

			if (Q.DotProd(pCD->m_Cam_Rot) < 0.0f)
			{
				Q.k[0] = -Q.k[0];
				Q.k[1] = -Q.k[1];
				Q.k[2] = -Q.k[2];
				Q.k[3] = -Q.k[3];
			}

			CRegistry* pUserReg = _pWClient->Registry_GetUser();
			int RotFilter = 128;
			int MoveFilter = 128;
			if( pUserReg )
			{
				RotFilter = pUserReg->GetValuei("DEMOFILTERROT", 128, 0);
				MoveFilter = pUserReg->GetValuei("DEMOFILTERMOVE", 128, 0);
			}

			ModerateQuat(pCD->m_Cam_Rot, Q, pCD->m_Cam_dRot, RotFilter);
			ModerateVec3f(pCD->m_Cam_Pos, _pObj->GetPosition(), pCD->m_Cam_dPos, MoveFilter);
		}
	}

	{
//		MSCOPE(RefreshItemAnimation, RefreshItemAnimationMem);
		// Fix to make clientmessage OnGetAnimstate work correctly
		// (why is PredictFrameFrac != 0 when we are in a ClientRefresh?)
		fp32 Old = pCD->m_PredictFrameFrac;
		pCD->m_PredictFrameFrac = 0;

		// Evolve model instances on all attached models
		pCD->m_Item0_Model.Refresh(_pObj, _pWClient, pCD->m_GameTick);
		pCD->m_Item1_Model.Refresh(_pObj, _pWClient, pCD->m_GameTick);
		pCD->m_Item2_Model.Refresh(_pObj, _pWClient, pCD->m_GameTick);

		pCD->m_PredictFrameFrac = Old;
	}

	if(!pCD->m_bIsPredicting)
	{
		if(_pObj->m_ClientData[0] != 0)
		{
			const int Mask = 7;
			CVec3Dfp32 MouthPos = Char_GetMouthPos(_pObj);
			int Tick = (pCD->m_GameTick + _pObj->m_iObject) & Mask;
			if(Tick == 0)
			{
				// Update occlusion
				CMat4Dfp32 Mat;
				_pWClient->Render_GetLastRenderCamera(Mat);
				uint8 Res = _pWClient->Phys_IntersectLine(CVec3Dfp32::GetRow(Mat, 3), MouthPos, 0, OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, NULL);
				pCD->m_Occlusion = (pCD->m_Occlusion << 8) | Res;
			}
			fp32 Occ1 = (pCD->m_Occlusion >> 8);
			fp32 Occ2 = pCD->m_Occlusion & 255;
			fp32 Occ = Occ1 + (Occ2 - Occ1) * Tick / Mask;
			Voice_Refresh(_pObj, _pWClient, MouthPos, 0.0f, Occ);
		}
	}

#ifdef LOG_PREDICT2
ConOutL(CStrF("(Char_EvolveClientEffects) With (%.8x), CD %.8x, Z %f", _pObj, pCD, pCD->m_Cam_BobMove[2]));
#endif
}
