#include "PCH.h"

#include "../Platform/Platform.h"
#include "MFloat.h"
#include "WBSPModel.h"
#include "WBSPDef.h"

//#include "../../XR/XRVBPrior.h"
#include "../../XR/XREngineVar.h"

// -------------------------------------------------------------------
//  Stuff in MImage.cpp
// -------------------------------------------------------------------
void PPA_Mul_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);
void PPA_MulAddSaturate_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

void PPA_Mul_RGBA32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

// -------------------------------------------------------------------
int g_nFogTraceNode = 0;
int g_nFogTraceLeaf = 0;
int g_nFogRays = 0;
int g_nFogRayLamps = 0;
int g_nLightIterSave = 0;
int g_nLightIter = 0;

CPixel32 Fog_EvalLight(const CXR_Light* _pL, const CVec3Dfp32& _p0, int _Alpha);
CPixel32 Fog_EvalLight(const CXR_Light* _pL, const CVec3Dfp32& _p0, int _Alpha)
{
	MAUTOSTRIP(Fog_EvalLight, CPixel32());
	int R,G,B;
	R = G = B = 0;

	fp32 IntensSqr = _pL->GetDistanceSqr(_p0);

	if (IntensSqr < Sqr(_pL->m_Range))
	{
		CPixel32 I32;
		M_VSt_V4f32_Pixel32(M_VMul(M_VScalar(255.0f), _pL->GetIntensityv().v), (uint32*)&I32);
		int Intens = RoundToInt((1.0f - M_Sqrt(IntensSqr) * _pL->m_RangeInv) * 255.0f);
		R += Intens*I32.GetR();
		G += Intens*I32.GetG();
		B += Intens*I32.GetB();
	}

	return 
		(Min(R >> 8, 255) << 16) +
		(Min(G >> 8, 255) << 8) +
		(Min(B >> 8, 255) << 0) +
		(_Alpha << 24);
}

// -------------------------------------------------------------------
//  CBSP_FogBox
// -------------------------------------------------------------------

CBSP_FogBox::CBSP_FogBox()
{
	MAUTOSTRIP(CBSP_FogBox_ctor, MAUTOSTRIP_VOID);
	m_BoxOrigin = 0;
	m_BoxScale = 0;
	m_BoxX = 0;
	m_BoxY = 0;
	m_BoxZ = 0;
	m_BoxModY = 0;
	m_BoxModZ = 0;
	m_Flags = 0;
	m_UpdateCount = 1;
}

void CBSP_FogBox::Create(const CBox3Dfp32& _Box, const CVec3Dfp32& _CellSize)
{
	MAUTOSTRIP(CBSP_FogBox_Create, MAUTOSTRIP_VOID);
	CVec3Dfp32 Ext;
	_Box.m_Max.Sub(_Box.m_Min, Ext);
	CVec3Dfp32 CellSizeInv(1.0f / _CellSize[0], 1.0f / _CellSize[1], 1.0f / _CellSize[2]);
	Ext += _CellSize*2.0f;
	Ext.CompMul(CellSizeInv, Ext);
	Ext += 1.0f;
	m_BoxOrigin = _Box.m_Min;
	m_BoxOrigin -= _CellSize;
	m_BoxScale = _CellSize;
	m_BoxScaleInv = CellSizeInv;
	m_BoxX = (uint32)Ext.k[0];
	m_BoxY = (uint32)Ext.k[1];
	m_BoxZ = (uint32)Ext.k[2];
	m_BoxModY = m_BoxX;
	m_BoxModZ = m_BoxY*m_BoxX;
	m_lBox.SetLen(m_BoxX * m_BoxY * m_BoxZ);
	m_MaxIndex = m_lBox.Len() - (1+m_BoxModY+m_BoxModZ);
	m_Flags = 0;

//ConOutL(CStrF("(CBSP_FogBox::Create) %d, %d, %d", m_BoxX, m_BoxY, m_BoxZ));
}

void CBSP_FogBox::Create(const CBSP_LightVolumeInfo& _LightVol)
{
	MAUTOSTRIP(CBSP_FogBox_Create_2, MAUTOSTRIP_VOID);
	m_BoxOrigin = _LightVol.m_VolumeSize.m_Min;
	m_BoxScale = _LightVol.m_CellSize;
	m_BoxScaleInv = _LightVol.m_CellSizeInv;
	m_BoxX = _LightVol.m_GridSize[0];
	m_BoxY = _LightVol.m_GridSize[1];
	m_BoxZ = _LightVol.m_GridSize[2];
	m_BoxModY = m_BoxX;
	m_BoxModZ = m_BoxY*m_BoxX;
	m_lBox.SetLen(m_BoxX * m_BoxY * m_BoxZ);
	m_MaxIndex = m_lBox.Len() - (1+m_BoxModY+m_BoxModZ);
	m_Flags = 0;

//ConOutL(CStrF("(CBSP_FogBox::Create) Create from lightvolume, %d, %d, %d", m_BoxX, m_BoxY, m_BoxZ));
}

void CBSP_FogBox::Clear()
{
	MAUTOSTRIP(CBSP_FogBox_Clear, MAUTOSTRIP_VOID);
	FillChar(m_lBox.GetBasePtr(), m_lBox.ListSize(), 0);
}

void CBSP_FogBox::AddLight(const CXR_Light* _pL)
{
	MAUTOSTRIP(CBSP_FogBox_AddLight, MAUTOSTRIP_VOID);
	for(int z = 0; z < m_BoxZ; z++)
		for(int y = 0; y < m_BoxY; y++)
			for(int x = 0; x < m_BoxX; x++)
			{
				CVec3Dfp32 V(x, y, z);
				V.CompMul(m_BoxScale, V);
				V += m_BoxOrigin;
				CPixel32 C(m_lBox[GetIndex(x,y,z)]);
				CPixel32 C2(::Fog_EvalLight(_pL, V, 255));
				m_lBox[GetIndex(x,y,z)] = CPixel32(
					C.GetR() + C2.GetR(), 
					C.GetG() + C2.GetG(), 
					C.GetB() + C2.GetB());

//				m_lBox[GetIndex(x,y,z)] += Fog_EvalLight(_pL, V, 255);
			}
}

void CBSP_FogBox::SetAlphaFromBrightness()
{
	MAUTOSTRIP(CBSP_FogBox_SetAlphaFromBrightness, MAUTOSTRIP_VOID);
	int nc = m_lBox.Len();
	CPixel32* pC = m_lBox.GetBasePtr();
	for(int i = 0; i < nc; i++)
		pC[i] = (pC[i] & 0xffffff) + (Min(255, ((pC[i].GetR() + 2*pC[i].GetG() + pC[i].GetB()) >> 0) + 0x40) << 24);
//		pC[i] = (pC[i] & 0xffffff) + (((pC[i].GetR() + 2*pC[i].GetG() + pC[i].GetB()) >> 3) << 24) + 0x80000000;
//		(int&)pC[i] |= 0xff000000;
}


