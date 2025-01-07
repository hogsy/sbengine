#include "PCH.h"

#include "MWinCtrlMod.h"
#include "WClientMod.h"
#include "WClientModWnd.h"
#include "WFrontEndMod.h"
#include "../GameClasses/WObj_Game/WObj_GameMod.h"
#include "../GameClasses/WObj_Char.h"
#include "../../Shared/MOS/Classes/GameWorld/WMapData.h"
#include "../../Shared/MOS/XR/XREngineVar.h"
#include "../../Shared/MOS/Classes/GameContext/WGameContext.h"

#include "../../Shared/MOS/Classes/Win/MWinCtrlGfx.h"
#include "../../Shared/MOS/XR/XREngineImp.h"
#include "../../Shared/MOS/MSystem/Raster/MRender_NVidia.h"
#include "../../../Shared/MOS/MSystem/Misc/MPerfGraph.h"

#include "../Exe/WGameContextMain.h"

#include "MFloat.h"

#ifdef PLATFORM_XBOX1

#include "../../Shared/MOS/RndrXbox/MRndrXbox.h"
#endif

MRTC_IMPLEMENT_DYNAMIC(CWClient_Mod, CWorld_ClientCore);

//#define WCLIENT_NOPREDICTION

#define CONTROLLER_DEFAULT_SENSITIVITYX			90.0f
#define CONTROLLER_DEFAULT_SENSITIVITYY			16.0f
#define CONTROLLER_DEFAULT_DEADZONE				3.0f
#define CONTROLLER_DEFAULT_STARTSPEEDX			0.01f
#define CONTROLLER_DEFAULT_ACCELERATIONX		(2.0f)
#define CONTROLLER_DEFAULT_ACCELERATIONY		(1.5f)
#define CONTROLLER_DEFAULT_HEADSTRAIGHT			0.20f
#define CONTROLLER_DEFAULT_PITCHSTRAIGHT		0.80f
#define CONTROLLER_DEFAULT_ROUGHPOINTSPEED		0.15f
#define CONTROLLER_DEFAULT_ROUGHPOINTPERCENT	0.80f
#define CONTROLLER_MAXVALUE						255.0f

CWClient_Mod::CWClient_Mod()
{
	MAUTOSTRIP(CWClient_Mod_ctor, MAUTOSTRIP_VOID);

	m_OverrideFilter = 0;
	m_Control_Forward = 0;
	m_Control_Backward = 0;
	m_Control_Left = 0;
	m_Control_Right = 0;
	m_Control_Up = 0;
	m_Control_Down = 0;
	m_Control_Look = 0;
	m_Control_LookAdd = 0;
	m_Control_Press = 0;

	m_MPTeam = -1;

#ifdef WADDITIONALCAMERACONTROL
	m_Control_Forward2 = 0.0f;
	m_Control_Backward2 = 0.0f;
	m_Control_Left2 = 0.0f;
	m_Control_Right2 = 0.0f;
	m_Control_Up2 = 0.0f;
	m_Control_Down2 = 0.0f;
	m_Control_LookAdd2 = 0.0f;
	m_Control_Look2 = 0.0f;
	m_Control_Press2 = 0;
	m_Control_Analog2[MAX_ANALOGCHANNELS];
	m_Control_CurrentLookVelocity2 = 0.0f;
	m_Control_TargetLookVelocity2 = 0.0f;
#endif

	m_bRenderBorders = false;
	m_bHideHUD = false;
	m_bShowMultiplayerStatus = false;
	m_bNVViewScale = true;

	m_TextureIDSneak0 = 0;
	m_TextureIDNVPerturb = 0;

	m_Control_CurrentLookVelocity = 0;
//	m_Control_LastLookVelUpdate = 0;

	m_AutoLoadingTextStartTime.MakeInvalid();
	m_AutoLoadingTextLastTime.MakeInvalid();

#ifdef M_Profile
//	m_LastRenderTime = 0;
#endif

	m_EnvBox_iFace = -1;
	m_bForceNoWorld = false;

	m_ShadesLevel = 0;

	m_ShaderDiff = 0x80;
	m_ShaderDiffNV = 0x70;
	m_ShaderSpec = 215;
	m_ShaderSpecNV = 0xff;

	MACRO_GetSystemEnvironment(pEnv);
	m_CamFXMode = pEnv->GetValuei("DARKNESSVISION", 1);
	
	m_InGameGUIWanted = false;

//	ConOutL(CStrF("(CWClient_Mod::CWClient_Mod) this %.8x", this));

	m_DeadZone = CONTROLLER_DEFAULT_DEADZONE;
	m_LookAcceleration[0] = CONTROLLER_DEFAULT_ACCELERATIONX;
	m_LookAcceleration[1] = CONTROLLER_DEFAULT_ACCELERATIONY;
	m_Sensitivity[0] = CONTROLLER_DEFAULT_SENSITIVITYX;
	m_Sensitivity[1] = CONTROLLER_DEFAULT_SENSITIVITYY;
	m_Control_CurrentLookVelocity = 0;
	m_Control_TargetLookVelocity = 0;

	m_bLeanPressed = false;
	m_bWalkPressed = false;

	m_DebugCamera = 0;
	m_DebugCameraMat.Unit();
	m_DebugCameraMovement = 0.0f;
#ifndef M_RTM
	m_DebugCameraSpeedFactor = 1.0f;
#endif
}

CWClient_Mod::~CWClient_Mod()
{
	MAUTOSTRIP(CWClient_Mod_dtor, MAUTOSTRIP_VOID);
	m_spWnd = NULL;
	m_Window = "";
	m_DeferredWindow = "";
}

void CWClient_Mod::Create(CWC_CreateInfo& _CreateInfo)
{
	MAUTOSTRIP(CWClient_Mod_Create, MAUTOSTRIP_VOID);
	CWorld_ClientCore::Create(_CreateInfo);

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys)
		m_bNVViewScale = pSys->GetEnvironment()->GetValuei("CL_NVVIEWSCALE", 1);
}

void CWClient_Mod::OnRegistryChange()
{
	MAUTOSTRIP(CWClient_Mod_OnRegistryChange, MAUTOSTRIP_VOID);
	CWorld_ClientCore::OnRegistryChange();

	const CRegistry* pReg = Registry_Get(WCLIENT_REG_GAMESTATE);
	if (pReg)
	{
		CFStr NewWindow = pReg->GetValue("WINDOW");
		if(NewWindow.Len())
		{
			if(NewWindow != m_DeferredWindow)
			{
					SetInventoryWindowDeferred(NewWindow);
			}
			m_InGameGUIWanted = true;
		}
		else
			m_InGameGUIWanted = false;
		UpdateInGameGui();
		
		/*
			if(NewWindow.Len())
				SetInventoryWindow("mission_objectives"); // force
			else
				SetInventoryWindow(""); // force
		*/

		m_ClientMode &= ~WCLIENT_MODE_GUI;
		if (m_spWnd)
			m_ClientMode |= WCLIENT_MODE_GUI;
	}
	
	UpdateControllerSettings();
}

void CWClient_Mod::UpdateInGameGui()
{
/*	if(m_InGameGUIWanted && m_spWnd == NULL && !m_DeferredWindow.Len())
	{
		M_TRACEALWAYS("UpdateInGameGui:creating window\n");
		ConExecute("cg_cubeside(1)");
		if (m_DeferredWindow.Compare("mission_objectives"))
		{
			SetInventoryWindowDeferred("mission_objectives");
		}
	}
	else */
	if(!m_InGameGUIWanted && m_spWnd != NULL)
	{
		M_TRACEALWAYS("UpdateInGameGui:destroying window\n");
		m_Window = "";
		m_DeferredWindow = "";
		m_spWnd = NULL;
	}
}

void CWClient_Mod::UpdateControllerSettings()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	m_Sensitivity = 1.0f;
	if(pSys && pSys->GetOptions())
	{
		fp32 SensMod = 0.2f+pow(pSys->GetOptions()->GetValuef("CONTROLLER_SENSITIVTY", 0.5f), 1.0f)*4.0f;
		m_Sensitivity[0] = SensMod * 1.2f;
		m_Sensitivity[1] = SensMod;
	#ifdef PLATFORM_WIN
		if(pSys->GetOptions()->GetValuei("CONTROLLER_INVERTYAXIS") == 1)
			m_Sensitivity[1] = -m_Sensitivity[1];
	#else
		if(pSys->GetOptions()->GetValuei("CONTROLLER_INVERTYAXIS") == 0)
			m_Sensitivity[1] = -m_Sensitivity[1];
	#endif
		m_DeadZone = pSys->GetOptions()->GetValuef("CONTROLLER_DEADZONE",CONTROLLER_DEFAULT_DEADZONE);
		m_LookAcceleration[0] = pSys->GetOptions()->GetValuef("CONTROLLER_ACCELERATIONLX",CONTROLLER_DEFAULT_ACCELERATIONX);
		m_LookAcceleration[1] = pSys->GetOptions()->GetValuef("CONTROLLER_ACCELERATIONLY",CONTROLLER_DEFAULT_ACCELERATIONY);

		CStr SendString = CStrF("%f", pSys->GetOptions()->GetValuef("CONTROLLER_AUTOAIM", 0.5f));
		Con_StringCmdForced(CONTROL_UPDATE_CD_AUTOAIMVAL, SendString);
	}
}


int CWClient_Mod::GetSplitScreenMode()
{
	MAUTOSTRIP(CWClient_Mod_GetSplitScreenMode, 0);
	if(CWorld_ClientCore::GetSplitScreenMode() == WCLIENT_SPLITSCREEN_SHARE)
		return WCLIENT_SPLITSCREEN_SHARE;

	CRegistry *pReg = Registry_GetNonConst(WCLIENT_REG_GAMESTATE);
	if (pReg && pReg->GetValuei("NOSPLIT") == 1)
		return WCLIENT_SPLITSCREEN_SHARE;

	return WCLIENT_SPLITSCREEN_REQUIRED;
}

bool CWClient_Mod::CanRenderWorld()
{
	MAUTOSTRIP(CWClient_Mod_CanRenderWorld, false);
	// Returns wether the client has completed the connection sequence and is able to render the game.
	CWObject_CoreData* pObj = Object_GetCD(Player_GetLocalObject());
	if (!pObj)
	{
		const CRegistry *pReg = Registry_Get(WCLIENT_REG_GAMESTATE);
		if(!pReg)
			return false;

		int iForce = pReg->GetValuei("FORCECAMERATARGET");
		return iForce > 0;
	}
	
	bool bInGame = 
		(m_ClientState == WCLIENT_STATE_INGAME || (m_RecMode == 2)) &&
		(m_spMapData != NULL) &&
		m_lspObjects.Len();
	
	//	return true;
	return bInGame;
}

int CWClient_Mod::GetInteractiveState()
{
	MAUTOSTRIP(CWClient_Mod_GetInteractiveState, 0);
	if (!CWorld_ClientCore::GetInteractiveState())
		return 0;

	if (IsFullScreenGUI())
		return 0;

	if (m_WorldName.CompareNoCase("campaign") == 0)
		return 0;

	return 1;
}

