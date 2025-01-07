#ifndef DYNAMICSENGINE_H
#define DYNAMICSENGINE_H

#define RIGHTMATORDER

#ifdef OSX
#include "mcccompat.h"
#endif

#include "MMath.h"

//#pragma optimize( "", off )
//#pragma inline_depth(0)

#include <limits>
using namespace std;

template <class T>
class TVector
{
public:
	TVector()
	{
		m_Vector.SetLen(0);
	}

	TVector(int _length)
	{
		m_Vector.SetLen(_length);
	}

	T& operator [] (int index)
	{
		return m_Vector[index];
	}

	T operator [] (int index) const
	{
		return m_Vector[index];
	}

	void SetLen(int _length)
	{
		m_Vector.SetLen(_length);
	}

	int GetLength() const
	{
		return m_Vector.Len();
	}

	TVector<T>& operator *= (T x)
	{
		for (int i = 0; i < m_Vector.Len(); i++)
		{
			m_Vector[i] *= x;
		}
		return *this;
	}


	void Print() const
	{
		M_TRACEALWAYS("{");
		for (int i = 0; i < m_Vector.Len(); i++)
		{
			M_TRACEALWAYS("%.3f",m_Vector[i]);
			if (i != m_Vector.Len() - 1)
			{
				M_TRACEALWAYS(",");
			}

		}
		M_TRACEALWAYS("}\n");

	}

protected:
	TArray<T> m_Vector;
};

template <class T>
class TMatrix
{
public:
	TMatrix()
	{
		m_nRows = 0;
		m_nCols = 0;

		m_Matrix.SetLen(0);
	}

	void SetIdentity()
	{
		M_ASSERT(m_nRows == m_nCols, "");
		for (int i = 0; i < m_nRows; i++)
		{
			for (int j = 0; j < m_nCols; j++)
			{
				if (i == j)
					(*this)[i][j] = T(1.0);
				else
					(*this)[i][j] = T(0.0);
			}
		}
	}

	void SetSubMatrix(int ii, int jj, const TMatrix<T>& _M)
	{
		int nRows = _M.GetRowCount();
		int nCols = _M.GetColumnCount();

		for (int i = 0; i < nRows; i++)
		{
			for (int j = 0; j < nCols; j++)
			{
				(*this)[i+ii][j+jj] = _M[i][j];
			}
		}
	}
	
	void GetColumn(int c, TVector<T>& _V)
	{
		_V.SetLen(m_nRows);
		for (int i = 0; i < m_nRows; i++)
		{
			_V[i] = (*this)[i][c];
		}
	}

	TMatrix(int nrows, int ncols)
	{
		m_Matrix.SetLen(nrows);
		for (int i = 0; i < nrows; i++)
		{
			m_Matrix[i].SetLen(ncols);
		}
		m_nRows = nrows;
		m_nCols = ncols;
	}

	void Transpose(TMatrix<T>& _Dest)
	{
		int nRows = GetRowCount();
		int nCols = GetColumnCount();

		_Dest.SetSize(nCols, nRows);

		for (int i = 0; i < nRows; i++)
		{
			for (int j = 0; j < nCols; j++)
			{
				_Dest[j][i] = (*this)[i][j];
			}
		}
	}

	void SetSize(int nrows, int ncols)
	{
		m_Matrix.SetLen(nrows);
		for (int i = 0; i < nrows; i++)
		{
			m_Matrix[i].SetLen(ncols);
		}
		m_nRows = nrows;
		m_nCols = ncols;
	}

	void Mult(const TMatrix<T>& _m2, TMatrix<T>& ret)
	{
		int m = this->GetRowCount();
		int n = this->GetColumnCount();
		int p = _m2.GetColumnCount();

		ret.SetSize(m,p);

		for (int i = 0; i < m; i++)
		{
			for (int j = 0; j < p; j++)
			{
				for (int k = 0; k < n; k++)
				{
					ret[i][j] += (*this)[i][k] * _m2[k][j];

				}
			}
		}
	}

	void Mult(const TVector<T>& _v, TMatrix<T>& ret)
	{
		int m = this->GetRowCount();
		int n = this->GetColumnCount();
		int p = 1;

		ret.SetSize(m,p);

		for (int i = 0; i < m; i++)
		{
			for (int j = 0; j < p; j++)
			{
				for (int k = 0; k < n; k++)
				{
					ret[i][j] += (*this)[i][k] * _v[k];

				}
			}
		}
	}

/*
	TMatrix<T> operator * (const TMatrix<T>& _m2)
	{
		int m = this->GetRowCount();
		int n = this->GetColumnCount();
		int p = _m2.GetColumnCount();

		TMatrix<T> ret(m,p);
		

		for (int i = 0; i < m; i++)
		{
			for (int j = 0; j < p; j++)
			{
				for (int k = 0; k < n; k++)
				{
					ret[i][j] += (*this)[i][k] * _m2[k][j];

				}
			}
		}
		return ret;
	}
	*/

	TMatrix<T>& operator *= (T x)
	{
		int nRows = GetRowCount();
		int nCols = GetColumnCount();

		for (int i = 0; i < nRows; i++)
		{
			for (int j = 0; j < nCols; j++)
			{
				(*this)[i][j] *= x;
			}
		}
		return *this;
	}

	TMatrix<T>& operator -= (T x)
	{
		int nRows = GetRowCount();
		int nCols = GetColumnCount();

		for (int i = 0; i < nRows; i++)
		{
			for (int j = 0; j < nCols; j++)
			{
				(*this)[i][j] -= x;
			}
		}
		return *this;
	}

	TMatrix<T>& operator += (T x)
	{
		int nRows = GetRowCount();
		int nCols = GetColumnCount();

		for (int i = 0; i < nRows; i++)
		{
			for (int j = 0; j < nCols; j++)
			{
				(*this)[i][j] += x;
			}
		}
		return *this;
	}


	TMatrix<T>& operator -= (const TMatrix<T>& _M)
	{
		int nRows = GetRowCount();
		int nCols = GetColumnCount();

		for (int i = 0; i < nRows; i++)
		{
			for (int j = 0; j < nCols; j++)
			{
				(*this)[i][j] -= _M[i][j];
			}
		}
		return *this;
	}

