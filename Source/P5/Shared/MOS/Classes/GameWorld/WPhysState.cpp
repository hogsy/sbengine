#include "PCH.h"
#include "WPhysState.h"
#include "WPhysState_Hash.h"
#include "MFloat.h"
#include "WBlockNavGrid.h"
#include "../../XRModels/Model_BSP2/WBSP2Model.h"  // needed for  Phys_CollideBSP2

#include "WObjects/WObj_PhysCluster.h"

#include "../Render/MRenderCapture.h"

#if 0
static void RenormalizeMatrix(CMat4Dfp32& _Dest)
{
	MAUTOSTRIP(RenormalizeMatrix, MAUTOSTRIP_VOID);
	CVec3Dfp32::GetMatrixRow(_Dest, 0).Normalize();
	CVec3Dfp32::GetMatrixRow(_Dest, 2) = -(CVec3Dfp32::GetMatrixRow(_Dest, 1) / CVec3Dfp32::GetMatrixRow(_Dest, 0)).Normalize();
	CVec3Dfp32::GetMatrixRow(_Dest, 1) = CVec3Dfp32::GetMatrixRow(_Dest, 2) / CVec3Dfp32::GetMatrixRow(_Dest, 0);
}
#endif

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

//#pragma optimize( "", off )
//#pragma inline_depth(0)

// -------------------------------------------------------------------
//  CWorld_PhysState
// -------------------------------------------------------------------
CWorld_PhysState::CWorld_PhysState()
{
	MAUTOSTRIP(CWorld_PhysState_ctor, MAUTOSTRIP_VOID);
	m_iObject_Worldspawn = 0;
	m_iObject_DisabledLinkage = 0;
	m_pSceneGraph = NULL;
	m_pBlockNavGrid = NULL;
	m_pNavGraph = NULL;
	m_DirtyMask_InsertPosition = CWO_DIRTYMASK_POS;
	m_MovePhysRecurse = 0;
	m_Unit.Unit();
	m_MTState = 0;

	// Init time variables
	MACRO_GetSystemEnvironment(pEnv);
	m_SimulationTime.Reset();
	m_SimulationTick = 0;
	m_RefreshRate = pEnv->GetValuef("SERVER_REFRESHRATE", 30.0f);
	m_TickTime = 1.0f / m_RefreshRate;
	m_TimeScale = 1.0f;
	m_TickRealTime = m_TickTime / m_TimeScale;
}

CWorld_PhysState::~CWorld_PhysState()
{
	MAUTOSTRIP(CWorld_PhysState_dtor, MAUTOSTRIP_VOID);
}

//
extern CVec3Dfp32 g_DEBUG_Position;

////////////////////////////////////////////////////////////////////////
void CWorld_PhysState::World_CommitDeferredSceneGraphLinkage()
{
	MSCOPESHORT(CWorld_PhysState::World_CommitDeferredSceneGraphLinkage);
	M_NAMEDEVENT("DeferredSG", 0xff000000);

	if (!m_spSceneGraphInstance)
		return;

	// Start
	const CIndexPool16::CHashIDInfo* pLinks = m_SceneGraphDeferredLinkage.GetLinks();
	int iLink = m_SceneGraphDeferredLinkage.GetFirst();

	TProfileDef(Time);
	{
		TMeasureProfile(Time);

		int nLink = 0;
		int nLinkInf = 0;
		int nUnlink = 0;
		for(; iLink != -1; iLink = pLinks[iLink].m_iNext)
		{
			int iObj = pLinks[iLink].m_iElem;

			CWObject_CoreData* pObj = Object_GetCD(iObj);

			// insert the object so we can update it when the current pathfinding is done
			if(m_spBlockNavSearcher)
				m_spBlockNavSearcher->RegisterObjectMovement(iObj);

			if (!pObj || pObj->m_ClientFlags & CWO_CLIENTFLAGS_INVISIBLE)
			{
				m_spSceneGraphInstance->SceneGraph_UnlinkElement(iObj);
				nUnlink++;
			}
			else
			{
				int SGFlags = 0;
				if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_SHADOWCASTER)
					SGFlags |= CXR_SCENEGRAPH_SHADOWCASTER;

				if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_LINKINFINITE)
				{
					m_spSceneGraphInstance->SceneGraph_LinkInfiniteElement(iObj, SGFlags);
					nLinkInf++;
				}
				else
				{
					CBox3Dfp32 VisBox;
					pObj->GetAbsVisBoundBox(VisBox);
					const CBox3Dfp32& PhysBound = pObj->m_PhysAbsBoundBox;
					VisBox.Expand(PhysBound);

					m_spSceneGraphInstance->SceneGraph_LinkElement(iObj, VisBox, SGFlags);
					nLink++;

					if ((m_pBlockNavGrid) &&
						(pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_NAVIGATION))
					{
						m_pBlockNavGrid->InvalidateBox(PhysBound);			
					}
				}
			}
		}
	}

//Removed semi-spamming line... /AndersO
//	if (Time > fp32(GetCPUFrequency()) * 0.0002f)
//		ConOut(CStrF("(CWorld_PhysState::World_CommitDeferredSceneGraphLinkage) Time %.2f ms (%d,%d,%d)", 1000.0f * fp32(Time) / GetCPUFrequency(), nLink, nLinkInf, nUnlink ));

	m_SceneGraphDeferredLinkage.RemoveAll();

	m_spSceneGraphInstance->SceneGraph_CommitDeferred();
}

void CWorld_PhysState::SetMultiThreadState(uint _MTState)
{
	m_MTState = _MTState;
}

CXR_SceneGraphInterface* CWorld_PhysState::World_GetSceneGraph()
{
	if (!m_pSceneGraph)
		Error("World_GetSceneGraph", "Not created.");

	return m_pSceneGraph;
}

CXR_SceneGraphInstance* CWorld_PhysState::World_GetSceneGraphInstance()
{
	return m_spSceneGraphInstance;
}

void CWorld_PhysState::Phys_SetDirtyMask(int _Mask)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_SetDirtyMask, MAUTOSTRIP_VOID);
	m_DirtyMask_InsertPosition = _Mask;
}

int CWorld_PhysState::Phys_GetDirtyMask()
{
	MAUTOSTRIP(CWorld_PhysState_Phys_GetDirtyMask, 0);
	return m_DirtyMask_InsertPosition;
}

