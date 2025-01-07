/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WModel_EffectSystem.cpp

Author:			Patrik Willbo

Copyright:		2006 Starbreeze Studios AB

Contents:		CFXSysUtil					Helper
				CSpline_Beam				Create render beams on splines
				CSpline_BeamStrip			Create a beamstrip from spline
				CEffectSystemRenderChain	Render storage
				CEffectSystemRenderData		Used in render chain
				CFXLayer					Overloading particle systems using layers
				CFXLayerTwirl				Twirl particles
				CFXLayerBoneBind			Spawn particles on bones
				CFXLayerBoxSpawn			Spawn particles in a collection of boxes
				CFXLayerNoise				Apply noise on particles
				CFXLayerPath				Follow path
				CFXDataCollect				Collect setup/init data
				CFXDataObject				Base fx object
				CFXDataShader				Shader object
				CFXDataRenderTarget			Render target object
				CFXDataBeam					Beam object
				CFXDataLight				Light object
				CFXDataWallmark				Wallmark object
				CFXDataModel				Model object
				CFXDataQuad					Quad object
				CFXDataCone					Cone object
				CFXDataParticlesOpt			ParticlesOpt wrapped
				CFXDataBeamStrip			Beam strip object
				CFXDataParticleHook			Hook into ParticlesOpt class for layer rendering
				CFXDataFXParticle			FXParticle containing hook and layers
				CEffectSystemHistory		History object
				CXR_Model_EffectSystem		Main model taking care of all data objects

Comments:

History:
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WModel_EffectSystem.h"
#include "WModel_EffectSystem.h"
#include "../WObj_Misc/WObj_EffectSystem.h"
//#include "../WObj_Misc/WObj_TentacleSystem_ClientData.h"
#include "../../../../Shared/mos/Classes/GameWorld/Client/WClient.h"
#include "../../../../Shared/mos/Classes/GameWorld/WObjCore.h"
#include "../../../../Shared/MOS/XR/XREngineImp.h"
#include "../../../../Shared/MOS/XR/XREngineVar.h"

/*************************************************************************************************\
¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
Shader program:

	HeatHaze:		Heat haze using normal map without mask

					Parameters:
						FragmentMap0	$FrameBuffer
						FragmentMap1	NormalMap
						FragmentParam0	Speed, Distortion, Not in use, Not in use


	HeatHazeMask:	Heat haze with mask and normal map

					Parameters:
						FragmentMap0	$FrameBuffer
						FragmentMap1	NormalMap
						FragmentMap2	Mask
						FragmentParam0	Speed, Distortion, Not in use, Not in use
___________________________________________________________________________________________________
\*************************************************************************************************/
bool CFXSysUtil::ms_bInit = false;

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_EffectSystem, CXR_Model_Custom);

// Statics
uint16 CXR_Model_EffectSystem::m_lQuadCrossTriangles[12] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };
bool CXR_Model_EffectSystem::m_bStaticsInitialized = false;
fp32 CXR_Model_EffectSystem::m_lQuadCrossTVertices[5][2] = { {0,0}, {0,0}, {0,0}, {0,0}, {0,0} };
uint16 CXR_Model_EffectSystem::m_lBoxTriangles[36] = { 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0 };

static const char* g_lpEFFECTSYSTEM_FLAGS[] = { "timemodecontinuous", "timemodecontrolled", "timemodecontinuouscontrolled", "__not_used__hashistory__", "__not_used__isalive__", "__not_used_hascollision__", "__not_used_0x0040__", NULL };

static const char* g_lpEFFECTSYSTEM_QUAD_FLAGS[] = { "Billboard", "PlaneXY", "PlaneXZ", "PlaneYZ", "Height", "EdgeFade", "AllowHistory", "Width2", NULL }; // "This_Is_A_No_No", NULL };
static const char* g_lpEFFECTSYSTEM_QUAD_RANDFLAGS[] = { "Surface", "Width", "RotateAngle", "Color", "BaseRotateAngle", NULL };

static const char* g_lpEFFECTSYSTEM_CONE_FLAGS[] = { "PlaneXY", "PlaneXZ", "PlaneYZ", "Spheroid", "AllowHistory", NULL };
static const char* g_lpEFFECTSYSTEM_CONE_RANDFLAGS[] = { "Radius", "Surface", "BaseRotateAngle", NULL };

static const char* g_lpEFFECTSYSTEM_BEAMSTRIP_FLAGS[] = { "AllowHistory", "PlaneXY", "PlaneXZ", "PlanyYZ", NULL };
static const char* g_lpEFFECTSYSTEM_BEAMSTRIP_RANDFLAGS[] = { "Surface", "Width", "Segments", "StripHeight", "Color", "WidthNoise", "PosNoise", "Velocity", "TextureScroll", "VelocityNoise", "Seed", NULL };

static const char* g_lpEFFECTSYSTEM_BEAM_FLAGS[] = { "EdgeFade", "AllowHistory", "PlaneX", "PlaneY", "PlaneZ", "PlaneXYZ", "Multi", NULL };
static const char* g_lpEFFECTSYSTEM_BEAM_RANDFLAGS[] = { "Surface", "Width", "Length", "Color", "Direction", NULL };

static const char* g_lpEFFECTSYSTEM_FXPARTICLE_FLAGS[] = { "AllowHistory", "WorldCollide", NULL };
//static const char* lpEFFECTSYSTEM_FXPARTICLE_RANDFLAGS[] = { NULL };

static const char* g_lpEFFECTSYSTEM_LIGHT_FLAGS[] = { "NoShadows", "NoDiffuse", "NoSpecular", "AnimTime", "ExcludeOwner", NULL };

//static const char* g_lpEFFECTSYSTEM_SHADER_FLAGS[] = { "WriteZFirst", "NoAnimate", "PostProcess", "AdvancedSurface", "Transparent", "Opaque", "SurfaceTransparent", "SurfaceOpaque", NULL };
static const char* g_lpEFFECTSYSTEM_SHADER_FLAGS[] = { "WriteZFirst", "NoAnimate", "AdvancedSurface", NULL };
static const char* g_lpEFFECTSYSTEM_SHADER_TEXGEN[] = { "None", "ScreenCoord", "EmitDirection", NULL };

static const char* g_lpEFFECTSYSTEM_EnableDisable[] = { "__0x00000001__", "ZCompare", "ZWrite", "Blend", "Smooth", "Scissor", "Fog", "SeparateStencil", "__0x00000100__", "Perspective", "AlphaWrite", "Cull", "CullCW", "Clip", "Stencil", "ColorWrite", "SecondaryColor", "__0x00020000__", "PolygonOffset", "Lighting", NULL };
static const char* g_lpEFFECTSYSTEM_RasterMode[] = { "None", "AlphaBlend", "LightmapBlend", "Multiply", "Add", "AlphaAdd", "AlphaMultiply", "Multiply2", "MulAdd", "OneAlpha", "OneInvAlpha", "DestAlphaBlend", "DestAdd", "MulAddDestAlpha", NULL };

static const char* g_lpEFFECTSYSTEM_RenderType[] = { "None", "CreateConeScreenQuad", NULL };

static const char* g_lpEFFECTSYSTEM_Compare[] = { "__0__", "Never", "Less", "Equal", "LessEqual", "Greater", "NotEqual", "GreaterEqual", "Always", NULL };
static const char* g_lpEFFECTSYSTEM_Priority[] = { "Default", "PostProcess", "Transparent", "Opaque", "SurfaceTransparent", "SurfaceOpaque", NULL };
//static const char* g_lpEFFECTSYSTEM_Priority[] = { "Default", "LightPriority", "PostProcess", "AdvancedSurface", "Transparent", "Opaque", "SurfaceTransparent", "SurfaceOpaque", NULL };



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXSysUtil
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CFXSysUtil::CalcBoxScissor(const CXR_Engine* _pEngine, CRC_Viewport* _pVP, const CMat4Dfp32* _pW2V, const CBox3Dfp32& _Box, CScissorRect& _Scissor)
{
	CVec4Dfp32 BoxV[8];
	_Box.GetVerticesV4(BoxV);
	
	const CRct& ViewRect = _pVP->GetViewRect();
	const CVec2Dfp32 VPScale(_pVP->GetXScale() * 0.5f, _pVP->GetYScale() * 0.5f);
	const CVec2Dfp32 VPMid((ViewRect.p0.x + ViewRect.p1.x) >> 1, (ViewRect.p0.y + ViewRect.p1.y) >> 1);
	const CXR_ViewContext* pVC = const_cast<CXR_Engine*>(_pEngine)->GetVC();
	const CMat4Dfp32* pM = (_pW2V) ? _pW2V : &pVC->m_W2VMat;

	vec128 m0 = pM->r[0];
	vec128 m1 = pM->r[1];
	vec128 m2 = pM->r[2];
	vec128 m3 = pM->r[3];
	M_VTranspose4x4(m0, m1, m2, m3);

	vec128 ScrExtMinVec = CVec4Dint32(ViewRect.p0.x, ViewRect.p0.y, ViewRect.p0.x, ViewRect.p0.y).v;
	vec128 ScrExtMaxVec = CVec4Dint32(ViewRect.p1.x, ViewRect.p1.y, ViewRect.p1.x, ViewRect.p1.y).v;

	vec128 zcomp = M_VScalar(0.1f);
	vec128 v0 = BoxV[0].v;
	vec128 v1 = BoxV[1].v;
	vec128 v2 = BoxV[2].v;
	vec128 v3 = BoxV[3].v;
	vec128 v4 = BoxV[4].v;
	vec128 v5 = BoxV[5].v;
	vec128 v6 = BoxV[6].v;
	vec128 v7 = BoxV[7].v;
	vec128 z03 = M_VDp4x4(v0, m2, v1, m2, v2, m2, v3, m2);
	vec128 z47 = M_VDp4x4(v4, m2, v5, m2, v6, m2, v7, m2);
	vec128 zmin = M_VMin(z03, z47);
	if (M_VCmpAnyLT(zmin, zcomp))
	{
		_Scissor.SetRect(ViewRect.p0.x, ViewRect.p0.y, ViewRect.p1.x, ViewRect.p1.y);
		return;
	}

	vec128 z03rcp = M_VRcp(z03);
	vec128 z47rcp = M_VRcp(z47);
	vec128 x03 = M_VMul(z03rcp, M_VDp4x4(v0, m0, v1, m0, v2, m0, v3, m0));
	vec128 x47 = M_VMul(z47rcp, M_VDp4x4(v4, m0, v5, m0, v6, m0, v7, m0));
	vec128 y03 = M_VMul(z03rcp, M_VDp4x4(v0, m1, v1, m1, v2, m1, v3, m1));
	vec128 y47 = M_VMul(z47rcp, M_VDp4x4(v4, m1, v5, m1, v6, m1, v7, m1));

	vec128 xmin1 = M_VMin(x03, x47);
	vec128 xmax1 = M_VMax(x03, x47);
	vec128 ymin1 = M_VMin(y03, y47);
	vec128 ymax1 = M_VMax(y03, y47);
	vec128 xmin2 = M_VMin(xmin1, M_VShuf(xmin1, M_VSHUF(2,3,0,1)));
	vec128 xmax2 = M_VMax(xmax1, M_VShuf(xmax1, M_VSHUF(2,3,0,1)));
	vec128 ymin2 = M_VMin(ymin1, M_VShuf(ymin1, M_VSHUF(2,3,0,1)));
	vec128 ymax2 = M_VMax(ymax1, M_VShuf(ymax1, M_VSHUF(2,3,0,1)));
	vec128 xymin1 = M_VMin(M_VMrgXY(xmin1, ymin1), M_VMrgZW(xmin1, ymin1));
	vec128 xymax1 = M_VMax(M_VMrgXY(xmax1, ymax1), M_VMrgZW(xmax1, ymax1));
	vec128 xymin2 = M_VMin(xymin1, M_VShuf(xymin1, M_VSHUF(2,3,0,1)));
	vec128 xymax2 = M_VMax(xymax1, M_VShuf(xymax1, M_VSHUF(2,3,0,1)));

	vec128 VPScaleVec = CVec4Dfp32(VPScale.k[0], VPScale.k[1], VPScale.k[0], VPScale.k[1]).v;
	vec128 VPMidVec = CVec4Dfp32(VPMid.k[0], VPMid.k[1], VPMid.k[0], VPMid.k[1]);
	vec128 xyminmax = M_VAdd(M_VMul(M_VSelMsk(M_VConstMsk(1,1,0,0), xymin2, xymax2), VPScaleVec), VPMidVec);

	vec128 xyminmax_u32 = M_VCnv_f32_i32(M_VAdd(xyminmax, M_VHalf()));
	vec128 xyminmax2_u32 = M_VMin_i32(M_VMax_i32(xyminmax_u32, ScrExtMinVec), ScrExtMaxVec);
#ifdef CPU_BIGENDIAN
	vec128 shuf = M_VShuf(xyminmax2_u32, M_VSHUF(1,0,3,2));
#else
	vec128 shuf = xyminmax2_u32;
#endif
	vec128 final = M_VCnvS_i32_i16(shuf, shuf);
	M_VStAny64(final, &_Scissor);
}


void CFXSysUtil::TransformAABB(const CMat4Dfp32& _Mat, const CBox3Dfp32& _Box, CBox3Dfp32& _DestBox)
{
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


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CSpline_Beam
|__________________________________________________________________________________________________
\*************************************************************************************************/
CSpline_Beam::CSpline_Beam(const uint16* _pSurfaceID, const int _nSurfaceID, const int _nPoints)
: m_nCachePoints(0)
, m_nPoints(0)
, m_bCalcedVBMem(0)
, m_bFade(0)
, m_UOffset(0)
{
	const int nPoints = _nPoints << 1;
	m_lSegments.SetLen(_nSurfaceID);
	m_lPoints.SetLen(nPoints);

	TAP<Segment> lSegments = m_lSegments;
	for (int i = 0; i < lSegments.Len(); i++)
	{
		Segment& Seg = lSegments[i];
		Seg.m_liPoints.SetLen(_nPoints);
		Seg.m_lProperties.SetLen(_nPoints * 3);
		Seg.m_nPoints = 0;
		Seg.m_SurfaceID = _pSurfaceID[i];
		Seg.m_pVB = NULL;
	}
}


bool CSpline_Beam::IsEmpty() const
{
	// returns whether or not the spline has any real segments
	return (m_nPoints <= 1);
}


void CSpline_Beam::SetSpline(const fp32 _Width, const fp32 _Length, const fp32 _UOffset, const CVec3Dfp32& _Pos1, const CVec3Dfp32& _Pos2, const fp32 _LengthRcp, const CVec3Dfp32& _Tang1, const CVec3Dfp32& _Tang2, const int _iSeg)
{
	Segment& Seg = m_lSegments[_iSeg];
	int iSegProp = Seg.m_nPoints * 3;
	Seg.m_liPoints[Seg.m_nPoints++] = m_nPoints;
	Seg.m_lProperties[iSegProp++] = _Width;
	Seg.m_lProperties[iSegProp++] = _Length;
	Seg.m_lProperties[iSegProp++] = _UOffset;

	Point& P1 = m_lPoints[m_nPoints++];
	Point& P2 = m_lPoints[m_nPoints++];

	// First point
	P1.m_Pos = CVec4Dfp32(_Pos1.k[0], _Pos1.k[1], _Pos1.k[2], 0.0f);
	P1.m_TangentIn = CVec4Dfp32(_Tang1.k[0], _Tang1.k[1], _Tang1.k[2], 0.0f);
	P1.m_TangentOut = CVec4Dfp32(_Tang1.k[0], _Tang1.k[1], _Tang1.k[2], 0.0f);

	// Second point
	P2.m_Pos = CVec4Dfp32(_Pos2.k[0], _Pos2.k[1], _Pos2.k[2], _LengthRcp);
	P2.m_TangentIn = CVec4Dfp32(_Tang2.k[0], _Tang2.k[1], _Tang2.k[2], 0.0f);
	P2.m_TangentOut = CVec4Dfp32(_Tang2.k[0], _Tang2.k[1], _Tang2.k[2], 0.0f);
}


void CSpline_Beam::CalcVBMem(CFXVBMAllocUtil* _pVBMHelp, const int _nCachePoints)
{
	if (!m_bCalcedVBMem)
	{
		const uint32 nV = FXAlign4(_nCachePoints  << 1);
		_pVBMHelp->Store_Alloc(0, 0, 1, nV, nV, nV);

		TAP<Segment> lSegments = m_lSegments;
		for (int i = 0; i < lSegments.Len(); i++)
		{
			int nIntSeg = lSegments[i].m_nPoints;
			if (nIntSeg > 0)
			{
				_pVBMHelp->Alloc_VB(CXR_VB_ATTRIB);
				_pVBMHelp->Alloc_Store(nIntSeg);
				_pVBMHelp->Alloc_SurfaceKeyFrame();
			}
		}

		_pVBMHelp->Alloc_M4();
		m_bCalcedVBMem = 1;
	}
}


bool CSpline_Beam::Alloc_VBMem(CFXVBMAllocUtil* _pVBMHelp, CXR_VBManager* _pVBM, const CMat4Dfp32& _W2V, const int _nCachePoints)
{
	const int nPrim = (_nCachePoints - 1) << 1;
	const int nV = _nCachePoints  << 1;
	//CMat4Dfp32* pVMove = _pVBM->Alloc_M4();
	CMat4Dfp32* pVMove = _pVBMHelp->Get_M4(_W2V);
	if (!pVMove)
		return false;

	TAP<Segment> lSegments = m_lSegments;
	for (int i = 0; i < lSegments.Len(); i++)
	{
		int nIntSeg = lSegments[i].m_nPoints;
		if (nIntSeg <= 0)
			continue;

		CXR_VertexBuffer* pVB = _pVBMHelp->Get_VB(CXR_VB_ATTRIB);

		// Setup VB Chains for this blood spline
		CXR_VBChain* pLastVBChain = NULL;
		for (int j = 0; j < nIntSeg; j++)
		{
			CXR_VBChain* pVBChain = _pVBMHelp->Get_VBChain(CXR_VB_VERTICES | CXR_VB_COLORS | CXR_VB_TVERTICES0, nV);

			// Initialize vertex buffer chain
			pVBChain->Render_IndexedTriangles((uint16*)CXR_Util::m_lBeamStripTriangles, nPrim);

			// Chain linkage
			pVBChain->m_pNextVB		= pLastVBChain;
			pLastVBChain			= pVBChain;
		}

		// Initialize vertex buffer
		pVB->Matrix_Set(pVMove);
		pVB->SetVBChain(pLastVBChain);

		lSegments[i].m_pVB = pVB;
		lSegments[i].m_pSurfaceKeyFrame = _pVBMHelp->Get_SurfaceKeyFrame();
	}

	return true;
}


bool CSpline_Beam::Finalize(CFXVBMAllocUtil* _pVBMHelp, CXR_VBManager* _pVBM, const CMat4Dfp32& _W2V, bool _bUVRot, const int _nCachePoints)
{
	// Calculate beams
	M_ASSERT(_nCachePoints <= MaxCachePoints, "");

	if (IsEmpty())
		return false;

	// Allocate memory and fetch some pointers
	if (!Alloc_VBMem(_pVBMHelp, _pVBM, _W2V, _nCachePoints))
		return false;

	vec128 W2Vx = _W2V.r[0];
	vec128 W2Vy = _W2V.r[1];
	vec128 W2Vz = _W2V.r[2];
	vec128 W2Vw = _W2V.r[3];
	vec128 VInvPos = M_VDp3x3(W2Vw, W2Vx, W2Vw, W2Vy, W2Vw, W2Vz);

	bool bFade = GetFade();

	TAP<Segment> lSegments = m_lSegments;
	for (int iSeg = 0; iSeg < lSegments.Len(); iSeg++)
	{
		Segment& Seg = lSegments[iSeg];
		if (Seg.m_nPoints <= 0)
			continue;

		CXR_VBChain* pVBChain = (CXR_VBChain*)Seg.m_pVB->m_pVBChain;
		for (int i = 0; i < Seg.m_nPoints; i++)
		{
			fp32* M_RESTRICT pV = (fp32*)pVBChain->m_pV;
			fp32* M_RESTRICT pTV = pVBChain->m_pTV[0];
			int32* M_RESTRICT pC = (int32*)pVBChain->m_pCol;

			uint iProp = i * 3;
			uint iPoint = Seg.m_liPoints[i];

			Point& P1 = m_lPoints[iPoint++];
			Point& P2 = m_lPoints[iPoint++];

			// Width, Length, UOffset
			vec128 VProperties = M_VSelMsk(M_VConstMsk(1,1,0,0), 
				M_VSelMsk(M_VConstMsk(1,0,1,0), M_VLdScalar(Seg.m_lProperties[iProp]), M_VLdScalar(Seg.m_lProperties[iProp + 1])),
				M_VZero());
			vec128 VLen = M_VLen3x3(M_VSub(P2.m_Pos, P1.m_Pos), P1.m_TangentIn, P2.m_TangentIn);
			VLen = M_VSelMsk(M_VConstMsk(1,0,0,1), VLen, M_VMul(M_VSplat(VLen,0), VLen));

			vec128 VTemp0 = M_VMul(M_VShuf(VLen, M_VSHUF(1,1,2,2)),
				M_VRsq(M_VDp3x4(P1.m_TangentIn, P1.m_TangentIn, P1.m_TangentOut, P1.m_TangentOut, P2.m_TangentIn, P2.m_TangentIn, P2.m_TangentOut, P2.m_TangentOut)));

			P1.m_TangentIn	= M_VMul(M_VSplat(VTemp0, 0), P1.m_TangentIn);
			P1.m_TangentOut	= M_VMul(M_VSplat(VTemp0, 1), P1.m_TangentOut);
			P2.m_TangentIn	= M_VMul(M_VSplat(VTemp0, 2), P2.m_TangentIn);
			P2.m_TangentOut	= M_VMul(M_VSplat(VTemp0, 3), P2.m_TangentOut);

			VTemp0 = M_VSelMsk(M_VConstMsk(1,0,0,0), M_VMin(M_VMul(M_VSplatY(VProperties), M_VRcp(M_VSplat(VLen, 0))), M_VOne()),
				M_VRcp(M_VCnv_i32_f32(M_VLdScalar_i32(int32(_nCachePoints - 1)))));
			VTemp0 = M_VSelMsk(M_VConstMsk(1,1,0,0), VTemp0, M_VMul(M_VSplatW(P2.m_Pos), M_VMul(M_VSplat(VTemp0, 0), M_VSplat(VTemp0, 1))));

			//vec128 VWidth = M_VSelMsk(M_VConstMsk(1,0,1,1), M_VLdScalar(Width1), M_VLdScalar(Width2));;
			vec128 VColor = M_VLdScalar_u32(0xffffffff);
			vec128 VColorSub = M_VLdScalar_u32(0x0);
			vec128 VMaskUV = M_VSelMsk(M_VConstMsk(1,0,0,1), M_VOne(), M_VZero());
			vec128 VttYY = M_VSelMsk(M_VConstMsk(1,0,1,1), M_VOne(), M_VSplat(VTemp0,2));
			VttYY = M_VSelMsk(M_VConstMsk(1,1,0,1), VttYY, M_VAdd(M_VLdScalar(Seg.m_lProperties[iProp + 2]), M_VOne()));
			VttYY = M_VSelMsk(M_VConstMsk(1,1,1,0), VttYY, M_VSplat(VTemp0,1));

			vec128 VMskUV = (_bUVRot) ? M_VConstMsk(0,1,0,1) : M_VConstMsk(1,0,1,0);

			// Make sure last entry is zero
			uint32 Color0 = 0xffffffff;
			uint32 Color1 = 0xffffffff;
			uint32 ColorSub = 0;
			if (bFade)
			{
				ColorSub = ((255 / (_nCachePoints-1)) << 24);
				Color0 = 0x00ffffff;
				Color1 = 0x00ffffff + ColorSub;
				VColor = M_VSelMsk(M_VConstMsk(1,1,0,0), M_VLdScalar_u32(Color0), M_VLdScalar_u32(Color1));
			}

			for (int iCache = 0; iCache < _nCachePoints; iCache += 2)
			{
				vec128 VPos1 = CalcPosVec128(iSeg, i, M_VSplatX(VttYY));
				vec128 VPos2 = CalcPosVec128(iSeg, i, M_VSplatX(M_VSub(VttYY, M_VSplatY(VttYY))));

				// Calculate direction and (up*width)
				vec128 VDir = M_VSub(VPos2, VPos1);
				vec128 VUp1 = M_VXpd(VDir, M_VAdd(VPos1, VInvPos));
				vec128 VUp2 = M_VXpd(VDir, M_VAdd(VPos2, VInvPos));
				VUp1 = M_VMul(VUp1, M_VMul(M_VSplatX(VProperties), M_VLenrcp3_Est(VUp1)));
				VUp2 = M_VMul(VUp2, M_VMul(M_VSplatX(VProperties), M_VLenrcp3_Est(VUp2)));

				// Store 4 vertices
				M_VSt_V3x4(pV, M_VAdd(VPos1, VUp1), M_VSub(VPos1, VUp1), M_VAdd(VPos2, VUp2), M_VSub(VPos2, VUp2));
				pV += 12;

				// Store 4 color values
				*((vec128 * M_RESTRICT)pC) = VColor;
				pC += 4;

				// Store 4 texture coordinate info			
				*((vec128 * M_RESTRICT)pTV) = M_VSelMsk(VMskUV, M_VSplat(VttYY,2), VMaskUV); pTV += 4;
				*((vec128 * M_RESTRICT)pTV) = M_VSelMsk(VMskUV, M_VSub(M_VSplat(VttYY,2), M_VSplat(VttYY,3)), VMaskUV); pTV += 4;

				// Decrease texture u offset and spline t position
				VttYY = M_VSub(VttYY, M_VSelMsk(M_VConstMsk(1,0,1,0), M_VMul(M_VShuf(VttYY, M_VSHUF(1,0,3,2)), M_VTwo()), M_VZero()));

				Color0 = Color1 + ColorSub;
				Color1 = Color0 + ColorSub;
				VColor = M_VSelMsk(M_VConstMsk(1,1,0,0), M_VLdScalar_u32(Color0), M_VLdScalar_u32(Color1));
			}

			// Make sure last entry is zero
			if (bFade && _nCachePoints > 0)
			{
				uint i = (_nCachePoints << 1);
				pVBChain->m_pCol[i-1] = 0xffffffff;
				pVBChain->m_pCol[i-2] = 0xffffffff;
			}

			pVBChain = pVBChain->m_pNextVB;
		}
	}

	m_nCachePoints = _nCachePoints;
	return true;
}


vec128 CSpline_Beam::CalcPosVec128(const int _iSeg, const int _iLocal, vec128 _Time)
{
	//const int iPoint = _iSeg >> 1;
	const int iPoint = m_lSegments[_iSeg].m_liPoints[_iLocal];
	const Point& P1 = m_lPoints[iPoint];
	const Point& P2 = m_lPoints[iPoint + 1];

	// Calculate t, t*t and t*t*t		{ t, t2, t3, t3 }
	_Time = M_VSelMsk(M_VConstMsk(1,0,0,0), _Time, M_VMul(_Time, _Time));
	_Time = M_VSelMsk(M_VConstMsk(1,1,0,0), _Time, M_VMul(_Time, M_VSplat(_Time, 0)));

	// calculate h { h0, h1, h2, h3 }
	vec128 Vh = M_VAdd(M_VMul(M_VSplatZ(_Time), M_VConst( 2.0f, -2.0f,  1.0f,  1.0f)), M_VMul(M_VSplatY(_Time), M_VConst(-3.0f,  3.0f, -2.0f, -1.0f)));
	Vh = M_VAdd(M_VAdd(Vh, M_VConst(1.0f,0.0f,0.0f,0.0f)), M_VSelMsk(M_VConstMsk(1,1,0,1), M_VZero(), M_VSplatX(_Time)));

	// Calculate and return result
	return M_VAdd(M_VMAdd(M_VSplat(Vh,0), P1.m_Pos, M_VMul(M_VSplat(Vh,1), P2.m_Pos)), M_VMAdd(M_VSplat(Vh,2), P1.m_TangentOut, M_VMul(M_VSplat(Vh,3), P2.m_TangentIn)));
}


void CSpline_Beam::Render(CXR_SurfaceContext* _pSC, CXR_Engine* _pEngine, CXR_VBManager* _pVBM, CXR_RenderInfo* _pRenderInfo, const CMTime& _AnimTime, uint _Flags, CXR_RenderSurfExtParam* _pParam)
{
	TAP<Segment> lSegments = m_lSegments;
	for (int i = 0; i < lSegments.Len(); i++)
	{
		CXR_VertexBuffer* pVB = lSegments[i].m_pVB;
		if (!pVB)
			continue;

		CXW_Surface* pSurface = _pSC->GetSurface(lSegments[i].m_SurfaceID);
		pSurface = (pSurface) ? pSurface->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps) : NULL;
		CXW_SurfaceKeyFrame* pSurfaceKey = (pSurface) ? pSurface->GetFrame(0, _AnimTime, lSegments[i].m_pSurfaceKeyFrame) : NULL;
		if (pSurface && pSurfaceKey)
		{
			int Flags = RENDERSURFACE_MATRIXSTATIC_M2V;
			fp32 BasePrio = (pSurface->m_Flags & XW_SURFFLAGS_TRANSPARENT) ?
				_pRenderInfo->m_BasePriority_Transparent : _pRenderInfo->m_BasePriority_Opaque;

			pVB->m_Priority = BasePrio;
			if ((pSurface->m_Flags & XW_SURFFLAGS_LIGHTING) && (_Flags & BEAMSPLINE_FLAGS_LIGHTING) && _pParam)
			{
				Flags |= (RENDERSURFACE_DEPTHFOG | RENDERSURFACE_LIGHTNONORMAL);
				CXR_Util::Render_Surface(Flags, _AnimTime, pSurface, pSurfaceKey, _pEngine, _pVBM, (CMat4Dfp32*) NULL,
					(CMat4Dfp32*) NULL, pVB->m_pTransform, pVB, BasePrio, 0.0001f, _pParam);
			}
			else
			{
				CXR_Util::Render_Surface(Flags, _AnimTime, pSurface, pSurfaceKey, _pEngine, _pVBM, (CMat4Dfp32*) NULL,
					(CMat4Dfp32*) NULL, pVB->m_pTransform, pVB, BasePrio);
			}
		}
	}
}


#ifndef M_RTM
void CSpline_Beam::Debug_Render()
{
	// Render spline
	MACRO_GetRegisterObject(CWireContainer, pWC, "GAMECONTEXT.CLIENT.WIRECONTAINER");
	if (!pWC)
		return;
	/*
	const int nSegments = m_nSegments;
	const int nCache = m_nCachePoints;
	const TAP_RCD<CXR_BeamStrip> lBeamStrips = m_lBeamStrips;

	int iBeamStrip = 1;
	for (int iSeg = 0; iSeg < nSegments; iSeg++)
	{
	CVec3Dfp32 PrevPos = lBeamStrips[iBeamStrip++].m_Pos;
	Debug_RenderVertexSize(pWC, PrevPos, 0xff00ff00, 1.0f);
	for (int iCache = 1; iCache < nCache; iCache++)
	{
	CVec3Dfp32 Pos = lBeamStrips[iBeamStrip++].m_Pos;
	pWC->RenderWire(PrevPos, Pos, 0xffff0000, 0.0f, false);
	Debug_RenderVertexSize(pWC, Pos, 0xff00ff00, 1.0f);
	PrevPos = Pos;
	}

	iBeamStrip++;
	}
	*/
}


void CSpline_Beam::Debug_RenderVertexSize(CWireContainer* _pWC, const CVec3Dfp32& _Pos, const CPixel32& _Color, const fp32 _Size, const fp32 _Duration, const bool _bFade)
{
	//	_pWC->RenderWire(_Pos - CVec3Dfp32(_Size,0,0), _Pos + CVec3Dfp32(_Size,0,0), _Color, _Duration, _bFade);
	//	_pWC->RenderWire(_Pos - CVec3Dfp32(0,_Size,0), _Pos + CVec3Dfp32(0,_Size,0), _Color, _Duration, _bFade);
	//	_pWC->RenderWire(_Pos - CVec3Dfp32(0,0,_Size), _Pos + CVec3Dfp32(0,0,_Size), _Color, _Duration, _bFade);
}
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXLayer / CFXLayerData
|__________________________________________________________________________________________________
\*************************************************************************************************/
CSpline_BeamStrip::CSpline_BeamStrip(uint _nStrips)
	: m_bVBMem(0)
{
	m_lStrips.SetLen(_nStrips);
}


void CSpline_BeamStrip::AddBeamData(CSpline_BeamStripData& _BeamData, uint _iStrip)
{
	m_lStrips[_iStrip].m_lBeamData.Add(_BeamData);
}


void CSpline_BeamStrip::AddBeamData(const CVec3Dfp32& _Pos, const CVec3Dfp32& _Tng, const CPixel32 _Color, fp32 _Width, fp32 _UOffset, uint _iStrip)
{
	m_lStrips[_iStrip].m_lBeamData.Add(CSpline_BeamStripData(_Pos, _Tng, _Color, _Width, _UOffset));
}


void CSpline_BeamStrip::SetSurface(uint _iStrip, uint16 _SurfaceID)
{
	m_lStrips[_iStrip].m_SurfaceID = _SurfaceID;
}


void CSpline_BeamStrip::RotateTexCoord(uint _iStrip, bool _bRotate)
{
	m_lStrips[_iStrip].m_TexCoordRotate = (_bRotate) ? 1 : 0;
}


void CSpline_BeamStrip::VBMem_Calculate(CFXVBMAllocUtil* _pVBMHelp, const uint _nCachePoints)
{
	if (!m_bVBMem)
	{
		_pVBMHelp->Alloc_M4();

		uint nStrips = m_lStrips.Len();
		for (uint i = 0; i < nStrips; i++)
		{
			CStrip& Strip = m_lStrips[i];
			uint nBeams = Strip.m_lBeamData.Len();
			if (nBeams > 1)
			{
				//uint nV = FXAlign4((((_nCachePoints-1) * (nBeams-1)) + 1) << 1);
				uint nV = FXAlign4((_nCachePoints * (nBeams-1)) << 1);
				_pVBMHelp->Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES | CXR_VB_COLORS | CXR_VB_TVERTICES0, nV);
				_pVBMHelp->Alloc_SurfaceKeyFrame();
			}
		}

		m_bVBMem = 1;
	}
}


bool CSpline_BeamStrip::VBMem_Collect(CFXVBMAllocUtil* _pVBMHelp, const CMat4Dfp32& _W2V, const uint _nCachePoints)
{
	if (m_bVBMem)
	{
		CXR_Util::Init();

		CMat4Dfp32* pW2V = _pVBMHelp->Get_M4(_W2V);

		uint nStrips = m_lStrips.Len();
		for (uint iStrip = 0; iStrip < nStrips; iStrip++)
		{
			CStrip& Strip = m_lStrips[iStrip];
			uint nBeams = Strip.m_lBeamData.Len();
			if (nBeams > 1)
			{
				uint nV = (_nCachePoints * (nBeams-1)) << 1;
				uint nP = ((_nCachePoints-1) * (nBeams-1)) << 1;
				uint nV4 = FXAlign4(nV);
				CXR_VertexBuffer* pVB = _pVBMHelp->Get_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES | CXR_VB_COLORS | CXR_VB_TVERTICES0, nV);
				CXR_VBChain* pVBChain = pVB->GetVBChain();
				
				pVB->Matrix_Set(pW2V);
				pVB->Render_IndexedTriangles((uint16*)CXR_Util::m_lBeamStripTriangles, nP);
				pVBChain->m_nV = nV;
				
				Strip.m_pVB = pVB;
				Strip.m_pSurfaceKeyFrame = _pVBMHelp->Get_SurfaceKeyFrame();
			}
		}

		return true;
	}
	return false;
}


