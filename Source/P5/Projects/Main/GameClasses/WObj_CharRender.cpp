#include "PCH.h"

#include "WObj_Char.h"

#include "../../../Shared/MOS/XR/XREngineVar.h"
#include "../../../Shared/MOS/XR/XREngineImp.h"
#include "../../../Shared/MOS/XRModels/Model_TriMesh/WTriMesh.h"
#include "../../../Shared/MOS/Classes/Win/MWindows.h"
#include "WObj_Game/WObj_GameMod.h"
#include "WObj_Misc/WObj_ActionCutscene.h"
#include "WObj_Misc/WObj_ActionCutsceneCamera.h"
#include "WObj_Misc/WObj_Telephone.h"
#include "WObj_Misc/WObj_CreepingDark.h"
#include "WRPG/WRPGSpell.h"
#include "../GameWorld/WClientMod.h"
#include "../GameWorld/WFrontEndMod.h"
#include "WObj_Char/WObj_CharPlayer_ClientData.h"
#include "WObj_Char/WObj_CharShapeshifter.h"
#include "WObj_Game/WObj_GameCore.h"
#include "WObj_Misc/WObj_DarklingSpawn.h"

#define SCROLLTIME 0.15f

CWO_Character_HealthHudItem::CWO_Character_HealthHudItem()
{
	m_Fraction = 0;
	m_TargetFraction = 0;
}

void CWO_Character_HealthHudItem::SetFraction(int _Fraction)
{
	m_TargetFraction = _Fraction;

	if(m_Fraction < m_TargetFraction)
		m_Fraction++;
	else if(m_Fraction > m_TargetFraction)
		m_Fraction--;
}

void RenderLaserDot(int _iItem, CWObject_CoreData *_pObj, CWorld_Client *_pWClient, CXR_Engine *_pEngine, CXR_Skeleton *_pSkel, CXR_SkeletonInstance *_pSkelInstance, CMat4Dfp32 *_pMatIP)
{
	if(_pEngine->GetVCDepth() > 0)
		return;

	CWO_Character_ClientData *pCDFirst;
	CWObject_CoreData* pObjFirst;
	pObjFirst = _pWClient->Object_GetFirstCopy(_pObj->m_iObject);
	pCDFirst = pObjFirst ? CWObject_Character::GetClientData(pObjFirst) : NULL;

	if(!pCDFirst)
		return;
	//CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	//if(!pCD)
	//	return;

	CMat4Dfp32 Mat;
	CXR_AnimState AnimState;
	CCollisionInfo Info;
	Info.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
	Info.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION | CXR_COLLISIONRETURNVALUE_SURFACE);
	CVec3Dfp32 From;

	CXR_Model *pModel;
	if(_iItem == 0)
	{
		if((!pCDFirst->m_Item0_Model.GetModel0_RenderInfo(_pWClient->GetMapData(), NULL, _pSkelInstance, _pSkel, pCDFirst->m_GameTime, AnimState, Mat, pModel, _pWClient))
			|| pCDFirst->m_PostAnimSystem.GetHandIsRetracted(true))// if weapons are all pulled back, dont render any dots

			return;
	}
	else
	{
		if((!pCDFirst->m_Item1_Model.GetModel0_RenderInfo(_pWClient->GetMapData(), NULL, _pSkelInstance, _pSkel, pCDFirst->m_GameTime, AnimState, Mat, pModel, _pWClient))
			|| pCDFirst->m_PostAnimSystem.GetHandIsRetracted(false))// if weapons are all pulled back, dont render any dots)
			return;
	}

	CXR_Skeleton* pSkelItem = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
	const CXR_SkeletonAttachPoint *pAttach = pSkelItem ? pSkelItem->GetAttachPoint(1) : NULL;
	if (pAttach)
		Mat.GetRow(3) = pAttach->m_LocalPos.GetRow(3) * Mat;

	From = CVec3Dfp32::GetRow(Mat, 3);

	/*
	// Proj-map test
	{
		CMat4Dfp32 Mat2 = *_pMatIP;
		(Mat.GetRow(3) + _pMatIP->GetRow(0)).SetRow(Mat2, 3);

		CXR_Light Light(Mat2, CVec3Dfp32(2.0f, 0.0f, 0.0f), 1024, 0, CXR_LIGHTTYPE_SPOT);
		Light.m_SpotHeight = 0.008f;
		Light.m_SpotWidth = 0.008f;
		Light.m_LightGUID = _pObj->m_iObject * 5 + 0x4005 + _iItem;
		Light.m_iLight = 0;

		MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
		Light.m_ProjMapID = pTC->GetTextureID("projmap_laserdot_01");

		CXR_SceneGraphInstance* pSGI = _pWClient->World_GetSceneGraphInstance();
		if(pSGI)
			pSGI->SceneGraph_Light_LinkDynamic(Light);
	}*/

	{

		CVec3Dfp32 Dest;
		
		// stabilize beam if firing
		if(_iItem == 0)
		{
			if(pCDFirst->m_PostAnimSystem.GetHandIKDisabled())
				Dest = From + _pMatIP->GetRow(0) * 2048;
			else
				Dest = From + (pCDFirst->m_PostAnimSystem.GetAimTargetPos() - From).Normalize() *  2048;
		}
		else if(_iItem == 1)
			Dest = From + (pCDFirst->m_PostAnimSystem.GetAimTargetPos() - From).Normalize() *  2048;
		else
			Dest = From + Mat.GetRow(0) * 2048;	

		if(_pWClient->Phys_IntersectLine(From, Dest, OBJECT_FLAGS_PHYSOBJECT, OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_PHYSMODEL, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS, &Info, _pObj->m_iObject,true) && Info.m_bIsValid)
		{
			fp32 Distance = CVec3Dfp32::GetRow(_pEngine->GetVC()->m_CameraWMat, 3).Distance(Info.m_Pos);
			fp32 Size = Distance / 175;
			if(Size < 0.5f)
				Size *= Sqr(Size + 0.5f);
			fp32 Fade = (1.0f - Info.m_Plane.n * (-_pMatIP->GetRow(0))) + Sqr(Distance / 1024.0f);
			if(Size >= 0.1f && Fade < 1.0f)
			{
				if(Info.m_iObject == _pWClient->Object_GetWorldspawnIndex())
				{
					CMat4Dfp32 Mat;
					Mat.r[0]=_pMatIP->r[2];
					Mat.r[2]=M_VSetW0(Info.m_Plane.v);
					Mat.RecreateMatrix<2,0>();
					Mat.r[3]=M_VLd_P3_Slow(&Info.m_Pos);
					_pWClient->Wallmark_Create(Mat, Size * 2, Size * 2, CMTime::CreateFromSeconds(Fade), "laserdot", XR_WALLMARK_TEMPORARY | XR_WALLMARK_ALLOW_PERPENDICULAR | XR_WALLMARK_NOANIMATE);
				}
				else
				{
					int iModel = _pWClient->GetMapData()->GetResourceIndex_Model("Sprite:laserdot");
					CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(iModel);
					if(pModel)
					{
						CMat4Dfp32 Mat;
						Mat.Unit();
						fp32 Scaler = Max( 1.0f, (Info.m_Pos - From).Length() * (1.0f/32.0f) );
						(Info.m_Pos - (Dest - From).Normalize() * Scaler).SetRow(Mat, 3);
						CXR_AnimState AnimState;
						int iSize = TruncToInt(Size);
						AnimState.m_Anim0 = iSize;
						AnimState.m_Anim1 = (int16)((Size - iSize) * 100);
						AnimState.m_AnimTime0 = CMTime::CreateFromSeconds(0.01f);

						_pEngine->Render_AddModel(pModel, Mat, AnimState);
		#ifndef M_RTM
						CRegistry* pUserReg = _pWClient->Registry_GetUser();
						int bShowSurface = (pUserReg) ? pUserReg->GetValuei("SHOWSURFACE") : 0;
						if (bShowSurface)
						{
							if (Info.m_pSurface)
								ConOut(CStrF("Surface: %s, Pos %s, iObject %d", Info.m_pSurface->m_Name.Str(), Info.m_Pos.GetString().Str(), Info.m_iObject ));
							else
								ConOut(CStrF("Surface: <NULL>, Pos %s, iObject %d", Info.m_Pos.GetString().Str(), Info.m_iObject ));
						}
		#endif
					}
				}
			}
		}
	}
}

void CWObject_Character::OnClientRenderVis(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, const CMat4Dfp32& _ParentMat)
{
	MSCOPESHORT(CWObject_Character::OnClientRenderVis);
	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if(!pCD)
		return;

	CXR_SceneGraphInstance* pSGI = _pWClient->World_GetSceneGraphInstance();

#ifdef WADDITIONALCAMERACONTROL
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CRegistry* pReg = pSys ? pSys->GetEnvironment() : NULL;
	bool bNoDialogLight = (pReg ? pReg->GetValuei("ADDITIONALCAMERACONTROL_NODIALOGLIGHT", 0, 0) : 0);
	if((pCD->m_3PI_LightState != THIRDPERSONINTERACTIVE_LIGHT_STATE_OFF) && (!pCD->m_bAdditionalCameraControl || !bNoDialogLight) && pSGI)
#else
	if(pSGI && (pCD->m_3PI_LightState != THIRDPERSONINTERACTIVE_LIGHT_STATE_OFF)) 
#endif
	{
		// FJ-NOTE: Adding a light source above speaker´s head. Might need to fade this up/down to make it look good.

		//CMat4Dfp32 Mat;
		CMat4Dfp32 NewMat;
		NewMat = _pObj->GetPositionMatrix();
		NewMat.GetRow(3) += NewMat.GetRow(0) * 16.0f;
		NewMat.GetRow(3) += NewMat.GetRow(1) * 28.0f;
		NewMat.GetRow(3).k[2] += 24.0f;
				
		// Set position, type and add light
		fp32 p = 1.0f;
		bool bAddLight = true;
		switch(pCD->m_3PI_LightState)
		{
		case THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_IN:
			{
				int32 diff = _pWClient->GetGameTick() - pCD->m_3PI_LightFadeStart;
				fp32 light_fade_duration = 2.5f * _pWClient->GetGameTicksPerSecond();
				p = ((fp32)diff / light_fade_duration);
				p = Min(p, 1.0f);
				p = Max(p, 0.0f);
			}
			break;

		case THIRDPERSONINTERACTIVE_LIGHT_STATE_OFF:
			{
				bAddLight = false;
			}
			break;

		case THIRDPERSONINTERACTIVE_LIGHT_STATE_ON:
			break;

		case THIRDPERSONINTERACTIVE_LIGHT_STATE_FADE_OUT:
			{
				int32 diff = _pWClient->GetGameTick() - pCD->m_3PI_LightFadeStart;
				fp32 light_fade_duration = 2.5f * _pWClient->GetGameTicksPerSecond();
				p = ((fp32)diff / light_fade_duration);
				p = Min(p, 1.0f);
				p = 1.0f-p;
				p = Max(p, 0.0f);
			}
			break;

		default:
			bAddLight = false;
			break;
		}

		if(false)// bAddLight)			-- turned off dialogue lights for EGM build (didn't look so good in otherworld)    /anton  
		{
			CXR_Light Light(NewMat, CVec3Dfp32(0.075f * p, 0.065f * p, 0.06f * p), 512, CXR_LIGHT_NOSHADOWS, CXR_LIGHTTYPE_POINT);
			Light.m_LightGUID = _pObj->m_iObject * 5 + 0x4001;
			Light.m_iLight = 0;
			pSGI->SceneGraph_Light_LinkDynamic(Light);
		}
	}

	bool bMuzzle = (pCD->m_MuzzleFlashTick0 > pCD->m_GameTick - 5) | (pCD->m_MuzzleFlashTick1 > pCD->m_GameTick - 5);
	int bFlashLight = _pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_FLASHLIGHT || pCD->m_Item0_Flags & RPG_ITEM_FLAGS_FLASHLIGHT;
	bool bLightning = (pCD->m_LightningLightTick0 > pCD->m_GameTick - 28) | (pCD->m_LightningLightTick1 > pCD->m_GameTick - 28);
	if(bMuzzle || bFlashLight || pCD->m_PannLampa || bLightning)
	{
		bool bPredicted = false;
		bool bLocalPlayer = (_pObj->m_iObject == _pWClient->Player_GetLocalObject());

		if (bLocalPlayer)
		{
			CWC_Player* pP = _pWClient->Player_GetLocal();
			if (!pP) return;

			if (_pObj->GetNext() != NULL)
			{
				_pObj = Player_GetLastValidPrediction(_pObj);
				bPredicted = true;
			}
		}

		fp32 IPTime = _pWClient->GetRenderTickFrac();
		CMat4Dfp32 MatIP;
		if (bPredicted)
		{
			MatIP = _pObj->GetPositionMatrix();
			IPTime = pCD->m_PredictFrameFrac;
		}
		else
		{
			if(pCD->m_LastTeleportTick != pCD->m_GameTick)
			{
				fp64 PeriodScale = _pWClient->GetModeratedFramePeriod() * _pWClient->GetGameTicksPerSecond();
				Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime / PeriodScale);
			}
			else
			{
				MatIP = _pObj->GetPositionMatrix();
			}
		}

		Char_UpdatePannlampa(_pWClient,_pObj);
		CMat4Dfp32 Mat;
		if(OnRefresh_WeaponLights(_pObj, _pWClient, MatIP, IPTime, &Mat))
		{
			bool bCutSceneView = (_pObj->m_ClientFlags & (PLAYER_CLIENTFLAGS_CUTSCENE | PLAYER_CLIENTFLAGS_DIALOGUE)) != 0;
			bool bThirdPerson = (pCD->m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON | PLAYER_EXTRAFLAGS_FORCETHIRPERSON)) != 0;
			int iPass = _pEngine->GetVCDepth();
			bool bFirstPerson = bLocalPlayer && !iPass && !bThirdPerson && !bCutSceneView;

			if (!bFirstPerson)
			{
				CXR_AnimState AnimState;
				CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(pCD->m_iLightCone);
				_pEngine->Render_AddModel(pModel, Mat, AnimState);
			}

			if(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_FLASHLIGHT && pCD->m_iPlayer == -1 && pSGI)
			{
				// Self light
				CXR_Light Light(Mat, CVec3Dfp32(((pCD->m_FlashLightColor.m_Value >> 16) & 0xff) / 255.0f,
					((pCD->m_FlashLightColor.m_Value >> 8) & 0xff) / 255.0f,
					((pCD->m_FlashLightColor.m_Value >> 0) & 0xff) / 255.0f) * 0.50f, 100, CXR_LIGHT_NOSHADOWS, CXR_LIGHTTYPE_POINT);
				Light.m_LightGUID = _pObj->m_iObject * 5 + 0x4000;
				pSGI->SceneGraph_Light_AddPrivateDynamic(Light);
			}
		}
	}

	if(pCD->m_iPlayer != -1)
	{
		int iCM = Char_GetControlMode(_pObj);
		if(pSGI && (iCM == PLAYER_CONTROLMODE_ACTIONCUTSCENE ||
		   iCM == PLAYER_CONTROLMODE_LADDER ||
		   iCM == PLAYER_CONTROLMODE_HANGRAIL ||
		   iCM == PLAYER_CONTROLMODE_LEDGE ||
		   iCM == PLAYER_CONTROLMODE_LEDGE2))
		{
			CXR_Light Light(_pEngine->GetVC()->m_CameraWMat, CVec3Dfp32(0.1f), 200, CXR_LIGHT_NOSHADOWS, CXR_LIGHTTYPE_POINT);
			Light.m_LightGUID = _pObj->m_iObject * 5 + 0x4004;
			pSGI->SceneGraph_Light_LinkDynamic(Light);
		}

		/*{
			bool bLocalPlayer = (_pObj->m_iObject == _pWClient->Player_GetLocalObject());
			bool bCutSceneView = (_pObj->m_ClientFlags & (PLAYER_CLIENTFLAGS_CUTSCENE | PLAYER_CLIENTFLAGS_DIALOGUE)) != 0;
			bool bThirdPerson = (pCD->m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON | PLAYER_EXTRAFLAGS_FORCETHIRPERSON)) != 0;
			int iPass = _pEngine->GetVCDepth();
			bool bFirstPerson = bLocalPlayer && !iPass && !bThirdPerson && !bCutSceneView;

			CWO_Character_ClientData *pCD = GetClientData(_pObj);
			CXR_SceneGraphInstance* pSGI = _pWClient->World_GetSceneGraphInstance();
			if (pSGI && pCD)
			{
				CXR_AnimState Anim;
				CXR_Skeleton* pSkel;
				CXR_SkeletonInstance* pSkelInst;
				const CMat4Dfp32& CharPos = _pObj->GetPositionMatrix();
				if(bFirstPerson && Char_IsPlayerViewControlled(_pObj))
				{
					if(GetEvaluatedPhysSkeleton(_pObj, _pWClient, pSkelInst, pSkel, Anim, pCD->m_PredictFrameFrac, &CharPos))
					{
						CMat4Dfp32 Mat = CharPos.Get4x4();
						RenderLaserDot(0, _pObj, _pWClient, _pEngine, pSkel, pSkelInst, &Mat);
						RenderLaserDot(1, _pObj, _pWClient, _pEngine, pSkel, pSkelInst, &Mat);
					}
				}
			}
		}*/
	}
}

// 
void CWObject_Character::DebugRenderSkeletonJoints(CWorld_Client* _pWClient,
												const CMat4Dfp32 &_Mat,
												CXR_SkeletonInstance* _pSkelInstance,
												CXR_Skeleton* _pSkel)
{
	TArray<CMat4Dfp32> lMat;
	lMat.SetLen(_pSkel->m_lNodes.Len());
	CalcMatrices_r(0,_Mat,_pSkelInstance,_pSkel,lMat);
	int i;
/*
	for (i = 0; i < _pSkel->m_lNodes.Len(); i++)
	{
		CXR_SkeletonNode* pN = &_pSkel->m_lNodes[i];
		CStr Msg = CStrF("Index %d, Parent %d iRot %d iMove %d",i,pN->m_iNodeParent,pN->m_iRotationSlot,pN->m_iMovementSlot);
		LogToSystemLog(Msg);
	}
*/
//	_pWClient->Debug_RenderSkeleton(_pSkel,_pSkelInstance,0x007f3f,1.0,true);
#if 1
	for (i = 1; i < lMat.Len(); i++)
	{
		CVec3Dfp32 BonePos = CVec3Dfp32::GetRow(lMat[i],3);
		_pWClient->Debug_RenderVertex(BonePos,0xff7f7f7f,0.5f);
	}
#endif

};