void CWorld_PhysState::Phys_GetMinMaxBox(const CWO_PhysicsState& _PhysState, const CWO_PhysicsPrim& _PhysPrim, const CMat4Dfp32& _Pos, CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_GetMinMaxBox, MAUTOSTRIP_VOID);
	const CMat4Dfp32* pPos = &_Pos;
	CMat4Dfp32 PosNew;

	if (!(_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION))
	{
		PosNew.Unit();
		CVec3Dfp32::GetMatrixRow(PosNew, 3) = CVec3Dfp32::GetMatrixRow(*pPos, 3);
		pPos = &PosNew;
	}
	
	// Add offset to primitive 1?
	if (_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_OFFSET)
	{
		PosNew = *pPos;
		CVec3Dfp32 WOffs = _PhysPrim.GetOffset();
		if (_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION)
			WOffs.MultiplyMatrix3x3(*pPos);
		CVec3Dfp32::GetMatrixRow(PosNew, 3) += WOffs;
		pPos = &PosNew;
	}

	switch(_PhysPrim.m_PrimType)
	{
	case OBJECT_PRIMTYPE_PHYSMODEL :
		{
			CXR_Model* pModel = m_spMapData->GetResource_Model(_PhysPrim.m_iPhysModel);
			if (!pModel)
			{
				_Box.m_Min = CVec3Dfp32::GetMatrixRow(*pPos, 3);
				_Box.m_Max = CVec3Dfp32::GetMatrixRow(*pPos, 3);
				return;
			}

			if (_PhysPrim.m_PhysModelMask != 0xffff)
			{
				pModel->GetBound_Box(_Box, _PhysPrim.m_PhysModelMask);
				_Box.Transform(*pPos, _Box);
				return;
			}

				//Error("Phys_GetMinMaxBox", CStrF("NULL physics model. (%d)", _PhysPrim.m_iPhysModel));
			CXR_PhysicsModel* pPhys = pModel->Phys_GetInterface();
			if (pPhys)
				pPhys->Phys_GetBound_Box(*pPos, _Box);
			else
			{
				_Box.m_Min = CVec3Dfp32::GetMatrixRow(*pPos, 3);
				_Box.m_Max = CVec3Dfp32::GetMatrixRow(*pPos, 3);
			}
			return;
		}
	case OBJECT_PRIMTYPE_SPHERE :
		{
			fp32 r = _PhysPrim.GetRadius() + 0.0001f;
		r += 16.0f;
			_Box.m_Min = CVec3Dfp32::GetMatrixRow(*pPos, 3);
			_Box.m_Max = CVec3Dfp32::GetMatrixRow(*pPos, 3);
			for(int i = 0; i < 3; i++)
			{
				_Box.m_Min.k[i] -= r;
				_Box.m_Max.k[i] += r;
			}
			return;
		}
/*	case OBJECT_PRIMTYPE_CYLINDER_Z :
		{
			Error("Phys_GetMinMaxBox", "No implemented. (OBJECT_PRIMTYPE_CYLINDER_Z)");
			return;
		}*/
	case OBJECT_PRIMTYPE_BOX :
		{
			CPhysOBB OBB;
			OBB.SetDimensions(_PhysPrim.GetDim());
			OBB.SetPosition(*pPos);
			OBB.GetBoundBox(_Box);
			return;
		}

	case OBJECT_PRIMTYPE_POINT :
	case OBJECT_PRIMTYPE_NONE :
		{
			_Box.m_Min = CVec3Dfp32::GetMatrixRow(*pPos, 3);
			_Box.m_Min -= CVec3Dfp32(16);
			_Box.m_Max = CVec3Dfp32::GetMatrixRow(*pPos, 3);
			_Box.m_Max += CVec3Dfp32(16);
			return;
		}
	}
	Error("Phys_GetMinMaxBox", "Can't be here.");
}

void CWorld_PhysState::Phys_GetMinMaxBox(const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Pos, CBox3Dfp32& _Box)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_GetMinMaxBox_2, MAUTOSTRIP_VOID);
	_Box.m_Min = _FP32_MAX;
	_Box.m_Max = -_FP32_MAX;
	for(int iPrim = 0; iPrim < _PhysState.m_nPrim; iPrim++)
	{
		CBox3Dfp32 Box;
		Phys_GetMinMaxBox(_PhysState, _PhysState.m_Prim[iPrim], _Pos, Box);
		_Box.Expand(Box);
	}

	if (_Box.m_Min.k[0] > _Box.m_Max.k[0])
	{
		_Box.m_Min = _Pos.GetRow(3);
		_Box.m_Max = _Pos.GetRow(3);
		_Box.m_Min -= 8.0f;
		_Box.m_Max += 8.0f;
	}
}

void CWorld_PhysState::Phys_UpdateTree_r(int _iObj, CWObject_CoreData* _pObj)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_UpdateTree_r, MAUTOSTRIP_VOID);
	if (_pObj->m_ClientFlags & CWO_CLIENTFLAGS_RECURSED)
	{
		ConOutL(CStrF("§cf80WARNING: (CWorld_PhysState::Phys_UpdateTree_r) Recursion error at object %d", _iObj));
		return;
	}
	_pObj->m_ClientFlags |= CWO_CLIENTFLAGS_RECURSED;

	const int MaxRecurse = 128;
#ifndef M_RTM
	int lChilds[18];
#endif
	int iRecurs = 0;
	int iChild = _pObj->GetFirstChild();
	while(iChild)
	{
		CWObject_CoreData* pChild = Object_GetCD(iChild);
		if (!pChild) break;
//		if (!pChild) Error("Phys_UpdateTree_r", CStrF("Invalid child object %d", iChild));

//		pChild->m_LocalPos.Multiply(_pObj->GetPositionMatrix(), pChild->m_Pos);
		if (pChild->GetParent())
		{
			const CWObject_CoreData* pParent = Object_GetCD(pChild->GetParent());
			if(pParent)
			{
				if (pChild->m_ClientFlags & CWO_CLIENTFLAGS_NOROTINHERITANCE)
				{
					pChild->m_Pos = pChild->m_LocalPos;
					pParent->GetPosition().AddMatrixRow(pChild->m_Pos, 3);
				}
				else
				{
					pChild->m_LocalPos.Multiply(pParent->GetPositionMatrix() , pChild->m_Pos);
				}
				
			}
			else
			{
				ConOutL(CStrF("(CWorld_PhysState::Phys_UpdateTree_r) Could not access parent %d", _iObj));
				return;
			}
		}
		else
			pChild->m_Pos = pChild->m_LocalPos;

		Phys_GetMinMaxBox(pChild->m_PhysState, pChild->GetPositionMatrix(), pChild->m_PhysAbsBoundBox);
		if (m_spSpaceEnum != NULL)
			m_spSpaceEnum->Insert(pChild);

		if (m_spSceneGraphInstance != NULL)
			m_SceneGraphDeferredLinkage.Insert(iChild);

		Object_NotifyListeners(iChild, CWO_LISTENER_EVENT_MOVED);

		if (pChild->GetFirstChild())
			Phys_UpdateTree_r(iChild, pChild);

//		int iOldChild = iChild;
		iChild = pChild->GetNextChild();

#ifndef M_RTM
		if(iRecurs >= (MaxRecurse-16))
			lChilds[iRecurs - (MaxRecurse-16)] = iChild;
#endif
		if(iRecurs++ >= MaxRecurse)
		{
			ConOutD(CStrF("(CWorld_PhysState::Phys_UpdateTree_r) More than %d recursions! (%d)", MaxRecurse, iChild));

/*			MACRO_GetRegisterObject(CConsole, pCon, "SYSTEM.CONSOLE");
			if(pCon)
				pCon->ExecuteString("wdump(-1)");
			
			ConOutL(CStrF("(CWorld_PhysState::Phys_UpdateTree_r) More than 256 recursions! (%d)", iChild));
			for(int i = 0; i < 16; i++)
				ConOutL(CStrF("\t\tChild %i: %i", i, lChilds[i]));
*/
			break;
		}
	}

	_pObj->m_ClientFlags &= ~CWO_CLIENTFLAGS_RECURSED;
}

