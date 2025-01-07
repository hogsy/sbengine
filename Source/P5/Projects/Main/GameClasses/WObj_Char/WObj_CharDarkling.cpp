/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character class for Darkling characters.

	Author:			Anton Ragnarsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CWObject_CharDarkling implementation

	Comments:

	History:		
		050311		Created file
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_CharDarkling.h"
#include "../WObj_Misc/WObj_ScenePoint.h"
#include "../WObj_Misc/WObj_EffectSystem.h"

#include "../WObj_AI/AI_Custom/AICore_Darkling.h"

#define CWO_DARKLING_SPAWNTIME	2.0f	// Spawn time set to 2 seconds!

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_CharDarkling, CWObject_CharPlayer, 0x0100);



CWObject_CharDarkling::CWObject_CharDarkling()
{
	m_iLightHurtObj = 0;
}

const CWO_CharDarkling_ClientData& CWObject_CharDarkling::GetClientData(const CWObject_CoreData* _pObj)
{
	M_ASSERT(_pObj, "[CWO_CharDarkling_ClientData] Bad this-pointer!");
	const CWO_CharDarkling_ClientData* pCD = safe_cast<const CWO_CharDarkling_ClientData>((const CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	M_ASSERT(pCD, "[CWObject_CharDarkling] No clientdata?!");
	return *pCD;
}


CWO_CharDarkling_ClientData& CWObject_CharDarkling::GetClientData(CWObject_CoreData* _pObj)
{
	M_ASSERT(_pObj, "[CWO_Character_ClientData] Bad this-pointer!");
	CWO_CharDarkling_ClientData* pCD = safe_cast<CWO_CharDarkling_ClientData>((CReferenceCount*) _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]);
	M_ASSERT(pCD, "[CWObject_CharDarkling] No clientdata?!");
	return *pCD;
}


void CWObject_CharDarkling::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA];
	CWO_CharDarkling_ClientData* pCD = TDynamicCast<CWO_CharDarkling_ClientData>(pData);

	if (!pCD || pCD->m_pObj != _pObj || pCD->m_pWPhysState != _pWPhysState)
	{
		pCD = MNew(CWO_CharDarkling_ClientData);
		if (!pCD)
			Error_static("CWObject_CharDarkling", "Could not allocate client data!");

		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = pCD;
		pCD->Clear(_pObj, _pWPhysState);
	}

	if (!InitClientObjects(_pObj, _pWPhysState))
		Error_static("CWObject_CharDarkling", "InitClientObjects failed");
}


void CWObject_CharDarkling::OnInitInstance(const aint* _pParam, int _nParam)
{
	CWO_CharDarkling_ClientData& CD = GetClientData();
	if (_pParam && _nParam > 0)
		CD.m_iPlayer = (int16)_pParam[0];
}


void CWObject_CharDarkling::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	CWO_CharDarkling_ClientData& CD = GetClientData();

	const CStr KeyName = _pKey->GetThisName();
	CStr KeyValue = _pKey->GetThisValue();
		
	switch (_KeyHash)
	{
	case MHASH5('WORL','DSPA','CEMO','VEME','NT'): // "WORLDSPACEMOVEMENT"
		{
			if (KeyValue.Val_int() != 0)
				CD.m_Flags = CD.m_Flags | DARKLING_FLAGS_WORLDSPACE;
			break;
		}
	case MHASH4('GRAV','ITYF','REET','IME'): // "GRAVITYFREETIME"
		{
			fp32 MaxTime = KeyValue.Val_fp64();
			CD.m_GravityFreeMaxTicks = int(MaxTime * m_pWServer->GetGameTicksPerSecond());
			break;
		}
	case MHASH4('EYET','RAIL','MODE','L'): // "EYETRAILMODEL"
		{
			CD.m_iEyeTrailModel = (KeyValue != "") ? m_pWServer->GetMapData()->GetResourceIndex_Model(KeyValue) : 0;
			break;
		}
	default:
		{
			parent::OnEvalKey(_KeyHash, _pKey);
			break;
		}
	}
}


void CWObject_CharDarkling::OnIncludeClass(CMapData* _pWData, CWorld_Server* _pWServer)
{
	parent::OnIncludeClass(_pWData, _pWServer);

	_pWData->GetResourceIndex_Model("EffectSystem:FXEffect,fx_darklingdeath");
	_pWData->GetResourceIndex_Sound("D_Darkling_Hrt_Charge_01");

	IncludeClass("Darkling_SpawnSmokeLayer", _pWData);
	IncludeClass("Darkling_LightHurt", _pWData);

	IncludeModel("EffectSystem:FXEffect,fx_darkling_lighthurt_1", _pWData);
	IncludeModel("EffectSystem:FXEffect,fx_darkling_lighthurt_2", _pWData);
}


void CWObject_CharDarkling::OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer)
{
	IncludeModelFromKey("EYETRAILMODEL", _pReg, _pMapData);

	parent::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
}


