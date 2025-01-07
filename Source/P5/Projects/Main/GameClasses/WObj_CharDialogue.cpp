#include "PCH.h"
#include "WObj_Char.h"
#include "../../../Shared/MOS/Classes/GameWorld/WDataRes_Sound.h"
#include "../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_DialogueCamera.h"
#include "WObj_CharDialogue.h"
#include "WObj_AI/AICore.h"
#include "WObj_AI/AI_ResourceHandler.h"
#include "WRPG/WRPGChar.h"
#include "../../../Shared/Mos/Classes/GameWorld/WObjects/WObj_Game.h"
#include "WObj_Misc/WObj_ActionCutscene.h"

/* // MultiLog writes to game console, log file and debugger output
static void M_ARGLISTCALL MultiLog(const char* _pStr, ...)
{
	char lBuffer[1000];
	va_list arg;
	va_start(arg, _pStr);
	CStrBase::vsnprintf((char*) &lBuffer[0], sizeof(lBuffer)-1, _pStr, arg); 
	lBuffer[sizeof(lBuffer) - 1] = 0;

	ConOutL(lBuffer);
	M_TRACEALWAYS("%s\n", lBuffer);
}*/
#define DO_IF(x) (!(x)) ? (void)0 :
#define DBG_OUT_LOG DO_IF(0) M_TRACEALWAYS			//MultiLog
#define DBG_OUT DO_IF(0) M_TRACEALWAYS


#define PLAYER_CLIENTFLAGS_DIALOGUECOMBO (PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK | PLAYER_CLIENTFLAGS_DIALOGUE)



static M_INLINE fp32 Dot2(const CVec3Dfp32& a, const CVec3Dfp32& b)
{
	return (a.k[0] * b.k[0] + a.k[1] * b.k[1]) / (M_Sqrt(Sqr(a.k[0])+Sqr(a.k[1])) * M_Sqrt(Sqr(b.k[0])+Sqr(b.k[1])));
}




bool g_bSkippable = false;

void CWObject_Character::Char_SpawnDialogue(int _CameraObject, bool _bInstant)
{
}

bool CWObject_Character::Char_BeginDialogue(int _iSpeaker, int _iStartItem)
{
	CWObject_Character* pSpeaker = (CWObject_Character *)m_pWServer->Object_Get(_iSpeaker);
	if (!pSpeaker)
		return false;

	CWO_Character_ClientData* pSpeakerCD = GetClientData(pSpeaker);
	CWO_Character_ClientData* pCD = GetClientData(this);
	if (!(pSpeakerCD && pCD))
		return false;

	// Notify camera that this the first frame
	//pCD->m_bFirstFrame = true;

	//M_TRACEALWAYS(CStrF("%i. BeginDialogue\n", m_pWServer->GetGameTick()));
	//ClientFlags() |= PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK | PLAYER_CLIENTFLAGS_DIALOGUE;
	
	UpdateVisibilityFlag();
	
	// Make sure the player does not try to move or look around
	// AI characters are allowed to do so though, but only under script-control, i.e. not when behaviour controlled
	m_spAI->PauseBehaviour();		

	pCD->m_iDialogueTargetObject = (int32)_iSpeaker;

	// Calc temporary player position
	CVec3Dfp32 Pos = pSpeaker->GetPosition() + (GetPosition() - pSpeaker->GetPosition()).Normalize() * PLAYER_DIALOGUE_DISTANCE;

	CMat4Dfp32 SpeakerMat = pSpeaker->GetPositionMatrix();
	CVec3Dfp32 Dir = Pos - CVec3Dfp32::GetRow(SpeakerMat, 3);

	//  Make speaker look at Player
	/*if(!(pSpeaker->m_Flags & PLAYER_FLAGS_NODIALOGUETURN))
	{
		Dir.SetMatrixRow(SpeakerMat, 0);
		SpeakerMat.RecreateMatrix(0, 2);
		m_pWServer->Object_SetRotation(pSpeaker->m_iObject, SpeakerMat);
	}*/

	// Make Player look at speaker
	CMat4Dfp32 PlayerMat;
	PlayerMat.Unit();
	(-Dir).SetMatrixRow(PlayerMat, 0);
	PlayerMat.RecreateMatrix(0, 2);
	Pos.SetRow(PlayerMat, 3);

	// Test if the playerpos is ok
	TSelection<CSelection::LARGE_BUFFER> Selection;
	
	// Use physbox from current physstate...
	CWO_PhysicsState PhysState = GetPhysState();
	/*	PhysState.AddPhysicsPrim(0, CWO_PhysicsPrim(OBJECT_PRIMTYPE_BOX, 0, 
	CVec3Dfp32(_Width*0.5f, _Width*0.5f, LEDGETYPE_HEIGHT_MEDIUM*0.5f), 0, 1.0f));*/
	PhysState.m_nPrim = 1;
	PhysState.m_ObjectFlags = OBJECT_FLAGS_PROJECTILE; //OBJECT_FLAGS_CHARACTER;
	PhysState.m_MediumFlags = XW_MEDIUM_SOLID|XW_MEDIUM_PHYSSOLID; //XW_MEDIUM_PLAYERSOLID;
	//PhysState.m_PhysFlags = 0;
	// Intersect doors and doors/platforms
	PhysState.m_ObjectIntersectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL|OBJECT_FLAGS_PHYSOBJECT;
	PhysState.m_iExclude = m_iObject;

	m_pWServer->Selection_AddIntersection(Selection, PlayerMat, PhysState);

	const int16* pSel;
	int nSel = m_pWServer->Selection_Get(Selection, &pSel);
	if (nSel > 0)
	{
		//ConOut(CStr("Something's in the way of character dialog position"));
		// Just make new matrix with current position and given direction
		PlayerMat.Unit();
		(-Dir).SetMatrixRow(PlayerMat, 0);
		PlayerMat.RecreateMatrix(0, 2);
		GetPosition().SetRow(PlayerMat, 3);
		pCD->m_DialoguePlayerPos = GetPositionMatrix();
	}

	pCD->m_DialoguePlayerPos = PlayerMat;

	// Initialize dialog camera
	
//	_pWServer->Object_SetRotation(m_iObject, PlayerMat);

	// Open Window
/*	CWObject_Message Msg(OBJMSG_GAME_SETCLIENTWINDOW, aint("dialogue"), m_pWServer->Game_GetObject()->Player_GetClient(pCD->m_iPlayer));
	m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());*/

	if (_iStartItem != 0)
	{
		OnMessage(CWObject_Message(OBJMSG_CHAR_SETDIALOGUETOKENHOLDER, m_iObject, 0, m_iObject));
		PlayDialogue_Hash(_iStartItem, DIALOGUEFLAGS_FROMLINK, 0);
	}

	// Pausing AI should not be relevant in the Darkness semi dialogue mode
	// *** Is this what we want Jesse? ***
	//PauseAllAI(CAI_ResourceHandler::PAUSE_DIALOGUE);
	
	pCD->m_SoundLevel.Set(0, pCD->m_GameTick, 0);
	pCD->m_SoundLevel.MakeDirty();

	return true;
}

bool CWObject_Character::Char_EndDialogue()
{
	ClientFlags() &= ~(PLAYER_CLIENTFLAGS_NOMOVE | PLAYER_CLIENTFLAGS_NOLOOK | PLAYER_CLIENTFLAGS_DIALOGUE);
	//M_TRACEALWAYS(CStrF("%i. EndDialogue\n", m_pWServer->GetGameTick()));
	UpdateVisibilityFlag();
	Char_SetControlMode(this, PLAYER_CONTROLMODE_FREE);

	CWO_Character_ClientData *pCD = GetClientData(this);
	if(!pCD)
		return false;
	
	//Allow behaviour controlled bots to act again
	m_spAI->ReleaseBehaviour();

	CWObject_Message Msg(OBJMSG_GAME_SETCLIENTWINDOW, 0, m_pWServer->Game_GetObject()->Player_GetClient(pCD->m_iPlayer));
	m_pWServer->Message_SendToObject(Msg, m_pWServer->Game_GetObjectIndex());

	// Make sure we no longer have a dialogue target
	pCD->m_iDialogueTargetObject = 0;
	m_liDialogueChoices.SetLen(0);
	{
		CNetMsg Msg(PLAYER_NETMSG_SETDIALOGUECHOICES);
		Msg.AddInt8(0); 
		m_pWServer->NetMsg_SendToObject(Msg, m_iObject, 1000000);
	}

	// Invalidate token
	//pCD->m_PlayerDialogueToken.Clear();
	
	CNetMsg NetMsg(PLAYER_NETMSG_CLEARTOKEN);
	m_pWServer->NetMsg_SendToObject(NetMsg, m_iObject);

	Char_EndTimeLeap();

	UnpauseAllAI(CAI_ResourceHandler::PAUSE_DIALOGUE);
	{
		CRPG_Object_Item* pEquippedItem = Char()->GetFinalSelectedItem(AG2_ITEMSLOT_WEAPONS);
		int32 ItemClass = pCD->m_EquippedItemClass;
		if (pEquippedItem && (ItemClass != pEquippedItem->m_AnimProperty))
		{
			pCD->m_EquippedItemClass = (uint16)pEquippedItem->m_AnimProperty;
			pCD->m_EquippedItemType = (uint16)pEquippedItem->m_iItemType;
			pCD->m_AnimGraph2.SetPropertyInt(PROPERTY_INT_ITEMANIMTYPE,pEquippedItem->m_AnimType);
		}	
	}

	pCD->m_SoundLevel.Set(255, pCD->m_GameTick, 0);
	pCD->m_SoundLevel.MakeDirty();

	// Leave third-person mode
	uint8 Mode = (pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK);
	//pCD->m_3PI_Mode = Mode | THIRDPERSONINTERACTIVE_STATE_LEAVING;
	pCD->m_3PI_Mode = THIRDPERSONINTERACTIVE_MODE_NONE;

	// Clear old dialogue token
	//pCD->m_PlayerDialogueToken.Clear();
	//if(pCD->m_pCurrentDialogueToken)
		//pCD->m_pCurrentDialogueToken->Clear();

	return false;
}

