#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

//----------------------------------------------------------------------

#define getRand()		(MFloat_GetRand(m_Randseed++))
#define getSign(x)		(Sign(x))
#define getAbs(x)		(((x) < 0) ? -(x) : (x))

//----------------------------------------------------------------------
// CModelHistory
//----------------------------------------------------------------------

enum { MaxHistoryLength = 10 };

class CModelHistory : public CReferenceCount
{

public:

	struct Entry
	{
		CMat43fp32*	m_LocalToWorld;
		fp32			m_Time;
	};

private:

	CMat43fp32	m_Matrix[MaxHistoryLength];
	fp32			m_Time[MaxHistoryLength];
	int32		m_iEntry, m_iOldestEntry;
	int32		m_numEntries;

public:

	CModelHistory()
	{
		m_iEntry = -1;
		m_iOldestEntry = 0;
		m_numEntries = 0;

		for (int i = 0; i < MaxHistoryLength; i++) {
			m_Matrix[i].Unit();
			m_Time[i] = MaxHistoryLength - i;
		}

//		ConOutL(CStrF("CModelHistory(%X) CREATED!", this));
	}

	~CModelHistory()
	{
//		ConOutL(CStrF("CModelHistory(%X) DESTROYED!", this));
	}

	void
	addEntry(const CMat43fp32& LocalToWorld, fp32 time)
	{
		if (m_numEntries < MaxHistoryLength) m_numEntries++;

		m_iEntry++;
		if (m_iEntry >= MaxHistoryLength) m_iEntry -= MaxHistoryLength;

		m_iOldestEntry = m_iEntry - m_numEntries + 1;
		if (m_iOldestEntry < 0) m_iOldestEntry += MaxHistoryLength;

//		ConOutL(CStrF("CModelHistory(%X)::addEntry(time = %f), numEntries = %d, iEntry = %d, iOldestEntry = %d", this, time, m_numEntries, m_iEntry, m_iOldestEntry));

		m_Matrix[m_iEntry] = LocalToWorld;
		m_Time[m_iEntry] = time;
	}

	fp32
	getOldestTime()
	{
		return m_Time[m_iOldestEntry];
	}

	bool
	getInterpolatedMatrix(fp32 time, CMat43fp32& LocalToWorld)
	{
		if (m_numEntries == 0)
			return false;

		if ((time > m_Time[m_iEntry]) || (time < m_Time[m_iOldestEntry]))
			return false;

		if (time == m_Time[m_iEntry]) {
			LocalToWorld = m_Matrix[m_iEntry];
			return true;
		}

		if (time == m_Time[m_iOldestEntry]) {
			LocalToWorld = m_Matrix[m_iOldestEntry];
			return true;
		}

		int indexcount = 1;
		int index = m_iEntry - 1;
		if (index < 0) index += MaxHistoryLength;

		while ((m_Time[index] > time) && (indexcount < m_numEntries))
		{
			index--;
			if (index < 0) index += MaxHistoryLength;
			indexcount++;
		}

		int32 index1 = index;
		int32 index2 = index + 1;
		if (index2 >= MaxHistoryLength) index2 -= MaxHistoryLength;

		fp32 t1, t2;

		t1 = m_Time[index1];
		t2 = m_Time[index2];

		fp32 tf = (time - t1) / (t2 - t1);

		for (int i = 0; i < 4; i++)
			for (int j = 0; j < 4; j++)
				LocalToWorld.k[i][j] = m_Matrix[index1].k[i][j] * (1.0f - tf) + m_Matrix[index2].k[i][j] * tf;

		return true;
	}

};

//----------------------------------------------------------------------
// CXR_Model_FireBlade
//----------------------------------------------------------------------

class CXR_Model_FireBlade : public CXR_Model_Custom
{

	MRTC_DECLARE;

protected:

	int32 m_iSurface;

public:

	enum { SinRandTableSize = 1024 };

	enum { NumTrailSteps = 6, NumBladeSegments = 15 };

private:

	//----------------------------------------------------------------------

	fp32				m_Time; // Total lifetime of model.
	fp32				m_Distance; // Distance from camera to model origin.
	int32			m_Randseed, m_RandseedBase;

	fp32				m_SinRandTable[SinRandTableSize];
	fp32				m_SinRandTimeOffset;

	fp32				m_PosAmp, m_PosTimeScale;

	CMat43fp32		m_LocalToWorld[NumTrailSteps];
	CVec3Dfp32		m_TrailDir[NumTrailSteps]; // Directions of the trails.
	CVec3Dfp32		m_TrailPos[NumTrailSteps]; // Origin positions of the trails.
	fp32				m_TrailDuration; // Duration of the trail.
	fp32				m_Length; // Length of the blade.
	fp32				m_FlameRise; // Height flame will rise after full trail duration;
	fp32				m_FlameStartWidth, m_FlameEndWidth, m_FlameUnalignedWidth;
	fp32				m_TexURepeats, m_TexVRepeats, m_TexVScroll;

