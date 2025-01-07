#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"
#include "CSurfaceKey.h"
#include "../../../../Shared/MOS/Classes/Render/MWireContainer.h"
#include "WModel_EffectSystem.h"

//----------------------------------------------------------------------

#define LENGTH_SUBPRECISION (10.0f)

//----------------------------------------------------------------------

enum INSTANTTRAIL_FLAGS
{
	FLAGS_ALIGNFADE		= 0x01,
	FLAGS_FADEEDGES		= 0x02
};

//----------------------------------------------------------------------

#define LERP(a,b,t) ((a) + ((b) - (a))*t)
#define FRAC(a) ((a) - M_Floor(a))

#ifndef M_DISABLE_TODELETE

//----------------------------------------------------------------------

static const char* slINSTANTTRAIL_FLAGS[] = { "ALIGNFADE", "FADEEDGES", NULL };

//----------------------------------------------------------------------
// CXR_Model_InstantTrail
//----------------------------------------------------------------------

class CXR_Model_InstantTrail : public CXR_Model_Custom
{

	MRTC_DECLARE;

private:

	//----------------------------------------------------------------------

	fp32					m_Time;
	fp32					m_Duration;

	CXR_BeamStrip*		m_BeamStrips;
	int32				m_NumBeamStrips, m_MaxNumBeamStrips;

	CXR_VertexBuffer*	m_pVB;
	CSurfaceKey			m_SK;

	CVec3Dfp32			m_Pos, m_RevDir;
	fp32					m_Length;

	fp32					m_DurationFade;

	int					m_iSurface;
	fp32					m_Width;
	fp32					m_TexRepeatLength;
	int32				m_NumSegments;
	fp32					m_SegmentLength;
	fp32					m_FadeInTime, m_FadeOutTime;
	fp32					m_WantedDuration;
	int32				m_Flags;

	CWireContainer*		m_pWC;

	//----------------------------------------------------------------------

	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
	{
		MAUTOSTRIP(CXR_Model_InstantTrail_GetBound_Box, MAUTOSTRIP_VOID);
		if (_pAnimState == NULL)
		{
			_Box.m_Min = 0;
			_Box.m_Max = 0;
		}
		else
		{
			_Box.m_Min = -m_Width;
			_Box.m_Max = +m_Width;

			m_Length = _pAnimState->m_Anim1 / LENGTH_SUBPRECISION;
			_Box.m_Min[0] -= m_Length;
		}

		return;
	}

	//----------------------------------------------------------------------

	void
	AddBeamStrip(CXR_BeamStrip& beamstrip)
	{
		MAUTOSTRIP(CXR_Model_InstantTrail_AddBeamStrip, MAUTOSTRIP_VOID);
		if (m_NumBeamStrips >= m_MaxNumBeamStrips)
			return;

		m_BeamStrips[m_NumBeamStrips] = beamstrip;
		m_NumBeamStrips++;
	}

	//----------------------------------------------------------------------

	void
	GenerateTrail()
	{
		MAUTOSTRIP(CXR_Model_InstantTrail_GenerateTrail, MAUTOSTRIP_VOID);

		if (m_pWC && false)
		{
			m_pWC->RenderWire(m_Pos + m_RevDir * m_Length, m_Pos, 0xFFFFFFFF, 20.0f);
		}

		CXR_BeamStrip beamstrip;

		for (int iBeamStrip = 0; iBeamStrip < m_MaxNumBeamStrips; iBeamStrip++)
		{
			fp32 Alpha = 1.0f * m_DurationFade;
			fp32 TrailFraction = (fp32)iBeamStrip / (fp32)m_MaxNumBeamStrips;

			if ((m_Flags & FLAGS_FADEEDGES) && ((iBeamStrip == 0) || (iBeamStrip == (m_MaxNumBeamStrips - 1))))
			{
				Alpha = 0;
			}

			beamstrip.m_Flags = (iBeamStrip == 0) ? CXR_BEAMFLAGS_BEGINCHAIN : 0;
			beamstrip.m_Pos = m_Pos + (m_RevDir * m_Length) * (1.0f - TrailFraction);
			beamstrip.m_Width = m_Width;
			beamstrip.m_TextureYOfs = (m_Length * TrailFraction) / m_TexRepeatLength;
			beamstrip.m_Color = 0x00FFFFFF | (((int32)(Alpha * 255.0f) << 24));

			AddBeamStrip(beamstrip);
		}
	}
	
