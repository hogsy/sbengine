#include "PCH.h"

#include "WBSP2Model.h"
#include "WBSP2Def.h"
#include "MFloat.h"
#include "../../XR/XREngineVar.h"
#include "../../Classes/BitString/MBitString.h"

//#define MODEL_BSP_EXACTPORTALCULL
//#define MODEL_BSP_EXACTRELAXED

extern int aShiftMulTab[];

//#ifdef PLATFORM_WIN_PC
//#define MODEL_BSP_NUMRPORTALS	192
//#else
#define MODEL_BSP_NUMRPORTALS	512
//#endif

#define MACRO_ISMIRRORED(Mat)	\
	((CVec3Dfp32::GetMatrixRow(Mat, 0) / CVec3Dfp32::GetMatrixRow(Mat, 1)) * CVec3Dfp32::GetMatrixRow(Mat, 2) < 0.0f)

#define fMin(a, b) ((((a)+(b)) - fp32(M_Fabs((a)-(b)))) * 0.5f)
#define fMax(a, b) ((((a)+(b)) + fp32(M_Fabs((a)-(b)))) * 0.5f)
#define fMin2(a, b) (((a)+(b)) - fp32(M_Fabs((a)-(b))))
#define fMax2(a, b) (((a)+(b)) + fp32(M_Fabs((a)-(b))))

void CalcBoxScissorOld(const CBSP2_ViewInstance* _pView, const CBox3Dfp32& _Box, CScissorRect& _Scissor)
{
	CVec3Dfp32 BoxV[8];
	_Box.GetVertices(BoxV);

	const CMat4Dfp32* pM = &_pView->m_VPVMat;

	int ScrX = _pView->m_VPRect.p0.x;
	int ScrY = _pView->m_VPRect.p0.y;
	int ScrW = _pView->m_VPRect.GetWidth();
	int ScrH = _pView->m_VPRect.GetHeight();
	CVec2Dfp32 VMin(_FP32_MAX);
	CVec2Dfp32 VMax(-_FP32_MAX);

	{
		fp32 mx = pM->k[0][2];
		fp32 my = pM->k[1][2];
		fp32 mz = pM->k[2][2];
		fp32 mw = pM->k[3][2];
		fp32 z0 = (BoxV[0].k[0] * mx + BoxV[0].k[1] * my + BoxV[0].k[2] * mz + mw) - 0.1f;
		fp32 z1 = (BoxV[1].k[0] * mx + BoxV[1].k[1] * my + BoxV[1].k[2] * mz + mw) - 0.1f;
		fp32 z2 = (BoxV[2].k[0] * mx + BoxV[2].k[1] * my + BoxV[2].k[2] * mz + mw) - 0.1f;
		fp32 z3 = (BoxV[3].k[0] * mx + BoxV[3].k[1] * my + BoxV[3].k[2] * mz + mw) - 0.1f;
		fp32 z4 = (BoxV[4].k[0] * mx + BoxV[4].k[1] * my + BoxV[4].k[2] * mz + mw) - 0.1f;
		fp32 z5 = (BoxV[5].k[0] * mx + BoxV[5].k[1] * my + BoxV[5].k[2] * mz + mw) - 0.1f;
		fp32 z6 = (BoxV[6].k[0] * mx + BoxV[6].k[1] * my + BoxV[6].k[2] * mz + mw) - 0.1f;
		fp32 z7 = (BoxV[7].k[0] * mx + BoxV[7].k[1] * my + BoxV[7].k[2] * mz + mw) - 0.1f;

		fp32 z = M_FSel(z7, M_FSel(z6, M_FSel(z5, M_FSel(z4, M_FSel(z3, M_FSel(z2, M_FSel(z1, M_FSel(z0, 0.0f, 1.0f), 1.0f), 1.0f), 1.0f), 1.0f), 1.0f), 1.0f), 1.0f);
		if(z > 0.0f)
		{
			_Scissor.SetRect(ScrX, ScrY, ScrX + ScrW, ScrY + ScrH);
			return;
		}
	}

	for(int v = 0; v < 8; v++)
	{
		fp32 vx = BoxV[v].k[0];
		fp32 vy = BoxV[v].k[1];
		fp32 vz = BoxV[v].k[2];
		fp32 z = pM->k[0][2]*vx + pM->k[1][2]*vy + pM->k[2][2]*vz + pM->k[3][2];
		fp32 zinv = 1.0f / z;
		fp32 x = (pM->k[0][0]*vx + pM->k[1][0]*vy + pM->k[2][0]*vz + pM->k[3][0]) * zinv;
		VMin.k[0] = Min(VMin.k[0], x);
		VMax.k[0] = Max(VMax.k[0], x);
		fp32 y = (pM->k[0][1]*vx + pM->k[1][1]*vy + pM->k[2][1]*vz + pM->k[3][1]) * zinv;
		VMin.k[1] = Min(VMin.k[1], y);
		VMax.k[1] = Max(VMax.k[1], y);
	}

	{
		int xmin = RoundToInt(VMin[0] * _pView->m_VPScaleVec[0] + _pView->m_VPMidVec[0]);
		int min0 = Max(ScrX, Min(ScrX + ScrW, xmin));
		int ymin = RoundToInt(VMin[1] * _pView->m_VPScaleVec[1] + _pView->m_VPMidVec[1]);
		int min1 = Max(ScrY, Min(ScrY + ScrH, ymin));

		int xmax = RoundToInt(VMax[0] * _pView->m_VPScaleVec[0] + _pView->m_VPMidVec[0]);
		int max0 = Max(ScrX, Min(ScrX + ScrW, xmax));
		int ymax = RoundToInt(VMax[1] * _pView->m_VPScaleVec[1] + _pView->m_VPMidVec[1]);
		int max1 = Max(ScrY, Min(ScrY + ScrH, ymax));

		_Scissor.SetRect(min0, min1, Max(max0, min0), Max(max1, min1));
	}
}

M_FORCEINLINE void M_VGetBoxVertices(vec128 _min, vec128 _max, vec128& _v0, vec128& _v1, vec128& _v2, vec128& _v3, vec128& _v4, vec128& _v5, vec128& _v6, vec128& _v7)
{
	vec128 xsel = M_VConstMsk(1, 0, 0, 0);
	vec128 ysel = M_VConstMsk(0, 1, 0, 0);
	vec128 zsel = M_VConstMsk(0, 0, 1, 0);

	_v0 = _min;
	_v1 = M_VSelMsk(xsel, _max, _min);
	_v2 = M_VSelMsk(ysel, _max, _v0);
	_v3 = M_VSelMsk(ysel, _max, _v1);
	_v4 = M_VSelMsk(zsel, _max, _v0);
	_v5 = M_VSelMsk(zsel, _max, _v1);
	_v6 = M_VSelMsk(zsel, _max, _v2);
	_v7 = M_VSelMsk(zsel, _max, _v3);
}

