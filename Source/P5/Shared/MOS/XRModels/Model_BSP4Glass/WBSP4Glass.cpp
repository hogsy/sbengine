#include "PCH.h"
#include "WBSP4Glass.h"
#include "WBSP4GlassDelaunay2D.h"
#include "../../Classes/Render/MWireContainer.h"
//#include "../../XR/XRShader.h"


#if GLASS_OPTIMIZE_OFF
#pragma xrMsg("optimize off!")
#pragma optimize("", off)
#pragma inline_depth(0)
#endif


// Implement dynamic
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_BSP4Glass, CXR_Model_BSP4);


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Statics
|__________________________________________________________________________________________________
\*************************************************************************************************/
uint16 CXR_Model_BSP4Glass_Instance::m_lP[GLASS_NUM_INDICES];
uint32 CXR_Model_BSP4Glass_Instance::m_lMI[GLASS_NUM_MATRIXINDICES];
fp32 CXR_Model_BSP4Glass_Instance::m_lMW[GLASS_NUM_MATRIXWEIGHTS];
CMat4Dfp32 CXR_Model_BSP4Glass_Instance::m_lRandRotMat[GLASS_ROT_MAT_MAX];
bool CXR_Model_BSP4Glass_Instance::m_bStaticInit = false;

//CRC_Attributes CXR_Model_BSP4Glass::ms_UnifiedZBuffer;


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| Velocity callbacks
|__________________________________________________________________________________________________
\*************************************************************************************************/
void GlassDefaultVelocityCB(CVec3Dfp32& _Velocity, const CVec3Dfp32& _Vertex, const CBSP4Glass_CrushData& _CrushData)
{
	_Velocity = CVec3Dfp32(0,0,_CrushData.m_ForceScale);
}


void GlassVelocityFromPointCB(CVec3Dfp32& _Velocity, const CVec3Dfp32& _Vertex, const CBSP4Glass_CrushData& _CrushData)
{
	_Velocity.k[_CrushData.m_iComp0] = (_Vertex.k[0] - _CrushData.m_HitPos.k[0]) * _CrushData.m_ForceScale;
	_Velocity.k[_CrushData.m_iComp1] = (_Vertex.k[1] - _CrushData.m_HitPos.k[1]) * _CrushData.m_ForceScale;
	_Velocity.k[_CrushData.m_iComp2] = (_Vertex.k[2] - _CrushData.m_HitPos.k[2]) * _CrushData.m_ForceScale;
}


void GlassVelocityZeroCB(CVec3Dfp32& _Velocity, const CVec3Dfp32& _Vertex, const CBSP4Glass_CrushData& _CrushData)
{
	_Velocity = CVec3Dfp32(0,0,0);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CBSP4Glass_ModelShard
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::AllocMeshData(const uint32 _nShards, const uint32 _nV)
{
	m_lV.SetLen(_nV);
	m_lTV.SetLen(_nV);
	m_lVelocity.SetLen(_nShards);
	m_lSrcPos.SetLen(_nShards);

	// Clear...
	FillChar(m_lVelocity.GetBasePtr(), sizeof(CVec4Dfp32) * m_lVelocity.Len(), 0x0);
}


void CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::UpdateVelocities(const int32 _iHitIndex, CBSP4Glass_Grid& _Grid, const int32 _nRefresh, const CVec3Dfp32& _Force)
{
	TAP_RCD<CVec4Dfp32> lVelocity = m_lVelocity;

	const int32 nX = _Grid.GetQuadsX();
	const int32 nY = _Grid.GetQuadsY();
	const int32 nXY = nX + nY;
	const int32 iHitY = _iHitIndex / nX;
	const int32 iHitX = _iHitIndex - (iHitY * nX);
	const fp32 Rcp = 1.0f / (fp32)nXY;

	uint32 iVel = 0;
	uint32 iRand = 0;

	int32 RangeX = MaxMT((nX >> 1), 1);
	int32 RangeY = MaxMT((nY >> 1), 1);
	fp32 RangeXRcp = 1.0f / fp32(RangeX);
	fp32 RangeYRcp = 1.0f / fp32(RangeY);

	if(_Grid.IsFullBreak())
	{
		for(int32 iY = 0; iY < nY; iY++)
		{
			const int32 iYQuadDist = Abs(iHitY - iY);
			const fp32 ForceY = (fp32(RangeY - MinMT(iYQuadDist, RangeY)) * RangeYRcp);// * 0.5f;
			for(int32 iX = 0; iX < nX; iX++)
			{
				CVec3Dfp32& Vel3D0 = *(CVec3Dfp32*)&lVelocity[iVel];
				fp32& Vel0 = lVelocity[iVel++].k[3];

				CVec3Dfp32& Vel3D1 = *(CVec3Dfp32*)&lVelocity[iVel];
				fp32& Vel1 = lVelocity[iVel++].k[3];

				Vel0 = (ForceY + ((fp32(RangeX - MinMT(Abs(iHitX - iX), RangeX)) * RangeXRcp))) * 3.0f;// * 0.5f));
				Vel1 = Vel0 * (MFloat_GetRand(iRand++) + 0.5f);
				Vel0 = Vel0 * (MFloat_GetRand(iRand++) + 0.5f);

				const int32 QuadDistance = Abs((Abs(iHitX - iX) + iYQuadDist) - nXY);
				const fp32 ForceXY = Vel0 * 18.0f;

				//const CVec3Dfp32 ForceScale = _Force * (Rcp * (QuadDistance+1)) * 12.0f;
				//pVelocity[iVel++] = ForceScale * (MFloat_GetRand(iRand++)+0.5f);// * (QuadDistance+1));
				//pVelocity[iVel++] = ForceScale * (MFloat_GetRand(iRand++)+0.5f);// * (QuadDistance+1));

				Vel3D0 = _Force * Vel0;
				Vel3D1 = _Force * Vel1;
			}
		}
	}
	else if(_Grid.IsTimeBreak())
	{
		_Grid.SetNewVelocity(lVelocity, _iHitIndex, _Force);
		_Grid.SetNewRefreshVelocity(lVelocity, _Grid.GetNumRefresh() - _nRefresh, _Force);
	}
}


void CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::UpdateMesh(int32 _iSurface, CGlassAttrib& _Attrib, CBSP4Glass_Grid& _Grid, CBSP4Glass_TangentSetup& _Tang, CBSP4Glass_MappingSetup& _Mapping, const CMat4Dfp32& _WMat)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::UpdateMesh, XR_BSP4GLASS);

	// If we are breaking whole glass, just build shard pieces
//	if(_Attrib.Attrib_FullBreak())
	if(!m_lSrcPos.GetBasePtr())
	{
		TThinArray<CVec2Dfp32> lTVTemp;
		//CMat4Dfp32 WMatInv;
		//_Grid.GetInverseOrthogonalWMat(WMatInv);

		//_WMat.InverseOrthogonal(WMatInv);

		CVec3Dfp32 PlaneN = _Grid.GetPlaneN();
		CVec3Dfp32 NegPlaneN = -PlaneN;

		const uint32 nQuadsX = _Grid.GetQuadsX();
		const uint32 nQuadsY = _Grid.GetQuadsY();
		//const CVec3Dfp32& WPlaneN = _Grid.GetWPlaneN();
		//const CVec3Dfp32 VThickness = WPlaneN * _Attrib.m_Thickness;
		const CVec3Dfp32 VThickness = PlaneN * _Attrib.m_Thickness;
		//const CVec3Dfp32* pGridPoints = _Grid.GetGridPoints();
		TAP_RCD<const CVec3Dfp32> lGridPoints = _Grid.GetGridPoints();

		const uint32 nShards = (nQuadsX * nQuadsY) << 1;
		const uint32 nPointsX = nQuadsX + 1;
		const uint32 nPointsY = nQuadsY + 1;
		
		const bool bUseThickness = _Attrib.Attrib_UseThickness();
		const bool bIgnoreEdges = _Attrib.Attrib_IgnoreEdge();

		// Allocate needed data to do a full break
		const uint32 nV = nShards * 6; //(!bUseThickness) ? (nShards * 6) : (nShards * 6);
		
		AllocMeshData(nShards, nV);

		lTVTemp.SetLen(nPointsX << 1);
		CVec2Dfp32* pTVTemp = lTVTemp.GetBasePtr();
		CVec3Dfp32* pV = m_lV.GetBasePtr();
		CVec2Dfp32* pTV = m_lTV.GetBasePtr();
		CVec4Dfp32* pVel = m_lVelocity.GetBasePtr();
		
		// Texture coordinate creation data
		const CVec3Dfp32& UVec = _Mapping.GetUVec();
		const CVec3Dfp32& VVec = _Mapping.GetVVec();
		const fp32 UVecLenSqrInv = _Mapping.GetUVecLenSqrInv();
		const fp32 VVecLenSqrInv = _Mapping.GetVVecLenSqrInv();
		const fp32 UOffset = _Mapping.GetUOffset();
		const fp32 VOffset = _Mapping.GetVOffset();
		const fp32 TxtWidthInv = _Mapping.GetTxtWidthInv();
		const fp32 TxtHeightInv = _Mapping.GetTxtHeightInv();

		// Tangent data
		const CVec3Dfp32& BackSideTangU = _Tang.GetBackSideTangU();
		const CVec3Dfp32& BackSideTangV = _Tang.GetBackSideTangV();
		const CVec3Dfp32& FrontSideTangU = _Tang.GetFrontSideTangU();
		const CVec3Dfp32& FrontSideTangV = _Tang.GetFrontSideTangV();

		_Grid.SetFrontTang(FrontSideTangU, FrontSideTangV);
		_Grid.SetBackTang(BackSideTangU, BackSideTangV);

		uint32 iGridQuad = 0;
		uint32 iVFront = 0;
		uint32 iVBack = nV >> 1;
		uint32 iTVertex = 0;
		uint32 iTVTempStart = nPointsX;

		// Generate first row of texture vertices
		for(iTVertex = 0; iTVertex < nPointsX; iTVertex++)
		{
			const CVec3Dfp32& V = lGridPoints[iTVertex];// * WMatInv;
			const fp32 UProj = ((V * UVec) * UVecLenSqrInv + UOffset);
			const fp32 VProj = ((V * VVec) * VVecLenSqrInv + VOffset);
			pTVTemp[iTVertex].k[0] = UProj * TxtWidthInv;
			pTVTemp[iTVertex].k[1] = VProj * TxtHeightInv;
			_Mapping.ImproveProj(UProj, VProj);
		}

		// Setup constant data
		MemSetD(m_lC, 0xffffffff, GLASS_NUM_COLORS);
		for(uint32 i = 0; i < GLASS_NUM_COMP_SINGLE; i++)
		{
			m_lTangU[i] = FrontSideTangU;
			m_lTangV[i] = FrontSideTangV;
			//m_lN[i] = WPlaneN;
			m_lN[i] = PlaneN;
		}

		for(uint32 i = GLASS_NUM_COMP_SINGLE; i < GLASS_NUM_COMP_DUAL; i++)
		{
			m_lTangU[i] = BackSideTangU;
			m_lTangV[i] = BackSideTangV;
			//m_lN[i] = -WPlaneN;
			m_lN[i] = NegPlaneN;
		}

		// This will have to do for now
		if(!_Attrib.Attrib_UseThickness())
		{
			CVec3Dfp32* pSrcPos = m_lSrcPos.GetBasePtr();

			uint32 Rand = _Grid.GetSeed();
			uint32 iMP = 0;
			for(uint32 y = 0; y < nQuadsY; y++)
			{
				uint32 iTVTemp = iTVTempStart;
				iTVTempStart = -(int32)iTVTempStart + nPointsX;

				// Calculate first texture coordinate for new row
				{
					const CVec3Dfp32& Vy = lGridPoints[iTVertex++];// * WMatInv;
					const fp32 UProj = ((Vy * UVec) * UVecLenSqrInv + UOffset);
					const fp32 VProj = ((Vy * VVec) * VVecLenSqrInv + VOffset);
					pTVTemp[iTVTemp].k[0] = UProj * TxtWidthInv;
					pTVTemp[iTVTemp].k[1] = VProj * TxtHeightInv;
					_Mapping.ImproveProj(UProj, VProj);
				}
				
				iTVTemp++;

				for(uint32 x = 0; x < nQuadsX; x++)
				{
                    // Setup texture coordinate to complete grid
					{
						const CVec3Dfp32& Vx = lGridPoints[iTVertex++];// * WMatInv;
						const fp32 UProj = ((Vx * UVec) * UVecLenSqrInv + UOffset);
						const fp32 VProj = ((Vx * VVec) * VVecLenSqrInv + VOffset);
						pTVTemp[iTVTemp].k[0] = UProj * TxtWidthInv;
						pTVTemp[iTVTemp].k[1] = VProj * TxtHeightInv;
						_Mapping.ImproveProj(UProj, VProj);
					}

					iTVTemp++;

					const CVec3Dfp32& GridPoint0 = lGridPoints[iGridQuad];
					const CVec3Dfp32& GridPoint1 = lGridPoints[iGridQuad+nPointsX];
					const CVec3Dfp32& GridPoint2 = lGridPoints[iGridQuad+nPointsX+1];
					const CVec3Dfp32& GridPoint3 = lGridPoints[iGridQuad+1];
					const CVec2Dfp32& TVCoord0 = pTVTemp[x+iTVTempStart];
					const CVec2Dfp32& TVCoord1 = pTVTemp[(x+nPointsX)-iTVTempStart];
					const CVec2Dfp32& TVCoord2 = pTVTemp[(x+nPointsX+1)-iTVTempStart];
					const CVec2Dfp32& TVCoord3 = pTVTemp[(x+1)+iTVTempStart];

					const CVec3Dfp32 RandC0 = GetRandomPointHalf(Rand, GridPoint0, GridPoint1, GridPoint2);
					const CVec3Dfp32 RandC1 = GetRandomPointHalf(Rand, GridPoint0, GridPoint2, GridPoint3);

					//pVel[i] = CVec3Dfp32(MFloat_GetRand(iMP*4), MFloat_GetRand(iMP*4+1), MFloat_GetRand(iMP*4+2)) * (MFloat_GetRand(iMP*4+4) * 4.0f);
					//pVel[i] = CVec3Dfp32(MFloat_GetRand(i*4), MFloat_GetRand(i*4+1), MFloat_GetRand(i*4+2)) * (MFloat_GetRand(i*4+4) * 4.0f);

					pSrcPos[iMP++] = RandC0;
					pSrcPos[iMP++] = RandC1;
					
					// Setup render data
					{
						// 1-st Front
						pTV[iVFront] = TVCoord0; pV[iVFront++] = GridPoint0 - RandC0;// pV[iVBack++] = pV[iVFront++];
						pTV[iVFront] = TVCoord1; pV[iVFront++] = GridPoint1 - RandC0;// pV[iVBack++] = pV[iVFront++];
						pTV[iVFront] = TVCoord2; pV[iVFront++] = GridPoint2 - RandC0;// pV[iVBack++] = pV[iVFront++];
						
						// 2-nd Front
						pTV[iVFront] = TVCoord0; pV[iVFront++] = GridPoint0 - RandC1;// pV[iVBack++] = pV[iVFront++];
						pTV[iVFront] = TVCoord2; pV[iVFront++] = GridPoint2 - RandC1;// pV[iVBack++] = pV[iVFront++];
						pTV[iVFront] = TVCoord3; pV[iVFront++] = GridPoint3 - RandC1;// pV[iVBack++] = pV[iVFront++];
					}
				
					iGridQuad++;
				}
				iGridQuad++;
			}
		}
		else
		{
			// Thickness!!
			// (This is not yet fully implemented.. Too heavy anyway)
		}

		// Run extra mapping pass :/
		const CVec2Dfp32 TMid = _Mapping.GetMid();
		if(TMid.k[0] != 0 || TMid.k[1] != 0)
		{
			for(int i = 0; i < nV; i++)
				pTV[i] -= TMid;
		}

		// Set spawn tick and surface
		//m_SpawnTick = -1;
		m_iSurface = _iSurface;
		m_nShards = nShards;
		
		// Free grid points, no longer needed
		//_Grid.CleanGrid(true);
	}
//	else
//	{
		// Bits and pieces
//	}
}


void CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::RefreshMesh(const int32 _GameTick, CBSP4Glass_Grid& _Grid, const CMat4Dfp32& _WMat)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::RefreshMesh, XR_BSP4GLASS);

	if(_Grid.IsValid())
		_Grid.Refresh(m_lVelocity.GetBasePtr(), _GameTick, m_lSrcPos.GetBasePtr(), _WMat);
}


void CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::RenderMask(CBSP4Glass_RenderParam* _pRenderParam, CXW_SurfaceKeyFrame* _pSurfKey, const fp32& _Priority, CXW_Surface* _pSurfaceCrush)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::RenderMask, XR_BSP4GLASS);

	CXR_VBManager* pVBM = _pRenderParam->m_pVBM;
	
	CRC_Attributes* pA = _pRenderParam->GetMaskAttr();
	CMat4Dfp32* pVBM2V = _pRenderParam->GetVBMat();
	if(!pVBM2V || !pA)
		return;

	uint32 Color = 0xffffffff;
	uint32 DiffuseTxtID = 0;
	const int nLayers = _pSurfKey->m_lTextures.Len();
	const CXW_SurfaceLayer* pLayers = _pSurfKey->m_lTextures.GetBasePtr();

	for(int i = 0; i < nLayers; i++)
	{
		if(pLayers->m_Type)
		{
			const uint32 iTxt = pLayers->m_Type - 1;
			if(iTxt <= XW_LAYERTYPE_ANISOTROPICDIR)
			{
				if (iTxt == XR_SHADERMAP_DIFFUSE)
				{
					//	const uint8 A = TruncToInt(Clamp01(pLayers->m_HDRColor.k[3])*255.0f);
					//	Color = CPixel32(0,0,0,A);
					vec128 col = M_VLd_V4f16_f32(&pLayers->m_HDRColor);
					vec128 scale = M_VConst(0,0,0,255.0f);
					M_VSt_V4f32_Pixel32(M_VMul(col, scale), &Color);

					DiffuseTxtID = pLayers->m_TextureID;
					break;
				}
			}
		}

		pLayers++;
	}

	// Setup attributes
	pA->SetDefault();
	pA->Attrib_RasterMode(CRC_RASTERMODE_ONE_INVALPHA);
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pA->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);
	pA->Attrib_TextureID(0, DiffuseTxtID);

	CXR_VertexBuffer* pVBList = _pRenderParam->GetVBList();
	const uint32 nChains = _pRenderParam->GetChainLen();
	for(uint32 i = 0; i < nChains; i++)
	{
		CBSP4Glass_Chain* pGlassChain = _pRenderParam->GetChain(i);

		CXR_VertexBuffer& VB = pVBList[i];

		VB.m_pAttrib = pA;
		VB.m_pVBChain = pGlassChain->GetChain();
		VB.m_Flags |= CXR_VBFLAGS_VBCHAIN;
		VB.Matrix_Set(pVBM2V);
		VB.m_Priority = _Priority;
//		VB.m_VpuShadowInfo.m_TaskId = 0xffffffff;
		VB.Geometry_Color(Color);

		// Set matrix palette
		VB.m_pMatrixPaletteArgs = pGlassChain->GetMatrixPalette();

		pVBM->AddVB(&VB);
	}

	// Extra blending surface
	CXR_Engine* pEngine = _pRenderParam->m_pEngine;
	CXR_AnimState* pAnimState = _pRenderParam->m_pCurAnimState;
	if(_pSurfaceCrush && nChains > 0 && pEngine && pVBM && pAnimState)
	{
		GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::Render::RenderMask::RenderSurfaceOverlay, XR_BSP4GLASS);

		CXW_SurfaceKeyFrame* pSurfaceKey = _pSurfaceCrush->GetFrame(0, pAnimState->m_AnimTime0);

		int Flags = RENDERSURFACE_MATRIXSTATIC_M2V;
		fp32 Priority = _pSurfaceCrush->m_PriorityOffset;
		fp32 Offset = CXR_VBPRIORITY_MODEL_LAYEROFFSET;

		if (_pSurfaceCrush->m_Flags & XW_SURFFLAGS_TRANSPARENT)
			Priority += _pRenderParam->m_pRenderInfo->m_BasePriority_Transparent;
		else
			Priority += _pRenderParam->m_pRenderInfo->m_BasePriority_Opaque;

		CMTime SurfTime = _pRenderParam->GetSurfaceTime(_pSurfaceCrush);

		// Render on glass shards.
		for(uint32 i = 0; i < nChains; i++)
			CXR_Util::Render_Surface(Flags, SurfTime, _pSurfaceCrush, pSurfaceKey, pEngine, pVBM,(CMat4Dfp32*) NULL,(CMat4Dfp32*) NULL, pVBList[i].m_pTransform, &pVBList[i], Priority, Offset, NULL);
	}
}


void CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::RenderShading(CBSP4Glass_RenderParam* _pRenderParam, const CXR_Light& _Light, const CXR_ShaderParams& _ShaderParams, const CXR_SurfaceShaderParams* _pSurfParams)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::RenderShading, XR_BSP4GLASS);

	CXR_VBManager* pVBM = _pRenderParam->m_pVBM;

	CMat4Dfp32* pVBM2V = _pRenderParam->GetVBMat();
	if(!pVBM2V)
		return;

	// Get vb list
	uint nChain = _pRenderParam->GetChainLen();
	CXR_VertexBuffer* pVBList = _pRenderParam->GetVBList();

	const uint MaxChain = 256;
	if (nChain > MaxChain)
		nChain = MaxChain;

	CXR_VertexBufferGeometry lVBGeom[MaxChain];	// 20 * 256 = 5k
	const CXR_SurfaceShaderParams* lpSSP[MaxChain];

	// Render vb list
	for(uint32 i = 0; i < nChain; i++)
	{
		CBSP4Glass_Chain* pGlassChain = _pRenderParam->GetChain(i);
//		CXR_VertexBuffer& VB = pVBList[i];

		CXR_VertexBufferGeometry& VB = lVBGeom[i];

		// Set vb list
		VB.Create(pGlassChain->GetChain(), pVBM2V);
		// Set matrix palette
		VB.m_pMatrixPaletteArgs = pGlassChain->GetMatrixPalette();

		lpSSP[i] = _pSurfParams;
	}

	CXR_ShaderParams Params;
	Params = _ShaderParams;
	Params.m_nVB = nChain;

	{
		GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::RenderShading_EngineRenderShading, XR_BSP4GLASS);
		_pRenderParam->GetShader()->RenderShading(_Light, lVBGeom, &Params, lpSSP);
	}
}


void CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::VB_CreateChain(CXR_RenderInfo& _RenderInfo, CBSP4Glass_Model* _pModel, CXR_VBManager* _pVBM, CBSP4Glass_RenderParam* _pRenderParam)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::VB_CreateChain, XR_BSP4GLASS);
	//return VB_GenerateChain(_Attrib, _Grid, _pVBM);

	CBSP4Glass_Grid& Grid = _pModel->m_Grid;
	CGlassAttrib& Attrib = _pModel->m_Attrib;

	//const CVec3Dfp32 PlaneN = Grid.GetPlaneN();
	//const CVec3Dfp32& WPlaneN = Grid.GetWPlaneN();
	//const CVec3Dfp32& FrontTangU = Grid.GetFrontTangU();
	//const CVec3Dfp32& FrontTangV = Grid.GetFrontTangV();

	const uint32 nX = Grid.GetQuadsX();
	const uint32 nY = Grid.GetQuadsY();
	const uint32 nXY = nX * nY;

	uint32 nXYTotalGen = nXY << 1;
	const uint32 nTotalV = (uint32)m_lV.Len();
	const uint32 nChains = (nXYTotalGen+(GLASS_BATCH_MAX-1)) / GLASS_BATCH_MAX;
	const uint32 nVB = (_RenderInfo.m_nLights + 1) * nChains;
		
	//CMat4Dfp32* pMPMats = m_lMP.GetBasePtr();
	CVec2Dfp32* pTV = m_lTV.GetBasePtr();
	CVec3Dfp32* pSrcV = m_lV.GetBasePtr();

	
	// Number of matrices allowed
	//const uint32 nMatAllowed = GLASS_NUM_PALETTEMATRICES;
	const uint32 TotSizeMaskAttr = TUtilAlign(sizeof(CRC_Attributes));
	const uint32 TotSizeVBMat = TUtilAlign(sizeof(CMat4Dfp32));
	const uint32 TotSizeVBChain = TUtilAlignElem(sizeof(CXR_VBChain), (nChains << 1));
	const uint32 TotSizeMatPalette = TUtilAlignElem(sizeof(CRC_MatrixPalette), nChains);
	const uint32 TotSizeVBList = TUtilAlignElem(sizeof(CXR_VertexBuffer), nVB);
	const uint32 TotSizeMPMats = TUtilAlignElem(sizeof(CMat4Dfp32), nXYTotalGen);

	// Allocate all data needed for rendering
	const int VBRAM = TotSizeMaskAttr +
					  TotSizeVBMat +
					  TotSizeVBChain +
					  TotSizeMatPalette +
					  TotSizeVBList +
					  TotSizeMPMats;
	
	uint8* pVBData = (uint8*)_pVBM->Alloc(VBRAM);
	if(!pVBData)
		return;

	// Clear
	MemSetD(pVBData, 0x0, VBRAM >> 2);
	#ifndef M_RTM
		uint8* pVBStart = pVBData;
	#endif
	
	CMat4Dfp32* pVBMat = TUtilGetAlign<CMat4Dfp32>(pVBData);
	CMat4Dfp32* pMPMats = TUtilGetAlignElem<CMat4Dfp32>(pVBData, nXYTotalGen);
	CRC_Attributes* pA = TUtilGetAlign<CRC_Attributes>(pVBData);
	
	//const CVec3Dfp32* pSrcPos = m_lSrcPos.GetBasePtr();
	//const CVec4Dfp32* pVelocity = m_lVelocity.GetBasePtr();
	const TAP_RCD<CVec3Dfp32> lSrcPos = m_lSrcPos;
	const TAP_RCD<CVec4Dfp32> lVelocity = m_lVelocity;
	
	CXR_AnimState* pAnimState = _pRenderParam->m_pCurAnimState;

	if(pAnimState)
	{
		CMat4Dfp32 UnitMat;
		CQuatfp32 qUnitMat;//, qRandRotMat, qResult;

		UnitMat.Unit();
		qUnitMat.Create(UnitMat);

		// Create rotation matrix
		CQuatfp32 lqRandRotMat[GLASS_ROT_MAT_MAX];
		for(uint32 i = 0; i < GLASS_ROT_MAT_MAX; i++)
			lqRandRotMat[i].Create(m_lRandRotMat[i]);

		// Initialize seeding values
		uint32 RandSeed = (m_SpawnTick + 0xfab5);
		const uint32 DynSeed = (RandSeed * nXYTotalGen * 4);

		const int32 AnimTick = pAnimState->m_Data[GLASS_DATA_GAMETICK];
		const fp32 GameTickTime = pAnimState->m_AnimAttr0;
		const fp32 RenderTickFrac = pAnimState->m_AnimAttr1;

		const fp32 AnimStateTime = CMTime::CreateFromTicks(AnimTick - m_SpawnTick, GameTickTime, RenderTickFrac).GetTime();
		const fp32 AnimTime = AnimStateTime + 0.001f;

		CQuatfp32 qFinalResult;
		CMat43fp32 FinalMat;
		CMat43fp32 DynRandMat;
		DynRandMat.Unit();
		DynRandMat.M_x_RotZ(MFloat_GetRand(DynSeed + 249) * (AnimTime*0.5));
		DynRandMat.M_x_RotY(MFloat_GetRand(DynSeed + 359) * (AnimTime*0.5));
		DynRandMat.M_x_RotX(MFloat_GetRand(DynSeed + 508) * (AnimTime*0.5));

		CQuatfp32 qDynRandMat;
		qDynRandMat.Create(DynRandMat);

		CVec3Dfp32 M2WPos = _pRenderParam->m_M2W.GetRow(3);
		if(Grid.IsFullBreak())
		{
			GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::VB_CreateChain::AdminMatrixPalette, XR_BSP4GLASS);
			const fp32 AnimTimeHalfPi = Clamp(AnimTime, 0.0f, _PIHALF);
			const fp32 AnimTimeSin = M_Sin(AnimTimeHalfPi);
			const CVec3Dfp32 Gravity(0, 0, 64.0f * (AnimTimeSin * AnimTime));
		
			const fp32 VelTime = AnimTimeSin * 2.0f;

			const uint32 iEndX = nX - 1;
			const uint32 iEndY = nY - 1;
			{
				// Interpolate into rotation matrix so glass doesn't get a rotation from start of shatter
				fp32 AnimTimeClamp = Clamp01(AnimTime);
				for(uint32 i = 0; i < nXYTotalGen; i++)
				{
					CMat4Dfp32& MPMat = pMPMats[i];

					// Create matrix
					uint iqMat = (RandSeed++ & GLASS_ROT_MAT_AND);
					qUnitMat.Lerp(lqRandRotMat[iqMat] * qDynRandMat, AnimTimeClamp, qFinalResult);
					qFinalResult.CreateMatrix(MPMat);

					MPMat.GetRow(3) = (lSrcPos[i]-M2WPos) + ((*(CVec3Dfp32*)&lVelocity[i]) * VelTime) - Gravity;
				}
			}
		}
		else if(Grid.IsTimeBreak())
		{
			const CVec3Dfp32 Gravity(0, 0, 64.0f);

			TAP_RCD<int32> lActiveTime = Grid.GetActiveTime();
			for(uint32 i = 0, j = 0, k = 1; i < nXY; i++, j += 2, k += 2)
			{
				int32 ActiveTime = lActiveTime[i];

				CMat4Dfp32& MPMat0 = pMPMats[j];
				CMat4Dfp32& MPMat1 = pMPMats[k];

				MPMat0.Unit();
				MPMat1.Unit();
				if(ActiveTime != 0)
				{
					const fp32 AnimTimeActive = CMTime::CreateFromTicks(AnimTick - ActiveTime, GameTickTime, RenderTickFrac).GetTime();
					
					// Create matrix
					const uint iqMat0 = (RandSeed++ & GLASS_ROT_MAT_AND);
					const uint iqMat1 = (RandSeed++ & GLASS_ROT_MAT_AND);
					const fp32 AnimVelocity = AnimTimeActive * 4.0f;
					const CVec3Dfp32 AnimGravity = Gravity * (AnimTimeActive * AnimTimeActive);
					
					// First matrix palette matrix
					qUnitMat.Lerp(lqRandRotMat[iqMat0] * qDynRandMat, Clamp01(AnimTimeActive), qFinalResult);
					qFinalResult.CreateMatrix(MPMat0);

					// Second matrix palette matrix
					qUnitMat.Lerp(lqRandRotMat[iqMat1] * qDynRandMat, Clamp01(AnimTimeActive), qFinalResult);
					qFinalResult.CreateMatrix(MPMat1);

					MPMat0.GetRow(3) = ((lSrcPos[j]-M2WPos) + (*(CVec3Dfp32*)&lVelocity[j] * AnimVelocity)) - AnimGravity;
					MPMat1.GetRow(3) = ((lSrcPos[k]-M2WPos) + (*(CVec3Dfp32*)&lVelocity[k] * AnimVelocity)) - AnimGravity;
				}
				else
				{
					RandSeed += 2;
					MPMat0.GetRow(3) = lSrcPos[j];
					MPMat1.GetRow(3) = lSrcPos[k];
				}
			}
		}
	}

	// Set some attriubtes
	_pRenderParam->SetVBMat_M4D(pVBMat, _pRenderParam->m_M2V);
	_pRenderParam->SetMaskAttr(pA);
	_pRenderParam->SetMPMatrices(pMPMats);
	_pRenderParam->SetChainLen(nChains);
	uint32 iChain = 0;
	uint8* pVBDataPos = pVBData;
	while(nXYTotalGen > 0)
	{
		// Fetch data
		CXR_VBChain* pChain = TUtilGetAlign<CXR_VBChain>(pVBData);
		CXR_VBChain* pChain2 = TUtilGetAlign<CXR_VBChain>(pVBData);;
		CRC_MatrixPalette* pMP = TUtilGetAlign<CRC_MatrixPalette>(pVBData);
		new(pMP) CRC_MatrixPalette;

		const uint32 nXYGen = MinMT(nXYTotalGen, (uint32)GLASS_NUM_PALETTEMATRICES);
		const uint32 nChainV = nXYGen * 3;
		const uint32 nPrim = nXYGen;

		// Setup VBChain Front/Back
		{
			pChain->m_pN			= m_lN;
			pChain->m_pCol			= m_lC;
			pChain->m_pTV[0]		= (fp32*)pTV;
			pChain->m_pTV[1]		= (fp32*)m_lTangU;
			pChain->m_pTV[2]		= (fp32*)m_lTangV;
			pChain->m_nTVComp[0]	= 2;
			pChain->m_nTVComp[1]	= 3;
			pChain->m_nTVComp[2]	= 3;
			pChain->m_nMWComp		= 4;
			pChain->m_pMI			= m_lMI;
			pChain->m_pMW			= m_lMW;
			pChain->m_nPrim			= nPrim;
			pChain->m_PrimType		= CRC_RIP_TRIANGLES;
			pChain->m_piPrim		= m_lP;
			pChain->m_pV			= pSrcV;
			pChain->m_nV			= nChainV;
			pChain->m_pNextVB		= pChain2;
			pChain->m_TaskId		= 0xffff;
						
			pChain2->m_pN			= m_lN + GLASS_NUM_COMP_SINGLE;
			pChain2->m_pCol			= m_lC + GLASS_NUM_COMP_SINGLE;
			pChain2->m_pTV[0]		= (fp32*)pTV;
			pChain2->m_pTV[1]		= (fp32*)(m_lTangU + GLASS_NUM_COMP_SINGLE);
			pChain2->m_pTV[2]		= (fp32*)(m_lTangV + GLASS_NUM_COMP_SINGLE);
			pChain2->m_nTVComp[0]	= 2;
			pChain2->m_nTVComp[1]	= 3;
			pChain2->m_nTVComp[2]	= 3;
			pChain2->m_nMWComp		= 4;
			pChain2->m_pMI			= m_lMI;
			pChain2->m_pMW			= m_lMW;
			pChain2->m_nPrim		= nPrim;
			pChain2->m_PrimType		= CRC_RIP_TRIANGLES;
			pChain2->m_piPrim		= m_lP + GLASS_NUM_COMP_SINGLE;
			pChain2->m_pV			= pSrcV;
			pChain2->m_nV			= nChainV;
			pChain2->m_TaskId		= 0xffff;
		}

		// Setup Matrix palette
		pMP->m_nMatrices		= nXYGen;
		pMP->m_pMatrices		= (void*)pMPMats;

		// Set data in glass chain structure
		iChain = _pRenderParam->SetChain(iChain, pChain, pMP);

		// Increate pointers
		pSrcV += nChainV;
		pTV += nChainV;
		pMPMats += nXYGen;
		nXYTotalGen -= nXYGen;
	}
	
	// Increate ptr to find position of vb list
	pVBData = pVBDataPos + (TotSizeVBChain + TotSizeMatPalette);
    _pRenderParam->SetVBList((CXR_VertexBuffer*)pVBData);

	// Assert check
	#ifndef M_RTM
	    pVBData += TotSizeVBList;
		M_ASSERT(pVBData == (pVBStart + VBRAM), "CXR_BSP4Glass_Instance::CGlassShardModel::VB_CreateChain: VB Memory not initialized properly!");
	#endif
}


void CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::Render(CBSP4Glass_RenderParam* _pRenderParam, CXW_Surface* _pSurface, const CMat4Dfp32& _WMat, CXW_Surface* _pSurfaceCrush)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::Render, XR_BSP4GLASS);
	
	CGlassModel& Model = _pRenderParam->GetGlassInstance()->m_lModels[m_iModel];
	if (Model.m_Attrib.Attrib_Inactive())
		return;

	CBox3Dfp32 BoundingBox;
	GetBound_Box(BoundingBox, &_WMat);
	
	CXR_RenderInfo& RenderInfo = *_pRenderParam->m_pRenderInfo;
	CXR_ViewClipInterface* pViewClip = _pRenderParam->m_pViewClip;

	if(!pViewClip->View_GetClip_Box(BoundingBox.m_Min, BoundingBox.m_Max, 0, 0, NULL, &RenderInfo))
		return;

	// Get some useful variables we are going to need
	const CMat4Dfp32& W2V = _pRenderParam->m_W2V;
	const CMat4Dfp32& M2W = _pRenderParam->m_M2W;
	const CMat4Dfp32& M2V = _pRenderParam->m_M2V;
	const CMat4Dfp32& InvM2W = _pRenderParam->m_InvM2W;
	CXR_VBManager* pVBM = _pRenderParam->m_pVBM;
	CXR_SceneGraphInstance* pSGI = _pRenderParam->GetSceneGraphInstance();

	// Render bounding for these shards
	#ifndef M_RTM
	{
		MACRO_GetRegisterObject(CWireContainer, pWC, "GAMECONTEXT.CLIENT.WIRECONTAINER");
		if(pWC)
			pWC->RenderAABB(BoundingBox, CPixel32(255,0,0), 0.0f, false);
	}
	#endif

	// Get surface properties
	CXR_SurfaceContext* pSC = _pRenderParam->m_pEngine->GetSC();
	CXW_SurfaceKeyFrame* pSurfKey = _pRenderParam->GetSurfaceKey(_pSurface, pSC->GetTempSurfaceKeyFrame());

	// Create shader parameters
	CXR_SurfaceShaderParams SSP;
	CXR_ShaderParams ShaderParams;
	SSP.Create(_pSurface, pSurfKey);
	ShaderParams.Create(&M2W, &W2V, _pRenderParam->m_pEngine->GetShader());
	
	//ShaderParams.m_Priority = RenderInfo.m_BasePriority_Transparent + (_pSurface->m_PriorityOffset * 0.001f);
	ShaderParams.m_Flags = XR_SHADERFLAGS_NOSTENCILTEST | XR_SHADERFLAGS_USEZLESS | XR_SHADERFLAGS_TRANSPARENT | XR_SHADERFLAGS_GLASS;
	//ShaderParams.m_Flags &= ~XR_SHADERFLAGS_USEZEQUAL;
		
	// Calculate scissor box for shard model
	CRect2Duint16 ShardBoundingScissor;
	CBox3Dfp32 ShardBounding = BoundingBox;
	ShardBounding.Grow(8.0f);
	CXR_Model_BSP4Glass::CalcBoxScissor(pVBM, W2V, ShardBounding, ShardBoundingScissor);
	
	// For the moment just maximize to viewport (Big no no, waving with the index finger and everything)
	const CRct& VPRect = pVBM->Viewport_Get()->GetViewRect();
	ShardBoundingScissor = CRect2Duint16(CVec2Duint16((uint16)MaxMT(0,VPRect.p0.x),(uint16)MaxMT(0,VPRect.p0.y)),
										 CVec2Duint16((uint16)VPRect.GetWidth(),(uint16)VPRect.GetHeight()));

	// Create chain only once for shards!
	// First, we render all the glass stuff, using no stencil tests, transparent and z less.
	CVec3Dfp32 WCenter = Model.m_Center * M2W;
	fp32 Dist = -_pRenderParam->m_FrontPlaneW.Distance(WCenter) * _pRenderParam->m_BackPlaneInv;
	ShaderParams.m_Priority = ClampRange(Dist + 0.25f, 1.5f) - 0.25f + CXR_VBPRIORITY_MODEL_TRANSPARENT;

	_pRenderParam->ClearChain();
	VB_CreateChain(RenderInfo, &Model, pVBM, _pRenderParam);
	ShaderParams.m_nVB = (uint16)_pRenderParam->GetChainLen();

	//_pRenderParam->SetVBMatFrom_M4D(_pRenderParam->m_W2V);
	//_pRenderParam->SetVBMatFrom_M4D(M2V);
    RenderMask(_pRenderParam, pSurfKey, ShaderParams.m_Priority - 0.0002f, _pSurfaceCrush);

    // Setup shard model for shader rendering
	int nRenderLights = RenderInfo.m_nLights;
	for(int i = 0; i < nRenderLights; i++)
	{
		// Fetch light and transform
		CXR_Light Light;
		pSGI->SceneGraph_Light_Get(RenderInfo.m_pLightInfo[i].m_pLight->m_iLight, Light);
		Light.Transform(InvM2W);

		// Expand scissor box and render shard model using fetched light
		_pRenderParam->ExpandLightOcclusion(Light.m_iLight, ShardBoundingScissor);
		RenderShading(_pRenderParam, Light, ShaderParams, &SSP);
	}
}


void CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::GetBound_Box(CBox3Dfp32& _BBox, const CMat4Dfp32* _pMat)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::GetBound_Box, XR_BSP4GLASS);

	// Set bounding box from min/max point updated from refresh
	_BBox.m_Max = m_BBox.m_Max;
	_BBox.m_Min = m_BBox.m_Min;
	_BBox.Grow(1);

	// Run through vertices and check size of bounding box
	//const int nV = m_lV.Len();
	//const CVec3Dfp32* pV = m_lV.GetBasePtr();
	//for(int i = 0; i < nV; i++)
	//	_BBox.Expand(pV[i]);

	// Do we want to transform bounding box ?
	if(_pMat)
	{
		CBox3Dfp32 BBox = _BBox;
		BBox.Transform(*_pMat, _BBox);
	}
}


#ifndef M_RTM
void CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::Debug_Render(CWireContainer* _pWC)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass_Instance::CBSP4Glass_ModelShard::Debug_Render, XR_BSP4GLASS);

	// Remove to render debug pieces
	return;

	// Debug statics
	static int Debug_DebugRender_iFirstShard = 0;
	static int Debug_DebugRender_nShardsRender = 0;

	int iFirst = 0;
	int iLast = m_nShards;

	// Only adjust if we want to, and make sure we're not trying to render more than allowed
	if(Debug_DebugRender_nShardsRender != 0)
	{
		// If shards reach zero, we don't try to fiddle with it.
		iFirst = MinMT(m_nShards - 1, Debug_DebugRender_iFirstShard);
		iLast = MinMT(m_nShards, iFirst + Debug_DebugRender_nShardsRender);
	}
	
	// Render debug lines for each shard
	const bool bUseThickness = false;
	const int nPerFaceV = (bUseThickness) ? 6 : 3;
	
	CVec3Dfp32* pV = m_lV.GetBasePtr();
	for(int i = iFirst; i < iLast; i++)
	{
		// Make triangle a little bit smaller when rendering debug lines so we don't get multiple lines on each other
		int iV = i * nPerFaceV;
		const CVec3Dfp32 V0  = pV[iV+0] + ((pV[iV+2] - pV[iV+0]) + (pV[iV+1] - pV[iV+0])).Normalize();
		const CVec3Dfp32 V1  = pV[iV+1] + ((pV[iV+2] - pV[iV+1]) + (pV[iV+0] - pV[iV+1])).Normalize();
		const CVec3Dfp32 V2  = pV[iV+2] + ((pV[iV+0] - pV[iV+2]) + (pV[iV+1] - pV[iV+2])).Normalize();
		
		// Draw debug lines (front/back)
		_pWC->RenderWire(V0, V1, GLASS_WIRE_RED, 0.0f, false);
		_pWC->RenderWire(V1, V2, GLASS_WIRE_RED, 0.0f, false);
		_pWC->RenderWire(V2, V0, GLASS_WIRE_RED, 0.0f, false);

		if(bUseThickness)
		{
			const CVec3Dfp32 V3  = pV[iV+3] + ((pV[iV+5] - pV[iV+3]) + (pV[iV+4] - pV[iV+3])).Normalize();
			const CVec3Dfp32 V4  = pV[iV+4] + ((pV[iV+5] - pV[iV+4]) + (pV[iV+3] - pV[iV+4])).Normalize();
			const CVec3Dfp32 V5  = pV[iV+5] + ((pV[iV+3] - pV[iV+5]) + (pV[iV+4] - pV[iV+5])).Normalize();
			_pWC->RenderWire(V3, V4, GLASS_WIRE_GREEN, 0.0f, false);
			_pWC->RenderWire(V4, V5, GLASS_WIRE_GREEN, 0.0f, false);
			_pWC->RenderWire(V5, V3, GLASS_WIRE_GREEN, 0.0f, false);

			// Edges
			_pWC->RenderWire(V0, V4, GLASS_WIRE_BLUE, 0.0f, false);
			_pWC->RenderWire(V1, V3, GLASS_WIRE_BLUE, 0.0f, false);
			_pWC->RenderWire(V2, V4, GLASS_WIRE_YELLOW, 0.0f, false);
			_pWC->RenderWire(V1, V5, GLASS_WIRE_YELLOW, 0.0f, false);
			_pWC->RenderWire(V2, V3, GLASS_WIRE_PURPLE, 0.0f, false);
			_pWC->RenderWire(V0, V5, GLASS_WIRE_PURPLE, 0.0f, false);
		}
	}
}
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass_Instance
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXR_Model_BSP4Glass_Instance::CXR_Model_BSP4Glass_Instance(const CGlassCreateInfo& _GlassInfo)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass_Instance::CXR_Model_BSP4Glass_Instance(CGlassCreateInfo), XR_BSP4GLASS);
	m_lModels.Clear();
	CreateVertexBufferData(_GlassInfo);
}


CXR_Model_BSP4Glass_Instance::CXR_Model_BSP4Glass_Instance()
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass_Instance::CXR_Model_BSP4Glass_Instance, XR_BSP4GLASS);
	m_lModels.Clear();
}


CXR_Model_BSP4Glass_Instance::~CXR_Model_BSP4Glass_Instance()
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass_Instance::~CXR_Model_BSP4Glass_Instance, XR_BSP4GLASS);
	m_lModels.Clear();
}


void CXR_Model_BSP4Glass_Instance::Create(CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass_Instance::Create, XR_BSP4GLASS);
	// Get creation info from model.
	if(_pModel)
	{
		CXR_Model_BSP4Glass* pGlassModel = (CXR_Model_BSP4Glass*)_pModel;
		CXR_Model_BSP2* pMaster = pGlassModel->GetMaster();
		if(!pMaster)
			return;

		CGlassCreateInfo GlassInfo;
		pGlassModel->GetGlassCreateInfo(GlassInfo);
		CreateVertexBufferData(GlassInfo);

		m_spWMC = MNew(CBSP4Glass_WallmarkContext);
		m_spWMC->Create(pGlassModel->GetMaster(), 128);

		// Setup static data
		if(!m_bStaticInit)
		{
			MemSet(m_lMW, 0x0, sizeof(fp32) * GLASS_NUM_MATRIXWEIGHTS);
			
			uint32 iP = 0;
			uint32 iP2 = GLASS_NUM_COMP_SINGLE;
			uint32 iMI = 0;
			uint32 iMW = 0;
			uint32 iV = 0;
			for(uint32 i = 0; i < GLASS_BATCH_MAX; i++)
			{
				m_lMI[iMI++] = ((i << 24) | (i << 16) | (i << 8) | i);
				m_lMI[iMI++] = ((i << 24) | (i << 16) | (i << 8) | i);
				m_lMI[iMI++] = ((i << 24) | (i << 16) | (i << 8) | i);

				m_lMW[iMW++ << 2] = 1.0f;
				m_lMW[iMW++ << 2] = 1.0f;
				m_lMW[iMW++ << 2] = 1.0f;

				m_lP[iP++] = iV + 0;
				m_lP[iP++] = iV + 1;
				m_lP[iP++] = iV + 2;
				m_lP[iP2++] = iV + 0;
				m_lP[iP2++] = iV + 2;
				m_lP[iP2++] = iV + 1;
				iV += 3;
			}

			for(uint32 i = 0, iR = 0; i < GLASS_ROT_MAT_MAX; i++)
			{
				// Setup random rotation matrices to utilize
				m_lRandRotMat[i].Unit();
				m_lRandRotMat[i].M_x_RotZ(MFloat_GetRand(iR++) * (fp32)((i*4) + 1));
				m_lRandRotMat[i].M_x_RotY(MFloat_GetRand(iR++) * (fp32)((i*4) + 2));
				m_lRandRotMat[i].M_x_RotX(MFloat_GetRand(iR++) * (fp32)((i*4) + 3));
			}

			m_bStaticInit = true;
		}
	}
}


void CXR_Model_BSP4Glass_Instance::OnDeltaLoad(CCFile* _pFile)
{
	// Load instance data
	uint8 State = 0;
	TAP_RCD<CBSP4Glass_Model> lModels = m_lModels;
	for (uint i = 0; i < lModels.Len(); i++)
	{
		if (!lModels[i].m_Attrib.Attrib_NoSave())
		{
			// We touched this window in some kind of way, load it
			_pFile->ReadLE(State);
			if (State > 0)
			{
				lModels[i].m_Attrib.OnDeltaLoad(_pFile);
				lModels[i].m_Grid.OnDeltaLoad(_pFile);
			}
		}
	}
}


