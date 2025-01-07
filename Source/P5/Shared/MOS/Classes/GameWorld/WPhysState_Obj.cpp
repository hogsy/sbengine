#include "PCH.h"
#include "WPhysState.h"
#include "WPhysState_Hash.h"
#include "MFloat.h"
#include "WBlockNavGrid.h"
#include "WDynamics.h"

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

#define DYNAMICSENGINE2

// -------------------------------------------------------------------
#ifdef _DEBUG
	#include "MFloat.h"
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
	#define DEBUG_CHECK_VECTOR(v)
	#define DEBUG_CHECK_AXISANGLE(q)
#endif

// -------------------------------------------------------------------
const CVec3Dfp32 g_ZeroVec(0);
const CAxisRotfp32 g_UnitRot(0,0);

int CWorld_PhysState::Object_GetWorldspawnIndex()
{
	MAUTOSTRIP(CWorld_PhysState_Object_GetWorldspawnIndex, 0);
	return m_iObject_Worldspawn;
}

void CWorld_PhysState::Object_SetWorldspawnIndex(int _iObj)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetWorldspawnIndex, MAUTOSTRIP_VOID);
	m_iObject_Worldspawn = _iObj;
}

const CVec3Dfp32& CWorld_PhysState::Object_GetPosition(int _iObj)
{
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
		return CVec3Dfp32::GetMatrixRow(pObj->m_Pos, 3);
	else
		return g_ZeroVec;
}

vec128 CWorld_PhysState::Object_GetPosition_vec128(int _iObj)
{
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
		return pObj->m_Pos.r[3];
	else
		return M_VConst(0,0,0,1.0f);
}


const CMat4Dfp32& CWorld_PhysState::Object_GetPositionMatrix(int _iObj)
{
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
		return pObj->m_Pos;
	else
		return m_Unit;
}

const CVec3Dfp32& CWorld_PhysState::Object_GetVelocity(int _iObj)
{
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
		return pObj->m_PhysVelocity.m_Move;
	else
		return g_ZeroVec;
}

const CMat4Dfp32& CWorld_PhysState::Object_GetVelocityMatrix(int _iObj)
{
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
	{
//		return pObj->GetVelocityMatrix();
		ConOutL("Object_GetVelocityMatrix");
		return m_Unit;
	}
	else
		return m_Unit;
}

const CAxisRotfp32& CWorld_PhysState::Object_GetRotVelocity(int _iObj)
{
	MAUTOSTRIP(CWorld_PhysState_Object_GetRotVelocity, MAUTOSTRIP_VOID);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
		return pObj->m_PhysVelocity.m_Rot;

	return g_UnitRot;
}

//#define PhysError(func, msg) ConOutL("(CWorld_PhysState::"##func##") "##msg)
#ifdef _DEBUG
	#define PhysError(func, msg) M_ASSERT(0, "!")
#else
	#define PhysError(func, msg) {}
#endif

// -------------------------------------------------------------------
bool CWorld_PhysState::Object_SetPhysics(int _iObj, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Pos)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetPhysics, false);
	MSCOPESHORT(CWorld_PhysState::Object_SetPhysics_1);
	DEBUG_CHECK_VECTOR(_Pos.GetRow(3));

	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj)
	{
		PhysError("Object_SetPhysics", "NULL Object.");
		return false;
	}
	if(_PhysState.m_nPrim > 0)
	{
		if (Phys_IntersectWorld((CSelection*) NULL, _PhysState, _Pos, _Pos, _iObj)) 
			return false;
	}

	DEBUG_CHECK_MATRIX(_Pos);
	pObj->m_PhysState = _PhysState;
	pObj->m_LocalPos = _Pos;
	Phys_InsertPosition(_iObj, pObj);

	return true;
}

