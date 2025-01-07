#include "PCH.h"

#include "WBSPModel.h"
#include "WBSPDef.h"

void TransformAABB(const CMat4Dfp32& _Mat, const CBox3Dfp32& _Box, CBox3Dfp32& _DestBox);
int BoxInClipVolume(const CBox3Dfp32& _Box, const CRC_ClipVolume& _Volume);

//#define SPLINE_DEBUG

#define CSB_MAX_TESS 32

void CXR_Model_BSP::RenderSplineBrushFace_VB(CSplineBrush* _pBrush, int _iFace, int _iPL)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderSplineBrushFace_VB, MAUTOSTRIP_VOID);
	CSplineBrush_Face* pF = &_pBrush->m_lFaces[_iFace];
	VB_AddPrimitives(pF->m_iVB, &m_lSBPrim[pF->m_iPrim], pF->m_nPrim);
}

void CXR_Model_BSP::RenderSplineBrushFace(CSplineBrush* _pBrush, int _iFace, bool _bUseCompiled, int _iPL)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderSplineBrushFace, MAUTOSTRIP_VOID);
	if (!m_lVBQueues.Len()) return;

	MIncProfile(ms_nSurfSwitch);
	CSplineBrush_Face* pF = &_pBrush->m_lFaces[_iFace];

	const int MaxVerts = CSB_MAX_TESS*CSB_MAX_TESS;
	const int MaxPrim = 1024;
	CVec2Dfp32 STArray[MaxVerts];
	CVec2Dfp32* pSTArray = NULL;

	CXR_VertexBuffer* pVB = ms_pVBM->Alloc_VB();
	if (!pVB) return;

//ConOut(CStrF("pBrush %.8x, iFace %d", _pBrush, _iFace));
	// --------------
	int bLightMap = (pF->m_Flags & XW_FACE_LIGHTMAP);

	int WireColor;
	int nV = 0;
	int nS, nT;
	nS = nT = 0;

	int bNHF = false;
	int NHFMediumFlags = 0;
#ifdef PLATFORM_PS2
	int bHWDynLight = 0;
#else
	int bHWDynLight = 
		(ms_pCurrentEngine->m_RenderCaps_nMultiTexture >= 2) ||
		(ms_CurrentRCCaps & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20);
#endif
	int bHWNHF = bHWDynLight;
	const CBSP_PortalLeafExt* pNHFPL = NULL;

	if (ms_pSceneFog && ms_pSceneFog->NHFEnable())
	{
		pNHFPL = (_iPL) ? &m_lPortalLeaves[_iPL] : NULL;
		NHFMediumFlags = (pNHFPL) ? (pNHFPL->m_ContainsMedium) : ms_RenderInfo.m_MediumFlags;
		if (NHFMediumFlags & XW_MEDIUM_FOG)
		{
			bNHF = true;
			if (!bHWNHF || (NHFMediumFlags & XW_MEDIUM_CLIPFOG))
			{
				_bUseCompiled = false;
				bHWNHF = false;
			}
		}
	}