void CXR_Model_BSP4Glass_Instance::OnDeltaSave(CCFile* _pFile)
{
	// Save instance data
	uint8 State = 0;
	TAP_RCD<CBSP4Glass_Model> lModels = m_lModels;
	uint32 TotalSize = 0;
	for (uint i = 0; i < lModels.Len(); i++)
	{
		if (!lModels[i].m_Attrib.Attrib_NoSave())
		{
			// Check state and save window if it has changed
			if (lModels[i].m_Grid.IsValid())
				State = 1;

			// Save this window since we have done something to it
			_pFile->WriteLE(State);
			if (State > 0)
			{
				TotalSize += lModels[i].m_Attrib.OnDeltaSave(_pFile);
				TotalSize += lModels[i].m_Grid.OnDeltaSave(_pFile);
				State = 0;
			}

			TotalSize += sizeof(uint8);
			M_TRACEALWAYS(CStrF("GlassModel(%d): %d bytes.\n", i, TotalSize));
		}
	}

	M_TRACEALWAYS(CStrF("GlassInstance TotalSize (%d) bytes - (%.2f) kB.\n", TotalSize, fp32(TotalSize) / 1024.0f));
}


#ifndef M_RTM
void CXR_Model_BSP4Glass_Instance::Debug_RenderWire(CWireContainer* _pWC, CPixel32 _Color, const CMat4Dfp32& _WMat)
{
	for (uint i = 0; i < m_lModels.Len(); i++)
	{
		if (m_lModels[i].m_Grid.IsValid())
			m_lModels[i].m_Grid.Debug_RenderGrid(_pWC, _Color, _WMat);
	}
}
#endif


void CXR_Model_BSP4Glass_Instance::OnRefresh(const CXR_ModelInstanceContext& _Context, const CMat4Dfp32* _pMat, int _nMat, int _Flags)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::OnRefresh, XR_BSP4GLASS);

	// Update all shard models
	int nShardModels = m_lpShardModels.Len();
	
	// Server refresh
	if (!_Context.m_pClient)
	{
		TAP_RCD<CBSP4Glass_Model> lModels = m_lModels;
		int nModels = lModels.Len();
		for (int i = 0; i < nModels; i++)
		{
			CBSP4Glass_Model& Model = lModels[i];
			if (Model.m_Attrib.Attrib_TimeBreaker() && Model.m_Grid.IsValid() && _nMat > 0)
				Model.m_Grid.Refresh(NULL, _Context.m_GameTick, NULL, _pMat[0]);
		}
	}

	// Refresh client shard models
	else if (nShardModels > 0)
	{
		const fp32 AnimTime = (_Context.m_pAnimState) ? _Context.m_pAnimState->m_AnimTime0.GetTime() : _Context.m_GameTime.GetTime();
		TAP_RCD<CGlassModelShard*> lpShardModels = m_lpShardModels;

		TArray<uint> lRemove;
		lRemove.SetGrow(32);

		// Run through all shard models
		bool bClean = true;
		for(int i = 0; i < lpShardModels.Len(); i++)
		{
			CGlassModelShard* pShardModel = lpShardModels[i];
			if(pShardModel->m_nShards > 0)
			{
                CGlassModel& Model = m_lModels[pShardModel->m_iModel];
				if(Model.m_Attrib.Attrib_TimeBreaker() && _pMat && _nMat > 0)
					pShardModel->RefreshMesh(_Context.m_GameTick, Model.m_Grid, _pMat[0]);

				if(((_Context.m_GameTick - pShardModel->m_SpawnTick) * _Context.m_TimePerTick > 8.0f) && Model.m_Attrib.Attrib_FullBreak())
				{
					// Set zero shards and continue, this will prevent further update on object
					pShardModel->m_nShards = 0;
					pShardModel->ClearMemory();
					lRemove.Add(i);
					continue;
				}

				// TODO: Increase bounding box,,,
				bClean = false;
			}
		}

		// Remove entries
		int Len = lRemove.Len();
		for(int i = Len - 1; i >= 0; i--)
			m_lpShardModels.Del(lRemove[i]);

		// Clean up
		//if(bClean)
		//	m_lpShardModels.Clear();
	}
}


bool CXR_Model_BSP4Glass_Instance::NeedRefresh(CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass_Instance::NeedRefresh, XR_BSP4GLASS);
	return false;
}


TPtr<CXR_ModelInstance> CXR_Model_BSP4Glass_Instance::Duplicate() const
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass_Instance::Duplicate, XR_BSP4GLASS);
	CXR_Model_BSP4Glass_Instance* pGlassInstance = MNew(CXR_Model_BSP4Glass_Instance);
	pGlassInstance->m_lModels.Clear();
	pGlassInstance->m_lModels.Add(m_lModels);
	return TPtr<CXR_ModelInstance>(pGlassInstance);
}


void CXR_Model_BSP4Glass_Instance::operator = (const CXR_ModelInstance& _Instance)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass_Instance::operator = (CXR_ModelInstance), XR_BSP4GLASS);
	const CXR_Model_BSP4Glass_Instance& Src = *safe_cast<const CXR_Model_BSP4Glass_Instance>(&_Instance);
	*this = Src;
}


void CXR_Model_BSP4Glass_Instance::operator = (const CXR_Model_BSP4Glass_Instance& _Instance)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass_Instance::operator = (CXR_Model_BSP4Glass_Instance), XR_BSP4GLASS);
	m_lModels.Clear();
	m_lModels.Add(_Instance.m_lModels);
}


TPtr<CGlassModelShard> CXR_Model_BSP4Glass_Instance::AllocShardModel(CGlassModel* _pModel, const int32 _iInstance, const int32 _Seed, const CMat4Dfp32& _WMat)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass_Instance::AllocShardModel, XR_BSP4GLASS);

	// Create shard model if needed
	if(!_pModel->m_spShardModel)
	{
		_pModel->m_spShardModel = MNew(CGlassModelShard);
		if(!_pModel->m_spShardModel)
		{
			M_ASSERT(_pModel->m_spShardModel, "CXR_Model_BSP4Glass_Instance: Error allocation shard model data!");
			return NULL;
		}

		TPtr<CGlassModelShard> pShardModel = _pModel->m_spShardModel;
		if(_pModel->m_lV.Len())
		{
			//pShardModel->m_BBox = CBox3Dfp32(_pModel->m_lV[0] * _WMat, _pModel->m_lV[0] * _WMat);
			pShardModel->m_BBox = CBox3Dfp32(_pModel->m_lV[0], _pModel->m_lV[0]);
			for(int i = 0; i < _pModel->m_lV.Len(); i++)
				pShardModel->m_BBox.Expand(_pModel->m_lV[i]);
				//pShardModel->m_BBox.Expand(_pModel->m_lV[i] * _WMat);
		}

		pShardModel->m_SpawnTick = _Seed;
		pShardModel->m_iModel = _iInstance;

		// Add model to refresh/render list
		m_lpShardModels.Add(pShardModel);
	}

	// Return shard model
	return _pModel->m_spShardModel;
}


void CXR_Model_BSP4Glass_Instance::RemoveShardModel(CGlassModel* _pModel)
{
	if (_pModel->m_spShardModel)
	{
		CGlassModelShard* pShardModel = _pModel->m_spShardModel;
		
		TAP_RCD<CGlassModelShard*> lpShardModels = m_lpShardModels;
		for (uint i = 0; i < lpShardModels.Len(); i++)
		{
			if (lpShardModels[i] == pShardModel)
			{
				m_lpShardModels.Del(i);
				break;
			}
		}

		_pModel->m_spShardModel = NULL;
	}
}


int32 CXR_Model_BSP4Glass_Instance::GetSurface(uint16 _iInstance)
{
	if (_iInstance >= GLASS_MAX_INSTANCE)
		return 0;

	return m_lModels[_iInstance].m_iSurface;
}


CVec3Dfp32 CXR_Model_BSP4Glass_Instance::GetLocalPosFrom01(uint16 _iInstance, fp32 _x01, fp32 _y01)
{
	if (_iInstance >= GLASS_MAX_INSTANCE)
		return CVec3Dfp32(0.0f);

	CVec3Dfp32* pV = m_lModels[_iInstance].m_lV.GetBasePtr();
	return (pV[0] + ((pV[3] - pV[0]) * _x01) + ((pV[1] - pV[0]) * _y01));
}


bool CXR_Model_BSP4Glass_Instance::GetAttrib_NoPhys(uint16 _iInstance)
{
	if (_iInstance >= GLASS_MAX_INSTANCE)
		return false;

	return m_lModels[_iInstance].m_Attrib.Attrib_NoPhys();
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BSP4Glass
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXR_Model_BSP4Glass::CXR_Model_BSP4Glass()
	: m_pMappings(NULL)
	, m_piFaceMapping(NULL)
	, m_piFaceInstance(NULL)
	, m_pGlassIndices(NULL)
	, m_spLinkContext(NULL)
	, m_spMaster(NULL)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::CXR_Model_BSP4Glass, XR_BSP4GLASS);

	SetThreadSafe(true);
	m_lMappings.Clear();
	m_liFaceMapping.Clear();
	m_lGlassIndices.Clear();
	m_liFaceInstance.Clear();

//	ms_UnifiedZBuffer.SetDefault();
//	ms_UnifiedZBuffer.Attrib_Enable(CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL | CRC_FLAGS_ZCOMPARE | CRC_FLAGS_CULL);
//	ms_UnifiedZBuffer.Attrib_StencilRef(128, 255);
//	ms_UnifiedZBuffer.Attrib_StencilFrontOp(CRC_COMPARE_ALWAYS, CRC_STENCILOP_REPLACE, CRC_STENCILOP_REPLACE, CRC_STENCILOP_REPLACE);
//	ms_UnifiedZBuffer.Attrib_StencilWriteMask(255);
//	//ms_UnifiedZBuffer.Attrib_TexCoordSet(1, 0);
//	#ifdef XR_DEFAULTPOLYOFFSETSCALE
//		ms_UnifiedZBuffer.Attrib_PolygonOffset(XR_DEFAULTPOLYOFFSETSCALE, XR_DEFAULTPOLYOFFSET);
//	#endif
//	//ms_UnifiedZBuffer.Attrib_Enable(CRC_FLAGS_CULLCW);
}


CXR_Model_BSP4Glass::~CXR_Model_BSP4Glass()
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::~CXR_Model_BSP4Glass, XR_BSP4GLASS);
	m_pMappings = NULL;
	m_piFaceMapping = NULL;
	m_pGlassIndices = NULL;
	m_piFaceInstance = NULL;

	m_lMappings.Clear();
	m_liFaceMapping.Clear();
	m_lGlassIndices.Clear();
	m_liFaceInstance.Clear();
}


void CXR_Model_BSP4Glass::PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip, const CXR_AnimState* _pAnimState,
									const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::PreRender, XR_BSP4GLASS);
	CXR_Model_BSP4::PreRender(_pEngine, _pViewClip, _pAnimState, _WMat, _VMat, _Flags);
}


void CXR_Model_BSP4Glass::OnRender(CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_ViewClipInterface* _pViewClip, spCXR_WorldLightState _spWLS,
								   const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int Flags)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass::OnRender, XR_BSP4GLASS);

	if (!_pAnimState)
		return;

	// Create render info
	CXR_RenderInfo RenderInfo(_pEngine);

	// Initialize light infos etc.
	CBSP4Glass_RenderParam RenderParam;
	RenderParam.Clear();
	RenderParam.InitializeRender(_pViewClip, _pVBM, _pEngine, &RenderInfo, _pRender, (CXR_AnimState*)_pAnimState, _spWLS);
	RenderParam.SetupMatrices(_WMat, _VMat);

	// Make sure render params are valid
	if(!RenderParam.IsValid())
		return;

	// Enumerate visible glass indices
	int nVisible = 0;
	uint16 piRet[GLASS_MAX_RENDER];
	const bool bLinkContext = (m_spLinkContext && m_spMaster) ? true : false;
	if(bLinkContext)
	{
		// Enumerate visible glass nodes
		CBSP2_ViewInstance* pView = m_spMaster->m_lspViews[RenderParam.m_pEngine->GetVCDepth()];
		if(pView)
			nVisible = m_spLinkContext->EnumVisible(pView, piRet, GLASS_MAX_RENDER, NULL, 0);

		// Don't do anything, nothing visible
		if(!nVisible)
			return;
	}

	// Get glass instance
	CXR_Model_BSP4Glass_Instance* pGlass = RenderParam.GetGlassInstance();

	// Fetch surface context
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSurfCtx, "SYSTEM.SURFACECONTEXT");
	if (!pSurfCtx)
		Error("OnRender", "No surface-context!");

	CBox3Dfp32 BoundBox;
	Phys_GetBound_Box(_WMat, BoundBox);

	fp32 BasePriority_Opaque = 0;
	fp32 BasePriority_Transparent = 0;
	
	CXR_LightInfo lRenderLightInfo[GLASS_MAX_LIGHTS];

	if(!bLinkContext)
	{
		if(_pViewClip)
		{
			RenderInfo.m_pLightInfo = lRenderLightInfo;
			RenderInfo.m_MaxLights = GLASS_MAX_LIGHTS;
			RenderInfo.m_nLights = 0;

			if(_pAnimState->m_iObject)
			{
				_pViewClip->View_GetClip(_pAnimState->m_iObject, &RenderInfo);
				if((RenderInfo.m_Flags & CXR_RENDERINFO_INVISIBLE) && !RenderInfo.m_nLights)
					return;
			}
			else
			{
				if(!_pViewClip->View_GetClip_Box(BoundBox.m_Min, BoundBox.m_Max, 0, 0, NULL, &RenderInfo))
					return;
			}

			BasePriority_Opaque = RenderInfo.m_BasePriority_Opaque;
			BasePriority_Transparent = RenderInfo.m_BasePriority_Transparent;
		}

		// Simple per node culling rendering.
		#if GLASS_DEBUG
			ClearRenderCalls(_pAnimState);
		#endif

		const int nIndices = m_lGlassIndices.Len();
		for(int i = 0; i < nIndices; i++)
			OnRender_Index(m_pGlassIndices[i], &RenderParam, 0);
	}
	else
	{
		RenderInfo.m_pLightInfo = lRenderLightInfo;

		// Render visible glass indices
		#if GLASS_DEBUG
			ClearRenderCalls(_pAnimState, piRet, nVisible);
		#endif

		Wallmark_Render(&RenderParam, pGlass->m_spWMC, piRet, nVisible);

		for(int i = 0; i < nVisible; i++)
		{
			// Fetch index. Do some debuging if not rtm
			#ifndef M_RTM
			{			
				if(piRet[i] == 0)
				{
					ConOut("§cff0WARNING (CXR_Model_BSP4Glass::OnRender): Fetching index 0 from link context!");
					continue;
				}
			}
			#endif

			const uint16& iID = piRet[i];
			OnRender_Index(m_pGlassIndices[iID-1], &RenderParam, iID);

			/* Never ever render client grid
			#ifndef M_RTM
				MACRO_GetRegisterObject(CWireContainer, pWC, "GAMECONTEXT.CLIENT.WIRECONTAINER");
				if(pWC)
				{
					CXR_Model_BSP4Glass_Instance* pGlass = RenderParam.GetGlassInstance();
					CGlassModel* pGlassModels = pGlass->m_lModels.GetBasePtr();

					const CBSP4Glass_Index& GlassIndex = m_pGlassIndices[iID-1];
					for(uint32 i = 0; i < GlassIndex.m_nInstance; i++)
					{
						CGlassModel& GlassInfo = pGlassModels[GlassIndex.m_iInstance+i];
						CBSP4Glass_Grid& GlassGrid = GlassInfo.m_Grid;
						GlassGrid.Debug_RenderGrid(pWC, CPixel32(255,0,0,255), RenderParam.m_M2W);
					}
				}
			#endif
			*/
		}
	}

	// Render all shard 'models'
	int nShardModels = pGlass->m_lpShardModels.Len();
	if(nShardModels > 0)
	{
		CXR_SurfaceContext* pSC = RenderParam.m_pEngine->GetSC();
		CXW_Surface* pSurfaceCrush = (pSC) ? pSC->GetSurface("glass_shatter") : NULL;

		// Recalculate some of the matrices we use since shard vertices are given in world-space
		//CMat4Dfp32 Unit;
		//Unit.Unit();
		//RenderParam.RecalcMatrices(Unit, _VMat, false, false, false, true, true);

		// Fetch shard information
		TAP_RCD<CGlassModelShard*> lpShardModels = pGlass->m_lpShardModels;
		
		// Render all shard models
		for(int i = 0; i < nShardModels; i++)
		{
			// Get shard model and surface
			CGlassModelShard* pShardModel = lpShardModels[i];
			if(pShardModel->m_nShards <= 0)
				continue;

			CXW_Surface* pSurface = m_lspSurfaces[pShardModel->m_iSurface];

			// Recalculate matrices
			//RenderParam.RecalcMatrices(pGlass->m_lModels[pShardModel->m_iModel].m_Grid.GetPositionMatrix(), _VMat, false, false, false, false, false);
			//RenderParam.RecalcMatrices(_WMat, _VMat, false, false, false, false, false);

			RenderInfo.m_pLightInfo = lRenderLightInfo;
			RenderInfo.m_MaxLights = GLASS_MAX_LIGHTS;
			RenderInfo.m_nLights = 0;

			// And call for render
			pShardModel->Render(&RenderParam, pSurface, _WMat, pSurfaceCrush);
		}

		// Render shard debugging information
		#ifndef M_RTM
		{
			MACRO_GetRegisterObject(CWireContainer, pWC, "GAMECONTEXT.CLIENT.WIRECONTAINER");
			if(pWC)
			{
				for(int i = 0; i < nShardModels; i++)
					lpShardModels[i]->Debug_Render(pWC);
			}
		}
		#endif
	}

	// Commit light infos and end glass rendering
	RenderParam.CommitRender();
}