	TMatrix<T>& operator += (const TMatrix<T>& _M)
	{
		int nRows = GetRowCount();
		int nCols = GetColumnCount();

		for (int i = 0; i < nRows; i++)
		{
			for (int j = 0; j < nCols; j++)
			{
				(*this)[i][j] += _M[i][j];
			}
		}
		return *this;
	}

	void Print() const
	{
		int nRows = GetRowCount();
		int nCols = GetColumnCount();

		M_TRACEALWAYS("{\n");
		for (int i = 0; i < nRows; i++)
		{
			M_TRACEALWAYS("{");
			for (int j = 0; j < nCols; j++)
			{
				M_TRACEALWAYS("%.3f", (*this)[i][j]);
				if (j != nCols -1)
					M_TRACEALWAYS(",");
			}
			if (i != nRows -1)
				M_TRACEALWAYS("},\n");
			else
				M_TRACEALWAYS("}\n");
		}
		M_TRACEALWAYS("}\n");

	}

	TVector<T>& operator[] (int row)
	{
		return m_Matrix[row];
	}

	const TVector<T>& operator[] (int row) const
	{
		return m_Matrix[row];
	}

	int GetRowCount() const
	{
		return m_nRows;
	}

	int GetColumnCount() const
	{
		return m_nCols;
	}

protected:
	TArray<TVector<T> > m_Matrix;
	int m_nRows, m_nCols;
};

template <class T>
class GaussSiedel
{
public:

	static T Iterate(const TMatrix<T>& _A, TVector<T>& _x, const TVector<T>& _b)
	{
		int nRows = _A.GetRowCount();
		int nCols = _A.GetColumnCount();

		TVector<T> dx(_x.GetLength());

		for (int i = 0; i < nRows; i++)
		{
			T tmp = 0;

			for (int j = 0; j < nCols; j++)
			{
				tmp += _A[i][j] * _x[j];
			}

			/*				for (int j = 0; j < i; j++)
			{
			tmp += _A[i][j] * _x[j];
			}

			for (int j = i+1; j < nCols; j++)
			{
			tmp += _A[i][j] * _x[j];
			}*/

			//T dx = (_b[i] - tmp) / _A[i][i];
			//_x[i] += dx;

			_x[i] = (_b[i] + _A[i][i] * _x[i] - tmp) / _A[i][i];
		}

		return T(0);
	}
};

	class CInertia {
	public:
		static CVec3Dfp64 Sphere (double mass, double r);
		static CVec3Dfp64 Block (double mass, double a, double b, double c);
	};

	class CWorld;
	class CRigidBody;

	class CRigidBodyState {
	public:
		CRigidBodyState() {
			m_position= CVec3Dfp64(0,0,0);
			m_angularvelocity= CVec3Dfp64(0,0,0);
			m_force= CVec3Dfp64(0,0,0);
			m_torque= CVec3Dfp64(0,0,0);
			m_velocity= CVec3Dfp64(0,0,0);
			m_orientation.Unit();
			m_matrix_orientation.Unit();
			m_mass= 1.0;
			m_active= true;
			m_Stationary = false;
		}

		M_INLINE void GetTransform(CMat4Dfp64& transform) const 
		{
			// TODO: Onödigt?!?!?!?
//			m_orientation.Normalize();
			m_orientation.CreateMatrix(transform);

#ifdef RIGHTMATORDER
			CVec3Dfp64::GetRow(transform,3) = m_position;
#else
			transform.k[0][3]= m_position.k[0];
			transform.k[1][3]= m_position.k[1];
			transform.k[2][3]= m_position.k[2];
#endif
		}

		CVec3Dfp64 m_position, m_angularvelocity;
		CVec3Dfp64 m_force, m_torque;
		CVec3Dfp64 m_velocity;
		CQuatfp64 m_orientation;
		CMat4Dfp64 m_matrix_orientation;
		CMat4Dfp64 m_WorldInertiaTensorInverted; // TODO: Is to be removed after pending reorg.
		fp64 m_mass;
		bool m_active;
		bool m_Stationary;
	};

	class CWorld;

#if 0
#define MRIGIDBODY_BODYSTATE m_world->m_bodystate[m_id]
#define MRIGIDBODY_SAVEDBODYSTATE m_world->m_savedbodystate[m_id]
#else
#define MRIGIDBODY_BODYSTATE (*m_pstate)
#define MRIGIDBODY_SAVEDBODYSTATE (*m_psavedstate)
	/*
	#define MRIGIDBODY_BODYSTATE m_state
	#define MRIGIDBODY_SAVEDBODYSTATE m_savedstate
	*/
#endif