void CWClient_Mod::Precache_Init()
{
	MAUTOSTRIP(CWClient_Mod_Precache_Init, MAUTOSTRIP_VOID);
	CWorld_ClientCore::Precache_Init();

	ConOut("Precache Init");

//	m_AutoLoadingTextStartTime = CMTime::GetCPU();
	m_AutoLoadingTextStartTime.Snapshot();
	m_AutoLoadingTextLastTime = m_AutoLoadingTextStartTime;
	m_Control_Press = 0;

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("Precache_Init", "No texture-context available.");
	m_TextureIDSneak0 = pTC->GetTextureID("SNEAKVIEW01");
	pTC->SetTextureParam(m_TextureIDSneak0, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	m_TextureIDNVPerturb = pTC->GetTextureID("SPECIAL_PBNVPerturb");
	pTC->SetTextureParam(m_TextureIDNVPerturb, CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);

	GUI_GetWindowsResource();
}

void CWClient_Mod::World_Close()
{
	MAUTOSTRIP(CWClient_Mod_World_Close, MAUTOSTRIP_VOID);

	m_AutoLoadingTextStartTime.MakeInvalid();
	m_AutoLoadingTextLastTime.MakeInvalid();
	Con_ReleaseAll();

	CWorld_ClientCore::World_Close();
}

void CWClient_Mod::World_Init(int _nObj, int _nRc, CStr _WName, int32 _SpawnMask)
{
	MAUTOSTRIP(CWClient_Mod_World_Init, MAUTOSTRIP_VOID);
	CWorld_ClientCore::World_Init(_nObj, _nRc, _WName, _SpawnMask);

	m_Control_LastLookVelUpdate.Reset();
	m_RenderLoadingCounter = 0;
	m_RenderLoadingCounterTime.Reset();

#ifdef WADDITIONALCAMERACONTROL
	m_Control_LastLookVelUpdate2.Reset();
#endif

	OnRegistryChange();
}

int CWClient_Mod::Net_OnProcessMessage(const CNetMsg& _Msg)
{
	MAUTOSTRIP(CWClient_Mod_Net_OnProcessMessage, 0);
	switch(_Msg.m_MsgType)
	{
	case WPACKET_PLAYSONG :
		{
			int Pos = 0;
			m_Stream_Pending = _Msg.GetStr(Pos);
		}
		break;
	
	case WPACKET_RELEASECONTROLS:
		{
			Con_ReleaseAll();
		}
		break;

	default :
		return CWorld_ClientCore::Net_OnProcessMessage(_Msg);
	};

	return 1;
}

bool CWClient_Mod::Net_UnpackClientRegUpdate(CWObjectUpdateBuffer* _pDeltaBuffer)
{
	MAUTOSTRIP(CWClient_Mod_Net_UnpackClientRegUpdate, false);
	if(CWorld_ClientCore::Net_UnpackClientRegUpdate(_pDeltaBuffer))
	{
		if(m_spWnd)
		{
			CMWnd_Message Msg(WMSG_REGISTRYCHANGED, NULL, 0);
			m_spWnd->OnMessage(&Msg);
		}
		return true;
	}
	else
		return false;
}

CRegistry* CWClient_Mod::GUI_GetWindowsResource()
{
	MAUTOSTRIP(CWClient_Mod_GUI_GetWindowsResource, NULL);
	//CFStr RcName = CFStrF("GUI\\ModInv%d", m_ClientPlayerNr);
	CFStr RcName = CFStrF("GUI\\CubeInv%d", m_ClientPlayerNr);
	int iRc = m_spGUIData->GetResourceIndex_Registry(RcName);
	CRegistry* pRc = m_spGUIData->GetResource_Registry(iRc);
	return pRc;
}

void CWClient_Mod::SetInventoryWindowDeferred(const CStr& _WinID)
{
	m_DeferredWindow = _WinID;
	if (_WinID.CompareSubStr("mission_") == 0)
	{
		ConExecute(CStrF("deferredscriptgrabscreen(\"cg_cubeside(1);cl_invmenu (\\\"%s\\\")\")", _WinID.Str()));
	}
	else
	{
		SetInventoryWindow(_WinID);
//		ConExecute(CStr("deferredscriptgrabscreen(\"cl_invmenu(\\\"%s\\\")\")", _WinID.Str()));
	}
}

void CWClient_Mod::SetInventoryWindow(const CStr& _ID)
{
	MAUTOSTRIP(CWClient_Mod_SetInventoryWindow, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys && pSys->GetEnvironment() && pSys->GetEnvironment()->GetValuei("XWC_NOGUI", 0, 0) != 0)
		return;
		
	m_Window = _ID;
	//M_TRACEALWAYS("SetInventoryWindow: " + m_Window + "\n");

	if (_ID == "")
		m_spWnd = NULL;
	else
	{
		// Create desktop
		CRegistry* pRc = GUI_GetWindowsResource();
		if (pRc)
		{
			spCMWnd spWnd = CMWnd_Create(pRc, _ID);
			if (!spWnd)
			{
				ConOutL("(CWClient_Mod::SetInventoryWindow) Could not create window " + _ID);
				m_Window = "";
				m_DeferredWindow = "";
			}
			else
			{
				//JK-FIX: Do with without TDynamicCast?
				CMWnd_ClientInfo* pCGI = spWnd->GetClientInfo();
				if (pCGI)
				{
					MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
					CWFrontEnd_Mod *pFrontEnd = NULL;

					if(pGame)
						pFrontEnd = TDynamicCast<CWFrontEnd_Mod>((CWFrontEnd*)pGame->m_spFrontEnd);
					pCGI->SetContext(pFrontEnd, this);
				}
				else
					ConOut("(CWClient_Mod::SetInventoryWindow) Window was not an object of class CMWnd_ClientInfo");

				m_spWnd = spWnd;
				m_spWnd->AddFont(m_spGUIData->GetResource_Font(m_spGUIData->GetResourceIndex_Font("TEXT")), "TEXT");
				m_spWnd->AddFont(m_spGUIData->GetResource_Font(m_spGUIData->GetResourceIndex_Font("MONOPRO")), "SYSTEM");
				CMWnd_Message Msg(WMSG_REGISTRYCHANGED, NULL, 0);
				m_spWnd->OnMessage(&Msg);

				Con_ReleaseAll();
			}
		}
	}
}

void CWClient_Mod::EngineClient_EnumerateView(CXR_Engine* _pEngine, int _iVC, int _EnumViewType)
{
	CWorld_ClientCore::EngineClient_EnumerateView(_pEngine, _iVC, _EnumViewType);

	int CamFXMode = m_CamFXMode;
	
	if (_iVC == 0)	// Scene recursion level must be 0 (ie we dont do all this crap in mirrors/portals)
	{
		CXR_SceneGraphInstance* pSGI = World_GetSceneGraphInstance();

		if (!pSGI)
			return;

		if ((CamFXMode & CXR_MODEL_CAMFX_VISIONSENABLED) && _EnumViewType == CXR_ENGINE_EMUMVIEWTYPE_VIEWCLIP)
		{
			// Add darkness vision light
			CWO_CameraEffects NVInfo;
			CWObject_Message Msg(OBJMSG_CHAR_GETNVINTENSITY);
			Msg.m_pData = &NVInfo;
			fp32 Intensity = 0.0f;
			fp32 Perturb = 0.0f;
			fp32 LightIntensity = 0.0f;
			aint Data0 = 0;
			aint Data1 = 0;
			aint Data2 = 0;
			fp32 LightOffsetZ = -100;
			fp32 TSLVZ = -300;
			
			
			if (ClientMessage_SendToObject(Msg, Player_GetLocalObject()) || (CamFXMode & CXR_MODEL_CAMFX_MASK))
			{
				// Render any of the vision effects as fully on
				if(CamFXMode & CXR_MODEL_CAMFX_DARKNESSVISION)
				{
					NVInfo.m_Nightvision = 1.0f;
					NVInfo.m_LightIntensity = 1.0f;
				}
	
				if(CamFXMode & CXR_MODEL_CAMFX_OTHERWORLD)
					NVInfo.m_OtherWorld = 1.0f;

				if(CamFXMode & CXR_MODEL_CAMFX_CREEPINGDARK)
				{
					NVInfo.m_CreepingDark = 1.0f;
					NVInfo.m_LightIntensity = 1.0f;
				}

				LightIntensity = NVInfo.m_LightIntensity;
				Intensity = NVInfo.m_Nightvision;

				// Any effect active?
				if (!AlmostEqual(Intensity, 0.0f, 0.0001f))
					CamFXMode |= CXR_MODEL_CAMFX_DARKNESSVISION;

				if (!AlmostEqual(NVInfo.m_CreepingDark, 0.0f, 0.0001f))
					CamFXMode |= CXR_MODEL_CAMFX_CREEPINGDARK;

				if (!AlmostEqual(NVInfo.m_OtherWorld, 0.0f, 0.0001f))
					CamFXMode |= CXR_MODEL_CAMFX_OTHERWORLD;
				
				uint8 FadeColor = 0;
				const fp32 CreepingDark = NVInfo.m_CreepingDark;
				LightOffsetZ = (fp32)LERP(-100.0f, 0.0f, CreepingDark);
				TSLVZ = (fp32)LERP(-300.0f, 0.0f, CreepingDark);
				FadeColor = uint8(MinMT(255.0f, CreepingDark * 255.0f));
				
				// Engine user color on pulsating creepingdark usable objects
				_pEngine->m_lEngineUserColors[2] = CPixel32::From_fp32(FadeColor*0.079f, FadeColor*0.079f, FadeColor*0.181f, FadeColor);
			}

			// Radial blur parameters
			bool bRadialBlur = (NVInfo.m_RadialBlur > 0.0001f);
			CVec4Dfp32* pRadialBlurFilterParams = NVInfo.m_lRadialBlurFilterParams;
			uint nRadialBlurFilterParams = NVInfo.m_nRadialBlurFilterParams;

			// Setup visions and radial blur
			m_CamFX_Model.PrepareVisions(NVInfo.m_CreepingDark, NVInfo.m_OtherWorld);
			m_CamFX_Model.PrepareRadialBlur(bRadialBlur, NVInfo.m_RadialBlur, NVInfo.m_RadialBlurColorIntensity,
										 NVInfo.m_RadialBlurAffection, NVInfo.m_RadialBlurColorScale, NVInfo.m_RadialBlurCenter,
										 NVInfo.m_UVExtra, NVInfo.m_RadialBlurMode, pRadialBlurFilterParams, nRadialBlurFilterParams,
										 NVInfo.m_pRadialBlurFilter);

			// Add darkness vision model
			CMat4Dfp32 Pos; Pos.Unit();
			CXR_AnimState Anim; Anim.Clear();
			Anim.m_AnimAttr0 = Intensity;
			Anim.m_AnimAttr1 = Perturb;
			Anim.m_AnimTime0 = GetRenderTime();
			Anim.m_Data[0] = Data0;
			Anim.m_Data[1] = Data1;
			Anim.m_Data[2] = Data2;
			Anim.m_Anim0 = CamFXMode;				// Flags for diffrent darkness vision modes
			
			_pEngine->Render_AddModel(&m_CamFX_Model, Pos, Anim, XR_MODEL_STANDARD);
		}
		else if (_EnumViewType == CXR_ENGINE_EMUMVIEWTYPE_VIEWCLIP)
		{
			CMat4Dfp32 Pos; Pos.Unit();
			CXR_AnimState Anim; Anim.Clear();
			Anim.m_AnimTime0 = GetRenderTime();
			Anim.m_Anim0 = 0;

			m_CamFX_Model.PrepareSimple();
			_pEngine->Render_AddModel(&m_CamFX_Model, Pos, Anim, XR_MODEL_STANDARD);
		}
	}
}

int CWClient_Mod::IsFullScreenGUI()
{
	MAUTOSTRIP(CWClient_Mod_IsFullScreenGUI, 0);
	return (m_spWnd != NULL);
}

int CWClient_Mod::IsCutscene()
{
	MAUTOSTRIP(CWClient_Mod_IsCutscene, 0);

	CWObject_Client* pObj = Object_Get(Player_GetLocalObject());
	if(pObj && (pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE))
		return true;

	return false;
}

// #pragma optimize("",off)
// #pragma inline_depth(0)

void CWClient_Mod::Render(CXR_VBManager* _pVBM, CRenderContext* _pRender, CRC_Viewport& _GUIVP, fp32 _InterpolationTime, int _Context)
{
	MAUTOSTRIP(CWClient_Mod_Render, MAUTOSTRIP_VOID);

	{
		//
		// Aiming Help for thumbsticks
		//
		// n should be read from the registry
		//

		fp32 dX = m_Control_TargetLookVelocity[0];
		fp32 dY = m_Control_TargetLookVelocity[1];

		bool bMaxHeadingSpeed = true;
		if(M_Fabs(dX) < 255.0f*(CONTROLLER_DEFAULT_PITCHSTRAIGHT))
			bMaxHeadingSpeed = false;

		/********************************************************
		* The target velocity should be following a x5 curve. 
		* This implies that you have a lot of space for small
		* adjustments close to the center. While the outer
		* perimeter of the controller renders in rough movements
		*********************************************************/

/*		if(M_Fabs(dX) < m_DeadZone && M_Fabs(dY) < m_DeadZone)
		{
			dX = 0;
			dY = 0;
		}
		else
		{
			if(dX > 0)
				dX -= m_DeadZone;		
			else
				dX += m_DeadZone;

			if(dY > 0)
				dY -= m_DeadZone;
			else
				dY += m_DeadZone;
		}*/

		dX = dX * (1.0f/CONTROLLER_MAXVALUE);
		dY = dY * (1.0f/CONTROLLER_MAXVALUE);

		fp32 SignX = Sign(dX);
/*		if(bMaxHeadingSpeed)
		{
			CVec2Dfp32 Dir(dX,dY);
			dX = Dir.Length();
		}

		dX = M_Fabs(dX);
		if(dX > CONTROLLER_DEFAULT_ROUGHPOINTPERCENT)
			dX = CONTROLLER_DEFAULT_ROUGHPOINTSPEED + (1.0f - CONTROLLER_DEFAULT_ROUGHPOINTSPEED)*((dX - CONTROLLER_DEFAULT_ROUGHPOINTPERCENT)/(1.0f - CONTROLLER_DEFAULT_ROUGHPOINTPERCENT));
		else
			dX = CONTROLLER_DEFAULT_ROUGHPOINTSPEED * (dX/CONTROLLER_DEFAULT_ROUGHPOINTPERCENT);

		dX *= SignX;
*/
//		M_TRACEALWAYS("LookVelocity %f, %f => %f, %f\n", m_Control_TargetLookVelocity[0], m_Control_TargetLookVelocity[1], dX, dY);

		CFStr Event = CFStrF("Look %.3f, %.3f", m_Control_TargetLookVelocity[0], m_Control_TargetLookVelocity[1]);
		M_NAMEDEVENT(Event.GetStr(), 0xffffffff);

		CVec2Dfp32 TargetLookVelocity(dX, dY);

		CMTime Time = GetTime();
		fp32 dTime = (Time - m_LastLookUpdateTime).GetTime();
		dTime = Clamp01(dTime) * m_TimeScale;

		m_LastLookUpdateTime = Time;
		CRC_Viewport CurVP;
		Render_GetLastRenderViewport(CurVP);		
		fp32 ZoomLevel = CurVP.GetFOV() / 90.0f;
		int iPlayer = Player_GetLocalObject();

		fp32 AutoAimScale = 1.0f;
		if(ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_HASAUTOAIMTARGET), iPlayer))
			AutoAimScale = 0.5f;

		CVec2Dfp32 VelNoSens = m_Control_CurrentLookVelocity;
		VelNoSens[0] /= /*m_Sensitivity[0] * */ZoomLevel*AutoAimScale;
		VelNoSens[1] /= /*m_Sensitivity[1] * */ZoomLevel*AutoAimScale;

		if(VelNoSens.DistanceSqr(TargetLookVelocity) > 0.0001f*0.0001f)
		{
			for(int i = 0; i < 2 ; ++i)
			{
				fp32 AccMod = Max(M_Sqrt(M_Fabs(TargetLookVelocity[i])), 0.1f);

				fp32 TargetVel = TargetLookVelocity[i] * /*m_Sensitivity[i] * */ZoomLevel*AutoAimScale;
				fp32 CurVel = m_Control_CurrentLookVelocity[i];
				if((CurVel > TargetVel && CurVel >0)||
					(CurVel < TargetVel && CurVel <0))
				{
					//Deacceleration is immediate to the lesser velocity. If switching directions the new velocity will start at 0
					if(i== 0 && CurVel * TargetVel < 0)
						CurVel = Sign(TargetVel) * Min(CONTROLLER_DEFAULT_STARTSPEEDX,M_Fabs(TargetVel));
					else
						CurVel = TargetVel;
				}
				else 
				{
					fp32 Diff1 = TargetVel - CurVel;
					if(i == 0)
					{
						if(CurVel < TargetVel)
							CurVel = Max(CONTROLLER_DEFAULT_STARTSPEEDX,CurVel) + AccMod * m_LookAcceleration[i] * dTime;
						else
							CurVel = Min(-CONTROLLER_DEFAULT_STARTSPEEDX,CurVel) - AccMod * m_LookAcceleration[i] * dTime;
					}
					else
					{
						if(CurVel < TargetVel)
							CurVel += AccMod * m_LookAcceleration[i] * dTime;
						else
							CurVel -= AccMod * m_LookAcceleration[i] * dTime;
					}
					fp32 Diff2 = TargetVel - CurVel;

					// Don't let the target velocity "pass" the current velocity
					if(Diff1 * Diff2 <= 0)
						CurVel = TargetVel;
				}
				m_Control_CurrentLookVelocity[i] = CurVel;
			}
	
			Net_UpdateLook(1.0f / 120.0f);
		}
#ifdef WADDITIONALCAMERACONTROL
		RenderAdditional();
#endif


	
		// Update HeadLightRange from gametype
		m_ForcedHeadLightRange = ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAMEP4_GETNIGHTVISIONLIGHTRANGE), Game_GetObjectIndex());

		CWorld_ClientCore::Render(_pVBM, _pRender, _GUIVP, _InterpolationTime, _Context);
	}
}

void CWClient_Mod::Render_GUI(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP)
{
	MAUTOSTRIP(CWClient_Mod_Render_GUI, MAUTOSTRIP_VOID);
	m_3DViewPort = _3DVP;

	Render_Borders(_pVBM, _pRC, _3DVP, *_pVBM->Viewport_Get());

	if (m_EnvBox_iFace >= 0)
		EnvBox_CaptureFrame(_pVBM);

	CWorld_ClientCore::Render_GUI(_pVBM, _pRC, _3DVP);

	
	bool bForceGrabScreen = false;
	
	// need to grab due to darkness voice effect?
	
	CWObject_Client *pObj = Object_Get(Player_GetLocalObject());
	CWO_Character_ClientData* pCD = pObj ? safe_cast<CWO_Character_ClientData>((CReferenceCount*) pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA]) : NULL;
	
	{
		MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT"); 
		if(pGame)
		{
			// need to grab due to Tentacle effects in gui?
			CWFrontEnd_Mod *pFrontEnd = TDynamicCast<CWFrontEnd_Mod>((CWFrontEnd*)pGame->m_spFrontEnd);
			
			if(!(pFrontEnd->m_Cube.m_GUIFlags & CCube::GUI_FLAGS_EXITCOMPLETE))
				m_bHideHUD = true;
			else
				m_bHideHUD = false;


			if(pFrontEnd && pFrontEnd->m_Cube.GetOkToGrabScreen())
			{
				pFrontEnd->m_Cube.SetScreenIsGrabbed();
				bForceGrabScreen = true;
			}
		}
	}

	int iGameObj = Game_GetObjectIndex();
	bool bCanRenderWorld = ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_NEVERRENDERWORLD), iGameObj) == 0;			

	if (bCanRenderWorld)
	{
		CXR_Engine* pEngine = GetEngine();

		CXR_Engine_PostProcessParams PPP;
		PPP = pEngine->m_PostProcessParams_Current;
		PPP.m_ViewFlags = m_ViewFlags;
		PPP.m_AspectChange = m_AspectChange;

		if ((PPP.m_DebugFlags & XR_PPDEBUGFLAGS_NOOVERRIDE) == 0)
		{
			CWObject_Message Msg(OBJMSG_CHAR_POSTPROCESSOVERRIDE);
			Msg.m_pData = &PPP;
			ClientMessage_SendToObject(Msg, Player_GetLocalObject());
		}

		GetEngine()->Engine_PostProcess(_pVBM, _3DVP, &PPP);
	}

	if(!m_bHideHUD)
		RenderStatusBar(_pVBM, _pRC, _3DVP);

	if(m_bShowMultiplayerStatus)
		RenderMultiplayerStatus(_pVBM, _pRC, _3DVP);

	while(bForceGrabScreen)
	{
		// This need to be done last. Stored for GUI-Background processing
		CTextureContainer_Screen* pTCScreen = safe_cast<CTextureContainer_Screen>(GetEngine()->GetInterface(XR_ENGINE_TCSCREEN));
		if (!pTCScreen)
			break;

		_pVBM->ScopeBegin("CWClient_Mod::Render_GUI::bForceGrabScreen", false);
		uint TextureID_BlurScreen = pTCScreen->GetTextureID(2);	// Deferred diffuse
		CRct VPRect = _pVBM->Viewport_Get()->GetViewRect();
		_pVBM->AddCopyToTexture(0, VPRect, VPRect.p0, TextureID_BlurScreen, false);
		_pVBM->ScopeEnd();
		break;
	}
}

bool CWClient_Mod::HideHud()
{
	return m_bHideHUD != 0;
}

