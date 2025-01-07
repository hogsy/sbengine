
#include "PCH.h"
#include "WBlockNavGrid.h"

//
MRTC_IMPLEMENT_DYNAMIC(CXR_BlockNav_Grid_GameWorld, CXR_BlockNav_Grid);
MRTC_IMPLEMENT_DYNAMIC(CXR_BlockNavSearcher_VPUDispatcher, CXR_BlockNavSearcher);
MRTC_IMPLEMENT_DYNAMIC(CXR_BlockNavSearcher_Plain, CXR_BlockNavSearcher);

//
CXR_BlockNav_Grid_GameWorld::CXR_BlockNav_Grid_GameWorld()
{
	MAUTOSTRIP(CXR_BlockNav_Grid_GameWorld_ctor, MAUTOSTRIP_VOID);
	m_pWPhysState = NULL;
	m_pCurrentRenderModel = NULL;
	m_pCurrentRenderTile = NULL;
}

void CXR_BlockNav_Grid_GameWorld::RenderModel_r(CXR_PhysicsContext* _pPhys, const CVec3Dint32& _Origin, int _Size) const
{
	MAUTOSTRIP(CXR_BlockNav_Grid_GameWorld_RenderModel_r, MAUTOSTRIP_VOID);
	CBox3Dfp32 Box;
	fp32 BoxSize = fp32(_Size)*m_UnitsPerCell;
	Box.m_Min[0] = m_CurrentTileWPos[0] + fp32(_Origin[0]) * m_UnitsPerCell;
	Box.m_Min[1] = m_CurrentTileWPos[1] + fp32(_Origin[1]) * m_UnitsPerCell;
	Box.m_Min[2] = m_CurrentTileWPos[2] + fp32(_Origin[2]) * m_UnitsPerCell;
	Box.m_Max[0] = Box.m_Min[0] + BoxSize;
	Box.m_Max[1] = Box.m_Min[1] + BoxSize;
	Box.m_Max[2] = Box.m_Min[2] + BoxSize;

	int MediumFlags = m_pCurrentRenderModel->Phys_GetCombinedMediumFlags(_pPhys,Box);
	if (MediumFlags & (XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_PLAYERSOLID | XW_MEDIUM_AISOLID | XW_MEDIUM_NAVGRIDFLAGS | XW_MEDIUM_DYNAMICSSOLID))
	{
		if ((_Size > 1) && 
			(MediumFlags & XW_MEDIUM_AIR))
		{
			// Subdivide
			_Size >>= 1;
			RenderModel_r(_pPhys, _Origin + CVec3Dint32(0,0,0),			_Size);
			RenderModel_r(_pPhys, _Origin + CVec3Dint32(_Size,0,0),		_Size);
			RenderModel_r(_pPhys, _Origin + CVec3Dint32(0,_Size,0),		_Size);
			RenderModel_r(_pPhys, _Origin + CVec3Dint32(_Size,_Size,0),	_Size);
			RenderModel_r(_pPhys, _Origin + CVec3Dint32(0,0,_Size),		_Size);
			RenderModel_r(_pPhys, _Origin + CVec3Dint32(_Size,0,_Size),	_Size);
			RenderModel_r(_pPhys, _Origin + CVec3Dint32(0,_Size,_Size),	_Size);
			RenderModel_r(_pPhys, _Origin + CVec3Dint32(_Size,_Size,_Size), _Size);
		}
		else
		{
			// FIXME: Optimize fill
#ifdef XR_NAVGRID_BYTECELL
			CDecompressedTile* pDT = m_pCurrentRenderTile;
			for(int x = 0; x < _Size; x++)
				for(int y = 0; y < _Size; y++)
					for(int z = 0; z < _Size; z++)
						pDT->SetAt(_Origin + CVec3Dint32(x,y,z), 0);

#else
			CXR_BlockNav_BitPlane* pBitPlanes = m_pCurrentRenderTile->m_lBitPlanes;
			int nBitPlanes = m_pCurrentRenderTile->m_nBitPlanes;

			// Fill
			for(int x = 0; x < _Size; x++)
				for(int y = 0; y < _Size; y++)
					for(int z = 0; z < _Size; z++)
						CXR_BlockNav_BitPlane::SetAt(_Origin + CVec3Dint32(x,y,z), pBitPlanes, nBitPlanes, 0);
#endif
		}
	}
}

