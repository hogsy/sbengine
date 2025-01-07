
#include "PCH.h"

#include "XRUtil.h"
#include "XRVertexBuffer.h"
#include "XRVBManager.h"

#include "MFloat.h"

#include "XRFog.h"
#include "XRVBPrior.h"

#include "MSIMD.h"
#include "MFloat.h"

#include "XRVBUtil.h"

#define LERP(a,b,t) ((a) + ((b) - (a))*t)

// -------------------------------------------------------------------
//  Stuff in MImage.cpp. These should be moved to MSIMD.cpp
// -------------------------------------------------------------------

void PPA_Mul_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);
void PPA_MulAddSaturate_RGB32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

void PPA_Mul_RGBA32(int _EffectValue1, const void* pSrc, void* pDest, int nPixels);

bool CXR_Util::m_bInit = false;

CVec2Dfp32 CXR_Util::m_lBeamTVertices[4 * MAXBEAMS];
uint16 CXR_Util::m_lBeamTriangles[6 * MAXBEAMS];

CVec2Dfp32 CXR_Util::m_lBeamStripTVertices[2 * MAXBEAMS];
uint16 CXR_Util::m_lBeamStripTriangles[6 * MAXBEAMS];

CVec2Dfp32 CXR_Util::m_lQuadParticleTVertices[4 * MAXPARTICLES];
uint16 CXR_Util::m_lQuadParticleTriangles[6 * MAXPARTICLES];

CVec2Dfp32 CXR_Util::m_lTriParticleTVertices[3 * MAXPARTICLES];
uint16 CXR_Util::m_lTriParticleTriangles[3 * MAXPARTICLES];

uint16 CXR_Util::m_lTriFanTriangles[3 * MAXTRIFAN];


CRC_ExtAttributes_FragmentProgram20 CXR_Util::ms_lFPLighting[4];

/*
void CXR_Util::CalcEnvironmentMapping(const CVec3Dfp32 &_Pos, const CMat4Dfp32& _Mat, int _nV, const uint16* _piV, const CVec3Dfp32& _N, const CVec3Dfp32* _pV, CVec2Dfp32* _pTV)
{
	MAUTOSTRIP(CXR_Util_CalcEnvironmentMapping, MAUTOSTRIP_VOID);
	// FIXME: Optimize for SSE & 3DNow!

	// This is too bloody expensive (5 square-roots) but I can't think of any other way to get it right.

	for(int v = 0; v < _nV; v++)
	{
		int iv = (_piV) ? _piV[v] : v;
		CVec3Dfp32 v0;
		_Pos.Sub(_pV[iv], v0);
		v0.Normalize();

		fp32 s = QAcos(v0 * CVec3Dfp32::GetMatrixRow(_Mat, 0)) / _PI;
		fp32 t = QAcos(v0 * CVec3Dfp32::GetMatrixRow(_Mat, 2)) / _PI;
		s -= QAcos(_N * CVec3Dfp32::GetMatrixRow(_Mat, 0)) / _PI;
		t -= QAcos(_N * CVec3Dfp32::GetMatrixRow(_Mat, 2)) / _PI;
//		fp32 s = 0;
//		fp32 t = 0;
		_pTV[iv][0] = s + 0.5f;
		_pTV[iv][1] = -t + 0.5f;
	}
}
*/
//#define Sign(x) ((x < 0.0f) ? -1.0f : 1.0f)

// #define MACRO_FASTRECP(x) pMA->m_pFp32InverseTab[RoundToInt(x * fp32(CONST_INVERSEMAX/2))]*fp32(CONST_INVERSEMAX/2)

#define XR_UTIL_NEWENV

#ifdef XR_UTIL_NEWENV

void CXR_Util::Render_Text(CXR_VBManager* _pVBM, CRC_Font* _pFont, const CVec3Dfp32& _Position, const CMat4Dfp32& _VMat, const char* _TextString, const fp32& _Size, const CPixel32& _Color)
{
	CXR_VertexBuffer* pVB = _pVBM->Alloc_VB();
	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	CMat4Dfp32* pMat = _pVBM->Alloc_M4(_VMat);
	
	if(!pVB || !pMat || !pA ||!_pFont)
		return;

	CMat4Dfp32 InvVMat;
	_VMat.InverseOrthogonal(InvVMat);

	const CVec3Dfp32& Down = InvVMat.GetRow(1);
	const CVec3Dfp32& Dir = InvVMat.GetRow(0);

	pA->SetDefault();
	pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);

	pVB->m_pAttrib = pA;
	_pFont->Write(_pVBM, pVB, _Position, Dir, Down, _TextString, CVec2Dfp32(_Size,_Size), _Color);
	
	pVB->m_Priority = CXR_VBPRIORITY_CURSOR - 1.0f;
	pVB->Matrix_Set(pMat);
	_pVBM->AddVB(pVB);
}

//void CXR_Util::CalcEnvironmentMapping(const CVec3Dfp32 &_Pos, const CMat4Dfp32& _Mat, int _nV, const uint16* _piV, const CVec3Dfp32* _pN, const CVec3Dfp32* _pV, CVec2Dfp32* _pTV)
void CXR_Util::CalcEnvironmentMapping(const CVec3Dfp32 &_Pos, int _nV, const uint16* _piV, const CVec3Dfp32* _pN, const CVec3Dfp32* _pV, CVec2Dfp32* _pTV)
{
	MAUTOSTRIP(CXR_Util_CalcEnvironmentMapping_2, MAUTOSTRIP_VOID);
	const int MaxVertices = 256;
	if (_nV > MaxVertices)
	{
		int nV = _nV;
		int iV = 0;
		while(nV > 0)
		{
			if (_piV)
				CalcEnvironmentMapping(_Pos, Min(nV, MaxVertices), &_piV[iV], _pN, _pV, _pTV);
			else
				CalcEnvironmentMapping(_Pos, Min(nV, MaxVertices), NULL, &_pN[iV], &_pV[iV], &_pTV[iV]);
//			if (_piV)
//				CalcEnvironmentMapping(_Pos, _Mat, Min(nV, MaxVertices), &_piV[iV], _pN, _pV, _pTV);
//			else
//				CalcEnvironmentMapping(_Pos, _Mat, Min(nV, MaxVertices), NULL, &_pN[iV], &_pV[iV], &_pTV[iV]);
			iV += MaxVertices;
			nV -= MaxVertices;
		}
		return;
	}

//fp64 t; T_Start(t);

	uint8 Data[MaxVertices*(sizeof(CVec3Dfp32)*1 + 1*4) + 16*2];
	uint8* p = (uint8*)(((mint)&Data+15) & 0xfffffff0);
	CVec3Dfp32* pRefV = (CVec3Dfp32*) p;
	p += MaxVertices*sizeof(CVec3Dfp32);
	p = (uint8*)(((mint)p+15) & 0xfffffff0);
	fp32* pRecpSqrt = (fp32*)p;

	int v = 0;
	for(v = 0; v < _nV; v++)
	{
		int iv = (_piV) ? _piV[v] : v;
		CVec3Dfp32 v0, r;
		_Pos.Sub(_pV[iv], v0);
		v0.Reflect(_pN[iv], r);
		pRefV[v] = r;
		pRecpSqrt[v] = r.LengthSqr();
	}

	SIMD_RecpSqrt8(pRecpSqrt, NULL, _nV);

	for(v = 0; v < _nV; v++)
	{
		int iv = (_piV) ? _piV[v] : v;

		CVec3Dfp32& r = pRefV[v];
//		fp32 rx = r[0] * pRecpSqrt[v];
//		fp32 ry = r[1] * pRecpSqrt[v];
		fp32 rz = r[2] * pRecpSqrt[v];

		fp32 s = 1.0f/_SQRT2;
		fp32 rx = (r[0]*s - r[1]*s) * pRecpSqrt[v];
		fp32 ry = (r[0]*s + r[1]*s) * pRecpSqrt[v];

		fp32 z = (1.0f - M_Fabs(rx)) * 0.5f;
		fp32 absr1 = M_Fabs(ry);
		fp32 absr2 = M_Fabs(rz);

		if (absr1 > absr2)
		{
			_pTV[iv][0] = Sign(ry) * z + 0.5f;
//			_pTV[iv][1] = -rz * MACRO_FASTRECP(absr1) * z + 0.5f;
			_pTV[iv][1] = -rz / absr1 * z + 0.5f;
		}
		else
		{
//			_pTV[iv][0] = ry * MACRO_FASTRECP(absr2) * z + 0.5f;
			_pTV[iv][0] = ry / absr2 * z + 0.5f;
			_pTV[iv][1] = -Sign(rz) * z + 0.5f;
		}
	}

//T_Stop(t);ConOut(CStrF("EnvMap2  nV %d, ClocksPerV %.1f", _nV, t / _nV));
}

#else

void CXR_Util::CalcEnvironmentMapping(const CVec3Dfp32 &_Pos, const CMat4Dfp32& _Mat, int _nV, const uint16* _piV, const CVec3Dfp32* _pN, const CVec3Dfp32* _pV, CVec2Dfp32* _pTV)
{
	MAUTOSTRIP(CXR_Util_CalcEnvironmentMapping_3, MAUTOSTRIP_VOID);
//fp64 t; T_Start(t);

	// This is too bloody expensive (5 square-roots, 5 div, billions of muls.) but I can't think of any other way to get it right.
	for(int v = 0; v < _nV; v++)
	{
		int iv = (_piV) ? _piV[v] : v;
		CVec3Dfp32 v0;
		_Pos.Sub(_pV[iv], v0);
		v0.Normalize();
		fp32 s = QAcos(v0 * CVec3Dfp32::GetMatrixRow(_Mat, 0)) / _PI;
		fp32 t = QAcos(v0 * CVec3Dfp32::GetMatrixRow(_Mat, 1)) / _PI;
		s -= QAcos(_pN[iv] * CVec3Dfp32::GetMatrixRow(_Mat, 0)) / _PI;
		t -= QAcos(_pN[iv] * CVec3Dfp32::GetMatrixRow(_Mat, 1)) / _PI;
//		fp32 s = 0;
//		fp32 t = 0;
		_pTV[iv][0] = s + 0.5f;
		_pTV[iv][1] = -t + 0.5f;
	}

//T_Stop(t);ConOut(CStrF("EnvMap2  nV %d, ClocksPerV %.1f", _nV, t / _nV));
}
#endif
/*
void CXR_Util::CalcEnvironmentMapping(const CVec3Dfp32 &_Pos, const CMat4Dfp32& _Mat, int _nV, const uint16* _piV, const CVec3Dfp32* _pN, const uint32* _piN, const CVec3Dfp32* _pV, CVec2Dfp32* _pTV)
{
	MAUTOSTRIP(CXR_Util_CalcEnvironmentMapping_4, MAUTOSTRIP_VOID);
fp64 t;
T_Start(t);

	// This is too bloody expensive (5 square-roots) but I can't think of any other way to get it right.
	for(int v = 0; v < _nV; v++)
	{
		int iv = (_piV) ? _piV[v] : v;
		CVec3Dfp32 v0;
		_Pos.Sub(_pV[iv], v0);
		v0.Normalize();
		fp32 s = QAcos(v0 * CVec3Dfp32::GetMatrixRow(_Mat, 0)) / _PI;
		fp32 t = QAcos(v0 * CVec3Dfp32::GetMatrixRow(_Mat, 1)) / _PI;
		s -= QAcos(_pN[_piN[iv]] * CVec3Dfp32::GetMatrixRow(_Mat, 0)) / _PI;
		t -= QAcos(_pN[_piN[iv]] * CVec3Dfp32::GetMatrixRow(_Mat, 1)) / _PI;
//		fp32 s = 0;
//		fp32 t = 0;
		_pTV[iv][0] = s + 0.5f;
		_pTV[iv][1] = -t + 0.5f;
	}

T_Stop(t);
ConOut(CStrF("EnvMap3  nV %d, ClocksPerV %.1f", _nV, t / _nV));
}
*/

