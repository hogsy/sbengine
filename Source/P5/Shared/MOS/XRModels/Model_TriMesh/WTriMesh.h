#ifndef _INC_WTRIMESH
#define _INC_WTRIMESH

/*
// +-----------------------------------------------------------------+
// | CXR_Model_TriangleMesh (XR-Component)                           |
// +-----------------------------------------------------------------+
// | Creator:          Magnus Högdahl                                |
// | Created:          1997-09-??                                    |
// | Last modified:    2000-05-19                                    |
// |                                                                 |
// | Description:                                                    |
// |                                                                 |
// | A general purpose model-class with support for multiple animated|
// | surfaces, LOD-meshes, interpolated vertex-animation, skeleton   |
// | animation & deformation, non-homogenous fog, diffuse & specular |
// | lighting, stencil volume-shadows, ray-collision to animated     |
// | deformation-solids, surface occlusion-masks, wire-frame         |
// | rendering.                                                      |
// |                                                                 |
// +-----------------------------------------------------------------+
// | Copyright Starbreeze Studios AB 1997, 2000                      |
// +-----------------------------------------------------------------+
*/

#include "MMath.h"
#include "../../XR/XREngine.h"
#include "XMDCommn.h"
#include "../../XR/XRSurf.h"
#include "../../XR/XRAnim.h"
#include "../../XR/XRVBManager.h"
#include "../../XR/XRVBContainer.h"
#include "MDA_Hash2D.h"
#ifndef PLATFORM_CONSOLE
#include "../../XR/Solids/XWSolid.h"
#endif
//#include "WModels.h"

enum
{
//	CTM_RFLAGS_VERTEXANIM			= M_Bit(0),
	CTM_RFLAGS_NOTSTATIC			= M_Bit(1),
	CTM_RFLAGS_NOVERTEXANIMIP		= M_Bit(2),
	CTM_RFLAGS_NOLIGHTING			= M_Bit(3),
	CTM_RFLAGS_WIRE					= M_Bit(4),
	CTM_RFLAGS_NOTRIANGLES			= M_Bit(5),
	CTM_RFLAGS_MINLOD				= M_Bit(6),
	CTM_RFLAGS_SOLIDS				= M_Bit(7),
	CTM_RFLAGS_WALLMARKS			= M_Bit(8),
//	CTM_RFLAGS_TVERTEXANIM			= M_Bit(9),
//	CTM_RFLAGS_SKELETONANIM			= M_Bit(10),
	CTM_RFLAGS_ORDERTRANS			= M_Bit(11),		// Solid mesh transparency
	
	CTM_MAX_LIGHTS = 32,
};

#define CTM_PARAM_OCCLUSIONMASK		0x0100
#define CTM_PARAM_RENDERFLAGS		0x0101
#define CTM_PARAM_SHADOWOCCLUSIONMASK 0x0102

#define TRIMESH_NUMSECONDARYSURF	2
#define TRIMESH_NUMSURF				(2 + TRIMESH_NUMSECONDARYSURF)

// -------------------------------------------------------------------
//  CXR_Model_TriangleMesh
// -------------------------------------------------------------------
class CXR_Model_TriangleMesh;
typedef TPtr<CXR_Model_TriangleMesh> spCXR_Model_TriangleMesh;

class CXR_SurfaceContext;

class CXR_Model_TriangleMesh : 

	public CXR_PhysicsModel,			// <-- got CReferenceCount
	public CTriangleMeshCore,
	public CXR_WallmarkInterface, 
	public CXR_VBContainer