void CWObject_CharDarkling::OnFinishEvalKeys()
{
	// Init gravity by using start position up vector
	CWO_CharDarkling_ClientData& CD = GetClientData();
	CMat4Dfp32 PosMat = GetPositionMatrix();
	CD.m_UpVector = PosMat.GetRow(2);
	CD.SetGravity( -PosMat.GetRow(2) );

	// Move object position from feet -> center
	fp32 Radius = CD.m_Phys_Height * 0.5f;
	PosMat.GetRow(3) += PosMat.GetRow(2) * Radius;
	m_pWServer->Object_SetPosition(m_iObject, PosMat);
	m_pWServer->Debug_RenderSphere(PosMat, 16.0f, 0xff0000ff, 4.0f, true);

	if (AI_GetAI()->m_CharacterClass == CAI_Core::CLASS_DARKLING)
	{
		// Precache extra data for diffrent darklings
		CMapData& MapData = *m_pWServer->GetMapData();
		CD.m_DK_Special = ((CAI_Core_Darkling*)AI_GetAI())->m_DK_Special;
		switch (CD.m_DK_Special)
		{
		case CAI_Core_Darkling::DK_SPECIAL_GUNNER:
			{
				MapData.GetResourceIndex_Model("EffectSystem:FXEffect,fx_cigar_smoke");
				break;
			}

		case CAI_Core_Darkling::DK_SPECIAL_LIGHTKILLER:
			{
				MapData.GetResourceIndex_Model("TrailStrip:lightning_trail");
				MapData.GetResourceIndex_Model("particles:SU=elballani,CO=0xEE494949,DU=0.4,DUN=0.1,FI=0.1,AL0=1.5,AX=0 0 -4,AXN=0 0 2,DI=sphere,DIS=0.25 0.25 0.25,RT0=1,RT1=-1,RT2=-1,RT3=1,RT4=0.2,EP=0.37,EPC=0.9,FL=nh");

				for (uint i = 1; i < 10; i++)
					MapData.GetResourceIndex_Sound(CStrF("$env_elec_fras_001_%.3d", i).GetStr());
				break;
			}

		default:
			{
				break;
			}
		}
	}

	parent::OnFinishEvalKeys();
}


void CWObject_CharDarkling::OnSpawnWorld()
{
	parent::OnSpawnWorld();
}


void CWObject_CharDarkling::OnDestroy()
{
	if (m_iLightHurtObj)
		m_pWServer->Object_Destroy(m_iLightHurtObj);

	parent::OnDestroy();
}


