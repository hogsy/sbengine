#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

//----------------------------------------------------------------------
// CXR_Model_TeleportSuck
//----------------------------------------------------------------------

class CXR_Model_TeleportSuck : public CXR_Model_Custom
{

	MRTC_DECLARE;

protected:

	int32 m_iTexture;
	int32 m_iSurface;

public:

	// CXR_Util::MAXBEAMS
	enum { MaxNumParticles = 200, MaxNumStrips = 250 };

private:

	//----------------------------------------------------------------------

	fp32				time;
	fp32				maxTimeOffset;
	fp32				duration;
	fp32				particle_duration;
	int32			randseed, randseedbase;
	int32			octaves;
	fp32				phase_shift;
	fp32				posbasetimescale, postimescale;
	static fp32		posbaseboostarrapos[8];
	static fp32		posboostarrapos[8];
	fp32				distance;
	fp32				unity;
	fp32				amp;
	fp32				particle_size;
	fp32				trail_size;
	int32			color, color_rgb, color_alpha;

	CVec3Dfp32		source_center;
	CVec3Dfp32		destination_pos;
	CVec3Dfp32		relative_destination_pos;

	int32			wantedNumTrailStrips;
	fp32				trail_duration;
	fp32				trailstrip_duration;

	CXR_Particle2*	particles;
	int32			numParticles, wantedNumParticles;

	CXR_BeamStrip*	strips;
	int32			numStrips, wantedNumStrips;

	//----------------------------------------------------------------------

	fp32 getRand()
	{
		MAUTOSTRIP(CXR_Model_TeleportSuck_, 0);
		return MFloat_GetRand(randseed++);
	}

	//----------------------------------------------------------------------

	fp32 getSinRand(fp32* boost, fp32 timescale, fp32 time_adjustment = 0.0f);
	void fillParticle(int32 index);
	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat);


	//----------------------------------------------------------------------

	CVec3Dfp32
	getParticlePos(int32 index)
	{
		MAUTOSTRIP(CXR_Model_TeleportSuck_getParticlePos, CVec3Dfp32(0,0,0));
		CVec3Dfp32 pos;

		pos.k[0] = 2.0f * (getRand() - 0.5f);
		pos.k[1] = 2.0f * (getRand() - 0.5f);
		pos.k[2] = 2.0f * (getRand() - 0.5f);
		pos.Normalize();
		pos *= 20.0f;

		pos += source_center;

		return pos;
	}

	//----------------------------------------------------------------------

	void
	addParticle(CXR_Particle2& particle)
	{
		if (numParticles >= wantedNumParticles)
			return;

		particles[numParticles] = particle;

		numParticles++;
	}

	//----------------------------------------------------------------------

	void
	addStrip(CXR_BeamStrip& strip)
	{
		MAUTOSTRIP(CXR_Model_TeleportSuck_addStrip, MAUTOSTRIP_VOID);
		if (numStrips >= wantedNumStrips)
			return;

		strips[numStrips] = strip;

		numStrips++;
	}

	//----------------------------------------------------------------------

	void
	fillParticleArray(const CXR_AnimState* animstate, 
					  const CMat43fp32& LocalToWorld, const CMat43fp32& LocalToCamera,
					  CXR_Particle2* _particles, int32& _numParticles,
					  CXR_BeamStrip* _strips, int32& _numStrips)
	{
		MAUTOSTRIP(CXR_Model_TeleportSuck_fillParticleArray, MAUTOSTRIP_VOID);
		time = animstate->m_AnimTime0;
		duration = animstate->m_AnimTime1;
		randseedbase = animstate->m_Anim0;
		randseed = randseedbase;

		unity = 0.5f;
		amp = 15.0f;

		particle_size = 4.0f;
		trail_size = 3.0f;
		trail_duration = 0.15f * duration;
		maxTimeOffset = 0.4f * duration;
		particle_duration = duration - trail_duration - maxTimeOffset;

		wantedNumParticles = 20;
		wantedNumTrailStrips = 10;
		wantedNumStrips = wantedNumParticles * wantedNumTrailStrips;

		source_center.k[0] = 0.0f;
		source_center.k[1] = 0.0f;
		source_center.k[2] = 0.0f;
		source_center.k[0] = 2.0f * (getRand() - 0.5f);
		source_center.k[1] = 2.0f * (getRand() - 0.5f);
		source_center.k[2] = getRand();
		source_center.Normalize();
		source_center *= 100.0f;
		source_center.k[2] += 30.0f;

		distance = LocalToCamera.k[3][2];

		destination_pos.k[0] = LocalToWorld.k[3][0];
		destination_pos.k[1] = LocalToWorld.k[3][1];
		destination_pos.k[2] = LocalToWorld.k[3][2] + 10.0f;

		CVec3Dfp32 straight_path = (destination_pos - source_center);
		m_BoundRadius = straight_path.Length() * 1.5f;

//		ConOut(CStrF("destination = <%f, %f, %f>", destination_pos.k[0], destination_pos.k[1], destination_pos.k[2]));

		color = 0xFFFFFFFF;
		color_rgb = color & 0x00FFFFFF;
		color_alpha = ((color >> 24) & 0xFF);

		posbasetimescale = 0.5f / duration;
		postimescale = 1.0f / duration;
		phase_shift = 0.0f;

		octaves = 8;

		if (_numParticles < wantedNumParticles) {
			wantedNumParticles = _numParticles;
			wantedNumStrips = wantedNumParticles * wantedNumTrailStrips;
		}

		if (_numStrips < wantedNumStrips) {
			wantedNumTrailStrips = _numStrips / wantedNumParticles;
			wantedNumStrips = wantedNumParticles * wantedNumTrailStrips;
		}

		trailstrip_duration = trail_duration / (fp32)wantedNumTrailStrips;

		numParticles = 0;
		numStrips = 0;

		particles = _particles;
		strips = _strips;

		for (int32 i = 0; i < wantedNumParticles; i++) {
			randseed = randseedbase + i;
			fillParticle(i);
		}

		_numParticles = numParticles;
		_numStrips = numStrips;
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *surface)
	{
		MAUTOSTRIP(CXR_Model_TeleportSuck_OnCreate, MAUTOSTRIP_VOID);
//		m_iTexture = GetTextureID("Electric1");

		if (surface != NULL) 
			m_iSurface = GetSurfaceID(surface);
		else
//			m_iSurface = GetSurfaceID("TeleportOut");
			m_iSurface = GetSurfaceID("Electric1");
//			m_iSurface = GetSurfaceID("Effects_Flare01");

	}

	//----------------------------------------------------------------------

};

