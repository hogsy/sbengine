
#ifndef WDYNAMICSENGINE2_H
#define WDYNAMICSENGINE2_H

#include "WDynamicsEngineBase.h"

struct CVector4Pair
{
	CVector4Pair() {}


	CVector4Pair(vec128 _a, vec128 _b) 
	{
		a = _a;
		b = _b;
	}

	CVector4Pair(CVec4Dfp32& _a, CVec4Dfp32& _b) 
	{
		a = _a;
		b = _b;
	}

	CVec4Dfp32 a,b;
};

//#include "WDynamicsEngineLCP.h"

//#define WDYNAMICSENGINE_PEDANTIC
#define WDYNAMICSENGINE_DEBUG

#define CWD_DYNAMICS_MAX_COLLISIONS (1000)
#define CWD_DYNAMICS_MAX_CONSTRAINT_COLLISIONS (1000)


#ifdef WDYNAMICSENGINE_PEDANTIC
	M_FORCEINLINE void WDYNAMICS_CHECK_VEC128(vec128 _v)
	{
		CVec4Dfp32 v = _v;
		if (!(v.k[0] == v.k[0] && v.k[1] == v.k[1] && v.k[2] == v.k[2] && v.k[3] == v.k[3]))
			M_BREAKPOINT;
	}
#else
#	define WDYNAMICS_CHECK_VEC128(_v) 
#endif



class CVectorUtil
{
public:

	static void M_FORCEINLINE QuatToMatrix(const CVec4Dfp32& _Quat, CMat4Dfp32& _Mat)
	{
		fp32 xs,ys,zs,wx,wy,wz,xx,xy,xz,yy,yz,zz;
		fp32 s = fp32(2.0f);

		xs = _Quat.k[0]*s;  ys = _Quat.k[1]*s;  zs = _Quat.k[2]*s;
		wx = _Quat.k[3]*xs; wy = _Quat.k[3]*ys; wz = _Quat.k[3]*zs;
		xx = _Quat.k[0]*xs; xy = _Quat.k[0]*ys; xz = _Quat.k[0]*zs;
		yy = _Quat.k[1]*ys; yz = _Quat.k[1]*zs; zz = _Quat.k[2]*zs;

		_Mat.k[0][0] = (fp32(1.0f) - (yy+zz));
		_Mat.k[0][1] = (xy-wz);
		_Mat.k[0][2] = (xz+wy);

		_Mat.k[1][0] = (xy+wz);
		_Mat.k[1][1] = (fp32(1.0f) - (xx+zz));
		_Mat.k[1][2] = (yz-wx);

		_Mat.k[2][0] = (xz-wy);
		_Mat.k[2][1] = (yz+wx);
		_Mat.k[2][2] = (fp32(1.0f) - (xx+yy));

		_Mat.Transpose();
	}
};


class CWD_CollisionEvent
{
public:
	CWD_CollisionEvent()
	{
		m_ImpulseTimesInvMass = 0.0f;
		m_PointOfCollision = CVec3Dfp32(0.0f);
		m_Velocity1 = m_Velocity2 = CVec3Dfp32(0.0f);
		m_iRB1 = -1;
		m_iRB2 = -1;
		m_UserData1 = 0;
		m_UserData2 = 0;
	}

	CWD_CollisionEvent(fp32 _Impulse, const CVec3Dfp32 _PointOfCollision)
	{
		m_ImpulseTimesInvMass = _Impulse;
		m_PointOfCollision = _PointOfCollision;
		m_Velocity1 = m_Velocity2 = CVec3Dfp32(0.0f); // set in CollectCollisionEvents
		m_iRB1 = -1;
		m_iRB2 = -1;
	}

	fp32 m_ImpulseTimesInvMass;
	CVec3Dfp32 m_PointOfCollision;
	CVec3Dfp32 m_Velocity1;
	CVec3Dfp32 m_Velocity2;
	int m_iRB1, m_iRB2;
	int m_UserData1, m_UserData2;
};

class CWD_Inertia
{
public:
	static CVec3Dfp32 Block(fp32 _Mass, const CVec3Dfp32& _Dim);
	static CVec3Dfp32 Block(fp32 _Mass, fp32 _x, fp32 _y, fp32 _z);
};

#define CNORMALCLUSTER_N_DIRECTIONS (3)
class CNormalCluster 
{
public:

	void Clear() 
	{
		CVec4Dfp32 Z(0.0f);
		for (int i = 0; i < CNORMALCLUSTER_N_DIRECTIONS; i++)
		{
			m_Normals[i] = Z;
			m_nNormals[i] = 0;
		}
	}

	void AddNormal(const CVec4Dfp32& _Normal)
	{
		int iBest = 0;
		for (int i = 0; i < CNORMALCLUSTER_N_DIRECTIONS; i++)
		{
			fp32 d = _Normal * m_Normals[i];

			if (m_nNormals[i] == 0 || d > 0.5f)
			{
				iBest = i;
				break;
			}
		}

		int N = m_nNormals[iBest];
		if (N > 0)
			m_Normals[iBest] = (m_Normals[iBest] + _Normal) * 0.5f;
		else
			m_Normals[iBest] = _Normal;

		m_nNormals[iBest] = N + 1;
	}

	M_FORCEINLINE const CVec4Dfp32& GetNormal(int _i)
	{
		M_ASSERT(_i < CNORMALCLUSTER_N_DIRECTIONS, "!");
		return m_Normals[_i];
	}

//protected:
	CVec4Dfp32 m_Normals[CNORMALCLUSTER_N_DIRECTIONS];
	int m_nNormals[CNORMALCLUSTER_N_DIRECTIONS];

};

#define CWD_RIGIDBODY2_MAX_CONNECTIONS (8)
class CWD_RigidBody2
{
public:
	CWD_RigidBody2();

	M_INLINE void SetActive(bool _Active)
	{
		m_bActive = _Active;
	}

	M_INLINE bool IsActive(bool _Active)
	{
		return m_bActive;
	}

	M_INLINE void AddConnection(int _iRB)
	{
		if (m_lConnectedTo.Len() >= CWD_RIGIDBODY2_MAX_CONNECTIONS) return;

		if (IsConnectedTo(_iRB)) return;

		m_lConnectedTo.Add(_iRB);
	}