vec128 CalcBoxScissor(const CBSP2_ViewInstance* _pView, vec128 _BoxMin, vec128 _BoxMax)
{
	vec128 v0, v1, v2, v3, v4, v5, v6, v7;
	M_VGetBoxVertices(_BoxMin, _BoxMax, v0, v1, v2, v3, v4, v5, v6, v7);
//	CVec4Dfp32 BoxV[8];
//	_Box.GetVerticesV4(BoxV);

	const CMat4Dfp32* pM = &_pView->m_VPVMat;

	vec128 m0 = pM->r[0];
	vec128 m1 = pM->r[1];
	vec128 m2 = pM->r[2];
	vec128 m3 = pM->r[3];
	M_VTranspose4x4(m0, m1, m2, m3);

	vec128 ScrExtMinVec = _pView->m_VPRectMinVec.v;
	vec128 ScrExtMaxVec = _pView->m_VPRectMaxVec.v;

	vec128 zcomp = M_VScalar(0.1f);
/*	vec128 v0 = BoxV[0].v;
	vec128 v1 = BoxV[1].v;
	vec128 v2 = BoxV[2].v;
	vec128 v3 = BoxV[3].v;
	vec128 v4 = BoxV[4].v;
	vec128 v5 = BoxV[5].v;
	vec128 v6 = BoxV[6].v;
	vec128 v7 = BoxV[7].v;*/
	vec128 z03 = M_VDp4x4(v0, m2, v1, m2, v2, m2, v3, m2);
	vec128 z47 = M_VDp4x4(v4, m2, v5, m2, v6, m2, v7, m2);
	vec128 zmin = M_VMin(z03, z47);
	if (M_VCmpAnyLT(zmin, zcomp))
	{
		// Retarded, I know. Doesn't happen too often though.
		CScissorRect Scissor;
		Scissor.SetRect(_pView->m_VPRect.p0.x, _pView->m_VPRect.p0.y, _pView->m_VPRect.p1.x, _pView->m_VPRect.p1.y);
		return M_VLd64(&Scissor);
//		return;
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

	vec128 xyminmax = M_VAdd(M_VMul(M_VSelMsk(M_VConstMsk(1,1,0,0), xymin2, xymax2), _pView->m_VPScaleVec), _pView->m_VPMidVec.v);

	vec128 xyminmax_u32 = M_VCnv_f32_i32(M_VAdd(xyminmax, M_VHalf()));
	vec128 xyminmax2_u32 = M_VMin_i32(M_VMax_i32(xyminmax_u32, ScrExtMinVec), ScrExtMaxVec);
#ifdef CPU_BIGENDIAN
	vec128 shuf = M_VShuf(xyminmax2_u32, M_VSHUF(1,0,3,2));
#else
	vec128 shuf = xyminmax2_u32;
#endif
	vec128 final = M_VCnvS_i32_i16(shuf, shuf);
	return final;
//	_Scissor.SetRect(xyminmax2_u32.k[0], xyminmax2_u32.k[1], xyminmax2_u32.k[2], xyminmax2_u32.k[3]);
/*	CVec4Duint32 xyminmax2_u32;
	xyminmax2_u32.v = M_VMin_i32(M_VMax_i32(xyminmax_u32, ScrExtMinVec), ScrExtMaxVec);
	_Scissor.SetRect(xyminmax2_u32.k[0], xyminmax2_u32.k[1], xyminmax2_u32.k[2], xyminmax2_u32.k[3]);*/
}

/*
void CalcBoxScissor(const CBSP2_ViewInstance* _pView, const CBox3Dfp32& _Box, CScissorRect& _Scissor)
{
	CScissorRect Ref;
	CalcBoxScissorOld(_pView, _Box, Ref);
	CalcBoxScissorNew(_pView, _Box, _Scissor);

	if (!_Scissor.IsEqual(Ref))
	{
		M_TRACEALWAYS("New %d, %d, %d, %d\n", _Scissor.GetMinX(), _Scissor.GetMinY(), _Scissor.GetMaxX(), _Scissor.GetMaxY());
		M_TRACEALWAYS("Old %d, %d, %d, %d\n", Ref.GetMinX(), Ref.GetMinY(), Ref.GetMaxX(), Ref.GetMaxY());
	}
}*/

// -------------------------------------------------------------------
//#define PORTAL_AND_DEBUG

int CXR_Model_BSP2::Portal_And(const CRC_ClipVolume* _pPortal, const CBSP2_PortalExt* _pP, bool _bFrontThis, CRC_ClipVolume* _pPDest, CXR_FogState* _pSceneFog) const
{
	MAUTOSTRIP(CXR_Model_BSP_Portal_And, 0);
	MIncProfile(m_nPortal0);
	// ANDs _pPortal and _pP, puts the result in _DestP
	
	fp32 Dist = m_pPlanes[_pP->m_iPlane].Distance(_pPortal->m_POV);
	if (!_bFrontThis) Dist = -Dist;
	if (Dist < -0.1f) return 0;

	int nv = _pP->m_nVertices;
	int iiv = _pP->m_iiVertices;
	if (nv < 3)
	{
		ConOut(CStrF("§cf80WARNING: Fucked up portal!  %d verts.", nv));
		return 0;
	}

	int nPlanes = _pPortal->m_nPlanes;
	CVec3Dfp32 POV = _pPortal->m_POV;

//m_TimePortal0 -= GetCPUClock();

	if (Dist < 1.0f)
	{
		*_pPDest = *_pPortal;
#ifdef PORTAL_AND_DEBUG
		ConOut("Nu!");
#endif
		return 1;
	}
#ifdef PORTAL_AND_DEBUG
	ConOut("Inte nu!");
#endif

//m_TimePortal0 -= GetCPUClock();
//	_pPDest->GetVertices(&m_pVertices[m_piVertices[iiv]], nv, POV, _bFrontThis);
	{
		_pPDest->m_nPlanes = nv;
		_pPDest->m_POV = POV;
		if (_bFrontThis)
			for(int v = 0; v < nv; v++)
				_pPDest->m_Vertices[v] = m_pVertices[m_piVertices[iiv + nv - v - 1]];
		else
			for(int v = 0; v < nv; v++)
				_pPDest->m_Vertices[v] = m_pVertices[m_piVertices[iiv + v]];
	}

	// Clip portal to backplane
	{
		int bClip;
		_pPDest->m_nPlanes = BSPModel_CutFace3(&_pPDest->m_Vertices[0], _pPDest->m_nPlanes, &m_pView->m_LocalBackPlane, 1, bClip);
		if (!_pPDest->m_nPlanes) return 0;

		if (_pSceneFog && _pSceneFog->VertexFogEnable())
			_pPDest->m_nPlanes = BSPModel_CutFace3(&_pPDest->m_Vertices[0], _pPDest->m_nPlanes, &_pSceneFog->m_VtxFog_EndPlane, 1, bClip);
	}

#ifdef MODEL_BSP_PORTALBOUNDBOX

	{
		CPlane3Dfp32 ClipPlanes[16];

		CVec3Dfp32 BoxMin = _pP->m_BoxMin;
		CVec3Dfp32 BoxMax = _pP->m_BoxMax;

		int nClip = 0;
		for(int ip = 0; ip < nPlanes; ip++)
		{
			fp32 MinDist, MaxDist;
			CPlane3Dfp32 Plane = _pPortal->m_Planes[ip];
			Plane.GetBoxMinAndMaxDistance(BoxMin, BoxMax, MinDist, MaxDist);
			if(MinDist > 0.0f)
			{
				_pPDest->m_nPlanes = 0;
				return 0;
			}
			if(MaxDist > 0.0f)
			{
				ClipPlanes[nClip++] = Plane;
			}
		}
/*
		const CVec3Dfp32& VMin = _pP->m_BoxMin;
		const CVec3Dfp32& VMax = _pP->m_BoxMax;
//		CVec3Dfp32 Center;
//		Center.k[0] = (VMin.k[0] + VMax.k[0]) * 0.5f;
//		Center.k[1] = (VMin.k[1] + VMax.k[1]) * 0.5f;
//		Center.k[2] = (VMin.k[2] + VMax.k[2]) * 0.5f;

		int nClip = 0;
		for(int ip = 0; ip < nPlanes; ip++)
		{
			const CPlane3Dfp32* pP = &_pPortal->m_Planes[ip];
//			if (pP->Distance(Center) > 0.0f)
			{
//				fp32 x0 = VMin.k[0]*pP->n.k[0];
//				fp32 x1 = VMax.k[0]*pP->n.k[0];
//				fp32 min = fMin2(x0, x1);
//				fp32 max = fMax2(x0, x1);
//				fp32 y0 = VMin.k[1]*pP->n.k[1];
//				fp32 y1 = VMax.k[1]*pP->n.k[1];
//				min += fMin2(y0, y1);
//				max += fMax2(y0, y1);
//				fp32 z0 = VMin.k[2]*pP->n.k[2];
//				fp32 z1 = VMax.k[2]*pP->n.k[2];
//				min += fMin2(z0, z1);
//				max += fMax2(z0, z1);
//				if (min > pP->d*-2.0f)
//				{
//					_pPDest->m_nPlanes = 0;
//					return 0;
//				}
//
//				if (max > pP->d*-2.0f)
//					ClipPlanes[nClip++] = *pP;

				fp32 xMin, xMax, yMin, yMax, zMin, zMax;
				if (pP->n.k[0] > 0.0f) 
					{ xMin = VMin.k[0]*pP->n.k[0]; xMax = VMax.k[0]*pP->n.k[0]; }
				else
					{ xMin = VMax.k[0]*pP->n.k[0]; xMax = VMin.k[0]*pP->n.k[0]; }

				if (pP->n.k[1] > 0.0f) 
					{ yMin = VMin.k[1]*pP->n.k[1]; yMax = VMax.k[1]*pP->n.k[1]; }
				else
					{ yMin = VMax.k[1]*pP->n.k[1]; yMax = VMin.k[1]*pP->n.k[1]; }

				if (pP->n.k[2] > 0.0f) 
					{ zMin = VMin.k[2]*pP->n.k[2]; zMax = VMax.k[2]*pP->n.k[2]; }
				else
					{ zMin = VMax.k[2]*pP->n.k[2]; zMax = VMin.k[2]*pP->n.k[2]; }

				if (xMin + yMin + zMin + pP->d > 0.0f)
				{
					_pPDest->m_nPlanes = 0;
	//				m_TimePortal0 += GetCPUClock(); //ConOut("Easy skip.");
					return 0;
				}

				if (xMax + yMax + zMax + pP->d > 0.0f)
					ClipPlanes[nClip++] = *pP;
			}
		}
*/
		if (!nClip)
		{
#ifndef MODEL_BSP_CFV3
			_pPDest->CreateFromVertices();
#else
			_pPDest->CreateFromVertices3(m_pView->m_LocalBackPlane);
#endif
//			ConOut("No Clip.");
//			m_TimePortal0 += GetCPUClock();
			return 1;
		}
		MAddProfile(m_nPortal2, nPlanes - nClip);

		int bClip;
		_pPDest->m_nPlanes = BSPModel_CutFace3(&_pPDest->m_Vertices[0], _pPDest->m_nPlanes, &ClipPlanes[0], nClip, bClip);
//		_pPDest->m_nPlanes = BSPModel_CutFace2(&_pPDest->m_Vertices[0], _pPDest->m_nPlanes, &ClipPlanes[0], nClip, true);
		if (_pPDest->m_nPlanes) 
#ifndef MODEL_BSP_CFV3
			_pPDest->CreateFromVertices();
#else
			_pPDest->CreateFromVertices3(m_pView->m_LocalBackPlane);
#endif

//		ms_TimePortal0 += GetCPUClock();
		return bClip ^ 1;
	}
#else
	int bClip;
	_pPDest->m_nPlanes = BSPModel_CutFace3(&_pPDest->m_Vertices[0], _pPDest->m_nPlanes, &_pPortal->m_Planes[0], nPlanes, bClip);
//	_pPDest->m_nPlanes = BSPModel_CutFace2(&_pPDest->m_Vertices[0], _pPDest->m_nPlanes, &_pPortal->m_Planes[0], nPlanes, true);
	if (!_pPDest->m_nPlanes)
	{
//ms_TimePortal0 += GetCPUClock();
		return 0;
	}
#ifndef MODEL_BSP_CFV3
	_pPDest->CreateFromVertices();
#else
	_pPDest->CreateFromVertices3(m_pView->m_LocalBackPlane);
#endif
#endif
//RenderRPortal(_pPDest, 0xff4040ff);
//ms_TimePortal0 += GetCPUClock();


#ifdef PORTAL_AND_DEBUG
	for(v = 0; v < _pPDest->m_nPlanes; v++)
	{
		for (int p = 0; p < _pPortal->m_nPlanes; p++)
			if (_pPortal->m_Planes[p].GetPlaneSide_Epsilon(_pPDest->m_Vertices[v], 0.1f) > 0)
			{
				ConOut(CStrF("P-And: Point %d outside plane %d", v, p));
			}
	}

#endif

	return 1;
}


#define PORTAL_OR_DEBUG

void CXR_Model_BSP2::Portal_Or(CRC_ClipVolume* _pP1, const CRC_ClipVolume* _pP2, CBSP2_View_Params* _pViewParam) const
{
	MAUTOSTRIP(CXR_Model_BSP_Portal_Or, MAUTOSTRIP_VOID);
	if (!_pP1->m_nPlanes)
	{
		// Portal 1 is void, just copy.
//		_pP1->Copy(*_pP2);
		*_pP1 = *_pP2;
		return;
	}
	else if (!_pP2->m_nPlanes)
	{
		// Portal 2 is void, do nada.
		return;
	}
	else
	{
#ifdef PORTAL_OR_DEBUG
//			ConOut("Or-ing..");
#endif
//		m_nPortal3++;

		int nv1 = _pP1->m_nPlanes;
		int nv2 = _pP2->m_nPlanes;

		CVec2Dfp32 VMin(_FP32_MAX);
		CVec2Dfp32 VMax(-_FP32_MAX);
		const CMat4Dfp32* pM = &_pViewParam->m_pViewData->m_CurVMat;

		bool bErr = false;
		{
			const CVec3Dfp32* pV = &_pP1->m_Vertices[0];
			for(int v = 0; v < nv1; v++)
			{
				fp32 vx = pV[v].k[0];
				fp32 vy = pV[v].k[1];
				fp32 vz = pV[v].k[2];
				fp32 z = pM->k[0][2]*vx + pM->k[1][2]*vy + pM->k[2][2]*vz + pM->k[3][2];
				if (z < 0.1f) { bErr = true; break; }
				fp32 zinv = m_pView->m_ViewBackPlane / z;
				fp32 x = (pM->k[0][0]*vx + pM->k[1][0]*vy + pM->k[2][0]*vz + pM->k[3][0]) * zinv;
				if (x < VMin.k[0]) VMin.k[0] = x;
				if (x > VMax.k[0]) VMax.k[0] = x;
				fp32 y = (pM->k[0][1]*vx + pM->k[1][1]*vy + pM->k[2][1]*vz + pM->k[3][1]) * zinv;
				if (y < VMin.k[1]) VMin.k[1] = y;
				if (y > VMax.k[1]) VMax.k[1] = y;
			}
		}

		if (!bErr)
		{
			const CVec3Dfp32* pV = &_pP2->m_Vertices[0];
			for(int v = 0; v < nv2; v++)
			{
				fp32 vx = pV[v].k[0];
				fp32 vy = pV[v].k[1];
				fp32 vz = pV[v].k[2];
				fp32 z = pM->k[0][2]*vx + pM->k[1][2]*vy + pM->k[2][2]*vz + pM->k[3][2];
				if (z < 0.1f) { bErr = true; break; }
				fp32 zinv = m_pView->m_ViewBackPlane / z;
				fp32 x = (pM->k[0][0]*vx + pM->k[1][0]*vy + pM->k[2][0]*vz + pM->k[3][0]) * zinv;
				if (x < VMin.k[0]) VMin.k[0] = x;
				if (x > VMax.k[0]) VMax.k[0] = x;
				fp32 y = (pM->k[0][1]*vx + pM->k[1][1]*vy + pM->k[2][1]*vz + pM->k[3][1]) * zinv;
				if (y < VMin.k[1]) VMin.k[1] = y;
				if (y > VMax.k[1]) VMax.k[1] = y;
			}
		}

		if (bErr)
		{
			ConOut("P-Or: Using view min/max");

			VMin = _pViewParam->m_PortalOrMin;
			VMax = _pViewParam->m_PortalOrMax;
			VMin *= m_pView->m_ViewBackPlane;
			VMax *= m_pView->m_ViewBackPlane;
		}

		CVec3Dfp32 VVerts[4];
		if (MACRO_ISMIRRORED(_pViewParam->m_pViewData->m_CurVMatInv))
		{
			VVerts[0] = CVec3Dfp32(VMin.k[0], VMin.k[1], m_pView->m_ViewBackPlane);
			VVerts[1] = CVec3Dfp32(VMin.k[0], VMax.k[1], m_pView->m_ViewBackPlane);
			VVerts[2] = CVec3Dfp32(VMax.k[0], VMax.k[1], m_pView->m_ViewBackPlane);
			VVerts[3] = CVec3Dfp32(VMax.k[0], VMin.k[1], m_pView->m_ViewBackPlane);
		}
		else
		{
			VVerts[3] = CVec3Dfp32(VMin.k[0], VMin.k[1], m_pView->m_ViewBackPlane);
			VVerts[2] = CVec3Dfp32(VMin.k[0], VMax.k[1], m_pView->m_ViewBackPlane);
			VVerts[1] = CVec3Dfp32(VMax.k[0], VMax.k[1], m_pView->m_ViewBackPlane);
			VVerts[0] = CVec3Dfp32(VMax.k[0], VMin.k[1], m_pView->m_ViewBackPlane);
		}

//		ConOut(CStrF("Min (%f, %f),  Max (%f, %f)", VMin.k[0], VMin.k[1], VMax.k[0], VMax.k[1]));
		CVec3Dfp32::MultiplyMatrix(&VVerts[0], &_pP1->m_Vertices[0], _pViewParam->m_pViewData->m_CurVMatInv, 4);
		_pP1->m_nPlanes = 4;
		_pP1->CreateFromVertices2();
		
/*		for(v = 0; v < 4; v++)
			ConOut(_pP1->m_Vertices[v].GetString());*/

//	RenderRPortal(_pP1, 0xffff40);
	}
}


#ifdef MODEL_BSP_WRITEPORTALTIMING
static int g_nPotentialPVSRejects = 0;
#endif

void CXR_Model_BSP2::Portal_AddNode(CBSP2_View_Params* _pViewParams, int _iNode, int _iClipRPortal)
{
	MAUTOSTRIP(CXR_Model_BSP_Portal_AddNode, MAUTOSTRIP_VOID);
	const CBSP2_Node* pNode = &(m_pNodes[_iNode]);
	if (!(pNode->m_Flags & XW_NODE_STRUCTURE) ||
		(!(pNode->m_Flags & XW_NODE_ENABLE))) return;

	if (m_pView->m_nVisLeaves >= m_pView->m_MaxVisLeaves) return;

	if (pNode->m_Flags & XW_NODE_PORTAL)
	{
		// This is a portal leaf
		int iPL = pNode->m_iPortalLeaf;
		if ((ms_Enable & MODEL_BSP_ENABLE_PVSCULL) && !m_pView->InPVS(iPL))
		{
//			ConOut(CStrF("PL %d rejected with PVS.", iPL));
			return;
		}

		{
//if (!NumFacesInTree(_iNode)) return;
			const CBSP2_PortalLeafExt* pPL = &m_pPortalLeaves[pNode->m_iPortalLeaf];

			int np = pPL->m_nPortals;
			int iip = pPL->m_iiPortals;

#ifndef MODEL_BSP_EXACTPORTALCULL
			bool bCanDelete = false;
			int iRPortal = m_pView->m_liLeafRPortals[pNode->m_iPortalLeaf];
			if (!iRPortal)
			{
				if (m_pView->m_nRPortals >= m_pView->m_MaxRPortals)
				{
					ConOut("§cf80WARNING: Insufficient portal storage.");
					return;
				}
				iRPortal = m_pView->m_nRPortals;
				m_pWorldView->m_liLeafRPortals[pNode->m_iPortalLeaf] = iRPortal;
				bCanDelete = true;
				m_pWorldView->m_pRPortals[iRPortal].Init(_pViewParams->m_pViewData->m_CurLocalVP);
				m_pWorldView->m_piRPortalNext[iRPortal] = 0;
				m_pWorldView->m_nRPortals++;
			}
#endif

//			if (m_iView > 0)
//				RenderRPortal(&m_pView->m_pRPortals[_iClipRPortal], 0xffffffff);

			bool bFrontThis = true;
			int iNodeNgbr = 0;
			for(int p = 0; p < np; p++)
			{
				int ip = m_piPortals[iip + p];
				const CBSP2_PortalExt* pP = &m_pPortals[ip];

//#ifndef MODEL_BSP_ALLPORTALSOPEN
#ifdef MODEL_BSP_ALLUSERPORTALSCLOSED
				if (pP->m_PortalID) continue;
#endif
				if (pP->m_PortalID && !m_pView->PortalOpen(pP->m_PortalID)) continue;
//#endif

				if (pP->m_iNodeFront == _iNode)
				{	bFrontThis = true;	iNodeNgbr = pP->m_iNodeBack; }
				else if (pP->m_iNodeBack == _iNode)
				{	bFrontThis = false;	iNodeNgbr = pP->m_iNodeFront; }
				else
/*#ifdef MODEL_BSP_PARANOIA
					Error("Portal_AddNode", "Invalid portal linking.");
#else*/
					continue;
//#endif

				int iRPNgbr = m_pView->m_liLeafRPortals[m_pNodes[iNodeNgbr].m_iPortalLeaf];
//ConOut(CStrF("PL %d, iNodeNgbr %d, iRPNgbr %d", iPL, iNodeNgbr, iRPNgbr));
#ifdef MODEL_BSP_EXACTPORTALCULL
#ifdef MODEL_BSP_EXACTRELAXED
				int iRPNew = m_pView->m_nRPortals;
				m_pView->m_pRPortals[iRPNew].Init(_pViewParams->m_pViewData->m_CurLocalVP);
#endif

				int nClipVol = 0;
				while(iRPNgbr)
				{
					CRC_ClipVolume* pPortal = &m_pView->m_pRPortals[iRPNgbr];
#ifdef MODEL_BSP_EXACTRELAXED
					CRC_ClipVolume DestPortal;
#else
					int iRPNew = m_pView->m_nRPortals;
					CRC_ClipVolume& DestPortal = m_pView->m_pRPortals[iRPNew];
#endif

					if (m_pView->m_nRPortals >= m_pView->m_MaxRPortals)
					{
						ConOut("§cf80WARNING: Insufficient portal storage.");
						return;
					}

					DestPortal.Init(_pViewParams->m_pViewData->m_CurLocalVP);
					T_StartAddProfile(m_TimePortalAnd)
					Portal_And(pPortal, &m_pPortals[ip], !bFrontThis, &DestPortal, _pViewParams->m_pSceneFog);
					T_StopProfile(m_TimePortalAnd);

#ifdef MODEL_BSP_EXACTRELAXED
					if (DestPortal.m_nPlanes >= 3)
					{
						Portal_Or(&m_pView->m_pRPortals[iRPNew], &DestPortal);
						nClipVol++;
					}
#else
					if (DestPortal.m_nPlanes >= 3)
					{
						int iRPLast = m_pView->m_liLeafRPortals[iPL];
						m_pView->m_piRPortalNext[iRPNew] = iRPLast;
						m_pView->m_liLeafRPortals[iPL] = iRPNew;
						m_pView->m_nRPortals++;
						nClipVol++;
	//				ConOut(CStrF("PL %d, Added RP %d after %d, nRP %d", iPL, iRPNew, iRPLast, m_pView->m_nRPortals));
					}
#endif

					iRPNgbr = m_pView->m_piRPortalNext[iRPNgbr];
				}

#ifdef MODEL_BSP_EXACTRELAXED
				if (m_pView->m_pRPortals[iRPNew].m_nPlanes >= 3)
				{
					int iRPLast = m_pView->m_liLeafRPortals[iPL];
					m_pView->m_piRPortalNext[iRPNew] = iRPLast;
					m_pView->m_liLeafRPortals[iPL] = iRPNew;
					m_pView->m_nRPortals++;
//				ConOut(CStrF("PL %d, Added RP %d after %d, nRP %d", iPL, iRPNew, iRPLast, m_pView->m_nRPortals));
				}
#endif

//				if (nClipVol > 4) ConOut(CStrF("PL %d, nRP %d", iPL, nClipVol));
#else
				if (iRPNgbr != 0)
				{
					CRC_ClipVolume* pPortal = &m_pView->m_pRPortals[iRPNgbr];
					CRC_ClipVolume DestPortal;

					DestPortal.Init(_pViewParams->m_pViewData->m_CurLocalVP);
					{
						TMeasureProfile(m_TimePortalAnd);
						Portal_And(pPortal, &m_pPortals[ip], !bFrontThis, &DestPortal, _pViewParams->m_pSceneFog);
					}

					{
						TMeasureProfile(m_TimePortalOr);
						if (DestPortal.m_nPlanes >= 3)
							Portal_Or(&m_pView->m_pRPortals[iRPortal], &DestPortal, _pViewParams);
					}
				}
#endif

			} // end portal loop

#ifdef MODEL_BSP_EXACTPORTALCULL
			int iRPFirst = m_pView->m_liLeafRPortals[iPL];
			if (iRPFirst)
#else
			if (m_pView->m_pRPortals[iRPortal].m_nPlanes >= 3)
#endif
			{
				MIncProfile(m_nPortal3);
				for(int p = 0; p < np; p++)
				{
					int ip = m_piPortals[iip + p];
					const CBSP2_PortalExt* pP = &m_pPortals[ip];
//#ifndef MODEL_BSP_ALLPORTALSOPEN
#ifdef MODEL_BSP_ALLUSERPORTALSCLOSED
					if (pP->m_PortalID) continue;
#endif
					if (pP->m_PortalID && !m_pView->PortalOpen(pP->m_PortalID)) continue;

					EnableTreeFromNode((pP->m_iNodeFront == _iNode) ? pP->m_iNodeBack : pP->m_iNodeFront);
				}

//#ifndef MODEL_BSP_EXACTPORTALCULL
				_iClipRPortal = m_pView->m_liLeafRPortals[iPL];
//#endif
			}
			else
			{
#ifndef MODEL_BSP_EXACTPORTALCULL
				if (bCanDelete)
				{
					m_pWorldView->m_liLeafRPortals[iPL] = 0;
					m_pWorldView->m_nRPortals--;
				}
#endif
				return;
			}
		}
//		RenderNodePortals(_iNode);

		// Add portal-leaf to list
		{
			if (!m_pWorldView->InPVS(iPL))
			{
#ifdef MODEL_BSP_WRITEPORTALTIMING
				g_nPotentialPVSRejects++;
#endif
				//ConOut(CStrF("PL %d rejected with PVS.", iPL));
				// return;
			}
			
			m_pWorldView->m_liVisPortalLeaves[m_pWorldView->m_nVisLeaves++] = iPL;
//			m_pView->m_liLeafRPortals[pNode->m_iPortalLeaf] = _iClipRPortal;
		}
		return;
	}

	if (pNode->IsLeaf())
	{
//		ConOut("Ett vadå?");
		// This is a leaf!
//		m_pView->m_pCurVisLeaves[m_pView->m_nCurVisLeaves++] = _iNode;
//		m_pView->m_liLeafRPortals[pNode->m_iPortalLeaf] = _iClipRPortal;
	}
	else
	{
//m_TimePortal3 -= GetCPUClock();
		// What sides of the plane can we see?

//#ifdef PORTAL_NO_BSP_CULL
		if (!(pNode->m_Flags & XW_NODE_STRUCTURE))
		{
			int iPlane = m_pNodes[_iNode].m_iPlane;
			const CPlane3Dfp32* pP = &m_pPlanes[iPlane];
			fp32 d = pP->Distance(_pViewParams->m_pViewData->m_CurLocalVP);

			int iFront = m_pNodes[_iNode].m_iNodeFront;
			int iBack = m_pNodes[_iNode].m_iNodeBack;
			if (d >= 0.0f)
			{
				if (iFront) Portal_AddNode(_pViewParams, iFront, _iClipRPortal);
				if (iBack) Portal_AddNode(_pViewParams, iBack, _iClipRPortal);
			}
			else
			{
				if (iBack) Portal_AddNode(_pViewParams, iBack, _iClipRPortal);
				if (iFront) Portal_AddNode(_pViewParams, iFront, _iClipRPortal);
			}
		}
		else
		{
//#else // PORTAL_NO_BSP_CULL
			int iPlane = m_pNodes[_iNode].m_iPlane;
			const CPlane3Dfp32* pP = &m_pPlanes[iPlane];
			fp32 d = pP->Distance(_pViewParams->m_pViewData->m_CurLocalVP);
			if (d >= 0.0f)
			{
				int iFront = m_pNodes[_iNode].m_iNodeFront;
				if (iFront) Portal_AddNode(_pViewParams, iFront, _iClipRPortal);

				int iBack = m_pNodes[_iNode].m_iNodeBack;
				if (iBack)
				{
					CRC_ClipVolume* pRP = &m_pView->m_pRPortals[_iClipRPortal];
					for(int v = 0; v < pRP->m_nPlanes; v++)
						if (pP->Distance(pRP->m_Vertices[v]) < 0.0f)
						{
							Portal_AddNode(_pViewParams, iBack, _iClipRPortal);
							return;
						}
				}
			}
			else
			{
				int iBack = m_pNodes[_iNode].m_iNodeBack;
				if (iBack) Portal_AddNode(_pViewParams, iBack, _iClipRPortal);

				int iFront = m_pNodes[_iNode].m_iNodeFront;
				if (iFront)
				{
					CRC_ClipVolume* pRP = &m_pView->m_pRPortals[_iClipRPortal];
					for(int v = 0; v < pRP->m_nPlanes; v++)
						if (pP->Distance(pRP->m_Vertices[v]) > 0.0f)
						{
							Portal_AddNode(_pViewParams, iFront, _iClipRPortal);
							return;
						}
				}
			}
		}
//#endif // PORTAL_NO_BSP_CULL
	}
}

void CXR_Model_BSP2::Portal_OpenClipFront_r(CBSP2_View_Params* _pViewParams, const CBSP2_Node* pNode, const CVec3Dfp32* _pVPortal, int _nVPortal)
{
	MAUTOSTRIP(CXR_Model_BSP_Portal_OpenClipFront_r, MAUTOSTRIP_VOID);
//LogFile(CStrF("ClipFront %d", pNode->m_iNodeFront));
	CVec3Dfp32 Portal[32];
	memcpy(Portal, _pVPortal, _nVPortal*sizeof(CVec3Dfp32));

	int bClip;
	CPlane3Dfp32 Plane(m_lPlanes[pNode->m_iPlane]);
	Plane.Inverse();
	int Side = Plane.GetArrayPlaneSideMask_Epsilon(_pVPortal, _nVPortal, 0.01f);
	if (Side == 4)
	{
		Portal_Open_r(_pViewParams, pNode->m_iNodeFront, Portal, _nVPortal);
	}
	else
	{
		_nVPortal = BSPModel_CutFace3(Portal, _nVPortal, &Plane, 1, bClip);
		if (_nVPortal > 2) Portal_Open_r(_pViewParams, pNode->m_iNodeFront, Portal, _nVPortal);
	}
}

void CXR_Model_BSP2::Portal_OpenClipBack_r(CBSP2_View_Params* _pViewParams, const CBSP2_Node* pNode, const CVec3Dfp32* _pVPortal, int _nVPortal)
{
	MAUTOSTRIP(CXR_Model_BSP_Portal_OpenClipBack_r, MAUTOSTRIP_VOID);
//LogFile(CStrF("ClipBack %d", pNode->m_iNodeBack));
	CVec3Dfp32 Portal[32];
	memcpy(Portal, _pVPortal, _nVPortal*sizeof(CVec3Dfp32));

	int bClip;
	int Side = m_lPlanes[pNode->m_iPlane].GetArrayPlaneSideMask_Epsilon(_pVPortal, _nVPortal, 0.01f);
	if (Side == 4)
	{
		Portal_Open_r(_pViewParams, pNode->m_iNodeBack, Portal, _nVPortal);
	}
	else
	{
		_nVPortal = BSPModel_CutFace3(Portal, _nVPortal, &m_lPlanes[pNode->m_iPlane], 1, bClip);
		if (_nVPortal > 2) Portal_Open_r(_pViewParams, pNode->m_iNodeBack, Portal, _nVPortal);
	}
}

void CXR_Model_BSP2::Portal_Open_r(CBSP2_View_Params* _pViewParams, int _iNode, const CVec3Dfp32* _pVPortal, int _nVPortal)
{
	MAUTOSTRIP(CXR_Model_BSP_Portal_Open_r, MAUTOSTRIP_VOID);
	const CBSP2_Node* pNode = &(m_pNodes[_iNode]);
//LogFile(CStrF("Open %d, F %d, B %d", _iNode, pNode->m_iNodeFront, pNode->m_iNodeBack));
	if (pNode->m_Flags & XW_NODE_PORTAL)
	{
//LogFile(CStrF("   Assign %d", m_pView->m_nRPortals));
		if (m_pWorldView->m_nRPortals >= m_pWorldView->m_MaxRPortals)
		{
//LogFile("   Full");
			ConOut("Full");
			return;
		}

		int iPL = pNode->m_iPortalLeaf;
/*		if (!m_pWorldView->m_pPVS)
		{
			m_pWorldView->m_pPVS = SceneGraph_PVSLock(0, iPL);
		}*/

		CRC_ClipVolume* pRP = &m_pWorldView->m_pRPortals[m_pWorldView->m_nRPortals];
		m_pWorldView->m_liLeafRPortals[iPL] = m_pWorldView->m_nRPortals;
		if (_nVPortal > 16)
		{
			ConOut(CStrF("Portal %d", _nVPortal));
			_nVPortal = 16;
		}

		pRP->GetVertices(_pVPortal, _nVPortal, _pViewParams->m_pViewData->m_CurLocalVP, false);
		pRP->CreateFromVertices3(m_pWorldView->m_LocalBackPlane);
		m_pWorldView->m_nRPortals++;
		EnableTreeFromNode(_iNode);


		// Markera alla grannars träd.
		{
			const CBSP2_PortalLeafExt* pPL = &m_pPortalLeaves[pNode->m_iPortalLeaf];
			int np = pPL->m_nPortals;
			int iip = pPL->m_iiPortals;

			for(int p = 0; p < np; p++)
			{
				int ip = m_piPortals[iip + p];
				const CBSP2_PortalExt* pP = &m_pPortals[ip];
//#ifndef MODEL_BSP_ALLPORTALSOPEN
#ifdef MODEL_BSP_ALLUSERPORTALSCLOSED
				if (pP->m_PortalID) continue;
#endif
				if (pP->m_PortalID && !m_pWorldView->PortalOpen(pP->m_PortalID)) continue;
//#endif
				EnableTreeFromNode((pP->m_iNodeFront == _iNode) ? pP->m_iNodeBack : pP->m_iNodeFront);
			}
		}

	}
	else if (pNode->IsNode())
	{
		if (!(pNode->m_Flags & XW_NODE_STRUCTURE)) return;

//		const CPlane3Dfp32* pP = &m_lPlanes[pNode->m_iPlane];

/*		int SideMask = 0;
		for(int v = 0; v < _nVPortal; v++)
		{
			SideMask |= pP->GetPlaneSideMask_Epsilon(_pVPortal[v], 0.0001f);
			if ((SideMask & 3) == 3)
			{
				if (pNode->m_iNodeFront)
					Portal_OpenClipFront_r(pNode, _pVPortal, _nVPortal);
				if (pNode->m_iNodeBack)
					Portal_OpenClipBack_r(pNode, _pVPortal, _nVPortal);
				return;
			}
		}

		if ((SideMask & 3 == 1) && pNode->m_iNodeFront)
			Portal_Open_r(pNode->m_iNodeFront, _pVPortal, _nVPortal);
		if ((SideMask & 3 == 2) && pNode->m_iNodeBack)
			Portal_Open_r(pNode->m_iNodeBack, _pVPortal, _nVPortal);*/

		if (pNode->m_iNodeFront)
			Portal_OpenClipFront_r(_pViewParams, pNode, _pVPortal, _nVPortal);
		if (pNode->m_iNodeBack)
			Portal_OpenClipBack_r(_pViewParams, pNode, _pVPortal, _nVPortal);
	}
}


int CXR_Model_BSP2::Portal_GetFirstOpen_r(int _iNode, const CVec3Dfp32& _v)
{
	MAUTOSTRIP(CXR_Model_BSP_Portal_GetFirstOpen_r, 0);
	CBSP2_Node* pN = &m_lNodes[_iNode];
	if (pN->m_Flags & XW_NODE_PORTAL)
	{

		return _iNode;
	}
	else if (pN->IsNode())
	{
		if (pN->IsLeaf()) 
			Error("Portal_GetFirstOpen", "*krick*!");
		
	}
	return 0;
}

// -------------------------------------------------------------------
//  Visibility
// -------------------------------------------------------------------

void CXR_Model_BSP2::View_CalcScissors(CBSP2_View_Params* _pViewParam)
{
	int nRP = m_pWorldView->m_nRPortals;
	const CMat4Dfp32* pM = &_pViewParam->m_pViewData->m_CurVMat;

//	CRC_Viewport* pVP = _pViewParam->m_pCurrentEngine->GetVBM()->Viewport_Get();
	const CRct ViewRect = m_pWorldView->m_VPRect;
	int ScrX = ViewRect.p0.x;
	int ScrY = ViewRect.p0.y;
	int ScrW = ViewRect.GetWidth();
	int ScrH = ViewRect.GetHeight();
	fp32 MidX = m_pWorldView->m_VPMidVec[0];
	fp32 MidY = m_pWorldView->m_VPMidVec[1];
	fp32 ScaleX = m_pWorldView->m_VPScaleVec[0];
	fp32 ScaleY = m_pWorldView->m_VPScaleVec[1];

	for(int i = 0; i < nRP; i++)
	{
		const CRC_ClipVolume* pClip = &m_pWorldView->m_lRPortals[i];

		CVec2Dfp32 VMin(_FP32_MAX);
		CVec2Dfp32 VMax(-_FP32_MAX);

		const CVec3Dfp32* pV = &pClip->m_Vertices[0];
		int nV = pClip->m_nPlanes;
		bool bErr = (nV == 0);

		for(int v = 0; v < nV; v++)
		{
			fp32 vx = pV[v].k[0];
			fp32 vy = pV[v].k[1];
			fp32 vz = pV[v].k[2];
			fp32 z = pM->k[0][2]*vx + pM->k[1][2]*vy + pM->k[2][2]*vz + pM->k[3][2];
			if (z < 0.1f) 
			{ 
				bErr = true; 
				break;
			}
			fp32 zinv = 1.0f / z;
			fp32 x = (pM->k[0][0]*vx + pM->k[1][0]*vy + pM->k[2][0]*vz + pM->k[3][0]) * zinv;
			VMin.k[0] = Min(VMin.k[0], x);
			VMax.k[0] = Max(VMax.k[0], x);
			fp32 y = (pM->k[0][1]*vx + pM->k[1][1]*vy + pM->k[2][1]*vz + pM->k[3][1]) * zinv;
			VMin.k[1] = Min(VMin.k[1], y);
			VMax.k[1] = Max(VMax.k[1], y);
		}

		CScissorRect Scissor;

		if (bErr)
		{
			Scissor.SetRect(ScrX, ScrY, ScrX + ScrW, ScrY + ScrH);
		}
		else
		{
			int xmin = Max(ScrX, Min(ScrW + ScrX, RoundToInt(VMin[0] * ScaleX + MidX)));
			int ymin = Max(ScrY, Min(ScrH + ScrY, RoundToInt(VMin[1] * ScaleY + MidY)));

			int xmax = Max(ScrX, Min(ScrW + ScrX, RoundToInt(VMax[0] * ScaleX + MidX)));
			int ymax = Max(ScrY, Min(ScrH + ScrY, RoundToInt(VMax[1] * ScaleY + MidY)));

			Scissor.SetRect(xmin, ymin, Max(xmin, xmax), Max(ymin, ymax));
		}
		m_pWorldView->m_lPortalScissor[i] = Scissor;
	}
}

void CXR_Model_BSP2::View_InitLightOcclusion()
{
	MAUTOSTRIP(CXR_Model_BSP_View_InitLightOcclusion, MAUTOSTRIP_VOID);

	CBSP2_SceneGraph* pSG = m_pWorldView->m_pSceneGraph;
	if (!pSG)
		return;

	CBSP2_LightData* pLD = m_spLightData;
	if (!pLD)
		return;

	// Take SceneGraph criticalsection so we're threadsafe
	M_LOCK(pSG->m_Lock);
	int nLights = pSG->m_lLights.Len();
	m_pWorldView->m_lLightOcclusion.SetLen(nLights);

	CXR_LightOcclusionInfo* pLO = m_pWorldView->m_lLightOcclusion.GetBasePtr();
	{
		for(int i = 0; i < nLights; i++)
			pLO[i].Clear();
	}

	{
		const uint16* piPL = m_pWorldView->m_liVisPortalLeaves.GetBasePtr();
		int nPL = m_pWorldView->m_nVisLeaves;

		for(int i = 0; i < nPL; i++)
		{
			int iPL = piPL[i];
			int iRP = m_pWorldView->m_liLeafRPortals[iPL];
			M_ASSERT(iRP, "wtf!");
			
			const CBSP2_Link* pLinks = pSG->m_Lights.GetLinks();
			M_ASSERT(pLinks, "!");

			CScissorRect PLScissor = m_pWorldView->m_lPortalScissor[iRP];

			// Expand the visibility for all lights in this PL.
			int iLink = pSG->m_Lights.GetLink_PL(iPL);
			while(iLink)
			{
				CXR_Light* pL = &pSG->m_lLights[pLinks[iLink].m_ID];
				int iLight = pL->m_iLight;
				if (iLight >= pSG->m_iFirstDynamic && iLight < nLights)
				{
					// ToDo: CULL!!!!

					pLO[iLight].m_ScissorVisible.Expand(PLScissor);
				}

				iLink = pLinks[iLink].m_iLinkNextObject;
			}

			const CBSP2_ShadowVolume* pSV = pLD->m_lSV.GetBasePtr();
//			uint16 iSV = pLD->m_liSVFirstPL[iPL];
			const CRC_ClipVolume* pClip = &m_pWorldView->m_pRPortals[iRP];

			if (iPL >= pLD->m_lnSVPL.Len())
				continue;

			int nSVPL = pLD->m_lnSVPL[iPL];
			int iSVFirstPL = pLD->m_liSVFirstPL[iPL];

			for(int i = iSVFirstPL; i < iSVFirstPL+nSVPL; i++)
			{
				if (pSV[i].m_iPL == iPL)
				{
					const CBSP2_ShadowVolume& SV = pSV[i];

					int p;
					for (p = 0; p < pClip->m_nPlanes; p++)
						if (pClip->m_Planes[p].GetBoxMinDistance(SV.m_BoundBoxLight.m_Min, SV.m_BoundBoxLight.m_Max) > MODEL_BSP_EPSILON) break;
					if (p != pClip->m_nPlanes) 
						continue;

					CScissorRect LightScissor;
					CalcBoxScissor(m_pWorldView, SV.m_BoundBoxLight, LightScissor);

					LightScissor.And(m_pWorldView->m_lPortalScissor[iRP]);
					pLO[SV.m_iLight].m_ScissorVisible.Expand(LightScissor);
				}
			}

/*			while(iSV)
			{

				iSV = pSV[iSV]->m_iSVNextLight;
			}*/
		}
	}


	m_pWorldView->m_lActiveSV.SetLen((pLD->m_lSV.Len() + 7) >> 3);
	FillChar(m_pWorldView->m_lActiveSV.GetBasePtr(), m_pWorldView->m_lActiveSV.Len(), 0);
}

void CXR_Model_BSP2::View_InitPLPriorities(CBSP2_RenderParams* _pRenderParams)
{
	const uint16* piVisPortalLeaves = m_pWorldView->m_liVisPortalLeaves.GetBasePtr();

	CXR_FogState* pSceneFog = _pRenderParams->m_pCurrentEngine->m_pCurrentFogState;
	for(int l = m_pWorldView->m_nVisLeaves-1; l >= 0; l--)
	{
		int iPL = piVisPortalLeaves[l];
		CBSP2_PortalLeafExt* pPL = &m_pPortalLeaves[iPL];

		if (pSceneFog && pSceneFog->NHFEnable())
		{
//			int bSwitch = (m_LastMedium ^ m_pCurMedium->m_MediumFlags) & XW_MEDIUM_FOG;
//			int bSwitch = (m_LastMedium | m_pCurMedium->m_MediumFlags) & XW_MEDIUM_FOG;
			int bSwitch = (_pRenderParams->m_pCurMedium->m_MediumFlags) & XW_MEDIUM_FOG;

			const int MAXFOGPORTALS = 32;
			uint32 iFogPortals[MAXFOGPORTALS];
			int nFogPortals = 0;
			if (bSwitch)
			{
				nFogPortals = Fog_GetVisiblePortals(_pRenderParams, iPL, iFogPortals, MAXFOGPORTALS);
				if (!nFogPortals) bSwitch = false;
			}

			//		if (m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOG)
			if (bSwitch)
			{
				VB_RenderFaceQueues(_pRenderParams);
				VB_Flush(_pRenderParams);
				VB_RenderQueues(_pRenderParams);
				VB_ClearQueues(_pRenderParams);
				VB_ClearFaceQueues(_pRenderParams);
#ifndef PLATFORM_PS2
				_pRenderParams->m_BasePriority_Opaque += CXR_VBPRIORITY_PORTALLEAFOFFSET;
				_pRenderParams->m_BasePriority_Transparent += CXR_VBPRIORITY_PORTALLEAFOFFSET;
#endif
			}
		}

//		bool bHWNHF = (_pRenderParams->m_CurrentRCCaps & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20) && !(_pRenderParams->m_pCurMedium->m_MediumFlags & XW_MEDIUM_CLIPFOG);

		pPL->m_BasePriority = _pRenderParams->m_BasePriority_Opaque;

	}

}

void CXR_Model_BSP2::View_Reset(int _iView)
{
	MAUTOSTRIP(CXR_Model_BSP_View_Reset, MAUTOSTRIP_VOID);
	if (!m_lspViews.ValidPos(_iView))
		Error("View_Reset", "Invalid view index.");
	m_pView = m_lspViews[_iView];
	m_pWorldView = m_lspViews[_iView];
	m_pViewClipData	= &m_lViewData[_iView];

	// Set portals to open by default
	FillChar(m_pWorldView->m_lPortalIDStates.GetBasePtr(), m_pWorldView->m_lPortalIDStates.ListSize(), 1);
}

void CXR_Model_BSP2::View_SetState(int _iView, int _State, int _Value)
{
	MAUTOSTRIP(CXR_Model_BSP_View_SetState, MAUTOSTRIP_VOID);
	if (!m_lspViews.ValidPos(_iView))
		Error("View_SetState", "Invalid view index.");
	m_pView = m_lspViews[_iView];
	m_pWorldView = m_lspViews[_iView];
	m_pViewClipData	= &m_lViewData[_iView];

	if (_State == XR_VIEWCLIPSTATE_PORTAL)
	{
		// 16 LSB = PortalID
		// 16 MSB = Portal state

		int PortalID = _Value & 0xffff;
		int State = _Value >> 16;

		int Mask = aShiftMulTab[PortalID & 7];
		int Pos = PortalID >> 3;
		if (!m_pWorldView->m_lPortalIDStates.ValidPos(Pos)) return;

		if (State)
			m_pWorldView->m_lPortalIDStates[Pos] |= Mask;
		else
			m_pWorldView->m_lPortalIDStates[Pos] &= ~Mask;
	}
}

#define DEFINE_THIS_IF_Z_PORTAL_CULLING_CAN_BE_USED

void CXR_Model_BSP2::View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CXR_ViewClipInterface* _pViewClip,
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_View_Init, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP2::View_Init, XR_BSPMODEL);

	if ((_iView < 0) || (_iView >= m_lspViews.Len()))
		Error("View_Init", "Invalid view index.");
	m_iView = _iView;
	m_pView = m_lspViews[m_iView];
	m_pWorldView = m_lspViews[m_iView];
	m_pViewClipData = &m_lViewData[m_iView];

	CBSP2_View_Params ViewParam;
	ViewParam.m_pViewData = &m_lViewData[m_iView];
	ViewParam.m_pCurrentEngine = _pEngine;
	ViewParam.m_pSceneFog		= _pEngine->m_pCurrentFogState;