void CWorld_PhysState::Phys_InsertPosition(int _iObj, CWObject_CoreData* _pObj)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_InsertPosition, MAUTOSTRIP_VOID);
	MSCOPE(CWorld_PhysState::Phys_InsertPosition, PHYSSTATE);

	PHYSSTATE_ASSERTUNSAFE;

	DEBUG_CHECK_MATRIX(_pObj->m_LocalPos);
	if (_pObj->GetParent())
	{
		const CWObject_CoreData* pParent = Object_GetCD(_pObj->GetParent());
		if (pParent)
		{
			DEBUG_CHECK_MATRIX(pParent->GetPositionMatrix());
			if (_pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOROTINHERITANCE)
			{
				_pObj->m_Pos = _pObj->m_LocalPos;
				pParent->GetPosition().AddMatrixRow(_pObj->m_Pos, 3);
			}
			else
			{
				_pObj->m_LocalPos.Multiply(pParent->GetPositionMatrix() , _pObj->m_Pos);
			}
		}
		else
			_pObj->m_Pos = _pObj->m_LocalPos;
	}
	else
		_pObj->m_Pos = _pObj->m_LocalPos;

	if (_pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOREFRESH)
		_pObj->m_LastPos = _pObj->m_Pos;

	if( _pObj->m_pPhysCluster )
	{
		_pObj->m_PhysAbsBoundBox = _pObj->m_pPhysCluster->GetBoundingBox();
		_pObj->m_PhysAbsBoundBox.m_Min -= CVec3Dfp32(16.0f,16.0f,16.0f);
		_pObj->m_PhysAbsBoundBox.m_Max += CVec3Dfp32(16.0f,16.0f,16.0f);
	}
	else
		Phys_GetMinMaxBox(_pObj->m_PhysState, _pObj->GetPositionMatrix(), _pObj->m_PhysAbsBoundBox);

	if (!(_pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOHASH))
	{
		if (m_spSpaceEnum != NULL)
			m_spSpaceEnum->Insert(_pObj);
		_pObj->m_ClientFlags &= ~CWO_CLIENTFLAGS_HASHDIRTY;
	}
	else
	{
		if (m_spSpaceEnum != NULL)
			m_spSpaceEnum->Remove(_iObj);

		_pObj->m_ClientFlags |= CWO_CLIENTFLAGS_HASHDIRTY;
	}

	Object_SetDirty(_iObj, m_DirtyMask_InsertPosition);
	Object_NotifyListeners(_iObj, CWO_LISTENER_EVENT_MOVED);

	if (m_spSceneGraphInstance != NULL)
		m_SceneGraphDeferredLinkage.Insert(_iObj);

	if (_pObj->GetFirstChild())
		Phys_UpdateTree_r(_iObj, _pObj);

/*
	CStr s = CStrF("%d (%.4f, %.4f)", _iObj, pObj->m_Pos.k[3][0], pObj->m_Pos.k[3][1]);
	ConOut(s); LogFile(s);
	{
		int nEnum = m_spSpaceEnum->EnumerateBox(pObj->m_PhysAbsBoundBox.m_Min, pObj->m_PhysAbsBoundBox.m_Max, m_lEnumSpace.GetBasePtr(), m_lEnumSpace.Len());

		CStr s; for(int i = 0; i < nEnum; i++) s += CStrF("%d, ", m_lEnumSpace[i]);
		ConOut(s); LogFile(s);
	}*/
}

#define PHYS_AABB_EPSILON 0.001f

static bool Phys_Intersect_AABB(
	const CVec3Dfp32& _Origin1, const CVec3Dfp32& _Dest1, const CVec3Dfp32& _Extents1,
	const CVec3Dfp32& _Origin2, const CVec3Dfp32& _Dest2, const CVec3Dfp32& _Extents2,
	int _MediumFlags,
	CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(Phys_Intersect_AABB, false);

	if (M_Fabs(_Dest1[0] - _Dest2[0]) - (_Extents1[0] + _Extents2[0]) > 0.0f ||
		M_Fabs(_Dest1[1] - _Dest2[1]) - (_Extents1[1] + _Extents2[1]) > 0.0f ||
		M_Fabs(_Dest1[2] - _Dest2[2]) - (_Extents1[2] + _Extents2[2]) > 0.0f)
		return false;

	if (!_pCollisionInfo)
	{
		return true;
	}
	else
	{
/*		if (M_Fabs(_Dest1[0] - _Dest2[0]) - (_Extents1[0] + _Extents2[0]) > 0.0f ||
			M_Fabs(_Dest1[1] - _Dest2[1]) - (_Extents1[1] + _Extents2[1]) > 0.0f ||
			M_Fabs(_Dest1[2] - _Dest2[2]) - (_Extents1[2] + _Extents2[2]) > 0.0f)
			return false;*/
/*		CVec3Dfp32 Origin2;
		Origin2[0] = _Origin2[0] - _Dest1[0] + _Origin1[0];
		Origin2[1] = _Origin2[1] - _Dest1[1] + _Origin1[1];
		Origin2[2] = _Origin2[2] - _Dest1[2] + _Origin1[2];
*/
		CVec3Dfp32 BoxDist;
		BoxDist[0] = M_Fabs(_Origin1[0] - _Origin2[0]) - (_Extents1[0] + _Extents2[0]);
		BoxDist[1] = M_Fabs(_Origin1[1] - _Origin2[1]) - (_Extents1[1] + _Extents2[1]);
		BoxDist[2] = M_Fabs(_Origin1[2] - _Origin2[2]) - (_Extents1[2] + _Extents2[2]);

		int Plane = -1;
		if (BoxDist[0] > -PHYS_AABB_EPSILON)
			Plane = 0;
		else if (BoxDist[1] > -PHYS_AABB_EPSILON)
			Plane = 1;
		else if (BoxDist[2] > -PHYS_AABB_EPSILON)
			Plane = 2;

		if (Plane == -1)
		{
			if (BoxDist[0] > -1)
				Plane = 0;
			else if (BoxDist[1] > -1)
				Plane = 1;
			else if (BoxDist[2] > -1)
				Plane = 2;
		}

		if (Plane >= 0)
		{
			_pCollisionInfo->m_bIsValid = true;
			_pCollisionInfo->m_bIsCollision = true;

			if (!(_pCollisionInfo->m_ReturnValues & CXR_COLLISIONRETURNVALUE_TIME))
				return true;

			fp32 NormalSign = Sign(_Origin2[Plane] - _Origin1[Plane]);
			CVec3Dfp32 Normal(0);
			Normal[Plane] = NormalSign;


//			Sign((_Dest2[Plane]-_Origin2[Plane]) - (_Dest1[Plane]-_Origin1[Plane]))
			CVec3Dfp32 dV;
//			_Dest2.Sub(Origin2, dV);
			dV[0] = (_Dest2[0]-_Origin2[0]) - (_Dest1[0]-_Origin1[0]);
			dV[1] = (_Dest2[1]-_Origin2[1]) - (_Dest1[1]-_Origin1[1]);
			dV[2] = (_Dest2[2]-_Origin2[2]) - (_Dest1[2]-_Origin1[2]);

			fp32 Dist = M_Fabs(_Dest1[Plane] - _Dest2[Plane]) - (_Extents1[Plane] + _Extents2[Plane]);

			_pCollisionInfo->m_Distance = Dist-0.01f;
			_pCollisionInfo->m_Plane.CreateNV(Normal, _Dest1 + (Normal * _Extents1[Plane]));

			// Calculate a nice position for impact.
			CVec3Dfp32 Pos;
			_Dest1.Lerp(_Dest2, 0.5, Pos);
			Pos[0] = Max(Min(_Dest1[0] + _Extents1[0], Pos[0]), _Dest1[0] - _Extents1[0]);
			Pos[1] = Max(Min(_Dest1[1] + _Extents1[1], Pos[1]), _Dest1[1] - _Extents1[1]);
			Pos[2] = Max(Min(_Dest1[2] + _Extents1[2], Pos[2]), _Dest1[2] - _Extents1[2]);
			Pos[0] = Max(Min(_Dest2[0] + _Extents2[0], Pos[0]), _Dest2[0] - _Extents2[0]);
			Pos[1] = Max(Min(_Dest2[1] + _Extents2[1], Pos[1]), _Dest2[1] - _Extents2[1]);
			Pos[2] = Max(Min(_Dest2[2] + _Extents2[2], Pos[2]), _Dest2[2] - _Extents2[2]);
			Pos[Plane] = _Dest1[Plane] + Normal[Plane] * _Extents1[Plane];
			
			_pCollisionInfo->m_Pos = Pos;
			_pCollisionInfo->m_LocalPos = Pos;

			fp32 vProj = _pCollisionInfo->m_Plane.n * dV;
			fp32 dVLen = dV.Length();
			if (vProj < -0.001f * dVLen)
				_pCollisionInfo->m_Time = Clamp01(1.0f - _pCollisionInfo->m_Distance / vProj);
			else
			{
	//			ConOut(CStrF("Bad velocity projection dV %s, N %s", (char*) dV.GetString(), (char*)_pCollisionInfo->m_Plane.n.GetString()));
				_pCollisionInfo->m_Time = 1.0f;
			}


/*			M_TRACEALWAYS("Plane %d, Sign %.2f, Dist %f, Proj %f, T %f, Dest1 %s, Dest2 %s, dV %s\n", 
			Plane, NormalSign, _pCollisionInfo->m_Distance, vProj, _pCollisionInfo->m_Time, 
			_Dest1.GetString().Str(), _Dest2.GetString().Str(), dV.GetString().Str() );
*/
			return true;
		}
		else
		{
			// Boxes intersected from the beginning.
			_pCollisionInfo->m_bIsValid = false;
			_pCollisionInfo->m_bIsCollision = true;
			return true;
		}
	}
}