	M_INLINE bool IsConnectedTo(int _iRB)
	{
		for (int i = 0; i < m_lConnectedTo.Len(); i++)
		{
			if (m_lConnectedTo[i] == _iRB) return true;
		}

		return false;
	}

	M_INLINE void ClearConnections()
	{
		m_lConnectedTo.SetLen(0);
	}

	M_INLINE fp32 GetMass()
	{
		return m_Mass;
	}

	M_FORCEINLINE void SetStationary(int _Stat)
	{
		m_bStationary = _Stat;
	}

	M_FORCEINLINE int IsStationary() const
	{
		return m_bStationary;
	}

	fp32 m_Mass;
	CVec3Dfp32 m_InertiaTensor;
	CVec3Dfp32 m_CenterOfMass;

	uint32 m_FreezeCounter : 28;
	uint32 m_bActive : 1;
	uint32 m_bInSimulation : 1;
	uint32 m_bUseOriginalFreezeThreshold : 1;	//Used to avoid using the lamp freeze threshold hack

	fp32 m_StaticFriction;
	fp32 m_DynamicFriction;
	fp32 m_CoefficientOfRestitution;

	uint32 m_iRB;
	CVec4Dfp32 m_ExternalForce, m_ExternalTorque;
	void *m_pUserData;
	void *m_pUserData2;

	bool m_bVisited;
	//CDoubleLinkable<CWD_RigidBody2 *> m_lNeighbours;
	TDisjointSet<CWD_RigidBody2 *>::Node m_ContactGraphNode;

	bool m_bDebris;

	CNormalCluster m_CollisionToWorldCluster;

	TStaticArray<int32, CWD_RIGIDBODY2_MAX_CONNECTIONS> m_lConnectedTo;

	friend class CWD_DynamicsWorld;

protected:
	uint32 m_bStationary : 1;

};

class CWD_RigidBodyState
{
public:

	CWD_RigidBodyState()
	{
#ifdef WDYNAMICSENGINE_PEDANTIC
		CVec3Dfp32 V(TNumericProperties<fp32>::QuietNaN());

		m_Position = V;
		m_Orientation = V;
		m_Force = V;
		m_Torque = V;
		m_Velocity = V;
		m_AngularVelocity = V;
		m_TensorInverted = V;
		m_Mass = V;
		m_MassInverted = V;

		for (int i = 0; i < 4; i++)
		{
			m_WorldInertiaTensorInvert.GetRow(i) = V;
			m_MatrixOrientation.GetRow(i) = V;
		}
#endif
	}

//#define DYNAMICS_CACHE_TRANSFORM

	M_INLINE void GetTransform(CMat4Dfp32& _T) const
	{
#ifndef DYNAMICS_CACHE_TRANSFORM
		CVectorUtil::QuatToMatrix(m_Orientation, _T);
		_T.k[0][3] = 0.0f;
		_T.k[1][3] = 0.0f;
		_T.k[2][3] = 0.0f;
		_T.r[3] = m_Position;
#else
		_T = m_MatrixOrientation;
		_T.r[3] = m_Position;

#endif
	}

	M_INLINE CMat4Dfp32 GetTransform() const
	{
		// TODO: Cache?
		// Antingen får man lagra den i CWD_DynamicsWorld eller i denna
		// om det visar sig att denna kostar mycket. Lagras den i CWD_DynamicsWorld
		// måste detta anrop bort. 

#ifndef DYNAMICS_CACHE_TRANSFORM
		CMat4Dfp32 T;
		CVectorUtil::QuatToMatrix(m_Orientation, T);
		T.k[0][3] = 0.0f;
		T.k[1][3] = 0.0f;
		T.k[2][3] = 0.0f;
		T.r[3] = m_Position;
		T.k[3][3] = 1.0f;
		return T;
#else
		CMat4Dfp32 T;
		T.k[0][0] = m_MatrixOrientation.k[0][0];
		T.k[0][1] = m_MatrixOrientation.k[0][1];
		T.k[0][2] = m_MatrixOrientation.k[0][2];
		T.k[0][3] = 0.0f;

		T.k[1][0] = m_MatrixOrientation.k[1][0];
		T.k[1][1] = m_MatrixOrientation.k[1][1];
		T.k[1][2] = m_MatrixOrientation.k[1][2];
		T.k[1][3] = 0.0f;

		T.k[2][0] = m_MatrixOrientation.k[2][0];
		T.k[2][1] = m_MatrixOrientation.k[2][1];
		T.k[2][2] = m_MatrixOrientation.k[2][2];
		T.k[2][3] = 0.0f;

		T.k[3][0] = m_Position[0];
		T.k[3][1] = m_Position[1];
		T.k[3][2] = m_Position[2];
		T.k[3][3] = 1.0f;

		return T;

#endif
	}

	void Clear()
	{
		CVec4Dfp32 Zero(0.0f);
		CVec4Dfp32 One(1.0f);
		CVec4Dfp32 UnitQ = CVec4Dfp32(0.0f, 0.0f, 0.0f, 1.0f);

		m_Position = CVec4Dfp32(0.0f, 0.0f, 0.0f, 1.0f);
		m_Orientation = UnitQ;
		m_Force = Zero;
		m_Torque = Zero;
		m_Velocity = Zero;
		m_AngularVelocity = Zero;
		m_TensorInverted = One;
		m_Mass = One;
		m_MassInverted = One;

		m_WorldInertiaTensorInvert.Unit();
		m_MatrixOrientation.Unit();

		m_FreezeCounter = 0;
		m_bActive = true;
		m_bStationary = false;
	}

	void Create(const CVec3Dfp32& _Position, const CQuatfp32& _Orientation, fp32 _Mass, const CVec3Dfp32 _Tensor)
	{
		m_Position = To4D(_Position);
		m_Orientation = QuatToVec4(_Orientation);
/*		m_Orientation.k[0] = _Orientation.k[0];
		m_Orientation.k[1] = _Orientation.k[1];
		m_Orientation.k[2] = _Orientation.k[2];
		m_Orientation.k[3] = _Orientation.k[3];*/
		m_Mass = _Mass;
		m_MassInverted = 1.0f / _Mass;
		m_TensorInverted = To4D(CVec3Dfp32(1.0f / _Tensor[0], 1.0f / _Tensor[1], 1.0f / _Tensor[2]));
		WDYNAMICS_CHECK_VEC128(m_TensorInverted);

		UpdateOrientationMatrix();
		UpdateWorldInertiaTensorInvert();
	}

