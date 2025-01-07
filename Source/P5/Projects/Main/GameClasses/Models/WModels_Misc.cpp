#include "PCH.h"

#include "WModels_Misc.h"

#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XREngineVar.h"

#include "../WObj_Char.h"

// -------------------------------------------------------------------
//  CClientData_Trail
// -------------------------------------------------------------------

void CClientData_Trail::operator=(const CXR_ModelInstance &_Instance)
{
	MAUTOSTRIP(CClientData_Trail_operator_assign, MAUTOSTRIP_VOID);
	const CClientData_Trail *pTrail = safe_cast<const CClientData_Trail >(&_Instance);
	if(pTrail)
	{
		memcpy(m_History, pTrail->m_History, sizeof(m_History));
		memcpy(m_Flags, pTrail->m_Flags, sizeof(m_Flags));
		m_LastGameTick = pTrail->m_LastGameTick;
		m_nHistory = pTrail->m_nHistory;
		m_iCurPos = pTrail->m_iCurPos;
		m_BoundBox = pTrail->m_BoundBox;
		m_LastIWMat = pTrail->m_LastIWMat;
	}
}

TPtr<CXR_ModelInstance> CClientData_Trail::Duplicate() const
{
	MAUTOSTRIP(CClientData_Trail_Duplicate, NULL);
	MSCOPE(CClientData_Trail::Duplicate, CUSTOMMODELS);
	TPtr<CClientData_Trail> spObj = DNew(CClientData_Trail) CClientData_Trail;
	*spObj = *(CXR_ModelInstance *)this;
	return TPtr<CXR_ModelInstance>(spObj);
}

CClientData_Trail::CClientData_Trail()
{
	MAUTOSTRIP(CClientData_Trail_ctor, MAUTOSTRIP_VOID);
	Clear();
}

void CClientData_Trail::Clear()
{
	MAUTOSTRIP(CClientData_Trail_Clear, MAUTOSTRIP_VOID);
	m_LastGameTick = -1;
	m_nHistory = 0;
	m_iCurPos = -1;
}

bool CClientData_Trail::NeedRefresh(class CXR_Model* _pModel, void* _pContext0, void* _pContext1)
{
	MAUTOSTRIP(CClientData_Trail_NeedRefresh, false);
	CWObject_CoreData *pObj = safe_cast<CWObject_CoreData >((CReferenceCount *)_pContext0);
	if(!pObj)
		return false;

	CWorld_Client *pWClient = safe_cast<CWorld_Client >((CReferenceCount *)_pContext1);
	if(!pWClient)
		return false;

	return pWClient->ClientMessage_SendToObject(CWObject_Message(OBJMSG_CHAR_GETEVOLVETRAIL), pObj->m_iObject) != 0;
}

void CClientData_Trail::OnRefresh(void* _pContext0, void* _pContext1, int _GameTick, const CMat43fp32 *_pMat, int _nMat, int _Flags)
{
	MAUTOSTRIP(CClientData_Trail_OnRefresh, MAUTOSTRIP_VOID);
	CWObject_CoreData *pObj = safe_cast<CWObject_CoreData >((CReferenceCount *)_pContext0);
	if(!pObj)
		return;

	if(_nMat == 0)
		return;
	
	if(_GameTick > m_LastGameTick + 3)
		Clear();

	if(m_nHistory == 0)
	{
		m_BoundBox.m_Min = CVec3Dfp32::GetMatrixRow(_pMat[0], 3);
		m_BoundBox.m_Max = CVec3Dfp32::GetMatrixRow(_pMat[0], 3);
	}
	else
	{
		m_BoundBox.m_Min[0] = Min(m_BoundBox.m_Min[0], _pMat[0].k[3][0]);
		m_BoundBox.m_Min[1] = Min(m_BoundBox.m_Min[1], _pMat[0].k[3][1]);
		m_BoundBox.m_Min[2] = Min(m_BoundBox.m_Min[2], _pMat[0].k[3][2]);
		m_BoundBox.m_Max[0] = Max(m_BoundBox.m_Max[0], _pMat[0].k[3][0]);
		m_BoundBox.m_Max[1] = Max(m_BoundBox.m_Max[1], _pMat[0].k[3][1]);
		m_BoundBox.m_Max[2] = Max(m_BoundBox.m_Max[2], _pMat[0].k[3][2]);
	}
	
	m_iCurPos++;
	if(m_iCurPos == MAXLEN)
		m_iCurPos = 0;
	
	if(m_nHistory < MAXLEN)
		m_nHistory++;
	
	m_History[m_iCurPos] = _pMat[0];
	m_Flags[m_iCurPos] = _Flags;
	
	CMat43fp32 IWMat;
	pObj->GetPositionMatrix().InverseOrthogonal(IWMat);
	m_LastIWMat = IWMat;
	m_LastGameTick = _GameTick;
}

void CClientData_Trail::AddPosition(int _GameTick, const CMat43fp32 &_WMat, const CMat43fp32 &_Pos, int _Flags)
{
	MAUTOSTRIP(CClientData_Trail_AddPosition, MAUTOSTRIP_VOID);
	if(_GameTick > m_LastGameTick + 3)
		Clear();
	
	if(m_nHistory == 0)
	{
		m_BoundBox.m_Min = CVec3Dfp32::GetMatrixRow(_Pos, 3);
		m_BoundBox.m_Max = CVec3Dfp32::GetMatrixRow(_Pos, 3);
	}
	else
	{
		m_BoundBox.m_Min[0] = Min(m_BoundBox.m_Min[0], _Pos.k[3][0]);
		m_BoundBox.m_Min[1] = Min(m_BoundBox.m_Min[1], _Pos.k[3][1]);
		m_BoundBox.m_Min[2] = Min(m_BoundBox.m_Min[2], _Pos.k[3][2]);
		m_BoundBox.m_Max[0] = Max(m_BoundBox.m_Max[0], _Pos.k[3][0]);
		m_BoundBox.m_Max[1] = Max(m_BoundBox.m_Max[1], _Pos.k[3][1]);
		m_BoundBox.m_Max[2] = Max(m_BoundBox.m_Max[2], _Pos.k[3][2]);
	}
	
	m_iCurPos++;
	if(m_iCurPos == MAXLEN)
		m_iCurPos = 0;

/*	if(m_iCurPos < 0 || m_iCurPos >= MAXLEN)
		_asm int 3;*/
	
	if(m_nHistory < MAXLEN)
		m_nHistory++;
	
	m_History[m_iCurPos] = _Pos;
	m_Flags[m_iCurPos] = _Flags;
	
	CMat43fp32 IWMat;
	_WMat.InverseOrthogonal(IWMat);
	m_LastIWMat = IWMat;
	m_LastGameTick = _GameTick;
}
/*
void CClientData_Trail::OnRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MAUTOSTRIP(CClientData_Trail_OnRefresh_2, MAUTOSTRIP_VOID);
	if(_pWClient->GetClientMode() != WCLIENT_MODE_MIRROR)
	{
		if(!_pObj->m_lspClientObj[0])
		{
			TPtr<CClientData_Trail> spTrail = DNew(CClientData_Trail) CClientData_Trail;
			//				spTrail->AddPosition(_pWClient->GetGameTick(), _pObj->GetPosition());
			_pObj->m_lspClientObj[0] = spTrail;
		}
		
		CClientData_Trail *pTrail = (CClientData_Trail *)(CReferenceCount *)_pObj->m_lspClientObj[0];
		if(!pTrail)
			return;
		
		CMat4Dfp32 Mat = _pObj->GetPositionMatrix();
		
		//Fix: We don't know this is a projectile
		if(_pObj->m_ClientFlags & CWObject_Projectile::CLIENTFLAGS_RENDERHANSOFFSET)
		{
			CVec3Dfp32 RelPos = CWObject_Projectile::GetHandsDistance(_pObj, _pObj->AnimTime(_pWClient) * SERVER_TIMEPERFRAME + _pWClient->GetInterpolateTime());
			RelPos.MultiplyMatrix3x3(Mat);
			CVec3Dfp32::GetMatrixRow(Mat, 3) += RelPos;
		}
		
		pTrail->AddPosition(_pWClient->GetGameTick(), Mat, Mat);
	}
}
*/
// -------------------------------------------------------------------
//  CXR_Model_Smoke
// -------------------------------------------------------------------
void CXR_Model_Smoke::OnCreate(const char *)
{
	MAUTOSTRIP(CXR_Model_Smoke_OnCreate, MAUTOSTRIP_VOID);
	m_iSmokeTexture = GetTextureID("SMOKE01");
	m_BoundRadius = 256;
}

