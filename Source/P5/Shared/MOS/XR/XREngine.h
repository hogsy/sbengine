
/*------------------------------------------------------------------------------------------------
NAME:		XREngine.cpp/h
PURPOSE:	Extended Reality Engine
CREATION:	981003
AUTHOR:		Magnus Högdahl
COPYRIGHT:	(c) Copyright 1996 Magnus Högdahl

CONTENTS:
-

MAINTAINANCE LOG:

981003:		Created by moving & cleaning up code from CWorld_Client. The rendering code 
is now more versatile and can be used for other purposes without the 
client/server-contexts.

??????:		Lots of stuff..

010317:		Some recursion logic exposed to allow scene recursion by external force.
Shadow-decal rendering support.

------------------------------------------------------------------------------------------------*/
#ifndef _INC_XREngine
#define _INC_XREngine

#include "XRClass.h"
#include "../Classes/Render/MRenderUtil.h"	// Bad dependency, fix later..
//#include "MRenderVB.h"

#define CXR_DEFAULTPORTALDEPTH 8		// Max view-context recursion-depth.
#define CXR_MAXPORTALS 32				// Max portals in a view-context.
#define CXR_MAXPORTALFACES 64			// Max portals in a view-context.

enum
{
	// XR global rendering mode
	XR_MODE_LIGHTMAP			= 0,	// Lightmap- and vertex-lit scene. (Enclave)
	XR_MODE_UNIFIED				= 1,	// Unified shadow & illumination mode.

	// Optional parameter for Render_InitViewContext(), Render_BuildViewContext()
	XR_SCENETYPE_MAIN			= 0,	// First recursion of normal scenes.
	XR_SCENETYPE_PORTAL			= 1,	// Mirrors and portals.
	XR_SCENETYPE_TEXTUREPORTAL	= 2,	// Mirrors and portals rendered to texture.
	XR_SCENETYPE_SHADOWDECAL	= 3,	// Models rendered to texture for shadow projection.

	// GetDebugFlags()
	XR_DEBUGFLAGS_SHOWSOLIDS	= 1,
	XR_DEBUGFLAGS_SHOWBOUNDINGVOLUMES = 2,
	XR_DEBUGFLAGS_SHOWLIGHTCOUNT = 4,
	XR_DEBUGFLAGS_SHOWOCTAAABB = 8,

	// Optional parameter for Render_AddModel()
	XR_MODEL_STANDARD			= 0,
	XR_MODEL_WORLD				= 1,
	XR_MODEL_SKY				= 2,

	XR_ENGINECREATE_NOCONSOLE	= 1,

	XR_VIEWFLAGS_WIDESCREEN		= 1,

};


enum
{
	XR_PPMAXHSC					= 4,

	XR_PPDEBUGFLAGS_NOGAUSS_X	= 1,
	XR_PPDEBUGFLAGS_NOGAUSS_Y	= 2,
	XR_PPDEBUGFLAGS_NOGLOW		= 4,
	XR_PPDEBUGFLAGS_NOFINAL		= 8,
	XR_PPDEBUGFLAGS_SHOWGLOW	= 16,
	XR_PPDEBUGFLAGS_NOOVERRIDE	= 32,
	XR_PPDEBUGFLAGS_EXPOSUREDEBUG= 64,
	XR_PPDEBUGFLAGS_TRACEHISTOGRAM= 128,
};

class CXR_Engine_PostProcessParams
{
public:
	// Hue Saturation Correction
	fp32 M_ALIGN(16) m_lHSC_Intensity[XR_PPMAXHSC];
	fp32 M_ALIGN(16) m_lHSC_Blend[XR_PPMAXHSC];
	uint16 m_lHSC_TextureID[XR_PPMAXHSC];
	uint m_nHSC;

	uint m_DebugFlags : 31;
	uint m_bDynamicExposure : 1;
	const char* m_pGlowFilter;
	fp32 m_GlowAttenuationExp;
	CVec4Dfp32 m_GlowBias;
	CVec3Dfp32 m_GlowScale;
	CVec3Dfp32 m_GlowGamma;
	CVec2Dfp32 m_GlowCenter;
	CVec3Dfp32 m_ExposureExp;
	CVec3Dfp32 m_ExposureScale;
	fp32 m_ExposureContrast;
	fp32 m_ExposureSaturation;
	fp32 m_ExposureBlackLevel;