bool CWObject_Character::IsValidDialogueCamera(CWorld_Client* _pWClient,
											   CMat4Dfp32& _PrevCamera,
											   CMat4Dfp32& _NewCamera,
											   int _iSpeakerObj,
											   int _iListenerObj,
											   float _FOV)
{
	// Collision
	CVec3Dfp32 Start, Stop;
	CCollisionInfo CInfo;
	CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
	CInfo.SetReturnValues(0);

	int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
	int32 ObjectFlags = OBJECT_FLAGS_WORLD;// | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_CHARACTER;
	int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;

	// Previous matrix was unit matrix so we know that this is the first GetCameraAt called since dialogue begun
	CWObject_Client* pSpeakerObj = _pWClient->Object_Get(_iSpeakerObj);
	CWObject_Client* pListenerObj = _pWClient->Object_Get(_iListenerObj);
	if (!(pSpeakerObj && pListenerObj)) return false;

	CWO_Character_ClientData* pSpeakerCD = CWObject_Character::GetClientData(pSpeakerObj);
	CWO_Character_ClientData* pListenerCD = CWObject_Character::GetClientData(pListenerObj);
	if (!(pSpeakerCD && pListenerCD)) return false;
	
	//
	// Make sure the camera is seen from the listener (so the camera isn't inside a volume)
	//
	Start = pListenerObj->GetPosition() + pListenerCD->m_Camera_StandHeadOffset;
	Stop = CVec3Dfp32::GetMatrixRow(_NewCamera, 3);
	if (_pWClient->Phys_IntersectLine(Start, Stop, OwnFlags, ObjectFlags, MediumFlags, &CInfo, _iListenerObj))
	{
		// Collision. This camera is invalid
		_pWClient->Debug_RenderWire(Start, Stop, 0xffff0000, 10.0f);
		return false;
	}
	else
	{
		_pWClient->Debug_RenderWire(Start, Stop, 0xff00ff00, 10.0f);
	}


	//
	// Make sure the camera doesn't try to move inside a volume
	//
	Start = CVec3Dfp32::GetMatrixRow(_PrevCamera, 3);
	Stop = CVec3Dfp32::GetMatrixRow(_NewCamera, 3);
	if (_pWClient->Phys_IntersectLine(Start, Stop, OwnFlags, ObjectFlags, MediumFlags, &CInfo))
	{
		_pWClient->Debug_RenderWire(Start,Stop,0xffff00f0,20.0f,false);
	}
	else
	{
		// No collision
		_pWClient->Debug_RenderWire(Start,Stop,0xff00fff0,20.0f,false);
	}

	//
	// Make sure both speaker and listener are in camera view
	//
	CVec3Dfp32 p = pListenerObj->GetPosition() + pListenerCD->m_Camera_StandHeadOffset;
	if (!CWO_DialogueCamera::IsInView(_NewCamera, p, _FOV, 1.0f))
	{
		// Listener was not in view
//		ConOut(CStr("Listener was not in view"));
	};

	return true;
}

/*
#define DEFAULT_HEADOFFSET (56.0f)
#define HEAD_NECK_OFFSET (5.0f)
fp32 GetHeadOffset(CWorld_Client* _pWClient, int _iObj,const CMat4Dfp32& _Position, fp32 _IPTime)
{
	CWObject_CoreData* pObj = _pWClient->Object_GetCD(_iObj);
	if (!pObj)
		return DEFAULT_HEADOFFSET;
	// Find skeleton and evalutate current headoffset
	CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(pObj->m_iModel[0]);
	CXR_Model* pModel1 = _pWClient->GetMapData()->GetResource_Model(pObj->m_iModel[1]);
	CXR_Skeleton *pSkel1 = pModel1 ? pModel1->GetSkeleton() : NULL;
	CXR_Skeleton *pSkel0 = pModel ? pModel->GetSkeleton() : NULL;

	CXR_Skeleton *pSkel = pSkel1;
	if(!pSkel || (pSkel0 && pSkel0->m_lNodes.Len() > pSkel->m_lNodes.Len()))
		pSkel = pSkel0;

	// Default if no skeleton exists
	if (!pSkel)
		return 56.0f;

	CXR_AnimState Anim;
	if (!CWObject_Character::OnGetAnimState(pObj, _pWClient, pSkel, 1, _Position, _IPTime, Anim, NULL))
		return DEFAULT_HEADOFFSET;

	CVec3Dfp32 Pos(0,0,DEFAULT_HEADOFFSET);
	if(PLAYER_ROTTRACK_HEAD < Anim.m_pSkeletonInst->m_nBoneTransform)
	{
		CMat43fp32 Trans;
		Trans = Anim.m_pSkeletonInst->GetBoneTransform(PLAYER_ROTTRACK_HEAD);
		Pos = pSkel->m_lNodes[PLAYER_ROTTRACK_HEAD].m_LocalCenter;
		Pos *= Trans;
	}

	//_pWClient->Debug_RenderSkeleton(pSkel,Anim.m_pSkeletonInst);

	return Pos.k[2] + HEAD_NECK_OFFSET - CVec3Dfp32::GetMatrixRow(_Position,3).k[2];
}*/


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:			Get desired camera for client

Parameters:			
_pCD:				Character data
_pWClient:			Client
_iSpeakerObj:		Speaker index
_iListenerObj:	    Listener index
_Camera:			Camera to set
_IPTime:			Time
_bSwappedSpeaker:	Have we swapped speaker?

Returns: Nothing.			
\*____________________________________________________________________*/

/*
void CWObject_Character::GetDialogueCamera_Client(CWO_Character_ClientData* _pCD, CWorld_Client* _pWClient, int _iSpeakerObj,
												  int _iListenerObj, CMat4Dfp32& _Camera, float _IPTime, bool _bSwappedSpeaker)*/

void CWObject_Character::GetDialogueCamera_Client(CWO_Character_ClientData* _pCD, CWorld_Client* _pWClient, const CDialogueInstance* _pDialogueInst, 
												  const int *_piObjectIndices, CMat4Dfp32& _Camera, float _IPTime, bool _bSwappedSpeaker)
{
	const int nCameras = 3;
	static const int CameraPriorityList[nCameras] =
	{
		DIALOGUECAMERA_MODE_DEFAULT_TWOSHOT,
		DIALOGUECAMERA_MODE_THIRDPERSON_NOCLIP,
		DIALOGUECAMERA_MODE_BOUNDING_BOX,
	};

	int iCameraToTest = 0;

	if (!(_piObjectIndices[0] && _piObjectIndices[1])) 
		return;

	bool bFirstTry = true;
	
	CMat4Dfp32 nMatrices[2];
	nMatrices[0] = _pWClient->Object_GetPositionMatrix(_piObjectIndices[0]);
	nMatrices[1] = _pWClient->Object_GetPositionMatrix(_piObjectIndices[1]);

	fp32 nHeadOffsets[2];
	nHeadOffsets[0] = GetHeadOffset(_pWClient, _piObjectIndices[0], nMatrices[0], _IPTime);
	nHeadOffsets[1] = GetHeadOffset(_pWClient, _piObjectIndices[1], nMatrices[1], _IPTime);

	bool bIsTelephone1 = _pWClient->Phys_Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_ISTELEPHONE), _piObjectIndices[0]) ? true : false;
	bool bIsTelephone2 = _pWClient->Phys_Message_SendToObject(CWObject_Message(OBJMSG_ACTIONCUTSCENE_ISTELEPHONE), _piObjectIndices[1]) ? true : false;
	bool bIsTelephone = bIsTelephone1 || bIsTelephone2;

	CMat4Dfp32 MouthPos;
	if(bIsTelephone)
	{
		int iObj = _piObjectIndices[1];
		if(!bIsTelephone1) iObj = _piObjectIndices[0];
		CWObject_CoreData *pObj = _pWClient->Object_GetCD(iObj);
		MouthPos = pObj->GetPositionMatrix();
		MouthPos.GetRow(3) = CWObject_Character::Char_GetMouthPos(pObj);
	}
	
	static bool bIsCameraScripted = false;

	CMTime Time = (_pWClient->GetGameTime() - _pCD->m_PlayerDialogueToken.m_StartGameTime) + CMTime::CreateFromSeconds(_IPTime * _pWClient->GetGameTickTime());
	
	while(1)
	{
		if(_pCD->m_PlayerDialogueToken.m_Camera_Scripted != NULL)
		{ 
			bIsCameraScripted = true;
		}
			
		bool bFirstFrameIn3PI = (_pCD->m_bFirstFrameIn3PI) ? true : false;

		// Try to set desired camera
		int Res = _pCD->m_DialogueCamera.GetCameraAt(_pWClient, &_pCD->m_PlayerDialogueToken, nMatrices, nHeadOffsets, _piObjectIndices,_Camera, Time, MouthPos, _bSwappedSpeaker, bFirstFrameIn3PI, bIsTelephone);
		//if(Res == 2)
		//	Res = _pCD->m_DialogueCamera.GetCameraAt(_pWClient, &_pCD->m_PlayerDialogueToken, nMatrices, nHeadOffsets, _piObjectIndices,_Camera, Time, _bSwappedSpeaker,bFirstFrameIn3PI);

		if(bFirstFrameIn3PI)
			_pCD->m_bFirstFrameIn3PI = 0;

		// Valid camera
		_pCD->m_bFirstFrameOfDialogue = true;
		_pCD->m_PrevDialogueCameraMatrix = _Camera;

		if(Res == 2)
		{
			_pCD->m_PlayerDialogueToken.m_CameraMode = 2; // Bounding-box camera
			_pCD->m_DialogueCamera.GetCameraAt(_pWClient, &_pCD->m_PlayerDialogueToken, nMatrices, nHeadOffsets, _piObjectIndices,_Camera, Time, MouthPos, _bSwappedSpeaker, bFirstFrameIn3PI, bIsTelephone);
		}

		if(Res)
			break;

		/*
		CMat4Dfp32 lMat[2];
		lMat[0] = MatSpeaker;
		lMat[1] = MatListener;
        
		int Res = _pCD->m_DialogueCamera.GetCameraAt(_pWClient, &_pCD->m_PlayerDialogueToken, MatSpeaker, MatListener, 
			SpeakerHeadOffset, ListenerHeadOffset,_iSpeakerObj,_iListenerObj, _Camera, Time, false, _bSwappedSpeaker, bIsCameraScripted);*/

		/*if(Res != 0)
		{
			if(!IsValidDialogueCamera(_pWClient, _pCD->m_PrevDialogueCameraMatrix, _Camera, _iSpeakerObj, _iListenerObj, _pCD->m_PlayerDialogueToken.m_FOV)
				|| Res == 2)
			{
				// Invalid camera, select another
				//			ConOut(CStrF("Dialoguecamera mode %i could not be used.", _pCD->m_DialogueCamera.GetMode()));

				int NextMode = CameraPriorityList[iCameraToTest++];
				if((NextMode == _pCD->m_PlayerDialogueToken.m_CameraMode)
					&& (bFirstTry))
				{
					// We're about to test the same mode that we've already tried and failed with. Avoid this.
					if (iCameraToTest < nCameras)
					{
						NextMode = CameraPriorityList[iCameraToTest++];
					}
					else
					{
						_Camera.Unit();
						break;
					}
				}

				_pCD->m_DialogueCamera.SetMode(NextMode);
				if (iCameraToTest < nCameras)
				{
					// Try next camera
					continue;
				}
				else
				{
					if (bFirstTry)
					{
						int iTemp = _iSpeakerObj;
						_iSpeakerObj = _iListenerObj;
						_iListenerObj = iTemp;
						bFirstTry = false;
						iCameraToTest = 0;
					}
					else
					{
						ConOut(CStr("Cannot find a valid camera!"));
						_pCD->m_DialogueCamera.SetMode(_pCD->m_PlayerDialogueToken.m_CameraMode);
						_pCD->m_DialogueCamera.GetCameraAt(_pWClient, &_pCD->m_PlayerDialogueToken, MatSpeaker, MatListener, SpeakerHeadOffset, ListenerHeadOffset,_iSpeakerObj, _iListenerObj,_Camera, Time, true);
						break;
					}
				}
			}
			else
			{
				// Valid camera
				_pCD->m_bFirstFrameOfDialogue = true;
				_pCD->m_PrevDialogueCameraMatrix = _Camera;
				break;
			}
		}
		else
		{
			// Valid camera
			_pCD->m_bFirstFrameOfDialogue = true;
			_pCD->m_PrevDialogueCameraMatrix = _Camera;
			break;
		}*/
	}
}