void CXR_Util::Init()
{
	MAUTOSTRIP(CXR_Util_Init, MAUTOSTRIP_VOID);
	if(!m_bInit)
	{
		ms_lFPLighting[0].Clear();
		ms_lFPLighting[1].Clear();
		ms_lFPLighting[2].Clear();
		ms_lFPLighting[3].Clear();
		ms_lFPLighting[0].SetProgram("XRUtil_VL3P0_T1", StringToHash("XRUtil_VL3P0_T1"));
		ms_lFPLighting[1].SetProgram("XRUtil_VL3P1_T1", StringToHash("XRUtil_VL3P1_T1"));
		ms_lFPLighting[2].SetProgram("XRUtil_VL3P2_T1", StringToHash("XRUtil_VL3P2_T1"));
		ms_lFPLighting[3].SetProgram("XRUtil_VL3P3_T1", StringToHash("XRUtil_VL3P3_T1"));

		{ //Quad Particle Init
			int iV = 0;
			int iT = 0;
			for(int b = 0; b < MAXPARTICLES; b++)
			{
				m_lQuadParticleTVertices[iV + 0][0] = 0;
				m_lQuadParticleTVertices[iV + 0][1] = 0;
				m_lQuadParticleTVertices[iV + 1][0] = 1;
				m_lQuadParticleTVertices[iV + 1][1] = 0;
				m_lQuadParticleTVertices[iV + 2][0] = 1;
				m_lQuadParticleTVertices[iV + 2][1] = 1;
				m_lQuadParticleTVertices[iV + 3][0] = 0;
				m_lQuadParticleTVertices[iV + 3][1] = 1;

				m_lQuadParticleTriangles[iT + 0] = iV + 0;
				m_lQuadParticleTriangles[iT + 1] = iV + 1;
				m_lQuadParticleTriangles[iT + 2] = iV + 3;
				m_lQuadParticleTriangles[iT + 3] = iV + 3;
				m_lQuadParticleTriangles[iT + 4] = iV + 1;
				m_lQuadParticleTriangles[iT + 5] = iV + 2;

				iV += 4;
				iT += 6;
			}
		}

		{ //Tri Particle Init
			int iV = 0;
			int iT = 0;
			for(int b = 0; b < MAXPARTICLES; b++)
			{
				m_lTriParticleTVertices[iV + 0][0] = 0;
				m_lTriParticleTVertices[iV + 0][1] = 0;
				m_lTriParticleTVertices[iV + 1][0] = 2;
				m_lTriParticleTVertices[iV + 1][1] = 0;
				m_lTriParticleTVertices[iV + 2][0] = 0;
				m_lTriParticleTVertices[iV + 2][1] = 2;

				m_lTriParticleTriangles[iT + 0] = iV + 0;
				m_lTriParticleTriangles[iT + 1] = iV + 1;
				m_lTriParticleTriangles[iT + 2] = iV + 2;

				iV += 3;
				iT += 3;
			}
		}

		{
			// Tri fan indices init
			int iV = 0;
			int iT = 0;
			for(int i = 0; i < MAXTRIFAN; i++)
			{
				m_lTriFanTriangles[iT + 0] = 0;
				m_lTriFanTriangles[iT + 1] = i + 1;
				m_lTriFanTriangles[iT + 2] = i + 2;
				iT += 3;
			}
		}

		{ //Beam Init
			int iV = 0;
			int iT = 0;
			for(int b = 0; b < MAXBEAMS; b++)
			{
				m_lBeamTVertices[iV + 0][0] = 1;
				m_lBeamTVertices[iV + 0][1] = 1;
				m_lBeamTVertices[iV + 1][0] = 1;
				m_lBeamTVertices[iV + 1][1] = 0;
				m_lBeamTVertices[iV + 2][0] = 0;
				m_lBeamTVertices[iV + 2][1] = 0;
				m_lBeamTVertices[iV + 3][0] = 0;
				m_lBeamTVertices[iV + 3][1] = 1;

				m_lBeamTriangles[iT + 0] = iV + 0;
				m_lBeamTriangles[iT + 1] = iV + 1;
				m_lBeamTriangles[iT + 2] = iV + 2;
				m_lBeamTriangles[iT + 3] = iV + 0;
				m_lBeamTriangles[iT + 4] = iV + 2;
				m_lBeamTriangles[iT + 5] = iV + 3;

				iV += 4;
				iT += 6;
			}
		}

		{ //Beam Strip Init
			int iV = 0;
			int iT = 0;
			for(int b = 0; b < MAXBEAMS; b++)
			{
				m_lBeamStripTVertices[iV + 0][0] = 1;
				m_lBeamStripTVertices[iV + 0][1] = 1 + b;
				m_lBeamStripTVertices[iV + 1][0] = 0;
				m_lBeamStripTVertices[iV + 1][1] = 1 + b;

				m_lBeamStripTriangles[iT + 0] = iV + 0;
				m_lBeamStripTriangles[iT + 1] = iV + 1;
				m_lBeamStripTriangles[iT + 2] = iV + 2;
				m_lBeamStripTriangles[iT + 3] = iV + 1;
				m_lBeamStripTriangles[iT + 4] = iV + 2;
				m_lBeamStripTriangles[iT + 5] = iV + 3;

				iV += 2;
				iT += 6;
			}
		}

		m_bInit = true;
	}
}
/*
void CXR_Util::Render_Particles(CXR_Engine* _pEngine, CRenderContext* _pRC, CXR_VBManager* _pVBM, const CRC_Particle* _pParticles, int _nParticles, fp32 _Size, const CMat4Dfp32* _pAlign, int _PrimType)
{
	MAUTOSTRIP(CXR_Util_Render_Particles, MAUTOSTRIP_VOID);
	const int nVertices = MAXPARTICLES*4;

	if (_nParticles > MAXPARTICLES) return;

	// Get alignment matrix from render if none was provided.
	CMat4Dfp32 Mat;
	_pRC->Matrix_Get(Mat);


	// Vertex data
	CVec3Dfp32 lVertices[nVertices];
	CVec2Dfp32 lTVertices[nVertices];
	CPixel32 lColors[nVertices];
	uint16 lTriangles[MAXPARTICLES*6];

	CVec3Dfp32* pV = lVertices;
	CVec2Dfp32* pTV = lTVertices;
	CPixel32* pC = lColors;

	// Build particle vertices and triangles
	fp32 Scale = _Size*0.5f;

	if (!_pAlign)
	{
		// View aligned particles

		int iv = 0;
		for(int p = 0; p < _nParticles; p++)
		{
			const CRC_Particle* pP = &_pParticles[p];
			pP->m_Pos.MultiplyMatrix(Mat, pV[iv + 0]);
			pV[iv + 0][0] -= Scale*0.5f;
			pV[iv + 0][1] -= Scale*0.5f;

			pV[iv + 1][0] = pV[iv + 0][0] + Scale;
			pV[iv + 1][1] = pV[iv + 0][1];
			pV[iv + 1][2] = pV[iv + 0][2];

			pV[iv + 2][0] = pV[iv + 0][0] + Scale;
			pV[iv + 2][1] = pV[iv + 0][1] + Scale;
			pV[iv + 2][2] = pV[iv + 0][2];

			pV[iv + 3][0] = pV[iv + 0][0];
			pV[iv + 3][1] = pV[iv + 0][1] + Scale;
			pV[iv + 3][2] = pV[iv + 0][2];
			iv += 4;
		}
	}
	else
	{
		// Arbitrarily aligned particles
		const CMat4Dfp32* pMat = _pAlign;
		CVec3Dfp32 vDown, vRight;
		vRight[0] = pMat->k[1][0];
		vRight[1] = pMat->k[1][1];
		vRight[2] = pMat->k[1][2];
		vDown[0] = pMat->k[2][0];
		vDown[1] = pMat->k[2][1];
		vDown[2] = pMat->k[2][2];
		vRight *= Mat;
		vDown *= Mat;

		int iv = 0;
		for(int p = 0; p < _nParticles; p++)
		{
			const CRC_Particle* pP = &_pParticles[p];
			pP->m_Pos.MultiplyMatrix(Mat, pV[iv + 0]);
			pV[iv + 0].Combine(vRight, -Scale*0.5f, pV[iv + 0]);
			pV[iv + 0].Combine(vDown, Scale*0.5f, pV[iv + 0]);
			pV[iv + 0].Combine(vRight, Scale, pV[iv + 1]);
			pV[iv + 1].Combine(vDown, -Scale, pV[iv + 2]);
			pV[iv + 2].Combine(vRight, -Scale, pV[iv + 3]);
			iv += 4;
		}
	}

	// Fill in texture-coordinates and colors.
	int iv = 0;
	for(int p = 0; p < _nParticles; p++)
	{
		const CRC_Particle* pP = &_pParticles[p];
		pC[iv + 0] = pP->m_Color;
		pC[iv + 1] = pP->m_Color;
		pC[iv + 2] = pP->m_Color;
		pC[iv + 3] = pP->m_Color;

		pTV[iv + 0][0] = 0.0f;		pTV[iv + 0][1] = 0.0f;
		pTV[iv + 1][0] = 0.5f;		pTV[iv + 1][1] = 0.0f;
		pTV[iv + 2][0] = 0.5f;		pTV[iv + 2][1] = 0.5f;
		pTV[iv + 3][0] = 0.0f;		pTV[iv + 3][1] = 0.5f;

		lTriangles[p*6 + 0] = iv;
		lTriangles[p*6 + 1] = iv+1;
		lTriangles[p*6 + 2] = iv+3;
		lTriangles[p*6 + 3] = iv+3;
		lTriangles[p*6 + 4] = iv+1;
		lTriangles[p*6 + 5] = iv+2;

		iv += 4;

//		LogFile(CStrF("%d, %d, %s, %d", p, iv, (char*)lVertices[iv].GetString(), pC[p]));
	}

	// Render
	_pRC->Matrix_Push();
	{
		_pRC->Matrix_SetUnit();

		_pRC->Geometry_VertexArray(lVertices, _nParticles*4);
		_pRC->Geometry_TVertexArray(lTVertices, 0);
		_pRC->Geometry_ColorArray(lColors);

		_pRC->Render_IndexedTriangles(lTriangles, _nParticles*2);

		_pRC->Geometry_Clear();
	}
	_pRC->Matrix_Pop();
}

void CXR_Util::Render_Particles(CXR_Engine* _pEngine, CRenderContext* _pRC, CXR_VBManager* _pVBM, const CXR_Particle2* _pParticles, int _nParticles, fp32 _SizeScale, const CMat4Dfp32* _pAlign, int _PrimType)
{
	MAUTOSTRIP(CXR_Util_Render_Particles_2, MAUTOSTRIP_VOID);
	const int nVertices = MAXPARTICLES*4;

	if (_nParticles > MAXPARTICLES) return;

	// Get alignment matrix from render if none was provided.
	CMat4Dfp32 Mat;
	_pRC->Matrix_Get(Mat);


	// Vertex data
	CVec3Dfp32 lVertices[nVertices];
	CVec2Dfp32 lTVertices[nVertices];
	CPixel32 lColors[nVertices];
	uint16 lTriangles[MAXPARTICLES*6];

	CVec3Dfp32* pV = lVertices;
	CVec2Dfp32* pTV = lTVertices;
	CPixel32* pC = lColors;

	// Build particle vertices and triangles
	fp32 Scale = _SizeScale*0.25f;

	if (!_pAlign)
	{
		// View aligned particles

		int iv = 0;
		for(int p = 0; p < _nParticles; p++)
		{
			const CXR_Particle2* pP = &_pParticles[p];
			pP->m_Pos.MultiplyMatrix(Mat, pV[iv + 0]);

			fp32 x0, x1;
			if(_PrimType & CXR_PARTICLETYPE_ANGLE)
			{
				fp32 v = (pP->m_Angle - 0.125f) * 2.0f * _PI;
				fp32 sinv, cosv;
				QSinCos(v, sinv, cosv);
				x0 = cosv*Scale*pP->m_Size;
				x1 = -sinv*Scale*pP->m_Size;
//				fp32 x0 = fcos(v)*Scale*pP->m_Size;
//				fp32 x1 = -fsin(v)*Scale*pP->m_Size;
			}
			else
			{
				x0 = Scale*pP->m_Size;
				x1 = -Scale*pP->m_Size;
			}

			fp32 y0 = -x1;
			fp32 y1 = x0;

			pV[iv + 1] = pV[iv + 0];
			pV[iv + 2] = pV[iv + 0];
			pV[iv + 3] = pV[iv + 0];

			pV[iv + 0][0] -= y0;
			pV[iv + 0][1] -= y1;

			pV[iv + 1][0] += x0;
			pV[iv + 1][1] += x1;

			pV[iv + 2][0] += y0;
			pV[iv + 2][1] += y1;

			pV[iv + 3][0] -= x0;
			pV[iv + 3][1] -= x1;
			iv += 4;
		}
	}
	else
	{
		// Arbitrarily aligned particles
		const CMat4Dfp32* pMat = _pAlign;
		CVec3Dfp32 vDown, vRight;
		vRight[0] = pMat->k[1][0];
		vRight[1] = pMat->k[1][1];
		vRight[2] = pMat->k[1][2];
		vDown[0] = pMat->k[2][0];
		vDown[1] = pMat->k[2][1];
		vDown[2] = pMat->k[2][2];
		vRight *= Mat;
		vDown *= Mat;

		int iv = 0;
		for(int p = 0; p < _nParticles; p++)
		{
			const CXR_Particle2* pP = &_pParticles[p];
			pP->m_Pos.MultiplyMatrix(Mat, pV[iv + 0]);

			CVec3Dfp32 rvDown, rvRight;
			fp32 v = (pP->m_Angle - 0.125f) * 2.0f * _PI;
			fp32 sinv, cosv;
			QSinCos(v, sinv, cosv);
			vDown.Scale(cosv, rvDown);
			rvDown.Combine(vRight, sinv, rvDown);

			vDown.Scale(-sinv, rvRight);
			rvRight.Combine(vRight, cosv, rvRight);

			pV[iv + 0].Combine(rvRight, -Scale*0.5f, pV[iv + 0]);
			pV[iv + 0].Combine(rvDown, Scale*0.5f, pV[iv + 0]);
			pV[iv + 0].Combine(rvRight, Scale, pV[iv + 1]);
			pV[iv + 1].Combine(rvDown, -Scale, pV[iv + 2]);
			pV[iv + 2].Combine(rvRight, -Scale, pV[iv + 3]);
			iv += 4;
		}
	}

	// Fill in texture-coordinates and colors.
	int iv = 0;
	for(int p = 0; p < _nParticles; p++)
	{
		const CXR_Particle2* pP = &_pParticles[p];
		pC[iv + 0] = pP->m_Color;
		pC[iv + 1] = pP->m_Color;
		pC[iv + 2] = pP->m_Color;
		pC[iv + 3] = pP->m_Color;

		pTV[iv + 0][0] = 0.0f;		pTV[iv + 0][1] = 0.0f;
		pTV[iv + 1][0] = 0.5f;		pTV[iv + 1][1] = 0.0f;
		pTV[iv + 2][0] = 0.5f;		pTV[iv + 2][1] = 0.5f;
		pTV[iv + 3][0] = 0.0f;		pTV[iv + 3][1] = 0.5f;

		lTriangles[p*6 + 0] = iv;
		lTriangles[p*6 + 1] = iv+1;
		lTriangles[p*6 + 2] = iv+3;
		lTriangles[p*6 + 3] = iv+3;
		lTriangles[p*6 + 4] = iv+1;
		lTriangles[p*6 + 5] = iv+2;

		iv += 4;
	}

	// Render
	_pRC->Matrix_Push();
	{
		_pRC->Matrix_SetUnit();

		_pRC->Geometry_VertexArray(lVertices, _nParticles*4);
		_pRC->Geometry_TVertexArray(lTVertices, 0);
		_pRC->Geometry_ColorArray(lColors);

		_pRC->Render_IndexedTriangles(lTriangles, _nParticles*2);

		_pRC->Geometry_Clear();
	}
	_pRC->Matrix_Pop();
}
*/
bool CXR_Util::Render_Particles(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_Mat, const CXR_Particle* _pParticles, int _nParticles, fp32 _Size, const CMat4Dfp32 *_pAlign, int _PrimType)
{
	MAUTOSTRIP(CXR_Util_Render_Particles_3, false);
//	const int nVertices = MAXPARTICLES * 4;

	if (_nParticles > MAXPARTICLES) return false;
	if (_nParticles <= 0) return false;

	int nPVert = ((_PrimType & CXR_PARTICLETYPE_TYPE_AND) == CXR_PARTICLETYPE_TRIANGLE) ? 3 : 4;

	CVec3Dfp32 *pV = _pVBM->Alloc_V3(nPVert * _nParticles);
	if(!pV) return false;
	CPixel32 *pC = _pVBM->Alloc_CPixel32(nPVert * _nParticles);
	if(!pC) return false;

	Init();

	// Build particle vertices and triangles
	fp32 Scale = _Size*0.5f;

	if (!_pAlign)
	{
		// View aligned particles

		if (nPVert == 4)
		{
			int iv = 0;
			for(int p = 0; p < _nParticles; p++)
			{
				const CRC_Particle* pP = &_pParticles[p];
				pP->m_Pos.MultiplyMatrix(_Mat, pV[iv + 0]);
				pV[iv + 0][0] -= Scale*0.5f;
				pV[iv + 0][1] -= Scale*0.5f;

				pV[iv + 1][0] = pV[iv + 0][0] + Scale;
				pV[iv + 1][1] = pV[iv + 0][1];
				pV[iv + 1][2] = pV[iv + 0][2];

				pV[iv + 2][0] = pV[iv + 0][0] + Scale;
				pV[iv + 2][1] = pV[iv + 0][1] + Scale;
				pV[iv + 2][2] = pV[iv + 0][2];

				pV[iv + 3][0] = pV[iv + 0][0];
				pV[iv + 3][1] = pV[iv + 0][1] + Scale;
				pV[iv + 3][2] = pV[iv + 0][2];
				iv += 4;
			}
		}
		else if (nPVert == 3)
		{
			int iv = 0;
			for(int p = 0; p < _nParticles; p++)
			{
				const CRC_Particle* pP = &_pParticles[p];
				pP->m_Pos.MultiplyMatrix(_Mat, pV[iv + 0]);
				pV[iv + 0][0] -= Scale*0.5f;
				pV[iv + 0][1] -= Scale*0.5f;

				pV[iv + 1][0] = pV[iv + 0][0] + Scale*2.0f;
				pV[iv + 1][1] = pV[iv + 0][1];
				pV[iv + 1][2] = pV[iv + 0][2];

				pV[iv + 2][0] = pV[iv + 0][0];
				pV[iv + 2][1] = pV[iv + 0][1] + Scale*2.0f;
				pV[iv + 2][2] = pV[iv + 0][2];
				iv += 3;
			}
		}
	}
	else
	{
		// Arbitrarily aligned particles
		const CMat4Dfp32* pMat = _pAlign;
		CVec3Dfp32 vDown, vRight;
		vRight[0] = pMat->k[1][0];
		vRight[1] = pMat->k[1][1];
		vRight[2] = pMat->k[1][2];
		vDown[0] = pMat->k[2][0];
		vDown[1] = pMat->k[2][1];
		vDown[2] = pMat->k[2][2];
		vRight.MultiplyMatrix3x3(_Mat);
		vDown.MultiplyMatrix3x3(_Mat);

		int iv = 0;
		for(int p = 0; p < _nParticles; p++)
		{
			const CRC_Particle* pP = &_pParticles[p];
			pP->m_Pos.MultiplyMatrix(_Mat, pV[iv + 0]);
			pV[iv + 0].Combine(vRight, -Scale*0.5f, pV[iv + 0]);
			pV[iv + 0].Combine(vDown, Scale*0.5f, pV[iv + 0]);
			pV[iv + 0].Combine(vRight, Scale, pV[iv + 1]);
			pV[iv + 1].Combine(vDown, -Scale, pV[iv + 2]);
			pV[iv + 2].Combine(vRight, -Scale, pV[iv + 3]);
			iv += 4;
		}
	}

	// Fill in colors.
	if (nPVert == 4)
	{
		int iv = 0;
		for(int p = 0; p < _nParticles; p++)
		{
			const CXR_Particle* pP = &_pParticles[p];
			pC[iv + 0] = pP->m_Color;
			pC[iv + 1] = pP->m_Color;
			pC[iv + 2] = pP->m_Color;
			pC[iv + 3] = pP->m_Color;
			iv += 4;
		}
	}
	else if (nPVert == 3)
	{
		int iv = 0;
		for(int p = 0; p < _nParticles; p++)
		{
			const CXR_Particle* pP = &_pParticles[p];
			pC[iv + 0] = pP->m_Color;
			pC[iv + 1] = pP->m_Color;
			pC[iv + 2] = pP->m_Color;
			iv += 3;
		}
	}

	// Render
	if (!_pVB->AllocVBChain(_pVBM, false))
		return false;
	_pVB->Geometry_ColorArray(pC);

	if (!(_PrimType & CXR_PARTICLETYPE_NOPRIMITIVES))
	{
		switch(_PrimType & CXR_PARTICLETYPE_TYPE_AND)
		{
		case CXR_PARTICLETYPE_AUTO :
		case CXR_PARTICLETYPE_QUAD :
			_pVB->Geometry_VertexArray(pV, _nParticles * 4, true);
			_pVB->Geometry_TVertexArray(m_lQuadParticleTVertices, 0);
			_pVB->Render_IndexedTriangles(m_lQuadParticleTriangles, _nParticles * 2);
			break;

		case CXR_PARTICLETYPE_TRIANGLE:
			_pVB->Geometry_VertexArray(pV, _nParticles * 3, true);
			_pVB->Geometry_TVertexArray(m_lTriParticleTVertices, 0);
			_pVB->Render_IndexedTriangles(m_lTriParticleTriangles, _nParticles);
			break;
		}
	}

	return true;
}

