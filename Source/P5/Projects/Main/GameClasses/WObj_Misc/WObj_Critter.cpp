#include "PCH.h"
#include "WObj_Critter.h"
#include "WObj_ScenePoint.h"
#include "../../GameWorld/WServerMod.h"

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CCritterAnimGraph
|__________________________________________________________________________________________________
\*************************************************************************************************/
CCritterAnimGraph::CCritterAnimGraph()
{
	m_spAGI = MNew(CWAG2I);
	m_spMirror = MNew1(CWAG2I_Mirror, 1);
#ifndef M_RTM
	m_spAGI->m_bDisableDebug = false;
#endif
	AG2_RegisterCallbacks(NULL);
	AG2_RegisterCallbacks2(CWO_Clientdata_Character_AnimGraph2::ms_lpfnConditions_Server, MAX_ANIMGRAPH2_CONDITIONS,
		CWO_Clientdata_Character_AnimGraph2::ms_lpfnProperties_Server, MAX_ANIMGRAPH2_PROPERTIES,
		CWO_Clientdata_Character_AnimGraph2::ms_lpfnOperators_Server, MAX_ANIMGRAPH2_OPERATORS,
		CWO_Clientdata_Character_AnimGraph2::ms_lpfnEffects_Server, MAX_ANIMGRAPH2_EFFECTS);
}


void CCritterAnimGraph::SetInitialProperties(const CWAG2I_Context* _pContext)
{
	SetNumProperties(AG2_MAXPROPERTYFLOAT_DARKNESS, AG2_MAXPROPERTYINT_DARKNESS,AG2_MAXPROPERTYBOOL_DARKNESS);
	SetPropertyBool(PROPERTY_BOOL_ALWAYSTRUE, true);
	SetPropertyBool(PROPERTY_BOOL_ISSERVER, _pContext->m_pWPhysState->IsServer());
}


void CCritterAnimGraph::AG2_OnEnterState(const CWAG2I_Context* _pContext, CAG2TokenID _TokenID, CAG2StateIndex _iState, CAG2AnimGraphID _iAnimGraph, CAG2ActionIndex _iEnterAction)
{
	CWAG2I* pAG2I = GetAG2I();

	if (!pAG2I || !pAG2I->AcquireAllResources(_pContext))
		return;

	const CXRAG2* pAG = pAG2I->GetAnimGraph(_iAnimGraph);
	M_ASSERT(pAG,"Invalid animgraph");
	const CXRAG2_State* pState = pAG->GetState(_iState);
	M_ASSERT(pState,"Invalid State");
	m_FlagsLo = pState->GetFlags(0);
}

void CCritterAnimGraph::UpdateImpulseState(const CWAG2I_Context* _pContext)
{

}

void CCritterAnimGraph::Copy(const CCritterAnimGraph& _CD)
{
	CWO_ClientData_AnimGraph2Interface::Copy(_CD);
	m_FlagsLo = _CD.m_FlagsLo;
}

void CCritterAnimGraph::Clear(void)
{
	//SetNumProperties(AG2_MAXPROPERTYFLOAT_DARKNESS, AG2_MAXPROPERTYINT_DARKNESS,AG2_MAXPROPERTYBOOL_DARKNESS);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWO_Critter_ClientData
|__________________________________________________________________________________________________
\*************************************************************************************************/

CWO_Critter_ClientData::CWO_Critter_ClientData()
: m_pObj(NULL)
, m_pWPhysState(NULL)
{
}


void CWO_Critter_ClientData::Clear(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	m_pObj = _pObj;
	m_pWPhysState = _pWPhysState;

	m_AnimGraph.Clear();
	m_AnimGraph.GetAG2I()->SetEvaluator(&m_AnimGraph);
	m_AnimGraph.SetAG2I(m_AnimGraph.GetAG2I());

	CWAG2I_Context AG2Context(_pObj, _pWPhysState, CMTime());
	m_AnimGraph.SetInitialProperties(&AG2Context);

	m_LastSkelInstanceFrac = 0.0f;
	m_LastSkelInstanceTick = 0;

	m_AIState = 0;

	CWAG2I* pAGI = m_AnimGraph.GetAG2I();
	CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_WEAPONTYPE, AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED);
	CWAG2I_Context AGIContext(m_pObj, _pWPhysState, _pWPhysState->GetGameTime());
	pAGI->SendImpulse(&AGIContext, Impulse, 0);
}

void CWO_Critter_ClientData::OnRefresh()
{
	const CVec3Dfp32& ObjPos = m_pObj->GetPosition();
	CVec3Dfp32 Min, Max;
	Min = ObjPos;
	Max = ObjPos;
	Min.k[0] -= 4.0f;
	Min.k[1] -= 4.0f;
	Min.k[2] -= 4.0f;
	Max.k[0] += 4.0f;
	Max.k[1] += 4.0f;
	Max.k[2] += 4.0f;
	CBox3Dfp32 BBox(Min, Max);

	// Refresh and update impulse states on animgraphs
	CWAG2I_Context AGIContext(m_pObj, m_pWPhysState, m_pWPhysState->GetGameTime());
	m_AnimGraph.m_spAGI->Refresh(&AGIContext);
	m_AnimGraph.UpdateImpulseState(&AGIContext);

	// Set new bound box
	CMat4Dfp32 InvObjMat;
	m_pObj->GetPositionMatrix().InverseOrthogonal(InvObjMat);
	BBox.Transform(InvObjMat, BBox);
	m_pWPhysState->Object_SetVisBox(m_pObj->m_iObject, BBox.m_Min, BBox.m_Max);
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Critter
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Critter, CWObject_Model, 0x0100);

CWObject_Critter::CWObject_Critter()
{
	m_bWaitOnImpulse	= false;

	m_SpeedForward		= 4.0f;

	m_EffectDie			= "hiteffect_crow";
}

const CWO_Critter_ClientData& CWObject_Critter::GetClientData(const CWObject_CoreData* _pObj)
{
	const CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who deleted my client data?!");
	return *safe_cast<const CWO_Critter_ClientData>(pData);
}


CWO_Critter_ClientData& CWObject_Critter::GetClientData(CWObject_CoreData* _pObj)
{
	CReferenceCount* pData = _pObj->m_lspClientObj[0];
	M_ASSERT(pData, "Who deleted my client data?!");
	return *safe_cast<CWO_Critter_ClientData>(pData);
}

void CWObject_Critter::OnEvalKey(uint32 _KeyHash, const CRegistry* _pKey)
{
	CWO_Critter_ClientData& CD = GetClientData(this);

	CStr KeyValue = _pKey->GetThisValue();
	fp32 KeyValuef = (fp32)KeyValue.Val_fp64();
	int KeyValuei = KeyValue.Val_int();

	switch(_KeyHash) 
	{
	case MHASH4('spee','d_fo','rwar','d'):	//speed_forward
		{
			m_SpeedForward = KeyValuef;
		}
		break;
	case MHASH3('ANIM','GRAP','H2'): // "ANIMGRAPH"
		{
			//CWAG2I_Context Context(this, m_pWServer, CMTime());
			for (int32 i = 0; i < _pKey->GetNumChildren(); i++)
			{
				CStr Val = _pKey->GetChild(i)->GetThisValue();
				int32 iAG2 = m_pWServer->GetMapData()->GetResourceIndex_AnimGraph2(Val);
				CD.m_AnimGraph.GetAG2I()->AddResourceIndex_AnimGraph2(iAG2, Val);
			}
			break;
		}
	case MHASH3('EFFE','CT_D','IE'): // "EFFECT_DIE"
		{
			 m_EffectDie = KeyValue;
			 m_pWServer->GetMapData()->GetResourceIndex_Class(m_EffectDie);
		}
		break;
	case MHASH3('ESCA','PE_R','ANGE'):
		{
			m_EscapeRange = KeyValuef;
		}
		break;
	case MHASH2('SOUN','D'):	//sound
		{
			m_iSound = m_pWServer->GetMapData()->GetResourceIndex_Sound(KeyValue);
		}
		break;
	case MHASH4('SOUN','D_MI','N_TI','ME'):	//sound_min_time
		{
			m_SoundMinTime = KeyValuei;
		}
		break;
	case MHASH4('SOUN','D_RA','ND_T','IME'):	//sound_rand_time
		{
			m_SoundRandTime = KeyValuei;
		}
		break;
	case MHASH4('BORE','DOM_','MIN_','TIME'):	//boredom_min_time
		{
			m_BoredomMinTime = KeyValuei;
		}
		break;
	case MHASH5('BORE','DOM_','RAND','_TIM','E'):	//boredom_rand_time
		{
			m_BoredomRandTime = KeyValuei;
		}
		break;
	case MHASH3('STAR','T_ST','ATE'):
		{
			if(KeyValuei)
				m_bWaitOnImpulse = true;
		}
		break;
	}
	CWObject_Model::OnEvalKey(_KeyHash, _pKey);
}

void CWObject_Critter::OnCreate()
{
	CWO_PhysicsState Phys = GetPhysState();

	Phys.m_Prim[0].Create(OBJECT_PRIMTYPE_BOX, -1, CVec3Dfp32(5.0f, 5.0f, 5.0f), CVec3Dfp32(0.0f, 0.0f, 5.0f));
	Phys.m_nPrim = 1;
	Phys.m_ObjectFlags = OBJECT_FLAGS_TRIGGER;
	Phys.m_ObjectIntersectFlags = OBJECT_FLAGS_PROJECTILE;
	Phys.m_PhysFlags |= OBJECT_PHYSFLAGS_OFFSET;
	m_pWServer->Object_SetPhysics(m_iObject, Phys);

	m_pBlockNavSearcher = m_pWServer->Path_GetBlockNavSearcher();
}


CWObject_ScenePointManager* CWObject_Critter::GetSPM()
{
	return (CWObject_ScenePointManager*)m_pWServer->Message_SendToObject(
		CWObject_Message(OBJMSG_GAME_GETSCENEPOINTMANAGER), m_pWServer->Game_GetObjectIndex());
}


void CWObject_Critter::OnClientRefresh(CWObject_Client* _pObj, CWorld_Client* _pWClient)
{
	MSCOPESHORT(CWObject_Critter::OnClientRefresh);

	CWObject_Model::OnClientRefresh(_pObj,_pWClient);
	CWO_Critter_ClientData& CD = GetClientData(_pObj);
	CD.OnRefresh();
}

void CWObject_Critter::OnClientRender(CWObject_Client* _pObj, CWorld_Client* _pWClient, CXR_Engine* _pEngine, const CMat4Dfp32& _ParentMat)
{
	// .. render stuff..
	CWO_Critter_ClientData& CD = GetClientData(_pObj);

	// Get animstate..
	fp32 IPTime = _pWClient->GetRenderTickFrac();
	CMat4Dfp32 MatIP, ParentMat;
	Interpolate2(_pObj->GetLastPositionMatrix(), _pObj->GetPositionMatrix(), MatIP, IPTime);

	CXR_Model *pMainModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[0]);
	if (!pMainModel)
		return;

	CXR_AnimState AnimState;
	AnimState.m_iObject = _pObj->m_iObject;
	CXR_Skeleton* pSkel = pMainModel->GetSkeleton();
	if (pSkel)
	{
		AnimState.m_pSkeletonInst = CXR_VBMHelper::Alloc_SkeletonInst(_pEngine->GetVBM(), pSkel->m_lNodes.Len(), pSkel->m_nUsedRotations, pSkel->m_nUsedMovements);

		OnGetAnimState(_pObj, _pWClient, pSkel, 0, MatIP, IPTime, AnimState);
	}

	for (int32 i = 0; i < CWO_NUMMODELINDICES; i++)
	{
		CXR_Model *pModel = _pWClient->GetMapData()->GetResource_Model(_pObj->m_iModel[i]);
		if (pModel)
			_pEngine->Render_AddModel(pModel, MatIP, AnimState);
	}
}