void CWObject_CharDarkling::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	CWO_CharDarkling_ClientData& CD = GetClientData(_pObj);

	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);
	CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);

	fp32 GameTickTime = _pWClient->GetGameTickTime();

	// If darkling is dead, add particle model and make sure we don't render darkling
	if (Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD)
	{
		if (CD.m_DeathTick != -1)
		{
			AnimState.m_AnimTime0 = CMTime::CreateFromSeconds(fp32(CD.m_GameTick - CD.m_DeathTick) * GameTickTime);

			// Add death effect
			int iDeathEffect = _pWClient->GetMapData()->GetResourceIndex_Model("EffectSystem:FXEffect,fx_darklingdeath");
			CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(iDeathEffect);

			if(pModel)
				_pEngine->Render_AddModel(pModel, MatIP, AnimState);
		}

		return;
	}
	
	// Parent client render
	parent::OnClientRender(_pObj, _pWClient, _pEngine, _ParentMat);

	bool bLightBurn = (CD.m_Flags & DARKLING_FLAGS_BURNING) != 0;
	int32 GameTick = _pWClient->GetGameTick();
	int32 TicksPerSec = TruncToInt(_pWClient->GetGameTicksPerSecond());
	int32 HurtTick = bLightBurn ? MinMT((GameTick - CD.m_LightHurtTick), TicksPerSec) : TicksPerSec + (CD.m_LightHurtTick - (GameTick - TicksPerSec));

	// Render burn effect for darkling in light areas
	if (bLightBurn || HurtTick >= 0)
	{
		int iBurnModel1 = _pWClient->GetMapData()->GetResourceIndex_Model("EffectSystem:FXEffect,fx_darkling_lighthurt_1");
		int iBurnModel2 = _pWClient->GetMapData()->GetResourceIndex_Model("EffectSystem:FXEffect,fx_darkling_lighthurt_2");
		CXR_Model* pBurnModel1 = _pWClient->GetMapData()->GetResource_Model(iBurnModel1);
		CXR_Model* pBurnModel2 = _pWClient->GetMapData()->GetResource_Model(iBurnModel2);
		if (pBurnModel1 || pBurnModel2)
		{
			CXR_AnimState AnimStateBurn;
			AnimStateBurn.Clear();
			AnimStateBurn.m_AnimTime0 = CMTime::CreateFromTicks(HurtTick, GameTickTime, (bLightBurn) ? IPTime : 1.0f - IPTime);
			AnimStateBurn.m_AnimTime1 = CMTime::CreateFromTicks(GameTick, GameTickTime, IPTime);
			CWObject_EffectSystem::ShaderUseAnimTime1(AnimStateBurn, true);

			if (pBurnModel1)
				_pEngine->Render_AddModel(pBurnModel1, MatIP, AnimStateBurn, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_NOCULL);

			if (pBurnModel2)
			{
				CWObject_EffectSystem::SetupBoneAttachData(_pObj, _pWClient, AnimStateBurn, 2, IPTime, &MatIP);
				_pEngine->Render_AddModel(pBurnModel2, MatIP, AnimStateBurn, XR_MODEL_STANDARD, CXR_MODEL_ONRENDERFLAGS_NOCULL);
			}
		}
	}

	bool bLocalPlayer = (_pObj->m_iObject == _pWClient->Player_GetLocalObject());
	bool bThirdPerson = (CD.m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON | PLAYER_EXTRAFLAGS_FORCETHIRPERSON)) != 0;
	bool bFirstPerson = bLocalPlayer && !bThirdPerson;

	// Render smoke model and eye trails so we can see where our darklings is. When it's dark.
	bool bRenderSpecial = false;
	switch (CD.m_DK_Special)
	{
	case CAI_Core_Darkling::DK_SPECIAL_GUNNER:
	case CAI_Core_Darkling::DK_SPECIAL_LIGHTKILLER:
		{
			bRenderSpecial = true;
			break;
		}

	default:
		{
			break;
		}
	}

	if (!bFirstPerson && (bRenderSpecial || CD.m_iEyeTrailModel))
	{
		CXR_AnimState Anim;
		CXR_Skeleton* pSkel;
		CXR_SkeletonInstance* pSkelInstance;
		if (GetEvaluatedPhysSkeleton(_pObj, _pWClient, pSkelInstance, pSkel, Anim, IPTime, &MatIP))
		{
			if (CD.m_iEyeTrailModel)
			{
				CXR_Model* pEyeTrail = _pWClient->GetMapData()->GetResource_Model(CD.m_iEyeTrailModel);
				AnimState.m_AnimTime0 = CMTime::CreateFromTicks(GameTick - CD.m_SpawnTick, GameTickTime, IPTime);
				AnimState.m_AnimTime1 = _pWClient->GetGameTime();

				//CVec3Dfp32 RightEye = CVec3Dfp32::GetMatrixRow(MatIP, 3);
				//CVec3Dfp32 LeftEye = CVec3Dfp32::GetMatrixRow(MatIP, 3);
				// Add both eyes
				if (pSkelInstance->m_nBoneTransform > 36)
				{
					const CMat4Dfp32& RightMat = pSkelInstance->m_pBoneTransform[36];
					CVec3Dfp32 RightEye = pSkel->m_lNodes[36].m_LocalCenter;
					RightEye *= RightMat;

					RightEye.SetRow(MatIP, 3);
					AnimState.m_pModelInstance = CD.GetModelInstance(pEyeTrail, 0);
					_pEngine->Render_AddModel(pEyeTrail, MatIP, AnimState);
				}

				if (pSkelInstance->m_nBoneTransform > 74)
				{
					const CMat4Dfp32& LeftMat = pSkelInstance->m_pBoneTransform[74];
					CVec3Dfp32 LeftEye = pSkel->m_lNodes[74].m_LocalCenter;
					LeftEye *= LeftMat;

					LeftEye.SetRow(MatIP, 3);
					AnimState.m_pModelInstance = CD.GetModelInstance(pEyeTrail, 1);
					_pEngine->Render_AddModel(pEyeTrail, MatIP, AnimState);
				}
			}

			if (bRenderSpecial)
			{
				CMapData& MapData = *_pWClient->GetMapData();
				switch (CD.m_DK_Special)
				{
				case CAI_Core_Darkling::DK_SPECIAL_GUNNER:
					{
						int iModel = MapData.GetResourceIndex_Model("EffectSystem:FXEffect,fx_cigar_smoke");
						CXR_Model* pSmokeModel = MapData.GetResource_Model(iModel);
						AnimState.m_AnimTime0 = CMTime::CreateFromTicks(GameTick - CD.m_SpawnTick, GameTickTime, IPTime);
						AnimState.m_AnimTime1 = _pWClient->GetGameTime();

						CMat4Dfp32 SmokeMat;
						uint iNode = pSkel->m_lAttachPoints[6].m_iNode;
						pSkel->m_lAttachPoints[6].m_LocalPos.Multiply(pSkelInstance->m_pBoneTransform[iNode], SmokeMat);
						AnimState.m_pModelInstance = CD.GetModelInstance(pSmokeModel, 2);
						_pEngine->Render_AddModel(pSmokeModel, SmokeMat, AnimState);
						break;
					}

				case CAI_Core_Darkling::DK_SPECIAL_LIGHTKILLER:
					{
						uint bAttach = 0;
						uint nAttach = 0;
						GetLightSparks(_pWClient, bAttach, nAttach, CD.m_Flags);
						
						CXR_VBManager* pVBM = _pEngine->GetVBM();
						CVec3Dfp32* pParam = (nAttach > 0) ? ((pVBM) ? pVBM->Alloc_V3(nAttach << 1) : NULL) : NULL;
						if (pParam)
						{
							CXR_AnimState AnimParticles;
							AnimParticles.Clear();
							AnimParticles.m_AnimTime0 = _pWClient->GetRenderTime();

							AnimState.m_pModelInstance = NULL;

							int iParticles = MapData.GetResourceIndex_Model("particles:SU=elballani,CO=0xEE494949,DU=0.4,DUN=0.1,FI=0.1,AL0=1.5,AX=0 0 -4,AXN=0 0 2,DI=sphere,DIS=0.25 0.25 0.25,RT0=1,RT1=-1,RT2=-1,RT3=1,RT4=0.2,EP=0.37,EPC=0.9,FL=nh");
							CXR_Model* pParticles = MapData.GetResource_Model(iParticles);

							int iModel = MapData.GetResourceIndex_Model("TrailStrip:lightning_trail");
							CXR_Model* pModel = MapData.GetResource_Model(iModel);

							// Calculate matrices (0=Head, 1=RHand, 2=LHand, 3=LBattery, 4=HBattery)
							CMat4Dfp32 TempMat[5];
							for (uint i = 0, j = 6; i < 5; i++, j++)
								pSkel->m_lAttachPoints[j].m_LocalPos.Multiply(pSkelInstance->m_pBoneTransform[pSkel->m_lAttachPoints[j].m_iNode], TempMat[i]);

							bool bAddParticles[] = { false, false, false, false, false, false, NULL };

							// Put all spline creation data in same model, this is unnecessary
							uint iParam = 0;
							AnimState.m_Data[2] = 0;
							if (bAttach & M_Bit(0))
							{
								pParam[iParam] = TempMat[3].GetRow(3);
								pParam[iParam+1] = (TempMat[3].GetRow(0) - TempMat[3].GetRow(2)).Normalize();
								AnimState.m_Data[1] = TruncToInt((TempMat[4].GetRow(3) - TempMat[3].GetRow(3)).Length()+0.99f);
								AnimState.m_Data[0] = (aint)&pParam[iParam]; _pEngine->Render_AddModel(pModel, TempMat[4], AnimState); iParam += 2;
								_pEngine->Render_AddModel(pParticles, TempMat[3], AnimParticles); bAddParticles[3] = true;
								_pEngine->Render_AddModel(pParticles, TempMat[4], AnimParticles); bAddParticles[4] = true;
							}
							if (bAttach & M_Bit(1))
							{
								pParam[iParam] = TempMat[0].GetRow(3);
								pParam[iParam+1] = -TempMat[0].GetRow(2);
								AnimState.m_Data[1] = TruncToInt((TempMat[4].GetRow(3) - TempMat[0].GetRow(3)).Length()+0.99f);
								AnimState.m_Data[0] = (aint)&pParam[iParam]; _pEngine->Render_AddModel(pModel, TempMat[4], AnimState); iParam += 2;
								_pEngine->Render_AddModel(pParticles, TempMat[0], AnimParticles); bAddParticles[0] = true;
								if (!bAddParticles[4]) { _pEngine->Render_AddModel(pParticles, TempMat[4], AnimParticles); bAddParticles[4] = true; }
							}
							if (bAttach & M_Bit(2))
							{
								pParam[iParam] = TempMat[0].GetRow(3);
								pParam[iParam+1] = (TempMat[0].GetRow(0) + TempMat[0].GetRow(1)).Normalize();
								AnimState.m_Data[1] = TruncToInt((TempMat[1].GetRow(3) - TempMat[0].GetRow(3)).Length()+0.99f);
								AnimState.m_Data[0] = (aint)&pParam[iParam]; _pEngine->Render_AddModel(pModel, TempMat[1], AnimState); iParam += 2;
								if (!bAddParticles[0]) { _pEngine->Render_AddModel(pParticles, TempMat[0], AnimParticles); bAddParticles[0] = true; }
								_pEngine->Render_AddModel(pParticles, TempMat[1], AnimParticles); bAddParticles[1] = true;
							}
							if (bAttach & M_Bit(3))
							{
								pParam[iParam] = TempMat[0].GetRow(3);
								pParam[iParam+1] = (TempMat[0].GetRow(0) - TempMat[0].GetRow(1)).Normalize();
								AnimState.m_Data[1] = TruncToInt((TempMat[0].GetRow(3) - TempMat[2].GetRow(3)).Length()+0.99f);
								AnimState.m_Data[0] = (aint)&pParam[iParam]; _pEngine->Render_AddModel(pModel, TempMat[2], AnimState); iParam += 2;
								if (!bAddParticles[0]) { _pEngine->Render_AddModel(pParticles, TempMat[0], AnimParticles); bAddParticles[0] = true; }
								_pEngine->Render_AddModel(pParticles, TempMat[2], AnimParticles); bAddParticles[2] = true;
							}
							if (bAttach & M_Bit(4))
							{
								pParam[iParam] = TempMat[1].GetRow(3);
								pParam[iParam+1] = -TempMat[1].GetRow(2);
								AnimState.m_Data[1] = TruncToInt((TempMat[1].GetRow(3) - TempMat[2].GetRow(3)).Length()+0.99f);
								AnimState.m_Data[0] = (aint)&pParam[iParam]; _pEngine->Render_AddModel(pModel, TempMat[2], AnimState); iParam += 2;
								if (!bAddParticles[1]) { _pEngine->Render_AddModel(pParticles, TempMat[1], AnimParticles); bAddParticles[1] = true; }
								if (!bAddParticles[2]) { _pEngine->Render_AddModel(pParticles, TempMat[2], AnimParticles); bAddParticles[2] = true; }
							}
							if (CD.m_Flags & DARKLING_FLAGS_LIGHTNING)
							{
								const CVec3Dfp32& LightningPos = CD.m_LightningPos;
								pParam[iParam] = pParam[iParam+2] = LightningPos;
								pParam[iParam+1] = (LightningPos - TempMat[1].GetRow(3)).Normalize();
								pParam[iParam+3] = (LightningPos - TempMat[2].GetRow(3)).Normalize();
								AnimState.m_Data[2] = M_Bit(0);
								AnimState.m_Data[1] = TruncToInt((TempMat[1].GetRow(3) - LightningPos).Length()+0.99f);
								AnimState.m_Data[0] = (aint)&pParam[iParam]; _pEngine->Render_AddModel(pModel, TempMat[1], AnimState); iParam += 2;
								AnimState.m_Data[1] = TruncToInt((TempMat[2].GetRow(3) - LightningPos).Length()+0.99f);
								AnimState.m_Data[0] = (aint)&pParam[iParam]; _pEngine->Render_AddModel(pModel, TempMat[2], AnimState); iParam += 2;

								if (!bAddParticles[1]) { _pEngine->Render_AddModel(pParticles, TempMat[1], AnimParticles); bAddParticles[1] = true; }
								if (!bAddParticles[2]) { _pEngine->Render_AddModel(pParticles, TempMat[2], AnimParticles); bAddParticles[2] = true; }
							}
						}
						break;
					}

				default:
					{
						break;
					}
				}
			}
		}
	}
}


