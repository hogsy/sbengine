#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

#include "CEnvelope.h"

//----------------------------------------------------------------------

#define getRand()		(MFloat_GetRand(m_Randseed++))
#define getSign(x)		(Sign(x))
#define getAbs(x)		(((x) < 0) ? -(x) : (x))
#define InvSqr(x)		(1.0f - (1.0f - (x)) * (1.0f - (x)))


//----------------------------------------------------------------------
// CXR_Model_ShockWave
//----------------------------------------------------------------------

class CXR_Model_ShockWave : public CXR_Model_Custom
{

	MRTC_DECLARE;

protected:

	int32 m_iSurface;

public:

	enum { SinRandTableSize = 1024 };
	enum { NumSides = 10, NumSegments = 5 };

private:

	//----------------------------------------------------------------------

	fp32				m_Time; // Total lifetime of model.
	fp32				m_Duration; // Total duration of model. -1 = infinite.
	fp32				m_TimeFraction; // Fraction of time from 0 to duration.
	fp32				m_Distance; // Distance from camera to model origin.
	int32			m_Randseed, m_RandseedBase;

	fp32				m_SinRandTable[SinRandTableSize];
	fp32				m_SinRandTimeOffset;

	CVec3Dfp32		m_ViewDir;
	CVec3Dfp32		m_Origin, m_Dir;
	fp32				m_TravelLength; // Distance from origin to target.
	uint32			m_ColorRGB, m_ColorAlpha;

	CEnvelope		m_PosEnv, m_RadiusEnv, m_LengthEnv, m_Alpha0Env, m_Alpha1Env, m_TV0Env, m_TV1Env;

	int32			m_iVertex, m_iIndex;
	CVec3Dfp32*		m_pVertexPos;
	CVec2Dfp32*		m_pVertexTex;
	CPixel32*		m_pVertexColor;
	uint16*			m_pIndex;

#ifndef	M_RTM
	CWireContainer*	m_pWC;
#endif

	//----------------------------------------------------------------------

	void
	createSinRandTable()
	{
		MAUTOSTRIP(CXR_Model_ShockWave_createSinRandTable, MAUTOSTRIP_VOID);
		fp32 pi2 = 2.0f * _PI;
		fp32 x, y, dy;

		for (int i = 0; i < SinRandTableSize; i++) {
			x = (fp32)i / (fp32)SinRandTableSize;
			y = 0;

			dy = M_Sin(3.0f * pi2 * (x + 0.0f));
			if (dy < -1.0f) dy = -1.0f;
			if (dy > +1.0f) dy = +1.0f;
			y += 0.5f * dy;

			dy = M_Sin(7.0f * pi2 * (x + 0.3f));
			if (dy < -1.0f) dy = -1.0f;
			if (dy > +1.0f) dy = +1.0f;
			y += 0.25f * dy;

			dy = M_Sin(13.0f * pi2 * (x + 0.7f));
			if (dy < -1.0f) dy = -1.0f;
			if (dy > +1.0f) dy = +1.0f;
			y += 0.125f * dy;

			m_SinRandTable[i] = 0.5f + 0.5f * (y / 0.875f);
		}
	}

	//----------------------------------------------------------------------

	fp32
	getSinRandTable(fp32 timescale)
	{
		MAUTOSTRIP(CXR_Model_ShockWave_getSinRandTable, 0.0f);
		fp32 y, y1, y2;
		int32 x, xi, xf;

		x = RoundToInt(1023.0f * 255.0f * (m_Time + m_SinRandTimeOffset * getRand()) * timescale);

		xi = (x >> 8) & 0x3FF;
		xf = x & 0xFF;

		y1 = m_SinRandTable[xi];
		y2 = m_SinRandTable[(xi + 1) & 0x3FF];

		y = y1 + (y2 - y1) * (fp32)xf / 255.0f;

		return y;
	}

	//----------------------------------------------------------------------

