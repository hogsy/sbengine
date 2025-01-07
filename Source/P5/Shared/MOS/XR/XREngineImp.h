
#ifndef _INC_XREngineImp
#define _INC_XREngineImp

#include "XRPContainer.h"
#include "XRShader.h"

//#define _DEBUG_TEXTUREID_SCREEN
//#define XR_SUPPORT_SHADOWDECALS

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_Render
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CTextureContainer_Render : public CTextureContainer
{
	MRTC_DECLARE;

protected:
	class CRenderTextureInfo
	{
	public:
		int m_TextureID;
		int m_iVC;
		uint16 m_Width;
		uint16 m_Height;
		uint32 m_Format;

		CRenderTextureInfo()
		{
			m_TextureID = 0;
			m_iVC = 0;
			m_Width = 0;
			m_Height = 0;
			m_Format = IMAGE_FORMAT_BGRA8;
		}
	};

	class CXR_Engine* m_pEngine;

public:
	TArray<CRenderTextureInfo> m_lTxtInfo;

	virtual void Create(int _nTextures, class CXR_Engine* _pEngine);
	~CTextureContainer_Render();

	virtual void PrepareFrame(CPnt _TextureSize);

	virtual int GetNumLocal();
	virtual int GetTextureID(int _iLocal);
	virtual int GetTextureDesc(int _iLocal, class CImage* _pTargetImg, int& _Ret_nMipmaps);
	virtual void GetTextureProperties(int _iLocal, CTC_TextureProperties&);

	virtual void BuildInto(int _iLocal, class CImage** _ppImg, int _nMipmaps, int _TextureVersion, int _ConvertType = IMAGE_CONVERT_RGB, int _iStartMip = 0, uint32 _BuildFlags = 0);
	virtual void BuildInto(int _iLocal, class CRenderContext* _pRC) pure;
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_EnginePortals
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CTextureContainer_EnginePortals : public CTextureContainer_Render
{
	MRTC_DECLARE;

public:
	int m_iLocalEnabled;

	CTextureContainer_EnginePortals();
	virtual void BuildInto(int _iLocal, class CRenderContext* _pRC);
	virtual void GetTextureProperties(int _iLocal, CTC_TextureProperties&);
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_ShadowDecals
|__________________________________________________________________________________________________
\*************************************************************************************************/

#ifdef XR_SUPPORT_SHADOWDECALS

class CTextureContainer_ShadowDecals : public CTextureContainer_Render
{
	MRTC_DECLARE;

protected:
	int m_iNextTexture : 8;
	int m_iLastTexture : 8;
	int m_bFull : 1;
	int m_bIgnoreRender : 1;
	int m_Tick;

	class CShadowInfo
	{
	public:
		int m_GUID;
		int m_TickUsed;
		int m_TickRendered;

		CShadowInfo()
		{
			m_GUID = -1;
			m_TickUsed = 0;
			m_TickRendered = 0;
		}
	};

	TThinArray<CShadowInfo> m_lShadows;

public:
	CTextureContainer_ShadowDecals();
	virtual void PrepareFrame(CPnt _TextureSize);

	virtual int Shadow_Begin(const CMat4Dfp32& _CameraWMat, fp32 _ProjSize, int _GUID, int _Update);
	virtual void Shadow_AddModel(int _hShadow, class CXR_Model* _pModel, const CMat4Dfp32& _Pos, const CXR_AnimState& _Anim);
	virtual void Shadow_End(int _hShadow);
	virtual int Shadow_GetTextureID(int _hShadow);

	virtual int Shadow_GetNumUsed();

	virtual void BuildInto(int _iLocal, class CRenderContext* _pRC);
};

#endif

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_Screen
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CTextureContainer_Screen : public CTextureContainer_Render
{
	MRTC_DECLARE;

	TThinArray<CTC_TextureProperties> m_lProperties;
	TThinArray<uint32> m_lFormats;

public:
	virtual void SetTextureProperties(int _iLocal, CTC_TextureProperties& _Prop);
	void SetTextureFormat(int _iLocal, uint32 _Format);

	CTextureContainer_Screen();
	virtual void Create(int _nTextures, class CXR_Engine* _pEngine);
	virtual void PrepareFrame(CPnt _TextureSize);
	virtual void GetSnapshot(int _iScreen, class CRenderContext *_pRC=NULL);
	virtual int GetTextureDesc(int _iLocal, class CImage* _pTargetImg, int& _Ret_nMipmaps);
	virtual void GetTextureProperties(int _iLocal, CTC_TextureProperties& _Prop);
	virtual void BuildInto(int _iLocal, class CRenderContext* _pRC);

#ifdef _DEBUG_TEXTUREID_SCREEN
	virtual int GetTextureID(int _iLocal);
#endif
};

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_ScreenCopy
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CTextureContainer_ScreenCopy : public CTextureContainer_Screen
{
	MRTC_DECLARE;

public:
	virtual void GetTextureProperties(int _iLocal, CTC_TextureProperties& _Prop);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CTextureContainer_RenderCallback
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CTextureContainer_RenderCallback : public CTextureContainer_Render
{
	MRTC_DECLARE;
public:
	typedef void (*CALLBACKPROC)(int _iLocal, class CRenderContext* _pRC, void *_pData);
	void *m_pData;
	CALLBACKPROC m_pfnCallback;

	CTextureContainer_RenderCallback();
	void SetCallback(CALLBACKPROC _pfnProc, void *_pData)
	{
		m_pData = _pData;
		m_pfnCallback = _pfnProc;
	}
	virtual void BuildInto(int _iLocal, class CRenderContext* _pRC);
};


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_ViewContextImpl
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_ViewContextImpl : public CXR_ViewContext
{
public:
	CXR_ViewContextImpl();
	~CXR_ViewContextImpl();

	fp32 m_LODScale;
#ifndef	CPU_SOFTWARE_FP64
	CPlane3Dfp64 m_ClipPlanesW8[16];
	CPlane3Dfp64 m_FrontPlaneW8;
#endif	// CPU_SOFTWARE_FP64
	CXR_AnimState m_SkyAnimState;

	virtual void Create(class CXR_Engine* _pEngine, int _MaxObjects, int _Depth, int _MaxPortals);
	virtual void OnPrecache();
	virtual void Clear(const CMat4Dfp32& _CameraWMat, const CMat4Dfp32& _CameraWVelMat, const CPlane3Dfp32& _FrontPlaneW, const CPlane3Dfp32& _BackPlaneW);

	virtual void AddModel(class CXR_Model* _pModel, const CMat4Dfp32& _Pos, const CXR_AnimState& _Anim, int _OnRenderFlags);
	virtual void AddWorld(class CXR_Model* _pModel, const CMat4Dfp32& _Pos, const CXR_AnimState& _Anim, int _OnRenderFlags);
	virtual void AddSky(class CXR_Model_Sky* _pModel, const CMat4Dfp32& _Pos, const CXR_AnimState& _Anim);
	virtual void Register(CScriptRegisterContext &_Context)
	{
	}
};

typedef TPtr<CXR_ViewContextImpl> spCXR_ViewContextImpl;

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_EngineImpl
|__________________________________________________________________________________________________
\*************************************************************************************************/
class CXR_EngineImpl : public CXR_Engine
{
	MRTC_DECLARE;

public:
	spCRC_Font m_spDebugFont;

	CXR_WorldLightState m_TempLightState;

	uint32 m_bWorldOnly : 1;
	uint32 m_bObjectsOnly : 1;
	uint32 m_bPortalsOnly : 1;
	uint32 m_ShowTiming : 4;
	uint32 m_bShowFogInfo : 1;
	uint32 m_bShowRecurse : 1;
	uint32 m_bShowPortalFence : 1;
	uint32 m_bStencilShadows : 1;
	uint32 m_bNoRenderToTexture : 1;
	uint32 m_bGeneratedSurfaces : 1;
	uint32 m_DrawMode : 2;
	uint32 m_bFreeze : 1;
	uint32 m_bClearViewport : 1;
	uint32 m_bSyncOnRender : 1;
	uint32 m_PostProcess_MBDebugFlags : 4;

	int m_iiTV0; int m_diiTV0;
	int m_iiTV1; int m_diiTV1;
	int m_iiTV2; int m_diiTV2;
	int m_iHexagonColor; int m_dHexagonColor;
	spCTextureContainer_Plain m_spHexagonTest;

	uint16 m_TextureID_MotionMap;
	fp32 m_PostProcess_MBMaxRadius;		// Viewport width factor

	fp32 m_LODScaleBias;
	int m_VBSkip;
	int m_nRecurse;
	int m_PortalTextureSize;
	int m_ShadowDecalTextureSize;
	fp32 m_FogCullOffset;
	fp32 m_FogCullOffsetOverride;

	int m_VBMFlags;
	CMTime m_RenderTime;			// As in GameTime + RenderTimeFrac

	TArray<spCXR_ViewContextImpl> m_lspVC;
	int m_iCurrentVC;

	// VBM
	int m_iCurrentVBMDepth;
	int m_nMaxVB;	// Maximum scopesize, depends on env. var. XR_MAXVBCOUNT

	// Unified
	int m_Unified_Clear;
	int m_Unified_Ambience;

	uint32 m_PBNightVisionLight;
	CXR_Shader m_Shader;

	// Portal texture container
	TPtr<CTextureContainer_EnginePortals> m_spTCPortals;
	int m_iNextPortalTexture;

	// Shadow decal texture container
#ifdef XR_SUPPORT_SHADOWDECALS
	TPtr<CTextureContainer_ShadowDecals> m_spTCShadowDecals;
#endif

	// Screen texture container
	TPtr<CTextureContainer_Screen> m_spTCScreen;
	TPtr<CTextureContainer_Screen> m_spTCDepthStencil;
	TPtr<CTextureContainer_Screen> m_spTCColorCorrection;
	TPtr<CTextureContainer_Screen> m_spTCColorCorrectionTemp;	// Hexagon temp container

	class CXR_EngineClient* m_pClient;

	MRTC_CriticalSection m_RenderAddLock;

	void fp_UpdateEnvironment();

public:
	CXR_EngineImpl();
	~CXR_EngineImpl();
	virtual void Create(int _MaxRecurse, int _CreateFlags);
	virtual void SetDebugFont(spCRC_Font _spFont);
	virtual void OnPrecache();


	virtual void Create_PortalTextureContainer();
	virtual void Create_ShadowDecalTextureContainer();
	virtual void Create_ScreenTextureContainer();
	virtual void Create_ColorCorrectionTextureContainer();

	virtual void RenderModel(class CXR_VCModelInstance* _pObjInfo, class CXR_ViewClipInterface* _pViewClip, int _Flags);

	// Add portals, return value is portal-number, -1 if failed. (only used internaly by CXR_Engine)
	virtual int Render_AddPortal(const CVec3Dfp32* _pPortal, int _nVertices, const CMat4Dfp32& _WCam, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, CPixel32 _SurfColor = 0x00000000, class CXR_PortalSurfaceRender* _pSurfaceRender = NULL, void* _pSurfContext = NULL);
	virtual int Render_AddMirror(const CVec3Dfp32* _pPortal, int _nVertices, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, CPixel32 _SurfColor = 0x00000000, class CXR_PortalSurfaceRender* _pSurfaceRender = NULL, void* _pSurfContext = NULL);

	// Add texture-portal, return value is TextureID, zero if failed.
	virtual int Render_AddPortal_Texture(const CVec3Dfp32* _pPortal, int _nVertices, const CMat4Dfp32& _WCam, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);
	virtual int Render_AddMirror_Texture(const CVec3Dfp32* _pPortal, int _nVertices, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat);

	virtual bool Render_AddModel(class CXR_Model* _pModel, const CMat4Dfp32& _Pos, const CXR_AnimState& _Anim, int _ModelType = XR_MODEL_STANDARD, int _OnRenderFlags = 0);

	virtual void Render_Light_AddDynamic(const CXR_Light& _Light, int _Priority = 0);
	virtual void Render_Light_AddDynamic(const CVec3Dfp32& _Pos, const CVec3Dfp32& _Intensity, fp32 _Range, int _Flags, int _Type, int _Priority = 0);
	virtual void Render_Light_AddStatic(const CXR_Light& _Light);
	virtual void Render_Light_AddStatic(int _LightID, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Intensity, fp32 _Range, int _Flags, int _Type);
	virtual void Render_Light_State(int _LightID, const CVec3Dfp32& _Intensity);

	virtual int GetMode() { return m_EngineMode; }
	virtual int GetSceneType() { return GetVC()->m_SceneType; };
	virtual int GetVCDepth() { return m_iCurrentVC; };
	virtual int GetDebugFlags() { return m_DebugFlags; };
	virtual class CXR_VBManager* GetVBM();
	virtual class CXR_ViewContext* GetVC();
	virtual class CXR_WorldLightState* GetWLS();
	virtual class TPtr<CXR_FogState> GetFogState();
	virtual class CXR_Portal* GetPortal(int _iPortal);
	virtual class CXR_SurfaceContext* GetSC();
	virtual class CTextureContext* GetTC();

	virtual void VBM_End();
	virtual void VBM_Begin(int _nMaxVB = 0);
	virtual void VBM_ClearDependencies();

	// Occlusion culling, these functions can only be used when running OnClientRender on object that does NOT have CWO_CLIENTFLAGS_VISIBILITY on them.
	virtual bool View_GetClip_Sphere(const CVec3Dfp32& _Origin, fp32 _Radius, int _MediumFlags, int _ObjectMask, class CRC_ClipVolume* _pClipVolume = NULL, class CXR_RenderInfo* _pRenderInfo = NULL);
	virtual bool View_GetClip_Box(const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax, int _MediumFlags, int _ObjectMask, class CRC_ClipVolume* _pClipVolume = NULL, class CXR_RenderInfo* _pRenderInfo = NULL);

	virtual void Engine_PushViewContext(const CMat4Dfp32& _CameraWMat, const CMat4Dfp32& _CameraWVelMat, const CVec3Dfp32& _VisPos, const CRC_ClipVolume& _Clip, const CPlane3Dfp32& _FrontPlaneW, int _SceneType = XR_SCENETYPE_MAIN);
	virtual void Engine_PopViewContext();
	virtual void Engine_BuildViewContext(const CMat4Dfp32& _CameraWMat, const CMat4Dfp32& _CameraWVelMat, const CVec3Dfp32& _VisPos, const CRC_ClipVolume& _Clip, const CPlane3Dfp32& _FrontPlaneW, int _SceneType = XR_SCENETYPE_MAIN);

protected:
	virtual void Engine_SetCurrentVC(int _iVC);
	// These are called from Engine_RenderViewContext()
	void Engine_RVC_RenderSky(CXR_ViewContextImpl* _pVC);
	virtual void Engine_RVC_PrePortal(class CXR_ViewContextImpl* _pVC);
	virtual void Engine_RVC_RenderPortals(class CXR_ViewContextImpl* _pVC);
	virtual void Engine_RVC_RestorePortals(CXR_ViewContextImpl* _pVC);

public:
	virtual void Engine_RenderViewContext();

	void Engine_CopyHexagonToCube(CXR_VBManager* _pVBM, CVec3Dfp32* _pVHexagon, int _SrcTextureID, int _DstTextureID);
	void Engine_CreateColorCorrection(CXR_VBManager* _pVBM, const CXR_Engine_PostProcessParams *M_RESTRICT _pParams, int _TempTextureID, uint16 _lDstTextureID[4]);
	virtual void Engine_PostProcess(CXR_VBManager* _pVBM, CRC_Viewport& _3DVP, const CXR_Engine_PostProcessParams *M_RESTRICT _pParams);

	virtual void Engine_SetRenderContext(class CRenderContext* _pRC);
	void Engine_InitRenderTextureProjection();
	void Engine_InitRenderTextures();
	void Engine_ClearRenderTextures();

	virtual void Engine_Render(CXR_EngineClient* _pClient, class CXR_VBManager* _pVBM, class CRenderContext* _pRC, const CMat4Dfp32& _CameraWMat, const CMat4Dfp32& _CameraWVelMat, class CXR_SceneGraphInstance* _pSceneGraphInstance = NULL, class CXR_TriangleMeshDecalContainer* _pTMDC = NULL);
	virtual void Engine_ClearReferences();

//	virtual void SetDefaultAttrib(class CRC_Attributes* _pAttr);

	virtual CObj* GetInterface(int _Var);
	virtual int GetVar(int _Var);
	virtual fp32 GetVarf(int _Var);
	virtual void SetVar(int _Var, aint _Value);
	virtual void SetVarf(int _Var, fp32 _Value);

	virtual CMTime GetEngineTime();
	virtual void SetEngineTime(CMTime &_Time);

#ifdef MRTC_ENABLE_REMOTEDEBUGGER
	int m_bPendingRecordVBHeap;
	void Con_RecordVBHeap()
	{
		m_bPendingRecordVBHeap = 2;
	}
#endif

	void Con_DebugFlags(int _v);
	void Con_LightDebugFlags(int _v);
	void Con_Flares(int _v);
	void Con_DLight(int _v);
	void Con_FastLight(int _v);
	void Con_Wallmarks(int _v);
	void Con_ShadowDecals(int _v);
	void Con_LODOffset(fp32 _v);
	void Con_LODScaleBias(fp32 _v);
	void Con_ShowBoundings(int _v);
	void Con_WorldOnly(int _v);
	void Con_ObjectsOnly(int _v);
	void Con_PortalsOnly(int _v);
	void Con_Sky(int _v);
	void Con_ShowTiming(int _v);
	void Con_ShowVBTime(int _v);
	void Con_ShowFogInfo(int _v);
	void Con_ShowRecurse(int _v);
	void Con_ShowPortalFence(int _v);
	void Con_VertexFog(int _v);
	void Con_DepthFog(int _v);
	void Con_NHF(int _v);
	void Con_SurfOptions(int _v);
	void Con_StencilShadows(int _v);
	void Con_PortalTextureSize(int _v);
	void Con_ShadowDecalTextureSize(int _v);
	void Con_UnifiedClear(int _v);
	void Con_UnifiedAmbience(int _r,int _g,int _b);
	void Con_TempTLEnable(int _v);
	void Con_ShowVBTypes(int _v);
	void Con_GeneratedSurfaces(int _v);
	void Con_DrawMode(int _v);
	void Con_Freeze(int _v);
	void Con_ClearViewport(int _v);
	void Con_FogCullOffset(int _v);
	void Con_XRModeOverride(int _v);
	void Con_SyncOnRefresh(int _v);

	void Con_PPDebugFlags(int _v);
	void Con_PPGlowExp(fp32 _Exp);
	void Con_PPGlowBias(fp32 _r, fp32 _g, fp32 _b, fp32 _a);
	void Con_PPGlowScale(fp32 _r, fp32 _g, fp32 _b);
	void Con_PPGlowGamma(fp32 _r, fp32 _g, fp32 _b);
	void Con_PPGlowCenter(fp32 _u, fp32 _v);
	void Con_PPExposureExp(fp32 _r, fp32 _g, fp32 _b);
	void Con_PPExposureScale(fp32 _r, fp32 _g, fp32 _b);
	void Con_PPExposureContrast(fp32 _Contrast);
	void Con_PPExposureSaturation(fp32 _Saturation);
	void Con_PPExposureBlackLevel(fp32 _BlackLevel);
	void Con_PPToggleExposureDebug();
	void Con_PPExposureDebug(int _v);
	void Con_PPToggleDynamicExposure();

	void Con_PPMBMaxRadius(fp32 _v);
	void Con_PPMBDebugFlags(int _v);

	void Con_BSPDebugFlags(int _Flags);
	void Con_BSPToggleDebugFlags(int _Flags);

	void Con_HexCubeTV0(int _iiTV0, int _diiTV0);
	void Con_HexCubeTV1(int _iiTV1, int _diiTV1);
	void Con_HexCubeTV2(int _iiTV2, int _diiTV2);
	void Con_HexagonColor(int _iCol, int _dCol);
	void Con_HexagonTest(CStr _Name);

	void Register(CScriptRegisterContext &_RegContext);

	friend class CXR_FogState;
};

#endif // _INC_XREngineImp