int CWObject_CharDarkling::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	CWO_CharDarkling_ClientData& CD = GetClientData(_pObj);

	switch (_Event)
	{
	case CWO_PHYSEVENT_GETACCELERATION:
		if (_pMat)
		{
			_pMat->Unit();
			CD.Phys_GetAcceleration(_pMat, _dTime);
			return SERVER_PHYS_HANDLED;
		}
		return SERVER_PHYS_DEFAULTHANDLER;


	case CWO_PHYSEVENT_IMPACT:
		CD.Phys_OnImpact(_pPhysState,_pObj,*_pCollisionInfo);
		
		return SERVER_PHYS_DEFAULTHANDLER;
	}

	return parent::OnPhysicsEvent(_pObj, _pObjOther, _pPhysState, _Event, _Time, _dTime, _pMat, _pCollisionInfo);
}


void CWObject_CharDarkling::Char_PrepareSpawn()
{
	CWO_CharDarkling_ClientData& CD = GetClientData();
	AI_GetAI()->PauseBehaviour();

	CWAG2I_Context AG2Context(this, m_pWServer, m_pWServer->GetGameTime());
	CD.m_AnimGraph2.GetAG2I()->Refresh(&AG2Context);
	CD.m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_GENERAL,RoundToInt(Random * 4.0f) % 4);
	CD.m_AnimGraph2.GetAG2I()->SendImpulse(&AG2Context, CXRAG2_Impulse(AG2_IMPULSETYPE_GAMEPLAY, AG2_IMPULSEVALUE_GAMEPLAY_DARKLING_SPAWNPUSHJUMPUP), 0);
	
	// Make character immobile and turn off character collisions
	CD.m_Phys_Flags = CD.m_Phys_Flags | (PLAYER_PHYSFLAGS_NOCHARACTERCOLL | PLAYER_PHYSFLAGS_IMMOBILE);
	CD.m_Spawning = 1;

	m_pWServer->Object_Create("Darkling_SpawnSmokeLayer", GetPositionMatrix(), m_iObject);
}