//
// Renders borders around the view, both the widescreen-border 
// and the cutscene-border.
//
void CWClient_Mod::Render_Borders(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP)
{
	MAUTOSTRIP(CWClient_Mod_Render_Borders, MAUTOSTRIP_VOID);
//	int XBorder=0, YBorder=0;
	
	CRC_Viewport VP = _3DVP;
	VP.SetAspectRatio(1.0);
	
	CXR_VBManager* pVBM = _pVBM;
	CRenderContext* pRC = _pRC;

	int RenderSkipText = 0;
	uint32 Fade = ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETSCREENFADE, aint(&RenderSkipText)), Game_GetObjectIndex());
	if((Fade & 0xff000000) != 0)
	{		
		VP.SetFOV(90);
		VP.SetAspectRatio(1.0);
		CClipRect Clip(0, 0, 640, 480);
		
		if (pVBM->Viewport_Push(&VP))
		{
			pVBM->ScopeBegin("CWClient_Mod::Render_Borders", false);
			m_Util2D.Begin(pRC, pVBM->Viewport_Get(), pVBM);
			m_Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE | CRC_FLAGS_CULL);
			m_Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			CRct Rect = pVBM->Viewport_Get()->GetViewRect();
			m_Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetWidth() / 640.0f , Rect.GetHeight() / 480.0f));
			m_Util2D.Rect(Clip, CRct(0, 0, 640, 480), Fade);
			m_Util2D.End();
			pVBM->ScopeEnd();

			pVBM->Viewport_Pop();
		}
	}

	if(RenderSkipText)
	{
		CClipRect Clip(0, 0, 640, 480);
		CRC_Font *pF = m_spGUIData->GetResource_Font(m_spGUIData->GetResourceIndex_Font("TEXT"));
		if(pF)
		{
			int Dots = int(CMTime::GetCPU().GetTimeModulus(3));
			CFStr Text = "§Z16§LMENU_SKIPPING§pq";
			for(int i = 0; i < Dots; i++)
				Text += ".";
			wchar Buf[1024];
			Localize_Str(Text.Str(), Buf, 1024);

			if (pVBM->Viewport_Push(&VP))
			{
				pVBM->ScopeBegin("Skipping Text", false);
				m_Util2D.Begin(pRC, pVBM->Viewport_Get(), pVBM);
				m_Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE | CRC_FLAGS_CULL);
				m_Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
				CRct Rect = pVBM->Viewport_Get()->GetViewRect();
				m_Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetWidth() / 640.0f , Rect.GetHeight() / 480.0f));

				m_Util2D.Text(Clip, pF, 500, 400, Buf, 0xff808080, -1);

				m_Util2D.End();
				pVBM->ScopeEnd();
				pVBM->Viewport_Pop();
			}
		}
	}

	int SoundFade = ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETSOUNDFADE), Game_GetObjectIndex());
	Sound_SetGameMasterVolume(1.0f - fp32(SoundFade) / 255);
}

CVec2Dfp32 CWClient_Mod::Render_GetViewScaleMultiplier()
{
	if (m_bNVViewScale)
	{
		fp32 Scale = Clamp01(m_ShadesLevel * 10.0f);
		return CVec2Dfp32(LERP(1.0f, 0.75f, Scale));
	}
	else
		return CVec2Dfp32(1, 1);
}

bool CWClient_Mod::Render_World_AllowTextureLoad()
{
	MAUTOSTRIP(CWClient_Mod_Render_World_AllowTextureLoad, false);
	return (m_WorldName == "CAMPAIGN") != 0;
}

void CWClient_Mod::Render_SetViewport(CXR_VBManager* _pVBM)
{
	MAUTOSTRIP(CWClient_Mod_Render_SetViewport, MAUTOSTRIP_VOID);
	if (m_EnvBox_iFace >= 0)
		EnvBox_SetViewPort(_pVBM);
	else
		CWorld_ClientCore::Render_SetViewport(_pVBM);
}

void CWClient_Mod::Render_GetCamera(CMat4Dfp32& _Camera)
{
	MAUTOSTRIP(CWClient_Mod_Render_GetCamera, MAUTOSTRIP_VOID);
	MSCOPE(CWClient_Mod::Render_GetCamera, WCLIENTMOD);

	// Update look input before we evaluate the camera. This if thingy avoids some CCmd spamming when
	// we're not really in the game and won't benefit from the look improvement.
	if (m_Precache == PRECACHE_DONE &&
		m_ClientState == WCLIENT_STATE_INGAME)
	{
		Net_UpdateLook(1.0f / 120.0f);
#ifdef WADDITIONALCAMERACONTROL
		Net_UpdateLookAdditional(1.0f / 120.0f);
#endif
	}

	if (m_EnvBox_iFace >= 0)
		EnvBox_GetCamera(_Camera);
	else
	{
		const CRegistry *pReg = Registry_Get(WCLIENT_REG_GAMESTATE);
		if(pReg)
		{
			int iForce = pReg->GetValuei("FORCECAMERATARGET");
			if(iForce > 0)
			{
				CWObject_Message Msg(OBJSYSMSG_GETCAMERA);
				Msg.m_pData = &_Camera;
				Msg.m_DataSize = sizeof(_Camera);
				
				_Camera.Unit();
				
				if (ClientMessage_SendToObject(Msg, iForce))
				{
					CWObject_CoreData* pObj = Object_GetCD(iForce);
					if(pObj)
					{
						// Object did not respond: Interpolate direction and position for player-object.
						Interpolate2(pObj->GetLastPositionMatrix(), pObj->GetPositionMatrix(), _Camera, Min(1.0f, m_RenderTickFrac) );
						
						// Do some camera rotation. Object's forward vector is the matrix's x-axis, but for rendering the forward-vector needs to be z
						_Camera.RotX_x_M(-0.25f);
						_Camera.RotY_x_M(0.25f);

						// Players needs to be requested a camera to function properly...
						CMat4Dfp32 Dummy;
						CWorld_ClientCore::Render_GetCamera(Dummy);
						
						goto end;
					}
				}
			}
		}
		CWorld_ClientCore::Render_GetCamera(_Camera);
	}
end:
	if (m_DebugCamera != 0)
	{
		_Camera = m_DebugCameraMat;
		_Camera.RotX_x_M(-0.25f);
		_Camera.RotY_x_M(0.25f);
	}
}

void CWClient_Mod::SB_RenderInventory()
{
	MAUTOSTRIP(CWClient_Mod_SB_RenderInventory, MAUTOSTRIP_VOID);
	MSCOPE(CWClient_Mod::SB_RenderInventory, WCLIENTMOD);

	CWFrontEnd_Mod *pFrontEndMod = NULL;
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	if(pGame)
		pFrontEndMod = TDynamicCast<CWFrontEnd_Mod>((CWFrontEnd*)pGame->m_spFrontEnd); // UGLY

	if(pFrontEndMod)
	{
		CClipRect Clip(0,0,640,480);

		if(m_spWnd != NULL)
		{
			MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
			if (!pSys) return;
			CPnt MousePos = (pSys->m_spInput) ? pSys->m_spInput->GetMousePosition() : CPnt(0,0);

 			TPtr<CMWnd> spWndMouseOver = m_spWnd->GetCursorOverWnd(MousePos, false);
			
			int OldStatus = 0;
			if (spWndMouseOver != NULL)
			{
				OldStatus = spWndMouseOver->m_Status;
				spWndMouseOver->m_Status |= WSTATUS_MOUSEOVER;
			}
//			CMWnd_ModInvObject::ms_Time = GetRenderTime();

			m_spWnd->RenderTree(&m_Util2D, Clip);

			// Check if outside button has been pressed
			if(m_spWnd && m_spWnd->m_lOutsideButtons.Len() != 0)
			{
				pFrontEndMod->m_ClientMouse = true;
				pFrontEndMod->RenderHelpButtons(m_spWnd, &m_Util2D, Clip);

				MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
				if (!pSys) return;
				int MouseBtn = (pSys->m_spInput) ? pSys->m_spInput->GetMouseButtons() : 0;
				static int LastMouseBtn = 0;

				if((MouseBtn&1) && !(LastMouseBtn&1))
					pFrontEndMod->Cube_MouseClick(m_spWnd);

				LastMouseBtn = MouseBtn;
			}
			else
				pFrontEndMod->m_ClientMouse = false;

			if (spWndMouseOver != NULL) spWndMouseOver->m_Status = OldStatus;
		}
		else
			pFrontEndMod->m_ClientMouse = false;

		PostRenderInterface(m_spWnd, &m_Util2D, Clip);
	}
}

void CWClient_Mod::PostRenderInterface(CMWnd* pWndTree, CRC_Util2D* _pRCUtil, CClipRect _Clip)
{
	MAUTOSTRIP(CWClient_Mod_PostRenderInterface, MAUTOSTRIP_VOID);
	MSCOPE(CWClient_Mod::PostRenderInterface, WCLIENTMOD);
//	CMapData * MapData = (CMapData*)m_spGUIData;

	if(m_InfoScreen != "")
	{
		CClipRect Clip(0,0,640,480);
		_pRCUtil->SetTextureScale(512.0f / 640.0f, 512.0f / 480.0f);
		_pRCUtil->SetTexture(m_InfoScreen);
		_pRCUtil->SetTextureOrigo(Clip, CPnt(0, 0));
		_pRCUtil->Rect(Clip, CRct(0, 0, 640, 480), 0xff808080);
	}

	/*
	// Plot A&B descriptors

	if (MapData)
	{
		// Get core registry

		int iRc = MapData->GetResourceIndex_Registry("GUI\\ModWnd");
		
		CRegistry* pRc = MapData->GetResource_Registry(iRc);
		
		const CRegistry *pMainReg = NULL;
		const CRegistry *pTempReg = NULL;

		if (pRc)
			pMainReg = pRc->Find("PAD_BUTTON_DESCRIPTOR");

		if (pMainReg)
		{
			// AICONSURFACE

			CStr aIconSurface;

			pTempReg = pMainReg->Find("AICONSURFACE");
			
			if (pTempReg)
				aIconSurface = pTempReg->GetThisValue();

			// BICONSURFACE

			CStr bIconSurface;

			pTempReg = pMainReg->Find("BICONSURFACE");
			
			if (pTempReg)
				bIconSurface = pTempReg->GetThisValue();
	
			// AICONPOSITION

			pTempReg = pMainReg->Find("AICONPOSITION");

			CPnt AIconPos;

			if (pTempReg)
			{
				CStr Pos = pTempReg->GetThisValue();
				AIconPos.x = Pos.GetStrSep(",").Val_int();
				AIconPos.y = Pos.GetStrSep(",").Val_int();
			}
			
			// BICONPOSITION

			pTempReg = pMainReg->Find("BICONPOSITION");

			CPnt BIconPos;

			if (pTempReg)
			{
				CStr Pos = pTempReg->GetThisValue();
				BIconPos.x = Pos.GetStrSep(",").Val_int();
				BIconPos.y = Pos.GetStrSep(",").Val_int();
			}

			// ATEXTCONFIG

			CStr aTEXTCONFIG;

			pTempReg = pMainReg->Find("ATEXTCONFIG");
			
			if (pTempReg)
				aTEXTCONFIG = pTempReg->GetThisValue();
	
			// BTEXTCONFIG

			CStr bTEXTCONFIG;

			pTempReg = pMainReg->Find("BTEXTCONFIG");
			
			if (pTempReg)
				bTEXTCONFIG = pTempReg->GetThisValue();
	
			// ATEXTPOSITION

			pTempReg = pMainReg->Find("ATEXTPOSITION");

			CPnt ATEXTPos;

			if (pTempReg)
			{
				CStr Pos = pTempReg->GetThisValue();
				ATEXTPos.x = Pos.GetStrSep(",").Val_int();
				ATEXTPos.y = Pos.GetStrSep(",").Val_int();
			}
			
			// BTEXTPOSITION

			pTempReg = pMainReg->Find("BTEXTPOSITION");

			CPnt BTEXTPos;

			if (pTempReg)
			{
				CStr Pos = pTempReg->GetThisValue();
				BTEXTPos.x = Pos.GetStrSep(",").Val_int();
				BTEXTPos.y = Pos.GetStrSep(",").Val_int();
			}

			int xyadj = 35;

			// Plot if focused
				
			if (CMWnd* pFocusWnd = pWndTree->GetFocusWnd())
			{
				_pRCUtil->GetRC()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
				
				// Text
				
				CRC_Font* pF = pWndTree->GetFont("TEXT");
				
				// Text
				
				if (pF) 
				{
					int Style = WSTYLE_TEXT_CENTERY | WSTYLE_TEXT_WORDWRAP;
					
					int TextColorM = 0xffdddddd;
					int TextColorH = 0x60ffffff;
					int TextColorD = 0x60000000;						
					
					if (pFocusWnd->m_AButtonDescriptor.Len())
					{
						CStr Text = aTEXTCONFIG+pFocusWnd->m_AButtonDescriptor;

						wchar LocText[1024];
						Localize_Str(Text, LocText, 1023);

						fp32 Width = pF->GetWidth(CStrBase::StrLen(LocText), LocText);

						CMWnd_Text_DrawFormated(
							_pRCUtil, 
							_Clip, 
							pF, 
							aTEXTCONFIG+pFocusWnd->m_AButtonDescriptor, 
							ATEXTPos.x-Width, 
							ATEXTPos.y+pFocusWnd->m_iButtonDescriptorVCorrection,
							Style, 
							TextColorM, TextColorH, TextColorD, 
							640, 
							50);
					}
					
					if (pFocusWnd->m_XButtonDescriptor.Len())
					{
						CStr Text = aTEXTCONFIG+pFocusWnd->m_XButtonDescriptor;

						wchar LocText[1024];
						Localize_Str(Text, LocText, 1023);

						fp32 Width = pF->GetWidth(CStrBase::StrLen(LocText), LocText);

						CMWnd_Text_DrawFormated(
							_pRCUtil, 
							_Clip, 
							pF, 
							aTEXTCONFIG+pFocusWnd->m_XButtonDescriptor, 
							ATEXTPos.x-Width, 
							ATEXTPos.y-xyadj+pFocusWnd->m_iButtonDescriptorVCorrection,
							Style, 
							TextColorM, TextColorH, TextColorD, 
							640, 
							50);
					}
					
					if (pFocusWnd->m_BButtonDescriptor.Len())
					{
						CMWnd_Text_DrawFormated(
							_pRCUtil, 
							_Clip, 
							pF, 
							bTEXTCONFIG+pFocusWnd->m_BButtonDescriptor, 
							BTEXTPos.x, 
							BTEXTPos.y+pFocusWnd->m_iButtonDescriptorVCorrection,
							Style, 
							TextColorM, TextColorH, TextColorD, 
							640, 
							50);
					}
					
					if (pFocusWnd->m_YButtonDescriptor.Len())
					{
						CMWnd_Text_DrawFormated(
							_pRCUtil, 
							_Clip, 
							pF, 
							bTEXTCONFIG+pFocusWnd->m_YButtonDescriptor, 
							BTEXTPos.x, 
							BTEXTPos.y-xyadj+pFocusWnd->m_iButtonDescriptorVCorrection,
							Style, 
							TextColorM, TextColorH, TextColorD, 
							640, 
							50);
					}
				}
				
				if (pFocusWnd->m_AButtonDescriptor.Len())
				{
					// Icon
					
					_pRCUtil->SetTexture(_pRCUtil->GetRC()->Texture_GetTC()->GetTextureID(aIconSurface));
					_pRCUtil->SetTextureScale(1.0f, 1.0f);
					_pRCUtil->SetTextureOrigo(_Clip, CPnt(AIconPos.x, AIconPos.y+pFocusWnd->m_iButtonDescriptorVCorrection));
					_pRCUtil->Rect(_Clip, CRct(AIconPos.x, AIconPos.y+pFocusWnd->m_iButtonDescriptorVCorrection, AIconPos.x+32, AIconPos.y+32+pFocusWnd->m_iButtonDescriptorVCorrection), 0xff7f7f7f);
				}
				
				if (pFocusWnd->m_XButtonDescriptor.Len())
				{
					// Icon
					
					_pRCUtil->SetTexture(_pRCUtil->GetRC()->Texture_GetTC()->GetTextureID("GUI_Button_X_32"));
					_pRCUtil->SetTextureScale(1.0f, 1.0f);
					_pRCUtil->SetTextureOrigo(_Clip, CPnt(AIconPos.x, AIconPos.y-xyadj+pFocusWnd->m_iButtonDescriptorVCorrection));
					_pRCUtil->Rect(_Clip, CRct(AIconPos.x, AIconPos.y-xyadj+pFocusWnd->m_iButtonDescriptorVCorrection, AIconPos.x+32, AIconPos.y+32-xyadj+pFocusWnd->m_iButtonDescriptorVCorrection), 0xff7f7f7f);
				}
				
				if (pFocusWnd->m_BButtonDescriptor.Len())
				{
					// Icon
					
					_pRCUtil->SetTexture(_pRCUtil->GetRC()->Texture_GetTC()->GetTextureID(bIconSurface));
					_pRCUtil->SetTextureScale(1.0f, 1.0f);
					_pRCUtil->SetTextureOrigo(_Clip, CPnt(BIconPos.x, BIconPos.y+pFocusWnd->m_iButtonDescriptorVCorrection));
					_pRCUtil->Rect(_Clip, CRct(BIconPos.x, BIconPos.y+pFocusWnd->m_iButtonDescriptorVCorrection, BIconPos.x+32, BIconPos.y+32+pFocusWnd->m_iButtonDescriptorVCorrection), 0xff7f7f7f);
				}
				
				if (pFocusWnd->m_YButtonDescriptor.Len())
				{
					// Icon
					
					_pRCUtil->SetTexture(_pRCUtil->GetRC()->Texture_GetTC()->GetTextureID("GUI_Button_Y_32"));
					_pRCUtil->SetTextureScale(1.0f, 1.0f);
					_pRCUtil->SetTextureOrigo(_Clip, CPnt(BIconPos.x, BIconPos.y-xyadj+pFocusWnd->m_iButtonDescriptorVCorrection));
					_pRCUtil->Rect(_Clip, CRct(BIconPos.x, BIconPos.y-xyadj+pFocusWnd->m_iButtonDescriptorVCorrection, BIconPos.x+32, BIconPos.y+32-xyadj+pFocusWnd->m_iButtonDescriptorVCorrection), 0xff7f7f7f);
				}
			}
		}
	}
	*/
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_RenderCallback
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CTextureContainer_DOFTranslate : public CTextureContainer_Render
{
public:
	CTextureContainer_DOFTranslate()
	{
		m_MidStart = -1;
		m_MidEnd = -1;
	}

	fp32 m_MidStart;
	fp32 m_MidEnd;
	fp32 m_AmountStart;
	fp32 m_AmountEnd;

	void SetParams(CRenderContext *_pRC, fp32 _MidStart, fp32 _MidEnd, fp32 _AmountStart, fp32 _AmountEnd)
	{
		MAUTOSTRIP(CTextureContainer_Screen_GetSnapshot, MAUTOSTRIP_VOID);

		if (_MidStart != m_MidStart || _MidEnd != m_MidEnd ||
			_AmountStart != m_AmountStart || _AmountEnd != m_AmountEnd)
		{
			m_MidStart = _MidStart;
			m_MidEnd = _MidEnd;
			m_AmountStart = _AmountStart;
			m_AmountEnd = _AmountEnd;

			int TextureID = m_lTxtInfo[0].m_TextureID;
			
			if(_pRC)
			{
				_pRC->Texture_GetTC()->MakeDirty(TextureID);
				_pRC->Texture_Precache(TextureID);
			}
			else if(m_pEngine)
			{
				m_pEngine->GetTC()->MakeDirty(TextureID);
				m_pEngine->m_pRender->Texture_Precache(TextureID);
			}
		}
	}

	void RenderQuad(class CRenderContext* _pRC, fp32 _StartY, fp32 _EndY, fp32 _Color0, fp32 _Color1)
	{ 
		_pRC->Matrix_SetUnit();

		const fp32 _ZValue = 0.99f;

		CRC_Viewport VP = *_pRC->Viewport_Get();
		const CVec3Dfp32* pVV = VP.GetViewVertices();
		M_ASSERT(VP.GetNumViewVertices() == 5, "!");

		CVec3Dfp32 lVerts[4];
		CPixel32 lColors[4];

		lColors[0] = CPixel32(CVec4Dfp32(_Color1*255.0f));
		lColors[1] = CPixel32(CVec4Dfp32(_Color0*255.0f));
		lColors[2] = CPixel32(CVec4Dfp32(_Color0*255.0f));
		lColors[3] = CPixel32(CVec4Dfp32(_Color1*255.0f));

		for(int i = 0; i < 4; i++)
		{
			pVV[i+1].Scale(_ZValue, lVerts[i]);
		}
		fp32 Width = lVerts[0].k[0] - lVerts[1].k[0];
//		fp32 Height = lVerts[0].k[1] - lVerts[2].k[1];
		fp32 Left = lVerts[1].k[0];
		lVerts[0].k[0] = Left + Width * _EndY;   // Bottom Right
		lVerts[1].k[0] = Left + Width * _StartY; // Bottom Left
		lVerts[2].k[0] = Left + Width * _StartY; // Top Left
		lVerts[3].k[0] = Left + Width * _EndY;   // Top Right

		_pRC->Geometry_Clear();
		_pRC->Geometry_VertexArray(lVerts, 4);
		_pRC->Geometry_ColorArray(lColors);
		uint16 lTriPrim[6] = { 0,1,2,0,2,3 };
		_pRC->Render_IndexedTriangles(lTriPrim, 2);
		_pRC->Geometry_Clear();
	}

	virtual void BuildInto(int _iLocal, class CRenderContext* _pRC)
	{
		MSCOPESHORT(CTextureContainer_DOFTranslate::BuildInto);

		_pRC->Attrib_Push();

		int w = m_lTxtInfo[_iLocal].m_Width;
		int h = m_lTxtInfo[_iLocal].m_Height;

		CRC_Attributes Attrib;
		CRC_Attributes* pAttrib = &Attrib;
		pAttrib->SetDefault();
		pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);
		pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL);

		CRC_Viewport VP = *_pRC->Viewport_Get();
		VP.SetView(VP.GetViewClip(), CRct(0,0,w,h));
		_pRC->Viewport_Push();
		_pRC->Viewport_Set(&VP);

		pAttrib->Attrib_Disable(CRC_FLAGS_CULLCW);
		_pRC->Attrib_Set(*pAttrib); 

		{
			CPixel32 Color = 0xffffffff;
			RenderQuad(_pRC, 0, m_MidStart, m_AmountStart, 0.0);
			RenderQuad(_pRC, m_MidStart, m_MidEnd, 0.0, 0.0);
			RenderQuad(_pRC, m_MidEnd, 1.0, 0.0, m_AmountEnd);
		}

		_pRC->Viewport_Pop();

		_pRC->Attrib_Pop();
	}
};


