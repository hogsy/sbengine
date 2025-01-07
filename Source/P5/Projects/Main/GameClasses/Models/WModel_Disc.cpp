#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "CDynamicVB.h"
#include "CSurfaceKey.h"
#include "CSinRandTable.h"
#include "CMultiModelInstance.h"
#include "ModelsMisc.h"

//----------------------------------------------------------------------

const char* lpDISC_FLAGS[] = { "", "", "", NULL };
enum
{
//	DISC_FLAGS_QUAD		= 0x0001, // Ignore inner radius and sides, render as quad.
//	DISC_FLAGS_EXPLODE	= 0x0002, // Don't interpolate radius linearly, use an explosion curve instead.
//	DISC_FLAGS_FACECAM	= 0x0004, // Generate axis to point towards camera.
};

const char* lpDISC_GEOMETRYTYPE[] = { "ring", "fan", "quad", NULL };
enum DiscGeometryType
{
	DISC_GEOMETRYTYPE_RING	= 0x00,
	DISC_GEOMETRYTYPE_FAN	= 0x01,
	DISC_GEOMETRYTYPE_QUAD	= 0x02,
};

const char* lpDISC_TEXMAPTYPE[] = { "rect", "circular", NULL };
enum DiscTexMapType
{
	DISC_TEXMAPTYPE_RECTANGULAR	= 0x00,
	DISC_TEXMAPTYPE_RECTMIRROR	= 0x01,
	DISC_TEXMAPTYPE_CIRCULAR	= 0x02,
};

const char* lpDISC_RADIUSIPTYPE[] = { "linear", "explode", "slowstart", NULL };
enum DiscRadiusIPType
{
	DISC_RADIUSIPTYPE_NONE		= 0x00,
	DISC_RADIUSIPTYPE_LINEAR	= 0x01,
	DISC_RADIUSIPTYPE_EXPLODE	= 0x02,
	DISC_RADIUSIPTYPE_SLOWSTART	= 0x03,
};

const char* lpDISC_AXISTYPE[] = { "up", "fwd", "cam", NULL };
enum DiscAxisType
{
	DISC_AXISTYPE_UP	= 0x00,
	DISC_AXISTYPE_FWD	= 0x01,
	DISC_AXISTYPE_CAM	= 0x02,
};

//----------------------------------------------------------------------
// CXR_Model_Disc
//----------------------------------------------------------------------

class CXR_Model_Disc : public CXR_Model_Custom
{

	MRTC_DECLARE;

public:

private:

	//----------------------------------------------------------------------


#ifndef M_RTM
	CWireContainer*		m_pWC;
#endif

	int					m_Randseed, m_RandseedBase;

	CDynamicVB			m_DVB;
	CSurfaceKey			m_SK;

	CSinRandTable1024x8	m_SinRand;

	CStr				m_Keys;

	CVec3Dfp32			m_CameraFwd;
	CMat43fp32			m_LocalToWorld;

	//----------------------------------------------------------------------

	fp32					m_Time;
	fp32					m_TimeScale, m_TimeOffset;
	fp32					m_Duration, m_ObjDuration;
	fp32					m_FadeInDuration, m_FadeOutDuration;
	fp32					m_Fade;

	int					m_Flags;

	int					m_iSurface;
	fp32					m_RenderPriorityBias;

	CVec4Dfp32			m_OuterColor, m_InnerColor;
	CVec4Dfp32			m_ObjColor;

	fp32					m_StartOuterRadius, m_EndOuterRadius;
	fp32					m_StartInnerRadius, m_EndInnerRadius;
	fp32					m_StartRadiusTime, m_EndRadiusTime;
	uint8				m_iRadiusIPType;

	int					m_nSides;

	CVec3Dfp32			m_LocalOffset;

	uint8				m_iAxisType;
	fp32					m_AxisAngle;
	fp32					m_AxisRot, m_AxisRotFluct, m_AxisRotFluctSpeed;

	uint8				m_iGeometryType;

	uint8				m_iTexMapType;
	CVec2Dfp32			m_TexCenter;
	CVec2Dfp32			m_TexScale;
	CVec2Dfp32			m_TexScroll;

