
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

	// Optional parameter for Render_AddModel()
	XR_MODEL_STANDARD			= 0,
	XR_MODEL_WORLD				= 1,
	XR_MODEL_SKY				= 2,

	XR_ENGINECREATE_NOCONSOLE	= 1,
};


class CXR_Engine;

//#define XR_DEFAULTPOLYOFFSETSCALE -0.5
//#define XR_DEFAULTPOLYOFFSETSCALE 0.0f
//#define XR_DEFAULTPOLYOFFSET -2.0f

// -------------------------------------------------------------------
//  CXR_Portal
// -------------------------------------------------------------------
class CXR_PortalSurfaceRender
{
public:
	// Called immediately after a portal's view-context has been rendered.
	virtual void RenderPortalSurface(CXR_Engine* _pEngine, void* _pSurfContext, const CVec3Dfp4* _pVertices, int _nVertices, const CMat43fp4& _WMat, const CMat43fp4& _VMat) pure;
};

// -------------------------------------------------------------------
class CXR_Portal : public CReferenceCount
{
public:
	CPlane3Dfp4 m_Plane;
	CPlane3Dfp4 m_PlaneDest;
	CRC_ClipVolume m_Portal;
	CRC_ClipVolume m_PortalDest;
	CVec3Dfp4 m_VisPos;
	CMat43fp4 m_WCam;							// Destination camera
	CMat43fp4 m_dCam;							// Transform between src and dst cameras.
	CMat43fp4 m_ModelWMat;
	CMat43fp4 m_ModelVMat;
	fp4 m_SortZ;
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
	CMat43fp4 m_Pos;
	CXR_AnimState m_Anim;
#ifndef M_RTM
	CMTime m_Time;
#endif
	CXR_Model* m_pModel;
	int m_RenderPass;
	int m_OnRenderFlags;

	CXR_VCModelInstance()
	{
	}

	CXR_VCModelInstance(CXR_Model* _pModel, const CMat43fp4& _Pos, const CXR_AnimState& _Anim, int _RenderPass = 0, int _OnRenderFlags = 0)
	{
		m_pModel = _pModel->OnResolveVariationProxy(_Anim, m_Anim);
		m_Pos = _Pos;
		m_RenderPass = _RenderPass;
		m_OnRenderFlags = _OnRenderFlags;
	}

