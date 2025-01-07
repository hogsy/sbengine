#include "PCH.h"
#include "WServer_Core.h"
#include "../WPhysState_Hash.h"
#include "../WEvilTemplate.h"
#include "../WDynamics.h"
#include "../WDynamicsSupport.h"
#include "../WObjects/WObj_PhysCluster.h"

#define SERVER_VERBOSE 0
#define VERBOSE_OUT (!(SERVER_VERBOSE)) ? (void)0 : M_TRACE

#ifdef _DEBUG
	#include "MFloat.h"
	#define DEBUG_CHECK_ROW(v, r)\
		M_ASSERT(!FloatIsInvalid((v).k[r][0]) && \
		         !FloatIsInvalid((v).k[r][1]) && \
		         !FloatIsInvalid((v).k[r][2]) && \
		         !FloatIsInvalid((v).k[r][3]) && \
		         (M_Fabs((v).k[r][0]) + M_Fabs((v).k[r][1]) + M_Fabs((v).k[r][2]) + M_Fabs((v).k[r][3]) < 1000000.0f), "Invalid vector!");
	#define DEBUG_CHECK_MATRIX(m)\
		DEBUG_CHECK_ROW(m, 0)\
		DEBUG_CHECK_ROW(m, 1)\
		DEBUG_CHECK_ROW(m, 2)\
		DEBUG_CHECK_ROW(m, 3)
	#define DEBUG_CHECK_VECTOR(v) \
		M_ASSERT(!FloatIsInvalid((v).k[0]) && \
		         !FloatIsInvalid((v).k[1]) && \
		         !FloatIsInvalid((v).k[2]) && \
		         (M_Fabs((v).k[0]) + M_Fabs((v).k[1]) + M_Fabs((v).k[2]) < 1000000.0f), "Invalid vector!");
	#define DEBUG_CHECK_AXISANGLE(q) \
		M_ASSERT(!FloatIsInvalid((q).m_Axis.k[0]) && \
		         !FloatIsInvalid((q).m_Axis.k[1]) && \
		         !FloatIsInvalid((q).m_Axis.k[2]) && \
		         !FloatIsInvalid((q).m_Angle) && \
		         (M_Fabs((q).m_Axis.k[0]) + M_Fabs((q).m_Axis.k[0]) + M_Fabs((q).m_Axis.k[0]) + M_Fabs((q).m_Angle) <= 1000000.0f), "Invalid axis-angle!");
#else
	#define DEBUG_CHECK_ROW(v, r)
	#define DEBUG_CHECK_MATRIX(m)
	#define DEBUG_CHECK_VECTOR(v)
	#define DEBUG_CHECK_AXISANGLE(q)
#endif

//#pragma optimize("", off)
//#pragma inline_depth(0)

/*static void DumpSearchTree_r(CTargetNameTreeNode* _pNode, int _RecLevel, int _iChild = 0)
{
	MAUTOSTRIP(DumpSearchTree_r, NULL);
	LogFile(CStrF(' ', _RecLevel*4) + CStrF("%i.", _iChild) + _pNode->GetElement().m_Name);

	if(_pNode->GetChild(0))
		DumpSearchTree_r(_pNode->GetChild(0), _RecLevel + 1, 0);
	if(_pNode->GetChild(1))
		DumpSearchTree_r(_pNode->GetChild(1), _RecLevel + 1, 1);
}*/

// -------------------------------------------------------------------
int CWorld_ServerCore::Object_HeapSize()
{
	MAUTOSTRIP(CWorld_ServerCore_Object_HeapSize, 0);
	return m_lspObjects.Len();
}

// #define SERVER_NO_GET_DESTROYED_OBJECT

void CWorld_ServerCore::Game_Register(int _iGame)
{
	m_iGame = _iGame;
}

void CWorld_ServerCore::Game_Unregister()
{
	m_iGame = -1;
}

int CWorld_ServerCore::Game_GetObjectIndex()
{
	MAUTOSTRIP(CWorld_ServerCore_Object_GetGameObject, 0);
	if(m_iGame >= 0)
		return m_iGame;
	return Selection_GetSingleTarget(WSERVER_GAMEOBJNAME);
}

CWObject_Game* CWorld_ServerCore::Game_GetObject()
{
	return (CWObject_Game *)ThisClass::Object_Get(Game_GetObjectIndex());
}

CWObject_CoreData* CWorld_ServerCore::Object_GetCD(int _iObj)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_GetCD, NULL);
	if (!m_lspObjects.ValidPos(_iObj)) return NULL;
	CWObject** lpObjects = reinterpret_cast<CWObject**>(m_lspObjects.GetBasePtr());
	CWObject_CoreData* pObj = lpObjects[_iObj];
#ifdef SERVER_NO_GET_DESTROYED_OBJECT
	if (pObj)
		return (pObj->IsDestroyed()) ? NULL : pObj;
	else
		return NULL;
#else
	return pObj;
#endif
}

CWObject* CWorld_ServerCore::Object_Get(int _iObj)
{
	//MSCOPESHORT(CWorld_ServerCore::Object_Get);
	MAUTOSTRIP(CWorld_ServerCore_Object_Get, NULL);
	if (!m_lspObjects.ValidPos(_iObj)) return NULL;
	CWObject** lpObjects = reinterpret_cast<CWObject**>(m_lspObjects.GetBasePtr());
	CWObject* pObj = lpObjects[_iObj];
#ifdef SERVER_NO_GET_DESTROYED_OBJECT
	if (pObj)
		return (pObj->IsDestroyed()) ? NULL : pObj;
	else
		return NULL;
#else
	return pObj;
#endif
}

