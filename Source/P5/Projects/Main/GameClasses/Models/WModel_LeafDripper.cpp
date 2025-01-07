#include "PCH.h"
#include "MFloat.h"
#include "../../../../Shared/MOS/XR/XR.h"
#include "../../../../Shared/MOS/XR/XRCustomModel.h"

//----------------------------------------------------------------------

#define getRand()		(MFloat_GetRand(randseed++))
#define getSign(x)		(Sign(x))

//----------------------------------------------------------------------
// CXR_Model_LeafDripper
//----------------------------------------------------------------------

class CXR_Model_LeafDripper : public CXR_Model_Custom
{

	MRTC_DECLARE;

protected:

	int32 m_iTexture;
	int32 m_iSurface;

public:

	enum
	{
		COLOR32_XSIZE_SHIFT = 21,
		COLOR32_YSIZE_SHIFT = 10,
		COLOR32_ZSIZE_SHIFT = 0,
		COLOR32_XSIZE_MASK = 0x7FF,
		COLOR32_YSIZE_MASK = 0x7FF,
		COLOR32_ZSIZE_MASK = 0x3FF,
	};

	enum { SinRandTableSize = 1024 };

//	enum { MaxNumLeaves = 512 };
	enum { MaxNumLeaves = 64 };

private:

	//----------------------------------------------------------------------

	fp32				xsize, ysize, height; // The dimensions of this model (height is temporary).
	int32			numEmitters; // The total number of emitters in this model.
	fp32				emitProb; // The probability of emitting a leaf at any moment.
	fp32				minEmitDelay; // Minimum delay between two emissions.
	fp32				lifespan; // How long a leaf lives, and thus how fast it falls.
	fp32				fadein_time, fadeout_time; // Duration of fadein & fadeout phases.
	fp32				inv_fadein_time, inv_fadeout_time;
	fp32				leaf_xsize, leaf_ysize, leaf_break; // Leaf size and "breaking height".
	fp32				emitter_spread; // Spread in distance from original emitter position.
	fp32				target_spread; // Spread in distance from downprojected position.
	fp32				pos_amp; // Spread in distance from strait travel path from emitter to target pos.
	fp32				rot_amp; // Spread in distance from strait travel path from emitter to target pos.

	int32			numLeaves; // Number of leaves emitted so far this frame.
	int32			minLeafCycles, maxLeafCycles, avgLeafCycles, numLeafCycles; // Leaf profiling.

	int32			numTotalCells; // Number of total randindex cells needed to cover whole model lifetime.
	int32			numCellsToBacktrace; // Number of randindex cells needed to backtrace to cover a lifespan.

	fp32				time; // Total lifetime of model.
	fp32				distance; // Distance from camera to model origin.
	int32			randseed, randseedbase;
	fp32				postimescale, rottimescale;
	fp32				phase_shift;

	int32			iVertex, iIndex;
	CVec3Dfp32*		pVertexPos;
	CVec2Dfp32*		pVertexTex;
	CPixel32*		pVertexColor;
	uint16*			pIndex;

	fp32				SinRandTable[SinRandTableSize];
	fp32				timeoffset;

	//----------------------------------------------------------------------

