#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

//----------------------------------------------------------------------
// CXR_Model_DustCloud
//----------------------------------------------------------------------

class CXR_Model_DustCloud : public CXR_Model_Custom
{

	MRTC_DECLARE;

protected:

	int m_iTexture;

public:

	enum { MaxNumParticles = 10 };

private:

	fp32 getRand(int32& randseed)
	{
		MAUTOSTRIP(CXR_Model_DustCloud_getRand, 0);
		return MFloat_GetRand(randseed++);
	}

	void fillParticleArray(const CXR_AnimState* animstate, CMat43fp32& LocalToCamera, CXR_Particle2* particles, int32& numParticles);
	bool fillParticle(fp32 time, fp32 duration, fp32 size, int32 color, int32 randseed, CMat43fp32& LocalToCamera, CXR_Particle2& particle);

	//----------------------------------------------------------------------
	
private:

	virtual void OnCreate(const char *)
	{
		MAUTOSTRIP(CXR_Model_DustCloud_OnCreate, MAUTOSTRIP_VOID);
		m_iTexture = GetTextureID("DustCloud1");
	}

	virtual bool RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat);
};

//----------------------------------------------------------------------

void CXR_Model_DustCloud::fillParticleArray(const CXR_AnimState* animstate, CMat43fp32& LocalToCamera, CXR_Particle2* particles, int32& numParticles)
{
	MAUTOSTRIP(CXR_Model_DustCloud_fillParticleArray, MAUTOSTRIP_VOID);
	fp32 time = animstate->m_AnimTime0;
	fp32 duration = animstate->m_AnimTime1;
	fp32 size = animstate->m_Anim1;
	int32 color = animstate->m_Colors[0];
	int32 randseedbase = animstate->m_Anim0;

	m_BoundRadius = size / 1.5f;

	ConOut(CStrF("time = %f, duration = %f, size = %f", time, duration, size));

	if (time > duration) {
		numParticles = 0;
		return;
	}

	if (numParticles < MaxNumParticles) {
		numParticles = 0;
		return; // Not enough space in array.
	}

	numParticles = 0;

	for (int32 i = 0; i < MaxNumParticles; i++) {
		if (fillParticle(time, duration, size, color, randseedbase + i, LocalToCamera, particles[numParticles])) {
			numParticles++;
		}
	}
}

bool CXR_Model_DustCloud::fillParticle(fp32 time, fp32 duration, fp32 size, int32 color, int32 randseed, CMat43fp32& LocalToCamera, CXR_Particle2& particle)
{
	MAUTOSTRIP(CXR_Model_DustCloud_fillParticle, false);
	fp32 ptime = time - 0.1f * getRand(randseed);
	fp32 pduration = duration * (0.5f + 0.5f * getRand(randseed)); // particle duration.

	if ((ptime < 0) || (ptime > pduration)) {
		return false;
	}

	fp32 t = ptime / pduration; // fraction of particle duration.
	fp32 t2 = t * t;
	fp32 t4 = t2 * t2;
	fp32 ct = 1.0f - t; // complement of t.
	fp32 ct2 = ct * ct;
	fp32 ct4 = ct2 * ct2;
	fp32 ct8 = ct4 * ct4;
	fp32 ct16 = ct8 * ct8;
	fp32 ct32 = ct16 * ct16;

	fp32 above = M_Fabs(LocalToCamera.k[2][2]);
	fp32 above2 = above * above;
	fp32 above3 = above2 * above;
	fp32 above4 = above2 * above2;

	fp32 distance = LocalToCamera.k[3][2];
	fp32 sinview = M_Fabs(LocalToCamera.k[0][0]);
	fp32 cosview = M_Fabs(LocalToCamera.k[1][0]);

	fp32 radius = (size / 2.0f) * (0.0f + 1.0f * getRand(randseed)); // radius of dust cloud (i.e. not of the particle).
	fp32 alpha = ((1.0f - t2) / (0.25f * (fp32)MaxNumParticles)) * (0.5f + 0.5f * getRand(randseed));

	int32 color_alpha = ((color >> 24) & 0xFF);
	int32 color_rgb = color & 0x00FFFFFF;

	fp32 neardist = 200;
	fp32 fardist = 300;

	if (distance > neardist) {
		alpha *= 1.0f - ((distance - neardist) / (fardist - neardist));
	}

	if ((alpha <= 0) || (distance < 0)) {
		return false;
	}

	fp32 pi2 = 2.0f * _PI;
		
	CVec3Dfp32 dir;

	dir.k[0] = (getRand(randseed) - 0.5f) * (1.0f - ((0.5f + 0.5f * cosview) * (1.0f - above)));
	dir.k[1] = (getRand(randseed) - 0.5f) * (1.0f - ((0.5f + 0.5f * sinview) * (1.0f - above)));
	dir.k[2] = 0;
	dir.Normalize();
	fp32 size_sin = (0.5f + 0.5f * M_Sin(0.5f * (ptime / pduration) * pi2));
	particle.m_Size = (1.0f + 0.5f * above3) * size_sin * size * (0.5f + 0.5f * getRand(randseed));
	particle.m_Pos = dir * radius * ((1.0f - ct8) * 0.5f + t2 * 0.5f);
	particle.m_Pos.k[2] = particle.m_Size * 0.1f;
	particle.m_Angle = 0.5f + 0.4f * ct4 * (M_Sin(3.0f * t * (getRand(randseed) - 0.5f) * pi2)) * getRand(randseed);
	particle.m_Color = color_rgb | ((int32)(alpha * (fp32)color_alpha) << 24);

	return true;
}

//----------------------------------------------------------------------
	
bool CXR_Model_DustCloud::RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
{
	MAUTOSTRIP(CXR_Model_DustCloud_RenderVB, false);
//	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);
//	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	_pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);

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

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_DustCloud, CXR_Model_Custom);

//----------------------------------------------------------------------
	
