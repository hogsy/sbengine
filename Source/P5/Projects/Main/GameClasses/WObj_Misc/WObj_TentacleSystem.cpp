/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			CWObject_TentacleSystem.

	Author:			Anton Ragnarsson, Patrik Willbo

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_TentacleSystem

	Comments:

	History:		
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_TentacleSystem.h"
#include "../../GameWorld/WServerMod.h"
#include "../CConstraintSystem.h"
#include "WObj_CreepingDark.h"
#include "WObj_Object.h"
#include "../WObj_AI/AICore.h"
#include "../WRPG/WRPGChar.h"

#include "../../../../Shared/MOS/Classes/GameWorld/WObjects/WObj_PhysCluster.h"

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_TentacleSystem, CWObject_Model, 0x0100);

enum
{
	class_SwingDoor =	MHASH6('CWOb','ject','_','Swin','g','Door'),
	class_CharPlayer =	MHASH6('CWOb','ject','_','Char','Play','er'),
	class_CharNPC =		MHASH5('CWOb','ject','_','Char','NPC'),
	class_Object_Lamp =	MHASH7('CWOb','ject','_','Obje','ct','_','Lamp'),
};


#define TENTACLES_TRACE 1 ? (void)0 : M_TRACEALWAYS

//TODO: Remove this - velocities should be [units / second] instead of [units / tick]
#define PHYSSTATE_CONVERTFROM20HZ(x) ((x) * 20.0f * m_pWServer->GetGameTickTime())



CWObject_TentacleSystem::CWObject_TentacleSystem()
	: m_ArmMaxLength(192.0f)
	, m_ArmMinLength(32.0f)
{
}


const CWO_TentacleSystem_ClientData& CWObject_TentacleSystem::GetClientData(const CWObject_CoreData* _pObj)
{
	const CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who deleted my client data?!");
	return *safe_cast<const CWO_TentacleSystem_ClientData>(pData);
}


CWO_TentacleSystem_ClientData& CWObject_TentacleSystem::GetClientData(CWObject_CoreData* _pObj)
{
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who deleted my client data?!");
	return *safe_cast<CWO_TentacleSystem_ClientData>(pData);
}


void CWObject_TentacleSystem::OnIncludeClass(CMapData* _pMapData, CWorld_Server* _pWServer)
{
	parent::OnIncludeClass(_pMapData, _pWServer);
	_pMapData->GetResourceIndex_Model("Tentacles");

	_pMapData->GetResourceIndex_Class("hitEffect_tentacle");
	_pMapData->GetResourceIndex_Class("hitEffect_tentacle2");
	_pMapData->GetResourceIndex_Class("Tentacle_PhysLink");
	_pMapData->GetResourceIndex_Class("hitEffect_HeartBlood");

	_pMapData->GetResourceIndex_Surface("HoleDarkEntry01");

	IncludeModel("Items/heart_01", _pMapData);
}


void CWObject_TentacleSystem::OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer)
{
	parent::OnIncludeTemplate(_pReg, _pMapData, _pWServer);

	uint nChildren = _pReg->GetNumChildren();
	for (uint i = 0; i < nChildren; i++)
	{
		CStr KeyName = _pReg->GetName(i);
		CStr KeyValue = _pReg->GetValue(i);

		if (KeyName == "ARM")
		{
			CTentacleArmSetup tmp;
			tmp.Setup(*_pReg->GetChild(i), *_pWServer);
		}
		else if (KeyName.CompareSubStr("SOUND_") == 0)
		{
			_pMapData->GetResourceIndex_Sound(KeyValue.Str());
		}
		else if (KeyName == "ANIMGRAPH")	// (Willbo) Replaced with ANIMGRAPH2, this one is old and should be removed.
		{
			_pMapData->GetResourceIndex_AnimGraph2(KeyValue.Str());
		}
		else if (KeyName == "ANIMGRAPH2")
		{
			MSCOPE(GetResourceIndex_AnimGraph, TENTACLESYSTEM);
			CRegistry* pAnimGraph2Reg = _pReg->GetChild(i);
			if(pAnimGraph2Reg)
			{
				int32 NumChildren = pAnimGraph2Reg->GetNumChildren();
				for (int32 i = 0; i < NumChildren; i++)
				{
					CStr KeyName = pAnimGraph2Reg->GetName(i);
					if (KeyName.CompareSubStr("GRAPH") == 0)
						_pMapData->GetResourceIndex_AnimGraph2(pAnimGraph2Reg->GetValue(i));
					else
						_pMapData->GetResourceIndex_AnimGraph2(KeyName);
				}
			}
		}
	}
}


void CWObject_TentacleSystem::OnCreate()
{
	m_bNoSave = true;
	GetClientData(this).m_iOwner = m_iOwner;

	m_InterestingType = CAI_Core::INTERESTING_NOTHING;
	m_pSelectedObject = NULL;
	m_LocalGrabPoint = 0.0f;


}