//	if (bNHF)
//		_bUseCompiled = false;

	if (pF->m_DynLightMask && !bHWDynLight)
		_bUseCompiled = false;

	CXW_Surface* pSurf = m_lspSurfaces[pF->m_iSurface]->GetSurface(ms_SurfOptions, ms_SurfCaps);

	if (_bUseCompiled)
	{
		CBSP_VBChain Chain;
		VB_CopyGeometry(pF->m_iVB, &Chain);
		Chain.SetToVB(pVB);		
		pVB->Render_IndexedPrimitives(&m_lSBPrim[pF->m_iPrim], pF->m_nPrim);
	}
	else
	{
		pSTArray = STArray;
		CVec3Dfp32* pVArray = _pBrush->Tesselate(_iFace, MaxVerts, NULL, pSTArray, nS, nT, ms_pVBM);
		nV = nS*nT;
		if (!nV || !pVArray)
		{
			ConOut("(CXR_Model_BSP::RenderSplineBrushFace) No vertices in VB.");
			return;
		}
		if (!pVB->AllocVBChain(ms_pVBM, false))
			return;

		pVB->GetVBChain()->m_nV = nV;
		pVB->GetVBChain()->m_pV = pVArray;

		CVec2Dfp32* pLMUVArray = NULL;
		CVec2Dfp32* pUVArray = ms_pVBM->Alloc_V2(nV);
		if (!pUVArray) return;
	
		if (bLightMap)
		{
			pLMUVArray = ms_pVBM->Alloc_V2(nV);
			if (!pLMUVArray) return;
			pF->EvalUVLM(nV, pVArray, pSTArray, pUVArray, pLMUVArray);
			pVB->Geometry_TVertexArray(pUVArray, 0);
			pVB->Geometry_TVertexArray(pLMUVArray, 1);
		}
		else
		{
			pF->EvalUV(nV, pVArray, pSTArray, pUVArray);
			pVB->Geometry_TVertexArray(pUVArray, 0);
		}

		
		// Build primitive-batch
		int nPrim = (nS-1)*(nT*2+2);// + 2;
		uint16* pPrim = ms_pVBM->Alloc_Int16(nPrim);
		if (!pPrim) return;

		int iP = 0;
		{
			for(int s = 0; s < nS-1; s++)
			{
				if (iP > MaxPrim-nT*2-8) break;
				pPrim[iP++] = CRC_RIP_TRISTRIP + ((nT*2+2) << 8);
				pPrim[iP++] = nT*2;
				for(int t = 0; t < nT; t++)
				{
					pPrim[iP++] = (s+1)*nT + t;
					pPrim[iP++] = s*nT + t;
				}
			}
//			pPrim[iP++] = CRC_RIP_END + (1 << 8);
		}

		if (iP > nPrim)
			Error("RenderSplineBrushFace", "Internal error.");

		pVB->Render_IndexedPrimitives(pPrim, iP);

		// Need normals?
		if (pSurf->m_Flags & XW_SURFFLAGS_NEEDNORMALS || pF->m_DynLightMask)
		{
			CVec3Dfp32* pN = ms_pVBM->Alloc_V3(nV);
			if (!pN) return;

			if (pF->m_DynLightMask && bHWDynLight)
			{
				// This occur at very rare occations. (NHF + DynamicLighting)

				CVec3Dfp32* pTangU = ms_pVBM->Alloc_V3(nV);
				CVec3Dfp32* pTangV = ms_pVBM->Alloc_V3(nV);
				if (!pTangU || !pTangV) return;

				_pBrush->EvalTangentSpaceArrays(_iFace, nV, pSTArray, pN, pTangU, pTangV);

				pVB->Geometry_TVertexArray(pTangU, 2);
				pVB->Geometry_TVertexArray(pTangV, 3);
			}
			else
				_pBrush->EvalNormal(_iFace, nV, pSTArray, pN, true);

			pVB->GetVBChain()->m_pN = pN;
		}
	}

	fp32 BasePriority = ms_BasePriority_Opaque;
	fp32 Offset = 1.0f;
	if (pF->m_DynLightMask || (pF->m_Flags & XW_FACE_TRANSPARENT))
	{
		CXR_VBManager* pVBM = ms_pVBM;

		CMTime AnimTime;
		CXW_SurfaceKeyFrame* pSurfKey;
		if (m_pCurAnimState) 
		{
			AnimTime = pSurf->m_iController ? m_pCurAnimState->m_AnimTime1 : m_pCurAnimState->m_AnimTime0;
			pSurfKey = pSurf->GetFrame(
				pSurf->m_iController ? m_pCurAnimState->m_Anim1 : m_pCurAnimState->m_Anim0, 
				AnimTime, &m_TmpSurfKeyFrame);
		}
		else
			pSurfKey = pSurf->GetBaseFrame();

		bool bAddLight = false;
//		int Priority = 0;

		CXR_WorldLightState* pWLS = m_spTempWLS;



		if (pWLS->m_nDynamic && pF->m_DynLightMask)
		{
#ifndef M_RTM
			if (pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT)
			{
				ConOut(CStrF("§cf80WARNING: Bad surface (Transparent and allowing dynamic lighting) %s", pSurf->m_Name.Str() ));
				pF->m_DynLightMask = 0;
			}
#endif

			for(int iLight = 0; iLight < pWLS->m_nDynamic; iLight++)
			{
				int CurMask = (1 << iLight);
				if (!(pF->m_DynLightMask & CurMask)) continue;

				bAddLight = true;

				CXR_Light* pL = &pWLS->m_lDynamic[iLight];

				CXR_VertexBuffer* pVBL = ms_pVBM->Alloc_VB();
				if (!pVBL) continue;

				pVBL->Matrix_Set(ms_pVBMatrixM2V);
				if (!ms_pVBM->Alloc_VBChainCopy(pVBL, pVB))
					continue;

#ifndef PLATFORM_PS2
				if (pL->m_Flags & CXR_LIGHT_HINT_ADDITIVE)
					pVBL->m_Priority = ms_BasePriority_Opaque + fp32(iLight) * 0.01f + CXR_VBPRIORITY_POSTDYNLIGHT;
				else
					pVBL->m_Priority = ms_BasePriority_Opaque + fp32(iLight) * 0.01f + CXR_VBPRIORITY_DYNLIGHT;

				if (bHWDynLight)
				{
					if (ms_pCurrentEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20)
					  Light_ProjectDynamic_NV20(pVBL, iLight, pL, pSurf);
					else
						Light_ProjectDynamic_MultiTexture(pVBL, iLight, pL, pSurf);
				}
				else
#endif
				{

					pVBL->m_pAttrib = pVBM->Alloc_Attrib();
					if (!pVBL->m_pAttrib) return;
					pVBL->m_pAttrib->SetDefault();
					if (ms_pCurrentEngine) 
						ms_pCurrentEngine->SetDefaultAttrib(pVBL->m_pAttrib);

	//					m_pCurrentRC->Attrib_Get(*pVBL->m_pAttrib);
					CPixel32* pCol = pVBM->Alloc_CPixel32(pVBL->GetVBChain()->m_nV);
					CVec2Dfp32* pUV = pVBM->Alloc_V2(pVBL->GetVBChain()->m_nV);

					M_ASSERT(pCol, "Out of memory!");
					M_ASSERT(pUV, "Out of memory!");

					pVBL->m_pAttrib->Attrib_TextureID(0, ms_TextureID_DynLightProj);
					pVBL->m_pAttrib->Attrib_Enable(CRC_FLAGS_ZWRITE | CRC_FLAGS_CULL);
					pVBL->Geometry_ColorArray(pCol);
					pVBL->Geometry_TVertexArray(pUV, 0);
					Light_ProjectDynamic(pL, pF->m_LightUVec, pF->m_LightVVec, _pBrush->m_lCullPlanes[pF->m_iLightingPlane], pVB->GetVBChain()->m_nV, pVB->GetVBChain()->m_pV, pUV, pCol);
					if (ms_pSceneFog) ms_pSceneFog->SetDepthFogBlack(pVBL->m_pAttrib);
					pVBL->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ADD);
					
#ifdef PLATFORM_PS2
					pVBL->m_Priority = ms_BasePriority_Opaque + CXR_VBPRIORITY_DYNLIGHT;
					pVBL->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);
					pVBL->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
					pVBL->m_pAttrib->Attrib_TexEnvMode( 0, CRC_PS2_TEXENVMODE_ADD | CRC_PS2_TEXENVMODE_MODULATE | CRC_PS2_TEXENVMODE_COLORBUFFER | CRC_PS2_TEXENVMODE_RGBMASK | CRC_PS2_TEXENVMODE );
					pVBL->m_pAttrib->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);
					pVBL->m_pAttrib->m_nChannels = 1;
#endif
					
				}

				pVBM->AddVB(pVBL);
			}
		}



		int Flags = /*RENDERSURFACE_MATRIXSTATIC_M2W |*/ RENDERSURFACE_MATRIXSTATIC_M2V;// | RENDERSURFACE_VERTEXFOG;
		if (ms_Enable & MODEL_BSP_ENABLE_FULLBRIGHT) Flags |= RENDERSURFACE_FULLBRIGHT;
		if (bAddLight) 
			Flags |= RENDERSURFACE_ADDLIGHT;

		if (pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT)
		{
			CVec3Dfp32 Center;
			pF->m_BoundBox.GetCenter(Center);
			fp32 Dist = M_Sqrt(m_pView->m_CurLocalVP.DistanceSqr(Center));
			BasePriority = ms_BasePriority_Transparent + (1.0f - Dist*m_pView->m_BackPlaneInv);
			Offset = 0.0001f;
		}

		CXR_RenderSurfExtParam Params;
		Params.m_TextureIDLightMap = (bLightMap && m_spLMTC != NULL) ? m_spLMTC->GetTextureID(m_lLightMapInfo[pF->m_iLightInfo].m_iLMC) : 0;


		CXR_Util::Render_Surface(Flags, AnimTime, pSurf, pSurfKey, ms_pCurrentEngine, pVBM, ms_pVBMatrixM2W, ms_pVBMatrixW2V, ms_pVBMatrixM2V, pVB, 
			BasePriority, Offset, &Params);

		WireColor = 0xff007f00;
	}
	else
	{
		m_lVBQueues[pF->m_iVBQueue].AddFromVB(pVB);
		WireColor = 0xff7f0000;
	}



	// NHF?
