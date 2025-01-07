/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
File:			WObj_Burnable.cpp

Author:			Patrik Willbo

Copyright:		2006 Starbreeze Studios AB

Contents:		CWO_Burnable

Comments:		

History:
	060523:		Created File
\*____________________________________________________________________________________________*/
#include "PCH.h"
#include "WObj_Burnable.h"

#include "../Models/WModel_EffectSystem.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Burnable
|__________________________________________________________________________________________________
\*************************************************************************************************/
CWO_Burnable::CWO_Burnable()
{
	m_bDirty = false;

	for (uint i = 0; i < BURNABLE_NUM_MODELS; i++)
	{
		m_iBurnModel[i] = 0;
		m_iBurnSkelType[i] = 0;
		m_lModelInstance[i] = NULL;
	}
	m_BurnStartTick = 0;
	m_BurnEndTick = 0;
	m_BurnFlags = 0;
	m_BurnTime = 0;
}


void CWO_Burnable::Pack(uint8*& _pD) const
{
	for (uint i = 0; i < BURNABLE_NUM_MODELS; i++)
	{
		TAutoVar_Pack(m_iBurnModel[i], _pD);
		TAutoVar_Pack(m_iBurnSkelType[i], _pD);
	}
	TAutoVar_Pack(m_BurnStartTick, _pD);
	TAutoVar_Pack(m_BurnEndTick, _pD);
	TAutoVar_Pack(m_BurnFlags, _pD);
	TAutoVar_Pack(m_BurnTime, _pD);
}


void CWO_Burnable::Unpack(const uint8*& _pD)
{
	for (uint i = 0; i < BURNABLE_NUM_MODELS; i++)
	{
		TAutoVar_Unpack(m_iBurnModel[i], _pD);
		TAutoVar_Unpack(m_iBurnSkelType[i], _pD);
	}
	TAutoVar_Unpack(m_BurnStartTick, _pD);
	TAutoVar_Unpack(m_BurnEndTick, _pD);
	TAutoVar_Unpack(m_BurnFlags, _pD);
	TAutoVar_Unpack(m_BurnTime, _pD);
}


bool CWO_Burnable::OnEvalKey(CWorld_Server* _pWServer, uint32 _KeyHash, const CRegistry* _pReg)
{
	switch(_KeyHash)
	{
	case MHASH3('BURN','_MOD','EL'):	// "BURN_MODEL"
	case MHASH3('BURN','_MOD','EL0'):	// "BURN_MODEL0"
		{
			CFStr ModelName("EffectSystem:");
			ModelName += _pReg->GetThisValue();
			iBurnModel(0) = _pWServer->GetMapData()->GetResourceIndex_Model(ModelName);
		}
		return true;

	case MHASH3('BURN','_MOD','EL1'):	// "BURN_MODEL1"
		{
			CFStr ModelName("EffectSystem:");
			ModelName += _pReg->GetThisValue();
			iBurnModel(1) = _pWServer->GetMapData()->GetResourceIndex_Model(ModelName);
		}
		return true;

	case MHASH3('BURN','_MOD','EL2'):	// "BURN_MODEL2"
		{
			CFStr ModelName("EffectSystem:");
			ModelName += _pReg->GetThisValue();
			iBurnModel(2) = _pWServer->GetMapData()->GetResourceIndex_Model(ModelName);
		}
		return true;

	case MHASH3('BURN','_SKE','L'):		// "BURN_SKEL"
	case MHASH3('BURN','_SKE','L0'):	// "BURN_SKEL0"
		{
			m_iBurnSkelType[0] = _pReg->GetThisValuei();
		}
		return true;

	case MHASH3('BURN','_SKE','L1'):	// "BURN_SKEL1"
		{
			m_iBurnSkelType[1] = _pReg->GetThisValuei();
		}
		return true;

	case MHASH3('BURN','_SKE','L2'):	// "BURN_SKEL2"
		{
			m_iBurnSkelType[2] = _pReg->GetThisValuei();
		}
		return true;
	}
	return false;
}


bool CWO_Burnable::SetBurning(bool _bBurning, CWorld_Server* _pWServer)
{
	if (_bBurning && m_BurnStartTick == 0)
	{
		m_BurnFlags |= BURNABLE_FLAGS_ISBURNING;
		m_BurnStartTick = _pWServer->GetGameTick();
		m_BurnEndTick = (TruncToInt(_pWServer->GetGameTicksPerSecond() * 5.0f));
		return true;
	}
	else if (!_bBurning && m_BurnEndTick == 0)
	{
		m_BurnFlags &= ~BURNABLE_FLAGS_ISBURNING;
		m_BurnEndTick = (_pWServer->GetGameTick() - m_BurnStartTick) + TruncToInt(_pWServer->GetGameTicksPerSecond());
		return true;
	}
	return false;
}