int CWObject_Critter::OnClientUpdate(CWObject_Client* _pObj, CWorld_Client* _pWClient, const uint8* _pData, int& _Flags)
{
	const uint8 *pD = _pData;
	pD += CWObject_Model::OnClientUpdate(_pObj, _pWClient, _pData, _Flags);
	if (_pObj->m_iClass == 0 || (pD - _pData) == 0)
		return 0;

	CWO_Critter_ClientData& CD = GetClientData(_pObj);

	if (_pObj->m_bAutoVarDirty)
	{ // Handle AutoVars
		CD.AutoVar_Unpack(pD, _pWClient->GetMapData(), 0);
	}

	{ // Handle AnimGraph
		CWAG2I_Context AG2IContext(_pObj, _pWClient, _pWClient->GetGameTime());
		CWAG2I* pMirrorAGI = CD.m_AnimGraph.m_spMirror->GetWAG2I(0);
		CWAG2I* pAGI = ((_pWClient->GetClientMode() & WCLIENT_MODE_MIRROR) == 0) ? CD.m_AnimGraph.GetAG2I() : NULL;
		pD += CWAG2I::OnClientUpdate2(&AG2IContext, pMirrorAGI, pAGI, pD);
	}

	//	int nSize = (pD - _pData);
	//	M_TRACE("OnClientUpdate, %d, %d, %d\n", _pObj->m_iObject, _pObj->m_iClass, nSize);
	return (pD - _pData);
}

void CWObject_Critter::OnGetAnimState(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState, CXR_Skeleton* _pSkel, int _iCacheSlot, const CMat4Dfp32 &_Pos, fp32 _IPTime, CXR_AnimState& _Anim, CVec3Dfp32 *_pRetPos, bool _bIgnoreCache)
{
	CWO_Critter_ClientData& CD = GetClientData(_pObj);
	CXR_SkeletonInstance* pSkelInstance = CD.m_spSkelInstance;

	if(_Anim.m_pSkeletonInst)
	{
		pSkelInstance = _Anim.m_pSkeletonInst;
		_bIgnoreCache = true;
	}
	else if (_pSkel && !pSkelInstance && (_iCacheSlot == 1))
	{
		MRTC_SAFECREATEOBJECT(spSkel, "CXR_SkeletonInstance", CXR_SkeletonInstance);
		CD.m_spSkelInstance = spSkel;
		spSkel->Create(_pSkel->m_lNodes.Len());
		pSkelInstance = spSkel;
		_bIgnoreCache = true;
	}

	if (_bIgnoreCache || CD.m_LastSkelInstanceTick != _pWPhysState->GetGameTick() || CD.m_LastSkelInstanceFrac != _IPTime)
	{
		// Eval skel
		CXR_AnimLayer Layers[16];
		int nLayers = 16;
		CWAG2I_Context AGIContext(_pObj, _pWPhysState, CMTime::CreateFromTicks(_pWPhysState->GetGameTick(),_pWPhysState->GetGameTickTime(),_IPTime));
		CD.m_AnimGraph.GetAG2I()->GetAnimLayers(&AGIContext, Layers, nLayers, 0);
		CMat4Dfp32 Pos = _Pos;
		if (nLayers > 0)
			_pSkel->EvalAnim(Layers, nLayers, pSkelInstance, Pos);
		else if (_iCacheSlot == 1)
		{
			// Clear skeleton
			for (int32 i = 0; i < pSkelInstance->m_nBoneLocalPos; i++)
				pSkelInstance->m_pBoneLocalPos[i].Unit();
			for (int32 i = 0; i < pSkelInstance->m_nBoneTransform; i++)
				pSkelInstance->m_pBoneTransform[i].Unit();
		}

		if (!_bIgnoreCache)
		{
			CD.m_LastSkelInstanceTick = _pWPhysState->GetGameTick();
			CD.m_LastSkelInstanceFrac = _IPTime;
		}
	}

	_Anim.m_pSkeletonInst = pSkelInstance;
	_Anim.m_SurfaceOcclusionMask = 0;
	_Anim.m_SurfaceShadowOcclusionMask = 0;
	_Anim.m_AnimTime0 = _pWPhysState->GetGameTime();
}

void CWObject_Critter::OnInitClientObjects(CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState)
{
	// Check if we need to (re)allocate client data
	CReferenceCount* pData = _pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA];
	CWO_Critter_ClientData* pCD = TDynamicCast<CWO_Critter_ClientData>(pData);

	// Allocate clientdata
	if (!pCD || pCD->m_pObj != _pObj || pCD->m_pWPhysState != _pWPhysState)
	{
		pCD = MNew(CWO_Critter_ClientData);
		if (!pCD)
			Error_static("CWObject_Critter", "Could not allocate client data!");

		_pObj->m_lspClientObj[PLAYER_CLIENTOBJ_CLIENTDATA] = pCD;
		pCD->Clear(_pObj, _pWPhysState);
	}
}


void CWObject_Critter::OnIncludeTemplate(CRegistry* _pReg, CMapData* _pMapData, CWorld_Server* _pWServer)
{
	CRegistry *pAnimGraph2Reg = _pReg->FindChild("ANIMGRAPH2");
	if(pAnimGraph2Reg)
	{
		MSCOPE(GetResourceIndex_AnimGraph, CHARACTER);
		for (int32 i = 0; i < pAnimGraph2Reg->GetNumChildren(); i++)
			_pMapData->GetResourceIndex_AnimGraph2(pAnimGraph2Reg->GetChild(i)->GetThisValue());
	}

	CWObject_Model::OnIncludeTemplate(_pReg, _pMapData, _pWServer);
}

aint CWObject_Critter::OnMessage(const CWObject_Message& _Msg)
{
	return CWObject_Model::OnMessage(_Msg);
}

int CWObject_Critter::OnCreateClientUpdate(int _iClient, const CWS_ClientObjInfo* _pClObjInfo, const CWObject_CoreData* _pOld, uint8* _pData, int _Flags) const
{
	uint8* pD = _pData;

	int Flags = CWO_CLIENTUPDATE_EXTRADATA;
	if ((_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT) & ~CRITTER_AG2_DIRTYMASK)
		Flags |= CWO_CLIENTUPDATE_AUTOVAR;

	pD += CWObject_Model::OnCreateClientUpdate(_iClient, _pClObjInfo, _pOld, _pData, Flags);
	if ((pD - _pData) == 0)
		return 0;

	const CWO_Critter_ClientData& CD = GetClientData(this);

	{ // Handle AutoVars, remove flag used for ag2)
		CD.AutoVar_Pack((_pClObjInfo->m_DirtyMask >> CWO_DIRTYMASK_USERSHIFT) & ~CRITTER_AG2_DIRTYMASK, pD, m_pWServer->GetMapData(), 0);
	}

	{ // Handle AnimGraph
		static const CWAG2I DummyAGI;
		const CWAG2I* pMirrorAGI = &DummyAGI;
		if (_pOld->m_lspClientObj[0])
			pMirrorAGI = GetClientData(_pOld).m_AnimGraph.m_spMirror->GetWAG2I(0);
		int8 AGPackType = (m_pWServer->GetPlayMode() == SERVER_PLAYMODE_DEMO) ? WAG2I_CLIENTUPDATE_NORMAL : WAG2I_CLIENTUPDATE_DIRECTACCESS;
		pD += CD.m_AnimGraph.GetAG2I()->OnCreateClientUpdate2(pD, pMirrorAGI, AGPackType);
	}

	//	int nSize = (pD - _pData);
	//	M_TRACE("OnCreateClientUpdate, %d, %d, %d\n", m_iObject, m_iClass, nSize);
	return (pD - _pData);
}

fp32 CWObject_Critter::CheckForScaryStuff(CVec3Dfp32 pos, CVec3Dfp32 *escape_vec)
{
	const int16* pSel = NULL;
	TSelection<CSelection::LARGE_BUFFER> Selection;
	m_pWServer->Selection_AddBoundSphere(Selection, OBJECT_FLAGS_CHARACTER, pos, m_EscapeRange);
	m_pWServer->Debug_RenderSphere(GetPositionMatrix(), m_EscapeRange);
	uint nSel = m_pWServer->Selection_Get(Selection, &pSel);
	if(nSel > 0)
	{
		for(uint i = 0; i < nSel; i++)
		{
			int iObject = pSel[i];
			if (iObject == m_iObject)
				continue;

			CWObject_CoreData* pObjectCD = m_pWServer->Object_GetCD(pSel[i]);
			CVec3Dfp32 pos = pObjectCD->GetPosition();
			CVec3Dfp32 avoidvector = (GetPosition() - pos);

			fp32 dist = avoidvector.Length();

			if(escape_vec)
				*escape_vec = avoidvector.Normalize();
			
			return dist;
		}
	}
	return 0.0f;
}

bool CWObject_Critter::GetNextTarget(void)
{

	return false;
}

void CWObject_Critter::UpdateSP(void)
{
	if(m_pTargetSP)
	{
		if (!m_pTargetSP->Request(m_iObject, m_pWServer, GetSPM()))
		{	//We lost the scenepoint, get a new target
			GetNextTarget();
		}
	}
}


/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Crow
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Crow, CWObject_Critter, 0x0100);

CWObject_Crow::CWObject_Crow()
{
	m_pTargetSP				= NULL;
	m_pLastTargetSP			= NULL;
	m_SpeedForward			= 4.0f;
	m_EscapeRange			= 150.0f;
	m_StartFlyingGameTick	= 0;
	m_LastTraceTick			= 0;
	m_LastTraceTick2		= 1;
	m_FlyTick				= 0;
	m_bForceCircling		= false;
	m_bAvoidWall			= false;
	m_bWallAhead			= false;
	m_bLand					= false;
	m_bPanic				= false;
	m_bNoTraces				= false;
	m_bWaitOnImpulse		= false;
	m_iSound				= -1;
	m_SoundMinTime			= 10;
	m_SoundRandTime			= 10;
	m_SoundNextTick			= 0;
	m_BoredomMinTime		= 20;
	m_BoredomRandTime		= 20;
	m_BoredomNextTick		= 0;
	m_EscapeHeight			= 0.0f;
	m_BobbingLastZMod		= 0.0f;
	m_BobbingLastTick		= 0;
	m_NameHash				= "Crow";
	m_WalkTick				= 0;
	m_WalkIdleTick			= 0;
	m_IdleTick				= 0;
	m_MaxTurnSpeed			= 0.0f;
	m_FlyDir				= FLY_FORWARD;
}

CWObject_Crow::~CWObject_Crow()
{

}

void CWObject_Crow::OnFinishEvalKeys(void)
{
	parent::OnFinishEvalKeys();

	CWO_Critter_ClientData& CD = GetClientData(this);

	int32 iAG2 = m_pWServer->GetMapData()->GetResourceIndex_AnimGraph2("AnimGraphs\\AG2Crow");
	CD.m_AnimGraph.GetAG2I()->AddResourceIndex_AnimGraph2(iAG2, "AnimGraphs\\AG2Crow");
	
	// Make sure all animations are precached
	CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
	CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();

	TArray<CXRAG2_Impulse> lImpulses;
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_TAKEOFF));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_LAND));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_SOAR));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_FLY));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR, CROW_AG2_GESTURE_SHRUG));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR, CROW_AG2_GESTURE_LOOK));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR, CROW_AG2_GESTURE_PICKFEATHERS));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE, AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED));
	pAGI->TagAnimSetFromImpulses(&AGIContext, m_pWServer->GetMapData(), m_pWServer->m_spWData, lImpulses);

	CWServer_Mod* pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
	uint nAnimGraphs = pAGI->GetNumResource_AnimGraph2();
	for (uint i = 0; i < nAnimGraphs; i++)
		pServerMod->RegisterAnimGraph2(pAGI->GetResourceIndex_AnimGraph2(i));

	m_LocalDebug = GetLocalPositionMatrix();
	m_PosDebug = GetPositionMatrix();
}

void CWObject_Crow::OnCreate()
{
	parent::OnCreate();
	m_LastTraceTick = m_pWServer->GetGameTick();
	m_LastTraceTick2 = m_LastTraceTick + 1;	
}

void CWObject_Crow::OnSpawnWorld(void)
{
	parent::OnSpawnWorld();

	CWO_Critter_ClientData& CD = GetClientData(this);
	if(!m_bWaitOnImpulse)
	{
		CD.OnRefresh();
		CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();
		CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_FLY);
		CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
		pAGI->SendImpulse(&AGIContext, Impulse, 0);
	}
	else
		CD.m_AIState = CROW_STATE_LANDED;
}