	void Create(const CVec3Dfp32& _Position, const CQuatfp32& _Orientation, const CVec3Dfp32& _Velocity, const CVec3Dfp32& _AngularVelocity, fp32 _Mass, const CVec3Dfp32 _Tensor)
	{
		m_Position = To4D(_Position);
		m_Orientation = QuatToVec4(_Orientation);
		m_Velocity = To4D_LastZero(_Velocity);
		m_AngularVelocity = To4D_LastZero(_AngularVelocity);
		m_Mass = _Mass;
		m_MassInverted = 1.0f / _Mass;
		m_TensorInverted = To4D(CVec3Dfp32(1.0f / _Tensor[0], 1.0f / _Tensor[1], 1.0f / _Tensor[2]));
		WDYNAMICS_CHECK_VEC128(m_TensorInverted);

		UpdateOrientationMatrix();
		UpdateWorldInertiaTensorInvert();
	}

	M_INLINE void AddImpulse(vec128 _Impulse, vec128 _r)
	{
		WDYNAMICS_CHECK_VEC128(_Impulse);

		m_Velocity = M_VMAdd(_Impulse, m_MassInverted, m_Velocity);

		//_r = M_VConst(0,-0.5, -0.5, 0);

		vec128 Tmp1 = M_VXpd(_r, _Impulse);
		//vec128 Tmp1 = M_VXpd(_Impulse, _r);
		vec128 Tmp2 = M_VMulMat(Tmp1, m_WorldInertiaTensorInvert);
		m_AngularVelocity = M_VAdd(m_AngularVelocity, Tmp2);
	}

	M_INLINE void AddImpulse(vec128 _Impulse)
	{
		WDYNAMICS_CHECK_VEC128(_Impulse);

		m_Velocity = M_VMAdd(_Impulse, m_MassInverted, m_Velocity);
	}

	void UpdateOrientationMatrix()
	{
		m_MatrixOrientation.Unit();
		CVectorUtil::QuatToMatrix(m_Orientation, m_MatrixOrientation);
	}

	void UpdateWorldInertiaTensorInvert();

	M_INLINE void SetPosition(const CVec3Dfp32& _Pos);
	M_INLINE CVec3Dfp32 GetPosition() const;

	M_INLINE void SetVelocity(const CVec3Dfp32& _Velocity);
	M_INLINE CVec3Dfp32 GetVelocity() const;

	M_INLINE void SetAngularVelocity(const CVec3Dfp32& _Velocity);
	M_INLINE CVec3Dfp32 GetAngularVelocity() const;

	M_INLINE void SetOrientation(const CQuatfp32& _Orientation);
	M_INLINE CQuatfp32 GetOrientation() const;

	M_INLINE void SetMass(fp32 _Mass);
	M_INLINE fp32 GetMass() const;

	M_INLINE void SetInertiaTensor(const CVec3Dfp32& _Tensor);

	M_INLINE static CVec4Dfp32 To4D(const CVec3Dfp32& _V)
	{
		return CVec4Dfp32(_V[0], _V[1], _V[2], 1.0f);
	}

	M_INLINE static CVec4Dfp32 To4D_LastZero(const CVec3Dfp32& _V)
	{
		return CVec4Dfp32(_V[0], _V[1], _V[2], 0.0f);
	}

	M_INLINE static CVec3Dfp32 To3D(const CVec4Dfp32& _V)
	{
		return CVec3Dfp32(_V[0], _V[1], _V[2]);
	}

	M_INLINE static CVec4Dfp32 QuatToVec4(const CQuatfp32& _V)
	{
		return CVec4Dfp32(_V.k[0], _V.k[1], _V.k[2], _V.k[3]);
	}

	CVec4Dfp32 m_Position;
	CVec4Dfp32 m_Orientation;
	CMat4Dfp32 m_MatrixOrientation; // TODO: Kanske onödig att spara undan här?
	CVec4Dfp32 m_Force;
	CVec4Dfp32 m_Torque;
	CVec4Dfp32 m_Velocity;
	CVec4Dfp32 m_AngularVelocity;

	CVec4Dfp32 m_TensorInverted;
	CMat4Dfp32 m_WorldInertiaTensorInvert;
	CVec4Dfp32 m_Mass;
	CVec4Dfp32 m_MassInverted;

	union
	{
		struct
		{
			uint32 m_FreezeCounter;
			uint32 m_bActive;
			uint32 m_bStationary;
			uint32 __pad;
		};

		vec128 m_Misc;
	};

/*	uint32 m_FreezeCounter : 30;
	uint32 m_bActive : 1;
	uint32 m_bStationary : 1;
	uint32 __pad[3];*/
};

M_INLINE void CWD_RigidBodyState::SetPosition(const CVec3Dfp32& _Pos)
{
	m_Position = To4D(_Pos);		
}

M_INLINE CVec3Dfp32 CWD_RigidBodyState::GetPosition() const
{
	return To3D(m_Position);
}

M_INLINE void CWD_RigidBodyState::SetVelocity(const CVec3Dfp32& _Velocity)
{
	m_Velocity = To4D(_Velocity);
}

M_INLINE CVec3Dfp32 CWD_RigidBodyState::GetVelocity() const
{
	return To3D(m_Velocity);
}

M_INLINE void CWD_RigidBodyState::SetAngularVelocity(const CVec3Dfp32& _AngularVelocity)
{
	m_AngularVelocity = To4D(_AngularVelocity);
}

M_INLINE CVec3Dfp32 CWD_RigidBodyState::GetAngularVelocity() const
{
	return To3D(m_AngularVelocity);
}

M_INLINE void CWD_RigidBodyState::SetOrientation(const CQuatfp32& _Orientation)
{
	m_Orientation[0] = _Orientation.k[0];
	m_Orientation[1] = _Orientation.k[1];
	m_Orientation[2] = _Orientation.k[2];
	m_Orientation[3] = _Orientation.k[3];
}

M_INLINE CQuatfp32 CWD_RigidBodyState::GetOrientation() const
{
	CQuatfp32 r;
	r.k[0] = m_Orientation[0];
	r.k[1] = m_Orientation[1];
	r.k[2] = m_Orientation[2];
	r.k[3] = m_Orientation[3];

	return r;
}