bool CWorld_PhysState::Phys_IntersectPrimitives(
	const CWO_PhysicsState& _PhysState1, const CWO_PhysicsPrim& _PhysPrim1, 
	const CWO_PhysicsState& _PhysState2, const CWO_PhysicsPrim& _PhysPrim2, 
		const CMat4Dfp32& _Origin1, const CMat4Dfp32& _Dest1, 
		const CMat4Dfp32& _Origin2, const CMat4Dfp32& _Dest2, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_IntersectPrimitives, false);
	MSCOPESHORT(CWorld_PhysState::Phys_IntersectPrimitives); //AR-SCOPE

	if ((_PhysPrim1.m_PrimType == OBJECT_PRIMTYPE_BOX) &&
		(_PhysPrim2.m_PrimType == OBJECT_PRIMTYPE_BOX) &&
		!(_PhysState1.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION) &&
		!(_PhysState2.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION))
	{
		CVec3Dfp32 Origin1 = CVec3Dfp32::GetRow(_Origin1, 3);
		CVec3Dfp32 Dest1 = CVec3Dfp32::GetRow(_Dest1, 3);
		CVec3Dfp32 Origin2 = CVec3Dfp32::GetRow(_Origin2, 3);
		CVec3Dfp32 Dest2 = CVec3Dfp32::GetRow(_Dest2, 3);
		if (_PhysState1.m_PhysFlags & OBJECT_PHYSFLAGS_OFFSET)
		{
			CVec3Dfp32 WOffs(_PhysPrim1.GetOffset());
			Origin1 += WOffs;
			Dest1 += WOffs;
		}
		if (_PhysState2.m_PhysFlags & OBJECT_PHYSFLAGS_OFFSET)
		{
			CVec3Dfp32 WOffs(_PhysPrim2.GetOffset());
			Origin2 += WOffs;
			Dest2 += WOffs;
		}

		return ::Phys_Intersect_AABB(
			Origin1, Dest1, _PhysPrim1.GetDim(),
			Origin2, Dest2, _PhysPrim2.GetDim(),
			0, _pCollisionInfo);

/*		return ::Phys_Intersect_AABB(
			CVec3Dfp32::GetRow(*pWMat1, 3),	// Note, motion from prim1 has been transfered to prim2
			CVec3Dfp32::GetRow(*pWMat1, 3),
			_PhysPrim1.GetDim(),
			CVec3Dfp32::GetRow(*pWMat2Org, 3),
			CVec3Dfp32::GetRow(*pWMat2, 3),
			_PhysPrim2.GetDim(),
			0, _pCollisionInfo);*/
	}

	const CMat4Dfp32* pWMat1Org = &_Origin1;
	const CMat4Dfp32* pWMat1 = &_Dest1;
	const CMat4Dfp32* pWMat2Org = &_Origin2;
	const CMat4Dfp32* pWMat2 = &_Dest2;

	CMat4Dfp32 WNew1, WNew1Org;
	CMat4Dfp32 WNew2, WNew2Org, WNew2Org2;

	// Add offset to primitive 1?
	if (_PhysState1.m_PhysFlags & OBJECT_PHYSFLAGS_OFFSET)
	{
		WNew1 = _Dest1;
		WNew1Org = _Origin1;
		CVec3Dfp32 WOffs(_PhysPrim1.GetOffset());
		if (_PhysState1.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION)
		{
			WNew1.k[3][0] += WOffs.k[0]*_Dest1.k[0][0] + WOffs.k[1]*_Dest1.k[1][0] + WOffs.k[2]*_Dest1.k[2][0];
			WNew1.k[3][1] += WOffs.k[0]*_Dest1.k[0][1] + WOffs.k[1]*_Dest1.k[1][1] + WOffs.k[2]*_Dest1.k[2][1];
			WNew1.k[3][2] += WOffs.k[0]*_Dest1.k[0][2] + WOffs.k[1]*_Dest1.k[1][2] + WOffs.k[2]*_Dest1.k[2][2];
			WNew1Org.k[3][0] += WOffs.k[0]*_Origin1.k[0][0] + WOffs.k[1]*_Origin1.k[1][0] + WOffs.k[2]*_Origin1.k[2][0];
			WNew1Org.k[3][1] += WOffs.k[0]*_Origin1.k[0][1] + WOffs.k[1]*_Origin1.k[1][1] + WOffs.k[2]*_Origin1.k[2][1];
			WNew1Org.k[3][2] += WOffs.k[0]*_Origin1.k[0][2] + WOffs.k[1]*_Origin1.k[1][2] + WOffs.k[2]*_Origin1.k[2][2];
		}
		else
		{
			CVec3Dfp32::GetRow(WNew1, 3) += WOffs;
			CVec3Dfp32::GetRow(WNew1Org, 3) += WOffs;
		}
		pWMat1 = &WNew1;
		pWMat1Org = &WNew1Org;
	}

	// Add offset to primitive 2?
	if (_PhysState2.m_PhysFlags & OBJECT_PHYSFLAGS_OFFSET)
	{
		WNew2 = *pWMat2;
		WNew2Org = *pWMat2Org;
		CVec3Dfp32 WOffs(_PhysPrim2.GetOffset());
		if (_PhysState2.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION)
		{
			WNew2.k[3][0] += WOffs.k[0]*_Dest2.k[0][0] + WOffs.k[1]*_Dest2.k[1][0] + WOffs.k[2]*_Dest2.k[2][0];
			WNew2.k[3][1] += WOffs.k[0]*_Dest2.k[0][1] + WOffs.k[1]*_Dest2.k[1][1] + WOffs.k[2]*_Dest2.k[2][1];
			WNew2.k[3][2] += WOffs.k[0]*_Dest2.k[0][2] + WOffs.k[1]*_Dest2.k[1][2] + WOffs.k[2]*_Dest2.k[2][2];
			WNew2Org.k[3][0] += WOffs.k[0]*_Origin2.k[0][0] + WOffs.k[1]*_Origin2.k[1][0] + WOffs.k[2]*_Origin2.k[2][0];
			WNew2Org.k[3][1] += WOffs.k[0]*_Origin2.k[0][1] + WOffs.k[1]*_Origin2.k[1][1] + WOffs.k[2]*_Origin2.k[2][1];
			WNew2Org.k[3][2] += WOffs.k[0]*_Origin2.k[0][2] + WOffs.k[1]*_Origin2.k[1][2] + WOffs.k[2]*_Origin2.k[2][2];
		}
		else
		{
			CVec3Dfp32::GetRow(WNew2, 3) += WOffs;
			CVec3Dfp32::GetRow(WNew2Org, 3) += WOffs;
		}
		pWMat2 = &WNew2;
		pWMat2Org = &WNew2Org;
	}

	// Make all motion relative to a static prim1.
	CMat4Dfp32 Org1Inv, Prim1Delta;
	_Origin1.InverseOrthogonal(Org1Inv);
	_Dest1.Multiply(Org1Inv, Prim1Delta);

	pWMat2Org->Multiply(Prim1Delta, WNew2Org2);
	pWMat2Org = &WNew2Org2;

	switch(_PhysPrim1.m_PrimType)
	{
	case OBJECT_PRIMTYPE_NONE :
		{
			return false;
		}

	case OBJECT_PRIMTYPE_PHYSMODEL :
		{
			CXR_Model* pModel = m_spMapData->GetResource_Model(_PhysPrim1.m_iPhysModel);
			CXR_PhysicsModel* pPhysModel = (pModel) ? pModel->Phys_GetInterface() : NULL;
			if (!pPhysModel) return false;
			
			// Support more than one model instance ?
			CXR_AnimState AnimState;
			AnimState.Clear();
			AnimState.m_pModelInstance = _PhysState1.m_pModelInstance;
			
			CXR_PhysicsContext PhysContext(*pWMat1, &AnimState, m_spWireContainer);
			PhysContext.m_PhysGroupMaskThis = _PhysPrim1.m_PhysModelMask;
			PhysContext.m_PhysGroupMaskCollider = _PhysPrim2.m_PhysModelMask;
			pPhysModel->Phys_Init(&PhysContext);

			switch(_PhysPrim2.m_PrimType)
			{
			case OBJECT_PRIMTYPE_PHYSMODEL :
				{
	//	disabled	CXR_PhysicsModel* pPhysModel2 = m_spMapData->GetResource_Model(_PhysPrim2.m_iPhysModel)->Phys_GetInterface();
	//	 for now	int ModelClass = pPhysModel2->GetModelClass();
	//				if (ModelClass == CXR_MODEL_CLASS_BSP2)
	//				{
	//					CXR_Model_BSP2* pBSP2 = safe_cast<CXR_Model_BSP2>(pPhysModel2);
	//					int nColl = pPhysModel->Phys_CollideBSP2(&PhysContext, pBSP2, *pWMat2, 0, _PhysState2.m_MediumFlags, _pCollisionInfo, _pCollisionInfo ? 1 : 0);
	//					return (nColl != 0);
	//				}
					return false;
					break;
				}
			case OBJECT_PRIMTYPE_SPHERE :
				return pPhysModel->Phys_IntersectSphere(
					&PhysContext,
					CVec3Dfp32::GetMatrixRow(*pWMat2Org, 3),
					CVec3Dfp32::GetMatrixRow(*pWMat2, 3),
					_PhysPrim2.GetRadius(), _PhysState2.m_MediumFlags, _pCollisionInfo);

			case OBJECT_PRIMTYPE_BOX :
				{
					CPhysOBB OBBOrg;
					CPhysOBB OBBDest;
					OBBOrg.SetDimensions(_PhysPrim2.GetDim());
					OBBOrg.SetPosition(*pWMat2Org);
					OBBDest.SetDimensions(_PhysPrim2.GetDim());
					OBBDest.SetPosition(*pWMat2);
					return pPhysModel->Phys_IntersectBox(&PhysContext, OBBOrg, OBBDest, _PhysState2.m_MediumFlags, _pCollisionInfo);
				}

			case OBJECT_PRIMTYPE_POINT :
				{
					CVec3Dfp32 P0(CVec3Dfp32::GetMatrixRow(_Origin2, 3));
					CVec3Dfp32 P1(CVec3Dfp32::GetMatrixRow(_Dest2, 3));
					CVec3Dfp32 dV1;
					CVec3Dfp32::GetMatrixRow(_Dest1, 3).Sub(CVec3Dfp32::GetMatrixRow(_Origin1, 3), dV1);
					P1 -= dV1;
					return pPhysModel->Phys_IntersectLine(&PhysContext, P0, P1, _PhysState2.m_MediumFlags, _pCollisionInfo);
				}

			default :
				Error("Phys_IntersectPrimitives", CStrF("Invalid primitive type 2. (PhysModel <-> %d)", _PhysPrim2.m_PrimType));
			}
		}

	case OBJECT_PRIMTYPE_SPHERE :
		{
			m_PhysModel_Sphere.Phys_SetDimensions(_PhysPrim1.GetRadius());
			CXR_PhysicsContext PhysContext(*pWMat1);
			m_PhysModel_Sphere.Phys_Init(&PhysContext);
			switch(_PhysPrim2.m_PrimType)
			{
			case OBJECT_PRIMTYPE_SPHERE :
//				return false;
				return m_PhysModel_Sphere.Phys_IntersectSphere(
					&PhysContext,
					CVec3Dfp32::GetMatrixRow(*pWMat2Org, 3),
					CVec3Dfp32::GetMatrixRow(*pWMat2, 3),
					_PhysPrim2.GetRadius(), 0, _pCollisionInfo);
	
			case OBJECT_PRIMTYPE_BOX :
				{
					CPhysOBB OBBOrg;
					CPhysOBB OBBDest;
					OBBOrg.SetDimensions(_PhysPrim2.GetDim());
					OBBOrg.SetPosition(*pWMat2Org);
					OBBDest.SetDimensions(_PhysPrim2.GetDim());
					OBBDest.SetPosition(*pWMat2);
					return m_PhysModel_Sphere.Phys_IntersectBox(&PhysContext, OBBOrg, OBBDest, 0, _pCollisionInfo);
//					return m_PhysModel_Sphere.Phys_IntersectBox(OBB, 0, _pCollisionInfo);
				}

			case OBJECT_PRIMTYPE_POINT :
				{
					CVec3Dfp32 P0(CVec3Dfp32::GetMatrixRow(_Origin2, 3));
					CVec3Dfp32 P1(CVec3Dfp32::GetMatrixRow(_Dest2, 3));
					CVec3Dfp32 dV1;
					CVec3Dfp32::GetMatrixRow(_Dest1, 3).Sub(CVec3Dfp32::GetMatrixRow(_Origin1, 3), dV1);
					P1 -= dV1;
					return m_PhysModel_Sphere.Phys_IntersectLine(&PhysContext, P0, P1, _PhysState2.m_MediumFlags, _pCollisionInfo);
				}

			default :
				Error("Phys_IntersectPrimitives", CStrF("Invalid primitive type 2. (Sphere <-> %d)", _PhysPrim2.m_PrimType));
			}
		}

	case OBJECT_PRIMTYPE_BOX :
		{
			switch(_PhysPrim2.m_PrimType)
			{
			case OBJECT_PRIMTYPE_BOX :
				{
					m_PhysModel_Box.Phys_SetDimensions(_PhysPrim1.GetDim());
					CXR_PhysicsContext PhysContext(*pWMat1);
					m_PhysModel_Box.Phys_Init(&PhysContext);
					CPhysOBB OBBOrg;
					CPhysOBB OBBDest;
					OBBOrg.SetDimensions(_PhysPrim2.GetDim());
					OBBOrg.SetPosition(*pWMat2Org);
					OBBDest.SetDimensions(_PhysPrim2.GetDim());
					OBBDest.SetPosition(*pWMat2);
					return m_PhysModel_Box.Phys_IntersectBox(&PhysContext, OBBOrg, OBBDest, 0, _pCollisionInfo);
				}

			case OBJECT_PRIMTYPE_POINT :
				{
					m_PhysModel_Box.Phys_SetDimensions(_PhysPrim1.GetDim());
					CXR_PhysicsContext PhysContext(*pWMat1);
					m_PhysModel_Box.Phys_Init(&PhysContext);

					CVec3Dfp32 P0(CVec3Dfp32::GetMatrixRow(*pWMat2Org, 3));
					CVec3Dfp32 P1(CVec3Dfp32::GetMatrixRow(*pWMat2, 3));
/*					CVec3Dfp32 dV1;
					CVec3Dfp32::GetMatrixRow(*pWMat1, 3).Sub(CVec3Dfp32::GetMatrixRow(*pWMat1Org, 3), dV1);
					P1 -= dV1;*/
					bool Res = m_PhysModel_Box.Phys_IntersectLine(&PhysContext, P0, P1, _PhysState2.m_MediumFlags, _pCollisionInfo);
/*				ConOut(CStrF("Box<->Point: %d", Res));
				if (_pCollisionInfo)
					if (Res) ConOut(CStrF("bValid %d, Plane %d", _pCollisionInfo->m_bIsValid, (char*)_pCollisionInfo->m_Plane.GetString() ));
				else
					ConOut("No CInfo");*/
					return Res;
				}

			default :
				Error("Phys_IntersectPrimitives", CStrF("Invalid primitive type 2. (Box <-> %d)", _PhysPrim2.m_PrimType));
			}
		}

	case OBJECT_PRIMTYPE_POINT :
		{
			// Points can't collide to each-other.
			return false;
		}

	default :
		Error("Phys_IntersectPrimitives", CStrF("Invalid primitive type 1. (%d, %d)", _PhysPrim1.m_PrimType, _PhysPrim2.m_PrimType));
	}
	return false;
}