void CWObject_TentacleSystem::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	MSCOPE(CWObject_TentacleSystem::OnEvalKey, TENTACLES);

	CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	CMapData* pMapData = m_pWServer->GetMapData();
	CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();

	switch (_KeyHash)
	{
	case MHASH1('ARM'): // "ARM"
		{
			int iArm = CD.m_lArmSetup.Len();
			if (iArm >= CD.m_lArmSetup.GetMax())
			{
				ConOutL(CStrF("CWObject_TentacleSystem::OnEvalKey, Too many tentacle arms! (max = %d)", CD.m_lArmSetup.GetMax()));
				break;
			}

			CD.m_lArmSetup.SetLen(iArm + 1);
			CD.m_lArmSetup[iArm].Setup(*_pKey, *m_pWServer);
			CD.m_lArmSetup.MakeDirty();

			CD.m_lArmState.SetLen(iArm + 1);
			CD.m_lArmState.MakeDirty();

			CD.m_lArmRetract.SetLen(iArm + 1);
			CD.m_lArmRetract.MakeDirty();

	/*		if (CD.m_lArmSetup[iArm].m_bPhysLinks)
				CD.m_lArmState[iArm].m_plBones = DNew(TStaticArray<uint16, 100>) TStaticArray<uint16, 100>;*/
			break;
		}
	case MHASH2('SHAD','OWS'): // "SHADOWS"
		{
			if (KeyValue.Val_int() == 1)
				m_ClientFlags |= CWO_CLIENTFLAGS_SHADOWCASTER;
			break;
		}
	case MHASH3('ANIM','GRAP','H2'): // "ANIMGRAPH2"
		{
			int32 NumChildren = _pKey->GetNumChildren();
			for (int32 i = 0; i < NumChildren; i++)
			{
				CWAG2I_Context Context(this, m_pWServer, CMTime());
				CStr AGName = _pKey->GetName(i);
				int32 iAGSlot = -1;
				if (AGName.CompareSubStr("GRAPH") == 0)
				{
					CStr Name = AGName;
					AGName = _pKey->GetValue(i);

					if (NumChildren > 1)
					{
						CStr Right = Name.RightFrom(5);
						iAGSlot = Right.Val_int() - 1;
					}
				}

				int32 iAG2 = m_pWServer->GetMapData()->GetResourceIndex_AnimGraph2(AGName);
				if (iAGSlot == -1) CD.m_AnimGraph.GetAG2I()->AddResourceIndex_AnimGraph2(iAG2, AGName);
				else CD.m_AnimGraph.GetAG2I()->SetResourceIndex_AnimGraph2(iAG2, AGName, iAGSlot);
			}
			break;
		}
	case MHASH4('ARMM','AX_L','ENGT','H'):	// "ARMMAX_LENGTH"
		{
			m_ArmMaxLength = (fp32)KeyValue.Val_fp64();
			break;
		}
	case MHASH4('ARMM','IN_L','ENGT','H'):	// "ARMMIN_LENGTH"
		{
			m_ArmMinLength = (fp32)KeyValue.Val_fp64();
			break;
		}
	default:
		{
			if (KeyName.CompareSubStr("SOUND_") == 0)
			{
				static const char* SoundKeys[] = { "SOUND_REACH", "SOUND_GRAB", "SOUND_HOLD", "SOUND_RETURN", "SOUND_DEVOUR", "SOUND_BRTH_SFT_DH1", "SOUND_BRTH_SFT_DH2", "SOUND_BRTH_HRD_DH1", "SOUND_BRTH_HRD_DH2", NULL };
				int iSlot = KeyName.TranslateInt(SoundKeys);
				if (iSlot >= 0)
				{
					CD.m_liSounds[iSlot] = pMapData->GetResourceIndex_Sound(_pKey->GetThisValue().Str());
					CD.m_liSounds.MakeDirty();
				}
			}
			else
				parent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}


void CWObject_TentacleSystem::OnFinishEvalKeys()
{
	CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	if (m_iOwner > 0 && CD.m_lArmState.Len() <= TENTACLE_DEMONHEAD2)
		Error("CWObject_TentacleSystem::OnFinishEvalKeys", "Too few tentacle arms!");

	// Make sure all animations are precached
	CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
	CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();

	TArray<CXRAG2_Impulse> lImpulses;
	lImpulses.Add(CXRAG2_Impulse(TENTACLE_AG2_IMPULSETYPE_DEMONHEAD, TENTACLE_AG2_IMPULSEVALUE_HUGIN));
	lImpulses.Add(CXRAG2_Impulse(TENTACLE_AG2_IMPULSETYPE_DEMONHEAD, TENTACLE_AG2_IMPULSEVALUE_MUNIN));
	lImpulses.Add(CXRAG2_Impulse(TENTACLE_AG2_IMPULSETYPE_GUITENTACLE, TENTACLE_AG2_IMPULSEVALUE_GUITENTACLE));
	lImpulses.Add(CXRAG2_Impulse(TENTACLE_AG2_IMPULSETYPE_TENTACLESQUID, TENTACLE_AG2_IMPULSEVALUE_TENTACLESQUID));
	lImpulses.Add(CXRAG2_Impulse(TENTACLE_AG2_IMPULSETYPE_HEART, TENTACLE_AG2_IMPULSEVALUE_HEART));
	pAGI->TagAnimSetFromImpulses(&AGIContext, m_pWServer->GetMapData(), m_pWServer->m_spWData, lImpulses);

	CWServer_Mod* pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
	uint nAnimGraphs = pAGI->GetNumResource_AnimGraph2();
	for (uint i = 0; i < nAnimGraphs; i++)
		pServerMod->RegisterAnimGraph2(pAGI->GetResourceIndex_AnimGraph2(i));
}


aint CWObject_TentacleSystem::OnMessage(const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJMSG_CHAR_ONANIMEVENT:
		{
			CXR_Anim_DataKey *pKey = (CXR_Anim_DataKey *)_Msg.m_Param0;
			if(pKey)
			{
				if (pKey->m_Type == ANIM_EVENT_TYPE_EFFECT)
				{
					if (pKey->m_Param == 1)	// OtherCrap
					{
						CWO_TentacleSystem_ClientData& CD = GetClientData(this);
						CStr KeyData(pKey->Data());
						int32 iArm = KeyData.GetStrSep(",").Val_int();
						uint32 TypeHashName = StringToHash(KeyData.GetStrSep(",").GetStr());
						CTentacleArmState& ArmState = CD.m_lArmState[iArm];
						CWObject_CoreData* pTarget = m_pWServer->Object_Get(ArmState.m_iTarget);
						CMat4Dfp32 PosMat;
						if (pTarget)
							CD.GetCharBoneMat(*pTarget, ArmState.m_iRotTrack, PosMat);
						else
						{
							PosMat.Unit();
							PosMat.GetRow(3) = ArmState.m_Pos;
						}

						switch (TypeHashName)
						{
						case MHASH3('Dev','our','Obj'):
							{
								int iOwner = (ArmState.m_iTarget) ? ArmState.m_iTarget : -1;
								m_pWServer->Object_Create(KeyData.GetStr(), PosMat, iOwner);
								break;
							}
						case MHASH3('Dev','our','Mdl0'):
							{
								PosMat.Unit3x3();
								m_pWServer->Object_Create(KeyData.GetStr(), PosMat);
								break;
							}

						case MHASH4('Dev','our','Blo','od'):
							{
								CD.Server_CreateDevourBlood(ArmState);
								break;
							}

						case MHASH5('Dev','our','Pre','Fini','sh'):
							{
								m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DEVOURTARGET_PREFINISH), m_iOwner);
								break;
							}

						case MHASH4('Dev','our','Fini','sh'):
							{
								m_pWServer->Message_SendToObject(CWObject_Message(OBJMSG_CHAR_DEVOURTARGET_FINISH, ArmState.m_iTarget), m_iOwner);
								ArmState.UpdateRunQueue();
								if (ArmState.m_pControlArm)
								{
									ArmState.m_pControlArm->UpdateRunQueue();
									ArmState.m_pControlArm->m_pControlArm = NULL;
									ArmState.m_pControlArm = NULL;
								}

								ArmState.m_iRotTrack = 0;
								ArmState.m_iTarget = 0;

								CD.m_lArmState.MakeDirty();
								break;
							}
						case MHASH4('Dev','our','Hit','Body'):
							{
								CWObject_Character* pTarget = (ArmState.m_iTarget) ? safe_cast<CWObject_Character>(m_pWServer->Object_Get(ArmState.m_iTarget)) : NULL;
								if (pTarget)
								{
									CMat4Dfp32 HeartMat;
									CD.GetCharBoneMat(*pTarget, ArmState.m_iRotTrack, HeartMat);
									CVec3Dfp32 Force = 0.0f;
									Force.ParseString(KeyData);
#ifdef INCLUDE_OLD_RAGDOLL
									if (pTarget->m_spRagdoll != NULL)
									{
										m_spRagdoll->AddPendingImpulse(HeartMat.GetRow(3), Force);
									}
#endif // INCLUDE_OLD_RAGDOLL
									if (pTarget->m_pPhysCluster != NULL)
									{
										uint32 iBest = pTarget->m_pPhysCluster->QuickGetClosest(HeartMat.GetRow(3));
										m_pWServer->Phys_AddForce(pTarget->m_pPhysCluster->m_lObjects[iBest].m_pRB, Force * RAGDOLL_FORCE_MULTIPLIER);
									}
								}
								break;
							}
						}
					}
				}
			}

			return 1;
		}

	case OBJMSG_CHAR_ANIMIMPULSE:
		{
			if (!_Msg.m_pData)
				return 0;

			CWO_TentacleSystem_ClientData& CD = GetClientData(this);
			CXRAG2_Impulse Impulse;
			Impulse.m_ImpulseType = _Msg.m_Param0;
			CFStr Token = (char *)_Msg.m_pData;
			Impulse.m_ImpulseValue = Token.GetStrSep(",").Val_int();
			CWAG2I_Context AGContext(this, m_pWServer,m_pWServer->GetGameTime(),m_pWServer->GetGameTickTime());
			CD.m_AnimGraph.m_bNeedUpdate = CD.m_AnimGraph.GetAG2I()->SendImpulse(&AGContext,Impulse,Token.Val_int()) || CD.m_AnimGraph.m_bNeedUpdate;
			m_DirtyMask |= CD.m_AnimGraph.m_bNeedUpdate ? TENTACLE_AG2_DIRTYMASK : 0 << CWO_DIRTYMASK_USERSHIFT;
			return 1;
		}

	case OBJMSG_CHAR_DEMONHEAD_ANIMIMPULSE:
		{
			if (!_Msg.m_pData)
				return 0;

			CWO_TentacleSystem_ClientData& CD = GetClientData(this);
			CWAG2I_Context AGContext(this, m_pWServer, m_pWServer->GetGameTime(), m_pWServer->GetGameTickTime());
			CFStr AnimImpulse = (char *)_Msg.m_pData;
			
			// Read impulse from string "<TokenID>,<impulse type>,<impulse value>,< extra <impulse type>,<impulse value> >"
			CXRAG2_Impulse Impulse;
			int8 TokenID = int8(AnimImpulse.GetStrSep(",").Val_int());
			Impulse.m_ImpulseType = CAG2ImpulseValue(AnimImpulse.GetStrSep(",").Val_int());
			Impulse.m_ImpulseValue = CAG2ImpulseType(AnimImpulse.GetStrSep(",").Val_int());
			CFStr ImpulseType = AnimImpulse.GetStrSep(",");
			if (ImpulseType.Len())
			{
				Impulse.m_ImpulseType = CAG2ImpulseValue(ImpulseType.Val_int());
				Impulse.m_ImpulseValue = CAG2ImpulseType(AnimImpulse.GetStrSep(",").Val_int());
			}

			// Send impulse and check for update
			CD.m_AnimGraph.m_bNeedUpdate |= CD.m_AnimGraph.GetAG2I()->SendImpulse(&AGContext, Impulse, TokenID);
			m_DirtyMask |= CD.m_AnimGraph.m_bNeedUpdate ? TENTACLE_AG2_DIRTYMASK : 0 << CWO_DIRTYMASK_USERSHIFT;
			return 1;
		}

	case OBJMSG_OBJECT_ISBREAKING:
		{
			CWO_TentacleSystem_ClientData& CD = GetClientData(this);
			CTentacleArmState& DemonArm = CD.m_lArmState[TENTACLE_DEMONARM];

			// Are we grabbing the destroyed object?
			if (_Msg.m_iSender == DemonArm.m_iTarget)
			{
				DemonArm.m_iTarget = 0;

				uint nNewObjects = _Msg.m_Param0;
				const uint16* piNewObjects = (const uint16*)_Msg.m_Param1;

				CVec3Dfp32 CurrPos = DemonArm.m_Grabber.m_Pid[0].m_LastPos;
				uint iBestObj = 0;
				fp32 Best = 1e10f;
				for (uint i = 0; i < nNewObjects; i++)
				{
					int iNewObj = piNewObjects[i];
					CWObject* pObj = m_pWServer->Object_Get(iNewObj);
					CVec3Dfp32 ObjCenter;
					pObj->GetAbsBoundBox()->GetCenter(ObjCenter);
					fp32 d = CurrPos.DistanceSqr(ObjCenter);
					iBestObj = (d < Best) ? iNewObj : iBestObj;
					Best = Min(d, Best);
				}
				if (iBestObj)
				{
					DemonArm.m_iTarget = iBestObj;
					m_pWServer->Object_Get(iBestObj)->m_iOwner = (m_iOwner ? m_iOwner : m_iObject);
				}
				CD.m_lArmState.MakeDirty();
			}
		}
		break;
	case OBJMSG_CHAR_GETGRABBEDOBJECT:
		{
			CWO_TentacleSystem_ClientData& CD = GetClientData(this);
			CTentacleArmState& DemonArm = CD.m_lArmState[TENTACLE_DEMONARM];
			if (DemonArm.m_State == TENTACLE_STATE_GRABOBJECT)
				return DemonArm.m_iTarget;
			else
				return 0;
		}
	case OBJMSG_CHAR_GETDEMONARMREACH:
		{
			return (aint)m_ArmMaxLength;
		}
	case OBJSYSMSG_PRECACHEMESSAGE:
		{
			return OnPrecacheMessage(_Msg);
		}
	}

	return 0;
}