void CXR_BlockNav_Grid_GameWorld::RenderBox(const CBox3Dfp32& _Box, int _CellValue)
{
	int x0 = TruncToInt((_Box.m_Min[0] - m_CurrentTileWPos[0]) * m_CellsPerUnit);
	int y0 = TruncToInt((_Box.m_Min[1] - m_CurrentTileWPos[1]) * m_CellsPerUnit);
	int z0 = TruncToInt((_Box.m_Min[2] - m_CurrentTileWPos[2]) * m_CellsPerUnit);
	int x1 = TruncToInt((_Box.m_Max[0] - m_CurrentTileWPos[0]) * m_CellsPerUnit + 1.0f - _FP32_EPSILON);
	int y1 = TruncToInt((_Box.m_Max[1] - m_CurrentTileWPos[1]) * m_CellsPerUnit + 1.0f - _FP32_EPSILON);
	int z1 = TruncToInt((_Box.m_Max[2] - m_CurrentTileWPos[2]) * m_CellsPerUnit + 1.0f - _FP32_EPSILON);

	if (x0 < 0) x0 = 0; else if(x0 >= XR_NAVTILE_DIM) return;
	if (x1 >= XR_NAVTILE_DIM) x1 = XR_NAVTILE_DIM-1; else if(x1 < 0) return;
	if (y0 < 0) y0 = 0; else if(y0 >= XR_NAVTILE_DIM) return;
	if (y1 >= XR_NAVTILE_DIM) y1 = XR_NAVTILE_DIM-1; else if(y1 < 0) return;
	if (z0 < 0) z0 = 0; else if(z0 >= XR_NAVTILE_DIM) return;
	if (z1 >= XR_NAVTILE_DIM) z1 = XR_NAVTILE_DIM-1; else if(z1 < 0) return;

	// Fill
#ifdef XR_NAVGRID_BYTECELL
	CDecompressedTile* pDT = m_pCurrentRenderTile;
	for(int x = x0; x <= x1; x++)
		for(int y = y0; y <= y1; y++)
			for(int z = z0; z <= z1; z++)
				pDT->SetAt(CVec3Dint32(x,y,z), _CellValue);

#else
	CXR_BlockNav_BitPlane* pBitPlanes = m_pCurrentRenderTile->m_lBitPlanes;
	int nBitPlanes = m_pCurrentRenderTile->m_nBitPlanes;

	for(int x = x0; x <= x1; x++)
		for(int y = y0; y <= y1; y++)
			for(int z = z0; z <= z1; z++)
				CXR_BlockNav_BitPlane::SetAt(CVec3Dint32(x,y,z), pBitPlanes, nBitPlanes, _CellValue);
#endif
}

void CXR_BlockNav_Grid_GameWorld::RenderOBB(const CBox3Dfp32& _Box, const CMat4Dfp32& _Pos, int _CellValue)
{
	CBox3Dfp32 Bound;
	_Box.Transform(_Pos, Bound); 


	int x0 = TruncToInt((Bound.m_Min[0] - m_CurrentTileWPos[0]) * m_CellsPerUnit);
	int y0 = TruncToInt((Bound.m_Min[1] - m_CurrentTileWPos[1]) * m_CellsPerUnit);
	int z0 = TruncToInt((Bound.m_Min[2] - m_CurrentTileWPos[2]) * m_CellsPerUnit);
	int x1 = TruncToInt((Bound.m_Max[0] - m_CurrentTileWPos[0]) * m_CellsPerUnit + 1.0f - _FP32_EPSILON);
	int y1 = TruncToInt((Bound.m_Max[1] - m_CurrentTileWPos[1]) * m_CellsPerUnit + 1.0f - _FP32_EPSILON);
	int z1 = TruncToInt((Bound.m_Max[2] - m_CurrentTileWPos[2]) * m_CellsPerUnit + 1.0f - _FP32_EPSILON);
	if (x0 < 0) x0 = 0; else if(x0 >= XR_NAVTILE_DIM) return;
	if (x1 >= XR_NAVTILE_DIM) x1 = XR_NAVTILE_DIM-1; else if(x1 < 0) return;
	if (y0 < 0) y0 = 0; else if(y0 >= XR_NAVTILE_DIM) return;
	if (y1 >= XR_NAVTILE_DIM) y1 = XR_NAVTILE_DIM-1; else if(y1 < 0) return;
	if (z0 < 0) z0 = 0; else if(z0 >= XR_NAVTILE_DIM) return;
	if (z1 >= XR_NAVTILE_DIM) z1 = XR_NAVTILE_DIM-1; else if(z1 < 0) return;

	CVec3Dfp32 TraceOrigin(m_CurrentTileWPos);
	TraceOrigin[0] -= m_UnitsPerTile*8.0f;
	TraceOrigin[1] += m_UnitsPerCell*0.5f;
	TraceOrigin[2] += m_UnitsPerCell*0.5f;
	CVec3Dfp32 TraceX(m_UnitsPerTile*16.0f, 0, 0);
	CVec3Dfp32 TraceY(0, m_UnitsPerCell, 0);
	CVec3Dfp32 TraceZ(0, 0, m_UnitsPerCell);

	CMat4Dfp32 PosInv;
	_Pos.InverseOrthogonal(PosInv);
	TraceOrigin.MultiplyMatrix(PosInv);
	TraceX.MultiplyMatrix3x3(PosInv);
	TraceY.MultiplyMatrix3x3(PosInv);
	TraceZ.MultiplyMatrix3x3(PosInv);

	fp32 xscale = Sqr(1.0f / (m_UnitsPerTile * 16.0f)) * 16.0f * fp32(XR_NAVTILE_DIM);
	fp32 xadd = -8.0f*fp32(XR_NAVTILE_DIM);

#ifdef XR_NAVGRID_BYTECELL
	CDecompressedTile* pDT = m_pCurrentRenderTile;

#else
	CXR_BlockNav_BitPlane* pBitPlanes = m_pCurrentRenderTile->m_lBitPlanes;
	int nBitPlanes = m_pCurrentRenderTile->m_nBitPlanes;
#endif

	for(int z = z0; z <= z1; z++)
		for(int y = y0; y <= y1; y++)
		{
			CVec3Dfp32 TracePos0, TracePos1;
			TraceOrigin.Combine(TraceY, fp32(y), TracePos0);
			TracePos0.Combine(TraceZ, fp32(z), TracePos0);
			TracePos0.Add(TraceX, TracePos1);

			CVec3Dfp32 HitPos0, HitPos1;
			if (_Box.IntersectLine(TracePos0, TracePos1, HitPos0))
				if (_Box.IntersectLine(TracePos1, TracePos0, HitPos1))
				{
					HitPos0 -= TraceOrigin;
					HitPos1 -= TraceOrigin;
					fp32 fx0 = (HitPos0*TraceX) * xscale + xadd;
					fp32 fx1 = (HitPos1*TraceX) * xscale + xadd;
					if (fx0 > fx1)
						Swap(fx0, fx1);

					int x0 = TruncToInt(fx0);
					if (x0 < 0) x0 = 0; else if(x0 >= XR_NAVTILE_DIM) continue;
					int x1 = TruncToInt(fx1 + 1.0f - _FP32_EPSILON);
					if (x1 >= XR_NAVTILE_DIM) x1 = XR_NAVTILE_DIM-1; else if(x1 < 0) continue;

#ifdef XR_NAVGRID_BYTECELL
					for(int x = x0; x < x1; x++)
						pDT->SetAt(CVec3Dint32(x,y,z), _CellValue);

#else
					for(int x = x0; x < x1; x++)
						CXR_BlockNav_BitPlane::SetAt(CVec3Dint32(x,y,z), pBitPlanes, nBitPlanes, _CellValue);

#endif

				}
		}
}