	//----------------------------------------------------------------------

	void PostProcessTex(CVec2Dfp32& _Tex)
	{
		CVec2Dfp32 TexCenter = m_TexCenter + 0.5f;
		_Tex[0] = TexCenter[0] + (_Tex[0] - TexCenter[0]) * m_TexScale[0];
		_Tex[1] = TexCenter[1] + (_Tex[1] - TexCenter[1]) * m_TexScale[1];
		_Tex[0] += m_TexScroll[0] * m_Time;
		_Tex[1] += m_TexScroll[1] * m_Time;
	}

	//----------------------------------------------------------------------

	void GetRectTex(fp32 _SideFraction, fp32 _Radius, CVec2Dfp32& _Tex)
	{
		fp32 tc, ts; QSinCosUnit(_SideFraction - 0.25f, ts, tc);
		CVec3Dfp32 TexPos(tc, ts, 0);
		_Tex[0] = TexPos[0] * 0.5f * _Radius + 0.5f;
		_Tex[1] = TexPos[1] * 0.5f * _Radius + 0.5f;
		PostProcessTex(_Tex);
	}

	//----------------------------------------------------------------------

	void GetRectMirrorTex(fp32 _SideFraction, fp32 _Radius, CVec2Dfp32& _Tex)
	{
		fp32 tc, ts; QSinCosUnit(_SideFraction - 0.25f, ts, tc);
		CVec3Dfp32 TexPos(tc, ts, 0);
		_Tex[0] = M_Fabs(TexPos[0] * _Radius);
		_Tex[1] = M_Fabs(TexPos[1] * _Radius);
		PostProcessTex(_Tex);
	}

	//----------------------------------------------------------------------

	void GetCircularTex(fp32 _SideFraction, fp32 _Radius, CVec2Dfp32& _Tex)
	{
		_Tex[0] = _SideFraction;
		_Tex[1] = 1.0f - _Radius;
		PostProcessTex(_Tex);
	}

	//----------------------------------------------------------------------

	void GenerateQuad(fp32 _Radius, fp32 _BaseAngle)
	{
		m_DVB.AddTriangle(0, 2, 1, 0);
		m_DVB.AddTriangle(1, 2, 3, 0);

		fp32 c, s; QSinCosUnit(_BaseAngle, s, c);
		CVec3Dfp32 w(-s * _Radius, c * _Radius, 0);
		CVec3Dfp32 h(c * _Radius, s * _Radius, 0);

		uint32 OuterColor = CPixel32(m_OuterColor[0], m_OuterColor[1], m_OuterColor[2], m_Fade*m_OuterColor[3]);
		m_DVB.AddVertex(-w+h, 0.0f, 0.0f, OuterColor);
		m_DVB.AddVertex(w+h, 1.0f, 0.0f, OuterColor);
		m_DVB.AddVertex(-w-h, 0.0f, 1.0f, OuterColor);
		m_DVB.AddVertex(w-h, 1.0f, 1.0f, OuterColor);
	}	
	
	//----------------------------------------------------------------------

	void GenerateVertices_RectTex(fp32 _Radius, fp32 _BaseAngle, fp32 _TexRadius, uint32 _Color)
	{
		for (int iSide = 0; iSide < (m_nSides + 1); iSide++)
		{
			fp32 SideFraction = (fp32)iSide / (fp32)m_nSides;
			fp32 Angle = SideFraction + _BaseAngle * 2 * _PI;
			fp32 c, s; QSinCosUnit(Angle, s, c);
			CVec3Dfp32 Pos(c, s, 0);
			CVec2Dfp32 Tex; GetRectTex(SideFraction, _TexRadius, Tex);
			m_DVB.AddVertex(Pos * _Radius, Tex[0], Tex[1], _Color);
		}
	}

	//----------------------------------------------------------------------

