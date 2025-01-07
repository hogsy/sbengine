#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "CDynamicVB.h"
#include "CSurfaceKey.h"
#include "CSinRandTable.h"
#include "CPropertyControl.h"
#include "ModelsMisc.h"

//----------------------------------------------------------------------

static
const char* lpEXPANDSPHERE_FLAGS[] = { "clampcolors", "bendnormals", "fadepoles", "postscale", "shade", "flatshade", "worldrot", "cullsort", "fadeedges", NULL };
enum
{
	EXPANDSPHERE_FLAGS_CLAMPCOLORS		= 0x0001,
	EXPANDSPHERE_FLAGS_BENDNORMALS		= 0x0002,
	EXPANDSPHERE_FLAGS_FADEPOLES		= 0x0004,
	EXPANDSPHERE_FLAGS_POSTSCALE		= 0x0008,
	EXPANDSPHERE_FLAGS_SHADE			= 0x0010,
	EXPANDSPHERE_FLAGS_FLATSHADE		= 0x0020,
	EXPANDSPHERE_FLAGS_WORLDROT			= 0x0040,
	EXPANDSPHERE_FLAGS_CULLSORT			= 0x0080,
	EXPANDSPHERE_FLAGS_FADEEDGES		= 0x0100,
};

//----------------------------------------------------------------------

#define EDGEFADEMARGIN (0.1f)

//----------------------------------------------------------------------
// CXR_Model_ExpandSphere
//----------------------------------------------------------------------

class CXR_Model_ExpandSphere : public CXR_Model_Custom
{

	MRTC_DECLARE;

public:

private:

	//----------------------------------------------------------------------

	fp32					m_Time; // Total lifetime of model.
	fp32					m_Duration;
	uint32				m_Randseed, m_RandseedBase;

	CDynamicVB			m_DVB;
	CSurfaceKey			m_SK;

	CWireContainer*		m_pWC;

	int					m_iSurface;
	fp32					m_RenderPriorityBias;

	CStr				m_Keys;

	fp32					m_DurationFade;
	fp32					m_WantedDuration;

	CVec3Dfp32			m_CameraFwd;

	int					m_Segments, m_Sides;

	fp32					m_RadiusStart, m_RadiusEnd;

	CVec3Dfp32			m_Rotation, m_RotationOffset, m_RotationNoise;
	fp32					m_RotationNoiseFluctSpeed;

	CVec3Dfp32			m_Scaling;

	CVec3Dfp32			m_Center;

	CMat43fp32			m_TransformationMatrix, m_NormalTransformationMatrix;
	bool				m_bApplyTransformation;

	CVec2Dfp32			m_TexOffset;
	CVec2Dfp32			m_TexScale;
	CVec2Dfp32			m_TexNoise;
	CVec2Dfp32			m_TexNoiseFluctSpeed;

	CVec4Dfp32			m_Color, m_ColorNoise;
	fp32					m_ColorTurbulence;
	fp32					m_ColorNoiseFluctSpeed;

	fp32					m_PoleThreshold;

	int					m_Flags;

	//----------------------------------------------------------------------

	fp32 GetScalarFromNormal(CVec3Dfp32 _Normal) const
	{
		MAUTOSTRIP(CXR_Model_ExpandSphere_GetScalarFromNormal, 0);
		_Normal[0] = g_SinRandTable.GetRand(1.0f + _Normal[0] * 0.03f);
		_Normal[1] = g_SinRandTable.GetRand(1.0f + _Normal[1] * 0.05f);
		_Normal[2] = g_SinRandTable.GetRand(1.0f + _Normal[2] * 0.07f);

		fp32 Scalar;
		Scalar = _Normal[0] + _Normal[1] + _Normal[2];

		return Scalar;
	}

	//----------------------------------------------------------------------

	fp32 GetSinRand(fp32 _TimeOffset, fp32 _TimeScale) const
	{
		MAUTOSTRIP(CXR_Model_ExpandSphere_GetSinRand, 0);
		return g_SinRandTable.GetRand((m_Time + _TimeOffset) * _TimeScale);
//		return m_SinRandTable.GetRand(m_Time * _TimeScale + _TimeOffset);
	}

	//----------------------------------------------------------------------

