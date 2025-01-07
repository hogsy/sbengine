#ifndef _INC_WCLMOD
#define _INC_WCLMOD

#include "WClientMod_Defines.h"



// -------------------------------------------------------------------
//  CXR_Model_CameraEffects
// -------------------------------------------------------------------
class CXR_Model_CameraEffects : public CXR_Model
{
public:
	fp32 m_AncientWeapon;
	fp32 m_CreepingDark;
	uint m_TextureID_OWHSC;
	fp32 m_RadialBlur;
	fp32 m_RadialBlurColorIntensity;
	bool m_bRadialBlur;
	uint m_RadialBlurMode;
	CVec3Dfp32 m_RadialBlurAffection;
	CVec3Dfp32 m_RadialBlurColorScale;
	CVec2Dfp32 m_RadialBlurCenter;
	CVec2Dfp32 m_RadialBlurUVExtra;
	const char* m_pRadialBlurFilter;
	CVec4Dfp32 m_lRadialBlurFilterParams[XR_RADIALBLUR_MAX_PARAMS];
	uint m_nRadialBlurFilterParams;
	
	CXR_Model_CameraEffects();
	void PrepareVisions(const fp32& _CreepingDark, const fp32& _AncientWeapon);
	void PrepareRadialBlur(bool _bRadialBlur, const fp32 _RadialBlur, const fp32 _RadialBlurColorIntensity,
						   const CVec3Dfp32& _RadialBlurAffection, const CVec3Dfp32& _RadialBlurColorScale,
						   const CVec2Dfp32& _RadialBlurCenter, const CVec2Dfp32& _RadialBlurUVExtra,
						   uint _RadialBlurMode = CRC_TEXENVMODE_COMBINE_ADD, CVec4Dfp32* _pRadialBlurFilterParams = NULL,
						   uint _nRadialBlurFilterParams = 0, const char* _pRadialBlurFilter = NULL);
	void PrepareSimple();
	
	virtual void OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags = 0);
	virtual void OnPrecache(class CXR_Engine* _pEngine, int _iVariation);

public:

	// Render helpers
	bool ClearScreen(CXR_VBManager* _pVBM, CRC_Viewport* _pVP, CRect2Duint16& _VPRect16, const fp32& _Priority, const uint32& _Color);
	bool RenderQuad(CRC_Viewport* _pVP, CXR_VBManager* _pVBM, const CRect2Duint16& _VPRect16, const int& _bRenderTextureVertFlip, const CVec2Dfp32& _UVMin, const CVec2Dfp32& _UVMax, const uint16& _TextureID, const uint32& _Color, const fp32& _Priority, const uint32 _Enable = 0, const uint32 _Disable = 0, const uint32 _RasterMode = CRC_RASTERMODE_NONE);
	static bool RenderDownsample(const int& _nDownsamples, const int& _bRenderTextureVertFlip, CRC_Viewport* _pVP, CXR_VBManager* _pVBM, CRect2Duint16 _VPRect16, const fp32& _Priority, const int& _TextureID, const uint16& _CaptureID, const CVec2Dfp32& _UVMin, const CVec2Dfp32& _UVMax, const uint32& _Color = 0xffffffff);
};


// -------------------------------------------------------------------
//  CWClient_Mod
// -------------------------------------------------------------------
class CWClient_Mod : public CWorld_ClientCore
{
	MRTC_DECLARE;

public:
	uint8 m_bForceNoWorld : 1;

	CMTime m_Control_LastLookVelUpdate;
	CMTime m_RenderLoadingCounterTime;
	CMTime m_AutoLoadingTextStartTime;
	CMTime m_AutoLoadingTextLastTime;

	fp32 m_ShadesLevel;
	uint8 m_ShaderDiff;
	uint8 m_ShaderDiffNV;
	uint8 m_ShaderSpec;
	uint8 m_ShaderSpecNV;

	CXR_Model_CameraEffects m_CamFX_Model;
	uint16	m_CamFXMode;

	fp32 m_Control_Forward;
	fp32 m_Control_Backward;
	fp32 m_Control_Left;
	fp32 m_Control_Right;
	fp32 m_Control_Up;
	fp32 m_Control_Down;
	CVec3Dfp32 m_Control_LookAdd;
	CVec3Dfp32 m_Control_Look;
	int m_Control_Press;
	uint8 m_Control_Analog[MAX_ANALOGCHANNELS];

#ifdef WADDITIONALCAMERACONTROL
	fp32 m_Control_Forward2;
	fp32 m_Control_Backward2;
	fp32 m_Control_Left2;
	fp32 m_Control_Right2;
	fp32 m_Control_Up2;
	fp32 m_Control_Down2;
	CVec3Dfp32 m_Control_LookAdd2;
	CVec3Dfp32 m_Control_Look2;
	int m_Control_Press2;
	uint8 m_Control_Analog2[MAX_ANALOGCHANNELS];

	CMTime m_LastLookUpdateTime2;
	CMTime m_Control_LastLookVelUpdate2;
	CVec2Dfp32 m_Control_CurrentLookVelocity2;
	CVec2Dfp32 m_Control_TargetLookVelocity2;
#endif