/*CPixel32 CBSP_FogBox::Filter(const CPixel32* pC, int x, int y, int z)
{
	MAUTOSTRIP(CBSP_FogBox_Filter, NULL);
}
*/

void CBSP_FogBox::Filter()
{
	MAUTOSTRIP(CBSP_FogBox_Filter_2, MAUTOSTRIP_VOID);
	CPixel32 Box[2048];
	if (m_lBox.Len() > 2048) return;

	{
		int bmy = m_BoxModY;
		int bmz = m_BoxModZ;
		const CPixel32* pC = m_lBox.GetBasePtr();
		for(int z = 0; z < m_BoxZ; z++)
			for(int y = 0; y < m_BoxY; y++)
				for(int x = 0; x < m_BoxX; x++)
/*		for(int z = 1; z < m_BoxZ-1; z++)
			for(int y = 1; y < m_BoxY-1; y++)
				for(int x = 1; x < m_BoxX-1; x++)*/
				{
					int iC = GetIndex(x, y, z);
					CPixel32 c00 = pC[iC];
/*					CPixel32 cx0 = pC[iC - 1];
					CPixel32 cx1 = pC[iC + 1];
					CPixel32 cy0 = pC[iC - bmy];
					CPixel32 cy1 = pC[iC + bmy];
					CPixel32 cz0 = pC[iC - bmz];
					CPixel32 cz1 = pC[iC + bmz];*/
					CPixel32 cx0 = pC[iC - ((x > 0) ? 1 : -1)];
					CPixel32 cx1 = pC[iC + ((x < m_BoxX-1) ? 1 : -1)];
					CPixel32 cy0 = pC[iC - ((y > 0) ? bmy : -bmy)];
					CPixel32 cy1 = pC[iC + ((y < m_BoxY-1) ? bmy : -bmy)];
					CPixel32 cz0 = pC[iC - ((z > 0) ? bmz : -bmz)];
					CPixel32 cz1 = pC[iC + ((z < m_BoxZ-1) ? bmz : -bmz)];

					int r = (c00.GetR() * 2 + 
						cx0.GetR() + cx1.GetR() + 
						cy0.GetR() + cy1.GetR() + 
						cz0.GetR() + cz1.GetR()) >> 3;
					int g = (c00.GetG() * 2 + 
						cx0.GetG() + cx1.GetG() + 
						cy0.GetG() + cy1.GetG() + 
						cz0.GetG() + cz1.GetG()) >> 3;
					int b = (c00.GetB() * 2 + 
						cx0.GetB() + cx1.GetB() + 
						cy0.GetB() + cy1.GetB() + 
						cz0.GetB() + cz1.GetB()) >> 3;
					int a = (c00.GetA() * 2 + 
						cx0.GetA() + cx1.GetA() + 
						cy0.GetA() + cy1.GetA() + 
						cz0.GetA() + cz1.GetA()) >> 3;

					Box[iC] = (a << 24) + (r << 16) + (g << 8) + b;
				}
	}

	{
		CPixel32* pC = m_lBox.GetBasePtr();
/*		for(int z = 1; z < m_BoxZ-1; z++)
			for(int y = 1; y < m_BoxY-1; y++)
				for(int x = 1; x < m_BoxX-1; x++)*/
		for(int z = 0; z < m_BoxZ; z++)
			for(int y = 0; y < m_BoxY; y++)
				for(int x = 0; x < m_BoxX; x++)
				{
					int iC = GetIndex(x, y, z);
					pC[iC] = Box[iC];
				}
	}
}

CPixel32 CBSP_FogBox::GetFog(const CVec3Dfp32& _v, int _Alpha) const
{
	MAUTOSTRIP(CBSP_FogBox_GetFog, CPixel32());
	CVec3Dfp32 v;
	_v.Sub(m_BoxOrigin, v);
	uint32 x = RoundToInt(v.k[0]*m_BoxScaleInv.k[0] - 0.5f); if (x >= m_BoxX-1) x = 0;
	uint32 y = RoundToInt(v.k[1]*m_BoxScaleInv.k[1] - 0.5f); if (y >= m_BoxY-1) y = 0;
	uint32 z = RoundToInt(v.k[2]*m_BoxScaleInv.k[2] - 0.5f); if (z >= m_BoxZ-1) z = 0;
	int fx = RoundToInt(256.0f * (v.k[0] - fp32(x)*m_BoxScale.k[0]) * m_BoxScaleInv.k[0]);
	int fy = RoundToInt(256.0f * (v.k[1] - fp32(y)*m_BoxScale.k[1]) * m_BoxScaleInv.k[1]);
	int fz = RoundToInt(256.0f * (v.k[2] - fp32(z)*m_BoxScale.k[2]) * m_BoxScaleInv.k[2]);

	const CPixel32* pC = m_lBox.GetBasePtr();
	int i = GetIndex(x, y, z);
	uint32 Col = CPixel32::TrilinearRGBA(
		pC[i], pC[i+1], pC[i+m_BoxModY], pC[i+1+m_BoxModY], 
		pC[i+m_BoxModZ], pC[i+1+m_BoxModZ], pC[i+m_BoxModY+m_BoxModZ], pC[i+1+m_BoxModY+m_BoxModZ],
		fx, fy, fz);

	int Alpha = ((Col >> 24) * _Alpha) >> 8;
	return (Col & 0xffffff) + (Alpha << 24);
}

//static fp32 g_FogTime = 0.0f;

CPixel32 CBSP_FogBox::GetFog_BoxSpace(const CVec3Dfp32& _v, int _Alpha) const
{
	MAUTOSTRIP(CBSP_FogBox_GetFog_BoxSpace, CPixel32());
	CVec3Dint iv;
	iv.k[0] = RoundToInt(_v.k[0]);
	iv.k[1] = RoundToInt(_v.k[1]);
	iv.k[2] = RoundToInt(_v.k[2]);
//	RoundToInt(&_v.k[0], &iv.k[0], 3);

	int fx = iv.k[0] & 255;
	int fy = iv.k[1] & 255;
	int fz = iv.k[2] & 255;

	uint32 i = GetIndex(iv.k[0] >> 8, iv.k[1] >> 8, iv.k[2] >> 8);
	if (i >= m_MaxIndex) i = 0;
	const CPixel32* pC = m_lBox.GetBasePtr();
	uint32 Col = CPixel32::TrilinearRGBA(
		pC[i], pC[i+1], pC[i+m_BoxModY], pC[i+1+m_BoxModY], 
		pC[i+m_BoxModZ], pC[i+1+m_BoxModZ], pC[i+m_BoxModY+m_BoxModZ], pC[i+1+m_BoxModY+m_BoxModZ],
		fx, fy, fz);

/*	fp32 t = g_FogTime*5.0f;
	fp32 Density = 
		(cos(t + (_v[0]*_PI*2.0 + -_v[1]*_PI*3.0 + _v[2]*_PI*4.0) / 384.0f) +
		sin(t*0.7f + (-_v[0]*_PI*4.0 + _v[1]*_PI*2.0 + -_v[2]*_PI*3.0) / 384.0f) + 2.0f) * 0.125f + 0.5f;

	_Alpha = (_Alpha * RoundToInt(Density*256.0f)) >> 8;*/

	int Alpha = ((Col >> 24) * _Alpha) >> 8;
	return (Col & 0xffffff) + (Alpha << 24);
}