#define DEFAULT_HEADOFFSET (56.0f)
#define HEAD_NECK_OFFSET (5.0f)

fp32 CWObject_Character::GetHeadOffset(CWorld_Client* _pWClient, int _iObj, const CMat4Dfp32& _Position, fp32 _IPTime)
{
	CWObject_CoreData* pObj = _pWClient->Object_GetCD(_iObj);
	if (!pObj)
		return DEFAULT_HEADOFFSET;

	// Find skeleton and evaluate current head offset
	CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(pObj->m_iModel[0]);
	CXR_Model* pModel1 = _pWClient->GetMapData()->GetResource_Model(pObj->m_iModel[1]);
	CXR_Skeleton *pSkel1 = pModel1 ? pModel1->GetSkeleton() : NULL;
	CXR_Skeleton *pSkel0 = pModel ? pModel->GetSkeleton() : NULL;

	CXR_Skeleton *pSkel = pSkel1;
	if(!pSkel || (pSkel0 && pSkel0->m_lNodes.Len() > pSkel->m_lNodes.Len()))
		pSkel = pSkel0;

	// Default if no skeleton exists
	if (!pSkel)
		return 56.0f;

	CXR_AnimState Anim;
	if (!CWObject_Character::OnGetAnimState(pObj, _pWClient, pSkel, 1, _Position, _IPTime, Anim, NULL))
		return DEFAULT_HEADOFFSET;

	CVec3Dfp32 Pos(0,0,DEFAULT_HEADOFFSET);
	if(PLAYER_ROTTRACK_HEAD < Anim.m_pSkeletonInst->m_nBoneTransform)
	{
		CMat4Dfp32 Trans;
		Trans = Anim.m_pSkeletonInst->GetBoneTransform(PLAYER_ROTTRACK_HEAD);
		Pos = pSkel->m_lNodes[PLAYER_ROTTRACK_HEAD].m_LocalCenter;
		Pos *= Trans;
	}

	return Pos.k[2] + HEAD_NECK_OFFSET - CVec3Dfp32::GetMatrixRow(_Position,3).k[2];
}


fp32 CWObject_Character::GetDialogueCameraFOV_Client(CWO_Character_ClientData* _pCD, CWorld_Client* _pWClient, float _IPtime)
{
	if (_pCD)
	{
		CMTime Time = CMTime::CreateFromTicks(_pWClient->GetGameTick(), _pWClient->GetGameTickTime(), _IPtime);
		return _pCD->m_DialogueCamera.GetFOVAt(_pWClient, &_pCD->m_PlayerDialogueToken, Time);
	}
	// return _pCD->m_CameraUtil.GetFOV();
	ConOut(CStr("GetDialogueCameraFOV_Client: Failed, no valid ClientData"));
	return 90.0f;
}

bool CWObject_Character::CheckContinueDialogue()
{
	if (IS_VALID_ITEMHASH(m_InterruptedItemHash) && (m_iSpeaker == 0) && (m_iListener == 0))
	{
		PlayDialogue_Hash(m_InterruptedItemHash, 0, 0);
		m_InterruptedItemHash = 0;
		return(true);
	}
	return(false);
};


bool CWObject_Character::PlayDialogue_Hash(uint32 _DialogueHash, uint _Flags, int _Material, int _StartOffset, uint32 _SyncGroupID, int8 _AttnType)
{
	MSCOPESHORT(CWObject_Character::PlayDialogue);

	CWO_Character_ClientData *pCD = GetClientData(this);
	if(!pCD)
		return false;
	
	pCD->m_DarknessVoiceUse = 0;
	pCD->m_3PI_NoCamera = 0;

	CWRes_Dialogue* pDialogue = GetDialogueResource(this, m_pWServer);

	if (!pDialogue || !pDialogue->GetHashDialogueItem(_DialogueHash))
		return false;

	if(Char_GetPhysType(this) == PLAYER_PHYS_DEAD)
		return false;

	CWRes_Dialogue::CRefreshRes Res;
	bool bResValid = false;
	bool bStartDarknessVoice = false;
	if (!pDialogue->IsQuickSound_Hash(_DialogueHash))
	{
		if(pCD->m_pCurrentDialogueToken && !(_Flags & DIALOGUEFLAGS_FROMLINK))
			return false;

		//pDialogue->FindEvent_Hash(_iDialogue, CWRes_Dialogue::EVENTTYPE_LISTENER)

		Char_SetListener(0);

		int Prio = pDialogue->GetPriority_Hash(_DialogueHash);
		if (Prio < m_spAI->GetCurrentPriorityClass())
			return false;

		CFStr Listener;
		int iListener = 0;
		int Flags = 0;
		const char* pListener = pDialogue->FindEvent_Hash(_DialogueHash, CWRes_Dialogue::EVENTTYPE_LISTENER);
		const char* pSub = pDialogue->FindEvent_Hash(_DialogueHash, CWRes_Dialogue::EVENTTYPE_SUBTITLE);
		if(pListener)
		{
			Flags = *pListener;
			pListener++;
			Listener = pListener;
		}
		else
		{
			if(pCD->m_iPlayer == -1)
			{
				if(pSub)
				{
					uint16 Flags = *(uint16*)pSub;
					if((Flags & SUBTITLE_TYPE_MASK) == SUBTITLE_TYPE_INTERACTIVE)
						Listener = "player";
				}
			}

			if(Listener == "")
			{
				const char* pLink = pDialogue->FindEvent_Hash(_DialogueHash, CWRes_Dialogue::EVENTTYPE_LINK);
				if(pLink)
					Listener = CFStr(pLink).GetStrSep(":");
			}
		}

		if(pSub)
		{
			uint16 Flags = *(uint16*)pSub;
			uint16 Type = (Flags & SUBTITLE_TYPE_MASK);
			if ((Type == SUBTITLE_TYPE_CUTSCENE) || (Type == SUBTITLE_TYPE_CUTSCENEKEEPENDANIM))
			{
				int iSound = pDialogue->GetSoundIndex_Hash(_DialogueHash, m_pWServer->GetMapData());
				CSC_SFXDesc* pResSound = m_pWServer->GetMapData()->GetResource_SoundDesc(iSound);
				if (pResSound)
				{
					pCD->m_VoCap.Init(pCD->m_lAnim_VoCap.GetBasePtr(), pCD->m_lAnim_VoCap.Len());

					uint32 SoundNameHash = pResSound->GetNameHash();
					int iAnimRc = -1, iSeq = -1;
					CXR_Anim_SequenceData* pSeq = pCD->m_VoCap.GetSequenceFromName(m_pWServer->GetMapData(), SoundNameHash, &iAnimRc, &iSeq);
					if (pSeq)
					{
						CMat4Dfp32 OrgPos = GetPositionMatrix();

						vec128 Move;
						CQuatfp32 Rot;
						pSeq->EvalTrack0(CMTime(), Move, Rot);
						CMat4Dfp32 RotMat,Temp;
						Move = M_VSetW1(M_VNeg(Move));
						Move = M_VMul(Move, M_VLdScalar(pCD->m_CharGlobalScale * (1.0f - pCD->m_CharSkeletonScale)));
						Move = M_VSetW1(Move);
						Rot.Inverse();
						Rot.CreateMatrix(RotMat);
						OrgPos.Multiply3x3(RotMat,Temp);
						Move = M_VMulMat4x3(Move, OrgPos);
						OrgPos = Temp;
						OrgPos.r[3] = Move;

						pCD->m_AnimGraph2.SetDestinationLock(OrgPos);
						uint8 Flags = Type == SUBTITLE_TYPE_CUTSCENE ? CXRAG2_Animation::XRAG2_OVERLAYREMOVEWHENFINISHED : 0;
						CWResource* pWRes = m_pWServer->GetMapData()->GetResource(iAnimRc);
						pCD->m_AnimGraph2.GetAG2I()->SetOverlayAnim(pWRes->m_iRc, iSeq, m_pWServer->GetGameTime() + CMTime::CreateFromTicks(_StartOffset, m_pWServer->GetGameTickTime()), Flags);
					}
				}
			}
		}

		if(Listener.CompareNoCase("player") == 0)
			iListener = m_pWServer->Game_GetObject()->Player_GetObjectIndex(0);
		else if(Listener.CompareNoCase("none") != 0)
			iListener = m_pWServer->Selection_GetSingleTarget(Listener);

		if(iListener == m_iObject)
			iListener = -1;

		bool bDarkVoice = pCD->m_DarknessVoiceUse ? true : false;

		OnRefresh_Dialogue_Hash(this, m_pWServer, _DialogueHash, _Flags, &Res);

		if (iListener > 0)
		{
			CWObject_Character *pChar = TDynamicCast<CWObject_Character>(m_pWServer->Object_Get(iListener));
			if(pChar && CWObject_Character::Char_GetPhysType(pChar) == PLAYER_PHYS_DEAD)
				return false;

			Char_SetListener(iListener, Flags);
			pCD->m_DialogueInstance.m_Priority = Prio;
		}
		EvalDialogueLink(Res);

		if(!bDarkVoice && pCD->m_DarknessVoiceUse)
			bStartDarknessVoice = true;
	}

	if (bStartDarknessVoice)
		_AttnType = WCLIENT_ATTENUATION_2D_CUTSCENE;

	bool bRet = CWObject_RPG::PlayDialogue_Hash(_DialogueHash, _Flags, pCD->m_GroundMaterial, _StartOffset, _SyncGroupID, _AttnType);
	DBG_OUT_LOG("[%.2f, Char %d, %s], PlayDialogue_Hash: %08X  (result: %d)!", m_pWServer->GetGameTime().GetTime(), m_iObject, GetName(), _DialogueHash, bRet);
	return bRet;
}


