#include "pch.h"
#include "WDynamics2.h"

#include "../../../XRModels/Model_BSP2/WBSP2Model.h"
#include "../../../XR/Phys/SeparatingAxis.h"
#include "../WDynamicsConvexCollision.h"
#include "../WPhysState_Hash.h"

#include "../Server/WServer_Core.h"
#include "../WObjects/WObj_Game.h"

#include "../WObjects/WObj_PhysCluster.h"

enum
{
	class_CharPlayer =			MHASH6('CWOb','ject','_','Char','Play','er'),
	class_Object = 				MHASH5('CWOb','ject','_','Obje','ct'),
	class_WorldSpawn = 			MHASH6('CWOb','ject','_','Worl','dSpa','wn'),
	class_WorldPlayerPhys = 	MHASH7('CWOb','ject','_','Worl','dPla','yerP','hys'),
	class_SwingDoor =			MHASH6('CWOb','ject','_','Swin','g','Door'),
};


//Put these in a better place
//and rewrite them to vec128. Should've been done in the first place,
//but it needed to fit the rest of the code...

/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
    Function:	 Capsule-capsule (line-SSV) collision
    Parameters:
		_Cen1:				Center point of first capsule
		_Dir1:				Direction vector of first capsule
		_Len1:				Half-length of first capsule
		_Rad1:				Radius of first capsule
		_Cen2:				Center point of second capsule
		_Dir2:				Direction vector of second capsule
		_Len2:				Length of second capsule
		_Rad2:				Radius of second capsule
		_pCollisionInfo:	Return values
    Returns:    Number of contacts (1 or 0)
    Comments:   Simple closest point of 2 line segments- test 
				DOES NOT handle degenerate capsules (spheres)
\*____________________________________________________________________*/ 
int Phys_Collide_Capsule(const CVec3Dfp32 &_Cen1,const CVec3Dfp32 &_Dir1,fp32 _Len1,fp32 _Rad1,
						 const CVec3Dfp32 &_Cen2,const CVec3Dfp32 &_Dir2,fp32 _Len2,fp32 _Rad2,
						 CCollisionInfo* _pCollisionInfo)
{	
	fp32 Proj1,Proj2;
	{
		CVec3Dfp32 Diff = _Cen1 - _Cen2;
		Proj1 = _Dir1 * Diff;
		Proj2 = _Dir2 * Diff;
	}

	fp32 Dt = _Dir1 * _Dir2;
	fp32 Denom = 1.0f - Sqr(Dt);

	fp32 t1,t2;

	//Lines are parallel
	if( Denom == 0.0f )
		t1 = 0.0f;
	else
		t1 = Clamp((Dt*Proj2 - Proj1) / Denom,-_Len1,_Len1);

	t2 = Dt * t1 + Proj2;
	if( t2 < -_Len2 )
	{
		t2 = -_Len2;
		t1 = Clamp(-Proj1 - Dt*_Len2,-_Len1,_Len1);
	}
	else if( t2 > _Len2 )
	{
		t2 = _Len2;
		t1 = Clamp(Dt*_Len2 - Proj1,-_Len1,_Len1);
	}

	CVec3Dfp32 Pos1 = (_Cen1 + _Dir1 * t1);
	CVec3Dfp32 Pos2 = (_Cen2 + _Dir2 * t2);
	CVec3Dfp32 Diff = Pos1 - Pos2;
	
	fp32 Ln = Diff.LengthSqr();
	if( Ln > Sqr(_Rad1 + _Rad2) ) return 0;

	Ln = M_Sqrt(Ln);
	Diff *= (1.0f / Ln);
	fp32 Depth = (_Rad1 + _Rad2) - Ln;
	_pCollisionInfo->m_bIsCollision = true;
	_pCollisionInfo->m_Distance = Depth;
	_pCollisionInfo->m_Pos = Pos1 + Diff * (_Rad1 + Depth*0.5f);
	_pCollisionInfo->m_Plane.n = Diff;

	return 1;
}

//To here



//#pragma check_stack(off)
//#pragma optimize("", off)
//#pragma inline_depth(0)

//#define BUOYANCYGRAVITY (1.81f)
//#define BUOYANCYLIQUIDDRAG (1.0f)


float BUOYANCYGRAVITY = 2.81f;
float BUOYANCYLIQUIDDRAG = 0.4f;
float BUOYANCYLIQUIDANGULARDAMPENING = 0.995f;

class CBouyancyFunctions
{
public:
	struct CBouyancyForce 
	{
		CBouyancyForce() {}

		CBouyancyForce(CVec3Dfp32 _Force, CVec3Dfp32 _ApplyAt, fp32 _VolumeInLiquid)
		{
			m_Force = _Force;
			m_ApplyAt = _ApplyAt;
			m_VolumeInLiquid = _VolumeInLiquid;
		}

		CVec3Dfp32 m_Force;
		CVec3Dfp32 m_ApplyAt;
		fp32 m_VolumeInLiquid;
	};

	template <class T>
	M_INLINE static int SelectMax(T x1, T x2, T x3)
	{
		return x1 > x2 ? (x1 > x3 ? 0 : 2) : (x2 > x3 ? 1 : 2);
	}

	static void RenderSphere(CWorld_Server *_pServer, const CVec3Dfp32& _Position, fp32 _Radius, CPixel32 _Colour)
	{
		CMat4Dfp32 T;
		T.Unit();
		T.GetRow(3) = _Position;
		_pServer->Debug_RenderSphere(T, _Radius, _Colour, 1.0f/20.0f, false);
	}

	static CBouyancyForce CalculateBouyancyForce(CWorld_Server *_pServer, int _iObject, const CMat4Dfp32 &_T, CBox3Dfp32 _Box)
	{
		MSCOPESHORT(CBouyancyFunctions::CalculateBouyancyForce);

		CVec3Dfp32 Min = _Box.m_Min;
		CVec3Dfp32 Max = _Box.m_Max;

		CVec3Dfp32 Size = (_Box.m_Max - _Box.m_Min) * (10.0f / 32.0f);
		fp32 Volume = Size.k[0] * Size.k[1] * Size.k[2];

		COBBfp32 Obb, ObbTransformed;
		Obb.Create(_Box);
		Obb.Transform(_T, ObbTransformed);

		int MaxAxis = SelectMax(M_Fabs(ObbTransformed.m_A[0][2] * ObbTransformed.m_E[0]),
			M_Fabs(ObbTransformed.m_A[1][2] * ObbTransformed.m_E[1]),
			M_Fabs(ObbTransformed.m_A[2][2] * ObbTransformed.m_E[2]));

		CVec3Dfp32 Center = (_Box.m_Max - _Box.m_Min) * 0.5f;

		CVec3Dfp32 ExtentAlongAxis(0.0f);
		ExtentAlongAxis[MaxAxis] = _Box.m_Max[MaxAxis] - _Box.m_Min[MaxAxis];
		CVec3Dfp32 p0 = (ExtentAlongAxis * 0.5f) * _T;
		CVec3Dfp32 p1 = (-ExtentAlongAxis * 0.5f) * _T;
		//p0 *= 32.0f;
		//p1 *= 32.0f;

		CVec3Dfp32 SaveP0 = p0;
		CVec3Dfp32 SaveP1 = p1;

		CXR_MediumDesc MediumDescMid;

		CCollisionInfo CollisionInfo2;

		CVec3Dfp32 p0tmp = p0;
		CVec3Dfp32 p1tmp = p0;
		p1tmp[2] = p1[2] + 72.0f;
		p0tmp[2] = p0[2] - 72.0f;

		//RenderSphere(_pServer, _T.GetRow(3) * 32.0f, 16.0f, 0xff0000ff);

#ifndef M_RTM
		RenderSphere(_pServer, p1tmp, 8.0f, 0xff0000ff);
		RenderSphere(_pServer, p0tmp, 8.0f, 0xffffff00);
#endif

		CollisionInfo2.m_CollisionType = CXR_COLLISIONTYPE_PROJECTILE;
		bool IsIntersecting = _pServer->Phys_IntersectLine(p1tmp, p0tmp, OBJECT_FLAGS_PROJECTILE, OBJECT_FLAGS_PHYSMODEL | OBJECT_FLAGS_PHYSOBJECT, XW_MEDIUM_LIQUID, &CollisionInfo2);



		CXR_MediumDesc MediumDesc, MediumDesc1, MediumDesc2;

		_pServer->Phys_GetMedium(p0, &MediumDesc1);
		_pServer->Phys_GetMedium(p1, &MediumDesc2);

		bool SubMerged = false;
		if (MediumDesc1.m_Density == MediumDesc2.m_Density && MediumDesc1.m_Density > 0.01f)
		{
			SubMerged = true;
		}

#ifndef M_RTM
		RenderSphere(_pServer, p0 , 4.0f, 0xffff0000);
		RenderSphere(_pServer, p1, 4.0f, 0xffff0000);
#endif


#if 1
		if (!IsIntersecting)
		{
			if (SubMerged)
			{
				if (MediumDesc1.m_Density < 0.01)
					return CBouyancyForce(CVec3Dfp32(0.0f), CVec3Dfp32(0.0f), 0.0f);

				fp32 ForceMag = Volume * MediumDesc1.m_Density;
				CVec3Dfp32 Force = CVec3Dfp32(0.0f, 0.0f, ForceMag);

				Force *= BUOYANCYGRAVITY;
				CVec3Dfp32 ApplyAt(0.0f);

				ApplyAt = (p0 + p1) * 0.5f;
				//				ApplyAt += CVec3Dfp32(5.0f, 5.0f, 0.0f);

				return CBouyancyForce(Force, ApplyAt, Volume);
			}
			else
				return CBouyancyForce(CVec3Dfp32(0.0f), CVec3Dfp32(0.0f), 0.0f);
		}
#endif

		CVec3Dfp32 Normal = CollisionInfo2.m_Plane.n;
		Normal[0] = Clamp(Normal[0], -0.1f, 0.1f);
		Normal[1] = Clamp(Normal[1], -0.1f, 0.1f);
		Normal.Normalize(); 

		_pServer->Phys_GetMedium(CollisionInfo2.m_Pos - CVec3Dfp32(0.0f, 0.0f, 1.0f), &MediumDesc);


		fp32 FooBar10= M_Fabs(MediumDesc1.m_Density - MediumDesc2.m_Density);



#ifndef M_RTM
		if (!SubMerged)
			RenderSphere(_pServer, CollisionInfo2.m_Pos, 8.0f, 0xff00ffff);
		else
			RenderSphere(_pServer, CollisionInfo2.m_Pos, 8.0f, 0xffffffff);
#endif


		CVec3Dfp32 MidP;

		p0 = SaveP0;
		p1 = SaveP0;
		p1[2] = SaveP1[2];
		MidP = CollisionInfo2.m_Pos;


		CMat4Dfp32 TInv;
		_T.InverseOrthogonal(TInv);

		CVec3Dfp32 MidPLocal = MidP * TInv;

		fp32 VolumeInLiquid = 0.0f;
		bool Upper = ObbTransformed.m_A[MaxAxis] * CVec3Dfp32(0.0f, 0.0f, 1.0f) > 0;
		CVec3Dfp32 TmpSize = Size;
		TmpSize[MaxAxis] = 1.0f;
		fp32 CrossSectionArea = TmpSize[0] * TmpSize[1] * TmpSize[2];


		fp32 a = 0.0f;
		CVec3Dfp32 FaceCenterPoint;

		CVec3Dfp32 ApplyAt(0.0f);
		int nVolumePoints = 0;

		CVec3Dfp32 vFaceCenterOfMass[3];
		fp32 vdV[3] = {0.0f, 0.0f, 0.0f};

		for (int i = 0; i < 3; i++)
		{
			CVec3Dfp32 TmpExt = ObbTransformed.m_E * 10.0f / 32.0f;

			fp32 FooVol = TmpExt[0] * 2.0f * TmpExt[1] * 2.0f * TmpExt[2] * 2.0f;

			TmpExt *= 2.0f;

			TmpExt[i] = 1.0f;
			a = TmpExt[0] * 1.0f * TmpExt[1] * 1.0f * TmpExt[2] * 1.0f;

			fp32 FaceSign = 1.0f;

			if (ObbTransformed.m_A[i][2] < 0.0f)
				FaceSign = 1.0f;
			else
				FaceSign = -1.0f;

			FaceCenterPoint = ObbTransformed.m_C + ObbTransformed.m_A[i] * ObbTransformed.m_E[i] * FaceSign;

			const CVec3Dfp32 vATimesE[3] = { ObbTransformed.m_A[0] * ObbTransformed.m_E[0], 
				ObbTransformed.m_A[1] * ObbTransformed.m_E[1], 
				ObbTransformed.m_A[2] * ObbTransformed.m_E[2] };

			int index1 = 1;
			int index2 = 2;

			if (i == 1)
			{
				index1 = 0;
				index2 = 2;
			}
			else if (i == 2)
			{
				index1 = 0;
				index2 = 1;
			}

			CVec3Dfp32 FacePoint1 = FaceCenterPoint + vATimesE[index1] + vATimesE[index2];
			CVec3Dfp32 FacePoint2 = FaceCenterPoint - vATimesE[index1] - vATimesE[index2];
			CVec3Dfp32 FacePoint3 = FaceCenterPoint - vATimesE[index1] + vATimesE[index2];
			CVec3Dfp32 FacePoint4 = FaceCenterPoint + vATimesE[index1] - vATimesE[index2];

			CVec3Dfp32 SurfacePoint = FaceCenterPoint;
			SurfacePoint[2] = MidP[2];

			fp32 Dist1 = M_Fabs(SurfacePoint[2] - FacePoint1[2]);
			fp32 Dist2 = M_Fabs(SurfacePoint[2] - FacePoint2[2]);
			fp32 Dist3 = M_Fabs(SurfacePoint[2] - FacePoint3[2]);
			fp32 Dist4 = M_Fabs(SurfacePoint[2] - FacePoint4[2]);

			fp32 zMaxDist = Dist1;
			zMaxDist = ::Max(zMaxDist, Dist2);
			zMaxDist = ::Max(zMaxDist, Dist3);
			zMaxDist = ::Max(zMaxDist, Dist4);

			fp32 zDistSum = (Dist1 + Dist2 + Dist3 + Dist4) / zMaxDist;


			CVec3Dfp32 FaceCenterOfMass = FacePoint1 * (Dist1 / zMaxDist / zDistSum) +
				FacePoint2 * (Dist2 / zMaxDist / zDistSum) +
				FacePoint3 * (Dist3 / zMaxDist / zDistSum) +
				FacePoint4 * (Dist4 / zMaxDist / zDistSum);

			vFaceCenterOfMass[i] = FaceCenterOfMass;

			if (FaceCenterPoint[2] <= MidP[2])
			{
				ApplyAt += FaceCenterPoint;
				nVolumePoints++;

				fp32 dV = ((SurfacePoint - FaceCenterPoint) * ObbTransformed.m_A[i]) * a * 10.0 / 32.0f;
				dV = M_Fabs(dV);

				vdV[i] = dV;
				VolumeInLiquid += dV;
			}				
		}

		if (VolumeInLiquid < TNumericProperties<fp32>::Epsilon())
		{
			// TODO: Varför sker detta?
			return CBouyancyForce(CVec3Dfp32(0.0f), CVec3Dfp32(0.0f), 0.0f);
		}

		ApplyAt *= 1.0f / fp32(nVolumePoints);

		ApplyAt = CVec3Dfp32(0.0f);
		for (int i = 0; i < 3; i++)
		{
			ApplyAt += vFaceCenterOfMass[i] * vdV[i] / VolumeInLiquid;
		}

		VolumeInLiquid = Clamp(VolumeInLiquid, 0.0f, Volume);

		fp32 ForceMag = VolumeInLiquid * MediumDesc.m_Density;
		CVec3Dfp32 Force = Normal * ForceMag;

		Force *= BUOYANCYGRAVITY;
		return CBouyancyForce(Force, ApplyAt, VolumeInLiquid);
	}

};



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWD_ConstraintDescriptor
|__________________________________________________________________________________________________
\*************************************************************************************************/

