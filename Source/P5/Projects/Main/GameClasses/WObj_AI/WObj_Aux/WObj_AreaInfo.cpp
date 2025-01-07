#include "PCH.h"

#ifndef M_DISABLE_TODELETE
#include "WObj_AreaInfo.h"
#include "../../../../../Shared/MOS/Classes/GameWorld/Server/WServer.h"

//Gets this objects corresponding physics model, or NULL if physmodel is lacking
CXR_PhysicsModel * CWObject_AreaInfo::GetPhysModel()
{
	CXR_Model* pModel = m_pWServer->GetMapData()->GetResource_Model(m_PhysState.m_Prim[0].m_iPhysModel);
	if (!pModel)
		return NULL;

	return pModel->Phys_GetInterface();
};


//Constructor
CWObject_AreaInfo::CWObject_AreaInfo()
	: CWObject()
{
	m_iBSPModel = 0;	
};


void CWObject_AreaInfo::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	switch (_KeyHash)
	{
	case MHASH4('BSPM','ODEL','INDE','X'): // "BSPMODELINDEX"
		{
			CStr ModelName = CStrF("$WORLD:%d", _pKey->GetThisValuei());
	/*
			int XRMode = m_pWServer->Registry_GetGame()->GetValuei("XR_MODE", 0, 0);

			if( XRMode == 1 )
				m_iBSPModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel2(ModelName);
			else if( XRMode == 2 )
				m_iBSPModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel3(ModelName);
			else
				m_iBSPModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
	*/
			m_iBSPModel = m_pWServer->GetMapData()->GetResourceIndex_BSPModel(ModelName);
			if(!m_iBSPModel)
				Error("OnEvalKey", "Failed to acquire bsp-model.");
			break;
		}
	default:
		{
			CWObject::OnEvalKey(_pKey);
			break;
		}
	}
}



void CWObject_AreaInfo::OnFinishEvalKeys()
{
	CWObject::OnFinishEvalKeys();

	if(m_iBSPModel != 0)
	{
		if(m_iModel[0] != 0)
			m_iModel[0] = m_iBSPModel;

		// Setup physics
		CWO_PhysicsState Phys;
		Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_PHYSMODEL, m_iBSPModel, 0, 0);
		Phys.m_nPrim = 1;
		Phys.m_PhysFlags = 0;
		Phys.m_ObjectFlags = OBJECT_FLAGS_STATIC;
		Phys.m_ObjectIntersectFlags = 0;
		if (!m_pWServer->Object_SetPhysics(m_iObject, Phys))
			LogFile("WARNING: Unable to set area info physics state.");
		
		// Set bound-box.
		if(m_iModel[0])
		{
			CBox3Dfp32 Box;
			CXR_Model *pModel = m_pWServer->GetMapData()->GetResource_Model(m_iModel[0]);
			if(pModel)
			{
				pModel->GetBound_Box(Box);
				m_pWServer->Object_SetVisBox(m_iObject, Box.m_Min, Box.m_Max);
			};
		};
	};
}


int CWObject_AreaInfo::OnMessage(const CWObject_Message& _Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_GET_POSITION:
		{
			//Param1 and the vecparams are the parameters for the getareaposition method, see below
			//Param0 should be a CVec3Dfp32 * were the result is stored. 
			CVec3Dfp32 * pRes = (CVec3Dfp32 *)_Msg.m_Param0;
			*pRes = GetAreaPosition(_Msg.m_Param1, _Msg.m_VecParam0, _Msg.m_VecParam1);
			return 1;
		};
	case OBJMSG_POSITION_IN_AREA:
		{
			//Param0 should be a bool * which is set to true if VecParam0 is 
			bool * pRes = (bool *)_Msg.m_Param0;
			*pRes = IsInArea(_Msg.m_VecParam0);
			return 1;
		};
	case OBJMSG_BOX_IN_AREA:
		{
			//Param0 should be a bool * which is set to true if the box spanned by VecParam0 and 
			//Vecparam1 intersects area.
			bool * pRes = (bool *)_Msg.m_Param0;
			*pRes = IsInArea(_Msg.m_VecParam0, _Msg.m_VecParam1);
			return 1;
		};
	case OBJMSG_OBJECT_IN_AREA:
		{
			//Param0 should be a bool * which is set to true if the objects (indexed by param1) 
			//bounding box intersects the area 
			bool * pRes = (bool *)_Msg.m_Param0;
			CWObject * pObj = m_pWServer->Object_Get(_Msg.m_Param1); 
			*pRes = IsInArea(pObj);
			return 1;
		};
	case OBJMSG_DEBUG_RENDER:
		{
			DebugRender(*((CPixel32*)_Msg.m_Param0), *((fp32*)_Msg.m_Param1), *((CPixel32*)_Msg.m_Param0));
		};
	default:
		return CWObject::OnMessage(_Msg);
	};
}