CWObject* CWorld_ServerCore::Object_GetWithGUID(int _GUID)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_GetWithGUID, NULL);
	int iObj = m_spGUIDHash->GetIndex(_GUID);
	if (iObj && m_lspObjects.ValidPos(iObj))
	{
		CWObject** lpObjects = reinterpret_cast<CWObject**>(m_lspObjects.GetBasePtr());
		return lpObjects[iObj];
	}
	return NULL;
}

CWS_ClientObjInfo* CWorld_ServerCore::Object_GetPerClientData(int _iObj, int _iClient)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_GetPerClientData, NULL);
	CWServer_ClientInfo* pCI = Client_GetInfo(_iClient);
	if (!pCI) return NULL;
	if (!pCI->m_lObjInfo.ValidPos(_iObj)) return NULL;
	return &pCI->m_lObjInfo.GetBasePtr()[_iObj];
}

int CWorld_ServerCore::Object_GetGUID(int _iObj)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_GetGUID, 0);
	CWObject* pObj = ThisClass::Object_Get(_iObj);
	if (pObj)
		return pObj->m_GUID;
	else
		return 0;
}

int CWorld_ServerCore::Object_GetIndex(int _GUID)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_GetIndex, 0);
	return m_spGUIDHash->GetIndex(_GUID);
}

int CWorld_ServerCore::Object_SetDirty(int _iObj, int _Mask)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_SetDirty, 0);
	CWObject* pObj = ThisClass::Object_Get(_iObj);
	if (!pObj) return 0;

	TAP<spCWServer_ClientInfo> pspClientInfo = m_lspClientInfo;
	for (int i = 0; i < pspClientInfo.Len(); i++)
	{
		CWServer_ClientInfo* pCI = pspClientInfo[i];
		if (!pCI) continue;
		if (pCI->m_lObjInfo.ValidPos(_iObj))
			pCI->m_lObjInfo.GetBasePtr()[_iObj].m_DirtyMask |= _Mask;
	}
	return 1;
}

int CWorld_ServerCore::Object_SetDirtyTree(int _iObj, int _Mask)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_SetDirtyTree, 0);
	CWObject* pObj = ThisClass::Object_Get(_iObj);
	if (!pObj) return 0;

	ThisClass::Object_SetDirty(_iObj, _Mask);

	int iChild = pObj->GetFirstChild();
	while(iChild)
	{
		ThisClass::Object_SetDirtyTree(iChild, _Mask);
		iChild = Object_GetNextChild(iChild);
	}

	return 0;
}

// -------------------------------------------------------------------
bool CWorld_ServerCore::Object_SetPhysics_DoNotify(int _iObj, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Pos)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetPhysics_DoNotify, false);
	MSCOPESHORT(CWorld_PhysState::Object_SetPhysics_DoNotify);
	CWObject* pObj = ThisClass::Object_Get(_iObj);
	if (!pObj)
		return true;

	int NotifyFlags = pObj->m_IntersectNotifyFlags;

	TSelection<CSelection::MEDIUM_BUFFER> Selection1;
	TSelection<CSelection::MEDIUM_BUFFER> Selection2;

	bool bIntersect = Phys_IntersectWorld((CSelection*) NULL, _PhysState, _Pos, _Pos, _iObj, NULL, NotifyFlags, &Selection1, &Selection2);

	if (!bIntersect)
	{
		DEBUG_CHECK_MATRIX(_Pos);
		pObj->m_PhysState = _PhysState;
		pObj->m_LocalPos = _Pos;
		pObj->m_DirtyMask |= m_DirtyMask_InsertPosition;

		Phys_InsertPosition(_iObj, pObj);

		{
			const int16* pSel;
			int nSel = Selection_Get(Selection1, &pSel);
			for(int iSelObj = 0; iSelObj < nSel; iSelObj++)
				MessageQueue_SendToObject(CWObject_Message(OBJSYSMSG_NOTIFY_INTERSECTION, pSel[iSelObj]), _iObj);
		}

		{
			const int16* pSel;
			int nSel = Selection_Get(Selection2, &pSel);
			for(int iSelObj = 0; iSelObj < nSel; iSelObj++)
				MessageQueue_SendToObject(CWObject_Message(OBJSYSMSG_NOTIFY_INTERSECTION, _iObj), pSel[iSelObj]);
		}

	}

	return !bIntersect;
}

// -------------------------------------------------------------------
const char* CWorld_ServerCore::Object_GetName(int _iObj)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_GetName, NULL);
	CWObject* pObj = ThisClass::Object_Get(_iObj);
	if (pObj) 
		return pObj->m_Name.Str();
	else
		return NULL;
}


// -------------------------------------------------------------------
uint32 CWorld_ServerCore::Object_GetNameHash(uint _iObj)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_GetNameHash, 0);
	CWObject* pObj = ThisClass::Object_Get(_iObj);
	if (pObj && pObj->m_pNameNode) 
		return pObj->m_pNameNode->m_NameHash;
	else
		return 0;
}

// -------------------------------------------------------------------
void CWorld_ServerCore::Object_SetName(int _iObj, const char* _pName)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_SetName, MAUTOSTRIP_VOID);
	CWObject* pObj = Object_Get(_iObj);
	if (pObj)
	{

		CTargetNameElement Elem;
		Elem.m_NameHash = StringToHash(_pName);
		Elem.m_pObj = pObj;

		if (pObj->m_pNameNode && (pObj->m_pNameNode->m_NameHash == Elem.m_NameHash))
			return;

		if (pObj->m_pNameNode)
		{
//			LogFile(CStrF("Remove %.8x", pObj));
			m_NameSearchTree.Remove(pObj->m_pNameNode);
			pObj->m_pNameNode = NULL;
		}
#ifndef M_RTMCONSOLE
		pObj->m_Name = _pName;
#endif

		if (!_pName || !_pName[0])
			return;

		if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_DESTROYED)
			return;

//		LogFile(CStrF("Insert %.8x, %s", pObj, _pName));
		pObj->m_pNameNode = m_NameSearchTree.Insert(Elem);