void CWD_ConstraintDescriptor::Read(CCFile* _pFile, CWorld_Server* _pWServer)
{
	_pFile->ReadLE(m_ID);
	_pFile->ReadLE(m_Type);
	_pFile->ReadLE(m_lVectorParams->k, sizeof(m_lVectorParams) / sizeof(m_lVectorParams->k[0]));
	_pFile->ReadLE(m_lScalarParms, MAXSCALARPARAMS);

	uint32 GUID_A, GUID_B;
	_pFile->ReadLE(GUID_A);
	_pFile->ReadLE(GUID_B);

	_pFile->ReadLE(m_iSubObjectA);
	_pFile->ReadLE(m_iSubObjectB);

	m_iObjectA = _pWServer->Object_GetIndex(GUID_A);
	m_iObjectB = _pWServer->Object_GetIndex(GUID_B);
}

void CWD_ConstraintDescriptor::Write(CCFile* _pFile, CWorld_Server* _pWServer) const
{
	_pFile->WriteLE(m_ID);
	_pFile->WriteLE(m_Type);
	_pFile->WriteLE(m_lVectorParams->k, sizeof(m_lVectorParams) / sizeof(m_lVectorParams->k[0]));
	_pFile->WriteLE(m_lScalarParms, MAXSCALARPARAMS);

	uint32 GUID_A = _pWServer->Object_GetGUID(m_iObjectA);
	uint32 GUID_B = _pWServer->Object_GetGUID(m_iObjectB);
	_pFile->WriteLE(GUID_A);
	_pFile->WriteLE(GUID_B);

	_pFile->WriteLE(m_iSubObjectA);
	_pFile->WriteLE(m_iSubObjectB);
}



/*************************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯
| CWD_WorldModelCollider
|__________________________________________________________________________________________________
\*************************************************************************************************/
bool CWD_WorldModelCollider::Collide(const CWD_DynamicsWorld *_world, CWD_RigidBody2 *_pBody, CWD_ContactInfo *_pContactInfo, void *_pArgument1, void *_pArgument2)
{
	M_ASSERT(false, "!");
	return false;
}

static void GetBoundBox(CMapData *_pWorldData, const CWObject_CoreData *_pObj, CBox3Dfp32& _Box)
{
	const CWO_PhysicsPrim& Prim = _pObj->GetPhysState().m_Prim[0];
	CVec3Dfp32 Offset = Prim.GetOffset();

	if (Prim.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL)
	{
		CXR_Model* pModel = _pWorldData->GetResource_Model(Prim.m_iPhysModel);
		M_ASSERT(pModel, "No Model!");
		CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();
		M_ASSERT(pPhysModel, "No Physmodel!");

		pPhysModel->GetBound_Box(_Box, Prim.m_PhysModelMask);
		_Box.m_Min += Offset;
		_Box.m_Max += Offset;
	}
	else if (Prim.m_PrimType == OBJECT_PRIMTYPE_BOX)
	{
		CVec3Dfp32 Dim = Prim.GetDim();
		_Box.m_Min = Offset - Dim;
		_Box.m_Max = Offset + Dim;
	}
	else if (Prim.m_PrimType == OBJECT_PRIMTYPE_SPHERE)
	{
		//TODO! proper sphere stuff
		CVec3Dfp32 Dim = Prim.GetDim();
		_Box.m_Min = Offset - Dim;
		_Box.m_Max = Offset + Dim;
	}
	else 
		M_ASSERT(false,"Unsupported primtype!");
}


static void GetClusterItemBoundBox(const CWPhys_ClusterObject &_PO,const CVec3Dfp32 &_Dim,CBox3Dfp32 &_Box)
{
	switch( _PO.m_PhysPrim.m_PrimType )
	{

	case OBJECT_PRIMTYPE_BOX:
		{
			const CMat4Dfp32 &Mat = _PO.m_Transform;
			CVec3Dfp32 Center = Mat.GetRow(3);
			CVec3Dfp32 RealDim;
			
			RealDim.k[0] = M_Fabs(_Dim.k[0] * Mat.k[0][0]) + M_Fabs(_Dim.k[1] * Mat.k[1][0]) + M_Fabs(_Dim.k[2] * Mat.k[2][0]);
			RealDim.k[1] = M_Fabs(_Dim.k[0] * Mat.k[0][1]) + M_Fabs(_Dim.k[1] * Mat.k[1][1]) + M_Fabs(_Dim.k[2] * Mat.k[2][1]);
			RealDim.k[2] = M_Fabs(_Dim.k[0] * Mat.k[0][2]) + M_Fabs(_Dim.k[1] * Mat.k[1][2]) + M_Fabs(_Dim.k[2] * Mat.k[2][2]);

			_Box.m_Max = Center + RealDim;
			_Box.m_Min = Center - RealDim;
		}
		break;

	case OBJECT_PRIMTYPE_CAPSULE:
		{
			CBox3Dfp32 Bx;
			const CMat4Dfp32 &Mt = _PO.m_Transform;
			CVec3Dfp32 Offs( Abs(Mt.k[0][0]),Abs(Mt.k[0][1]),Abs(Mt.k[0][2]) );

			Offs *= _Dim.k[0];
			Bx.m_Min = Mt.GetRow(3) - Offs;
			Bx.m_Max = Mt.GetRow(3) + Offs;
			Bx.m_Min -= CVec3Dfp32(_Dim.k[1],_Dim.k[1],_Dim.k[1]);
			Bx.m_Max += CVec3Dfp32(_Dim.k[1],_Dim.k[1],_Dim.k[1]);
			_Box = Bx;
		}
		break;

	default:
		M_ASSERT(false,"Unsupported PhysCluster object type!");
		break;

	}
}


static int CollideSolidBsp(const CWD_DynamicsWorld *_pWorld,
						   const CWD_RigidBody2 *_pBody1, 
						   CXR_IndexedSolidContainer32 *_pSolidContainer,
						   const CWObject_CoreData *pObj2, 
						   TAP_RCD<CWD_ContactInfo> _pContactInfo,
						   CMapData *_pWorldData,
						   CWorld_PhysState *_pPhysState)
{
	MSCOPESHORT(CollideSolidBsp);

	const CWObject_CoreData *pObj1 = (CWObject_CoreData *) _pBody1->m_pUserData;

	CBox3Dfp32 Box;
	GetBoundBox(_pWorldData, pObj1, Box);

	const CWO_PhysicsPrim& Prim1 = pObj1->GetPhysState().m_Prim[0];
	const CWO_PhysicsPrim& Prim2 = pObj2->GetPhysState().m_Prim[0];

	CXR_Model* pModel2 = _pWorldData->GetResource_Model(Prim2.m_iPhysModel);
	M_ASSERT( pModel2, "" );
	CXR_PhysicsModel* pPhysModel2 = pModel2->Phys_GetInterface();
	M_ASSERT( pPhysModel2, "" );

//	const CMat4Dfp32 &P1 = _pWorld->GetTransform(_pBody1->m_iRB);
	CMat4Dfp32 P1 = _pWorld->GetTransform(_pBody1->m_iRB);
	P1.GetRow(3) *= 32.0f;
	const CMat4Dfp32 &BspTransform = pObj2->GetPositionMatrix();

	const int MaxBspCollisions = 100;
	// TODO: static!!! temp hack due to constructor (not thread-safe!)
	uint8 CollisionInfoBuf[MaxBspCollisions * sizeof(CCollisionInfo)];
	CCollisionInfo *CollisionInfo = (CCollisionInfo *) CollisionInfoBuf;
	//CCollisionInfo CollisionInfo[MaxBspCollisions];
	CXR_PhysicsContext Context(BspTransform);
	Context.m_PhysGroupMaskThis = Prim2.m_PhysModelMask;
	Context.m_PhysGroupMaskCollider = Prim1.m_PhysModelMask;
	Context.m_pWC = _pPhysState->Debug_GetWireContainer();

	int nCollisions = pPhysModel2->Phys_CollideBSP2(&Context, _pSolidContainer, P1, Prim1.GetOffset() - _pBody1->m_CenterOfMass, pObj1->GetPhysState().m_MediumFlags, CollisionInfo, MaxBspCollisions);

	if (nCollisions > 30)
	{
//		M_BREAKPOINT;
	}

	M_ASSERT(nCollisions <= MaxBspCollisions, "!");

	CWD_RigidBody2 *pHack = (CWD_RigidBody2 *) _pBody1;

	int nTotCol = 0;
	for (int i = 0; i < MaxBspCollisions && i < nCollisions && i < _pContactInfo.Len(); i++)
	{	
		CVec4Dfp32 N;
		N = -CollisionInfo[i].m_Plane.n;
		N[3] = 0.0f;

		pHack->m_CollisionToWorldCluster.AddNormal(N);

		_pContactInfo[nTotCol].m_iRB1= _pBody1->m_iRB;
		_pContactInfo[nTotCol].m_iRB2 = 0;
		_pContactInfo[nTotCol].m_Normal = N;
		_pContactInfo[nTotCol].m_PointOfCollision = CollisionInfo[i].m_Pos * (1.0f / 32.0f);
		_pContactInfo[nTotCol].m_Distance = M_Fabs(CollisionInfo[i].m_Distance) * (1.0f / 32.0f);
		_pContactInfo[nTotCol].m_bIsColliding = CollisionInfo[i].m_bIsCollision;
		_pContactInfo[nTotCol].m_BiasVelocity = M_VZero();
		_pContactInfo[nTotCol].m_UserData1 = pObj1->m_iObject;
		_pContactInfo[nTotCol].m_UserData2 = pObj2->m_iObject;
		_pContactInfo[nTotCol].m_Friction_Elasticity = CVec4Dfp32(CollisionInfo[i].m_Friction * 0.75f, CollisionInfo[i].m_Friction, 0.4f, 0.0f);

		nTotCol++;
	}
	return nTotCol;
}

#define N_MAX_COLL_HACK 3987

class CCollisionHash : public CMap32
{
public:
	uint m_nColl;
	enum { EMaxCollisions = 1000 };

	CCollisionHash()
		: m_nColl(0)
	{
		Create(EMaxCollisions, 10);
	}

	M_INLINE static uint32 GetHashValue(uint id1, uint id2) { return id1 + id2*N_MAX_COLL_HACK; }

	bool Test(uint id1, uint id2)
	{
		uint Hash = GetHashValue(id1, id2);
		return (GetIndex(Hash) != -1);
	}

	void Add(uint id1, uint id2)
	{
		if (!(m_nColl < EMaxCollisions-1))
		{
			int breakme =0 ;
		}
		M_ASSERTHANDLER(m_nColl < EMaxCollisions-1, "Too many collisions!", return);
		Insert(m_nColl++, GetHashValue(id1, id2));
		Insert(m_nColl++, GetHashValue(id2, id1));
	}

	void SetEmpty()
	{
		m_nColl = 0;

		uint n = m_lHash.Len();
		for (uint i = 0; i < n; i++)
			m_pHash[i] = -1;

		n = m_lIDInfo.Len();
		CHashIDInfo* pIDI = m_lIDInfo.GetBasePtr();
		for (uint i = 0; i < n; i++)
			pIDI[i].ClearLinkage();
	}
};

M_INLINE static CMat4Dfp32 ConvertMatrix(const CMat4Dfp64& _Mat)
{
	CMat4Dfp32 ret;
	ret.Unit();
	for (int i=0; i<4; i++)
		for (int j=0; j<4; j++)
			ret.k[i][j] = _Mat.k[i][j];
	return ret;
}


static void SetupCollisionSingle(const CWD_DynamicsWorld *_pWorld,
						   const CWObject_CoreData *_pObject,
						   CMapData *_pWorldData,
						   CMat4Dfp32& _Transform)
{
	const CWO_PhysicsPrim& PhysicsPrim = _pObject->GetPhysState().m_Prim[0];

	const CWD_RigidBody2 *pRB = _pObject->m_pRigidBody2;

	CVec3Dfp32 CenterOfMass(0.0f);
	CMat4Dfp64 tmp;

	// Get transform
	CMat4Dfp32 Transform;
	if (_pObject->m_pRigidBody2)
	{
		Transform = _pWorld->GetTransform(pRB->m_iRB);
		CenterOfMass = pRB->m_CenterOfMass * 1.0f;
	}
	else
	{
		Transform = _pObject->GetPositionMatrix();
	}

	Transform.GetRow(3) *= 32.0f;

	// Correct transform for center of mass and offset
	CVec3Dfp32 TotalOffset = PhysicsPrim.GetOffset() - CenterOfMass;

	Transform.GetRow(3) = TotalOffset * Transform;

	_Transform = Transform;
}


