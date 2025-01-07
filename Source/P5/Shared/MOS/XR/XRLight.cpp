

#include "PCH.h"

#include "XRClass.h"
#include "MFloat.h"
#include "MSIMD.h"
#include "XRVBManager.h"

int FindMostContributingLights( const int _MaxLights, uint8* _pLightIndices, const CMat4Dfp4& _WMat, const CXR_RenderInfo* _pRenderInfo );
int FindMostContributingLights( const int _MaxLights, uint8* _pLightIndices, const CMat4Dfp4& _WMat, const CXR_RenderInfo* _pRenderInfo )
{
	MAUTOSTRIP( FindMostContributingLights, 0 );
	int nLights = 0;
	
	nLights	= MinMT( _MaxLights, _pRenderInfo->m_nLights );

	for( int i = 0; i < nLights; i++ )
		_pLightIndices[i]	= i;

	if( _MaxLights < _pRenderInfo->m_nLights )
	{
		// All light didn't fit.. do some setup and then start evaluating which lights we should actually use
		// Should weigh in intensity of light here aswell.. not just falloff...
		fp4 aAttenuation[32];
		
		for( int i = 0; i < _MaxLights; i++ )
		{
			const CXR_Light* pL = _pRenderInfo->m_pLightInfo[i].m_pLight;

			CVec3Dfp4 LightVec = pL->GetPosition() - CVec3Dfp4::GetRow( _WMat, 3 );

			fp4 Intensity = ( pL->m_Intensity[0] + pL->m_Intensity[1] + pL->m_Intensity[2] ) * ( 1.0f / 3.0f );
			aAttenuation[i] = Intensity * Clamp01( 1.0f - ( LightVec.LengthSqr() * Sqr(pL->m_RangeInv) ) );
		}
		
		// Sort lights by attenuation for.. bubble sort... 
		bool bSorted;
		do
		{
			bSorted = true;
			for( int i = 1; i < _MaxLights; i++ )
			{
				if( aAttenuation[i-1] < aAttenuation[i] )
				{
					fp4 t = aAttenuation[i-1];
					aAttenuation[i-1]	= aAttenuation[i];
					aAttenuation[i]	= t;
					
					uint8 idx = _pLightIndices[i-1];
					_pLightIndices[i-1]	= _pLightIndices[i];
					_pLightIndices[i]	= idx;

					bSorted = false;
				}
			}
		}
		while( bSorted == false );
		
		
		for( int i = _MaxLights; i < _pRenderInfo->m_nLights; i++ )
		{
			const CXR_Light* pL = _pRenderInfo->m_pLightInfo[i].m_pLight;

			CVec3Dfp4 LightVec = pL->GetPosition() - CVec3Dfp4::GetRow( _WMat, 3 );

			fp4 Intensity = ( pL->m_Intensity[0] + pL->m_Intensity[1] + pL->m_Intensity[2] ) * ( 1.0f / 3.0f );
			fp4 Attenuation = Intensity * Clamp01( 1.0f - ( LightVec.LengthSqr() * Sqr(pL->m_RangeInv) ) );
			
			for( int j = 0; j < _MaxLights; j++ )
			{
				if( Attenuation > aAttenuation[j] )
				{
					if( j < ( _MaxLights - 1 ) )
					{
						for( int k = _MaxLights - 1; k > j; k-- )
						{
							aAttenuation[k]	= aAttenuation[k-1];
							_pLightIndices[k]	= _pLightIndices[k-1];
						}
					}
					aAttenuation[j]	= Attenuation;
					_pLightIndices[j]	= i;
					break;
				}
			}
			
		}
	}

	return nLights;
}