bool CWObject_Character::Char_PlayDialogue_Hash(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, uint32 _DialogueItemHash, int _AttnType, bool _bNeedGroundMaterial)
{
	// Move this to CWObject_RPG::PlayDialogue somehow?

	CWO_Character_ClientData* pCD = GetClientData(_pObj);
	M_ASSERTHANDLER(pCD, "CHAR: no client data!!", return false);

	// Do we need to update groundmaterial
	if (_bNeedGroundMaterial)
	{
		UpdateGroundMaterial(_pObj, _pWClient);
		if(pCD->m_bIsPredicting) // Making sure we write to the non-predicted object also
		{
			CWObject_Client *pOrg = _pWClient->Object_GetFirstCopy(_pObj->m_iObject);
			CWO_Character_ClientData *pOrgCD = GetClientData(pOrg);
			pOrgCD->m_GroundMaterial = pCD->m_GroundMaterial;
		}
	}

	int iMaterial = pCD->m_GroundMaterial;

	uint16 iSound = 0;
	switch (_DialogueItemHash)
	{
	case MHASH1('1'):  iSound = pCD->m_Item0_AnimSound0; break;
	case MHASH1('2'):  iSound = pCD->m_Item0_AnimSound1; break;
	case MHASH1('3'):  iSound = pCD->m_Item0_AnimSound2; break;
	case MHASH1('4'):  iSound = pCD->m_Item0_AnimSound3; break;
	case MHASH1('5'):  iSound = pCD->m_AnimSound5; break;
	case MHASH1('11'): iSound = pCD->m_Item1_AnimSound0; break;
	case MHASH1('12'): iSound = pCD->m_Item1_AnimSound1; break;
	case MHASH1('13'): iSound = pCD->m_Item1_AnimSound2; break;
	case MHASH1('14'): iSound = pCD->m_Item1_AnimSound3; break;
	}

	if (iSound > 0)
	{
		_pWClient->Sound_At(WCLIENT_CHANNEL_SFX, Char_GetMouthPos(_pObj), iSound, _AttnType, iMaterial);
		return true;
	}

	return CWObject_RPG::PlayDialogue_Hash(_pObj, _pWClient, _DialogueItemHash, 0, iMaterial, _AttnType, 0);
}


int CWObject_Character::GetDialogueLength(int _iDialogue)
{
	CWRes_Dialogue* pDialogue = GetDialogueResource(this, m_pWServer);
	if (!pDialogue)
		return -1;
	
	uint32 Hash = pDialogue->IntToHash(_iDialogue);
	return GetDialogueLength_Hash(Hash);
}


int CWObject_Character::GetDialogueLength_Hash(uint32 _iDialogue)
{
	CWRes_Dialogue* pDialogue = GetDialogueResource(this, m_pWServer);
	if (!pDialogue)
		return -1;

	int SampleLength = RoundToInt(pDialogue->GetSampleLength_Hash(_iDialogue) * m_pWServer->GetGameTicksPerSecond());
	return SampleLength;
}


void CWObject_Character::OnRefresh_Dialogue(CWObject_CoreData* _pObj,
											CWorld_PhysState* _pWPhysState,
											int _iNewDialogue,
											int _Flags,
											CWRes_Dialogue::CRefreshRes *_pRes)
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if(!pCD)
		return;

	CWRes_Dialogue *pDialogue = GetDialogueResource(_pObj, _pWPhysState);
	if(!pDialogue)
		return;

	uint32 Hash = 0;
	
	if(_iNewDialogue != 0)
		Hash = pDialogue->IntToHash(_iNewDialogue);

	OnRefresh_Dialogue_Hash(_pObj, _pWPhysState, Hash, _Flags, _pRes);	
}


void CWObject_Character::OnRefresh_Dialogue_Hash(CWObject_CoreData* _pObj,
												 CWorld_PhysState* _pWPhysState,
												 uint32 _NewDialogueHash,
												 int _Flags,
												 CWRes_Dialogue::CRefreshRes *_pRes)
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if(!pCD)
		return;

	if (IS_VALID_ITEMHASH(_NewDialogueHash))
	{
		CWRes_Dialogue *pDialogue = GetDialogueResource(_pObj, _pWPhysState);
		if(pDialogue)
		{
			fp32 SampleLength = pDialogue->GetSampleLength_Hash(_NewDialogueHash);
			pCD->m_DialogueInstance.Reset(pCD->m_GameTime, _NewDialogueHash, SampleLength, _Flags);
		}
	}

	if (pCD->m_DialogueInstance.IsValid())
	{
		if (IS_VALID_ITEMHASH(_NewDialogueHash) || !(pCD->m_DialogueInstance.m_Flags & DIALOGUEFLAGS_PAUSED))
		{
			CWRes_Dialogue* pDialogue = GetDialogueResource(_pObj, _pWPhysState);
			if (pDialogue)
			{
				pDialogue->Refresh(pCD->m_GameTime, _pWPhysState->GetGameTickTime(), pCD->m_DialogueInstance, pCD->m_pCurrentDialogueToken, _pRes);
			}
		}
	}
}

void CWObject_Character::Char_SendDialogueImpulse(int32 _iImpulse)
{
	if (m_lDialogueImpulses.ValidPos(_iImpulse))
	{
		DBG_OUT_LOG("[%.2f, Char %d, %s], Sending dialogue impulse %d", m_pWServer->GetGameTime().GetTime(), m_iObject, GetName(), _iImpulse);
		m_lDialogueImpulses[_iImpulse].SendMessage(m_iObject, m_iObject, m_pWServer);
	}
}