static void SetupCollision(const CWD_DynamicsWorld *_pWorld,
						   const CWObject_CoreData *_pObject1,
						   const CWObject_CoreData *_pObject2,
						   CMapData *_pWorldData,
						   CMat4Dfp32& _Transform1, 
						   CMat4Dfp32& _Transform2, 
						   CXR_PhysicsContext& _Context)
{
	const CWO_PhysicsPrim& PhysicsPrim1 = _pObject1->GetPhysState().m_Prim[0];
	const CWO_PhysicsPrim& PhysicsPrim2 = _pObject2->GetPhysState().m_Prim[0];

	const CWD_RigidBody2 *pRB1 = _pObject1->m_pRigidBody2;
	const CWD_RigidBody2 *pRB2 = _pObject2->m_pRigidBody2;

	CVec3Dfp32 CenterOfMass1(0.0f);
	CVec3Dfp32 CenterOfMass2(0.0f);
	CMat4Dfp64 tmp;

	// Get transform
	CMat4Dfp32 Transform1, Transform2;
	if (_pObject1->m_pRigidBody2)
	{
		Transform1 = _pWorld->GetTransform(pRB1->m_iRB);
		CenterOfMass1 = pRB1->m_CenterOfMass * 1.0f;
	}
	else
	{
		Transform1 = _pObject1->GetPositionMatrix();
	}

	if (_pObject2->m_pRigidBody2)
	{
		Transform2 = _pWorld->GetTransform(pRB2->m_iRB);
		CenterOfMass2 = pRB2->m_CenterOfMass * 1.0f;
	}
	else
	{
		Transform2 = _pObject2->GetPositionMatrix();
	}

	Transform1.GetRow(3) *= 32.0f;
	Transform2.GetRow(3) *= 32.0f;


	// Correct transform for center of mass and offset
	CVec3Dfp32 TotalOffset1 = PhysicsPrim1.GetOffset() - CenterOfMass1;
	CVec3Dfp32 TotalOffset2 = PhysicsPrim2.GetOffset() - CenterOfMass2;

	//TotalOffset1 *= -1.0f;
	//TotalOffset2 *= -1.0f;

	Transform1.GetRow(3) = TotalOffset1 * Transform1;
	Transform2.GetRow(3) = TotalOffset2 * Transform2;

	_Transform1 = Transform1;
	_Transform2 = Transform2;

	// Create physcontext
	_Context = CXR_PhysicsContext(Transform2);
	_Context.m_PhysGroupMaskThis = PhysicsPrim2.m_PhysModelMask;
	_Context.m_PhysGroupMaskCollider = PhysicsPrim1.m_PhysModelMask;
}

static CXR_Model *GetModel(const CWObject_CoreData *_pObject, CMapData *_pWorldData)
{
	const CWO_PhysicsPrim& Prim = _pObject->GetPhysState().m_Prim[0];
	return _pWorldData->GetResource_Model(Prim.m_iPhysModel);
}

M_FORCEINLINE static void TransposeOBB(CPhysOBB& _OBB)
{
/*	CMat4Dfp32 M;
	M.GetRow(0) = _OBB.m_A[0];
	M.GetRow(1) = _OBB.m_A[1];
	M.GetRow(2) = _OBB.m_A[2];

	M.Transpose3x3();
	_OBB.m_A[0] = M.GetRow(0);
	_OBB.m_A[1] = M.GetRow(1);
	_OBB.m_A[2] = M.GetRow(2);
*/
//	fp32 a00 = _OBB.m_A[0][0];
	fp32 a01 = _OBB.m_A[0][1];
	fp32 a02 = _OBB.m_A[0][2];
	fp32 a10 = _OBB.m_A[1][0];
//	fp32 a11 = _OBB.m_A[1][1];
	fp32 a12 = _OBB.m_A[1][2];
	fp32 a20 = _OBB.m_A[2][0];
	fp32 a21 = _OBB.m_A[2][1];
//	fp32 a22 = _OBB.m_A[2][2];

	_OBB.m_A[1][0] = a01;
	_OBB.m_A[2][0] = a02;
	_OBB.m_A[0][1] = a10;
	_OBB.m_A[0][2] = a20;
	_OBB.m_A[1][2] = a21;
	_OBB.m_A[2][1] = a12;
}

static int CopyCollisionInfo(int _nC, CCollisionInfo *_pCollisionInfo, CContactInfo *_pContactInfo, int _Max,
							 CRigidBody *_pRigidBody1, CRigidBody *_pRigidBody2,
							 const CWObject_CoreData *_pObject1, const CWObject_CoreData *_pObject2)
{
	int i;
	for (i = 0; i < _nC && i < _Max; i++)
	{
		_pContactInfo[i].m_pRigidBody1 = _pRigidBody1;
		_pContactInfo[i].m_pRigidBody2 = _pRigidBody2;
		_pContactInfo[i].m_Normal = _pCollisionInfo[i].m_Plane.n.Getfp64();
		_pContactInfo[i].m_Normal[3] = 0.0f;
		_pContactInfo[i].m_PointOfCollision = _pCollisionInfo[i].m_Pos.Getfp64();
		_pContactInfo[i].m_distance = M_Fabs(_pCollisionInfo[i].m_Distance);
		_pContactInfo[i].m_isColliding = _pCollisionInfo[i].m_bIsCollision;
		_pContactInfo[i].m_UserData1 = _pObject1->m_iObject;
		_pContactInfo[i].m_UserData2 = _pObject2->m_iObject;
	}
	return i;
}


static int CollideSolidSolid(const CWD_DynamicsWorld *_pWorld, 
							 const CWObject_CoreData *_pObject1,
							 const CWObject_CoreData *_pObject2,
							 CMapData *_pWorldData,
							 TAP_RCD<CWD_ContactInfo> _pContactInfo)
{
	MSCOPESHORT(CollideSolidSolid);

	CXR_PhysicsContext Context;

	int iRB1 = _pObject1->m_pRigidBody2->m_iRB;
	int iRB2 = _pObject2->m_pRigidBody2->m_iRB;

	CMat4Dfp32 Transform1, Transform2;
	SetupCollision(_pWorld, _pObject1, _pObject2, _pWorldData, Transform1, Transform2, Context);

//	Transform1.GetRow(3) *= 32.0f;
//	Transform2.GetRow(3) *= 32.0f;

	CXR_Model *pModel1 = GetModel(_pObject1, _pWorldData);
	CXR_Model *pModel2 = GetModel(_pObject2, _pWorldData);

	const CWO_PhysicsPrim& PhysicsPrim1 = _pObject1->GetPhysState().m_Prim[0];
	const CWO_PhysicsPrim& PhysicsPrim2 = _pObject2->GetPhysState().m_Prim[0];

	if (!(pModel1->GetModelClass() == CXR_MODEL_CLASS_BSP2 && pModel2->GetModelClass() == CXR_MODEL_CLASS_BSP2))
		return 0;

	CXR_Model_BSP2 *pBSP1 = (CXR_Model_BSP2 *) pModel1;
	CXR_Model_BSP2 *pBSP2 = (CXR_Model_BSP2 *) pModel2;

	CXR_IndexedSolidContainer32 *pIndexedSolids1 = pBSP1->m_spIndexedSolids;
	CXR_IndexedSolidContainer32 *pIndexedSolids2 = pBSP2->m_spIndexedSolids;

	if (!(pIndexedSolids1 != NULL && pIndexedSolids2 != NULL))
		return 0;

	TAP_RCD<CXR_IndexedSolid32> pSolids1 = pIndexedSolids1->m_lSolids;
	TAP_RCD<CXR_IndexedSolid32> pSolids2 = pIndexedSolids2->m_lSolids;

	TAP_RCD<CXR_MediumDesc> pMediumDesc1 = pIndexedSolids1->m_lMediums;
	TAP_RCD<CXR_MediumDesc> pMediumDesc2 = pIndexedSolids2->m_lMediums;

	TAP_RCD<CPhysOBB> pOBB1 = pIndexedSolids1->m_lObbs;
	TAP_RCD<CPhysOBB> pOBB2 = pIndexedSolids2->m_lObbs;

	TAP_RCD<bool> pIsOBB1 = pIndexedSolids1->m_lIsObbs;
	TAP_RCD<bool> pIsOBB2 = pIndexedSolids2->m_lIsObbs;

	const int MaxCollisions = 100;
	// TODO: static!!! temp hack due to constructor (not thread-safe!)
	uint8 CollisionInfoBuf[MaxCollisions * sizeof(CCollisionInfo)];
	CCollisionInfo *CollisionInfo = (CCollisionInfo *) CollisionInfoBuf;
	//CCollisionInfo CollisionInfo[MaxCollisions];
	int iCollision = 0;

	CVec3Dfp32 PointOfCollisions[MaxCollisions];
	CVec3Dfp32 Normals[MaxCollisions];
	fp32 Depths[MaxCollisions];

	for (int is1 = 0; is1 < pSolids1.Len(); is1++)
	{
		for (int is2 = 0; is2 < pSolids2.Len(); is2++)
		{
			const CXR_MediumDesc& MediumDesc1 = pMediumDesc1[pSolids1[is1].m_iMedium];
			const CXR_MediumDesc& MediumDesc2 = pMediumDesc2[pSolids2[is2].m_iMedium];

			fp32 StaticFriction = Min(pSolids1[is1].m_Friction, pSolids2[is2].m_Friction);
			fp32 DynamicFriction = StaticFriction * 0.75f;

			if (!(MediumDesc1.m_MediumFlags & XW_MEDIUM_SOLID && MediumDesc2.m_MediumFlags && XW_MEDIUM_SOLID)) continue;
			if (!(PhysicsPrim1.m_PhysModelMask & M_BitD(MediumDesc1.m_iPhysGroup))) continue;
			if (!(PhysicsPrim2.m_PhysModelMask & M_BitD(MediumDesc2.m_iPhysGroup))) continue;

			int nC = 0;
			if ((pIsOBB1[is1] && pIsOBB2[is2]))
			{
				CPhysOBB OBB1 = pOBB1[is1];
				CPhysOBB OBB2 = pOBB2[is2];

				OBB1.m_E *= 2.0f;
				OBB2.m_E *= 2.0f;

				OBB1 *= Transform1;
				OBB2 *= Transform2;

				TransposeOBB(OBB1);
				TransposeOBB(OBB2);

				nC = Phys_Collide_OBB(OBB1, OBB2, CollisionInfo, MaxCollisions);
				M_ASSERT(nC <= MaxCollisions, "!");

				/*
				iCollision += CopyCollisionInfo(nC, CollisionInfo, &_pContactInfo[iCollision], _MaxCollisions,
												(CRigidBody *) _pObject1->m_pRigidBody,
												(CRigidBody *) _pObject2->m_pRigidBody,
												_pObject1, _pObject2);

												*/
				for (int i = 0; i < nC && iCollision < _pContactInfo.Len(); i++)
				{
					CVec3Dfp32 DeleteMe = CollisionInfo[i].m_Plane.n;

					_pContactInfo[iCollision].m_iRB1= iRB1;
					_pContactInfo[iCollision].m_iRB2 = iRB2;
					_pContactInfo[iCollision].m_Normal = CollisionInfo[i].m_Plane.n;
					_pContactInfo[iCollision].m_Normal[3] = 0.0f;
					_pContactInfo[iCollision].m_PointOfCollision = CollisionInfo[i].m_Pos * (1.0f / 32.0f);
					_pContactInfo[iCollision].m_Distance = M_Fabs(CollisionInfo[i].m_Distance) * (1.0f / 32.0f);
					_pContactInfo[iCollision].m_bIsColliding = CollisionInfo[i].m_bIsCollision;
					_pContactInfo[iCollision].m_BiasVelocity = M_VZero();
					_pContactInfo[iCollision].m_UserData1 = _pObject1->m_iObject;
					_pContactInfo[iCollision].m_UserData2 = _pObject2->m_iObject;
					_pContactInfo[iCollision].m_Friction_Elasticity = CVec4Dfp32(DynamicFriction, StaticFriction, 0.4f, 0.0f);

					iCollision++;
				}

				/*
				for (int i = 0; i < nC && iCollision < _MaxCollisions; i++, iCollision++)
				{
				_pContactInfo[iCollision].m_pRigidBody1 = (CRigidBody *) _pObject1->m_pRigidBody;
				_pContactInfo[iCollision].m_pRigidBody2 = (CRigidBody *) _pObject2->m_pRigidBody;
				_pContactInfo[iCollision].m_Normal = CollisionInfo[i].m_Plane.n.Getfp64();
				_pContactInfo[iCollision].m_PointOfCollision = CollisionInfo[i].m_Pos.Getfp64();
				_pContactInfo[iCollision].m_distance = CollisionInfo[i].m_Distance;
				_pContactInfo[iCollision].m_isColliding = CollisionInfo[i].m_bIsCollision;
				}*/

			}
			else
			{
				CXR_IndexedSolidDesc32 SolidDescr1, SolidDescr2;
				pIndexedSolids1->GetSolid(is1, SolidDescr1);
				pIndexedSolids2->GetSolid(is2, SolidDescr2);


				CVec3Dfp32 Center1 = (SolidDescr1.m_BoundBox.m_Max + SolidDescr1.m_BoundBox.m_Min) * 0.5f;
				CVec3Dfp32 Center2 = (SolidDescr2.m_BoundBox.m_Max + SolidDescr2.m_BoundBox.m_Min) * 0.5f;
				Center1 *= Transform1;
				Center2 *= Transform2;
				CVec3Dfp32 CenterAxis = Center2 - Center1;
				CenterAxis.Normalize();

				if( pIsOBB1[is1] )
				{
					CPhysOBB OBB = pOBB1[is1];
					OBB *= Transform1;
					CMat4Dfp32 Trans;
					for(int i = 0;i < 3;i++)
						Trans.GetRow(i) = OBB.m_A[i];
					Trans.GetRow(3) = OBB.m_C;

					CSolidBoxSeparating TempSolid1(OBB.m_E);
					CSolidPolyhedronSeparating TempSolid2(&SolidDescr2, pIndexedSolids2->m_lSolids[is2].m_nVertices);
					nC = TPolyhedraPolyhedraCollider<CSolidBoxSeparating, CSolidPolyhedronSeparating, fp32>
						::Collide(&TempSolid1, Trans,
						&TempSolid2, Transform2, CenterAxis, PointOfCollisions, Normals, Depths,MaxCollisions);
					M_ASSERT(nC <= MaxCollisions, "!");
				}
				else if( pIsOBB2[is2] )
				{
					CPhysOBB OBB = pOBB2[is2];
					OBB *= Transform2;
					CMat4Dfp32 Trans;
					for(int i = 0;i < 3;i++)
						Trans.GetRow(i) = OBB.m_A[i];
					Trans.GetRow(3) = OBB.m_C;

					CSolidPolyhedronSeparating TempSolid1(&SolidDescr1, pIndexedSolids1->m_lSolids[is1].m_nVertices);
					CSolidBoxSeparating TempSolid2(OBB.m_E);
					nC = TPolyhedraPolyhedraCollider<CSolidPolyhedronSeparating, CSolidBoxSeparating, fp32>
						::Collide(&TempSolid1, Transform1,
						&TempSolid2, Trans, CenterAxis, PointOfCollisions, Normals, Depths,MaxCollisions);
					M_ASSERT(nC <= MaxCollisions, "!");
				}
				else
				{
					CSolidPolyhedronSeparating TempSolid1(&SolidDescr1, pIndexedSolids1->m_lSolids[is1].m_nVertices);
					CSolidPolyhedronSeparating TempSolid2(&SolidDescr2, pIndexedSolids2->m_lSolids[is2].m_nVertices);
					nC = TPolyhedraPolyhedraCollider<CSolidPolyhedronSeparating, CSolidPolyhedronSeparating, fp32>
						::Collide(&TempSolid1, Transform1,
						&TempSolid2, Transform2, CenterAxis, PointOfCollisions, Normals, Depths, MaxCollisions);

					M_ASSERT(nC <= MaxCollisions, "!");
				}

				for (int i = 0; i < nC && iCollision < _pContactInfo.Len(); i++)
				{
					_pContactInfo[iCollision].m_iRB1= iRB1;
					_pContactInfo[iCollision].m_iRB2 = iRB2;
					_pContactInfo[iCollision].m_Normal = Normals[i];
					_pContactInfo[iCollision].m_PointOfCollision = PointOfCollisions[i] * (1.0f / 32.0f);
					_pContactInfo[iCollision].m_Distance = Depths[i] * (1.0f / 32.0f);
					_pContactInfo[iCollision].m_bIsColliding = CollisionInfo[i].m_bIsCollision;
					_pContactInfo[iCollision].m_BiasVelocity = M_VZero();
					_pContactInfo[iCollision].m_UserData1 = _pObject1->m_iObject;
					_pContactInfo[iCollision].m_UserData2 = _pObject2->m_iObject;
					_pContactInfo[iCollision].m_Friction_Elasticity = CVec4Dfp32(DynamicFriction, StaticFriction, 0.4f, 0.0f);

					iCollision++;
				}
			}
		}
	}

	return iCollision;
}