int ConvertWLSToCRCLight(CXR_WorldLightState& _WLS, CXR_VBManager& _VBM, CRC_Light*& _pLights, fp4 _LightScale);
int ConvertWLSToCRCLight(CXR_WorldLightState& _WLS, CXR_VBManager& _VBM, CRC_Light*& _pLights, fp4 _LightScale)
{
	MAUTOSTRIP(ConvertWLSToCRCLight, 0);
	int nLights = 0;
	{
		CXR_Light* pL = _WLS.GetFirst();
		while(pL)
		{
			nLights++;
			if (pL->m_Type == CXR_LIGHTTYPE_LIGHTVOLUME)
				nLights++;
			pL = pL->GetNext();
		}
	}

	CRC_Light* pLights = _VBM.Alloc_Lights(nLights);
	if (!pLights) return 0;

//				CMat4Dfp4 WMatInv;
//				_WMat.InverseOrthogonal(WMatInv);

	int iL = 0;

/*	{
		CRC_Light& L = pLights[iL];
		L.m_Type = CRC_LIGHTTYPE_PARALLELL;
		L.m_Color = 0x00383838;
		L.m_Direction = CVec3Dfp4(0,0,1);

		iL++;
	}*/

	CXR_Light* pL = _WLS.GetFirst();
	while(pL)
	{
		CRC_Light& Light = pLights[iL];

		switch(pL->m_Type)
		{
		case CXR_LIGHTTYPE_POINT :
			{
				CRC_Light L;
				L.m_Type = CRC_LIGHTTYPE_POINT;
				L.m_Color = pL->m_IntensityInt32;
				L.m_Ambient = 0x00000000;
				L.m_Attenuation[0] = 0;
				L.m_Attenuation[1] = pL->m_RangeInv;
				L.m_Attenuation[2] = 0;
				L.m_Pos = pL->GetPosition();

				Light = L;
//							ConOut("CXR_LIGHTTYPE_POINT");
			}
			break;

/*						case CXR_LIGHTTYPE_AMBIENT :
			{
//							ConOut("CXR_LIGHTTYPE_AMBIENT");
			}
			break;*/

		case CXR_LIGHTTYPE_PARALLELL :
			{
				CRC_Light L;
				L.m_Type = CRC_LIGHTTYPE_PARALLELL;
				L.m_Color = pL->m_IntensityInt32;
				L.m_Ambient = 0x00000000;
				L.m_Attenuation[0] = 0;
				L.m_Attenuation[1] = 0;
				L.m_Attenuation[2] = 0;
				L.m_Direction = pL->GetDirection() * _LightScale;

				Light = L;
//							ConOut("CXR_LIGHTTYPE_PARALLELL");
			}
			break;

		case CXR_LIGHTTYPE_AMBIENT :
			{
				CRC_Light L;
				L.m_Type = CRC_LIGHTTYPE_AMBIENT;
				L.m_Color = pL->m_IntensityInt32;
				L.m_Ambient = 0x00000000;
				L.m_Attenuation[0] = 0;
				L.m_Attenuation[1] = 0;
				L.m_Attenuation[2] = 0;
				L.m_Direction = 0;
				
				Light = L;
				//							ConOut("CXR_LIGHTTYPE_PARALLELL");
			}
			break;

		case CXR_LIGHTTYPE_LIGHTVOLUME :
			{
//							ConOut("CXR_LIGHTTYPE_LIGHTVOLUME");

				fp4 LightScale = _LightScale;
				fp4 CompCap = Max3(pL->m_Intensity[0], pL->m_Intensity[1], pL->m_Intensity[2]) * 2.0f * (1.0f + 2*pL->m_Range) * _LightScale;
				if (CompCap > 1.5f)
					LightScale *= 1.5f / CompCap;

//				fp4 BiasScale = pL->m_Range * LightScale * 2.0f;
//				ConOut(CStrF("%f, %s, %f, %f->%f, %s, %f", CompCap, pL->m_Intensity.GetString().Str(), pL->m_Range, _LightScale, LightScale, pL->m_LightVec.GetString().Str(), BiasScale));

				CRC_Light L;
				L.m_Type = CRC_LIGHTTYPE_PARALLELL;
				L.m_Color = pL->m_IntensityInt32 * LightScale * 2.0f;
				L.m_Ambient = 0x00000000;
				L.m_Direction = pL->GetDirection();
				L.m_Attenuation[0] = 0;
				L.m_Attenuation[1] = 0;
				L.m_Attenuation[2] = 0;
				Light = L;

				iL++;
				CRC_Light& Light2 = pLights[iL];

				CRC_Light L2;
				L2.m_Type = 3;
				L2.m_Color = pL->m_IntensityInt32 * Clamp01(pL->m_Range * LightScale * 4.0f);
				Light2 = L2;
			}
			break;

		default :
			{
				CRC_Light L;
				Light = L;
			}
		}

		pL = pL->GetNext();
		iL++;
	}

	if (iL != nLights)
		Error_static("ConvertWLSToCRCLight", CStrF("Internal error. %d != %d", iL, nLights));

	_pLights = pLights;
	return nLights;
}