	//----------------------------------------------------------------------

	void Generate(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _LocalToWorld, const CMat4Dfp32& _WorldToCamera)
	{
		MAUTOSTRIP(CXR_Model_InstantTrail_Generate, MAUTOSTRIP_VOID);

		CXR_BeamStrip	Beamstrips[CXR_Util::MAXBEAMS];
		int32			NumBeamstrips = CXR_Util::MAXBEAMS;

		m_Time = _pAnimState->m_AnimTime0.GetTime(); // CMTIMEFIX
		m_Duration = _pAnimState->m_AnimTime1.GetTime(); // CMTIMEFIX

		m_DurationFade = ::GetFade(m_Time, m_WantedDuration, m_FadeInTime, m_FadeOutTime);

		if (m_DurationFade == 0)
			return;

		m_Pos = CVec3Dfp32::GetMatrixRow(_LocalToWorld, 3);
		m_RevDir = -CVec3Dfp32::GetMatrixRow(_LocalToWorld, 0);
		m_Length = _pAnimState->m_Anim1 / LENGTH_SUBPRECISION;

		if (m_SegmentLength > 0)
			m_NumSegments = m_Length / m_SegmentLength;

		if (m_TexRepeatLength == 0)
			m_TexRepeatLength = m_Length;

		if ((m_Flags & FLAGS_FADEEDGES) && (m_NumSegments < 4))
			m_NumSegments = 4;

		m_pVB = AllocVB();
		if (m_pVB == NULL)
			return;

		m_SK.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface);

		m_BeamStrips = Beamstrips;
		m_MaxNumBeamStrips = Min(int32(NumBeamstrips), int32(m_NumSegments + 1));
		m_NumBeamStrips = 0;

		GenerateTrail();

		CMat4Dfp32 Unit; Unit.Unit();

		if (CXR_Util::Render_BeamStrip2(_pRenderParams->m_pVBM, m_pVB, 
										Unit, _WorldToCamera, 
										m_BeamStrips, m_NumBeamStrips,
										((m_Flags & FLAGS_ALIGNFADE) ? CXR_BEAMFLAGS_EDGEFADE : 0)))
		{
			m_SK.Render(m_pVB, _pRenderParams->m_pVBM, m_pEngine);
		}
	}

	//----------------------------------------------------------------------

	void OnEvalKey(uint32 _KeyHash, const CRegistry *_pKey)
	{
		MAUTOSTRIP(CXR_Model_InstantTrail_OnEvalKey, MAUTOSTRIP_VOID);
		CFStr Name = _pKey->GetThisName();
		CFStr Value = _pKey->GetThisValue();

		const fp32 Valuef = Value.Val_fp64();
		const int Valuei = Value.Val_int();

		if (Name == "SURFACE")
			m_iSurface = GetSurfaceID(Value);
		else if (Name == "WIDTH")
			m_Width = Valuef;
		else if (Name == "TEXREPLEN")
			m_TexRepeatLength = Valuef;
		else if (Name == "SEGMENTS")
		{
			m_NumSegments = Valuei;
			m_SegmentLength = 0;
		}
		else if (Name == "SEGLEN")
		{
			m_NumSegments = 0;
			m_SegmentLength = Valuei;
		}
		else if (Name == "DURATION")
			m_WantedDuration = Valuef;
		else if (Name == "FADEINTIME")
			m_FadeInTime = Valuef;
		else if (Name == "FADEOUTTIME")
			m_FadeOutTime = Valuef;
		else if (Name == "FLAGS")
			m_Flags = Value.TranslateFlags(slINSTANTTRAIL_FLAGS);
		else
			CXR_Model_Custom::OnEvalKey(_pKey);
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *_params)
	{
		MAUTOSTRIP(CXR_Model_InstantTrail_OnCreate, MAUTOSTRIP_VOID);
		// FIXME: Mondelore. SegmentLength istaället för antalet segment, så att inte alla segment trycks ihop för korta trails.

		m_iSurface = GetSurfaceID("");
		m_Length = 1000.0f;
		m_Width = 20.0f;
		m_TexRepeatLength = 0;
		m_NumSegments = 1;
		m_SegmentLength = 0;
		m_FadeInTime = 0;
		m_FadeOutTime = 0;
		m_WantedDuration = 0;
		m_Flags = 0;

		ParseKeys(_params);
	}

	//----------------------------------------------------------------------

	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_InstantTrail_Render, MAUTOSTRIP_VOID);
		MSCOPESHORT(CXR_Model_InstantTrail::Render);
		m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));

		Generate(_pAnimState, _WMat, _VMat);
	}

};

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_InstantTrail, CXR_Model_Custom);
#endif

