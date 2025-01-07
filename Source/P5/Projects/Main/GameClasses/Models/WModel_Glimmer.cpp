#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

//----------------------------------------------------------------------
// CXR_Model_Glimmer
//----------------------------------------------------------------------

class CXR_Model_Glimmer : public CXR_Model_Custom
{

	MRTC_DECLARE;

protected:

	int32 m_iTexture;
	int32 m_iSurface;

public:

	// CXR_Util::MAXBEAMS
//	enum { MaxNumParticles = 100 };
	enum { MaxNumParticles = 20 };

private:

	//----------------------------------------------------------------------

	fp32				time;
	fp32				fade;
	fp32				size;
	int32			octaves;
	int32			randseed;
	fp32				phase_shift;
	fp32				ptimescale, stimescale, atimescale, ftimescale;
	static fp32		pboostarray[8];
	static fp32		sboostarray[8];
	static fp32		aboostarray[8];
	static fp32		fboostarray[8];
	fp32				distance;
	fp32				psize;
	fp32				speed_fluct, size_fluct, angle_fluct, fade_fluct;

	//----------------------------------------------------------------------

	fp32 getRand(int32& randseed);
	fp32 getSinRand(const fp32* boost, fp32 timescale, fp32 time_adjustment);
	bool fillParticle(CXR_Particle2& particle);
	virtual void OnCreate(const char *surface);
	virtual bool RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat);
	void fillParticleArray(const CXR_AnimState* animstate, const CMat43fp32& LocalToCamera, CXR_Particle2* particles, int32& numParticles);
};