	int m_RenderLoadingCounter;
	spCMWnd m_spWnd;
	CFStr m_Window; // not needed?
	CFStr m_DeferredWindow; // not needed?
	bool m_InGameGUIWanted;

	CFStr m_InfoScreen;

	CFStr m_Stream_Pending;
	CRC_Viewport m_3DViewPort;

	int m_TextureIDSneak0;
	int m_TextureIDNVPerturb;

	uint8 m_bRenderBorders:1;
	uint8 m_bHideHUD:1;
	uint8 m_bShowMultiplayerStatus:1;
	uint8 m_bNVViewScale:1;		// Enable/Disable viewscaling if NV is on
	uint8 m_DebugCamera:2;		// 0: disabled, 1: active, 2: locked

	/********* Variables regarding the controller input ***************************/

	CVec2Dfp32 m_Control_CurrentLookVelocity;
	CVec2Dfp32 m_Control_TargetLookVelocity;

	//CVec2Dfp32 m_ControllerSensitivity;
	CVec2Dfp32 m_Sensitivity;

	CVec2Dfp32 m_LookAcceleration;
	CMTime m_LastLookUpdateTime;
	fp32 m_DeadZone;

	uint8 m_bLeanPressed : 1;
	uint8 m_bWalkPressed : 1;

	/********* End Variables regarding the controller input ***********************/

	/**** Debug Camera ****/
	CMat4Dfp32 m_DebugCameraMat;
	CVec4Dfp32 m_DebugCameraMovement;	// Forward, Backward, Left, Right
#ifndef M_RTM
	fp32 m_DebugCameraSpeedFactor;
#endif
	/**** ****/

	int8	m_MPTeam;	//the team the player is in for TDM and CTF

	CWClient_Mod();
	~CWClient_Mod();
	virtual void Create(CWC_CreateInfo& _CreateInfo);

	virtual int GetInteractiveState();

	virtual void World_Close();
	virtual void World_Init(int _nObj, int _nRc, CStr _WName, int32 _SpawnMask);

	virtual int Net_OnProcessMessage(const CNetMsg& _Msg);
	virtual void Net_AddIdleCommand();
	virtual void Net_UpdateLook(fp32 _MinDelta);
    
	virtual bool Net_UnpackClientRegUpdate(CWObjectUpdateBuffer* _pDeltaBuffer);

	virtual CRegistry* GUI_GetWindowsResource();
	virtual void UpdateInGameGui();
	virtual void SetInventoryWindow(const CStr& _WinID);
	void SetInventoryWindowDeferred(const CStr& _WinID);

	virtual void SB_RenderInventory();
	virtual void SB_RenderMainStatus();

	virtual void RenderStatusBar(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP);
	virtual void RenderMultiplayerStatus(CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP);
	void PostRenderInterface(CMWnd* pWndTree, CRC_Util2D* _pRCUtil, CClipRect _Clip); // Postrender after rendertree specific to p3

	virtual void OnRegistryChange();
	void UpdateControllerSettings();
	virtual int GetSplitScreenMode();
	virtual bool CanRenderWorld();							// 'true' indicates the client has completed connection sequence and is now capable of rendering the game.
	
	virtual void Precache_Init();

	virtual void EngineClient_EnumerateView(CXR_Engine* _pEngine, int _iVC, int _EnumViewType);

	virtual int IsFullScreenGUI();
	virtual int IsCutscene();
	virtual bool HideHud();
	virtual void Render_SetViewport(class CXR_VBManager* _pVBM);
	virtual void Render(class CXR_VBManager* _pVBM, CRenderContext* _pRender, CRC_Viewport& _GUIVP, fp32 _InterpolationTime, int _Context);
	virtual void Render_GUI(class CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _GUIVP);
	virtual void Render_GetCamera(CMat4Dfp32& _Camera);			// NOTE: This function invokes client-prediction and, hence, is VERY costly.
	virtual void Render_Borders(class CXR_VBManager* _pVBM, CRenderContext* _pRC, CRC_Viewport& _3DVP, CRC_Viewport& _GUIVP);
	CVec2Dfp32 Render_GetViewScaleMultiplier();
	
	virtual bool Render_World_AllowTextureLoad();

	uint32 GetViewFlags();
	bool UpdateDebugCamera_Look(const CVec2Dfp32& _dLook);
	bool UpdateDebugCamera_Move(uint8 _iComponent, fp32 _Amount);
	void RefreshDebugCamera();

	
	// Environment box snapshot (This is a development tool)
	int m_EnvBox_iFace;
	int m_EnvBox_Size;
	CStr m_EnvBox_FileName;
	CVec3Dfp32 m_EnvBox_CameraPos;
	int m_OverrideFilter;


	virtual void EnvBox_BeginCapture();
	virtual void EnvBox_GetCamera(CMat4Dfp32& _Camera);
	virtual void EnvBox_SetViewPort(class CXR_VBManager* _pVBM);
	virtual void EnvBox_CaptureFrame(CXR_VBManager* _pVBM);