fp32 CXR_Model_TeleportSuck::posbaseboostarrapos[8] = { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };
fp32 CXR_Model_TeleportSuck::posboostarrapos[8] = { 1.0f, 1.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f, 0.0f };

//----------------------------------------------------------------------
fp32 CXR_Model_TeleportSuck::getSinRand(fp32* boost, fp32 timescale, fp32 time_adjustment)
{
	MAUTOSTRIP(CXR_Model_TeleportSuck_getSinRand, 0);
	fp32 t = (time + time_adjustment) * timescale;
	fp32 y = 0.0f;
	fp32 z = 0.0f;
	fp32 amp = 0.5f;
	fp32 wavelen = 1.0f;

	fp32 phase;
	fp32 boostamp;
	fp32 pi2 = 2.0f * _PI;

	for (int32 o = 0; o < octaves; o++)
	{
		phase = getRand() + getRand() * phase_shift * t;

		boostamp = boost[o];

		fp32 dy = boostamp * M_Sin((t + phase) / wavelen * pi2);

		if (dy < -1.0f) dy = -1.0f;
		if (dy > +1.0f) dy = +1.0f;
		if (boostamp > 1.0f) boostamp = 1.0f;

		y += amp * dy;
		z += amp * boostamp;

		amp *= 0.5f;
		wavelen *= 0.5f;
	}

	if (z != 0.0f)
		y /= z;
	else
		y = 0.0f;

	y = 0.5f + 0.5f * y;

	return y;
}

