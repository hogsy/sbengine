#include "PCH.h"
#include "WModel_ExplosionSystem.h"


MRTC_IMPLEMENT_DYNAMIC(CXR_Model_ExplosionSystem, CXR_Model_Custom);


static const char* g_lpTranslateExplosionSystemFlags[] = { "ds", "lt", NULL };

CXR_Model_ExplosionSystem::CXR_Model_ExplosionSystem()
{
	SetThreadSafe(true);
}

void CXR_Model_ExplosionSystem::OnCreate(const char* _pParam)
{
	// Make sure utility system has been initialized and parse model keys
	CXR_Util::Init();
	SetDefaultParameters();
	ParseKeys(_pParam);
}


void CXR_Model_ExplosionSystem::SetDefaultParameters()
{
	// Setup default smoke pillar values
	m_SmokePillar.m_Num.SetRange(1,1);
	m_SmokePillar.m_Force.SetRange(CVec2Dfp32(100,300),CVec2Dfp32(300,900));
	m_SmokePillar.m_Size.SetRange(0,10);
	m_SmokePillar.m_Rot.SetRange(0,0);
	m_SmokePillar.m_Color.SetRange(1,CVec4Dfp32(1,1,1,0));
	m_SmokePillar.m_ColorFadeTime = 1.0f;
	m_SmokePillar.m_NumTimeCellParticles = 1;
	m_SmokePillar.m_TimeCell = 0.01f;
	m_SmokePillar.m_LifeTime = 2.0f;
	m_SmokePillar.m_FadeInTime = 0.0f;
	m_SmokePillar.m_FadeOutTime = 0.0f;
	m_SmokePillar.m_Gravity = 314.24f;
	m_SmokePillar.m_SizeGrow = 1.0f;
	m_SmokePillar.m_EmissionStop = 2.0f;
	m_SmokePillar.m_ZSize = 0;
	m_SmokePillar.m_SurfaceID = 0;

	m_AlphaFadeOut = 0.0f;
	m_Duration = 4.0f;
	m_TimeOffset = 0.0f;
	m_TimeScale = 1.0f;
	m_Flags = 0;
}