	uint m_ViewFlags;
	fp32 m_AspectChange;

	void SetDefault();

	void SetHSC(int _iHSC, fp32 _Intensity, fp32 _Blend, uint _TextureID)
	{
		m_lHSC_Intensity[_iHSC] = _Intensity;
		m_lHSC_Blend[_iHSC] = _Blend;
		m_lHSC_TextureID[_iHSC] = _TextureID;
	}
};

// -------------------------------------------------------------------
//  CXR_Portal
// -------------------------------------------------------------------
class CXR_Engine;

class CXR_PortalSurfaceRender
{
public:
	// Called immediately after a portal's view-context has been rendered.
	virtual void RenderPortalSurface(CXR_Engine* _pEngine, void* _pSurfContext, const CVec3Dfp32* _pVertices, int _nVertices, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat) pure;
};

// -------------------------------------------------------------------
class CXR_Portal : public CReferenceCount
{
public:
	CPlane3Dfp32 m_Plane;
	CPlane3Dfp32 m_PlaneDest;
	CRC_ClipVolume m_Portal;
	CRC_ClipVolume m_PortalDest;
	CVec3Dfp32 m_VisPos;
	CMat4Dfp32 m_WCam;							// Destination camera
	CMat4Dfp32 m_dCam;							// Transform between src and dst cameras.
	CMat4Dfp32 m_ModelWMat;
	CMat4Dfp32 m_ModelVMat;
	fp32 m_SortZ;
	CPixel32 m_SurfColor;						// Used only if no SurfRender was specified.

	// Callback
	CXR_PortalSurfaceRender* m_pSurfRender;		// May be NULL.
	void* m_pSurfContext;

	CRC_Viewport m_Viewport;

	int m_iLocalTexture;
	int m_TextureID;

	void Clear();

	CXR_Portal();
	const CXR_Portal& operator= (const CXR_Portal& _Src);
	bool IsTexture() { return m_TextureID != 0; };
};

// -------------------------------------------------------------------
//  CXR_VCModelInstance, View-context model instance.
// -------------------------------------------------------------------
class CXR_VCModelInstance
{
public:
	CMat4Dfp32 m_Pos;
	CMat4Dfp32 m_Velocity;
	CXR_AnimState m_Anim;
#ifdef M_Profile
	CMTime m_Time;
#endif
	CXR_Model* m_pModel;
	int m_RenderPass;
	int m_OnRenderFlags;

	CXR_VCModelInstance()
	{
	}

	CXR_VCModelInstance(CXR_Model* _pModel, const CMat4Dfp32& _Pos, const CXR_AnimState& _Anim, int _RenderPass = 0, int _OnRenderFlags = 0)
	{
		m_pModel = _pModel->OnResolveVariationProxy(_Anim, m_Anim);
		m_Pos = _Pos;
		m_RenderPass = _RenderPass;
		m_OnRenderFlags = _OnRenderFlags;
		m_Velocity.Unit();
	}

	void Create(CXR_Model* _pModel, const CMat4Dfp32& _Pos, const CXR_AnimState& _Anim, int _RenderPass = 0, int _OnRenderFlags = 0)
	{
		m_Pos = _Pos;
		m_Velocity.Unit();
		m_pModel = _pModel->OnResolveVariationProxy(_Anim, m_Anim);
		m_RenderPass = _RenderPass;
		m_OnRenderFlags = _OnRenderFlags;
	}
};

// -------------------------------------------------------------------
//  CXR_ViewContext
// -------------------------------------------------------------------
class CXR_FogState;
typedef TPtr<CXR_FogState> spCXR_FogState;

class CXR_ViewContext : public CConsoleClient
{
protected:
	int m_RecursionDepth;
	class CXR_Engine* m_pEngine;

public:
	int m_SceneType;