bool CXR_Util::Render_Particles(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_Mat, const CXR_Particle2* _pParticles, int _nParticles, const CMat4Dfp32* _pAlign, int _PrimType, uint32* _piParticles)
{
	MAUTOSTRIP(CXR_Util_Render_Particles_4, false);
//	const int nVertices = MAXPARTICLES * 4;

	if (_nParticles > MAXPARTICLES) return false;
	if (_nParticles <= 0) return false;

	int nPVert = ((_PrimType & CXR_PARTICLETYPE_TYPE_AND) == CXR_PARTICLETYPE_TRIANGLE) ? 3 : 4;

	CVec3Dfp32 *pV = _pVBM->Alloc_V3(nPVert * _nParticles);
	if(!pV) return false;
	CPixel32 *pC = _pVBM->Alloc_CPixel32(nPVert * _nParticles);
	if(!pC) return false;

	Init();

	// Build particle vertices and triangles

	if (!_pAlign)
	{
		// View aligned particles
		switch(_PrimType & CXR_PARTICLETYPE_TYPE_AND)
		{
		case CXR_PARTICLETYPE_AUTO :
		case CXR_PARTICLETYPE_QUAD :
			{
				const fp32 Scale = 0.5f;
				int iv = 0;
				for(int p = 0; p < _nParticles; p++)
				{
					const CXR_Particle2* pP = (_piParticles) ? 
						&_pParticles[_piParticles[p]] : 
						&_pParticles[p];
					pP->m_Pos.MultiplyMatrix(_Mat, pV[iv + 0]);

					fp32 x0, x1;
					if(_PrimType & CXR_PARTICLETYPE_ANGLE)
					{
						fp32 v = (pP->m_Angle - 0.125f) * _PI2;

						fp32 sinv, cosv;
						QSinCos(v, sinv, cosv);
						x0 = cosv*Scale*pP->m_Size * _SQRT2;
						x1 = -sinv*Scale*pP->m_Size * _SQRT2;
		//				x0 = fcos(v)*Scale*pP->m_Size;
		//				x1 = -fsin(v)*Scale*pP->m_Size;
					}
					else
					{
						x0 = Scale*pP->m_Size;
						x1 = -Scale*pP->m_Size;
					}

					fp32 y0 = -x1;
					fp32 y1 = x0;

					pV[iv + 1] = pV[iv + 0];
					pV[iv + 2] = pV[iv + 0];
					pV[iv + 3] = pV[iv + 0];

					pV[iv + 0][0] -= y0;
					pV[iv + 0][1] -= y1;

					pV[iv + 1][0] += x0;
					pV[iv + 1][1] += x1;

					pV[iv + 2][0] += y0;
					pV[iv + 2][1] += y1;

					pV[iv + 3][0] -= x0;
					pV[iv + 3][1] -= x1;
					iv += 4;
				}
			}
			break;

		case CXR_PARTICLETYPE_TRIANGLE :
			{
				const fp32 Scale = 0.5f;
				int iv = 0;
				for(int p = 0; p < _nParticles; p++)
				{
					const CXR_Particle2* pP = (_piParticles) ? 
						&_pParticles[_piParticles[p]] : 
						&_pParticles[p];

					pP->m_Pos.MultiplyMatrix(_Mat, pV[iv + 0]);

					fp32 x0, x1;
					if(_PrimType & CXR_PARTICLETYPE_ANGLE)
					{
						fp32 v = (pP->m_Angle) * _PI2;

						fp32 sinv, cosv;
						QSinCos(v, sinv, cosv);
						x0 = cosv*Scale*pP->m_Size;
						x1 = -sinv*Scale*pP->m_Size;
		//				x0 = fcos(v)*Scale*pP->m_Size;
		//				x1 = -fsin(v)*Scale*pP->m_Size;
					}
					else
					{
						x0 = Scale*pP->m_Size;
						x1 = 0* -Scale*pP->m_Size;
					}

					fp32 y0 = -x1;
					fp32 y1 = x0;

					pV[iv + 1] = pV[iv + 0];
					pV[iv + 2] = pV[iv + 0];
					pV[iv + 3] = pV[iv + 0];

					pV[iv + 0][0] += x0*-1.0f + x1*-1.0f;
					pV[iv + 0][1] += y0*-1.0f + y1*-1.0f;

					pV[iv + 1][0] += x0*3.0f + x1*-1.0f;
					pV[iv + 1][1] += y0*3.0f + y1*-1.0f;

					pV[iv + 2][0] += x0*-1.0f + x1*3.0f;
					pV[iv + 2][1] += y0*-1.0f + y1*3.0f;

					iv += 3;
				}
			}
			break;
		}
	}
	else
	{
		// Arbitrarily aligned particles
		const CMat4Dfp32* pMat = _pAlign;
		CVec3Dfp32 vDown, vRight;
		vRight[0] = pMat->k[1][0];
		vRight[1] = pMat->k[1][1];
		vRight[2] = pMat->k[1][2];
		vDown[0] = pMat->k[2][0];
		vDown[1] = pMat->k[2][1];
		vDown[2] = pMat->k[2][2];
		vRight *= _Mat;
		vDown *= _Mat;

		int iv = 0;
		for(int p = 0; p < _nParticles; p++)
		{
			const CXR_Particle2* pP = (_piParticles) ? 
				&_pParticles[_piParticles[p]] : 
				&_pParticles[p];

			pP->m_Pos.MultiplyMatrix(_Mat, pV[iv + 0]);

			CVec3Dfp32 rvDown, rvRight;
			fp32 v = (pP->m_Angle - 0.125f) * _PI2;
			fp32 sinv, cosv;
			QSinCos(v, sinv, cosv);
			vDown.Scale(cosv, rvDown);
			rvDown.Combine(vRight, sinv, rvDown);

			vDown.Scale(-sinv, rvRight);
			rvRight.Combine(vRight, cosv, rvRight);

			if (nPVert == 4)
			{
				const fp32 Scale = 0.5f;
				pV[iv + 0].Combine(rvRight, -Scale*0.5f, pV[iv + 0]);
				pV[iv + 0].Combine(rvDown, Scale*0.5f, pV[iv + 0]);
				pV[iv + 0].Combine(rvRight, Scale, pV[iv + 1]);
				pV[iv + 1].Combine(rvDown, -Scale, pV[iv + 2]);
				pV[iv + 2].Combine(rvRight, -Scale, pV[iv + 3]);
				iv += 4;
			}
			else if (nPVert == 3)
			{
				const fp32 Scale = 0.25f;
				pV[iv + 0].Combine(rvRight, -Scale*0.5f, pV[iv + 0]);
				pV[iv + 0].Combine(rvDown, Scale*0.5f, pV[iv + 0]);
				pV[iv + 0].Combine(rvRight, Scale*2.0f, pV[iv + 1]);
				pV[iv + 1].Combine(rvDown, -Scale*2.0f, pV[iv + 2]);
				pV[iv + 2].Combine(rvRight, -Scale*2.0f, pV[iv + 2]);
				iv += 3;
			}
		}
	}

	// Fill in colors.
	if (nPVert == 4)
	{
		int iv = 0;
		for(int p = 0; p < _nParticles; p++)
		{
			const CXR_Particle2* pP = (_piParticles) ? 
				&_pParticles[_piParticles[p]] : 
				&_pParticles[p];

			pC[iv + 0] = pP->m_Color;
			pC[iv + 1] = pP->m_Color;
			pC[iv + 2] = pP->m_Color;
			pC[iv + 3] = pP->m_Color;
			iv += 4;
		}
	}
	else if (nPVert == 3)
	{
		int iv = 0;
		for(int p = 0; p < _nParticles; p++)
		{
			const CXR_Particle2* pP = (_piParticles) ? 
				&_pParticles[_piParticles[p]] : 
				&_pParticles[p];

			pC[iv + 0] = pP->m_Color;
			pC[iv + 1] = pP->m_Color;
			pC[iv + 2] = pP->m_Color;
			iv += 3;
		}
	}

	// Render
	if (!_pVB->AllocVBChain(_pVBM, false))
		return false;
	_pVB->Geometry_ColorArray(pC);

	if (!(_PrimType & CXR_PARTICLETYPE_NOPRIMITIVES))
	{
		switch(_PrimType & CXR_PARTICLETYPE_TYPE_AND)
		{
		case CXR_PARTICLETYPE_AUTO :
		case CXR_PARTICLETYPE_QUAD :
			_pVB->Geometry_VertexArray(pV, _nParticles * 4, true);
			_pVB->Geometry_TVertexArray(m_lQuadParticleTVertices, 0);
			_pVB->Render_IndexedTriangles(m_lQuadParticleTriangles, _nParticles * 2);
			break;

		case CXR_PARTICLETYPE_TRIANGLE:
			_pVB->Geometry_VertexArray(pV, _nParticles * 3, true);
			_pVB->Geometry_TVertexArray(m_lTriParticleTVertices, 0);
			_pVB->Render_IndexedTriangles(m_lTriParticleTriangles, _nParticles);
			break;
		}
	}

	return true;
}


bool CXR_Util::Render_Particles(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_Mat, const CXR_Particle3* _pParticles, int _nParticles, int _nFramesX, int _nFramesY, int _nTotalFrames, fp32 _Duration)
{
	MAUTOSTRIP(CXR_Util_Render_Particles_5, false);
//	const int nVertices = MAXPARTICLES * 4;

	if (_nParticles > MAXPARTICLES || _nParticles <= 0)
		return false;

	int nPVert = 4;

	CVec3Dfp32 *pV = _pVBM->Alloc_V3(nPVert * _nParticles);
	CPixel32 *pC = _pVBM->Alloc_CPixel32(nPVert * _nParticles);
	CVec2Dfp32 *pTV = _pVBM->Alloc_V2(nPVert * _nParticles);
	
	if(!pV || !pC || !pTV)
		return false;

	Init();

	{
		fp32 UScale = 1.0f / _nFramesX;
		fp32 VScale = 1.0f / _nFramesY;
		fp32 IDuration = _nTotalFrames / _Duration;
		// Build particle vertices and triangles
		const fp32 Scale = 0.5f;
		int iv = 0;
		for(int p = 0; p < _nParticles; p++)
		{
			const CXR_Particle3* pP = &_pParticles[p];
			pP->m_Pos.MultiplyMatrix(_Mat, pV[iv + 0]);

			fp32 x0, x1;
/*			if(_PrimType & CXR_PARTICLETYPE_ANGLE)
			{
				fp32 v = (pP->m_Angle - 0.125f) * 2.0f * _PI;

				fp32 sinv, cosv;
				QSinCos(v, sinv, cosv);
				x0 = cosv*Scale*pP->m_Size * _SQRT2;
				x1 = -sinv*Scale*pP->m_Size * _SQRT2;
//				x0 = fcos(v)*Scale*pP->m_Size;
//				x1 = -fsin(v)*Scale*pP->m_Size;
			}
			else*/
			{
				x0 = Scale*pP->m_Size;
				x1 = -Scale*pP->m_Size;
			}

			fp32 y0 = -x1;
			fp32 y1 = x0;

			pV[iv + 1] = pV[iv + 0];
			pV[iv + 2] = pV[iv + 0];
			pV[iv + 3] = pV[iv + 0];

			pV[iv + 0][0] -= y0;
			pV[iv + 0][1] -= y1;

			pV[iv + 1][0] += x0;
			pV[iv + 1][1] += x1;

			pV[iv + 2][0] += y0;
			pV[iv + 2][1] += y1;

			pV[iv + 3][0] -= x0;
			pV[iv + 3][1] -= x1;

			int iFrame = Min(TruncToInt(pP->m_Time * IDuration), _nTotalFrames - 1);
			int iFrameX = iFrame % _nFramesX;
			int iFrameY = iFrame / _nFramesY;
			fp32 BaseU = iFrameX * UScale;
			fp32 BaseV = iFrameY * VScale;
			pTV[iv + 0][0] = BaseU;
			pTV[iv + 0][1] = BaseV;
			pTV[iv + 1][0] = BaseU + UScale;
			pTV[iv + 1][1] = BaseV;
			pTV[iv + 2][0] = BaseU + UScale;
			pTV[iv + 2][1] = BaseV + VScale;
			pTV[iv + 3][0] = BaseU;
			pTV[iv + 3][1] = BaseV + VScale;
			iv += 4;
		}
	}

	{
		// Fill in colors.
		int iv = 0;
		for(int p = 0; p < _nParticles; p++)
		{
			const CXR_Particle3* pP = &_pParticles[p];
			pC[iv + 0] = pP->m_Color;
			pC[iv + 1] = pP->m_Color;
			pC[iv + 2] = pP->m_Color;
			pC[iv + 3] = pP->m_Color;
			iv += 4;
		}
	}

	// Render
	if (!_pVB->AllocVBChain(_pVBM, false))
		return false;
	_pVB->Geometry_ColorArray(pC);

//	if (!(_PrimType & CXR_PARTICLETYPE_NOPRIMITIVES))
	{
		_pVB->Geometry_VertexArray(pV, _nParticles * 4, true);
		_pVB->Geometry_TVertexArray(pTV, 0);
		_pVB->Render_IndexedTriangles(m_lQuadParticleTriangles, _nParticles * 2);
	}

	return true;
}


#ifdef PLATFORM_PS2

#include "MDispPS2.h"
#include "MRndrPS2_DmaEngine.h"

void CXR_Util::Render_ApplyColorBufferPreRender(CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext)
{
	CDisplayContextPS2::ApplyColorBuffer( DmaEngine.m_VIF1Chain );
}

void CXR_Util::Render_ClearColorBufferPreRender(CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext)
{
	CDisplayContextPS2::ClearColorBuffer( DmaEngine.m_VIF1Chain );
}

void CXR_Util::Render_GlowFilter(CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext)
{
	CDisplayContextPS2::GlowFilterEffect( DmaEngine.m_VIF1Chain );
}
#endif


void CXR_Util::Render_FlaresPreRender(CRenderContext* _pRender, CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, void* _pContext, CXR_VBMScope* _pScope, int _Flags)
{
	MAUTOSTRIP(CXR_Util_Render_FlaresPreRender, MAUTOSTRIP_VOID);
	fp32* pData = (fp32*) _pContext;

	bool bOcclusionQuery = (_pRender->Caps_Flags() & CRC_CAPS_FLAGS_OCCLUSIONQUERY) != 0;

	// Get parameters
	fp32 DepthOffset = pData[0];
	int nSample = Max(1, Min(int(pData[1]), 10));
	int nFlares = int(pData[2]);

	fp32 Fade[MAXPARTICLES];
	int iFlareVis[MAXPARTICLES];
	int nFlareVis = 0;

	for(int iFlare = 0; iFlare < nFlares; iFlare++)
	{
		int iData = 3 + iFlare * 4;
		fp32 x2d = pData[iData + 0];
		fp32 y2d = pData[iData + 1];
		fp32 Z = pData[iData + 2];
		fp32 FlareID = pData[iData + 3];

		int nVis = 0;
		int nPixels = Sqr(nSample);

		if (bOcclusionQuery)
		{
			//_pRender->Attrib_ZCompare(CRC_COMPARE_LESSEQUAL);
			nVis = _pRender->OcclusionQuery_Rect(int(FlareID), CRct::From_fp32(x2d*0.5f, y2d*0.5f, 
				x2d*0.5f+nSample, y2d*0.5f+nSample),  Z-DepthOffset);
		}
		else
		{
			const int MaxPixels = 10*10;
			fp32 Depth[MaxPixels];
			int Disp = (nSample-1) >> 1;

	//ConOut(CStrF("Z %f, x %f, y %f, Smp %d, Ofs %f", Z, x2d, y2d, nSample, DepthOffset));
			if (_pRender->ReadDepthPixels(int(x2d*0.5f-Disp), int(y2d*0.5f-2*Disp), nSample, nSample, Depth))
			{
				const CRC_Viewport* pVP =_pRender->Viewport_Get();
				fp32 f = pVP->GetBackPlane();
				fp32 n = pVP->GetFrontPlane() * 0.5f;

				for(int i = 0; i < nPixels; i++)
				{
					fp32 zw = Depth[i];
					fp32 zd = (zw - 0.5f) * 2.0f;
					fp32 z = -2.0f * f * n / (zd*(f - n) - (f + n));
					z += DepthOffset;
					if (z >= Z) nVis++;
				}
			}
		}

		if (nVis)
		{
//ConOut(CStrF("%d, %d, Flare nVis %d", iFlare, nFlareVis, nVis));

			Fade[nFlareVis] = fp32(nVis) / fp32(nPixels);
			iFlareVis[nFlareVis] = iFlare;
			nFlareVis++;
		}
	}

	CXR_VBChain *pChain = _pVB->GetVBChain();
	pChain->m_nPrim = 0;
	if (nFlareVis)
	{
		int nPrim = nFlareVis*6;
		uint16* pP = pChain->m_piPrim;//_pVBM->Alloc_Int16(nPrim);
		if (!pP) return;
		int iP = 0;

		// Fade colors and build new triangle list
		for(int iiFlare = 0; iiFlare < nFlareVis; iiFlare++)
		{
			int iFlare = iFlareVis[iiFlare];
			int iV = iFlare*4;
			CPixel32* pC = pChain->m_pCol;
			if (pC)
			{
				for(int v = 0; v < 4; v++)
				{
					CPixel32 Col = pC[iV + v];
					pC[iV + v] = (int(Col) & 0x00ffffff) + (RoundToInt(Col.GetA()*Fade[iiFlare]) << 24);
				}
			}

			pP[iP++] = iV + 0;
			pP[iP++] = iV + 1;
			pP[iP++] = iV + 3;
			pP[iP++] = iV + 3;
			pP[iP++] = iV + 1;
			pP[iP++] = iV + 2;
		}

		_pVB->Geometry_Color(CPixel32(255,255,255,255));
		_pVB->Render_IndexedTriangles(pP, nFlareVis*2);
	}
}


bool CXR_Util::Render_Flares(CRenderContext* _pRC, CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_Mat, const CXR_Particle4* _pParticles, int _nParticles, fp32 _Attenuation, fp32 _DepthOffset, int _nSamplesSqr, bool _b3DProj)
{
	MAUTOSTRIP(CXR_Util_Render_Flares, false);
	// Attenuation		Fade out distance in units, zero equals no attenuation.
	// DepthOffset		Depth tolerance for occlusion. (for flares close to walls, etc.)
	// nSampleSqr		Number of depth-buffer samples (nSampleSqr x nSampleSqr depth-buffer fragments).
	// _b3DProj			false = Size implicates a two dimensional size, as if the flare is a "sprite", true = Size is projected in 3D.

	// NOTE: Rotation is not supported.

	if (!(_pRC->Caps_Flags() & (CRC_CAPS_FLAGS_READDEPTH | CRC_CAPS_FLAGS_OCCLUSIONQUERY)))
		return false;

	Init();
//	const int nVertices = MAXPARTICLES * 4;

	if (_nParticles > MAXPARTICLES) return false;
	if (_nParticles <= 0) return false;

	CVec3Dfp32 *pV = _pVBM->Alloc_V3(4 * _nParticles);
	if(!pV) return false;
	CPixel32 *pC = _pVBM->Alloc_CPixel32(4 * _nParticles);
	if(!pC) return false;

	//CRC_Viewport* pVP = _pRC->Viewport_Get();
	CRC_Viewport* pVP = _pVBM->Viewport_Get();

	fp32* pData = _pVBM->Alloc_fp32((3 + _nParticles*4));
	if (!pData) return false;

	// Set parameters
	pData[0] = _DepthOffset;
	pData[1] = _nSamplesSqr;
	pData[2] = 0;

	// Attenuation scale
	fp32 Scale = (_Attenuation) ? 1.0f / _Attenuation : 0.0f;

	// Calc projection scale
	//fp32 Z = pVP->GetXScale();
	fp32 Z = pVP->GetFrontPlane() + 1.0f;
	fp32 xs = 1.0f;
	fp32 ys = 1.0f;
	if (!_b3DProj)
	{
		xs =  Z / pVP->GetXScale();
		ys =  Z / pVP->GetYScale();
	}

	CRct Rct = pVP->GetViewRect();
	fp32 ws = xs * 0.5f * Rct.GetWidth() * (1.0f/640.0f);
	fp32 hs = ys * 0.5f * Rct.GetWidth() * (1.0f/640.0f);

	// Build particle vertices
	int nP = 0;
	int iv = 0;
	for(int p = 0; p < _nParticles; p++)
	{
		const CXR_Particle4* pP = &_pParticles[p];
		CVec3Dfp32 PosV = pP->m_Pos;
		//pP->m_Pos.MultiplyMatrix(_Mat, PosV);
		PosV *= _Mat;
		if (PosV.k[2] < pVP->GetFrontPlane()) continue;
		if (PosV.k[2] > pVP->GetBackPlane()) continue;
		fp32 Fade = 1.0f - PosV.k[2] * Scale;
		if (Fade < 0.0f) continue;

		fp32 x2d = PosV.k[0] * pVP->GetXScale() / PosV.k[2];
		fp32 y2d = PosV.k[1] * pVP->GetYScale() / PosV.k[2];

		fp32 x3d, y3d;
		if (_b3DProj)
		{
			x3d = PosV[0];
			y3d = PosV[1];
			Z = PosV.k[2];
		}
		else
		{
			x3d = x2d * xs;
			y3d = y2d * ys;
		}

		fp32 wh = pP->m_Dimensions.k[0] * ws;
		fp32 hh = pP->m_Dimensions.k[1] * hs;

		pV[iv+0].k[0] = x3d - wh;
		pV[iv+0].k[1] = y3d - hh;
		pV[iv+0].k[2] = Z;
		pV[iv+1].k[0] = x3d + wh;
		pV[iv+1].k[1] = y3d - hh;
		pV[iv+1].k[2] = Z;
		pV[iv+2].k[0] = x3d + wh;
		pV[iv+2].k[1] = y3d + hh;
		pV[iv+2].k[2] = Z;
		pV[iv+3].k[0] = x3d - wh;
		pV[iv+3].k[1] = y3d + hh;
		pV[iv+3].k[2] = Z;

		CPixel32 Col(pP->m_Color);
		Col = (pP->m_Color & 0x00ffffff) + (RoundToInt(Col.GetA()*Fade) << 24);
		pC[iv+0] = Col;
		pC[iv+1] = Col;
		pC[iv+2] = Col;
		pC[iv+3] = Col;

		pData[3 + nP*4 + 0] = x2d;
		pData[3 + nP*4 + 1] = y2d;
		pData[3 + nP*4 + 2] = PosV.k[2];
		pData[3 + nP*4 + 3] = pP->m_iObject;//pP->m_Pos[0] + pP->m_Pos[1] + pP->m_Pos[2];

		iv += 4;
		nP++;
	}

	if (!nP) return false;

	// Set number of flares that were actually projectable and within range.
	pData[2] = nP;

	// Render
	if (!_pVB->AllocVBChain(_pVBM, false))
		return false;
	_pVB->Geometry_VertexArray(pV, _nParticles * 4, true);
	_pVB->Geometry_TVertexArray(m_lQuadParticleTVertices, 0);
	_pVB->Geometry_ColorArray(pC);

	// Note the use of number of projectable flares, not the number of incoming particles.
	_pVB->Render_IndexedTriangles(m_lQuadParticleTriangles, nP * 2);

	_pVB->m_pPreRender = _pVBM->Alloc_PreRender(Render_FlaresPreRender, pData);

	return true;
}