void CWObject_Character::CalcMatrices_r(int _iNode,
										const CMat4Dfp32 &_Mat,
										CXR_SkeletonInstance* _pSkelInstance,
										CXR_Skeleton* _pSkel,TArray<CMat4Dfp32> &_lMat)
{
	if(!_pSkel || _pSkel->m_lNodes.Len() <= _iNode)
		return;

	CXR_SkeletonNode* pN = &_pSkel->m_lNodes[_iNode];
	
	CMat4Dfp32 Mat, KMat;

	int iRot = pN->m_iRotationSlot;
	if((iRot >= 0) && (iRot < _pSkelInstance->m_nTracksRot))
		_pSkelInstance->m_pTracksRot[iRot].CreateMatrix(Mat);
	else
		Mat.Unit();
	
	int iMove = pN->m_iMovementSlot;
	if((iMove >= 0) && (iMove < _pSkelInstance->m_nTracksMove))
		Mat.r[3] = M_VSetW1(M_VNeg(_pSkelInstance->m_pTracksMove[iMove]));

	CVec3Dfp32 Pos = pN->m_LocalCenter;
	Pos *= Mat;
	Pos -= pN->m_LocalCenter;
	CVec3Dfp32::GetMatrixRow(Mat, 3) = -Pos;

	CMat4Dfp32 Tmp;
	Mat.Multiply(_Mat,Tmp);

	Pos = pN->m_LocalCenter;
	Pos *= Tmp;

	_lMat[_iNode] = Tmp;
	Pos.SetRow(_lMat[_iNode], 3);

	int nCh = pN->m_nChildren;
	for(int i = 0; i < nCh; i++)
	{
		CalcMatrices_r(_pSkel->m_liNodes[pN->m_iiNodeChildren + i],Tmp,_pSkelInstance,_pSkel,_lMat);
	}
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Character rendering.
					
	Contents:		OnClientRender
					OnClientRenderStatusBar
					Char_GetCamera
					GetChaseCam
\*____________________________________________________________________________________________*/
void CWObject_Character::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	MAUTOSTRIP(CWObject_Character_OnClientRender, MAUTOSTRIP_VOID);
	if (Char_GetPhysType(_pObj) == PLAYER_PHYS_NOCLIP) 
		return;

	if((_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_BRIEFING) != 0)
		return;

	CRegistry* pUserReg = _pWClient->Registry_GetUser();
#ifdef M_RTM
	int bNoRender = (pUserReg) ? pUserReg->GetValuei("NOCHARRENDER") : 0;
	if (bNoRender)
		return;
#endif
	
	MSCOPE(CWObject_Character::OnClientRender, CHARACTER);

	int iPass = _pEngine->GetVCDepth();
	bool bLocalPlayer = (_pObj->m_iObject == _pWClient->Player_GetLocalObject());
	bool bPredicted = false;
 
	bool bMirrored = (_pEngine->GetVCDepth() > 0);
//	bool bDead = (Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD) != 0;
	bool bCutSceneView = (_pObj->m_ClientFlags & (PLAYER_CLIENTFLAGS_CUTSCENE | PLAYER_CLIENTFLAGS_DIALOGUE)) != 0;

	//JK-NOTE: Setting pObjPlayer const so multithreading won't blow skyhigh, don't fuck with it inside a threaded call
	const CWObject_CoreData* pObjPlayer = _pWClient->Object_GetCD(_pWClient->Player_GetLocalObject());
	bool bCutSceneViewOther = (pObjPlayer) ? 
		((pObjPlayer->m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE) != 0) : false;

	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if (!pCD) return;

	// Return if invisible...
	if((pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_HIDDEN) || (pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_INVISIBLE))
		return;
	
	int bNoPredict = true;
#ifdef WCLIENT_FIXEDRATE
#else
	if (bLocalPlayer)
		bNoPredict = (pUserReg) ? pUserReg->GetValuei("NOPREDICT") : 0;
#endif
	bool bThirdPerson = (pCD->m_ExtraFlags & (PLAYER_EXTRAFLAGS_THIRDPERSON | PLAYER_EXTRAFLAGS_FORCETHIRPERSON)) != 0;
	bool bDebugCameraMode = (safe_cast<CWClient_Mod>(_pWClient)->m_DebugCamera != 0); // AR: I hate this, it's ugly!
	bool bFirstPerson = bLocalPlayer && !iPass && !bThirdPerson && !bCutSceneView && !bDebugCameraMode;

	if (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK)
	{
		CWObject_CoreData* pCreep = _pWClient->Object_GetCD(pCD->m_iCreepingDark);
		if (pCreep && CWObject_CreepingDark::Renderhead(pCreep))
			bFirstPerson = false;
	}

	if ((pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK) != THIRDPERSONINTERACTIVE_MODE_NONE && !pCD->m_3PI_NoCamera)
	{
		uint8 Slide8 = pCD->m_3PI_CameraSlide.Get(pCD->m_GameTick, pCD->m_PredictFrameFrac);
		if (Slide8 > 64) //FIXME: tweak this!
			bFirstPerson = false;
	}

	if (bLocalPlayer && !bNoPredict)
	{
		CWC_Player* pP = _pWClient->Player_GetLocal();
		if (!pP) return;

		if (_pObj->GetNext() != NULL)
		{
			_pObj = Player_GetLastValidPrediction(_pObj);
			bPredicted = true;
		}
	}

/*	if (pCD->m_SpawnTick == -1)
		return;

	if (pCD->m_SpawnTick > 0)
	{
		int HideTicks = PLAYER_SPAWNANIM_HIDETIME * SERVER_TICKSPERSECOND;
		int TicksSinceSpawn = (int32)pCD->m_GameTick - pCD->m_SpawnTick;
		if (TicksSinceSpawn <= HideTicks)
			return;
	}*/
	
	// Consider player to be zooming only if camera behind offset is smaller than 50 or so =).
	bool bZooming = ((pCD->m_MaxZoom > 1.0f) && (pCD->m_Camera_CurBehindOffset.LengthSqr() < Sqr(50.0f)));

	// Do whatever interpolation or extrapolation we should do.
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	CMat4Dfp32 MatIP;
	if (bPredicted && (!bCutSceneViewOther || (Char_GetControlMode(_pObj) == PLAYER_CONTROLMODE_ACTIONCUTSCENE)))
	{
		MatIP = _pObj->GetPositionMatrix();
		IPTime = pCD->m_PredictFrameFrac;

		/*fp32 Diff = pCD->m_GameTime.GetTime() - (pCD->m_GameTick + IPTime) * SERVER_TIMEPERFRAME;
		if(Abs(Diff) > 0.001f)
			ConOut(CStrF("GameTime diff: %f", Diff));*/
			//M_ASSERT(Diff > 0.001f, "GameTime shouldn't differ from GameTick+IPTime");
			//		But it does when pause is used

		{
			CWObject_CoreData* pObjFirst = _pWClient->Object_GetFirstCopy(_pObj->m_iObject);
			if (pObjFirst)
			{
				CWO_Character_ClientData *pCDFirst = GetClientData(pObjFirst);
				if (!pCDFirst) return;

				fp32 IPPredMiss = 1.0f - Clamp01(fp32(_pWClient->GetGameTick() - pCDFirst->m_PredMiss_Tick) + _pWClient->GetRenderTickFrac());
				CVec3Dfp32 PredMissAdd;
				pCDFirst->m_PredMiss_dPos.Scale(IPPredMiss, PredMissAdd);

				if (pCDFirst->m_LastTeleportTick != pCDFirst->m_GameTick)
					MatIP.GetRow(3) += PredMissAdd;
				//				MatIP.k[3][0] += 50.0f;

				//				LogFile(CStrF("Tick %d, Frac %f, IPPredMiss %f, dPos %s",_pWClient->GetGameTick(), _pWClient->GetRenderTickFrac(), IPPredMiss, PredMissAdd.GetString().Str()));
			}
		}
		// When in fighting mode the character might move outside the bounding box, this 
		// fixes so that the character at least is in the right position
		if (pCD->m_iPlayer != -1)
		{
			// Actioncutscene position
			int32 CameraMode = pCD->m_ActionCutSceneCameraMode;
			int32 ACSType = (int32)pCD->m_ControlMode_Param2;
			int32 Flags = pCD->m_ControlMode_Param4;
			if ((Flags & ACTIONCUTSCENE_FLAGS_USEFLOATINGSTART) && 
				((Char_GetControlMode(_pObj) == PLAYER_CONTROLMODE_ACTIONCUTSCENE) &&
				((ACSType == ACTIONCUTSCENE_TYPE_USEHEALTHSTATION) || (CameraMode & CActionCutsceneCamera::ACS_CAMERAMODE_ACTIVE) || 
				((pCD->m_3PI_Mode & THIRDPERSONINTERACTIVE_MODE_MASK) == THIRDPERSONINTERACTIVE_MODE_ACS))))
			{
				CWObject_ActionCutscene::GetStartPosition(MatIP, pCD, _pWClient);
			}
		}
	}
	else
	{
		if (pCD->m_LastTeleportTick == pCD->m_GameTick)
		{
			MatIP = _pObj->GetPositionMatrix();
		}
	/*	else if (bCutSceneViewOther)
		{
			MatIP = _pObj->GetPositionMatrix();

			// Get AnimRelOffsets
			CVec3Dfp32 AnimMoveRelOffset;
			CQuatfp32 AnimRotRelOffset;
			pCD->m_AnimGraph.m_AGI.GetAnimRelOffset(&AGIContext, pCD->m_AnimMoveOffset, pCD->m_AnimRotOffset, AnimMoveRelOffset, AnimRotRelOffset);

			// Apply AnimMoveRelOffset to MatIP
			AnimMoveRelOffset.AddMatrixRow(MatIP, 3);

			// Apply AnimRotRelOffset to MatIP
			//CMat4Dfp32 RotMat, TempMat;
			//AnimRotRelOffset.CreateMatrix3x3(RotMat);
			//CVec3Dfp32 TempPos = CVec3Dfp32::GetMatrixRow(MatIP, 3);
		//	RotMat.Multiply(MatIP, TempMat);
			//MatIP = TempMat;
			//TempPos.SetMatrixRow(MatIP, 3);
		}*/
		else
		{
			fp64 PeriodScale = _pWClient->GetModeratedFramePeriod() * _pWClient->GetGameTicksPerSecond();
			Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime / PeriodScale);
		}
	}

	if (pCD->m_RenderAttached != 0)
	{
		CWObject_Client* pObj = _pWClient->Object_Get(pCD->m_RenderAttached);
		if (pObj)
		{
			IPTime = _pWClient->GetRenderTickFrac();
			bFirstPerson = false;
		}
	}

	if(pCD->m_iMountedObject != 0)
	{
		if(pCD->m_iPlayer != -1)
			bFirstPerson = false;
		else
		{
			bFirstPerson = !(pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_RENDERHEAD);
			/*CMat4Dfp32 Pos = _pWClient->Object_GetPositionMatrix(pCD->m_iMountedObject);
			CVec3Dfp32::GetRow(MatIP, 3).SetRow(Pos, 3);
			MatIP = Pos;*/
		}
	}

	if(((pCD->m_DialogueInstance.m_SubtitleFlags & SUBTITLE_TYPE_MASK) == SUBTITLE_TYPE_CUTSCENE) ||
		((pCD->m_DialogueInstance.m_SubtitleFlags & SUBTITLE_TYPE_MASK) == SUBTITLE_TYPE_CUTSCENEKEEPENDANIM))
	{
		CWorld_ClientCore *pWClient = safe_cast<CWorld_ClientCore>(_pWClient);
		if(pWClient->m_spSound != NULL)
		{
			int iHold = pCD->m_AnimGraph2.GetPropertyInt(PROPERTY_INT_VOCAPINFO) & AG2_VOCAP_FLAG_IDLE;
			int nLayers = 0;
			pCD->m_VoCap.Eval(pWClient->m_spSound, _pObj->m_ClientData[0], pCD->m_GameTime, iHold, NULL, nLayers, 0);
			CVoCap_AnimItem *pQueue = pCD->m_VoCap.m_spAnimQueue;
			if(pQueue && pQueue->m_spCurSequence)
			{
				if (pCD->m_RenderAttached == 0)
					MatIP = pCD->m_VocapOrigin;
				CMat4Dfp32 RotMat,Temp;
				vec128 Move;
				CQuatfp32 Rot;
				pQueue->m_spCurSequence->EvalTrack0(CMTime::CreateFromSeconds(pQueue->m_Time), Move, Rot);
				Move = M_VMul(Move,M_VLdScalar(pCD->m_CharGlobalScale * (1.0f - pCD->m_CharSkeletonScale)));
				Move = M_VSetW1(Move);
				Rot.CreateMatrix(RotMat);
				MatIP.Multiply3x3(RotMat,Temp);
				Move = M_VMulMat4x3(Move, MatIP);
				MatIP = Temp;
				MatIP.r[3] = Move;
			}
		}
	}

/*	if(pCD->m_iPlayer != -1)
		ConOut(CStrF("Player time: %f", (pCD->m_GameTick - _pObj->m_AnimTime + IPTime) * SERVER_TIMEPERFRAME));
	else
		ConOut(CStrF("AI time: %f", (pCD->m_GameTick - _pObj->m_AnimTime + IPTime) * SERVER_TIMEPERFRAME));*/
/*	if(pCD->m_Cutscene_Camera != 0)
	{
		CWObject_Client *pCamera = _pWClient->Object_Get(pCD->m_Cutscene_Camera);
		ConOut(CStrF("Camera time: %f", (pCD->m_GameTick - pCamera->m_AnimTime + IPTime) * SERVER_TIMEPERFRAME));
	}*/
	
	if(_pObj->m_iModel[0] == 0)
		return;

	// Get all models and skeletons and create anim track mask
	CXR_Model* lpObjectModels[CWO_NUMMODELINDICES];
	CXR_Skeleton* lpObjectModelSkel[CWO_NUMMODELINDICES];
	int liVariations[CWO_NUMMODELINDICES];
#ifndef M_RTM
	int liLOD[CWO_NUMMODELINDICES];
	memset(liLOD, 0, sizeof(liLOD));
#endif

	{
		for(int i = 0; i < CWO_NUMMODELINDICES; i++)
		{
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model((i == 0 && pCD->m_HeadGibbed) ? pCD->m_HeadGibbed.m_Value : _pObj->m_iModel[i]);
			if (!pModel)
			{
				lpObjectModels[i] = NULL;
				lpObjectModelSkel[i] = NULL;
				continue;
			}

			// Since the code below manually fetches the LOD models, we need to manually resolve variation index aswell
			CXR_AnimState Tmp;
			pModel = pModel->OnResolveVariationProxy(Tmp, Tmp);
			liVariations[i] = Tmp.m_Variation;

			if(!bLocalPlayer)
			{
#ifndef M_RTM
				pModel = pModel->GetLOD(MatIP, _pEngine->GetVC()->m_W2VMat, _pEngine, &liLOD[i]);
#else
				pModel = pModel->GetLOD(MatIP, _pEngine->GetVC()->m_W2VMat, _pEngine);
#endif
			}
			lpObjectModels[i] = pModel;

			CXR_Skeleton* pSkel = pModel->GetSkeleton();
			lpObjectModelSkel[i] = pSkel;
		}
	}

/*#ifndef M_RTM
	static bool bFirstSkelMismatchLog = true;
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys && pSys->GetEnvironment()->GetValuei("CON_SHOWSKELETONMISMATCH", 0))
		bFirstSkelMismatchLog = true;
	if(bFirstSkelMismatchLog)
	{
		if (lpObjectModelSkel[0] && lpObjectModelSkel[1])
		{
			CXR_Skeleton* pS0 = lpObjectModelSkel[0];
			CXR_Skeleton* pS1 = lpObjectModelSkel[1];
			CXR_Skeleton* pS2 = lpObjectModelSkel[2];

			if (pS0->m_lNodes.Len() != pS1->m_lNodes.Len() ||
				pS0->m_nUsedRotations != pS1->m_nUsedRotations ||
				pS0->m_nUsedMovements != pS1->m_nUsedMovements ||
				(pS2 && (
				 pS2->m_lNodes.Len() != pS1->m_lNodes.Len() ||
				 pS2->m_nUsedRotations != pS1->m_nUsedRotations ||
				 pS2->m_nUsedMovements != pS1->m_nUsedMovements)))
			{
				CXR_Model_TriangleMesh* pTM0 = TDynamicCast<CXR_Model_TriangleMesh>(lpObjectModels[0]);
				CXR_Model_TriangleMesh* pTM1 = TDynamicCast<CXR_Model_TriangleMesh>(lpObjectModels[1]);
				CStr MeshName0 = (pTM0) ? pTM0->m_MeshName.GetFilenameNoExt() : CStr();
				CStr MeshName1 = (pTM1) ? pTM1->m_MeshName.GetFilenameNoExt() : CStr();

				CStr St = CStrF("§cf80WARNING: Skeleton missmatch. iObj %d,  Body %s (LOD %d, Nodes %d, UsedRot %d, UsedMove %d), Head %s (LOD %d, Nodes %d, UsedRot %d, UsedMove %d)", 
					_pObj->m_iObject, 
					MeshName0.GetStr(), liLOD[0], pS0->m_lNodes.Len(), pS0->m_nUsedRotations, pS0->m_nUsedMovements, 
					MeshName1.GetStr(), liLOD[1], pS1->m_lNodes.Len(), pS1->m_nUsedRotations, pS1->m_nUsedMovements);
				ConOutL(St);
				bFirstSkelMismatchLog = false;
			}
		}
	}
#endif*/

	CXR_Model* pModel = lpObjectModels[0];
	CXR_Skeleton* pSkel = lpObjectModelSkel[0];
/*	for (uint i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		uint nCurr = pSkel ? pSkel->m_lNodes.Len() : 0;
		if (lpObjectModelSkel[i] && lpObjectModelSkel[i]->m_lNodes.Len() > nCurr)
			pSkel = lpObjectModelSkel[i];
	}*/
	if (!pSkel)
		return;

//	CXR_Model *pShadowModel = pModel;
//	int OcclusionMask = (pModel) ? pModel->GetParam(CTM_PARAM_OCCLUSIONMASK) : 0;

	// Init animstate
	CVec3Dfp32 AnimatedPos;
	CXR_AnimState Anim;
	if(pCD->m_iPlayer != -1)
	{
		CWObject_CoreData* pObjFirst = _pWClient->Object_GetFirstCopy(_pObj->m_iObject);
		if (pObjFirst)
		{
			CWO_Character_ClientData *pCDFirst = GetClientData(pObjFirst);
			CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(MatIP,3);
			Pos += pCDFirst->m_Camera_CharacterOffset;
			Pos.SetMatrixRow(MatIP,3);
		}
	}

	uint nNodesNeeded = pSkel->m_lNodes.Len();
	for (uint i = 1; i < CWO_NUMMODELINDICES; i++)
	{
		CXR_Skeleton* pExtraSkel = lpObjectModelSkel[i];
		if (pExtraSkel)
			nNodesNeeded = Max(nNodesNeeded, (uint)pExtraSkel->m_lNodes.Len());
	}
		
	Anim.m_pSkeletonInst = CXR_VBMHelper::Alloc_SkeletonInst(_pEngine->GetVBM(), nNodesNeeded, pSkel->m_nUsedRotations, pSkel->m_nUsedMovements);
	if (!Anim.m_pSkeletonInst)
		return;

	fp32 OverrideRagdollHeight = 0.0f;
	if(!bLocalPlayer && Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD && _pWClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), _pWClient->Game_GetObjectIndex()))
	{
		int DeadTicks = _pWClient->GetGameTick() - pCD->m_DeathTick;
		OverrideRagdollHeight = DeadTicks * (4.0f / 30.0f);
		if(DeadTicks > 120)
			return;
	}

	// Cutscene skeletoninstance blending
	if (pCD->m_iMountedObject != 0 && (pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_BLENDANIM))
	{
		CXR_SkeletonInstance* pSkelInst = NULL;
		CWObject_Client* pOther = _pWClient->Object_Get(pCD->m_iMountedObject);
		fp32 BlendTime = 0.5f; // Fixed blendtime atm..
		bool bOk = false;
		if (pOther)
		{
			CWO_Character_ClientData *pCDOther = CWObject_Character::GetClientData(pOther);
			fp32 IPTimeOther = pCDOther->m_PredictFrameFrac;
			if (pOther->GetNext() != NULL)
			{
				pOther = Player_GetLastValidPrediction(pOther);
				pCDOther = CWObject_Character::GetClientData(pOther);
				IPTimeOther = pCDOther->m_PredictFrameFrac;
			}
			CMTime OtherTime = CMTime::CreateFromTicks(_pWClient->GetGameTick() - pCD->m_MountedStartTick,_pWClient->GetGameTickTime(),IPTime);
			fp32 SkelBlend = Max(0.0f,Min(1.0f,(fp32)(OtherTime.GetTime()) / BlendTime));
			if (pCD->m_MountedMode & PLAYER_MOUNTEDMODE_FLAG_BLENDOUTANIM)
			{
				// We're about to blend out, reverse order
				SkelBlend = 1.0f - SkelBlend;
			}
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(pOther->m_iModel[0]);
			CXR_Skeleton* pSkelOther = pModel ? pModel->GetSkeleton() : NULL;
			CMat4Dfp32 MatIPOther;
			Interpolate2(pOther->GetLastPositionMatrix(), pOther->GetPositionMatrix(), MatIPOther, IPTimeOther / (_pWClient->GetModeratedFramePeriod() * _pWClient->GetGameTicksPerSecond()));
			if (SkelBlend > 0.0f)
			{
				if (SkelBlend < 1.0f)
				{
					// Get animstate for this object, the target object and blend them
					CXR_AnimState AnimThis,AnimOther;
					CVec3Dfp32 AnimPosOther,AnimPosThis;
					OnGetAnimState(pOther,_pWClient,pSkelOther,0,MatIPOther,IPTimeOther,AnimOther,&AnimPosOther);
					OnGetAnimState(_pObj,_pWClient,lpObjectModelSkel[0],0,MatIP,IPTime,AnimThis,&AnimPosThis);
					if (AnimThis.m_pSkeletonInst && AnimOther.m_pSkeletonInst)
					{
						AnimOther.m_pSkeletonInst->BlendInstance(AnimThis.m_pSkeletonInst,Anim.m_pSkeletonInst,SkelBlend);
						// Evaluate target skeleton
						CMat4Dfp32 MatBlend;
						CVec3Dfp32 VRes;
						CVec3Dfp32::GetMatrixRow(AnimOther.m_pSkeletonInst->m_pBoneTransform[0], 3).Lerp(CVec3Dfp32::GetMatrixRow(AnimThis.m_pSkeletonInst->m_pBoneTransform[0], 3), SkelBlend, VRes);

						CQuatfp32 Q1, Q2;
						Q1.Create(AnimOther.m_pSkeletonInst->m_pBoneTransform[0]);
						Q2.Create(AnimThis.m_pSkeletonInst->m_pBoneTransform[0]);

						CQuatfp32 QRes;
						Q1.Lerp(Q2, SkelBlend, QRes);

						QRes.CreateMatrix(MatBlend);
						VRes.SetMatrixRow(MatBlend, 3);
						lpObjectModelSkel[0]->EvalNode(PLAYER_ROTTRACK_ROOT, &MatBlend, Anim.m_pSkeletonInst);
						bOk = true;
					}
				}
			}
			else
			{
				// Do Other char
				OnGetAnimState2(pOther, _pWClient, lpObjectModels, MatIPOther, IPTimeOther, Anim, &AnimatedPos, true, bFirstPerson ? 1.0f : -1.0f);
				bOk = true;
			}
		}
		if (!bOk)
		{
			// Do this char
			OnGetAnimState2(_pObj, _pWClient, lpObjectModels, MatIP, IPTime, Anim, &AnimatedPos, true, bFirstPerson ? 1.0f : -1.0f);	
		}
	}
	else
	{
		OnGetAnimState2(_pObj, _pWClient, lpObjectModels, MatIP, IPTime, Anim, &AnimatedPos, true, bFirstPerson ? 1.0f : -1.0f, OverrideRagdollHeight);
	}
	Anim.m_pModelInstance = _pObj->m_lModelInstances[0];