void CXR_Model_ExplosionSystem::OnEvalKey(const CRegistry* _pReg)
{
	CStr KeyName = _pReg->GetThisName();
	uint32 KeyHash = StringToHash(KeyName);
	CStr Value = _pReg->GetThisValue();
	const int Valuei = Value.Val_int();
	const fp32 Valuef = (fp32)Value.Val_fp64();

	switch(KeyHash)
	{
	case MHASH1('TS'):	// "TS" TimeScale
		{
			m_TimeScale = Valuef;
			break;
		}

	case MHASH1('TO'):	// "TO" TimeOffset
		{
			m_TimeOffset = Valuef;
			break;
		}

	case MHASH1('SU'):	// "SU" Surface
		{
			m_SmokePillar.m_SurfaceID = GetSurfaceID(Value.GetStr());
			break;
		}

	case MHASH1('SP0'):	// "SP0" Min SmokePillars
		{
			m_SmokePillar.m_Num.m_Min = (uint8)Valuei;
			break;
		}

	case MHASH1('SP1'):	// "SP1" Max SmokePillars
		{
			m_SmokePillar.m_Num.m_Max = (uint8)Valuei;
			break;
		}

	case MHASH1('FO0'):	// "FO" Force Min
		{
			m_SmokePillar.m_Force.m_Min.ParseString(Value);
			break;
		}

	case MHASH1('FO1'): // "FO1" Force Max
		{
			m_SmokePillar.m_Force.m_Max.ParseString(Value);
			break;
		}

	case MHASH1('GR'):	// "GR" Gravity
		{
			m_SmokePillar.m_Gravity = Valuef;
			break;
		}

	case MHASH1('SZ0'):	// "SZ0" Size start min
		{
			m_SmokePillar.m_Size.m_Min = Valuef;
			break;
		}

	case MHASH1('SZ1'):	// "SZ1" Size start max
		{
			m_SmokePillar.m_Size.m_Max = Valuef;
			break;
		}

	case MHASH1('SZ2'):	// "SZ2" Size grow (over time)
		{
			m_SmokePillar.m_SizeGrow = Valuef;
			break;
		}

	case MHASH1('AL0'):	// "AL0" Alpha Duration
		{
			m_AlphaFadeOut = Valuef;
			break;
		}

	case MHASH1('ES'):	// "ES" Emission stop
		{
			m_SmokePillar.m_EmissionStop = Valuef;
			break;
		}

	case MHASH1('DU'):	// "DU" Duration
		{
			m_Duration = Valuef;
			break;
		}

	case MHASH1('CL0'):	// "CL0" Color Fade Time
		{
			m_SmokePillar.m_ColorFadeTime = Valuef;
			break;
		}

	case MHASH1('CL1'):	// "CL1" Color Start
		{
			m_SmokePillar.m_Color.m_Min.ParseString(Value);
			break;
		}

	case MHASH1('CL2'):	// "CL2" Color End
		{
			m_SmokePillar.m_Color.m_Max.ParseString(Value);
			break;
		}

	case MHASH1('FA0'):	// "FA0" Fade in time
		{
			m_SmokePillar.m_FadeInTime = Valuef;
			break;
		}

	case MHASH1('FA1'):	// "FA1" Fade out time
		{
			m_SmokePillar.m_FadeOutTime = Valuef;
			break;
		}

	case MHASH1('FL'):	// "FL" Flags
		{
			m_Flags = Value.TranslateFlags(g_lpTranslateExplosionSystemFlags);
			break;
		}

	case MHASH1('TC'):	// "TC" Timecell
		{
			m_SmokePillar.m_TimeCell = Valuef;
			break;
		}

	case MHASH1('TP'):	// "TP" Timecell particles
		{
			m_SmokePillar.m_NumTimeCellParticles = Valuei;
			break;
		}

	case MHASH1('LT'):	// "LT" Lifetime
		{
			m_SmokePillar.m_LifeTime = Valuef;
			break;
		}

	case MHASH1('ZS'):	// "ZS" Z-Size multiplier
		{
			m_SmokePillar.m_ZSize = Valuef;
			break;
		}
	
	case MHASH1('RO0'):	// "RO0" Rotation Start min/max
		{
			m_SmokePillar.m_Rot.m_Min.ParseString(Value);
			break;
		}
	
	case MHASH1('RO1'):	// "RO1" Rotation end min/max
		{
			m_SmokePillar.m_Rot.m_Max.ParseString(Value);
			break;
		}

	default:
		{
			CXR_Model_Custom::OnEvalKey(_pReg);
			break;
		}
	}
}


void CXR_Model_ExplosionSystem::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	_Box = CBox3Dfp32(-1,1);

	fp32 Size = m_SmokePillar.m_Size.m_Max + (m_SmokePillar.m_SizeGrow * Sqr(m_Duration));
	
	CVec3Dfp32 Force;
	Force.k[0] = 
	Force.k[1] = (m_SmokePillar.m_Force.m_Max.k[0] * m_Duration);
	Force.k[2] = 0;
	_Box.Expand(CBox3Dfp32(-Force, Force));
	
	Force.k[0] = 
	Force.k[1] = 0;
	Force.k[2] = (m_SmokePillar.m_Force.m_Max.k[1] * m_Duration) + (m_SmokePillar.m_ZSize * Size);
	_Box.Expand(Force);
	
	_Box.Expand(-(m_SmokePillar.m_Gravity * Sqr(m_Duration)));

	_Box.Grow(Size);
}