	void Create(CXR_Model* _pModel, const CMat43fp4& _Pos, const CXR_AnimState& _Anim, int _RenderPass = 0, int _OnRenderFlags = 0)
	{
		m_pModel = _pModel->OnResolveVariationProxy(_Anim, m_Anim);
		m_Pos = _Pos;
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

	TList_Vector<CXR_Portal> m_lPortals;
	int m_nPortals;

	TList_Vector<CXR_VCModelInstance> m_lObjects;
	CXR_VCModelInstance* m_pObjects;

	CXR_ViewClipInterface* m_pViewClip;

	CXR_VCModelInstance m_World;
	CMat43fp4 m_CameraWMat;
	CMat43fp4 m_W2VMat;

	CVec3Dfp4 m_VisPos;
	CPlane3Dfp4 m_FrontPlaneW;
	CPlane3Dfp4 m_BackPlaneW;

	int m_MaxObjects;
	int m_nObjects;
	int m_bIsMirrored:1;
	int m_bWorldFound:1;
	int m_bNeedResolve_TempFixMe;

	virtual void Create(CXR_Engine* _pEngine, int _MaxObjects, int _Depth, int _MaxPortals) pure;
	virtual void OnPrecache() pure;

	virtual void ClearReferences();

	virtual class CXR_Engine* GetEngine() { return m_pEngine; };
	virtual CXR_Model* GetSky() { return m_pSky; };
	virtual int GetDepth() const { return m_RecursionDepth; };
	virtual CXR_WorldLightState *GetLightState();

	static void ReflectMatrix(const CPlane3Dfp4& _Plane, const CMat43fp4& _Mat, CMat43fp4& _RefMat);

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
	class CXR_VBManager* m_pVBM;

	int m_EngineMode;
	int m_DebugFlags;
	fp4 m_LODOffset;
	fp4 m_LightScale;
	fp4 m_LightScaleRecp;
	int m_bFastLight;
	int m_ShowVBTypes;
	int m_bTLEnableEngine;
	int m_bTLEnableRenderer;
	int m_bTLEnableEnabled;

	int32 m_lEngineUserColors[16]; // Mapped as UserColor 16-31
	CVec4Dfp4 m_RenderSurfaceLightScale;

	int m_RenderCaps_Flags;
	int m_RenderCaps_nMultiTexture;				// Shader pipeline textures limit
	int m_RenderCaps_nMultiTextureEnv;			// Fixed-function pipeline limit, <= m_RenderCaps_nMultiTextureEnv
	int m_RenderCaps_nMultiTextureCoords;		// Max texture coordinates, <= m_RenderCaps_nMultiTexture
	int m_SurfOptions;
	int m_SurfCaps;

	// Variables shadowed from viewcontext
	fp4 m_LODScale;
	int m_SceneType;
	int m_ShadowDecals;

	CMat4Dfp4 m_Screen_TextureProjection;
	CVec2Dfp4 m_Screen_PixelUV;
	uint16 m_TextureID_Screen;
	uint16 m_TextureID_ResolveScreen;
	uint16 m_TextureID_Depth;
	uint16 m_TextureID_DeferredNormal;
	uint16 m_TextureID_DeferredDiffuse;
	uint16 m_TextureID_DeferredSpecular;

	// Soft-shadows special
	uint16 m_TextureID_DepthStencil;
	uint16 m_TextureID_ShadowMask;

	CXR_Engine();
	virtual void Create(int _MaxRecurse, int _CreateFlags) pure;
	virtual void OnPrecache() pure;
	virtual void SetDebugFont(spCRC_Font _Font) pure;

	virtual void RenderModel(CXR_VCModelInstance* _pObjInfo, CXR_ViewClipInterface* _pViewClip, int _Flags) pure;

	// Add portals, return value is portal-number, -1 if failed. Note that each VC has it's own portal-list
	virtual int Render_AddPortal(const CVec3Dfp4* _pPortal, int _nVertices, const CMat43fp4& _WCam, const CMat43fp4& _WMat, const CMat43fp4& _VMat, CPixel32 _SurfColor = 0x00000000, CXR_PortalSurfaceRender* _pSurfaceRender = NULL, void* _pSurfContext = NULL) pure;
	virtual int Render_AddMirror(const CVec3Dfp4* _pPortal, int _nVertices, const CMat43fp4& _WMat, const CMat43fp4& _VMat, CPixel32 _SurfColor = 0x00000000, CXR_PortalSurfaceRender* _pSurfaceRender = NULL, void* _pSurfContext = NULL) pure;

	// Add texture-portal, return value is portal-number, -1 if failed. Note that each VC has it's own portal-list
	virtual int Render_AddPortal_Texture(const CVec3Dfp4* _pPortal, int _nVertices, const CMat43fp4& _WCam, const CMat43fp4& _WMat, const CMat43fp4& _VMat) pure;
	virtual int Render_AddMirror_Texture(const CVec3Dfp4* _pPortal, int _nVertices, const CMat43fp4& _WMat, const CMat43fp4& _VMat) pure;

	virtual bool Render_AddModel(CXR_Model* _pModel, const CMat43fp4& _Pos, const CXR_AnimState& _Anim, int _ModelType = XR_MODEL_STANDARD, int _OnRenderFlags = 0) pure;

	virtual void Render_Light_AddDynamic(const CXR_Light& _Light, int _Priority = 0) pure;
	virtual void Render_Light_AddDynamic(const CVec3Dfp4& _Pos, const CVec3Dfp4& _Intensity, fp4 _Range, int _Flags, int _Type, int _Priority = 0) pure;
	virtual void Render_Light_AddStatic(const CXR_Light& _Light) pure;
	virtual void Render_Light_AddStatic(int _LightID, const CVec3Dfp4& _Pos, const CVec3Dfp4& _Intensity, fp4 _Range, int _Flags, int _Type) pure;
	virtual void Render_Light_State(int _LightID, const CVec3Dfp4& _Intensity) pure;

	// Occlusion culling, these functions can only be used when running OnClientRender on object that does NOT have CWO_CLIENTFLAGS_VISIBILITY on them.
	virtual bool View_GetClip_Sphere(const CVec3Dfp4& _Origin, fp4 _Radius, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume = NULL, CXR_RenderInfo* _pRenderInfo = NULL) pure;
	virtual bool View_GetClip_Box(const CVec3Dfp4& _BoxMin, const CVec3Dfp4& _BoxMax, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume = NULL, CXR_RenderInfo* _pRenderInfo = NULL) pure;

	// Advanced usage, you'd better know what you're doing before using these:
	virtual void Engine_PushViewContext(const CMat43fp4& _CameraWMat, const CVec3Dfp4& _VisPos, const CRC_ClipVolume& _Clip, const CPlane3Dfp4& _FrontPlaneW, int _SceneType = XR_SCENETYPE_MAIN) pure;
	virtual void Engine_PopViewContext() pure;

	virtual void Engine_BuildViewContext(const CMat43fp4& _CameraWMat, const CVec3Dfp4& _VisPos, const CRC_ClipVolume& _Clip, const CPlane3Dfp4& _FrontPlaneW, int _SceneType = XR_SCENETYPE_MAIN) pure;
	virtual void Engine_RenderViewContext() pure;

	// Standard client entry point
	virtual void Engine_SetRenderContext(CRenderContext* _pRC) pure;
	virtual void Engine_Render(CXR_EngineClient *_pEngineClient, class CXR_VBManager* _pVBM, CRenderContext* _pRC, const CMat43fp4& _CameraWMat, CXR_SceneGraphInstance* _pSceneGraphInstance = NULL, class CXR_TriangleMeshDecalContainer* _pTMDC = NULL) pure;
	virtual void Engine_ClearReferences() pure;

	// State & interface access:
	virtual int GetMode() pure;
	virtual int GetSceneType() pure;
	virtual int GetVCDepth() pure;
	virtual int GetDebugFlags() pure;
	virtual class CXR_Shader* GetShader() pure;

	virtual CXR_VBManager* GetVBM() pure;
	virtual CXR_ViewContext* GetVC() pure;
	virtual CXR_WorldLightState* GetWLS() pure;
	virtual TPtr<CXR_FogState> GetFogState() pure;
	virtual CXR_Portal* GetPortal(int _iPortal) pure;
	virtual class CXR_SurfaceContext* GetSC() pure;
	virtual class CTextureContext* GetTC() pure;

	virtual void SetDefaultAttrib(CRC_Attributes* _pAttr) pure;

	virtual CObj* GetInterface(int _Var) pure;
	virtual int GetVar(int _Var) pure;
	virtual fp4 GetVarf(int _Var) pure;
	virtual void SetVar(int _Var, aint _Value) pure;
	virtual void SetVarf(int _Var, fp4 _Value) pure;

	virtual CMTime GetEngineTime() pure;
	virtual void SetEngineTime(CMTime &_Time) pure;

	virtual void Register(CScriptRegisterContext &_RegContext) pure;

	friend class CXR_FogState;
};

typedef TPtr<CXR_Engine> spCXR_Engine;

#endif // _INC_XREngine