#define MAX_COLLISIONS_PER_OBJECT 200

	class CRigidBody {
	public:
		CRigidBody(CWorld *world);

		CRigidBody() 
		{
			Init(NULL);
		}

		M_INLINE CRigidBody *GetRigidBody()  { return this; }
		M_INLINE const CRigidBody *GetRigidBody() const { return this; }

		void *GetUserData();
		void SetUserData(void *userdata);

		void SetDynamicFriction(fp64 _DynamicsFriction)
		{
			m_DynamicsFriction = _DynamicsFriction;
		}

		fp64 GetDynamicFriction()
		{
			return m_DynamicsFriction;
		}

		void SetStaticFriction(fp64 _StaticFriction)
		{
			m_StaticFriction = _StaticFriction;
		}

		fp64 GetStaticFriction()
		{
			return m_StaticFriction;
		}

		fp64 GetCoefficientOfRestitution()
		{
			return m_CoefficientOfRestitution;
		}

		void SetCoefficientOfRestitution(fp64 _CoefficientOfRestitution)
		{
			m_CoefficientOfRestitution = _CoefficientOfRestitution;
		}

		void SetBoundingBox(const CBox3Dfp64& _Box)
		{
			m_BoundingBox = _Box;
		}

		void SetBoundingBox(const CBox3Dfp32& _Box)
		{
			m_BoundingBox.m_Min = _Box.m_Min.Getfp64();
			m_BoundingBox.m_Max = _Box.m_Max.Getfp64();
		}

		void GetBoundingBox(CBox3Dfp64& _Box)
		{
			_Box = m_BoundingBox;
		}

		M_INLINE void SetPosition(fp64 x, fp64 y, fp64 z) 
		{
			SetPosition(CVec3Dfp64(x,y,z));
		}

		M_INLINE void SetPosition(const CVec3Dfp64& pos) 
		{
			MRIGIDBODY_BODYSTATE.m_position= pos;
		}

		M_INLINE void GetPosition(CVec3Dfp64& pos) const 
		{
			pos= MRIGIDBODY_BODYSTATE.m_position;
		}

		M_INLINE const CVec3Dfp64& GetPosition() const 
		{
			return MRIGIDBODY_BODYSTATE.m_position;
		}

		M_INLINE void SetVelocity(fp64 vx, fp64 vy, fp64 vz)
		{
			SetVelocity(CVec3Dfp64(vx,vy,vz));
		}

		M_INLINE void SetVelocity(const CVec3Dfp64& vel)
		{
			MRIGIDBODY_BODYSTATE.m_velocity= vel;
		}

		M_INLINE void GetVelocity(CVec3Dfp64& vel) const
		{
			vel= MRIGIDBODY_BODYSTATE.m_velocity;
		}

		M_INLINE const CVec3Dfp64& GetVelocity() const
		{
			return MRIGIDBODY_BODYSTATE.m_velocity;
		}

		M_INLINE void GetAngularVelocity( CVec3Dfp64& angvel) const 
		{
			angvel= MRIGIDBODY_BODYSTATE.m_angularvelocity;
		}

		M_INLINE const CVec3Dfp64& GetAngularVelocity() const 
		{
			return MRIGIDBODY_BODYSTATE.m_angularvelocity;
		}

		M_INLINE void SetAngularVelocity(const CVec3Dfp64& angvel)
		{
			MRIGIDBODY_BODYSTATE.m_angularvelocity = angvel;
		}

		M_INLINE void SetAngularVelocity(fp64 ax, fp64 ay, fp64 az) 
		{
			SetAngularVelocity(CVec3Dfp64(ax,ay,az));
		}

		M_INLINE void SetOrientaion(const CQuatfp64& quat)
		{
			MRIGIDBODY_BODYSTATE.m_orientation= quat;
		}

		M_INLINE void GetOrientation(/* out */ CMat4Dfp64& mat) const 
		{
			MRIGIDBODY_BODYSTATE.m_orientation.CreateMatrix(mat);
		}

		M_INLINE void GetOrientation(/* out */ CQuatfp64& quat) const 
		{
			quat= MRIGIDBODY_BODYSTATE.m_orientation;
		}

		M_INLINE const CQuatfp64& GetOrientation() const 
		{
			return MRIGIDBODY_BODYSTATE.m_orientation;
		}

		M_INLINE CMat4Dfp64 GetOrientationMatrix() const 
		{
			CMat4Dfp64 ret;
			MRIGIDBODY_BODYSTATE.m_orientation.CreateMatrix(ret);
			return ret;
		}

		void AddMomentum(const CVec3Dfp64& dmomentum);
		void AddMomentum(fp64 dmx, fp64 dmy, fp64 dmz);

		void AddForce(const CVec3Dfp64& dforce);
		void AddForce(fp64 dx, fp64 dy, fp64 dz);
		void AddTorque(const CVec3Dfp64& dtorque);
		void AddTorque(fp64 dx, fp64 dy, fp64 dz);
		void ClearForces();

		void AddAngularMomentum(const CVec3Dfp64& dangularmomentum);

		M_INLINE void GetTransform(CMat4Dfp64& transform) const 
		{
			// TODO: Onödigt?!?!?!?
			MRIGIDBODY_BODYSTATE.m_orientation.Normalize();
			MRIGIDBODY_BODYSTATE.m_orientation.CreateMatrix(transform);

#ifdef RIGHTMATORDER
			CVec3Dfp64::GetRow(transform,3) = MRIGIDBODY_BODYSTATE.m_position;
#else
			transform.k[0][3]= MRIGIDBODY_BODYSTATE.m_position.k[0];
			transform.k[1][3]= MRIGIDBODY_BODYSTATE.m_position.k[1];
			transform.k[2][3]= MRIGIDBODY_BODYSTATE.m_position.k[2];
#endif
		}

		/*
			Method due to quat.-bug!
		 */
		M_INLINE void GetCorrectTransform(CMat4Dfp64& transform) const
		{
			GetTransform(transform);
			transform.Transpose3x3();
		}

		/*
		Method due to quat.-bug!
		*/
		M_INLINE void GetCorrectTransform(CMat4Dfp32& _Transform) const
		{
			CMat4Dfp64 Tmp;
			GetTransform(Tmp);
			Tmp.Transpose3x3();
			_Transform = _Transform.Getfp32();
		}

		/*
		Method due to quat.-bug!
		*/
/*		M_INLINE void GetCorrectTransform(CMat4Dfp32& _Transform) const
		{
			CMat4Dfp64 Tmp;
			GetTransform(Tmp);
			Tmp.Transpose3x3();

			for (int i = 0; i < 4; i++)
			{
				for (int j = 0; j < 3; j++)
				{
					_Transform.k[i][j] = Tmp.k[i][j];
				}
			}
		}*/

		M_INLINE CRigidBodyState *GetBodyState() 
		{
			return m_pstate;
		}

		M_INLINE const CRigidBodyState *GetBodyState() const 
		{
			return m_pstate;
		}

		void Stop();

		void SetMass(fp64 mass);
		fp64 GetMass() const;

		void SetInertiaTensor(const CVec3Dfp64& tensor);
		M_INLINE void GetInertiaTensor(CVec3Dfp64& tensor) const 
		{
			tensor= m_tensor;
		}

		M_INLINE CVec3Dfp64 GetInertiaTensor() const 
		{
			return m_tensor;
		}

		bool IsActive() const;
		dllvirtual void SetActive(bool active);
		bool IsStationary() const;
		void SetStationary(bool stationary);
		void CheckStationary();
		bool StationaryCandidate();

		void UpdatePosition(fp64 dt);
		void UpdateRotation(fp64 dt);
		void UpdateVelocity(fp64 dt);
		void UpdateState();

		void SaveFullState();
		void RestoreVelocityState();
		void RestorePositionOrientationState();

		void GetVelocityAt(const CVec3Dfp64& point, /* out */ CVec3Dfp64& vel ) const;
		CVec3Dfp64 GetVelocityAt(const CVec3Dfp64& point) const;

		M_INLINE void AddCollisionInfo(int iCollisionInfo) 
		{
			if (!IsActive())
				return;

			if (m_nCollisions < MAX_COLLISIONS_PER_OBJECT) {
				m_iCollisions[m_nCollisions] = iCollisionInfo;
				m_nCollisions++;
			}
			else {
				// TODO: assert -> log
				int foo;
				foo=1;
				M_ASSERT(false, "Not enough collisionsslots!");
			}
		}

		void GetWorldInertiaTensorInvert(/* out */ CMat4Dfp64& tensorinverted);
		const CMat4Dfp64 &GetWorldInertiaTensorInvert();
		void UpdateWorldInertiaTensorInvert();
		CRigidBodyState m_state, m_savedstate;
		CRigidBodyState *m_pstate, *m_psavedstate;

		CVec3Dfp64 m_avg_velocity;
		CVec3Dfp64 m_avg_angularvelocity;
		CVec3Dfp64 m_stationary_movement;
		int m_id;
		CWorld *m_world_;

		CVec3Dfp64 m_MassCenter;		// local offset from object postion to rigid body position
		CVec3Dfp64 m_tensor, m_tensorInverted;
		bool m_dirtyworldinertiatensor;

		bool m_childObject;

		bool m_processed;
		bool m_PartOfCompound;

		int m_nCollisions;
		int m_iCollisions[MAX_COLLISIONS_PER_OBJECT];

		fp64 m_TotalImpulseApplied;

		int m_FreezeCounter;

		fp64 m_StaticFriction, m_DynamicsFriction;
		fp64 m_CoefficientOfRestitution;

#define DYNAMICS_MAX_CONNECT_COUNT 10
		CRigidBody *m_pConnectedTo[DYNAMICS_MAX_CONNECT_COUNT];

		friend class CWorld;

	protected:
		CBox3Dfp64 m_BoundingBox;

		CVec3Dfp64 m_ExternalForce, m_ExternalTorque;

		void Init(CWorld *_pWorld);
		void *m_userdata;

		CMat4Dfp64 m_worldinertiatensor;
		bool m_stationary;
		fp64 m_stationary_candidate_for;
	};

	class CDynamicsCompound : public CRigidBody {
	public:
		CDynamicsCompound(CWorld *world);
		void AddRigidBody(CRigidBody *rigidbody, 
			const CVec3Dfp64& position, 
			const CVec3Dfp64& inertiatensor);

		TArray<CRigidBody *> *GetChildrens();
		void UpdateChildrens();

	protected:
		void UpdateInertiaTensor();
		TArray<CRigidBody *> m_rigidbodylist;
		TArray<CVec3Dfp64> m_inertiatensorlist;
		TArray<CVec3Dfp64> m_positionslist;
	};


	class CRigidBodyGroup {
	public:
		CRigidBodyGroup(const char *name);
		void AddRigidBody(CRigidBody *rigidbody);
		bool IsInternalCollision();
		void SetInternalCollision(bool internalcollision);
		TArray<CRigidBody *>& GetRigidBodyList();

	protected:
		TArray<CRigidBody *> m_rigidbodylist;
		bool m_internalCollisions;
		CStr m_name;
	};

	class CContactInfo {
	public:
		CContactInfo();
		M_INLINE fp64 GetRelativeVelocity() const;

		// Other properties
		/*
		* Distance
		* Normal
		* Relative velocity
		* IsColliding
		* 
		*/
		bool m_isColliding;
		fp64 m_relativevelocity;
		//CVec3Dfp64 m_relativetangentvelocity;
		fp64 m_distance;
		CVec3Dfp64 m_PointOfCollision;
		CVec3Dfp64 m_Normal;
		CVec3Dfp64 m_ra, m_rb;

		fp64 m_collision_impulse_denominator;

		// För kollisionsevents. Ska kanske inte ligga här?
		fp64 m_MaxAppliedImpulse;

		int m_UserData; // Remove?

		int m_iCluster;

		CRigidBody *m_pRigidBody1, *m_pRigidBody2;
		int m_UserData1, m_UserData2;
	protected:
	};

