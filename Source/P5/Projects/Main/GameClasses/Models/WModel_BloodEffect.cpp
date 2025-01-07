/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WModel_BloodEffect.cpp

Author:			Patrik Willbo

Copyright:		2006 Starbreeze Studios AB

Contents:		CXR_Model_BloodEffect

Comments:

History:
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WModel_BloodEffect.h"
#include "WModel_EffectSystem.h"


MRTC_IMPLEMENT_DYNAMIC(CXR_Model_BloodEffect, CXR_Model_Custom);


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_BloodEffect
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXR_Model_BloodEffect::CXR_Model_BloodEffect()
{
}


void CXR_Model_BloodEffect::OnCreate(const char* _pParam)
{
	// Reset parameters
	m_CloudSurfaces.Clear();
	m_SpurtSurfaces.Clear();

	CFStr Param(_pParam);
	while(Param != "")
	{
		CFStr Key = Param.GetStrSep(",");
		const uint32 KeyHash = StringToHash(Key.GetStrSep("="));
		switch(KeyHash)
		{
		case MHASH1('SS'):	// "SS" Spurt Surfaces
			{
				uint8 nSurfaces = 0;
				uint16* pSurfaceID = m_SpurtSurfaces.m_lSurfaceID;
				while(Key != "" && nSurfaces < MAXSURFACES)
					pSurfaceID[nSurfaces++] = GetSurfaceID(Key.GetStrSep(";"));
				m_SpurtSurfaces.m_nSurfaces = nSurfaces;
			}
			break;

		case MHASH1('CS'):	// "CS" Cloud Surfaces
			{
				uint8 nSurfaces = 0;
				uint16* pSurfaceID = m_CloudSurfaces.m_lSurfaceID;
				while(Key != "" && nSurfaces < MAXSURFACES)
					pSurfaceID[nSurfaces++] = GetSurfaceID(Key.GetStrSep(";"));
				m_CloudSurfaces.m_nSurfaces = nSurfaces;
			}
			break;

		case MHASH1('SW1'):	// "SW1" Spurt Width Exit
			{
				m_SpurtWidth1 = (fp32)Key.Val_fp64();
			}
			break;

		case MHASH1('SW2'):	// "SW2" Spurt Width Entrance
			{
				m_SpurtWidth2 = (fp32)Key.Val_fp64();
			}
			break;

		case MHASH1('SL1'):	// "SL1" Spurt Length Exit
			{
				m_SpurtLength1 = (fp32)Key.Val_fp64();
			}
			break;

		case MHASH1('SL2'):	// "SL2" Spurt Length Entrance
			{
				m_SpurtLength2 = (fp32)Key.Val_fp64();
			}
			break;

		case MHASH1('CR1'):	// "CR1" Cloud Radius Exit
			{
				m_CloudRadius1 = (fp32)Key.Val_fp64();
			}
			break;

		case MHASH1('CR2'):	// "CR2" Cloud Radius Entrance
			{
				m_CloudRadius2 = (fp32)Key.Val_fp64();
			}
			break;

		case MHASH1('CH1'):	// "CH1" Cloud Height Exit
			{
				m_CloudHeight1 = (fp32)Key.Val_fp64();
			}
			break;

		case MHASH1('CH2'):	// "CH2" Cloud Height Entrance
			{
				m_CloudHeight2 = (fp32)Key.Val_fp64();
			}
			break;
		}
	}
}


void CXR_Model_BloodEffect::OnRender(CXR_Engine *_pEngine, CRenderContext *_pRender, CXR_VBManager *_pVBM, CXR_ViewClipInterface *_pViewClip,
									spCXR_WorldLightState _spWLS, const CXR_AnimState *_pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, int _Flags)
{
	// Setup render parameter
	CBloodEffectRenderParam RenderParam;
	if (!RenderParam.Init(_pVBM, _pEngine, _pAnimState, _WMat, _VMat))
		return;

	uint8* pPoints = RenderParam.m_pModelParam->m_nPoints;
	uint32 nPoints = pPoints[0] + pPoints[1] + pPoints[2] + pPoints[3];

	CFXVBMAllocUtil VBMHelp;
	int32 Seed = _pAnimState->m_Data[1];
	CSpline_BloodSpurt SpurtSpline(m_SpurtSurfaces.m_lSurfaceID, m_SpurtSurfaces.m_nSurfaces, nPoints);

	// Create blood effect components
	OnCreate_Spurt(&VBMHelp, &SpurtSpline, RenderParam.m_pModelParam, Seed);
	OnCreate_Cloud(&VBMHelp, RenderParam.m_pModelParam, Seed + 0x100);

	if (VBMHelp.Alloc(_pVBM))
	{
		OnRender_SetupLights(&RenderParam);
		
		// Render our components
		OnRender_Spurt(&VBMHelp, &SpurtSpline, &RenderParam);
		OnRender_Cloud(&VBMHelp, &RenderParam, Seed + 0x100);
	}
}


