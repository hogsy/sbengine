#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

//----------------------------------------------------------------------

#define getRand()		(MFloat_GetRand(m_Randseed++))
#define getSign(x)		(Sign(x))

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

	enum { NumTrailSteps = 10, NumBladeSegments = 9 };
	enum { MaxNumVertices = 128, MaxNumTriangles = 256 };

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
	fp32				m_TexURepeats, m_TexVRepeats, m_TexVScroll;
	fp32				m_TriangleSplitFlipPosition;

	int32			m_iVertex, m_iIndex;
	CVec3Dfp32*		m_pVertexPos;
	CVec2Dfp32*		m_pVertexTex;
	CPixel32*		m_pVertexColor;
	uint16*			m_pIndex;

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
	getSinRandTable(fp32 timescale)
	{
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
		m_pVertexPos[m_iVertex] = pos;
		m_pVertexTex[m_iVertex][0] = tu;
		m_pVertexTex[m_iVertex][1] = tv;
		m_pVertexColor[m_iVertex] = 0x00FFFFFF + ((int32)(alpha * 255.0f) << 24);
		m_iVertex++;
	}

	//----------------------------------------------------------------------

	inline
	void
	addTriangle(int32 v1, int32 v2, int32 v3)
	{
		m_pIndex[m_iIndex + 0] = v1;
		m_pIndex[m_iIndex + 1] = v2;
		m_pIndex[m_iIndex + 2] = v3;
		m_iIndex += 3;
	}

	//----------------------------------------------------------------------

	void
	generateBlade()
	{
		fp32 alpha;
		fp32 trailfraction, bladefraction;
		int32 iTrail, iSegment;

		for (iTrail = 0; iTrail < NumTrailSteps; iTrail++)
		{
			trailfraction = (fp32)iTrail / (fp32)(NumTrailSteps - 1);

			for (iSegment = 0; iSegment <= NumBladeSegments; iSegment++)
			{
				bladefraction = (fp32)iSegment / (fp32)NumBladeSegments;

				fp32 height;

				height = M_Sin(bladefraction * _PI) * 0.5f;
				if (bladefraction >= 0.4f)
					height += M_Sin((bladefraction - 0.4f) / 0.6f * _PI) * 0.5f;

				if (trailfraction < height)
					alpha = 1.0f - (trailfraction / height);
				else
					alpha = 0.0f;

				CVec3Dfp32 dpos;
				dpos.k[0] = m_PosAmp * (getSinRandTable(m_PosTimeScale) - 0.5f);
				dpos.k[1] = m_PosAmp * (getSinRandTable(m_PosTimeScale) - 0.5f);
				dpos.k[2] = m_PosAmp * (getSinRandTable(m_PosTimeScale) - 0.5f);

				addVertex(m_TrailPos[iTrail] + m_TrailDir[iTrail] * m_Length * bladefraction + dpos, 
						  bladefraction * m_TexURepeats, 
						  trailfraction * m_TexVRepeats + m_Time * m_TexVScroll, alpha);
			}
		}

		int32 v1, v2, v3, v4;

		for (iTrail = 0; iTrail < (NumTrailSteps - 1); iTrail++)
		{
			for (iSegment = 0; iSegment < (NumBladeSegments); iSegment++)
			{
				bladefraction = (fp32)iSegment / (fp32)(NumBladeSegments - 1);

				v1 = iTrail * (NumBladeSegments + 1) + iSegment;
				v2 = v1 + 1;
				v3 = v1 + (NumBladeSegments + 1);
				v4 = v2 + (NumBladeSegments + 1);

				if (bladefraction < m_TriangleSplitFlipPosition) {
					addTriangle(v1, v3, v4);
					addTriangle(v1, v4, v2);
				} else {
					addTriangle(v1, v2, v3);
					addTriangle(v2, v3, v4);
				}
			}
		}

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

	bool
	fillVB(const CXR_AnimState* pAnimState, 
		   const CMat43fp32& LocalToWorld, const CMat43fp32& WorldToCamera,
		   CXR_VertexBuffer* pVB)
	{
		m_Time = pAnimState->m_AnimTime0;
		m_RandseedBase = pAnimState->m_Anim0;
		m_Randseed = m_RandseedBase;

		m_Length = 60.0f;
		m_FlameRise = 30.0f;
		m_TexURepeats = 2.0f;
		m_TexVRepeats = 1.0f;
		m_TexVScroll = -0.5f;
 
		m_TriangleSplitFlipPosition = 0.7f;

		m_PosAmp = 2.0f;
		m_PosTimeScale = 0.25f;

		m_BoundRadius = m_Length + m_FlameRise;

		m_SinRandTimeOffset = 1.0f;

		CMat43fp32 LocalToCamera;
		LocalToWorld.Multiply(WorldToCamera, LocalToCamera);

		m_Distance = LocalToCamera.k[3][2];

		m_pVertexPos = m_pVBM->Alloc_V3(MaxNumVertices);
		if (!m_pVertexPos)
			return false;

		m_pVertexTex = m_pVBM->Alloc_V2(MaxNumVertices);
		if (!m_pVertexTex)
			return false;

		m_pVertexColor = m_pVBM->Alloc_CPixel32(MaxNumVertices);
		if (!m_pVertexColor)
			return false;

		m_pIndex = m_pVBM->Alloc_Int16(3 * MaxNumTriangles);
		if (!m_pIndex)
			return false;

		CMat4Dfp32 *pMatrix = m_pVBM->Alloc_M4();
		if(!pMatrix)
			return false;

		if (!updateHistory(pAnimState, LocalToWorld))
			return false;

		m_iVertex = 0;
		m_iIndex = 0;

		generateBlade();

		ConOut(CStrF("numVertices = %d, numTriangles = %d", m_iVertex, m_iIndex / 3));

		*pMatrix = WorldToCamera;
		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
		pVB->Matrix_Set(pMatrix);
		if (!pVB->SetVBChain(m_pVBM, false))
			return false;
		pVB->Geometry_VertexArray(m_pVertexPos, m_iVertex);
		pVB->Geometry_TVertexArray(m_pVertexTex, 0);
		pVB->Geometry_ColorArray(m_pVertexColor);
		pVB->Render_IndexedTriangles(m_pIndex, m_iIndex / 3);

		ConOut(CStr("Rendering..."));

		return true;
	}

	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *surface)
	{
		if (surface != NULL) 
			m_iSurface = m_pSurfaceContext->GetSurfaceID(surface);
		else
			m_iSurface = m_pSurfaceContext->GetSurfaceID("FireBlade01");

		createSinRandTable();
	}

	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		CXR_VertexBuffer* pVB = AllocVB();
		if (pVB == NULL)
			return;

		if (fillVB(_pAnimState, _WMat, _VMat, pVB))
			Render_Surface(m_iSurface, pVB, _pAnimState->m_AnimTime0);
	}
};

//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_FireBlade, CXR_Model_Custom);

//----------------------------------------------------------------------
