#include "PCH.h"

#include "WTriMesh.h"
#include "WTriMeshDC.h"
#include "MFloat.h"
#include "../../XR/XR.h"
#include "../../XR/XREngineVar.h"
#include "../../XR/XRShader.h"
#include "../../XR/XRVBContext.h"
#include "../../XR/XRVBUtil.h"
#include "../../../mcc/MRTC_VPUManager.h"
#include "MMath_Vec128.h"
#ifdef	PLATFORM_PS2
#include "../../RndrPS2/MRndrPS2.h"
#endif

//#pragma optimize("",off)

#if !defined(M_RTM) && !defined(PLATFORM_CONSOLE) 
#define	PHYS_RENDER
#endif

#ifndef	PLATFORM_PS2
#define	ENABLE_SHADOWVOLUME 1
#else
#define	ENABLE_SHADOWVOLUME 0
#endif

#include "WTriMeshRIP.h"

enum
{
	e_Platform_PC    = 0,
	e_Platform_Xbox  = 1,
	e_Platform_PS2   = 2,
	e_Platform_GC    = 3,
	e_Platform_Xenon = 4,
	e_Platform_PS3	 = 5,

	e_Platform_Default = e_Platform_PC,
};



#define SOME_PROFILING

//#pragma optimize("",off)
//#pragma inline_depth(0)

// #define TEMP_NO_SKELETON_ANIM

// This can probably be static
CRC_Attributes CXR_Model_TriangleMesh::ms_RenderZBuffer;
CRC_Attributes CXR_Model_TriangleMesh::ms_RenderZBufferCCW;

#define CTM_HWT
#define CTM_HWLIGHT

#if defined(PLATFORM_PS2) || defined(PLATFORM_XBOX)
#define CTM_USEPRIMITIVES
#endif

#define MACRO_ISMIRRORED(Mat)	\
	((CVec3Dfp32::GetMatrixRow(Mat, 0) / CVec3Dfp32::GetMatrixRow(Mat, 1)) * CVec3Dfp32::GetMatrixRow(Mat, 2) < 0.0f)

// XRLight.cpp
int FindMostContributingLights( const int _MaxLights, uint8* _pLightIndices, const CMat4Dfp32& _WMat, const CXR_RenderInfo* _pRenderInfo );

static void TransformAABB(const CMat4Dfp32& _Mat, const CBox3Dfp32& _Box, CBox3Dfp32& _DestBox)
{
	MAUTOSTRIP(TransformAABB, MAUTOSTRIP_VOID);
	// Transform axis-aligned bounding box.

	CVec3Dfp32 E;
	CVec3Dfp32 C;
	_Box.m_Max.Lerp(_Box.m_Min, 0.5f, C);
	_Box.m_Max.Sub(_Box.m_Min, E);
	E *= 0.5f;

	C *= _Mat;

	_DestBox.m_Max = 0;
	for(int axis = 0; axis < 3; axis++)
		for(int k = 0; k < 3; k++)
			_DestBox.m_Max.k[k] += M_Fabs(_Mat.k[axis][k]*E[axis]);

	_DestBox.m_Min = -_DestBox.m_Max;
	_DestBox.m_Min += C;
	_DestBox.m_Max += C;
}


// -------------------------------------------------------------------
//  CXR_Model_TriangleMesh
// -------------------------------------------------------------------
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_TriangleMesh, CXR_Model);

aint CXR_Model_TriangleMesh::GetParam(int _Param)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_GetParam, 0);
	switch(_Param)
	{
	case CXR_MODEL_PARAM_ANIM :
		return (aint)(CXR_Anim_Base*)m_spAnim;

	case CXR_MODEL_PARAM_SKELETON :
		return (aint)(CXR_Skeleton*)m_spSkeleton;

	case CXR_MODEL_PARAM_NUMLODS:
		return m_lspLOD.Len() + 1;

#ifndef PLATFORM_CONSOLE
	case CXR_MODEL_PARAM_NUMTRIANGLES:
		return GetNumTriangles();

	case CXR_MODEL_PARAM_NUMVERTICES:
		return GetNumVertices();

	case CXR_MODEL_PARAM_KEYS:
		return aint(&m_Keys);
#endif

	case CTM_PARAM_OCCLUSIONMASK :
		return m_OcclusionMask;
	case CTM_PARAM_SHADOWOCCLUSIONMASK :
		return m_ShadowOcclusionMask;		

	case CTM_PARAM_RENDERFLAGS :
		return m_RenderFlags;

	case CXR_MODEL_PARAM_TIMEMODE:
		return CXR_MODEL_TIMEMODE_CONTROLLED;
		
//	case CXR_MODEL_PARAM_ISALIVE:
//		return 1;

	case CXR_MODEL_PARAM_ISSHADOWCASTER :
		{
			for(int i = 0; i < GetNumClusters(); i++)
			{
				CTM_Cluster* pC = GetCluster(i);
				if (pC->m_Flags & (CTM_CLUSTERFLAGS_SHADOWVOLUME | CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE))
					return 1;
			}

			return 0;
		}
	case CXR_MODEL_PARAM_THINSOLIDS:
		return (aint) m_lSolids.GetBasePtr();

	case CXR_MODEL_PARAM_N_THINSOLIDS:
		return m_lSolids.Len();

	case CXR_MODEL_PARAM_CAPSULES:
		return (aint) m_lCapsules.GetBasePtr();

	case CXR_MODEL_PARAM_N_CAPSULES:
		return m_lCapsules.Len();

	case CXR_MODEL_PARAM_GLOBALSCALE:
		return RoundToInt(m_GlobalScale * 1000.0f);

	default:
		return 0;
	}
}

int CXR_Model_TriangleMesh::GetParamfv(int _Param, fp32* _pRetValues)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_GetParamfv, 0);
#ifndef PLATFORM_CONSOLE
	switch(_Param)
	{
		case CXR_MODEL_PARAM_NUMTRIANGLES:
		{
			_pRetValues[0] = GetNumTriangles();
			for(int i = 0; i < m_lspLOD.Len(); i++)
				_pRetValues[i + 1] = m_lspLOD[i]->GetNumTriangles();
			return m_lspLOD.Len() + 1;
		}

		case CXR_MODEL_PARAM_NUMVERTICES:
		{
			_pRetValues[0] = GetNumVertices();
			for(int i = 0; i < m_lspLOD.Len(); i++)
				_pRetValues[i + 1] = m_lspLOD[i]->GetNumVertices();
			return m_lspLOD.Len() + 1;
		}

		default:
			return 0;
	}
#endif
	return 0;
}

void CXR_Model_TriangleMesh::SetParam(int _Param, aint _Value)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_SetParam, MAUTOSTRIP_VOID);
	switch(_Param)
	{
	case CXR_MODEL_PARAM_ANIM :
		m_spAnim = (CXR_Anim_Base*)(aint)_Value;
		break;

	case CXR_MODEL_PARAM_SKELETON :
		m_spSkeleton = (CXR_Skeleton*)(aint)_Value;
		break;

	case CTM_PARAM_OCCLUSIONMASK :
		m_OcclusionMask = _Value;
		break;

	case CTM_PARAM_SHADOWOCCLUSIONMASK :
		m_ShadowOcclusionMask = _Value;
		break;

	case CTM_PARAM_RENDERFLAGS :
		{
			m_RenderFlags = _Value;
			for(int i = 0; i < m_lspLOD.Len(); i++)
				m_lspLOD[i]->m_RenderFlags = _Value;
		}
		break;
		
	case CXR_MODEL_PARAM_SETPICMIPGROUP:
		{
			for(int l = 0; l < m_lspLOD.Len(); l++)
				m_lspLOD[l]->SetParam(CXR_MODEL_PARAM_SETPICMIPGROUP, _Value);

			for(int s = 0; s < m_lspSurfaces.Len(); s++)
				m_lspSurfaces[s]->SetTextureParam(CTC_TEXTUREPARAM_PICMIPINDEX, _Value);
		}
		break;
		
	default:
		CXR_Model::SetParam(_Param, _Value);
		break;
	}
}

void CXR_Model_TriangleMesh::SetParamfv(int _Param, const fp32* _pValues)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_SetParamfv, MAUTOSTRIP_VOID);
	switch(_Param)
	{
	case CXR_MODEL_PARAM_TEXTUREPARAM :
		{
			for(int i = 0; i < m_lspSurfaces.Len(); i++)
				m_lspSurfaces[i]->SetTextureParam((int)_pValues[0], (int)_pValues[1]);

			for(int iLOD = 0; iLOD < m_lspLOD.Len(); iLOD++)
				m_lspLOD[iLOD]->SetParamfv(_Param, _pValues);
		}
		break;

	default :
		CXR_PhysicsModel::SetParamfv(_Param, _pValues);
	}
}

#ifndef PLATFORM_CONSOLE
int CXR_Model_TriangleMesh::GetNumTriangles()
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_GetNumTriangles, 0);
	int nTri = 0;
	int nVB = GetNumVertexBuffers();
	for(int i = 0; i < nVB; i++)
		nTri += GetVertexBuffer(i)->GetNumTriangles(this);
	return nTri;
}

int CXR_Model_TriangleMesh::GetNumVertices()
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_GetNumVertices, 0);
	int nV = 0;
	int nVB = GetNumVertexBuffers();
	for(int i = 0; i < nVB; i++)
		nV += GetVertexBuffer(i)->GetNumVertices(this);
	return nV;
}

CStr CXR_Model_TriangleMesh::GetModelInfo()
{
	int nLod = GetParam(MODEL_PARAM_NUMLODS);

	CBox3Dfp32 Bound;
	GetBound_Box(Bound);

	int nVB[2] = {0, 0};
	int nClusters[2] = {0, 0};

	int nTri[6] = { 0, 0, 0, 0, 0, 0 };
	int iBase = 0;
	for(int m = 0; m < 2; m++)
	{
		CXR_Model_TriangleMesh *pMesh;
		if(m == 0)
			pMesh = this;
		else
		{
			if(!m_lspLOD.Len())
				break;
			pMesh = m_lspLOD[m_lspLOD.Len() - 1];
			iBase += 3;
		}
	
		
		nVB[m] = pMesh->GetNumVertexBuffers();
		int nC = pMesh->GetNumClusters();
		nClusters[m] = nC;
		for(int i = 0; i < nC; i++)
		{
			CTM_Cluster* pC = pMesh->GetCluster(i);
			if(pC)
			{
				CTM_VertexBuffer* pVB = pMesh->GetVertexBuffer(pC->m_iVB);
				int nTriangles = pC->m_nIBPrim?(pC->m_nIBPrim / 3):pVB->GetNumTriangles(this);
				if(pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE)
					nTri[2 + iBase] += nTriangles;
				else if(pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUME)
					nTri[1 + iBase] += nTriangles;
				else
					nTri[0 + iBase] += nTriangles;
			}
		}
	}

	CStr St;
	St += CStrF("Size: %.0fx%.0fx%.0f", Bound.m_Max[0] - Bound.m_Min[0], Bound.m_Max[1] - Bound.m_Min[1], Bound.m_Max[2] - Bound.m_Min[2]);
	St += CStrF(", VertexBuffers: %i (%i)", nVB[0], nVB[1]);
	St += CStrF(", Clusters: %i (%i)", nClusters[0], nClusters[1]);
	St += CStrF(", Surfaces: %i", m_lspSurfaces.Len());
	if(m_lspLOD.Len() > 0)
	{
		St += CStrF(", Lods: %i", m_lspLOD.Len());
		St += CStrF(", Triangles: %i (%i)", nTri[0], nTri[3]);
		if(nTri[1] > 0)
			St += CStrF(", HW ShadowTriangles: %i (%i)", nTri[1], nTri[4]);
		if(nTri[2] > 0)
			St += CStrF(", SW ShadowTriangles: %i (%i)", nTri[2], nTri[5]);
	}
	else
	{
		St += CStrF(", Triangles: %i ", nTri[0]);
		if(nTri[1] > 0)
			St += CStrF(", HW ShadowTriangles: %i", nTri[1]);
		if(nTri[2] > 0)
			St += CStrF(", SW ShadowTriangles: %i", nTri[2]);
	}
	return St;
}

#endif

CXR_Model_TriangleMesh::CXR_Model_TriangleMesh()
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_ctor, MAUTOSTRIP_VOID);
	SetThreadSafe(true);
//	m_MeshFlags = 0;
	m_BoundRadius = 0;
	m_RenderFlags = 0;
	m_RenderPass = 0;
	m_OcclusionMask = 0;
	m_ShadowOcclusionMask = 0;
	m_ParallellTresh = 0.5f;
	m_BoundBox.m_Min = 0;
	m_BoundBox.m_Max = 0;
	m_pSC = NULL;
	
	m_bDefaultSurface = true;

	for(int a = 0; a < TRIMESH_NUMSECONDARYSURF; a++)
		m_iSecondarySurf[a] = -1;

//	ClearRenderPointers();
	static bool bInitStaticAttribs = true;

	if (bInitStaticAttribs)
	{
		bInitStaticAttribs = false;

		ms_RenderZBuffer.SetDefault();
		ms_RenderZBuffer.Attrib_Enable(CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);
	#ifdef PLATFORM_CONSOLE
		ms_RenderZBuffer.Attrib_Disable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE);
	#endif
		ms_RenderZBuffer.Attrib_StencilRef(128, 255);
		ms_RenderZBuffer.Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_REPLACE);
		ms_RenderZBuffer.Attrib_StencilWriteMask(255);
	#ifdef XR_DEFAULTPOLYOFFSETSCALE
		ms_RenderZBuffer.Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
	#endif

		ms_RenderZBufferCCW = ms_RenderZBuffer;
		ms_RenderZBufferCCW.Attrib_Enable(CRC_FLAGS_CULLCW);
	}
}

void CXR_Model_TriangleMesh::Create(const char* _pParam)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Create, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_TriangleMesh::Create, MODEL_TRIMESH);

	MACRO_GetRegisterObject(CXR_SurfaceContext , pSC, "SYSTEM.SURFACECONTEXT");
	m_pSC = pSC;
	m_GlobalScale = 1.0f;

	for(int l = 0; l < m_lspLOD.Len(); l++)
	{
		m_lspLOD[l]->m_pSC = pSC;
		m_lspLOD[l]->m_UniqueSortName = CStrF("%s:%d", m_UniqueSortName.Str(), l);
	}

	if(_pParam == NULL || !pSC)
		return;

	CStr st = _pParam;
	while(st != "")
	{
		CFStr key = st.GetStrSep(",");
		if(key.Copy(0, 5).CompareNoCase("surf=") == 0)
		{
			CStr aurast = key.Copy(5, 100000);
			int iAura = 0;
			while(aurast != "")
			{
				if(iAura >= TRIMESH_NUMSECONDARYSURF)
				{
					LogFile("CXR_Model_TriangleMesh::Create, Too many secondary surfaces");
					break;
				}

				CStr aura = aurast.GetStrSep(";");
				int iIndex = pSC->GetSurfaceID(aura);
				if(iIndex > 0)
				{
					m_iSecondarySurf[iAura] = iIndex;
					for(int i = 0; i < m_lspLOD.Len(); i++)
						m_lspLOD[i]->m_iSecondarySurf[iAura] = iIndex;
					iAura++;
				}
				else
					LogFile(CStrF("CXR_Model_TriangleMesh::Create, Missing surface %s", (char *)aura));
			}
		}
		else if(key.CompareNoCase("nosurf") == 0)
		{
			m_bDefaultSurface = false;
			for(int i = 0; i < m_lspLOD.Len(); i++)
				m_lspLOD[i]->m_bDefaultSurface = false;
		}
		else if (key.Copy(0, 6).CompareNoCase("scale=") == 0)
		{
			m_GlobalScale = key.Copy(6, 20).Val_fp64();
		}
	}
}

void CXR_Model_TriangleMesh::CreateVB()
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_CreateVB, MAUTOSTRIP_VOID);
	int nVB = GetNumVertexBuffers();
	for(int i = 0; i < nVB; i++)
	{
		CTM_VertexBuffer* pTVB = GetVertexBuffer(i);
		if (pTVB->m_VBID == 0)
		{
			pTVB->m_VBID = m_pVBCtx->AllocID(m_iVBC, i);
		}
	}
}

void CXR_Model_TriangleMesh::ClearRenderPointers(CTriMesh_RenderInstanceParamters* _pRenderParams)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_ClearRenderPointers, MAUTOSTRIP_VOID);
	_pRenderParams->ClearPointers();
}

int CXR_Model_TriangleMesh::GetRenderPass(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_GetRenderPass, 0);
	return m_RenderPass;
}

int CXR_Model_TriangleMesh::GetVariationIndex(const char* _pName)
{
	const CTM_Variation* pV = m_lVariations.GetBasePtr();
	int nV = m_lVariations.Len();
	for(int i = 0; i < nV; i++)
		if (pV[i].m_Name.CompareNoCase(_pName) == 0)
			return i;
	return 0;
}

#ifndef M_RTMCONSOLE

CStr CXR_Model_TriangleMesh::GetVariationName(int _iVariation)
{
	if (m_lVariations.ValidPos(_iVariation))
		return m_lVariations[_iVariation].m_Name;
	return CStr();
}

#endif


CXR_Model* CXR_Model_TriangleMesh::GetLOD(const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, CXR_Engine* _pEngine, int *_piLod)
{
	CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(_WMat, 3);
	Pos *= _VMat;
	fp32 Dist = Pos.Length();

	if (_pEngine)
	{
		Dist += _pEngine->m_LODOffset;
		Dist *= _pEngine->m_LODScale;
	}

	int nLOD = m_lspLOD.Len();

	for(int i = nLOD-1; i >= 0; i--)
	{
		if (Dist >= m_lLODBias[i])
		{
			m_lspLOD[i]->m_RenderFlags = m_RenderFlags;
			if(_piLod)
				*_piLod = 1 + i;
			return m_lspLOD[i];
		}
	}

	if(_piLod)
		*_piLod = 0;
	return this;
}

CXR_Skeleton* CXR_Model_TriangleMesh::GetSkeleton()
{
	return m_spSkeleton;
}

CXR_Skeleton* CXR_Model_TriangleMesh::GetPhysSkeleton()
{
	int nLOD = m_lspLOD.Len();
	if (nLOD)
		return m_lspLOD[nLOD-1]->m_spSkeleton;
	else
		return m_spSkeleton;
}

fp32 CXR_Model_TriangleMesh::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_GetBound_Sphere, 0.0f);

	return m_BoundRadius;
}

void CXR_Model_TriangleMesh::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_GetBound_Box, MAUTOSTRIP_VOID);
	_Box = m_BoundBox;

	if (_pAnimState && _pAnimState->m_pSkeletonInst && _pAnimState->m_pSkeletonInst->m_nBoneTransform >= 2)
	{
		const CMat4Dfp32* pMatrixPalette = _pAnimState->m_pSkeletonInst->m_pBoneTransform;
		CMat4Dfp32 M0Inv;
		CMat4Dfp32 M0M1;
		pMatrixPalette[0].InverseOrthogonal(M0Inv);
		pMatrixPalette[1].Multiply(M0Inv, M0M1);
		CBox3Dfp32 BoxTemp;
		_Box.Transform(M0M1, BoxTemp);
		_Box = BoxTemp;
	}
}

void CXR_Model_TriangleMesh::GetBound_SolidSkeleton_Box(CBox3Dfp32& _Box, fp32 _JointRadius, const CXR_AnimState* _pAnimState)
{
	_Box = m_BoundBox;
	if (m_spSkeleton && _pAnimState && _pAnimState->m_pSkeletonInst)
	{
		TAP_RCD<CXR_ThinSolid> lSolids = m_lSolids;
		TAP_RCD<CXR_SkeletonNode> lNodes = m_spSkeleton->m_lNodes;
		TAP_RCD<CMat4Dfp32> lBoneTransform(_pAnimState->m_pSkeletonInst->m_pBoneTransform, _pAnimState->m_pSkeletonInst->m_nBoneTransform);
		
		uint16 iNode = 0;
		CVec3Dfp32 Pos = 0.0f;
		CVec3Dfp32 JointRadius = _JointRadius;
		_Box = CBox3Dfp32(_FP32_MAX, -_FP32_MAX);
		if (lSolids.Len())
		{
			for (uint i = 0; i < lSolids.Len(); i++)
			{
				iNode = lSolids[i].m_iNode;
				Pos = (lNodes[iNode].m_LocalCenter * lBoneTransform[iNode]);
				_Box.Expand(CBox3Dfp32(Pos - JointRadius, Pos + JointRadius));
			}
		}
	}
}

void CXR_Model_TriangleMesh::Init()
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Init, MAUTOSTRIP_VOID);
	// Get treshold for parallell light.
#ifndef	PLATFORM_CONSOLE
	{
		int iKey = m_Keys.GetKeyIndex("PARALIGHTTRESH");
		if (iKey >= 0)
			m_ParallellTresh = m_Keys.GetKeyValue(iKey).Val_fp64();
	}

	// Rendering flags
	{
		int iKey = m_Keys.GetKeyIndex("RENDERFLAGS");
		if (iKey >= 0)
		{
			m_RenderFlags |= m_Keys.GetKeyValue(iKey).Val_int();
//			LogFile(CStrF("(CXR_Model_TriangleMesh::Init) Renderflags %d", m_RenderFlags));
		}
	}
	{
		int iKey = m_Keys.GetKeyIndex("SOLID_TRANSPARENT");
		if (iKey >= 0)
		{
			if (m_Keys.GetKeyValue(iKey).Val_int())
				m_RenderFlags |= CTM_RFLAGS_ORDERTRANS;
			else
				m_RenderFlags &= ~CTM_RFLAGS_ORDERTRANS;
		}
	}

	// Rendering flags
	{
		int iKey = m_Keys.GetKeyIndex("RENDERPASS");
		if (iKey >= 0)
		{
			m_RenderPass |= m_Keys.GetKeyValue(iKey).Val_int();
//			LogFile(CStrF("(CXR_Model_TriangleMesh::Init) RenderPass %d", m_RenderPass));
		}
	}

	// Rendering flags
	{
		int iKey = m_Keys.GetKeyIndex("OCCLUSIONMASK");
		if (iKey >= 0)
		{
			m_OcclusionMask |= m_Keys.GetKeyValue(iKey).Val_int();
//			LogFile(CStrF("(CXR_Model_TriangleMesh::Init) RenderPass %d", m_RenderPass));
		}
	}
	{
		int iKey = m_Keys.GetKeyIndex("SHADOWOCCLUSIONMASK");
		if (iKey >= 0)
		{
			m_ShadowOcclusionMask |= m_Keys.GetKeyValue(iKey).Val_int();
//			LogFile(CStrF("(CXR_Model_TriangleMesh::Init) RenderPass %d", m_RenderPass));
		}
	}

#endif	// PLATFORM_CONSOLE

//	DetermineStatic();

	MACRO_GetSystemEnvironment(pReg);
	if (pReg && pReg->GetValuei("MINLOD", 0, 0))
		m_RenderFlags |= CTM_RFLAGS_MINLOD;

//	if (m_RenderFlags & CTM_RFLAGS_STATIC) InitStaticVertexList();
}

CXR_Model_TriangleMesh::~CXR_Model_TriangleMesh()
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_dtor, MAUTOSTRIP_VOID);
//	M_TRY
//	{
		if (m_pVBCtx)
		{
			Cluster_DestroyAll();
		}
/*	}
	M_CATCH(
	catch(CCException)
	{
	}
	)*/
}

void CXR_Model_TriangleMesh::Clear()
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Clear, MAUTOSTRIP_VOID);
	CTriangleMeshCore::Clear();

//	m_MeshFlags &= ~(CTM_MESHFLAGS_EDGES | CTM_MESHFLAGS_MATRIXIW | CTM_MESHFLAGS_TANGENTS);
	m_lspLOD.Clear();
	m_spAnim = NULL;

#if !defined(M_RTM) && !defined(PLATFORM_CONSOLE) 
	m_lspDebugSolids.Clear();
#endif
}

#ifndef M_RTMCONSOLE
void CXR_Model_TriangleMesh::ValidateVariations(CStr _Name)
{
	// Validate variations and clusterrefs
	{
		int nCR = m_lClusterRefs.Len();
		int nV = m_lVariations.Len();
		const CTM_Variation* pV = m_lVariations.GetBasePtr();
		for(int v = 0; v < nV; v++)
		{
			if (pV[v].m_iClusterRef + pV[v].m_nClusters > nCR)
				Error_static("CTriangleMeshCore::Read", CStrF("Invalid variation cluster list. %d, %d/%d, %s", pV[v].m_iClusterRef, pV[v].m_nClusters, nCR, _Name.Str() ));
		}

		int nC = GetNumClusters();
		int nSurf = m_lspSurfaces.Len();
		const CTM_ClusterRef* pCR = m_lClusterRefs.GetBasePtr();
		for(int i = 0; i < nCR; i++)
		{
			if (pCR[i].m_iCluster >= nC)
				Error_static("CTriangleMeshCore::Read", CStrF("Invalid cluster reference. (iCluster %d >= %d, %s)", pCR[i].m_iCluster, nC, _Name.Str() ));
			if (pCR[i].m_iSurface >= nSurf)
				Error_static("CTriangleMeshCore::Read", CStrF("Invalid cluster reference. (iSurface %d >= %d, %s)", pCR[i].m_iSurface, nSurf, _Name.Str() ));
		}
	}

	for(int i = 0; i < m_lspLOD.Len(); i++)
		m_lspLOD[i]->ValidateVariations(_Name);
}
#endif

void CXR_Model_TriangleMesh::Read(CDataFile* _pDFile)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Read, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_TriangleMesh::Read, MODEL_TRIMESH);

//	CCFile* pF = _pDFile->GetFile();

	if (!_pDFile->GetNext("EXTENDEDMODEL")) Error("Read", "Can't find EXTENDEDMODEL");
	if (!_pDFile->HasSubDir()) Error("Read", "Corrupt model. (1)");
	_pDFile->GetSubDir();

	CTriangleMeshCore::Read(_pDFile);

	_pDFile->GetParent();

	// Check for animation and read it if present.
	{
		_pDFile->PushPosition();
		bool bReadAnim = _pDFile->GetNext("ANIMATIONSET");
		_pDFile->PopPosition();
		if (bReadAnim)
		{
			m_spAnim = MNew(CXR_Anim_Base);
			if (!m_spAnim) MemError("ReadXMD");
			m_spAnim->Read(_pDFile);
		}
	}

	for(int iSurf = 0; iSurf < m_lspSurfaces.Len(); iSurf++)
	{
		CXW_Surface* pS = m_lspSurfaces[iSurf];
		pS->InitTextures(false);	// Don't report failures.
	}

	// -------------------------------------------------------------------
	Init();

	m_UniqueSortName = m_FileName;
	MACRO_GetRegisterObject(CXR_SurfaceContext , pSC, "SYSTEM.SURFACECONTEXT");
	m_pSC = pSC;
	for(int l = 0; l < m_lspLOD.Len(); l++)
	{
		m_lspLOD[l]->m_pSC = pSC;
		m_lspLOD[l]->m_UniqueSortName = CStrF("%s:%d", m_UniqueSortName.Str(), l);
	}
}

bool CXR_Model_TriangleMesh::CanShareSurfaces(CXR_Model_TriangleMesh* _pM)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_CanShareSurfaces, false);
	if (m_lspSurfaces.Len() != _pM->m_lspSurfaces.Len())
		return false;

	for(int i = 0; i < m_lspSurfaces.Len(); i++)
	{
		if (m_lspSurfaces[i]->m_Name.CompareNoCase(_pM->m_lspSurfaces[i]->m_Name) != 0)
			return false;
	}

	return true;
}

void CXR_Model_TriangleMesh::Read(CStr _FileName)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Read_2, MAUTOSTRIP_VOID);
//	CFStr Scope = CFStrF("CXR_Model_TriangleMesh::Read(%s)", _FileName.Str());
//	MSCOPE_STR(Scope.Str(), MODEL_TRIMESH);
	MSCOPE(CXR_Model_TriangleMesh::Read, MODEL_TRIMESH);

	CDataFile DFile;
	DFile.Open(_FileName);
	Read(&DFile);

	do
	{
		DFile.PushPosition();
		if (!DFile.GetNext("EXTENDEDMODEL")) break;
		DFile.PopPosition();

		spCXR_Model_TriangleMesh spMesh = MNew(CXR_Model_TriangleMesh);
		M_TRY
		{ 
			spMesh->m_GlobalScale = m_GlobalScale;
			spMesh->Read(&DFile); 
		}
		M_CATCH(
		catch(CCException)
		{ 
			Error("Read", CStrF("Failed reading LOD %d", m_lspLOD.Len() )); 
		}
		)

		spMesh->m_lSolids.Clear();

		if (spMesh->m_spAnim != NULL) spMesh->m_spAnim = m_spAnim;

		// Share surfaces
		if (!spMesh->m_lspSurfaces.Len())
			spMesh->m_lspSurfaces = m_lspSurfaces;

		if(spMesh->m_spSkeleton && m_spSkeleton)
			spMesh->m_spSkeleton->m_lAttachPoints = m_spSkeleton->m_lAttachPoints;

		spMesh->m_iLOD = m_lspLOD.Add(spMesh);
	}
	while(1);

	DFile.Close();

	ValidateVariations(_FileName);

	m_UniqueSortName = m_FileName;
	MACRO_GetRegisterObject(CXR_SurfaceContext , pSC, "SYSTEM.SURFACECONTEXT");
	m_pSC = pSC;
	for(int l = 0; l < m_lspLOD.Len(); l++)
	{
		m_lspLOD[l]->m_pSC = pSC;
		m_lspLOD[l]->m_UniqueSortName = CStrF("%s:%d", m_UniqueSortName.Str(), l);
	}
}

#ifndef PLATFORM_CONSOLE
void CXR_Model_TriangleMesh::Write(CStr _FileName)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Write, MAUTOSTRIP_VOID);
//	DecompressAll(-1);

	// Nuke solids on LODs
	{
		for(int i = 0; i < m_lspLOD.Len(); i++)
			m_lspLOD[i]->m_lSolids.Clear();
	}

	// Share surfaces
	TArray<spCXW_Surface> lspSurf;

	if (m_lspLOD.Len())
	{
		{
			for(int i = 0; i < m_lspSurfaces.Len(); i++)
				lspSurf.Add(m_lspSurfaces[i]->Duplicate());
		}

		{
			for(int iLOD = 0; iLOD < m_lspLOD.Len(); iLOD++)
			{
				CXR_Model_TriangleMesh& LOD = *m_lspLOD[iLOD];
//				LOD.DecompressAll(-1);

				int nC = LOD.GetNumClusters();
				for(int iC = 0; iC < nC; iC++)
				{
					CTM_Cluster* pC = LOD.GetCluster(iC);
					int iSurf = CXW_Surface::GetSurfaceIndex(lspSurf, LOD.m_lspSurfaces[pC->m_iSurface]->m_Name);
					pC->m_iSurface = (iSurf < 0) ?
						lspSurf.Add(LOD.m_lspSurfaces[pC->m_iSurface]->Duplicate()) :
						iSurf;
				}
			} 
		}

		{
			m_lspSurfaces = lspSurf;
			for(int iLOD = 0; iLOD < m_lspLOD.Len(); iLOD++)
				m_lspLOD[iLOD]->m_lspSurfaces.Destroy();
		}
	}

	// CLOTH LOD
	if(m_spSkeleton)
	{
		for(int iLOD = 0; iLOD < m_lspLOD.Len(); iLOD++)
		{
			CXR_Model_TriangleMesh& LOD = *m_lspLOD[iLOD];
			if(LOD.m_spSkeleton)
			{
				int nCloth = m_spSkeleton->m_lCloth.Len();
				int nLODCloth = LOD.m_spSkeleton->m_lCloth.Len();

				for (int iLODCloth = 0; iLODCloth < nLODCloth; iLODCloth++)
				{
					for (int iCloth = 0; iCloth < nCloth; iCloth++)
					{
						CStr LODName = LOD.m_spSkeleton->m_lCloth[iLODCloth].m_Name;
						CStr Name = m_spSkeleton->m_lCloth[iCloth].m_Name;

						if (LOD.m_spSkeleton->m_lCloth[iLODCloth].m_Name == m_spSkeleton->m_lCloth[iCloth].m_Name)
						{
							memcpy(LOD.m_spSkeleton->m_lCloth[iLODCloth].m_lParams, m_spSkeleton->m_lCloth[iCloth].m_lParams, sizeof(m_spSkeleton->m_lCloth[iCloth].m_lParams));
						}
					}
				}

		/*
				for (int iCloth = 0; iCloth < m_spSkeleton->m_lCloth.Len() && iCloth < LOD.m_spSkeleton->m_lCloth.Len(); iCloth++)
				{
					M_TRACEALWAYS("SimFreq: %f\n",m_spSkeleton->m_lCloth[iCloth].m_lParams[CLOTHPARAM_SIMULATIONFREQUENCY]);

					memcpy(LOD.m_spSkeleton->m_lCloth[iCloth].m_lParams, m_spSkeleton->m_lCloth[iCloth].m_lParams, sizeof(m_spSkeleton->m_lCloth[iCloth].m_lParams));
				}
				*/
			}
		}
	}


	CDataFile DFile;

	// Funkar om man gör så här.... men märkligt var det ju
	try
	{
		DFile.Create(_FileName);
	}
	catch (CCException)
	{
		throw;
	}

	Write(&DFile);

	for(int i = 0; i < m_lspLOD.Len(); i++)
	{
/*		bool bSurfShare = CanShareSurfaces(m_lspLOD[i]);
		if (bSurfShare)
			m_lspLOD[i]->m_lspSurfaces.Destroy();*/

		m_lspLOD[i]->Write(&DFile);
		m_lspLOD[i]->m_lspSurfaces = lspSurf;

//		if (bSurfShare)
//			m_lspLOD[i]->m_lspSurfaces = m_lspSurfaces;
	}

	if(m_spAnim != NULL)
	{
		CXR_AnimWriteInfo WriteInfo;
		m_spAnim->Write(&DFile, WriteInfo);
	}

	DFile.Close();
}

void CXR_Model_TriangleMesh::Write(CDataFile *_pDFile)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Write_2, MAUTOSTRIP_VOID);
	_pDFile->BeginEntry("EXTENDEDMODEL");
	_pDFile->EndEntry(0);
	_pDFile->BeginSubDir();

	CTriangleMeshCore::Write(_pDFile);

	_pDFile->EndSubDir();
	if(m_spAnim != NULL)
	{
		CXR_AnimWriteInfo WriteInfo;
		m_spAnim->Write(_pDFile, WriteInfo);
	}
}
#endif

// -------------------------------------------------------------------
void CXR_Model_TriangleMesh::Cluster_DestroyAll()
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_DestroyAll, MAUTOSTRIP_VOID);
	int nVB = GetNumVertexBuffers();

	for(int i = 0; i < nVB; i++)
	{
		CTM_VertexBuffer* pTVB = GetVertexBuffer(i);
		if (pTVB->m_VBID)
		{
			m_pVBCtx->FreeID(pTVB->m_VBID);
			pTVB->m_VBID = 0;
		}
	}

	ClearVertexBuffers();
//	m_MeshFlags &= ~(CTM_MESHFLAGS_EDGES | CTM_MESHFLAGS_MATRIXIW | CTM_MESHFLAGS_TANGENTS);
}
// -------------------------------------------------------------------
void CXR_Model_TriangleMesh::CalcBoxScissor(const CRC_Viewport* _pVP, const CMat4Dfp32* _pVMat, const CBox3Dfp32& _Box, CScissorRect& _Scissor)
{
	CRC_Viewport VP = *_pVP;
	CRct ViewRect = VP.GetViewRect();
	int ScrX = ViewRect.p0.x;
	int ScrY = ViewRect.p0.y;
	int ScrW = ViewRect.GetWidth();
	int ScrH = ViewRect.GetHeight();
	CVec2Dfp32 VPScale = CVec2Dfp32(VP.GetXScale() * 0.5f, VP.GetYScale() * 0.5f);
	CVec2Dfp32 VPMid;
	VPMid[0] = (ViewRect.p0.x + ViewRect.p1.x) >> 1;
	VPMid[1] = (ViewRect.p0.y + ViewRect.p1.y) >> 1;

	CVec3Dfp32 BoxV[8];
	_Box.GetVertices(BoxV);

	CVec2Dfp32 VMin(_FP32_MAX);
	CVec2Dfp32 VMax(-_FP32_MAX);

	for(int v = 0; v < 8; v++)
	{
		fp32 vx = BoxV[v].k[0];
		fp32 vy = BoxV[v].k[1];
		fp32 vz = BoxV[v].k[2];
		fp32 z = _pVMat->k[0][2]*vx + _pVMat->k[1][2]*vy + _pVMat->k[2][2]*vz + _pVMat->k[3][2];
		if (z < 0.1f) 
		{ 
			_Scissor.SetRect(ScrX, ScrY, ScrX + ScrW, ScrY + ScrH);
			return;
		}
		fp32 zinv = 1.0f / z;
		fp32 x = (_pVMat->k[0][0]*vx + _pVMat->k[1][0]*vy + _pVMat->k[2][0]*vz + _pVMat->k[3][0]) * zinv;
		VMin.k[0] = Min(VMin.k[0], x);
		VMax.k[0] = Max(VMax.k[0], x);
		fp32 y = (_pVMat->k[0][1]*vx + _pVMat->k[1][1]*vy + _pVMat->k[2][1]*vz + _pVMat->k[3][1]) * zinv;
		VMin.k[1] = Min(VMin.k[1], y);
		VMax.k[1] = Max(VMax.k[1], y);
	}

	{
		int xmin = RoundToInt(VMin[0] * VPScale[0] + VPMid[0]);
		int min0 = Max(ScrX, Min(ScrX + ScrW, xmin));
		int ymin = RoundToInt(VMin[1] * VPScale[1] + VPMid[1]);
		int min1 = Max(ScrY, Min(ScrY + ScrH, ymin));

		int xmax = RoundToInt(VMax[0] * VPScale[0] + VPMid[0]);
		int max0 = Max(ScrX, Min(ScrX + ScrW, xmax));
		int ymax = RoundToInt(VMax[1] * VPScale[1] + VPMid[1]);
		int max1 = Max(ScrY, Min(ScrY + ScrH, ymax));

		_Scissor.SetRect(min0, min1, Max(min0, max0), Max(min1, max1));
	}
};


bool CXR_Model_TriangleMesh::Cluster_SetMatrixPalette(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_Cluster* _pC, CTM_VertexBuffer* _pTVB, CMat4Dfp32* _pMatrixPalette, int _nMatrixPalette, CXR_VertexBuffer* _pVB, uint16 _VpuTaskId, CRC_MatrixPalette* _pMP)
{
	MSCOPESHORT(CXR_Model_TriangleMesh::Cluster_SetMatrixPalette);
	CXR_VBManager* pVBM = _pRenderParams->m_pVBM;
	CRC_MatrixPalette* M_RESTRICT pMP = _pMP;
	if(!pMP)
	{
		pMP = new(pVBM->Alloc(sizeof(CRC_MatrixPalette))) CRC_MatrixPalette;
		if (!pMP)
			return false;
	}
	int nMP = _pC->GetNumBDMatrixMap(this);
	if (nMP && _pMatrixPalette)
	{
		pMP->m_Flags = 0;
		pMP->m_pMatrices = _pMatrixPalette;
		pMP->m_piMatrices = _pC->GetBDMatrixMap(this);
		pMP->m_nMatrices = nMP;
		pMP->m_VpuTaskId = _VpuTaskId;
		_pVB->m_pMatrixPaletteArgs = pMP;

		/*
		const uint8* pBDMatrixMap = _pTVB->m_lBDMatrixMap.GetBasePtr();
		for(int i = 0; i < nMP; i++)
		{
			int iMat = pBDMatrixMap[i];
			if (iMat >= _nMatrixPalette)
				pMP[i].Unit();
			else
				pMP[i] = _pMatrixPalette[iMat];
//pMP[i] = pMatrixPalette[0];
		}

		_pVB->m_pMatrixPalette = pMP;
		_pVB->m_nMatrixPalette = nMP;
		*/
	}
	else
	{
		pMP->m_Flags = 0;
		pMP->m_nMatrices = _nMatrixPalette;
		pMP->m_pMatrices = _pMatrixPalette;
		pMP->m_piMatrices = NULL;
		pMP->m_VpuTaskId = _VpuTaskId;

		_pVB->m_pMatrixPaletteArgs = pMP;
//		_pVB->m_pMatrixPalette = _pMatrixPalette;
//		_pVB->m_nMatrixPalette = _nMatrixPalette;
	}

	return true;
}


void CXR_Model_TriangleMesh::Cluster_Render(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_Cluster* _pC)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_Render, MAUTOSTRIP_VOID);

	CXR_VBManager* pVBM = _pRenderParams->m_pVBM;
	CPixel32* pDiffuse = NULL;
//	CPixel32* pSpecular = NULL;
	fp32 FogPriority = 0; 

	CTM_VertexBuffer* pTVB = GetVertexBuffer(_pC->m_iVB);

	int nV = 0;
	if (_pRenderParams->m_bRenderTempTLEnable)
	{
		if (!_pRenderParams->m_RenderVB.AllocVBChain(pVBM, true))
			return;
		if(m_bUsePrimitives || (_pC->m_nIBPrim == 0))
			_pRenderParams->m_RenderVB.Render_VertexBuffer(pTVB->m_VBID);
		else
		{
			CTM_VertexBuffer* pTIB = GetVertexBuffer(_pC->m_iIB);
			_pRenderParams->m_RenderVB.Render_VertexBuffer_IndexBufferTriangles(pTVB->m_VBID, pTIB->m_VBID, _pC->m_nIBPrim / 3, _pC->m_iIBOffset);
		}
	}
	else
	{
		nV = pTVB->GetNumVertices(this);
		if (!_pRenderParams->m_RenderVB.AllocVBChain(pVBM, false))
			return;
		_pRenderParams->m_RenderVB.Geometry_VertexArray(_pRenderParams->m_pRenderV, nV, true);
		_pRenderParams->m_RenderVB.Geometry_TVertexArray(_pRenderParams->m_pRenderTV, 0);
		if (m_bUsePrimitives)
			_pRenderParams->m_RenderVB.Render_IndexedPrimitives(pTVB->GetPrimitives(this), pTVB->GetNumPrimitives(this));
		else if(_pC->m_nIBPrim != 0)
		{
			CTM_VertexBuffer* pTIB = GetVertexBuffer(_pC->m_iIB);
			_pRenderParams->m_RenderVB.Render_IndexedTriangles(pTIB->GetTriangles(this) + _pC->m_iIBOffset, _pC->m_nIBPrim);
		}
		else
			_pRenderParams->m_RenderVB.Render_IndexedTriangles(pTVB->GetTriangles(this), pTVB->GetNumTriangles(this));

//		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
//		m_lpSurfaces[0] = pSC->GetSurface("DEFAULT");
//		m_lpSurfaceKeyFrames[0] = m_lpSurfaces[0]->GetFrame(0, 0, _pRenderParams->m_lTmpSurfaceKeyFrames[0]);
	}
/*#ifndef	PLATFORM_PS2
	{
		// Render cluster into Z-Buffer
		CRC_Attributes* pAZBuffer = pVBM->Alloc_Attrib();
		if (!pAZBuffer)
			return;

		pAZBuffer->SetDefault();

		pAZBuffer->Attrib_Enable(CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);

		pAZBuffer->Attrib_Disable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE);

		{
			CXW_Surface* pSurf = m_lpSurfaces[0];
			const uint32 Ambience = _pRenderParams->m_pCurrentEngine->GetVar(XR_ENGINE_UNIFIED_AMBIENCE);

			CXR_VertexBuffer* pVB = pVBM->Alloc_VB();
			if (!pVB) 
				return;

			pVB->CopyVBChain(&_pRenderParams->m_RenderVB);
			pVB->m_pMatrixPaletteArgs = _pRenderParams->m_RenderVB.m_pMatrixPaletteArgs;
			pVB->m_Color = Ambience;

			CXW_SurfaceLayer* pLayer = &pSurf->GetBaseFrame()->m_lTextures[0];
			if (pLayer->m_Flags & XW_LAYERFLAGS_ALPHACOMPARE)
			{
				CRC_Attributes* pA = pVBM->Alloc_Attrib();
				*pA = *pAZBuffer;
				pA->Attrib_TextureID(0, pLayer->m_TextureID);
				pA->Attrib_AlphaCompare(pLayer->m_AlphaFunc, pLayer->m_AlphaRef);
				pVB->m_pAttrib = pA;
			}
			else
			{
				pVB->m_pAttrib = pAZBuffer;
			}
			pVB->Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
			pVB->m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER;

			pVBM->AddVB(pVB);
		}
	}
#endif*/

	fp32 Offset, BasePriority;
	{
		CXW_Surface* pSurf = _pRenderParams->m_lpSurfaces[0];
//		CXW_SurfaceKeyFrame* pSurfKey = _pRenderParams->m_lpSurfaceKeyFrames[0];
		if (!pSurf) return;

		Offset = pSurf->m_PriorityOffset;

#ifdef PLATFORM_PS2
		BasePriority = _pRenderParams->m_RenderInfo.m_BasePriority_Opaque;
//		BasePriority = CXR_VBPRIORITY_MODEL_OPAQUE_LM;
#else
		BasePriority = _pRenderParams->m_RenderInfo.m_BasePriority_Opaque + 5;
#endif

		if ((pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT) || 
			(CPixel32(_pRenderParams->m_pCurAnimState->m_Data[0]).GetA() < 255) /*||
			// Is the following line really needed? -JA
			// Dunno...  it's an old fix for models with poorly defined surfaces.  -MH
			(pSurfKey->m_lTextures.Len() > 0 && pSurfKey->m_lTextures[0].m_RasterMode)*/)
		{
			BasePriority = _pRenderParams->m_RenderInfo.m_BasePriority_Transparent;
		}
		Offset *= 0.001f;
		BasePriority += Offset;
		FogPriority = BasePriority;
	}

	
	if (!_pRenderParams->m_RenderVB.IsVBIDChain())
		_pRenderParams->m_RenderVB.GetVBChain()->m_pN = _pRenderParams->m_pRenderN;
	for(int iSurf = m_bDefaultSurface ? 0 : 1 ; (iSurf < TRIMESH_NUMSURF); iSurf++)
	{
		CXW_Surface* pSurf = _pRenderParams->m_lpSurfaces[iSurf];
		if (!pSurf)
			continue;
		CXW_SurfaceKeyFrame* pSurfKey = _pRenderParams->m_lpSurfaceKeyFrames[iSurf];

		if(!(m_RenderFlags & CTM_RFLAGS_NOTRIANGLES))
		{
			CXR_RenderSurfExtParam Params;

// FIXME: Broken
//Unbroken =) - ae
			Params.m_pLights = _pRenderParams->m_pRenderLights;
			Params.m_nLights = _pRenderParams->m_nRenderLights;
			Params.m_lUserColors[0] = _pRenderParams->m_pCurAnimState->m_Data[0];
			Params.m_lUserColors[1] = _pRenderParams->m_pCurAnimState->m_Data[1];

			int Flags = RENDERSURFACE_MATRIXSTATIC_M2V;// | RENDERSURFACE_VERTEXFOG;
			if (Params.m_lUserColors[0].GetA() < 255)
				Flags |= RENDERSURFACE_ALPHABLEND;

			CXR_Util::Render_Surface(Flags, _pRenderParams->m_lSurfaceTime[iSurf], pSurf, pSurfKey, _pRenderParams->m_pCurrentEngine, pVBM, &_pRenderParams->m_CurVertex2WorldMat, NULL, _pRenderParams->m_RenderVB.m_pTransform, &_pRenderParams->m_RenderVB, BasePriority, 0.001f, &Params);
		}

		BasePriority += 0.01f;
	}

	if (m_RenderFlags & CTM_RFLAGS_WIRE)
	{
		CXR_VertexBuffer VB;
		if (!VB.AllocVBChain(pVBM, false))
			return;
		CTM_Edge* pEdges = pTVB->GetEdges(this);
		int nEdges = pTVB->GetNumEdges(this);
		if(_pC->m_nEdges)
		{
			pEdges += _pC->m_iEdges;
			nEdges = _pC->m_nEdges;
		}
		VB.Render_IndexedWires(pEdges->m_iV, nEdges * 2);
		if(!_pRenderParams->m_pRenderV)
		{
			nV = pTVB->GetNumVertices(this);
			_pRenderParams->m_pRenderV = pVBM->Alloc_V3(nV);
			memcpy(_pRenderParams->m_pRenderV, pTVB->GetVertexPtr(this, 0), nV*sizeof(CVec3Dfp32));
		}
		VB.Geometry_VertexArray(_pRenderParams->m_pRenderV, nV, true);
		VB.Geometry_ColorArray(pDiffuse);
		CRC_Attributes* pA = pVBM->Alloc_Attrib();
		if (!pA) return;
		pA->SetDefault();
		pA->m_iTexCoordSet[1] = 0; pA->m_iTexCoordSet[2] = 1; pA->m_iTexCoordSet[3] = 2;
		VB.m_pAttrib = pA;
		VB.Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
		pVBM->AddVB(VB);
	}

	if (_pRenderParams->m_pSceneFog && _pRenderParams->m_RenderInfo.m_Flags & CXR_RENDERINFO_NHF)
	{
		CRC_Attributes* pA = pVBM->Alloc_Attrib();
		if (!pA) return;
		_pRenderParams->m_RenderVB.m_pAttrib = pA;
		pA->SetDefault();

		if(_pRenderParams->m_pCurrentEngine) _pRenderParams->m_pCurrentEngine->SetDefaultAttrib(pA);
		pA->m_iTexCoordSet[1] = 0; pA->m_iTexCoordSet[2] = 1; pA->m_iTexCoordSet[3] = 2;

		CXW_Surface* pSurf = _pRenderParams->m_lpSurfaces[0];
		if (!pSurf)
			return;

		if (pSurf->m_Flags & XW_SURFFLAGS_NOCULL)
			pA->Attrib_Disable(CRC_FLAGS_CULL);
		else
			pA->Attrib_Enable(CRC_FLAGS_CULL);
		pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
		pA->Attrib_TextureID(0, _pRenderParams->m_pSceneFog->m_FogTableTextureID);
		pA->Attrib_ZCompare(CRC_COMPARE_EQUAL);
		pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

		if ((_pRenderParams->m_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20) ||
		    (_pRenderParams->m_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_TEXGENMODE_BOXNHF))
		{
			CVec4Dfp32* pParams = pVBM->Alloc_V4(10);
			if (!pParams)
				return;

			for(int i = 0; i < 8; i++)
			{
				pParams[i][0] = fp32(_pRenderParams->m_RenderFog[i].GetR()) / 255.0f;
				pParams[i][1] = fp32(_pRenderParams->m_RenderFog[i].GetG()) / 255.0f;
				pParams[i][2] = fp32(_pRenderParams->m_RenderFog[i].GetB()) / 255.0f;
				pParams[i][3] = fp32(_pRenderParams->m_RenderFog[i].GetA()) / 255.0f;
			}

			pParams[8][0] = _pRenderParams->m_RenderFogBox.m_Min[0];
			pParams[8][1] = _pRenderParams->m_RenderFogBox.m_Min[1];
			pParams[8][2] = _pRenderParams->m_RenderFogBox.m_Min[2];
			pParams[8][3] = 0;
			pParams[9][0] = 1.0f / (_pRenderParams->m_RenderFogBox.m_Max[0] - _pRenderParams->m_RenderFogBox.m_Min[0]);
			pParams[9][1] = 1.0f / (_pRenderParams->m_RenderFogBox.m_Max[1] - _pRenderParams->m_RenderFogBox.m_Min[1]);
			pParams[9][2] = 1.0f / (_pRenderParams->m_RenderFogBox.m_Max[2] - _pRenderParams->m_RenderFogBox.m_Min[2]);
			pParams[9][3] = 0;

			pA->Attrib_TexGenAttr((fp32*)pParams);
			pA->Attrib_TexGen(0, CRC_TEXGENMODE_BOXNHF, CRC_TEXGENCOMP_ALL);

			_pRenderParams->m_RenderVB.m_Priority = FogPriority + CXR_VBPRIORITY_VOLUMETRICFOG;
			if (_pRenderParams->m_pSceneFog) _pRenderParams->m_pSceneFog->SetDepthFogNone(_pRenderParams->m_RenderVB.m_pAttrib);
			pVBM->AddVB(_pRenderParams->m_RenderVB);
		}
		else
		{
			CPixel32* pFog = pVBM->Alloc_CPixel32(nV);
			CVec2Dfp32* pTV = pVBM->Alloc_V2(nV);
			if (!pFog || !pTV) return;
			_pRenderParams->m_RenderVB.Geometry_ColorArray(pFog);
			_pRenderParams->m_RenderVB.Geometry_Color(0xffffffff);
			_pRenderParams->m_RenderVB.Geometry_TVertexArray(pTV, 0);

			const CMat4Dfp32* pMat = (_pRenderParams->m_bRenderFogWorldSpace) ? &_pRenderParams->m_CurVertex2WorldMat : &_pRenderParams->m_CurVertex2LocalMat;
			_pRenderParams->m_pSceneFog->InterpolateBox(_pRenderParams->m_RenderFogBox, _pRenderParams->m_RenderFog, nV, _pRenderParams->m_pRenderV, pFog, pTV, pMat);

			{
				_pRenderParams->m_RenderVB.m_Priority = FogPriority + CXR_VBPRIORITY_VOLUMETRICFOG;
				if (_pRenderParams->m_pSceneFog) _pRenderParams->m_pSceneFog->SetDepthFogNone(_pRenderParams->m_RenderVB.m_pAttrib);
				pVBM->AddVB(_pRenderParams->m_RenderVB);
			}
		}
	}
}

void CXR_Model_TriangleMesh::Cluster_RenderProjLight(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_Cluster* _pC)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_Render, MAUTOSTRIP_VOID);

	CXR_VBManager* pVBM = _pRenderParams->m_pVBM;
	CPixel32* pDiffuse = NULL;
//	CPixel32* pSpecular = NULL;
	fp32 FogPriority = 0; 

	CTM_VertexBuffer* pTVB = GetVertexBuffer(_pC->m_iVB);

	int nV = 0;
	if (_pRenderParams->m_bRenderTempTLEnable)
	{
		if (!_pRenderParams->m_RenderVB.AllocVBChain(pVBM, true))
			return;
		if(m_bUsePrimitives || (_pC->m_nIBPrim == 0))
			_pRenderParams->m_RenderVB.Render_VertexBuffer(pTVB->m_VBID);
		else
		{
			CTM_VertexBuffer* pTIB = GetVertexBuffer(_pC->m_iIB);
			_pRenderParams->m_RenderVB.Render_VertexBuffer_IndexBufferTriangles(pTVB->m_VBID, pTIB->m_VBID, _pC->m_nIBPrim / 3, _pC->m_iIBOffset);
		}
	}
	else
	{
		nV = pTVB->GetNumVertices(this);
		if (!_pRenderParams->m_RenderVB.AllocVBChain(pVBM, false))
			return;
		_pRenderParams->m_RenderVB.Geometry_VertexArray(_pRenderParams->m_pRenderV, nV, true);
		_pRenderParams->m_RenderVB.Geometry_TVertexArray(_pRenderParams->m_pRenderTV, 0);
		if (m_bUsePrimitives)
			_pRenderParams->m_RenderVB.Render_IndexedPrimitives(pTVB->GetPrimitives(this), pTVB->GetNumPrimitives(this));
		else if(_pC->m_nIBPrim != 0)
		{
			CTM_VertexBuffer* pTIB = GetVertexBuffer(_pC->m_iIB);
			_pRenderParams->m_RenderVB.Render_IndexedTriangles(pTIB->GetTriangles(this) + _pC->m_iIBOffset, _pC->m_nIBPrim);
		}
		else
			_pRenderParams->m_RenderVB.Render_IndexedTriangles(pTVB->GetTriangles(this), pTVB->GetNumTriangles(this));

//		MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
//		m_lpSurfaces[0] = pSC->GetSurface("DEFAULT");
//		m_lpSurfaceKeyFrames[0] = m_lpSurfaces[0]->GetFrame(0, 0, _pRenderParams->m_lTmpSurfaceKeyFrames[0]);
	}

#ifndef	PLATFORM_PS2
	{
		// Render cluster into Z-Buffer
		CRC_Attributes* pAZBuffer = pVBM->Alloc_Attrib();
		if (!pAZBuffer)
			return;

		pAZBuffer->SetDefault();

		pAZBuffer->Attrib_Enable(CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);

		pAZBuffer->Attrib_Disable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE);

		{
			CXW_Surface* pSurf = _pRenderParams->m_lpSurfaces[0];
			const uint32 Ambience = _pRenderParams->m_pCurrentEngine->GetVar(XR_ENGINE_UNIFIED_AMBIENCE);

			CXR_VertexBuffer* pVB = pVBM->Alloc_VB();
			if (!pVB) 
				return;

			pVB->CopyVBChain(&_pRenderParams->m_RenderVB);
			pVB->m_pMatrixPaletteArgs = _pRenderParams->m_RenderVB.m_pMatrixPaletteArgs;
			pVB->Geometry_Color(Ambience);

			CXW_SurfaceLayer* pLayer = &pSurf->GetBaseFrame()->m_lTextures[0];
			if (pLayer->m_Flags & XW_LAYERFLAGS_ALPHACOMPARE)
			{
				CRC_Attributes* pA = pVBM->Alloc_Attrib();
				*pA = *pAZBuffer;
				pA->Attrib_TextureID(0, pLayer->m_TextureID);
				pA->Attrib_AlphaCompare(pLayer->m_AlphaFunc, pLayer->m_AlphaRef);
				pVB->m_pAttrib = pA;
			}
			else
			{
				pVB->m_pAttrib = pAZBuffer;
			}
			pVB->Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
			pVB->m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER;

			pVBM->AddVB(pVB);
		}
	}
#endif

	fp32 Offset, BasePriority;
	{
		CXW_Surface* pSurf = _pRenderParams->m_lpSurfaces[0];
//		CXW_SurfaceKeyFrame* pSurfKey = _pRenderParams->m_lpSurfaceKeyFrames[0];
		if (!pSurf) return;

		Offset = pSurf->m_PriorityOffset;

#ifdef PLATFORM_PS2
//		BasePriority = _pRenderParams->m_RenderInfo.m_BasePriority_Opaque;
		BasePriority = CXR_VBPRIORITY_MODEL_OPAQUE_LM;
#else
		BasePriority = _pRenderParams->m_RenderInfo.m_BasePriority_Opaque + 5;
#endif

		if ((pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT) || 
			(CPixel32(_pRenderParams->m_pCurAnimState->m_Data[0]).GetA() < 255) /*||
			// Is the following line really needed? -JA
			// Dunno...  it's an old fix for models with poorly defined surfaces.  -MH
			(pSurfKey->m_lTextures.Len() > 0 && pSurfKey->m_lTextures[0].m_RasterMode)*/)
		{
			BasePriority = _pRenderParams->m_RenderInfo.m_BasePriority_Transparent;
		}
		Offset *= 0.001f;
		BasePriority += Offset;
		FogPriority = BasePriority;
	}

	if (!_pRenderParams->m_RenderVB.IsVBIDChain())
		_pRenderParams->m_RenderVB.GetVBChain()->m_pN = _pRenderParams->m_pRenderN;

	for(int iSurf = m_bDefaultSurface ? 0 : 1 ; (iSurf < TRIMESH_NUMSURF) && _pRenderParams->m_lpSurfaces[iSurf]; iSurf++)
	{
		CXW_Surface* pSurf = _pRenderParams->m_lpSurfaces[iSurf];
		CXW_SurfaceKeyFrame* pSurfKey = _pRenderParams->m_lpSurfaceKeyFrames[iSurf];

		if (m_RenderFlags & CTM_RFLAGS_WIRE)
		{
			CXR_VertexBuffer VB;
			if (!VB.AllocVBChain(pVBM, false))
				return;
			CTM_Edge* pEdges = pTVB->GetEdges(this);
			int nEdges = pTVB->GetNumEdges(this);
			if(_pC->m_nEdges)
			{
				pEdges += _pC->m_iEdges;
				nEdges = _pC->m_nEdges;
			}
			VB.Render_IndexedWires(pEdges->m_iV, nEdges * 2);
			VB.Geometry_VertexArray(_pRenderParams->m_pRenderV, nV, true);
			VB.Geometry_ColorArray(pDiffuse);
			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			if (!pA) return;
			pA->SetDefault();
			pA->m_iTexCoordSet[1] = 0; pA->m_iTexCoordSet[2] = 1; pA->m_iTexCoordSet[3] = 2;
			VB.m_pAttrib = pA;
			VB.Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
			pVBM->AddVB(VB);
		}

		if(!(m_RenderFlags & CTM_RFLAGS_NOTRIANGLES))
		{
			CXR_RenderSurfExtParam Params;

// FIXME: Broken
//Unbroken =) - ae
			Params.m_pLights = _pRenderParams->m_pRenderLights;
			Params.m_nLights = _pRenderParams->m_nRenderLights;
			Params.m_lUserColors[0] = _pRenderParams->m_pCurAnimState->m_Data[0];
			Params.m_lUserColors[1] = _pRenderParams->m_pCurAnimState->m_Data[1];

			int Flags = RENDERSURFACE_MATRIXSTATIC_M2V;// | RENDERSURFACE_VERTEXFOG;
			if (Params.m_lUserColors[0].GetA() < 255)
				Flags |= RENDERSURFACE_ALPHABLEND;

#ifndef	PLATFORM_PS2
			if( iSurf == 0 )
			{
				Flags |= RENDERSURFACE_MODULATELIGHT;
			}
			else
			{
				Flags &= ~RENDERSURFACE_MODULATELIGHT;
			}
#endif

			CXR_Util::Render_Surface(Flags, _pRenderParams->m_lSurfaceTime[iSurf], pSurf, pSurfKey, _pRenderParams->m_pCurrentEngine, pVBM, &_pRenderParams->m_CurVertex2WorldMat, NULL, _pRenderParams->m_RenderVB.m_pTransform, &_pRenderParams->m_RenderVB,  BasePriority, 0.001f, &Params);
		}

		BasePriority += 0.01f;
	}

	for( int iLight = 0; iLight < _pRenderParams->m_RenderInfo.m_nLights; iLight++ )
	{
#ifdef	PLATFORM_PS2
		const fp32 LightScaler = 64.0f;
#else
		const fp32 LightScaler = 128.0f;
#endif

		const CXR_Light* pEngineLight = _pRenderParams->m_RenderInfo.m_pLightInfo[iLight].m_pLight;

		CXR_VertexBuffer* pVB = pVBM->Alloc_VB();
		if (!pVB) 
			break;

		CVec4Dfp32 LightColor = pEngineLight->GetIntensityv();
		pVB->CopyVBChain(&_pRenderParams->m_RenderVB);
		pVB->m_pMatrixPaletteArgs = _pRenderParams->m_RenderVB.m_pMatrixPaletteArgs;
		pVB->m_Color	= CPixel32((int)(LightColor[0] * LightScaler), (int)(LightColor[1] * LightScaler), (int)(LightColor[2] * LightScaler), 255 );

		pVB->m_pAttrib = _pRenderParams->m_pLightAttrib[iLight];
		pVB->m_Priority = CXR_VBPRIORITY_DYNLIGHT + pEngineLight->m_iLight*0.01f;
		pVB->Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
		pVBM->AddVB( pVB );
	}

	if (_pRenderParams->m_pSceneFog && _pRenderParams->m_RenderInfo.m_Flags & CXR_RENDERINFO_NHF)
	{
		CRC_Attributes* pA = pVBM->Alloc_Attrib();
		if (!pA) return;
		_pRenderParams->m_RenderVB.m_pAttrib = pA;
		pA->SetDefault();

		if(_pRenderParams->m_pCurrentEngine) _pRenderParams->m_pCurrentEngine->SetDefaultAttrib(pA);
		pA->m_iTexCoordSet[1] = 0; pA->m_iTexCoordSet[2] = 1; pA->m_iTexCoordSet[3] = 2;

		CXW_Surface* pSurf = _pRenderParams->m_lpSurfaces[0];
		if (!pSurf)
			return;

		if (pSurf->m_Flags & XW_SURFFLAGS_NOCULL)
			pA->Attrib_Disable(CRC_FLAGS_CULL);
		else
			pA->Attrib_Enable(CRC_FLAGS_CULL);
		pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
		pA->Attrib_TextureID(0, _pRenderParams->m_pSceneFog->m_FogTableTextureID);
		pA->Attrib_ZCompare(CRC_COMPARE_EQUAL);
		pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

		if ((_pRenderParams->m_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20) ||
		    (_pRenderParams->m_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_TEXGENMODE_BOXNHF))
		{
			CVec4Dfp32* pParams = pVBM->Alloc_V4(10);
			if (!pParams)
				return;

			for(int i = 0; i < 8; i++)
			{
				pParams[i][0] = fp32(_pRenderParams->m_RenderFog[i].GetR()) / 255.0f;
				pParams[i][1] = fp32(_pRenderParams->m_RenderFog[i].GetG()) / 255.0f;
				pParams[i][2] = fp32(_pRenderParams->m_RenderFog[i].GetB()) / 255.0f;
				pParams[i][3] = fp32(_pRenderParams->m_RenderFog[i].GetA()) / 255.0f;
			}

			pParams[8][0] = _pRenderParams->m_RenderFogBox.m_Min[0];
			pParams[8][1] = _pRenderParams->m_RenderFogBox.m_Min[1];
			pParams[8][2] = _pRenderParams->m_RenderFogBox.m_Min[2];
			pParams[8][3] = 0;
			pParams[9][0] = 1.0f / (_pRenderParams->m_RenderFogBox.m_Max[0] - _pRenderParams->m_RenderFogBox.m_Min[0]);
			pParams[9][1] = 1.0f / (_pRenderParams->m_RenderFogBox.m_Max[1] - _pRenderParams->m_RenderFogBox.m_Min[1]);
			pParams[9][2] = 1.0f / (_pRenderParams->m_RenderFogBox.m_Max[2] - _pRenderParams->m_RenderFogBox.m_Min[2]);
			pParams[9][3] = 0;

			pA->Attrib_TexGenAttr((fp32*)pParams);
			pA->Attrib_TexGen(0, CRC_TEXGENMODE_BOXNHF, CRC_TEXGENCOMP_ALL);

			_pRenderParams->m_RenderVB.m_Priority = FogPriority + CXR_VBPRIORITY_VOLUMETRICFOG;
			if (_pRenderParams->m_pSceneFog) _pRenderParams->m_pSceneFog->SetDepthFogNone(_pRenderParams->m_RenderVB.m_pAttrib);
			pVBM->AddVB(_pRenderParams->m_RenderVB);
		}
		else
		{
			CXR_VertexBuffer* pVB = &_pRenderParams->m_RenderVB;
			CPixel32* pFog = pVBM->Alloc_CPixel32(nV);
			CVec2Dfp32* pTV = pVBM->Alloc_V2(nV);
			if (!pFog || !pTV) return;
			pVB->Geometry_ColorArray(pFog);
			pVB->Geometry_Color(0xffffffff);
			pVB->Geometry_TVertexArray(pTV, 0);

			const CMat4Dfp32* pMat = (_pRenderParams->m_bRenderFogWorldSpace) ? &_pRenderParams->m_CurVertex2WorldMat : &_pRenderParams->m_CurVertex2LocalMat;
			_pRenderParams->m_pSceneFog->InterpolateBox(_pRenderParams->m_RenderFogBox, _pRenderParams->m_RenderFog, nV, _pRenderParams->m_pRenderV, pFog, pTV, pMat);

			{
				pVB->m_Priority = FogPriority + CXR_VBPRIORITY_VOLUMETRICFOG;
				_pRenderParams->m_pSceneFog->SetDepthFogNone(pVB->m_pAttrib);
				pVBM->AddVB(*pVB);
			}
		}
	}
}

void VBM_Get2DMatrix(CRC_Viewport* _pVB, CMat4Dfp32& _Mat)
{
	MAUTOSTRIP( VBM_Get2DMatrix, MAUTOSTRIP_VOID );
	CRct View = _pVB->GetViewRect();
	CClipRect Clip = _pVB->GetViewClip();
//	fp32 cw = Clip.clip.GetWidth();
//	fp32 ch = Clip.clip.GetHeight();
	fp32 w = View.GetWidth();
	fp32 h = View.GetHeight();
	fp32 dx = -w;
	fp32 dy = -h;

	_Mat.Unit();
//	CVec3Dfp32::GetMatrixRow(m_CurTransform, 3) = CVec3Dfp32(dx/2.0f, dy/2.0f, _pVP->GetXScale()*0.5f);
//	const fp32 Z = 16.0f;
	const fp32 Z = _pVB->GetBackPlane() - 16.0f;

	fp32 xScale = _pVB->GetXScale() * 0.5;
	fp32 yScale = _pVB->GetXScale() * 0.5;

	_Mat.k[0][0] = Z / xScale;
	_Mat.k[1][1] = Z / yScale;
	_Mat.k[3][0] = (Z*((dx)/2.0f /*- m_TextOffset*/) / xScale);
	_Mat.k[3][1] = (Z*((dy)/2.0f /*- m_TextOffset*/) / yScale);
	_Mat.k[3][2] = Z;
}

void VBM_RenderRect(CRC_Viewport* _pVP, CXR_VBManager* _pVBM, const CScissorRect& _Rect, CPixel32 _Color, fp32 _Priority, CRC_Attributes* _pA)
{
	MAUTOSTRIP( VBM_RenderRect, MAUTOSTRIP_VOID );
	CXR_VertexBuffer* pVB = _pVBM->Alloc_VB(CXR_VB_VERTICES, 4);
	if (!pVB)
		return;

	CMat4Dfp32 Mat;
	VBM_Get2DMatrix(_pVP, Mat);

	pVB->m_pAttrib = _pA;
	CVec3Dfp32* pV = pVB->GetVBChain()->m_pV;

	uint32 MinX, MinY, MaxX, MaxY;
	_Rect.GetRect(MinX, MinY, MaxX, MaxY);

	CVec3Dfp32(MinX, MinY, 0).MultiplyMatrix(Mat, pV[0]);
	CVec3Dfp32(MaxX, MinY, 0).MultiplyMatrix(Mat, pV[1]);
	CVec3Dfp32(MaxX, MaxY, 0).MultiplyMatrix(Mat, pV[2]);
	CVec3Dfp32(MinX, MaxY, 0).MultiplyMatrix(Mat, pV[3]);

	pVB->m_Priority = _Priority;
	CXR_Util::Init();
	pVB->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, 2);
	pVB->m_Color = _Color;
	_pVBM->AddVB(pVB);
}

void VBM_RenderRect_AlphaBlend(CRC_Viewport* _pVP, CXR_VBManager* _pVBM, const CScissorRect& _Rect, CPixel32 _Color, fp32 _Priority)
{
	MAUTOSTRIP( VBM_RenderRect_AlphaBlend, MAUTOSTRIP_VOID );
	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if (!pA)
		return;

	pA->SetDefault();
	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);

	VBM_RenderRect(_pVP, _pVBM, _Rect, _Color, _Priority, pA);
}

static void CalcCircleTangentPoints(fp32 _p, fp32 _q, fp32 _r, CVec2Dfp32& _Tang0, CVec2Dfp32& _Tang1)
{
	MAUTOSTRIP( CalcCircleTangentPoints, MAUTOSTRIP_VOID );
	fp32 rsqr1 = M_Sqrt(Sqr(_p) + Sqr(_q) - Sqr(_r));
	fp32 recp = 1.0f / (Sqr(_p) + Sqr(_q));

	fp32 t0x = rsqr1 * (_p * rsqr1 + _q*_r) * recp;
	fp32 t1x = rsqr1 * (_p * rsqr1 - _q*_r) * recp;
	fp32 t0y = M_Sqrt(Sqr(_p) + Sqr(_q) - Sqr(_r) - Sqr(t0x));
	fp32 t1y = M_Sqrt(Sqr(_p) + Sqr(_q) - Sqr(_r) - Sqr(t1x));

	_Tang0[0] = t0x;
	_Tang0[1] = t0y;
	_Tang1[0] = t1x;
	_Tang1[1] = t1y;
}


void CXR_Model_TriangleMesh::Cluster_RenderUnified(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_Cluster* _pC, int _iCluster)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_RenderUnified, MAUTOSTRIP_VOID);

	CXR_VBManager* pVBM = _pRenderParams->m_pVBM;
	CTM_VertexBuffer* pTVB = GetVertexBuffer(_pC->m_iVB);

	if (_pRenderParams->m_bRenderTempTLEnable)
	{
		if (!_pRenderParams->m_RenderVB.AllocVBChain(pVBM, true))
			return;
		int PrimType;
		if (_pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE)
		{
			_pRenderParams->m_RenderVB.Geometry_VertexBuffer(pTVB->m_VBID, false);
			PrimType = 0;
		}
		else
		{
			if(m_bUsePrimitives || (_pC->m_nIBPrim == 0))
			{
				_pRenderParams->m_RenderVB.Render_VertexBuffer(pTVB->m_VBID);
				PrimType = CRC_RIP_VBID;
			}
			else
			{
				CTM_VertexBuffer* pTIB = GetVertexBuffer(_pC->m_iIB);
				_pRenderParams->m_RenderVB.Render_VertexBuffer_IndexBufferTriangles(pTVB->m_VBID, pTIB->m_VBID, _pC->m_nIBPrim / 3, _pC->m_iIBOffset);
				PrimType = CRC_RIP_VBID_IBTRIANGLES;
			}
		}

		uint nC = _pRenderParams->m_nClusters;
		CXR_VBIDChain* pHead = _pRenderParams->m_RenderVB.GetVBIDChain();
		CXR_VBIDChain* pTail = pHead;
		for(uint iC = 0; iC < nC; iC++)
		{
			CXR_VBIDChain* pChain = pVBM->Alloc_VBIDChain();
			CTM_Cluster* pC2 = _pRenderParams->m_lpCluster[iC];
			CTM_VertexBuffer* pTVB = GetVertexBuffer(pC2->m_iVB);
			if(PrimType == CRC_RIP_VBID_IBTRIANGLES)
			{
				CTM_VertexBuffer* pTIB = GetVertexBuffer(pC2->m_iIB);
				pChain->Render_VertexBuffer_IndexBufferTriangles(pTVB->m_VBID, pTIB->m_VBID, pC2->m_nIBPrim / 3, pC2->m_iIBOffset);
			}
			else
				pChain->m_VBID = pTVB->m_VBID;
			pChain->m_PrimType = PrimType;

			pTail->m_pNextVB = pChain;
			pTail = pChain;
		}

	}
	else
	{
		if (!_pRenderParams->m_RenderVB.AllocVBChain(pVBM, false))
			return;
		int nV = pTVB->GetNumVertices(this);
		_pRenderParams->m_RenderVB.Geometry_VertexArray(_pRenderParams->m_pRenderV, nV, true);
		_pRenderParams->m_RenderVB.Geometry_TVertexArray(_pRenderParams->m_pRenderTV, 0);

		if (!_pRenderParams->m_pRenderTangU || !_pRenderParams->m_pRenderTangV)
			return;

		if (!(_pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE))
		{
			if (m_bUsePrimitives)
				_pRenderParams->m_RenderVB.Render_IndexedPrimitives(pTVB->GetPrimitives(this), pTVB->GetNumPrimitives(this));
			else if(_pC->m_nIBPrim)
			{
				CTM_VertexBuffer* pTIB = GetVertexBuffer(_pC->m_iIB);
				_pRenderParams->m_RenderVB.Render_IndexedTriangles(pTIB->GetTriangles(this) + _pC->m_iIBOffset, _pC->m_nIBPrim);
			}
			else
				_pRenderParams->m_RenderVB.Render_IndexedTriangles(pTVB->GetTriangles(this), pTVB->GetNumTriangles(this));
		}
	}

	bool bDrawShading;
	VB_RenderUnified(_pRenderParams, _pC, _iCluster, bDrawShading);

	fp32 Offset, BasePriority, BaseOffset;
	{
		CXW_Surface* pSurf = _pRenderParams->m_lpSurfaces[0];
		if (!pSurf) return;

		Offset = pSurf->m_PriorityOffset;
		BasePriority = _pRenderParams->m_RenderInfo.m_BasePriority_Opaque;

		if (pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT)
		{
			BasePriority = _pRenderParams->m_RenderInfo.m_BasePriority_Transparent;
			Offset *= 0.001f;
		}
		BasePriority += Offset;
		BaseOffset = 0.001f;
		// FogPriority = BasePriority;
	}

	if (!(_pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUME))
	{
		if (m_RenderFlags & CTM_RFLAGS_ORDERTRANS)
		{
			BasePriority = _pRenderParams->m_PriorityOverride;
			BaseOffset = 0.0f;
		}

		for(int iSurf = m_bDefaultSurface ? 0 : 1 ; (iSurf < TRIMESH_NUMSURF) && _pRenderParams->m_lpSurfaces[iSurf]; iSurf++)
		{
			CXW_Surface* pSurf = _pRenderParams->m_lpSurfaces[iSurf];
			CXW_SurfaceKeyFrame* pSurfKey = _pRenderParams->m_lpSurfaceKeyFrames[iSurf];

			if(!(m_RenderFlags & CTM_RFLAGS_NOTRIANGLES) && !(pSurf->m_Flags & XW_SURFFLAGS_SHADERONLY))
			{
				CXR_RenderSurfExtParam Params;
				Params.m_pLights = _pRenderParams->m_pRenderLights;
				Params.m_nLights = _pRenderParams->m_nRenderLights;
				Params.m_pLightFieldAxes = _pRenderParams->m_lLightFieldAxes;
				Params.m_lUserColors[0] = _pRenderParams->m_pCurAnimState->m_Data[0];
				Params.m_lUserColors[1] = _pRenderParams->m_pCurAnimState->m_Data[1];

				int Flags = RENDERSURFACE_FULLBRIGHT | RENDERSURFACE_NOSHADERLAYERS | RENDERSURFACE_MATRIXSTATIC_M2V | RENDERSURFACE_MODULATELIGHT; // RENDERSURFACE_VERTEXFOG | 
				if (!bDrawShading)
					Flags = RENDERSURFACE_DEPTHFOG;

				CXR_Util::Render_Surface(Flags, _pRenderParams->m_lSurfaceTime[iSurf], pSurf, pSurfKey, _pRenderParams->m_pCurrentEngine, pVBM, &_pRenderParams->m_CurVertex2WorldMat, NULL, _pRenderParams->m_RenderVB.m_pTransform, &_pRenderParams->m_RenderVB, 
					BasePriority, BaseOffset, &Params);

				_pRenderParams->m_PrioCoverageMin = MinMT(_pRenderParams->m_PrioCoverageMin, BasePriority + BaseOffset);
				_pRenderParams->m_PrioCoverageMax = MaxMT(_pRenderParams->m_PrioCoverageMax, BasePriority + BaseOffset);
			}

			BasePriority += 0.01f;
		}
	}


	if (m_RenderFlags & CTM_RFLAGS_WIRE)
	{
		/*
		// Old version.
		// If animated wireframes becomes unhip, use this code to create
		// static wireframes for that slick, retro feel! -ae

		CXR_VertexBuffer VB;
		if (!VB.SetVBChain(pVBM, false))
		return;
		VB.Render_IndexedWires(pTVB->GetEdges()->m_iV, pTVB->GetNumEdges()*2);
		int nV = pTVB->GetNumVertices();
		if(!_pRenderParams->m_pRenderV)
		{
		_pRenderParams->m_pRenderV = pVBM->Alloc_V3(nV);
		memcpy(_pRenderParams->m_pRenderV, pTVB->GetVertexPtr(0), nV*sizeof(CVec3Dfp32));
		}
		VB.Geometry_VertexArray(_pRenderParams->m_pRenderV, nV, true);
		VB.Geometry_ColorArray(pDiffuse);
		CRC_Attributes* pA = pVBM->Alloc_Attrib();
		if (!pA) return;
		pA->SetDefault();
		pA->m_iTexCoordSet[1] = 0; pA->m_iTexCoordSet[2] = 1; pA->m_iTexCoordSet[3] = 2;
		VB.m_pAttrib = pA;
		VB.Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
		VB.m_pMatrixPaletteArgs = pMatrixPaletteArgs;
		pVBM->AddVB(VB);
		//*/

		CXR_VertexBuffer VB = _pRenderParams->m_RenderVB;
		if( !pVBM->Alloc_VBChainCopy(&VB,&_pRenderParams->m_RenderVB) )
			return;

		int nV = pTVB->GetNumVertices(this);
		CTM_Edge* pEdges = pTVB->GetEdges(this);
		int nEdges = pTVB->GetNumEdges(this);
		if(_pC->m_nEdges)
		{
			pEdges += _pC->m_iEdges;
			nEdges = _pC->m_nEdges;
		}
		VB.Render_IndexedWires(pEdges->m_iV, nEdges * 2);
		CRC_Attributes* pA = pVBM->Alloc_Attrib();
		if (!pA) return;
		pA->SetDefault();
		pA->m_iTexCoordSet[1] = 0; pA->m_iTexCoordSet[2] = 1; pA->m_iTexCoordSet[3] = 2;
		VB.m_pAttrib = pA;
		VB.Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
		VB.m_pMatrixPaletteArgs = _pRenderParams->m_RenderVB.m_pMatrixPaletteArgs;
		pVBM->AddVB(VB);
	}
}

void CXR_Model_TriangleMesh::VB_RenderUnified(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_Cluster *_pC, int _iCluster, bool &_bDrawShading)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_RenderVB, MAUTOSTRIP_VOID);
#ifndef	PLATFORM_PS2

	CXR_VBManager* pVBM = _pRenderParams->m_pVBM;

	const CRC_MatrixPalette *pMatrixPaletteArgs = _pRenderParams->m_RenderVB.m_pMatrixPaletteArgs;
//	CMat4Dfp32* pMatrixPalette = _pRenderParams->m_RenderVB.m_pMatrixPalette;
//	int nMatrixPalette = _pRenderParams->m_RenderVB.m_nMatrixPalette;



	int bOnlyShadows = (_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(18));


	_bDrawShading = ((_pRenderParams->m_lpSurfaces[0]->m_Flags & XW_SURFFLAGS_SHADER) > 0);
	bool bDrawSolid = !(m_RenderFlags & CTM_RFLAGS_NOTRIANGLES) && !bOnlyShadows;

	CPixel32* pDiffuse = NULL;
//	CPixel32* pSpecular = NULL;
//	fp32 FogPriority = 0; 

	// Opt this: -mh
	CXR_Shader* pShader = _pRenderParams->m_pCurrentEngine->GetShader();

	CXR_SurfaceShaderParams SSP;
	CXR_ShaderParams Params;
	if (_bDrawShading && bDrawSolid)
	{
		SSP.Create(_pRenderParams->m_lpSurfaces[0], _pRenderParams->m_lpSurfaceKeyFrames[0]);
		Params.Create(&_pRenderParams->m_CurVertex2WorldMat, _pRenderParams->m_pVMat, pShader);
		Params.m_pCurrentWVelMat = _pRenderParams->m_pCurWVelMat;
	}


	CRC_Viewport* pVP = pVBM->Viewport_Get();
	fp32 ViewportFrontPlane = pVP->GetFrontPlane();
	fp32 ViewportBackPlane = pVP->GetBackPlane();
	CRct ViewRect = pVP->GetViewRect();
	int ScrW = ViewRect.GetWidth();
	int ScrH = ViewRect.GetHeight();
	CVec2Dfp32 Mid;
	Mid[0] = (ViewRect.p0.x + ViewRect.p1.x) >> 1;
	Mid[1] = (ViewRect.p0.y + ViewRect.p1.y) >> 1;

	CXR_LightOcclusionInfo* pLO = _pRenderParams->m_pLO;
	int nMaxLights = _pRenderParams->m_nLOMaxLights;
	
//	ScrW = 520;
//	ScrH = 300;

//	CXR_Light* pL = m_CurrentWLS.GetFirst();
//	for(; pL; pL = pL->m_pNext)

	uint8 nLit = 0;

	for(int iL = 0; iL < _pRenderParams->m_RenderInfo.m_nLights; iL++)
	{
		const CXR_LightInfo& LightInfo = _pRenderParams->m_RenderInfo.m_pLightInfo[iL];
		const CXR_Light* pL = LightInfo.m_pLight;

		if (pL->m_LightGUID == _pRenderParams->m_pCurAnimState->m_ExcludeLightGUID && pL->m_LightGUID)		//  && pL->m_LightGUID is a safety fix in case a light doesn't have a proper GUID
			continue;

		if (pL->m_Type != CXR_LIGHTTYPE_POINT && pL->m_Type != CXR_LIGHTTYPE_SPOT)
		{
			continue;
		}
		if (_pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUME)
		{
			if (!(_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(13)) &&
				(pL->m_LightGUID != _pRenderParams->m_pCurAnimState->m_NoShadowLightGUID || !pL->m_LightGUID) &&		//  || !pL->m_LightGUID is a safety fix in case a light doesn't have a proper GUID
				!(pL->m_Flags & CXR_LIGHT_NOSHADOWS))
			{
				// Shadow visible at all?
				if (!LightInfo.m_ShadowScissor.IsValid())
					continue;

				// Check if light was culled
				if(_pRenderParams->m_plLightCulled[iL])
					continue;

				CXR_VertexBuffer VB;
				VB = _pRenderParams->m_RenderVB;
				if (!pVBM->Alloc_VBChainCopy(&VB, &_pRenderParams->m_RenderVB))
					continue;

				if (_pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE && _pRenderParams->m_pppShadowPrimData)
				{
					uint TaskId = _pRenderParams->m_pVpuShadowTaskId;
					if(VB.IsVBIDChain())
					{
						CXR_VBIDChain* pVBChain = VB.GetVBIDChain();
						pVBChain->Render_IndexedTriangles((uint16 *)(((mint *)_pRenderParams->m_pppShadowPrimData[_iCluster][iL])), 0xffff);
						pVBChain->m_TaskId = TaskId;

						pVBChain = pVBChain->m_pNextVB;
						uint iC = 0;
						uint nChains = _pRenderParams->m_nClusters;
						while(nChains > 0 && pVBChain)
						{
							uint iCluster = _pRenderParams->m_iCluster[iC++];
							pVBChain->Render_IndexedTriangles((uint16 *)(((mint *)_pRenderParams->m_pppShadowPrimData[iCluster][iL])), 0xffff);
							pVBChain->m_TaskId = TaskId;

							pVBChain = pVBChain->m_pNextVB;
							nChains--;
						}

#ifndef M_RTM
						if(nChains != 0 || pVBChain)
						{
							ConOutL("Something is wrong with the trimesh chains!!!!");
						}
#endif
					}
					else
					{
						CXR_VBChain* pVBChain = VB.GetVBChain();
						pVBChain->Render_IndexedTriangles((uint16 *)(((mint *)_pRenderParams->m_pppShadowPrimData[_iCluster][iL])), 0xffff);
						pVBChain->m_TaskId = TaskId;

						pVBChain = pVBChain->m_pNextVB;
						uint iC = 0;
						uint nChains = _pRenderParams->m_nClusters;
						while(nChains > 0 && pVBChain)
						{
							uint iCluster = _pRenderParams->m_iCluster[iC++];
							pVBChain->Render_IndexedTriangles((uint16 *)(((mint *)_pRenderParams->m_pppShadowPrimData[iCluster][iL])), 0xffff);
							pVBChain->m_TaskId = TaskId;

							pVBChain = pVBChain->m_pNextVB;
							nChains--;
						}
#ifndef M_RTM
						if(nChains != 0 || pVBChain)
						{
							ConOutL("Something is wrong with the trimesh chains!!!!");
						}
#endif
					}
				}

				if(!_pRenderParams->m_plLightParams)
					continue;

				CVec4Dfp32* pLightParams = &_pRenderParams->m_plLightParams[iL];

				CRC_Attributes* pA = _pRenderParams->m_lpShadowLightAttributes[iL];
				if(_pC->m_Flags & (CTM_CLUSTERFLAGS_DUALSIDED | CTM_CLUSTERFLAGS_BACKLIGHTSELFSHADOW))
				{
					CRC_Attributes* pNewA = pVBM->Alloc_Attrib();
					if (!pNewA)
						return;
					*pNewA = *pA;
					pA = pNewA;
					pA->Attrib_ZCompare(CRC_COMPARE_GREATEREQUAL);
				}
				VB.m_pAttrib = pA;
				VB.m_Flags |= CXR_VBFLAGS_LIGHTSCISSOR;
				VB.m_iLight = pL->m_iLight;

				VB.Geometry_Color(0xff100010);
				VB.m_Priority = pL->m_iLight + 0.0001f;

		//		VB.Render_IndexedPrimitives(_pTVB->m_lPrimitives.GetBasePtr(), _pTVB->m_lPrimitives.Len());
				VB.Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
				VB.m_pMatrixPaletteArgs = pMatrixPaletteArgs;
				VB.SetVBEColor(0xff202040);
				pVBM->AddVB(VB);

				if( !( _pRenderParams->m_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_SEPARATESTENCIL ) )
				{
					CRC_Attributes* pA2 = _pRenderParams->m_lpShadowLightAttributesBackside[iL];
					VB.m_pAttrib = pA2;
					pVBM->AddVB(VB);
				}
			}
		}
		else if (_bDrawShading && bDrawSolid)
		{
			// -------------------------------------------------------------------
			// Render shading
			CXR_VertexBuffer VB;
			VB = _pRenderParams->m_RenderVB;
			VB.Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
			VB.m_pMatrixPaletteArgs = pMatrixPaletteArgs;

			if (pL->m_Flags & CXR_LIGHT_NOSHADOWS)
				Params.m_Flags |= XR_SHADERFLAGS_NOSTENCILTEST;
			else
				Params.m_Flags &= ~XR_SHADERFLAGS_NOSTENCILTEST;

			CXR_Light Light;
			Light = *pL;
			Light.Transform(_pRenderParams->m_CurWorld2VertexMat);

			if(!(_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(19)))
			{
				const CXR_SurfaceShaderParams* lpSSP[1] = { &SSP };
				pShader->RenderShading(Light, &VB, &Params, lpSSP);
				nLit++;
			}

			const uint16 iLight = pL->m_iLight;
			if (iLight < nMaxLights)
			{
				if (!_pRenderParams->m_LOUsage.Get(iL))
				{
					_pRenderParams->m_LOUsage.Set1(iL);
					_pRenderParams->m_piSrcLO[iL] = _pRenderParams->m_nLO++;
					_pRenderParams->m_piDstLO[iL] = iLight;
				}
				const uint16 index = _pRenderParams->m_piSrcLO[iL];
				pLO[index].m_ScissorShaded.Expand(_pRenderParams->m_RenderBoundScissor);
			}
//			if (pLO && pL->m_iLight < nMaxLights)
//				pLO[pL->m_iLight].m_ScissorShaded.Expand(_pRenderParams->m_RenderBoundScissor);
		
//			pShader->RenderShading(*pL, &VB, &Params);
		}

	}

#ifdef M_Profile
	// -------------------------------------------------------------------
	// Light colorkey debug rendering
	if( (nLit > 0) &&
		(_pRenderParams->m_pCurrentEngine->m_DebugFlags & XR_DEBUGFLAGS_SHOWLIGHTCOUNT) )
	{

		//These picked from CBSP2Model.cpp
		// Might want to add the remaining 22 colors and skip the redundancy -ae
		const CPixel32 lColors[CTM_MAX_LIGHTS] = {
			CPixel32(0x00,0x00,0xFF,0x80),CPixel32(0x00,0x80,0x80,0x80),
			CPixel32(0x00,0xFF,0x00,0x80),CPixel32(0x80,0xFF,0x00,0x80),
			CPixel32(0xFF,0xFF,0x00,0x80),CPixel32(0xFF,0x80,0x00,0x80),
			CPixel32(0xFF,0x00,0x00,0x80),CPixel32(0xFF,0x00,0xFF,0x80),
			CPixel32(0x80,0x80,0x80,0x80),CPixel32(0xFF,0xFF,0xFF,0x80)
		};

		CXR_VertexBuffer VB;
		VB = _pRenderParams->m_RenderVB;
		VB.Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
		VB.m_pMatrixPaletteArgs = pMatrixPaletteArgs;
		VB.m_Color = lColors[nLit-1];

		CRC_Attributes* pA = pVBM->Alloc_Attrib();
		if (!pA)
			return;

		pA->SetDefault();
		pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		pA->Attrib_Enable(CRC_TEXENVMODE_BLEND);
		pA->Attrib_SourceBlend(CRC_BLEND_SRCALPHA);
		pA->Attrib_DestBlend(CRC_BLEND_INVSRCALPHA);
		pA->Attrib_AlphaCompare(CRC_COMPARE_ALWAYS,0);
		VB.m_pAttrib = pA;
		VB.m_Priority = CXR_VBPRIORITY_MODEL_TRANSPARENT + 1.0f;
		
		pVBM->AddVB(VB);
	}
#endif


	// -------------------------------------------------------------------
	// Render light field shading
	if (_pRenderParams->m_RenderInfo.m_pLightVolume && !(_pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUME) && _bDrawShading && bDrawSolid)
	{
		CXR_VertexBuffer VB;
		VB = _pRenderParams->m_RenderVB;
		VB.Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
		VB.m_pMatrixPaletteArgs = pMatrixPaletteArgs;

		CXR_ShaderParams_LightField LFParams;
		LFParams.Create(&_pRenderParams->m_CurVertex2WorldMat, _pRenderParams->m_pVMat, pShader);
		memcpy(&LFParams.m_lLFAxes, &_pRenderParams->m_lLightFieldAxes, sizeof(LFParams.m_lLFAxes));
		const CXR_SurfaceShaderParams* lpSSP[1] = { &SSP };
		pShader->RenderShading_LightField(&VB, &LFParams, lpSSP);
	}

	// -------------------------------------------------------------------
	// Render geometry into z-buffer
	if (!(_pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUME) && _bDrawShading && bDrawSolid)
	{
		{
			CXR_VertexBuffer VB;
			VB = _pRenderParams->m_RenderVB;
			
			VB.m_pAttrib = (_pRenderParams->m_bIsMirrored) ? &ms_RenderZBufferCCW : &ms_RenderZBuffer;

			const CXW_SurfaceLayer* pLayer = &_pRenderParams->m_lpSurfaceKeyFrames[0]->m_lTextures[0];

			if (pLayer->m_Flags & XW_LAYERFLAGS_ALPHACOMPARE)
			{
				CRC_Attributes* pA = pVBM->Alloc_Attrib();
				if (!pA) return;
				*pA = *VB.m_pAttrib;
				VB.m_pAttrib = pA;

				pA->Attrib_TextureID(0, pLayer->m_TextureID);
				pA->Attrib_AlphaCompare(pLayer->m_AlphaFunc, pLayer->m_AlphaRef);
			}

			VB.Geometry_Color(_pRenderParams->m_pCurrentEngine->GetVar(XR_ENGINE_UNIFIED_AMBIENCE));
			VB.m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER;

			VB.Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
			VB.m_pMatrixPaletteArgs = pMatrixPaletteArgs;
			VB.SetVBEColor(0xff403030);
			pVBM->AddVB(VB);
		}

		// Render components to deferred rendering buffers
		if (pShader->m_ShaderModeTraits & XR_SHADERMODETRAIT_DEFERRED)
		{
			{
				/*
				static CRC_ExtAttributes_FragmentProgram20 FPDeferredNormal;
				FPDeferredNormal.Clear();
				FPDeferredNormal.SetProgram("XRShader_DeferredNormal", MHASH6('XRSh','ader','_Def','erre','dNor','mal'));

				int VBSpace = sizeof(CXR_VertexBuffer) * 3 + sizeof(CRC_Attributes) * 3;
				uint8* pVBSpace = (uint8*)pVBM->Alloc(VBSpace);
				if (!pVBSpace)
					return;

				CXR_VertexBuffer* pVBNorm = (CXR_VertexBuffer*) pVBSpace; pVBSpace += sizeof(CXR_VertexBuffer);
				CXR_VertexBuffer* pVBDiff = (CXR_VertexBuffer*) pVBSpace; pVBSpace += sizeof(CXR_VertexBuffer);
				CXR_VertexBuffer* pVBSpec = (CXR_VertexBuffer*) pVBSpace; pVBSpace += sizeof(CXR_VertexBuffer);
				CRC_Attributes* pANorm = (CRC_Attributes*) pVBSpace; pVBSpace += sizeof(CRC_Attributes);
				CRC_Attributes* pADiff = (CRC_Attributes*) pVBSpace; pVBSpace += sizeof(CRC_Attributes);
				CRC_Attributes* pASpec = (CRC_Attributes*) pVBSpace; pVBSpace += sizeof(CRC_Attributes);
				*pVBNorm = _pRenderParams->m_RenderVB;
				*pVBDiff = _pRenderParams->m_RenderVB;
				*pVBSpec = _pRenderParams->m_RenderVB;
				pANorm->SetDefault();
				pADiff->SetDefault();
				pASpec->SetDefault();
				
				pANorm->m_pExtAttrib = &FPDeferredNormal;
				pANorm->Attrib_TextureID(0, Params.m_lTextureIDs[XR_SHADERMAP_NORMAL]);
				pADiff->Attrib_TextureID(0, Params.m_lTextureIDs[XR_SHADERMAP_DIFFUSE]);
				pASpec->Attrib_TextureID(0, Params.m_lTextureIDs[XR_SHADERMAP_SPECULAR]);
//				pA->Attrib_AlphaCompare(pLayers->m_AlphaFunc, pLayers->m_AlphaRef);
				pANorm->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL);
				pADiff->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL);
				pASpec->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL);
				int Enable = CRC_FLAGS_CULL;
				if (_pRenderParams->m_bIsMirrored)
					Enable |= CRC_FLAGS_CULLCW;
				pANorm->Attrib_Enable(Enable);
				pADiff->Attrib_Enable(Enable);
				pASpec->Attrib_Enable(Enable);
				pANorm->Attrib_ZCompare(CRC_COMPARE_EQUAL);
				pADiff->Attrib_ZCompare(CRC_COMPARE_EQUAL);
				pASpec->Attrib_ZCompare(CRC_COMPARE_EQUAL);
				pVBNorm->m_pAttrib = pANorm;
				pVBDiff->m_pAttrib = pADiff;
				pVBSpec->m_pAttrib = pASpec;
				pVBNorm->m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER + 8.0f;
				pVBDiff->m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER + 9.0f;
				pVBSpec->m_Priority = CXR_VBPRIORITY_UNIFIED_ZBUFFER + 10.0f;

				pVBM->AddVB(pVBNorm);
				pVBM->AddVB(pVBDiff);
				pVBM->AddVB(pVBSpec);
				//*/
//				CXR_ShaderParamsDef ParamsDef;
//				ParamsDef.Create(*_pRenderParams->m_lpSurfaceKeyFrames[0]);
				int Enable = CRC_FLAGS_CULL |((_pRenderParams->m_bIsMirrored) ? CRC_FLAGS_CULLCW : 0);
				const CXR_SurfaceShaderParams* pSSP = &SSP;
				pShader->RenderDeferredArray(1, &_pRenderParams->m_RenderVB, &Params, &pSSP, Enable,
					CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL,CXR_VBPRIORITY_UNIFIED_ZBUFFER+8.0f,1.0f);
				//*/
			}
		}

		CXR_FogState* pFogState = _pRenderParams->m_pCurrentEngine->m_pCurrentFogState;

		if (pFogState && pFogState->DepthFogEnable())
		{
			// -------------------------------------------------------------------
			// Depth fog

			CXR_VertexBuffer VB;
			VB = _pRenderParams->m_RenderVB;

			fp32 FogEnd = pFogState->m_DepthFogEnd;
			fp32 FogStart = pFogState->m_DepthFogStart;

			CMat4Dfp32 View2Vertex;
			if (_pRenderParams->m_RenderVB.m_pTransform)
				_pRenderParams->m_RenderVB.m_pTransform->InverseOrthogonal(View2Vertex);
			else
				View2Vertex.Unit();

			CVec4Dfp32* pTexGenAttr = pVBM->Alloc_V4(2);
			if (!pTexGenAttr)
				return;
			const CVec3Dfp32& VFwd = CVec3Dfp32::GetRow(View2Vertex, 2);
			fp32 k = 1.0f / (FogEnd - FogStart);
			pTexGenAttr[0][0] = VFwd[0] * k;
			pTexGenAttr[0][1] = VFwd[1] * k;
			pTexGenAttr[0][2] = VFwd[2] * k;
			pTexGenAttr[0][3] = -(VFwd * CVec3Dfp32::GetRow(View2Vertex, 3)) * k - (k * FogStart);
			pTexGenAttr[1] = 0;

			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			if (!pA)
				return;
			VB.m_pAttrib = pA;
			pA->SetDefault();
			pA->m_iTexCoordSet[1] = 0; pA->m_iTexCoordSet[2] = 1; pA->m_iTexCoordSet[3] = 2;
			pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL);
			pA->Attrib_Enable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);
			if (_pRenderParams->m_bIsMirrored)
				pA->Attrib_Enable(CRC_FLAGS_CULLCW);
			pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
			pA->Attrib_TextureID(0, pFogState->m_DepthFogTableTextureID);
			pA->Attrib_TexGen(0, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
			pA->Attrib_TexGenAttr((fp32*)pTexGenAttr);
#ifdef XR_DEFAULTPOLYOFFSETSCALE
			pA->Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
#endif

			CPixel32 FogColor = pFogState->m_DepthFogColor | 0xff000000;

			const CXW_SurfaceLayer* pLayer = &_pRenderParams->m_lpSurfaceKeyFrames[0]->m_lTextures[0];
			if (pLayer->m_Flags & XW_LAYERFLAGS_ALPHACOMPARE)
			{
				pA->Attrib_ZCompare(CRC_COMPARE_EQUAL);
//				pA->Attrib_TextureID(0, pLayer->m_TextureID);
//				pA->Attrib_AlphaCompare(pLayer->m_AlphaFunc, pLayer->m_AlphaRef);
			}

			VB.Geometry_Color(FogColor);
			VB.m_Priority = _pRenderParams->m_RenderInfo.m_BasePriority_Opaque + CXR_VBPRIORITY_DEPTHFOG;

			//		VB.Render_IndexedPrimitives(_pTVB->m_lPrimitives.GetBasePtr(), _pTVB->m_lPrimitives.Len());
			VB.Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
			VB.m_pMatrixPaletteArgs = pMatrixPaletteArgs;
			VB.SetVBEColor(0xffe0e0ff);
			pVBM->AddVB(VB);
		}

		if (_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(14))
		{
//			DecompressAll(-1);

			CTM_VertexBuffer* pTVB = GetVertexBuffer(_pC->m_iVB);

			CVec3Dfp32* pV = pTVB->GetVertexPtr(this, 0);
			CVec3Dfp32* pN = pTVB->GetNormalPtr(this, 0);
			CVec3Dfp32* pTgU = pTVB->GetTangentUPtr(this, 0);
			CVec3Dfp32* pTgV = pTVB->GetTangentVPtr(this, 0);

			if (pV && pN && pTgU && pTgV)
			{
				CXR_VertexBuffer* pVB = pVBM->Alloc_VB(CXR_VB_ATTRIB);
				if(pVB)
				{
					*pVB->m_pAttrib	= *_pRenderParams->m_RenderVB.m_pAttrib;
					CMat4Dfp32* pMat = pVBM->Alloc_M4();
					if (!pMat) return;
					*pMat	= *_pRenderParams->m_RenderVB.m_pTransform;
					CXR_VBChain* pChain = pVB->GetVBChain();
					fp32 Len = 1.0f;
					int nV = pTVB->GetNumVertices(this);
					pChain->m_nV		= nV * 6;
					pChain->m_pV		= pVBM->Alloc_V3(nV * 6);
					pChain->m_pCol		= (CPixel32*)pVBM->Alloc(sizeof(CPixel32) * 6);
					pChain->m_piPrim	= (uint16*)pVBM->Alloc(nV * 6);
					if (!pChain->m_pCol || !pChain->m_piPrim)
						return;
					pChain->m_PrimType	= CRC_RIP_WIRES;
					pChain->m_nPrim		= nV * 6;
					pVB->m_pTransform	= pMat;
					for(int v = 0; v < nV; v++)
					{
						int iv = v;
						const CVec3Dfp32& V(pV[iv]);
						const CVec3Dfp32& N(pN[iv]);
						const CVec3Dfp32& TgU(pTgU[iv]);
						const CVec3Dfp32& TgV(pTgV[iv]);
						pChain->m_pV[v*6+0]	= V;
						pChain->m_pV[v*6+1]	= V + TgU * Len;
						pChain->m_pV[v*6+2]	= V;
						pChain->m_pV[v*6+3]	= V + TgV * Len;
						pChain->m_pV[v*6+4]	= V;
						pChain->m_pV[v*6+5]	= V + N * Len;
						pChain->m_pCol[v*6+0]	= 0xffff0000;
						pChain->m_pCol[v*6+1]	= 0xffff0000;
						pChain->m_pCol[v*6+2]	= 0xff00ff00;
						pChain->m_pCol[v*6+3]	= 0xff00ff00;
						pChain->m_pCol[v*6+4]	= 0xff0000ff;
						pChain->m_pCol[v*6+5]	= 0xff0000ff;
					}
				}
			}
		}
	}

	//BasePriority stuff here... In case fog is again needed!

#ifdef NEVER
	if (_pRenderParams->m_pSceneFog && _pRenderParams->m_RenderInfo.m_Flags & CXR_RENDERINFO_NHF)
	{
		CRC_Attributes* pA = pVBM->Alloc_Attrib();
		if (!pA) return;
		_pRenderParams->m_RenderVB.m_pAttrib = pA;
		pA->SetDefault();
		if(_pRenderParams->m_pCurrentEngine) _pRenderParams->m_pCurrentEngine->SetDefaultAttrib(pA);
		pA->m_iTexCoordSet[1] = 0; pA->m_iTexCoordSet[2] = 1; pA->m_iTexCoordSet[3] = 2;
		pA->Attrib_Enable(CRC_FLAGS_CULL);
		pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
		pA->Attrib_TextureID(0, _pRenderParams->m_pSceneFog->m_FogTableTextureID);
		pA->Attrib_ZCompare(CRC_COMPARE_EQUAL);
		pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
		CPixel32* pFog = pVBM->Alloc_CPixel32(nV);
		CVec2Dfp32* pTV = pVBM->Alloc_V2(nV);
		if (!pFog || !pTV) return;
		_pRenderParams->m_RenderVB.Geometry_ColorArray(pFog);
		_pRenderParams->m_RenderVB.Geometry_Color(0xffffffff);
		_pRenderParams->m_RenderVB.Geometry_TVertexArray(pTV, 0);

		_pRenderParams->m_pSceneFog->InterpolateBox(_pRenderParams->m_RenderFogBox, _pRenderParams->m_RenderFog, nV, _pRenderParams->m_pRenderV, pFog, pTV, &_pRenderParams->m_CurVertex2LocalMat /*&_pRenderParams->m_CurVertex2WorldMat*/);

		{
			_pRenderParams->m_RenderVB.m_Priority = FogPriority + CXR_VBPRIORITY_VOLUMETRICFOG;
			if (_pRenderParams->m_pSceneFog) _pRenderParams->m_pSceneFog->SetDepthFogNone(_pRenderParams->m_RenderVB.m_pAttrib);
			pVBM->AddVB(_pRenderParams->m_RenderVB);
		}
	}
#endif

	if (!_pRenderParams->m_RenderVB.IsVBIDChain())
		_pRenderParams->m_RenderVB.GetVBChain()->m_pN = _pRenderParams->m_pRenderN;

#endif	// PLATFORM_PS2
}


void CXR_Model_TriangleMesh::RenderDecals(CTriMesh_RenderInstanceParamters* _pRenderParams, int _GUID, CMat4Dfp32* _pMP, int _nMP)
{
	MSCOPESHORT(CXR_Model_TriangleMesh::RenderDecals);
	CXR_TriangleMeshDecalContainer* pTMDC = _pRenderParams->m_pCurrentEngine->m_pTMDC;
	if (!pTMDC)
		return;

	int nDecals = 0;
	int nTriangles = 0;

	CXR_VBManager* pVBM = _pRenderParams->m_pVBM;

	uint16* pIndices = pTMDC->GetIndices();
	CMTime RenderTime = _pRenderParams->m_pCurrentEngine->GetEngineTime();

	const CXR_TMDC_Decal* pDecal = pTMDC->GetDecalFirst(_GUID);
	for(; pDecal; pDecal = pTMDC->GetDecalNext(pDecal))
	{
		/*
		//Debug rendering!
		CVec3Dfp32 Delta(-2.0f,-2.0f,0.0f);
		CVec3Dfp32 Delta2(2.0f,-2.0f,0.0f);
		pVBM->RenderWire(_pRenderParams->m_CurL2VMat,pDecal->m_Pos-Delta,pDecal->m_Pos+Delta,CPixel32(255,255,255,255));
		pVBM->RenderWire(_pRenderParams->m_CurL2VMat,pDecal->m_Pos-Delta2,pDecal->m_Pos+Delta2,CPixel32(255,255,255,255));
		pVBM->RenderWire(_pRenderParams->m_CurL2VMat,pDecal->m_Pos,pDecal->m_Pos+pDecal->m_Normal,CPixel32(255,128,128,255));
		//*/

		if (pDecal->m_ModelGUID != (mint)this)
			continue;

		{
			uint32 iC = pDecal->m_iCluster;
			if (iC >= GetNumClusters())
				continue;

			CTM_Cluster* pC = GetCluster(iC);
			if (_pRenderParams->m_pCurAnimState && ((1 << (pC->m_OcclusionIndex)) & _pRenderParams->m_pCurAnimState->m_SurfaceOcclusionMask)) 
				continue;

			CTM_VertexBuffer* pTVB = GetVertexBuffer(pC->m_iVB);

			int ReqSize = sizeof(CXR_VertexBuffer) + sizeof(CRC_MatrixPalette) + sizeof(CRC_Attributes) + sizeof(CXR_VBIDChain) + sizeof(CVec4Dfp32) * 3 + sizeof(uint16) * pDecal->m_nIndices;

			uint8* pVBMem = (uint8*)pVBM->Alloc(ReqSize);
			if(!pVBMem)
				break;

			// Alignment issue for texgenattr.. place it first (since it will retain alignment after alloc)
			CVec4Dfp32* pTexGenAttr = (CVec4Dfp32*)pVBMem; pVBMem += sizeof(CVec4Dfp32) * 3;
			CRC_Attributes* pAttrib = new (pVBMem) CRC_Attributes; pVBMem += sizeof(CRC_Attributes);
			CXR_VertexBuffer* pVB = new (pVBMem) CXR_VertexBuffer; pVBMem += sizeof(CXR_VertexBuffer);
			CRC_MatrixPalette* pMP = new (pVBMem) CRC_MatrixPalette; pVBMem += sizeof(CRC_MatrixPalette);		// This one is only required if there is a MP but since it's only 16 bytes its faster to always allocate it
			CXR_VBIDChain* pVBIDChain = new (pVBMem) CXR_VBIDChain; pVBMem += sizeof(CXR_VBIDChain);
			uint16* piPrim = (uint16*)pVBMem; pVBMem += sizeof(uint16) * pDecal->m_nIndices;
			pVB->Clear();
			pVB->m_pAttrib = pAttrib;
			pAttrib->SetDefault();
			pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
			pVBIDChain->Clear();
			pVB->SetVBIDChain(pVBIDChain);
			pVB->Geometry_VertexBuffer(pTVB->m_VBID, false);
			memcpy(piPrim, &pIndices[pDecal->m_iIndices], pDecal->m_nIndices * sizeof(uint16));

			pVB->Render_IndexedTriangles(piPrim, pDecal->m_nIndices / 3);
			pVB->Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
			if (_nMP)
				Cluster_SetMatrixPalette(_pRenderParams, pC, pTVB, _pMP, _nMP, pVB, InvalidVpuTask, pMP);

			if ((_pMP && (pDecal->m_iBone < _nMP)) || (_nMP == 0))
			{
				CVec3Dfp32 TexU(pDecal->m_TexU);
				CVec3Dfp32 TexV(pDecal->m_TexV);
				CVec3Dfp32 TexPos(pDecal->m_Pos);
				CVec3Dfp32 Norm(pDecal->m_Normal);

				if (_pMP)
				{
					TexU.MultiplyMatrix3x3(_pMP[pDecal->m_iBone]);
					TexV.MultiplyMatrix3x3(_pMP[pDecal->m_iBone]);
					Norm.MultiplyMatrix3x3(_pMP[pDecal->m_iBone]);
					TexPos *= _pMP[pDecal->m_iBone];
				}

				pTexGenAttr[0][0] = TexU[0];
				pTexGenAttr[0][1] = TexU[1];
				pTexGenAttr[0][2] = TexU[2];
				pTexGenAttr[0][3] = -(TexU*TexPos) + 0.5f;
				pTexGenAttr[1][0] = TexV[0];
				pTexGenAttr[1][1] = TexV[1];
				pTexGenAttr[1][2] = TexV[2];
				pTexGenAttr[1][3] = -(TexV*TexPos) + 0.5f;

				//Might want to use something other than constants here...
				Norm *= 0.15f;
				pTexGenAttr[2] = Norm;
				pTexGenAttr[2].k[3] = Norm * TexPos;
			}

			{
				CMTime Time = RenderTime - pDecal->m_SpawnTime;

				CXW_Surface* pSurf = _pRenderParams->m_pCurrentEngine->m_pSC->GetSurfaceVersion(pDecal->m_SurfaceID, _pRenderParams->m_pCurrentEngine);
				//				CXW_SurfaceKeyFrame* pSurfKey = pSurf->GetFrame(0, CMTime_zero(), _pRenderParams->m_lTmpSurfaceKeyFrames[0]);
				//pSurf = _pRenderParams->m_pCurrentEngine->m_pSC->GetSurface(pDecal->m_SurfaceID)->GetSurface(_pRenderParams->m_pCurrentEngine->m_SurfOptions, _pRenderParams->m_pCurrentEngine->m_SurfCaps);
				CXW_SurfaceKeyFrame* pSurfKey = pSurf->GetFrame(0, Time, _pRenderParams->CreateTempSurfKeyFrame(0));

				uint8 DummyIndex = (mint(pDecal)>>6) & 255;
				fp32 Prio = _pRenderParams->m_RenderInfo.m_BasePriority_Opaque + (fp32)DummyIndex * 0.0001f;//_pRenderParams->m_RenderInfo.m_BasePriority_Opaque + pSurf->m_PriorityOffset + (DummyIndex * 0.0005f);

				if( pSurf->m_Flags & XW_SURFFLAGS_SHADER )
				{
					CXR_SurfaceShaderParams SSP;
					CXR_Shader* pShader = _pRenderParams->m_pCurrentEngine->GetShader();
					SSP.Create(pSurf, pSurfKey);
					const CXR_SurfaceShaderParams* pSSP = &SSP;

					CXR_ShaderParams Params;
					Params.Create(&_pRenderParams->m_CurVertex2WorldMat, _pRenderParams->m_pVMat, pShader);
					Params.m_pCurrentWVelMat = _pRenderParams->m_pCurWVelMat;

					pShader->RenderDeferredArray(1, pVB, &Params, &pSSP, CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE,CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL, Prio - 2059.9f, 1.0f, 2, pTexGenAttr);
				}

				if( !(pSurf->m_Flags & XW_SURFFLAGS_SHADERONLY) )
				{
					CXR_RenderSurfExtParam Params;
					Params.m_lUserColors[0] = _pRenderParams->m_pCurAnimState->m_Data[0];
					Params.m_lUserColors[1] = _pRenderParams->m_pCurAnimState->m_Data[1];

					int Flags = RENDERSURFACE_FULLBRIGHT | RENDERSURFACE_NOSHADERLAYERS | RENDERSURFACE_MATRIXSTATIC_M2V;

					//pVB->m_Priority = CXR_VBPRIORITY_WALLMARK;

					CXR_VertexBuffer *VBs[128];
					CXR_VBManager::CVBAddCollector VBAddCollector(VBs, 128);
					{
						pVBM->SetVBAddCollector(&VBAddCollector);
						CXR_Util::Render_Surface(Flags, Time, pSurf, pSurfKey, _pRenderParams->m_pCurrentEngine, pVBM, &_pRenderParams->m_CurVertex2WorldMat, 
							NULL, _pRenderParams->m_RenderVB.m_pTransform, pVB, Prio, 0.0001f, &Params);
						pVBM->SetVBAddCollector(NULL);
					}
					
					// Override tex gen on channel 0 for all buffers that were added.
					for(int iVB = 0; iVB != VBAddCollector.m_iIndex; ++iVB)
					{
						CXR_VertexBuffer *pVB = VBAddCollector.m_pVBs[iVB];
						CRC_Attributes* pA = pVB->m_pAttrib;
						if (!pA)
							continue;
						pA->Attrib_TexGen(0, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
						pA->Attrib_TexGenAttr((fp32*)pTexGenAttr);

						pA->Attrib_TexGen(1, CRC_TEXGENMODE_MSPOS, CRC_TEXGENCOMP_ALL);
						pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL);
						{

							// Hack
							// Replaced the strcmp with a hash compare, but it still needs optimizing
							if (pA->m_pExtAttrib && pA->m_pExtAttrib->m_AttribType == CRC_ATTRIBTYPE_FP20 && 
								(((CRC_ExtAttributes_FragmentProgram20 *)pA->m_pExtAttrib)->m_ProgramNameHash == 
								MHASH8('XRSh','ader','_Dec','alNo','rmal','Tran','sfor','m')))
							{
								pA->Attrib_TexGen(2, CRC_TEXGENMODE_DECALTSTRANSFORM, CRC_TEXGENCOMP_ALL);
								pA->Attrib_TexGen(3, CRC_TEXGENMODE_VOID, CRC_TEXGENCOMP_ALL);
								pA->Attrib_TexCoordSet(2, 1);
								pA->Attrib_TexCoordSet(3, 2);
								CRC_ExtAttributes_FragmentProgram20 * pFP = (CRC_ExtAttributes_FragmentProgram20*)pA->m_pExtAttrib;
								pFP->SetProgram("XRShader_DecalNormalTransformTM",MHASH8('XRSh','ader','_Dec','alNo','rmal','Tran','sfor','mTM'));
								pFP->SetParameters(pTexGenAttr + 2,1);
							}

							//Can only use this shader on SRCALPHA - INVSRCALPHA
							else if((pA->m_SourceBlend == CRC_BLEND_SRCALPHA) && (pA->m_DestBlend == CRC_BLEND_INVSRCALPHA))
							{
								CRC_ExtAttributes_FragmentProgram20 * pFP = (CRC_ExtAttributes_FragmentProgram20*)pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
								pFP->Clear();
								pFP->SetProgram("XRShader_DecalTM",MHASH4('XRSh','ader','_Dec','alTM'));
								pFP->SetParameters(pTexGenAttr + 2,1);
								pA->m_pExtAttrib = pFP;
							}
						}
					}
				}
			}


/*			CRC_Attributes* pA = pVBM->Alloc_Attrib();
			pA->SetDefault();
			pA->Attrib_TextureID(0, _pRenderParams->m_pCurrentEngine->m_pTC->GetTextureID("bloodfloor03_03"));

			pA->Attrib_TexGen(0, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
			pA->Attrib_TexGenAttr((fp32*)pTexGenAttr);
			pA->Attrib_RasterMode(CRC_RASTERMODE_MULTIPLY2);

			pVB->m_pAttrib = pA;
			pVB->m_Priority = CXR_VBPRIORITY_MODEL_TRANSPARENT+1000;

			pVBM->AddVB(pVB);*/
		}

		nTriangles += pDecal->m_nIndices;
		nDecals++;
	}

//	if (nDecals)
//		ConOut(CStrF("%d decals on trimesh, guid %.4x, %d triangles", nDecals, _GUID, nTriangles / 3));
}

void CXR_Model_TriangleMesh::Cluster_RenderSingleColor(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_Cluster* _pC, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_RenderSingleColor, MAUTOSTRIP_VOID);
	CXR_VertexBuffer VB;	
	CXR_VBManager* pVBM = _pRenderParams->m_pVBM;
	CTM_VertexBuffer* pTVB = GetVertexBuffer(_pC->m_iVB);
	int nV = 0;
	if (_pRenderParams->m_bRenderTempTLEnable)
	{
		if (_Color != 0xffffffff)
			ConOut(CStr("§cf80WARNING: (CXR_Model_TriangleMesh::Cluster_RenderSingleColor) HW Color != 0xffffffff"));

		if (!VB.AllocVBChain(pVBM, true))
			return;
		if(m_bUsePrimitives || (_pC->m_nIBPrim == 0))
			VB.Render_VertexBuffer(pTVB->m_VBID);
		else
		{
			CTM_VertexBuffer* pTIB = GetVertexBuffer(_pC->m_iIB);
			VB.Render_VertexBuffer_IndexBufferTriangles(pTVB->m_VBID, pTIB->m_VBID, _pC->m_nIBPrim / 3, _pC->m_iIBOffset);
		}
		VB.m_pMatrixPaletteArgs = _pRenderParams->m_RenderVB.m_pMatrixPaletteArgs;
	}
	else
	{
		nV = pTVB->GetNumVertices(this);
		if (!VB.AllocVBChain(pVBM, false))
			return;
		VB.Geometry_VertexArray(_pRenderParams->m_pRenderV, nV, true);
		VB.Geometry_TVertexArray(_pRenderParams->m_pRenderTV, 0);
		VB.Geometry_Color(_Color);
		if (m_bUsePrimitives)
			VB.Render_IndexedPrimitives(pTVB->GetPrimitives(this), pTVB->GetNumPrimitives(this));
		else if(_pC->m_nIBPrim != 0)
		{
			CTM_VertexBuffer* pTIB = GetVertexBuffer(_pC->m_iIB);
			VB.Render_IndexedTriangles(pTIB->GetTriangles(this) + _pC->m_iIBOffset, _pC->m_nIBPrim);
		}
		else
			VB.Render_IndexedTriangles(pTVB->GetTriangles(this), pTVB->GetNumTriangles(this));
	}

	CRC_Attributes* pA = pVBM->Alloc_Attrib();
	if (!pA) return;
	pA->SetDefault();
	if(_pRenderParams->m_pCurrentEngine) _pRenderParams->m_pCurrentEngine->SetDefaultAttrib(pA);
	pA->Attrib_TextureID(0, _pRenderParams->m_pCurrentEngine->GetTC()->GetTextureID("SPECIAL_FFFFFF"));
	VB.m_pAttrib = pA;
	pA->m_iTexCoordSet[1] = 0; pA->m_iTexCoordSet[2] = 1; pA->m_iTexCoordSet[3] = 2;
	VB.Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);

	pVBM->AddVB(VB);
}

void CXR_Model_TriangleMesh::Cluster_Transform_V(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_VertexBuffer* _pTVB, const CMat4Dfp32* _pMat, const CMTime& _Time)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_Transform_V_N, MAUTOSTRIP_VOID);
	int nV = _pTVB->GetNumVertices(this);

	CVec3Dfp32* pVV = _pRenderParams->m_pRenderV;

	// Added by Mondelore {START}.
	fp32 TimeFrac;
	int iFrm0, iFrm1;

	_pTVB->GetFrameAndTimeFraction(_Time, iFrm0, iFrm1, TimeFrac); // Added by Mondelore.

	// Added by Mondelore {END}.

	const CVec3Dfp32* pV1 = _pTVB->GetVertexPtr(this, iFrm0);
	if (!pV1) return;

	const CVec3Dfp32* pV2 = _pTVB->GetVertexPtr(this, iFrm1);

	// Is frame1 == frame2 we don't want to interpolate
	if (pV2 == pV1) pV2 = NULL;

	// If time is close enough to the first frame we don't want to interpolate
	if (TimeFrac < 0.001f) pV2 = NULL;

	// If interpolation is disabled, well - don't interpolate..
	if (m_RenderFlags & CTM_RFLAGS_NOVERTEXANIMIP) 
		pV2 = NULL;

	if (pV2)
	{
		for(int v = 0; v < nV; v++)
		{
			fp32 x,y,z;
			x = pV1[v].k[0] + fp32(pV2[v].k[0] - pV1[v].k[0])*TimeFrac;
			y = pV1[v].k[1] + fp32(pV2[v].k[1] - pV1[v].k[1])*TimeFrac;
			z = pV1[v].k[2] + fp32(pV2[v].k[2] - pV1[v].k[2])*TimeFrac;

			pVV[v].k[0] = _pMat->k[0][0]*x + _pMat->k[1][0]*y + _pMat->k[2][0]*z + _pMat->k[3][0];
			pVV[v].k[1] = _pMat->k[0][1]*x + _pMat->k[1][1]*y + _pMat->k[2][1]*z + _pMat->k[3][1];
			pVV[v].k[2] = _pMat->k[0][2]*x + _pMat->k[1][2]*y + _pMat->k[2][2]*z + _pMat->k[3][2];
		}
	}
	else
	{
		for(int v = 0; v < nV; v++)
		{
			fp32 x,y,z;
			x = pV1[v].k[0]; y = pV1[v].k[1]; z = pV1[v].k[2];
			pVV[v].k[0] = _pMat->k[0][0]*x + _pMat->k[1][0]*y + _pMat->k[2][0]*z + _pMat->k[3][0];
			pVV[v].k[1] = _pMat->k[0][1]*x + _pMat->k[1][1]*y + _pMat->k[2][1]*z + _pMat->k[3][1];
			pVV[v].k[2] = _pMat->k[0][2]*x + _pMat->k[1][2]*y + _pMat->k[2][2]*z + _pMat->k[3][2];
		}
	}
}

void CXR_Model_TriangleMesh::Cluster_Transform_V_N(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_VertexBuffer* _pTVB, const CMat4Dfp32* _pMat, const CMat4Dfp32* _pNormMat, const CMTime& _Time)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_Transform_V_N, MAUTOSTRIP_VOID);
	int nV = _pTVB->GetNumVertices(this);
	int nTV = nV;

//	ConOut(CStrF("Transform! %d", nV));

	CVec3Dfp32* pVV = _pRenderParams->m_pRenderV;
	CVec2Dfp32* pVT = _pRenderParams->m_pRenderTV; // Added by Mondelore.
	CVec3Dfp32* pVN = _pRenderParams->m_pRenderN;

//	CVec3Dfp32* pVV = m_lVVertices.GetBasePtr();
//	CVec3Dfp32* pVN = m_lVNormals.GetBasePtr();

/* Removed by Mondelore.
	fp32 Time = _Time;
	int iFrm0 = int(floor(Time)) % _pTVB->GetNumFrames();
	int iFrm1 = (iFrm0 + 1) % _pTVB->GetNumFrames();
	fp32 TimeFrac = Time - floor(Time);
*/	

	// Added by Mondelore {START}.
	fp32 TimeFrac;
	int iFrm0, iFrm1, iTFrm0, iTFrm1;

	_pTVB->GetFrameAndTimeFraction(_Time, iFrm0, iFrm1, TimeFrac); // Added by Mondelore.

	if (_pTVB->GetNumTFrames() == 1)
	{
		iTFrm0 = 0;
		iTFrm1 = 1;
	}
	else
	{
		iTFrm0 = iFrm0;
		iTFrm1 = iFrm1;
	}
	// Added by Mondelore {END}.

	const int bTransformNormals = 1;

	const CVec3Dfp32* pV1 = _pTVB->GetVertexPtr(this, iFrm0);
	const CVec3Dfp32* pN1 = _pTVB->GetNormalPtr(this, iFrm0);
	const CVec2Dfp32* pT1 = _pTVB->GetTVertexPtr(this, iTFrm0); // Added by Mondelore.
	if (!pV1) return;

	const CVec3Dfp32* pV2 = _pTVB->GetVertexPtr(this, iFrm1);
	const CVec3Dfp32* pN2 = _pTVB->GetNormalPtr(this, iFrm1);
	const CVec2Dfp32* pT2 = _pTVB->GetTVertexPtr(this, iTFrm1); // Added by Mondelore.

	// Is frame1 == frame2 we don't want to interpolate
	if (pV2 == pV1) pV2 = NULL;

	// If time is close enough to the first frame we don't want to interpolate
	if (TimeFrac < 0.001f) pV2 = NULL;

	// If interpolation is disabled, well - don't interpolate..
	if (m_RenderFlags & CTM_RFLAGS_NOVERTEXANIMIP) pV2 = NULL;

	if (pV2)
	{
		for(int v = 0; v < nV; v++)
		{
			fp32 x,y,z;
			x = pV1[v].k[0] + fp32(pV2[v].k[0] - pV1[v].k[0])*TimeFrac;
			y = pV1[v].k[1] + fp32(pV2[v].k[1] - pV1[v].k[1])*TimeFrac;
			z = pV1[v].k[2] + fp32(pV2[v].k[2] - pV1[v].k[2])*TimeFrac;

			pVV[v].k[0] = _pMat->k[0][0]*x + _pMat->k[1][0]*y + _pMat->k[2][0]*z + _pMat->k[3][0];
			pVV[v].k[1] = _pMat->k[0][1]*x + _pMat->k[1][1]*y + _pMat->k[2][1]*z + _pMat->k[3][1];
			pVV[v].k[2] = _pMat->k[0][2]*x + _pMat->k[1][2]*y + _pMat->k[2][2]*z + _pMat->k[3][2];

			if (bTransformNormals)
			{
				fp32 nx,ny,nz;
				nx = pN1[v].k[0]; ny = pN1[v].k[1]; nz = pN1[v].k[2];
				nx += (fp32(pN2[v].k[0]) - nx)*TimeFrac;
				ny += (fp32(pN2[v].k[1]) - ny)*TimeFrac;
				nz += (fp32(pN2[v].k[2]) - nz)*TimeFrac;

				pVN[v].k[0] = _pNormMat->k[0][0]*nx + _pNormMat->k[1][0]*ny + _pNormMat->k[2][0]*nz;
				pVN[v].k[1] = _pNormMat->k[0][1]*nx + _pNormMat->k[1][1]*ny + _pNormMat->k[2][1]*nz;
				pVN[v].k[2] = _pNormMat->k[0][2]*nx + _pNormMat->k[1][2]*ny + _pNormMat->k[2][2]*nz;
			}
		}

		// Added by Mondelore {START}.
		if (pT2)
		{
			for (int i = 0; i < nTV; i++)
			{
				pVT[i].k[0] = pT1[i].k[0] + fp32(pT2[i].k[0] - pT1[i].k[0]) * TimeFrac;
				pVT[i].k[1] = pT1[i].k[1] + fp32(pT2[i].k[1] - pT1[i].k[1]) * TimeFrac;
			}
		}
		else
		{
			for (int i = 0; i < nTV; i++)
			{
				pVT[i].k[0] = pT1[i].k[0];
				pVT[i].k[1] = pT1[i].k[1];
			}
		}
		// Added by Mondelore {END}.
	}
	else
	{
		for(int v = 0; v < nV; v++)
		{
			fp32 x,y,z;
			x = pV1[v].k[0]; y = pV1[v].k[1]; z = pV1[v].k[2];
			pVV[v].k[0] = _pMat->k[0][0]*x + _pMat->k[1][0]*y + _pMat->k[2][0]*z + _pMat->k[3][0];
			pVV[v].k[1] = _pMat->k[0][1]*x + _pMat->k[1][1]*y + _pMat->k[2][1]*z + _pMat->k[3][1];
			pVV[v].k[2] = _pMat->k[0][2]*x + _pMat->k[1][2]*y + _pMat->k[2][2]*z + _pMat->k[3][2];

			if (bTransformNormals)
			{
				fp32 nx,ny,nz;
				nx = pN1[v].k[0]; ny = pN1[v].k[1]; nz = pN1[v].k[2];
				pVN[v].k[0] = _pNormMat->k[0][0]*nx + _pNormMat->k[1][0]*ny + _pNormMat->k[2][0]*nz;
				pVN[v].k[1] = _pNormMat->k[0][1]*nx + _pNormMat->k[1][1]*ny + _pNormMat->k[2][1]*nz;
				pVN[v].k[2] = _pNormMat->k[0][2]*nx + _pNormMat->k[1][2]*ny + _pNormMat->k[2][2]*nz;
			}
		}

		// Added by Mondelore {START}.
		for (int i = 0; i < nTV; i++)
		{
			pVT[i].k[0] = pT1[i].k[0];
			pVT[i].k[1] = pT1[i].k[1];
		}
		// Added by Mondelore {END}.
	}
}

void CXR_Model_TriangleMesh::Cluster_Transform_V_N_TgU_TgV(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_VertexBuffer* _pTVB, const CMat4Dfp32* _pMat, const CMat4Dfp32* _pNormMat, const CMTime& _Time)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_Transform_V_N_TgU_TgV, MAUTOSTRIP_VOID);
	int nV = _pTVB->GetNumVertices(this);
	int nTV = nV; // Added by Mondelore.

//	ConOut(CStrF("Transform! %d", nV));

	CVec3Dfp32* pVV = _pRenderParams->m_pRenderV;
	CVec2Dfp32* pVT = _pRenderParams->m_pRenderTV; // Added by Mondelore.
	CVec3Dfp32* pVN = _pRenderParams->m_pRenderN;
	CVec3Dfp32* pVTU = _pRenderParams->m_pRenderTangU;
	CVec3Dfp32* pVTV = _pRenderParams->m_pRenderTangV;

//	CVec3Dfp32* pVV = m_lVVertices.GetBasePtr();
//	CVec3Dfp32* pVN = m_lVNormals.GetBasePtr();

/* Removed by Mondelore.
	fp32 Time = _Time;
	int iFrm0 = int(floor(Time)) % _pTVB->GetNumFrames();
	int iFrm1 = (iFrm0 + 1) % _pTVB->GetNumFrames();
	fp32 TimeFrac = Time - floor(Time);
*/

	// Added by Mondelore {START}.
	fp32 TimeFrac;
	int iFrm0, iFrm1, iTFrm0, iTFrm1;

	_pTVB->GetFrameAndTimeFraction(_Time, iFrm0, iFrm1, TimeFrac); // Added by Mondelore.

	if (_pTVB->GetNumTFrames() == 1)
	{
		iTFrm0 = 0;
		iTFrm1 = 1;
	}
	else
	{
		iTFrm0 = iFrm0;
		iTFrm1 = iFrm1;
	}
	// Added by Mondelore {END}.

	const CVec3Dfp32* pV1 = _pTVB->GetVertexPtr(this, iFrm0);
	const CVec3Dfp32* pN1 = _pTVB->GetNormalPtr(this, iFrm0);
	const CVec3Dfp32* pTU1 = _pTVB->GetTangentUPtr(this, iFrm0);
	const CVec3Dfp32* pTV1 = _pTVB->GetTangentVPtr(this, iFrm0);
	const CVec2Dfp32* pT1 = _pTVB->GetTVertexPtr(this, iTFrm0); // Added by Mondelore.
	if (!pV1 || !pN1 || !pTU1 || !pTV1 || !pT1)
	{
		ConOut("§cf80WARNING: (CXR_Model_TriangleMesh::Cluster_Transform_V_N_TgU_TgV) Missing vertex component.");
		return;
	}

	const CVec3Dfp32* pV2 = _pTVB->GetVertexPtr(this, iFrm1);
	const CVec3Dfp32* pN2 = _pTVB->GetNormalPtr(this, iFrm1);
	const CVec3Dfp32* pTU2 = _pTVB->GetTangentUPtr(this, iFrm1);
	const CVec3Dfp32* pTV2 = _pTVB->GetTangentVPtr(this, iFrm1);
	const CVec2Dfp32* pT2 = _pTVB->GetTVertexPtr(this, iTFrm1); // Added by Mondelore.
	if (!pV1 || !pN1 || !pTU1 || !pTV1 || !pT2)
	{
		ConOut("§cf80WARNING: (CXR_Model_TriangleMesh::Cluster_Transform_V_N_TgU_TgV) Missing vertex component.");
		return;
	}

	// Is frame1 == frame2 we don't want to interpolate
	if (pV2 == pV1) pV2 = NULL;

	// If time is close enough to the first frame we don't want to interpolate
	if (TimeFrac < 0.001f) pV2 = NULL;

	// If interpolation is disabled, well - don't interpolate..
	if (m_RenderFlags & CTM_RFLAGS_NOVERTEXANIMIP) pV2 = NULL;

	if (pV2)
	{
		for(int v = 0; v < nV; v++)
		{
			fp32 x,y,z;
			x = pV1[v].k[0] + fp32(pV2[v].k[0] - pV1[v].k[0])*TimeFrac;
			y = pV1[v].k[1] + fp32(pV2[v].k[1] - pV1[v].k[1])*TimeFrac;
			z = pV1[v].k[2] + fp32(pV2[v].k[2] - pV1[v].k[2])*TimeFrac;

			pVV[v].k[0] = _pMat->k[0][0]*x + _pMat->k[1][0]*y + _pMat->k[2][0]*z + _pMat->k[3][0];
			pVV[v].k[1] = _pMat->k[0][1]*x + _pMat->k[1][1]*y + _pMat->k[2][1]*z + _pMat->k[3][1];
			pVV[v].k[2] = _pMat->k[0][2]*x + _pMat->k[1][2]*y + _pMat->k[2][2]*z + _pMat->k[3][2];

			{
				fp32 nx,ny,nz;
				nx = pN1[v].k[0]; ny = pN1[v].k[1]; nz = pN1[v].k[2];
				nx += (fp32(pN2[v].k[0]) - nx)*TimeFrac;
				ny += (fp32(pN2[v].k[1]) - ny)*TimeFrac;
				nz += (fp32(pN2[v].k[2]) - nz)*TimeFrac;

				pVN[v].k[0] = _pNormMat->k[0][0]*nx + _pNormMat->k[1][0]*ny + _pNormMat->k[2][0]*nz;
				pVN[v].k[1] = _pNormMat->k[0][1]*nx + _pNormMat->k[1][1]*ny + _pNormMat->k[2][1]*nz;
				pVN[v].k[2] = _pNormMat->k[0][2]*nx + _pNormMat->k[1][2]*ny + _pNormMat->k[2][2]*nz;
			}
			{
				fp32 nx,ny,nz;
				nx = pTU1[v].k[0]; ny = pTU1[v].k[1]; nz = pTU1[v].k[2];
				nx += (fp32(pTU2[v].k[0]) - nx)*TimeFrac;
				ny += (fp32(pTU2[v].k[1]) - ny)*TimeFrac;
				nz += (fp32(pTU2[v].k[2]) - nz)*TimeFrac;

				pVTU[v].k[0] = _pNormMat->k[0][0]*nx + _pNormMat->k[1][0]*ny + _pNormMat->k[2][0]*nz;
				pVTU[v].k[1] = _pNormMat->k[0][1]*nx + _pNormMat->k[1][1]*ny + _pNormMat->k[2][1]*nz;
				pVTU[v].k[2] = _pNormMat->k[0][2]*nx + _pNormMat->k[1][2]*ny + _pNormMat->k[2][2]*nz;
			}
			{
				fp32 nx,ny,nz;
				nx = pTV1[v].k[0]; ny = pTV1[v].k[1]; nz = pTV1[v].k[2];
				nx += (fp32(pTV2[v].k[0]) - nx)*TimeFrac;
				ny += (fp32(pTV2[v].k[1]) - ny)*TimeFrac;
				nz += (fp32(pTV2[v].k[2]) - nz)*TimeFrac;

				pVTV[v].k[0] = _pNormMat->k[0][0]*nx + _pNormMat->k[1][0]*ny + _pNormMat->k[2][0]*nz;
				pVTV[v].k[1] = _pNormMat->k[0][1]*nx + _pNormMat->k[1][1]*ny + _pNormMat->k[2][1]*nz;
				pVTV[v].k[2] = _pNormMat->k[0][2]*nx + _pNormMat->k[1][2]*ny + _pNormMat->k[2][2]*nz;
			}

		}

		// Added by Mondelore {START}.
		if (pT2)
		{
			for (int i = 0; i < nTV; i++)
			{
				pVT[i].k[0] = pT1[i].k[0] + fp32(pT2[i].k[0] - pT1[i].k[0]) * TimeFrac;
				pVT[i].k[1] = pT1[i].k[1] + fp32(pT2[i].k[1] - pT1[i].k[1]) * TimeFrac;
			}
		}
		else
		{
			for (int i = 0; i < nTV; i++)
			{
				pVT[i].k[0] = pT1[i].k[0];
				pVT[i].k[1] = pT1[i].k[1];
			}
		}
		// Added by Mondelore {END}.
	}
	else
	{
		for(int v = 0; v < nV; v++)
		{
			fp32 x,y,z;
			x = pV1[v].k[0]; y = pV1[v].k[1]; z = pV1[v].k[2];
			pVV[v].k[0] = _pMat->k[0][0]*x + _pMat->k[1][0]*y + _pMat->k[2][0]*z + _pMat->k[3][0];
			pVV[v].k[1] = _pMat->k[0][1]*x + _pMat->k[1][1]*y + _pMat->k[2][1]*z + _pMat->k[3][1];
			pVV[v].k[2] = _pMat->k[0][2]*x + _pMat->k[1][2]*y + _pMat->k[2][2]*z + _pMat->k[3][2];

			{
				fp32 nx,ny,nz;
				nx = pN1[v].k[0]; ny = pN1[v].k[1]; nz = pN1[v].k[2];
				pVN[v].k[0] = _pNormMat->k[0][0]*nx + _pNormMat->k[1][0]*ny + _pNormMat->k[2][0]*nz;
				pVN[v].k[1] = _pNormMat->k[0][1]*nx + _pNormMat->k[1][1]*ny + _pNormMat->k[2][1]*nz;
				pVN[v].k[2] = _pNormMat->k[0][2]*nx + _pNormMat->k[1][2]*ny + _pNormMat->k[2][2]*nz;
			}
			{
				fp32 nx,ny,nz;
				nx = pTU1[v].k[0]; ny = pTU1[v].k[1]; nz = pTU1[v].k[2];
				pVTU[v].k[0] = _pNormMat->k[0][0]*nx + _pNormMat->k[1][0]*ny + _pNormMat->k[2][0]*nz;
				pVTU[v].k[1] = _pNormMat->k[0][1]*nx + _pNormMat->k[1][1]*ny + _pNormMat->k[2][1]*nz;
				pVTU[v].k[2] = _pNormMat->k[0][2]*nx + _pNormMat->k[1][2]*ny + _pNormMat->k[2][2]*nz;
			}
			{
				fp32 nx,ny,nz;
				nx = pTV1[v].k[0]; ny = pTV1[v].k[1]; nz = pTV1[v].k[2];
				pVTV[v].k[0] = _pNormMat->k[0][0]*nx + _pNormMat->k[1][0]*ny + _pNormMat->k[2][0]*nz;
				pVTV[v].k[1] = _pNormMat->k[0][1]*nx + _pNormMat->k[1][1]*ny + _pNormMat->k[2][1]*nz;
				pVTV[v].k[2] = _pNormMat->k[0][2]*nx + _pNormMat->k[1][2]*ny + _pNormMat->k[2][2]*nz;
			}
		}

		// Added by Mondelore {START}.
		for (int i = 0; i < nTV; i++)
		{
			pVT[i].k[0] = pT1[i].k[0];
			pVT[i].k[1] = pT1[i].k[1];
		}
		// Added by Mondelore {END}.
	}
}

void CXR_Model_TriangleMesh::Cluster_TransformBones_V(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_VertexBuffer* _pTVB, const CMat4Dfp32 *_pMatrixPaletteArgs, const CMTime& _Time)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_TransformBones_V, MAUTOSTRIP_VOID);
	int nV = _pTVB->GetNumVertices(this);
	CVec3Dfp32* pVV = _pRenderParams->m_pRenderV;

//	fp32 Time = 5.0f*GetCPUClock() / GetCPUFrequency();
	int iFrm0 = TruncToInt(_Time.GetTimeModulus(_pTVB->GetNumFrames()));

	const CVec3Dfp32* pV1 = _pTVB->GetVertexPtr(this, iFrm0);
	if (!pV1) return;

//  Currently, Vertex-animation interpolation is not supported with bone-deformation.
//	CTM_Vertex_fp32* pV2 = _pTVB->GetVertexPtr(iFrm1);
//	if (pV2 == pV1) pV2 = NULL;

	const CTM_BDVertexInfo* pVI = _pTVB->GetBDVertexInfo(this);
	const CTM_BDInfluence* pBI = _pTVB->GetBDInfluence(this);

	const CVec3Dfp32* pV = pV1;
#if 1
	for(int v = 0; v < nV; v++)
	{
		int nBones = pVI[v].m_nBones;
		int iBI = pVI[v].m_iBoneInfluence;

#ifdef PLATFORM_XBOX1
			const CMat4Dfp32* pM = _pMatrixPaletteArgs + pBI[iBI + 0].m_iBone;
			fp32 Influence = pBI[iBI + 0].m_Influence;
			// Blend matrices
#if 0
			__asm
			{
				movss  xmm0, [Influence]
				shufps xmm0, xmm0, 0
				mov ecx, pM

				movups xmm4, [ecx]
				mulps  xmm4, xmm0

				movups xmm5, [ecx+0xc]
				mulps  xmm5, xmm0

				movups xmm6, [ecx+0x18]
				mulps  xmm6, xmm0

				movups xmm7, [ecx+0x24]
				mulps  xmm7, xmm0
			}
#else
			// Reorder for speed
			__asm
			{
				movss  xmm0, [Influence]
				mov ecx, pM
				movups xmm4, [ecx]
				shufps xmm0, xmm0, 0
				movups xmm5, [ecx+0xc]
				mulps  xmm4, xmm0
				movups xmm6, [ecx+0x18]
				mulps  xmm5, xmm0
				movups xmm7, [ecx+0x24]
				mulps  xmm6, xmm0
				mulps  xmm7, xmm0
			}
#endif
			--nBones;

			while (nBones)
			{
				++iBI;

				pM = _pMatrixPaletteArgs + pBI[iBI].m_iBone;
				Influence = pBI[iBI].m_Influence;
#if 0
				__asm
				{
					movss  xmm0, [Influence]
					shufps xmm0, xmm0, 0
					mov ecx, pM
						
					movups xmm1, [ecx]
					mulps  xmm1, xmm0
					addps  xmm4, xmm1

					movups xmm1, [ecx+0xc]
					mulps  xmm1, xmm0
					addps  xmm5, xmm1

					movups xmm1, [ecx+0x18]
					mulps  xmm1, xmm0
					addps  xmm6, xmm1

					movups xmm1, [ecx+0x24]
					mulps  xmm1, xmm0
					addps  xmm7, xmm1
				}
#else
				// Reorder for speed
				__asm
				{
					movss  xmm0, [Influence]
					mov ecx, pM
						
					movups xmm1, [ecx]
					shufps xmm0, xmm0, 0
					movups xmm2, [ecx+0xc]
					mulps  xmm1, xmm0
					movups xmm3, [ecx+0x18]
					mulps  xmm2, xmm0
					addps  xmm4, xmm1
					mulps  xmm3, xmm0
					addps  xmm5, xmm2
					movups xmm1, [ecx+0x24]
					mulps  xmm1, xmm0
					addps  xmm6, xmm3
					addps  xmm7, xmm1
				}
#endif
				--nBones;
			}
			// Multiply vertex
#if 0
			__asm
			{
				// Store source
				mov    eax, pV
				mov ecx, pVV

				movss  xmm0, [eax]
				movss  xmm1, [eax+4]
				movss  xmm2, [eax+8]

				// Expand source
				shufps xmm0, xmm0, 0
				shufps xmm1, xmm1, 0
				shufps xmm2, xmm2, 0

				// Mul
				mulps  xmm0, xmm4
				mulps  xmm1, xmm5
				mulps  xmm2, xmm6

				// Add
				addps xmm0, xmm1
				addps xmm0, xmm2

				// Add translate
				addps xmm0, xmm7

				// Save vertex and don't care about 4:th overwrite
				movups [ecx], xmm0
			}
#else
// Reorder for speed
			__asm
			{
				// Store source
				mov    ecx, pV

				movss  xmm0, [ecx]
				movss  xmm1, [ecx+4]
				shufps xmm0, xmm0, 0
				movss  xmm2, [ecx+8]
				shufps xmm1, xmm1, 0
				mulps  xmm0, xmm4
				shufps xmm2, xmm2, 0
				mov    ecx, pVV
				mulps  xmm1, xmm5
				addps xmm0, xmm7
				mulps  xmm2, xmm6
				addps xmm0, xmm1
				addps xmm0, xmm2


				// Save vertex and don't care about 4:th overwrite
				movups [ecx], xmm0
			}
#endif


#else
		if (nBones == 1)
		{
			//---------------------------------------
			// 1 Bone, 18 Muls.

			const CMat4Dfp32* pM = &_pMatrixPaletteArgs[pBI[iBI].m_iBone];
			fp32 x = pV->k[0];
			fp32 y = pV->k[1];
			fp32 z = pV->k[2];
			(*pVV).k[0] = pM->k[0][0]*x + pM->k[1][0]*y + pM->k[2][0]*z + pM->k[3][0];
			(*pVV).k[1] = pM->k[0][1]*x + pM->k[1][1]*y + pM->k[2][1]*z + pM->k[3][1];
			(*pVV).k[2] = pM->k[0][2]*x + pM->k[1][2]*y + pM->k[2][2]*z + pM->k[3][2];
		}
		else if (nBones == 2)
		{
			//---------------------------------------
			// 2 Bones, 18 + 24 Muls.

			const CMat4Dfp32* pM1 = &_pMatrixPaletteArgs[pBI[iBI].m_iBone];
			fp32 s1 = pBI[iBI].m_Influence;
			const CMat4Dfp32* pM2 = &_pMatrixPaletteArgs[pBI[iBI+1].m_iBone];
			fp32 s2 = pBI[iBI+1].m_Influence;

			{
				fp32 mx = (pM1->k[0][0]*s1 + pM2->k[0][0]*s2);
				fp32 my = (pM1->k[1][0]*s1 + pM2->k[1][0]*s2);
				fp32 mz = (pM1->k[2][0]*s1 + pM2->k[2][0]*s2);
				fp32 mt = (pM1->k[3][0]*s1 + pM2->k[3][0]*s2);
				(*pVV).k[0] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
			}

			{
				fp32 mx = (pM1->k[0][1]*s1 + pM2->k[0][1]*s2);
				fp32 my = (pM1->k[1][1]*s1 + pM2->k[1][1]*s2);
				fp32 mz = (pM1->k[2][1]*s1 + pM2->k[2][1]*s2);
				fp32 mt = (pM1->k[3][1]*s1 + pM2->k[3][1]*s2);
				(*pVV).k[1] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
			}

			{
				fp32 mx = (pM1->k[0][2]*s1 + pM2->k[0][2]*s2);
				fp32 my = (pM1->k[1][2]*s1 + pM2->k[1][2]*s2);
				fp32 mz = (pM1->k[2][2]*s1 + pM2->k[2][2]*s2);
				fp32 mt = (pM1->k[3][2]*s1 + pM2->k[3][2]*s2);
				(*pVV).k[2] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
			}
		}
		else
		{
			//---------------------------------------
			// n Bones, 12n + 18 Muls
			{
				fp32 mx, my, mz, mt; mx = my = mz = mt = 0;
				for(int iiBone = 0; iiBone < nBones; iiBone++)
				{
					const CMat4Dfp32* pM = &_pMatrixPaletteArgs[pBI[iBI + iiBone].m_iBone];
					fp32 s = pBI[iBI + iiBone].m_Influence;
					mx += pM->k[0][0]*s; my += pM->k[1][0]*s; mz += pM->k[2][0]*s; mt += pM->k[3][0]*s;
				}
				(*pVV).k[0] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
			}

			{
				fp32 mx, my, mz, mt; mx = my = mz = mt = 0;
				for(int iiBone = 0; iiBone < nBones; iiBone++)
				{
					const CMat4Dfp32* pM = &_pMatrixPaletteArgs[pBI[iBI + iiBone].m_iBone];
					fp32 s = pBI[iBI + iiBone].m_Influence;
					mx += pM->k[0][1]*s; my += pM->k[1][1]*s; mz += pM->k[2][1]*s; mt += pM->k[3][1]*s;
				}
				(*pVV).k[1] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
			}

			{
				fp32 mx, my, mz, mt; mx = my = mz = mt = 0;
				for(int iiBone = 0; iiBone < nBones; iiBone++)
				{
					const CMat4Dfp32* pM = &_pMatrixPaletteArgs[pBI[iBI + iiBone].m_iBone];
					fp32 s = pBI[iBI + iiBone].m_Influence;
					mx += pM->k[0][2]*s; my += pM->k[1][2]*s; mz += pM->k[2][2]*s; mt += pM->k[3][2]*s;
				}
				(*pVV).k[2] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
			}			
		}
#endif

		pV++;
		pVV++;
	}
#else

	if (!_pTVB->m_lMatrixIndices.Len())
		_pTVB->CreateMatrixIW();

	uint32 *pIndex = _pTVB->m_lMatrixIndices.GetBasePtr();
	fp32 *pWeights = _pTVB->m_lMatrixWeights.GetBasePtr();

	for(int v = 0; v < nV; v++)
	{
		int iWeight = v*3;
		//---------------------------------------
		// n Bones, 12n + 18 Muls
		int i0 = *pIndex;
		int i1 = (i0 << 8) & 0xff;
		int i2 = (i0 << 16) & 0xff;
		i0 &= 0xff;

		{
			fp32 mx, my, mz, mt;
			fp32 mx1, my1, mz1, mt1;
			fp32 mx2, my2, mz2, mt2;
			fp32 s = pWeights[iWeight + 0];
			const CMat4Dfp32* pM = &_pMatrixPaletteArgs[i0];
			mx = pM->k[0][0]*s; 
			my = pM->k[1][0]*s; 
			mz = pM->k[2][0]*s; 
			mt = pM->k[3][0]*s;
			mx1 = pM->k[0][1]*s; 
			my1 = pM->k[1][1]*s; 
			mz1 = pM->k[2][1]*s; 
			mt1 = pM->k[3][1]*s;
			mx2 = pM->k[0][2]*s; 
			my2 = pM->k[1][2]*s; 
			mz2 = pM->k[2][2]*s; 
			mt2 = pM->k[3][2]*s;
			s = pWeights[iWeight + 1];
			pM = &_pMatrixPaletteArgs[i1];
			mx += pM->k[0][0]*s; 
			my += pM->k[1][0]*s; 
			mz += pM->k[2][0]*s; 
			mt += pM->k[3][0]*s;
			mx1 += pM->k[0][1]*s; 
			my1 += pM->k[1][1]*s; 
			mz1 += pM->k[2][1]*s; 
			mt1 += pM->k[3][1]*s;
			mx2 += pM->k[0][2]*s; 
			my2 += pM->k[1][2]*s; 
			mz2 += pM->k[2][2]*s; 
			mt2 += pM->k[3][2]*s;
			s = pWeights[iWeight + 2];
			pM = &_pMatrixPaletteArgs[i2];
			mx += pM->k[0][0]*s; 
			my += pM->k[1][0]*s; 
			mz += pM->k[2][0]*s; 
			mt += pM->k[3][0]*s;
			mx1 += pM->k[0][1]*s; 
			my1 += pM->k[1][1]*s; 
			mz1 += pM->k[2][1]*s; 
			mt1 += pM->k[3][1]*s;
			mx2 += pM->k[0][2]*s; 
			my2 += pM->k[1][2]*s; 
			mz2 += pM->k[2][2]*s; 
			mt2 += pM->k[3][2]*s;

			pVV[v].k[0] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
			pVV[v].k[1] = mx1*pV->k[0] + my1*pV->k[1] + mz1*pV->k[2] + mt1;
			pVV[v].k[2] = mx2*pV->k[0] + my2*pV->k[1] + mz2*pV->k[2] + mt2;
		}
		pV++;
	}
#endif
}
void CXR_Model_TriangleMesh::Cluster_TransformBones_V_VPU(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_VertexBuffer* _pTVB, const CMat4Dfp32 *_pMatrixPaletteArgs, mint _nMat, const CMTime& _Time)
{
/*
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_TransformBones_V_VPU, MAUTOSTRIP_VOID);
	int nV = _pTVB->GetNumVertices();
	CVec3Dfp32* pVV = _pRenderParams->m_pRenderV;

	//	fp32 Time = 5.0f*GetCPUClock() / GetCPUFrequency();
	int iFrm0 = TruncToInt(_Time.GetTimeModulus(_pTVB->GetNumFrames()));

	const CVec3Dfp32* pV1 = _pTVB->GetVertexPtr(iFrm0);
	if (!pV1) return;

	//  Currently, Vertex-animation interpolation is not supported with bone-deformation.
	//	CTM_Vertex_fp32* pV2 = _pTVB->GetVertexPtr(iFrm1);
	//	if (pV2 == pV1) pV2 = NULL;

	const CTM_BDVertexInfo* pVI = _pTVB->GetBDVertexInfo();
	const CTM_BDInfluence* pBI = _pTVB->GetBDInfluence();
	int nBDInfluences=_pTVB->GetNumBDInfluence();
	const CVec3Dfp32* pV = pV1;


	CVPU_JobDefinition JobDef;
	JobDef.AddSimpleBuffer(0,(void*) _pMatrixPaletteArgs,_nMat,sizeof(CMat4Dfp32),VPU_IN_BUFFER);
	JobDef.AddInStreamBuffer(1,(void*) pV1,(nV+15)/16,128,16*sizeof(CVec3Dfp32));
	JobDef.AddOutStreamBuffer(2,(void*) pVV,(nV+15)/16,128,16*sizeof(CVec3Dfp32));
	JobDef.AddInStreamBuffer(3,(void*) pVI,(nV+15)/16,128,16*sizeof(CTM_BDVertexInfo));
	JobDef.AddCacheBuffer(4,(void*) pBI,nBDInfluences,sizeof(CTM_BDInfluence),16);
	JobDef.AddLParams(5,nV,0,0,0);
	JobDef.SetJob(MHASH5('TRAN','SFOR','M_V_','BONE','S'));
	uint32 taskId=MRTC_ThreadPoolManager::VPU_AddTask(JobDef);
	MRTC_ThreadPoolManager::VPU_BlockOnTask(taskId);


	for(int v = 0; v < nV; v++)
	{
		int nBones = pVI[v].m_nBones;
		int iBI = pVI[v].m_iBoneInfluence;

		if (nBones == 1)
		{
			//---------------------------------------
			// 1 Bone, 18 Muls.
			CMat4Dfp32 m;
			vec128 a,b,c,d;
			CMat4Dfp32 M_ALIGN(16) mm=_pMatrixPaletteArgs[pBI[iBI].m_iBone];
			M_VLd_V3x4(&mm,a,b,c,d);
			m.r[0]=a;
			m.r[1]=b;
			m.r[2]=c;
			m.r[3]=d;

			vec128 v0=M_VLd(pV->k[0],pV->k[1],pV->k[2],1);
			vec128 v1=M_VMulMat4x3(v0,m);
			(*pVV) = M_VGetV3_Slow(v1);
		}
		else if (nBones == 2)
		{
			//---------------------------------------
			// 2 Bones, 18 + 24 Muls.

			CMat4Dfp32 M_ALIGN(16) m0=_pMatrixPaletteArgs[pBI[iBI].m_iBone];
			fp32 s1 = pBI[iBI].m_Influence;
			CMat4Dfp32 M_ALIGN(16) m1=_pMatrixPaletteArgs[pBI[iBI+1].m_iBone];
			fp32 s2 = pBI[iBI+1].m_Influence;
			CMat4Dfp32 m;
			vec128 a0,b0,c0,d0;
			vec128 a1,b1,c1,d1;
			M_VLd_V3x4(&m0,a0,b0,c0,d0);
			M_VLd_V3x4(&m1,a1,b1,c1,d1);
			vec128 lrp=M_VLdScalar(s2);
			m.r[0]=M_VLrp(a0,a1,lrp);
			m.r[1]=M_VLrp(b0,b1,lrp);
			m.r[2]=M_VLrp(c0,c1,lrp);
			m.r[3]=M_VLrp(d0,d1,lrp);
			vec128 v0=M_VLd(pV->k[0],pV->k[1],pV->k[2],1);
			vec128 v1=M_VMulMat4x3(v0,m);
			(*pVV) = M_VGetV3_Slow(v1);
		}

		else
		{
			//---------------------------------------
			// n Bones, 12n + 18 Muls
			{
				vec128 a=M_VZero();
				vec128 b=M_VZero();
				vec128 c=M_VZero();
				vec128 d=M_VZero();
				for(int iiBone = 0; iiBone < nBones; iiBone++)
				{
					CMat4Dfp32 M_ALIGN(16) mm=_pMatrixPaletteArgs[pBI[iBI + iiBone].m_iBone];
					vec128 s=M_VLdScalar(pBI[iBI + iiBone].m_Influence);
					vec128 atmp,btmp,ctmp,dtmp;
					M_VLd_V3x4(&mm,atmp,btmp,ctmp,dtmp);
					a=M_VAdd(a,M_VMul(atmp,s));
					b=M_VAdd(b,M_VMul(btmp,s));
					c=M_VAdd(c,M_VMul(ctmp,s));
					d=M_VAdd(d,M_VMul(dtmp,s));
				}
				CMat4Dfp32 m;
				m.r[0]=a;
				m.r[1]=b;
				m.r[2]=c;
				m.r[3]=d;
				vec128 v0=M_VLd(pV->k[0],pV->k[1],pV->k[2],1);
				vec128 v1=M_VMulMat4x3(v0,m);
				(*pVV) = M_VGetV3_Slow(v1);
			}
		}

		pV++;
		pVV++;
	}
*/
}


// -------------------------------------------------------------------
void CXR_Model_TriangleMesh::Cluster_TransformBones_V_N(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_VertexBuffer* _pTVB, const CRC_MatrixPalette *_pMatrixPaletteArgs, const CMTime& _Time)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_TransformBones_V_N, MAUTOSTRIP_VOID);
	int nV = _pTVB->GetNumVertices(this);
	CVec3Dfp32* pVV = _pRenderParams->m_pRenderV;
	CVec3Dfp32* pVN = _pRenderParams->m_pRenderN;
	if(!pVV || !pVN)
		return;

//	CVec3Dfp32* pVV = m_lVVertices.GetBasePtr();
//	CVec3Dfp32* pVN = m_lVNormals.GetBasePtr();

//	fp32 Time = 5.0f*GetCPUClock() / GetCPUFrequency();
	fp32 Time = _Time.GetTimeModulus(_pTVB->GetNumFrames());
	int iFrm0 = TruncToInt(Time);
//	int iFrm1 = (iFrm0 + 1) % _pTVB->GetNumFrames();
//	fp32 TimeFrac = Time - (fp32)iFrm0;

	const CVec3Dfp32* pV1 = _pTVB->GetVertexPtr(this, iFrm0);
	const CVec3Dfp32* pN1 = _pTVB->GetNormalPtr(this, iFrm0);
	if (!pV1 || !pN1) return;

//  Currently, Vertex-animation interpolation is not supported with bone-deformation.
//	CTM_Vertex_fp32* pV2 = _pTVB->GetVertexPtr(iFrm1);
//	if (pV2 == pV1) pV2 = NULL;

	const CTM_BDVertexInfo* pVI = _pTVB->GetBDVertexInfo(this);
	const CTM_BDInfluence* pBI = _pTVB->GetBDInfluence(this);

	const CVec3Dfp32* pV = pV1;
	const CVec3Dfp32* pN = pN1;
	for(int v = 0; v < nV; v++)
	{
		int nBones = pVI[v].m_nBones;
		int iBI = pVI[v].m_iBoneInfluence;

		if (nBones == 1)
		{
			//---------------------------------------
			// 1 Bone, 18 Muls.

			const CMat43fp32* pM = &_pMatrixPaletteArgs->Index(pBI[iBI].m_iBone);
			fp32 x = pV->k[0];
			fp32 y = pV->k[1];
			fp32 z = pV->k[2];
			pVV[v].k[0] = pM->k[0][0]*x + pM->k[1][0]*y + pM->k[2][0]*z + pM->k[3][0];
			pVV[v].k[1] = pM->k[0][1]*x + pM->k[1][1]*y + pM->k[2][1]*z + pM->k[3][1];
			pVV[v].k[2] = pM->k[0][2]*x + pM->k[1][2]*y + pM->k[2][2]*z + pM->k[3][2];

			fp32 nx = pN->k[0];
			fp32 ny = pN->k[1];
			fp32 nz = pN->k[2];
			pVN[v].k[0] = pM->k[0][0]*nx + pM->k[1][0]*ny + pM->k[2][0]*nz;
			pVN[v].k[1] = pM->k[0][1]*nx + pM->k[1][1]*ny + pM->k[2][1]*nz;
			pVN[v].k[2] = pM->k[0][2]*nx + pM->k[1][2]*ny + pM->k[2][2]*nz;
		}
		else if (nBones == 2)
		{
			//---------------------------------------
			// 2 Bones, 18 + 24 Muls.

			const CMat43fp32* pM1 = &_pMatrixPaletteArgs->Index(pBI[iBI].m_iBone);
			fp32 s1 = pBI[iBI].m_Influence;
			const CMat43fp32* pM2 = &_pMatrixPaletteArgs->Index(pBI[iBI+1].m_iBone);
			fp32 s2 = pBI[iBI+1].m_Influence;

			{
				fp32 mx = (pM1->k[0][0]*s1 + pM2->k[0][0]*s2);
				fp32 my = (pM1->k[1][0]*s1 + pM2->k[1][0]*s2);
				fp32 mz = (pM1->k[2][0]*s1 + pM2->k[2][0]*s2);
				fp32 mt = (pM1->k[3][0]*s1 + pM2->k[3][0]*s2);
				pVV[v].k[0] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
				pVN[v].k[0]	= mx*pN->k[0] + my*pN->k[1] + mz*pN->k[2];
			}

			{
				fp32 mx = (pM1->k[0][1]*s1 + pM2->k[0][1]*s2);
				fp32 my = (pM1->k[1][1]*s1 + pM2->k[1][1]*s2);
				fp32 mz = (pM1->k[2][1]*s1 + pM2->k[2][1]*s2);
				fp32 mt = (pM1->k[3][1]*s1 + pM2->k[3][1]*s2);
				pVV[v].k[1] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
				pVN[v].k[1]	= mx*pN->k[0] + my*pN->k[1] + mz*pN->k[2];
			}

			{
				fp32 mx = (pM1->k[0][2]*s1 + pM2->k[0][2]*s2);
				fp32 my = (pM1->k[1][2]*s1 + pM2->k[1][2]*s2);
				fp32 mz = (pM1->k[2][2]*s1 + pM2->k[2][2]*s2);
				fp32 mt = (pM1->k[3][2]*s1 + pM2->k[3][2]*s2);
				pVV[v].k[2] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
				pVN[v].k[2]	= mx*pN->k[0] + my*pN->k[1] + mz*pN->k[2];
			}
		}
		else
		{
			//---------------------------------------
			// n Bones, 12n + 18 Muls
			{
				fp32 mx, my, mz, mt; mx = my = mz = mt = 0;
				for(int iiBone = 0; iiBone < nBones; iiBone++)
				{
					const CMat43fp32* pM = &_pMatrixPaletteArgs->Index(pBI[iBI + iiBone].m_iBone);
					fp32 s = pBI[iBI + iiBone].m_Influence;
					mx += pM->k[0][0]*s; my += pM->k[1][0]*s; mz += pM->k[2][0]*s; mt += pM->k[3][0]*s;
				}
				pVV[v].k[0] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
				pVN[v].k[0]	= mx*pN->k[0] + my*pN->k[1] + mz*pN->k[2];
			}

			{
				fp32 mx, my, mz, mt; mx = my = mz = mt = 0;
				for(int iiBone = 0; iiBone < nBones; iiBone++)
				{
					const CMat43fp32* pM = &_pMatrixPaletteArgs->Index(pBI[iBI + iiBone].m_iBone);
					fp32 s = pBI[iBI + iiBone].m_Influence;
					mx += pM->k[0][1]*s; my += pM->k[1][1]*s; mz += pM->k[2][1]*s; mt += pM->k[3][1]*s;
				}
				pVV[v].k[1] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
				pVN[v].k[1]	= mx*pN->k[0] + my*pN->k[1] + mz*pN->k[2];
			}

			{
				fp32 mx, my, mz, mt; mx = my = mz = mt = 0;
				for(int iiBone = 0; iiBone < nBones; iiBone++)
				{
					const CMat43fp32* pM = &_pMatrixPaletteArgs->Index(pBI[iBI + iiBone].m_iBone);
					fp32 s = pBI[iBI + iiBone].m_Influence;
					mx += pM->k[0][2]*s; my += pM->k[1][2]*s; mz += pM->k[2][2]*s; mt += pM->k[3][2]*s;
				}
				pVV[v].k[2] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
				pVN[v].k[2]	= mx*pN->k[0] + my*pN->k[1] + mz*pN->k[2];
			}			
		}

		pV++;
		pN++;
	}
}

// -------------------------------------------------------------------
void CXR_Model_TriangleMesh::Cluster_TransformBones_V_N_TgU_TgV(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_VertexBuffer* _pTVB, const CRC_MatrixPalette *_pMatrixPaletteArgs, const CMTime& _Time)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_TransformBones_V_N_TgU_TgV, MAUTOSTRIP_VOID);
	int nV = _pTVB->GetNumVertices(this);
	CVec3Dfp32* pVV = _pRenderParams->m_pRenderV;
	CVec3Dfp32* pVN = _pRenderParams->m_pRenderN;
	CVec3Dfp32* pVTU = _pRenderParams->m_pRenderTangU;
	CVec3Dfp32* pVTV = _pRenderParams->m_pRenderTangV;
	if(!pVV || !pVN || !pVTU || !pVTV)
		return;

//	CVec3Dfp32* pVV = m_lVVertices.GetBasePtr();
//	CVec3Dfp32* pVN = m_lVNormals.GetBasePtr();

//	fp32 Time = 5.0f*GetCPUClock() / GetCPUFrequency();
	fp32 Time = _Time.GetTimeModulus(_pTVB->GetNumFrames());
	int iFrm0 = TruncToInt(Time);
//	int iFrm1 = (iFrm0 + 1) % _pTVB->GetNumFrames();
//	fp32 TimeFrac = Time - (fp32)iFrm0;

	const CVec3Dfp32* pV = _pTVB->GetVertexPtr(this, iFrm0);
	const CVec3Dfp32* pN = _pTVB->GetNormalPtr(this, iFrm0);
	const CVec3Dfp32* pTU = _pTVB->GetTangentUPtr(this, iFrm0);
	const CVec3Dfp32* pTV = _pTVB->GetTangentVPtr(this, iFrm0);
	if (!pV || !pN || !pTU || !pTV)
	{
		ConOut("§cf80WARNING: (CXR_Model_TriangleMesh::Cluster_TransformBones_V_N_TgU_TgV) Missing vertex component.");
		return;
	}
	if (!pVV || !pVN || !pVTU || !pVTV)
	{
		ConOut("§cf80WARNING: (CXR_Model_TriangleMesh::Cluster_TransformBones_V_N_TgU_TgV) Missing target array.");
		return;
	}

//  Currently, Vertex-animation interpolation is not supported with bone-deformation.
//	CTM_Vertex_fp32* pV2 = _pTVB->GetVertexPtr(iFrm1);
//	if (pV2 == pV1) pV2 = NULL;

	const CTM_BDVertexInfo* pVI = _pTVB->GetBDVertexInfo(this);
	const CTM_BDInfluence* pBI = _pTVB->GetBDInfluence(this);

	for(int v = 0; v < nV; v++)
	{
		int nBones = pVI[v].m_nBones;
		int iBI = pVI[v].m_iBoneInfluence;

		if (nBones == 1)
		{
			//---------------------------------------
			// 1 Bone, 18 Muls.

			const CMat43fp32* pM = &_pMatrixPaletteArgs->Index(pBI[iBI].m_iBone);
			fp32 x = pV->k[0];
			fp32 y = pV->k[1];
			fp32 z = pV->k[2];
			pVV[v].k[0] = pM->k[0][0]*x + pM->k[1][0]*y + pM->k[2][0]*z + pM->k[3][0];
			pVV[v].k[1] = pM->k[0][1]*x + pM->k[1][1]*y + pM->k[2][1]*z + pM->k[3][1];
			pVV[v].k[2] = pM->k[0][2]*x + pM->k[1][2]*y + pM->k[2][2]*z + pM->k[3][2];

			fp32 nx = pN->k[0];
			fp32 ny = pN->k[1];
			fp32 nz = pN->k[2];
			pVN[v].k[0] = pM->k[0][0]*nx + pM->k[1][0]*ny + pM->k[2][0]*nz;
			pVN[v].k[1] = pM->k[0][1]*nx + pM->k[1][1]*ny + pM->k[2][1]*nz;
			pVN[v].k[2] = pM->k[0][2]*nx + pM->k[1][2]*ny + pM->k[2][2]*nz;

			fp32 tux = pTU->k[0];
			fp32 tuy = pTU->k[1];
			fp32 tuz = pTU->k[2];
			pVTU[v].k[0] = pM->k[0][0]*tux + pM->k[1][0]*tuy + pM->k[2][0]*tuz;
			pVTU[v].k[1] = pM->k[0][1]*tux + pM->k[1][1]*tuy + pM->k[2][1]*tuz;
			pVTU[v].k[2] = pM->k[0][2]*tux + pM->k[1][2]*tuy + pM->k[2][2]*tuz;

			fp32 tvx = pTV->k[0];
			fp32 tvy = pTV->k[1];
			fp32 tvz = pTV->k[2];
			pVTV[v].k[0] = pM->k[0][0]*tvx + pM->k[1][0]*tvy + pM->k[2][0]*tvz;
			pVTV[v].k[1] = pM->k[0][1]*tvx + pM->k[1][1]*tvy + pM->k[2][1]*tvz;
			pVTV[v].k[2] = pM->k[0][2]*tvx + pM->k[1][2]*tvy + pM->k[2][2]*tvz;
		}
		else if (nBones == 2)
		{
			//---------------------------------------
			// 2 Bones, 18 + 24 Muls.

			const CMat43fp32* pM1 = &_pMatrixPaletteArgs->Index(pBI[iBI].m_iBone);
			fp32 s1 = pBI[iBI].m_Influence;
			const CMat43fp32* pM2 = &_pMatrixPaletteArgs->Index(pBI[iBI+1].m_iBone);
			fp32 s2 = pBI[iBI+1].m_Influence;

			{
				fp32 mx = (pM1->k[0][0]*s1 + pM2->k[0][0]*s2);
				fp32 my = (pM1->k[1][0]*s1 + pM2->k[1][0]*s2);
				fp32 mz = (pM1->k[2][0]*s1 + pM2->k[2][0]*s2);
				fp32 mt = (pM1->k[3][0]*s1 + pM2->k[3][0]*s2);
				pVV[v].k[0] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
				pVN[v].k[0]	= mx*pN->k[0] + my*pN->k[1] + mz*pN->k[2];
				pVTU[v].k[0] = mx*pTU->k[0] + my*pTU->k[1] + mz*pTU->k[2];
				pVTV[v].k[0] = mx*pTV->k[0] + my*pTV->k[1] + mz*pTV->k[2];
			}

			{
				fp32 mx = (pM1->k[0][1]*s1 + pM2->k[0][1]*s2);
				fp32 my = (pM1->k[1][1]*s1 + pM2->k[1][1]*s2);
				fp32 mz = (pM1->k[2][1]*s1 + pM2->k[2][1]*s2);
				fp32 mt = (pM1->k[3][1]*s1 + pM2->k[3][1]*s2);
				pVV[v].k[1] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
				pVN[v].k[1]	= mx*pN->k[0] + my*pN->k[1] + mz*pN->k[2];
				pVTU[v].k[1] = mx*pTU->k[0] + my*pTU->k[1] + mz*pTU->k[2];
				pVTV[v].k[1] = mx*pTV->k[0] + my*pTV->k[1] + mz*pTV->k[2];
			}

			{
				fp32 mx = (pM1->k[0][2]*s1 + pM2->k[0][2]*s2);
				fp32 my = (pM1->k[1][2]*s1 + pM2->k[1][2]*s2);
				fp32 mz = (pM1->k[2][2]*s1 + pM2->k[2][2]*s2);
				fp32 mt = (pM1->k[3][2]*s1 + pM2->k[3][2]*s2);
				pVV[v].k[2] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
				pVN[v].k[2]	= mx*pN->k[0] + my*pN->k[1] + mz*pN->k[2];
				pVTU[v].k[2] = mx*pTU->k[0] + my*pTU->k[1] + mz*pTU->k[2];
				pVTV[v].k[2] = mx*pTV->k[0] + my*pTV->k[1] + mz*pTV->k[2];
			}
		}
		else
		{
			//---------------------------------------
			// n Bones, 12n + 18 Muls
			{
				fp32 mx, my, mz, mt; mx = my = mz = mt = 0;
				for(int iiBone = 0; iiBone < nBones; iiBone++)
				{
					const CMat43fp32* pM = &_pMatrixPaletteArgs->Index(pBI[iBI + iiBone].m_iBone);
					fp32 s = pBI[iBI + iiBone].m_Influence;
					mx += pM->k[0][0]*s; my += pM->k[1][0]*s; mz += pM->k[2][0]*s; mt += pM->k[3][0]*s;
				}
				pVV[v].k[0] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
				pVN[v].k[0]	= mx*pN->k[0] + my*pN->k[1] + mz*pN->k[2];
				pVTU[v].k[0] = mx*pTU->k[0] + my*pTU->k[1] + mz*pTU->k[2];
				pVTV[v].k[0] = mx*pTV->k[0] + my*pTV->k[1] + mz*pTV->k[2];
			}

			{
				fp32 mx, my, mz, mt; mx = my = mz = mt = 0;
				for(int iiBone = 0; iiBone < nBones; iiBone++)
				{
					const CMat43fp32* pM = &_pMatrixPaletteArgs->Index(pBI[iBI + iiBone].m_iBone);
					fp32 s = pBI[iBI + iiBone].m_Influence;
					mx += pM->k[0][1]*s; my += pM->k[1][1]*s; mz += pM->k[2][1]*s; mt += pM->k[3][1]*s;
				}
				pVV[v].k[1] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
				pVN[v].k[1]	= mx*pN->k[0] + my*pN->k[1] + mz*pN->k[2];
				pVTU[v].k[1] = mx*pTU->k[0] + my*pTU->k[1] + mz*pTU->k[2];
				pVTV[v].k[1] = mx*pTV->k[0] + my*pTV->k[1] + mz*pTV->k[2];
			}

			{
				fp32 mx, my, mz, mt; mx = my = mz = mt = 0;
				for(int iiBone = 0; iiBone < nBones; iiBone++)
				{
					const CMat43fp32* pM = &_pMatrixPaletteArgs->Index(pBI[iBI + iiBone].m_iBone);
					fp32 s = pBI[iBI + iiBone].m_Influence;
					mx += pM->k[0][2]*s; my += pM->k[1][2]*s; mz += pM->k[2][2]*s; mt += pM->k[3][2]*s;
				}
				pVV[v].k[2] = mx*pV->k[0] + my*pV->k[1] + mz*pV->k[2] + mt;
				pVN[v].k[2]	= mx*pN->k[0] + my*pN->k[1] + mz*pN->k[2];
				pVTU[v].k[2] = mx*pTU->k[0] + my*pTU->k[1] + mz*pTU->k[2];
				pVTV[v].k[2] = mx*pTV->k[0] + my*pTV->k[1] + mz*pTV->k[2];
			}
		}

		pV++;
		pN++;
		pTU++;
		pTV++;
	}
}

// -------------------------------------------------------------------
void CXR_Model_TriangleMesh::Cluster_ProjectVertices(CTriMesh_RenderInstanceParamters* _pRenderParams, const CVec3Dfp32* _pSrc, CVec3Dfp32* _pDest, int _nV, const CVec3Dfp32& _ProjPoint, fp32 _Length)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_ProjectVertices, MAUTOSTRIP_VOID);
	for(int v = 0; v < _nV; v++)
	{
		CVec3Dfp32 projv;
		_pSrc[v].Sub(_ProjPoint, projv);
		fp32 l = M_Sqrt(projv*projv);
		fp32 s = _Length / l;
		_ProjPoint.Combine(projv, s, _pDest[v]);
	}
}


bool CXR_Model_TriangleMesh::ShadowProjectHardware_ThreadSafe(CTriMesh_RenderInstanceParamters* _pRenderParams, const CMat4Dfp32* _pMat,mint _nMat)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_ShadowProjectHardware_ThreadSafe, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_TriangleMesh::ShadowProjectHardware_ThreadSafe);
#ifndef	PLATFORM_PS2

	if (!m_spShadowData)
		return true;

	CXR_VBManager* pVBM = _pRenderParams->m_pVBM;

	CTM_Cluster* pC = &m_spShadowData->m_Cluster;
	CTM_VertexBuffer *pTVB = &m_spShadowData->m_VertexBuffer;
	const CTM_Edge *pShadowEdge = m_spShadowData->m_lEdge.GetBasePtr();
	const int32 *pShadowEdgeClusterLen = m_spShadowData->m_lEdgeCluster.GetBasePtr();
	const CTM_Triangle *pShadowTriangle = m_spShadowData->m_lTriangle.GetBasePtr();
	const int32 *pShadowTriangleClusterLen = m_spShadowData->m_lTriangleCluster.GetBasePtr();
	const CTM_ShadowData::CRenderData *pRenderData = m_spShadowData->m_lRenderData.GetBasePtr();
	int RenderDataLen = m_spShadowData->m_lRenderData.Len();

	bool bNoShadows = true;
	for(int iL = 0; iL < _pRenderParams->m_RenderInfo.m_nLights; iL++)
	{
		if(!(_pRenderParams->m_RenderInfo.m_pLightInfo[iL].m_pLight->m_Flags & CXR_LIGHT_NOSHADOWS))
		{
			bNoShadows = false;
			break;
		}
	}
	if(bNoShadows)
		return true;

//	_pRenderParams->m_pppShadowPrimData = (uint16***)pVBM->Alloc(sizeof(uint16**) * GetNumVertexBuffers());
	_pRenderParams->m_pppShadowPrimData = (uint16***)pVBM->Alloc(sizeof(uint16**) * GetNumClusters());
	if(!_pRenderParams->m_pppShadowPrimData)
		return false;

	// This model doesn't have any weights, ignore palette
	if(!pTVB->m_bHaveBones)
		_pMat = NULL;

	uint16*** pIndexLists = _pRenderParams->m_pppShadowPrimData;

	uint8 *pCalcCluster = (uint8 *)pVBM->Alloc(RenderDataLen);
	if(!pCalcCluster)
		return false;

	const int *plnClusterVertices = m_spShadowData->m_lnVertices.GetBasePtr();
	const int* plnVBVertices = m_spShadowData->m_lnVBVertices.GetBasePtr();
	if(!plnClusterVertices)
	{
		M_TRACEALWAYS(CStrF("WARNING: (CXR_Model_TriangleMesh::ShadowProjectHardware_ThreadSafe) %s is not precached", m_FileName.Str()));
		m_spShadowData->m_lnVBVertices.SetLen(GetNumVertexBuffers());
		for(int i = 0; i < m_spShadowData->m_lnVBVertices.Len(); i++)
		{
			CTM_VertexBuffer* pVB = GetVertexBuffer(i);
			m_spShadowData->m_lnVBVertices[i] = pVB->GetNumVertices(this);
		}
		plnVBVertices = m_spShadowData->m_lnVBVertices.GetBasePtr();

		m_spShadowData->m_lnVertices.SetLen(m_spShadowData->m_lRenderData.Len());
		for (int i = 0; i < m_spShadowData->m_lnVertices.Len(); ++i)
		{
			CTM_Cluster* pC = GetCluster(m_spShadowData->m_lRenderData[i].m_iShadowCluster);
			int nVertices = pC->m_nVBVert;
			if(nVertices == 0)
			{
				// Old data format, separate VB for each cluster so GetNumTriangles on VB is reliable
				CTM_VertexBuffer *pVB = GetVertexBuffer(pC->m_iVB);
				nVertices = pVB->GetNumVertices(this);
			}
			m_spShadowData->m_lnVertices[i] = nVertices;
		}
		plnClusterVertices = m_spShadowData->m_lnVertices.GetBasePtr();
	}

	int nV = pTVB->GetNumVertices(this);

	int nTri = pTVB->GetNumTriangles(this);
	CTM_Triangle *pTri = (CTM_Triangle *)pTVB->GetTriangles(this);
	int nEdges = pTVB->GetNumEdges(this);
//	CTM_Edge *pEdge = pTVB->GetEdges();
//	int nEdgeTris = pTVB->GetNumEdgeTris();
	CTM_EdgeTris *pEdgeTris = pTVB->GetEdgeTris(this);

	int nTriNeeded = nTri;

	for (int i = 0; i < RenderDataLen; ++i)
	{
		int iRealCluster = pRenderData[i].m_iShadowCluster;

		CTM_Cluster* pC = GetCluster(iRealCluster);

		pCalcCluster[i] = true;

		if (pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUME)
		{
			if(_pRenderParams->m_pCurAnimState)
			{
				if ((1 << (pC->m_OcclusionIndex)) & _pRenderParams->m_pCurAnimState->m_SurfaceShadowOcclusionMask) 
					pCalcCluster[i] = false;
				CXW_Surface* pSurf = m_lspSurfaces[pC->m_iSurface];
				if (pSurf->m_OcclusionMask & _pRenderParams->m_pCurAnimState->m_SurfaceShadowOcclusionMask) 
					pCalcCluster[i] = false;
			}
		}
		if (!pCalcCluster[i])
		{
			nTriNeeded -= pShadowTriangleClusterLen[i];
		}
	}



	bool bLightTransform = false;
	int nNeededPointers = (5 * _pRenderParams->m_RenderInfo.m_nLights) * RenderDataLen;
	int Needed = (nEdges*6 + nTriNeeded*6) * 2 * _pRenderParams->m_RenderInfo.m_nLights + nNeededPointers * sizeof(void *);

	Needed += sizeof(fp32); // Align

	if (pVBM->GetAvail() < Needed)
		return false;

	{
		uint16** pCurrentIndex = (uint16**)pVBM->Alloc(sizeof(uint16**) * nNeededPointers);
		if(!pCurrentIndex)
			return false;

		int nNeededScratchPad = nTri + 4 + nTriNeeded * sizeof(CVec3Dfp32);
		if(_pMat)
			nNeededScratchPad += nV * sizeof(CVec3Dfp32);
		uint8* pScratchPad = MRTC_ScratchPadManager::Get(nNeededScratchPad);
		uint8* pTriVis = pScratchPad; pScratchPad += (nTri + 3) & ~3;

		int bCalcNormal = true;
		CVec3Dfp32* pN = (CVec3Dfp32*)pScratchPad; pScratchPad += sizeof(CVec3Dfp32) * nTriNeeded;
		CVec3Dfp32* pV = (CVec3Dfp32*)pScratchPad; pScratchPad += sizeof(CVec3Dfp32) * nV;	// This pointer is only valid if _pMat is set

		for (int j = 0; j < RenderDataLen; ++j)
		{
			int iCluster = pRenderData[j].m_iShadowCluster;
			pIndexLists[iCluster] = pCurrentIndex;
			pCurrentIndex += _pRenderParams->m_RenderInfo.m_nLights;
			for (int i = 0; i < _pRenderParams->m_RenderInfo.m_nLights; ++i)
			{
				pIndexLists[iCluster][i] = (uint16 *)pCurrentIndex;
				*((uint32 *)pCurrentIndex) = 0; // We render 2 indexed triangle lists
				pCurrentIndex += 3; // We save 5 mint for every Light (NumLists) (NumInList) (List) (NumInList) (List)
			}
		}

		_pRenderParams->m_pRenderV = pV;

		if (_pMat)
		{
			// Todo, only transform the vertices that are needed for the unhidden clusters
			Cluster_TransformBones_V(_pRenderParams, pTVB, _pMat, _pRenderParams->m_Time);
		}
		else
		{
			bLightTransform = true;
			pV = pTVB->GetVertexPtr(this, 0);
		}
		//#define SHADOWINVERT

		CRC_Viewport* pVP = pVBM->Viewport_Get();
		fp32 ViewportFrontPlane = pVP->GetFrontPlane();
		fp32 ViewportBackPlane = pVP->GetBackPlane();

		for(int iL = 0; iL < _pRenderParams->m_RenderInfo.m_nLights; iL++)
		{
			const CXR_LightInfo& LightInfo = _pRenderParams->m_RenderInfo.m_pLightInfo[iL];
			const CXR_Light* pL = LightInfo.m_pLight;

			if (pL->m_LightGUID == _pRenderParams->m_pCurAnimState->m_ExcludeLightGUID && pL->m_LightGUID)		//  && pL->m_LightGUID is a safety fix in case a light doesn't have a proper GUID
				continue;
			if (pL->m_Type != CXR_LIGHTTYPE_POINT && pL->m_Type != CXR_LIGHTTYPE_SPOT)
			{
				continue;
			}

			if ((pL->m_Flags & CXR_LIGHT_NOSHADOWS))
				continue;

			// Shadow visible at all?
			if (!LightInfo.m_ShadowScissor.IsValid())
				continue;

			CVec3Dfp32 LightPos = pL->GetPosition();
			LightPos *= _pRenderParams->m_CurWorld2VertexMat;

			fp32 Range = pL->m_Range;
			CVec3Dfp32 PosV;
			if (_pRenderParams->m_RenderVB.m_pTransform)
				LightPos.MultiplyMatrix(*_pRenderParams->m_RenderVB.m_pTransform, PosV);
			else
				PosV = LightPos;


			if (PosV.k[2]+Range < ViewportFrontPlane)
				continue;
			if (PosV.k[2]-Range > ViewportBackPlane) 
				continue;

			fp32 BoundRadius = GetBoundRadius() * 2;
			if (_pRenderParams->m_RenderVB.m_pTransform)
			{
				// Make sure we don't project past back plane

				CVec3Dfp32 Pos = _pRenderParams->m_CurWorldPosition;
				Pos *= _pRenderParams->m_CurWorld2VertexMat;
				fp32 Distance = Pos.Distance(LightPos);
				CVec3Dfp32 PosV;
				Pos.MultiplyMatrix(*_pRenderParams->m_RenderVB.m_pTransform, PosV);
				fp32 WantedRange = (pL->m_Range + BoundRadius) - Distance;

				if (WantedRange + PosV.k[2] + BoundRadius > ViewportBackPlane) 
					WantedRange = ViewportBackPlane - (PosV.k[2] + BoundRadius);

				if (WantedRange < 0)
					continue;
			}


			CVec3Dfp32 Proj;
			Proj = pL->GetPosition();	
			if (bLightTransform)
				Proj *= _pRenderParams->m_CurWorld2LocalMat;

			uint iP1 = 0;

			// Tag triangles visible from the projection point
			if (bCalcNormal)
			{
				bCalcNormal = false;
				uint t = 0;
				uint t2 = 0;
				uint iCluster = 0;

				while(t < nTri)
				{
					uint Target = t + pShadowTriangleClusterLen[iCluster];
					if (!pCalcCluster[iCluster])
					{
						t = Target;
						++iCluster;
						continue;
					}
					M_ASSERT(Target <= nTri, "Error");
					uint nRealV = plnClusterVertices[iCluster];
					++iCluster;

					while (t < Target)
					{
						const CTM_Triangle &Tri = pTri[t];
						CVec3Dfp32 n = (pV[Tri.m_iV[0]] - pV[Tri.m_iV[1]]) 
							/ (pV[Tri.m_iV[2]] - pV[Tri.m_iV[1]]);

						pN[t2] = n;
						fp32 DotProd = (n * (Proj - pV[Tri.m_iV[1]]));

						if (DotProd	< 0.0f)
						{
							pTriVis[t] = 1;

							const CTM_Triangle &Tri = pShadowTriangle[t];
							M_ASSERT(Tri.m_iV[0] < nRealV, "Error");
							M_ASSERT(Tri.m_iV[1] < nRealV, "Error");
							M_ASSERT(Tri.m_iV[2] < nRealV, "Error");

							iP1	+= 6;
						}
						else
							pTriVis[t] = 0;
						++t;
						++t2;
					}
				}
				M_ASSERT(t == nTri, "Must be same");
			}
			else
			{
				uint t = 0;
				uint t2 = 0;
				uint iCluster = 0;

				while(t < nTri)
				{
					uint Target = t + pShadowTriangleClusterLen[iCluster];
					if (!pCalcCluster[iCluster])
					{
						t = Target;
						++iCluster;
						continue;
					}
					uint nRealV = plnClusterVertices[iCluster];
					++iCluster;

					while (t < Target)
					{
						if ((pN[t2] * (Proj - pV[pTri[t].m_iV[1]])) < 0.0f)
						{
							pTriVis[t] = 1;

							const CTM_Triangle &Tri = pShadowTriangle[t];
							M_ASSERT(Tri.m_iV[0] < nRealV, "Error");
							M_ASSERT(Tri.m_iV[1] < nRealV, "Error");
							M_ASSERT(Tri.m_iV[2] < nRealV, "Error");

							iP1	+= 6;
						}
						else
							pTriVis[t] = 0;
						++t;
						++t2;
					}
				}
				M_ASSERT(t == nTri, "Must be same");
			}

			// Sides
			uint iCluster = 0;
			uint e = 0;
			while(e < nEdges)
			{
				uint Target = e + pShadowEdgeClusterLen[iCluster];
				if (!pCalcCluster[iCluster])
				{
					e = Target;
					++iCluster;
					continue;
				}
				++iCluster;

				while (e < Target)
				{
					uint t0 = pEdgeTris[e].m_iTri[0];
					uint t1 = pEdgeTris[e].m_iTri[1];
					uint bT0 = (t0 != 0xffff) ? (pTriVis[t0]) : 0;
					uint bT1 = (t1 != 0xffff) ? (pTriVis[t1]) : 0;
					if (bT0 ^ bT1)
					{ 
						iP1	+= 6;
					}
					++e;
				}
			}

			// Time to do the actual primitive assemly
			uint16* pPrim = pVBM->Alloc_Int16(iP1);
			if(!pPrim)
				return false;
			iP1 = 0;
			{
				// Tri-cap
				uint t = 0;
				uint e = 0;

				uint iCluster = 0;
				while(t < nTri)
				{
					uint TriTarget = t + pShadowTriangleClusterLen[iCluster];
					uint EdgeTarget = e + pShadowEdgeClusterLen[iCluster];
					if (!pCalcCluster[iCluster])
					{
						t = TriTarget;
						e = EdgeTarget;
						++iCluster;
						continue;
					}

					uint iP1Start = iP1;
					uint iRealCluster = pRenderData[iCluster].m_iShadowCluster;
					(((mint *)(pIndexLists[iRealCluster][iL]))[0]) = 1;
					(((uint16 **)(pIndexLists[iRealCluster][iL]))[2]) = (pPrim + iP1);
					uint nRealClusterV = plnClusterVertices[iCluster];

					CTM_Cluster* pRealC = GetCluster(iRealCluster);
					uint iRealClusterOffset = pRealC->m_iVBVert;
					uint nRealVBV = plnVBVertices[pRealC->m_iVB];

					// Cap
					while (t < TriTarget)
					{
						if(pTriVis[t])
						{
							const CTM_Triangle &Tri = pShadowTriangle[t];
							M_ASSERT(Tri.m_iV[0] < nRealClusterV, "Error");
							M_ASSERT(Tri.m_iV[1] < nRealClusterV, "Error");
							M_ASSERT(Tri.m_iV[2] < nRealClusterV, "Error");

							pPrim[iP1++] = iRealClusterOffset + Tri.m_iV[0]+nRealVBV;
							pPrim[iP1++] = iRealClusterOffset + Tri.m_iV[1]+nRealVBV;
							pPrim[iP1++] = iRealClusterOffset + Tri.m_iV[2]+nRealVBV;
							pPrim[iP1++] = iRealClusterOffset + Tri.m_iV[0];
							pPrim[iP1++] = iRealClusterOffset + Tri.m_iV[2];
							pPrim[iP1++] = iRealClusterOffset + Tri.m_iV[1];
						}
						++t;
					}

					// Edges
					while (e < EdgeTarget)
					{
						uint t0 = pEdgeTris[e].m_iTri[0];
						uint t1 = pEdgeTris[e].m_iTri[1];
						uint bT0 = (t0 != 0xffff) ? (pTriVis[t0]) : 0;
						uint bT1 = (t1 != 0xffff) ? (pTriVis[t1]) : 0;
						if (bT0 ^ bT1)
						{ 
							uint iv0 = iRealClusterOffset + pShadowEdge[e].m_iV[0];
							uint iv1 = iRealClusterOffset + pShadowEdge[e].m_iV[1];

							if (bT0)
							{
								pPrim[iP1++] = iv0;
								pPrim[iP1++] = iv1;
								pPrim[iP1++] = iv1 + nRealVBV;
								pPrim[iP1++] = iv0;
								pPrim[iP1++] = iv1 + nRealVBV;
								pPrim[iP1++] = iv0 + nRealVBV;

							}
							else
							{
								pPrim[iP1++] = iv0;
								pPrim[iP1++] = iv1 + nRealVBV;
								pPrim[iP1++] = iv1;
								pPrim[iP1++] = iv0;
								pPrim[iP1++] = iv0 + nRealVBV;
								pPrim[iP1++] = iv1 + nRealVBV;
							}
						}
						++e;
					}

					++iCluster;

					// Save number of indices used
					(((mint *)(pIndexLists[iRealCluster][iL]))[1]) = (iP1 - iP1Start) / 3;
				}
				M_ASSERT(t == nTri, "Must be same");
			}
		}
	}

	return true;

#endif	// PLATFORM_PS2
}


bool CXR_Model_TriangleMesh::ShadowProjectHardware_ThreadSafe_VPU(CTriMesh_RenderInstanceParamters* _pRenderParams, const CMat4Dfp32* _pMat,mint _nMat)
{
#if defined(SHADOW_NAMEDEVENTS)
	M_NAMEDEVENT("SetupTriMeshShadowsVPU",0xff000055);
#endif
	MAUTOSTRIP(CXR_Model_TriangleMesh_ShadowProjectHardware_ThreadSafe_VPU, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_TriangleMesh::ShadowProjectHardware_ThreadSafe_VPU);

	if (!m_spShadowData)
		return true;

	CXR_VBManager* pVBM = _pRenderParams->m_pVBM;

	CTM_Cluster* pC = &m_spShadowData->m_Cluster;
	CTM_VertexBuffer *pTVB = &m_spShadowData->m_VertexBuffer;
	const CTM_Edge *pShadowEdge = m_spShadowData->m_lEdge.GetBasePtr();
	const int32 *pShadowEdgeClusterLen = m_spShadowData->m_lEdgeCluster.GetBasePtr();
	const CTM_Triangle *pShadowTriangle = m_spShadowData->m_lTriangle.GetBasePtr();
	const int32 *pShadowTriangleClusterLen = m_spShadowData->m_lTriangleCluster.GetBasePtr();
	const CTM_ShadowData::CRenderData *pRenderData = m_spShadowData->m_lRenderData.GetBasePtr();
	int RenderDataLen = m_spShadowData->m_lRenderData.Len();

	bool bNoShadows = true;
	for(int iL = 0; iL < _pRenderParams->m_RenderInfo.m_nLights; iL++)
	{
		if(_pRenderParams->m_plLightCulled[iL])
			continue;

		if(!(_pRenderParams->m_RenderInfo.m_pLightInfo[iL].m_pLight->m_Flags & CXR_LIGHT_NOSHADOWS))
		{
			bNoShadows = false;
			break;
		}
	}
	if(bNoShadows)
		return true;

	_pRenderParams->m_pppShadowPrimData = (uint16***)pVBM->Alloc(sizeof(uint16**) * GetNumClusters());  // Todo: merge all vb allocs
	_pRenderParams->m_pVpuShadowTaskId= 0xffffffff;
	if(!_pRenderParams->m_pppShadowPrimData)
		return false;


	// This model doesn't have any weights, ignore palette
	if(!pTVB->m_bHaveBones)
		_pMat = NULL;

	uint16*** pIndexLists = _pRenderParams->m_pppShadowPrimData;

	uint8 *pCalcCluster = (uint8 *)pVBM->Alloc(RenderDataLen);
	if(!pCalcCluster)
		return false;

	const int *plnClusterVertices = m_spShadowData->m_lnVertices.GetBasePtr();
	const int* plnVBVertices = m_spShadowData->m_lnVBVertices.GetBasePtr();
	if(!plnClusterVertices)
	{
		M_TRACEALWAYS(CStrF("WARNING: %s is not precached", m_FileName.Str()));
		m_spShadowData->m_lnVBVertices.SetLen(GetNumVertexBuffers());
		for(int i = 0; i < m_spShadowData->m_lnVBVertices.Len(); i++)
		{
			CTM_VertexBuffer* pVB = GetVertexBuffer(i);
			m_spShadowData->m_lnVBVertices[i] = pVB->GetNumVertices(this);
		}
		plnVBVertices = m_spShadowData->m_lnVBVertices.GetBasePtr();

		m_spShadowData->m_lnVertices.SetLen(m_spShadowData->m_lRenderData.Len());
		for (int i = 0; i < m_spShadowData->m_lnVertices.Len(); ++i)
		{
			CTM_Cluster* pC = GetCluster(m_spShadowData->m_lRenderData[i].m_iShadowCluster);
			int nVertices = pC->m_nVBVert;
			if(nVertices == 0)
			{
				// Old data format, separate VB for each cluster so GetNumTriangles on VB is reliable
				CTM_VertexBuffer *pVB = GetVertexBuffer(pC->m_iVB);
				nVertices = pVB->GetNumVertices(this);
			}
			m_spShadowData->m_lnVertices[i] = nVertices;
		}
		plnClusterVertices = m_spShadowData->m_lnVertices.GetBasePtr();
	}

	int nV = pTVB->GetNumVertices(this);

	int nTri = pTVB->GetNumTriangles(this);
	CTM_Triangle *pTri = (CTM_Triangle *)pTVB->GetTriangles(this);
	int nEdges = pTVB->GetNumEdges(this);
	//	CTM_Edge *pEdge = pTVB->GetEdges();
	//	int nEdgeTris = pTVB->GetNumEdgeTris();
	CTM_EdgeTris *pEdgeTris = pTVB->GetEdgeTris(this);

	int nTriNeeded = nTri;

	for (int i = 0; i < RenderDataLen; ++i)
	{
		int iRealCluster = pRenderData[i].m_iShadowCluster;

		CTM_Cluster* pC = GetCluster(iRealCluster);
		CTM_VertexBuffer *pTVB = GetVertexBuffer(pC->m_iVB);

		pCalcCluster[i] = true;

		if (pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUME)
		{
			if(_pRenderParams->m_pCurAnimState)
			{
				if ((1 << (pC->m_OcclusionIndex)) & _pRenderParams->m_pCurAnimState->m_SurfaceShadowOcclusionMask) 
					pCalcCluster[i] = false;
				CXW_Surface* pSurf = m_lspSurfaces[pC->m_iSurface];
				if (pSurf->m_OcclusionMask & _pRenderParams->m_pCurAnimState->m_SurfaceShadowOcclusionMask) 
					pCalcCluster[i] = false;
			}
		}
		if (!pCalcCluster[i])
		{
			nTriNeeded -= pShadowTriangleClusterLen[i];
		}
	}

	bool bLightTransform = false;
	int nNeededPointers = (5 * _pRenderParams->m_RenderInfo.m_nLights) * RenderDataLen;
	int Needed = (nEdges*6 + nTriNeeded*6) * 2 * _pRenderParams->m_RenderInfo.m_nLights + nNeededPointers * sizeof(void *);

	Needed += sizeof(fp32); // Align

	if (pVBM->GetAvail() < Needed)
		return false;

	{
		uint16** pCurrentIndex = (uint16**)pVBM->Alloc(sizeof(uint16**) * nNeededPointers);
		if(!pCurrentIndex)
			return false;

		for (int j = 0; j < RenderDataLen; ++j)
		{
			int iCluster = pRenderData[j].m_iShadowCluster;
			pIndexLists[iCluster] = pCurrentIndex;
			pCurrentIndex += _pRenderParams->m_RenderInfo.m_nLights;
			for (int i = 0; i < _pRenderParams->m_RenderInfo.m_nLights; ++i)
			{
				pIndexLists[iCluster][i] = (uint16 *)pCurrentIndex;
				*((uint32 *)pCurrentIndex) = 0; // We render 2 indexed triangle lists
				pCurrentIndex += 3; // We save 5 mint for every Light (NumLists) (NumInList) (List) (NumInList) (List)
			}
		}

		if (!_pMat)
			bLightTransform = true;

		CRC_Viewport* pVP = pVBM->Viewport_Get();
		fp32 ViewportFrontPlane = pVP->GetFrontPlane();
		fp32 ViewportBackPlane = pVP->GetBackPlane();

		CVPU_JobDefinition JobDef;
		uint32 LightCount=0;
		for(int iL = 0; iL < _pRenderParams->m_RenderInfo.m_nLights; iL++)
		{
			const CXR_LightInfo& LightInfo = _pRenderParams->m_RenderInfo.m_pLightInfo[iL];
			const CXR_Light* pL = LightInfo.m_pLight;

			if(_pRenderParams->m_plLightCulled[iL])
				continue;

			if (pL->m_LightGUID == _pRenderParams->m_pCurAnimState->m_ExcludeLightGUID && pL->m_LightGUID)		//  && pL->m_LightGUID is a safety fix in case a light doesn't have a proper GUID
				continue;
			if (pL->m_Type != CXR_LIGHTTYPE_POINT && pL->m_Type != CXR_LIGHTTYPE_SPOT)
				continue;

			if ((pL->m_Flags & CXR_LIGHT_NOSHADOWS))
				continue;

			// Shadow visible at all?
			if (!LightInfo.m_ShadowScissor.IsValid())
				continue;

			CVec3Dfp32 LightPos = pL->GetPosition();
			LightPos *= _pRenderParams->m_CurWorld2VertexMat;

			fp32 Range = pL->m_Range;
			CVec3Dfp32 PosV;
			if (_pRenderParams->m_RenderVB.m_pTransform)
				LightPos.MultiplyMatrix(*_pRenderParams->m_RenderVB.m_pTransform, PosV);
			else
				PosV = LightPos;


			if (PosV.k[2]+Range < ViewportFrontPlane)
				continue;
			if (PosV.k[2]-Range > ViewportBackPlane) 
				continue;

			fp32 BoundRadius = GetBoundRadius() * 2;
			if (_pRenderParams->m_RenderVB.m_pTransform)
			{
				// Make sure we don't project past back plane

				CVec3Dfp32 Pos = _pRenderParams->m_CurWorldPosition;
				Pos *= _pRenderParams->m_CurWorld2VertexMat;
				fp32 Distance = Pos.Distance(LightPos);
				CVec3Dfp32 PosV;
				Pos.MultiplyMatrix(*_pRenderParams->m_RenderVB.m_pTransform, PosV);
				fp32 WantedRange = (pL->m_Range + BoundRadius) - Distance;

				if (WantedRange + PosV.k[2] + BoundRadius > ViewportBackPlane) 
					WantedRange = ViewportBackPlane - (PosV.k[2] + BoundRadius);

				if (WantedRange < 0)
					continue;
			}

			CVec3Dfp32 Proj;
			Proj = pL->GetPosition();	
			if (bLightTransform)
				Proj *= _pRenderParams->m_CurWorld2LocalMat;
			union FI_Mix
			{
				float f;
				uint32 u;
			};
			FI_Mix niL;
			niL.u=iL;
			vec128 LightPosition= {Proj[0],Proj[1],Proj[2],niL.f};
			JobDef.AddVParam(LightCount+19,LightPosition); // Light position
			LightCount++;
		}
		if (LightCount)
		{
			JobDef.AddInStreamBuffer(0,pTri,nTri);
			JobDef.AddSimpleBuffer(4,(void*) pShadowTriangleClusterLen,RenderDataLen,VPU_IN_BUFFER);
			JobDef.AddSimpleBuffer(5,(void*) pCalcCluster,RenderDataLen,VPU_IN_BUFFER);
			JobDef.AddSimpleBuffer(6,(void*) pShadowEdgeClusterLen,RenderDataLen,VPU_IN_BUFFER);
			JobDef.AddInStreamBuffer(7,(void*) pEdgeTris,nEdges);
			{
				union PointersMix
				{
					mint m_mint;
					NThread::CSpinLock* m_SpinLock;
					uint8** m_ppuint8;
					mint* m_pmint;
					int* m_pint;
					void* m_pvoid;
					uint16*** m_pppuint16;
					CTM_Cluster* m_pCTM_Cluster;
				};
				PointersMix AllocPos,Lock,Heap,HeapSize,IndexLists,CTM_ClusterBasePtr, pnVBVerticies;
				AllocPos.m_mint=Lock.m_mint=Heap.m_mint=HeapSize.m_mint=0;
				CTM_ClusterBasePtr.m_pCTM_Cluster = m_lClusters.GetBasePtr();
				pnVBVerticies.m_pint=m_spShadowData->m_lnVBVertices.GetBasePtr();

				pVBM->GetAllocParams(AllocPos.m_pint,Lock.m_SpinLock,Heap.m_ppuint8,HeapSize.m_pmint);
				JobDef.AddPntParams(3,AllocPos.m_pvoid,Lock.m_pvoid);
				JobDef.AddPntParams(9,Heap.m_pvoid,HeapSize.m_pvoid);
				CVPU_ParamData ParamData0;
				ParamData0.m_PntrData[0] = CTM_ClusterBasePtr.m_mint;
				ParamData0.m_LongData[2] = sizeof(CTM_Cluster);
				ParamData0.m_LongData[3] = 0;
				JobDef.AddParamData(2,ParamData0);
				IndexLists.m_pppuint16=pIndexLists;
				CVPU_ParamData ParamData1;
				ParamData1.m_PntrData[0]=IndexLists.m_mint;
				ParamData1.m_LongData[2]=uint32(_pMat!=NULL);
				ParamData1.m_LongData[3]=LightCount;
				JobDef.AddParamData(11,ParamData1);
				CVPU_ParamData ParamData2;
				ParamData2.m_PntrData[0]=pnVBVerticies.m_mint;
				JobDef.AddParamData(18,ParamData2);
			}
			JobDef.AddSimpleBuffer(10,(void*) pRenderData,RenderDataLen,VPU_IN_BUFFER);

			JobDef.AddSimpleBuffer(12,(void*) plnClusterVertices,RenderDataLen,VPU_IN_BUFFER);
			JobDef.AddCacheBuffer(13,(void*) pShadowEdge,nEdges);
			JobDef.AddCacheBuffer(14,(void*) pShadowTriangle,nTri);

			if (_pMat)
			{
				const int iFrm0 = TruncToInt(_pRenderParams->m_Time.GetTimeModulus(pTVB->GetNumFrames()));
				const CVec3Dfp32* pV1 = pTVB->GetVertexPtr(this, iFrm0);
				JobDef.AddCacheBuffer(1,(void*) pV1,nV);
				const CTM_BDVertexInfo* pVI = pTVB->GetBDVertexInfo(this);
				const CTM_BDInfluence* pBI = pTVB->GetBDInfluence(this);
				const int nBDInfluences=pTVB->GetNumBDInfluence(this);
				JobDef.AddSimpleBuffer(15,(void*) _pMat,_nMat,VPU_IN_BUFFER);
				JobDef.AddCacheBuffer(16,(void*) pVI,nV);
				JobDef.AddCacheBuffer(17,(void*) pBI,nBDInfluences);
			}
			else
			{
				JobDef.AddSimpleBuffer(15,(void*) NULL,0,VPU_IN_BUFFER);
				JobDef.AddCacheBuffer(16,(void*) NULL,0);
				JobDef.AddCacheBuffer(17,(void*) NULL,0);
				JobDef.AddCacheBuffer(1,(void*) pTVB->GetVertexPtr(this, 0),nV);
			}
			JobDef.SetJob(MHASH2('SHAD','OW'));
			_pRenderParams->m_pVpuShadowTaskId=MRTC_ThreadPoolManager::VPU_AddTask(JobDef,VpuWorkersContext);
//		MRTC_ThreadPoolManager::VPU_BlockOnTask(_pRenderParams->m_pVpuShadowTaskId);
		}
	}
	return true;
}


// -------------------------------------------------------------------
void CXR_Model_TriangleMesh::Cluster_RenderProjection(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_VertexBuffer* _pTVB, const CVec3Dfp32& _ProjV, CXR_Light* _pL)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_RenderProjection, MAUTOSTRIP_VOID);
	int nV = _pTVB->GetNumVertices(this);
	CVec3Dfp32* pV = _pRenderParams->m_pRenderV;
	CVec3Dfp32* pVP = &_pRenderParams->m_pRenderV[nV];
	CXR_VBManager* pVBM = _pRenderParams->m_pVBM;

	if (_pL)
	{
		pV = pVBM->Alloc_V3(nV*2);
		if (!pV) return;
		pVP = &pV[nV];

		memcpy(pV, _pRenderParams->m_pRenderV, sizeof(CVec3Dfp32)*nV);

		Cluster_ProjectVertices(_pRenderParams, pV, pVP, nV, _ProjV, _pL->m_Range);
	}

	int nTri = _pTVB->GetNumTriangles(this);
	CTM_Triangle* pTri = (CTM_Triangle*)_pTVB->GetTriangles(this);
	int nEdges = _pTVB->GetNumEdges(this);

	CXR_VertexBuffer* pVB1 = pVBM->Alloc_VB(CXR_VB_ATTRIB);
	if (!pVB1) return;
	CXR_VertexBuffer* pVB2 = pVBM->Alloc_VB(CXR_VB_ATTRIB);
	if (!pVB2) return;
	CRC_Attributes* pA1 = pVB1->m_pAttrib;
	CRC_Attributes* pA2 = pVB2->m_pAttrib;
	pA1->m_iTexCoordSet[1] = 0; pA1->m_iTexCoordSet[2] = 1; pA1->m_iTexCoordSet[3] = 2;
	pA2->m_iTexCoordSet[1] = 0; pA2->m_iTexCoordSet[2] = 1; pA2->m_iTexCoordSet[3] = 2;


	if (!pVB1->AllocVBChain(pVBM, false))
		return;
	if (!pVB2->AllocVBChain(pVBM, false))
		return;
	pVB1->Geometry_VertexArray(pV, nV*2, true);
	pVB2->Geometry_VertexArray(pV, nV*2, true);

	uint16* pPrim1 = pVBM->Alloc_Int16(nEdges*2*3*2 + nTri*3*2);
	uint16* pPrim2 = pVBM->Alloc_Int16(nEdges*2*3*2 + nTri*3*2);
	if (!pPrim1 || !pPrim2) return;

	int iP1 = 0;

	uint8* pTriVis = (uint8*) pVBM->Alloc(nTri);
	if (!pTriVis) return;
	FillChar(pTriVis, nTri, 0);

	// Tag triangles visible from the projection point
	int nTVis = 0;
	{
		for(int t = 0; t < nTri; t++)
		{
			CTM_Triangle* pT = &pTri[t];
			CVec3Dfp32 n = (pV[pT->m_iV[0]] - pV[pT->m_iV[1]]) / (pV[pT->m_iV[2]] - pV[pT->m_iV[1]]);
			if ((n * (_ProjV - pV[pT->m_iV[1]])) < 0.0f)
			{
				pTriVis[t] = 1;
				nTVis++;

				pPrim1[iP1++] = pT->m_iV[0]+nV;
				pPrim1[iP1++] = pT->m_iV[1]+nV;
				pPrim1[iP1++] = pT->m_iV[2]+nV;
				pPrim1[iP1++] = pT->m_iV[0];
				pPrim1[iP1++] = pT->m_iV[2];
				pPrim1[iP1++] = pT->m_iV[1];

			}
		}
	}

//	int nPos = 0;
//	int nNeg = 0;

	CTM_Edge *pEdges = _pTVB->GetEdges(this);
	CTM_EdgeTris *pEdgeTris = _pTVB->GetEdgeTris(this);
	for(int e = 0; e < nEdges; e++)
	{
		CTM_Edge* pE = &pEdges[e];
		CTM_EdgeTris* pET = &pEdgeTris[e];
		int t0 = pET->m_iTri[0];
		int t1 = pET->m_iTri[1];
		int bT0 = (t0 != 0xffff) ? (pTriVis[t0]) : 0;
		int bT1 = (t1 != 0xffff) ? (pTriVis[t1]) : 0;
		if (bT0 ^ bT1)
		{ 
			int iv0 = pE->m_iV[0];
			int iv1 = pE->m_iV[1];
//			if (bT1) Swap(iv0, iv1);
			CVec3Dfp32 veye, e0, e1;
			_pRenderParams->m_CurViewVP.Sub(pV[iv1], veye);

			if (bT0)
			{
				pPrim1[iP1++] = iv0;
				pPrim1[iP1++] = iv1;
				pPrim1[iP1++] = iv1+nV;
				pPrim1[iP1++] = iv0;
				pPrim1[iP1++] = iv1+nV;
				pPrim1[iP1++] = iv0+nV;
			}
			else
			{
				pPrim1[iP1++] = iv0;
				pPrim1[iP1++] = iv1+nV;
				pPrim1[iP1++] = iv1;
				pPrim1[iP1++] = iv0;
				pPrim1[iP1++] = iv0+nV;
				pPrim1[iP1++] = iv1+nV;
			}
		}
	}

	if (iP1)
	{
		pA2->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_COLORWRITE);
		pA2->Attrib_Enable(CRC_FLAGS_CULL | CRC_FLAGS_STENCIL/* | CRC_FLAGS_BLEND*/ | CRC_FLAGS_CULLCW);
		pA2->Attrib_ZCompare(CRC_COMPARE_GREATER);
		pA2->Attrib_StencilRef(1, 255);
		pA2->Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_DEC);
		pA2->Attrib_StencilWriteMask(255);
		pVB2->Render_IndexedTriangles(pPrim1, iP1/3);
		pVB2->Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
		if (_pL)
			pVB2->m_Priority = _pL->m_iLight + 0.1f;
		else
			pVB2->m_Priority = _pRenderParams->m_RenderInfo.m_BasePriority_Opaque + 17.0f;
		pVBM->AddVB(pVB2);
	}

	if (iP1)
	{
		pA1->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_COLORWRITE | CRC_FLAGS_CULLCW);
		pA1->Attrib_Enable(CRC_FLAGS_CULL | CRC_FLAGS_STENCIL/* | CRC_FLAGS_BLEND*/);
		pA1->Attrib_ZCompare(CRC_COMPARE_GREATER);
		pA1->Attrib_StencilRef(1, 255);
		pA1->Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_INC);
		pA1->Attrib_StencilWriteMask(255);
		pVB1->Render_IndexedTriangles(pPrim1, iP1/3);
		pVB1->Matrix_Set(_pRenderParams->m_RenderVB.m_pTransform);
		if (_pL)
			pVB1->m_Priority = _pL->m_iLight + 0.2f;
		else
			pVB1->m_Priority = _pRenderParams->m_RenderInfo.m_BasePriority_Opaque + 17.5f;
		pVBM->AddVB(pVB1);
	}

//	ConOut(CStrF("VisTri %d, Front %d, Back %d, nV %d", nTVis, iP1 / 3, iP2 / 3, nV));
}
	
// -------------------------------------------------------------------
void CXR_Model_TriangleMesh::Cluster_Light(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_VertexBuffer* _pTVB, CPixel32* _pDiffuse, int _bOmni)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_Light, MAUTOSTRIP_VOID);
	int nv = _pTVB->GetNumVertices(this);
	if (_pRenderParams->m_bRenderFastLight)
	{
		for(int v = 0; v < nv; v++)
			_pDiffuse[v] = _pRenderParams->m_FastLight | 0xff000000;
	}
	else
	{
		CXR_WorldLightState::LightDiffuse(
			_pRenderParams->m_CurrentWLS.GetFirst(),
			nv, 
			_pRenderParams->m_pRenderV, 
			_pRenderParams->m_pRenderN,
			_bOmni,
			_pDiffuse,
			255, ((_pRenderParams->m_pCurrentEngine) ? _pRenderParams->m_pCurrentEngine->m_LightScale : 1.0f) * 1.75f);
	}
}

void CXR_Model_TriangleMesh::Cluster_Specular(CTriMesh_RenderInstanceParamters* _pRenderParams, CTM_VertexBuffer* _pTVB, CPixel32* _pSpec, int _Power)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Cluster_Specular, MAUTOSTRIP_VOID);
	int nv = _pTVB->GetNumVertices(this); 
	if (_pRenderParams->m_bRenderFastLight)
	{
		for(int v = 0; v < nv; v++)
			_pSpec[v] = 0xff000000;
	}
	else
	{
		CXR_WorldLightState::LightSpecular(
			_pRenderParams->m_CurrentWLS.GetFirst(),
			nv, 
			_pRenderParams->m_pRenderV, 
			_pRenderParams->m_pRenderN,
			_Power,
			_pSpec,
			_pRenderParams->m_CurViewVP, 255, (_pRenderParams->m_pCurrentEngine) ? _pRenderParams->m_pCurrentEngine->m_LightScale : 1.0f);
	}
}


#ifndef	PLATFORM_PS2
fp64 g_CTM_Time_Transform;
#endif	// PLATFORM_PS2


bool CXR_Model_TriangleMesh::CullCluster(CTM_Cluster* _pC, CTriMesh_RenderInstanceParamters* _pRP, spCXW_Surface* _lspSurfaces, uint _iSurf, uint _bDrawSolid)
{
	if (_pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUME)
	{
		if (_pRP->m_OnRenderFlags & CXR_MODEL_ONRENDERFLAGS_NOSHADOWS)
			return false;
		if (_pRP->m_RenderInfo.m_Flags & CXR_RENDERINFO_NOSHADOWVOLUMES)
			return false;
		if (!_pRP->m_bRender_Unified)
			return false;
		if (_pRP->m_pCurAnimState && (1 << (_pC->m_OcclusionIndex)) & _pRP->m_pCurAnimState->m_SurfaceShadowOcclusionMask) 
			return false;
		CXW_Surface* pSurf = _lspSurfaces[_iSurf];
		if (_pRP->m_pCurAnimState && pSurf->m_OcclusionMask & _pRP->m_pCurAnimState->m_SurfaceShadowOcclusionMask) 
			return false;
		if (_pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE && !_pRP->m_pppShadowPrimData)
			return false;
	}
	else
	{
		if (_pRP->m_OnRenderFlags & CXR_MODEL_ONRENDERFLAGS_INVISIBLE)
			return false;
		if (_pRP->m_RenderInfo.m_Flags & CXR_RENDERINFO_INVISIBLE)
			return false;
		if (_pRP->m_pCurAnimState && (1 << (_pC->m_OcclusionIndex)) & _pRP->m_pCurAnimState->m_SurfaceOcclusionMask) 
			return false;

		CXW_Surface* pSurf = _lspSurfaces[_iSurf];
		if (pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE) 
			return false;
		if (_pRP->m_pCurAnimState && pSurf->m_OcclusionMask & _pRP->m_pCurAnimState->m_SurfaceOcclusionMask) 
			return false;
		if (!_bDrawSolid && pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT) 
			return false;
	}

	return true;
}

// #define CTM_SHADOWVOLUMES
int ConvertWLSToCRCLight(CXR_WorldLightState& _WLS, CXR_VBManager& _VBM, CRC_Light*& _pLights, fp32 _LightScale);

void CXR_Model_TriangleMesh::OnRender_Scissors(class CTriMesh_RenderInstanceParamters* _pRenderParams)
{
	MSCOPESHORT(CXR_Model_TriangleMesh::OnRender_Scissors);
	{
		CXR_VBManager* pVBM = _pRenderParams->m_pVBM;
		CRC_Viewport* pVP = pVBM->Viewport_Get();
		fp32 ViewportFrontPlane = pVP->GetFrontPlane();
		fp32 ViewportBackPlane = pVP->GetBackPlane();
		CRct ViewRect = pVP->GetViewRect();
		int ScrW = ViewRect.GetWidth();
		int ScrH = ViewRect.GetHeight();
		CVec2Dfp32 Mid;
		Mid[0] = (ViewRect.p0.x + ViewRect.p1.x) >> 1;
		Mid[1] = (ViewRect.p0.y + ViewRect.p1.y) >> 1;
		// Calculate scissors
		CXR_LightOcclusionInfo* pLO = _pRenderParams->m_pLO;
		uint nMaxLights = _pRenderParams->m_nLOMaxLights;

		bool bDoScissor = true;
		uint bSolidShadowVolumes = (_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(17));
		if (bSolidShadowVolumes)
			bDoScissor = false;

		int bShadowVolumeSoftware = true;

		uint nLights = _pRenderParams->m_RenderInfo.m_nLights;
		for(uint iL = 0; iL < _pRenderParams->m_RenderInfo.m_nLights; iL++)
		{
			const CXR_LightInfo& LightInfo = _pRenderParams->m_RenderInfo.m_pLightInfo[iL];
			const CXR_Light* pL = LightInfo.m_pLight;

			_pRenderParams->m_plLightCulled[iL] = 1;

			if (pL->m_LightGUID == _pRenderParams->m_pCurAnimState->m_ExcludeLightGUID && pL->m_LightGUID)		//  && pL->m_LightGUID is a safety fix in case a light doesn't have a proper GUID
				continue;

			if (pL->m_Type != CXR_LIGHTTYPE_POINT && pL->m_Type != CXR_LIGHTTYPE_SPOT)
			{
				continue;
			}

			if ((_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(13)) ||
				(pL->m_LightGUID == _pRenderParams->m_pCurAnimState->m_NoShadowLightGUID && pL->m_LightGUID) ||		//  || !pL->m_LightGUID is a safety fix in case a light doesn't have a proper GUID
				(pL->m_Flags & CXR_LIGHT_NOSHADOWS))
				continue;

			CVec4Dfp32* pLightParams = &_pRenderParams->m_plLightParams[iL];
			CVec3Dfp32* pLightPos = (CVec3Dfp32*)pLightParams;
			*pLightPos = pL->GetPosition();
			*pLightPos *= _pRenderParams->m_CurWorld2VertexMat;
			{
				fp32 Range = pL->m_Range;
				CVec3Dfp32 PosV;
				if (_pRenderParams->m_RenderVB.m_pTransform)
					pLightPos->MultiplyMatrix(*_pRenderParams->m_RenderVB.m_pTransform, PosV);
				else
					PosV = *pLightPos;

				if (PosV.k[2]+Range < ViewportFrontPlane)
					continue;
				if (PosV.k[2]-Range > ViewportBackPlane) 
					continue;

				fp32 BoundRadius = GetBoundRadius() * 2;
				if (_pRenderParams->m_RenderVB.m_pTransform)
				{
					// Make sure we don't project past back plane

					CVec3Dfp32 Pos = _pRenderParams->m_CurWorldPosition;
					Pos *= _pRenderParams->m_CurWorld2VertexMat;
					fp32 Distance = Pos.Distance(*pLightPos);
					CVec3Dfp32 ObjectPosV;
					Pos.MultiplyMatrix(*_pRenderParams->m_RenderVB.m_pTransform, ObjectPosV);
					fp32 WantedRange = (pL->m_Range + BoundRadius) - Distance;

					if (WantedRange + ObjectPosV.k[2] + BoundRadius > ViewportBackPlane) 
						WantedRange = ViewportBackPlane - (ObjectPosV.k[2] + BoundRadius);

					if (WantedRange < 0)
						continue;

					pLightParams->k[3] = WantedRange;
				}
				else
				{
					CVec3Dfp32 Pos = _pRenderParams->m_CurWorldPosition;
					Pos *= _pRenderParams->m_CurWorld2VertexMat;
					fp32 Distance = Pos.Distance(*pLightPos);
					fp32 WantedRange = (pL->m_Range + BoundRadius) - Distance;
					pLightParams->k[3] = WantedRange;
				}

				CScissorRect Scissor = LightInfo.m_ShadowScissor;
				if ((Sqr(PosV[0]) + Sqr(PosV[2]) - Sqr(Range) > 0.1f) &&
					(Sqr(PosV[1]) + Sqr(PosV[2]) - Sqr(Range) > 0.1f) && 
					(PosV.k[2] > ViewportFrontPlane))
				{
					CVec2Dfp32 TangX0, TangX1, TangY0, TangY1;
					CalcCircleTangentPoints(PosV[0], PosV[2], Range, TangX0, TangX1);
					CalcCircleTangentPoints(PosV[0], PosV[2], Range, TangX0, TangX1);
					CalcCircleTangentPoints(PosV[1], PosV[2], Range, TangY0, TangY1);
					CalcCircleTangentPoints(PosV[1], PosV[2], Range, TangY0, TangY1);

					fp32 TangX02D = 0.5f * TangX0[0] * pVP->GetXScale() / TangX0[1] + Mid[0];
					fp32 TangX12D = 0.5f * TangX1[0] * pVP->GetXScale() / TangX1[1] + Mid[0];
					fp32 TangY02D = 0.5f * TangY0[0] * pVP->GetYScale() / TangY0[1] + Mid[1];
					fp32 TangY12D = 0.5f * TangY1[0] * pVP->GetYScale() / TangY1[1] + Mid[1];

					int TangXMin = RoundToInt(Min(TangX02D, TangX12D));
					int TangXMax = RoundToInt(Max(TangX02D, TangX12D));
					int TangYMin = RoundToInt(Min(TangY02D, TangY12D));
					int TangYMax = RoundToInt(Max(TangY02D, TangY12D));

	/*						fp32 x2df = 0.5f * PosV.k[0] * pVP->GetXScale() / PosV.k[2] + Mid[0];
					fp32 y2df = 0.5f * PosV.k[1] * pVP->GetYScale() / PosV.k[2] + Mid[1];
					int x2d = RoundToInt(x2df);
					int y2d = RoundToInt(y2df);
	ConOut(CStrF("Light proj (%d, %d), LightViewPos %s", x2d, y2d, PosV.GetString().Str()));

					Scissor.m_Min[0] = Min(ScrW-2, Max(0, x2d - 100));
					Scissor.m_Min[1] = Min(ScrH-2, Max(0, y2d - 100));
					Scissor.m_Max[0] = Max(0, Min(ScrW-1, x2d + 100));
					Scissor.m_Max[1] = Max(0, Min(ScrH-1, y2d + 100));*/

					CScissorRect LightScissor;
					LightScissor.SetRect(Min(ScrW-1, Max(0, TangXMin)), Min(ScrH-1, Max(0, TangYMin)), Max(0, Min(ScrW, TangXMax)), Max(0, Min(ScrH, TangYMax)));

	//						Scissor.And(LightInfo.m_Scissor);
					Scissor.And(LightScissor);

	//						if (_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(12))
	//							VBM_RenderRect_AlphaBlend(pVP, pVBM, Scissor, 0x2f008000, 100000);
				}

				{
					uint32 MinX, MinY, MaxX, MaxY;
					Scissor.GetRect(MinX, MinY, MaxX, MaxY);
					Scissor.SetRect(MinX, MinY, Max(MaxX, MinX + 1), Max(MaxY, MinY + 1));
				}

				if (_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(12))
					VBM_RenderRect_AlphaBlend(pVP, pVBM, Scissor, 0x1f008000, 100000);

				const uint16 iLight = pL->m_iLight;
				if (iLight < nMaxLights)
				{
					if (!_pRenderParams->m_LOUsage.Get(iL))
					{
						_pRenderParams->m_LOUsage.Set1(iL);
						_pRenderParams->m_piSrcLO[iL] = _pRenderParams->m_nLO++;
						_pRenderParams->m_piDstLO[iL] = iLight;
					}
					const uint16 index = _pRenderParams->m_piSrcLO[iL];
					pLO[index].m_ScissorShadow.Expand(Scissor);
				}

				_pRenderParams->m_plLightCulled[iL] = 0;
				_pRenderParams->m_lLightScissors[iL] = Scissor;


				{
					CRC_Attributes* pA = pVBM->Alloc_Attrib();
					if (!pA) return;

					pA->SetDefault();

	//				if (_pRenderParams->m_pCurrentEngine) 
	//					_pRenderParams->m_pCurrentEngine->SetDefaultAttrib(pA);

					pA->m_iTexCoordSet[1] = 0; pA->m_iTexCoordSet[2] = 1; pA->m_iTexCoordSet[3] = 2;

					int ColorWriteDisable = (_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(12)) ? 0 : CRC_FLAGS_COLORWRITE;

					if (bDoScissor)
					{
						pA->m_Scissor = Scissor;
		//				pA->m_Scissor.m_Min = 100;
		//				pA->m_Scissor.m_Max = 200;
						pA->m_Flags |= CRC_FLAGS_SCISSOR;
	//					if (pLO && pL->m_iLight < nMaxLights)
	//						pLO[pL->m_iLight].m_ScissorShadow.Expand(Scissor);
					}

					if (bSolidShadowVolumes)
					{
						pA->Attrib_Enable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_CULL);
						pA->Attrib_Disable(CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_STENCIL);
						pA->Attrib_RasterMode(CRC_RASTERMODE_NONE);
					}
					else
					{
						if ((bShadowVolumeSoftware != 0) ^ (_pRenderParams->m_bIsMirrored != 0))
							pA->Attrib_Enable(CRC_FLAGS_CULL | CRC_FLAGS_STENCIL);
						else
							pA->Attrib_Enable(CRC_FLAGS_CULL | CRC_FLAGS_CULLCW | CRC_FLAGS_STENCIL);

						pA->Attrib_Disable(ColorWriteDisable | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE);
						pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
					}

	//				pA->Attrib_PolygonOffset(0, 10);
					pA->Attrib_TexGenAttr(pLightParams->k);
				
					if (bShadowVolumeSoftware)
						pA->Attrib_TexGen(0, CRC_TEXGENMODE_SHADOWVOLUME2, CRC_TEXGENCOMP_ALL);
					else
						pA->Attrib_TexGen(0, CRC_TEXGENMODE_SHADOWVOLUME, CRC_TEXGENCOMP_ALL);

					pA->Attrib_ZCompare(CRC_COMPARE_GREATER);

					pA->Attrib_StencilWriteMask(255);

					if( ( _pRenderParams->m_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_SEPARATESTENCIL ) )
					{
						pA->Attrib_Enable( CRC_FLAGS_SEPARATESTENCIL );
						pA->Attrib_Disable( CRC_FLAGS_CULL | CRC_FLAGS_CULLCW );
						pA->Attrib_StencilRef( 1, 255 );
						{
							if (((bShadowVolumeSoftware) != 0) ^ (_pRenderParams->m_bIsMirrored != 0))
							{
								pA->Attrib_StencilBackOp( CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_INCWRAP );
								pA->Attrib_StencilFrontOp( CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_DECWRAP );
							}
							else
							{
								pA->Attrib_StencilBackOp( CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_DECWRAP );
								pA->Attrib_StencilFrontOp( CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_INCWRAP );
							}
						}
					}
					else
					{
						pA->Attrib_StencilRef(1, 255);
						pA->Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_INC);
					}
					_pRenderParams->m_lpShadowLightAttributes[iL] = pA;
					if( !( _pRenderParams->m_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_SEPARATESTENCIL ) )
					{
						CRC_Attributes* pA2 = pVBM->Alloc_Attrib();
						if (!pA2) return;
						pA2->SetDefault();
	//					if(_pRenderParams->m_pCurrentEngine) 
	//						_pRenderParams->m_pCurrentEngine->SetDefaultAttrib(pA2);

						pA2->m_iTexCoordSet[1] = 0; pA2->m_iTexCoordSet[2] = 1; pA2->m_iTexCoordSet[3] = 2;
						if (bSolidShadowVolumes)
						{
							pA2->Attrib_Enable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_CULLCW | CRC_FLAGS_CULL);
							pA2->Attrib_Disable(CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_STENCIL);
							pA2->Attrib_RasterMode(CRC_RASTERMODE_NONE);
						}
						else
						{
							if (((bShadowVolumeSoftware) != 0) ^ (_pRenderParams->m_bIsMirrored != 0))
								pA2->Attrib_Enable(CRC_FLAGS_CULL | CRC_FLAGS_CULLCW | CRC_FLAGS_STENCIL);
							else
								pA2->Attrib_Enable(CRC_FLAGS_CULL | CRC_FLAGS_STENCIL);
							pA2->Attrib_Disable(ColorWriteDisable | CRC_FLAGS_ALPHAWRITE | CRC_FLAGS_ZWRITE);
							pA2->Attrib_RasterMode(CRC_RASTERMODE_ADD);
						}

		//				pA2->Attrib_PolygonOffset(0, 10);
						pA2->Attrib_TexGenAttr(pLightParams->k);

						if (bShadowVolumeSoftware)
							pA2->Attrib_TexGen(0, CRC_TEXGENMODE_SHADOWVOLUME2, CRC_TEXGENCOMP_ALL);
						else
							pA2->Attrib_TexGen(0, CRC_TEXGENMODE_SHADOWVOLUME, CRC_TEXGENCOMP_ALL);

						pA2->Attrib_ZCompare(CRC_COMPARE_GREATER);

						pA2->Attrib_StencilRef(1, 255);
						pA2->Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_NONE, CRC_STENCILOP_NONE, CRC_STENCILOP_DEC);
						pA2->Attrib_StencilWriteMask(255);

						if (bDoScissor)
						{
							pA2->m_Scissor = Scissor;
			//				pA2->m_Scissor.m_Min = 100;
			//				pA2->m_Scissor.m_Max = 200;
							pA2->m_Flags |= CRC_FLAGS_SCISSOR;
						}

						_pRenderParams->m_lpShadowLightAttributesBackside[iL] = pA2;
					}
				}
			}
		}
	}
}

void CXR_Model_TriangleMesh::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
		const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	CMat4Dfp32 WVelMat;
	WVelMat.Unit();
	CXR_Model_TriangleMesh::OnRender2(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS, _pAnimState, _WMat, _VMat, WVelMat, _Flags);
}

void CXR_Model_TriangleMesh::OnRender2(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS, 
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, const CMat4Dfp32& _WVelMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_OnRender, MAUTOSTRIP_VOID);

	int SceneType = (_pEngine) ? _pEngine->m_SceneType : XR_SCENETYPE_MAIN;

	if (m_lspLOD.Len() && !(_Flags & CXR_MODEL_ONRENDERFLAGS_MAXLOD))
	{
		int nLOD = Min(m_lspLOD.Len(), m_lLODBias.Len());

		if (SceneType == XR_SCENETYPE_SHADOWDECAL)
		{
			int iShadowLOD = Max(0, nLOD-2);
			m_lspLOD[iShadowLOD]->m_RenderFlags = m_RenderFlags;
			m_lspLOD[iShadowLOD]->OnRender(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS, _pAnimState, _WMat, _VMat, _Flags | CXR_MODEL_ONRENDERFLAGS_SHADOW | CXR_MODEL_ONRENDERFLAGS_ISLOD);
			return;
		}

		CVec3Dfp32 Pos = CVec3Dfp32::GetMatrixRow(_WMat, 3);
		Pos *= _VMat;

		if (m_RenderFlags & CTM_RFLAGS_MINLOD || _Flags & CXR_MODEL_ONRENDERFLAGS_MINLOD)
		{
			m_lspLOD[nLOD-1]->m_RenderFlags = m_RenderFlags;
			m_lspLOD[nLOD-1]->OnRender(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS, _pAnimState, _WMat, _VMat, _Flags | CXR_MODEL_ONRENDERFLAGS_SHADOW | CXR_MODEL_ONRENDERFLAGS_ISLOD);
			return;
		}

//		fp32 Dist = CVec3Dfp32::GetMatrixRow(m_CurL2VMat, 3).LengthSqr();
		fp32 Dist = Pos.Length();

		if (_pEngine)
		{
			Dist += _pEngine->m_LODOffset;
			Dist *= _pEngine->m_LODScale;
		}
//		LogFile(CStrF("%f", Dist));
		//	ConOut(CStrF("Dist %f", Dist));
		for(int i = nLOD-1; i >= 0; i--)
		{
			if (Dist >= m_lLODBias[i])
			{
				m_lspLOD[i]->m_RenderFlags = m_RenderFlags;
				if (i == nLOD-1)
					m_lspLOD[i]->OnRender(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS, _pAnimState, _WMat, _VMat, _Flags | CXR_MODEL_ONRENDERFLAGS_SHADOW | CXR_MODEL_ONRENDERFLAGS_ISLOD);
				else
				{
#ifdef CTM_SHADOWVOLUMES
					m_lspLOD[nLOD-1]->m_RenderFlags = m_RenderFlags;
					m_lspLOD[nLOD-1]->OnRender(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS, _pAnimState, _WMat, _VMat, _Flags | CXR_MODEL_ONRENDERFLAGS_SHADOW | CXR_MODEL_ONRENDERFLAGS_NOSOLID | CXR_MODEL_ONRENDERFLAGS_ISLOD);
#endif
					m_lspLOD[i]->OnRender(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS, _pAnimState, _WMat, _VMat, _Flags | CXR_MODEL_ONRENDERFLAGS_ISLOD);
				}
				return;
			}
		}

#ifdef CTM_SHADOWVOLUMES
		m_lspLOD[nLOD-1]->m_RenderFlags = m_RenderFlags;
		m_lspLOD[nLOD-1]->OnRender(_pEngine, _pRender, _pVBM, _pViewClip, _spWLS, _pAnimState, _WMat, _VMat, _Flags | CXR_MODEL_ONRENDERFLAGS_SHADOW | CXR_MODEL_ONRENDERFLAGS_NOSOLID | CXR_MODEL_ONRENDERFLAGS_ISLOD);
#endif
	}
	else
	{
#ifdef CTM_SHADOWVOLUMES
		if (!(_Flags & CXR_MODEL_ONRENDERFLAGS_ISLOD))
			_Flags |= CXR_MODEL_ONRENDERFLAGS_SHADOW;
#endif
	}

	MSCOPE(CXR_Model_TriangleMesh::OnRender, MODEL_TRIMESH);

	if ((!_pEngine || !(_pEngine->m_EngineMode == XR_MODE_UNIFIED)) &&
		(_Flags & CXR_MODEL_ONRENDERFLAGS_INVISIBLE))
		return;

#ifdef CTM_SHADOWVOLUMES
	bool bShadowEnabled = (_pEngine) ? _pEngine->GetVar(XR_ENGINE_STENCILSHADOWS) != 0 : 0;
	bool bShadowVolume = (_Flags & CXR_MODEL_ONRENDERFLAGS_SHADOW) && bShadowEnabled && _pRender->Caps_StencilDepth();
#else
	bool bShadowVolume = (!_pEngine) ? false : _pEngine->m_EngineMode == XR_MODE_UNIFIED;
#endif

#if	ENABLE_SHADOWVOLUME
	#define	bSHADOWVOLUME	bShadowVolume
#else
	#define	bSHADOWVOLUME	0
#endif

	bool bDrawSolid = !(_Flags & CXR_MODEL_ONRENDERFLAGS_NOSOLID);
	bool bDrawFilled = false;
	if (SceneType == XR_SCENETYPE_SHADOWDECAL)
	{
		bDrawFilled = true;
		bDrawSolid = false;
		bShadowVolume = false;
	}


	// Anything to draw?
	if (!bSHADOWVOLUME && !bDrawSolid && !bDrawFilled) return;


	if (!_pAnimState) return;
	if (!_pVBM) return;

#ifdef SOME_PROFILING
	enum
	{
		PERF_VIEWCLIP,
		PERF_SCISSORS,
		PERF_SHADOWS,
		PERF_RENDERDECALS,
		PERF_RENDERCLUSTERS,
		MAX_PERF_TREE
	};

	uint64 M_ALIGN(128) lPerfData[MAX_PERF_TREE];
	memset(lPerfData, 0, sizeof(uint64) * MAX_PERF_TREE);
#endif

	CTriMesh_RenderInstanceParamters RenderParams(_pEngine, _pVBM, _pAnimState);
	ClearRenderPointers(&RenderParams);

	RenderParams.m_OnRenderFlags = _Flags;
	RenderParams.m_CurWorldPosition = CVec3Dfp32::GetMatrixRow(_WMat, 3);
	RenderParams.m_pVMat = &_VMat;

	_WMat.Multiply(_VMat, RenderParams.m_CurL2VMat);

	CMat4Dfp32 VMatInv;
	_VMat.InverseOrthogonal(VMatInv);
	RenderParams.m_CurViewVP = CVec3Dfp32::GetMatrixRow(VMatInv, 3);
	RenderParams.m_CurWorldVP = RenderParams.m_CurViewVP;
	RenderParams.m_pCurWVelMat = &_WVelMat;

	CBox3Dfp32 BoundBoxL;
	CBox3Dfp32 BoundBoxW;
	GetBound_Box(BoundBoxL, RenderParams.m_pCurAnimState);
	TransformAABB(_WMat, BoundBoxL, BoundBoxW);
	RenderParams.m_RenderInfo.Clear(RenderParams.m_pCurrentEngine);

	CVec3Dfp32 ObjWPos = CVec3Dfp32::GetMatrixRow(_WMat, 3);
	if (_pViewClip)
	{
#ifdef SOME_PROFILING
		CMTime Timer;
		Timer.Start();
#endif
		MSCOPESHORT(CXR_Model_TriangleMesh::_pViewClip);
		RenderParams.m_RenderInfo.m_pLightInfo = RenderParams.m_RenderLightInfo;
		RenderParams.m_RenderInfo.m_MaxLights = CTM_MAX_LIGHTS;
		RenderParams.m_RenderInfo.m_nLights = 0;

		int bClipRes = false;
		if (RenderParams.m_pCurAnimState->m_iObject)
		{
			_pViewClip->View_GetClip(RenderParams.m_pCurAnimState->m_iObject, &RenderParams.m_RenderInfo);
			bClipRes = (RenderParams.m_RenderInfo.m_Flags & CXR_RENDERINFO_INVISIBLE) == 0;
		}
		else

/*		if(_Flags & CXR_MODEL_ONRENDERFLAGS_NOCULL)
		{
			if (!_pViewClip->View_GetClip_Box(BoundBoxW.m_Min - CVec3Dfp32(16384, 16384, 16384), BoundBoxW.m_Max + CVec3Dfp32(16384, 16384, 16384), 0, 0, NULL, &RenderParams.m_RenderInfo))
				return;
		}
		else*/
		{
			if(RenderParams.m_pCurAnimState->m_pSkeletonInst)
			{
				// FIXME:
				// The culling system for skeletonanimated TriMeshes should be rewritten as follows:
				//
				// Each animation should be exported together with a model which should be used to evaluate
				// a skeletoninstance. From this a bounding-box should be calculated, and stored for every
				// keyframe in the animation. During gameplay, SkeletonInstance::EvalAnim should use these
				// keyframes to evaulate a final bounding-box for that particular animation-instance. This
				// bounding should be stored in the SkeletonInstance and in TriMesh send into the
				// View_GetClip_Box instead of the default bounding.

				bClipRes = _pViewClip->View_GetClip_Box(BoundBoxW.m_Min - CVec3Dfp32(32, 32, 0), BoundBoxW.m_Max + CVec3Dfp32(32, 32, 0), 0, 0, NULL, &RenderParams.m_RenderInfo);
			}
			else
				bClipRes = _pViewClip->View_GetClip_Box(BoundBoxW.m_Min, BoundBoxW.m_Max, 0, 0, NULL, &RenderParams.m_RenderInfo);
		}

#ifdef SOME_PROFILING
		Timer.Stop();
		lPerfData[PERF_VIEWCLIP] = Timer.GetCycles();
#endif

		if (!bClipRes && (!RenderParams.m_RenderInfo.m_nLights || !bSHADOWVOLUME) )
		{
#ifdef SOME_PROFILING
			uint64* pMem = (uint64*)_pVBM->Alloc(sizeof(uint64) * MAX_PERF_TREE);
			if(pMem)
			{
				memcpy(pMem, lPerfData, sizeof(uint64) * MAX_PERF_TREE);
				const_cast<CXR_AnimState*>(_pAnimState)->m_Data[3] = (aint)pMem;
			}
			else
				const_cast<CXR_AnimState*>(_pAnimState)->m_Data[3] = NULL;
#else
			const_cast<CXR_AnimState*>(_pAnimState)->m_Data[3] = NULL;
#endif
			return;
		}
	}

//	return;

	bool bHWAnim = false;
	RenderParams.m_bRenderTempTLEnable = false;
	RenderParams.m_bRender_ProjLight = false;

	if (RenderParams.m_pCurrentEngine)
	{
		RenderParams.m_pSceneFog = RenderParams.m_pCurrentEngine->m_pCurrentFogState;
		RenderParams.m_RenderSurfOptions = RenderParams.m_pCurrentEngine->m_SurfOptions;
		RenderParams.m_RenderSurfCaps = RenderParams.m_pCurrentEngine->m_SurfCaps;
		RenderParams.m_bRender_Unified = RenderParams.m_pCurrentEngine->m_EngineMode == XR_MODE_UNIFIED;

		bHWAnim = (RenderParams.m_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_MATRIXPALETTE) != 0;
		RenderParams.m_bRenderTempTLEnable = RenderParams.m_pCurrentEngine->m_bTLEnableEnabled;

		if(RenderParams.m_RenderInfo.m_Flags & CXR_RENDERINFO_NHF &&
			!((RenderParams.m_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20) ||
              (RenderParams.m_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_TEXGENMODE_BOXNHF)))
		{
			RenderParams.m_bRenderTempTLEnable = false;
		}
	}
	else
	{
		RenderParams.m_pSceneFog = NULL;
		RenderParams.m_RenderSurfOptions = 0;
		RenderParams.m_RenderSurfCaps = 0;
		RenderParams.m_bRender_Unified = 0;
	}


#ifndef	PLATFORM_PS2
	g_CTM_Time_Transform = 0;
#endif	// PLATFORM_PS2

	// // Turn off acceleration if it's a vertex animated model.
	if (RenderParams.m_bRenderTempTLEnable && (m_bVertexAnim))
	{
		RenderParams.m_bRenderTempTLEnable = false;
	}

	CXR_SkeletonInstance* pSkelInstance = NULL;
	if (RenderParams.m_pCurAnimState->m_pSkeletonInst) pSkelInstance = RenderParams.m_pCurAnimState->m_pSkeletonInst;

	bool bAnim = false;
	if(pSkelInstance) bAnim = true;

#ifdef TEMP_NO_SKELETON_ANIM
bAnim = false;
#endif

	// Turn off T&L if anim is not HW accellerated.
	if (bAnim && !bHWAnim)
		RenderParams.m_bRenderTempTLEnable = false;

	/*
#if defined( PLATFORM_PS2 ) && !defined( ENABLE_PS2_VERTEXBUFFERS )
	if(m_bRenderTempTLEnable)
		DecompressAll(CTM_CLUSTERCOMP_VB);
	DecompressAll(CTM_CLUSTERCOMP_VERTICES | CTM_CLUSTERCOMP_PRIMITIVES);
#else
	int ClusterComp = 
		(m_bRenderTempTLEnable) ? 
			CTM_CLUSTERCOMP_VB :
			CTM_CLUSTERCOMP_VERTICES | CTM_CLUSTERCOMP_PRIMITIVES;

	DecompressAll(ClusterComp);
#endif
	*/

	int iVariation = _pAnimState->m_Variation;
	if (iVariation >= m_lVariations.Len())
		iVariation = 0;

	if(m_lVariations.Len() == 0 || m_lClusterRefs.Len() == 0)
		return;

	const CTM_Variation& Variation = m_lVariations[iVariation];
	const CTM_ClusterRef* piClusterRefs = &m_lClusterRefs[Variation.m_iClusterRef];
	int nClusters = Variation.m_nClusters;

/*#ifndef M_RTM -- disabled, since this check isn't valid when having cloth on multiple models (no skeleton will have a complete trackmask)
                // (the solution would be to store a merged track mask in the skeleton instance, instead of just m_pDebugSkel)
	if(bAnim && pSkelInstance)
	{
		CXR_Skeleton* pSkel = pSkelInstance->m_pDebugSkel;
		if (pSkel)
		{
			for(int iiC = 0; iiC < nClusters; iiC++)
			{
				int iC = piClusterRefs[iiC].m_iCluster;
				CTM_VertexBuffer* pTVB = GetVertexBuffer(iC);
				int nMP = pTVB->GetNumBDMatrixMap();
				if (nMP)
				{
					const uint16* piM = pTVB->GetBDMatrixMap();
					for(int i = 0; i < nMP; i++)
					{
						uint16 iM = piM[i];
						if (iM >= pSkel->m_lNodes.Len())
							ConOut(CStrF("(CXR_Model_TriangleMesh::OnRender) WARNING: Matrix index out of range %d/%d", iM, pSkel->m_lNodes.Len() ));
						else
						{
							const CXR_SkeletonNode& Node = pSkel->m_lNodes[iM];
							if (Node.m_iRotationSlot >= 0 && !pSkel->m_TrackMask.IsEnabledRot(Node.m_iRotationSlot))
							{
								ConOut(CStrF("(CXR_Model_TriangleMesh::OnRender) WARNING: Rot track not enabled, Node %d, Track %d", iM, Node.m_iRotationSlot ));
							}
							if (Node.m_iMovementSlot >= 0 && !pSkel->m_TrackMask.IsEnabledMove(Node.m_iMovementSlot))
							{
								ConOut(CStrF("(CXR_Model_TriangleMesh::OnRender) WARNING: Move track not enabled, Node %d, Track %d", iM, Node.m_iMovementSlot));
							}
						}
					}
				}
			}
		}
	}
#endif*/


	CalcBoxScissor(RenderParams.m_pVBM->Viewport_Get(), &_VMat, BoundBoxW, RenderParams.m_RenderBoundScissor);

	if (pSkelInstance && bAnim && !m_bMatrixPalette)
	{
		bAnim = false;
	}

	CMat4Dfp32* pMatrixPalette = NULL;
	int nMatrixPalette = 0;
	uint16 SkeletonVpuTaskId = InvalidVpuTask;
	if (pSkelInstance)
	{
		MSCOPESHORT(CXR_Model_TriangleMesh::pSkelInstance);
		pMatrixPalette = pSkelInstance->GetBoneTransforms();
		nMatrixPalette = pSkelInstance->GetNumBones();
		SkeletonVpuTaskId = pSkelInstance->m_VpuTaskId;
		pSkelInstance->m_VpuTaskId=InvalidVpuTask;
	}


	RenderParams.m_Time = RenderParams.m_pCurAnimState->m_AnimTime0;
//	CVec3Dfp32 Proj;

	if (bDrawSolid || bSHADOWVOLUME)
	{
		OnRender_Lights(_spWLS,_WMat,_VMat,&RenderParams,BoundBoxW,bAnim,_Flags);
	}
	else
	{
	}

	CMat4Dfp32* pTransform = NULL;

	if (/*m_bRenderTempTLEnable ||*/
		!m_bVertexAnim && !bAnim)
		pTransform = RenderParams.m_pVBM->Alloc_M4(RenderParams.m_CurL2VMat);
	else
	{
		if (bAnim) 
			pTransform = RenderParams.m_pVBM->Alloc_M4(_VMat);
		else
		{
			CMat4Dfp32 Unit;
			Unit.Unit();
			pTransform = RenderParams.m_pVBM->Alloc_M4(Unit);
		}
	}

	if(!pTransform)
		return;

	RenderParams.m_RenderVB.Matrix_Set(pTransform);

	{
#ifdef SOME_PROFILING
		CMTime Timer;
		Timer.Start();
#endif
		OnRender_Scissors(&RenderParams);

#ifdef SOME_PROFILING
		Timer.Stop();
		lPerfData[PERF_SCISSORS] = Timer.GetCycles();
#endif
	}

	if (m_spShadowData && 
		RenderParams.m_bRender_Unified && 
		!(RenderParams.m_RenderInfo.m_Flags & CXR_RENDERINFO_NOSHADOWVOLUMES) &&
		!(RenderParams.m_OnRenderFlags & CXR_MODEL_ONRENDERFLAGS_NOSHADOWS) &&
		!(RenderParams.m_pCurrentEngine->m_DebugFlags & M_Bit(13)))
	{
#ifdef SOME_PROFILING
		CMTime Timer;
		Timer.Start();
#endif

		// Has to wait for cloth vpu before calculating shadows :(
		if (SkeletonVpuTaskId!=InvalidVpuTask)
		{
			CMTime T; T.Start();
			{
#if defined(VPU_NAMEDEVENTS)
				M_NAMEDEVENT("VPU_BlockOnTask", 0xffffc040);
#endif
				MRTC_ThreadPoolManager::VPU_BlockOnTask(SkeletonVpuTaskId,VpuWorkersContext);
			}
#if !defined(_DEBUG) && defined(M_Profile)
			T.Stop();
			fp32 t = T.GetTime();
			static bool bSpamMyOutputPrettyPlease = false;
			if (bSpamMyOutputPrettyPlease && t > 0.001f)
				M_TRACEALWAYS("(TriMesh->VPU_BlockOnTask) %f ms\n", t*1000.0f);
#endif
		}

		// Precalc shadows for all clusters
			
#if (defined(PLATFORM_PS3) || defined(PLATFORM_XENON)) && !defined(VBM_OVERWRITE_DEBUGGING)
/*
		CMTime timer;
		timer.Reset();
		timer.Start();

		if (!ShadowProjectHardware_ThreadSafe(&RenderParams, pMatrixPalette,nMatrixPalette))
			return;
		timer.Stop();
		float t1=timer.GetTime();
		timer.Reset();
		timer.Start();
*/
		if (!ShadowProjectHardware_ThreadSafe_VPU(&RenderParams, pMatrixPalette,nMatrixPalette))
			return;
/*
		timer.Stop();
		float t0=timer.GetTime();

		M_TRACEALWAYS("Time VPU: %f\n",t0);
		M_TRACEALWAYS("Time CPU: %f\n",t1);

		M_TRACEALWAYS("Ratio: %f\n",t0/t1);
*/
#else		 
/*
		CMTime timer;
		timer.Reset();
		timer.Start();
*/
		if (!ShadowProjectHardware_ThreadSafe(&RenderParams, pMatrixPalette,nMatrixPalette))
			return;
/*		timer.Stop();
		float t1=timer.GetTime();
		timer.Reset();
		timer.Start();

		if (!ShadowProjectHardware_ThreadSafe_VPU(&RenderParams, pMatrixPalette,nMatrixPalette))
			return;

		timer.Stop();
		float t0=timer.GetTime();

		M_TRACEALWAYS("Time VPU: %f\n",t0);
		M_TRACEALWAYS("Time CPU: %f\n",t1);

		M_TRACEALWAYS("Ratio: %f\n",t0/t1);
*/
#endif

#ifdef SOME_PROFILING
		Timer.Stop();
		lPerfData[PERF_SHADOWS] = Timer.GetCycles();
#endif
	}

//	if (RenderParams.m_bRender_Unified)	// ms_bRender_Unified cannot be set unless _pEngine is valid
//		RenderParams.m_pCurrentEngine->GetShader()->SetCurrentTransform(&RenderParams.m_CurVertex2WorldMat , &_VMat);

	if (/*!(_Flags & CXR_MODEL_ONRENDERFLAGS_ISLOD) && */RenderParams.m_pCurAnimState->m_GUID)
	{
#ifdef SOME_PROFILING
		CMTime Timer;
		Timer.Start();
#endif

		RenderDecals(&RenderParams, RenderParams.m_pCurAnimState->m_GUID, pMatrixPalette, nMatrixPalette);

#ifdef SOME_PROFILING
		Timer.Stop();
		lPerfData[PERF_RENDERDECALS] = Timer.GetCycles();
#endif
	}

	if (RenderParams.m_pCurrentEngine && RenderParams.m_pCurrentEngine->m_pViewClip)
	{
		const int nLights = RenderParams.m_RenderInfo.m_nLights;
		for(int i = 0; i < nLights; i++)
			RenderParams.m_pLO[i].Clear();
		RenderParams.m_nLOMaxLights= RenderParams.m_pCurrentEngine->m_pViewClip->View_Light_GetOcclusionSize();
		RenderParams.m_nLO= 0;
		RenderParams.m_LOUsage.Clear();
	}

	// Render clusters
	{
		MSCOPESHORT(CXR_Model_TriangleMesh::RenderClusters);
		uint8 M_ALIGN(128) DoneCluster[128];
	//	M_PREZERO128(0, DoneCluster);
		memset(DoneCluster, 0, 128);

		// Calculate new surface priority
		if (m_RenderFlags & CTM_RFLAGS_ORDERTRANS)
		{
			CVec3Dfp32 WCenter;
			BoundBoxW.GetCenter(WCenter);
			CPlane3Dfp32 FrontPlaneW = _pEngine->GetVC()->m_FrontPlaneW;
			fp32 BackPlaneInv = 1.0f / _pVBM->Viewport_Get()->GetBackPlane();

			fp32 TransparentZPriority = (CXR_VBPRIORITY_MODEL_OPAQUE + CXR_VBPRIORITY_MODEL_TRANSPARENT + 5);
			fp32 TransparentZRange = MinMT(CXR_VBPRIORITY_UNIFIED_MATERIAL - TransparentZPriority, 750);
			fp32 Dist = (-FrontPlaneW.Distance(WCenter) * TransparentZRange) * BackPlaneInv;
			
			RenderParams.m_PriorityOverride = ClampRange(Dist, TransparentZRange) + TransparentZPriority;
		}

#ifdef SOME_PROFILING
		CMTime Timer;
		Timer.Start();
#endif
		for (int iiC = 0; iiC < nClusters; iiC++)
		{
			if(DoneCluster[iiC])
				continue;

			int iC = piClusterRefs[iiC].m_iCluster;
			int iSurf = piClusterRefs[iiC].m_iSurface;

			CTM_Cluster* pC = GetCluster(iC);

			CTM_VertexBuffer* pTVB = GetVertexBuffer(pC->m_iVB);

			if(!CullCluster(pC, &RenderParams, m_lspSurfaces.GetBasePtr(), iSurf, bDrawSolid))
				continue;

			ClearRenderPointers(&RenderParams);
			RenderParams.m_RenderVB.Matrix_Set(pTransform);
	//		RenderParams.m_pRenderTV = pVB->GetTVertexPtr(0); // Removed by Mondelore.

			// Deformation?
			if ((pTVB->m_bHaveBones) && bAnim)
			{
				if (pMatrixPalette)
					if (!Cluster_SetMatrixPalette(&RenderParams, pC, pTVB, pMatrixPalette, nMatrixPalette,  &RenderParams.m_RenderVB, SkeletonVpuTaskId))
						break;

				if (!RenderParams.m_bRenderTempTLEnable)
				{
					int nV = pTVB->GetNumVertices(this);
					int nVAlloc = /*(bSHADOWVOLUME) ? 2*nV :*/ nV;
					RenderParams.m_pRenderV = RenderParams.m_pVBM->Alloc_V3(nVAlloc);
					RenderParams.m_pRenderN = RenderParams.m_pVBM->Alloc_V3(nVAlloc);
					RenderParams.m_pRenderTV = pTVB->GetTVertexPtr(this, 0); // Added by Mondelore.

		//			if (!RenderParams.m_pRenderV || !RenderParams.m_pRenderN) continue; // Removed by Mondelore.
					if (!RenderParams.m_pRenderV || !RenderParams.m_pRenderTV || !RenderParams.m_pRenderN) continue; // Added by Mondelore.

					if (RenderParams.m_bRender_Unified)
					{
						RenderParams.m_pRenderTangU = RenderParams.m_pVBM->Alloc_V3(nVAlloc);
						RenderParams.m_pRenderTangV = RenderParams.m_pVBM->Alloc_V3(nVAlloc);
						if (!RenderParams.m_pRenderTangU || !RenderParams.m_pRenderTangV) continue;
						Cluster_TransformBones_V_N_TgU_TgV(&RenderParams, pTVB, RenderParams.m_RenderVB.m_pMatrixPaletteArgs, RenderParams.m_Time);
					}
					else
						Cluster_TransformBones_V_N(&RenderParams, pTVB, RenderParams.m_RenderVB.m_pMatrixPaletteArgs, RenderParams.m_Time);

	//				if (bSHADOWVOLUME)
	//					Cluster_ProjectVertices(&RenderParams.m_pRenderV[0], &RenderParams.m_pRenderV[nV], nV, Proj, 500);

					RenderParams.m_RenderVB.m_pMatrixPaletteArgs = NULL;
				}
				else
				{
					uint nC = 0;
	//				if(!(pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE))
					{
						// Try to chain VB's
						uint ClusterFlags = pC->m_Flags;
						uint16* pBDI = pC->GetBDMatrixMap(this);
						for(uint iiC2 = iiC + 1; iiC2 < nClusters && (nC < CTM_MAX_MERGECLUSTERS); iiC2++)
						{
							if(DoneCluster[iiC2])
								continue;

							int iSurf2 = piClusterRefs[iiC2].m_iSurface;
							if(iSurf != iSurf2)
								continue;

							int iC2 = piClusterRefs[iiC2].m_iCluster;
							CTM_Cluster* pC2 = GetCluster(iC2);

							if((ClusterFlags ^ pC2->m_Flags) & CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE)
								continue;

							CTM_VertexBuffer* pVB2 = GetVertexBuffer(pC2->m_iVB);
							if(pTVB->m_bHaveBones != pVB2->m_bHaveBones)
								continue;

							uint16* pBDI2 = pC2->GetBDMatrixMap(this);
							if(pBDI != pBDI2)
								continue;

							// Cluster culling is pretty expensive, do the cheaper cullings first
							if(!CullCluster(pC2, &RenderParams, m_lspSurfaces.GetBasePtr(), iSurf2, bDrawSolid))
								continue;

							DoneCluster[iiC2] = 1;
							RenderParams.m_lpCluster[nC] = pC2;
							RenderParams.m_iCluster[nC] = iC2;
							nC++;
						}
					}
					RenderParams.m_nClusters = nC;
				}
			}
			else
			{
				// Transform
				if (!RenderParams.m_bRenderTempTLEnable )
				{
					if( (/*bSHADOWVOLUME ||*/ (m_bVertexAnim)))
					{
						int nV = pTVB->GetNumVertices(this);
						int nVAlloc = /*(bSHADOWVOLUME) ? 2*nV :*/ nV;
						RenderParams.m_pRenderV = RenderParams.m_pVBM->Alloc_V3(nVAlloc);
						RenderParams.m_pRenderTV = RenderParams.m_pVBM->Alloc_V2(nVAlloc); // Added by Mondelore.
						RenderParams.m_pRenderN = RenderParams.m_pVBM->Alloc_V3(nVAlloc);

		//				if (!RenderParams.m_pRenderV || !RenderParams.m_pRenderN) continue; // Removed by Mondelore.
						if (!RenderParams.m_pRenderV || !RenderParams.m_pRenderTV || !RenderParams.m_pRenderN) continue; // Added by Mondelore.

						if (RenderParams.m_bRender_Unified)
						{
							RenderParams.m_pRenderTangU = RenderParams.m_pVBM->Alloc_V3(nVAlloc);
							RenderParams.m_pRenderTangV = RenderParams.m_pVBM->Alloc_V3(nVAlloc);
							if (!RenderParams.m_pRenderTangU || !RenderParams.m_pRenderTangV) continue;
						}

						if (m_bVertexAnim)
						{
							if (RenderParams.m_bRender_Unified)
								Cluster_Transform_V_N_TgU_TgV(&RenderParams, pTVB, &RenderParams.m_CurL2VMat, &RenderParams.m_CurL2VMat, RenderParams.m_Time);
							else
								Cluster_Transform_V_N(&RenderParams, pTVB, &RenderParams.m_CurL2VMat, &RenderParams.m_CurL2VMat, RenderParams.m_Time);
						}
						else
						{
							memcpy(RenderParams.m_pRenderV, pTVB->GetVertexPtr(this, 0), nV*sizeof(CVec3Dfp32));
							memcpy(RenderParams.m_pRenderN, pTVB->GetNormalPtr(this, 0), nV*sizeof(CVec3Dfp32));
							if (RenderParams.m_pRenderTangU) memcpy(RenderParams.m_pRenderTangU, pTVB->GetTangentUPtr(this, 0), nV*sizeof(CVec3Dfp32));
							if (RenderParams.m_pRenderTangV) memcpy(RenderParams.m_pRenderTangV, pTVB->GetTangentVPtr(this, 0), nV*sizeof(CVec3Dfp32));
						}

	//					if (bSHADOWVOLUME)
	//						Cluster_ProjectVertices(&RenderParams.m_pRenderV[0], &RenderParams.m_pRenderV[nV], nV, Proj, 500);
					}
					else
					{
						RenderParams.m_pRenderV = pTVB->GetVertexPtr(this, 0);
						RenderParams.m_pRenderN = pTVB->GetNormalPtr(this, 0);
						RenderParams.m_pRenderTangU = pTVB->GetTangentUPtr(this, 0);
						RenderParams.m_pRenderTangV = pTVB->GetTangentVPtr(this, 0);
						RenderParams.m_pRenderTV = pTVB->GetTVertexPtr(this, 0); // Added by Mondelore.
					}
				}
				else
				{
					// Try to chain VB's
					uint nC = 0;
	//				if(!(pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE))
					{
						uint ClusterFlags = pC->m_Flags;
						uint16* pBDI = pC->GetBDMatrixMap(this);
						for(uint iiC2 = iiC + 1; iiC2 < nClusters && (nC < CTM_MAX_MERGECLUSTERS); iiC2++)
						{
							if(DoneCluster[iiC2])
								continue;

							int iSurf2 = piClusterRefs[iiC2].m_iSurface;
							if(iSurf != iSurf2)
								continue;

							int iC2 = piClusterRefs[iiC2].m_iCluster;
							CTM_Cluster* pC2 = GetCluster(iC2);
							CTM_VertexBuffer* pVB2 = GetVertexBuffer(pC2->m_iVB);
							if(pTVB->m_bHaveBones != pVB2->m_bHaveBones)
								continue;

							if((ClusterFlags ^ pC2->m_Flags) & CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE)
								continue;

							uint16* pBDI2 = pC2->GetBDMatrixMap(this);

							if(pBDI != pBDI2)
								continue;

							// Cluster culling is pretty expensive, do the cheaper cullings first
							if(!CullCluster(pC2, &RenderParams, m_lspSurfaces.GetBasePtr(), iSurf2, bDrawSolid))
								continue;

							DoneCluster[iiC2] = 1;
							RenderParams.m_lpCluster[nC] = pC2;
							RenderParams.m_iCluster[nC] = iC2;
							nC++;
						}
					}
					RenderParams.m_nClusters = nC;
				}
			}

			if (bDrawSolid)
			{

				RenderParams.m_lpSurfaces[0] = NULL;
				RenderParams.m_lpSurfaces[1] = NULL;

				CXW_Surface* pSurf = m_lspSurfaces[iSurf]->GetSurface(RenderParams.m_RenderSurfOptions, RenderParams.m_RenderSurfCaps);
				M_ASSERT(pSurf, "No surface!");
				uint8 iGroup = pSurf->m_iGroup;
				uint8 Mode = (RenderParams.m_OnRenderFlags >> (CXR_MODEL_ONRENDERFLAGS_SURFMODE_BASE + iGroup)) & 1;

				const CXR_AnimState* pAnimState = RenderParams.m_pCurAnimState;
				CXW_Surface* pUserSurface = pAnimState->m_lpSurfaces[iGroup];
				if (pUserSurface)
					pUserSurface = pUserSurface->GetSurface(RenderParams.m_RenderSurfOptions, RenderParams.m_RenderSurfCaps);

				int iSlot = 0;
				if (!pUserSurface || Mode == 1)
				{ // Setup regular surface
					int16 iAnim = pSurf->m_iController ? pAnimState->m_Anim1 : pAnimState->m_Anim0;
					const CMTime& AnimTime = pSurf->m_iController ? pAnimState->m_AnimTime1 : pAnimState->m_AnimTime0;

					RenderParams.m_lSurfaceTime[iSlot] = AnimTime;
					RenderParams.m_lpSurfaceKeyFrames[iSlot] = pSurf->GetFrame(iAnim, AnimTime, RenderParams.CreateTempSurfKeyFrame(iSlot));
					RenderParams.m_lpSurfaces[iSlot++] = pSurf;
				}

				if (pUserSurface)
				{ // Set up user-specified surface
					int16 iAnim = pUserSurface->m_iController ? pAnimState->m_Anim1 : pAnimState->m_Anim0;
					const CMTime& AnimTime = pUserSurface->m_iController ? pAnimState->m_AnimTime1 : pAnimState->m_AnimTime0;

					RenderParams.m_lSurfaceTime[iSlot] = AnimTime;
					RenderParams.m_lpSurfaceKeyFrames[iSlot] = pUserSurface->GetFrame(iAnim, AnimTime, RenderParams.CreateTempSurfKeyFrame(iSlot));
					RenderParams.m_lpSurfaces[iSlot++] = pUserSurface;
				}

				if (RenderParams.m_bRender_Unified)
				{
					Cluster_RenderUnified(&RenderParams, pC, iC);
				}
				else if( RenderParams.m_bRender_ProjLight )
					Cluster_RenderProjLight(&RenderParams, pC);
				else
					Cluster_Render(&RenderParams, pC);
			}

			if (bDrawFilled) Cluster_RenderSingleColor(&RenderParams, pC, 0xffffffff);

	/*		if (bSHADOWVOLUME) 
			{
	//			if (!pTVB->m_lTriEdges.Len()) 
	//				CreateEdges();
				Cluster_RenderProjection(pTVB, Proj);
			}*/
		}

		// Render trimesh as it would appear transparent if enabled
		if (m_RenderFlags & CTM_RFLAGS_ORDERTRANS && RenderParams.m_PrioCoverageMax >= RenderParams.m_PrioCoverageMin)
		{
			CBox3Dfp32 BoundBoxTransparent;
			GetBound_SolidSkeleton_Box(BoundBoxTransparent, 6.5f, RenderParams.m_pCurAnimState);
			
			CScissorRect RenderTransparentScissor;
			CalcBoxScissor(RenderParams.m_pVBM->Viewport_Get(), &_VMat, BoundBoxTransparent, RenderTransparentScissor);

			// Get scissor boxes
			uint32 MinX, MinY, MaxX, MaxY;
			RenderTransparentScissor.GetRect(MinX, MinY, MaxX, MaxY);
			CRct RenderBoundScissorRect(MinX, MinY, MaxX, MaxY);

			// Take a copy of the area where the tri mesh is going to be rendered
			int TextureID = _pEngine->m_TextureID_Screen;
			_pVBM->AddCopyToTexture(RenderParams.m_PrioCoverageMin - 0.000251f, RenderBoundScissorRect, RenderBoundScissorRect.p0, TextureID, true);
			
			// Restore screen after mesh has been rendered
			{
				CRect2Duint16 VPRect16(CVec2Duint16(MinX, MinY), CVec2Duint16(MaxX, MaxY));
				CVec2Dfp32 UVMin(fp32(MinX) * _pEngine->m_Screen_PixelUV[0], fp32(MinY) * _pEngine->m_Screen_PixelUV[1]);
				CVec2Dfp32 UVMax(fp32(MaxX) * _pEngine->m_Screen_PixelUV[0], fp32(MaxY) * _pEngine->m_Screen_PixelUV[1]);
				
				int bRenderTextureVertFlip  = _pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;
				CVec2Dfp32* pTV = (bRenderTextureVertFlip) ? CXR_Util::VBM_CreateRectUV_VFlip(_pVBM, UVMin, UVMax) :
															 CXR_Util::VBM_CreateRectUV(_pVBM, UVMin, UVMax);

				CMat4Dfp32* pMat2D = _pVBM->Alloc_M4_Proj2DRelBackPlane();

				// Render copied surface with destination alphablend to make trimeshes appear transparent
				if (pTV && pMat2D)
				{
					CRC_Attributes* pA = _pVBM->Alloc_Attrib();
					pA->SetDefault();
					pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
					pA->Attrib_RasterMode(CRC_RASTERMODE_DESTALPHABLEND);
					pA->Attrib_TextureID(0, TextureID);
					CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(_pVBM, pMat2D, VPRect16, 0xffffffff, RenderParams.m_PrioCoverageMax + 0.000201f, pA);
					if (pVB)
					{
						pVB->Geometry_TVertexArray(pTV, 0);
						_pVBM->AddVB(pVB);
					}
				}
			}
		}

#ifdef SOME_PROFILING
		Timer.Stop();
		lPerfData[PERF_RENDERCLUSTERS] = Timer.GetCycles();
#endif
	}

#ifdef M_Profile
	if( RenderParams.m_pCurrentEngine->m_DebugFlags & XR_DEBUGFLAGS_SHOWLIGHTCOUNT )
	{
		CVec3Dfp32 lV[CTM_MAX_LIGHTS*2];
		CPixel32 lC[CTM_MAX_LIGHTS*2];

		int nV = 0;
		for(int i = 0;i < RenderParams.m_RenderInfo.m_nLights;i++)
		{
			const CXR_Light * pL = RenderParams.m_RenderInfo.m_pLightInfo[i].m_pLight;

			if(pL->GetIntensitySqrf() < 0.0001f)
				continue;

			lV[nV] = _WMat.GetRow(3);
			lV[nV+1] = pL->GetPosition();
			lC[nV] = 0xff202000;
			lC[nV+1] = 0xffffff00;
			nV += 2;

//			RenderParams.m_pVBM->RenderWire(_VMat,_WMat.GetRow(3), pL->GetPosition(),CPixel32(255,255,255,255));
		}
		if (nV)
			RenderParams.m_pVBM->RenderWires(_VMat, lV, g_IndexRamp32, nV, lC, false);
	}
#endif

	if(!RenderParams.m_LOUsage.IsClear())
	{
		// Need to update ViewClip's LightOcclusionInfo with alterations made
		if (RenderParams.m_pCurrentEngine && RenderParams.m_pCurrentEngine->m_pViewClip)
			RenderParams.m_pCurrentEngine->m_pViewClip->View_Light_ApplyOcclusionArray(RenderParams.m_nLO, RenderParams.m_piDstLO, RenderParams.m_pLO);
	}

	//Render debug data (BoxTree)
	if( HasBoxTree() )
	{
		CTM_ShadowData::CBTNode * pBoxNodes = m_spShadowData->m_lBoxNodes.GetBasePtr();
	}

//	if (RenderParams.m_bRender_Unified)
//		RenderParams.m_pCurrentEngine->GetShader()->SetCurrentTransform(NULL, NULL);

#ifdef CTM_USELESS_SHADOWS
	for (iC = 0; iC < m_lspVertexBuffers.Len(); iC++)
	{
		MSCOPESHORT(CXR_Model_TriangleMesh::CTM_USELESS_SHADOWS);
		CTM_VertexBuffer* pTVB = m_lspVertexBuffers[iC];
		CXW_Surface* pSurf = m_lspSurfaces[pTVB->m_iSurface];
		if (pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE) continue;

		// Deformation?
		if (pTVB->m_lBDVertexInfo.Len() && bAnim)
		{
			Cluster_TransformBones_V_N(pTVB, BMat, t);
		}
		else
		{
			// Transform
			if (!(m_RenderFlags & CTM_RFLAGS_STATIC))
				Cluster_Transform_V_N(pTVB, &RenderParams.m_CurL2VMat, &RenderParams.m_CurL2VMat, t);
		}
		CVec3Dfp32 Proj = CVec3Dfp32(100, 100, 300) + CVec3Dfp32::GetMatrixRow(_WMat, 3);

		Proj *= LTransform;
		if (!pTVB->m_lTriEdges.Len()) CreateEdges();
		Cluster_ProjectVertices(pTVB, Proj, 500);
		Cluster_RenderProjection(pTVB, Proj);
	}
#endif

	if (RenderParams.m_pSceneFog && (RenderParams.m_RenderInfo.m_Flags & CXR_RENDERINFO_NHF))
	{
		MSCOPESHORT(CXR_Model_TriangleMesh::RenderParams.m_pSceneFog);
		RenderParams.m_pSceneFog->TraceBoundRelease();
	}
	
#if !defined(M_RTM) && !defined(PLATFORM_CONSOLE) 
	// -------------------------------------------------------------------
	//  Debug: Draw physics solids in wireframe.
	if ((m_RenderFlags & CTM_RFLAGS_SOLIDS) ||
		(RenderParams.m_pCurrentEngine && (RenderParams.m_pCurrentEngine->GetDebugFlags() & XR_DEBUGFLAGS_SHOWSOLIDS)))
	{
		MSCOPESHORT(CXR_Model_TriangleMesh::CTM_RFLAGS_SOLIDS);

		// Draw solids
		InitDebugSolids();
		for(int i = 0; i < m_lspDebugSolids.Len(); i++)
		{
			CSolid* pS = m_lspDebugSolids[i];
			
			if (!pS->GetNumPlanes())
				continue;


//			if (pS->m_BoundMin.Distance(pS->m_BoundMax) > 10000.0f) continue;
			
			int Sub = m_lSolids[i].m_iSurface % 10;
/*			int Base = m_lSolids[i].m_iSurface / 10;
			int Col;
			switch(Base)
			{
			case 0: Col = 0xff002020; break;
			case 1: Col = 0xff002080; break;
			case 2: Col = 0xff0080ff; break;
			case 3: Col = 0xff00ff80; break;
			case 4: Col = 0xff00ffff; break;
			default: Col = 0xffffffff; break;
			}*/
			int Col = 0xff808080;
			if(m_lSolids[i].m_iSurface < 150 && m_lSolids[i].m_iSurface > 0)
			{
				switch(Sub)
				{
				case 0: Col = 0xff800000; break;
				case 1: Col = 0xff008000; break;
				case 2: Col = 0xff000080; break;
				case 3: Col = 0xff808000; break;
				case 4: Col = 0xff008080; break;
				case 5: Col = 0xff800080; break;
				case 6: Col = 0xff808080; break;
				case 7: Col = 0xff400040; break;
				case 8: Col = 0xff404000; break;
				case 9: Col = 0xff004040; break;
				default: break;
				}
			}

			if (pSkelInstance)
			{
				int iNode = m_lSolids[i].m_iNode;
				if(pSkelInstance->m_nBoneTransform > iNode)
				{
					pS->RenderWire(RenderParams.m_pVBM, pSkelInstance->m_pBoneTransform[iNode], _VMat, Col);
				}
			}
			else
			{
				pS->RenderWire(RenderParams.m_pVBM, _WMat, _VMat, Col);
			}
		}

		for (int i = 0; i < m_lCapsules.Len(); i++)
		{
			const TCapsule<fp32> &capsule = m_lCapsules[i];

			int iNode = capsule.m_UserValue;
			if(pSkelInstance && pSkelInstance->m_nBoneTransform > iNode)
			{
				CMat4Dfp32 trans;
				trans.Unit();
				CVec3Dfp32 x = capsule.m_Point2 - capsule.m_Point1;
				x.Normalize();
				CVec3Dfp32 y = x + CVec3Dfp32(1,2,3);
				CVec3Dfp32 z = y / x;
				z.Normalize();
				y = x / z;
				y.Normalize();

				fp32 distance = capsule.m_Point1.Distance(capsule.m_Point2);

				CVec3Dfp32::GetRow(trans, 0) = x;
				CVec3Dfp32::GetRow(trans, 1) = y;
				CVec3Dfp32::GetRow(trans, 2) = z;
				CVec3Dfp32::GetRow(trans, 3) = (capsule.m_Point2 + capsule.m_Point1)*0.5;

				CMat4Dfp32 tmp,tmp2, transform1, transform2, transform3;

				tmp = pSkelInstance->m_pBoneTransform[iNode];
				trans.Multiply(tmp, tmp2);
				tmp2.Multiply(_VMat, transform1);

				CVec3Dfp32::GetRow(trans, 3) = capsule.m_Point1;
				trans.Multiply(tmp, tmp2);
				tmp2.Multiply(_VMat, transform2);

				CVec3Dfp32::GetRow(trans, 3) = capsule.m_Point2;
				trans.Multiply(tmp, tmp2);
				tmp2.Multiply(_VMat, transform3);

				CXR_VertexBuffer *pVBCyl = CXR_Util::Create_Cylinder(_pVBM, transform1, 0, distance, capsule.m_Radius, CPixel32(200,200,200), 8, 16);
				_pVBM->AddVB(pVBCyl);

				CXR_VertexBuffer *pVBSphere1 = CXR_Util::Create_Sphere(_pVBM, transform2, capsule.m_Radius, CPixel32(200,200,200));
				_pVBM->AddVB(pVBSphere1);

				CXR_VertexBuffer *pVBSphere2 = CXR_Util::Create_Sphere(_pVBM, transform3, capsule.m_Radius, CPixel32(200,200,200));
				_pVBM->AddVB(pVBSphere2);
			}
		}
	}

	// -------------------------------------------------------------------
#endif // PLATFORM_CONSOLE

#ifdef SOME_PROFILING
	uint64* pMem = (uint64*)_pVBM->Alloc(sizeof(uint64) * MAX_PERF_TREE);
	if (pMem)
		memcpy(pMem, lPerfData, sizeof(uint64) * MAX_PERF_TREE);
	const_cast<CXR_AnimState*>(_pAnimState)->m_Data[3] = (aint)pMem;
#else
	const_cast<CXR_AnimState*>(_pAnimState)->m_Data[3] = NULL;
#endif
	return;

//	LogFile(T_String("TriMesh ", Time_All) + T_String(", Skel ", Time_Skeleton - g_CTM_Time_Transform) + T_String(", Transform ", g_CTM_Time_Transform) + T_String(", Render ", Time_Render));
}

void CXR_Model_TriangleMesh::OnRender_Lights(CXR_WorldLightState* _pWLS, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat,
		class CTriMesh_RenderInstanceParamters* _pRenderParams,const CBox3Dfp32 & _BoundBoxW,bool _bAnim,int _Flags)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_OnRender_Lights, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_Model_TriangleMesh::OnRender_Lights);
	_pRenderParams->m_pRenderLights = NULL;
	_pRenderParams->m_nRenderLights = 0;

	// Figure out in what space we're doing lighting
	CMat4Dfp32 LTransform;
	if (_bAnim)
	{
		// Matrix palette
		LTransform.Unit();
	}
	else if (m_bVertexAnim)
	{
		// Vertex animated
		LTransform = _VMat;
	}
	else
	{
		// Static
		_WMat.InverseOrthogonal(LTransform);
	}

	int SurfCombFlags = 0;

	// Get all surface flags
	for(int iSurf = 0; iSurf < m_lspSurfaces.Len(); iSurf++)
		SurfCombFlags |= m_lspSurfaces[iSurf]->m_Flags;
//*
	if( (_Flags & CXR_MODEL_ONRENDERFLAGS_USEWLS) && (_pWLS) )
	{
		CXR_Light * pL = _pWLS->GetFirst();
		uint16 nL = 0;
		while( pL ) { pL = pL->m_pNext; nL++; };

		//Cap
		uint16 nCurrent = _pRenderParams->m_RenderInfo.m_nLights;
		uint16 nTotal = Min<uint16>(CTM_MAX_LIGHTS,nCurrent + nL);

		pL = _pWLS->GetFirst();
		CXR_Light* pLights = _pRenderParams->m_pVBM->Alloc_XRLights(nTotal - nCurrent);
		if (!pLights)
			return;

		for(int i = nCurrent; i < nTotal;i++)
		{
			CXR_LightInfo& LI = _pRenderParams->m_RenderLightInfo[i];
			CXR_Light* pNVLight = pLights + i - nCurrent;
			*pNVLight = *pL;
			pL->m_Pos.Multiply(LTransform,pNVLight->m_Pos);
			pL = pL->m_pNext;
			LI.m_pLight = pNVLight;
		}
		_pRenderParams->m_RenderInfo.m_nLights = nTotal;

		_pRenderParams->m_nRenderLights = nTotal;
		_pRenderParams->m_pRenderLights = pLights;

		_pRenderParams->m_RenderInfo.m_pLightInfo = _pRenderParams->m_RenderLightInfo;
	}
//*/

	if( _pRenderParams->m_pCurrentEngine)
	switch(_pRenderParams->m_pCurrentEngine->m_EngineMode)
	{
	case XR_MODE_LIGHTMAP :
		{
			_pRenderParams->m_CurrentWLS.Create(256, 4, 4);

			if (SurfCombFlags & (XW_SURFFLAGS_NEEDDIFFUSE | XW_SURFFLAGS_NEEDSPECULAR))
			{
				CVec3Dfp32 VCenter;
				_BoundBoxW.GetCenter(VCenter);

				// Get lights.
				if (_pWLS != NULL)
				{
					int MaxDynamic = (_pRenderParams->m_bRender_Unified) ? 4 : 2;
					_pRenderParams->m_CurrentWLS.CopyAndCull(_pWLS, GetBound_Sphere(), CVec3Dfp32::GetMatrixRow(_WMat, 3), 3, MaxDynamic);
				}
				else
					_pRenderParams->m_CurrentWLS.PrepareFrame();

				_pRenderParams->m_CurrentWLS.AddLightVolume(_pRenderParams->m_RenderInfo.m_pLightVolume, VCenter);
				CPixel32 PBNightVisionLight = (_pRenderParams->m_pCurrentEngine) ? _pRenderParams->m_pCurrentEngine->GetVar(XR_ENGINE_PBNIGHTVISIONLIGHT) : 0;
				if ((int)PBNightVisionLight)
				{
					CXR_Light L(0, CVec3Dfp32(PBNightVisionLight.GetR(), PBNightVisionLight.GetG(), PBNightVisionLight.GetB()), 0, 0, CXR_LIGHTTYPE_PARALLELL);
					L.SetDirection(CVec3Dfp32(0,0,1));
					_pRenderParams->m_CurrentWLS.AddStatic(L);
				}
				_pRenderParams->m_CurrentWLS.InitLinks();

				_pRenderParams->m_CurrentWLS.Optimize(CVec3Dfp32::GetMatrixRow(_WMat,3), GetBound_Sphere(), m_ParallellTresh, &LTransform);
				fp32 LightScale = ((_pRenderParams->m_pCurrentEngine) ? _pRenderParams->m_pCurrentEngine->m_LightScale : 1.0f) * 1.75f;
//				_pRenderParams->m_nRenderLights = ConvertWLSToCRCLight(_pRenderParams->m_CurrentWLS, *_pRenderParams->m_pVBM, _pRenderParams->m_pRenderLights, LightScale);

				{
					//Create local copies of CXR lights
					CXR_Light * pL = _pRenderParams->m_CurrentWLS.GetFirst();
					uint16 nL = 0;
					while( pL ) { pL = pL->m_pNext; nL++; };

					_pRenderParams->m_nRenderLights = nL;
					pL = _pRenderParams->m_CurrentWLS.GetFirst();
					CXR_Light * pLights = _pRenderParams->m_pVBM->Alloc_XRLights(nL);
					for(int i = 0; i < nL;i++)
					{
						CXR_Light* pNVLight = pLights + i;
						*pNVLight = *pL;
						pL = pL->m_pNext;
					}
					_pRenderParams->m_pRenderLights = pLights;
				}
			}
			else
				_pRenderParams->m_CurrentWLS.PrepareFrame();
		}
		break;

	case XR_MODE_UNIFIED :
		{
			// Add private light
			if (_pRenderParams->m_pCurAnimState->m_Data[3] != 0xffffffff && _pRenderParams->m_RenderInfo.m_nLights < CTM_MAX_LIGHTS)
			{
				int iLight = _pRenderParams->m_pCurrentEngine->m_pSceneGraphInstance->SceneGraph_Light_GetIndex(_pRenderParams->m_pCurAnimState->m_Data[3]);
				if (iLight)
				{
					CXR_LightInfo& LI = _pRenderParams->m_RenderLightInfo[_pRenderParams->m_RenderInfo.m_nLights];
					_pRenderParams->m_RenderInfo.m_nLights++;

					CXR_Light* pNVLight = _pRenderParams->m_pVBM->Alloc_XRLights(1);
					_pRenderParams->m_pCurrentEngine->m_pSceneGraphInstance->SceneGraph_Light_Get(iLight, *pNVLight);
					LI.m_pLight = pNVLight;
				}
			}

			_pRenderParams->m_plLightParams = _pRenderParams->m_pVBM->Alloc_V4(_pRenderParams->m_RenderInfo.m_nLights);

			if (_pRenderParams->m_RenderInfo.m_pLightVolume)
			{
				CVec3Dfp32 BoundV[8];
				_BoundBoxW.GetVertices(BoundV);
				_pRenderParams->m_RenderInfo.m_pLightVolume->Light_EvalPointArrayMax(BoundV, 8, _pRenderParams->m_lLightFieldAxes);

//				_pRenderParams->m_RenderInfo.m_pLightVolume->Light_EvalPoint(CVec3Dfp32::GetRow(_WMat, 3), _pRenderParams->m_lLightFieldAxes);
			}
		}
		break;

	case XR_MODE_UNIFIED+1 :
		{
			_pRenderParams->m_bRender_ProjLight = false;
			MSCOPESHORT( LightEval );
			// Add private light
			if (_pRenderParams->m_pCurAnimState->m_Data[3] != 0xffffffff && _pRenderParams->m_RenderInfo.m_nLights < CTM_MAX_LIGHTS)
			{
				int iLight = _pRenderParams->m_pCurrentEngine->m_pSceneGraphInstance->SceneGraph_Light_GetIndex(_pRenderParams->m_pCurAnimState->m_Data[3]);
				if (iLight)
				{
					CXR_LightInfo& LI = _pRenderParams->m_RenderLightInfo[_pRenderParams->m_RenderInfo.m_nLights];
					_pRenderParams->m_RenderInfo.m_nLights++;

					CXR_Light* pNVLight = _pRenderParams->m_pVBM->Alloc_XRLights(1);
					_pRenderParams->m_pCurrentEngine->m_pSceneGraphInstance->SceneGraph_Light_Get(iLight, *pNVLight);
					LI.m_pLight = pNVLight;
				}
			}

			// Convert lights to CRC_Light
			if( _pRenderParams->m_bRender_ProjLight == false )
			{
				uint8 aLightIndices[32];
				int nLights = FindMostContributingLights( 3, aLightIndices, _WMat, &_pRenderParams->m_RenderInfo );
				_pRenderParams->m_nRenderLights = nLights;
				_pRenderParams->m_pRenderLights = _pRenderParams->m_pVBM->Alloc_XRLights(nLights);

#ifdef	PLATFORM_PS2
				/*
				// Old RC -> XR light conversion. If PS2 is used again, this might need fixing.

				for(int iRenderLight = 0; iRenderLight < nLights; iRenderLight++)
				{
					const CXR_Light* pEngineLight = _pRenderParams->m_RenderInfo.m_pLightInfo[aLightIndices[iRenderLight]].m_pLight;
					CRC_Light* pRenderLight = &_pRenderParams->m_pRenderLights[iRenderLight];

					CVec3Dfp32 LightVec = pEngineLight->GetPosition() - CVec3Dfp32::GetRow( _WMat, 3 );
					LightVec.MultiplyMatrix3x3( LTransform );

					fp32 FallOffScaler = 0.0f;
					if( 0 && pEngineLight->m_Type == CXR_LIGHTTYPE_SPOT )
					{
						fp32 LightVecLen = LightVec.Length();
						fp32 DistScalar = ( pEngineLight->m_Range - LightVecLen ) * pEngineLight->m_RangeInv;
						DistScalar = M_Cos((1.0f-DistScalar)*_PIHALF);
						FallOffScaler = 255.0f * Clamp01( DistScalar * 128.0f / ( LightVecLen + 128 ) * 0.5f );
						//						FallOffScaler = 255.0f * Clamp01( 1.0f - ( LightVec.LengthSqr() * Sqr(pEngineLight->m_RangeInv) ) );
					}
					else
					{
						FallOffScaler = 255.0f * Clamp01( 1.0f - ( LightVec.LengthSqr() * Sqr(pEngineLight->m_RangeInv) ) );
					}

					pRenderLight->m_Color = CPixel32( pEngineLight->m_Intensity[0] * FallOffScaler, pEngineLight->m_Intensity[1] * FallOffScaler, pEngineLight->m_Intensity[2] * FallOffScaler );
					pRenderLight->m_Type = CRC_LIGHTTYPE_PARALLELL;
					pRenderLight->m_Ambient = 0;
					pRenderLight->m_Attenuation[0] = 0;
					pRenderLight->m_Attenuation[1] = pEngineLight->m_RangeInv;
					pRenderLight->m_Attenuation[2] = 0;

					pRenderLight->m_Direction = LightVec;
					pRenderLight->m_Direction.Normalize();
				}

				if( nLights == 0 )
				{
					CRC_Light* pLight = _pRenderParams->m_pVBM->Alloc_Lights(1);
					pLight->m_Color	= 0;
					pLight->m_Type = CRC_LIGHTTYPE_PARALLELL;
					_pRenderParams->m_pRenderLights	= pLight;
					_pRenderParams->m_nRenderLights	= 1;
					_pRenderParams->m_RenderInfo.m_nLights	= 1;
				}
				*/
#else
				for(int iRenderLight = 0; iRenderLight < nLights; iRenderLight++)
				{
					const CXR_Light* pL = _pRenderParams->m_RenderInfo.m_pLightInfo[iRenderLight].m_pLight;
					_pRenderParams->m_pRenderLights[iRenderLight] = *pL;
					pL->m_Pos.Multiply(LTransform,_pRenderParams->m_pRenderLights[iRenderLight].m_Pos);
					/*
					pL->GetPosition().MultiplyMatrix(LTransform, L.m_Pos);
					L.m_Color = pL->m_IntensityInt32;
					L.m_Type = CRC_LIGHTTYPE_POINT;
					L.m_Ambient = 0;
					L.m_Direction = 0;
					L.m_Attenuation[0] = 0;
					L.m_Attenuation[1] = pL->m_RangeInv;
					L.m_Attenuation[2] = 0;
					*/
				}
#endif
			}
			else
			{
				for( int iLight = 0; iLight < _pRenderParams->m_RenderInfo.m_nLights; iLight++ )
				{
					const CXR_Light* pEngineLight = _pRenderParams->m_RenderInfo.m_pLightInfo[iLight].m_pLight;

					{
						CRC_Attributes* pA= _pRenderParams->m_pVBM->Alloc_Attrib();
						_pRenderParams->m_pLightAttrib[iLight] = pA;
						if(!pA)
							continue;
						CPlane3Dfp32* pTexGenAttr = (CPlane3Dfp32*)_pRenderParams->m_pVBM->Alloc_fp32(4*4*2);
						if (!pTexGenAttr)
							continue;

						pA->SetDefault();
						pA->Attrib_TexGenAttr((fp32*)pTexGenAttr);

#define ATTR1 pA
#define TEX1 0
#define TEXATTR1(i) pTexGenAttr[i]
#define ATTR2 pA
#define TEX2 1
#define TEXATTR2(i) pTexGenAttr[2+i]

						pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
						pA->Attrib_Enable(CRC_FLAGS_CULL);
						pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
#ifndef PLATFORM_PS2
						pA->Attrib_ZCompare(CRC_COMPARE_EQUAL);
#endif

						CMat4Dfp32 LocalPos;
						pEngineLight->m_Pos.Multiply( LTransform, LocalPos );
						const CVec3Dfp32& LPos = CVec3Dfp32::GetRow( LocalPos, 3 );

						int nStages = 0;
						const fp32 Scale = 0.5f * pEngineLight->m_RangeInv;

						if( pEngineLight->m_Type == CXR_LIGHTTYPE_SPOT )
						{

#ifdef PLATFORM_PS2
							ATTR1->m_nChannels = TEX2+1;
							ATTR1->Attrib_TexEnvMode(TEX1, CRC_PS2_TEXENVMODE_REPLACE | CRC_PS2_TEXENVMODE_COLORBUFFER | CRC_PS2_TEXENVMODE_ALPHAMASK | CRC_PS2_TEXENVMODE_STENCILMASK | CRC_PS2_TEXENVMODE);
							ATTR2->Attrib_TexEnvMode(TEX2, CRC_PS2_TEXENVMODE_MULADD_DESTALPHA | CRC_PS2_TEXENVMODE_MODULATE | CRC_PS2_TEXENVMODE_COLORBUFFER | CRC_PS2_TEXENVMODE_RGBMASK | CRC_PS2_TEXENVMODE);

							ATTR2->Attrib_PolygonOffset(-10, -130); // Because of PS2 q-clipping, we need to offset the polys a bit..
#endif
							nStages = 2;

							const CVec3Dfp32& ProjX = CVec3Dfp32::GetRow(LocalPos, 0);
							CVec3Dfp32 ProjY; CVec3Dfp32::GetRow(LocalPos, 1).Scale(1.0f / pEngineLight->m_SpotWidth, ProjY);
							CVec3Dfp32 ProjZ; CVec3Dfp32::GetRow(LocalPos, 2).Scale(1.0f / pEngineLight->m_SpotHeight, ProjZ);

							// LDir Range
							ATTR1->Attrib_TextureID(TEX1, m_TextureID_AttenuationExp);
							ATTR1->Attrib_TexGen(TEX1, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
							TEXATTR1(0).CreateND(ProjX*Scale, 0.5f - (ProjX * LPos)*Scale);
							TEXATTR1(1).CreateND(CVec3Dfp32(0.0f, 0.0f, 0.0f), 0.5f);

							// Project Texture
							ATTR2->Attrib_TextureID(TEX2, m_TextureID_DefaultLens);
							ATTR2->Attrib_TexGen(TEX2, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V | CRC_TEXGENCOMP_W | CRC_TEXGENCOMP_Q);
							TEXATTR2(0).CreateND((ProjY+ProjX) * 0.5f, -((ProjY+ProjX) * LPos) * 0.5f);
							TEXATTR2(1).CreateND((ProjZ+ProjX) * 0.5f, -((ProjZ+ProjX) * LPos) * 0.5f);
							TEXATTR2(2).CreateND(CVec3Dfp32(0.0f, 0.0f, 0.0f), 1.0f);
							TEXATTR2(3).CreateND(ProjX, -(ProjX * LPos));
						}
						else if( pEngineLight->m_Type == CXR_LIGHTTYPE_POINT )
						{
#ifdef PLATFORM_PS2
							ATTR1->m_nChannels = TEX2+1;
							ATTR1->Attrib_TexEnvMode(TEX1, CRC_PS2_TEXENVMODE_REPLACE | CRC_PS2_TEXENVMODE_COLORBUFFER | CRC_PS2_TEXENVMODE_ALPHAMASK | CRC_PS2_TEXENVMODE_STENCILMASK | CRC_PS2_TEXENVMODE);
							ATTR2->Attrib_TexEnvMode(TEX2, CRC_PS2_TEXENVMODE_MULADD_DESTALPHA | CRC_PS2_TEXENVMODE_MODULATE | CRC_PS2_TEXENVMODE_COLORBUFFER | CRC_PS2_TEXENVMODE_RGBMASK | CRC_PS2_TEXENVMODE);
#endif
							nStages = 2;

							// XY Range
							ATTR1->Attrib_TextureID(TEX1, m_TextureID_AttenuationExp);
							ATTR1->Attrib_TexGen(TEX1, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
							TEXATTR1(0).CreateND(CVec3Dfp32(Scale, 0.0f, 0.0f), -Scale * LPos.k[0] + 0.5f);
							TEXATTR1(1).CreateND(CVec3Dfp32(0.0f, Scale, 0.0f), -Scale * LPos.k[1] + 0.5f);

							// Z Range
							ATTR2->Attrib_TextureID(TEX2, m_TextureID_AttenuationExp);
							ATTR2->Attrib_TexGen(TEX2, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_U | CRC_TEXGENCOMP_V);
							TEXATTR2(0).CreateND(CVec3Dfp32(0.0f, 0.0f, Scale), -Scale * LPos.k[2] + 0.5f);
							TEXATTR2(1).CreateND(CVec3Dfp32(0.0f, 0.0f, 0), 0.5f);
						}
					}
				}
			}
		}
		break;

	}

	// Make some matrices
	{
		_pRenderParams->m_CurViewVP *= LTransform;
		_pRenderParams->m_CurWorld2VertexMat = LTransform;
		LTransform.InverseOrthogonal(_pRenderParams->m_CurVertex2WorldMat);
		_WMat.InverseOrthogonal(_pRenderParams->m_CurWorld2LocalMat);
		_pRenderParams->m_CurVertex2WorldMat.Multiply(_pRenderParams->m_CurWorld2LocalMat, _pRenderParams->m_CurVertex2LocalMat);
	}

	if (_pRenderParams->m_pSceneFog && (_pRenderParams->m_RenderInfo.m_Flags & CXR_RENDERINFO_NHF))
	{
		_pRenderParams->m_pSceneFog->TraceBound(_BoundBoxW);
		if (_bAnim)
		{
			_pRenderParams->m_pSceneFog->SetTransform(NULL);
			_pRenderParams->m_RenderFogBox = _BoundBoxW;
			_pRenderParams->m_bRenderFogWorldSpace = true;
			_pRenderParams->m_pSceneFog->TraceBox(_BoundBoxW, _pRenderParams->m_RenderFog);
		}
		else
		{
			_pRenderParams->m_pSceneFog->SetTransform(&_WMat);
			_pRenderParams->m_RenderFogBox = m_BoundBox;
			_pRenderParams->m_bRenderFogWorldSpace = false;
			_pRenderParams->m_pSceneFog->TraceBox(m_BoundBox, _pRenderParams->m_RenderFog);
		}

		/*		int FogAnd = 255;
		int FogOr = 0;
		for(int i = 0; i < 8; i++)
		{
		FogOr |= m_RenderFog[i].GetA();
		FogAnd &= m_RenderFog[i].GetA();
		}

		if (FogAnd == 255)
		return;

		if (FogOr == 0)
		_pRenderParams->m_RenderInfo.m_Flags &= ~CXR_RENDERINFO_NHF;*/
	}

	// Initialize secondary surfaces
	int iSecSurf = 2;
	if (m_pSC)
	{
		for(int j = 0; j < TRIMESH_NUMSECONDARYSURF; j++)
		{
			if(m_iSecondarySurf[j] == -1)
				break;

			CXW_Surface* pSurf = m_pSC->GetSurface(m_iSecondarySurf[j]);
			if(pSurf)
			{
				pSurf = pSurf->GetSurface(_pRenderParams->m_RenderSurfOptions, _pRenderParams->m_RenderSurfCaps);
				CMTime AnimTime = (pSurf->m_iController) ? _pRenderParams->m_pCurAnimState->m_AnimTime1 : _pRenderParams->m_pCurAnimState->m_AnimTime0;
				_pRenderParams->m_lpSurfaces[iSecSurf] = pSurf;
				_pRenderParams->m_lSurfaceTime[iSecSurf] = AnimTime;
				if( pSurf->m_iController )
					_pRenderParams->m_lpSurfaceKeyFrames[iSecSurf] = pSurf->GetFrame( _pRenderParams->m_pCurAnimState->m_Anim1, AnimTime, _pRenderParams->CreateTempSurfKeyFrame(iSecSurf));
				else
					_pRenderParams->m_lpSurfaceKeyFrames[iSecSurf] = pSurf->GetFrame( _pRenderParams->m_pCurAnimState->m_Anim0, AnimTime, _pRenderParams->CreateTempSurfKeyFrame(iSecSurf));

				iSecSurf++;
			}
		}
	}
	for(int k = iSecSurf; k < TRIMESH_NUMSURF; k++)
		_pRenderParams->m_lpSurfaces[k] = NULL;

	// Init shadow-volumes projection direction?
	/*		if (bSHADOWVOLUME)
	{
	// Get first light-source, which should be the volume light-source.
	CXR_Light* pL = _pRenderParams->m_CurrentWLS.GetFirst();
	if (pL)
	{
	// A little hack to drag the shadows down towards the floor, basically to prevent the 
	// stencil-shadows to occationally get completely screwed-up
	Proj = pL->GetDirection();								// Model-space
	Proj.MultiplyMatrix3x3(_pRenderParams->m_CurVertex2WorldMat);		// --> World-space
	Proj.Normalize();
	CVec3Dfp32 Down(0,0,2);								// FIXME: This offset should be specified somewhere.
	Proj += Down;
	Proj.Normalize();
	Proj *= 400.0f;										// FIXME: This number, as well as the '500' number below, should be specified somewhere.
	Proj += CVec3Dfp32::GetMatrixRow(_WMat, 3);
	Proj *= LTransform;									// --> Back to model-space
	}
	else
	{
	// No light, use 'up'n to the left'  :)
	Proj = CVec3Dfp32(100, 100, 400) + CVec3Dfp32::GetMatrixRow(_WMat, 3);
	Proj *= LTransform;
	}
	}*/
}


// -------------------------------------------------------------------
void CXR_Model_TriangleMesh::OnPostPrecache(CXR_Engine* _pEngine)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_OnPostPrecache, MAUTOSTRIP_VOID);
	
/*	bool bNoFree = false;

	for( int iC = 0; iC < m_lspVertexBuffers.Len(); iC++ )
	{
		if( m_lspVertexBuffers[iC]->m_ClusterHibernateFlags & CTM_CLUSTERHIBERNATE_NODESTROY )
		{
			bNoFree = true;
			break;
		}
	}

	// Free compressed data
#ifdef PLATFORM_PS2
	if( bNoFree == false )
		DestroyCompressedClusters();
#endif

	for( int i = 0; i < m_lspLOD.Len(); i++ )
		m_lspLOD[i]->OnPostPrecache( _pEngine );*/
}
// -------------------------------------------------------------------
void CXR_Model_TriangleMesh::OnPrecache(CXR_Engine* _pEngine, int _iVariation)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_OnPrecache, MAUTOSTRIP_VOID);
	// URGENTFIXME: Uncompressed meshes test:
//	DecompressAll(-1);
//	m_lCompressedClusters.Destroy();
#	ifdef CTM_USEPRIMITIVES
		m_bUsePrimitives = true;
#	elif defined (PLATFORM_WIN_PC)

	m_bUsePrimitives = false;
	MACRO_GetSystemEnvironment(pEnv);
	if(!pEnv)
		Error("OpenStream", "No env");
	
	int bXDF = D_MXDFCREATE;
	int Platform = D_MPLATFORM;

	switch (Platform)
	{
	case 0: // PC
	case 3: // GC
		break;
	case 1: // Xbox
	case 2: // PS2
		m_bUsePrimitives = true;
		break;
	}

#	else
		m_bUsePrimitives = false;
#	endif
		m_bUsePrimitives = false;
	
	//DecompressAll(CTM_CLUSTERCOMP_VB);
	CreateVB();

	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC) Error("Read", "No texture-context available.");

	// Init surfaces
	uint iVariation = _iVariation;
	if (iVariation >= m_lVariations.Len())
		iVariation = 0;

	if(m_lVariations.Len() == 0 || m_lClusterRefs.Len() == 0)
		return;

	const CTM_Variation& Variation = m_lVariations[iVariation];
	const CTM_ClusterRef* piClusterRefs = &m_lClusterRefs[Variation.m_iClusterRef];
	int nClusters = Variation.m_nClusters;

	for(int iiC = 0; iiC < nClusters; iiC++)
	{
//		int iC = piClusterRefs[iiC].m_iCluster;
		int iSurf = piClusterRefs[iiC].m_iSurface;

		CXW_Surface* pS = m_lspSurfaces[iSurf];
		pS = pS->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
		if (!pS) continue;

		pS->InitTextures(pTC, false);	// Don't report failures.
		if (!(pS->m_Flags & XW_SURFFLAGS_INVISIBLE))
			pS->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	}

/*	{
		for(int iSurf = 0; iSurf < m_lspSurfaces.Len(); iSurf++)
		{
			CXW_Surface* pS = m_lspSurfaces[iSurf];
			pS = pS->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps);
			if (!pS) continue;

			pS->InitTextures(pTC, false);	// Don't report failures.
			if (!(pS->m_Flags & XW_SURFFLAGS_INVISIBLE))
				pS->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		}
	}*/

	// Set precache flags on all vertex buffers
	{
		CCFile ModelFile;
		{
			int nClusters = GetNumClusters();

			//VB flags
			m_lClusterFlags.SetLen(nClusters);

			for(int i = 0; i < nClusters; i++)
			{
				CTM_Cluster* pC = GetCluster(i);
				CTM_VertexBuffer* pTVB = GetVertexBuffer(pC->m_iVB);

				//VB flags
				m_lClusterFlags[i] = GetVertexBufferFlags(pTVB, &ModelFile);

#if		defined(PLATFORM_PS2)
				if( pC->m_Flags & (CTM_CLUSTERFLAGS_SHADOWVOLUME|CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE) )
					continue;

#elif	defined (PLATFORM_WIN_PC)
				if( ( pC->m_Flags & (CTM_CLUSTERFLAGS_SHADOWVOLUME|CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE) ) && ( bXDF && Platform == 2/*e_Platform_PS2*/ ) )
					continue;

#endif

//				pTVB->GetBDMatrixMap(this);
//				pTVB->m_bSave_BoneDeform = true;
//				pTVB->m_bSave_BoneDeformMatrixMap = true;
				int VBID = pTVB->m_VBID;
				if (VBID)
					_pEngine->m_pVBC->VB_SetFlags(VBID, _pEngine->m_pVBC->VB_GetFlags(VBID) | CXR_VBFLAGS_PRECACHE);

				CTM_VertexBuffer* pTIB = GetVertexBuffer(pC->m_iIB);
				int IBID = pTIB->m_VBID;
				if(IBID)
					_pEngine->m_pVBC->VB_SetFlags(IBID, _pEngine->m_pVBC->VB_GetFlags(IBID) | CXR_VBFLAGS_PRECACHE);
			}

#ifdef	PLATFORM_WIN_PC
			InitPreloader(_pEngine,bXDF,Platform);
#else
			InitPreloader(_pEngine,0,0);
#endif
		}


		if (m_spShadowData)
		{
			// Preload these 
			m_spShadowData->m_VertexBuffer.GetVertexPtr(this, 0,&ModelFile);
			m_spShadowData->m_VertexBuffer.GetEdges(this, &ModelFile);
			m_spShadowData->m_VertexBuffer.GetEdgeTris(this, &ModelFile);
			m_spShadowData->m_VertexBuffer.GetTriangles(this, &ModelFile);
			m_spShadowData->m_VertexBuffer.GetBDVertexInfo(this, &ModelFile);
			m_spShadowData->m_VertexBuffer.GetBDInfluence(this, &ModelFile);

			m_spShadowData->m_lnVertices.SetLen(m_spShadowData->m_lRenderData.Len());

			m_spShadowData->m_lnVBVertices.SetLen(GetNumVertexBuffers());
			for(int i = 0; i < m_spShadowData->m_lnVBVertices.Len(); i++)
			{
				CTM_VertexBuffer *pVB = GetVertexBuffer(i);
				int nVertices = pVB->GetNumVertices(this, &ModelFile);
				m_spShadowData->m_lnVBVertices[i] = nVertices;
			}

			for (int i = 0; i < m_spShadowData->m_lnVertices.Len(); ++i)
			{
				CTM_Cluster* pC = GetCluster(m_spShadowData->m_lRenderData[i].m_iShadowCluster);
				// Do "old style" first to make sure VB actually gets loaded atleast once
				CTM_VertexBuffer *pVB = GetVertexBuffer(pC->m_iVB);
				int nVertices = pVB->GetNumVertices(this, &ModelFile);
				if(pC->m_nVBVert)
				{
					// New version
					// Make sure incides gets loaded
					CTM_VertexBuffer *pIB = GetVertexBuffer(pC->m_iIB);
					pIB->GetTriangles(this, &ModelFile);
					nVertices = pC->m_nVBVert;
				}
				m_spShadowData->m_lnVertices[i] = nVertices;
			}
		}
	}

	// Do OnPrecache on all LODs.
	for(int i = 0; i < m_lspLOD.Len(); i++)
		m_lspLOD[i]->OnPrecache(_pEngine, _iVariation);

#ifdef PLATFORM_XBOX1
//	CreateEdges();
//	CreateTangents();
#endif

}

void CXR_Model_TriangleMesh::InitPreloader(CXR_Engine * _pEngine, int _bXDF, int _Platform)
{
	if (!_bXDF)
	{
		m_spPreloader = MNew(CPreloader);
		m_spPreloader->m_nVB = 0;
		m_spPreloader->m_spFile = MNew(CCFile);
	}

	int nVB = GetNumVertexBuffers();
	for(int i = 0; i < nVB; i++)
	{
		CTM_VertexBuffer* pVB = GetVertexBuffer(i);

#if		defined(PLATFORM_PS2)
		if( pVB->m_VBFlags & (CTM_CLUSTERFLAGS_SHADOWVOLUME|CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE) )
			continue;
#elif	defined (PLATFORM_WIN_PC)
		if( ( pVB->m_VBFlags & (CTM_CLUSTERFLAGS_SHADOWVOLUME|CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE) ) && ( _bXDF && _Platform == 2/*e_Platform_PS2*/ ) )
			continue;
#endif
		
		int VBID = pVB->m_VBID;
		if (VBID)
		{
			_pEngine->m_pVBC->VB_SetFlags(VBID, _pEngine->m_pVBC->VB_GetFlags(VBID) | CXR_VBFLAGS_PRECACHE);
			if (m_spPreloader)
				m_spPreloader->m_nVB++;
		}
	}
}

void CXR_Model_TriangleMesh::OnRefreshSurfaces()
{
	for(int s = 0; s < m_lspSurfaces.Len(); s++)
	{
		m_lspSurfaces[s]->Init();
		m_lspSurfaces[s]->InitTextures(false);
	}

	for(int l = 0; l < m_lspLOD.Len(); l++)
	{
		for(int s = 0; s < m_lspLOD[l]->m_lspSurfaces.Len(); s++)
		{
			m_lspLOD[l]->m_lspSurfaces[s]->Init();
			m_lspLOD[l]->m_lspSurfaces[s]->InitTextures(false);
		}
	}
}

void CXR_Model_TriangleMesh::OnResourceRefresh(int _Flags)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_OnResourceRefresh, MAUTOSTRIP_VOID);
// URGENTFIXME: Uncompressed meshes test:
//return;

/*	Cluster_AutoHibernate(5.0f);

	for(int i = 0; i < m_lspLOD.Len(); i++)
		m_lspLOD[i]->OnResourceRefresh(_Flags);*/
}
/*
void CXR_Model_TriangleMesh::OnHibernate(int _Flags)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_OnHibernate, MAUTOSTRIP_VOID);
	if (HaveCompressedClusters())
		Cluster_DestroyAll();

	for(int i = 0; i < m_lspLOD.Len(); i++)
		m_lspLOD[i]->OnHibernate(_Flags);
}
*/
/*
bool CXR_Model_TriangleMesh::DecompressAll(int _Flags)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_DecompressAll, false);
	MSCOPESHORT(CXR_Model_TriangleMesh::DecompressAll);
	if (CTriangleMeshCore::DecompressAll(_Flags))
	{
		CreateVB();
		return true;
	}

	return false;
}
*/
// -------------------------------------------------------------------
//  CXR_PhysicsModel stuff
// -------------------------------------------------------------------
void CXR_Model_TriangleMesh::Phys_Init(CXR_PhysicsContext* _pPhysContext)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Phys_Init, MAUTOSTRIP_VOID);
//	m_PhysWMat = _WMat;
//	_WMat.InverseOrthogonal(m_PhysWMatInv);
//	m_pPhysAnimState = _pAnimState;

#ifndef M_RT
	#ifndef PLATFORM_PS2
		/*
		// ***Le'ts draw the bbox before transforming ***
		if ((ms_pPhysRC) && (m_pPhysAnimState) && (m_pPhysAnimState->m_pSkeletonInst))
		{
			CVec3Dfp32 Min = m_BoundBox.m_Min;
			CVec3Dfp32 Max = m_BoundBox.m_Max;
			Min *= _pAnimState->m_pSkeletonInst->m_lBoneTransform[1];
			Max *= _pAnimState->m_pSkeletonInst->m_lBoneTransform[1];
			CVec3Dfp32 Diff = Max - Min;
			ms_pPhysRC->Render_Wire(Min,Min+CVec3Dfp32(Diff[0],0,0),0xff888800);
			ms_pPhysRC->Render_Wire(Min,Min+CVec3Dfp32(0,Diff[1],0),0xff888800);
			ms_pPhysRC->Render_Wire(Min,Min+CVec3Dfp32(0,0,Diff[2]),0xff888800);

			ms_pPhysRC->Render_Wire(Max,Max-CVec3Dfp32(Diff[0],0,0),0xff888800);
			ms_pPhysRC->Render_Wire(Max,Max-CVec3Dfp32(0,Diff[1],0),0xff888800);
			ms_pPhysRC->Render_Wire(Max,Max-CVec3Dfp32(0,0,Diff[2]),0xff888800);

			Max = Min+CVec3Dfp32(Diff[0],0,0);
			ms_pPhysRC->Render_Wire(Max,Max+CVec3Dfp32(0,Diff[1],0),0xff888800);
			ms_pPhysRC->Render_Wire(Max,Max+CVec3Dfp32(0,0,Diff[2]),0xff888800);

			Max = Min+CVec3Dfp32(0,Diff[1],0);
			ms_pPhysRC->Render_Wire(Max,Max+CVec3Dfp32(Diff[0],0,0),0xff888800);
			ms_pPhysRC->Render_Wire(Max,Max+CVec3Dfp32(0,0,Diff[2]),0xff888800);
			Max = Min+CVec3Dfp32(0,0,Diff[2]);
			ms_pPhysRC->Render_Wire(Max,Max+CVec3Dfp32(Diff[0],0,0),0xff888800);
			ms_pPhysRC->Render_Wire(Max,Max+CVec3Dfp32(0,Diff[1],0),0xff888800);
		}
		*/
		// ***
	#endif
#endif
}

void CXR_Model_TriangleMesh::Phys_GetBound_Sphere(const CMat4Dfp32& _Pos, CVec3Dfp32& _RetPos, fp32& _Radius)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Phys_GetBound_Sphere, MAUTOSTRIP_VOID);
	_Radius = GetBound_Sphere();
	_RetPos = CVec3Dfp32::GetMatrixRow(_Pos, 3);
}

void CXR_Model_TriangleMesh::Phys_GetBound_Box(const CMat4Dfp32& _Pos, CBox3Dfp32& _RetBox)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Phys_GetBound_Box, MAUTOSTRIP_VOID);

	m_BoundBox.Transform(_Pos, _RetBox);
}

bool CXR_Model_TriangleMesh::Phys_IntersectLine(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Phys_IntersectLine, false);
	int CollisionType = (_pCollisionInfo) ? _pCollisionInfo->m_CollisionType : CXR_COLLISIONTYPE_PHYSICS;

	if (CollisionType == CXR_COLLISIONTYPE_BOUND)
	{
		return Phys_IntersectLine_Bound(_pPhysContext, _v0, _v1, _MediumFlags, _pCollisionInfo);
	}
	else if (CollisionType == CXR_COLLISIONTYPE_PROJECTILE)
	{
		if (m_lSolids.Len())
		{
			// Not needed since this check has been done in the object's intersect line already.
		//	if (Phys_IntersectLine_Bound(_v0, _v1, _MediumFlags, NULL))
				return Phys_IntersectLine_Solids(_pPhysContext, _v0, _v1, _MediumFlags, _pCollisionInfo);
		}
		else if (HasBoxTree())
		{
			return Phys_IntersectLine_BoxTree(_pPhysContext, _v0, _v1, _MediumFlags, _pCollisionInfo);
		}
		else
			return Phys_IntersectLine_Bound(_pPhysContext, _v0, _v1, _MediumFlags, _pCollisionInfo);
	}
#ifndef PLATFORM_CONSOLE
	else if (CollisionType == CXR_COLLISIONTYPE_RAYTRACING)
	{
		if (Phys_IntersectLine_Bound(_pPhysContext, _v0, _v1, _MediumFlags, NULL))
			return Phys_IntersectLine_Triangles(_pPhysContext, _v0, _v1, _MediumFlags, _pCollisionInfo);
	}
#endif

	return false;
}


bool CXR_Model_TriangleMesh::Phys_IntersectLine_BoxTree(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, class CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Phys_IntersectLine_BoxTree, false);

	CVec3Dfp32 v0(_v0);
	CVec3Dfp32 v1(_v1);

	if( _pPhysContext && 
		_pPhysContext->m_pAnimState && 
		_pPhysContext->m_pAnimState->m_pSkeletonInst &&
		_pPhysContext->m_pAnimState->m_pSkeletonInst->m_nBoneTransform )
	{
		ConOutL(CStrF("ERROR: Boxtree collision testing is not supported on animated models '%s'", GetMeshName()));
		return false;
//		Error("Phys_IntersectLine_Boxtree", "Boxtree collision testing does not support animated models");
	}

	{
		v0 *= _pPhysContext->m_WMatInv;
		v1 *= _pPhysContext->m_WMatInv;
	}

	if( !_pCollisionInfo )
	{
		return m_spShadowData->IntersectLine(this, v0,v1,_pPhysContext->m_PhysGroupMaskThis);
	}

	CVec3Dfp32 HitPos;
	int iCls;
	if( m_spShadowData->IntersectLine(this, v0,v1,HitPos,_pCollisionInfo->m_Plane,_pPhysContext->m_PhysGroupMaskThis,&iCls) )
	{
		fp32 Time = v0.Distance(HitPos) / v1.Distance(v0);
		if( !_pCollisionInfo->IsImprovement(Time) )
			return true;

		_pCollisionInfo->m_Distance = v0.Distance(HitPos);
		_pCollisionInfo->m_Time = Time;
		_pCollisionInfo->m_Plane.Transform(_pPhysContext->m_WMat);
		
		// _pCollisionInfo->m_Pos = HitPos;
		_pCollisionInfo->m_LocalPos = HitPos;
		
		CVec3Dfp32 MondeloreHitPos = LERP(_v0,_v1,_pCollisionInfo->m_Time);
		_pCollisionInfo->m_Pos = MondeloreHitPos;

		_pCollisionInfo->m_pSurface = m_lspSurfaces[GetCluster(iCls)->m_iSurface];
		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_bIsCollision = true;

		_pCollisionInfo->SetReturnValues(CXR_COLLISIONRETURNVALUE_LOCALPOSITION|CXR_COLLISIONRETURNVALUE_POSITION|
			CXR_COLLISIONRETURNVALUE_TIME|CXR_COLLISIONRETURNVALUE_SURFACE);

		return true;
	}

	return false;
}


bool CXR_Model_TriangleMesh::Phys_IntersectLine_Bound(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{	
	MAUTOSTRIP(CXR_Model_TriangleMesh_Phys_IntersectLine_Bound, false);
//	if (!_pCollisionInfo) Error("Phys_IntersectLine", "Internal error.");

	CVec3Dfp32 v0(_v0);
	CVec3Dfp32 v1(_v1);
	CXR_SkeletonInstance* pSkelInstance = (_pPhysContext->m_pAnimState) ? _pPhysContext->m_pAnimState->m_pSkeletonInst : NULL;
	if (pSkelInstance && pSkelInstance->m_nBoneTransform)
	{
		CMat4Dfp32 MInv;
		pSkelInstance->m_pBoneTransform[10].InverseOrthogonal(MInv);
		v0 *= MInv;
		v1 *= MInv;
	}
	else
	{
		v0 *= _pPhysContext->m_WMatInv;
		v1 *= _pPhysContext->m_WMatInv;
	}

	COBBfp32 OBB(m_BoundBox);
	
	CVec3Dfp32 bv0,bv1;
	OBB.TransformToBoxSpace(v0, bv0);
	OBB.TransformToBoxSpace(v1, bv1);

	if (OBB.LocalPointInBox(bv0))
	{
		//ConOut("Source Inside MissedBox.");
		return true;
	}

	CVec3Dfp32 HitPos;
	if (OBB.LocalIntersectLine(bv0, bv1, HitPos))
	{
		if (!_pCollisionInfo)
			return true;

		fp32 Time = bv0.Distance(HitPos) / bv0.Distance(bv1);
		if (!_pCollisionInfo->IsImprovement(Time))
			return true;

		_pCollisionInfo->m_Distance = bv0.Distance(HitPos);
		_pCollisionInfo->m_Time = Time;
		_pCollisionInfo->m_LocalPos = HitPos;
		OBB.TransformFromBoxSpace(HitPos, _pCollisionInfo->m_Pos);
		CVec3Dfp32 MondeloreHitPos = LERP(_v0, _v1, _pCollisionInfo->m_Time);
		_pCollisionInfo->m_Pos = MondeloreHitPos;

		// FIXME: We probably don't need to return a plane so I'll fix this later. /mh
		// OBB.GetPlaneFromLocalPoint(_pCollisionInfo->m_LocalPos, _pCollisionInfo->m_Plane);
		_pCollisionInfo->m_pSurface = NULL;
		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_bIsCollision = true;
		return true;
	}

	return false;
}

#ifndef PLATFORM_CONSOLE

static void __Phys_RenderSolid(CWireContainer* _pWC, CSolid* _pSolid, const CMat4Dfp32& _WMat, int _Color)
{
	MAUTOSTRIP(__Phys_RenderSolid, MAUTOSTRIP_VOID);
	if (_pWC)
	{
		CMat4Dfp32 Unit; Unit.Unit();
		_pSolid->RenderWire(_pWC, _WMat, Unit, _Color);
	}
}
#endif // PLATFORM_CONSOLE

#define IsValidFloat(f) ((f) > -_FP32_MAX && (f) < _FP32_MAX)


bool CXR_Model_TriangleMesh::Phys_IntersectLine_Solids(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{	
	MAUTOSTRIP(CXR_Model_TriangleMesh_Phys_IntersectLine_Solids, false);
	CXR_SkeletonInstance* pSkelInstance = (_pPhysContext->m_pAnimState) ? _pPhysContext->m_pAnimState->m_pSkeletonInst : NULL;

	fp32 TraceLenRecp;
	if( _v0.AlmostEqual(_v1,0.0001f) )
		TraceLenRecp = _FP32_MAX;
	else
		TraceLenRecp = 1.0f / _v0.Distance(_v1);

	int bHit = false;
	if (pSkelInstance)
	{
		int nSolids = m_lSolids.Len();
		for(int i = 0; i < nSolids; i++)
		{
			CXR_ThinSolid* pS = &m_lSolids[i];
			int iNode = pS->m_iNode;

			if (iNode >= pSkelInstance->m_nBoneTransform)
				continue;
			CMat4Dfp32 M = pSkelInstance->m_pBoneTransform[iNode];

#ifndef M_RTM
			if (!IsValidFloat(M.k[0][0]))
			{
				M_TRACE("(CXR_Model_TriangleMesh::Phys_IntersectLine_Solids) Invalid bone transform!\n");
				continue;
			}
#endif

			CMat4Dfp32 MInv;
			M.InverseOrthogonal(MInv);
			CVec3Dfp32 v0(_v0);
			CVec3Dfp32 v1(_v1);
			v0 *= MInv;
			v1 *= MInv;
	//		if (pS->m_BoundMin.DistanceSqr(pS->m_BoundMax) > Sqr(10000.0f)) continue;

			fp32 Dist = pS->IntersectRay(v0, v1-v0) * TraceLenRecp;
			if (Dist < 0.0f || Dist > 1.0f) continue;
			if (!_pCollisionInfo) return true;

			if (_pCollisionInfo->IsImprovement(Dist))
//			if (!_pCollisionInfo->m_bIsValid || (_pCollisionInfo->m_bIsValid && Dist < _pCollisionInfo->m_Time))
			{
				v0.Lerp(v1, Dist, _pCollisionInfo->m_LocalPos);

				const CPlane3Dfp32* pPlanes = pS->m_lPlanes.GetBasePtr();
				int nPlanes = pS->m_lPlanes.Len();

				int MinPlane = -1;
				fp32 MinAbsDist = _FP32_MAX;
				for(int iP = 0; iP < nPlanes; iP++)
				{
					fp32 d = M_Fabs(pPlanes[iP].Distance(_pCollisionInfo->m_LocalPos));
					if (d < MinAbsDist)
					{
						MinAbsDist = d;
						MinPlane = iP;
					}
				}

				if(MinPlane == -1)
				{
#ifndef M_RTM
					ConOut(CStrF("(CXR_Model_TriangleMesh::Phys_IntersectLine_Solids) Invalid collision solid (%s)", m_MeshName.Str()));
#endif
					continue;
				}

				bHit = true;
				_pCollisionInfo->m_bIsValid = true;
				_pCollisionInfo->m_bIsCollision = true;
				_pCollisionInfo->m_Time = Dist;
				_v0.Lerp(_v1, Dist, _pCollisionInfo->m_Pos);
				_pCollisionInfo->m_Plane = pPlanes[MinPlane];
				_pCollisionInfo->m_Plane.Transform(M);
				_pCollisionInfo->m_LocalNode = iNode;
				_pCollisionInfo->m_LocalNodePos = MInv;
				_pCollisionInfo->m_SurfaceType = pS->m_iSurface;

#ifdef	PHYS_RENDER
				if (_pPhysContext->m_pWC && m_lspDebugSolids.Len())
					__Phys_RenderSolid(_pPhysContext->m_pWC, m_lspDebugSolids[i], M, 0xff804000);
#endif
			}
		}
	}
	else
	{

		CVec3Dfp32 v0(_v0);
		CVec3Dfp32 v1(_v1);
		v0 *= _pPhysContext->m_WMatInv;
		v1 *= _pPhysContext->m_WMatInv;

		int nSolids = m_lSolids.Len();
		for(int i = 0; i < nSolids; i++)
		{
			CXR_ThinSolid* pS = &m_lSolids[i];
	//		if (pS->m_BoundMin.DistanceSqr(pS->m_BoundMax) > Sqr(10000.0f)) continue;

			fp32 Dist = pS->IntersectRay(v0, v1-v0) * TraceLenRecp;
			if (Dist < 0.0f || Dist > 1.0f) continue;
			if (!_pCollisionInfo) return true;

			if (_pCollisionInfo->IsImprovement(Dist))
//			if (!_pCollisionInfo->m_bIsValid || (_pCollisionInfo->m_bIsValid && Dist < _pCollisionInfo->m_Time))
			{
				v0.Lerp(v1, Dist, _pCollisionInfo->m_LocalPos);

				const CPlane3Dfp32* pPlanes = pS->m_lPlanes.GetBasePtr();
				int nPlanes = pS->m_lPlanes.Len();

				int MinPlane = 0;
				fp32 MinAbsDist = _FP32_MAX;
				for(int iP = 0; iP < nPlanes; iP++)
				{
					fp32 d = M_Fabs(pPlanes[iP].Distance(_pCollisionInfo->m_LocalPos));
					if (d < MinAbsDist)
					{
						MinAbsDist = d;
						MinPlane = iP;
					}
				}

				bHit = true;
				_pCollisionInfo->m_bIsValid = true;
				_pCollisionInfo->m_bIsCollision = true;
				_pCollisionInfo->m_Time = Dist;
				_v0.Lerp(_v1, Dist, _pCollisionInfo->m_Pos);
				_pCollisionInfo->m_Plane = pPlanes[MinPlane];
				_pCollisionInfo->m_LocalNode = 0;
				_pCollisionInfo->m_LocalNodePos.Unit();
				_pCollisionInfo->m_SurfaceType = pS->m_iSurface;

#ifdef	PHYS_RENDER
				if (_pPhysContext->m_pWC && m_lspDebugSolids.Len())
					__Phys_RenderSolid(_pPhysContext->m_pWC, m_lspDebugSolids[i], _pPhysContext->m_WMat, 0xff800000);
#endif

			}
			else
			{
#ifdef	PHYS_RENDER
				if (_pPhysContext->m_pWC && m_lspDebugSolids.Len())
					__Phys_RenderSolid(_pPhysContext->m_pWC, m_lspDebugSolids[i], _pPhysContext->m_WMat, 0xff000020);
#endif
			}
		}
	}

/*	if (bHit) 
		ConOut(CStrF("(CXR_Model_TriangleMesh::Phys_IntersectLine) Hit T = %f", (_pCollisionInfo) ? _pCollisionInfo->m_Time : -1));
	else
		ConOut("(CXR_Model_TriangleMesh::Phys_IntersectLine) No hit  " + v0.GetString() + v1.GetString());
*/
	return bHit != 0;
}

#ifndef PLATFORM_CONSOLE

#define EPSILON 0.000001f

static bool IntersectTriangle(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, const CVec3Dfp32& _v2, CVec3Dfp32& _Pos, bool _bIntersectBackSide = true)
{
	MAUTOSTRIP(IntersectTriangle, false);
	CPlane3Dfp32 Pi4(_v0, _v1, _v2);
	if (Pi4.Distance(_p0) > 0.0f)
	{
		if (Pi4.Distance(_p1) > 0.0f) return false;

		CPlane3Dfp32 Pi1(_p0, _v1, _v0);
		if (Pi1.Distance(_p1) < 0.0f) return false;
		CPlane3Dfp32 Pi2(_p0, _v2, _v1);
		if (Pi2.Distance(_p1) < 0.0f) return false;
		CPlane3Dfp32 Pi3(_p0, _v0, _v2);
		if (Pi3.Distance(_p1) < 0.0f) return false;

		Pi4.GetIntersectionPoint(_p0, _p1, _Pos);
	}
	else
	{
		if (!_bIntersectBackSide) return false;

		if (Pi4.Distance(_p1) < 0.0f) return false;
	
		CPlane3Dfp32 Pi1(_p0, _v0, _v1);
		if (Pi1.Distance(_p1) < 0.0f) return false;
		CPlane3Dfp32 Pi2(_p0, _v1, _v2);
		if (Pi2.Distance(_p1) < 0.0f) return false;
		CPlane3Dfp32 Pi3(_p0, _v2, _v0);
		if (Pi3.Distance(_p1) < 0.0f) return false;

		Pi4.GetIntersectionPoint(_p0, _p1, _Pos);
	}

//if (_Pos.k[1] < 64 && _Pos.k[1] > -64 && _Pos.k[2] < 80)
//	LogFile("Hit " + _Pos.GetString() + ", Tri " + _v0.GetString()+ _v1.GetString()+ _v2.GetString());
	return true;
}

// Return true if hit, false otherwise
bool CXR_Model_TriangleMesh::Phys_IntersectLine_Triangles(CXR_PhysicsContext* _pPhysContext, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{	
	MAUTOSTRIP(CXR_Model_TriangleMesh_Phys_IntersectLine_Triangles, false);

	CCollisionInfo Closest;
	CVec3Dfp32 v0(_v0);
	CVec3Dfp32 v1(_v1);
	v0 *= _pPhysContext->m_WMatInv;
	v1 *= _pPhysContext->m_WMatInv;

	// Loop thru the clusters 
	int nClusters = GetNumClusters();
	for(int i = 0; i < nClusters; i++)
	{
		CTM_Cluster* pC = GetCluster(i);
		CTM_VertexBuffer* pTVB = GetVertexBuffer(pC->m_iVB);
	
		CVec3Dfp32 orig = v0;
		CVec3Dfp32 dir = v1 - v0;

		CVec2Dfp32 *pTV = pTVB->GetTVertexPtr(this, 0);
		CVec3Dfp32 *pV = pTVB->GetVertexPtr(this, 0);
		
		// Loop thru the triangles
		//JK-NOTE: Make sure this still works
		int nTriangles = pTVB->GetNumTriangles(this);
		uint16* piPrim = pTVB->GetTriangles(this);
		CTM_Triangle *pTri = (CTM_Triangle *)piPrim;
		if(pC->m_nIBPrim)
		{
			nTriangles = pC->m_nIBPrim / 3;
			pTri = (CTM_Triangle *)(piPrim + pC->m_iIBOffset);
		}
		for(int j = 0; j < nTriangles; j++)
		{
			// Get the vertices of frame 0 ignore the other anim frames
			CVec3Dfp32 vert0 = pV[pTri[j].m_iV[0]]; 
			CVec3Dfp32 vert1 = pV[pTri[j].m_iV[1]]; 
			CVec3Dfp32 vert2 = pV[pTri[j].m_iV[2]]; 
			
			// Find the two edges sharing vert0
			CVec3Dfp32 edge1 = vert1 - vert0;
			CVec3Dfp32 edge2 = vert2 - vert0;
			
			CVec2Dfp32 uvweight;

			if(Phys_TriangleIntersectRay(orig, dir, vert0, edge1, edge2, uvweight, _pCollisionInfo))
			{
				// Below code crashed, so alpha check is now disabled.
				return true;

				// Calculate UV so we can check if the point is transparent
				// NOTE: Could probably be optimized a bit by precalculating some values (edges, etc)
				fp32 w1, w2, w3;
				CVec2Dfp32 UV;

				// Get the texture coordinates for the vertices
				CVec2Dfp32 uv0 = pTV[pTri[j].m_iV[0]];
				CVec2Dfp32 uv1 = pTV[pTri[j].m_iV[1]];
				CVec2Dfp32 uv2 = pTV[pTri[j].m_iV[2]];
				
				// Calculate the edges between the points
				CVec3Dfp32 edge21 = vert2 - vert1;
				CVec3Dfp32 edge20 = edge2;
				//CVec3Dfp32 edge10 = edge1;			// Not needed since we use the weight for vert2 calculated in Phys_TriangleIntersectRay
				//CVec3Dfp32 edge12 = vert1 - vert2; //  See above
				
				// ---------------- this is for vert0 ---------------- 
				// find A 
				// A is a vector from this vertex to the intersection point 
				CVec3Dfp32 vA = _pCollisionInfo->m_Pos - vert0;
				
				// find B 
				// B is a vector from this intersection to the opposite side (Side1) 
				//    which is the exact length to get to the side                   
				// to do this we split Side2 into two components, but only keep the  
				//    one that's perp to Side2                                       
				fp32 t1 = edge20 * edge21;
				fp32 t2 = edge21 * edge21;
				CVec3Dfp32 vB = edge21 * (t1/t2);
				vB -= edge20;
				
				// finding the weight is the scale part of a projection of A onto B 
				t1 = vA * vB;
				t2 = vB * vB;
				w1 = 1 + t1/t2;

				// ---------------- this is for vert1 ---------------- 
				// find A 
				vA = _pCollisionInfo->m_Pos - vert1;
				
				// find B 
				t1 = edge21 * edge20;
				t2 = edge21 * edge21;
				vB = edge20 * (t1/t2);
				vB -= edge21;
				
				// find the weight
				t1 = vA * vB;
				t2 = vB * vB;
				w2 = 1+t1/t2;

				// ---------------- this is for vert2 ---------------- 
				/* Already calculated in Phys_TriangleIntersectRay
				// find A 
				vA = _pCollisionInfo->m_Pos - vert2;
				
				// find B 
				t1 = DOTPROD(edge12, edge10);
				t2 = DOTPROD(edge10, edge10);
				vB = edge10 * (t1/t2);
				vB -= edge12;
				
				// find the weight
				t1 = DOTPROD(vA, vB);
				t2 = DOTPROD(vB, vB);
				w3 = 1+t1/t2;
				*/
				w3 = uvweight[1];
				
				UV[0] =  w1 * uv0[0] + w2 * uv1[0] + w3 * uv2[0];
				UV[1] =  w1 * uv0[1] + w2 * uv1[1] + w3 * uv2[1];
				
				// Check alpha
				MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
				if (!pTC) Error("Read", "No texture-context available.");
				CXW_SurfaceKeyFrame* frame = m_lspSurfaces[i]->GetBaseFrame();
				if(!frame->IsTransparent(pTC, UV)) 
					return true;
				
			}
		}
	}
	return false;
}


// return true if hit, false otherwise
bool CXR_Model_TriangleMesh::Phys_TriangleIntersectRay(CVec3Dfp32& _orig, CVec3Dfp32& _dir, 
												  CVec3Dfp32& _vert0, CVec3Dfp32& _edge1, CVec3Dfp32& _edge2,
												  CVec2Dfp32& uv,
												  CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Phys_TriangleIntersectRay, false);
	//CVec2Dfp32 uv;
	// Calculate determinant
	CVec3Dfp32 pvec;
	_dir.CrossProd(_edge2, pvec);
	fp32 det = _edge1 * pvec;

	if(det < EPSILON && det > -EPSILON)
		return false; // No inter
	
	fp32 inv_det = 1.0f / det;

	// Calculate distance from vert0 to ray origin 
	CVec3Dfp32 tvec = _orig - _vert0;
	
	// Calculate U parameter and test bounds 
	uv[0] = (tvec * pvec) * inv_det;

	if (uv[0] < 0.0f || uv[0] > 1.0f)
		return false; // No inter
	
	CVec3Dfp32 qvec;
	tvec.CrossProd(_edge1, qvec);

	// Calculate V parameter and test bounds 
	uv[1] = (_dir * qvec) * inv_det;

	if (uv[1] < 0.0f || uv[0] + uv[1] > 1.0f)
		return false; // No inter
	
	// Calculate t (distance) 
	fp32 t = (_edge2 * qvec) * inv_det;
	
	// Store return info
	_pCollisionInfo->m_Pos = _orig + (_dir * t);
	_pCollisionInfo->m_bIsValid = true;

	return true;
}
#endif

void CXR_Model_TriangleMesh::CollectPCS(CXR_PhysicsContext* _pPhysContext, const uint8 _IN1N2Flags, CPotColSet *_pcs, const int _iObj, const int _MediumFlags )
{
}


// -------------------------------------------------------------------
// Wallmark interface:
// -------------------------------------------------------------------
int CXR_Model_TriangleMesh::Wallmark_CreateContext(const CXR_WallmarkContextCreateInfo& _CreateInfo)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Wallmark_CreateContext, 0);
	return 0;
}

void CXR_Model_TriangleMesh::Wallmark_DestroyContext(int _hContext)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Wallmark_DestroyContext, MAUTOSTRIP_VOID);
}

int CXR_Model_TriangleMesh::Wallmark_Create(int _hContext, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Wallmark_Create, 0);
	
	return 0;
}

#if 0
static CVec3Dfp32 g_IST_SpherePos;
static fp32 g_IST_SphereRadius;
static CVec3Dfp32* g_IST_pV;

static bool IntersectSphereTriangle(const uint16* _piV)
{
	MAUTOSTRIP(CXR_Model_BSP___Phys_IntersectSphere_Polygon, false);

	CVec3Dfp32 lEdges[3];
	CVec3Dfp32 lSphereVec[3];
	CVec3Dfp32 Normal;
	g_IST_pV[_piV[1]].Sub(g_IST_pV[_piV[0]], lEdges[0]);
	g_IST_pV[_piV[2]].Sub(g_IST_pV[_piV[1]], lEdges[1]);

	lEdges[0].CrossProd(lEdges[1], Normal);
	fp32 NormalLenSqr = Normal.LengthSqr();
	if (NormalLenSqr < 0.000001f)
		return false;

	fp32 NormalLenRecp = M_InvSqrt(NormalLenSqr);

	g_IST_SpherePos.Sub(g_IST_pV[_piV[0]], lSphereVec[0]);
	fp32 Dist = (Normal * lSphereVec[0]) * NormalLenRecp;

	if (M_Fabs(Dist) > g_IST_SphereRadius)
		return false;

	g_IST_pV[_piV[0]].Sub(g_IST_pV[_piV[2]], lEdges[2]);
	g_IST_SpherePos.Sub(g_IST_pV[_piV[1]], lSphereVec[1]);
	g_IST_SpherePos.Sub(g_IST_pV[_piV[2]], lSphereVec[2]);

	int InsideMask = 0;
	int iv0 = 0;
	for(int v = 0; v < _nV; v++, iv1 = iv0)
	{
		CVec3Dfp32 EdgeNormal;
		lEdges[v].CrossProd(Normal, EdgeNormal);
		if (EdgeNormal * lSphereVec[v] < 0.0f)
		{
			fp32 EdgeLenSqr = lEdges[v].LengthSqr();
			fp32 EdgeLenRecp = M_InvSqrt(EdgeLenSqr);
			fp32 EdgeNormalLen = EdgeNormal.Length();
			fp32 dist_e = EdgeNormalLen * EdgeLenRecp;
			if (dist_e > g_IST_SphereRadius)
				return false;

			fp32 t = (lSphereVec[v] * lEdges[v]) * EdgeLenRecp;
			if (t < 0.0f)
			{
				if (lSphereVec[v].Length() < g_IST_SphereRadius)
					return true;
			}
			else if (t < 1.0f)
			{
			}

		}
		else
			InsideMask |= (1 << v);



		iv0 = _piV[v];
		CVec3Dfp32 vs, ve, n;
		_Pos.Sub(_pV[iv0], vs);
		_pV[iv1].Sub(_pV[iv0], ve);
		fp32 len_e = ve.Length();
		vs.CrossProd(ve, n);
		fp32 dist_e = n.Length() / len_e;
		
		fp32 Side = n*_Plane.n;
		if (Side > 0.0f)
		{
			fp32 t = (vs*ve) / (ve*ve);
			if (t < 0.0f)
			{
				fp32 l = vs.Length();
				if (l > _Radius) continue;
				if (!_pCollisionInfo) return true;
				if (_pCollisionInfo && (l < Edge_Nearest))
				{
					Edge_Nearest = l;
					Edge_Plane.CreateNV(vs.Normalize(), _pV[iv0]);
					Edge_Pos = _pV[iv0];
				}
				Edge_bCollided = true;
			}
			else if (t > 1.0f)
			{
				CVec3Dfp32 vs2 = _Pos-_pV[iv1];
				fp32 l = vs2.Length();
				if (l > _Radius) continue;
				if (!_pCollisionInfo) return true;
				if (_pCollisionInfo && (l < Edge_Nearest))
				{
					Edge_Nearest = l;
					Edge_Plane.CreateNV(vs2.Normalize(), _pV[iv1]);
					Edge_Pos = _pV[iv1];
				}
				Edge_bCollided = true;
			}
			else 
			{
				fp32 l = Abs(dist_e);
				if (l > _Radius) return false;
				if (!_pCollisionInfo) return true;
				if (_pCollisionInfo && (l < Edge_Nearest))
				{
					Edge_Nearest = l;

					CVec3Dfp32 vsproj;
					vs.Project(ve, vsproj);
					Edge_Plane.CreateNV((vs - vsproj).Normalize(), _pV[iv0]);
					Edge_Pos = _pV[iv0] + ve*t;
				}
				Edge_bCollided = true;
			}
		}
		else
			InsideMask |= (1 << v);

	}

	if (!_pCollisionInfo)
	{
		return Edge_bCollided || (InsideMask == (1 << _nV) -1);
	}

	if (InsideMask == (1 << _nV) -1)
	{
		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_bIsCollision = true;
		_pCollisionInfo->m_Plane = _Plane;
		_pCollisionInfo->m_Distance = Abs(d) - _Radius;
		_pCollisionInfo->m_LocalPos = _Pos - _pCollisionInfo->m_Plane.n*d;
		_pCollisionInfo->m_Velocity = 0;
	}
	else
	{
		if (!Edge_bCollided)
			return false;
		_pCollisionInfo->m_bIsValid = true;
		_pCollisionInfo->m_bIsCollision = true;
		_pCollisionInfo->m_Plane = Edge_Plane;
		_pCollisionInfo->m_Distance = Edge_Nearest - _Radius;
		_pCollisionInfo->m_LocalPos = Edge_Pos;
		_pCollisionInfo->m_Velocity = 0;
	}
	return true;
}

#endif

#if defined(PLATFORM_XENON) || defined(PLATFORM_PS3)
static int TriangleInBox(const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, const CVec3Dfp32& _v2, const CBox3Dfp32& _Box)
{
	fp32 xmin, xmax;
	xmin = M_FSel(_v1[0] - _v0[0], _v0[0], _v1[0]);
	xmax = M_FSel(_v1[0] - _v0[0], _v1[0], _v0[0]);
	xmin = M_FSel(xmin - _v2[0], _v2[0], xmin);
	xmax = M_FSel(xmax - _v2[0], xmax, _v2[0]);

	fp32 ymin, ymax;
	ymin = M_FSel(_v1[1] - _v0[1], _v0[1], _v1[1]);
	ymax = M_FSel(_v1[1] - _v0[1], _v1[1], _v0[1]);
	ymin = M_FSel(ymin - _v2[1], _v2[1], ymin);
	ymax = M_FSel(ymax - _v2[1], ymax, _v2[1]);

	fp32 zmin, zmax;
	zmin = M_FSel(_v1[2] - _v0[2], _v0[2], _v1[2]);
	zmax = M_FSel(_v1[2] - _v0[2], _v1[2], _v0[2]);
	zmin = M_FSel(zmin - _v2[2], _v2[2], zmin);
	zmax = M_FSel(zmax - _v2[2], zmax, _v2[2]);

	fp32 Outside0 = M_FSel(xmax - _Box.m_Min[0], 0.0f, -1.0f);
	fp32 Outside1 = M_FSel(_Box.m_Max[0] - xmin, 0.0f, -1.0f);
	fp32 Outside2 = M_FSel(ymax - _Box.m_Min[1], 0.0f, -1.0f);
	fp32 Outside3 = M_FSel(_Box.m_Max[1] - ymin, 0.0f, -1.0f);
	fp32 Outside4 = M_FSel(zmax - _Box.m_Min[2], 0.0f, -1.0f);
	fp32 Outside5 = M_FSel(_Box.m_Max[2] - zmin, 0.0f, -1.0f);

	return M_FSel(Outside5, M_FSel(Outside4, M_FSel(Outside3, M_FSel(Outside2, M_FSel(Outside1, M_FSel(Outside0, 1.0f, 0.0f), 0.0f), 0.0f), 0.0f), 0.0f), 0.0f) > 0.0f;
}
#else
static int TriangleInBox(const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, const CVec3Dfp32& _v2, const CBox3Dfp32& _Box)
{
	fp32 xmin, xmax;
	if (_v0[0] < _v1[0])
	{
		xmin = _v0[0];
		xmax = _v1[0];
	}
	else
	{
		xmin = _v1[0];
		xmax = _v0[0];
	}

	if (xmax < _v2[0])
		xmax = _v2[0];
	if (xmax < _Box.m_Min[0])
		return false;
	if (xmin > _v2[0])
		xmin = _v2[0];
	if (xmin > _Box.m_Max[0])
		return false;

	// ----------------------
	fp32 ymin, ymax;
	if (_v0[1] < _v1[1])
	{
		ymin = _v0[1];
		ymax = _v1[1];
	}
	else
	{
		ymin = _v1[1];
		ymax = _v0[1];
	}

	if (ymax < _v2[1])
		ymax = _v2[1];
	if (ymax < _Box.m_Min[1])
		return false;
	if (ymin > _v2[1])
		ymin = _v2[1];
	if (ymin > _Box.m_Max[1])
		return false;

	// ----------------------
	fp32 zmin, zmax;
	if (_v0[2] < _v1[2])
	{
		zmin = _v0[2];
		zmax = _v1[2];
	}
	else
	{
		zmin = _v1[2];
		zmax = _v0[2];
	}

	if (zmax < _v2[2])
		zmax = _v2[2];
	if (zmax < _Box.m_Min[2])
		return false;
	if (zmin > _v2[2])
		zmin = _v2[2];
	if (zmin > _Box.m_Max[2])
		return false;

	return true;
}
#endif

void CXR_Model_TriangleMesh::Wallmark_CreateWithContainer(void* _pContainer, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material)
{
	MSCOPESHORT(CXR_Model_TriangleMesh::Wallmark_CreateWithContainer);
	if (!_pContainer)
		return;
#ifdef _DEBUG
	if (!TDynamicCast<CXR_TriangleMeshDecalContainer>((CReferenceCount*)_pContainer))
		Error("Wallmark_CreateWithContainer", "Invalid container class.");
#endif

	if(m_lspLOD.Len())
	{
		CXR_Model_TriangleMesh* pLOD0 = m_lspLOD[0];
		if(pLOD0->m_spShadowData)
			pLOD0->Wallmark_CreateWithContainer(_pContainer, _WM, _Origin, _Tolerance, _Flags, _Material);
	}

	if (!m_spShadowData)
		return;

	CTM_VertexBuffer* pSDC = &m_spShadowData->m_VertexBuffer;
	const CVec3Dfp32* pV = pSDC->GetVertexPtr(this, 0);
//	const CVec3Dfp32* pN = pSDC->GetNormalPtr(0); // Not needed !!!!!!!!!!!!
	const CTM_Triangle* pTri = (const CTM_Triangle*)pSDC->GetTriangles(this);

	const CTM_Triangle* pTriReal = m_spShadowData->m_lTriangle.GetBasePtr();

	CXR_TriangleMeshDecalContainer* pTMDC = (CXR_TriangleMeshDecalContainer*)_pContainer;

	int32 *pShadowTriangleClusterLen = m_spShadowData->m_lTriangleCluster.GetBasePtr();

	const CVec3Dfp32& SpherePos = CVec3Dfp32::GetRow(_Origin, 3);
	fp32 SphereRadius = Max(_WM.m_Size, _Tolerance);
//	SphereRadius = M_Sqrt(3*Sqr(SphereRadius));
	CBox3Dfp32 DecalBox;
	DecalBox.m_Min[0] = SpherePos[0] - SphereRadius;
	DecalBox.m_Min[1] = SpherePos[1] - SphereRadius;
	DecalBox.m_Min[2] = SpherePos[2] - SphereRadius;
	DecalBox.m_Max[0] = SpherePos[0] + SphereRadius;
	DecalBox.m_Max[1] = SpherePos[1] + SphereRadius;
	DecalBox.m_Max[2] = SpherePos[2] + SphereRadius;

	int nTri = pSDC->GetNumTriangles(this);
	int iTri = 0;
	int iCluster = 0;

	const int MaxDecalTri = 256;
	uint16 lDecalTriIndices[3 * MaxDecalTri];

	while(iTri < nTri)
	{
		int nDecalTri = 0;
		int Target = iTri + pShadowTriangleClusterLen[iCluster];

		//Shadow clusters don't have texcoords
		//int iRealCluster = /*iCluster;//*/m_spShadowData->m_lRenderData[iCluster].m_iCluster - GetNumVertexBuffers() / 2;
		int iRealCluster = m_spShadowData->m_lRenderData[iCluster].m_iRenderCluster;
		CTM_Cluster* pC = GetCluster(iRealCluster);
		if(!pC->m_BoundBox.IsInside(DecalBox))
		{
			iTri = Target;
			iCluster++;
			continue;
		}
		if(_Material != 0 && m_lspSurfaces[pC->m_iSurface]->GetBaseFrame()->m_MaterialType != _Material)
		{
			iTri = Target;
			iCluster++;
			continue;
		}

		for(; iTri < Target; iTri++)
		{
/*			CVec3Dfp32 Tri[3];
			Tri[0] = pV[pTri[iTri].m_iV[0]];
			Tri[1] = pV[pTri[iTri].m_iV[1]];
			Tri[2] = pV[pTri[iTri].m_iV[2]];
*/
//			if (Phys_Intersect_TriSphere(Tri, SpherePos, SpherePos, SphereRadius, false, NULL))
			const CVec3Dfp32& v0 = pV[pTri[iTri].m_iV[0]];
			const CVec3Dfp32& v1 = pV[pTri[iTri].m_iV[1]];
			const CVec3Dfp32& v2 = pV[pTri[iTri].m_iV[2]];

			if (TriangleInBox(v0, v1, v2, DecalBox))
			{

				CVec3Dfp32 e0, e1, n;
				v1.Sub(v0, e0);
				v2.Sub(v0, e1);
				e1.CrossProd(e0, n);
				if ( (n * CVec3Dfp32::GetRow(_Origin, 2)) > 0.0f)
				{
					if (nDecalTri >= MaxDecalTri)
					{
						ConOut("§cf80WARNING: (TriMeshDecal) Too many triangles in decal.");
						iTri = Target;
						break;
					}
					lDecalTriIndices[nDecalTri*3 + 0] = pTriReal[iTri].m_iV[0] + pC->m_iVBVert;
					lDecalTriIndices[nDecalTri*3 + 1] = pTriReal[iTri].m_iV[1] + pC->m_iVBVert;
					lDecalTriIndices[nDecalTri*3 + 2] = pTriReal[iTri].m_iV[2] + pC->m_iVBVert;
					nDecalTri++;
				}
			}
		}

		if (nDecalTri)
		{
			CXR_TMDC_Decal Decal;
			Decal.m_TexU = CVec3Dfp32::GetRow(_Origin, 0);
			Decal.m_TexV = CVec3Dfp32::GetRow(_Origin, 1);
			fp32 SizeRecp = 1.0f / _WM.m_Size;
			Decal.m_TexU *= SizeRecp;
			Decal.m_TexV *= SizeRecp;
			Decal.m_Normal = CVec3Dfp32::GetRow(_Origin, 2);

			Decal.m_Pos = SpherePos;
			Decal.m_iBone = _WM.m_iNode;
			Decal.m_iCluster = iRealCluster;
			Decal.m_SurfaceID = _WM.m_SurfaceID;
			Decal.m_ModelGUID = (mint)this;
			Decal.m_SpawnTime = _WM.m_SpawnTime;

//ConOutL(CStrF("AddDecal Size %f, iBone %d, iCluster %d, Origin %s", _WM.m_Size, Decal.m_iBone, Decal.m_iCluster, _Origin.GetString().Str()));
			pTMDC->AddDecal(_WM.m_GUID, Decal, lDecalTriIndices, nDecalTri*3);
		}
		iCluster++;
	}
}


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Acquires flags for VertexProgram params

Parameters:		
	_pTVB:	Pointer to cluster in question
	_pFile:	File pointer

Returns:	Flags
\*____________________________________________________________________*/ 
uint32 CXR_Model_TriangleMesh::GetVertexBufferFlags(CTM_VertexBuffer * _pTVB,CCFile * _pFile)
{
	uint32 Flags = 0;

	{
		const CTM_BDVertexInfo* pVI = _pTVB->GetBDVertexInfo(this, _pFile);
	
		if( pVI )
		{
			int nV = _pTVB->GetNumVertices(this, _pFile);

			for(int v = 0; v < nV; v++)
			{
				if( pVI[v].m_nBones > 4 )
				{
					Flags |= VBF_DUAL_MATRIX_LISTS;
					break;
				}
			}
		}
	}

	return Flags;
}


// -------------------------------------------------------------------
// CXR_VBContainer overrides
// -------------------------------------------------------------------
int CXR_Model_TriangleMesh::GetNumLocal()
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_GetNumLocal, 0);
	return GetNumVertexBuffers();
}

int CXR_Model_TriangleMesh::GetID(int _iLocal)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_GetID, 0);
//	if (!m_lspVertexBuffers.ValidPos(_iLocal))
//		Error("GetID", CStrF("Invalid local ID %d", _iLocal));

	return GetVertexBuffer(_iLocal)->m_VBID;
}

CFStr CXR_Model_TriangleMesh::GetName(int _iLocal)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_GetName, CFStr());
	int iLod = 0;
	if (!m_lspLOD.Len())
		iLod = m_iLOD + 1;
	return CFStrF("%s:%04d:%04d", m_FileName.Str(), iLod, _iLocal);
}

uint32 CXR_Model_TriangleMesh::GetFlags(int _iLocal)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_GetFlags, uint32);
	return m_lClusterFlags[_iLocal];
}


template <int ePlatform>
static void SetWantTransform(CRC_BuildVertexBuffer& _VB, const CBox3Dfp32& _BoundBox, const CTM_VertexBuffer* _pVB);

template <>
static void SetWantTransform<e_Platform_Default>(CRC_BuildVertexBuffer& _VB, const CBox3Dfp32& _BoundBox, const CTM_VertexBuffer* _pVB)
{
}

template <>
static void SetWantTransform<e_Platform_Xenon>(CRC_BuildVertexBuffer& _VB, const CBox3Dfp32& _BoundBox, const CTM_VertexBuffer* _pVB)
{
	_VB.Geometry_SetWantFormat(CRC_VREG_POS, CRC_VREGFMT_NU3_P32);
	// Transform Positions
	{
		CRC_VRegTransform Transform;
		Transform.m_Scale = CVec4Dfp32((_BoundBox.m_Max[0] - _BoundBox.m_Min[0]), (_BoundBox.m_Max[1] - _BoundBox.m_Min[1]), (_BoundBox.m_Max[2] - _BoundBox.m_Min[2]), 1.0f);
		Transform.m_Offset = CVec4Dfp32(_BoundBox.m_Min[0], _BoundBox.m_Min[1], _BoundBox.m_Min[2], 0.0f);
		_VB.Geometry_SetWantTransform(CRC_VREG_POS, Transform);
	}

	if (!(_pVB->m_VBFlags & CTM_CLUSTERFLAGS_SHADOWVOLUME) && _pVB->m_lTVFrames.Len())
	{
		CRC_VRegTransform Transform;
		Transform.m_Scale = CVec4Dfp32((_pVB->m_lTVFrames[0].m_Bound[2] - _pVB->m_lTVFrames[0].m_Bound[0]) / 65535.0f, (_pVB->m_lTVFrames[0].m_Bound[3] - _pVB->m_lTVFrames[0].m_Bound[1]) / 65535.0f, 1.0f, 1.0f);
		Transform.m_Offset = CVec4Dfp32(_pVB->m_lTVFrames[0].m_Bound[0], _pVB->m_lTVFrames[0].m_Bound[1], 0.0f, 0.0f);

		_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD0, CRC_VREGFMT_V2_U16);
		_VB.Geometry_SetWantTransform(CRC_VREG_TEXCOORD0, Transform);

		// Normal and tangents
		_VB.Geometry_SetWantFormat(CRC_VREG_NORMAL, CRC_VREGFMT_NS3_P32);
		_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD1, CRC_VREGFMT_NS3_P32);
		_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD2, CRC_VREGFMT_NS3_P32);

	}

	// Matrix palette
	uint32 MatrixFmt = CRC_VREGFMT_N4_UI8_P32_NORM;
	if (_VB.m_lpVReg[CRC_VREG_MI0])
		_VB.Geometry_SetWantFormat(CRC_VREG_MI0, MatrixFmt);
	if (_VB.m_lpVReg[CRC_VREG_MI1])
		_VB.Geometry_SetWantFormat(CRC_VREG_MI1, MatrixFmt);

	if (_VB.m_lpVReg[CRC_VREG_MW0])
		_VB.Geometry_SetWantFormat(CRC_VREG_MW0, MatrixFmt);
	if (_VB.m_lpVReg[CRC_VREG_MW1])
		_VB.Geometry_SetWantFormat(CRC_VREG_MW1, MatrixFmt);
}

template <>
static void SetWantTransform<e_Platform_PS3>(CRC_BuildVertexBuffer& _VB, const CBox3Dfp32& _BoundBox, const CTM_VertexBuffer* _pVB)
{
	// Transform Positions
	{
		CRC_VRegTransform Transform;
		CVec3Dfp32 Center;
		_BoundBox.GetCenter(Center);
		Transform.m_Scale = CVec4Dfp32((_BoundBox.m_Max[0] - _BoundBox.m_Min[0]) * 0.5f, (_BoundBox.m_Max[1] - _BoundBox.m_Min[1]) * 0.5f, (_BoundBox.m_Max[2] - _BoundBox.m_Min[2]) * 0.5f, 1.0f);
		Transform.m_Offset = CVec4Dfp32(Center.k[0], Center.k[1], Center.k[2], 0.0f);
		_VB.Geometry_SetWantFormat(CRC_VREG_POS, CRC_VREGFMT_NS3_P32);
		_VB.Geometry_SetWantTransform(CRC_VREG_POS, Transform);
	}

	if (!(_pVB->m_VBFlags & CTM_CLUSTERFLAGS_SHADOWVOLUME) && _pVB->m_lTVFrames.Len())
	{
		CVec2Dfp32 Min2D = CVec2Dfp32(_pVB->m_lTVFrames[0].m_Bound[0], _pVB->m_lTVFrames[0].m_Bound[1]);
		CVec2Dfp32 Max2D = CVec2Dfp32(_pVB->m_lTVFrames[0].m_Bound[2], _pVB->m_lTVFrames[0].m_Bound[3]);
		if(0 && Min2D.k[0] >= -1.0f && Min2D.k[1] >= -1.0f && Max2D.k[0] <= 1.0f && Max2D.k[1] <= 1.0f)
		{
			// For some not having a transform breaks stuff...
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD0, CRC_VREGFMT_NS2_I16);
		}
		else
		{
			CRC_VRegTransform Transform;
			Transform.m_Scale = CVec4Dfp32((Max2D.k[0] - Min2D.k[0]) * 0.5f, (Max2D.k[1] - Min2D.k[1]) * 0.5f, 1.0f, 1.0f);
			Transform.m_Offset = CVec4Dfp32((Max2D.k[0] + Min2D.k[0]) * 0.5f, (Max2D.k[1] + Min2D.k[1]) * 0.5f, 0.0f, 0.0f);
			_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD0, CRC_VREGFMT_NS2_I16);

			_VB.Geometry_SetWantTransform(CRC_VREG_TEXCOORD0, Transform);
		}

		// Normal and tangents
		_VB.Geometry_SetWantFormat(CRC_VREG_NORMAL, CRC_VREGFMT_NS3_P32);
		_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD1, CRC_VREGFMT_NS3_P32);
		_VB.Geometry_SetWantFormat(CRC_VREG_TEXCOORD2, CRC_VREGFMT_NS3_P32);

	}

	// Matrix palette
	uint32 MatrixFmt = CRC_VREGFMT_N4_UI8_P32_NORM;
	if (_VB.m_lpVReg[CRC_VREG_MI0])
		_VB.Geometry_SetWantFormat(CRC_VREG_MI0, MatrixFmt);
	if (_VB.m_lpVReg[CRC_VREG_MI1])
		_VB.Geometry_SetWantFormat(CRC_VREG_MI1, MatrixFmt);

	if (_VB.m_lpVReg[CRC_VREG_MW0])
		_VB.Geometry_SetWantFormat(CRC_VREG_MW0, MatrixFmt);
	if (_VB.m_lpVReg[CRC_VREG_MW1])
		_VB.Geometry_SetWantFormat(CRC_VREG_MW1, MatrixFmt);
}


void CXR_Model_TriangleMesh::Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Get, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_TriangleMesh::Get, MODEL_TRIMESH); 

#if defined (PLATFORM_WIN_PC)
	int bXDF = D_MXDFCREATE;
	int Platform = D_MPLATFORM;
#endif

	CTM_VertexBuffer* pTVB = GetVertexBuffer(_iLocal);

	int bSaveArrays = _Flags & VB_GETFLAGS_FALLBACK;
	if (!pTVB->m_lVFrames.Len())
	{
		M_ASSERT(0, "!");
		return;
	}

	_VB.Clear();

	CCFile *pModelFile;
	if( m_spPreloader == NULL )
	{
	//	ConOut(CStrF("§cf80WARNING: (CXR_Model_TriangleMesh::Get) Missing preloader for model %s cluster %i",m_FileName.Str(),_iLocal));
		pModelFile = NULL;
	}
	else
	{
		pModelFile = m_spPreloader->m_spFile;
	}

#if defined (PLATFORM_WIN_PC)
	if( (pTVB->m_VBFlags & CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE) && ( !bXDF || ( bXDF && Platform != 2/*e_Platform_PS2*/ )  ) )
#else
	if (pTVB->m_VBFlags & CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE)
#endif
	{
		int Len = pTVB->GetNumVertices(this, pModelFile);
		// For now lets use the texture coordinates to communicate which vertices that are supposed to be extruded
		pTVB->CreateMatrixIW(this, pModelFile);

		CVec3Dfp32 *pSrcVert = pTVB->GetVertexPtr(this, 0);
		fp32 *pSrcMI = (fp32 *)pTVB->m_lMatrixIndices.GetBasePtr();
		fp32 *pSrcMW = pTVB->m_lMatrixWeights.GetBasePtr();
		bool bBones = pSrcMW != 0;

		int nMWComp = 0,nMWC2 = 0;
		if (Len)
		{
			nMWComp = pTVB->m_lMatrixWeights.Len() / Len;
			if( nMWComp > 4 )
			{
				nMWC2 = nMWComp - 4;
				nMWComp = 4;
			}

			int nNeededSize = 3; // The tangent
			nNeededSize += 1; // Extrude 0 or 1

			if (bBones)
			{
				nNeededSize += nMWComp + nMWC2; // Matirx Weights
				if (nMWC2 > 0)
					nNeededSize += 2; // Matirx Indicies
				else
					nNeededSize += 1; // Matirx Indicies
			}

			nNeededSize *= Len*2; // Num Verticies
			nNeededSize += 2; // Align
			nNeededSize /= 3; // sizeof(CVec3Dfp32) / sizeof(fp32)

			pTVB->m_lVFrames[0].m_lTangentU.SetLen(nNeededSize);
		}

		fp32 *pStart = (fp32 *)pTVB->m_lVFrames[0].m_lTangentU.GetBasePtr();

		CVec3Dfp32 *pVert = (CVec3Dfp32 *)pStart;
		fp32 *pTex = pStart + (Len*2) * 3;
		fp32 *pMI = pTex + (Len*2);
		fp32 *pMW = pMI + (Len*(2 + ((nMWC2 > 0) ? 2 : 0)));
		// Don't extrude
		for (int i = 0; i < Len; ++i)
		{
			pTex[i] = 0.0;
		}
		memcpy(pVert, pSrcVert, Len * 3 * 4);
		if (bBones)
		{
			memcpy(pMI, pSrcMI, Len * 4);
			memcpy(pMW, pSrcMW, Len * nMWComp * 4);
		}
		// Extrude
		for (int i = 0; i < Len; ++i)
		{
			pTex[Len + i] = 1.0;
		}
		memcpy(pVert + Len, pSrcVert, Len * 4 * 3);
		if (bBones)
		{
			memcpy(pMI + Len, pSrcMI, Len * 4);
			memcpy(pMW + Len * nMWComp, pSrcMW, Len * nMWComp * 4);
		}

		if (bBones && (nMWC2 > 0))
		{
			memcpy(pMI + Len*2, pSrcMI + Len,Len * 4);
			memcpy(pMI + Len*3, pSrcMI + Len,Len * 4);
			memcpy(pMW + Len * nMWComp * 2, pSrcMW + Len * nMWComp, Len * nMWC2 * 4);
			memcpy(pMW + Len * nMWComp * 3, pSrcMW + Len * nMWComp, Len * nMWC2 * 4);
		}

		if (nMWComp)
		{
			_VB.Geometry_MatrixIndex0((uint32 *)pMI);
			if (nMWC2 > 0)
				_VB.Geometry_MatrixIndex1((uint32 *)pMI + Len * 2);

			switch(nMWComp + nMWC2)
			{
			case 1:
				_VB.Geometry_MatrixWeight0((fp32 *)pMW);
				break;
			case 2:
				_VB.Geometry_MatrixWeight0((CVec2Dfp32 *)pMW);
				break;
			case 3:
				_VB.Geometry_MatrixWeight0((CVec3Dfp32 *)pMW);
				break;
			case 4:
				_VB.Geometry_MatrixWeight0((CVec4Dfp32 *)pMW);
				break;
			case 5:
				_VB.Geometry_MatrixWeight0((CVec4Dfp32 *)pMW);
				_VB.Geometry_MatrixWeight1((fp32 *)(pMW + Len * nMWComp * 2));
				break;
			case 6:
				_VB.Geometry_MatrixWeight0((CVec4Dfp32 *)pMW);
				_VB.Geometry_MatrixWeight1((CVec2Dfp32 *)(pMW + Len * nMWComp * 2));
				break;
			case 7:
				_VB.Geometry_MatrixWeight0((CVec4Dfp32 *)pMW);
				_VB.Geometry_MatrixWeight1((CVec3Dfp32 *)(pMW + Len * nMWComp * 2));
				break;
			case 8:
				_VB.Geometry_MatrixWeight0((CVec4Dfp32 *)pMW);
				_VB.Geometry_MatrixWeight1((CVec4Dfp32 *)(pMW + Len * nMWComp * 2));
				break;
			}
		}

		_VB.Geometry_VertexArray(pVert);
		_VB.m_nV = Len*2;
		_VB.Geometry_TVertexArray(pTex, 0);

		if (bSaveArrays)
		{
			pTVB->m_bSave_Vertices = true;
			pTVB->m_bSave_TangentU = true;
		}
	}
	else
	{
		_VB.m_nV = pTVB->GetNumVertices(this, pModelFile);
		_VB.Geometry_VertexArray(pTVB->GetVertexPtr(this, 0,pModelFile));
		if( pTVB->GetNormalPtr(this, 0,pModelFile) ) _VB.Geometry_NormalArray(pTVB->GetNormalPtr(this, 0,pModelFile));
		if (bSaveArrays)
		{
			pTVB->m_bSave_Vertices = true;
			pTVB->m_bSave_Normals = true;
		}

		if (!(pTVB->m_VBFlags & CTM_CLUSTERFLAGS_SHADOWVOLUME))
		{
			CVec2Dfp32 * pTV = pTVB->GetTVertexPtr(this, 0,pModelFile);
			if( pTV ) _VB.Geometry_TVertexArray(pTV, 0);

//			_VB.m_pTV[1] = (fp32*)pTVB->GetTVertexPtr(0);
//			_VB.m_nTVComp[1] = 2;

			if (bSaveArrays)
				pTVB->m_bSave_TVertices = true;

			// No tangents needed for shadow volumes
		#ifndef	PLATFORM_PS2
		#if defined(PLATFORM_WIN_PC)
			if( pTV && (!bXDF || ( bXDF && Platform != 2/*e_Platform_PS2*/) )  )
		#endif
			{
				_VB.Geometry_TVertexArray(pTVB->GetTangentUPtr(this, 0,pModelFile), 1);
				_VB.Geometry_TVertexArray(pTVB->GetTangentVPtr(this, 0,pModelFile), 2);
				if (bSaveArrays)
				{
					pTVB->m_bSave_TangentU = true;
					pTVB->m_bSave_TangentV = true;
				}
			}
		#endif
		}
		pTVB->CreateMatrixIW(this, pModelFile);
//		pTVB->GetNumBDMatrixMap(this, pModelFile);	// Forceload bonedeform

	#ifndef TEMP_NO_SKELETON_ANIM
		if (bSaveArrays)
		{
			pTVB->m_bSave_VBBoneDeform = true;
		}

		if (pTVB->GetNumVertices(this, pModelFile))
		{
			int nComp = pTVB->m_lMatrixWeights.Len() / pTVB->GetNumVertices(this, pModelFile);

			if (nComp > 4)
			{
				_VB.Geometry_MatrixIndex0(pTVB->m_lMatrixIndices.GetBasePtr());
				_VB.Geometry_MatrixIndex1(pTVB->m_lMatrixIndices.GetBasePtr() + _VB.m_nV);
			}
			else if (nComp)
				_VB.Geometry_MatrixIndex0(pTVB->m_lMatrixIndices.GetBasePtr());

			switch (nComp)
			{
			case 1:
				_VB.Geometry_MatrixWeight0((fp32 *)pTVB->m_lMatrixWeights.GetBasePtr());
				break;
			case 2:
				_VB.Geometry_MatrixWeight0((CVec2Dfp32 *)pTVB->m_lMatrixWeights.GetBasePtr());
				break;
			case 3:
				_VB.Geometry_MatrixWeight0((CVec3Dfp32 *)pTVB->m_lMatrixWeights.GetBasePtr());
				break;
			case 4:
				_VB.Geometry_MatrixWeight0((CVec4Dfp32 *)pTVB->m_lMatrixWeights.GetBasePtr());
				break;
			case 5:
				_VB.Geometry_MatrixWeight0((CVec4Dfp32 *)pTVB->m_lMatrixWeights.GetBasePtr());
				_VB.Geometry_MatrixWeight1((fp32 *)(pTVB->m_lMatrixWeights.GetBasePtr() + _VB.m_nV * 4));
				break;
			case 6:
				_VB.Geometry_MatrixWeight0((CVec4Dfp32 *)pTVB->m_lMatrixWeights.GetBasePtr());
				_VB.Geometry_MatrixWeight1((CVec2Dfp32 *)(pTVB->m_lMatrixWeights.GetBasePtr() + _VB.m_nV * 4));
				break;
			case 7:
				_VB.Geometry_MatrixWeight0((CVec4Dfp32 *)pTVB->m_lMatrixWeights.GetBasePtr());
				_VB.Geometry_MatrixWeight1((CVec3Dfp32 *)(pTVB->m_lMatrixWeights.GetBasePtr() + _VB.m_nV * 4));
				break;
			case 8:
				_VB.Geometry_MatrixWeight0((CVec4Dfp32 *)pTVB->m_lMatrixWeights.GetBasePtr());
				_VB.Geometry_MatrixWeight1((CVec4Dfp32 *)(pTVB->m_lMatrixWeights.GetBasePtr() + _VB.m_nV * 4));
				break;
			}


		}
	#endif
	}

	// If matrix palette transform is not needed the code below works out to reset the matrix palette members.

	if (!(pTVB->m_VBFlags & CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE))
	{
		if (m_bUsePrimitives)
		{
			_VB.m_piPrim = (uint16*) pTVB->GetPrimitives(this, pModelFile);
			_VB.m_nPrim = pTVB->GetNumPrimitives(this, pModelFile);
			_VB.m_PrimType = CRC_RIP_STREAM;
			if (bSaveArrays)
			{
				pTVB->m_bSave_Primitives = true;
			}
		}
		else
		{
			_VB.m_piPrim = (uint16*) pTVB->GetTriangles(this, pModelFile);
			_VB.m_nPrim = pTVB->GetNumTriangles(this, pModelFile);
			_VB.m_PrimType = CRC_RIP_TRIANGLES;
			if (bSaveArrays)
			{
				pTVB->m_bSave_Triangles = true;
			}
		}
	}

	switch (D_MPLATFORM)
	{
	case e_Platform_Xenon: SetWantTransform<e_Platform_Xenon>(_VB, m_BoundBox, pTVB); break;
	case e_Platform_PS3:   SetWantTransform<e_Platform_PS3>(_VB, m_BoundBox, pTVB); break;
	default:               SetWantTransform<e_Platform_Default>(_VB, m_BoundBox, pTVB); break;
	}

	//Handle preloader
	if( (m_spPreloader != NULL) && (--m_spPreloader->m_nVB == 0) )
	{
		m_spPreloader = NULL;
	}

	// Compress the usual, position and tangents
/*	//AR-ADD: Calculate bounding box
	if (_pBoundBox)
	{
		_pBoundBox->m_Min=   _FP32_MAX;
		_pBoundBox->m_Max = -_FP32_MAX;
		for (int i=0; i<_VB.m_nV; i++)
			_pBoundBox->Expand(_VB.m_pV[i]);
	}*/
}

void CXR_Model_TriangleMesh::Release(int _iLocal)
{
	MAUTOSTRIP(CXR_Model_TriangleMesh_Release, MAUTOSTRIP_VOID);
//	if (!m_lspVertexBuffers.ValidPos(_iLocal))
//		Error("Release", CStrF("Invalid local ID %d", _iLocal));

	CTM_VertexBuffer* pTVB = GetVertexBuffer(_iLocal);

	if (pTVB->m_bDelayLoad)
	{
		// Destroy everything that we havn't been told to keep

		for (int i = 0; i < pTVB->m_lVFrames.Len(); ++i)
		{
			if (!pTVB->m_bSave_Vertices)
				pTVB->m_lVFrames[i].m_lVertices.Destroy();
			if (!pTVB->m_bSave_Normals)
				pTVB->m_lVFrames[i].m_lNormals.Destroy();
			if (!pTVB->m_bSave_TangentU)
				pTVB->m_lVFrames[i].m_lTangentU.Destroy();
			if (!pTVB->m_bSave_TangentV)
				pTVB->m_lVFrames[i].m_lTangentV.Destroy();
		}

		for (int i = 0; i < pTVB->m_lTVFrames.Len(); ++i)
		{
			if (!pTVB->m_bSave_TVertices)
				pTVB->m_lTVFrames[i].m_lVertices.Destroy();
		}

		if (!pTVB->m_bSave_BoneDeform)
		{
			pTVB->m_lBDVertexInfo.Destroy();
			pTVB->m_lBDInfluence.Destroy();
		}

//		if (!pTVB->m_bSave_BoneDeformMatrixMap)
//		{
//			pTVB->m_lBDMatrixMap.Destroy();
//		}

		if (!pTVB->m_bSave_VBBoneDeform)
		{
			pTVB->m_lMatrixIndices.Destroy();
			pTVB->m_lMatrixWeights.Destroy();
		}

		if (!pTVB->m_bSave_Triangles)
		{
			pTVB->m_lTriangles.Destroy();
		}

		if (!pTVB->m_bSave_Primitives)
		{
			pTVB->m_lPrimitives.Destroy();
		}

		if (!pTVB->m_bSave_Edges)
		{
			pTVB->m_lEdges.Destroy();
		}

		if (!pTVB->m_bSave_EdgeTris)
		{
			pTVB->m_lEdgeTris.Destroy();
		}
		
		if (!pTVB->m_bSave_TriEdges)
		{
			pTVB->m_lTriEdges.Destroy();
		}
	}
}

#if !defined(M_RTM) && !defined(PLATFORM_CONSOLE) 
void CXR_Model_TriangleMesh::InitDebugSolids()
{
	if (!m_lspDebugSolids.Len() && m_lSolids.Len())
	{
		m_lspDebugSolids.SetLen(m_lSolids.Len());

		for(int i = 0; i < m_lSolids.Len(); i++)
		{
			CXR_ThinSolid* pS = &m_lSolids[i];

			spCSolid spS = MNew(CSolid);
			if (!spS) MemError("OnRender");

			for(int p = 0; p < pS->m_lPlanes.Len(); p++)
			{
				CPlane3Dfp64 Plane(pS->m_lPlanes[p].n.Getfp64(), pS->m_lPlanes[p].d);
//				Plane.Inverse();
				spS->AddPlane(Plane);
			}

			spS->UpdateMesh();

			//LogFile(CStrF("Solid %d, Planes %d, Final faces %d", i, pS->m_lPlanes.Len(), spS->m_lspFaces.Len()));

			m_lspDebugSolids[i] = spS;
		}
	}
}
#endif