void CXR_Model_BSP4Glass::OnRender_Index(const CBSP4Glass_Index& _Index, CBSP4Glass_RenderParam* _pRenderParam, const uint16& _iID)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass::OnRender_Index, XR_BSP4GLASS);

	CXR_Model_BSP4Glass_Instance* pGlass = _pRenderParam->GetGlassInstance();
	CGlassModel* pGlassModels = pGlass->m_lModels.GetBasePtr();

	// Return if nothing needs rendering
	bool bRender = false;
	for(uint32 i = 0; i < _Index.m_nInstance; i++)
	{
		CGlassAttrib& Attrib = pGlassModels[_Index.m_iInstance+i].m_Attrib;
		if (!Attrib.Attrib_NoBaseRender() && !Attrib.Attrib_Inactive())
		{
			bRender = true;
			break;
		}
	}
	if(!bRender)
		return;
    
	// Transform bounding box
	CBox3Dfp32 BoundingBox;
	_Index.m_BoundingBox.Transform(_pRenderParam->m_M2W, BoundingBox);
	CXR_RenderInfo& RenderInfo = *_pRenderParam->m_pRenderInfo;

	const CMat4Dfp32& M2W = _pRenderParam->m_M2W;
	const CMat4Dfp32& M2V = _pRenderParam->m_M2V;
	const CMat4Dfp32& W2V = _pRenderParam->m_W2V;
	const CMat4Dfp32& InvM2W = _pRenderParam->m_InvM2W;

	CXR_VBManager* pVBM = _pRenderParam->m_pVBM;
	CXR_ViewClipInterface* pViewClip = _pRenderParam->m_pViewClip;

	// Fetch scenegraph
	CXR_SceneGraphInstance* pSGI = _pRenderParam->GetSceneGraphInstance();

    if(_iID == 0)
	{
		if (!pViewClip->View_GetClip_Box(BoundingBox.m_Min, BoundingBox.m_Max, 0, 0, NULL, &RenderInfo))
			return;
	}
	else
	{
		uint16 piPL[GLASS_MAX_PORTALLEAVES];
		const int nPL = m_spLinkContext->EnumPortalLeaves(_iID, piPL, GLASS_MAX_PORTALLEAVES);

		// Enumerate lights
		CXR_Light const* lpLights[16];
		int nLights = pSGI->SceneGraph_Light_Enum(piPL, nPL, BoundingBox, lpLights, GLASS_MAX_LIGHTS);

		// Copy enumerated lights to render info
		RenderInfo.m_nLights = nLights;
		for(int i = 0; i < nLights; i++)
		{
			CXR_LightInfo& LI = RenderInfo.m_pLightInfo[i];
			LI.m_pLight = lpLights[i];
			LI.m_Scissor.SetRect(0xffff, 0x0000);
			LI.m_ShadowScissor.SetRect(0xffff, 0x0000);
//			LI.m_Scissor.m_Max = LI.m_ShadowScissor.m_Max = 0;
//			LI.m_Scissor.m_Min = LI.m_ShadowScissor.m_Min = 0xffff;
		}
	}

	// Front or backside rendering?
	CVec3Dfp32 Dir = (_Index.m_Center - _pRenderParam->m_MVPos).Normalize();
	
	// Render all the instances
	for(int i = 0; i < _Index.m_nInstance; i++)
	{
		CGlassModel& _GlassInfo = pGlassModels[_Index.m_iInstance+i];
		if (!_GlassInfo.m_Attrib.Attrib_NoBaseRender())
		{
			const bool bFrontSide = ((Dir * _GlassInfo.m_Attrib.m_Plane.n) < 0.0f) ? true : false;

			#if GLASS_DEBUG
				_GlassInfo.m_Debug_nRendered++;

				// Render a sphere around glass index being rendered
				MACRO_GetRegisterObject(CWireContainer, pWC, "GAMECONTEXT.CLIENT.WIRECONTAINER");
				if(pWC)
				{
					CMat4Dfp32 Mat;
					Mat = M2W;

					CMat4Dfp32 SpherePos;
					SpherePos.Unit();
					SpherePos.GetRow(3) = _Index.m_Center * M2W;//WMat;
					if(_GlassInfo.m_Debug_nRendered > 1)
					{
						CPixel32 Color = CPixel32(255,0,0,255);
						pWC->RenderSphere(SpherePos, _Index.m_Radius+2.0f, Color, 0.0f, false);
						pWC->RenderSphere(SpherePos, 4.0f, Color, 0.0f, false);
					}
					else
					{
						CPixel32 Color(255,255,0,255);
						pWC->RenderSphere(SpherePos, _Index.m_Radius, Color, 0.0f, false);
						pWC->RenderVertex(SpherePos.GetRow(3), Color, 0.0f, false);
					}

					bool bUnbreakable = _GlassInfo.m_Attrib.Attrib_Unbreakable();
					bool bHasDurability = _GlassInfo.m_Attrib.Attrib_UseDurability();
					uint32 DurLife = _GlassInfo.m_Attrib.m_Durability;
					CXR_Util::Render_Text(pVBM, pGlass->m_pDebug_Font, _Index.m_Center * M2W, W2V, CStrF("Glass Index: %d - Hash: %d - Prop: %s(%d)", _Index.m_iInstance, _Index.m_NameHash,
						(bUnbreakable ? "Unbreakable" : (bHasDurability ? "Durability" : "N/A")), (bUnbreakable ? 0 : (bHasDurability ? DurLife : 0))), 2.0f);
				}
			#endif

			// Fetch surface
			CXW_Surface* pSurf = m_lspSurfaces[_GlassInfo.m_iSurface];
			CXR_SurfaceContext* pSC = _pRenderParam->m_pEngine->GetSC();
			CXW_SurfaceKeyFrame* pSurfKey = _pRenderParam->GetSurfaceKey(pSurf, pSC->GetTempSurfaceKeyFrame());

			// Create shader parameters
			CXR_SurfaceShaderParams SSP;
			CXR_ShaderParams ShaderParams;
			SSP.Create(pSurf, pSurfKey);
			ShaderParams.Create(&M2W, &W2V, _pRenderParam->m_pEngine->GetShader());
			const CXR_SurfaceShaderParams* lpSSP[1] = { &SSP };

			// First, we render all the glass stuff, using no stencil tests, transparent and z less.
			CVec3Dfp32 WCenter = _Index.m_Center * M2W;
			fp32 Dist = -_pRenderParam->m_FrontPlaneW.Distance(WCenter) * _pRenderParam->m_BackPlaneInv;
			ShaderParams.m_Priority = ClampRange(Dist + 0.25f, 1.5f) - 0.25f + CXR_VBPRIORITY_MODEL_TRANSPARENT;
			//ShaderParams.m_Flags |= XR_SHADERFLAGS_NOSTENCILTEST | XR_SHADERFLAGS_USEZLESS | XR_SHADERFLAGS_TRANSPARENT;
			//ShaderParams.m_Flags &= ~XR_SHADERFLAGS_USEZEQUAL;
			ShaderParams.m_Flags = XR_SHADERFLAGS_NOSTENCILTEST | XR_SHADERFLAGS_USEZLESS | XR_SHADERFLAGS_TRANSPARENT | XR_SHADERFLAGS_GLASS;

			// Render mask
			OnRender_IndexMask(_GlassInfo, _pRenderParam, pSurfKey, ShaderParams.m_Priority - 0.0002f, false, bFrontSide);
			OnRender_IndexMask(_GlassInfo, _pRenderParam, pSurfKey, ShaderParams.m_Priority - 0.0002f, true, bFrontSide);

			// Calculate scissor box for glass instance
			CRect2Duint16 GlassBoundingScissor;
			CBox3Dfp32 GlassBounding = BoundingBox;
			GlassBounding.Grow(8.0f);
			CXR_Model_BSP4Glass::CalcBoxScissor(pVBM, W2V, GlassBounding, GlassBoundingScissor);
			
			// Maximize to viewport and hope this shit helps for the moment, otherwise I just fail to understand how light occlusion info works =)
			const CRct& VPRect = pVBM->Viewport_Get()->GetViewRect();
			GlassBoundingScissor = CRect2Duint16(CVec2Duint16((uint16)MaxMT(0,VPRect.p0.x),(uint16)MaxMT(0,VPRect.p0.y)),
												 CVec2Duint16((uint16)VPRect.GetWidth(),(uint16)VPRect.GetHeight()));

			// Render shading
			int nRenderLights = RenderInfo.m_nLights;
			for(int j = 0; j < nRenderLights; j++)
			{	
				// Fetch light and transform
				CXR_Light Light;
				pSGI->SceneGraph_Light_Get(RenderInfo.m_pLightInfo[j].m_pLight->m_iLight, Light);
				
				#ifndef M_RTM
					// Render light bounding box and print out light id if scissor hasn't been set (means first time we use it in glass renderer)
					if(pWC && !_pRenderParam->IsValid_LightScissor(Light.m_iLight))
					{
						//pWC->RenderAABB(Light.m_BoundBox, CPixel32(0,255,0,255), 0.0f, false);
						CXR_Util::Render_Text(pVBM, pGlass->m_pDebug_Font, Light.m_Pos.GetRow(3), W2V, CStrF("Light Index: %d", Light.m_iLight), 2.0f);
					}
				#endif

				// Transform light
				Light.Transform(InvM2W);

				// Expand scissor boxes
				_pRenderParam->ExpandLightOcclusion(Light.m_iLight, GlassBoundingScissor);

				// Render glass using current light light
				OnRender_IndexShading(_GlassInfo, _pRenderParam, ShaderParams, &SSP, Light, false, bFrontSide);
				OnRender_IndexShading(_GlassInfo, _pRenderParam, ShaderParams, &SSP, Light, true, bFrontSide);
			}
		}
	}
}


void CXR_Model_BSP4Glass::CalcBoxScissor(CXR_VBManager* _pVBM, const CMat4Dfp32& _VMat, const CBox3Dfp32& _Box, CRect2Duint16& _Scissor)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass::CalcBoxScissor, XR_BSP4GLASS);

	CVec3Dfp32 BoxV[8];
	_Box.GetVertices(BoxV);

	CRC_Viewport* pVP = _pVBM->Viewport_Get();
	const CRct& VPRect = pVP->GetViewRect();
	const fp32 XScale = pVP->GetXScale() * 0.5f;
	const fp32 YScale = pVP->GetYScale() * 0.5f;
	const fp32 XMid = (VPRect.p0.x + VPRect.p1.x) >> 1;
	const fp32 YMid = (VPRect.p0.y + VPRect.p1.y) >> 1;

	const int ScrX = VPRect.p0.x;
	const int ScrY = VPRect.p0.y;
	const int ScrW = VPRect.GetWidth();
	const int ScrH = VPRect.GetHeight();
	CVec2Dfp32 VMin(_FP32_MAX);
	CVec2Dfp32 VMax(-_FP32_MAX);

	const CMat4Dfp32* pM = &_VMat;

	for(int v = 0; v < 8; v++)
	{
		fp32 vx = BoxV[v].k[0];
		fp32 vy = BoxV[v].k[1];
		fp32 vz = BoxV[v].k[2];
		fp32 z = pM->k[0][2]*vx + pM->k[1][2]*vy + pM->k[2][2]*vz + pM->k[3][2];
		if (z < 0.1f) 
		{ 
			_Scissor.m_Min[0] = ScrX;
			_Scissor.m_Min[1] = ScrY;
			_Scissor.m_Max[0] = ScrX + ScrW;
			_Scissor.m_Max[1] = ScrY + ScrH;
			return;
		}
		fp32 zinv = 1.0f / z;
		fp32 x = (pM->k[0][0]*vx + pM->k[1][0]*vy + pM->k[2][0]*vz + pM->k[3][0]) * zinv;
		VMin.k[0] = Min(VMin.k[0], x);
		VMax.k[0] = Max(VMax.k[0], x);
		fp32 y = (pM->k[0][1]*vx + pM->k[1][1]*vy + pM->k[2][1]*vz + pM->k[3][1]) * zinv;
		VMin.k[1] = Min(VMin.k[1], y);
		VMax.k[1] = Max(VMax.k[1], y);
	}

	{
		int xmin = RoundToInt(VMin[0] * XScale + XMid);
		int min0 = Max(ScrX, Min(ScrX + ScrW, xmin));
		int ymin = RoundToInt(VMin[1] * YScale + YMid);
		int min1 = Max(ScrY, Min(ScrY + ScrH, ymin));

		int xmax = RoundToInt(VMax[0] * XScale + XMid);
		int max0 = Max(ScrX, Min(ScrX + ScrW, xmax));
		int ymax = RoundToInt(VMax[1] * YScale + YMid);
		int max1 = Max(ScrY, Min(ScrY + ScrH, ymax));

		_Scissor.m_Min[0] = min0;
		_Scissor.m_Min[1] = min1;
		_Scissor.m_Max[0] = Max(max0, min0);
		_Scissor.m_Max[1] = Max(max1, min1);
	}
}


CXR_VBChain* CXR_Model_BSP4Glass::VB_CreateChain(CGlassModel& _GlassInfo, CXR_VBManager* _pVBM, const CMat4Dfp32& _M2W, const CMat4Dfp32& _W2V, const bool _bEdge, const bool _bFrontSide)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::VB_CreateChain, XR_BSP4GLASS);

	CXR_VBChain* pChain = _pVBM->Alloc_VBChain();
	if(!pChain)
		return NULL;

	// Setup chain information
	pChain->m_pV			= _GlassInfo.m_lV.GetBasePtr();
	pChain->m_nV			= _GlassInfo.m_lV.Len();
	pChain->m_pN			= _GlassInfo.m_lN.GetBasePtr();
	pChain->m_pCol			= _GlassInfo.m_lC.GetBasePtr();
	pChain->m_pTV[0]		= (fp32*)_GlassInfo.m_lTV.GetBasePtr();
	pChain->m_pTV[1]		= (fp32*)_GlassInfo.m_lTangU.GetBasePtr();
	pChain->m_pTV[2]		= (fp32*)_GlassInfo.m_lTangV.GetBasePtr();
	pChain->m_nTVComp[0]	= 2;
	pChain->m_nTVComp[1]	= 3;
	pChain->m_nTVComp[2]	= 3;
	pChain->m_TaskId		= 0xffff;

	if(_bEdge)
	{
		// Fetch edge rendering information
		pChain->m_piPrim		= _GlassInfo.m_lEdgeP.GetBasePtr();
		pChain->m_nPrim			= _GlassInfo.m_nRenderEdgePrim;
		pChain->m_PrimType		= CRC_RIP_TRIANGLES;
	}
	else
	{
		// Decide back or front face rendering
		pChain->m_piPrim	= ((_bFrontSide) ? _GlassInfo.m_lFrontP.GetBasePtr() : (_GlassInfo.m_lFrontP.GetBasePtr() + (_GlassInfo.m_nRenderMainPrim*3)));
		pChain->m_nPrim		= _GlassInfo.m_nRenderMainPrim;
		pChain->m_PrimType	= CRC_RIP_TRIANGLES;
	}

	return pChain;
}


