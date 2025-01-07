#include "PCH.h"
#include "WBSP4Glass.h"
#include "WBSP4GlassDelaunay2D.h"
#include "../../Classes/Render/MWireContainer.h"


#if GLASS_OPTIMIZE_OFF
#pragma xrMsg("optimize off!")
#pragma optimize("", off)
#pragma inline_depth(0)
#endif


bool CXR_Model_BSP4Glass_Instance::Wallmark_Create(CXR_Model_BSP4Glass* _pModelGlass, const CMat4Dfp32& _GlassMat, const CXR_WallmarkDesc& _WM, const CMat4Dfp32& _Origin, fp32 _Tolerance, int _Flags, int _Material)
{
	return _pModelGlass->Wallmark_Create(this, _GlassMat, _WM, _Origin, _Tolerance, _Flags, _Material);
}


uint8 CXR_Model_BSP4Glass_Instance::Server_OnDamage(const int32& _iInstance, const uint32& _Damage)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::Server_OnDamage, XR_BSP4GLASS);

	// Glass is taking damage,
	if(_iInstance >= GLASS_MAX_INSTANCE)
		return GLASS_MSG_NA;

	CGlassModel& Model = m_lModels[_iInstance];
	
	// Glass in unbreakable, don't do much
	if(Model.m_Attrib.Attrib_Unbreakable())
		return GLASS_MSG_WALLMARK;

	// If we are using durability, check "health" and decrease it
	if(Model.m_Attrib.Attrib_UseDurability())
	{
		if(_Damage >= Model.m_Attrib.m_Durability)
		{
			Model.m_Attrib.m_Durability = 0;
			return GLASS_MSG_CRUSH_POINT;
		}
		else		
			Model.m_Attrib.m_Durability -= _Damage;

		return GLASS_MSG_WALLMARK;
	}

	// No attributes found which was satisfying enough for damage, just send crush msg so glass can be destroyed
	return GLASS_MSG_CRUSH_POINT;
}


bool CXR_Model_BSP4Glass_Instance::Server_CrushGlassSurface(uint8 _CrushType, int32 _iInstance, const CVec3Dfp32& _LocalPos, fp32 _Radius, const CVec3Dfp32& _BoxSize, int32 _GameTick, const CMat4Dfp32& _WMat, uint8 _CrushFlags)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::Server_CrushGlassSurface, XR_BSP4GLASS);

	// Remove any server sided phys testing.
	if(_iInstance >= GLASS_MAX_INSTANCE)
		return false;

	CGlassModel& Model = m_lModels[_iInstance];
	CGlassAttrib& Attrib = Model.m_Attrib;
	
	// Alloc some data if needed
	bool bResult = false;
	if(Attrib.Attrib_TimeBreaker())
	{
		CBSP4Glass_Grid& GlassGrid = Model.m_Grid;
		
		const int nV = Model.m_lV.Len() >> 1;
		GlassGrid.CreateGrid(Attrib, Model.m_lV.GetBasePtr(), nV, _WMat, 0, _CrushFlags);

		// Check for hit location
		switch (_CrushType)
		{
		case GLASS_CRUSH_CUBE:
			{
				TThinArray<uint32> lResult;
				
				CMat4Dfp32 BoxMat;
				CPhysOBB EnumBox;
				
				BoxMat.Unit();
				BoxMat.GetRow(3) = _LocalPos;
				EnumBox.SetPosition(BoxMat);
				EnumBox.SetDimensions(_BoxSize);

				int32 nResult = GlassGrid.Phys_EnumBox(EnumBox, _WMat, lResult);
				if (nResult > 0)
				{
					GlassGrid.InvalidateGrid(nResult, lResult.GetBasePtr(), _GameTick, NULL, _WMat);
					bResult = true;
				}
				break;
			}

		case GLASS_CRUSH_SPHERE:
			{
				TThinArray<uint32> lResult;
				int32 nResult = GlassGrid.Phys_EnumSphere(_LocalPos, _Radius, _WMat, lResult);
				if (nResult > 0)
				{
					GlassGrid.InvalidateGrid(nResult, lResult.GetBasePtr(), _GameTick, NULL, _WMat);
					bResult = true;
				}
				break;
			}

		case GLASS_CRUSH_POINT:
			{
				CBSP4Glass_Grid_CollisionInfo GridCInfo = GlassGrid.Phys_LineIntersectGrid(_LocalPos, _WMat);
				if (GridCInfo.IsValid())
				{
					GlassGrid.InvalidateGrid(GridCInfo, _GameTick, NULL, _WMat);
					bResult = true;
				}
				break;
			}
		}
	}
	else
		bResult = true;

	if (bResult)
		Attrib.Attrib_SetPhys(false);

	return bResult;
}


