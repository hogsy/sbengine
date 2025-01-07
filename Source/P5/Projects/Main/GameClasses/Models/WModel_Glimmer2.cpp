#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

//----------------------------------------------------------------------
// CXR_Model_Glimmer2
//----------------------------------------------------------------------
class CXR_Model_Glimmer2 : public CXR_Model_Custom
{

	MRTC_DECLARE;

protected:

	int32 m_iTexture;
	int32 m_iSurface;

public:
	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState)
	{
		MAUTOSTRIP(CXR_Model_Glimmer2_GetBound_Sphere, 0.0f);
		if (_pAnimState)
		{
			return Max3(Max(M_Fabs((fp32)int16(_pAnimState->m_Colors[0])), M_Fabs((fp32)int16(_pAnimState->m_Colors[0] >> 16))),
					Max(M_Fabs((fp32)int16(_pAnimState->m_Colors[1])), M_Fabs((fp32)int16(_pAnimState->m_Colors[1] >> 16))),
					Max(M_Fabs((fp32)int16(_pAnimState->m_Colors[2])), M_Fabs((fp32)int16(_pAnimState->m_Colors[2] >> 16))));
		}
		else
			return 16*_SQRT3;
	}

	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
	{
		MAUTOSTRIP(CXR_Model_Glimmer2_GetBound_Box, MAUTOSTRIP_VOID);
		if (_pAnimState)
		{
			_Box.m_Min[0] = int16(_pAnimState->m_Colors[0]);
			_Box.m_Max[0] = int16(_pAnimState->m_Colors[0] >> 16);
			_Box.m_Min[1] = int16(_pAnimState->m_Colors[1]);
			_Box.m_Max[1] = int16(_pAnimState->m_Colors[1] >> 16);
			_Box.m_Min[2] = int16(_pAnimState->m_Colors[2]);
			_Box.m_Max[2] = int16(_pAnimState->m_Colors[2] >> 16);
		}
		else
		{
			_Box.m_Min[0] = -16;
			_Box.m_Max[0] = 16;
			_Box.m_Min[1] = -16;
			_Box.m_Max[1] = 16;
			_Box.m_Min[2] = -16;
			_Box.m_Max[2] = 16;
		}
	}

	//----------------------------------------------------------------------
	virtual void OnCreate(const char *surface)
	{
		MAUTOSTRIP(CXR_Model_Glimmer2_OnCreate, MAUTOSTRIP_VOID);
//		m_iTexture = m_pTextureContext->GetTextureID("Electric1");

		if (surface != NULL) 
			m_iSurface = GetSurfaceID(surface);
		else
			m_iSurface = GetSurfaceID("Glimmer2");

	}

	//----------------------------------------------------------------------
	virtual bool RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_Glimmer2_RenderVB, MAUTOSTRIP_VOID);
		const int32 MaxParticles = 80;
		CXR_Particle2 lParticles[MaxParticles];

		CMat43fp32 LocalToCamera;
		_WMat.Multiply(_VMat, LocalToCamera);

		const fp32 DistSqr = CVec3Dfp32::GetRow(LocalToCamera, 3).LengthSqr();
		if (DistSqr > Sqr(1000.0f))
			return false;

		if (!_pAnimState)
			return false;

		const fp32 Dist = GetCorrectedDistance(m_pEngine, M_Sqrt(DistSqr));
		fp32 LodLevel = 2.99f;
		if (Dist < 200.0f)
			LodLevel -= Dist * ( 1.0f / 100.0f );
		else
			LodLevel -= 2.0f + (Dist-200.0f) * ( 1.0f / 800.0f );

		const fp32 Fade = GetFade(_pAnimState);

		int ParticleType = CXR_PARTICLETYPE_QUAD | CXR_PARTICLETYPE_ANGLE;
		if (Dist > 150.0f)
		{
			ParticleType = CXR_PARTICLETYPE_TRIANGLE | CXR_PARTICLETYPE_ANGLE;
		}
		CBox3Dfp32 Box;
		Box.m_Min[0] = int16(_pAnimState->m_Colors[0]);
		Box.m_Max[0] = int16(_pAnimState->m_Colors[0] >> 16);
		Box.m_Min[1] = int16(_pAnimState->m_Colors[1]);
		Box.m_Max[1] = int16(_pAnimState->m_Colors[1] >> 16);
		Box.m_Min[2] = int16(_pAnimState->m_Colors[2]);
		Box.m_Max[2] = int16(_pAnimState->m_Colors[2] >> 16);

		CVec3Dfp32 Origin;
		CVec3Dfp32 Radius;
		Box.m_Min.Lerp(Box.m_Max, 0.5f, Origin);
		Box.m_Max.Sub(Origin, Radius);

		for(int i = 0; i < 3; i++)
			if (Radius[i] < 2.0f) Radius[i] = 2.0f;

		CVec3Dfp32 RadiusSorted(Radius);
		{
			//JK-TODO: Replade this with a QSort or something! (only 4 element bubblesort but bubblesort should never be used)
			for(int i = 0; i < 2; i++)
				for(int j = i+1; j < 3; j++)
					if (RadiusSorted[i] > RadiusSorted[j])
						Swap(RadiusSorted[i], RadiusSorted[j]);
		}

		fp32 MaxRadius = RadiusSorted[2];
		fp32 MinRadius = RadiusSorted[0];
		fp32 MedianRadius = RadiusSorted[1];