void CWObject_Character::EvalDialogueLink(const CWRes_Dialogue::CRefreshRes &_Res)
{
	if(_Res.m_Events & (1 << CWRes_Dialogue::EVENTTYPE_IMPULSE))
	{
		int ImpulseMask = _Res.m_Impulse;
		int i = 0;
		while(ImpulseMask != 0)
		{
			if(ImpulseMask & 1)
			{
				if (m_lDialogueImpulses.Len() > i && m_lDialogueImpulses[i].IsValid())
				{
					DBG_OUT_LOG("[%.2f, Char %d, %s], Sending dialogue impulse %d", m_pWServer->GetGameTime().GetTime(), m_iObject, GetName(), i);
					m_lDialogueImpulses[i].SendMessage(m_iObject, m_iObject, m_pWServer);
				}
				else
					DBG_OUT_LOG("[%.2f, Char %d, %s], Invalid DialogueImpulse: %d", m_pWServer->GetGameTime().GetTime(), m_iObject, GetName(), i);
			}
			i++;
			ImpulseMask >>= 1;
		}
	}
	if(_Res.m_Events & (1 << CWRes_Dialogue::EVENTTYPE_MESSAGE))
	{
		CWO_SimpleMessage Msg;
		Msg.Parse(_Res.m_pMessage, m_pWServer);
		if(Msg.IsValid())
			Msg.SendMessage(m_iObject, m_iObject, m_pWServer);
	}
	if(_Res.m_Events & (1 << CWRes_Dialogue::EVENTTYPE_SPECIALVOICE))
	{
		CWO_Character_ClientData *pCD = GetClientData(this);
		if(!pCD)
			return;

		pCD->m_DarknessVoiceUse = 1;
	}
	if(_Res.m_Events & (1 << CWRes_Dialogue::EVENTTYPE_SETHOLD))
	{
		CWO_Character_ClientData *pCD = GetClientData(this);
		if(!pCD)
			return;

		pCD->m_VoCap.SetHold(m_pWServer->GetMapData(), _Res.m_SetHoldAnim, _Res.m_SetHoldBegin, _Res.m_SetHoldEnd);
	}
	if(_Res.m_Events & (1 << CWRes_Dialogue::EVENTTYPE_NOCAMERA))
	{
		CWO_Character_ClientData *pCD = GetClientData(this);
		if(!pCD)
			return;

		if(pCD->m_iPlayer == -1)
		{
			CWObject *pObj = m_pWServer->Object_Get(m_pWServer->Game_GetObject()->Player_GetObjectIndex(0));
			pCD = GetClientData(pObj);
		}

		pCD->m_3PI_NoCamera = 1;
	}
	for (int j = 0; j < CWRes_Dialogue::NUMDIALOGUEITEMS; j++)
	{
		uint Event = CWRes_Dialogue::EVENTTYPE_SETITEM_APPROACH + j;
		if (_Res.m_Events & (1 << Event))
		{
			DBG_OUT_LOG("[%.2f, Char %d, %s], SetItem (%d): %s", m_pWServer->GetGameTime().GetTime(), m_iObject, GetName(), j, _Res.m_pItems[j]);

			const static int lMessages[] = {
				OBJMSG_CHAR_SETDIALOGUEITEM_APPROACH,
				OBJMSG_CHAR_SETDIALOGUEITEM_APPROACH2,
				OBJMSG_CHAR_SETDIALOGUEITEM_THREATEN,
				OBJMSG_CHAR_SETDIALOGUEITEM_IGNORE,
				OBJMSG_CHAR_SETDIALOGUEITEM_TIMEOUT,
				OBJMSG_CHAR_SETDIALOGUEITEM_EXIT,
			};
			CFStr St = _Res.m_pItems[j];
			uint Flip = 0;
			if (St.Len() >= 1 && St[0] == '#') // Uses old syntax (SETAPPROACHITEM instead of SETITEM_APPROACH)
			{
				Flip = 1;
				St = St.CopyFrom(1);
			}
			while (St != "")
			{
				CFStr Target = St.GetStrSep(",");
				CFStr StringItem = St.GetStrSep(",");
				StringItem.Trim();

				CWObject_Message Msg(lMessages[j]);
				if (StringItem != "" && StringItem[0] == '-')
				{
					Msg.m_pData = (void*)(StringItem.Str() + 1);
					Msg.m_Param0 = Flip ^ 1;
				}
				else
				{
					Msg.m_pData = (void*)StringItem.Str();
					Msg.m_Param0 = Flip;
				}

				DBG_OUT("  EvalDialogueLink1: Target: '%s', Item: %s\n", Target.Str(), StringItem.Str());

				if (Target != "" && Target[0] == '$')
				{
					if (Target.CompareNoCase("$this") == 0)
					{	
						// Set on self
						m_pWServer->Message_SendToObject(Msg, m_iObject);
					}
					else
					{
						int iGameObject = m_pWServer->Game_GetObjectIndex();
						CWObject_Message Msg2(OBJMSG_GAME_PIPEMESSAGETOWORLD);
						Msg2.m_pData = &Msg;
						Msg2.m_Param0 = aint(Target.Str() + 1);

						if(m_pWServer->Message_SendToObject(Msg2, iGameObject))
							ConOut("Sucessfully sent world-message to: " + Target);
						else
							ConOutL("(CWObject_Character::EvalDialogueLink) Failed to send world-message to: " + Target);
					}
				}
				else
				{	
					m_pWServer->Message_SendToTarget(Msg, Target);
				}
			}
		}
	}

	if(_Res.m_Events & (1 << CWRes_Dialogue::EVENTTYPE_SETANIMPROPERTIES))
	{
		CWO_Character_ClientData *pCD = GetClientData(this);
		if(!pCD)
			return;

		CWAG2I_Context AG2IContext(this, m_pWServer, pCD->m_GameTime);
		const CWRes_Dialogue::CDialoguePropertyChange *lpChanges = _Res.m_lPropertyChanges.GetBasePtr();
		int nChanges = _Res.m_lPropertyChanges.Len();
		for(int i = 0; i < nChanges; i++)
			pCD->m_AnimGraph2.SetPackedAnimProperty(&AG2IContext, lpChanges[i].m_Property, lpChanges[i].m_Value);
	}

	bool bUseNewBuf = false;
	CFStr NewBuf;

	if(_Res.m_Events & (1 << CWRes_Dialogue::EVENTTYPE_RANDOMLINK))
	{
		CWO_Character_ClientData *pCD = GetClientData(this);
		if(!pCD)
			return;
		
		pCD->m_DarknessVoiceUse = 0;
		pCD->m_3PI_NoCamera = 0;

		int nRandoms = 0;
		CFStr St = _Res.m_pLink;

		CFStr Target = St.GetStrSep(":");
		nRandoms = St.GetStrSep(":").Val_int();

		if(nRandoms && nRandoms < 64)
		{
			int iChoice = MRTC_RAND() % nRandoms;
			CFStr Choice;
			for(int i = 0; i < iChoice; i++)
				St.GetStrSep(",");
			bUseNewBuf = true;
			NewBuf = CFStrF("%s:%s", Target.Str(), St.GetStrSep(",").Str());
		}

		if(Target.CompareNoCase("player") == 0)
		{
			int iTarget = m_pWServer->Game_GetObject()->Player_GetObjectIndex(0);
			if(iTarget == m_iObject && nRandoms)
			{
				NewBuf.GetStrSep(":");

				uint32 Hash = StringToHash(NewBuf.Str());
				PlayDialogue_Hash(Hash, 0, 0, 0);

				bUseNewBuf = false;
			}
		}
	}

	if(_Res.m_Events & (1 << CWRes_Dialogue::EVENTTYPE_LINK) || bUseNewBuf)
	{
		CWO_Character_ClientData *pCD = GetClientData(this);
		if(!pCD)
			return;

		pCD->m_DarknessVoiceUse = 0;
		pCD->m_3PI_NoCamera = 0;

		//Char_SetListener(0); //can we disable this? - don't want the AI to wander off while we're talking

		if(pCD->m_pCurrentDialogueToken)
			Char_EndTimeLeap();

		const int MaxOptions = 16;
		if(aint(_Res.m_pLink) == -1)
		{
			Char_SetListener(0);
			if(pCD->m_pCurrentDialogueToken)
			{
				int iOwner = pCD->m_pCurrentDialogueToken->m_iOwner;
				pCD->m_pCurrentDialogueToken = NULL;
				// Reached end of item and we are still holding the token, Abort Dialouge
				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETDIALOGUETOKENHOLDER, -1, 0, m_iObject), iOwner);
			}
		}
		else
		{
			CFStr Targets[MaxOptions];
			CFStr Items[MaxOptions];
			int nTargets = 0;
			CFStr St;
			if(!bUseNewBuf)
				St = _Res.m_pLink;
			else
				St = NewBuf;

			DBG_OUT_LOG("[%.2f, Char %d, %s], Link: %s", m_pWServer->GetGameTime().GetTime(), m_iObject, GetName(), St.Str());

			CFStr Cmd;
			CFStr Param;
			bool bInvert = false;

			while(St != "")
			{
				Targets[nTargets] = St.GetStrSep(":");
				Items[nTargets] = St.GetStrSep("#");

				if(St != "")
				{
					Cmd = St.GetStrSep(":");
					Param = St.GetStrSep("|");

					if(Cmd != "" && Cmd[0] == '!')
					{
						Cmd = Cmd.Copy(1, 1024);
						bInvert = true;
					}
				}

				nTargets++;
				St = "";
				if(nTargets == MaxOptions)
					break;
			}

			DBG_OUT("  EvalDialogueLink2: nTargets = %d\n", nTargets);
			for (uint i = 0; i < nTargets; i++)
				DBG_OUT("   - %s, %s\n", Targets[i].Str(), Items[i].Str());


			if(Cmd.CompareNoCase("POwns") == 0)
			{
				int iTarget = m_pWServer->Game_GetObject()->Player_GetObjectIndex(0);
				int ItemType = Param.GetStrSep(":").Val_int();
				CRPG_Object_Item *pItem = (CRPG_Object_Item *)m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETITEM, ItemType), iTarget);
				if((!pItem || pItem->m_NumItems < Param.Val_int()) ^ bInvert)
				{
					return;
				}
			}
			else if(Cmd.CompareNoCase("ParamEquals") == 0)
			{
				CFStr Target = Param.GetStrSep(":");
				int TargetParam = Param.Val_int();
				CWObject *pTarget;

				if(Target == "$player")
					pTarget = m_pWServer->Object_Get(m_pWServer->Game_GetObject()->Player_GetObjectIndex(0));
				else if(Target == "$this")
					pTarget = this;
				else
					pTarget = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget(Target));
				int Param = pTarget ? pTarget->m_Param : 0;
				if((Param != TargetParam) ^ bInvert)
				{
					return;
				}
			}
			else if(Cmd.CompareNoCase("C") == 0)
			{
				CWO_SimpleMessage Msg;
				CFStr St = Param.GetStrSep(":");
				Msg.Parse(St, m_pWServer);
				if(!Msg.SendMessage(m_iObject, m_iObject, m_pWServer))
				{
					return;
				}
			}

			int iSel = MRTC_RAND() % nTargets;
			int iTarget = 0;
			if(Targets[iSel].CompareNoCase("player") == 0)
			{
				iTarget = m_pWServer->Game_GetObject()->Player_GetObjectIndex(0);
				pCD->m_liDialogueChoice.SetLen(0);
				CFStr St = Items[iSel].Str();
				while(St != "")
				{
					const char* Sp = St.Str();
					CFStr ItemStr = St.GetStrSep(",");

					CDialogueLink Link = Char_ParseDialogueChoice(ItemStr, m_iObject, iTarget);
					if (Link.IsValid())
					{
						pCD->m_liDialogueChoice.Add(Link);
						if (pCD->m_liDialogueChoice.Len() == pCD->m_liDialogueChoice.GetMax())
							break;
					}
				}
				pCD->m_liDialogueChoice.MakeDirty();
				pCD->m_DialogueChoiceTick = pCD->m_GameTick;
				return;
			}
			else if(Targets[iSel].CompareNoCase("$this") == 0)
				iTarget = m_iObject;
			else if(Targets[iSel].CompareNoCase("$phone") == 0)
				iTarget = m_pWServer->Selection_GetSingleTarget("TELEPHONEREG");
			else
				iTarget = m_pWServer->Selection_GetSingleTarget(Targets[iSel]);

			Char_SetListener(0);
			if(iTarget > 0)
			{
				// Set new token holder
				int iOwner = 0;
				if(pCD->m_pCurrentDialogueToken)
				{
					iOwner = pCD->m_pCurrentDialogueToken->m_iOwner;
					pCD->m_pCurrentDialogueToken = NULL;
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETDIALOGUETOKENHOLDER, iTarget, 0, m_iObject), iOwner);
				}
				
				if(!m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETDIALOGUECHOICES, aint(Items[iSel].Str()), iOwner, m_iObject), iTarget))
				{
					// Failed to set Dialogue choices, Abort dialogue
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETDIALOGUETOKENHOLDER, -1, 0, m_iObject), iOwner);
				}
			}
			else
			{
				if(pCD->m_pCurrentDialogueToken)
				{
					int iOwner = pCD->m_pCurrentDialogueToken->m_iOwner;
					pCD->m_pCurrentDialogueToken = NULL;
					// Link target not found, Abort Dialouge
					m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETDIALOGUETOKENHOLDER, -1, 0, m_iObject), iOwner);
				}
			}
		}
	}
}

void CWObject_Character::EvalDialogueLink_Client(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, const CWRes_Dialogue::CRefreshRes &_Res)
{
	if(_Res.m_Events & (1 << CWRes_Dialogue::EVENTTYPE_SUBTITLE))
	{
 		CWO_Character_ClientData *pCD = GetClientData(_pObj);
		if(pCD && pCD->m_DialogueInstance.m_Subtitle != "")
		{
			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
			bool bOk = false;
			bool bBattle = false;
			if(pSys && pSys->GetOptions())
			{
				switch(pCD->m_DialogueInstance.m_SubtitleFlags & SUBTITLE_TYPE_MASK)
				{
					case SUBTITLE_TYPE_IDLE: bOk = pSys->GetOptions()->GetValuei("GAME_SUBTITLE_CASUAL", 0) != 0; break;
					case SUBTITLE_TYPE_INTERACTIVE: bOk = pSys->GetOptions()->GetValuei("GAME_SUBTITLE_INTERACTIVE", 0) != 0; break;
					case SUBTITLE_TYPE_AI: bOk = pSys->GetOptions()->GetValuei("GAME_SUBTITLE_FIGHTING", 0) != 0; bBattle = true; break;
					case SUBTITLE_TYPE_CUTSCENE: bOk = pSys->GetOptions()->GetValuei("GAME_SUBTITLE_CUTSCENE", 0) != 0; break;
				}
			}

			if(bOk)
			{
				CWRes_Dialogue *pDialogue = GetDialogueResource(_pObj, _pWPhysState);
				CWObject_Message Msg(OBJMSG_GAME_ADDSUBTITLE, (int)(pDialogue->m_SubtitleRange * (bBattle ? 3.5f : 1)));
				Msg.m_iSender = _pObj->m_iObject;
				_pWPhysState->Phys_Message_SendToObject(Msg, _pWPhysState->Game_GetObjectIndex());
			}
			else
			{
				// Clear subtitle
				CWObject_Message Msg(OBJMSG_GAME_ADDSUBTITLE);
				Msg.m_iSender = _pObj->m_iObject;
				_pWPhysState->Phys_Message_SendToObject(Msg, _pWPhysState->Game_GetObjectIndex());
			}
		}
		else
		{
			CWObject_Message Msg(OBJMSG_GAME_ADDSUBTITLE);
			Msg.m_iSender = _pObj->m_iObject;
			_pWPhysState->Phys_Message_SendToObject(Msg, _pWPhysState->Game_GetObjectIndex());
		}
	}

	if(_Res.m_Events & (1 << CWRes_Dialogue::EVENTTYPE_SETHOLD))
	{
		CWO_Character_ClientData *pCD = GetClientData(_pObj);
		if(!pCD)
			return;

		pCD->m_VoCap.SetHold(_pWPhysState->GetMapData(), _Res.m_SetHoldAnim, _Res.m_SetHoldBegin, _Res.m_SetHoldEnd);
	}

	/*if(_Res.m_Events & (1 << CWRes_Dialogue::EVENTTYPE_SETANIMPROPERTIES))
	{
		CWO_Character_ClientData *pCD = GetClientData(_pObj);
		if(!pCD)
			return;

		CWAG2I_Context AG2IContext(_pObj, _pWPhysState, pCD->m_GameTime);
		TAP<CWRes_Dialogue::CDialoguePropertyChange> lProperties = (TThinArray<CWRes_Dialogue::CDialoguePropertyChange>)_Res.m_lPropertyChanges; // const casting
		for(int i = 0; i < lProperties.Len(); i++)
			pCD->m_AnimGraph2.SetPackedAnimProperty(&AG2IContext, lProperties[i].m_Property, lProperties[i].m_Value);
	}*/
}