int CWorld_PhysState::Phys_IntersectStates(const CWO_PhysicsState& _PhysState1, const CWO_PhysicsState& _PhysState2, 
	const CMat4Dfp32& _Origin1, const CMat4Dfp32& _Dest1, 
	const CMat4Dfp32& _Origin2, const CMat4Dfp32& _Dest2, CCollisionInfo* _pCollisionInfo, int _NotifyFlags1, int _NotifyFlags2)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_IntersectStates, false);
	MSCOPESHORT(CWorld_PhysState::Phys_IntersectStates); //AR-SCOPE

	bool bIntersect = false;

	uint16 OF1 = _PhysState1.m_ObjectFlags;
	uint16 OIF1 = _PhysState1.m_ObjectIntersectFlags;
	uint16 OF2 = _PhysState2.m_ObjectFlags;
	uint16 OIF2 = _PhysState2.m_ObjectIntersectFlags;

	int nPrim1 = _PhysState1.m_nPrim;
	int nPrim2 = _PhysState2.m_nPrim;
	for(int iPrim1 = 0; iPrim1 < nPrim1; iPrim1++)
		for(int iPrim2 = 0; iPrim2 < nPrim2; iPrim2++)
		{
			int TestMask = 0;
			const CWO_PhysicsPrim& Prim1 = _PhysState1.m_Prim[iPrim1];
			const CWO_PhysicsPrim& Prim2 = _PhysState2.m_Prim[iPrim2];

			if (((OF1 & Prim1.m_ObjectFlagsMask) & OIF2) ||
				((OF2 & Prim2.m_ObjectFlagsMask) & OIF1))
				TestMask = 1;

			if ((OF2 & Prim2.m_ObjectFlagsMask) & _NotifyFlags1)
				TestMask = 2;

			if ((OF1 & Prim1.m_ObjectFlagsMask) & _NotifyFlags2)
				TestMask = 4;

			if ((TestMask & 1) && _pCollisionInfo)
			{
				CCollisionInfo CInfo;
				CInfo.SetReturnValues(_pCollisionInfo->m_ReturnValues);

				if (Prim1.m_PrimType > Prim2.m_PrimType)
				{
					bIntersect = Phys_IntersectPrimitives(_PhysState2, Prim2, _PhysState1, Prim1, _Origin2, _Dest2, _Origin1, _Dest1, &CInfo);
					if (bIntersect)
						CInfo.m_Plane.n = -CInfo.m_Plane.n;
				}
				else
				{
					bIntersect = Phys_IntersectPrimitives(_PhysState1, Prim1, _PhysState2, Prim2, _Origin1, _Dest1, _Origin2, _Dest2, &CInfo);
				}

				if (bIntersect)
				{
					if (_pCollisionInfo)
					{
						_pCollisionInfo->Improve(CInfo);
						if (_pCollisionInfo->IsComplete())
							return TestMask;
					}
					else
						return TestMask;
				}
			}
			else if (TestMask)
			{
				if (Prim1.m_PrimType > Prim2.m_PrimType)
				{
					bIntersect = Phys_IntersectPrimitives(_PhysState2, Prim2, _PhysState1, Prim1, _Origin2, _Dest2, _Origin1, _Dest1, NULL);
				}
				else
				{
					bIntersect = Phys_IntersectPrimitives(_PhysState1, Prim1, _PhysState2, Prim2, _Origin1, _Dest1, _Origin2, _Dest2, NULL);
				}
				if (bIntersect)
					return TestMask;
			}
		}

	if (_pCollisionInfo)
		return _pCollisionInfo->m_bIsCollision;
	else
		return false;
}