M_INLINE void CWD_RigidBodyState::SetMass(fp32 _Mass)
{
	m_Mass = CVec3Dfp32(_Mass, _Mass, _Mass);
	fp32 MassRecip = 1.0f / _Mass;
	m_MassInverted = CVec3Dfp32(MassRecip, MassRecip, MassRecip);
}

M_INLINE fp32 CWD_RigidBodyState::GetMass() const
{
	CVec4Dfp32 tmp = m_Mass;
	return tmp[0];
}

M_INLINE void CWD_RigidBodyState::SetInertiaTensor(const CVec3Dfp32& _Tensor)
{
	m_TensorInverted = To4D(CVec3Dfp32(1.0f / _Tensor[0], 1.0f / _Tensor[1], 1.0f / _Tensor[2]));
	WDYNAMICS_CHECK_VEC128(m_TensorInverted);
}

/*
#define CWD_COLLISIONINFO_MAX_COLLISIONS (10)

class CWD_CollisionInfo
{
public:
	CWD_CollisionInfo()
	{
	}

	fp32 GetRelativeVelocity() const;

	bool m_bIsColliding;
	fp32 m_RelativeVelocity;
	fp32 m_Distance;

	vec128 m_PointOfCollision[CWD_COLLISIONINFO_MAX_COLLISIONS];
	vec128 m_Normal;
	//vec128 m_RA, m_RB;
	CWD_RigidBody *m_pRigidBody1, *m_pRigidBody2;

	fp32 m_Epsilon;

	fp32 m_CollisionImpulseDenominator;
	fp32 m_MaxAppliedImpulse;

	int m_UserData1;
	int m_UserData2;
};
*/

class CWD_Island
{
public:
	TAP_RCD<CWD_RigidBody2 *> GetBodies();
	const TAP_RCD<CWD_RigidBody2 *> GetBodies() const;
	
	TAP_RCD<CWD_RigidBodyState> GetBodyStates();
	const TAP_RCD<CWD_RigidBodyState> GetBodyStates() const;

protected:
	TAP_RCD<CWD_RigidBody2 *> m_pRigidBodies;
	TAP_RCD<CWD_RigidBodyState> m_pRigidBodyStates;
};

class CWD_DynamicsSolver
{
public:
	virtual void Solve(const CWD_Island& _Island) = 0;
};

class CWD_Collider;


class M_ALIGN(16) CWD_ContactInfo 
{
public:
	CWD_ContactInfo()
	{
	}

	M_INLINE fp32 GetRelativeVelocity() const;

	CVec4Dfp32 m_PointOfCollision;
	CVec4Dfp32 m_Normal;

	int32 m_bIsColliding : 1;
	int32 m_iCluster : 31;

	union
	{
		struct
		{
			fp32 m_RelativeVelocity;
			fp32 m_Distance;

			fp32 m_CollisionImpulseDenominator;
			fp32 __m_MaxAppliedImpulse;
		};

		vec128 m_Misc;
	};

	CVec4Dfp32 M_ALIGN(16) m_Friction_Elasticity;		// [ StaticFriction, DynamicFriction, Elasticity, 0 ]

	vec128 m_RestImpulse;
	vec128 m_RelativeTangentVelocityDirection;
	vec128 m_BiasVelocity;

	CVec4Dfp32 m_MaxAppliedImpulse;

	CVec3Dfp32 GetPointOfCollision() const
	{
		return CVec3Dfp32(m_PointOfCollision[0], m_PointOfCollision[1], m_PointOfCollision[2]);
	}

	CVec3Dfp32 GetNormal() const
	{
		return CVec3Dfp32(m_Normal[0], m_Normal[1], m_Normal[2]);
	}

	int32 m_iRB1, m_iRB2;
	//CWD_RigidBody *m_pRigidBody1, *m_pRigidBody2;
	int32 m_UserData1, m_UserData2;
protected:
}; 

class CWD_DynamicsDebugRenderer
{
public:
	void SetColour(fp32 _r, fp32 _g, fp32 _b)
	{
		m_Colour = CVec3Dfp32(_r, _g, _b);
	}

	CVec3Dfp32 GetColour() const
	{
		return m_Colour;
	}

	virtual void RenderVector(const CVec3Dfp32& _P, const CVec3Dfp32& _V) = 0;

protected:
	CVec3Dfp32 m_Colour;
};


class CWD_Constraint;

template <typename T, int MaxInstanceSize>
class TFunkyArray
{
public:
	struct InstanceBlock
	{
		M_ALIGN(16) uint8 m_V[MaxInstanceSize];			
	};

	void SetLen(int _Size)
	{
		m_lVector.SetLen(_Size);
	}

	void QuickSetLen(int _Size)
	{
		m_lVector.QuickSetLen(_Size);
	}

	template <typename S>
	void Add(const S& _s)
	{		
		m_lVector.Add(InstanceBlock());
		Set(Len() - 1, _s);
	}

	int Len() const
	{
		return m_lVector.Len();
	}

	template <typename S>
	void Set(int _i, const S& _s)
	{
		int foo1 = sizeof(S);
		int foo2 = MaxInstanceSize;

		M_ASSERT(sizeof(S) <= MaxInstanceSize, "!");
		InstanceBlock *pB = &m_lVector[_i];
		memcpy(pB, &_s, sizeof(S));
	}

	T* Get(int _i)
	{
		InstanceBlock *pB = &m_lVector[_i];
		T *pT = (T *) pB;
		//T &t = *pT;
		return pT;
	}

protected:
	TArray<InstanceBlock> m_lVector;
};

class CWD_DynamicsWorld;

class M_ALIGN(16) CWD_Constraint
{
public:
	//	virtual void GetJacobian() = 0;
	CWD_Constraint()
	{
		m_bUseSolve = false;
	}

	virtual int GenerateContact(const CWD_DynamicsWorld& _World, vec128 _dt, TAP_RCD<CWD_ContactInfo> pContactInfo) const = 0;
	virtual fp32 Solve(CWD_DynamicsWorld& _World, vec128 _dt) = 0;

	bool m_bUseSolve;

protected:

};

class CWD_DynamicsWorld
{
public:
	CWD_DynamicsWorld();

	int AddRigidBody(CWD_RigidBody2 *_pRigidBody, const CWD_RigidBodyState& _State);

	template <typename S>
	int AddConstraint(const S& _Constraint)
	{
		int ID = m_lConstraints.Len();
		m_lConstraints.Add(_Constraint);
		return ID;
	}