void CWObject_Crow::OnRefresh(void)
{
	parent::OnRefresh();

	CWO_Critter_ClientData& CD = GetClientData(this);
	CD.OnRefresh();

	m_pWServer->Debug_RenderMatrix(GetPositionMatrix(), 1.0f, true);

	//Sound
	if(m_SoundNextTick < m_pWServer->GetGameTick())
	{
		m_pWServer->Sound_At(GetPosition(), m_iSound, 0);
		m_SoundNextTick = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (m_SoundMinTime + ((MRTC_RAND() % m_SoundRandTime) + 1)));
	}

	UpdateSP();

	if(m_pTargetSP || m_bWaitOnImpulse)
	{	//Update position and do AI stuff
		switch(CD.m_AIState) 
		{
		case CROW_STATE_FLYING:
			{
				FlyNormal();
		    }
			break;
		case CROW_STATE_LANDING:
			{
				FlyLanding();
			}
			break;
		case CROW_STATE_LANDED:
		    {
				IdleLanded();
		    }
			break;
		case CROW_STATE_DEPARTING:
			{
				FlyDeparting();
			}
			break;
		case CROW_STATE_ESCAPING:
			{
				FlyEscape();
			}
			break;
		case CROW_STATE_WALK:
			{
				Walk();
			}
			break;
		}
	}
	else
	{	//Get next roam target, if no scenepoint found roam around randomly
		if(!GetNextTarget())
		{
			FlyNormalNoSP();
		}
	}
}

fp32 CWObject_Crow::UpdateBobbing(void)
{
	m_BobbingLastTick = m_pWServer->GetGameTick();
	CWO_Critter_ClientData& CD = GetClientData(this);
	if(CD.m_AnimGraph.m_FlagsLo & CROW_FLAGS_FLYING)
	{
		m_BobbingLastZMod = M_Sin(((m_BobbingLastTick % 30) / 30.0f) * 4.0f * _PI);
	}
	else
	{
		if(m_BobbingLastZMod >= 0.1f)
			m_BobbingLastZMod -= 0.1f;
		else if(m_BobbingLastZMod <= -0.1f)
			m_BobbingLastZMod += 0.1f;
		else
			m_BobbingLastZMod = 0.0f;
	}

	return m_BobbingLastZMod;
}

aint CWObject_Crow::OnMessage(const CWObject_Message &_Msg)
{
	switch(_Msg.m_Msg) 
	{
	case OBJMSG_IMPULSE:
	    {
			CWO_Critter_ClientData& CD = GetClientData(this);
			if(CD.m_AIState == CROW_STATE_LANDED || CD.m_AIState == CROW_STATE_WALK)
			{
				m_EscapeDir = GetPositionMatrix().GetRow(0);
				if(m_EscapeDir.k[2] < 0.5f)
				{
					m_EscapeDir.k[2] = 0.5f;
					//'fake' normalize
					fp32 temp = M_Sqrt((1.0f - m_EscapeDir.k[2] * m_EscapeDir.k[2]) / (m_EscapeDir.k[0] * m_EscapeDir.k[0] + m_EscapeDir.k[1] * m_EscapeDir.k[1]));
					m_EscapeDir.k[0] *= temp;
					m_EscapeDir.k[1] *= temp;
				}

				CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();
				CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_TAKEOFF);
				CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
				pAGI->SendImpulse(&AGIContext, Impulse, 0);

				CD.m_AIState = CROW_STATE_ESCAPING;

				m_pWServer->Sound_At(GetPosition(), m_iSound, 0);
				m_SoundNextTick = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (m_SoundMinTime * MRTC_RAND() % m_SoundRandTime));

				m_bNoTraces = false;

				if(m_pTargetSP)
				{
					m_pTargetSP->Release(m_iObject, m_pWServer, GetSPM());
				}

				m_bWaitOnImpulse = false;

				return 1;
			}
	    }
		break;
	case OBJMSG_DAMAGE:
	    {
			CWObject_Message Msg;
			Msg.m_Msg = OBJMSG_IMPULSE;
			m_pWServer->Message_SendToTarget(Msg, "Crow");

			m_pWServer->Object_Create(m_EffectDie, GetPosition());
			Destroy();
	    }
		break;
	}

	return parent::OnMessage(_Msg);
}

int CWObject_Crow::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	switch(_Event)
	{
	case CWO_PHYSEVENT_GETACCELERATION:
		{
			CWO_Critter_ClientData& CD = GetClientData(_pObj);
			if(CD.m_AIState == CROW_STATE_WALK)
			{
				CWO_PhysicsState PhysState(_pObj->GetPhysState());

				CCollisionInfo CInfo_1;
				CMat4Dfp32 Dest; 
				Dest = _pObj->GetPositionMatrix();
				Dest.k[3][2] -= 5.0f;

				PhysState.m_ObjectIntersectFlags |= OBJECT_FLAGS_WORLD;

				bool bOnGround = _pPhysState->Phys_IntersectWorld((CSelection *) NULL, PhysState, _pObj->GetPositionMatrix(), Dest, _pObj->m_iObject, &CInfo_1);

				if(bOnGround)
				{
					_pMat->Unit();
					if(CInfo_1.m_Distance)
					{
						_pMat->k[3][2] = -(5.0f + CInfo_1.m_Distance);
						if(_pMat->k[3][2] > -0.01f && _pMat->k[3][2] < 0.01f)
							_pMat->k[3][2] = 0.0f;
					}
					if(_pMat->k[3][2] > 0.05f)
						_pMat->k[3][2] = 0.05f;

					return SERVER_PHYS_HANDLED;
				}
				else
				{
					_pMat->Unit();
					_pMat->k[3][2] = -5.0f;
					return SERVER_PHYS_HANDLED;
				}
			}
			else
			{
				_pMat->Unit();
				return SERVER_PHYS_HANDLED;
			}
		}

	default :
		return SERVER_PHYS_DEFAULTHANDLER;
	}
}

void CWObject_Crow::FlyNormal(void)
{
	CheckWallCollision();

	CWO_Critter_ClientData& CD = GetClientData(this);

	//Fly
	CVec3Dfp32 SPPos = m_pTargetSP->GetPos();
	CVec3Dfp32 TargetDir = SPPos - GetPosition();
	fp32 Length = TargetDir.Length();
	if(m_StartFlyingGameTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * 20.0f) < m_pWServer->GetGameTick())	
	{
		GetNextTarget();
		return;
	}

	bool bTurning = false;
	if(!m_pTargetSP->IsAt(GetPosition()))
	{
		if(15.0f > M_Sqrt(TargetDir.k[0] * TargetDir.k[0] + TargetDir.k[1] * TargetDir.k[1]))
			m_bForceCircling = true;

		TargetDir.Normalize();
		CMat4Dfp32 PosMat = GetPositionMatrix();
		CVec3Dfp32 MoveVec = GetPositionMatrix().GetRow(0) * m_SpeedForward;	
		CMat4Dfp32 TargetPos = GetPositionMatrix();

		if(m_bLand)
		{
			TargetPos.k[3][2] += 15.0f;
			SPPos.k[2] += 15.0f;
		}

		//Kill z rotation for now, we will apply it later so we can make sure we don't do any loops
		CVec3Dfp32 TmpTargetDir = TargetDir;
		TmpTargetDir.k[2] = PosMat.k[0][2];
		// Scale x, y to get unit length. Can't just normalize (z may still be too dominant).
		fp32 temp = M_Sqrt((1.0f - TmpTargetDir.k[2] * TmpTargetDir.k[2]) / (TmpTargetDir.k[0] * TmpTargetDir.k[0] + TmpTargetDir.k[1] * TmpTargetDir.k[1]));
		TmpTargetDir.k[0] *= temp;
		TmpTargetDir.k[1] *= temp;
		fp32 old_dot = TmpTargetDir * PosMat.GetRow(0);
		fp32 dot = M_ACos(old_dot) / (_PI * 2.0f);
		if((dot > 0.01f || dot < -0.01f || m_bForceCircling) && !m_bAvoidWall)
		{	//turn left/right
			CMat4Dfp32 MatPlaneRotation;

			bool bNoLeftRight = false;
			if(dot > 0.3f)
				bNoLeftRight = true;

			dot = Min(dot, 0.01f);
			dot = Max(dot, -0.01f);

			if(m_bForceCircling)
			{
				if(PosMat.GetRow(1) * TargetDir < 0.0f)
					dot = 0.01f;
				else
					dot = -0.01f;
			}

			if(m_bPanic)
			{
				dot = M_ACos(old_dot) / (_PI * 2.0f);
				dot = Min(dot, 0.05f);
				dot = Max(dot, -0.05f);
			}

			CVec3Dfp32 RotAxis;
			TmpTargetDir.CrossProd(PosMat.GetRow(0), RotAxis);
			RotAxis.Normalize();
			if(!RotAxis.LengthSqr())
				RotAxis.k[2] = 1.0f;
			CQuatfp32(RotAxis, dot).CreateMatrix3x3(MatPlaneRotation);
			PosMat.Multiply3x3(MatPlaneRotation, TargetPos);

			bTurning = true;
		}
		else if(m_bAvoidWall || m_bWallAhead)
		{
			AvoidWalls(&TargetPos);
			bTurning = true;
		}

		if(!bTurning && !m_bAvoidWall && !m_bWallAhead)	
		{
			if(!(SPPos.k[2] + 5.0f < PosMat.k[3][2] || SPPos.k[2] - 5.0f > PosMat.k[3][2]))
				m_bNoTraces = true;
		}
		//turn up/down
		if(SPPos.k[2] + 5.0f < PosMat.k[3][2])
		{	//We are above, fly down
			if(TargetPos.k[0][2] > -0.2f)
			{	//We can dive more
				CMat4Dfp32 MatPlaneRotation;
				CQuatfp32(TargetPos.GetRow(1), -0.02f).CreateMatrix3x3(MatPlaneRotation);
				CMat4Dfp32 Tmp;
				TargetPos.Multiply3x3(MatPlaneRotation, Tmp);
				TargetPos = Tmp;
			}
			else if(TargetPos.k[0][2] < -0.3f)
			{	//diving to much, go up some
				CVec3Dfp32 Up(0.0f, 0.0f, 1.0f);
				CVec3Dfp32 CurUp;
				CurUp = PosMat.GetRow(2);
				fp32 dot = M_ACos(Up * CurUp) / (_PI * 2.0f);
				if(dot > 0.01f || dot < -0.01f)
				{
					CMat4Dfp32 MatPlaneRotation;

					dot = Min(dot, 0.02f);
					dot = Max(dot, -0.02f);

					CVec3Dfp32 RotAxis;
					Up.CrossProd(CurUp, RotAxis);
					if(!RotAxis.LengthSqr())
						RotAxis = PosMat.GetRow(1);
					CQuatfp32(RotAxis, dot).CreateMatrix3x3(MatPlaneRotation);
					CMat4Dfp32 Tmp;
					TargetPos.Multiply3x3(MatPlaneRotation, Tmp);
					TargetPos = Tmp;
				}
			}

		}
		else if(SPPos.k[2] - 5.0f > PosMat.k[3][2])
		{	//We are under, fly up
			if(TargetPos.k[0][2] < 0.2f)
			{	//We can fly up more
				CMat4Dfp32 MatPlaneRotation;
				CQuatfp32(TargetPos.GetRow(1), 0.02f).CreateMatrix3x3(MatPlaneRotation);
				CMat4Dfp32 Tmp;
				TargetPos.Multiply3x3(MatPlaneRotation, Tmp);
				TargetPos = Tmp;
			}
			else if(TargetPos.k[0][2] > 0.3f)
			{	//climbing to much, go down some
				CVec3Dfp32 Up(0.0f, 0.0f, 1.0f);
				CVec3Dfp32 CurUp;
				CurUp = PosMat.GetRow(2);
				fp32 dot = M_ACos(Up * CurUp) / (_PI * 2.0f);
				if(dot > 0.01f || dot < -0.01f)
				{
					CMat4Dfp32 MatPlaneRotation;

					dot = Min(dot, 0.02f);
					dot = Max(dot, -0.02f);

					CVec3Dfp32 RotAxis;
					Up.CrossProd(CurUp, RotAxis);
					if(!RotAxis.LengthSqr())
						RotAxis = PosMat.GetRow(1);
					CQuatfp32(RotAxis, dot).CreateMatrix3x3(MatPlaneRotation);
					CMat4Dfp32 Tmp;
					TargetPos.Multiply3x3(MatPlaneRotation, Tmp);
					TargetPos = Tmp;
				}
			}
		}
		else
		{	//Not diving/climbing, make sure up is up
			m_bForceCircling = false;
			CVec3Dfp32 Up(0.0f, 0.0f, 1.0f);
			CVec3Dfp32 CurUp;
			CurUp = PosMat.GetRow(2);
			fp32 dot = M_ACos(Up * CurUp) / (_PI * 2.0f);
			if(dot > 0.01f || dot < -0.01f)
			{
				CMat4Dfp32 MatPlaneRotation;

				dot = Min(dot, 0.01f);
				dot = Max(dot, -0.01f);

				CVec3Dfp32 RotAxis;
				Up.CrossProd(CurUp, RotAxis);
				if(!RotAxis.LengthSqr())
					RotAxis = PosMat.GetRow(1);
				CQuatfp32(RotAxis, dot).CreateMatrix3x3(MatPlaneRotation);
				CMat4Dfp32 Tmp;
				TargetPos.Multiply3x3(MatPlaneRotation, Tmp);
				TargetPos = Tmp;
			}
		}

		if(TargetPos.k[2][2] < 0.7f)
		{	//We are leaning to much, fix it
			CVec3Dfp32 Up(0.0f, 0.0f, 1.0f);
			CVec3Dfp32 CurUp;
			CurUp = PosMat.GetRow(2);
			fp32 dot = M_ACos(Up * CurUp) / (_PI * 2.0f);
			if(dot > 0.01f || dot < -0.01f)
			{
				CMat4Dfp32 MatPlaneRotation;

				dot = Min(dot, 0.03f);
				dot = Max(dot, -0.03f);

				CVec3Dfp32 RotAxis;
				Up.CrossProd(CurUp, RotAxis);
				if(!RotAxis.LengthSqr())
					RotAxis = PosMat.GetRow(1);
				CQuatfp32(RotAxis, dot).CreateMatrix3x3(MatPlaneRotation);
				CMat4Dfp32 Tmp;
				TargetPos.Multiply3x3(MatPlaneRotation, Tmp);
				TargetPos = Tmp;
			}
		}

		//TODO
		//roll

		if(TargetPos.k[0][2] > 0.4f || TargetPos.k[0][2] < -0.4f)
		{	//Make sure we don't dive/climb to much
			TargetPos.k[0][2] = Min(TargetPos.k[0][2], 0.4f);
			TargetPos.k[0][2] = Max(TargetPos.k[0][2], -0.4f);

			fp32 temp = M_Sqrt((1.0f - TargetPos.k[0][2] * TargetPos.k[0][2]) / (TargetPos.k[0][0] * TargetPos.k[0][0] + TargetPos.k[0][1] * TargetPos.k[0][1]));
			TargetPos.k[0][0] *= temp;
			TargetPos.k[0][1] *= temp;

			//Måste göre en riktigt normalize efter fusk normalizen
			TargetPos.Normalize3x3();
		}

		MoveVec = TargetPos.GetRow(0) * m_SpeedForward;	//Should be speed
		TargetPos.GetRow(3) = PosMat.GetRow(3) + MoveVec;

//		fp32 CurBob = m_BobbingLastZMod;
//		TargetPos.k[3][2] += CurBob - UpdateBobbing();

		m_pWServer->Object_MoveTo(m_iObject, TargetPos);

		if(m_bPanic)
		{
			m_LastTraceTick2 = 0;
			m_LastTraceTick = m_pWServer->GetGameTick();
		}
	}
	else
	{	//We are at the target, get a new one
		if(!m_bLand)
			GetNextTarget();
		else
		{
			CVec3Dfp32 Dir = GetPositionMatrix().GetRow(0);
			Dir.k[2] = 0.0f;	//We don't want IsAligned to care about z direction, no need to normalize, IsAligned will do it for us
			if(m_pTargetSP->IsAligned(Dir))
			{
				CWO_Critter_ClientData& CD = GetClientData(this);
				CD.m_AIState = CROW_STATE_LANDING;
				CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();
				CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_LAND);
				CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
				pAGI->SendImpulse(&AGIContext, Impulse, 0);

				m_pTargetSP->Activate(m_iObject, m_pWServer, GetSPM());
			}
			else
			{
				GetNextTarget();
			}
		}
	}

	bool bShouldSoar = false;
	
	if(CD.m_AnimGraph.m_FlagsLo & CROW_FLAGS_FLYING)
		m_FlyTick++;
	else
		m_FlyTick = 0;

	CMat4Dfp32 Pos = GetPositionMatrix();
	if(Pos.k[0][2] < -0.1f && m_FlyTick > (20 + MRTC_RAND() % 10))
		bShouldSoar = true;
	else if(Pos.k[0][2] < 0.1f && m_FlyTick > (60 + MRTC_RAND() % 20))
		bShouldSoar = true;
	
	if(bShouldSoar && !bTurning)
	{
		CWO_Critter_ClientData& CD = GetClientData(this);
		CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();
		CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_SOAR);
		CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
		pAGI->SendImpulse(&AGIContext, Impulse, 0);
	}
}