	void
	createSinRandTable()
	{
		MAUTOSTRIP(CXR_Model_LeafDripper_createSinRandTable, MAUTOSTRIP_VOID);
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

			SinRandTable[i] = 0.5f + 0.5f * (y / 0.875f);
		}
	}

	//----------------------------------------------------------------------

	fp32 getSinRandTable(fp32 timescale);
	void addVertex(CVec3Dfp32 pos, fp32 tu, fp32 tv, int32 alpha);
	void addTriangle(int32 v1, int32 v2, int32 v3);

	//----------------------------------------------------------------------

	void
	addLeaf(CVec3Dfp32 pos, CQuatfp32 rot, fp32 alpha)
	{
		MAUTOSTRIP(CXR_Model_LeafDripper_addLeaf, MAUTOSTRIP_VOID);
		if (numLeaves >= MaxNumLeaves) {
//			ConOut(CStrF("LeafDripper: Too Many Leaves!!! (%d/%d)", numLeaves, MaxNumLeaves));
			numLeaves++;
			return;
		}

		numLeaves++;

		CVec3Dfp32 rx, ry, rz;
		CMat43fp32 matrix;
		rot.CreateMatrix(matrix);
		rx.GetDirectionX(matrix);
		ry.GetDirectionY(matrix);
		rz.GetDirectionZ(matrix);

		int32 ialpha = (RoundToInt(alpha * 255.0f) << 24);

		CVec3Dfp32 half_rx_leaf_xsize = rx * leaf_xsize * 0.5f;
		CVec3Dfp32 half_ry_leaf_ysize = ry * leaf_ysize * 0.5f;

		addTriangle(0, 1, 3);
		addTriangle(0, 3, 2);
		addVertex(pos - half_rx_leaf_xsize, 0.0f, 0.0f, ialpha);
		addVertex(pos + half_ry_leaf_ysize + rz * leaf_break, 1.0f, 0.0f, ialpha);
		addVertex(pos - half_ry_leaf_ysize + rz * leaf_break, 0.0f, 1.0f, ialpha);
		addVertex(pos + half_rx_leaf_xsize, 1.0f, 1.0f, ialpha);
	}

	//----------------------------------------------------------------------

	bool
	fillVB(const CXR_AnimState* pAnimState, 
		   const CMat43fp32& LocalToWorld, const CMat43fp32& WorldToCamera,
		   CXR_VertexBuffer* pVB)
	{
		MAUTOSTRIP(CXR_Model_LeafDripper_fillVB, false);
		time = pAnimState->m_AnimTime0;
		randseedbase = pAnimState->m_Anim0;
		randseed = randseedbase;

		xsize = (pAnimState->m_Colors[3] >> COLOR32_XSIZE_SHIFT) & COLOR32_XSIZE_MASK;
		ysize = (pAnimState->m_Colors[3] >> COLOR32_YSIZE_SHIFT) & COLOR32_YSIZE_MASK;
		height = (pAnimState->m_Colors[3] >> COLOR32_ZSIZE_SHIFT) & COLOR32_ZSIZE_MASK;
		emitProb = (fp32)pAnimState->m_AnimAttr0 / 255.0f;
		numEmitters = xsize * ysize / 0x1000;

		m_BoundRadius = height + Max(xsize, ysize);

		fadein_time = 2.0f;
		fadeout_time = 5.0f;
		inv_fadein_time = 1.0f / fadein_time;
		inv_fadeout_time = 1.0f / fadeout_time;

		fp32 fallspeed = 35.0f; // heights per second.
		lifespan = (height / fallspeed) + fadein_time + fadeout_time;
		leaf_xsize = 10.0f;
		leaf_ysize = 10.0f;
		leaf_break = 2.5f;
		emitter_spread = 50.0f;
		target_spread = 50.0f;
		pos_amp = 30.0f;
		rot_amp = 2.0f;

		minEmitDelay = 0.5f;

		// Calculate number of randindex cells needed to cover a whole lifespan.
		// Each cell is minEmitDelay long in time.
		numCellsToBacktrace = TruncToInt(lifespan / minEmitDelay) + 1;
		numTotalCells = TruncToInt(time / minEmitDelay);

		CMat43fp32 LocalToCamera;
		LocalToWorld.Multiply(WorldToCamera, LocalToCamera);

		distance = LocalToCamera.k[3][2];

		postimescale = 0.1f;
		rottimescale = 0.1f;
		timeoffset = 50.0f;

		pVertexPos = m_pVBM->Alloc_V3(4 * MaxNumLeaves);
		if (!pVertexPos)
			return false;

		pVertexTex = m_pVBM->Alloc_V2(4 * MaxNumLeaves);
		if (!pVertexTex)
			return false;

		pVertexColor = m_pVBM->Alloc_CPixel32(4 * MaxNumLeaves);
		if (!pVertexColor)
			return false;

		pIndex = m_pVBM->Alloc_Int16(6 * MaxNumLeaves);
		if (!pIndex)
			return false;

		CMat4Dfp32 *pMatrix = m_pVBM->Alloc_M4(WorldToCamera);
		if(!pMatrix)
			return false;

		numLeaves = 0;
		iVertex = 0;
		iIndex = 0;

		minLeafCycles = 10000000;
		maxLeafCycles = 0;
		avgLeafCycles = 0;
		numLeafCycles = 0;

		processEmitters(LocalToWorld);

		pVB->m_pAttrib->Attrib_Disable(CRC_FLAGS_CULL);
		pVB->Matrix_Set(pMatrix);
		if (!pVB->SetVBChain(m_pVBM, false))
			return false;
		pVB->Geometry_VertexArray(pVertexPos, iVertex, true);
		pVB->Geometry_TVertexArray(pVertexTex, 0);
		pVB->Geometry_ColorArray(pVertexColor);
		pVB->Render_IndexedTriangles(pIndex, iIndex / 3);

//		ConOut(CStrF("LeafCycles: %d min, %d max, %d avg", minLeafCycles, maxLeafCycles, avgLeafCycles / numLeafCycles));
//		ConOut(CStrF("numLeaves: %d", numLeaves));

		return true;
	}

	//----------------------------------------------------------------------

	inline
	void
	processEmitters(const CMat43fp32& LocalToWorld)
	{
		MAUTOSTRIP(CXR_Model_LeafDripper_processEmitters, MAUTOSTRIP_VOID);
		for (int i = 0; i < numEmitters; i++) {
			randseed = randseedbase + i * numCellsToBacktrace;
			processEmitter(LocalToWorld);
		}
	}
	
	//----------------------------------------------------------------------

	inline
	void
	processEmitter(const CMat43fp32& LocalToWorld)
	{
		MAUTOSTRIP(CXR_Model_LeafDripper_processEmitter, MAUTOSTRIP_VOID);
		CVec3Dfp32 emitter_pos;

		// Spread emitter in local layout quad.
		emitter_pos.k[0] = xsize * (getRand() - 0.5f);
		emitter_pos.k[1] = ysize * (getRand() - 0.5f);
		emitter_pos.k[2] = 0;
		emitter_pos.MultiplyMatrix(LocalToWorld); // Rotate and translate into worldspace.

		int cell = randseed + numTotalCells;

		for (int i = 0; i < numCellsToBacktrace; i++) {
			randseed = cell - i;
			if (getRand() < emitProb) {
				randseed = cell - i;

				fillLeaf(emitter_pos, time - (numTotalCells - (fp32)i) * minEmitDelay);
			}
		}
	}
	
	//----------------------------------------------------------------------

	inline
	void
	fillLeaf(CVec3Dfp32 emitter_pos, fp32 leaftime)
	{
		MAUTOSTRIP(CXR_Model_LeafDripper_fillLeaf, MAUTOSTRIP_VOID);
		int64 leafprofile;
		leafprofile = GetCPUClock();

		CVec3Dfp32 target_pos;

		// Downproject and sidespread target.
		target_pos.k[0] = emitter_pos.k[0] + target_spread * (getRand() - 0.5f);
		target_pos.k[1] = emitter_pos.k[1] + target_spread * (getRand() - 0.5f);
		target_pos.k[2] = emitter_pos.k[2] - height;

		// Sidespread emitter.
		emitter_pos.k[0] += emitter_spread * (getRand() - 0.5f);
		emitter_pos.k[1] += emitter_spread * (getRand() - 0.5f);

		fp32 t;
		fp32 alpha;
		CVec3Dfp32 pos;
		CQuatfp32 rot;

		if (leaftime < fadein_time) {
			t = 0.0f;
			alpha = leaftime * inv_fadein_time;
		} else if ((lifespan - leaftime) < fadeout_time) {
			t = 1.0f;
			alpha = (lifespan - leaftime) * inv_fadeout_time;
		} else {
			t = (leaftime - fadein_time) / (lifespan - fadein_time - fadeout_time);
			alpha = 1.0f;
		}

		if (alpha < 0.0f) alpha = 0.0f;
		if (alpha > 1.0f) alpha = 1.0f;

		pos = emitter_pos * (1.0f - t) + target_pos * t;
		rot.Unit();

		fp32 amp = alpha * 4.0f * M_Sin(t * _PI);
		if (amp > 1.0f) amp = 1.0f;

		fp32 amp_rot_amp = amp * rot_amp;
		fp32 one_minus_amp_rot_amp = 1.0f - amp_rot_amp;

		fp32 r1 = (getSinRandTable(postimescale) - 0.5f);
		fp32 r2 = (getSinRandTable(postimescale) - 0.5f);
		fp32 r3 = (getSinRandTable(rottimescale) - 0.5f);
		fp32 r4 = (getSinRandTable(rottimescale) - 0.5f);
//		fp32 r5 = (getSinRandTable(rottimescale) - 0.5f);
//		fp32 r6 = (getSinRandTable(rottimescale) - 0.5f);
		pos.k[0] += amp * pos_amp * r1;
		pos.k[1] += amp * pos_amp * r2;
		pos.k[2] += amp * pos_amp * r3;
		rot.k[0] += amp_rot_amp * r3 + one_minus_amp_rot_amp * getRand();
		rot.k[1] += amp_rot_amp * r4 + one_minus_amp_rot_amp * getRand();
		rot.k[2] += amp_rot_amp * r1 + one_minus_amp_rot_amp * getRand();
		rot.k[3] += amp_rot_amp * r2 + one_minus_amp_rot_amp * getRand();
		rot.Normalize();

		addLeaf(pos, rot, alpha);

		leafprofile = GetCPUClock() - leafprofile;

		if (leafprofile < minLeafCycles) minLeafCycles = leafprofile;
		if (leafprofile > maxLeafCycles) maxLeafCycles = leafprofile;
		avgLeafCycles += leafprofile;
		numLeafCycles++;

	}
	
	//----------------------------------------------------------------------
	
	virtual void OnCreate(const char *surface)
	{
		MAUTOSTRIP(CXR_Model_LeafDripper_OnCreate, MAUTOSTRIP_VOID);
		if (surface != NULL) 
			m_iSurface = GetSurfaceID(surface);
		else
			m_iSurface = GetSurfaceID("Leaf01");

		createSinRandTable();
	}

	//----------------------------------------------------------------------



	//----------------------------------------------------------------------

	virtual void Render(const CXR_AnimState* _pAnimState, const CMat43fp32 &_WMat, const CMat43fp32 &_VMat)
	{
		MAUTOSTRIP(CXR_Model_LeafDripper_Render, MAUTOSTRIP_VOID);
		MSCOPE(Mondelore, Mondelore);

		CXR_VertexBuffer* leafVB = AllocVB();
		if (leafVB == NULL)
			return;

		if (fillVB(_pAnimState, _WMat, _VMat, leafVB))
			Render_Surface(m_iSurface, leafVB, _pAnimState->m_AnimTime0);
	}
};