uint32 CWClient_Mod::GetViewFlags()
{
	CWObject_Client *pObj = Object_Get(Player_GetLocalObject());

	if(pObj)
	{
		return XR_VIEWFLAGS_WIDESCREEN;
/*
		int bCutscene = pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_CUTSCENE;
		int bDialogue = pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_DIALOGUE;

		if((bDialogue || bCutscene) && Phys_Message_SendToObject(CWObject_Message(OBJMSG_CHAR_CSHASBORDER), pObj->m_iObject))
			return EViewFlags_VirtualWideScreen;
*/
	}
	return 0;
}

/**** Debug Camera ****/
bool CWClient_Mod::UpdateDebugCamera_Look(const CVec2Dfp32& _dLook)
{
	if (m_DebugCamera != 1)
		return false;

	CMat4Dfp32 dRot, Temp, BaseMat = m_DebugCameraMat;
	BaseMat.GetRow(2) = CVec3Dfp32(0,0,1);
	BaseMat.RecreateMatrix(2, 0);

	// Look
	fp32 CosAngle = m_DebugCameraMat.GetRow(0) * BaseMat.GetRow(0);
	CosAngle = Sign(CosAngle) * Clamp01(CosAngle); // Safety precaution
	fp32 Angle = M_ACos(CosAngle) * (1.0f / _PI2) * Sign(m_DebugCameraMat.GetRow(0) * BaseMat.GetRow(2));
	fp32 NewAngle = Min(0.2f, Max(-0.2f, Angle + _dLook.k[1]));
	fp32 dAngle = NewAngle - Angle;
	dRot.Unit();
	dRot.M_x_RotY(dAngle);
	dRot.Multiply(m_DebugCameraMat, Temp);		// Rotate around local Y-axis

	CQuatfp32(CVec3Dfp32(0,0,1), -_dLook.k[0]).SetMatrix(dRot);
	Temp.Multiply(dRot, m_DebugCameraMat);			// Rotate around gravity Z-axis

	m_DebugCameraMat.GetRow(3) = BaseMat.GetRow(3);
	return true;
}


bool CWClient_Mod::UpdateDebugCamera_Move(uint8 _iComponent, fp32 _Amount)
{
	if (m_DebugCamera != 1)
		return false;

	m_DebugCameraMovement.k[_iComponent] = _Amount;
	return true;
}


void CWClient_Mod::RefreshDebugCamera()
{
	static CMTime Prev;
	CMTime Now; Now.Snapshot();
	fp32 dTime = Min(0.1f, (Now - Prev).GetTime());
	Prev = Now;

	CVec3Dfp32& Pos = m_DebugCameraMat.GetRow(3);
	fp32 SpeedFactor = 100.0f;
#ifndef M_RTM
	SpeedFactor *= m_DebugCameraSpeedFactor;
#endif
	Pos += m_DebugCameraMat.GetRow(0) * ((m_DebugCameraMovement.k[0] - m_DebugCameraMovement.k[1]) * SpeedFactor * dTime);
	Pos += m_DebugCameraMat.GetRow(1) * ((m_DebugCameraMovement.k[2] - m_DebugCameraMovement.k[3]) * SpeedFactor * dTime);
}


void CWClient_Mod::SB_RenderMainStatus()
{
	MAUTOSTRIP(CWClient_Mod_SB_RenderMainStatus, MAUTOSTRIP_VOID);
	MSCOPE(CWClient_Mod::SB_RenderMainStatus, WCLIENTMOD);
	CWObject_CoreData* pObj = Object_Get(Player_GetLocalObject());
	if (!pObj) return;
//	if (pObj->m_ClientFlags & PLAYER_CLIENTFLAGS_NOCLIP) return;

	CRct Rect(0,0,640,480);
	CClipRect Clip(Rect);

#ifndef	DEF_DISABLE_PERFGRAPH
	if (m_ShowGraphs & 1)
	{
		CXR_VBManager* pVBM = m_Util2D.GetVBM();
		if (pVBM)
		{
			pVBM->ScopeBegin("CWClient_Mod::SB_RenderMainStatus", false);

			int x = 640-64;
			for(int i = 0; i < m_lspGraphs.Len(); i++)
			{
				pVBM->AddVB(m_lspGraphs[i]->Render(pVBM, CVec2Dfp32(x, 0), m_Util2D.GetTransform()) );
				x -= 64;
			}

			MACRO_GetRegisterObject(CPerfGraph, pGraph, "SYSTEM.SOUND.PERFGRAPH");
			if (pGraph)
			{
				pVBM->AddVB(pGraph->Render(pVBM, CVec2Dfp32(x, 0), m_Util2D.GetTransform()) );
				x -= 64;
			}

/*			pVBM->AddVB(m_spLagGraph->Render(pVBM, CVec2Dfp32(640-64, 0), m_Util2D.GetTransform()) );
			pVBM->AddVB(m_spRenderGraph->Render(pVBM, CVec2Dfp32(640-64*2, 0), m_Util2D.GetTransform()) );
			pVBM->AddVB(m_spNetGraph->Render(pVBM, CVec2Dfp32(640-64*3, 0), m_Util2D.GetTransform()) );
			pVBM->AddVB(m_spServerGraph->Render(pVBM, CVec2Dfp32(640-64*4, 0), m_Util2D.GetTransform()) );
			pVBM->AddVB(m_spNetTimeGraph->Render(pVBM, CVec2Dfp32(640-64*5, 0), m_Util2D.GetTransform()) );*/

			pVBM->ScopeEnd();
		}
	}
#endif


	int w = Clip.clip.GetWidth();
	int h = Clip.clip.GetHeight();
//	int xmid = Rect.GetWidth() / 2;
//	int ymid = Rect.GetHeight() / 2;


	if (m_lspFrameIn.Len() && (m_hConnection >= 0))
	{
//		CMTime Time = CMTime::GetCPU();
		CMTime Time;
		Time.Snapshot();
		fp32 LagTime = (Time - m_LastUnpackTime).GetTime();
		if (LagTime > GetGameTickTime()*20)
		{
//			m_pRender->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			int x = w/2-32;
			int y = h/2-32-64;
			m_Util2D.SetTextureOrigo(Clip, CPnt(x, y));
			m_Util2D.SetTexture("ICONNETTIMEOUT");
			m_Util2D.Rect3D(Clip, CRct(x, y, x+64, y+64), 0xffffffff, 0xffffffff, 0xffffffff);
		}
	}
}


void CWClient_Mod::RenderStatusBar(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP)
{
	MAUTOSTRIP(CWClient_Mod_RenderStatusBar, MAUTOSTRIP_VOID);
	MSCOPE(CWClient_Mod::RenderStatusBar, WCLIENTMOD);
//	CWObject_CoreData* pObj = Object_Get(Player_GetLocalObject());
//	if (!pObj) return;
	
	CRenderContext* pRC = _pRC;
	CXR_VBManager* pVBM = _pVBM;
	pVBM->ScopeBegin("CWClient_Mod::RenderStatusBar", false);
	m_Util2D.Begin(pRC, pVBM->Viewport_Get(), pVBM);

	m_Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE | CRC_FLAGS_CULL);
	m_Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	
//	m_Util2D.SetAutoFlush(true);

	CRct Rect = pVBM->Viewport_Get()->GetViewRect();
//	m_Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetWidth() / 640.0f ,  Rect.GetHeight() / 480.0f));
	m_Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetHeight() / 480.0f, Rect.GetHeight() / 480.0f));
	
	M_TRY
	{
		int iGameObj = Game_GetObjectIndex();
		if(iGameObj >= 0/* && ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_NEVERRENDERWORLD), iGameObj) == 0*/)
		{
			CWObject_Message Msg(OBJMSG_GAME_RENDERSTATUSBAR, aint((CXR_Engine *)m_spEngine), aint(&m_Util2D));
			CVec2Dfp32 NewRect(Rect.GetWidth() / 640.0f , Rect.GetHeight() / 480.0f);
			Msg.m_pData = &NewRect;
			Msg.m_DataSize = sizeof(CVec2Dfp32);
			ClientMessage_SendToObject(Msg, iGameObj);
		}

		SB_RenderMainStatus();
		SB_RenderInventory();
	}
	M_CATCH(
	catch(CCException)
	{
		m_Util2D.End();
		pVBM->ScopeEnd();
		throw;
	}
	)
	m_Util2D.End();
	pVBM->ScopeEnd();
}

void CWClient_Mod::RenderMultiplayerStatus(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP)
{
	CRenderContext* pRC = _pRC;
	CXR_VBManager* pVBM = _pVBM;
	pVBM->ScopeBegin("CWClient_Mod::RenderMultiplayerStatus", false);
	m_Util2D.Begin(pRC, pVBM->Viewport_Get(), pVBM);

	m_Util2D.GetAttrib()->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE | CRC_FLAGS_CULL);
	m_Util2D.GetAttrib()->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	//	m_Util2D.SetAutoFlush(true);

	CRct Rect = pVBM->Viewport_Get()->GetViewRect();
	m_Util2D.SetCoordinateScale(CVec2Dfp32(Rect.GetWidth() / 640.0f ,  Rect.GetHeight() / 480.0f));

	M_TRY
	{
		int iGameObj = Game_GetObjectIndex();
		if(iGameObj >= 0)
			ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAMEMOD_RENDERMULTIPLAYERSTATUS, aint((CXR_Engine *)m_spEngine), aint(&m_Util2D)), iGameObj);
	}
	M_CATCH(
		catch(CCException)
	{
		m_Util2D.End();
		pVBM->ScopeEnd();
		throw;
	}
	)
		m_Util2D.End();
	pVBM->ScopeEnd();
}

// -------------------------------------------------------------------
void CWClient_Mod::EnvBox_BeginCapture()
{
	MAUTOSTRIP(CWClient_Mod_EnvBox_BeginCapture, MAUTOSTRIP_VOID);
	CMat4Dfp32 Cam;
	Render_GetLastRenderCamera(Cam);
	m_EnvBox_iFace = 0;
	m_EnvBox_CameraPos = CVec3Dfp32::GetRow(Cam, 3);
}