void CXR_Model_BSP4Glass::OnRender_IndexShading(CGlassModel& _GlassInfo, CBSP4Glass_RenderParam* _pRenderParam, const CXR_ShaderParams& _ShaderParams, const CXR_SurfaceShaderParams* _pSurfParams, const CXR_Light& _Light, const bool _bEdge, const bool _bFrontSide)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass::OnRender_IndexShading, XR_BSP4GLASS);

	if(_bEdge && _GlassInfo.m_Attrib.Attrib_IgnoreEdge())
		return;

	const CMat4Dfp32& M2V = _pRenderParam->m_M2V;
	const CMat4Dfp32& M2W = _pRenderParam->m_M2W;
	const CMat4Dfp32& W2V = _pRenderParam->m_W2V;
	CXR_VBManager* pVBM = _pRenderParam->m_pVBM;

	//CXR_VBChain* pChain = VB_CreateChain(_GlassInfo, pVBM, M2W, W2V, _bEdge, _GlassInfo.m_Attrib.Attrib_UseThickness() ? _bFrontSide : true);
	CXR_VBChain* pChain = VB_CreateChain(_GlassInfo, pVBM, M2W, W2V, _bEdge, _bFrontSide);
	CMat4Dfp32* pVBM2V = pVBM->Alloc_M4(M2V);
	if(!pChain || !pVBM2V)
		return;

	CXR_VertexBuffer* pVB = (CXR_VertexBuffer*)pVBM->Alloc(TUtilAlign(sizeof(CXR_VertexBuffer)));
	if(pVB)
	{
		pVB->Construct();
		//CXR_VertexBuffer VB;
		//VB.m_pVBChain = pChain;
		//VB.m_Flags |= CXR_VBFLAGS_VBCHAIN;// | CXR_VBFLAGS_LIGHTSCISSOR;
		//VB.Matrix_Set(pVBM2V);

		pVB->m_pVBChain = pChain;
		pVB->m_Flags = CXR_VBFLAGS_VBCHAIN;
		pVB->Matrix_Set(pVBM2V);

		const CXR_SurfaceShaderParams* lpSSP[1] = { _pSurfParams };
		_pRenderParam->GetShader()->RenderShading(_Light, pVB, &_ShaderParams, lpSSP);
	}
}


void CXR_Model_BSP4Glass::OnRender_IndexMask(CGlassModel& _GlassInfo, CBSP4Glass_RenderParam* _pRenderParam, CXW_SurfaceKeyFrame* _pSurfKey, const fp32& _Priority, const bool _bEdge, const bool _bFrontSide)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass::OnRender_IndexMask, XR_BSP4GLASS);

	if(_bEdge && _GlassInfo.m_Attrib.Attrib_IgnoreEdge())
		return;

	const CMat4Dfp32& M2V = _pRenderParam->m_M2V;
	const CMat4Dfp32& M2W = _pRenderParam->m_M2W;
	const CMat4Dfp32& W2V = _pRenderParam->m_W2V;
	CXR_VBManager* pVBM = _pRenderParam->m_pVBM;

	CXR_VBChain* pChain = VB_CreateChain(_GlassInfo, pVBM, M2W, W2V, _bEdge, _GlassInfo.m_Attrib.Attrib_UseThickness() ? _bFrontSide : true);
	CRC_Attributes* pA = pVBM->Alloc_Attrib();
	CMat4Dfp32* pVBM2V = pVBM->Alloc_M4(M2V);
	if(!pChain || !pVBM2V || !pA)
		return;

	uint32 Color(0xffffffff);
	uint32 DiffuseTxtID = 0;
	const int nLayers = _pSurfKey->m_lTextures.Len();
	const CXW_SurfaceLayer* pLayers = _pSurfKey->m_lTextures.GetBasePtr();

	for(int i = 0; i < nLayers; i++)
	{
		if (pLayers->m_Type)
		{
			uint32 iTxt = pLayers->m_Type-1;
			if (iTxt <= XW_LAYERTYPE_ANISOTROPICDIR)
			{
				if (iTxt == XR_SHADERMAP_DIFFUSE)
				{
//					uint8 A = TruncToInt(Clamp01(pLayers->m_HDRColor.k[3])*255.0f);
//					Color = CPixel32(0,0,0,A);

					vec128 col = M_VLd_V4f16_f32(&pLayers->m_HDRColor);
					vec128 scale = M_VConst(0,0,0,255.0f);
					M_VSt_V4f32_Pixel32(M_VMul(col, scale), &Color);

					DiffuseTxtID = pLayers->m_TextureID;
					break;
				}
			}
		}

		pLayers++;
	}

	// Setup attributes
	pA->SetDefault();
	pA->Attrib_RasterMode(CRC_RASTERMODE_ONE_INVALPHA);
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE);
	pA->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);
	pA->Attrib_TextureID(0, DiffuseTxtID);
	
	CXR_VertexBuffer VB;	
	VB.m_pAttrib = pA;
	VB.m_pVBChain = pChain;
	VB.m_Flags |= CXR_VBFLAGS_VBCHAIN;
	VB.Matrix_Set(pVBM2V);
	VB.m_Priority = _Priority;
	VB.Geometry_Color(Color);

	pVBM->AddVB(VB);
}


/*
void CXR_Model_BSP4Glass::OnRender_IndexZBuffer(const CSGlassInfo& _GlassInfo, CXR_VBManager* _pVBM, const CXR_Engine* _pEngine, const CMat4Dfp32& _M2V, const CXW_SurfaceLayer* _pLayers, const bool _bEdge)
{
	CXR_VertexBuffer* pVB	= _pVBM->Alloc_VB();
	CXR_VBChain* pChain		= VB_CreateChain(_GlassInfo, _pVBM, _bEdge);
	CMat4Dfp32* pVBM2V		= _pVBM->Alloc_M4(_M2V);

	if(!pVB || !pChain || !pVBM2V)
		return;

	CRC_Attributes* pZAttr = &ms_UnifiedZBuffer;

	uint32 Ambience = const_cast<CXR_Engine*>(_pEngine)->GetVar(XR_ENGINE_UNIFIED_AMBIENCE);
	pVB->Geometry_Color(Ambience);

	if(_pLayers->m_Flags & XW_LAYERFLAGS_ALPHACOMPARE)
	{
		CRC_Attributes* pA = _pVBM->Alloc_Attrib();
		if(!pA)
			return;

		*pA = *pZAttr;
		pA->Attrib_TextureID(0, _pLayers->m_TextureID);
		pA->Attrib_AlphaCompare(_pLayers->m_AlphaFunc, _pLayers->m_AlphaRef);
		pVB->m_pAttrib = pA;
	}
	else
	{
		pVB->m_pAttrib = pZAttr;
	}

	pVB->m_Flags	|= CXR_VBFLAGS_VBCHAIN;
	pVB->m_Priority	 = CXR_VBPRIORITY_UNIFIED_ZBUFFER;
	pVB->m_pVBChain	 = pChain;
	
	pVB->Matrix_Set(pVBM2V);
	_pVBM->AddVB(pVB);
}
*/


#if GLASS_DEBUG
void CXR_Model_BSP4Glass::ClearRenderCalls(const CXR_AnimState* _pAnimState, const uint16* _piRet, const int& _nVisible)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::ClearRenderCalls, XR_BSP4GLASS);

	if(!_pAnimState)
		return;

	CXR_Model_BSP4Glass_Instance* pGlass = (CXR_Model_BSP4Glass_Instance*)_pAnimState->m_pModelInstance;
	CGlassModel* pGlassModels = pGlass->m_lModels.GetBasePtr();
	if(_piRet)
	{
		TAP_RCD<CBSP4Glass_Index> lpGlassIndices(m_lGlassIndices);
		for(int i = 0; i < _nVisible; i++)
		{
			CBSP4Glass_Index& GlassIndex =  lpGlassIndices[_piRet[i]-1];
			for(int j = 0; j < GlassIndex.m_nInstance; j++)
				pGlassModels[GlassIndex.m_iInstance+j].m_Debug_nRendered = 0;
		}
	}
	else
	{
		const int nGlassModels = pGlass->m_lModels.Len();

		for(int i = 0; i < nGlassModels; i++)
			pGlassModels[i].m_Debug_nRendered = 0;
	}
}
#endif


void CXR_Model_BSP4Glass::OnPrecache(CXR_Engine* _pEngine, int _iVariation)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::OnPrecache, XR_BSP4GLASS);
	MAUTOSTRIP(CXR_Model_BSP4Glass_OnPrecache, MAUTOSTRIP_VOID);
	
	MACRO_GetRegisterObject(CTextureContext, pTC, "SYSTEM.TEXTURECONTEXT");
	if (!pTC)
		Error("Read", "No texture-context available.");

	// Precache textures
	{
		// Set precache flags on all surface textures
		int nSurfaces = m_lspSurfaces.Len();
		for(int iSurf = 0; iSurf < nSurfaces; iSurf++)
		{
			CXW_Surface* pSurface = m_lspSurfaces[iSurf];
			pSurface = (pSurface) ? pSurface->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps) : NULL;
			if (!pSurface)
				continue;

			// Don't report failures and set precache flag if surface isn't of invisible type
			pSurface->InitTextures(false);
			if(!(pSurface->m_Flags & XW_SURFFLAGS_INVISIBLE))
				pSurface->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
		}
	}

	// Extra textures used on glass model
	CXR_SurfaceContext* pSurfCtx = _pEngine->GetSC();
	if(pSurfCtx)
	{
		PrecacheSurface(_pEngine, pSurfCtx, "glass_shatter");
	}
}


void CXR_Model_BSP4Glass::PrecacheSurface(CXR_Engine* _pEngine, CXR_SurfaceContext* _pSurfCtx, const char* _pSurfaceName)
{
	CXW_Surface* pSurface = _pSurfCtx->GetSurface(_pSurfaceName);
	pSurface = (pSurface) ? pSurface->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps) : NULL;
	if(!pSurface)
		return;

	pSurface->InitTextures(false);
	if(!(pSurface->m_Flags & XW_SURFFLAGS_INVISIBLE))
		pSurface->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
}


TPtr<CXR_ModelInstance> CXR_Model_BSP4Glass::CreateModelInstance()
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::CreateModelInstance, XR_BSP4GLASS);
	return MNew(CXR_Model_BSP4Glass_Instance);
}


void CXR_Model_BSP4Glass::GetGlassCreateInfo(CGlassCreateInfo& _GlassInfo)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::GetGlassCreateInfo, XR_BSP4GLASS);

	MACRO_GetRegisterObject(CXR_SurfaceContext, pSurfCtx, "SYSTEM.SURFACECONTEXT");
	if (!pSurfCtx)
		Error("CreateModelInstance", "No surface-context!");

	InitializeListPtrs();

	int nIndices = m_lGlassIndices.Len();
	_GlassInfo.m_lModels.SetLen(nIndices);
	if (nIndices == 0)
		return;

	TAP_RCD<CGlassCreateInfo::SGlassCreateInfo> pModels = _GlassInfo.m_lModels;
	for (int i = 0; i < nIndices; i++)
		pModels[i].m_bValid = false;

	TAP_RCD<uint16> liFaceInstance = m_liFaceInstance;
	TAP_RCD<CXR_PlaneMapping> pMappings = m_lMappings;
	int nFaces = m_lFaces.Len();
	for(int i = 0; i < nFaces; i++)
	{
		const CBSP4_Face& Face = m_pFaces[i];
		if(m_lspSurfaces[Face.m_iSurface]->m_Name.CompareNoCase("*GLASS") == 0)
			continue;

		CGlassCreateInfo::SGlassCreateInfo ModelInfo;
		ModelInfo.m_Thickness = m_pMediums[Face.m_iBackMedium].m_Thickness;
		ModelInfo.m_Plane = m_pPlanes[Face.m_iPlane];
		ModelInfo.m_iSurface = Face.m_iSurface;
		ModelInfo.m_pMapping = &pMappings[m_piFaceMapping[i]];
		ModelInfo.m_Durability = m_pMediums[Face.m_iBackMedium].m_User1;
		ModelInfo.m_UserFlags = m_pMediums[Face.m_iBackMedium].m_User2;
		ModelInfo.m_bValid = true;

		CreateInstance_CollectFaceData(ModelInfo, Face);
		CreateInstance_GetPrimCount(ModelInfo, Face, true);
		CreateInstance_CollectRenderPrim(ModelInfo, Face);
		CreateInstance_CollectSurfaceData(ModelInfo, Face.m_iSurface);

		uint16 iFaceInstance = liFaceInstance[i];
		if (iFaceInstance != GLASS_MAX_INSTANCE && !pModels[iFaceInstance].m_bValid)
			pModels[iFaceInstance] = ModelInfo;

		CreateInstance_Clear(ModelInfo);
	}
}


void CXR_Model_BSP4Glass::CreateInstance_Clear(CSGlassCreateInfo& _CreateInfo)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::CreateInstance_Clear, XR_BSP4GLASS);
	_CreateInfo.m_lV.Clear();
	_CreateInfo.m_lFrontP.Clear();
	_CreateInfo.m_lEdgeP.Clear();
	_CreateInfo.m_bValid = false;
}


void CXR_Model_BSP4Glass::CreateInstance_CollectSurfaceData(CSGlassCreateInfo& _CreateInfo, const int& _iSurface)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::CreateInstance_CollectSurfaceData, XR_BSP4GLASS);

	_CreateInfo.m_iSurface = _iSurface;

	CXW_Surface* pSurface = m_lspSurfaces[_iSurface];

	CVec3Dfp32 RefDim = pSurface->GetTextureMappingReferenceDimensions();
	_CreateInfo.m_TxtWidthInv = 1.0f / RefDim[0];
	_CreateInfo.m_TxtHeightInv = 1.0f / RefDim[1];
}


void CXR_Model_BSP4Glass::CreateInstance_CollectRenderPrim(CSGlassCreateInfo& _CreateInfo, const CBSP4_Face& _Face)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::CreateInstance_CollectRenderPrim, XR_BSP4GLASS);

	const bool bUseThickness = ((_CreateInfo.m_UserFlags & CGlassAttrib::ATTRIB_USETHICKNESS) != 0);
	const bool bIgnoreEdges = ((_CreateInfo.m_UserFlags & CGlassAttrib::ATTRIB_IGNOREEDGE) != 0);

	uint16* pFrontP = _CreateInfo.m_lFrontP.GetBasePtr();
	uint16* pBackP = pFrontP + (_CreateInfo.m_nRenderMainPrim * 3); //(bUseThickness) ? (pFrontP + (_CreateInfo.m_nRenderMainPrim * 3)) : NULL;
	uint16* pEdgeP = _CreateInfo.m_lEdgeP.GetBasePtr();
	CVec3Dfp32* pV = _CreateInfo.m_lV.GetBasePtr();
	const int& nV = _CreateInfo.m_lV.Len();
	int nP = 0;
	int iV = 0;

	{
		const CBSP4_Face& Face = _Face;
		const int nv = Face.m_nVertices;
		const int nFacePrim = nv - 2;

		// Setup front primitives
		for(int j = 0; j < nFacePrim; j++)
		{
			pFrontP[nP++] = iV;
			pFrontP[nP++] = iV+j+1;
			pFrontP[nP++] = iV+j+2;
		}

		// Setup back primitves if we want them
		//if(bUseThickness)
		{
			nP -= (3 * nFacePrim);
			for(int j = 0; j < nFacePrim; j++)
			{
				pBackP[nP++] = nV+iV;
				pBackP[nP++] = nV+iV+j+2;
				pBackP[nP++] = nV+iV+j+1;
			}
		}

		for(int j = 0; j < nv; j++)
			pV[iV+j] = m_pVertices[m_piVertices[Face.m_iiVertices+j]];

		iV += nv;
	}

	// Setup edge triangles rendering information
	if(bUseThickness && !bIgnoreEdges)
	{
		CSBSP4Glass_Edge* pEdges = _CreateInfo.m_Edges.m_lEdgeData.GetBasePtr();
		const int nEdges = _CreateInfo.m_Edges.NumEdges();

		nP = 0;
		for(int i = 0; i < nEdges; i++)
		{
			pEdgeP[nP++] = pEdges[i].m_i1;
			pEdgeP[nP++] = pEdges[i].m_i1 + nV;
			pEdgeP[nP++] = pEdges[i].m_i2;

			pEdgeP[nP++] = pEdges[i].m_i2;
			pEdgeP[nP++] = pEdges[i].m_i1 + nV;
			pEdgeP[nP++] = pEdges[i].m_i2 + nV;
		}
	}
}