//----------------------------------------------------------------------
fp32 CXR_Model_LeafDripper::getSinRandTable(fp32 timescale)
{
	MAUTOSTRIP(CXR_Model_LeafDripper_getSinRandTable, 0);
	fp32 y, y1, y2;
	int32 x, xi, xf;

	x = RoundToInt(1023.0f * 255.0f * (time + timeoffset * getRand()) * timescale);

	xi = (x >> 8) & 0x3FF;
	xf = x & 0xFF;

	y1 = SinRandTable[xi];
	y2 = SinRandTable[(xi + 1) & 0x3FF];

	y = y1 + (y2 - y1) * (fp32)xf / 255.0f;

	return y;
}

void CXR_Model_LeafDripper::addVertex(CVec3Dfp32 pos, fp32 tu, fp32 tv, int32 alpha)
{
	MAUTOSTRIP(CXR_Model_LeafDripper_addVertex, MAUTOSTRIP_VOID);
	pVertexPos[iVertex] = pos;
	pVertexTex[iVertex][0] = tu;
	pVertexTex[iVertex][1] = tv;
	pVertexColor[iVertex] = 0x00FFFFFF + alpha;
	iVertex++;
}

void CXR_Model_LeafDripper::addTriangle(int32 v1, int32 v2, int32 v3)
{
	MAUTOSTRIP(CXR_Model_LeafDripper_addTriangle, MAUTOSTRIP_VOID);
	pIndex[iIndex + 0] = iVertex + v1;
	pIndex[iIndex + 1] = iVertex + v2;
	pIndex[iIndex + 2] = iVertex + v3;
	iIndex += 3;
}
//----------------------------------------------------------------------
	
MRTC_IMPLEMENT_DYNAMIC(CXR_Model_LeafDripper, CXR_Model_Custom);

//----------------------------------------------------------------------