	inline
	void
	addVertex(CVec3Dfp32 pos, fp32 tu, fp32 tv, fp32 alpha)
	{
		MAUTOSTRIP(CXR_Model_ShockWave_addVertex, MAUTOSTRIP_VOID);
		m_pVertexPos[m_iVertex] = pos;
		m_pVertexTex[m_iVertex][0] = tu;
		m_pVertexTex[m_iVertex][1] = tv;
		m_pVertexColor[m_iVertex] = m_ColorRGB + ((int32)(alpha * (fp32)m_ColorAlpha) << 24);
		m_iVertex++;
	}

	//----------------------------------------------------------------------

	inline
	void
	addTriangle(int32 v1, int32 v2, int32 v3)
	{
		MAUTOSTRIP(CXR_Model_ShockWave_addTriangle, MAUTOSTRIP_VOID);
		m_pIndex[m_iIndex + 0] = v1;
		m_pIndex[m_iIndex + 1] = v2;
		m_pIndex[m_iIndex + 2] = v3;
		m_iIndex += 3;
	}

	//----------------------------------------------------------------------

	void
	generateShield(CVec3Dfp32 basepos, fp32 radius, fp32 length, fp32 width, fp32 alpha0, fp32 alpha1, fp32 tv0, fp32 tv1, const CMat43fp32& LocalToCamera)
	{
		MAUTOSTRIP(CXR_Model_ShockWave_generateShield, MAUTOSTRIP_VOID);
		fp32 pi = _PI;
		fp32 pi2 = 2.0f * _PI;

		int numVerticesPerSide = (NumSegments + 1) * 2;
		int numIndicesPerSide = (NumSegments * 6);

		int iSide, iSegment;
		int iVertexBase, iIndexBase;
		fp32 sidefraction, segmentfraction;

		CVec3Dfp32 pos, normal;
		fp32 ta, tr, tu, tv, alpha;

		normal.k[0] = 1.0f;
		normal.k[1] = 0.0f;
		normal.k[2] = 0.0f;
		normal.MultiplyMatrix3x3(LocalToCamera);

		fp32 frontview = getAbs(normal.k[2]);
		frontview = frontview * frontview;
		frontview = frontview * frontview;

		for (iSide = 0; iSide < NumSides; iSide++) {
			sidefraction = (fp32)iSide / (fp32)(NumSides - 1);

			iVertexBase = iSide * numVerticesPerSide;
			iIndexBase = iSide * numIndicesPerSide;

			for (iSegment = 0; iSegment <= NumSegments; iSegment++) {
				segmentfraction = (fp32)iSegment / (fp32)(NumSegments);

				fp32 baseangle = ((fp32)iSide / (fp32)NumSides);
				fp32 halfwidthangle = width * (0.5f / (fp32)NumSides);
				tr = tv1 + segmentfraction * (tv0 - tv1);

				normal.k[0] = M_Cos(segmentfraction * 0.5f * pi);
				normal.k[1] = M_Sin(segmentfraction * 0.5f * pi) * M_Sin((baseangle - halfwidthangle) * pi2);
				normal.k[2] = M_Sin(segmentfraction * 0.5f * pi) * M_Cos((baseangle - halfwidthangle) * pi2);

				pos.k[0] = normal.k[0] * length;
				pos.k[1] = normal.k[1] * radius;
				pos.k[2] = normal.k[2] * radius;

				normal.MultiplyMatrix3x3(LocalToCamera);

				if (normal.Length() > 1.0f) {
					int x = 0;
				}

//				alpha = (1.0f - frontview) + (1.0f - segmentfraction) * (1.0f - segmentfraction) * frontview;
//				alpha = (1.0f - frontview) + (1.0f - segmentfraction) * frontview;
				alpha = alpha1 + segmentfraction * (alpha0 - alpha1);
				alpha *= 1.0f - (1.0f - frontview) * getAbs(normal.k[2]);
				alpha = alpha * alpha;

				ta = (baseangle - halfwidthangle);

				tu = 0.5f + 0.5f * tr * M_Cos(ta * pi2);
				tv = 0.5f + 0.5f * tr * M_Sin(ta * pi2);

				addVertex(basepos + pos, tu, tv, alpha);

				normal.k[0] = M_Cos(segmentfraction * 0.5f * pi);
				normal.k[1] = M_Sin(segmentfraction * 0.5f * pi) * M_Sin((baseangle + halfwidthangle) * pi2);
				normal.k[2] = M_Sin(segmentfraction * 0.5f * pi) * M_Cos((baseangle + halfwidthangle) * pi2);

				pos.k[0] = normal.k[0] * length;
				pos.k[1] = normal.k[1] * radius;
				pos.k[2] = normal.k[2] * radius;

				normal.MultiplyMatrix3x3(LocalToCamera);

//				alpha = (1.0f - frontview) + (1.0f - segmentfraction) * (1.0f - segmentfraction) * frontview;
//				alpha = (1.0f - frontview) + (1.0f - segmentfraction) * frontview;
				alpha = alpha1 + segmentfraction * (alpha0 - alpha1);
				alpha *= 1.0f - (1.0f - frontview) * getAbs(normal.k[2]);
				alpha = alpha * alpha;

				ta = (baseangle + halfwidthangle);

				tu = 0.5f + 0.5f * tr * M_Cos(ta * pi2);
				tv = 0.5f + 0.5f * tr * M_Sin(ta * pi2);

				addVertex(basepos + pos, tu, tv, alpha);
			}

			int v0, v1, v2, v3;
			for (iSegment = 0; iSegment < NumSegments; iSegment++) {
				v0 = iVertexBase + iSegment * 2;
				v1 = v0 + 1;
				v2 = v0 + 2;
				v3 = v0 + 3;
				addTriangle(v0, v1, v3);
				addTriangle(v0, v3, v2);
			}
		}
	}

