#include "PCH.h"
#include "WModel_MultiTriMesh.h"

#include "MFloat.h"
#include "../../XR/XR.h"
#include "../../XR/XREngineVar.h"
#include "../../XR/XRShader.h"
#include "../../XR/XRVBContext.h"

#include "../Model_Trimesh/WTriMeshRIP.h"

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_MultiTriMesh, CXR_Model);
IMPLEMENT_OPERATOR_NEW(CXR_Model_MultiTriMesh);


//undefine to use GPU instead - doesn't work anymore!
#define MTMESH_USE_CPU

CXR_Model_MultiTriMesh::CXR_Model_MultiTriMesh()
{
	SetThreadSafe(true);
}

// -------------------------------------------------------------------
// Derived from CXR_Model

void CXR_Model_MultiTriMesh::Create(const char * _pParam)
{
	if( !m_spTriangleMesh )
	{
		m_spTriangleMesh = MNew(CXR_Model_TriangleMesh);
	}
	CStr Param(_pParam);
	CStr Filename = Param.GetStrSep(",");
	if( Filename.Len() < 5 ) Filename = Param;

	m_spTriangleMesh->Create(_pParam);
	m_spTriangleMesh->Read(Filename + ".xmd");

	//Default value
	GenerateInstances(m_spTriangleMesh);

#ifndef MTMESH_USE_CPU
	/*
	int i;
	for(i=0;i < m_spTriangleMesh->m_lspLOD.Len();i++)
	{
		GenerateInstances(nInstances,m_spTriangleMesh->m_lspLOD[i]);
	}
	*/
#else
	int i;
	for(i=0;i < m_spTriangleMesh->m_lspLOD.Len();i++)
	{
		//clear shadow data - just in case
		m_spTriangleMesh->m_lspLOD[i]->m_spShadowData = NULL;
	}
#endif
}


void CXR_Model_MultiTriMesh::Create(const char* _pParam, CDataFile* _pDFile, CCFile* _pFile)
{
	if( !m_spTriangleMesh )
	{
		m_spTriangleMesh = MNew(CXR_Model_TriangleMesh);
	}
	CStr Param(_pParam);
	m_spTriangleMesh->Create(_pParam);
	m_spTriangleMesh->Read(_pDFile);

	GenerateInstances(m_spTriangleMesh);

#ifndef MTMESH_USE_CPU
	/*
	int i;
	for(i=0;i < m_spTriangleMesh->m_lspLOD.Len();i++)
	{
		GenerateInstances(nInstances,m_spTriangleMesh->m_lspLOD[i]);
	}
	*/
#else
	int i;
	for(i=0;i < m_spTriangleMesh->m_lspLOD.Len();i++)
	{
		//clear shadow data - just in case
		m_spTriangleMesh->m_lspLOD[i]->m_spShadowData = NULL;
	}
#endif
}

int CXR_Model_MultiTriMesh::GetRenderPass(const CXR_AnimState * _pAnimState /* = NULL */)
{
	return m_spTriangleMesh->GetRenderPass(_pAnimState);
}

int CXR_Model_MultiTriMesh::GetVariationIndex(const char* _pName)
{
	return m_spTriangleMesh->GetVariationIndex(_pName);
}

int CXR_Model_MultiTriMesh::GetNumVariations()
{
	return m_spTriangleMesh->GetNumVariations();
}

#ifndef M_RTMCONSOLE
CStr CXR_Model_MultiTriMesh::GetVariationName(int _iVariation)
{
	return m_spTriangleMesh->GetVariationName(_iVariation);
}
#endif

CXR_Model * CXR_Model_MultiTriMesh::GetLOD(const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, CXR_Engine* _pEngine, 
										   int *_piLod /* = NULL */)
{
	return m_spTriangleMesh->GetLOD(_WMat,_VMat,_pEngine,_piLod);
}

CXR_Skeleton * CXR_Model_MultiTriMesh::GetSkeleton()
{
	return m_spTriangleMesh->GetSkeleton();
}

CXR_Skeleton * CXR_Model_MultiTriMesh::GetPhysSkeleton()
{
	return m_spTriangleMesh->GetPhysSkeleton();
}

aint CXR_Model_MultiTriMesh::GetParam(int _Param)
{
	return m_spTriangleMesh->GetParam(_Param);
}

void CXR_Model_MultiTriMesh::SetParam(int _Param,aint _Value)
{
	return m_spTriangleMesh->SetParam(_Param,_Value);
}

int CXR_Model_MultiTriMesh::GetParamfv(int _Param, fp32 * _pRetValues)
{
	return m_spTriangleMesh->GetParamfv(_Param,_pRetValues);
}

void CXR_Model_MultiTriMesh::SetParamfv(int _Param, const fp32 * _pValues)
{
	return m_spTriangleMesh->SetParamfv(_Param,_pValues);
}

fp32 CXR_Model_MultiTriMesh::GetBound_Sphere(const CXR_AnimState * _pAnimState)
{
	return m_spTriangleMesh->GetBound_Sphere(_pAnimState);
}

void CXR_Model_MultiTriMesh::GetBound_Box(CBox3Dfp32& _Box, int _Mask, 
										  const CXR_AnimState* _pAnimState /* = NULL */)
{
	_Box = m_BoundingBox;
}

void CXR_Model_MultiTriMesh::GetBound_Box(CBox3Dfp32& _Box, int _Mask, 
										  CXR_AnimState* _pAnimState /* = NULL */)
{
	_Box = m_BoundingBox;
}

//In WTriMesh.cpp
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