#ifndef MODEL_BSP_DISABLENHF
	if (bNHF)
	{
		if (bHWNHF && pNHFPL && !(NHFMediumFlags & XW_MEDIUM_CLIPFOG))
		{
			if (pVB->IsVBIDChain())
				VB_RenderNHF(pF->m_iVB, pVB->GetVBIDChain()->m_piPrim, pVB->GetVBIDChain()->m_nPrim, const_cast<CBSP_PortalLeafExt*>(pNHFPL));
			else
				VB_RenderNHF(pF->m_iVB, pVB->GetVBChain()->m_piPrim, pVB->GetVBChain()->m_nPrim, const_cast<CBSP_PortalLeafExt*>(pNHFPL));
		}
		else
		{
			CXR_VBManager* pVBM = ms_pVBM;
			CXR_VertexBuffer VB;
			VB.m_pAttrib = pVBM->Alloc_Attrib();
			if (!VB.m_pAttrib) return;
			VB.m_pAttrib->SetDefault();
			VB.Matrix_Set(ms_pVBMatrixM2V);
			if (ms_pCurrentEngine) ms_pCurrentEngine->SetDefaultAttrib(VB.m_pAttrib);
			CPixel32* pFogArray = pVBM->Alloc_CPixel32(nV);
			CVec2Dfp32* pFogUV = pVBM->Alloc_V2(nV);
			if (!pFogArray || !pFogUV) return;

			if (NHFMediumFlags & XW_MEDIUM_FOGADDITIVE)
				VB.m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
			else
				VB.m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

			VB.m_pAttrib->Attrib_TextureID(0, ms_pSceneFog->m_FogTableTextureID);
			VB.m_pAttrib->Attrib_TextureID(1, 0);
			VB.m_pAttrib->Attrib_Enable(CRC_FLAGS_CULL);
			VB.m_pAttrib->Attrib_ZCompare(CRC_COMPARE_EQUAL);
			VB.m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);

			if (pNHFPL)
			{
				// This is the world and the current portal-leaf was specified.

				if (pF->m_BoundBox.m_Max.DistanceSqr(pF->m_BoundBox.m_Min) < Sqr(64.0f*_SQRT3))
				{
					CPixel32 RenderFog[8];
					ms_pSceneFog->TraceBound(pF->m_BoundBox);
					ms_pSceneFog->SetTransform(NULL);
					ms_pSceneFog->TraceBox(pF->m_BoundBox, RenderFog);
					ms_pSceneFog->InterpolateBox(pF->m_BoundBox, RenderFog, nV, pVB->GetVBChain()->m_pV, pFogArray, pFogUV, NULL);
				}
				else
				{
					CVec3Dfp32 TraceVol[9];
					pF->m_BoundBox.GetVertices(TraceVol);
					TraceVol[8] = m_pView->m_CurLocalVP;

					// Do BSP-bounding.
					int iFirstNode = Structure_GetFirstSplitNode_Polyhedron(TraceVol, 9);
					Structure_EnableTreeFromNode_Polyhedron(iFirstNode, TraceVol, 9);
					Structure_InitBoundNodesFromEnabledNodes(iFirstNode, 0, 0);

					EnableWithPortalFloodFill(pNHFPL->m_iNode, XW_NODE_FOGENABLE, XW_MEDIUM_FOG);

					for(int v = 0; v < nV; v++)
	//					pFogArray[v] = 0x3f00ff00;
						pFogArray[v] = Fog_TracePortalLeaf(iFirstNode, _iPL, pVB->GetVBChain()->m_pV[v], m_pView->m_CurLocalVP, 255);

					// Remove BSP-bounding.

					DisableWithPortalFloodFill(pNHFPL->m_iNode, XW_NODE_FOGENABLE);
					DisableTree(iFirstNode);
					ms_pSceneFog->ConvertFogColors(nV, pFogArray, pFogUV);
				}
			}
			else
			{
				// This is most likely a BSP-entity and we must therefore use the fog-state properly.
				CBox3Dfp32 Box;
//				TransformAABB(ms_pSceneFog->m_Transform, pF->m_BoundBox, Box);
				TransformAABB(ms_pSceneFog->m_Transform, m_BoundBox, Box);
				ms_pSceneFog->TraceBound(Box);
				if (!ms_pSceneFog->Trace(Box, nV, pVB->GetVBChain()->m_pV, pFogArray, pFogUV))
					pFogArray = NULL;
			}

			ms_pSceneFog->TraceBoundRelease();

			if (pFogArray)
			{
				if (!VB.AllocVBChain(ms_pVBM, false))
					return;
				VB.Geometry_VertexArray(pVB->GetVBChain()->m_pV, pVB->GetVBChain()->m_nV, true);
				VB.Geometry_ColorArray(pFogArray);
				VB.Geometry_TVertexArray(pFogUV, 0);
				VB.Render_IndexedPrimitives(pVB->GetVBChain()->m_piPrim, pVB->GetVBChain()->m_nPrim);
#ifdef PLATFORM_PS2
				VB.m_Priority = BasePriority + CXR_VBPRIORITY_VOLUMETRICFOG;
#else
			VB.m_Priority = BasePriority + 10*Offset;
#endif
			if (ms_pSceneFog) ms_pSceneFog->SetDepthFogNone(VB.m_pAttrib);
			pVBM->AddVB(VB);
		}
	}
	}