//----------------------------------------------------------------------


// -------------------------------------------------------------------
//  CXR_Model_InstantTrail2
// -------------------------------------------------------------------
class CXR_Model_InstantTrail2 : public CXR_Model_Custom
{
	MRTC_DECLARE;

public:
	int m_iTrailSurface[2];
	int m_iParticleSurface[2];
	int m_iTracerFreq;
	fp32 m_Speed;
	fp32	m_LengthRand;
	fp32 m_Length;
	fp32 m_Width;

	CXR_Model_InstantTrail2()
	{
		SetThreadSafe(true);
	}

	virtual void Create(const char* _pParam)
	{
		CXR_Model_Custom::Create(_pParam);

		CFStr St = _pParam;
		m_Width = St.GetStrSep(",").Val_fp64();
		m_Length = St.GetStrSep(",").Val_fp64();
		m_LengthRand = St.GetStrSep(",").Val_fp64();
		m_Speed = St.GetStrSep(",").Val_fp64();
		m_iTracerFreq = St.GetStrSep(",").Val_int();
		m_iTrailSurface[0] = GetSurfaceID(St.GetStrSep(","));
		m_iParticleSurface[0] = GetSurfaceID(St.GetStrSep(","));
		m_iTrailSurface[1] = GetSurfaceID(St.GetStrSep(","));
		m_iParticleSurface[1] = GetSurfaceID(St.GetStrSep(","));
	}

	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
	{
		if(!_pAnimState)
			return CXR_Model_Custom::GetBound_Box(_Box, _pAnimState);

		const fp32 LengthRand = (-1.0f + 2.0f * MFloat_GetRand(_pAnimState->m_Anim0)) * m_LengthRand;

		fp32 ProjectileDistance = M_Sqrt(_pAnimState->m_AnimAttr0);
		fp32 TrailDistance = m_Speed * _pAnimState->m_AnimTime0.GetTime() * 32.0f;

		fp32 Diff = TrailDistance - ProjectileDistance;
		if(Diff > 0)
		{
			// Trail is ahead of projectile
			_Box.m_Max = CVec3Dfp32(1, 1, 1);
			_Box.m_Min = CVec3Dfp32(-1 - (m_Length + LengthRand), -1, -1);
		}
		else
		{
			_Box.m_Max = CVec3Dfp32(1 + Diff, 1, 1);
			_Box.m_Min = CVec3Dfp32(Diff - (m_Length + LengthRand) - 1, -1, -1);
		}
	}

	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
	{
		MSCOPESHORT(CXR_Model_InstantTrail2::Render);
		CMat4Dfp32 *pMat = (CMat4Dfp32 *)_pAnimState->m_Data[0];
		CVec3Dfp32 *pDir = (CVec3Dfp32 *)_pAnimState->m_Data[2];
		if(!pMat || !pDir)
			return;

		bool bNoLit = (m_iTracerFreq >= 0) ? ((m_iTracerFreq == 0) || (_pAnimState->m_Anim0 % m_iTracerFreq > 0)) : false;
		if(_pAnimState->m_AnimAttr0 <= 0)
			return;

		CMat4Dfp32 Mat;
		Mat.Unit();
		CVec3Dfp32::GetMatrixRow(*pMat, 3).SetMatrixRow(Mat, 3);
		CVec3Dfp32 Dir = *pDir;
		Dir.Normalize();
		
		/*if(bNoLit)
			return;*/

		CXR_BeamStrip Beam[2];
		Beam[0].m_Color = 0xffffffff;
		Beam[1].m_Color = 0xffffffff;
		Beam[0].m_Width = m_Width;
		Beam[1].m_Width = m_Width;

		fp32 ProjectileDistance = M_Sqrt(_pAnimState->m_AnimAttr0);
		fp32 TrailDistance = m_Speed * _pAnimState->m_AnimTime0.GetTime() * 32.0f;
		fp32 Length;
		const fp32 LengthRand = (-1.0f + 2.0f * MFloat_GetRand(_pAnimState->m_Anim0)) * m_LengthRand;
		if(TrailDistance > ProjectileDistance)
		{
			// Trail is ahead of projectile
			Length = (m_Length + LengthRand) - (TrailDistance - ProjectileDistance);
			if(Length < 0)
				return;

//			Beam[0].m_TextureYOfs = -(1.0f - Length / m_Length);
			Beam[0].m_TextureYOfs = 0;
			//Beam[0].m_Pos = CVec3Dfp32(ProjectileDistance, 0, 0);
			
			Beam[0].m_Pos = (Dir * ProjectileDistance);
		}
		else
		{
			Length = m_Length + LengthRand;
			//Beam[0].m_Pos = CVec3Dfp32(TrailDistance, 0, 0);
			Beam[0].m_TextureYOfs = 0;

			Beam[0].m_Pos = (Dir * TrailDistance);
		}

		if(TrailDistance < (m_Length+LengthRand))
		{
			// Trail isn't fully grown
			//Beam[1].m_Pos = CVec3Dfp32(0, 0, 0);
//			Beam[1].m_TextureYOfs = 1.0f - TrailDistance / m_Length;
			Beam[1].m_TextureYOfs = 0;

			Beam[1].m_Pos = CVec3Dfp32(0, 0, 0);
		}
		else
		{
			Beam[1].m_Pos = CVec3Dfp32(TrailDistance - (m_Length + LengthRand), 0, 0);
			Beam[1].m_TextureYOfs = 0;

			Beam[1].m_Pos = (Dir * (TrailDistance - (m_Length+LengthRand)));
		}

		if(TrailDistance < ProjectileDistance)
		{
			CXR_Particle2 Particle;
			Particle.m_Color = 0xffffffff;
			//Particle.m_Pos = Beam[0].m_Pos;
			//Particle.m_Pos -= CVec3Dfp32(4, 0, 0) * MinMT(TrailDistance / m_Length, 1.0f);
			//Particle.m_Pos *= *pMat;
			Particle.m_Pos = CVec3Dfp32::GetMatrixRow(*pMat, 3);
			Particle.m_Pos += Dir * (MinMT(TrailDistance / (m_Length+LengthRand), 1.0f) * TrailDistance);
			Particle.m_Size = m_Width / 2;
			CXR_VertexBuffer *pVB = AllocVB(_pRenderParams);
			if(CXR_Util::Render_Particles(_pRenderParams->m_pVBM, pVB, _VMat, &Particle, 1))
				Render_Surface(_pRenderParams, m_iParticleSurface[bNoLit ? 1 : 0], pVB, _pAnimState->m_AnimTime0);
		}

		CXR_VertexBuffer *pVB = AllocVB(_pRenderParams);
		//if(CXR_Util::Render_BeamStrip(m_pVBM, pVB, *pMat, _VMat, Beam, 2))
		if(CXR_Util::Render_BeamStrip(_pRenderParams->m_pVBM, pVB, Mat, _VMat, Beam, 2))
			Render_Surface(_pRenderParams, m_iTrailSurface[bNoLit ? 1 : 0], pVB, _pAnimState->m_AnimTime0);
	}
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_InstantTrail2, CXR_Model_Custom);