void CXR_WorldLightState::Optimize(const CVec3Dfp4 _Pos, fp4 _Radius, fp4 _ParallellTresh, const CMat43fp4* _pMat)
{
	MAUTOSTRIP(CXR_WorldLightState_Optimize, MAUTOSTRIP_VOID);
	CXR_Light* pL = GetFirst();
	while(pL)
	{
		if (pL->m_Type == CXR_LIGHTTYPE_POINT)
		{
			CVec3Dfp4 LVec;
			pL->GetPosition().Sub(_Pos, LVec);
//			fp4 Dist = LVec.Length();
			
/*	#ifndef ALWAYS_PARALLELL
			if ((Dist > 4.0f*_Radius) && (((Dist - _Radius) / (Dist + _Radius)) > _ParallellTresh))
	#endif */

#ifdef ALWAYS_PARALLELL
			{
				pL->m_Type = CXR_LIGHTTYPE_PARALLELL;
				pL->m_Intensity *= Max(0.0f, 1.0f - (Dist*pL->m_RangeInv));
				pL->m_IntensityInt32 = CPixel32(pL->m_Intensity.k[0]*255.0f, pL->m_Intensity.k[1]*255.0f, pL->m_Intensity.k[2]*255.0f);
				LVec.Scale(1.0f/Dist, pL->m_LightVec);
			}
#endif
		}

		if (pL->m_Type == CXR_LIGHTTYPE_FAKESKYRADIOSITY)
		{
			pL->SetDirection(CVec3Dfp4(0,0,1));
		}

		if (_pMat) 
		{
			CMat43fp4 M;
			pL->m_Pos.Multiply(*_pMat, M);
			pL->m_Pos = M;
		}

		pL = pL->m_pNext;
	}
}

