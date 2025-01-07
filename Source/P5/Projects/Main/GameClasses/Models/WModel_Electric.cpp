#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

//----------------------------------------------------------------------

#define LENGTH_SUBPRECISION (10.0f)

//----------------------------------------------------------------------
// CXR_Model_Electric
//----------------------------------------------------------------------

class CXR_Model_Electric : public CXR_Model_Custom
{

	MRTC_DECLARE;

protected:

	int32 m_iTexture;
	int32 m_iSurface;

public:

	// CXR_Util::MAXBEAMS
	enum { MaxNumStrips = 50 };

private:

	//----------------------------------------------------------------------

	fp32				time;
	int32			octaves;
	fp32				repulsion;
	int32			randseed;
	fp32				phase_shift;
	fp32				ytimescale, atimescale, wtimescale;
	static fp32		yboostarray[8];
	static fp32		aboostarray[8];
	static fp32		wboostarray[8];
	int32			maxdepth;
	fp32				ythreshold;

	int32			numEstimatedStrips;
	fp32				texturescale;

	fp32				length;
	fp32				width;
	CVec3Dfp32		dir;
	CVec3Dfp32		up, left;

	CXR_BeamStrip*	strips;
	int32			numStrips, numStripsLeft;

	int32			color, color_rgb, color_alpha;

	fp32				m_Length;
	fp32				m_TimeScale;

	struct Noise
	{
		fp32 x, y, a, w;
	};

	//----------------------------------------------------------------------

	fp32 getRand(int32& randseed)
	{
		MAUTOSTRIP(CXR_Model_Electric_getRand, 0);
		return MFloat_GetRand(randseed++);
	}

	fp32 getSinRand(fp32* boost, fp32 timescale);
	fp32 repulse(fp32 x, fp32 weight);
	void emit(Noise n, fp32 alpha);
	void recurse(Noise n1, Noise n2, fp32 yscale, fp32 ascale, fp32 wscale, int32 depth);
	void fillBeamStripArray(const CXR_AnimState* animstate, const CMat43fp32& LocalToWorld, CXR_BeamStrip* _strips, int32& _numStrips);
	virtual void OnCreate(const char *_params);
	virtual bool RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat);
};

fp32 CXR_Model_Electric::yboostarray[8] = { 8.0f, 6.0f, 0.0f, 0.0f, 1.0f, 1.0f, 1.0f, 1.0f };
fp32 CXR_Model_Electric::aboostarray[8] = { 4.0f, 4.0f, 1.0f, 0.5f, 0.1f, 0.0f, 0.0f, 0.0f };
fp32 CXR_Model_Electric::wboostarray[8] = { 4.0f, 4.0f, 0.0f, 4.0f, 0.0f, 0.0f, 0.0f, 0.0f };

	//----------------------------------------------------------------------

fp32 CXR_Model_Electric::repulse(fp32 x, fp32 weight)
{
	MAUTOSTRIP(CXR_Model_Electric_repulse, 0);
	fp32 y;

	if (x < -1.0f) x = -1.0f;
	if (x > +1.0f) x = +1.0f;

	if (x > 0) {
		y = 1.0f - x;
	} else {
		y = -1.0f - x;
	}

	y = weight * (M_Sqrt(1.0f - y * y) * Sign(x)) + (1.0f - weight) * x;

	return y;
}

//----------------------------------------------------------------------