bool CWorld_PhysState::Object_SetPhysics(int _iObj, const CWO_PhysicsState& _PhysState, const CVec3Dfp32& _Pos)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetPhysics_2, false);
	MSCOPESHORT(CWorld_PhysState::Object_SetPhysics_2);
	DEBUG_CHECK_VECTOR(_Pos);

	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj)
	{
		PhysError("Object_SetPhysics", "NULL Object.");
		return false;
	}
	CMat4Dfp32 Mat = pObj->GetPositionMatrix();
	_Pos.SetMatrixRow(Mat, 3);
	if(_PhysState.m_nPrim > 0)
	{
		if (Phys_IntersectWorld((CSelection*) NULL, _PhysState, Mat, Mat, _iObj)) 
			return false;
	}

	DEBUG_CHECK_MATRIX(Mat);
	pObj->m_PhysState = _PhysState;
	pObj->m_LocalPos = Mat;
	Phys_InsertPosition(_iObj, pObj);

	return true;
}


bool CWorld_PhysState::Object_SetPhysics(int _iObj, const CWO_PhysicsState& _PhysState)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetPhysics_3, false);
	MSCOPESHORT(CWorld_PhysState::Object_SetPhysics_3);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj)
	{
		PhysError("Object_SetPhysics", "NULL Object.");
		return false;
	}
	if(_PhysState.m_nPrim > 0)
	{
		if (Phys_IntersectWorld((CSelection*) NULL, _PhysState, pObj->m_Pos, pObj->m_Pos, _iObj)) 
			return false;
	}

	CVec3Dfp32 p = CVec3Dfp32::GetMatrixRow(pObj->m_Pos, 3);
	pObj->m_PhysState = _PhysState;
	Phys_InsertPosition(_iObj, pObj);

	return true;
}

bool CWorld_PhysState::Object_SetPhysics_ObjectFlags(int _iObj, uint _ObjectFlags)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetPhysics_ObjectFlags, false);
	MSCOPESHORT(CWorld_PhysState::Object_SetPhysics_ObjectFlags);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj)
	{
		PhysError("Object_SetPhysics", "NULL Object.");
		return false;
	}

	if(pObj->m_PhysState.m_nPrim > 0)
	{
		CWO_PhysicsState PhysState = pObj->m_PhysState;
		PhysState.m_ObjectFlags = _ObjectFlags;

		if (Phys_IntersectWorld((CSelection*) NULL, PhysState, pObj->m_Pos, pObj->m_Pos, _iObj)) 
			return false;
	}

	CVec3Dfp32 p = CVec3Dfp32::GetMatrixRow(pObj->m_Pos, 3);
	pObj->m_PhysState.m_ObjectFlags = _ObjectFlags;
	Phys_InsertPosition(_iObj, pObj);

	return true;
}


bool CWorld_PhysState::Object_SetPhysics_DoNotify(int _iObj, const CWO_PhysicsState& _PhysState, const CMat4Dfp32& _Pos)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetPhysics, false);
	Error("Object_SetPhysics_DoNotify", "Invalid call.");
	return false;
}


bool CWorld_PhysState::Object_SetPosition(int _iObj, const CVec3Dfp32& _Pos)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetPosition, false);
	DEBUG_CHECK_VECTOR(_Pos);

	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj)
	{
		PhysError("Object_SetPosition", "NULL Object.");
		return false;
	}
	CMat4Dfp32 Mat = pObj->GetPositionMatrix();
	_Pos.SetMatrixRow(Mat, 3);
	if(pObj->m_PhysState.m_nPrim > 0)
	{
		if (Phys_IntersectWorld((CSelection*) NULL, pObj->m_PhysState, Mat, Mat, _iObj))
			return false;
	}

	DEBUG_CHECK_MATRIX(Mat);
	pObj->m_LocalPos = Mat;
	Phys_InsertPosition(_iObj, pObj);

#if !defined(DYNAMICSENGINE2) && defined(DYNAMICS_KEEP_STATE)
	if (pObj->m_pRigidBody)
		pObj->m_pRigidBody->SetPosition(_Pos.k[0], _Pos.k[1], _Pos.k[2]);