int CollideObjectOBB(const CWD_DynamicsWorld* _pWorld, 
					 const CWD_RigidBody2* _pBody1, 
					 CPhysOBB& _OBB,
					 CMapData *_pWorldData,
					 TAP_RCD<CWD_ContactInfo> _pContactInfo)
{
	const CWObject_CoreData *pObj1 = (CWObject_CoreData *) _pBody1->m_pUserData;

	CBox3Dfp32 Box1;
	GetBoundBox(_pWorldData, pObj1, Box1);
	CPhysOBB Obb1;

	const CMat4Dfp32& P1 = _pWorld->GetTransform(_pBody1->m_iRB);

	Obb1.SetPosition(P1);
	Obb1.m_C *= 32.0f;
	CVec3Dfp32 Extent1 = Box1.m_Max - Box1.m_Min;
	Obb1.SetDimensions(Extent1);

	CCollisionInfo CollisionInfo[10];
	int nCollisions = Phys_Collide_OBB(Obb1, _OBB, CollisionInfo, 10);

	int nTotCol = 0;
	// OBS i < 1 här!!!
	for (int i = 0; i < nCollisions && i < _pContactInfo.Len() && i < 1; i++)
	{
		_pContactInfo[nTotCol].m_iRB1= _pBody1->m_iRB;
		_pContactInfo[nTotCol].m_iRB2 = 0;
		_pContactInfo[nTotCol].m_Normal = CollisionInfo[i].m_Plane.n;
		_pContactInfo[nTotCol].m_Normal[3] = 0.0f;
		_pContactInfo[nTotCol].m_Normal[2] = 0.0f;
		_pContactInfo[nTotCol].m_Normal.Normalize();
		_pContactInfo[nTotCol].m_PointOfCollision = P1.GetRow(3);
//		_pContactInfo[nTotCol].m_PointOfCollision = CollisionInfo[i].m_Pos * 1.0f / 32.0f;
		_pContactInfo[nTotCol].m_Distance = M_Fabs(CollisionInfo[i].m_Distance) * (1.0f / 32.0f);
		_pContactInfo[nTotCol].m_bIsColliding = CollisionInfo[i].m_bIsCollision;
		_pContactInfo[nTotCol].m_BiasVelocity = M_VZero();
		_pContactInfo[nTotCol].m_UserData1 = pObj1->m_iObject;
		_pContactInfo[nTotCol].m_UserData2 = 0;
		nTotCol++;

	}
	return nTotCol;
}


static int CollideSolidOBB(const CWD_DynamicsWorld* _pWorld, 
                           const CWObject_CoreData* _pObject,
                           CPhysOBB& _OBB, CPhysOBB& _OBBDest, 
						   CMapData* _pWorldData,
						   CCollisionInfo* _pDest, uint _nMaxDest)
{
	/*
	const CWD_RigidBody2* pRB = _pObject->m_pRigidBody2;
	if (!pRB)
		return 0;

	const CWO_PhysicsPrim& PhysPrim = _pObject->GetPhysState().m_Prim[0];
	CVec3Dfp32 CenterOfMass(0.0f);

	// Get transform
	CMat4Dfp32 Transform = _pWorld->GetTransform(pRB->m_iRB);
	Transform.GetRow(3) *= 32.0f;
	CenterOfMass = pRB->m_CenterOfMass;

	// Correct transform for center of mass and offset
	CVec3Dfp32 TotalOffset = PhysPrim.GetOffset() - CenterOfMass;
	Transform.GetRow(3) = TotalOffset * Transform;
	*/
	CMat4Dfp32 Transform;
	SetupCollisionSingle(_pWorld,_pObject,_pWorldData,Transform);
	const CWO_PhysicsPrim& PhysPrim = _pObject->GetPhysState().m_Prim[0];

	// Get physmodel
	CXR_Model* pModel = _pWorldData->GetResource_Model(PhysPrim.m_iPhysModel);
	if (!pModel || pModel->GetModelClass() != CXR_MODEL_CLASS_BSP2)
	{
		if(PhysPrim.m_PrimType != OBJECT_PRIMTYPE_PHYSMODEL)
			M_TRACEALWAYS("CollideSolidOBB trying to collide with object that does not have a physmodel primtype (%d)!!!\r\n", PhysPrim.m_PrimType);
		return 0;
	}

	// Create physcontext
	CXR_PhysicsContext PhysContext(Transform);
	PhysContext.m_PhysGroupMaskThis = PhysPrim.m_PhysModelMask;
	PhysContext.m_PhysGroupMaskCollider = PhysPrim.m_PhysModelMask;

	CXR_Model_BSP2* pBSP = (CXR_Model_BSP2*)pModel;
//	return pBSP->Phys_CollideBox(&PhysContext, _OBB, XW_MEDIUM_SOLID, _pDest, _nMaxDest);*/

	if (pBSP->Phys_IntersectBox(&PhysContext, _OBB, _OBBDest, XW_MEDIUM_SOLID | XW_MEDIUM_PLAYERSOLID | XW_MEDIUM_PHYSSOLID, _pDest))
		return 1;
	return 0;
}


//
// This is used to create an OBB with in front of characters to make the characters
// push objects ahead of them. The correct rigid body is used, so mass calculations 
// are valid.
//
int CreatePushCollision(const CWD_DynamicsWorld* _pWorld, 
						CMapData* _pMapData,
						const CWObject_CoreData* _pObj,
						const CWObject_CoreData* _pChar, 
						TAP_RCD<CWD_ContactInfo> _pContactInfo)
{
	MSCOPESHORT(CreatePushCollision);

	CBox3Dfp32 Box2;
	CPhysOBB Obb2;

	// Object
	int iRB1 = _pObj->m_pRigidBody2->m_iRB;

	// Character
	CVec3Dfp32 CharMoveVelocity = _pChar->GetMoveVelocity();
	if (CharMoveVelocity.LengthSqr() < 0.1f)
		return 0;

	int iRB2 = _pChar->m_pRigidBody2 ? _pChar->m_pRigidBody2->m_iRB : 0;
	Box2 = *_pChar->GetAbsBoundBox();
	CMat4Dfp32 P2;
	P2.Unit();
	Box2.GetCenter(P2.GetRow(3));
	P2.GetRow(3) += CharMoveVelocity * 16.0f;	// place the box in front of character
	Obb2.SetPosition(P2);
	CVec3Dfp32 Extent2 = Box2.m_Max - Box2.m_Min;
	Obb2.SetDimensions(Extent2);
	Obb2.m_E *= 0.5f;

	// NOTE: I gave up trying to use 'Phys_CollideBox', so I use 'Phys_IntersectBox' which needs a start & dest OBB   /anton
	CPhysOBB ObbFakeDest = Obb2;
	CVec3Dfp32 FixedNormal;
	{
		FixedNormal = _pObj->GetPosition() - _pChar->GetPosition();
		FixedNormal.k[2] = 0.0f;
		FixedNormal.Normalize();
		ObbFakeDest.m_C += FixedNormal;
		ObbFakeDest.m_C += _pChar->GetMoveVelocity();
		ObbFakeDest.m_C -= _pObj->GetMoveVelocity();
	}

	CCollisionInfo CollisionInfo[10];
	uint nColl = CollideSolidOBB(_pWorld, _pObj, Obb2, ObbFakeDest, _pMapData, CollisionInfo, 10);
	int nTotCol = MinMT(nColl, _pContactInfo.Len());

	bool bIsDoor = _pObj->IsClass(class_SwingDoor);

	for (int i = 0; i < nTotCol; i++)
	{
		CWD_ContactInfo& Dest = _pContactInfo[i];
		Dest.m_iRB1 = iRB1;
		Dest.m_iRB2 = iRB2;	
		Dest.m_BiasVelocity = M_VZero();
		
		Dest.m_Friction_Elasticity = CVec4Dfp32(0.4f, 0.3f, 0.0f, 0.0f);
		Dest.m_bIsColliding = 0;//CollisionInfo[i].m_bIsCollision;
		Dest.m_Normal = FixedNormal;
		
		if(!bIsDoor)
		{
			Dest.m_UserData1 = 0; // _pObj->m_iObject; -- don't want this to start playing impact sounds
			Dest.m_UserData2 = 0; // _pChar->m_iObject; -- don't want this to create phys damage
			Dest.m_Distance = 0.0f; //M_Fabs(CollisionInfo[i].m_Distance) * (1.0f / 32.0f);
		}
		else
		{	//Hack for swingdoors.
			Dest.m_UserData1 = _pObj->m_iObject;
			Dest.m_UserData2 = _pChar->m_iObject;
			Dest.m_Distance = 1.2f;
		}

		if (CollisionInfo[i].m_bIsValid)
			Dest.m_PointOfCollision = CollisionInfo[i].m_Pos * (1.0f / 32.0f);
		else
			Dest.m_PointOfCollision = Obb2.m_C * (1.0f / 32.0f);  // whatever...
	}
	return nTotCol;
}

//
// This function collides a physics object with a character object.
// The character is treated as 'world' (i.e. non-movable), so it's not suitable for 
// making the character push the object, but is pretty safe against getting stuck situations.
//
int CollideObjectCharacter(const CWD_DynamicsWorld* _pWorld, 
						   CMapData* _pMapData,
						   const CWObject_CoreData* _pObj, 
						   const CWObject_CoreData* _pChar,
						   TAP_RCD<CWD_ContactInfo> _pContactInfo)
{
	MSCOPESHORT(CollideObjectCharacter);

	CBox3Dfp32 Box1, Box2;
	CPhysOBB Obb1,Obb2;

	// Object
	int iRB1 = _pObj->m_pRigidBody2->m_iRB;
/*	GetBoundBox(_pWorldData, pObj1, Box1);
	CVec3Dfp32 Center;
	Box1.GetCenter(Center);
	CMat4Dfp32 P1 = pObj1->GetPositionMatrix();
	P1.GetRow(3) = Center * P1;
	Obb1.SetPosition(P1);								
	CVec3Dfp32 Extent1 = Box1.m_Max - Box1.m_Min;
	Obb1.SetDimensions(Extent1);
*/

	// Character
	int iRB2 = _pChar->m_pRigidBody2 ? _pChar->m_pRigidBody2->m_iRB : 0;
	const CWO_PhysicsPrim& CharPhysPrim = _pChar->GetPhysState().m_Prim[0];
	CMat4Dfp32 P2;
	P2.Unit();
	P2.GetRow(3) = _pChar->GetPosition() + CharPhysPrim.GetOffset();
	Obb2.SetPosition(P2);
	Obb2.SetDimensions(CharPhysPrim.GetDim());

	// NOTE: I gave up trying to use 'Phys_CollideBox', so I use 'Phys_IntersectBox' which needs a start & dest OBB   /anton
	CPhysOBB ObbFakeStart = Obb2;
	CVec3Dfp32 Movement;
	{
		Movement = _pObj->GetPosition() - _pChar->GetPosition();
		Movement.SetLength(2.0f);
		Movement += _pChar->GetMoveVelocity();
		Movement -= _pObj->GetMoveVelocity();
		ObbFakeStart.m_C -= Movement;
	}

	bool bCharIsPlayer = (_pChar->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER) != 0;

	CCollisionInfo CollisionInfo[10];
	uint nColl = CollideSolidOBB(_pWorld, _pObj, ObbFakeStart, Obb2, _pMapData, CollisionInfo, 10);
	int nTotCol = MinMT(nColl, _pContactInfo.Len());

	for (int i = 0; i < nTotCol; i++)
	{
		CWD_ContactInfo& Dest = _pContactInfo[i];
		Dest.m_iRB1 = iRB1;
		Dest.m_iRB2 = iRB2;
		Dest.m_BiasVelocity = M_VZero();
		Dest.m_UserData1 = _pObj->m_iObject;
		Dest.m_UserData2 = _pChar->m_iObject;
		Dest.m_Friction_Elasticity = CVec4Dfp32(0.4f, 0.3f, 0.0f, 0.0f);
		Dest.m_bIsColliding = CollisionInfo[i].m_bIsCollision;

		if (bCharIsPlayer)
			Dest.m_iRB2 = 0; // NOTE: 0 means world -> i.e. infinite mass -> lower risk of getting stuck

		//M_TRACE("[CollideObjectCharacter] Distance: %.5f\n", CollisionInfo[i].m_Distance);

		if (CollisionInfo[i].m_bIsValid)
		{
			// Hack: make sure normal points away from character
			CVec3Dfp32 a = (_pObj->GetPosition() - Obb2.m_C).Normalize();
			CVec3Dfp32 n = -CollisionInfo[i].m_Plane.n;
			n *= Sign(a * n); 
			Dest.m_Normal = n;
			Dest.m_PointOfCollision = CollisionInfo[i].m_Pos * (1.0f / 32.0f);
			Dest.m_Distance = M_Fabs(CollisionInfo[i].m_Distance) * (1.0f / 32.0f)  * 1.0f;  // *10 -> make sure the fucker stays away from player!
		}
		else
		{ // wtf?  (better safe than sorry)
			Dest.m_Normal = Movement;
			Dest.m_Normal.Normalize();
			Dest.m_PointOfCollision = _pChar->GetPosition() * (1.0f / 32.0f);
			Dest.m_Distance = 8.0f / 32.0f;
		}
	}
	return nTotCol;
}