void CWObject_Crow::FlyNormalNoSP(void)
{
	CheckWallCollision();

	CMat4Dfp32 PosMat = GetPositionMatrix();
	CMat4Dfp32 TargetPos = GetPositionMatrix();

	AvoidWalls(&TargetPos);

	CVec3Dfp32 Up(0.0f, 0.0f, 1.0f);
	CVec3Dfp32 CurUp;
	CurUp = PosMat.GetRow(2);
	fp32 dot = M_ACos(Up * CurUp) / (_PI * 2.0f);
	if(dot > 0.01f || dot < -0.01f)
	{
		CMat4Dfp32 MatPlaneRotation;

		dot = Min(dot, 0.01f);
		dot = Max(dot, -0.01f);

		CVec3Dfp32 RotAxis;
		Up.CrossProd(CurUp, RotAxis);
		if(!RotAxis.LengthSqr())
			RotAxis = PosMat.GetRow(1);
		CQuatfp32(RotAxis, dot).CreateMatrix3x3(MatPlaneRotation);
		CMat4Dfp32 Tmp;
		TargetPos.Multiply3x3(MatPlaneRotation, Tmp);
		TargetPos = Tmp;
	}

	CVec3Dfp32 MoveVec = TargetPos.GetRow(0) * m_SpeedForward;	//Should be speed
	TargetPos.GetRow(3) = PosMat.GetRow(3) + MoveVec;

//	fp32 CurBob = m_BobbingLastZMod;
//	TargetPos.k[3][2] += CurBob - UpdateBobbing();

	m_pWServer->Object_MoveTo(m_iObject, TargetPos);
}

void CWObject_Crow::FlyDeparting(void)
{
	CheckWallCollision();

	CVec3Dfp32 SPPos = m_pTargetSP->GetPos();
	CMat4Dfp32 PosMat = GetPositionMatrix();
	CMat4Dfp32 TargetPos = GetPositionMatrix();

	AvoidWalls(&TargetPos);

	if(SPPos.k[2] + 10.0f > PosMat.k[3][2])
	{	//Takeoff
		if(TargetPos.k[0][2] < 0.4f)
		{	//We can fly up more
			CMat4Dfp32 MatPlaneRotation;
			CQuatfp32(TargetPos.GetRow(1), 0.03f).CreateMatrix3x3(MatPlaneRotation);
			CMat4Dfp32 Tmp;
			TargetPos.Multiply3x3(MatPlaneRotation, Tmp);
			TargetPos = Tmp;
		}
		else if(TargetPos.k[0][2] > 0.5f)
		{	//climbing to much, go down some
			CVec3Dfp32 Up(0.0f, 0.0f, 1.0f);
			CVec3Dfp32 CurUp;
			CurUp = PosMat.GetRow(2);
			fp32 dot = M_ACos(Up * CurUp) / (_PI * 2.0f);
			if(dot > 0.01f || dot < -0.01f)
			{
				CMat4Dfp32 MatPlaneRotation;

				dot = Min(dot, 0.02f);
				dot = Max(dot, -0.02f);

				CVec3Dfp32 RotAxis;
				Up.CrossProd(CurUp, RotAxis);
				if(!RotAxis.LengthSqr())
					RotAxis = PosMat.GetRow(1);
				CQuatfp32(RotAxis, dot).CreateMatrix3x3(MatPlaneRotation);
				CMat4Dfp32 Tmp;
				TargetPos.Multiply3x3(MatPlaneRotation, Tmp);
				TargetPos = Tmp;
			}
		}
	}
	else
	{
		GetNextTarget();
		CWO_Critter_ClientData& CD = GetClientData(this);
		CD.m_AIState = CROW_STATE_FLYING;

		CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();
		CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_WEAPONTYPE, AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED);
		CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
		pAGI->SendImpulse(&AGIContext, Impulse, 0);
	}

	CVec3Dfp32 MoveVec = PosMat.GetRow(0) * m_SpeedForward;	//Should be speed
	TargetPos.GetRow(3) = PosMat.GetRow(3) + MoveVec;

	m_pWServer->Object_MoveTo(m_iObject, TargetPos);
}

void CWObject_Crow::FlyLanding(void)
{
	CMat4Dfp32 PosMat = GetPositionMatrix();
	CMat4Dfp32 TargetPos = PosMat;
	CVec3Dfp32 Pos = GetPosition();
	CVec3Dfp32 Up(0.0f, 0.0f, 1.0f);

	if(!m_pTargetSP->GetCrouchFlag())
	{	//If it's a non walking landing spot, we try and align ourself to it
		CVec3Dfp32 DirAt, DirTarget;
		DirAt = PosMat.GetRow(0);
		DirAt.k[2] = 0.0f;
		DirAt.Normalize();

		DirTarget = m_pTargetSP->GetDirection();

		fp32 old_dot = DirTarget * DirAt;
		fp32 dot = M_ACos(old_dot) / (_PI * 2.0f);
		if((dot > 0.01f || dot < -0.01f || m_bForceCircling) && !m_bAvoidWall)
		{	//turn left/right
			CMat4Dfp32 MatPlaneRotation;

			dot = Min(dot, 0.01f);
			dot = Max(dot, -0.01f);

			CQuatfp32(-Up, dot).CreateMatrix3x3(MatPlaneRotation);
			PosMat.Multiply3x3(MatPlaneRotation, TargetPos);
		}
	}

	CVec3Dfp32 CurUp;
	CurUp = PosMat.GetRow(2);
	fp32 dot = M_ACos(Up * CurUp) / (_PI * 2.0f);
	if(dot > 0.01f || dot < -0.01f)
	{
		CMat4Dfp32 MatPlaneRotation;

		dot = Min(dot, 0.01f);
		dot = Max(dot, -0.01f);

		CVec3Dfp32 RotAxis;
		Up.CrossProd(CurUp, RotAxis);
		if(!RotAxis.LengthSqr())
			RotAxis = PosMat.GetRow(1);
		CQuatfp32(RotAxis, dot).CreateMatrix3x3(MatPlaneRotation);
		CMat4Dfp32 Tmp;
		TargetPos.Multiply3x3(MatPlaneRotation, Tmp);
		TargetPos = Tmp;
	}

	CVec3Dfp32 MoveVec = m_pTargetSP->GetPos() - GetPosition();
	fp32 length = MoveVec.Length();
	MoveVec.Normalize();
	fp32 scale = 1.0f;
	if(length < m_SpeedForward)
		scale = length / m_SpeedForward;
	TargetPos.GetRow(3) = Pos + MoveVec * m_SpeedForward * scale;

//	fp32 CurBob = m_BobbingLastZMod;
//	TargetPos.k[3][2] += CurBob - UpdateBobbing();

	m_pWServer->Object_MoveTo(m_iObject, TargetPos);

	if(length < 2.0f)
	{
		CWO_Critter_ClientData& CD = GetClientData(this);
		if(m_pTargetSP->GetCrouchFlag())
		{
			CD.m_AIState = CROW_STATE_WALK;
			m_WalkIdleTick = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (3 + MRTC_RAND() % 4));	//We will idle between 3-7 seconds
			DoRandomIdleGesture();
			m_BoredomNextTick = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (m_BoredomMinTime + MRTC_RAND() % m_BoredomRandTime));
			m_WalkTick = m_pWServer->GetGameTick() + 10000;
			m_bWalking = false;
			if(m_pTargetSP)
			{
				m_pTargetSP->Release(m_iObject, m_pWServer, GetSPM());
			}
		}
		else
		{
			CD.m_AIState = CROW_STATE_LANDED;
		
			DoRandomIdleGesture();
			m_BoredomNextTick = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (m_BoredomMinTime + MRTC_RAND() % m_BoredomRandTime));
		}
	}
}