	CXR_VBManager* m_pVBM;
	spCXR_WorldLightState m_spLightState;
	spCXR_FogState m_spFogState;
	CXR_Model* m_pSky;

	CRC_ClipVolume m_Clip;	// Viewspace
	CRC_ClipVolume m_ClipW;	// Worldspace
	CRC_ClipVolume m_Portal;// Worldspace

	TThinArray<CXR_Portal> m_lPortals;
	TThinArray<CXR_Portal*> m_lpPortalsSorted;
	uint m_nPortals;
	uint m_nPortalsSorted;

	TThinArray<CXR_VCModelInstance> m_lObjects;
	CXR_VCModelInstance* m_pObjects;

	CXR_ViewClipInterface* m_pViewClip;

	CXR_VCModelInstance m_World;
	CMat4Dfp32 m_CameraWMat;
	CMat4Dfp32 m_dCameraWMat;
	CMat4Dfp32 m_W2VMat;
	CMat4Dfp32 m_dW2VMat;

	CVec3Dfp32 m_VisPos;
	CPlane3Dfp32 m_FrontPlaneW;
	CPlane3Dfp32 m_BackPlaneW;

	int m_MaxObjects;
	int m_nObjects;
	uint m_bIsMirrored : 1;
	uint m_bWorldFound : 1;
	uint m_bNeedResolve_TempFixMe : 1;

	virtual void Create(CXR_Engine* _pEngine, int _MaxObjects, int _Depth, int _MaxPortals) pure;
	virtual void OnPrecache() pure;

	virtual void ClearReferences();

	virtual class CXR_Engine* GetEngine() { return m_pEngine; };
	virtual CXR_Model* GetSky() { return m_pSky; };
	virtual int GetDepth() const { return m_RecursionDepth; };
	virtual CXR_WorldLightState *GetLightState();

	static void ReflectMatrix(const CPlane3Dfp32& _Plane, const CMat4Dfp32& _Mat, CMat4Dfp32& _RefMat);

	friend class CXR_EngineImpl;
};

typedef TPtr<CXR_ViewContext> spCXR_ViewContext;

// -------------------------------------------------------------------
enum
{
	CXR_ENGINE_EMUMVIEWTYPE_NORMAL = 0,
	CXR_ENGINE_EMUMVIEWTYPE_VIEWCLIP = 1,
};

class CXR_EngineClient
{
public:
	virtual void EngineClient_EnumerateView(CXR_Engine* _pEngine, int _iVC, int _EnumViewType) pure;
	virtual void EngineClient_Refresh() pure;
};

// -------------------------------------------------------------------
//  CXR_Engine
// -------------------------------------------------------------------
class CXR_Engine : public CConsoleClient
{
	/*
	MRTC_DECLARE;
	*/

public:
	class CSystem* m_pSystem;
	class CTextureContext* m_pTC;
	class CXR_SurfaceContext* m_pSC;
	class CXR_VBContext* m_pVBC;
	class CXR_SceneGraphInstance* m_pSceneGraphInstance;
	class CXR_ViewClipInterface* m_pViewClip;	// Don't assume this pointer is valid, ever
	class CXR_TriangleMeshDecalContainer* m_pTMDC;
	class CRenderContext* m_pRender;		// Only valid during a Engine_Render() call
	class CXR_Shader* m_pShader;			// Only valid during a Engine_Render() call
	class CXR_FogState* m_pCurrentFogState; // Only valid during a Engine_Render() call, points to current VC's fogstate
	class CXR_VBManager* m_pVBM;

	int m_EngineMode;
	int m_DebugFlags;
	int m_LightDebugFlags;
	int m_BSPDebugFlags;
	fp32 m_LODOffset;
	fp32 m_LightScale;
	fp32 m_LightScaleRecp;
	int m_bFastLight;
	int m_ShowVBTypes;
	int m_bTLEnableEngine;
	int m_bTLEnableRenderer;
	int m_bTLEnableEnabled;

	uint32 m_bFlares : 1;
	uint32 m_bDynLight : 1;
	uint32 m_bWallmarks : 1;
	uint32 m_bShadowDecals : 1;
	uint32 m_bShowBoundings : 1;
	uint32 m_bSky : 1;
	uint32 m_bAllowDepthFog : 1;
	uint32 m_bAllowVertexFog : 1;
	uint32 m_bAllowNHF : 1;
	uint32 m_bCurrentVCMirrored : 1;