bool CXR_Util::Render_Beams(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, const CXR_Beam *_pBeams, int _nBeams, int _Flags)
{
	MAUTOSTRIP(CXR_Util_Render_Beams, false);
	if(_nBeams > MAXBEAMS || _nBeams <= 0)
		return false;

	const int DataCount = _nBeams * 4;
	CMat4Dfp32 *pVMove = _pVBM->Alloc_M4();
	if(!pVMove)
		return false;
	CVec3Dfp32 *pVertices = _pVBM->Alloc_V3(DataCount);
	if(!pVertices)
		return false;
	CPixel32 *pColors = _pVBM->Alloc_CPixel32(DataCount);
	if(!pColors)
		return false;

	Init();

	CMat4Dfp32 Move;
	Move.Unit();
	CVec3Dfp32::GetMatrixRow(_WMat, 3).SetMatrixRow(Move, 3);
	Move.Multiply(_VMat, *pVMove);

/*	CMat4Dfp32 VInv;
	_VMat.InverseOrthogonal(VInv);
	CVec3Dfp32 InvPos = -CVec3Dfp32::GetMatrixRow(VInv, 3);
	InvPos += CVec3Dfp32::GetMatrixRow(_WMat, 3);*/
	CVec3Dfp32 InvPos(_VMat.k[3][0] * _VMat.k[0][0] + _VMat.k[3][1] * _VMat.k[0][1] + _VMat.k[3][2] * _VMat.k[0][2] + _WMat.k[3][0],
					 _VMat.k[3][0] * _VMat.k[1][0] + _VMat.k[3][1] * _VMat.k[1][1] + _VMat.k[3][2] * _VMat.k[1][2] + _WMat.k[3][1],
					 _VMat.k[3][0] * _VMat.k[2][0] + _VMat.k[3][1] * _VMat.k[2][1] + _VMat.k[3][2] * _VMat.k[2][2] + _WMat.k[3][2]);

	int iV = 0;
	for(int b = 0; b < _nBeams; b++)
	{
		CVec3Dfp32 VDir = _pBeams[b].m_Dir;
		VDir.MultiplyMatrix3x3(_WMat);

		CVec3Dfp32 VPos = _pBeams[b].m_Pos;
		VPos.MultiplyMatrix3x3(_WMat);

//		CVec3Dfp32 Up = VDir / (VPos + InvPos);
		CVec3Dfp32 Up(VDir[1] * (VPos[2] + InvPos[2]) - VDir[2] * (VPos[1] + InvPos[1]),
					-VDir[0] * (VPos[2] + InvPos[2]) + VDir[2] * (VPos[0] + InvPos[0]),
					 VDir[0] * (VPos[1] + InvPos[1]) - VDir[1] * (VPos[0] + InvPos[0]));
		Up *= _pBeams[b].m_Width * Up.LengthInv();

		pVertices[iV + 0] = VPos + Up;
		pVertices[iV + 1] = VPos + Up + VDir;
		pVertices[iV + 2] = VPos - Up + VDir;
		pVertices[iV + 3] = VPos - Up;
		pColors[iV + 0] = _pBeams[b].m_Color0;
		pColors[iV + 1] = _pBeams[b].m_Color1;
		pColors[iV + 2] = _pBeams[b].m_Color1;
		pColors[iV + 3] = _pBeams[b].m_Color0;

		iV += 4;

	}
	
	_pVB->Matrix_Set(pVMove);

	if (!_pVB->AllocVBChain(_pVBM, false))
		return false;
	_pVB->Geometry_VertexArray(pVertices, DataCount, true);
	_pVB->Geometry_TVertexArray(m_lBeamTVertices, 0);
	_pVB->Geometry_ColorArray(pColors);

	_pVB->Render_IndexedTriangles(m_lBeamTriangles, _nBeams * 2);
	return true;
}
/*
bool CXR_Util::Render_Beams(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, const CXR_BeamStrip *_pBeams, int _nBeams)
{
	MAUTOSTRIP(CXR_Util_Render_Beams_2, false);
	if(_nBeams > MAXBEAMS - 1 || _nBeams == 0)
		return false;

	CMat4Dfp32 *pVMove = _pVBM->Alloc_M4();
	if(!pVMove)
		return false;
	CVec3Dfp32 *pVertices = _pVBM->Alloc_V3(2 * _nBeams);
	if(!pVertices)
		return false;
	CPixel32 *pColors = _pVBM->Alloc_CPixel32(2 * _nBeams);
	if(!pColors)
		return false;

	Init();

	CMat4Dfp32 Move;
	Move.Unit();
	CVec3Dfp32::GetMatrixRow(_WMat, 3).SetMatrixRow(Move, 3);
	Move.Multiply(_VMat, *pVMove);

//	CMat4Dfp32 VInv;
//	_VMat.InverseOrthogonal(VInv);
//	CVec3Dfp32 InvPos = -CVec3Dfp32::GetMatrixRow(VInv, 3);
//	InvPos += CVec3Dfp32::GetMatrixRow(_WMat, 3);
	CVec3Dfp32 InvPos(_VMat.k[3][0] * _VMat.k[0][0] + _VMat.k[3][1] * _VMat.k[0][1] + _VMat.k[3][2] * _VMat.k[0][2] + _WMat.k[3][0],
					 _VMat.k[3][0] * _VMat.k[1][0] + _VMat.k[3][1] * _VMat.k[1][1] + _VMat.k[3][2] * _VMat.k[1][2] + _WMat.k[3][1],
					 _VMat.k[3][0] * _VMat.k[2][0] + _VMat.k[3][1] * _VMat.k[2][1] + _VMat.k[3][2] * _VMat.k[2][2] + _WMat.k[3][2]);

	int iV = 0;
	
	int LastAlpha = 0;

	for(int b = 0; b < _nBeams; b++)
	{
		CVec3Dfp32 VDir;
		int Next = _pBeams[b].m_Color & 0xff000000;
		if(LastAlpha == 0)
			VDir = _pBeams[b + 1].m_Pos - _pBeams[b].m_Pos;
		else if(Next == 0 || b == _nBeams - 1)
			VDir = _pBeams[b].m_Pos - _pBeams[b - 1].m_Pos;
		else
			VDir = _pBeams[b + 1].m_Pos - _pBeams[b - 1].m_Pos;
		LastAlpha = Next;

		VDir.MultiplyMatrix3x3(_WMat);

		CVec3Dfp32 VPos = _pBeams[b].m_Pos;
		VPos.MultiplyMatrix3x3(_WMat);

//		CVec3Dfp32 Up = VDir / (VPos + InvPos);
		CVec3Dfp32 Up(VDir[1] * (VPos[2] + InvPos[2]) - VDir[2] * (VPos[1] + InvPos[1]),
					-VDir[0] * (VPos[2] + InvPos[2]) + VDir[2] * (VPos[0] + InvPos[0]),
					 VDir[0] * (VPos[1] + InvPos[1]) - VDir[1] * (VPos[0] + InvPos[0]));
		Up *= _pBeams[b].m_Width / Up.Length();

		pVertices[iV + 0] = VPos + Up;
		pVertices[iV + 1] = VPos - Up;
		pColors[iV + 0] = _pBeams[b].m_Color;
		pColors[iV + 1] = _pBeams[b].m_Color;

		iV += 2;
	}

	_pVB->Matrix_Set(pVMove);

	_pVB->Geometry_VertexArray(pVertices, _nBeams * 2);
	_pVB->Geometry_TVertexArray(m_lBeamStripTVertices, 0);
	_pVB->Geometry_ColorArray(pColors);

	_pVB->Render_IndexedTriangles(m_lBeamStripTriangles, (_nBeams - 1) * 2);
	return true;
}
*/

bool CXR_Util::Render_BeamStrip(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, const CXR_BeamStrip *_pBeams, int _nBeams, int _Flags)
{
	MAUTOSTRIP(CXR_Util_Render_BeamStrip, false);
	if(_nBeams > MAXBEAMS - 1 || _nBeams <= 0)
		return false;

	CMat4Dfp32 *pVMove = _pVBM->Alloc_M4();
	if(!pVMove)
		return false;
	CVec3Dfp32 *pVertices = _pVBM->Alloc_V3(2 * _nBeams);
	if(!pVertices)
		return false;
	CVec2Dfp32 *pTVertices = _pVBM->Alloc_V2(2 * _nBeams);
	if(!pTVertices)
		return false;
	CPixel32 *pColors = _pVBM->Alloc_CPixel32(2 * _nBeams);
	if(!pColors)
		return false;
	uint16 *pTris = _pVBM->Alloc_Int16(6 * _nBeams);
	if(!pTris)
		return false;

	CMat4Dfp32 Move;
	Move.Unit();
	CVec3Dfp32::GetMatrixRow(_WMat, 3).SetMatrixRow(Move, 3);
	Move.Multiply(_VMat, *pVMove);

//	CMat4Dfp32 VInv;
//	_VMat.InverseOrthogonal(VInv);
//	CVec3Dfp32 InvPos = -CVec3Dfp32::GetMatrixRow(VInv, 3);
//	InvPos += CVec3Dfp32::GetMatrixRow(_WMat, 3);
	CVec3Dfp32 InvPos(_VMat.k[3][0] * _VMat.k[0][0] + _VMat.k[3][1] * _VMat.k[0][1] + _VMat.k[3][2] * _VMat.k[0][2] + _WMat.k[3][0],
					 _VMat.k[3][0] * _VMat.k[1][0] + _VMat.k[3][1] * _VMat.k[1][1] + _VMat.k[3][2] * _VMat.k[1][2] + _WMat.k[3][1],
					 _VMat.k[3][0] * _VMat.k[2][0] + _VMat.k[3][1] * _VMat.k[2][1] + _VMat.k[3][2] * _VMat.k[2][2] + _WMat.k[3][2]);

	int iV = 0;
	int iTri = 0;
	
	int Last = 0;

	for(int b = 0; b < _nBeams; b++)
	{
		CVec3Dfp32 VDir;
		int Next = _pBeams[b].m_Color & 0xff000000;
		if(((Last == 0 && Next == 0) || b == 0) && b < _nBeams - 1)
			VDir = _pBeams[b + 1].m_Pos - _pBeams[b].m_Pos;
		else
		{
			if(Next == 0 || b == _nBeams - 1)
				VDir = _pBeams[b].m_Pos - _pBeams[b - 1].m_Pos;
			else
				VDir = _pBeams[b + 1].m_Pos - _pBeams[b - 1].m_Pos;

			if(VDir.LengthSqr() > 0.1f)
			{
				pTris[iTri++] = iV - 2;
				pTris[iTri++] = iV - 1;
				pTris[iTri++] = iV + 0;
				pTris[iTri++] = iV - 1;
				pTris[iTri++] = iV + 0;
				pTris[iTri++] = iV + 1;
			}
		}

//		if(VDir.LengthSqr() > 0.1)
		{
			VDir.MultiplyMatrix3x3(_WMat);

			CVec3Dfp32 VPos = _pBeams[b].m_Pos;
			VPos.MultiplyMatrix3x3(_WMat);

	//		CVec3Dfp32 Up = VDir / (VPos + InvPos);

			CVec3Dfp32 Up(VDir[1] * (VPos[2] + InvPos[2]) - VDir[2] * (VPos[1] + InvPos[1]),
						-VDir[0] * (VPos[2] + InvPos[2]) + VDir[2] * (VPos[0] + InvPos[0]),
						 VDir[0] * (VPos[1] + InvPos[1]) - VDir[1] * (VPos[0] + InvPos[0]));
			Up *= _pBeams[b].m_Width * Up.LengthInv();

			pVertices[iV + 0] = VPos + Up;
			pVertices[iV + 1] = VPos - Up;

			if(_Flags & CXR_BEAMFLAGS_EDGEFADE)
			{
				CVec3Dfp32 V0 = VPos + InvPos;
				CVec3Dfp32 V1 = VDir;
				V0.Normalize();
				V1.Normalize();
				int Alpha = int(255 - 255 * M_Fabs(V0 * V1));
				pColors[iV + 0] = _pBeams[b].m_Color & 0x00ffffff | ((((_pBeams[b].m_Color & 0xff000000) >> 8) * Alpha) & 0xff000000);
				pColors[iV + 1] = _pBeams[b].m_Color & 0x00ffffff | ((((_pBeams[b].m_Color & 0xff000000) >> 8) * Alpha) & 0xff000000);
			}
			else
			{
				pColors[iV + 0] = _pBeams[b].m_Color;
				pColors[iV + 1] = _pBeams[b].m_Color;
			}

			pTVertices[iV + 0][0] = 1;
			pTVertices[iV + 1][0] = 0;
			if(_Flags & CXR_BEAMFLAGS_TEXFROMOFFSET)
			{
				pTVertices[iV + 0][1] = _pBeams[b].m_TextureYOfs;
				pTVertices[iV + 1][1] = _pBeams[b].m_TextureYOfs;
			}
			else
			{
				pTVertices[iV + 0][1] = b + _pBeams[b].m_TextureYOfs;
				pTVertices[iV + 1][1] = b + _pBeams[b].m_TextureYOfs;
			}

			iV += 2;
		}

		Last = Next;
	}

	if(iTri == 0)
		return false;

	_pVB->Matrix_Set(pVMove);

	if (!_pVB->AllocVBChain(_pVBM, false))
		return false;
	_pVB->Geometry_VertexArray(pVertices, _nBeams * 2, true);
	_pVB->Geometry_TVertexArray(pTVertices, 0);
	_pVB->Geometry_ColorArray(pColors);

	_pVB->Render_IndexedTriangles(pTris, iTri / 3);
	return true;
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

bool CXR_Util::Render_BeamStrip2(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, const CXR_BeamStrip *_pBeams, int _numBeams, int _Flags)
{
	MAUTOSTRIP(CXR_Util_Render_BeamStrip2, false);
	if ((_numBeams > (MAXBEAMS - 1)) || (_numBeams <= 0))
		return false;

	CVec3Dfp32 *pVertexPos = _pVBM->Alloc_V3(2 * _numBeams);
	if (!pVertexPos)
		return false;

	CVec2Dfp32 *pVertexTex = _pVBM->Alloc_V2(2 * _numBeams);
	if (!pVertexTex)
		return false;

	CPixel32 *pVertexColor = _pVBM->Alloc_CPixel32(2 * _numBeams);
	if (!pVertexColor)
		return false;

	// Worst case scenario, where every beamstrip requires a stream opcode.
	// 2 index per beam + 1 opcode & 1 lencode per beam + end opcode.
	uint16 *pStripIndex = _pVBM->Alloc_Int16(2 * _numBeams + 2 * _numBeams + 1);
	if (!pStripIndex)
		return false;

	CMat4Dfp32 matrix;

	// Calculate local to camera transformation matrix.
	_WMat.Multiply(_VMat, matrix);

	CVec3Dfp32 pos[MAXBEAMS];

	// Transform vertices from local space to camera space.
	for (int i = 0; i < _numBeams; i++)
	{
		pos[i] = _pBeams[i].m_Pos * matrix;
	}

	int iBeam, jBeam, iVertex, iIndex, iChainStart = 0;
	bool first, last = 0, prefetched = 0;
	CVec3Dfp32 dir, midpos, zero, curup, prevup, up;
	fp32 viewangle, last_viewangle = 0;

	zero = CVec3Dfp32(0, 0, 0);

	viewangle = 1.0f;
	first = true;
	iBeam = 0;
	iVertex = 0;
	iIndex = 0;

	// Process all beamstrip joints.
	while (iBeam < _numBeams)
	{
		if (first)
		{
			last = false;
			curup = zero;
			prefetched = false;
		}

		jBeam = iBeam; // Save current beam index, since the index may be increased in case of prefetching.

		if (!prefetched)
		{
			prevup = curup;
			do {
				if (((iBeam + 1) == _numBeams) || (_pBeams[iBeam + 1].m_Flags & CXR_BEAMFLAGS_BEGINCHAIN))
				{
					dir = zero; curup = zero;
					last = true;
				} else if (pos[iBeam] == pos[iBeam + 1]) {
					dir = zero; curup = zero;
					prefetched = true;
				} else {
					dir = pos[iBeam + 1] - pos[iBeam];
					midpos = (pos[iBeam] + pos[iBeam + 1]) * 0.5f;
					dir.CrossProd(midpos, curup);
					curup.Normalize();

					if (_Flags & CXR_BEAMFLAGS_EDGEFADE) {
						last_viewangle = viewangle;
						dir.Normalize();
						midpos.Normalize();
						viewangle = 1.0f - M_Fabs(dir * midpos);

						// InvSqrInv to lessen/focus the edgefade effect.
						viewangle = 1.0f - Sqr(1.0f - viewangle); // Once
						viewangle = 1.0f - Sqr(1.0f - viewangle); // Twice
						viewangle = 1.0f - Sqr(1.0f - viewangle); // Third time =)
					}
					else
					{
						last_viewangle = viewangle;
						dir.Normalize();
						midpos.Normalize();
						viewangle = M_Fabs(dir * midpos);

						viewangle = 0.5f - 0.5f * cosf(viewangle * _PI);

//						viewangle = 1.0f - Sqr(1.0f - viewangle); // Once
//						viewangle = 1.0f - Sqr(1.0f - viewangle); // Twice
//						viewangle = 1.0f - Sqr(1.0f - viewangle); // Third time =)
						
//						viewangle = Sqr(viewangle);
//						viewangle = Sqr(viewangle);

						viewangle = 0.99f;
						curup = LERP(curup, prevup, viewangle);
						/*
						fp32 limit = 1 / _SQRT2;
						if (viewangle < 0.1f)
						{
							if (curup[0] > limit)
								curup = CVec3Dfp32(1, 0, 0);
							else if (curup[0] < -limit)
								curup = CVec3Dfp32(-1, 0, 0);
							else if (curup[1] > limit)
								curup = CVec3Dfp32(0, 1, 0);
							else if (curup[1] < -limit)
								curup = CVec3Dfp32(0, -1, 0);
							else if (curup[2] > limit)
								curup = CVec3Dfp32(0, 0, 1);
							else if (curup[2] < -limit)
								curup = CVec3Dfp32(0, 0, -1);
						}
						*/
					}
				}
				iBeam++;
			} while((dir == zero) && !last);
		} else {
			prefetched = false;
			jBeam--; // Why this? Can't rememeber. It's in the pseudo =).
		}

		if (first)
		{
			if (last) {
				up = zero;
			} else {
				up = curup;
				first = false;
			}

			// Save strip chain start index.
			iChainStart = iIndex;
			iIndex += 2;
		} else {
			if (last) {
				up = prevup;
				first = true;
			} else {
				up = (curup + prevup) * 0.5f;
			}
		}

		if (up != zero)
		{
			// Normalize up/width vector.
			up *= 0.5f * _pBeams[jBeam].m_Width * up.LengthInv();

			// Set vertex positions.
			pVertexPos[iVertex + 0] = pos[jBeam] - up;
			pVertexPos[iVertex + 1] = pos[jBeam] + up;

			// Set vertex colors.
			if (_Flags & CXR_BEAMFLAGS_EDGEFADE) {
				fp32 fade = Min(viewangle, last_viewangle);
				int32 color = _pBeams[jBeam].m_Color;
				int32 alpha = (color & 0xFF000000) >> 24;
				color = (color & 0x00FFFFFF) + ((int32)(fade * (fp32)alpha) << 24);
				pVertexColor[iVertex + 0] = color; pVertexColor[iVertex + 1] = color;
			} else {
				pVertexColor[iVertex + 0] = _pBeams[jBeam].m_Color; pVertexColor[iVertex + 1] = _pBeams[jBeam].m_Color;
			}

			// Set vertex texture coordinates.
			pVertexTex[iVertex + 0][0] = 0;
			pVertexTex[iVertex + 1][0] = 1;
			if (_Flags & CXR_BEAMFLAGS_AUTOTEXOFFSET)
				pVertexTex[iVertex + 0][1] = pVertexTex[iVertex + 1][1] = jBeam;
			else
			{
				pVertexTex[iVertex + 0][1] = _pBeams[jBeam].m_TextureYOfs;
				pVertexTex[iVertex + 1][1] = _pBeams[jBeam].m_TextureYOfs;
			}

			// Add strip indices.
			pStripIndex[iIndex + 0] = iVertex + 0;
			pStripIndex[iIndex + 1] = iVertex + 1;

			// Increase vertex & index count.
			iVertex += 2;
			iIndex += 2;
		}

		if (last) {
			// Calculate chain length.
			int chainlen = iIndex - iChainStart;
			if (chainlen > 255)
				return false;

			// Set strip chain start,end and length.
			pStripIndex[iChainStart] = CRC_RIP_TRISTRIP + (chainlen << 8);
			pStripIndex[iChainStart + 1] = chainlen - 2;
		}
	}

	pStripIndex[iIndex++] = CRC_RIP_END + (1 << 8);

//	ConOut(CStrF("iVertex = %i, iIndex = %i", iVertex, iIndex));

	if (!_pVB->AllocVBChain(_pVBM, false))
		return false;
	_pVB->Geometry_VertexArray(pVertexPos, iVertex, true);
	_pVB->Geometry_TVertexArray(pVertexTex, 0);
	_pVB->Geometry_ColorArray(pVertexColor);
//	_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
	_pVB->Render_IndexedPrimitives(pStripIndex, iIndex);

	return true;
}

//----------------------------------------------------------------------
//----------------------------------------------------------------------
//----------------------------------------------------------------------

bool CXR_Util::Render_QuadStrip(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVB, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat, const CXR_QuadStrip *_pQuads, int _nQuads, int _Flags)
{
	MAUTOSTRIP(CXR_Util_Render_QuadStrip, false);
	if(_nQuads > MAXQUADS - 1 || _nQuads <= 2)
		return false;

	CMat4Dfp32 *pVMove = _pVBM->Alloc_M4();
	if(!pVMove)
		return false;

	int iSplit = 1;
	if(_Flags & CXR_QUADFLAGS_SPLITX2)
		iSplit = 2;
	if(_Flags & CXR_QUADFLAGS_SPLITX4)
		iSplit = 4;
	if(_Flags & CXR_QUADFLAGS_SPLITX8)
		iSplit = 8;

	CVec3Dfp32 *pV = _pVBM->Alloc_V3((1 + iSplit) * _nQuads);
	if(!pV)
		return false;
	CVec2Dfp32 *pTV = _pVBM->Alloc_V2((1 + iSplit) * _nQuads);
	if(!pTV)
		return false;
	CPixel32 *pC = _pVBM->Alloc_CPixel32((1 + iSplit) * _nQuads);
	if(!pC)
		return false;
	uint16 *pTri = _pVBM->Alloc_Int16(6 * _nQuads * iSplit);
	if(!pTri)
		return false;

	_WMat.Multiply(_VMat, *pVMove);

	int iV = 0;
	int iTri = 0;
	fp32 SplitAdd = 1.0f / fp32(iSplit);
	for (int b = 0; b < _nQuads; b++)
	{
		if(iSplit == 1)
		{
			pV[iV + 0] = _pQuads[b].m_Pos0;
			pV[iV + 1] = _pQuads[b].m_Pos1;
			pC[iV + 0] = _pQuads[b].m_Color0;
			pC[iV + 1] = _pQuads[b].m_Color1;
			pTV[iV + 0][0] = 1;
			pTV[iV + 0][1] = _pQuads[b].m_TextureYOfs;
			pTV[iV + 1][0] = 0;
			pTV[iV + 1][1] = _pQuads[b].m_TextureYOfs;

			if(b != 0)
			{
				pTri[iTri++] = iV - 2;
				pTri[iTri++] = iV - 1;
				pTri[iTri++] = iV + 0;
				pTri[iTri++] = iV - 1;
				pTri[iTri++] = iV + 0;
				pTri[iTri++] = iV + 1;
			}
			iV += 2;
		}
		else
		{
			pV[iV + 0] = _pQuads[b].m_Pos0;
			pC[iV + 0] = _pQuads[b].m_Color0;
			pTV[iV + 0][0] = 1;
			pTV[iV + 0][1] = _pQuads[b].m_TextureYOfs;
			CPixel32 Color0 = _pQuads[b].m_Color0;
			CPixel32 Color1 = _pQuads[b].m_Color1;
			fp32 Split = SplitAdd;
			for(int i = 1; i < iSplit; i++)
			{
				pV[iV + i] = _pQuads[b].m_Pos0 + (_pQuads[b].m_Pos1 - _pQuads[b].m_Pos0) * Split;
				pC[iV + i] = Color0;//Color0.AlphaBlendRGBA(Color1, TruncToInt(Split * 255));
				pTV[iV + i][0] = 1.0f - Split;
				pTV[iV + i][1] = _pQuads[b].m_TextureYOfs;
				Split += SplitAdd;
			}
			pV[iV + iSplit] = _pQuads[b].m_Pos1;
			pC[iV + iSplit] = _pQuads[b].m_Color1;
			pTV[iV + iSplit][0] = 0;
			pTV[iV + iSplit][1] = _pQuads[b].m_TextureYOfs;

			if(b != 0)
			{
				for(int i = 0; i < iSplit; i++)
				{
					pTri[iTri++] = i + iV - (1 + iSplit);
					pTri[iTri++] = i + iV + 1 - (1 + iSplit);
					pTri[iTri++] = i + iV + 0;
					pTri[iTri++] = i + iV + 1 - (1 + iSplit);
					pTri[iTri++] = i + iV + 0;
					pTri[iTri++] = i + iV + 1;
				}
			}

			iV += 1 + iSplit;
		}
	}

	if(iTri == 0)
		return false;

	_pVB->Matrix_Set(pVMove);

	if (!_pVB->AllocVBChain(_pVBM, false))
		return false;
	_pVB->Geometry_VertexArray(pV, (1 + iSplit) * _nQuads, true);
	_pVB->Geometry_TVertexArray(pTV, 0);
	_pVB->Geometry_ColorArray(pC);

	_pVB->Render_IndexedTriangles(pTri, iTri / 3);
	return true;
}

CXR_VertexBuffer *CXR_Util::Create_Box(CXR_VBManager *_pVBM, const CMat4Dfp32 &_L2V, const CBox3Dfp32 &_Box, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_Util_Create_Box, NULL);
	CXR_VertexBuffer *pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB);
	if(!pVB)
		return NULL;

	CVec3Dfp32 *pV = _pVBM->Alloc_V3(8);
	if(!pV) return NULL;
	uint16 *pWire = _pVBM->Alloc_Int16(24);
	if(!pWire) return NULL;

	for(int i = 0; i < 8; i++)
	{
		pV[i] = CVec3Dfp32((((i >> 0) & 1) ? _Box.m_Min[0] : _Box.m_Max[0]),
						  (((i >> 1) & 1) ? _Box.m_Min[1] : _Box.m_Max[1]), 
						  (((i >> 2) & 1) ? _Box.m_Min[2] : _Box.m_Max[2])) * _L2V;
	}

	int iWire = 0;
	pWire[iWire++] = 0; pWire[iWire++] = 1;
	pWire[iWire++] = 1; pWire[iWire++] = 3;
	pWire[iWire++] = 3; pWire[iWire++] = 2;
	pWire[iWire++] = 2; pWire[iWire++] = 0;
	pWire[iWire++] = 4; pWire[iWire++] = 5;
	pWire[iWire++] = 5; pWire[iWire++] = 7;
	pWire[iWire++] = 7; pWire[iWire++] = 6;
	pWire[iWire++] = 6; pWire[iWire++] = 4;
	pWire[iWire++] = 0; pWire[iWire++] = 4;
	pWire[iWire++] = 1; pWire[iWire++] = 5;
	pWire[iWire++] = 2; pWire[iWire++] = 6;
	pWire[iWire++] = 3; pWire[iWire++] = 7;

	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);

	if (!pVB->AllocVBChain(_pVBM, false))
		return NULL;
	pVB->Geometry_VertexArray(pV, 8, true);
	pVB->Geometry_Color(_Color);

	pVB->Render_IndexedWires(pWire, 24);

	return pVB;
}