	void SetCollider(CWD_Collider *_pCollider);
	CWD_Collider *GetCollider();

	void SetDebugRenderer(CWD_DynamicsDebugRenderer *_pRenderer);
	CWD_DynamicsDebugRenderer *GetDebugRenderer();

	void SetGravity(const CVec3Dfp32& _Gravity);
	CVec3Dfp32 GetGravity() const;

	void RenderDebugInfo();

	M_INLINE CMat4Dfp32 GetTransform(int _iRigidBody) const;
	M_INLINE void GetTransform(int _iRigidBody, CMat4Dfp32& _T) const;
	M_INLINE bool IsStationary(int _iRigidBody) const;
	M_INLINE bool IsActive(int _iRigidBody) const;

	M_INLINE bool IsConnectedToWorld(int _iRigidBody) const;
	M_INLINE bool IsConnected(int _iRigidBody1, int _iRigidBody2) const;

	const CWD_RigidBodyState& GetRigidBodyState(uint _iRB) const;
	CWD_RigidBodyState& GetRigidBodyState(uint _iRB);

	bool IsValidID(uint _iRB) const
	{
		return _iRB < (uint)m_lRigidBodyStates.Len();
	}

	void Clear();

	void Simulate(fp32 _dt, int _nSteps, void *_pArgument1, void *_pArgument2);

	TAP_RCD<CWD_RigidBody2 *> GetRigidBodies();
	void GetWorldInertiaTensorInverted(int _iObject, CMat4Dfp32& _TensorInverted);

	static vec128 CollisionImpulseMag(const CWD_ContactInfo& _CI,  vec128 _RelativeVelocity, vec128 _Epsilon, CWD_RigidBodyState *_pRBState1, CWD_RigidBodyState *_pRBState2);

	friend class CWD_DynamicsWorldUnitTester;

	void AddLocalImpulse(int _iRigidBody, vec128 _Impulse, vec128 _Position);
	void AddWorldImpulse(int _iRigidBody, vec128 _Impulse, vec128 _Position);

	const TArray<CWD_CollisionEvent>& GetCollisionEvents();

protected:

	void Step(fp32 _dt, int _iStep, int _nSteps);
	void FilterDebrisCollisions(TAP_RCD<CWD_ContactInfo> _pContactInfo);
	void CollectCollisionEvents(const TArray<CWD_ContactInfo>& _lCollisionList, int _nCollisions);
	int DetectCollisions(fp32 _dt);
	int DetectConstraintCollisions(fp32 _dt);
	void ProcessCollisionsSimpleFriction(TAP_RCD<CWD_ContactInfo> _pContactInfo, fp32 _Epsilon, fp32 _dt);
	void ProcessCollisionsFirst(TAP_RCD<CWD_ContactInfo> _pContactInfo, fp32 _Epsilon, fp32 _dt);
	void ProcessConstraintCollisionsFirst(TAP_RCD<CWD_ContactInfo> _pContactInfo, fp32 _Epsilon, fp32 _dt);
	void ProcessCollisions(TAP_RCD<CWD_ContactInfo> _pContactInfo, fp32 _Epsilon, fp32 _dt);
	void ProcessContacts(fp32 _dt);
	void CheckStationary(fp32 _dt);
	void CopyFlags();

	void SaveFullState();
	void RestoreFullState();
	void RestoreMomentumState();
	void RestorePositionOrientationState();

	void AddExternalForces(fp32 _dt);
	void ClearExternalForces();

	void UpdateVelocity(fp32 _dt);
	void UpdatePosition(fp32 _dt);
	void UpdateOrientation(fp32 _dt);
	void UpdateOrientatioMatrixAndTensor();

	M_INLINE static fp32 GetKineticEnergy(const CWD_RigidBodyState& _State);
	M_INLINE static fp32 GetFreezeEnergyThreshold(const CWD_RigidBodyState& _State, const CVec4Dfp32& _Gravity, fp32 _dt);

	M_INLINE static vec128 GetKineticEnergySIMD(const CWD_RigidBodyState& _State);
	M_INLINE static vec128 GetFreezeEnergyThresholdSIMD(const CWD_RigidBodyState& _State, vec128 _Gravity, vec128 _dt);

	void GetTensorInverted(int _iRigidBody, CMat4Dfp32& _TensorInverted);

	void PreCalculateImpulseDenominator(TAP_RCD<CWD_ContactInfo> _pContactInfo);

	TArray<CWD_RigidBody2 *> m_lpRigidBodies;
	TArray<CWD_RigidBodyState> m_lRigidBodyStates;
	TArray<CWD_RigidBodyState> m_lRigidBodyStatesSaved;

	TFunkyArray<CWD_Constraint, sizeof(CWD_Constraint) + 1024> m_lConstraints;

	TArray<CWD_ContactInfo> m_lContactInfo;
	TArray<CWD_ContactInfo> m_lConstraintContactInfo;
	int m_nContacts;
	int m_nConstraintContacts;

	int m_nCollisionEvents;
	TArray<CWD_CollisionEvent> m_lCollisionEvents;
	TArray<CWD_CollisionEvent> m_lTemporaryCollisionEvents;

	fp32 m_ScaleFactor;
	fp32 m_ScaleFactorInv;
	CVec4Dfp32 m_Gravity;

	CWD_Collider *m_pCollider;
	CWD_DynamicsDebugRenderer *m_pDebugRenderer;

	// Dummy object
	CWD_RigidBodyState m_DummyWorldState;
	CWD_RigidBody2 m_DummyRigidBody;

	void *m_pArgument1, *m_pArgument2;
};

M_INLINE void CWD_DynamicsWorld::GetTransform(int _iRigidBody, CMat4Dfp32& _T) const
{
	M_ASSERT(_iRigidBody < m_lRigidBodyStates.Len(), "!");

	m_lRigidBodyStates.GetBasePtr()[_iRigidBody].GetTransform(_T);
}


M_INLINE CMat4Dfp32 CWD_DynamicsWorld::GetTransform(int _iRigidBody) const
{
	M_ASSERT(_iRigidBody < m_lRigidBodyStates.Len(), "!");

	return m_lRigidBodyStates.GetBasePtr()[_iRigidBody].GetTransform();
	//const CWD_RigidBodyState& State = m_lRigidBodyStates[_iRigidBody];
	//return State.GetTransform();
}