	void GenerateVertices_RectMirrorTex(fp32 _Radius, fp32 _BaseAngle, fp32 _TexRadius, uint32 _Color)
	{
		for (int iSide = 0; iSide < (m_nSides + 1); iSide++)
		{
			fp32 SideFraction = (fp32)iSide / (fp32)m_nSides;
			fp32 Angle = SideFraction + _BaseAngle * 2 * _PI;
			fp32 c, s; QSinCosUnit(Angle, s, c);
			CVec3Dfp32 Pos(c, s, 0);
			CVec2Dfp32 Tex; GetRectMirrorTex(SideFraction, _TexRadius, Tex);
			m_DVB.AddVertex(Pos * _Radius, Tex[0], Tex[1], _Color);
		}
	}

	//----------------------------------------------------------------------

	void GenerateVertices_CircularTex(fp32 _Radius, fp32 _BaseAngle, fp32 _TexRadius, uint32 _Color)
	{
		for (int iSide = 0; iSide < (m_nSides + 1); iSide++)
		{
			fp32 SideFraction = (fp32)iSide / (fp32)m_nSides;
			fp32 Angle = SideFraction + _BaseAngle * 2 * _PI;
			fp32 c, s; QSinCosUnit(Angle, s, c);
			CVec3Dfp32 Pos(c, s, 0);
			CVec2Dfp32 Tex; GetCircularTex(SideFraction, _TexRadius, Tex);
			m_DVB.AddVertex(Pos * _Radius, Tex[0], Tex[1], _Color);
		}
	}

	//----------------------------------------------------------------------

	void GenerateFan(fp32 _Radius, fp32 _BaseAngle)
	{
		uint32 InnerColor = CPixel32(m_InnerColor[0], m_InnerColor[1], m_InnerColor[2], m_Fade*m_InnerColor[3]);
		uint32 OuterColor = CPixel32(m_OuterColor[0], m_OuterColor[1], m_OuterColor[2], m_Fade*m_OuterColor[3]);
		int iSide;
		for (iSide = 0; iSide < m_nSides; iSide++)
			m_DVB.AddTriangle(0, iSide + 2, iSide + 1, 0);

		if (m_iTexMapType == DISC_TEXMAPTYPE_RECTANGULAR)
		{
			CVec2Dfp32 CenterTex; GetRectTex(m_TexCenter[0], 0, CenterTex);
			m_DVB.AddVertex(CVec3Dfp32(0, 0, 0), CenterTex[0], CenterTex[1], InnerColor);
			GenerateVertices_RectTex(_Radius, _BaseAngle, 1, OuterColor);
/*
			for (iSide = 0; iSide < (m_nSides + 1); iSide++)
			{
				fp32 SideFraction = (fp32)iSide / (fp32)m_nSides;
				fp32 Angle = SideFraction + _BaseAngle * 2 * _PI;
				fp32 c, s; QSinCosUnit(Angle, s, c);
				CVec3Dfp32 Pos(c, s, 0);
				CVec2Dfp32 Tex; GetRectTex(SideFraction, 1, Tex);
				m_DVB.AddVertex(Pos * _Radius, Tex[0], Tex[1], OuterColor);
			}
*/
		}
		else if (m_iTexMapType == DISC_TEXMAPTYPE_RECTMIRROR)
		{
			CVec2Dfp32 CenterTex; GetRectMirrorTex(m_TexCenter[0], 0, CenterTex);
			m_DVB.AddVertex(CVec3Dfp32(0, 0, 0), CenterTex[0], CenterTex[1], InnerColor);
			GenerateVertices_RectMirrorTex(_Radius, _BaseAngle, 1, OuterColor);
/*			
			for (iSide = 0; iSide < (m_nSides + 1); iSide++)
			{
				fp32 SideFraction = (fp32)iSide / (fp32)m_nSides;
				fp32 Angle = SideFraction + _BaseAngle * 2 * _PI;
				fp32 c, s; QSinCosUnit(Angle, s, c);
				CVec3Dfp32 Pos(c, s, 0);
				CVec2Dfp32 Tex; GetRectMirrorTex(SideFraction, 1, Tex);
				m_DVB.AddVertex(Pos * _Radius, Tex[0], Tex[1], OuterColor);
			}
*/
		}
		else if (m_iTexMapType == DISC_TEXMAPTYPE_CIRCULAR)
		{
			CVec2Dfp32 CenterTex; GetCircularTex(m_TexCenter[0], 0, CenterTex);
			m_DVB.AddVertex(CVec3Dfp32(0, 0, 0), CenterTex[0], CenterTex[1], InnerColor);
			GenerateVertices_CircularTex(_Radius, _BaseAngle, 1, OuterColor);
/*
			for (iSide = 0; iSide < (m_nSides + 1); iSide++)
			{
				fp32 SideFraction = (fp32)iSide / (fp32)m_nSides;
				fp32 Angle = SideFraction + _BaseAngle * 2 * _PI;
				fp32 c, s; QSinCosUnit(Angle, s, c);
				CVec3Dfp32 Pos(c, s, 0);
				CVec2Dfp32 Tex; GetCircularTex(SideFraction, 1, Tex);
				m_DVB.AddVertex(Pos * _Radius, Tex[0], Tex[1], OuterColor);
			}
*/
		}
	}	
	