CXR_VertexBuffer *CXR_Util::Create_Sphere(CXR_VBManager *_pVBM, const CMat4Dfp32 &_L2V, fp32 _Radius, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_Util_Create_Sphere, NULL);
	CXR_VertexBuffer *pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB);
	if(!pVB)
		return NULL;

	const int nSubDiv = 16;
	CVec3Dfp32 *pV = _pVBM->Alloc_V3(nSubDiv * 3);
	if(!pV) return NULL;
	uint16 *pWire = _pVBM->Alloc_Int16(nSubDiv * 2 * 3);
	if(!pWire) return NULL;

	int iV = 0;
	int iWire = 0;
	for(int i = 0; i < nSubDiv; i++)
	{
		if(i != nSubDiv - 1)
		{
			pWire[iWire++] = iV;
			pWire[iWire++] = iV + 3;
			pWire[iWire++] = iV + 1;
			pWire[iWire++] = iV + 4;
			pWire[iWire++] = iV + 2;
			pWire[iWire++] = iV + 5;
		}
		else
		{
			pWire[iWire++] = iV;
			pWire[iWire++] = 0;
			pWire[iWire++] = iV + 1;
			pWire[iWire++] = 1;
			pWire[iWire++] = iV + 2;
			pWire[iWire++] = 2;
		}

		fp32 QSin, QCos;
		QSinCos(_PI2 * float(i) / nSubDiv, QSin, QCos);
		pV[iV++] = CVec3Dfp32(0, _Radius * QSin, _Radius * QCos) * _L2V;
		pV[iV++] = CVec3Dfp32(_Radius * QSin, 0, _Radius * QCos) * _L2V;
		pV[iV++] = CVec3Dfp32(_Radius * QSin, _Radius * QCos, 0) * _L2V;
	}

	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);

	if (!pVB->AllocVBChain(_pVBM, false))
		return NULL;
	pVB->Geometry_VertexArray(pV, nSubDiv * 3, true);
	pVB->Geometry_Color(_Color);

	pVB->Render_IndexedWires(pWire, nSubDiv * 2 * 3);

	return pVB;
}

CXR_VertexBuffer *CXR_Util::Create_Star(CXR_VBManager *_pVBM, const CMat4Dfp32 &_L2V, fp32 _Radius, CPixel32 _Color)
{
	MAUTOSTRIP(CXR_Util_Create_Star, NULL);
	CXR_VertexBuffer *pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB);
	if(!pVB)
		return NULL;

	CVec3Dfp32 *pV = _pVBM->Alloc_V3(18);
	if(!pV) return NULL;
	uint16 *pWire = _pVBM->Alloc_Int16(2 * 9);
	if(!pWire) return NULL;

	pV[0] = CVec3Dfp32(_Radius, 0, 0) * _L2V;
	pV[1] = CVec3Dfp32(-_Radius, 0, 0) * _L2V;
	pV[2] = CVec3Dfp32(0, _Radius, 0) * _L2V;
	pV[3] = CVec3Dfp32(0, -_Radius, 0) * _L2V;
	pV[4] = CVec3Dfp32(0, 0, _Radius) * _L2V;
	pV[5] = CVec3Dfp32(0, 0, -_Radius) * _L2V;

	float R2 = 0.707f * _Radius;
	pV[6] = CVec3Dfp32(R2, R2, 0) * _L2V;
	pV[7] = CVec3Dfp32(-R2, -R2, 0) * _L2V;
	pV[8] = CVec3Dfp32(0, R2, R2) * _L2V;
	pV[9] = CVec3Dfp32(0, -R2, -R2) * _L2V;
	pV[10] = CVec3Dfp32(R2, 0, R2) * _L2V;
	pV[11] = CVec3Dfp32(-R2, 0, -R2) * _L2V;

	pV[12] = CVec3Dfp32(-R2, R2, 0) * _L2V;
	pV[13] = CVec3Dfp32(R2, -R2, 0) * _L2V;
	pV[14] = CVec3Dfp32(0, -R2, R2) * _L2V;
	pV[15] = CVec3Dfp32(0, R2, -R2) * _L2V;
	pV[16] = CVec3Dfp32(-R2, 0, R2) * _L2V;
	pV[17] = CVec3Dfp32(R2, 0, -R2) * _L2V;

	pWire[0] = 0;
	pWire[1] = 1;
	pWire[2] = 2;
	pWire[3] = 3;
	pWire[4] = 4;
	pWire[5] = 5;

	pWire[6] = 6;
	pWire[7] = 7;
	pWire[8] = 8;
	pWire[9] = 9;
	pWire[10] = 10;
	pWire[11] = 11;

	pWire[12] = 12;
	pWire[13] = 13;
	pWire[14] = 14;
	pWire[15] = 15;
	pWire[16] = 16;
	pWire[17] = 17;

	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);

	if (!pVB->AllocVBChain(_pVBM, false))
		return NULL;
	pVB->Geometry_VertexArray(pV, 18, true);
	pVB->Geometry_Color(_Color);

	pVB->Render_IndexedWires(pWire, 2 * 9);

	return pVB;
}

CXR_VertexBuffer *CXR_Util::Create_SOR(CXR_VBManager *_pVBM, const CMat4Dfp32 &_L2V, const CVec3Dfp32 *_pEdge, int _nEdgeVertices, fp32 _NumSegments, fp32 _StartAngle, int _iMappingType, float _MappingRadius)
{
	MAUTOSTRIP(CXR_Util_Create_SOR, NULL);
	int nSegments = TruncToInt(_NumSegments + 0.99999f);

	int nVertices = nSegments * (_nEdgeVertices + 1);
	CXR_VertexBuffer *pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES | CXR_VB_TVERTICES0, nVertices);
	if(!pVB)
		return NULL;

	int nTriangles = (nSegments * (_nEdgeVertices - 1)) * 2;
	uint16 *pTris = _pVBM->Alloc_Int16(nTriangles * 3);
	if(!pTris)
		return NULL;

	CXR_VBChain *pChain = pVB->GetVBChain();

	CVec3Dfp32 *pV = pChain->m_pV;
	fp32 *pTV = pChain->m_pTV[0];
	int nTVComp = pChain->m_nTVComp[0];

	fp32 Angle = _StartAngle;
	fp32 UVAngle = 0;
	fp32 dAngle = _PI2 / _NumSegments;
	
	fp32 dU = 1.0f / _NumSegments;
	fp32 dV = 1.0f / (_nEdgeVertices - 1);
	fp32 U = 0;

	fp32 InvMappingRadius = 1.0f;
	if(_iMappingType == 1)
		InvMappingRadius = 0.5f / _MappingRadius;

	//Create vertices
	int iV = 0;
	for(int s = 0; s < nSegments; s++)
	{
		float eSin, eCos, uvSin, uvCos;
		QSinCos(Angle, eSin, eCos);
		QSinCos(UVAngle, uvSin, uvCos);

		if(_iMappingType == 0)
		{
			fp32 V = 0;
			for(int e = 0; e < _nEdgeVertices; e++)
			{
				pV[iV] = CVec3Dfp32(_pEdge[e][0] * eCos - _pEdge[e][1] * eSin,
								   _pEdge[e][0] * eSin + _pEdge[e][1] * eCos,
								   _pEdge[e][2]);
				
				pTV[iV*nTVComp] = U;
				pTV[iV*nTVComp+1] = V;
				pV[iV] *= _L2V;

				iV++;
				V += dV;
			}
		}
		else if(_iMappingType == 1)
		{
			fp32 V = 0;
			for(int e = 0; e < _nEdgeVertices; e++)
			{
				pV[iV] = CVec3Dfp32(_pEdge[e][0] * eCos - _pEdge[e][1] * eSin,
								   _pEdge[e][0] * eSin + _pEdge[e][1] * eCos,
								   _pEdge[e][2]);
				
				
				pTV[iV*nTVComp] = (_pEdge[e][0] * uvCos - _pEdge[e][1] * uvSin) * InvMappingRadius + 0.5f;
				pTV[iV*nTVComp+1] = (_pEdge[e][0] * uvSin + _pEdge[e][1] * uvCos) * InvMappingRadius + 0.5f;
//				pTV[iV] = CVec2Dfp32(pV[iV][0] * InvMappingRadius + 0.5f, pV[iV][1] * InvMappingRadius + 0.5f);
				pV[iV] *= _L2V;

				iV++;
				V += dV;
			}
		}

		Angle += dAngle;
		UVAngle += dAngle;
		U += dU;
	}

	{
		//Create "wrap"-vertices
		fp32 V = 0;
		if(_iMappingType == 0)
		{
			for(int e = 0; e < _nEdgeVertices; e++)
			{
				pV[iV] = pV[e];
				pTV[iV*nTVComp] = 1.0f;
				pTV[iV*nTVComp+1] = V;
				iV++;
				V += dV;
			}
		}
		else if(_iMappingType == 1)
		{
			for(int e = 0; e < _nEdgeVertices; e++)
			{
				pV[iV] = pV[e];
				pTV[iV*nTVComp] = pTV[e*nTVComp];
				pTV[iV*nTVComp+1] = pTV[e*nTVComp+1];
				iV++;
			}
		}
	}

	//Create tri-strips
	int iTri = 0;
	for(int e = 0; e < _nEdgeVertices - 1; e++)
	{
		int iBase = e;
		for(int s = 0; s < nSegments; s++)
		{
			pTris[iTri++] = iBase + 1;
			pTris[iTri++] = iBase;
			pTris[iTri++] = iBase + _nEdgeVertices;

			pTris[iTri++] = iBase + 1;
			pTris[iTri++] = iBase + _nEdgeVertices;
			pTris[iTri++] = iBase + _nEdgeVertices + 1;

			iBase += _nEdgeVertices;
		}
	}

	pVB->Render_IndexedTriangles(pTris, nTriangles);

	return pVB;
}