/*		if (strlen(_pName) > 15) Error("Object_SetName", "Too long object name.");
		strcpy((char*)&pObj->m_Name.m_Name, _pName);
		pObj->m_Name.m_iObject = _iObj;
		if (pObj->m_Name.m_Name[0])
			pObj->m_pNameNode = &m_NameSearchTree.Insert(&pObj->m_Name);
*/
	}
}

void CWorld_ServerCore::Object_SetModel(int _iObj, int _iModel, int _ModelNr)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_SetModel, MAUTOSTRIP_VOID);
	if(_ModelNr < 0 || _ModelNr >= CWO_NUMMODELINDICES) return;
	CWObject* pObj = Object_Get(_iObj);
	if (pObj)
	{
		pObj->m_iModel[_ModelNr] = _iModel;
	}
}

void CWorld_ServerCore::Object_SetSound(int _iObj, int _iSound, int _SoundNr)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_SetSound, MAUTOSTRIP_VOID);
	if(_SoundNr < 0 || _SoundNr >= CWO_NUMSOUNDINDICES) return;
	CWObject* pObj = Object_Get(_iObj);
	if (pObj)
	{
		pObj->m_iSound[_SoundNr] = _iSound;
	}
}

// -------------------------------------------------------------------
//  Object hierarchy
// -------------------------------------------------------------------
bool CWorld_ServerCore::Object_AddChild(int _iParent, int _iChild)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_AddChild, false);
	// Makes iChild a child if iParent, if _iParent is zero, _iChild is unconnected from any parent it might have.
	// Returns false if:
	//		- the child cannot be placed, or 
	//		- Parent is a child of _iChild, or
	//		- Parent is not a valid object
	//		- Child is not a valid object
	//		- Child and parent are the same object.

	if(CWorld_PhysState::Object_AddChild(_iParent, _iChild))
	{
		Object_SetDirtyTree(_iParent, CWO_DIRTYMASK_HIERARCHY);
		return true;
	}
	else
		return false;
}

// -------------------------------------------------------------------
// Object creation
// -------------------------------------------------------------------
void CWorld_ServerCore::Object_EvalKey(int _iObj, const CStr _Key, const CStr _Value)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_EvalKey, MAUTOSTRIP_VOID);
	CWObject* pObj = m_lspObjects[_iObj];
	if (!pObj) Error("Object_EvalKey", "NULL Object.");

	CRegistry_Dynamic reg;
	CStr key = _Key;
	key.UpperCase();
	reg.SetThisKey(key, _Value);
	pObj->OnEvalKey(key.StrHash(), &reg);
}

int CWorld_ServerCore::Object_Insert(spCWObject _spObj, int _iObj)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_Insert, 0);
	// Allocate ID
	if(_iObj == -1 || !m_spObjectHeap->ForceAllocID(_iObj))
	{
		if(_iObj != -1)
			ConOutL(CStrF("§cf80WARNING: Failed to allocated specific objectindex %i, for Object %s %s", _iObj, _spObj->GetName(), _spObj->GetTemplateName()));
		_iObj = m_spObjectHeap->AllocID();
		if(_iObj == -1)
		{
			ConOutL("§cf80WARNING: Too many entities.");
			return -1;
		}
	}

	// Clear per-client data
	for(int i = 0; i < m_lspClientInfo.Len(); i++)
		if (m_lspClientInfo[i] != NULL)
			m_lspClientInfo[i]->m_lObjInfo[_iObj].Clear();

	m_lspObjects[_iObj] = _spObj;
	_spObj->m_pWServer = this;
	_spObj->m_iObject = _iObj;

	m_spGUIDHash->Insert(_iObj, _spObj->m_GUID);

	Object_SetPosition(_iObj, _spObj->GetPositionMatrix());
	_spObj->m_LastPos = _spObj->GetPositionMatrix();

	Object_SetDirty(_iObj, CWO_DIRTYMASK_COREMASK);
	Object_SetName(_iObj, (char *)_spObj->GetName());
	return _iObj;
}


int CWorld_ServerCore::Object_Create(spCWObject _spObj, const CMat4Dfp32 &_Pos, const CRegistry *_pKeys, int _iOwner, const aint* _pParam, int _nParam, int _iObj, uint32 _GUID)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_Create, 0);
	MSCOPE(CWorld_ServerCore::Object_Create, WORLD_SERVER);

	// Allocate ID
	if(_iObj == -1 || !m_spObjectHeap->ForceAllocID(_iObj))
	{
		if(_iObj != -1)
			ConOutL(CStrF("§cf80WARNING: Failed to allocate specific objectindex %i, for Object %s %s", _iObj, _spObj->GetName(), _spObj->GetTemplateName()));
		_iObj = m_spObjectHeap->AllocID();
		if(_iObj == -1)
		{
			ConOutL("§cf80WARNING: Too many entities.");
			return -1;
		}
	}

	if (_GUID == 0 || m_spGUIDHash->GetIndex(_GUID) != 0)
	{
		if (_GUID != 0)
			ConOutL(CStrF("§cf80WARNING: Failed to allocate specific GUID %08X for object %s %s", _GUID, _spObj->GetName(), _spObj->GetTemplateName()));

		_GUID = m_NextGUID++;
		int iCurr = m_spGUIDHash->GetIndex(_GUID);
		M_ASSERT(iCurr == 0, "GUID from guid-pool is already in use?!");
	}

	// Clear per-client data
	for(int i = 0; i < m_lspClientInfo.Len(); i++)
		if (m_lspClientInfo[i] != NULL)
			m_lspClientInfo[i]->m_lObjInfo[_iObj].Clear();

	VERBOSE_OUT("[SERVER] Object_Create, GUID: %08X, Index: %d, class %s, Name: '%s'\n", _GUID, _iObj, _spObj->MRTC_ClassName(), _pKeys ? _pKeys->GetValue("TARGETNAME").Str() : "?");
	m_lspObjects[_iObj] = _spObj;
	_spObj->m_pWServer = this;
	_spObj->m_iObject = _iObj;
	_spObj->m_iOwner = _iOwner;