void CBSP_FogBox::GetFogBoxVectors(const CVec3Dfp32& _p0, const CVec3Dfp32& _v, CVec3Dfp32& _fp0, CVec3Dfp32& _fv) const
{
	MAUTOSTRIP(CBSP_FogBox_GetFogBoxVectors, MAUTOSTRIP_VOID);
	CVec3Dfp32 p;
	_p0.Sub(m_BoxOrigin, p);
	fp32 Scale = 256.0f * m_BoxScaleInv.k[0];
	p.Scale(Scale, _fp0);
	_v.Scale(Scale, _fv);
}

// -------------------------------------------------------------------
//  CXR_Model_BSP::Fog_...
// -------------------------------------------------------------------
void CXR_Model_BSP::Fog_InitFogBox(int _iPL)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_InitFogBox, MAUTOSTRIP_VOID);
	CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[_iPL];
	const CBSP_LightVolumeInfo* pLightVol = (pPL->m_iLightVolume) ? &m_lLightVolumeInfo[pPL->m_iLightVolume] : NULL;
	if (!pLightVol) return;

//	const CXR_MediumDesc* pMedium = &m_lMediums[pPL->m_iMedium];

	spCBSP_FogBox spBox = MNew(CBSP_FogBox);
	if (!spBox) MemError("Fog_InitFogBox");

	spBox->Create(*pLightVol);

	pPL->m_iFogBox = m_lspFogBoxes.Add(spBox);
}

void CXR_Model_BSP::Fog_AddLightVolume(CBSP_FogBox* _pBox, int _iLightVolume)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_AddLightVolume, MAUTOSTRIP_VOID);
	const CBSP_LightVolumeInfo* pLightVol = &m_lLightVolumeInfo[_iLightVolume];

//	CPixel32 TmpLight[XW_LIGHTVOLUME_MAXGRIDSIZE * XW_LIGHTVOLUME_MAXGRIDSIZE * XW_LIGHTVOLUME_MAXGRIDSIZE];

//	CMathAccel* pMA = GetMathAccel();

	CPixel32* pBoxPixels = _pBox->m_lBox.GetBasePtr();

	while(pLightVol)
	{
		const CXR_LightGridPoint* pCells = pLightVol->m_pCells;
		if (pLightVol->m_CellsPerGrid != _pBox->m_lBox.Len())
		{
			ConOut(CStrF("(CXR_Model_BSP::Fog_AddLightVolume) Light-volume size missmatch. (%d != %d)", pLightVol->m_CellsPerGrid, _pBox->m_lBox.Len() ));
			return;
		}

//ConOut(CStrF("(CXR_Model_BSP::Fog_AddLightVolume) Cells %d, ", pLightVol->m_CellsPerGrid));

		for(int i = 0; i < pLightVol->m_CellsPerGrid; i++)
		{
			const CXR_LightGridPoint& Cell = pCells[i];
			fp32 Len = fp32(Cell.GetLightBias()) + M_Sqrt( Sqr((fp32)Cell.GetLightDirX()) + Sqr((fp32)Cell.GetLightDirY()) + Sqr((fp32)Cell.GetLightDirZ()) );
			int iLen = Min(255, 4 * RoundToInt(Len));
			CPixel32 Intens(iLen + (iLen << 8) + (iLen << 16));
			Intens *= Cell.GetLight();
			pBoxPixels[i] = Intens;
//			pBoxPixels[i] = 0xffffffff;
		}

		pLightVol = pLightVol->m_pNext;
	}
}

void CXR_Model_BSP::Fog_BuildFogBox(int _iPL)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_BuildFogBox, MAUTOSTRIP_VOID);
	CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[_iPL];
	if (!pPL->m_iFogBox) return;
	const CXR_MediumDesc* pMedium = &m_lMediums[pPL->m_iMedium];

	CBSP_FogBox* pBox = m_lspFogBoxes[pPL->m_iFogBox];
	if (!pBox->m_UpdateCount && (pBox->m_Flags & 1)) return;
	pBox->m_UpdateCount--;

	pBox->Clear();
	Fog_AddLightVolume(pBox, pPL->m_iLightVolume);

	CXR_Light* pL = m_spTempWLS->GetFirst();
	while(pL)
	{
		if (pL->m_Type == CXR_LIGHTTYPE_POINT) pBox->AddLight(pL);
		pL = pL->GetNext();
	}

	PPA_Mul_RGB32(pMedium->m_FogColor, pBox->m_lBox.GetBasePtr(), pBox->m_lBox.GetBasePtr(), pBox->m_lBox.Len());
	pBox->SetAlphaFromBrightness();
	pBox->Filter();
//	pBox->Filter();
//	pBox->Filter();

	pBox->m_Flags |= 1;
}

CPixel32 Fog_Add(CPixel32 _a, CPixel32 _b);
CPixel32 Fog_Add(CPixel32 _a, CPixel32 _b)
{
	MAUTOSTRIP(Fog_Add, CPixel32());
//	if (!_a.GetA())
//		return _b;
	if (!_b.GetA())
		return _a;
	if (_b.GetA() == 255)
		return _b;

	int Alpha = _b.GetA();
	int aR = _a.GetR();  
	int aG = _a.GetG();  
	int aB = _a.GetB();  
	int bR = _b.GetR();  
	int bG = _b.GetG();  
	int bB = _b.GetB();  
	int dR = bR - aR;
	int dG = bG - aG;
	int dB = bB - aB;
	if (dR > 0)
		aR = aR + Min(dR, Alpha);
	else if (dR < 0)
		aR = aR + Max(dR, -Alpha);

	if (dG > 0)
		aG = aG + Min(dG, Alpha);
	else if (dG < 0)
		aG = aG + Max(dG, -Alpha);

	if (dB > 0)
		aB = aB + Min(dB, Alpha);
	else if (dB < 0)
		aB = aB + Max(dB, -Alpha);

/*	aR = Min(255, Max(aR, 0));
	aG = Min(255, Max(aG, 0));
	aB = Min(255, Max(aB, 0));*/

	return aB + (aG << 8) + (aR << 16) + (Min(255, _b.GetA() + _a.GetA()) << 24);
}


//#define BSP_MODEL_FOGFLOATTRACE