void CWClient_Mod::EnvBox_GetCamera(CMat4Dfp32& _Camera)
{
	MAUTOSTRIP(CWClient_Mod_EnvBox_GetCamera, MAUTOSTRIP_VOID);
	CMat4Dfp32 Mat;
	Mat.Unit();
	Mat.RotX_x_M(-0.25f);
	Mat.RotY_x_M(0.25f);
	switch(m_EnvBox_iFace)
	{
	case 0 :
	case 1 :
	case 2 :
	case 3 :
		{
			int aLUT[4] = {0, 2, 3, 1};
			Mat.M_x_RotZ(fp32(aLUT[m_EnvBox_iFace]) / 4.0f);
		}
		break;
	case 4 :
			Mat.M_x_RotY(0.25f);
		break;
	case 5 :
			Mat.M_x_RotY(-0.25f);
		break;
	};

	m_EnvBox_CameraPos.SetRow(Mat, 3);
	_Camera = Mat;
}

void CWClient_Mod::EnvBox_SetViewPort(CXR_VBManager* _pVBM)
{
	MAUTOSTRIP(CWClient_Mod_EnvBox_SetViewPort, MAUTOSTRIP_VOID);
	CRC_Viewport CurVP;
	Render_ModifyViewport(CurVP);	// Need to get game viewport to get proper near/far plane

	CRC_Viewport VP = *_pVBM->Viewport_Get();
	CRct Rect = VP.GetViewRect();
	int Size = Min(Rect.GetWidth(), Rect.GetHeight());
	Size = 1 << Log2(Size);
	m_EnvBox_Size = Size;
	CRct NewRect(0,0,Size,Size);
	VP.SetView(CClipRect(NewRect, CPnt(0,0)), NewRect);
	VP.SetBackPlane(CurVP.GetBackPlane());
	VP.SetFrontPlane(CurVP.GetFrontPlane());
	VP.SetFOV(90);
	VP.SetFOVAspect(1.0f);
	VP.SetAspectRatio(1.0f);
	CMat4Dfp32 Mat = VP.GetProjectionMatrix();

	*_pVBM->Viewport_Get() = VP;
}

class CXR_PreRenderData_WriteScreenCapture
{
public:
	CRct m_SrcRect;
	char* m_pFileName;

	bool Create(CXR_VBManager* _pVBM, CRct _SrcRect, CStr _Name)
	{
		m_SrcRect = _SrcRect;

		int Len = _Name.Len();
		m_pFileName = (char*)_pVBM->Alloc(Len+1);
		if (!m_pFileName)
			return false;
		strcpy(m_pFileName, _Name.Str());
		return true;
	}
	
	static void WriteScreenCapture(CRenderContext* _pRC, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
	{
		CXR_PreRenderData_WriteScreenCapture* pData = (CXR_PreRenderData_WriteScreenCapture*)_pContext;

		MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
		if (!pSys)
			return;
			CImage* pImg = pSys->m_spDisplay->GetFrameBuffer();
		if (!pImg)
			return;

		spCImage spConv = pImg->Convert(pImg->GetFormat());
		spConv = spConv->Convert(IMAGE_FORMAT_BGR8);

		spCImage spSized = MNew(CImage);
		spSized->Create(pData->m_SrcRect.GetWidth(), pData->m_SrcRect.GetHeight(), IMAGE_FORMAT_BGR8, IMAGE_MEM_IMAGE);
		spSized->Blt(CClipRect(pData->m_SrcRect, CPnt(0,0)), *spConv, IMAGE_OPER_COPY, CPnt(0,0), 0 );
		spSized->Write(pData->m_pFileName);
	}
};

void CWClient_Mod::EnvBox_CaptureFrame(CXR_VBManager* _pVBM)
{
	MAUTOSTRIP(CWClient_Mod_EnvBox_CaptureFrame, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys)
	{
		m_EnvBox_iFace = -1;
		return;
	}

	if (!CDiskUtil::CreatePath(pSys->m_ExePath + "ScreenShots"))
		return;

	_pVBM->ScopeBegin(false, 5);

	CImage* pImg = pSys->m_spDisplay->GetFrameBuffer();
	if (pImg != NULL)
	{
		int i = 0;
		while(CDiskUtil::FileExists(pSys->m_ExePath + CStrF("Screenshots\\Cube_%s%.2d_%.1d.TGA", m_EnvBox_FileName.Str(), i, m_EnvBox_iFace))) i++;

		CXR_PreRenderData_WriteScreenCapture* pCapture = (CXR_PreRenderData_WriteScreenCapture*) _pVBM->Alloc(sizeof(CXR_PreRenderData_WriteScreenCapture));
		if (pCapture)
		{
			CStr Filename = pSys->m_ExePath + CStrF("Screenshots\\Cube_%s%.2d_%.1d.TGA", m_EnvBox_FileName.Str(), i, m_EnvBox_iFace);
			if (pCapture->Create(_pVBM, CRct(0, 0, m_EnvBox_Size, m_EnvBox_Size), Filename))
				_pVBM->AddCallback(CXR_PreRenderData_WriteScreenCapture::WriteScreenCapture, pCapture, 100000000);
		}

		m_EnvBox_iFace++;
		if (m_EnvBox_iFace >= 6)
		{
			m_EnvBox_iFace = -1;
			m_EnvBox_FileName.Clear();
		}
	}
	else
		m_EnvBox_iFace = -1;

	_pVBM->ScopeEnd();
}

/*
void CWClient_Mod::EnvBox_CaptureFrame()
{
	MAUTOSTRIP(CWClient_Mod_EnvBox_CaptureFrame, MAUTOSTRIP_VOID);
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (!pSys)
	{
		m_EnvBox_iFace = -1;
		return;
	}

	CImage* pImg = pSys->m_spDisplay->GetFrameBuffer();
	if (pImg != NULL)
	{
		int i = 0;
		if (!CDiskUtil::CreatePath(pSys->m_ExePath + "ScreenShots")) return;
		while(CDiskUtil::FileExists(pSys->m_ExePath + CStrF("Screenshots\\Cube_%s%.2d_%.1d.TGA", m_EnvBox_FileName.Str(), i, m_EnvBox_iFace))) i++;


		spCImage spConv = pImg->Convert(pImg->GetFormat());
		spConv = spConv->Convert(IMAGE_FORMAT_BGR8);
		spConv->Blt(spConv->GetClipRect(), *spConv, IMAGE_OPER_ADDSATURATE, CPnt(0,0), 0);

		spCImage spSize = MNew(CImage);
		spSize->Create(m_EnvBox_Size, m_EnvBox_Size, IMAGE_FORMAT_BGR8, IMAGE_MEM_IMAGE);
		spSize->Blt(CClipRect(CRct(0, 0, m_EnvBox_Size, m_EnvBox_Size), CPnt(0,0)), *spConv, IMAGE_OPER_COPY, CPnt(0,0), 0 );

		CStr Filename = pSys->m_ExePath + CStrF("Screenshots\\Cube_%s%.2d_%.1d.TGA", m_EnvBox_FileName.Str(), i, m_EnvBox_iFace);
		spSize->Write(Filename);

		m_EnvBox_iFace++;
		if (m_EnvBox_iFace >= 6)
		{
			m_EnvBox_iFace = -1;
			m_EnvBox_FileName.Clear();
		}
	}
	else
		m_EnvBox_iFace = -1;
}
*/

// -------------------------------------------------------------------
void AddLook(CControlFrame* _pMsg, CVec3Dfp32& _Look);
void AddLook(CControlFrame* _pMsg, CVec3Dfp32& _Look)
{
	MAUTOSTRIP(AddLook, MAUTOSTRIP_VOID);
	int Pos = _pMsg->GetCmdStartPos();
	while(Pos < _pMsg->m_MsgSize)
	{
		int Ctrl, Size, dTime;
		_pMsg->GetCmd(Pos, Ctrl, Size, dTime);
		if (Pos + Size > _pMsg->m_MsgSize) break;
		int PosSave = Pos;
		{
			if (Ctrl == CONTROL_LOOK)
			{
				if (Size == 6)
				{
//	ConOut("Added delta mouse");
					CVec3Dfp32 dLook = _pMsg->GetVecInt16_Max32768(Pos);
					dLook *= 1.0f / 65536;

			//		ConOutL("dLook " + dLook.GetString());
			//		m_Look[0] += dLook[0];
					_Look[1] = Min(0.249f, Max(-0.249f, _Look[1] + dLook[1]));
					_Look[2] += dLook[2];
				}
			}


		}
		Pos = PosSave + Size;
	}

}


#define DTIME 0

bool CWClient_Mod::ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey)
{
	MAUTOSTRIP(CWClient_Mod_ProcessKey, false);
	MSCOPE(CWClient_Mod::ProcessKey, WCLIENTMOD);
	if (m_spWnd != NULL)
	{
		if (m_spWnd->ProcessKey(_Key, _OriginalKey)) return true;
		return true;
	}


/*	if (_Key.GetKey9() == SKEY_MOUSEMOVEREL)
	{
		Con_MouseMove(_Key.GetPosX(), _Key.GetPosY());
		return true;
	}*/

	if (CWorld_ClientCore::ProcessKey(_Key, _OriginalKey)) return true;

	return false;
}

void CWClient_Mod::Net_AddIdleCommand()
{
	MAUTOSTRIP(CWClient_Mod_Net_AddIdleCommand, MAUTOSTRIP_VOID);
	AddMoveCmd();
}

void CWClient_Mod::Net_UpdateLook(fp32 _MinDelta)
{
	MAUTOSTRIP(CWClient_Mod_Net_UpdateLook, MAUTOSTRIP_VOID);
	CMTime Time = GetTime();
	fp32 dTime = (Time - m_Control_LastLookVelUpdate).GetTime();
	dTime = Clamp01(dTime) * m_TimeScale;
	if (dTime > _MinDelta)
	{
		if(m_Control_CurrentLookVelocity != CVec2Dfp32(0))
		{
			if(!(m_Control_Press & CONTROLBITS_AIMASSISTANCE))
			{
				// By having a lookvelocity, we assume pad-control and also
				// turn on AimAssistance

				m_Control_Press |= CONTROLBITS_AIMASSISTANCE;
				CCmd Cmd;
				Cmd.Create_StateBits(m_Control_Press, int(GetCmdDeltaTime()));
				AddCmd(Cmd);
			}
			dTime *= GetGameTicksPerSecond();		// VELOCITY-TODO
			CVec2Dfp32 Look(0);
			Look.Combine(m_Control_CurrentLookVelocity, dTime * 20.0f, Look);

			m_Control_Press &= ~CONTROLBITS_AIMASSISTANCE; // Don't let Con_Look detect AimAssitance
			Con_Look(Look[0], Look[1]);
			m_Control_Press |= CONTROLBITS_AIMASSISTANCE;
		}

		m_Control_LastLookVelUpdate = Time;
	}
}

void CWClient_Mod::Refresh()
{
	MAUTOSTRIP(CWClient_Mod_Refresh, MAUTOSTRIP_VOID);
	MSCOPE(CWClient_Mod::Refresh, WCLIENTMOD);

	if (m_DebugCamera == 1)
		RefreshDebugCamera();

	if (m_ClientMode & WCLIENT_MODE_PRIMARY)
	{
		
		const CRegistry* pReg = Registry_Get(WCLIENT_REG_GAMESTATE);
		if (pReg)
		{
			CFStr NewWindow = pReg->GetValue("WINDOW");
			if(NewWindow.Len())
			{
				if(NewWindow != m_DeferredWindow)
					SetInventoryWindowDeferred(NewWindow);
				m_InGameGUIWanted = true;
			}
			else
				m_InGameGUIWanted = false;
			UpdateInGameGui();

			m_ClientMode &= ~WCLIENT_MODE_GUI;
			if (m_spWnd)
				m_ClientMode |= WCLIENT_MODE_GUI;
		}

		if (m_spWnd != NULL)
			m_spWnd->OnRefresh();
	}

	m_ClientMode &= ~WCLIENT_MODE_GUI;
	if (m_spWnd != NULL)
		m_ClientMode |= WCLIENT_MODE_GUI;

	Net_UpdateLook(1.0f / 20.0f);
#ifdef WADDITIONALCAMERACONTROL
	Net_UpdateLookAdditional(1.0f / 20.0f);
#endif
	CWorld_ClientCore::Refresh();
}

void CWClient_Mod::Simulate(int _bCalcInterpolation)
{
	MAUTOSTRIP(CWClient_Mod_Simulate, MAUTOSTRIP_VOID);
	CWorld_ClientCore::Simulate(_bCalcInterpolation);

//	CMat4Dfp32 Pos; Render_GetLastRenderCamera(Pos);
//	ClientObject_Create("CoordinateSystem", Pos);
}

void CWClient_Mod::Con_ShowInfoScreen(CStr _Info)
{
	MAUTOSTRIP(CWClient_Mod_Con_ShowInfoScreen, MAUTOSTRIP_VOID);

	m_InfoScreen = _Info;
}

void CWClient_Mod::Con_ForceNoWorld(int _bNoWorld)
{
	MAUTOSTRIP(CWClient_Mod_Con_ForceNoWorld, MAUTOSTRIP_VOID);
	MSCOPE(Con_ForceNoWorld, WCLIENTMOD);
	m_bForceNoWorld = _bNoWorld != 0;
}

// -------------------------------------------------------------------
//  Player command stuff
// -------------------------------------------------------------------
void CWClient_Mod::AddMoveCmd()
{
	MAUTOSTRIP(CWClient_Mod_AddMoveCmd, MAUTOSTRIP_VOID);
	CVec3Dfp32 Move(0);

	if(m_bLeanPressed)
	{
		int Press = m_Control_Press & ~(CONTROLBITS_DPAD_UP | CONTROLBITS_DPAD_RIGHT | CONTROLBITS_DPAD_DOWN | CONTROLBITS_DPAD_LEFT);
		if(m_Control_Left > 0)
			Press |= CONTROLBITS_DPAD_LEFT;
		if(m_Control_Right > 0)
			Press |= CONTROLBITS_DPAD_RIGHT;
		if(m_Control_Forward > 0)
			Press |= CONTROLBITS_DPAD_UP;
		if(m_Control_Backward > 0)
			Press |= CONTROLBITS_DPAD_DOWN;

		if(Press != m_Control_Press)
		{
			m_Control_Press = Press;
			CCmd Cmd;
			Cmd.Create_StateBits(m_Control_Press, int(GetCmdDeltaTime()));
			AddCmd(Cmd);
		}
	}
	else
	{
		Move[1] += m_Control_Left;
		Move[1] -= m_Control_Right;
		Move[0] += m_Control_Forward;
		Move[0] -= m_Control_Backward;
		Move[2] += m_Control_Up;
		Move[2] -= m_Control_Down;
		for(int k = 0; k < 3; k++)
			Move[k] = Max(-1.0f, Min(1.0f, Move[k]));
	}

	CCmd Cmd;
	if(m_bWalkPressed)
		Cmd.Create_Move(Move * 256.0f / 2, int(GetCmdDeltaTime()));
	else
		Cmd.Create_Move(Move * 256.0f, int(GetCmdDeltaTime()));

	AddCmd(Cmd);

//ConOutL(CStrF("Command %f, ID %d, Cmd %d, dTime", GetCPUClock() / GetCPUFrequency(), Cmd.m_ID, Cmd.m_Cmd, Cmd.m_dTime));
}
#ifdef WADDITIONALCAMERACONTROL
void CWClient_Mod::AddMoveCmdAdditional()
{
	MAUTOSTRIP(CWClient_Mod_AddMoveCmdAdditional, MAUTOSTRIP_VOID);
	CVec3Dfp32 Move(0);

	Move[1] += m_Control_Left2;
	Move[1] -= m_Control_Right2;
	Move[0] += m_Control_Forward2;
	Move[0] -= m_Control_Backward2;
	Move[2] += m_Control_Up2;
	Move[2] -= m_Control_Down2;
	for(int k = 0; k < 3; k++)
		Move[k] = Max(-1.0f, Min(1.0f, Move[k]));

	CCmd Cmd;
	Cmd.Create_MoveAdditional(Move * 256.0f, GetCmdDeltaTime());
	AddCmd(Cmd);
}