void CWObject_CharDarkling::Char_ReleaseSpawn(CWO_CharDarkling_ClientData* _pCD)
{
	CWAG2I_Context AG2Context(this, m_pWServer, m_pWServer->GetGameTime());
	fp32 SpawnTime = (_pCD->m_AnimGraph2.GetAG2I()->GetCurrentLoopDuration(&AG2Context, 0) + m_pWServer->GetGameTickTime() + 0.001f) * m_pWServer->GetGameTicksPerSecond();
	int32 SpawnTimeTicks = TruncToInt(SpawnTime+0.999f);

	if((_pCD->m_GameTick - _pCD->m_SpawnTick) > SpawnTimeTicks)
	{
		// Check if we arent running a spawn behaviour
		if (m_spAI)
		{
			m_spAI->ReleaseBehaviour();
			/*
			CVec3Dfp32 VecParam;
			CStr StrParam = "$ROOM";
			m_spAI->OnImpulse(CAI_Core::IMPULSE_RESTRICT_OBJECT,m_iObject,m_iObject,VecParam,StrParam.GetStr());
			*/
		}

		
		// Turn on character collisions and make darkling mobile. Also set spawning state to false
		_pCD->m_Phys_Flags = _pCD->m_Phys_Flags & ~(PLAYER_PHYSFLAGS_NOCHARACTERCOLL | PLAYER_PHYSFLAGS_IMMOBILE);
		_pCD->m_Spawning = 0;
	}
}


bool CWObject_CharDarkling::Char_IsSpawning()
{
	return GetClientData().Char_IsSpawning();
}

void CWObject_CharDarkling::OnRefresh()
{
	CWO_CharDarkling_ClientData& CD = GetClientData();

	if (CD.Char_IsSpawning())
	{
		// Try to push away all objects that's in the way before the spawn sequence is finished...
		//   (this shouldn't normally happen, since the player needs clear line of sight in order to activate a darkling spawn point)
		TSelection<CSelection::SMALL_BUFFER> Sel;
		m_pWServer->Selection_AddBoundSphere(Sel, OBJECT_FLAGS_OBJECT, GetPosition(), 16.0f);
		for (uint i = 0; i < Sel.Len(); i++)
		{
			CWObject* pObj = m_pWServer->Object_Get(Sel[i]);
			if (pObj)
			{
				CVec3Dfp32 PushVel;
				pObj->GetAbsBoundBox()->GetCenter(PushVel);
				PushVel -= GetPosition();
				PushVel.SafeNormalize() *= 2.0f;   // 2 m/s
				m_pWServer->Phys_AddMassInvariantImpulse(pObj->m_iObject, GetPosition(), PushVel);
			}
		}

		// Check if the spawn sequence is finished
		Char_ReleaseSpawn(&CD);
	}

	// Make sure we can't shoot on dead darklings.
	if (Char_GetPhysType(this) == PLAYER_PHYS_DEAD)
	{
		uint WantFlags = (PLAYER_PHYSFLAGS_NOGRAVITY | PLAYER_PHYSFLAGS_NOCHARACTERCOLL | PLAYER_PHYSFLAGS_NOWORLDCOLL |
						  PLAYER_PHYSFLAGS_IMMOBILE | PLAYER_PHYSFLAGS_IMMUNE | PLAYER_PHYSFLAGS_NOANIMPHYS | PLAYER_PHYSFLAGS_NOPROJECTILECOLL |
						  PLAYER_PHYSFLAGS_NOMEDIUMMOVEMENT | PLAYER_PHYSFLAGS_IMMOBILE);
		if ((CD.m_Phys_Flags & WantFlags) != WantFlags)
		{
			ClientFlags() = m_ClientFlags & PLAYER_CLIENTFLAGS_NOGRAVITY;
			CD.m_Phys_Flags = CD.m_Phys_Flags | WantFlags;
			Char_SetPhysics(this,m_pWServer,m_pWServer,PLAYER_PHYS_DEAD,false,true);
		}
	}

	parent::OnRefresh();

	CD.OnRefresh();

	m_BaseMat = GetPositionMatrix();
	m_BaseMat.GetRow(3) -= CD.m_UpVector.m_Value * 16.0f;	// Move from center to feet
	m_BaseMat.GetRow(2) = -CD.m_Gravity.m_Value;		// Align with ground
	m_BaseMat.RecreateMatrix(2, 0);						// -
	m_WeaponMat = GetPositionMatrix();
	m_HeadMat = GetPositionMatrix();
	m_HeadMat.GetRow(3) += m_HeadMat.GetRow(2) * 32.0f;
	m_bHeadMatUpdated = true;

	if(CD.m_State == DARKLING_STATE_JUMP_LAND)
	{
		if(CD.m_iScenePointTarget != -1)
		{
			CWObject_ScenePointManager* pSPM = (CWObject_ScenePointManager*)m_pWServer->Message_SendToObject(
				CWObject_Message(OBJMSG_GAME_GETSCENEPOINTMANAGER), m_pWServer->Game_GetObjectIndex());
			if (pSPM)
			{
				CWO_ScenePoint* pSP = pSPM->GetScenePointFromIndex(CD.m_iScenePointTarget);

//				CD.m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_LOOPMODE, AG2_BEHAVIOR_LOOPMODE_ONCE); 

				CAI_Core* pAC = AI_GetAI();
				if(!pAC->m_AH.ActivateScenePoint(pSP))
					ConOutL("Failed to activate Scenepoint");

				CD.m_AnimGraph2.SetPropertyInt(PROPERTY_INT_BEHAVIOR_LOOPMODE, AG2_BEHAVIOR_LOOPMODE_ONCE); 
			}
			CD.m_iScenePointTarget = -1;
		}
	}

	if (CD.m_Flags & DARKLING_FLAGS_LIGHTNING && CD.m_LightningTick < m_pWServer->GetGameTick())
		CD.m_Flags = CD.m_Flags & ~DARKLING_FLAGS_LIGHTNING;
}