void CXR_BlockNav_Grid_GameWorld::Create(int _nCachedTiles, CWorld_PhysState* _pWPhysState)
{
	MAUTOSTRIP(CXR_BlockNav_Grid_GameWorld_Create, MAUTOSTRIP_VOID);
	m_pWPhysState = _pWPhysState;
	CXR_BlockNav_Grid::Create(_nCachedTiles);
}

#define XR_NAVGRID_NEWFILL

void CXR_BlockNav_Grid_GameWorld::OnPostDecompressTile(CDecompressedTile& _Tile)
{
	MAUTOSTRIP(CXR_BlockNav_Grid_GameWorld_OnPostDecompressTile, MAUTOSTRIP_VOID);
	m_pCurrentRenderTile = &_Tile;

	CBox3Dfp32 Box;
	Box.m_Min[0] = fp32(_Tile.m_TilePos[0]) * m_UnitsPerTile + m_Origin[0];
	Box.m_Min[1] = fp32(_Tile.m_TilePos[1]) * m_UnitsPerTile + m_Origin[1];
	Box.m_Min[2] = fp32(_Tile.m_TilePos[2]) * m_UnitsPerTile + m_Origin[2];
	Box.m_Max[0] = fp32(_Tile.m_TilePos[0]+1) * m_UnitsPerTile + m_Origin[0];
	Box.m_Max[1] = fp32(_Tile.m_TilePos[1]+1) * m_UnitsPerTile + m_Origin[1];
	Box.m_Max[2] = fp32(_Tile.m_TilePos[2]+1) * m_UnitsPerTile + m_Origin[2];
	m_CurrentTileWPos = Box.m_Min;

//	CMTime Time;
//	Time.Start();

	// Create a selection with all objects within 'Box' with OBJECT_FLAGS_NAVIGATION flags.
	TSelection<CSelection::LARGE_BUFFER> Selection;

	m_pWPhysState->Selection_AddBoundBox(Selection, OBJECT_FLAGS_NAVIGATION, Box.m_Min, Box.m_Max);
//	m_pWPhysState->Selection_AddBoundBox(Selection, OBJECT_FLAGS_PHYSMODEL, Box.m_Min, Box.m_Max);
	const int16* piObj;
	int nObj = m_pWPhysState->Selection_Get(Selection, &piObj);
	int nModelsRendered = 0;
	int nQuickModels = 0;

	bool bModelFound = false;
	for(int i = 0; i < nObj; i++)
	{
		CWObject_CoreData* pObj = m_pWPhysState->Object_GetCD(piObj[i]);
		if (!pObj) continue;

		const CWO_PhysicsState& PhysState = pObj->GetPhysState();

		if (PhysState.m_ObjectFlags & (OBJECT_FLAGS_WORLD))// | OBJECT_FLAGS_TRIGGER))
			continue;

		// NOTE: only render model 0
		if(PhysState.m_nPrim)
		{
			int iPrim = 0;
			int iPhysModel = PhysState.m_Prim[iPrim].m_iPhysModel;
			CXR_Model* pModel = m_pWPhysState->GetMapData()->GetResource_Model(iPhysModel);
			if (!pModel) continue;

			CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();
			if (!pPhysModel) continue;

			if (!bModelFound)
			{
				// Clear traversable flag
				_Tile.ClearBitPlane(1);
				bModelFound = true;
			}

			_Tile.SetMinBitPlanes(1);

#ifdef XR_NAVGRID_NEWFILL
			CVec3Dfp32 ObjExt;
			pObj->GetAbsBoundBox()->m_Max.Sub(pObj->GetAbsBoundBox()->m_Min, ObjExt);
			fp32 MaxExt = Max3(ObjExt[0], ObjExt[1], ObjExt[2]);
			if (MaxExt < m_UnitsPerCell * 2.0f)
			{
				if (PhysState.m_ObjectFlags & (OBJECT_FLAGS_OBJECT))
				{
					RenderBox(*pObj->GetAbsBoundBox(),XR_CELL_DIE);
				}
				else
				{
					RenderBox(*pObj->GetAbsBoundBox(),0);
				}
				nQuickModels++;
			}
			else
			{
				CBox3Dfp32 Box;
				pPhysModel->GetBound_Box(Box, PhysState.m_Prim[iPrim].m_PhysModelMask, NULL);
				Box.Grow(m_UnitsPerCell * 0.5f);
				if (PhysState.m_ObjectFlags & (OBJECT_FLAGS_OBJECT))
				{
					RenderOBB(Box, pObj->GetPositionMatrix(),XR_CELL_DIE);
				}
				else
				{
					RenderOBB(Box, pObj->GetPositionMatrix(),0);
				}
			}
#else
			CXR_PhysicsContext PhysContext(pObj->GetPositionMatrix());
			PhysContext.m_PhysGroupMaskThis = PhysState.m_Prim[iPrim].m_PhysModelMask;
			pPhysModel->Phys_Init(&PhysContext);
			m_pCurrentRenderModel = pPhysModel;
			RenderModel_r(&PhysContext, CVec3Dint32(0), XR_NAVTILE_DIM);
#endif
			nModelsRendered++;
		}
	}

	m_pCurrentRenderModel = NULL;
	m_pCurrentRenderTile = NULL;

//	Time.Stop();
//	ConOut(CStrF("GridRender (%d, %d, %d), Models %d (%d quick), Time %.3f ms", _Tile.m_TilePos[0], _Tile.m_TilePos[1], _Tile.m_TilePos[2], nModelsRendered, nQuickModels, Time.GetTime() * 1000.0f));
//	M_TRACE(CStrF("GridRender (%d, %d, %d), Models %d (%d quick), Time %.3f ms\n", _Tile.m_TilePos[0], _Tile.m_TilePos[1], _Tile.m_TilePos[2], nModelsRendered, nQuickModels, Time.GetTime() * 1000.0f));
}