#endif

	return true;
}


bool CWorld_PhysState::Object_SetPosition(int _iObj, const CMat4Dfp32& _Pos)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetPosition_2, false);
	DEBUG_CHECK_VECTOR(_Pos.GetRow(3));

	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj)
	{
		PhysError("Object_SetPosition", "NULL Object.");
		return false;
	}
	if(pObj->m_PhysState.m_nPrim > 0)
	{
		if (Phys_IntersectWorld((CSelection*) NULL, pObj->m_PhysState, _Pos, _Pos, _iObj))
			return false;
	}

	DEBUG_CHECK_MATRIX(_Pos);
	CVec3Dfp32 p = CVec3Dfp32::GetMatrixRow(_Pos, 3);
	pObj->m_LocalPos = _Pos;
	Phys_InsertPosition(_iObj, pObj);

#if !defined(DYNAMICSENGINE2) && defined(DYNAMICS_KEEP_STATE)
	if (pObj->m_pRigidBody)
	{
		const CVec3Dfp32& tmp = CVec3Dfp32::GetRow(_Pos,3);
		pObj->m_pRigidBody->SetPosition(tmp.k[0], tmp.k[1], tmp.k[2]);
	}
#endif

	return true;
}


void CWorld_PhysState::Object_SetPositionNoIntersection(int _iObj, const CMat4Dfp32& _WorldPos)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetPositionNoIntersection, false);
	DEBUG_CHECK_VECTOR(_WorldPos.GetRow(3));

	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj)
	{
		PhysError("Object_SetPositionNoIntersection", "NULL Object.");
		return;
	}

	DEBUG_CHECK_MATRIX(_WorldPos);
	CWObject_CoreData* pParent = (pObj->m_iObjectParent > 0) ? Object_GetCD(pObj->m_iObjectParent) : NULL;
	if (pParent)
	{
		CMat4Dfp32 InvParentMat;
		pParent->GetPositionMatrix().InverseOrthogonal(InvParentMat);
		M_VMatMul(_WorldPos, InvParentMat, pObj->m_LocalPos);
	}
	else
	{
		pObj->m_LocalPos = _WorldPos;
	}

	Phys_InsertPosition(_iObj, pObj);
}

void CWorld_PhysState::Object_SetPositionNoIntersection(int _iObj, const CVec3Dfp32& _WorldPos)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetPositionNoIntersection, false);
	DEBUG_CHECK_VECTOR(_WorldPos);

	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj)
	{
		PhysError("Object_SetPositionNoIntersection", "NULL Object.");
		return;
	}

	CWObject_CoreData* pParent = (pObj->m_iObjectParent > 0) ? Object_GetCD(pObj->m_iObjectParent) : NULL;
	if (pParent)
	{
		CMat4Dfp32 InvParentMat,Temp;
		pParent->GetPositionMatrix().InverseOrthogonal(InvParentMat);
		Temp = pObj->m_LocalPos;
		_WorldPos.SetRow(Temp,3);
		M_VMatMul(Temp, InvParentMat, pObj->m_LocalPos);
	}
	else
	{
		_WorldPos.SetRow(pObj->m_LocalPos,3);
	}

	Phys_InsertPosition(_iObj, pObj);
}


bool CWorld_PhysState::Object_SetPosition(int _iObj, const CVec3Dfp32& _Pos, const CVec3Dfp32 _Angles, int _AnglePriority)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetPosition_3, false);
	DEBUG_CHECK_VECTOR(_Pos);

	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj)
	{
		PhysError("Object_SetPosition", "NULL Object.");
		return false;
	}
	CMat4Dfp32 Mat;
	CVec3Dfp32 v(_Angles);
	v.CreateMatrixFromAngles(_AnglePriority, Mat);
	_Pos.SetMatrixRow(Mat, 3);
	if(pObj->m_PhysState.m_nPrim > 0)
	{
		if (Phys_IntersectWorld((CSelection*) NULL, pObj->m_PhysState, Mat, Mat, _iObj))
			return false;
	}

	DEBUG_CHECK_MATRIX(Mat);
	pObj->m_LocalPos = Mat;
	Phys_InsertPosition(_iObj, pObj);