void CWObject_Crow::FlyEscape(void)
{
	CheckWallCollision();

	CVec3Dfp32 SPPos = GetPosition();
	if(m_pTargetSP)
		SPPos = m_pTargetSP->GetPos();
	CMat4Dfp32 PosMat = GetPositionMatrix();
	CMat4Dfp32 TargetPos = PosMat;
	TargetPos.GetRow(3) = TargetPos.GetRow(3) + m_EscapeDir * 40.0f;

	AvoidWalls(&TargetPos);

	if(SPPos.k[2] + m_EscapeHeight > PosMat.k[3][2])
	{	//Takeoff
		if(TargetPos.k[0][2] < 0.4f)
		{	//We can fly up more
			CMat4Dfp32 MatPlaneRotation;
			CQuatfp32(TargetPos.GetRow(1), 0.03f).CreateMatrix3x3(MatPlaneRotation);
			CMat4Dfp32 Tmp;
			TargetPos.Multiply3x3(MatPlaneRotation, Tmp);
			TargetPos = Tmp;
		}
		else if(TargetPos.k[0][2] > 0.5f)
		{	//climbing to much, go down some
			CVec3Dfp32 Up(0.0f, 0.0f, 1.0f);
			CVec3Dfp32 CurUp;
			CurUp = PosMat.GetRow(2);
			fp32 dot = M_ACos(Up * CurUp) / (_PI * 2.0f);
			if(dot > 0.01f || dot < -0.01f)
			{
				CMat4Dfp32 MatPlaneRotation;

				dot = Min(dot, 0.02f);
				dot = Max(dot, -0.02f);

				CVec3Dfp32 RotAxis;
				Up.CrossProd(CurUp, RotAxis);
				if(!RotAxis.LengthSqr())
					RotAxis = PosMat.GetRow(1);
				CQuatfp32(RotAxis, dot).CreateMatrix3x3(MatPlaneRotation);
				CMat4Dfp32 Tmp;
				TargetPos.Multiply3x3(MatPlaneRotation, Tmp);
				TargetPos = Tmp;
			}
		}
	}
	else
	{
		GetNextTarget();
		CWO_Critter_ClientData& CD = GetClientData(this);
		CD.m_AIState = CROW_STATE_FLYING;

		CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();
		CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_WEAPONTYPE, AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED);
		CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
		pAGI->SendImpulse(&AGIContext, Impulse, 0);
	}

	CVec3Dfp32 MoveVec = PosMat.GetRow(0) * m_SpeedForward;	//Should be speed
	TargetPos.GetRow(3) = PosMat.GetRow(3) + MoveVec;

	m_pWServer->Object_MoveTo(m_iObject, TargetPos);
}

void CWObject_Crow::IdleLanded(void)
{	
	if(!m_bWaitOnImpulse && m_BoredomNextTick < m_pWServer->GetGameTick())
	{
		CWO_Critter_ClientData& CD = GetClientData(this);
		CD.m_AIState = CROW_STATE_DEPARTING;

		CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();

		CXRAG2_Impulse ImpulseUnarmed(AG2_IMPULSETYPE_WEAPONTYPE, AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED);
		CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
		pAGI->SendImpulse(&AGIContext, ImpulseUnarmed, 0);

		CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_TAKEOFF);
		pAGI->SendImpulse(&AGIContext, Impulse, 0);

		m_bNoTraces = false;

		m_pTargetSP->Release(m_iObject, m_pWServer, GetSPM());
	}
	else
	{
		//Check for scary stuff
		uint32 compare = m_LastTraceTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * 0.5f);
		if(compare < m_pWServer->GetGameTick())
		{
			m_LastTraceTick = m_pWServer->GetGameTick();
			if(CheckForScaryStuff(GetPosition(), &m_EscapeDir))
			{
				if(m_EscapeDir.k[2] < 0.5f)
				{
					m_EscapeDir.k[2] = 0.5f;
					//'fake' normalize
					fp32 temp = M_Sqrt((1.0f - m_EscapeDir.k[2] * m_EscapeDir.k[2]) / (m_EscapeDir.k[0] * m_EscapeDir.k[0] + m_EscapeDir.k[1] * m_EscapeDir.k[1]));
					m_EscapeDir.k[0] *= temp;
					m_EscapeDir.k[1] *= temp;
				}

				CWO_Critter_ClientData& CD = GetClientData(this);
				CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();

				CXRAG2_Impulse ImpulseUnarmed(AG2_IMPULSETYPE_WEAPONTYPE, AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED);
				CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
				pAGI->SendImpulse(&AGIContext, ImpulseUnarmed, 0);

				CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_TAKEOFF);
				pAGI->SendImpulse(&AGIContext, Impulse, 0);

				//Oh noez!1! scary stuff!!!
				CD.m_AIState = CROW_STATE_ESCAPING;

				m_pWServer->Sound_At(GetPosition(), m_iSound, 0);
				m_SoundNextTick = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (m_SoundMinTime * MRTC_RAND() % m_SoundRandTime));

				m_bNoTraces = false;
				m_bWaitOnImpulse = false;

				if(m_pTargetSP)
				{
					m_pTargetSP->Release(m_iObject, m_pWServer, GetSPM());
				}
			}
		}
	}

	CWO_Critter_ClientData& CD = GetClientData(this);
	if(!(CD.m_AnimGraph.m_FlagsLo & CROW_FLAGS_IDLE_BEHAVIOR))
	{
		m_IdleTick++;
		if(m_IdleTick > 100)
		{
			m_NextIdleBehavior = m_pWServer->GetGameTick() + MRTC_RAND() % 80;
			m_IdleTick = 0;
		}
		if(m_NextIdleBehavior && m_NextIdleBehavior <  m_pWServer->GetGameTick())
		{
			DoRandomIdleGesture();
			m_NextIdleBehavior = 0;
		}
	}
	else
	{
		m_IdleTick = 0;
	}
}

void CWObject_Crow::Walk(void)
{
	CWO_Critter_ClientData& CD = GetClientData(this);
	uint32 compare = m_LastTraceTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * 0.5f);
	if(compare < m_pWServer->GetGameTick())
	{
		m_LastTraceTick = m_pWServer->GetGameTick();
		if(CheckForScaryStuff(GetPosition(), &m_EscapeDir))
		{
			if(m_EscapeDir.k[2] < 0.5f)
			{
				m_EscapeDir.k[2] = 0.5f;
				//'fake' normalize
				fp32 temp = M_Sqrt((1.0f - m_EscapeDir.k[2] * m_EscapeDir.k[2]) / (m_EscapeDir.k[0] * m_EscapeDir.k[0] + m_EscapeDir.k[1] * m_EscapeDir.k[1]));
				m_EscapeDir.k[0] *= temp;
				m_EscapeDir.k[1] *= temp;
			}

			CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();

			CXRAG2_Impulse ImpulseUnarmed(AG2_IMPULSETYPE_WEAPONTYPE, AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED);
			CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
			pAGI->SendImpulse(&AGIContext, ImpulseUnarmed, 0);

			CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_TAKEOFF);
			pAGI->SendImpulse(&AGIContext, Impulse, 0);

			//Oh noez!1! scary stuff!!!
			CD.m_AIState = CROW_STATE_ESCAPING;

			m_pWServer->Sound_At(GetPosition(), m_iSound, 0);
			m_SoundNextTick = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (m_SoundMinTime * MRTC_RAND() % m_SoundRandTime));

			m_bNoTraces = false;

			if(m_pTargetSP)
			{
				m_pTargetSP->Release(m_iObject, m_pWServer, GetSPM());
			}
		}
	}


	if(m_WalkIdleTick < m_pWServer->GetGameTick())
	{	//We've been idle long enough, let's walk around
		if(MRTC_RAND() % 10 < 2)
		{
			if(GetNextTarget())
			{
				CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();

				CXRAG2_Impulse ImpulseUnarmed(AG2_IMPULSETYPE_WEAPONTYPE, AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED);
				CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
				pAGI->SendImpulse(&AGIContext, ImpulseUnarmed, 0);

				CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_TAKEOFF);
				pAGI->SendImpulse(&AGIContext, Impulse, 0);
					
				CD.m_AIState = CROW_STATE_DEPARTING;
				m_bNoTraces = false;
				return;
			}
			else
			{
				m_EscapeDir = GetPositionMatrix().GetRow(0);
				if(m_EscapeDir.k[2] < 0.5f)
				{
					m_EscapeDir.k[2] = 0.5f;
					//'fake' normalize
					fp32 temp = M_Sqrt((1.0f - m_EscapeDir.k[2] * m_EscapeDir.k[2]) / (m_EscapeDir.k[0] * m_EscapeDir.k[0] + m_EscapeDir.k[1] * m_EscapeDir.k[1]));
					m_EscapeDir.k[0] *= temp;
					m_EscapeDir.k[1] *= temp;
				}

				CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();

				CXRAG2_Impulse ImpulseUnarmed(AG2_IMPULSETYPE_WEAPONTYPE, AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED);
				CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
				pAGI->SendImpulse(&AGIContext, ImpulseUnarmed, 0);

				CXRAG2_Impulse Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_TAKEOFF);
				pAGI->SendImpulse(&AGIContext, Impulse, 0);

				CD.m_AIState = CROW_STATE_ESCAPING;
				m_bNoTraces = false;
				return;
			}
		}
		else
		{
			m_WalkTick = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (1 + MRTC_RAND() % 2));	//We will walk between 1-3 seconds
			m_WalkDir.k[0] += ((MRTC_RAND() % 1001) / 2000.0f) - 0.25f;
			m_WalkDir.k[1] += ((MRTC_RAND() % 1001) / 2000.0f) - 0.25f;
			m_WalkDir.k[2] = 0.0f;
			m_WalkDir.Normalize();
			m_WalkIdleTick = m_pWServer->GetGameTick() + 10000;
			m_bWalking = true;
			CVec3Dfp32 Dir = m_pTargetSP->GetPos() - GetPosition();
			if(Dir.LengthSqr() > m_pTargetSP->GetSqrArcMinRange())
				m_WalkDir = Dir.Normalize();
		}
	}

	if(m_WalkTick < m_pWServer->GetGameTick())
	{	//We've been walking long enough, let's idle
		m_WalkIdleTick = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (3 + MRTC_RAND() % 4));	//We will idle between 3-7 seconds
		DoRandomIdleGesture();
		m_BoredomNextTick = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (m_BoredomMinTime + MRTC_RAND() % m_BoredomRandTime));
		m_WalkTick = m_pWServer->GetGameTick() + 10000;
		m_bWalking = false;
	}

	if(!m_bWalking)
		return;

	CheckWallCollision(true);

	CVec3Dfp32 TargetDir;
	TargetDir = m_WalkDir;

	CVec3Dfp32 TD = TargetDir * 5.0f;
	CVec3Dfp32 SP = GetPosition();
	SP.k[2] += 5.0f;
	m_pWServer->Debug_RenderVector(SP, TD, 0xffffffff);
		
	TargetDir.Normalize();
	CMat4Dfp32 PosMat = GetPositionMatrix();
	CMat4Dfp32 TargetPos = GetPositionMatrix();

	//Kill z rotation for now, we will apply it later so we can make sure we don't do any loops
	CVec3Dfp32 TmpTargetDir = PosMat.GetRow(0);
	TmpTargetDir.k[2] = 0.0f;
	TmpTargetDir.Normalize();

	fp32 old_dot = TmpTargetDir * TargetDir;
	fp32 dot = M_ACos(old_dot) / (_PI * 2.0f);
	if((dot > 0.01f || dot < -0.01f) && !m_bAvoidWall)
	{	//turn left/right
		CMat4Dfp32 MatPlaneRotation;

		bool bNoLeftRight = false;
		if(dot > 0.3f)
			bNoLeftRight = true;

		if(!bNoLeftRight && PosMat.GetRow(1) * TargetDir > 0.0f)
			dot = -dot;

		dot = Min(dot, 0.01f);
		dot = Max(dot, -0.01f);

		if(m_bPanic)
		{
			dot = M_ACos(old_dot) / (_PI * 2.0f);
			dot = Min(dot, 0.15f);
			dot = Max(dot, -0.15f);
		}

		CVec3Dfp32 Up(0.0f, 0.0f, 1.0f);
		CQuatfp32(Up, dot).CreateMatrix3x3(MatPlaneRotation);
		PosMat.Multiply3x3(MatPlaneRotation, TargetPos);
	}
	else if(m_bAvoidWall || m_bWallAhead)
	{
		AvoidWalls(&TargetPos);
	}

	CVec3Dfp32 MoveVec = TargetPos.GetRow(0) * 0.75f;	//Should be speed
	TargetPos.GetRow(3) = PosMat.GetRow(3) + MoveVec;

	m_pWServer->Object_MoveTo(m_iObject, TargetPos);

	CD.m_AnimGraph.SetPropertyFloat(PROPERTY_FLOAT_MOVERADIUSCONTROL, 1.0f);
	CD.m_AnimGraph.SetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITCONTROL, 0.0f);

	if(m_bPanic)
	{
		m_LastTraceTick2 = 0;
		m_LastTraceTick = m_pWServer->GetGameTick();
	}
}