void CXR_BlockNav_Grid_GameWorld::InvalidateBox(const CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CXR_BlockNav_Grid_GameWorld_InvalidateBox, MAUTOSTRIP_VOID);
	int xmin = TruncToInt((_Box.m_Min[0] - m_Origin[0]) * m_TilesPerUnit);
	int ymin = TruncToInt((_Box.m_Min[1] - m_Origin[1]) * m_TilesPerUnit);
	int zmin = TruncToInt((_Box.m_Min[2] - m_Origin[2]) * m_TilesPerUnit);
	int xmax = TruncToInt((_Box.m_Max[0] - m_Origin[0]) * m_TilesPerUnit);
	int ymax = TruncToInt((_Box.m_Max[1] - m_Origin[1]) * m_TilesPerUnit);
	int zmax = TruncToInt((_Box.m_Max[2] - m_Origin[2]) * m_TilesPerUnit);

/*#ifndef M_RTM
	if (xmin < 0 || ymin < 0 || zmin < 0 || 
		(xmax << XR_NAVTILE_DIMSHIFT) > m_CellGridDim[0] ||
		(ymax << XR_NAVTILE_DIMSHIFT) > m_CellGridDim[1] ||
		(zmax << XR_NAVTILE_DIMSHIFT) > m_CellGridDim[2])
	{
		ConOut(CStrF("§c0f0NOTE: Nav-grid invalidation outside grid. (%d,%d,%d - %d,%d,%d)", xmin, ymin, zmin, xmax, ymax, zmax));
	}
#endif*/
	
	for(int x = xmin; x <= xmax; x++)
		for(int y = ymin; y <= ymax; y++)
			for(int z = zmin; z <= zmax; z++)
			{
				int iDT = Hash_GetDT(CVec3Dint32(x, y, z));
				if (iDT >= 0)
				{
					CDecompressedTile& DT = m_lDecompressedTiles[iDT];		
					Hash_RemoveDT(iDT);
					DT.Reset();
				}
			}
}

//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------

#include "VPUWorkerNavGrid_Params.h"
#include "../../../MCC/MRTC_VPUManager.h"


CXR_BlockNavSearcher_VPUDispatcher::CVPUSearchInstance::CVPUSearchInstance()
{
	m_JobID = -1;
	m_bInUse = false;
	m_LastReleasedTick = -1;
	m_pGrid = 0x0;
	m_pBlockNav = 0x0;
	m_NumResultNodes = 0;
}

CXR_BlockNavSearcher_VPUDispatcher::CVPUSearchInstance::~CVPUSearchInstance()
{
}

void CXR_BlockNavSearcher_VPUDispatcher::CVPUSearchInstance::Create(CWO_SpaceEnum_RW_NavGrid *_pNavGridSpace, CXR_BlockNav_GridProvider* _Grid, CXR_BlockNav* _pBlockNav)
{
	m_pNavGridSpace = _pNavGridSpace;
	m_pGrid = _Grid;
	m_pBlockNav = _pBlockNav;
}