void CWClient_Mod::Con_LookAdditional(fp32 _dX, fp32 _dY)
{
	MAUTOSTRIP(CWClient_Mod_Con_LookAdditional, MAUTOSTRIP_VOID);
	MSCOPE(Con_LookAdditional, WCLIENTMOD);
	CVec3Dfp32 dLook;
	dLook[0] = 0;
	dLook[1] = -fp32(_dY) * m_Sensitivity[1] * 32.0f;
	dLook[2] = fp32(_dX) * m_Sensitivity[0] * 32.0f;

	if(dLook != CVec3Dfp32(0))
	{
		CCmd Cmd;
		Cmd.Create_LookAdditional(dLook, GetCmdDeltaTime());
		AddCmd(Cmd);
	}
}

void CWClient_Mod::Con_MoveForwardAdditional(fp32 _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_MoveForwardAdditional, MAUTOSTRIP_VOID);
	MSCOPE(Con_MoveForwardAdditional, WCLIENTMOD);
	m_Control_Forward2 = _v;
	AddMoveCmdAdditional();
}

void CWClient_Mod::Con_MoveRightAdditional(fp32 _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_MoveRightAdditional, MAUTOSTRIP_VOID);
	MSCOPE(Con_MoveRight, WCLIENTMOD);
	m_Control_Right2 = _v;
	AddMoveCmdAdditional();
}

void CWClient_Mod::Con_LookVelocityAdditional(fp32 _dX, fp32 _dY)
{
	MAUTOSTRIP(CWClient_Mod_Con_LookVelocityAdditional, MAUTOSTRIP_VOID);
	MSCOPE(Con_LookVelocityAdditional, WCLIENTMOD);

	m_Control_TargetLookVelocity2[0] = _dX;
	m_Control_TargetLookVelocity2[1] = _dY;
}

void CWClient_Mod::Con_PressAdditional(int _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_PressAdditional, MAUTOSTRIP_VOID);
	MSCOPE(Con_PressAdditional, WCLIENTMOD);
	m_Control_Press2 |= _v;

	CCmd Cmd;
	Cmd.Create_StateBitsAdditional(m_Control_Press2, GetCmdDeltaTime());
	AddCmd(Cmd);

	ConOutL(CStrF("Con_Press %.8x", m_Control_Press));
}

void CWClient_Mod::Con_ReleaseAdditional(int _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_ReleaseAdditional, MAUTOSTRIP_VOID);
	m_Control_Press2 &= ~_v;

	CCmd Cmd;
	Cmd.Create_StateBitsAdditional(m_Control_Press2, GetCmdDeltaTime());
	AddCmd(Cmd);

	ConOutL(CStrF("Con_Release %.8x", m_Control_Press));
}

void CWClient_Mod::Net_UpdateLookAdditional(fp32 _MinDelta)
{
	MAUTOSTRIP(CWClient_Mod_Net_UpdateLookAdditional, MAUTOSTRIP_VOID);
	CMTime Time = GetTime();
	fp32 dTime = (Time - m_Control_LastLookVelUpdate2).GetTime();
	dTime = Clamp01(dTime) * m_TimeScale;
	if (dTime > _MinDelta)
	{
		dTime /= CWCLIENT_TIMEPERFRAME;
		CVec2Dfp32 Look(0);
		Look.Combine(m_Control_CurrentLookVelocity2, dTime, Look);
		Con_LookAdditional(Look[0], Look[1]);

		m_Control_LastLookVelUpdate2 = Time;
	}
}

void CWClient_Mod::RenderAdditional()
{
	CMTime Time = GetTime();
	fp32 dTime = (Time - m_LastLookUpdateTime2).GetTime();
	dTime = Clamp01(dTime) * m_TimeScale;
	m_LastLookUpdateTime2 = Time;

	CVec2Dfp32 VelNoSens = m_Control_CurrentLookVelocity2;
	VelNoSens[0] /= m_Sensitivity[0];
	VelNoSens[1] /= m_Sensitivity[1];

	if(VelNoSens.DistanceSqr(m_Control_TargetLookVelocity2) > 0.0001f*0.0001f)
	{
		for(int i = 0; i < 2 ; i++)
		{
			fp32 TargetVel = m_Control_TargetLookVelocity2[i] * m_Sensitivity[i];
			fp32 CurVel = m_Control_CurrentLookVelocity2[i];
			if((CurVel > TargetVel && CurVel >0)||
				(CurVel < TargetVel && CurVel <0))
			{
				//Deacceleration is immediate to the lesser velocity. If switching directions the new velocity will start at 0
				if(i== 0 && CurVel * TargetVel < 0)
					CurVel = Sign(TargetVel) * Min(CONTROLLER_DEFAULT_STARTSPEEDX,M_Fabs(TargetVel));
				else
					CurVel = TargetVel;
			}
			else 
			{
				fp32 Diff1 = TargetVel - CurVel;
				if(i == 0)
				{
					if(CurVel < TargetVel)
						CurVel = Max(CONTROLLER_DEFAULT_STARTSPEEDX,CurVel) + m_LookAcceleration[i] * dTime;
					else
						CurVel = Min(-CONTROLLER_DEFAULT_STARTSPEEDX,CurVel) - m_LookAcceleration[i] * dTime;
				}
				else
				{
					if(CurVel < TargetVel)
						CurVel += m_LookAcceleration[i] * dTime;
					else
						CurVel -= m_LookAcceleration[i] * dTime;
				}
				fp32 Diff2 = TargetVel - CurVel;

				// Don't let the target velocity "pass" the current velocity
				if(Diff1 * Diff2 <= 0)
					CurVel = TargetVel;
			}
			m_Control_CurrentLookVelocity2[i] = CurVel;
		}

		Net_UpdateLookAdditional(1.0f / 50.0f);
	}
}
#endif

void CWClient_Mod::Con_MoveForward(fp32 _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_MoveForward, MAUTOSTRIP_VOID);
	MSCOPE(Con_MoveForward, WCLIENTMOD);
	if (UpdateDebugCamera_Move(0, _v))
		return;

	m_Control_Forward = _v;
	AddMoveCmd();
}

void CWClient_Mod::Con_MoveBackward(fp32 _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_MoveBackward, MAUTOSTRIP_VOID);
	MSCOPE(Con_MoveBackward, WCLIENTMOD);
	if (UpdateDebugCamera_Move(1, _v))
		return;

	m_Control_Backward = _v;
	AddMoveCmd();
}

void CWClient_Mod::Con_MoveLeft(fp32 _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_MoveLeft, MAUTOSTRIP_VOID);
	MSCOPE(Con_MoveLeft, WCLIENTMOD);
	if (UpdateDebugCamera_Move(2, _v))
		return;

	m_Control_Left = _v;
	AddMoveCmd();
}

void CWClient_Mod::Con_MoveRight(fp32 _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_MoveRight, MAUTOSTRIP_VOID);
	MSCOPE(Con_MoveRight, WCLIENTMOD);
	if (UpdateDebugCamera_Move(3, _v))
		return;

	m_Control_Right = _v;
	AddMoveCmd();
}

void CWClient_Mod::Con_MoveUp(fp32 _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_MoveUp, MAUTOSTRIP_VOID);
	MSCOPE(Con_MoveUp, WCLIENTMOD);
	m_Control_Up = _v;
	AddMoveCmd();
}

void CWClient_Mod::Con_MoveDown(fp32 _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_MoveDown, MAUTOSTRIP_VOID);
	MSCOPE(Con_MoveDown, WCLIENTMOD);
	m_Control_Down = _v;
	AddMoveCmd();
}

void CWClient_Mod::Con_LookLeft(fp32 _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_LookLeft, MAUTOSTRIP_VOID);
	MSCOPE(Con_LookLeft, WCLIENTMOD);
	m_Control_LookAdd[2] += _v * m_Sensitivity[0];
}

void CWClient_Mod::Con_LookRight(fp32 _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_LookRight, MAUTOSTRIP_VOID);
	MSCOPE(Con_LookRight, WCLIENTMOD);
	m_Control_LookAdd[2] -= _v * m_Sensitivity[0];
}

void CWClient_Mod::Con_LookUp(fp32 _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_LookUp, MAUTOSTRIP_VOID);
	MSCOPE(Con_LookUp, WCLIENTMOD);
	m_Control_LookAdd[1] += _v * m_Sensitivity[1];
}

void CWClient_Mod::Con_LookDown(fp32 _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_LookDown, MAUTOSTRIP_VOID);
	MSCOPE(Con_LookDown, WCLIENTMOD);
	m_Control_LookAdd[1] -= _v * m_Sensitivity[1];
}

void CWClient_Mod::Con_Look(fp32 _dX, fp32 _dY)
{
	MAUTOSTRIP(CWClient_Mod_Con_Look, MAUTOSTRIP_VOID);
	MSCOPE(Con_Look, WCLIENTMOD);

	if (UpdateDebugCamera_Look(CVec2Dfp32(_dX * -0.0005f * m_Sensitivity[0], _dY * -0.0005f * m_Sensitivity[1])))
		return;

	if(m_Control_Press & CONTROLBITS_AIMASSISTANCE)
	{
		// By calling this command, we assume this is mouse-controlled look
		// and therefor turn off AimAssistance

		m_Control_Press &= ~CONTROLBITS_AIMASSISTANCE; // temp for debug testing
		CCmd Cmd;
		Cmd.Create_StateBits(m_Control_Press, int(GetCmdDeltaTime()));
		AddCmd(Cmd);
	}

	CVec3Dfp32 dLook;
	dLook[0] = 0;
	dLook[1] = -fp32(_dY) * m_Sensitivity[1] * 32.0f;
	dLook[2] = fp32(_dX) * m_Sensitivity[0] * 32.0f;

	if(dLook != CVec3Dfp32(0))
	{
		CCmd Cmd;
		Cmd.Create_Look(dLook, int(GetCmdDeltaTime()));
		AddCmd(Cmd);
	}
}

void CWClient_Mod::Con_LookVelocity(fp32 _dX, fp32 _dY)
{
	MAUTOSTRIP(CWClient_Mod_Con_LookVelocity0, MAUTOSTRIP_VOID);
	MSCOPE(Con_LookVelocity, WCLIENTMOD);

	m_Control_TargetLookVelocity[0] = _dX;
	m_Control_TargetLookVelocity[1] = _dY;
}

void CWClient_Mod::Con_LookVelocityX(fp32 _dX)
{
	MAUTOSTRIP(CWClient_Mod_Con_LookVelocityX, MAUTOSTRIP_VOID);
	MSCOPE(Con_LookVelocityX, WCLIENTMOD);

	m_Control_TargetLookVelocity[0] = _dX;
}

void CWClient_Mod::Con_LookVelocityY(fp32 _dY)
{
	MAUTOSTRIP(CWClient_Mod_Con_LookVelocityY, MAUTOSTRIP_VOID);
	MSCOPE(Con_LookVelocityY, WCLIENTMOD);

	m_Control_TargetLookVelocity[1] = _dY;
}

void CWClient_Mod::Con_ToggleCrouch()
{
	if(m_Control_Press & CONTROLBITS_CROUCH)
		m_Control_Press &= ~CONTROLBITS_CROUCH;
	else
		m_Control_Press |= CONTROLBITS_CROUCH;

	CCmd Cmd;
	Cmd.Create_StateBits(m_Control_Press, int(GetCmdDeltaTime()));
	AddCmd(Cmd);
}

void CWClient_Mod::Con_Lean(int _v)
{
	if(m_bLeanPressed && !_v)
	{
		m_bLeanPressed = _v;
		m_Control_Press &= ~(CONTROLBITS_DPAD_UP | CONTROLBITS_DPAD_RIGHT | CONTROLBITS_DPAD_DOWN | CONTROLBITS_DPAD_LEFT);

		CCmd Cmd;
		Cmd.Create_StateBits(m_Control_Press, int(GetCmdDeltaTime()));

		AddCmd(Cmd);
		AddMoveCmd();
	}
	else if(!m_bLeanPressed && _v)
	{
		m_bLeanPressed = _v;
		AddMoveCmd();
	}
}

void CWClient_Mod::Con_Walk(int _v)
{
	m_bWalkPressed = _v;
	AddMoveCmd();
}

void CWClient_Mod::Con_Press(int _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_Press, MAUTOSTRIP_VOID);
	MSCOPE(Con_Press, WCLIENTMOD);

	if(ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_CLIENTINPUTOVERRIDE, _v), Player_GetLocalObject()))
		return;

	m_Control_Press |= _v;

	CCmd Cmd;
	Cmd.Create_StateBits(m_Control_Press, int(GetCmdDeltaTime()));
	AddCmd(Cmd);

	//M_TRACEALWAYS("press %x (%x)\n", _v, m_Control_Press);
}

void CWClient_Mod::Con_PressAnalog(int _Channel, fp32 _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_PressAnalog, MAUTOSTRIP_VOID);

	if((_Channel<0)
	|| (_Channel>=MAX_ANALOGCHANNELS))
		return;

	CCmd Cmd;
	Cmd.m_Cmd = CONTROL_ANALOG;
	Cmd.m_Size = 2;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Data[0] = _Channel;
	Cmd.m_Data[1] = uint8(_v);
	AddCmd(Cmd);

	m_Control_Analog[_Channel] = uint8(_v);		// _v should range from 0 (not pressed at all) to 255 (fully pressed)
}

void CWClient_Mod::Con_PressADC(fp32 _v, int _op, fp32 _threshold, int _button)
{
	bool CondMet = false;

	switch(_op)
	{
		case ADC_OP_GT: CondMet = _v > _threshold; break;
		case ADC_OP_LT: CondMet = _v < _threshold; break;
		default: M_ASSERT(false, CStrF("Con_PressADC, invalid operand '%i'", _op));
	}

	if (CondMet)
		Con_Press(_button);
	else
		Con_Release(_button);
}

void CWClient_Mod::Con_Release(int _v)
{
	MAUTOSTRIP(CWClient_Mod_Con_Release, MAUTOSTRIP_VOID);
	m_Control_Press &= ~_v;

	CCmd Cmd;
	Cmd.Create_StateBits(m_Control_Press, int(GetCmdDeltaTime()));
	AddCmd(Cmd);
	//M_TRACEALWAYS("release %x (%x)\n", _v, m_Control_Press);
}


void CWClient_Mod::Con_ReleaseAll()
{
	MAUTOSTRIP(CWClient_Mod_Con_ReleaseAll, MAUTOSTRIP_VOID);
	MSCOPE(Con_ReleaseAll, WCLIENTMOD);
	int Mask = m_Control_Press & ~(CONTROLBITS_CROUCH);
	if(Mask != 0)
	{
		m_Control_Press = m_Control_Press & CONTROLBITS_CROUCH;
		CCmd Cmd;
		Cmd.Create_StateBits(m_Control_Press, int(GetCmdDeltaTime()));
		AddCmd(Cmd);
	}

	bool bMove = false;
	if(m_Control_Forward != 0)	{ m_Control_Forward = 0; bMove = true; }
	if(m_Control_Backward != 0)	{ m_Control_Backward = 0; bMove = true; }
	if(m_Control_Left != 0)		{ m_Control_Left = 0; bMove = true; }
	if(m_Control_Right != 0)	{ m_Control_Right = 0; bMove = true; }
	if(m_Control_Up != 0)		{ m_Control_Up = 0; bMove = true; }
	if(m_Control_Down != 0)		{ m_Control_Down = 0; bMove = true; }
	if(bMove)
		AddMoveCmd();

	m_Control_CurrentLookVelocity = 0;
	m_Control_TargetLookVelocity = 0;
	m_Control_LookAdd = 0;
	m_Control_Look = 0;
	m_Control_Analog[0] = 0;
	m_Control_Analog[1] = 0;
	m_Control_Analog[2] = 0;
	m_Control_Analog[3] = 0;

	m_bLeanPressed = false;
	m_bWalkPressed = false;

#ifdef WADDITIONALCAMERACONTROL
	m_Control_Forward2 = 0.0f;
	m_Control_Backward2 = 0.0f;
	m_Control_Left2 = 0.0f;
	m_Control_Right2 = 0.0f;
	m_Control_Up2 = 0.0f;
	m_Control_Down2 = 0.0f;
	m_Control_LookAdd2 = 0.0f;
	m_Control_Look2 = 0.0f;
	m_Control_Press2 = 0.0f;
	for (int32 i = 0; i < MAX_ANALOGCHANNELS; i++)
		m_Control_Analog2[i] = 0;

	m_Control_LastLookVelUpdate2.Reset();
	m_Control_CurrentLookVelocity2 = 0.0f;
	m_Control_TargetLookVelocity2 = 0.0f;
#endif
}