bool CXR_Model_Smoke::RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
{
	MAUTOSTRIP(CXR_Model_Smoke_RenderVB, false);
	int nMaxParticles = _pAnimState->m_Anim0 ? (_pAnimState->m_Anim0 >> 8) : 30;
	int nMinParticles = _pAnimState->m_Anim0 ? (_pAnimState->m_Anim0 & 255): 10;
	
	const float Size = _pAnimState->m_Anim1 ? (_pAnimState->m_Anim1 >> 8) : 40;
	const float Dist = _pAnimState->m_Anim1 ? (_pAnimState->m_Anim1 & 255) : 20;
	
	CPixel32 Color = _pAnimState->m_Colors[0];
	const float Height = _pAnimState->m_Colors[1] != -1 ? (_pAnimState->m_Colors[1] >> 16) : 64;
	const float Pressure = _pAnimState->m_Colors[1] != -1 ? (_pAnimState->m_Colors[1] & 65535): 20;
	
	const float SplineDist = _pAnimState->m_Colors[2] != -1 ? (_pAnimState->m_Colors[2] >> 16) : 0;
	const float SplineFreq = _pAnimState->m_Colors[2] != -1 ? (_pAnimState->m_Colors[2] & 65535): 4;
	const float Speed = _pAnimState->m_Colors[3] != -1 ? float(_pAnimState->m_Colors[3]) / 256 : 64.0f / 256;
	
	CMat43fp32 L2V;
	_WMat.Multiply(_VMat, L2V);
	
	_pVB->m_Priority = m_RenderInfo.m_BasePriority_Transparent;
	_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	_pVB->m_pAttrib->Attrib_TextureID(0, m_iSmokeTexture);
	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	if (m_pEngine)
		m_pEngine->GetFogState()->SetDepthFog(_pVB->m_pAttrib);
	
	const float Time = _pAnimState->m_AnimTime0 + 100;
	
	fp32 Spline[4];
	SetSplineWeights(Time - TruncToInt(Time), Spline);
	
	int iRand = _pAnimState->m_Anim0;
	int iRand0 = iRand + 200;
	int iRand1 = iRand + 400;
	
	float Lod = 4096.0f / L2V.k[3][2];
	int nParticles = Lod;
	float AlphaLast = Lod - nParticles;
	if(nParticles > nMaxParticles)
	{
		nParticles = nMaxParticles;
		AlphaLast = 1.0f;
	}
	
	if(nParticles <= nMinParticles)
	{
		nParticles = nMinParticles;
		AlphaLast = 1.0f;
	}

	if(nParticles > 128)
		nParticles = 128;
	
	CMat43fp32 IWMat;
	_WMat.InverseOrthogonal(IWMat);
	CVec3Dfp32 up = CVec3Dfp32::GetMatrixRow(IWMat, 2);
	CVec3Dfp32 fwd = CVec3Dfp32::GetMatrixRow(_WMat, 0);

	fp32 AlphaScale = GetFogFade(_WMat);
	
	CXR_Particle2 lParticles[128];
	for(int p = 0; p < nParticles; p++)
	{
		float t = (MFloat_GetRand(iRand++) + 1) * Speed * Time;
		float Frac = t - TruncToInt(t);
		
		lParticles[p].m_Pos = up * Sqr(Frac) * Height;
		lParticles[p].m_Pos[0] += (1.0f - Sqr(1.0f - Frac)) * Pressure;
		lParticles[p].m_Angle = Sqr(Frac) * (MFloat_GetRand(iRand++) - 0.5f);
		
		float sTime = Time + Frac * SplineFreq;
		//			lParticles[p].m_Pos[0] = (GetRandInterpolated(iRand0, Time + Frac * 4) - 0.5f) * 20 * Frac;
		//			lParticles[p].m_Pos[1] = (GetRandInterpolated(iRand1, Time + Frac * 4) - 0.5f) * 20 * Frac;
		if(SplineDist > 0)
		{
			SetSplineWeights(sTime - TruncToInt(sTime), Spline);
			lParticles[p].m_Pos[0] = (GetRandSplineInterpolated(Spline, iRand0, sTime) - 0.5f) * SplineDist * Frac;
			lParticles[p].m_Pos[1] = (GetRandSplineInterpolated(Spline, iRand1, sTime) - 0.5f) * SplineDist * Frac;
		}
		lParticles[p].m_Pos[0] += (MFloat_GetRand(iRand++) - 0.5f) * Dist * Frac;
		lParticles[p].m_Pos[1] += (MFloat_GetRand(iRand++) - 0.5f) * Dist * Frac;
		
		float f = 1.0f - Frac;
		float Alpha = 6.5f * f * f - 6.5f * f * f * f;
		if(p == nParticles - 1)
			Alpha *= AlphaLast;
		Alpha *= AlphaScale;
		lParticles[p].m_Color = CPixel32(Color.GetR(), Color.GetG(), Color.GetB(), Alpha * Color.GetA());
		lParticles[p].m_Size = Size * (1.0f + 2.0f * Frac);
	}
	return CXR_Util::Render_Particles(m_pVBM, _pVB, L2V, lParticles, nParticles, NULL, CXR_PARTICLETYPE_ANGLE);
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Smoke, CXR_Model_Custom);