#ifdef M_Profile
	m_Time.Reset();
	m_TimeRenderLeafList.Reset();
	m_TimeRenderFaceQueue.Reset();
#endif

	_WMat.Multiply(_VMat, ViewParam.m_pViewData->m_CurVMat);

	// Clear
	{
		int nPL = m_lPortalLeaves.Len();
		m_pWorldView->m_liVisPortalLeaves.SetLen(nPL);
		m_pWorldView->m_MaxVisLeaves = m_pWorldView->m_liVisPortalLeaves.Len();
		m_pWorldView->m_nVisLeaves = 0;
//		m_pWorldView->m_lPVS.SetLen(SceneGraph_PVSGetLen());

		// Initialize space for RPortals.
		m_pWorldView->m_MaxRPortals = MODEL_BSP_NUMRPORTALS;
		m_pWorldView->m_lRPortals.SetLen(m_pWorldView->m_MaxRPortals);
		m_pWorldView->m_pRPortals = &m_pWorldView->m_lRPortals[0];
		m_pWorldView->m_nRPortals = 0;

//		m_CurrentView++;
//		m_pViewData->m_CurrentView = m_CurrentView;

//		m_nPortalFaces = 0;
//		m_liPortalFaces.SetLen(CXR_MAXPORTALFACES);
	}


	CMat4Dfp32 WMatInv;
	_WMat.InverseOrthogonal(WMatInv);
	ViewParam.m_pViewData->m_CurVMat.InverseOrthogonal(ViewParam.m_pViewData->m_CurVMatInv);

	// Get bound-sphere, get the CRC_ClipView
	{
		CRC_Viewport* pVP = _pEngine->GetVBM()->Viewport_Get();
		
		fp32 FogCullOffset = _pEngine->GetVarf(XR_ENGINE_FOGCULLOFFSET);
		m_pWorldView->m_ViewBackPlane = pVP->GetBackPlane();
#ifdef DEFINE_THIS_IF_Z_PORTAL_CULLING_CAN_BE_USED
		if (ViewParam.m_pSceneFog && ViewParam.m_pSceneFog->DepthFogEnable() /*&& m_pSceneFog->m_DepthFogDensity >= 1.0f*/)
			m_pWorldView->m_ViewBackPlane = Min(m_pWorldView->m_ViewBackPlane, ViewParam.m_pSceneFog->m_DepthFogEnd + FogCullOffset);
#endif

		CRC_ClipVolume Clip; 
		if (_pViewClip)
		{
			CRC_ClipVolume Clip2; 
			fp32 BoundR = GetBound_Sphere();				// Local
			if (!_pViewClip->View_GetClip_Sphere(CVec3Dfp32::GetMatrixRow(_WMat, 3), BoundR, 0, 0, &Clip2, NULL)) return;

			Clip.CopyAndTransform(Clip2, _VMat);
			ViewParam.m_pViewData->m_CurClipVolume.CopyAndTransform(Clip2, WMatInv);
		}
		else
		{
			pVP->GetClipVolume(Clip);
			ViewParam.m_pViewData->m_CurClipVolume.CopyAndTransform(Clip, ViewParam.m_pViewData->m_CurVMatInv);
		}
		ViewParam.m_pViewData->m_CurLocalVP = CVec3Dfp32::GetRow(ViewParam.m_pViewData->m_CurVMatInv, 3);

		// Clip i V-space
		// m_CurClipVolume i L-Space

		CVec2Dfp32 VVert2D[16];
		for(int v = 0; v < Clip.m_nPlanes; v++)
		{
			VVert2D[v].k[0] = Clip.m_Vertices[v].k[0] / Clip.m_Vertices[v].k[2];
			VVert2D[v].k[1] = Clip.m_Vertices[v].k[1] / Clip.m_Vertices[v].k[2];
		}
		CVec2Dfp32::GetMinBoundRect(&VVert2D[1], ViewParam.m_PortalOrMin, ViewParam.m_PortalOrMax, Clip.m_nPlanes);

		m_pWorldView->m_BackPlaneInv = 1.0f / m_pWorldView->m_ViewBackPlane;

		// Create back-plane in model-space.
		m_pWorldView->m_LocalBackPlane.n = CVec3Dfp32(0,0,1);
		m_pWorldView->m_LocalBackPlane.d = -m_pWorldView->m_ViewBackPlane;
		m_pWorldView->m_LocalBackPlane.Transform(ViewParam.m_pViewData->m_CurVMatInv);

		// Create front-plane in model-space.
		m_pWorldView->m_LocalFrontPlane.n = CVec3Dfp32(0,0,1);
		m_pWorldView->m_LocalFrontPlane.d = -pVP->GetFrontPlane();
		m_pWorldView->m_LocalFrontPlane.Transform(ViewParam.m_pViewData->m_CurVMatInv);

		// 2D
		CRct ViewRect = pVP->GetViewRect();
		m_pWorldView->m_VPRect = ViewRect;
		m_pWorldView->m_VPWidth = m_pWorldView->m_VPRect.GetWidth();
		m_pWorldView->m_VPHeight = m_pWorldView->m_VPRect.GetHeight();
		m_pWorldView->m_VPScaleVec = CVec4Dfp32(pVP->GetXScale() * 0.5f, pVP->GetYScale() * 0.5f, pVP->GetXScale() * 0.5f, pVP->GetYScale() * 0.5f);
		m_pWorldView->m_VPMidVec[0] = (ViewRect.p0.x + ViewRect.p1.x) >> 1;
		m_pWorldView->m_VPMidVec[1] = (ViewRect.p0.y + ViewRect.p1.y) >> 1;
		m_pWorldView->m_VPMidVec[2] = (ViewRect.p0.x + ViewRect.p1.x) >> 1;
		m_pWorldView->m_VPMidVec[3] = (ViewRect.p0.y + ViewRect.p1.y) >> 1;
		m_pWorldView->m_VPVMat = _VMat;
		m_pWorldView->m_VPRectMinVec = CVec4Dint32(ViewRect.p0.x, ViewRect.p0.y, ViewRect.p0.x, ViewRect.p0.y);
		m_pWorldView->m_VPRectMaxVec = CVec4Dint32(ViewRect.p1.x, ViewRect.p1.y, ViewRect.p1.x, ViewRect.p1.y);
	}
	
	{
		TMeasureResetProfile(m_Time);

		// Initialize view vertex and mask lists.
	//	m_lVVertMask.SetLen(m_lVertices.Len());
	//	m_lVVertices.SetLen(m_lVertices.Len());
	//	FillChar(&m_lVVertMask[0], m_lVertices.Len(), 0);

		if (m_lPortalLeaves.Len())
		{
			// Initialize nodes RPortal look-up table.
			if (m_pWorldView->m_liLeafRPortals.Len() != m_lPortalLeaves.Len())
				m_pWorldView->m_liLeafRPortals.SetLen(m_lPortalLeaves.Len());
			FillChar(&m_pWorldView->m_liLeafRPortals[0], m_pWorldView->m_liLeafRPortals.ListSize(), 0);

			// Initialize space for RPortals.
			m_pWorldView->m_MaxRPortals = MODEL_BSP_NUMRPORTALS;
			m_pWorldView->m_lRPortals.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_liRPortalNext.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_lPortalScissor.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_pRPortals = m_pWorldView->m_lRPortals.GetBasePtr();
			m_pWorldView->m_piRPortalNext = m_pWorldView->m_liRPortalNext.GetBasePtr();
			m_pWorldView->m_nRPortals = 0;
		}
		else
		{
			m_pWorldView->m_MaxRPortals = 2;
			m_pWorldView->m_lRPortals.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_liRPortalNext.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_pRPortals = m_pWorldView->m_lRPortals.GetBasePtr();
			m_pWorldView->m_piRPortalNext = m_pWorldView->m_liRPortalNext.GetBasePtr();
			m_pWorldView->m_nRPortals = 0;
		}

		InitializeListPtrs();

		// --------- Rendering is OK from here ------------------
	#ifdef M_Profile
		m_TimePortalAnd.Reset();
		m_TimePortalOr.Reset();
		m_TimePortal0.Reset();
		m_TimePortal1.Reset();
		m_TimePortal2.Reset();
		m_TimePortal3.Reset();
		m_nPortal0 = 0;
		m_nPortal1 = 0;
		m_nPortal2 = 0;
		m_nPortal3 = 0;
	#endif

		if (m_pPortals)
		{
			int iNodePortal = GetPortalLeaf(ViewParam.m_pViewData->m_CurLocalVP);
			if (iNodePortal == -1)
			{
		//		Error("Render", "No portal leaf!");
				ConOut("No portal leaf!");
	//			_pRender->Attrib_Pop();
				return;
			}

			{
				m_pWorldView->m_nRPortals = 1;
				m_pWorldView->m_pRPortals[m_pWorldView->m_nRPortals] = ViewParam.m_pViewData->m_CurClipVolume;
				m_pWorldView->m_piRPortalNext[m_pWorldView->m_nRPortals] = 0;
				m_pWorldView->m_nRPortals = 2;
				m_pWorldView->m_liLeafRPortals[m_pNodes[iNodePortal].m_iPortalLeaf] = 1;
			}

			//		if (!m_pWorldView->m_lPVS.GetBasePtr()) Error("Render", "No PVS.");
			//		CBSP2_PortalLeafExt* pPL = &m_lPortalLeaves[m_lNodes[iNodePortal].m_iPortalLeaf];
			//		int PVSLen = CBS_Uncompress(&m_lPVS[pPL->m_iPVS], pPL->m_PVSBitLen, m_pWorldView->m_lPVS.GetBasePtr());
			//ConOut(CStrF("PL %d, iPVS %d, PVSLen %d", m_lNodes[iNodePortal].m_iPortalLeaf, pPL->m_iPVS, pPL->m_PVSBitLen));
			//		uint8* pPVS = m_pWorldView->m_lPVS.GetBasePtr();
#ifdef MODEL_BSP_WRITEPORTALTIMING
			g_nPotentialPVSRejects = 0;
#endif

			{
				TMeasureResetProfile(m_TimePortals);
		#ifndef MODEL_BSP_DISABLE_PVS
				m_pWorldView->m_pPVS = SceneGraph_PVSLock(0, m_lNodes[iNodePortal].m_iPortalLeaf);
		#endif
					DisableTree(1);
					EnableTreeFromNode(iNodePortal);
					//JK-NOTE: Assert here because m_pViewClipData is NULL?
					Portal_AddNode(&ViewParam, 1, 1);
					DisableTree(1);
		#ifndef MODEL_BSP_DISABLE_PVS
				SceneGraph_PVSRelease(m_pWorldView->m_pPVS);
		#endif
				m_pWorldView->m_pPVS = NULL;
			}
		}

		CScissorRect* pScissors = m_pWorldView->m_lPortalScissor.GetBasePtr();
		View_CalcScissors(&ViewParam);
		View_InitLightOcclusion();

		// Add plane for vertex-fog clipping
		bool bVtxFog = _pEngine && _pEngine->m_pCurrentFogState->VertexFogEnable();

	//	if (bVtxFog || bDepthFog)
		{
			fp32 FogCullOffset = _pEngine->GetVarf(XR_ENGINE_FOGCULLOFFSET);
			fp32 ViewBackPlane = m_pWorldView->m_ViewBackPlane;
			if (ViewParam.m_pSceneFog && ViewParam.m_pSceneFog->DepthFogEnable() /*&& ViewParam.m_pSceneFog->m_DepthFogDensity >= 1.0f*/)
				ViewBackPlane = Min(m_pWorldView->m_ViewBackPlane, ViewParam.m_pSceneFog->m_DepthFogEnd + FogCullOffset);

			// Create back-plane in model-space.
			CPlane3Dfp32 LocalBackPlane; 
			LocalBackPlane.n = CVec3Dfp32(0,0,1);
			LocalBackPlane.d = -ViewBackPlane;
			LocalBackPlane.Transform(ViewParam.m_pViewData->m_CurVMatInv);

			CXR_FogState* pSceneFog = _pEngine->m_pCurrentFogState;
			for(int i = 1; i < m_pWorldView->m_nRPortals; i++)
			{
				CRC_ClipVolume* pClip = &m_pWorldView->m_pRPortals[i];
				if (bVtxFog)
					if (pClip->m_nPlanes < CRC_CLIPVOLUME_MAXPLANES)
						pClip->m_Planes[pClip->m_nPlanes++] = pSceneFog->m_VtxFog_EndPlane;

				if (pClip->m_nPlanes < CRC_CLIPVOLUME_MAXPLANES)
					pClip->m_Planes[pClip->m_nPlanes++] = LocalBackPlane;
			}
		}

		// Add plane for portal plane clipping
		if (_pEngine && _pEngine->GetVCDepth())
		{
			CPlane3Dfp32 Plane = _pEngine->GetVC()->m_FrontPlaneW;
			for(int i = 1; i < m_pWorldView->m_nRPortals; i++)
			{
				CRC_ClipVolume* pClip = &m_pWorldView->m_pRPortals[i];
				if (pClip->m_nPlanes < CRC_CLIPVOLUME_MAXPLANES)
					pClip->m_Planes[pClip->m_nPlanes++] = Plane;
			}
		}

	#ifdef MODEL_BSP_WRITEPORTALTIMING
		if (m_pPortals && !(ms_Enable & MODEL_BSP_ENABLE_NOPORTALSTATS))
		{
				int iNodePortal = GetPortalLeaf(ViewParam.m_pViewData->m_CurLocalVP);
				int iPL = m_lNodes[iNodePortal].m_iPortalLeaf;
			ConOut(CStrF("iPL %d, iNode %d, nPL %d, nPotPVSCull %d, nRP %d, ", iPL, iNodePortal, m_pWorldView->m_nVisLeaves, g_nPotentialPVSRejects, m_pWorldView->m_nRPortals) + 
				TString("P ", m_TimePortals) + 
				TString(", And", m_TimePortalAnd) + 
				TString(", Or", m_TimePortalOr) +
				TString(", (0 nAnd)", m_TimePortal0) + CStrF(" %d, ", m_nPortal0) +
				TString(", (1)", m_TimePortal1) + CStrF(" %d, ", m_nPortal1) +
				TString(", (2 clpsv)", m_TimePortal2) + CStrF(" %d, ", m_nPortal2) +
				TString(", (3 nOpen)", m_TimePortal3) + CStrF(" %d, ", m_nPortal3)
				);
		}
	#endif
	}
}