#ifndef M_RTM
	static bool REPORT_MISSING_JOINTS = false;
	if (lpObjectModelSkel[1] || lpObjectModelSkel[2])
	{ // Hack to prevent random matrix palette data when model1/model2 contains bones not used by model0
	  // (will set bind pose matrix, so that the error is still visible)
		bool bHeader = false;
		enum { nWords = sizeof_buffer(lpObjectModelSkel[0]->m_TrackMask.m_TrackMaskRot.m_lMask), iClothStart = 100 };
		const static uint32 aEmptyMask[nWords] = {0};
		const uint32* M_RESTRICT pMask0 = lpObjectModelSkel[0]->m_TrackMask.m_TrackMaskRot.m_lMask;
		const uint32* M_RESTRICT pMask1 = lpObjectModelSkel[1] ? lpObjectModelSkel[1]->m_TrackMask.m_TrackMaskRot.m_lMask : aEmptyMask;
		const uint32* M_RESTRICT pMask2 = lpObjectModelSkel[2] ? lpObjectModelSkel[2]->m_TrackMask.m_TrackMaskRot.m_lMask : aEmptyMask;
		CMat4Dfp32* M_RESTRICT pBones = Anim.m_pSkeletonInst->m_pBoneTransform;

		for (uint i = 0; i < nWords; i++)
		{
			uint32 x = (pMask1[i] | pMask2[i]) & ~pMask0[i];	// check bits found in mask1/mask2 and not in mask0
			for (uint iRot = 32*i; x > 0 && iRot < iClothStart; x >>= 1, iRot++)
			{
				if (x & 1)
				{
if (REPORT_MISSING_JOINTS)
{
					if (!bHeader)
					{
						M_TRACE("[Char %d] model0 is missing joints!\n - Model0: %s (lod %d)\n - Model1: %s (lod %d)\n - Model2: %s (lod %d)\nJoints:",
							_pObj->m_iObject, 
							lpObjectModels[0] ? ((CXR_Model_TriangleMesh*)lpObjectModels[0])->m_FileName.Str() : "(n/a)", liLOD[0],
							lpObjectModels[1] ? ((CXR_Model_TriangleMesh*)lpObjectModels[1])->m_FileName.Str() : "(n/a)", liLOD[1],
							lpObjectModels[2] ? ((CXR_Model_TriangleMesh*)lpObjectModels[2])->m_FileName.Str() : "(n/a)", liLOD[2]);
						bHeader = true;
					}
					M_TRACE(" %d", iRot);
}
					pBones[iRot] = MatIP;
				}
			}
		}
		if (bHeader)
			M_TRACE("\n");
	}
#endif

	const fp32 GameTickTime = _pWClient->GetGameTickTime();

	// WB hack
	if(pCD->m_Anim_FreezeTick != 0)
		Anim.m_AnimTime0 = CMTime::CreateFromTicks(pCD->m_GameTick - pCD->m_Anim_FreezeTick, GameTickTime, IPTime);

	uint32 GlobalOnRenderFlags = 0;
	if(pCD->m_ElectroShockTick != 0)
	{
		CMTime Duration = pCD->m_GameTime - CMTime::CreateFromTicks(pCD->m_ElectroShockTick, GameTickTime);
		if(Duration.GetTime() < 4)
		{
			CXW_Surface* pSurf = _pEngine->GetSC()->GetSurface("ELECTROSHOCK");
			if(pSurf)
			{
				Anim.m_lpSurfaces[1] = pSurf;
				Anim.m_AnimTime1 = Duration;
				GlobalOnRenderFlags |= CXR_MODEL_ONRENDERFLAGS_SURF0_ADD;
			}
		}
	}

	// Darkling fade effect
	if (pCD->IsDarkling())
	{
		fp32 AnimTime;
		if (pCD->m_DeathTick != -1)
		{
			fp32 TimeSinceDeath = (pCD->m_GameTick - pCD->m_DeathTick) * GameTickTime;
			AnimTime = TimeSinceDeath;
		}
		else
		{
			fp32 TimeSinceSpawn = (pCD->m_GameTick - pCD->m_SpawnTick) * GameTickTime;
			AnimTime = Max(0.0f, 1.8f - TimeSinceSpawn);
		}
		Anim.m_AnimTime0 = CMTime::CreateFromSeconds(AnimTime);
	}

	uint Opacity = Max((uint)(pCD->m_Opacity.m_Target ? 1 : 0),(uint)pCD->m_Opacity.Get(pCD->m_GameTick, IPTime));
	if (Opacity != 0)
	{ // Assumes a surface with animated alpha (0.0s - transparent,  1.0s - opaque)
		Anim.m_AnimTime0 = CMTime::CreateFromTicks(Opacity - 1, 1.0f / 254.0f);
	}

//	CXR_SkeletonInstance* pSkelInstance = GetClientSkelInstance(pCD, 0);
	CXR_SkeletonInstance* pSkelInstance = Anim.m_pSkeletonInst;
	if (!pSkelInstance || !pModel) return;
	// ***
	
	if (_pWClient && pSkelInstance)
	{
		CMat4Dfp32 Mat;
		Mat.Unit();
//		DebugRenderSkeletonJoints(_pWClient,Mat,pSkelInstance,pSkel);
	}

#ifndef M_RTM
	if (0) { // Debug render camera frustum
		CMat4Dfp32 CameraMat = MatIP;
		Camera_Get_FirstPerson(_pWClient, &CameraMat, _pObj, IPTime);

		CRC_Viewport Viewport;
		_pWClient->Render_GetLastRenderViewport(Viewport);

		CVec3Dfp32 lV[5];
		for (uint i = 0; i < 5; i++)
			lV[i] = Viewport.GetViewVertices()[i] * CameraMat;

		_pWClient->Debug_RenderVertex(lV[0], 0xffff0000, 0.0f, false);
		for (uint i = 0; i < 4; i++)
			_pWClient->Debug_RenderWire(lV[0], lV[i+1], 0xffff0000, 0.0f, false);
	}
#endif

	// ***

#if 0
	//JK-NOTE: Focus frame is broken, do not use without rewrite
	if(bLocalPlayer)
	{
		// Focusframe object
		if ((pCD->m_iFocusFrameObject != 0) && (Char_IsPlayerViewControlled(_pObj)) &&
			!(Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD) &&
			!(safe_cast<CWClient_Mod>(_pWClient))->m_bHideHUD)
		{
			/*int32 iFocusModel = _pWClient->GetMapData()->GetResourceIndex_Model("focusframe");
			CXR_Model* pFocusFrame = _pWClient->GetMapData()->GetResource_Model(iFocusModel);*/
			//CRC_Font* pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
			// Get bounding box size and set it to the animstate
			CWObject_CoreData* pFocusObject = _pWClient->Object_GetCD(pCD->m_iFocusFrameObject);
			//static bool bFocus = false;
			if (pFocusObject)
			{
				/*CBox3Dfp32 Box;
				pFocusObject->GetVisBoundBox(Box);
				fp32 Radius = Max(M_Fabs(Min(Box.m_Min.k[0], Min(Box.m_Min.k[1],Box.m_Min.k[2]))),
					Max(Box.m_Max.k[0], Max(Box.m_Max.k[1], Box.m_Max.k[2])));*/

				CXR_ViewContext* pContext = _pEngine->GetVC();
				CRC_Viewport ViewPort;
				CMat4Dfp32 CamMat;
				_pWClient->Render_GetLastRenderViewport(ViewPort);
				pContext->m_W2VMat.Multiply(ViewPort.GetProjectionMatrix(),CamMat);

				CMat4Dfp32 FocusMatIP;
				fp32 FocusIPTime = _pWClient->GetRenderTickFrac();
				fp64 PeriodScale = _pWClient->GetModeratedFramePeriod() * _pWClient->GetGameTicksPerSecond();
				Interpolate2(pFocusObject->GetLastPositionMatrix(), pFocusObject->GetPositionMatrix(), FocusMatIP, FocusIPTime / PeriodScale);
				CVec3Dfp32 Pos = CVec3Dfp32::GetRow(FocusMatIP, 3);

				if ((pCD->m_FocusFrameType & SELECTION_MASK_TYPE) == SELECTION_CHAR)
				{
					// Get skeleton for this char and get position of torso (or something)
					// Find position from skeleton
					// Find skeleton and evalutate current headoffset
//					CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(pFocusObject->m_iModel[0]);
//					CXR_Model* pModel1 = _pWClient->GetMapData()->GetResource_Model(pFocusObject->m_iModel[1]);
//					CXR_Skeleton *pSkel1 = pModel1 ? pModel1->GetPhysSkeleton() : NULL;
//					CXR_Skeleton *pSkel0 = pModel ? pModel->GetPhysSkeleton() : NULL;

/*					CXR_Skeleton *pSkel = pSkel1;
					if(!pSkel || (pSkel0 && pSkel0->m_lNodes.Len() > pSkel->m_lNodes.Len()))
						pSkel = pSkel0;*/

					CXR_AnimState Anim;
					bool bOk = false;

					if (pSkel && CWObject_Character::OnGetAnimState(pFocusObject, _pWClient, pSkel, 1, FocusMatIP, FocusIPTime, Anim, NULL))
					{
						if(PLAYER_ROTTRACK_RARM < Anim.m_pSkeletonInst->m_nBoneTransform && pSkel->m_lNodes.ValidPos(PLAYER_ROTTRACK_RARM))
						{
							CMat4Dfp32 Trans = Anim.m_pSkeletonInst->GetBoneTransform(PLAYER_ROTTRACK_RARM);
							Pos = pSkel->m_lNodes[PLAYER_ROTTRACK_RARM].m_LocalCenter;
							Pos *= Trans;
							bOk = true;
						}
					}
					if (!bOk)
						Pos.k[2] += pCD->m_FocusFrameOffset;
				}
				else
				{
					Pos.k[2] += pCD->m_FocusFrameOffset;
				}
				CVec4Dfp32 Screenpos;
				Screenpos.k[0] = Pos.k[0];
				Screenpos.k[1] = Pos.k[1];
				Screenpos.k[2] = Pos.k[2];
				Screenpos.k[3] = 1.0f;
				Screenpos = Screenpos * CamMat;
				Screenpos.k[0] /= Screenpos.k[3];//ViewPort.GetViewRect().GetWidth();
				Screenpos.k[1] /= Screenpos.k[3] * -1;//ViewPort.GetViewRect().GetHeight();
				Screenpos.k[0] += (fp32)ViewPort.GetViewRect().GetWidth() * 0.5 + ViewPort.GetViewRect().p0.x;
				Screenpos.k[1] += (fp32)ViewPort.GetViewRect().GetHeight() * 0.5 + ViewPort.GetViewRect().p0.y;
				Screenpos.k[0] /= ViewPort.GetViewRect().GetWidth();
				Screenpos.k[1] /= ViewPort.GetViewRect().GetHeight();

				pCD->m_FocusFramePosX = Screenpos.k[0];
				pCD->m_FocusFramePosY = Screenpos.k[1];

				/*CXR_AnimState AnimState;
				AnimState.m_AnimAttr0 = 0.5f;
				AnimState.m_AnimAttr1 = Radius;
				AnimState.m_pContext = pFont;
				AnimState.m_Colors[0] = (int32)pCD->m_FocusFrameUseText.m_Value.Str();
				AnimState.m_Colors[1] = (int32)pCD->m_FocusFrameDescText.m_Value.Str();
				// Offset 
				//AnimState.m_SurfAnim0 = (pCD->m_FocusFrameType == SELECTIONISCHAR ? 64 : 0);
				CMat4Dfp32 PosMat = _pWClient->Object_GetPositionMatrix(pCD->m_iFocusFrameObject) ;
				if (pCD->m_FocusFrameType != SELECTIONISDEADCHAR)
					PosMat.k[3][2] += pCD->m_FocusFrameOffset;//(pCD->m_FocusFrameType == SELECTIONISCHAR ? pCD->m_FocusFrameOffset : 0.0f);
				if (bFocus)
					_pEngine->Render_AddModel(pFocusFrame, PosMat, AnimState);*/
			}
		}
	}
#endif

	const int MaxModels = 6;
	int nModels = 0;
	uint16 lModelOnRenderFlags[MaxModels];
	CXR_AnimState lModelAnims[MaxModels];
	CXR_Model* lpModels[MaxModels];
	CMat4Dfp32 lModelPos[MaxModels];
	FillChar(lModelOnRenderFlags, sizeof(lModelOnRenderFlags), 0);

	int GUIDOffset = 0x2000;
	int ModelGUID = _pObj->m_iObject;

	// Fade the character
	if(bCutSceneView)
	{
		pCD->m_iWeaponFade = 255;
	}
	
	// Nightvision lighting
	if(!bLocalPlayer && (Char_GetPhysType(_pObj) != PLAYER_PHYS_DEAD))
	{
		int iObj = _pWClient->Player_GetLocalObject();

		CWO_CameraEffects NVInfo;
		CWObject_Message Msg(OBJMSG_CHAR_GETNVINTENSITY);
		Msg.m_pData = &NVInfo;

		if(_pWClient->ClientMessage_SendToObject(Msg, iObj))
		{
			fp32 Intens = 0;
			if(NVInfo.m_Nightvision > 0)
				Intens += 0.3f * NVInfo.m_Nightvision;
			
/*			if(NVInfo.m_Crouch > 0.0f && (NVInfo.m_Sneak > 0.8f || NVInfo.m_NV > 0.8f))
			{
				fp32 Dist = (_pWClient->Object_GetPosition(iObj) - _pObj->GetPosition()).Length();
				if(Dist < 512.0f)
				{
					fp32 HeartBeat = GetHeartBeat(_pObj, _pWClient, CMTime::CreateFromTicks(pCD->m_GameTick, SERVER_TIMEPERFRAME, IPTime), false);
					Intens += HeartBeat * (512.0f - Dist) / 512.0f;
				}
			}

			if(Intens > 0)
			{
				CVec3Dfp32 Pos = _pWClient->Object_GetPosition(iObj);
				Pos[2] += 56;
				CVec3Dfp32 Dir = Pos - _pObj->GetPosition();
				Dir *= 100 / Dir.Length();
				CXR_Light NVLight(_pObj->GetPosition() + Dir, CVec3Dfp32(Intens), 2000, CXR_LIGHT_NOSHADOWS, CXR_LIGHTTYPE_POINT);
				NVLight.m_LightGUID = _pObj->m_iObject * 5 + 0x4000;
				_pEngine->m_pSceneGraphInstance->SceneGraph_Light_AddPrivateDynamic(NVLight);
				Anim.m_Data[3] = NVLight.m_LightGUID;
			}*/
		}
	}

	bool bFlashLight = (_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_FLASHLIGHT) != 0;
	if(bFlashLight)
		Anim.m_Data[3] = _pObj->m_iObject * 5 + 0x4000;

	lpModels[nModels] = pModel;
	lModelAnims[nModels] = Anim;
	lModelAnims[nModels].m_GUID = ModelGUID;
	lModelOnRenderFlags[nModels] = CXR_MODEL_ONRENDERFLAGS_MAXLOD;
	ModelGUID += GUIDOffset;

	CMat4Dfp32 OnlyZRot = MatIP;
	OnlyZRot.r[2]=M_VConst(0,0,1.0f,0);
	OnlyZRot.RecreateMatrix<2,0>();
	lModelPos[nModels] = OnlyZRot;

	nModels++;

	CMat4Dfp32 ExtraModelMat;
	ExtraModelMat = MatIP;
//	_pObj->GetAbsBoundBox()->GetCenter(CVec3Dfp32::GetMatrixRow(ExtraModelMat, 3));

	for (int iModel = 1; iModel < CWO_NUMMODELINDICES; iModel++)
	{
		CXR_Model *pModel = lpObjectModels[iModel];
		if ((pModel != NULL) && true)
		{
			lpModels[nModels] = pModel;
			lModelPos[nModels] = ExtraModelMat;
			lModelAnims[nModels] = Anim;
		//	lModelAnims[nModels].m_AnimTime0 = pCD->m_GameTime;
			lModelAnims[nModels].m_pModelInstance = _pObj->m_lModelInstances[iModel];
			lModelAnims[nModels].m_GUID = ModelGUID;
			lModelOnRenderFlags[nModels] = CXR_MODEL_ONRENDERFLAGS_MAXLOD;
			nModels++;
		}
		ModelGUID += GUIDOffset;
	}

	if(!(Char_GetPhysType(_pObj) == PLAYER_PHYS_DEAD))
	{
		if(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_LASERBEAM)
		{
			// Laserbeam from head
			CMat4Dfp32 Mat;
			CXR_AnimState AnimState;
			CAutoVar_AttachModel Model;
			Model.m_iAttachPoint[0] = 1;
			Model.m_iAttachRotTrack = PLAYER_ROTTRACK_HEAD;
			Model.m_iModel[0] = pCD->m_iLaserBeam;
			CXR_Model *pHeadModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[1]);
			CXR_Skeleton *pHeadSkel = pHeadModel ? (CXR_Skeleton*)pHeadModel->GetParam(MODEL_PARAM_SKELETON) : NULL;
			bool bValid = false;
			if(pHeadSkel)
			{
				CXR_Model *pModel;
				if(Model.GetModel0_RenderInfo(_pWClient->GetMapData(), NULL,pSkelInstance, pHeadSkel, CMTime::CreateFromTicks(pCD->m_GameTick, GameTickTime, IPTime) , AnimState, Mat, pModel, _pWClient))
					bValid = true;
			}
			if(bValid)
			{
				AnimState.m_Data[3] = 0xffff0000;

				CCollisionInfo Info;
				Info.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
				AnimState.m_AnimAttr0 = 256; // Max length
				CVec3Dfp32 &From = CVec3Dfp32::GetRow(Mat, 3);
				CVec3Dfp32 To = From + CVec3Dfp32::GetRow(Mat, 0) * AnimState.m_AnimAttr0;
				if(_pWClient->Phys_IntersectLine(From, To, 0, OBJECT_FLAGS_PHYSMODEL /*| OBJECT_FLAGS_PHYSOBJECT*/, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &Info, _pObj->m_iObject) && Info.m_bIsValid)
					AnimState.m_AnimAttr0 *= Info.m_Time;

				CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(pCD->m_iLaserBeam);
				//AnimState.m_AnimTime0.Reset();

				_pEngine->Render_AddModel(pModel, Mat, AnimState);
			}
		}

		if(pCD->m_iFlagIndex != -1)
		{
			CMat4Dfp32 Mat;
			Mat = pCD->m_pObj->GetPositionMatrix();

			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(pCD->m_iFlagIndex);

			_pEngine->Render_AddModel(pModel, Mat, AnimState);
		}

		if(bLocalPlayer && Char_IsPlayerViewControlled(_pObj))
		{
			//if(pCD->m_Item0_Flags & RPG_ITEM_FLAGS_LASERBEAM)
				RenderLaserDot(0, _pObj, _pWClient, _pEngine, pSkel, pSkelInstance, &MatIP);
			//if (pCD->m_Item1_Flags && RPG_ITEM_FLAGS_LASERBEAM)
				RenderLaserDot(1, _pObj, _pWClient, _pEngine, pSkel, pSkelInstance, &MatIP);
		}