// -------------------------------------------------------------------
//  CXR_Model_Flames
// -------------------------------------------------------------------

void CXR_Model_Flames::OnCreate(const char *)
{
	MAUTOSTRIP(CXR_Model_Flames_OnCreate, MAUTOSTRIP_VOID);
	m_BoundRadius = 128;
	
	m_iTexture = GetTextureID("INFERNO0");
}

void CXR_Model_Flames::CreateFlame(CVec3Dfp32 _Pos, const CVec3Dfp32 &_Dir, fp32 _Force, int &_iRand, CXR_BeamStrip *pStrip, int &_iBeam)
{
	MAUTOSTRIP(CXR_Model_Flames_CreateFlame, MAUTOSTRIP_VOID);
	fp32 Width = m_Width;
	float TOfs = -m_Time * 2 + MFloat_GetRand(_iRand++) + MFloat_GetRand(_iRand++) * m_Time * 0.2f;
	for(int b = 0; b < 3; b++)
	{
		pStrip[_iBeam].m_Flags = (_iBeam == 0) ? CXR_BEAMFLAGS_BEGINCHAIN : 0;
		pStrip[_iBeam].m_Pos = _Pos;
		pStrip[_iBeam].m_Width = Width;
		pStrip[_iBeam].m_TextureYOfs = TOfs;
		if(b == 1)
			pStrip[_iBeam].m_Color = m_Color[b];
		else
			pStrip[_iBeam].m_Color = m_TransColor[b];

		Width -= m_Width * 0.31f;
		if(_Force > 0)
			_Pos += CVec3Dfp32((m_Width * 0.5f) * (GetRandSplineInterpolated(m_Spline, _iRand++, m_WaveTime) - 0.5f),
				(m_Width * 0.5f) * (GetRandSplineInterpolated(m_Spline, _iRand++, m_WaveTime) - 0.5f),
				m_Height * (GetRandSplineInterpolated(m_Spline, _iRand++, m_WaveTime) - 0.5f) + m_Height) + _Dir * _Force * b * b;
		else
			_Pos += CVec3Dfp32((m_Width * 0.5f) * (GetRandSplineInterpolated(m_Spline, _iRand++, m_WaveTime) - 0.5f),
				(m_Width * 0.5f) * (GetRandSplineInterpolated(m_Spline, _iRand++, m_WaveTime) - 0.5f),
				m_Height * (GetRandSplineInterpolated(m_Spline, _iRand++, m_WaveTime) - 0.5f) + m_Height);
		_iBeam++;
	}
}

bool CXR_Model_Flames::RenderFlames(CXR_VertexBuffer *_pVB, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat, CXR_BeamStrip *pStrip, int _iBeam)
{
	MAUTOSTRIP(CXR_Model_Flames_RenderFlames, false);
	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	_pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
	return CXR_Util::Render_BeamStrip(m_pVBM, _pVB, _WMat, _VMat, pStrip, _iBeam, CXR_BEAMFLAGS_EDGEFADE);
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Flames, CXR_Model_Custom);

// -------------------------------------------------------------------
//  CXR_Model_Fire
// -------------------------------------------------------------------
void CXR_Model_Fire::OnCreate(const char *_pParams)
{
	MAUTOSTRIP(CXR_Model_Fire_OnCreate, MAUTOSTRIP_VOID);
	SetFadeTime(0.5f);
	
	CXR_Model_Flames::OnCreate(_pParams);
}

void CXR_Model_Fire::OnPreRender(const CXR_AnimState *_pAnimState, const CMat43fp32 &_WMat)
{
	MAUTOSTRIP(CXR_Model_Fire_OnPreRender, MAUTOSTRIP_VOID);
	if(_pAnimState->m_Colors[3] != -1 && (_pAnimState->m_Colors[3] & 0x00010000))
		AddDistortLight(_pAnimState, CVec3Dfp32::GetMatrixRow(_WMat, 3),
		CVec3Dfp32(1.0f, 0.6f, 0), CVec3Dfp32(0.2f, 0.2f, 0), 128, 0);
}

bool CXR_Model_Fire::RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
{
	MAUTOSTRIP(CXR_Model_Fire_RenderVB, false);
//	CWireContainer* pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));
//	if (pWC != NULL)	
//		pWC->RenderMatrix(_WMat, 20);
	
	int nFlames = _pAnimState->m_AnimAttr0;
	CVec3Dfp32 SpreadX(uint8(_pAnimState->m_Anim0 & 255), 0, 0); SpreadX.MultiplyMatrix3x3(_WMat);
	CVec3Dfp32 SpreadY(0, uint8(_pAnimState->m_Anim0 >> 8), 0); SpreadY.MultiplyMatrix3x3(_WMat);
	CVec3Dfp32 Center = CVec3Dfp32::GetMatrixRow(_WMat, 3);
	int iRand = _pAnimState->m_AnimAttr1;
	
	fp32 Fade = GetFade(_pAnimState) * GetFogFade(_WMat);
	if(Fade <= 0)
		return false;
	
	for(int i = 0; i < 3; i++)
	{
		uint32 Col = _pAnimState->m_Colors[i];
		m_Color[i] = (Col & 0xffffff) | (TruncToInt(Col * Fade) & 0xff000000);
		m_TransColor[i] = m_Color[i] & 0xffffff;
	}
	m_Width = uint8(_pAnimState->m_Anim1 >> 8);
	
	fp32 HeightHigh = fp32(uint8(_pAnimState->m_Anim1 & 255));
	fp32 HeightLow = HeightHigh;

	if(_pAnimState->m_Colors[3] != -1)
	{
		HeightLow = _pAnimState->m_Colors[3] & 255;
		HeightHigh = (_pAnimState->m_Colors[3] >> 8) & 255;
	}
	
	if(nFlames == 0)
		nFlames = 15;
	if(m_Height == 0)
		m_Height = 16;
	if(m_Width == 0)
		m_Width = 8;
	
	m_Time = _pAnimState->m_AnimTime0;
	//		m_Time = 0;
	m_WaveTime = m_Time * 1.2f;
	//		m_WaveTime = fp32(TruncToInt(_pAnimState->m_AnimTime0 * 0.2f * 50)) / 50;
	
	//		SetSplineWeights(GetFrac_Forcedfp32(m_WaveTime), m_Spline);
	SetSplineWeights(m_WaveTime - TruncToInt(m_WaveTime), m_Spline);
	//		SetSplineWeights(m_WaveTime - iWaveTime, m_Spline);
	//		SetSplineWeights(m_WaveTime - int(m_WaveTime), m_Spline);
	//		volatile fp32 Time = m_WaveTime;
	//		SetSplineWeights(Time - int(Time), m_Spline);
	
	if(nFlames * 3 > CXR_Util::MAXBEAMS)
		return false;
	CXR_BeamStrip lBeams[CXR_Util::MAXBEAMS];

	int iBeam = 0;
	for(int f = 0; f < nFlames; f++)
	{
		float x = (MFloat_GetRand(iRand++) - 0.5f);
		float y = (MFloat_GetRand(iRand++) - 0.5f);
		float r = M_Sqrt(Sqr(x * 2.0f) + Sqr(y * 2.0f));
		m_Height = LERP(HeightLow, HeightHigh, (1.0f - r));
		if (m_Height >= HeightLow)
			CreateFlame(Center + SpreadX * x + SpreadY * y, 0, 0, iRand, lBeams, iBeam);
	}

	CMat43fp32 Unit; Unit.Unit();
	return RenderFlames(_pVB, Unit, _VMat, lBeams, iBeam);
}


MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Fire, CXR_Model_Flames);

// -------------------------------------------------------------------
//  CXR_Model_FireTorch
// -------------------------------------------------------------------
class CXR_Model_FireTorch : public CXR_Model_Fire
{
	MRTC_DECLARE;

protected:
	int m_iAttach0;
	fp32 m_Range;
	fp32 m_Intensity;

	virtual void OnCreate(const char *_pParam)
	{
		CXR_Model_Fire::OnCreate(_pParam);

		if(!_pParam)
		{
			m_iAttach0 = 1;
			m_Range = 256;
		}
		else
		{
			CStr Param = _pParam;
			m_Range = Param.GetStrSep(",").Val_int();
			m_iAttach0 = Param.GetStrSep(",").Val_int();
			m_Intensity = Param.GetStrSep(",").Val_fp64();
			if(m_Intensity <= 0)
				m_Intensity = 1;
		}
	}

	virtual void OnPreRender(const CXR_AnimState *_pAnimState, const CMat43fp32 &_WMat)
	{
		CVec3Dfp32 V = CVec3Dfp32::GetRow(_WMat, 3);
		fp32 Tick = _pAnimState->m_AnimTime0 * 15;
		int iTick = TruncToInt(Tick);
		fp32 Frac = Tick - iTick;

		int iRand0 = iTick * 4;
		int iRand1 = (iTick - 1) * 4;
		CVec3Dfp32 Ofs0(MFloat_GetRand(iRand0++) * 12 - 6, MFloat_GetRand(iRand0++) * 12 - 6, MFloat_GetRand(iRand0++) * 8 + 1);
		fp32 Range0 = Min(m_Range, Tick * 32) + MFloat_GetRand(iRand0++) * m_Range / 4;
		CVec3Dfp32 Ofs1(MFloat_GetRand(iRand1++) * 12 - 6, MFloat_GetRand(iRand1++) * 12 - 6, MFloat_GetRand(iRand1++) * 8 + 1);
		fp32 Range1 = Min(m_Range, Tick * 32) + MFloat_GetRand(iRand1++) * m_Range / 4;

		CVec3Dfp32 Pos = CVec3Dfp32::GetRow(_WMat, 3);
		Pos += CVec3Dfp32::GetRow(_WMat, 2) * 32.0f;

		if(_pAnimState->m_pContext)
			CalcPositionsFromSkeleton((CXR_Skeleton *)_pAnimState->m_pContext, _WMat, Pos, m_iAttach0);

//		Pos += Ofs1 + (Ofs0 - Ofs1) * Frac;
		fp32 Range = Range1 + (Range0 - Range1) * Frac;

//		m_pEngine->Render_Light_AddDynamic(Pos, CVec3Dfp32(0.4f, 0.35f, 0.2f), Range, 0, 0, 1);
		m_pEngine->Render_Light_AddDynamic(Pos, CVec3Dfp32(0.9f, 0.7f, 0.4f) * m_Intensity , Range*2.0f, 0, 0, 1);
	}
	
	virtual bool RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		if(!_pAnimState->m_pContext)
			return false;

		CXR_AnimState AS = *_pAnimState;
		AS.m_Anim1 = 773;
		AS.m_Colors[0] = 0xffff5300;
		AS.m_Colors[1] = 0xffffb871;

		CMat43fp32 Pos;
		Pos.Unit();
		
		CVec3Dfp32 V;
		CalcPositionsFromSkeleton((CXR_Skeleton *)_pAnimState->m_pContext, _WMat, V, m_iAttach0);
		V.SetRow(Pos, 3);

		return CXR_Model_Fire::RenderVB(_pVB, &AS, Pos, _VMat);
	}
};

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_FireTorch, CXR_Model_Fire);

// -------------------------------------------------------------------
//  CXR_Model_TraceLight2
// -------------------------------------------------------------------
void CXR_Model_TraceLight2::OnCreate(const char *)
{
	MAUTOSTRIP(CXR_Model_TraceLight2_OnCreate, MAUTOSTRIP_VOID);
	SetFadeTime(0.15f);
	
	m_iTexture = GetTextureID("B_TRACELIGHT01");
	m_BoundRadius = 512;
}

void CXR_Model_TraceLight2::OnPreRender(const CXR_AnimState *_pAnimState, const CMat43fp32 &_WMat)
{
	MAUTOSTRIP(CXR_Model_TraceLight2_OnPreRender, MAUTOSTRIP_VOID);
	//		AddDistortLight(_pAnimState, CVec3Dfp32::GetMatrixRow(_WMat, 3),
	//					    CVec3Dfp32(0.8f, 0.8f, 0.8f), CVec3Dfp32(0.2f, 0.2f, 0.2f), 128, 0);
}