	//----------------------------------------------------------------------

	void
	TestEnvelope()
	{
		MAUTOSTRIP(CXR_Model_ShockWave_TestEnvelope, MAUTOSTRIP_VOID);
		int steps = 1000;
		fp32 time, amp;
		int i;
		for (i = 0; i < steps; i++) {
			time = (fp32)i / (fp32)(steps - 1);
			time = -1.0f + 3.0f * time;
			amp = m_PosEnv.getTCBSpline(time);
			addVertex(CVec3Dfp32(100,0,0) * time + CVec3Dfp32(0, 0, 100) * amp, 0.5f, 0.5f, 1.0f);
		}

		for (i = 0; i < (steps - 1); i++)
			addTriangle(i, i, i+1);
	}

	//----------------------------------------------------------------------

	void
	generateShockwave(const CMat43fp32& LocalToCamera)
	{
		MAUTOSTRIP(CXR_Model_ShockWave_generateShockwave, MAUTOSTRIP_VOID);
		TestEnvelope();
		return;


		fp32 pos, radius, length, alpha0, alpha1, tv0, tv1;

//		pos = m_PosEnv.getValue(m_TimeFraction);
//		radius = m_RadiusEnv.getValue(m_TimeFraction);
//		length = m_LengthEnv.getValue(m_TimeFraction);

//		pos = 0;
//		radius = 10;
//		length = 10;

/*
		alpha0 = m_Alpha0Env.getValue(m_TimeFraction);
		alpha1 = m_Alpha1Env.getValue(m_TimeFraction);
		tv0 = m_TV0Env.getValue(m_TimeFraction);
		tv1 = m_TV1Env.getValue(m_TimeFraction);
*/
		alpha0 = 1.0f;
		alpha1 = 1.0f;
		tv0 = 1.0f;
		tv1 = 0.0f;

		generateShield(CVec3Dfp32(1, 0, 0) * pos, radius, length, 1.0f, alpha0, alpha1, tv0, tv1, LocalToCamera);
	}

	//----------------------------------------------------------------------