void CXR_Model_BSP2::View_Init(int _iView, CXR_Engine* _pEngine, CRenderContext* _pRender, CVec3Dfp32* _pVPortal, int _nVPortal,
	const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _Flags)
{
	MAUTOSTRIP(CXR_Model_BSP_View_Init_2, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP2::View_Init, XR_BSPMODEL);

	if ((_iView < 0) || (_iView >= m_lspViews.Len()))
		Error("View_Init", "Invalid view index.");
	m_iView = _iView;
	m_pView = m_lspViews[m_iView];
	m_pWorldView = m_lspViews[m_iView];
	m_pViewClipData	= &m_lViewData[m_iView];

	CBSP2_View_Params ViewParam;
	ViewParam.m_pViewData = &m_lViewData[m_iView];
	ViewParam.m_pCurrentEngine = _pEngine;
	ViewParam.m_pSceneFog = _pEngine->m_pCurrentFogState;

#ifdef M_Profile
	m_Time.Reset();
	m_TimeRenderLeafList.Reset();
	m_TimeRenderFaceQueue.Reset();
#endif

	_WMat.Multiply(_VMat, ViewParam.m_pViewData->m_CurVMat);

	// Clear
	{
		int nPL = m_lPortalLeaves.Len();
		m_pWorldView->m_liVisPortalLeaves.SetLen(nPL);
		m_pWorldView->m_MaxVisLeaves = m_pWorldView->m_liVisPortalLeaves.Len();
		m_pWorldView->m_nVisLeaves = 0;
//		m_pWorldView->m_lPVS.SetLen(SceneGraph_PVSGetLen());

		// Initialize space for RPortals.
		m_pWorldView->m_MaxRPortals = MODEL_BSP_NUMRPORTALS;
		m_pWorldView->m_lRPortals.SetLen(m_pWorldView->m_MaxRPortals);
		m_pWorldView->m_liRPortalNext.SetLen(m_pWorldView->m_MaxRPortals);
		m_pWorldView->m_pRPortals = m_pWorldView->m_lRPortals.GetBasePtr();
		m_pWorldView->m_piRPortalNext = m_pWorldView->m_liRPortalNext.GetBasePtr();
		m_pWorldView->m_nRPortals = 0;
//		m_liPortalFaces.SetLen(CXR_MAXPORTALFACES);
//		m_nPortalFaces = 0;

//		m_CurrentView++;
//		ViewParam.m_pViewData->m_CurrentView = m_CurrentView;
	}


	CMat4Dfp32 WMatInv;
	_WMat.InverseOrthogonal(WMatInv);
	ViewParam.m_pViewData->m_CurVMat.InverseOrthogonal(ViewParam.m_pViewData->m_CurVMatInv);

	// Get bound-sphere, get the CRC_ClipView
	{
		CRC_Viewport* pVP = _pEngine->GetVBM()->Viewport_Get();
		
		m_pWorldView->m_ViewBackPlane = pVP->GetBackPlane();
#ifdef DEFINE_THIS_IF_Z_PORTAL_CULLING_CAN_BE_USED
		if (ViewParam.m_pSceneFog && ViewParam.m_pSceneFog->DepthFogEnable() /*&& m_pSceneFog->m_DepthFogDensity >= 1.0f*/)
			m_pWorldView->m_ViewBackPlane = Min(m_pWorldView->m_ViewBackPlane, ViewParam.m_pSceneFog->m_DepthFogEnd);
#endif

		ViewParam.m_pViewData->m_CurLocalVP = CVec3Dfp32::GetRow(ViewParam.m_pViewData->m_CurVMatInv, 3);
		CRC_ClipVolume Clip; 
		if (_pVPortal)
		{
			ViewParam.m_pViewData->m_CurClipVolume.GetVertices(_pVPortal, _nVPortal, ViewParam.m_pViewData->m_CurLocalVP, false);
			Clip.CopyAndTransform(ViewParam.m_pViewData->m_CurClipVolume, _VMat);
		}
		else
		{
			pVP->GetClipVolume(Clip);
			ViewParam.m_pViewData->m_CurClipVolume.CopyAndTransform(Clip, ViewParam.m_pViewData->m_CurVMatInv);
		}

		// Clip i V-space
		// m_CurClipVolume i L-Space

		CVec2Dfp32 VVert2D[16];
		for(int v = 0; v < Clip.m_nPlanes; v++)
		{
			VVert2D[v].k[0] = Clip.m_Vertices[v].k[0] / Clip.m_Vertices[v].k[2];
			VVert2D[v].k[1] = Clip.m_Vertices[v].k[1] / Clip.m_Vertices[v].k[2];
		}
		CVec2Dfp32::GetMinBoundRect(&VVert2D[1], ViewParam.m_PortalOrMin, ViewParam.m_PortalOrMax, Clip.m_nPlanes);

		m_pWorldView->m_BackPlaneInv = 1.0f / m_pWorldView->m_ViewBackPlane;

		// Create back-plane in model-space.
		m_pWorldView->m_LocalBackPlane.n = CVec3Dfp32(0,0,1);
		m_pWorldView->m_LocalBackPlane.d = -m_pWorldView->m_ViewBackPlane;
		m_pWorldView->m_LocalBackPlane.Transform(ViewParam.m_pViewData->m_CurVMatInv);

		// Create front-plane in model-space.
		m_pWorldView->m_LocalFrontPlane.n = CVec3Dfp32(0,0,1);
		m_pWorldView->m_LocalFrontPlane.d = -pVP->GetFrontPlane();
		m_pWorldView->m_LocalFrontPlane.Transform(ViewParam.m_pViewData->m_CurVMatInv);

		// 2D
		CRct ViewRect = pVP->GetViewRect();
		m_pWorldView->m_VPRect = ViewRect;
		m_pWorldView->m_VPWidth = m_pWorldView->m_VPRect.GetWidth();
		m_pWorldView->m_VPHeight = m_pWorldView->m_VPRect.GetHeight();
		m_pWorldView->m_VPScaleVec = CVec4Dfp32(pVP->GetXScale() * 0.5f, pVP->GetYScale() * 0.5f, pVP->GetXScale() * 0.5f, pVP->GetYScale() * 0.5f);
		m_pWorldView->m_VPMidVec[0] = (ViewRect.p0.x + ViewRect.p1.x) >> 1;
		m_pWorldView->m_VPMidVec[1] = (ViewRect.p0.y + ViewRect.p1.y) >> 1;
		m_pWorldView->m_VPMidVec[2] = (ViewRect.p0.x + ViewRect.p1.x) >> 1;
		m_pWorldView->m_VPMidVec[3] = (ViewRect.p0.y + ViewRect.p1.y) >> 1;
		m_pWorldView->m_VPVMat = _VMat;
		m_pWorldView->m_VPRectMinVec = CVec4Dint32(ViewRect.p0.x, ViewRect.p0.y, ViewRect.p0.x, ViewRect.p0.y);
		m_pWorldView->m_VPRectMaxVec = CVec4Dint32(ViewRect.p1.x, ViewRect.p1.y, ViewRect.p1.x, ViewRect.p1.y);
	}

	// Get bound-sphere, get the CRC_ClipView
/*	{
		CRC_Viewport* pVP = _pRender->Viewport_Get();
		m_pWorldView->m_ViewBackPlane = pVP->GetBackPlane();
		ViewParam.m_pViewData->m_CurLocalVP = CVec3Dfp32::GetRow(ViewParam.m_pViewData->m_CurVMatInv, 3);

		CRC_ClipVolume Clip; 
		if (_pVPortal)
		{
			ViewParam.m_pViewData->m_CurClipVolume.GetVertices(_pVPortal, _nVPortal, ViewParam.m_pViewData->m_CurLocalVP, false);
			Clip.CopyAndTransform(ViewParam.m_pViewData->m_CurClipVolume, _VMat);
		}
		else
		{
			pVP->GetClipVolume(Clip);
			ViewParam.m_pViewData->m_CurClipVolume.CopyAndTransform(Clip, ViewParam.m_pViewData->m_CurVMatInv);
		}

		// Clip i V-space
		// m_CurClipVolume i L-Space

		CVec2Dfp32 VVert2D[16];
		for(int v = 0; v < Clip.m_nPlanes; v++)
		{
			VVert2D[v].k[0] = Clip.m_Vertices[v].k[0] / Clip.m_Vertices[v].k[2];
			VVert2D[v].k[1] = Clip.m_Vertices[v].k[1] / Clip.m_Vertices[v].k[2];
		}
		CVec2Dfp32::GetMinBoundRect(&VVert2D[1], m_PortalOrMin, m_PortalOrMax, Clip.m_nPlanes);

		// Create back-plane in model-space.
		m_pWorldView->m_LocalBackPlane.n = CVec3Dfp32(0,0,1);
		m_pWorldView->m_LocalBackPlane.d = -pVP->GetBackPlane();
		m_pWorldView->m_LocalBackPlane.Transform(ViewParam.m_pViewData->m_CurVMatInv);

		// Create front-plane in model-space.
		m_pWorldView->m_LocalFrontPlane.n = CVec3Dfp32(0,0,1);
		m_pWorldView->m_LocalFrontPlane.d = -pVP->GetFrontPlane();
		m_pWorldView->m_LocalFrontPlane.Transform(ViewParam.m_pViewData->m_CurVMatInv);

		ViewParam.m_pViewData->m_CurClipVolume.CreateFromVertices3(m_pWorldView->m_LocalBackPlane);
	}*/

	// Get clip mask
	{
		fp32 BoundR = GetBound_Sphere();
		_pEngine->GetVBM()->Viewport_Get()->SphereInView(CVec3Dfp32::GetMatrixRow(ViewParam.m_pViewData->m_CurVMat, 3), BoundR);

		// FIXME:
		// If *this doesn't have a pViewClip, it's probably the world. If it's not rendered
		// there will be bugs when objects try to use it's view-interface. 
		// This prevents it from exiting.
		//if (!_pViewClip) 
		//	if (!ViewMask) return;
	}
	
	{
		TMeasureProfile(m_Time);

		// Initialize view vertex and mask lists.
	//	m_lVVertMask.SetLen(m_lVertices.Len());
	//	m_lVVertices.SetLen(m_lVertices.Len());
	//	FillChar(&m_lVVertMask[0], m_lVertices.Len(), 0);

		if (m_lPortalLeaves.Len())
		{
			// Initialize nodes RPortal look-up table.
			if (m_pWorldView->m_liLeafRPortals.Len() != m_lPortalLeaves.Len())
			{
				m_pWorldView->m_liLeafRPortals.SetLen(m_lPortalLeaves.Len());
	//			m_pWorldView->m_lnLeafRPortals.SetLen(m_lPortalLeaves.Len());
			}
			FillChar(&m_pWorldView->m_liLeafRPortals[0], m_pWorldView->m_liLeafRPortals.ListSize(), 0);
	//		FillChar(&m_pWorldView->m_lnLeafRPortals[0], m_pWorldView->m_lnLeafRPortals.ListSize(), 0);

			// Initialize space for RPortals.
			m_pWorldView->m_MaxRPortals = MODEL_BSP_NUMRPORTALS;
			m_pWorldView->m_lRPortals.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_liRPortalNext.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_lPortalScissor.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_pRPortals = m_pWorldView->m_lRPortals.GetBasePtr();
			m_pWorldView->m_piRPortalNext = m_pWorldView->m_liRPortalNext.GetBasePtr();
			m_pWorldView->m_nRPortals = 0;
		}
		else
		{
			m_pWorldView->m_MaxRPortals = 2;
			m_pWorldView->m_lRPortals.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_liRPortalNext.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_pRPortals = m_pWorldView->m_lRPortals.GetBasePtr();
			m_pWorldView->m_piRPortalNext = m_pWorldView->m_liRPortalNext.GetBasePtr();
			m_pWorldView->m_nRPortals = 0;
		}

		InitializeListPtrs();

		// --------- Rendering is OK from here ------------------
	#ifdef M_Profile
		m_TimePortalAnd.Reset();
		m_TimePortalOr.Reset();
		m_TimePortal0.Reset();
		m_TimePortal1.Reset();
		m_TimePortal2.Reset();
		m_TimePortal3.Reset();
		m_nPortal0 = 0;
		m_nPortal1 = 0;
		m_nPortal2 = 0;
		m_nPortal3 = 0;
	#endif

		if (m_pPortals)
		{
	//		int iNodePortal = GetPortalLeaf(ViewParam.m_pViewData->m_CurLocalVP, );
			CVec3Dfp32 Mid(0);
			for(int v = 0; v < _nVPortal; v++)
				Mid += _pVPortal[v];
			Mid *= 1.0f/fp32(_nVPortal);
	//		ConOut(Mid.GetString());
	//		ConOut(CStrF("%d, ", _nVPortal) + _pVPortal[0].GetString());


			{
				TMeasureResetProfile(m_TimePortals);
				DisableTree(1);
				m_pWorldView->m_nRPortals = 1;

				GetPortalLeaf(Mid);
				m_pWorldView->m_nRPortals = 1;


				m_pWorldView->m_pPVS = NULL;
				Portal_Open_r(&ViewParam, 1, _pVPortal, _nVPortal);

				if (m_pWorldView->m_nRPortals > 1)
				{
					Portal_AddNode(&ViewParam, 1, 1);
				}
				SceneGraph_PVSRelease(m_pWorldView->m_pPVS);
		//		ConOut(CStrF("nRPortals %d", m_pWorldView->m_nRPortals));
				DisableTree(1);

				View_CalcScissors(&ViewParam);
				View_InitLightOcclusion();

				// Add plane for vertex-fog clipping
				if (_pEngine && _pEngine->m_pCurrentFogState->VertexFogEnable())
				{
					CXR_FogState* pSceneFog = _pEngine->m_pCurrentFogState;
					for(int i = 1; i < m_pWorldView->m_nRPortals; i++)
					{
						CRC_ClipVolume* pClip = &m_pWorldView->m_pRPortals[i];
						if (pClip->m_nPlanes < CRC_CLIPVOLUME_MAXPLANES)
							pClip->m_Planes[pClip->m_nPlanes++] = pSceneFog->m_VtxFog_EndPlane;
					}
				}

				// Add plane for portal plane clipping
				if (_pEngine && _pEngine->GetVCDepth())
				{
					CPlane3Dfp32 Plane = _pEngine->GetVC()->m_FrontPlaneW;
					for(int i = 1; i < m_pWorldView->m_nRPortals; i++)
					{
						CRC_ClipVolume* pClip = &m_pWorldView->m_pRPortals[i];
						if (pClip->m_nPlanes < CRC_CLIPVOLUME_MAXPLANES)
							pClip->m_Planes[pClip->m_nPlanes++] = Plane;
					}
				}
			}
		}
	}
}