void CXR_Model_BSP4Glass_Instance::Server_Restore(const uint16 _iInstance)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::Server_Restore, XR_BSP4GLASS);

	if (_iInstance >= GLASS_MAX_INSTANCE)
		return;

	CGlassModel& Model = m_lModels[_iInstance];
	Model.m_Attrib.Attrib_SetPhys(true);
	Model.m_Grid.CleanGrid(false);
}


void CXR_Model_BSP4Glass_Instance::Server_Inactive(const uint16 _iInstance, bool _bInactive)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::Server_Inactive, XR_BSP4GLASS);

	if (_iInstance >= GLASS_MAX_INSTANCE)
		return;

	CGlassModel& Model = m_lModels[_iInstance];
	Model.m_Attrib.Attrib_SetInactive(_bInactive);
}


void CXR_Model_BSP4Glass_Instance::Server_RestoreAll()
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::Server_RestoreAll, XR_BSP4GLASS);

	TAP_RCD<CGlassModel> lModels = m_lModels;
	for (uint i = 0; i < lModels.Len(); i++)
	{
		CGlassModel& Model = lModels[i];
		Model.m_Attrib.Attrib_SetPhys(true);
		Model.m_Grid.CleanGrid(false);
	}
}


void CXR_Model_BSP4Glass_Instance::Client_Restore(const uint16 _iInstance)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::Client_Restore, XR_BSP4GLASS);

	if (_iInstance >= GLASS_MAX_INSTANCE)
		return;

	CGlassModel& Model = m_lModels[_iInstance];
	CGlassAttrib& Attrib = Model.m_Attrib;
	Attrib.Attrib_SetPhys(true);
	Attrib.Attrib_SetBaseRender(true);

	RemoveShardModel(&Model);
	Model.m_Grid.CleanGrid(false);
	Model.m_Grid.ResetSound();
}


void CXR_Model_BSP4Glass_Instance::Client_Inactive(const uint16 _iInstance, bool _bInactive)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::Client_Inactive, XR_BSP4GLASS);

	if (_iInstance >= GLASS_MAX_INSTANCE)
		return;

	CGlassModel& Model = m_lModels[_iInstance];
	Model.m_Attrib.Attrib_SetInactive(_bInactive);
}


void CXR_Model_BSP4Glass_Instance::Client_RestoreAll()
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::Client_RestoreAll, XR_BSP4GLASS);

	TAP_RCD<CGlassModel> lModels = m_lModels;
	for (uint i = 0; i < lModels.Len(); i++)
	{
		CGlassModel& Model = lModels[i];
		CGlassAttrib& Attrib = Model.m_Attrib;
		
		Model.m_spShardModel = NULL;
		Attrib.Attrib_SetPhys(true);
		Attrib.Attrib_SetBaseRender(true);
		Model.m_Grid.CleanGrid(false);
		Model.m_Grid.ResetSound();
	}

	m_lpShardModels.Clear();
}