CPixel32 CXR_Model_BSP::Fog_EvalFogBox(int _iPL, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MaxAlpha)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_EvalFogBox, CPixel32());
	const CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[_iPL];
	const CXR_MediumDesc* pMedium = &m_lMediums[pPL->m_iMedium];
	if (!(pMedium->m_MediumFlags & XW_MEDIUM_FOG)) return 0;
	
	if (pPL->m_iFogBox)
	{
//		g_FogTime = m_pCurAnimState->m_AnimTime0;
		CBSP_FogBox* pFogBox = m_lspFogBoxes[pPL->m_iFogBox];

		fp32 d0 = 1.0f - pMedium->m_FogPlane.Distance(_p0) * pMedium->m_FogAttenuation;
		fp32 d1 = 1.0f - pMedium->m_FogPlane.Distance(_p1) * pMedium->m_FogAttenuation;

//if (pMedium->m_FogAttenuation)
//ConOut(CStrF("FogAttn %f, ", pMedium->m_FogAttenuation) + pMedium->m_FogPlane.GetString());

		fp32 Length = M_Sqrt(Sqr(_p1.k[0]-_p0.k[0]) + Sqr(_p1.k[1]-_p0.k[1]) + Sqr(_p1.k[2]-_p0.k[2]));
	//	if (Length < 0.001f) 
	//		return 0xff00ff00;
	//		return pFogBox->GetFog(_p0, 0);

		const fp32 SubDivLen = pMedium->m_FogResolution;
//		const fp32 SubDivLenInv = 1.0f / SubDivLen;
		const fp32 DensityScale = pMedium->m_FogDensity / 512.0f;

	g_nFogTraceLeaf++;
		if (Length < SubDivLen)
		{
			fp32 Density = (d0+d1) * 0.5f * DensityScale;
			CVec3Dfp32 LPos;
			LPos.k[0] = (_p0.k[0] + _p1.k[0]) * 0.5f;
			LPos.k[1] = (_p0.k[1] + _p1.k[1]) * 0.5f;
			LPos.k[2] = (_p0.k[2] + _p1.k[2]) * 0.5f;
	g_nLightIter++;
			int Alpha = RoundToInt(255.0f*Clamp01(Density * Length));
			return pFogBox->GetFog(LPos, Alpha);
		}
		else
		{
			CPixel32 FogColors[256];
#ifdef BSP_MODEL_FOGFLOATTRACE
			fp32 Len = 0.0f;
			CVec3Dfp32 v, LPos;
			_p0.Sub(_p1, v);
			LPos = _p1;
			CPixel32 Fog = 0;
			fp32 d = d1 * DensityScale;
			fp32 LengthInv = 1.0f / Length;
			fp32 dstep = (d0 - d1) * LengthInv * DensityScale;
			fp32 LastLen = 0;
			int nColors = 0;
			int TotAlpha = 0;
			while(Len < Length)
			{
				fp32 l = Min(Length - Len, SubDivLen);
				Len += l;
				fp32 tmpd = d;
				d += dstep*l;
				LPos.Combine(v, (l+LastLen)*0.5f * LengthInv, LPos);
				fp32 Density = (d + tmpd) * 0.5f;
				int Alpha = RoundToInt(255.0f*Clamp01(Density * l));
				LastLen = l;
	g_nLightIter++;
				FogColors[nColors] = pFogBox->GetFog(LPos, Alpha);
				TotAlpha += FogColors[nColors].GetA();
				nColors++;
				if (nColors > 255) break;
				if (TotAlpha >= _MaxAlpha)
				{
	g_nLightIterSave++;
					if (_MaxAlpha < 255) break;
				}
			}
#else
			fp32 Len = 0.0f;
			CVec3Dfp32 tmpv, LPos, v;
			_p0.Sub(_p1, tmpv);
			pFogBox->GetFogBoxVectors(_p1, tmpv, LPos, v);
			CPixel32 Fog = 0;
			fp32 d = d1 * DensityScale;
			fp32 LengthInv = 1.0f / Length;
			fp32 dstep = (d0 - d1) * LengthInv * DensityScale;
			fp32 LastLen = 0;
			int nColors = 0;
			int TotAlpha = 0;
			while(Len < Length)
			{
				fp32 l = Min(Length - Len, SubDivLen);
				Len += l;
				fp32 tmpd = d;
				d += dstep*l;
				LPos.Combine(v, (l+LastLen)*0.5f * LengthInv, LPos);
				fp32 Density = (d + tmpd) * 0.5f;
				int Alpha = RoundToInt(255.0f*Clamp01(Density * l));
				LastLen = l;
	g_nLightIter++;
				FogColors[nColors] = pFogBox->GetFog_BoxSpace(LPos, Alpha);
				TotAlpha += FogColors[nColors].GetA();
				nColors++;
				if (nColors > 255) break;
				if (TotAlpha >= _MaxAlpha)
				{
	g_nLightIterSave++;
					if (_MaxAlpha < 255) break;
				}
			}
#endif

			if (nColors)
			{
				Fog = FogColors[nColors-1];
				for(int i = nColors-2; i >= 0; i--)
					Fog = Fog_Add(Fog, FogColors[i]);
			}

			return Fog;
		}
	}
	else
	{
		CVec3Dfp32 p1(_p1);
		const CPlane3Dfp32* pP = &pMedium->m_FogPlane;
		fp32 d2 = pP->Distance(_p1);
		if (d2*pMedium->m_FogAttenuation > 1.0f)
		{
			fp32 d1 = pP->Distance(_p0);
			d2 *= pMedium->m_FogAttenuation;
			d1 *= pMedium->m_FogAttenuation;
			fp32 s = d2 - d1;
			d1 -= 1.0f;
			if (M_Fabs(s) > 0.001f)
			{
				fp32 vscale = -d1/s;
				_p0.Lerp(p1, vscale, p1);
			}
		}
		fp32 d0 = Clamp01(1.0f - pMedium->m_FogPlane.Distance(_p0) * pMedium->m_FogAttenuation);
		fp32 d1 = Clamp01(1.0f - pMedium->m_FogPlane.Distance(p1) * pMedium->m_FogAttenuation);

//if (d1 < -0.01f) ConOut(CStrF("NegDensity PL %d, %f, %f, %s, %s", _iPL, d0, d1, (char*) _p0.GetString(), (char*) _p1.GetString() ));

		const fp32 DensityScale = pMedium->m_FogDensity / 512.0f;
		fp32 Density = (d0+d1)*0.5f * DensityScale;

		fp32 l = M_Sqrt(p1.DistanceSqr(_p0));
		int Alpha = RoundToInt(255.0f*Clamp01(Density * l));

//ConOut(CStrF("PL %d, Len %f, Dens %f, Alpha %d", _iPL, l, Density, Alpha));

		return (pMedium->m_FogColor & 0xffffff) + (Alpha << 24);
	}
}

CPixel32 CXR_Model_BSP::Fog_TracePortalLeaf(int _iFirstNode, int _iDestPL, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MaxAlpha)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_TracePortalLeaf, CPixel32());
	// _p0 is assumed to be in _iDestNode.
	// _p1 is cut with all ancestor node-splits.

	const CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[_iDestPL];
	if (pPL->m_ContainsMedium & XW_MEDIUM_CLIPFOG)
		return Fog_TraceLine_FogBox(_iFirstNode, _p0, _p1, _MaxAlpha);
	else
		return Fog_EvalFogBox(_iDestPL, _p0, _p1, _MaxAlpha);
}