void CXR_Model_ExplosionSystem::Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
{
	uint8 GenerateFlags = EXPLOSIONSYSTEM_GEN_ROTATION;

	CRenderParams RP;

	RP.m_Rand = _pAnimState->m_Anim0;
	RP.m_Time = (_pAnimState->m_AnimTime0.GetTime() + m_TimeOffset) * m_TimeScale;
	RP.m_pVBM = _pRenderParams->m_pVBM;

	// Calculate size needed in vertex buffer
	VB_CalcSize(&RP);

	// Allocate a buffer that can hold all our data
	//const int nVPerParticle = (m_Flags & PARTICLE_FLAGS_QUADS) ? 4 : 3;
	//const int nTriPerParticle = (m_Flags & PARTICLE_FLAGS_QUADS) ? 2 : 1;

    CMat4Dfp32 InvVMat;
	_VMat.InverseOrthogonal(InvVMat);
	RP.m_CameraPos = InvVMat.GetRow(3);
	RP.m_CameraLeft = InvVMat.GetRow(0);
	RP.m_CameraUp = InvVMat.GetRow(1);

	// Generate all modules
	Generate_SmokePillars(&RP);
	
	// Sort all particles if needed
	if(m_Flags & EXPLOSIONSYSTEM_FLAGS_DEPTHSORT)
		QuickSortParticles_r(&RP, 0, RP.m_nParticles);

	// Generate render
	Generate_Render(&RP, GenerateFlags);

	if (RP.m_pChain)
	{
		CXR_VertexBuffer VB;
	
		RP.m_SK.Create(GetSurfaceContext(), _pRenderParams->m_pEngine, _pAnimState, m_SmokePillar.m_SurfaceID);

		VB.SetVBChain(RP.m_pChain);
		CXW_Surface* pSurf = RP.m_SK.m_pSurface;
		if (pSurf->m_Flags & XW_SURFFLAGS_TRANSPARENT)
			VB.m_Priority = _pRenderParams->m_RenderInfo.m_BasePriority_Transparent;
		else
			VB.m_Priority = _pRenderParams->m_RenderInfo.m_BasePriority_Opaque;
		//VB.m_Priority += (pSurf->m_PriorityOffset * 0.001f) + (m_RenderPriorityBias * 0.01f);
		VB.m_Priority += (pSurf->m_PriorityOffset * 0.001f);

		CMat4Dfp32* pMat = _pRenderParams->m_pVBM->Alloc_M4();
		if (!pMat) return;
		_WMat.Multiply(_VMat, *pMat);
		VB.Matrix_Set(pMat);

		CXR_RenderSurfExtParam Params;

		// Enumerate lights and transform them to model space
		if ((m_Flags & EXPLOSIONSYSTEM_FLAGS_LIGHTING) && 
			(RP.m_SK.m_pSurface->m_Flags & XW_SURFFLAGS_LIGHTING) && 
			_pRenderParams->m_pEngine && 
			_pRenderParams->m_pEngine->m_pSceneGraphInstance)
		{
			const int MaxLights = 3;
			CXR_Light const* lpLights[MaxLights];

			CBox3Dfp32 Box, BoxW;
			GetBound_Box(Box, _pAnimState);
			Box.Transform(_WMat, BoxW);

			CMat4Dfp32 WMatInv;
			_WMat.InverseOrthogonal(WMatInv);
			int nLights = _pRenderParams->m_pEngine->m_pSceneGraphInstance->SceneGraph_Light_Enum(BoxW, lpLights, MaxLights);

			CXR_Light lLights[MaxLights];
			for(int i = 0; i < nLights; i++)
				lLights[i] = *(lpLights[i]);

			Params.m_pLights = lLights;
			Params.m_nLights = nLights;
		}

		RP.m_SK.Render(&VB, _pRenderParams->m_pVBM, _pRenderParams->m_pEngine, &Params);
	}
}