/*		if(bLaserDot)
		{
			CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
//			CXR_SkeletonInstance* pSkelInstance = GetClientSkelInstance(_pObj);
			if(pModel)
			{
				CXR_Skeleton* pSkel = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);

				CMat4Dfp32 Mat;
				CXR_AnimState AnimState;
				if(pSkel)
				{
					CCollisionInfo Info;
					Info.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
					Info.SetReturnValues(CXR_COLLISIONRETURNVALUE_POSITION | CXR_COLLISIONRETURNVALUE_SURFACE);
					CVec3Dfp32 From;
					bool bUnequip = pCD->m_WeaponUnequipTick > 0 && ((pCD->m_GameTick - pCD->m_WeaponUnequipTick) < 5 * _pWClient->GetGameTicksPerSecond());
					bool bRenderInfo = bUnequip ? false : pCD->m_Item0_Model.GetModel0_RenderInfo(_pWClient->GetMapData(), NULL, pSkelInstance, pSkel, pCD->m_GameTime, 
						AnimState, Mat, pModel, _pWClient);

					// If we are equipping/unequipping, use the other type of position
					if (bUnequip || (bRenderInfo && 
						((pCD->m_AnimGraph2.GetStateFlagsLoCombined() & AG2_STATEFLAG_EQUIPPING))))
					{
						// Get bone position of erm, some bone, (neck seems to give a good position)
						CMat4Dfp32 Trans;
						Anim.m_pSkeletonInst->GetBoneTransform(PLAYER_ROTTRACK_NECK,&Trans);
						From = pSkel->m_lNodes[PLAYER_ROTTRACK_NECK].m_LocalCenter;
						From *= Trans;
						From -= CVec3Dfp32(0.5f,4.0f,1.234f);//Char_GetPhysType(_pObj) == PLAYER_PHYS_CROUCH ? CVec3Dfp32(0,5.36,18.5): CVec3Dfp32(0.5f,4.0f,1.234f);
					}
					else if (bRenderInfo)
					{
						CXR_Skeleton* pSkelItem = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
						const CXR_SkeletonAttachPoint *pAttach = pSkelItem ? pSkelItem->GetAttachPoint(1) : NULL;
						if (pAttach)
							Mat.GetRow(3) = pAttach->m_LocalPos.GetRow(3) * Mat;

						From = CVec3Dfp32::GetRow(Mat, 3);
					}		

					CVec3Dfp32 Dest = From + CVec3Dfp32::GetRow(MatIP, 0) * 2048;

					//_pWClient->Debug_RenderWire(From, Dest, -1, 0.05f, false);
					if((bUnequip || bRenderInfo) && _pWClient->Phys_IntersectLine(From, Dest, OBJECT_FLAGS_PROJECTILE, OBJECT_FLAGS_PHYSOBJECT | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_CHARACTER, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS, &Info, _pObj->m_iObject) && Info.m_bIsValid)
					{
						int iModel = _pWClient->GetMapData()->GetResourceIndex_Model("Sprite:laserdot");
						CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(iModel);
						if(pModel)
						{
							CMat4Dfp32 Mat;
							Mat.Unit();
							fp32 Scaler = Max( 1.0f, (Info.m_Pos - From).Length() * (1.0f/32.0f) );
							(Info.m_Pos - (Dest - From).Normalize() * Scaler).SetRow(Mat, 3);
							CXR_AnimState AnimState;
							fp32 Size = 3.0f + CVec3Dfp32::GetRow(_pEngine->GetVC()->m_CameraWMat, 3).Distance(Info.m_Pos) / 50;
							Size *= 0.2f;
							int iSize = TruncToInt(Size);
							AnimState.m_Anim0 = iSize;
							AnimState.m_AnimTime0 = CMTime::CreateFromSeconds(0.01f);
							AnimState.m_Anim1 = (int16)((Size - iSize) * 100);

							_pEngine->Render_AddModel(pModel, Mat, AnimState);
#ifndef M_RTM
							CRegistry* pUserReg = _pWClient->Registry_GetUser();
							int bShowSurface = (pUserReg) ? pUserReg->GetValuei("SHOWSURFACE") : 0;
							if (bShowSurface)
							{
								if (Info.m_pSurface)
									ConOut(CStrF("Surface: %s, Pos %s, iObject %d", Info.m_pSurface->m_Name.Str(), Info.m_Pos.GetString().Str(), Info.m_iObject ));
								else
									ConOut(CStrF("Surface: <NULL>, Pos %s, iObject %d", Info.m_Pos.GetString().Str(), Info.m_iObject ));
							}
#endif

							// Render laserdot for second gun (if any..)
							if (!bUnequip && !(pCD->m_AnimGraph2.GetStateFlagsLoCombined() & AG2_STATEFLAG_EQUIPPING)
								&& pCD->m_Item1_Flags && RPG_ITEM_FLAGS_LASERBEAM)
							{

							}
						}
					}
				}
			}
		}*/
	}


	if (!bZooming || !bLocalPlayer || bMirrored || bCutSceneView)
	{
		CXR_Skeleton* pSkel = pModel ? (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON) : NULL;
		// Render items
		if(!bLocalPlayer || !(pCD->m_ActionCutSceneCameraMode & CActionCutsceneCamera::ACS_CAMERAMODE_ACTIVE) && !(_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE))
		{
			if(pCD->m_Item0_Model.IsValid() && !(pCD->m_Item0_Flags & RPG_ITEM_FLAGS_NORENDERMODEL) && (pCD->m_Item0_Flags & RPG_ITEM_FLAGS_EQUIPPED) && !(pCD->m_AnimGraph2.GetStateFlagsLoCombined() & AG2_STATEFLAG_NOITEMRENDER))
			{
				pCD->m_Item0_Model.SetAG2I(pCD->m_WeaponAG2.GetAG2I());
				pCD->m_WeaponAG2.SetPropertyFloat(PROPERTYWEAPONAG_FLOAT_CONTROLANALOG,((fp32)pCD->m_Analog0) * (1.0f / 128.0f));
				if(pCD->m_Item0_Model.GetModel0_RenderInfo(_pWClient->GetMapData(), _pEngine, pSkelInstance, pSkel, CMTime::CreateFromTicks(pCD->m_GameTick, GameTickTime, IPTime), 
														lModelAnims[nModels], lModelPos[nModels], lpModels[nModels],_pWClient))
				{
					lModelAnims[nModels].m_AnimTime0 = CMTime::CreateFromSeconds(fp32(pCD->m_Item0_Num) + 0.001f);
					lModelAnims[nModels].m_Data[3] = lModelAnims[0].m_Data[3];
					lModelAnims[nModels].m_SurfaceOcclusionMask = pCD->m_Item0_Model.m_SurfaceOcclusionMask;
					lModelAnims[nModels].m_SurfaceShadowOcclusionMask = pCD->m_Item0_Model.m_SurfaceShadowOcclusionMask;

					pCD->m_Item0_Model.RenderExtraModels(_pWClient->GetMapData(), _pEngine, pSkelInstance, pSkel, lModelPos[nModels], CMTime::CreateFromTicks(pCD->m_GameTick, GameTickTime, IPTime), _pObj->m_iObject);

					nModels++;
				}
			}

			if(pCD->m_Item1_Model.IsValid() && !(pCD->m_Item1_Flags & RPG_ITEM_FLAGS_NORENDERMODEL) && (pCD->m_Item1_Flags & RPG_ITEM_FLAGS_EQUIPPED) && !(pCD->m_AnimGraph2.GetStateFlagsLoCombined() & AG2_STATEFLAG_NOITEMRENDERSECONDARY))
			{
				pCD->m_Item1_Model.SetAG2I(pCD->m_WeaponAG2.GetAG2I());
				pCD->m_WeaponAG2.SetPropertyFloat(PROPERTYWEAPONAG_FLOAT_CONTROLANALOG,((fp32)pCD->m_Analog1) * (1.0f / 128.0f));
				if(pCD->m_Item1_Model.GetModel0_RenderInfo(_pWClient->GetMapData(), _pEngine, pSkelInstance, pSkel, CMTime::CreateFromTicks(pCD->m_GameTick, GameTickTime, IPTime),
														lModelAnims[nModels], lModelPos[nModels], lpModels[nModels],_pWClient))
				{
					pCD->m_Item1_Model.RenderExtraModels(_pWClient->GetMapData(), _pEngine, pSkelInstance, pSkel, lModelPos[nModels], CMTime::CreateFromTicks(pCD->m_GameTick, GameTickTime, IPTime), _pObj->m_iObject);
					lModelAnims[nModels].m_Data[3] = lModelAnims[0].m_Data[3];
					lModelAnims[nModels].m_SurfaceOcclusionMask = pCD->m_Item1_Model.m_SurfaceOcclusionMask;
					lModelAnims[nModels].m_SurfaceShadowOcclusionMask = pCD->m_Item1_Model.m_SurfaceShadowOcclusionMask;
					nModels++;
				}
			}
		}

		if(pCD->m_Item2_Model.IsValid())
		{
			if(pCD->m_Item2_Model.GetModel0_RenderInfo(_pWClient->GetMapData(), _pEngine, pSkelInstance, pSkel, CMTime::CreateFromTicks(pCD->m_GameTick, GameTickTime, IPTime),
				lModelAnims[nModels], lModelPos[nModels], lpModels[nModels],_pWClient))
			{
				pCD->m_Item2_Model.RenderExtraModels(_pWClient->GetMapData(), _pEngine, pSkelInstance, pSkel, lModelPos[nModels], CMTime::CreateFromTicks(pCD->m_GameTick, GameTickTime, IPTime), _pObj->m_iObject);
				lModelAnims[nModels].m_Data[3] = lModelAnims[0].m_Data[3];
				lModelAnims[nModels].m_SurfaceOcclusionMask = pCD->m_Item2_Model.m_SurfaceOcclusionMask;
				lModelAnims[nModels].m_SurfaceShadowOcclusionMask = pCD->m_Item2_Model.m_SurfaceShadowOcclusionMask;
				nModels++;
			}
		}

		//Render drain model if active
		/* REMOVED, SAVED FOR REFERENCE...
		bool bDarknessDrainMode = ((pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DRAIN) != 0);
		if(bDarknessDrainMode || (pCD->m_DarknessDrainTime > 0.001f && pCD->m_DarknessDrainTime < 3.0f))
		{
			CXR_AnimState DarknessDrainAnimState;
			CMat4Dfp32 DrainMat;
			DrainMat.Unit();
			DrainMat.GetRow(3) = MatIP.GetRow(3) + CVec3Dfp32(0,0,8.0f);

			fp32 DrainTime = pCD->m_DarknessDrainTime + (IPTime * GameTickTime);
			if(bDarknessDrainMode)
				DrainTime = Clamp01(DrainTime);
			else
				DrainTime = Clamp(DrainTime, 2.0f, 3.0f);
			
			DarknessDrainAnimState.m_AnimTime0 = CMTime::CreateFromSeconds(DrainTime);
			CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(pCD->m_iDarkness_Drain_1);
			if (pModel)
				_pEngine->Render_AddModel(pModel, DrainMat, DarknessDrainAnimState);
		}
		*/

		// Render burning models
		pCD->m_Burnable.m_Value.OnClientRender(_pObj, _pWClient, MatIP, _pEngine, pSkel, pSkelInstance);

		// Render all models
		{
			int Flags = 0;
/*			if(bCutSceneView || bCutSceneViewOther)
				Flags |= CXR_MODEL_ONRENDERFLAGS_MAXLOD;*/
			if(bLocalPlayer)
				Flags |= CXR_MODEL_ONRENDERFLAGS_MAXLOD;
			if(_pObj->m_ClientFlags & CWO_CLIENTFLAGS_LINKINFINITE)
				Flags |= CXR_MODEL_ONRENDERFLAGS_NOCULL;

			if (!_pWClient->m_Render_CharacterShadows)
				Flags |= CXR_MODEL_ONRENDERFLAGS_NOSHADOWS;

			
			for(int i = 0; i < nModels; i++)
			{
				int NewFlags = Flags | GlobalOnRenderFlags | lModelOnRenderFlags[i];
				bool bRenderHead = true;
#ifdef WADDITIONALCAMERACONTROL
				if (i == 1 && bFirstPerson && _pObj->m_iModel[1] != 0 && !pCD->m_bAdditionalCameraControl)
					bRenderHead = false;
#else
				if (bFirstPerson || pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_NORENDERHEAD)
					bRenderHead = false;
#endif

				if (bLocalPlayer && bRenderHead && (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK) != 0 && !pCD->IsDarkling())
					lModelAnims[i].m_AnimTime1 = CMTime::CreateFromSeconds(1.0f);

				// don't render special hair-shadow polygons
				lModelAnims[i].m_SurfaceOcclusionMask = 4;
				lModelAnims[i].m_SurfaceShadowOcclusionMask = 4;

				if (bRenderHead == false)
				{
					lModelAnims[i].m_SurfaceOcclusionMask |= 2;			// don't render head
					lModelAnims[i].m_SurfaceShadowOcclusionMask &= ~4;	// render special hair-shadow polygons
				}

				// Check if this is not a darkling and not the player
				int nAnimSurface = 0;
				if (!bLocalPlayer && !pCD->IsDarkling())
				{
					// Fetch player client data
					CWObject_Character* pPlayer = (CWObject_Character*)_pWClient->Object_Get(_pWClient->Player_GetLocalObject());
					CWO_Character_ClientData* pPlayerCD = GetClientData(pPlayer);
					
					// Check if we want to apply a surface with user color
					if(pPlayerCD->m_CreepingLens > 0.001f)
						lModelAnims[i].m_lpSurfaces[nAnimSurface++] = _pEngine->m_pSC->GetSurface("creepdark01");
				}

				// Add scorching to burning characters
				if (!bLocalPlayer && pCD->m_Burnable.m_Value.IsBurned())
				{
					lModelAnims[i].m_lpSurfaces[nAnimSurface++] = _pEngine->m_pSC->GetSurface("burnscorch");
					lModelAnims[i].m_AnimTime1 = pCD->m_Burnable.m_Value.GetBurnTime(_pWClient);
				}

				switch(nAnimSurface)
				{
				case 2:
					NewFlags |= CXR_MODEL_ONRENDERFLAGS_SURF1_ADD;

				case 1:
					NewFlags |= CXR_MODEL_ONRENDERFLAGS_SURF0_ADD;
					break;
				}

				lModelAnims[i].m_ExcludeLightGUID = _pObj->m_iObject*5 + 0x4003;	// Exclude flash light if it's on
				lModelAnims[i].m_iObject = _pObj->m_iObject;
				lModelAnims[i].m_Variation = liVariations[i];
				if(pCD->m_DemonHeadTargetFade > -1)
				{
					switch(pCD->m_DemonHeadTargetFade)
					{
					case 0:
						lModelAnims[i].m_Data[0] = CPixel32(0,0,0,0);
						break;
					default:
						lModelAnims[i].m_Data[0] = CPixel32(255,255,255,255);
						lModelAnims[i].m_AnimTime0 = CMTime::CreateFromSeconds(pCD->m_DemonHeadCurrentFade - 1.0f);
						break;
/*					case 2:
						lModelAnims[i].m_Data[0] = CPixel32(255,255,255,255);
						lModelAnims[i].m_AnimTime0 = CMTime::CreateFromSeconds(1.0f);
						break;
					case 3:
						lModelAnims[i].m_Data[0] = CPixel32(255,255,255,255);
						lModelAnims[i].m_AnimTime0 = CMTime::CreateFromSeconds(2.0f);
						break;*/
					}
				}
				if(!_pEngine->Render_AddModel(lpModels[i], lModelPos[i], lModelAnims[i], XR_MODEL_STANDARD, NewFlags))
				{
//					int i = 0;
				}
			}
		}

		// Render drop-shadow
#if 0
		if(!(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_NOSHADOW) &&
		   (_pEngine->GetVCDepth() == 0))
		{
			CVec3Dfp32 ModelPosition;
			CVec3Dfp32 Base = pCD->m_BaseFightingPosition;
			//CVec3Dfp32 Target = pCD->m_TargetFightingPosition;

			if (!(Base == 0))
			{
				fp32 CurrTime = (((fp32)(pCD->m_GameTick - pCD->m_FightPosTime)) + pCD->m_PredictFrameFrac)
					* SERVER_TIMEPERFRAME * pCD->m_FightPosTimeScale;
				
				CVec3Dfp32 Target;
		
				if ((pCD->m_iFightingCharacter == -1) && (CurrTime >= 0) && (CurrTime <= 2.0f))
				{
					// Make target the actual position of the character if we are moving out of 
					// fighting mode (character might be running away for example...)
					Target = _pObj->GetPosition();
				}
				else
				{
					Target = GetTargetFightPos(_pObj, _pWClient, Clamp01(CurrTime));
				}

				if (CurrTime >= 1.0f)
				{
					ModelPosition = Target;
				}
				else if (CurrTime > 0.0f)
				{
					Base.Lerp(Target, CurrTime, ModelPosition);
				}
				else
				{
					// If time is less than zero
					ModelPosition = Base;
				}
			}
			else
			{
//				ModelPosition = CVec3Dfp32::GetRow(MatIP, 3);
				ModelPosition = AnimatedPos;
			}

			fp32 d = CVec3Dfp32::GetRow(_pEngine->GetVC()->m_CameraWMat, 3).Distance(ModelPosition);

			// Inte så jävla snyggt..
	/*		if ((_pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_DEAD) && 
				(lModelAnims[0].m_AnimTime0 > 5.0f*SERVER_TICKSPERSECOND))
				d = 2000;*/

			CTextureContainer_ShadowDecals* pTC = NULL;
			if (pCD->m_iPlayer != -1 && !(pObjPlayer->m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE)
				&& !bCutSceneViewOther)
			{
				pTC = safe_cast<CTextureContainer_ShadowDecals>(_pEngine->GetInterface(XR_ENGINE_TCSHADOWDECALS));
			}

			fp32 ShadowFade;
			if (pTC)
			{
				if (pCD->m_iPlayer == -1/*!bCutSceneView && !bCutSceneViewOther*/)
					ShadowFade = Clamp01((250.0f - d) * (1.0f / 50.0f) );
				else
					ShadowFade = 1.0f;
			}
			else
				ShadowFade = 0.0f;

			fp32 WallmarkFade = (1.0f - ShadowFade) * Clamp01((700.0f - d) * (1.0f / 100.0f) );
			
			if (pTC && (ShadowFade > 0.001f))
			{
				CXR_RenderInfo RenderInfo;
				RenderInfo.m_pCurrentEngine = _pEngine;
				if (_pEngine->View_GetClip_Box(_pObj->GetAbsBoundBox()->m_Min, _pObj->GetAbsBoundBox()->m_Max, 0, 0, NULL, &RenderInfo) &&
					RenderInfo.m_pLightVolume != NULL)
				{
					CVec3Dfp32 ShadowOrigin, BoundBoxDiff;
					_pObj->GetAbsBoundBox()->GetCenter(ShadowOrigin);
					BoundBoxDiff = ShadowOrigin - CVec3Dfp32::GetRow(MatIP, 3);
					ShadowOrigin = ModelPosition + BoundBoxDiff;

					CXR_LightGridPoint LGP;
					CVec3Dfp32 LDir, LColor;
					fp32 Bias = RenderInfo.m_pLightVolume->Light_EvalVertex(NULL, ShadowOrigin, LDir, LColor);
					vec128 LDirv=M_VLd_V3_Slow(&LDir);
					LDirv=M_VNrm3_Est(LDirv);
					LDirv=M_VAdd(LDirv,M_VConst(0,0,0.1f,0));
					LDirv=M_VNrm3(LDirv);

					CMat4Dfp32 WallMarkPos;
					WallMarkPos.r[2] = LDirv;
					WallMarkPos.r[0] = M_VConst(1.0f,0,0,0);
					WallMarkPos.RecreateMatrix<2,0>();
					WallMarkPos.r[3] = M_VLd_P3_Slow(&ShadowOrigin);

					MSCOPE(ShadowDecals, CHARACTER);

					fp32 WMSize = 72.0f;
					CMat4Dfp32 Camera = WallMarkPos;

#ifndef PLATFORM_DOLPHIN
					//	SS: Varför görs detta? Kopieras texturerna flippat i opengl/dx???
					CVec3Dfp32::GetRow(Camera, 2) = -CVec3Dfp32::GetRow(Camera, 2);
					CVec3Dfp32::GetRow(Camera, 1) = -CVec3Dfp32::GetRow(Camera, 1);
#endif
					
/*				CWireContainer* pWC = _pWClient->Debug_GetWireContainer();
				if (pWC)
				{
//					pWC->RenderMatrix(WallMarkPos, 0.05f);
					pWC->RenderOBB(WallMarkPos, CVec3Dfp32(WMSize*0.5f), 0xff000080, 0.05f);
				}*/

					int Update = 1;
					if (pCD->m_iPlayer == -1) 
					{
	/*					if (d > 225.0f)
						else*/
							Update = 2;
					}

					int hShadow = pTC->Shadow_Begin(Camera, WMSize, _pObj->m_iObject, Update);
					if (hShadow)
					{
						lpModels[0] = pShadowModel;
						for(int i = 0; i < nModels; i++)
							pTC->Shadow_AddModel(hShadow, lpModels[i], lModelPos[i], lModelAnims[i]);
						pTC->Shadow_End(hShadow);
						int TextureID = pTC->Shadow_GetTextureID(hShadow);

						CXR_WallmarkDesc WMD = CreateWallMarkDesc(XR_WALLMARK_FADECOLOR, WMSize, "S_SHADOWDECAL", TextureID);
						WMD.m_Time = 1.0f - ShadowFade;
						CreateTempWallmark(_pWClient, WMD, WallMarkPos, WMSize);
					}
				}
			}

			if (WallmarkFade > 0.001f)
			{
				CMat4Dfp32 WallMarkPos;
				WallMarkPos.Unit();
				CVec3Dfp32::GetRow(WallMarkPos, 3) = ModelPosition;

				CXR_WallmarkDesc WMD = CreateWallMarkDesc(0, 40, "S_DROPSHADOW", 0);
				WMD.m_Time = 1.0f - WallmarkFade;
				CreateTempWallmark(_pWClient, WMD, WallMarkPos, 40);
			}
		}
#endif
	}
}