bool CWorld_PhysState::Phys_SetPosition(const CSelection& _Selection, int _iObj, const CMat4Dfp32& _Pos, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_SetPosition, false);
	Error("Phys_SetPosition", "Internal error.");
	return false;
}


bool CWorld_PhysState::Phys_MovePosition(CPotColSet *_pcs, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_MovePosition, false);
	Error("Phys_MovePosition", "Internal error.");
	return false;
}

bool CWorld_PhysState::Phys_MovePosition(const CSelection* _pSelection, int _iObj, const CMat4Dfp32& _Origin, const CMat4Dfp32& _Dest, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_MovePosition, false);
	Error("Phys_MovePosition", "Internal error.");
	return false;
}

void CWorld_PhysState::Phys_MessageQueue_Flush()
{
	MAUTOSTRIP(CWorld_PhysState_Phys_MessageQueue_Flush, MAUTOSTRIP_VOID);
	Error("Phys_SetPosition", "Internal error.");
}

// -------------------------------------------------------------------
int CWorld_PhysState::Phys_GetMedium(const CVec3Dfp32& _Pos, CXR_MediumDesc* _pRetMedium)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_GetMedium, 0);
#ifdef	SERVER_STATS
	m_nFunc_GetMedium++;
	TMeasure(m_TFunc_GetMedium);
#endif
	M_ASSERT(m_spSpaceEnum != NULL, "!");

	CWO_EnumParams_Box Params;
	Params.m_Box.m_Min = _Pos;
	Params.m_Box.m_Max = _Pos;
	Params.m_ObjectFlags = OBJECT_FLAGS_PHYSMODEL;
	Params.m_ObjectIntersectFlags = 0;
	Params.m_ObjectNotifyFlags = 0;
	int nEnum = m_spSpaceEnum->EnumerateBox(Params, m_lEnumSpace.GetBasePtr(), m_lEnumSpace.Len());

	int Medium = XW_MEDIUM_AIR;
	CXR_MediumDesc MediumDesc;
	MediumDesc.SetAir();
	for(int i = 0; i < nEnum; i++)
	{
		int iObj = m_lEnumSpace[i];
		CWObject_CoreData* pObj = Object_GetCD(iObj);
		if (!pObj) continue;

		const CWO_PhysicsState& PhysState = pObj->GetPhysState();

		for(int iPrim = 0; iPrim < PhysState.m_nPrim; iPrim++)
		{
			const CWO_PhysicsPrim& PhysPrim = PhysState.m_Prim[iPrim];

			if (PhysPrim.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL)
			{
				CXR_Model* pModel = m_spMapData->GetResource_Model(PhysPrim.m_iPhysModel);
				if (!pModel) continue;
				CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();
				if (!pPhysModel) continue;
				CXR_AnimState Anim(CMTime::CreateFromTicks(GetGameTick() - pObj->m_CreationGameTick, GetGameTickTime(), -pObj->m_CreationGameTickFraction), CMTime(), pObj->m_iAnim0, pObj->m_iAnim1, 0, 0);
				CXR_PhysicsContext PhysContext(pObj->GetPositionMatrix(), &Anim, m_spWireContainer);
				PhysContext.m_PhysGroupMaskThis = PhysPrim.m_PhysModelMask;
				pPhysModel->Phys_Init(&PhysContext);
				CXR_MediumDesc TmpMedium;
				pPhysModel->Phys_GetMedium(&PhysContext, _Pos, TmpMedium);
				int m = TmpMedium.m_MediumFlags;
				if (m && ((m & XW_MEDIUM_TYPEMASK) < (Medium & XW_MEDIUM_TYPEMASK)))
				{
					Medium = m;
					MediumDesc = TmpMedium;
					MediumDesc.m_User2 = iObj;
				}
			}
		}
	}

	if (_pRetMedium)
	{
		*_pRetMedium = MediumDesc;
	}

	return Medium;
}