	//----------------------------------------------------------------------

	void GenerateRing(fp32 _InnerRadius, fp32 _OuterRadius, fp32 _BaseAngle)
	{
		uint32 InnerColor = CPixel32(m_InnerColor[0], m_InnerColor[1], m_InnerColor[2], m_Fade*m_InnerColor[3]);
		uint32 OuterColor = CPixel32(m_OuterColor[0], m_OuterColor[1], m_OuterColor[2], m_Fade*m_OuterColor[3]);
		int iSide;
		for (iSide = 0; iSide < m_nSides; iSide++)
		{
			if ((iSide & 1) == 0)
			{
				m_DVB.AddTriangle(iSide, iSide + 1, iSide + m_nSides + 1, 0);
				m_DVB.AddTriangle(iSide + 1, iSide + m_nSides + 2, iSide + m_nSides + 1, 0);
			}
			else
			{
				m_DVB.AddTriangle(iSide, iSide + 1, iSide + m_nSides + 2, 0);
				m_DVB.AddTriangle(iSide + m_nSides + 2, iSide + m_nSides + 1, iSide, 0);
			}
		}

		if (m_iTexMapType == DISC_TEXMAPTYPE_RECTANGULAR)
		{
			GenerateVertices_RectTex(_InnerRadius, _BaseAngle, 0, InnerColor);
			GenerateVertices_RectTex(_OuterRadius, _BaseAngle, 1, OuterColor);
/*
			for (iSide = 0; iSide < (m_nSides + 1); iSide++)
			{
				fp32 SideFraction = (fp32)iSide / (fp32)m_nSides;
				fp32 Angle = SideFraction + _BaseAngle * 2 * _PI;
				fp32 c, s; QSinCosUnit(Angle, s, c);
				CVec3Dfp32 Pos(c, s, 0);
				CVec2Dfp32 Tex; GetRectTex(SideFraction, 0, Tex);
				m_DVB.AddVertex(Pos * _InnerRadius, Tex[0], Tex[1], InnerColor);
			}
			for (iSide = 0; iSide < (m_nSides + 1); iSide++)
			{
				fp32 SideFraction = (fp32)iSide / (fp32)m_nSides;
				fp32 Angle = SideFraction + _BaseAngle * 2 * _PI;
				fp32 c, s; QSinCosUnit(Angle, s, c);
				CVec3Dfp32 Pos(c, s, 0);
				CVec2Dfp32 Tex; GetRectTex(SideFraction, 1, Tex);
				m_DVB.AddVertex(Pos * _OuterRadius, Tex[0], Tex[1], OuterColor);
			}
*/
		}
		else if (m_iTexMapType == DISC_TEXMAPTYPE_RECTMIRROR)
		{
			GenerateVertices_RectMirrorTex(_InnerRadius, _BaseAngle, 0, InnerColor);
			GenerateVertices_RectMirrorTex(_OuterRadius, _BaseAngle, 1, OuterColor);
/*
			for (iSide = 0; iSide < (m_nSides + 1); iSide++)
			{
				fp32 SideFraction = (fp32)iSide / (fp32)m_nSides;
				fp32 Angle = SideFraction + _BaseAngle * 2 * _PI;
				fp32 c, s; QSinCosUnit(Angle, s, c);
				CVec3Dfp32 Pos(c, s, 0);
				CVec2Dfp32 Tex; GetRectMirrorTex(SideFraction, 0, Tex);
				m_DVB.AddVertex(Pos * _InnerRadius, Tex[0], Tex[1], InnerColor);
			}
			for (iSide = 0; iSide < (m_nSides + 1); iSide++)
			{
				fp32 SideFraction = (fp32)iSide / (fp32)m_nSides;
				fp32 Angle = SideFraction + _BaseAngle * 2 * _PI;
				fp32 c, s; QSinCosUnit(Angle, s, c);
				CVec3Dfp32 Pos(c, s, 0);
				CVec2Dfp32 Tex; GetRectMirrorTex(SideFraction, 1, Tex);
				m_DVB.AddVertex(Pos * _OuterRadius, Tex[0], Tex[1], OuterColor);
			}
*/
		}
		else if (m_iTexMapType == DISC_TEXMAPTYPE_CIRCULAR)
		{
			GenerateVertices_CircularTex(_InnerRadius, _BaseAngle, 0, InnerColor);
			GenerateVertices_CircularTex(_OuterRadius, _BaseAngle, 1, OuterColor);
/*
			for (iSide = 0; iSide < (m_nSides + 1); iSide++)
			{
				fp32 SideFraction = (fp32)iSide / (fp32)m_nSides;
				fp32 Angle = SideFraction + _BaseAngle * 2 * _PI;
				fp32 c, s; QSinCosUnit(Angle, s, c);
				CVec3Dfp32 Pos(c, s, 0);
				CVec2Dfp32 Tex; GetCircularTex(SideFraction, 0, Tex);
				m_DVB.AddVertex(Pos * _InnerRadius, Tex[0], Tex[1], InnerColor);
			}
			for (iSide = 0; iSide < (m_nSides + 1); iSide++)
			{
				fp32 SideFraction = (fp32)iSide / (fp32)m_nSides;
				fp32 Angle = SideFraction + _BaseAngle * 2 * _PI;
				fp32 c, s; QSinCosUnit(Angle, s, c);
				CVec3Dfp32 Pos(c, s, 0);
				CVec2Dfp32 Tex; GetCircularTex(SideFraction, 1, Tex);
				m_DVB.AddVertex(Pos * _OuterRadius, Tex[0], Tex[1], OuterColor);
			}
*/
		}
	}	
	