void CXR_Model_TraceLight2::RenderRings(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
{
	MAUTOSTRIP(CXR_Model_TraceLight2_RenderRings, MAUTOSTRIP_VOID);
	fp32 Fade = GetFade(_pAnimState, 0.5f) * (float((uint32)_pAnimState->m_Colors[0]) / (65536.0f * 65536));
	if(Fade <= 0)
		return;
	
	const fp32 Length = _pAnimState->m_Anim0;
	//		const float SegLength = 128;
	const fp32 SegLength = _pAnimState->m_Anim0;
	const int nBeams = 1;
	
	for(int b = 0; b  < nBeams; b++)
	{
		int iBaseRand = _pAnimState->m_Anim0 + b * 1000;
		
		CXR_VertexBuffer *pVB = AllocVB();
		if(!pVB)
			return;
		
		const fp32 Time = _pAnimState->m_AnimTime0;
		const fp32 Time05 = _pAnimState->m_AnimTime0 * 0.5f;
		
		CXR_BeamStrip Beams[CXR_Util::MAXBEAMS];
		int iBeam = 0;
		
		//			float Radius = Time * 3 + 3 + b;
		const fp32 Radius = 0;
		
		//			int nSegments = Length / SegLength;
		const int nSegments = 2;
		fp32 Distance = 0;
		for(int s = 0; s < nSegments; s++)
		{
			const fp32 FadeTime = Tooth(Max(Min(float((Time - s * 0.002f) * 3), 1.0f), 0.0f));
			//				if(FadeTime <= 0)
			//					continue;
			
			uint32 Color = 0x007f3f3f | (TruncToInt(FadeTime * 192) << 24);
			
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
			
			const fp32 a = GetRandInterpolated(iBaseRand, float(s) / 5) * 2 * _PI;
			
			float qsin, qcos;
			QSinCos(a, qsin, qcos);
			
			Beams[iBeam].m_Pos = CVec3Dfp32(Distance, qsin * Radius, qcos * Radius);
			Beams[iBeam].m_Color = Color;
			Beams[iBeam].m_Width = 0.5f;
			Beams[iBeam].m_TextureYOfs = 0;
			iBeam++;
			Distance += SegLength;
		}
		//			Beams[0].m_Color &= 0xffffff;
		//			Beams[iBeam - 1].m_Color &= 0xffffff;
		
		pVB->Geometry_Color(0xffffffff);
		pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
		pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
		
		if(CXR_Util::Render_BeamStrip(m_pVBM, pVB, _WMat, _VMat, Beams, iBeam))
			m_pVBM->AddVB(pVB);
	}
}

void CXR_Model_TraceLight2::Render(const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, const CMat43fp32& _VMat)
{
	MAUTOSTRIP(CXR_Model_TraceLight2_Render, MAUTOSTRIP_VOID);
	{
		RenderRings(_pAnimState, _WMat, _VMat);
	}
	
	/*		fp32 Fade = GetFadeDown(_pAnimState) * (float((uint32)_pAnimState->m_Colors[0]) / (65536.0f * 65536), 0.5f);
	if(Fade <= 0)
	return;
	
	  const float Length = _pAnimState->m_Anim0;
	  const float Radius = 20;
	  
		uint32 Color = 0xff000000;
		Color = (Color & 0xffffff) | (int(Color * Fade) & 0xff000000);
		
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

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_TraceLight2, CXR_Model_Custom);

// -------------------------------------------------------------------
//  CXR_Model_BoltTrail
// -------------------------------------------------------------------
void CXR_Model_BoltTrail::OnCreate(const char *)
{
	MAUTOSTRIP(CXR_Model_BoltTrail_OnCreate, MAUTOSTRIP_VOID);
	SetFadeTime(0.15f);
	
//	m_iTexture = GetTextureID("B_TRAIL03"); // Removed by Mondelore.
	m_iSurface = GetSurfaceID("BoltTrail04"); // Added by Mondelore.
	m_BoundRadius = 512;
}

fp32 CXR_Model_BoltTrail::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_BoltTrail_GetBound_Sphere, 0.0f);
	CBox3Dfp32 Box;
	GetBound_Box(Box, _pAnimState);
	
	CVec3Dfp32 vMax;
	for(int i = 0; i < 3; i++)
		vMax[i] = Max(M_Fabs(Box.m_Max[i]), M_Fabs(Box.m_Min[i]));
	
	//Fix: Remove m_BoundRadius
	m_BoundRadius = vMax.Length();
	
	return m_BoundRadius;
}

void CXR_Model_BoltTrail::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_BoltTrail_GetBound_Box, MAUTOSTRIP_VOID);
	_Box.m_Max = CVec3Dfp32(1, 1, 1);
	if(_pAnimState)
		_Box.m_Min = CVec3Dfp32(-1 - _pAnimState->m_Anim0, -1, -1);
	else
		_Box.m_Min = CVec3Dfp32(-1, -1, -1);
}

void CXR_Model_BoltTrail::Render(const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, const CMat43fp32& _VMat)
{
	MAUTOSTRIP(CXR_Model_BoltTrail_Render, MAUTOSTRIP_VOID);
	// Added by Mondelore.
	CXW_Surface *pSurface = GetSurfaceContext()->GetSurfaceVersion(m_iSurface, m_pEngine);
	CXW_SurfaceKeyFrame *pKeyFrame = pSurface->GetFrame(0, _pAnimState->m_AnimTime0, *GetSurfaceContext()->m_spTempKeyFrame);

	fp32 Fade = GetFade(_pAnimState) * GetFogFade(_WMat);

	// Added by Mondelore.
//	uint32 Color = 0x0080a0ff; // Removed by Mondelore.
	uint32 Color = pKeyFrame->m_Medium.m_Color & 0x00FFFFFF;
	Fade *= (fp32)((pKeyFrame->m_Medium.m_Color >> 24) & 0xFF) / 255.0f;

	if (Fade <= 0)
		return;
	
	const float SegLen = 32;
	const float TimePerSegment = 0.04f;
	const float Duration = 1;
	
	const float TotLen = _pAnimState->m_Anim0;
	if(TotLen <= 1)
		return;
	
	CXR_VertexBuffer *pVB = AllocVB();
	if(!pVB)
		return;
	
	CXR_BeamStrip Beams[CXR_Util::MAXBEAMS];
	
	int iBeam = 0;
	
	fp32 Time = _pAnimState->m_AnimTime0 - int(TotLen/SegLen) * TimePerSegment;
	
	for(fp32 Len = 0; Len < TotLen; Len += SegLen)
	{
		if(Time > _pAnimState->m_AnimTime1)
			break;
		Beams[iBeam].m_Pos = CVec3Dfp32(-1, 0, 0) * Len;
		Beams[iBeam].m_Color = Color | (TruncToInt((1.0f - Min(MaxMT(Time, 0), Duration) / Duration) * 192 * Fade) << 24);
		Time += TimePerSegment;
		Beams[iBeam].m_Width = 6;
		Beams[iBeam].m_TextureYOfs = 0;
		iBeam++;
	}
	
	if(Time <= _pAnimState->m_AnimTime1)
	{
		Beams[iBeam].m_Pos = CVec3Dfp32(-1, 0, 0) * TotLen;
		Beams[iBeam].m_Color = Color/* | (TruncToInt((1.0f - Min(MaxMT(Time, 0), Duration) / Duration) * 192 * Fade) << 24)*/;
		Beams[iBeam].m_Width = 6;
		Beams[iBeam].m_TextureYOfs = 0;
		iBeam++;
	}
	
	if(iBeam < 3)
		return;

/* Removed by Mondelore. (made it a surface instead)
	pVB->Geometry_Color(0xffffffff);
	pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
*/

	if(CXR_Util::Render_BeamStrip(m_pVBM, pVB, _WMat, _VMat, Beams, iBeam))
	{
		// Added by Mondelore.
		int Flags = RENDERSURFACE_DEPTHFOG;// | RENDERSURFACE_VERTEXFOG;
		if(pVB->m_pTransform[0])
			Flags |= RENDERSURFACE_MATRIXSTATIC_M2V;

		CXR_Util::Render_Surface(Flags, pSurface, pKeyFrame, m_pEngine, m_pVBM, NULL, NULL, pVB->m_pTransform[0], pVB, pVB->m_Priority, 0.0001f);

//		Render_Surface(m_iSurface, pVB, _pAnimState->m_AnimTime0); // Added by Mondelore.
//		m_pVBM->AddVB(pVB); // Removed by Mondelore.
	}

}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_BoltTrail, CXR_Model_Custom);