void CWObject_CharDarkling::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_CharDarkling::OnClientRefresh);
	
	// Make sure lightkiller darklings client side flash effects play sounds
	CWO_CharDarkling_ClientData& CD = GetClientData(_pObj);
	if (CD.m_DK_Special == CAI_Core_Darkling::DK_SPECIAL_LIGHTKILLER && CD.m_Flags != DARKLING_FLAGS_LIGHTNING)
	{
		uint bAttach = 0;
		uint nAttach = 0;
		GetLightSparks(_pWClient, bAttach, nAttach, 0);
		if (nAttach > 0 && !CD.m_SparkSound)
		{
			// Try to play a spark sound
			int iSound = _pWClient->GetMapData()->GetResourceIndex_Sound(CStrF("$env_elec_fras_001_%.3d", (_pWClient->GetGameTick() % 9) + 1).GetStr());
			if (iSound != 0)
			{
				_pWClient->Sound_On(WCLIENT_CHANNEL_SFX, _pObj->m_iObject, iSound, WCLIENT_ATTENUATION_3D);
				CD.m_SparkSound = 1;
			}
		}
		else if (!nAttach && CD.m_SparkSound)
			CD.m_SparkSound = 0;
	}

	// Do nothing while spawning.
	if(CD.Char_IsSpawning())
		return;

	parent::OnClientRefresh(_pObj, _pWClient);
	CD.OnRefresh();
}


void CWObject_CharDarkling::GetLightSparks(CWorld_Client* _pWClient, uint& _bAttach, uint& _nAttach, uint _Flags)
{
	uint TickRand = uint(_pWClient->GetGameTick() / TruncToInt(_pWClient->GetGameTicksPerSecond()*0.25f));

	fp32 Dec = 0.0f;
	if (_Flags & DARKLING_FLAGS_LIGHTNING) { Dec += 0.4f; _nAttach += 2; }
	if (MFloat_GetRand(TickRand*0xbabe) > 0.93f-Dec) { _bAttach |= M_Bit(0); _nAttach++; Dec += 0.01f; }	// On battery
	if (MFloat_GetRand(TickRand*0xc0de) > 0.97f-Dec) { _bAttach |= M_Bit(1); _nAttach++; Dec += 0.01f; }	// Battery to head
	if (MFloat_GetRand(TickRand*0x0001) > 0.97f-Dec) { _bAttach |= M_Bit(2); _nAttach++; Dec += 0.01f; }	// LHand to head
	if (MFloat_GetRand(TickRand*0xdead) > 0.97f-Dec) { _bAttach |= M_Bit(3); _nAttach++; Dec += 0.01f; }	// RHand to head
	if (MFloat_GetRand(TickRand*0xbeef) > 0.98f-Dec) { _bAttach |= M_Bit(4); _nAttach++; }					// LHand to RHand
}


aint CWObject_CharDarkling::OnClientMessage(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CWObject_Message& _Msg)
{
	switch (_Msg.m_Msg)
	{
	case OBJSYSMSG_GETCAMERA:
		{
			if (_Msg.m_DataSize != sizeof(CMat4Dfp32)) 
				return 0;

			CMat4Dfp32& Camera = *(CMat4Dfp32*)_Msg.m_pData;

			((CWorld_ClientCore*)_pWClient)->Con_Cmd(127);	// insert dummy command
			CWObject_Character::OnClientPredict(_pObj, _pWClient, 0, 0, 1); // perform prediction (this is the magic place)
			_pObj = Player_GetLastValidPrediction(_pObj);

			GetClientData(_pObj).GetCamera(Camera, _pWClient->GetInterpolateTime());
			return 1;
		}
	}
	return parent::OnClientMessage(_pObj, _pWClient, _Msg);
}


