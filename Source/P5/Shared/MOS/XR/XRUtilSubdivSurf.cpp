
#include "PCH.h"


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Subdivision surface functions.
					
	Author:			Magnus Högdahl
					
	Copyright:		Starbreeze Studios AB, 1998-2001
					
	Comments:		So far subdivision surfaces is pure research and should not
					be used in production. Please don't touch anything.

					
\*____________________________________________________________________________________________*/

#include "XRUtil.h"
#include "XRVertexBuffer.h"
#include "XRVBManager.h"
#include "MSIMD.h"
#include "MFloat.h"

#ifdef NEVER

CVec3Dfp32* ms_SubDiv_pV;
CVec3Dfp32* ms_SubDiv_pN;
CVec2Dfp32* ms_SubDiv_pTV[CRC_MAXTEXCOORDS];
CVec3Dfp32 ms_SubDiv_BaseNormal;
int ms_SubDiv_iVNext;
int ms_SubDiv_VMax;

uint16* ms_SubDiv_piPrim;
int ms_SubDiv_iPrim;
int ms_SubDiv_PrimMax;

CMat4Dfp32 ms_SubDiv_WMat;
CMat4Dfp32 ms_SubDiv_VMat;

int SubdivisionSurface_GetHeight(const CVec2Dfp32& _UV)
{
	MAUTOSTRIP(SubdivisionSurface_GetHeight, 0);
	return 0;
}