// -------------------------------------------------------------------
//  CXR_Model_Trail
// -------------------------------------------------------------------
void CXR_Model_Trail::OnCreate(const char *)
{
	MAUTOSTRIP(CXR_Model_Trail_OnCreate, MAUTOSTRIP_VOID);
	m_iTexture = GetTextureID("B_TRAIL03");
	m_BoundRadius = 512;
}

fp32 CXR_Model_Trail::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_Trail_GetBound_Sphere, 0.0f);
	CBox3Dfp32 Box;
	GetBound_Box(Box, _pAnimState);
	
	CVec3Dfp32 vMax;
	for(int i = 0; i < 3; i++)
		vMax[i] = Max(M_Fabs(Box.m_Max[i]), M_Fabs(Box.m_Min[i]));
	
	//Fix: Remove m_BoundRadius
	m_BoundRadius = vMax.Length();
	
	return m_BoundRadius;
}

CBox3Dfp32 CXR_Model_Trail::GetAppliedBound(const CBox3Dfp32 &_Bound, const CMat43fp32 &_Mat)
{
	CBox3Dfp32 Bound;
	MAUTOSTRIP(CXR_Model_Trail_GetAppliedBound, Bound);
	
	CVec3Dfp32 c[8];
	int i;
	for(i = 0; i < 8; i++)
	{
		c[i] = CVec3Dfp32((((i >> 0) & 1) ? _Bound.m_Min[0] : _Bound.m_Max[0]),
			(((i >> 1) & 1) ? _Bound.m_Min[1] : _Bound.m_Max[1]), 
			(((i >> 2) & 1) ? _Bound.m_Min[2] : _Bound.m_Max[2]));
		c[i] *= _Mat;
	}
	Bound.m_Min = c[0];
	Bound.m_Max = c[0];
	for(i = 1; i < 8; i++)
	{
		Bound.m_Min = CVec3Dfp32(Min(Bound.m_Min[0], c[i][0]), 
			Min(Bound.m_Min[1], c[i][1]), 
			Min(Bound.m_Min[2], c[i][2]));
		Bound.m_Max = CVec3Dfp32(Max(Bound.m_Max[0], c[i][0]), 
			Max(Bound.m_Max[1], c[i][1]), 
			Max(Bound.m_Max[2], c[i][2]));
	}
	
	return Bound;
}

void CXR_Model_Trail::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_Trail_GetBound_Box, MAUTOSTRIP_VOID);
	_Box.m_Max = 0;
	_Box.m_Min = 0;
	
	if(_pAnimState)
	{
		if(!_pAnimState->m_pspClientData)
			return;
		
		CClientData_Trail *pTrail = (CClientData_Trail *)(CReferenceCount *)*_pAnimState->m_pspClientData;
		if(!pTrail)
			return;
		
		_Box = GetAppliedBound(pTrail->m_BoundBox, pTrail->m_LastIWMat);
		
	}
}

void CXR_Model_Trail::Render(const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, const CMat43fp32& _VMat)
{
	MAUTOSTRIP(CXR_Model_Trail_Render, MAUTOSTRIP_VOID);
	if(!_pAnimState->m_pspClientData)
		return;
	
	CClientData_Trail *pTrail = (CClientData_Trail *)(CReferenceCount *)*_pAnimState->m_pspClientData;
	if(!pTrail)
		return;
	
	if(pTrail->m_nHistory < 2)
		return;
	
	CXR_BeamStrip Beams[CXR_Util::MAXBEAMS];
	
	int Color = 0x0080a0ff;
	const fp32 Tick = _pAnimState->m_AnimTime0 * SERVER_TICKSPERSECOND;
	const fp32 IPTime = Tick - int(Tick);
	
	int iBeam = 0;
	{
		//Add current position to trail
		Beams[iBeam].m_Pos = CVec3Dfp32::GetMatrixRow(_WMat, 3);
		Beams[iBeam].m_Color = 0xff000000 | Color;
		Beams[iBeam].m_Width = 6;
		Beams[iBeam].m_TextureYOfs = 0;
		iBeam++;
	}
	
	int iIndex = pTrail->m_iCurPos;
	
	for(int i = 0; i < pTrail->m_nHistory; i++)
	{
		CXR_BeamStrip& Beam = Beams[iBeam];
		Beam.m_Pos = CVec3Dfp32::GetMatrixRow(pTrail->m_History[iIndex], 3);

		iIndex--;
		if(iIndex < 0)
			iIndex += CClientData_Trail::MAXLEN;
		
		if(iBeam > 0 && Beams[iBeam - 1].m_Pos == Beam.m_Pos)
			continue;
		
		const fp32 TimeFade = (fp32(CClientData_Trail::MAXLEN - i) - IPTime) / CClientData_Trail::MAXLEN;
		Beam.m_Color = Color | (int(255.0f * TimeFade) << 24);
		
		Beam.m_Width = 6;
		Beam.m_TextureYOfs = 0;
		iBeam++;
	}
	
	CXR_VertexBuffer *pVB = AllocVB();
	if(!pVB)
		return;
	
	pVB->Geometry_Color(0xffffffff);
	pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	
	CMat43fp32 Unit;
	Unit.Unit();
	if(CXR_Util::Render_BeamStrip(m_pVBM, pVB, Unit, _VMat, Beams, iBeam))
		m_pVBM->AddVB(pVB);
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Trail, CXR_Model_Custom);

// -------------------------------------------------------------------
//  CXR_Model_DoubleTrail
// -------------------------------------------------------------------
void CXR_Model_DoubleTrail::OnCreate(const char *_pParam)
{
	MAUTOSTRIP(CXR_Model_DoubleTrail_OnCreate, MAUTOSTRIP_VOID);
	m_BoundRadius = 96;

	if(!_pParam)
	{
		m_iAttach0 = 2;
		m_iAttach1 = 3;
	}
	else
	{
		CStr Param = _pParam;

		CStr Surface = Param.GetStrSep(",");
		m_iSurface = GetSurfaceID(Surface);
		
		m_iAttach0 = Param.GetStrSep(",").Val_int();
		m_iAttach1 = Param.GetStrSep(",").Val_int();
	}
}

TPtr<CXR_ModelInstance> CXR_Model_DoubleTrail::CreateModelInstance()
{
	MAUTOSTRIP(CXR_Model_DoubleTrail_CreateModelInstance, NULL);
	return DNew(CClientData_Trail) CClientData_Trail;
}