/*		fp32 AvgRadius = (Radius[0] + Radius[1] + Radius[2]) / 3.0f;
		fp32 MedianRadius = (Radius[0] != MaxRadius && Radius[0] != MinRadius) ? Radius[0] :
			((Radius[1] != MaxRadius && Radius[1] != MinRadius) ? Radius[1] : Radius[2]);
*/
//		fp32 Volume = Radius[0]* Radius[1]*Radius[2];

		int WavePhase0 = RoundToInt(_pAnimState->m_AnimTime0 * ( 65536.0f * 0.15f ));
		int WavePhase1 = RoundToInt(_pAnimState->m_AnimTime0 * ( 65536.0f * 0.19f ));
		int WavePhase2 = RoundToInt(_pAnimState->m_AnimTime0 * ( 65536.0f * 0.21356f ));
		int WavePhase3 = RoundToInt(_pAnimState->m_AnimTime0 * ( 65536.0f * 0.1476f ));
		int WavePhase4 = RoundToInt(_pAnimState->m_AnimTime0 * ( 65536.0f * 0.2136f ));
		int WavePhase5 = RoundToInt(_pAnimState->m_AnimTime0 * ( 65536.0f * 0.116f ));
		int WavePhase6 = RoundToInt(_pAnimState->m_AnimTime0 * ( 65536.0f * 1.716f ));
		int WavePeriod0 = 12137;
		int WavePeriod1 = 15172;
		int WavePeriod2 = 27191;
		int WavePeriod3 = 11172;
		int WavePeriod4 = 9191;
		int WavePeriod5 = 22472;
		int WavePeriod6 = 45472;

		int nParticles = 0;
//		fp32 CurrentSize = M_Sqrt(Volume) / 8.0;
		fp32 CurrentSize = Min(1.75f, M_Sqrt(MaxRadius * MedianRadius) * ( 1.0f / 4.0f ));
//		fp32 RadiusScale = 3.0f;
		fp32 RadiusScale = (1.0f / 2.0f);
		fp32 MaxFade = 0.15f;
		fp32 MinFade = 0.35f;

		int iLod = 0;
		for(;LodLevel > 0.0f; LodLevel -= 1.0f)
		{
			const fp32 ClampLOD = Clamp01(LodLevel) * Fade * 255.0f;
			const fp32 CurrentFade = MaxFade * ClampLOD;
			const fp32 CurrentFadeOffset = MinFade * ClampLOD;
			const int32 CurrentColor = 0x00ffffff;

			for(int i = 0; i < 20; i++)
			{
				CXR_Particle2& P = lParticles[nParticles++];
				const fp32 Wave0 = QSini(WavePhase0);
				const fp32 Wave1 = QSini(WavePhase1);
				const fp32 Wave2 = QSini(WavePhase2);
				const fp32 Wave3 = QSini(WavePhase3);
				const fp32 Wave4 = QSini(WavePhase4);
				const fp32 Wave5 = QSini(WavePhase5);
				const fp32 Wave6 = QSini(WavePhase6);
				const fp32 Wave6Pow2 = Sqr(Sqr((Wave6 + 1.0f) * 0.5f));

				fp32 Alpha = Wave6Pow2 * CurrentFade + CurrentFadeOffset;
				P.m_Color = CurrentColor + (RoundToInt(Alpha) << 24);
				P.m_Pos[0] = Origin[0] + (Wave0 + Wave1) * RadiusScale * Radius[0];
				P.m_Pos[1] = Origin[1] + (Wave2 + Wave3) * RadiusScale * Radius[1];
				P.m_Pos[2] = Origin[2] + (Wave4 + Wave5) * RadiusScale * Radius[2];
				P.m_Size = (10.0f + Wave6*4.0f) * CurrentSize;
				P.m_Angle = 0.125f;

				WavePhase0 += WavePeriod0;
				WavePhase1 += WavePeriod1;
				WavePhase2 += WavePeriod2;
				WavePhase3 += WavePeriod3;
				WavePhase4 += WavePeriod4;
				WavePhase5 += WavePeriod5;
				WavePhase6 += WavePeriod6;
			}

			CurrentSize *= 0.5f;
			RadiusScale *= 1.4f;
			MaxFade = Min(1.0f, MaxFade + 0.45f);
			MinFade = Max(0.0f, MinFade - 0.20f);
			iLod++;
		}

		if (CXR_Util::Render_Particles(m_pVBM, _pVB, LocalToCamera, lParticles, nParticles, NULL, ParticleType))
			Render_Surface(m_iSurface, _pVB, _pAnimState->m_AnimTime0);

		return false;
	}
};

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Glimmer2, CXR_Model_Custom);

//----------------------------------------------------------------------
	