void SubdivisionSurface_Tesselate_r(int _iv0, int _iv1, int _iv2, int _Level, int _Flags)
{
	MAUTOSTRIP(SubdivisionSurface_Tesselate_r, MAUTOSTRIP_VOID);
	if (ms_SubDiv_iVNext-3 >= ms_SubDiv_VMax) return;

	int SMask = 0;

	CVec3Dfp32 dv0 = ms_SubDiv_pV[_iv1] - ms_SubDiv_pV[_iv0];
	CVec3Dfp32 dv1 = ms_SubDiv_pV[_iv2] - ms_SubDiv_pV[_iv1];
	CVec3Dfp32 dv2 = ms_SubDiv_pV[_iv0] - ms_SubDiv_pV[_iv2];
	fp32 l0 = dv0.Length();
	fp32 l1 = dv1.Length();
	fp32 l2 = dv2.Length();

	CVec3Dfp32 V0View = ms_SubDiv_pV[_iv0] * ms_SubDiv_WMat;
	CVec3Dfp32 V1View = ms_SubDiv_pV[_iv1] * ms_SubDiv_WMat;
	CVec3Dfp32 V2View = ms_SubDiv_pV[_iv2] * ms_SubDiv_WMat;

	CVec3Dfp32 N0View = ms_SubDiv_pN[_iv0];
	CVec3Dfp32 N1View = ms_SubDiv_pN[_iv1];
	CVec3Dfp32 N2View = ms_SubDiv_pN[_iv2];
	N0View.MultiplyMatrix3x3(ms_SubDiv_WMat);
	N1View.MultiplyMatrix3x3(ms_SubDiv_WMat);
	N2View.MultiplyMatrix3x3(ms_SubDiv_WMat);

	fp32 Dot0 = M_Fabs(N0View * V0View);
	fp32 Dot1 = M_Fabs(N1View * V1View);
	fp32 Dot2 = M_Fabs(N2View * V2View);
	Dot0 /= V0View.Length();
	Dot1 /= V1View.Length();
	Dot2 /= V2View.Length();

	fp32 Z0 = V0View.k[2];
	fp32 Z1 = V1View.k[2];
	fp32 Z2 = V2View.k[2];

	if (FloatIsNeg(Z0) && FloatIsNeg(Z1) && FloatIsNeg(Z2)) return;

	fp32 ZMinClamp = 64;
	fp32 TessMin = 2.0f;
	fp32 TessMax = 8.0f;

	fp32 TessK0 = TessMax + (TessMin - TessMax) * (Dot0 + Dot1) * 0.5f;
	fp32 TessK1 = TessMax + (TessMin - TessMax) * (Dot1 + Dot2) * 0.5f;
	fp32 TessK2 = TessMax + (TessMin - TessMax) * (Dot2 + Dot0) * 0.5f;

	fp32 MinLen = 32.0f;

	fp32 ZMin;
	ZMin = Min(Z0, Z1);
	fp32 k0 = TessK0*l0 / Max(ZMin, ZMinClamp);
	if (l0 > MinLen && k0 > 1.0f) SMask |= 1;

	ZMin = Min(Z1, Z2);
	fp32 k1 = TessK1*l1 / Max(ZMin, ZMinClamp);
	if (l1 > MinLen && k1 > 1.0f) SMask |= 2;

	ZMin = Min(Z2, Z0);
	fp32 k2 = TessK2*l2 / Max(ZMin, ZMinClamp);
	if (l2 > MinLen && k2 > 1.0f) SMask |= 4;

	if (_Level > 5) SMask = 0;

//SMask = 0;

	if (SMask)
	{
		int iva = ms_SubDiv_iVNext++;
		int ivb = ms_SubDiv_iVNext++;
		int ivc = ms_SubDiv_iVNext++;

		CVec3Dfp32* pV = ms_SubDiv_pV;
		CVec3Dfp32* pN = ms_SubDiv_pN;

		CVec3Dfp32& va = pV[iva];
		CVec3Dfp32& vb = pV[ivb];
		CVec3Dfp32& vc = pV[ivc];
		va = (ms_SubDiv_pV[_iv0] + ms_SubDiv_pV[_iv1]) * 0.5f;
		vb = (ms_SubDiv_pV[_iv1] + ms_SubDiv_pV[_iv2]) * 0.5f;
		vc = (ms_SubDiv_pV[_iv2] + ms_SubDiv_pV[_iv0]) * 0.5f;
		CVec3Dfp32 e0 = va - pV[_iv0];
		CVec3Dfp32 e1 = vb - pV[_iv1];
		CVec3Dfp32 e2 = vc - pV[_iv2];

		CVec3Dfp32& na = pN[iva];
		CVec3Dfp32& nb = pN[ivb];
		CVec3Dfp32& nc = pN[ivc];
		na = (pN[_iv0] + pN[_iv1]) * 0.5f;
		nb = (pN[_iv1] + pN[_iv2]) * 0.5f;
		nc = (pN[_iv2] + pN[_iv0]) * 0.5f;

		na.Combine(e0, -(na * e0) / e0.LengthSqr(), na);
		nb.Combine(e1, -(nb * e1) / e1.LengthSqr(), nb);
		nc.Combine(e2, -(nc * e2) / e2.LengthSqr(), nc);

		na.Normalize();
		nb.Normalize();
		nc.Normalize();

		if (ms_SubDiv_pTV[1])
		{
			CVec2Dfp32* pTV = ms_SubDiv_pTV[1];
			CVec2Dfp32& tva = pTV[iva];
			CVec2Dfp32& tvb = pTV[ivb];
			CVec2Dfp32& tvc = pTV[ivc];
			tva = (pTV[_iv0] + pTV[_iv1]) * 0.5f;
			tvb = (pTV[_iv1] + pTV[_iv2]) * 0.5f;
			tvc = (pTV[_iv2] + pTV[_iv0]) * 0.5f;
		}

		CVec2Dfp32* pTV = ms_SubDiv_pTV[0];
		CVec2Dfp32& tva = pTV[iva];
		CVec2Dfp32& tvb = pTV[ivb];
		CVec2Dfp32& tvc = pTV[ivc];
		tva = (pTV[_iv0] + pTV[_iv1]) * 0.5f;
		tvb = (pTV[_iv1] + pTV[_iv2]) * 0.5f;
		tvc = (pTV[_iv2] + pTV[_iv0]) * 0.5f;

		int tl0 = (tva.k[0]*133.0f + tva.k[1]*173.0f + 1277.0f);
		int tl1 = (tvb.k[0]*133.0f + tvb.k[1]*173.0f + 1277.0f);
		int tl2 = (tvc.k[0]*133.0f + tvc.k[1]*173.0f + 1277.0f);

/*		int tl0 = 63+31*(M_Sin(tva.k[0]*1.0f*_PI) + M_Sin(tva.k[1]*1.0f*_PI));
		int tl1 = 63+31*(M_Sin(tvb.k[0]*1.0f*_PI) + M_Sin(tvb.k[1]*1.0f*_PI));
		int tl2 = 63+31*(M_Sin(tvc.k[0]*1.0f*_PI) + M_Sin(tvc.k[1]*1.0f*_PI));
*/
/*		fp32 Amp0 = ((int(tl0+tl1) & 0x7f) - 0x3f) / 64.0f;
		fp32 Amp1 = ((int(tl1+tl2) & 0x7f) - 0x3f) / 64.0f;
		fp32 Amp2 = ((int(tl2+tl0) & 0x7f) - 0x3f) / 64.0f;

		fp32 Lev = _Level;
		fp32 LevelDiv = fp32(Lev + Sqr(Lev));

		const fp32 MaxAmp = 128.0f;
		Amp0 = MaxAmp/LevelDiv;
		Amp1 = MaxAmp/LevelDiv;
		Amp2 = MaxAmp/LevelDiv;*/

/*Amp0 = 0;
Amp1 = 0;
Amp2 = 0;*/

		SMask &= _Flags;

		const fp32 DisplaceK = 0.50f;

		const fp32 AmpEdgeFactor = 1.0f / 8.0f;

		// Subdivide...
		if (SMask & 1)
		{
			fp32 Amp0 = fp32((tl0 & 0x7f) - 64) / 64.0f;
			Amp0 *= l0 * AmpEdgeFactor;

			fp32 scale = (k0 < 2.0f) ? (k0 - 1.0f) : 1.0f;
			va += na * Amp0 * scale;
			
			fp32 d0 = -pN[_iv0] * e0;
			fp32 d1 = pN[_iv1] * e0;
			va += na * DisplaceK * scale * (d0 + d1) * 0.5f;
		}

		if (SMask & 2) 
		{
			fp32 Amp1 = fp32((tl1 & 0x7f) - 64) / 64.0f;
			Amp1 *= l1 * AmpEdgeFactor;
			fp32 scale = (k1 < 2.0f) ? (k1 - 1.0f) : 1.0f;
			vb += nb * Amp1 * scale;

			CVec3Dfp32 e1 = vb - pV[_iv1];
			fp32 d0 = -pN[_iv1] * e1;
			fp32 d1 = pN[_iv2] * e1;
			vb += nb * DisplaceK * scale * (d0 + d1) * 0.5f;
		}

		if (SMask & 4) 
		{
			fp32 Amp2 = fp32((tl2 & 0x7f) - 64) / 64.0f;
			Amp2 *= l2 * AmpEdgeFactor;
			fp32 scale = (k2 < 2.0f) ? (k2 - 1.0f) : 1.0f;
			vc += nc * Amp2 * scale;

			CVec3Dfp32 e2 = vc - pV[_iv2];
			fp32 d0 = -pN[_iv2] * e2;
			fp32 d1 = pN[_iv0] * e2;
			vc += nc * DisplaceK * scale * (d0 + d1) * 0.5f;
		}

		SubdivisionSurface_Tesselate_r(_iv0, iva, ivc, _Level+1, _Flags | 2 | (SMask & 3));
		SubdivisionSurface_Tesselate_r(iva, _iv1, ivb, _Level+1, _Flags | 4 | (SMask & 5));
		SubdivisionSurface_Tesselate_r(ivc, ivb, _iv2, _Level+1, _Flags | 1 | (SMask & 6));
		SubdivisionSurface_Tesselate_r(iva, ivb, ivc, _Level+1, _Flags | 7);
		return;
	}
	else
	{
		if (ms_SubDiv_iPrim-3 >= ms_SubDiv_PrimMax) return;

		ms_SubDiv_piPrim[ms_SubDiv_iPrim++] = _iv0;
		ms_SubDiv_piPrim[ms_SubDiv_iPrim++] = _iv1;
		ms_SubDiv_piPrim[ms_SubDiv_iPrim++] = _iv2;
	}
}