int8 CXR_Model_BSP4Glass_Instance::Client_CrushGlassSurface(uint8 _CrushType, const CMat4Dfp32& _WMat, int32 _GameTick, int32 _iInstance, int32 _Seed, const CVec3Dfp32& _LocalPosition, const CVec3Dfp32& _Force, fp32 _ForceScale, fp32 _Radius, const CVec3Dfp32& _BoxSize, uint8 _CrushFlags)
{
	GLASS_MSCOPE(CXR_Model_BSP4Glass_Instance::Client_CrushGlassSurface, XR_BSP4GLASS);

	if(_iInstance >= GLASS_MAX_INSTANCE)
		return -1;

	CGlassModel* pModels = m_lModels.GetBasePtr();
	CGlassModel& Model = pModels[_iInstance];

	// Retrive some attributes from the glass instance
	CGlassAttrib& Attrib = Model.m_Attrib;
	const CXR_PlaneMapping* pMapping = Model.m_pMapping;
	const CVec3Dfp32& PlaneN	= Attrib.m_Plane.n;
	const int nV = Model.m_lV.Len() >> 1;
	// Create glass grid if needed
	#ifndef M_RTM
	CMTime TimeGlassGen;
	{
		TimeGlassGen.Reset();
		TimeGlassGen.Start();
	}
	#endif

	CBSP4Glass_Grid& GlassGrid = Model.m_Grid;
	GlassGrid.CreateGrid(Attrib, Model.m_lV.GetBasePtr(), nV, _WMat, 0, _CrushFlags);
		
	// Check for hit location
	const int32 nRefresh = GlassGrid.GetNumRefresh();
	
	if (_CrushType == GLASS_CRUSH_POINT)
	{
		const CBSP4Glass_Grid_CollisionInfo GridCInfo = GlassGrid.Phys_LineIntersectGrid(_LocalPosition, _WMat);
		if(GridCInfo.IsValid())
		{
			CGlassModelShard* pShardModel = AllocShardModel(&Model, _iInstance, _Seed, _WMat);
			
			// Construct mapping and tangent data
			CBSP4Glass_MappingSetup Mapping(*pMapping, Attrib);
			CBSP4Glass_TangentSetup Tangent(Mapping, PlaneN);
			
			pShardModel->UpdateMesh((int32)Model.m_iSurface, Attrib, GlassGrid, Tangent, Mapping, _WMat);
			pShardModel->UpdateVelocities(GridCInfo.m_iInside, GlassGrid, nRefresh, _Force);

			GlassGrid.InvalidateGrid(GridCInfo, _GameTick, pShardModel->m_lSrcPos.GetBasePtr(), _WMat);
		}
	}
	else if (_CrushType == GLASS_CRUSH_CUBE || _CrushType == GLASS_CRUSH_SPHERE)
	{
		TThinArray<uint32> lResult;
		uint32 nResult = 0;
		
		if (_CrushType == GLASS_CRUSH_CUBE)
		{
			CPhysOBB EnumBox;
			CMat4Dfp32 BoxMat;
			BoxMat.Unit();
			BoxMat.GetRow(3) = _LocalPosition;
			EnumBox.SetPosition(BoxMat);
			EnumBox.SetDimensions(_BoxSize);
			nResult = GlassGrid.Phys_EnumBox(EnumBox, _WMat, lResult);
		}
		else
			nResult = GlassGrid.Phys_EnumSphere(_LocalPosition, _Radius, _WMat, lResult);

		if (nResult > 0)
		{
			CGlassModelShard* pShardModel = AllocShardModel(&Model, _iInstance, _Seed, _WMat);

			CBSP4Glass_MappingSetup Mapping(*pMapping, Attrib);
			CBSP4Glass_TangentSetup Tangent(Mapping, PlaneN);

			pShardModel->UpdateMesh((int32)Model.m_iSurface, Attrib, GlassGrid, Tangent, Mapping, _WMat);
			pShardModel->UpdateVelocities(0, GlassGrid, nRefresh, _Force);

			GlassGrid.InvalidateGrid(nResult, lResult.GetBasePtr(), _GameTick, pShardModel->m_lSrcPos.GetBasePtr(), _WMat);
			GlassGrid.SetNextSound();
		}
	}

	#ifndef M_RTM
	{
		TimeGlassGen.Stop();
		//M_TRACEALWAYS("Generating glass took %f milliseconds\n", TimeGlassGen.GetTime());
	}
	#endif

	// Disable all collision testing with this model on client also
	Attrib.Attrib_SetPhys(false);

	int8 iPlaySound = GlassGrid.GetSound();
	GlassGrid.SetNextSound();
	
	// Turn off rendering for the base model
	Attrib.Attrib_SetBaseRender(false);
	if (0)
	{
		Model.m_lV.Clear();
		Model.m_lC.Clear();
		Model.m_lN.Clear();
		Model.m_lTangU.Clear();
		Model.m_lTangV.Clear();
		Model.m_lTV.Clear();
		Model.m_lFrontP.Clear();
		Model.m_lEdgeP.Clear();

		// No rendering of main model, it's broken...
		Model.m_nRenderMainPrim = 0;
		Model.m_nRenderEdgePrim = 0;
	}

	return iPlaySound;
}