	//----------------------------------------------------------------------

	void Generate()
	{
		fp32 BaseAngle = m_AxisAngle + m_AxisRot * m_Time + m_AxisRotFluct * m_SinRand.GetRand(m_Time * m_AxisRotFluctSpeed);

		fp32 InnerRadius;
		fp32 OuterRadius;
		if (m_iRadiusIPType != DISC_RADIUSIPTYPE_NONE)
		{
			fp32 RadiusFraction = Clamp01((m_Time - m_StartRadiusTime) / (m_EndRadiusTime - m_StartRadiusTime));
			switch (m_iRadiusIPType)
			{
				case DISC_RADIUSIPTYPE_LINEAR: break;
				case DISC_RADIUSIPTYPE_EXPLODE: RadiusFraction = 1.0f - Sqr(Sqr(1.0f - RadiusFraction)); break;
				case DISC_RADIUSIPTYPE_SLOWSTART: RadiusFraction = Sqr(Sqr(RadiusFraction)); break;
			}

			InnerRadius = LERP(m_StartInnerRadius, m_EndInnerRadius, RadiusFraction);
			OuterRadius = LERP(m_StartOuterRadius, m_EndOuterRadius, RadiusFraction);
		}
		else
		{
			InnerRadius = m_StartInnerRadius;
			OuterRadius = m_StartOuterRadius;
		}
		
		switch (m_iGeometryType)
		{
			case DISC_GEOMETRYTYPE_QUAD:
				GenerateQuad(OuterRadius, BaseAngle);
				break;
			case DISC_GEOMETRYTYPE_FAN:
				GenerateFan(OuterRadius, BaseAngle);
				break;
			case DISC_GEOMETRYTYPE_RING:
				GenerateRing(InnerRadius, OuterRadius, BaseAngle);
				break;
		}
	}

	//----------------------------------------------------------------------