void CWObject_Character::OnClientRenderTargetHud(int _X, int _Y, CRC_Util2D *_pUtil2D, fp32 _Fade, fp32 _HealthFrac, const char *_pName, CRC_Font *_pFont)
{
	MAUTOSTRIP(CWObject_Character_OnClientRenderTargetHud_2, MAUTOSTRIP_VOID);
/*	char *Background = "GUI_HudEnemyBar01_01";
	char *HealthBar = "GUI_HudEnemyHealth01_01";
	
	// Screen resolution should be requested from somewhere else.
	CClipRect Clip(0, 0, 640, 480);
	
	// Absolute offset of the whole hud (to which the parts are relatively offsetted).
	int32 Alpha = (int32(_Fade * 255.0f) << 24);
	int32 Color = 0x00808080 | Alpha;
	
	{
		_pUtil2D->SetSurface(Background, 0);
		CVec2Dfp32 Size(_pUtil2D->GetTextureWidth(), _pUtil2D->GetTextureHeight());
		CVec2Dfp32 TopLeft(_X, _Y);
		_pUtil2D->SetTextureOrigo(Clip, CPnt(TopLeft[0], TopLeft[1]));
		_pUtil2D->Rect(Clip, CRct(TopLeft[0], TopLeft[1], TopLeft[0] + Size[0], TopLeft[1] + Size[1]), Color);
	}
	
	{
		_pUtil2D->SetSurface(HealthBar, 0);
		CVec2Dfp32 Size(116, 8);
		CVec2Dfp32 TopLeft(_X + 35, _Y + 30);
		_pUtil2D->SetTextureOrigo(Clip, CPnt(TopLeft[0], TopLeft[1]));
		_pUtil2D->Rect(Clip, CRct(TopLeft[0], TopLeft[1], TopLeft[0] + Size[0] * _HealthFrac, TopLeft[1] + Size[1]), Color);
	}
	
	if(_pFont && _pName)
	{
		int32 ColorDark = 0x00202020 | Alpha;
		int32 ColorLight = 0x00808080 | Alpha;
		wchar Buffer[1024];
		Localize_Str(_pName, Buffer, 1024);
		
		CVec2Dfp32 Size(_pFont->GetWidth(_pFont->GetOriginalSize(), Buffer), _pFont->GetHeight(_pFont->GetOriginalSize(), Buffer));
		CVec2Dfp32 TopLeft(_X + 13, _Y - 12);

		{
			wchar Lines[2][512];
			const int MaxLines = 15;
			wchar* lpLines[2];
			for(int i = 0; i < 2; i++)
				lpLines[i] = &Lines[i][0];
			
			int Len = CStrBase::StrLen(Buffer);
			int nLines = _pUtil2D->Text_WordWrap(_pFont, 160, (wchar*)Buffer, Len, lpLines, 2);
			if(nLines == 1)
				TopLeft[1] += 17;
		}
		
		int Style = WSTYLE_TEXT_WORDWRAP | WSTYLE_TEXT_CENTER | WSTYLE_TEXT_SHADOW;
		_pUtil2D->Text_DrawFormatted(Clip, _pFont, _pName, TopLeft[0], TopLeft[1], Style, ColorLight, ColorLight, ColorDark, 160, 60, false);
	}*/
}
#ifdef WDEBUGCONTROL_ENABLED
void ClientRenderControlDebug(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D)
{
	CWO_Character_ClientData *pCD = CWObject_Character::GetClientData(_pObj);
	if(!pCD)
		return;

	CClipRect Clip(0, 0, 640, 480);

	// Movement
	CVec2Dfp32 Mid(440,60);
	CVec2Dfp32 Move;
	Move.k[0] = -pCD->m_DebugControlMoveY;
	Move.k[1] = -pCD->m_DebugControlMoveX;
	CVec2Dfp32 P2 = Mid + Move * 50.0f;
	//_pUtil2D->Circle(Clip,Mid,50,20,CPixel32(30,30,30,170));
	CRct Rect;
	Rect.p0.x = (int)(Mid.k[0] - 50);
	Rect.p0.y = (int)(Mid.k[1] - 50);
	Rect.p1.x = (int)(Mid.k[0] + 50);
	Rect.p1.y = (int)(Mid.k[1] + 50);
	_pUtil2D->Rect(Clip,Rect,CPixel32(30,30,30,170));
	_pUtil2D->Line(Clip,Mid,P2,2.0f,CPixel32(255,255,255,255));

	// Look
	Mid = CVec2Dfp32(570,60);
	_pUtil2D->Circle(Clip,Mid,50,20,CPixel32(30,30,30,170));
	Move[0] = M_Sin((-pCD->m_DebugControlLookH - 0.5f)*_PI2);
	Move[1] = M_Cos((-pCD->m_DebugControlLookH - 0.5f)*_PI2);
	P2 = Mid + Move * 50.0f;
	_pUtil2D->Line(Clip,Mid,P2,2.0f,CPixel32(255,255,255,255));
	Move[0] = M_Sin((pCD->m_DebugControlLookV + 0.25f)*_PI2);
	Move[1] = M_Cos((pCD->m_DebugControlLookV + 0.25f)*_PI2);
	P2 = Mid + Move * 50.0f;
	_pUtil2D->Line(Clip,Mid,P2,2.0f,CPixel32(255,0,255,255));

	// dLook
	Move[0] = pCD->m_DebugControlDLookH;//M_Sin((-pCD->m_DebugControlDLookH - 0.5f)*_PI2);
	Move[1] = pCD->m_DebugControlDLookV;//M_Cos((-pCD->m_DebugControlDLookH - 0.5f)*_PI2);
	P2 = Mid + Move * 200.0f;
	_pUtil2D->Line(Clip,Mid,P2,2.0f,CPixel32(255,255,0,255));

	// Buttons
	for (int32 i = 0; i < 8; i++)
	{
		Rect.p0.x = 440 + i * 15;
		Rect.p0.y = 130;
		Rect.p1.x = 450 + i * 15;
		Rect.p1.y = 140;
		bool bOn = (pCD->m_DebugControlPress & (1 << i)) != 0;
		CPixel32 Color = (bOn ? CPixel32(0,255,0,255) : CPixel32(255,0,0,255));
		_pUtil2D->Rect(Clip,Rect,Color);
	}
}
#endif
#ifndef PLATFORM_CONSOLE
#ifndef M_RTM
void RenderAG2IHud(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D);
#endif
#endif
void CWObject_Character::OnClientRenderStatusBar(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D)
{
	MAUTOSTRIP(CWObject_Character_OnClientRenderStatusBar, MAUTOSTRIP_VOID);
	MSCOPESHORT(CWObject_Character::OnClientRenderStatusBar);
	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if(!pCD)
		return;

#ifdef WDEBUGCONTROL_ENABLED
	if (pCD->m_iDebugControlObject != 0)
		ClientRenderControlDebug(_pObj,_pWClient,_pEngine,_pUtil2D);
#endif
#ifndef PLATFORM_CONSOLE
#ifndef M_RTM
	CWObject_Client* pDebugObj = _pObj;
#ifdef WDEBUGCONTROL_ENABLED
	if (pCD->m_iDebugControlObject != 0)
	{
		CWObject_Client* pDObj = _pWClient->Object_Get(pCD->m_iDebugControlObject);
		if (pDObj)
			pDebugObj = pDObj;
	}
#endif
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	int32 AGIDebugFlags = 0;
	CRegistry* pReg = pSys ? pSys->GetEnvironment() : NULL;
	if (pReg != NULL)
		AGIDebugFlags = pReg->GetValuei("AG2I_DEBUG_HUD");

	if (AGIDebugFlags != 0)
		RenderAG2IHud(pDebugObj, _pWClient, _pEngine, _pUtil2D);
#endif
#endif
	
	CRenderContext* pContext = _pUtil2D->GetRC();
	CRC_Viewport* pViewPort = pContext->Viewport_Get();
	_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	#if defined(M_RTM)
	{
		//MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		//if(pSys->GetEnvironment()->GetValuei("CON_LOGOS", 0))
			//_pUtil2D->DrawTexture(Clip, CPnt(640 - 64 - 20, 20), "GUI_SBZStamp", 0xffffffff, CVec2Dfp32(2.0f, 2.0f));
	}
	#endif

	// "Activation" icons (from mission items for instance), temp render
	CClipRect Clip(0, 0, 640, 480);
	CVec2Dfp32 OldCoord = _pUtil2D->GetCoordinateScale();
	CPnt ScreenSize = _pUtil2D->GetRC()->GetDC()->GetScreenSize();
	_pUtil2D->SetCoordinateScale(CVec2Dfp32(ScreenSize.x / 640.0f, ScreenSize.y / 480.0f));

	fp32 Aspect = _pUtil2D->GetRC()->GetDC()->GetScreenAspect() * _pUtil2D->GetRC()->GetDC()->GetPixelAspect();
	int32 iLastTick = pCD->m_GameTick - ((int32)M_Ceil(_pWClient->GetGameTicksPerSecond())) * 4;
	int32 iCurrentItem = 0;
	int32 PickupTick = 0;
	fp32 NumItems = 0.0f;
	int16 iSurface = pCD->m_InventoryInfo.GetNextActivationSurface(iCurrentItem,iLastTick,PickupTick);
	fp32 IconSize = 38.4f;
	while (iSurface)
	{
#ifdef PLATFORM_CONSOLE
		CVec2Dfp32 TopLeft(Clip.clip.p1.x - IconSize - (640.0f*0.075f), /*Clip.clip.p1.y -*/ 35.0f + NumItems * (IconSize + 4.0f));
#else
		CVec2Dfp32 TopLeft(Clip.clip.p1.x - 60.0f,  /*Clip.clip.p1.y -*/ 35.0f + NumItems * (IconSize + 4.0f));
#endif
		int iSurf = iSurface;
		CMTime Time = PHYSSTATE_TICKS_TO_TIME(pCD->m_GameTick - PickupTick, _pWClient);
		fp32 Duration = 4.0f;
		if(iSurf & 0x8000)
		{
			iSurf &= 0x7fff;
			TopLeft[1] += 50;
		}
		CXW_Surface *pSurface = _pWClient->GetMapData()->GetResource_Surface(iSurf);
		if (pSurface)
		{
			_pUtil2D->SetSurface(pSurface, CMTime());
			//CVec2Dfp32 Size(_pUtil2D->GetTextureWidth(), _pUtil2D->GetTextureHeight());
			CVec2Dfp32 Size(IconSize, IconSize);
			_pUtil2D->SetTextureScale(Aspect * fp32(_pUtil2D->GetTextureWidth()) / Size[0] , fp32(_pUtil2D->GetTextureHeight()) / Size[1]);
			_pUtil2D->SetTextureOrigo(Clip, CPnt((int)M_Ceil(TopLeft[0]), (int)M_Ceil(TopLeft[1])));
			int Color = (int(255 - (255 * Time.GetTime() / Duration)) << 24) | 0xffffff;
			_pUtil2D->Rect(Clip, CRct((int)M_Ceil(TopLeft[0]) , (int)M_Ceil(TopLeft[1]), (int)M_Ceil(TopLeft[0] + Size[0]/Aspect) , (int)M_Ceil(TopLeft[1] + Size[1])), Color);
		}
		iSurface = pCD->m_InventoryInfo.GetNextActivationSurface(iCurrentItem,iLastTick,PickupTick);
		NumItems += 1.0f;
	}
	_pUtil2D->SetCoordinateScale(OldCoord);
	// Pickup icon
  	/*if(pCD->m_Pickup_iIcon != 0)
	{
		//Duration shown is hardcoded here when picking up normally or based on selection timeout duration if selecting
		CMTime Time = PHYSSTATE_TICKS_TO_TIME(pCD->m_GameTick - pCD->m_Pickup_StartTick, _pWClient);
		const fp32 Duration = (pCD->m_SelectItemTimeOutTick > pCD->m_GameTick) ? PLAYER_WEAPONSELECT_DURATION : 4.0f;
		if(Time.Compare(Duration) < 0)
		{
   			CVec2Dfp32 TopLeft(540, 200);
	 		int iSurf = pCD->m_Pickup_iIcon;
			if(iSurf & 0x8000)
			{
				iSurf &= 0x7fff;
				TopLeft[1] += 50;
			}
			CXW_Surface *pSurface = _pWClient->GetMapData()->GetResource_Surface(iSurf);
	 		if (pSurface)
			{
				_pUtil2D->SetSurface(pSurface, CMTime());
				//CVec2Dfp32 Size(_pUtil2D->GetTextureWidth(), _pUtil2D->GetTextureHeight());
				CVec2Dfp32 Size(64, 64);
				_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / Size[0], fp32(_pUtil2D->GetTextureHeight()) / Size[1]);
				_pUtil2D->SetTextureOrigo(Clip, CPnt(TopLeft[0], TopLeft[1]));
				int Color = (int(255 - (255 * Time.GetTime() / Duration)) << 24) | 0xffffff;
				_pUtil2D->Rect(Clip, CRct(TopLeft[0], TopLeft[1], TopLeft[0] + Size[0], TopLeft[1] + Size[1]), Color);
			}

			//Render any additional magazine clips (if weapon uses ammo)
			if (pCD->m_Pickup_Magazine_Num.m_Value != -1)
			{
				CRC_Font* pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
				if(pFont)
				{
					CVec2Dfp32 TopLeftMag(550, 240);
					int Color = (int(255 - (255 * Time.GetTime() / Duration)) << 24) | 0xffffff;
					int ColorDark = (int(255 - (255 * Time.GetTime() / Duration)) << 24) | 0x000000;
					CStr St = CStrF("§Z22%i", pCD->m_Pickup_Magazine_Num.m_Value);
					_pUtil2D->Text_DrawFormatted(Clip, pFont, St, TopLeftMag[0] - 22, TopLeftMag[1] + 18, 0, ColorDark, ColorDark, ColorDark, 40, 40, true);
					_pUtil2D->Text_DrawFormatted(Clip, pFont, St, TopLeftMag[0] - 22, TopLeftMag[1] + 18, 0, Color, Color, Color, 40, 40, false);
				}

			}
		}
	}*/

	OnClientRenderHurtEffectAndOverlaySurfaces(_pObj, _pWClient, _pEngine, _pUtil2D);

	bool bCutSceneView = (_pObj->m_ClientFlags & (PLAYER_CLIENTFLAGS_CUTSCENE | PLAYER_CLIENTFLAGS_DIALOGUE)) != 0;
	int PhysType = Char_GetPhysType(_pObj);
	if (bCutSceneView || (PhysType != PLAYER_PHYS_STAND && PhysType != PLAYER_PHYS_CROUCH))
		return;

	OnClientRenderHealthHud(_pObj, _pWClient, _pEngine, _pUtil2D);
	OnClientRenderChoices(_pObj, _pWClient, _pEngine, _pUtil2D);

	//Temp removed for x06 and hopefully forever
//	OnClientRenderDarklingsChoices(_pObj, _pWClient, _pEngine, _pUtil2D);
}


void CWObject_Character::OnClientRenderDarklingsChoices(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D)
{
	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if(!pCD)
		return;

	int StartFadeTick = pCD->m_DarklingSpawnRenderChoicesStartTick + TruncToInt(4.0f * _pWClient->GetGameTicksPerSecond());
	if(!pCD->m_DarklingSpawnRenderChoicesStartTick || !pCD->m_DarklingSpawnRenderChoices || StartFadeTick > _pWClient->GetGameTick())
		return;

	CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("HEADINGS"));
	if(!pFont)
		return;

	int Alpha = 255;
	int EndFadeTick = pCD->m_DarklingSpawnRenderChoicesStartTick + TruncToInt(5.0f * _pWClient->GetGameTicksPerSecond());
	if(EndFadeTick > _pWClient->GetGameTick())
		Alpha = RoundToInt((fp32(_pWClient->GetGameTick() - StartFadeTick + 1) / fp32(EndFadeTick - StartFadeTick)) * 255.0f);

	CClipRect Clip(0, 0, 640, 480);
	CPixel32 Color(255,255,255,Alpha);

	CVec2Dfp32 OldCoord = _pUtil2D->GetCoordinateScale();
	CPnt ScreenSize = _pUtil2D->GetRC()->GetDC()->GetScreenSize();
	_pUtil2D->SetCoordinateScale(CVec2Dfp32(ScreenSize.x / 640.0f, ScreenSize.y / 480.0f));

	const int Size = 38;

	fp32 Aspect = _pUtil2D->GetRC()->GetDC()->GetScreenAspect() * _pUtil2D->GetRC()->GetDC()->GetPixelAspect();
	int CenterX = 320;
	int CenterY = 140;
	if(pCD->m_DarklingSpawnRenderChoices & CWObject_DarklingSpawn::DARKLINGSPAWN_FLAGS2_LIGHTKILLER)
	{
		_pUtil2D->SetSurface("GUI_HUD_darkness", pCD->m_GameTime);
		_pUtil2D->SetTextureOrigo(Clip, CPnt((int)(CenterX - Size / Aspect / 2), CenterY - Size / 2));
		_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / Size * Aspect, fp32(_pUtil2D->GetTextureHeight()) / Size);
		_pUtil2D->Rect(Clip, CRct((int)(CenterX - Size / 2 / Aspect), CenterY - Size / 2, (int)(CenterX + Size / 2 / Aspect), CenterY + Size / 2 ), Color);
		_pUtil2D->Text(Clip, pFont, 290, 160, "Lightkiller", Color, 12.0f);
	}

	if(pCD->m_DarklingSpawnRenderChoices & CWObject_DarklingSpawn::DARKLINGSPAWN_FLAGS2_KAMIKAZE)
	{
		CenterX = 420;
		CenterY = 240;
		_pUtil2D->SetSurface("GUI_HUD_blackhole", pCD->m_GameTime);
		_pUtil2D->SetTextureOrigo(Clip, CPnt((int)(CenterX - Size / Aspect / 2), CenterY - Size / 2));
		_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / Size * Aspect, fp32(_pUtil2D->GetTextureHeight()) / Size);
		_pUtil2D->Rect(Clip, CRct((int)(CenterX - Size / 2 / Aspect), CenterY - Size / 2, (int)(CenterX + Size / 2 / Aspect), CenterY + Size / 2 ), Color);
		_pUtil2D->Text(Clip, pFont, 380, 260, "Kamikaze", Color, 12.0f);
	}

	if(pCD->m_DarklingSpawnRenderChoices & CWObject_DarklingSpawn::DARKLINGSPAWN_FLAGS2_BERSERKER)
	{
		CenterX = 320;
		CenterY = 340;
		_pUtil2D->SetSurface("GUI_HUD_demonarm", pCD->m_GameTime);
		_pUtil2D->SetTextureOrigo(Clip, CPnt((int)(CenterX - Size / Aspect / 2), CenterY - Size / 2));
		_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / Size * Aspect, fp32(_pUtil2D->GetTextureHeight()) / Size);
		_pUtil2D->Rect(Clip, CRct((int)(CenterX - Size / 2 / Aspect), CenterY - Size / 2, (int)(CenterX + Size / 2 / Aspect), CenterY + Size / 2 ), Color);
		_pUtil2D->Text(Clip, pFont, 290, 365, "Berserker", Color, 12.0f);
	}

	if(pCD->m_DarklingSpawnRenderChoices & CWObject_DarklingSpawn::DARKLINGSPAWN_FLAGS2_GUNNER)
	{
		CenterX = 220;
		CenterY = 240;
		_pUtil2D->SetSurface("GUI_HUD_creepingdark", pCD->m_GameTime);
		_pUtil2D->SetTextureOrigo(Clip, CPnt((int)(CenterX - Size / Aspect / 2), CenterY - Size / 2));
		_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / Size * Aspect, fp32(_pUtil2D->GetTextureHeight()) / Size);
		_pUtil2D->Rect(Clip, CRct((int)(CenterX - Size / 2 / Aspect), CenterY - Size / 2, (int)(CenterX + Size / 2 / Aspect), CenterY + Size / 2 ), Color);
		_pUtil2D->Text(Clip, pFont, 195, 265, "Gunner", Color, 12.0f);
	}

	_pUtil2D->SetCoordinateScale(OldCoord);
}