//----------------------------------------------------------------------


// -------------------------------------------------------------------
//  CXR_Model_TrailStrip
// -------------------------------------------------------------------
class CXR_Model_TrailStrip : public CXR_Model_Custom
{
	MRTC_DECLARE;

	uint16 m_SurfaceID;
public:
	CXR_Model_TrailStrip()
	{
		SetThreadSafe(true);
	}

	virtual void Create(const char* _pParam)
	{
		CXR_Model_Custom::Create(_pParam);
		CFStr St = _pParam;
		m_SurfaceID = GetSurfaceID(St.GetStrSep(",").GetStr());
	}

	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
	{
		if (!_pAnimState)
			CXR_Model_Custom::GetBound_Box(_Box, _pAnimState);
		else if (_pAnimState->m_Data[1] != -1)
		{
			_Box.m_Max = (fp32)_pAnimState->m_Data[1];
			_Box.m_Min = -_Box.m_Max;
		}
	}

	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
	{
		MSCOPESHORT(CXR_Model_TrailStrip::Render);
		CVec3Dfp32* pParam = (CVec3Dfp32*)_pAnimState->m_Data[0];
		if (!pParam)
			return;
		
		fp32 AnimTime = _pAnimState->m_AnimTime0.GetTime() * 3.0f;
		CXR_Engine* pEngine = _pRenderParams->m_pEngine;
		CXR_VBManager* pVBM = _pRenderParams->m_pVBM;
		CFXVBMAllocUtil AllocUtil;
		CSpline_BeamStrip BeamStrip(1);
		fp32 TexCoordLength = (_WMat.GetRow(3) - pParam[0]).Length() * 0.25f;
		CVec3Dfp32 Tng = (_pAnimState->m_Data[2] & M_Bit(0)) ? -_WMat.GetRow(0) : _WMat.GetRow(0);
		BeamStrip.AddBeamData(_WMat.GetRow(3), Tng, 0xffffffff, 3.0f, AnimTime, 0);
		BeamStrip.AddBeamData(pParam[0], pParam[1] * 2.0f, 0xffffffff, 2.0f, TexCoordLength + AnimTime, 0);

		BeamStrip.VBMem_Calculate(&AllocUtil);
		BeamStrip.SetSurface(0, m_SurfaceID);
		BeamStrip.RotateTexCoord(0, true);
		if (AllocUtil.Alloc(pVBM) && BeamStrip.Finalize(&AllocUtil, _VMat))
		{
			CXR_RenderInfo RenderInfo(pEngine);
			BeamStrip.Render(GetSurfaceContext(), pEngine, pVBM, &RenderInfo, _pAnimState->m_AnimTime0, 0, NULL);
		}
	}
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_TrailStrip, CXR_Model_Custom);



