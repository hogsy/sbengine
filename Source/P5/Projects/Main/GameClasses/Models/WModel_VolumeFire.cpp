#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

//----------------------------------------------------------------------
// CXR_Model_VolumeFire
//----------------------------------------------------------------------

class CXR_Model_VolumeFire : public CXR_Model_Custom
{

	MRTC_DECLARE;

protected:

	int m_iSurface;

public:

	enum { MaxNumParticles = 10 };
	enum { SinRandTableSize = 1024 };

private:

//	fp32				m_SinRandTable[SinRandTableSize];
//	fp32				m_SinRandTimeOffset;

	//----------------------------------------------------------------------

	void
	CreateSinRandTable()
	{
		MAUTOSTRIP(CXR_Model_VolumeFire_CreateSinRandTable, MAUTOSTRIP_VOID);
/*
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
*/
	}

	//----------------------------------------------------------------------

	fp32
	GetSinRandTable(fp32 time, fp32 timescale)
	{
		MAUTOSTRIP(CXR_Model_VolumeFire_GetSinRandTable, 0.0f);
		return 0.0f;
/*		fp32 y, y1, y2;
		int32 x, xi, xf;

		x = RoundToInt(1023.0f * 255.0f * time * timescale);

		xi = (x >> 8) & 0x3FF;
		xf = x & 0xFF;

		y1 = m_SinRandTable[xi];
		y2 = m_SinRandTable[(xi + 1) & 0x3FF];

		y = y1 + (y2 - y1) * (fp32)xf / 255.0f;

		return y;
*/	}

	//----------------------------------------------------------------------

	fp32
	GetRand(int32& randseed)
	{
		MAUTOSTRIP(CXR_Model_VolumeFire_GetRand, 0.0f);
		return MFloat_GetRand(randseed++);
	}

	//----------------------------------------------------------------------

	fp32
	GetAbs(fp32 x)
	{
		MAUTOSTRIP(CXR_Model_VolumeFire_GetAbs, 0);
		return ((x < 0.0f) ? -x : x);
	}

	//----------------------------------------------------------------------

	void
	fillParticleArray(const CXR_AnimState* _pAnimState, CMat43fp32& _LocalToCamera, CXR_Particle2* _pParticles, int32& _numParticles)
	{
		MAUTOSTRIP(CXR_Model_VolumeFire_fillParticleArray, MAUTOSTRIP_VOID);
		fp32 time = _pAnimState->m_AnimTime0;
		fp32 duration = _pAnimState->m_AnimTime1;
		int32 randseedbase = _pAnimState->m_Anim0;

		if (time > duration) {
			_numParticles = 0;
			return;
		}
/*
		CXW_Surface *pSurface = m_pSurfaceContext->GetSurfaceVersion(m_iSurface, m_pEngine);
		CXW_SurfaceKeyFrame *pKeyFrame = pSurface->GetFrame(0, _pAnimState->m_AnimTime0, *m_pSurfaceContext->m_spTempKeyFrame);
		m_ColorRGB = pKeyFrame->m_Medium.m_Color & 0x00FFFFFF;
		m_ColorAlpha = ((pKeyFrame->m_Medium.m_Color >> 24) & 0xFF);

		int numVertices = m_iVertex;
		int numTriangles = m_iIndex / 3;

		*pMatrix = LocalToCamera;
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
		pVB->Matrix_Set(pMatrix);
		pVB->Geometry_VertexArray(m_pVertexPos, numVertices);
		pVB->Geometry_TVertexArray(m_pVertexTex, 0);
		pVB->Geometry_ColorArray(m_pVertexColor);
		pVB->Render_IndexedTriangles(m_pIndex, numTriangles);

		int Flags = RENDERSURFACE_DEPTHFOG | RENDERSURFACE_VERTEXFOG;
		if(pVB->m_pTransform[0])
			Flags |= RENDERSURFACE_MATRIXSTATIC_M2V;

		CXR_Util::Render_Surface(Flags, pSurface, pKeyFrame, m_pEngine, m_pVBM, NULL, NULL, pVB->m_pTransform[0], pVB, pVB->m_Priority, 0.0001f);

		if (numParticles < MaxNumParticles) {
			numParticles = 0;
			return; // Not enough space in array.
		}
*/
		_numParticles = 0;
	}

	void
	fillParticle(fp32 time, fp32 lifetime, CMat43fp32& LocalToCamera)
	{
		MAUTOSTRIP(CXR_Model_VolumeFire_fillParticle, MAUTOSTRIP_VOID);
/*
		CXR_Particle2* particle;
		particle = AddParticle();
		if (particle == NULL)
			return;

		fp32 alpha = 1.0f;

		fp32 t = M_FMod(time, lifetime) / lifetime;

		fp32 zoffset = t;
		fp32 znoise = GetSinRandTable(time, ZNOISE_TIMESCALE);

		fp32 xoffset = GetRand() - 0.5f;
		fp32 yoffset = GetRand() - 0.5f;
		fp32 xnoise = XYNOISE * GetSinRandTable(time, XYNOISE_TIMESCALE);
		fp32 ynoise = XYNOISE * GetSinRandTable(time, XYNOISE_TIMESCALE);

		fp32 x = xoffset + xnoise;
		fp32 y = yoffset + ynoise;
		fp32 z = zoffset + znoise;

		particle.m_Size = size * PARTICLE_SIZE;
		particle.m_Angle = angle * PARTICLE_ROTATION;
		particle.m_Pos.k[0] = x * FLAME_RADIUS;
		particle.m_Pos.k[1] = y * FLAME_RADIUS;
		particle.m_Pos.k[2] = z * FLAME_HEIGHT;
		particle.m_Color = m_ColorRGB | (((int32)(alpha * (fp32)m_ColorAlpha) << 24) & 0xFF000000);
*/
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *)
	{
		MAUTOSTRIP(CXR_Model_VolumeFire_OnCreate, MAUTOSTRIP_VOID);
		m_iSurface = GetSurfaceID("FireBall01");

		CreateSinRandTable();
	}

	//----------------------------------------------------------------------
	
	virtual bool RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_VolumeFire_RenderVB, false);
		int32				numParticles = MaxNumParticles;
		CXR_Particle2		particles[MaxNumParticles];

		CMat43fp32 LocalToCamera;
		_WMat.Multiply(_VMat, LocalToCamera);

		fillParticleArray(_pAnimState, LocalToCamera, particles, numParticles);

		if (numParticles == 0) {
			return FALSE;
		}

		return CXR_Util::Render_Particles(m_pVBM, _pVB, LocalToCamera, particles, numParticles, NULL, CXR_PARTICLETYPE_ANGLE | CXR_PARTICLETYPE_QUAD);
	}
};

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_VolumeFire, CXR_Model_Custom);

//----------------------------------------------------------------------
	