void CWorld_PhysState::Phys_GetMediums(const CSelection& _Selection, const CVec3Dfp32* _pV, int _nV, CXR_MediumDesc* _pRetMediums)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_GetMediums, MAUTOSTRIP_VOID);
#ifdef	SERVER_STATS
	m_nFunc_GetMedium++;
	TMeasure(m_TFunc_GetMedium);
#endif

	// Reset
	for(int v = 0; v < _nV; v++)
		_pRetMediums[v].SetAir();

	const int16* piSel;
	int nSel = Selection_Get(_Selection, &piSel);

	for(int i = 0; i < nSel; i++)
	{
		int iObj = piSel[i];
		CWObject_CoreData* pObj = Object_GetCD(iObj);
		if (!pObj) continue;
		const CWO_PhysicsState& PhysState = pObj->GetPhysState();

		for(int iPrim = 0; iPrim < PhysState.m_nPrim; iPrim++)
		{
			const CWO_PhysicsPrim& PhysPrim = PhysState.m_Prim[iPrim];
			if ((PhysPrim.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL) &&
				(PhysState.m_ObjectFlags & OBJECT_FLAGS_PHYSMODEL))
			{
				CXR_Model* pModel = m_spMapData->GetResource_Model(PhysPrim.m_iPhysModel);
				if (!pModel) continue;
				CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();
				if (!pPhysModel) continue;
				CXR_AnimState Anim(CMTime::CreateFromTicks(GetGameTick() - pObj->m_CreationGameTick, GetGameTickTime(), -pObj->m_CreationGameTickFraction), CMTime(), pObj->m_iAnim0, pObj->m_iAnim1, 0, 0);
				CXR_PhysicsContext PhysContext(pObj->GetPositionMatrix(), &Anim, m_spWireContainer);
				PhysContext.m_PhysGroupMaskThis = PhysPrim.m_PhysModelMask;
				pPhysModel->Phys_Init(&PhysContext);
				CXR_MediumDesc TmpMediums[32];
				if (_nV > 32) Error("Phys_GetMediums", "Maximum number of vertices is 32.");
				pPhysModel->Phys_GetMedium(&PhysContext, _pV, _nV, TmpMediums);

				for(int v = 0; v < _nV; v++)
				{
					int m = TmpMediums[v].m_MediumFlags;
					if (m && ((m & XW_MEDIUM_TYPEMASK) < (_pRetMediums[v].m_MediumFlags & XW_MEDIUM_TYPEMASK)))
					{
						_pRetMediums[v] = TmpMediums[v];
						_pRetMediums[v].m_Velocity += pObj->GetMoveVelocity();
					}
				}
			}
		}
	}

}

void CWorld_PhysState::Phys_GetMediums(const CVec3Dfp32* _pV, int _nV, CXR_MediumDesc* _pRetMediums)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_GetMediums_2, MAUTOSTRIP_VOID);
	M_ASSERT(m_spSpaceEnum != NULL, "!");
#ifdef	SERVER_STATS
	m_nFunc_GetMedium++;
	TMeasure(m_TFunc_GetMedium);
#endif

	CWO_EnumParams_Box Params;
	CVec3Dfp32::GetMinBoundBox(_pV, Params.m_Box.m_Min, Params.m_Box.m_Max, _nV);
	Params.m_ObjectFlags = OBJECT_FLAGS_PHYSMODEL;
	Params.m_ObjectIntersectFlags = 0;
	Params.m_ObjectNotifyFlags = 0;
	int nEnum = m_spSpaceEnum->EnumerateBox(Params, m_lEnumSpace.GetBasePtr(), m_lEnumSpace.Len());

	// Reset
	for(int v = 0; v < _nV; v++)
		_pRetMediums[v].SetAir();

	for(int i = 0; i < nEnum; i++)
	{
		int iObj = m_lEnumSpace[i];
		CWObject_CoreData* pObj = Object_GetCD(iObj);
		if (!pObj) continue;
		const CWO_PhysicsState& PhysState = pObj->GetPhysState();

		for(int iPrim = 0; iPrim < pObj->m_PhysState.m_nPrim; iPrim++)
		{
			const CWO_PhysicsPrim& PhysPrim = PhysState.m_Prim[iPrim];
			if (PhysPrim.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL)
			{
				CXR_Model* pModel = m_spMapData->GetResource_Model(PhysPrim.m_iPhysModel);
				if (!pModel) continue;
				CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();
				if (!pPhysModel) continue;
				CXR_AnimState Anim(CMTime::CreateFromTicks(GetGameTick() - pObj->m_CreationGameTick, GetGameTickTime(), -pObj->m_CreationGameTickFraction), CMTime(), pObj->m_iAnim0, pObj->m_iAnim1, 0, 0);
				CXR_PhysicsContext PhysContext(pObj->GetPositionMatrix(), &Anim, m_spWireContainer);
				PhysContext.m_PhysGroupMaskThis = PhysPrim.m_PhysModelMask;
				pPhysModel->Phys_Init(&PhysContext);
				CXR_MediumDesc TmpMediums[32];
				if (_nV > 32) Error("Phys_GetMediums", "Maximum number of vertices is 32.");
				pPhysModel->Phys_GetMedium(&PhysContext, _pV, _nV, TmpMediums);

				for(int v = 0; v < _nV; v++)
				{
					int m = TmpMediums[v].m_MediumFlags;
					if (m && ((m & XW_MEDIUM_TYPEMASK) < (_pRetMediums[v].m_MediumFlags & XW_MEDIUM_TYPEMASK)))
					{
						_pRetMediums[v] = TmpMediums[v];
						_pRetMediums[v].m_Velocity += pObj->GetMoveVelocity();
					}
				}
			}
		}
	}

}

bool CWorld_PhysState::Phys_IntersectLine(const CSelection& _Selection, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _ObjectFlags, int _ObjectIntersectFlags, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_IntersectLine, false);
	// _ObjectFlags				Correspond to CWO_PhysicsState::m_ObjectFlags, it says what the "object" you are tracing is.
	// _ObjectIntersectFlags	Correspond to CWO_PhysicsState::m_ObjectIntersectFlags, it says what the "object" you are tracing is colliding to.

	// For example, a rocket may have _ObjectFlags = OBJECT_FLAGS_PROJECTILE, and _ObjectIntersectFlags = OBJECT_FLAGS_PHYSOBJECT
#ifdef	SERVER_STATS
	m_nFunc_IntersectLine++;
	TMeasure(m_TFunc_IntersectLine);
#endif

//	if (_pCollisionInfo) _pCollisionInfo->m_bIsValid = false;

	const int invalid = ( FloatIsInvalid(_p0[0]) << 0 )
				| ( FloatIsInvalid(_p0[1]) << 1 )
				| ( FloatIsInvalid(_p0[2]) << 2 )
				| ( FloatIsInvalid(_p1[0]) << 3 )
				| ( FloatIsInvalid(_p1[1]) << 4 )
				| ( FloatIsInvalid(_p1[2]) << 5 );
	const fp32 sum = M_Fabs(_p0[0]) + M_Fabs(_p0[1]) + M_Fabs(_p0[2]) + M_Fabs(_p1[0]) + M_Fabs(_p1[1]) + M_Fabs(_p1[2]);