// -------------------------------------------------------------------
//  CXR_Model_MuzzleFlame
// -------------------------------------------------------------------
class CXR_Model_MuzzleFlame : public CXR_Model_Custom
{
	MRTC_DECLARE;

public:
	int m_iFlameSurface[10];
	int m_iBaseSurface[10];
	int m_nSurfaces;
	int m_nBaseQuads;
	fp32 m_Duration;
	fp32 m_Length;
	fp32 m_Width;
	int8 m_bRotateBase;

	CXR_Model_MuzzleFlame()
	{
		SetThreadSafe(true);
	}

	virtual void Create(const char* _pParam)
	{
		CXR_Model_Custom::Create(_pParam);

		CFStr St = _pParam;
		m_Duration = St.GetStrSep(",").Val_fp64();
		m_Width = St.GetStrSep(",").Val_fp64();
		m_Length = St.GetStrSep(",").Val_fp64();
		m_nSurfaces = 0;
		m_nBaseQuads = 3;
		m_bRotateBase = true;
		while(St != "")
		{
			m_iFlameSurface[m_nSurfaces] = GetSurfaceID(St.GetStrSep(","));
			m_iBaseSurface[m_nSurfaces] = GetSurfaceID(St.GetStrSep(","));
			m_nSurfaces++;
		}
	}

/*	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
	{
		if(!_pAnimState)
			return CXR_Model_Custom::GetBound_Box(_Box, _pAnimState);

		fp32 ProjectileDistance = M_Sqrt(_pAnimState->m_AnimAttr0);
		fp32 TrailDistance = SERVER_TICKSPERSECOND * m_Speed * _pAnimState->m_AnimTime0 * 32.0f / 20.0f;

		fp32 Diff = TrailDistance - ProjectileDistance;
		if(Diff > 0)
		{
			// Trail is ahead of projectile
			_Box.m_Max = CVec3Dfp32(1, 1, 1);
			_Box.m_Min = CVec3Dfp32(-1 - m_Length, -1, -1);
		}
		else
		{
			_Box.m_Max = CVec3Dfp32(1 + Diff, 1, 1);
			_Box.m_Min = CVec3Dfp32(Diff - m_Length - 1, -1, -1);
		}
	}*/