#define DYNAMICS_CRIGIGCONSTRAINT_N_VECTORPARAMS (10)
#define DYNAMICS_CRIGIGCONSTRAINT_N_SCALARPARAMS (10)

	class CRigidConstraint
	{
	public:
		CRigidConstraint()
		{
			m_Type = -1;
/*
			m_pRigidBody1 = NULL;
			m_pRigidBody2 = NULL;
			m_pRigidBodyState1 = NULL;
			m_pRigidBodyState2 = NULL;

			for (int i = 0; i < DYNAMICS_CRIGIGCONSTRAINT_N_VECTORPARAMS; i++)
			{
				SetVectorParam(i, CVec3Dfp64(10000,10000,100000));
			}

			for (int i = 0; i < DYNAMICS_CRIGIGCONSTRAINT_N_SCALARPARAMS; i++)
			{
				SetScalarParam(i, 100000);
			}
*/
			

		}

		enum
		{
			BALLJOINTCONSTRAINT = 0,
			FIXEDPOINTCONSTRAINT = 1,
			AXISCONSTRAINT = 2,
			HINGECONSTRAINT = 3,
			MAXDISTANCETOPOINTCONSTRAINT = 4,
			MAXDISTANCECONSTRAINT = 5,
			BALLJOINTTOWORLD = 6,
			MINDISTANCECONSTRAINT = 7,
		};

		void SetVectorParam(int _iParam, const CVec3Dfp64& _Value)
		{
			M_ASSERT(_iParam < DYNAMICS_CRIGIGCONSTRAINT_N_VECTORPARAMS, "");
			m_VectorParams[_iParam] = _Value;
		}

		const CVec3Dfp64& GetVectorParam(int _iParam) const
		{
			M_ASSERT(_iParam < DYNAMICS_CRIGIGCONSTRAINT_N_VECTORPARAMS, "");
			return m_VectorParams[_iParam];
		}

		CVec3Dfp64& GetVectorParam(int _iParam)
		{
			M_ASSERT(_iParam < DYNAMICS_CRIGIGCONSTRAINT_N_VECTORPARAMS, "");
			return m_VectorParams[_iParam];
		}

		void SetScalarParam(int _iParam, fp64 _Value)
		{
			M_ASSERT(_iParam < DYNAMICS_CRIGIGCONSTRAINT_N_SCALARPARAMS, "");
			m_ScalarParams[_iParam] = _Value;
		}
		
		fp64 GetScalarParam(int _iParam) const
		{
			M_ASSERT(_iParam < DYNAMICS_CRIGIGCONSTRAINT_N_SCALARPARAMS, "");
			return m_ScalarParams[_iParam];
		}

		fp64& GetScalarParam(int _iParam)
		{
			M_ASSERT(_iParam < DYNAMICS_CRIGIGCONSTRAINT_N_SCALARPARAMS, "");
			return m_ScalarParams[_iParam];
		}

		int16 m_Type;

		CRigidBody *m_pRigidBody1, *m_pRigidBody2;
		CRigidBodyState *m_pRigidBodyState1, *m_pRigidBodyState2;
		int m_iRigidBody1, m_iRigidBody2;

		CVec3Dfp64 m_VectorParams[DYNAMICS_CRIGIGCONSTRAINT_N_VECTORPARAMS];
		fp64 m_ScalarParams[DYNAMICS_CRIGIGCONSTRAINT_N_SCALARPARAMS];
	};


	class CFixedPointConstraintSolver
	{
	public:
		enum
		{
			LOCALPOSITION = 0,
			WORLDPOSITION = 1,
			MAXDISTANCEPOINT = 2,
		};
		static void PreSolve(const CWorld *_pWorld, CRigidConstraint  *_pConstraint, fp64 _dt);
		static void Solve(const CWorld *_pWorld, CRigidConstraint  *_pConstraint, fp64 _dt);
		static void PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
	};

	class CBallJointToWorldSolver
	{
	public:
		enum VectorParam
		{
			LOCALPOSITION = 0,
			WORLDPOSITION = 1,
			MAXDISTANCEPOINT = 2,
		};

		enum ScalarParam
		{
			MAXDISTANCE = 0,
		};

		static void PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
	};

	class CBallJointConstraintSolver
	{
	public:
		enum VectorParam
		{
			POSITION1 = 0,
			POSITION2 = 1,
			MAXDISTREFPOINT1 = 2,
			MAXDISTREFPOINT2 = 3,
		};

		enum ScalarParam
		{
			MINDISTANCE = 0,
		};

		static void PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
	};

	class CAxisConstraintSolver
	{
	public:
		enum VectorParam
		{
			AXIS = 0,
			LOCALPOSITION1 = 1,
			WORLDPOSITION1 = 2,
			LOCALPOSITION2 = 3,
			WORLDPOSITION2 = 4,
			MAXDISTANCEPOINT = 5,
		};

		enum ScalarParam
		{
			MINDISTANCE = 0
		};

		static void PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
	};


	class CHingeConstraintSolver
	{
	public:
		enum VectorParam
		{
			AXIS = 0,
			POSITION1a = 1,
			POSITION2a = 2,
			POSITION1b = 3,
			POSITION2b = 4,
			MAXDISTREFPOINT1 = 5,
			MAXDISTREFPOINT2 = 6,
		};

		enum ScalarParam
		{
			MINDISTANCE = 0,
			RELATIVEANGLE = 1,
		};

		static void PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
	};

	class CMaxDistanceToPointConstraintSolver
	{
	public:
		enum VectorParam
		{
			WORLDPOSITION = 0,
		};

		enum ScalarParam
		{
			MAXDISTANCE = 0,
		};

		static void PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
	};

	class CMinDistanceToPointConstraintSolver
	{
	public:
		enum VectorParam
		{
			WORLDPOSITION = 0,
		};

		enum ScalarParam
		{
			MINDISTANCE = 0,
		};

		static void PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
	};

	class CMaxDistanceConstraintSolver
	{
	public:
		enum VectorParam
		{
			LOCALPOSITION1 = 0,
			LOCALPOSITION2 = 1,
		};

		enum ScalarParam
		{
			MAXDISTANCE = 0,
		};

		static void PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
	};


	class CMinDistanceConstraintSolver
	{
	public:
		enum VectorParam
		{
			LOCALPOSITION1 = 0,
			LOCALPOSITION2 = 1,
		};

		enum ScalarParam
		{
			MINDISTANCE = 0,
		};

		static void PreSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void Solve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
		static void PostSolve(const CWorld *_pWorld, CRigidConstraint *_pConstraint, fp64 _dt);
	};

	enum ConstraintType
	{
		FOOCONSTRAINT = 10,
		FIXEDPOINTCONSTRAINT2 = 11,
	};

	class CConstraint {
	public:
		CConstraint();
		void SetUserData(void *userdata);
		void *GetUserData();
		virtual int Apply(fp64 dt)=0;
		virtual void Render();
		int m_Type;
	protected:
		void *m_userdata;
	};

	class CSpringConstraint : public CConstraint {
	public:
		CSpringConstraint(CRigidBody *rigidbody1, CRigidBody *rigidbody2, fp64 equilibriumlen, fp64 k, fp64 damp);
		virtual int Apply(fp64 dt);
		// TODO: temp unprot.
		//protected:
		CRigidBody *m_rigidbody1, *m_rigidbody2;
		fp64 m_equilibrium, m_k, m_damp;
	};

	class CConstantDistanceConstraint : public CConstraint {
	public:
		CConstantDistanceConstraint(CRigidBody *rigidbody1, CRigidBody *rigidbody2, fp64 distance);
		virtual int Apply(fp64 dt);

		CRigidBody *m_rigidbody1, *m_rigidbody2;
		fp64 m_distance;
	};

	class CBallJointConstraint : public CConstraint
	{
	public:
		CBallJointConstraint(CRigidBody *rigidbody1, CRigidBody *rigidbody2);
		virtual int Apply(fp64 dt);

		CRigidBody *m_rigidbody1, *m_rigidbody2;
		fp64 m_distance;
	};

	class IWorldCollider
	{
	public:
		virtual void PreCollide(CWorld *_pWorld) = 0;

		virtual bool Collide(const CWorld *_world, 
							 CRigidBody *_pBody,
 							 CContactInfo *_pContactInfo,
							 void *_pArgument1,
							 void *_pArgument2) = 0;

		virtual int Collide(const CWorld *_world, 
							const TArray<CRigidBody *>& _BodyList,
							CContactInfo *_pContactInfo, 
							int _MaxCollisions,
							void *_pArgument1,
							void *_pArgument2) = 0;

		virtual void PreApplyExternalForces(void *_pArgument1, void *_pArgument2) = 0;
		virtual void ApplyExternalForces(void *_pArgument1, void *_pArgument2) = 0;
		virtual void PostApplyExternalForces(void *_pArgument1, void *_pArgument2) = 0;

		virtual void GetBoundingBox(const CRigidBody *_pRigidBody, CBox3Dfp64& _Box, void *_pArgument1, void *_pArgument2) = 0;
	};

	class IDynamicsDebugRenderer
	{
	public:
		virtual void Render(const CContactInfo& _ContactInfo) = 0;
		virtual void Render(const CRigidBody *_pRigidBody) = 0;
	};

	class CCollisionEvent
	{
	public:
		CCollisionEvent()
		{
			m_ImpulseTimesInvMass = 0.0f;
			m_PointOfCollision = CVec3Dfp32(0.0f);
			m_pRigidBodyA = NULL;
			m_pRigidBodyB = NULL;
			m_UserData1 = 0;
			m_UserData2 = 0;
		}

		CCollisionEvent(fp32 _Impulse, const CVec3Dfp32 _PointOfCollision)
		{
			m_ImpulseTimesInvMass = _Impulse;
			m_PointOfCollision = _PointOfCollision;
			m_pRigidBodyA = NULL;
			m_pRigidBodyB = NULL;
		}

		fp32 m_ImpulseTimesInvMass;
		CVec3Dfp32 m_PointOfCollision;
		CRigidBody *m_pRigidBodyA, *m_pRigidBodyB;
		int m_UserData1, m_UserData2;
	};

	class CWorld {
	public:
		enum CollisionPrecision
		{
			Default = 0,
			Box = 1,
			Sphere = 2
		};

		CWorld();
		virtual ~CWorld();

		void SetCollisionPrecision(int _Precision)
		{
			m_CollisionPrecision = _Precision;
		}

		int GetCollisionPrecision() const
		{
			return m_CollisionPrecision;
		}

		/*
		CGeometry *CreateGeometry(int rigidbodytype);
		void AttachBodies(const CRigidBody& rb1, const CRigidBody& rb2,
		const CJoint& joint );*/

		M_INLINE bool IsConnected(const CRigidBody *_pRigidBody1, const CRigidBody *_pRigidBody2) const
		{
			for (int i = 0; i < DYNAMICS_MAX_CONNECT_COUNT; i++)
			{
				const CRigidBody *pR = _pRigidBody1->m_pConnectedTo[i];
				if (pR == NULL) return false;
				if (pR == _pRigidBody2) 
				{
					return true;
				}
			}
			return false;
		}

		M_INLINE bool IsConnectedToWorld(const CRigidBody *_pRigidBody) const
		{
			for (int i = 0; i < DYNAMICS_MAX_CONNECT_COUNT; i++)
			{
				const CRigidBody *pR = _pRigidBody->m_pConnectedTo[i];
				if (pR == NULL) return false;
				if (pR == &m_DummyRigidBody) return true;
			}
			return false;
		}

		void AddConnection(CRigidBody *_pRigidBody1, CRigidBody *_pRigidBody2)
		{
			for (int i = 0; i < DYNAMICS_MAX_CONNECT_COUNT; i++)
			{
				CRigidBody *pR = _pRigidBody1->m_pConnectedTo[i];
				if (pR == NULL) 
				{
					_pRigidBody1->m_pConnectedTo[i] = _pRigidBody2;
					return;
				}
			}
			M_ASSERT(false, "Too many connections!");
		}

		void RemoveConnection(CRigidBody* _pRigidBody1, CRigidBody* _pRigidBody2)
		{
			M_ASSERT(_pRigidBody1, "RemoveConnection: Invalid body 1");
			M_ASSERT(_pRigidBody2, "RemoveConnection: Invalid body 2");

			CRigidBody** ppConnectedTo = _pRigidBody1->m_pConnectedTo;
			for (int i = 0; i < DYNAMICS_MAX_CONNECT_COUNT; i++)
			{
				if (ppConnectedTo[i] == _pRigidBody2)
				{
					for (int j = i; j < (DYNAMICS_MAX_CONNECT_COUNT-1); j++)
						ppConnectedTo[j] = ppConnectedTo[j+1];
					ppConnectedTo[(DYNAMICS_MAX_CONNECT_COUNT-1)] = NULL;
					i--;
				}
			}

			ppConnectedTo = _pRigidBody2->m_pConnectedTo;
			for (int i = 0; i < DYNAMICS_MAX_CONNECT_COUNT; i++)
			{
				if (ppConnectedTo[i] == _pRigidBody1)
				{
					for (int j = i; j < (DYNAMICS_MAX_CONNECT_COUNT-1); j++)
						ppConnectedTo[j] = ppConnectedTo[j+1];
					ppConnectedTo[(DYNAMICS_MAX_CONNECT_COUNT-1)] = NULL;
					i--;
				}
			}
		}

		void SetScaleFactor(fp64 _ScaleFactor)
		{
			m_ScaleFactor = _ScaleFactor;
			m_ScaleFactorInv = 1.0/_ScaleFactor;
		}

		fp64 GetScaleFactor() const
		{
			return m_ScaleFactor;
		}

		void SetWorldCollider(IWorldCollider *_pCollider);
		IWorldCollider *GetWorldCollider();

		void AddRigidBody(CRigidBody *rigidbody);
		void RemoveRigidBody(CRigidBody *_pRigidBody);

		void Clear();

		void AddRigidBodyGroup(CRigidBodyGroup *group);
		void AddConstraint(CConstraint *constraint);


		int AddBallJointConstraint(CRigidBody *_pRigidBody1, CRigidBody *_pRigidBody2, const CVec3Dfp64& _p1, const CVec3Dfp64& _p2, fp64 _MaxAngle);
		int AddFixedPointConstraint(CRigidBody *_pRigidBody, const CVec3Dfp64& _LocalPosition, const CVec3Dfp64& _WorldPosition);
		int AddAxisConstraint(CRigidBody *_pRigidBody, const CVec3Dfp64& _Axis, fp64 _AxisLength, const CVec3Dfp64& _Position, const CVec3Dfp64& _AngleAxis, fp64 _MaxAngle);
		int AddHingeConstraint(CRigidBody *_pRigidBody1, CRigidBody *_pRigidBody2, const CVec3Dfp64& _Axis, const CVec3Dfp64& _AngleAxis, fp64 _AxisLength, const CVec3Dfp64& _Position, fp64 _MaxAngle);
		int AddMaxDistanceToPointConstraint(CRigidBody *_pRigidBody, const CVec3Dfp64& _WorldPosition, fp64 _MaxDistance);
		int AddMinDistanceToPointConstraint(CRigidBody *_pRigidBody, const CVec3Dfp64& _WorldPosition, fp64 _MinDistance);
		int AddBallJointToWorld(CRigidBody *_pRigidBody, const CVec3Dfp64& _LocalPosition, const CVec3Dfp64& _WorldPosition, CVec3Dfp64 _AngleAxis, fp64 _MaxAngle);
		int AddMaxDistanceConstraint(CRigidBody *_pRigidBody1, CRigidBody *_pRigidBody2, const CVec3Dfp64& _LocalPosition1, const CVec3Dfp64& _LocalPosition2, fp64 _MaxDistance);
		int AddMinDistanceConstraint(CRigidBody *_pRigidBody1, CRigidBody *_pRigidBody2, const CVec3Dfp64& _LocalPosition1, const CVec3Dfp64& _LocalPosition2, fp64 _MinDistance);

		int AddBallJointConstraint(CRigidBody *_pRigidBody1, CRigidBody *_pRigidBody2, const CVec3Dfp32& _p1, const CVec3Dfp32& _p2, fp32 _MaxAngle);
		int AddFixedPointConstraint(CRigidBody *_pRigidBody, const CVec3Dfp32& _LocalPosition, const CVec3Dfp32& _WorldPosition);
		int AddAxisConstraint(CRigidBody *_pRigidBody, const CVec3Dfp32& _Axis, fp32 _AxisLength, const CVec3Dfp32& _Position, const CVec3Dfp32& _AngleAxis, fp32 _MaxAngle);
		int AddHingeConstraint(CRigidBody *_pRigidBody1, CRigidBody *_pRigidBody2, const CVec3Dfp32& _Axis, const CVec3Dfp32& _AngleAxis, fp32 _AxisLength, const CVec3Dfp32& _Position, fp32 _MaxAngle);
		int AddBallJointToWorld(CRigidBody *_pRigidBody, const CVec3Dfp32& _LocalPosition, const CVec3Dfp32& _WorldPosition, CVec3Dfp32 _AngleAxis, fp32 _MaxAngle);

		void UpdateBallConstraint(int _iConstraint, const CVec3Dfp64& _WorldPos);
		void UpdateAxisConstraint(int _iConstraint, const CMat4Dfp64& _WorldPos, fp64 _AxisLength);

		void RemoveRigidConstraint(int _iConstraint);
		void GetConnectedObjects(int _iConstraint, CRigidBody **_pRigidBody1, CRigidBody **_pRigidBody2);
		void UpdateConnectedObject(int _iConstraint, CRigidBody* _pOld, CRigidBody* _pNew);

		template <typename T, int Size>
		class DummySet
		{
		public:

			DummySet()
			{
				m_Length = 0;
			}

			bool Add(T &_x)
			{
				for (int i = 0; i < m_Length; i++)
				{
					if (m_pV[i] == _x)
					{
						return true;
					}
				}

				if (m_Length >= Size)
				{
					return false;
				}

				m_pV[m_Length] = _x;
				m_Length++;

				return true;
			}

			bool Contains(T &_x)
			{
				for (int i = 0; i < m_Length; i++)
				{
					if (m_pV[i] == _x)
					{
						return true;
					}
				}

				return false;
			}

			T m_pV[Size];
			int m_Length;
		};

		fp64 GetSystemMass(const CRigidBody *_pRigidBody);
		fp64 DoGetSystemMass(const CRigidBody *_pRigidBody, DummySet<CRigidBody *, 16>& _Set);

		const TArray<CRigidBody *>&  GetRigidBodyList() const;
		TArray<CRigidBody *>&  GetRigidBodyList();

		const TArray<CConstraint *>& GetConstraints();

		void SetCollectCollisionEvents(bool _val)
		{
			m_CollectCollisionEvents = _val;
		}

		bool IsCollectingCollisionEvents()
		{
			return m_CollectCollisionEvents;
		}

		void Simulate(fp64 dt, int nSteps, void *_pColliderArgument1, void *_pColliderArgument2,IDynamicsDebugRenderer *_pDebugRenderer = NULL);

		void UpdatePosition(fp64 dt);
		void UpdatePositionSubdiv(fp64 _dt);
		void UpdateVelocity(fp64 dt);
		void UpdateRotation(fp64 dt); 
		void SaveFullState();
		void RestoreVelocityState();
		void RestorePositionOrientationState();

		void SetInertiaTensor(CRigidBody *_pRigidBody, const CVec3Dfp64& _Tensor)
		{
			CVec3Dfp64 tmp = _Tensor;
			tmp *= m_ScaleFactor*m_ScaleFactor;
			//			tmp *= m_ScaleFactor;
			_pRigidBody->SetInertiaTensor(tmp);
		}

		void AddImpulse(CRigidBody *_pRigidBody,
						const CVec3Dfp64& _ApplyAt, 
						const CVec3Dfp64& _Velocity,
						fp64 _Mass,
						fp64 _Restitution);

		void AddImpulse(CRigidBody *_pRigidBody,
						const CVec3Dfp32& _ApplyAt, 
						const CVec3Dfp32& _Velocity,
						fp32 _Mass,
						fp32 _Restitution);

		void AddImpulse(CRigidBody *_pRigidBody,
						const CVec3Dfp64& _ApplyAt, 
						const CVec3Dfp64& _Force);

		void AddImpulse(CRigidBody *_pRigidBody,
						const CVec3Dfp32& _ApplyAt, 
						const CVec3Dfp32& _Force);

		void AddMassInvariantImpulse(CRigidBody *_pRigidBody, 
									 const CVec3Dfp32& _ApplyAt, 
									 const CVec3Dfp32& _Force);

		void AddMassInvariantImpulse(CRigidBody *_pRigidBody, 
									 const CVec3Dfp64& _ApplyAt, 
									 const CVec3Dfp64& _Force);

		void AddForce(CRigidBody *_pRigidBody,
					  const CVec3Dfp32 _Force);

		void AddForce(CRigidBody *_pRigidBody,
					  const CVec3Dfp64 _Force);
	


		M_INLINE fp64 GetKineticEnergy(CRigidBody *_pRigidBody)
		{
			fp64 VelocitySq = _pRigidBody->GetVelocity().LengthSqr();
			fp64 Mass = _pRigidBody->GetMass();

			// 0.5*m*v^2
//			fp64 LinearEnergy = 0.5*Mass*VelocitySq*m_ScaleFactor*m_ScaleFactor;
			fp64 LinearEnergy = 0.5*Mass*VelocitySq;
			CVec3Dfp64 AngularVelocity = _pRigidBody->GetAngularVelocity();
			CVec3Dfp64 InertiaTensor = _pRigidBody->GetInertiaTensor();
			// 0.5 * w^T * I * w
			fp64 AngularEnergy = 
				  InertiaTensor[0]*AngularVelocity[0]*AngularVelocity[0]
				+ InertiaTensor[1]*AngularVelocity[1]*AngularVelocity[1]
				+ InertiaTensor[2]*AngularVelocity[2]*AngularVelocity[2];
			AngularEnergy *= 0.5;

			return LinearEnergy + AngularEnergy;
		}

		M_INLINE fp64 GetFreezeEnergyThreshold(CRigidBody *_pRigidBody, fp64 _dt)
		{
			fp64 Mass = _pRigidBody->GetMass();
			CVec3Dfp64 pg =  GetGravity()*(Mass*_dt);
			return pg.LengthSqr()/(2.0*Mass);
		}

		void SetStationary(CRigidBody *_pRigidBody, bool _stationary)
		{
			_pRigidBody->SetStationary(_stationary);

			TAP<CRigidBody *> pJustUnfreezedObjects = m_lJustUnfreezedObjects;

			for (int i = 0; i < pJustUnfreezedObjects.Len(); i++)
			{
				if (pJustUnfreezedObjects[i] == _pRigidBody)
				{
					return;
				}
			}

			m_lJustUnfreezedObjects.Add(_pRigidBody);
		}

		bool IsStationary(const CRigidBody *_pRigidBody) const
		{
			return _pRigidBody->IsStationary();
		}

		void SetGravity(const CVec3Dfp64& gravity);
		CVec3Dfp64 GetGravity();

		void ClearForces();
		void AddExternalForces();
		void ClearExternalForces();
		void AddForces(fp64 dt);

		void SetMinCollisionEventImpulse(fp64 _MinImpulse)
		{
			m_MinEventImpulse = _MinImpulse;
		}

		fp64 GetMinCollisionEventImpulse() const
		{
			return m_MinEventImpulse;
		}

		void SetMaxImpulseEvents(int _nMaxEvents)
		{
			m_nMaxImpulseEvents = _nMaxEvents;
		}

		int GetMaxImpulseEvents()
		{
			return m_nMaxImpulseEvents;
		}

		const TArray<CCollisionEvent> GetCollisionEvents();

		friend class CRigidBody;

		TArray<CRigidBodyState> m_bodystate;
	protected:
		void Step(fp64 dt,  
				  int iStep,
				  int nSteps,
				  void *_pColliderArgument1, void *_pColliderArgument2,
				  IDynamicsDebugRenderer *_pDebugRenderer = NULL,
				  bool _CollectCollisionEvents = false);

		void CollectCollisionEvents(const TArray<CContactInfo>& _lCollisionList, int _nCollisions);

		void SeperateObjects(int ncollisions);
		void UpdateInternalStates();
		void UpdateInternalStates2();

		void CheckStationary(fp64 _dt);

		void DetectCollisions2(int *startindex, TArray<CContactInfo>& collisioninfo, void *_pArgument1, void *_pArgument2);
		// TODO: Lägg till dt?
		void HandleCollision(CContactInfo& collisioninfo, fp64 epsilon);
		int ProcessCollisions(TArray<CContactInfo>& collisioninfolist, 
							  int ncollisions,
							  fp64 dt, 
						 	  fp64 epsilon, 
							  bool downorder);
		fp64 CalculateImpulseDenominator(const CContactInfo& collisioninfo);
		void CalculateImpulseDenominator(TArray<CContactInfo>& collisioninfolist, int ncollisions);
		TArray<CConstraint *> m_constraintlist;
		TArray<CContactInfo> m_tmpcollisionlist;

		TArray<CRigidConstraint> m_lRigidConstraints;

		void *m_pColliderArgument1, *m_pColliderArgument2;

		TArray<CRigidBodyState> m_savedbodystate;
		TArray<CRigidBody *> m_rigidbodylist;
		TArray<bool> m_testedcollisions;
		TArray<CRigidBody *> m_lJustUnfreezedObjects;
		int m_nobjects;
		int m_nextid;

		CVec3Dfp64 m_Gravity;
		fp64 m_ScaleFactor, m_ScaleFactorInv;

		IWorldCollider *m_pWorldCollider;

		fp64 m_MinEventImpulse;
		int m_nMaxImpulseEvents;
		int m_nMaxTemporaryImpulseEvents;

		bool m_CollectCollisionEvents;
		int m_nCollisionEvents;
		TArray<CCollisionEvent> m_lCollisionEvents;
		TArray<CCollisionEvent> m_lTemporaryCollisionEvents;

		int m_CollisionPrecision;

		bool m_bSimulating;

		CRigidBody m_DummyRigidBody;
	};

#define CDYNAMICSENGINE_MAXCOLLIDERTABLESIZE 255



#endif