CXR_VertexBuffer *CXR_Util::Create_SOR(CXR_VBManager *_pVBM, const CMat4Dfp32 &_L2V, const CVec3Dfp32 *_pEdge, const CPixel32 *_pColors, int _nEdgeVertices, fp32 _NumSegments, fp32 _StartAngle)
{
	MAUTOSTRIP(CXR_Util_Create_SOR_2, NULL);
	CXR_VertexBuffer *pVB = Create_SOR(_pVBM, _L2V, _pEdge, _nEdgeVertices, _NumSegments, _StartAngle);
	CXR_VBChain *pChain = pVB->GetVBChain();
	int nVert = pChain->m_nV;
	pChain->m_pCol = _pVBM->Alloc_CPixel32(nVert);
	if(!pChain->m_pCol)
		return NULL;

	int nSegments = TruncToInt(_NumSegments + 0.99999f);
	int iV = 0;
	for(int s = 0; s < nSegments + 1; s++)
		for(int e = 0; e < _nEdgeVertices; e++)
			pChain->m_pCol[iV++] = _pColors[e];

	return pVB;
}

CXR_VertexBuffer *CXR_Util::Create_Cylinder(CXR_VBManager *_pVBM, const CMat4Dfp32 &_L2V, int _Axis, fp32 _Height, fp32 _Radius, CPixel32 _Color, int _nSegments, int _nSlices)
{
	MAUTOSTRIP(CXR_Util_Create_Cylinder, NULL);
	CXR_VertexBuffer *pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB);
	if(!pVB)
		return NULL;

	const int nVertices = _nSegments * _nSlices * 2 + _nSlices*2;
	const int nWires = _nSegments * _nSlices * 2 + _nSlices*2;

	CVec3Dfp32 *pV = _pVBM->Alloc_V3(nVertices);
	if(!pV) return NULL;
	uint16 *pWire = _pVBM->Alloc_Int16(nWires);
	if(!pWire) return NULL;

	M_ASSERT(_Axis <= 2, "");
	fp32 DAxis = _Height / _nSegments;
	int SegmentAxis1, SegmentAxis2;
	if (_Axis == 0)
	{
		SegmentAxis1 = 1;
		SegmentAxis2 = 2;
	}
	else if (_Axis == 1)
	{
		SegmentAxis1 = 0;
		SegmentAxis2 = 2;
	}
	else 
	{
		SegmentAxis1 = 0;
		SegmentAxis2 = 1;
	}

	int iV = 0;
	int iWire = 0;
	for (int i = 0; i < _nSegments; i++)
	{
		for (int j = 0; j < _nSlices; j++)
		{
			fp32 x = _Radius * M_Cos(2.0f * _PI * j / fp32(_nSlices) );
			fp32 y = _Radius * M_Sin(2.0f * _PI * j / fp32(_nSlices) );

			CVec3Dfp32 pos1;
			pos1[SegmentAxis1] = x;
			pos1[SegmentAxis2] = y;
			pos1[_Axis] = DAxis*i - _Height/2.0;

			x = _Radius * M_Cos(2.0f * _PI * (j+1) / fp32(_nSlices) );
			y = _Radius * M_Sin(2.0f * _PI * (j+1) / fp32(_nSlices) );
			CVec3Dfp32 pos2;
			pos2[SegmentAxis1] = x;
			pos2[SegmentAxis2] = y;
			pos2[_Axis] = DAxis*i - _Height/2.0;

			pV[iV++] = pos1 * _L2V;
			pV[iV++] = pos2 * _L2V;

			pWire[iWire++] = iV - 2;
			pWire[iWire++] = iV - 1;

			if (i == 0)
			{
				pos2 = pos1;
				pos1[_Axis] = -_Height/2.0;
				pos2[_Axis] = +_Height/2.0;
				pV[iV++] = pos1 * _L2V;
				pV[iV++] = pos2 * _L2V;

				pWire[iWire++] = iV - 2;
				pWire[iWire++] = iV - 1;
			}
		}
	}

	M_ASSERT(iV == nVertices, "Internal error!");
	M_ASSERT(iWire == nWires, "Internal error!");

	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);

	if (!pVB->AllocVBChain(_pVBM, false))
		return NULL;
	pVB->Geometry_VertexArray(pV, nVertices, true);
	pVB->Geometry_Color(_Color);

	pVB->Render_IndexedWires(pWire, nWires);

	return pVB;
}

void CXR_Util::VB_ScrollTexture(CXR_VertexBuffer *_pVB, const CVec2Dfp32 &_Ofs, int _iTextureBank)
{
	MAUTOSTRIP(CXR_Util_VB_ScrollTexture, MAUTOSTRIP_VOID);
	CXR_VBChain *pChain = _pVB->GetVBChain();
	if(!pChain->m_pTV)
		return;

	fp32 *pTV = pChain->m_pTV[_iTextureBank];
	int nTVComp = pChain->m_nTVComp[_iTextureBank];
	int nVert = pChain->m_nV;
	for(int v = 0; v < nVert; v++)
	{
		pTV[v*nTVComp] += _Ofs[0];
		pTV[v*nTVComp+1] += _Ofs[1];
	}
}

void CXR_Util::VB_RenderDebug(CXR_VBManager *_pVBM, CXR_VertexBuffer *_pVBSource, int _iType, int _Color)
{
	MAUTOSTRIP(CXR_Util_VB_RenderDebug, MAUTOSTRIP_VOID);
	if(_iType & CXR_DEBUGVB_VERTICES)
	{
		CXR_VertexBuffer *pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB);
		if(!pVB)
			return;

//		CXR_VBChain *pChain = pVB->GetVBChain();
		CXR_VBChain *pChainSrc = _pVBSource->GetVBChain();

		int nVert = pChainSrc->m_nV;
		if(nVert > CXR_Util::MAXPARTICLES)
		{
			ConOut("CXR_Util::CreateDebugVB, Too many particles");
			nVert = CXR_Util::MAXPARTICLES;
		}

		CXR_Particle lParticles[CXR_Util::MAXPARTICLES];
		for(int i = 0; i < nVert; i++)
		{
			lParticles[i].m_Pos = pChainSrc->m_pV[i];
			lParticles[i].m_Color = _Color;
		}
		CMat4Dfp32 Mat;
		Mat.Unit();

		if (_pVBSource->m_pTransform) 
		{
			Mat = *_pVBSource->m_pTransform;
		}

		if(CXR_Util::Render_Particles(_pVBM, pVB, Mat, lParticles, nVert, 5))
			_pVBM->AddVB(pVB);
	}

	if(_iType & CXR_DEBUGVB_TEXCOORDS)
	{
		CXR_VBChain *pChainSrc = _pVBSource->GetVBChain();
		if(pChainSrc->m_PrimType == CRC_RIP_TRIANGLES)
		{
			if(!pChainSrc->m_pTV[0])
				return;

			CXR_VertexBuffer *pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB);
			if(!pVB)
				return;

			CXR_VBChain *pChain = pVB->GetVBChain();

			pChain->m_pV = pChainSrc->m_pV;
			pChain->m_nPrim = pChainSrc->m_nPrim;
			pChain->m_piPrim = pChainSrc->m_piPrim;

			int nVert = pChainSrc->m_nV;
			pChain->m_pCol = _pVBM->Alloc_CPixel32(nVert);
			if(!pChain->m_pCol)
				return;

			fp32* pTV = pChainSrc->m_pTV[0];
			int nTVComp = pChainSrc->m_nTVComp[0];

			for(int v = 0; v < nVert; v++)
				pChain->m_pCol[v] = CPixel32(int(Clamp01(pTV[v*nTVComp]) * 255),
										  int(Clamp01(pTV[v*nTVComp+1]) * 255),
										  0, 255);

			_pVBM->AddVB(pVB);
		}
	}

	if(_iType & CXR_DEBUGVB_TRIANGLES)
	{
		CXR_VBChain *pChainSrc = _pVBSource->GetVBChain();
		if(pChainSrc->m_PrimType == CRC_RIP_TRIANGLES)
		{
			int nVert = pChainSrc->m_nV;
			CXR_VertexBuffer *pVB = _pVBM->Alloc_VB(CXR_VB_ATTRIB | CXR_VB_VERTICES, nVert);
			if(!pVB)
				return;

			CXR_VBChain *pChain = pVB->GetVBChain();

			pChain->m_pV = pChainSrc->m_pV;

			int nTris = pChainSrc->m_nPrim;
			uint16 *pWire = _pVBM->Alloc_Int16(nTris * 6);
			if(!pWire)
				return;

			int iW = 0;
			int iTri = 0;
			for(int t = 0; t < nTris; t++)
			{
				pWire[iW++] = pChainSrc->m_piPrim[iTri + 0];
				pWire[iW++] = pChainSrc->m_piPrim[iTri + 1];

				pWire[iW++] = pChainSrc->m_piPrim[iTri + 1];
				pWire[iW++] = pChainSrc->m_piPrim[iTri + 2];
				
				pWire[iW++] = pChainSrc->m_piPrim[iTri + 2];
				pWire[iW++] = pChainSrc->m_piPrim[iTri + 0];
				iTri += 3;
			}

			pVB->Render_IndexedWires(pWire, iW);
			pVB->Geometry_Color(_Color);
			_pVBM->AddVB(pVB);
		}
	}

	if(_iType & CXR_DEBUGVB_NORMALS)
	{
	}
}

CXR_VertexBuffer* CXR_Util::VBM_RenderRect(CXR_VBManager* _pVBM, const CMat4Dfp32 *M_RESTRICT _pMat, const CScissorRect& _Rect, CPixel32 _Color, fp32 _Priority, CRC_Attributes* _pA)
{
	CXR_VertexBuffer* pVB = _pVBM->Alloc_VB(CXR_VB_VERTICES, 4);
	if (!pVB)
		return NULL;

	pVB->m_pAttrib = _pA;
	CXR_VBChain *pChain = pVB->GetVBChain();
	CVec3Dfp32* pV = pChain->m_pV;

	uint32 MinX, MinY, MaxX, MaxY;
	_Rect.GetRect(MinX, MinY, MaxX, MaxY);

	CVec3Dfp32(MaxX, MaxY, 0).MultiplyMatrix(*_pMat, pV[0]);
	CVec3Dfp32(MinX, MaxY, 0).MultiplyMatrix(*_pMat, pV[1]);
	CVec3Dfp32(MinX, MinY, 0).MultiplyMatrix(*_pMat, pV[2]);
	CVec3Dfp32(MaxX, MinY, 0).MultiplyMatrix(*_pMat, pV[3]);

	pVB->m_Priority = _Priority;
	CXR_Util::Init();
	pVB->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, 2);
	pVB->m_Color = _Color;
	return pVB;
}

void CXR_Util::VBM_RenderRect(CXR_VBManager* _pVBM, CXR_VertexBuffer* _pVB, CXR_VBChain* _pVBChain, CRC_Attributes* _pA, CPixel32 _Color, fp32 _Priority)
{
	_pVB->m_pAttrib = _pA;
	_pVB->SetVBChain(_pVBChain);
	_pVB->m_Priority = _Priority;
	_pVB->m_Color = _Color;
	_pVBM->AddVB(_pVB);
}

void CXR_Util::VBM_RenderRect(CXR_VBManager* _pVBM, const CMat4Dfp32 *M_RESTRICT _pMat, CXR_VertexBuffer* _pVB, const CRect2Duint16& _Rect, CPixel32 _Color, fp32 _Priority)
{
	CXR_VBChain* pChain = _pVB->GetVBChain();
	CVec3Dfp32* pV = pChain->m_pV;

	VBM_CreateRect(_pMat, _Rect, pV);

	_pVB->m_Priority = _Priority;
	CXR_Util::Init();
	_pVB->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, 2);
	_pVB->m_Color = _Color;

	_pVBM->AddVB(_pVB);
}

CXR_VertexBuffer* CXR_Util::VBM_RenderRect(CXR_VBManager* _pVBM, const CMat4Dfp32 *M_RESTRICT _pMat, const CRect2Duint16& _Rect, CPixel32 _Color, fp32 _Priority, CRC_Attributes* _pA)
{
	CXR_VertexBuffer* pVB = _pVBM->Alloc_VB(CXR_VB_VERTICES, 4);
	if (!pVB)
		return NULL;

	pVB->m_pAttrib = _pA;
	CXR_VBChain *pChain = pVB->GetVBChain();
	CVec3Dfp32* pV = pChain->m_pV;

	VBM_CreateRect(_pMat, _Rect, pV);

	pVB->m_Priority = _Priority;
	CXR_Util::Init();
	pVB->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, 2);
	pVB->m_Color = _Color;
	return pVB;
}

void CXR_Util::VBM_CreateRect(const CMat4Dfp32 *M_RESTRICT _pMat, const CRect2Duint16& _Rect, CVec3Dfp32* _pV)
{
	CVec3Dfp32(_Rect.m_Max[0], _Rect.m_Max[1], 0).MultiplyMatrix(*_pMat, _pV[0]);
	CVec3Dfp32(_Rect.m_Min[0], _Rect.m_Max[1], 0).MultiplyMatrix(*_pMat, _pV[1]);
	CVec3Dfp32(_Rect.m_Min[0], _Rect.m_Min[1], 0).MultiplyMatrix(*_pMat, _pV[2]);
	CVec3Dfp32(_Rect.m_Max[0], _Rect.m_Min[1], 0).MultiplyMatrix(*_pMat, _pV[3]);
}

CVec3Dfp32* CXR_Util::VBM_CreateRect(CXR_VBManager* _pVBM, const CMat4Dfp32 *M_RESTRICT _pMat, const CRect2Duint16& _Rect)
{
	CVec3Dfp32* pV = _pVBM->Alloc_V3(4);
	if (!pV)
		return NULL;

	VBM_CreateRect(_pMat, _Rect, pV);
	return pV;
}


void CXR_Util::VBM_CreateRectUV(const CVec2Dfp32& _UVMin, const CVec2Dfp32& _UVMax, int _bVFlip, CVec2Dfp32* _pTV)
{
	if (_bVFlip)
	{
		_pTV[0][0] = _UVMax[0];		_pTV[0][1] = 1.0f - _UVMax[1];
		_pTV[1][0] = _UVMin[0];		_pTV[1][1] = 1.0f - _UVMax[1];
		_pTV[2][0] = _UVMin[0];		_pTV[2][1] = 1.0f - _UVMin[1];
		_pTV[3][0] = _UVMax[0];		_pTV[3][1] = 1.0f - _UVMin[1];
	}
	else
	{
		_pTV[0][0] = _UVMax[0];		_pTV[0][1] = _UVMax[1];
		_pTV[1][0] = _UVMin[0];		_pTV[1][1] = _UVMax[1];
		_pTV[2][0] = _UVMin[0];		_pTV[2][1] = _UVMin[1];
		_pTV[3][0] = _UVMax[0];		_pTV[3][1] = _UVMin[1];
	}
}

CVec2Dfp32* CXR_Util::VBM_CreateRectUV(CXR_VBManager* _pVBM, const CVec2Dfp32& _UVMin, const CVec2Dfp32& _UVMax)
{
	CVec2Dfp32* pTV = _pVBM->Alloc_V2(4);
	if (!pTV)
		return NULL;

	VBM_CreateRectUV(_UVMin, _UVMax, false, pTV);
	return pTV;
}

CVec2Dfp32* CXR_Util::VBM_CreateRectUV_VFlip(CXR_VBManager* _pVBM, const CVec2Dfp32& _UVMin, const CVec2Dfp32& _UVMax)
{
	CVec2Dfp32* pTV = _pVBM->Alloc_V2(4);
	if (!pTV)
		return NULL;

	VBM_CreateRectUV(_UVMin, _UVMax, true, pTV);
	return pTV;
}

M_FORCEINLINE void M_VClipLine2D(vec128& _p0, vec128& _p1, vec128 _min, vec128 _max, uint32& _bLine)
{
	vec128 p0 = _p0;
	vec128 p1 = _p1;
	vec128 v01 = M_VSub(p1, p0);
	vec128 vrcp = M_VAbs(M_VRcp(v01));
	vec128 zero = M_VZero();
	vec128 p0min = M_VMax(zero, M_VSub(_min, p0));
	vec128 maxp0 = M_VMax(zero, M_VSub(p0, _max));
	vec128 p0maxclip = M_VMul(vrcp, M_VMax(p0min, maxp0));
	p0maxclip = M_VMax(M_VSplatX(p0maxclip), M_VSplatY(p0maxclip));
	p0 = M_VMAdd(v01, p0maxclip, p0);

	vec128 p1min = M_VMax(zero, M_VSub(_min, p1));
	vec128 maxp1 = M_VMax(zero, M_VSub(p1, _max));
	vec128 p1maxclip = M_VMul(vrcp, M_VMax(p1min, maxp1));
	p1maxclip = M_VMax(M_VSplatX(p1maxclip), M_VSplatY(p1maxclip));
	p1 = M_VMAdd(M_VNeg(v01), p1maxclip, p1);

	vec128 pmin = M_VMin(p0, p1);
	vec128 pmax = M_VMax(p0, p1);
	vec128 cull = M_VOr(M_VCmpGTMsk(pmax, _max), M_VCmpLTMsk(pmin, _min));
	cull = M_VOr(M_VSplatX(cull), M_VSplatY(cull));

	vec128 maxclip = M_VSelMsk(cull, M_VOne(), M_VMax(p0maxclip, p1maxclip));

	_p0 = p0;
	_p1 = p1;
	M_VStAny32(M_VCmpLTMsk(maxclip, M_VOne()), &_bLine);
}

