/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_Cloth.cpp

	Author:			Anton Ragnarsson

	Copyright:		Copyright Starbreeze AB 2005

	History:		
		050426:		Created File
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_Cloth.h"


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Cloth
|__________________________________________________________________________________________________
\*************************************************************************************************/
MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Cloth, CWObject_Model, 0x0100);



const CWO_Cloth_ClientData& CWObject_Cloth::GetClientData(const CWObject_CoreData* _pObj)
{
	const CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "no client data!!");
	return *safe_cast<const CWO_Cloth_ClientData>(pData);
}

CWO_Cloth_ClientData& CWObject_Cloth::GetClientData(CWObject_CoreData* _pObj)
{
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "no client data!!");
	return *safe_cast<CWO_Cloth_ClientData>(pData);
}


void CWObject_Cloth::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Create client data - will *only* be created here and is *never* allowed to fail!
	CWO_Cloth_ClientData* pCD = MNew(CWO_Cloth_ClientData);
	if (!pCD)
		Error_static("CWObject_Cloth::OnInitClientObjects", "Unable to create client data.");
	_pObj->m_lspClientObj[0] = pCD;
}


int CWObject_Cloth::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const
{
	int Flags = CWO_CLIENTUPDATE_EXTRADATA;
	if (_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT)
		Flags |= CWO_CLIENTUPDATE_AUTOVAR;

	uint8 *pD = _pData;
	pD += parent::OnCreateClientUpdate(_iClient, _pClObjInfo, _pOld, pD, Flags);

	const CWO_Cloth_ClientData& CD = GetClientData(this);
	CD.AutoVar_Pack(_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT, pD, m_pWServer->GetMapData(), 0);

	return (pD - _pData);
}


int CWObject_Cloth::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	const uint8 *pD = _pData;
	pD += parent::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);

	if (_pObj->m_iClass == 0 || (pD - _pData) == 0)
		return (pD - _pData);

	CWO_Cloth_ClientData& CD = GetClientData(_pObj);
	if (_pObj->m_bAutoVarDirty)
		CD.AutoVar_Unpack(pD, _pWClient->GetMapData(), 0);

	return (pD - _pData);
}


void CWObject_Cloth::OnRefresh()
{
	CWO_Cloth_ClientData& CD = GetClientData(this);
//	CD.m_Temp = CD.m_Temp + 1;

	// Update Autovars
	m_DirtyMask |= CD.AutoVar_RefreshDirtyMask() << CWO_DIRTYMASK_USERSHIFT;
}


void CWObject_Cloth::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	CWO_Cloth_ClientData& CD = GetClientData(_pObj);

	// Get model
	CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if (!pModel)
		return;

	// Get model's skeleton
	CXR_Skeleton* pSkel = (CXR_Skeleton*)pModel->GetParam(MODEL_PARAM_SKELETON);
	if (!pSkel)
		return;

	// Get animstate & allocate skeleton instance
	CXR_AnimState Anim = _pObj->GetDefaultAnimState(_pWClient);
	Anim.m_pSkeletonInst = CXR_VBMHelper::Alloc_SkeletonInst(_pEngine->GetVBM(), pSkel->m_lNodes.Len(), pSkel->m_nUsedRotations, pSkel->m_nUsedMovements);
	if (!Anim.m_pSkeletonInst)
		return;

	// Interpolate position matrix
	CMat4Dfp32 MatIP;
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

	// TEMP: Create default pose
	for (uint i = 0; i < Anim.m_pSkeletonInst->m_nBoneTransform; i++)
		Anim.m_pSkeletonInst->m_pBoneTransform[i] = MatIP;

	//
	// Do the cloth stuff
	//

	// Create client data cloth if it doesn't exist
	if(pSkel->m_lCloth.Len() > 0 && pSkel->m_lCloth.Len() != CD.m_lCloth.Len())
		CD.m_lCloth.SetLen(pSkel->m_lCloth.Len());

	CXR_Model* pModels[CWO_NUMMODELINDICES];
	int nModels = 0;
	for(int k = 0; k < 3; k++)
	{
		if(_pObj->m_iModel[k] > 0)
		{
			pModels[nModels] = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
			nModels++;
		}
	}

	CXR_SkeletonInstance* pSkelInstance = Anim.m_pSkeletonInst;
	for(int i = 0; i < pSkel->m_lCloth.Len(); i++)
	{
		if(!CD.m_lCloth[i].m_IsCreated)
		{
			CD.m_lCloth[i].Create(i, pSkel, pSkelInstance, pModels, nModels);
			CD.m_lCloth[i].m_IsCreated = true;
		}
	
		CXR_SkeletonCloth& Cloth = CD.m_lCloth[i];
		CMTime Time = CMTime::CreateFromTicks(_pWClient->GetGameTick(), _pWClient->GetGameTickTime(), IPTime);
		if(Time.Compare(Cloth.m_LastUpdate) > 0)
		{
			fp32 dt = 0;
			
			if(Cloth.m_LastUpdate.IsInvalid())
				Cloth.m_LastUpdate = Time;

			dt = (Time - Cloth.m_LastUpdate).GetTime();
			Cloth.m_LastUpdate = Time;

			if(dt > 0)
			{
				if(dt > 0.02f)
					dt = 0.02f;

				CD.m_lCloth[i].Step(pModels, nModels, pSkel, pSkelInstance, dt);
/*
				int nIter = 6;
				CD.m_lCloth[i].PreIntegrate(pModels, nModels, pSkel, pSkelInstance, dt/nIter, (void*)_pWClient);
				
				for(int j = 0; j < nIter; j++)
					CD.m_lCloth[i].Integrate(pModels, nModels, pSkel, pSkelInstance, dt / nIter, j / fp32(nIter-1), NULL, (void*)_pWClient);

				CD.m_lCloth[i].PostIntegrate(pSkel, pSkelInstance, _pWClient);
				*/
			}
		}
	}

	// Add model for rendering
	_pEngine->Render_AddModel(pModel, MatIP, Anim);
}