void CXR_Model_MultiTriMesh::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, 
									  CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, 
									  spCXR_WorldLightState _spWLS, const CXR_AnimState* _pAnimState, 
									  const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags /* = 0 */)
{
//Most of this copied from WTriMesh.cpp
	MAUTOSTRIP(CXR_Model_MultiTriMesh_OnRender, MAUTOSTRIP_VOID);

	int SceneType = (_pEngine) ? _pEngine->m_SceneType : XR_SCENETYPE_MAIN;

//In GPU-mode, we need to know what LOD-mesh to use. We always use the root-mesh in CPU mode
	CXR_Model_TriangleMesh * pMesh = m_spTriangleMesh;

//We don't want to render if we aren't assigned any matrices
	if( (_pAnimState->m_pSkeletonInst == NULL) || 
		(_pAnimState->m_pSkeletonInst->m_pBoneTransform == NULL) ||
		(_pAnimState->m_pSkeletonInst->m_nBoneTransform == 0) )
	{
		return;
	}

	uint BoneTransformOffset = (_pAnimState->m_Data[0] >= 0) ? _pAnimState->m_Data[0] : 0;
	CMat4Dfp32 * pMatrices = _pAnimState->m_pSkeletonInst->m_pBoneTransform + BoneTransformOffset;
	uint16	nCurrentInstances = Min<uint16>((_pAnimState->m_Data[1] >= 0) ? _pAnimState->m_Data[1] : _pAnimState->m_pSkeletonInst->m_nBoneTransform,CTM_MULTITRIMESH_MAXINSTANCES);
	SetInstances(pMatrices,nCurrentInstances);

	uint8 liLOD[CTM_MULTITRIMESH_MAXINSTANCES];

	if (pMesh->m_lspLOD.Len() && !(_Flags & CXR_MODEL_ONRENDERFLAGS_MAXLOD))
	{
		int nLOD = Min(pMesh->m_lspLOD.Len(), pMesh->m_lLODBias.Len());

		if (pMesh->m_RenderFlags & CTM_RFLAGS_MINLOD || _Flags & CXR_MODEL_ONRENDERFLAGS_MINLOD)
		{
#ifndef MTMESH_USE_CPU
			/*
			pMesh = m_spTriangleMesh->m_lspLOD[nLOD-1];
			*/
#else
			int i;
			for(i=0;i < nCurrentInstances;i++)
			{
				liLOD[i] = nLOD-1;
			}
#endif
		}
		else
		{
			int i,j;

			/*
			for(i = 0;i < m_nCurrentInstances;i++)
			{
				m_lMatrices[i].Multiply(_WMat,m_lRenderMatrices[i]);
			}
			*/

			CVec3Dfp32 Pos(0,0,0);
			for(i=0;i < nCurrentInstances;i++)
			{

#ifndef MTMESH_USE_CPU
			/*
				Pos += CVec3Dfp32::GetMatrixRow(pMatrices[i],3);
			}
			Pos *= 1.0f / (fp32)nCurrentInstances;
			*/
#else
				Pos = CVec3Dfp32::GetMatrixRow(pMatrices[i],3);
				liLOD[i] = 0;
#endif
				Pos *= _VMat;
				fp32 Dist = Pos.Length();

				if( _pEngine )
				{
					Dist += _pEngine->m_LODOffset;
					Dist *= _pEngine->m_LODScale;
				}

				for(j = nLOD-1; j >= 0; j--)
				{
					if (Dist >= m_spTriangleMesh->m_lLODBias[j])
					{
#ifndef MTMESH_USE_CPU
						/*
						pMesh = m_spTriangleMesh->m_lspLOD[j];
						*/
#else
						liLOD[i] = j+1;
					}
#endif
				}
			}
		}
	}
	else
	{
#ifndef MTMESH_USE_CPU

#else
		int i;
		for(i=0;i < nCurrentInstances;i++)
		{
			liLOD[i] = 0;
		}
#endif
	}

	if ((!_pEngine || !(_pEngine->m_EngineMode == XR_MODE_UNIFIED)) &&
		(_Flags & CXR_MODEL_ONRENDERFLAGS_INVISIBLE))
		return;

// Might be able to pull this...
#ifdef CTM_SHADOWVOLUMES
	bool bShadowEnabled = (_pEngine) ? _pEngine->GetVar(XR_ENGINE_STENCILSHADOWS) != 0 : 0;
	bool bShadowVolume = (_Flags & CTM_FLAGS_SHADOW) && bShadowEnabled && _pRender->Caps_StencilDepth();
#else
	bool bShadowVolume = _pEngine->m_EngineMode == XR_MODE_UNIFIED;
#endif

#if	ENABLE_SHADOWVOLUME
#define	bSHADOWVOLUME	bShadowVolume
#else
#define	bSHADOWVOLUME	0
#endif

	bool bDrawSolid = true;
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

	CTriMesh_RenderInstanceParamters RenderParams(_pEngine, _pVBM, _pAnimState);
	CTriMesh_RenderInstanceParamters* _pRenderParams = &RenderParams;	// This is just to ease the copy-paste thingy ;)
	_pRenderParams->ClearPointers();

	_pRenderParams->m_OnRenderFlags = _Flags;
	_pRenderParams->m_CurWorldPosition = 0;//CVec3Dfp32::GetMatrixRow(_WMat, 3);
	_pRenderParams->m_pVMat = &_VMat;

	//_WMat.Multiply(_VMat, _pRenderParams->m_CurL2VMat);
	_pRenderParams->m_CurL2VMat = _VMat;

	CMat4Dfp32 VMatInv;
	_VMat.InverseOrthogonal(VMatInv);
	_pRenderParams->m_CurViewVP = CVec3Dfp32::GetMatrixRow(VMatInv, 3);
	_pRenderParams->m_CurWorldVP = _pRenderParams->m_CurViewVP;

	_pRenderParams->m_RenderInfo.Clear(_pRenderParams->m_pCurrentEngine);

	// CVec3Dfp32 ObjWPos = CVec3Dfp32::GetMatrixRow(_WMat, 3);
	if (_pViewClip)
	{
		MSCOPESHORT(CXR_Model_MultiTriMesh::_pViewClip);
		_pRenderParams->m_RenderInfo.m_pLightInfo = _pRenderParams->m_RenderLightInfo;
		_pRenderParams->m_RenderInfo.m_MaxLights = CTM_MAX_LIGHTS;
		_pRenderParams->m_RenderInfo.m_nLights = 0;

		int bClipRes = false;
		/*
		// Cannot use this since we update the boundingbox of the TriMesh when changing instances
		if (_pRenderParams->m_pCurAnimState->m_iObject)
		{
			_pViewClip->View_GetClip(_pRenderParams->m_pCurAnimState->m_iObject, &_pRenderParams->m_RenderInfo);
			bClipRes = (_pRenderParams->m_RenderInfo.m_Flags & CXR_RENDERINFO_INVISIBLE) == 0;
		}
		else
		*/
		{
			//Use local bb instead of world
			bClipRes = _pViewClip->View_GetClip_Box(m_BoundingBox.m_Min, m_BoundingBox.m_Max, 0, 0, NULL, &_pRenderParams->m_RenderInfo);
		}

		if (!bClipRes && (!_pRenderParams->m_RenderInfo.m_nLights || !bSHADOWVOLUME) )
		{
			return;
		}
	}

	//	return;

	bool bHWAnim = false;
	_pRenderParams->m_bRenderTempTLEnable = false;
	_pRenderParams->m_bRender_ProjLight = false;

	//Might be able to skip this part
	if (_pRenderParams->m_pCurrentEngine)
	{
		_pRenderParams->m_pSceneFog = _pRenderParams->m_pCurrentEngine->m_pCurrentFogState;
		_pRenderParams->m_RenderSurfOptions = _pRenderParams->m_pCurrentEngine->m_SurfOptions;
		_pRenderParams->m_RenderSurfCaps = _pRenderParams->m_pCurrentEngine->m_SurfCaps;
		_pRenderParams->m_bRender_Unified = _pRenderParams->m_pCurrentEngine->m_EngineMode == XR_MODE_UNIFIED;

		bHWAnim = (_pRenderParams->m_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_MATRIXPALETTE) != 0;
		_pRenderParams->m_bRenderTempTLEnable = _pRenderParams->m_pCurrentEngine->m_bTLEnableEnabled;

		if(_pRenderParams->m_RenderInfo.m_Flags & CXR_RENDERINFO_NHF &&
			!((_pRenderParams->m_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20) ||
			(_pRenderParams->m_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_TEXGENMODE_BOXNHF)))
		{
			_pRenderParams->m_bRenderTempTLEnable = false;
		}
	}
	else
	{
		_pRenderParams->m_pSceneFog = NULL;
		_pRenderParams->m_RenderSurfOptions = 0;
		_pRenderParams->m_RenderSurfCaps = 0;
		_pRenderParams->m_bRender_Unified = 0;
	}

	int iVariation = _pAnimState->m_Variation;
	if (iVariation >= pMesh->m_lVariations.Len())
		iVariation = 0;

	if(pMesh->m_lVariations.Len() == 0 || pMesh->m_lClusterRefs.Len() == 0)
		return;

	const CTM_Variation& Variation = pMesh->m_lVariations[iVariation];
	const CTM_ClusterRef* piClusterRefs = &pMesh->m_lClusterRefs[Variation.m_iClusterRef];
	int nClusters = Variation.m_nClusters;

	{
		pMesh->CalcBoxScissor(_pRenderParams->m_pVBM->Viewport_Get(), &_VMat, m_BoundingBox, _pRenderParams->m_RenderBoundScissor);
	}

	if (bDrawSolid || bSHADOWVOLUME)
	{
		CMat4Dfp32 UnitMat;
		UnitMat.Unit();
		pMesh->OnRender_Lights(_spWLS,UnitMat,_VMat,_pRenderParams,m_BoundingBox,false);
	}

	CMat4Dfp32* pTransform = _pRenderParams->m_pVBM->Alloc_M4(_pRenderParams->m_CurL2VMat);

/*	No shadows in current iteration
	if (m_spShadowData && 
		_pRenderParams->m_bRender_Unified && 
		!(_pRenderParams->m_RenderInfo.m_Flags & CXR_RENDERINFO_NOSHADOWVOLUMES) &&
		!(_pRenderParams->m_pCurrentEngine->m_DebugFlags & M_Bit(13)))
	{		
		_pRenderParams->m_RenderVB.Matrix_Set(pTransform);
		// Precalc shadows for all clusters
		if (!ShadowProjectHardware_ThreadSafe(_pRenderParams, pMatrixPalette)) // Out of vbheap
			return;
	}
//*/

/*
	if (m_iLOD == 0 && _pRenderParams->m_pCurAnimState->m_GUID)
		RenderDecals(_pRenderParams, _pRenderParams->m_pCurAnimState->m_GUID, pMatrixPalette, nMatrixPalette);
*/

	//	if (_pRenderParams->m_bRender_Unified)	// ms_bRender_Unified cannot be set unless _pEngine is valid
	//		_pRenderParams->m_pCurrentEngine->GetShader()->SetCurrentTransform(&_pRenderParams->m_CurVertex2WorldMat , &_VMat);
	if (_pRenderParams->m_pCurrentEngine->m_pViewClip)
	{
		const int nLights = _pRenderParams->m_RenderInfo.m_nLights;
		for(int i = 0; i < nLights; i++)
			_pRenderParams->m_pLO[i].Clear();
		_pRenderParams->m_nLOMaxLights= _pRenderParams->m_pCurrentEngine->m_pViewClip->View_Light_GetOcclusionSize();
		_pRenderParams->m_nLO= 0;
		_pRenderParams->m_LOUsage.Clear();
	}

	// Render clusters

	for (int iiC = 0; iiC < nClusters; iiC++)
	{
		MSCOPESHORT(CXR_Model_MultiTriMesh::RenderClusters);

		int iC = piClusterRefs[iiC].m_iCluster;
		int iSurf = piClusterRefs[iiC].m_iSurface;
		CTM_Cluster* pC = pMesh->GetCluster(iC);

		if(!CXR_Model_TriangleMesh::CullCluster(pC, _pRenderParams, pMesh->m_lspSurfaces.GetBasePtr(), iSurf, bDrawSolid))
			continue;
		CTM_VertexBuffer* pTVB = pMesh->GetVertexBuffer(pC->m_iVB);
/*
		if (pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUME)
		{
			if (_pRenderParams->m_OnRenderFlags & CXR_MODEL_ONRENDERFLAGS_NOSHADOWS)
				continue;
			if (_pRenderParams->m_RenderInfo.m_Flags & CXR_RENDERINFO_NOSHADOWVOLUMES)
				continue;
			if (!_pRenderParams->m_bRender_Unified)
				continue;
			if (_pRenderParams->m_pCurAnimState && (1 << (pC->m_OcclusionIndex)) & _pRenderParams->m_pCurAnimState->m_SurfaceShadowOcclusionMask) 
				continue;
			CXW_Surface* pSurf = pMesh->m_lspSurfaces[iSurf];
			if (_pRenderParams->m_pCurAnimState && pSurf->m_OcclusionMask & _pRenderParams->m_pCurAnimState->m_SurfaceShadowOcclusionMask) 
				continue;
			if (pC->m_Flags & CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE && !_pRenderParams->m_pppShadowPrimData)
				continue;
		}
		else
		{
			if (_pRenderParams->m_OnRenderFlags & CXR_MODEL_ONRENDERFLAGS_INVISIBLE)
				continue;
			if (_pRenderParams->m_RenderInfo.m_Flags & CXR_RENDERINFO_INVISIBLE)
				continue;
			if (_pRenderParams->m_pCurAnimState && (1 << (pC->m_OcclusionIndex)) & _pRenderParams->m_pCurAnimState->m_SurfaceOcclusionMask) 
				continue;

			CXW_Surface* pSurf = pMesh->m_lspSurfaces[iSurf];
			if (pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE) 
				continue;
			if (_pRenderParams->m_pCurAnimState && pSurf->m_OcclusionMask & _pRenderParams->m_pCurAnimState->m_SurfaceOcclusionMask) 
				continue;
			if (!bDrawSolid && pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT) 
				continue;
		}
*/
		//pTransform is needed- regardless of anim data
		_pRenderParams->ClearPointers();
		_pRenderParams->m_RenderVB.Matrix_Set(pTransform);
		
//Setup VB here!!
		if( !GenerateVB(_pRenderParams, _pEngine, pMesh, pTVB, iC, pMatrices, nCurrentInstances, liLOD) )
		{
			// Why continue here? If it fails with 1 VB it'll most likelly fail with everything that follows
			continue;
		}

		if (bDrawSolid)
		{
			_pRenderParams->m_lpSurfaces[0] = NULL;
			_pRenderParams->m_lpSurfaces[1] = NULL;

			CXW_Surface* pSurf = pMesh->m_lspSurfaces[iSurf]->GetSurface(_pRenderParams->m_RenderSurfOptions, _pRenderParams->m_RenderSurfCaps);
			M_ASSERT(pSurf, "No surface!");
			uint8 iGroup = pSurf->m_iGroup;
			uint8 Mode = (_pRenderParams->m_OnRenderFlags >> (CXR_MODEL_ONRENDERFLAGS_SURFMODE_BASE + iGroup)) & 1;

			const CXR_AnimState* pAnimState = _pRenderParams->m_pCurAnimState;
			CXW_Surface* pUserSurface = pAnimState->m_lpSurfaces[iGroup];
			if (pUserSurface)
				pUserSurface = pUserSurface->GetSurface(_pRenderParams->m_RenderSurfOptions, _pRenderParams->m_RenderSurfCaps);

			int iSlot = 0;
			if (!pUserSurface || Mode == 1)
			{ // Setup regular surface
				int16 iAnim = pSurf->m_iController ? pAnimState->m_Anim1 : pAnimState->m_Anim0;
				const CMTime& AnimTime = pSurf->m_iController ? pAnimState->m_AnimTime1 : pAnimState->m_AnimTime0;

				_pRenderParams->m_lpSurfaceKeyFrames[iSlot] = pSurf->GetFrame(iAnim, AnimTime);
				_pRenderParams->m_lpSurfaces[iSlot++] = pSurf;
			}

			if (pUserSurface)
			{ // Set up user-specified surface
				int16 iAnim = pUserSurface->m_iController ? pAnimState->m_Anim1 : pAnimState->m_Anim0;
				const CMTime& AnimTime = pUserSurface->m_iController ? pAnimState->m_AnimTime1 : pAnimState->m_AnimTime0;

				_pRenderParams->m_lpSurfaceKeyFrames[iSlot] = pUserSurface->GetFrame(iAnim, AnimTime);
				_pRenderParams->m_lpSurfaces[iSlot++] = pUserSurface;
			}

			bool b1;
			pMesh->VB_RenderUnified(_pRenderParams, pC, iC, b1);
		}

	// If this is really needed, we need to make it public in CXR_Model_TriangleMesh
	//	if (bDrawFilled) pMesh->Cluster_RenderSingleColor(_pRenderParams, pC, 0xffffffff);
	}

	if(_pRenderParams->m_LOUsage.IsClear())
	{
		// Need to update ViewClip's LightOcclusionInfo with alterations made
		_pRenderParams->m_pCurrentEngine->m_pViewClip->View_Light_ApplyOcclusionArray(_pRenderParams->m_nLO,_pRenderParams->m_piDstLO,_pRenderParams->m_pLO);
	}

	if (_pRenderParams->m_pSceneFog && (_pRenderParams->m_RenderInfo.m_Flags & CXR_RENDERINFO_NHF))
	{
		MSCOPESHORT(CXR_Model_MultiTriMesh::_pRenderParams->m_pSceneFog);
		_pRenderParams->m_pSceneFog->TraceBoundRelease();
	}
}