//	_spObj->m_ClientFlags |= CWO_CLIENTFLAGS_NOHASH;

	_spObj->m_GUID = _GUID;
	m_spGUIDHash->Insert(_iObj, _spObj->m_GUID);

	Object_SetPosition(_iObj, _Pos);
	_spObj->m_LastPos = _Pos;


	// Run OnCreate & OnInitClientObjects
	M_TRY
	{
		_spObj->m_pRTC->m_pfnOnInitClientObjects(_spObj, this);
		_spObj->OnCreate();
	}
	M_CATCH(
	catch(CCException)
	{
		ConOutL(CStr("(CWorld_ServerCore::Object_Init) Exception in OnCreate(), class ") + _spObj->m_RuntimeClass.m_ClassName);
		m_spSpaceEnum->Remove(_iObj);
		m_spGUIDHash->Remove(_iObj);
		m_lspObjects[_iObj] = NULL;
		m_spObjectHeap->FreeID(_iObj);
		return -1;
	}
	)

	// Parse attributes
	if(_pKeys)
	{
		int nKeys = _pKeys->GetNumChildren();
		for(int k = 0; k < nKeys; k++)
		{
			const CRegistry* pKey = _pKeys->GetChild(k);
			uint32 KeyHash = pKey->GetThisNameHash();

			if (KeyHash != MHASH3('CLAS','SNAM','E'))
			{
				M_TRY
				{ 
					_spObj->OnEvalKey(KeyHash, pKey); 
				}
				M_CATCH(
				catch(CCException)
				{
					ConOutL(CStrF("(CWorld_ServerCore::Object_EvalRegistry) Exception in OnEvalKey(\"%s\", \"%s\"), class %s.", pKey->GetThisName().Str(), pKey->GetThisValue().Str(), _spObj->MRTC_ClassName()));
					m_spSpaceEnum->Remove(_iObj);
					m_spGUIDHash->Remove(_iObj);
					m_lspObjects[_iObj] = NULL;
					m_spObjectHeap->FreeID(_iObj);
					return -1;
				}
				)
			}
		}
	}

	if(m_lspObjects[_iObj] == NULL)
		return -1;

	{
		_spObj->OnInitInstance(_pParam, _nParam);
		if(m_lspObjects[_iObj] == NULL)
			return -1;
	}

	Object_EnableLinkage(_iObj);
//	_spObj->m_ClientFlags &= ~CWO_CLIENTFLAGS_NOHASH;
	{
		MSCOPE(CWorld_ServerCore::Object_Create::OnFinishEvalKeys, WORLD_SERVER);
		_spObj->OnFinishEvalKeys();
		if(m_lspObjects[_iObj] == NULL)
			return -1;
	}

	if (!(m_ServerMode & SERVER_MODE_SPAWNWORLD))
	{
		_spObj->OnSpawnWorld();
		if(m_lspObjects[_iObj] == NULL)
			return -1;

		_spObj->OnSpawnWorld2();
		if(m_lspObjects[_iObj] == NULL)
			return -1;
	}

	Object_SetDirty(_iObj, CWO_DIRTYMASK_COREMASK);
	// Replace with this line after Enclave GM
//	Object_SetDirty(_iObj, CWO_DIRTYMASK_COREMASK | _spObj->m_DirtyMask);
	
//	Object_InitRigidBody(_iObj);
	return _iObj;
}