void CXR_Model_ExplosionSystem::VB_CalcSize(CRenderParams* M_RESTRICT _pRP)
{
	const uint8 nPillars = _pRP->m_Smoke_NumPillars = m_SmokePillar.m_Num.GetRandomInt(_pRP->m_Rand);
	CXR_VBManager* pVBM = _pRP->m_pVBM;

	uint32 nParticles = 0;
	_pRP->m_pChain = NULL;
	
	// Calculate amount of particles in smoke pillars
	if(nPillars)
	{
		const fp32 ModelTime = _pRP->m_Time;
		const fp32 Time = MinMT(m_SmokePillar.m_EmissionStop, ModelTime);
		const fp32 TimeCell = m_SmokePillar.m_TimeCell;
		const fp32 LifeTime = m_SmokePillar.m_LifeTime;
		const uint32 nTotalCells = (uint32)TruncToInt(Time / TimeCell);
		const uint8 nCellParticles = m_SmokePillar.m_NumTimeCellParticles;
		const uint32 nCells = MinMT((uint32)TruncToInt(LifeTime / TimeCell), nTotalCells) + 1;
		const uint32 nCellsRemoved = MaxMT(TruncToInt((ModelTime - (LifeTime - TimeCell)) / TimeCell), 0);
		nParticles += ((nCells * nCellParticles) * nPillars);
	}

	// Allocate vertex buffers and data
	const uint32 nV = nParticles * 3;
	if(nV > 0)
	{
		// Allocate size for vertex data
		_pRP->m_pV = pVBM->Alloc_V3(nV);
		_pRP->m_pC = pVBM->Alloc_CPixel32(nV);
		_pRP->m_pTempDepth = (fp32*)&_pRP->m_pC[nParticles];
		_pRP->m_pTempRand = (int32*)&_pRP->m_pC[nParticles*2];
		_pRP->m_lSize.SetLen(nParticles);
		_pRP->m_lRot.SetLen(nParticles);

		for(uint8 i = 0; i < nPillars; i++)
			_pRP->m_pTempRand[i] = TruncToInt((MFloat_GetRand(_pRP->m_Rand + (i * 100)) * 0xffffffff));

        // Allocate chains
		uint32 Offset = 0;
		for(int32 i = nParticles, j = 0; i > 0; i -= CXR_Util::MAXPARTICLES, j++)
		{
			// Calculate amount of particles in this chain
			const uint32 nRenderParticles = MinMT((uint32)(nParticles - (j * CXR_Util::MAXPARTICLES)), (uint32)CXR_Util::MAXPARTICLES);
			const uint32 nRenderV = nRenderParticles * 3;
			const uint32 iV = i * 3;

			// Allocate vb chain
			CXR_VBChain* pVB = pVBM->Alloc_VBChain();
			
			// Set data for chain
			pVB->m_nV = nRenderV;
			pVB->m_pV = &_pRP->m_pV[iV - nRenderV];
			pVB->m_pCol = &_pRP->m_pC[iV - nRenderV];
			pVB->m_pTV[0] = (fp32 *)CXR_Util::m_lTriParticleTVertices;
			pVB->m_nTVComp[0] = 2;
			pVB->Render_IndexedTriangles(CXR_Util::m_lTriParticleTriangles, nRenderParticles);
			
			// Increate offset into vertex and color pointer
			Offset += nRenderV;

			// Link
			pVB->m_pNextVB = _pRP->m_pChain;
			_pRP->m_pChain = pVB;
		}
	}

	_pRP->m_nParticles = nParticles;
}