M_INLINE bool CWD_DynamicsWorld::IsStationary(int _iRigidBody) const
{
	return m_lRigidBodyStates[_iRigidBody].m_bStationary != 0;
}

M_INLINE bool CWD_DynamicsWorld::IsActive(int _iRigidBody) const
{
	return m_lRigidBodyStates[_iRigidBody].m_bActive != 0;
}

M_INLINE bool CWD_DynamicsWorld::IsConnectedToWorld(int _iRigidBody) const
{
	return m_lpRigidBodies[_iRigidBody]->IsConnectedTo(0);
}

M_INLINE bool CWD_DynamicsWorld::IsConnected(int _iRigidBody1, int _iRigidBody2) const
{
	return m_lpRigidBodies[_iRigidBody1]->IsConnectedTo(_iRigidBody2);
}

M_INLINE fp32 CWD_DynamicsWorld::GetKineticEnergy(const CWD_RigidBodyState& _State)
{
	fp32 VelocitySq = _State.m_Velocity.LengthSqr();
	fp32 Mass = _State.m_Mass[0];

	fp32 LinearEnergy = 0.5f * Mass * VelocitySq;
	CVec4Dfp32 AngularVelocity = _State.m_AngularVelocity;
	CVec4Dfp32 InertiaTensorInv = _State.m_TensorInverted;

	fp32 AngularEnergy = 
		(1.0f / InertiaTensorInv[0]) * AngularVelocity[0] * AngularVelocity[0]
		+ (1.0f / InertiaTensorInv[1]) * AngularVelocity[1] * AngularVelocity[1]
		+ (1.0f / InertiaTensorInv[2]) * AngularVelocity[2] * AngularVelocity[2];
		AngularEnergy *= 0.5f;

	return LinearEnergy + AngularEnergy;
}

M_INLINE fp32 CWD_DynamicsWorld::GetFreezeEnergyThreshold(const CWD_RigidBodyState& _State, const CVec4Dfp32& _Gravity, fp32 _dt)
{
	fp32 Mass = _State.m_Mass[0];
	CVec4Dfp32 pg =  _Gravity * Mass * _dt;
	return pg.LengthSqr() / (2.0f * Mass);
}

M_INLINE vec128 CWD_DynamicsWorld::GetKineticEnergySIMD(const CWD_RigidBodyState& _State)
{
	vec128 V = _State.m_Velocity;
	vec128 w = _State.m_AngularVelocity;
	vec128 wsq = M_VMul(w, w);
	vec128 I = M_VRcp(_State.m_TensorInverted);

	vec128 VSq = M_VMul(V, V);
	vec128 Mass = _State.m_Mass;

	vec128 LinearEnergy = M_VMul(M_VHalf(), M_VMul(Mass, VSq));
	vec128 AngularEnergy = M_VMul(M_VHalf(), M_VDp3(I, wsq));

	return M_VSelComp(0, M_VAdd(LinearEnergy, AngularEnergy), M_VZero());
}

M_INLINE vec128 CWD_DynamicsWorld::GetFreezeEnergyThresholdSIMD(const CWD_RigidBodyState& _State, vec128 _Gravity, vec128 _dt)
{
	// TODO: TESTA!

	vec128 Mass = _State.m_Mass;
	vec128 MassInv = _State.m_MassInverted;
	vec128 pg = M_VMul(_Gravity, M_VMul(Mass, _dt));
	vec128 T = M_VMul(MassInv, M_VMul(M_VHalf(), M_VMul(pg, pg)));
	return T;
}

/*
void f()
{
	TArray<Pair> lPairs;

	int nPairs = pCollider->Broadphase(pWorld, pRigigbodies, pPairs, NULL, NULL);

	const int MaxIslands = 10;
	TArray<CWD_Island> lIslands;
	lIslands.QuickSetLen(MaxIslands);

	int nIslands = 0;

	for (int i = 0; i < nPairs; i++)
	{

	}
}

*/


class CContactGraph2
{
public:

	CContactGraph2()
	{
//		m_lRoot.QuickSetLen(20);
	}

	void Clear()
	{
//		m_lRoot.QuickSetLen(0);
	}

	static void Connect(CWD_RigidBody2 *_pRB1, CWD_RigidBody2 *_pRB2)
	{
/*		CDoubleLinkable<CWD_RigidBody2 *> *pLinked1 = &_pRB1->m_lNeighbours;
		CDoubleLinkable<CWD_RigidBody2 *> *pLinked2 = &_pRB2->m_lNeighbours;

		_pRB1->m_bVisited = false;
		_pRB2->m_bVisited = false;

		pLinked1->Concat(pLinked2);*/

		TDisjointSet<CWD_RigidBody2 *>::Union(&_pRB1->m_ContactGraphNode, &_pRB2->m_ContactGraphNode);
	}

	void BuildGraph(CWD_DynamicsWorld *pWorld)
	{
		//m_lRoot.QuickSetLen(0);

		TAP_RCD<CWD_RigidBody2 *> pRigidBodies = pWorld->GetRigidBodies();

		for (int i = 0; i < pRigidBodies.Len(); i++)
		{
			CWD_RigidBody2 *pRB = pRigidBodies[i];
			TDisjointSet<CWD_RigidBody2 *>::MakeSet(&pRB->m_ContactGraphNode);


/*			CDoubleLinkable<CWD_RigidBody2 *> *pFirst = pRB->m_lNeighbours.m_pFirst;

			if (pFirst->m_Value->m_bVisited) continue;
			pFirst->m_Value->m_bVisited = true;

			m_lRoot.Add(pRigidBodies[i]->m_lNeighbours.m_pFirst);	*/
		}

	}

protected:

	//TArray<CDoubleLinkable<CWD_RigidBody2 *> *> m_lRoot;
};




/*
class Node
{
public:

	//List<Node> m_lNeighbours;

	CSingleLinkable<Node *> *lNeighbours;
};
*/