/************************************************************************************\
|¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯| 
	Primitive intersection
|____________________________________________________________________________________|
\************************************************************************************/


static int CollidePrimSolid(const CWD_DynamicsWorld *_pWorld,
							const CWObject_CoreData *_pObject1, const CWPhys_ClusterObject &_PO,const CVec3Dfp32 &_Dim,
							const CWObject_CoreData *_pObject2,
							CMapData *_pWorldData,
							TAP_RCD<CWD_ContactInfo> _pContactInfo)
{
	CMat4Dfp32 Transform;
	SetupCollisionSingle(_pWorld,_pObject2,_pWorldData,Transform);

	// Create physcontext
	CXR_PhysicsContext PhysContext(Transform);
	const CWO_PhysicsPrim& PhysicsPrim2 = _pObject2->GetPhysState().m_Prim[0];
	PhysContext.m_PhysGroupMaskThis = PhysicsPrim2.m_PhysModelMask;
	PhysContext.m_PhysGroupMaskCollider = _PO.m_PhysPrim.m_PhysModelMask;

	CXR_Model * pModel = GetModel(_pObject2,_pWorldData);

	//Class test is performed before we enter function
	CXR_Model_BSP2 * pBSP = (CXR_Model_BSP2*)pModel;

	CXR_IndexedSolidContainer32 *piSolids = pBSP->m_spIndexedSolids;
	if( !piSolids ) return 0;

	TAP_RCD<CXR_IndexedSolid32> pSolids = piSolids->m_lSolids;
	TAP_RCD<CXR_MediumDesc> pMediumDesc = piSolids->m_lMediums;
	TAP_RCD<CPhysOBB> pOBB = piSolids->m_lObbs;
	TAP_RCD<bool> pIsOBB1 = piSolids->m_lIsObbs;


	const int MaxCollisions = 100;
	uint8 CollisionInfoBuf[MaxCollisions * sizeof(CCollisionInfo)];
	CCollisionInfo *CollisionInfo = (CCollisionInfo *) CollisionInfoBuf;
	int iCollision = 0;

	CVec3Dfp32 PointOfCollisions[MaxCollisions];
	CVec3Dfp32 Normals[MaxCollisions];
	fp32 Depths[MaxCollisions];

	int i;
	uint32 iRB = _PO.m_pRB->m_iRB;
	switch( _PO.m_PhysPrim.m_PrimType )
	{

	case OBJECT_PRIMTYPE_BOX:
		{
			//Swap this travesty for something more efficient
			CPhysOBB OBB;
			{
				OBB.SetDimensions(_Dim);
				OBB.SetPosition(_PO.m_Transform);
			}

			for(i = 0;i < pSolids.Len();i++)
			{
				const CXR_MediumDesc& MediumDesc = pMediumDesc[pSolids[i].m_iMedium];

				fp32 StaticFriction = Min(pSolids[i].m_Friction, _PO.m_pRB->m_StaticFriction);
				fp32 DynamicFriction = StaticFriction * 0.75f;

				if (!(MediumDesc.m_MediumFlags & XW_MEDIUM_SOLID)) continue;
				if (!(PhysicsPrim2.m_PhysModelMask & M_BitD(MediumDesc.m_iPhysGroup))) continue;

				int nC = 0;

				if( pIsOBB1[i] )
				{
					CPhysOBB OBB2 = pOBB[i], OBBtest = OBB;

					OBBtest.m_E *= 2.0f;
					TransposeOBB(OBBtest);

					OBB2.m_E *= 2.0f;
					OBB2 *= Transform;
					TransposeOBB(OBB2);

					nC = Phys_Collide_OBB(OBBtest,OBB2,CollisionInfo,MaxCollisions);
					M_ASSERT(nC <= MaxCollisions, "!");

					for (int j = 0; j < nC && iCollision < _pContactInfo.Len(); j++)
					{
						_pContactInfo[iCollision].m_iRB1= iRB;
						_pContactInfo[iCollision].m_iRB2 = _pObject2->m_pRigidBody2->m_iRB;
						_pContactInfo[iCollision].m_Normal = CollisionInfo[j].m_Plane.n;
						_pContactInfo[iCollision].m_Normal[3] = 0.0f;
						_pContactInfo[iCollision].m_PointOfCollision = CollisionInfo[j].m_Pos * (1.0f / 32.0f);
						_pContactInfo[iCollision].m_Distance = M_Fabs(CollisionInfo[j].m_Distance) * (1.0f / 32.0f);
						_pContactInfo[iCollision].m_bIsColliding = CollisionInfo[j].m_bIsCollision;
						_pContactInfo[iCollision].m_BiasVelocity = M_VZero();
						_pContactInfo[iCollision].m_UserData1 = _pObject1->m_iObject;
						_pContactInfo[iCollision].m_UserData2 = _pObject2->m_iObject;
						_pContactInfo[iCollision].m_Friction_Elasticity = CVec4Dfp32(DynamicFriction, StaticFriction, 0.4f, 0.0f);

						iCollision++;
					}
				}
				else
				{
					CXR_IndexedSolidDesc32 SolidDescr;
					piSolids->GetSolid(i,SolidDescr);

					CVec3Dfp32 Center = (SolidDescr.m_BoundBox.m_Max + SolidDescr.m_BoundBox.m_Min) * 0.5f;
					Center *= Transform;
					CVec3Dfp32 CenterAxis = Center - OBB.m_C;
					CenterAxis.Normalize();
					
					CSolidBoxSeparating TempSolid1(OBB.m_E);
					CSolidPolyhedronSeparating TempSolid2(&SolidDescr,piSolids->m_lSolids[i].m_nVertices);
					nC = TPolyhedraPolyhedraCollider<CSolidBoxSeparating, CSolidPolyhedronSeparating, fp32>
						::Collide(&TempSolid1,_PO.m_Transform,
						&TempSolid2,Transform,CenterAxis,PointOfCollisions,Normals,Depths,MaxCollisions);
					M_ASSERT(nC <= MaxCollisions, "!");

					for (int j = 0; j < nC && iCollision < _pContactInfo.Len(); j++)
					{
						_pContactInfo[iCollision].m_iRB1= iRB;
						_pContactInfo[iCollision].m_iRB2 = _pObject2->m_pRigidBody2->m_iRB;
						_pContactInfo[iCollision].m_Normal = Normals[j];
						_pContactInfo[iCollision].m_PointOfCollision = PointOfCollisions[j] * (1.0f / 32.0f);
						_pContactInfo[iCollision].m_Distance = Depths[j] * (1.0f / 32.0f);
						_pContactInfo[iCollision].m_bIsColliding = true;//CollisionInfo[j].m_bIsCollision;
						_pContactInfo[iCollision].m_BiasVelocity = M_VZero();
						_pContactInfo[iCollision].m_UserData1 = _pObject1->m_iObject;
						_pContactInfo[iCollision].m_UserData2 = _pObject2->m_iObject;
						_pContactInfo[iCollision].m_Friction_Elasticity = CVec4Dfp32(DynamicFriction, StaticFriction, 0.4f, 0.0f);

						iCollision++;
					}
				}
			}
		}
		break;
	
	case OBJECT_PRIMTYPE_CAPSULE:
		{
			for(i = 0;i < pSolids.Len();i++)
			{
				const CXR_MediumDesc& MediumDesc = pMediumDesc[pSolids[i].m_iMedium];

				fp32 StaticFriction = Min(pSolids[i].m_Friction, _PO.m_pRB->m_StaticFriction);
				fp32 DynamicFriction = StaticFriction * 0.75f;

				if (!(MediumDesc.m_MediumFlags & XW_MEDIUM_SOLID)) continue;
				if (!(PhysicsPrim2.m_PhysModelMask & M_BitD(MediumDesc.m_iPhysGroup))) continue;

				int nC = 0;

				CXR_IndexedSolidDesc32 SolidDescr;
				piSolids->GetSolid(i,SolidDescr);

				CVec3Dfp32 Center = (SolidDescr.m_BoundBox.m_Max + SolidDescr.m_BoundBox.m_Min) * 0.5f;
				Center *= Transform;
				CVec3Dfp32 CenterAxis = Center - _PO.m_Transform.GetRow(3);
				CenterAxis.Normalize();

				CXR_IndexedSolid32 &Sld = piSolids->m_lSolids[i];
				CSolidCapsuleSeparating TempSolid1(_Dim,SolidDescr.m_pV.m_pArray,Sld.m_nVertices,&Transform);
				CSolidPolyhedronSeparating TempSolid2(&SolidDescr,Sld.m_nVertices);
				nC = TPolyhedraPolyhedraCollider<CSolidCapsuleSeparating, CSolidPolyhedronSeparating, fp32>
					::Collide(&TempSolid1,_PO.m_Transform,
					&TempSolid2,Transform,CenterAxis,PointOfCollisions,Normals,Depths,MaxCollisions);
				M_ASSERT(nC <= MaxCollisions, "!");

				for (int j = 0; j < nC && iCollision < _pContactInfo.Len(); j++)
				{
					_pContactInfo[iCollision].m_iRB1= iRB;
					_pContactInfo[iCollision].m_iRB2 = _pObject2->m_pRigidBody2->m_iRB;
					_pContactInfo[iCollision].m_Normal = Normals[j];
					_pContactInfo[iCollision].m_PointOfCollision = PointOfCollisions[j] * (1.0f / 32.0f);
					_pContactInfo[iCollision].m_Distance = Depths[j] * (1.0f / 32.0f);
					_pContactInfo[iCollision].m_bIsColliding = CollisionInfo[j].m_bIsCollision;
					_pContactInfo[iCollision].m_BiasVelocity = M_VZero();
					_pContactInfo[iCollision].m_UserData1 = _pObject1->m_iObject;
					_pContactInfo[iCollision].m_UserData2 = _pObject2->m_iObject;
					_pContactInfo[iCollision].m_Friction_Elasticity = CVec4Dfp32(DynamicFriction, StaticFriction, 0.4f, 0.0f);

					iCollision++;
				}
			}
		}
		break;

	}

	return iCollision;
}


static int CollidePrimOBB(const CWObject_CoreData* _pObj, const CWPhys_ClusterObject &_PO,const CVec3Dfp32 &_Dim,
						  const CPhysOBB &_OBB, CCollisionInfo * _pClsInfo, int _nMaxCls)
{
	switch( _PO.m_PhysPrim.m_PrimType )
	{

	case OBJECT_PRIMTYPE_BOX:
		{
			CPhysOBB OBB;
			OBB.SetDimensions(_Dim);
			OBB.SetPosition(_PO.m_Transform);

			uint nC = Phys_Collide_OBB(OBB,_OBB,_pClsInfo,_nMaxCls);
			M_ASSERT(nC <= _nMaxCls, "!");
			return nC;
		}

	case OBJECT_PRIMTYPE_CAPSULE:
		{
			CVec3Dfp32 CenterAxis = _PO.m_Transform.GetRow(3) - _OBB.m_C;
			CenterAxis.Normalize();

			CVec3Dfp32 Pts[8];
			Pts[0] = CVec3Dfp32(_OBB.m_E.k[0],_OBB.m_E.k[1],_OBB.m_E.k[2]);
			Pts[1] = CVec3Dfp32(_OBB.m_E.k[0],_OBB.m_E.k[1],-_OBB.m_E.k[2]);
			Pts[2] = CVec3Dfp32(_OBB.m_E.k[0],-_OBB.m_E.k[1],_OBB.m_E.k[2]);
			Pts[3] = CVec3Dfp32(_OBB.m_E.k[0],-_OBB.m_E.k[1],-_OBB.m_E.k[2]);
			Pts[4] = CVec3Dfp32(-_OBB.m_E.k[0],_OBB.m_E.k[1],_OBB.m_E.k[2]);
			Pts[5] = CVec3Dfp32(-_OBB.m_E.k[0],_OBB.m_E.k[1],-_OBB.m_E.k[2]);
			Pts[6] = CVec3Dfp32(-_OBB.m_E.k[0],-_OBB.m_E.k[1],_OBB.m_E.k[2]);
			Pts[7] = CVec3Dfp32(-_OBB.m_E.k[0],-_OBB.m_E.k[1],-_OBB.m_E.k[2]);

			const int MaxCollisions = 4;
			CVec3Dfp32 PointOfCollisions[MaxCollisions];
			CVec3Dfp32 Normals[MaxCollisions];
			fp32 Depths[MaxCollisions];

			CMat4Dfp32 Mt;
			_OBB.GetPosition(Mt);
			CSolidCapsuleSeparating TempSolid1(_Dim,Pts,8,&Mt);
			CSolidBoxSeparating TempSolid2(_OBB.m_E);
			int nC = TPolyhedraPolyhedraCollider<CSolidCapsuleSeparating,CSolidBoxSeparating,fp32>
				::Collide(&TempSolid1,_PO.m_Transform,&TempSolid2,Mt,CenterAxis,
				PointOfCollisions,Normals,Depths,MaxCollisions);

			M_ASSERT(nC <= MaxCollisions, "!");

			for(int i = 0;i < nC;i++)
			{
				_pClsInfo[i].m_Pos = PointOfCollisions[i];
				_pClsInfo[i].m_Distance = Depths[i];
				_pClsInfo[i].m_Plane.n = Normals[i];
				_pClsInfo[i].m_bIsCollision = true;
			}

			return nC;
		}
	}

	return 0;
}