void CXR_BlockNavSearcher_VPUDispatcher::CVPUSearchInstance::Search_Create(const CXBN_SearchParams& _Params)
{
//	return;
	m_Params = _Params;

	CVPU_JobDefinition JobDef;
	CXR_BlockNav_Grid *pGrid = (CXR_BlockNav_Grid *)m_pGrid;

	//
	m_lResultBuffer.SetLen(MAX_RESULT_NODES);

	// grid data access
	JobDef.AddCacheBuffer(VPU_NAVGRID_PARAM_TILETILEINDEX, pGrid->m_liTileTiles.GetBasePtr(), pGrid->m_liTileTiles.Len()*XR_NAVTILETILE_NUMCELLS);
	JobDef.AddCacheBuffer(VPU_NAVGRID_PARAM_TILETILES, pGrid->m_lTileTiles.GetBasePtr(), pGrid->m_lTileTiles.Len()*XR_NAVTILETILE_NUMCELLS);
	JobDef.AddCacheBuffer(VPU_NAVGRID_PARAM_TILES, pGrid->m_lTiles.GetBasePtr(), pGrid->m_lTiles.Len()*2);
	JobDef.AddCacheBuffer(VPU_NAVGRID_PARAM_BITPLANES, pGrid->m_liBitPlanes.GetBasePtr(), pGrid->m_liBitPlanes.Len());
	JobDef.AddCacheBuffer(VPU_NAVGRID_PARAM_BITPLANESDATA, pGrid->m_BitPlaneContainer.m_liBitPlaneData.GetBasePtr(), pGrid->m_BitPlaneContainer.m_liBitPlaneData.Len());
	JobDef.AddCacheBuffer(VPU_NAVGRID_PARAM_COMPRESSEDDATA, pGrid->m_BitPlaneContainer.m_lData.GetBasePtr(), pGrid->m_BitPlaneContainer.m_lData.Len());

	// Add the buffers
	JobDef.AddCacheBuffer(VPU_NAVGRID_PARAM_HASH1_LINKS		, (void*)m_pNavGridSpace->GetHash(0).GetLinksPtr()		, m_pNavGridSpace->GetHash(0).NumLinks());
	JobDef.AddCacheBuffer(VPU_NAVGRID_PARAM_HASH1_ELEMENTS	, (void*)m_pNavGridSpace->GetHash(0).GetElementsPtr()	, m_pNavGridSpace->GetHash(0).NumElements());
	JobDef.AddCacheBuffer(VPU_NAVGRID_PARAM_HASH1_IDS		, (void*)m_pNavGridSpace->GetHash(0).GetHashPtr()		, m_pNavGridSpace->GetHash(0).NumHash());

	JobDef.AddCacheBuffer(VPU_NAVGRID_PARAM_HASH2_LINKS		, (void*)m_pNavGridSpace->GetHash(1).GetLinksPtr()		, m_pNavGridSpace->GetHash(1).NumLinks());
	JobDef.AddCacheBuffer(VPU_NAVGRID_PARAM_HASH2_ELEMENTS	, (void*)m_pNavGridSpace->GetHash(1).GetElementsPtr()	, m_pNavGridSpace->GetHash(1).NumElements());
	JobDef.AddCacheBuffer(VPU_NAVGRID_PARAM_HASH2_IDS		, (void*)m_pNavGridSpace->GetHash(1).GetHashPtr()		, m_pNavGridSpace->GetHash(1).NumHash());

	// Settings
	m_lSettings.SetLen(2);
	m_lSettings[0] = m_pNavGridSpace->GetHash(0);
	m_lSettings[1] = m_pNavGridSpace->GetHash(1);
	JobDef.AddSimpleBuffer(VPU_NAVGRID_PARAM_HASH_SETTINGS, m_lSettings.GetBasePtr(), 2, VPU_IN_BUFFER);

	// parameters
	static CXBN_SearchParams Params;
	Params = _Params;
	JobDef.AddSimpleBuffer(VPU_NAVGRID_PARAM_SEARCHPARAMS, &Params, 1, VPU_IN_BUFFER);

	// layout
	static CXR_BlockNav_GridLayout Layout;
	Layout = *m_pGrid;
	JobDef.AddSimpleBuffer(VPU_NAVGRID_PARAM_GRIDLAYOUT, &Layout, 1, VPU_IN_BUFFER);

	// result buffer
	JobDef.AddSimpleBuffer(VPU_NAVGRID_PARAM_RESULTNODES, m_lResultBuffer.GetBasePtr(), MAX_RESULT_NODES, VPU_OUT_BUFFER);

	// return value (hoho. 64bit nastyness, UGLYYYYYYYYYYYYYYYYYYYY!!)
	m_NumResultNodes = 0;
	m_ReturnValue = 0;
#ifdef PLATFORM_PS3
	JobDef.AddLParams(VPU_NAVGRID_PARAM_RESULT, (uint32)&m_ReturnValue, (uint32)&m_NumResultNodes, (uint32)&m_Cost, 0);
#else
	JobDef.AddLParams(VPU_NAVGRID_PARAM_RESULT, (uint32 __w64)&m_ReturnValue, (uint32 __w64)&m_NumResultNodes, (uint32 __w64)&m_Cost, 0);
#endif

	// No Debugging
	JobDef.AddLParams(VPU_NAVGRID_PARAM_DEBUG, 0, 0, 0, 0);

	// Set job
	JobDef.SetJob(MHASH2('NAV','GRID'));

	// kick off
	m_JobID = MRTC_ThreadPoolManager::VPU_AddTask(JobDef, VpuAIContext);
}

int CXR_BlockNavSearcher_VPUDispatcher::CVPUSearchInstance::Search_Execute(int& _nNodeExpansions, int _nMaxExpansions, int _nMaxSlop)
{
	//
	if(MRTC_ThreadPoolManager::VPU_IsTaskComplete(m_JobID, VpuAIContext))
		return m_ReturnValue;
	return SEARCH_IN_PROGRESS;
}

bool CXR_BlockNavSearcher_VPUDispatcher::CVPUSearchInstance::Search_Get(CXBN_SearchResultCore *_pRes, bool _ReturnPartial)
{
	if(!_pRes)
		return false;
	//m_pGrid->m_UnitsPerCell;

	_pRes->Create(m_lResultBuffer.GetBasePtr(), m_NumResultNodes, m_Cost, m_pGrid->m_UnitsPerCell);
	return true;
}