// returns true if data was changed 
bool CWO_Burnable::SetValidModel(CWorld_Server* _pWServer)
{
	for (uint i = 0; i < BURNABLE_NUM_MODELS; i++)
	{
		if (m_iBurnModel[i] != 0) // found valid
			return false;
	}

	// No models found, add a default model
	m_iBurnModel[0] = _pWServer->GetMapData()->GetResourceIndex_Model("EffectSystem:FXEffect,fx_burningchar");
	return true;
}


void CWO_Burnable::OnIncludeClass(CMapData* _pWData, CWorld_Server *_pWServer)
{
	_pWData->GetResourceIndex_Model("EffectSystem:FXEffect,fx_burningchar");
	_pWData->GetResourceIndex_Surface("burnscorch");
}


void CWO_Burnable::OnIncludeTemplate(CRegistry *_pReg, CMapData *_pMapData, CWorld_Server* )
{
	IncludeModelFromKey("BURN_MODEL", _pReg, _pMapData);
	IncludeModelFromKey("BURN_MODEL0", _pReg, _pMapData);
	IncludeModelFromKey("BURN_MODEL1", _pReg, _pMapData);
	IncludeModelFromKey("BURN_MODEL2", _pReg, _pMapData);
}


void CWO_Burnable::CopyBurnable(const CWO_Burnable& _Burnable)
{
	for (uint i = 0; i < BURNABLE_NUM_MODELS; i++)
	{
		m_iBurnModel[i] = _Burnable.m_iBurnModel[i];
		m_iBurnSkelType[i] = _Burnable.m_iBurnSkelType[i];
	}
	m_BurnStartTick = _Burnable.m_BurnStartTick;
	m_BurnEndTick = _Burnable.m_BurnEndTick;
	m_BurnFlags = _Burnable.m_BurnFlags;
	m_BurnTime = _Burnable.m_BurnTime;
}


bool CWO_Burnable::OnRefresh(CWObject_CoreData* _pObj, CWorld_Server* _pWServer)
{
	bool bDirty = m_bDirty;
	m_bDirty = false;

	if (!(m_BurnFlags & BURNABLE_FLAGS_ISBURNING))
		return bDirty;

	// Should we stop burning?
	if (m_BurnEndTick > 0 && m_BurnEndTick < (_pWServer->GetGameTick() - m_BurnStartTick))
	{
		BurnTime() = m_BurnTime + m_BurnEndTick;
		BurnEndTick() = 0;
		BurnStartTick() = 0;
		BurnFlags() = m_BurnFlags & ~(BURNABLE_FLAGS_ISBURNING);
	}
	else
	{
		// In water, make object stop burning
		int Medium = _pWServer->Phys_GetMedium(_pObj->GetPosition());
		if(Medium == XW_MEDIUM_WATER)
		{
			int32 TicksPerSecond = (int32)TruncToInt(_pWServer->GetGameTicksPerSecond() + 20.0f);
			if(m_BurnEndTick > _pWServer->GetGameTick() + TicksPerSecond)
				BurnEndTick() = (_pWServer->GetGameTick() - m_BurnStartTick) + TicksPerSecond;
		}
		else
		{
			// Take damage,
			if(_pWServer->GetGameTick() % TruncToInt(_pWServer->GetGameTicksPerSecond()) == 0)
			{
				CWO_DamageMsg DmgMsg(2, DAMAGETYPE_FIRE);
				DmgMsg.Send(_pObj->m_iObject, _pObj->m_iObject, _pWServer);
			}
		}
	}
	return bDirty;
}


void CWO_Burnable::IncludeModelFromKey(const char* _pKey, CRegistry* _pReg, CMapData* _pMapData)
{
	if(_pReg)
	{
		CRegistry* pChild = _pReg->FindChild(_pKey);
		if(pChild)
		{
			CStr ModelName("EffectSystem:");
			ModelName += pChild->GetThisValue();
			_pMapData->GetResourceIndex_Model(ModelName);
		}
	}
}


void CWO_Burnable::OnClientUpdate(CWObject_CoreData* _pObj, CWorld_Client* _pWClient)
{
	UpdateModelInstance(0, _pObj, _pWClient);
	UpdateModelInstance(1, _pObj, _pWClient);
	UpdateModelInstance(2, _pObj, _pWClient);
}