	virtual aint GetParam(int _Param)
	{
		if(_Param == CXR_MODEL_PARAM_TIMEMODE)
			return CXR_MODEL_TIMEMODE_CONTROLLED;
		else
			return CXR_Model_Custom::GetParam(_Param);
	}

	void RenderFlame(CXR_Model_Custom_RenderParams* _pRenderParams, CMTime _Time, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat, fp32 _Width, int _iSurface, fp32 _Scale)
	{
		// Flame
		CXR_BeamStrip Beam[2];
		Beam[0].m_Color = 0xffffffff;
		Beam[1].m_Color = 0xffffffff;
		Beam[0].m_Width = m_Width * _Scale;
		Beam[1].m_Width = m_Width * _Scale;
		Beam[0].m_TextureYOfs = 0;
		Beam[1].m_TextureYOfs = 0;
		Beam[0].m_Pos = CVec3Dfp32(m_Length * _Scale, 0, 0);
		Beam[1].m_Pos = CVec3Dfp32(-m_Length * _Scale / 20, 0, 0);

		CXR_VertexBuffer *pVB = AllocVB(_pRenderParams);
		if(pVB && CXR_Util::Render_BeamStrip(_pRenderParams->m_pVBM, pVB, _WMat, _VMat, Beam, 2))
		{
			if (!pVB->AllocVBChain(_pRenderParams->m_pVBM, false))
				return;
			int Fade = 255 - RoundToInt(255 * _Time.GetTime() / m_Duration);
			pVB->Geometry_ColorArray(NULL);
			pVB->Geometry_Color(CPixel32(Fade, Fade, Fade, 0));
			Render_Surface(_pRenderParams, m_iFlameSurface[_iSurface], pVB, _Time);
		}
	}