void CWClient_Mod::Con_StringCmd(int _Cmd, CStr _Str)
{
	MAUTOSTRIP(CWClient_Mod_Con_StringCmd, MAUTOSTRIP_VOID);
	MSCOPE(Con_StringCmd, WCLIENTMOD);
	int Len = _Str.Len();
	if (Len > 240) Error("Con_StringCmd", "Invalid string length.");

	CCmd Cmd;
	Cmd.m_Cmd = _Cmd;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Size = Len;
	memcpy(&Cmd.m_Data[0], (const char*)_Str, Len);

	AddCmd(Cmd);
}

void CWClient_Mod::Con_StringCmdForced(int _Cmd, CStr _Str)
{
	MAUTOSTRIP(CWClient_Mod_Con_StringCmdForced, MAUTOSTRIP_VOID);
	MSCOPE(Con_StringCmd, WCLIENTMOD);
	int Len = _Str.Len();
	if (Len > 240) Error("Con_StringCmd", "Invalid string length.");

	CCmd Cmd;
	Cmd.m_Cmd = _Cmd;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Size = Len;
	memcpy(&Cmd.m_Data[0], (const char*)_Str, Len);

	AddCmd_Forced(Cmd);
}

void CWClient_Mod::Con_AddItem(CStr _Item)
{
	MAUTOSTRIP(CWClient_Mod_Con_AddItem, MAUTOSTRIP_VOID);
	if (!_Item.Len()) return;

	Con_StringCmdForced(CONTROL_ADDITEM, _Item);
}


void CWClient_Mod::Con_Summon(CStr _PlayerCls)
{
	MAUTOSTRIP(CWClient_Mod_Con_Summon, MAUTOSTRIP_VOID);
	if (!_PlayerCls.Len()) return;

	Con_StringCmd(CONTROL_SUMMON, _PlayerCls);
}

void CWClient_Mod::Con_MenuTrigger(int _ID)
{
	MAUTOSTRIP(CWClient_Mod_Con_MenuTrigger, MAUTOSTRIP_VOID);
	MSCOPE(Con_MenuTrigger, WCLIENTMOD);
	
	CCmd Cmd;
	Cmd.m_Cmd = CONTROL_MENUTRIGGER;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Size = 1;
	Cmd.m_Data[0] = _ID;
	AddCmd_Forced(Cmd);
}

void CWClient_Mod::Con_SelectItemType(int _Type)
{
	MAUTOSTRIP(CWClient_Mod_Con_SelectItemType, MAUTOSTRIP_VOID);
	MSCOPE(Con_SelectItemType, WCLIENTMOD);

	CCmd Cmd;
	Cmd.m_Cmd = CONTROL_SELECTITEM;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Size = 4;
	*(int*)Cmd.m_Data = _Type;
	AddCmd_Forced(Cmd);
}

void CWClient_Mod::Con_DebugMsg(CStr _Msg)
{
	MAUTOSTRIP(CWClient_Mod_Con_DebugMsg, MAUTOSTRIP_VOID);
	MSCOPE(Con_DebugMSg, WCLIENTMOD);
	int Len = _Msg.Len();
	if(Len > 236 || Len <= 0)
		Error("Con_DebugImpulse", "Invalid string length.");

	CCmd Cmd;
	Cmd.m_Cmd = CONTROL_DEBUGMSG;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Size = Len;
	memcpy(Cmd.m_Data, _Msg.Str(), Len);
	AddCmd(Cmd);
}

void CWClient_Mod::Con_ShowDebugHud()
{
	if (ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_ISDEBUGHUDVISIBLE), Player_GetLocalObject()))
		ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_SHOWDEBUGHUD, 0), Player_GetLocalObject());
	else
		ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_SHOWDEBUGHUD, 1), Player_GetLocalObject());
}

#ifndef M_RTM

void CWClient_Mod::Con_ActivateItem(CStr _PlayerCls)
{
	MAUTOSTRIP(CWClient_Mod_Con_ActivateItem, MAUTOSTRIP_VOID);
	if (!_PlayerCls.Len()) return;

	Con_StringCmd(CONTROL_ACTIVATEITEM, _PlayerCls);
}

void CWClient_Mod::Con_SetApproachItem(CStr _Char, CStr _Item)
{
	MAUTOSTRIP(CWClient_Mod_Con_SetApproachItem, MAUTOSTRIP_VOID);
	if (_Char.Len() == 0 || _Item.Len() == 0)
		return;

	CStr str = CStrF("%s:%s", _Char.Str(), _Item.Str());

	CCmd Cmd;
	Cmd.m_Cmd = CONTROL_SETAPPROACHITEM;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Size = str.Len() + 1;
	strcpy((char *)Cmd.m_Data, str.Str());
	AddCmd(Cmd);
}

void CWClient_Mod::Con_DebugImpulse(CStr _Target, int _Impulse)
{
	MAUTOSTRIP(CWClient_Mod_Con_DebugImpulse, MAUTOSTRIP_VOID);
	MSCOPE(Con_DebugImpulse, WCLIENTMOD);
	int Len = _Target.Len();
	if(Len > 236 || Len <= 0)
		Error("Con_DebugImpulse", "Invalid string length.");
	
	CCmd Cmd;
	Cmd.m_Cmd = CONTROL_DEBUGIMPULSE;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Size = Len + 4;
	*((int *)&Cmd.m_Data[0]) = _Impulse;
	memcpy(&Cmd.m_Data[4], _Target.Str(), Len);
	AddCmd(Cmd);
}

void CWClient_Mod::Con_EnvBoxSnapshot(CStr _Name)
{
	MAUTOSTRIP(CWClient_Mod_Con_EnvBoxSnapshot, MAUTOSTRIP_VOID);
	m_EnvBox_FileName	= _Name;
	m_EnvBox_FileName.MakeUnique();
	EnvBox_BeginCapture();
}

void CWClient_Mod::Con_DumpWnd()
{
	MAUTOSTRIP(CWClient_Mod_Con_DumpWnd, MAUTOSTRIP_VOID);
	if (m_spWnd != NULL)
		m_spWnd->OnDumpTree();
	else
		ConOut("No current window system.");
}

// HACK! This is currently overridden for the sole purpose of being able to detect no-clip speed factor commands and apply them to debug camera as well.
void CWClient_Mod::Con_Cmd2(int _cmd, int d0)
{
	MAUTOSTRIP(CWorld_ClientCore_Con_Cmd2, MAUTOSTRIP_VOID);
	if (_cmd == CMD_NOCLIPSPEED)
	{
		if (d0 == 1)
		{
			m_DebugCameraSpeedFactor *= 2.0f;
			if (m_DebugCameraSpeedFactor > 64.0f)
				m_DebugCameraSpeedFactor = 64.0f;
		}
		else if (d0 == -1)
		{
			m_DebugCameraSpeedFactor /= 2.0f;
			if (m_DebugCameraSpeedFactor < 1.0f /64.0f)
				m_DebugCameraSpeedFactor = 1.0f /64.0f;
		}
		else
		{
			m_DebugCameraSpeedFactor = 1.0f;
		}
	}
	/*
	else if (_cmd == CMD_AIDEBUG)
	{
		CVec3Dfp32 Dir;
		CVec3Dfp32 Pos;
		if ((m_DebugCamera)&&(m_DebugCameraMat.GetRow(3).LengthSqr() < Sqr(32767.0f)))
		{
			Dir = m_DebugCameraMat.GetRow(0);
			Dir = m_DebugCameraMat.GetRow(3);
		}
		else
		{
			Dir[0] = 0.0f;
			Dir[1] = 0.0f;
			Dir[2] = 0.0f;
			Pos[0] = 0.0f;
			Pos[1] = 0.0f;
			Pos[2] = 0.0f;
		}
		
		CCmd Cmd;
		Cmd.Create_AIDebugMsg(_cmd,d0,Dir,Pos, int(GetCmdDeltaTime()));
		AddCmd(Cmd);
		return;
	}
	*/
	CWorld_ClientCore::Con_Cmd2(_cmd, d0);
}

void CWClient_Mod::Con_HurtEffect(int _v)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys)
		pSys->GetEnvironment()->SetValuei("CL_HURTEFFECT", _v);
}

#endif

void CWClient_Mod::Con_JoinGame(CStr _PlayerCls)
{
	if (_PlayerCls == "")
		return;

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CStr ProfileName = pSys->GetOptions()->GetValue("GAME_PROFILE");

	m_LocalPlayer.m_Name = ProfileName;
//	CStr cmd = CStrF("%s§%s", _PlayerCls, ProfileName);
#if defined(PLATFORM_XENON)
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);
    
	CCmd Cmd;
	Cmd.m_Cmd = CONTROL_SETNAME_PLAYERID;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Size = sizeof(int);
	int ID = pGameMod->m_pMPHandler->GetPlayerID(-1);
	memcpy(&Cmd.m_Data[0], (const char*)ID, Cmd.m_Size);

	AddCmd(Cmd);
#else
	Con_StringCmd(CONTROL_SETNAME, m_LocalPlayer.m_Name);
#endif
	Con_StringCmd(CONTROL_JOINGAME, _PlayerCls);
}

void CWClient_Mod::Con_SelectDialogueChoice(int _iChoice)
{
	MAUTOSTRIP(CWClient_Mod_Con_SelectDialogueChoice, MAUTOSTRIP_VOID);

	CCmd Cmd;
	Cmd.m_Cmd = CONTROL_SELECTDIALOGUECHOICE;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Size = 1;
	Cmd.m_Data[0] = _iChoice;
	AddCmd_Forced(Cmd);
}

void CWClient_Mod::Con_SelectTelephoneChoice(int _iChoice)
{
	MAUTOSTRIP(CWClient_Mod_Con_SelectTelephoneChoice, MAUTOSTRIP_VOID);

	CCmd Cmd;
	Cmd.m_Cmd = CONTROL_SELECTTELEPHONECHOICE;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Size = 1;
	Cmd.m_Data[0] = _iChoice;
	AddCmd_Forced(Cmd);
}

void CWClient_Mod::Con_InvMenu(CStr _Name)
{
	MAUTOSTRIP(CWClient_Mod_Con_InvMenu, MAUTOSTRIP_VOID);
	SetInventoryWindow(_Name);
}

void CWClient_Mod::Con_OpenGameMenu()
{
	MAUTOSTRIP(CWClient_Mod_Con_OpenGameMenu, MAUTOSTRIP_VOID);

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CRegistry *pReg = pSys->GetEnvironment();
	if(pReg)
	{
		CStr Map = pReg->GetValue("QUICKMAP");
		if(Map != "")
		{
			ConExecute("quit()");
			return;
		}
	}
/*	int iGameState = ClientMessage_SendToObject(CWObject_Message(OBJMSG_GAME_GETGAMESTATE), Game_GetObjectIndex());
	if(iGameState == CWObject_Mod::MOD_GAMESTATE_GAMEON)*/

	CWorld_ClientCore::Con_OpenGameMenu();
}

void CWClient_Mod::Con_ToggleInvertMouse()
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if(pSys && pSys->GetOptions())
	{
		int bInvert = pSys->GetOptions()->GetValuei("CONTROLLER_INVERTYAXIS");
		pSys->GetOptions()->SetValuei("CONTROLLER_INVERTYAXIS", bInvert ^ 1);
		UpdateControllerSettings();
		ConExecute("option_save()");
	}
}

void CWClient_Mod::Con_ToggleShowHUD()
{
	m_bHideHUD = ~m_bHideHUD;
}

void CWClient_Mod::Con_ToggleMultiplayerStatus()
{
	m_bShowMultiplayerStatus = ~m_bShowMultiplayerStatus;
}

void CWClient_Mod::Con_SetMultiplayerStatus(int _Show)
{
	m_bShowMultiplayerStatus = _Show;
}

void CWClient_Mod::Con_GUI_Prev(int _ScreenID)
{
	MAUTOSTRIP(CWClient_Mod_Con_GUI_Prev, MAUTOSTRIP_VOID);
	MSCOPE(Con_GUI_Prev, WCLIENTMOD);
	CCmd Cmd;
	Cmd.m_Cmd = CONTROL_GUI_PREV;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Size = 1;
	Cmd.m_Data[0] = _ScreenID;
	AddCmd_Forced(Cmd);
}

void CWClient_Mod::Con_GUI_Next(int _ScreenID)
{
	MAUTOSTRIP(CWClient_Mod_Con_GUI_Next, MAUTOSTRIP_VOID);
	MSCOPE(Con_GUI_Next, WCLIENTMOD);
	CCmd Cmd;
	Cmd.m_Cmd = CONTROL_GUI_NEXT;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Size = 1;
	Cmd.m_Data[0] = _ScreenID;
	AddCmd_Forced(Cmd);
}

void CWClient_Mod::Con_OverrideFilter(int _v)
{
	m_OverrideFilter = _v;
}

void CWClient_Mod::Con_ShaderDiff(int _v)
{
	m_ShaderDiff = _v;
}

void CWClient_Mod::Con_ShaderDiffNV(int _v)
{
	m_ShaderDiffNV = _v;
}

void CWClient_Mod::Con_ShaderSpec(int _v)
{
	m_ShaderSpec = _v;
}

void CWClient_Mod::Con_ShaderSpecNV(int _v)
{
	m_ShaderSpecNV = _v;
}

void CWClient_Mod::Con_NVViewScale(int _v)
{
	m_bNVViewScale = _v;

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	if (pSys)
		pSys->GetEnvironment()->SetValuei("CL_NVVIEWSCALE", (m_bNVViewScale) ? 1 : 0);
}

void CWClient_Mod::Con_VisionsEnabled(int _v)
{
	if (_v != 0)
		m_CamFXMode |= CXR_MODEL_CAMFX_VISIONSENABLED;
	else
		m_CamFXMode &= ~CXR_MODEL_CAMFX_VISIONSENABLED;
}

void CWClient_Mod::Con_DarknessVision(int _v)
{
	if (_v != 0)
		m_CamFXMode |= CXR_MODEL_CAMFX_DARKNESSVISION;
	else
		m_CamFXMode &= ~CXR_MODEL_CAMFX_DARKNESSVISION;
}

void CWClient_Mod::Con_CreepingDark(int _v)
{
	if (_v != 0)
		m_CamFXMode |= CXR_MODEL_CAMFX_CREEPINGDARK;
	else
		m_CamFXMode &= ~CXR_MODEL_CAMFX_CREEPINGDARK;
}

void CWClient_Mod::Con_OtherWorld(int _v)
{
	if (_v != 0)
		m_CamFXMode |= CXR_MODEL_CAMFX_OTHERWORLD;
	else
		m_CamFXMode &= ~CXR_MODEL_CAMFX_OTHERWORLD;
}

void CWClient_Mod::Con_RetinaDots(int _v)
{
	if (_v != 0)
		m_CamFXMode |= CXR_MODEL_CAMFX_RETINADOTS;
	else
		m_CamFXMode &= ~CXR_MODEL_CAMFX_RETINADOTS;
}

void CWClient_Mod::Con_ToggleDebugCamera()
{
	M_TRACE("Con_ToggleDebugCamera, mode %d -> %d\n", m_DebugCamera, (m_DebugCamera + 1) % 3);

	m_DebugCamera = (m_DebugCamera + 1) % 3;		// 0: disabled, 1: active, 2: locked

	if (m_DebugCamera == 1)
	{
		CMat4Dfp32& M = m_DebugCameraMat;
		M = m_LastRenderCamera;
		M.RotY_x_M(-0.25f);
		M.RotX_x_M(0.25f);

		if (M_Fabs(M.GetRow(0).k[2]) > 0.9f)
		{
			M.GetRow(1).CrossProd(CVec3Dfp32(0,0,1), M.GetRow(0));
			M.RecreateMatrix(0, 1);
		}
		else
		{
			CVec3Dfp32(0,0,1).CrossProd(M.GetRow(0), M.GetRow(1));
			M.RecreateMatrix(1, 0);
		}

		m_DebugCameraMovement = 0.0f;
	}
}


void CWClient_Mod::Con_DumpTextureUsage()
{
	CStr Path = m_spMapData->ResolvePath("MemUsage\\");
	CDiskUtil::CreatePath(Path);
	DumpTextureUsage(Path + "TextureUsageDump.txt", false, true);
}