static int CollidePrimPrim(const CWObject_CoreData *_pObject1, const CWPhys_ClusterObject &_PO1,const CVec3Dfp32 &_Dim1,
						   const CWObject_CoreData *_pObject2, const CWPhys_ClusterObject &_PO2,const CVec3Dfp32 &_Dim2,
						   TAP_RCD<CWD_ContactInfo> _pContactInfo)
{
	const int MaxCollisions = 32;
	uint8 CollisionInfoBuf[MaxCollisions * sizeof(CCollisionInfo)];
	CCollisionInfo *CollisionInfo = (CCollisionInfo *) CollisionInfoBuf;
	int iCollision = 0;

	//This code just so we won't have to write all code twice
	const CWPhys_ClusterObject *pPO1 = &_PO1;
	const CWPhys_ClusterObject *pPO2 = &_PO2;
	if( _PO2.m_PhysPrim.m_PrimType < _PO1.m_PhysPrim.m_PrimType )
	{
		pPO1 = &_PO2;
		pPO2 = &_PO1;
		Swap(_pObject1,_pObject2);
	}

	CVec3Dfp32 PointOfCollisions[MaxCollisions];
	CVec3Dfp32 Normals[MaxCollisions];
	fp32 Depths[MaxCollisions];

	const fp32 StaticFriction = Min(_PO1.m_pRB->m_StaticFriction,pPO2->m_pRB->m_StaticFriction);
	const fp32 DynamicFriction = StaticFriction * 0.75f;

	switch( pPO1->m_PhysPrim.m_PrimType )
	{

	case OBJECT_PRIMTYPE_BOX:
		{
			CPhysOBB OBB;
			OBB.SetDimensions(_Dim1);
			OBB.SetPosition(pPO1->m_Transform);

			switch( pPO2->m_PhysPrim.m_PrimType )
			{

			case OBJECT_PRIMTYPE_BOX:
				{
					CPhysOBB OBB2;
					OBB2.SetDimensions( _Dim2 * 2.0f);
					OBB2.SetPosition(pPO2->m_Transform );
					OBB.m_E *= 2.0f;
					
					TransposeOBB(OBB);
					TransposeOBB(OBB2);

					uint nC = Phys_Collide_OBB(OBB,OBB2,CollisionInfo,MaxCollisions);
					M_ASSERT(nC <= MaxCollisions, "!");

					for (int i = 0; i < nC && iCollision < _pContactInfo.Len(); i++)
					{
						_pContactInfo[iCollision].m_iRB1 = pPO1->m_pRB->m_iRB;
						_pContactInfo[iCollision].m_iRB2 = pPO2->m_pRB->m_iRB;
						_pContactInfo[iCollision].m_Normal = CollisionInfo[i].m_Plane.n;
						_pContactInfo[iCollision].m_Normal[3] = 0.0f;
						_pContactInfo[iCollision].m_PointOfCollision = CollisionInfo[i].m_Pos * (1.0f / 32.0f);
						_pContactInfo[iCollision].m_Distance = M_Fabs(CollisionInfo[i].m_Distance) * (1.0f / 32.0f);
						_pContactInfo[iCollision].m_bIsColliding = CollisionInfo[i].m_bIsCollision;
						_pContactInfo[iCollision].m_BiasVelocity = M_VZero();
						_pContactInfo[iCollision].m_UserData1 = _pObject1->m_iObject;
						_pContactInfo[iCollision].m_UserData2 = _pObject2->m_iObject;
						_pContactInfo[iCollision].m_Friction_Elasticity = CVec4Dfp32(DynamicFriction, StaticFriction, 0.4f, 0.0f);

						iCollision++;
					}
					return iCollision;
				}

			case OBJECT_PRIMTYPE_CAPSULE:
				{
					CVec3Dfp32 CenterAxis = pPO2->m_Transform.GetRow(3) - OBB.m_C;
					CenterAxis.Normalize();

					CVec3Dfp32 Pts[8];
					Pts[0] = CVec3Dfp32(OBB.m_E.k[0],OBB.m_E.k[1],OBB.m_E.k[2]);
					Pts[1] = CVec3Dfp32(OBB.m_E.k[0],OBB.m_E.k[1],-OBB.m_E.k[2]);
					Pts[2] = CVec3Dfp32(OBB.m_E.k[0],-OBB.m_E.k[1],OBB.m_E.k[2]);
					Pts[3] = CVec3Dfp32(OBB.m_E.k[0],-OBB.m_E.k[1],-OBB.m_E.k[2]);
					Pts[4] = CVec3Dfp32(-OBB.m_E.k[0],OBB.m_E.k[1],OBB.m_E.k[2]);
					Pts[5] = CVec3Dfp32(-OBB.m_E.k[0],OBB.m_E.k[1],-OBB.m_E.k[2]);
					Pts[6] = CVec3Dfp32(-OBB.m_E.k[0],-OBB.m_E.k[1],OBB.m_E.k[2]);
					Pts[7] = CVec3Dfp32(-OBB.m_E.k[0],-OBB.m_E.k[1],-OBB.m_E.k[2]);

					CSolidBoxSeparating TempSolid1(OBB.m_E);
					CSolidCapsuleSeparating TempSolid2(_Dim2,Pts,8,&pPO1->m_Transform);
					int nC = TPolyhedraPolyhedraCollider<CSolidBoxSeparating,CSolidCapsuleSeparating,fp32>
						::Collide(&TempSolid1,pPO1->m_Transform,&TempSolid2,pPO2->m_Transform,CenterAxis,
						PointOfCollisions,Normals,Depths,MaxCollisions);

					M_ASSERT(nC <= MaxCollisions, "!");

					for (int i = 0; i < nC && iCollision < _pContactInfo.Len(); i++)
					{
						_pContactInfo[iCollision].m_iRB1 = pPO1->m_pRB->m_iRB;
						_pContactInfo[iCollision].m_iRB2 = pPO2->m_pRB->m_iRB;
						_pContactInfo[iCollision].m_Normal = Normals[i];
						_pContactInfo[iCollision].m_Normal[3] = 0.0f;
						_pContactInfo[iCollision].m_PointOfCollision = PointOfCollisions[i] * (1.0f / 32.0f);
						_pContactInfo[iCollision].m_Distance = Depths[i] * (1.0f / 32.0f);
						_pContactInfo[iCollision].m_bIsColliding = true;
						_pContactInfo[iCollision].m_BiasVelocity = M_VZero();
						_pContactInfo[iCollision].m_UserData1 = _pObject1->m_iObject;
						_pContactInfo[iCollision].m_UserData2 = _pObject2->m_iObject;
						_pContactInfo[iCollision].m_Friction_Elasticity = CVec4Dfp32(DynamicFriction, StaticFriction, 0.4f, 0.0f);

						iCollision++;
					}
					return iCollision;
				}
			}
		}
		break;

	case OBJECT_PRIMTYPE_CAPSULE:
		{
			switch( pPO2->m_PhysPrim.m_PrimType )
			{

			case OBJECT_PRIMTYPE_CAPSULE:
				{
					const CMat4Dfp32 &M1 = pPO1->m_Transform;
					const CMat4Dfp32 &M2 = pPO2->m_Transform;

					uint nC = Phys_Collide_Capsule(M1.GetRow(3),M1.GetRow(0),_Dim1.k[0],_Dim1.k[1],
						M2.GetRow(3),M2.GetRow(0),_Dim2.k[0],_Dim2.k[1],CollisionInfo);
					M_ASSERT(nC <= MaxCollisions, "!");

					for (int i = 0; i < nC && iCollision < _pContactInfo.Len(); i++)
					{
						_pContactInfo[iCollision].m_iRB1 = pPO1->m_pRB->m_iRB;
						_pContactInfo[iCollision].m_iRB2 = pPO2->m_pRB->m_iRB;
						_pContactInfo[iCollision].m_Normal = CollisionInfo[i].m_Plane.n;
						_pContactInfo[iCollision].m_Normal[3] = 0.0f;
						_pContactInfo[iCollision].m_PointOfCollision = CollisionInfo[i].m_Pos * (1.0f / 32.0f);
						_pContactInfo[iCollision].m_Distance = M_Fabs(CollisionInfo[i].m_Distance) * (1.0f / 32.0f);
						_pContactInfo[iCollision].m_bIsColliding = CollisionInfo[i].m_bIsCollision;
						_pContactInfo[iCollision].m_BiasVelocity = M_VZero();
						_pContactInfo[iCollision].m_UserData1 = _pObject1->m_iObject;
						_pContactInfo[iCollision].m_UserData2 = _pObject2->m_iObject;
						_pContactInfo[iCollision].m_Friction_Elasticity = CVec4Dfp32(DynamicFriction, StaticFriction, 0.4f, 0.0f);

						iCollision++;
					}
					return iCollision;
				}
			}
		}
		break;

	}
	return 0;
}


static int CollidePrimBSP(const CWObject_CoreData *_pObj1, const CWPhys_ClusterObject &_PO,const CVec3Dfp32 &_Dim,
						  const CBox3Dfp32 &_Bounds,
						  const CWObject_CoreData *_pObj2, 
						  CMapData *_pWorldData,
						  TAP_RCD<CWD_ContactInfo> _pContactInfo,
						  CWorld_PhysState *_pPhysState)
{
	const CWO_PhysicsPrim& Prim2 = _pObj2->GetPhysState().m_Prim[0];

	CXR_Model* pModel2 = _pWorldData->GetResource_Model(Prim2.m_iPhysModel);
	M_ASSERT( pModel2, "" );
	CXR_PhysicsModel* pPhysModel2 = pModel2->Phys_GetInterface();
	M_ASSERT( pPhysModel2, "" );

	const CMat4Dfp32 &BspTransform = _pObj2->GetPositionMatrix();

	CXR_PhysicsContext Context(BspTransform);
	Context.m_PhysGroupMaskThis = Prim2.m_PhysModelMask;
	Context.m_PhysGroupMaskCollider = _PO.m_PhysPrim.m_PhysModelMask;
	Context.m_pWC = _pPhysState->Debug_GetWireContainer();

	const int MaxCollisions = 100;
	uint8 CollisionInfoBuf[MaxCollisions * sizeof(CCollisionInfo)];
	CCollisionInfo *CollisionInfo = (CCollisionInfo *)(&CollisionInfoBuf);

	int nCollisions = 0;
	switch( _PO.m_PhysPrim.m_PrimType )
	{

	case OBJECT_PRIMTYPE_BOX:
		{
			CPhysOBB OBB;//,OBBFake;
			OBB.SetDimensions(_Dim);
			OBB.SetPosition(_PO.m_Transform);
		//	OBBFake = OBB;
		//	OBBFake.m_C += _PO.m_Velocity.m_Move;
		//	if( _PO.m_Velocity.m_Move.AlmostEqual(CVec3Dfp32(0),0.1f) ) OBBFake.m_C -= CVec3Dfp32(0,0,1.0f);
		//	nCollisions = pPhysModel2->Phys_IntersectBox(&Context,OBB,OBBFake,11,CollisionInfo);
			nCollisions = pPhysModel2->Phys_CollideBox(&Context,OBB,_pObj1->GetPhysState().m_MediumFlags,CollisionInfo,MaxCollisions);
		}
		break;

	case OBJECT_PRIMTYPE_CAPSULE:
		{
			CPhysCapsule Cps;
			Cps.SetPosition(_PO.m_Transform);
			Cps.SetDimensions(_Dim);
			nCollisions = pPhysModel2->Phys_CollideCapsule(&Context,Cps,_pObj1->GetPhysState().m_MediumFlags,CollisionInfo,MaxCollisions);
		}
		break;

	}

	M_ASSERT(nCollisions <= MaxCollisions, "!");

	CWD_RigidBody2 *pBody1 = _PO.m_pRB;

	int nTotCol = 0;
	for (int i = 0; i < nCollisions && i < _pContactInfo.Len(); i++)
	{	
		CVec4Dfp32 N;
		N = -CollisionInfo[i].m_Plane.n;
		N[3] = 0.0f;

		pBody1->m_CollisionToWorldCluster.AddNormal(N);

		_pContactInfo[nTotCol].m_iRB1= pBody1->m_iRB;
		_pContactInfo[nTotCol].m_iRB2 = 0;
		_pContactInfo[nTotCol].m_Normal = N;
		_pContactInfo[nTotCol].m_PointOfCollision = CollisionInfo[i].m_Pos * (1.0f / 32.0f);
		_pContactInfo[nTotCol].m_Distance = M_Fabs(CollisionInfo[i].m_Distance) * (1.0f / 32.0f);
		_pContactInfo[nTotCol].m_bIsColliding = CollisionInfo[i].m_bIsCollision;
		_pContactInfo[nTotCol].m_BiasVelocity = M_VZero();
		_pContactInfo[nTotCol].m_UserData1 = _pObj1->m_iObject;
		_pContactInfo[nTotCol].m_UserData2 = _pObj2->m_iObject;
		CollisionInfo[i].m_Friction = Min(CollisionInfo[i].m_Friction,pBody1->m_StaticFriction);
		_pContactInfo[nTotCol].m_Friction_Elasticity = CVec4Dfp32(CollisionInfo[i].m_Friction * 0.75f, CollisionInfo[i].m_Friction, 0.4f, 0.0f);

		nTotCol++;
	}
	return nTotCol;
}


static int CollidePrimCharacter(const CWObject_CoreData* _pObj,const CWPhys_ClusterObject &_PO,const CVec3Dfp32 &_Dim,
								const CWObject_CoreData* _pChar,
								TAP_RCD<CWD_ContactInfo> _pContactInfo)
{
	CPhysOBB Obb2;

	//Character
	int iRB2 = _pChar->m_pRigidBody2 ? _pChar->m_pRigidBody2->m_iRB : 0;
	const CWO_PhysicsPrim& CharPhysPrim = _pChar->GetPhysState().m_Prim[0];
	CMat4Dfp32 P2;
	P2.Unit();
	P2.GetRow(3) = _pChar->GetPosition() + CharPhysPrim.GetOffset();
	Obb2.SetPosition(P2);
	Obb2.SetDimensions(CharPhysPrim.GetDim());

	CCollisionInfo CollisionInfo[10];
	int nColl = CollidePrimOBB(_pObj,_PO,_Dim,Obb2,CollisionInfo,10);
	int nTotCol = MinMT(nColl, _pContactInfo.Len());

	for (int i = 0; i < nTotCol; i++)
	{
		CWD_ContactInfo& Dest = _pContactInfo[i];
		Dest.m_iRB1 = _PO.m_pRB->m_iRB;
		Dest.m_iRB2 = 0; // iRB2;	  -- NOTE: 0 means world -> i.e. infinite mass -> lower risk of getting stuck
		Dest.m_BiasVelocity = M_VZero();
		Dest.m_UserData1 = _pObj->m_iObject;
		Dest.m_UserData2 = _pChar->m_iObject;
		Dest.m_Friction_Elasticity = CVec4Dfp32(0.4f, 0.3f, 0.0f, 0.0f);
		Dest.m_bIsColliding = CollisionInfo[i].m_bIsCollision;

		//M_TRACE("[CollideObjectCharacter] Distance: %.5f\n", CollisionInfo[i].m_Distance);

		//if (CollisionInfo[i].m_bIsValid)
		{
			// Hack: make sure normal points away from character
			CVec3Dfp32 a = (_PO.m_Transform.GetRow(3) - Obb2.m_C).Normalize();
			CVec3Dfp32 n = CollisionInfo[i].m_Plane.n;
			n *= Sign(a * n); 
			Dest.m_Normal = n;
			Dest.m_PointOfCollision = CollisionInfo[i].m_Pos * (1.0f / 32.0f);
			Dest.m_Distance = M_Fabs(CollisionInfo[i].m_Distance) * (1.0f / 32.0f)  * 1.0f;  // *10 -> make sure the fucker stays away from player!
		}/*
		else
		{ // wtf?  (better safe than sorry)
			Dest.m_Normal = (_PO.m_Transform.GetRow(3) - Obb2.m_C).Normalize();
			Dest.m_Normal.Normalize();
			Dest.m_PointOfCollision = _pChar->GetPosition() * (1.0f / 32.0f);
			Dest.m_Distance = 8.0f / 32.0f;
		}*/
	}
	return nTotCol;

}