CPixel32 CXR_Model_BSP::Fog_TraceLine_FogBox(int _iNode, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _MaxAlpha)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_TraceLine_FogBox, CPixel32());
	if (_MaxAlpha <= 0) return 0;

	if (!_iNode) return 0;
	CBSP_Node* pN = &m_lNodes[_iNode];
	if (!(pN->m_Flags & XW_NODE_ENABLE)) return 0;

	if (pN->IsStructureLeaf())
	{
		if (!(pN->m_Flags & XW_NODE_FOGENABLE)) return 0;
		int iPL = pN->m_iPortalLeaf;
		return Fog_EvalFogBox(iPL, _p0, _p1, _MaxAlpha);
	}

	if (pN->IsLeaf())
	{
		return 0;
	}

g_nFogTraceNode++;
	int iFront = pN->m_Bound.m_iBoundNodeFront;
	int iBack = pN->m_Bound.m_iBoundNodeBack;

	CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];

	fp32 d1 = pP->Distance(_p0);
	fp32 d2 = pP->Distance(_p1);

	int Side1 = (d1 > 0.0f) ? 1 : 2;
	int Side2 = (d2 > 0.0f) ? 1 : 2;
	int Side = Side1 | Side2;

	if (Side == 1)
		return Fog_TraceLine_FogBox(iFront, _p0, _p1, _MaxAlpha);
	else
	{
		if (Side == 2)
			return Fog_TraceLine_FogBox(iBack, _p0, _p1, _MaxAlpha);
		else
		{
			if (!(iFront || iBack)) return 0;

			fp32 s = d2 - d1;
			if (s == 0.0f) return 0;
			CVec3Dfp32 CutPoint;
			_p0.Lerp(_p1, -d1/s, CutPoint); 

			CPixel32 Fog(0);
			if ((Side1 & 1) || (Side2 & 2))
			{
				CPixel32 FogFront = (iFront) ? Fog_TraceLine_FogBox(iFront, _p0, CutPoint, _MaxAlpha) : CPixel32( 0 );
				if (iBack) Fog = Fog_TraceLine_FogBox(iBack, CutPoint, _p1, _MaxAlpha);
				if (FogFront)
				{
					if (Fog)
						Fog = Fog_Add(FogFront, Fog);
//						Fog = Fog_Add(Fog, FogFront);
					else
						Fog = FogFront;
				}
				return Fog;
			}
			else
			{
				CPixel32 FogBack = (iBack) ? Fog_TraceLine_FogBox(iBack, _p0, CutPoint, _MaxAlpha) : CPixel32( 0 );
				if (iFront) Fog = Fog_TraceLine_FogBox(iFront, CutPoint, _p1, _MaxAlpha);
				if (FogBack)
				{
					if (Fog)
						Fog = Fog_Add(FogBack, Fog);
//						Fog = Fog_Add(Fog, FogBack);
					else
						Fog = FogBack;
				}
				return Fog;
			}
		}
	}
}


void CXR_Model_BSP::Fog_TraceVertices(const uint32* _piFaces, int _nFaces, int _iPL)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_TraceVertices, MAUTOSTRIP_VOID);
	// Fog-tracing in portalized bsp-model

	CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[_iPL];

	CVec3Dfp32 TraceVol[9];

	// URGENTFIXME: DON'T USE FACEBOUNDBOX, IT DOES NOT REPRESENT THE VOLUME PROPERLY!!
	pPL->m_FaceBoundBox.GetVertices(TraceVol);
	TraceVol[8] = m_pView->m_CurLocalVP;

	// Do BSP-bounding.
	int iFirstNode = Structure_GetFirstSplitNode_Polyhedron(TraceVol, 9);
	Structure_EnableTreeFromNode_Polyhedron(iFirstNode, TraceVol, 9);
	Structure_InitBoundNodesFromEnabledNodes(iFirstNode, 0, 0);

	if (_iPL) EnableWithPortalFloodFill(pPL->m_iNode, XW_NODE_FOGENABLE, XW_MEDIUM_FOG);

	g_nFogTraceNode = 0;
	g_nFogTraceLeaf = 0;
	g_nLightIter = 0;
	g_nLightIterSave = 0;
	g_nFogRays = 0;
	g_nFogRayLamps = 0;

	int MaxFogTags = m_liFogTags.Len();

	int8* pFogTags = m_lFogTags.GetBasePtr();
	uint32* piFogTags = m_liFogTags.GetBasePtr();

	int iVBase = pPL->m_iBaseVertex;
	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = _piFaces[i];
		CBSP_Face* pF = &m_pFaces[iFace];
		int nv = pF->m_nVertices;
		int iiv = pF->m_iiVertices;
//		CPixel32 Fog[16];
//		bool bFog = false;
		for(int v = 0; v < nv; v++)
		{
			int iv = m_piVertices[v + iiv];
			int itv = iv - iVBase;

			if (!pFogTags[itv])
			{
g_nFogRays++;
				CPixel32 F = Fog_TracePortalLeaf(iFirstNode, _iPL, m_pVertices[iv], m_pView->m_CurLocalVP, 255);
				m_lVertexFog[itv] = F;

				if (m_nFogTags >= MaxFogTags)
				{
					m_liFogTags.SetLen(m_liFogTags.Len() + 256);
					piFogTags = m_liFogTags.GetBasePtr();
					MaxFogTags = m_liFogTags.Len();
				}
				piFogTags[m_nFogTags++] = itv;
			}
		}
	}

//ConOut(CStrF("N/L %d, %d, Rays %d, Lights/Ray %.2f, LightIterSave %d / %d", g_nFogTraceNode, g_nFogTraceLeaf, g_nFogRays, fp32(g_nFogRayLamps)/fp32(g_nFogRays), g_nLightIterSave, g_nLightIter));

	if (_iPL) DisableWithPortalFloodFill(pPL->m_iNode, XW_NODE_FOGENABLE);

	// Remove BSP-bounding.
	DisableTree(iFirstNode);
}