void CWO_Burnable::UpdateModelInstance(uint32 _iModelInstance, CWObject_CoreData* _pObj, CWorld_Client* _pWClient)
{
	// Update model instances
	CXR_Model* pModel = (CXR_Model*)_pWClient->GetMapData()->GetResource_Model(m_iBurnModel[_iModelInstance]);
	if (pModel && !m_lModelInstance[_iModelInstance])
	{
		m_lModelInstance[_iModelInstance] = pModel->CreateModelInstance();
		if (m_lModelInstance[_iModelInstance])
			m_lModelInstance[_iModelInstance]->Create(pModel, CXR_ModelInstanceContext(_pWClient->GetGameTick(), _pWClient->GetGameTickTime(), _pObj, _pWClient));
	}
	else if (!pModel)
		m_lModelInstance[_iModelInstance] = NULL;
}


void CWO_Burnable::OnClientRefresh(CWObject_CoreData* _pObj, CWorld_Client* _pWClient)
{
	// Refresh model instances
	for(uint i = 0; i < BURNABLE_NUM_MODELS; i++)
	{
		if(m_lModelInstance[i])
		{
			CXR_Model* pModel = (m_iBurnModel[i]) ? _pWClient->GetMapData()->GetResource_Model(m_iBurnModel[i]) : NULL;
			CXR_ModelInstanceContext ModelInstContext(_pWClient->GetGameTick(), _pWClient->GetGameTickTime(), _pObj, _pWClient);
			if(m_lModelInstance[i]->NeedRefresh(pModel, ModelInstContext))
				m_lModelInstance[i]->OnRefresh(ModelInstContext);
		}
	}
}


void CWO_Burnable::OnClientRender(CWObject_CoreData* _pObj, CWorld_Client* _pWClient, CMat4Dfp32& _WMat, CXR_Engine* _pEngine, CXR_Skeleton* _pSkeleton, CXR_SkeletonInstance* _pSkeletonInstance, const CBox3Dfp32* _pBBox, const uint32* _pnBBox) const
{
	if(!(m_BurnFlags & BURNABLE_FLAGS_ISBURNING))
		return;

	CMat4Dfp32 IPMat;
	CXR_AnimState AnimState;

	// Set animstate
	AnimState.Clear();
	AnimState.m_AnimTime0 = CMTime::CreateFromTicks(_pWClient->GetGameTick() - m_BurnStartTick, _pWClient->GetGameTickTime(), _pWClient->GetRenderTickFrac());
	if(m_BurnEndTick != 0)
		AnimState.m_AnimTime1 = CMTime::CreateFromTicks(m_BurnEndTick, _pWClient->GetGameTickTime(), 1.0f);
	else
		AnimState.m_AnimTime1.Reset();

	// Walk through all models and add to engine correctly
	for(uint i = 0; i < BURNABLE_NUM_MODELS; i++)
	{
		// Make sure we have a valid model
		CXR_Model* pModel = (CXR_Model*)_pWClient->GetMapData()->GetResource_Model(m_iBurnModel[i]);
		if(!pModel)
			continue;

		// Setup skeleton
		if(m_iBurnSkelType[i] > 0 && _pSkeleton && _pSkeletonInstance)
		{
			AnimState.m_pSkeletonInst = _pSkeletonInstance;
			AnimState.m_Data[FXANIM_DATA_FLAGS] = FXANIM_FLAGS_USESKELETON;
			AnimState.m_Data[FXANIM_DATA_SKELETONTYPE] = m_iBurnSkelType[i];
			AnimState.m_Data[FXANIM_DATA_SKELETON] = (aint)_pSkeleton;
		}
		else if (_pBBox && _pnBBox && *_pnBBox > 0)
		{
			// Try to setup some bounding boxes
			AnimState.m_Data[FXANIM_DATA_FLAGS] = FXANIM_FLAGS_USEBOXES;
			AnimState.m_Data[FXANIM_DATA_NUMBOXES] = (aint)*_pnBBox;
			AnimState.m_Data[FXANIM_DATA_BOXES] = (aint)_pBBox;
		}
		else
		{
			// Set neither boxes or skeleton
			AnimState.m_pSkeletonInst = NULL;
			AnimState.m_Data[FXANIM_DATA_FLAGS] = 0;
			AnimState.m_Data[FXANIM_DATA_DATA1] = 0;
			AnimState.m_Data[FXANIM_DATA_DATA2] = 0;
		}

		// Set model instance and add model for rendering
		AnimState.m_pModelInstance = (CXR_ModelInstance*)(const CXR_ModelInstance*)m_lModelInstance[i];
		_pWClient->Render_GetEngine()->Render_AddModel(pModel, _WMat, AnimState, XR_MODEL_STANDARD);
	}
}