aint CWObject_TentacleSystem::OnPrecacheMessage(const CWObject_Message& _Msg)
{
	if(_Msg.m_DataSize == sizeof(CWObject_Message))
	{
		CWObject_Message* pMsg = (CWObject_Message*)_Msg.m_pData;
		CWO_TentacleSystem_ClientData& CD = GetClientData(this);

		switch (pMsg->m_Msg)
		{
		case OBJMSG_CHAR_DEMONHEAD_ANIMIMPULSE:
			{
				TArray<CXRAG2_Impulse> lImpulse;
				CXRAG2_Impulse Impulse;
				CFStr AnimState = (char *)pMsg->m_pData;
				int8 TokenID = (int8)AnimState.GetStrSep(",").Val_int();
				Impulse.m_ImpulseType = AnimState.GetStrSep(",").Val_int();
				Impulse.m_ImpulseValue = AnimState.GetStrSep(",").Val_int();
				lImpulse.Add(Impulse);
				CStr Val = AnimState.GetStrSep(",");
				if (Val.Len())
				{
					Impulse.m_ImpulseType = Val.Val_int();
					Impulse.m_ImpulseValue = AnimState.GetStrSep(",").Val_int();
					lImpulse.Add(Impulse);
				}

				CWAG2I_Context AGContext(this, m_pWServer, m_pWServer->GetGameTime(), m_pWServer->GetGameTickTime());
				CD.m_AnimGraph.GetAG2I()->TagAnimSetFromImpulses(&AGContext, m_pWServer->GetMapData(), m_pWServer->m_spWData, lImpulse);
			}
			return 1;
		}
	}

	return 0;
}


aint CWObject_TentacleSystem::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_CHAR_ONANIMEVENT:
		{
			CXR_Anim_DataKey *pKey = (CXR_Anim_DataKey *)_Msg.m_Param0;
			if(pKey)
			{
				/*if (pKey->m_Type == ANIM_EVENT_TYPE_DIALOGUE)
				{
					int iDialogue = pKey->m_Param;
					Char_PlayDialogue(_pObj, _pWClient, iDialogue);
				}
				else*/ if (pKey->m_Type == ANIM_EVENT_TYPE_SOUND)
				{
					int iSound = _pWClient->GetMapData()->GetResourceIndex_Sound(pKey->Data());
					if (iSound > 0)
						_pWClient->Sound_At(WCLIENT_CHANNEL_SFX, _pObj->GetPosition(), iSound, WCLIENT_ATTENUATION_3D);
				}
			}

			return 1;
		}

	case OBJSYSMSG_GETCAMERA:
		{
			CWO_TentacleSystem_ClientData& CD = GetClientData(_pObj);
			CTentacleArmState& Creep = CD.m_lArmState[TENTACLE_DEMONHEAD1];
			if(Creep.IsCreepingDark())
			{
				CMat4Dfp32& Camera = *(CMat4Dfp32*)_Msg.m_pData;
				Camera = GetCreepCam(_pObj, _pWClient, Camera);
				return 1;
			}
		}
	}

	return CWObject::OnClientMessage(_pObj, _pWClient, _Msg);
}


void CWObject_TentacleSystem::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	switch (_Msg.m_MsgType)
	{
	case TENTACLE_NETMSG_CREATEDECAL:
		{
			int iPos = 0;
			CVec3Dfp32 Pos0, Pos1;
			
			// Get net message data
			int iTarget = _Msg.GetInt16(iPos);
			Pos0.k[0] = _Msg.Getfp32(iPos);
			Pos0.k[1] = _Msg.Getfp32(iPos);
			Pos0.k[2] = _Msg.Getfp32(iPos);
			Pos1.k[0] = _Msg.Getfp32(iPos);
			Pos1.k[1] = _Msg.Getfp32(iPos);
			Pos1.k[2] = _Msg.Getfp32(iPos);
			
			CWObject_Client* pTarget = _pWClient->Object_Get(iTarget);
			if (pTarget)
			{
				CMat4Dfp32 DecalMat;
				CWO_TentacleSystem_ClientData& CD = GetClientData(_pObj);
				if (CD.DemonArmIntersect(Pos0, Pos1, iTarget, &DecalMat))
				{
					//M_TRACEALWAYS("CWObject_TentacleSystem::OnClientNetMsg: Black ninja arm penetrated target migthy fine!!\n");
					int iRc = _pWClient->GetMapData()->GetResourceIndex_Surface("HoleDarkEntry01");

					CXR_WallmarkDesc WMD;
					WMD.m_SurfaceID = _pWClient->GetMapData()->GetResource_SurfaceID(iRc);
					WMD.m_Size = 15.0f;
					WMD.m_SpawnTime = _pWClient->GetRenderTime();
					WMD.m_GUID = iTarget;
					WMD.m_iNode = 0;

					CWObject_Message DecalMsg(OBJMSG_SPAWN_WALLMARK, (aint)&WMD, (aint)&DecalMat);
					_pWClient->ClientMessage_SendToObject(DecalMsg, iTarget);
				}
				else
				{
					//M_TRACEALWAYS("CWObject_TentacleSystem::OnClientNetMsg: Black ninja arm was too drunk and got it's vectors screwed over by the evil police officers.\n");
				}
			}
		}
		break;
	}
}


int CWObject_TentacleSystem::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const
{
	MSCOPE(CWObject_TentacleSystem::OnCreateClientUpdate, TENTACLES);

	uint8* pD = _pData;

	int Flags = CWO_CLIENTUPDATE_EXTRADATA;
	if ((_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT) & ~TENTACLE_AG2_DIRTYMASK)
		Flags |= CWO_CLIENTUPDATE_AUTOVAR;

	pD += parent::OnCreateClientUpdate(_iClient, _pClObjInfo, _pOld, _pData, Flags);
	if ((pD - _pData) == 0)
		return 0;

	const CWO_TentacleSystem_ClientData& CD = GetClientData(this);

	{ // Handle AutoVars, remove flag used for ag2)
		CD.AutoVar_Pack((_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT) & ~TENTACLE_AG2_DIRTYMASK, pD, m_pWServer->GetMapData(), 0);
	}

	{ // Handle AnimGraph
		static const CWAG2I DummyAGI;
		const CWAG2I* pMirrorAGI = &DummyAGI;
		if (_pOld->m_lspClientObj[0])
			pMirrorAGI = GetClientData(_pOld).m_AnimGraph.m_spMirror->GetWAG2I(0);
		int8 AGPackType = (m_pWServer->GetPlayMode() == SERVER_PLAYMODE_DEMO) ? WAG2I_CLIENTUPDATE_NORMAL : WAG2I_CLIENTUPDATE_DIRECTACCESS;
		pD += CD.m_AnimGraph.GetAG2I()->OnCreateClientUpdate2(pD, pMirrorAGI, AGPackType);
	}

//	int nSize = (pD - _pData);
//	M_TRACE("OnCreateClientUpdate, %d, %d, %d\n", m_iObject, m_iClass, nSize);
	return (pD - _pData);
}


void CWObject_TentacleSystem::OnRefresh()
{
	CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	CD.OnRefresh();

	UpdateInteresting();

	// Evolves from tick base forward 1 tick...
	CMTime Time = CMTime::CreateFromTicks(m_pWServer->GetGameTick(), m_pWServer->GetGameTickTime(), 0.0f);
	CWAG2I_Context AG2IContext(this, m_pWServer, Time, m_pWServer->GetGameTickTime());
	CD.m_AnimGraph.GetAG2I()->CheckAnimEvents(&AG2IContext, OBJMSG_CHAR_ONANIMEVENT, ANIM_EVENT_MASK_EFFECT, false);

	uint32 AGDirty = CD.m_AnimGraph.m_bNeedUpdate ? TENTACLE_AG2_DIRTYMASK : 0;
	m_DirtyMask |= (CD.AutoVar_RefreshDirtyMask() | AGDirty) << CWO_DIRTYMASK_USERSHIFT;
}


void CWObject_TentacleSystem::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_TentacleSystem::OnClientRefresh);
	CWO_TentacleSystem_ClientData& CD = GetClientData(_pObj);
	CD.OnRefresh();
	// Evolves from tick base forward 1 tick...
	CMTime Time = CMTime::CreateFromTicks(_pWClient->GetGameTick(), _pWClient->GetGameTickTime(), 0.0f/*pCD->m_PredictFrameFrac*/);
	CWAG2I_Context AG2IContext(_pObj, _pWClient, Time,_pWClient->GetGameTickTime());
	CD.m_AnimGraph.GetAG2I()->CheckAnimEvents(&AG2IContext, OBJMSG_CHAR_ONANIMEVENT);
}


void CWObject_TentacleSystem::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _RenderMat)
{
	GetClientData(_pObj).OnRender(_pWClient, _pEngine);
}