//void CXR_Model_BloodEffect::OnPrecache(CXR_Engine *_pEngine, int _iVariation)
//{
//}


void CXR_Model_BloodEffect::OnRender_SetupLights(CBloodEffectRenderParam* _pRP)
{
	if (_pRP->m_pEngine->m_pSceneGraphInstance)
	{
		GetBound_Box(_pRP->m_BBox, _pRP->m_pAnimState);
		CXR_RenderSurfExtParam& Params = _pRP->m_Params;
		
		CXR_Light const* lpLights[BLOODEFFECT_LIGHTS_MAX];
		
		CBox3Dfp32 BoxW;
		_pRP->m_BBox.Transform(_pRP->m_M2W, BoxW);

		CMat4Dfp32 WMatInv;
		_pRP->m_M2W.InverseOrthogonal(WMatInv);
		int nLights = _pRP->m_pEngine->m_pSceneGraphInstance->SceneGraph_Light_Enum(BoxW, lpLights, BLOODEFFECT_LIGHTS_MAX);

		CXR_Light* lLights = _pRP->m_lLights;
		for (int i = 0; i < nLights; i++)
			lLights[i] = *(lpLights[i]);

		Params.m_pLights = lLights;
		Params.m_nLights = nLights;

		CVec4Dfp32 lLFAxes[6];
		if (_pRP->m_RenderInfo.m_pLightVolume)
		{
			CVec3Dfp32 BoundV[8];
			BoxW.GetVertices(BoundV);
			_pRP->m_RenderInfo.m_pLightVolume->Light_EvalPointArrayMax(BoundV, 8, lLFAxes);
			Params.m_pLightFieldAxes = lLFAxes;
		}

		_pRP->m_Flags |= BLOODEFFECT_LIGHTING;
	}
}


void CXR_Model_BloodEffect::OnCreate_Cloud(CFXVBMAllocUtil* _pVBMHelp, CBloodEffectModelParam* _pModelParam, const int32 _Seed)
{
	const int nClouds = ((_pModelParam->m_bCloudEntrance) ? 1 : 0) + ((_pModelParam->m_bCloudExit) ? 1 : 0);
	if (nClouds > 0)
	{
		const int nSeg = 4;			// PARAMETER CONTROLED!!
		const int nV = nSeg + 1;
		const int nP = nSeg * 3;
		
		// Add vb memory
		_pVBMHelp->Alloc_Any(FXAlign16(sizeof(CMat4Dfp32)) * nClouds);
		_pVBMHelp->Alloc_Int16(nP);

		_pVBMHelp->Store_Alloc(1, 1, 1, nV, nV, 0);
		_pVBMHelp->Alloc_Store(nClouds);
		_pVBMHelp->Alloc_Any(FXAlign16(sizeof(CXW_SurfaceKeyFrame))*nClouds);
	}
}