#if !defined(DYNAMICSENGINE2) && defined(DYNAMICS_KEEP_STATE)
	if (pObj->m_pRigidBody)
		pObj->m_pRigidBody->SetPosition(_Pos.k[0], _Pos.k[1], _Pos.k[2]);
#endif

	return true;
}


bool CWorld_PhysState::Object_SetRotation(int _iObj, const CMat4Dfp32& _Pos)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetRotation, false);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj)
	{
		PhysError("Object_SetRotation", "NULL Object.");
		return false;
	}
	DEBUG_CHECK_MATRIX(_Pos);
	CMat4Dfp32 Pos = _Pos;
	CVec3Dfp32::GetMatrixRow(Pos, 3) = pObj->GetLocalPosition();
	if (!pObj->GetParent())
	{
		if (pObj->m_PhysState.m_PhysFlags & OBJECT_PHYSFLAGS_ROTATION)
		{
			if(pObj->m_PhysState.m_nPrim > 0)
			{
				if (Phys_IntersectWorld((CSelection*) NULL, pObj->m_PhysState, Pos, Pos, _iObj))
					return false;
			}
		}
		pObj->m_LocalPos = Pos;
		Phys_InsertPosition(_iObj, pObj);
	}
	else
	{
		pObj->m_LocalPos = Pos;
		Phys_InsertPosition(_iObj, pObj);
	}
	return true;
}


bool CWorld_PhysState::Object_SetPosition_World(int _iObj, const CMat4Dfp32& _Pos)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetPosition_World, false);
	DEBUG_CHECK_VECTOR(_Pos.GetRow(3));

	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj)
	{
		PhysError("Object_SetPosition", "NULL Object.");
		return false;
	}

	if(pObj->m_PhysState.m_nPrim > 0)
	{
		if (Phys_IntersectWorld((CSelection*) NULL, pObj->m_PhysState, _Pos, _Pos, _iObj))
			return false;
	}
	
	if (pObj->GetParent())
	{
		CWObject_CoreData* pParent = Object_GetCD(pObj->GetParent());
		if (!pParent)
			return false;

		CMat4Dfp32 Local;
		CMat4Dfp32 ParentInv;
		pParent->GetPositionMatrix().InverseOrthogonal(ParentInv);
		_Pos.Multiply(ParentInv, Local);
		DEBUG_CHECK_MATRIX(Local);
		pObj->m_LocalPos = Local;
		Phys_InsertPosition(_iObj, pObj);
	}
	else
	{
		CVec3Dfp32 p = CVec3Dfp32::GetMatrixRow(_Pos, 3);
		DEBUG_CHECK_MATRIX(_Pos);
		pObj->m_LocalPos = _Pos;
		Phys_InsertPosition(_iObj, pObj);
	}
	return true;
}


void CWorld_PhysState::Object_SetVisBox(int _iObj, const CVec3Dfp32& _Min, const CVec3Dfp32& _Max)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetVisBox, MAUTOSTRIP_VOID);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
	{
		CBox3Dfp32 Box(_Min, _Max);
		pObj->SetVisBoundBox(Box);
		Phys_InsertPosition(_iObj, pObj);
	}
}