void CWObject_Crow::DoRandomIdleGesture(void)
{
	CWO_Critter_ClientData& CD = GetClientData(this);
	CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();
	CXRAG2_Impulse Impulse;
	Impulse.m_ImpulseType = AG2_IMPULSETYPE_BEHAVIOR;
	switch(MRTC_RAND() % 3)
	{
	case 0:
		Impulse.m_ImpulseValue = CROW_AG2_GESTURE_SHRUG;
		break;
	case 1:
		Impulse.m_ImpulseValue = CROW_AG2_GESTURE_LOOK;
		break;
	case 2:
		Impulse.m_ImpulseValue = CROW_AG2_GESTURE_PICKFEATHERS;
		break;
	}
	CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
	pAGI->SendImpulse(&AGIContext, Impulse, 0);
	SetDirty(CRITTER_AG2_DIRTYMASK);
}

/*void DebugNavGridCritter(CXR_BlockNavSearcher *_pBlock, CWorld_Server *_pServer, int _x, int _y, int _z, int _Val)
{
	CVec3Dfp32 WPos = _pBlock->GetBlockNav()->GetWorldPosition(CVec3Dint16(_x, _y, _z));

	int32 Clr = 0xff000000;	// Black
	if(_Val & XR_CELL_AIR)
		Clr = 0xff0000ff;
	else
		Clr = 0xffff0000;

	_pServer->Debug_RenderWire(WPos + CVec3Dfp32(0,0,4), WPos - CVec3Dfp32(0,0,4), Clr, 0.1f); 
	_pServer->Debug_RenderWire(WPos + CVec3Dfp32(0,4,0), WPos - CVec3Dfp32(0,4,0), Clr, 0.1f); 
	_pServer->Debug_RenderWire(WPos + CVec3Dfp32(4,0,0), WPos - CVec3Dfp32(4,0,0), Clr, 0.1f); 
}*/

void CWObject_Crow::CheckWallCollision(bool _bNoTargetSPTrace)
{
	CVec3Dint16 Pos, TargetVec, LeftVec, RightVec;
	Pos = m_pBlockNavSearcher->GetBlockNav()->GetGridPosition(GetPosition());
	TargetVec = m_pBlockNavSearcher->GetBlockNav()->GetGridPosition(GetPosition() + GetPositionMatrix().GetRow(0) * 200.0f) - Pos;

	int FD = 0;
	int LD = 0;
	int RD = 0;

	CVec3Dfp32 CollisionPos(0.0f, 0.0f, 0.0f);

	int x, y, z, Val;
	for(int i = 0; i < 20; i++)
	{
		if(!FD)
		{
			x = RoundToInt(Pos.k[0] + (TargetVec.k[0] / 20.0f) * i);
			y = RoundToInt(Pos.k[1] + (TargetVec.k[1] / 20.0f) * i);
			z = RoundToInt(Pos.k[2] + (TargetVec.k[2] / 20.0f) * i);
			Val = m_pBlockNavSearcher->GetBlockNav()->GetCellValue(x, y, z);

//			DebugNavGridCritter(m_pBlockNavSearcher, m_pWServer, x, y, z, Val);

			if(!(Val & XR_CELL_AIR) || (Val & (XR_CELL_WALL | XR_CELL_DIE)))
			{
				FD = i;
				Pos = CVec3Dint16(x, y, z);
				CollisionPos = m_pBlockNavSearcher->GetBlockNav()->GetWorldPosition(CVec3Dint16(x, y, z));
				break;
			}
		}
	}

	m_FlyDir = FLY_FORWARD;
	m_MaxTurnSpeed = 0.01f;
	if(FD)
	{	//Collision straight ahead
		LeftVec = m_pBlockNavSearcher->GetBlockNav()->GetGridPosition(CollisionPos + GetPositionMatrix().GetRow(1) * 100.0f) - CVec3Dint16(x, y, z);
		RightVec = m_pBlockNavSearcher->GetBlockNav()->GetGridPosition(CollisionPos + GetPositionMatrix().GetRow(1) * -100.0f) - CVec3Dint16(x, y, z);

		m_bWallAhead = true;
		m_bAvoidWall = true;

		if(FD < 5)
			m_MaxTurnSpeed = 0.05f;
		
		for(int i = 0; i < 10; i++)
		{
			if(!LD)
			{
				x = RoundToInt(Pos.k[0] + (LeftVec.k[0] / 10.0f) * i);
				y = RoundToInt(Pos.k[1] + (LeftVec.k[1] / 10.0f) * i);
				z = RoundToInt(Pos.k[2] + (LeftVec.k[2] / 10.0f) * i);
				Val = m_pBlockNavSearcher->GetBlockNav()->GetCellValue(x, y, z);

//				DebugNavGridCritter(m_pBlockNavSearcher, m_pWServer, x, y, z, Val);

				if(!(Val & XR_CELL_AIR) || (Val & (XR_CELL_WALL | XR_CELL_DIE)))
				{
					LD = i;
					break;
				}
			}

			if(!RD)
			{
				x = RoundToInt(Pos.k[0] + (RightVec.k[0] / 10.0f) * i);
				y = RoundToInt(Pos.k[1] + (RightVec.k[1] / 10.0f) * i);
				z = RoundToInt(Pos.k[2] + (RightVec.k[2] / 10.0f) * i);
				Val = m_pBlockNavSearcher->GetBlockNav()->GetCellValue(x, y, z);

//				DebugNavGridCritter(m_pBlockNavSearcher, m_pWServer, x, y, z, Val);

				if(!(Val & XR_CELL_AIR) || (Val & (XR_CELL_WALL | XR_CELL_DIE)))
				{
					RD = i;
					break;
				}
			}
		}

		if(LD)
			m_FlyDir = FLY_RIGHT;
		else 
			m_FlyDir = FLY_LEFT;
	}

	return;
	uint32 compare = m_LastTraceTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * 0.1f);
	if(compare < m_pWServer->GetGameTick())
	{	
		m_LastTraceTick = m_pWServer->GetGameTick();

		if(m_pTargetSP && !m_bNoTraces && !_bNoTargetSPTrace)
		{
			CCollisionInfo CInfo;
			CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
			int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
			int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
			int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
			int32 iExclude = m_iObject;

			CVec3Dfp32 TargetPos = m_pTargetSP->GetPos();
			CVec3Dfp32 StartPos = GetPosition();
			StartPos.k[2] += 2.5f;
			m_pWServer->Debug_RenderWire(StartPos, TargetPos, 0xff00ff00, 1.0f, true);

			fp32 dist = 100000.0f;
			bool bHit = m_pWServer->Phys_IntersectLine(StartPos, TargetPos, OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude);
			if(bHit)	//To target
			{	//Something is in the way, do a very simple attempt at flying around it
				//We are gonne follow the surface of the object that is between us and our target until we can see our target

				CVec3Dfp32 up(0.0f, 0.0f, 1.0f);
				CVec3Dfp32 new_wall_dir;
				CInfo.m_Plane.n.CrossProd(up, new_wall_dir);
				CVec3Dfp32 at = TargetPos - m_OldTargetPos;
				at.Normalize();

				fp32 test = new_wall_dir * at;
				if(test < 0.0f)
					up.CrossProd(CInfo.m_Plane.n, new_wall_dir);

				dist = CVec3Dfp32(GetPosition() - CInfo.m_Pos).Length();

				if(CInfo.m_Plane.n.k[2] > 0.9f || CInfo.m_Plane.n.k[2] < -0.9f)
				{	//This is a floor or ceiling, abort and get new target
					GetNextTarget();
				}

				m_bAvoidWall = true;
			}
			else
			{
				m_bAvoidWall = false;
			}
		}
	}

	compare = m_LastTraceTick2 + RoundToInt(m_pWServer->GetGameTicksPerSecond() * 0.1f);
	if(compare < m_pWServer->GetGameTick())
	{	
		m_LastTraceTick2 = m_pWServer->GetGameTick();

		if(!m_bNoTraces)
		{
			CCollisionInfo CInfo;
			CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
			int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
			int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
			int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
			int32 iExclude = m_iObject;

			CVec3Dfp32 pos = GetPosition();
			pos.k[2] += 2.5f;
			fp32 l = 100.0f;
			CWO_Critter_ClientData& CD = GetClientData(this);
			if(CD.m_AIState == CROW_STATE_WALK)
				l = 30.0f;
			CVec3Dfp32 TargetPos = pos + GetPositionMatrix().GetRow(0) * l;
			m_pWServer->Debug_RenderWire(pos, TargetPos, 0xff0000ff, 1.0f, true);
			bool bHit = m_pWServer->Phys_IntersectLine(pos, TargetPos, OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude);
			if(bHit)	//infront of bird
			{
				fp32 in_front_dist = CVec3Dfp32(pos - CInfo.m_Pos).Length();

				CVec3Dfp32 up(0.0f, 0.0f, 1.0f);
				CVec3Dfp32 new_wall_dir;
				CInfo.m_Plane.n.CrossProd(up, new_wall_dir);
				CVec3Dfp32 at = TargetPos - m_OldTargetPos;
				at.Normalize();

				fp32 test = new_wall_dir * at;
				if(test < 0.0f)
					up.CrossProd(CInfo.m_Plane.n, new_wall_dir);

				new_wall_dir.Normalize();

				//Make sure new_wall_dir isn't pointing against another wall (corner hack)
				TargetPos = pos + new_wall_dir * 20.0f;
				m_pWServer->Debug_RenderWire(pos, TargetPos, 0xffffffff);
				bHit = m_pWServer->Phys_IntersectLine(pos, TargetPos, OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude);
				if(bHit)	//We found a wall, this is a corner, invert wall dir
					new_wall_dir *= -1.0f;

/*				if(CInfo.m_Plane.n.k[2] > 0.7f || CInfo.m_Plane.n.k[2] < -0.7f)
				{	//This is a floor or ceiling, abort and get new target
					m_bWallAhead = false;
					return;
				}*/

				m_bWallAhead = true;
				fp32 p = 30.0f;
				if(CD.m_AIState == CROW_STATE_WALK)
					p = 15.0f;
				if(in_front_dist < p)
					m_bPanic = true;
				else
					m_bPanic = false;
			}
			else
			{
				m_bWallAhead = false;
				m_bPanic = false;
			}
		}
	}
}

void CWObject_Crow::AvoidWalls(CMat4Dfp32 *_pTargetPos)
{
	if(m_FlyDir != FLY_FORWARD)
	{
		CMat4Dfp32 PosMat = GetPositionMatrix();
		CMat4Dfp32 MatPlaneRotation;

		CVec3Dfp32 RotAxis(0.0f, 0.0f, 1.0f);
		fp32 Angle = m_MaxTurnSpeed;
		if(m_FlyDir == FLY_LEFT)
			Angle = -m_MaxTurnSpeed;
		CQuatfp32(RotAxis, Angle).CreateMatrix3x3(MatPlaneRotation);
		PosMat.Multiply3x3(MatPlaneRotation, *_pTargetPos);
	}
}