void CXR_Model_BSP2::View_InitForLight(int _iView, const CVec3Dfp32& _Pos, CVec3Dfp32* _pVPortal, int _nVPortal, const CXR_Light* _pLight)
{
	MAUTOSTRIP(CXR_Model_BSP_View_Init_2, MAUTOSTRIP_VOID);
	MSCOPE(CXR_Model_BSP2::View_Init, XR_BSPMODEL);

	if ((_iView < 0) || (_iView >= m_lspViews.Len()))
		Error("View_Init", "Invalid view index.");

	CBSP2_ViewInstance* pBaseView = m_lspViews[m_iView];

	CBSP2_View_Params ViewParam;
	ViewParam.m_pViewData = &m_lViewData[m_iView];

	m_iView = _iView;
	m_pView = m_lspViews[m_iView];
	m_pWorldView = m_lspViews[m_iView];
	m_pViewClipData	= &m_lViewData[m_iView];

#ifdef M_Profile
	m_Time.Reset();
	m_TimeRenderLeafList.Reset();
	m_TimeRenderFaceQueue.Reset();
#endif

//	_WMat.Multiply(_VMat, ViewParam.m_pViewData->m_CurVMat);

	// Clear
	{
		int nPL = m_lPortalLeaves.Len();
		m_pWorldView->m_liVisPortalLeaves.SetLen(nPL);
		m_pWorldView->m_MaxVisLeaves = m_pWorldView->m_liVisPortalLeaves.Len();
		m_pWorldView->m_nVisLeaves = 0;
//		m_pWorldView->m_lPVS.SetLen(SceneGraph_PVSGetLen());

		// Initialize space for RPortals.
		m_pWorldView->m_MaxRPortals = MODEL_BSP_NUMRPORTALS;
		m_pWorldView->m_lRPortals.SetLen(m_pWorldView->m_MaxRPortals);
		m_pWorldView->m_liRPortalNext.SetLen(m_pWorldView->m_MaxRPortals);
		m_pWorldView->m_pRPortals = m_pWorldView->m_lRPortals.GetBasePtr();
		m_pWorldView->m_piRPortalNext = m_pWorldView->m_liRPortalNext.GetBasePtr();
		m_pWorldView->m_nRPortals = 0;
//		m_liPortalFaces.SetLen(CXR_MAXPORTALFACES);
//		m_nPortalFaces = 0;

//		m_CurrentView++;
//		ViewParam.m_pViewData->m_CurrentView = m_CurrentView;
	}


//	CMat4Dfp32 WMatInv;
//	_WMat.InverseOrthogonal(WMatInv);

	CMat4Dfp32 VMatInv;
	CVec3Dfp32::GetRow(_pLight->m_Pos, 0).SetRow(VMatInv, 2);
	CVec3Dfp32::GetRow(_pLight->m_Pos, 1).SetRow(VMatInv, 0);
	CVec3Dfp32::GetRow(_pLight->m_Pos, 2).SetRow(VMatInv, 1);
	CVec3Dfp32::GetRow(_pLight->m_Pos, 3).SetRow(VMatInv, 3);

	ViewParam.m_pViewData->m_CurVMatInv = VMatInv;
	ViewParam.m_pViewData->m_CurVMatInv.InverseOrthogonal(ViewParam.m_pViewData->m_CurVMat);

//	ViewParam.m_pViewData->m_CurVMat.InverseOrthogonal(ViewParam.m_pViewData->m_CurVMatInv);

	// Get bound-sphere, get the CRC_ClipView
	{
		m_pWorldView->m_ViewBackPlane = _pLight->m_Range;

		ViewParam.m_pViewData->m_CurLocalVP = _Pos;

		ViewParam.m_pViewData->m_CurClipVolume.GetVertices(_pVPortal, _nVPortal, ViewParam.m_pViewData->m_CurLocalVP, false);

		CRC_ClipVolume Clip; 
		Clip.CopyAndTransform(ViewParam.m_pViewData->m_CurClipVolume, ViewParam.m_pViewData->m_CurVMat);

		// Clip i V-space
		// m_CurClipVolume i L-Space

		CVec2Dfp32 VVert2D[16];
		for(int v = 0; v < Clip.m_nPlanes; v++)
		{
			VVert2D[v].k[0] = Clip.m_Vertices[v].k[0] / Clip.m_Vertices[v].k[2];
			VVert2D[v].k[1] = Clip.m_Vertices[v].k[1] / Clip.m_Vertices[v].k[2];
		}
		CVec2Dfp32::GetMinBoundRect(&VVert2D[1], ViewParam.m_PortalOrMin, ViewParam.m_PortalOrMax, Clip.m_nPlanes);

		m_pWorldView->m_BackPlaneInv = 1.0f / m_pWorldView->m_ViewBackPlane;

		// Create back-plane in model-space.
		m_pWorldView->m_LocalBackPlane.n = CVec3Dfp32(0,0,1);
		m_pWorldView->m_LocalBackPlane.d = -m_pWorldView->m_ViewBackPlane;
		m_pWorldView->m_LocalBackPlane.Transform(ViewParam.m_pViewData->m_CurVMatInv);

		// Create front-plane in model-space.
		m_pWorldView->m_LocalFrontPlane.n = CVec3Dfp32(0,0,1);
		m_pWorldView->m_LocalFrontPlane.d = 0;
		m_pWorldView->m_LocalFrontPlane.Transform(ViewParam.m_pViewData->m_CurVMatInv);

		ViewParam.m_pViewData->m_CurClipVolume.CreateFromVertices3(m_pWorldView->m_LocalBackPlane);

		// 2D
/*		CRct ViewRect = pVP->GetViewRect();
		m_pWorldView->m_VPRect = ViewRect;
		m_pWorldView->m_VPWidth = m_pWorldView->m_VPRect.GetWidth();
		m_pWorldView->m_VPHeight = m_pWorldView->m_VPRect.GetHeight();
		m_pWorldView->m_VPScale = CVec2Dfp32(pVP->GetXScale() * 0.5f, pVP->GetYScale() * 0.5f);
		m_pWorldView->m_VPMid[0] = (ViewRect.p0.x + ViewRect.p1.x) >> 1;
		m_pWorldView->m_VPMid[1] = (ViewRect.p0.y + ViewRect.p1.y) >> 1;
		m_pWorldView->m_VPVMat = _VMat;*/
	}

	{
		TMeasureResetProfile(m_Time);

		{
			// Initialize nodes RPortal look-up table.
			if (m_pWorldView->m_liLeafRPortals.Len() != m_lPortalLeaves.Len())
			{
				m_pWorldView->m_liLeafRPortals.SetLen(m_lPortalLeaves.Len());
	//			m_pWorldView->m_lnLeafRPortals.SetLen(m_lPortalLeaves.Len());
			}
			FillChar(&m_pWorldView->m_liLeafRPortals[0], m_pWorldView->m_liLeafRPortals.ListSize(), 0);
	//		FillChar(&m_pWorldView->m_lnLeafRPortals[0], m_pWorldView->m_lnLeafRPortals.ListSize(), 0);

			// Initialize space for RPortals.
			m_pWorldView->m_MaxRPortals = MODEL_BSP_NUMRPORTALS;
			m_pWorldView->m_lRPortals.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_liRPortalNext.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_lPortalScissor.SetLen(m_pWorldView->m_MaxRPortals);
			m_pWorldView->m_pRPortals = m_pWorldView->m_lRPortals.GetBasePtr();
			m_pWorldView->m_piRPortalNext = m_pWorldView->m_liRPortalNext.GetBasePtr();
			m_pWorldView->m_nRPortals = 0;

			m_pWorldView->m_lPortalIDStates = pBaseView->m_lPortalIDStates;
		}

		InitializeListPtrs();

		// --------- Rendering is OK from here ------------------
	#ifdef M_Profile
		m_TimePortalAnd.Reset();
		m_TimePortalOr.Reset();
		m_TimePortal0.Reset();
		m_TimePortal1.Reset();
		m_TimePortal2.Reset();
		m_TimePortal3.Reset();
		m_nPortal0 = 0;
		m_nPortal1 = 0;
		m_nPortal2 = 0;
		m_nPortal3 = 0;
	#endif

		if (m_pPortals)
		{
			int iNodePortal = GetPortalLeaf(ViewParam.m_pViewData->m_CurLocalVP);
			if (iNodePortal == -1)
			{
		//		Error("Render", "No portal leaf!");
				ConOut("No portal leaf!");
	//			_pRender->Attrib_Pop();
				return;
			}

			{
				m_pWorldView->m_nRPortals = 1;
				m_pWorldView->m_pRPortals[m_pWorldView->m_nRPortals] = ViewParam.m_pViewData->m_CurClipVolume;
				m_pWorldView->m_piRPortalNext[m_pWorldView->m_nRPortals] = 0;
				m_pWorldView->m_nRPortals = 2;
				m_pWorldView->m_liLeafRPortals[m_pNodes[iNodePortal].m_iPortalLeaf] = 1;
			}

		#ifdef MODEL_BSP_WRITEPORTALTIMING
			g_nPotentialPVSRejects = 0;
		#endif
			{
				TMeasureResetProfile(m_TimePortals);
		#ifndef MODEL_BSP_DISABLE_PVS
				m_pWorldView->m_pPVS = SceneGraph_PVSLock(0, m_lNodes[iNodePortal].m_iPortalLeaf);
		#endif

				DisableTree(1);
				EnableTreeFromNode(iNodePortal);
				// Boom?
				Portal_AddNode(&ViewParam, 1, 1);
				DisableTree(1);

		#ifndef MODEL_BSP_DISABLE_PVS
				SceneGraph_PVSRelease(m_pWorldView->m_pPVS);
		#endif
				m_pWorldView->m_pPVS = NULL;
			}

		}

	}
}


void CXR_Model_BSP2::View_SetCurrent(int _iView, CXR_SceneGraphInstance* _pSceneGraphInstance)
{
	MAUTOSTRIP(CXR_Model_BSP_View_SetCurrent, MAUTOSTRIP_VOID);
	m_iView = _iView;
	m_pView = m_lspViews[m_iView];
	m_pWorldView = m_lspViews[m_iView];
	m_pViewClipData	= &m_lViewData[m_iView];
	m_pWorldView->m_pSceneGraph = safe_cast<CBSP2_SceneGraph>(_pSceneGraphInstance);
}

bool CXR_Model_BSP2::View_GetClip_Sphere_r(int _iNode, CBSP2_View_Params* _pViewParams) const
{
	MAUTOSTRIP(CXR_Model_BSP_View_GetClip_Sphere_r, false);
	if (!_iNode) return false;
	const CBSP2_Node* pN = &m_pNodes[_iNode];
	if (pN->m_Flags & XW_NODE_PORTAL)
	{
		int iPL = pN->m_iPortalLeaf;
		int iRP = m_pView->m_liLeafRPortals[iPL];
		if (iRP != 0)
		{
			if (m_pView->m_pRPortals[iRP].m_nPlanes < 3) return false;
			const CRC_ClipVolume* pClip = &m_pView->m_pRPortals[iRP];

			int p;
			for (p = 0; p < pClip->m_nPlanes; p++)
				if (pClip->m_Planes[p].Distance(_pViewParams->m_v0) > _pViewParams->m_Radius) break;
			if (p != pClip->m_nPlanes) return false;

			if (_pViewParams->m_pClipVolume)
			{
				Portal_Or(_pViewParams->m_pClipVolume, pClip, _pViewParams);
			}

			const CBSP2_PortalLeafExt* pPL = &m_lPortalLeaves[iPL];

			if (_pViewParams->m_pRenderInfo)
				_pViewParams->m_pRenderInfo->m_MediumFlags |= m_lMediums[pPL->m_iMedium].m_MediumFlags;

			if (_pViewParams->m_pRenderInfo)
//				if (iRP < m_iClosestRPortal)
				if (pPL->m_BasePriority > _pViewParams->m_HighestPrior)
			{
				_pViewParams->m_HighestPrior = pPL->m_BasePriority;

				_pViewParams->m_iClosestRPortal = iRP;

				// Set light-volume
//				if (pPL->m_iLightVolume)
//					m_pRenderInfo->m_pLightVolume = &m_lLightVolumeInfo[pPL->m_iLightVolume];

				// NHF?
				_pViewParams->m_pRenderInfo->m_Flags = 0;
//				if (ms_Enable & MODEL_BSP_ENABLE_FOG)
				if (_pViewParams->m_pSceneFog && _pViewParams->m_pSceneFog->NHFEnable())
					if (m_lMediums[pPL->m_iMedium].m_MediumFlags & XW_MEDIUM_FOG)
						_pViewParams->m_pRenderInfo->m_Flags |= CXR_RENDERINFO_NHF;

				if (_pViewParams->m_pSceneFog && _pViewParams->m_pSceneFog->DepthFogEnable())
					_pViewParams->m_pRenderInfo->m_Flags |= CXR_RENDERINFO_DEPTHFOG;

				if (_pViewParams->m_pSceneFog && _pViewParams->m_pSceneFog->VertexFogEnable())
					_pViewParams->m_pRenderInfo->m_Flags |= CXR_RENDERINFO_VERTEXFOG;

				// Set rendering proirities.
				_pViewParams->m_pRenderInfo->m_BasePriority_Opaque = pPL->m_BasePriority + CXR_VBPRIORITY_MODEL_OPAQUE;
				_pViewParams->m_pRenderInfo->m_BasePriority_Transparent = pPL->m_BasePriority + CXR_VBPRIORITY_MODEL_TRANSPARENT;
				fp32 Dist = m_pView->m_LocalFrontPlane.Distance(_pViewParams->m_v0);
				fp32 dPrior = 1.0f - Dist*m_pView->m_BackPlaneInv;
				_pViewParams->m_pRenderInfo->m_BasePriority_Transparent += dPrior;
			}

			return true;
		}
		return false;
	}
	else if (pN->IsNode())
	{
		fp32 Dist = m_pPlanes[pN->m_iPlane].Distance(_pViewParams->m_v0);
		bool bVis = false;
		if (Dist > -_pViewParams->m_Radius)
			bVis = View_GetClip_Sphere_r(pN->m_iNodeFront, _pViewParams);

		if (Dist < _pViewParams->m_Radius) 
			if (_pViewParams->m_pRenderInfo || _pViewParams->m_pClipVolume || !bVis)
				bVis |= View_GetClip_Sphere_r(pN->m_iNodeBack, _pViewParams);

		return bVis;
	}
	return false;
}

bool CXR_Model_BSP2::View_GetClip_Sphere(CVec3Dfp32 _v0, fp32 _Radius, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_View_GetClip_Sphere, false);

	CBSP2_View_Params Params;
	Params.SetupForSphere(_pRenderInfo?_pRenderInfo->m_pCurrentEngine->m_pCurrentFogState:NULL, _v0, _Radius, _pClipVolume, _pRenderInfo, _pRenderInfo?_pRenderInfo->m_pCurrentEngine:NULL);

	if (_pRenderInfo) _pRenderInfo->Clear(_pRenderInfo->m_pCurrentEngine);

	if (_pClipVolume)
	{
		_pClipVolume->Init(m_pViewClipData->m_CurLocalVP);
		View_GetClip_Sphere_r(1, &Params);
/*		if (_pRenderInfo && _pClipVolume->m_nPlanes >= 3)
		{
			int iPL = GetStructurePortalLeaf(_v0);
			if (iPL > 0)
			{
				const CBSP2_PortalLeaf* pPL = &m_lPortalLeaves[iPL];
				if (pPL->m_iLightVolume)
					_pVisRenderInfo->m_pLightVolume = &m_lLightVolumeInfo[pPL->m_iLightVolume];
			}
		}*/
		return (_pClipVolume->m_nPlanes >= 3);
	}
	else
	{
		if (View_GetClip_Sphere_r(1, &Params))
		{
/*			if (_pRenderInfo)
			{
				int iPL = GetStructurePortalLeaf(_v0);
				if (iPL > 0)
				{
					const CBSP2_PortalLeaf* pPL = &m_lPortalLeaves[iPL];
					if (pPL->m_iLightVolume)
						_pVisRenderInfo->m_pLightVolume = &m_lLightVolumeInfo[pPL->m_iLightVolume];
				}
			}*/
			return true;
		}
		else
			return false;
	}
}


static void AddLightToRenderInfo(CXR_RenderInfo* _pRenderInfo, CXR_Light* _pLight, const CScissorRect& _Scissor)
{
	int nL = _pRenderInfo->m_nLights;
	CXR_LightInfo* pLI = _pRenderInfo->m_pLightInfo;

	// Check if light is already added
	for(int i = 0; i < nL; i++)
		if (pLI[i].m_pLight->m_iLight == _pLight->m_iLight)
		{
			// Found the light, expand the scissor with the new scissor
			pLI[i].m_Scissor.Expand(_Scissor);
			return;
		}

	// Add the light since it was not found
	if (nL < _pRenderInfo->m_MaxLights)
	{
		pLI[nL].m_Scissor = _Scissor;
		pLI[nL].m_pLight = _pLight;
		_pRenderInfo->m_nLights++;
	}
}

static int FindSV(const CBSP2_LightData* _pLD, int _iPL, int _iLight)
{
	const CBSP2_ShadowVolume* pSV = _pLD->m_lSV.GetBasePtr();

	if (_iPL >= _pLD->m_lnSVPL.Len())
		return 0;

	int iFirstSV = _pLD->m_liSVFirstPL[_iPL];
	int nSV = _pLD->m_lnSVPL[_iPL];

	for(int i = 0; i < nSV; i++)
		if (pSV[iFirstSV + i].m_iLight == _iLight)
			return iFirstSV + i;

	return 0;
}

bool CXR_Model_BSP2::View_GetClip_Box_StructureLeaf(int _iNode, CBSP2_View_Params* _pViewParams) const
{
	MAUTOSTRIP(CXR_Model_BSP_View_GetClip_Box_StructureLeaf, false);
	const CBSP2_Node* pN = &m_pNodes[_iNode];

	CBSP2_View_Params* pViewParams = _pViewParams;

	bool bRes = false;
	uint iPL = pN->m_iPortalLeaf;
	uint iRP = m_pWorldView->m_liLeafRPortals[iPL];
	if (iRP != 0)
	{
		CRC_ClipVolume* pClip = &m_pWorldView->m_pRPortals[iRP];
		uint nPlanes = pClip->m_nPlanes;
		if (nPlanes < 3) return false;

		CVec3Dfp32 VBoxMin = pViewParams->m_VisVBox.m_Min;
		CVec3Dfp32 VBoxMax = pViewParams->m_VisVBox.m_Max;

		fp32 Dist0 = pClip->m_Planes[0].GetBoxMinDistance(VBoxMin, VBoxMax);
		fp32 Dist1 = pClip->m_Planes[1].GetBoxMinDistance(VBoxMin, VBoxMax);
		fp32 Dist2 = pClip->m_Planes[2].GetBoxMinDistance(VBoxMin, VBoxMax);

		if(Max(Dist2, Max(Dist1, Dist0)) <= MODEL_BSP_EPSILON)
		{
			uint p;
			for (p = 3; p < nPlanes; p++)
				if (pClip->m_Planes[p].GetBoxMinDistance(VBoxMin, VBoxMax) > MODEL_BSP_EPSILON) break;
			if (p == nPlanes) 
				bRes = true;
		}

//	if (_pClipVolume->m_nPlanes) ConOut("Vis-OR");
		if (pViewParams->m_pClipVolume)
			Portal_Or(pViewParams->m_pClipVolume, pClip, pViewParams);

		const CBSP2_PortalLeafExt* pPL = &m_lPortalLeaves[iPL];

		// RenderInfo?
		if (pViewParams->m_pRenderInfo)
		{
			const CXR_MediumDesc* pMedium = &m_lMediums[pPL->m_iMedium];
			pViewParams->m_pRenderInfo->m_MediumFlags |= pMedium->m_MediumFlags;

			// NHF?
//				if (ms_Enable & MODEL_BSP_ENABLE_FOG)
			if(pViewParams->m_pSceneFog)
			{
				if (pViewParams->m_pSceneFog->NHFEnable())
					if (pMedium->m_MediumFlags & XW_MEDIUM_FOG)
						pViewParams->m_pRenderInfo->m_Flags |= CXR_RENDERINFO_NHF;

				if (pViewParams->m_pSceneFog->DepthFogEnable())
					pViewParams->m_pRenderInfo->m_Flags |= CXR_RENDERINFO_DEPTHFOG;

				if (pViewParams->m_pSceneFog->VertexFogEnable())
					pViewParams->m_pRenderInfo->m_Flags |= CXR_RENDERINFO_VERTEXFOG;
			}

			if (pPL->m_BasePriority > pViewParams->m_HighestPrior)
//				if (iRP < m_iClosestRPortal)
			{
				pViewParams->m_HighestPrior = pPL->m_BasePriority;
				pViewParams->m_iClosestRPortal = iRP;

				// Set light-volume
//					if (pPL->m_iLightVolume)
//						pViewParams->m_pRenderInfo->m_pLightVolume = &m_lLightVolumeInfo[pPL->m_iLightVolume];

				// Set rendering priorities.
				pViewParams->m_pRenderInfo->m_BasePriority_Opaque = pPL->m_BasePriority + CXR_VBPRIORITY_MODEL_OPAQUE;
				pViewParams->m_pRenderInfo->m_BasePriority_Transparent = pPL->m_BasePriority + CXR_VBPRIORITY_MODEL_TRANSPARENT;
				CVec3Dfp32 Pos;
				VBoxMax.Lerp(VBoxMin, 0.5f, Pos);
				fp32 Dist = m_pWorldView->m_LocalFrontPlane.Distance(Pos);
				fp32 dPrior = 1.0f - Dist*m_pWorldView->m_BackPlaneInv;
				dPrior = Max(-0.25f, Min(dPrior, 1.25f));
/*				if (dPrior < -0.5f)
			{
				ConOutL(CStrF("dPrior %f, Dist %f, %f, %s - %s", dPrior, Dist, Dist*m_pWorldView->m_BackPlaneInv, m_VisVBox.m_Min.GetString().Str(), m_VisVBox.m_Max.GetString().Str()));
			}*/
				pViewParams->m_pRenderInfo->m_BasePriority_Transparent += dPrior;
			}
		}

		if (pViewParams->m_pRenderInfo)
		{
			// Return lights?
			CBSP2_SceneGraph* pSG = m_pWorldView->m_pSceneGraph;
			if (pViewParams->m_pRenderInfo->m_MaxLights && pViewParams->m_pRenderInfo->m_pLightInfo && pSG)
			{
				M_LOCK(pSG->m_Lock);
				const CBSP2_Link* pLinks = pSG->m_Lights.GetLinks();
				M_ASSERT(pLinks, "!");

				const CPlane3Dfp32* pPlanes = m_spLightData->m_lPlanes.GetBasePtr();

				int iLink = pSG->m_Lights.GetLink_PL(iPL);
				for(; iLink; iLink = pLinks[iLink].m_iLinkNextObject)
				{
					CXR_Light* pL = &pSG->m_lLights[pLinks[iLink].m_ID];
					if (pL->GetIntensitySqrf() > 0.0001f &&
						pL->m_BoundBox.m_Min[0] < VBoxMax[0] &&
						pL->m_BoundBox.m_Min[1] < VBoxMax[1] &&
						pL->m_BoundBox.m_Min[2] < VBoxMax[2] &&
						pL->m_BoundBox.m_Max[0] > VBoxMin[0] &&
						pL->m_BoundBox.m_Max[1] > VBoxMin[1] &&
						pL->m_BoundBox.m_Max[2] > VBoxMin[2])
					{
						int iSV = FindSV(m_spLightData, iPL, pL->m_iLight);
						if (iSV)
						{
							const CBSP2_ShadowVolume& SV = m_spLightData->m_lSV[iSV];
							if (SV.m_nPlaneLightBound)
							{
								int p = 0;
								for(; p < SV.m_nPlaneLightBound; p++)
								{
									if (pPlanes[SV.m_iPlaneLightBound + p].GetBoxMinDistance(VBoxMin, VBoxMax) > -MODEL_BSP_EPSILON)
										break;
								}

								if (p != SV.m_nPlaneLightBound)
									continue;
							}
							else
							{
								if (!pViewParams->m_VisVBox.IsInside(SV.m_BoundBoxLight))
									continue;
							}

							m_pWorldView->m_lActiveSV[iSV >> 3] |= aShiftMulTab[iSV & 7];
							AddLightToRenderInfo(pViewParams->m_pRenderInfo, pL, m_pWorldView->m_lPortalScissor[iRP]);

#ifndef PLATFORM_CONSOLE
							if (pViewParams->m_pCurrentEngine && (pViewParams->m_pCurrentEngine->m_DebugFlags & M_Bit(16)))
							{
								CXR_VBManager* pVBM = pViewParams->m_pCurrentEngine->GetVBM();
								CMat4Dfp32 Pos; Pos.Unit();
								CVec3Dfp32 Center;
								pViewParams->m_VisVBox.GetCenter(Center);
								Center.SetRow(Pos, 3);
								pVBM->RenderWire(m_pWorldView->m_VPVMat, Center, pL->GetPosition(), 0xff003f7f);

								if (SV.m_nPlaneLightBound)
								{
									CSolid Solid;
									for(int p = 0; p < SV.m_nPlaneLightBound; p++)
									{
										const CPlane3Dfp32& P(pPlanes[SV.m_iPlaneLightBound + p]);
										Solid.AddPlane(CPlane3Dfp64(P.n.Getfp64(), P.d));
									}

									Solid.UpdateMesh();
									Solid.RenderEdges(pVBM, &m_pWorldView->m_VPVMat, 0x3f006f6f);
								}
								else
									pVBM->RenderBox(m_pWorldView->m_VPVMat, pL->m_BoundBox, 0xff6f0000);
							}
#endif
						}
						else
						{
							if (pL->m_iLight >= pSG->m_iFirstDynamic)
							{
								if (pL->m_Type != CXR_LIGHTTYPE_SPOT || 
									pSG->m_lDynamicLightClip[pL->m_iLight-pSG->m_iFirstDynamic].IntersectBox(pViewParams->m_VisVBox))
								{
									AddLightToRenderInfo(pViewParams->m_pRenderInfo, pL, m_pWorldView->m_lPortalScissor[iRP]);

									if (pViewParams->m_pCurrentEngine && (pViewParams->m_pCurrentEngine->m_DebugFlags & M_Bit(16)))
									{
										CXR_VBManager* pVBM = pViewParams->m_pCurrentEngine->GetVBM();
										CMat4Dfp32 Pos; Pos.Unit();
										CVec3Dfp32 Center;
										pViewParams->m_VisVBox.GetCenter(Center);
										Center.SetRow(Pos, 3);
										pVBM->RenderWire(m_pWorldView->m_VPVMat, Center, pL->GetPosition(), 0xff3f7f00);
										pVBM->RenderBox(m_pWorldView->m_VPVMat, pL->m_BoundBox, 0xff6f0000);
									}
								}
							}
						}
					}
				}
			}
		}
	}
	return bRes;
}