//TODO
//The anim eval will happen in OnRender also, add a cache for it
CMat4Dfp32 CWObject_TentacleSystem::GetCreepCam(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CMat4Dfp32 &_WMat)
{
	CMat4Dfp32 Camera = _WMat;

	CWO_TentacleSystem_ClientData &CD = GetClientData(_pObj);

	int iModel = CD.m_lArmSetup[TENTACLE_DEMONHEAD1].m_liTemplateModels[1];
	CXR_Model* pModel = _pWPhysState->GetMapData()->GetResource_Model(iModel);
	if (!pModel)
		return Camera;

	CXR_Skeleton *pSkel;
	pSkel = pModel->GetSkeleton();

	CWorld_Client *pClient = safe_cast<CWorld_Client>(_pWPhysState);

	CWAG2I* pAGI = CD.m_AnimGraph.m_spAGI;
	CMTime Time = CMTime::CreateFromTicks(_pWPhysState->GetGameTick(), _pWPhysState->GetGameTickTime(), pClient->GetRenderTickFrac());
	CWAG2I_Context AGIContext(_pObj, _pWPhysState, Time);

	CXR_AnimLayer Layers[4];
	int nLayers = sizeof_buffer(Layers);
	pAGI->GetAnimLayersFromToken(&AGIContext, CD.m_lArmSetup[TENTACLE_DEMONHEAD1].m_iAGToken, Layers, nLayers, 0);

	CXR_SkeletonInstance SkelInst;
	pSkel->EvalAnim(Layers, nLayers, &SkelInst, Camera);
	
//	Camera.Create(SkelInst.m_pTracksRot[TENTACLE_CAMERA_BONE], SkelInst.m_pTracksMove[TENTACLE_CAMERA_BONE]);

	Camera = SkelInst.m_pBoneTransform[TENTACLE_CAMERA_BONE];
	Camera.GetRow(3) = pSkel->m_lNodes[TENTACLE_CAMERA_BONE].m_LocalCenter * SkelInst.m_pBoneTransform[TENTACLE_CAMERA_BONE];
	
	return Camera;
}

int CWObject_TentacleSystem::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	CBox3Dfp32 BBox;
	_pObj->GetVisBoundBox(BBox);

	const uint8 *pD = _pData;
	pD += CWObject::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);
	if (_pObj->m_iClass == 0 || (pD - _pData) == 0)
		return 0;

	CWO_TentacleSystem_ClientData& CD = GetClientData(_pObj);

	if (_pObj->m_bAutoVarDirty)
	{ // Handle AutoVars
		CD.AutoVar_Unpack(pD, _pWClient->GetMapData(), 0);
		
		// Create model instances if needed
		CD.UpdateModelInstances();
	}

	{ // Handle AnimGraph
		CWAG2I_Context AG2IContext(_pObj, _pWClient, _pWClient->GetGameTime());
		CWAG2I* pMirrorAGI = CD.m_AnimGraph.m_spMirror->GetWAG2I(0);
		CWAG2I* pAGI = ((_pWClient->GetClientMode() & WCLIENT_MODE_MIRROR) == 0) ? CD.m_AnimGraph.GetAG2I() : NULL;
		pD += CWAG2I::OnClientUpdate2(&AG2IContext, pMirrorAGI, pAGI, pD);
	}

	//_pObj->SetVisBoundBox(BBox);
	_pWClient->Object_SetVisBox(_pObj->m_iObject, BBox.m_Min, BBox.m_Max);

	for (uint i = TENTACLE_DEMONHEAD1; i <= TENTACLE_DEMONHEAD2; i++)
	{
		if (CD.m_lArmState.ValidPos(i) && CD.m_lArmState[i].m_State == TENTACLE_STATE_IDLE)
			CD.m_lArmState[i].m_CurrAttachDir = 0.0f;
	}

//	int nSize = (pD - _pData);
//	M_TRACE("OnClientUpdate, %d, %d, %d\n", _pObj->m_iObject, _pObj->m_iClass, nSize);
	return (pD - _pData);
}


void CWObject_TentacleSystem::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA];
	CWO_TentacleSystem_ClientData* pCD = TDynamicCast<CWO_TentacleSystem_ClientData>(pData);

	// Allocate clientdata
	if (!pCD || pCD->m_pObj != _pObj || pCD->m_pWPhysState != _pWPhysState)
	{
		pCD = MNew(CWO_TentacleSystem_ClientData);
		if (!pCD)
			Error_static("CWObject_TentacleSystem", "Could not allocate client data!");

		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = pCD;
		pCD->Clear(_pObj, _pWPhysState);
	}
}

void CWObject_TentacleSystem::OnClientLoad(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject::OnClientLoad(_pObj, _pWorld, _pFile, _pWData, _Flags);

	CWO_TentacleSystem_ClientData &CD = GetClientData(_pObj);
	CD.AutoVar_Read(_pFile, _pWData);
}

void CWObject_TentacleSystem::OnClientSave(CWObject_Client* _pObj, CWorld_Client* _pWorld, CCFile* _pFile, CMapData* _pWData, int _Flags)
{
	CWObject::OnClientSave(_pObj, _pWorld, _pFile, _pWData, _Flags);

	CWO_TentacleSystem_ClientData &CD = GetClientData(_pObj);
	CD.AutoVar_Write(_pFile, _pWData);
}