// -------------------------------------------------------------------
void CXR_WorldLightState::LightDiffuse(CXR_Light* _pLights, int _nV, const CVec3Dfp4* _pV, const CVec3Dfp4* _pN, int _bOmni, CPixel32* _pVLight, int _Alpha, fp4 _Scale)
{
	MAUTOSTRIP(CXR_WorldLightState_LightDiffuse, MAUTOSTRIP_VOID);
	const int MaxVertices = 256;
	if (_nV > MaxVertices)
	{
		int nV = _nV;
		int iV = 0;
		while(nV > 0)
		{
			LightDiffuse(_pLights, Min(nV, MaxVertices), &_pV[iV], &_pN[iV], _bOmni, &_pVLight[iV], _Alpha, _Scale);
			iV += MaxVertices;
			nV -= MaxVertices;
		}
		return;
	}

	if (!_pLights)
	{
		FillD(_pVLight, _nV, _Alpha << 24);
		return;
	}

//	CMathAccel* pMathAccel = GetMathAccel();
	const CVec3Dfp4* pVV = _pV;
	const CVec3Dfp4* pVN = _pN;
//	CPixel32* pVL = _pVLight;
//	int nV = _nV;

	uint8 Data[MaxVertices*(sizeof(CVec4Dfp4) + 3*4) + 16];
	uint8* p = (uint8*)(((mint)&Data+15) & 0xfffffff0);
	CVec4Dfp4* pLight = (CVec4Dfp4*) p;
	p += MaxVertices*sizeof(CVec4Dfp4);
	fp4* pSqrt = (fp4*)p;
	p += MaxVertices*sizeof(fp4);
//	fp4* pRecp = (fp4*)p;
	p += MaxVertices*sizeof(fp4);
	fp4* pDot = (fp4*)p;

	FillChar(pLight, sizeof(CVec4Dfp4)*_nV, 0);

	const CXR_Light* pL = _pLights;
	while(pL)
	{
		if (_bOmni)
		{
			// Omni, surface normal is ignored and is considered pointing towards the light in all cases.

			switch(pL->m_Type)
			{
			case CXR_LIGHTTYPE_FAKESKYRADIOSITY :
				{
					fp4 s = 256.0f*_Scale;
					for(int v = 0; v < _nV; v++)
					{
						// Sky
						pLight[v][0] += pL->m_Intensity.k[0] * s;
						pLight[v][1] += pL->m_Intensity.k[1] * s;
						pLight[v][2] += pL->m_Intensity.k[2] * s;
					}
					break;
				}

			case CXR_LIGHTTYPE_PARALLELL :
				{
					// Parallell light
					fp4 s = 256.0f*_Scale;
					for(int v = 0; v < _nV; v++)
					{
						pLight[v][0] += pL->m_Intensity.k[0] * s;
						pLight[v][1] += pL->m_Intensity.k[1] * s;
						pLight[v][2] += pL->m_Intensity.k[2] * s;
					}
					break;
				}

			case CXR_LIGHTTYPE_AMBIENT :
				{
					// Ambient light-source.
					CVec3Dfp4 Offset;
					pL->m_Intensity.Scale(256.0f*_Scale, Offset);
					for(int v = 0; v < _nV; v++)
					{
						pLight[v][0] += Offset[0];
						pLight[v][1] += Offset[1];
						pLight[v][2] += Offset[2];
					}
					break;
				}

			case CXR_LIGHTTYPE_LIGHTVOLUME :
				{
					fp4 Bias = pL->m_Range;
					fp4 dotp = 2.0f*256.0f*(Bias + pL->GetDirection().Length()) * _Scale;
					CVec3Dfp4 Offset;
					pL->m_Intensity.Scale(dotp, Offset);
					for(int v = 0; v < _nV; v++)
					{
						pLight[v][0] += Offset[0];
						pLight[v][1] += Offset[1];
						pLight[v][2] += Offset[2];
					}

					break;
				}

			case CXR_LIGHTTYPE_POINT :
			default :
				{
					// Point light-source.
					{
						CVec3Dfp4 Pos = pL->GetPosition();
						for(int v = 0; v < _nV; v++)
						{
							fp4 vlx = Pos.k[0] - pVV[v].k[0];
							fp4 vly = Pos.k[1] - pVV[v].k[1];
							fp4 vlz = Pos.k[2] - pVV[v].k[2];
							pSqrt[v] = Sqr(vlx) + Sqr(vly) + Sqr(vlz);
						}
					}

					SIMD_Sqrt8(pSqrt, NULL, _nV);

					{
						fp4 s = 256.0f * pL->m_RangeInv * _Scale;
						for(int v = 0; v < _nV; v++)
						{
							fp4 ld = pSqrt[v];
							if (ld > pL->m_Range) continue;

							fp4 dotp = (pL->m_Range - ld) * s;
							pLight[v][0] += pL->m_Intensity.k[0] * dotp;
							pLight[v][1] += pL->m_Intensity.k[1] * dotp;
							pLight[v][2] += pL->m_Intensity.k[2] * dotp;
						}
					}
				}
			}

		}
		else
		{
			switch(pL->m_Type)
			{
			case CXR_LIGHTTYPE_FAKESKYRADIOSITY :
				{
					CVec3Dfp4 LightVec = pL->GetDirection();
					for(int v = 0; v < _nV; v++)
					{
						fp4 dotp = pVN[v] * LightVec;
						if (dotp > 0.0f)
							dotp = ((0.655757f-0.203727f)*dotp + 0.203727f) * 256.0f;
						else
							dotp = (1.0f+dotp)*0.203727f*256.0f;

						dotp *= _Scale;
						pLight[v][0] += pL->m_Intensity.k[0] * dotp;
						pLight[v][1] += pL->m_Intensity.k[1] * dotp;
						pLight[v][2] += pL->m_Intensity.k[2] * dotp;
					}
					break;
				}

			case CXR_LIGHTTYPE_PARALLELL :
				{
					// Parallell light
					CVec3Dfp4 LightVec = pL->GetDirection();
					for(int v = 0; v < _nV; v++)
					{
						fp4 dotp = 256.0f*(pVN[v] * LightVec);
						if (FloatIsNeg(dotp)) continue;
						dotp *= _Scale;

						pLight[v][0] += pL->m_Intensity.k[0] * dotp;
						pLight[v][1] += pL->m_Intensity.k[1] * dotp;
						pLight[v][2] += pL->m_Intensity.k[2] * dotp;
					}
					break;
				}

			case CXR_LIGHTTYPE_AMBIENT :
				{
					// Ambient light-source.
					CVec3Dfp4 Offset;
					pL->m_Intensity.Scale(256.0f*_Scale, Offset);
					for(int v = 0; v < _nV; v++)
					{
						pLight[v][0] += Offset[0];
						pLight[v][1] += Offset[1];
						pLight[v][2] += Offset[2];
					}
					break;
				}

			case CXR_LIGHTTYPE_LIGHTVOLUME :
				{
					fp4 Bias = pL->m_Range;
					CVec3Dfp4 LightVec = pL->GetDirection();
					for(int v = 0; v < _nV; v++)
					{
						fp4 dotp = 2.0f*256.0f*((pVN[v] * LightVec) + Bias);
						if (FloatIsNeg(dotp)) continue;
						dotp *= _Scale;

						pLight[v][0] += pL->m_Intensity.k[0] * dotp;
						pLight[v][1] += pL->m_Intensity.k[1] * dotp;
						pLight[v][2] += pL->m_Intensity.k[2] * dotp;
					}

					break;
				}

			case CXR_LIGHTTYPE_POINT :
			default :
				{
					// Point light-source.
					{
						CVec3Dfp4 Pos = pL->GetPosition();
						for(int v = 0; v < _nV; v++)
						{
							fp4 vlx = Pos.k[0] - pVV[v].k[0];
							fp4 vly = Pos.k[1] - pVV[v].k[1];
							fp4 vlz = Pos.k[2] - pVV[v].k[2];
							pSqrt[v] = Sqr(vlx) + Sqr(vly) + Sqr(vlz);
							pDot[v] = (pVN[v].k[0]*vlx + pVN[v].k[1]*vly + pVN[v].k[2]*vlz);
	//						if (lDot[v] < 0.0f) lSqrt[v] = 0;
						}
					}

					SIMD_RecpSqrt8(pSqrt, NULL, _nV);

					{
						fp4 s = 256.0f * pL->m_RangeInv * _Scale;
						for(int v = 0; v < _nV; v++)
						{
							fp4 dotp = pDot[v];
							if (FloatIsNeg(dotp)) continue;

							fp4 lRecp = pSqrt[v];
							if (lRecp < pL->m_RangeInv) continue;
							dotp *= s * (pL->m_Range*lRecp - 1);

							pLight[v][0] += pL->m_Intensity.k[0] * dotp;
							pLight[v][1] += pL->m_Intensity.k[1] * dotp;
							pLight[v][2] += pL->m_Intensity.k[2] * dotp;
						}
					}
				}
			}
		}

		pL = pL->GetNext();
	}

	SIMD_ConvertRGB(pLight, (uint32*)_pVLight, _nV, _Alpha);
}