void CXR_Model_BSP4Glass::CreateInstance_GetPrimCount(CSGlassCreateInfo& _CreateInfo, const CBSP4_Face& _Face, const bool& _bAlloc)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::CreateInstance_GetPrimCount, XR_BSP4GLASS);

	int nTri = 0;
	int nV = 0;
	int nP = 0;

	const bool bUseThickness = ((_CreateInfo.m_UserFlags & CGlassAttrib::ATTRIB_USETHICKNESS) != 0);
	const bool bIgnoreEdges = ((_CreateInfo.m_UserFlags & CGlassAttrib::ATTRIB_IGNOREEDGE) != 0);

	// Front back primitives
	{
		nV += _Face.m_nVertices;
		nTri += _Face.m_nVertices - 2;
		nP += (nTri * 6);//bUseThickness ? (nTri * 6) : (nTri * 3);
		_CreateInfo.m_nRenderMainPrim = nTri;
		
		if(_bAlloc)
		{
			_CreateInfo.m_lFrontP.SetLen(nP);
			_CreateInfo.m_lV.SetLen(nV);
		}
	}

	// Edge primitives
	if(bUseThickness && !bIgnoreEdges)
	{
		_CreateInfo.m_Edges.RemoveSharedEdges();
		nTri = (_CreateInfo.m_Edges.NumEdges() * 2);

		_CreateInfo.m_nRenderEdgePrim = nTri;
		if(_bAlloc)
			_CreateInfo.m_lEdgeP.SetLen(nTri * 3);
	}
}


void CXR_Model_BSP4Glass::CreateInstance_CollectFaceData(CSGlassCreateInfo& _CreateInfo, const CBSP4_Face& _Face)
{
	GLASS_MSCOPE_ALL(CXR_Model_BSP4Glass::CreateInstance_CollectFaceData, XR_BSP4GLASS);

	int nV = 0;
	int nPrim = 0;

	const bool bUseThickness = ((_CreateInfo.m_UserFlags & CGlassAttrib::ATTRIB_USETHICKNESS) != 0);
	const bool bIgnoreEdges = ((_CreateInfo.m_UserFlags & CGlassAttrib::ATTRIB_IGNOREEDGE) != 0);
	{
		const CBSP4_Face& Face = _Face;

		nPrim = (Face.m_nVertices - 2);
		if(bUseThickness && !bIgnoreEdges)
		{
			for(int j = 0; j < nPrim; j++)
			{
				_CreateInfo.m_Edges.AddEdge(nV, nV+j+1);
				_CreateInfo.m_Edges.AddEdge(nV+j+1, nV+j+2);
				_CreateInfo.m_Edges.AddEdge(nV+j+2, nV);
			}
		}
		nV += Face.m_nVertices;
	}
}


uint16 CXR_Model_BSP4Glass::GetInstanceFromHash(uint32 _NameHash)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass::GetInstanceFromHash, XR_BSP4GLASS);

	TAP_RCD<CBSP4Glass_Index> lIndices = m_lGlassIndices;
	uint16 nIndices = uint16(lIndices.Len());
	for (uint16 i = 0; i < nIndices; i++)
	{
		if (lIndices[i].m_NameHash == _NameHash)
			return lIndices[i].m_iInstance;
	}

	return GLASS_MAX_INSTANCE;
}


bool CXR_Model_BSP4Glass::Wallmark_Create(CXR_Model_BSP4Glass_Instance* _pGlass, const CMat4Dfp32& _GlassMat, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material)
{
	if(!_pGlass || (_pGlass && (_pGlass->m_lModels.Len() <= _WM.m_iNode)))
		return false;

	CGlassModel& GlassModel = _pGlass->m_lModels[_WM.m_iNode];

	if(!GlassModel.m_Attrib.Attrib_Unbreakable())
		return false;

	CBSP4Glass_WallmarkContext* pWMC = _pGlass->m_spWMC;
	if(!pWMC)
		return false;

	const CVec3Dfp32& N = _Origin.GetRow(2);
	const CVec3Dfp32& O = _Origin.GetRow(3);

	// Get surface
	CXW_Surface *pSurf = pWMC->m_pSC->GetSurface(_WM.m_SurfaceID)->GetSurface(0, 0);
	if(_Flags & XR_WALLMARK_SCANFOROVERLAP && pSurf->GetNumSequences() > 1)
	{
	//	CBSP2_Wallmark* pWM = _pWMC->m_spqWM->GetTail();
	//
	//	while(pWM)
	//	{
	//		if((pWM->m_Flags & XR_WALLMARK_PRIMARY) && !(pWM->m_Flags & XR_WALLMARK_OVERLAPPING) && (O - pWM->m_Pos).Length() < _WM.m_Size / 3)
	//		{0
	//			_Flags |= XR_WALLMARK_OVERLAPPING;
	//			break;
	//		}
	//		pWM = _pWMC->m_spqWM->GetNext(pWM);
	//	}
	}
	_Flags |= XR_WALLMARK_PRIMARY;

	CXW_Surface* pSurface = m_lspSurfaces[GlassModel.m_iSurface];
	if(pSurface->m_Flags & XW_SURFFLAGS_NOWALLMARK)
		return false;

	if(_Material != 0 && pSurface->GetBaseFrame()->m_MaterialType != _Material)
		return false;

    CPlane3Dfp32 Plane0 = GlassModel.m_Attrib.m_Plane;
	CPlane3Dfp32 Plane1(-Plane0.n, Plane0.d);

	Plane0.Transform(_GlassMat);
	Plane1.Transform(_GlassMat);

	fp32 NDot0 = N * Plane0.n;
	fp32 NDot1 = N * Plane1.n;
	if(!(_Flags & XR_WALLMARK_ALLOW_PERPENDICULAR))
	{
		if(NDot0 < GLASS_WM_PERPENDICULAR_TREHOLD && NDot1 < GLASS_WM_PERPENDICULAR_TREHOLD)
			return false;
	}
	else
	{
		if(Abs(NDot0) < 0.0001f)
			NDot0 = 0.0001f;
		if(Abs(NDot1) < 0.0001f)
			NDot1 = 0.0001f;
	}
	
	const fp32 Dist0 = Plane0.Distance(O);
	const fp32 Dist1 = Plane1.Distance(O);
	if(Abs(Dist0) > _Tolerance && Abs(Dist1) > _Tolerance)
		return false;

	bool bRes = Wallmark_CreateOnModel(pWMC, _GlassMat, _WM, _Origin, _Tolerance, _Flags, GlassModel);
	if(bRes)
		_Flags &= ~XR_WALLMARK_PRIMARY;

	return bRes;
}

/*
bool CXR_Model_BSP4Glass::Wallmark_Create(CXR_AnimState* _pAnimState, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material)
{
	MAUTOSTRIP(CXR_Model_BSP4Glass::Wallmark_Create, MAUTOSTRIP_VOID);
	GLASS_MSCOPE(CXR_Model_BSP4Glass::Wallmark_Create, XR_BSP4GLASS);

	if(!_pAnimState)
		return false;

    CXR_Model_BSP4Glass_Instance* pGlass = safe_cast<CXR_Model_BSP4Glass_Instance>(_pAnimState->m_pModelInstance);
	return Wallmark_Create(pGlass, _WM, _Origin, _Tolerance, _Flags, _Material);
}
*/


bool CXR_Model_BSP4Glass::Wallmark_CreateOnModel(CBSP4Glass_WallmarkContext* _pWMC, const CMat4Dfp32& _GlassMat, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, CGlassModel& _GlassModel)
{
	MAUTOSTRIP(CXR_Model_BSP4Glass::Wallmark_CreateOnModel, MAUTOSTRIP_VOID);
	GLASS_MSCOPE(CXR_Model_BSP4Glass::Wallmark_CreateOnModel, XR_BSP4GLASS);

	if(_GlassModel.m_lV.Len() < 3)
		return false;

	const CVec3Dfp32& U = _Origin.GetRow(0);
	const CVec3Dfp32& V = _Origin.GetRow(1);
	const CVec3Dfp32& N = _Origin.GetRow(2);
	const CVec3Dfp32& O = _Origin.GetRow(3);

	// Which one should we use?? Front or back side?!?!
	CPlane3Dfp32 Plane = _GlassModel.m_Attrib.m_Plane;
	//Plane.Transform(_GlassMat);
    fp32 NDot = N * Plane.n;
	fp32 Dist = Plane.Distance(O);

	CPlane3Dfp32 lPlanes[18];
	CVec3Dfp32 lVerts[32];
	CVec2Dfp32 lTVerts[32];
	CVec2Dfp32 lTVerts2[32];
	CPixel32 lColor[32];

	CVec3Dfp32* pV = _GlassModel.m_lV.GetBasePtr();
	CVec3Dfp32 lV[4];
	lV[0] = pV[0] * _GlassMat;
	lV[1] = pV[1] * _GlassMat;
	lV[2] = pV[2] * _GlassMat;
	lV[3] = pV[3] * _GlassMat;

	Plane.Create(lV[0], lV[2], lV[1]);
	NDot = N * Plane.n;
	Dist = Plane.Distance(O);

	lPlanes[0].Create(lV[0], lV[3], lV[3] + Plane.n);
	lPlanes[1].Create(lV[1], lV[0], lV[0] + Plane.n);
	lPlanes[2].Create(lV[2], lV[1], lV[1] + Plane.n);
	lPlanes[3].Create(lV[3], lV[2], lV[2] + Plane.n);
	lPlanes[4].CreateNV( N, O + (N * ( _Tolerance)));
	lPlanes[5].CreateNV(-N, O + (N * (-_Tolerance)));

	CVec3Dfp32 OProj, UProj, VProj;
	O.Combine(N, -Dist / NDot, OProj);
	OProj += Plane.n * GLASS_WM_DISTANCE;
	U.Combine(N, -(U*Plane.n) / NDot, UProj);
	V.Combine(N, -(V*Plane.n) / NDot, VProj);

	const fp32 Size = _WM.m_Size;
	lVerts[0] = OProj;
	OProj.Combine(UProj, -Size * 0.5f, lVerts[0]);
	lVerts[0].Combine(VProj, -Size * 0.5f,	lVerts[0]);
	lVerts[0].Combine(UProj,  Size,	lVerts[1]);
	lVerts[1].Combine(VProj,  Size,	lVerts[2]);
	lVerts[2].Combine(UProj, -Size,	lVerts[3]);

	// Setup default texture coordinates
	lTVerts[0] = CVec2Dfp32(1,0);
	lTVerts[1] = CVec2Dfp32(0,0);
	lTVerts[2] = CVec2Dfp32(0,1);
	lTVerts[3] = CVec2Dfp32(1,1);

	int nV = 0;
	nV = CutFence(lVerts, 4, lPlanes, 6, true, lTVerts);
	if (nV <= 2)
		return false;

	const int iV = _pWMC->AddVertices(nV);
	const fp32 Rcp = 1.0f / 128.0f;
	for(int i = 0; i < nV; i++)
	{
		lTVerts2[i] = CVec2Dfp32(lVerts[i].k[0] * Rcp, lVerts[i].k[1] * Rcp);
		lColor[i] = 0xffffffff;
	}

	// Copy memory
	{
        memcpy(&_pWMC->m_lV[iV], lVerts, sizeof(CVec3Dfp32)*nV);
		memcpy(&_pWMC->m_lTV[iV], lTVerts, sizeof(CVec2Dfp32)*nV);
		memcpy(&_pWMC->m_lTV2[iV], lTVerts2, sizeof(CVec2Dfp32)*nV);
		memcpy(&_pWMC->m_lCol[iV], lColor, sizeof(CPixel32)*nV);
		
		for(int i = 0; i < nV; i++)
			_pWMC->m_lN[iV + i] = Plane.n;
	}

	// Create decal tangent space -> wall tangentspace transform
	{
		const CXR_PlaneMapping& Mapping = *_GlassModel.m_pMapping;
		
		CVec2Dfp32 RotU;
		CVec2Dfp32 RotV;
		RotU[0] = V * Mapping.m_U;
		RotU[1] = V * Mapping.m_V;
		RotV[0] = U * Mapping.m_U;
		RotV[1] = U * Mapping.m_V;
		RotU.Normalize();
 		RotV.Normalize();
		if ((U / V) * N > 0.0f)
			Swap(RotU[0], RotV[1]);

		for(int i = 0; i < nV; i++)
		{
			_pWMC->m_lTangU[iV + i] = CVec3Dfp32(RotU[0], RotU[1], 0);
			_pWMC->m_lTangV[iV + i] = CVec3Dfp32(RotV[0], RotV[1], 0);
		}
	}

	CBSP4Glass_Wallmark WM;
	WM = _WM;
	WM.m_iV = iV;
	WM.m_nV = nV;
	WM.m_Pos = O;
	WM.m_Flags = _Flags;
	WM.m_iNode;

	const int iWM = _pWMC->AddWallmark(WM);
	_pWMC->m_spLink->Insert(iWM, WM.m_iNode+1);

	return true;
}


bool CXR_Model_BSP4Glass::Wallmark_Destroy(CXR_AnimState* _pAnimState, const int _GUID)
{
	MAUTOSTRIP(CXR_Model_BSP4Glass::Wallmark_Destroy, MAUTOSTRIP_VOID);
	GLASS_MSCOPE(CXR_Model_BSP4Glass::Wallmark_Destroy, XR_BSP4GLASS);

	if(!_pAnimState)
		return false;

    CXR_Model_BSP4Glass_Instance* pGlass = safe_cast<CXR_Model_BSP4Glass_Instance>(_pAnimState->m_pModelInstance);
	CBSP4Glass_WallmarkContext* pWMC = (pGlass) ? pGlass->m_spWMC : NULL;
	if(!pGlass || !pWMC)
		return false;

	// Walk through whole list, _GUID is a connection to an instance!
	bool bRes = false;
	CBSP4Glass_Wallmark* pWM = pWMC->m_spQWM->GetTail();
	while(pWM)
	{
		if(pWM->m_iNode == _GUID)
		{
			pWM->m_Flags |= XR_WALLMARK_DESTROYED;
			bRes = true;
		}

		pWM = pWMC->m_spQWM->GetNext(pWM);
	}

	return bRes;
}


uint8* CXR_Model_BSP4Glass::Wallmark_AllocVBMem(CBSP4Glass_RenderParam* _pRenderParam, CBSP4Glass_WallmarkContext* _pWMC, const uint16* _piWM, const uint32 _nWM)
{
	MAUTOSTRIP(CXR_Model_BSP4Glass::Wallmark_AllocVBMem, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP4Glass::Wallmark_AllocVBMem, XR_BSP4GLASS);

	uint32 TotVBMemSize = 0;
	uint32 SizeOfVBAttribChain = TUtilAlign(sizeof(CXR_VertexBuffer)) + TUtilAlign(sizeof(CRC_Attributes)) + TUtilAlign(sizeof(CXR_VBChain));

	CXR_Model_BSP4Glass_Instance* pGlass = _pRenderParam->GetGlassInstance();
	CGlassModel* pGlassModels = pGlass->m_lModels.GetBasePtr();

	if(!_piWM)
	{
		CBSP4Glass_Wallmark* pWM = _pWMC->m_spQWM->GetTail();
		while(pWM)
		{
			if(!(pWM->m_Flags & XR_WALLMARK_DESTROYED) && !pGlassModels[pWM->m_iNode].m_Attrib.Attrib_Inactive())
				TotVBMemSize += SizeOfVBAttribChain + TUtilAlign(sizeof(uint16) * (pWM->m_nV + 2));

			pWM = _pWMC->m_spQWM->GetNext(pWM);
		}
	}
	else
	{
		for(uint32 i = 0; i < _nWM; i++)
		{
			const CBSP4Glass_Wallmark* pWM = &_pWMC->m_spQWM->GetElem(_piWM[i]);
			if(!(pWM->m_Flags & XR_WALLMARK_DESTROYED) && !pGlassModels[pWM->m_iNode].m_Attrib.Attrib_Inactive())
				TotVBMemSize += SizeOfVBAttribChain + TUtilAlign(sizeof(uint16) * (pWM->m_nV + 2));
		}
	}

	// Allocate and clear needed data
	if(TotVBMemSize)
	{
		TotVBMemSize += TUtilAlign(sizeof(CMat4Dfp32));
		M_ASSERT(TUtilAlign(TotVBMemSize) == TotVBMemSize, "CXR_Model_BSP4Glass::Wallmark_AllocVBMem: Missalignment!");
		uint8* pVBMem = (uint8*)_pRenderParam->m_pVBM->Alloc(TotVBMemSize);
		FillChar(pVBMem, TotVBMemSize, 0x0);

		// Set VBM2VMatrix
		_pRenderParam->m_pVBMatM2V = TUtilGetAlign<CMat4Dfp32>(pVBMem);
		*_pRenderParam->m_pVBMatM2V = _pRenderParam->m_W2V;

		return pVBMem;
	}

	return NULL;
}