static int CreatePushCollisionPrim(const CWObject_CoreData* _pObj,const CWPhys_ClusterObject &_PO,const CVec3Dfp32 &_Dim,
								   const CWObject_CoreData* _pChar, 
								   TAP_RCD<CWD_ContactInfo> _pContactInfo)
{
	CBox3Dfp32	Box2;
	CPhysOBB	Obb2;

	// Character
	int iRB2 = _pChar->m_pRigidBody2 ? _pChar->m_pRigidBody2->m_iRB : 0;
	Box2 = *_pChar->GetAbsBoundBox();
	CMat4Dfp32 P2;
	P2.Unit();
	Box2.GetCenter(P2.GetRow(3));
	P2.GetRow(3) += _pChar->GetMoveVelocity() * 16.0f;	// place the box in front of character
	Obb2.SetPosition(P2);
	CVec3Dfp32 Extent2 = Box2.m_Max - Box2.m_Min;
	Obb2.SetDimensions(Extent2);
	Obb2.m_E *= 0.5f;

	CCollisionInfo CollisionInfo[10];
	uint nColl = CollidePrimOBB(_pObj,_PO,_Dim,Obb2,CollisionInfo,10);
	int nTotCol = MinMT(nColl, _pContactInfo.Len());

	for (int i = 0; i < nTotCol; i++)
	{
		CWD_ContactInfo& Dest = _pContactInfo[i];
		Dest.m_iRB1 = _PO.m_pRB->m_iRB;
		Dest.m_iRB2 = iRB2;	
		Dest.m_BiasVelocity = M_VZero();
		Dest.m_UserData1 = 0; // _pObj->m_iObject; -- don't want this to start playing impact sounds
		Dest.m_UserData2 = 0; // _pChar->m_iObject; -- don't want this to create phys damage
		Dest.m_Friction_Elasticity = CVec4Dfp32(0.4f, 0.3f, 0.0f, 0.0f);
		Dest.m_bIsColliding = 0;//CollisionInfo[i].m_bIsCollision;
		Dest.m_Normal = (_PO.m_Transform.GetRow(3) - _pChar->GetPosition()).Normalize();
		Dest.m_Distance = 0.0f; //M_Fabs(CollisionInfo[i].m_Distance) * (1.0f / 32.0f);

		//if (CollisionInfo[i].m_bIsValid)
			Dest.m_PointOfCollision = CollisionInfo[i].m_Pos * (1.0f / 32.0f);
		//else
		//	Dest.m_PointOfCollision = Obb2.m_C * (1.0f / 32.0f);  // whatever...
	}
	return nTotCol;
}

//-----------------------------------------------------------------------------


static CCollisionHash s_TestColl2;

void CWD_WorldModelCollider::OnSpawnWorld(CMapData* _pWorldData)
{
	m_lBouyancyObjects.QuickSetLen(0);
}

void CWD_WorldModelCollider::OnWorldClose()
{
}

void CWD_WorldModelCollider::PreApplyExternalForces(CWD_DynamicsWorld *_pWorld, void *_pArgument1, void *_pArgument2)
{
//	m_lBouyancyObjects.QuickSetLen(0);
}

void CWD_WorldModelCollider::ApplyExternalForces(CWD_DynamicsWorld *_pWorld, void *_pArgument1, void *_pArgument2) 
{
	CWorld_Server *pServer= (CWorld_Server *) _pArgument1;
	CWorld_ServerCore *pServerCore= safe_cast<CWorld_ServerCore>(pServer);

	CWorld_PhysState *pPhysState = safe_cast<CWorld_PhysState>(pServer);
	CMapData *pWorldData = pPhysState->GetMapData();

#if 1
	TAP<CWD_RigidBody2 *> pBouyancyObjects = m_lBouyancyObjects;

	CMat4Dfp32 T;
	CBox3Dfp32 Box;

	for (int i = 0; i < pBouyancyObjects.Len(); i++)
	{
		CWD_RigidBody2 *pBody = pBouyancyObjects[i];
		int iRB = pBody->m_iRB;
		CWD_RigidBodyState *pState = &_pWorld->GetRigidBodyState(iRB);
		CWObject_CoreData *pCore = (CWObject_CoreData *) pBody->m_pUserData;

		const CWPhys_ClusterObject * pPCO = NULL;
		CVec3Dfp32 Dim = pPCO->m_PhysPrim.GetDim();
		if( pBody->m_pUserData2 )
		{
			pPCO = (const CWPhys_ClusterObject*)pBody->m_pUserData2;
		}

		_pWorld->GetTransform(pBody->m_iRB, T);
		if( pPCO ) GetClusterItemBoundBox(*pPCO, Dim, Box);
		else GetBoundBox(pWorldData, pCore, Box);

		Box.m_Min -= pBody->m_CenterOfMass;
		Box.m_Max -= pBody->m_CenterOfMass;

		//CBouyancyFunctions::CBouyancyForce Force;
		T.GetRow(3) *= 32.0f;
		CBouyancyFunctions::CBouyancyForce Force = CBouyancyFunctions::CalculateBouyancyForce(pServer, pBody->m_iRB, T, Box);
		Force.m_Force *= 0.1f;
		T.GetRow(3) *= 1.0f / 32.0f;
		//Force.m_Force = CVec3Dfp32(0.0f);
		//Force.m_ApplyAt = CVec3Dfp32(0.0f);
		//Force.m_VolumeInLiquid = 1.0f;

#if 1

		//CRigidBodyState *pState = (CRigidBodyState *) pBody->GetBodyState();

		CVec3Dfp32 Size = (Box.m_Max - Box.m_Min) * (10.0f / 32.0f);
		fp32 Volume = Size.k[0] * Size.k[1] * Size.k[2];

		CVec3Dfp32 AreaX = CVec3Dfp32(Size[1] * Size[2], 0.0f, 0.0f);
		CVec3Dfp32 AreaY = CVec3Dfp32(0.0f, Size[0] * Size[2], 0.0f);
		CVec3Dfp32 AreaZ = CVec3Dfp32(0.0f, 0.0f, Size[0] * Size[1]);

		T.UnitNot3x3();
		AreaX *= T;
		AreaY *= T;
		AreaZ *= T;

		CVec3Dfp32 Vel = pState->GetVelocity();
		Vel.Normalize();

		fp32 KX = M_Fabs(Vel * AreaX);
		fp32 KY = M_Fabs(Vel * AreaY);
		fp32 KZ = M_Fabs(Vel * AreaZ);

		fp32 K = KX + KY + KZ;

		pServer->Phys_AddImpulse(pBody, Force.m_ApplyAt, Force.m_Force);

#ifndef M_RTM
		pServer->Debug_RenderVector(Force.m_ApplyAt, CVec3Dfp32(0.0f, 0.0f, 32.0f), 0xffff0000, 1.0f/20.0f, false);
#endif

		CVec3Dfp32 PointVelocity = pState->GetAngularVelocity() / (Force.m_ApplyAt);

		K = Clamp(K, 0.0f, 800.0f);
		if (Force.m_Force.LengthSqr() > _FP32_EPSILON)
		{
			fp32 Factor = Force.m_VolumeInLiquid / Volume;
			Factor *= BUOYANCYLIQUIDDRAG;
			CVec3Dfp32 FrictionForce = -pState->GetVelocity() * K * Factor;
			pServer->Phys_AddForce(pBody, FrictionForce);
		}

#endif

	}
#endif

}

void CWD_WorldModelCollider::PostApplyExternalForces(CWD_DynamicsWorld *_pWorld, void *_pArgument1, void *_pArgument2) 
{
	TAP<CWD_RigidBody2 *> pBouyancyObjects = m_lBouyancyObjects;

	for (int i = 0; i < pBouyancyObjects.Len(); i++)
	{
		CWD_RigidBody2 *pBody = pBouyancyObjects[i];
		CWD_RigidBodyState *pState = &_pWorld->GetRigidBodyState(pBody->m_iRB);

		pState->m_AngularVelocity[0] *= BUOYANCYLIQUIDANGULARDAMPENING;
		pState->m_AngularVelocity[1] *= BUOYANCYLIQUIDANGULARDAMPENING;
		pState->m_AngularVelocity[2] *= BUOYANCYLIQUIDANGULARDAMPENING;
	}

	m_lBouyancyObjects.QuickSetLen(0);
}