	int32 m_lEngineUserColors[16]; // Mapped as UserColor 16-31
	CVec4Dfp32 m_RenderSurfaceLightScale;

	int m_RenderCaps_Flags;
	int m_RenderCaps_nMultiTexture;				// Shader pipeline textures limit
	int m_RenderCaps_nMultiTextureEnv;			// Fixed-function pipeline limit, <= m_RenderCaps_nMultiTextureEnv
	int m_RenderCaps_nMultiTextureCoords;		// Max texture coordinates, <= m_RenderCaps_nMultiTexture
	int m_SurfOptions;
	int m_SurfCaps;

	// Variables shadowed from viewcontext
	fp32 m_LODScale;
	int m_SceneType;
	int m_ShadowDecals;

	CMat4Dfp32 m_Screen_TextureProjection;
	CVec2Dfp32 m_Screen_PixelUV;
	CVec2Duint16 m_Screen_TextureSize;
	CVec2Duint16 m_Screen_Size;					// This is not necessarily the current rendering viewport size, so don't use it like that.
	uint16 m_TextureID_Screen;
	uint16 m_TextureID_ResolveScreen;
	uint16 m_TextureID_PostProcess;				// This should be removed
	uint16 m_TextureID_Depth;
	uint16 m_TextureID_DeferredNormal;
	uint16 m_TextureID_DeferredDiffuse;
	uint16 m_TextureID_DeferredSpecular;
	uint16 m_TextureID_DefaultEnvMap;

	// Soft-shadows special
	uint16 m_TextureID_DepthStencil;
	uint16 m_TextureID_ShadowMask;

	CXR_Engine_PostProcessParams m_PostProcessParams;
	CXR_Engine_PostProcessParams m_PostProcessParams_Current;
	fp32 m_PostProcess_PrevLevels;				// Ask before using this. -mh

	CXR_Engine();
	virtual void Create(int _MaxRecurse, int _CreateFlags) pure;
	virtual void OnPrecache() pure;
	virtual void SetDebugFont(spCRC_Font _Font) pure;

	virtual void RenderModel(CXR_VCModelInstance* _pObjInfo, CXR_ViewClipInterface* _pViewClip, int _Flags) pure;

	// Add portals, return value is portal-number, -1 if failed. Note that each VC has it's own portal-list
	virtual int Render_AddPortal(const CVec3Dfp32* _pPortal, int _nVertices, const CMat4Dfp32& _WCam, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, CPixel32 _SurfColor = 0x00000000, CXR_PortalSurfaceRender* _pSurfaceRender = NULL, void* _pSurfContext = NULL) pure;
	virtual int Render_AddMirror(const CVec3Dfp32* _pPortal, int _nVertices, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, CPixel32 _SurfColor = 0x00000000, CXR_PortalSurfaceRender* _pSurfaceRender = NULL, void* _pSurfContext = NULL) pure;

	// Add texture-portal, return value is portal-number, -1 if failed. Note that each VC has it's own portal-list
	virtual int Render_AddPortal_Texture(const CVec3Dfp32* _pPortal, int _nVertices, const CMat4Dfp32& _WCam, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat) pure;
	virtual int Render_AddMirror_Texture(const CVec3Dfp32* _pPortal, int _nVertices, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat) pure;

	virtual bool Render_AddModel(CXR_Model* _pModel, const CMat4Dfp32& _Pos, const CXR_AnimState& _Anim, int _ModelType = XR_MODEL_STANDARD, int _OnRenderFlags = 0) pure;

	virtual void Render_Light_AddDynamic(const CXR_Light& _Light, int _Priority = 0) pure;
	virtual void Render_Light_AddDynamic(const CVec3Dfp32& _Pos, const CVec3Dfp32& _Intensity, fp32 _Range, int _Flags, int _Type, int _Priority = 0) pure;
	virtual void Render_Light_AddStatic(const CXR_Light& _Light) pure;
	virtual void Render_Light_AddStatic(int _LightID, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Intensity, fp32 _Range, int _Flags, int _Type) pure;
	virtual void Render_Light_State(int _LightID, const CVec3Dfp32& _Intensity) pure;