bool CXR_Model_BSP2::View_GetClip_Box_i(int _iNode, CBSP2_View_Params* _pViewParams) const
{
	MAUTOSTRIP(CXR_Model_BSP_View_GetClip_Box_i, false);
	if (!_iNode) return false;

	CBSP2_View_Params* pViewParams = _pViewParams;

	CVec3Dfp32 VMin = pViewParams->m_VisVBox.m_Min;
	CVec3Dfp32 VMax = pViewParams->m_VisVBox.m_Max;

	bool bRes = false;
	uint aWorkStack[256];
	uint iWorkPos = 0;
	uint iWorkNode = _iNode;
	goto StartOf_GetClipBox_i_NoAdd;

StartOf_GetClipBox_i:
	iWorkNode = aWorkStack[--iWorkPos];

StartOf_GetClipBox_i_NoAdd:

	const CBSP2_Node* pN = &m_pNodes[iWorkNode];

	if (pN->IsStructureLeaf())
	{
		if(View_GetClip_Box_StructureLeaf(iWorkNode, pViewParams))
		{
			if (!(pViewParams->m_pClipVolume || pViewParams->m_pRenderInfo))
				return true;

			bRes = true;
		}
	}
	else if (pN->IsNode())
	{
#ifdef MODEL_BSP_USEPLANETYPES
		int Type = pN->m_Flags & XW_NODE_PLANETYPEANDSIGN;
		if (!Type)
		{
#endif
			uint iBack = pN->m_iNodeBack;
			uint iPlane = pN->m_iPlane;
			if (!iBack)
			{
				int iFront = pN->m_iNodeFront;
				if (m_pPlanes[iPlane].GetBoxMaxDistance(VMin, VMax) > -MODEL_BSP_EPSILON)
				{
					iWorkNode = iFront;
					goto StartOf_GetClipBox_i_NoAdd;
				}
			}
			else
			{
				uint iFront = pN->m_iNodeFront;
				if (!iFront)
				{
					if (m_pPlanes[iPlane].GetBoxMinDistance(VMin, VMax) < MODEL_BSP_EPSILON)
					{
						iWorkNode = iBack;
						goto StartOf_GetClipBox_i_NoAdd;
					}
				}
				else
				{
					fp32 MinDist, MaxDist;
					m_pPlanes[iPlane].GetBoxMinAndMaxDistance(VMin, VMax, MinDist, MaxDist);
					
					uint32 bAdd = 0;
					if (MaxDist > -MODEL_BSP_EPSILON)
					{
						bAdd = 1;
						aWorkStack[iWorkPos] = iFront;
						iWorkNode = iFront;
					}
					if (MinDist < MODEL_BSP_EPSILON)
					{
						iWorkPos += bAdd;
						iWorkNode = iBack;
						goto StartOf_GetClipBox_i_NoAdd;
					}

					if(bAdd)
						goto StartOf_GetClipBox_i_NoAdd;
				}
			}
#ifdef MODEL_BSP_USEPLANETYPES
		}
		else
		{
			const CPlane3Dfp32* M_RESTRICT pP = &pPlanes[pN->m_iPlane];
			uint k = ((Type & XW_NODE_PLANETYPEAND) >> XW_NODE_PLANETYPESHIFT)-1;

			uint iBack = pN->m_iNodeBack;
			uint iFront = pN->m_iNodeFront;
			fp32 d = pP->d;
			if (Type & XW_NODE_PLANENEG)
			{
				if (iFront && (pViewParams->m_VisVBox.m_Min.k[k] < d))
					aWorkStack[iWorkPos++] = iFront;
				if (iBack && (pViewParams->m_VisVBox.m_Max.k[k] > d))
				{
					iWorkNode = iBack;
					goto StartOf_GetClipBox_i_NoAdd;
				}
			}
			else
			{
				if (iFront && (pViewParams->m_VisVBox.m_Max.k[k] + d > 0.0f))
					aWorkStack[iWorkPos++] = iFront;
				if (iBack && (pViewParams->m_VisVBox.m_Min.k[k] + d < 0.0f))
				{
					iWorkNode = iBack;
					goto StartOf_GetClipBox_i_NoAdd;
				}
			}
		}
#endif
/*		int Side = m_pPlanes[pN->m_iPlane].GetBoxPlaneSideMask(m_VisVBox.m_Min, m_VisVBox.m_Max);
		if (Side & 1) View_GetClip_Box_r(pN->m_iNodeFront, m_VisVBox.m_Min, m_VisVBox.m_Max, _pClipVolume);
		if (Side & 2) View_GetClip_Box_r(pN->m_iNodeBack, m_VisVBox.m_Min, m_VisVBox.m_Max, _pClipVolume);*/
	}

	if(iWorkPos > 0) goto StartOf_GetClipBox_i;

	return bRes;
}
/*
bool CXR_Model_BSP2::View_GetClip_Box_r(int _iNode, CBSP2_View_Params* _pViewParams) const
{
	MAUTOSTRIP(CXR_Model_BSP_View_GetClip_Box_r, false);
	if (!_iNode) return false;
	const CBSP2_Node* pN = &m_pNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		return View_GetClip_Box_StructureLeaf(_iNode, _pViewParams);
	}

	if (pN->IsNode())
	{
#ifdef MODEL_BSP_USEPLANETYPES
		int Type = pN->m_Flags & XW_NODE_PLANETYPEANDSIGN;
		if (!Type)
		{
#endif
			int iBack = pN->m_iNodeBack;
			if (!iBack)
			{
				int iFront = pN->m_iNodeFront;
				if (m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_pViewParams->m_VisVBox.m_Min, _pViewParams->m_VisVBox.m_Max) > -MODEL_BSP_EPSILON)
					return View_GetClip_Box_r(iFront, _pViewParams);
				return false;
			}
			else
			{
				int iFront = pN->m_iNodeFront;
				if (!iFront)
				{
					if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_pViewParams->m_VisVBox.m_Min, _pViewParams->m_VisVBox.m_Max) < MODEL_BSP_EPSILON)
						return View_GetClip_Box_r(iBack, _pViewParams);
					return false;
				}
				else
				{
					bool bVis = false;
					if (m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_pViewParams->m_VisVBox.m_Min, _pViewParams->m_VisVBox.m_Max) > -MODEL_BSP_EPSILON)
						bVis = View_GetClip_Box_r(iFront, _pViewParams);

					if (!(_pViewParams->m_pClipVolume || _pViewParams->m_pRenderInfo) && bVis) return true;
					if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_pViewParams->m_VisVBox.m_Min, _pViewParams->m_VisVBox.m_Max) < MODEL_BSP_EPSILON)
						bVis |= View_GetClip_Box_r(iBack, _pViewParams);

					return bVis;
//					int Side = m_pPlanes[pN->m_iPlane].GetBoxPlaneSideMask(m_VisVBox.m_Min, m_VisVBox.m_Max);
//					if (Side & 5) View_GetClip_Box_r(pN->m_iNodeFront);
//					if (Side & 6) View_GetClip_Box_r(pN->m_iNodeBack);
				}
			}
#ifdef MODEL_BSP_USEPLANETYPES
		}
		else
		{
			CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
			int k = ((Type & XW_NODE_PLANETYPEAND) >> XW_NODE_PLANETYPESHIFT)-1;

			int iBack = pN->m_iNodeBack;
			int iFront = pN->m_iNodeFront;
			fp32 d = pP->d;
			if (Type & XW_NODE_PLANENEG)
			{
				bool bVis = false;
				if (iFront && (_pViewParams->m_VisVBox.m_Min.k[k] < d))
					bVis = View_GetClip_Box_r(iFront, _pViewParams);
				if (!_pViewParams->m_pClipVolume && bVis) return true;
				if (iBack && (_pViewParams->m_VisVBox.m_Max.k[k] > d))
					bVis |= View_GetClip_Box_r(iBack, _pViewParams);
				return bVis;
			}
			else
			{
				bool bVis = false;
				if (iFront && (_pViewParams->m_VisVBox.m_Max.k[k] + d > 0.0f))
					bVis = View_GetClip_Box_r(iFront, _pViewParams);
				if (!(_pViewParams->m_pClipVolume || _pViewParams->m_pRenderInfo) && bVis) return true;
				if (iBack && (_pViewParams->m_VisVBox.m_Min.k[k] + d < 0.0f))
					bVis |= View_GetClip_Box_r(iBack, _pViewParams);
				return bVis;
			}
		}
#endif
//		int Side = m_pPlanes[pN->m_iPlane].GetBoxPlaneSideMask(m_VisVBox.m_Min, m_VisVBox.m_Max);
//		if (Side & 1) View_GetClip_Box_r(pN->m_iNodeFront, m_VisVBox.m_Min, m_VisVBox.m_Max, _pClipVolume);
//		if (Side & 2) View_GetClip_Box_r(pN->m_iNodeBack, m_VisVBox.m_Min, m_VisVBox.m_Max, _pClipVolume);
	}
	return false;
}
*/
//__forceinline void ProjectRectExpandBox(int _XC, int _YC, int _ZC, 

#define PROJECTEXPANDMIN(Comp) \
		fp32 pmin = _Box.m_Min[Comp] + (_Box.m_Min[Comp] - LightPos[Comp]) * Scale;	\
		_Shadow.m_Min[Comp] = Min(_Shadow.m_Min[Comp], pmin);						

#define PROJECTEXPANDMAX(Comp) \
		fp32 pmax = _Box.m_Max[Comp] + (_Box.m_Max[Comp] - LightPos[Comp]) * Scale;	\
		_Shadow.m_Max[Comp] = Max(_Shadow.m_Max[Comp], pmax);						


/*#define CHECKINTERVALSETMAXDEPTH(Comp, VarDepth, MaxDepth) \
	if (pmin < LightBound.m_Max[Comp] && pmax > LightBound.m_Min[Comp]) VarDepth = MaxDepth;
*/

#define CHECKINTERVALSETMAXDEPTH(Comp, VarDepth, MaxDepth) \
	VarDepth = M_FSel(pmin - LightBound.m_Max[Comp], VarDepth, M_FSel(LightBound.m_Min[Comp] - pmax, VarDepth, MaxDepth));

//	if (_Box.m_Max[Comp] < LightBound.m_Max[Comp])									
//	if (_Box.m_Min[Comp] > LightBound.m_Min[Comp])									


void CalcShadowBox(const CXR_Light& _Light, const CBox3Dfp32& _Box, CBox3Dfp32& _Shadow)
{
	_Shadow = _Box;

	CVec3Dfp32 LightPos = _Light.GetPosition();
	CBox3Dfp32 LightBound = _Light.m_BoundBox;

	fp32 Case = M_FSel((_Box.m_Max[0] + 0.01f) - LightPos[0],
					M_FSel((_Box.m_Min[0] - 0.01f) - LightPos[0], 0.0f,
						M_FSel((_Box.m_Max[1] + 0.01f) - LightPos[1],
							M_FSel((_Box.m_Min[1] - 0.01f) - LightPos[1], 0.0f,
								M_FSel((_Box.m_Max[2] + 0.01f) - LightPos[2],
									M_FSel((_Box.m_Min[2] - 0.01f) - LightPos[2], 0.0f, 1.0f),
							1.0f)),
					1.0f)),
				1.0f);

//	if ((LightPos[0] < _Box.m_Max[0] + 0.01f) && (LightPos[0] > _Box.m_Min[0] - 0.01f) &&
//		(LightPos[1] < _Box.m_Max[1] + 0.01f) && (LightPos[1] > _Box.m_Min[1] - 0.01f) &&
//		(LightPos[2] < _Box.m_Max[2] + 0.01f) && (LightPos[2] > _Box.m_Min[2] - 0.01f))
	if(Case != 0.0f)
	{
		// Light is (almost) inside the box, so the shadow bound is the whole light volume.
		_Shadow = LightBound;
		return;
	}

//	if ((LightPos[2] > _Box.m_Max[2] + 0.001f) && (LightBound.m_Min[2] < _Box.m_Max[2]))
	if (M_FSel((_Box.m_Max[2] + 0.001f) - LightPos[2], 0.0f, M_FSel(LightBound.m_Min[2] - _Box.m_Max[2], 0.0f, 1.0f)))
	{
		fp32 Scale = (_Box.m_Max[2] - LightBound.m_Min[2]) / (LightPos[2] - _Box.m_Max[2]);
		{
			PROJECTEXPANDMIN(0);
			PROJECTEXPANDMAX(0);
			CHECKINTERVALSETMAXDEPTH(0, _Shadow.m_Min[2], LightBound.m_Min[2]);
		}
		{
			PROJECTEXPANDMIN(1);
			PROJECTEXPANDMAX(1);
			CHECKINTERVALSETMAXDEPTH(1, _Shadow.m_Min[2], LightBound.m_Min[2]);
		}
	}
//	else if ((LightPos[2] < _Box.m_Min[2] - 0.001f) && (LightBound.m_Max[2] > _Box.m_Min[2]))
	else if (M_FSel(LightPos[2] - (_Box.m_Min[2] - 0.001f), 0.0f, M_FSel(_Box.m_Min[2] - LightBound.m_Max[2], 0.0f, 1.0f)))
	{
		fp32 Scale = (_Box.m_Min[2] - LightBound.m_Max[2]) / (LightPos[2] - _Box.m_Min[2]);
		{
			PROJECTEXPANDMIN(0);
			PROJECTEXPANDMAX(0);
			CHECKINTERVALSETMAXDEPTH(0, _Shadow.m_Max[2], LightBound.m_Max[2]);
		}
		{
			PROJECTEXPANDMIN(1);
			PROJECTEXPANDMAX(1);
			CHECKINTERVALSETMAXDEPTH(1, _Shadow.m_Max[2], LightBound.m_Max[2]);
		}
	}


//	if ((LightPos[0] > _Box.m_Max[0] + 0.001f) && (LightBound.m_Min[0] < _Box.m_Max[0]))
	if (M_FSel((_Box.m_Max[0] + 0.001f) - LightPos[0], 0.0f, M_FSel(LightBound.m_Min[0] - _Box.m_Max[0], 0.0f, 1.0f)))
	{
		fp32 Scale = (_Box.m_Max[0] - LightBound.m_Min[0]) / (LightPos[0] - _Box.m_Max[0]);
		{
			PROJECTEXPANDMIN(1);
			PROJECTEXPANDMAX(1);
			CHECKINTERVALSETMAXDEPTH(1, _Shadow.m_Min[0], LightBound.m_Min[0]);
		}
		{
			PROJECTEXPANDMIN(2);
			PROJECTEXPANDMAX(2);
			CHECKINTERVALSETMAXDEPTH(2, _Shadow.m_Min[0], LightBound.m_Min[0]);
		}
	}
//	else if ((LightPos[0] < _Box.m_Min[0] - 0.001f) && (LightBound.m_Max[0] > _Box.m_Min[0]))
	else if (M_FSel(LightPos[0] - (_Box.m_Min[0] - 0.001f), 0.0f, M_FSel(_Box.m_Min[0] - LightBound.m_Max[0], 0.0f, 1.0f)))
	{
		fp32 Scale = (_Box.m_Min[0] - LightBound.m_Max[0]) / (LightPos[0] - _Box.m_Min[0]);
		{
			PROJECTEXPANDMIN(1);
			PROJECTEXPANDMAX(1);
			CHECKINTERVALSETMAXDEPTH(1, _Shadow.m_Max[0], LightBound.m_Max[0]);
		}
		{
			PROJECTEXPANDMIN(2);
			PROJECTEXPANDMAX(2);
			CHECKINTERVALSETMAXDEPTH(2, _Shadow.m_Max[0], LightBound.m_Max[0]);
		}
	}

//	if ((LightPos[1] > _Box.m_Max[1] + 0.001f) && (LightBound.m_Min[1] < _Box.m_Max[1]))
	if (M_FSel((_Box.m_Max[1] + 0.001f) - LightPos[1], 0.0f, M_FSel(LightBound.m_Min[1] - _Box.m_Max[1], 0.0f, 1.0f)))
	{
		fp32 Scale = (_Box.m_Max[1] - LightBound.m_Min[1]) / (LightPos[1] - _Box.m_Max[1]);
		{
			PROJECTEXPANDMIN(0);
			PROJECTEXPANDMAX(0);
			CHECKINTERVALSETMAXDEPTH(0, _Shadow.m_Min[1], LightBound.m_Min[1]);
		}
		{
			PROJECTEXPANDMIN(2);
			PROJECTEXPANDMAX(2);
			CHECKINTERVALSETMAXDEPTH(2, _Shadow.m_Min[1], LightBound.m_Min[1]);
		}
	}
//	else if ((LightPos[1] < _Box.m_Min[1] - 0.001f) && (LightBound.m_Max[1] > _Box.m_Min[1]))
	else if (M_FSel(LightPos[1] - (_Box.m_Min[1] - 0.001f), 0.0f, M_FSel(_Box.m_Min[1] - LightBound.m_Max[1], 0.0f, 1.0f)))
	{
		fp32 Scale = (_Box.m_Min[1] - LightBound.m_Max[1]) / (LightPos[1] - _Box.m_Min[1]);
		{
			PROJECTEXPANDMIN(0);
			PROJECTEXPANDMAX(0);
			CHECKINTERVALSETMAXDEPTH(0, _Shadow.m_Max[1], LightBound.m_Max[1]);
		}
		{
			PROJECTEXPANDMIN(2);
			PROJECTEXPANDMAX(2);
			CHECKINTERVALSETMAXDEPTH(2, _Shadow.m_Max[1], LightBound.m_Max[1]);
		}
	}

	_Shadow.And(_Light.m_BoundBox);
}