int CXR_BlockNavSearcher_VPUDispatcher::CVPUSearchInstance::Search_GetUser()
{
	if (m_bInUse)
		return m_Params.m_iUser;
	else
		return 0;
}

// ----------------------------------------------------------------------------------------------------------------------------------

#include "WPhysState_NavGridHash.h"

#ifndef PLATFORM_PS3
	extern uint32 VPU_Worker_NavGrid(uint32 _JobHash, CVPU_JobInfo &_JobInfo);
#endif

CXR_BlockNavSearcher_VPUDispatcher::CXR_BlockNavSearcher_VPUDispatcher()
{
	m_pPhysState = 0x0;
	m_IsAsync = 1;
}


CXR_BlockNavSearcher_VPUDispatcher::~CXR_BlockNavSearcher_VPUDispatcher()
{
	for(unsigned i = 1; i < m_lspSearches.Len(); i++)
	{
		if(m_lspSearches[i]->InUse(-1))
		{
			int dummy = 0;
			while(m_lspSearches[i]->Search_Execute(dummy,0,0) == SEARCH_IN_PROGRESS)
				i = i;
		}
	}

	CSpaceEnumSpecialization_NavGrid::ms_pWorldData = 0x0;
}

void CXR_BlockNavSearcher_VPUDispatcher::UpdateNavGrid()
{
	CSpaceEnumSpecialization_NavGrid::ms_pWorldData = m_pPhysState->m_spMapData;

	// Update the navgrid hash
	const CIndexPool16::CHashIDInfo *pLinks = m_NavGridSpaceUpdates.GetLinks();
	for(int iLink = m_NavGridSpaceUpdates.GetFirst(); iLink != -1; iLink = pLinks[iLink].m_iNext)
	{
		int iObj = pLinks[iLink].m_iElem;
		m_NavGridSpace.Remove(iObj);

		CWObject_CoreData *pObj = m_pPhysState->Object_GetCD(iObj);

		// check all the criteras for the object to enter the hash
		if(!pObj)
			continue;
		
		if (pObj->IsDestroyed())
			continue;	

		const CWO_PhysicsState& PhysState = pObj->GetPhysState();
		if(!(PhysState.m_ObjectFlags & OBJECT_FLAGS_NAVIGATION))
			continue;

		if(!PhysState.m_nPrim)
			continue;

		int iPhysModel = PhysState.m_Prim[0].m_iPhysModel;
		CXR_Model *pModel = m_pPhysState->m_spMapData->GetResource_Model(iPhysModel);

		if(!pModel)
			continue;

		CXR_PhysicsModel *pPhysModel = pModel->Phys_GetInterface();
		if(!pPhysModel)
			continue;

		// ok.. insert the object
		m_NavGridSpace.Insert(pObj);
	}

	m_NavGridSpaceUpdates.RemoveAll();
}

CMapData *CSpaceEnumSpecialization_NavGrid::ms_pWorldData = 0x0;

void CXR_BlockNavSearcher_VPUDispatcher::Create(class CXR_BlockNav *_pBlockNav, int _MaxInstances)
{ 
	m_NavGridSpaceUpdates.Create(2560);
	m_NavGridSpace.Create(SERVER_HASH1_SIZE, SERVER_HASH1_BUCKET_SHIFT_SIZE, SERVER_HASH2_SIZE, SERVER_HASH2_BUCKET_SHIFT_SIZE, 2560, true);

	CXR_BlockNavSearcher::Create(_pBlockNav, _MaxInstances);
	m_lspSearches.SetLen(_MaxInstances + 1);

#ifndef PLATFORM_PS3
	MRTC_ThreadPoolManager::VPU_RegisterContext(VpuAIContext, VPU_Worker_NavGrid);
#endif

	for(int i = 1; i < m_lspSearches.Len(); i++)
	{
		m_lspSearches[i] = MNew(CVPUSearchInstance);
		m_lspSearches[i]->Create(&m_NavGridSpace, _pBlockNav->GetGrid(),  _pBlockNav);
	}
}

void CXR_BlockNavSearcher_VPUDispatcher::RegisterObjectMovement(int _iObj)
{
	m_NavGridSpaceUpdates.Insert(_iObj);
}

int CXR_BlockNavSearcher_VPUDispatcher::Search_Create(const CXBN_SearchParams& _Params, int _GameTick)
{
	MAUTOSTRIP(CXR_BlockNavSearcher_VPUDispatcher_Search_Create, 0);
	MSCOPESHORT(CXR_BlockNavSearcher_VPUDispatcher::Search_Create);
	for(int i = 1; i < m_lspSearches.Len(); i++)
	{
		if (!m_lspSearches[i]->InUse(_GameTick))
		{
			UpdateNavGrid();
			m_lspSearches[i]->m_bInUse = true;
			m_lspSearches[i]->Search_Create(_Params);
			return i;
		}
	}
	return 0;
}

int CXR_BlockNavSearcher_VPUDispatcher::Search_Execute(int _iSearch, int& _nNodeExpansions, int _nMaxExpansions,int _nMaxSlop)
{
	MAUTOSTRIP(CXR_BlockNavSearcher_VPUDispatcher_Search_Execute, 0);
	MSCOPESHORT(CXR_BlockNavSearcher_VPUDispatcher::Search_Execute);
	//Fail if index is invalid
	if ((!_iSearch) || (!m_lspSearches.ValidPos(_iSearch))) 
		return SEARCH_INSTANCE_INVALID;

	//Fail if search has been destroyed
	if (!m_lspSearches[_iSearch]->m_bInUse) 
		return SEARCH_INSTANCE_DESTROYED;
	
	//Ok, lets try to search
	return m_lspSearches[_iSearch]->Search_Execute(_nNodeExpansions, _nMaxExpansions, _nMaxSlop);
}