void CXR_Model_TeleportSuck::fillParticle(int32 index)
{
	MAUTOSTRIP(CXR_Model_TeleportSuck_fillParticle, MAUTOSTRIP_VOID);
	fp32 alpha = 1.0f;

	CXR_Particle2 particle;
	CXR_BeamStrip strip;

	fp32 timeoffset = maxTimeOffset * getRand();

	fp32 t = time - timeoffset;
/*
	if ((t < 0) || (t > (particle_duration + trail_duration)))
		return;
*/
	fp32 pi = _PI;
	fp32 pi2 = 2.0f * _PI;

	CVec3Dfp32 source_pos = getParticlePos(index);

	CVec3Dfp32 dir, left, up;
		
	dir = destination_pos - source_pos;
	left = CVec3Dfp32(0.0f, 1.0f, 0.0f);
	dir.CrossProd(left, up); 
	dir.CrossProd(up, left);

	dir.Normalize();
	up.Normalize();
	left.Normalize();

	int32 randseed_temp = randseed;

	for (int32 i = 0; i < wantedNumTrailStrips; i++) {
		fp32 striptimeoffset = trailstrip_duration * (fp32)i;
		fp32 x = (t - striptimeoffset) / (particle_duration + trailstrip_duration);

		fp32 fraction = 1.0f - (fp32)i / (fp32)(wantedNumTrailStrips - 1);

		if (x < 0) x = 0;
		else if (x > 1) x = 1;

		x = x * x; // Slow start...

		fp32 alpha = M_Sin(x * pi) * fraction;
//			alpha = 1.0f;

		randseed = 0;

		fp32 ybase = getSinRand(posbaseboostarrapos, posbasetimescale, t - striptimeoffset);
		fp32 zbase = getSinRand(posbaseboostarrapos, posbasetimescale, t - striptimeoffset);

		randseed = randseed_temp;

		fp32 y = getSinRand(posboostarrapos, postimescale, t - striptimeoffset);
		fp32 z = getSinRand(posboostarrapos, postimescale, t - striptimeoffset);

		y = unity * ybase + (1.0f - unity) * y;
		z = unity * zbase + (1.0f - unity) * z;

		CVec3Dfp32 pos = source_pos * (1.0f - x) + (left * y + up * z) * amp * M_Sin(x * pi) + destination_pos * x;

		if (i == 0) {
			particle.m_Pos = pos;
			particle.m_Size = M_Sin(x * pi) * particle_size * fraction;
			particle.m_Angle = 0;
			particle.m_Color = color_rgb | ((int32)(alpha * (fp32)color_alpha) << 24);
//				addParticle(particle);

			strip.m_Flags = CXR_BEAMFLAGS_BEGINCHAIN;
		} else {
			strip.m_Flags = 0;
		}

		strip.m_Pos = pos;
		strip.m_Width = M_Sin(x * pi) * trail_size * fraction;
//			strip.m_TextureYOfs = i;
//			strip.m_TextureYOfs = (fp32)i * 0.1f;
		strip.m_TextureYOfs = 0;
		strip.m_Color = color_rgb | ((int32)(alpha * (fp32)color_alpha) << 24);
//			strip.m_Color = 0x80FFFFFF;

		addStrip(strip);
	}
}

void CXR_Model_TeleportSuck::Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
{
	MAUTOSTRIP(CXR_Model_TeleportSuck_Render, MAUTOSTRIP_VOID);
	int32				numParticles = MaxNumParticles;
	CXR_Particle2		particles[MaxNumParticles];

	int32				numStrips = MaxNumStrips;
	CXR_BeamStrip		strips[MaxNumStrips];

	CMat43fp32			identity; identity.Unit();
	CMat43fp32			LocalToCamera;
	_WMat.Multiply(_VMat, LocalToCamera);

	fillParticleArray(_pAnimState, _WMat, LocalToCamera, particles, numParticles, strips, numStrips);

	CXR_VertexBuffer* particleVB = AllocVB();
	if (particleVB == NULL)
		return;
	
	CXR_VertexBuffer* stripVB = AllocVB();
	if (stripVB == NULL)
		return;

	if (numParticles > 0) {
		if (CXR_Util::Render_Particles(m_pVBM, particleVB, _VMat, particles, numParticles, NULL, CXR_PARTICLETYPE_ANGLE | CXR_PARTICLETYPE_QUAD))
			Render_Surface(m_iSurface, particleVB, _pAnimState->m_AnimTime0);
		else
			return;
	}

	if (numStrips > 0) {
		if (CXR_Util::Render_BeamStrip2(m_pVBM, stripVB, identity, _VMat, strips, numStrips))
			Render_Surface(m_iSurface, stripVB, _pAnimState->m_AnimTime0);
		else
			return;
	}

}
//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_TeleportSuck, CXR_Model_Custom);

//----------------------------------------------------------------------
	