bool CWObject_Crow::GetNextTarget(void)
{
	CWObject_ScenePointManager* pSPM = GetSPM();
	if (pSPM)
	{
		TThinArray<CWO_ScenePoint *> lValidScenePoints;

		pSPM->Selection_Clear();
		pSPM->Selection_AddRange(GetPosition(), 960.0f);
		TArray<CWO_ScenePoint*> lpScenepoints = pSPM->Selection_Get();
		int16 iNextPoint = -1;

		lValidScenePoints.SetLen(lpScenepoints.Len());

		int num = 0;
		for(int i = 0; i < lpScenepoints.Len(); i++)
		{
			CWO_ScenePoint* pCur = lpScenepoints[i];

			if(pCur->GetType() & CWO_ScenePoint::ROAM && pCur->GetType() & CWO_ScenePoint::JUMP_CLIMB)
			{
				if(m_pTargetSP == pCur || m_pLastTargetSP == pCur)
					continue;

//				if(pCur->GetPositionMatrix().GetRow(2).k[2] != 1.0f)
//					continue;

				if(!pCur->IsValid(m_pWServer,m_NameHash))
					continue;

				if(!pCur->PeekRequest(m_iObject, m_pWServer))
					continue;

				if(!pCur->InFrontArc(GetPosition()))
					continue;

				lValidScenePoints[num] = pCur;
				num++;
			}
		}

		int count = 0;
		int ii = 0;
		if(num)
			ii = MRTC_RAND() % num;
		while(count < num)
		{
			count++;
			
			CWO_ScenePoint* pCur = lValidScenePoints[ii];
			ii++;
			if(ii == num)
				ii = 0;

			m_StartFlyingGameTick = m_pWServer->GetGameTick();
			pSPM->Selection_Clear();
			m_bForceCircling = false;

			m_LastTraceTick = 0;	//This will force new traces
			m_LastTraceTick2 = m_pWServer->GetGameTick();

			if(pCur->GetType() & CWO_ScenePoint::WALK_CLIMB)
			{
				if(!CheckForScaryStuff(pCur->GetPos()))
					m_bLand = true;
				else 
					continue;
			}
			else			
				m_bLand = false;

			if(!pCur->Request(m_iObject, m_pWServer, pSPM))
				continue;

			if(m_pTargetSP)
				m_OldTargetPos = m_pTargetSP->GetPos();
			else
				m_OldTargetPos = GetPosition();

			m_pLastTargetSP = m_pTargetSP;
			m_pTargetSP = pCur;

			m_bNoTraces = false;
			pSPM->Selection_Clear();
			return true;
		}

		pSPM->Selection_Clear();
	}
	if(m_pTargetSP)
		m_pLastTargetSP = m_pTargetSP;
	m_pTargetSP = NULL;

	return false;
}

/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWObject_Ratlanding
|__________________________________________________________________________________________________
\*************************************************************************************************/

MRTC_IMPLEMENT_SERIAL_WOBJECT(CWObject_Rat, CWObject_Critter, 0x0100);

CWObject_Rat::CWObject_Rat()
{
	m_SpeedForward			= 5.0f;
	m_EscapeRange			= 75.0f;
	m_BoredomMinTime		= 5;
	m_BoredomRandTime		= 3;
	m_BoredomNextTick		= 0;
	m_SoundMinTime			= 10;
	m_SoundRandTime			= 10;
	m_SoundNextTick			= 0;
	m_LastTraceTick			= 0;
	m_LastScaryTick			= 0;
	m_EscapeTick			= 0;
	m_NextChangeDir			= 0;
	m_HideTick				= 0;
	m_pTargetSP				= NULL;
	m_bWaitOnImpulse		= false;

	m_NameHash				= "Rat";
}

CWObject_Rat::~CWObject_Rat()
{

}

void CWObject_Rat::OnFinishEvalKeys(void)
{
	parent::OnFinishEvalKeys();

	CWO_Critter_ClientData& CD = GetClientData(this);

/*	int32 iAG2 = m_pWServer->GetMapData()->GetResourceIndex_AnimGraph2("AnimGraphs\\AG2Crow");
	CD.m_AnimGraph.GetAG2I()->AddResourceIndex_AnimGraph2(iAG2, "AnimGraphs\\AG2Crow");

	// Make sure all animations are precached
	CWAG2I_Context AGIContext(this, m_pWServer, m_pWServer->GetGameTime());
	CWAG2I* pAGI = CD.m_AnimGraph.GetAG2I();

	//TODO
	//replace crow with rat

	TArray<CXRAG2_Impulse> lImpulses;
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_TAKEOFF));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_LAND));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_SOAR));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_RESPONSE, CROW_AG2_FLY));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR, CROW_AG2_GESTURE_SHRUG));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR, CROW_AG2_GESTURE_LOOK));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_BEHAVIOR, CROW_AG2_GESTURE_PICKFEATHERS));
	lImpulses.Add(CXRAG2_Impulse(AG2_IMPULSETYPE_WEAPONTYPE, AG2_IMPULSEVALUE_WEAPONTYPE_UNARMED));
	pAGI->TagAnimSetFromImpulses(&AGIContext, m_pWServer->GetMapData(), m_pWServer->m_spWData, lImpulses);

	CWServer_Mod* pServerMod = safe_cast<CWServer_Mod>(m_pWServer);
	uint nAnimGraphs = pAGI->GetNumResource_AnimGraph2();
	for (uint i = 0; i < nAnimGraphs; i++)
		pServerMod->RegisterAnimGraph2(pAGI->GetResourceIndex_AnimGraph2(i));*/
}

void CWObject_Rat::OnCreate()
{
	parent::OnCreate();
	m_LastTraceTick = m_pWServer->GetGameTick() + (m_iObject % 20);
}

void CWObject_Rat::OnSpawnWorld(void)
{
	parent::OnSpawnWorld();

	if(m_bWaitOnImpulse)
	{
		CWO_Critter_ClientData& CD = GetClientData(this);
		CD.m_AIState = RAT_STATE_HIDING;
	}
}

void CWObject_Rat::ChangeDirRandom(void)
{
	if(m_NextChangeDir < m_pWServer->GetGameTick())
	{
		m_NextChangeDir = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (MRTC_RAND() % 20 / 10.0f) + 1.0f);
		CVec3Dfp32 new_target = GetPositionMatrix().GetRow(0);
		if(MRTC_RAND() % 10)
		{
			new_target.k[0] += ((MRTC_RAND() % 1001) / 2000.0f) - 0.25f;
			new_target.k[1] += ((MRTC_RAND() % 1001) / 2000.0f) - 0.25f;
			new_target.Normalize();
		}
		else
			new_target *= -1.0f;

		CCollisionInfo CInfo;
		CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
		int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
		int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
		int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
		int32 iExclude = m_iObject;

		CVec3Dfp32 pos = GetPosition();
		pos.k[2] += 1.0f; //Move line collision up a bit
		CVec3Dfp32 TargetPos = pos + new_target * 25.0f;
		m_pWServer->Debug_RenderWire(pos, TargetPos, 0xff0000ff, 1.0f, true);
		bool bHit = m_pWServer->Phys_IntersectLine(pos, TargetPos, OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude);
		if(bHit)	//infront of rat
			new_target *= -1.0f;

		m_TargetDir = new_target;
	}
}


aint CWObject_Rat::OnMessage(const CWObject_Message &_Msg)
{
	switch(_Msg.m_Msg)
	{
	case OBJMSG_IMPULSE:
		{
			m_EscapeDir = GetPositionMatrix().GetRow(0);

			CWO_Critter_ClientData& CD = GetClientData(this);
			CD.m_AIState = RAT_STATE_ESCAPE;

			if(m_pTargetSP)
			{
				m_pTargetSP->Release(m_iObject, m_pWServer, GetSPM());
			}

			m_bWaitOnImpulse = false;
			m_EscapeTick = m_pWServer->GetGameTick();

			return 1;

		}
		break;
	case OBJMSG_DAMAGE:
		{
			CWObject_Message Msg;
			Msg.m_Msg = OBJMSG_IMPULSE;
			m_pWServer->Message_SendToTarget(Msg, "Rat");

			Destroy();
		}
		break;
	}

	return parent::OnMessage(_Msg);
}

int CWObject_Rat::OnPhysicsEvent(CWObject_CoreData* _pObj, CWObject_CoreData* _pObjOther, CWorld_PhysState* _pPhysState, int _Event, fp32 _Time, fp32 _dTime, CMat4Dfp32* _pMat, CCollisionInfo* _pCollisionInfo)
{
	switch(_Event)
	{
	case CWO_PHYSEVENT_GETACCELERATION:
		{
			CWO_PhysicsState PhysState(_pObj->GetPhysState());

			CCollisionInfo CInfo_1;
			CMat4Dfp32 Dest; 
			Dest = _pObj->GetPositionMatrix();
			Dest.k[3][2] -= 5.0f;

			PhysState.m_ObjectIntersectFlags |= OBJECT_FLAGS_WORLD;

			bool bOnGround = _pPhysState->Phys_IntersectWorld((CSelection *) NULL, PhysState, _pObj->GetPositionMatrix(), Dest, _pObj->m_iObject, &CInfo_1);

			if(bOnGround)
			{
				_pMat->Unit();
				if(CInfo_1.m_Distance)
					_pMat->k[3][2] = -(5.0f + CInfo_1.m_Distance);
				return SERVER_PHYS_HANDLED;
			}
			else
			{
				_pMat->Unit();
				_pMat->k[3][2] = -5.0f;
				return SERVER_PHYS_HANDLED;
			}
		}

	default :
		return SERVER_PHYS_DEFAULTHANDLER;
	}
}

void CWObject_Rat::OnRefresh(void)
{
	parent::OnRefresh();

	m_pWServer->Debug_RenderMatrix(GetPositionMatrix(), 1.0f, true);

	CWO_Critter_ClientData& CD = GetClientData(this);
	CD.OnRefresh();

	UpdateSP();

	switch(CD.m_AIState) 
	{
	case RAT_STATE_IDLE:
	    {
			Idle();
	    }
		break;
	case RAT_STATE_EXPLORING:
	    {
			Explore();
	    }
		break;
	case RAT_STATE_ESCAPE:
	    {
			Escape();
	    }
		break;
	case RAT_STATE_HIDING:
		{
			Hide();
		}
		break;
	}
}

void CWObject_Rat::Explore(void)
{
	CWO_Critter_ClientData& CD = GetClientData(this);
	if(m_BoredomNextTick < m_pWServer->GetGameTick())
	{
		CD.m_AIState = RAT_STATE_IDLE;

		DoRandomIdleGesture();
	
		m_BoredomNextTick = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (m_BoredomMinTime + MRTC_RAND() % m_BoredomRandTime));
		return;
	}

	ChangeDirRandom();

	CheckWallCollision();

	CMat4Dfp32 PosMat = GetPositionMatrix();
	CVec3Dfp32 MoveVec = GetPositionMatrix().GetRow(0) * m_SpeedForward;	//Should be speed
	CMat4Dfp32 TargetPos = GetPositionMatrix();

	fp32 old_dot = m_TargetDir * PosMat.GetRow(0);
	fp32 dot = M_ACos(old_dot) / (_PI * 2.0f);
	if(dot > 0.01f || dot < -0.01f)
	{	//turn left/right
		dot = Min(dot, 0.1f);
		dot = Max(dot, -0.1f);

		CVec3Dfp32 right = -PosMat.GetRow(1);
		fp32 lor = m_TargetDir * right;
		if(lor < 0.0f)
			dot = -dot;

		CMat4Dfp32 MatPlaneRotation;
		CQuatfp32(PosMat.GetRow(2), dot).CreateMatrix3x3(MatPlaneRotation);
		PosMat.Multiply3x3(MatPlaneRotation, TargetPos);
	}

	MoveVec = TargetPos.GetRow(0) * m_SpeedForward;	//Should be speed
	TargetPos.GetRow(3) = PosMat.GetRow(3) + MoveVec;

	m_pWServer->Object_MoveTo(m_iObject, TargetPos);

	CD.m_AnimGraph.SetPropertyFloat(PROPERTY_FLOAT_MOVERADIUSCONTROL, 1.0f);
	CD.m_AnimGraph.SetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITCONTROL, 0.0f);

	//Check for scary stuff
	uint32 compare = m_LastScaryTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * 0.5f);
	if(compare < m_pWServer->GetGameTick())
	{
		m_LastScaryTick = m_pWServer->GetGameTick();
		if(CheckForScaryStuff(GetPosition(), &m_TargetDir))
		{
			CWO_Critter_ClientData& CD = GetClientData(this);
			//Oh noez!1! scary stuff!!!
			CD.m_AIState = RAT_STATE_ESCAPE;
			GetNextTarget();
			m_EscapeTick = m_pWServer->GetGameTick();
		}
	}
}

void CWObject_Rat::Idle(void)
{
	CWO_Critter_ClientData& CD = GetClientData(this);

	if(m_BoredomNextTick < m_pWServer->GetGameTick())
	{
		CWO_Critter_ClientData& CD = GetClientData(this);
		CD.m_AIState = RAT_STATE_EXPLORING;

		CVec3Dfp32 new_target = GetPositionMatrix().GetRow(0);
		new_target.k[0] += ((MRTC_RAND() % 2001) / 1000.0f) - 1.0f;
		new_target.k[1] += ((MRTC_RAND() % 2001) / 1000.0f) - 1.0f;
		new_target.Normalize();
		m_TargetDir = new_target;

		m_NextChangeDir = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (MRTC_RAND() % 20 / 10.0f) + 1.0f);

		m_BoredomNextTick = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (m_BoredomMinTime + MRTC_RAND() % m_BoredomRandTime));
	}
	else
	{
		//Check for scary stuff
		uint32 compare = m_LastScaryTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * 0.5f);
		if(compare < m_pWServer->GetGameTick())
		{
			m_LastScaryTick = m_pWServer->GetGameTick();
			if(CheckForScaryStuff(GetPosition(), &m_TargetDir))
			{
				CWO_Critter_ClientData& CD = GetClientData(this);
				//Oh noez!1! scary stuff!!!
				CD.m_AIState = RAT_STATE_ESCAPE;
				GetNextTarget();
				m_EscapeTick = m_pWServer->GetGameTick();
			}
		}
	}

	CD.m_AnimGraph.SetPropertyFloat(PROPERTY_FLOAT_MOVERADIUSCONTROL, 0.0f);
	CD.m_AnimGraph.SetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITCONTROL, 0.0f);
}