//Returns true if search is done and sets _Res to the search result
//If search is not done, false is returned and if _ReturnPartial is true
//then _Res is set to the current best partial path
bool CXR_BlockNavSearcher_VPUDispatcher::Search_Get(int _iSearch, CXBN_SearchResultCore* _pRes, bool _ReturnPartial)
{
	MAUTOSTRIP(CXR_BlockNavSearcher_VPUDispatcher_Search_Get, false);
	MSCOPESHORT(CXR_BlockNavSearcher_VPUDispatcher::Search_Get);
	//Fail if index is 0 or otherwise invalid
	if ((!_iSearch) || (!m_lspSearches.ValidPos(_iSearch)))
		return false;

	//Fail if search has been destroyed
	if (!m_lspSearches[_iSearch]->m_bInUse) 
		return false;

	return m_lspSearches[_iSearch]->Search_Get(_pRes, _ReturnPartial);
}


void CXR_BlockNavSearcher_VPUDispatcher::Search_Destroy(int _iSearch, int _GameTick)
{
	MAUTOSTRIP(CXR_BlockNavSearcher_VPUDispatcher_Search_Destroy, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_BlockNavSearcher_VPUDispatcher::Search_Destroy);
	if (!m_lspSearches.ValidPos(_iSearch)) return;
	if (!_iSearch) return;

	{
		int Dummy0 = 0;
		int Dummy1 = 0;
		int Dummy2 = 0;
		if(m_lspSearches[_iSearch]->Search_Execute(Dummy0, Dummy1, Dummy2) == SEARCH_IN_PROGRESS)
			return;
	}

	//CSpaceEnumSpecialization_NavGrid::ms_spWorldData = NULL;
	m_lspSearches[_iSearch]->Release(_GameTick);
}


//Get user of search instance
int CXR_BlockNavSearcher_VPUDispatcher::Search_GetUser(int _iSearch)
{
	MAUTOSTRIP(CXR_BlockNavSearcher_VPUDispatcher_Search_GetUser, 0);
	MSCOPESHORT(CXR_BlockNavSearcher_VPUDispatcher::Search_GetUser);
	if (!m_lspSearches.ValidPos(_iSearch)) 
		return 0;
	if (!_iSearch) 
		return 0;
	return m_lspSearches[_iSearch]->Search_GetUser();
}


//Get number of search instances
int CXR_BlockNavSearcher_VPUDispatcher::Search_GetNum()
{
	MAUTOSTRIP(CXR_BlockNavSearcher_VPUDispatcher_Search_GetNum, 0);
	return m_lspSearches.Len() - 1;
}

void CXR_BlockNavSearcher_VPUDispatcher::Debug_Render(CWireContainer* _pWC, CVec3Dfp32 _Pos)
{
	//UpdateNavGrid();

	CWO_EnumParams_Box Params;
	Params.m_Box.m_Min = _Pos-CVec3Dfp32(64,64,64);
	Params.m_Box.m_Max = _Pos+CVec3Dfp32(64,64,64);
	Params.m_ObjectFlags = 0; // these three parameters isn't used
	Params.m_ObjectIntersectFlags = 0;
	Params.m_ObjectNotifyFlags = 0;

	// debug render the box
	{
		CPixel32 color(0xff00ff00);
		_pWC->RenderAABB(Params.m_Box, color, 0.0f);
	}

	const int MaxHits = 128;
	CSpaceEnumSpecialization_NavGrid::EnumerateReturnType lObjects[MaxHits];
	int Hits = m_NavGridSpace.EnumerateBox(Params, lObjects, MaxHits);
	CPixel32 color(0xff0000ff);
	if(rand()%2)
		color = 0xffff00ff;
	for(int i = 0; i < Hits; i++)
	{

 		const CBox3Dfp32 &box = lObjects[i].m_Bounding;
		
		CVec3Dfp32 extent;
		extent.k[0] = Max(M_Fabs(box.m_Max.k[0]), M_Fabs(box.m_Min.k[0]));
		extent.k[1] = Max(M_Fabs(box.m_Max.k[1]), M_Fabs(box.m_Min.k[1]));
		extent.k[2] = Max(M_Fabs(box.m_Max.k[2]), M_Fabs(box.m_Min.k[2]));
		
		_pWC->RenderOBB(lObjects[i].m_Position, extent, color, 0.0f);
	}
}



//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------
//---------------------------------------------------------------------------------------------------------

void CXR_BlockNavSearcher_Plain::Create(class CXR_BlockNav *_pBlockNav, int _MaxInstances)
{ 
	CXR_BlockNavSearcher::Create(_pBlockNav, _MaxInstances);
	m_lspSearches.SetLen(_MaxInstances);

	m_lspSearches.SetLen(_MaxInstances+1);
	for(int i = 1; i < m_lspSearches.Len(); i++)
	{
		m_lspSearches[i] = MNew(CXBN_SearchInstance);
		if (!m_lspSearches[i]) MemError("Create");
		m_lspSearches[i]->Create(_pBlockNav->GetGrid(), _pBlockNav, 64, 4096);
	}
}

void CXR_BlockNavSearcher_Plain::RegisterObjectMovement(int _iObj)
{}