{
	MRTC_DECLARE;

public:
	TArray<spCXR_Model_TriangleMesh> m_lspLOD;
	TThinArray<uint32> m_lClusterFlags;
	spCXR_Anim_Base m_spAnim;

#if !defined(M_RTM) && !defined(PLATFORM_CONSOLE) 
	TThinArray<spCSolid> m_lspDebugSolids;
	void InitDebugSolids();
#endif
	CStr m_UniqueSortName;
protected:
	int m_RenderPass;
	int m_OcclusionMask;
	int m_ShadowOcclusionMask;
	fp32 m_ParallellTresh;

	class CXR_SurfaceContext *m_pSC;
	int m_iSecondarySurf[TRIMESH_NUMSECONDARYSURF];
	uint32 m_bDefaultSurface:1;
	uint32 m_bUsePrimitives:1;

	// Data used globaly within CXR_Model_TriangleMesh during rendering. 
	// This is to avoid passing around tons of parameters to each function.

	void ClearRenderPointers(class CTriMesh_RenderInstanceParamters* _pRenderParams);

	static CRC_Attributes ms_RenderZBuffer;
	static CRC_Attributes ms_RenderZBufferCCW;

	int m_TextureID_AttenuationExp;
	int m_TextureID_DefaultLens;

	// -------------------------------------------------------------------
	//  CPreloader
	// -------------------------------------------------------------------
	class CPreloader : public CReferenceCount
	{
	public:
		spCCFile	m_spFile;
		uint32		m_nVB;
	};
	TPtr<CPreloader>	m_spPreloader;

public:

	void InitPreloader(CXR_Engine * _pEngine, int _bXDF, int _Platform);

	// Overrides
	virtual aint GetParam(int _Param);
	virtual int GetParamfv(int _Param, fp32* _pRetValues);
	virtual void SetParam(int _Param, aint _Value);
	virtual void SetParamfv(int _Param, const fp32* _pValues);

	virtual int GetModelClass() { return CXR_MODEL_CLASS_TRIMESH; };
	virtual int GetRenderPass(const class CXR_AnimState* _pAnimState);

	virtual int GetNumVariations() { return m_lVariations.Len(); }
	virtual int GetVariationIndex(const char* _pName);
#ifndef M_RTMCONSOLE
	virtual CStr GetVariationName(int _iVariation);
#endif

	virtual CXR_Model* GetLOD(const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, CXR_Engine* _pEngine, int *_piLod = NULL);
	virtual CXR_Skeleton* GetSkeleton();
	virtual CXR_Skeleton* GetPhysSkeleton();

	virtual fp32 GetBound_Sphere(const class CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_Box(CBox3Dfp32& _Box, const class CXR_AnimState* _pAnimState = NULL);
	virtual void GetBound_SolidSkeleton_Box(CBox3Dfp32& _Box, fp32 _JointRadius, const class CXR_AnimState* _pAnimState = NULL);

	CBox3Dfp32 GetBoundBox(){return m_BoundBox;};
	fp32 GetBoundRadius(){return m_BoundRadius;};

#ifndef PLATFORM_CONSOLE
	int GetNumTriangles();
	int GetNumVertices();
	CStr GetModelInfo();
#endif

	void Init();
	void SetRenderFlags(int _Flg) { m_RenderFlags = _Flg; };
	int GetRenderFlags() { return m_RenderFlags; };

	CXR_Model_TriangleMesh();
	~CXR_Model_TriangleMesh();

	virtual void Create(const char* _pParam);
//protected:
	virtual void CreateVB();
public:

	// Model-IO
	virtual void Clear();

#ifndef M_RTMCONSOLE
	void ValidateVariations(CStr _Name);
#endif
	bool CanShareSurfaces(CXR_Model_TriangleMesh* _pModel);
	virtual void Read(class CDataFile* _pDFile);
	virtual void Read(CStr _FileName);
#ifndef PLATFORM_CONSOLE
	virtual void Write(class CDataFile *_pDFile);
	virtual void Write(CStr _FileName);
#endif

	// Rendering
public:
	void CalcBoxScissor(const CRC_Viewport* _pVP, const CMat4Dfp32* _pVMat, const CBox3Dfp32& _Box, CScissorRect& _Scissor);
	void VB_RenderUnified(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_Cluster* _pC, int _iCluster, bool &_bDrawShading);
	bool Cluster_SetMatrixPalette(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_Cluster* _pC, class CTM_VertexBuffer* _pTVB, CMat4Dfp32* _pMatrixPalette, int _nMatrixPalette, class CXR_VertexBuffer* _pVB, uint16 _VpuTaskId, class CRC_MatrixPalette* _pMP = NULL);

	
protected:
	void Cluster_DestroyAll();
//	void Cluster_AutoHibernate(fp64 _TimeOut);
	void Cluster_Render(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_Cluster* _pC);
	void Cluster_RenderProjLight(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_Cluster* _pC);
	void Cluster_RenderUnified(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_Cluster* _pC, int _iCluster);
	void Cluster_RenderSingleColor(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_Cluster* _pC, CPixel32 _Color);
	void RenderDecals(class CTriMesh_RenderInstanceParamters* _pRenderParams, int _GUID, CMat4Dfp32* _pMP, int _nMP);

	void Cluster_Transform_V(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_VertexBuffer* _pC, const CMat4Dfp32* _pMat, const CMTime& _Time);
	void Cluster_Transform_V_N(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_VertexBuffer* _pC, const CMat4Dfp32* _pMat, const CMat4Dfp32* _pNormMat, const CMTime& _Time);
	void Cluster_Transform_V_N_TgU_TgV(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_VertexBuffer* _pC, const CMat4Dfp32* _pMat, const CMat4Dfp32* _pNormMat, const CMTime& _Time);
	void Cluster_TransformBones_V(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_VertexBuffer* _pC, const CMat4Dfp32 *_pMatrixPaletteArgs, const CMTime& _Time);
	void Cluster_TransformBones_V_VPU(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_VertexBuffer* _pC, const CMat4Dfp32 *_pMatrixPaletteArgs, mint _nMat, const CMTime& _Time);
	void Cluster_TransformBones_V_N(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_VertexBuffer* _pC, const class CRC_MatrixPalette *_pMatrixPaletteArgs, const CMTime& _Time);
	void Cluster_TransformBones_V_N_TgU_TgV(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_VertexBuffer* _pC, const class CRC_MatrixPalette *_pMatrixPaletteArgs, const CMTime& _Time);
	void Cluster_ProjectVertices(class CTriMesh_RenderInstanceParamters* _pRenderParams, const CVec3Dfp32* _pSrc, CVec3Dfp32* _pDest, int _nV, const CVec3Dfp32& _ProjPoint, fp32 _Length);
	void Cluster_RenderProjection(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_VertexBuffer* _pC, const CVec3Dfp32& _ProjV, class CXR_Light* _pL = NULL);

	bool ShadowProjectHardware_ThreadSafe(class CTriMesh_RenderInstanceParamters* _pRenderParams, const CMat4Dfp32* _pMat,mint _nMat);
	bool ShadowProjectHardware_ThreadSafe_VPU(class CTriMesh_RenderInstanceParamters* _pRenderParams, const CMat4Dfp32* _pMat,mint _nMat);

	void Cluster_Light(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_VertexBuffer* _pC, struct CPixel32* _pDiffuse, int _bOmni);
	void Cluster_Specular(class CTriMesh_RenderInstanceParamters* _pRenderParams, class CTM_VertexBuffer* _pC, struct CPixel32* _pSpec, int _Power);

	void OnRender_Scissors(class CTriMesh_RenderInstanceParamters* _pRenderParams);
public:
	virtual void OnRender(class CXR_Engine* _pEngine, class CRenderContext* _pRender, class CXR_VBManager* _pVBM, class CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
			const class CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags);
	virtual void OnRender2(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _WVelMat, int _Flags);

	void OnRender_Lights(CXR_WorldLightState* _pWLS, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat,
		class CTriMesh_RenderInstanceParamters* _pRenderParams,const CBox3Dfp32 & _BoundBoxW,bool _bAnim,int _Flags = 0);
	virtual void OnPrecache(class CXR_Engine* _pEngine, int _iVariation);
	virtual void OnPostPrecache(class CXR_Engine* _pEngine);
	virtual void OnRefreshSurfaces();
	virtual void OnResourceRefresh(int _Flags = 0);
//	virtual void OnHibernate(int _Flags = 0);
	
//	virtual bool DecompressAll(int _Flags = 0);

	// -------------------------------------------------------------------
	// PhysicsModel:

	// Bounding volumes are in world-space.
	virtual void Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _RetPos, fp32& _Radius);
	virtual void Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _RetBox);

	// Collision services. All indata is in world coordinates.
	virtual void Phys_Init(CXR_PhysicsContext* _pPhysContext);

	virtual int Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0) 
	{ 
		return XW_MEDIUM_AIR; 
	};
	virtual void Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, CXR_MediumDesc& _RetMedium) 
	{
		_RetMedium.SetAir();
	};
	
	virtual bool Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, class CCollisionInfo* _pCollisionInfo = NULL);
	virtual bool Phys_IntersectSphere(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _Origin, const CVec3Dfp32& _Dest, fp32 _Radius, int _MediumFlags, class CCollisionInfo* _pCollisionInfo = NULL) { return false; };
	virtual bool Phys_IntersectBox(CXR_PhysicsContext* _pPhysContext, const CPhysOBB& _BoxOrigin, const CPhysOBB& _BoxDest, int _MediumFlags, class CCollisionInfo* _pCollisionInfo = NULL) 
	{ 
		return false; 
	};

	virtual void CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, class CPotColSet *_pcs, const int _iObj, const int _MediumFlags );