void SubdivisionSurface_Init(const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, int _nV, int _nMaxV, uint16* _piPrim, int _MaxPrim, CVec3Dfp32* _pV, CVec3Dfp32* _pN, CVec2Dfp32* _pTV0, CVec2Dfp32* _pTV1, CVec2Dfp32* _pTV2, CVec2Dfp32* _pTV3, CImage* _pDisplacementMap)
{
	MAUTOSTRIP(SubdivisionSurface_Init, MAUTOSTRIP_VOID);
	ms_SubDiv_pV = _pV;
	ms_SubDiv_pN = _pN;
	ms_SubDiv_pTV[0] = _pTV0;
	ms_SubDiv_pTV[1] = _pTV1;
	ms_SubDiv_pTV[2] = _pTV2;
	ms_SubDiv_pTV[3] = _pTV3;
	ms_SubDiv_iVNext = _nV;
	ms_SubDiv_VMax = _nMaxV;

	ms_SubDiv_piPrim = _piPrim;
	ms_SubDiv_iPrim = 0;
	ms_SubDiv_PrimMax = _MaxPrim;

	ms_SubDiv_WMat = _WMat;
	ms_SubDiv_VMat = _VMat;
}

void SubdivisionSurface_TesselateTriangle(int _iv0, int _iv1, int _iv2)
{
	MAUTOSTRIP(SubdivisionSurface_TesselateTriangle, MAUTOSTRIP_VOID);
	CPlane3Dfp32 Plane(ms_SubDiv_pV[_iv0], ms_SubDiv_pV[_iv1], ms_SubDiv_pV[_iv2]);
	ms_SubDiv_BaseNormal = Plane.n;
	ms_SubDiv_BaseNormal.Normalize();

	SubdivisionSurface_Tesselate_r(_iv0, _iv1, _iv2, 1, -1);
}