void CWorld_PhysState::Object_SetVelocity(int _iObj, const CVelocityfp32& _Velocity)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetVelocity, MAUTOSTRIP_VOID);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
	{
		DEBUG_CHECK_VECTOR(_Velocity.m_Move);
		pObj->m_PhysVelocity = _Velocity;

#if !defined(DYNAMICSENGINE2) && defined(DYNAMICS_KEEP_STATE)
		CRigidBody* pRB = pObj->m_pRigidBody;
		if (pRB)
		{ // Update rigid body state
			fp32 TicksPerSec = GetGameTicksPerSecond();

			CVec3Dfp32 Vel = _Velocity.m_Move;
			Vel *= (1.0f / 32.0f) * TicksPerSec; // [m/s]
			pRB->SetVelocity(Vel.Get<fp64>());

			CVec3Dfp32 AngVel = _Velocity.m_Rot.m_Axis;
			AngVel *= _Velocity.m_Rot.m_Angle * (-_PI2 * TicksPerSec); // [rad/s]
			pRB->SetAngularVelocity(AngVel.Get<fp64>());
		}
#endif
	}
}

void CWorld_PhysState::Object_SetVelocity(int _iObj, const CVec3Dfp32& _Velocity)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetVelocity_2, MAUTOSTRIP_VOID);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
	{
		DEBUG_CHECK_VECTOR(_Velocity);
		pObj->m_PhysVelocity.m_Move = _Velocity;
	}
}

void CWorld_PhysState::Object_AddVelocity(int _iObj, const CVec3Dfp32& _dVelocity)
{
	MAUTOSTRIP(CWorld_PhysState_Object_AddVelocity, MAUTOSTRIP_VOID);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
	{
		DEBUG_CHECK_VECTOR(_dVelocity);
		pObj->m_PhysVelocity.m_Move += _dVelocity;
	}
}

void CWorld_PhysState::Object_SetRotVelocity(int _iObj, const CAxisRotfp32& _Rot)
{
	MAUTOSTRIP(CWorld_PhysState_Object_SetRotVelocity, MAUTOSTRIP_VOID);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
	{
		pObj->m_PhysVelocity.m_Rot = _Rot;
	}
}

void CWorld_PhysState::Object_AddRotVelocity(int _iObj, const CAxisRotfp32& _dRot)
{
	MAUTOSTRIP(CWorld_PhysState_Object_AddRotVelocity, MAUTOSTRIP_VOID);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
	{
/*		CQuatfp32 Q1,Q2,Q3;
		_dRot.CreateQuaternion(Q1);
		pObj->m_PhysVelocity.m_Rot.CreateQuaternion(Q2);
		Q1.Multiply(Q2, Q3);
		pObj->m_PhysVelocity.m_Rot.Create(Q3);*/

/*		CAxisRotfp32 Tmp(_dRot);
		Tmp.Multiply(pObj->m_PhysVelocity.m_Rot);
		pObj->m_PhysVelocity.m_Rot = Tmp;*/

//		pObj->m_PhysVelocity.m_Rot.Multiply(_dRot);
		CMat4Dfp32 VMat, dVMat,Res;
		Res.Unit();
		_dRot.CreateMatrix(dVMat);
		pObj->GetVelocityMatrix(VMat);
		dVMat.Multiply3x3(VMat, Res);
		pObj->m_PhysVelocity.m_Rot.Create(Res);
	}
}

void CWorld_PhysState::Object_AccelerateTo(int _iObj, const CVec3Dfp32& _Velocity, const CVec3Dfp32& _Acceleration)
{
	MAUTOSTRIP(CWorld_PhysState_Object_AccelerateTo, MAUTOSTRIP_VOID);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
	{
		CVec3Dfp32 dV;
		_Velocity.Sub(pObj->GetMoveVelocity(), dV);
		if (_Acceleration.k[0] == 0.0f) dV.k[0] = 0;
		if (_Acceleration.k[1] == 0.0f) dV.k[1] = 0;
		if (_Acceleration.k[2] == 0.0f) dV.k[2] = 0;

		const fp32 l = dV.Length();
		if (l <= 0.0f) return;
		fp32 a = _Acceleration.Length();
		if (a > l) a = l;

		for(int i = 0; i < 3; i++)
			if (_Acceleration.k[i] != 0.0f)
				pObj->m_PhysVelocity.m_Move[i] += a*dV.k[i] / l;


/*		for(int i = 0; i < 3; i++)
		{
			if (pObj->m_PhysVelocity.k[i] < _Velocity.k[i])
				pObj->m_PhysVelocity.k[i] = Min(pObj->m_PhysVelocity.k[i] + _Acceleration.k[i], _Velocity.k[i]);
			else
				pObj->m_PhysVelocity.k[i] = Max(pObj->m_PhysVelocity.k[i] - _Acceleration.k[i], _Velocity.k[i]);
		}*/
	}
}