aint CWObject_CharDarkling::OnMessage(const CWObject_Message& _Msg)
{
	CWO_CharDarkling_ClientData& CD = GetClientData();

	switch (_Msg.m_Msg)
	{
		case OBJSYSMSG_GETCAMERA:
		{
			if (_Msg.m_DataSize != sizeof(CMat4Dfp32)) 
				return 0;

			CMat4Dfp32& Camera = *(CMat4Dfp32*)_Msg.m_pData;
			GetClientData().GetCamera(Camera, 0.0f);
			return 1;
		}

		case OBJMSG_CHAR_DARKLING_SMOKE:
		{
			int32 GameTick = m_pWServer->GetGameTick();
			int32 TicksPerSec = TruncToInt(m_pWServer->GetGameTicksPerSecond());
			bool bEnable = (_Msg.m_Param0 != 0);

			// Turn on hurt effects in light (if not already on)
			if (bEnable && (CD.m_Flags & DARKLING_FLAGS_BURNING) == 0)
			{
				m_iSound[0] = m_pWServer->GetMapData()->GetResourceIndex_Sound("Env_Fire_Sizzleloop01");
				m_pWServer->Sound_On(m_iObject, m_iSound[0], 2);
				CD.m_Flags = CD.m_Flags | DARKLING_FLAGS_BURNING;

				FadeLightHurt(true);
				CD.m_LightHurtTick = (GameTick - TicksPerSec) + MinMT(TicksPerSec, GameTick - CD.m_LightHurtTick);

				m_DirtyMask |= CWO_DIRTYMASK_SOUND;
			}

			// Turn off hurt effects in light (if not already off)
			else if (!bEnable && (CD.m_Flags & DARKLING_FLAGS_BURNING) != 0)
			{
				CD.m_Flags = CD.m_Flags & ~DARKLING_FLAGS_BURNING;
				m_iSound[0] = 0;

				FadeLightHurt(false);
				CD.m_LightHurtTick = GameTick - MinMT(TicksPerSec, GameTick - CD.m_LightHurtTick);

				m_DirtyMask |= CWO_DIRTYMASK_SOUND;
			}

			return 1;
		}

		case OBJMSG_CHAR_DARKLING_LIGHTNING:
		{
			// Turn on lightning effect, automatically switched of in refresh

			// Check for flag changes
			uint16 Flags = (_Msg.m_Param0) ? (CD.m_Flags | DARKLING_FLAGS_LIGHTNING) : (CD.m_Flags & ~DARKLING_FLAGS_LIGHTNING);
			
			// Update parameters as needed
			CD.m_Flags = Flags;
			if (_Msg.m_Param0)
			{
				CD.m_LightningPos = _Msg.m_VecParam0;
				CD.m_LightningTick = m_pWServer->GetGameTick() + TruncToInt(m_pWServer->GetGameTicksPerSecond() * 1.5f);

				// Start spark sound
				int iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound("$env_elec_fras_001_004");
				if (iSound != 0)
					m_pWServer->Sound_On(m_iObject, iSound, WCLIENT_ATTENUATION_3D);
			}

			return 0;
		}
	}

	return parent::OnMessage(_Msg);
}

void CWObject_CharDarkling::FadeLightHurt(bool _bFadeIn)
{
	if (!m_iLightHurtObj)
		m_iLightHurtObj = m_pWServer->Object_Create("Darkling_LightHurt", GetPositionMatrix(), m_iObject);

	CWObject_Message Msg(OBJMSG_EFFECTSYSTEM_SETFADE, (_bFadeIn) ? 1 : 2);
	m_pWServer->Phys_Message_SendToObject(Msg, m_iLightHurtObj);
}

// AI-interface
bool CWObject_CharDarkling::AI_IsForceRotated()
{
//	CWO_CharDarkling_ClientData& CD = GetClientData();
	return false; // *** Test ***
	//(CD.m_Flags & DARKLING_FLAGS_IS_ROTATING) != 0;
}


bool CWObject_CharDarkling::AI_IsJumping()
{
	CWO_CharDarkling_ClientData& CD = GetClientData();
	return (CD.m_State >= DARKLING_STATE_JUMP_INIT && CD.m_State <= DARKLING_STATE_JUMP_LAND);
}

bool CWObject_CharDarkling::AI_CanJump(const bool _bFromAir)
{
	CWO_CharDarkling_ClientData& CD = GetClientData();
	if ((CD.m_State == DARKLING_STATE_NORMAL)&&(CD.m_Flags & DARKLING_FLAGS_CAN_CLIMB)&&((_bFromAir)||(!CD.m_Phys_bInAir)))
	{
		return(true);
	}
	else
	{
		return(false);
	}
}