void CXR_Model_BSP::Fog_TraceVertices(const uint32* _piFaces, int _nFaces)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_TraceVertices_2, MAUTOSTRIP_VOID);
	// Fog-tracing using fog-state
	g_nFogTraceNode = 0;
	g_nFogTraceLeaf = 0;
	g_nLightIter = 0;
	g_nLightIterSave = 0;
	g_nFogRays = 0;
	g_nFogRayLamps = 0;


	if (!m_lVertexFog.Len())
		m_lVertexFog.SetLen(m_lVertices.Len());

	int MaxFogTags = m_liFogTags.Len();

	int8* pFogTags = m_lFogTags.GetBasePtr();
	uint32* piFogTags = m_liFogTags.GetBasePtr();

	int iVBase = 0;
	for(int i = 0; i < _nFaces; i++)
	{
		int iFace = _piFaces[i];
		CBSP_Face* pF = &m_pFaces[iFace];
		int nv = pF->m_nVertices;
		int iiv = pF->m_iiVertices;
//		CPixel32 Fog[16];
//		bool bFog = false;
		for(int v = 0; v < nv; v++)
		{
			int iv = m_piVertices[v + iiv];
			int itv = iv - iVBase;


			if (!pFogTags[itv])
			{
g_nFogRays++;
				m_lVertexFog[itv] = ms_pSceneFog->Trace(m_pVertices[iv]);

				if (m_nFogTags >= MaxFogTags)
				{
					m_liFogTags.SetLen(m_liFogTags.Len() + 256);
					piFogTags = m_liFogTags.GetBasePtr();
					MaxFogTags = m_liFogTags.Len();
				}
				piFogTags[m_nFogTags++] = itv;
			}
		}
	}
}

void CXR_Model_BSP::Fog_RenderPortal(int _iFirstNode, int _iPL, int _ip, int _iStartNode)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_RenderPortal, MAUTOSTRIP_VOID);
	const CBSP_Portal* pP = &m_lPortals[_ip];
	if (!pP->m_iFogPortal) return;

	const CBSP_FogPortal* pFP = &m_lFogPortals[pP->m_iFogPortal];
//ConOut(CStrF("   FogPortal %d, iP %d, PL %d", pP->m_iFogPortal, _ip, _iPL));
	int nv = pFP->m_nVertices;
	int ivbase = pFP->m_iVertices;

	CXR_VertexBuffer* pVB = ms_pVBM->Alloc_VB();
	if (!pVB) return;

	int nV = pFP->m_nVertices;
	int nP = 0;
	{ 	
		for(int f = 0; f < pFP->m_nFaces; f++)
			nP += (m_lFogPortalFaces[f + pFP->m_iFaces].m_nVertices + 2);
		nP += 1;
	}

	CVec3Dfp32* pV = &m_pVertices[ivbase];
	CVec2Dfp32* pTV = ms_pVBM->Alloc_V2(nV);
	CPixel32* pCol = ms_pVBM->Alloc_CPixel32(nV);
	uint16* pPrim = ms_pVBM->Alloc_Int16(nP);
	CRC_Attributes* pA = ms_pVBM->Alloc_Attrib();
	if (!pV || !pTV || !pCol || !pPrim || !pA) return;

	pA->SetDefault();
	if (ms_pCurrentEngine) ms_pCurrentEngine->SetDefaultAttrib(pA);
	pVB->m_pAttrib = pA;
	if (!pVB->AllocVBChain(ms_pVBM, false))
		return;
	pVB->Geometry_VertexArray(pV, nV, true);
	pVB->Geometry_TVertexArray(pTV, 0);
	pVB->Geometry_ColorArray(pCol);
	pVB->m_Priority = ms_BasePriority_Opaque + CXR_VBPRIORITY_FOGPORTAL;
	pVB->Matrix_Set(ms_pVBMatrixM2V);

	pA->Attrib_TextureID(0, ms_pSceneFog->m_FogTableTextureID);

	if (m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOGADDITIVE)
		pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	else
		pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

//	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE /*| CRC_FLAGS_ZCOMPARE*/ | CRC_FLAGS_CULL);

/*	if (_iFirstNode)
	{
		for(int v = 0; v < nv; v++)
			pCol[v] = Fog_TraceLine_FogBox(_iFirstNode, _iPL, m_pVertices[ivbase + v], m_pView->m_CurLocalVP, 255);
	}
	else*/
	{
		for(int v = 0; v < nv; v++)
			pCol[v] = Fog_TracePortalLeaf(_iFirstNode, _iPL, m_pVertices[ivbase + v], m_pView->m_CurLocalVP, 255);
	}

	if (!ms_pSceneFog->ConvertFogColors(nv, pCol, pTV))
		return;

	{
//		for(int v = 0; v < nv; v++)
//			pCol[v] = 0xff3f3f3f;

		int iP = 0;
		for(int f = 0; f < pFP->m_nFaces; f++)
		{
			const CBSP_FogPortalFace* pFPF = &m_lFogPortalFaces[f + pFP->m_iFaces];
			int iiv = pFPF->m_iiVertices;
			int fnv = pFPF->m_nVertices;
			pPrim[iP++] = CRC_RIP_TRIFAN + ((fnv+2) << 8);
			pPrim[iP++] = fnv;

			{
				for(int v = 0; v < fnv; v++)
					pPrim[iP++] = m_piVertices[iiv + v] - ivbase;
			}
		}
		pPrim[iP++] = CRC_RIP_END + (1 << 8);
		pVB->Render_IndexedPrimitives(pPrim, iP);
	}
	ms_pVBM->AddVB(pVB);

//ConOut(CStrF("   FogPortal %d, Priority %f", pP->m_iFogPortal, pVB->m_Priority));

//ConOut(CStrF("   FogPortal %d, iP %d, PL %d, nV %d, nVUsed %d", pP->m_iFogPortal, _ip, _iPL, nv, nvtrace));

}

void CXR_Model_BSP::Fog_RenderPortal_HW(int _iFirstNode_NotUsed_Remove, int _iPL, int _ip, int _iStartNode)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_RenderPortal_HW, MAUTOSTRIP_VOID);
	CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[_iPL];
	const CBSP_Portal* pP = &m_lPortals[_ip];
	if (!pP->m_iFogPortal) return;

	const CBSP_FogPortal* pFP = &m_lFogPortals[pP->m_iFogPortal];
//ConOut(CStrF("   FogPortal %d, iP %d, PL %d", pP->m_iFogPortal, _ip, _iPL));
//	int nv = pFP->m_nVertices;
	int ivbase = pFP->m_iVertices;

	CXR_VertexBuffer* pVB = ms_pVBM->Alloc_VB();
	if (!pVB) return;

	int nV = pFP->m_nVertices;
	int nP = 0;
	{ 	
		for(int f = 0; f < pFP->m_nFaces; f++)
			nP += (m_lFogPortalFaces[f + pFP->m_iFaces].m_nVertices + 2);
		nP += 1;
	}

	CVec3Dfp32* pV = &m_pVertices[ivbase];
	uint16* pPrim = ms_pVBM->Alloc_Int16(nP);
	CRC_Attributes* pA = ms_pVBM->Alloc_Attrib();
	if (!pV || !pPrim || !pA) return;

	pA->SetDefault();
	if (ms_pCurrentEngine) ms_pCurrentEngine->SetDefaultAttrib(pA);
	pVB->m_pAttrib = pA;

	Fog_SetAttrib(pVB->m_pAttrib, pPL, m_pView->m_CurLocalVP);

	if (!pVB->AllocVBChain(ms_pVBM, false))
		return;
	pVB->Geometry_VertexArray(pV, nV, true);
