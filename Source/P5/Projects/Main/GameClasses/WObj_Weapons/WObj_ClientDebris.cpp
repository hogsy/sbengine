#include "PCH.h"

#ifndef M_DISABLE_CURRENTPROJECT

#include "WObj_ClientDebris.h"

void CWObject_ClientDebris::CreateSparks(const CVec3Dfp32 &_Pos, CWorld_Server *_pWServer)
{
	CNetMsg Msg(NETMSG_SPAWN_SPARKS);
	Msg.Addfp32(_Pos[0]);
	Msg.Addfp32(_Pos[1]);
	Msg.Addfp32(_Pos[2]);
	_pWServer->NetMsg_SendToClass(Msg, _pWServer->GetMapData()->GetResourceIndex_Class("ClientDebris"));
}

void CWObject_ClientDebris::CreateClientDebris(CCollisionInfo *_pInfo, const CVec3Dfp32 &_Dir, CWorld_Server *_pWServer)
{
	MSCOPESHORT(CWObject_ClientDebris::CreateClientDebris); //AR-SCOPE

//	return; // FIXME: Debris

	if(_pInfo && _pInfo->m_bIsValid)
	{
		if(_pInfo->m_pSurface && !(_pInfo->m_pSurface->GetBaseFrame()->m_Medium.m_MediumFlags & XW_MEDIUM_SKY))
		{
			CNetMsg Msg(NETMSG_SPAWN_SURFACEDEBRIS);
			CVec3Dfp32 Pos = _pInfo->m_Pos;
			Msg.Addfp32(_pInfo->m_Pos[0]);
			Msg.Addfp32(_pInfo->m_Pos[1]);
			Msg.Addfp32(_pInfo->m_Pos[2]);
			Msg.Addfp32(_pInfo->m_Plane.n[0]);
			Msg.Addfp32(_pInfo->m_Plane.n[1]);
			Msg.Addfp32(_pInfo->m_Plane.n[2]);

			Msg.Addfp32(_Dir[0]);
			Msg.Addfp32(_Dir[1]);
			Msg.Addfp32(_Dir[2]);
			_pWServer->NetMsg_SendToClass(Msg, _pWServer->GetMapData()->GetResourceIndex_Class("ClientDebris"));
		}
	}
}

void CWObject_ClientDebris::OnClientCreate(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient)
{
	CWObject_Ext_Model::OnClientCreate(_pObj, _pWClient);
	_pObj->m_iModel[0] = _pWClient->GetMapData()->GetResourceIndex_Model("SurfaceDebris");

	for(int i = 0; i < 4; i++)
	{
		int iModel = _pWClient->GetMapData()->GetResourceIndex_Model(CFStrF("Effects\\Scatter%i", int(Random * 4) + 1));
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(iModel);
		if(pModel)
			_pObj->m_Data[i] = ~int(pModel);
	}

	_pObj->m_iAnim0 = MRTC_RAND();
}

void CWObject_ClientDebris::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	_pWData->GetResourceIndex_Model("SurfaceDebris");
	_pWData->GetResourceIndex_Model("Effects\\Scatter1");
	_pWData->GetResourceIndex_Model("Effects\\Scatter2");
	_pWData->GetResourceIndex_Model("Effects\\Scatter3");
	_pWData->GetResourceIndex_Model("Effects\\Scatter4");
	_pWData->GetResourceIndex_Model("Effects\\Scatter5");
}