bool CSpline_BeamStrip::Finalize(CFXVBMAllocUtil* _pVBMHelp, const CMat4Dfp32& _W2V, const uint _nCachePoints)
{
	M_ASSERT(_nCachePoints <= MaxCachePoints, "CSpline_BeamStrip::Finalize: Too many interpolator points");

	if (!VBMem_Collect(_pVBMHelp, _W2V, _nCachePoints))
		return false;

	vec128 W2Vx = _W2V.r[0];
	vec128 W2Vy = _W2V.r[1];
	vec128 W2Vz = _W2V.r[2];
	vec128 W2Vw = _W2V.r[3];
	vec128 VInvPos = M_VDp3x3(W2Vw, W2Vx, W2Vw, W2Vy, W2Vw, W2Vz);

	uint nStrips = m_lStrips.Len();
	for (uint iStrip = 0; iStrip < nStrips; iStrip++)
	{
		CStrip& Strip = m_lStrips[iStrip];
		uint nBeams = Strip.m_lBeamData.Len();
		if (nBeams > 1)
		{
			int32 nPoints = ((_nCachePoints-1) * (nBeams-1)) + 1;

			// Fetch pointer data
			CXR_VBChain* pVBChain = Strip.m_pVB->GetVBChain();
			fp32* M_RESTRICT pV = (fp32*)pVBChain->m_pV;
			fp32* M_RESTRICT pTV = pVBChain->m_pTV[0];
			int32* M_RESTRICT pC = (int32*)pVBChain->m_pCol;

			// Create Vec4 position and tangent from Vec3
			CVec4Dfp32 Pos0 = CFXSysUtil::CreateVec4Dfp32(Strip.m_lBeamData[0].m_Pos);
			CVec4Dfp32 Tng0 = CFXSysUtil::CreateVec4Dfp32(Strip.m_lBeamData[0].m_Tng);
			CVec4Dfp32 Pos1 = 0;
			CVec4Dfp32 Tng1 = 0;

			// Fetch width, VWidth = Strip.m_lBeamData[0].m_Width * 0.5f;
			vec128 Vvvvv = M_VSelMsk(M_VConstMsk(1,0,0,1), M_VOne(), M_VZero());
			vec128 VWidth = M_VMul(M_VHalf(), M_VLdScalar(Strip.m_lBeamData[0].m_Width));
			//vec128 VColor = M_VLdScalar_u32(Strip.m_lBeamData[0].m_Color);
			vec128 Vuu = M_VLdScalar(Strip.m_lBeamData[0].m_UOffset);
			vec128 VPixel0 = M_VLd_Pixel32_f32(&Strip.m_lBeamData[0].m_Color);
			vec128 UVMsk = (Strip.m_TexCoordRotate) ? M_VConstMsk(0,1,0,1) : M_VConstMsk(1,0,1,0);
			uint32 Color = 0;
			for (uint iBeam = 1; iBeam < nBeams; iBeam++)
			{
				// Create Vec4 position and tangent to interpolate to from Vec3
				CSpline_BeamStripData& Beam1 = Strip.m_lBeamData[iBeam];
				Pos1 = CFXSysUtil::CreateVec4Dfp32(Beam1.m_Pos);
				Tng1 = CFXSysUtil::CreateVec4Dfp32(Beam1.m_Tng);
				
				// Fetch width, VWidth.y = Beam1.m_Width * 0.5f;
				VWidth = M_VSelMsk(M_VConstMsk(1,0,1,1), VWidth, M_VMul(M_VHalf(), M_VLdScalar(Beam1.m_Width)));
				//VColor = M_VSelMsk(M_VConstMsk(1,0,1,1), VWidth, M_VLdScalar_u32(Beam1.m_Color));
				Vuu = M_VSelMsk(M_VConstMsk(1,0,1,1), Vuu, M_VLdScalar(Beam1.m_UOffset));
				vec128 VPixel1 = M_VLd_Pixel32_f32(&Beam1.m_Color);
				
				// Calculate lengths, VLen = { |P1-P0|, |Tang0|, |Tang1|, 0 } 
				// Calculate t and it's step, Vttt = { 0, (1/nCachePointsT), (1/nCachePointsT)*2, 0 }
				int32 nCachePointT = _nCachePoints - MaxMT(0, int32(iBeam+2) - int32(nBeams));
				vec128 Vttt = M_VMul(M_VConst(0.0f, 1.0f, 2.0f, 0.0f), M_VRcp(M_VLdScalar_i32_f32(nCachePointT)));
				
				Tng0 = M_VSelMsk(M_VConstMsk(1,1,1,0), Tng0, M_VLen3(M_VSub(Pos1, Pos0)));

				// Run through cache points
				for (uint i = 0; i < _nCachePoints; i += 2)
				{
					// Calculate interpolated positions
					vec128 VPos0 = CalcPosVec128(Pos0, Pos1, Tng0, Tng1, M_VSplatX(Vttt));
					vec128 VPos1 = CalcPosVec128(Pos0, Pos1, Tng0, Tng1, M_VSplatY(Vttt));

					// Calculate interpolated width in zw, VWidth = { x, y, Lrp(x,y,Vttt.x), Lrp(x,y,Vttt.y) }
					VWidth = M_VSelMsk(M_VConstMsk(1,1,0,0), VWidth, M_VLrp(M_VSplatX(VWidth), M_VSplatY(VWidth), M_VShuf(Vttt, M_VSHUF(0,0,0,1))));
					//VColor = M_VSelMsk(M_VConstMsk(1,1,0,0), VColor, M_VLrp(M_VSplatX(VColor), M_VSplatY(VColor), M_VShuf(Vttt, M_VSHUF(0,0,0,1))));
					M_VSt_V4f32_Pixel32(M_VLrp(VPixel0, VPixel1, M_VShuf(Vttt, M_VSHUF(0,0,0,1))), &Color);
					Vuu = M_VSelMsk(M_VConstMsk(1,1,0,0), Vuu, M_VLrp(M_VSplatX(Vuu), M_VSplatY(Vuu), M_VShuf(Vttt, M_VSHUF(0,0,0,1))));
					
					// Calculate direction and up vector, up vector is scaled with width
					vec128 VDir = M_VSub(VPos0, VPos1);
					vec128 VUp0 =  M_VMul(M_VNrm3_Est(M_VXpd(VDir, M_VAdd(VPos0, VInvPos))), M_VSplatZ(VWidth));
					vec128 VUp1 = M_VMul(M_VNrm3_Est(M_VXpd(VDir, M_VAdd(VPos1, VInvPos))), M_VSplatW(VWidth));

					// Store 4 vertices (Pos0+Up0, Pos0-Up0, Pos1+Up1, Pos1-Up1)
					M_VSt_V3x4(pV, M_VAdd(VPos0, VUp0), M_VSub(VPos0, VUp0), M_VAdd(VPos1, VUp1), M_VSub(VPos1, VUp1)); pV += 12;

					// Store 4 color values
					*((vec128 * M_RESTRICT)pC) = M_VLdScalar_u32(Color); pC += 4;

					// Store 4 texture coordinates
					*((vec128 * M_RESTRICT)pTV) = M_VSelMsk(UVMsk, M_VSplatZ(Vuu), Vvvvv); pTV += 4;
					*((vec128 * M_RESTRICT)pTV) = M_VSelMsk(UVMsk, M_VSplatW(Vuu), Vvvvv); pTV += 4;

					// Increase t, { Vttt.x+Vttt.z, Vttt.y+Vttt.z, Vttt.z, 0 }
					Vttt = M_VAdd(Vttt, M_VShuf(Vttt, M_VSHUF(2,2,3,3)));
				}

				VWidth = M_VShuf(VWidth, M_VSHUF(1,0,2,3));
				//VColor = M_VShuf(VColor, M_VSHUF(1,0,2,3));
				VPixel0 = VPixel1;
				Vuu = M_VShuf(Vuu, M_VSHUF(1,0,2,3));

				Pos0 = Pos1;
				Tng0 = Tng1;
			}
		}
	}

	return true;
}


void CSpline_BeamStrip::Render(CXR_SurfaceContext* _pSC, CXR_Engine* _pEngine, CXR_VBManager* _pVBM, CXR_RenderInfo* _pRenderInfo, const CMTime& _AnimTime, uint _Flags, CXR_RenderSurfExtParam* _pParam)
{
	uint nStrips = m_lStrips.Len();
	TAP_RCD<CStrip> lStrips = m_lStrips;
	for (int i = 0; i < nStrips; i++)
	{
		CXR_VertexBuffer* pVB = lStrips[i].m_pVB;
		if (!pVB)
			continue;

		CXW_Surface* pSurface = _pSC->GetSurface(lStrips[i].m_SurfaceID);
		pSurface = (pSurface) ? pSurface->GetSurface(_pEngine->m_SurfOptions, _pEngine->m_SurfCaps) : NULL;
		CXW_SurfaceKeyFrame* pSurfaceKey = (pSurface) ? pSurface->GetFrame(0, _AnimTime, lStrips[i].m_pSurfaceKeyFrame) : NULL;
		if (pSurface && pSurfaceKey)
		{
			int Flags = RENDERSURFACE_MATRIXSTATIC_M2V;
			fp32 BasePrio = (pSurface->m_Flags & XW_SURFFLAGS_TRANSPARENT) ?
				_pRenderInfo->m_BasePriority_Transparent : _pRenderInfo->m_BasePriority_Opaque;

			pVB->m_Priority = BasePrio;
			/* Lighting support
			if ((pSurface->m_Flags & XW_SURFFLAGS_LIGHTING) && (_Flags & BEAMSPLINE_FLAGS_LIGHTING) && _pParam)
			{
				Flags |= (RENDERSURFACE_DEPTHFOG | RENDERSURFACE_LIGHTNONORMAL);
				CXR_Util::Render_Surface(Flags, pSurface, pSurfaceKey, _pEngine, _pVBM, (CMat4Dfp32*) NULL,
					(CMat4Dfp32*) NULL, pVB->m_pTransform, pVB, BasePrio, 0.0001f, _pParam);
			}
			else
			*/
			{
				CXR_Util::Render_Surface(Flags, _AnimTime, pSurface, pSurfaceKey, _pEngine, _pVBM, (CMat4Dfp32*) NULL,
					(CMat4Dfp32*) NULL, pVB->m_pTransform, pVB, BasePrio);
			}
		}
	}
}


vec128 CSpline_BeamStrip::CalcPosVec128(const CVec4Dfp32& _P0, const CVec4Dfp32& _P1, const CVec4Dfp32& _T0, const CVec4Dfp32& _T1, vec128 _Time)
{
	// Calculate t, t*t and t*t*t		{ t, t2, t3, t3 }
	_Time = M_VSelMsk(M_VConstMsk(1,0,0,0), _Time, M_VMul(_Time, _Time));
	_Time = M_VSelMsk(M_VConstMsk(1,1,0,0), _Time, M_VMul(_Time, M_VSplat(_Time, 0)));

	// calculate h { h0, h1, h2, h3 }
	vec128 Vh = M_VAdd(M_VMul(M_VSplatZ(_Time), M_VConst( 2.0f, -2.0f,  1.0f,  1.0f)), M_VMul(M_VSplatY(_Time), M_VConst(-3.0f,  3.0f, -2.0f, -1.0f)));
	Vh = M_VMul(M_VAdd(M_VAdd(Vh, M_VConst(1.0f,0.0f,0.0f,0.0f)), M_VSelMsk(M_VConstMsk(1,1,0,1), M_VZero(), M_VSplatX(_Time))),
				M_VSelMsk(M_VConstMsk(0,0,1,1), M_VSplatW(_T0), M_VOne()));

	// Calculate and return result { x,y,z,0 }
	return M_VSelMsk(M_VConstMsk(1,1,1,0), M_VAdd(M_VMAdd(M_VSplat(Vh,0), _P0, M_VMul(M_VSplat(Vh,1), _P1)), M_VMAdd(M_VSplat(Vh,2), _T0, M_VMul(M_VSplat(Vh,3), _T1))), M_VZero());
}


#ifndef M_RTM
void CSpline_BeamStrip::Debug_Render(CWireContainer* _pWC)
{
	uint nStrips = m_lStrips.Len();
	CMat4Dfp32 UnitMat; UnitMat.Unit();
	for (uint iStrip = 0; iStrip < nStrips; iStrip++)
	{
		CStrip& Strip = m_lStrips[iStrip];
		uint nBeams = Strip.m_lBeamData.Len();

		/*
		CSpline_Vec3Dfp32 Spline;
				
		for (uint iBeam = 0; iBeam < nBeams; iBeam++)
		{
			UnitMat.GetRow(3) = Strip.m_lBeamData[iBeam].m_Pos;
			Spline.AddPoint(UnitMat, Strip.m_lBeamData[iBeam].m_Tng);
		}

		Spline.Finalize(CSpline_Vec3Dfp32::Segment::MaxCachePoints/2);
		for (uint iSeg = 0; iSeg < Spline.m_lSegments.Len(); iSeg++)
		{
			const CSpline_Vec3Dfp32::Segment& s = Spline.m_lSegments[iSeg];
			CVec3Dfp32 Pos, PrevPos;
			for (uint i = 0; i < Spline.m_nCachePoints; i++)
			{
				Spline.CalcPos(s.m_Cache[i].t, Pos);
				_pWC->RenderWire(Pos - CVec3Dfp32(0.5f,0,0), Pos + CVec3Dfp32(0.5f,0,0), 0xff00ff00, 0.0f, false);
				_pWC->RenderWire(Pos - CVec3Dfp32(0,0.5f,0), Pos + CVec3Dfp32(0,0.5f,0), 0xff00ff00, 0.0f, false);
				_pWC->RenderWire(Pos - CVec3Dfp32(0,0,0.5f), Pos + CVec3Dfp32(0,0,0.5f), 0xff00ff00, 0.0f, false);

				if (i > 0)
					_pWC->RenderWire(PrevPos, Pos, 0xffff00ff, 0.0f, false);
				PrevPos = Pos;
			}
		}

		for (uint iPoint = 0; iPoint < Spline.m_lPoints.Len(); iPoint++)
		{
			const CSpline_Vec3Dfp32::Point& p = Spline.m_lPoints[iPoint];
			_pWC->RenderVertex(p.m_Pos, 0xff800080, 0.0f, false);
		}
		*/

		// Debug render vertices
		uint nV = Strip.m_pVB->GetVBChain()->m_nV;
		if (nV > 1)
		{
			CVec3Dfp32* pV = Strip.m_pVB->GetVBChain()->m_pV;
			CVec3Dfp32 Last0 = pV[0];
			CVec3Dfp32 Last1 = pV[1];
			for (uint iV = 2; iV < nV;)
			{
				CVec3Dfp32& Cur0 = pV[iV++];
				CVec3Dfp32& Cur1 = pV[iV++];
				_pWC->RenderWire(Last0, Cur0, CPixel32(255,0,0,255), 0.0f, false);
				_pWC->RenderWire(Last1, Cur1, CPixel32(0,0,255,255), 0.0f, false);
				Last0 = Cur0;
				Last1 = Cur1;
			}
		}
	}
}
#endif


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXLayer / CFXLayerData
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFXLayer::CFXLayer()
{
	m_lpExtLayers.Clear();
}


CFXLayer::~CFXLayer()
{
	m_lpExtLayers.Clear();
}


CFXLayer& CFXLayer::operator = (const CFXLayer& _Layer)
{
	m_lpExtLayers.Add(_Layer.m_lpExtLayers);
	return *this;
}


void CFXLayer::GenerateRender(FXPerRenderVariables* M_RESTRICT _pPR, CFXLayerModelParticles::CBatch* M_RESTRICT _pBatch, CXR_Model_Custom_RenderParams* _pRenderParams, int _GenerateFlags)
{
	// Run layer operations
	TAP_RCD< TPtr<CFXLayerData> > lspLayerData = m_lpExtLayers;
	for(uint i = 0; i < lspLayerData.Len(); i++)
		lspLayerData[i]->GenerateRender(_pPR, _pBatch, _pRenderParams, _GenerateFlags);
}


