#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "CDynamicVB.h"
#include "CSurfaceKey.h"
#include "CSinRandTable.h"

//----------------------------------------------------------------------
/*
	Refraction of tiled buttom texture
	(Caustics of tiled caustics texture)
	Tint with surface texture
	Reflection of env cubemap texture
	Alpha blend of white breaking texture

	Vertex
		Pos
		Normal

	Pixel
		State0.A = 1.0
		State0.RGB = Lerp(BottomTex.RGB, SurfaceTex.RGB, SurfaceTex.A)
		State1.RGB = Lerp(State0.RGB, SurfaceTex.A)



*/
//----------------------------------------------------------------------
/*
	Parameters:

		fp32 TimeScale
		fp32 AmplitudeMin
		fp32 AmplitudeMax
		fp32 Sharpness 
		fp32 WindStrength
		fp32 WindDirection
		fp32 IndexOfRefraction (defines refraction per angle of incident)
		fp32 IndexOfReflection (defines reflection per angle of incident)
		fp32 FresnellTransparency (defines transparency per angle of incident)
		int iRefractionSurface
		int iReflectionSurface
		uint32 Color
		CVec3Dfp32 TexScale
		CVec3Dfp32 TexOffset

*/
//----------------------------------------------------------------------
/*
Refraction:

	iAir = 1.00029f
	iWater = 1.33f
	AngleOut = asin(iAir / iWater * sin(AngleIn))
*/
//----------------------------------------------------------------------
/*
*/
//----------------------------------------------------------------------

#define PERLINCACHEROWS		64
#define PERLINCACHECOLS		64
#define PERLINCACHESIZE		(PERLINCACHEROWS * PERLINCACHECOLS)

//----------------------------------------------------------------------

const char* lpWATER_FLAGS[] = { "reflect", "refract", "fresnell" };
enum
{
	WATER_FLAGS_ = 0x0001,
};

//----------------------------------------------------------------------

class CXR_Model_Water : public CXR_Model_Custom
{

	MRTC_DECLARE;

public:

private:

	//----------------------------------------------------------------------

	CStr				m_Keys;

	int					m_iSurface;
	fp32					m_RenderPriorityBias;

	int					m_Flags;

	uint32				m_nSizeX, m_nSizeY;
	fp32					m_SizeX, m_SizeY;

	fp32					m_TimeScale;
	fp32					m_SurfaceHeight;
	fp32					m_WaveAmp;
	fp32					m_BottomHeight;
	fp32					m_Sharpness;
	fp32					m_WindStrength;
	fp32					m_WindDirection;

	uint32				m_ColorRGB;

	CVec2Dfp32			m_SurfaceTexOffset;
	CVec2Dfp32			m_SurfaceTexScale;
	CVec2Dfp32			m_BottomTexOffset;
	CVec2Dfp32			m_BottomTexScale;

	fp32					m_Refraction;

	struct CPerRenderStatics
	{
		fp32					m_Time;
		fp32					m_Duration;
		int					m_Randseed, m_RandseedBase;

		CWireContainer*		m_pWC;

		CVec3Dfp32*			m_pPerlinCache;
		CVec3Dfp32*			m_pPerlinCacheRows[PERLINCACHEROWS];
		uint8				m_iPerlinCacheRowShift;
		int					m_PerlinCacheX, m_PerlinCacheY;

		CVec3Dfp32			m_CameraPos;
		CVec3Dfp32			m_CameraFwd;

		CVec3Dfp32			m_Center;

		int					m_nVertices;
		int					m_nTriangles;

		CSurfaceKey			m_SK;
		CXR_VertexBuffer*	m_pVB;
		CVec3Dfp32*			m_pVertexPos;
		CVec2Dfp32*			m_pVertexTex0;
		CVec2Dfp32*			m_pVertexTex1;
		CVec3Dfp32*			m_pVertexNormal;
		CPixel32*			m_pVertexColor;
		uint16*				m_pIndex;
		CMat4Dfp32*			m_pMatrix;

	} m_PRS;

	//----------------------------------------------------------------------

	fp32 GetWaveOffset(fp32 _x, fp32 _y, fp32 _dx, fp32 _dy)
	{
		CVec2Dfp32 Pos(_x, _y);
		CVec2Dfp32 Dir(_dx, _dy); Dir.Normalize();
		return (Dir[0] * Pos[0] + Dir[1] * Pos[1]);
	}

	//----------------------------------------------------------------------

	fp32 GetWaveAmp(fp32 _offset, fp32 _wavelength, fp32 _time)
	{
		return M_Sin((_offset / _wavelength + _time) * (_PI * 2));
	}

	//----------------------------------------------------------------------