	void
	fillVB(const CXR_AnimState* pAnimState, 
		   const CMat43fp32& LocalToWorld, const CMat43fp32& WorldToCamera)
	{
		MAUTOSTRIP(CXR_Model_ShockWave_fillVB, MAUTOSTRIP_VOID);
		m_Time = pAnimState->m_AnimTime0;
		m_Duration = (pAnimState->m_AnimTime1 > 0.0f) ? pAnimState->m_AnimTime1 : 100.0f;
		m_RandseedBase = pAnimState->m_Anim0;
		m_Randseed = m_RandseedBase;

		if (m_Time > m_Duration)
			return;

		m_TimeFraction = m_Time / m_Duration;

		m_TravelLength = 300.0f;

		m_BoundRadius = m_TravelLength;

		m_SinRandTimeOffset = 1.0f;

		fp32 p = M_Sin(m_Time);
		fp32 t = 2.0f * p;
		fp32 d = 2.0f * p;
		fp32 b = 2.0f * p;

		CEnvelope::Key poskeys[] = 
		{
/*
			CEnvelope::Key(0.00f, 0.20f, t, d, b), 
			CEnvelope::Key(0.10f, 0.00f, t, d, b), 
			CEnvelope::Key(0.20f, 0.00f, t, d, b),
			CEnvelope::Key(0.30f, 0.20f, t, d, b), 
			CEnvelope::Key(0.40f, 0.00f, t, d, b), 
			CEnvelope::Key(0.50f, 0.90f, t, d, b), 
			CEnvelope::Key(0.60f, 0.70f, t, d, b), 
			CEnvelope::Key(0.70f, 0.90f, t, d, b), 
			CEnvelope::Key(0.80f, 0.70f, t, d, b), 
			CEnvelope::Key(0.90f, 0.90f, t, d, b), 
			CEnvelope::Key(1.00f, 0.70f, t, d, b)
*/

			CEnvelope::Key(0.00f, 0.00f, 0, 0, 0), 
			CEnvelope::Key(0.05f, 0.40f, 0, 0, 0), 
			CEnvelope::Key(0.10f, 0.70f, 0, 0, 0), 
			CEnvelope::Key(0.20f, 0.90f, 0, 0, 0), 
			CEnvelope::Key(0.50f, 1.00f, 0, 0, 0),
			CEnvelope::Key(1.00f, 1.00f, 0, 0, 0)
/*
			CEnvelope::Key(0.00f, 0.00f, t, 0, 0), 
			CEnvelope::Key(0.05f, 0.50f, t, 0, 0), 
			CEnvelope::Key(0.55f, 1.00f, t, 0, 0),
			CEnvelope::Key(1.00f, 1.00f, t, 0, 0)
*/
		};

/*
		CEnvelope::Key radkeys[] = 
		{
			CEnvelope::Key(0.0f, 40.0), 
			CEnvelope::Key(0.3f, 30.0), 
			CEnvelope::Key(0.9f, 30.0), 
			CEnvelope::Key(1.0f, 50.0)
		};

		CEnvelope::Key lenkeys[] = 
		{
			CEnvelope::Key(0.0f, 10.0), 
			CEnvelope::Key(0.3f, 30.0), 
			CEnvelope::Key(0.9f, 30.0), 
			CEnvelope::Key(1.0f, 0.0)
		};
*/
		m_PosEnv.Create(poskeys, sizeof(poskeys) / sizeof(CEnvelope::Key), CEnvelope::IPMethod_TCBSpline);

#ifndef	M_RTM
		m_PosEnv.Render(m_pWC, 200.0f, 0xFF00FF00, 0xFF101010, 1.0f);
#endif
		return;

//		m_PosEnv.Create(poskeys, sizeof(poskeys) / sizeof(CEnvelope::Key), CEnvelope::IPMethod_Linear);
/*
		m_RadiusEnv.Create(radkeys, sizeof(radkeys) / sizeof(CEnvelope::Key), CEnvelope::IPMethod_TCBSpline);
		m_LengthEnv.Create(lenkeys, sizeof(lenkeys) / sizeof(CEnvelope::Key), CEnvelope::IPMethod_TCBSpline);
*/
		int numVertices = NumSides * ((NumSegments + 1) * 2);
		int numTriangles = NumSides * (NumSegments * 2);

		numVertices = 1001;
		numTriangles = 1000;

		CMat43fp32 LocalToCamera;
		LocalToWorld.Multiply(WorldToCamera, LocalToCamera);

		m_Distance = LocalToCamera.k[3][2];

/*
		CMat4Dfp32 InvWorldToCamera;
		if (!WorldToCamera.Inverse3x3(InvWorldToCamera))
			return;
*/
//		m_ViewDir = CVec3Dfp32::GetMatrixRow(InvWorldToCamera, 0);
		m_ViewDir = CVec3Dfp32(1, 0, 0);

		m_Origin = CVec3Dfp32::GetMatrixRow(LocalToWorld, 3);
		m_Dir = CVec3Dfp32::GetMatrixRow(LocalToWorld, 0);

		CXR_VertexBuffer* pVB = AllocVB();
		if (pVB == NULL)
			return;

		m_pVertexPos = m_pVBM->Alloc_V3(numVertices);
		if (!m_pVertexPos)
			return;

		m_pVertexTex = m_pVBM->Alloc_V2(numVertices);
		if (!m_pVertexTex)
			return;

		m_pVertexColor = m_pVBM->Alloc_CPixel32(numVertices);
		if (!m_pVertexColor)
			return;

		m_pIndex = m_pVBM->Alloc_Int16(3 * numTriangles);
		if (!m_pIndex)
			return;

		CMat4Dfp32 *pMatrix = m_pVBM->Alloc_M4(LocalToCamera);
		if(!pMatrix)
			return;

		m_iVertex = 0;
		m_iIndex = 0;

		CXW_Surface *pSurface = GetSurfaceContext()->GetSurfaceVersion(m_iSurface, m_pEngine);
		CXW_SurfaceKeyFrame *pKeyFrame = pSurface->GetFrame(0, pAnimState->m_AnimTime0, *GetSurfaceContext()->m_spTempKeyFrame);
		m_ColorRGB = pKeyFrame->m_Medium.m_Color & 0x00FFFFFF;
		m_ColorAlpha = ((pKeyFrame->m_Medium.m_Color >> 24) & 0xFF);

		generateShockwave(LocalToCamera);

		ConOut(CStrF("numVertices = %d, numTriangles = %d", m_iVertex, m_iIndex / 3));
		numVertices = m_iVertex;
		numTriangles = m_iIndex / 3;

		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
		pVB->Matrix_Set(pMatrix);
		if (!pVB->SetVBChain(m_pVBM, false))
			return false;
		pVB->Geometry_VertexArray(m_pVertexPos, numVertices, true);
		pVB->Geometry_TVertexArray(m_pVertexTex, 0);
		pVB->Geometry_ColorArray(m_pVertexColor);
		pVB->Render_IndexedTriangles(m_pIndex, numTriangles);

		int Flags = RENDERSURFACE_DEPTHFOG;// | RENDERSURFACE_VERTEXFOG;
		if(pVB->m_pTransform[0])
			Flags |= RENDERSURFACE_MATRIXSTATIC_M2V;

		CXR_Util::Render_Surface(Flags, pSurface, pKeyFrame, m_pEngine, m_pVBM, NULL, NULL, pVB->m_pTransform[0], pVB, pVB->m_Priority, 0.0001f);
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *surface)
	{
		MAUTOSTRIP(CXR_Model_ShockWave_OnCreate, MAUTOSTRIP_VOID);
		if (surface != NULL) 
			m_iSurface = GetSurfaceID(surface);
		else
			m_iSurface = GetSurfaceID("ShockWave4");

//		m_iSurface = 0;

		createSinRandTable();

	}

	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_ShockWave_Render, MAUTOSTRIP_VOID);
#ifndef	M_RTM
		m_pWC = (CWireContainer*)(CReferenceCount*)(MRTC_GOM()->GetRegisteredObject("GAMECONTEXT.CLIENT.WIRECONTAINER"));
#endif
		fillVB(_pAnimState, _WMat, _VMat);
	}
};

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_ShockWave, CXR_Model_Custom);

//----------------------------------------------------------------------