void CWObject_Character::OnClientRenderChoices(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D)
{
	CWO_Character_ClientData *pCD = GetClientData(_pObj);
	if(!pCD)
		return;

	if(pCD->m_nChoices == 0)
		return;

	CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("HEADINGS"));
	if(!pFont)
		return;

	CClipRect Clip(0, 0, 640, 480);

	CPixel32 PixColor(255, 255, 255, 255);
	CPixel32 PixColorGrayedOut(96, 96, 96, 255);
	CPixel32 PixColorBlack(0, 0, 0, 255);
	CPixel32 PixColorBlackOutline(0, 0, 0, 105);
	CPixel32 PixColorTextBox(255, 255, 255, 64);

#ifdef PLATFORM_CONSOLE
	int Border = 96;
#else
	int Border = 60;
#endif
 
	int XStart = Border;
	const int ChoiceHeight = 20;
	int YStart = 480 - (Border / 2) - ChoiceHeight * 3 -  12;
	int YStartArrows = YStart;

	_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	CRct FlashRect;
	FlashRect.p0.x = XStart - 6;
	FlashRect.p0.y = YStart + ChoiceHeight - 4;
	//FlashRect.p1.x = XStart + _pUtil2D->TextWidth(pFont, Text, (FontSize / pFont->GetOriginalSize())) + 6; // comes later when textwidth is known
	FlashRect.p1.y = YStart + ChoiceHeight - 2 + ChoiceHeight;

	//------------------------------------------------------
	// update timer for scroll up/down smoothness

	CMTime Time = CMTime::GetCPU();
	fp32 CurTime = Time.GetTime();
	fp32 DeltaTime = CurTime - pCD->m_LastTime;

	if(pCD->m_ScrollDownTimer != 0.0f)
	{
		pCD->m_ScrollDownTimer += DeltaTime;
		if(pCD->m_ScrollDownTimer > SCROLLTIME)
			pCD->m_ScrollDownTimer = 0.0f;
	}

	if(pCD->m_ScrollUpTimer)
	{
		pCD->m_ScrollUpTimer += DeltaTime;
		if(pCD->m_ScrollUpTimer > SCROLLTIME)
			pCD->m_ScrollUpTimer = 0.0f;
	}

	pCD->m_LastTime = CurTime;

	if(pCD->m_ScrollDownTimer != 0.0f)
		YStart += TruncToInt((1.0f - (pCD->m_ScrollDownTimer / SCROLLTIME)) * ChoiceHeight);
	else if(pCD->m_ScrollUpTimer != 0.0f)
		YStart -= TruncToInt((1.0f - (pCD->m_ScrollUpTimer / SCROLLTIME)) * ChoiceHeight);

	//--------------------------------------------------------

	CStr lChoice[3];

	CStr Choice = pCD->m_Choices.Ansi();
	int i;
	for(i = 0; i < pCD->m_iCurChoice - 1; i++)
		Choice.GetStrSep(";");

	//-----------------------------------------------------------
	// grey out used lines, and chose the first ungray line
	
	bool bGreyOut[3];
	bGreyOut[0] = false;
	bGreyOut[1] = false;
	bGreyOut[2] = false;
	
	// Make sure we have a character
	CWObject_Client *pFocusFrameObj = _pWClient->Object_Get(pCD->m_iFocusFrameObject);
	CWObject_Character* pChar = (pFocusFrameObj) ? CWObject_Character::IsCharacter(pFocusFrameObj) : NULL;
	if (pChar)
	{
		// Get character clientdata
		CWO_Character_ClientData* pCharCD = GetClientData(pFocusFrameObj);
		if(pCharCD)
		{
			uint32 ItemHash;
			uint32 UsedItemHash;
			int32 iCharDlgChoicesLen = pCharCD->m_liDialogueChoice.Len();

			for(i = 0; i < pCD->m_lUsedChoices.Len(); i++)
			{
				UsedItemHash = pCD->m_lUsedChoices[i];

				if(pCD->m_iCurChoice > 0 && iCharDlgChoicesLen)
				{
					ItemHash = pCharCD->m_liDialogueChoice[pCD->m_iCurChoice-1].m_ItemHash;
					if(ItemHash == UsedItemHash)
						bGreyOut[0] = true;
				}
				
				if(iCharDlgChoicesLen > pCD->m_iCurChoice)
				{
					ItemHash = pCharCD->m_liDialogueChoice[pCD->m_iCurChoice].m_ItemHash;
					if(ItemHash == UsedItemHash)
						bGreyOut[1] = true;
				}
				
								
				if((pCD->m_iCurChoice+1) <  pCD->m_nChoices && iCharDlgChoicesLen > pCD->m_iCurChoice+1)
				{
					ItemHash = pCharCD->m_liDialogueChoice[pCD->m_iCurChoice+1].m_ItemHash;
					if(ItemHash == UsedItemHash)
						bGreyOut[2] = true;
				}
			}
		}
	}

	if(pCD->m_iCurChoice > 0)
		lChoice[0] = Choice.GetStrSep(";");
	lChoice[1] = Choice.GetStrSep(";");
	lChoice[2] = Choice.GetStrSep(";");

	fp32 FontSize = 15.0f; 
	for(int i = 0; i < 3; i++)
	{

		CPixel32 UseTextColor;
		if(bGreyOut[i])
			UseTextColor = PixColorGrayedOut;
		else
			UseTextColor = PixColor;
		CStr St = lChoice[i];
		
		if(St != "")
		{
			CStr Text;
			if(St.Find("PHONENUMBER") != -1)
				Text = Localize_Str(St);
			else 
				Text = St;
			
			if(i == 1)
			{
				FlashRect.p1.x = XStart + _pUtil2D->TextWidth(pFont, Text, (FontSize / pFont->GetOriginalSize())) + 6; 
				_pUtil2D->Rect(Clip, FlashRect, PixColorTextBox);
				_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, XStart, YStart + i * ChoiceHeight, 0, PixColorBlackOutline, PixColorBlackOutline, PixColorBlackOutline, Clip.GetWidth()-Border, Clip.GetHeight(), true, 0, (FontSize / pFont->GetOriginalSize()));
				_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, XStart, YStart + i * ChoiceHeight, 0, UseTextColor, UseTextColor, PixColorBlack, Clip.GetWidth()-Border, Clip.GetHeight(), false, 0, (FontSize / pFont->GetOriginalSize()));
			}
			else
			{
				_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, XStart, YStart + i * ChoiceHeight, 0, PixColorBlackOutline, PixColorBlackOutline, PixColorBlackOutline, Clip.GetWidth()-Border, Clip.GetHeight(), true, 0, (FontSize / pFont->GetOriginalSize()));
				_pUtil2D->Text_DrawFormatted(Clip, pFont, Text, XStart, YStart + i * ChoiceHeight, 0, UseTextColor, UseTextColor, PixColorBlack, Clip.GetWidth()-Border, Clip.GetHeight(), false, 0, (FontSize / pFont->GetOriginalSize()));
			}
		}
	}

	// render the up/down arrows
	YStart = YStartArrows;
	bool bPressed;
	int TextureID = 0;
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	CXR_VBManager *pVBM = _pUtil2D->GetVBM();
	CRC_Viewport* pViewport = pVBM->Viewport_Get();
	CMat4Dfp32 Mat2D;
	pViewport->Get2DMatrix_RelBackPlane(Mat2D);
	CVec2Dfp32 CoordScale = _pUtil2D->GetCoordinateScale();
	
	fp32 PictureWidthBase	 =  12;
	fp32 PicSize  = PictureWidthBase * CoordScale.k[0];

	CRect2Duint16 PictureScreenCoord;
	CPnt ScrollButtonPointBase;
	ScrollButtonPointBase.x = 140;
	ScrollButtonPointBase.y = YStart -  TruncToInt(FontSize) - 6;

	for(int i = 0; i < 2; i++)
	{	
		if(i == 0 && (pCD->m_iCurChoice == 0))
		{
			ScrollButtonPointBase.y += (3 * ChoiceHeight ) + TruncToInt(PicSize) + 6;
			continue;
		}
		else if(i == 1 && pCD->m_iCurChoice == pCD->m_nChoices-1)
		{
			continue;
		}
			
		bPressed = false;
		if(((pCD->m_ScrollDownTimer != 0.0f) && i) || ((pCD->m_ScrollUpTimer != 0.0f) && !i))
		{
			bPressed = true;
		}

		if(i)
		{
			if(bPressed)
				TextureID = pTC->GetTextureID("GUI_Arrow_Down_pressed");
			else
				TextureID = pTC->GetTextureID("GUI_Arrow_Down");
		}
		else
		{
			if(bPressed)
				TextureID = pTC->GetTextureID("GUI_Arrow_Up_pressed");
			else
				TextureID = pTC->GetTextureID("GUI_Arrow_Up");
		}

		bPressed = false;

		if(TextureID)
		{
			PictureScreenCoord.m_Min = CVec2Duint16(TruncToInt(ScrollButtonPointBase.x * CoordScale.k[0]), TruncToInt(ScrollButtonPointBase.y * CoordScale.k[1]));
			PictureScreenCoord.m_Max = CVec2Duint16(PictureScreenCoord.m_Min.k[0] + TruncToInt(PicSize), PictureScreenCoord.m_Min.k[1] + TruncToInt(PicSize));

			// for flip or no-flip UV cords
			CVec2Dfp32 UVMin(0.0f, 0.0f);
			CVec2Dfp32 UVMax(1.0f, 1.0f);

			CVec2Dfp32* pTV;
			pTV = CXR_Util::VBM_CreateRectUV(pVBM, UVMin, UVMax);

			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			pA->SetDefault();
			pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
			pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
			pA->Attrib_TextureID(0, TextureID);

			CXR_VertexBuffer* pVBScreenQuad = CXR_Util::VBM_RenderRect(pVBM, &Mat2D, PictureScreenCoord, PixColor, 0.1f, pA);
			pVBScreenQuad->Geometry_TVertexArray(pTV, 0);
			pVBM->AddVB(pVBScreenQuad);
		}

		// second scrollbutton
		ScrollButtonPointBase.y += (3 * ChoiceHeight ) + TruncToInt(PicSize) + 6;
	}
}

//
//
//
CMat4Dfp32 CreateRotationMatrix(fp32 x, fp32 y, fp32 z)
{
	CMat4Dfp32 mX, mY, mZ, mTemp, mRes;
	mX.SetXRotation(x/_PI2);
	mY.SetYRotation(y/_PI2);
	mZ.SetZRotation(z/_PI2);
	mX.Multiply(mY, mTemp);
	mTemp.Multiply(mZ, mRes);
	return mRes;
}
/*
CMat4Dfp32 operator *(const CMat4Dfp32 &_b, const CMat4Dfp32 &_a)
{
	CMat4Dfp32 Res;
	_a.Multiply(_b, Res);
	return Res;
}*/

static char* s_DarknessGUISurf[PLAYER_DARKNESSMODE_SELECTION_NUMPOWERS] = 
{
	"GUI_HUD_creepingdark",
	"GUI_HUD_demonarm",
	"GUI_HUD_ancient",
	"GUI_HUD_blackhole",
};
#ifndef PLATFORM_CONSOLE
#ifndef M_RTM
void RenderAG2IHud(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D)
{
	bool bLocalPlayer = (_pObj->m_iObject == _pWClient->Player_GetLocalObject());
	if (bLocalPlayer)
	{
		CWC_Player* pP = _pWClient->Player_GetLocal();
		if (!pP) return;

		if (_pObj->GetNext() != NULL)
		{
			_pObj = CWObject_Character::Player_GetLastValidPrediction(_pObj);
		}
	}
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
	if (!pCD)
		return;
	_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	_pUtil2D->SetTexture(0);
	CClipRect Clip(0, 0, 640, 480);
	CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
	if (!pFont)
		return;

	CWAG2I* pAGI = pCD->m_AnimGraph2.GetAG2I();
	fp32 Fade = 1.0f;
	int32 StartX = 50;
	int32 StartY = 80;
	int32 OffsetY = 15;
	int32 OffsetX = 20;
	int32 iRow = 0;
	// Meh, get animlayers...
	CXR_AnimLayer Layers[AG2I_MAXANIMLAYERS];
	int nLayers = AG2I_MAXANIMLAYERS;
	CWAG2I* pAG2I = pCD->m_AnimGraph2.m_spAG2I;
	CWAG2I_Context AG2IContext(_pObj, _pWClient, pCD->m_GameTime);
	pAG2I->GetAnimLayers(&AG2IContext, Layers, nLayers, 0);

	int32 Color = CPixel32((255), (255), (255), (int)(Fade * 0.80f * 255.0f));
	for (int32 i = 0; i < pAGI->GetNumTokens(); i++)
	{
		const CWAG2I_Token* pToken = pAGI->GetToken(i);
		CFStr TokenInfo = CFStrF("iD: %d NumStates: %d",pToken->GetID(),pToken->GetNumStateInstances());
		_pUtil2D->Text(Clip,pFont,StartX,StartY + iRow * OffsetY, TokenInfo.Str(), Color,10.0f);
		iRow++;
		for (int32 j = 0; j < pToken->GetNumStateInstances(); j++)
		{
			const CWAG2I_StateInstance* pInst = pToken->GetStateInstance(j);
			const CXRAG2* pXRAG = pAGI->GetAnimGraph(pInst->GetAnimGraphIndex());
			CStr StateName = pXRAG->GetExportedStateName(pInst->GetStateIndex());
			CStr InstInfo = CStrF("State: %d(%s) iMT: %d, Time: %f ",pInst->GetStateIndex(),StateName.Str(),pInst->GetEnterMoveTokenIndex(),(pCD->m_GameTime - pInst->GetEnterTime()).GetTime());
			_pUtil2D->Text(Clip,pFont,StartX+OffsetX ,StartY + iRow * OffsetY,InstInfo,Color,10.0f);
			iRow++;
		}
	}
	// Layers
	_pUtil2D->Text(Clip,pFont,StartX,StartY + iRow * OffsetY,CStr("Layers:"),Color,10.0f);
	iRow++;
	for (int32 i = 0; i < nLayers; i++)
	{
		CStr Info = CStrF("Anim: %s BaseJoint: %d Time: %f  Blend: %f",Layers[i].m_spSequence->m_Name.Str(),Layers[i].m_iBlendBaseNode,Layers[i].m_Time,Layers[i].m_Blend);
		_pUtil2D->Text(Clip,pFont,StartX+10,StartY + iRow * OffsetY,Info,Color,10.0f);
		iRow++;
	}
}
#endif
#endif
//
// 
//

void CWObject_Character::RenderSurfaceOverlay(CWorld_Client* _pWClient, CXR_Engine* _pEngine, CRC_Util2D* _pUtil2D, CClipRect& _Clip, uint16 _SurfaceID, int32 _StartTick, uint16 _Duration, int8 _Type, uint32 _WH)
{
	int Tick = _pWClient->GetGameTick() - _StartTick;
	if(_SurfaceID != 0 && (Tick <= _Duration || _Duration == 0))
	{
		CMTime Time = CMTime::CreateFromTicks(Tick, _pWClient->GetGameTickTime(), _pWClient->GetRenderTickFrac());
		_pUtil2D->SetSurface(_pWClient->GetMapData()->GetResource_SurfaceID(_SurfaceID), Time);
		fp32 PixelAspect = _pUtil2D->GetRC()->GetDC()->GetPixelAspect();
		CPnt Size = CPnt(RoundToInt(_pUtil2D->GetTextureWidth() * PixelAspect), _pUtil2D->GetTextureHeight());
		
		if (_Type == 1)
		{
			CTextureContainer_Screen* pTCScreen = safe_cast<CTextureContainer_Screen>(_pEngine->GetInterface(XR_ENGINE_TCSCREEN));
			int ScreenID = (pTCScreen) ? pTCScreen->GetTextureID(6) : 0;
			if(ScreenID != 0)
			{
				CXR_VBManager* pVBM = _pUtil2D->GetVBM();

				CRC_Viewport* pVP = pVBM->Viewport_Get();
				CRct VPRect = pVP->GetViewRect();
				CClipRect Clip(pVP->GetViewClip());

				CPnt TopLeft(VPRect.p0.x, VPRect.p0.y);
				//CPnt TopLeft(RoundToInt(_Clip.clip.p1.x * 0.95f - Size.x), RoundToInt(_Clip.clip.p1.y * 0.08f));
				_pUtil2D->SetTextureOrigo(Clip, TopLeft);
				//_pUtil2D->SetTextureScale(1.0f * PixelAspect, 1.0f);
				int sw = VPRect.GetWidth();
				int sh = VPRect.GetHeight();
				fp32 fw = fp32(sw);
				fp32 fh = fp32(sh);

				//CRenderContext* pRC = _pUtil2D->GetRC();
				//CPnt ScreenSize = pRC->GetDC()->GetScreenSize();

				//CVec2Dfp32 CoordScale = _pUtil2D->GetCoordinateScale();
				//fp32 ScreenAspect = pRC->GetDC()->GetScreenAspect();
				//fp32 PixelAspect = pRC->GetDC()->GetPixelAspect();
				//bool bWidrScreen = pRC->GetDC()->IsWidescreen();

				if (_WH)
				{
					sw = _WH >> 16 & 0xffff;
					sh = _WH & 0xffff;
					fw = fp32(sw);
					fh = fp32(sh);
				}

				_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / fw, fp32(_pUtil2D->GetTextureHeight()) / fh);
				//_pUtil2D->SetTextureScale(1.0f * PixelAspect, 1.0f);
				_pUtil2D->Rect(Clip, CRct(TopLeft.x, TopLeft.y, TopLeft.x + sw, TopLeft.y + sh), 0xffffffff);

				// Copy screen to texture
				pVBM->AddCopyToTexture(0.0f, VPRect, CPnt(0, 0), ScreenID, false);
			}
			return;
		}

		CXW_Surface* pSurface = _pWClient->GetMapData()->GetResource_Surface(_SurfaceID);

		CPnt TopLeft(RoundToInt(_Clip.clip.p1.x * 0.80f - Size.x), RoundToInt(_Clip.clip.p1.y * 0.15f));
		_pUtil2D->SetTextureScale(1.0f * PixelAspect, 1.0f);
		_pUtil2D->SetTextureOrigo(_Clip, TopLeft);
		
		// Copy to texture
		CTextureContainer_Screen* pTCScreen = safe_cast<CTextureContainer_Screen>(_pEngine->GetInterface(XR_ENGINE_TCSCREEN));
		int ScreenID = (pTCScreen) ? pTCScreen->GetTextureID(6) : 0;
		if(ScreenID != 0)
		{
			CXR_VBManager* pVBM = _pUtil2D->GetVBM();

			CRC_Viewport* pVP = pVBM->Viewport_Get();
			CRct VPRect = pVP->GetViewRect();
			CMat4Dfp32* pMat2D = pVBM->Alloc_M4_Proj2DRelBackPlane();
			if (!pMat2D)
				return;
			
			CRect2Duint16 VPRect16;
			VPRect16.m_Min.k[0] = VPRect.p0.x;
			VPRect16.m_Min.k[1] = VPRect.p0.y;
			VPRect16.m_Max.k[0] = VPRect.p1.x;
			VPRect16.m_Max.k[1] = VPRect.p1.y;

			// Copy screen to texture
			pVBM->AddCopyToTexture(0.0f, VPRect, CPnt(0, 0), ScreenID, false);
			
			// Render surface overlay
			{
				CRenderContext* pRC = _pUtil2D->GetRC();
				CXR_VBManager* pVBM = _pUtil2D->GetVBM();
				CRC_Viewport* pVP = pVBM->Viewport_Get();
				CRct VPRect = pVP->GetViewRect();
				VPRect16.m_Min.k[0] = VPRect.p0.x;
				VPRect16.m_Min.k[1] = VPRect.p0.y;
				VPRect16.m_Max.k[0] = VPRect.p1.x;
				VPRect16.m_Max.k[1] = VPRect.p1.y;
				int bRenderTextureVertFlip  = pRC->Caps_Flags() & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;

				CRC_Attributes* pA = pVBM->Alloc_Attrib();
				if (!pA)
					return;
				pA->SetDefault();
				pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
				pA->Attrib_TextureID(0, ScreenID);

				CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPRect16, 0xffffffff, 0.0f, pA);
				CVec2Dfp32* pTV = (bRenderTextureVertFlip) ? CXR_Util::VBM_CreateRectUV_VFlip(pVBM, CVec2Dfp32(0.0f, 0.0f), CVec2Dfp32(1.0f, 1.0f)) : CXR_Util::VBM_CreateRectUV(pVBM, CVec2Dfp32(0.0f, 0.0f), CVec2Dfp32(1.0f, 1.0f));
				if (!pTV)
					return;

				pVB->Geometry_TVertexArray(pTV, 0);
				
				CXR_SurfaceContext* pSurfaceContext = _pEngine->GetSC();
				if(pSurfaceContext)
				{
					CMTime SurfaceTime = CMTime::CreateFromTicks(_pWClient->GetGameTick() - _StartTick, _pWClient->GetGameTickTime(), _pWClient->GetRenderTickFrac());
					CXW_SurfaceKeyFrame* pSurfaceKey = pSurface->GetFrame(0, SurfaceTime, &pSurfaceContext->GetTempSurfaceKeyFrame());
					CXR_Util::Render_Surface(0, SurfaceTime, pSurface, pSurfaceKey, _pEngine, pVBM, (CMat4Dfp32*) NULL, (CMat4Dfp32*) NULL, pVB->m_pTransform, pVB);
				}
			}

			// Restore on xenon
			#ifdef PLATFORM_XENON
			{
				CRenderContext* pRC = _pUtil2D->GetRC();
				int bRenderTextureVertFlip  = pRC->Caps_Flags() & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;
				CPnt ScreenSize = pRC->GetDC()->GetScreenSize();

				int sw = pVP->GetViewRect().GetWidth();
				int sh = pVP->GetViewRect().GetHeight();

				CVec2Dfp32 UVMin;
				CVec2Dfp32 UVMax;
				CVec2Dfp32 UVMinFullScreen;
				CVec2Dfp32 UVMaxFullScreen;
				if (pRC->Caps_Flags() & CRC_CAPS_FLAGS_ARBITRARY_TEXTURE_SIZE)
				{
					UVMin = CVec2Dfp32(0.0f / fp32(ScreenSize.x), 0.0f / fp32(ScreenSize.y));
					UVMax = CVec2Dfp32((fp32(sw) - 0.0f) / fp32(ScreenSize.x), (fp32(sh) - 0.0f) / fp32(ScreenSize.y));
					UVMinFullScreen = CVec2Dfp32(0.0f / fp32(ScreenSize.x), 0.0f / fp32(ScreenSize.y));
					UVMaxFullScreen = CVec2Dfp32((fp32(ScreenSize.x) - 0.0f) / fp32(ScreenSize.x), (fp32(ScreenSize.y) - 0.0f) / fp32(ScreenSize.y));
				}
				else
				{
					UVMin = CVec2Dfp32(0.0f / fp32(GetGEPow2(ScreenSize.x)), 0.0f / fp32(GetGEPow2(ScreenSize.y)));
					UVMax = CVec2Dfp32((fp32(sw) - 0.0f) / fp32(GetGEPow2(ScreenSize.x)), (fp32(sh) - 0.0f) / fp32(GetGEPow2(ScreenSize.y)));
					UVMinFullScreen = CVec2Dfp32(0.0f / fp32(GetGEPow2(ScreenSize.x)), 0.0f / fp32(GetGEPow2(ScreenSize.y)));
					UVMaxFullScreen = CVec2Dfp32((fp32(ScreenSize.x) - 0.0f) / fp32(GetGEPow2(ScreenSize.x)), (fp32(ScreenSize.y) - 0.0f) / fp32(GetGEPow2(ScreenSize.y)));
				}

				// Reposition/stretch screen
				CVec2Dfp32* pTV = (bRenderTextureVertFlip) ?
					CXR_Util::VBM_CreateRectUV_VFlip(pVBM, UVMin, UVMax) :
					CXR_Util::VBM_CreateRectUV(pVBM, UVMin, UVMax);

				CRC_Attributes* pA = pVBM->Alloc_Attrib();
				if (!pA)
					return;

				pA->SetDefault();
				pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
				pA->Attrib_TextureID(0, ScreenID);

				CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPRect16, 0xffffffff, 0.0f, pA);
				if (!pVB)
					return;

				pVB->Geometry_TVertexArray(pTV, 0);
				pVBM->AddVB(pVB);
			}
			#endif

		}
	}
}