	void GenerateVertex(CVec3Dfp32 _Normal, CVec2Dfp32 _Tex, bool _FreezeRandseed = false)
	{
		MAUTOSTRIP(CXR_Model_ExpandSphere_GenerateVertex, MAUTOSTRIP_VOID);
		int FrozenRandseed = m_Randseed;
		
		fp32 NoiseScalar = GetScalarFromNormal(_Normal);

		CVec3Dfp32 Pos = _Normal;
		Pos *= LERP(m_RadiusStart, m_RadiusEnd, m_Time / m_WantedDuration);
		_Tex += m_TexOffset;
		_Tex[0] += m_TexNoise[0] * GetSinRand(GetRand(m_Randseed) + NoiseScalar, m_TexNoiseFluctSpeed[0]);
		_Tex[1] += m_TexNoise[1] * GetSinRand(GetRand(m_Randseed) + NoiseScalar, m_TexNoiseFluctSpeed[1]);
		_Tex[0] *= m_TexScale[0];
		_Tex[1] *= m_TexScale[1];

		CVec4Dfp32 Color = m_Color + m_ColorNoise * (GetSinRand(NoiseScalar * m_ColorTurbulence, m_ColorNoiseFluctSpeed) - 0.5f);

		if (m_Flags & EXPANDSPHERE_FLAGS_CLAMPCOLORS)
		{
			Color[0] = Max(0.0f, Min(Color[0], 255.0f));
			Color[1] = Max(0.0f, Min(Color[1], 255.0f));
			Color[2] = Max(0.0f, Min(Color[2], 255.0f));
			Color[3] = Max(0.0f, Min(Color[3], 255.0f));
		}

		if (m_Flags & EXPANDSPHERE_FLAGS_FADEPOLES)
		{
			fp32 n = M_Fabs(_Normal[2]);
			if (n > m_PoleThreshold)
				Color[3] *= 1.0f - (n - m_PoleThreshold) / (1.0f - m_PoleThreshold);
		}

		Color[3] *= m_DurationFade;

		if (m_bApplyTransformation)
		{
			Pos.MultiplyMatrix3x3(m_TransformationMatrix);

			if (!(m_Flags & EXPANDSPHERE_FLAGS_BENDNORMALS))
				_Normal.MultiplyMatrix3x3(m_NormalTransformationMatrix);
		}

		if (m_Flags & EXPANDSPHERE_FLAGS_FADEEDGES)
		{
			fp32 EdgeFade = M_Fabs(_Normal * m_CameraFwd);
			Color[3] *= Max(0.0f, EdgeFade - EDGEFADEMARGIN) * (1.0f / (1.0f - EDGEFADEMARGIN));
		}

		Pos += m_Center;

		m_DVB.AddVertex(Pos, _Normal, _Tex, CPixel32(Color[0], Color[1], Color[2], Color[3]));

		if (_FreezeRandseed)
			m_Randseed = FrozenRandseed;
	}

	//----------------------------------------------------------------------

	void GenerateExpandSphere()
	{
		MAUTOSTRIP(CXR_Model_ExpandSphere_GenerateExpandSphere, MAUTOSTRIP_VOID);
		int v0, v1, v2, v3;
		int segment, side;
		fp32 a, b, u, v;
		fp32 xyradius;
		CVec3Dfp32 n;
		CVec2Dfp32 t;

		// Triangles...
		for (side = 0; side < m_Sides; side++)
		{
			v0 = side;
			v1 = m_Sides + 1 + side;
			v2 = v1 - 1;
			m_DVB.AddTriangle(v0, v1, v2, 0);
		}
		
		for (segment = 0; segment < (m_Segments - 2); segment++)
		{
			for (side = 0; side < m_Sides; side++)
			{
				v0 = m_Sides + segment * (m_Sides + 1) + side;
				v1 = v0 + 1;
				v2 = v1 + (m_Sides + 1);
				v3 = v2 - 1;
				m_DVB.AddTriangle(v0, v1, v2, 0);
				m_DVB.AddTriangle(v2, v3, v0, 0);
			}
		}

		for (side = 0; side < m_Sides; side++)
		{
			v0 = m_Sides + (m_Segments - 2) * (m_Sides + 1) + side;
			v1 = v0 + 1;
			v2 = m_Sides * 2 + (m_Segments - 2) * (m_Sides + 1) + 1 + side;
			m_DVB.AddTriangle(v0, v1, v2, 0);
		}

		// Vertices...
		for (side = 0; side < m_Sides; side++)
		{
			u = ((fp32)side + 0.5f) / (fp32)m_Sides;
			GenerateVertex(CVec3Dfp32(0, 0, 1), CVec2Dfp32(u, 0.0f), true);
		}
		
		for (segment = 0; segment < (m_Segments - 1); segment++)
		{
			u = (fp32)(segment + 1) / (fp32)(m_Segments);
			a = _PI * u;
			n[2] = M_Cos(a);
			xyradius = M_Sin(a);

			for (side = 0; side < (m_Sides + 1); side++)
			{
				v = (fp32)side / (fp32)m_Sides;
				b = 2 * _PI * v;
				n[0] = xyradius * M_Cos(b);
				n[1] = xyradius * M_Sin(b);
				t[0] = v;
				t[1] = u;
				GenerateVertex(n, t, true);
			}
		}
		
		for (side = 0; side < m_Sides; side++)
		{
			u = ((fp32)side + 0.5f) / (fp32)m_Sides;
			GenerateVertex(CVec3Dfp32(0, 0, -1), CVec2Dfp32(u, 1.0f), true);
		}

		// Normals...
		if ((m_Flags & EXPANDSPHERE_FLAGS_BENDNORMALS) && (!(m_Flags & EXPANDSPHERE_FLAGS_FLATSHADE)))
			m_DVB.RecalculateNormals();

		if (m_Flags & EXPANDSPHERE_FLAGS_FLATSHADE)
			m_DVB.ConvertToFlatShaded();

		if ((m_Flags & EXPANDSPHERE_FLAGS_SHADE) || (m_Flags & EXPANDSPHERE_FLAGS_FLATSHADE))
		{
			m_DVB.ApplyDiffuseLight(m_Center, this, m_pVBM);
		}

		/*
		m_DVB.DebugRenderNormals(m_pWC, 0.05f);

		if (m_pWC != NULL)
			m_pWC->RenderMatrix(m_NormalTransformationMatrix, 0.05f);
		*/
	}	
	