void CXR_Model_BloodEffect::OnRender_Cloud(CFXVBMAllocUtil* _pVBMHelp, CBloodEffectRenderParam* _pRP, const int32 _Seed)
{
	int nClouds = 0;
	CVec3Dfp32 Pos[2];
	CMat4Dfp32 WMat;

	static fp32 sRadius1 = 64.0f;
	static fp32 sHeight1 = 8.0f;
	static fp32 sRadius2 = 16.0f;
	static fp32 sHeight2 = 4.0f;
	static int sParamCtrl = 1;

	fp32 lRadius[2] = { m_CloudRadius1, m_CloudRadius2 };
	fp32 lHeight[2] = { m_CloudHeight1, m_CloudHeight2 };

	if (!sParamCtrl)
	{
		lRadius[0] = sRadius1;
		lRadius[1] = sRadius2;
		lHeight[0] = sHeight1;
		lHeight[1] = sHeight2;
	}

	WMat = _pRP->m_M2W;
	CBloodEffectModelParam* pModelParam = _pRP->m_pModelParam;
	if (pModelParam->m_bCloudEntrance)
	{
		Pos[nClouds++] = *pModelParam->m_pEntrance;
		if (pModelParam->m_bCloudExit)
			Pos[nClouds++] = *pModelParam->m_pExit;
	}
	else if (pModelParam->m_bCloudExit)
	{
		Swap(lRadius[0], lRadius[1]);
		Swap(lHeight[0], lHeight[1]);
		Pos[nClouds++] = *pModelParam->m_pExit;
		WMat.GetRow(0) = -WMat.GetRow(0);
	}

	if (nClouds > 0)
	{
		const int nSeg = 4;
		const int Len = nSeg - 1;
		const int nV = nSeg + 1;
		const int nP = nSeg * 3;
		
		const int SurfaceMask = m_CloudSurfaces.m_nSurfaces - 1;
		CRand_MersenneTwister* pRand = MRTC_GetRand();
		pRand->InitRand(_Seed);
		const uint16 SurfaceID = m_CloudSurfaces.m_lSurfaceID[pRand->GenRand32() & (m_CloudSurfaces.m_nSurfaces - 1)];
		if (!SurfaceID)
			return;

		// Setup primitive indices
		uint16* pP = (uint16*)_pVBMHelp->Get_Int16(nP);
		int iIndex = 0;
		for(int i = 0; i < Len; i)
		{
			pP[iIndex++] = i++;
			pP[iIndex++] = nSeg;
			pP[iIndex++] = i;
		}

		pP[iIndex++] = Len;
		pP[iIndex++] = nSeg;
		pP[iIndex++] = 0;

		const fp32 SegAngleRcp = 1.0f / nSeg;
		
		// Setup VB chains
		CXR_SurfaceContext* pSC = _pRP->m_pSC;
		CXR_Engine* pEngine = _pRP->m_pEngine;
		CMTime& AnimTime0 = _pRP->m_pAnimState->m_AnimTime0;
		CXR_RenderInfo& RenderInfo = _pRP->m_RenderInfo;
		CXR_VBManager* pVBM = _pRP->m_pVBM;

		for (int iCloud = 0; iCloud < nClouds; iCloud++)
		{
			const fp32 Radius = lRadius[iCloud];
			const fp32 Height = lHeight[iCloud];

			CXR_VertexBuffer* pVB = _pVBMHelp->Get_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES | CXR_VB_TVERTICES0, nV);
			CXR_VBChain* pVBChain = (CXR_VBChain*)pVB->m_pVBChain;

			pVBChain->Render_IndexedTriangles(pP, nSeg);
			CVec3Dfp32* pV = pVBChain->m_pV;
			CVec2Dfp32* pTV = (CVec2Dfp32*)pVBChain->m_pTV[0];
			
			fp32 sinv, cosv;
			for (int i = 0; i < nSeg; i++)
			{
				QSinCosUnit(SegAngleRcp * i, sinv, cosv);
				const fp32 sincos0 = sinv + cosv;
				const fp32 sincos1 = sinv - cosv;

				pV[i] = CVec3Dfp32(Height, sincos0 * Radius, sincos1 * Radius);
				pTV[i] = CVec2Dfp32(1-((sincos0 * 0.5f) + 0.5f), (sincos1 * 0.5f) + 0.5f);
			}

			pV[nSeg] = 0.0f;
			pTV[nSeg] = 0.5f;

			CMat4Dfp32* pTransform = _pVBMHelp->Get_M4();
			WMat.GetRow(3) = Pos[iCloud];
			WMat.Multiply(_pRP->m_W2V, *pTransform);
			WMat.GetRow(0) = -WMat.GetRow(0);

			pVB->Matrix_Set(pTransform);

			CXW_Surface* pSurface = pSC->GetSurface(SurfaceID);
			pSurface = (pSurface) ? pSurface->GetSurface(pEngine->m_SurfOptions, pEngine->m_SurfCaps) : NULL;
			CXW_SurfaceKeyFrame* pKeyFrameStorage = (CXW_SurfaceKeyFrame*)_pVBMHelp->Get_Any(FXAlign16(sizeof(CXW_SurfaceKeyFrame)));
			CXW_SurfaceKeyFrame* pSurfaceKey = (pSurface) ? pSurface->GetFrame(0, AnimTime0, pKeyFrameStorage) : NULL;
			if (pSurface && pSurfaceKey)
			{
				int Flags = RENDERSURFACE_MATRIXSTATIC_M2V;
				fp32 BasePrio = (pSurface->m_Flags & XW_SURFFLAGS_TRANSPARENT) ?
					RenderInfo.m_BasePriority_Transparent : RenderInfo.m_BasePriority_Opaque;

				pVB->m_Priority = BasePrio;
				if ((pSurface->m_Flags & XW_SURFFLAGS_LIGHTING) && (_pRP->m_Flags & BLOODEFFECT_LIGHTING))
				{
					Flags |= (RENDERSURFACE_DEPTHFOG | RENDERSURFACE_LIGHTNONORMAL);
					CXR_Util::Render_Surface(Flags, AnimTime0, pSurface, pSurfaceKey, pEngine, pVBM, (CMat4Dfp32*) NULL,
						(CMat4Dfp32*) NULL, pVB->m_pTransform, pVB, BasePrio, 0.0001f, &_pRP->m_Params);
				}
				else
				{
					CXR_Util::Render_Surface(Flags, AnimTime0, pSurface, pSurfaceKey, pEngine, pVBM, (CMat4Dfp32*) NULL,
						(CMat4Dfp32*) NULL, pVB->m_pTransform, pVB, BasePrio);
				}
			}
		}
	}
}