	// Occlusion culling, these functions can only be used when running OnClientRender on object that does NOT have CWO_CLIENTFLAGS_VISIBILITY on them.
	virtual bool View_GetClip_Sphere(const CVec3Dfp32& _Origin, fp32 _Radius, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume = NULL, CXR_RenderInfo* _pRenderInfo = NULL) pure;
	virtual bool View_GetClip_Box(const CVec3Dfp32& _BoxMin, const CVec3Dfp32& _BoxMax, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume = NULL, CXR_RenderInfo* _pRenderInfo = NULL) pure;

	// Advanced usage, you'd better know what you're doing before using these:
	virtual void Engine_PushViewContext(const CMat4Dfp32& _CameraWMat, const CMat4Dfp32& _CameraWVelMat, const CVec3Dfp32& _VisPos, const CRC_ClipVolume& _Clip, const CPlane3Dfp32& _FrontPlaneW, int _SceneType = XR_SCENETYPE_MAIN) pure;
	virtual void Engine_PopViewContext() pure;

	virtual void Engine_BuildViewContext(const CMat4Dfp32& _CameraWMat, const CMat4Dfp32& _CameraWVelMat, const CVec3Dfp32& _VisPos, const CRC_ClipVolume& _Clip, const CPlane3Dfp32& _FrontPlaneW, int _SceneType = XR_SCENETYPE_MAIN) pure;
	virtual void Engine_RenderViewContext() pure;
	virtual void Engine_PostProcess(CXR_VBManager* _pVBM, CRC_Viewport& _3DVP, const CXR_Engine_PostProcessParams *M_RESTRICT _pParams) pure;

	// Standard client entry point
	virtual void Engine_SetRenderContext(CRenderContext* _pRC) pure;
	virtual void Engine_Render(CXR_EngineClient *_pEngineClient, class CXR_VBManager* _pVBM, CRenderContext* _pRC, const CMat4Dfp32& _CameraWMat, const CMat4Dfp32& _CameraWVelMat, CXR_SceneGraphInstance* _pSceneGraphInstance = NULL, class CXR_TriangleMeshDecalContainer* _pTMDC = NULL) pure;
	virtual void Engine_ClearReferences() pure;

	// State & interface access:
	virtual int GetMode() pure;
	virtual int GetSceneType() pure;
	virtual int GetVCDepth() pure;
	virtual int GetDebugFlags() pure;
	M_FORCEINLINE class CXR_Shader* GetShader() { return m_pShader; };

	virtual CXR_VBManager* GetVBM() pure;
	virtual CXR_ViewContext* GetVC() pure;
	virtual CXR_WorldLightState* GetWLS() pure;
	virtual TPtr<CXR_FogState> GetFogState() pure;
	virtual CXR_Portal* GetPortal(int _iPortal) pure;
	virtual class CXR_SurfaceContext* GetSC() pure;
	virtual class CTextureContext* GetTC() pure;

	M_FORCEINLINE void SetDefaultAttrib(CRC_Attributes* _pAttr)
	{
		if (m_bCurrentVCMirrored)
			_pAttr->Attrib_Enable(CRC_FLAGS_CULLCW);
		else
			_pAttr->Attrib_Disable(CRC_FLAGS_CULLCW);
	}

	virtual CObj* GetInterface(int _Var) pure;
	virtual int GetVar(int _Var) pure;
	virtual fp32 GetVarf(int _Var) pure;
	virtual void SetVar(int _Var, aint _Value) pure;
	virtual void SetVarf(int _Var, fp32 _Value) pure;

	virtual CMTime GetEngineTime() pure;
	virtual void SetEngineTime(CMTime &_Time) pure;

	virtual void Register(CScriptRegisterContext &_RegContext) pure;

	friend class CXR_FogState;
};

typedef TPtr<CXR_Engine> spCXR_Engine;

#endif // _INC_XREngine