fp32 CXR_Model_Glimmer::pboostarray[8] = { 1.0f, 0.5f, 0.25f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
fp32 CXR_Model_Glimmer::sboostarray[8] = { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
fp32 CXR_Model_Glimmer::aboostarray[8] = { 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
fp32 CXR_Model_Glimmer::fboostarray[8] = { 2.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

fp32 CXR_Model_Glimmer::getRand(int32& randseed)
{
	MAUTOSTRIP(CXR_Model_Glimmer_getRand, 0);
	return MFloat_GetRand(randseed++);
}

//----------------------------------------------------------------------

fp32 CXR_Model_Glimmer::getSinRand(const fp32* boost, fp32 timescale, fp32 time_adjustment = 0.0f)
{
	MAUTOSTRIP(CXR_Model_Glimmer_getSinRand, 0);
	const fp32 t = (time + time_adjustment) * timescale;
	fp32 y = 0.0f;
	fp32 z = 0.0f;
	fp32 amp = 0.5f;
	fp32 wavelen = 1.0f;

	fp32 boostamp;

	for (int32 o = 0; o < octaves; o++)
	{
		const fp32 phase = getRand(randseed) + getRand(randseed) * phase_shift * t;

		boostamp = boost[o];

		fp32 dy = boostamp * M_Sin((t + phase) / wavelen * _PI2 );

		if (dy < -1.0f) dy = -1.0f;
		else if (dy > +1.0f) dy = +1.0f;

		if (boostamp > 1.0f) boostamp = 1.0f;

		y += amp * dy;
		z += amp * boostamp;

		amp *= 0.5f;
		wavelen *= 0.5f;
	}

	if (z != 0.0f)
		y = 0.5f + 0.5f * ( y / z );
	else
		y = 0.5f;


	return y;
}

//----------------------------------------------------------------------

void CXR_Model_Glimmer::fillParticleArray(const CXR_AnimState* animstate, const CMat43fp32& LocalToCamera, CXR_Particle2* particles, int32& numParticles)
{
	MAUTOSTRIP(CXR_Model_Glimmer_fillParticleArray, MAUTOSTRIP_VOID);
	time = animstate->m_AnimTime0;
	size = animstate->m_Anim1;
	randseed = animstate->m_Anim0;
	fade = GetFade(animstate);
		
	if (size == 0.0f)
		size = 80.0f;

	m_BoundRadius = 0.5f * size;

	SetFadeTime(1.0f);

	int32 wantedNumParticles = MaxNumParticles;

	psize = 0.6f * size;

	distance = LocalToCamera.k[3][2];
	distance = 0;

	speed_fluct = 0.2f;
	size_fluct = 0.2f;
	angle_fluct = 1.0f;
	fade_fluct = 1.0f;

	ptimescale = 0.1f;
	stimescale = 0.1f;
	atimescale = 0.1f;
	ftimescale = 0.5f;
	phase_shift = 0.5f;

	octaves = 8;

	numParticles = 0;

	for (int32 i = 0; i < wantedNumParticles; i++) {
		randseed = animstate->m_Anim0 + i;
		if (fillParticle(particles[numParticles])) {
			numParticles++;
		}
	}
}

bool CXR_Model_Glimmer::fillParticle(CXR_Particle2& particle)
{
	MAUTOSTRIP(CXR_Model_Glimmer_fillParticle, false);
	fp32 alpha = (1.0f - fade_fluct) + fade_fluct * getSinRand(fboostarray, ftimescale);

	alpha *= fade;

	const fp32 neardist = 200;
	const fp32 fardist = 300;

	if (distance > neardist)
		alpha *= 1.0f - ((distance - neardist) / (fardist - neardist));

	if (alpha <= 0.0f)
		return false;

	particle.m_Size = psize * ((1.0f - size_fluct) + size_fluct * getSinRand(sboostarray, stimescale));
	particle.m_Size *= alpha;

	if (particle.m_Size <= 0.0f)
		return false;

	const fp32 radius = 0.5f * size;

	CVec3Dfp32 dir;
	const fp32 speed = speed_fluct * 2.0f * (getSinRand(pboostarray, ptimescale) - 0.5f);
	dir.k[0] = 2.0f * (getSinRand(pboostarray, ptimescale, speed) - 0.5f);
	dir.k[1] = 2.0f * (getSinRand(pboostarray, ptimescale, speed) - 0.5f);
	dir.k[2] = 2.0f * (getSinRand(pboostarray, ptimescale, speed) - 0.5f);

	CVec3Dfp32 normdir = dir;
	normdir.Normalize();
	dir.k[0] *= normdir.k[0] * Sign(dir.k[0]);
	dir.k[1] *= normdir.k[1] * Sign(dir.k[1]);
	dir.k[2] *= normdir.k[2] * Sign(dir.k[2]);

	particle.m_Pos = dir * radius;
	particle.m_Pos.k[2] += radius * 0.5f;

	particle.m_Angle = angle_fluct * getSinRand(aboostarray, atimescale);

	particle.m_Color = ( 0x00ffffff ) | ((int32)(alpha * 255.0f) << 24);

	return true;
}

//----------------------------------------------------------------------
	
void CXR_Model_Glimmer::OnCreate(const char *surface)
{
	MAUTOSTRIP(CXR_Model_Glimmer_OnCreate, MAUTOSTRIP_VOID);
//	m_iTexture = m_pTextureContext->GetTextureID("Electric1");

	if (surface != NULL) 
		m_iSurface = GetSurfaceID(surface);
	else
		m_iSurface = GetSurfaceID("Glimmer1");

}

//----------------------------------------------------------------------

 bool CXR_Model_Glimmer::RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
{
	MAUTOSTRIP(CXR_Model_Glimmer_RenderVB, false);
//	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);
//	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
/*
	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	_pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
*/

	int32				numParticles;
	CXR_Particle2		particles[MaxNumParticles];

	CMat43fp32 LocalToCamera;
	_WMat.Multiply(_VMat, LocalToCamera);

	fillParticleArray(_pAnimState, _WMat, particles, numParticles);

	if (numParticles == 0) {
//		ConOut("Zero particles!");
		return false;
	}

	if (CXR_Util::Render_Particles(m_pVBM, _pVB, LocalToCamera, particles, numParticles, NULL, CXR_PARTICLETYPE_ANGLE | CXR_PARTICLETYPE_QUAD)) {
//		ConOut("Render successfull!");
		Render_Surface(m_iSurface, _pVB, _pAnimState->m_AnimTime0);
		return false;
	} else {
//		ConOut("Render failed!");
		return false;
	}
}

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Glimmer, CXR_Model_Custom);

//----------------------------------------------------------------------
	