void CXR_Model_DoubleTrail::VSpline(CVec3Dfp32 *pMoveA0, CVec3Dfp32 *pMoveA1, CVec3Dfp32 *pMoveA2,
									CVec3Dfp32 *pMoveB0, CVec3Dfp32 *pMoveB1, CVec3Dfp32 *pMoveB2, 
									CVec3Dfp32* _pDest, fp32 _tFrac, fp32 _tA0, fp32 _tA1, fp32 _tB0, fp32 _tB1, int _nV)
{
	const fp32 tSqr = Sqr(_tFrac);
	const fp32 tCube = tSqr * _tFrac;
	
	const fp32 k = 1.0f;
	const fp32 tsA0 = k * _tA1 / _tA0;
	const fp32 tsA1 = k * _tA1 / _tA1;
	const fp32 tsB0 = k * _tA1 / _tB0;
	const fp32 tsB1 = k * _tA1 / _tB1;
	
	for(int iV = 0; iV < _nV; iV++)
	{
		// dQuatA
		CVec3Dfp32 dMA = (pMoveA1[iV] - pMoveA0[iV]) * tsA0;
		dMA += (pMoveA2[iV] - pMoveA1[iV]) * tsA1;
		
		CVec3Dfp32 dMB = (pMoveB1[iV] - pMoveB0[iV]) * tsB0;
		dMB += (pMoveB2[iV] - pMoveB1[iV]) * tsB1;
		
		
		// Spline it
		for(int i = 0; i < 3; i++)
		{
			const fp32 v0 = dMA.k[i];
			const fp32 v1 = dMB.k[i];
			const fp32 p0 = pMoveA1[iV].k[i];
			const fp32 p1 = pMoveB1[iV].k[i];
			const fp32 D = p0;
			const fp32 C = v0;
			const fp32 B = 3.0f*(p1 - D) - (2.0f*v0) - v1;
			const fp32 A = -(2.0f * B + v0 - v1) / 3.0f;
			_pDest[iV].k[i] = A*tCube + B*tSqr + C*_tFrac + D;
			
		}
	}
}

void CXR_Model_DoubleTrail::Render(const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, const CMat43fp32& _VMat)
{
	MAUTOSTRIP(CXR_Model_DoubleTrail_Render, MAUTOSTRIP_VOID);
	const CClientData_Trail *pTrail = safe_cast<const CClientData_Trail >((const CXR_ModelInstance *)_pAnimState->m_spModelInstance);
	if(!pTrail)
		return;

	if(pTrail->m_nHistory < 1)
		return;

	if(!_pAnimState->m_pContext)
		return;

	const fp32 Tick = _pAnimState->m_AnimTime0 * SERVER_TICKSPERSECOND;
	const fp32 IPTime = Tick - int(Tick);

	const fp32 Age = Tick - fp32(pTrail->m_LastGameTick) - 1.0f;
	if(Age > fp32(pTrail->m_nHistory) || Age < -1.0f)
		return;

	int Color = 0x00ffffff;
	const fp32 MaxSpeed = 18.0f;
	const fp32 MinSpeed = 12.0f;
	const int SubDivide = 4;

	CVec3Dfp32 History0[CClientData_Trail::MAXLEN + 1];
	CVec3Dfp32 History1[CClientData_Trail::MAXLEN + 1];
	int Flags[CClientData_Trail::MAXLEN + 1];

	int iStart = 0;
	CalcPositionsFromSkeleton((CXR_Skeleton *)_pAnimState->m_pContext, _WMat, History0[0], History1[0], m_iAttach0, m_iAttach1);
	
	int iIndex = pTrail->m_iCurPos;
	Flags[0] = _pAnimState->m_Colors[3];//pTrail->m_Flags[iIndex];
	
/*	CRC_Particle lPartices[20];
	int nParticles = 0;
	lPartices[nParticles].m_Pos = History1[0];
	if(Flags[0])
		lPartices[nParticles++].m_Color = -1;
	else
		lPartices[nParticles++].m_Color = 0xffff0000;*/
	
	int i;
	for(i = 1; i <= pTrail->m_nHistory; i++)
	{
		CalcPositionsFromSkeleton((CXR_Skeleton *)_pAnimState->m_pContext, pTrail->m_History[iIndex], History0[i], History1[i], m_iAttach0, m_iAttach1);
		
		Flags[i] = pTrail->m_Flags[iIndex];
/*		{
			lPartices[nParticles].m_Pos = History1[i];
			if(Flags[i])
				lPartices[nParticles++].m_Color = -1;
			else
				lPartices[nParticles++].m_Color = 0xffff0000;
		}*/

		iIndex--;
		if(iIndex < 0)
			iIndex += CClientData_Trail::MAXLEN;
	}

/*	{
		CMat4Dfp32 L2V;
		_WMat.Multiply(_VMat, L2V);
		CXR_VertexBuffer *pVB = AllocVB();
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZCOMPARE);
		if(CXR_Util::Render_Particles(m_pVBM, pVB, _VMat, lPartices, nParticles, 2))
			m_pVBM->AddVB(pVB);
	}*/

	CXR_QuadStrip Quads[CXR_Util::MAXQUADS];
	int iQuad = 0;

	for(i = iStart; i <= pTrail->m_nHistory - 1; i++)
	{
//		if(iQuad > 0 && Quads[iQuad - 1].m_Pos1 == History1[iIndex])
//			continue;

		{
			int i0 = i - 1;
			if(i0 < iStart)
				i0 = iStart;
			int i1 = i + 0;
			int i2 = i + 1;
			int i3 = i + 2;
			if(i3 > pTrail->m_nHistory)
				i3 = i2;

			fp32 fAdd = 1.0f / SubDivide;
			if(i == iStart)
			{
				if(Age < -0.9999f)
					continue;
				fAdd = fAdd / (Age + 1.0f);
			}

			for(fp32 f = 0; f < 0.9999f; f += fAdd)
			{
				VSpline(&History0[i0], &History0[i1], &History0[i2],
								  &History0[i1], &History0[i2], &History0[i3],
								  &Quads[iQuad].m_Pos0, f, 1, 1, 1, 1, 1);
				VSpline(&History1[i0], &History1[i1], &History1[i2],
								  &History1[i1], &History1[i2], &History1[i3],
								  &Quads[iQuad].m_Pos1, f, 1, 1, 1, 1, 1);

				int Col = Color;
				switch(Flags[i2])
				{
				case 1: Col |= 0x60000000; break;
				case 2: Col |= 0xa0080000; break;
				case 3: Col |= 0xff0f0000; break;
				}
				Quads[iQuad].m_Color0 = Col;
				Quads[iQuad].m_Color1 = Col;
				Quads[iQuad].m_TextureYOfs = ((fp32(i) + f + Age) / (CClientData_Trail::MAXLEN));
				iQuad++;
				if(Quads[iQuad - 1].m_TextureYOfs >= 1.0f)
					break;
			}
		}

		iIndex--;
		if(iIndex < 0)
			iIndex += CClientData_Trail::MAXLEN;
	}

	CXR_VertexBuffer *pVB = AllocVB();
	if(!pVB)
		return;

	pVB->Geometry_Color(0xffffffff);
//	pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);

	CMat43fp32 Unit;
	Unit.Unit();
//	LogFile("hirr");
	if(CXR_Util::Render_QuadStrip(m_pVBM, pVB, Unit, _VMat, Quads, iQuad, CXR_QUADFLAGS_SPLITX4))
//		m_pVBM->AddVB(pVB);
		Render_Surface(m_iSurface, pVB, _pAnimState->m_AnimTime0);
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_DoubleTrail, CXR_Model_Custom);