//Return position within the area info object, depending on mode etc, or CVec3Dfp32(_FP32_MAX)
//if fail to get position
CVec3Dfp32 CWObject_AreaInfo::GetAreaPosition(int _iMode, const CVec3Dfp32& _Pos, const CVec3Dfp32& _Dir)
{
	//FIX
	return CVec3Dfp32(_FP32_MAX);	
};

//Succeeds iff position is inside the area
bool CWObject_AreaInfo::IsInArea(const CVec3Dfp32& _Pos)
{
	/*//Get physics model
	CXR_PhysicsModel * pPhysModel = GetPhysModel();
	if (pPhysModel)
	{
		return pPhysModel->Phys_IntersectLine(_Pos, _Pos, 0xffffffff);
	}
	else
		//No physics model
		return false;*/
	
	//Use the below if phys_intersectbox is efficient
	return IsInArea(_Pos, _Pos);
};

//Succeeds iff object's bounding box intersects area
bool CWObject_AreaInfo::IsInArea(CWObject * pObj)
{
	if (pObj && pObj->GetAbsBoundBox())
		return IsInArea(pObj->GetAbsBoundBox()->m_Min, pObj->GetAbsBoundBox()->m_Max);
	else
		return false;
};

//Succeeds iff box spanned by the two positions intersects area
bool CWObject_AreaInfo::IsInArea(const CVec3Dfp32& _Min, const CVec3Dfp32& _Max)
{
	/*
	//Check if box intersects physmodel
	CXR_PhysicsModel * pPhysModel = GetPhysModel();
	if (pPhysModel)
	{
		//Make sure values are set correctly
		CVec3Dfp32 Min;
		CVec3Dfp32 Max;
		for (int i = 0; i < 3; i++)
		{
			Min[i] = Min(_Min[i], _Max[i]);
			Max[i] = Max(_Min[i], _Max[i]);
		};

		//Create non-aligned box
		CMat4Dfp32 Center;
		Center.Unit();
		CVec3Dfp32 Dim = (Max - Min) * 0.5;
		CVec3Dfp32(Min + Dim).SetRow(Center, 3);
		CPhysOBB Box;
		Box.SetPosition(Center);
		Box.SetDimensions(Dim);

		//Check intersection..FIX
		//pPhysModel->Phys_IntersectBox(Box, Box,)
	}
	else
		//No physics model, cannot intersect
		return false,
	*/

	//Check if box intersects object by getting all objects selected by that box

	CBox3Dfp32 Box(_Min, _Max);
	const CBox3Dfp32* pPhysBox = GetAbsBoundBox();
	return pPhysBox->IsInside(Box);

/*	int iSel;
	bool bRes = false;
	try
	{
		TSelection<CSelection::LARGE_BUFFER> Selection;
	}
	catch (CCException) //Assume this is stack overflow... 
	{
		//Since we couldn't set up a selection we can't find any objects.
		return false;
	};
	
	//Add all objects that intersects box, and check if this is one of them
	m_pWServer->Selection_AddBoundBox(Selection, OBJECT_FLAGS_STATIC, _Min, _Max);
	bRes = m_pWServer->Selection_ContainsObject(Selection, m_iObject);

	return bRes;*/
};



//Debug method. Render wire shape, using debug render methods. 
void CWObject_AreaInfo::DebugRender(CPixel32 _Colour, fp32 _Time, CPixel32 _BoundBoxColour)
{
	//Render bounding box
	const CBox3Dfp32 * BoundBox = GetAbsBoundBox();
	if (BoundBox && _BoundBoxColour)
	{
		m_pWServer->Debug_RenderAABB(*BoundBox, _BoundBoxColour, _Time);
	};

	//Render model ...fix
};



MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_AreaInfo, CWObject, 0x0100);

#endif