void CWObject_Character::OnClientRenderHealthHud(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine *_pEngine, CRC_Util2D *_pUtil2D)
{
	MSCOPESHORT(CWObject_Character::OnClientRenderHealthHud);
	
	// Get health
	// If fade is zero, just skip the rendering
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
	M_ASSERT(pCD, "no client data!");
//	CWO_CharPlayer_ClientData& CD = *safe_cast<CWO_CharPlayer_ClientData>(pCD);

	_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	_pUtil2D->SetTexture(0);
	CClipRect Clip(0, 0, 640, 480);
	CWObject_Telephone::RenderPad(_pWClient, _pUtil2D,_pObj, pCD);
	_pUtil2D->SetTexture(NULL);

	if (pCD->m_PagerStartTick > 0 && (pCD->m_GameTick - pCD->m_PagerStartTick) < 200)
	{
		fp32 GameTickTime = _pWClient->GetGameTickTime();
		fp32 Flash = ((pCD->m_GameTick - pCD->m_PagerStartTick) + pCD->m_PredictFrameFrac) * GameTickTime;
		fp32 Fade = (200 * GameTickTime - Flash) * _PI;
		Fade = (M_Sin(Fade - _PI*0.5f) + 1.0f)*0.5f;
		CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
		CStr Number = (CFStr)pCD->m_PagerPhoneNumber;
		int32 Color = CPixel32((255), (255), (255), (int)(Fade * 0.80f * 255.0f));
		_pUtil2D->Text(Clip,pFont,100,400,Number,Color,20.0f);
	}

//	fp32 Fade = LERP(pCD->m_LastHealthHudFade, pCD->m_HealthHudFade, pCD->m_PredictFrameFrac);
//	if (Fade <= 0.0f)
//		return;
	fp32 Fade = 1.0f;

// Make sure to disable this on release
#if 1
	{
		if (pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_GODMODE)
		{
			CRC_Util2D Util;
			Util.Begin(_pUtil2D->GetRC(), _pUtil2D->GetVBM()->Viewport_Get(), _pUtil2D->GetVBM());
			Util.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			Util.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE);

			CClipRect ClipRect(_pUtil2D->GetVBM()->Viewport_Get()->GetViewRect());

			CRC_Font* pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
			const fp32 Size = 20.0f;
			const char* pMessage = "GOD MODE";
			int sw = ClipRect.GetWidth();
			int tw = TruncToInt(pFont->GetWidth(Size, pMessage));
			int th = TruncToInt(pFont->GetHeight(Size, pMessage));
			CPnt Pos(int(fp32(sw-tw)*0.5f), th+5);
			Util.Text(ClipRect, pFont, Pos.x, Pos.y, pMessage, 0xffffffff, Size);

			Util.End();
		}
	}

	if (_pWClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_ISDEBUGHUDVISIBLE), _pWClient->Player_GetLocalObject()))
	{
		_pUtil2D->SetTexture(0);
		CPnt ScreenSize = _pUtil2D->GetRC()->GetDC()->GetScreenSize();

		// Render Health and darkness
		int32 StartX = 42;
		int32 StartY = 36;
		int32 SizeX = TruncToInt(fp32(ScreenSize.x) * 0.2f);
		int32 SizeY = 4;

		int32 AmountX = (int32)TruncToInt((fp32(SizeX) / fp32(pCD->m_MaxHealth.m_Value)) * fp32(pCD->m_Health.m_Value));

		uint8 GodModeColorFill = 0;
		uint8 GodModeColorEx1 = 0;
		uint8 GodModeColorEx2 = 0;
		if(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_GODMODE)
		{
			GodModeColorFill = 196;
			GodModeColorEx1 = 128;
			GodModeColorEx2 = 64;
		}

		CPixel32 ColorBorder(128,0,0,196);
		CPixel32 ColorFill(196,GodModeColorFill,0,128);

		uint MaxDarkness = pCD->m_MaxDarkness.m_Value;
		uint Darkness = pCD->m_Darkness.m_Value;
		CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("MONOPRO"));
		_pUtil2D->Text(Clip, pFont, StartX + SizeX + 12, StartY, CStrF("%d / %d", pCD->m_Health.m_Value, pCD->m_MaxHealth.m_Value), ColorFill);
		for (uint i = 0; i < 2; i++)
		{
			_pUtil2D->Rect(Clip, CRct(StartX - 2, StartY - 2, StartX + SizeX + 2, StartY - 1), ColorBorder);
			_pUtil2D->Rect(Clip, CRct(StartX - 2, StartY - 1, StartX - 1, StartY + SizeY + 2), ColorBorder);
			_pUtil2D->Rect(Clip, CRct(StartX - 1, StartY + SizeY + 1, StartX + SizeX + 2, StartY + SizeY + 2), ColorBorder);
			_pUtil2D->Rect(Clip, CRct(StartX + SizeX + 1, StartY - 1, StartX + SizeX + 2, StartY + SizeY + 1), ColorBorder);
			if (AmountX > 0)
				_pUtil2D->Rect(Clip, CRct(StartX, StartY, StartX + AmountX, StartY + SizeY), ColorFill);

			ColorBorder = CPixel32(0,0,128,196);
			ColorFill = CPixel32(GodModeColorFill/2,0,196,128);
			StartY += 10;
			AmountX = (int32)TruncToInt((fp32(SizeX) / fp32(MaxDarkness)) * fp32(Darkness));
		}
		_pUtil2D->Text(Clip, pFont, StartX + SizeX + 12, StartY - 10, CStrF("%d / %d", Darkness, MaxDarkness), ColorFill);

		CPixel32 ColorDarknessShield	= (pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD) ? 0xff808080 : 0xff404040;
		CPixel32 ColorDarknessVision	= (pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_DARKNESSVISION) ? 0xff808080 : 0xff404040;
		CPixel32 ColorDrain				= 0xff808080;
		CPixel32 ColorDevour			= 0xff808080;
		CPixel32 ColorCreepingDark		= (pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK) ? 0xff808080 : 0xff404040;
		CPixel32 ColorDemonArm			= (pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_DEMONARM) ? 0xff808080 : 0xff404040;
		CPixel32 ColorBlackHole			= (pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_BLACKHOLE) ? 0xff808080 : 0xff404040;
		CPixel32 ColorAncientWeapons	= (pCD->m_DarknessPowersAvailable & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS) ? 0xff808080 : 0xff404040;

		ColorDarknessShield	= (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD) ? CPixel32(0xff80ff80) : ColorDarknessShield;
		ColorDarknessVision	= (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DARKNESSVISION) ? CPixel32(0xff80ff80) : ColorDarknessVision;
		ColorDrain			= (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DRAIN) ? CPixel32(0xff80ff80) : ColorDrain;
		ColorDevour			= (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DEVOUR) ? CPixel32(0xff80ff80) : ColorDevour;
		ColorCreepingDark	= (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_CREEPINGDARK) ? CPixel32(0xff80ff80) : ColorCreepingDark;
		ColorDemonArm		= (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_DEMONARM) ? CPixel32(0xff80ff80) : ColorDemonArm;
		ColorBlackHole		= (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_BLACKHOLE) ? CPixel32(0xff80ff80) : ColorBlackHole;
		ColorAncientWeapons	= (pCD->m_DarknessSelectionMode & PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS) ? CPixel32(0xff80ff80) : ColorAncientWeapons;

		switch (ResolveDarknessSelection(pCD->m_DarknessSelectedPower))
		{
		case PLAYER_DARKNESSMODE_POWER_CREEPINGDARK:
			ColorCreepingDark += 0x00400000;
			break;
		case PLAYER_DARKNESSMODE_POWER_DEMONARM:
			ColorDemonArm += 0x00400000;
			break;
		case PLAYER_DARKNESSMODE_POWER_BLACKHOLE:
			ColorBlackHole += 0x00400000;
			break;
		case PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS:
			ColorAncientWeapons += 0x00400000;
			break;
		}

		int Item0Num = pCD->m_Item0_Num.m_Value;
		int Item1Num = pCD->m_Item1_Num.m_Value;
		_pUtil2D->Text(Clip, pFont, StartX, StartY, CStr("DarknessShield"), ColorDarknessShield); StartX += 90;
		_pUtil2D->Text(Clip, pFont, StartX, StartY, CStr("DarknessVision"), ColorDarknessVision); StartX -= 90; StartY += 8;
		_pUtil2D->Text(Clip, pFont, StartX, StartY, CStr("Drain"), ColorDrain); StartY += 8;
		_pUtil2D->Text(Clip, pFont, StartX, StartY, CStr("Devour"), ColorDevour); StartY += 8;
		_pUtil2D->Text(Clip, pFont, StartX, StartY, CStr("CreepingDark"), ColorCreepingDark); StartX += 90;
		if (pCD->m_DarknessLevel.GetPowerLevel(PLAYER_DARKNESSMODE_POWER_DEMONARM) == 0)
			_pUtil2D->Text(Clip, pFont, StartX, StartY, CStr("DemonArm"), ColorDemonArm);
		else _pUtil2D->Text(Clip, pFont, StartX, StartY, CStr("DemonArm (Boosted)"), ColorDemonArm); StartX -= 90; StartY += 8;
		_pUtil2D->Text(Clip, pFont, StartX, StartY, CStr("BlackHole"), ColorBlackHole); StartX += 90;
		_pUtil2D->Text(Clip, pFont, StartX, StartY, CStr("Ancient Weapons"), ColorAncientWeapons); StartX -= 90; StartY += 16;
		_pUtil2D->Text(Clip, pFont, StartX, StartY, CStr("Left Weapon"), CPixel32(0xffffffff)); StartX += 90;
		_pUtil2D->Text(Clip, pFont, StartX, StartY, CStr("Right Weapon"), CPixel32(0xffffffff)); StartX -= 80; StartY += 8;
		_pUtil2D->Text(Clip, pFont, StartX, StartY, CStrF("%d", Item1Num), CPixel32(0xffffffff)); StartX += 90;
		_pUtil2D->Text(Clip, pFont, StartX, StartY, CStrF("%d", Item0Num), CPixel32(0xffffffff)); StartX -= 100; StartY += 8;

		// Render Darkness progression using a detailed juice bar
		StartY += 312;
		CRC_Attributes APrev = *_pUtil2D->GetAttrib();
		CRC_Attributes ADebugHUD;
		ADebugHUD.SetDefault();
		ADebugHUD.Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZCOMPARE);
		ADebugHUD.Attrib_RasterMode(CRC_RASTERMODE_MULTIPLY);
		_pUtil2D->SetAttrib(&ADebugHUD);
		_pUtil2D->Rect(Clip, CRct(StartX - 17, StartY - 302, StartX + 17, StartY + 10), CPixel32(128,128,128,255));
		_pUtil2D->SetAttrib(&APrev);

		CPixel32 ColorDarknessProgression = CPixel32(128+GodModeColorEx1,128,255,196);
		if (Darkness)
		{
			_pUtil2D->Rect(Clip, CRct(StartX - 7, StartY, StartX + 7, StartY + 1), ColorDarknessProgression);
			StartY -= 3;
			for (uint i = 1; i < Darkness; i++)
			{
				if ((i+1)%20 == 0) _pUtil2D->Rect(Clip, CRct(StartX - 7, StartY, StartX + 7, StartY + 1), ColorDarknessProgression);
				else _pUtil2D->Rect(Clip, CRct(StartX - 4, StartY, StartX + 4, StartY + 1), ColorDarknessProgression);
				StartY -= 3;
			}
		}

		ColorDarknessProgression = CPixel32(64+GodModeColorEx2,64,128,196);
		if (!Darkness)
		{
			_pUtil2D->Rect(Clip, CRct(StartX - 7, StartY, StartX + 7, StartY + 1), ColorDarknessProgression);
			StartY -= 3;
		}
		for (uint i = (!Darkness) ? 1 : Darkness; i < MaxDarkness; i++)
		{
			if ((i+1)%20 == 0) _pUtil2D->Rect(Clip, CRct(StartX - 7, StartY, StartX + 7, StartY + 1), ColorDarknessProgression);
			else _pUtil2D->Rect(Clip, CRct(StartX - 4, StartY, StartX + 4, StartY + 1), ColorDarknessProgression);
			StartY -= 3;
		}

		ColorDarknessProgression = CPixel32(64,64,64,196);
		for (uint i = MaxDarkness; i < 100; i++)
		{
			if ((i+1)%20 == 0) _pUtil2D->Rect(Clip, CRct(StartX - 7, StartY, StartX + 7, StartY + 1), ColorDarknessProgression);
			else _pUtil2D->Rect(Clip, CRct(StartX - 4, StartY, StartX + 4, StartY + 1), ColorDarknessProgression);
			StartY -= 3;
		}

		// Render darkness level
		StartX = 22;
		StartY += 328;
		ColorBorder = CPixel32(0,128,0,196);
		ColorFill = CPixel32(0,255,0,128);
		SizeX = 20;
		_pUtil2D->Rect(Clip, CRct(StartX - 2, StartY - 2, StartX + SizeX + 2, StartY - 1), ColorBorder);
		_pUtil2D->Rect(Clip, CRct(StartX - 2, StartY - 1, StartX - 1, StartY + SizeY + 2), ColorBorder);
		_pUtil2D->Rect(Clip, CRct(StartX - 1, StartY + SizeY + 1, StartX + SizeX + 2, StartY + SizeY + 2), ColorBorder);
		_pUtil2D->Rect(Clip, CRct(StartX + SizeX + 1, StartY - 1, StartX + SizeX + 2, StartY + SizeY + 1), ColorBorder);
		uint DarknessLevel = pCD->m_DarknessLevel.GetLevel();
		AmountX = pCD->m_DarknessLevel.GetLevelHearts();
		if (AmountX > 0) _pUtil2D->Rect(Clip, CRct(StartX, StartY, StartX + MinMT(20, AmountX), StartY + SizeY), ColorFill);
		StartX += SizeX + 12;
		if (DarknessLevel < 4) _pUtil2D->Text(Clip, pFont, StartX, StartY, CStrF("Level %d (%d/20)", DarknessLevel + 1, AmountX), CPixel32(0xffffffff));
		else _pUtil2D->Text(Clip, pFont, StartX, StartY, CStrF("Level %d (%d/*)", DarknessLevel + 1, AmountX), CPixel32(0xffffffff));
	}