void CWorld_ServerCore::Object_InitRigidBody(int _iObj, bool _bDebris) 
{
	spCWObject* lspObjects = m_lspObjects.GetBasePtr();
	CWObject* pObj = lspObjects[_iObj];
	M_ASSERT(pObj, "invalid argument");

	if (pObj->m_pPhysCluster)
	{
		uint nObj = pObj->m_pPhysCluster->m_lObjects.Len();
		CWPhys_ClusterObject *pPCO = pObj->m_pPhysCluster->m_lObjects.GetBasePtr();

		fp32 Density=0;
		bool bActive = (pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_PHYSMOVEMENT) != 0;
		if( bActive )
		{
			CVec3Dfp32 Tmp,Tmp2;

			fp32 Mass = pObj->GetMass();
			if( Mass == 0.0f ) 
			{
				CWD_DynamicsUtil::GetPhysicalProperties(m_spMapData, pObj, Mass, Tmp, Tmp2);
				Mass = Clamp(Mass * 0.25f,1.0f,_FP32_MAX); // -Hack to decrease mass, autogenerated is too high!
			}
			
			//Assume even distribution of mass
			fp32 Vol = 0.0f;
			for(uint i = 0;i < nObj;i++)
				Vol += CWPhys_Cluster::GetObjectVolume(pPCO[i].m_PhysPrim);

			Density = Mass / Vol;
		}

		for(uint i = 0;i < nObj;i++)
		{
			CWD_RigidBody2 * pRB = pPCO[i].m_pRB;
			if( !pRB )
			{
				pRB = DNew(CWD_RigidBody2) CWD_RigidBody2();
				pRB->m_pUserData = pObj;
				pRB->m_pUserData2 = &(pPCO[i]);
				pPCO[i].m_pRB = pRB;
			}
			pRB->m_bDebris = _bDebris;
			pRB->m_CenterOfMass = CVec3Dfp32(0,0,0);
			pRB->SetActive(bActive);

			if( bActive )
			{
					CVec3Dfp32 InertiaTensor, MassCenter;
					fp32 Mass;
					CWD_DynamicsUtil::GetPhysicalProperties(m_spMapData, pObj, Mass, MassCenter, InertiaTensor);
					Mass = Clamp(Mass * 0.25f, 1.0f, _FP32_MAX);  // TWEAK/HACK! the auto-generated mass is generally way too high...
					fp32 CustomMass = pObj->GetMass();
					fp32 Scale = CustomMass / Mass;
					InertiaTensor *= Scale;
					Mass = CustomMass;
					InertiaTensor = InertiaTensor * (1.0f / (32.0f * 32.0f));

				pRB->m_Mass = Density * CWPhys_Cluster::GetObjectVolume(pPCO[i].m_PhysPrim);

				pRB->m_InertiaTensor = CWPhys_Cluster::GetObjectInertia(pPCO[i].m_PhysPrim,pRB->m_Mass)
					* Sqr(1.0f/32.0f);
				
				Mass = 0.0f;
			}
			else
			{
				pRB->m_Mass = 1000000.0f;
				pRB->m_InertiaTensor = CWD_Inertia::Block(1000000.0f, 100.0f, 100.0f, 100.0f);
			}
		}
		m_ObjPoolDynamics.Insert(_iObj);
	}
	else if (pObj->GetPhysState().m_nPrim)
	{
		CWD_RigidBody2 *pRB2 = pObj->m_pRigidBody2;
		if (pRB2 == NULL)
		{
			pRB2 = DNew(CWD_RigidBody2) CWD_RigidBody2();
			pRB2->m_pUserData = pObj;
		}
		pRB2->m_bDebris = _bDebris;

		m_ObjPoolDynamics.Insert(_iObj);


//			Value of FooTensor could be used to compare the old and new tensor calculations.
//			CVec3Dfp64 FooTensor = CInertia::Block(mass, extent.k[0], extent.k[1],extent.k[2]);
		CVec3Dfp32 InertiaTensor, MassCenter;
		fp32 Mass;
		CWD_DynamicsUtil::GetPhysicalProperties(m_spMapData, pObj, Mass, MassCenter, InertiaTensor);

		// TWEAK/HACK! the auto-generated mass is generally way too high...
		{
			fp32 AdjustedMass = Clamp(Mass * 0.25f, 1.0f, _FP32_MAX);  
			InertiaTensor *= AdjustedMass / Mass;
			Mass = AdjustedMass;
		}

		const CWO_PhysicsAttrib& PhysAttr = pObj->m_PhysAttrib;
#if 0
		fp32 StaticFriction = PhysAttr.m_Friction;
		fp32 DynamicsFriction = StaticFriction * 0.75;
		pRB->SetStaticFriction(StaticFriction);
		pRB->SetDynamicFriction(DynamicsFriction);
		pRB->SetCoefficientOfRestitution(PhysAttr.m_Elasticy);
#endif

#if 0
		pRB2->m_StaticFriction = PhysAttr.m_Friction;
		pRB2->m_DynamicFriction = PhysAttr.m_Friction * 0.75f;
		pRB2->m_CoefficientOfRestitution = PhysAttr.m_Elasticy;
#endif

		fp32 CustomMass = pObj->GetMass();
		if (CustomMass > 0.0f)
		{
			InertiaTensor *= CustomMass / Mass;
			Mass = CustomMass;
		}

		pObj->SetMass(Mass);
//		pRB->m_MassCenter = MassCenter.Getfp64();
		pRB2->m_CenterOfMass = MassCenter;

		bool bActive = (pObj->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_PHYSMOVEMENT) != 0;
//		pRB->SetActive(bActive);
		pRB2->SetActive(bActive);

		if (bActive)
		{
//			pRB->SetMass(Mass);
			//m_DynamicsWorld.SetInertiaTensor(pRB, InertiaTensor.Getfp64());

			pRB2->m_Mass = Mass;
			pRB2->m_InertiaTensor = InertiaTensor * (1.0f / (32.0f * 32.0f));
			M_ASSERT(M_Fabs(InertiaTensor.k[0] * InertiaTensor.k[1] * InertiaTensor.k[2]) > 0.0f, "invalid tensor!");
		}
		else // !bActive
		{
//			pRB->SetMass(1000000);
//			pRB->SetInertiaTensor(CInertia::Sphere(1000000,1));

			pRB2->m_Mass = 1000000.0f;
			pRB2->m_InertiaTensor = CWD_Inertia::Block(1000000.0f, 100.0f, 100.0f, 100.0f);
		}

		{
			// Set position
			const CMat4Dfp32& mat = pObj->GetPositionMatrix();
			CVec3Dfp32 pos = MassCenter * mat;
//			pRB->SetPosition(pos.Getfp64());
			CQuatfp32 orient;
			orient.Create(mat);
			orient.Inverse();
//			pRB->SetOrientaion(ConvertQuat(orient));

			// Set velocity
			fp32 TicksPerSec = GetGameTicksPerSecond();
			const CVelocityfp32& Velo = pObj->GetVelocity();
//			pRB->SetVelocity( (Velo.m_Move * ((1.0f / 32.0f) * TicksPerSec)).Get<fp64>() ); // [m/s]
//			pRB->SetAngularVelocity( (Velo.m_Rot.m_Axis * (Velo.m_Rot.m_Angle * _PI2 * TicksPerSec)).Get<fp64>() ); // [rad/s]
		}

	#ifndef WSERVER_DYNAMICSENGINE2
		if (pObj->m_pRigidBody == NULL)
		{
			pObj->m_pRigidBody = pRB;
			m_DynamicsWorld.AddRigidBody(pRB);
		}
	#endif

		if (pObj->m_pRigidBody2 == NULL)
		{
			pObj->m_pRigidBody2 = pRB2;
		}
	}
	else
	{
		m_ObjPoolDynamics.Remove(_iObj);
		if (pObj->m_pRigidBody2)
			pObj->m_pRigidBody2->SetActive(false);
		if (pObj->m_pPhysCluster)
		{
			CWPhys_ClusterObject * pPCO = pObj->m_pPhysCluster->m_lObjects.GetBasePtr();
			for(int i = 0;i < pObj->m_pPhysCluster->m_lObjects.Len();i++)
			{
				if( pPCO[i].m_pRB ) pPCO[i].m_pRB->SetActive(false);
			}
		}
	}
}

