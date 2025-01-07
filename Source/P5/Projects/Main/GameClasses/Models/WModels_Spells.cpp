#include "PCH.h"

#include "WModels_Spells.h"


//-------------------------------------------------------------------
// Blizzard
//-------------------------------------------------------------------
void CXR_Model_Blizzard::OnCreate(const char* _pParam)
{
	SetFadeTime(0.5f);
	m_iTexture = GetTextureID("BLIZZARD");
	
	Critters_Create(MAXCRITTERS, 0.3f);
	
	m_iIceTexture = GetTextureID("LIGHTNING2");
	
	m_BoundRadius = 128;
}

bool CXR_Model_Blizzard::RenderIceRain(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
{
	fp32 Fade = GetFade(_pAnimState);
	if(Fade <= 0)
		return false;
	
	CVec3Dfp32 Light = CVec3Dfp32::GetMatrixRow(_WMat, 3) + CVec3Dfp32(0, 0, 50);
	CPixel32 Color;
	if(m_pWLS)
	{
		fp32 LightScale = (m_pEngine) ? m_pEngine->m_LightScale : 1.0f;
		m_WLS.CopyAndCull(m_pWLS, GetBound_Sphere(), Light, 3, 2);
		m_WLS.AddLightVolume(m_RenderInfo.m_pLightVolume, Light);
		m_WLS.InitLinks();
		CXR_WorldLightState::LightDiffuse(m_WLS.GetFirst(), 1, &Light, NULL, 1, &Color, 255, LightScale);
		Color *= CPixel32(0xc0, 0xc0, 0xff);
	}
	else
		Color = 0x00c0c0ff;
	
	const float Width = 64*2.5f;
	
	float Time = _pAnimState->m_AnimTime0;
	
	Critters_SetTime(Time);
	CXR_Beam lBeams[MAXCRITTERS];
	int iBeam = 0;
	for(int i = 0; i < MAXCRITTERS; i++)
	{
		int iRand = (Critters_GetID(i) << 2);
		int iCritter = Critters_GetID(i);
		float Age = Critters_GetTime(i)/* * (MFloat_GetRand(iRand++) * 0.4f + 1.0f)*/;
		
		int Alpha = Tooth(Age) * 255 * Fade;
		
		lBeams[iBeam].m_Color0 = Color | (Alpha << 24);
		lBeams[iBeam].m_Color1 = Color | (Alpha << 24);
		lBeams[iBeam].m_Pos[0] = (MFloat_GetRand(iRand++) - 0.5f) * Width;
		lBeams[iBeam].m_Pos[1] = (MFloat_GetRand(iRand++) - 0.5f) * Width;
		lBeams[iBeam].m_Pos[2] = 140 - Age * 140;
		lBeams[iBeam].m_Dir = CVec3Dfp32(0, 0, -16);
		lBeams[iBeam].m_Width = 2;
		iBeam++;
	}
	
	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	_pVB->m_pAttrib->Attrib_TextureID(0, m_iIceTexture);
	return CXR_Util::Render_Beams(m_pVBM, _pVB, _WMat, _VMat, lBeams, iBeam);
}

bool CXR_Model_Blizzard::RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
{
	{
		CXR_VertexBuffer *pVB = AllocVB();
		if(!pVB)
			return false;
		
		if(RenderIceRain(pVB, _pAnimState, _WMat, _VMat))
			m_pVBM->AddVB(pVB);
	}
	
	const int nSegments = 80;
	const float AngleSpeed = 0.1f * 2 * _PI;
	const float RotateSpeed = 0.4f * 2 * _PI;
	const float Radius = 64*2.5f;
	
	int iRand = 0;
	
	fp32 Fade = GetFade(_pAnimState) * 200;
	if(Fade < 10)
		return false;
	
	CVec3Dfp32 Light = CVec3Dfp32::GetMatrixRow(_WMat, 3) + CVec3Dfp32(0, 0, 98);
	CPixel32 Color;
	if(m_pWLS)
	{
		fp32 LightScale = (m_pEngine) ? m_pEngine->m_LightScale : 1.0f;
		m_WLS.CopyAndCull(m_pWLS, GetBound_Sphere(), Light, 3, 2);
		m_WLS.AddLightVolume(m_RenderInfo.m_pLightVolume, Light);
		m_WLS.InitLinks();
		CXR_WorldLightState::LightDiffuse(m_WLS.GetFirst(), 1, &Light, NULL, 1, &Color, 255, LightScale);
		Color *= CPixel32(0xa0, 0xa0, 0xff);
	}
	else
		Color = 0x00a0a0ff;
	
	CXR_BeamStrip Beams[nSegments];
	
	float Time = _pAnimState->m_AnimTime0;
	
	float Angle = Time * RotateSpeed;
	float dRadius = Radius / float(nSegments);
	float R = Radius;
	float ASpeed = AngleSpeed;
	int iBeam = 0;
	float Width = 12*4.0f;
	float dWidth = 6 * 1.0f / nSegments;
	for(int s = 0; s < nSegments; s++)
	{
		float qsin, qcos;
		QSinCos(Angle, qsin, qcos);
		Beams[iBeam].m_Pos = CVec3Dfp32(qsin * R, qcos * R, 120 + (MFloat_GetRand(iRand++) - 0.5f) * 28*2.0f /** (1.0f - float(s) / nSegments)*/);
		int Alpha = Fade * float(s) / nSegments + 55;
		Beams[iBeam].m_Color = Color | (Alpha << 24);
		Beams[iBeam].m_TextureYOfs = s * 0.2f;
		Beams[iBeam].m_Width = Width;
		float ADist = QSin(Time * 0.5f + s * 0.1f) * 0.1f;
		Angle += ASpeed + ADist;
		R -= dRadius;
		iBeam++;
		Width -= dWidth;
	}
	
	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	_pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
	return CXR_Util::Render_BeamStrip(m_pVBM, _pVB, _WMat, _VMat, Beams, iBeam, CXR_BEAMFLAGS_EDGEFADE);
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Blizzard, CXR_Model_Custom);

// -------------------------------------------------------------------
//  CXR_Model_Rail
// -------------------------------------------------------------------
void CXR_Model_Rail::OnCreate(const char *)
{
	SetFadeTime(0.2f);
	
	m_iTexture = GetTextureID("B_TRAIL01");
	m_BoundRadius = 512;
}

bool CXR_Model_Rail::RenderRings_Old(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
{
	fp32 Fade = GetFade(_pAnimState) * (float((uint32)_pAnimState->m_Colors[0]) / (65536.0f * 65536));
	if(Fade <= 0)
		return false;
	
	const float Length = _pAnimState->m_Anim0;
	const int MaxLength = 1024;
	const int RingDistance = 64;
	const int nSegments = 16;
	const float SegRadius = 12;
	
	int nRings = Length / RingDistance;
	
	CXR_BeamStrip Beams[CXR_Util::MAXBEAMS];
	int iBeam = 0;
	
	int32 ColorRGB = _pAnimState->m_Colors[0] & 0x00FFFFFF;
	
	uint32 Color = ColorRGB | (TruncToInt(Fade * 255) << 24);
	uint32 TColor = ColorRGB;
	
	float dAngle = 2 * _PI / nSegments;
	for(int r = 0; r < nRings; r++)
	{
		float Time = (_pAnimState->m_AnimTime0 - r * 0.05f) * 2;
		if(Time < 0)
			break;
		
		float Fwd = InvSqrInv(Time) * 10;
		float x = RingDistance * r + RingDistance / 2;
		float z = Time * 4;
		
		float WidthMul = (Time + 1.0f) / 2;
		
		Beams[iBeam].m_Pos = CVec3Dfp32(x, -2, SegRadius * WidthMul + z);
		Beams[iBeam].m_Width = 2;
		Beams[iBeam].m_TextureYOfs = 0;
		Beams[iBeam++].m_Color = TColor;
		
		Beams[iBeam].m_Pos = CVec3Dfp32(x, -0.1f, SegRadius * WidthMul + z);
		Beams[iBeam].m_Width = 2;
		Beams[iBeam].m_TextureYOfs = 0;
		Beams[iBeam++].m_Color = Color;
		
		float Angle = 0;
		for(int s = 1; s < nSegments + 1; s++)
		{
			float qsin, qcos;
			QSinCos(Angle, qsin, qcos);
			
			Beams[iBeam].m_Pos = CVec3Dfp32(x, qsin * SegRadius * WidthMul, qcos * SegRadius * WidthMul + z);
			Beams[iBeam].m_Color = Color;
			Beams[iBeam].m_Width = 2;
			Beams[iBeam].m_TextureYOfs = 0;
			iBeam++;
			
			Angle += dAngle;
		}
		
		Beams[iBeam].m_Pos = CVec3Dfp32(x, -2, SegRadius * WidthMul + z);
		Beams[iBeam].m_Width = 2;
		Beams[iBeam].m_TextureYOfs = 0;
		Beams[iBeam++].m_Color = Color;
		
		Beams[iBeam].m_Pos = CVec3Dfp32(x, 0, SegRadius * WidthMul + z);
		Beams[iBeam].m_Width = 2;
		Beams[iBeam].m_TextureYOfs = 0;
		Beams[iBeam++].m_Color = TColor;
	}
	
	_pVB->Geometry_Color(0xffffffff);
	_pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	
	return CXR_Util::Render_BeamStrip(m_pVBM, _pVB, _WMat, _VMat, Beams, iBeam);
}

void CXR_Model_Rail::OnPreRender(const CXR_AnimState *_pAnimState, const CMat43fp32 &_WMat)
{
	AddDistortLight(_pAnimState, CVec3Dfp32::GetMatrixRow(_WMat, 3),
		CVec3Dfp32(0.8f, 0.8f, 0.8f), CVec3Dfp32(0.2f, 0.2f, 0.2f), 128, 0);
}

void CXR_Model_Rail::RenderRings(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
{
	fp32 Fade = GetFade(_pAnimState, 0.5f) * (float((uint32)_pAnimState->m_Colors[0]) / (65536.0f * 65536));
	if(Fade <= 0)
		return;
	
	const float Length = _pAnimState->m_Anim0;
	const float SegLength = 8;
	const int nBeams = 3;
	
	int32 ColorRGB = _pAnimState->m_Colors[0] & 0x00FFFFFF;
	
	for(int b = 0; b  < nBeams; b++)
	{
		int iRand = _pAnimState->m_Anim0 + b * 200;
		int iBaseRand = _pAnimState->m_Anim0 + b * 1000;
		
		CXR_VertexBuffer *pVB = AllocVB();
		if(!pVB)
			return;
		
		float Time = _pAnimState->m_AnimTime0;
		float Time05 = _pAnimState->m_AnimTime0 * 0.5f;
		
		CXR_BeamStrip Beams[CXR_Util::MAXBEAMS];
		int iBeam = 0;
		
		float Radius = Time * 3 + 3 + b;
		
		int nSegments = Min(int(Length / SegLength), CXR_Util::MAXBEAMS - 1);
		float Distance = 0;
		for(int s = 0; s < nSegments; s++)
		{
			float FadeTime = Tooth(Max(Min(float((Time - s * 0.002f) * 3), 1.0f), 0.0f));
			//				if(FadeTime <= 0)
			//					continue;
			
			uint32 Color = ColorRGB | (TruncToInt(FadeTime * 255) << 24);
			
			/*				float a;
			{
			float _ITime = float(s) / 5;
			int ITime = _ITime;
			fp32 Frac = _ITime - ITime;
			
			  float r0;
			  float r1;
			  
				if(Time05 > MFloat_GetRand(ITime + iBaseRand))
				r0 = MFloat_GetRand(ITime + iBaseRand);
				else
				r0 = MFloat_GetRand(ITime + 1000 + iBaseRand);
				
				  if(Time05 > MFloat_GetRand(ITime + 1 + iBaseRand))
				  r1 = MFloat_GetRand(ITime + 1 + iBaseRand);
				  else
				  r1 = MFloat_GetRand(ITime + 1001 + iBaseRand);
				  
					a = (r0 + (r1 - r0) * Frac) * 2 * _PI;
		}*/
			
			float a = GetRandInterpolated(iBaseRand, float(s) / 5) * 2 * _PI;
			
			float qsin, qcos;
			QSinCos(a, qsin, qcos);
			
			Beams[iBeam].m_Pos = CVec3Dfp32(Distance, qsin * Radius, qcos * Radius);
			Beams[iBeam].m_Color = Color;
			Beams[iBeam].m_Width = 2;
			Beams[iBeam].m_TextureYOfs = 0;
			iBeam++;
			Distance += SegLength;
		}
		Beams[0].m_Color =  Beams[0].m_Color & 0x00FFFFFF;
		Beams[iBeam - 1].m_Color = Beams[iBeam - 1].m_Color & 0x00FFFFFF;
		
		pVB->Geometry_Color(0xffffffff);
		pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
		pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
		
		if(CXR_Util::Render_BeamStrip(m_pVBM, pVB, _WMat, _VMat, Beams, iBeam))
			m_pVBM->AddVB(pVB);
	}
}

void CXR_Model_Rail::Render(const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, const CMat43fp32& _VMat)
{
	{
		RenderRings(_pAnimState, _WMat, _VMat);
	}
	
	/*		fp32 Fade = GetFadeDown(_pAnimState) * (float((uint32)_pAnimState->m_Colors[0]) / (65536.0f * 65536), 0.5f);
	if(Fade <= 0)
	return;
	
	  const float Length = _pAnimState->m_Anim0;
	  const float Radius = 20;
	  
		uint32 Color = 0xff000000;
		Color = (Color & 0xffffff) | (TruncToInt(Color * Fade) & 0xff000000);
		
		  float Time = _pAnimState->m_AnimTime0;
		  
			CVec3Dfp32 Edge[2];
			Edge[0] = CVec3Dfp32(Radius * Sqr(Time), 0, 0);
			Edge[1] = CVec3Dfp32(Radius * Sqr(Time), 0, Length);
			
			  CMat4Dfp32 Mat, Mat0, L2V;
			  Mat.SetYRotation(0.25f);
			  Mat.Multiply(_WMat, Mat0);
			  Mat0.Multiply(_VMat, L2V);
			  
				CXR_VertexBuffer *pVB = CXR_Util::Create_SOR(m_pVBM, L2V, Edge, 2, 12);
				
				  pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;
				  pVB->Geometry_Color(Color);
				  pVB->m_pAttrib->Attrib_TextureID(0, 0);
				  pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
				  
	m_pVBM->AddVB(pVB);*/
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Rail, CXR_Model_Custom);

// -------------------------------------------------------------------
//  CXR_Model_FlameTounge - How do you spell Tounge???
// -------------------------------------------------------------------
void CXR_Model_FlameTounge::OnCreate(const char *)
{
	m_BoundRadius = 128;
	
	m_iTexture = GetTextureID("INFERNO0");
}

bool CXR_Model_FlameTounge::RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
{
	const float Length = _pAnimState->m_Anim0;
	const int nBeamStrips = 3;
	
	float Time = _pAnimState->m_AnimTime0;
	const int Color = _pAnimState->m_Colors[0] & 0x00ffffff;
	const int MaxAlpha = uint32(_pAnimState->m_Colors[0]) >> 24;
	float Duration = _pAnimState->m_AnimTime1;
	float Attack = _pAnimState->m_AnimAttr0;
	
	int iRand = _pAnimState->m_Anim1;
	int iBeam = 0;
	
	float WaveTime = Time * 3;
	float WaveFrac = WaveTime - TruncToInt(WaveTime);
	
	int nSegments = Min(int(Length / 16) + 1, CXR_Util::MAXBEAMS - 1);
	
	CXR_BeamStrip lBeams[CXR_Util::MAXBEAMS];
	
	fp32 Spline[4];
	SetSplineWeights(WaveFrac, Spline);
	
	CVec3Dfp32 Ofs(0);
	
	float fStart;
	if(Duration == 0)
		fStart = 0;
	else
		fStart = (Time * nSegments) / Duration;
	int start = fStart;
	int StartFrac = 255 - 255 * (fStart - start);
	
	float fAttack;
	if(Attack == 0)
		fAttack = nSegments;
	else
		fAttack = Min((Time * nSegments) / Attack, float(nSegments));
	int attack = fAttack;
	int AttackFrac = 255 * (fAttack - attack);
	
	if(attack - start < 3)
		return false;
	
	for(int b = 0; b < nBeamStrips; b++)
	{
		float TOfs = -Time * 3 + MFloat_GetRand(iRand++) + MFloat_GetRand(iRand++) * Time * 0.5f;
		
		for(int s = start; s < attack; s++)
		{
			int iRand = b * 20 + s * 5;
			lBeams[iBeam].m_Pos = CVec3Dfp32(s * 16 + 16 * (GetRandSplineInterpolated(Spline, iRand++, WaveTime) - 0.5f),
				8 * (GetRandSplineInterpolated(Spline, iRand++, WaveTime) - 0.5f),
				8 * (GetRandSplineInterpolated(Spline, iRand++, WaveTime) - 0.5f));
			uint32 Alpha = TruncToInt(GetRandSplineInterpolated(Spline, iRand++, WaveTime) * MaxAlpha) << 24;
			Alpha = 0xff000000;
			if(s == start || s == attack - 1)
				Alpha = 0;
			if(s == start + 1)
				Alpha = ((Alpha >> 8) * StartFrac) & 0xff000000;
			if(s == attack - 2)
				Alpha = ((Alpha >> 8) * AttackFrac) & 0xff000000;
			lBeams[iBeam].m_Color = Color | Alpha;
			lBeams[iBeam].m_TextureYOfs = TOfs;
			lBeams[iBeam].m_Width = GetRandSplineInterpolated(Spline, iRand++, WaveTime) * 2 + 6;
			iBeam++;
		}
	}
	
	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	_pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
	return CXR_Util::Render_BeamStrip(m_pVBM, _pVB, _WMat, _VMat, lBeams, iBeam);
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_FlameTounge, CXR_Model_Custom);

// -------------------------------------------------------------------
//  CXR_Model_IceTounge - How do you spell Tounge???
// -------------------------------------------------------------------
void CXR_Model_IceTounge::OnCreate(const char *)
{
	m_BoundRadius = 128;
	
	m_iTexture = GetTextureID("BLIZZARD");
}

bool CXR_Model_IceTounge::RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
{
	const float Length = _pAnimState->m_Anim0;
	const int nBeamStrips = 3;
	
	float Time = _pAnimState->m_AnimTime0;
	const int Color = _pAnimState->m_Colors[0] & 0x00ffffff;
	const int MaxAlpha = uint32(_pAnimState->m_Colors[0]) >> 24;
	float Duration = _pAnimState->m_AnimTime1;
	float Attack = _pAnimState->m_AnimAttr0;
	
	int iRand = _pAnimState->m_Anim1;
	int iBeam = 0;
	
	float WaveTime = Time * 3;
	float WaveFrac = WaveTime - TruncToInt(WaveTime);
	
	int nSegments = Min(int(Length / 16) + 1, CXR_Util::MAXBEAMS - 1);
	
	CXR_BeamStrip lBeams[CXR_Util::MAXBEAMS];
	
	fp32 Spline[4];
	SetSplineWeights(WaveFrac, Spline);
	
	CVec3Dfp32 Ofs(0);
	
	float fStart;
	if(Duration == 0)
		fStart = 0;
	else
		fStart = (Time * nSegments) / Duration;
	int start = fStart;
	int StartFrac = 255 - 255 * (fStart - start);
	
	float fAttack;
	if(Attack == 0)
		fAttack = nSegments;
	else
		fAttack = Min((Time * nSegments) / Attack, float(nSegments));
	int attack = fAttack;
	int AttackFrac = 255 * (fAttack - attack);
	
	if(attack - start < 3)
		return false;
	
	for(int b = 0; b < nBeamStrips; b++)
	{
		float TOfs = -Time * 3 + MFloat_GetRand(iRand++) + MFloat_GetRand(iRand++) * Time * 0.5f;
		
		for(int s = start; s < attack; s++)
		{
			int iRand = b * 20 + s * 5;
			lBeams[iBeam].m_Pos = CVec3Dfp32(s * 16 + 16 * (GetRandSplineInterpolated(Spline, iRand++, WaveTime) - 0.5f),
				8 * (GetRandSplineInterpolated(Spline, iRand++, WaveTime) - 0.5f),
				8 * (GetRandSplineInterpolated(Spline, iRand++, WaveTime) - 0.5f));
			uint32 Alpha = TruncToInt(GetRandSplineInterpolated(Spline, iRand++, WaveTime) * MaxAlpha) << 24;
			Alpha = 0xff000000;
			if(s == start || s == attack - 1)
				Alpha = 0;
			if(s == start + 1)
				Alpha = ((Alpha >> 8) * StartFrac) & 0xff000000;
			if(s == attack - 2)
				Alpha = ((Alpha >> 8) * AttackFrac) & 0xff000000;
			lBeams[iBeam].m_Color = Color | Alpha;
			lBeams[iBeam].m_TextureYOfs = TOfs;
			lBeams[iBeam].m_Width = GetRandSplineInterpolated(Spline, iRand++, WaveTime) * 2 + 6;
			iBeam++;
		}
	}
	
	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	_pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
	return CXR_Util::Render_BeamStrip(m_pVBM, _pVB, _WMat, _VMat, lBeams, iBeam);
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_IceTounge, CXR_Model_Custom);

// -------------------------------------------------------------------
//  CXR_Model_Ball
// -------------------------------------------------------------------
class CXR_Model_Ball : public CXR_Model_Custom
{
protected:
	int m_iTexture;
	
	virtual bool RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		// m_Anim0 = Length
		// m_Anim1 = Width

		// TODO: Take iRand from animstate

		const int nPass = 4;
		const int nMaxBeams = 24;

		CMat43fp32 L2V;
		_WMat.Multiply(_VMat, L2V);
		float Lod = 4096.0f / L2V.k[3][2];
		int nBeams = Lod;
		int ColorLast;
		if(nBeams > nMaxBeams)
		{
			nBeams = nMaxBeams;
			ColorLast = 0xffffffff;
		}
		else
			ColorLast = CPixel32(255, 255, 255, 255 * (Lod - nBeams));


		float Length = _pAnimState->m_Anim0;
		float Width = _pAnimState->m_Anim1;
		if(!Length)
			Length = 40;
		if(!Width)
			Width = 20;

		_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
		_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
		_pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
		{
			CXR_Beam Beams[nPass * nMaxBeams];

			int iRand = 0;
			int iBeam = 0;
			for(int p = 0; p < nPass; p++)
			{
				CMat43fp32 Rot;
				CVec3Dfp32 D(MFloat_GetRand(iRand++), MFloat_GetRand(iRand++), MFloat_GetRand(iRand++));
				D -= 0.5f;
				D *= _pAnimState->m_AnimTime0 * 0.1f / D.Length();
				D.CreateMatrixFromAngles(0, Rot);

				int iRandOld = iRand;
				for(int i = 0; i < nBeams; i++)
				{
					Beams[iBeam].m_Pos = 0;

					Beams[iBeam].m_Dir = CVec3Dfp32(MFloat_GetRand(iRand++) - 0.5f, MFloat_GetRand(iRand++) - 0.5f, MFloat_GetRand(iRand++) - 0.5f);

					Beams[iBeam].m_Dir *= Length / Beams[iBeam].m_Dir.Length();
					Beams[iBeam].m_Dir *= Rot;

					Beams[iBeam].m_Width = Width;

					if(i == nBeams - 1)
						Beams[iBeam].m_Color0 = ColorLast;
					else
						Beams[iBeam].m_Color0 = 0xffffffff;
					Beams[iBeam].m_Color1 = Beams[iBeam].m_Color0;
					iBeam++;
				}
				iRand = iRandOld + 200;
			}

			CXR_Util::Render_Beams(m_pVBM, _pVB, _WMat, _VMat, Beams, iBeam);
		}

		return true;
	}
};

// -------------------------------------------------------------------
//  CXR_Model_FireSphere
// -------------------------------------------------------------------
class CXR_Model_FireSphere : public CXR_Model_Ball
{
	MRTC_DECLARE;

	virtual void OnCreate(const char *)
	{
		m_iTexture = GetTextureID("FLAME1");
	}

	MACRO_OPERATOR_TPTR(CXR_Model_FireSphere);
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_FireSphere, CXR_Model_Ball);



// -------------------------------------------------------------------
//  CXR_Model_LightningSphere
// -------------------------------------------------------------------
class CXR_Model_LightningSphere : public CXR_Model_Ball
{
	MRTC_DECLARE;

	virtual void OnCreate(const char *)
	{
		m_iTexture = GetTextureID("FLASH1");
	}

	MACRO_OPERATOR_TPTR(CXR_Model_LightningSphere);
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_LightningSphere, CXR_Model_Ball);