	//----------------------------------------------------------------------

	virtual fp32 GetBound_Sphere(const CXR_AnimState* _pAnimState)
	{
		MAUTOSTRIP(CXR_Model_ExpandSphere_GetBound_Sphere, 0);
		if (_pAnimState != NULL)
		{
			m_Time = _pAnimState->m_AnimTime0;
			m_Duration = _pAnimState->m_AnimTime1;
		}

		if (m_Time > m_WantedDuration)
			return 0;

		return LERP(m_RadiusStart, m_RadiusEnd, m_Time / m_WantedDuration);
	}

	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_ExpandSphere_Render, MAUTOSTRIP_VOID);
		m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));

		if (_pAnimState == NULL)
			return;

		m_Time = _pAnimState->m_AnimTime0;
		m_Duration = _pAnimState->m_AnimTime1;

		if (m_Time > m_WantedDuration)
			return;

		// Set fadetime to m_ParticleFadeInTime and use GetFade() if duration > 0.
		SetFadeTime(m_FadeTime);
		m_DurationFade = GetFade(_pAnimState);

		m_DurationFade *= 1.0f - m_Time / m_WantedDuration;

		if (false)
			ConOut(CStrF("Time = %f, Duration = %f, DurationFade = %f", m_Time, m_Duration, m_DurationFade));

		m_RandseedBase = _pAnimState->m_Anim0;
		m_Randseed = m_RandseedBase;

		CMat43fp32 InvWorldToCamera;
		_VMat.InverseOrthogonal(InvWorldToCamera);
		m_CameraFwd = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 2);

		m_Center = CVec3Dfp32::GetMatrixRow(_WMat, 3);

		m_bApplyTransformation = false;

		m_TransformationMatrix.Unit3x3();

		CVec3Dfp32(0).SetMatrixRow(m_TransformationMatrix, 3);

		if ((m_Rotation != 0) || (m_RotationOffset != 0) || (m_RotationNoise != 0))
		{
			m_bApplyTransformation = true;
			m_TransformationMatrix.M_x_RotZ(m_RotationOffset[2] + m_Rotation[2] * m_Time + m_RotationNoise[2] * (GetSinRand(GetRand(m_Randseed) * 100.0f, m_RotationNoiseFluctSpeed) - 0.5f));
			m_TransformationMatrix.M_x_RotY(m_RotationOffset[1] + m_Rotation[1] * m_Time + m_RotationNoise[1] * (GetSinRand(GetRand(m_Randseed) * 100.0f, m_RotationNoiseFluctSpeed) - 0.5f));
			m_TransformationMatrix.M_x_RotX(m_RotationOffset[0] + m_Rotation[0] * m_Time + m_RotationNoise[0] * (GetSinRand(GetRand(m_Randseed) * 100.0f, m_RotationNoiseFluctSpeed) - 0.5f));
		}

		if (m_Scaling != 1)
		{
			m_bApplyTransformation = true;
			if (m_Flags & EXPANDSPHERE_FLAGS_POSTSCALE)
			{
				for (int i = 0; i < 3; i++)
					for (int j = 0; j < 3; j++)
						m_TransformationMatrix.k[i][j] *= m_Scaling[j];
			}
			else
			{
				for (int i = 0; i < 3; i++)
					for (int j = 0; j < 3; j++)
						m_TransformationMatrix.k[j][i] *= m_Scaling[j];
			}
		}

		if (!(m_Flags & EXPANDSPHERE_FLAGS_WORLDROT))
		{
			CMat43fp32 TempMatrix;
			m_TransformationMatrix.Multiply3x3(_WMat, TempMatrix);
			m_TransformationMatrix = TempMatrix;
		}

		{
			CMat43fp32 TempMatrix;
			m_TransformationMatrix.Inverse3x3(TempMatrix);
			TempMatrix.Transpose3x3(m_NormalTransformationMatrix);
			CVec3Dfp32(0).SetMatrixRow(m_NormalTransformationMatrix, 3);
			m_NormalTransformationMatrix.RecreateMatrix(0, 1);
		}

		int NumVertices = (m_Segments - 1) * (m_Sides + 1) + 2 * m_Sides;
		int NumTriangles = m_Sides * (2 + (m_Segments - 2) * 2);
	
		if (!m_DVB.Create(this, m_pVBM, NumVertices, NumTriangles, 
						  DVBFLAGS_NORMALS | 
						  DVBFLAGS_TEXTURE | 
						  DVBFLAGS_COLORS | 
						  ((m_Flags & EXPANDSPHERE_FLAGS_FLATSHADE) ? DVBFLAGS_FLATSHADED : 0) |
						  ((m_Flags & EXPANDSPHERE_FLAGS_CULLSORT) ? DVBFLAGS_CULLSORT : 0)))
			return;

		m_DVB.GetVB()->m_Priority += m_RenderPriorityBias;

		m_SK.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface);

		GenerateExpandSphere();

		if (m_DVB.IsValid())
		{
			m_DVB.Render(_VMat);
			m_SK.Render(m_DVB.GetVB(), m_pVBM, m_pEngine);
			if (m_DVB.GetFlippedVB() != NULL)
				m_SK.Render(m_DVB.GetFlippedVB(), m_pVBM, m_pEngine);
		}
	}

	//----------------------------------------------------------------------