void CWObject_ClientDebris::OnClientNetMsg(CWObject_Client* _pObj, CWorld_Client* _pWClient, const CNetMsg& _Msg)
{
	if(_Msg.m_MsgType == NETMSG_SPAWN_SURFACEDEBRIS)
	{
		if(_pWClient->GetClientMode() == WCLIENT_MODE_MIRROR)
			return;

		//TODO: Skip creation of if camera is too far away

		CVec3Dfp32 Pos;
		int Index = 0;
		Pos[0] = _Msg.Getfp32(Index);
		Pos[1] = _Msg.Getfp32(Index);
		Pos[2] = _Msg.Getfp32(Index);
		CVec3Dfp32 In;
		In[0] = _Msg.Getfp32(Index);
		In[1] = _Msg.Getfp32(Index);
		In[2] = _Msg.Getfp32(Index);
		CVec3Dfp32 Dir;
		Dir[0] = _Msg.Getfp32(Index);
		Dir[1] = _Msg.Getfp32(Index);
		Dir[2] = _Msg.Getfp32(Index);

		CCollisionInfo Info;
		if(_pWClient->Phys_IntersectLine(Pos + In, Pos - In, 0, OBJECT_FLAGS_WORLD, XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID, &Info))
		{
			CVec3Dfp32 Res;
			Dir.Reflect(Info.m_Plane.n, Res);
			
			CMat4Dfp32 Mat;
			Mat.Unit();
			Res.SetMatrixRow(Mat, 0);
			CVec3Dfp32(0, 0, 1).SetMatrixRow(Mat, 2);
			Mat.RecreateMatrix(0, 2);
			(Pos + Res).SetMatrixRow(Mat, 3);
			int iObj = _pWClient->ClientObject_Create("ClientDebris", Mat);
			if(iObj > 0)
			{
				CWObject_Client *pObj = _pWClient->ClientObject_Get(iObj);
				if(pObj)
				{
					pObj->m_ClientData[0] = 1;
					pObj->m_Data[4] = int(Info.m_pSurface);
//					pObj->m_ClientFlags |= CLIENTFLAGS_V2;
//					strncpy((char *)pObj->m_Data, Info.m_pSurface->m_Name, 4 * 8);

				}
			}
		}
	}
	else if(_Msg.m_MsgType == NETMSG_SPAWN_SPARKS)
	{
		if(_pWClient->GetClientMode() == WCLIENT_MODE_MIRROR)
			return;

		CVec3Dfp32 Pos;
		int Index = 0;
		Pos[0] = _Msg.Getfp32(Index);
		Pos[1] = _Msg.Getfp32(Index);
		Pos[2] = _Msg.Getfp32(Index);

		CMat4Dfp32 Mat;
		Mat.Unit();
		Pos.SetMatrixRow(Mat, 3);
		_pWClient->ClientObject_Create("ClientDebris", Mat);
		int iObj = _pWClient->ClientObject_Create("ClientDebris", Mat);
		if(iObj > 0)
		{
			CWObject_Client *pObj = _pWClient->ClientObject_Get(iObj);
			if(pObj)
				pObj->m_ClientData[0] = 1;
		}
	}
}

void CWObject_ClientDebris::OnClientExecute(CWObject_ClientExecute* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_ClientDebris::OnClientExecute);
	if(_pObj->m_ClientData[0] == 1 && _pObj->m_Data[4] != 0)
		InitializeDebris(_pObj, _pWClient, 0);

	if (_pObj->GetAnimTick(_pWClient) > (4.0f*SERVER_TICKSPERSECOND + _pObj->GetAnimTickFraction()))
	{
		_pWClient->Object_Destroy(_pObj->m_iObject);
		return;
	}
}

void CWObject_ClientDebris::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	if(_pObj->m_ClientData[0] == 1)
		CWObject_Ext_Model::OnClientRender(_pObj, _pWClient, _pEngine);

/*	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetInterpolateTime() / fp32(SERVER_TIMEPERFRAME);
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

	for(int i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		int iModel = _pObj->m_iModel[i];
		CXR_Model* pModel = _pWClient->GetMapData()->GetResource_Model(iModel);
		if(iModel)
		{
			MACRO_GetRegisterObject(CXR_SurfaceContext, pSC, "SYSTEM.SURFACECONTEXT");
			if(!pSC)
				return;
			
			char Name[4 * 8 + 1];
			memcpy(Name, _pObj->m_Data, 4 * 8);
			Name[4 * 8] = 0;
			CXW_Surface *pSurf = pSC->GetSurface(Name);

			CXR_AnimState AnimState = _pObj->GetDefaultAnimState(_pWClient);
			AnimState.m_AnimAttr0 = *(fp32 *)&pSurf;
			for(int c = 0; c < 4; c++)
				AnimState.m_Colors[c] = ~_pObj->m_Data[c];
			_pEngine->Render_AddModel(pModel, MatIP, AnimState);
		}
	}*/
}

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_ClientDebris, CWObject_Ext_Model, 0x0100);

#endif