/*
class CContactGraph
{
public:

	CContactGraph()
	{
		m_nAllocated = 0;
	}

	void AddConnection(CWD_RigidBody2 *_pRB1, CWD_RigidBody2 *_pRB2)
	{
		CSingleLinkable<CWD_RigidBody2 *> *pLinkable1 = AllocLink(_pRB1);
		CSingleLinkable<CWD_RigidBody2 *> *pLinkable2 = AllocLink(_pRB2);

		if (pLinkable1 && pLinkable2)
		{
			pLinkable1->Add(&_pRB1->lNeighbours);
			pLinkable2->Add(&_pRB2->lNeighbours);
		}
	}

	void GetSubGraph(int _MagicNumber, CWD_RigidBody2 *_pRBStart, CWD_RigidBody2 *ppConnected, int _MaxConnected)
	{
		M_ASSERT(_MaxConnected >= 1, "!");
		M_ASSERT(_pRBStart->m_MagicNumber != _MagicNumber, "!");

		int n = 0;
		ppConnected[n++] = _pRBStart;
		_pRBStart->m_MagicNumber = _MagicNumber;

		int i = 0;
		while (i < n)
		{
			CSingleLinkable<CWD_RigidBody2 *> pL = ppConnected[i]->m_lNeighbours;
			while (pL != NULL)
			{
				if (pL->m_Value->m_MagicNumber != _MagicNumber)
				{
					ppConnected[n++] = pL->m_Value;
				}
				else
				{
					pL->m_Value->m_MagicNumber = _MagicNumber;
				}
				pL = pL->m_pNext;
			}
			i++;
		}
	}

	CSingleLinkable<CWD_RigidBody2 *> *AllocLink(CWD_RigidBody2* _pRB)
	{
		if (m_nAllocated < m_lMemoryPool.Len())
		{
			CSingleLinkable<CWD_RigidBody2 *> *pLinkable = &m_lMemoryPool[m_nAllocated++];
			pLinkable->m_Value = _pRB;
			return pLinkable;
		}
		return NULL;
	}

	void Clear()
	{
		m_nAllocated = 0;
	}

	void SetSize(int _n)
	{
		m_nAllocated.QuickSetLen(_n);
	}

protected:
	TThinArray<CSingleLinkable<CWD_RigidBody2 *> > m_lMemoryPool;
	int m_nAllocated;
};

void g()
{
	std::set<int> Pairs;

	Stack stack;
	stack.Add(...);

	while (stack.IsEmpty())
	{
		Node n = stack.First();

		Node **conn = GetConnections(n);
		Island island;

		foreach (nn in conn)
		{
			if (!nn.IsTagged())
			{
				island.Add(nn);
				stack.Add(nn);
			}
		}
	}
}

*/

class CWD_Collider
{
public:

	class Pair
	{
	public:
		int m_First, m_Second;
	};

	/*
	virtual void Broadphase(const CWD_DynamicsWorld *_world, 
							TAP_RCD<CWD_RigidBody2 *> _pRigidBodies,
							TAP_RCD<Pair> pContactPairs,
							void *_pArgument1,
							void *_pArgument2) = 0;
*/

	virtual void PreApplyExternalForces(CWD_DynamicsWorld *_pWorld, void *_pArgument1, void *_pArgument2) = 0;
	virtual void ApplyExternalForces(CWD_DynamicsWorld *_pWorld, void *_pArgument1, void *_pArgument2) = 0;
	virtual void PostApplyExternalForces(CWD_DynamicsWorld *_pWorld, void *_pArgument1, void *_pArgument2) = 0;

	virtual bool Collide(const CWD_DynamicsWorld *_world, 
						 CWD_RigidBody2 *_pBody,
 						 CWD_ContactInfo *_pContactInfo,
						 void *_pArgument1,
						 void *_pArgument2) = 0;

	virtual int Collide(const CWD_DynamicsWorld *_world, 
						TArray<CWD_RigidBody2 *> &_lpRigidBodies,
						TAP_RCD<CWD_ContactInfo> _pContactInfo,
						void *_pArgument1,
						void *_pArgument2) = 0;

	/*
	virtual void PreApplyExternalForces(void *_pArgument1, void *_pArgument2) = 0;
	virtual void ApplyExternalForces(void *_pArgument1, void *_pArgument2) = 0;
	virtual void PostApplyExternalForces(void *_pArgument1, void *_pArgument2) = 0;

	virtual void GetBoundingBox(const CRigidBody *_pRigidBody, CBox3Dfp64& _Box, void *_pArgument1, void *_pArgument2) = 0;
	*/
};

class CWD_DynamicsUtilFunctions
{
public:

	M_INLINE static vec128 RelativeVelocity(const CWD_RigidBodyState *_pRBState1, const CWD_RigidBodyState *_pRBState2, vec128 _RA, vec128 _RB, vec128 _Normal)
	{
		vec128 V1 = M_VAdd(M_VXpd(_pRBState1->m_AngularVelocity, _RA), _pRBState1->m_Velocity);
		vec128 V2 = M_VAdd(M_VXpd(_pRBState2->m_AngularVelocity, _RB), _pRBState2->m_Velocity);
		return M_VDp3(_Normal, M_VSub(V2, V1));
	}

#if 1
	M_INLINE static vec128 RelativeTangentVelocity(const CWD_RigidBodyState *_pRBState1, const CWD_RigidBodyState *_pRBState2, vec128 _RA, vec128 _RB, vec128 _Normal)
	{
		vec128 V1 = M_VAdd(M_VXpd(_pRBState1->m_AngularVelocity, _RA), _pRBState1->m_Velocity);
		vec128 V2 = M_VAdd(M_VXpd(_pRBState2->m_AngularVelocity, _RB), _pRBState2->m_Velocity);
		vec128 V2_Sub_V1 = M_VSub(V2, V1);

		vec128 T1 = M_VDp3(V2_Sub_V1, _Normal);
		//vec128 T2 = M_VDp3(_Normal, T1);
		vec128 T2 = M_VMul(_Normal, T1);

		vec128 T3 = M_VSub(V2_Sub_V1, T2);
		return M_VSelComp(3, M_VZero(), T3);

		//return M_VSub(V2_Sub_V1, M_VDp3(_Normal, M_VDp3(V2_Sub_V1, _Normal)));
	}

#else

	__declspec(noinline) static vec128 RelativeTangentVelocity(const CWD_RigidBodyState *_pRBState1, const CWD_RigidBodyState *_pRBState2, vec128 _RA, vec128 _RB, vec128 _Normal)
	{
		vec128 V1 = M_VAdd(M_VXpd(_pRBState1->m_AngularVelocity, _RA), _pRBState1->m_Velocity);
		vec128 V2 = M_VAdd(M_VXpd(_pRBState2->m_AngularVelocity, _RB), _pRBState2->m_Velocity);
		vec128 V2_Sub_V1 = M_VSub(V2, V1);

		vec128 T1 = M_VDp3(V2_Sub_V1, _Normal);
		vec128 T2 = M_VSub(V2_Sub_V1, T1);
		vec128 T3 = M_VSelComp(3, M_VZero(), T2);

		return T3;
	}

#endif