/*
	CVec3Dfp32 ParseVector3D(CStr _Vector, bool _bReplicate)
	{
		CVec3Dfp32 Vector;

		CFStr x, y, z;
		x = _Vector.GetStrSep(" ").Str(); x.Trim();
		y = _Vector.GetStrSep(" ").Str(); y.Trim();
		z = _Vector.GetStrSep(" ").Str(); z.Trim();

		Vector[0] = x.Val_fp64();

		if (_bReplicate)
		{
			if (y != "")
			{
				Vector[1] = y.Val_fp64();
				if (z != "")
				{
					Vector[2] = z.Val_fp64();
				}
				else
				{
					Vector[2] = 0;
				}
			}
			else
			{
				Vector[1] = Vector[0];
				Vector[2] = Vector[0];
			}
		}
		else
		{
			Vector[1] = y.Val_fp64();
			Vector[2] = z.Val_fp64();
		}

		// Verify that _Vector is empty, i.e. only contained three space separated tokens.
//		if (_Vector != "") return 0;

		return Vector;
	}

	//----------------------------------------------------------------------

	CVec2Dfp32 ParseVector2D(CStr _Vector, bool _bReplicate)
	{
		CVec2Dfp32 Vector;

		CFStr x, y;
		x = _Vector.GetStrSep(" ").Str(); x.Trim();
		y = _Vector.GetStrSep(" ").Str(); y.Trim();

		Vector[0] = x.Val_fp64();

		if (_bReplicate)
		{
			if (y != "")
			{
				Vector[1] = y.Val_fp64();
			}
			else
			{
				Vector[1] = Vector[0];
			}
		}
		else
		{
			Vector[1] = y.Val_fp64();
		}

		// Verify that _Vector is empty, i.e. only contained three space separated tokens.
//		if (_Vector != "") return 0;

		return Vector;
	}
*/
	//----------------------------------------------------------------------

	void OnEvalKey(const CRegistry *_pReg)
	{
		MAUTOSTRIP(CXR_Model_ExpandSphere_OnEvalKey, MAUTOSTRIP_VOID);
		CStr Name = _pReg->GetThisName();
		CStr Value = _pReg->GetThisValue();
		int Valuei = _pReg->GetThisValue().Val_int();
		fp32 Valuef = _pReg->GetThisValue().Val_fp64();

		if (Name == "SU")
		{
			CFStr SurfaceName = Value.GetStrMSep(" #");
			m_iSurface = GetSurfaceID(SurfaceName);
			m_RenderPriorityBias = Value.Val_fp64() * TransparencyPriorityBiasUnit;
		}

		else if (Name == "SEG")
			m_Segments = Valuei;
		else if (Name == "SID")
			m_Sides = Valuei;

		else if (Name == "DU")
			m_WantedDuration = Valuef;

		else if (Name == "RA0")
			m_RadiusStart = Valuef;
		else if (Name == "RA1")
			m_RadiusEnd = Valuef;

		else if (Name == "RO0")
			m_Rotation.ParseString(Value);
		else if (Name == "RO1")
			m_RotationNoise.ParseString(Value);
		else if (Name == "RO2")
			m_RotationNoiseFluctSpeed = Valuef;
		else if (Name == "RO3")
			m_RotationOffset.ParseString(Value);

		else if (Name == "SCL")
			m_Scaling.ParseString(Value);

		else if (Name == "TX0")
			m_TexOffset.ParseString(Value);
		else if (Name == "TX1")
			m_TexScale.ParseString(Value);
		else if (Name == "TX2")
			m_TexNoise.ParseString(Value);
		else if (Name == "TX3")
			m_TexNoiseFluctSpeed.ParseString(Value);

		else if (Name == "CO0")
			m_Color.ParseColor(Value);
		else if (Name == "CO1")
			m_ColorNoise.ParseColor(Value);
		else if (Name == "CO2")
			m_ColorTurbulence = Valuef;
		else if (Name == "CO3")
			m_ColorNoiseFluctSpeed = Valuef;

		else if (Name == "FT")
			m_FadeTime = Valuef;

		else if (Name == "FLG")
			m_Flags = Value.TranslateFlags(lpEXPANDSPHERE_FLAGS);

		else if (Name == "PTH")
			m_PoleThreshold = Valuef;

		else
			CXR_Model_Custom::OnEvalKey(_pReg);
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *_keys)
	{
		MAUTOSTRIP(CXR_Model_ExpandSphere_OnCreate, MAUTOSTRIP_VOID);
		m_Keys = _keys;
		SetDefaultParameters();
		ParseKeys(_keys);
		PreprocessParameters();
	}

	//----------------------------------------------------------------------

	void SetDefaultParameters()
	{
		MAUTOSTRIP(CXR_Model_ExpandSphere_SetDefaultParameters, MAUTOSTRIP_VOID);
		m_iSurface = GetSurfaceID("");
		m_RenderPriorityBias = 0;

		m_Segments = 10;
		m_Sides = 10;

		m_WantedDuration = 3.0f;

		m_RadiusStart = 10;
		m_RadiusEnd = 100;

		m_Rotation = 0;
		m_RotationOffset = 0.0f;
		m_RotationNoise = 0;
		m_RotationNoiseFluctSpeed = 0;

		m_Scaling = 1;

		m_TexOffset = 0;
		m_TexScale = 1;
		m_TexNoise = 0;
		m_TexNoiseFluctSpeed = 0;

		m_Color.ParseColor(CStr("0xFFFFFFFF"));
		m_ColorNoise.ParseColor(CStr("0x00000000"));
		m_ColorTurbulence = 0;
		m_ColorNoiseFluctSpeed = 0;

		m_FadeTime = 0;

		m_PoleThreshold = 0.9f;

		m_Flags = EXPANDSPHERE_FLAGS_CLAMPCOLORS | EXPANDSPHERE_FLAGS_FADEPOLES;
	}

	//----------------------------------------------------------------------

	void PreprocessParameters()
	{
		MAUTOSTRIP(CXR_Model_ExpandSphere_PreprocessParameters, MAUTOSTRIP_VOID);
		m_RenderPriorityBias += TransparencyPriorityBaseBias;
		m_Rotation *= 1.0f / 10.0f;
		m_RotationNoise *= 1.0f / 10.0f;
		m_RotationNoiseFluctSpeed *= 1.0f / 10.0f;
		m_TexNoise *= 1.0f / 10.0f;
		m_TexNoiseFluctSpeed *= 1.0f / 10.0f;
		m_ColorNoiseFluctSpeed *= 1.0f / 10.0f;
	}

	//----------------------------------------------------------------------

};

//----------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_ExpandSphere, CXR_Model_Custom);

//----------------------------------------------------------------------