void CXR_Model_ExplosionSystem::Generate_SmokePillars(CRenderParams* M_RESTRICT _pRP)
{
	// Check how many smoke pillars we're going to create
	const uint8 nPillars = _pRP->m_Smoke_NumPillars;
    if(nPillars)
	{
		const CVec3Dfp32& CamPos = _pRP->m_CameraPos;
		CVec3Dfp32* pPos = _pRP->m_pV;
		CPixel32* pCol = _pRP->m_pC;
		fp32* pDepth = _pRP->m_pTempDepth;
		fp32* pSize = _pRP->m_lSize.GetBasePtr();
		fp32* pRot = _pRP->m_lRot.GetBasePtr();

		uint32 PtrOffset = 0;

		const fp32 ModelTime = _pRP->m_Time;
		const fp32 Time = MinMT(m_SmokePillar.m_EmissionStop, ModelTime);
		const CVec4Dfp32& ColorMin = m_SmokePillar.m_Color.m_Min;
		const CVec4Dfp32& ColorMax = m_SmokePillar.m_Color.m_Max;
		const fp32 FadeInRcp = (m_SmokePillar.m_FadeInTime != 0.0f) ? (1.0f / m_SmokePillar.m_FadeInTime) : ModelTime / m_SmokePillar.m_TimeCell;
		const fp32 FadeOutRcp = (m_SmokePillar.m_FadeOutTime != 0.0f) ? (1.0f / m_SmokePillar.m_FadeOutTime) : ModelTime / m_SmokePillar.m_TimeCell;
		const fp32 ColorFadeTimeRcp = (m_SmokePillar.m_ColorFadeTime != 0.0f) ? (1.0f / m_SmokePillar.m_ColorFadeTime) : ModelTime / m_SmokePillar.m_TimeCell;
		const fp32 TimeCell = m_SmokePillar.m_TimeCell;
		const uint32 nTotalCells = (uint32)TruncToInt(Time / TimeCell);
		const uint32 nCells = MinMT((uint32)TruncToInt(m_SmokePillar.m_LifeTime / TimeCell), nTotalCells) + 1;
		const uint32 nCellsRemoved = (uint32)Clamp(TruncToInt((ModelTime - (m_SmokePillar.m_LifeTime - TimeCell)) / TimeCell), 0, (int32)nCells);
		const uint8 nTimeCellElem = m_SmokePillar.m_NumTimeCellParticles;
		const fp32 Gravity = m_SmokePillar.m_Gravity;
		const fp32 LifeTime = m_SmokePillar.m_LifeTime;
		const fp32 LifeTimeRcp = 1.0f / m_SmokePillar.m_LifeTime;
		const uint32 nVisCells = nCells - nCellsRemoved;
		const fp32 ZSize = m_SmokePillar.m_ZSize;
		fp32 SysAlphaMul = (m_AlphaFadeOut != 0.0f) ? Clamp01((m_Duration - ModelTime) * (1.0f / m_AlphaFadeOut)) : 1.0f;
		
		// Generate all pillars
		for(uint8 i = 0; i < nPillars; i++)
		{
			CVec3Dfp32 Offset;
			const CVec2Dfp32 Force = m_SmokePillar.m_Force.GetRandomFp(_pRP->m_Rand);
						
			// Get completly random direction
			//Offset.k[0] = (-1 + MFloat_GetRand(_pRP->m_Rand++) * 2) * Force.k[0];
			//Offset.k[1] = (-1 + MFloat_GetRand(_pRP->m_Rand++) * 2) * Force.k[0];
			//Offset.k[2] = (-1 + MFloat_GetRand(_pRP->m_Rand++) * 2) * Force.k[1];
			
			Offset.k[0] = -Force.k[0] + (MFloat_GetRand(_pRP->m_Rand++) * Force.k[0] * 2.0f);
			Offset.k[1] = -Force.k[0] + (MFloat_GetRand(_pRP->m_Rand++) * Force.k[0] * 2.0f);
			Offset.k[2] = Force.k[1];
			
			pPos = &pPos[PtrOffset];
			pDepth = &pDepth[PtrOffset];
			pCol = &pCol[PtrOffset];
			pSize = &pSize[PtrOffset];
			pRot = &pRot[PtrOffset];
			PtrOffset = (nVisCells * nTimeCellElem);

			int32 iPillarRand = _pRP->m_pTempRand[i];

			// Generate all cells
			for(uint32 iCell = 0; iCell < nVisCells; iCell++)
			{
				const int32 EmissionCell = (nTotalCells - iCell);
				const int32 CellID = (EmissionCell * nTimeCellElem);

				// Generate all particles in a cell
				for(uint32 iElem = 0, iPtr = iCell*nTimeCellElem; iElem < nTimeCellElem; iElem++)
				{
					int32 iElemRand = iPillarRand + CellID + (iElem * 2);
					fp32 EmissionTime = (fp32)EmissionCell * TimeCell;
					fp32 ElemTime = ModelTime - EmissionTime;
					fp32 ElemTimeRcp = (LifeTimeRcp * ElemTime);

					if(ElemTime > m_SmokePillar.m_LifeTime)
						continue;

					CPixel32& Color = pCol[iPtr];
					CVec3Dfp32& Pos = pPos[iPtr];

					// Calculate position
					Pos = CVec3Dfp32(0) + (Offset * EmissionTime);
					Pos.k[2] -= (Gravity * Sqr(EmissionTime));
					
					fp32 Size = m_SmokePillar.m_Size.GetRandomFp(iElemRand);
					Size += m_SmokePillar.m_SizeGrow * Sqr(ElemTime);
					Pos.k[2] += (ZSize * Size);

					// Calculate color
					const fp32 ColorFade = Clamp01(ColorFadeTimeRcp * ElemTime);
					CVec4Dfp32 ColorFp = ColorMin * (1.0f - ColorFade) + (ColorMax * ColorFade);
					ColorFp.k[3] *= Clamp01(FadeInRcp * ElemTime);
					ColorFp.k[3] *= Clamp01(FadeOutRcp * (LifeTime - ElemTime));
					ColorFp.k[3] *= SysAlphaMul;
					Color = CPixel32((uint8)Clamp(TruncToInt(ColorFp.k[0] * 255.0f), 0, 255),
									 (uint8)Clamp(TruncToInt(ColorFp.k[1] * 255.0f), 0, 255),
									 (uint8)Clamp(TruncToInt(ColorFp.k[2] * 255.0f), 0, 255),
									 (uint8)Clamp(TruncToInt(ColorFp.k[3] * 255.0f), 0, 255));

					// Calculate rotation
					CVec2Dfp32 Rot = m_SmokePillar.m_Rot.GetRandomFpVec2(iElemRand);
					pRot[iPtr] = Rot.k[0] * (1.0f - ElemTimeRcp) + Rot.k[1] * ElemTimeRcp;

					// Calculate distance to camera so we can sort them later if needed
					pDepth[iPtr] = (Pos - CamPos).LengthSqr();
					pSize[iPtr] = Size;
					iPtr++;
				}
			}
		}
	}
}