#endif

	bool bIsMP = _pWClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETISMP), _pWClient->Game_GetObjectIndex()) ? true : false;
	int DurTick = TruncToInt(3.0f * _pWClient->GetGameTicksPerSecond());
	if(!(bIsMP || pCD->m_WeaponHUDStartTick && pCD->m_WeaponHUDStartTick + DurTick > _pWClient->GetGameTick()))
		return;

	int32 ColorWeapon;
	fp32 WeaponFade = 0.0f;
	if(!bIsMP)
	{
		int CurTick = _pWClient->GetGameTick() - pCD->m_WeaponHUDStartTick;
		WeaponFade = (fp32)CurTick / (DurTick / 3.0f);
		if(WeaponFade > 2.0f)
			WeaponFade = 1.0f - (WeaponFade - 2.0f);
		WeaponFade = Max(0.0f, Min(1.0f, WeaponFade));
	}
	else
		WeaponFade = 1.0f;
	ColorWeapon = CPixel32(215, 235, 255, TruncToInt(WeaponFade * 0.5f * 255.0f));
	
	CRct Rect = _pUtil2D->GetVBM()->Viewport_Get()->GetViewRect();
	CVec2Dfp32 OrgScale = _pUtil2D->GetCoordinateScale();
	_pUtil2D->SetCoordinateScale(CVec2Dfp32(Rect.GetWidth() / 640.0f, Rect.GetHeight() / 480.0f));

	if(pCD->m_Item0_Icon != 0)
	{
		// Item 0
		CXW_Surface *pSurface = _pWClient->GetMapData()->GetResource_Surface(pCD->m_Item0_Icon);
		if (pSurface)
		{
			fp32 Aspect = _pUtil2D->GetRC()->GetDC()->GetScreenAspect() * _pUtil2D->GetRC()->GetDC()->GetPixelAspect();
			CPnt point = _pUtil2D->GetRC()->GetDC()->GetScreenSize();
			_pUtil2D->SetSurface(pSurface, pCD->m_GameTime);
			CPnt Size((int)(0.55f * _pUtil2D->GetTextureWidth() / Aspect), (int)(0.55f * _pUtil2D->GetTextureHeight()));

#ifdef PLATFORM_CONSOLE
			CPnt Pos(592 - Size.x, 435 - Size.y);
#else
			CPnt Pos(594 - Size.x, 424 - Size.y / 2);
#endif
			_pUtil2D->SetTextureOrigo(Clip, CPnt(Pos.x + Size.x, Pos.y));
			_pUtil2D->SetTextureScale(-fp32(_pUtil2D->GetTextureWidth()) / Size.x, fp32(_pUtil2D->GetTextureHeight()) / Size.y);
			_pUtil2D->Rect(Clip, CRct(Pos.x, Pos.y, Pos.x + Size.x, Pos.y + Size.y), ColorWeapon);

			CRC_Font* pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
			if(pFont && (pCD->m_Item0_Num.m_Value > 0 || pCD->m_Pickup_Magazine_Num.m_Value))
				_pUtil2D->Text(Clip, pFont, (int)535, (int)(Pos.y + Size.y * 0.8f), CFStrF("%i/%i", pCD->m_Item0_Num.m_Value, pCD->m_Pickup_Magazine_Num.m_Value).Str(), ColorWeapon, 15);
		}
	}
	if(pCD->m_Item1_Icon != 0)
	{
		// Item 1
		CXW_Surface *pSurface = _pWClient->GetMapData()->GetResource_Surface(pCD->m_Item1_Icon);
		if (pSurface)
		{
			fp32 Aspect = _pUtil2D->GetRC()->GetDC()->GetScreenAspect() * _pUtil2D->GetRC()->GetDC()->GetPixelAspect();
			_pUtil2D->SetSurface(pSurface, pCD->m_GameTime);
			CPnt Size((int)(0.55f * _pUtil2D->GetTextureWidth() / Aspect), (int)(0.55f * _pUtil2D->GetTextureHeight()));

#ifdef PLATFORM_CONSOLE
			CPnt Pos(50, 435 - Size.y);
#else
			CPnt Pos(22, 424 - Size.y / 2);
#endif
			_pUtil2D->SetTextureOrigo(Clip, Pos);
			_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / Size.x, fp32(_pUtil2D->GetTextureHeight()) / Size.y);
			_pUtil2D->Rect(Clip, CRct(Pos.x, Pos.y, Pos.x + Size.x, Pos.y + Size.y), ColorWeapon);

			CRC_Font* pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("TEXT"));
			if(pFont && pCD->m_Item1_Num.m_Value > 0)
				_pUtil2D->Text(Clip, pFont, (int)(Pos.x + Size.x * 0.6f), (int)(Pos.y + Size.y * 0.8f), CFStrF("%i", pCD->m_Item1_Num.m_Value).Str(), ColorWeapon, 15);
		}
	}

	if(!bIsMP)
		return;

	// Depending on whether a power is active, change color
	int32 ColorHealth;
	//if(pCD->m_DarknessSelectionMode & (PLAYER_DARKNESSMODE_POWER_DARKNESSHIELD | PLAYER_DARKNESSMODE_POWER_ANCIENTWEAPONS))
	//	ColorHealth = CPixel32((15), (112), (208), (int)(Fade * 0.50f * 255.0f));
	//else
		ColorHealth = CPixel32((215), (235), (255), (int)(Fade * 0.50f * 255.0f));

	{
		// Render shield
		const int32 SizeX = 100;
		const int32 SizeY = 8;
#ifdef PLATFORM_CONSOLE
		const int32 StartX = 50;
		const int32 StartY = 40;
#else
		const int32 StartX = 27;
		const int32 StartY = 36;
#endif

		const int CenterX = StartX - 13;
		const int CenterY = StartY + 3;
		const int Size = 22;
		fp32 Aspect = _pUtil2D->GetRC()->GetDC()->GetScreenAspect() * _pUtil2D->GetRC()->GetDC()->GetPixelAspect();

		_pUtil2D->SetTexture(0);
		_pUtil2D->Rect(Clip, CRct(StartX - 2, StartY - 2, StartX + SizeX + 2, StartY - 1), ColorHealth);
		_pUtil2D->Rect(Clip, CRct(StartX - 2, StartY - 1, StartX - 1, StartY + SizeY + 2), ColorHealth);
		_pUtil2D->Rect(Clip, CRct(StartX - 1, StartY + SizeY + 1, StartX + SizeX + 2, StartY + SizeY + 2), ColorHealth);
		_pUtil2D->Rect(Clip, CRct(StartX + SizeX + 1, StartY - 1, StartX + SizeX + 2, StartY + SizeY + 1), ColorHealth);

		// Reserve 1 health for bottom stuff
		int32 nHealth = pCD->m_MaxHealth > 0 ? Max(Min(((pCD->m_Health-1) * SizeX) / (pCD->m_MaxHealth-1),SizeX),(int32)0) : 0;
		if (nHealth > 0)
		{
			int Color = ColorHealth;
			if(pCD->m_ExtraFlags & PLAYER_EXTRAFLAGS_GODMODE)
				Color &= 0xffffff00;

			_pUtil2D->Rect(Clip, CRct(StartX, StartY, StartX + nHealth, StartY + SizeY), Color);
		}
		
/*		CWO_CharShapeshifter_ClientData& CD = CWObject_CharShapeshifter::GetClientData(_pObj);
		if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_SHIELD)
		{
			int ShieldAmount = CD.m_ShieldBoostAmount;
			int32 nShield = pCD->m_MaxHealth > 0 ? Max(Min(((ShieldAmount - 1) * SizeX) / (pCD->m_MaxHealth-1),SizeX),(int32)0) : 0;
			if (nShield > 0)
			{
				int Color = CPixel32(215, 235, 255, 128);
				_pUtil2D->Rect(Clip, CRct(StartX, StartY, StartX + nShield, StartY + SizeY), Color);
			}
		}*/
	}

	{	//Render powerups
		const int32 SizeX = 100;
		const int32 SizeY = 8;
#ifdef PLATFORM_CONSOLE
		const int32 StartX = 450;
		const int32 StartY = 65;
#else
		const int32 StartX = 485;
		const int32 StartY = 36;
#endif

		const int CenterY = StartY + 3;
		const int Size = 64;
		fp32 Aspect = _pUtil2D->GetRC()->GetDC()->GetScreenAspect() * _pUtil2D->GetRC()->GetDC()->GetPixelAspect();

		CWO_CharShapeshifter_ClientData& CD = CWObject_CharShapeshifter::GetClientData(_pObj);
		int CenterX = StartX + SizeX + 18;
		if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_SPEED)
		{
			int Alpha = 255;
			int TicksLeft = CD.m_SpeedBoostDuration - (_pWClient->GetGameTick() - CD.m_SpeedBoostStartTick);
			if(TicksLeft < _pWClient->GetGameTicksPerSecond() * 5.0f)//Less then 5 seconds left, start pulsating
				Alpha = TruncToInt(M_Fabs(((TicksLeft % 30) - 15) / 15.0f) * 200.0f) + 55;
			_pUtil2D->SetSurface("GUI_HUD_darkness", pCD->m_GameTime);
			_pUtil2D->SetTextureOrigo(Clip, CPnt((int)(CenterX - Size / Aspect / 2), CenterY - Size / 2));
			_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / Size * Aspect, fp32(_pUtil2D->GetTextureHeight()) / Size);
			_pUtil2D->Rect(Clip, CRct((int)(CenterX - Size / 2 / Aspect), CenterY - Size / 2, (int)(CenterX + Size / 2 / Aspect), CenterY + Size / 2 ), CPixel32(215,235,255,Alpha));
			CenterX -= 45;
		}
		if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_DAMAGE)
		{
			int Alpha = 255;
			int TicksLeft = CD.m_DamageBoostDuration - (_pWClient->GetGameTick() - CD.m_DamageBoostStartTick);
			if(TicksLeft < _pWClient->GetGameTicksPerSecond() * 5.0f)//Less then 5 seconds left, start pulsating
				Alpha = TruncToInt(M_Fabs(((TicksLeft % 30) - 15) / 15.0f) * 200.0f) + 55;
			_pUtil2D->SetSurface("GUI_quad", pCD->m_GameTime);
			_pUtil2D->SetTextureOrigo(Clip, CPnt((int)(CenterX - Size / Aspect / 2), CenterY - Size / 2));
			_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / Size * Aspect, fp32(_pUtil2D->GetTextureHeight()) / Size);
			_pUtil2D->Rect(Clip, CRct((int)(CenterX - Size / 2 / Aspect), CenterY - Size / 2, (int)(CenterX + Size / 2 / Aspect), CenterY + Size / 2 ), CPixel32(215,235,255,Alpha));
			CenterX -= 45;
		}
		if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_HEALTH)
		{
			int Alpha = 255;
			int TicksLeft = CD.m_HealthBoostDuration - (_pWClient->GetGameTick() - CD.m_HealthBoostStartTick);
			if(TicksLeft < _pWClient->GetGameTicksPerSecond() * 5.0f)//Less then 5 seconds left, start pulsating
				Alpha = TruncToInt(M_Fabs(((TicksLeft % 30) - 15) / 15.0f) * 200.0f) + 55;
			_pUtil2D->SetSurface("GUI_healthboost", pCD->m_GameTime);
			_pUtil2D->SetTextureOrigo(Clip, CPnt((int)(CenterX - Size / Aspect / 2), CenterY - Size / 2));
			_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / Size * Aspect, fp32(_pUtil2D->GetTextureHeight()) / Size);
			_pUtil2D->Rect(Clip, CRct((int)(CenterX - Size / 2 / Aspect), CenterY - Size / 2, (int)(CenterX + Size / 2 / Aspect), CenterY + Size / 2 ), CPixel32(215,235,255,Alpha));
			CenterX -= 45;
		}
		if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_SHIELD)
		{
			int Alpha = 255;
			int TicksLeft = CD.m_ShieldBoostDuration - (_pWClient->GetGameTick() - CD.m_ShieldBoostStartTick);
			if(TicksLeft < _pWClient->GetGameTicksPerSecond() * 5.0f)//Less then 5 seconds left, start pulsating
				Alpha = TruncToInt(M_Fabs(((TicksLeft % 30) - 15) / 15.0f) * 200.0f) + 55;
			_pUtil2D->SetSurface("GUI_shield", pCD->m_GameTime);
			_pUtil2D->SetTextureOrigo(Clip, CPnt((int)(CenterX - Size / Aspect / 2), CenterY - Size / 2));
			_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / Size * Aspect, fp32(_pUtil2D->GetTextureHeight()) / Size);
			_pUtil2D->Rect(Clip, CRct((int)(CenterX - Size / 2 / Aspect), CenterY - Size / 2, (int)(CenterX + Size / 2 / Aspect), CenterY + Size / 2 ), CPixel32(215,235,255,Alpha));
			CenterX -= 45;
		}
		if(CD.m_ActiveBoosts & MP_BOOST_ACTIVE_INVISIBILITY)
		{
			int Alpha = 255;
			int TicksLeft = CD.m_ShieldBoostDuration - (_pWClient->GetGameTick() - CD.m_ShieldBoostStartTick);
			if(TicksLeft < _pWClient->GetGameTicksPerSecond() * 5.0f)//Less then 5 seconds left, start pulsating
				Alpha = TruncToInt(M_Fabs(((TicksLeft % 30) - 15) / 15.0f) * 200.0f) + 55;
			_pUtil2D->SetSurface("GUI_HUD_blackhole", pCD->m_GameTime);
			_pUtil2D->SetTextureOrigo(Clip, CPnt((int)(CenterX - Size / Aspect / 2), CenterY - Size / 2));
			_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / Size * Aspect, fp32(_pUtil2D->GetTextureHeight()) / Size);
			_pUtil2D->Rect(Clip, CRct((int)(CenterX - Size / 2 / Aspect), CenterY - Size / 2, (int)(CenterX + Size / 2 / Aspect), CenterY + Size / 2 ), CPixel32(215,235,255,Alpha));
		}
	}

	CWObject_Client *pObj = _pWClient->Object_Get(_pWClient->Game_GetObjectIndex());
	if(pObj)
	{
		CWObject_GameCore::CClientData *pCD = (CWObject_GameCore::CClientData *)_pWClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETCLIENTDATA), _pWClient->Game_GetObjectIndex());;
		CWO_CharShapeshifter_ClientData& CD = CWObject_CharShapeshifter::GetClientData(_pObj);

		int PlayerScore, BestNonPlayerScore;
		CStr PlayerName, BestNonPlayerName;
		
		PlayerScore = -9999;
		BestNonPlayerScore = -9999;

		PlayerScore = pCD->m_lPlayerScores[CD.m_iPlayer];
		PlayerName = pCD->m_lPlayerNames[CD.m_iPlayer];
		
		for(int i = 0; i < pCD->m_lPlayerNames.Len(); i++)
		{
			if(pCD->m_lPlayerNames[i] != "" && (pCD->m_lPlayerNames[i] != PlayerName || pCD->m_lPlayerScores[i] != PlayerScore))
			{
				if(pCD->m_lPlayerScores[i] > BestNonPlayerScore)
				{
					BestNonPlayerScore = pCD->m_lPlayerScores[i];
					BestNonPlayerName = pCD->m_lPlayerNames[i];
				}
			}
		}

		CRC_Font *pFont = _pWClient->m_spGUIData->GetResource_Font(_pWClient->m_spGUIData->GetResourceIndex_Font("HEADINGS"));
		CStr St;
		if(PlayerScore > BestNonPlayerScore)
		{
			if(PlayerName.IsUnicode())
				St = CStrF(WTEXT("%i - %s"), PlayerScore, PlayerName.StrW());
			else
				St = CStrF("%i - %s", PlayerScore, PlayerName.Str());
		}
		else
		{
			if(BestNonPlayerName.IsUnicode())
				St = CStrF(WTEXT("%i - %s"), BestNonPlayerScore, BestNonPlayerName.StrW());
			else
				St = CStrF("%i - %s", BestNonPlayerScore, BestNonPlayerName.Str());
		}
#ifdef PLATFORM_CONSOLE
		_pUtil2D->Text(Clip, pFont, 50, 60, St, CPixel32(255, 255, 255, 255), 9.0f);
#else
		_pUtil2D->Text(Clip, pFont, 25, 55, St, CPixel32(255, 255, 255, 255), 9.0f);
#endif
		St = "";

		if(PlayerScore > BestNonPlayerScore)
		{
			if(BestNonPlayerScore > -99)
			{
				if(BestNonPlayerName.IsUnicode())
					St = CStrF(WTEXT("%i - %s"), BestNonPlayerScore, BestNonPlayerName.StrW());
				else
					St = CStrF("%i - %s", BestNonPlayerScore, BestNonPlayerName.Str());
			}
		}
		else
		{
			if(PlayerName.IsUnicode())
				St = CStrF(WTEXT("%i - %s"), PlayerScore, PlayerName.StrW());
			else
				St = CStrF("%i - %s", PlayerScore, PlayerName.Str());
		}
#ifdef PLATFORM_CONSOLE
		_pUtil2D->Text(Clip, pFont, 50, 70, St, CPixel32(255, 255, 255, 255), 9.0f);
#else
		_pUtil2D->Text(Clip, pFont, 25, 65, St, CPixel32(255, 255, 255, 255), 9.0f);
#endif
	}

/*	{
		// Render which darkness power is currently selected
		const int32 SizeX = 100;
		const int32 SizeY = 8;
		const int32 StartX = 485;
		const int32 StartY = 36;

		const int CenterX = StartX + SizeX + 18;
		const int CenterY = StartY + 3;
		const int Size = 38;
		fp32 Aspect = _pUtil2D->GetRC()->GetDC()->GetScreenAspect() * _pUtil2D->GetRC()->GetDC()->GetPixelAspect();
		
		if(pCD->m_DarknessSelectedPower >= 0 && pCD->m_DarknessSelectedPower != 2)
		{
			int Color = ColorHealth;
			int DarknessLeft = MinMT(TruncToInt((255.0f / fp32(pCD->m_MaxDarkness)) * fp32(pCD->m_Darkness)), 0xff);
			Color &= CPixel32(255,DarknessLeft,DarknessLeft,255);
			//if(!(pCD->m_DarknessPowers & CWObject_Character::ResolveDarknessSelection(pCD->m_DarknessSelectedPower)))
			//	Color &= CPixel32(255,0,0,255);

			_pUtil2D->SetSurface(s_DarknessGUISurf[pCD->m_DarknessSelectedPower], pCD->m_GameTime);
			_pUtil2D->SetTextureOrigo(Clip, CPnt((int)(CenterX - Size / Aspect / 2), CenterY - Size / 2));
			_pUtil2D->SetTextureScale(fp32(_pUtil2D->GetTextureWidth()) / Size * Aspect, fp32(_pUtil2D->GetTextureHeight()) / Size);
			// Can we activate power atm, if so render with another color (red?)
			_pUtil2D->Rect(Clip, CRct((int)(CenterX - Size / 2 / Aspect), CenterY - Size / 2, (int)(CenterX + Size / 2 / Aspect), CenterY + Size / 2 ), Color);
		}

		_pUtil2D->SetTexture(0);
		_pUtil2D->Rect(Clip, CRct(StartX - 2, StartY - 2, StartX + SizeX + 2, StartY - 1), ColorHealth);
		_pUtil2D->Rect(Clip, CRct(StartX - 2, StartY - 1, StartX - 1, StartY + SizeY + 2), ColorHealth);
		_pUtil2D->Rect(Clip, CRct(StartX - 1, StartY + SizeY + 1, StartX + SizeX + 2, StartY + SizeY + 2), ColorHealth);
		_pUtil2D->Rect(Clip, CRct(StartX + SizeX + 1, StartY - 1, StartX + SizeX + 2, StartY + SizeY + 1), ColorHealth);

		int32 nDarkness = pCD->m_MaxDarkness > 0 ? Max(Min((pCD->m_Darkness * SizeX) / pCD->m_MaxDarkness,SizeX),(int32)0) : 0;
		if (nDarkness > 0)
			_pUtil2D->Rect(Clip, CRct(StartX, StartY, StartX + nDarkness, StartY + SizeY), ColorHealth);
	}*/

	/*if (pCD->m_DarknessBarFlashTick)
	{
		fp32 GameTickTime = _pWClient->GetGameTickTime();
		CMTime GameTime = CMTime::CreateFromTicks(pCD->m_GameTick, GameTickTime, pCD->m_PredictFrameFrac);
		fp32 Flash = (GameTime - CMTime::CreateFromTicks(pCD->m_DarknessBarFlashTick, GameTickTime)).GetTime();
		if(Flash > 0.0f && Flash < 0.5)
		{
			int Alpha = RoundToInt((0.5 - Flash) * 255.0f);
			int32 ColorFlash = (pCD->m_DarknessBarFlashColor.m_Value & 0x00ffffff) | (Alpha << 24);
			//_pUtil2D->Rect(Clip, CRct(StartX, StartY + SizeY/2, StartX + Darkness, StartY + SizeY), ColorFlash);
			_pUtil2D->Rect(Clip, CRct(StartX - 1, StartY - 1, StartX + SizeX + 2, StartY + SizeY + 2), ColorFlash);
		}
	}*/

	_pUtil2D->SetCoordinateScale(OrgScale);
}



void CWObject_Character::OnClientRenderHurtEffectAndOverlaySurfaces(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, CRC_Util2D* _pUtil2D)
{
	CWO_Character_ClientData* pCD = CWObject_Character::GetClientData(_pObj);
	M_ASSERT(pCD, "no client data!");

	_pUtil2D->GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	_pUtil2D->SetTexture(0);
	CClipRect Clip(0, 0, 640, 480);

	CWO_GameCameraOverlay GameCameraOverlay;
	CWObject_Message Msg(OBJMSG_GAME_GETOVERLAYSURFACE);
	Msg.m_pData = (void*)&GameCameraOverlay;
	if (!_pWClient->ClientMessage_SendToObject(Msg, _pWClient->Game_GetObjectIndex()))
		GameCameraOverlay.m_Type = -1;

	if (GameCameraOverlay.m_Type == 1)
		RenderSurfaceOverlay(_pWClient, _pEngine, _pUtil2D, Clip, GameCameraOverlay.m_SurfaceID, GameCameraOverlay.m_StartTick, GameCameraOverlay.m_Duration, GameCameraOverlay.m_Type, GameCameraOverlay.m_WH);

//	{
		// Old hit effect was here
//	}

	// Render surface overlay (part2 - stuff that should be rendered after the effect)
	if (GameCameraOverlay.m_Type == 0)
		RenderSurfaceOverlay(_pWClient, _pEngine, _pUtil2D, Clip, GameCameraOverlay.m_SurfaceID, GameCameraOverlay.m_StartTick, GameCameraOverlay.m_Duration, GameCameraOverlay.m_Type, GameCameraOverlay.m_WH);
}