void CWorld_PhysState::Object_DisableLinkage(int _iObj)
{
	MAUTOSTRIP(CWorld_PhysState_Object_DisableLinkage, MAUTOSTRIP_VOID);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
	{
		pObj->m_ClientFlags |= CWO_CLIENTFLAGS_NOHASH;
	}

	m_iObject_DisabledLinkage = _iObj;
}

void CWorld_PhysState::Object_EnableLinkage(int _iObj)
{
	MAUTOSTRIP(CWorld_PhysState_Object_EnableLinkage, MAUTOSTRIP_VOID);
	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (pObj)
	{
		if (!(pObj->m_ClientFlags & CWO_CLIENTFLAGS_NOHASH)) return;

		pObj->m_ClientFlags &= ~CWO_CLIENTFLAGS_NOHASH;
		if (pObj->m_ClientFlags & CWO_CLIENTFLAGS_HASHDIRTY)
		{
			M_ASSERT(m_spSpaceEnum != NULL, "!");
			m_spSpaceEnum->Insert(pObj);

			pObj->m_ClientFlags &= ~CWO_CLIENTFLAGS_HASHDIRTY;
		}
	}

	m_iObject_DisabledLinkage = 0;
}

// -------------------------------------------------------------------
bool CWorld_PhysState::Object_IntersectLine(int _iObj, const CVec3Dfp32& _p0, const CVec3Dfp32& _p1, int _ObjectFlags, int _ObjectIntersectFlags, int _MediumFlags, CCollisionInfo* _pCollisionInfo)
{
	MAUTOSTRIP(CWorld_PhysState_Object_IntersectLine, false);
	MSCOPESHORT(CWorld_PhysState::Object_IntersectLine);

	// _pCollisionInfo must be cleared.

	CWObject_CoreData* pObj = Object_GetCD(_iObj);
	if (!pObj) return false;

	if (!((pObj->m_PhysState.m_ObjectFlags & _ObjectIntersectFlags) ||
		  (pObj->m_PhysState.m_ObjectIntersectFlags & _ObjectFlags))) return false;

	CWO_OnIntersectLineContext Ctx;
	Ctx.m_pObj = pObj;
	Ctx.m_pPhysState = this;
	Ctx.m_p0 = _p0;
	Ctx.m_p1 = _p1;
	Ctx.m_ObjectFlags = _ObjectFlags;
	Ctx.m_ObjectIntersectionFlags = _ObjectIntersectFlags;
	Ctx.m_MediumFlags = _MediumFlags;

	if (_pCollisionInfo)
	{
		CCollisionInfo CInfo;
		CInfo.CopyParams(*_pCollisionInfo);

		if (pObj->m_pRTC->m_pfnOnIntersectLine(Ctx, &CInfo))
		{
			if (_pCollisionInfo->Improve(CInfo))
			{
				_pCollisionInfo->m_iObject = _iObj;
#ifdef _DEBUG
				if (_pCollisionInfo->m_bIsValid && (_pCollisionInfo->m_Time < -0.01f || _pCollisionInfo->m_Time > 1.01f))
				{
					ConOut(CStrF("(CWorld_PhysState::Object_IntersectLine) Entity %d OnIntersectLine returned T = %f", _iObj, _pCollisionInfo->m_Time));
				}
#endif
			}
			return true;
		}
		return false;
	}
	else
	{
		return pObj->m_pRTC->m_pfnOnIntersectLine(Ctx, NULL);
	}
}