	fp32 PerlinNoise3D(fp32 _x, fp32 _y, fp32 _time)
	{
		fp32 Amp0 = GetWaveAmp(GetWaveOffset(_x, _y, 4, 1), 2000, _time * 0.4f);
		fp32 Amp1 = GetWaveAmp(GetWaveOffset(_x, _y, 2, 1), 1000, _time * 0.4f);
		fp32 Amp2 = GetWaveAmp(GetWaveOffset(_x, _y, 5, 2), 400, _time * 0.3f);
		fp32 Amp3 = GetWaveAmp(GetWaveOffset(_x, _y, 5, 1), 150, _time * 0.3f);
		fp32 Amp4 = GetWaveAmp(GetWaveOffset(_x, _y, 1, 6), 400, _time * 0.3f);
		fp32 Amp5 = GetWaveAmp(GetWaveOffset(_x, _y, 1, 5), 120, _time * 0.6f);
		fp32 Amp6 = GetWaveAmp(GetWaveOffset(_x, _y, 1, 4), 120, _time * 0.8f);
		fp32 Amp7 = GetWaveAmp(GetWaveOffset(_x, _y, 3, 2), 80, _time * 1.0f);
		fp32 Amp8 = GetWaveAmp(GetWaveOffset(_x, _y, 8, 4), 300, _time * 0.5f);
		fp32 Amp9 = GetWaveAmp(GetWaveOffset(_x, _y, 4, 8), 200, _time * 0.3f);
		fp32 Amp = 0;
		Amp += 0.4f * Amp0 * Amp1;
		Amp += 0.3f * Amp2 * Amp3;
		Amp += 0.2f * Amp4 * Amp5 * Amp6;
		Amp += 0.1f * Amp7;
		Amp = LERP(Amp, Amp * Amp8, 0.3f);
		Amp = LERP(Amp, Amp * Amp9, 0.3f);
		//Amp = Amp * Amp;
		return Amp;
	}

	//----------------------------------------------------------------------

	int GetVertexIndex(int _x, int _y)
	{
		return (_x + _y * (m_nSizeX + 1));
	}

	//----------------------------------------------------------------------

	int GetTriangleIndex(int _x, int _y)
	{
		return ((_x + _y * m_nSizeX) * 2);
	}

	//----------------------------------------------------------------------

	void GenerateCachePos(int _x, int _y)
	{
		CVec3Dfp32& Pos = GetCachedPos(_x, _y);
		Pos[0] = m_PRS.m_Center[0] + ((fp32(_x) / fp32(m_nSizeX + 1)) -0.5) * m_SizeX;
		Pos[1] = m_PRS.m_Center[1] + ((fp32(_y) / fp32(m_nSizeY + 1)) -0.5) * m_SizeY;
		Pos[2] = m_PRS.m_Center[2];
		Pos[2] += m_SurfaceHeight + m_WaveAmp * (PerlinNoise3D(Pos[0], Pos[1], m_PRS.m_Time));
	}

	//----------------------------------------------------------------------

	CVec3Dfp32& GetCachedPos(int _x, int _y)
	{
		int x = _x - m_PRS.m_PerlinCacheX;
		int y = _y - m_PRS.m_PerlinCacheY;
		return (m_PRS.m_pPerlinCacheRows[y][x]);
	}

	//----------------------------------------------------------------------