void CWObject_Rat::Escape(void)
{	//if we have a scenepoint, go there, otherwise run blindly straight on
	CMat4Dfp32 PosMat = GetPositionMatrix();
	CMat4Dfp32 TargetPos = GetPositionMatrix();

	if(m_pTargetSP)
	{
		m_TargetDir = m_pTargetSP->GetPos() - GetPosition();

		if(m_TargetDir.Length() < 10.0f)
		{
			CWO_Critter_ClientData& CD = GetClientData(this);
			CD.m_AIState = RAT_STATE_HIDING;
			m_HideTick = 0;

			m_pTargetSP->Activate(m_iObject, m_pWServer, GetSPM());
			return;
		}

		m_TargetDir.k[2] = 0.0f;
		m_TargetDir.Normalize();
	}
	else
	{	//We are running blindly, don't do it forever
		uint32 compare = m_EscapeTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * 5.0f);
		if(compare < m_pWServer->GetGameTick())
		{
			m_EscapeTick = m_pWServer->GetGameTick();
			if(!CheckForScaryStuff(GetPosition(), &m_TargetDir))
			{
				CWO_Critter_ClientData& CD = GetClientData(this);
				CD.m_AIState = RAT_STATE_EXPLORING;
				m_pTargetSP = NULL;
				m_NextChangeDir = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (MRTC_RAND() % 20 / 10.0f) + 1.0f);

				return;
			}
		}
		CheckWallCollision();
	}

	fp32 old_dot = m_TargetDir * PosMat.GetRow(0);
	fp32 dot = M_ACos(old_dot) / (_PI * 2.0f);
	if(dot > 0.01f || dot < -0.01f)
	{	//turn left/right
		dot = Min(dot, 0.15f);
		dot = Max(dot, -0.15f);

		CMat4Dfp32 MatPlaneRotation;
		CQuatfp32(CVec3Dfp32(0.0f, 0.0f, 1.0f), dot).CreateMatrix3x3(MatPlaneRotation);
		PosMat.Multiply3x3(MatPlaneRotation, TargetPos);
	}

	CVec3Dfp32 MoveVec = TargetPos.GetRow(0) * m_SpeedForward;	//Should be speed
	TargetPos.GetRow(3) = PosMat.GetRow(3) + MoveVec;

	m_pWServer->Object_MoveTo(m_iObject, TargetPos);

	CWO_Critter_ClientData& CD = GetClientData(this);
	CD.m_AnimGraph.SetPropertyFloat(PROPERTY_FLOAT_MOVERADIUSCONTROL, 1.0f);
	CD.m_AnimGraph.SetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITCONTROL, 0.0f);
}

void CWObject_Rat::Hide(void)
{	//We are at the scenepoint, hide here until player is too close, then escape again, or until player is far away, then exploring
	CVec3Dfp32 escape;
	m_HideTick++;
	uint32 compare = m_LastScaryTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * 1.0f);

	CWO_Critter_ClientData& CD = GetClientData(this);
	CD.m_AnimGraph.SetPropertyFloat(PROPERTY_FLOAT_MOVERADIUSCONTROL, 0.0f);
	CD.m_AnimGraph.SetPropertyFloat(PROPERTY_FLOAT_MOVEANGLEUNITCONTROL, 0.0f);

	if(compare < m_pWServer->GetGameTick())
	{
		m_LastScaryTick = m_pWServer->GetGameTick();
		fp32 dist = CheckForScaryStuff(GetPosition(), &escape);

		if(dist && dist < m_EscapeRange / 2.0f)
		{
			CD.m_AIState = RAT_STATE_ESCAPE;
			m_TargetDir = escape;

			if(m_pTargetSP)
			{
				m_pTargetSP->Release(m_iObject, m_pWServer, GetSPM());
				m_pTargetSP = NULL;
			}
		}
		else if(!m_bWaitOnImpulse && dist < 1.0f && m_HideTick > 100)
		{
			CWO_Critter_ClientData& CD = GetClientData(this);
			CD.m_AIState = RAT_STATE_EXPLORING;
			m_NextChangeDir = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (MRTC_RAND() % 20 / 10.0f) + 1.0f);
			m_TargetDir = m_pTargetSP->GetDirection();
			m_BoredomNextTick = m_pWServer->GetGameTick() + RoundToInt(m_pWServer->GetGameTicksPerSecond() * (m_BoredomMinTime + MRTC_RAND() % m_BoredomRandTime));

			m_pTargetSP->Release(m_iObject, m_pWServer, GetSPM());
			m_pTargetSP = NULL;
		}
	}
	else
	{
		//Align with the SP, so we wont start with running into the wall when we are done hiding
		if(m_pTargetSP)
		{
			CMat4Dfp32 PosMat = GetPositionMatrix();
			fp32 old_dot = m_pTargetSP->GetDirection() * PosMat.GetRow(0);
			fp32 dot = M_ACos(old_dot) / (_PI * 2.0f);
			if(dot > 0.01f || dot < -0.01f)
			{	//turn left/right
				dot = Min(dot, 0.05f);
				dot = Max(dot, -0.05f);

				CMat4Dfp32 MatPlaneRotation;
				CQuatfp32(CVec3Dfp32(0.0f, 0.0f, 1.0f), dot).CreateMatrix3x3(MatPlaneRotation);
				CMat4Dfp32 TargetPos;
				PosMat.Multiply3x3(MatPlaneRotation, TargetPos);
				TargetPos.GetRow(3) = PosMat.GetRow(3);

				m_pWServer->Object_MoveTo(m_iObject, TargetPos);
			}
			else
			{	//Do random fear behaviors?
				//TODO
				//Need timer and ag flags here
				DoRandomIdleGesture();
			}
		}
		else
		{	//Do random fear behaviors?
			//TODO
			//Need timer and ag flags here
			DoRandomIdleGesture();
		}
	}
}

bool CWObject_Rat::GetNextTarget(void)
{
	CWObject_ScenePointManager* pSPM = GetSPM();
	if (pSPM)
	{
		TThinArray<CWO_ScenePoint *> lValidScenePoints;

		pSPM->Selection_Clear();
		pSPM->Selection_AddRange(GetPosition(), 960.0f);
		TArray<CWO_ScenePoint*> lpScenepoints = pSPM->Selection_Get();
		int16 iNextPoint = -1;

		lValidScenePoints.SetLen(lpScenepoints.Len());

		int num = 0;
		for(int i = 0; i < lpScenepoints.Len(); i++)
		{
			CWO_ScenePoint* pCur = lpScenepoints[i];

			if((pCur->GetType() & CWO_ScenePoint::ROAM) && (pCur->GetType() & CWO_ScenePoint::WALK_CLIMB) && !(pCur->GetType() & CWO_ScenePoint::JUMP_CLIMB))
			{
				if(m_pTargetSP == pCur)
					continue;

				if(!pCur->IsValid(m_pWServer, m_NameHash))
					continue;

				if(!pCur->PeekRequest(m_iObject, m_pWServer))
					continue;

				if(!pCur->InFrontArc(GetPosition()))
					continue;

				lValidScenePoints[num] = pCur;
				num++;
			}
		}

		int count = 0;
		int ii = 0;
		if(num)
			ii = MRTC_RAND() % num;
		while(count < num)
		{
			count++;

			CWO_ScenePoint* pCur = lValidScenePoints[ii];
			ii++;
			if(ii == num)
				ii = 0;

			pSPM->Selection_Clear();

			m_LastTraceTick = 0;	//This will force new traces

			if(CheckForScaryStuff(pCur->GetPos()))
				continue;

			CCollisionInfo CInfo;
			CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
			int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
			int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
			int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
			int32 iExclude = m_iObject;

			CVec3Dfp32 pos = GetPosition();
			pos.k[2] += 1.0f; //Move line collision up a bit
			CVec3Dfp32 TargetPos = pCur->GetPos();
			m_pWServer->Debug_RenderWire(pos, TargetPos, 0xff0000ff, 1.0f, true);
			bool bHit = m_pWServer->Phys_IntersectLine(pos, TargetPos, OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude);
			if(bHit)	
				continue;

			if(!pCur->Request(m_iObject, m_pWServer, pSPM))
				continue;

			m_pTargetSP = pCur;

			pSPM->Selection_Clear();
			return true;
		}

		pSPM->Selection_Clear();
	}

	m_pTargetSP = NULL;

	return false;
}

void CWObject_Rat::DoRandomIdleGesture(void)
{

}

void CWObject_Rat::CheckWallCollision(void)
{
	CVec3Dint16 Pos, TargetVec;
	Pos = m_pBlockNavSearcher->GetBlockNav()->GetGridPosition(GetPosition());
	TargetVec = m_pBlockNavSearcher->GetBlockNav()->GetGridPosition(GetPosition() + GetPositionMatrix().GetRow(0) * 100.0f) - Pos;

	int FD = 0;

	int x, y, z, Val;
	for(int i = 0; i < 10; i++)
	{
		if(!FD)
		{
			x = RoundToInt(Pos.k[0] + (TargetVec.k[0] / 10.0f) * i);
			y = RoundToInt(Pos.k[1] + (TargetVec.k[1] / 10.0f) * i);
			z = RoundToInt(Pos.k[2] + (TargetVec.k[2] / 10.0f) * i);
			Val = m_pBlockNavSearcher->GetBlockNav()->GetCellValue(x, y, z);

//			DebugNavGridCritter(m_pBlockNavSearcher, m_pWServer, x, y, z, Val);

			if(!(Val & (XR_CELL_AIR | XR_CELL_TRAVERSABLE)) || (Val & (XR_CELL_WALL | XR_CELL_DIE)))
			{
				FD = i;
				break;
			}
		}
	}

	if(FD)
	{	//Collision straight ahead
		uint32 compare = m_LastTraceTick + RoundToInt(m_pWServer->GetGameTicksPerSecond() * 0.1f);
		if(compare < m_pWServer->GetGameTick())
		{	
			m_LastTraceTick = m_pWServer->GetGameTick();

			CCollisionInfo CInfo;
			CInfo.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
			int32 OwnFlags = OBJECT_FLAGS_PROJECTILE;
			int32 ObjectFlags = OBJECT_FLAGS_WORLD | OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT;
			int32 MediumFlags = XW_MEDIUM_SOLID | XW_MEDIUM_PHYSSOLID | XW_MEDIUM_GLASS;
			int32 iExclude = m_iObject;

			CVec3Dfp32 pos = GetPosition();
			pos.k[2] += 1.0f; //Move line collision up a bit
			CVec3Dfp32 TargetPos = pos + GetPositionMatrix().GetRow(0) * 50.0f;
			m_pWServer->Debug_RenderWire(pos, TargetPos, 0xff0000ff, 1.0f, true);
			bool bHit = m_pWServer->Phys_IntersectLine(pos, TargetPos, OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude);
			if(bHit)	//infront of rat
			{
				CVec3Dfp32 up(0.0f, 0.0f, 1.0f);
				CVec3Dfp32 new_wall_dir;
				CInfo.m_Plane.n.CrossProd(up, new_wall_dir);
				CVec3Dfp32 at = TargetPos - pos;
				at.Normalize();

				fp32 test = new_wall_dir * at;
				if(test < 0.0f)
					up.CrossProd(CInfo.m_Plane.n, new_wall_dir);

				//Make sure new_wall_dir is pointing against another wall (corner hack)
				TargetPos = pos + new_wall_dir * 40.0f;
				bHit = m_pWServer->Phys_IntersectLine(pos, TargetPos, OwnFlags, ObjectFlags, MediumFlags, &CInfo, iExclude);
				if(bHit)	//We found a wall, this is a corner, invert wall dir
					new_wall_dir *= -1.0f;

				m_TargetDir = new_wall_dir;
				m_TargetDir.Normalize();
			}
		}
	}
}