#endif

/*
	if (ms_Enable & MODEL_BSP_ENABLE_SPLINEWIRE)
	{
		// -------------------------------------------------------------------
		//  Wire
		// -------------------------------------------------------------------
		ms_pCurrentRC->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		uint32 iVStrip[1024];
		int s;
		for(s = 0; s < nS-1; s++)
		{
			int nVStrip = 0;
			for(int t = 0; t < nT; t++)
			{
				iVStrip[nVStrip++] = (s+1)*nT + t;
				iVStrip[nVStrip++] = s*nT + t;
			}
			ms_pCurrentRC->Render_WireStrip(pVB->GetVBChain()->m_pV, iVStrip, nVStrip, (_bUseCompiled) ? 0xff00003f : WireColor);
		}
		for(s = 0; s < nS; s++)
		{
			int nVStrip = 0;
			for(int t = 0; t < nT; t++)
				iVStrip[nVStrip++] = s*nT + t;
			ms_pCurrentRC->Render_WireStrip(pVB->GetVBChain()->m_pV, iVStrip, nVStrip, 0xff3f3f7f);
		}
		ms_pCurrentRC->Attrib_RasterMode(CRC_RASTERMODE_NONE);
	}
*/
}


//#define SPLINE_VIEWINTERFACECULLING
//#define SPLINE_NOCULL

