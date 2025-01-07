/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			WObj_PhysCluster.h

	Author:			Anders Ekermo

	Copyright:		Copyright Starbreeze Studios AB 2005

	Contents:		CWPhys_Cluster

	History:		
		061004:		Created File
\*____________________________________________________________________________________________*/
#ifndef __WObj_PhysCluster_h__
#define __WObj_PhysCluster_h__

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			PhysClusterObject

	Comments:		Stripped-down object for rigid bodies
\*____________________________________________________________________*/
class CWPhys_ClusterObject
{

public:

	CWO_PhysicsPrim		m_PhysPrim;
	CWD_RigidBody2*		m_pRB;
	CMat4Dfp32			m_Transform;
	CVelocityfp32		m_Velocity;

	CWPhys_ClusterObject()
	{
		m_PhysPrim.m_PrimType = OBJECT_PRIMTYPE_NONE;
		m_pRB = NULL;
	}
};


enum ePhysClusterFlags
{

	PHYSCLUSTER_NONE	= 0,
	PHYSCLUSTER_SKIPFIRSTTRANSFORM	= 1 << 0,	//Don't update transform matrix first frame

};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			PhysCluster

	Comments:		Contains information about a cluster of Rigid Bodies
\*____________________________________________________________________*/
class CWPhys_Cluster
{

public:

	TThinArray<CWPhys_ClusterObject> m_lObjects;
	uint32	m_Flags;

	CWPhys_Cluster() : m_Flags(0) { };

// Helpers
//-----------------------------------------------------------------------------

	CBox3Dfp32 GetBoundingBox() 
	{
		CBox3Dfp32 Ret;
		Ret.m_Min = CVec3Dfp32(3.4e+38f,3.4e+38f,3.4e+38f);
		Ret.m_Max = CVec3Dfp32(-3.4e+38f,-3.4e+38f,-3.4e+38f);

		TAP<CWPhys_ClusterObject> pObj = m_lObjects;
		for(int i = 0;i < pObj.Len();i++)
		{
			CWPhys_ClusterObject &PCO = pObj[i];

			switch( PCO.m_PhysPrim.m_PrimType )
			{

			case OBJECT_PRIMTYPE_BOX:
				{
					CVec3Dfp32 Dim;
					CVec3Dfp32 BoxDim = PCO.m_PhysPrim.GetDim();
					const CMat4Dfp32 &Tr = PCO.m_Transform;
					Dim.k[0] = Abs(BoxDim.k[0] * Tr.k[0][0]) + Abs(BoxDim.k[1] * Tr.k[1][0]) + Abs(BoxDim.k[2] * Tr.k[2][0]);
					Dim.k[1] = Abs(BoxDim.k[0] * Tr.k[0][1]) + Abs(BoxDim.k[1] * Tr.k[1][1]) + Abs(BoxDim.k[2] * Tr.k[2][1]);
					Dim.k[2] = Abs(BoxDim.k[0] * Tr.k[0][2]) + Abs(BoxDim.k[1] * Tr.k[1][2]) + Abs(BoxDim.k[2] * Tr.k[2][2]);
					CBox3Dfp32 Bx;
					Bx.m_Min = PCO.m_Transform.GetRow(3) - Dim;
					Bx.m_Max = PCO.m_Transform.GetRow(3) + Dim;
					Ret.Expand(Bx);
				}
				break;

			case OBJECT_PRIMTYPE_CAPSULE:
				{
					CBox3Dfp32 Bx;
					CVec3Dfp32 Dim = PCO.m_PhysPrim.GetDim();
					const CMat4Dfp32 &Mt = PCO.m_Transform;
					CVec3Dfp32 Offs( Abs(Mt.k[0][0]),Abs(Mt.k[0][1]),Abs(Mt.k[0][2]) );

					Offs *= Dim.k[0];
					Bx.m_Min = Mt.GetRow(3) - Offs;
					Bx.m_Max = Mt.GetRow(3) + Offs;
					Bx.m_Min -= CVec3Dfp32(Dim.k[1],Dim.k[1],Dim.k[1]);
					Bx.m_Max += CVec3Dfp32(Dim.k[1],Dim.k[1],Dim.k[1]);
					Ret.Expand(Bx);
				}
				break;

			default:
				M_ASSERT(false,"Unsupported PhysCluster object type!");
				break;

			}
		}

		return Ret;
	}