// Ooooh,, no no... Rewrite and make it correct!!
void CXR_Model_ExplosionSystem::QuickSortParticles_r(CRenderParams* M_RESTRICT _pRP, uint32 _Offset, uint32 _nParticles)
{
	// Get offset into pointers
	CVec3Dfp32* pPos = _pRP->m_pV + _Offset;
	CPixel32* pCol = _pRP->m_pC + _Offset;
	fp32* pSize = _pRP->m_lSize.GetBasePtr() + _Offset;
	fp32* pDepth = _pRP->m_pTempDepth + _Offset;
	fp32* pRot = _pRP->m_lRot.GetBasePtr() + _Offset;
	
	// If we only have two particles, do special case here
	if(_nParticles == 2)
	{
		if(pDepth[0] < pDepth[1])
		{
			Swap(pDepth[0], pDepth[1]);
			Swap(pPos[0], pPos[1]);
			Swap(pCol[0], pCol[1]);
			Swap(pSize[0], pSize[1]);
			Swap(pRot[0], pRot[1]);
		}

		return;
	}

	// Get pivot and setup indicing
	uint iPivot = _nParticles / 2;
	fp32 PivotDepth = pDepth[iPivot];
	int iLeft = -1;
	int iRight = _nParticles;
	
	while(iLeft < iRight)
	{
		do { iLeft++;	} while(iLeft <= iRight && pDepth[iLeft]  > PivotDepth);
		do { iRight--;	} while(iLeft <= iRight && pDepth[iRight] < PivotDepth);

		if(iLeft < iRight)
		{
			Swap(pDepth[iLeft], pDepth[iRight]);
			Swap(pPos[iLeft], pPos[iRight]);
			Swap(pCol[iLeft], pCol[iRight]);
			Swap(pSize[iLeft], pSize[iRight]);
			Swap(pRot[iLeft], pRot[iRight]);
		}
	}

	// Recurse
	if(iLeft > 1)
		QuickSortParticles_r(_pRP, _Offset, iLeft);
	if(iRight < _nParticles - 2)
		QuickSortParticles_r(_pRP, _Offset + iRight + 1, _nParticles - iRight - 1);
}