void CXR_Model_BSP::RenderSplineBrushes(bool _bNoCull)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderSplineBrushes, MAUTOSTRIP_VOID);
	if (m_spSBLink!=NULL && !_bNoCull)
	{
		int bNHF = (ms_RenderInfo.m_Flags & CXR_RENDERINFO_NHF);

		uint16 SBBuffer[1024];
		int nSB = m_spSBLink->EnumVisible(m_pView, SBBuffer, 1024, NULL, 0);
//ConOut(CStrF("(CXR_Model_BSP::RenderSplineBrushes) %d", nSB));
		for(int i = 0; i < nSB; i++)
		{
			int iSB = SBBuffer[i];
			CSplineBrush* pSB = m_lspSplineBrushes[iSB];
			if (pSB->m_LastViewRendered == m_CurrentView) continue;
			pSB->m_LastViewRendered = m_CurrentView;
#ifdef SPLINE_NOCULL
			bool bVis = true;
			pSB->SetAllFacesVisible();
#else
//ConOutL(CStrF("    ID %d", iSB));

//			RenderBox(m_pCurrentRC, pSB->m_BoundBox, 0xffffff80);
			uint16 PLBuffer[128];
			uint16 VisPLBuffer[128];
			int nPL = m_spSBLink->EnumPortalLeaves(iSB, PLBuffer, 128);
			int nVisPL = 0;

/*CStr s("      PL ");
for(int i = 0; i < nPL; i++)
	s += CStrF("%d, ", PLBuffer[i]);
ConOutL(s);*/

			for(int iiPL = 0; iiPL < nPL; iiPL++)
			{
				int iPL = PLBuffer[iiPL];
				int iRP = m_pView->m_liLeafRPortals[iPL];
				if (!iRP) continue;
				if (BoxInClipVolume(pSB->m_BoundVertexBox, m_pView->m_lRPortals[iRP]))
					VisPLBuffer[nVisPL++] = iPL;
			}
//ConOutL(CStrF("         VisPL %d", nVisPL));
			if (!nVisPL) continue;
//			RenderBox(m_pCurrentRC, pSB->m_BoundVertexBox, 0xff80ff80);

//			if (bLowTess) = pSB->
//bUseCompiled = false;
			bool bVis = false;
			for(int iFace = 0; iFace < m_lspSplineBrushes[iSB]->m_lFaces.Len(); iFace++)
			{
				CSplineBrush_Face* pF = &pSB->m_lFaces[iFace];
				pF->m_Flags &= ~XW_FACE_VISIBLE;
				CXW_Surface* pSurf = m_lspSurfaces[pF->m_iSurface];
				if (pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE) continue;

				int iiPL;
				for(iiPL = 0; iiPL < nVisPL; iiPL++)
				{
					int iPL = VisPLBuffer[iiPL];
					int iRP = m_pView->m_liLeafRPortals[iPL];
					if (!iRP) continue;
					if (BoxInClipVolume(pF->m_BoundBox, m_pView->m_lRPortals[iRP])) break;
				}
				if (iiPL == nVisPL) continue;

				if (!(pSurf->m_Flags & XW_SURFFLAGS_NOCULL))
				{
					if (pSB->Face_IsVisible(iFace, m_pView->m_CurLocalVP))
					{
						pF->m_Flags |= XW_FACE_VISIBLE;
						bVis = true;
					}
				}
				else
				{
					pF->m_Flags |= XW_FACE_VISIBLE;
					bVis = true;
				}

//			RenderBox(m_pCurrentRC, pF->m_BoundBox, 0xff4040ff);
			}
#endif

			if (bVis)
			{
				int iPL = m_spSBLink->EnumPortalLeaves_Single(iSB);
				bool bUseCompiled = (ms_Enable & MODEL_BSP_ENABLE_MINTESSELATION) ? true : pSB->InitTesselation(m_pView->m_CurLocalVP);
				for(int iFace = 0; iFace < m_lspSplineBrushes[iSB]->m_lFaces.Len(); iFace++)
				{
					TMeasureProfile(ms_TimeSplineVB);

					CSplineBrush_Face* pF = &pSB->m_lFaces[iFace];
					if (pF->m_Flags & XW_FACE_VISIBLE)
						if (bUseCompiled && !bNHF && !pF->m_DynLightMask && !(pF->m_Flags & XW_FACE_TRANSPARENT))
							VB_AddPrimitives(pF->m_iVB, &m_lSBPrim[pF->m_iPrim], pF->m_nPrim);
//							RenderSplineBrushFace_VB(m_lspSplineBrushes[iSB], iFace, iPL);
						else
							RenderSplineBrushFace(m_lspSplineBrushes[iSB], iFace, bUseCompiled, iPL);
				}
			}
		}
	}
	else
	{
		int bNHF = (ms_RenderInfo.m_Flags & CXR_RENDERINFO_NHF);

		for(int iSB = 0; iSB < m_lspSplineBrushes.Len(); iSB++)
		{
			CSplineBrush* pSB = m_lspSplineBrushes[iSB];
//			pSB->SetAllFacesVisible();

			bool bVis = false;
			int iFace;
			for(iFace = 0; iFace < m_lspSplineBrushes[iSB]->m_lFaces.Len(); iFace++)
			{
				CSplineBrush_Face* pF = &pSB->m_lFaces[iFace];
				pF->m_Flags &= ~XW_FACE_VISIBLE;
				CXW_Surface* pSurf = m_lspSurfaces[pF->m_iSurface];
				if (pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE) continue;

				if (!(pSurf->m_Flags & XW_SURFFLAGS_NOCULL))
				{
					if (pSB->Face_IsVisible(iFace, m_pView->m_CurLocalVP))
					{
						pF->m_Flags |= XW_FACE_VISIBLE;
						bVis = true;
					}
				}
				else
				{
					pF->m_Flags |= XW_FACE_VISIBLE;
					bVis = true;
				}
			}


			if (bVis)
			{
//				bool bUseCompiled = (ms_Enable & MODEL_BSP_ENABLE_MINTESSELATION) ? true : pSB->InitTesselation(m_pView->m_CurLocalVP);
				bool bUseCompiled = true;

//				bool bUseCompiled = pSB->InitTesselation(m_pView->m_CurLocalVP);
				for(int iFace = 0; iFace < m_lspSplineBrushes[iSB]->m_lFaces.Len(); iFace++)
				{
					CSplineBrush_Face* pF = &pSB->m_lFaces[iFace];
					if (pF->m_Flags & XW_FACE_VISIBLE)
					{
//						ConOut(CStrF("Spline face vis. %d, %d", iSB, iFace));
						if (bUseCompiled && !bNHF && !pF->m_DynLightMask && !(pF->m_Flags & XW_FACE_TRANSPARENT))
							VB_AddPrimitives(pF->m_iVB, &m_lSBPrim[pF->m_iPrim], pF->m_nPrim);
//							RenderSplineBrushFace_VB(m_lspSplineBrushes[iSB], iFace, 0);
						else
							RenderSplineBrushFace(m_lspSplineBrushes[iSB], iFace, bUseCompiled);
					}
				}
			}
		}
	}
}