// -------------------------------------------------------------------
//  Object hierarchy
// -------------------------------------------------------------------
bool CWorld_PhysState::Object_AddChild(int _iParent, int _iChild)
{
	MAUTOSTRIP(CWorld_PhysState_Object_AddChild, false);
	// Makes iChild a child if iParent, if _iParent is zero, _iChild is unconnected from any parent it might have.
	// Returns false if:
	//		- the child cannot be placed, or 
	//		- Parent is a child of _iChild, or
	//		- Parent is not a valid object
	//		- Child is not a valid object
	//		- Child and parent are the same object.

	if (_iParent == _iChild) return false;
	if (Object_IsAncectorOf(_iChild, _iParent)) return false;

	CWObject_CoreData* pParent = Object_GetCD(_iParent);
	if (_iParent && !pParent) return false;
	CWObject_CoreData* pChild = Object_GetCD(_iChild);
	if (!pChild) return false;

	// Same as before?
	if (pChild->GetParent() == _iParent) return true;

	// Unlink from current parent
	if (pChild->GetParent())
	{
		// Unlink pChild
		if (pChild->m_iObjectChildPrev)
		{
			CWObject_CoreData* pPrev = Object_GetCD(pChild->m_iObjectChildPrev);
			if(pPrev)
				pPrev->m_iObjectChildNext = pChild->m_iObjectChildNext;
		}
		else
		{
			CWObject_CoreData* pParent = Object_GetCD(pChild->m_iObjectParent);
			if(pParent)
				pParent->m_iObjectChild = pChild->m_iObjectChildNext;
		}

		if (pChild->m_iObjectChildNext)
		{
			CWObject_CoreData* pNext = Object_GetCD(pChild->m_iObjectChildNext);
			if(pNext)
				pNext->m_iObjectChildPrev = pChild->m_iObjectChildPrev;
		}

		pChild->m_iObjectChildNext = 0;
		pChild->m_iObjectChildPrev = 0;
		pChild->m_iObjectParent = 0;

		DEBUG_CHECK_MATRIX(pChild->m_Pos);
		pChild->m_LocalPos = pChild->m_Pos;
		Phys_InsertPosition(pChild->m_iObject, pChild);
	}

	// Link to new parent.
	if (pParent)
	{
		pChild->m_iObjectChildNext = pParent->m_iObjectChild;
		if (pParent->m_iObjectChild)
		{
			CWObject_CoreData* pChild = Object_GetCD(pParent->m_iObjectChild);
			if(pChild)
				pChild->m_iObjectChildPrev = _iChild;
		}
		pParent->m_iObjectChild = _iChild;
		pChild->m_iObjectParent = _iParent;

		if (pChild->m_ClientFlags & CWO_CLIENTFLAGS_NOROTINHERITANCE)
		{
			DEBUG_CHECK_MATRIX(pChild->m_Pos);
			pChild->m_LocalPos = pChild->m_Pos;
			pChild->m_LocalPos.GetRow(3) -= pParent->GetPosition();
		}
		else
		{
			CMat4Dfp32 ParentInv;
			DEBUG_CHECK_MATRIX(pParent->GetPositionMatrix());
			DEBUG_CHECK_MATRIX(pChild->GetPositionMatrix());
			pParent->GetPositionMatrix().InverseOrthogonal(ParentInv);
			pChild->GetPositionMatrix().Multiply(ParentInv, pChild->m_LocalPos);
		}

		Phys_InsertPosition(pChild->m_iObject, pChild);
//		Object_SetDirtyTree(pParent->m_iObject, CWO_DIRTYMASK_HIERARCHY);

		// If an object is part of a hierarchy it must be replicated.
		pParent->m_ClientFlags &= ~CWO_CLIENTFLAGS_NOUPDATE;
		pChild->m_ClientFlags &= ~CWO_CLIENTFLAGS_NOUPDATE;
	}

	// Todo: Update object-positions, set dirty-masks

	return true;
}