#if 0
void CalcShadowBox(const CXR_Light& _Light, const CBox3Dfp32& _Box, CBox3Dfp32& _Shadow)
{
	CVec3Dfp32 lVBox[8];
	_Shadow = _Box;
	_Shadow.And(_Light.m_BoundBox);
	_Shadow.GetVertices(lVBox);

	for(int v = 0; v < 8; v++)
	{
		CVec3Dfp32 BoxHit;
		CVec3Dfp32 TraceV, TracePos;
		lVBox[v].Sub(_Light.GetPosition(), TraceV);

		fp32 TraceVLenSqr = TraceV.LengthSqr();

		if (TraceVLenSqr > Sqr(_Light.m_Range*0.125f))
		{
			lVBox[v].Combine(TraceV, 8.0f, TracePos);
		}
		else
		{
			lVBox[v].Combine(TraceV, _Light.m_Range*2.0f / M_Sqrt(TraceVLenSqr), TracePos);
		}

		if (_Light.m_BoundBox.IntersectLine(TracePos, lVBox[v], BoxHit))
		{
			_Shadow.Expand(BoxHit);
/*			if (BoxHit[0] < _Shadow.m_Min[0]) _Shadow.m_Min[0] = BoxHit[0];
			if (BoxHit[1] < _Shadow.m_Min[1]) _Shadow.m_Min[1] = BoxHit[1];
			if (BoxHit[2] < _Shadow.m_Min[2]) _Shadow.m_Min[2] = BoxHit[2];
			if (BoxHit[0] > _Shadow.m_Max[0]) _Shadow.m_Max[0] = BoxHit[0];
			if (BoxHit[1] > _Shadow.m_Max[1]) _Shadow.m_Max[1] = BoxHit[1];
			if (BoxHit[2] > _Shadow.m_Max[2]) _Shadow.m_Max[2] = BoxHit[2];*/
		}
	}

/*
	const CVec3Dfp32& Pos = _Light.GetPosition();
	for(int k = 0; k < 3; k++)
	{
		if (Pos[k] < _Box.m_Min.k[k])
		{
			_Shadow.m_Min[k] = _Box.m_Min.k[k];
			_Shadow.m_Max[k] = _Light.m_BoundBox.m_Max[k];
		}
		else if (Pos[k] > _Box.m_Max.k[k])
		{
			_Shadow.m_Min[k] = _Light.m_BoundBox.m_Min[k];
			_Shadow.m_Max[k] = _Box.m_Max.k[k];
		}
		else
		{
			_Shadow.m_Min[k] = _Light.m_BoundBox.m_Min[k];
			_Shadow.m_Max[k] = _Light.m_BoundBox.m_Max[k];
		}
	}*/
}
#endif

void CXR_Model_BSP2::View_GetScissorWithPVS_Box_i(int _iNode, const CBox3Dfp32& _VisBox, const uint8* _pPVS, CScissorRect* _pScissor) const
{
	MAUTOSTRIP(CXR_Model_BSP_View_GetScissorWithPVS_Box_i, false);
	if (!_iNode)
		return;

	int lWorkingStack[256];
	int StackPos = 0;

	const CBSP2_Node* pNodes = m_pNodes;

	goto IterNoPop;

IterPopStack:
	StackPos--;
	_iNode = lWorkingStack[StackPos];

IterNoPop:

	const CBSP2_Node* pN = &pNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		int iPL = pN->m_iPortalLeaf;
		if (_pPVS)
		{
			if (!(_pPVS[iPL >> 3] & aShiftMulTab[iPL & 7]))
				goto IterEnd;
		}

		int iRP = m_pView->m_liLeafRPortals[iPL];
		if (iRP != 0)
		{
			if (m_pView->m_pRPortals[iRP].m_nPlanes < 3) 
				goto IterEnd;
			CRC_ClipVolume* pClip = &m_pView->m_pRPortals[iRP];
			const CBSP2_PortalLeafExt* pPL = &m_lPortalLeaves[iPL];

			CBox3Dfp32 Cut;
			_VisBox.And(pPL->m_BoundBox, Cut);
/*			const CBox3Dfp32& PLBox = pPL->m_BoundBox;
			Cut.m_Min[0] = Max(PLBox.m_Min[0], m_VisVBox.m_Min[0]);
			Cut.m_Min[1] = Max(PLBox.m_Min[1], m_VisVBox.m_Min[1]);
			Cut.m_Min[2] = Max(PLBox.m_Min[2], m_VisVBox.m_Min[2]);
			Cut.m_Max[0] = Min(PLBox.m_Max[0], m_VisVBox.m_Max[0]);
			Cut.m_Max[1] = Min(PLBox.m_Max[1], m_VisVBox.m_Max[1]);
			Cut.m_Max[2] = Min(PLBox.m_Max[2], m_VisVBox.m_Max[2]);*/

			//Unroll first 5 planes (since that is the general case)
			const CPlane3Dfp32* pPlanes = pClip->m_Planes;
			int nPlanes = pClip->m_nPlanes;
			{
				M_ASSERT(nPlanes > 3, "Valid volume consists of atleast 4 planes");
				fp32 Dist0 = pPlanes[0].GetBoxMinDistance(Cut.m_Min, Cut.m_Max);
				fp32 Dist1 = pPlanes[1].GetBoxMinDistance(Cut.m_Min, Cut.m_Max);
				fp32 Dist2 = pPlanes[2].GetBoxMinDistance(Cut.m_Min, Cut.m_Max);
				fp32 Dist3 = pPlanes[3].GetBoxMinDistance(Cut.m_Min, Cut.m_Max);

				fp32 Dist = Max(Max(Dist0, Dist1), Max(Dist2, Dist3));

				if(Dist > MODEL_BSP_EPSILON)
					goto IterEnd;
			}
			for (int p = 4; p < nPlanes; p++)
				if (pClip->m_Planes[p].GetBoxMinDistance(Cut.m_Min, Cut.m_Max) > MODEL_BSP_EPSILON)
					goto IterEnd;

			CScissorRect BoxScissor;
			CalcBoxScissor(m_pView, Cut, BoxScissor);

			BoxScissor.And(m_pView->m_lPortalScissor[iRP]);

			_pScissor->Expand(BoxScissor);
		}
	}
	else if (pN->IsNode())
	{
		int Type = pN->m_Flags & XW_NODE_PLANETYPEANDSIGN;
		if (Type)
		{
			const CPlane3Dfp32& P = m_pPlanes[pN->m_iPlane];
			int iComp = ((Type & XW_NODE_PLANETYPEAND) >> XW_NODE_PLANETYPESHIFT) - 1;
			fp32 MaxVal = _VisBox.m_Max[iComp];
			fp32 MinVal = _VisBox.m_Min[iComp];
			if (!(Type & XW_NODE_PLANENEG))
			{
				int iBack = pN->m_iNodeBack;
				if (!iBack)
				{
					int iFront = pN->m_iNodeFront;
					if (MaxVal + P.d > -MODEL_BSP_EPSILON)
					{
						_iNode = iFront;
						goto IterNoPop;
					}
				}
				else
				{
					int iFront = pN->m_iNodeFront;
					if (!iFront)
					{
						if (MinVal + P.d < MODEL_BSP_EPSILON)
						{
							_iNode = iBack;
							goto IterNoPop;
						}
					}
					else
					{
						if (MaxVal + P.d > -MODEL_BSP_EPSILON)
						{
							lWorkingStack[StackPos] = iFront;
							StackPos++;
						}

						if (MinVal + P.d < MODEL_BSP_EPSILON)
						{
							_iNode = iBack;
							goto IterNoPop;
						}
					}
				}
			}
			else
			{
				int iBack = pN->m_iNodeBack;
				if (!iBack)
				{
					int iFront = pN->m_iNodeFront;
					if (P.d-MinVal > -MODEL_BSP_EPSILON)
					{
						_iNode = iFront;
						goto IterNoPop;
					}
				}
				else
				{
					int iFront = pN->m_iNodeFront;
					if (!iFront)
					{
						if (P.d-MaxVal < MODEL_BSP_EPSILON)
						{
							_iNode = iBack;
							goto IterNoPop;
						}
					}
					else
					{
						if (P.d-MinVal > -MODEL_BSP_EPSILON)
						{
							lWorkingStack[StackPos] = iFront;
							StackPos++;
						}

						if (P.d-MaxVal < MODEL_BSP_EPSILON)
						{
							_iNode = iBack;
							goto IterNoPop;
						}
					}
				}
			}
		}
		else
		{
			fp32 MinDist, MaxDist;
			m_pPlanes[pN->m_iPlane].GetBoxMinAndMaxDistance(_VisBox.m_Min, _VisBox.m_Max, MinDist, MaxDist);
			int iFront = pN->m_iNodeFront;
			int iBack = pN->m_iNodeBack;
			if(iFront && iBack)
			{
				if(MaxDist > MODEL_BSP_EPSILON)
				{
					lWorkingStack[StackPos++]	= iFront;
				}

				if(MinDist < -MODEL_BSP_EPSILON)
				{
					_iNode = iBack;
					goto IterNoPop;
				}
			}
			else if(iFront)
			{
				if(MaxDist > MODEL_BSP_EPSILON)
				{
					_iNode = iFront;
					goto IterNoPop;
				}
			}
			else
			{
				if(MinDist < -MODEL_BSP_EPSILON)
				{
					_iNode = iBack;
					goto IterNoPop;
				}
			}
/*
			int iBack = pN->m_iNodeBack;
			if (!iBack)
			{
				int iFront = pN->m_iNodeFront;
				if (m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_VisBox.m_Min, _VisBox.m_Max) > -MODEL_BSP_EPSILON)
				{
					_iNode = iFront;
					goto IterNoPop;
				}
			}
			else
			{
				int iFront = pN->m_iNodeFront;
				if (!iFront)
				{
					if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_VisBox.m_Min, _VisBox.m_Max) < MODEL_BSP_EPSILON)
					{
						_iNode = iBack;
						goto IterNoPop;
					}
				}
				else
				{
					fp32 MinDistance, MaxDistance;
					m_pPlanes[pN->m_iPlane].GetBoxMinAndMaxDistance(_VisBox.m_Min, _VisBox.m_Max, MinDistance, MaxDistance);

					if (MaxDistance > -MODEL_BSP_EPSILON)
					{
						lWorkingStack[StackPos] = iFront;
						StackPos++;
					}

					if (MinDistance < MODEL_BSP_EPSILON)
					{
						_iNode = iBack;
						goto IterNoPop;
					}
				}
			}
*/		}
	}
IterEnd:
	if (StackPos)
		goto IterPopStack;
}

void CXR_Model_BSP2::View_GetVisibleBoxWithPVS_Box_r(int _iNode, const CBox3Dfp32* _pVisBox, const uint8* _pPVS, CBox3Dfp32* _pDestBox) const
{
	MAUTOSTRIP(CXR_Model_BSP_View_GetVisibleBoxWithPVS_Box_r, false);
	if (!_iNode)
		return;
	const CBSP2_Node* pN = &m_pNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		int iPL = pN->m_iPortalLeaf;
		if (_pPVS)
		{
			if (!(_pPVS[iPL >> 3] & aShiftMulTab[iPL & 7]))
				return;
		}

/*		int iRP = m_pView->m_liLeafRPortals[iPL];
		if (iRP != 0)
		{
			if (m_pView->m_pRPortals[iRP].m_nPlanes < 3) 
				return;
			CRC_ClipVolume* pClip = &m_pView->m_pRPortals[iRP];*/
			const CBSP2_PortalLeafExt* pPL = &m_lPortalLeaves[iPL];

			const CBox3Dfp32& PLBox = pPL->m_BoundBox;
			CBox3Dfp32 Cut;
			Cut.m_Min[0] = Max(PLBox.m_Min[0], _pVisBox->m_Min[0]);
			Cut.m_Min[1] = Max(PLBox.m_Min[1], _pVisBox->m_Min[1]);
			Cut.m_Min[2] = Max(PLBox.m_Min[2], _pVisBox->m_Min[2]);
			Cut.m_Max[0] = Min(PLBox.m_Max[0], _pVisBox->m_Max[0]);
			Cut.m_Max[1] = Min(PLBox.m_Max[1], _pVisBox->m_Max[1]);
			Cut.m_Max[2] = Min(PLBox.m_Max[2], _pVisBox->m_Max[2]);

/*			int p;
			for (p = 0; p < pClip->m_nPlanes; p++)
				if (pClip->m_Planes[p].GetBoxMinDistance(Cut.m_Min, Cut.m_Max) > MODEL_BSP_EPSILON) break;
			if (p != pClip->m_nPlanes) 
				return;
*/
			_pDestBox->Expand(Cut);
//		}
		return;
	}

	if (pN->IsNode())
	{
		int iBack = pN->m_iNodeBack;
		if (!iBack)
		{
			int iFront = pN->m_iNodeFront;
			if (m_pPlanes[pN->m_iPlane].GetBoxMaxDistance(_pVisBox->m_Min, _pVisBox->m_Max) > -MODEL_BSP_EPSILON)
				View_GetVisibleBoxWithPVS_Box_r(iFront, _pVisBox, _pPVS, _pDestBox);
		}
		else
		{
			int iFront = pN->m_iNodeFront;
			if (!iFront)
			{
				if (m_pPlanes[pN->m_iPlane].GetBoxMinDistance(_pVisBox->m_Min, _pVisBox->m_Max) < MODEL_BSP_EPSILON)
					View_GetVisibleBoxWithPVS_Box_r(iBack, _pVisBox, _pPVS, _pDestBox);
			}
			else
			{
				fp32 MinDistance, MaxDistance;
				m_pPlanes[pN->m_iPlane].GetBoxMinAndMaxDistance(_pVisBox->m_Min, _pVisBox->m_Max, MinDistance, MaxDistance);

				if (MaxDistance > -MODEL_BSP_EPSILON)
					View_GetVisibleBoxWithPVS_Box_r(iFront, _pVisBox, _pPVS, _pDestBox);

				if (MinDistance < MODEL_BSP_EPSILON)
					View_GetVisibleBoxWithPVS_Box_r(iBack, _pVisBox, _pPVS, _pDestBox);
			}
		}
	}
}

#include "../../Classes/Render/MWireContainer.h"

#define CXR_RENDERINFO_INVISIBLE 32

bool CXR_Model_BSP2::View_GetClip_Box(CVec3Dfp32 _VMin, CVec3Dfp32 _VMax, int _MediumFlags, int _ObjectMask, CRC_ClipVolume* _pClipVolume, CXR_RenderInfo* _pRenderInfo)
{
	MAUTOSTRIP(CXR_Model_BSP_View_GetClip_Box, false);
	MSCOPESHORT(CXR_Model_BSP2::View_GetClip_Box);

	CBSP2_View_Params Params;
	Params.SetupForBox(_pRenderInfo?_pRenderInfo->m_pCurrentEngine->m_pCurrentFogState:NULL, CBox3Dfp32(_VMin, _VMax), _pClipVolume, _pRenderInfo, _pRenderInfo?_pRenderInfo->m_pCurrentEngine:NULL);

	CBSP2_LightData* pLD = m_spLightData;
	if (!pLD)
		Error("View_GetClip_Box", "No light data.");

	if (_pClipVolume)
	{
#ifndef M_RTM
		ConOutL("ERROR: (CXR_Model_BSP2::View_GetClip_Box) Don't use clip volume.");
#endif
		return false;
	}
	else
	{
		bool bRes = View_GetClip_Box_i(1, &Params);
		if (_pRenderInfo && _pRenderInfo->m_nLights)
		{
			// No shadows allowed
			_pRenderInfo->m_Flags |= CXR_RENDERINFO_NOSHADOWVOLUMES;

			for(int i = 0; i < _pRenderInfo->m_nLights; i++)
			{
				CXR_LightInfo& Info = _pRenderInfo->m_pLightInfo[i];
				Info.m_ShadowScissor.SetRect(0xffff, 0x0000);

/*				m_Vis_Scissor.m_Min = 0xffff;
				m_Vis_Scissor.m_Max = 0;
				m_Vis_Scissor_pPVS = SceneGraph_PVSLock(0, Info.m_pLight->GetPosition());
				CalcShadowBox(*Info.m_pLight, CBox3Dfp32(_VMin, _VMax), m_VisVBox);

				View_GetScissorWithPVS_Box_r(1);

				Info.m_ShadowScissor = m_Vis_Scissor;
				if (m_Vis_Scissor_pPVS)
				{
					SceneGraph_PVSRelease(m_Vis_Scissor_pPVS);
					m_Vis_Scissor_pPVS = NULL;
				}*/
			}
		}
		if (_pRenderInfo)
		{
			_pRenderInfo->m_pLightVolume = m_spLightFieldOcttree;
			if (!bRes)
			{
				_pRenderInfo->m_Flags |= CXR_RENDERINFO_INVISIBLE;
			}
		}
		return bRes;
	}
}