	CMultiModelInstance* GetMultiInstance(const CXR_AnimState* _pAnimState)
	{
		if (_pAnimState == NULL)
			return NULL;

		if (_pAnimState->m_pContext == NULL)
			return NULL;

		CMultiModelInstance* pContext = (CMultiModelInstance*)_pAnimState->m_pContext;
		return (TDynamicCast<CMultiModelInstance>(pContext));
	}

	//----------------------------------------------------------------------

	bool GetMultiBoundBox(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
	{
		CMultiModelInstance* pInstances = GetMultiInstance(_pAnimState);
		
		if (pInstances == NULL)
			return false;

		_Box.m_Min = 0;
		_Box.m_Max = 0;

		for (int iInstance = 0; iInstance < pInstances->GetNumInstances(); iInstance++)
		{
			CXR_AnimState AnimState;
			CMat43fp32 LocalToWorld;
			if (!pInstances->GetInstance(iInstance, _pAnimState->m_AnimTime0, &AnimState, &LocalToWorld))
				continue;

			CBox3Dfp32 LocalBox, WorldBox;
			GetBound_Box(LocalBox, &AnimState);
			LocalBox.Transform(LocalToWorld, WorldBox);

			if (iInstance == 0)
				_Box = WorldBox;
			else
				_Box.Expand(WorldBox);
		}

		return true;
	}

	//----------------------------------------------------------------------

	virtual void GetBound_Box(CBox3Dfp32& _Box, const CXR_AnimState* _pAnimState)
	{
		if (GetMultiBoundBox(_Box, _pAnimState))
			return;

		const fp32 MaxRadius = Max(Max(m_StartInnerRadius, m_EndInnerRadius), Max(m_StartOuterRadius, m_EndOuterRadius));
		_Box.m_Min[0] = -MaxRadius;
		_Box.m_Min[1] = -MaxRadius;
		_Box.m_Min[2] = -1;
		_Box.m_Max[0] = MaxRadius;
		_Box.m_Max[1] = MaxRadius;
		_Box.m_Max[2] = 1;
	}

	//----------------------------------------------------------------------

	virtual bool RenderMultipleInstances(const CXR_AnimState* _pAnimState, const CMat43fp32& _LocalToWorld, const CMat43fp32& _WorldToCamera)
	{
		CMultiModelInstance* pInstances = GetMultiInstance(_pAnimState);
		
		if (pInstances == NULL)
			return false;

		for (int iInstance = 0; iInstance < pInstances->GetNumInstances(); iInstance++)
		{
			CXR_AnimState AnimState;
			CMat43fp32 LocalToWorld;
			if (!pInstances->GetInstance(iInstance, _pAnimState->m_AnimTime0, &AnimState, &LocalToWorld))
			{
				continue;
			}

			RenderDisc(&AnimState, LocalToWorld, _WorldToCamera);
		}

		return true;
	}

	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_Disc_Render, MAUTOSTRIP_VOID);

#ifndef M_RTM
		{
			//JK-NOTE: Just set this if it isn't initialized... GetRegisterObject is slow
			m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));
		}
#endif		

		CMat4Dfp32 InvWorldToCamera;
		_VMat.InverseOrthogonal(InvWorldToCamera);
		m_CameraFwd = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 2);

		if (RenderMultipleInstances(_pAnimState, _WMat, _VMat))
			return;

		RenderDisc(_pAnimState, _WMat, _VMat);
	}


	void RenderDisc(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_Disc_RenderDisc, MAUTOSTRIP_VOID);

		m_Time = _pAnimState->m_AnimTime0;
		m_ObjDuration = _pAnimState->m_AnimTime1;
		m_ObjColor = CVec4Dfp32((CVec4Dfp32)CPixel32(_pAnimState->m_Colors[0]));		

		m_Time = (m_Time + m_TimeOffset) * m_TimeScale;

		fp32 MinDuration;
		if (m_Duration != 0)
		{
			if (m_ObjDuration != 0)
				MinDuration = Min(m_Duration, m_ObjDuration);
			else
				MinDuration = m_Duration;
		}
		else
		{
			if (m_ObjDuration != 0)
				MinDuration = m_ObjDuration;
			else
				MinDuration = 0;
		}


		m_Fade = ::GetFade(m_Time, MinDuration, m_FadeInDuration, m_FadeOutDuration) * (m_ObjColor[3] / 255.0f);

		if (m_Fade <= 0)
			return;

		m_RandseedBase = _pAnimState->m_Anim0;
		m_Randseed = m_RandseedBase;

		m_LocalToWorld	= _WMat;