int CWD_WorldModelCollider::Collide(const CWD_DynamicsWorld *_pWorld, TArray<CWD_RigidBody2 *> &_lpRigidBodies, TAP_RCD<CWD_ContactInfo> _pContactInfo, void *_pArgument1, void *_pArgument2)
{
	MSCOPESHORT(CWO_DynamicsCollider::Collide_2);

	//int CollisionPrecision = _pWorld->GetCollisionPrecision();

	//const TArray<CRigidBody *>& BodyList = _pWorld->GetRigidBodyList();
//	int nBodies = _BodyList.Len();
	int nTotCollisions = 0;

//	CMapData *pWorldData = (CMapData *) _pArgument1;
//	CWorld_PhysState *pPhysState = (CWorld_PhysState *) _pArgument2;

//	CWorld_PhysState *pPhysState = safe_cast<CWorld_PhysState>(_pArgument1);
	CWorld_Server *pServer= (CWorld_Server *) _pArgument1;
	CWorld_ServerCore *pServerCore= safe_cast<CWorld_ServerCore>(pServer);
	CWorld_PhysState *pPhysState = (CWorld_PhysState *) pServer;

	CMapData *pWorldData = pPhysState->GetMapData();

	s_TestColl2.SetEmpty();

	const int N_ENUM_IDS = 200;
	int16 EnumIds[N_ENUM_IDS];

	//Store these so newly added bodies won't be processed until next step
	CWD_RigidBody2 ** ppRB2 = _lpRigidBodies.GetBasePtr()+1;
	uint32 nBodies = _lpRigidBodies.Len()-1;

	for (int i = 0; i < nBodies; i++)
	{
		const CWD_RigidBody2 *pBody =  ppRB2[i];

//		if (_pWorld->IsStationary(pBody->m_iRB))
//			continue;

#ifndef M_RTM
		//if (pPhysState->Debug_GetWireContainer())
//			DebugRender(pWorldData, pPhysState->Debug_GetWireContainer(), pBody->m_pCoreData);
#endif

		//		int EnumerateBox(const CWO_EnumParams_Box& _Params, int16* _pEnumRetIDs, int _MaxEnumIDs);
		//		EnumBox.m_Box = 

//		const CWObject_CoreData *pObj = safe_cast<CWObject_CoreData>(pBody->m_pUserData);

		const CWObject_CoreData *pObj = (CWObject_CoreData *) pBody->m_pUserData;
		int iRB1 = pBody->m_iRB;

		const CWPhys_ClusterObject *pPCO = NULL;
		CVec3Dfp32 Dim;
		if( pBody->m_pUserData2 )
		{
			pPCO = (const CWPhys_ClusterObject*)pBody->m_pUserData2;
			Dim = pPCO->m_PhysPrim.GetDim();
		}
		
		CBox3Dfp32 BoundBox;
		if( !pPCO ) GetBoundBox(pWorldData, pObj, BoundBox);
		else GetClusterItemBoundBox(*pPCO,Dim, BoundBox);

		// Primitive is only required if no physcluster present
		CWO_EnumParams_Box EnumBoxParams;
		const CWO_PhysicsState& PhysState = pObj->GetPhysState();
		if ( (!PhysState.m_nPrim) && (!pPCO) )
			continue;

		EnumBoxParams.m_ObjectFlags = PhysState.m_ObjectIntersectFlags;		// <-- Swapped these assignments	-mh
		EnumBoxParams.m_ObjectIntersectFlags = PhysState.m_ObjectFlags;
		EnumBoxParams.m_ObjectNotifyFlags = 0;
		BoundBox.m_Min -= CVec3Dfp32(1,1,1);
		BoundBox.m_Max += CVec3Dfp32(1,1,1);
		if( !pPCO ) BoundBox.Transform(pObj->GetPositionMatrix(), EnumBoxParams.m_Box);
		else EnumBoxParams.m_Box = BoundBox;

		EnumBoxParams.m_ObjectFlags |= OBJECT_FLAGS_PLAYERPHYSMODEL;
		EnumBoxParams.m_ObjectIntersectFlags |= OBJECT_FLAGS_PLAYERPHYSMODEL;
		EnumBoxParams.m_RigidBodyID = i;  // Don't want to do redundant overlap tests
		uint16 ObjectIntersectFlags = PhysState.m_ObjectIntersectFlags;

		if (true)
		{
			ObjectIntersectFlags |= OBJECT_FLAGS_PLAYERPHYSMODEL;
			//ObjectIntersectFlags &= ~OBJECT_FLAGS_PHYSMODEL;
		}
/*
		AABB and ABS-AABB temp rendering for clusters (sloooooowww)

		if( pObj->m_pPhysCluster )
		{
			pServer->Debug_RenderAABB(*pObj->GetAbsBoundBox(),CPixel32(0xFF,0xFF,0,0xFF),0.1f,false);
			pServer->Debug_RenderAABB(BoundBox,CPixel32(0xFF,0,0xFF,0xFF),0.1f,false);
		}
*/

		int nPresumptiveColl = pPhysState->m_spSpaceEnum->EnumerateBox(EnumBoxParams, EnumIds, N_ENUM_IDS);

		for (int j = 0; j < nPresumptiveColl; j++)
		{
			CWObject_CoreData *pObj2 = pPhysState->Object_GetCD(EnumIds[j]);

			//Cluster self-collision
			if( pObj->m_pPhysCluster && (pObj == pObj2) )
			{
				int nActivated = 0;

				//Test cluster -> prim
				for(int i = 0;i < pObj2->m_pPhysCluster->m_lObjects.Len();i++)
				{
					CWPhys_ClusterObject &PCO2 = pObj2->m_pPhysCluster->m_lObjects[i];
					CWD_RigidBody2 * pRB2 = PCO2.m_pRB;
					if( (pRB2 == pBody) || _pWorld->IsConnected(pBody->m_iRB,pRB2->m_iRB) ) continue;
					if( s_TestColl2.Test(iRB1,pRB2->m_iRB) ) continue;

					int nColl = CollidePrimPrim(pObj,*pPCO,Dim,pObj2,PCO2,PCO2.m_PhysPrim.GetDim(),
						_pContactInfo + nTotCollisions);
					nTotCollisions += nColl;

					s_TestColl2.Add(iRB1,pRB2->m_iRB);

					nActivated += pRB2->IsStationary();
					pRB2->SetStationary(false);
				}

				//Only do this if we have actually added something, no point in iterating again otherwise
				if( nActivated )
				{
					pServer->Phys_AddRigidBodyToSimulation(pObj->m_iObject,true);
					ppRB2 = _lpRigidBodies.GetBasePtr()+1;
				}

				continue;
			}

			if (pObj == pObj2) continue; 

			if (!pObj2)
				continue;
			const CWO_PhysicsState& PhysState2 = pObj2->GetPhysState();
			if (!PhysState2.m_nPrim)
				continue;

			// Should test all primitives unless we make it an official rule that only Prim 0 can be used for dynamics.
			const CWO_PhysicsPrim& PhysPrim1 = PhysState.m_Prim[0];
			const CWO_PhysicsPrim& PhysPrim2 = PhysState2.m_Prim[0];
			if (!((PhysState.m_ObjectFlags & PhysPrim1.m_ObjectFlagsMask & PhysState2.m_ObjectIntersectFlags) ||
				(PhysState2.m_ObjectFlags & PhysPrim2.m_ObjectFlagsMask & ObjectIntersectFlags)))
				continue;

			const CWD_RigidBody2 *pBody2 = pObj2->m_pRigidBody2;
			int iRB2 = -1;

			if( pObj2->m_pPhysCluster )
			{
				TAP<CWPhys_ClusterObject> pPCO = pObj2->m_pPhysCluster->m_lObjects;
				int nAdd = 0;
				for(int i = 0;i < pPCO.Len();i++)
				{
					CWD_RigidBody2 * pRB = pPCO[i].m_pRB;
					if( pRB->m_bInSimulation ) continue;

					// If it can be figured out how to freeze subobjects only, remove this
					//pRB->m_FreezeCounter = 0;
					nAdd++;
					pRB->SetStationary(false);
				}
				if( nAdd )
				{
					((CWObject*)pObj2)->ClientFlags() &= ~CWO_CLIENTFLAGS_NOREFRESH;
					pServer->Phys_AddRigidBodyToSimulation(pObj2->m_iObject,true);
					ppRB2 = _lpRigidBodies.GetBasePtr()+1;
				}
			}
			else if (pBody2)
			{
				iRB2 = pBody2->m_iRB;

				if (!pBody2->m_bInSimulation)
				{
					// THESE STATEMENTS HAS TO BE IN THIS ORDER DUE TO FREEZECOUNTER!
					int OldFreezeCounter = pObj2->m_pRigidBody2->m_FreezeCounter;
					pServer->Phys_SetStationary(pObj2->m_iObject, false);
					pObj2->m_pRigidBody2->m_FreezeCounter = OldFreezeCounter;
					//pObj2->m_pRigidBody2->m_FreezeCounter = 19;
					pServer->Phys_AddRigidBodyToSimulation(pObj2->m_iObject, true);
					ppRB2 = _lpRigidBodies.GetBasePtr()+1;
				}
			}

			//			if (_pWorld->IsStationary(pBody) && pBody2 && _pWorld->IsStationary(pBody2))
			//				continue;

			//			if (_pWorld->IsStationary(pBody) && pBody2 == NULL)
			//				continue;

			int id1 = pBody->m_iRB;//pObj->m_iObject;

			//We cannot have body ID:s larger than 2048. Dangerous?
			int id2 = (pBody2) ? pBody2->m_iRB : pObj2->m_iObject | 0x800; //pObj2->m_iObject;

			M_ASSERT(id1 < N_MAX_COLL_HACK, "");
			M_ASSERT(id2 < N_MAX_COLL_HACK, "");


			//There is no client collision for physcluster objects as of 2006-11-14, pushcollisions disabled but functional
			bool bObj1IsChar = ((PhysState.m_ObjectFlags & OBJECT_FLAGS_CHARACTER) != 0) && (pObj->m_pPhysCluster == NULL);
			bool bObj2IsChar = ((PhysState2.m_ObjectFlags & OBJECT_FLAGS_CHARACTER) != 0) && (pObj2->m_pPhysCluster == NULL);
			if (bObj2IsChar && !bObj1IsChar && _pWorld->IsActive(pBody->m_iRB))
			{
				int nColl = (!pObj->m_pPhysCluster) ?
					CollideObjectCharacter(_pWorld, pWorldData, pObj, pObj2, _pContactInfo + nTotCollisions) :
					0;//CollidePrimCharacter(pObj,*pPCO,pObj2,_pContactInfo + nTotCollisions);
				nTotCollisions += nColl;

				// push object?
				if (pServerCore->m_bCharacterPush)
				{
					int nColl = (!pObj->m_pPhysCluster) ?
						CreatePushCollision(_pWorld, pWorldData, pObj, pObj2, _pContactInfo + nTotCollisions) :
						0;//CreatePushCollisionPrim(pObj,*pPCO,pObj2, _pContactInfo + nTotCollisions);
					nTotCollisions += nColl;
				}
		
				if (!s_TestColl2.Test(id1, id2))
					s_TestColl2.Add(id1, id2);
			}

			if (!s_TestColl2.Test(id1, id2))
			{
				//const CWO_PhysicsPrim& Prim1 = pObj->GetPhysState().m_Prim[0];
				//const CWO_PhysicsPrim& Prim2 = pObj2->GetPhysState().m_Prim[0];
				CXR_Model* pModel2 = pWorldData->GetResource_Model(PhysPrim2.m_iPhysModel);

				if (pModel2 && (pObj2->m_pPhysCluster != NULL))
				{
					const CMat4Dfp32 &LiqTransform = pObj2->GetPositionMatrix();
					//CBox3Dfp32 Box = BoundBox;

					//Box.m_Min -= pBody->m_CenterOfMass;
					//Box.m_Max -= pBody->m_CenterOfMass;

					if (pObj2->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_MEDIUMQUERY)
					{
						CMat4Dfp32 T = _pWorld->GetTransform(pBody->m_iRB);
						//((CWD_RigidBody2 *)pBody)->SetStationary(false);
						pServer->Phys_SetStationary(pBody->m_iRB, false);

						// Lite av ett hack...
						((CWD_RigidBodyState &) _pWorld->GetRigidBodyState(pBody->m_iRB)).m_FreezeCounter = 0;

						//((CWD_RigidBody *) pBody)->SetStationary(false);

						s_TestColl2.Add(id1, id2);

						bool Found = false;
						for (int i = 0; i < m_lBouyancyObjects.Len(); i++)
						{
							if (m_lBouyancyObjects[i] == pBody)
							{
								Found = true;
								break;
							}
						}

						if (!Found)
							m_lBouyancyObjects.Add((CWD_RigidBody2 *) pBody);
					}
				}
			} 



			/*
			   BSP collide
			*/
			//			if (pBody != pBody2 && pBody2 == NULL && pObj2->m_iClass == s_iWorldSpawnClass && pBody->IsActive())
			if (pBody != pBody2 && pBody2 == NULL && pObj2->IsClass(class_WorldPlayerPhys) && _pWorld->IsActive(iRB1))
			{
				if (!_pWorld->IsConnectedToWorld(iRB1) && !s_TestColl2.Test(id1, id2) )
				{
					//const CWO_PhysicsPrim& Prim = pObj2->GetPhysState().m_Prim[0];
					CXR_Model* pModel = pWorldData->GetResource_Model(PhysPrim2.m_iPhysModel);
					if (!pModel)
						continue;

					int nColl = 0;
					if( pObj->m_pPhysCluster )
					{
						nColl = CollidePrimBSP(pObj,*pPCO,Dim,BoundBox,pObj2,pWorldData,
							_pContactInfo + nTotCollisions,pPhysState);
					}
					else
					{

						CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();
						if (!pPhysModel)
							continue;
						//pPhysModel->Phys_IntersectBox()

						//const CWO_PhysicsPrim& Prim1 = pObj->GetPhysState().m_Prim[0];
						if (PhysPrim1.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL)
						{
							CXR_Model* pModel1 = pWorldData->GetResource_Model(PhysPrim1.m_iPhysModel);
							if (!pModel1)
								continue;
							CXR_PhysicsModel* pPhysModel1 = pModel1->Phys_GetInterface();
							if (!pPhysModel1)
								continue;

							int ModelClass1 = pModel1->GetModelClass();
							if (ModelClass1 == CXR_MODEL_CLASS_BSP2)
							{
								CXR_Model_BSP2 *pModelBSP2_1 = (CXR_Model_BSP2 *) pModel1;
								spCXR_IndexedSolidContainer32 spIndexedSolids1 = pModelBSP2_1->m_spIndexedSolids;
								if (spIndexedSolids1 != NULL)
								{
									nColl = CollideSolidBsp(_pWorld, pBody, spIndexedSolids1, 
															pObj2,
															_pContactInfo + nTotCollisions,
															pWorldData,
															pPhysState);
								}
							}
							else {

								int breakme = 0;

							}
						}
						else if (PhysPrim2.m_PrimType == OBJECT_PRIMTYPE_BOX)
						{
							
						}
					}

					nTotCollisions += nColl;

					s_TestColl2.Add(id1, id2);
				}

			}

			//Note that we *never* test ordinary objects->physcluster, only the other way around, to reduce redundant code
			if (!s_TestColl2.Test(id1, id2))
			{
				int nColl = 0;

				//Test cluster -> prim
				if (pObj2->m_pPhysCluster && pObj->m_pPhysCluster)
				{
					for(int i = 0;i < pObj2->m_pPhysCluster->m_lObjects.Len();i++)
					{
						CWPhys_ClusterObject &PCO2 = pObj2->m_pPhysCluster->m_lObjects[i];
						CWD_RigidBody2 * pRB2 = PCO2.m_pRB;
						if( s_TestColl2.Test(id1,pRB2->m_iRB) || _pWorld->IsConnected(iRB1,pRB2->m_iRB) ) continue;

						nColl = CollidePrimPrim(pObj,*pPCO,Dim,pObj2,PCO2,PCO2.m_PhysPrim.GetDim(),
							_pContactInfo + nTotCollisions);
						nTotCollisions += nColl;

						s_TestColl2.Add(id1,pRB2->m_iRB);
					}

				}
				else if (pBody2 != NULL && pBody != pBody2 && !_pWorld->IsConnected(iRB1, iRB2)) 
				{
//					if (!(!pBody->IsActive() && !pBody2->IsActive()))
					{

						//InspectSolidData(pWorldData, pObj);
						//InspectSolidData(pWorldData, pObj2);

						bool solidsolid = false;


						//const CWO_PhysicsPrim& Prim1 = pObj->GetPhysState().m_Prim[0];
						//const CWO_PhysicsPrim& Prim2 = pObj2->GetPhysState().m_Prim[0];


//						if (CollisionPrecision == 0)
						if( PhysPrim2.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL )
						{

							//Test a prim -> object
							if( pObj->m_pPhysCluster )
							{
								CXR_Model* pModel1 = pWorldData->GetResource_Model(PhysPrim2.m_iPhysModel);
								if (pModel1->GetModelClass() == CXR_MODEL_CLASS_BSP2)
								{
									CXR_Model_BSP2 *pModelBSP2_1 = (CXR_Model_BSP2 *) pModel1;
									if (pModelBSP2_1->m_spIndexedSolids == NULL) continue;

									nColl = CollidePrimSolid(_pWorld, pObj, *pPCO, Dim, pObj2, pWorldData, _pContactInfo + nTotCollisions);

									nTotCollisions += nColl;
									s_TestColl2.Add(id1, id2);
								}
							}

							//Test object
							else if ((PhysPrim1.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL) &&
								!(pObj2->m_pPhysCluster) )
							{
								CXR_Model* pModel1 = pWorldData->GetResource_Model(PhysPrim1.m_iPhysModel);
								CXR_Model* pModel2 = pWorldData->GetResource_Model(PhysPrim2.m_iPhysModel);
								//							CXR_PhysicsModel* pPhysModel1 = pModel1->Phys_GetInterface();
								//							CXR_PhysicsModel* pPhysModel2 = pModel2->Phys_GetInterface();

								int ModelClass1 = pModel1->GetModelClass();
								int ModelClass2 = pModel2->GetModelClass();
								if (ModelClass1 == CXR_MODEL_CLASS_BSP2 && ModelClass2 == CXR_MODEL_CLASS_BSP2)
								{
									solidsolid = true;

									CXR_Model_BSP2 *pModelBSP2_1 = (CXR_Model_BSP2 *) pModel1;
									CXR_Model_BSP2 *pModelBSP2_2 = (CXR_Model_BSP2 *) pModel2;
									CXR_IndexedSolidContainer32 *pIndexedSolids1 = pModelBSP2_1->m_spIndexedSolids;
									CXR_IndexedSolidContainer32 *pIndexedSolids2 = pModelBSP2_2->m_spIndexedSolids;
									if (pIndexedSolids1 == NULL || pIndexedSolids2 == NULL) continue;

									nColl = CollideSolidSolid(_pWorld, pObj, pObj2, pWorldData, _pContactInfo + nTotCollisions);

									nTotCollisions += nColl;
									s_TestColl2.Add(id1, id2);
							}
						}
					}
				}
			}
		}
	}
}	
	return nTotCollisions;
}