	void GenerateVertex(int _x, int _y)
	{
		int iVertex = GetVertexIndex(_x, _y);

		CVec3Dfp32 Pos = GetCachedPos(_x, _y);
		CVec2Dfp32 BottomTex;
		CVec2Dfp32 SurfaceTex;
		CVec3Dfp32 Normal;
		CVec4Dfp32 Color;

		CVec2Dfp32 Sharpness;
		Sharpness[0] = m_Sharpness;
		Sharpness[1] = m_Sharpness;

		CVec2Dfp32 Slope;
		if ((_x > 0) && (_x < m_nSizeX))
		{
			CVec3Dfp32& PX1 = GetCachedPos(_x - 1, _y);
			CVec3Dfp32& PX2 = GetCachedPos(_x + 1, _y);
			Slope[0] = PX2[2] - PX1[2];
			Pos[0] += Slope[0] * Sharpness[0];
		}
		else
			Slope[0] = 0;

		if ((_y > 0) && (_y < m_nSizeY))
		{
			CVec3Dfp32& PY1 = GetCachedPos(_x, _y - 1);
			CVec3Dfp32& PY2 = GetCachedPos(_x, _y + 1);
			Slope[1] = PY2[2] - PY1[2];
			Pos[1] += Slope[1] * Sharpness[1];
		}
		else
			Slope[1] = 0;

		fp32 Amp = (Pos[2] - (m_PRS.m_Center[2] + m_SurfaceHeight)) / m_WaveAmp;
		fp32 Wind = Amp * Amp * 10.0f;
		Pos[0] -= Wind;
		Pos[1] -= Wind;
		Pos[2] -= Wind;
		
		CVec2Dfp32 RefractOffset = 0;
		fp32 Depth = (Pos[2] - (m_PRS.m_Center[2] + m_BottomHeight));
		CVec3Dfp32 IncidentOffset = Pos - m_PRS.m_CameraPos;
		RefractOffset[0] += (IncidentOffset[0] / IncidentOffset[2]) * 1.0f;
		RefractOffset[1] += (IncidentOffset[1] / IncidentOffset[2]) * 1.0f;
		RefractOffset[0] += Slope[0] * 0.2f;
		RefractOffset[1] += Slope[0] * 0.2f;
		RefractOffset *= Depth;
		RefractOffset *= m_Refraction;
		
		BottomTex[0] = ((Pos[0] - m_PRS.m_Center[0] + RefractOffset[0]) + m_BottomTexOffset[0]) * m_BottomTexScale[0];
        BottomTex[1] = ((Pos[1] - m_PRS.m_Center[1] + RefractOffset[1]) + m_BottomTexOffset[1]) * m_BottomTexScale[1];
		
        SurfaceTex[0] = ((Pos[0] - m_PRS.m_Center[0]) + m_SurfaceTexOffset[0]) * m_SurfaceTexScale[0];
        SurfaceTex[1] = ((Pos[1] - m_PRS.m_Center[1]) + m_SurfaceTexOffset[1]) * m_SurfaceTexScale[1];

		Normal = CVec3Dfp32(Slope[0], Slope[1], 1); Normal.Normalize();
		Color[0] = 128;
		Color[1] = 196;
		Color[2] = 255;
		Color[3] = 255;
		
		m_PRS.m_pVertexPos[iVertex] = Pos;
		m_PRS.m_pVertexTex0[iVertex] = BottomTex;
		m_PRS.m_pVertexTex1[iVertex] = SurfaceTex;
		m_PRS.m_pVertexNormal[iVertex] = Normal;
		m_PRS.m_pVertexColor[iVertex] = CPixel32(Color[0], Color[1], Color[2], Color[3]);
	}

	//----------------------------------------------------------------------

	void GenerateWater()
	{
		for (int y = 0; y < (m_nSizeY + 1); y++)
			for (int x = 0; x < (m_nSizeX + 1); x++)
				GenerateCachePos(x, y);

		for (int y = 0; y < (m_nSizeY + 1); y++)
			for (int x = 0; x < (m_nSizeX + 1); x++)
				GenerateVertex(x, y);

		for (int y = 0; y < m_nSizeY; y++)
		{
			for (int x = 0; x < m_nSizeX; x++)
			{
				int iTriangle = GetTriangleIndex(x, y);
				int iVertex0 = GetVertexIndex(x, y);
				int iVertex1 = GetVertexIndex(x + 1, y);
				int iVertex2 = GetVertexIndex(x, y + 1);
				int iVertex3 = GetVertexIndex(x + 1, y + 1);
				m_PRS.m_pIndex[iTriangle * 3 + 0] = iVertex0;
				m_PRS.m_pIndex[iTriangle * 3 + 1] = iVertex2;
				m_PRS.m_pIndex[iTriangle * 3 + 2] = iVertex1;
				m_PRS.m_pIndex[iTriangle * 3 + 3] = iVertex1;
				m_PRS.m_pIndex[iTriangle * 3 + 4] = iVertex2;
				m_PRS.m_pIndex[iTriangle * 3 + 5] = iVertex3;
			}
		}
	}

	//----------------------------------------------------------------------