CXR_VertexBuffer* CXR_Util::VBM_RenderWires2D(CXR_VBManager* _pVBM, const CClipRect& _Clip, const CVec2Dfp32 *_p0, const CVec2Dfp32 *_p1, const CPixel32 *_Color0, const CPixel32 *_Color1, int _nWires, CRC_Attributes* _pA)
{
	CXR_VertexBuffer *pVB = _pVBM->Alloc_VB(CXR_VB_VERTICES|CXR_VB_COLORS, _nWires * 2);
	if(!pVB)
		return NULL;
	uint16 *pInd = (uint16 *)_pVBM->Alloc_Int16(_nWires * 2);
	if (!pInd)
		return NULL;
	uint32*M_RESTRICT pInd32 = (uint32*)pInd;

	CXR_VBChain *pVBChain = pVB->GetVBChain();
	CPixel32*M_RESTRICT pCol = pVBChain->m_pCol;

	CVec3Dfp32 Verts[2];
	int iDest = 0;
	vec128 Mask = M_VConstMsk(1,1,0,0);
	vec128 Maskv = M_VConst(0.0f,0.0f,0.0f,1.0f);
	CMat4Dfp32 Mat;
	_pVBM->Viewport_Get()->Get2DMatrix_RelBackPlane(Mat);
	vec128 clipofs = M_VCnv_i32_f32(M_VMrgXY(M_VLdScalar_i32(_Clip.ofs.x), M_VLdScalar_i32(_Clip.ofs.y)));
	vec128 clipmin = M_VCnv_i32_f32(M_VMrgXY(M_VLdScalar_i32(_Clip.clip.p0.x), M_VLdScalar_i32(_Clip.clip.p0.y)));
	vec128 clipmax = M_VCnv_i32_f32(M_VMrgXY(M_VLdScalar_i32(_Clip.clip.p1.x), M_VLdScalar_i32(_Clip.clip.p1.y)));
	int i = 0;
	for (; i < _nWires-1; i += 2)
	{
		CVec3Dfp32 * M_RESTRICT Verts3D = pVBChain->m_pV + i * 2;
		vec128 p0 = M_VMrgXY(M_VLdScalar(_p0[i].k[0]), M_VLdScalar(_p0[i].k[1]));
		vec128 p1 = M_VMrgXY(M_VLdScalar(_p1[i].k[0]), M_VLdScalar(_p1[i].k[1]));
		vec128 p2 = M_VMrgXY(M_VLdScalar(_p0[i+1].k[0]), M_VLdScalar(_p0[i+1].k[1]));
		vec128 p3 = M_VMrgXY(M_VLdScalar(_p1[i+1].k[0]), M_VLdScalar(_p1[i+1].k[1]));
		p0 = M_VSelMsk(Mask, M_VAdd(p0, clipofs), Maskv);
		p1 = M_VSelMsk(Mask, M_VAdd(p1, clipofs), Maskv);
		p2 = M_VSelMsk(Mask, M_VAdd(p2, clipofs), Maskv);
		p3 = M_VSelMsk(Mask, M_VAdd(p3, clipofs), Maskv);
		//			uint32 bLine0, bLine1;
		//			M_VClipLine2D(p0, p1, clipmin, clipmax, bLine0);
		//			M_VClipLine2D(p2, p3, clipmin, clipmax, bLine1);

		vec128 v0 = M_VMulMat(p0, Mat);
		vec128 v1 = M_VMulMat(p1, Mat);
		vec128 v2 = M_VMulMat(p2, Mat);
		vec128 v3 = M_VMulMat(p3, Mat);

		/*			CPnt p0 = _p0[i];
		CPnt p1 = _p1[i];

		p0 += _Clip.ofs;
		p1 += _Clip.ofs;

		if (!CImage::ClipLine(_Clip.clip, p0.x, p0.y, p1.x, p1.y)) 
		continue;
		*/
		M_VSt_V3x4(Verts3D, v0, v1, v2, v3);

		pCol[i * 2] = _Color0[i];
		pCol[i * 2 + 1] = _Color1[i];
		pCol[i * 2 + 2] = _Color0[i+1];
		pCol[i * 2 + 3] = _Color1[i+1];

		/*			Verts[0] = CVec3Dfp32(p0.x, p0.y, 0);
		Verts[1] = CVec3Dfp32(p1.x, p1.y, 0);
		CVec3Dfp32::MultiplyMatrix(Verts, Verts3D, m_CurTransform, 2);*/

		//			if (bLine0)
		pInd32[iDest++] = (i * 2) + ((i*2+1) << 16);
		//			if (bLine1)
		pInd32[iDest++] = (i * 2+2) + ((i*2+3) << 16);
		//			pInd[iDest++] = i * 2;
		//			pInd[iDest++] = i * 2 + 1;
	}
	for (; i < _nWires; i += 2)
	{
		CVec3Dfp32 *M_RESTRICT Verts3D = pVBChain->m_pV + i * 2;
		vec128 p0 = M_VMrgXY(M_VLdScalar(_p0[i].k[0]), M_VLdScalar(_p0[i].k[1]));
		vec128 p1 = M_VMrgXY(M_VLdScalar(_p1[i].k[0]), M_VLdScalar(_p1[i].k[1]));
		p0 = M_VSelMsk(Mask, M_VAdd(p0, clipofs), Maskv);
		p1 = M_VSelMsk(Mask, M_VAdd(p1, clipofs), Maskv);
		uint32 bLine0;
		M_VClipLine2D(p0, p1, clipmin, clipmax, bLine0);

		vec128 v0 = M_VMulMat(p0, Mat);
		vec128 v1 = M_VMulMat(p1, Mat);
		M_VStAny32(M_VSplatX(v0), &Verts3D[0].k[0]);
		M_VStAny32(M_VSplatY(v0), &Verts3D[0].k[1]);
		M_VStAny32(M_VSplatZ(v0), &Verts3D[0].k[2]);
		M_VStAny32(M_VSplatX(v1), &Verts3D[1].k[0]);
		M_VStAny32(M_VSplatY(v1), &Verts3D[1].k[1]);
		M_VStAny32(M_VSplatZ(v1), &Verts3D[1].k[2]);

		pCol[i * 2] = _Color0[i];
		pCol[i * 2 + 1] = _Color1[i];

		if (bLine0)
			pInd32[iDest++] = (i * 2) + ((i*2+1) << 16);
	}

	if (!iDest)
		return NULL;

	pVB->Render_IndexedWires(pInd, iDest*2);
	pVB->m_pAttrib = _pA;
	return pVB;
}

CVec2Dfp32 CXR_Util::VBM_Convert3DTo2DPosition(CRC_Viewport* _pVP, const CMat4Dfp32& _VProjMat, const CVec3Dfp32& _3DPos, bool _bUVSpace)
{
	CVec4Dfp32 PosTmp = CVec4Dfp32(_3DPos.k[0], _3DPos.k[1], _3DPos.k[2], 1.0f) * _VProjMat;
	fp32 w = 1.0f / PosTmp.k[3];

	CVec2Dfp32 Result(PosTmp.k[0] * w, PosTmp.k[1] * w);
	if (_bUVSpace)
	{
		const CRct& VRect = _pVP->GetViewRect();
		Result.k[0] = (Result.k[0] / fp32(VRect.GetWidth())) + 0.5f;
		Result.k[1] = (Result.k[1] / fp32(VRect.GetHeight())) + 0.5f;
	}

	return Result;
}

CVec2Dfp32 CXR_Util::VBM_Convert3DTo2DPosition(CRC_Viewport* _pVP, const CVec3Dfp32& _3DPos, const CMat4Dfp32& _VMat, bool _bUVSpace)
{
	CMat4Dfp32 VProjMat; _VMat.Multiply(_pVP->GetProjectionMatrix(), VProjMat);
	return VBM_Convert3DTo2DPosition(_pVP, VProjMat, _3DPos, _bUVSpace);
}

CVec2Dfp32 CXR_Util::VBM_Convert3DTo2DPosition(CXR_VBManager* _pVBM, const CMat4Dfp32& _VMat, const CVec3Dfp32& _3DPos, bool _bUVSpace)
{
	CRC_Viewport* pVP = _pVBM->Viewport_Get();
	CMat4Dfp32 VProjMat; _VMat.Multiply(pVP->GetProjectionMatrix(), VProjMat);
	return VBM_Convert3DTo2DPosition(pVP, VProjMat, _3DPos, _bUVSpace);
}

CVec2Dfp32 CXR_Util::VBM_Convert3DTo2DPosition(CXR_VBManager* _pVBM, const CMat4Dfp32& _VMat, const CMat4Dfp32& _3DPos, bool _bUVSpace)
{
	return VBM_Convert3DTo2DPosition(_pVBM, _VMat, _3DPos.GetRow(3), _bUVSpace);
}

// -------------------------------------------------------------------
void CXR_Util::Text_PatchAttribute(CRC_Font* _pF, CRC_Attributes* _pA)
{
	_pA->Attrib_TextureID(0, _pF->m_TextureID);
}

void CXR_Util::Text_SetAttribute(CRC_Font* _pF, CRC_Attributes* _pA)
{
	_pA->SetDefault();
	_pA->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
	_pA->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	_pA->Attrib_TextureID(0, _pF->m_TextureID);
}

CRC_Attributes* CXR_Util::Text_CreateAttribute(CXR_VBManager* _pVBM, CRC_Font* _pF)
{
	CRC_Attributes* pA = _pVBM->Alloc_Attrib();
	if (!pA)
		return NULL;

	Text_SetAttribute(_pF, pA);
	return pA;
}

CXR_VertexBuffer* CXR_Util::Text_Create(CXR_VBManager* _pVBM, CRC_Font* _pF, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const wchar* _pStr, 
					 const CVec2Dfp32& _Size, CPixel32 _Color, CRC_Attributes* _pA, const CVec2Dfp32& _MinLimit, const CVec2Dfp32& _MaxLimit)
{
	MAUTOSTRIP(CXR_Util_Text_Create_1, false);
	int len = CStr::StrLen(_pStr);
	if (len < 1)
		return NULL;

	int nV = len*4;
	int nP = len*6;

	int Contents = CXR_VB_VERTICES | CXR_VB_TVERTICES0 | CXR_VB_COLORS | ((_pA) ? 0 : CXR_VB_ATTRIB);
	CXR_VertexBuffer* pVB = _pVBM->Alloc_VB(Contents, nV);
	if (!pVB)
		return NULL;

	CXR_VBChain* pChain = pVB->GetVBChain();

	CVec3Dfp32* pV = pChain->m_pV;
	CVec2Dfp32* pTV = (CVec2Dfp32*) pChain->m_pTV[0];
	CPixel32* pCol = pChain->m_pCol;

	M_ASSERT(pV && pTV && pCol, "!");

	int nTri = _pF->Write(nV, pV, pTV, pCol, NULL, _Color, _Pos, _Dir, _VDown, _pStr, _Size, _MinLimit, _MaxLimit);
	if (!nTri)
		return NULL;

	if (nTri > CXR_Util::MAXPARTICLES*2)
		nTri = CXR_Util::MAXPARTICLES*2;

	pChain->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, nTri);
	pChain->m_nV = nTri * 2;

	if (!_pA)
		CXR_Util::Text_SetAttribute(_pF, pVB->m_pAttrib);
	else
		pVB->m_pAttrib = _pA;

	return pVB;
}

CXR_VertexBuffer* CXR_Util::Text_Create(CXR_VBManager* _pVBM, CRC_Font* _pF, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, const char* _pStr, 
					 const CVec2Dfp32& _Size, CPixel32 _Color, CRC_Attributes* _pA, const CVec2Dfp32& _MinLimit, const CVec2Dfp32& _MaxLimit)
{
	MAUTOSTRIP(CXR_Util_Text_Create_2, false);
	wchar Buffer[MAXSTRCONVERT];
	mint Len = MinMT(MAXSTRCONVERT-1, CStrBase::StrLen(_pStr));
	CStrBase::mfsncpy(Buffer, CSTR_FMT_UNICODE, _pStr, CSTR_FMT_ANSI, Len);
	Buffer[Len] = 0;

	return Text_Create(_pVBM, _pF, _Pos, _Dir, _VDown, Buffer, _Size, _Color, _pA, _MinLimit, _MaxLimit);
}

CXR_VertexBuffer* CXR_Util::Text_Create(CXR_VBManager* _pVBM, CRC_Font* _pF, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir, const CVec3Dfp32& _VDown, CStr& _Str, 
					 const CVec2Dfp32& _Size, CPixel32 _Color, CRC_Attributes* _pA, const CVec2Dfp32& _MinLimit, const CVec2Dfp32& _MaxLimit)
{
	MAUTOSTRIP(CXR_Util_Text_Create_3, false);
	if (_Str.IsUnicode())
		return Text_Create(_pVBM, _pF, _Pos, _Dir, _VDown, _Str.StrW(), _Size, _Color, _pA, _MinLimit, _MaxLimit);
	else
		return Text_Create(_pVBM, _pF, _Pos, _Dir, _VDown, _Str.Str(), _Size, _Color, _pA, _MinLimit, _MaxLimit);
}

CXR_VertexBuffer* CXR_Util::Text_Create(CXR_VBManager* _pVBM, CRC_Font* _pF, CMat4Dfp32* _pMat, const CVec2Dfp32& _Pos, const char* _pText, uint32 _Color, CRC_Attributes* _pA)
{
	CVec3Dfp32 Pos(_Pos[0], _Pos[1], 0);
	Pos *= *_pMat;
	CXR_VertexBuffer* pVB = Text_Create(_pVBM, _pF, Pos, _pMat->GetRow(0), _pMat->GetRow(1), _pText, _pF->GetOriginalSize(), _Color, _pA);
	return pVB;
}

CXR_VertexBuffer* CXR_Util::Text_Create(CXR_VBManager* _pVBM, CRC_Font* _pF, CMat4Dfp32* _pMat, const CVec2Dfp32& _Pos, const wchar* _pText, uint32 _Color, CRC_Attributes* _pA)
{
	CVec3Dfp32 Pos(_Pos[0], _Pos[1], 0);
	Pos *= *_pMat;
	CXR_VertexBuffer* pVB = Text_Create(_pVBM, _pF, Pos, _pMat->GetRow(0), _pMat->GetRow(1), _pText, _pF->GetOriginalSize(), _Color, _pA);
	return pVB;
}

bool CXR_Util::Text_Add(CXR_VBManager* _pVBM, CRC_Font* _pF, CMat4Dfp32* _pMat, const CVec2Dfp32& _Pos, const char* _pText, uint32 _Color, CRC_Attributes* _pA)
{
	CXR_VertexBuffer* pVB = Text_Create(_pVBM, _pF, _pMat, _Pos, _pText, _Color, _pA);
	if (!pVB)
		return false;
	_pVBM->AddVB(pVB);
	return true;
}

bool CXR_Util::Text_Add(CXR_VBManager* _pVBM, CRC_Font* _pF, CMat4Dfp32* _pMat, const CVec2Dfp32& _Pos, const wchar* _pText, uint32 _Color, CRC_Attributes* _pA)
{
	CXR_VertexBuffer* pVB = Text_Create(_pVBM, _pF, _pMat, _Pos, _pText, _Color, _pA);
	if (!pVB)
		return false;
	_pVBM->AddVB(pVB);
	return true;
}

// -------------------------------------------------------------------
bool CXR_Util::ShrinkTexture(const CXR_ShrinkParams *M_RESTRICT _pParams, CVec2Dfp32*& _pRetTVShrink)
{
	// Render gauss X & Y
	{
		const fp32 PriorityOffset = 0.01f;
		fp32 Priority = _pParams->m_Priority;
		_pRetTVShrink = NULL;

		CXR_VBManager* pVBM = _pParams->m_pVBM;
		CMat4Dfp32* pMat2D = pVBM->Alloc_M4_Proj2DRelBackPlane();
		if (!pMat2D)
			return false;

		uint bRenderTextureVertFlip  = _pParams->m_RenderCaps & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;

		CRect2Duint16 VPShrink = _pParams->m_Rect;

		CVec2Dfp32* pTV = (bRenderTextureVertFlip) ? 
			CXR_Util::VBM_CreateRectUV_VFlip(pVBM, _pParams->m_SrcUVMin, _pParams->m_SrcUVMax) :
		CXR_Util::VBM_CreateRectUV(pVBM, _pParams->m_SrcUVMin, _pParams->m_SrcUVMax);
		if (!pTV)
			return false;

		CVec2Dfp32 UVShrinkMin = _pParams->m_DstUVMin;
		CVec2Dfp32 UVShrinkMax = _pParams->m_DstUVMax;

		// Shrink once
		uint TextureID_ShrinkSrc = _pParams->m_TextureID_Src;
		for(uint iShrink = 0; iShrink < _pParams->m_nShrink; iShrink++)
		{
			VPShrink.m_Max[0] = VPShrink.m_Min[0] + ((VPShrink.m_Max[0] - VPShrink.m_Min[0]) >> 1);
			VPShrink.m_Max[1] = VPShrink.m_Min[1] + ((VPShrink.m_Max[1] - VPShrink.m_Min[1]) >> 1);

			{
				CRC_Attributes* pA = pVBM->Alloc_Attrib();
				if (!pA)
					return false;
				pA->SetDefault();
				pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
				pA->Attrib_TextureID(0, TextureID_ShrinkSrc);
				CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPShrink, 0xffffffff, Priority, pA);
				if (!pVB)
					return false;
				pVB->Geometry_TVertexArray(pTV, 0);
				pVBM->AddVB(pVB);
				Priority += PriorityOffset;
			}

			// Capture screen
			pVBM->AddCopyToTexture(Priority, CRct(VPShrink.m_Min[0], VPShrink.m_Min[1], VPShrink.m_Max[0], VPShrink.m_Max[1]), CPnt(VPShrink.m_Min[0], VPShrink.m_Min[1]), _pParams->m_TextureID_Dst, false);
			TextureID_ShrinkSrc = _pParams->m_TextureID_Dst;
			Priority += PriorityOffset;

			UVShrinkMax = UVShrinkMin + ((UVShrinkMax - UVShrinkMin) * 0.5f);

			pTV = (bRenderTextureVertFlip) ? 
				CXR_Util::VBM_CreateRectUV_VFlip(pVBM, UVShrinkMin, UVShrinkMax) :
			CXR_Util::VBM_CreateRectUV(pVBM, UVShrinkMin, UVShrinkMax);
			if (!pTV)
				return false;
		}

		_pRetTVShrink = pTV;
	}
	return true;
}