CDialogueLink CWObject_Character::Char_ParseDialogueChoice(CFStr&_St, int _iSender, int _iOwner)
{
	CDialogueLink ReturnLink;
	ReturnLink.Set(_St.GetStrSep("|"), false);

	while (_St != "")
	{
		CFStr Cmd = _St.GetStrSep(":");
		CFStr Param = _St.GetStrSep("|");
		bool bInvert = false;
		if(Cmd != "" && Cmd[0] == '!')
		{
			Cmd = Cmd.Copy(1, 1024);
			bInvert = true;
		}
		if(Cmd.CompareNoCase("Owns") == 0 || Cmd.CompareNoCase("POwns") == 0)
		{
			int iTarget = _iSender;
			if(Cmd.CompareNoCase("POwns") == 0)
				iTarget = _iOwner;
			int ItemType = Param.GetStrSep(":").Val_int();
			CRPG_Object_Item *pItem = (CRPG_Object_Item *)m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_GETITEM, ItemType), iTarget);
			if((!pItem || pItem->m_NumItems < Param.Val_int()) ^ bInvert)
			{
				ReturnLink.m_ItemHash = 0;
				break;
			}
		}
		else if(Cmd.CompareNoCase("ParamEquals") == 0)
		{
			CFStr Target = Param.GetStrSep(":");
			int TargetParam = Param.Val_int();
			CWObject *pTarget;

			if(Target == "$player")
				pTarget = m_pWServer->Object_Get(_iOwner);
			else if(Target == "$this")
				pTarget = this;
			else
				pTarget = m_pWServer->Object_Get(m_pWServer->Selection_GetSingleTarget(Target));
			int Param = pTarget ? pTarget->m_Param : 0;
			if((Param != TargetParam) ^ bInvert)
			{
				ReturnLink.m_ItemHash = 0;
				break;
			}
		}
		else if(Cmd.CompareNoCase("C") == 0)
		{
			CWO_SimpleMessage Msg;
			CFStr St = Param.GetStrSep(":");
			Msg.Parse(St, m_pWServer);
			if(!Msg.SendMessage(m_iObject, _iSender, m_pWServer))
			{
				ReturnLink.m_ItemHash = 0;
				break;
			}
		}
	}

	return ReturnLink;
}


bool CWObject_Character::Char_SetDialogueChoices(const char *_pSt, int _iSender, int _iOwner)
{
	CWO_Character_ClientData *pCD = GetClientData(this);
	if(!pCD)
		return false;

	m_liDialogueChoices.SetLen(0);
	if(_pSt != NULL)
	{
		CFStr St = _pSt;
	
		int i = 0;
		while(St != "")
		{
			CFStr ItemStr = St.GetStrSep(",");
			CDialogueLink Link = Char_ParseDialogueChoice(ItemStr, _iSender, _iOwner);

			if (Link.IsValid())
			{
				m_liDialogueChoices.Add(Link);
				if (m_liDialogueChoices.Len() == 16)
					break;
			}
		}
	}

	if(/*m_liDialogueChoices.Len() == 1 || */(pCD->m_iPlayer == -1 && m_liDialogueChoices.Len() > 0))
	{
//		int iItem = MRTC_RAND() % m_liDialogueChoices.Len();
 		PlayDialogue_Hash(m_liDialogueChoices[0].m_ItemHash, DIALOGUEFLAGS_FROMLINK, 0);
		m_liDialogueChoices.SetLen(0);
	}
	else if(m_liDialogueChoices.Len() > 0)
	{
		//PlayDialogue(m_liDialogueChoices[m_iCurDialogueChoice], DIALOGUEFLAGS_FROMLINK | DIALOGUEFLAGS_PAUSED | DIALOGUEFLAGS_ARROWDOWN, 0);
		uint8 Mode = (pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK);
//		uint8 State = (pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_STATE_MASK);
		bool b3PI = (Mode != THIRDPERSONINTERACTIVE_MODE_NONE);
		if(b3PI)
		{
			CNetMsg Msg(PLAYER_NETMSG_SETDIALOGUECHOICES);
			Msg.AddInt8(m_liDialogueChoices.Len());
			for (int i = 0; i < m_liDialogueChoices.Len(); i++)
				Msg.AddInt32(m_liDialogueChoices[i].m_ItemHash);
			m_pWServer->NetMsg_SendToObject(Msg, m_iObject, 1000000);
		}
	}
	else
		return false;

	return true;
}

void CWObject_Character::Char_SelectDialogueChoice(int _iChoice)
{
	CWO_Character_ClientData *pCD = GetClientData(this);
	if(!pCD)
		return;

	/*CWRes_Dialogue *pDialogue = GetDialogueResource(this, m_pWServer);
	if(!pDialogue)
		return;*/

	if (pCD->m_iFocusFrameObject != 0)  //m_3PI_FocusObject
	{
		uint nParam = -(1 + _iChoice);
		CWObject_Message Msg(OBJMSG_CHAR_USE, nParam);
		Msg.m_iSender = m_iObject;
		m_iLastAutoUseObject = pCD->m_iFocusFrameObject;
		m_pWServer->Message_SendToObject(Msg, pCD->m_iFocusFrameObject);
	}

/*	if(m_liDialogueChoices.Len() > 0)
	{
		if(_iChoice >= 0 && _iChoice < m_liDialogueChoices.Len())
		{
			CStr Choice = (wchar *)pDialogue->FindEvent(m_liDialogueChoices[_iChoice], CWRes_Dialogue::EVENTTYPE_CHOICE);
			if(Choice != "")
			{
				//CStr Choice = pCD->m_DialogueInstance.m_Choice;
				bool bUD = Choice.CompareSubStr("$UD") == 0;
				bool bMoth = Choice.CompareSubStr("$MO") == 0;
				if(bUD || bMoth)
				{
					CStr NumParam = Choice.GetStrSep(":").Copy(3, 1024);
					if((bUD && NumParam.Val_int() > pCD->m_nUDMoney) ||
					(bMoth && NumParam.Val_int() > pCD->m_nMoth))
					{
						m_pWServer->Sound_Global(m_pWServer->GetMapData()->GetResourceIndex_Sound("gui_m_open"));
						m_liDialogueChoices.SetLen(0);
						return;
					}
				}
			}

			PlayDialogue(m_liDialogueChoices[_iChoice], DIALOGUEFLAGS_FROMLINK, 0);
			m_liDialogueChoices.SetLen(0);
		}
	}
	else
	{
		if(g_bSkippable)
		{
			// Skip current DialogueItem
			if(pCD->m_PlayerDialogueToken.m_iLastSpeaker != 0)
			{
				CNetMsg Msg(PLAYER_NETMSG_KILLVOICE);
				m_pWServer->NetMsg_SendToObject(Msg, pCD->m_PlayerDialogueToken.m_iLastSpeaker);
			}
			Char_BeginTimeLeap(false);
			g_bSkippable = false;
		}
	}*/
}

bool CWObject_Character::Char_SkipDialogue(void)
{
	CWO_Character_ClientData *pCD = GetClientData(this);
	if(!pCD)
		return false;

	int iterations = 0;
	CWRes_Dialogue::CRefreshRes Res;
	int iObj = m_iSpeaker;
	CWObject *pSpeakerObj = m_pWServer->Object_Get(m_iSpeaker);
	if(!pSpeakerObj)
	{
		pSpeakerObj = m_pWServer->Object_Get(m_iObject);
		iObj = m_iObject;
		if(!pSpeakerObj)
			return false;
	}
	CWRes_Dialogue *pDialogue = GetDialogueResource(pSpeakerObj, m_pWServer);
	if(!pDialogue)
		return false;
	CWO_Character_ClientData *pCDSpeaker = GetClientData(pSpeakerObj);
	if(!pCDSpeaker)
		return false;
	pCDSpeaker->m_VoCap.StopVoice();
	pCDSpeaker->m_VoCap.ClearQueue();

	CMTime simulate_time = pCDSpeaker->m_GameTime;

	CWObject_Character* pSpeakChar = safe_cast<CWObject_Character>(pSpeakerObj);
	if(!pSpeakChar)
		return false;

	while(!pDialogue->Refresh(simulate_time, m_pWServer->GetGameTickTime(), pCDSpeaker->m_DialogueInstance, pCDSpeaker->m_pCurrentDialogueToken, &Res))
	{
		pSpeakChar->EvalDialogueLink(Res);
		pSpeakChar->RefreshInteractiveDialogue();
		simulate_time = CMTime::CreateFromTicks(m_pWServer->GetGameTick() + iterations, m_pWServer->GetGameTickTime(), pCDSpeaker->m_PredictFrameFrac);
		iterations++;
	}

	//Kill the sound
	CNetMsg Msg(NETMSG_STOPDIALOGUE);
	m_pWServer->NetMsg_SendToObject(Msg, iObj);

	CNetMsg Msg2(NETMSG_FORWARDDIALOGUE);
	m_pWServer->NetMsg_SendToObject(Msg2, iObj);

	pSpeakChar->EvalDialogueLink(Res);
	pSpeakChar->RefreshInteractiveDialogue();

	return true;
}

bool CWObject_Character::Char_SetDialogueTokenHolder(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, int _iHolder, int _iSender)
{
	// This function should only be called on the Player that "owns" the dialogue
	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if(!pCD)
		return false;

	if(_iHolder == -1)
		return false;

	// Always make sure we are the owner
	pCD->m_PlayerDialogueToken.m_iOwner = _pObj->m_iObject;
	//ConOut(CStrF("(Char_SetDialogueTokenHolder) Sender: %i  Owner: %i", _iSender, _pObj->m_iObject));

	{
		// Remove sender as controller of dialogue
		CWObject_CoreData *pCore = _pWPhysState->Object_GetCD(_iSender);
		if(!pCore)
			return false;
		
		CWO_Character_ClientData *pCDSender = GetClientData(pCore);
		if(!pCD)
			return false;
		
		pCDSender->m_pCurrentDialogueToken = NULL;
	}

	{
		// Set holder as new controller of dialogue
		CWObject_CoreData *pCore = _pWPhysState->Object_GetCD(_iHolder);
		if(!pCore)
			return false;

		CWO_Character_ClientData *pCDHolder = GetClientData(pCore);
		if(!pCD)
			return false;

		pCD->m_PlayerDialogueToken.m_CameraMode = 0;
		pCD->m_PlayerDialogueToken.m_CameraModeParameter = 0;

		pCDHolder->m_pCurrentDialogueToken = &pCD->m_PlayerDialogueToken;

//		int32 Player = pCD->m_iPlayer;
		g_bSkippable = true;
	}
	
	return true;
}