void CFXLayer::GenerateDistribution(uint _MaxNumParticles, FXPerRenderVariables* M_RESTRICT _pPR, CFXLayerModelParticles::CBatch* M_RESTRICT _pBatch, int& _GenerateFlags)
{
	// Run layer operations
	TAP_RCD< TPtr<CFXLayerData> > lspLayerData = m_lpExtLayers;
	for(uint i = 0; i < lspLayerData.Len(); i++)
		lspLayerData[i]->GenerateDistribution(_MaxNumParticles, _pPR, _pBatch, _GenerateFlags);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXLayerTwirl
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CFXLayerTwirl::OnEvalKey(CStr& _Key)
{
	CStr UCase = _Key.GetStrSep(",").UpperCase();
	
	// Set components
	if(UCase == "X")
		m_iComponents = 0;
	else if (UCase == "Y")
		m_iComponents = (1 << 2);
	else if (UCase == "Z")
		m_iComponents = (2 << 2);

	UCase = _Key.GetStrSep(",").UpperCase();
	if(UCase == "Y")
		m_iComponents |= 1;
	else if(UCase == "Z")
		m_iComponents |= 2;

	// Set data
	m_Offset = (fp32)_Key.GetStrSep(",").Val_fp64();
	m_Speed  = (fp32)_Key.GetStrSep(",").Val_fp64();
	m_Radius = (fp32)_Key.GetStrSep(",").Val_fp64();
	m_Loops  = (fp32)_Key.GetStrSep(",").Val_fp64();
}


void CFXLayerTwirl::GenerateRender(CFXLayer::FXPerRenderVariables* M_RESTRICT _pPR, CFXLayer::FXBatch* M_RESTRICT _pBatch, CXR_Model_Custom_RenderParams* _pRenderParams, int _GenerateFlags)
{
	const int nParallelParticles = _pPR->m_NumParallelParticles;
	CVec4Dfp32* pPos = _pBatch->m_Pos;

	const uint8 Comp0 = m_iComponents >> 2;
	const uint8 Comp1 = m_iComponents & 0x3;

	fp32 L2W0 = _pBatch->m_LocalToWorld.GetRow(3).k[Comp0];
	fp32 L2W1 = _pBatch->m_LocalToWorld.GetRow(3).k[Comp1];

	fp32 sinv, cosv;
	const fp32* pTime = _pBatch->m_Time;
	const fp32* pTimeFrac = _pBatch->m_TimeFraction;

	for(int i = 0; i < nParallelParticles; i++)
	{
		fp32 Scale = pTimeFrac[i];
		const fp32 InvScale = 1.0f - Scale;

		QSinCosUnit(m_Offset + (pTime[i] * m_Speed) + (Scale * m_Loops), sinv, cosv);
		const fp32 SinAddCosRadius = (sinv + cosv) * m_Radius;
		const fp32 SinSubCosRadius = (sinv - cosv) * m_Radius;

		fp32& Pos0 = pPos[i].k[Comp0];
		fp32& Pos1 = pPos[i].k[Comp1];
		Pos0 = CFXSysUtil::LerpMT(Pos0 + SinAddCosRadius, L2W0 + (SinAddCosRadius * InvScale), Scale);
		Pos1 = CFXSysUtil::LerpMT(Pos1 + SinSubCosRadius, L2W1 + (SinSubCosRadius * InvScale), Scale);
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXLayerBoneBind
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CFXLayerBoneBind::GenerateDistribution(uint _MaxNumParticles, CFXLayer::FXPerRenderVariables* M_RESTRICT _pPR, CFXLayer::FXBatch* M_RESTRICT _pBatch, int& _GenerateFlags)
{
	if(!_pPR->m_pSkeletonInst || !_pPR->m_pSkeleton)
		return;

	CVec4Dfp32* pPos = _pBatch->m_Pos;
	const uint32* pRandSeed = _pBatch->m_iRandseed;
	const int nParticles = _pPR->m_NumParallelParticles;

	if(_pPR->m_pHistory)
	{
		CVec3Dfp32* pHistoryPos = _pPR->m_pHistory->GetPositionHistory();
		fp32* pHistoryTimeFrac = _pPR->m_pHistory->GetTimeFracHistory();
		if(!pHistoryPos && !pHistoryTimeFrac)
		{
			_pPR->m_pHistory->UseUnit(true);
			uint MaxNumParticles = 0;
			_pPR->m_pHistory->CreateFXHistory(_MaxNumParticles);
			pHistoryPos = _pPR->m_pHistory->GetPositionHistory();
			pHistoryTimeFrac = _pPR->m_pHistory->GetTimeFracHistory();
		}

		const uint32* pID = _pBatch->m_ID;
		const fp32* pTimeFrac = _pBatch->m_TimeFraction;
		for(int iParticle = 0; iParticle < nParticles; iParticle++)
		{
			const uint32 ID = pID[iParticle] % _MaxNumParticles;
			CVec3Dfp32& DistributedPoint = (CVec3Dfp32&)pPos[iParticle];
			if(pTimeFrac[iParticle] < pHistoryTimeFrac[ID])
			{
				GetDistributedPoint_InSkeleton(_pPR->m_SkeletonType, _pPR->m_pSkeleton, _pPR->m_pSkeletonInst, DistributedPoint, pRandSeed[iParticle]);
				pHistoryPos[ID] = DistributedPoint;
			}
			else
				DistributedPoint = pHistoryPos[ID];

			pHistoryTimeFrac[ID] = pTimeFrac[iParticle];
		}
	}
	else
	{
		for(int iParticle = 0; iParticle < nParticles; iParticle++)
		{
			CVec3Dfp32& DistributedPoint = (CVec3Dfp32&)pPos[iParticle];
			GetDistributedPoint_InSkeleton(_pPR->m_SkeletonType, _pPR->m_pSkeleton, _pPR->m_pSkeletonInst, DistributedPoint, pRandSeed[iParticle]);
		}
	}

	_GenerateFlags |= CFXDataFXParticleHook::GENERATE_FLAGS_POSSET;
}


void CFXLayerBoneBind::GetDistributedPoint_InSkeleton(uint _SkeletonType, CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInst, CVec3Dfp32& _point, uint32 _iRandSeed)
{
	uint16 iBone = 0;
	switch(_SkeletonType)
	{
	case 1:
		{
			iBone = (uint16)TruncToInt((MFloat_GetRand(_iRandSeed) * 20) + 1);
			if(iBone == 20)
				iBone = 21;
		}
		break;

	case 2:
		{
			iBone = (uint16)TruncToInt((MFloat_GetRand(_iRandSeed) * 28) + 1);
			if(iBone == 20)
				iBone = 21;
			else if(iBone > 21)
				iBone += 3;
		}
		break;

	default:
		break;
	}

	_point = (_pSkel->m_lNodes[iBone].m_LocalCenter * _pSkelInst->m_pBoneTransform[iBone]);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXLayerBoxSpawn
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CFXLayerBoxSpawn::GenerateDistribution(uint _MaxNumParticles, CFXLayer::FXPerRenderVariables* M_RESTRICT _pPR, CFXLayer::FXBatch* M_RESTRICT _pBatch, int& _GenerateFlags)
{
	if(!_pPR->m_pSpawnBoxes || _pPR->m_nSpawnBoxes <= 0)
		return;

	CVec4Dfp32* pPos = _pBatch->m_Pos;
	const uint32* pRandSeed = _pBatch->m_iRandseed;
	const int nParticles = _pPR->m_NumParallelParticles;

	if(_pPR->m_pHistory)
	{
		CVec3Dfp32* pHistoryPos = _pPR->m_pHistory->GetPositionHistory();
		fp32* pHistoryTimeFrac = _pPR->m_pHistory->GetTimeFracHistory();
		if(!pHistoryPos && !pHistoryTimeFrac)
		{
			_pPR->m_pHistory->UseUnit(true);
			uint MaxNumParticles = 0;
			_pPR->m_pHistory->CreateFXHistory(_MaxNumParticles);
			pHistoryPos = _pPR->m_pHistory->GetPositionHistory();
			pHistoryTimeFrac = _pPR->m_pHistory->GetTimeFracHistory();
		}

		const uint32* pID = _pBatch->m_ID;
		const fp32* pTimeFrac = _pBatch->m_TimeFraction;
		for(int iParticle = 0; iParticle < nParticles; iParticle++)
		{
			const uint32 ID = pID[iParticle] % _MaxNumParticles;
			CVec3Dfp32& DistributedPoint = (CVec3Dfp32&)pPos[iParticle];
			if(pTimeFrac[iParticle] < pHistoryTimeFrac[ID])
			{
				GetDistributedPoint_InBounding(_pPR->m_pSpawnBoxes, _pPR->m_nSpawnBoxes, DistributedPoint, pRandSeed[iParticle]);
				 DistributedPoint = (DistributedPoint * _pBatch->m_LocalToWorld);
				 pHistoryPos[ID] = DistributedPoint;
			}
			else
				DistributedPoint = pHistoryPos[ID];

			pHistoryTimeFrac[ID] = pTimeFrac[iParticle];
		}
	}
	else
	{
		for(int iParticle = 0; iParticle < nParticles; iParticle++)
		{
			CVec3Dfp32& DistributedPoint = (CVec3Dfp32&)pPos[iParticle];
			GetDistributedPoint_InBounding(_pPR->m_pSpawnBoxes, _pPR->m_nSpawnBoxes, DistributedPoint, pRandSeed[iParticle]);
			DistributedPoint = DistributedPoint * _pBatch->m_LocalToWorld;
		}
	}

	_GenerateFlags |= CFXDataFXParticleHook::GENERATE_FLAGS_POSSET;
}


void CFXLayerBoxSpawn::GetDistributedPoint_InBounding(CBox3Dfp32* _pBoxes, uint32 _nBoxes, CVec3Dfp32& _Point, uint32 _iRandSeed)
{
	int iBox = TruncToInt((MFloat_GetRand(_iRandSeed) * (_nBoxes-1)));
	_Point = CFXSysUtil::RandPointInBox(_pBoxes[iBox], _iRandSeed);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXLayerNoise
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CFXLayerNoise::OnEvalKey(CStr& _Key)
{
	m_bAnimated = _Key.GetStrSep(",").Val_int() ? true : false;
	m_Timescale = (fp32)_Key.GetStrSep(",").Val_fp64();
	m_Noise.ParseString(_Key.GetStrSep(","));
}


void CFXLayerNoise::EvaluateBeamStrip(CXR_BeamStrip* _pBeams, uint _nBeams, fp32 _AnimTime, const CMat4Dfp32& _WMat)
{
	uint32 BaseSeed = uint32(aint(this));
	//uint16 iRand = (uint16)((m_RandFlags & FXRAND_BEAMSTRIP_SEED) ? (RandSeed + (BaseSeed * (_pRD->m_Time1-_pRD->m_Time0))) : BaseSeed);
	uint16 iRand = BaseSeed;

	CVec3Dfp32 NoisePos = 0;
	fp32 NoiseX = m_Noise.k[0];
	fp32 NoiseY = m_Noise.k[1];
	fp32 NoiseZ = m_Noise.k[2];

	fp32 NoiseAnim = 1;
	if(m_bAnimated)
		NoiseAnim = QSin(_AnimTime * m_Timescale);

	for(int i = 0; i < _nBeams; i++)
	{
		NoisePos.k[0] = (-NoiseX + (NoiseX * MFloat_GetRand(iRand++) * 2.0f)) * NoiseAnim;
		NoisePos.k[1] = (-NoiseY + (NoiseY * MFloat_GetRand(iRand++) * 2.0f)) * NoiseAnim;
		NoisePos.k[2] = (-NoiseZ + (NoiseZ * MFloat_GetRand(iRand++) * 2.0f)) * NoiseAnim;

		_pBeams[i].m_Pos += NoisePos;
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXLayerPath
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CFXLayerPath::OnEvalKey(CStr& _Key)
{
	TAP<CVec3Dfp32> lPathNodes = m_lPath;
	for(uint i = 0; i < lPathNodes.Len(); i++)
		lPathNodes[i].ParseString(_Key.GetStrSep(","));
}


void CFXLayerPath::EvaluateBeamStrip(CXR_BeamStrip* _pBeams, uint _nBeams, fp32 _AnimTime, const CMat4Dfp32& _WMat)
{
	TAP_RCD<CVec3Dfp32> lPath = m_lPath;
	if (lPath.Len() == _nBeams)
	{
		const CVec3Dfp32& WMatPos = _WMat.GetRow(3);
		for (uint i = 0; i < _nBeams; i++)
		{
			_pBeams[i].m_Pos = lPath[i];
			_pBeams[i].m_Pos.MultiplyMatrix3x3(_WMat);
			_pBeams[i].m_Pos += WMatPos;
		}
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CEffectSystemRenderChain
|__________________________________________________________________________________________________
\*************************************************************************************************/
CEffectSystemRenderChain::CEffectSystemRenderChain(int _Seq, const CXR_RenderInfo& _RenderInfo, const CEffectSystemRenderData* _pRD, const CFXDataShader* _pShader, CXR_SurfaceContext* _pSurfaceContext, CXR_Engine* _pEngine, const CMTime& _AnimTime, CXR_VBChain* _pChain, CXR_VBManager* _pVBM, uint _SurfaceID, const CMat4Dfp32& _VMat, const fp32* _pPriority)
{
	//M_ASSERT((!(!_pShader && !_Surface)), "CEffectSystemRenderChain: At least one of Shader or Surface has to be specified!");

	// Add surface and shader for rendering if both specified
	m_Valid = false;
	if (_SurfaceID > 0)
		m_SurfaceKey.Create(_Seq, _pSurfaceContext, _pEngine, _AnimTime, _SurfaceID);

	if (!_pShader && !_SurfaceID) 
		return;

    if(_pChain)
	{
		m_Valid = true;
		m_VB.SetVBChain(_pChain);
		if (_SurfaceID > 0)
		{
			if(_pPriority)
			{
				// First thing that should happen during rendering
				m_VB.m_Priority = *_pPriority;
			}
			else
			{
				CXW_Surface* pSurface = m_SurfaceKey.m_pSurface;
				if(pSurface->m_Flags & XW_SURFFLAGS_TRANSPARENT)
					m_VB.m_Priority = _RenderInfo.m_BasePriority_Transparent;
				else
					m_VB.m_Priority = _RenderInfo.m_BasePriority_Opaque;

				#define PriorityBiasMightBeReadFromRegisterForEffectSystemSurfacesLongDefineName 0	// Not implemented ... yet =)
				m_VB.m_Priority += PriorityBiasMightBeReadFromRegisterForEffectSystemSurfacesLongDefineName * TransparencyPriorityBiasUnit;
				m_VB.m_Priority += pSurface->m_PriorityOffset;// * 0.001f;
			}

			// Fetch animation time
//			m_AnimTime = m_SurfaceKey.m_pKeyFrame->m_AnimTime;
//			m_AnimTime = _AnimTime;
//			m_AnimTimeWrapped = m_SurfaceKey.m_pKeyFrame->m_AnimTimeWrapped;
		}
		else
		{
//			m_AnimTime = _AnimTime;
//			m_AnimTimeWrapped = _AnimTime;
			m_Valid = false;
		}

		CMat4Dfp32* pVMat = _pVBM->Alloc_M4(_VMat);
		if(!pVMat) m_Valid = false;
		m_VB.Matrix_Set(pVMat);

		/*
		CRC_Attributes* pA = _pVBM->Alloc_Attrib();
		if(!pA)
			m_Valid = false;
		else
		{
			pA->SetDefault();
			pA->Attrib_Disable(_pShader->m_Disable);
			pA->Attrib_Enable(_pShader->m_Enable);
			pA->Attrib_RasterMode(_pShader->m_RasterMode);
		}
		*/

		if (_pShader)
		{
			if(_pShader->m_Flags & FXFLAG_SHADER_WRITEZFIRST)
			{
				CRC_Attributes* pA = _pVBM->Alloc_Attrib();
				if(pA)
				{
					CXR_VertexBuffer VBShaderZWrite;
					const fp32 Priority = _pShader->GetPriority(_pRD->m_pFXSystem->m_pRenderParams->m_RenderInfo);

					pA->SetDefault();
					pA->Attrib_Disable(CRC_FLAGS_COLORWRITE | CRC_FLAGS_ALPHAWRITE);

					VBShaderZWrite.CopyVBChain(&m_VB);
					VBShaderZWrite.m_pAttrib = pA;
					VBShaderZWrite.m_Priority = Priority - 0.001f;
					_pVBM->AddVB(VBShaderZWrite);
				}
			}

			CXR_VertexBuffer VBShader;
			VBShader.CopyVBChain(&m_VB);
			
			// Do we need to add vertex buffer or not ?
			if(((CEffectSystemRenderData*)_pRD)->CreateFragmentProgram(&VBShader, _pShader, m_SurfaceKey))
				_pVBM->AddVB(VBShader);
		}
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CEffectSystemRenderData
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CEffectSystemRenderData::CreateNewVBChain(uint _nV, uint _Contents)
{
	CXR_VBChain* pVBChain = m_pVBM->Alloc_VBChain(_Contents, _nV);
	if (pVBChain)
		pVBChain->m_pNextVB = NULL;
	m_pChain = pVBChain;
	m_nV = _nV;
	return (m_pChain) ? true : false;
}

void CEffectSystemRenderData::Render_Queue(uint _SurfaceID, const CMat4Dfp32& _VMat, const CFXDataShader* _pShader, const CMTime* _pTime, const fp32* _pPriority)
{
	// Adds a render chain for rendering
	CEffectSystemRenderChain Temp(0, m_pFXSystem->GetRenderInfo(m_pFXSystem->m_pRenderParams), this, _pShader, m_pSurfaceContext, m_pEngine, (_pTime) ? *_pTime : m_AnimTime0, m_pChain, m_pVBM, _SurfaceID, _VMat, _pPriority);
	memcpy(&m_lChain[m_nQueues++], &Temp, sizeof(CEffectSystemRenderChain));
}

void CEffectSystemRenderData::Render()
{
	// Go through chain and render all our registred queues=)
	for(int i = 0; i < m_nQueues; i++)
		m_lChain[i].Render(m_pVBM, m_pEngine);
}

CVec4Dfp32* CEffectSystemRenderData::AllocShaderParams(const CXR_VertexBuffer* _pVB, uint _nParams, uint32 _ShaderFlags, uint _nTexGenParams, uint _nPredefinedParams, uint _iTexGen)
{
	CVec4Dfp32* pFPParams = m_pVBM->Alloc_V4(_nParams);
	if (pFPParams)
	{
		if(_iTexGen == FXSHADER_TEXGEN_SCREENCOORD)
		{
			const CRC_Viewport* pVP = m_pVBM->Viewport_Get();
			const CMat4Dfp32& ProjMat = ((CRC_Viewport*)pVP)->GetProjectionMatrix();
			CMat4Dfp32 Matrix;

			if(_pVB->m_pTransform)
			{
				CMat4Dfp32 InvTransform;
				_pVB->m_pTransform->InverseOrthogonal(InvTransform);
				ProjMat.Multiply(InvTransform, Matrix);
			}
			else
				Matrix =ProjMat;

			// Setup default texture gen
			Matrix.Transpose();
			pFPParams[0] = *(CVec4Dfp32*)Matrix.k[0];
			pFPParams[1] = *(CVec4Dfp32*)Matrix.k[1];
			pFPParams[2] = *(CVec4Dfp32*)Matrix.k[2];
			pFPParams[3] = *(CVec4Dfp32*)Matrix.k[3];

			// Set predefined parameters
			//for(int i = 4, j = 0; i < FXSHADER_TEXGEN_PARAMS; i++, j++)
				//pFPParams[i] = m_TexGenParam[j];
		}

		uint8 nTexGenPredefinedParams = _nTexGenParams + _nPredefinedParams;
		for(int i = _nTexGenParams, j = 0; i < nTexGenPredefinedParams; i++, j++)
			pFPParams[i] = m_ShaderParam[j];

		// If shader isn't animated, we reset the time.
		if(_ShaderFlags & FXFLAG_SHADER_NOANIMATE)
			pFPParams[_nTexGenParams + 2].k[0] = 0;
	}

	return pFPParams;
}

bool CEffectSystemRenderData::CreateFragmentProgram(CXR_VertexBuffer* _pVB, const CFXDataShader* _pShader, CSurfaceKey& _SurfaceKey)
{
	CRC_Attributes* _pA = _pVB->m_pAttrib;
			
	if(!_pA)
	{
		_pA = m_pVBM->Alloc_Attrib();
		_pVB->m_pAttrib = _pA;
		if (!_pA)
			return false;
	}

	// Setup attributes
	_pA->SetDefault();
	
	// Get priority
	_pVB->m_Priority = _pShader->GetPriority(m_pFXSystem->m_pRenderParams->m_RenderInfo);
	/*
	if(_pShader->m_Flags & FXFLAG_SHADER_PRIO_POSTPROCESS)
	{
		_pVB->m_Priority = (CXR_VBPRIORITY_UNIFIED_MATERIAL + 5000 + 2.05f) + _pShader->m_PriorityOffset;
	}
//	else if(_pShader->m_Flags & FXFLAG_SHADER_PRIO_LIGHT)
//	{
//		_pVB->m_Priority = (fp32)m_pEngine->m_pSceneGraphInstance->SceneGraph_Light_GetIndex(TruncToInt(_pShader->m_Priority));
//	}
	else if(_pShader->m_Flags & FXFLAG_SHADER_PRIO_TRANSPARENT)
	{
		_pVB->m_Priority = (CXR_VBPRIORITY_MODEL_TRANSPARENT) + _pShader->m_PriorityOffset;
	}
	else if(_pShader->m_Flags & FXFLAG_SHADER_PRIO_OPAQUE)
	{
		_pVB->m_Priority = (CXR_VBPRIORITY_MODEL_OPAQUE) + _pShader->m_PriorityOffset;
	}
	else if(_pShader->m_Flags & FXFLAG_SHADER_PRIO_SURFACE_TRANSPARENT)
	{
		_pVB->m_Priority = m_pFXSystem->m_pRenderParams->m_RenderInfo.m_BasePriority_Transparent + _pShader->m_PriorityOffset;
	}
	else if(_pShader->m_Flags & FXFLAG_SHADER_PRIO_SURFACE_OPAQUE)
	{
		_pVB->m_Priority = m_pFXSystem->m_pRenderParams->m_RenderInfo.m_BasePriority_Opaque + _pShader->m_PriorityOffset;
	}
	else
	{
		_pVB->m_Priority = _pShader->m_Priority + _pShader->m_PriorityOffset;
	}
	*/

	_pA->Attrib_Disable(_pShader->m_Disable);
	_pA->Attrib_Enable(_pShader->m_Enable);
	_pA->Attrib_RasterMode(_pShader->m_RasterMode);
	_pA->Attrib_ZCompare(_pShader->m_ZCompare);
	_pA->Attrib_AlphaCompare(_pShader->m_AlphaCompare, _pShader->m_AlphaRef);

	// Allocate space for fragment program
	CRC_ExtAttributes_FragmentProgram20* pFP = (CRC_ExtAttributes_FragmentProgram20*) m_pVBM->Alloc(sizeof(CRC_ExtAttributes_FragmentProgram20));
	if (!pFP)
		return false;

	pFP->Clear();
	pFP->SetProgram(_pShader->m_Program, 0);
	_pA->m_pExtAttrib = pFP;

	// Allocate extra parameters and setup predefines
	uint nTexGenParams = _pShader->m_nTexGenParams;				//(_pShader->m_iTexGen) ? FXSHADER_TEXGEN_PARAMS : 0;
	uint nPredefinedParams = _pShader->m_nPredefinedParams;		//nTexGenParams + FXSHADER_FRAMENT_PARAMS;
	const uint8 nShaderParams = _pShader->m_lShaderParams.Len();
	const uint16 nFPParams = nTexGenParams + nPredefinedParams + nShaderParams;
	CVec4Dfp32* pFPParams = AllocShaderParams(_pVB, nFPParams, _pShader->m_Flags, nTexGenParams, nPredefinedParams, _pShader->m_iTexGen);
	if (!pFPParams)
		return false;

    // Setup extra shader parameters
	TAP_RCD<const CVec4Dfp32> lShaderParams = _pShader->m_lShaderParams;
	for(int i = nPredefinedParams + nTexGenParams, j = 0; i < nFPParams; i++, j++)
		pFPParams[i] = lShaderParams[j];

	// Set/create texgen
	switch(_pShader->m_iTexGen)
	{
		case FXSHADER_TEXGEN_SCREENCOORD:
			_pA->Attrib_TexGen(1, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_ALL);
			_pA->Attrib_TexGenAttr((fp32*)pFPParams);
			break;

		case FXSHADER_TEXGEN_EMITPOSITION:
			_pA->Attrib_TexCoordSet(1, 0);
			break;

		default:
			_pA->Attrib_TexCoordSet(1, 0);
			break;
	}

	//_pA->Attrib_TexGen(1, CRC_TEXGENMODE_LINEAR, CRC_TEXGENCOMP_ALL);
	//_pA->Attrib_TexGenAttr((fp32*)pFPParams);

	// Set fragment parameters
	pFP->SetParameters(pFPParams+nTexGenParams, nPredefinedParams + nShaderParams);

	// Set up our texture maps
	bool bResolve = false;
	int iTxt = 0;
	TAP<const int> lShaderMaps = _pShader->m_lShaderMaps;
	for (uint i = 0; i < lShaderMaps.Len(); i++)
	{
		// Is this a special ID ?
		if(lShaderMaps[i] < 0)
		{
			//switch(pSurfaceID[i]*-1)
			switch(lShaderMaps[i])
			{
			//case 1:		// Framebuffer
			case FXSHADER_MAP_FRAMEBUFFER:
				//_pA->Attrib_TextureID(iTxt++, m_pEngine->m_TextureID_LastScreen);
				//_pA->Attrib_TextureID(iTxt++, m_pEngine->m_TextureID_Screen);
				_pA->Attrib_TextureID(iTxt++, m_pEngine->m_TextureID_ResolveScreen);
				bResolve = true;

				break;
			
			// Depth / Stencil
			case FXSHADER_MAP_DEPTHBUFFER:
				if (m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_COPYDEPTH)
				{
					_pA->Attrib_TextureID(iTxt++, m_pEngine->m_TextureID_DepthStencil);
				}
				else
				{
					_pA->Attrib_TextureID(iTxt++, m_pEngine->m_TextureID_Depth);
				}
				break;

			case FXSHADER_MAP_DEPTHSTENCILBUFFER:
				if (m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_COPYDEPTH)
				{
					_pA->Attrib_TextureID(iTxt++, m_pEngine->m_TextureID_DepthStencil);
				}
				else
				{
					_pA->Attrib_TextureID(iTxt++, m_pEngine->m_TextureID_Depth);
				}
				break;

			// Deferred maps
			case FXSHADER_MAP_DEFERREDDIFFUSE:
				_pA->Attrib_TextureID(iTxt++, m_pEngine->m_TextureID_DeferredDiffuse);
				break;

			case FXSHADER_MAP_DEFERREDNORMAL:
				_pA->Attrib_TextureID(iTxt++, m_pEngine->m_TextureID_DeferredNormal);
				break;

			case FXSHADER_MAP_DEFERREDSPECULAR:
				_pA->Attrib_TextureID(iTxt++, m_pEngine->m_TextureID_DeferredSpecular);
				break;

			// Misc.
			case FXSHADER_MAP_LASTSCREEN:
				//_pA->Attrib_TextureID(iTxt++, m_pEngine->m_TextureID_ResolveScreen);
				_pA->Attrib_TextureID(iTxt++, m_pEngine->m_TextureID_ResolveScreen);
				break;

			case FXSHADER_MAP_SHADOWMASK:
				_pA->Attrib_TextureID(iTxt++, m_pEngine->m_TextureID_ShadowMask);
				break;

			case FXSHADER_MAP_POSTPROCESS:
				{
					// Testing...
					//CVec2Dfp32* pTVOld = (CVec2Dfp32*)((CXR_VBChain*)_pVB->m_pVBChain)->m_pTV[0];
					//CVec2Dfp32* pTV = m_pVBM->Alloc_V2(4);
					//for(int iScale = 0; iScale < 4; iScale++)
					//	pTV[iScale] = pTVOld[iScale] * 0.25f;
					//((CXR_VBChain*)_pVB->m_pVBChain)->m_pTV[0] = (fp32*)pTV;

					CTextureContainer_Screen* pTCScreen = safe_cast<CTextureContainer_Screen>(m_pEngine->GetInterface(XR_ENGINE_TCSCREEN));
					_pA->Attrib_TextureID(iTxt++, (pTCScreen) ? pTCScreen->GetTextureID(6) : 0);
				}
				break;

			default:	// Default - none
				_pA->Attrib_TextureID(iTxt++, 0);
				ConOutL(CStrF("§cf70WARNING: Invalid surface in EffectSystem program!"));
				break;
			}
		}
		else if (lShaderMaps[i] > 0)
		{
			// Setup our layers
			//const CXW_Surface* pSurface = m_pFXSystem->GetSurfaceContext()->GetSurface(pSurfaceID[i]);
			//const CXW_SurfaceKeyFrame* pSurfKey = pSurface->GetFrame();//GetBaseFrame();
			//const int nLayers = pSurfKey->m_lTextures.Len();
			//const CXW_SurfaceLayer* pLayers = pSurfKey->m_lTextures.GetBasePtr();
			
			_SurfaceKey.Create(0, m_pEngine->GetSC(), m_pEngine, m_AnimTime0, lShaderMaps[i]);
			
			// Handle advanced surface or not ?
			TAP<const CXW_SurfaceLayer> lLayers = _SurfaceKey.m_pKeyFrame->m_lTextures;
			if(_pShader->m_Flags & FXFLAG_SHADER_ADVANCEDSURFACE)
			{
				for(int iLayer = 0; iLayer < lLayers.Len(); iLayer++)
				{
					_pA->Attrib_TextureID(iTxt++, lLayers[iLayer].m_TextureID);

					// Assuming that all objects has their texture coordinates placed in channel 0.
					// This should be fixed, and be some kind of parameter.
					{
						if(!ProcessAdvancedSurface(0, _pVB, lLayers[iLayer], _SurfaceKey.m_pKeyFrame))
							return false;
					}
				}
			}
			else
			{
				for(int iLayer = 0; iLayer < lLayers.Len(); iLayer++)
					_pA->Attrib_TextureID(iTxt++, lLayers[iLayer].m_TextureID);
			}
		}
	}

#if 1	// Enabled,
	// Need to resolve screen
	if (bResolve)
	{
		// This resolve seemed to crash the great and mighty xenon machine!

		MSCOPESHORT(CEffectSystemRenderData::CreateFragmentProgram_ResolveScreen);
		CBox3Dfp32 BBox, BBoxWorld;
		m_pFXSystem->GetBound_Box(BBox, m_pAnimState);
		BBox.Transform(m_WMat, BBoxWorld);
		CScissorRect ScissorRect;

		CFXSysUtil::CalcBoxScissor(m_pEngine, m_pVBM->Viewport_Get(), &m_VMat, BBoxWorld, ScissorRect);
		CRct Rect(CPnt(ScissorRect.GetMinX(), ScissorRect.GetMinY()), CPnt(ScissorRect.GetMaxX(), ScissorRect.GetMaxY()));

		/* Render resolve area
		{
			CMat4Dfp32 Mat2D;
			m_pVBM->Viewport_Get()->Get2DMatrix_RelBackPlane(Mat2D);
			CRC_Attributes* pA = m_pVBM->Alloc_Attrib(); 
			pA->SetDefault();
			pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
			pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
			CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(m_pVBM, &Mat2D, ScissorRect, CPixel32(0,0,196,96), fp32(CXR_VBPRIORITY_UNIFIED_MATERIAL + 9000), pA);
			if (pVB)
				m_pVBM->AddVB(pVB);
		}
		*/

		// The fault is in here somewhere, don't know what thought...
		CFXVBMAllocUtil VBMAlloc;
		VBMAlloc.Alloc_CopyToTexture(1);
		if (!VBMAlloc.Alloc(m_pVBM))
			return false;
		
		// Add pre render data to this vertex buffer so we resolves the screen before any rendering
		VBMAlloc.AddCopyToTexture(_pVB, Rect, Rect.p0, m_pEngine->m_TextureID_ResolveScreen, true);
		//VBMAlloc.AddCopyToTexture(_pVB->m_Priority - 0.000215f, Rect, Rect.p0, m_pEngine->m_TextureID_ResolveScreen, true);
	}
#endif

	// If we are using advanced surface rendering, we don't need this vertex buffer anymore
	if(_pShader->m_Flags & FXFLAG_SHADER_ADVANCEDSURFACE)
	{
		_pVB->Clear();
		return false;
	}

	return true;
}

bool CEffectSystemRenderData::ProcessAdvancedSurface(int _iChannel, CXR_VertexBuffer* _pVB, const CXW_SurfaceLayer& _Layer, const CXW_SurfaceKeyFrame* _pSurfaceKey)
{
	// Evaluate surface VB Operators
	int iChannel = _iChannel;
	if(_Layer.m_lOper.Len())
	{
		// Create vertex buffer for this layer operations
		CXR_VertexBuffer* pVB = m_pVBM->Alloc_VB();

		if(!pVB)
			return false;

		pVB->CopyVBChain(_pVB);
		CXR_VBChain* pVBChain = pVB->GetVBChain();
		
		// Copy information from source object
		pVB->m_pAttrib = _pVB->m_pAttrib;
		pVB->m_pTransform = _pVB->m_pTransform;
		pVB->m_Priority = _pVB->m_Priority;

		// Create VB Operators context
		CXR_VBOperatorContext Context;
		Context.m_pEngine = m_pEngine;
		Context.m_pVBM = m_pVBM;
		Context.m_AnimTime = m_AnimTime0;
		Context.m_AnimTimeWrapped = _pSurfaceKey->m_AnimTimeWrapped;
		Context.m_iTexChannel = iChannel;
		Context.m_iTexChannelNext = iChannel;
		Context.m_VBArrayReadOnlyMask = -1;
		Context.m_pModel2View = pVB->m_pTransform;
		Context.m_pWorld2View = NULL;
		Context.m_pWorld2View = NULL;
		Context.m_pVBHead = pVB;
		Context.m_pVBHeadSrc = _pVB;

		int iFree = 0;
		Context.m_iFreeTexCoordSet = 0;
		if(pVBChain)
		{
			while(iFree < CRC_MAXTEXCOORDS && pVBChain->m_pTV[iFree] != NULL && iFree != 1) iFree++;
			if(iFree == CRC_MAXTEXCOORDS) iFree = -1;
			Context.m_iFreeTexCoordSet = iFree;
		}

		// Retrive surface context
		CXR_SurfaceContext* pSC;
		if(!m_pEngine)
		{
			MACRO_GetRegisterObject(CXR_SurfaceContext, pTempSC, "SYSTEM.SURFACECONTEXT");
			pSC = pTempSC;
			if(!pSC)
				Error_static("CEffectSystemRenderData", "No surface-context available.");
		}
		else
			pSC = m_pEngine->GetSC();

		// Run operators
		for(int iOper = 0; iOper < _Layer.m_lOper.Len(); iOper++)
		{
			const CXW_LayerOperation& Oper = _Layer.m_lOper[iOper];
			switch(Oper.m_OpCode)
			{
				case XW_LAYEROPCODE_OPERATOR:
				{
					CXR_VBOperator* pVBOperator = pSC->VBOperator_Get(Oper.m_iOperatorClass);
					if(pVBOperator)
					{
//						CXR_VertexBuffer* pTargetVB = pVB;
//						CXR_VertexBuffer* pSrcVB = _pVB;

						if(!pVBOperator->OnOperate(Context, Oper, pVB))
							return false;

						if(!pVBOperator->OnOperateFinish(Context, Oper, pVB))
							return false;
					}
				}
				break;

				default:
					break;
			}
		}

		iChannel = Context.m_iTexChannelNext;
		//nChannel = MaxMT(nChannels, iChannel+1);

		m_pVBM->AddVB(pVB);
	}

	return true;
}

fp32 CFXDataObject::GetPriority(const CXR_RenderInfo& _RenderInfo) const
{
	// Get priority
	switch(m_Priority)
	{
		case FXPRIO_POSTPROCESS:
			return (CXR_VBPRIORITY_UNIFIED_MATERIAL + 8000 + 4.05f) + m_PriorityOffset;
			//return (CXR_VBPRIORITY_UNIFIED_MATERIAL + 5000 + 4.05f) + m_PriorityOffset;

		case FXPRIO_TRANSPARENT:
		{
			return _RenderInfo.m_BasePriority_Transparent + m_PriorityOffset;
			return (CXR_VBPRIORITY_MODEL_TRANSPARENT) + m_PriorityOffset;
		}

		case FXPRIO_OPAQUE:
			return (CXR_VBPRIORITY_MODEL_OPAQUE) + m_PriorityOffset;

		case FXPRIO_SURFACE_TRANSPARENT:
			return _RenderInfo.m_BasePriority_Transparent + m_PriorityOffset;

		case FXPRIO_SURFACE_OPAQUE:
			return _RenderInfo.m_BasePriority_Opaque + m_PriorityOffset;
	}

	return m_Priority + m_PriorityOffset;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataCollect
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFXDataCollect::CFXDataCollect()
//	: m_Flags("")
	: m_TotalDuration(0)
//	, m_ParticleKeys("")
//	, m_RandomFlags("")
	, m_Seed(0)
	, m_IntFlags(0)
	, m_nMulti(1)
	, m_HistoryTime(6)
	, m_HistorySpawn(0.2f)
	, m_pShader(0)
	, m_Priority(0)
	, m_PriorityOffset(0)
//	, m_ProgramName("")
	, m_Enable(0)
	, m_Disable(0)
	, m_RasterMode(0)
	, m_ZCompare(CRC_COMPARE_LESSEQUAL)
	, m_AlphaCompare(CRC_COMPARE_ALWAYS)
	, m_AlphaRef(128)
	, m_iShader(0)
{
//	m_lDurationReciprocal.Clear();
//	m_lSurfaceID.Clear();
//	m_lDuration.Clear();
//	m_lLength.Clear();
//	m_lWidth.Clear();
//	m_lColor.Clear();
//	m_lRotationAngle.Clear();
//	m_lPosNoise.Clear();
//	m_lVelocity.Clear();
//	m_lTextureScroll.Clear();
//	m_lSegments.Clear();
//	m_lStripHeight.Clear();
//	m_lWidthNoise.Clear();
//	m_lHeight.Clear();
//	m_lDirection.Clear();
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataObject
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFXDataObject::CFXDataObject()
	: m_IntFlags(0)
	, m_Flags(0)
	, m_RandFlags(0)
	, m_TotalDuration(0)
	, m_Priority(0)
	, m_PriorityOffset(0)
	, m_BoundBox(CBox3Dfp32(0,0))
	, m_MaxBound(0)
	, m_iShader(0)
	, m_iRenderTarget(0)
	, m_UseHistoryObject(0)
	, m_HistoryTime(0)
	, m_HistorySpawn(0)
	, m_pShader(0)

//	, m_Priority(0)
//	, m_RasterMode(0)
//	, m_Enable(0)
//	, m_Disable(0)
{
//	m_lDuration.Clear();
//	m_lDurationReciprocal.Clear();
}


CFXDataObject::CFXDataObject(const CFXDataCollect& _Data)
	: m_IntFlags(_Data.m_IntFlags)
	, m_Flags(0)
	, m_RandFlags(0)
	, m_TotalDuration(_Data.m_TotalDuration)
	, m_Priority(_Data.m_Priority)
	, m_PriorityOffset(_Data.m_PriorityOffset)
	, m_BoundBox(CBox3Dfp32(0,0))
	, m_MaxBound(0)
	, m_iShader(_Data.m_iShader)
	, m_iRenderTarget(_Data.m_iRenderTarget)
	, m_UseHistoryObject(0)
	, m_HistoryTime(_Data.m_HistoryTime)
	, m_HistorySpawn(_Data.m_HistorySpawn)
	, m_pShader(_Data.m_pShader)
//	, m_Priority(_Data.m_Priority)
//	, m_RasterMode(_Data.m_RasterMode)
//	, m_Enable(_Data.m_Enable)
//	, m_Disable(_Data.m_Disable);
{
	m_lDuration.Add(_Data.m_lDuration);
	m_lDurationReciprocal.Add(_Data.m_lDurationReciprocal);
}

void CFXDataObject::ClipHomogenousLine(CVec4Dfp32& _v0, CVec4Dfp32& _v1)
{
	if(_v0.k[2] < 0.0f)
	{
		if(_v1.k[2] <= 0.0f)
			return;

		fp32 Temp = _v0.k[2] / (_v0.k[2] - _v1.k[2]);
		_v0.k[0] += (_v1.k[0] - _v0.k[0]) * Temp;
		_v0.k[1] += (_v1.k[1] - _v0.k[1]) * Temp;
		_v0.k[3] += (_v1.k[3] - _v0.k[3]) * Temp;
		_v0.k[2] = 0.0f;
	}
	else if(_v1.k[2] < 0.0f)
	{
		if(_v0.k[2] <= 0.0f)
			return;

		fp32 Temp = _v1.k[2] / (_v1.k[2] - _v0.k[2]);
		_v1.k[0] += (_v0.k[0] - _v1.k[0]) * Temp;
		_v1.k[1] += (_v0.k[1] - _v1.k[1]) * Temp;
		_v1.k[3] += (_v0.k[3] - _v1.k[3]) * Temp;
	}
}

bool CFXDataObject::Render_BeamStrip(CEffectSystemRenderData* _pRD, uint _SurfaceID, const CXR_BeamStrip* _pBeams, int _nBeams, int _Flags, const CMat4Dfp32* _pWMat, const CMat4Dfp32* _pVMat)
{
	MSCOPESHORT(CFXDataObject::Render_BeamStrip);
	if (!_SurfaceID && !m_pShader)
		return false;

	const uint16 nBeams = (_nBeams > CXR_Util::MAXBEAMS) ? CXR_Util::MAXBEAMS : _nBeams;

	#ifndef M_RTM
		if(_nBeams > CXR_Util::MAXBEAMS)
			ConOutLD(CStrF("§cf80WARNING: (CFXDataObject::Render_BeamStrip) Too many segments (%i / %i), discarding overflow!", _nBeams, CXR_Util::MAXBEAMS));
	#endif
	
	// Create new vb chain
	if (!_pRD->CreateNewVBChain(nBeams * 2))				// n*2		verts
		return false;

	// Setup default texture vertices
	if(_Flags & CXR_BEAMFLAGS_BEGINCHAIN)
		_pRD->m_pChain->m_pTV[0] = (fp32 *)CXR_Util::m_lBeamTVertices;
	else
		_pRD->m_pChain->m_pTV[0] = (fp32 *)CXR_Util::m_lBeamStripTVertices;
	_pRD->m_pChain->m_nTVComp[0] = 2;
	
	// NOTE: Oh oh,,, don't do this, the CXR_BEAMFLAGS_BEGINCHAIN is used for a diffrent purpose!!!
	// Check if we want to use default beam indices or if we want to setup our own
	if(_Flags & CXR_BEAMFLAGS_BEGINCHAIN)
		_pRD->SetupBeamIndices(nBeams);					// (n-1)*2	tris
	else
		_pRD->SetupBeamStripIndices((nBeams-1) * 2);	// (n-1)*2	tris
	
	// Fetch pointers
	CVec3Dfp32* pV = &_pRD->m_pChain->m_pV[0];
//	fp32** pTV = &_pRD->m_pChain->m_pTV[0];
	CVec2Dfp32* pTV2D = (_Flags & CXR_BEAMFLAGS_TEXFROMOFFSET) ? _pRD->AllocateTVertices() : 0;
	CPixel32* pC = &_pRD->m_pChain->m_pCol[0];

	const CMat4Dfp32& WMat = (_pWMat) ? *_pWMat : _pRD->m_WMat;
	const CMat4Dfp32& VMat = (_pVMat) ? *_pVMat : _pRD->m_VMat;

	CMat4Dfp32 Move, VMove;
	Move.Unit();
	CVec3Dfp32::GetMatrixRow(WMat, 3).SetMatrixRow(Move, 3);
	Move.Multiply(VMat, VMove);
	CVec3Dfp32 InvPos(VMat.k[3][0] * VMat.k[0][0] + VMat.k[3][1] * VMat.k[0][1] + VMat.k[3][2] * VMat.k[0][2] + WMat.k[3][0],
					 VMat.k[3][0] * VMat.k[1][0] + VMat.k[3][1] * VMat.k[1][1] + VMat.k[3][2] * VMat.k[1][2] + WMat.k[3][1],
					 VMat.k[3][0] * VMat.k[2][0] + VMat.k[3][1] * VMat.k[2][1] + VMat.k[3][2] * VMat.k[2][2] + WMat.k[3][2]);
	int iV = 0;
	
	if(_Flags & CXR_BEAMFLAGS_BEGINCHAIN)
	{
		for(int b = 0; b < _nBeams; b+=2)
		{
			CVec3Dfp32 VDir = _pBeams[b+1].m_Pos - _pBeams[b].m_Pos;
			VDir.MultiplyMatrix3x3(WMat);

			CVec3Dfp32 VPos = _pBeams[b].m_Pos;
			VPos.MultiplyMatrix3x3(WMat);

				CVec3Dfp32 Up(VDir[1] * (VPos[2] + InvPos[2]) - VDir[2] * (VPos[1] + InvPos[1]),
						-VDir[0] * (VPos[2] + InvPos[2]) + VDir[2] * (VPos[0] + InvPos[0]),
						VDir[0] * (VPos[1] + InvPos[1]) - VDir[1] * (VPos[0] + InvPos[0]));
			Up *= _pBeams[b].m_Width * Up.LengthInv();

			pV[iV + 0] = VPos + Up + VDir;
			pV[iV + 1] = VPos + Up;// + VDir;
			pV[iV + 2] = VPos - Up;// + VDir;
			pV[iV + 3] = VPos - Up + VDir;

			if(_Flags & CXR_BEAMFLAGS_EDGEFADE && _pBeams[b].m_Color > 0)
			{
				CVec3Dfp32 V0 = VPos + InvPos;
				CVec3Dfp32 V1 = VDir;
				V0.Normalize();
				V1.Normalize();
				int Alpha = (int)(255 - 255 * M_Fabs(V0 * V1));
				
				pC[iV + 0] = _pBeams[b].m_Color & 0x00ffffff | ((((_pBeams[b].m_Color & 0xff000000) >> 8) * Alpha) & 0xff000000);
				pC[iV + 1] = _pBeams[b].m_Color & 0x00ffffff | ((((_pBeams[b].m_Color & 0xff000000) >> 8) * Alpha) & 0xff000000);
				pC[iV + 2] = _pBeams[b+1].m_Color & 0x00ffffff | ((((_pBeams[b+1].m_Color & 0xff000000) >> 8) * Alpha) & 0xff000000);
				pC[iV + 3] = _pBeams[b+1].m_Color & 0x00ffffff | ((((_pBeams[b+1].m_Color & 0xff000000) >> 8) * Alpha) & 0xff000000);
			}
			else
			{
				pC[iV + 0] = _pBeams[b].m_Color;
				pC[iV + 1] = _pBeams[b].m_Color;
				pC[iV + 2] = _pBeams[b+1].m_Color;
				pC[iV + 3] = _pBeams[b+1].m_Color;
			}

			iV += 4;
		}
	}
	else
	{
		int Last = 0;
//		int iTri = 0;
		for(int b = 0; b < _nBeams; b++)
		{
			CVec3Dfp32 VDir;
			int Next = _pBeams[b].m_Color & 0xff000000;
			if(((Last == 0 && Next == 0) || b == 0) && (b < _nBeams - 1 || _nBeams == 1))
				VDir = _pBeams[b + 1].m_Pos - _pBeams[b].m_Pos;
			else
			{
				if(Next == 0 || b == _nBeams - 1)
					VDir = _pBeams[b].m_Pos - _pBeams[b - 1].m_Pos;
				else
					VDir = _pBeams[b + 1].m_Pos - _pBeams[b - 1].m_Pos;
			}

			{
				VDir.MultiplyMatrix3x3(WMat);

				CVec3Dfp32 VPos = _pBeams[b].m_Pos;
				VPos.MultiplyMatrix3x3(WMat);

				CVec3Dfp32 Up(VDir[1] * (VPos[2] + InvPos[2]) - VDir[2] * (VPos[1] + InvPos[1]),
							-VDir[0] * (VPos[2] + InvPos[2]) + VDir[2] * (VPos[0] + InvPos[0]),
							VDir[0] * (VPos[1] + InvPos[1]) - VDir[1] * (VPos[0] + InvPos[0]));
				Up *= _pBeams[b].m_Width * Up.LengthInv();

				pV[iV + 0] = VPos + Up;
				pV[iV + 1] = VPos - Up;

				if(_Flags & CXR_BEAMFLAGS_EDGEFADE && _pBeams[b].m_Color > 0)
				{
					CVec3Dfp32 V0 = VPos + InvPos;
					CVec3Dfp32 V1 = VDir;
					V0.Normalize();
					V1.Normalize();
					int Alpha = (int)(255 - 255 * M_Fabs(V0 * V1));
					
					pC[iV + 0] = _pBeams[b].m_Color & 0x00ffffff | ((((_pBeams[b].m_Color & 0xff000000) >> 8) * Alpha) & 0xff000000);
					pC[iV + 1] = _pBeams[b].m_Color & 0x00ffffff | ((((_pBeams[b].m_Color & 0xff000000) >> 8) * Alpha) & 0xff000000);
				}
				else
				{
					pC[iV + 0] = _pBeams[b].m_Color;
					pC[iV + 1] = _pBeams[b].m_Color;
				}

				
				if(_Flags & CXR_BEAMFLAGS_TEXFROMOFFSET)
				{
					pTV2D[iV + 0][0] = 1;
					pTV2D[iV + 0][1] = _pBeams[b].m_TextureYOfs;

					pTV2D[iV + 1][0] = 0;
					pTV2D[iV + 1][1] = _pBeams[b].m_TextureYOfs;
				}

				iV += 2;
			}

			Last = Next;
		}
	}
	_Flags &= ~CXR_BEAMFLAGS_BEGINCHAIN;

	if((_Flags & CXR_BEAMFLAGS_TEXFROMOFFSET))
		_pRD->m_pChain->m_pTV[0] = (fp32*)pTV2D;

	_pRD->Render_Queue(_SurfaceID, VMove, m_pShader);

	return true;
}


bool CFXDataObject::Render_Quad_Skeleton(CEffectSystemRenderData* _pRD, uint _SurfaceID, fp32 _Width, fp32 _Width2, fp32 _SphereRadius, fp32 _Angle, uint32 _Color, uint32 _Flags, const CMat4Dfp32* _pWMat, const CMat4Dfp32* _pVMat, const CMTime* _pTime)
{
	if (!_SurfaceID && !m_pShader)
		return false;

	TThinArray<uint16> liBones;
	uint16* piBones = NULL;
	int nBones = 0;

	switch(_pRD->m_iSkeletonType)
	{
		// Bone binding type 1
		case 1:
		{
			nBones = 20;
			liBones.SetLen(nBones);
			piBones = liBones.GetBasePtr();
			for(int i = 0; i < nBones; i++)
				piBones[i] = i+1;
			piBones[19] = 21;
		}
		break;

		// Bone binding type 2
		case 2:
		{
			nBones = 34;
			liBones.SetLen(nBones);
			piBones = liBones.GetBasePtr();
			
			int i = 0;
			for(int j = 1; i < 19; i++, j++)
				piBones[i] = j;

            piBones[i++] = 21;

			for(int j = 25; i < 28; i++, j++)
				piBones[i] = j;

			piBones[i++] = 40;
			piBones[i++] = 46;
			piBones[i++] = 52;
			piBones[i++] = 55;
			piBones[i++] = 61;
			piBones[i++] = 67;

			int iBreak = 0;
		}
		break;
	}

	const int nVerts = nBones * 4;
	const int nTris = nBones * 2;

	// Height is ignored when rendering on skeleton.
	if (!_pRD->CreateNewVBChain(nVerts))
		return false;

	_pRD->m_pChain->m_pTV[0] = (fp32 *)CXR_Util::m_lQuadParticleTVertices;
	_pRD->m_pChain->m_nTVComp[0] = 2;
	_pRD->SetupQuadIndices(nTris);

	fp32 sinv, cosv;
	QSinCos(_Angle, sinv, cosv);

	CVec3Dfp32* pV = &_pRD->m_pChain->m_pV[0];
	CPixel32* pC = &_pRD->m_pChain->m_pCol[0];

	const CMat4Dfp32& WMat = (_pWMat) ? *_pWMat : _pRD->m_WMat;
	const CMat4Dfp32& VMat = (_pVMat) ? *_pVMat : _pRD->m_VMat;

	MemSetD(pC, _Color, nVerts);

	CXR_SkeletonInstance* pSkelInst = _pRD->m_pSkeletonInst;
	CXR_Skeleton* pSkel = _pRD->m_pSkeleton;
	if(_Flags & FXFLAG_QUAD_BILLBOARD)
	{
		const fp32 c0 = cosv * _Width * _SQRT2;
		const fp32 c1 = -sinv * _Width * _SQRT2;

		CMat4Dfp32 Matrix, WVMat;

		if(_SphereRadius != 0)
		{
			for(int i = 0, j = 0; i < nBones; i++)
			{
				Matrix = pSkelInst->m_pBoneTransform[piBones[i]];
				
				Matrix.GetRow(3) = (pSkel->m_lNodes[piBones[i]].m_LocalCenter * pSkelInst->m_pBoneTransform[piBones[i]]);
				Matrix.Multiply(_pRD->m_VMat, WVMat);
				const CVec3Dfp32 BonePos = Matrix.GetRow(3) * _pRD->m_VMat;

				const CVec3Dfp32 SphereRadius = (CVec3Dfp32(0,0,1) * WVMat).Normalize() * _SphereRadius;
				pV[j++] = BonePos + CVec3Dfp32( c1, -c0, 0) + SphereRadius;
				pV[j++] = BonePos + CVec3Dfp32( c0,  c1, 0) + SphereRadius;
				pV[j++] = BonePos + CVec3Dfp32(-c1,  c0, 0) + SphereRadius;
				pV[j++] = BonePos + CVec3Dfp32(-c0, -c1, 0) + SphereRadius;
			}
		}
		else
		{
			for(int i = 0, j = 0; i < nBones; i++)
			{
				const CVec3Dfp32 BonePos = (pSkel->m_lNodes[piBones[i]].m_LocalCenter * pSkelInst->m_pBoneTransform[piBones[i]]) * _pRD->m_VMat;
				pV[j++] = BonePos + CVec3Dfp32( c1, -c0, 0);
				pV[j++] = BonePos + CVec3Dfp32( c0,  c1, 0);
				pV[j++] = BonePos + CVec3Dfp32(-c1,  c0, 0);
				pV[j++] = BonePos + CVec3Dfp32(-c0, -c1, 0);
			}
		}

		CMat4Dfp32 Unit;
		Unit.Unit();
		_pRD->Render_Queue(_SurfaceID, Unit, m_pShader, _pTime);
	}

	return true;
}

bool CFXDataObject::Render_ScreenQuad(CEffectSystemRenderData* _pRD, const CRect2Duint16& _VPRect16, int _SurfaceID, uint32 _Color, const CMTime* _pTime, CVec2Dfp32* _pTVTexGen)
{
	if (!_SurfaceID && !m_pShader)
		return false;

	if (!_pRD->CreateNewVBChain(4))
		return false;

	CRC_Viewport* pVP = _pRD->m_pVBM->Viewport_Get();
	fp32 RcpX = 1.0f / (fp32)pVP->GetViewRect().GetWidth();
	fp32 RcpY = 1.0f / (fp32)pVP->GetViewRect().GetHeight();

	fp32 MinX = (fp32)_VPRect16.m_Min[0] * RcpX;
	fp32 MinY = (fp32)_VPRect16.m_Min[1] * RcpY;
	fp32 MaxX = (fp32)_VPRect16.m_Max[0] * RcpX;
	fp32 MaxY = (fp32)_VPRect16.m_Max[1] * RcpY;

	_pRD->m_pChain->m_pTV[0] = (fp32 *)CXR_Util::VBM_CreateRectUV_VFlip(_pRD->m_pVBM, CVec2Dfp32(MinX,MinY), CVec2Dfp32(MaxX,MaxY));//CXR_Util::m_lQuadParticleTVertices;
	_pRD->m_pChain->m_nTVComp[0] = 2;
	_pRD->m_pChain->m_pTV[1] = (_pTVTexGen) ? (fp32 *)_pTVTexGen : NULL;
	_pRD->m_pChain->m_nTVComp[1] = (_pTVTexGen) ? 2 : 0;
	_pRD->SetupQuadIndices(2);

	CVec3Dfp32* pV = &_pRD->m_pChain->m_pV[0];
	CPixel32* pC = &_pRD->m_pChain->m_pCol[0];
//	CVec2Dfp32* pTV0 = &_pRD->m_pChain->m_pTV[0];
	
	MemSet(pC, _Color, sizeof(CPixel32) * 4);

	CMat4Dfp32 Mat;
	_pRD->m_pVBM->Viewport_Get()->Get2DMatrix_RelBackPlane(Mat);

	CVec3Dfp32(_VPRect16.m_Max[0], _VPRect16.m_Max[1], 0).MultiplyMatrix(Mat, pV[0]);
	CVec3Dfp32(_VPRect16.m_Min[0], _VPRect16.m_Max[1], 0).MultiplyMatrix(Mat, pV[1]);
	CVec3Dfp32(_VPRect16.m_Min[0], _VPRect16.m_Min[1], 0).MultiplyMatrix(Mat, pV[2]);
	CVec3Dfp32(_VPRect16.m_Max[0], _VPRect16.m_Min[1], 0).MultiplyMatrix(Mat, pV[3]);

	CMat4Dfp32 Unit; Unit.Unit();
	_pRD->Render_Queue(_SurfaceID, Unit, m_pShader, _pTime);

	return true;
}

void CFXDataObject::Render_Quad(CXR_VBManager* _pVBM, CXR_Engine* _pEngine, int _TextureID, const CVec3Dfp32& _LocalPos, fp32 _Size, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, bool _bNoZ, fp32 _Priority)
{
	CFXVBMAllocUtil AllocUtil;

	AllocUtil.Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES | CXR_VB_TVERTICES0, 4);
	AllocUtil.Alloc_M4();

	if (AllocUtil.Alloc(_pVBM))
	{
		// Get allocated data
		CMat4Dfp32* pWMat = AllocUtil.Get_M4(_WMat);
		CXR_VertexBuffer* pVB = AllocUtil.Get_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES | CXR_VB_TVERTICES0, 4);
		pVB->Matrix_Set(pWMat);
		
		// Get pointers
		CXR_VBChain* pVBChain = pVB->GetVBChain();
		CVec3Dfp32* pV = pVBChain->m_pV;
		CVec2Dfp32* pTV = (CVec2Dfp32*)pVBChain->m_pTV[0];
	
		// Setup vertices
		fp32 c0 = _Size * 0.5f;
		fp32 c1 = -c0;
		pV[0] = CVec3Dfp32(-c0, 0.0f,  c1);
		pV[1] = CVec3Dfp32(-c1, 0.0f, -c0);
		pV[2] = CVec3Dfp32( c0, 0.0f, -c1);
		pV[3] = CVec3Dfp32( c1, 0.0f,  c0);

		// Setup texture coordinates
		int bFlip = _pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;
		CXR_Util::Init();
		CXR_Util::VBM_CreateRectUV(CVec2Dfp32(0.0f), CVec2Dfp32(1.0f), bFlip, pTV);

		CRC_Attributes* pA = pVB->m_pAttrib;
		pA->SetDefault();
		pA->Attrib_RasterMode(CRC_RASTERMODE_ADD);
		pA->Attrib_Disable(CRC_FLAGS_CULL | ((_bNoZ) ? (CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE) : 0));
		pA->Attrib_TextureID(0, _TextureID);
		
		// Setup triangle indices
		pVB->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, 2);
		pVB->Geometry_Color(0xffffffff);
		pVB->m_Priority = _Priority;

		// Add vertex buffer
		_pVBM->AddVB(pVB);
	}
}

bool CFXDataObject::Render_Quad(CEffectSystemRenderData* _pRD, uint _SurfaceID, const CVec3Dfp32& _Pos, fp32 _Width, fp32 _Width2, fp32 _Height, fp32 _SphereRadius, fp32 _Angle, uint32 _Color, uint32 _Flags, const CMat4Dfp32* _pWMat, const CMat4Dfp32* _pVMat, const CMTime* _pTime)
{
	MSCOPESHORT(CFXDataObject::Render_Quad);
	if (!_SurfaceID && !m_pShader)
		return false;

	if(_pRD->m_pSkeletonInst && _pRD->m_iSkeletonType > 0)
	{
		Render_Quad_Skeleton(_pRD, _SurfaceID, _Width, _Width2, _SphereRadius, _Angle, _Color, _Flags, _pWMat, _pVMat, _pTime);
		return true;
	}

	// Determine vertex usage and setup indices
	if(_Flags & FXFLAG_QUAD_HEIGHT)
	{
		if (!_pRD->CreateNewVBChain(5))											// 5 vertices
			return false;

		_pRD->m_pChain->m_pTV[0] = (fp32 *)CXR_Model_EffectSystem::m_lQuadCrossTVertices;
		_pRD->m_pChain->m_nTVComp[0] = 2;
		_pRD->SetupIndices(4, CXR_Model_EffectSystem::m_lQuadCrossTriangles);	// 4 primitives
	}
	else
	{
		if (!_pRD->CreateNewVBChain(4))	// 4 vertices
			return false;

		_pRD->m_pChain->m_pTV[0] = (fp32 *)CXR_Util::m_lQuadParticleTVertices;
		_pRD->m_pChain->m_nTVComp[0] = 2;
		_pRD->SetupQuadIndices(2);	// 2 triangles
	}

	fp32 sinv, cosv;
	QSinCos(_Angle, sinv, cosv);

	// Fetch pointers
	CVec3Dfp32* pV = &_pRD->m_pChain->m_pV[0];
//	fp32** pTV = &_pRD->m_pChain->m_pTV[0];
	CPixel32* pC = &_pRD->m_pChain->m_pCol[0];

	const CMat4Dfp32& WMat = (_pWMat) ? *_pWMat : _pRD->m_WMat;
	const CMat4Dfp32& VMat = (_pVMat) ? *_pVMat : _pRD->m_VMat;

	pC[0] = _Color;
	pC[1] = _Color;
	pC[2] = _Color;
	pC[3] = _Color;
		
	if(_Flags & FXFLAG_QUAD_HEIGHT)
		pC[4] = _Color;

	CMat4Dfp32 Move;
	WMat.Multiply(VMat, Move);
	if(_Flags & FXFLAG_QUAD_BILLBOARD)
	{
		const fp32 c0 = cosv * _Width * _SQRT2;
		const fp32 c1 = -sinv * _Width2 * _SQRT2;
		
		pV[0] = _Pos + CVec3Dfp32( c1, -c0, _Height);
		pV[1] = _Pos + CVec3Dfp32( c0,  c1, _Height);
		pV[2] = _Pos + CVec3Dfp32(-c1,  c0, _Height);
		pV[3] = _Pos + CVec3Dfp32(-c0, -c1, _Height);

		// Do we have more than 4 vertices on this quad?, if so we define the middle vertex creating a pyramid
		if(_Flags & FXFLAG_QUAD_HEIGHT)
			pV[4] = _Pos;

		if(_SphereRadius != 0)
		{
			CMat4Dfp32 WVMat;
			_pRD->m_WMat.Multiply(_pRD->m_VMat, WVMat);
			const CVec3Dfp32 SphereRadius = (CVec3Dfp32(0,0,1) * WVMat).Normalize() * _SphereRadius;
			
			int nV = (_Flags & FXFLAG_QUAD_HEIGHT) ? 5 : 4;
			for(int i = 0; i < nV; i++)
				pV[i] += SphereRadius;
		}
	}
	else
	{
		const fp32 c0 = (sinv + cosv) * _Width;
		const fp32 c1 = (sinv - cosv) * _Width2;

		if(_Flags & FXFLAG_QUAD_PLANE_XY)
		{
			pV[0] = CVec3Dfp32(-c0,  c1, _Height) * Move;
			pV[1] = CVec3Dfp32(-c1, -c0, _Height) * Move;
			pV[2] = CVec3Dfp32( c0, -c1, _Height) * Move;
			pV[3] = CVec3Dfp32( c1,  c0, _Height) * Move;
		}
		else if(_Flags & FXFLAG_QUAD_PLANE_XZ)
		{
			pV[0] = CVec3Dfp32(-c0, _Height,  c1) * Move;
			pV[1] = CVec3Dfp32(-c1, _Height, -c0) * Move;
			pV[2] = CVec3Dfp32( c0, _Height, -c1) * Move;
			pV[3] = CVec3Dfp32( c1, _Height,  c0) * Move;
		}
		else if(_Flags & FXFLAG_QUAD_PLANE_YZ)
		{
			pV[0] = CVec3Dfp32(_Height, -c0,  c1) * Move;
			pV[1] = CVec3Dfp32(_Height, -c1, -c0) * Move;
			pV[2] = CVec3Dfp32(_Height,  c0, -c1) * Move;
			pV[3] = CVec3Dfp32(_Height,  c1,  c0) * Move;
		}

		if(_Flags & FXFLAG_QUAD_HEIGHT)
			pV[4] = CVec3Dfp32(0, 0, 0) * Move;

		Move.Unit();
	}
	
	// No, no, no, NO!!!
	/*
	if(_Flags & FXFLAG_QUAD_THIS_IS_A_NO_NO)
	{
		CVec4Dfp32* pTV2 = _pRD->m_pVBM->Alloc_V4((_Flags & FXFLAG_QUAD_HEIGHT) ? 5 : 4);
		CVec4Dfp32* pTV3 = _pRD->m_pVBM->Alloc_V4((_Flags & FXFLAG_QUAD_HEIGHT) ? 5 : 4);
		const fp32 Perturb = 0.4f; // Set a half strong perturb
		
		if(pTV2)
		{
			pTV2[0] = CVec4Dfp32(Perturb, 0, 1, 0);
			pTV2[1] = CVec4Dfp32(Perturb, 0, 0, 0);
			pTV2[2] = CVec4Dfp32(Perturb, 0, 0, 0);
			pTV2[3] = CVec4Dfp32(Perturb, 0, 1, 0);

			if(_Flags & FXFLAG_QUAD_HEIGHT)
				pTV2[4] = CVec4Dfp32(Perturb, 0, 0.5f, 0);
		}

		if(pTV3)
		{
			const int bRenderTextureVertFlip = (_pRD->m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP);
			if(bRenderTextureVertFlip)
			{
				pTV3[0] = CVec4Dfp32(0, Perturb, 0, 0);
				pTV3[1] = CVec4Dfp32(0, Perturb, 0, 0);
				pTV3[2] = CVec4Dfp32(0, Perturb, 1, 0);
				pTV3[3] = CVec4Dfp32(0, Perturb, 1, 0);

				if(_Flags & FXFLAG_QUAD_HEIGHT)
					pTV3[4] = CVec4Dfp32(0, Perturb, 0.5f, 0);
			}
			else
			{
				pTV3[0] = CVec4Dfp32(0, Perturb, 1, 0);
				pTV3[1] = CVec4Dfp32(0, Perturb, 1, 0);
				pTV3[2] = CVec4Dfp32(0, Perturb, 0, 0);
				pTV3[3] = CVec4Dfp32(0, Perturb, 0, 0);
				
				if(_Flags & FXFLAG_QUAD_HEIGHT)
					pTV3[4] = CVec4Dfp32(0, Perturb, 0.5f, 0);
			}
		}
		
		_pRD->m_pChain->m_pTV[2] = (fp32*)pTV2;
		_pRD->m_pChain->m_pTV[3] = (fp32*)pTV3;
		_pRD->m_pChain->m_nTVComp[2] = (pTV2) ? 4 : 0;
		_pRD->m_pChain->m_nTVComp[3] = (pTV3) ? 4 : 0;
	}
	*/

	if(m_Priority > 0)
	{
		const fp32 Priority = GetPriority(_pRD->m_pFXSystem->m_pRenderParams->m_RenderInfo);
		_pRD->Render_Queue(_SurfaceID, Move, m_pShader, _pTime, &Priority);
	}
	else
		_pRD->Render_Queue(_SurfaceID, Move, m_pShader, _pTime);

	return true;
}


//void CFXDataObject::Render(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, uint32 _Flags)
void CFXDataObject::Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat)
{
	MSCOPESHORT(CFXDataObject::Render);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataShader
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFXDataShader::CFXDataShader()
	: CFXDataObject()
//	, m_Program("")
//	, m_Priority(0)
//	, m_PriorityOffset(0)
	, m_Enable(0)
	, m_Disable(0)
	, m_AlphaRef(128)
	, m_RasterMode(0)
	, m_ZCompare(CRC_COMPARE_LESSEQUAL)
	, m_AlphaCompare(CRC_COMPARE_ALWAYS)
	, m_iTexGen(0)
	, m_nTexGenParams(0)
	, m_nPredefinedParams(0)
{
//	m_lShaderParams.Clear();
//	m_lShaderMaps.Clear();
}


CFXDataShader::CFXDataShader(const CFXDataCollect& _Data)
	: CFXDataObject(_Data)
{
	const uint8 nShaderParams = _Data.m_lShaderParams.Len();
	const uint8 nShaderMaps = _Data.m_lShaderMaps.Len();

	m_Program = _Data.m_ProgramName;
	//m_Priority = _Data.m_Priority;
	//m_PriorityOffset = _Data.m_PriorityOffset;
	m_RasterMode = _Data.m_RasterMode;
	m_ZCompare = _Data.m_ZCompare;
	m_AlphaCompare = _Data.m_AlphaCompare;
	m_AlphaRef = _Data.m_AlphaRef;
	m_Enable = _Data.m_Enable;
	m_Disable = _Data.m_Disable;
	m_lShaderParams.SetLen(nShaderParams);
	m_lShaderMaps.SetLen(nShaderMaps);

	m_iTexGen = _Data.m_iTexGen;
	m_nTexGenParams = 0;
	m_nPredefinedParams = 5;
	switch(m_iTexGen)
	{
		case FXSHADER_TEXGEN_SCREENCOORD:
			//m_nPredefinedParams = 5;
			m_nTexGenParams = 4;
			break;

		case FXSHADER_TEXGEN_EMITPOSITION:
			m_nTexGenParams = 0;
			//m_nPredefinedParams = 0;
			break;
	}

	// Copy parameters and surface maps
	memcpy(m_lShaderParams.GetBasePtr(), _Data.m_lShaderParams.GetBasePtr(), sizeof(CVec4Dfp32) * nShaderParams);
	memcpy(m_lShaderMaps.GetBasePtr(), _Data.m_lShaderMaps.GetBasePtr(), sizeof(int32) * nShaderMaps);

	m_Flags = ((CStr)_Data.m_Flags).TranslateFlags(g_lpEFFECTSYSTEM_SHADER_FLAGS);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataRenderTarget
|	Note: Used for handling the post processing layer in engine object
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFXDataRenderTarget::CFXDataRenderTarget()
	: CFXDataObject()
{
}

CFXDataRenderTarget::CFXDataRenderTarget(const CFXDataCollect& _Data)
	: CFXDataObject(_Data)
{
	m_RenderTarget = _Data.m_RenderTarget;
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataBeam
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFXDataBeam::CFXDataBeam()
	: CFXDataObject()
	, m_Seed(0)
	, m_nMulti(1)
{
	m_lSurfaceID.Clear();
	m_lWidth.Clear();
	m_lLength.Clear();
	m_lColor.Clear();
	m_lPosOffset.Clear();
	m_lDirection.Clear();
	m_lSegments.Clear();
	//m_lNoises.Clear();
	//m_lpExtLayers.Clear();
}


CFXDataBeam::CFXDataBeam(const CFXDataCollect& _Data)
	: CFXDataObject(_Data)
	, m_Seed(_Data.m_Seed)
	, m_nMulti(_Data.m_nMulti)
{
	m_lSurfaceID.Add(_Data.m_lSurfaceID);
	m_lWidth.Add(_Data.m_lWidth);
	m_lLength.Add(_Data.m_lLength);
	m_lColor.Add(_Data.m_lColor);
	m_lPosOffset.Add(_Data.m_lPosOffset);
	m_lDirection.Add(_Data.m_lDirection);
	m_lSegments.Add(_Data.m_lSegments);
	
	//m_lNoises.Add(_Data.m_lNoiseLayer);
	//m_lpExtLayers.Add(_Data.m_lpExtLayers);
	m_Layer = _Data.m_Layer;

	m_Flags = ((CStr)_Data.m_Flags).TranslateFlags(g_lpEFFECTSYSTEM_BEAM_FLAGS);
	m_RandFlags = ((CStr)_Data.m_RandomFlags).TranslateFlags(g_lpEFFECTSYSTEM_BEAM_RANDFLAGS);
}


void CFXDataBeam::CalcBound_Box(const CXR_AnimState* _pAnimState)
{
	if(!(m_IntFlags & FXFLAGS_INT_HASCALCEDBOUNDBOX))
	{
		const int nWidth = m_lWidth.Len();
		const int nLength = m_lLength.Len();
		const fp32* pWidth = m_lWidth.GetBasePtr();
		const fp32* pLength = m_lLength.GetBasePtr();
		fp32 MaxBound = 0.0f;

		if(nWidth == nLength)
		{
			for(int i = 0; i < nWidth; i++)
				MaxBound = MaxMT(MaxMT(M_Fabs(pWidth[i]), MaxBound), M_Fabs(pLength[i]));
		}
		else
		{
			for(int i = 0; i < nWidth; i++)
				MaxBound = MaxMT(M_Fabs(pWidth[i]), MaxBound);

			for(int i = 0; i < nLength; i++)
				MaxBound = MaxMT(M_Fabs(pLength[i]), MaxBound);
		}
		
		m_BoundBox = CBox3Dfp32(-MaxBound,MaxBound);
	}
}


//void CFXDataBeam::Render(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, uint32 _Flags)
void CFXDataBeam::Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat)
{
	MSCOPESHORT(CFXDataBeam::Render);

	CEffectSystemHistory::CEffectSystemHistoryObject* pObjectHistory = NULL;
	if(m_Flags & FXFLAG_BEAM_ALLOWHISTORY)
		pObjectHistory = _pRD->m_pHistory->GetHistory(m_UseHistoryObject);

	const CMat4Dfp32& WMat = _pRD->m_WMat;
	const CMat4Dfp32& VMat = _pRD->m_VMat;

	const fp32* pWidth = m_lWidth.GetBasePtr();
	const fp32* pDuration = m_lDuration.GetBasePtr();
	const int* pSurfaceID = m_lSurfaceID.GetBasePtr();
	const fp32* pLength = m_lLength.GetBasePtr();
	const uint32* pColor = m_lColor.GetBasePtr();
	const CVec3Dfp32* pPosOffset = m_lPosOffset.GetBasePtr();
	const CVec3Dfp32* pDirection = m_lDirection.GetBasePtr();
	//const CFXLayerNoise* pNoises = m_lNoises.GetBasePtr();
	//CFXLayer** pExtLayer = m_lpExtLayers.GetBasePtr();
	//const int nNoises = m_lNoises.Len();
	//const int nExtLayers = m_lpExtLayers.Len();
	const uint8* pSegments = m_lSegments.GetBasePtr();

	// Retrive keys
	GETKEY(LerpTimeAnim, _pRD->m_Time0, iCurKey, iNextKey, m_Flags);
	
	uint32 Rand = (m_Seed > 0) ? m_Seed : _pRD->m_Anim0 + _pRD->m_Seeding;
	fp32 DurationRand = pDuration[0];
	const fp32 RecipRand = 1.0f / DurationRand;
	const fp32 LerpTimeRand = MinMT( ((DurationRand - (DurationRand - _pRD->m_Time0)) * RecipRand), 1.0f);

	// Recreate effect
	CVec3Dfp32 PosOffset;
	const int SurfaceID	= (pSurfaceID) ? ((m_RandFlags & FXRAND_BEAM_SURFACE) ? pSurfaceID[_pRD->m_Anim0 % m_lSurfaceID.Len()] : pSurfaceID[iCurKey]) : 0;
	const fp32 Width = CFXSysUtil::LerpMixMT((m_RandFlags & FXRAND_BEAM_WIDTH) != 0, pWidth, Rand, iCurKey, iNextKey, LerpTimeRand, LerpTimeAnim);
	const fp32 Length = CFXSysUtil::LerpMixMT((m_RandFlags & FXRAND_BEAM_LENGTH) != 0, pLength, Rand, iCurKey, iNextKey, LerpTimeRand, LerpTimeAnim);
	const uint32 Color = CFXSysUtil::LerpColorMix((m_RandFlags & FXRAND_BEAM_COLOR) != 0, pColor, Rand, iCurKey, iNextKey, LerpTimeRand, LerpTimeAnim);
	const uint8 nSegments = (uint8)((pSegments) ? CFXSysUtil::LerpInt(pSegments[iCurKey], pSegments[iNextKey], LerpTimeAnim)+1 : 2);
	const uint8 nMulti = m_nMulti;
	pPosOffset[iCurKey].Lerp(pPosOffset[iNextKey], LerpTimeAnim, PosOffset);

	// If this beam isn't history controlled we render it here
	if(!(m_Flags & FXFLAG_BEAM_ALLOWHISTORY) && _pRD->m_Time0 > 0)
	{
		TThinArray<CXR_BeamStrip> m_lBeam;
		m_lBeam.SetLen(nMulti*(nSegments));
		CXR_BeamStrip* pBeam = m_lBeam.GetBasePtr();
	
		//CXR_BeamStrip Beam[2];

		const fp32 RcpLength = Length / (fp32)nSegments; //1.0f / Length;//(fp32)nSegments / Length;
		CVec3Dfp32 RcpLength3D = 0;
		if(m_Flags & FXFLAG_BEAM_PLANE_X)
			RcpLength3D.k[0] = RcpLength;
		else if(m_Flags & FXFLAG_BEAM_PLANE_Y)
			RcpLength3D.k[1] = RcpLength;
		else if(m_Flags & FXFLAG_BEAM_PLANE_Z)
			RcpLength3D.k[2] = RcpLength;

		for(uint8 iMulti = 0; iMulti < nMulti; iMulti++)
		{
			for(uint8 iSegment = 0; iSegment < nSegments-1; iSegment++)
			{
				//pBeam[0].m_Width = Width;
				//pBeam[1].m_Width = Width;
				//pBeam[0].m_Pos = PosOffset;

				pBeam[0].m_Width = pBeam[1].m_Width = Width;
				pBeam[0].m_Color = Color;  pBeam[1].m_Color = Color;
				pBeam[0].m_TextureYOfs = 0;
				pBeam[1].m_TextureYOfs = 1;

				//pBeam[iSegment].m_Pos = PosOffset;

				//if(m_Flags & FXFLAG_BEAM_PLANE_X)
				//	pBeam[1].m_Pos = PosOffset + CVec3Dfp32(Length, 0, 0);
				//else if(m_Flags & FXFLAG_BEAM_PLANE_Y)
				//	pBeam[1].m_Pos = PosOffset + CVec3Dfp32(0, Length, 0);
				//else if(m_Flags & FXFLAG_BEAM_PLANE_Z)
				//	pBeam[1].m_Pos = PosOffset + CVec3Dfp32(0, 0, Length);
				//else if(m_Flags & FXFLAG_BEAM_PLANE_ALL)
				if(m_Flags & FXFLAG_BEAM_PLANE_ALL)
				{
					CVec3Dfp32 Direction;
					if(m_RandFlags & FXRAND_BEAM_DIRECTION)
						Direction = CVec3Dfp32(-1.0f + (2.0f*MFloat_GetRand(Rand)), -1.0f + (2.0f*MFloat_GetRand(Rand+1)), -1.0f + (2.0f*MFloat_GetRand(Rand+2)));
					else
						pDirection[iCurKey].Lerp(pDirection[iNextKey], LerpTimeAnim, Direction);
					
					Rand += 3;
					//pBeam[1].m_Pos = PosOffset + (Direction * RcpLength);
					pBeam[0].m_Pos = PosOffset + (Direction * RcpLength * iSegment);
					pBeam[1].m_Pos = PosOffset + (Direction * RcpLength * (iSegment+1));
				}
				else
				{
					pBeam[0].m_Pos = PosOffset + (RcpLength3D * iSegment);
					pBeam[1].m_Pos = PosOffset + (RcpLength3D * (iSegment+1));
				}
					//pBeam[1].m_Pos = PosOffset + CVec3Dfp32(Length, 0, 0);
				
				//if(m_Flags & FXFLAG_BEAM_EDGEFADE)
				//{
					//pBeam[0].m_Color = 0xffffffff;
					//pBeam[1].m_Color = 0xffffffff;
					//pBeam[0].m_Color = Color;
					//pBeam[1].m_Color = Color;
				//}
				//else
				//{
				//	pBeam[0].m_Color = Color;
				//	pBeam[1].m_Color = Color;
				//}
				//pBeam[iSegment].m_Color = Color;
				//pBeam[iSegment].m_TextureYOfs = 0;
				//pBeam[0].m_TextureYOfs = 0;
				//pBeam[1].m_TextureYOfs = 0;

				pBeam++;
				pBeam++;
			}

			// Apply noise layer
			pBeam = m_lBeam.GetBasePtr();
			/*
			for(int j = 0; j < nNoises; j++)
			{
				CVec3Dfp32 RandPos = 0;
				for(uint8 iSegment = 0; iSegment < nSegments-1; iSegment++)
				{
					pBeam[0].m_Pos += RandPos;
					
					RandPos.k[0] = -1.0f + (2.0f*MFloat_GetRand(Rand++));
					RandPos.k[1] = -1.0f + (2.0f*MFloat_GetRand(Rand++));
					RandPos.k[2] = -1.0f + (2.0f*MFloat_GetRand(Rand++));
					RandPos *= 0.4f;

					pBeam[1].m_Pos += RandPos;
					
					pBeam++;
					pBeam++;
				}
			}
			*/
		}
		
		// New render code
		CFXDataObject::Render_BeamStrip(_pRD, SurfaceID, m_lBeam.GetBasePtr(), nMulti*(nSegments), CXR_BEAMFLAGS_EDGEFADE | CXR_BEAMFLAGS_BEGINCHAIN, &WMat, &VMat);
		m_lBeam.Clear();
	}
	else if(m_Flags & FXFLAG_BEAM_ALLOWHISTORY)
	{
        // Render single beams history
		//CMat4Dfp32 WMat;
		CXR_BeamStrip Beam[2];
		bool bSpawn = (_pRD->m_Time0 > 0) ? true : false;

		CMat4Dfp32 UnitMat;
		UnitMat.Unit();
		//WMat.Unit();

		uint16 nHistoryEntries = 0;
		uint8* pDataChunk = pObjectHistory->GetDataBasePtrNumEntries(nHistoryEntries);
		
		if(nHistoryEntries > 0)
		{
			bSpawn = false;

			for(int i = 0; i < nHistoryEntries; i++)
			{
				CEffectSystemHistory::CFXDataBeamHistory& BeamHistory = GETBEAMENTRY(pDataChunk);

				// Is it time to kill this beam?
				if(BeamHistory.m_Time + m_HistoryTime < _pRD->m_Time1)
				{
					pDataChunk = pObjectHistory->RemoveUpdate(i--);
					nHistoryEntries--;
					continue;
				}

				const fp32 HAnimTime = _pRD->m_Time1 - BeamHistory.m_Time;
				GETKEY(LerpTimeHAnim, HAnimTime, iCurHKey, iNextHKey, _pRD->m_Flags);
				const uint32 HColor = CFXSysUtil::LerpColor(pColor[iCurHKey], pColor[iNextHKey], LerpTimeHAnim);

				// First beam edge
				Beam[0].m_Width = Width;
				Beam[0].m_Pos = BeamHistory.m_Position;
				Beam[0].m_Color = HColor; //HistoryColor;
				Beam[0].m_TextureYOfs = 0;

				// Second building it
				Beam[1].m_Width = Width;
				Beam[1].m_Color = HColor; //HistoryColor;
				Beam[1].m_TextureYOfs = 0;
				Beam[1].m_Pos = BeamHistory.m_Position + (BeamHistory.m_Direction * Length);

				CMTime AnimTimeEntry;
				AnimTimeEntry = CMTime::CreateFromSeconds(_pRD->m_Time1 - BeamHistory.m_Time);
				
				// New Render Code
				CFXDataObject::Render_BeamStrip(_pRD, SurfaceID, Beam, 2, FXFLAG_BEAM_EDGEFADE, &UnitMat, &VMat);
			}

			if(_pRD->m_Time0 > 0)
			{
				const uint16 nEntriesLeft = pObjectHistory->GetNumEntries();
				if(nEntriesLeft > 0)
				{
					const CEffectSystemHistory::CFXDataBeamHistory& LastEntry = *pObjectHistory->GetBeamEntry(nEntriesLeft-1);
					if(_pRD->m_Time1 - LastEntry.m_Time > _pRD->m_Time0)//LastEntry.m_Time + 3.0f <= Time1)
					{
						CEffectSystemHistory::CFXDataBeamHistory Entry;
						Entry.m_Position = CVec3Dfp32::GetMatrixRow(_WMat, 3);
						Entry.m_Direction = CVec3Dfp32::GetMatrixRow(_WMat, 0);
						Entry.m_Time = _pRD->m_Time1;
						
						pObjectHistory->AddEntry(&Entry);
					}
				}
				else
					bSpawn = true;
			}
		}
		
		if(bSpawn)
		{
			// Add new beam
 			CEffectSystemHistory::CFXDataBeamHistory Entry;
			Entry.m_Position = CVec3Dfp32::GetMatrixRow(_WMat, 3);
			Entry.m_Direction = CVec3Dfp32::GetMatrixRow(_WMat, 0);
			Entry.m_Time = _pRD->m_Time1;

			pObjectHistory->AddEntry(&Entry);
		}
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataLight
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFXDataLight::CFXDataLight()
{
	m_lPosOffset.Clear();
	m_lIntensity.Clear();
	m_lRange.Clear();
}


CFXDataLight::CFXDataLight(const CFXDataCollect& _Data)
	: CFXDataObject(_Data)
{
	m_lPosOffset.Add(_Data.m_lPosOffset);
	m_lIntensity.Add(_Data.m_lIntensity);
	m_lRange.Add(_Data.m_lRange);

	m_Flags = ((CStr)_Data.m_Flags).TranslateFlags(g_lpEFFECTSYSTEM_LIGHT_FLAGS);
}


//void CFXDataLight::Render(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, uint32 _Flags)
//void CFXDataLight::RenderVis(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat)
void CFXDataLight::RenderVis(CWorld_Client* _pWClient, CXR_SceneGraphInstance* _pSGI, uint16& _LightGUID, int _iOwner, fp32 _AnimTime, const CMat4Dfp32& _WMat, uint32 _Flags)
{
	const fp32* pRange = m_lRange.GetBasePtr();
	const CVec3Dfp32* pIntensity = m_lIntensity.GetBasePtr();
	const CVec3Dfp32* pPosOffset = m_lPosOffset.GetBasePtr();
	GETKEY(LerpTimeAnim, _AnimTime, iCurKey, iNextKey, _Flags);
	//uint8 iCurKey, iNextKey;
	//const fp32 LerpTimeAnim = GetKey(_AnimTime, iCurKey, iNextKey, _Flags);
	
	int Flags = 0;
	if(m_Flags & FXFLAG_LIGHT_NOSHADOWS)	Flags |= CXR_LIGHT_NOSHADOWS;
	if(m_Flags & FXFLAG_LIGHT_NODIFFUSE)	Flags |= CXR_LIGHT_NODIFFUSE;
	if(m_Flags & FXFLAG_LIGHT_NOSPECULAR)	Flags |= CXR_LIGHT_NOSPECULAR;
	if(m_Flags & FXFLAG_LIGHT_ANIMTIME)		Flags |= CXR_LIGHT_ANIMTIME;

	// Retrive keys
	CVec3Dfp32 Intensity, PosOffset;
	pIntensity[iCurKey].Lerp(pIntensity[iNextKey], LerpTimeAnim, Intensity);
	pPosOffset[iCurKey].Lerp(pPosOffset[iNextKey], LerpTimeAnim, PosOffset);
	const fp32 Range = CFXSysUtil::LerpMT(pRange[iCurKey], pRange[iNextKey], LerpTimeAnim);
	
	// Apply position offset to light
	CMat4Dfp32 LightMat = _WMat;
	LightMat.GetRow(3) += (LightMat.GetRow(0) * PosOffset.k[0]) + (LightMat.GetRow(1) * PosOffset.k[1]) + (LightMat.GetRow(2) * PosOffset.k[2]);
	CXR_Light Light(LightMat, Intensity, Range, Flags, CXR_LIGHTTYPE_POINT);
	Light.m_LightGUID = _LightGUID++;
	_pSGI->SceneGraph_Light_LinkDynamic(Light);

	// Send message back to owner about which light to exclude
	if((m_Flags & FXFLAG_LIGHT_EXCLUDEOWNER) && _iOwner)
	{
		CWObject_Message Msg(OBJMSG_EFFECTSYSTEM_EXCLUDEOWNER, Light.m_LightGUID);
		_pWClient->ClientMessage_SendToObject(Msg, _iOwner);
	}
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataWallmark
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFXDataWallmark::CFXDataWallmark()
{
	m_lWidth.Clear();
	m_lTolerance.Clear();
	m_lSurfaceID.Clear();
}

CFXDataWallmark::CFXDataWallmark(const CFXDataCollect& _Data)
	: CFXDataObject(_Data)
{
	m_lWidth.Add(_Data.m_lWidth);
	m_lTolerance.Add(_Data.m_lTolerance);
	m_lSurfaceID.Add(_Data.m_lSurfaceID);
}

void CFXDataWallmark::RenderClient(const CXR_SurfaceContext* _pSurfaceContext, CWorld_Client* _pWClient, const CMTime& _AnimTime, const CMat4Dfp32& _WMat, uint32 _Flags)
{
	const fp32* pWidth = m_lWidth.GetBasePtr();
	const fp32* pTolerance = m_lTolerance.GetBasePtr();
	const int* pSurfaceID = m_lSurfaceID.GetBasePtr();
		
	// Retrive keys
	GETKEY(LerpTimeAnim, _AnimTime.GetTime(), iCurKey, iNextKey, _Flags);
	const fp32 Width = CFXSysUtil::LerpMT(pWidth[iCurKey], pWidth[iNextKey], LerpTimeAnim);
	const fp32 Tolerance = CFXSysUtil::LerpMT(pTolerance[iCurKey], pTolerance[iNextKey], LerpTimeAnim);
	const int SurfaceID = CFXSysUtil::LerpMT(pSurfaceID[iCurKey], pSurfaceID[iNextKey], LerpTimeAnim);

	CXR_WallmarkDesc WMD;
	WMD.m_SurfaceID = SurfaceID;
	WMD.m_Size = Width;
	WMD.m_SpawnTime = _pWClient->GetGameTime();
	WMD.m_SpawnTime = _AnimTime;
	WMD.m_SpawnTime = _pWClient->GetGameTime() + _AnimTime;
	WMD.m_SpawnTime = _pWClient->GetGameTime() - _AnimTime;
	WMD.m_Flags = XR_WALLMARK_NOFADE;

	// Create wallmark
	//_pWClient->Wallmark_Create(_WMat, Width, Tolerance, _AnimTime, ((CXR_SurfaceContext*)_pSurfaceContext)->GetSurfaceName(SurfaceID), XR_WALLMARK_TEMPORARY);
	//_pWClient->Wallmark_Create(_WMat, Width, Tolerance, _pWClient->GetGameTime(), ((CXR_SurfaceContext*)_pSurfaceContext)->GetSurfaceName(SurfaceID), XR_WALLMARK_TEMPORARY | XR_WALLMARK_ALLOW_PERPENDICULAR);
	//_pWClient->Wallmark_Create(_WMat, Width, Tolerance, _pWClient->GetGameTime(), ((CXR_SurfaceContext*)_pSurfaceContext)->GetSurfaceName(SurfaceID), XR_WALLMARK_TEMPORARY | XR_WALLMARK_NOFADE);
	_pWClient->Wallmark_Create(WMD, _WMat, Tolerance, XR_WALLMARK_TEMPORARY);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataModel
|__________________________________________________________________________________________________
\*************************************************************************************************/
void CFXDataModel::Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat)
{
	// Render model
	MSCOPESHORT(CFXDataModel::Render);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataQuad
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFXDataQuad::CFXDataQuad()
	: CFXDataObject()
	, m_Seed(0)
	, m_iRenderType(0)
{
	m_lSurfaceID.Clear();
	m_lWidth.Clear();
	m_lWidth2.Clear();
	m_lRotationAngle.Clear();
	m_lColor.Clear();
	m_lHeight.Clear();
	m_lPositionNoise.Clear();
	m_lSphereRadius.Clear();
	m_lPosOffset.Clear();
}


CFXDataQuad::CFXDataQuad(const CFXDataCollect& _Data)
	: CFXDataObject(_Data)
	, m_Seed(_Data.m_Seed)
{
	m_lSurfaceID.Add(_Data.m_lSurfaceID);
	m_lWidth.Add(_Data.m_lWidth);
	m_lWidth2.Add(_Data.m_lWidth2);
	m_lRotationAngle.Add(_Data.m_lRotationAngle);
	m_lColor.Add(_Data.m_lColor);
	m_lHeight.Add(_Data.m_lHeight);
	m_lPositionNoise.Add(_Data.m_lPosNoise);
	m_lSphereRadius.Add(_Data.m_lSphereRadius);
	m_lPosOffset.Add(_Data.m_lPosOffset);

	CalcMaxBound_Box();

	if(_Data.m_RenderType == "")
		m_iRenderType = 0;
	else
		m_iRenderType = ((CStr)_Data.m_RenderType).TranslateInt(g_lpEFFECTSYSTEM_RenderType);
	m_Flags = ((CStr)_Data.m_Flags).TranslateFlags(g_lpEFFECTSYSTEM_QUAD_FLAGS);
	m_RandFlags = ((CStr)_Data.m_RandomFlags).TranslateFlags(g_lpEFFECTSYSTEM_QUAD_RANDFLAGS);
}

void CFXDataQuad::CalcMaxBound_Box()
{
	m_MaxBound = 0.0f;

	const int nWidth = m_lWidth.Len();
	const fp32* pWidth = m_lWidth.GetBasePtr();
	for(int i = 0; i < nWidth; i++)
		m_MaxBound = MaxMT(m_MaxBound, M_Fabs(pWidth[i]) * _SQRT2);

	const int nWidth2 = m_lWidth2.Len();
	const fp32* pWidth2 = m_lWidth2.GetBasePtr();
	for(int i = 0; i < nWidth2; i++)
		m_MaxBound = MaxMT(m_MaxBound, M_Fabs(pWidth2[i]) * _SQRT2);

	const int nHeight = m_lHeight.Len();
	const fp32* pHeight = m_lHeight.GetBasePtr();
	for(int i = 0; i < nHeight; i++)
		m_MaxBound = MaxMT(m_MaxBound, M_Fabs(pHeight[i]));

	const int nSphereRadius = m_lSphereRadius.Len();
	const fp32* pSphereRadius = m_lSphereRadius.GetBasePtr();
	for(int i = 0; i < nSphereRadius; i++)
		m_MaxBound = MaxMT(m_MaxBound, M_Fabs(pSphereRadius[i]));
}

void CFXDataQuad::CalcBound_Box(const CXR_AnimState* _pAnimState)
{
	if(!(m_IntFlags & FXFLAGS_INT_HASCALCEDBOUNDBOX))
	{
		//fp32 Width = M_Fabs((m_lWidth[0] * 0.5f));
		m_BoundBox = CBox3Dfp32(-m_MaxBound, m_MaxBound);
	}
}

fp32 CFXDataQuad::CalcBound_Sphere(const CXR_AnimState* _pAnimState)
{
	return m_MaxBound;
}

void CFXDataObject::RenderQuad(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, int _SurfaceID, const CMTime& _AnimTime, const CVec3Dfp32* _pV, const uint32* _pC, const uint16* _pI, const CVec2Dfp32* _pTV, uint32 _Flags)
{
	int nV = 4;
	int nP = 2;
	int nI = 6;

	// Initialize counters for quad
	if(_Flags & FXFLAG_RENDERQUAD_X4)
	{
		nV++;
		nP += 2;
		nI += 6;
	}
	
	// Allocate memory
	CXR_VertexBuffer* pVB = _pFXSystem->AllocVB(_pFXSystem->m_pRenderParams);
	CVec3Dfp32* pV = _pVBM->Alloc_V3(nV);
	CPixel32* pC = (CPixel32*)_pVBM->Alloc_Int32(nV);
	CVec2Dfp32* pTV = _pVBM->Alloc_V2(nV);
	uint16* pI = _pVBM->Alloc_Int16(nI);

	if (!pVB->AllocVBChain(_pVBM, false))
		return;

	// Copy down data
	memcpy(pV, _pV, nV * 12);
	
	// Color data
	if(_pC)
		memcpy(pC, _pC, nV * 4);
	else
	{
		const uint32 C[5] = { 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff, 0xffffffff };
		memcpy(pC, C, nV * 4);
	}

	// Index data
	if(_pI)
		memcpy(pI, _pI, nI * 2);
	else
	{
		if(_Flags & FXFLAG_RENDERQUAD_X2)
		{
			const uint16 I[6] = { 0, 1, 2, 0, 2, 3 };
			memcpy(pI, I, nI * 2);
		}
		else
		{
			const uint16 I[12] = { 0, 1, 4, 1, 2, 4, 2, 3, 4, 3, 0, 4 };
			memcpy(pI, I, nI * 2);
		}
		
	}

	// UV data
	if(_pTV)
		memcpy(pTV, _pTV, nV * 8);
	else
	{
		const CVec2Dfp32 TV[5] = { CVec2Dfp32(0,0), CVec2Dfp32(0,1), CVec2Dfp32(1,1), CVec2Dfp32(1,0), CVec2Dfp32(0.5f,0.5f) };
		memcpy(pTV, TV, nV * 8);
	}

	// Set geometry array
	pVB->Geometry_VertexArray(pV, nV, true);
	pVB->Geometry_TVertexArray(pTV, 0);
	pVB->Geometry_ColorArray(pC);

	// Render quad
	pVB->Render_IndexedTriangles(pI, nP);
	_pFXSystem->Render_Surface(_pFXSystem->m_pRenderParams, _SurfaceID, pVB, _AnimTime);
}

void CFXDataQuad::CreateConeScreenQuad_Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat)
{
	//	const CMTime& AnimTime0 = _pRD->m_AnimTime0;
	fp32 Time0 = _pRD->m_Time0;
//	const CMTime& AnimTime1 = _pRD->m_AnimTime1;
	fp32 Time1 = _pRD->m_Time1;

	const CMat4Dfp32& WMat = _pRD->m_WMat;
	const CMat4Dfp32& VMat = _pRD->m_VMat;

	const fp32* pWidth = m_lWidth.GetBasePtr();
	const fp32* pDuration = m_lDuration.GetBasePtr();
	const fp32* pRotationAngle = m_lRotationAngle.GetBasePtr();
	const int* pSurfaceID = m_lSurfaceID.GetBasePtr();
	const uint32* pColor = m_lColor.GetBasePtr();
	const fp32* pHeight = m_lHeight.GetBasePtr();
	const fp32* pWidth2 = m_lWidth2.GetBasePtr();
	const CVec3Dfp32* pPosOffset = m_lPosOffset.GetBasePtr();
	//const fp32* pSphereRadius = m_lSphereRadius.GetBasePtr();
	//const CVec3Dfp32* pPosNoise = m_lPositionNoise.GetBasePtr();
	
	// Retrive keys
	GETKEY(LerpTimeAnim, Time0, iCurKey, iNextKey, _pRD->m_Flags);
	
	uint32 Rand = (m_Seed > 0) ? m_Seed : _pRD->m_Anim0 + _pRD->m_Seeding;
	fp32 DurationRand = pDuration[0];
	const fp32 RecipRand = 1.0f / DurationRand;
	const fp32 LerpTimeRand = MinMT( ((DurationRand - (DurationRand - Time0)) * RecipRand), 1.0f);

	// Recreate effect
	const int SurfaceID = (pSurfaceID) ? ((m_RandFlags & FXRAND_QUAD_SURFACE) ? pSurfaceID[_pRD->m_Anim0 % m_lSurfaceID.Len()] : pSurfaceID[iCurKey]) : 0;
	const uint32 Color = CFXSysUtil::LerpColorMix((m_RandFlags & FXRAND_QUAD_COLOR) != 0, pColor, Rand, iCurKey, iNextKey, LerpTimeRand, LerpTimeAnim);
	const fp32 Width = CFXSysUtil::LerpMixMT((m_RandFlags & FXRAND_QUAD_WIDTH) != 0, pWidth, Rand, iCurKey, iNextKey, LerpTimeRand, LerpTimeAnim);
	const fp32 Angle = CFXSysUtil::LerpMixMT((m_RandFlags & FXRAND_QUAD_ROTATEANGLE) != 0, pRotationAngle, Rand, iCurKey, iNextKey, LerpTimeRand, LerpTimeAnim);
//	const fp32 Angle = (m_RandFlags & FXRAND_QUAD_ROTATEANGLE) ? FXLERP(pRotationAngle[0], pRotationAngle[1], MFloat_GetRand(Rand + 0)) +
//																(FXLERP(pRotationAngle[0], pRotationAngle[1], MFloat_GetRand(Rand + 1)) * LerpTimeRand) :
//																FXLERP(pRotationAngle[iCurKey], pRotationAngle[iNextKey], LerpTimeAnim);
//	if(m_RandFlags & FXRAND_QUAD_ROTATEANGLE)
//		Rand	+= 2;
	const fp32 Height = (m_Flags & FXFLAG_QUAD_HEIGHT) ? -CFXSysUtil::LerpMT(pHeight[iCurKey], pHeight[iNextKey], LerpTimeAnim) : 0;
	const fp32 Width2 = (m_Flags & FXFLAG_QUAD_WIDTH2) ?  CFXSysUtil::LerpMT(pWidth2[iCurKey], pWidth2[iNextKey], LerpTimeAnim) : Width;
	//const fp32 SphereRadius = (pSphereRadius) ? FXLERP(pSphereRadius[iCurKey], pSphereRadius[iNextKey], LerpTimeAnim) : 0.0f;
	CVec3Dfp32 PosOffset = 0;
	if(pPosOffset)
	{
		pPosOffset[iCurKey].Lerp(pPosOffset[iNextKey], LerpTimeAnim, PosOffset);
		CVec3Dfp32 Dir = _WMat.GetRow(2);
		PosOffset.k[0] *= Dir.k[0];
		PosOffset.k[1] *= Dir.k[1];
		PosOffset.k[2] *= Dir.k[2];
	}

	fp32 sinv, cosv;
	QSinCos(Angle, sinv, cosv);

	if(m_Flags & FXFLAG_QUAD_BILLBOARD)
	{
	}
	else
	{
		CVec3Dfp32 Points[8];
		CVec3Dfp32 EmitPos;
		const fp32 c0 = (sinv + cosv) * Width;
		const fp32 c1 = (sinv - cosv) * Width2;
		CRC_Viewport* pVP = _pRD->m_pVBM->Viewport_Get();
		CMat4Dfp32 ProjMat = pVP->GetProjectionMatrix();
		CMat4Dfp32 ProjMat2, L2VMat;
		CMat4Dfp32 InvVMat;

		WMat.Multiply(VMat, L2VMat);
		ProjMat.k[1][1] = -ProjMat.k[1][1];
		L2VMat.Multiply(ProjMat, ProjMat2);
		VMat.InverseOrthogonal(InvVMat);

		if(m_Flags & FXFLAG_QUAD_PLANE_XY)
		{
			EmitPos = CVec3Dfp32(0, 0, Height) + PosOffset;
			Points[0] = CVec3Dfp32(-c0,  c1, 0) + PosOffset;
			Points[1] = CVec3Dfp32(-c1, -c0, 0) + PosOffset;
			Points[2] = CVec3Dfp32( c0, -c1, 0) + PosOffset;
			Points[3] = CVec3Dfp32( c1,  c0, 0) + PosOffset;
		}
		else if(m_Flags & FXFLAG_QUAD_PLANE_XZ)
		{
			EmitPos = CVec3Dfp32(0, Height, 0) + PosOffset;
			Points[0] = CVec3Dfp32(-c0, 0,  c1) + PosOffset;
			Points[1] = CVec3Dfp32(-c1, 0, -c0) + PosOffset;
			Points[2] = CVec3Dfp32( c0, 0, -c1) + PosOffset;
			Points[3] = CVec3Dfp32( c1, 0,  c0) + PosOffset;
		}
		else if(m_Flags & FXFLAG_QUAD_PLANE_YZ)
		{
			EmitPos = CVec3Dfp32(Height, 0, 0) + PosOffset;
			Points[0] = CVec3Dfp32(0, -c0,  c1) + PosOffset;
			Points[1] = CVec3Dfp32(0, -c1, -c0) + PosOffset;
			Points[2] = CVec3Dfp32(0,  c0, -c1) + PosOffset;
			Points[3] = CVec3Dfp32(0,  c1,  c0) + PosOffset;
		}

		Points[4] = Points[0] + (Points[0] - EmitPos);
		Points[5] = Points[1] + (Points[1] - EmitPos);
		Points[6] = Points[2] + (Points[2] - EmitPos);
		Points[7] = Points[3] + (Points[3] - EmitPos);

		{
			if (!_pRD->CreateNewVBChain(8, CXR_VB_ATTRIB | CXR_VB_VERTICES | CXR_VB_TVERTICES0))
				return;

			CVec3Dfp32* pV = _pRD->m_pChain->m_pV;
			CVec2Dfp32* pTV = (CVec2Dfp32*)_pRD->m_pChain->m_pTV[0];
			_pRD->SetupIndices(12, CXR_Model_EffectSystem::m_lBoxTriangles);
						
			if(!pV || !pTV)
				return;

			CVec3Dfp32::MultiplyMatrix(Points, pV, L2VMat, 8);

			CVec4Dfp32 Dir;
			{
				CRct r = pVP->GetViewRect();
				CRct ra = pVP->GetViewArea();
				fp32 xMid = (r.p0.x+r.p1.x) >> 1;
				fp32 yMid = (r.p0.y+r.p1.y) >> 1;
				fp32 xMidA = (ra.p0.x+ra.p1.x) >> 1;
				fp32 yMidA = (ra.p0.y+ra.p1.y) >> 1;

				CVec4Dfp32::MultiplyMatrix(&EmitPos, &Dir, ProjMat2, 1);
				const fp32 DirTemp = 1.0f / Dir.k[3];
				Dir.k[0] = (Dir.k[0] * DirTemp) + (xMid - (xMid - xMidA));
				Dir.k[1] = (Dir.k[1] * DirTemp) + (yMid - (yMid - yMidA));
			}

			pTV[4] = pTV[0] = CVec2Dfp32(Dir.k[0], -Dir.k[1]);
			pTV[5] = pTV[1] = CVec2Dfp32(Dir.k[0], -Dir.k[1]);
			pTV[6] = pTV[2] = CVec2Dfp32(Dir.k[0], -Dir.k[1]);
			pTV[7] = pTV[3] = CVec2Dfp32(Dir.k[0], -Dir.k[1]);

			CMat4Dfp32 Unit;
			Unit.Unit();
			
			if(m_Priority > 0)
			{
				const fp32 Priority = GetPriority(_pRD->m_pFXSystem->m_pRenderParams->m_RenderInfo);
				_pRD->Render_Queue(0, Unit, m_pShader, NULL, &Priority);
			}
			else
				_pRD->Render_Queue(0, Unit, m_pShader);
		}
	}
}

void CFXDataQuad::Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat)
{
	MSCOPESHORT(CFXDataQuad::Render);

	// Do we want to handle quad in a special way ?
	if(m_iRenderType)
	{
		switch(m_iRenderType)
		{
			case 1:
				CreateConeScreenQuad_Render(_pRD, _WMat);
				break;
		}

		return;
	}

	// Render quad
	int nVertices = 4;
	int nPrims = 2;

	if(m_Flags & FXFLAG_QUAD_HEIGHT)
	{
		nVertices = 5;
		nPrims = 4;
	}

	uint16 nHistoryEntries = 0;
	uint8* pDataChunk = 0;

	CEffectSystemHistory::CEffectSystemHistoryObject* pObjectHistory = NULL;
	if ((m_Flags & FXFLAG_QUAD_ALLOWHISTORY) && _pRD->m_pHistory)
	{
		pObjectHistory = _pRD->m_pHistory->GetHistory(m_UseHistoryObject);
		pDataChunk = pObjectHistory->GetDataBasePtrNumEntries(nHistoryEntries);
	}

//	const CMTime& AnimTime0 = _pRD->m_AnimTime0;
	fp32 Time0 = _pRD->m_Time0;
//	const CMTime& AnimTime1 = _pRD->m_AnimTime1;
	fp32 Time1 = _pRD->m_Time1;

	const CMat4Dfp32& WMat = _pRD->m_WMat;
	const CMat4Dfp32& VMat = _pRD->m_VMat;

	const fp32* pWidth = m_lWidth.GetBasePtr();
	const fp32* pDuration = m_lDuration.GetBasePtr();
	const fp32* pRotationAngle = m_lRotationAngle.GetBasePtr();
	const int* pSurfaceID = m_lSurfaceID.GetBasePtr();
	const uint32* pColor = m_lColor.GetBasePtr();
	const fp32* pHeight = m_lHeight.GetBasePtr();
	const fp32* pWidth2 = m_lWidth2.GetBasePtr();
	const fp32* pSphereRadius = m_lSphereRadius.GetBasePtr();
	const CVec3Dfp32* pPosNoise = m_lPositionNoise.GetBasePtr();
	const CVec3Dfp32* pPosOffset = m_lPosOffset.GetBasePtr();
	
	// Retrive keys
	GETKEY(LerpTimeAnim, Time0, iCurKey, iNextKey, _pRD->m_Flags);
	
	uint32 Rand = (m_Seed > 0) ? m_Seed : _pRD->m_Anim0 + _pRD->m_Seeding;
	fp32 DurationRand = pDuration[0];
	const fp32 RecipRand = 1.0f / DurationRand;
	const fp32 LerpTimeRand = MinMT( ((DurationRand - (DurationRand - Time0)) * RecipRand), 1.0f);

	// Recreate effect
	const fp32 BaseRotAngle = (m_RandFlags & FXRAND_QUAD_BASEROTATEANGLE) ? (MFloat_GetRand(Rand++) * _PI) : 0.0f;
	const int SurfaceID = (pSurfaceID) ? ((m_RandFlags & FXRAND_QUAD_SURFACE) ? pSurfaceID[_pRD->m_Anim0 % m_lSurfaceID.Len()] : pSurfaceID[iCurKey]) : 0;
	const uint32 Color = CFXSysUtil::LerpColorMix((m_RandFlags & FXRAND_QUAD_COLOR) != 0, pColor, Rand, iCurKey, iNextKey, LerpTimeRand, LerpTimeAnim);
	const fp32 Width = CFXSysUtil::LerpMixMT((m_RandFlags & FXRAND_QUAD_WIDTH) != 0, pWidth, Rand, iCurKey, iNextKey, LerpTimeRand, LerpTimeAnim);
	const fp32 Angle = CFXSysUtil::LerpMixMT((m_RandFlags & FXRAND_QUAD_ROTATEANGLE) != 0, pRotationAngle, Rand, iCurKey, iNextKey, LerpTimeRand, LerpTimeAnim) + BaseRotAngle;
	//const fp32 Angle = (m_RandFlags & FXRAND_QUAD_ROTATEANGLE) ? FXLERP(pRotationAngle[0], pRotationAngle[1], MFloat_GetRand(Rand + 0)) +
	//															(FXLERP(pRotationAngle[0], pRotationAngle[1], MFloat_GetRand(Rand + 1)) * LerpTimeRand) :
	//															FXLERP(pRotationAngle[iCurKey], pRotationAngle[iNextKey], LerpTimeAnim);
	//if(m_RandFlags & FXRAND_QUAD_ROTATEANGLE)
	//	Rand	+= 2;
	const fp32 Height = (m_Flags & FXFLAG_QUAD_HEIGHT) ? CFXSysUtil::LerpMT(pHeight[iCurKey], pHeight[iNextKey], LerpTimeAnim) : 0;
	const fp32 Width2 = (m_Flags & FXFLAG_QUAD_WIDTH2) ? CFXSysUtil::LerpMT(pWidth2[iCurKey], pWidth2[iNextKey], LerpTimeAnim) : Width;
	const fp32 SphereRadius = (pSphereRadius) ? CFXSysUtil::LerpMT(pSphereRadius[iCurKey], pSphereRadius[iNextKey], LerpTimeAnim) : 0.0f;
	
	CVec3Dfp32 PosOffset = 0;
	if(pPosOffset)
		pPosOffset[iCurKey].Lerp(pPosOffset[iNextKey], LerpTimeAnim, PosOffset);
    
	if(m_Flags & FXFLAG_QUAD_ALLOWHISTORY && pObjectHistory)
	{
		bool bSpawn = (Time0 > 0) ? true : false;

		//const uint16 nHistoryEntries = pObjectHistory->GetNumEntries();
		if(nHistoryEntries > 0)
		{
            bSpawn = false;
			
			for(int i = 0; i < nHistoryEntries; i++)
			{
				CEffectSystemHistory::CFXDataQuadHistory& QuadHistory = GETQUADENTRY(pDataChunk);

				if(QuadHistory.m_Time + m_HistoryTime < Time1)
				{
					//liRemove.Add(i);
					pDataChunk = pObjectHistory->RemoveUpdate(i--);
					nHistoryEntries--;
					continue;
				}

				if(m_Flags & FXFLAG_QUAD_BILLBOARD)
				{
					CMat4Dfp32 UMat, Mat;
					UMat.Unit();
					UMat.Multiply(VMat, Mat);

					CVec3Dfp32 QuadPos;
					//uint32 HistoryColor;

					QuadPos = QuadHistory.m_Position * VMat;

					const fp32 HAnimTime = Time1 - QuadHistory.m_Time;
					GETKEY(LerpTimeHAnim, HAnimTime, iCurHKey, iNextHKey, _pRD->m_Flags);
					const uint32 HColor = CFXSysUtil::LerpColor(pColor[iCurHKey], pColor[iNextHKey], LerpTimeHAnim);

					// Noise position
					const CVec3Dfp32& PosNoise = pPosNoise[0];
					CVec3Dfp32 AddNoisePos;
					int Seed = QuadHistory.m_Seed;
					AddNoisePos[0] = PosNoise[0] * ((MFloat_GetRand(Seed++) - 0.5f) * 2.0f);
					AddNoisePos[1] = PosNoise[1] * ((MFloat_GetRand(Seed++) - 0.5f) * 2.0f);
					AddNoisePos[2] = PosNoise[2] * ((MFloat_GetRand(Seed++) - 0.5f) * 2.0f);
					QuadPos = QuadPos + AddNoisePos;

					CMTime TempTime = CMTime::CreateFromSeconds(_pRD->m_Time1 - QuadHistory.m_Time);
					CFXDataObject::Render_Quad(_pRD, SurfaceID, QuadPos, Width, Width2, SphereRadius, Height, Angle, HColor, m_Flags, &UMat, &UMat, &TempTime);
				}
				else
				{
					
					/*
					CMat4Dfp32 L2V;
					CMat4Dfp32 WMat;
					_WMat.Multiply(_VMat, L2V);
					CVec3Dfp32::GetMatrixRow(WMat, 3) = QuadHistory.m_Position;
					WMat.Multiply(_VMat, L2V);

					const fp32 c0 = (sinv + cosv) * Width;
					const fp32 c1 = (sinv - cosv) * Width;

					if(m_Flags & FXFLAG_QUAD_PLANE_XY)
					{
						pV[0] = CVec3Dfp32( c0,  c1, Height) * L2V;
						pV[1] = CVec3Dfp32(-c1,  c0, Height) * L2V;
						pV[2] = CVec3Dfp32(-c0, -c1, Height) * L2V;
						pV[3] = CVec3Dfp32( c1, -c0, Height) * L2V;
					}
					else if(m_Flags & FXFLAG_QUAD_PLANE_XZ)
					{
						pV[0] = CVec3Dfp32( c0, Height,  c1) * L2V;
						pV[1] = CVec3Dfp32(-c1, Height,  c0) * L2V;
						pV[2] = CVec3Dfp32(-c0, Height, -c1) * L2V;
						pV[3] = CVec3Dfp32( c1, Height, -c0) * L2V;
					}
					else if(m_Flags & FXFLAG_QUAD_PLANE_YZ)
					{
						pV[0] = CVec3Dfp32(Height,  c0,  c1) * L2V;
						pV[1] = CVec3Dfp32(Height, -c1,  c0) * L2V;
						pV[2] = CVec3Dfp32(Height, -c0, -c1) * L2V;
						pV[3] = CVec3Dfp32(Height,  c1, -c0) * L2V;
					}

					if(m_Flags & FXFLAG_QUAD_HEIGHT)
						pV[4] = CVec3Dfp32(0, 0, 0) * L2V;
					*/
				}


			}

			const uint16 nEntriesLeft = pObjectHistory->GetNumEntries();
			if(Time0 > 0)
			{
				if(nEntriesLeft > 0)
				{
					CEffectSystemHistory::CFXDataQuadHistory& LastEntry = *pObjectHistory->GetQuadEntry(nEntriesLeft - 1);
					if(Time1 - LastEntry.m_Time > Time0)
					{
						CEffectSystemHistory::CFXDataQuadHistory Entry;
						CVec3Dfp32::GetMatrixRow(WMat, 3).MultiplyMatrix(VMat, Entry.m_Position);
						Entry.m_Position = CVec3Dfp32::GetMatrixRow(WMat, 3);
						Entry.m_Time = Time1;
						Entry.m_Seed = (uint32)((m_Seed + (Time1 * 10000)) * Time1);

						pObjectHistory->AddEntry(&Entry);
					}
				}
				else
					bSpawn = true;
			}
		}

		if(bSpawn)
		{
			CEffectSystemHistory::CFXDataQuadHistory Entry;
			CVec3Dfp32::GetMatrixRow(WMat, 3).MultiplyMatrix(VMat, Entry.m_Position);
			Entry.m_Position = CVec3Dfp32::GetMatrixRow(WMat, 3) + (PosOffset * WMat);
			Entry.m_Seed = (uint32)((m_Seed + (Time1 * 10000)) * Time1);
			Entry.m_Time = Time1;

			pObjectHistory->AddEntry(&Entry);
		}
	}
	else if(Time0 > 0)
	{
		if(m_Flags & FXFLAG_QUAD_BILLBOARD)
		{
			CMat4Dfp32 UMat;
			CVec3Dfp32 QuadPos;
			
			UMat.Unit();
			if(pPosOffset)
				QuadPos = ((PosOffset * WMat) + CVec3Dfp32::GetMatrixRow(WMat, 3)) * VMat;
			else
				QuadPos = WMat.GetRow(3) * VMat;
			CFXDataObject::Render_Quad(_pRD, SurfaceID, QuadPos, Width, Width2, Height, SphereRadius, Angle, Color, m_Flags, &UMat, &UMat);
		}
		else
		{
			// New render code
			if(pPosOffset)
			{
				PosOffset.MultiplyMatrix3x3(WMat);

				CMat4Dfp32 PosMat =WMat;
				PosMat.GetRow(3) += PosOffset;
				CFXDataObject::Render_Quad(_pRD, SurfaceID, CVec3Dfp32(0,0,0), Width, Width2, Height, SphereRadius, Angle, Color, m_Flags, &PosMat);
			}
			else
				CFXDataObject::Render_Quad(_pRD, SurfaceID, CVec3Dfp32(0,0,0), Width, Width2, Height, SphereRadius, Angle, Color, m_Flags);
		}
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataCone
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFXDataCone::CFXDataCone()
	: CFXDataObject()
	, m_Seed(0)
{
	m_lSurfaceID.Clear();
	m_lInnerRadius.Clear();
	m_lOuterRadius.Clear();
	m_lRotationAngle.Clear();
	m_lColor.Clear();
	m_lHeight.Clear();
	m_lSegments.Clear();
	m_lRings.Clear();
}


CFXDataCone::CFXDataCone(const CFXDataCollect& _Data)
	: CFXDataObject(_Data)
	, m_Seed(_Data.m_Seed)
{
	m_lSurfaceID.Add(_Data.m_lSurfaceID);
	m_lInnerRadius.Add(_Data.m_lInnerRadius);
	m_lOuterRadius.Add(_Data.m_lOuterRadius);
	m_lRotationAngle.Add(_Data.m_lRotationAngle);
	m_lColor.Add(_Data.m_lColor);
	m_lHeight.Add(_Data.m_lHeight);
	m_lSegments.Add(_Data.m_lSegments);
	m_lRings.Add(_Data.m_lRings);

	m_Flags = ((CStr)_Data.m_Flags).TranslateFlags(g_lpEFFECTSYSTEM_CONE_FLAGS);
	m_RandFlags = ((CStr)_Data.m_RandomFlags).TranslateFlags(g_lpEFFECTSYSTEM_CONE_RANDFLAGS);
	m_RandFlags &= FXRAND_CONE_BASEROTATEANGLE;
}


void CFXDataCone::CalcBound_Box(const CXR_AnimState* _pAnimState)
{
	if(!(m_IntFlags & FXFLAGS_INT_HASCALCEDBOUNDBOX))
	{
		fp32 Width = M_Fabs((m_lHeight[0] * 0.5f));
		m_BoundBox = CBox3Dfp32(-Width,Width);
	}
}

void CFXDataCone::Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat)
{
	MSCOPESHORT(CFXDataCone::Render);

	CEffectSystemHistory::CEffectSystemHistoryObject* pObjectHistory = NULL;
	if(m_Flags & FXFLAG_CONE_ALLOWHISTORY)
		pObjectHistory = _pRD->m_pHistory->GetHistory(m_UseHistoryObject);

	// Get values for creating effect
//	const fp32* pDuration = m_lDuration.GetBasePtr();
	const fp32* pHeight = m_lHeight.GetBasePtr();
	const fp32* pOuterRadius = m_lOuterRadius.GetBasePtr();
	const fp32* pInnerRadius = m_lInnerRadius.GetBasePtr();
	const fp32* pRotationAngle = m_lRotationAngle.GetBasePtr();
	const int* pSurfaceID = m_lSurfaceID.GetBasePtr();
	const uint32* pColor = m_lColor.GetBasePtr();
	const uint8* pSegments = m_lSegments.GetBasePtr();

	// Retrive keys
	GETKEY(LerpTimeAnim, _pRD->m_Time0, iCurKey, iNextKey, _pRD->m_Flags);
	
	uint32 Rand = ((m_Seed > 0) ? m_Seed : _pRD->m_Anim0 + _pRD->m_Seeding);
//	fp32 DurationRand = pDuration[0];
//	const fp32 RecipRand = 1.0f / DurationRand;
//	const fp32 LerpTimeRand = MinMT( ((DurationRand - (DurationRand - _pRD->m_Time0)) * RecipRand), 1.0f);

	// Recreate effect
	const fp32 BaseRotAngle = (m_RandFlags & FXRAND_CONE_BASEROTATEANGLE) ? (MFloat_GetRand(Rand++) * _PI) : 0.0f;
	const uint8 Segments = (uint8)CFXSysUtil::LerpInt(pSegments[iCurKey], pSegments[iNextKey], LerpTimeAnim);
	const fp32 OuterRadius = CFXSysUtil::LerpMT(pOuterRadius[iCurKey], pOuterRadius[iNextKey], LerpTimeAnim);
	const fp32 InnerRadius = CFXSysUtil::LerpMT(pInnerRadius[iCurKey], pInnerRadius[iNextKey], LerpTimeAnim);
	const fp32 Height = CFXSysUtil::LerpMT(pHeight[iCurKey], pHeight[iNextKey], LerpTimeAnim);
	const fp32 Angle = CFXSysUtil::LerpMT(pRotationAngle[iCurKey], pRotationAngle[iNextKey], LerpTimeAnim) + BaseRotAngle;
	const uint32 Color = CFXSysUtil::LerpColor(pColor[iCurKey], pColor[iNextKey], LerpTimeAnim);
	const int SurfaceID = (pSurfaceID) ? ((m_RandFlags & FXRAND_CONE_SURFACE) ? pSurfaceID[_pRD->m_Anim0 % m_lSurfaceID.Len()] : pSurfaceID[iCurKey]) : 0;
	
	// Calculate amount of space needed
	uint8 Rings = 2;
	uint16 nVertices = 0;
	uint16 nPrims = 0;
	if(m_Flags & FXFLAG_CONE_SPHEROID)
	{
		// Rings
		Rings = Segments;//LinearInterpolateMT(pRings[CurrKey], pRings[NextKey], LerpTimeAnim);

		nVertices = Segments * Rings;
		nPrims = (Segments * 2) * (Rings - 1);
	}
	else
	{
		// One of the radiuses is zero, only create necesary vertices
		if(InnerRadius == 0.0f || OuterRadius == 0.0f)
		{
			nVertices = Segments + 1;
			nPrims = Segments;
		}
		else
		{
			nVertices = Segments * 2;
			nPrims = nVertices;
		}
	}

	CXR_VertexBuffer* pVB = _pRD->m_pFXSystem->AllocVB(_pRD->m_pFXSystem->m_pRenderParams);
	CVec3Dfp32* pV = _pRD->m_pVBM->Alloc_V3(nVertices);
	CVec2Dfp32* pTV = _pRD->m_pVBM->Alloc_V2(nVertices);
	uint16* pP = _pRD->m_pVBM->Alloc_Int16(nPrims*3);

	if (!pVB || !pV || !pTV || !pP)
		return;

	fp32 sinv, cosv;
	QSinCos(Angle, sinv, cosv);

	CMat4Dfp32 L2V;
	_pRD->m_WMat.Multiply(_pRD->m_VMat, L2V);
	const fp32 SegmentAngle = 1.0f / Segments;
	
	if(!(m_Flags & FXFLAG_CONE_ALLOWHISTORY) && _pRD->m_Time0 > 0)
	{
		if(m_Flags & FXFLAG_CONE_SPHEROID)
		{
			const fp32 UVScale = 1.0f / (Rings-1);
			//const fp32 RadiusStep = (1.0f / (Rings-1)) * 4.0f;
			const fp32 HeightStep = (Height / (Rings-1));

			if(FXFLAG_CONE_PLANE_YZ)
			{
				for(int i = 0; i < Rings; i++)
				{
					//QSinCosUnit(RingAngle * (i), sinv, cosv);
					//fp32 Radius_ = RadiusStep * i * MinMT(1.0f, sinv * 2.0f);//((1.0f / (Rings-1)) * i * 4.0f) * MinMT(1.0f, sinv * 2);//(((i+1) * (4.0f / (Rings-1))) * sinv);
					fp32 SpheroidHeight = HeightStep * i;//(i) * (8.0f / (Rings-1));
					//Radius_ = ((1.0f / (Rings-1)) * 4.0f) * sinv * Height_;
					
					//QSinCosUnit( ((0.5f) / (16)) * i, sinv, cosv);
					QSinCosUnit((0.5f / (Rings*2)) * i, sinv, cosv);
					fp32 SpheroidRadius = 1.0f * (sinv*OuterRadius);
					//QSinCosUnit( ((0.5f) / (16)) * i, sinv, cosv);
					//Radius_ *= (sinv*4.0f);

					int iMulRings = i * Rings;
					for(int j = 0; j < Segments; j++)
					{
						QSinCosUnit(SegmentAngle * j, sinv, cosv);

						const fp32 c0 = (sinv + cosv) * SpheroidRadius;
						const fp32 c1 = (sinv - cosv) * SpheroidRadius;

						pV[iMulRings+j] = CVec3Dfp32(SpheroidHeight, c0, c1) * L2V;
						//if(i == 0)
						//	pTV[(i*Rings)+j] = CVec2Dfp32(0.5f, 0.5f);
						//else
						QSinCosUnit((SegmentAngle * j) + Angle, sinv, cosv);
						pTV[iMulRings+j] = CVec2Dfp32(0.5f, 0.5f) + (CVec2Dfp32(sinv * 0.5f, cosv * 0.5f) * (UVScale * i));
					}
				}
			}

			int iIndex = 0;
			for(int i = 0; i < Rings - 1; i++)
			{
				for(int j = 0; j < Segments - 1; j++)
				{
					pP[iIndex++] = (i * Segments) + j;
					pP[iIndex++] = (i * Segments) + Segments + j + 1;
					pP[iIndex++] = (i * Segments) + Segments + j;

					pP[iIndex++] = (i * Segments) + j;
					pP[iIndex++] = (i * Segments) + j + 1;
					pP[iIndex++] = (i * Segments) + Segments + j + 1;
				}

				pP[iIndex++] = (i * Segments) + Segments - 1;
				pP[iIndex++] = (i * Segments) + Segments;
				pP[iIndex++] = ((i+1) * Segments) + Segments - 1;

				pP[iIndex++] = (i * Segments) + Segments - 1;
				pP[iIndex++] = (i * Segments);
				pP[iIndex++] = (i * Segments) + Segments;
			}
		}
		else
		{
			// Check if we need to generate points for both inner and outer segments
			fp32 OuterMulInner = OuterRadius * InnerRadius;
			if (!AlmostEqual(OuterMulInner, 0.0f, 0.0001f)) // OuterRadius != 0.0f && InnerRadius != 0.0f)
			{
				uint i0 = (m_Flags & (FXFLAG_CONE_PLANE_XY | FXFLAG_CONE_PLANE_XZ)) ? 0 : 1;
				uint i1 = (m_Flags & FXFLAG_CONE_PLANE_XY) ? 1 : 2;
				uint iRH = 3 - (i0 + i1);

				CVec3Dfp32 Vo = Height;
				CVec3Dfp32 Vi = 0.0f;

				// Create vertex points
				for(int i = 0; i < Segments; i++)
				{
					QSinCosUnit(SegmentAngle * i, sinv, cosv);

					Vo.k[i0] = (sinv + cosv) * OuterRadius;
					Vo.k[i1] = (sinv - cosv) * OuterRadius;
					Vi.k[i0] = (sinv + cosv) * InnerRadius;
					Vi.k[i1] = (sinv - cosv) * InnerRadius;

					pV[i] = Vo * L2V;
					pV[Segments + i] = Vi * L2V;
				}

				// Generate UV's
				const fp32 UStep = (1.0f / (Segments-1)) * 2.0f;
				CVec2Dfp32 UVCoord0(0.0f, 0.0f);
				CVec2Dfp32 UVCoord1(0.0f, 1.0f);
				for(int i = 0; i < Segments; i++)
				{
					pTV[i] = UVCoord0;
					pTV[Segments + i] = UVCoord1;
					UVCoord0.k[0] = UVCoord1.k[0] = UVCoord0.k[0] + UStep;
				}

				// Create index buffer
				int iIndex = 0;
				for(int i = 0; i < Segments-1; i++)
				{
					pP[iIndex++] = i;
					pP[iIndex++] = i + Segments;
					pP[iIndex++] = i + Segments + 1;

					pP[iIndex++] = i;
					pP[iIndex++] = i + Segments + 1;
					pP[iIndex++] = i + 1;
				}

				pP[iIndex++] = Segments - 1;
				pP[iIndex++] = nPrims - 1;
				pP[iIndex++] = Segments;

				pP[iIndex++] = Segments - 1;
				pP[iIndex++] = Segments;
				pP[iIndex++] = 0;
			}
			else
			{
				// One is zero, grab it
				bool bInnerZero = AlmostEqual(InnerRadius, 0.0f, 0.0001f);
				const fp32 Radius = (bInnerZero) ? OuterRadius : InnerRadius;
				const fp32 RadiusHeight = (bInnerZero) ? Height : 0.0f;
				const fp32 BaseHeight = (bInnerZero) ? 0.0f : Height;

				uint ic0 = (m_Flags & (FXFLAG_CONE_PLANE_XY | FXFLAG_CONE_PLANE_XZ)) ? 0 : 1;
				uint ic1 = (m_Flags & FXFLAG_CONE_PLANE_XY) ? 1 : 2;
				uint iRH = 3 - (ic0 + ic1);

				CVec3Dfp32 V = 0.0f;
				V.k[iRH] = BaseHeight;

				pV[Segments] = V * L2V;
				pTV[Segments] = CVec2Dfp32(0.5f, 0.5f);
				V.k[iRH] = RadiusHeight;
				for(int i = 0; i < Segments; i++)
				{
					QSinCosUnit(SegmentAngle * i, sinv, cosv);

					V.k[ic0] = (sinv + cosv) * Radius;
					V.k[ic1] = (sinv - cosv) * Radius;
					pV[i] = V * L2V;
					
					pTV[i] = CVec2Dfp32(1-(((sinv + cosv) * 0.5f) + 0.5f), (((sinv - cosv) * 0.5f) + 0.5f));
				}

				int iIndex = 0;
				for(int i = 0; i < Segments-1; i++)
				{
					pP[iIndex++] = i;
					pP[iIndex++] = Segments;
					pP[iIndex++] = i+1;
				}

				pP[iIndex++] = Segments - 1;
				pP[iIndex++] = Segments;
				pP[iIndex++] = 0;
			}
		}

		//int Fade = 255 - RoundToInt(255 * Time / m_lDuration[0]);
		pVB->Geometry_Color(CPixel32(Color));
		if (!pVB->AllocVBChain(_pRD->m_pVBM, false))
			return;
		pVB->Geometry_VertexArray(pV, nVertices, true);
		//pVB->Geometry_VertexArray(pV, Segments*Rings, true);
		pVB->Geometry_TVertexArray(pTV, 0);
		pVB->Render_IndexedTriangles(pP, nPrims);

		if(m_pShader)
		{
			CSurfaceKey SurfaceKey;
			if (_pRD->CreateFragmentProgram(pVB, m_pShader, SurfaceKey))
				_pRD->m_pVBM->AddVB(pVB);
		}

		if(SurfaceID)
			_pRD->m_pFXSystem->Render_Surface(_pRD->m_pFXSystem->m_pRenderParams, SurfaceID, pVB, _pRD->m_AnimTime0);
	}
	else if(m_Flags & FXFLAG_CONE_ALLOWHISTORY)
	{
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataParticlesOpt
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFXDataParticlesOpt::CFXDataParticlesOpt()
	: m_spParticleSystem(NULL)
{
}

CFXDataParticlesOpt::CFXDataParticlesOpt(const CFXDataCollect& _Data)
	: CFXDataObject(_Data)
{
	m_spParticleSystem = MNew(ParticlesOptClassName);
	m_spParticleSystem->Create(_Data.m_ParticleKeys);
}


//void CFXDataParticlesOpt::Render(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, uint32 _Flags)
void CFXDataParticlesOpt::Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat)
{
	MSCOPESHORT(CFXDataParticlesOpt::Render);
	//m_ParticleSystem->Render(_pAnimState, _WMat, _VMat);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataFXParticleHook
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFXDataFXParticleHook::CFXDataFXParticleHook()
{
	SetThreadSafe(false);
}


void CFXDataFXParticleHook::CreateLayers(const CFXDataCollect& _Data)
{
	m_Layer = _Data.m_Layer;
}


void CFXDataFXParticleHook::RenderPerRenderVariables(const CXR_AnimState* _pAnimState, PerRenderVariables* M_RESTRICT _pPR)
{
	ParticlesOptClassName::RenderPerRenderVariables(_pAnimState, _pPR);

	if(_pAnimState->m_Data[FXANIM_DATA_FLAGS] & FXANIM_FLAGS_USESKELETON)
	{
		_pPR->m_pSkeletonInst = _pAnimState->m_pSkeletonInst;
		_pPR->m_SkeletonType = (uint8)_pAnimState->m_Data[FXANIM_DATA_SKELETONTYPE];
		_pPR->m_pSkeleton = (CXR_Skeleton*)_pAnimState->m_Data[FXANIM_DATA_SKELETON];
	}
	if(_pAnimState->m_Data[FXANIM_DATA_FLAGS] & FXANIM_FLAGS_USEBOXES)
	{
		_pPR->m_nSpawnBoxes = _pAnimState->m_Data[FXANIM_DATA_NUMBOXES];
		_pPR->m_pSpawnBoxes = (CBox3Dfp32*)_pAnimState->m_Data[FXANIM_DATA_BOXES];
	}
}


bool CFXDataFXParticleHook::GenerateParticles(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, CXR_Model_Custom_RenderParams* _pRenderParams)
{
	return ParticlesOptClassName::GenerateParticles(_pPR, _pBatch, _pRenderParams);
}


bool CFXDataFXParticleHook::Generate_Render(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, CXR_Model_Custom_RenderParams* _pRenderParams, int _GenerateFlags)
{
	m_Layer.GenerateRender(_pPR, _pBatch, _pRenderParams, _GenerateFlags);

	// Return result of this generated render batch
	return ParticlesOptClassName::Generate_Render(_pPR, _pBatch, _pRenderParams, _GenerateFlags);
}


TPtr<CXR_ModelInstance> CFXDataFXParticleHook::CreateModelInstance()
{
	return ParticlesOptClassName::CreateModelInstance();
}


void CFXDataFXParticleHook::Generate_Distribution_Point(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int& _GenerateFlags)
{
	ParticlesOptClassName::Generate_Distribution_Point(_pPR, _pBatch, _GenerateFlags);

	m_Layer.GenerateDistribution(GetMaxNumParticles(), _pPR, _pBatch, _GenerateFlags);
}


void CFXDataFXParticleHook::Generate_Distribution(PerRenderVariables* M_RESTRICT _pPR, CBatch* M_RESTRICT _pBatch, int& _GenerateFlags)
{
	ParticlesOptClassName::Generate_Distribution(_pPR, _pBatch, _GenerateFlags);

	m_Layer.GenerateDistribution(GetMaxNumParticles(), _pPR, _pBatch, _GenerateFlags);
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataFXParticle
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFXDataFXParticle::CFXDataFXParticle()
	: CFXDataObject()
{
}


CFXDataFXParticle::CFXDataFXParticle(const CFXDataCollect& _Data)
	: CFXDataObject(_Data)
{
	m_Flags = ((CStr)_Data.m_Flags).TranslateFlags(g_lpEFFECTSYSTEM_FXPARTICLE_FLAGS);
	m_spParticleSystem = MNew(CFXDataFXParticleHook);
	m_spParticleSystem->Create(_Data.m_ParticleKeys);

	m_spParticleSystem->CreateLayers(_Data);
}


void CFXDataFXParticle::Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat)
{
	MSCOPESHORT(CFXDataFXParticle::Render);

	// Do any hooked struff with old particle system
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CFXDataBeamStrip
|__________________________________________________________________________________________________
\*************************************************************************************************/
CFXDataBeamStrip::CFXDataBeamStrip()
	: CFXDataObject()
	, m_Seed(0)
{
	m_lWidth.Clear();
	m_lColor.Clear();
	m_lSurfaceID.Clear();
	m_lSegments.Clear();
	m_lPosNoise.Clear();
	m_lVelocity.Clear();
	m_lPosOffset.Clear();
	m_lWidthNoise.Clear();
	m_lStripHeight.Clear();
	m_lVelocityNoise.Clear();
	m_lTextureScroll.Clear();
	m_lDirection.Clear();
//	m_lTwirls.Clear();
//	m_lNoises.Clear();
//	m_lPaths.Clear();
//	m_lpExtLayers.Clear();
}


CFXDataBeamStrip::CFXDataBeamStrip(const CFXDataCollect& _Data)
	: CFXDataObject(_Data)
	, m_Seed(_Data.m_Seed)
{
	m_lColor.Add(_Data.m_lColor);
	m_lWidth.Add(_Data.m_lWidth);
	m_lSegments.Add(_Data.m_lSegments);
	m_lPosNoise.Add(_Data.m_lPosNoise);
	m_lVelocity.Add(_Data.m_lVelocity);
	m_lSurfaceID.Add(_Data.m_lSurfaceID);
	m_lPosOffset.Add(_Data.m_lPosOffset);
	m_lWidthNoise.Add(_Data.m_lWidthNoise);
	m_lStripHeight.Add(_Data.m_lStripHeight);
	m_lVelocityNoise.Add(_Data.m_lVelocityNoise);
	m_lTextureScroll.Add(_Data.m_lTextureScroll);
	m_lDirection.Add(_Data.m_lDirection);
	
	//m_lpExtLayers.Add(_Data.m_lpExtLayers);
//	m_lTwirls.Add(_Data.m_lTwirlLayer);
//	m_lNoises.Add(_Data.m_lNoiseLayer);
//	m_lPaths.Add(_Data.m_lPathLayer);
	m_Layer = _Data.m_Layer;

	m_Flags = ((CStr)_Data.m_Flags).TranslateFlags(g_lpEFFECTSYSTEM_BEAMSTRIP_FLAGS);
	m_RandFlags = ((CStr)_Data.m_RandomFlags).TranslateFlags(g_lpEFFECTSYSTEM_BEAMSTRIP_RANDFLAGS);
}


void CFXDataBeamStrip::CalcBound_Box(const CXR_AnimState* _pAnimState)
{
	fp32 Radius = MaxMT(M_Fabs(m_lStripHeight[0]), M_Fabs(m_lWidth[0]));
	m_BoundBox = CBox3Dfp32(-Radius,Radius);
	
	if((m_Flags & FXFLAG_BEAMSTRIP_ALLOWHISTORY) && _pAnimState && _pAnimState->m_pModelInstance)
	{
		// Object has history and needs to be taken into consideration
		
		CEffectSystemHistory* pEffectHistory = safe_cast<CEffectSystemHistory>(_pAnimState->m_pModelInstance);
		const CEffectSystemHistory::CEffectSystemHistoryObject* pObjectHistory = pEffectHistory->GetHistory(m_UseHistoryObject);
		
		uint16 nHistoryEntries = 0;
		const uint8* pDataChunk = pObjectHistory->GetDataBasePtrNumEntries(nHistoryEntries);//pObjectHistory->GetNumEntries();
		
		CBox3Dfp32 HistoryBox(-Radius,Radius);
		if(nHistoryEntries > 0 && pDataChunk)
		{
			CVec3Dfp32 Position = pObjectHistory->m_CurrentPos;
			CVec3Dfp32 Expand;

			for(uint16 i = 0; i < nHistoryEntries; i++)
			{
				CEffectSystemHistory::CFXDataBeamStripHistory& Entry = GETBEAMSTRIPENTRY(pDataChunk);
				Expand = Entry.m_Position - Position;
				HistoryBox.Expand(Expand);
			}
		}

		m_BoundBox = HistoryBox;
	}
}


//void CFXDataBeamStrip::Render(CXR_Model_EffectSystem* _pFXSystem, CXR_VBManager* _pVBM, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, uint32 _Flags)
void CFXDataBeamStrip::Render(CEffectSystemRenderData* _pRD, const CMat4Dfp32& _WMat)
{
	MSCOPESHORT(CFXDataBeamStrip::Render);
	
	uint16 nHistoryEntries = 0;
	CEffectSystemHistory::CEffectSystemHistoryObject* pObjectHistory = NULL;
	if (_pRD->m_pHistory && (m_Flags & FXFLAG_BEAMSTRIP_ALLOWHISTORY))
	{
		pObjectHistory = _pRD->m_pHistory->GetHistory(m_UseHistoryObject);
		
		nHistoryEntries = pObjectHistory->GetNumEntries();//((pObjectHistory) ? pObjectHistory->GetNumEntries() : 0);
		if((nHistoryEntries < 1) && !(_pRD->m_Time0 > 0))
			return;

		if((m_TotalDuration < _pRD->m_Time0) && (nHistoryEntries < 1))
			return;
	}

	const CMat4Dfp32& WMat = _pRD->m_WMat;
//	const CMat4Dfp32& VMat = _pRD->m_VMat;

	const fp32* pWidth = m_lWidth.GetBasePtr();
	const fp32* pDuration = m_lDuration.GetBasePtr();
	const int* pSurfaceID = m_lSurfaceID.GetBasePtr();
	const uint32* pColor = m_lColor.GetBasePtr();
	const fp32* pStripHeight = m_lStripHeight.GetBasePtr();
	const fp32* pWidthNoise = m_lWidthNoise.GetBasePtr();
	const uint8* pSegments = m_lSegments.GetBasePtr();
	const CVec3Dfp32* pPosNoise = m_lPosNoise.GetBasePtr();
	const CVec3Dfp32* pPosOffset = m_lPosOffset.GetBasePtr();
	const CVec3Dfp32* pVelocity = m_lVelocity.GetBasePtr();
	const CVec3Dfp32* pVelocityNoise = m_lVelocityNoise.GetBasePtr();
	const CVec2Dfp32* pTextureScroll = m_lTextureScroll.GetBasePtr();
	const CVec3Dfp32* pDirection = m_lDirection.GetBasePtr();
	const CVec3Dfp32& WMatPos = CVec3Dfp32::GetMatrixRow(WMat, 3);
		
	// Retrive keys
	GETKEY(LerpTimeAnim, _pRD->m_Time0, iCurKey, iNextKey, _pRD->m_Flags);
	uint32 Rand = ((m_Seed > 0) ? m_Seed : _pRD->m_Anim0 + _pRD->m_Seeding);
	fp32 DurationRand = pDuration[0];
	const fp32 RecipRand = 1.0f / DurationRand;
	const fp32 LerpTimeRand = MinMT( ((DurationRand - (DurationRand - _pRD->m_Time0)) * RecipRand), 1.0f);

	// Recreate effect
	CVec3Dfp32 PosNoise;
	CVec3Dfp32 PosOffset;
	CVec3Dfp32 Velocity;
	CVec3Dfp32 VelocityNoise;
	CVec2Dfp32 TextureScroll;
	CVec3Dfp32 Direction;

	// Surface
	const int SurfaceID = (pSurfaceID) ? ((m_RandFlags & FXRAND_BEAMSTRIP_SURFACE) ? pSurfaceID[_pRD->m_Anim0 % m_lSurfaceID.Len()] : pSurfaceID[iCurKey]) : 0;
	const fp32 Width = CFXSysUtil::LerpMixMT((m_RandFlags & FXRAND_BEAMSTRIP_WIDTH) != 0, pWidth, Rand, iCurKey, iNextKey, LerpTimeRand, LerpTimeAnim);
	const fp32 Height = CFXSysUtil::LerpMixMT((m_RandFlags & FXRAND_BEAMSTRIP_STRIPHEIGHT) != 0, pStripHeight, Rand, iNextKey, iCurKey, LerpTimeRand, LerpTimeAnim);
	const uint8 Segments = (uint8)CFXSysUtil::LerpIntMix((m_RandFlags & FXRAND_BEAMSTRIP_SEGMENTS) != 0, pSegments, Rand, iCurKey, iNextKey, LerpTimeRand, LerpTimeAnim);
	const uint32 Color = CFXSysUtil::LerpColorMix((m_RandFlags & FXRAND_BEAMSTRIP_COLOR) != 0, pColor, Rand, iCurKey, iNextKey, LerpTimeRand, LerpTimeAnim);
	const fp32 WidthNoise = CFXSysUtil::LerpMixMT((m_RandFlags & FXRAND_BEAMSTRIP_WIDTHNOISE) != 0, pWidthNoise, Rand, iCurKey, iNextKey, LerpTimeRand, LerpTimeAnim);
	//(m_RandFlags & FXRAND_BEAMSTRIP_POSNOISE) ? /* Random PosNoise Vector Here */ : CVec3Dfp32::Lerp(pPosNoise[iCurKey], pPosNoise[iNextKey], PosNoise, LerpAnimTime);
	pPosNoise[iCurKey].Lerp(pPosNoise[iNextKey], LerpTimeAnim, PosNoise);
	pPosOffset[iCurKey].Lerp( pPosOffset[iNextKey], LerpTimeAnim, PosOffset);
	pVelocity[iCurKey].Lerp(pVelocity[iNextKey], LerpTimeAnim, Velocity);
	pVelocityNoise[iCurKey].Lerp(pVelocityNoise[iNextKey], LerpTimeAnim, VelocityNoise);
	pTextureScroll[iCurKey].Lerp(pTextureScroll[iNextKey], LerpTimeAnim, TextureScroll);
	if(pDirection)
		pDirection[iCurKey].Lerp(pDirection[iNextKey], LerpTimeAnim, Direction);

	// Rendering
	TArray<CXR_BeamStrip>	BeamList;
	CXR_BeamStrip			Beam;
	CXR_BeamStrip			DummyBeam;
	//CMat4Dfp32				WMat;
	
	BeamList.SetGrow(Segments+1);
	
	DummyBeam.m_Width		= 0;
	DummyBeam.m_Color		= 0;
	DummyBeam.m_TextureYOfs	= 0;

	CMat4Dfp32 UnitMat;
	UnitMat.Unit();

	// First we render the defined segments as we want them
	if(Segments > 0)
	{
		//CVec3Dfp32 PosOffsetSeg = PosOffset / Segments;//WMatDir * PosOffset;
		const CVec3Dfp32& WMatUp = CVec3Dfp32::GetMatrixRow(WMat, 0);
		const CVec3Dfp32& WMatLeft = CVec3Dfp32::GetMatrixRow(WMat, 1);
		const CVec3Dfp32& WMatDir = CVec3Dfp32::GetMatrixRow(WMat, 2);
		const fp32 TexScroll = (TextureScroll.k[1] * _pRD->m_Time0);

		BeamList.SetLen(0);
		BeamList.SetGrow(Segments+1);

		Beam.m_Width		= Width;
		Beam.m_Pos			= WMatPos + (WMatLeft * PosOffset[0]) + (WMatUp * PosOffset[1]) + (WMatDir * PosOffset[2]);
		Beam.m_Color		= Color;//0xffffffff;	// Need color information from script!!
		Beam.m_TextureYOfs	= TexScroll; //(_pRD->m_Time0.k[1] * _pRD->m_Time1);// + 0;
		BeamList.Add(Beam);

		fp32 sinv, cosv;
		const fp32 vcoord = 1.0f / static_cast<fp32>(Segments);//(1.0f - (1.0f / static_cast<fp32>(Segments)));

		CVec3Dfp32 HeightDir = CVec3Dfp32(0,0,Height);	// Default to yz plane
		
		if(m_Flags & FXFLAG_BEAMSTRIP_PLANE_XY)
			HeightDir = WMatLeft * Height;//CVec3Dfp32(Height,0,0);
		else if(m_Flags & FXFLAG_BEAMSTRIP_PLANE_XZ)
			HeightDir = WMatDir * Height;//CVec3Dfp32(0,Height,0);
		else if(m_Flags & FXFLAG_BEAMSTRIP_PLANE_YZ)
			HeightDir = WMatUp * Height;//CVec3Dfp32(0,0,Height);
		else if(pDirection)
		{
			Direction.MultiplyMatrix3x3(WMat);
			HeightDir = Direction / Segments;
		}
		
		fp32 RandSeed = (_pRD->m_Time1 - _pRD->m_Time0);//MRTC_RAND();

		for(uint8 i = 1; i <= Segments; i++)
		{
			uint16 iRand = (uint16)((m_RandFlags & FXRAND_BEAMSTRIP_SEED) ? (RandSeed + (i * (_pRD->m_Time1-_pRD->m_Time0))) : (i + (i * 1000)));
			
			
			QSinCos(MFloat_GetRand(iRand++) * ((m_RandFlags & FXRAND_BEAMSTRIP_SEED) ? _pRD->m_Time1 : _pRD->m_Time0), sinv, cosv);
			CVec3Dfp32 Noise;
			Noise[0] = PosNoise[0] * (MFloat_GetRand(iRand++) * sinv);
			Noise[1] = PosNoise[1] * (MFloat_GetRand(iRand++) * cosv);
			Noise[2] = PosNoise[2] * (MFloat_GetRand(iRand++) * ((sinv + cosv) * 0.5f));
			Beam.m_Width		= Width + (sinv * (MFloat_GetRand(iRand++) * WidthNoise));
			Beam.m_Pos			= WMatPos + Noise;
			Beam.m_Pos			= Beam.m_Pos + (HeightDir * i);
			Beam.m_Pos			= Beam.m_Pos + (WMatLeft * PosOffset[0]) + (WMatUp * PosOffset[1]) + (WMatDir * PosOffset[2]);
			Beam.m_Color		= Color;//0xffffffff;
			Beam.m_TextureYOfs	= TexScroll + ((vcoord * i) * 4.0f);//-(vcoord * i);
			BeamList.Add(Beam);
		}

		CXR_BeamStrip* pBeam = BeamList.GetBasePtr();
		const uint16& nBeams = BeamList.Len();
		
		m_Layer.EvaluateBeamStrip(pBeam, nBeams, _pRD->m_Time0, WMat);
		// Render layers
		/*
		{
			// Apply path layer
			{
				const int nPaths = m_lPaths.Len();
				const CFXLayerPath* pPaths = m_lPaths.GetBasePtr();

				for(int i = 0; i < nPaths; i++)
				{
					const CFXLayerPath& Path = pPaths[i];
					
					const int nPathNodes = Path.m_lPath.Len();
					const CVec3Dfp32* pPathNodes = Path.m_lPath.GetBasePtr();
					if(nPathNodes == nBeams)
					{
						for(int i = 0; i < nBeams; i++)
						{
							pBeam[i].m_Pos = pPathNodes[i];
							pBeam[i].m_Pos.MultiplyMatrix3x3(WMat);
							pBeam[i].m_Pos += WMatPos;
							//pBeam[i].m_Pos = WMatPos + pPathNodes[i];
							//pBeam[i].m_Pos.MultiplyMatrix3x3(WMat);
						}
					}
					else
					{
						// Not supported
					}
				}
			}

			// Apply twirl layer
			{
				const int nTwirls = m_lTwirls.Len();
				const CFXLayerTwirl* pTwirls = m_lTwirls.GetBasePtr();

				for(int i = 0; i < nTwirls; i++)
				{
					const CFXLayerTwirl& Twirl = pTwirls[i];
					fp32 TwirlRadius = Twirl.m_Radius;
					fp32 TwirlOffset = Twirl.m_Offset;
					fp32 TwirlSpeed = Twirl.m_Speed;
					fp32 TwirlLoops = Twirl.m_Loops;
					const uint8 Comp0 = (Twirl.m_iComponents >> 2);
					const uint8 Comp1 = (Twirl.m_iComponents & 0x3);

					fp32 sinv, cosv;
					CVec3Dfp32 Move(0);
					//fp32* pTime = 

					const fp32 TwirlLoopSegment = TwirlLoops / (fp32)nBeams;
					const fp32 SpeedOffset = TwirlSpeed * _pRD->m_Time0;
					const fp32 Scale = 1.0f / (fp32)nBeams;
					for(int j = 0; j < nBeams; j++)
					{
						//QSinCosUnit(TwirlOffset + ((fp32)j * TwirlSpeed), sinv, cosv);
						QSinCosUnit(TwirlOffset + SpeedOffset + ((fp32)j * TwirlLoopSegment), sinv, cosv);
						const fp32 SinAddCosRadius = (sinv + cosv) * TwirlRadius;
						const fp32 SinSubCosRadius = (sinv - cosv) * TwirlRadius;

						// twirl beam position
						fp32& Pos0 = pBeam[j].m_Pos.k[Comp0];
						fp32& Pos1 = pBeam[j].m_Pos.k[Comp1];
						Pos0 = Pos0 + (SinAddCosRadius) * (1.0f - Scale * (fp32)j);// * Height);
						Pos1 = Pos1 + (SinSubCosRadius) * (1.0f - Scale * (fp32)j);// * Height);
					}
				}
			}

			// Apply noise layer
			{
				const int nNoises = m_lNoises.Len();
				const CFXLayerNoise* pNoises = m_lNoises.GetBasePtr();

				for(int i = 0; i < nNoises; i++)
				{
					uint16 iRand = (uint16)((m_RandFlags & FXRAND_BEAMSTRIP_SEED) ? (RandSeed + ((i+1) * (_pRD->m_Time1-_pRD->m_Time0))) : (i + ((i+1) * 1000)));

					const CFXLayerNoise& Noise = pNoises[i];

					CVec3Dfp32 NoisePos = 0;
					fp32 NoiseX = Noise.m_Noise.k[0];
					fp32 NoiseY = Noise.m_Noise.k[1];
					fp32 NoiseZ = Noise.m_Noise.k[2];

					fp32 NoiseAnim = 1;
					if(Noise.m_bAnimated)
						NoiseAnim = QSin(_pRD->m_Time0 * Noise.m_Timescale);

					for(int j = 0; j < nBeams; j++)
					{
						NoisePos.k[0] = (-NoiseX + (NoiseX * MFloat_GetRand(iRand++) * 2.0f)) * NoiseAnim;
						NoisePos.k[1] = (-NoiseY + (NoiseY * MFloat_GetRand(iRand++) * 2.0f)) * NoiseAnim;
						NoisePos.k[2] = (-NoiseZ + (NoiseZ * MFloat_GetRand(iRand++) * 2.0f)) * NoiseAnim;

						pBeam[j].m_Pos += NoisePos;
					}
				}
			}
		}
		*/

		// New Render code
		CFXDataObject::Render_BeamStrip(_pRD, SurfaceID, pBeam, nBeams, CXR_BEAMFLAGS_TEXFROMOFFSET, &UnitMat);
	}

	// Render object history
	if(pObjectHistory)
	{
		bool bSpawn = (_pRD->m_Time0 > 0) ? true : false;
		//uint8 SpawnCut = 0;
	
		BeamList.SetLen(0);
		BeamList.SetGrow(nHistoryEntries+1);

		pObjectHistory->m_CurrentPos = WMatPos;

		if(nHistoryEntries > 0)
		{
			// Entries exist, so we try to render them
            const fp32 TimeExpire = (_pRD->m_Time1 - (m_HistoryTime + m_HistorySpawn));
			bool bDummyBeamAdded = false;
			
			uint8* pDataChunk = pObjectHistory->GetDataBasePtr();
			if(!pDataChunk) return;

			bSpawn = false;
			
			fp32 sinv, cosv;
			for(uint16 i = 0; i < nHistoryEntries; i++)
			{
				CEffectSystemHistory::CFXDataBeamStripHistory& Entry = GETBEAMSTRIPENTRY(pDataChunk);

				if(Entry.m_Time < TimeExpire)
				{
					pDataChunk = pObjectHistory->RemoveUpdate(i--);
					nHistoryEntries--;
					continue;
				}

				uint16 Seed = Entry.m_Seed;
				const fp32 HistoryTime = _pRD->m_Time1 - Entry.m_Time;
				const fp32 TimeScale = MinMT(HistoryTime, 1.0f);

				QSinCos(MFloat_GetRand(Seed++) * _pRD->m_Time1, sinv, cosv);

				Beam.m_Width		= (Width * TimeScale) + ((sinv * MFloat_GetRand(Seed++) * WidthNoise) * TimeScale);
				Beam.m_Pos			= Entry.m_Position + (Velocity * HistoryTime);
				Beam.m_Color		= Color;
				Beam.m_Color.A()	= 255 - TruncToInt(MinMT((HistoryTime / m_HistoryTime) * 255.0f, 255.0f));
				Beam.m_TextureYOfs	= (Entry.m_Time * 0.35f) + (TextureScroll[0] * HistoryTime);

				// If a dummy been added before this one, we need to add one at new position before putting in new beam strip
				if(bDummyBeamAdded)
				{
					bDummyBeamAdded = false;
					DummyBeam.m_TextureYOfs = Beam.m_TextureYOfs;
					DummyBeam.m_Pos = Beam.m_Pos;
					BeamList.Add(DummyBeam);
				}
				BeamList.Add(Beam);

				// Do we have a cut in the strip here? If so, put a dummy beam here
				if(Entry.m_Cut)
				{
					// Add dummy beam
					DummyBeam.m_TextureYOfs = Beam.m_TextureYOfs;
					DummyBeam.m_Pos = Beam.m_Pos;
					BeamList.Add(DummyBeam);
					bDummyBeamAdded = true;
				}
			}

			// History has been recreated, now add dummy if we need one at our current position
			if(bDummyBeamAdded)
			{
				DummyBeam.m_Pos = WMatPos;
				BeamList.Add(DummyBeam);
			}

			const uint16 nEntriesLeft = pObjectHistory->GetNumEntries();
			pDataChunk = pObjectHistory->GetDataBasePtr();
			
			// Is effect active
			if(_pRD->m_Time0 > 0 && ((_pRD->m_Time0 < m_HistoryTime && !(_pRD->m_Flags & FXFLAGS_TIMEMODE_CONTINUOUS)) || (_pRD->m_Flags & FXFLAGS_TIMEMODE_CONTINUOUS)))
			{
				Beam.m_Width = 0;
				Beam.m_Pos = WMatPos;
				Beam.m_Color = CPixel32(255,255,255,0);
				Beam.m_TextureYOfs = _pRD->m_Time1 * 0.35f;
				BeamList.Add(Beam);

				// Do we have any history entries after update, and is it time to save the state?
				if(nEntriesLeft > 0)
				{
					CEffectSystemHistory::CFXDataBeamStripHistory& LastEntry = *pObjectHistory->GetBeamStripEntry(nEntriesLeft-1);
					if(LastEntry.m_Time < _pRD->m_Time1 - m_HistorySpawn)
					{
						CEffectSystemHistory::CFXDataBeamStripHistory Entry;

						Entry.m_Position = WMatPos;
						Entry.m_Time = _pRD->m_Time1;
						Entry.m_Seed = (uint32)(nEntriesLeft + (MFloat_GetRand((int)(_pRD->m_Time1*1000)) * 1000));
						Entry.m_Cut = 0;

						pObjectHistory->AddEntry(&Entry);
					}
				}
			}

			// System has come to a stop
			else
			{
				if(nEntriesLeft > 0)
				{
					CEffectSystemHistory::CFXDataBeamStripHistory& LastEntry = *pObjectHistory->GetBeamStripEntry(nEntriesLeft-1);
					if(!LastEntry.m_Cut)
					{
						CEffectSystemHistory::CFXDataBeamStripHistory Entry;

						Entry.m_Position = WMatPos;
						Entry.m_Time = _pRD->m_Time1;
						Entry.m_Seed = (uint32)(nEntriesLeft + (MFloat_GetRand((int)(_pRD->m_Time1*1000)) * 1000));
						Entry.m_Cut = true;

						pObjectHistory->AddEntry(&Entry);

						// Add it to the beam list
						Beam.m_Width = 0;
						Beam.m_Pos = WMatPos;
						Beam.m_Color = CPixel32(255,255,255,0);
						Beam.m_TextureYOfs = _pRD->m_Time1 * 0.35f;
						BeamList.Add(Beam);
					}
				}
			}
		}
		
		if(bSpawn)
		{
			// Add a segment if effect is active
			CEffectSystemHistory::CFXDataBeamStripHistory Entry;

			Entry.m_Position = WMatPos;
			Entry.m_Time = _pRD->m_Time1;
			Entry.m_Seed = nHistoryEntries + (nHistoryEntries * 1000);
			Entry.m_Cut = 0;

			pObjectHistory->AddEntry(&Entry);
		}
	
		const uint16& nBeams = BeamList.Len();
		if(nBeams > 0)
		{
			CXR_BeamStrip* pBeam = BeamList.GetBasePtr();
			CFXDataObject::Render_BeamStrip(_pRD, SurfaceID, pBeam, nBeams, CXR_BEAMFLAGS_TEXFROMOFFSET, &UnitMat);
		}
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CXR_Model_EffectSystem
|__________________________________________________________________________________________________
\*************************************************************************************************/
CXR_Model_EffectSystem::CXR_Model_EffectSystem()
{
	m_RenderingObject = 0;
	m_nObjects = 0;
	m_Flags = 0;
	m_pRD = NULL;
	m_pRegistry = NULL;
	m_pRenderParams = NULL;
}

void CXR_Model_EffectSystem::Init()
{
	if(!m_bStaticsInitialized)
	{
		int iV = 0;
		int iT = 0;
		m_lQuadCrossTriangles[0 + iT]  = 0 + iV;
		m_lQuadCrossTriangles[1 + iT]  = 1 + iV;
		m_lQuadCrossTriangles[2 + iT]  = 4 + iV;
		m_lQuadCrossTriangles[3 + iT]  = 1 + iV;
		m_lQuadCrossTriangles[4 + iT]  = 2 + iV;
		m_lQuadCrossTriangles[5 + iT]  = 4 + iV;
		m_lQuadCrossTriangles[6 + iT]  = 2 + iV;
		m_lQuadCrossTriangles[7 + iT]  = 3 + iV;
		m_lQuadCrossTriangles[8 + iT]  = 4 + iV;
		m_lQuadCrossTriangles[9 + iT]  = 3 + iV;
		m_lQuadCrossTriangles[10 + iT] = 0 + iV;
		m_lQuadCrossTriangles[11 + iT] = 4 + iV;

		m_lQuadCrossTVertices[0 + iV][0] = 0;
		m_lQuadCrossTVertices[0 + iV][1] = 0;
		m_lQuadCrossTVertices[1 + iV][0] = 1;
		m_lQuadCrossTVertices[1 + iV][1] = 0;
		m_lQuadCrossTVertices[2 + iV][0] = 1;
		m_lQuadCrossTVertices[2 + iV][1] = 1;
		m_lQuadCrossTVertices[3 + iV][0] = 0;
		m_lQuadCrossTVertices[3 + iV][1] = 1;
		m_lQuadCrossTVertices[4 + iV][0] = 0.5f;
		m_lQuadCrossTVertices[4 + iV][1] = 0.5f;

		m_lBoxTriangles[ 1 + iT] = m_lBoxTriangles[18 + iT] = m_lBoxTriangles[30 + iT] = 1;
		m_lBoxTriangles[11 + iT] = m_lBoxTriangles[17 + iT] = m_lBoxTriangles[29 + iT] = 7;
		m_lBoxTriangles[ 0 + iT] = m_lBoxTriangles[ 3 + iT] = m_lBoxTriangles[19 + iT] = m_lBoxTriangles[21 + iT] = m_lBoxTriangles[24 + iT] = 0;
		m_lBoxTriangles[ 2 + iT] = m_lBoxTriangles[ 4 + iT] = m_lBoxTriangles[12 + iT] = m_lBoxTriangles[31 + iT] = m_lBoxTriangles[33 + iT] = 2;
		m_lBoxTriangles[ 5 + iT] = m_lBoxTriangles[13 + iT] = m_lBoxTriangles[15 + iT] = m_lBoxTriangles[26 + iT] = m_lBoxTriangles[27 + iT] = 3;
		m_lBoxTriangles[ 6 + iT] = m_lBoxTriangles[ 9 + iT] = m_lBoxTriangles[23 + iT] = m_lBoxTriangles[25 + iT] = m_lBoxTriangles[28 + iT] = 4;
		m_lBoxTriangles[ 7 + iT] = m_lBoxTriangles[20 + iT] = m_lBoxTriangles[22 + iT] = m_lBoxTriangles[32 + iT] = m_lBoxTriangles[35 + iT] = 5;
		m_lBoxTriangles[ 8 + iT] = m_lBoxTriangles[10 + iT] = m_lBoxTriangles[14 + iT] = m_lBoxTriangles[16 + iT] = m_lBoxTriangles[34 + iT] = 6;

		m_bStaticsInitialized = true;
	}
}


void CXR_Model_EffectSystem::CreateEffect(CRegistry *_pRegistry)
{
	// Make sure we have an initialized static members
	Init();

	m_pRD = &m_RD;

	m_lObjectsType.SetLen(0);
	m_lObjectsType.SetGrow(1);
	m_liObjects.SetLen(0);
	m_liObjects.SetGrow(1);
	m_lBeams.SetLen(0);
	m_lBeams.SetGrow(1);
	m_lQuads.SetLen(0);
	m_lQuads.SetGrow(1);
	m_lParticlesOpt.SetLen(0);
	m_lParticlesOpt.SetGrow(0);
	m_lLights.SetLen(0);
	m_lLights.SetGrow(1);
	m_lBeamStrips.SetLen(0);
	m_lBeamStrips.SetGrow(1);
	m_lFXParticles.SetLen(0);
	m_lFXParticles.SetGrow(1);
	m_lShaders.SetLen(0);
	m_lShaders.SetGrow(1);
	m_lRenderTargets.SetLen(0);
	m_lRenderTargets.SetGrow(1);
	m_nObjects = 0;
	m_Duration = 0;

	m_Flags = 0;
	//m_Flags |= FXFLAGS_ISALIVE;

	int iChildren = _pRegistry->GetNumChildren();
	for(int i = 0; i < iChildren; i++)
	{
		CStr ThisName = _pRegistry->GetName(i).UpperCase();
		CStr ThisKey = _pRegistry->GetValue(i);

		// Create collector and fetch register child
		CFXDataCollect DataCollector;
		const spCRegistry spEntity = _pRegistry->GetChild(i);
		const int iEntityChildrens = spEntity->GetNumChildren();
		
		// Get number of keys used before evaluating
		RetriveNumKeys(spEntity, DataCollector);
		for(int j = 0; j < iEntityChildrens; j++)
			EvalRegisterObject(spEntity->GetChild(j), DataCollector);

		if(ThisName == "BEAM")
		{
			m_lBeams.Add(CFXDataBeam(DataCollector));
			m_liObjects.Add((m_lBeams.Len() - 1));
			m_lObjectsType.Add(FXDATA_TYPE_BEAM);
			++m_nObjects;

			if(m_lBeams[m_lBeams.Len()-1].m_Flags & FXFLAG_BEAM_ALLOWHISTORY)
				m_Flags |= FXFLAGS_HASHISTORY;
		}
		else if(ThisName == "WALLMARK")
		{
			m_lWallmarks.Add(CFXDataWallmark(DataCollector));
			m_liObjects.Add((m_lQuads.Len() - 1));
			m_lObjectsType.Add(FXDATA_TYPE_WALLMARK);
			++m_nObjects;

			m_Flags |= FXFLAGS_RENDERCLIENT;
		}
		else if(ThisName == "QUAD")
		{
			m_lQuads.Add(CFXDataQuad(DataCollector));
			m_liObjects.Add((m_lQuads.Len() - 1));
			m_lObjectsType.Add(FXDATA_TYPE_QUAD);
			++m_nObjects;

			if(m_lQuads[m_lQuads.Len() - 1].m_Flags & FXFLAG_QUAD_ALLOWHISTORY)
				m_Flags |= FXFLAGS_HASHISTORY;
		}
		else if(ThisName == "CONE")
		{
			m_lCones.Add(CFXDataCone(DataCollector));
			m_liObjects.Add((m_lCones.Len() - 1));
			m_lObjectsType.Add(FXDATA_TYPE_CONE);
			++m_nObjects;
		}
		else if(ThisName == "BEAMSTRIP")
		{
			m_lBeamStrips.Add(CFXDataBeamStrip(DataCollector));
			m_liObjects.Add((m_lBeamStrips.Len() - 1));
			m_lObjectsType.Add(FXDATA_TYPE_BEAMSTRIP);
			++m_nObjects;

			// Check if history should be used or not
			if(m_lBeamStrips[m_lBeamStrips.Len() - 1].m_Flags & FXFLAG_BEAMSTRIP_ALLOWHISTORY)
				m_Flags |= FXFLAGS_HASHISTORY;
		}
		else if(ThisName == "FXPARTICLE")
		{
			m_lFXParticles.Add(CFXDataFXParticle(DataCollector));
			m_liObjects.Add((m_lFXParticles.Len() - 1));
			m_lObjectsType.Add(FXDATA_TYPE_FXPARTICLE);
			++m_nObjects;

			if(m_lFXParticles[m_lFXParticles.Len() - 1].m_Flags & FXFLAG_FXPARTICLE_ALLOWHISTORY)
				m_Flags |= FXFLAGS_HASHISTORY;

			if(m_lFXParticles[m_lFXParticles.Len() - 1].m_Flags & FXFLAG_FXPARTICLE_WORLDCOLLIDE)
				m_Flags |= FXFLAGS_HASCOLLISION;
		}
		else if(ThisName == "LIGHT")
		{
			m_lLights.Add(CFXDataLight(DataCollector));
			m_liObjects.Add(m_lLights.Len() - 1);
			m_lObjectsType.Add(FXDATA_TYPE_LIGHT);
			++m_nObjects;

			// Flag system and tell it it needs onrendervis flags in object client flags
			m_Flags |= FXFLAGS_RENDERVIS;
		}
		else if(ThisName == "PARTICLESOPT")
		{
			m_lParticlesOpt.Add(CFXDataParticlesOpt(DataCollector));
			m_liObjects.Add((m_lParticlesOpt.Len() - 1));
			m_lObjectsType.Add(FXDATA_TYPE_PARTICLESOPT);
			++m_nObjects;
		}
		else if(ThisName == "FLAGS")
			m_Flags |= ThisKey.TranslateFlags(g_lpEFFECTSYSTEM_FLAGS);
		else if(ThisName == "DURATION")
			m_Duration = ThisKey.Val_fp64();
	}

	// Link shaders
	LinkShaders();

	#ifdef M_RTM
		OptimizeRenderTargets();
	#endif
}

void CXR_Model_EffectSystem::OptimizeRenderTargets()
{
	const int nRenderTargets = m_lRenderTargets.Len();
	CFXDataRenderTarget* pRenderTarget = m_lRenderTargets.GetBasePtr();
	for(int i = 0; i < nRenderTargets; i++)
		pRenderTarget[i].m_RenderTarget = "";
}

void CXR_Model_EffectSystem::Create(const char* _pParam)
{
	CXR_Model_Custom::Create(_pParam);

	// Fetch register file and register-class object
	CStr Params = _pParam;
	if(Params != "")
	{
		if (!m_pRegistry)
		{
			ConOutL("§cf00WARNING (CXR_Model_EffectSystem): Effect system registry performance warning!");

			// Ohoh!! This shouldn't be run anymore!!
			MACRO_GetSystem;
			CStr ContentPath = pSys->GetEnvironment()->GetValue("CONTENTPATH");
			if (ContentPath == "")
				ContentPath = pSys->m_ExePath + "Content\\";

			CStr RegisterFile = ContentPath + "Registry\\";
			RegisterFile += Params.GetStrSep(",");
			RegisterFile += ".xrg";

			CStr RegisterObject = Params.GetStrSep(",");

			// Read register
			static TPtr<CRegistryCompiled> spRegCompiled;
			spCRegistry spReg;

			if (spRegCompiled)
			{
				spReg = spRegCompiled->GetRoot();
			}
			else
			{
				CStr FileName = RegisterFile.GetPath() + RegisterFile.GetFilenameNoExt() + ".xcr";
				if (CDiskUtil::FileExists(FileName))
				{
					spRegCompiled = MNew(CRegistryCompiled);
					if (!spRegCompiled)
						MemError("Create");
					{
	//					D_NOXDF;
						spRegCompiled->Read_XCR(FileName);
					}
					spReg = spRegCompiled->GetRoot();
				}
				else if (CDiskUtil::FileExists(RegisterFile))
				{
					spReg = REGISTRY_CREATE;
					if(!spReg)
						MemError("CXR_Model_EffectSystem::Create REGISTRY_CREATE");
					spReg->XRG_Read(RegisterFile);
				}
				else
				{
					ConOutL(CStrF("§cf00ERROR (CXR_Model_EffectSystem::Create): file '%s' not found!", RegisterFile.Str()));
					M_TRACEALWAYS("ERROR (CXR_Model_EffectSystem::Create): file '%s' not found!\n", RegisterFile.Str());
					return;
				}
			}

			spCRegistry spRegSystem = spReg->Find(RegisterObject);
			spCRegistry spSystem = spReg->Find(RegisterObject);
			if (!spSystem)
				return;

			// Create effect
			#ifndef M_RTM
				m_EffectSystemName = spSystem->GetThisName();
			#endif
			CreateEffect(spSystem);
		}
		else
		{
			// Find effect system in registry
			Params.GetStrSep(",");
			CRegistry* pSystem = m_pRegistry->Find(Params);
			if (!pSystem)
				return;

			// Create effect system
			#ifndef M_RTM
				m_EffectSystemName = pSystem->GetThisName();
			#endif
			CreateEffect(pSystem);
		}
	}
}


static void PrecacheSurfaces(TAP<const int> _pList, const CXR_Engine& _Engine)
{
	MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
	if (!pSC) Error_static("PreacheSurfaces", "No surface-context available.");

	for (uint iiSurf = 0; iiSurf < _pList.Len(); iiSurf++)
	{
		int iSurf = _pList[iiSurf];
		CXW_Surface* pSurf = (iSurf > 0) ? pSC->GetSurface(iSurf) : NULL;
		if (!pSurf) continue;
		pSurf = pSurf->GetSurface(_Engine.m_SurfOptions, _Engine.m_SurfCaps);

		pSurf->InitTextures(false);	// Don't report failures.
		if (!(pSurf->m_Flags & XW_SURFFLAGS_INVISIBLE))
			pSurf->SetTextureParam(CTC_TEXTUREPARAM_FLAGS, CTC_TXTIDFLAGS_PRECACHE);
	}
}

template<class T>
static M_INLINE void PrecacheAllSurfaces(const TArray<T>& _List, const CXR_Engine& _Engine)
{
	TAP<const T> pList = _List;
	for (uint i = 0; i < pList.Len(); i++)
		PrecacheSurfaces(pList[i].m_lSurfaceID, _Engine);
}

void CXR_Model_EffectSystem::OnPrecache(CXR_Engine* _pEngine, int _iVariation)
{
	PrecacheAllSurfaces(m_lBeams, *_pEngine);
	PrecacheAllSurfaces(m_lQuads, *_pEngine);
	PrecacheAllSurfaces(m_lBeamStrips, *_pEngine);
	PrecacheAllSurfaces(m_lCones, *_pEngine);
	PrecacheAllSurfaces(m_lWallmarks, *_pEngine);

	TAP<const CFXDataShader> pShaders = m_lShaders;
	for (uint i = 0; i < pShaders.Len(); i++)
		PrecacheSurfaces(pShaders[i].m_lShaderMaps, *_pEngine);

	TAP<CFXDataParticlesOpt> pParticleSystems = m_lParticlesOpt;
	for (uint i = 0; i < pParticleSystems.Len(); i++)
		if (pParticleSystems[i].m_spParticleSystem)
			pParticleSystems[i].m_spParticleSystem->OnPrecache(_pEngine, _iVariation);
}


void CXR_Model_EffectSystem::LinkShaders()
{
	CFXDataShader* pShaders = m_lShaders.GetBasePtr();
	CFXDataBeam* pBeams = m_lBeams.GetBasePtr();
	int nObjs = m_lBeams.Len();
	for(int i = 0; i < nObjs; i++)
	{
		if(pBeams[i].m_pShader)
			pBeams[i].m_pShader = &pShaders[pBeams[i].m_iShader];
	}

	CFXDataQuad* pQuads = m_lQuads.GetBasePtr();
	nObjs = m_lQuads.Len();
	for(int i = 0; i < nObjs; i++)
	{
		if(pQuads[i].m_pShader)
			pQuads[i].m_pShader = &pShaders[pQuads[i].m_iShader];
	}

	CFXDataParticlesOpt* pParticlesOpt = m_lParticlesOpt.GetBasePtr();
	nObjs = m_lParticlesOpt.Len();
	for(int i = 0; i < nObjs; i++)
	{
		if(pParticlesOpt[i].m_pShader)
			pParticlesOpt[i].m_pShader = &pShaders[pParticlesOpt[i].m_iShader];
	}

	CFXDataBeamStrip* pBeamStrips = m_lBeamStrips.GetBasePtr();
	nObjs = m_lBeamStrips.Len();
	for(int i = 0; i < nObjs; i++)
	{
		if(pBeamStrips[i].m_pShader)
			pBeamStrips[i].m_pShader = &pShaders[pBeamStrips[i].m_iShader];
	}

	CFXDataFXParticle* pFXParticles = m_lFXParticles.GetBasePtr();
	nObjs = m_lFXParticles.Len();
	for(int i = 0; i < nObjs; i++)
	{
		if(pFXParticles[i].m_pShader)
			pFXParticles[i].m_pShader = &pShaders[pFXParticles[i].m_iShader];
	}

	CFXDataCone* pCones = m_lCones.GetBasePtr();
	nObjs = m_lCones.Len();
	for(int i = 0; i < nObjs; i++)
	{
		if(pCones[i].m_pShader)
			pCones[i].m_pShader = &pShaders[pCones[i].m_iShader];
	}

//	int nShaders = m_lShaders.Len();
//	int nMaxParams = 0;
//	for(int i = 0; i < nShaders; i++)
//		m_nMaxShaderParams = MaxMT(nMaxParams, pShaders[i].m_nPredefinedParams;

	//CFXDataLight*	m_lLights
	//CFXDataWallmark* m_lWallmarks
}

TPtr<CXR_ModelInstance> CXR_Model_EffectSystem::CreateModelInstance()
{
	CFXDataBeamStrip* pBeamStripList = m_lBeamStrips.GetBasePtr();
	CFXDataFXParticle* pFXParticleList = m_lFXParticles.GetBasePtr();
	CFXDataBeam* pBeamList = m_lBeams.GetBasePtr();
	CFXDataCone* pConeList = m_lCones.GetBasePtr();
	CFXDataQuad* pQuadList = m_lQuads.GetBasePtr();
	CFXDataParticlesOpt* pParticlesOptList = m_lParticlesOpt.GetBasePtr();
	
	uint nBeamStrips = m_lBeamStrips.Len();
	uint nFXParticles = m_lFXParticles.Len();
	uint nBeams = m_lBeams.Len();
	uint nCones = m_lCones.Len();
	uint nQuads = m_lQuads.Len();
	uint nParticlesOpt = m_lParticlesOpt.Len();

	TArray<CEffectSystemHistory::CCreationParam> lParams;
	CEffectSystemHistory::CCreationParam Param;

	uint16 nObjects = 0;

	// Search through all object allowing history, assign them a history object
	// and add parameters to creation parameters used in model instance
	Param.m_Param0 = FXSIZE_TYPE_BEAMSTRIP;
	for(uint8 i = 0; i < nBeamStrips; i++)
	{
		CFXDataBeamStrip& BeamStrip = pBeamStripList[i];
		if(BeamStrip.m_Flags & FXFLAG_BEAMSTRIP_ALLOWHISTORY)
		{
			BeamStrip.m_UseHistoryObject = nObjects++;
			lParams.Add(Param);
		}
	}

	Param.m_Param0 = FXSIZE_TYPE_BEAM;
	for(uint8 i = 0; i < nBeams; i++)
	{
		CFXDataBeam& Beam = pBeamList[i];
		if(Beam.m_Flags & FXFLAG_BEAM_ALLOWHISTORY)
		{
			Beam.m_UseHistoryObject = nObjects++;
			lParams.Add(Param);
		}
	}

	Param.m_Param0 = FXSIZE_TYPE_CONE;
	for(uint8 i = 0; i < nCones; i++)
	{
		CFXDataCone& Cone = pConeList[i];
		if(Cone.m_Flags & FXFLAG_CONE_ALLOWHISTORY)
		{
			Cone.m_UseHistoryObject = nObjects++;
			lParams.Add(Param);
		}
	}

	Param.m_Param0 = FXSIZE_TYPE_QUAD;
	for(uint8 i = 0; i < nQuads; i++)
	{
		CFXDataQuad& Quad = pQuadList[i];
		if(Quad.m_Flags & FXFLAG_QUAD_ALLOWHISTORY)
		{
			Quad.m_UseHistoryObject = nObjects++;
			lParams.Add(Param);
		}
	}



	CEffectSystemHistory* pHistory = NULL;
	{
		CEffectSystemHistory::CCreationParam* pParams = lParams.GetBasePtr();
		//return MNew3(CEffectSystemHistory, nObjects, nParticles + nFXParticles, pParams);
		pHistory = MNew3(CEffectSystemHistory, nObjects, nParticlesOpt + nFXParticles, pParams);
	}

	// Store model instances
	if(pHistory)
	{
		nObjects = 0;
		for(uint8 i = 0; i < nFXParticles; i++)
		{
			pFXParticleList[i].m_UseHistoryObject = nObjects;
			pHistory->m_lspModelInstances[nObjects++] = pFXParticleList[i].m_spParticleSystem->CreateModelInstance();
		}

		for(uint8 i = 0; i < nParticlesOpt; i++)
		{
			pParticlesOptList[i].m_UseHistoryObject = nObjects;
			pHistory->m_lspModelInstances[nObjects++] = pParticlesOptList[i].m_spParticleSystem->CreateModelInstance();
		}
	}

	return pHistory;
	//return NULL;
}


CFXDataObject* CXR_Model_EffectSystem::GetDataObject(uint _iObject, const uint8* _pObjectsType, const uint8* _piObjects)
{
//	uint ObjectIndex = _piObjects[_iObject];
//	uint DataType = _pObjectsType[_iObject];
	return NULL;
}


fp32 CXR_Model_EffectSystem::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	CXR_Model_Custom::GetBound_Sphere(_pAnimState);

	uint nObjects = m_nObjects;
	const uint8* pObjectsType = m_lObjectsType.GetBasePtr();

	CFXDataBeam* pBeamList = m_lBeams.GetBasePtr();
	CFXDataQuad* pQuadList = m_lQuads.GetBasePtr();
//	CFXDataParticlesOpt* pParticleList = m_lParticlesOpt.GetBasePtr();
	CFXDataBeamStrip* pBeamStripList = m_lBeamStrips.GetBasePtr();
	CFXDataFXParticle* pFXParticleList = m_lFXParticles.GetBasePtr();
	CFXDataCone* pConeList = m_lCones.GetBasePtr();
	const uint8* piObjects = m_liObjects.GetBasePtr();

	fp32 Radius = 0.0f;
	for(uint8 i = 0; i < nObjects; i++)
	{
		uint ObjectIndex = piObjects[i];
		uint DataType = pObjectsType[i];
		switch(DataType)
		{
			case FXDATA_TYPE_SHADER:
			{
			}
			break;

			case FXDATA_TYPE_BEAM:
			{
				CFXDataBeam* pBeam = &pBeamList[ObjectIndex];
				Radius = MaxMT(Radius, pBeam->CalcBound_Sphere(_pAnimState));
			}
			break;

			case FXDATA_TYPE_QUAD:
			{
				CFXDataQuad* pQuad = &pQuadList[ObjectIndex];
				Radius = MaxMT(Radius, pQuad->CalcBound_Sphere(_pAnimState));
			}
			break;

			case FXDATA_TYPE_CONE:
			{
				CFXDataCone* pCone = &pConeList[ObjectIndex];
				Radius = MaxMT(Radius, pCone->CalcBound_Sphere(_pAnimState));
			}
			break;

			case FXDATA_TYPE_BEAMSTRIP:
			{
				CFXDataBeamStrip* pBeamStrip = &pBeamStripList[ObjectIndex];
				Radius = MaxMT(Radius, pBeamStrip->CalcBound_Sphere(_pAnimState));
			}
			break;

			case FXDATA_TYPE_FXPARTICLE:
			{
				CFXDataFXParticle* pFXParticle = &pFXParticleList[ObjectIndex];
				Radius = MaxMT(Radius, pFXParticle->CalcBound_Sphere(_pAnimState));
			}
			break;

			default:
				break;
		}
	}

	return Radius;
}


void CXR_Model_EffectSystem::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	CXR_Model_Custom::GetBound_Box(_Box, _pAnimState);

	const uint8 &nObjects = m_nObjects;
	const uint8* pObjectsType = m_lObjectsType.GetBasePtr();
	
	CFXDataBeam* pBeamList = m_lBeams.GetBasePtr();
	CFXDataQuad* pQuadList = m_lQuads.GetBasePtr();
	CFXDataParticlesOpt* pParticleList = m_lParticlesOpt.GetBasePtr();
	CFXDataBeamStrip* pBeamStripList = m_lBeamStrips.GetBasePtr();
	CFXDataFXParticle* pFXParticleList = m_lFXParticles.GetBasePtr();
	CFXDataCone* pConeList = m_lCones.GetBasePtr();
	const uint8* piObjects = m_liObjects.GetBasePtr();

	for(uint8 i = 0; i < nObjects; i++)
	{
		uint ObjectIndex = piObjects[i];
		uint DataType = pObjectsType[i];
		switch(DataType)
		{
			case FXDATA_TYPE_SHADER:
				{
					//CFXDataShader* pShader = &pShaderList[ObjectIndex];
				}
				break;
			case FXDATA_TYPE_BEAM:
				{
					CFXDataBeam* pBeam = &pBeamList[ObjectIndex];
					pBeam->CalcBound_Box(_pAnimState);
					_Box.Expand(pBeam->m_BoundBox);
				}
				break;
			case FXDATA_TYPE_QUAD:
				{
					CFXDataQuad* pQuad = &pQuadList[ObjectIndex];
					pQuad->CalcBound_Box(_pAnimState);
					_Box.Expand(pQuad->m_BoundBox);
				}
				break;
			case FXDATA_TYPE_CONE:
				{
					CFXDataCone* pCone = &pConeList[ObjectIndex];
					pCone->CalcBound_Box(_pAnimState);
					_Box.Expand(pCone->m_BoundBox);
				}
				break;
			case FXDATA_TYPE_BEAMSTRIP:
				{
					CFXDataBeamStrip* pBeamStrip = &pBeamStripList[ObjectIndex];
					pBeamStrip->CalcBound_Box(_pAnimState);
					_Box.Expand(pBeamStrip->m_BoundBox);
				}
				break;
			case FXDATA_TYPE_FXPARTICLE:
				{
					CFXDataFXParticle* pFXParticle = &pFXParticleList[ObjectIndex];
					pFXParticle->CalcBound_Box(_pAnimState);
					_Box.Expand(pFXParticle->m_BoundBox);
				}
				break;
			case FXDATA_TYPE_PARTICLESOPT:
				{
					CFXDataParticlesOpt* pParticlesOpt = &pParticleList[ObjectIndex];
					ParticlesOptClassName* pParticleSystem = pParticlesOpt->m_spParticleSystem;

					if(_pAnimState)
					{
						CXR_AnimState* AnimState = (CXR_AnimState*)_pAnimState;
						CXR_ModelInstance* pModelInstance = AnimState->m_pModelInstance;
						AnimState->m_pModelInstance = NULL;
						
						// No model instance for particle systems
						pParticleSystem->GetBound_Box(pParticlesOpt->m_BoundBox, _pAnimState);
						
						AnimState->m_pModelInstance = pModelInstance;
					}
					else
						pParticleSystem->GetBound_Box(pParticlesOpt->m_BoundBox, _pAnimState);

					_Box.Expand(pParticlesOpt->m_BoundBox);
				}
				break;
			default:
				break;
		}
	}
}

void CXR_Model_EffectSystem::PreRender(CXR_Engine* _pEngine, CXR_ViewClipInterface* _pViewClip, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MSCOPESHORT(CXR_Model_EffectSystem::PreRender);

	const CMTime& AnimTime0 = _pAnimState->m_AnimTime0;
	fp32 Time0 = AnimTime0.GetTime();
	const bool& IsReset = AnimTime0.IsReset();

	if(((IsReset || Time0 < 0) && !(m_Flags & FXFLAGS_HASHISTORY)) )// || (m_Duration > 0 && m_Flags & FXFLAGS_TIMEMODE_CONTROLLED && _pAnimState->m_AnimTime0.GetTime() > m_Duration))
		return;

	// Time to bail? because of self destruction ?
	if(m_Duration > 0 && m_Flags & FXFLAGS_TIMEMODE_CONTROLLED && _pAnimState->m_AnimTime0.GetTime() > m_Duration)
		return;

	if(!IsReset)
	{
		// Check if we need to resolve screen
		/* No point checking for resolve, this will be automaticaly per system if they request the framebuffer.
		TAP<CFXDataShader> lShaders = m_lShaders;
		uint nShaders = (_pAnimState && !(_pAnimState->m_Data[FXANIM_DATA_FLAGS] & FXANIM_FLAGS_RESOLVE)) ? lShaders.Len() : 0;
		for(uint iShader = 0; iShader < nShaders; iShader++)
		{
			TAP<int> lShaderMaps = lShaders[iShader].m_lShaderMaps;
			for(uint iShaderMap = 0; iShaderMap < lShaderMaps.Len(); iShaderMap++)
			{
				if(lShaderMaps[iShaderMap] == FXSHADER_MAP_FRAMEBUFFER)
				{
					_pEngine->GetVC()->m_bNeedResolve_TempFixMe = 1;
					iShader = nShaders;
					break;
				}
			}
		}
		*/

		CXR_AnimState* pAnimState = (CXR_AnimState*)_pAnimState;
		CXR_ModelInstance* pModelInstance = pAnimState->m_pModelInstance;
		CEffectSystemHistory* pHistory = (CEffectSystemHistory*)pModelInstance;

		TAP<CFXDataParticlesOpt> lParticlesOpt = m_lParticlesOpt;
		for(int i = 0; i < lParticlesOpt.Len(); i++)
		{
			CFXDataParticlesOpt* pParticlesOpt = &lParticlesOpt[i];
			ParticlesOptClassName* pParticleSystem = pParticlesOpt->m_spParticleSystem;
			
			CBox3Dfp32 BoxWorld;
			((CFXDataParticlesOpt*)pParticlesOpt)->m_BoundBox.Transform(_WMat, BoxWorld);
			//_pRenderParams->m_RenderInfo.m_pCurrentEngine = _pRenderParams->m_pEngine;

			//const bool bResult = _pRenderParams->m_pEngine->View_GetClip_Box(BoxWorld.m_Min, BoxWorld.m_Max, 0, 0, NULL, &_pRenderParams->m_RenderInfo);
			const bool bResult = (_Flags & CXR_MODEL_ONRENDERFLAGS_NOCULL) ? true : _pViewClip->View_GetClip_Box(BoxWorld.m_Min, BoxWorld.m_Max, 0, 0, NULL, NULL);
			if(bResult)
			{
				// No model instance for particle systems
				//CXR_AnimState* pAnimState = (CXR_AnimState*)_pAnimState;
				//CXR_ModelInstance* pModelInstance = pAnimState->m_pModelInstance;
				pAnimState->m_pModelInstance = (pHistory) ? pHistory->GetModelInstance(pParticlesOpt->m_UseHistoryObject) : NULL;
				
				//_pRenderParams->m_pEngine->Render_AddModel(pParticleSystem, _WMat, *pAnimState);
				_pEngine->Render_AddModel(pParticleSystem, _WMat, *pAnimState, XR_MODEL_STANDARD, _Flags);
				
				//pAnimState->m_pModelInstance = pModelInstance;
			}
		}

		// Render fx particles
		TAP<CFXDataFXParticle> lFXParticles = m_lFXParticles;
		for(int i = 0; i < lFXParticles.Len(); i++)
		{
			CFXDataFXParticle* pFXParticle = &lFXParticles[i];
			CFXDataFXParticleHook* pParticleHook = pFXParticle->m_spParticleSystem;
			CBox3Dfp32 BoxWorld;
			
			pFXParticle->m_BoundBox.Grow(16);
			pFXParticle->m_BoundBox.Transform(_WMat, BoxWorld);
			pFXParticle->m_BoundBox.Grow(-16);

			//const bool bResult = _pRenderParams->m_pEngine->View_GetClip_Box(BoxWorld.m_Min, BoxWorld.m_Max, 0, 0, NULL, &_pRenderParams->m_RenderInfo);
			const bool bResult = (_Flags & CXR_MODEL_ONRENDERFLAGS_NOCULL) ? true : _pViewClip->View_GetClip_Box(BoxWorld.m_Min, BoxWorld.m_Max, 0, 0, NULL, NULL);
			if(bResult)
			{
				// No model instance for particle systems
				//CXR_AnimState* pAnimState = (CXR_AnimState*)_pAnimState;
				//CXR_ModelInstance* pModelInstance = pAnimState->m_pModelInstance;
				pAnimState->m_pModelInstance = (pHistory) ? pHistory->GetModelInstance(pFXParticle->m_UseHistoryObject) : NULL;

				// This is not OK
				pParticleHook->m_SystemFlags &= ~(PARTICLE_FLAGS_USESKELETON | PARTICLE_FLAGS_USEBOXES);

				/*
				if(0 && _pAnimState)
				{
					// Skeleton or boxes
					if(_pAnimState->m_Data[FXANIM_DATA_DATA1] != -1 && _pAnimState->m_Data[FXANIM_DATA_DATA2] != -1)
					{
						if(_pAnimState->m_pSkeletonInst)
							pParticleHook->m_SystemFlags |= PARTICLE_FLAGS_USESKELETON;
						else if(_pAnimState->m_Data[0] != -1 && _pAnimState->m_Data[1] != -1)
							pParticleHook->m_SystemFlags |= PARTICLE_FLAGS_USEBOXES;
					}
				}
				*/

				_pEngine->Render_AddModel(pParticleHook, _WMat, *pAnimState, XR_MODEL_STANDARD, _Flags);
				
				//pAnimState->m_pModelInstance = pModelInstance;
			}
		}

		pAnimState->m_pModelInstance = pModelInstance;
	}
}

void CXR_Model_EffectSystem::Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
{
	MSCOPESHORT(CXR_Model_EffectSystem::Render);

	if(!m_pRD)
	{
		// This happends is the resource is missing
		return;
	}

	const CMTime& AnimTime0 = _pAnimState->m_AnimTime0;
	fp32 Time0 = AnimTime0.GetTime();
	bool IsReset = AnimTime0.IsReset();

	if(((IsReset || Time0 < 0.0f) && !(m_Flags & FXFLAGS_HASHISTORY)) )// || (m_Duration > 0 && m_Flags & FXFLAGS_TIMEMODE_CONTROLLED && _pAnimState->m_AnimTime0.GetTime() > m_Duration))
		return;

	// Time to bail? because of self destruction ?
	if(m_Duration > 0.0f && (m_Flags & FXFLAGS_TIMEMODE_CONTROLLED) && Time0 > m_Duration)
	{
//		ConOutLD(CStrF("§cff0NOTE CXR_Model_EffectSystem::Render: §cfffTime exceeded (%f > %f), letting go of FXFLAGS_ISALIVE.", _pAnimState->m_AnimTime0.GetTime(), m_Duration));
		return;
		//m_Flags &= ~FXFLAGS_ISALIVE;
	}

	// Initialize Render Data for this effect

	// Assign camera orientations
	m_pRD->m_CameraUp	= CVec3Dfp32::GetMatrixRow(_VMat, 0);
	m_pRD->m_CameraLeft = CVec3Dfp32::GetMatrixRow(_VMat, 1);
	m_pRD->m_CameraFwd	= CVec3Dfp32::GetMatrixRow(_VMat, 2);
	m_pRD->m_CameraPos	= CVec3Dfp32::GetMatrixRow(_VMat, 3);

	// Assign system animation times
	m_pRD->m_AnimTime0 = _pAnimState->m_AnimTime0;
	m_pRD->m_AnimTime1 = _pAnimState->m_AnimTime1;
	m_pRD->m_Time0 = m_pRD->m_AnimTime0.GetTime();
	m_pRD->m_Time1 = m_pRD->m_AnimTime1.GetTime();
	m_pRD->m_Anim0 = _pAnimState->m_Anim0;
	m_pRD->m_Anim1 = _pAnimState->m_Anim1;
	m_pRD->m_AnimAttr0 = (uint32)_pAnimState->m_AnimAttr0;
	
	m_pRD->m_pAnimState = (CXR_AnimState*)_pAnimState;

	// Assign seeding
	m_pRD->m_Seeding = 0;

	// Assign system flags
	m_pRD->m_Flags = m_Flags;
	
	// Fetch history object
	m_pRD->m_pHistory = 0;
	if(m_Flags & FXFLAGS_HASHISTORY && _pAnimState && _pAnimState->m_pModelInstance)
		m_pRD->m_pHistory = safe_cast<CEffectSystemHistory>(_pAnimState->m_pModelInstance);

	// Get surface context
	m_pRD->m_pSurfaceContext = GetSurfaceContext();

	// Clear VB Chain
	m_pRD->InitVBChain(_pRenderParams->m_pVBM);

	// Fetch engine and class data
	m_pRD->m_pEngine = _pRenderParams->m_pEngine;
	m_pRD->m_pFXSystem = this;
	m_pRenderParams	= _pRenderParams;
	
	// Setup matrices
	m_pRD->m_WMat = _WMat;
	m_pRD->m_VMat = _VMat;

	// Skeleton instance (Used in quad rendering, fix this!)
	{
		m_pRD->m_pSkeletonInst = _pAnimState->m_pSkeletonInst;
		m_pRD->m_iSkeletonType = (uint32)_pAnimState->m_Data[FXANIM_DATA_SKELETONTYPE];
		m_pRD->m_pSkeleton = (CXR_Skeleton*)_pAnimState->m_Data[FXANIM_DATA_SKELETON];
	}

	if(m_Flags & FXFLAGS_HASSHADER)
	{
		int bRenderTextureVertFlip  = m_pRD->m_pEngine->m_RenderCaps_Flags & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;
		const CRC_Viewport* pVP = m_pRD->m_pVBM->Viewport_Get();
		const CPnt ScreenSize = m_pRD->m_pEngine->m_pRender->GetDC()->GetScreenSize();
		const CPnt ScreenTexSize = (m_pRD->m_pEngine->m_pRender->Caps_Flags() & CRC_CAPS_FLAGS_ARBITRARY_TEXTURE_SIZE) ?
									ScreenSize : CPnt(GetGEPow2(ScreenSize.x), GetGEPow2(ScreenSize.y));
		const fp32 sw = (fp32)pVP->GetViewRect().GetWidth();
		const fp32 sh = (fp32)pVP->GetViewRect().GetHeight();
		const CMat4Dfp32& ProjMat = ((CRC_Viewport*)pVP)->GetProjectionMatrix();
		
		// Setup all predefined parameters
//		if(m_pRD->m_lShaderParam.Len() < m_nMaxShaderParams)
//			m_pRD->m_lShaderParam.SetLen(m_nMaxShaderParams);
//		CVec4Dfp32* pShaderParam = m_pRD->m_lShaderParam.GetBasePtr();
		CVec4Dfp32* pShaderParam = m_pRD->m_ShaderParam;

		//CVec4Dfp32* pShaderParam = m_pRD->m_ShaderParam;

		pShaderParam[0].k[0] = 1.0f / sw;
		pShaderParam[0].k[1] = 1.0f / sh;
		pShaderParam[0].k[2] = sw / ScreenTexSize.x;
		pShaderParam[0].k[3] = sh / ScreenTexSize.y;

		// Min xy, Max xy
		pShaderParam[1].k[0] = 0.0f;
		pShaderParam[1].k[2] = pShaderParam[0].k[2];
		if(bRenderTextureVertFlip)
		{
			pShaderParam[1].k[1] = (1.0f - pShaderParam[0].k[3]);// + pShaderParam[0].k[1];
			pShaderParam[1].k[3] = 1.0f;
		}
		else
		{
			pShaderParam[1].k[1] = 1.0f;
			pShaderParam[1].k[3] = (1.0f - pShaderParam[0].k[3]);// + pShaderParam[0].k[1];
		}

		// Animation time etc.
		pShaderParam[2].k[0] = (_pAnimState->m_Data[FXANIM_DATA_FLAGS] & FXANIM_FLAGS_ANIMTIME0) ? m_pRD->m_Time0 : m_pRD->m_Time1;
		pShaderParam[2].k[1] = pVP->GetFrontPlane();	// 0; (bRenderTextureVertFlip) ? 0 : 1;
		pShaderParam[2].k[2] = pVP->GetBackPlane();		// 0; (bRenderTextureVertFlip) ? 1 : 0;
		pShaderParam[2].k[3] = 0;

		pShaderParam[3] = *(CVec4Dfp32*)ProjMat.k[0];
		pShaderParam[4] = *(CVec4Dfp32*)ProjMat.k[3];

		// Setup texture generation
		//pTaxGenParam[0] = 0;
	}

	const uint32 nHistoryEntries = (m_pRD->m_pHistory) ? m_pRD->m_pHistory->GetNumHistoryEntries() : 0;
	uint nObjects = m_nObjects;
	const uint8* pObjectsType = m_lObjectsType.GetBasePtr();
	const CFXDataBeam* pBeamList = m_lBeams.GetBasePtr();
	const CFXDataQuad* pQuadList = m_lQuads.GetBasePtr();
	//const CFXDataParticlesOpt* pParticleList = m_lParticlesOpt.GetBasePtr();
	const CFXDataBeamStrip* pBeamStripList = m_lBeamStrips.GetBasePtr();
	const CFXDataFXParticle* pFXParticleList = m_lFXParticles.GetBasePtr();
	const CFXDataCone* pConeList = m_lCones.GetBasePtr();
	const uint8* piObjects = m_liObjects.GetBasePtr();

	m_pRD->InitRenderChain(nObjects+nHistoryEntries);
	for(uint8 i = 0; i < nObjects; i++)
	{
		m_pRD->m_Seeding = i;
		m_RenderingObject = i;
		
		uint DataType = pObjectsType[i];
		uint ObjectIndex = piObjects[i];

		switch(DataType)
		{
			case FXDATA_TYPE_BEAM:
				{
					const CFXDataBeam* pBeam = &pBeamList[ObjectIndex];
					if((m_Flags & FXFLAGS_TIMEMODE_CONTINUOUS) || (Time0 < pBeam->m_TotalDuration /*&& !IsReset*/))
						((CFXDataBeam*)pBeam)->Render(m_pRD, _WMat);
				}
				break;
			case FXDATA_TYPE_BEAMSTRIP:
				{
					const CFXDataBeamStrip* pBeamStrip = &pBeamStripList[ObjectIndex];
					if((m_Flags & FXFLAGS_TIMEMODE_CONTINUOUS) || (Time0 < pBeamStrip->m_TotalDuration) ||(pBeamStrip->m_Flags & FXFLAG_BEAMSTRIP_ALLOWHISTORY))
						((CFXDataBeamStrip*)pBeamStrip)->Render(m_pRD, _WMat);
				}
				break;
			case FXDATA_TYPE_FXPARTICLE:
				{
					const CFXDataFXParticle* pFXParticle = &pFXParticleList[ObjectIndex];
					if((m_Flags & (FXFLAGS_TIMEMODE_CONTINUOUS | FXFLAGS_TIMEMODE_CONTINUOUSCONTROLLED)) || (Time0 <= pFXParticle->m_TotalDuration))
						((CFXDataFXParticle*)pFXParticle)->Render(m_pRD, _WMat);
				}
				break;
			case FXDATA_TYPE_QUAD:
				{
					const CFXDataQuad* pQuad = &pQuadList[ObjectIndex];
					if((m_Flags & FXFLAGS_TIMEMODE_CONTINUOUS) || (Time0 < pQuad->m_TotalDuration/* && !IsReset*/))
						((CFXDataQuad*)pQuad)->Render(m_pRD, _WMat);
				}
				break;
			case FXDATA_TYPE_CONE:
				{
					const CFXDataCone* pCone = &pConeList[ObjectIndex];
					if((m_Flags & FXFLAGS_TIMEMODE_CONTINUOUS) || (Time0 < pCone->m_TotalDuration /*&& !IsReset*/))
						((CFXDataCone*)pCone)->Render(m_pRD, _WMat);
				}
				break;
			case FXDATA_TYPE_LIGHT:
				{
					// Not yet supported, got to think about something that might queue the light for the next rendering pass
					// or something similar at least.
					// Currently supported through an attached effect system object for now.
//					CFXDataLight* pLight = &m_lLights[m_liObjects[i]];
				}
				break;
			default:
				break;
		}
	}

	m_pRD->Render();
	m_pRD->ClearRenderChain();
}

void CXR_Model_EffectSystem::OnClientRenderVis(CWorld_Client* _pWClient, const CXR_Model_EffectSystem* _pFXSystem, CXR_SceneGraphInstance* _pSGI, fp32 _AnimTime, const CMat4Dfp32& _WMat, int _iOwner)
{
	MSCOPESHORT(CXR_Model_EffectSystem::OnClientRenderVis);
	// Reset LightGUID depending on if we are on a new render frame or not
	static uint16 LightGUID = 0x2347;
	if(_pSGI->SceneGraph_Light_GetIndex(0x2347) == 0)
		LightGUID = 0x2347;

	// Fetch render data which needs to pass through RenderVis
	const CFXDataLight* pLights = _pFXSystem->m_lLights.GetBasePtr();
	const int nLights = _pFXSystem->m_lLights.Len();

	// Render lights
	uint Flags = _pFXSystem->m_Flags;
	for(int i = 0; i < nLights; i++)
		((CFXDataLight*)pLights)[i].RenderVis(_pWClient, _pSGI, LightGUID, _iOwner, _AnimTime, _WMat, Flags);
}

void CXR_Model_EffectSystem::OnClientRender(const CXR_Model_EffectSystem* _pFXSystem, CWorld_Client* _pWClient, const CMTime& _AnimTime, const CMat4Dfp32& _WMat)
{
	// Fetch render data that needs to pass through RenderClient
	const CFXDataWallmark* pWallmarks = _pFXSystem->m_lWallmarks.GetBasePtr();
	const int nWallmarks = _pFXSystem->m_lWallmarks.Len();

	// Hack...
	CMat4Dfp32 Wallmark;
	Wallmark = _WMat;
	Wallmark.GetRow(3) = _WMat.GetRow(3) + _WMat.GetRow(2);

	// Render wallmarks
	uint Flags = _pFXSystem->m_Flags;
	const CXR_SurfaceContext* pSurfaceContext = ((CXR_Model_EffectSystem*)_pFXSystem)->GetSurfaceContext();
	for(int i = 0; i < nWallmarks; i++)
		((CFXDataWallmark*)pWallmarks)[i].RenderClient(pSurfaceContext, _pWClient, _AnimTime, Wallmark, Flags);
		//((CFXDataWallmark*)pWallmarks)[i].RenderClient(pSurfaceContext, _pWClient, _AnimTime, _WMat, Flags);
}

aint CXR_Model_EffectSystem::GetParam(int _Param)
{
	if(_Param == CXR_MODEL_PARAM_TIMEMODE)
		return (m_Flags & FXFLAGS_TIMEMODE_CONTINUOUS) ? CXR_MODEL_TIMEMODE_CONTINUOUS : CXR_MODEL_TIMEMODE_CONTROLLED;
//	else if(_Param == CXR_MODEL_PARAM_ISALIVE)
//		return (m_Flags & FXFLAGS_ISALIVE) != 0;
	else if(_Param == CXR_MODEL_PARAM_ANIM)
		return (aint)&m_Duration;
	else if(_Param == CXR_MODEL_PARAM_REGISTRY)
		return (aint)m_pRegistry;

	return CXR_Model_Custom::GetParam(_Param);
}


bool CXR_Model_EffectSystem::NeedCollisionUpdate()
{
	return (m_Flags & FXFLAGS_HASCOLLISION) ? true : false;
}

bool CXR_Model_EffectSystem::NeedOnRenderVis()
{
	return (m_Flags & FXFLAGS_RENDERVIS) ? true : false;
}

bool CXR_Model_EffectSystem::NeedOnRenderClient()
{
	return (m_Flags & FXFLAGS_RENDERCLIENT) ? true : false;
}

void CXR_Model_EffectSystem::SetParam(int _Param, aint _Value)
{
//	if(_Param == CXR_MODEL_PARAM_ISALIVE)
//		m_Flags = ((_Value == 0) ? (m_Flags & ~FXFLAGS_ISALIVE) : (m_Flags | FXFLAGS_ISALIVE));
//	else
	if (_Param == CXR_MODEL_PARAM_REGISTRY)
	{
		m_pRegistry = (CRegistry*)(aint)_Value;
	}
}

CStr CXR_Model_EffectSystem::GetShader(uint _iShader)
{
	return m_lShaders[_iShader].m_Program;
}

CFXDataShader* CXR_Model_EffectSystem::GetShaderObject(uint _iShader)
{
	return &m_lShaders[_iShader];
}

void CXR_Model_EffectSystem::OnCollisionRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Model_EffectSystem& _FXModel,
												CEffectSystemHistory& _History, int _iOwner)
{
	uint nObjects = _FXModel.m_nObjects;
	
	const TArray<uint8>& lObjectType = _FXModel.m_lObjectsType;
	const TArray<uint8>& liObjects = _FXModel.m_liObjects;
	TArray<CFXDataFXParticle>& lFXParticle = _FXModel.m_lFXParticles;
	
	const uint8* pObjectType = lObjectType.GetBasePtr();
	const uint8* piObjects = liObjects.GetBasePtr();
	CFXDataFXParticle* pFXParticle = lFXParticle.GetBasePtr();

	for(int i = 0; i < nObjects; i++)
	{
		uint ObjectType = pObjectType[i];
		uint iObject = piObjects[i];

		switch(ObjectType)
		{
			case FXDATA_TYPE_FXPARTICLE:
			{
				CFXDataFXParticle& FXParticle = pFXParticle[iObject];
				FXParticle.OnCollisionRefresh(*_pWClient, _History, ((_iOwner) ? _iOwner : _pObj->m_iObject));
			}
			break;

			// List of systems that doesn't have update for collision (yet)
			case FXDATA_TYPE_BEAM:
			case FXDATA_TYPE_LIGHT:
			case FXDATA_TYPE_MODEL:
			case FXDATA_TYPE_QUAD:
			case FXDATA_TYPE_PARTICLESOPT:
			case FXDATA_TYPE_BEAMSTRIP:
				break;

			default:
				break;
		}
	}
}

void CXR_Model_EffectSystem::RetriveNumKeys(const CRegistry* _pReg, CFXDataCollect& _DataStorage)
{
	const CRegistry* pDuration = _pReg->FindChild("DURATION");

	// Found duration key, check number of keys
	if(pDuration)
	{
		CStr DurationString = pDuration->GetThisValue();
		_DataStorage.m_nKeys = 0;
		while(DurationString != "")
		{
			DurationString.GetStrSep(";");
			_DataStorage.m_nKeys++;
		}
		
		_DataStorage.m_IntFlags |= FXFLAGS_INT_ANIMATED;
	}

	// Duration wasn't found, assuming only 1 key
	else
		_DataStorage.m_nKeys = 1;

	const CRegistry* pRandomFlags = _pReg->FindChild("RANDOMFLAGS");
	if(pRandomFlags)
	{
		// Some parameters might be random during runtime, pre allocate those object parameters
		CStr RandomFlags = pRandomFlags->GetThisValue();
		while(RandomFlags != "")
		{
			CStr RandomType = RandomFlags.GetStrSep("+");
			RandomType.MakeUpperCase();
			const CRegistry* pRandomType = _pReg->FindChild(RandomType);
			if(pRandomType)
			{
				CStr Name = pRandomType->GetThisName();
				CStr Value = pRandomType->GetThisValue();
				_DataStorage.PreAllocateKey(Name, Value);
			}
			#ifndef M_RTM
				else
				{
					if(RandomType != "DIRECTION" && RandomType != "SEED" && RandomType != "BASEROTATEANGLE")
					{
						ConOutLD(CStrF("§cf00ERROR (CXR_Model_EffectSystem::RetriveNumKeys): Found random type in flag list but no random object!? (%s)", RandomType.GetStr()));
						M_TRACEALWAYS(CStrF("(CXR_Model_EffectSystem::RetriveNumKeys): Found random type in flag list but no random object!? (%s)\n", RandomType.GetStr()));
					}
				}
			#endif
		}
	}
}

void CXR_Model_EffectSystem::RetriveShaderSettings(const CRegistry* _pReg, CFXDataCollect& _DataStorage)
{
	const int nChilds = _pReg->GetNumChildren();

	int nShaderParams = 0;
	int nShaderMaps = 0;
	for(int i = 0; i < nChilds; i++)
	{
		const CRegistry* pChild = _pReg->GetChild(i);
		const CStr ThisName = pChild->GetThisName();
		if(ThisName.Find("FRAGMENTPARAM") >= 0)
			nShaderParams++;
		else if(ThisName.Find("FRAGMENTMAP") >= 0)
			nShaderMaps++;
	}

	_DataStorage.m_lShaderParams.SetLen(nShaderParams);
	_DataStorage.m_lShaderMaps.SetLen(nShaderMaps);
	CVec4Dfp32* pShaderParams = _DataStorage.m_lShaderParams.GetBasePtr();
	int* pShaderMaps = _DataStorage.m_lShaderMaps.GetBasePtr();

	// Walk nodes again, only this time we actually parse the values
	nShaderParams = 0;
	nShaderMaps = 0;
	for(int i = 0; i < nChilds; i++)
	{
		const CRegistry* pChild = _pReg->GetChild(i);
		const CStr ThisName = pChild->GetThisName();
		
		if(ThisName.Find("FRAGMENTPARAM") >= 0)
			pShaderParams[nShaderParams++].ParseString(pChild->GetThisValue());
		
		else if(ThisName.Find("FRAGMENTMAP") >= 0)
		{
			CStr ShaderMap = pChild->GetThisValue();
			ShaderMap.MakeUpperCase();
			if(ShaderMap[0] == '$')
			{
				if(ShaderMap.Find("$FRAMEBUFFER") >= 0)
					pShaderMaps[nShaderMaps++] = FXSHADER_MAP_FRAMEBUFFER;
				else if(ShaderMap.Find("$DEPTHBUFFER") >= 0)
					pShaderMaps[nShaderMaps++] = FXSHADER_MAP_DEPTHBUFFER;
				else if(ShaderMap.Find("$DEPTHSTENCILBUFFER") >= 0)
					pShaderMaps[nShaderMaps++] = FXSHADER_MAP_DEPTHSTENCILBUFFER;
				else if(ShaderMap.Find("$DEFERREDDIFFUSE") >= 0)
					pShaderMaps[nShaderMaps++] = FXSHADER_MAP_DEFERREDDIFFUSE;
				else if(ShaderMap.Find("$DEFERREDNORMAL") >= 0)
					pShaderMaps[nShaderMaps++] = FXSHADER_MAP_DEFERREDNORMAL;
				else if(ShaderMap.Find("$DEFERREDSPECULAR") >= 0)
					pShaderMaps[nShaderMaps++] = FXSHADER_MAP_DEFERREDSPECULAR;
				else if(ShaderMap.Find("$LASTSCREEN") >= 0)
					pShaderMaps[nShaderMaps++] = FXSHADER_MAP_LASTSCREEN;
				else if(ShaderMap.Find("$SHADOWMASK") >= 0)
					pShaderMaps[nShaderMaps++] = FXSHADER_MAP_SHADOWMASK;
				else if(ShaderMap.Find("$POSTPROCESS") >= 0)
					pShaderMaps[nShaderMaps++] = FXSHADER_MAP_POSTPROCESS;
			}
			else if(ShaderMap[0] == '#')
				pShaderMaps[nShaderMaps++] = FXSHADER_MAP_RENDERTARGET;
			else
				pShaderMaps[nShaderMaps++] = GetSurfaceID(ShaderMap);
		}
	}
}

void CXR_Model_EffectSystem::RetriveRenderTargetSettings(const CRegistry* _pReg, CFXDataCollect& _DataStorage)
{
	const int nChilds = _pReg->GetNumChildren();

	for(int i = 0; i < nChilds; i++)
	{
		const CRegistry* pChild = _pReg->GetChild(i);
		const CStr ThisName = pChild->GetThisName();

		if(ThisName == "NAME")
			_DataStorage.m_RenderTarget = pChild->GetThisValue();
	}
}

void CXR_Model_EffectSystem::AddRenderTarget(CFXDataCollect& _DataStorage)
{
	bool bAddRenderTarget = m_lRenderTargets.Len() ? false : true;
	CStr RenderTargetName = _DataStorage.m_RenderTarget;

	// Make sure render target doesn't already exist
	if(!bAddRenderTarget)
	{
		int nRenderTargets = m_lRenderTargets.Len();
		CFXDataRenderTarget* pRenderTargets = m_lRenderTargets.GetBasePtr();

		int i = 0;
		for(; i < nRenderTargets; i++)
		{
			if(pRenderTargets[i].m_RenderTarget == RenderTargetName)
			{
				_DataStorage.m_iRenderTarget = i + 1;
				break;
			}
		}

		if(i == nRenderTargets)
			bAddRenderTarget = true;
	}
	
	if(bAddRenderTarget)
	{
		m_lRenderTargets.Add(CFXDataRenderTarget(_DataStorage));
		_DataStorage.m_iRenderTarget = (uint8)m_lRenderTargets.Len();
	}
}

#ifndef M_RTM
uint8 CXR_Model_EffectSystem::Debug_ValueValidation(const CRegistry* _pReg, uint8 _nKeys)
{
	CStr Value = _pReg->GetThisValue();
	CStr Name = _pReg->GetThisName();

	uint8 nFoundKeys = 0;
	while(Value != "")
	{
		Value.GetStrSep(";");
		nFoundKeys++;
	}

	CRegistry* pKeyParent = _pReg->GetParent();
	CRegistry* pParentOwner = (pKeyParent) ? pKeyParent->GetParent() : NULL;
	Value = _pReg->GetThisValue();
	if(nFoundKeys < _nKeys)
	{
		ConOutLD(CStrF("§cf00ERROR (CXR_Model_EffectSystem::EvalRegisterObject): Too few values in key (%i/%i)! *%s { *%s { *%s \"%s\"} }", nFoundKeys, _nKeys, (pParentOwner) ? pParentOwner->GetThisName().GetStr() : "NULL", (pKeyParent) ? pKeyParent->GetThisName().GetStr() : "NULL", Name.GetStr(), Value.GetStr()));
		M_TRACEALWAYS(CStrF("ERROR (CXR_Model_EffectSystem::EvalRegisterObject): Too few values in key (%i/%i)!\n*%s\n{\n\t*%s\n\t{\n\t\t*%s\t\"%s\"\n\t}\n}\n\n", nFoundKeys, _nKeys, (pParentOwner) ? pParentOwner->GetThisName().GetStr() : "NULL", (pKeyParent) ? pKeyParent->GetThisName().GetStr() : "NULL", Name.GetStr(), Value.GetStr()));
	}
	else if(nFoundKeys > _nKeys)
	{
		ConOutLD(CStrF("§cf00ERROR (CXR_Model_EffectSystem::EvalRegisterObject): Too many values in key (%i/%i)! *%s { *%s { *%s \"%s\"} }", nFoundKeys, _nKeys, (pParentOwner) ? pParentOwner->GetThisName().GetStr() : "NULL", (pKeyParent) ? pKeyParent->GetThisName().GetStr() : "NULL", Name.GetStr(), Value.GetStr()));
		M_TRACEALWAYS(CStrF("ERROR (CXR_Model_EffectSystem::EvalRegisterObject): Too many values in key (%i/%i)!\n*%s\n{\n\t*%s\n\t{\n\t\t*%s\t\"%s\"\n\t}\n}\n\n", nFoundKeys, _nKeys, (pParentOwner) ? pParentOwner->GetThisName().GetStr() : "NULL", (pKeyParent) ? pKeyParent->GetThisName().GetStr() : "NULL", Name.GetStr(), Value.GetStr()));
	}

	_nKeys = MaxMT(_nKeys, nFoundKeys);
	return _nKeys;
}
#endif

void CXR_Model_EffectSystem::EvalRegisterObject(const CRegistry* _pReg, CFXDataCollect& _DataStorage)
{
	CStr ThisName = _pReg->GetThisName();
	CStr Value = _pReg->GetThisValue();
	uint8 iKeys = 0;
	const uint8 nKeys = _DataStorage.m_nKeys;

	CFXSysUtil::Init();

	if(ThisName == "SURFACE")
		CFXSysUtil::ParseAllocListSurface(_DataStorage.m_lSurfaceID, Value, nKeys);

	else if(ThisName == "SHADER")
	{
		CFXDataCollect DataCollector;
		RetriveShaderSettings(_pReg, DataCollector);
		
		const int nChilds = _pReg->GetNumChildren();
		for(int i = 0; i < nChilds; i++)
			EvalRegisterObject(_pReg->GetChild(i), DataCollector);

		m_lShaders.Add(CFXDataShader(DataCollector));
		m_liObjects.Add((m_lShaders.Len() - 1));
		m_lObjectsType.Add(FXDATA_TYPE_SHADER);
		++m_nObjects;

		// Flag for linking
		_DataStorage.m_pShader = &m_lShaders[m_lShaders.Len()-1];
		_DataStorage.m_iShader = m_lShaders.Len()-1;

		m_Flags |= FXFLAGS_HASSHADER;
	}

	else if(ThisName == "RENDERTARGET")
	{
		CFXDataCollect DataCollector;
		
		RetriveRenderTargetSettings(_pReg, DataCollector);
		AddRenderTarget(DataCollector);

		_DataStorage.m_iRenderTarget = DataCollector.m_iRenderTarget;
	}

	else if(ThisName == "RENDERTYPE")
		_DataStorage.m_RenderType = Value;

	else if(ThisName == "DURATION")
	{
		fp32* pDuration = CFXSysUtil::GetAllocList<fp32>(_DataStorage.m_lDuration, Value, nKeys);
		fp32* pDurationRecip = CFXSysUtil::GetAllocList<fp32>(_DataStorage.m_lDurationReciprocal, Value, nKeys);

		while(Value != "")
		{
			pDuration[iKeys] = Value.GetStrSep(";").Val_fp64();
			if (pDuration[iKeys])
				pDurationRecip[iKeys] = 1.0f / pDuration[iKeys];
			else
				pDurationRecip[iKeys] = 0.0f;
			_DataStorage.m_TotalDuration += pDuration[iKeys++];
		}
	}

	else if(ThisName == "PRIORITY")
		_DataStorage.m_Priority = (uint8)Value.TranslateInt(g_lpEFFECTSYSTEM_Priority);

	else if(ThisName == "PRIORITYOFFSET")
		_DataStorage.m_PriorityOffset = Value.Val_fp64();

	// Start Shader control
	else if(ThisName == "PROGRAM")
		_DataStorage.m_ProgramName = CStr("WModel_FX") + Value;

	else if(ThisName == "TEXGEN")
		_DataStorage.m_iTexGen = (uint8)Value.TranslateInt(g_lpEFFECTSYSTEM_SHADER_TEXGEN);

	else if(ThisName == "DISABLE")
		_DataStorage.m_Disable = Value.TranslateFlags(g_lpEFFECTSYSTEM_EnableDisable);

	else if(ThisName == "ENABLE")
		_DataStorage.m_Enable = Value.TranslateFlags(g_lpEFFECTSYSTEM_EnableDisable);

	else if(ThisName == "RASTERMODE")
		_DataStorage.m_RasterMode = Value.TranslateInt(g_lpEFFECTSYSTEM_RasterMode);

	else if(ThisName == "ZCOMPARE")
		_DataStorage.m_ZCompare = Value.TranslateInt(g_lpEFFECTSYSTEM_Compare);

	else if(ThisName == "ALPHACOMPARE")
		_DataStorage.m_AlphaCompare = Value.TranslateInt(g_lpEFFECTSYSTEM_Compare);

	else if(ThisName == "ALPHAREF")
		_DataStorage.m_AlphaRef = (uint16)Value.Val_int();
	// End Shader control

	else if(ThisName == "SEED")
		_DataStorage.m_Seed = Value.Val_int();

	else if(ThisName == "LENGTH")
		CFXSysUtil::ParseAllocListFp(_DataStorage.m_lLength, Value, nKeys);

	else if(ThisName == "TOLERANCE")
		CFXSysUtil::ParseAllocListFp(_DataStorage.m_lTolerance, Value, nKeys);

	else if(ThisName == "WIDTH")
		CFXSysUtil::ParseAllocListFp(_DataStorage.m_lWidth, Value, nKeys);

	else if(ThisName == "WIDTH2")
		CFXSysUtil::ParseAllocListFp(_DataStorage.m_lWidth2, Value, nKeys);

	else if(ThisName == "COLOR")
		CFXSysUtil::ParseAllocListCol(_DataStorage.m_lColor, Value, nKeys);

	else if(ThisName == "ROTATEANGLE")
		CFXSysUtil::ParseAllocListFp(_DataStorage.m_lRotationAngle, Value, nKeys);

	else if(ThisName == "SEGMENTS")
		CFXSysUtil::ParseAllocListInt(_DataStorage.m_lSegments, Value, nKeys);

	else if(ThisName == "MULTI")
		_DataStorage.m_nMulti = Value.Val_int();

	else if(ThisName == "RANGE")
		CFXSysUtil::ParseAllocListFp(_DataStorage.m_lRange, Value, nKeys);

	else if(ThisName == "INTENSITY")
		CFXSysUtil::ParseAllocListVec(_DataStorage.m_lIntensity, Value, nKeys);

	else if(ThisName == "STRIPHEIGHT")
		CFXSysUtil::ParseAllocListFp(_DataStorage.m_lStripHeight, Value, nKeys);

	else if(ThisName == "WIDTHNOISE")
		CFXSysUtil::ParseAllocListFp(_DataStorage.m_lWidthNoise, Value, nKeys);

	else if(ThisName == "POSNOISE")
		CFXSysUtil::ParseAllocListVec(_DataStorage.m_lPosNoise, Value, nKeys);

	else if(ThisName == "DIRECTION")
		CFXSysUtil::ParseAllocListVec(_DataStorage.m_lDirection, Value, nKeys);

	else if(ThisName == "VELOCITY")
		CFXSysUtil::ParseAllocListVec(_DataStorage.m_lVelocity, Value, nKeys);

	else if(ThisName == "VELOCITYNOISE")
		CFXSysUtil::ParseAllocListVec(_DataStorage.m_lVelocityNoise, Value, nKeys);

	else if(ThisName == "TEXTURESCROLL")
		CFXSysUtil::ParseAllocListVec(_DataStorage.m_lTextureScroll, Value, nKeys);

	else if(ThisName == "POSITIONOFFSET")
		CFXSysUtil::ParseAllocListVec(_DataStorage.m_lPosOffset, Value, nKeys);

	else if(ThisName == "INNERRADIUS")
		CFXSysUtil::ParseAllocListFp(_DataStorage.m_lInnerRadius, Value, nKeys);

	else if(ThisName == "OUTERRADIUS")
		CFXSysUtil::ParseAllocListFp(_DataStorage.m_lOuterRadius, Value, nKeys);

	else if(ThisName == "SPHERERADIUS")
		CFXSysUtil::ParseAllocListFp(_DataStorage.m_lSphereRadius, Value, nKeys);

	else if(ThisName == "HEIGHT")
		CFXSysUtil::ParseAllocListFp(_DataStorage.m_lHeight, Value, nKeys);
	
	else if(ThisName == "RINGS")
		CFXSysUtil::ParseAllocListInt(_DataStorage.m_lRings, Value, nKeys);

	else if(ThisName == "FLAGS")
		_DataStorage.m_Flags = Value;

	else if(ThisName == "RANDOMFLAGS")
		_DataStorage.m_RandomFlags = Value;

	else if(ThisName == "HISTORYTIME")
		_DataStorage.m_HistoryTime = Value.Val_fp64();

	else if(ThisName == "HISTORYSPAWN")
		_DataStorage.m_HistorySpawn = Value.Val_fp64();

	else if(ThisName == "LAYER_TWIRL")
		_DataStorage.m_Layer.OnEvalKey<CFXLayerTwirl>(Value);
		
	else if(ThisName == "LAYER_BONEBIND")
		_DataStorage.m_Layer.OnEvalKey<CFXLayerBoneBind>(Value);

	else if(ThisName == "LAYER_BOXSPAWN")
		_DataStorage.m_Layer.OnEvalKey<CFXLayerBoxSpawn>(Value);

	else if(ThisName == "LAYER_NOISE")
		_DataStorage.m_Layer.OnEvalKey<CFXLayerNoise>(Value);

	else if(ThisName == "LAYER_PATH")
		_DataStorage.m_Layer.OnEvalKey<CFXLayerPath>(Value);

	/*
	else if(ThisName == "LAYER_TWIRL")
	{
		const int iSet = _DataStorage.m_lTwirlLayer.Len();
		_DataStorage.m_lTwirlLayer.SetLen(iSet + 1);
		CFXLayerTwirl& Twirl = _DataStorage.m_lTwirlLayer[iSet];

		CStr UCase = Value.GetStrSep(",").UpperCase();
		if(UCase == "X")
			Twirl.m_iComponents = 0;
		else if(UCase == "Y")
			Twirl.m_iComponents = (1 << 2);
		else if(UCase == "Z")
			Twirl.m_iComponents = (2 << 2);

		UCase = Value.GetStrSep(",").UpperCase();
		if(UCase == "Y")
			Twirl.m_iComponents |= 1;
		else if (UCase == "Z")
			Twirl.m_iComponents |= 2;

		Twirl.m_Offset = (fp32)Value.GetStrSep(",").Val_fp64();
		Twirl.m_Speed = (fp32)Value.GetStrSep(",").Val_fp64();
		Twirl.m_Radius = (fp32)Value.GetStrSep(",").Val_fp64();
		Twirl.m_Loops = (fp32)Value.GetStrSep(",").Val_fp64();
	}

	else if(ThisName == "LAYER_BONEBIND")
	{
		const int iSet = _DataStorage.m_lBoneBindLayer.Len();
		_DataStorage.m_lBoneBindLayer.SetLen(iSet + 1);
		CFXLayerBoneBind& BoneBind = _DataStorage.m_lBoneBindLayer[iSet];

		// Read bone binding layer data
	}

	else if(ThisName == "LAYER_NOISE")
	{
		const int iSet = _DataStorage.m_lNoiseLayer.Len();
		_DataStorage.m_lNoiseLayer.SetLen(iSet + 1);
		CFXLayerNoise& Noise = _DataStorage.m_lNoiseLayer[iSet];

		Noise.m_bAnimated = Value.GetStrSep(",").Val_int() ? true : false;
		Noise.m_Timescale = (fp32)Value.GetStrSep(",").Val_fp64();
		Noise.m_Noise.ParseString(Value.GetStrSep(","));
	}

	else if(ThisName == "LAYER_PATH")
	{
		const int iSet = _DataStorage.m_lPathLayer.Len();
		_DataStorage.m_lPathLayer.SetLen(iSet + 1);
		CFXLayerPath& Path = _DataStorage.m_lPathLayer[iSet];

		int nPathNodes = Value.GetStrSep(",").Val_int();
		Path.m_lPath.SetLen(nPathNodes);
		CVec3Dfp32* pPathNodes = Path.m_lPath.GetBasePtr();
		for(int i = 0; i < nPathNodes; i++)
			pPathNodes[i].ParseString(Value.GetStrSep(","));
	}
	*/

	// Specific for optimized particle system / An extended name has been added for readability
	else if(ThisName == "OP_STRING")
	{
		// Make sure surface gets precached
		CStr FindSurface = Value;
		while(FindSurface != "")
		{
			CStr FindSurfaceParam = FindSurface.GetStrSep(",");
			if(FindSurfaceParam != "")
			{
				CStr FSP_Name = FindSurfaceParam.GetStrSep("=");
				FSP_Name.MakeUpperCase();
				if(FSP_Name == "SU")
					GetSurfaceID(FindSurfaceParam.GetStrMSep(" #"));
			}
		}

		// Store particle string
		_DataStorage.ParticleKeyString(Value);
	}

	// Particle string names
	else if(ThisName == "OP_SURFACENAME" || ThisName == "SU")
	{
        GetSurfaceID(Value);
		_DataStorage.ParticleKey("SU", Value);
	}
	else if(ThisName == "OP_MAXPARTICLES" || ThisName == "MP")
		_DataStorage.ParticleKey("MP", Value);
	else if(ThisName == "OP_TIMESCALE" || ThisName == "TS")
		_DataStorage.ParticleKey("TS", Value);
	else if(ThisName == "OP_TIMEOFFSET" || ThisName == "TO")
		_DataStorage.ParticleKey("TO", Value);
	else if(ThisName == "OP_DURATION" || ThisName == "DU")
		_DataStorage.ParticleKey("DU", Value);
	else if(ThisName == "OP_DURATIONNOISE" || ThisName == "DUN")
		_DataStorage.ParticleKey("DUN", Value);
	else if(ThisName == "OP_OFFSET" || ThisName == "OF")
		_DataStorage.ParticleKey("OF", Value);
	else if(ThisName == "OP_OFFSETNOISE" || ThisName == "OFN")
		_DataStorage.ParticleKey("OFN", Value);
	else if(ThisName == "OP_VELOCITY" || ThisName == "VE")
		_DataStorage.ParticleKey("VE", Value);
	else if(ThisName == "OP_VELOCITYNOISE" || ThisName == "VEN")
		_DataStorage.ParticleKey("VEN", Value);
	else if(ThisName == "OP_SLOWDOWNPOWER" || ThisName == "SD")
		_DataStorage.ParticleKey("SD", Value);
	else if(ThisName == "OP_STOPMOTIONTIME" || ThisName == "SMT")
		_DataStorage.ParticleKey("SMT", Value);
	else if(ThisName == "OP_LOCALOFFSET" || ThisName == "LO")
		_DataStorage.ParticleKey("LO", Value);
	else if(ThisName == "OP_ACCELERATION" || ThisName == "AX")
		_DataStorage.ParticleKey("AX", Value);
	else if(ThisName == "OP_ACCELERATIONNOISE" || ThisName == "AXN")
		_DataStorage.ParticleKey("AXN", Value);
	else if(ThisName == "OP_ANGLESPREADA" || ThisName == "SPA")
		_DataStorage.ParticleKey("SPA", Value);
	else if(ThisName == "OP_ANGLESPREADB" || ThisName == "SPB")
		_DataStorage.ParticleKey("SPB", Value);
	else if(ThisName == "OP_ANGLEOFFSETA" || ThisName == "OFA")
		_DataStorage.ParticleKey("OFA", Value);
	else if(ThisName == "OP_ANGLEOFFSETB" || ThisName == "OFB")
		_DataStorage.ParticleKey("OFB", Value);
	else if(ThisName == "OP_ANGLEOFFSETCHANGEA" || ThisName == "DOFA")
		_DataStorage.ParticleKey("DOFA", Value);
	else if(ThisName == "OP_ANGLEOFFSETCHANGEB" || ThisName == "DOFB")
		_DataStorage.ParticleKey("DOFB", Value);
	else if(ThisName == "OP_MOVESTARTMIN" || ThisName == "MV0")
		_DataStorage.ParticleKey("MV0", Value);
	else if(ThisName == "OP_MOVESTARTMAX" || ThisName == "MV1")
		_DataStorage.ParticleKey("MV1", Value);
	else if(ThisName == "OP_MOVEENDMIN" || ThisName == "MV2")
		_DataStorage.ParticleKey("MV2", Value);
	else if(ThisName == "OP_MOVEENDMAX" || ThisName == "MV3")
		_DataStorage.ParticleKey("MV3", Value);
	else if(ThisName == "OP_MOVESPEED" || ThisName == "MV4")
		_DataStorage.ParticleKey("MV4", Value);
	else if(ThisName == "OP_SIZESTARTMIN" || ThisName == "SZ0")
		_DataStorage.ParticleKey("SZ0", Value);
	else if(ThisName == "OP_SIZESTARTMAX" || ThisName == "SZ1")
		_DataStorage.ParticleKey("SZ1", Value);
	else if(ThisName == "OP_SIZEENDMIN" || ThisName == "SZ2")
		_DataStorage.ParticleKey("SZ2", Value);
	else if(ThisName == "OP_SIZEENDMAX" || ThisName == "SZ3")
		_DataStorage.ParticleKey("SZ3", Value);
	else if(ThisName == "OP_SIZESPEED" || ThisName == "SZ4")
		_DataStorage.ParticleKey("SZ4", Value);
	else if(ThisName == "OP_ALPHASTART" || ThisName == "AL0")
		_DataStorage.ParticleKey("AL0", Value);
	else if(ThisName == "OP_ALPHAEND" || ThisName == "AL1")
		_DataStorage.ParticleKey("AL1", Value);
	else if(ThisName == "OP_ROTATIONSTARTMIN" || ThisName == "RT0")
		_DataStorage.ParticleKey("RT0", Value);
	else if(ThisName == "OP_ROTATIONSTARTMAX" || ThisName == "RT1")
		_DataStorage.ParticleKey("RT1", Value);
	else if(ThisName == "OP_ROTATIONENDMIN" || ThisName == "RT2")
		_DataStorage.ParticleKey("RT2", Value);
	else if(ThisName == "OP_ROTATIONENDMAX" || ThisName == "RT3")
		_DataStorage.ParticleKey("RT3", Value);
	else if(ThisName == "OP_ROTATIONSPEED" || ThisName == "RT4")
		_DataStorage.ParticleKey("RT4", Value);
	else if(ThisName == "OP_FADEIN" || ThisName == "FI")
		_DataStorage.ParticleKey("FI", Value);
	else if(ThisName == "OP_FADESTILL" || ThisName == "FS")
		_DataStorage.ParticleKey("FS", Value);
	else if(ThisName == "OP_LENGTHSCALE" || ThisName == "LS")
		_DataStorage.ParticleKey("LS", Value);
	else if(ThisName == "OP_COLOR" || ThisName == "CO")
		_DataStorage.ParticleKey("CO", Value);
	else if(ThisName == "OP_COLORNOISE" || ThisName == "CON")
		_DataStorage.ParticleKey("CON", Value);
	else if(ThisName == "OP_EMISSIONPROB" || ThisName == "EP")
		_DataStorage.ParticleKey("EP", Value);
	else if(ThisName == "OP_EMISSIONPROBTIMESCALE" || ThisName == "EPC")
		_DataStorage.ParticleKey("EPC", Value);
	else if(ThisName == "OP_TIMECELL" || ThisName == "TC")
		_DataStorage.ParticleKey("TC", Value);
	else if(ThisName == "OP_TIMECELLSPREAD" || ThisName == "TCS")
		_DataStorage.ParticleKey("TCS", Value);
	else if(ThisName == "OP_EMISSIONSTOP" || ThisName == "ES")
		_DataStorage.ParticleKey("ES", Value);
	else if(ThisName == "OP_FLAGS" || ThisName == "FL")
		_DataStorage.ParticleKey("FL", Value);
	else if(ThisName == "OP_DISTRIBUTIONPRIMITIVE" || ThisName == "DI")
		_DataStorage.ParticleKey("DI", Value);
	else if(ThisName == "OP_DISTRIBUTIONSIZE" || ThisName == "DIS")
		_DataStorage.ParticleKey("DIS", Value);
	else if(ThisName == "OP_HOLLOWDISTRIBUTION" || ThisName == "HO")
		_DataStorage.ParticleKey("HO", Value);
	else if(ThisName == "OP_DISTRIBUTIONROTATION" || ThisName == "DR")
		_DataStorage.ParticleKey("DR", Value);
	else if(ThisName == "OP_RANDOMSEED" || ThisName == "RS")
		_DataStorage.ParticleKey("RS", Value);
	// End specific
}

CEffectSystemHistory::CEffectSystemHistory(uint _nObjects, uint _nModelInstances, const CEffectSystemHistory::CCreationParam* _pParams)
{
	m_lspModelInstances.SetLen(_nModelInstances);
	m_lObjectHistory.SetLen(_nObjects);
	m_lObjectHistory.SetGrow(0);
	m_Objects = _nObjects;
	m_ModelInstances = _nModelInstances;

	if(_pParams)
	{
		CEffectSystemHistory::CEffectSystemHistoryObject* pObjectHistory = m_lObjectHistory.GetBasePtr();
		for(int i = 0; i < _nObjects; i++)
			pObjectHistory[i].m_SizeEntry = _pParams[i].m_Param0;
	}

	TPtr<CXR_ModelInstance>* pspModelInstances = m_lspModelInstances.GetBasePtr();
	for(uint8 i = 0; i < _nModelInstances; i++)
		pspModelInstances[i] = NULL;
}

CEffectSystemHistory::CEffectSystemHistory(const CEffectSystemHistory* pObject)
{
	pObject->m_lspModelInstances.Duplicate(&m_lspModelInstances);
	pObject->m_lObjectHistory.Duplicate(&m_lObjectHistory);
	m_Objects = pObject->m_Objects;
	m_ModelInstances = pObject->m_ModelInstances;
	m_GameTick = pObject->m_GameTick;
}

void CEffectSystemHistory::Create(CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
{
}

bool CEffectSystemHistory::NeedRefresh(CXR_Model* _pModel, const CXR_ModelInstanceContext& _Context)
{
	return true;
}

void CEffectSystemHistory::OnRefresh(const CXR_ModelInstanceContext& _Context, const CMat4Dfp32* _pMat, int _nMat, int _Flags)
{
	// Check against last game tick
	m_GameTick = _Context.m_GameTick;
}

TPtr<CXR_ModelInstance> CEffectSystemHistory::Duplicate() const
{
	CEffectSystemHistory* pNewHistory = MNew2(CEffectSystemHistory, m_Objects, m_ModelInstances);
	
	m_lspModelInstances.Duplicate(&pNewHistory->m_lspModelInstances);
	m_lObjectHistory.Duplicate(&pNewHistory->m_lObjectHistory);
	pNewHistory->m_Objects = m_Objects;
	pNewHistory->m_GameTick = m_GameTick;
	pNewHistory->m_ModelInstances = m_ModelInstances;

	return TPtr<CXR_ModelInstance>(pNewHistory);
}


void CEffectSystemHistory::operator =(const CEffectSystemHistory& _Instance)
{
//	const CEffectSystemHistoryObject* pSrcObject = _Instance.m_lObjectHistory.GetBasePtr();
//	CEffectSystemHistoryObject* pDstObject = m_lObjectHistory.GetBasePtr();

	m_lspModelInstances = _Instance.m_lspModelInstances;
	m_lObjectHistory = _Instance.m_lObjectHistory;
	m_GameTick = _Instance.m_GameTick;
    m_Objects = _Instance.m_Objects;
	m_ModelInstances = _Instance.m_ModelInstances;
}


void CEffectSystemHistory::operator =(const CXR_ModelInstance& _Instance)
{
	const CEffectSystemHistory& From = *safe_cast<const CEffectSystemHistory>(&_Instance);
	*this = From;

}

CEffectSystemHistory::CEffectSystemHistoryObject* CEffectSystemHistory::GetHistory(uint _nObject)
{
	return &m_lObjectHistory[_nObject];
}

CEffectSystemHistory::CEffectSystemHistoryObject* CEffectSystemHistory::GetHistory(uint _nObject) const
{
	return const_cast<CEffectSystemHistory::CEffectSystemHistoryObject*>(&m_lObjectHistory[_nObject]);
}

TPtr<CXR_ModelInstance> CEffectSystemHistory::GetModelInstance(uint _iObject)
{
	return m_lspModelInstances[_iObject];
}

TPtr<CXR_ModelInstance> CEffectSystemHistory::GetModelInstance(uint _iObject) const
{
	return m_lspModelInstances[_iObject];
}