int CWorld_ServerCore::Object_Create(const CRegistry &_Keys, const CMat4Dfp32 *_pPos, int _iOwner, const aint* _pParam, int _nParam, int _iObj, uint32 _GUID)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_Create_2, 0);

	if (_Keys.GetValuei("EVALED", 0))
	{
		// The registry is preevaled

		const CRegistry *pTemplates = _Keys.FindChild("TEMPLATES");
		if (pTemplates && pTemplates->GetNumChildren())
		{
			CStr ClassName = _Keys.GetThisValue();

			int nObj = pTemplates->GetNumChildren();
			TArray<const CRegistry *> lspObjects;
			memset(lspObjects.GetBasePtr(), 0, lspObjects.ListSize());
			lspObjects.SetLen(nObj);

			for (int i = 0; i < nObj; ++i)
			{
				const CRegistry *pChild = pTemplates->GetChild(i);
				int iOrder = pChild->GetValuei("ORDER", i);
				lspObjects[iOrder] = pChild;
			}

			for (int i = 0; i < nObj; ++i)
			{
				M_ASSERT(lspObjects[i], "");
			}

			int iLast = -1;
			for (int i = nObj-1; i >= 0; --i)
			{
				CMat4Dfp32 Mat;
				const CRegistry *pMat = _Keys.FindChild("MAT");
				if (pMat)
				{
					M_ASSERT(pMat->Anim_ThisGetNumSeq() == 1, "Must be");
					M_ASSERT(pMat->Anim_ThisGetNumKF() >= nObj, "Must be");
					pMat->Anim_ThisGetKFValueaf(0, i, 16, (fp32 *)Mat.k);
				}
				else
					Mat.Unit();

				const CRegistry *pSubObj = lspObjects[i];
				const CRegistry *pClassName = pSubObj->FindChild("CLASSNAME");
				if (pClassName)
				{
					spCWObject spObj = m_spMapData->CreateObject(pClassName->GetThisValue());
					if (spObj)
					{
						CMat4Dfp32 DestMat;
						if (_pPos)
							Mat.Multiply(*_pPos, DestMat);
						else 
							DestMat = Mat;

						int iObj = Object_Create(spObj, DestMat, pSubObj, _iOwner, _pParam, _nParam, _iObj, _GUID);
						if (iObj > 0)
							iLast = iObj;
						if (iObj > 0 && _pPos)
						{
							CWObject *pObj = Object_Get(iObj);
							if(pObj)
								pObj->OnTransform(*_pPos);
						}

					}
				}
			}

			CWObject* pObj = Object_Get(iLast);
			if(pObj)
				pObj->SetTemplateName(ClassName);
		}

		return -1;
	}
	else
	{
		const CRegistry *pClassName = _Keys.FindChild("CLASSNAME");
		if(!pClassName)
		{
			//_Keys.XRG_Write("c:\\test.xrg");
			return -1;
		}

		bint bPreEval = _Keys.FindChild("ORDER") != NULL;

		MSCOPE_STR(pClassName->GetThisValue(), WORLD_SERVER);

		spCWObject spObj = m_spMapData->CreateObject(pClassName->GetThisValue());
		if(!spObj)
		{
			if(!m_spTemplateReg || bPreEval)
				return -1;

			TArray<spCRegistry> lspObjects;
			Entity_EvalTemplate_r(m_spTemplateReg, &_Keys, lspObjects);

			// Delete all objects that have the same class as the original object.
			int iLast = -1;
			for(int i = lspObjects.Len()-1; i >= 0; i--)
				if (lspObjects[i]->FindChild("CLASSNAME", pClassName->GetThisValue())) 
					lspObjects.Del(i);
				else
				{
					int iObj = Object_Create(*lspObjects[i], NULL, _iOwner, _pParam, _nParam, _iObj);
					if(iObj > 0)
						iLast = iObj;
				}

			CWObject* pObj = Object_Get(iLast);
			if(pObj)
				pObj->SetTemplateName(pClassName->GetThisValue());

			return iLast;
		}
		else
		{
			if(_pPos && _Keys.GetNumChildren() && !bPreEval)
			{
				spCRegistry spKeys = REGISTRY_CREATE;
				if(!spKeys)
					MemError("Object_Create");
				spKeys = _Keys.Duplicate();
				Entity_Transform_r(spKeys, *_pPos);

				CMat4Dfp32 Pos = Entity_GetPosition(spKeys);

				int iObj = Object_Create(spObj, Pos, spKeys, _iOwner, _pParam, _nParam, _iObj, _GUID);
				if(iObj > 0)
				{
					CWObject *pObj = Object_Get(iObj);
					if(pObj)
						pObj->OnTransform(*_pPos);
				}
				return iObj;
			}
		}

		if(!spObj) 
		{
			LogFile("(CWorld_ServerCore::Object_Create) Failed to create object: " + pClassName->GetThisValue());
			return -1;
		}

		if(_pPos)
			return Object_Create(spObj, *_pPos, &_Keys, _iOwner, _pParam, _nParam, _iObj, _GUID);
		else
			return Object_Create(spObj, spObj->m_Pos, &_Keys, _iOwner, _pParam, _nParam, _iObj, _GUID);
	}
}