	M_INLINE static vec128 M_VSafeNrm3(vec128 _v)
	{
		vec128 Len = M_VLen3(_v);
		vec128 Epsilon = M_VConst(0.0001f, 0.0001f, 0.0001f, 0.0001f);
		Len = M_VSelMsk(M_VCmpLEMsk(Len, Epsilon), M_VOne(), Len);

		return M_VMul(M_VRcp(Len), _v);
	}

	/*
		TODO: Funkar verkligen denna?
	 */
	M_INLINE static vec128 M_VSafeNrm3_2(vec128 _v) 
	{
		vec128 Default = M_VConst(1.0f, 0.0f, 0.0f, 0.0f);

		vec128 Len = M_VLen3(_v);
		vec128 Epsilon = M_VConst(0.0001f, 0.0001f, 0.0001f, 0.0001f);
		vec128 NewLen = M_VSelMsk(M_VCmpLEMsk(Len, Epsilon), M_VOne(), Len);

		vec128 X = M_VMul(M_VRcp(NewLen), _v);

		return M_VSelMsk(M_VCmpLEMsk(Len, Epsilon), Default, X);
	}

	/*
		TODO: Funkar verkligen denna?
	*/
	M_INLINE static vec128 M_VSafeNrm3_3(vec128 _v, vec128 _default)
	{
		vec128 Len = M_VLen3(_v);
		vec128 Epsilon = M_VConst(0.0001f, 0.0001f, 0.0001f, 0.0001f);
		vec128 NewLen = M_VSelMsk(M_VCmpLEMsk(Len, Epsilon), M_VOne(), Len);

		vec128 X = M_VMul(M_VRcp(NewLen), _v);

//		return M_VSelMsk(M_VCmpLEMsk(Len, _default), _default, X);
		return M_VSelMsk(M_VCmpLEMsk(Len, Epsilon), _default, X);
	}

	M_INLINE static vec128 RestImpulse(const CWD_RigidBodyState *_pRBState1, const CWD_RigidBodyState *_pRBState2, const CWD_ContactInfo& _CI, vec128 _RA, vec128 _RB)
	{
		vec128 Normal = _CI.m_Normal;
		vec128 RelTangentVelocity = RelativeTangentVelocity(_pRBState1, _pRBState2, _RA, _RB, Normal);
		vec128 T = CWD_DynamicsUtilFunctions::M_VSafeNrm3(RelTangentVelocity);

		vec128 Denominator = ImpulseDenominator(_pRBState1, _pRBState2, _CI.m_PointOfCollision, T);
		vec128 DenominatorRecip = M_VRcp(Denominator);

		// A = -(1.0 + Epsilon) * RelativeVelocity
		//vec128 A = M_VLen3(RelTangentVelocity);

		//		vec128 ImpulseMag = M_VMul(A, DenominatorRecip);
		vec128 ImpulseMag = M_VMul(RelTangentVelocity, DenominatorRecip);
		ImpulseMag = M_VSelComp(3, M_VZero(), ImpulseMag);
		//vec128 Impulse = M_VMul(T, ImpulseMag);

		return ImpulseMag;	 
	}

	M_INLINE static vec128 ImpulseDenominator(const CWD_RigidBodyState *_pRBState1, const CWD_RigidBodyState *_pRBState2, vec128 _PointOfCollision, vec128 _Normal)
	{
		vec128 B1 = _pRBState1->m_MassInverted;
		vec128 B2 = _pRBState2->m_MassInverted;

		vec128 RA = M_VSub(_PointOfCollision, _pRBState1->m_Position);
		vec128 RB = M_VSub(_PointOfCollision, _pRBState2->m_Position);

		const CMat4Dfp32& WITInv1 = _pRBState1->m_WorldInertiaTensorInvert;
		const CMat4Dfp32& WITInv2 = _pRBState2->m_WorldInertiaTensorInvert;

		vec128 tmp;
		// TODO: DENNA MULTIPLICERING ÄR ÅT FEL HÅLL!!!
		tmp = M_VMulMat(M_VXpd(RA, _Normal), WITInv1);
		vec128 B3 = M_VDp3(_Normal, M_VXpd(tmp, RA));

		// TODO: DENNA MULTIPLICERING ÄR ÅT FEL HÅLL!!!
		tmp = M_VMulMat(M_VXpd(RB, _Normal), WITInv2);
		vec128 B4 = M_VDp3(_Normal, M_VXpd(tmp, RB));

		vec128 B1_ADD_B2 = M_VAdd(B1, B2);
		vec128 B3_ADD_B4 = M_VAdd(B3, B4);
		vec128 Denom = M_VAdd(B1_ADD_B2, B3_ADD_B4);
		return Denom;
	}

	M_INLINE static vec128 ImpulseDenominator(const CWD_RigidBodyState *_pRBState1, const CWD_RigidBodyState *_pRBState2, vec128 _RA, vec128 _RB, vec128 _Normal)
	{
		vec128 B1 = _pRBState1->m_MassInverted;
		vec128 B2 = _pRBState2->m_MassInverted;

		const CMat4Dfp32& WITInv1 = _pRBState1->m_WorldInertiaTensorInvert;
		const CMat4Dfp32& WITInv2 = _pRBState2->m_WorldInertiaTensorInvert;

		vec128 tmp;
		// TODO: DENNA MULTIPLICERING ÄR ÅT FEL HÅLL!!!
		tmp = M_VMulMat(M_VXpd(_RA, _Normal), WITInv1);
		vec128 B3 = M_VDp3(_Normal, M_VXpd(tmp, _RA));

		// TODO: DENNA MULTIPLICERING ÄR ÅT FEL HÅLL!!!
		tmp = M_VMulMat(M_VXpd(_RB, _Normal), WITInv2);
		vec128 B4 = M_VDp3(_Normal, M_VXpd(tmp, _RB));

		vec128 B1_ADD_B2 = M_VAdd(B1, B2);
		vec128 B3_ADD_B4 = M_VAdd(B3, B4);
		vec128 Denom = M_VAdd(B1_ADD_B2, B3_ADD_B4);
		return Denom;
	}


};

#endif