void SubdivisionSurface_TesselateIndexedPrimitives(CXR_VertexBuffer* _pVB, uint16* _pPrim, int _nP)
{
	MAUTOSTRIP(SubdivisionSurface_TesselateIndexedPrimitives, MAUTOSTRIP_VOID);
	CRCPrimStreamIterator StreamIterate(_pPrim, _nP);
	
	if (!StreamIterate.IsValid())
		return;		

	while(1)
	{
		const uint16 *_pPrim = StreamIterate.GetCurrentPointer();

		switch(StreamIterate.GetCurrentType())
		{
		case CRC_RIP_TRIFAN :
			{
				int nTri = (*_pPrim)-2;
				uint16* piV = _pPrim+1;
				int iv0 = *piV; piV++;
				int iv1 = *piV; piV++;

				for(int i = 0; i < nTri; i++)
				{
					int iv2 = *piV; piV++;

					int iVBase = iTri*3;
					ms_SubDiv_pV[iVBase] = _pVB->m_pV[iv0];
					ms_SubDiv_pV[iVBase+1] = _pVB->m_pV[iv1];
					ms_SubDiv_pV[iVBase+2] = _pVB->m_pV[iv2];


					ms_SubDiv_pN[iVBase] = _pVB->m_pN[iv0];
					ms_SubDiv_pN[iVBase+1] = _pVB->m_pN[iv1];
					ms_SubDiv_pN[iVBase+2] = _pVB->m_pN[iv2];

/*				ConOut("N0: " + _pVB->m_pN[iv0].GetString());
				ConOut("N1: " + _pVB->m_pN[iv1].GetString());
				ConOut("N2: " + _pVB->m_pN[iv2].GetString());*/

//					if (_pVB->m_pTV[0])
					{
						CVec2Dfp32* pTV = _pVB->m_pTV[0];
						ms_SubDiv_pTV[0][iVBase] = pTV[iv0];
						ms_SubDiv_pTV[0][iVBase+1] = pTV[iv1];
						ms_SubDiv_pTV[0][iVBase+2] = pTV[iv2];
					}

					if (_pVB->m_pTV[1])
					{
						CVec2Dfp32* pTV = _pVB->m_pTV[1];
						ms_SubDiv_pTV[1][iVBase] = pTV[iv0];
						ms_SubDiv_pTV[1][iVBase+1] = pTV[iv1];
						ms_SubDiv_pTV[1][iVBase+2] = pTV[iv2];
					}

					SubdivisionSurface_TesselateTriangle(iVBase, iVBase+1, iVBase+2);
					iv1 = iv2;
					iTri++;
				}
			}
			break;

		case CRC_RIP_TRISTRIP :
		case CRC_RIP_POLYGON :
		case CRC_RIP_QUADSTRIP:
			{
			}
			break;
		case CRC_RIP_TRIANGLES :
			{
			}
			break;
		default :
			{
				Error_static("::SubdivisionSurface_TesselateIndexedPrimitives", CStrF("Unsupported primitive: %d", StreamIterate.GetCurrentType()));
				break;
			}
		}

		if(!StreamIterate.Next())
			break;
	}
}