void CXR_Model_BSP::RenderSplineBrushes(int _iPL)
{
	MAUTOSTRIP(CXR_Model_BSP_RenderSplineBrushes_2, MAUTOSTRIP_VOID);
	if (!m_spSBLink) return;

	uint16 liSB[1024];
	int nSB = m_spSBLink->EnumIDs(_iPL, liSB, 1024, NULL, 0);
	int iRP = m_pView->m_liLeafRPortals[_iPL];
	if (!iRP) return;

	CXR_FogState* pFog = (ms_pCurrentEngine) ? ms_pCurrentEngine->GetFogState() : NULL;
	if (pFog && !pFog->m_VtxFog_Enable) pFog = NULL;

	const CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[_iPL];
	int bNHF = pPL->m_ContainsMedium & XW_MEDIUM_FOG;

//ConOut("BackPlane " + m_pView->m_LocalBackPlane.GetString());
	for(int iiSB = 0; iiSB < nSB; iiSB++)
	{
		int iSB = liSB[iiSB];
		CSplineBrush* pSB = m_lspSplineBrushes[iSB];
		if (pSB->m_LastViewRendered != m_pView->m_CurrentView)
		{
			const CBox3Dfp32& Box = pSB->m_BoundBox;
//			ConOut(CStrF("%d", m_pView->m_LocalBackPlane.GetBoxMinDistance(Box.m_Min, Box.m_Max)));

			if (m_pView->m_LocalBackPlane.GetBoxMinDistance(Box.m_Min, Box.m_Max) > 0.0f) continue;

			if (pFog && pFog->m_VtxFog_EndPlane.GetBoxMinDistance(Box.m_Min, Box.m_Max) > 0.0f) continue;
			if (!BoxInClipVolume(Box, m_pView->m_lRPortals[iRP])) continue;

			pSB->m_LastViewRendered = m_pView->m_CurrentView;
#if 1
			for(int iFace = 0; iFace < pSB->m_lFaces.Len(); iFace++)
			{
				CSplineBrush_Face* pF = &pSB->m_lFaces[iFace];
				CXW_Surface* pSurf = m_lspSurfaces[pF->m_iSurface];

				if (pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE)
					continue;

				if (!(pSurf->m_Flags & XW_SURFFLAGS_NOCULL))
					if (!pSB->Face_IsVisible(iFace, m_pView->m_CurLocalVP))
						continue;

				if (!bNHF && !pF->m_DynLightMask && !(pF->m_Flags & XW_FACE_TRANSPARENT))
					VB_AddPrimitives(pF->m_iVB, &m_lSBPrim[pF->m_iPrim], pF->m_nPrim);
				else
				{
					RenderSplineBrushFace(pSB, iFace, true, _iPL);
				}
			}

#else

			bool bVis = false;
			int iFace;
			for(iFace = 0; iFace < pSB->m_lFaces.Len(); iFace++)
			{
				CSplineBrush_Face* pF = &pSB->m_lFaces[iFace];
				pF->m_Flags &= ~XW_FACE_VISIBLE;
				CXW_Surface* pSurf = m_lspSurfaces[pF->m_iSurface];
				if (pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE)
					continue;

//				if (!BoxInClipVolume(pF->m_BoundBox, m_pView->m_lRPortals[iRP])) continue;

				if (!(pSurf->m_Flags & XW_SURFFLAGS_NOCULL))
				{
					if (pSB->Face_IsVisible(iFace, m_pView->m_CurLocalVP))
					{
						pF->m_Flags |= XW_FACE_VISIBLE;
						bVis = true;
					}
				}
				else
				{
					pF->m_Flags |= XW_FACE_VISIBLE;
					bVis = true;
				}
			}

			if (bVis)
			{
				ms_TimeSplineVB -= GetCPUClock();

				bool bUseCompiled = (ms_Enable & MODEL_BSP_ENABLE_MINTESSELATION) ? true : pSB->InitTesselation(m_pView->m_CurLocalVP);
//				bool bUseCompiled = pSB->InitTesselation(m_pView->m_CurLocalVP);
				for(int iFace = 0; iFace < pSB->m_lFaces.Len(); iFace++)
				{
					CSplineBrush_Face* pF = &pSB->m_lFaces[iFace];
					if (pF->m_Flags & XW_FACE_VISIBLE)
						if (bUseCompiled && !bNHF && !pF->m_DynLightMask && !(pF->m_Flags & XW_FACE_TRANSPARENT))
							VB_AddPrimitives(pF->m_iVB, &m_lSBPrim[pF->m_iPrim], pF->m_nPrim);
//							RenderSplineBrushFace_VB(pSB, iFace, _iPL);
						else
							RenderSplineBrushFace(pSB, iFace, bUseCompiled, _iPL);
				}

				ms_TimeSplineVB += GetCPUClock();
			}
#endif
		}
	}
}