int CWorld_ServerCore::Object_Create(const char* _pClassName, const CMat4Dfp32& _Pos, int _iOwner, const aint* _pParam, int _nParam, int _iObj, uint32 _GUID)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_Create_3, 0);
	MSCOPE(CWorld_ServerCore::Object_Create, WORLD_SERVER);

	spCWObject spObj = m_spMapData->CreateObject(_pClassName);
	if(!spObj)
	{
		// Use preevaluated templates

		const TThinArray<CMat4Dfp32> *plMat;
		const TThinArray<spCRegistry> *plspObjects = m_spMapData->GetObjectTemplates(_pClassName, &plMat);

		if(!plspObjects || !plspObjects->Len())
		{
			ConOutL(CStrF("§cf80WARNING: (CWorld_ServerCore::Object_Create) Could not find a class or template named %s.", (char*)_pClassName));
			return -1;
		}

		int iLast = -1;
		for(int i = 0; i < plspObjects->Len(); i++)
		{
			CMat4Dfp32 Res;
			(*plMat)[i].Multiply(_Pos, Res);
			int iObj = Object_Create(*(*plspObjects)[i], &Res, _iOwner, _pParam, _nParam, _iObj, _GUID);
			if(iObj > 0)
				iLast = iObj;
		}

		CWObject* pObj = Object_Get(iLast);
		if (pObj)
			pObj->SetTemplateName(_pClassName);
		

		return iLast;
	}

	return Object_Create(spObj, _Pos, NULL, _iOwner, _pParam, _nParam, _iObj, _GUID);
}


int CWorld_ServerCore::Object_Create(const char* _pClassName, CVec3Dfp32 _Pos, int _iOwner, const aint* _pParam, int _nParam, int _iObj, uint32 _GUID)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_Create_4, 0);
	CMat4Dfp32 Mat;
	Mat.Unit();
	_Pos.SetMatrixRow(Mat, 3);
	return Object_Create(_pClassName, Mat, _iOwner, _pParam, _nParam, _iObj, _GUID);
}


bool CWorld_ServerCore::Object_ChangeGUID(int _iObj, int _NewGUID)
{
	CWObject* pObj = m_lspObjects[_iObj];
	if(!pObj)
		return false;

	if (pObj->m_GUID != _NewGUID)
	{
		int Index = m_spGUIDHash->GetIndex(_NewGUID);
		if (Index != 0 && _iObj != Index)
		{
			ConOutL(CStrF("§cf80WARNING: Failed to allocated specific GUID %i, for Object %s %s", _NewGUID, pObj->GetName(), pObj->GetTemplateName()));
			return false;
		}

		m_spGUIDHash->Remove(_iObj);
		pObj->m_GUID = _NewGUID;
		m_spGUIDHash->Insert(_iObj, _NewGUID);
	}
	return true;
}

void CWorld_ServerCore::Object_DestroyDeferred_r(int _iObj)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_DestroyDeferred_r, MAUTOSTRIP_VOID);
//	ConOutL(CStrF("   Object_DestroyDeferred_r, Tick %d, object %d, %d deferred", GetGameTick(), _iObj ));

	CWObject* pObj = m_lspObjects[_iObj];
	if (pObj)
	{
		Object_SetName(_iObj, "");

		// Dynamics
	#ifndef WSERVER_DYNAMICSENGINE2
		if (pObj->m_pRigidBody)
		{
			VERBOSE_OUT("Object_DestroyDeferred_r, iObj = %d, pObj = 0x%08X\n", _iObj, pObj);
			m_DynamicsWorld.RemoveRigidBody(pObj->m_pRigidBody);
			((CWD_RigidBody*)pObj->m_pRigidBody)->m_pCoreData = NULL;
			delete pObj->m_pRigidBody;
			pObj->m_pRigidBody = NULL;
		}
	#endif

		if (pObj->m_pRigidBody2)
		{
			m_ObjPoolDynamics.Remove(_iObj);
			delete pObj->m_pRigidBody2;
			pObj->m_pRigidBody2 = NULL;
		}
		if (pObj->m_pPhysCluster)
		{
			m_ObjPoolDynamics.Remove(_iObj);
			CWPhys_ClusterObject * pPO = pObj->m_pPhysCluster->m_lObjects.GetBasePtr();
			for(int i = 0;i < pObj->m_pPhysCluster->m_lObjects.Len();i++)
			{
				if( pPO[i].m_pRB ) delete pPO[i].m_pRB;
				pPO[i].m_pRB = NULL;
			}
		}

		// Just in case something got the object linked again, remove it.
		Object_SetDirty(_iObj, CWO_DIRTYMASK_COREMASK);
		m_spSpaceEnum->Remove(_iObj);
		m_spGUIDHash->Remove(_iObj);

		m_lspObjects[_iObj] = NULL;
		m_spObjectHeap->FreeID(_iObj);
	}
	else
		Error("Object_DestroyDeferred_r", "NULL object.");
}

void CWorld_ServerCore::Object_CommitDeferredDestruction()
{
	MAUTOSTRIP(CWorld_ServerCore_Object_CommitDeferredDestruction, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_ServerCore::Object_CommitDeferredDestruction, WORLD_SERVER);
	// ConOutL(CStrF("Object_CommitDeferredDestruction, Tick %d", GetGameTick()));

	m_ServerMode |= SERVER_MODE_COMMITDEFERRED;

	for(int i = 0; i < m_liObjectDeferredDestroy.Len(); i++)
		Object_DestroyDeferred_r(m_liObjectDeferredDestroy[i]);

	m_ServerMode &= ~SERVER_MODE_COMMITDEFERRED;

	m_liObjectDeferredDestroy.QuickSetLen(0);
}