void CXR_Model_BloodEffect::OnCreate_Spurt(CFXVBMAllocUtil* _pVBMHelp, CSpline_BloodSpurt* _pSpurtSpline, CBloodEffectModelParam* _pModelParam, const int32 _Seed)
{
	const CVec3Dfp32* pPoints = _pModelParam->m_pPointData;
	const fp32* pLength = _pModelParam->m_pLength;
	const CVec3Dfp32& Entrance = *_pModelParam->m_pEntrance;
	const CVec3Dfp32& Exit = *_pModelParam->m_pExit;
	const CVec3Dfp32 TangIn(0,0,1);
	const CVec3Dfp32 TangOut(0,0,-1);
	//const int SurfaceMask = m_SpurtSurfaces.m_nSurfaces - 1;
	const int nSurfaces = m_SpurtSurfaces.m_nSurfaces;

	fp32 Width1 = m_SpurtWidth1;
	fp32 Width2 = m_SpurtWidth2;
	fp32 Length1 = m_SpurtLength1;
	fp32 Length2 = m_SpurtLength2;

	static fp32 sWidth1 = 6.0f;
	static fp32 sWidth2 = 4.0f;
	static fp32 sLength1 = 32.0f;
	static fp32 sLength2 = 16.0f;
	static int sParamCtrl = 1;
	if (!sParamCtrl)
	{
		Width1 = Width1;
		Width2 = Width2;
		Length1 = Length1;
		Length2 = Length2;
	}

	static int iSpurt1 = -1;
	static int iSpurt2 = -1;

	// Add entrance and exit points
	CRand_MersenneTwister* pRand = MRTC_GetRand();
	pRand->InitRand(_Seed);
	for(int j = 0; j < 4; j++)
	{
		const int nPoints = _pModelParam->m_nPoints[j];

		if (j & 1)
		{
			for (int i = 0; i < nPoints; i++)
			{
				iSpurt1 = CFXSysUtil::RandExclude(nSurfaces, iSpurt1, pRand->GenRand32());
				_pSpurtSpline->SetSpline(Width1, Length1, 0.0f, Exit, pPoints[i], pLength[i], TangIn, TangOut, iSpurt1);
			}
		}
		else
		{
			for (int i = 0; i < nPoints; i++)
			{
				iSpurt2 = CFXSysUtil::RandExclude(nSurfaces, iSpurt2, pRand->GenRand32());
				_pSpurtSpline->SetSpline(Width2, Length2, 0.0f, Entrance, pPoints[i], pLength[i], TangIn, TangOut, iSpurt2);
			}
		}
		
		pPoints += nPoints;
		pLength += nPoints;
	}

	_pSpurtSpline->CalcVBMem(_pVBMHelp);
}


void CXR_Model_BloodEffect::OnRender_Spurt(CFXVBMAllocUtil* _pVBMHelp, CSpline_BloodSpurt* _pSpurtSpline, CBloodEffectRenderParam* _pRP)
{
	if (_pSpurtSpline->Finalize(_pVBMHelp, _pRP->m_pVBM, _pRP->m_W2V))
	{
		#ifndef M_RTM
			_pSpurtSpline->Debug_Render();
		#endif

		uint Flags = (_pRP->m_Flags & BLOODEFFECT_LIGHTING) ? BEAMSPLINE_FLAGS_LIGHTING : 0;
		_pSpurtSpline->Render(_pRP->m_pSC, _pRP->m_pEngine, _pRP->m_pVBM, &_pRP->m_RenderInfo, _pRP->m_pAnimState->m_AnimTime0, Flags, &_pRP->m_Params);
	}
}