bool CWObject_Character::Char_SetDialogueChoices_Client(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, uint16 *_liItems, int _nItems)
{
	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if(!pCD)
		return false;

	CWRes_Dialogue *pDialogue = GetDialogueResource(_pObj, _pWPhysState);
	if(!pDialogue)
		return false;

	pCD->m_DialogueInstance.m_Choice = "";

	int nChoices = 0;
	for(int i = 0; i < _nItems; i++)
	{
		CStr Choice = (wchar *)pDialogue->FindEvent(_liItems[i], CWRes_Dialogue::EVENTTYPE_CHOICE);
		if(Choice != "")
		{
			bool bUD = Choice.CompareSubStr("$UD") == 0;
			bool bMoth = Choice.CompareSubStr("$MO") == 0;
			if(bUD || bMoth)
			{
				CStr NumParam = Choice.GetStrSep(":").Copy(3, 1024);
				CStr Choice1 = Choice.GetStrSep("|");
				CStr Choice2 = Choice.GetStrSep("|");
				if((bUD && NumParam.Val_int() <= pCD->m_nUDMoney) ||
					(bMoth && NumParam.Val_int() <= pCD->m_nMoth))
					Choice = Choice1;
				else
					Choice = "$" + Choice2;
			}

			if(nChoices != 0)
				pCD->m_DialogueInstance.m_Choice += ";";
			nChoices++;
			pCD->m_DialogueInstance.m_Choice += Choice;
		}
		else
		{
			const wchar* pSub = (wchar *)pDialogue->FindEvent(_liItems[i], CWRes_Dialogue::EVENTTYPE_SUBTITLE);
			if (pSub)
			{
				pSub++; // skip flags
				CStr Sub = pSub;
				if(nChoices != 0)
					pCD->m_DialogueInstance.m_Choice += ";";
				nChoices++;
				pCD->m_DialogueInstance.m_Choice += Sub;
			}
		}
	}
	if(pCD->m_DialogueInstance.m_Choice != "")
	{
		pCD->m_DialogueInstance.m_StartTime = pCD->m_GameTime;
		pCD->m_DialogueInstance.m_Flags = DIALOGUEFLAGS_PAUSED;

		M_TRACEALWAYS("WARNING: OLD CODE!\n");
	//	pCD->m_DialogueInstance.m_iDialogueItem = (nChoices << 8);

		CWObject_Message Msg(OBJMSG_GAME_ADDSUBTITLE, 1000000);
		Msg.m_iSender = _pObj->m_iObject;
		_pWPhysState->Phys_Message_SendToObject(Msg, _pWPhysState->Game_GetObjectIndex());
	}
	return false;
}

void CWObject_Character::Char_SetListener(int _iListener, int _Flags)
{
	CWO_Character_ClientData *pCD = GetClientData(this);
	if(!pCD)
		return;

	if(m_iListener == _iListener)
		return;

	if(m_iListener)
	{
		OnMessage(CWObject_Message(OBJMSG_CHAR_DIALOGUE_SPEAKER, 0));
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DIALOGUE_LISTENER, 0), m_iListener);
		CWObject_Character *pObj = TDynamicCast<CWObject_Character>(m_pWServer->Object_Get(m_iListener));
		if(pObj)
		{
			pObj->m_iSpeaker = 0;
		}
		DBG_OUT_LOG("[%.2f, Char %d, %s], Clear listener. (old: %d, %s)", 
			m_pWServer->GetGameTime().GetTime(), m_iObject, GetName(), m_iListener, m_pWServer->Object_GetName(m_iListener));
		m_iListener = 0;

		// when m_iListener == 0, this tick is used to trigger 'ignore' item..
 		pCD->m_DialogueChoiceTick = pCD->m_GameTick;
	}

	if(_iListener)
	{
		OnMessage(CWObject_Message(OBJMSG_CHAR_DIALOGUE_SPEAKER, _iListener, _Flags));
		m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DIALOGUE_LISTENER, m_iObject, _Flags), _iListener);
		CWObject_Character *pObj = TDynamicCast<CWObject_Character>(m_pWServer->Object_Get(_iListener));
		if(pObj)
			pObj->m_iSpeaker = m_iObject;
		m_iListener = _iListener;

		DBG_OUT_LOG("[%.2f, Char %d, %s], Set listener: %d, %s", 
			m_pWServer->GetGameTime().GetTime(), m_iObject, GetName(), m_iListener, m_pWServer->Object_GetName(m_iListener));

		if (pCD->m_liDialogueChoice.Len())
			pCD->m_DialogueChoiceTick = pCD->m_GameTick;
	}
}

void CWObject_Character::Char_DestroyDialogue()
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
	if (!pCD)
		return;

	// Clear choices
	pCD->m_liDialogueChoice.SetLen(0);
	pCD->m_liDialogueChoice.MakeDirty();
	pCD->m_DialogueChoiceTick = -1;

	if (pCD->m_DialogueInstance.IsValid())
	{
		pCD->m_DialogueInstance.Reset(CMTime(), 0, 0.0f, 0);
		/*memset(pCD->m_AnimGraph2.m_Dialogue_Once, 0, sizeof(pCD->m_AnimGraph2.m_Dialogue_Once));
		memset(pCD->m_AnimGraph2.m_Dialogue_Cur, 0, sizeof(pCD->m_AnimGraph2.m_Dialogue_Cur));*/
		CNetMsg Msg(PLAYER_NETMSG_KILLVOICE); 
		m_pWServer->NetMsg_SendToObject(Msg, m_iObject); 
	}
	Char_SetListener(0);

   // pCD->m_PlayerDialogueToken.Clear();
}

void CWObject_Character::Char_DebugDialogueInfo(void)
{
	CWRes_Dialogue *pDialogue = GetDialogueResource(this, m_pWServer);
	ConOutL(CStrF("Dialogue file: %s\n", pDialogue->m_Name.Str()));

	struct { CDialogueLink& item; const char* pText; } Items[] =
	{
		{ m_DialogueItems.m_Approach, "Approach" },
		{ m_DialogueItems.m_ApproachScared, "Scared" },
		{ m_DialogueItems.m_Threaten, "Threaten" },
		{ m_DialogueItems.m_Ignore, "Ignore" },
		{ m_DialogueItems.m_Timeout, "Timeout" },
		{ m_DialogueItems.m_Exit, "Exit" },
	};

	for (uint i = 0; i < 6; i++)
	{
		const char* pBuf = pDialogue->GetHashDialogueItem(Items[i].item.m_ItemHash);
		const char *pSub = NULL;
		if(pBuf)
		{
			pSub = pDialogue->FindEventBuf(pBuf, 0);
			CStr s = CStr((wchar*)(pSub + 2));
			ConOutL(CStrF(WTEXT("%s: %s"), Items[i].pText, s.StrW())); 
		}
		else
			ConOutL(CStrF("%s: ERROR!", Items[i].pText)); 
	}
}


void CWObject_Character::Char_ActivateDialogueItem(CDialogueLink _DialogueItem, int _iUser)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(this);
	M_ASSERT(pCD, "!");

	// TEST: Clear the may-not-use-special-items flag
	m_Flags &= ~PLAYER_FLAGS_DIALOGUE_NOSPECIAL;
	m_Flags &= ~PLAYER_FLAGS_DIALOGUE_WAIT;

	if (_DialogueItem.IsValid())
	{
		// Abort current dialogues
		Char_DestroyDialogue();
		if (m_iSpeaker != 0)
			m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DESTROYCAUSUALDIALOGUE, CAI_Action::PRIO_FORCED), m_iSpeaker);
		Char_SetListener(0);

		/*CWObject_CoreData* pObj = m_pWServer->Object_GetCD(_iUser);
		//Turn character, to look at the thing that is trying to use this
		if (!(m_Flags & PLAYER_FLAGS_NODIALOGUETURN))
		{
		CMat4Dfp32 Mat = GetPositionMatrix();
		CVec3Dfp32 Dir = pObj->GetPosition() - GetPosition();
		Dir.SetMatrixRow(Mat, 0);
		Mat.RecreateMatrix(0, 2);
		SetPosition(Mat);
		}*/

		//int iSelfDialogueItem = 0;
		CDialogueLink SelfDialogueItem;
		bool bBegin = true;
		if (_DialogueItem.m_bIsPlayer)
		{
			SelfDialogueItem = _DialogueItem;
			SelfDialogueItem.m_bIsPlayer = !_DialogueItem.m_bIsPlayer;
			_DialogueItem.m_ItemHash = 0;

			CWRes_Dialogue* pDialogue = GetDialogueResource(this, m_pWServer);
			/*if (pDialogue && pDialogue->GetNumItems() > iSelfDialogueItem)
			{
			if(!pDialogue->HasLink(iSelfDialogueItem))
			bBegin = false;
			}*/
			// Is this safe? We can´t compare iSelfDialogueItem´s hash value with pDialogue->GetNumItems()..
			if (pDialogue)
			{
				if (!pDialogue->HasLink_Hash(SelfDialogueItem.m_ItemHash))
					bBegin = false;

				CWObject* pPlayer = m_pWServer->Object_Get(_iUser);
				if (pPlayer)
				{
					CWO_Character_ClientData* pCDPlayer = CWObject_Character::GetClientData(pPlayer);
					uint32 NewLightType = pDialogue->GetLightType(SelfDialogueItem.m_ItemHash);
					uint LightState = pCDPlayer->m_3PI_LightState;
					if (!(LightState == THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_IN || LightState == THIRDPERSONINTERACTIVE_LIGHT_STATE_ON) && 
					    NewLightType == THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_IN)
					{
						pCDPlayer->m_3PI_LightFadeStart = m_pWServer->GetGameTick();
						pCDPlayer->m_3PI_LightFadeStart.MakeDirty();

						pCDPlayer->m_3PI_LightState = NewLightType;
						pCDPlayer->m_3PI_LightState.MakeDirty();
					}
				}
			}
		}
		if (SelfDialogueItem.IsValid())
		{
			if (bBegin)
 				m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_SETDIALOGUETOKENHOLDER, m_iObject, 0, m_iObject), _iUser);
			if (!PlayDialogue_Hash(SelfDialogueItem.m_ItemHash, DIALOGUEFLAGS_FROMLINK, 0))
				bBegin = false;
		}
		if (bBegin)
		{
			CWObject_Message Msg(OBJMSG_CHAR_BEGINDIALOGUE, _DialogueItem.m_ItemHash);
			Msg.m_iSender = m_iObject;
			Msg.m_Reason = 1;
			m_pWServer->Message_SendToObject(Msg, _iUser);
		}
	}
}