//	if (FloatIsInvalid(_p0[0]) || FloatIsInvalid(_p0[1]) || FloatIsInvalid(_p0[2]) ||
//		FloatIsInvalid(_p1[0]) || FloatIsInvalid(_p1[1]) || FloatIsInvalid(_p1[2]) ||
//		M_Fabs(_p0[0]) + M_Fabs(_p0[1]) + M_Fabs(_p0[2]) + M_Fabs(_p1[0]) + M_Fabs(_p1[1]) + M_Fabs(_p1[2]) > 1000000)
	if( invalid || sum > 1000000 )
	{
		M_TRACEALWAYS("(CWorld_PhysState::Phys_IntersectLine) Invalid trace %s - %s\n", _p0.GetString().Str(), _p1.GetString().Str());
		M_ASSERT(0, "!");
		return false;
	}

	const int16* piObj;
	int nObj = Selection_Get(_Selection, &piObj);

	bool bImpact = false;
	for(int i = 0; i < nObj; i++)
	{
		int iObj = piObj[i];
		CWObject_CoreData* pObj = Object_GetCD(iObj);
		CVec3Dfp32 HitPos;
		if (pObj && pObj->GetAbsBoundBox()->IntersectLine(_p0, _p1, HitPos))
			if (Object_IntersectLine(iObj, _p0, _p1, _ObjectFlags, _ObjectIntersectFlags, _MediumFlags, _pCollisionInfo))
			{
				if (!_pCollisionInfo)
				{
					return true;
				}
				bImpact = true;
			}
	}

	return bImpact;
}

bool CWorld_PhysState::Phys_IntersectLine(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _ObjectFlags, int _ObjectIntersectFlags, int _MediumFlags, CCollisionInfo* _pCollisionInfo, int _iExclude, bool _bUseVisBox)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_IntersectLine_2, false);
	// _ObjectFlags				Correspond to CWO_PhysicsState::m_ObjectFlags, it says what the "object" you are tracing is.
	// _ObjectIntersectFlags	Correspond to CWO_PhysicsState::m_ObjectIntersectFlags, it says what the "object" you are tracing is colliding to.

	// For example, a rocket may have _ObjectFlags = OBJECT_FLAGS_PROJECTILE, and _ObjectIntersectFlags = OBJECT_FLAGS_PHYSOBJECT
	M_ASSERT(m_spSpaceEnum != NULL, "!");
#ifdef	SERVER_STATS
	m_nFunc_IntersectLine++;
	TMeasure(m_TFunc_IntersectLine);
#endif

//	if (_pCollisionInfo) _pCollisionInfo->m_bIsValid = false;

	if (FloatIsInvalid(_p0[0]) || FloatIsInvalid(_p0[1]) || FloatIsInvalid(_p0[2]) ||
		FloatIsInvalid(_p1[0]) || FloatIsInvalid(_p1[1]) || FloatIsInvalid(_p1[2]) ||
		M_Fabs(_p0[0]) + M_Fabs(_p0[1]) + M_Fabs(_p0[2]) + M_Fabs(_p1[0]) + M_Fabs(_p1[1]) + M_Fabs(_p1[2]) > 1000000)
	{
		M_TRACEALWAYS("(CWorld_PhysState::Phys_IntersectLine) Invalid trace %s - %s\n", _p0.GetString().Str(), _p1.GetString().Str());
		ConOut(CStrF("(CWorld_PhysState::Phys_IntersectLine) Invalid trace %s - %s\n", _p0.GetString().Str(), _p1.GetString().Str()));
//		M_ASSERT(0, "!");
		return false;
	}

	CWO_EnumParams_Line Params;
	Params.m_V0 = _p0;
	Params.m_V1 = _p1;
	Params.m_ObjectFlags = _ObjectIntersectFlags;
	Params.m_ObjectIntersectFlags = _ObjectFlags;
	Params.m_ObjectNotifyFlags = 0;

	bool bImpact = false;
	int nEnum = m_spSpaceEnum->EnumerateLine(Params, m_lEnumSpace.GetBasePtr(), m_lEnumSpace.Len(),_bUseVisBox);

	for(int i = 0; i < nEnum; i++)
	{
		int iObj = m_lEnumSpace[i];
		if (iObj == _iExclude) continue;
		if (Object_IntersectLine(iObj, _p0, _p1, _ObjectFlags, _ObjectIntersectFlags, _MediumFlags, _pCollisionInfo))
		{
			if (!_pCollisionInfo)
			{
				return true;
			}
			bImpact = true;
		}
	}

	return bImpact;
}

bool CWorld_PhysState::Phys_IntersectLine(const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _ObjectFlags, int _ObjectIntersectFlags, int _MediumFlags, int _iExclude0,int _iExclude1, bool _bUseVisBox)
{
	MAUTOSTRIP(CWorld_PhysState_Phys_IntersectLine_3, false);
	// _ObjectFlags				Correspond to CWO_PhysicsState::m_ObjectFlags, it says what the "object" you are tracing is.
	// _ObjectIntersectFlags	Correspond to CWO_PhysicsState::m_ObjectIntersectFlags, it says what the "object" you are tracing is colliding to.

	// For example, a rocket may have _ObjectFlags = OBJECT_FLAGS_PROJECTILE, and _ObjectIntersectFlags = OBJECT_FLAGS_PHYSOBJECT
	M_ASSERT(m_spSpaceEnum != NULL, "!");
#ifdef	SERVER_STATS
	m_nFunc_IntersectLine++;
	TMeasure(m_TFunc_IntersectLine);
#endif

	//	if (_pCollisionInfo) _pCollisionInfo->m_bIsValid = false;

	if (FloatIsInvalid(_p0[0]) || FloatIsInvalid(_p0[1]) || FloatIsInvalid(_p0[2]) ||
		FloatIsInvalid(_p1[0]) || FloatIsInvalid(_p1[1]) || FloatIsInvalid(_p1[2]) ||
		M_Fabs(_p0[0]) + M_Fabs(_p0[1]) + M_Fabs(_p0[2]) + M_Fabs(_p1[0]) + M_Fabs(_p1[1]) + M_Fabs(_p1[2]) > 1000000)
	{
		M_TRACEALWAYS("(CWorld_PhysState::Phys_IntersectLine) Invalid trace %s - %s\n", _p0.GetString().Str(), _p1.GetString().Str());
		ConOut(CStrF("(CWorld_PhysState::Phys_IntersectLine) Invalid trace %s - %s\n", _p0.GetString().Str(), _p1.GetString().Str()));
		//		M_ASSERT(0, "!");
		return false;
	}

	CWO_EnumParams_Line Params;
	Params.m_V0 = _p0;
	Params.m_V1 = _p1;
	Params.m_ObjectFlags = _ObjectIntersectFlags;
	Params.m_ObjectIntersectFlags = _ObjectFlags;
	Params.m_ObjectNotifyFlags = 0;

	bool bImpact = false;
	int nEnum = m_spSpaceEnum->EnumerateLine(Params, m_lEnumSpace.GetBasePtr(), m_lEnumSpace.Len(),_bUseVisBox);
	// *** TempFix for BSP2 bug that lets tracelines without CollInfo through ***
	CCollisionInfo CollInfo;
	CollInfo.m_bIsValid = false;
	// ***
	for(int i = 0; i < nEnum; i++)
	{
		int iObj = m_lEnumSpace[i];
		if ((iObj == _iExclude0)||(iObj == _iExclude1)) continue;
		if (Object_IntersectLine(iObj, _p0, _p1, _ObjectFlags, _ObjectIntersectFlags, _MediumFlags, &CollInfo))	// ***
		{
			return true;
		}
	}

	return bImpact;
}