int GetRIPTriangleCount(uint16* _pPrim, int _nPrim)
{
	MAUTOSTRIP(GetRIPTriangleCount, 0);
	int iP = 0;
	int nTri = 0;

	CRCPrimStreamIterator StreamIterate(_pPrim, _nPrim);
	
	if (!StreamIterate.IsValid())
		return;		
	
	while(1)
	{

		const uint16* _pPrim = StreamIterate.GetCurrentPointer();

		switch(StreamIterate.GetCurrentType())
		{
		case CRC_RIP_TRIFAN :
		case CRC_RIP_TRISTRIP :
		case CRC_RIP_POLYGON :
		case CRC_RIP_QUADSTRIP:
			{
				nTri += (*_pPrim)-2;
			}
			break;
		case CRC_RIP_TRIANGLES :
			{
				nTri += (*_pPrim);
			}
			break;
		default :
			{
				Error_static("GetRIPTriangleCount", CStrF("Unsupported primitive: %d", StreamIterate.GetCurrentType()));
				break;
			}
		}

		if (!StreamIterate.Next())
			break;

	}

	return nTri;
}

CXR_VertexBuffer* SubdivisionSurface_TesselateVB(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
{
	MAUTOSTRIP(SubdivisionSurface_TesselateVB, NULL);
ConOut(CStr("(SubdivisionSurface_TesselateVB) !"));

	CXR_VertexBuffer* pVBNew = _pVBM->Alloc_VB();
	if (!pVBNew) return NULL;
	*pVBNew = *_pVB;	// We're lazy, so we copy everything..

	if (pVBNew->m_pNextVB) ConOut(CStr("(SubdivisionSurface_TesselateVB) WARNING, Can't handle chains!"));

	switch(_pVB->m_PrimType)
	{
	case CRC_RIP_STREAM :
		{
			int nTri = GetRIPTriangleCount(_pVB->m_piPrim, _pVB->m_nPrim);
			int nTessTri = (nTri*200+6000);
			int nV = nTessTri;

		ConOut(CStrF("(SubdivisionSurface_TesselateVB) nTri %d, nTessTri %d, nV %d, nPrim %d", nTri, nTessTri, nV, nTessTri*3));

			CVec3Dfp32* pV = _pVBM->Alloc_V3(nV);
			CVec3Dfp32* pN = _pVBM->Alloc_V3(nV);
			CVec2Dfp32* pTV0 = _pVBM->Alloc_V2(nV);
			CVec2Dfp32* pTV1 = _pVBM->Alloc_V2(nV);
			uint16* piPrim = _pVBM->Alloc_Int16(nTessTri*3);
			if (!pV || !pN || !pTV0 || !pTV1 || !piPrim) return NULL;

			pVBNew->m_pTV[0] = pTV0;
			pVBNew->m_pTV[1] = pTV1;
			pVBNew->m_pN = pN;
			pVBNew->m_pCol = NULL;
			pVBNew->m_pSpec = NULL;

			SubdivisionSurface_Init(_WMat, _VMat, nTri*3, nV, piPrim, nTessTri*3, pV, pN, pTV0, pTV1, NULL, NULL, NULL);
			SubdivisionSurface_TesselateIndexedPrimitives(_pVB, _pVB->m_piPrim, _pVB->m_nPrim);

		ConOut(CStrF("(SubdivisionSurface_TesselateVB) iVNext %d, iPrim %d", ms_SubDiv_iVNext, ms_SubDiv_iPrim));

			if (!pVBNew->SetVBChain(_pVBM, false))
				return NULL;

			pVBNew->Geometry_VertexArray(pV, ms_SubDiv_iVNext);
			pVBNew->Render_IndexedTriangles(piPrim, ms_SubDiv_iPrim / 3);
		}
		break;

	default :
		return _pVB;
	}

	return pVBNew;
}



#ifdef NEVER

void ___(const CVec3Dfp32& _Normal, const CVec3Dfp32& _v0, const CVec3Dfp32& _v1, const CVec3Dfp32& _v2,
	const CVec2Dfp32& _tv0, const CVec2Dfp32& _tv1, const CVec2Dfp32& _tv2,
	int _Level, fp32 _Area, int _Flags, int _rnd0, int _rnd1, int _rnd2)
{
	MAUTOSTRIP(___, MAUTOSTRIP_VOID);
C	int SMask = 0;

	CVec3Dfp32 dv0 = _v1 - _v0;
	CVec3Dfp32 dv1 = _v2 - _v1;
	CVec3Dfp32 dv2 = _v0 - _v2;
	fp32 l0 = dv0.Length();
	fp32 l1 = dv1.Length();
	fp32 l2 = dv2.Length();

	fp32 ZMin;
	ZMin = Min(_v0.k[2], _v1.k[2]);
	fp32 k0 = 2.0f*l0 / Max(ZMin, 32.0f);
	if (k0 > 1.0f) SMask |= 1;
	ZMin = Min(_v1.k[2], _v2.k[2]);
	fp32 k1 = 2.0f*l1 / Max(ZMin, 32.0f);
	if (k1 > 1.0f) SMask |= 2;
	ZMin = Min(_v2.k[2], _v0.k[2]);
	fp32 k2 = 2.0f*l2 / Max(ZMin, 32.0f);
	if (k2 > 1.0f) SMask |= 4;

	if (_Level > 5) SMask = 0;

	if (SMask)
	{
		CVec2Dfp32 tva = (_tv0 + _tv1) * 0.5f;
		CVec2Dfp32 tvb = (_tv1 + _tv2) * 0.5f;
		CVec2Dfp32 tvc = (_tv2 + _tv0) * 0.5f;

		int tl0 = (tva.k[0]*133.0f + tva.k[1]*173.0f + 1277.0f);
		int tl1 = (tvb.k[0]*133.0f + tvb.k[1]*173.0f + 1277.0f);
		int tl2 = (tvc.k[0]*133.0f + tvc.k[1]*173.0f + 1277.0f);

/*		int tl0 = 63+31*(M_Sin(tva.k[0]*1.0f*_PI) + M_Sin(tva.k[1]*1.0f*_PI));
		int tl1 = 63+31*(M_Sin(tvb.k[0]*1.0f*_PI) + M_Sin(tvb.k[1]*1.0f*_PI));
		int tl2 = 63+31*(M_Sin(tvc.k[0]*1.0f*_PI) + M_Sin(tvc.k[1]*1.0f*_PI));
*/
		fp32 Amp0 = ((RoundToInt(tl0+tl1) & 0x7f) - 0x3f) / 64.0f;
		fp32 Amp1 = ((RoundToInt(tl1+tl2) & 0x7f) - 0x3f) / 64.0f;
		fp32 Amp2 = ((RoundToInt(tl2+tl0) & 0x7f) - 0x3f) / 64.0f;
		fp32 Lev = _Level;
		fp32 LevelDiv = fp32(Lev + Sqr(Lev));

		Amp0 = 64.0f/LevelDiv;
		Amp1 = 64.0f/LevelDiv;
		Amp2 = 64.0f/LevelDiv;

		_rnd0 = 0;
		_rnd1 = 0;
		_rnd2 = 0;

		SMask &= _Flags;

		// Subdivide...
		CVec3Dfp32 va = (_v0 + _v1) * 0.5f;
		if (SMask & 1)
		{
			Amp0 *= fp32((tl0 & 0x7f) - 64) / 64.0f;
			if (k0 < 2.0f) Amp0 *= (k0 - 1.0f);
//			if (k0 < 1.5f) Amp0 *= 2.0f*(k0 - 1.0f);
//			if (k0 < 3.0f) Amp0 *= 0.5f*(k0 - 1.0f);
			va += _Normal * Amp0;
		}

		CVec3Dfp32 vb = (_v1 + _v2) * 0.5f;
		if (SMask & 2) 
		{
			Amp1 *= fp32((tl1 & 0x7f) - 64) / 64.0f;
			if (k1 < 2.0f) Amp1 *= (k1 - 1.0f);
//			if (k1 < 1.5f) Amp1 *= 2.0f*(k1 - 1.0f);
//			if (k1 < 3.0f) Amp1 *= 0.5f*(k1 - 1.0f);
			vb += _Normal * Amp1;
		}

		CVec3Dfp32 vc = (_v2 + _v0) * 0.5f;
		if (SMask & 4) 
		{
			Amp2 *= fp32((tl2 & 0x7f) - 64) / 64.0f;
			if (k2 < 2.0f) Amp2 *= (k2 - 1.0f);
//			if (k2 < 1.5f) Amp2 *= 2.0f*(k2 - 1.0f);
//			if (k2 < 3.0f) Amp2 *= 0.5f*(k2 - 1.0f);
			vc += _Normal * Amp2;
		}

		_Area *= 0.25f;
		Render_FractalBumpedTriangle(_Normal, _v0, va, vc, _tv0, tva, tvc, _Level+1, _Area, _Flags | 2 | (SMask & 3),	// 3
			_rnd0, _rnd0 + _rnd2, _rnd2);
		Render_FractalBumpedTriangle(_Normal, va, _v1, vb, tva, _tv1, tvb, _Level+1, _Area, _Flags | 4 | (SMask & 5),	// 5
			_rnd0, _rnd1, _rnd0 + _rnd0);
		Render_FractalBumpedTriangle(_Normal, vc, vb, _v2, tvc, tvb, _tv2, _Level+1, _Area, _Flags | 1 | (SMask & 6),	// 6
			_rnd1 + _rnd2, _rnd1, _rnd2);
		Render_FractalBumpedTriangle(_Normal, va, vb, vc, tva, tvb, tvc, _Level+1, _Area, _Flags | 7,
			_rnd0 + _rnd1, _rnd1 + _rnd2, _rnd0 + _rnd2);
		return;
	}
	else
	{
		if (_Flags & 0x8000)
		{
			int _Col = 0;
			if (_Flags & 1) _Col |= 0xff0000;
			if (_Flags & 2) _Col |= 0xff00;
			if (_Flags & 4) _Col |= 0xff;

			RenderWire(_v0, _v1, _Col);
			RenderWire(_v1, _v2, _Col);
			RenderWire(_v2, _v0, _Col);
			return;
		}

		m_nPolygons++;
		m_nTriTotal++;

		glBegin(GL_TRIANGLES);

		glTexCoord2f(_tv0.k[0], _tv0.k[1]);
		glVertex3f(_v0.k[0], _v0.k[1], _v0.k[2]);

		glTexCoord2f(_tv1.k[0], _tv1.k[1]);
		glVertex3f(_v1.k[0], _v1.k[1], _v1.k[2]);

		glTexCoord2f(_tv2.k[0], _tv2.k[1]);
		glVertex3f(_v2.k[0], _v2.k[1], _v2.k[2]);

		glEnd();
	}
}
#endif

#endif