//
// Selection priority:
//
// 1) Live characters
// 2) Lamps 
// 3) Objects
// 4) Dead characters
//
CWObject* CWObject_TentacleSystem::SelectTarget(uint* _pSelectionMask)
{
	const CWObject_CoreData* pOwner = m_pWServer->Object_GetCD(m_iOwner);
	const CMat4Dfp32& Mat = pOwner ? pOwner->GetPositionMatrix() : GetPositionMatrix();

	const fp32 MaxArmLength = m_ArmMaxLength;	// Default: 6 meters
	const CVec3Dfp32& Dir = Mat.GetRow(0);
	CVec3Dfp32 StartPos = Mat.GetRow(3) + CVec3Dfp32(0,0,54);
	CVec3Dfp32 Center = StartPos + Dir * (MaxArmLength * 0.5f);
	fp32 Radius = MaxArmLength * 0.7f;
	CVec3Dfp32 EndPos;
	CCollisionInfo CInfo;

	uint SelectionMask = _pSelectionMask ? *_pSelectionMask : TENTACLE_SELECTION_ALL;

	uint nObjectFlags = 0;
	if (SelectionMask & TENTACLE_SELECTION_CHARACTER)
		nObjectFlags |= OBJECT_FLAGS_CHARACTER;

	if (SelectionMask & TENTACLE_SELECTION_CORPSE)
		nObjectFlags |= OBJECT_FLAGS_PICKUP;

	if (SelectionMask & TENTACLE_SELECTION_LAMP)
		nObjectFlags |= OBJECT_FLAGS_OBJECT;

	if (SelectionMask & TENTACLE_SELECTION_OBJECT)
		nObjectFlags |= OBJECT_FLAGS_OBJECT;

	if (SelectionMask & TENTACLE_SELECTION_SWINGDOOR)
		nObjectFlags |= OBJECT_FLAGS_PHYSOBJECT;

	CWObject* pBestObj = NULL;

	const int16* pSel = NULL;
	TSelection<CSelection::MEDIUM_BUFFER> Selection;
	m_pWServer->Selection_AddBoundSphere(Selection, nObjectFlags, Center, Radius);
	uint nSel = m_pWServer->Selection_Get(Selection, &pSel);
	if (nSel > 0)
	{
		fp32 BestScore = 0.0f;

		for (uint i = 0; i < nSel; i++)
		{
			int iObject = pSel[i];
			if (iObject == m_iObject || iObject == m_iOwner)
				continue;

			CWObject* pObj = m_pWServer->Object_Get(iObject);
			const CWO_PhysicsState& Phys = pObj->GetPhysState();

			uint nAcceptMask = 0;
			bool bIsChar = (pObj->IsClass(class_CharPlayer) || pObj->IsClass(class_CharNPC));
			bool bIsDead = false;
			bool bIsSwingdoor = false;
			if (bIsChar)
			{
				CWObject_Character* pChar = safe_cast<CWObject_Character>(pObj);
				// If target is carrying a shield snap that up and grab it
				bool bGotGrabbable = false;
				if (pChar && pChar->Char())
				{
					CRPG_Object_Item *pItem = pChar->Char()->GetEquippedItem(1);
					if (pItem && (pItem->m_Flags2 & RPG_ITEM_FLAGS2_GRABBABLEOBJECT))
					{
						// Check if we're infront of the character and he has a shield equipped
						CVec3Dfp32 Dir = GetPosition() - pChar->GetPosition();
						Dir.k[2] = 0.0f;
						Dir.Normalize();
						if (Dir * GetPositionMatrix().GetRow(0) < 0.0f)
						{
							// Ok, infront, so create an object here that the arm can grab hold of
							bGotGrabbable = true;
							nAcceptMask = TENTACLE_SELECTION_CHARACTERITEM;
						}
					}
				}
				if (!bGotGrabbable && pChar->m_Flags & PLAYER_FLAGS_RAGDOLL) // must have ragdoll...
				{
					bIsDead = (CWObject_Character::Char_GetPhysType(pChar) == PLAYER_PHYS_DEAD);
					if (bIsDead)
						nAcceptMask |= TENTACLE_SELECTION_CORPSE;

					//bool bIsZombie = (pChar->m_spAI->m_CharacterClass == CAI_Core::CLASS_UNDEAD);
					if (!pChar->IsImmune())// && !bIsZombie)
						nAcceptMask |= TENTACLE_SELECTION_CHARACTER;
				}
			}
			else if (pObj->IsClass(class_Object_Lamp))
			{
				if (!(pObj->Data(1) & M_Bit(1))) // check if lamp is broken
					nAcceptMask = TENTACLE_SELECTION_LAMP;
				else
					nAcceptMask = TENTACLE_SELECTION_OBJECT;
			}
			else if (Phys.m_ObjectFlags & OBJECT_FLAGS_OBJECT)
			{
				nAcceptMask = TENTACLE_SELECTION_OBJECT;
			}
			else if(pObj->IsClass(class_SwingDoor))
			{
				nAcceptMask = TENTACLE_SELECTION_SWINGDOOR;
				bIsSwingdoor = true;
			}

			if ((SelectionMask & nAcceptMask) == 0)
				continue;

			CVec3Dfp32 ObjPos;
			pObj->GetAbsBoundBox()->GetCenter(ObjPos);

			CVec3Dfp32 PosToObj = ObjPos - StartPos;
			fp32 ProjDist = Dir * PosToObj;
			if (ProjDist < 1.0f || ProjDist > MaxArmLength)
				continue;

			fp32 Distance = PosToObj.Length();
			fp32 CosAngle = ProjDist / Distance;
			if (CosAngle < 0.8f)
				continue;

			// test intersection
			EndPos = ObjPos;
			if (Phys.m_ObjectFlags & OBJECT_FLAGS_OBJECT || bIsSwingdoor)
			{
				// For objects, just trace forward (will look for grab point anyway)
				if (nAcceptMask == TENTACLE_SELECTION_OBJECT || bIsSwingdoor)
				{
					CWObject_Message Msg(OBJMSG_OBJECT_FIND_DEMONARMATTACHPOINT);
					Msg.m_VecParam0 = EndPos; // Msg.m_VecParam0 = StartPos; Msg.m_VecParam1 = Dir;
					const CVec3Dfp32* pAttachPoint = (const CVec3Dfp32*)pObj->OnMessage(Msg);
					if (!pAttachPoint)
						continue; // don't grab phys-objects that have no attachpoints

					EndPos = *pAttachPoint;
					EndPos *= pObj->GetPositionMatrix();
					//m_pWServer->Debug_RenderVertex(EndPos);
					//m_pWServer->Debug_RenderWire(EndPos, EndPos + CVec3Dfp32(0,0,100));
				}
				else
					EndPos = StartPos + Dir * ProjDist;
			}
			else if ((Phys.m_nPrim > 0) && (Phys.m_PhysFlags & OBJECT_PHYSFLAGS_OFFSET))
			{
				//If we have a physcluster, don't use the physprim offset
				CVec3Dfp32 Offset = (pObj->m_pPhysCluster) ? 0 : Phys.m_Prim[0].GetOffset();
				if (Phys.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION)
					Offset.MultiplyMatrix3x3(pObj->GetPositionMatrix());
				EndPos = pObj->GetPosition() + Offset;
			}

			//m_pWServer->Debug_RenderWire(StartPos, EndPos);

			bool bHit = m_pWServer->Phys_IntersectLine(StartPos, EndPos, 
				OBJECT_FLAGS_PROJECTILE, OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL, 
				XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS, &CInfo, m_iOwner);
			if (bHit && CInfo.m_bIsValid && CInfo.m_iObject != iObject && CInfo.m_iObject != m_iObject)
				continue;

			// Find grab-point
			if (nAcceptMask == TENTACLE_SELECTION_LAMP)
			{
				aint Ret = pObj->OnMessage( CWObject_Message(OBJMSG_CHAR_GETAUTOAIMOFFSET) );
				EndPos = CVec3Dfp32().Unpack32(Ret, 256.0f);
				EndPos *= pObj->GetPositionMatrix();
				m_pWServer->Debug_RenderVertex(EndPos);
			}
			else if (nAcceptMask == TENTACLE_SELECTION_OBJECT)
			{
				m_pWServer->Debug_RenderVertex(EndPos);
			}
			else if (nAcceptMask == TENTACLE_SELECTION_CHARACTERITEM)
			{
				// Get attachposition?
			}

			fp32 Score = CosAngle * Clamp01(1.0f - Distance / MaxArmLength);
			if (bIsChar && !bIsDead)
				Score += 1.0f;
			else if (nAcceptMask == TENTACLE_SELECTION_LAMP)
				Score += 0.6f;
			else if (nAcceptMask == TENTACLE_SELECTION_OBJECT)
				Score += 0.3f;
			else if (nAcceptMask == TENTACLE_SELECTION_CHARACTERITEM)
				Score += 1.0f;

			if (Score > BestScore)
			{
				BestScore = Score;
				pBestObj = pObj;
				m_LocalGrabPoint = EndPos;
				if (_pSelectionMask)
					*_pSelectionMask = nAcceptMask;
			}
		}
	}
	m_pSelectedObject = pBestObj;

	if (pBestObj)
	{
		CMat4Dfp32 InvMat;
		pBestObj->GetPositionMatrix().InverseOrthogonal(InvMat);
		m_LocalGrabPoint *= InvMat;
	}
	return pBestObj;
}


bool CWObject_TentacleSystem::IsIdle() const
{
	const CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	return CD.m_lArmState[TENTACLE_DEMONARM].IsIdle();
}


bool CWObject_TentacleSystem::IsDevouring() const
{
	const CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	return (CD.m_lArmState[TENTACLE_DEMONHEAD1].IsDevouring() | CD.m_lArmState[TENTACLE_DEMONHEAD2].IsDevouring());
}


void CWObject_TentacleSystem::SendDecalMsg(const CVec3Dfp32& _Pos0, const CVec3Dfp32& _Pos1, int32 _iTarget)
{
	CNetMsg DecalNetMsg(TENTACLE_NETMSG_CREATEDECAL);
	DecalNetMsg.AddInt16(_iTarget);

	DecalNetMsg.Addfp32(_Pos0.k[0]);
	DecalNetMsg.Addfp32(_Pos0.k[1]);
	DecalNetMsg.Addfp32(_Pos0.k[2]);

	DecalNetMsg.Addfp32(_Pos1.k[0]);
	DecalNetMsg.Addfp32(_Pos1.k[1]);
	DecalNetMsg.Addfp32(_Pos1.k[2]);

	// Send to ourself to handle on client
	m_pWServer->NetMsg_SendToObject(DecalNetMsg, m_iObject);
}


bool CWObject_TentacleSystem::ReleaseObject(const CVec3Dfp32* _pControlMove)
{
	CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	CTentacleArmState& DemonArm = CD.m_lArmState[TENTACLE_DEMONARM];

	if (DemonArm.m_State == TENTACLE_STATE_RETURN || 
		DemonArm.m_State == TENTACLE_STATE_IDLE ||
		DemonArm.m_State == TENTACLE_STATE_OFF)
		return true;

	if (DemonArm.m_Task == TENTACLE_TASK_BREAKOBJECT) 
		return false; // let it break the object without being interrupted, thank you

	if (DemonArm.m_Task == TENTACLE_TASK_GRABHOLD && DemonArm.m_State == TENTACLE_STATE_GRABOBJECT)
	{
		CWObject* pTarget = NULL;
		pTarget = m_pWServer->Object_Get(DemonArm.m_iTarget);
		CWObject_Character* pChar = TDynamicCast<CWObject_Character>(pTarget); //didn't work: CWObject_Character::IsCharacter(pTarget);
		if (pChar && CWObject_Character::Char_GetPhysType(pChar) != PLAYER_PHYS_DEAD)
		{
			pChar->OnMessage(CWObject_Message(OBJMSG_CHAR_GRABBED_BY_DEMONARM));
			// TEMP: Insta-kill the character when dropping it
			//pChar->Char_Die(DAMAGETYPE_DARKNESS, m_iOwner); 
			// Move behavior to exit...?
			CWO_Character_ClientData* pCharCD = CWObject_Character::GetClientData(pChar);
			if (pCharCD)
			{
				// Behavior exit impulse
				CWAG2I_Context AGContext(pChar,m_pWServer,CMTime::CreateFromTicks(pCharCD->m_GameTick,m_pWServer->GetGameTickTime()));
				CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_BEHAVIORCONTROL, AG2_IMPULSEVALUE_BEHAVIORCONTROL_RAGDOLLEXIT);
				pCharCD->m_AnimGraph2.GetAG2I()->SendImpulse(&AGContext, Impulse);
			}
		}
		// Send damage (if any)
		if (CD.m_nDamage)
		{
			CVec3Dfp32 Pos = DemonArm.m_Pos;
			CVec3Dfp32 Force = DemonArm.m_Dir * 10.0f; // tweakme
			CWO_DamageMsg Msg(CD.m_nDamage, DAMAGETYPE_DARKNESS, &Pos, &Force);
			Msg.Send(DemonArm.m_iTarget, m_iOwner, m_pWServer);
		}
	}

	// throw current object away?
	if (_pControlMove && _pControlMove->LengthSqr() > 0.01f)
	{
		// TODO: enter a new state and do some movement before throwing?
		const CMat4Dfp32& MatLook = (m_iOwner > 0) ? m_pWServer->Object_GetPositionMatrix(m_iOwner) : GetPositionMatrix();
		CVec3Dfp32 Dir;
		Dir  = MatLook.GetRow(0) * _pControlMove->k[0];
		Dir += MatLook.GetRow(1) * _pControlMove->k[1];

		CD.Server_ThrowObject(DemonArm, Dir);
	}

	CMat4Dfp32 LastAttachMat;
	CSpline_Tentacle Spline;
	CD.GetSpline(TENTACLE_DEMONARM, Spline, false, &LastAttachMat);
	M_ASSERT(Spline.m_Length < 100000.0f, "invalid arm length!");

	// Send decal message
	if (DemonArm.m_iTarget)
		SendDecalMsg(LastAttachMat.GetRow(3), DemonArm.m_Pos, DemonArm.m_iTarget);

	uint16 CurrentTarget = DemonArm.m_iTarget;

	DemonArm.m_iTarget = 0;
	M_TRACEALWAYS(CStr("Tentacle: DemonArm Release Object\n"));
	DemonArm.m_TargetPos = DemonArm.m_Pos;
	DemonArm.m_Length = Spline.m_Length;
	fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(-12.0f);
	DemonArm.UpdateTask(TENTACLE_TASK_IDLE, TENTACLE_STATE_RETURN, Speed);
	CD.m_lArmState.MakeDirty();

	// Send releasegrab message last (to avoid infinite recursion if we for example disable darknesspowers OnRelease on an object :P)
	CWObject_Message Msg(OBJMSG_OBJECT_DEMONARM_RELEASED);
	Msg.m_Param1 = m_iOwner;
	m_pWServer->Message_SendToObject(Msg, CurrentTarget);

	return true;
}