//	pVB->Geometry_TVertexArray(pV, 0);
//	pVB->Geometry_ColorArray((CPixel32*)pV);
	pVB->m_Priority = ms_BasePriority_Opaque + CXR_VBPRIORITY_FOGPORTAL;
	pVB->Matrix_Set(ms_pVBMatrixM2V);
	

//	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pA->Attrib_Disable(CRC_FLAGS_ZWRITE /*| CRC_FLAGS_ZCOMPARE*/ | CRC_FLAGS_CULL);
	pA->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);

	{
		int iP = 0;
		for(int f = 0; f < pFP->m_nFaces; f++)
		{
			const CBSP_FogPortalFace* pFPF = &m_lFogPortalFaces[f + pFP->m_iFaces];
			int iiv = pFPF->m_iiVertices;
			int fnv = pFPF->m_nVertices;
			pPrim[iP++] = CRC_RIP_TRIFAN + ((fnv+2) << 8);
			pPrim[iP++] = fnv;

			for(int v = 0; v < fnv; v++)
				pPrim[iP++] = m_piVertices[iiv + v] - ivbase;
		}
		pPrim[iP++] = CRC_RIP_END + (1 << 8);
		pVB->Render_IndexedPrimitives(pPrim, iP);
	}

	ms_pVBM->AddVB(pVB);

//ConOut(CStrF("   FogPortal %d, Priority %f", pP->m_iFogPortal, pVB->m_Priority));

//ConOut(CStrF("   FogPortal %d, iP %d, PL %d, nV %d, nVUsed %d", pP->m_iFogPortal, _ip, _iPL, nv, nvtrace));

}

int CXR_Model_BSP::Fog_GetVisiblePortals(int _iPL, uint32* _piPortals, int _MaxPortals)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_GetVisiblePortals, 0);
	const CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[_iPL];

	int nVisP = 0;
	int iip = pPL->m_iiPortals;
	int np = pPL->m_nPortals;
	for(int p = 0; p < np; p++)
	{
		int ip = m_liPortals[iip + p];
		const CBSP_Portal* pP = &m_lPortals[ip];
		if (!pP->m_iFogPortal) continue;
		fp32 d = m_pPlanes[pP->m_iPlane].Distance(m_pView->m_CurLocalVP);
		if (pP->m_iNodeBack == pPL->m_iNode)
		{
			d = -d;
			if (m_pView->m_liLeafRPortals[m_pNodes[pP->m_iNodeFront].m_iPortalLeaf] == 0) continue;	// If destination leaf is not visible, the portal is not visible either.
		}
		else
			if (m_pView->m_liLeafRPortals[m_pNodes[pP->m_iNodeBack].m_iPortalLeaf] == 0) continue;		// If destination leaf is not visible, the portal is not visible either.

		if (d < 0.0f) continue;

		if (nVisP >= _MaxPortals)
		{
			ConOut("(CXR_Model_BSP::Fog_GetVisiblePortals) Too many fog-portals.");
			continue;
		}
		_piPortals[nVisP++] = ip;
	}

	return nVisP;
}

void CXR_Model_BSP::Fog_RenderPortals(int _iPL, uint32* _piPortals, int _nP)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_RenderPortals, MAUTOSTRIP_VOID);
	if (!_nP) return;
	const CBSP_PortalLeafExt* pPL = &m_lPortalLeaves[_iPL];

	bool bHWNHF = 
		((ms_CurrentRCCaps & CRC_CAPS_FLAGS_EXTATTRIBUTES_NV20) 
		|| (ms_CurrentRCCaps & CRC_CAPS_FLAGS_TEXGENMODE_LINEARNHF)
		 ) && !(m_pCurMedium->m_MediumFlags & XW_MEDIUM_CLIPFOG);

	CVec3Dfp32 TraceVol[9];
	int iFirstNode = 1;

//	if (!bHWNHF)
	{
		pPL->m_FaceBoundBox.GetVertices(TraceVol);
		TraceVol[8] = m_pView->m_CurLocalVP;

		DisableTree(1);
		iFirstNode = Structure_GetFirstSplitNode_Polyhedron(TraceVol, 9);
		Structure_EnableTreeFromNode_Polyhedron(iFirstNode, TraceVol, 9);
		Structure_InitBoundNodesFromEnabledNodes(iFirstNode, 0, 0);

		if (_iPL) EnableWithPortalFloodFill(pPL->m_iNode, XW_NODE_FOGENABLE, XW_MEDIUM_FOG);
	}

	for(int p = 0; p < _nP; p++)
	{
		int ip = _piPortals[p];
		const CBSP_Portal* pP = &m_lPortals[ip];
		if (!pP->m_iFogPortal) continue;
		fp32 d = m_pPlanes[pP->m_iPlane].Distance(m_pView->m_CurLocalVP);
		if (pP->m_iNodeBack == pPL->m_iNode)
		{
			d = -d;
			if (m_pView->m_liLeafRPortals[m_pNodes[pP->m_iNodeFront].m_iPortalLeaf] == 0) continue;	// If destination leaf is not visible, the portal is not visible either.
		}
		else
			if (m_pView->m_liLeafRPortals[m_pNodes[pP->m_iNodeBack].m_iPortalLeaf] == 0) continue;		// If destination leaf is not visible, the portal is not visible either.

		if (d < 0.0f) continue;

		if (bHWNHF)
			Fog_RenderPortal_HW(iFirstNode, _iPL, ip, iFirstNode);
		else
			Fog_RenderPortal(iFirstNode, _iPL, ip, iFirstNode);
	}

//	if (!bHWNHF)
	{
		if (_iPL) DisableWithPortalFloodFill(pPL->m_iNode, XW_NODE_FOGENABLE);
		DisableTree(iFirstNode);
	}
}

bool CXR_Model_BSP::Fog_SetVertices(CXR_VertexBuffer* pVB, const uint32* _piFaces, int _nFaces, const uint32* _piFaceVerts, int _iPLVBase)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_SetVertices, false);
	int nV = pVB->GetVBChain()->m_nV;
	CPixel32* pCol = ms_pVBM->Alloc_CPixel32(nV);
	CVec2Dfp32* pTV = ms_pVBM->Alloc_V2(nV);
	if (!pCol || !pTV) return false;
	pVB->Geometry_ColorArray(pCol);
	pVB->Geometry_TVertexArray(pTV, 0);

	fp32 oow = ms_pSceneFog->m_FogTableWidthInv;
	fp32 omw = ms_pSceneFog->m_FogTableWidth - 1;

	for(int f = 0; f < _nFaces; f++)
	{
		const CBSP_Face* pF = &m_pFaces[_piFaces[f]];
		int nv = pF->m_nVertices;
		int iiv = pF->m_iiVertices;
		int iVB = _piFaceVerts[f];

		for(int v = 0; v < nv; v++)
		{
			int iv = m_piVertices[v + iiv];
			int iFogVert = iv - _iPLVBase;

			CPixel32 F = m_lVertexFog[iFogVert];
			pCol[iVB + v] = int(F) | 0xff000000;
			fp32 fFog = fp32(F.GetA()) / 255.0f;
			fFog = (fFog * omw + 0.5f) * oow;
			pTV[iVB + v].k[0] = fFog;
			pTV[iVB + v].k[1] = 0;
		}
	}

	return true;
}