fp32 CXR_Model_Electric::getSinRand(fp32* boost, fp32 timescale)
{
	MAUTOSTRIP(CXR_Model_Electric_getSinRand, 0);
	fp32 t = time * timescale;
	fp32 y = 0.0f;
	fp32 z = 0.0f;
	fp32 amp = 0.5f;
	fp32 wavelen = 1.0f;

	fp32 phase;
	fp32 boostamp;
	fp32 pi2 = 2.0f * _PI;

	for (int32 o = 0; o < octaves; o++)
	{
		phase = getRand(randseed) + getRand(randseed) * phase_shift * t;

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

//	fp32 _y = y;
		
	if (z != 0.0f)
		y /= z;
	else
		y = 0.0f;

//	ConOutL(CStrF("y = %f, z = %f, y/z = %f", _y, z, y));

	y = 0.5f + 0.5f * y;

	return y;
}

//----------------------------------------------------------------------

void CXR_Model_Electric::emit(Noise n, fp32 alpha)
//	emit(fp32 x, fp32 y, fp32 a, fp32 w, fp32 alpha)
{
	MAUTOSTRIP(CXR_Model_Electric_emit, MAUTOSTRIP_VOID);
	fp32 pi = _PI;
	fp32 pi2 = 2.0f * _PI;
	n.a *= pi2;

//	ConOutL(CStrF("x = %f, y = %f, a = %f, w = %f, alpha = %f", x, y, a, w, alpha));

//	n.a = 0;
//	n.y = 0;
//	n.w = 0;

	CVec3Dfp32 amp = (up * M_Cos(n.a) + left * M_Sin(n.a));

	strips[numStrips].m_Pos = dir * n.x * length + amp * n.y * length;
	strips[numStrips].m_Color = color_rgb | ((int32)(alpha * (fp32)color_alpha) << 24);
	strips[numStrips].m_Width = (0.4f + 0.6f * M_Sin(n.x * _PI)) * (0.6f + 0.2f * n.w + 0.2f * n.w * n.w) * width;

	fp32 texy = n.w * n.w * 0.5f;
	strips[numStrips].m_TextureYOfs = (texy * texturescale);
//	strips[numStrips].m_TextureYOfs = 0.0f;
	strips[numStrips].m_Flags = 0;

	numStrips++;
	numStripsLeft--;
}
//----------------------------------------------------------------------

void CXR_Model_Electric::recurse(Noise n1, Noise n2, fp32 yscale, fp32 ascale, fp32 wscale, int32 depth)
{
	MAUTOSTRIP(CXR_Model_Electric_recurse, MAUTOSTRIP_VOID);
	if (depth > maxdepth)
		return;

	Noise nm;

	fp32 dy, da, dw;

	dy = getSinRand(yboostarray, ytimescale);
	dy = 2.0f * (dy - 0.5f);
	dy = repulse(dy, repulsion);
	dy *= yscale;

	da = getSinRand(aboostarray, atimescale);
	da = 2.0f * (da - 0.5f);
	da = repulse(da, repulsion);
	da *= ascale;

	dw = getSinRand(wboostarray, wtimescale);
	dw = 2.0f * (dw - 0.5f);
	dw = repulse(dw, repulsion);
	dw *= wscale;

	nm.y = (n1.y + n2.y) * 0.5f + dy;
	nm.a = (n1.a + n2.a) * 0.5f + da;
	nm.w = (n1.w + n2.w) * 0.5f + dw;
	nm.x = (n1.x + n2.x) * 0.5f;

	int32 randseed_temp = randseed;

	if (numStripsLeft > 0) {
		recurse(n1, nm, yscale * 0.5f, ascale * 0.5f, wscale * 0.5f, depth + 1);
		emit(nm, 1.0f);

		randseed = randseed_temp + 1;
		recurse(nm, n2, yscale * 0.5f, ascale * 0.5f, wscale * 0.5f, depth + 1);
	} else {
		emit(nm, 1.0f);
	}

	randseed = randseed_temp + 2;
}

//----------------------------------------------------------------------

void CXR_Model_Electric::fillBeamStripArray(const CXR_AnimState* animstate, const CMat43fp32& LocalToWorld, CXR_BeamStrip* _strips, int32& _numStrips)
{
	MAUTOSTRIP(CXR_Model_Electric_fillBeamStripArray, MAUTOSTRIP_VOID);
	time = animstate->m_AnimTime0;
	fp32 duration = animstate->m_AnimTime1;
	
	m_Length = animstate->m_Anim1 / LENGTH_SUBPRECISION;

	length = m_Length;
	width = 40.0f + 0.05f * length;

	m_BoundRadius = length;

	maxdepth = 5;

	numEstimatedStrips = 1;
	for (int32 d = 0; d < maxdepth; d++)
		numEstimatedStrips *= 2;

//	texturescale = numEstimatedStrips;
	texturescale = 4;

	repulsion = 0.0f;

	ytimescale = 0.75f * m_TimeScale;
	atimescale = 0.1f * m_TimeScale;
	wtimescale = 0.5f * m_TimeScale;
	phase_shift = 0.5f;

	octaves = 8;

	ythreshold = 0.01f;

	fp32 distancescale = 1.0f;

	fp32 yscale = 0.05f * distancescale;
	fp32 ascale = 0.3f;
	fp32 wscale = 0.5f;

	fp32 screw0 = 0.5f;
	fp32 screw1 = 0.5f;

	color = animstate->m_Colors[0];
	if (color == 0)
		color = 0xFFFFFFFF;

	color_rgb = color & 0x00FFFFFF;
	color_alpha = ((color >> 24) & 0xFF);
	color_alpha = (int32)(::GetFade(time, duration, 0.25f, 0.25f) * (fp32)color_alpha);

	dir = CVec3Dfp32(-1.0f, 0.0f, 0.0f);
	left = CVec3Dfp32(0.0f, 1.0f, 0.0f);
	up = CVec3Dfp32(0.0f, 0.0f, 1.0f);

//	dir = CVec3Dfp32(1.0f, 0.0f, 0.0f) * LocalToWorld;
//	left = CVec3Dfp32(0.0f, 1.0f, 0.0f);
//	dir.CrossProd(left, up); 
//	dir.CrossProd(up, left);
//	dir.Normalize();
//	up.Normalize();
//	left.Normalize();

	numStrips = 0;
	strips = _strips;
	numStripsLeft = _numStrips;

	randseed = animstate->m_Anim0;

	if (numStripsLeft > 0)
	{
		Noise n1, n2;
		n1.x = 0.0f;
		n2.x = 1.0f;

		n1.y = 0.0f;
		n2.y = 0.0f;

		n1.w = 0.0f;
		n2.w = 0.0f;

		fp32 a = (getSinRand(aboostarray, atimescale) * screw0) - screw0 * 0.5f;
		n1.a = a + (getSinRand(aboostarray, atimescale) * screw1) - screw1 * 0.5f;
		n2.a = a + (getSinRand(aboostarray, atimescale) * screw1) - screw1 * 0.5f;

		emit(n1, 0.0f);
		recurse(n1, n2, yscale, ascale, wscale, 1);
		emit(n2, 0.0f);
/*
		fp32 a0 = (getSinRand(aboost, atimescale) * screw0) - screw0 * 0.5f;
		fp32 a1 = a0 + (getSinRand(aboost, atimescale) * screw1) - screw1 * 0.5f;
		fp32 a2 = a0 + (getSinRand(aboost, atimescale) * screw1) - screw1 * 0.5f;
//		fp32 a1 = 0;
//		fp32 a2 = 0;

		//emit(0.0f, 0.0f, 0.0f, 0.0f, 0.5f);
		//recurse(0.0f, 1.0f, 0.0f, 0.0f, a1, a2, yscale, ascale, 1);
		//emit(1.0f, 0.0f, 0.0f, 0.0f, 0.5f);
*/
	}

//	ConOut(CStrF("time = %f, Emitting %i of %i strips", time, numStrips, _numStrips));

	_numStrips = numStrips;

}

//----------------------------------------------------------------------
	
void CXR_Model_Electric::OnCreate(const char *_params)
{
	MAUTOSTRIP(CXR_Model_Electric_OnCreate, MAUTOSTRIP_VOID);
	if (_params != NULL)
	{
		CStr params(_params);
		m_iSurface = GetSurfaceID(params.GetStrSep(","));
		m_Length = params.Getfp64Sep(",");
		m_TimeScale = params.Getfp64Sep(",");
	}
	else
	{
		m_iSurface = GetSurfaceID("Electric1");
		m_Length = 0.0f;
		m_TimeScale = 0.0f;
	}

	if (m_Length == 0.0f)
		m_Length = 100.0f;

	if (m_TimeScale == 0.0f)
		m_TimeScale = 1.0f;
}

//----------------------------------------------------------------------

bool CXR_Model_Electric::RenderVB(CXR_VertexBuffer *_pVB, const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
{
	MAUTOSTRIP(CXR_Model_Electric_RenderVB, false);
//	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_NONE);
//	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHAADD);
/*
	_pVB->m_pAttrib->Attrib_RasterMode(CRC_RASTERMODE_ALPHABLEND);
	_pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_ZWRITE);
	_pVB->m_pAttrib->Attrib_TextureID(0, m_iTexture);
*/

	int32				numStrips = MaxNumStrips;
	CXR_BeamStrip		strips[MaxNumStrips];

	CMat43fp32 LocalToCamera;
	_WMat.Multiply(_VMat, LocalToCamera);

	fillBeamStripArray(_pAnimState, _WMat, strips, numStrips);

	if (numStrips == 0) {
//		ConOut("Zero strips!");
		return FALSE;
	}

	if (CXR_Util::Render_BeamStrip2(m_pVBM, _pVB, _WMat, _VMat, strips, numStrips, 0/*CXR_BEAMFLAGS_EDGEFADE*/)) {
//		ConOut("Render successfull!");
		Render_Surface(m_iSurface, _pVB, _pAnimState->m_AnimTime0);
		return FALSE;
	} else {
//		ConOut("Render failed!");
		return FALSE;
	}
}

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_Electric, CXR_Model_Custom);

//----------------------------------------------------------------------
	