//Mostly copied from WTriMesh.cpp -
//It's important that the MultiTriMesh "Owns" the VBID's, though
void CXR_Model_MultiTriMesh::OnPrecache(CXR_Engine* _pEngine, int _iVariation)
{
	MAUTOSTRIP(CXR_Model_MultiTriMesh_OnPrecache, MAUTOSTRIP_VOID);

	TThinArray<CXR_Model_TriangleMesh*>	lpMeshList;
	lpMeshList.SetLen(m_spTriangleMesh->m_lspLOD.Len() + 1);
	lpMeshList[0] = m_spTriangleMesh;
	int i;
	for(i = 0;i < m_spTriangleMesh->m_lspLOD.Len();i++)
	{
		lpMeshList[i+1] = m_spTriangleMesh->m_lspLOD[i];
	}

	uint16 iStartCluster = 0;

	for(int ctr=0;ctr < lpMeshList.Len();ctr++)
	{
		CXR_Model_TriangleMesh * pMesh = lpMeshList[ctr];

		// URGENTFIXME: Uncompressed meshes test:
		//	DecompressAll(-1);
		//	m_lCompressedClusters.Destroy();
		MACRO_GetSystemEnvironment(pEnv);
		if(!pEnv)
			Error("OpenStream", "No env");

		int bXDF = D_MXDFCREATE;
		int Platform = D_MPLATFORM;
		pMesh->SetUsePrimitives(true);

		//DecompressAll(CTM_CLUSTERCOMP_VB);
		int nClusters = pMesh->GetNumVertexBuffers();
		for(int i = 0; i < nClusters; i++)
		{
			CTM_VertexBuffer* pC = pMesh->GetVertexBuffer(i);
			if (pC->m_VBID == 0)
			{
				pC->m_VBID = m_pVBCtx->AllocID(m_iVBC, iStartCluster + i);
			}
		}
		iStartCluster += nClusters;

		MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
		if (!pTC) Error("Read", "No texture-context available.");

		// Init surfaces
		uint iVariation = _iVariation;
		if (iVariation >= pMesh->m_lVariations.Len())
			iVariation = 0;

		if(pMesh->m_lVariations.Len() == 0 || pMesh->m_lClusterRefs.Len() == 0)
			return;

		const CTM_Variation& Variation = pMesh->m_lVariations[iVariation];
		const CTM_ClusterRef* piClusterRefs = &pMesh->m_lClusterRefs[Variation.m_iClusterRef];
		for(int iiC = 0; iiC < nClusters; iiC++)
		{
			//		int iC = piClusterRefs[iiC].m_iCluster;
			int iSurf = piClusterRefs[iiC].m_iSurface;

			CXW_Surface* pS = pMesh->m_lspSurfaces[iSurf];
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

		CCFile ModelFile;

		// Set precache flags on all vertex buffers
		{
			int nVB = pMesh->GetNumVertexBuffers();
			for(int i = 0; i < nVB; i++)
			{
				CTM_VertexBuffer* pVB = pMesh->GetVertexBuffer(i);
	#if		defined(PLATFORM_PS2)
				if( pVB->m_VBFlags & (CTM_CLUSTERFLAGS_SHADOWVOLUME|CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE) )
					continue;

	#elif	defined (PLATFORM_WIN_PC)
				if( ( pVB->m_VBFlags & (CTM_CLUSTERFLAGS_SHADOWVOLUME|CTM_CLUSTERFLAGS_SHADOWVOLUMESOFTWARE) ) && ( bXDF && Platform == 2/*e_Platform_PS2*/ ) )
					continue;

	#endif

//				pVB->GetBDMatrixMap(pMesh, &ModelFile);
				//			pC->m_bSave_BoneDeform = true;
//				pVB->m_bSave_BoneDeformMatrixMap = true;
				int VBID = pVB->m_VBID;
				if (VBID)
					_pEngine->m_pVBC->VB_SetFlags(VBID, _pEngine->m_pVBC->VB_GetFlags(VBID) | CXR_VBFLAGS_PRECACHE);

				pVB->m_bSave_Normals = true;
				pVB->m_bSave_Primitives = true;
				pVB->m_bSave_Vertices = true;
				pVB->m_bSave_TVertices = true;
				pVB->m_bSave_TangentU = true;
				pVB->m_bSave_TangentV = true;
				pVB->GetVertexPtr(pMesh, 0, &ModelFile);
				pVB->GetNormalPtr(pMesh, 0, &ModelFile);
				if( pVB->GetTVertexPtr(pMesh, 0, &ModelFile) )
				{
					pVB->GetTangentUPtr(pMesh, 0, &ModelFile);
					pVB->GetTangentVPtr(pMesh, 0, &ModelFile);
				}
				pVB->GetPrimitives(pMesh, &ModelFile);
			}
		}
		pMesh->InitPreloader(_pEngine, bXDF, Platform);

	// Shadow data for multitrimesh-objects isn't supported, but it might be in the future so this is left in
		if (pMesh->m_spShadowData)
		{
			// Preload these 
			pMesh->m_spShadowData->m_VertexBuffer.GetVertexPtr(pMesh, 0, &ModelFile);
			pMesh->m_spShadowData->m_VertexBuffer.GetEdges(pMesh, &ModelFile);
			pMesh->m_spShadowData->m_VertexBuffer.GetEdgeTris(pMesh, &ModelFile);
			pMesh->m_spShadowData->m_VertexBuffer.GetTriangles(pMesh, &ModelFile);
			pMesh->m_spShadowData->m_VertexBuffer.GetBDVertexInfo(pMesh, &ModelFile);
			pMesh->m_spShadowData->m_VertexBuffer.GetBDInfluence(pMesh, &ModelFile);

			pMesh->m_spShadowData->m_lnVertices.SetLen(pMesh->m_spShadowData->m_lRenderData.Len());

			for (int i = 0; i < pMesh->m_spShadowData->m_lnVertices.Len(); ++i)
			{
				CTM_VertexBuffer *pCluster = pMesh->GetVertexBuffer(pMesh->m_spShadowData->m_lRenderData[i].m_iShadowCluster);
				pMesh->m_spShadowData->m_lnVertices[i] = pCluster ? pCluster->GetNumVertices(pMesh, &ModelFile) : 0;
			}
		}
	}
}

void CXR_Model_MultiTriMesh::OnPostPrecache(CXR_Engine * _pEngine)
{
	return m_spTriangleMesh->OnPostPrecache(_pEngine);
}

void CXR_Model_MultiTriMesh::OnResourceRefresh(int _Flags)
{
	return m_spTriangleMesh->OnResourceRefresh(_Flags);
}

CXR_WallmarkInterface * CXR_Model_MultiTriMesh::Wallmark_GetInterface()
{
	return m_spTriangleMesh->Wallmark_GetInterface();
}

CXR_PhysicsModel * CXR_Model_MultiTriMesh::Phys_GetInterface()
{
	return m_spTriangleMesh->Phys_GetInterface();
}


// -------------------------------------------------------------------
// Derived from CXR_VBContainer

// Most of these functions do the same things as their CXR_Model_TriangleMesh counterparts,
// although these all work with the clusters in the LODs as well

int CXR_Model_MultiTriMesh::GetNumLocal()
{
	return m_spTriangleMesh->GetNumVertexBuffers() * (1 + m_spTriangleMesh->m_lspLOD.Len());
}


int CXR_Model_MultiTriMesh::GetID(int _iLocal)
{
	CXR_Model_TriangleMesh * pMesh = m_spTriangleMesh;
	if (_iLocal >= m_spTriangleMesh->GetNumVertexBuffers())
	{
		int iLod = _iLocal / m_spTriangleMesh->GetNumVertexBuffers();
		_iLocal -= iLod * m_spTriangleMesh->GetNumVertexBuffers();
		pMesh = m_spTriangleMesh->m_lspLOD[iLod-1];
	}

	if (_iLocal < pMesh->GetNumVertexBuffers()) 
		return pMesh->GetVertexBuffer(_iLocal)->m_VBID;
	else
		return 0;
}

CFStr CXR_Model_MultiTriMesh::GetName(int _iLocal)
{
	if( _iLocal < m_spTriangleMesh->GetNumVertexBuffers() )
	{
		return m_spTriangleMesh->GetName(_iLocal);
	}

	int iLod = _iLocal / m_spTriangleMesh->GetNumVertexBuffers();
	_iLocal -= iLod * m_spTriangleMesh->GetNumVertexBuffers();
	return m_spTriangleMesh->m_lspLOD[iLod-1]->GetName(_iLocal);
}

void CXR_Model_MultiTriMesh::Get(int _iLocal, CRC_BuildVertexBuffer& _VB, int _Flags)
{
	CXR_Model_TriangleMesh * pMesh = m_spTriangleMesh;
	if( _iLocal >= m_spTriangleMesh->GetNumVertexBuffers() )
	{
		int iLod = _iLocal / m_spTriangleMesh->GetNumVertexBuffers();
		_iLocal -= iLod * m_spTriangleMesh->GetNumVertexBuffers();
		pMesh = m_spTriangleMesh->m_lspLOD[iLod-1];
	}

	pMesh->Get(_iLocal,_VB,_Flags);

	//If we got here with a GPU mesh, we need to change the number of primitives/vertices available
#ifndef MTMESH_USE_CPU
	/*
	//Set the instance counter
	CTM_VertexBuffer * pC = pMesh->GetVertexBuffer(_iLocal);
	_VB.m_nPrim = (pC->GetNumPrimitives() / m_nMaxInstances) * m_nCurrentInstances;
	_VB.m_nV = (pC->GetNumVertices() / m_nMaxInstances) * m_nCurrentInstances;
	*/
#endif

}

void CXR_Model_MultiTriMesh::Release(int _iLocal)
{
	if( _iLocal < m_spTriangleMesh->GetNumVertexBuffers() )
	{
		return m_spTriangleMesh->Release(_iLocal);
	}

	int iLod = _iLocal / m_spTriangleMesh->GetNumVertexBuffers();
	_iLocal -= iLod * m_spTriangleMesh->GetNumVertexBuffers();
	return m_spTriangleMesh->m_lspLOD[iLod-1]->Release(_iLocal);
}


// -------------------------------------------------------------------
// Local functions


#ifndef MTMESH_USE_CPU

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	GenerateInstances	

Parameters:		
_nInstances:Number of Instances to generate
_pMesh:		Mesh to generate instances in

Comments:	duplicates entries in all clusters and assigns each copy
			a specific bone ID
\*____________________________________________________________________*/ 
/*
void CXR_Model_MultiTriMesh::GenerateInstances(uint16 _nInstances,CXR_Model_TriangleMesh * _pMesh)
{
	MAUTOSTRIP(CXR_Model_MultiTriMesh_GenerateInstances, MAUTOSTRIP_VOID);

	m_nMaxInstances = _nInstances;
	m_nCurrentInstances = 1;

	// Temp disable shadows for testing
	_pMesh->m_spShadowData = NULL;
	_pMesh->m_bMatrixPalette = true;

	for(uint32 i = 0; i < _pMesh->GetNumVertexBuffers(); i++)
	{
		CTM_VertexBuffer &Cls = * _pMesh->GetVertexBuffer(i);
		uint16 * data = Cls.m_lPrimitives.GetBasePtr();

		const uint32 nVertices = Cls.GetNumVertices();
		const uint32 nTriangles = Cls.m_lTriangles.Len();
		const uint32 nEdges = Cls.m_lEdges.Len();
		const uint32 nPrim = Cls.m_lPrimitives.Len();

		//Create space for new data
		int j,k;
		for(j = 0;j < Cls.m_lVFrames.Len();j++)
		{
			uint32 nElem = Cls.m_lVFrames[j].m_lVertices.Len() * _nInstances;
			Cls.m_lVFrames[j].m_lVertices.SetLen(nElem);

			Cls.m_BoundRadius = 0;
			for(int ct=0;ct < nVertices;ct++)
			{
				Cls.m_BoundRadius = Max(Cls.m_BoundRadius,Cls.m_lVFrames[j].m_lVertices[ct].LengthSqr());
			}
			Cls.m_BoundRadius = M_Sqrt(Cls.m_BoundRadius);

			if( Cls.m_lVFrames[j].m_lNormals.Len() > 0 ) Cls.m_lVFrames[j].m_lNormals.SetLen(nElem);
			if( Cls.m_lVFrames[j].m_lTangentU.Len() > 0 ) Cls.m_lVFrames[j].m_lTangentU.SetLen(nElem);
			if( Cls.m_lVFrames[j].m_lTangentV.Len() > 0 ) Cls.m_lVFrames[j].m_lTangentV.SetLen(nElem);
		}

		for(j = 0;j < Cls.m_lTVFrames.Len();j++)
		{
			Cls.m_lTVFrames[j].m_lVertices.SetLen(Cls.m_lTVFrames[j].m_lVertices.Len() * _nInstances);
		}

		//We only need one "bone" with 100% influence per instance. And it's not really a bone either.
		Cls.m_lBDVertexInfo.SetLen(nVertices * _nInstances);
		Cls.m_lBDInfluence.SetLen(_nInstances);

		for(j = 0;j < _nInstances;j++)
		{
			Cls.m_lBDInfluence[j].m_iBone = j;
			Cls.m_lBDInfluence[j].m_Influence = 1.0f;

			for(k = 0;k < nVertices;k++)
			{
				Cls.m_lBDVertexInfo[k + j * nVertices].m_nBones = 1;
				Cls.m_lBDVertexInfo[k + j * nVertices].m_iBoneInfluence = j;
			}
		}

		Cls.m_lTriangles.SetLen(nTriangles * _nInstances);
		Cls.m_lEdges.SetLen(nEdges * _nInstances);
		Cls.m_lPrimitives.SetLen(nPrim * _nInstances);
		Cls.m_lEdgeTris.SetLen(nEdges * _nInstances);
		Cls.m_lTriEdges.SetLen(nTriangles * _nInstances);

		Cls.m_MaxUsedBones = _nInstances;
		Cls.m_bHaveBones = true;

		for(j = 1;j < _nInstances;j++)
		{
			uint32 iStart = nVertices * j;

			//New Vertex info
			for(k = 0;k < Cls.m_lVFrames.Len();k++)
			{
				memcpy( Cls.m_lVFrames[k].m_lVertices.GetBasePtr() + iStart, 
						Cls.m_lVFrames[k].m_lVertices.GetBasePtr(), sizeof(CVec3Dfp32) * nVertices );

				if( Cls.m_lVFrames[k].m_lNormals.Len() > 0 ) 
					memcpy( Cls.m_lVFrames[k].m_lNormals.GetBasePtr() + iStart, 
							Cls.m_lVFrames[k].m_lNormals.GetBasePtr(), sizeof(CVec3Dfp32) * nVertices );
				if( Cls.m_lVFrames[k].m_lTangentU.Len() > 0 ) 
					memcpy( Cls.m_lVFrames[k].m_lTangentU.GetBasePtr() + iStart, 
							Cls.m_lVFrames[k].m_lTangentU.GetBasePtr(), sizeof(CVec3Dfp32) * nVertices );
				if( Cls.m_lVFrames[k].m_lTangentV.Len() > 0 ) 
					memcpy( Cls.m_lVFrames[k].m_lTangentV.GetBasePtr() + iStart, 
							Cls.m_lVFrames[k].m_lTangentV.GetBasePtr(), sizeof(CVec3Dfp32) * nVertices );
			}

			//New Texture Vertex info
			for(k = 0;k < Cls.m_lTVFrames.Len();k++)
			{
				memcpy( Cls.m_lTVFrames[k].m_lVertices.GetBasePtr() + iStart,
						Cls.m_lTVFrames[k].m_lVertices.GetBasePtr(), sizeof(CVec2Dfp32) * nVertices );
			}

			//New primitive info
			iStart = nPrim * j;
			k = 0;
			while( k < nPrim )
			{
				uint16 l,Len = Cls.m_lPrimitives[k+1];
				Cls.m_lPrimitives[k+iStart] = Cls.m_lPrimitives[k];
				Cls.m_lPrimitives[k+iStart+1] = Len;
				for(l = 0;l < Len;l++)
				{
					Cls.m_lPrimitives[k+iStart+2+l] = Cls.m_lPrimitives[k+2+l] + nVertices * j;
				}
				k += (Len+2);
			}

			//New triangle & edge info
			//Since we're using primitives for drawing and disregard shadows, this can be skipped if necessary
			iStart = nTriangles * j;
			for(k = 0;k < nTriangles;k++)
			{
				CTM_Triangle &Tri = Cls.m_lTriangles[iStart + k];
				Tri = Cls.m_lTriangles[k];
				Tri.m_iV[0] += nVertices * j;
				Tri.m_iV[1] += nVertices * j;
				Tri.m_iV[2] += nVertices * j;
			}

			iStart = nEdges * j;
			for(k = 0;k < nEdges;k++)
			{
				CTM_Edge &Edge = Cls.m_lEdges[iStart + k];
				Edge = Cls.m_lEdges[k];
				Edge.m_iV[0] += nVertices * j;
				Edge.m_iV[1] += nVertices * j;
			}

			//Referencing
			iStart = nTriangles * j;
			for(k = 0;k < nTriangles;k++)
			{
				CTM_TriEdges &TE = Cls.m_lTriEdges[iStart + k];
				TE = Cls.m_lTriEdges[k];
				TE.m_iEdges[0] += nEdges * j;
				TE.m_iEdges[1] += nEdges * j;
				TE.m_iEdges[2] += nEdges * j;
			}

			iStart = nEdges * j;
			for(k = 0;k < nEdges;k++)
			{
				CTM_EdgeTris &ET = Cls.m_lEdgeTris[iStart + k];
				ET = Cls.m_lEdgeTris[k];
				ET.m_iTri[0] += nTriangles * j;
				ET.m_iTri[1] += nTriangles * j;
			}
		}
	}

	//Set instances- to be on the safe side
	SetInstances(NULL,m_nCurrentInstances);
}
*/

#else

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	GenerateInstances	

Parameters:		
_nInstances:Number of Instances to generate
_pMesh:		Mesh to generate instances in

Comments:	Just create space for matrices
\*____________________________________________________________________*/ 
void CXR_Model_MultiTriMesh::GenerateInstances(CXR_Model_TriangleMesh * _pMesh)
{
	MAUTOSTRIP(CXR_Model_MultiTriMesh_GenerateInstances, MAUTOSTRIP_VOID);
	// m_nMaxInstances = _nInstances;
	// m_nCurrentInstances = _nInstances;

//	m_lMatrices.SetLen(_nInstances);
//	m_lRenderMatrices.SetLen(_nInstances);
//	m_lLOD.SetLen(_nInstances);

	//Temp disable shadows
	_pMesh->m_spShadowData = NULL;

	//Model file
	CCFile ModelFile;

	//Make sure bound radius is generated for each cluster
	int nClusters = _pMesh->GetNumClusters();
	for(int iC = 0; iC < nClusters; iC++)
	{
		CTM_Cluster* pC = _pMesh->GetCluster(iC);
		CTM_VertexBuffer* pVB = _pMesh->GetVertexBuffer(pC->m_iVB);
		CVec3Dfp32* pV = pVB->GetVertexPtr(_pMesh, 0, &ModelFile);
		uint16* piPrim = pVB->GetTriangles(_pMesh, &ModelFile);
		int nPrim = pVB->GetNumTriangles(_pMesh) * 3;
		if(pC->m_nIBPrim)
		{
			CTM_VertexBuffer* pIB = _pMesh->GetVertexBuffer(pC->m_iIB);
			piPrim = pIB->GetTriangles(_pMesh, &ModelFile) + pC->m_iIBOffset;
			nPrim = pC->m_nIBPrim;
		}

		fp32 BoundRadius = 0;
		for(int iP = 0; iP < nPrim; iP++)
		{
			uint iPrim = piPrim[iP];
			BoundRadius = Max(BoundRadius, pV[iPrim].LengthSqr());
		}
		pC->m_BoundRadius = M_Sqrt(BoundRadius);
	}

	//Create identity matrices
	/*
	for(int ct=0;ct < _nInstances;ct++)
	{
		m_lMatrices[ct].Unit();
		m_lMatrices[ct].k[3][0] = ct * 32;
	}
	*/

	//Make sure to initiate everything else
	// SetInstances(NULL,1);
}

#endif


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Generates multiple triangle clusters
\*____________________________________________________________________*/ 
/* Function not needed since CPU trimeshes are rendered in immediate mode
void CXR_Model_MultiTriMesh::GenerateLODClusters()
{
	MAUTOSTRIP(CXR_Model_MultiTriMesh_GenerateLODClusters, MAUTOSTRIP_VOID);

#ifdef PLATFORM_CONSOLE
	for(int i = 0;i < m_spTriangleMesh->m_lClusters.Len();i++)
	{
		CTM_VertexBuffer & Cl = m_spTriangleMesh->m_lClusters[i];
#else
	for(int i = 0;i < m_spTriangleMesh->m_lspClusters.Len();i++)
	{
		CTM_VertexBuffer & Cl = *m_spTriangleMesh->m_lspClusters[i];
#endif

		uint16 nVert = 0;
		uint16 nPrim = 0;

		CVec3Dfp32 * pVert = Cl.GetVertexPtr(0);
		CVec3Dfp32 * pNorm = Cl.GetNormalPtr(0);
		CVec2Dfp32 * pTVer = Cl.GetTVertexPtr(0);
		CVec3Dfp32 * pTanU = pTVer ? Cl.GetTangentUPtr(0) : NULL;
		CVec3Dfp32 * pTanV = pTVer ? Cl.GetTangentVPtr(0) : NULL;
		
		uint16	  * pPrim = Cl.GetPrimitives();

		for(int j=0;j < m_nCurrentInstances;j++)
		{
			int k;
			uint16 nClVerts,nClPrims;

			CVec3Dfp32 *pNVert,*pNNorm,*pNTanU,*pNTanV;
			CVec2Dfp32 *pNTVer;
			uint16	  *pNPrim;

			if( m_lLOD[j] == 0 )
			{
				CTM_VertexBuffer_Copy & Cls = m_lClusterCopies[i];
				nClVerts = Cls.m_lVFrames[0].m_lVertices.Len();
				nClPrims = Cls.m_lPrimitives.Len();

				pNVert = Cls.m_lVFrames[0].m_lVertices.GetBasePtr();
				pNNorm = Cls.m_lVFrames[0].m_lNormals.GetBasePtr();
				if( pTVer )
				{
					pNTanU = Cls.m_lVFrames[0].m_lTangentU.GetBasePtr();
					pNTanV = Cls.m_lVFrames[0].m_lTangentV.GetBasePtr();

					//Tangents are created in postprocessing, so they might need to be
					//updated
					if( pNTanU == NULL )
					{
						Cls.m_lVFrames[0].m_lTangentU.SetLen(nClVerts);
						Cls.m_lVFrames[0].m_lTangentV.SetLen(nClVerts);
						memcpy(Cls.m_lVFrames[0].m_lTangentU.GetBasePtr(),
							Cl.m_lVFrames[0].m_lTangentU.GetBasePtr(),
							Cls.m_lVFrames[0].m_lTangentU.ListSize());
						memcpy(Cls.m_lVFrames[0].m_lTangentV.GetBasePtr(),
							Cl.m_lVFrames[0].m_lTangentV.GetBasePtr(),
							Cls.m_lVFrames[0].m_lTangentV.ListSize());
						pNTanU = Cls.m_lVFrames[0].m_lTangentU.GetBasePtr();
						pNTanV = Cls.m_lVFrames[0].m_lTangentV.GetBasePtr();
					}
				}
				else
				{
					pNTanV = pNTanU = NULL;
				}
				pNPrim = Cls.m_lPrimitives.GetBasePtr();
				pNTVer = (pTVer) ? Cls.m_lTVFrames[0].m_lVertices.GetBasePtr() : NULL;
			}
			else
			{
#ifdef PLATFORM_CONSOLE
				CTM_VertexBuffer & Cls = m_spTriangleMesh->m_lspLOD[m_lLOD[j]-1]->m_lClusters[i];
#else
				CTM_VertexBuffer & Cls = *m_spTriangleMesh->m_lspLOD[m_lLOD[j]-1]->m_lspClusters[i];
#endif
				nClVerts = Cls.GetNumVertices();
				nClPrims = Cls.GetNumPrimitives();

				pNVert = Cls.GetVertexPtr(0);
				pNNorm = Cls.GetNormalPtr(0);
				pNTanU = pTVer ? Cls.GetTangentUPtr(0) : NULL;
				pNTanV = pTVer ? Cls.GetTangentVPtr(0) : NULL;
				pNPrim = Cls.GetPrimitives();
				pNTVer = Cls.GetTVertexPtr(0);
			}

			if( pTVer )
			{
				for(k = 0; k < nClVerts; k++)
				{
					pNVert[k].MultiplyMatrix(m_lMatrices[j],pVert[k + nVert]);
					pNorm[k + nVert] = pNNorm[k];
					pTanU[k + nVert] = pNTanU[k];
					pTanV[k + nVert] = pNTanV[k];
					pTVer[k + nVert] = pNTVer[k];
					pNorm[k + nVert].MultiplyMatrix3x3(m_lMatrices[j]);
					pTanU[k + nVert].MultiplyMatrix3x3(m_lMatrices[j]);
					pTanV[k + nVert].MultiplyMatrix3x3(m_lMatrices[j]);
				}
			}
			else
			{
				for(k = 0; k < nClVerts; k++)
				{
					pNVert[k].MultiplyMatrix(m_lMatrices[j],pVert[k + nVert]);
					pNorm[k + nVert] = pNNorm[k];
					pNorm[k + nVert].MultiplyMatrix3x3(m_lMatrices[j]);
				}
			}

			k = 0;
			while( k < nClPrims )
			{
				uint16 l,Len = pNPrim[k+1];
				pPrim[k+nPrim] = pNPrim[k];
				pPrim[k+nPrim+1] = Len;
				for(l = 0; l < Len; l++)
				{
					pPrim[k+nPrim+2+l] = pNPrim[k+2+l] + nVert;
				}
				k += Len + 2;
			}

			nVert += nClVerts;
			nPrim += nClPrims;
		}

		m_lClusterCopies[i].m_nPrimTotal = nPrim;
		m_lClusterCopies[i].m_bSetLen = true;

		//We need to regenerate the list...
		if( Cl.m_VBID != 0) 
			m_pVBCtx->VB_MakeDirty(Cl.m_VBID);
	}
}
*/


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	SetInstances	

Parameters:		
_pPos:		Pointer to an array of matrices containing the orientation
			of the instances
_pMesh:		number of matrices in the array
\*____________________________________________________________________*/ 
void	CXR_Model_MultiTriMesh::SetInstances(CMat4Dfp32 * _pPos,uint8 _nPos)
{
	MAUTOSTRIP(CXR_Model_MultiTriMesh_SetInstances, MAUTOSTRIP_VOID);
	_nPos = Clamp((uint32)_nPos, (uint32)1, (uint32)CTM_MULTITRIMESH_MAXINSTANCES);

#ifndef MTMESH_USE_CPU

	/*
	// If we're doing this in hardware, we need to recreate the vertex buffers
	// when changing the number of instances
	if( m_nCurrentInstances != _nPos )
	{
		for(i = -1;i < m_spTriangleMesh->m_lspLOD.Len();i++)
		{
			CXR_Model_TriangleMesh * pMesh = (i < 0) ? m_spTriangleMesh : m_spTriangleMesh->m_lspLOD[i];

			for(j = 0;j < pMesh->GetNumVertexBuffers();j++)
			{
				m_pVBCtx->VB_MakeDirty(pMesh->GetVertexBuffer(j)->m_VBID);
			}
		}
	}

	//Update "current" counter
	m_nCurrentInstances = _nPos;
	*/

#endif

	if( !_pPos ) return;

	//Update bounding box
	// We only need to do this for the LODs if GPU rendering is active,
	// since the CPU method always uses the root model
#ifndef MTMESH_USE_CPU
	/*
	for(int iMesh = -1;iMesh < m_spTriangleMesh->m_lspLOD.Len();iMesh++)
	{
		CXR_Model_TriangleMesh * pMesh = (iMesh < 0) ? m_spTriangleMesh : m_spTriangleMesh->m_lspLOD[iMesh];
		*/
#else
	CXR_Model_TriangleMesh * pMesh = m_spTriangleMesh;
#endif

	for(int iC = 0; iC < pMesh->GetNumClusters(); iC++)
	{
		CTM_Cluster* pC = pMesh->GetCluster(iC);

		// Add all positions to the box...
		pC->m_BoundBox.m_Min = pC->m_BoundBox.m_Max = CVec3Dfp32::GetMatrixRow(_pPos[0],3);
		for(int j = 1; j < _nPos; j++)
		{
			pC->m_BoundBox.Expand(CVec3Dfp32::GetMatrixRow(_pPos[j],3));
		}

		//Grow the box by the maximum radius...
		pC->m_BoundBox.Grow(pC->m_BoundRadius);

		//Add to the mesh bounding box
		if( !iC ) m_BoundingBox = pC->m_BoundBox;
		else m_BoundingBox.Expand(pC->m_BoundBox);
	}

#ifndef MTMESH_USE_CPU
	/*
	}
	*/
#endif

}

static int Align(int _nSize, int _Align)
{
	return (_nSize + _Align - 1) & ~(_Align - 1);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
Function:	Sets up the Vertex Buffer for the rendering

Parameters:		
_VB:		The target vertex buffer
_pVBM:		Manager to use

Returns:	true on success, false on failure
\*____________________________________________________________________*/ 
bool	CXR_Model_MultiTriMesh::GenerateVB(CTriMesh_RenderInstanceParamters * _pRenderParams,CXR_Engine * _pEngine,
										   CXR_Model_TriangleMesh * _pMesh,CTM_VertexBuffer * _pC,int _iC,CMat4Dfp32 * _pMatrices,
										   uint16 _nMatrices,uint8 * _piLOD)
{
	MAUTOSTRIP(CXR_Model_MultiTriMesh_GenerateVB, MAUTOSTRIP_VOID);

#ifndef MTMESH_USE_CPU

	/*
	//GPU method- just update the matrix palette since we have the VBID
	uint16 nData = (uint16)(sizeof(CMat4Dfp32) * _nMatrices);
	CMat4Dfp32 * pMat = (CMat4Dfp32*)_pRenderParams->m_pVBM->Alloc(nData);
	memcpy(pMat,_pMatrices,nData);
	if( !_pMesh->Cluster_SetMatrixPalette(_pRenderParams, _pC, pMat, _nMatrices, &_pRenderParams->m_RenderVB) ||
		!_pRenderParams->m_RenderVB.SetVBChain(_pRenderParams->m_pVBM,true) )
		return false;

	_pRenderParams->m_RenderVB.Render_VertexBuffer(_pC->m_VBID);
	*/
#else

	//Cannot render these!

	/*
	if( (_pC->m_lVFrames[0].m_lTangentV.GetBasePtr() == NULL) ||
		(_pC->m_lVFrames[0].m_lTangentU.GetBasePtr() == NULL) )
		return false;
		*/

	//Determine the alloc size to minimize calls to VBM->Alloc
	// NOTE!
	// It would be possible to save some memory here by using the LOD levels to determine
	// the exact memory usage before acquiring it instead of just getting the largest possible,
	// this would require looping through all instances and verifying, so I'm not sure
	// it's worth the while - ae
	
	/*
	uint16 nTotalVert = _pC->m_lVFrames[0].m_lVertices.Len() * m_nCurrentInstances;
	uint16 nPrimitives = _pC->m_lPrimitives.Len();
	*/

	//This method is now in session - ae
	uint16 nTotalVert = 0;
	uint16 nPrimitives = 0;
	if( m_spTriangleMesh->m_lspLOD.Len() > 0 )
	{
		for(int i=0; i < _nMatrices; i++)
		{
			CXR_Model_TriangleMesh* pMesh = (_piLOD[i] == 0) ? _pMesh : ((CXR_Model_TriangleMesh*)m_spTriangleMesh->m_lspLOD[_piLOD[i]-1]);
			CTM_VertexBuffer *pC = (_piLOD[i] == 0) ? _pC : m_spTriangleMesh->m_lspLOD[_piLOD[i]-1]->GetVertexBuffer(_iC);
			nTotalVert += pC->GetNumVertices(pMesh);
			nPrimitives += pC->GetNumPrimitives(pMesh);
		}
	}
	else
	{
		nTotalVert = _pC->m_lVFrames[0].m_lVertices.Len() * _nMatrices;
		nPrimitives = _pC->m_lPrimitives.Len() * _nMatrices;
	}
	
	int nAllocBytes = Align(sizeof( CXR_VBChain ),4) +
						 Align((nTotalVert * (4 * sizeof(CVec3Dfp32) + sizeof(CVec2Dfp32))),4) +
						 Align((nPrimitives * sizeof(uint16)),4);

	uint8 * pData = (uint8*)_pRenderParams->m_pVBM->Alloc(nAllocBytes);

	//Out-of-memory
	if( !pData ) return false;

	CXR_VBChain * pChain = (CXR_VBChain*)pData;
	pData += sizeof(CXR_VBChain);

//Copy vertices into the array
	uint16 nVert = 0;
	uint16 nPrim = 0;

	CVec3Dfp32 * pVert = (CVec3Dfp32*)pData;
	pData += nTotalVert * sizeof(CVec3Dfp32);
	CVec3Dfp32 * pNorm = (CVec3Dfp32*)pData;
	pData += nTotalVert * sizeof(CVec3Dfp32);
	CVec2Dfp32 * pTVer = (CVec2Dfp32*)pData;
	pData += nTotalVert * sizeof(CVec2Dfp32);
	CVec3Dfp32 * pTanU = pTVer ? (CVec3Dfp32*)pData : NULL;
	pData += nTotalVert * sizeof(CVec3Dfp32);
	CVec3Dfp32 * pTanV = pTVer ? (CVec3Dfp32*)pData : NULL;
	pData += nTotalVert * sizeof(CVec3Dfp32);

	uint16	  * pPrim = (uint16*)pData;

	//Add LOD versions of all instances
	for(int j=0;j < _nMatrices;j++)
	{
		int k;
		uint16 nClVerts,nClPrims;

		CVec3Dfp32 *pNVert,*pNNorm,*pNTanU,*pNTanV;
		CVec2Dfp32 *pNTVer;
		uint16	  *pNPrim;

		CXR_Model_TriangleMesh* pMesh = (_piLOD[j] == 0) ? _pMesh : ((CXR_Model_TriangleMesh*)m_spTriangleMesh->m_lspLOD[_piLOD[j]-1]);
		CTM_VertexBuffer & Cls = (_piLOD[j] == 0) ? *_pC : *m_spTriangleMesh->m_lspLOD[_piLOD[j]-1]->GetVertexBuffer(_iC);

		nClVerts = Cls.GetNumVertices(pMesh);
		nClPrims = Cls.GetNumPrimitives(pMesh);

		pNVert = Cls.GetVertexPtr(pMesh, 0);
		pNNorm = Cls.GetNormalPtr(pMesh, 0);
		pNTVer = Cls.GetTVertexPtr(pMesh, 0);
		if (!pNTVer)
			continue;

		pNTanU = Cls.GetTangentUPtr(pMesh, 0);
		pNTanV = Cls.GetTangentVPtr(pMesh, 0);
		pNPrim = Cls.GetPrimitives(pMesh);

		for(k = 0; k < nClVerts; k++)
		{
			pNVert[k].MultiplyMatrix(_pMatrices[j],pVert[k + nVert]);
			pNorm[k + nVert] = pNNorm[k];
			pTanU[k + nVert] = pNTanU[k];
			pTanV[k + nVert] = pNTanV[k];
			pTVer[k + nVert] = pNTVer[k];
			pNorm[k + nVert].MultiplyMatrix3x3(_pMatrices[j]);
			pTanU[k + nVert].MultiplyMatrix3x3(_pMatrices[j]);
			pTanV[k + nVert].MultiplyMatrix3x3(_pMatrices[j]);
		}

		k = 0;
		while( k < nClPrims )
		{
			uint16 l,Len = pNPrim[k+1];
			pPrim[k+nPrim] = pNPrim[k];
			pPrim[k+nPrim+1] = Len;
			for(l = 0; l < Len; l++)
			{
				pPrim[k+nPrim+2+l] = pNPrim[k+2+l] + nVert;
			}
			k += Len + 2;
		}

		nVert += nClVerts;
		nPrim += nClPrims;
	}

	//Set the VB
	pChain->Clear();
	CXR_VertexBuffer &VB = _pRenderParams->m_RenderVB;
	VB.SetVBChain(pChain);
	VB.Geometry_VertexArray(pVert,nVert,true);
	VB.Geometry_TVertexArray(pTVer,0);
	VB.Geometry_Normal(pNorm);
	VB.Geometry_TVertexArray(pTanU,1);
	VB.Geometry_TVertexArray(pTanV,2);

	_pRenderParams->m_pRenderN = pNorm;
	_pRenderParams->m_pRenderTangU = pTanU;
	_pRenderParams->m_pRenderTangV = pTanV;

	_pRenderParams->m_RenderVB.Render_IndexedPrimitives(pPrim,nPrim);
	
#endif

	return true;
}