	bool SetupBuffers()
	{
			m_PRS.m_pVB = AllocVB();
			if (m_PRS.m_pVB == NULL)
				return false;

			m_PRS.m_pVertexPos = m_pVBM->Alloc_V3(m_PRS.m_nVertices);
			if (m_PRS.m_pVertexPos == NULL)
				return false;

			m_PRS.m_pVertexTex0 = m_pVBM->Alloc_V2(m_PRS.m_nVertices);
			if (m_PRS.m_pVertexTex0 == NULL)
				return false;

			m_PRS.m_pVertexTex1 = m_pVBM->Alloc_V2(m_PRS.m_nVertices);
			if (m_PRS.m_pVertexTex1 == NULL)
				return false;

			m_PRS.m_pVertexNormal = m_pVBM->Alloc_V3(m_PRS.m_nVertices);
			if (m_PRS.m_pVertexNormal == NULL)
				return false;

			m_PRS.m_pVertexColor = m_pVBM->Alloc_CPixel32(m_PRS.m_nVertices);
			if (m_PRS.m_pVertexColor == NULL)
				return false;

			m_PRS.m_pIndex = m_pVBM->Alloc_Int16(m_PRS.m_nTriangles * 3);
			if (m_PRS.m_pIndex == NULL)
				return false;

			m_PRS.m_pMatrix = m_pVBM->Alloc_M4();
			if(m_PRS.m_pMatrix == NULL)
				return false;

			if (!m_PRS.m_pVB->SetVBChain(m_pVBM, false))
				return false;
			m_PRS.m_pVB->Geometry_VertexArray(m_PRS.m_pVertexPos, m_PRS.m_nVertices);
			m_PRS.m_pVB->Geometry_TVertexArray(m_PRS.m_pVertexTex0, 0);
			m_PRS.m_pVB->Geometry_TVertexArray(m_PRS.m_pVertexTex1, 1);
			m_PRS.m_pVB->Geometry_ColorArray(m_PRS.m_pVertexColor);
			m_PRS.m_pVB->Geometry_ColorNormal(m_PRS.m_pVertexNormal);
			m_PRS.m_pVB->Matrix_Set(m_PRS.m_pMatrix);
			m_PRS.m_pVB->m_Priority -= TransparencyPriorityBiasUnit;
			m_PRS.m_pVB->m_Priority += m_RenderPriorityBias;

			return true;
	}

	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat4Dfp32 &_WMat, const CMat4Dfp32 &_VMat)
	{
		m_PRS.m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));

		m_PRS.m_Time = _pAnimState->m_AnimTime0 * m_TimeScale;
		m_PRS.m_Duration = _pAnimState->m_AnimTime1;

		m_PRS.m_RandseedBase = _pAnimState->m_Anim0;
		m_PRS.m_Randseed = m_PRS.m_RandseedBase;

		m_PRS.m_nVertices = (m_nSizeX + 1) * (m_nSizeY + 1);
		m_PRS.m_nTriangles = m_nSizeX * m_nSizeY * 2;

		m_PRS.m_Center = CVec3Dfp32::GetMatrixRow(_WMat, 3);

		CMat4Dfp32 InvWorldToCamera;
		_VMat.Inverse(InvWorldToCamera);
		m_PRS.m_CameraPos = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 3);
		m_PRS.m_CameraFwd = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 2);

		if (!SetupBuffers())
			return;

		m_PRS.m_SK.Create(GetSurfaceContext(), m_pEngine, _pAnimState, m_iSurface);

		CVec3Dfp32 PerlinCache[64 * 64];
		m_PRS.m_pPerlinCache = (CVec3Dfp32*)&PerlinCache;
		for (int row = 0; row < PERLINCACHEROWS; row++)
			m_PRS.m_pPerlinCacheRows[row] = &(m_PRS.m_pPerlinCache[row * PERLINCACHECOLS]);
		m_PRS.m_PerlinCacheX = 0;
		m_PRS.m_PerlinCacheY = 0;

		GenerateWater();

		CMat4Dfp32 WorldToCamera;
		_WMat.Multiply(_VMat, WorldToCamera);
		*m_PRS.m_pMatrix = WorldToCamera;
		m_PRS.m_pVB->Render_IndexedTriangles(m_PRS.m_pIndex, m_PRS.m_nTriangles);
		m_PRS.m_SK.Render(m_PRS.m_pVB, m_pVBM, m_pEngine);
	}

	//----------------------------------------------------------------------

	void OnEvalKey(const CRegistry *_pReg)
	{
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

		else if (Name == "FLG")
			m_Flags = Value.TranslateFlags(lpWATER_FLAGS);

		else
			CXR_Model_Custom::OnEvalKey(_pReg);
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *_keys)
	{
		m_Keys = _keys;
		SetDefaultParameters();
		ParseKeys(_keys);
		PreprocessParameters();
	}

	//----------------------------------------------------------------------

	void SetDefaultParameters()
	{
		m_iSurface = GetSurfaceID("WaterTest");
		m_RenderPriorityBias = 0;

		m_Flags = 0;

		m_nSizeX = 60;
		m_nSizeY = 60;
		m_SizeX = 500;
		m_SizeY = 500;

		m_TimeScale = 1.0f;

		m_SurfaceHeight = 0;
		m_WaveAmp = 50;
		m_BottomHeight = -100;
		m_Sharpness = 0.6f;
		m_WindStrength = 0;
		m_WindDirection = 0;

		m_ColorRGB = 255;

		m_SurfaceTexOffset = 0;
		m_SurfaceTexScale = 1.0f;
		m_BottomTexOffset = 0;
		m_BottomTexScale = 1.0f / 800.0f;

		m_Refraction = 0.07f;
	}

	//----------------------------------------------------------------------

	void PreprocessParameters()
	{
		m_RenderPriorityBias += TransparencyPriorityBaseBias;
	}

	//----------------------------------------------------------------------

};

//----------------------------------------------------------------------

MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Water, CXR_Model_Custom);

//----------------------------------------------------------------------