void CXR_Model_BSP2::View_GetClip(int _Elem, CXR_RenderInfo* _pRenderInfo)
{
	if (!_pRenderInfo)
		return;

	MSCOPESHORT(CXR_Model_BSP2::View_GetClip);

	{
		CBSP2_SceneGraph* pSG = m_pWorldView->m_pSceneGraph;
		M_LOCK(pSG->m_Lock);

		const CBox3Dfp32& Box = pSG->m_ObjectsNoShadow.m_lIDLinkBox[_Elem];

		int ElemFlags = pSG->m_Objects.m_lIDFlags[_Elem];
		if (ElemFlags & CXR_SCENEGRAPH_NOPORTALPVSCULL)
		{
			_pRenderInfo->m_Flags |= CXR_RENDERINFO_NOSHADOWVOLUMES;
			_pRenderInfo->m_nLights = 0;
			return;
		}

		CXR_Engine* pCurrentEngine = _pRenderInfo->m_pCurrentEngine;

		int bDebug = pCurrentEngine && (pCurrentEngine->m_DebugFlags & M_Bit(16));

		_pRenderInfo->m_pLightVolume = m_spLightFieldOcttree;

		if (ElemFlags & CXR_SCENEGRAPH_SHADOWCASTER)
		{
			const int MaxVisPL = 256;
			uint16 liVisPL[MaxVisPL];
			int nVisPL = 0;

			// Find out if the "body" of the object is visible
			{
				const CBSP2_Link* pObjLinks = pSG->m_ObjectsNoShadow.GetLinks();
				int iLinkPL = pSG->m_ObjectsNoShadow.GetLink_ID(_Elem);

				CVec3Dfp32 BoxMin = Box.m_Min;
				CVec3Dfp32 BoxMax = Box.m_Max;

				for(; iLinkPL; iLinkPL = pObjLinks[iLinkPL].m_iLinkNextPL)
				{
					int iPL = pObjLinks[iLinkPL].m_iPortalLeaf;
					int iRP = m_pWorldView->m_liLeafRPortals[iPL];
					if (iRP != 0)
					{
						CRC_ClipVolume* pClip = &m_pWorldView->m_pRPortals[iRP];
						uint nPlanes = pClip->m_nPlanes;
						if (nPlanes < 3)
							continue;

						fp32 Dist[3];
						Dist[0] = pClip->m_Planes[0].GetBoxMinDistance(BoxMin, BoxMax);
						Dist[1] = pClip->m_Planes[1].GetBoxMinDistance(BoxMin, BoxMax);
						Dist[2] = pClip->m_Planes[2].GetBoxMinDistance(BoxMin, BoxMax);

						if(Max(Dist[2], Max(Dist[1], Dist[0])) <= MODEL_BSP_EPSILON)
						{
							uint p;
							for (p = 3; p < nPlanes; p++)
								if (pClip->m_Planes[p].GetBoxMinDistance(BoxMin, BoxMax) > MODEL_BSP_EPSILON)
									break;
							if (p == nPlanes)
							{
								liVisPL[nVisPL] = iPL;
								nVisPL++;
								if (nVisPL == MaxVisPL)
									break;
							}
						}
					}
				}

				if (!nVisPL)
					_pRenderInfo->m_Flags |= CXR_RENDERINFO_INVISIBLE;
			}

			// Get clipping for lights
			if (_pRenderInfo->m_pLightInfo && _pRenderInfo->m_MaxLights)
			{
				_pRenderInfo->m_nLights = 0;

//				CXR_LightOcclusionInfo* pLO = m_pWorldView->m_lLightOcclusion.GetBasePtr();

				const CBSP2_Link* pObjLinks = pSG->m_ObjectLights.GetLinks();
				int iLink = pSG->m_ObjectLights.GetLink_ID(_Elem);
				for(; iLink; iLink = pObjLinks[iLink].m_iLinkNextPL)
				{
					unsigned int iLight = pObjLinks[iLink].m_iPortalLeaf;
					if (iLight >= pSG->m_lLights.Len())
						continue;

					if (pSG->m_lLights[iLight].GetIntensitySqrf() < 0.001f)
						continue;

					if (_pRenderInfo->m_nLights >= _pRenderInfo->m_MaxLights)
						break;

					CXR_LightInfo& Info = _pRenderInfo->m_pLightInfo[_pRenderInfo->m_nLights];
					Info.m_pLight = &pSG->m_lLights[iLight];

					Info.m_ShadowScissor.SetRect(0xffff, 0x0000);
					if (!(Info.m_pLight->m_Flags & CXR_LIGHT_NOSHADOWS))
					{
						CBox3Dfp32 ShadowBox;
						CalcShadowBox(*Info.m_pLight, Box, ShadowBox);

						View_GetScissorWithPVS_Box_i(1, ShadowBox, Info.m_pLight->m_pPVS, &Info.m_ShadowScissor);

						if (Info.m_pLight->m_pPVS)
						{
							SceneGraph_PVSRelease(Info.m_pLight->m_pPVS);
						}
					}

					// Activate passive shadow volumes
					if (iLight < pSG->m_iFirstDynamic)
					{
						for(int iiVisPL = 0; iiVisPL < nVisPL; iiVisPL++)
						{
							int iPL = liVisPL[iiVisPL];
							int iSV = FindSV(m_spLightData, iPL, iLight);
							if (iSV)
							{
								m_pWorldView->m_lActiveSV[iSV >> 3] |= aShiftMulTab[iSV & 7];

	#ifndef PLATFORM_CONSOLE
								if (bDebug)
								{
									CXR_VBManager* pVBM = pCurrentEngine->GetVBM();
									const CBSP2_ShadowVolume& SV = m_spLightData->m_lSV[iSV];
									const CPlane3Dfp32* pPlanes = m_spLightData->m_lPlanes.GetBasePtr();
									if (SV.m_nPlaneLightBound)
									{
										CSolid Solid;
										for(int p = 0; p < SV.m_nPlaneLightBound; p++)
										{
											const CPlane3Dfp32& P(pPlanes[SV.m_iPlaneLightBound + p]);
											Solid.AddPlane(CPlane3Dfp64(P.n.Getfp64(), P.d));
										}

										CMat4Dfp32 Unit; Unit.Unit();
										Solid.UpdateMesh();
										Solid.RenderEdges(pVBM, &pCurrentEngine->GetVC()->m_W2VMat, 0x3f006f6f);
									}
									else
										pVBM->RenderBox(pCurrentEngine->GetVC()->m_W2VMat, Info.m_pLight->m_BoundBox, 0xff6f0000);
								}
	#endif
							}
						}

	//					pLO[iLight].m_ScissorVisible.Expand(Info.m_Scissor);
					}

					_pRenderInfo->m_nLights++;
				}
			}
		}
		else
		{
			// No shadows allowed
			_pRenderInfo->m_Flags |= CXR_RENDERINFO_NOSHADOWVOLUMES;
			_pRenderInfo->m_nLights = 0;

			if (_pRenderInfo->m_pLightInfo && _pRenderInfo->m_MaxLights)
			{
				const int nMaxLights = 32;
				uint16 liLights[nMaxLights];

				int nLights = pSG->EnumElementLights(&pSG->m_Objects, _Elem, liLights, nMaxLights);

				for(int i = 0; i < nLights; i++)
				{
					unsigned int iLight = liLights[i];
					if (iLight >= pSG->m_lLights.Len())
						continue;

					if (_pRenderInfo->m_nLights >= _pRenderInfo->m_MaxLights)
						break;

					CXR_LightInfo& Info = _pRenderInfo->m_pLightInfo[_pRenderInfo->m_nLights];
					Info.m_pLight = &pSG->m_lLights[iLight];
					Info.m_Scissor.SetRect(0, 0xffff);

					Info.m_ShadowScissor.SetRect(0xffff, 0x0000);

	/*				if (Info.m_pLight->m_Flags & CXR_LIGHT_NOSHADOWS)
					{
						Info.m_ShadowScissor.m_Min = 0xffff;
						Info.m_ShadowScissor.m_Max = 0;
					}
					else
					{
						m_Vis_Scissor.m_Min = 0xffff;
						m_Vis_Scissor.m_Max = 0;
						m_Vis_Scissor_pPVS = SceneGraph_PVSLock(0, Info.m_pLight->GetPosition());
						CalcShadowBox(*Info.m_pLight, Box, m_VisVBox);

						View_GetScissorWithPVS_Box_r(1);

						Info.m_ShadowScissor = m_Vis_Scissor;
						if (m_Vis_Scissor_pPVS)
						{
							SceneGraph_PVSRelease(m_Vis_Scissor_pPVS);
							m_Vis_Scissor_pPVS = NULL;
						}
					}*/

					_pRenderInfo->m_nLights++;
				}
			}
		}

		if (bDebug)
		{
			CXR_VBManager* pVBM = pCurrentEngine->GetVBM();
			CMat4Dfp32 Pos; Pos.Unit();
			CVec3Dfp32 Center;
			Box.GetCenter(Center);
			Center.SetRow(Pos, 3);

			pVBM->RenderBox(pCurrentEngine->GetVC()->m_W2VMat, Box, 0xff001f1f);

			for(int i = 0; i < _pRenderInfo->m_nLights; i++)
			{
				const CXR_Light* pL = _pRenderInfo->m_pLightInfo[i].m_pLight;
				pVBM->RenderWire(pCurrentEngine->GetVC()->m_W2VMat, Center, pL->GetPosition(), 0xff003f7f);

				CVec3Dfp32 LPos = pL->GetPosition();
				pVBM->RenderWire(pCurrentEngine->GetVC()->m_W2VMat, LPos - CVec3Dfp32(4,0,0), LPos + CVec3Dfp32(4,0,0), 0xff7f7f00);
				pVBM->RenderWire(pCurrentEngine->GetVC()->m_W2VMat, LPos - CVec3Dfp32(0,4,0), LPos + CVec3Dfp32(0,4,0), 0xff7f7f00);
				pVBM->RenderWire(pCurrentEngine->GetVC()->m_W2VMat, LPos - CVec3Dfp32(0,0,4), LPos + CVec3Dfp32(0,0,4), 0xff7f7f00);
			}
		}
	}
}

CXR_LightOcclusionInfo* CXR_Model_BSP2::View_Light_GetOcclusionInt(int _iLight)
{
	if (!m_pWorldView)
		return NULL;

	uint32 iL = _iLight;
	if (iL >= m_pWorldView->m_lLightOcclusion.Len())
		return NULL;
	else
		return &m_pWorldView->m_lLightOcclusion[iL];
}

CXR_LightOcclusionInfo* CXR_Model_BSP2::View_Light_GetOcclusionArray(int& _nLights)
{
	if (!m_pWorldView)
	{
		_nLights = 0;
		return NULL;
	}

	_nLights = m_pWorldView->m_lLightOcclusion.Len();
	return m_pWorldView->m_lLightOcclusion.GetBasePtr();
}

// Input is CRect2Duint16 Shaded, Shadow;
void CXR_Model_BSP2::View_Light_ApplyOcclusionArray_ShadowShaded(uint _nLights, const uint16* _piLights, const CScissorRect* _pScissors)
{
	M_LOCK(m_OcclusionLock);
	if(m_pWorldView)
	{
		uint nLights = Min(_nLights, (uint)m_pWorldView->m_lLightOcclusion.Len());
		CXR_LightOcclusionInfo* M_RESTRICT pD = m_pWorldView->m_lLightOcclusion.GetBasePtr();

		vec128 Msk = M_VConst_u16(0xffff, 0xffff, 0x0000, 0x0000, 0xffff, 0xffff, 0x0000, 0x0000);

		uint Lights = nLights >> 2;
		for(uint i = 0; i < Lights; i++)
		{
			uint16 iL0 = _piLights[0];
			uint16 iL1 = _piLights[1];
			uint16 iL2 = _piLights[2];
			uint16 iL3 = _piLights[3];
			_piLights	+= 4;

			vec128 as0 = M_VLd(_pScissors + 0);
			vec128 ad0 = M_VLd(&pD[iL0].m_ScissorShaded);
			vec128 as1 = M_VLd(_pScissors + 2);
			vec128 ad1 = M_VLd(&pD[iL1].m_ScissorShaded);
			vec128 as2 = M_VLd(_pScissors + 4);
			vec128 ad2 = M_VLd(&pD[iL2].m_ScissorShaded);
			vec128 as3 = M_VLd(_pScissors + 8);
			vec128 ad3 = M_VLd(&pD[iL3].m_ScissorShaded);
			_pScissors	+= 8;

			vec128 anew0 = M_VSelMsk(Msk, M_VMin_u16(as0, ad0), M_VMax_u16(as0, ad0));
			vec128 anew1 = M_VSelMsk(Msk, M_VMin_u16(as1, ad1), M_VMax_u16(as1, ad1));
			vec128 anew2 = M_VSelMsk(Msk, M_VMin_u16(as2, ad2), M_VMax_u16(as2, ad2));
			vec128 anew3 = M_VSelMsk(Msk, M_VMin_u16(as3, ad3), M_VMax_u16(as3, ad3));

			M_VSt(anew0, &pD[iL0].m_ScissorShaded);
			M_VSt(anew1, &pD[iL1].m_ScissorShaded);
			M_VSt(anew2, &pD[iL2].m_ScissorShaded);
			M_VSt(anew3, &pD[iL3].m_ScissorShaded);
		}

		{
			for(uint i = (Lights << 2); i < nLights; i++)
			{
				uint16 iL0 = _piLights[0];
				_piLights++;

				vec128 as0 = M_VLd(_pScissors + 0);
				vec128 ad0 = M_VLd(&pD[iL0].m_ScissorShaded);
				_pScissors	+= 2;

				vec128 anew0 = M_VSelMsk(Msk, M_VMin_u16(as0, ad0), M_VMax_u16(as0, ad0));

				M_VSt(anew0, &pD[iL0].m_ScissorShaded);
			}
		}
	}
}

void CXR_Model_BSP2::View_Light_ApplyOcclusionArray(int _nLights, const uint16* _piLights, const CXR_LightOcclusionInfo* _pLO)
{
	M_LOCK(m_OcclusionLock);
	if(m_pWorldView)
	{
		uint nLights = Min(_nLights, m_pWorldView->m_lLightOcclusion.Len());
		CXR_LightOcclusionInfo* M_RESTRICT pD = m_pWorldView->m_lLightOcclusion.GetBasePtr();
		const CXR_LightOcclusionInfo* M_RESTRICT pS = _pLO;

		vec128 Msk = M_VConst_u16(0xffff, 0xffff, 0x0000, 0x0000, 0xffff, 0xffff, 0x0000, 0x0000);

		uint Lights = nLights >> 2;
		if (_piLights)
		{
			for(uint i = 0; i < Lights; i++)
			{
				const uint16 iL0 = _piLights[0];
				const uint16 iL1 = _piLights[1];
				const uint16 iL2 = _piLights[2];
				const uint16 iL3 = _piLights[3];
				_piLights+= 4;

				vec128 bs0 = M_VLd(&pS[0].m_ScissorShaded);
				vec128 bd0 = M_VLd(&pD[iL0].m_ScissorShaded);
				vec128 as0 = M_VLd(&pS[0].m_ScissorVisible);
				vec128 ad0 = M_VLd(&pD[iL0].m_ScissorVisible);
				vec128 bs1 = M_VLd(&pS[1].m_ScissorShaded);
				vec128 bd1 = M_VLd(&pD[iL1].m_ScissorShaded);
				vec128 as1 = M_VLd(&pS[1].m_ScissorVisible);
				vec128 ad1 = M_VLd(&pD[iL1].m_ScissorVisible);
				vec128 bs2 = M_VLd(&pS[2].m_ScissorShaded);
				vec128 bd2 = M_VLd(&pD[iL2].m_ScissorShaded);
				vec128 as2 = M_VLd(&pS[2].m_ScissorVisible);
				vec128 ad2 = M_VLd(&pD[iL2].m_ScissorVisible);
				vec128 bs3 = M_VLd(&pS[3].m_ScissorShaded);
				vec128 bd3 = M_VLd(&pD[iL3].m_ScissorShaded);
				vec128 as3 = M_VLd(&pS[3].m_ScissorVisible);
				vec128 ad3 = M_VLd(&pD[iL3].m_ScissorVisible);
				pS+=4;

				vec128 bnew0 = M_VSelMsk(Msk, M_VMin_u16(bs0, bd0), M_VMax_u16(bs0, bd0));
				vec128 anew0 = M_VSelMsk(Msk, M_VMin_u16(as0, ad0), M_VMax_u16(as0, ad0));
				vec128 bnew1 = M_VSelMsk(Msk, M_VMin_u16(bs1, bd1), M_VMax_u16(bs1, bd1));
				vec128 anew1 = M_VSelMsk(Msk, M_VMin_u16(as1, ad1), M_VMax_u16(as1, ad1));
				vec128 bnew2 = M_VSelMsk(Msk, M_VMin_u16(bs2, bd2), M_VMax_u16(bs2, bd2));
				vec128 anew2 = M_VSelMsk(Msk, M_VMin_u16(as2, ad2), M_VMax_u16(as2, ad2));
				vec128 bnew3 = M_VSelMsk(Msk, M_VMin_u16(bs3, bd3), M_VMax_u16(bs3, bd3));
				vec128 anew3 = M_VSelMsk(Msk, M_VMin_u16(as3, ad3), M_VMax_u16(as3, ad3));

				M_VSt(bnew0, &pD[iL0].m_ScissorShaded);
				M_VSt(anew0, &pD[iL0].m_ScissorVisible);
				M_VSt(bnew1, &pD[iL1].m_ScissorShaded);
				M_VSt(anew1, &pD[iL1].m_ScissorVisible);
				M_VSt(bnew2, &pD[iL2].m_ScissorShaded);
				M_VSt(anew2, &pD[iL2].m_ScissorVisible);
				M_VSt(bnew3, &pD[iL3].m_ScissorShaded);
				M_VSt(anew3, &pD[iL3].m_ScissorVisible);
			}

			{
				for(uint i = (Lights << 2); i < nLights; i++)
				{
					const uint16 iL = _piLights[0];
					_piLights++;

					vec128 as0 = M_VLd(&pS[0].m_ScissorShaded);
					vec128 ad0 = M_VLd(&pD[iL].m_ScissorShaded);
					vec128 bs0 = M_VLd(&pS[0].m_ScissorVisible);
					vec128 bd0 = M_VLd(&pD[iL].m_ScissorVisible);
					pS++;

					vec128 anew0 = M_VSelMsk(Msk, M_VMin_u16(as0, ad0), M_VMax_u16(as0, ad0));
					vec128 bnew0 = M_VSelMsk(Msk, M_VMin_u16(bs0, bd0), M_VMax_u16(bs0, bd0));

					M_VSt(anew0, &pD[iL].m_ScissorShaded);
					M_VSt(bnew0, &pD[iL].m_ScissorVisible);
				}
			}
		}
		else
		{
			for(uint i = 0; i < Lights; i++)
			{
				vec128 bs0 = M_VLd(&pS[0].m_ScissorShaded);
				vec128 bd0 = M_VLd(&pD[0].m_ScissorShaded);
				vec128 as0 = M_VLd(&pS[0].m_ScissorVisible);
				vec128 ad0 = M_VLd(&pD[0].m_ScissorVisible);
				vec128 bs1 = M_VLd(&pS[1].m_ScissorShaded);
				vec128 bd1 = M_VLd(&pD[1].m_ScissorShaded);
				vec128 as1 = M_VLd(&pS[1].m_ScissorVisible);
				vec128 ad1 = M_VLd(&pD[1].m_ScissorVisible);
				vec128 bs2 = M_VLd(&pS[2].m_ScissorShaded);
				vec128 bd2 = M_VLd(&pD[2].m_ScissorShaded);
				vec128 as2 = M_VLd(&pS[2].m_ScissorVisible);
				vec128 ad2 = M_VLd(&pD[2].m_ScissorVisible);
				vec128 bs3 = M_VLd(&pS[3].m_ScissorShaded);
				vec128 bd3 = M_VLd(&pD[3].m_ScissorShaded);
				vec128 as3 = M_VLd(&pS[3].m_ScissorVisible);
				vec128 ad3 = M_VLd(&pD[3].m_ScissorVisible);

				vec128 bnew0 = M_VSelMsk(Msk, M_VMin_u16(bs0, bd0), M_VMax_u16(bs0, bd0));
				vec128 anew0 = M_VSelMsk(Msk, M_VMin_u16(as0, ad0), M_VMax_u16(as0, ad0));
				vec128 bnew1 = M_VSelMsk(Msk, M_VMin_u16(bs1, bd1), M_VMax_u16(bs1, bd1));
				vec128 anew1 = M_VSelMsk(Msk, M_VMin_u16(as1, ad1), M_VMax_u16(as1, ad1));
				vec128 bnew2 = M_VSelMsk(Msk, M_VMin_u16(bs2, bd2), M_VMax_u16(bs2, bd2));
				vec128 anew2 = M_VSelMsk(Msk, M_VMin_u16(as2, ad2), M_VMax_u16(as2, ad2));
				vec128 bnew3 = M_VSelMsk(Msk, M_VMin_u16(bs3, bd3), M_VMax_u16(bs3, bd3));
				vec128 anew3 = M_VSelMsk(Msk, M_VMin_u16(as3, ad3), M_VMax_u16(as3, ad3));

				M_VSt(bnew0, &pD[0].m_ScissorShaded);
				M_VSt(anew0, &pD[0].m_ScissorVisible);
				M_VSt(bnew1, &pD[1].m_ScissorShaded);
				M_VSt(anew1, &pD[1].m_ScissorVisible);
				M_VSt(bnew2, &pD[2].m_ScissorShaded);
				M_VSt(anew2, &pD[2].m_ScissorVisible);
				M_VSt(bnew3, &pD[3].m_ScissorShaded);
				M_VSt(anew3, &pD[3].m_ScissorVisible);

				pD += 4;
				pS += 4;
			}

			{
				uint32* pMaskedLO = (uint32*)pD;
				uint32* pSourceLO = (uint32*)pS;
				uint32* pDest = pMaskedLO;

				for(uint i = (Lights << 2); i < nLights; i++)
				{
					vec128 as0 = M_VLd(&pS->m_ScissorShaded);
					vec128 ad0 = M_VLd(&pD->m_ScissorShaded);
					vec128 bs0 = M_VLd(&pS->m_ScissorVisible);
					vec128 bd0 = M_VLd(&pD->m_ScissorVisible);

					vec128 anew0 = M_VSelMsk(Msk, M_VMin_u16(as0, ad0), M_VMax_u16(as0, ad0));
					vec128 bnew0 = M_VSelMsk(Msk, M_VMin_u16(bs0, bd0), M_VMax_u16(bs0, bd0));

					M_VSt(anew0, &pD->m_ScissorShaded);
					M_VSt(bnew0, &pD->m_ScissorVisible);

					pD++;
					pS++;
				}
			}
		}
	}
}

int CXR_Model_BSP2::View_Light_GetOcclusionSize()
{
	int nLights = 0;
	if(m_pWorldView)
	{
		nLights = m_pWorldView->m_lLightOcclusion.Len();
	}
	return nLights;
}

const uint16* CXR_Model_BSP2::View_Light_GetVisible(int& _nRetLights)
{
	if (!m_pWorldView)
	{
		_nRetLights = 0;
		return NULL;
	}

	M_LOCK(m_OcclusionLock);
	int nLights = m_pWorldView->m_lLightOcclusion.Len();
	m_pWorldView->m_liLightsUsed.SetLen(nLights);

	uint16* piUsed = m_pWorldView->m_liLightsUsed.GetBasePtr();
	CXR_LightOcclusionInfo* pLO = m_pWorldView->m_lLightOcclusion.GetBasePtr();
	
	int nUsed = 0;
	for(int i = 0; i < nLights; i++)
		if (pLO[i].IsVisible())
			piUsed[nUsed++] = i;

	_nRetLights = nUsed;
	return piUsed;
}

CXR_LightOcclusionInfo* CXR_Model_BSP2::View_Light_GetOcclusion(int _iLight)
{
	return View_Light_GetOcclusionInt(_iLight);
}

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Function:	Used to determine the partial visibility of a given volume (_Box)

	Parameters:
		_Box: The original volume.
		_pPVS: The PVS to be used for visibility.
		_VisBox: The visible box. (return value)

	Return:
		true: Box was visible
		false: Box is invisible.
\*____________________________________________________________________*/
bool CXR_Model_BSP2::View_GetVisibleBoxWithPVS_Box(const CBox3Dfp32& _Box, const uint8* _pPVS, CBox3Dfp32& _VisBox)
{
	CBox3Dfp32 DestBox;
	DestBox.m_Min = _FP32_MAX;
	DestBox.m_Max = -_FP32_MAX;
	View_GetVisibleBoxWithPVS_Box_r(1, &_Box, _pPVS, &DestBox);

	if (DestBox.m_Min[0] == _FP32_MAX)
		return false;
	else
	{
		_VisBox = DestBox;
		return true;
	}
}

/*void CXR_Model_BSP2::View_SetPortalState(int _iPortal, int _State)
{
	MAUTOSTRIP(CXR_Model_BSP_View_SetPortalState, NULL);
	if ((_iPortal < 0) || (_iPortal >= XW_MAXUSERPORTALS)) return;

	m_PortalIDState[_iPortal] = _State;

}*/