/*
		switch (m_iAxisType)
		{
			case DISC_AXISTYPE_UP:
				break;
			case DISC_AXISTYPE_FWD:
				up -> fwd
				fwd -> up
				m_LocalToWorld
				break;
			case DISC_AXISTYPE_CAM:
				break;
		}
*/

		// FIXME: Add local offset (be sure to add it in localspace, not worldspace).

		int NumVertices, NumTriangles;
		switch (m_iGeometryType)
		{
			case DISC_GEOMETRYTYPE_QUAD:
				NumVertices = 4;
				NumTriangles = 2;
				break;
			case DISC_GEOMETRYTYPE_FAN:
				NumVertices = m_nSides + 2;
				NumTriangles = m_nSides;
				break;
			case DISC_GEOMETRYTYPE_RING:
				NumVertices = m_nSides * 2 + 2;
				NumTriangles = m_nSides * 2;
				break;
		}

		m_SK.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface);

		if (!m_DVB.Create(this, m_pVBM, NumVertices, NumTriangles, DVBFLAGS_TEXTURE | DVBFLAGS_COLORS))
		{
			return;
		}

		m_DVB.GetVB()->m_Priority += m_RenderPriorityBias;

		Generate();

		if (m_DVB.IsValid())
		{
			CMat43fp32 LocalToCamera;
			m_LocalToWorld.Multiply(_VMat, LocalToCamera);
			m_DVB.Render(LocalToCamera);
			m_SK.Render(m_DVB.GetVB(), m_pVBM, m_pEngine);

#ifndef M_RTM
			if (m_pWC != NULL)
				m_pWC->RenderWire(CVec3Dfp32(-1500, -2000, -1200), CVec3Dfp32::GetMatrixRow(_WMat, 3), 0xFF00FF00, 0.5f);
#endif
		}
	}

	//----------------------------------------------------------------------

	void OnEvalKey(const CRegistry *_pReg)
	{
		MAUTOSTRIP(CXR_Model_Disc_OnEvalKey, MAUTOSTRIP_VOID);
		CStr Name = _pReg->GetThisName();
		CStr Value = _pReg->GetThisValue();
		const int Valuei = _pReg->GetThisValue().Val_int();
		const fp32 Valuef = _pReg->GetThisValue().Val_fp64();

		if (Name == "SU")
		{
			CFStr SurfaceName = Value.GetStrMSep(" #");
			m_iSurface = GetSurfaceID(SurfaceName);
			m_RenderPriorityBias = (fp32)(Value.Val_fp64()) * TransparencyPriorityBiasUnit;
		}

		else if (Name == "F")
			m_Flags = Value.TranslateFlags(lpDISC_FLAGS);

		else if (Name == "TS")
			m_TimeScale = Valuef;

		else if (Name == "TO")
			m_TimeOffset = Valuef;

		else if (Name == "DU")
			m_Duration = Valuef;

		else if (Name == "FI")
			m_FadeInDuration = Valuef;

		else if (Name == "FO")
			m_FadeOutDuration = Valuef;

		else if (Name == "OC")
			m_OuterColor.ParseColor(Value);

		else if (Name == "IC")
			m_InnerColor.ParseColor(Value);

		else if (Name == "SOR")
			m_StartOuterRadius = Valuef;

		else if (Name == "EOR")
			m_EndOuterRadius = Valuef;

		else if (Name == "SIR")
			m_StartInnerRadius = Valuef;

		else if (Name == "EIR")
			m_EndInnerRadius = Valuef;

		else if (Name == "SRT")
			m_StartRadiusTime = Valuef;

		else if (Name == "ERT")
			m_EndRadiusTime = Valuef;

		else if (Name == "RT")
			m_iRadiusIPType = Value.TranslateInt(lpDISC_RADIUSIPTYPE);

		else if (Name == "SID")
			m_nSides = Valuei;

		else if (Name == "LO")
			m_LocalOffset.ParseString(Value);

		else if (Name == "AT")
			m_iAxisType = Value.TranslateInt(lpDISC_AXISTYPE);

		else if (Name == "AA")
			m_AxisAngle = Valuef;

		else if (Name == "AR")
			m_AxisRot = Valuef;

		else if (Name == "ARF")
			m_AxisRotFluct = Valuef;

		else if (Name == "ARFS")
			m_AxisRotFluctSpeed = Valuef;

		else if (Name == "GT")
			m_iGeometryType = Value.TranslateInt(lpDISC_GEOMETRYTYPE);

		else if (Name == "TT")
			m_iTexMapType = Value.TranslateInt(lpDISC_TEXMAPTYPE);

		else if (Name == "TXS")
			m_TexScale.ParseString(Value);

		else
			CXR_Model_Custom::OnEvalKey(_pReg);
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *_keys)
	{
		MAUTOSTRIP(CXR_Model_Disc_OnCreate, MAUTOSTRIP_VOID);
		m_Keys = _keys;
		InitParameters();
		ParseKeys(_keys);
		PostprocessParameters();
	}

	//----------------------------------------------------------------------

	void InitParameters()
	{
		MAUTOSTRIP(CXR_Model_Disc_SetDefaultParameters, MAUTOSTRIP_VOID);
		m_iSurface = GetSurfaceID("WaterRipple01");
		m_RenderPriorityBias = 0;
		m_Flags = 0;

		m_TimeScale = 1;
		m_TimeOffset = 0;
		m_Duration = 1.5f;
		m_FadeInDuration = 0.2f;
		m_FadeOutDuration = 0.8f;

//		m_OuterColor = CVec4Dfp32(176, 208, 240, 32);
//		m_InnerColor= CVec4Dfp32(176, 208, 240, 32);
		m_OuterColor.ParseColor("0x20B0D0E0");
		m_InnerColor.ParseColor("0x20B0D0E0");

		m_StartOuterRadius = 8;
		m_StartInnerRadius = 0;
		m_EndOuterRadius = 48;
		m_EndInnerRadius = 0;
		m_StartRadiusTime = 0;
		m_EndRadiusTime = 1.5f;
		m_iRadiusIPType = DISC_RADIUSIPTYPE_LINEAR;
		m_iGeometryType = DISC_GEOMETRYTYPE_QUAD;

		m_nSides = 0;

		m_LocalOffset = 0;
		m_iAxisType = DISC_AXISTYPE_UP;

		m_AxisAngle = 0;
		m_AxisRot = 0;
		m_AxisRotFluct = 0;
		m_AxisRotFluctSpeed = 0;

		m_iTexMapType = DISC_TEXMAPTYPE_RECTANGULAR;
		m_TexCenter = 0.0f;
		m_TexScale = CVec2Dfp32(1.0f, 1.0f);
		m_TexScroll = 0.0f;
	}

	//----------------------------------------------------------------------

	void PostprocessParameters()
	{
		MAUTOSTRIP(CXR_Model_Disc_PreprocessParameters, MAUTOSTRIP_VOID);
		m_RenderPriorityBias += TransparencyPriorityBaseBias;

		if ((m_iTexMapType == DISC_TEXMAPTYPE_CIRCULAR) && (m_iGeometryType == DISC_GEOMETRYTYPE_QUAD))
			m_iGeometryType = DISC_GEOMETRYTYPE_FAN;

		if ((m_StartInnerRadius == 0) && (m_EndInnerRadius == 0) && (m_iGeometryType == DISC_GEOMETRYTYPE_RING))
			m_iGeometryType = DISC_GEOMETRYTYPE_FAN;

		if ((m_EndRadiusTime - m_StartRadiusTime) <= 0)
			m_iRadiusIPType = DISC_RADIUSIPTYPE_NONE;

		if (m_nSides < 3)
			m_nSides = 3;
	}

	//----------------------------------------------------------------------

};

//----------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Disc, CXR_Model_Custom);

//----------------------------------------------------------------------