// -------------------------------------------------------------------
//  CXR_Model_MagicTrail
// -------------------------------------------------------------------
void CXR_Model_MagicTrail::OnCreate(const char *)
{
	MAUTOSTRIP(CXR_Model_MagicTrail_OnCreate, MAUTOSTRIP_VOID);
	m_iTexture = GetTextureID("BLIZZARD");
	m_BoundRadius = 512;
}

fp32 CXR_Model_MagicTrail::GetBound_Sphere(const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_MagicTrail_GetBound_Sphere, 0.0f);
	CBox3Dfp32 Box;
	GetBound_Box(Box, _pAnimState);
	
	CVec3Dfp32 vMax;
	for(int i = 0; i < 3; i++)
		vMax[i] = Max(M_Fabs(Box.m_Max[i]), M_Fabs(Box.m_Min[i]));
	
	//Fix: Remove m_BoundRadius
	m_BoundRadius = vMax.Length();
	
	return m_BoundRadius;
}

CBox3Dfp32 CXR_Model_MagicTrail::GetAppliedBound(const CBox3Dfp32 &_Bound, const CMat43fp32 &_Mat)
{
	CBox3Dfp32 Bound;
	MAUTOSTRIP(CXR_Model_MagicTrail_GetAppliedBound, Bound);
	
	CVec3Dfp32 c[8];
	int i;
	for(i = 0; i < 8; i++)
	{
		c[i] = CVec3Dfp32((((i >> 0) & 1) ? _Bound.m_Min[0] : _Bound.m_Max[0]),
			(((i >> 1) & 1) ? _Bound.m_Min[1] : _Bound.m_Max[1]), 
			(((i >> 2) & 1) ? _Bound.m_Min[2] : _Bound.m_Max[2]));
		c[i] *= _Mat;
	}
	Bound.m_Min = c[0];
	Bound.m_Max = c[0];
	for(i = 1; i < 8; i++)
	{
		Bound.m_Min = CVec3Dfp32(Min(Bound.m_Min[0], c[i][0]), 
			Min(Bound.m_Min[1], c[i][1]), 
			Min(Bound.m_Min[2], c[i][2]));
		Bound.m_Max = CVec3Dfp32(Max(Bound.m_Max[0], c[i][0]), 
			Max(Bound.m_Max[1], c[i][1]), 
			Max(Bound.m_Max[2], c[i][2]));
	}
	
	return Bound;
}

void CXR_Model_MagicTrail::GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
{
	MAUTOSTRIP(CXR_Model_MagicTrail_GetBound_Box, MAUTOSTRIP_VOID);
	_Box.m_Max = 0;
	_Box.m_Min = 0;
	
	if(_pAnimState)
	{
		if(!_pAnimState->m_pspClientData)
			return;
		
		CClientData_Trail *pTrail = (CClientData_Trail *)(CReferenceCount *)*_pAnimState->m_pspClientData;
		if(!pTrail)
			return;
		
		_Box = GetAppliedBound(pTrail->m_BoundBox, pTrail->m_LastIWMat);
		
	}
}

void CXR_Model_MagicTrail::Render(const CXR_AnimState* _pAnimState, const CMat43fp32& _WMat, const CMat43fp32& _VMat)
{
	MAUTOSTRIP(CXR_Model_MagicTrail_Render, MAUTOSTRIP_VOID);
	if(!_pAnimState->m_pspClientData)
		return;
	
	CClientData_Trail *pTrail = (CClientData_Trail *)(CReferenceCount *)*_pAnimState->m_pspClientData;
	if(!pTrail)
		return;
	
	if(pTrail->m_nHistory < 2)
		return;
	
	CXR_BeamStrip Beams[CXR_Util::MAXBEAMS];
	
	const int Color = 0x00ffa080;
	const fp32 Tick = _pAnimState->m_AnimTime0 * SERVER_TICKSPERSECOND;
	const fp32 IPTime = Tick - int(Tick);
	
	const int MaxLen = 5;//CClientData_Trail::MAXLEN
	
	int iBeam = 0;
	{
		//Add current position to trail
		CXR_BeamStrip& Beam = Beams[0];
		Beam.m_Pos = CVec3Dfp32::GetMatrixRow(_WMat, 3);
		Beam.m_Color = 0xff000000 | Color;
		Beam.m_Width = 6;
		Beam.m_TextureYOfs = 0;
		iBeam++;
	}
	
	int iIndex = pTrail->m_iCurPos;
	
	const int nSegments = Min(MaxLen, pTrail->m_nHistory);
	for(int i = 0; i < nSegments; i++)
	{
		CXR_BeamStrip& Beam = Beams[iBeam];
		Beam.m_Pos = CVec3Dfp32::GetMatrixRow(pTrail->m_History[iIndex], 3);
		if(iBeam > 0 && Beams[iBeam - 1].m_Pos == Beam.m_Pos)
			continue;
		
		Beam.m_Pos[2] += 4 * (fp32(i) + IPTime);
		fp32 TimeFade = (fp32(MaxLen - i - 1) - IPTime) / (MaxLen - 1);
		Beam.m_Color = Color | (int(256 * TimeFade) << 24);
		
		Beam.m_Width = 6/* + (fp32(i) + IPTime) * 0.1f*/;
		Beam.m_TextureYOfs = (fp32(i) + IPTime) / MaxLen - fp32(iBeam);
		iBeam++;
		
		iIndex--;
		if(iIndex < 0)
			iIndex += CClientData_Trail::MAXLEN;
	}
	if(iBeam <= 2)
		return;
	
	Beams[iBeam - 1].m_Color = Color;
	
	CXR_VertexBuffer *pVB = AllocVB();
	if(!pVB)
		return;
	
	pVB->Geometry_Color(0xffffffff);
	pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
	pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	
	CMat43fp32 Unit;
	Unit.Unit();
	if(CXR_Util::Render_BeamStrip(m_pVBM, pVB, Unit, _VMat, Beams, iBeam))
		m_pVBM->AddVB(pVB);
}

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_MagicTrail, CXR_Model_Custom);