void CWObject_TentacleSystem::GetNothing(const CVec3Dfp32& _GotoPos)
{
	CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	CTentacleArmState& DemonArm = CD.m_lArmState[TENTACLE_DEMONARM];
	if (!DemonArm.IsIdle() && !ReleaseObject())
		return;

/*	DemonArm.m_PhysArm_Pid0.Reset(GetPosition());
	DemonArm.m_PhysArm_Pid1.Reset(GetPosition());*/

	DemonArm.m_iTarget = 0;
	DemonArm.m_TargetPos = _GotoPos;
	M_TRACEALWAYS(CStr("Tentacle: Demonarm get nothing.\n"));
	fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(24.0f);
	DemonArm.UpdateTask(TENTACLE_TASK_GETNOTHING, TENTACLE_STATE_REACHPOSITION, Speed);
	CD.m_lArmState.MakeDirty();

	// Play 'reach' sound (once)
	int iReachSound = CD.m_liSounds[TENTACLE_SOUND_REACH];
	if (iReachSound)
		m_pWServer->Sound_On(m_iObject, iReachSound, WCLIENT_ATTENUATION_3D);

	// Start 'hold' sound (looping on the object we're holding)
	iSound(0) = CD.m_liSounds[TENTACLE_SOUND_HOLD];
}


void CWObject_TentacleSystem::BreakObject(CWObject& _Object, uint _nDamage)
{
	CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	CTentacleArmState& DemonArm = CD.m_lArmState[TENTACLE_DEMONARM];
	if (!DemonArm.IsIdle() && !ReleaseObject())
		return;

	DemonArm.m_iRotTrack = PLAYER_ROTTRACK_SPINE;
	DemonArm.m_iTarget = _Object.m_iObject;
	DemonArm.m_GrabPoint = m_LocalGrabPoint;
	fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(24.0f);
	DemonArm.UpdateTask(TENTACLE_TASK_BREAKOBJECT, TENTACLE_STATE_REACHOBJECT, Speed);
	CD.m_lArmState.MakeDirty();
	CD.m_nDamage = _nDamage;

	// Play 'reach' sound (once)
	int iReachSound = CD.m_liSounds[TENTACLE_SOUND_REACH];
	if (iReachSound)
		m_pWServer->Sound_On(m_iObject, iReachSound, WCLIENT_ATTENUATION_3D);
}


void CWObject_TentacleSystem::PushObject(CWObject& _Object, fp32 _GrabPower, uint _nDamage)
{
	CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	CTentacleArmState& DemonArm = CD.m_lArmState[TENTACLE_DEMONARM];
	if (!DemonArm.IsIdle() && !ReleaseObject())
		return;

	DemonArm.m_iRotTrack = PLAYER_ROTTRACK_SPINE;
	DemonArm.m_iTarget = _Object.m_iObject;
	DemonArm.m_GrabPoint = m_LocalGrabPoint;
	fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(24.0f);
	DemonArm.UpdateTask(TENTACLE_TASK_PUSHOBJECT, TENTACLE_STATE_REACHOBJECT, Speed);
	CD.m_lArmState.MakeDirty();
	CD.m_nDamage = _nDamage;
	CD.m_GrabPower = _GrabPower;

	// Play 'reach' sound (once)
	int iReachSound = CD.m_liSounds[TENTACLE_SOUND_REACH];
	if (iReachSound)
		m_pWServer->Sound_On(m_iObject, iReachSound, WCLIENT_ATTENUATION_3D);
}


void CWObject_TentacleSystem::GrabObject(CWObject& _Object, fp32 _GrabPower, uint _nDamage)
{
	CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	CTentacleArmState& DemonArm = CD.m_lArmState[TENTACLE_DEMONARM];
	if (!DemonArm.IsIdle() && !ReleaseObject())
		return;

	DemonArm.m_iTarget = _Object.m_iObject;
	DemonArm.m_iRotTrack = PLAYER_ROTTRACK_SPINE;
	DemonArm.m_GrabPoint = m_LocalGrabPoint;
	fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(24.0f);
	DemonArm.UpdateTask(TENTACLE_TASK_GRABHOLD, TENTACLE_STATE_REACHOBJECT, Speed);
	CD.m_lArmState.MakeDirty();
	CD.m_GrabPower = _GrabPower;
	CD.m_nDamage = _nDamage;

	// Play 'reach' sound (once)
	int iReachSound = CD.m_liSounds[TENTACLE_SOUND_REACH];
	if (iReachSound)
		m_pWServer->Sound_On(m_iObject, iReachSound, WCLIENT_ATTENUATION_3D);

	// Start 'hold' sound (looping on the object we're holding)
	iSound(0) = CD.m_liSounds[TENTACLE_SOUND_HOLD];

	CWObject_Message Msg(OBJMSG_OBJECT_DEMONARM_GRABBED);
	Msg.m_Param1 = m_iOwner; // Owner is activator
	m_pWServer->Message_SendToObject(Msg, _Object.m_iObject);
}

void CWObject_TentacleSystem::GrabCharacterObject(CWObject& _Object, fp32 _GrabPower)
{
	CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	CTentacleArmState& DemonArm = CD.m_lArmState[TENTACLE_DEMONARM];
	if (!DemonArm.IsIdle() && !ReleaseObject())
		return;

	DemonArm.m_iTarget = _Object.m_iObject;
	DemonArm.m_iRotTrack = PLAYER_ROTTRACK_SPINE;
	DemonArm.m_GrabPoint = m_LocalGrabPoint;
	fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(24.0f);
	DemonArm.UpdateTask(TENTACLE_TASK_GRABCHAROBJECT, TENTACLE_STATE_REACHOBJECT, Speed);
	CD.m_lArmState.MakeDirty();
	CD.m_GrabPower = _GrabPower;

	// Play 'reach' sound (once)
	int iReachSound = CD.m_liSounds[TENTACLE_SOUND_REACH];
	if (iReachSound)
		m_pWServer->Sound_On(m_iObject, iReachSound, WCLIENT_ATTENUATION_3D);

	// Start 'hold' sound (looping on the object we're holding)
	iSound(0) = CD.m_liSounds[TENTACLE_SOUND_HOLD];
}


bool CWObject_TentacleSystem::GrabAndDevour(CWObject& _Object, bool _bExtractDarkling)
{
	CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	
	CTentacleArmState& DemonHead1 = CD.m_lArmState[TENTACLE_DEMONHEAD1];
	CTentacleArmState& DemonHead2 = CD.m_lArmState[TENTACLE_DEMONHEAD2];
	CTentacleArmState& DemonArm = CD.m_lArmState[TENTACLE_DEMONARM];

	// If demon arm is active, make it return
	if(DemonArm.m_State != TENTACLE_STATE_IDLE)
	{
		DemonArm.m_iTarget = 0;
		fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(-12.0f);
		DemonArm.UpdateState(TENTACLE_STATE_RETURN, Speed);
	}

	// Is any of the arms devouring?
	if(DemonHead1.IsDevouring() || DemonHead2.IsDevouring())
		return false;

	// Link heads
	DemonHead1.m_pControlArm = &DemonHead2;
	DemonHead2.m_pControlArm = &DemonHead1;

	// Update queue
	DemonHead1.UpdateTaskQueue();
	DemonHead2.UpdateTaskQueue();
	DemonHead1.m_QueueSpeed = PHYSSTATE_CONVERTFROM20HZ(8.0f);
	DemonHead2.m_QueueSpeed = PHYSSTATE_CONVERTFROM20HZ(8.0f);

	// Set devour states and prepare to extrude down to character (Fast!)
	fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(12.0f);
	DemonHead1.UpdateTask(TENTACLE_TASK_DEVOURTARGET, TENTACLE_STATE_WIGGLE, Speed);
	DemonHead2.UpdateTask(TENTACLE_TASK_DEVOURTARGET, TENTACLE_STATE_WIGGLE, Speed);
	DemonHead1.m_iRotTrack = TENTACLE_DEVOUR_ROTTRACK_HEART;
	DemonHead1.m_iTarget = _Object.m_iObject;

	// If we haven't got the arms active when starting devour, make sure they return properly
	if (DemonHead1.IsIdle())
		DemonHead1.UpdateTask(TENTACLE_TASK_WIGGLE, TENTACLE_STATE_RETURN, PHYSSTATE_CONVERTFROM20HZ(-6.0f));

	if (DemonHead2.IsIdle())
		DemonHead2.UpdateTask(TENTACLE_TASK_WIGGLE, TENTACLE_STATE_RETURN, PHYSSTATE_CONVERTFROM20HZ(-6.0f));

	CD.m_lArmState.MakeDirty();

	// Play 'reach' sound (once)
	int iReachSound = CD.m_liSounds[TENTACLE_SOUND_REACH];
	if (iReachSound)
		m_pWServer->Sound_On(m_iObject, iReachSound, WCLIENT_ATTENUATION_3D);

	return true;
}