	void RenderBase(CXR_Model_Custom_RenderParams* _pRenderParams, CMTime _Time, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _L2V, int _iSurface, fp32 _XScale, fp32 _YScale)
	{
		// Base
		CXR_VertexBuffer *pVB = AllocVB(_pRenderParams);
		CVec3Dfp32 *pV = _pRenderParams->m_pVBM->Alloc_V3(4 * m_nBaseQuads);
		CVec2Dfp32 *pTV = _pRenderParams->m_pVBM->Alloc_V2(4 * m_nBaseQuads);
		uint16 *pP = _pRenderParams->m_pVBM->Alloc_Int16(6 * m_nBaseQuads);
		if(!pVB || !pV || !pTV || !pP)
			return;

		for(int i = 0; i < m_nBaseQuads; i++)
		{
			fp32 sinv, cosv;
			fp32 Rand = MFloat_GetRand(_pAnimState->m_Anim0 + i);
			fp32 Angle = 0;
			if(m_bRotateBase)
				Angle = ((_Time.GetTime() + 1000) * 8 * (Rand - 0.5f));
			QSinCos(Angle, sinv, cosv);
			fp32 a0 = (sinv + cosv) * m_Width;
			fp32 a1 = (sinv - cosv) * m_Width;

			pV[i * 4 + 0] = CVec3Dfp32(0, a1 * _XScale, a0 * _YScale) * _L2V;
			pV[i * 4 + 1] = CVec3Dfp32(0, -a0 * _XScale, a1 * _YScale) * _L2V;
			pV[i * 4 + 2] = CVec3Dfp32(0, -a1 * _XScale, -a0 * _YScale) * _L2V;
			pV[i * 4 + 3] = CVec3Dfp32(0, a0 * _XScale, -a1 * _YScale) * _L2V;
			pTV[i * 4 + 0] = CVec2Dfp32(0, 0);
			pTV[i * 4 + 1] = CVec2Dfp32(0, 1);
			pTV[i * 4 + 2] = CVec2Dfp32(1, 1);
			pTV[i * 4 + 3] = CVec2Dfp32(1, 0);
			pP[i * 6 + 0] = 0 + i * 4;
			pP[i * 6 + 1] = 1 + i * 4;
			pP[i * 6 + 2] = 2 + i * 4;
			pP[i * 6 + 3] = 0 + i * 4;
			pP[i * 6 + 4] = 2 + i * 4;
			pP[i * 6 + 5] = 3 + i * 4;
		}
		int Fade = 255 - RoundToInt(255 * _Time.GetTime() / m_Duration);
		pVB->Geometry_Color(CPixel32(Fade, Fade, Fade, 0));
		if (!pVB->AllocVBChain(_pRenderParams->m_pVBM, false))
			return;
		pVB->Geometry_VertexArray(pV, 4 * m_nBaseQuads, true);
		pVB->Geometry_TVertexArray(pTV, 0);
		pVB->Render_IndexedTriangles(pP, 2 * m_nBaseQuads);
		Render_Surface(_pRenderParams, m_iBaseSurface[_iSurface], pVB, _Time);
	}

	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
	{
		MSCOPESHORT(CXR_Model_MuzzleFlame::Render);
		if(_pAnimState->m_AnimTime0.IsReset() || _pAnimState->m_AnimTime0.Compare(m_Duration) > 0 || _pAnimState->m_AnimTime0.GetTime() < 0)
			return;

		int iSurface = _pAnimState->m_Anim0 % m_nSurfaces;

		RenderFlame(_pRenderParams, _pAnimState->m_AnimTime0, _pAnimState, _WMat, _VMat, m_Width, iSurface, 1.0f);

		CMat4Dfp32 L2V;
		_WMat.Multiply(_VMat, L2V);
		RenderBase(_pRenderParams, _pAnimState->m_AnimTime0, _pAnimState, L2V, iSurface, 1.0f, 1.0f);
	}
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_MuzzleFlame, CXR_Model_Custom);

// -------------------------------------------------------------------
//  CXR_Model_MuzzleRiot
// -------------------------------------------------------------------
class CXR_Model_MuzzleRiot : public CXR_Model_MuzzleFlame
{
	MRTC_DECLARE;

public:
	CXR_Model_MuzzleRiot()
	{
		SetThreadSafe(true);
	}

	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
	{
		MSCOPESHORT(CXR_Model_MuzzleRiot::Render);
		CVec3Dfp32 Ofs[3];
		Ofs[0] = CVec3Dfp32(3.7, 6.4, -17);
		Ofs[1] = CVec3Dfp32(3.7, -6.4, -17);
		Ofs[2] = CVec3Dfp32(-7.4, 0, -17);

		for(int i = 0; i < 3; i++)
		{
			Ofs[i].MultiplyMatrix3x3(_WMat);
			CMat4Dfp32 Base = _WMat;
			Base.RotY_x_M(0.25f);
			CVec3Dfp32::GetRow(Base, 3) += Ofs[i]; 
			CXR_Model_MuzzleFlame::Render(_pRenderParams, _pAnimState, Base, _VMat);
		}
	}
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_MuzzleRiot, CXR_Model_MuzzleFlame);