void CWObject_Character::RefreshInteractiveDialogue()
{
	CWO_Character_ClientData& pCD = *GetClientData(this);
	M_ASSERT(&pCD, "CWObject_Character::RefreshInteractiveDialogue() : pCD = NULL");

	if (m_spAI == NULL || pCD.m_iPlayer != -1)
		return; // This is only for AI's

	int iPlayer = m_spAI->GetClosestPlayer(); //hmm...
	CWObject_Character* pPlayer = CWObject_Character::IsCharacter(iPlayer, m_pWServer);
	if (!pPlayer)
		return;

	// Reset Wait-flag?
	if (m_Flags & PLAYER_FLAGS_DIALOGUE_WAIT)
	{
		fp32 TimeElapsed = (pCD.m_GameTick - pCD.m_DialogueChoiceTick) * m_pWServer->GetGameTickTime();
		if (TimeElapsed > 1.0f) // TWEAK THIS VALUE!!
		{
			m_Flags &= ~PLAYER_FLAGS_DIALOGUE_WAIT;
			Char_SetListener(0); // to enable auto-approach
		}
	}

	// Reset listener?
	if ((m_iListener == iPlayer) && (pCD.m_DialogueChoiceTick > 0 || !pCD.m_DialogueInstance.IsValid()))
	{ 
		CMat4Dfp32 HeadMat, PlayerHeadMat;
		AI_GetHeadMat(HeadMat);
		pPlayer->AI_GetHeadMat(PlayerHeadMat);

		const CVec3Dfp32& PlayerPos = PlayerHeadMat.GetRow(3);
		const CVec3Dfp32& PlayerLook = pPlayer->GetPositionMatrix().GetRow(0);
		const CVec3Dfp32& MyPos = HeadMat.GetRow(3);
		const CVec3Dfp32& MyLook = HeadMat.GetRow(0);//GetPositionMatrix().GetRow(0);

		CVec3Dfp32 MeToPlayer = PlayerPos - MyPos;
		fp32 DistanceSqr = MeToPlayer.LengthSqr();

		MeToPlayer.Normalize();
		fp32 DirCheck1 = -Dot2(MeToPlayer, PlayerLook); // Check if player is looking at me
		fp32 DirCheck2 =  Dot2(MeToPlayer, MyLook);     // Check if player is where I'm looking

		//CVec3Dfp32 MeToPlayerZ = MeToPlayer;
		//MeToPlayerZ.k[2] = pPlayer->GetPosition().k[2] - GetPosition().k[2];
		//DistanceSqr = Min(DistanceSqr, MeToPlayerZ.LengthSqr());

		if ((DistanceSqr > Sqr(64.0f) && (DirCheck1 < 0.1f || DirCheck2 < -0.2f)) || DistanceSqr > Sqr(128.0f))
		{
			CWObject_Character *pPlayer = (CWObject_Character *)m_pWServer->Object_Get(m_iListener);
			CWO_Character_ClientData *pPlayerCD = GetClientData(pPlayer);
			// if(pPlayerCD->m_iFocusFrameObject != m_iObject)

			{
				DBG_OUT_LOG("[%.2f, Char %d, %s], Resetting listener because of failed tests (Distance: %.1f [%.1f], DirCheck1: %.1f [%.1f], DirCheck2: %.1f [%.1f]",
					m_pWServer->GetGameTime().GetTime(), m_iObject, GetName(), M_Sqrt(DistanceSqr), 64.0f, DirCheck1, 0.1f, DirCheck2, -0.2f);
				Char_SetListener(0);
			}
		}
	}

	bool bHaveChoices = pCD.m_liDialogueChoice.Len() > 0;
	bool bSpeaking = (m_ClientFlags & PLAYER_CLIENTFLAGS_PLAYERSPEAK) != 0;
	bool bInDialogue = bHaveChoices || pCD.m_DialogueInstance.IsValid();

	// Check stuff for AIs in dialogue
	if (bInDialogue)
	{
		// Check priority
		int Prio = pCD.m_DialogueInstance.m_Priority; 
		if (Prio < m_spAI->GetCurrentPriorityClass())
		{
			DBG_OUT_LOG("[%.2f, Char %d, %s], AI prio-override (dialogue prio: %d, ai prio: %d)!", 
				m_pWServer->GetGameTime().GetTime(), m_iObject, GetName(), Prio, m_spAI->GetCurrentPriorityClass());
			Char_DestroyDialogue();
			return;
		}

		
		if((pCD.m_liDialogueChoice.Len() > 0) && (m_spAI->GetCurrentPriorityClass() > 0x60))
		{
			pCD.m_liDialogueChoice.SetLen(0);
			pCD.m_liDialogueChoice.MakeDirty();
			return;
		}

		bool bNoSpecial = (m_Flags & PLAYER_FLAGS_DIALOGUE_NOSPECIAL) != 0;

		// Check for timeout
		if (m_DialogueItems.m_Timeout.IsValid() && !bNoSpecial && m_iListener != 0 && bHaveChoices && pCD.m_DialogueChoiceTick > 0)
		{
			if (bSpeaking) // Hold timer until char is done speaking
				pCD.m_DialogueChoiceTick = pCD.m_GameTick;

			fp32 TimeElapsed = (pCD.m_GameTick - pCD.m_DialogueChoiceTick) * m_pWServer->GetGameTickTime();
			if (TimeElapsed > 7.5f) // TWEAK THIS VALUE!!
			{
				DBG_OUT_LOG("[%.2f, Char %d, %s], Dialogue timeout", 
					m_pWServer->GetGameTime().GetTime(), m_iObject, GetName());

				//DBG_OUT("Dialogue: '%s' *timeout*! item=%d\n", GetName(), m_iDialogueItem_Timeout);
				Char_ActivateDialogueItem(m_DialogueItems.m_Timeout, iPlayer);
				CWObject_Character* pChar = safe_cast<CWObject_Character>( m_pWServer->Object_Get(iPlayer) );
				pChar->m_iLastAutoUseObject = 0;
				m_Flags |= PLAYER_FLAGS_DIALOGUE_NOSPECIAL;
				m_Flags |= PLAYER_FLAGS_DIALOGUE_WAIT;
				pCD.m_DialogueChoiceTick = pCD.m_GameTick;
			}
		}
		
		fp32 TimeLeft = (pCD.m_DialogueInstance.m_StartTime - pCD.m_GameTime).GetTime() + pCD.m_DialogueInstance.m_SampleLength;
			
		if (m_DialogueItems.m_Ignore.IsValid() && (m_iListener == 0) && !bNoSpecial &&
			(pCD.m_DialogueInstance.m_DialogueItemHash != m_DialogueItems.m_Ignore.m_ItemHash) && 
			(TimeLeft > 1.0f) && (pCD.m_DialogueChoiceTick > 0))
		{
			fp32 TimeOut = 1.5f; // TWEAK THESE VALUES
			CWObject_Game *pGame = m_pWServer->Game_GetObject();
			CWO_Player *pPlayer = pGame->Player_Get(0); 
			CWObject_Character *pChar = safe_cast<CWObject_Character>( m_pWServer->Object_Get(pPlayer->m_iObject) );
			CVec3Dfp32 Vec = GetPosition() - pChar->GetPosition();
			fp32 Dist = Vec.Length();
			fp32 TimeElapsed = (pCD.m_GameTick - pCD.m_DialogueChoiceTick) * m_pWServer->GetGameTickTime();

			CMat4Dfp32 OtherCharHeadMat;
			pChar->AI_GetHeadMat(OtherCharHeadMat);
			fp32 Dot = GetPositionMatrix().GetRow(0) * OtherCharHeadMat.GetRow(0);
			if(Dot > 1.0f)
				TimeOut = 0.75f;		// uhm, what is this? A normalized dot can't be larger than 1...
			if(Dist > 128.0f)
				TimeElapsed += 1.0f;

			if (TimeElapsed > TimeOut) 
			{
				DBG_OUT_LOG("[%.2f, Char %d, %s], Playing ignore item (%.2f s > %.2f, TimeLeft: %.2f)", 
					m_pWServer->GetGameTime().GetTime(), m_iObject, GetName(), TimeElapsed, TimeOut, TimeLeft);

				//DBG_OUT("Dialogue: '%s' *ignore*! item=%x\n", GetName(), m_DialogueItems.m_Timeout.m_ItemHash);				
				Char_ActivateDialogueItem(m_DialogueItems.m_Ignore, iPlayer);
				pChar->m_iLastAutoUseObject = 0;
				m_Flags |= PLAYER_FLAGS_DIALOGUE_NOSPECIAL;
				m_Flags |= PLAYER_FLAGS_DIALOGUE_WAIT;
				pCD.m_DialogueChoiceTick = pCD.m_GameTick;
			}
		}

		// Check for exit
		fp32 DistanceSqr = GetPosition().DistanceSqr(pPlayer->GetPosition());
		if (bHaveChoices && DistanceSqr > Sqr(32*10.0f))
		{
			//DBG_OUT("Dialogue: '%s' *exit*! item=%x\n", GetName(), m_DialogueItems.m_Exit.m_ItemHash);
			Char_DestroyDialogue();
			Char_ActivateDialogueItem(m_DialogueItems.m_Exit, iPlayer);
		}

		// Check if threatened by player
		if (m_DialogueItems.m_Threaten.IsValid())
		{
			int Flags = CAI_Core::PLAYER_RELATION_GUNAIM | CAI_Core::PLAYER_RELATION_WAYBEYOND_RANGE | CAI_Core::PLAYER_RELATION_GUNOUT;
			m_spAI->GetPlayerRelation(&Flags);

			if (!bNoSpecial && (Flags & CAI_Core::PLAYER_RELATION_GUNAIM) && !(Flags & CAI_Core::PLAYER_RELATION_WAYBEYOND_RANGE))
			{
				//DBG_OUT("Dialogue: '%s' *threaten*! item=%x\n", GetName(), m_DialogueItems.m_Threaten.m_ItemHash);
				Char_ActivateDialogueItem(m_DialogueItems.m_Threaten, iPlayer);
				m_Flags |= PLAYER_FLAGS_DIALOGUE_NOSPECIAL;
			}
		}
	}
}


CDialogueLink CWObject_Character::Char_GetDialogueApproachItem()
{
	int Prio = m_spAI->GetCurrentPriorityClass();
	if (Prio >= CAI_Action::PRIO_DANGER)
		return m_DialogueItems.m_ApproachScared;
	else
		return m_DialogueItems.m_Approach;
}