void CWObject_TentacleSystem::UpdateArmControl(const CVec3Dfp32& _ControlMove)
{
	CWO_TentacleSystem_ClientData& CD = GetClientData(this);

	CD.m_ArmControl.k[0] += _ControlMove.k[0] * 4.0f;
//	CD.m_ArmControl.k[1] -= _ControlMove.k[1] * 0.003f;

	// moderate values
	CD.m_ArmControl.k[0] = Min(Max(CD.m_ArmControl.k[0], m_ArmMinLength), m_ArmMaxLength);
//	CD.m_ArmControl.k[1] = Sign(CD.m_ArmControl.k[1]) * Min(M_Fabs(CD.m_ArmControl.k[1]), 0.1f);
}


bool CWObject_TentacleSystem::GetMainTentaclePos(CVec3Dfp32* _pPos) const
{
	const CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	const CTentacleArmState& DemonArm = CD.m_lArmState[TENTACLE_DEMONARM];
	if (DemonArm.IsIdle())
		return false; // is this correct?

	*_pPos = CD.m_lArmState[TENTACLE_DEMONARM].m_Pos;
	return true;
}


void CWObject_TentacleSystem::ActivateDemonHeads(bool _bActivate)
{
	CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	bool bIsDirty = false;

	M_TRACE("ActivateDemonHeads: %d\n", _bActivate);

	for (uint iHead = TENTACLE_DEMONHEAD1; iHead <= TENTACLE_DEMONHEAD2; iHead++)
	{
		CTentacleArmState& DemonHead = CD.m_lArmState[iHead];

		// already deactive?
		if (!_bActivate && (DemonHead.m_Task == TENTACLE_TASK_IDLE && DemonHead.m_State == TENTACLE_STATE_IDLE))
			continue;

		// already active?
		bool bIsActive = (DemonHead.m_Task == TENTACLE_TASK_WIGGLE) && (DemonHead.m_State == TENTACLE_STATE_WIGGLE || DemonHead.m_State == TENTACLE_STATE_INTERESTED);
		if (_bActivate && bIsActive)
			continue;

		uint Task  = _bActivate ? TENTACLE_TASK_WIGGLE : TENTACLE_TASK_IDLE;
		uint State = _bActivate ? TENTACLE_STATE_WIGGLE : TENTACLE_STATE_RETURN;

		fp32 Speed  = PHYSSTATE_CONVERTFROM20HZ(_bActivate ? 12.0f : -3.0f);

		// Queue up or activate
		if (DemonHead.IsDevouring())
			DemonHead.UpdateTaskQueue(Task, State, Speed);
		else
			DemonHead.UpdateTask(Task, State, Speed);

		bIsDirty = true;
	}

	if (CD.m_lArmState.Len() > TENTACLE_SCREENSTUFF)
	{
		CTentacleArmState& ScreenStuff = CD.m_lArmState[TENTACLE_SCREENSTUFF];

		CWAG2I_Context AGContext(this, m_pWServer, m_pWServer->GetGameTime(), m_pWServer->GetGameTickTime());
		bool bUpdate = false;

		if (_bActivate && (ScreenStuff.m_State != TENTACLE_STATE_WIGGLE2))
		{
			fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(1.0f);
			ScreenStuff.UpdateState(TENTACLE_STATE_WIGGLE2, Speed);
			ScreenStuff.m_Length = 1.0f;
			CXRAG2_Impulse Impulse(TENTACLE_AG2_IMPULSETYPE_GUITENTACLE, TENTACLE_AG2_IMPULSEVALUE_GUITENTACLE_START);
			bUpdate = CD.m_AnimGraph.GetAG2I()->SendImpulse(&AGContext, Impulse, CD.m_lArmSetup[TENTACLE_SCREENSTUFF].m_iAGToken);
		}
		else if (!_bActivate && (ScreenStuff.m_State == TENTACLE_STATE_WIGGLE2))
		{
			fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(-0.1f);
			ScreenStuff.UpdateState(TENTACLE_STATE_RETURN, Speed);
			CXRAG2_Impulse Impulse(TENTACLE_AG2_IMPULSETYPE_GUITENTACLE, TENTACLE_AG2_IMPULSEVALUE_GUITENTACLE_END);
			bUpdate = CD.m_AnimGraph.GetAG2I()->SendImpulse(&AGContext, Impulse, CD.m_lArmSetup[TENTACLE_SCREENSTUFF].m_iAGToken);
		}
		CD.m_AnimGraph.m_bNeedUpdate = CD.m_AnimGraph.m_bNeedUpdate || bUpdate;
		m_DirtyMask |= (CD.m_AnimGraph.m_bNeedUpdate ? TENTACLE_AG2_DIRTYMASK : 0) << CWO_DIRTYMASK_USERSHIFT;
	}

	if (bIsDirty)
		CD.m_lArmState.MakeDirty();
}


bool CWObject_TentacleSystem::IsCreepingDark() const
{
	const CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	return (CD.m_lArmState[TENTACLE_DEMONHEAD1].IsCreepingDark() | CD.m_lArmState[TENTACLE_DEMONHEAD2].IsCreepingDark());
}


bool CWObject_TentacleSystem::StartCreepingDark(const CVec3Dfp32& _StartPos)
{
	CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	CTentacleArmState& DemonHead = CD.m_lArmState[TENTACLE_DEMONHEAD1];

	if (!CD.CanStartCreepingDark(TENTACLE_DEMONHEAD1, _StartPos))
		return false;

	// Remove any target pos we might have and update the task queue and switch state
	DemonHead.m_TargetPos = 0;
	M_TRACEALWAYS(CStr("Tentacle: DemonArm StartCreepingDark, targetpos clear!\n"));
	DemonHead.UpdateTaskQueue();
	fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(20.0f);
	DemonHead.UpdateTask(TENTACLE_TASK_TRAIL, TENTACLE_STATE_TRAIL, Speed);
	CD.m_lArmState.MakeDirty();

	// Start creeping dark movement hold
	CXRAG2_Impulse Impulse(TENTACLE_AG2_IMPULSETYPE_DEMONHEAD, TENTACLE_AG2_IMPULSEVALUE_CREEPINGDARKMH);
	CWAG2I_Context AGContext(this, m_pWServer, m_pWServer->GetGameTime(), m_pWServer->GetGameTickTime());
	CD.m_AnimGraph.m_bNeedUpdate = CD.m_AnimGraph.GetAG2I()->SendImpulse(&AGContext, Impulse, CD.m_lArmSetup[TENTACLE_DEMONHEAD1].m_iAGToken) || CD.m_AnimGraph.m_bNeedUpdate;
	m_DirtyMask |= CD.m_AnimGraph.m_bNeedUpdate ? TENTACLE_AG2_DIRTYMASK : 0 << CWO_DIRTYMASK_USERSHIFT;

	if (CD.m_lArmState.Len() > TENTACLE_CREEPINGDARK_SQUID)
	{
		CTentacleArmState& Squid = CD.m_lArmState[TENTACLE_CREEPINGDARK_SQUID];

		CWAG2I_Context AGContext(this, m_pWServer, m_pWServer->GetGameTime(), m_pWServer->GetGameTickTime());
		bool bUpdate = false;
		if (Squid.m_State != TENTACLE_STATE_WIGGLE2)
		{
			fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(1.0f);
			Squid.UpdateState(TENTACLE_STATE_WIGGLE2, Speed);
			Squid.m_Length = 1.0f;
			CXRAG2_Impulse Impulse(TENTACLE_AG2_IMPULSETYPE_TENTACLESQUID, TENTACLE_AG2_IMPULSEVALUE_TENTACLESQUID_CROUCH);
			bUpdate = CD.m_AnimGraph.GetAG2I()->SendImpulse(&AGContext, Impulse, CD.m_lArmSetup[TENTACLE_CREEPINGDARK_SQUID].m_iAGToken);
		}
		CD.m_AnimGraph.m_bNeedUpdate = CD.m_AnimGraph.m_bNeedUpdate || bUpdate;
		m_DirtyMask |= (CD.m_AnimGraph.m_bNeedUpdate ? TENTACLE_AG2_DIRTYMASK : 0) << CWO_DIRTYMASK_USERSHIFT;
	}

	return true;
}