// -------------------------------------------------------------------
//  CXR_Model_MuzzleRiotSingle
// -------------------------------------------------------------------
class CXR_Model_MuzzleRiotSingle : public CXR_Model_MuzzleFlame
{
	MRTC_DECLARE;

public:
	CXR_Model_MuzzleRiotSingle()
	{
		SetThreadSafe(true);
	}

	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
	{
		MSCOPESHORT(CXR_Model_MuzzleRiotSingle::Render);

		CMat4Dfp32 Base = _WMat;
		Base.RotY_x_M(0.25f);
		CXR_Model_MuzzleFlame::Render(_pRenderParams, _pAnimState, Base, _VMat);
	}
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_MuzzleRiotSingle, CXR_Model_MuzzleFlame);

// -------------------------------------------------------------------
//  CXR_Model_MuzzleAttackDroid
// -------------------------------------------------------------------
class CXR_Model_MuzzleAttackDroid : public CXR_Model_MuzzleFlame
{
	MRTC_DECLARE;

public:
	CXR_Model_MuzzleAttackDroid()
	{
		SetThreadSafe(true);
	}

	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
	{
		MSCOPESHORT(CXR_Model_MuzzleRiot::Render);
		CVec3Dfp32 Ofs[3];
		Ofs[0] = CVec3Dfp32(0, 0, 0);
		Ofs[1] = CVec3Dfp32(1, 9, -17);
		Ofs[2] = CVec3Dfp32(1, -9, -17);

		for(int i = 0; i < 3; i++)
		{
			Ofs[i].MultiplyMatrix3x3(_WMat);
			CMat4Dfp32 Base = _WMat;
			CVec3Dfp32::GetRow(Base, 3) += Ofs[i]; 
			CXR_Model_MuzzleFlame::Render(_pRenderParams, _pAnimState, Base, _VMat);
		}
	}
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_MuzzleAttackDroid, CXR_Model_MuzzleFlame);
// -------------------------------------------------------------------
//  CXR_Model_MuzzleShotgun
// -------------------------------------------------------------------
class CXR_Model_MuzzleShotgun : public CXR_Model_MuzzleFlame
{
	MRTC_DECLARE;

public:
	CXR_Model_MuzzleShotgun()
	{
		SetThreadSafe(true);
	}

	virtual void Create(const char* _pParam)
	{
		CFStr St = _pParam;

//		m_Duration = St.GetStrSep(",").Val_fp64();

		CXR_Model_MuzzleFlame::Create(St);

		m_nBaseQuads = 1;
		m_bRotateBase = false;
	}

	virtual void Render(CXR_Model_Custom_RenderParams* _pRenderParams, const CXR_AnimState* _pAnimState, const CMat4Dfp32& _WMat, const CMat4Dfp32& _VMat)
	{
		CMTime Time = _pAnimState->m_AnimTime0 /*- CMTime::CreateFromSeconds(0.1f)*/;
		if(Time.Compare(m_Duration) > 0)
			return;

		fp32 sTime = Time.GetTime();
		if(sTime <= 0)
			return;

		int iSurface = _pAnimState->m_Anim0 % m_nSurfaces;
		fp32 XScale = 1.0f + 30 * sTime;
		fp32 YScale = 1.0f + 10 * sTime;

		CMat4Dfp32 Mat = _WMat;
		CVec3Dfp32::GetRow(Mat, 3) += CVec3Dfp32::GetRow(_WMat, 0) * ((sTime - 0.005f) * 400);

		RenderFlame(_pRenderParams, Time, _pAnimState, _WMat, _VMat, m_Width, iSurface, YScale);

		CMat4Dfp32 L2V;
		Mat.Multiply(_VMat, L2V);

		RenderBase(_pRenderParams, Time, _pAnimState, L2V, iSurface, XScale, YScale);
	}
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_MuzzleShotgun, CXR_Model_MuzzleFlame);

// -------------------------------------------------------------------
//  CXR_Model_None
// -------------------------------------------------------------------
class CXR_Model_None : public CXR_Model_Custom
{
	MRTC_DECLARE;

public:
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_None, CXR_Model_Custom);