bool CXR_Util::GaussianBlur(const CXR_GaussianBlurParams *M_RESTRICT _pParams, CVec2Dfp32*& _pRetTVBlur)
{
	// Render gauss X & Y
	{
		const fp32 PriorityOffset = 0.01f;
		fp32 Priority = _pParams->m_Priority;
		_pRetTVBlur = NULL;

		CXR_VBManager* pVBM = _pParams->m_pVBM;
		CMat4Dfp32* pMat2D = pVBM->Alloc_M4_Proj2DRelBackPlane();
		if (!pMat2D)
			return false;
		uint bRenderTextureVertFlip  = _pParams->m_RenderCaps & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;

		bool bDoGlowX = (_pParams->m_Flags & XR_GAUSSFLAGS_NOBLUR_X) == 0;
		bool bDoGlowY = (_pParams->m_Flags & XR_GAUSSFLAGS_NOBLUR_Y) == 0;

		CRect2Duint16 VPBlur = _pParams->m_Rect;

		CVec2Dfp32* pTV = (bRenderTextureVertFlip) ? 
			CXR_Util::VBM_CreateRectUV_VFlip(pVBM, _pParams->m_SrcUVMin, _pParams->m_SrcUVMax) :
		CXR_Util::VBM_CreateRectUV(pVBM, _pParams->m_SrcUVMin, _pParams->m_SrcUVMax);
		if (!pTV)
			return false;

		CVec2Dfp32 UVBlurMin = _pParams->m_DstUVMin;
		CVec2Dfp32 UVBlurMax = _pParams->m_DstUVMax;

		// Shrink once
		uint TextureID_ShrinkSrc = _pParams->m_TextureID_Src;
		for(uint iShrink = 0; iShrink < _pParams->m_nShrink; iShrink++)
		{
			VPBlur.m_Max[0] = VPBlur.m_Min[0] + ((VPBlur.m_Max[0] - VPBlur.m_Min[0])  >> 1);
			VPBlur.m_Max[1] = VPBlur.m_Min[1] + ((VPBlur.m_Max[1] - VPBlur.m_Min[1])  >> 1);

			{
				CRC_Attributes* pA = pVBM->Alloc_Attrib();
				if (!pA)
					return false;
				pA->SetDefault();
				pA->Attrib_Disable(CRC_FLAGS_ZCOMPARE | CRC_FLAGS_ZWRITE);
				pA->Attrib_TextureID(0, TextureID_ShrinkSrc);
				CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPBlur, 0xffffffff, Priority, pA);
				if (!pVB)
					return false;
				pVB->Geometry_TVertexArray(pTV, 0);
				pVBM->AddVB(pVB);
				Priority += PriorityOffset;
			}

			// Capture screen
			pVBM->AddCopyToTexture(Priority, CRct(VPBlur.m_Min[0], VPBlur.m_Min[1], VPBlur.m_Max[0], VPBlur.m_Max[1]), CPnt(VPBlur.m_Min[0], VPBlur.m_Min[1]), _pParams->m_TextureID_Dst, false);
			TextureID_ShrinkSrc = _pParams->m_TextureID_Dst;
			Priority += PriorityOffset;

			UVBlurMax = UVBlurMin + ((UVBlurMax - UVBlurMin) * 0.5f);

			pTV = (bRenderTextureVertFlip) ? 
				CXR_Util::VBM_CreateRectUV_VFlip(pVBM, UVBlurMin, UVBlurMax) :
			CXR_Util::VBM_CreateRectUV(pVBM, UVBlurMin, UVBlurMax);
			if (!pTV)
				return false;
		}

		CRC_Attributes* pAX = pVBM->Alloc_Attrib();
		CRC_Attributes* pAY = pVBM->Alloc_Attrib();
		if (!pTV || !pAX || !pAY)
			return false;

		const int nFPParams = 12;
		uint8* pFPMem = (uint8*) pVBM->Alloc(2 * (sizeof(CRC_ExtAttributes_FragmentProgram20) + sizeof(CVec4Dfp32)*nFPParams));
		if (!pFPMem)
			return false;
		CVec4Dfp32* pFPXParams = (CVec4Dfp32*) pFPMem; pFPMem += nFPParams*sizeof(CVec4Dfp32);
		CVec4Dfp32* pFPYParams = (CVec4Dfp32*) pFPMem; pFPMem += nFPParams*sizeof(CVec4Dfp32);
		CRC_ExtAttributes_FragmentProgram20* pFPX = (CRC_ExtAttributes_FragmentProgram20*) pFPMem; pFPMem += sizeof(CRC_ExtAttributes_FragmentProgram20);
		CRC_ExtAttributes_FragmentProgram20* pFPY = (CRC_ExtAttributes_FragmentProgram20*) pFPMem; pFPMem += sizeof(CRC_ExtAttributes_FragmentProgram20);

		pFPX->Clear();
		pFPX->SetParameters(pFPXParams, nFPParams);
		pFPY->Clear();
		pFPY->SetParameters(pFPYParams, nFPParams);
		if (_pParams->m_pFilterShader)
		{
			pFPX->SetProgram(_pParams->m_pFilterShader, 0);
			pFPY->SetProgram(_pParams->m_pFilterShader, 0);
		}
		else
		{
			pFPX->SetProgram("XREngine_GaussClamped", 0);
			pFPY->SetProgram("XREngine_GaussClamped", 0);
		}

		pFPXParams[0] = 0;
		pFPXParams[2] = 1;
		fp32* pWeights = pFPXParams[5].k+3;
		fp32* pUVCenter = pFPXParams[5].k;
		CVec2Dfp32 PixelUV = _pParams->m_SrcPixelUV;
		CVec2Dfp32* pXDeltaUV = (CVec2Dfp32*) (pFPXParams+8);
		CVec2Dfp32* pYDeltaUV = (CVec2Dfp32*) (pFPYParams+8);

		fp32 lWeights[17];
		for(int w = 0; w < 17; w++)
		{
			fp32 weight = M_Pow(1.0f-fp32(w)*0.05f, _pParams->m_Exp);
			lWeights[w] = weight;
//			WSum += weight;
		}

		pWeights[0] = lWeights[0] * 0.5f;
		fp32 WSum = pWeights[0];
		for(int s = 1; s < 9; s++)
		{
			  fp32 w1 = lWeights[(s-1)*2 + 1];
			  fp32 w2 = lWeights[(s-1)*2 + 2];
			  fp32 u = w2 / (w1+w2);
			  fp32 w = (w1+w2) * 0.5f;
			  fp32 Pos = fp32(s-1)*2.0f + 1.0f + u;
			  pWeights[s] = w;
			  WSum += w * 2.0f;
			  pXDeltaUV[s-1] = CVec2Dfp32(Pos * PixelUV[0]*1.0f, 0);
			  pYDeltaUV[s-1] = CVec2Dfp32(0, Pos * PixelUV[1]);
		}
		pFPXParams[1] = 1.0f / WSum;


		CVec2Dfp32 UVClampMin, UVClampMax;
		if (bRenderTextureVertFlip)
		{
			UVClampMin = CVec2Dfp32(UVBlurMin[0], 1.0f - UVBlurMax[1]);
			UVClampMax = CVec2Dfp32(UVBlurMax[0], 1.0f - UVBlurMin[1]);
		}
		else
		{
			UVClampMin = UVBlurMin;
			UVClampMax = UVBlurMax;
		}

		UVClampMin += (PixelUV * 0.5f);
		UVClampMax -= (PixelUV * 0.5f);
		pFPXParams[3] = CVec4Dfp32(UVClampMin[0], UVClampMin[1], UVClampMin[0], UVClampMin[1]);
		pFPXParams[4] = CVec4Dfp32(UVClampMax[0], UVClampMax[1], UVClampMax[0], UVClampMax[1]);

		fp32 UCenter = UVBlurMin[0] + (_pParams->m_UVCenter[0] * (UVBlurMax[0] - UVBlurMin[0]));
		fp32 VCenter = UVBlurMin[1] + (_pParams->m_UVCenter[1] * (UVBlurMax[1] - UVBlurMin[1]));
		
		// Set center of blur screen
		pUVCenter[0] = UCenter;
		pUVCenter[1] = (bRenderTextureVertFlip) ? (1.0f - VCenter) : VCenter;

		memcpy(pFPYParams, pFPXParams, 8*sizeof(CVec4Dfp32));
		pFPYParams[0].k[0] = 0;

		CVec4Dfp32* pConditionParams = (bDoGlowY) ? pFPYParams : pFPXParams;

		pConditionParams[0].k[0] = _pParams->m_Bias[0];
		pConditionParams[0].k[1] = _pParams->m_Bias[1];
		pConditionParams[0].k[2] = _pParams->m_Bias[2];
		pConditionParams[0].k[3] = _pParams->m_Bias[3];
		pConditionParams[1].k[0] *= _pParams->m_Scale[0];
		pConditionParams[1].k[1] *= _pParams->m_Scale[1];
		pConditionParams[1].k[2] *= _pParams->m_Scale[2];
		pConditionParams[2].k[0] = _pParams->m_Gamma[0];
		pConditionParams[2].k[1] = _pParams->m_Gamma[1];
		pConditionParams[2].k[2] = _pParams->m_Gamma[2];
		pConditionParams[2].k[3] = 0;

		pAX->SetDefault();
		pAX->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
		pAX->Attrib_TextureID(0, TextureID_ShrinkSrc);
		pAX->Attrib_TextureID(1, _pParams->m_TextureID_Filter01);
		pAX->Attrib_TextureID(2, _pParams->m_TextureID_Filter02);
		pAX->Attrib_TextureID(3, _pParams->m_TextureID_Filter03);
		pAX->m_pExtAttrib = pFPX;
		CXR_VertexBuffer* pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPBlur, 0xffffffff, Priority, pAX);
		if (!pVB)
			return false;
		pVB->Geometry_TVertexArray(pTV, 0);
		if (bDoGlowX)
			pVBM->AddVB(pVB);
		Priority += PriorityOffset;

		// Capture screen
		pVBM->AddCopyToTexture(Priority, CRct(VPBlur.m_Min[0], VPBlur.m_Min[1], VPBlur.m_Max[0], VPBlur.m_Max[1]), CPnt(VPBlur.m_Min[0], VPBlur.m_Min[1]), _pParams->m_TextureID_Dst, false);
		Priority += PriorityOffset;

		pAY->SetDefault();
		pAY->Attrib_Disable(CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
		pAY->Attrib_TextureID(0, _pParams->m_TextureID_Dst);
		pAX->Attrib_TextureID(1, _pParams->m_TextureID_Filter01);
		pAX->Attrib_TextureID(2, _pParams->m_TextureID_Filter02);
		pAX->Attrib_TextureID(3, _pParams->m_TextureID_Filter03);
		pAY->m_pExtAttrib = pFPY;
		pVB = CXR_Util::VBM_RenderRect(pVBM, pMat2D, VPBlur, 0xffffffff, Priority, pAY);
		if (!pVB)
			return false;
		pVB->Geometry_TVertexArray(pTV, 0);
		if (bDoGlowY)
			pVBM->AddVB(pVB);
		Priority += PriorityOffset;

		pVBM->AddCopyToTexture(Priority, CRct(VPBlur.m_Min[0], VPBlur.m_Min[1], VPBlur.m_Max[0], VPBlur.m_Max[1]), CPnt(VPBlur.m_Min[0], VPBlur.m_Min[1]), _pParams->m_TextureID_Dst, false);
		Priority += PriorityOffset;

		_pRetTVBlur = pTV;
	}
	return true;
}

bool CXR_Util::RadialBlur(const CXR_RadialBlurParams *M_RESTRICT _pParams, CVec2Dfp32*& _pRetTVRadialBlur)
{
	// Calculate amount of VB memory required for streaks
	const uint nExtraParams = _pParams->m_nFilterParams;
	const uint nParams = nExtraParams + 7;
	uint VBMemReq =  TUtilAlign(sizeof(CRC_Attributes)) +
					(TUtilAlign(sizeof(CRC_VertexBuffer)) * 2) +
					 TUtilAlign(sizeof(CXR_VBChain)) +
					 TUtilAlign(sizeof(CRC_ExtAttributes_FragmentProgram20)) +
					 TUtilAlign(sizeof(CXR_VertexBuffer_PreRender)) +
					 TUtilAlign(sizeof(CXR_PreRenderData_RenderTarget_CopyToTexture1)) +
					 TUtilAlign(sizeof(CVec4Dfp32) * nParams) +
					 TUtilAlign(sizeof(CVec3Dfp32) * 4) +
					(TUtilAlign(sizeof(CVec2Dfp32) * 4) * 2);
					
	CXR_VBManager* pVBM = _pParams->m_pVBM;
	uint8* pVBMem = (uint8*)pVBM->Alloc(VBMemReq);
	if (pVBMem)
	{
		CXR_Util::Init();

		// Get pointers to allocated vb memory
		CRC_Attributes* pA = TUtilGetAlign<CRC_Attributes>(pVBMem);
		CXR_VertexBuffer* pVB = TUtilGetAlign<CXR_VertexBuffer>(pVBMem);
		CXR_VertexBuffer* pVBCopy = TUtilGetAlign<CXR_VertexBuffer>(pVBMem);
		CXR_VBChain* pVBChain = TUtilGetAlign<CXR_VBChain>(pVBMem);
		CRC_ExtAttributes_FragmentProgram20* pFP = TUtilGetAlign<CRC_ExtAttributes_FragmentProgram20>(pVBMem);
		CXR_VertexBuffer_PreRender* pVBPreRender = TUtilGetAlign<CXR_VertexBuffer_PreRender>(pVBMem);
		CXR_PreRenderData_RenderTarget_CopyToTexture1* pRTCopy = TUtilGetAlign<CXR_PreRenderData_RenderTarget_CopyToTexture1>(pVBMem);
		CVec4Dfp32* pFPParams = TUtilGetAlign<CVec4Dfp32>(pVBMem, nParams);
		CVec3Dfp32* pV = TUtilGetAlign<CVec3Dfp32>(pVBMem, 4);
		CVec2Dfp32* pTV = TUtilGetAlign<CVec2Dfp32>(pVBMem, 4);
		CVec2Dfp32* pTVOutput = TUtilGetAlign<CVec2Dfp32>(pVBMem, 4);

		// Setup priority and an offset
		const fp32 PriorityOffset = 0.001f;
		fp32 Priority = _pParams->m_Priority;

		fp32 Power = _pParams->m_Power * _pParams->m_PowerScale;

		// Texture properties
		uint bRenderTextureVertFlip = _pParams->m_RenderCaps & CRC_CAPS_FLAGS_RENDERTEXTUREVERTICALFLIP;

		// Create screen copy rectangle
		CRct Rect(CPnt(_pParams->m_Rect.m_Min.k[0], _pParams->m_Rect.m_Min.k[1]), 
				  CPnt(_pParams->m_Rect.m_Max.k[0], _pParams->m_Rect.m_Max.k[1]));

		// Get center point that zoom radial blur operates from (scales with uvmin/uvmax)
		const CVec2Dfp32& UVMin = _pParams->m_SrcUVMin;
		const CVec2Dfp32& UVMax = _pParams->m_SrcUVMax;
		fp32 SizeU = (UVMax[0] - UVMin[0]);
		fp32 SizeV = (UVMax[1] - UVMin[1]);
		fp32 RadialCenterU = UVMin[0] + (_pParams->m_RadialCenter[0] * SizeU);
		fp32 RadialCenterV = UVMin[1] + (_pParams->m_RadialCenter[1] * SizeV);
		fp32 CenterU = UVMin[0] + (_pParams->m_UVCenter[0] * SizeU);
		fp32 CenterV = UVMin[1] + (_pParams->m_UVCenter[1] * SizeV);
		fp32 ExtraU = UVMin[0] + (_pParams->m_UVExtra[0] * SizeU);
		fp32 ExtraV = UVMin[1] + (_pParams->m_UVExtra[1] * SizeV);
		fp32 ScreenPixelU = _pParams->m_Screen_PixelUV[0];
		fp32 ScreenPixelV = _pParams->m_Screen_PixelUV[1];

		CVec2Dfp32 ExtraSubCenterN = (_pParams->m_UVExtra - _pParams->m_UVCenter);
		ExtraSubCenterN.Normalize();

		// Get 2d matrix
		CMat4Dfp32 Mat2D;
		CRC_Viewport* pVP = pVBM->Viewport_Get();
		pVP->Get2DMatrix_RelBackPlane(Mat2D);

		// Setup fragment program parameters
		pFPParams[0][0] = RadialCenterU;
		pFPParams[0][1] = (bRenderTextureVertFlip) ? (1.0f - RadialCenterV) : RadialCenterV;
		pFPParams[0][2] = CenterU;
		pFPParams[0][3] = (bRenderTextureVertFlip) ? (1.0f - CenterV) : CenterV;

		pFPParams[1][0] = ScreenPixelU * _pParams->m_PixelScale;
		pFPParams[1][1] = ScreenPixelV * _pParams->m_PixelScale;
		pFPParams[1][2] = 0.0f;
		pFPParams[1][3] = _pParams->m_Power;

		pFPParams[2][0] = _pParams->m_BlurSpace[0];
		pFPParams[2][1] = _pParams->m_BlurSpace[1];
		pFPParams[2][2] = _pParams->m_BlurSpace[2];
		pFPParams[2][3] = _pParams->m_BlurSpace[3];

		pFPParams[3][0] = _pParams->m_BlurSpace[4];
		pFPParams[3][1] = _pParams->m_BlurSpace[5];
		pFPParams[3][2] = _pParams->m_BlurSpace[6];
		pFPParams[3][3] = _pParams->m_BlurSpace[7];

		pFPParams[4][0] = _pParams->m_Affection.k[0];
		pFPParams[4][1] = _pParams->m_Affection.k[1];
		pFPParams[4][2] = _pParams->m_Affection.k[2];
		pFPParams[4][3] = Clamp01(1.0f - _pParams->m_ColorIntensity);

		pFPParams[5][0] = _pParams->m_ColorScale[0] * Power;
		pFPParams[5][1] = _pParams->m_ColorScale[1] * Power;
		pFPParams[5][2] = _pParams->m_ColorScale[2] * Power;
		pFPParams[5][3] = 1.0f;

		pFPParams[6][0] = ExtraU;
		pFPParams[6][1] = (bRenderTextureVertFlip) ? (1.0f - ExtraV) : ExtraV;
		pFPParams[6][2] = ExtraSubCenterN.k[0];
		pFPParams[6][3] = ExtraSubCenterN.k[1];

		// Set extra parameters if any
		if (nExtraParams > 0)
			memcpy(&pFPParams[7], _pParams->m_lFilterParams, sizeof(CVec4Dfp32) * nExtraParams);

		// Setup fragment program
		pFP->Clear();
		pFP->SetParameters(pFPParams, nParams);
		if (_pParams->m_pFilterShader)
		{
			pFP->SetProgram(_pParams->m_pFilterShader, 0);
		}
		else
		{
			pFP->SetProgram("XREngine_RadialBlur", 0);
		}

		// Setup attribute
		pA->SetDefault();
		pA->m_pExtAttrib = pFP;
		pA->Attrib_Disable(CRC_FLAGS_CULL | CRC_FLAGS_ZWRITE | CRC_FLAGS_ZCOMPARE);
		pA->Attrib_TextureID(0, _pParams->m_TextureID_Src);

		// Setup vb chain
		pVBChain->Clear();

		// Setup vertex buffer
		pVB->Construct();
		pVB->m_pAttrib = pA;
		pVB->m_Priority = Priority;
		pVB->SetVBChain(pVBChain);
		pVB->Geometry_VertexArray(pV, 4, true);
		pVB->Geometry_TVertexArray(pTV, 0);
		pVB->Render_IndexedTriangles(CXR_Util::m_lQuadParticleTriangles, 2);

		CVec2Dfp32 UVMinOutput = CVec2Dfp32(fp32(Rect.p0.x) * ScreenPixelU, fp32(Rect.p0.y) * ScreenPixelV);
		CVec2Dfp32 UVMaxOutput = CVec2Dfp32(fp32(Rect.p1.x) * ScreenPixelU, fp32(Rect.p1.y) * ScreenPixelV);

		// Calculate vertices and texture coordinates
		CXR_Util::VBM_CreateRect(&Mat2D, _pParams->m_Rect, pV);
		CXR_Util::VBM_CreateRectUV(UVMin, UVMax, bRenderTextureVertFlip, pTV);
		CXR_Util::VBM_CreateRectUV(UVMinOutput, UVMaxOutput, bRenderTextureVertFlip, pTVOutput);

		// Create screen copy parameters
		pRTCopy->m_SrcRect = Rect;
		pRTCopy->m_Dst = Rect.p0;
		pRTCopy->m_bContinueTiling = false;
		pRTCopy->m_TextureID = _pParams->m_TextureID_Dst;
		pRTCopy->m_Slice = 0;
		pRTCopy->m_iMRT = 0;

		// Create screen copy pre render callback
		pVBPreRender->Create((void*)pRTCopy, PFN_VERTEXBUFFER_PRERENDER(CXR_PreRenderData_RenderTarget_CopyToTexture1::RenderTarget_CopyToTexture));

		// Create screen copy vertex buffer
		pVBCopy->Construct();
		pVBCopy->m_pPreRender = pVBPreRender;
		pVBCopy->m_Priority = Priority + PriorityOffset;
		pVBCopy->m_Flags |= CXR_VBFLAGS_PRERENDER;
		pVBCopy->SetVBEColor(0xffffff00);

		// Add vertex buffers
		pVBM->AddVB(pVB);
		pVBM->AddVB(pVBCopy);

		_pRetTVRadialBlur = pTVOutput;

		return true;
	}

	return false;
}