void CWObject_TentacleSystem::StopCreepingDark()
{
	CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	CTentacleArmState& DemonHead = CD.m_lArmState[TENTACLE_DEMONHEAD1];

	DemonHead.UpdateState(TENTACLE_STATE_RETURN, -20.0f);
	CD.m_lArmState.MakeDirty();

	if (CD.m_lArmState.Len() > TENTACLE_CREEPINGDARK_SQUID)
	{
		CTentacleArmState& Squid = CD.m_lArmState[TENTACLE_CREEPINGDARK_SQUID];

		CWAG2I_Context AGContext(this, m_pWServer, m_pWServer->GetGameTime(), m_pWServer->GetGameTickTime());
		bool bUpdate = false;
		if (Squid.m_State == TENTACLE_STATE_WIGGLE2)
		{
			fp32 Speed = PHYSSTATE_CONVERTFROM20HZ(-0.1f);
			Squid.UpdateState(TENTACLE_STATE_RETURN, Speed);
			CXRAG2_Impulse Impulse(TENTACLE_AG2_IMPULSETYPE_TENTACLESQUID, TENTACLE_AG2_IMPULSEVALUE_TENTACLESQUID);
			bUpdate = CD.m_AnimGraph.GetAG2I()->SendImpulse(&AGContext, Impulse, CD.m_lArmSetup[TENTACLE_CREEPINGDARK_SQUID].m_iAGToken);
		}
		CD.m_AnimGraph.m_bNeedUpdate = CD.m_AnimGraph.m_bNeedUpdate || bUpdate;
		m_DirtyMask |= (CD.m_AnimGraph.m_bNeedUpdate ? TENTACLE_AG2_DIRTYMASK : 0) << CWO_DIRTYMASK_USERSHIFT;
	}
}


void CWObject_TentacleSystem::DoHurtResponse()
{
	CWO_TentacleSystem_ClientData& CD = GetClientData(this);
	CWAG2I_Context AGContext(this, m_pWServer, m_pWServer->GetGameTime());
	CTentacleArmSetup& DemonHead1 = CD.m_lArmSetup[TENTACLE_DEMONHEAD1];
	CTentacleArmSetup& DemonHead2 = CD.m_lArmSetup[TENTACLE_DEMONHEAD2];
	
	// Run hurt
	int Direction1 = TENTACLE_AG2_DIRECTION_LEFT + TruncToInt(Random * 2.99f);
	bool bHurt1 = CD.m_AnimGraph.Token_Check(DemonHead1.m_iAGToken, TENTACLE_AG2_STATEFLAG_HURTACTIVE);
	bool bHurt2 = CD.m_AnimGraph.Token_Check(DemonHead2.m_iAGToken, TENTACLE_AG2_STATEFLAG_HURTACTIVE);
	if (bHurt1 && bHurt2)
	{
		// Both heads are hurt, pick random head to play new anim on
		if (m_pWServer->GetGameTick() & 1) CD.m_AnimGraph.Anim_Hurt(&AGContext, DemonHead1.m_iAGToken, Direction1);
		else CD.m_AnimGraph.Anim_Hurt(&AGContext, DemonHead2.m_iAGToken, Direction1);
	}
	else if (bHurt1 || bHurt2)
	{
		// Make opposite head take a beating
		CTentacleArmSetup& DemonHead = (bHurt1) ? DemonHead2 : DemonHead1;
		CD.m_AnimGraph.Anim_Hurt(&AGContext, DemonHead.m_iAGToken, Direction1);
	}
	else
	{
		int Direction2 = TENTACLE_AG2_DIRECTION_LEFT + TruncToInt(Random * 2.99f);
		CD.m_AnimGraph.Anim_Hurt(&AGContext, DemonHead1.m_iAGToken, Direction1);
		CD.m_AnimGraph.Anim_Hurt(&AGContext, DemonHead2.m_iAGToken, Direction2);
	}

	// Make dirty if needed
	m_DirtyMask |= (CD.m_AnimGraph.m_bNeedUpdate ? TENTACLE_AG2_DIRTYMASK : 0) << CWO_DIRTYMASK_USERSHIFT;
}


void CWObject_TentacleSystem::UpdateInteresting()
{
	CWObject* pOwner = m_pWServer->Object_Get(m_iOwner);
	if (pOwner)
	{
		CWObject_Character* pOwnerChar = TDynamicCast<CWObject_Character>(pOwner);
		if (pOwnerChar && pOwnerChar->m_spAI)
		{
			int32 Type;
			CVec3Dfp32 aPos[2];
			int nInteresting = pOwnerChar->m_spAI->GetInterestingObjects(Type, aPos[0], aPos[1]);

			CWO_TentacleSystem_ClientData& CD = GetClientData(this);
			for (uint iHead = TENTACLE_DEMONHEAD1; iHead <= TENTACLE_DEMONHEAD2; iHead++)
			{
				CTentacleArmState& DemonHead = CD.m_lArmState[iHead];
				bool bInteresting = false;
				if (DemonHead.m_State == TENTACLE_STATE_WIGGLE || DemonHead.m_State == TENTACLE_STATE_INTERESTED)
				{
					if (nInteresting > 0)
					{
						// Changed type?
						if (Type != m_InterestingType)
						{
							//INTERESTING_NOTHING = -1,
							//INTERESTING_LOOK = 0,
							//INTERESTING_FRIEND = 1,
							//INTERESTING_HOSTILE = 2,
							//INTERESTING_ENEMY = 3,
						}

						// Make sure Server_UpdateAnimGraph will update accordingly
						if (Type >= CAI_Core::INTERESTING_HOSTILE)
						{
							if (!DemonHead.m_bHostileNearby)
								DemonHead.m_bHostileNearby = true;
						}
						else if (DemonHead.m_bHostileNearby)
							DemonHead.m_bHostileNearby = false;

						// Pick best position
						const CMat4Dfp32& PosMat = pOwner->GetPositionMatrix();
						CVec3Dfp32 RefDir = CVec3Dfp32(1.0f, ((iHead&1) - 0.5f)*-0.2f, 0.0f).Normalize();
						CVec3Dfp32 RefPos = CVec3Dfp32(10.0f, ((iHead&1) - 0.5f)*-20.0f, 0.0f);
						RefDir.MultiplyMatrix3x3(PosMat);
						RefPos *= PosMat;  RefPos.k[2] += 56.0f;
						CVec3Dfp32 Pos = aPos[0];
						CVec3Dfp32 Diff = (Pos - RefPos);
						fp32 InvLen = Diff.LengthInv();
						fp32 Dot = (Diff * RefDir) * InvLen;

		//m_pWServer->Debug_RenderVector(RefPos, RefDir * 30.0f, 0xff00ff00, 0.0f, false);
		//m_pWServer->Debug_RenderVector(RefPos, (Diff*InvLen) * 30.0f, 0xff0000ff, 0.0f, false);

						if (nInteresting > 1)
						{
							CVec3Dfp32 Diff2 = (aPos[1] - RefPos);
							fp32 InvLen2 = Diff2.LengthInv();
							fp32 Dot2 = (Diff2 * RefDir) * InvLen2;

							if (Dot2 > 0.6f && (Dot2 * InvLen2 > Dot * InvLen))
							{
								Pos = aPos[1];
								Dot = Dot2;
								InvLen = InvLen2;
							}
						}

						if (Dot > 0.6f && InvLen > 0.0031f)
						{
							fp32 DistSqr = DemonHead.m_TargetPos.DistanceSqr(Pos);
							if (DistSqr > 1.0f || (DemonHead.m_State != TENTACLE_STATE_INTERESTED))
							{
								//M_TRACE("iHead: %d,  interested! pos: %s\n", iHead, Pos.GetString().Str());
								DemonHead.m_State = TENTACLE_STATE_INTERESTED;
								DemonHead.m_TargetPos = Pos;
								TENTACLES_TRACE("Tentacle: UpdateInteresting. Set interesting pos\n");
								CD.m_lArmState.MakeDirty();
							}
							bInteresting = true;
						}
					}

					if (!bInteresting)
					{
						if (DemonHead.m_bHostileNearby && nInteresting < 1)
							DemonHead.m_bHostileNearby = false;

						// Nothing interesting...
						if (DemonHead.m_State == TENTACLE_STATE_INTERESTED)
						{
							//M_TRACE("iHead: %d,  not interested...\n", iHead);
							DemonHead.m_State = TENTACLE_STATE_WIGGLE;
							DemonHead.m_TargetPos = 0.0f;
							TENTACLES_TRACE("Tentacle: UpdateInteresting, clear target pos!\n");
							DemonHead.m_LastTargetPos = 0.0f;
							CD.m_lArmState.MakeDirty();

							// Willbo:
							// Is there any special animations for just idling around in look mode? If so we should probably stop animations here.
						}
						else if (DemonHead.m_State == TENTACLE_STATE_WIGGLE && DemonHead.m_Task == TENTACLE_TASK_WIGGLE && DemonHead.m_TargetPos != CVec3Dfp32(0))
						{
							// Now something isn't quite right actually. Reset it.
							ConOut("Forcing tentacles target position to zero!!");
							DemonHead.m_TargetPos = 0.0f;
							DemonHead.m_LastTargetPos = 0.0f;
							CD.m_lArmState.MakeDirty();
						}
					}
				}
			}

			m_InterestingType = Type;
		}
	}
}


fp32 CWObject_TentacleSystem::GetArmMaxLength()
{
	return m_ArmMaxLength;
}