int CXR_BlockNavSearcher_Plain::Search_Create(const CXBN_SearchParams& _Params, int _GameTick)
{
	MAUTOSTRIP(CXR_BlockNavSearcher_Plain_Search_Create, 0);
	MSCOPESHORT(CXR_BlockNavSearcher_Plain::Search_Create);
	for(int i = 1; i < m_lspSearches.Len(); i++)
	{
		if (!m_lspSearches[i]->InUse(_GameTick))
		{
			/*
			if (m_lspSearches[i]->Search_Create(_Params))
			{
				return i;
			}
			*/
			m_lspSearches[i]->Search_Create(_Params);
			return i;
		}
	}
	return 0;
}

int CXR_BlockNavSearcher_Plain::Search_Execute(int _iSearch, int& _nNodeExpansions, int _nMaxExpansions,int _nMaxSlop)
{
	MAUTOSTRIP(CXR_BlockNavSearcher_Plain_Search_Execute, 0);
	MSCOPESHORT(CXR_BlockNavSearcher_Plain::Search_Execute);
	//Fail if index is invalid
	if ((!_iSearch) || (!m_lspSearches.ValidPos(_iSearch))) 
		return SEARCH_INSTANCE_INVALID;

	//Fail if search has been destroyed
	if (!m_lspSearches[_iSearch]->InUse(-1)) 
		return SEARCH_INSTANCE_DESTROYED;
	
	//Ok, lets try to search
	return m_lspSearches[_iSearch]->Search_Execute(_nNodeExpansions, _nMaxExpansions, _nMaxSlop);
}


//Returns true if search is done and sets _Res to the search result
//If search is not done, false is returned and if _ReturnPartial is true
//then _Res is set to the current best partial path
bool CXR_BlockNavSearcher_Plain::Search_Get(int _iSearch, CXBN_SearchResultCore* _pRes, bool _ReturnPartial)
{
	MAUTOSTRIP(CXR_BlockNavSearcher_Plain_Search_Get, false);
	MSCOPESHORT(CXR_BlockNavSearcher_Plain::Search_Get);
	//Fail if index is 0 or otherwise invalid
	if ((!_iSearch) || (!m_lspSearches.ValidPos(_iSearch)))
		return false;

	//Fail if search has been destroyed
	if (!m_lspSearches[_iSearch]->InUse(-1)) 
		return false;

	return m_lspSearches[_iSearch]->Search_Get(_pRes, _ReturnPartial);
}


void CXR_BlockNavSearcher_Plain::Search_Destroy(int _iSearch, int _GameTick)
{
	MAUTOSTRIP(CXR_BlockNavSearcher_Plain_Search_Destroy, MAUTOSTRIP_VOID);
	MSCOPESHORT(CXR_BlockNavSearcher_Plain::Search_Destroy);
	if (!m_lspSearches.ValidPos(_iSearch)) return;
	if (!_iSearch) return;
	m_lspSearches[_iSearch]->Release(_GameTick);
}


//Get user of search instance
int CXR_BlockNavSearcher_Plain::Search_GetUser(int _iSearch)
{
	MAUTOSTRIP(CXR_BlockNavSearcher_Plain_Search_GetUser, 0);
	MSCOPESHORT(CXR_BlockNavSearcher_Plain::Search_GetUser);
	if (!m_lspSearches.ValidPos(_iSearch)) 
		return 0;
	if (!_iSearch) 
		return 0;
	return m_lspSearches[_iSearch]->Search_GetUser();
}


//Get number of search instances
int CXR_BlockNavSearcher_Plain::Search_GetNum()
{
	MAUTOSTRIP(CXR_BlockNavSearcher_Plain_Search_GetNum, 0);
	return m_lspSearches.Len() - 1;
}

void CXR_BlockNavSearcher_Plain::Debug_Render(CWireContainer* _pWC, CVec3Dfp32 _Pos)
{
	/*
	CWO_EnumParams_Box Params;
	Params.m_Box.m_Min = _Pos-CVec3Dfp32(64,64,64);
	Params.m_Box.m_Max = _Pos+CVec3Dfp32(64,64,64);
	Params.m_ObjectFlags = 0; // these three parameters isn't used
	Params.m_ObjectIntersectFlags = 0;
	Params.m_ObjectNotifyFlags = 0;

	// debug render the box
	{
		CPixel32 color(0xff00ff00);
		_pWC->RenderAABB(Params.m_Box, color, 0.0f);
	}

	const int MaxHits = 128;
	CSpaceEnumSpecialization_NavGrid::EnumerateReturnType lObjects[MaxHits];
	int Hits = m_NavGridSpace.EnumerateBox(Params, lObjects, MaxHits);
	CPixel32 color(0xff0000ff);
	if(rand()%2)
		color = 0xffff00ff;
	for(int i = 0; i < Hits; i++)
	{

 		const CBox3Dfp32 &box = lObjects[i].m_Bounding;
		
		CVec3Dfp32 extent;
		extent.k[0] = Max(M_Fabs(box.m_Max.k[0]), M_Fabs(box.m_Min.k[0]));
		extent.k[1] = Max(M_Fabs(box.m_Max.k[1]), M_Fabs(box.m_Min.k[1]));
		extent.k[2] = Max(M_Fabs(box.m_Max.k[2]), M_Fabs(box.m_Min.k[2]));
		
		_pWC->RenderOBB(lObjects[i].m_Position, extent, color, 0.0f);
	}*/
}