void CXR_Model_BSP4Glass::Wallmark_Render(CBSP4Glass_RenderParam* _pRenderParams, CBSP4Glass_WallmarkContext* _pWMC, uint16* _piID, uint32 _nID)
{
	MAUTOSTRIP(CXR_Model_BSP4Glass::Wallmark_Render, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP4Glass::Wallmark_Render, XR_BSP4GLASS);

	if(!_pWMC || !_pWMC->m_spQWM || !_pRenderParams || !_pRenderParams->m_pEngine->GetVar(XR_ENGINE_WALLMARKS))
		return;

	if(_piID && _nID == 0)
		return;

	CMTime RenderTime = _pRenderParams->m_pEngine->GetEngineTime();

	if (!_piID)
	{
		uint8* pVBMem = Wallmark_AllocVBMem(_pRenderParams, _pWMC, NULL, 0);
		uint8* pCurVBMemPtr = pVBMem;

		CBSP4Glass_Wallmark* pWM = _pWMC->m_spQWM->GetTail();
		while(pWM)
		{
			pCurVBMemPtr = Wallmark_RenderWM(pCurVBMemPtr, _pRenderParams, _pWMC, pWM, RenderTime);
			pWM = _pWMC->m_spQWM->GetNext(pWM);
		}
	}
	else
	{
		uint16 piWM[256];
		int nWM = 0;

		for(int i = 0; i < _nID; i++)
			nWM += _pWMC->m_spLink->EnumIDs(_piID[i], piWM + nWM, 256 - nWM, NULL, 0);

		if(nWM > 0)
		{
			uint8* pVBMem = Wallmark_AllocVBMem(_pRenderParams, _pWMC, piWM, nWM);
			uint8* pCurVBMemPtr = pVBMem;

			for(int i = 0; i < nWM; i++)
				pCurVBMemPtr = Wallmark_RenderWM(pCurVBMemPtr, _pRenderParams, _pWMC, &_pWMC->m_spQWM->GetElem(piWM[i]), RenderTime);
		}
	}
}


uint8* CXR_Model_BSP4Glass::Wallmark_RenderWM(uint8* _pVBMem, CBSP4Glass_RenderParam* _pRenderParam, CBSP4Glass_WallmarkContext* _pWMC, const CBSP4Glass_Wallmark* _pWM, const CMTime& _Time)
{
	MAUTOSTRIP(CXR_Model_BSP4Glass::Wallmark_RenderWM, MAUTOSTRIP_VOID);
	GLASS_MSCOPE(CXR_Model_BSP4Glass::Wallmark_Render, XR_BSP4GLASS);

	CXR_Model_BSP4Glass_Instance* pGlass = _pRenderParam->GetGlassInstance();
	CGlassModel* pGlassModels = pGlass->m_lModels.GetBasePtr();

	if(!_pVBMem || _pWM->m_Flags & XR_WALLMARK_DESTROYED || pGlassModels[_pWM->m_iNode].m_Attrib.Attrib_Inactive())
		return _pVBMem;

	CXW_Surface* pSurf = _pWMC->m_pSC->GetSurface(_pWM->m_SurfaceID)->GetSurface(_pRenderParam->m_pEngine->m_SurfOptions, _pRenderParam->m_pEngine->m_SurfCaps);
	const int nV = _pWM->m_nV;
	
	CXR_VertexBuffer* pVB = TUtilGetAlign<CXR_VertexBuffer>(_pVBMem);
	pVB->m_Color = 0xffffffff;
	pVB->m_pAttrib = TUtilGetAlign<CRC_Attributes>(_pVBMem);
	pVB->m_pAttrib->SetDefault();
	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
	
	const int nPrim = nV + 2;
	uint16* pPrim = TUtilGetAlign<uint16>(_pVBMem, nPrim);

	int iP = 0;
	pPrim[iP++] = CRC_RIP_TRIFAN + ((nV + 2) << 8);
	pPrim[iP++] = nV;
	for(int i = 0; i < nV; i++)
		pPrim[iP++] = nV - 1 - i;

	pVB->m_pVBChain = TUtilGetAlign<CXR_VBChain>(_pVBMem);
	((CXR_VBChain*)pVB->m_pVBChain)->m_TaskId = 0xffff;
	pVB->m_Flags |= CXR_VBFLAGS_VBCHAIN;
	pVB->Geometry_VertexArray(&_pWMC->m_lV[_pWM->m_iV], _pWM->m_nV, true);
	pVB->Geometry_Normal(&_pWMC->m_lN[_pWM->m_iV]);
	pVB->Geometry_TVertexArray(&_pWMC->m_lTV[_pWM->m_iV], 0);
	pVB->Geometry_TVertexArray(&_pWMC->m_lTV2[_pWM->m_iV], 1);
	pVB->Geometry_TVertexArray(&_pWMC->m_lTangU[_pWM->m_iV], 2);
	pVB->Geometry_TVertexArray(&_pWMC->m_lTangV[_pWM->m_iV], 3);
	pVB->Geometry_ColorArray(&_pWMC->m_lCol[_pWM->m_iV]);
    pVB->Render_IndexedPrimitives(pPrim, nPrim);
	pVB->m_Priority = CXR_VBPRIORITY_WALLMARK;

	CXW_SurfaceKeyFrame* pSurfKey;
	CMTime Time;
	if(_pWM->m_Flags & XR_WALLMARK_NOANIMATE)
		Time = _pWM->m_SpawnTime;
	else
		Time = _Time - _pWM->m_SpawnTime;

	if(_pWM->m_Flags & XR_WALLMARK_OVERLAPPING)
		pSurfKey = pSurf->GetFrame(1, Time);
	else
		pSurfKey = pSurf->GetFrame(0, Time);

	CXR_RenderSurfExtParam Params;
	for(int i = 0; i < XR_WALLMARK_MAXTEXTUREPARAMS; i++)
		Params.m_TextureID[i] = _pWM->m_SurfaceParam_TextureID[i];

	uint8 DummyIndex = ((mint(_pWM)>>4) & 255);
	fp32 Prio = CXR_VBPRIORITY_MODEL_OPAQUE + (DummyIndex* 0.0001f);

	if( pSurfKey->m_lTextures.Len() >= 3 )
	{
		pVB->Matrix_Set(_pRenderParam->m_pVBMatM2V);
		CXR_SurfaceShaderParams SSP;
//		CXR_ShaderParams SParams;
		CXR_Shader* pShader = _pRenderParam->m_pEngine->GetShader();
		SSP.Create(pSurf, pSurfKey);
//		SParams.Create(&SSP, NULL, NULL, pShader);
		const CXR_SurfaceShaderParams* pSSP = &SSP;
		CXR_ShaderParams Params;
		Params.Clear();
		pShader->RenderDeferredArray(1, pVB, &Params, &pSSP, CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE,CRC_FLAGS_ZWRITE | CRC_FLAGS_STENCIL, Prio - 2059.9f, 1.0f, 1);
	}
	else
	{
		int Flags = RENDERSURFACE_MATRIXSTATIC_M2V;
		CXR_Util::Render_Surface(Flags, Time, pSurf, pSurfKey, _pRenderParam->m_pEngine, _pRenderParam->m_pVBM, (CMat4Dfp32*)NULL, (CMat4Dfp32*)NULL, _pRenderParam->m_pVBMatM2V, pVB, 
			Prio, 0.0001f, &Params);
	}

	return _pVBMem;
}


/*
int CXR_Model_BSP4Glass::GlassCutFence(CVec3Dfp32* _pVerts, int _nv, const CPlane3Dfp32* _pPlanes, int _np, bool _bInvertPlanes, CVec2Dfp32* _pTVerts1, CVec2Dfp32* _pTVerts2, CVec2Dfp32* _pTVerts3)
{
	if (!_nv) return 0;
	const int MaxVClip = 32;

	int bClipTVerts1 = (_pTVerts1 != NULL);
	int bClipTVerts2 = (_pTVerts2 != NULL);
	int bClipTVerts3 = (_pTVerts3 != NULL);

	CVec3Dfp32 VClip[MaxVClip];
	CVec2Dfp32 TVClip1[MaxVClip];
	CVec2Dfp32 TVClip2[MaxVClip];
	CVec2Dfp32 TVClip3[MaxVClip];

	CVec3Dfp32* pVDest = &VClip[0];
	CVec2Dfp32* pTVDest1 = &TVClip1[0];
	CVec2Dfp32* pTVDest2 = &TVClip2[0];
	CVec2Dfp32* pTVDest3 = &TVClip3[0];
	CVec3Dfp32* pVSrc = _pVerts;
	CVec2Dfp32* pTVSrc1 = _pTVerts1;
	CVec2Dfp32* pTVSrc2 = _pTVerts2;
	CVec2Dfp32* pTVSrc3 = _pTVerts3;

	for(int iPlane = 0; iPlane < _np; iPlane++)
	{
		const CPlane3Dfp32* pP = &_pPlanes[iPlane];
		fp32 VertPDist[32];
		//		fp32 d = -((pP->p.k[0] * pP->n.k[0]) + (pP->p.k[1] * pP->n.k[1]) + (pP->p.k[2] * pP->n.k[2]));
		//		fp32 d = pP->d;
		bool bBehind = false;
		bool bFront = false;

		// Calc point-2-plane distance for all vertices.
		for(int v = 0; v < _nv; v++)
		{
			VertPDist[v] = pP->Distance(pVSrc[v]);
			if (_bInvertPlanes) VertPDist[v] = -VertPDist[v];
			if (VertPDist[v] < 0.0f) bBehind = true; else bFront = true;
		}

		// If all points are on one side, return either all or none.
		if (!(bFront && bBehind))
		{
			if (bFront) continue;
			return 0;
		}

		int nClip = 0;
		{
			int v = _nv-1;
			for (int v2 = 0; v2 < _nv; v2++)
			{
				if (VertPDist[v] > -GLASS_CUTFENCE_EPSILON)
				{
					pVDest[nClip] = pVSrc[v];
					if (bClipTVerts1) pTVDest1[nClip] = pTVSrc1[v];
					if (bClipTVerts2) pTVDest2[nClip] = pTVSrc2[v];
					if (bClipTVerts3) pTVDest3[nClip] = pTVSrc3[v];
					nClip++;

					if ((VertPDist[v2] < -GLASS_CUTFENCE_EPSILON) && (VertPDist[v] > GLASS_CUTFENCE_EPSILON))
					{
						//					BSPModel_GetIntersectionPoint(_pi.n, d, pVSrc[v], pVSrc[v2], pVDest[nClip++]);

						fp32 dvx = (pVSrc[v2].k[0] - pVSrc[v].k[0]);
						fp32 dvy = (pVSrc[v2].k[1] - pVSrc[v].k[1]);
						fp32 dvz = (pVSrc[v2].k[2] - pVSrc[v].k[2]);
						fp32 s = dvx*pP->n.k[0] + dvy*pP->n.k[1] + dvz*pP->n.k[2];
						if (_bInvertPlanes) s = -s;
						if (s)
						{
							fp32 sp = VertPDist[v];
							fp32 t = -sp/s;
							pVDest[nClip].k[0] = pVSrc[v].k[0] + dvx * t;
							pVDest[nClip].k[1] = pVSrc[v].k[1] + dvy * t;
							pVDest[nClip].k[2] = pVSrc[v].k[2] + dvz * t;

							if (bClipTVerts1)
							{
								fp32 dtvx = (pTVSrc1[v2].k[0] - pTVSrc1[v].k[0]);
								fp32 dtvy = (pTVSrc1[v2].k[1] - pTVSrc1[v].k[1]);
								pTVDest1[nClip].k[0] = pTVSrc1[v].k[0] + dtvx * t;
								pTVDest1[nClip].k[1] = pTVSrc1[v].k[1] + dtvy * t;
							}
							if (bClipTVerts2)
							{
								fp32 dtvx = (pTVSrc2[v2].k[0] - pTVSrc2[v].k[0]);
								fp32 dtvy = (pTVSrc2[v2].k[1] - pTVSrc2[v].k[1]);
								pTVDest2[nClip].k[0] = pTVSrc2[v].k[0] + dtvx * t;
								pTVDest2[nClip].k[1] = pTVSrc2[v].k[1] + dtvy * t;
							}
							if (bClipTVerts3)
							{
								fp32 dtvx = (pTVSrc3[v2].k[0] - pTVSrc3[v].k[0]);
								fp32 dtvy = (pTVSrc3[v2].k[1] - pTVSrc3[v].k[1]);
								pTVDest3[nClip].k[0] = pTVSrc3[v].k[0] + dtvx * t;
								pTVDest3[nClip].k[1] = pTVSrc3[v].k[1] + dtvy * t;
							}
							nClip++;
						}
					}
				}
				else
				{
					if (VertPDist[v2] > GLASS_CUTFENCE_EPSILON)
					{
						//					BSPModel_GetIntersectionPoint(_pi.n, d, pVSrc[v], pVSrc[v2], pVDest[nClip++]);

						fp32 dvx = (pVSrc[v2].k[0] - pVSrc[v].k[0]);
						fp32 dvy = (pVSrc[v2].k[1] - pVSrc[v].k[1]);
						fp32 dvz = (pVSrc[v2].k[2] - pVSrc[v].k[2]);
						fp32 s = dvx*pP->n.k[0] + dvy*pP->n.k[1] + dvz*pP->n.k[2];
						if (_bInvertPlanes) s = -s;
						if (s)
						{
							fp32 sp = VertPDist[v];
							fp32 t = -sp/s;
							pVDest[nClip].k[0] = pVSrc[v].k[0] + dvx * t;
							pVDest[nClip].k[1] = pVSrc[v].k[1] + dvy * t;
							pVDest[nClip].k[2] = pVSrc[v].k[2] + dvz * t;

							if (bClipTVerts1)
							{
								fp32 dtvx = (pTVSrc1[v2].k[0] - pTVSrc1[v].k[0]);
								fp32 dtvy = (pTVSrc1[v2].k[1] - pTVSrc1[v].k[1]);
								pTVDest1[nClip].k[0] = pTVSrc1[v].k[0] + dtvx * t;
								pTVDest1[nClip].k[1] = pTVSrc1[v].k[1] + dtvy * t;
							}
							if (bClipTVerts2)
							{
								fp32 dtvx = (pTVSrc2[v2].k[0] - pTVSrc2[v].k[0]);
								fp32 dtvy = (pTVSrc2[v2].k[1] - pTVSrc2[v].k[1]);
								pTVDest2[nClip].k[0] = pTVSrc2[v].k[0] + dtvx * t;
								pTVDest2[nClip].k[1] = pTVSrc2[v].k[1] + dtvy * t;
							}
							if (bClipTVerts3)
							{
								fp32 dtvx = (pTVSrc3[v2].k[0] - pTVSrc3[v].k[0]);
								fp32 dtvy = (pTVSrc3[v2].k[1] - pTVSrc3[v].k[1]);
								pTVDest3[nClip].k[0] = pTVSrc3[v].k[0] + dtvx * t;
								pTVDest3[nClip].k[1] = pTVSrc3[v].k[1] + dtvy * t;
							}
							nClip++;
						}
					}
				}

				if (nClip > MaxVClip-1) Error_static("CutFace", "Too many vertices.");
				v = v2;
			}
		}

		if (nClip < 3) return 0;
		_nv = nClip;

		Swap(pVSrc, pVDest);
		Swap(pTVSrc1, pTVDest1);
		Swap(pTVSrc2, pTVDest2);
		Swap(pTVSrc3, pTVDest3);
	}

	// Move if the latest vertices are in the wrong array.
	if (pVSrc != _pVerts) 
	{
		memcpy(_pVerts, pVSrc, _nv*sizeof(CVec3Dfp32));
		if (bClipTVerts1) memcpy(_pTVerts1, pTVSrc1, _nv*sizeof(CVec2Dfp32));
		if (bClipTVerts2) memcpy(_pTVerts2, pTVSrc2, _nv*sizeof(CVec2Dfp32));
		if (bClipTVerts3) memcpy(_pTVerts3, pTVSrc3, _nv*sizeof(CVec2Dfp32));
	}
	return _nv;
}
*/