/*	virtual void Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32* _pV, int _nV, uint32* _pRetMediums) 
	{

	};
	virtual void Phys_GetMedium(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32* _pV, int _nV, class CXR_MediumDesc* _pRetMediums) 
	{

	};*/

	virtual CXR_PhysicsModel* Phys_GetInterface() { return (m_lSolids.Len() || HasBoxTree()) ? this : NULL; };

protected:
	virtual bool Phys_IntersectLine_Bound(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, class CCollisionInfo* _pCollisionInfo);
	virtual bool Phys_IntersectLine_Solids(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, class CCollisionInfo* _pCollisionInfo);
	virtual bool Phys_IntersectLine_BoxTree(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, class CCollisionInfo* _pCollisionInfo);
#ifndef PLATFORM_CONSOLE	
	virtual bool Phys_IntersectLine_Triangles(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, class CCollisionInfo* _pCollisionInfo);
	static bool Phys_TriangleIntersectRay(CVec3Dfp32& _orig, CVec3Dfp32& _dir, CVec3Dfp32& _vert0, CVec3Dfp32& _edge1, CVec3Dfp32& _edge2, CVec2Dfp32& uv, class CCollisionInfo* _pCollisionInfo);
#endif

public:
	// -------------------------------------------------------------------
	// Wallmark interface:
	virtual int Wallmark_CreateContext(const CXR_WallmarkContextCreateInfo& _CreateInfo);
	virtual void Wallmark_DestroyContext(int _hContext);
	virtual int Wallmark_Create(int _hContext, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material = 0);
	virtual void Wallmark_CreateWithContainer(void* _pContainer, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material = 0);

	virtual CXR_WallmarkInterface* Wallmark_GetInterface() { return this; };

	// -------------------------------------------------------------------
	// CXR_VBContainer overrides
	virtual int GetNumLocal();
	virtual int GetID(int _iLocal);
	virtual CFStr GetName(int _iLocal);
	virtual void Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags);
	virtual uint32 GetFlags(int _iLocal);
	const char *GetContainerSortName() 
	{
		return m_UniqueSortName.Str();
	}

	virtual void Release(int _iLocal);

	void SetUsePrimitives(bool flag) { m_bUsePrimitives = flag; }
	static bool CullCluster(CTM_Cluster* _pC, CTriMesh_RenderInstanceParamters* _pRP, spCXW_Surface* _lspSurfaces, uint _iSurf, uint _bDrawSolid);

protected:
	uint32	GetVertexBufferFlags(CTM_VertexBuffer* _pC, CCFile* _pFile);

};


#endif //_INC_WTRIMESH