// -------------------------------------------------------------------
void CXR_WorldLightState::LightSpecular(CXR_Light* _pLights, int _nV, const CVec3Dfp4* _pV, const CVec3Dfp4* _pN, int _Power, CPixel32* _pVLight, const CVec3Dfp4& _Eye, int _Alpha, fp4 _Scale)
{
	MAUTOSTRIP(CXR_WorldLightState_LightSpecular, MAUTOSTRIP_VOID);
	const int MaxVertices = 256;
	if (_nV > MaxVertices)
	{
		int nV = _nV;
		int iV = 0;
		while(nV > 0)
		{
			LightSpecular(_pLights, Min(nV, MaxVertices), &_pV[iV], &_pN[iV], _Power, &_pVLight[iV], _Eye, _Alpha, _Scale);
			iV += MaxVertices;
			nV -= MaxVertices;
		}
		return;
	}

	if (!_pLights)
	{
		FillD(_pVLight, _nV, _Alpha << 24);
		return;
	}

//	CMathAccel* pMathAccel = GetMathAccel();
	int nV = _nV;
	const CVec3Dfp4* pVV = _pV;
	const CVec3Dfp4* pVN = _pN;
//	CPixel32* pVL = _pVLight;
//	int Alpha = _Alpha << 24;

	uint8 Data[MaxVertices*(sizeof(CVec4Dfp4) + 3*4) + 16];
	uint8* p = (uint8*)(((mint)&Data+15) & 0xfffffff0);
	CVec4Dfp4* pLight = (CVec4Dfp4*) p;
	p += MaxVertices*sizeof(CVec4Dfp4);
	fp4* pSqrt = (fp4*)p;
	p += MaxVertices*sizeof(fp4);
	fp4* pRecp = (fp4*)p;
	p += MaxVertices*sizeof(fp4);
	fp4* pDot = (fp4*)p;

	FillChar(pLight, sizeof(CVec4Dfp4)*_nV, 0);

	CVec3Dfp4 lReflect[MaxVertices];


	for(int v = 0; v < nV; v++)
	{
		CVec3Dfp4 VCam;
		pVV[v].Sub(_Eye, VCam);
		VCam.Combine(pVN[v], -2.0f * (VCam*pVN[v]), lReflect[v]);
		pSqrt[v] = lReflect[v].LengthSqr();
	}

	SIMD_RecpSqrt8(pSqrt, NULL, _nV);


	TProfileDef(Time);
	{
		TMeasureProfile(Time);

		_Scale *= 256.0f;

		const CXR_Light* pL = _pLights;
		while(pL)
		{
			switch(pL->m_Type)
			{
			case CXR_LIGHTTYPE_FAKESKYRADIOSITY :
				{
					CVec3Dfp4 LightVec = pL->GetDirection();
					for(int v = 0; v < nV; v++)
					{
						fp4 dotp = (LightVec * lReflect[v]) * pSqrt[v];
						if (FloatIsNeg(dotp)) continue;

						int Pow = _Power;// >> 1;
						if (Pow)
						{
							if (Pow & 1) dotp = Sqr(dotp);
							if (Pow & 2) dotp = Sqr(Sqr(dotp));
							if (Pow & 4) dotp = Sqr(Sqr(Sqr(dotp)));
							if (Pow & 8) dotp = Sqr(Sqr(Sqr(Sqr(dotp))));
						}
						dotp *= _Scale;
						dotp *= 0.5f;
						pLight[v][0] += pL->m_Intensity.k[0] * dotp;
						pLight[v][1] += pL->m_Intensity.k[1] * dotp;
						pLight[v][2] += pL->m_Intensity.k[2] * dotp;
					}

					break;
				}

			case CXR_LIGHTTYPE_PARALLELL :
				{
					CVec3Dfp4 LightVec = pL->GetDirection();
					for(int v = 0; v < nV; v++)
					{
						fp4 dotp = (LightVec * lReflect[v]) * pSqrt[v];
						if (FloatIsNeg(dotp)) continue;
						if (_Power)
						{
							int Pow = _Power;
							if (Pow & 1) dotp = Sqr(dotp);
							if (Pow & 2) dotp = Sqr(Sqr(dotp));
							if (Pow & 4) dotp = Sqr(Sqr(Sqr(dotp)));
							if (Pow & 8) dotp = Sqr(Sqr(Sqr(Sqr(dotp))));
						}
						dotp *= _Scale;
						pLight[v][0] += pL->m_Intensity.k[0] * dotp;
						pLight[v][1] += pL->m_Intensity.k[1] * dotp;
						pLight[v][2] += pL->m_Intensity.k[2] * dotp;
					}

					break;
				}

			case CXR_LIGHTTYPE_AMBIENT :
				{
					break;
				}

			case CXR_LIGHTTYPE_LIGHTVOLUME :
				{
					fp4 Bias = pL->m_Range;
					CVec3Dfp4 LightVec = pL->GetDirection();
					for(int v = 0; v < nV; v++)
					{
						fp4 dotp = (LightVec * lReflect[v] + Bias) * pSqrt[v];
						if (FloatIsNeg(dotp)) continue;
						if (_Power)
						{
							int Pow = _Power;
							if (Pow & 1) dotp = Sqr(dotp);
							if (Pow & 2) dotp = Sqr(Sqr(dotp));
							if (Pow & 4) dotp = Sqr(Sqr(Sqr(dotp)));
							if (Pow & 8) dotp = Sqr(Sqr(Sqr(Sqr(dotp))));
						}
						dotp *= _Scale;
						pLight[v][0] += pL->m_Intensity.k[0] * dotp;
						pLight[v][1] += pL->m_Intensity.k[1] * dotp;
						pLight[v][2] += pL->m_Intensity.k[2] * dotp;
					}
					break;
				}

			case CXR_LIGHTTYPE_POINT :
			default :
				{
					{
						CVec3Dfp4 Pos = pL->GetPosition();
						for(int v = 0; v < nV; v++)
						{
							CVec3Dfp4 VLight;
							Pos.Sub(pVV[v], VLight);
							pRecp[v] = VLight.LengthSqr();
							pDot[v] = VLight * lReflect[v];
						}
					}

					SIMD_RecpSqrt8(pRecp, NULL, _nV);

					{
						for(int v = 0; v < nV; v++)
						{
							if (pRecp[v] < pL->m_RangeInv) continue;
							fp4 dotp = pDot[v] * pSqrt[v] * pRecp[v];
							if (FloatIsNeg(dotp)) continue;

							if (_Power)
							{
								int Pow = _Power;
								if (Pow & 1) dotp = Sqr(dotp);
								if (Pow & 2) dotp = Sqr(Sqr(dotp));
								if (Pow & 4) dotp = Sqr(Sqr(Sqr(dotp)));
								if (Pow & 8) dotp = Sqr(Sqr(Sqr(Sqr(dotp))));
							}

							dotp *= (pL->m_Range*pRecp[v] - 1.0f) * _Scale;

							pLight[v][0] += pL->m_Intensity.k[0] * dotp;
							pLight[v][1] += pL->m_Intensity.k[1] * dotp;
							pLight[v][2] += pL->m_Intensity.k[2] * dotp;
						}
					}
				}
			}
			pL = pL->m_pNext;
		}

		SIMD_ConvertRGB(pLight, (uint32*)_pVLight, _nV, _Alpha);

	}

/*	{
		int nD = m_CurrentWLS.m_nDynamic; int nS = m_CurrentWLS.m_nStatic;
		LogFile(T_String("Light ", Time) + CStrF(", nV %d, nDyn %d, nStat %d, TAvg ", nV, nD, nS) +
				T_String("", Time/(nD + nS)) + CStrF(", Clocks %f", Time / (nV*(nD + nS))));
	}*/
}