void CWorld_ServerCore::Object_Destroy(int _iObj, bool _bForceSafeDestroy)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_Destroy, MAUTOSTRIP_VOID);
//	Object_DestroyInstant(_iObj);

	// Safe destruction:
	// * Doesn't delete object until end of game tick. 
	// * Use this if you destroy the object from the object itself.
	// * Hierarchy is preserved.
	// * OnDestroy is executed
	// * Targetname removed
	// * Removed from GUID and spaceenum hashing.
	// * Object_Get() on object returns NULL after Object_Destroy()

	if (((m_ServerMode & SERVER_MODE_COMMITDEFERRED) ||
		!(m_ServerMode & SERVER_MODE_SIMULATE)) && !_bForceSafeDestroy)
	{
//		ConOutL(CStrF("CHAINED DESTROY!  Object_Destroy, Tick %d, object %d, %d deferred", GetGameTick(), _iObj, m_liObjectDeferredDestroy.Len() ));
		Object_DestroyInstant(_iObj);
		return;
	}

	CWObject* pObj = Object_Get(_iObj);
	if (pObj)
	{
		if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_DESTROYED) return;
		pObj->m_ClientFlags |= CWO_CLIENTFLAGS_DESTROYED;

		{
			// Send event to listeners
			TSelection<CSelection::MEDIUM_BUFFER> Sel;
			Selection_AddListeners(Sel, _iObj, CWO_LISTENER_EVENT_DELETED);
			CWObject_Message Msg(OBJSYSMSG_LISTENER_EVENT, CWO_LISTENER_EVENT_DELETED, 0, _iObj);
			ThisClass::Message_SendToSelection(Msg, Sel);

			// Remove from listener context
			m_Listeners.RemoveElement(_iObj);
		}

		// Destroy children
		int iChild = pObj->GetFirstChild();
		while(iChild)
		{
			int iNext = Object_GetNextChild(iChild);
			Object_Destroy(iChild);
			iChild = iNext;
		}

		Object_AddChild(0, pObj->m_iObject);
		pObj->OnDestroy();
		Object_SetDirty(_iObj, CWO_DIRTYMASK_COREMASK);
		Object_SetName(_iObj, "");
		if (m_spSceneGraphInstance != NULL)
			m_spSceneGraphInstance->SceneGraph_UnlinkElement(_iObj);
		m_spSpaceEnum->Remove(_iObj);
		m_spGUIDHash->Remove(_iObj);

		m_liObjectDeferredDestroy.Add(_iObj);

/*		if (m_ServerMode & SERVER_MODE_SIMULATE)
			ConOutL(CStrF("   Object_Destroy, Tick %d, object %d, %d deferred", GetGameTick(), _iObj, m_liObjectDeferredDestroy.Len() ));
		else
			ConOutL(CStrF("   NOT SIMULATING!  Object_Destroy, Tick %d, object %d, %d deferred", GetGameTick(), _iObj, m_liObjectDeferredDestroy.Len() ));*/
	}
}

void CWorld_ServerCore::Object_DestroyInstant(int _iObj)
{
	MAUTOSTRIP(CWorld_ServerCore_Object_DestroyInstant, MAUTOSTRIP_VOID);
	CWObject* pObj = Object_Get(_iObj);
	if (pObj)
	{
		if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_DESTROYED) return;
		pObj->m_ClientFlags |= CWO_CLIENTFLAGS_DESTROYED;

		{
			// Send event to listeners
			TSelection<CSelection::MEDIUM_BUFFER> Sel;
			Selection_AddListeners(Sel, _iObj, CWO_LISTENER_EVENT_DELETED);
			CWObject_Message Msg(OBJSYSMSG_LISTENER_EVENT, CWO_LISTENER_EVENT_DELETED, 0, _iObj);
			ThisClass::Message_SendToSelection(Msg, Sel);

			// Remove from listener context
			m_Listeners.RemoveElement(_iObj);
		}

		// Destroy rigid body
	#ifndef WSERVER_DYNAMICSENGINE2
		if (pObj->m_pRigidBody)
		{
			VERBOSE_OUT("Object_DestroyInstant, iObj = %d, pObj = 0x%08X\n", _iObj, pObj);
			m_DynamicsWorld.RemoveRigidBody(pObj->m_pRigidBody);
			((CWD_RigidBody*)pObj->m_pRigidBody)->m_pCoreData = NULL;
			delete pObj->m_pRigidBody;
			pObj->m_pRigidBody = NULL;
		}
	#endif

		if (pObj->m_pRigidBody2)
		{
			m_ObjPoolDynamics.Remove(_iObj);
			delete pObj->m_pRigidBody2;
			pObj->m_pRigidBody2 = NULL;
		}
		if (pObj->m_pPhysCluster)
		{
			m_ObjPoolDynamics.Remove(_iObj);
			CWPhys_ClusterObject *pPO = pObj->m_pPhysCluster->m_lObjects.GetBasePtr();
			for(int i = 0;i < pObj->m_pPhysCluster->m_lObjects.Len();i++)
			{
				if( pPO[i].m_pRB ) delete pPO[i].m_pRB;
				pPO[i].m_pRB = NULL;
			}
		}

		// Destroy children
		int iChild = pObj->GetFirstChild();
		while(iChild)
		{
			int iNext = Object_GetNextChild(iChild);
			Object_DestroyInstant(iChild);
			iChild = iNext;
		}

		Object_AddChild(0, pObj->m_iObject);
		pObj->OnDestroy();
		Object_SetDirty(_iObj, CWO_DIRTYMASK_COREMASK);
		Object_SetName(_iObj, "");
		if (m_spSceneGraphInstance != NULL)
			m_spSceneGraphInstance->SceneGraph_UnlinkElement(_iObj);
		m_spSpaceEnum->Remove(_iObj);
		m_spGUIDHash->Remove(_iObj);

		m_lspObjects[_iObj] = NULL;
		m_spObjectHeap->FreeID(_iObj);

//		ConOut(CStrF("   Deleted object %d, Free objects %d", _iObj, m_spObjectHeap->MaxAvail() ));
	}

}