void CWClient_Mod::Con_SetModelName(CStr _Name)
{
	ConOutL(CStrF("CWClient_Mod::Con_SetModelName, %s", _Name.Str()));

	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
	CStr ProfileName = pSys->GetOptions()->GetValue("GAME_PROFILE");

	m_LocalPlayer.m_Name = ProfileName;
#if defined(PLATFORM_XENON)	
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

	CCmd Cmd;
	Cmd.m_Cmd = CONTROL_SETNAME_PLAYERID;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Size = sizeof(int);
	int ID = pGameMod->m_pMPHandler->GetPlayerID(-1);
	memcpy(&Cmd.m_Data[0], (const char*)&ID, Cmd.m_Size);

	AddCmd(Cmd);
#else
	Con_StringCmd(CONTROL_SETNAME, m_LocalPlayer.m_Name);
#endif
	Con_StringCmd(CONTROL_JOINGAME, _Name);
}

void CWClient_Mod::Con_SetDarklingModelName(CStr _Name)
{
	ConOutL(CStrF("CWClient_Mod::Con_SetDarklingModelName, %s", _Name.Str()));
	Con_StringCmd(CONTROL_SETDARKLINGNAME, _Name);
}

void CWClient_Mod::Con_SetTeam(CStr _Team)
{
	MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");

	CStr ProfileName = pSys->GetOptions()->GetValue("GAME_PROFILE");
	m_LocalPlayer.m_Name = ProfileName;

	if(_Team == '1')
		m_MPTeam = 1;
	else
		m_MPTeam = 0;

#if defined(PLATFORM_XENON)	
	MACRO_GetRegisterObject(CGameContext, pGame, "GAMECONTEXT");
	CGameContextMod *pGameMod = safe_cast<CGameContextMod>(pGame);

	CCmd Cmd;
	Cmd.m_Cmd = CONTROL_SETNAME_PLAYERID;
	Cmd.m_dTime = uint8(GetCmdDeltaTime());
	Cmd.m_Size = sizeof(int);
	int ID = pGameMod->m_pMPHandler->GetPlayerID(-1);
	memcpy(&Cmd.m_Data[0], (const char*)&ID, Cmd.m_Size);

	AddCmd(Cmd);
#endif

//	Con_StringCmd(CONTROL_SETNAME, m_LocalPlayer.m_Name);
	Con_StringCmd(CONTROL_JOINTEAM, _Team);
}

// -------------------------------------------------------------------
void CWClient_Mod::Register(CScriptRegisterContext & _RegContext)
{

	MAUTOSTRIP(CWClient_Mod_Register, MAUTOSTRIP_VOID);
	CWorld_ClientCore::Register(_RegContext);

//ConOutL(CStrF("(CWClient_Mod::Register) this %.8x", this));

	CStr CmdPrefix = m_CmdPrefix;
	CStr CmdPrefix_Controls = m_CmdPrefix_Controls;


#ifdef WADDITIONALCAMERACONTROL
	_RegContext.RegFunction(CmdPrefix_Controls + "moveforwardadditional", this, &CWClient_Mod::Con_MoveForwardAdditional);
	_RegContext.RegFunction(CmdPrefix_Controls + "moverightadditional", this, &CWClient_Mod::Con_MoveRightAdditional);
	_RegContext.RegFunction(CmdPrefix_Controls + "lookvelocityadditional", this, &CWClient_Mod::Con_LookVelocityAdditional);
	_RegContext.RegFunction(CmdPrefix_Controls + "pressadditional", this, &CWClient_Mod::Con_PressAdditional);
	_RegContext.RegFunction(CmdPrefix_Controls + "releaseadditional", this, &CWClient_Mod::Con_ReleaseAdditional);
#endif

	_RegContext.RegFunction(CmdPrefix_Controls + "moveforward", this, &CWClient_Mod::Con_MoveForward);
	_RegContext.RegFunction(CmdPrefix_Controls + "movebackward", this, &CWClient_Mod::Con_MoveBackward);
	_RegContext.RegFunction(CmdPrefix_Controls + "moveleft", this, &CWClient_Mod::Con_MoveLeft);
	_RegContext.RegFunction(CmdPrefix_Controls + "moveright", this, &CWClient_Mod::Con_MoveRight);
	_RegContext.RegFunction(CmdPrefix_Controls + "moveup", this, &CWClient_Mod::Con_MoveUp);
	_RegContext.RegFunction(CmdPrefix_Controls + "movedown", this, &CWClient_Mod::Con_MoveDown);
	_RegContext.RegFunction(CmdPrefix_Controls + "lookleft", this, &CWClient_Mod::Con_LookLeft);
	_RegContext.RegFunction(CmdPrefix_Controls + "lookright", this, &CWClient_Mod::Con_LookRight);
	_RegContext.RegFunction(CmdPrefix_Controls + "lookup", this, &CWClient_Mod::Con_LookUp);
	_RegContext.RegFunction(CmdPrefix_Controls + "lookdown", this, &CWClient_Mod::Con_LookDown);
	_RegContext.RegFunction(CmdPrefix_Controls + "look", this, &CWClient_Mod::Con_Look);
	_RegContext.RegFunction(CmdPrefix_Controls + "lookvelocity", this, &CWClient_Mod::Con_LookVelocity);
	_RegContext.RegFunction(CmdPrefix_Controls + "lookvelocity_x", this, &CWClient_Mod::Con_LookVelocityX);
	_RegContext.RegFunction(CmdPrefix_Controls + "lookvelocity_y", this, &CWClient_Mod::Con_LookVelocityY);

	_RegContext.RegFunction(CmdPrefix_Controls + "press", this, &CWClient_Mod::Con_Press);
	_RegContext.RegFunction(CmdPrefix_Controls + "press_analog", this, &CWClient_Mod::Con_PressAnalog);
	_RegContext.RegFunction(CmdPrefix_Controls + "press_adc", this, &CWClient_Mod::Con_PressADC);
	_RegContext.RegFunction(CmdPrefix_Controls + "release", this, &CWClient_Mod::Con_Release);
	_RegContext.RegFunction(CmdPrefix_Controls + "releaseall", this, &CWClient_Mod::Con_ReleaseAll);
	_RegContext.RegFunction(CmdPrefix_Controls + "cmdstr", this, &CWClient_Mod::Con_StringCmd);
	_RegContext.RegFunction(CmdPrefix_Controls + "gui_prev", this, &CWClient_Mod::Con_GUI_Prev);
	_RegContext.RegFunction(CmdPrefix_Controls + "gui_next", this, &CWClient_Mod::Con_GUI_Next);
	_RegContext.RegFunction(CmdPrefix_Controls + "togglecrouch", this, &CWClient_Mod::Con_ToggleCrouch);
	_RegContext.RegFunction(CmdPrefix_Controls + "walk", this, &CWClient_Mod::Con_Walk);
	_RegContext.RegFunction(CmdPrefix_Controls + "lean", this, &CWClient_Mod::Con_Lean);
	_RegContext.RegFunction(CmdPrefix_Controls + "joingame", this, &CWClient_Mod::Con_JoinGame);

	_RegContext.RegFunction(CmdPrefix_Controls + "showdebughud", this, &CWClient_Mod::Con_ShowDebugHud);

#ifndef M_RTM
	_RegContext.RegFunction(CmdPrefix_Controls + "activateitem", this, &CWClient_Mod::Con_ActivateItem);
	_RegContext.RegFunction(CmdPrefix_Controls + "debugimpulse", this, &CWClient_Mod::Con_DebugImpulse);
	_RegContext.RegFunction(CmdPrefix_Controls + "debugmsg", this, &CWClient_Mod::Con_DebugMsg);
	_RegContext.RegFunction(CmdPrefix_Controls + "setapproachitem", this, &CWClient_Mod::Con_SetApproachItem);

	// Controls end

	_RegContext.RegFunction(CmdPrefix + "envboxsnapshot", this, &CWClient_Mod::Con_EnvBoxSnapshot);
	_RegContext.RegFunction(CmdPrefix + "dumpwnd", this, &CWClient_Mod::Con_DumpWnd);

	_RegContext.RegFunction(CmdPrefix + "hurteffect", this, &CWClient_Mod::Con_HurtEffect);
#endif

	_RegContext.RegFunction(CmdPrefix + "additem", this, &CWClient_Mod::Con_AddItem);
	_RegContext.RegFunction(CmdPrefix + "summon", this, &CWClient_Mod::Con_Summon);
	_RegContext.RegFunction(CmdPrefix + "invmenu", this, &CWClient_Mod::Con_InvMenu);
	_RegContext.RegFunction(CmdPrefix + "menutrigger", this, &CWClient_Mod::Con_MenuTrigger);
	_RegContext.RegFunction(CmdPrefix + "selectdialoguechoice", this, &CWClient_Mod::Con_SelectDialogueChoice);
	_RegContext.RegFunction(CmdPrefix + "selecttelephonechoice", this, &CWClient_Mod::Con_SelectTelephoneChoice);
	_RegContext.RegFunction(CmdPrefix + "selectitemtype", this, &CWClient_Mod::Con_SelectItemType);
	_RegContext.RegFunction(CmdPrefix + "forcenoworld", this, &CWClient_Mod::Con_ForceNoWorld);
	_RegContext.RegFunction(CmdPrefix + "toggleinvertmouse", this, &CWClient_Mod::Con_ToggleInvertMouse);
	_RegContext.RegFunction(CmdPrefix + "toggleshowhud", this, &CWClient_Mod::Con_ToggleShowHUD);

	_RegContext.RegFunction(CmdPrefix + "shaderdiff", this, &CWClient_Mod::Con_ShaderDiff);
	_RegContext.RegFunction(CmdPrefix + "shaderdiffnv", this, &CWClient_Mod::Con_ShaderDiffNV);
	_RegContext.RegFunction(CmdPrefix + "shaderspec", this, &CWClient_Mod::Con_ShaderSpec);
	_RegContext.RegFunction(CmdPrefix + "shaderspecnv", this, &CWClient_Mod::Con_ShaderSpecNV);

	_RegContext.RegFunction(CmdPrefix + "nvviewscale", this, &CWClient_Mod::Con_NVViewScale);
	_RegContext.RegFunction(CmdPrefix + "visionsenabled", this, &CWClient_Mod::Con_VisionsEnabled);
	_RegContext.RegFunction(CmdPrefix + "darknessvision", this, &CWClient_Mod::Con_DarknessVision);
	_RegContext.RegFunction(CmdPrefix + "creepingdark", this, &CWClient_Mod::Con_CreepingDark);
	_RegContext.RegFunction(CmdPrefix + "otherworld", this, &CWClient_Mod::Con_OtherWorld);
	_RegContext.RegFunction(CmdPrefix + "retinadots", this, &CWClient_Mod::Con_RetinaDots);
	
	_RegContext.RegFunction(CmdPrefix + "toggledebugcamera", this, &CWClient_Mod::Con_ToggleDebugCamera);
	_RegContext.RegFunction(CmdPrefix + "dumptextureusage", this, &CWClient_Mod::Con_DumpTextureUsage);

	// Prefix ends
	_RegContext.RegFunction("showinfoscreen", this, &CWClient_Mod::Con_ShowInfoScreen);
	_RegContext.RegFunction("cg_scrflt", this, &CWClient_Mod::Con_OverrideFilter);

	_RegContext.RegFunction("mp_setmodelname", this, &CWClient_Mod::Con_SetModelName);
	_RegContext.RegFunction("mp_setdarklingmodelname", this, &CWClient_Mod::Con_SetDarklingModelName);
	_RegContext.RegFunction("mp_setteam", this, &CWClient_Mod::Con_SetTeam);
	_RegContext.RegFunction("mp_togglemultiplayerstatus", this, &CWClient_Mod::Con_ToggleMultiplayerStatus);
	_RegContext.RegFunction("mp_setmultiplayerstatus", this, &CWClient_Mod::Con_SetMultiplayerStatus);
	_RegContext.RegConstant("mp_toggledarklinghuman", (int32)CMD_TOGGLEDARKLINGHUMAN);
	_RegContext.RegConstant("dumpautovarlog", (int32)CMD_DUMPAUTOVARLOG);

	_RegContext.RegConstant("sv_toggleweaponzoom", (int32)CMD_TOGGLEWEAPONZOOM);
	
	if (!(m_ClientMode & WCLIENT_MODE_REFERENCE))
	{
		_RegContext.RegConstant("primary", (int32)CONTROLBITS_PRIMARY);
		_RegContext.RegConstant("secondary", (int32)CONTROLBITS_SECONDARY);
		_RegContext.RegConstant("jump", (int32)CONTROLBITS_JUMP);
		_RegContext.RegConstant("crouch", (int32)CONTROLBITS_CROUCH);

		_RegContext.RegConstant("button0", (int32)CONTROLBITS_BUTTON0);
		_RegContext.RegConstant("button1", (int32)CONTROLBITS_BUTTON1);
		_RegContext.RegConstant("button2", (int32)CONTROLBITS_BUTTON2);
		_RegContext.RegConstant("button3", (int32)CONTROLBITS_BUTTON3);
		_RegContext.RegConstant("dpad_up", (int32)CONTROLBITS_DPAD_UP);
		_RegContext.RegConstant("dpad_down", (int32)CONTROLBITS_DPAD_DOWN);
		_RegContext.RegConstant("dpad_left", (int32)CONTROLBITS_DPAD_LEFT);
		_RegContext.RegConstant("dpad_right", (int32)CONTROLBITS_DPAD_RIGHT);
		_RegContext.RegConstant("button4", (int32)CONTROLBITS_BUTTON4);
		_RegContext.RegConstant("button5", (int32)CONTROLBITS_BUTTON5);

		_RegContext.RegConstant("noclip", (int32)CMD_NOCLIP);
		_RegContext.RegConstant("physlog", (int32)CMD_PHYSLOG);
		_RegContext.RegConstant("giveall", (int32)CMD_GIVEALL);
		_RegContext.RegConstant("givedarknesslevel1", (int32)CMD_GIVE_DARKNESSLEVEL1);
		_RegContext.RegConstant("givedarknesslevel2", (int32)CMD_GIVE_DARKNESSLEVEL2);
		_RegContext.RegConstant("givedarknesslevel3", (int32)CMD_GIVE_DARKNESSLEVEL3);
		_RegContext.RegConstant("givedarknesslevel4", (int32)CMD_GIVE_DARKNESSLEVEL4);
		_RegContext.RegConstant("givedarknesslevel5", (int32)CMD_GIVE_DARKNESSLEVEL5);
		_RegContext.RegConstant("givedemonarmlevel1", (int32)CMD_GIVE_DEMONARMLEVEL1);
		_RegContext.RegConstant("givedemonarmlevel2", (int32)CMD_GIVE_DEMONARMLEVEL2);
		_RegContext.RegConstant("godmode", (int32)CMD_GODMODE);
		_RegContext.RegConstant("noclip2", (int32)CMD_NOCLIP2);
		_RegContext.RegConstant("enddialogue", (int32)CMD_ENDDIALOGUE);
		_RegContext.RegConstant("closeclientwindow", (int32)CMD_CLOSECLIENTWINDOW);

		_RegContext.RegConstant("nextweapon", (int32)CMD_NEXTWEAPON);
		_RegContext.RegConstant("prevweapon", (int32)CMD_PREVWEAPON);
		_RegContext.RegConstant("weapon", (int32)CMD_WEAPON);
		_RegContext.RegConstant("nextitem", (int32)CMD_NEXTITEM);
		_RegContext.RegConstant("previtem", (int32)CMD_PREVITEM);

		_RegContext.RegConstant("selfsummon", (int32)CMD_SELFSUMMON);
		_RegContext.RegConstant("kill", (int32)CMD_KILL);
		_RegContext.RegConstant("skipingamecutscene", (int32)CMD_SKIPINGAMECUTSCENE);
		_RegContext.RegConstant("togglenightvision", (int32)CMD_TOGGLENIGHTVISION);
		_RegContext.RegConstant("cyclecamera", (int32)CMD_CYCLECAMERA);
		_RegContext.RegConstant("toggleaimassistance", (int32)CMD_TOGGLEAIMASSISTANCE);
		_RegContext.RegConstant("loadlastsave", (int32)CMD_LOADLASTSAVE);
		_RegContext.RegConstant("helpbutton", (int32)CMD_HELPBUTTON);

		_RegContext.RegConstant("gt", (int32)ADC_OP_GT);
		_RegContext.RegConstant("lt", (int32)ADC_OP_LT);

		_RegContext.RegConstant("showcontroller", (int32)CMD_SHOWCONTROLLER);
	}
}