	CXR_BeamStrip*	m_BeamStrips;
	int32			m_NumBeamStrips, m_MaxNumBeamStrips;

	//----------------------------------------------------------------------

	void
	createSinRandTable()
	{
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
	getSinRandTable(fp32 timescale, fp32 timeoffset = 0.0f)
	{
		fp32 y, y1, y2;
		int32 x, xi, xf;

		x = RoundToInt(1023.0f * 255.0f * (m_Time + m_SinRandTimeOffset * getRand() + timeoffset) * timescale);

		xi = (x >> 8) & 0x3FF;
		xf = x & 0xFF;

		y1 = m_SinRandTable[xi];
		y2 = m_SinRandTable[(xi + 1) & 0x3FF];

		y = y1 + (y2 - y1) * (fp32)xf / 255.0f;

		return y;
	}

	//----------------------------------------------------------------------

	// Get history from clientdata.
	// If no clientdata present, create new one and set it.
	CModelHistory*
	getHistory(const CXR_AnimState* pAnimState)
	{
		CModelHistory* history;

		if(pAnimState->m_pspClientData == NULL)
			return NULL;

		history = (CModelHistory*)(CReferenceCount*)(*pAnimState->m_pspClientData);

		if (history == NULL) {
			*pAnimState->m_pspClientData = DNew(CModelHistory) CModelHistory();
			if (*pAnimState->m_pspClientData == NULL) return NULL;
			history = (CModelHistory*)(CReferenceCount*)(*pAnimState->m_pspClientData);
//			ConOutL(CStrF("Created History(%X), RandAnim0 = %d", history, pAnimState->m_Anim0));
		} else {
//			ConOutL(CStrF("Found History(%X), RandAnim0 = %d", history, pAnimState->m_Anim0));
		}

		return history;
	}

	//----------------------------------------------------------------------

	// Updates history and matrices.
	// Returns true for success, false for failure.
	bool
	updateHistory(const CXR_AnimState* pAnimState, const CMat43fp32& LocalToWorld)
	{
		CModelHistory* history = getHistory(pAnimState);

		if (history == NULL)
			return false;

		// add current matrix to history.
		history->addEntry(LocalToWorld, m_Time);

		// compute new interpolated matrices from history.
		m_TrailDuration = m_Time - history->getOldestTime();

		fp32 time, fraction;

		for (int i = 0; i < NumTrailSteps; i++)
		{
			fraction = (fp32)i / (fp32)(NumTrailSteps - 1);
			time = m_Time - m_TrailDuration * fraction;
			if (!history->getInterpolatedMatrix(time, m_LocalToWorld[i]))
				return false;

			m_TrailDir[i] = CVec3Dfp32::GetMatrixRow(m_LocalToWorld[i], 0);
			m_TrailPos[i] = CVec3Dfp32::GetMatrixRow(m_LocalToWorld[i], 3);
			m_TrailPos[i].k[2] += m_FlameRise * fraction;
		}

		return true;
	}

	//----------------------------------------------------------------------

	void
	addBeamStrip(CXR_BeamStrip& beamstrip)
	{
		if (m_NumBeamStrips >= m_MaxNumBeamStrips)
			return;

		m_BeamStrips[m_NumBeamStrips] = beamstrip;

		m_NumBeamStrips++;
	}

	//----------------------------------------------------------------------

	void
	generateBlade(const CMat43fp32& WorldToCamera)
	{
		fp32 alpha;
		fp32 trailfraction, bladefraction;
		int32 iTrail, iSegment;

		fp32 TriangleSplitFlipPosition = sqrtf(0.5f);

		for (iSegment = 0; iSegment <= NumBladeSegments; iSegment++)
		{
			bladefraction = (fp32)iSegment / (fp32)NumBladeSegments;

			int32 randseed_temp = m_Randseed;

			for (iTrail = -1; iTrail < NumTrailSteps; iTrail++)
			{
				trailfraction = (fp32)iTrail / (fp32)(NumTrailSteps - 1);

				fp32 height = M_Sin(bladefraction * bladefraction * _PI);

				if ((trailfraction < height) && (iTrail >= 0))
					alpha = 1.0f - (trailfraction / height);
				else
					alpha = 0.0f;

				m_Randseed = m_RandseedBase + iSegment;

				CVec3Dfp32 dpos;
				dpos.k[0] = (getSinRandTable(m_PosTimeScale, -trailfraction) - 0.5f);
				dpos.k[1] = (getSinRandTable(m_PosTimeScale, -trailfraction) - 0.5f);
				dpos.k[2] = (getSinRandTable(m_PosTimeScale, -trailfraction) - 0.5f);

				CXR_BeamStrip beamstrip;
				if (iTrail == -1) {
					beamstrip.m_Pos = m_TrailPos[0] - CVec3Dfp32(0, 0, 1) + m_TrailDir[0] * m_Length * bladefraction;
					beamstrip.m_Width = m_FlameStartWidth;
					beamstrip.m_TextureYOfs = (m_Time + getRand()) * m_TexVScroll;
					beamstrip.m_Flags = CXR_BEAMFLAGS_BEGINCHAIN;

					CVec3Dfp32 temp = m_TrailDir[0];
					temp.MultiplyMatrix3x3(WorldToCamera);
					fp32 aligned = getAbs(temp.k[0]);
					beamstrip.m_Width *= aligned + (1.0f - aligned) * m_FlameUnalignedWidth;
					ConOut(CStrF("aligned = %f", aligned));
				} else {
					dpos *= trailfraction * m_PosAmp;
					beamstrip.m_Pos = m_TrailPos[iTrail] + m_TrailDir[iTrail] * m_Length * bladefraction + dpos;
					beamstrip.m_Width = m_FlameStartWidth + (m_FlameEndWidth - m_FlameStartWidth) * trailfraction;
					beamstrip.m_TextureYOfs = trailfraction * m_TexVRepeats + (m_Time + getRand()) * m_TexVScroll;
					beamstrip.m_Flags = 0;

					CVec3Dfp32 temp = m_TrailDir[iTrail];
					temp.MultiplyMatrix3x3(WorldToCamera);
					fp32 aligned = getAbs(temp.k[0]);
					beamstrip.m_Width *= aligned + (1.0f - aligned) * m_FlameUnalignedWidth;
					ConOut(CStrF("aligned = %f", aligned));
				}
				beamstrip.m_Color = 0x00FFFFFF + ((int32)(alpha * 255.0f) << 24);
				addBeamStrip(beamstrip);

			}

			m_Randseed = randseed_temp;
		}
	}
	
	//----------------------------------------------------------------------

	bool
	fillVB(const CXR_AnimState* pAnimState, 
		   const CMat43fp32& LocalToWorld, const CMat43fp32& WorldToCamera,
		   CXR_BeamStrip* beamstrips, int32& numBeamstrips)
	{
		m_Time = pAnimState->m_AnimTime0;
		m_RandseedBase = pAnimState->m_Anim0;
		m_Randseed = m_RandseedBase;

		m_Length = 25.0f;
		m_FlameRise = 10.0f;
		m_FlameStartWidth = 10.0f;
		m_FlameEndWidth = 2.0f;
		m_FlameUnalignedWidth = 0.5f;

		m_TexURepeats = 2.0f;
		m_TexVRepeats = 1.0f;
		m_TexVScroll = -0.5f;

		m_PosAmp = 1.0f;
		m_PosTimeScale = 0.25f;

		m_BoundRadius = m_Length + m_FlameRise;

		m_SinRandTimeOffset = 1.0f;

		CMat43fp32 LocalToCamera;
		LocalToWorld.Multiply(WorldToCamera, LocalToCamera);

		m_Distance = LocalToCamera.k[3][2];

		if (!updateHistory(pAnimState, LocalToWorld))
			return false;

		m_BeamStrips = beamstrips;
		m_MaxNumBeamStrips = numBeamstrips;
		m_NumBeamStrips = 0;

		generateBlade(WorldToCamera);

		numBeamstrips = m_NumBeamStrips;

		return true;
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *surface)
	{
		if (surface != NULL) 
			m_iSurface = GetSurfaceID(surface);
		else
			m_iSurface = GetSurfaceID("FireBlade01");

		createSinRandTable();
	}

	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		CXR_BeamStrip	beamstrips[CXR_Util::MAXBEAMS];
		int32			numBeamStrips = CXR_Util::MAXBEAMS;

		if (fillVB(_pAnimState, _WMat, _VMat, beamstrips, numBeamStrips)) {
			CXR_VertexBuffer* pVB = AllocVB();
			if (pVB == NULL)
				return;

			CMat43fp32 unit;
			unit.Unit();

			if (CXR_Util::Render_BeamStrip2(m_pVBM, pVB, unit, _VMat, 
											beamstrips, numBeamStrips, 
//											CXR_BEAMFLAGS_EDGEFADE))
											0))
				Render_Surface(m_iSurface, pVB, _pAnimState->m_AnimTime0);
		}
	}
};

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_FireBlade, CXR_Model_Custom);

//----------------------------------------------------------------------