bool CWObject_CharDarkling::AI_Jump(const CMat4Dfp32* _pDestMat, int32 _Flags)
{
	CWO_CharDarkling_ClientData& CD = GetClientData();
//	if ((CD.m_JumpTick != 0) || (!(CD.m_Flags & DARKLING_FLAGS_CAN_CLIMB)))
//		return false; // is jumping or can't jump/climb
	if (!(CD.m_Flags & DARKLING_FLAGS_CAN_CLIMB))
		return false;

	if (CD.m_State != DARKLING_STATE_NORMAL)
		return false;

	// Check if we're standing on ground
	if (!(_Flags & CAI_Core::JUMP_FROM_AIR)&&(CD.m_Phys_bInAir))
		return false;

/*	CMat4Dfp32 p0 = GetPositionMatrix(), p1 = GetPositionMatrix();
	p1.GetRow(3) += CD.m_Gravity;
	CCollisionInfo CInfo_1;
	bool bOnGround = m_pWServer->Phys_IntersectWorld(-1, GetPhysState(), p0, p1, m_iObject, &CInfo_1);
	if (!bOnGround || !CInfo_1.m_bIsValid)
		return false;*/

	CD.m_Flags = CD.m_Flags & ~DARKLING_FLAGS_JUMPTOAIR;

	if (_pDestMat && (_Flags & CAI_Core::JUMP_CHECK_COLL))
	{	// Collision checking - see if a jump from current pos to destination is possible
		CMat4Dfp32 DestMat = *_pDestMat;
		DestMat.GetRow(3) += DestMat.GetRow(2) * 12.0f;
		const CVec3Dfp32& Pos = GetPosition();
		CVec3Dfp32 Dir = DestMat.GetRow(3) - Pos;
		fp32 Len = Dir.Length();
		Dir *= (1.0f / Len);
		if (Len > 512.0f)
			return false;

		fp32 BoxWidth =  0.707f * (16.0f - 2.0f);
		fp32 BoxLenHalf = Len*0.5f - (16.0f + 2.0f);

		CWO_PhysicsState PhysState( GetPhysState() );
		PhysState.m_PhysFlags = OBJECT_PHYSFLAGS_ROTATION;
		PhysState.m_nPrim = 1;
		PhysState.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(BoxLenHalf, BoxWidth, BoxWidth), CVec3Dfp32(0,0,0));

		CMat4Dfp32 Mat;
		Mat.GetRow(0) = Dir;
		Mat.GetRow(2) = (Dir.k[2] > 0.9f) ? CVec3Dfp32(1,0,0) : CVec3Dfp32(0,0,1);
		Mat.RecreateMatrix(0, 2);
		Mat.GetRow(3) = Pos + Dir * (Len * 0.5f);

		bool bCollision = m_pWServer->Phys_IntersectWorld((CSelection *) NULL, PhysState, Mat, Mat, m_iObject, NULL);
		if (bCollision)
		{
	#ifndef M_RTM
		m_pWServer->Debug_RenderOBB(Mat, PhysState.m_Prim[0].GetDim(), DEBUG_COLOR_RED, 5.0f, true);
	#endif
			return false;
		}

	#ifndef M_RTM
		m_pWServer->Debug_RenderOBB(Mat, PhysState.m_Prim[0].GetDim(), DEBUG_COLOR_GREEN, 5.0f, true);
	#endif

		CD.m_JumpDestination = DestMat;
		CD.m_Flags = CD.m_Flags | DARKLING_FLAGS_JUMP_MATRIX;
		if (_Flags & CAI_Core::JUMP_TO_AIR)
		{
			CD.m_Flags = CD.m_Flags | DARKLING_FLAGS_JUMPTOAIR;
		}
	}
	else if (_pDestMat)
	{	// No collision checking
		if (_Flags & CAI_Core::JUMP_TO_AIR)
		{	// We should use _pDestMat as our landing matrix
			CMat4Dfp32 DestMat = *_pDestMat;
			DestMat.GetRow(3) += DestMat.GetRow(2) * 12.0f;
			CD.m_JumpDestination = DestMat;
			CD.m_Flags = CD.m_Flags | DARKLING_FLAGS_JUMP_MATRIX;
			if (_Flags & CAI_Core::JUMP_TO_AIR)
			{
				CD.m_Flags = CD.m_Flags | DARKLING_FLAGS_JUMPTOAIR;
			}
		}
		else
		{
			// No collision checking and _pDestMatjust tell us the direction of the jump
			const CVec3Dfp32& Pos = GetPosition();
			CVec3Dfp32 Dir = (_pDestMat->GetRow(3) - Pos).Normalize();
			CD.m_JumpDestination.m_Value.GetRow(0) = Dir;
			CD.m_JumpDestination.MakeDirty();
			CD.m_Flags = CD.m_Flags | DARKLING_FLAGS_JUMP_DIRECTION;
#ifndef M_RTM
			fp32 Len = (_pDestMat->GetRow(3) - Pos).Length();
			fp32 BoxWidth =  0.707f * (4.0f - 2.0f);
			fp32 BoxLenHalf = Max(1.0f, Len*0.5f - (4.0f + 2.0f));

			CWO_PhysicsState PhysState( GetPhysState() );
			PhysState.m_PhysFlags = OBJECT_PHYSFLAGS_ROTATION;
			PhysState.m_nPrim = 1;
			PhysState.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(BoxLenHalf, BoxWidth, BoxWidth), CVec3Dfp32(0,0,0));

			CMat4Dfp32 Mat;
			Mat.GetRow(0) = Dir;
			Mat.GetRow(2) = (Dir.k[2] > 0.9f) ? CVec3Dfp32(1,0,0) : CVec3Dfp32(0,0,1);
			Mat.RecreateMatrix(0, 2);
			Mat.GetRow(3) = Pos + Dir * (Len * 0.5f);

			m_pWServer->Debug_RenderOBB(Mat, PhysState.m_Prim[0].GetDim(), DEBUG_COLOR_BLUE, 5.0f, true);
#endif
		}
	}

//	CD.m_Flags = CD.m_Flags | DARKLING_FLAGS_DOJUMP;
	bool bResult = CD.PerformJump();
	return bResult;
}


void CWObject_CharDarkling::AI_EnableWallClimbing(bool _bActivate)
{
	CWO_CharDarkling_ClientData& CD = GetClientData();
	if (_bActivate)
	{
		CD.m_Flags = CD.m_Flags | DARKLING_FLAGS_CAN_CLIMB;
	}
	else
	{
		CD.m_Flags = CD.m_Flags & ~DARKLING_FLAGS_CAN_CLIMB;
		CD.m_Phys_IdleTicks = 0;
		CD.m_Gravity = CVec3Dfp32(0,0,-1);
		CD.m_Gravity_Estimated = CVec3Dfp32(0,0,-1);
		CD.m_UpVector = CVec3Dfp32(0,0,1);
		CD.m_JumpTick = 0;
	}
}


bool CWObject_CharDarkling::AI_IsWallClimbingEnabled()
{
	CWO_CharDarkling_ClientData& CD = GetClientData();
	return (CD.m_Flags & DARKLING_FLAGS_CAN_CLIMB) != 0;
}


bool CWObject_CharDarkling::AI_IsOnWall()
{
	CWO_CharDarkling_ClientData& CD = GetClientData();
	if (CD.m_Gravity.m_Value * CVec3Dfp32(0,0,-1) < 0.999f)
		return true;

	return false;
}