bool CXR_Model_BSP::Fog_SetAttrib(CRC_Attributes* _pAttrib, CBSP_PortalLeafExt* _pPL, const CVec3Dfp32& _LocalEye)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_SetAttrib, false);
	ms_pSceneFog->SetDepthFogNone(_pAttrib);

	if (m_pCurMedium->m_MediumFlags & XW_MEDIUM_FOGADDITIVE)
		_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	else
		_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);

	_pAttrib->Attrib_ZCompare(CRC_COMPARE_EQUAL);
	_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
//	_pAttrib->Attrib_AlphaCompare(CRC_COMPARE_ALWAYS, 0);
	_pAttrib->Attrib_TextureID(0, ms_pSceneFog->m_FogTableTextureID);

//	_pAttrib->Attrib_TextureID(0, ms_pCurrentEngine->m_pTC->GetTextureID("DM01_10"));

	CVec4Dfp32* pParams = ms_pVBM->Alloc_V4(3);
	if (!pParams)
		return false;

	const CXR_MediumDesc& Medium = m_lMediums[_pPL->m_iMedium];
	fp32 DensityFactor = Medium.m_FogDensity / 512.0f * 0.5f;

	if (!(Medium.m_MediumFlags & XW_MEDIUM_FOG))
		ConOut("§cf80WARNING: (CXR_Model_BSP::Fog_SetAttrib) Not a fog PL.");

	pParams[0][0] = -Medium.m_FogPlane.n[0] * Medium.m_FogAttenuation;
	pParams[0][1] = -Medium.m_FogPlane.n[1] * Medium.m_FogAttenuation;
	pParams[0][2] = -Medium.m_FogPlane.n[2] * Medium.m_FogAttenuation;
	pParams[0][3] = -Medium.m_FogPlane.d * Medium.m_FogAttenuation + 1.0f;

	pParams[1][0] = _LocalEye[0];
	pParams[1][1] = _LocalEye[1];
	pParams[1][2] = _LocalEye[2];
	pParams[1][3] = DensityFactor;

	CPixel32 FogColor(Medium.m_FogColor);
	pParams[2][0] = fp32(FogColor.GetR()) / 255.0f;
	pParams[2][1] = fp32(FogColor.GetG()) / 255.0f;
	pParams[2][2] = fp32(FogColor.GetB()) / 255.0f;
	pParams[2][3] = 1.0f;

	_pAttrib->Attrib_TexGenAttr((fp32*)pParams);
	_pAttrib->Attrib_TexGen(0, CRC_TEXGENMODE_LINEARNHF, CRC_TEXGENCOMP_ALL);

	return true;
}


// -------------------------------------------------------------------
//  Overrides from CXR_FogInterface
// -------------------------------------------------------------------
void CXR_Model_BSP::Fog_Trace(int _hAccelerator, const CVec3Dfp32& _POV, const CVec3Dfp32* _pV, int _nV, CPixel32* _pFog)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_Trace, MAUTOSTRIP_VOID);
	if (m_pNodes[_hAccelerator].IsStructureLeaf())
	{
		int iPL = m_pNodes[_hAccelerator].m_iPortalLeaf;
		for(int v = 0; v < _nV; v++)
			_pFog[v] = Fog_EvalFogBox(iPL, _pV[v], _POV, 255);
	}
	else
	{
		for(int v = 0; v < _nV; v++)
			_pFog[v] = Fog_TraceLine_FogBox(_hAccelerator, _pV[v], _POV, 255);
	}
}

void CXR_Model_BSP::Fog_EnableFogPL_r(int _iNode, const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_EnableFogPL_r, MAUTOSTRIP_VOID);
	const CBSP_Node* pN = &m_pNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		EnableWithPortalFloodFill(_iNode, XW_NODE_FOGENABLE, XW_MEDIUM_FOG);
		return;
	}

	if (!(pN->m_Flags & XW_NODE_STRUCTURE)) return;

	if (pN->IsNode())
	{
		const CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
		int Side = pP->GetBoxPlaneSideMask(_Box.m_Min, _Box.m_Max);

		if (Side & 1) Fog_EnableFogPL_r(m_pNodes[_iNode].m_iNodeFront, _Box);
		if (Side & 2) Fog_EnableFogPL_r(m_pNodes[_iNode].m_iNodeBack, _Box);
	}
}

void CXR_Model_BSP::Fog_DisableFogPL_r(int _iNode, const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_DisableFogPL_r, MAUTOSTRIP_VOID);
	const CBSP_Node* pN = &m_pNodes[_iNode];

	if (pN->IsStructureLeaf())
	{
		DisableWithPortalFloodFill(_iNode, XW_NODE_FOGENABLE);
		return;
	}

	if (!(pN->m_Flags & XW_NODE_STRUCTURE)) return;

	if (pN->IsNode())
	{
		const CPlane3Dfp32* pP = &m_pPlanes[pN->m_iPlane];
		int Side = pP->GetBoxPlaneSideMask(_Box.m_Min, _Box.m_Max);

		if (Side & 1) Fog_DisableFogPL_r(m_pNodes[_iNode].m_iNodeFront, _Box);
		if (Side & 2) Fog_DisableFogPL_r(m_pNodes[_iNode].m_iNodeBack, _Box);
	}
}

int CXR_Model_BSP::Fog_InitTraceBound(const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_InitTraceBound, 0);
	CVec3Dfp32 TraceVol[9];
	_Box.GetVertices(TraceVol);
	TraceVol[8] = m_pView->m_CurLocalVP;

//ConOut(_Box.GetString());
	DisableTree(1);
	int iFirstNode = Structure_GetFirstSplitNode_Polyhedron(TraceVol, 9);
	Structure_EnableTreeFromNode_Polyhedron(iFirstNode, TraceVol, 9);
	Structure_InitBoundNodesFromEnabledNodes(iFirstNode, 0, 0);

	ms_Fog_LastBox = _Box;
	Fog_EnableFogPL_r(iFirstNode, _Box);

/*	if (m_pNodes[iFirstNode].IsStructureLeaf())
		ConOut(CStrF("PL %d", iFirstNode));
	else
		ConOut(CStrF("Tree %d", iFirstNode));
*/
	return iFirstNode;
}

void CXR_Model_BSP::Fog_ReleaseTraceBound(int _hAccelerator)
{
	MAUTOSTRIP(CXR_Model_BSP_Fog_ReleaseTraceBound, MAUTOSTRIP_VOID);
	Fog_DisableFogPL_r(_hAccelerator, ms_Fog_LastBox);

	DisableTree(_hAccelerator);
}