	//Quick-and-dirty find closest
	//More correct would be to find the closest point on each volume and compare
	uint32 QuickGetClosest(const CVec3Dfp32 &_Pt) const
	{
		uint32 iBest = 0;
		fp32 BestDist = _FP32_MAX;

		TAP<const CWPhys_ClusterObject> pObj = m_lObjects;
		for(int i = 0;i < pObj.Len();i++)
		{
			CVec3Dfp32 Delta = pObj[i].m_Transform.GetRow(3) - _Pt;
			fp32 Dist = Delta.LengthSqr();
			if( Dist < BestDist )
			{
				iBest = i;
				BestDist = Dist;
			}
		}

		return iBest;
	}


// Volume
//-----------------------------------------------------------------------------

	static fp32 GetObjectVolume(const CVec3Dfp32 &_Dim,int _Type)
	{
		switch(_Type)
		{

		case OBJECT_PRIMTYPE_BOX:
			return _Dim.k[0] * _Dim.k[1] * _Dim.k[2] * 8.0f;

		case OBJECT_PRIMTYPE_CAPSULE:
			return (3.1415926f * Sqr(_Dim.k[1]) * _Dim.k[0] * 2.0f) +
				(3.1415926f * 4.0f * (Sqr(_Dim.k[1]) * _Dim.k[1]) / 3.0f);

		}
		return 0.0f;
	}

	static fp32 GetObjectVolume(const CWO_PhysicsPrim &_PhysPrim)
	{
		return GetObjectVolume(_PhysPrim.GetDim(),_PhysPrim.m_PrimType);
	}


// Inertia
//-----------------------------------------------------------------------------
	
	static CVec3Dfp32 GetObjectInertia(const CVec3Dfp32 &_Dim,int _Type,fp32 _Mass)
	{
		switch(_Type)
		{

		case OBJECT_PRIMTYPE_BOX:
			return  CVec3Dfp32( Sqr(_Dim.k[1]*2.0f) + Sqr(_Dim.k[2]*2.0f),
								Sqr(_Dim.k[0]*2.0f) + Sqr(_Dim.k[2]*2.0f),
								Sqr(_Dim.k[0]*2.0f) + Sqr(_Dim.k[1]*2.0f) ) * (_Mass/12.0f);
		
	// NOT CORRECT! this is for plain cylinders!
	//find the real inertia sometime, should be somewhere between sphere & cylinder
	//Capsule is assumed to be stretched out along Z
		case OBJECT_PRIMTYPE_CAPSULE:
			{
				fp32 Flat = (_Mass / 12.0f) * (3.0f * Sqr(_Dim.k[1]) + Sqr(_Dim.k[0]*2.0f + _Dim.k[1]));
				return CVec3Dfp32(0.5f * _Mass * Sqr(_Dim.k[1]),Flat,Flat);
			};

		}
		return CVec3Dfp32(0,0,0);
	}

	static CVec3Dfp32 GetObjectInertia(const CWO_PhysicsPrim &_PhysPrim,fp32 _Mass)
	{
		return GetObjectInertia(_PhysPrim.GetDim(),_PhysPrim.m_PrimType,_Mass);
	}


// Phys
//-----------------------------------------------------------------------------

	void AddForceToAll(CWorld_Server * _pWServer,const CVec3Dfp32 &_Force)
	{
		TAP<CWPhys_ClusterObject> pPO = m_lObjects;
		for(uint i = 0;i < pPO.Len();i++)
		{
			_pWServer->Phys_AddForce(pPO[i].m_pRB,_Force);
		}
	}

};

#endif