void CXR_Model_ExplosionSystem::Generate_Render(CRenderParams* M_RESTRICT _pRP, uint8 _GenerateFlags)
{
	if(_GenerateFlags & EXPLOSIONSYSTEM_GEN_ROTATION)
		Generate_Render_Rotation(_pRP);
	else
		Generate_Render_Simple(_pRP);

	_pRP->m_lSize.Clear();
	_pRP->m_lRot.Clear();
}


void CXR_Model_ExplosionSystem::Generate_Render_Rotation(CRenderParams* M_RESTRICT _pRP)
{
	const CVec3Dfp32& CamW = _pRP->m_CameraLeft;
	const CVec3Dfp32& CamH = _pRP->m_CameraUp;
	const int32 nParticles = _pRP->m_nParticles;
	int32 iV = (nParticles * 3) - 1;

    // Create triangles
	fp32* pSize = _pRP->m_lSize.GetBasePtr();
	fp32* pRot = _pRP->m_lRot.GetBasePtr();
	CVec3Dfp32* pSrcV = _pRP->m_pV;
	CVec3Dfp32* pDstV = _pRP->m_pV;
	CPixel32* pSrcC = _pRP->m_pC;
	CPixel32* pDstC = _pRP->m_pC;
    for(int32 i = nParticles - 1; i >= 0; i--)
	{
		fp32 Cos, Sin;
		fp32 Size = pSize[i] * 0.5f;
		CVec3Dfp32& SrcV = pSrcV[i];
		CPixel32& SrcC = pSrcC[i];

		QSinCos(pRot[i], Sin, Cos);
		
		CVec3Dfp32 ParticleW = (CamW * Cos - CamH * Sin) * Size;
		CVec3Dfp32 ParticleH = (CamW * Sin + CamH * Cos) * Size;

		pDstC[iV] = SrcC;
		pDstV[iV--] = SrcV - ParticleW + (ParticleH * 3.0f);
		pDstC[iV] = SrcC;
		pDstV[iV--] = SrcV + (ParticleW * 3.0f) - ParticleH;
		pDstC[iV] = SrcC;
		pDstV[iV--] = SrcV - ParticleW - ParticleH;
	}
}


void CXR_Model_ExplosionSystem::Generate_Render_Simple(CRenderParams* M_RESTRICT _pRP)
{
	const CVec3Dfp32 HalfWidth	= _pRP->m_CameraLeft * 0.5f;
	const CVec3Dfp32 HalfHeight	= _pRP->m_CameraUp * 0.5f;
	const CVec3Dfp32 HalfWidth3	= HalfWidth * 3;
	const CVec3Dfp32 HalfHeight3	= HalfHeight * 3;
	const int32 nParticles = _pRP->m_nParticles;
	int32 iV = (nParticles * 3) - 1;

	// Create triangles
	fp32* pSize = _pRP->m_lSize.GetBasePtr();
	CVec3Dfp32* pSrcV = _pRP->m_pV;
	CVec3Dfp32* pDstV = _pRP->m_pV;
	CPixel32* pSrcC = _pRP->m_pC;
	CPixel32* pDstC = _pRP->m_pC;
    for(int32 i = nParticles - 1; i >= 0; i--)
	{
		fp32 Size = pSize[i];
		CVec3Dfp32& SrcV = pSrcV[i];
		CPixel32& SrcC = pSrcC[i];

		pDstC[iV] = SrcC;
		pDstV[iV--] = SrcV - (HalfWidth * Size)  + (HalfHeight3 * Size);
		pDstC[iV] = SrcC;
		pDstV[iV--] = SrcV + (HalfWidth3 * Size) - (HalfHeight * Size);
		pDstC[iV] = SrcC;
		pDstV[iV--] = SrcV - (HalfWidth * Size)  - (HalfHeight * Size);
	}
}