bool CWorld_PhysState::Object_RemoveChild(int _iParent, int _iChild)
{
	MAUTOSTRIP(CWorld_PhysState_Object_RemoveChild, false);
	if (Object_IsParentOf(_iParent, _iChild))
	{
		Object_AddChild(0, _iChild);
		return true;
	}
	else
		return false;
}

int CWorld_PhysState::Object_GetParent(int _iObj)
{
	MAUTOSTRIP(CWorld_PhysState_Object_GetParent, 0);
	CWObject_CoreData* pChild = Object_GetCD(_iObj);
	if (!pChild) return 0;
	return pChild->GetParent();
}

int CWorld_PhysState::Object_GetFirstChild(int _iObj)
{
	MAUTOSTRIP(CWorld_PhysState_Object_GetFirstChild, 0);
	CWObject_CoreData* pChild = Object_GetCD(_iObj);
	if (!pChild) return 0;
	return pChild->GetFirstChild();
}

int CWorld_PhysState::Object_GetPrevChild(int _iObj)
{
	MAUTOSTRIP(CWorld_PhysState_Object_GetPrevChild, 0);
	CWObject_CoreData* pChild = Object_GetCD(_iObj);
	if (!pChild) return 0;
	return pChild->GetPrevChild();
}

int CWorld_PhysState::Object_GetNextChild(int _iObj)
{
	MAUTOSTRIP(CWorld_PhysState_Object_GetNextChild, 0);
	CWObject_CoreData* pChild = Object_GetCD(_iObj);
	if (!pChild) return 0;
	return pChild->GetNextChild();
}

int CWorld_PhysState::Object_GetNumChildren(int _iObj, bool _bRecursive)
{
	MAUTOSTRIP(CWorld_PhysState_Object_GetNumChildren, 0);
	CWObject_CoreData* pChild = Object_GetCD(_iObj);
	if (!pChild) return 0;
	int NumChild = 0;
	int iChild = pChild->GetFirstChild();
	while(iChild)
	{
		NumChild++;
		if(_bRecursive)
		{
			NumChild += Object_GetNumChildren(iChild, _bRecursive);
		}
		pChild = Object_GetCD(iChild);
		if (pChild)
			iChild = pChild->GetNextChild();
		else
			iChild = 0;
	}
	return NumChild;
}

int CWorld_PhysState::Object_IsParentOf(int _iParent, int _iObj)
{
	MAUTOSTRIP(CWorld_PhysState_Object_IsParentOf, 0);
	CWObject_CoreData* pChild = Object_GetCD(_iObj);
	if (!pChild) return 0;
	return pChild->GetParent() == _iParent;
}

int CWorld_PhysState::Object_IsAncectorOf(int _iAnc, int _iObj)
{
	MAUTOSTRIP(CWorld_PhysState_Object_IsAncectorOf, 0);
	// TODO: Implement
	return 0;
}


/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯| 
| Object event listener management
|____________________________________________________________________________________|
\************************************************************************************/
void CWorld_PhysState::Object_AddListener(uint _iObject, uint _iListener, uint16 _EventMask)
{
	M_ASSERT(IsServer(), "Currently only supported on server");
//	M_ASSERT(_iListener != _iObject, "Can't listen to yourself!");
	m_Listeners.AddOrUpdateLink(_iObject, _iListener, _EventMask);
}

void CWorld_PhysState::Object_RemoveListener(uint _iObject, uint _iListener, uint16 _EventMaskToRemove)
{
	M_ASSERT(IsServer(), "Currently only supported on server");
//	M_ASSERT(_iListener != _iObject, "Can't listen to yourself!");
	m_Listeners.RemoveOrUpdateLink(_iObject, _iListener, _EventMaskToRemove);
}

bool CWorld_PhysState::Object_HasListeners(uint _iObject) const
{
	// (This is pretty stupid. I would rather use a ClientFlag for this)
	return (m_Listeners.m_pLinks.m_pArray) && (m_Listeners.m_piHash[0][_iObject] != 0);
}