	virtual	bool ProcessKey(const CScanKey& _Key, const CScanKey& _OriginalKey);
	virtual void Refresh();

	virtual void Simulate(int _bCalcInterpolation);

	virtual void AddMoveCmd();

#ifdef WADDITIONALCAMERACONTROL
	virtual void AddMoveCmdAdditional();
	virtual void Con_LookAdditional(fp32 _dX, fp32 _dY);
	virtual void Con_MoveForwardAdditional(fp32 _v);
	virtual void Con_MoveRightAdditional(fp32 _v);
	virtual void Con_LookVelocityAdditional(fp32 _dX, fp32 _dY);
	
	virtual void Con_PressAdditional(int _v);
	virtual void Con_ReleaseAdditional(int _v);

	virtual void Net_UpdateLookAdditional(fp32 _MinDelta);
	virtual void RenderAdditional();
#endif

	virtual void Con_OverrideFilter(int _v);
	virtual void Con_MoveForward(fp32 _v);
	virtual void Con_MoveBackward(fp32 _v);
	virtual void Con_MoveLeft(fp32 _v);
	virtual void Con_MoveRight(fp32 _v);
	virtual void Con_MoveUp(fp32 _v);
	virtual void Con_MoveDown(fp32 _v);
	virtual void Con_LookLeft(fp32 _v);
	virtual void Con_LookRight(fp32 _v);
	virtual void Con_LookUp(fp32 _v);
	virtual void Con_LookDown(fp32 _v);
	virtual void Con_Look(fp32 _dX, fp32 _dY);
	virtual void Con_LookVelocity(fp32 _dX, fp32 _dY);
	virtual void Con_LookVelocityX(fp32 _dX);
	virtual void Con_LookVelocityY(fp32 _dY);
	virtual void Con_ToggleCrouch();
	virtual void Con_Lean(int _v);
	virtual void Con_Walk(int _v);

	virtual void Con_Press(int _v);
	virtual void Con_PressAnalog(int _Channel, fp32 _v);
	virtual void Con_PressADC(fp32 _v, int _op, fp32 _threshold, int _button);
	virtual void Con_Release(int _v);
	virtual void Con_ReleaseAll();
	virtual void Con_StringCmd(int _Cmd, CStr _Str);
	virtual void Con_StringCmdForced(int _Cmd, CStr _Str);
	virtual void Con_GUI_Next(int _ScreenID);
	virtual void Con_GUI_Prev(int _ScreenID);
	virtual void Con_JoinGame(CStr _Char);
	virtual void Con_AddItem(CStr _Item);
	virtual void Con_Summon(CStr _PlayerCls);
	virtual void Con_MenuTrigger(int _ID);
	virtual void Con_DebugMsg(CStr _Msg);
	virtual void Con_SelectItemType(int _ItemType);

#if 1
	virtual void Con_ShowDebugHud();
#endif

#ifndef M_RTM
	virtual void Con_ActivateItem(CStr _PlayerCls);
	virtual void Con_SetApproachItem(CStr _Char, CStr _iItem);
	virtual void Con_DebugImpulse(CStr _Target, int _Impulse);
	virtual void Con_EnvBoxSnapshot(CStr _Name);
	virtual void Con_DumpWnd();

	// HACK! This is currently overridden for the sole purpose of being able to detect no-clip speed factor commands and apply them to debug camera as well.
	// Even worse, it is used to get the debugcamera dir and pos back to ai debug
	virtual void Con_Cmd2(int _cmd, int d0);
	virtual void Con_HurtEffect(int _v);
#endif

	virtual void Con_ShowInfoScreen(CStr _Texture);
	virtual void Con_SelectDialogueChoice(int _iChoice);
	virtual void Con_SelectTelephoneChoice(int _iChoice);

	virtual void Con_InvMenu(CStr _Name);
	virtual void Con_OpenGameMenu();
	virtual void Con_ForceNoWorld(int _Force);
	virtual void Con_ToggleInvertMouse();
	virtual void Con_ToggleShowHUD();
	virtual void Con_ToggleMultiplayerStatus();
	virtual void Con_SetMultiplayerStatus(int _Show);

	virtual void Con_ShaderDiff(int _v);
	virtual void Con_ShaderDiffNV(int _v);
	virtual void Con_ShaderSpec(int _v);
	virtual void Con_ShaderSpecNV(int _v);

	virtual void Con_NVViewScale(int _v);
	
	virtual void Con_VisionsEnabled(int _v);
	virtual void Con_DarknessVision(int _v);
	virtual void Con_CreepingDark(int _v);
	virtual void Con_OtherWorld(int _v);
	virtual void Con_RetinaDots(int _v);

	virtual void Con_ToggleDebugCamera();
	virtual void Con_DumpTextureUsage();

	virtual void Con_SetModelName(CStr _Name);
	virtual void Con_SetDarklingModelName(CStr _Name);
	virtual void Con_SetTeam(CStr _Team);

	void Register(CScriptRegisterContext &_RegContext);
};

#endif // _INC_WCLMOD
