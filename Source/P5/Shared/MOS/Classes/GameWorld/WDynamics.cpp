
#include "PCH.H"
#include "WDynamics.h"

#include "../../XRModels/Model_BSP2/WBSP2Model.h"
#include "Server/WServer_Core.h"

//#define USE_DYNAMICSENGINE1


#ifndef M_RTM
//#define WDYNAMICS_VERBOSE
#endif

//#pragma optimize( "", off )
//#pragma inline_depth(0)

#include "../../XR/Phys/GjkCollision.h"
#include "../../XR/Phys/SeparatingAxis.h"
#include "WDynamicsConvexCollision.h"

#define IsValidFloat(f) ((f) == (f))
#define IsValidVec3(v) (IsValidFloat(v.k[0]) && IsValidFloat(v.k[1]) && IsValidFloat(v.k[2]))

#define BUOYANCYGRAVITY (9.81f)
#define BUOYANCYLIQUIDDRAG (25.0f)

enum
{
	class_CharPlayer =			MHASH6('CWOb','ject','_','Char','Play','er'),
	class_Object = 				MHASH5('CWOb','ject','_','Obje','ct'),
	class_WorldSpawn = 			MHASH6('CWOb','ject','_','Worl','dSpa','wn'),
	class_WorldPlayerPhys = 	MHASH7('CWOb','ject','_','Worl','dPla','yerP','hys'),
};


/*
	Contact-reduction

	I) k-Means-clustering
 */

class CContactReduction
{
public:

	struct CCluster
	{
		CVec3Dfp32 m_Normal;
		CVec3Dfp32 m_NewNormal;
		int m_nInCluster;
		int m_nNewInCluster;

		int m_ClusterID;

	};

	static bool Cluster(CCluster *_pClusters, int _nClusters, TAP<CContactInfo> _pContactInfo)
	{
		M_ASSERT(_pContactInfo.Len() > 3, "Internal error!");

		const int MaxIter = 5;

		int nMoved;
		int nIter = 0;

		for (int j = 0; j < _nClusters; j++)
		{
			_pClusters[j].m_nInCluster = 0;
			_pClusters[j].m_nNewInCluster = 0;
			_pClusters[j].m_ClusterID = j;
			_pClusters[j].m_Normal = _pContactInfo[j].m_Normal.Getfp32();
			_pClusters[j].m_NewNormal = CVec3Dfp32(0.0f);
		}

		do 
		{
			nMoved = 0;

			for (int j = 0; j < _nClusters; j++)
			{
				//_pClusters[j].m_NewNormal = CVec3Dfp32(0.0f);
				//_pClusters[j].m_nNewInCluster = 0;
				//_pClusters[j].m_nInCluster = 0;
			}

			for (int i = 0; i < _pContactInfo.Len(); i++)
			{
				fp32 BestClusterDistance = -100.0f;
				int iBestCluster = -1;

				if (nIter == 0)
					_pContactInfo[i].m_iCluster = 0;

				for (int j = 0; j < _nClusters; j++)
				{
					fp32 D = _pClusters[j].m_Normal * _pContactInfo[i].m_Normal.Getfp32();

					if (D > BestClusterDistance)
					{
						BestClusterDistance = D;
						iBestCluster = j;
					}

					// m_iCluster fattas.
					/*
					if (_pContactInfo[i].m_iCluster != iBestCluster)
					{
					_pContactInfo[i].m_iCluster = iBestCluster;
					nMoved++;
					}*/

				//	_pClusters[j].m_NewNormal += _pContactInfo[i].m_Normal.Getfp32();
//					_pClusters[j].m_nInCluster++;
				//	_pClusters[j].m_nNewInCluster++;
				}

				_pContactInfo[i].m_iCluster = iBestCluster;
			}

			for (int j = 0; j < _nClusters; j++)
			{
				int nInCluster = 0;
				CVec3Dfp32 NewNormal(0.0f);
				for (int i = 0; i < _pContactInfo.Len(); i++)
				{
					if (_pContactInfo[i].m_iCluster == j)
					{
						NewNormal += _pContactInfo[i].m_Normal.Getfp32();
						nInCluster++;
					}
				}

				if (nInCluster > 0)
				{
					NewNormal *= 1.0f / nInCluster;
				}

				_pClusters[j].m_Normal = NewNormal;
				_pClusters[j].m_nInCluster = nInCluster;

/*				_pClusters[j].m_Normal = _pClusters[j].m_NewNormal * (1.0f / fp32(_pClusters[j].m_nNewInCluster));
				_pClusters[j].m_NewNormal  = CVec3Dfp32(0.0f);
				_pClusters[j].m_nInCluster = _pClusters[j].m_nNewInCluster;
				_pClusters[j].m_nNewInCluster = 0;*/
			}

			nIter++;
		}
		while (nMoved != 0 && nIter < MaxIter);

		if (nMoved == 0)
		{
			return true;
		}
		else
		{
			return false;
			// No convergence!!
		}
	}

	static void ProjectAndReduce(const CCluster& _Cluster, TAP<CContactInfo> _pContactInfo, TAP<CContactInfo> _pResult)
	{
		//  I. Create plane
		// II. Project
		//III. Get min max.
		// IV. Generate contacts

		CVec3Dfp32 Normal = _Cluster.m_Normal;
		/*
		for (int i = 0; i < _pContactInfo.Len(); i++)
		{
			if (_pContactInfo[i].m_iCluster == _Cluster.m_ClusterID)
			{
				Normal += _pContactInfo[i].m_Normal.Getfp32();
			}
		}
*/
		//Normal *= 1.0f / fp32(_Cluster.m_nInCluster);

		CVec3Dfp32 VectorInPlane1;
		CVec3Dfp32 VectorInPlane2;

		if (M_Fabs(Normal.k[0]) > 0.1)
		{
			VectorInPlane1 = Normal / CVec3Dfp32(0.0f, 1.0f, 0.0f);
			VectorInPlane1.Normalize();

			VectorInPlane2 = VectorInPlane1 / Normal;
			VectorInPlane2.Normalize();
		}
		else
		{
			VectorInPlane1 = Normal / CVec3Dfp32(1.0f, 0.0f, 0.0f);
			VectorInPlane1.Normalize();

			VectorInPlane2 = VectorInPlane1 / Normal;
			VectorInPlane2.Normalize();
		}

		fp32 MinD1 = _FP32_MAX;
		fp32 MaxD1 = -_FP32_MAX;
		fp32 MinD2 = _FP32_MAX;
		fp32 MaxD2 = -_FP32_MAX;

		for (int i = 0; i < _pContactInfo.Len(); i++)
		{
			if (_pContactInfo[i].m_iCluster == _Cluster.m_ClusterID)
			{
//				fp32 D1 = VectorInPlane1 * _pContactInfo[i].m_Normal;
//				fp32 D2 = VectorInPlane2 * _pContactInfo[i].m_Normal;
				fp32 D1 = VectorInPlane1 * _pContactInfo[i].m_PointOfCollision.Getfp32();
				fp32 D2 = VectorInPlane2 * _pContactInfo[i].m_PointOfCollision.Getfp32();

				MinD1 = Min(MinD1, D1);
				MaxD1 = Max(MaxD1, D1);

				MinD2 = Min(MinD2, D2);
				MaxD2 = Max(MaxD2, D2);
			}
		}

		int i1 = -1, i2 = -1, i3 = -1, i4 = -1;

		fp32 DistSq1 = _FP32_MAX;
		fp32 DistSq2 = _FP32_MAX;
		fp32 DistSq3 = _FP32_MAX;
		fp32 DistSq4 = _FP32_MAX;

		for (int i = 0; i < _pContactInfo.Len(); i++)
		{
			if (_pContactInfo[i].m_iCluster == _Cluster.m_ClusterID)
			{
//				fp32 D1 = VectorInPlane1 * _pContactInfo[i].m_Normal;
//				fp32 D2 = VectorInPlane2 * _pContactInfo[i].m_Normal;
				fp32 D1 = VectorInPlane1 * _pContactInfo[i].m_PointOfCollision.Getfp32();
				fp32 D2 = VectorInPlane2 * _pContactInfo[i].m_PointOfCollision.Getfp32();

				fp32 DistSq;
				DistSq = (D1-MinD1)*(D1-MinD1) + (D2-MinD2)*(D2-MinD2);
				if (DistSq < DistSq1)
				{
					DistSq1 = DistSq;
					i1 = i;
				}

				DistSq = (D1-MinD1)*(D1-MinD1) + (D2-MaxD2)*(D2-MaxD2);
				if (DistSq < DistSq2)
				{
					DistSq2 = DistSq;
					i2 = i;
				}

				DistSq = (D1-MaxD1)*(D1-MaxD1) + (D2-MaxD2)*(D2-MaxD2);
				if (DistSq < DistSq3)
				{
					DistSq3 = DistSq;
					i3 = i;
				}

				DistSq = (D1-MaxD1)*(D1-MaxD1) + (D2-MinD2)*(D2-MinD2);
				if (DistSq < DistSq4)
				{
					DistSq4 = DistSq;
					i4 = i;
				}
			}
		}

		_pResult[0] = _pContactInfo[i1];
		_pResult[1] = _pContactInfo[i2];
		_pResult[2] = _pContactInfo[i3];
		_pResult[3] = _pContactInfo[i4];

	}

	static int Reduce(TAP<CContactInfo> _pContactInfo, TAP<CContactInfo> _pResult)
	{
		CCluster clusters[3];

		const int nClusters = 3;

		int nContacts = 0;

		if (Cluster(clusters, nClusters, _pContactInfo))
		{
			// Join clusters?

			for (int i = 0; i < nClusters; i++)
			{
				int nInCluster = clusters[i].m_nInCluster;
				if (nInCluster > 0)
				{
					if (nInCluster > 4)
					{
						TAP<CContactInfo> pTmp = _pResult;
						pTmp.m_pArray += nContacts;
						pTmp.m_Len -= nContacts;
						ProjectAndReduce(clusters[i], _pContactInfo, pTmp);
						nContacts += 4;
//						int nResult = ProjectAndReduce(clusters[i], _pContactInfo, _pResult);
//						nContacts += nResult;
					}
					else
					{
						for (int j = 0; j < nInCluster; j++)
						{
							_pResult[j + nContacts] = _pContactInfo[j];
						}
						nContacts += nInCluster;
					}
				}
				
			}
		}
		else
		{
			// ERROR, no convergence
		}

		return nContacts;
	}

};

class CBouyancyFunctions
{
public:
	struct CBouyancyForce 
	{
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

		CVec3Dfp32 SaveP0 = p0;
		CVec3Dfp32 SaveP1 = p1;

		CXR_MediumDesc MediumDescMid;

		CCollisionInfo CollisionInfo2;

		CVec3Dfp32 p0tmp = p0;
		CVec3Dfp32 p1tmp = p0;
		p1tmp[2] = p1[2] + 72.0f;
		p0tmp[2] = p0[2] - 72.0f;

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


void CWD_DynamicsUtil::GetPhysicalProperties(CMapData* _pWorldData, const CWObject* _pObj, fp32& _Mass, CVec3Dfp32& _CenterOfMass, CVec3Dfp32& _InertiaTensor)
{
	const CWO_PhysicsState& PhysState = _pObj->GetPhysState();

#ifdef WDYNAMICS_VERBOSE
	// FIXME!
	//M_TRACE("%s, prim: %d, iModel: %d\n",_pObj->GetName(), PhysState.m_nPrim - 1, Prim.m_iPhysModel);
#endif

	SPhysicalProperties Properties;
	bool bOK = GetPhysicalProperties(_pWorldData, PhysState, Properties);

#ifndef M_RTM
	if (!bOK)
	{
		const char* pName = _pObj->GetName();
		if (pName && pName[0])
			ConOutL(CStrF("§cf00ERROR: Invalid solid data for %s", pName));
		else
			ConOutL(CStr("§cf00ERROR: Invalid solid data"));
	}
#endif

	_Mass = Properties.m_Mass;
	_CenterOfMass = Properties.m_CenterOfMass;
	_InertiaTensor = Properties.m_InertiaTensor;
}


bool CWD_DynamicsUtil::GetPhysicalProperties(CMapData* _pWorldData, const CWO_PhysicsState& _PhysState, SPhysicalProperties& _Result)
{
	bool bRet = true;
	uint nPrim = _PhysState.m_nPrim;
	if (!nPrim)
	{
		 // No phys-prims in object!
		_Result.m_CenterOfMass = 0;
		_Result.m_InertiaTensor = 0;
		_Result.m_Mass = 0;
		_Result.m_Volume = 0;
		return false;
	}
	
	CWO_PhysicsPrim Prim = _PhysState.m_Prim[ (nPrim > 1) ? 1 : 0 ]; // pick slot 1 (highres) if available, otherwise slot 0 (lowres)
	uint Mask = _PhysState.m_Prim[0].m_PhysModelMask;

	CBox3Dfp32 BBox(Prim.GetOffset() - Prim.GetDim(), Prim.GetOffset() + Prim.GetDim());

	int nSolidsFound = 0;
	{
		if (Prim.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL && Prim.m_iPhysModel)
		{
			CXR_Model* pModel = _pWorldData->GetResource_Model(Prim.m_iPhysModel);
			int ModelClass = pModel->GetModelClass();
			if (ModelClass == CXR_MODEL_CLASS_BSP2)
			{
				CXR_Model_BSP2* pModelBSP2 = safe_cast<CXR_Model_BSP2>(pModel);
				CXR_IndexedSolidContainer32* pIndexedSolids = pModelBSP2->m_spIndexedSolids;

				TAP_RCD<CXR_IndexedSolid32> pSolids;
				if (pIndexedSolids)
					pSolids = pIndexedSolids->m_lSolids;

				for (int is1 = 0; is1 < pSolids.Len(); is1++)
				{
					int iMedium = pSolids[is1].m_iMedium;
					const CXR_MediumDesc& MediumDesc = pIndexedSolids->m_lMediums[iMedium];
					if (M_BitD(MediumDesc.m_iPhysGroup) & Mask)
						nSolidsFound++;
				}
			}
			else
			{
				pModel->GetBound_Box(BBox);
			}
		}
	}
	if (nSolidsFound == 0) // selected physprim wasn't a valid bsp2 model (object with only lowres + trimesh-collision) 
		Prim = _PhysState.m_Prim[0];

	_Result.m_Mass = 0.0f;
	_Result.m_Volume = 0.0f;
	_Result.m_InertiaTensor = CVec3Dfp32(0.0f);
	_Result.m_CenterOfMass = CVec3Dfp32(0.0f);
	int nSolids = 0;

	if (Prim.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL && Prim.m_iPhysModel)
	{
		CXR_Model* pModel = _pWorldData->GetResource_Model(Prim.m_iPhysModel);
//		CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();

		int ModelClass = pModel->GetModelClass();
		if (ModelClass == CXR_MODEL_CLASS_BSP2)
		{
			CXR_Model_BSP2 *pModelBSP2 = safe_cast<CXR_Model_BSP2>(pModel);
			CXR_IndexedSolidContainer32* pIndexedSolids = pModelBSP2->m_spIndexedSolids;
			//M_ASSERT(pIndexedSolids, "");
			TAP_RCD<CXR_IndexedSolid32> pSolids;
			if (pIndexedSolids)
				pSolids = pIndexedSolids->m_lSolids;

			for (int is1 = 0; is1 < pSolids.Len(); is1++)
			{
				int iMedium = pSolids[is1].m_iMedium;

				const CXR_MediumDesc& MediumDesc = pIndexedSolids->m_lMediums[iMedium];
				if (M_BitD(MediumDesc.m_iPhysGroup) & Mask)
				{
					CBox3Dfp32 BBox = pSolids[is1].m_BoundBox;
					CVec3Dfp32 Size = (BBox.m_Max - BBox.m_Min) * (10.0f / 32.0f);
					fp32 Volume = Size.k[0] * Size.k[1] * Size.k[2];
					//Volume = logf(Volume); // Ugly hack, non-linear mass test..
					_Result.m_Volume += Volume;

					fp32 M = Volume * MediumDesc.m_Density;
					_Result.m_CenterOfMass += (BBox.m_Max + BBox.m_Min) * 0.5f * M;
					_Result.m_Mass += M;
					nSolids++;

#ifdef WDYNAMICS_VERBOSE
					M_TRACE("\tGroup: %d\n", MediumDesc.m_iPhysGroup);
					M_TRACE("\tSize(m): %s\n", (Size * (1.0f / 10.0f)).GetString().Str());
					M_TRACE("\tSize(units): %s\n", (Size * (32.0f / 10.0f)).GetString().Str());
					M_TRACE("\tDensity: %f\n", MediumDesc.m_Density);
					M_TRACE("\tMass: %f\n", M);
					M_TRACE("\n");
#endif
				}
			}


#ifdef WDYNAMICS_VERBOSE
		M_TRACE("\tTotal Mass: %f\n", _Result.m_Mass);
#endif
			//M_ASSERT(nSolids != 0, "");
			if (nSolids)
			{
				_Result.m_CenterOfMass *= (1.0f / _Result.m_Mass);

				for (int is1 = 0; is1 < pSolids.Len(); is1++)
				{
					int iMedium = pSolids[is1].m_iMedium;

					const CXR_MediumDesc& MediumDesc = pIndexedSolids->m_lMediums[iMedium];
					if (M_BitD(MediumDesc.m_iPhysGroup) & Mask)
					{
						CBox3Dfp32 BBox = pSolids[is1].m_BoundBox;
	//					CVec3Dfp32 Size = (BBox.m_Max - BBox.m_Min) * (10.0f / 32.0f);
						CVec3Dfp32 Size = (BBox.m_Max - BBox.m_Min);
						fp32 Volume = Size.k[0] * Size.k[1] * Size.k[2] * (1000.0f / (32.0f*32.0f*32.0f)) ;
						//Volume = logf(Volume); // Ugly hack, non-linear mass test..

						fp32 M = Volume * MediumDesc.m_Density;

	//					CVec3Dfp32 IT = CInertia::Block(M, Size[0] * 32.0f / 10.0f, Size[1] * 32.0f / 10.0f,  Size[2] * 32.0f / 10.0f).Getfp32();
						CVec3Dfp32 IT = CInertia::Block(M, Size[0], Size[1],  Size[2]).Getfp32();
						CVec3Dfp32 IT_Translated(0.0f);

						CVec3Dfp32 C = (BBox.m_Max + BBox.m_Min) * 0.5f;
						CVec3Dfp32 TSQ = (_Result.m_CenterOfMass - C);

						for (int q = 0; q < 3; q++)
							TSQ.k[q] = TSQ.k[q] * TSQ.k[q];

						IT_Translated.k[0] = IT.k[0] + M * (TSQ.k[1] + TSQ.k[2]);
						IT_Translated.k[1] = IT.k[1] + M * (TSQ.k[0] + TSQ.k[2]);
						IT_Translated.k[2] = IT.k[2] + M * (TSQ.k[0] + TSQ.k[1]);

						_Result.m_InertiaTensor += IT_Translated;
					}
				}
				// all ok!
				return true;
			}
			// error!
			bRet = false;
		}	
	}

	// fallback using boundbox...
	CVec3Dfp32 Size = BBox.m_Max - BBox.m_Min;
	fp32 Volume = (Size.k[0] * Size.k[1] * Size.k[2]);
	_Result.m_Volume = Volume;
	_Result.m_Mass = Volume * 1.0f  * (1000.0f / 32768.0f);
	_Result.m_CenterOfMass = (BBox.m_Min + BBox.m_Max) * 0.5f;//Prim.GetOffset();
	_Result.m_InertiaTensor = CInertia::Block(_Result.m_Mass, Size.k[0], Size.k[1], Size.k[2]).Getfp32();

	return bRet;
}


/*
class SolidPolyhedron // : public Polyhedron
{
public:
	SolidPolyhedron(CXR_IndexedSolidContainer32 *_IndexedSolidContainer, int _index,  const CMat4Dfp32& _Trans)
	{
		m_pIndexedSolids = _IndexedSolidContainer;
		m_Trans = _Trans;
		m_SolidIndex = _index;
		m_pIndexedSolids->GetSolid(m_SolidIndex, m_SolidDescr);

		const CXR_IndexedSolid32& solid = m_pIndexedSolids->m_lSolids[m_SolidIndex];
		m_nVertices = solid.m_nVertices;

		int n = m_pIndexedSolids->m_lSolids.Len();
	}

	SolidPolyhedron(CXR_IndexedSolidContainer32 *_IndexedSolidContainer, int _index, const CMat4Dfp32& _Trans, const CVec3Dfp32& _Offset)
	{
		m_pIndexedSolids = _IndexedSolidContainer;
		m_Trans = _Trans;
		m_Trans.GetRow(3) = _Offset * _Trans;
		m_SolidIndex = _index;
		m_pIndexedSolids->GetSolid(m_SolidIndex, m_SolidDescr);

		const CXR_IndexedSolid32& solid = m_pIndexedSolids->m_lSolids[m_SolidIndex];
		m_nVertices = solid.m_nVertices;

		int n = m_pIndexedSolids->m_lSolids.Len();
	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P, const CMat4Dfp32& _Trans) const
	{
		M_ASSERT(GetNumberOfVertices() > 0, "");

		CVec3Dfp32 Vertex = GetVertex(0);
		Vertex = Vertex * _Trans;
		fp32 d = _P * Vertex;
		int iSupport = 0;
		for (int i = 1; i < GetNumberOfVertices(); i++)
		{
			Vertex = GetVertex(i);
			Vertex = Vertex * _Trans;
			fp32 tmp = _P * Vertex;
			if (tmp > d)
			{
				d = tmp;
				iSupport = i;
			}
		}
		//CVec3Dfp32 Ret;
		//Ret *= _Trans;
		return GetVertex(iSupport) * _Trans;
	}

	M_INLINE CVec3Dfp32 Support(const CVec3Dfp32& _P) const
	{
		M_ASSERT(GetNumberOfVertices() > 0, "");

		fp32 d = _P * GetVertex(0);
		int iSupport = 0;
		for (int i = 1; i < GetNumberOfVertices(); i++)
		{
			fp32 tmp = _P * GetVertex(i);
			if (tmp > d)
			{
				d = tmp;
				iSupport = i;
			}
		}
		//CVec3Dfp32 Ret;
		//Ret *= _Trans;
		return GetVertex(iSupport);
	}


	M_INLINE CVec3Dfp32 GetVertex(int index) const
	{
		//const CXR_IndexedSolid32& solid = m_spIndexedSolids->m_lSolids[m_SolidIndex];
		//CXR_IndexedSolidDesc32 soliddesr;
		//m_pIndexedSolids->GetSolid(m_SolidIndex, soliddesr);

		//int nVert = solid.m_nVertices;
//		CVec3Dfp32 ret = soliddesr.m_pV[soliddesr.m_piV[index]];
		CVec3Dfp32 ret = m_SolidDescr.m_pV[index];
		return ret * m_Trans;
	}

	M_INLINE int GetNumberOfVertices() const
	{
		return m_nVertices;
		//const CXR_IndexedSolid32& solid = m_pIndexedSolids->m_lSolids[m_SolidIndex];
		//return solid.m_nVertices;
	}

	void Dump()
	{
		const CXR_IndexedSolid32& solid = m_pIndexedSolids->m_lSolids[m_SolidIndex];
		CXR_IndexedSolidDesc32 soliddesr;
		m_pIndexedSolids->GetSolid(m_SolidIndex, soliddesr);

		int nVert = solid.m_nVertices;
		M_TRACEALWAYS("%d\n",nVert);
		for (int i = 0; i < nVert; i++)
		{
			M_TRACEALWAYS("%s %d\n",soliddesr.m_pV[i].GetString().Str(), soliddesr.m_piV[i]);
		}

	}

	void Render(CWorld_PhysState *pPhysState)
	{
		CMat4Dfp32 m;
		m.Unit();
		int nVerts = GetNumberOfVertices();
		for (int i = 0; i < nVerts; i++)
		{
			CVec3Dfp32::GetRow(m,3) = GetVertex(i);
			pPhysState->Debug_RenderVertex(m.GetRow(3), CPixel32(220,0,0), 1.0/10.0, false);
		}
		//Dump();
	}

	CXR_IndexedSolidContainer32 *m_pIndexedSolids;
	CXR_IndexedSolidDesc32 m_SolidDescr;
	CMat4Dfp32 m_Trans;
	int m_SolidIndex;
	int m_nVertices;
};

*/


/*
	TODO: Denna ska bort sedan...!!!
 */

M_INLINE static CVec3Dfp64 ConvertVector(const CVec3Dfp32& _Vec)
{
	CVec3Dfp64 ret;
	ret.k[0] = _Vec.k[0];
	ret.k[1] = _Vec.k[1];
	ret.k[2] = _Vec.k[2];
	return ret;
}

M_INLINE static CVec3Dfp32 ConvertVector(const CVec3Dfp64& _Vec)
{
	CVec3Dfp32 ret;
	ret.k[0] = _Vec.k[0];
	ret.k[1] = _Vec.k[1];
	ret.k[2] = _Vec.k[2];
	return ret;
}

M_INLINE static CMat4Dfp32 ConvertMatrix(const CMat4Dfp64& _Mat)
{
	CMat4Dfp32 ret;
	ret.Unit();
	for (int i=0; i<4; i++)
		for (int j=0; j<4; j++)
			ret.k[i][j] = _Mat.k[i][j];
	return ret;
}

M_INLINE static CMat4Dfp32 ConvertMatrix2(const CMat4Dfp64& _Mat)
{
	CMat4Dfp32 ret;
	ret.Unit();
	for (int i=0; i<4; i++)
		for (int j=0; j<4; j++)
			ret.k[i][j] = _Mat.k[i][j];
	return ret;
}

#ifdef USE_DYNAMICSENGINE1


void CWD_RigidBody::GetBoundBox(CMapData *_pWorldData, const CWObject_CoreData *_pObj, CBox3Dfp32& _Box)
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

#if 0
static void InspectSolidData(CMapData *_pWorldData, const CWObject_CoreData *_pObj)
{
	const CWO_PhysicsPrim& Prim = _pObj->GetPhysState().m_Prim[0];
	CVec3Dfp32 Offset = Prim.GetOffset();

	if (Prim.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL)
	{
		CXR_Model* pModel = _pWorldData->GetResource_Model(Prim.m_iPhysModel);
//		CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();

		int ModelClass = pModel->GetModelClass();
		if (ModelClass == CXR_MODEL_CLASS_BSP2)
		{
			CXR_Model_BSP2 *pModelBSP2 = (CXR_Model_BSP2 *) pModel;
			spCXR_IndexedSolidContainer32 spIndexedSolids = pModelBSP2->m_spIndexedSolids;

			for (int i = 0; i < spIndexedSolids->m_lSolids.Len(); i++)
			{
				const CXR_IndexedSolid32& solid = spIndexedSolids->m_lSolids[i];
				CXR_IndexedSolidDesc32 soliddesr;
				spIndexedSolids->GetSolid(i, soliddesr);
				
//				int nVert = solid.m_nVertices;
				for (int j = 0; j < solid.m_nVertices; j++)
				{
//					CVec3Dfp32 vert = soliddesr.m_pV[soliddesr.m_piV[j]];
					CVec3Dfp32 vert = soliddesr.m_pV[j];
//					int breakme = 0;
				}
				
			}

			

		}
	}
}
#endif


CWD_RigidBody::CWD_RigidBody(CWorld *_pWorld, CWObject_CoreData *_pCoreData) : CRigidBody(_pWorld)
{
	//m_pObject = NULL;
	m_pCoreData = _pCoreData;
}



CWO_DynamicsCollider *CWO_DynamicsCollider::m_pInstance = NULL;

CWO_DynamicsCollider::CWO_DynamicsCollider()
{
	m_pInstance = NULL;
	m_lBouyancyObjects.QuickSetLen(20);
}

void CWO_DynamicsCollider::Init(CMapData* _pMapData)
{
}

CWO_DynamicsCollider *CWO_DynamicsCollider::GetInstance()
{
	if (m_pInstance == NULL)
	{
		m_pInstance = DNew(CWO_DynamicsCollider) CWO_DynamicsCollider();
	}
	return m_pInstance;
}

bool CWO_DynamicsCollider::Collide(const CWorld *_pWorld, 
								   CRigidBody *_pBody,
								   CContactInfo *_pContactInfo,
								   void *_pArgument1,
								   void *_pArgument2)
{
	/*
		TODO: Inte helt optimalt men funkar så länge.
	 */

	TArray<CRigidBody *> List;
	List.Add(_pBody);

	int nColl = Collide(_pWorld, List, _pContactInfo, 1, _pArgument1, _pArgument2);
	return nColl > 0;
}

int CWO_DynamicsCollider::Collide(const CWD_RigidBody *_pBody1, 
								  const CWD_RigidBody *_pBody2, 
								  CContactInfo *_pContactInfo, 
								  int _MaxCollisions,
								  CMapData *_pWorldData)
{
	MSCOPESHORT(CWO_DynamicsCollider::Collide_1);
	M_ASSERT(_pWorldData != NULL, "");

	const CWObject_CoreData *pObj1 = _pBody1->m_pCoreData;
	const CWObject_CoreData *pObj2 = _pBody2->m_pCoreData;

	const CWO_PhysicsPrim& Prim1 = pObj1->GetPhysState().m_Prim[0];

	CXR_Model* pModel1 = _pWorldData->GetResource_Model(Prim1.m_iPhysModel);
	M_ASSERT( pModel1, "" );

	CXR_PhysicsModel* pPhysModel1 = pModel1->Phys_GetInterface();
	M_ASSERT( pPhysModel1, "" );

	CBox3Dfp32 Box;
	CWD_RigidBody::GetBoundBox(_pWorldData, pObj2, Box);
	CPhysOBB Obb;

#if 0
	CMat43fp64 M1 = _pBody1->GetOrientationMatrix();
	CMat43fp64 M2 = _pBody2->GetOrientationMatrix();

	CBox3Dfp32 box1, box2;
	pPhysModel1->GetBound_Box(box1);
	pPhysModel2->GetBound_Box(box2);

	int nTotColl = FooCollide(_pBody1, _pBody2, 
							  ConvertVector(box1.m_Max - box1.m_Min),
							  ConvertVector(box2.m_Max - box2.m_Min),
							  0, 0, _pContactInfo);
	for (int i = 0; i < nTotColl; i++)
	{
		CContactInfo& ci = _pContactInfo[i];
		ci.m_pRigidBody1 = (CRigidBody *) _pBody1;
		ci.m_pRigidBody2 = (CRigidBody *) _pBody2;
		ci.m_isColliding = true;
	}

	return nTotColl;

#else
	CMat43fp64 tmp;
	tmp = _pBody1->GetOrientationMatrix();
	CVec3Dfp64::GetRow(tmp,3) = _pBody1->GetPosition();
	CMat4Dfp32 P1 = ConvertMatrix(tmp);

	tmp = _pBody2->GetOrientationMatrix();
	CVec3Dfp64::GetRow(tmp,3) = _pBody2->GetPosition();
	CMat4Dfp32 P2 = ConvertMatrix(tmp);
	

	CVec3Dfp32 Extent = Box.m_Max - Box.m_Min;
	Obb.SetPosition(P2);
	Obb.SetDimensions(Extent);

	CCollisionInfo CollisionInfo[10];
	CXR_PhysicsContext Context(P1);
	int nCollisions = pPhysModel1->Phys_CollideBox(&Context, Obb, 0, CollisionInfo, 10);

	int nTotCol = 0;
	for (int i = 0; i < nCollisions && i < _MaxCollisions; i++)
	{
		nTotCol++;
		_pContactInfo[i].m_pRigidBody1 = (CRigidBody *) _pBody1;
		_pContactInfo[i].m_pRigidBody2 = (CRigidBody *) _pBody2;
		_pContactInfo[i].m_Normal = ConvertVector(CollisionInfo[i].m_Plane.n);
		_pContactInfo[i].m_PointOfCollision = ConvertVector(CollisionInfo[i].m_Pos);
		_pContactInfo[i].m_distance = CollisionInfo[i].m_Distance;
		_pContactInfo[i].m_isColliding = CollisionInfo[i].m_bIsCollision;

//		M_TRACEALWAYS("%s\n",_pContactInfo[i].m_Normal.GetString().Str());
//		M_TRACEALWAYS("%s\n",CollisionInfo[i].m_Plane.n.GetString().Str());

		
	}
	return nTotCol;
#endif

}

int CWO_DynamicsCollider::CollideAsBoxes(const CWD_RigidBody *_pBody1, 
										 const CWD_RigidBody *_pBody2, 
										 CContactInfo *_pContactInfo, 
										 int _MaxCollisions,
										 CMapData *_pWorldData)
{
	MSCOPESHORT(CWO_DynamicsCollider::CollideAsBoxes);

	M_ASSERT(_pWorldData != NULL, "");

	const CWObject_CoreData* pObj1 = _pBody1->m_pCoreData;
	const CWObject_CoreData* pObj2 = _pBody2->m_pCoreData;

	CBox3Dfp32 Box1,Box2;
	CWD_RigidBody::GetBoundBox(_pWorldData, pObj1, Box1);
	CWD_RigidBody::GetBoundBox(_pWorldData, pObj2, Box2);

	CMat43fp64 tmp;
	tmp = _pBody1->GetOrientationMatrix();
	tmp.GetRow(3) = _pBody1->GetPosition();
	CMat4Dfp32 P1 = ConvertMatrix(tmp);

	tmp = _pBody2->GetOrientationMatrix();
	tmp.GetRow(3) = _pBody2->GetPosition();
	CMat4Dfp32 P2 = ConvertMatrix(tmp);

	CPhysOBB Obb1;
	CVec3Dfp32 Extent1 = Box1.m_Max - Box1.m_Min;
	Obb1.SetPosition(P1);
	Obb1.SetDimensions(Extent1);

	CPhysOBB Obb2;
	CVec3Dfp32 Extent2 = Box2.m_Max - Box2.m_Min;
	Obb2.SetPosition(P2);
	Obb2.SetDimensions(Extent2);

	CCollisionInfo CollisionInfo[10];
	int nCollisions = Phys_Collide_OBB(Obb1,Obb2,CollisionInfo,10);

	int nTotCol = 0;
	for (int i = 0; i < nCollisions && i < _MaxCollisions; i++)
	{
		nTotCol++;
		_pContactInfo[i].m_pRigidBody1 = (CRigidBody *) _pBody1;
		_pContactInfo[i].m_pRigidBody2 = (CRigidBody *) _pBody2;
		_pContactInfo[i].m_Normal = ConvertVector(CollisionInfo[i].m_Plane.n);
		_pContactInfo[i].m_PointOfCollision = ConvertVector(CollisionInfo[i].m_Pos);
		_pContactInfo[i].m_distance = CollisionInfo[i].m_Distance;
		_pContactInfo[i].m_isColliding = CollisionInfo[i].m_bIsCollision;
	}
	return nTotCol;
}


int CWO_DynamicsCollider::Collide3(const CWD_RigidBody *_pBody1, 
								   const CWObject_CoreData *_pObj2,
								   CContactInfo *_pContactInfo, 
								   int _MaxCollisions,
								   CMapData *_pWorldData,
								   CWorld_PhysState *_pPhysState)
{
	MSCOPESHORT(CWO_DynamicsCollider::Collide3);
	M_ASSERT(_pWorldData != NULL, "");

	const CWObject_CoreData *pObj1 = _pBody1->m_pCoreData;
	//const CWObject_CoreData *pObj2 = _pBody2->m_pCoreData;

	CBox3Dfp32 Box1, Box2;
	CWD_RigidBody::GetBoundBox(_pWorldData, pObj1, Box1);
	CWD_RigidBody::GetBoundBox(_pWorldData, _pObj2, Box2);
	//pPhysModel2->GetBound_Box(Box);
	CPhysOBB Obb1,Obb2;

	CMat43fp64 tmp;
	tmp = _pBody1->GetOrientationMatrix();
	CVec3Dfp64::GetRow(tmp,3) = _pBody1->GetPosition();
	CMat4Dfp32 P1 = ConvertMatrix(tmp);

	CMat4Dfp32 P2 = _pObj2->GetPositionMatrix();
	/*
	tmp = _pBody2->GetOrientationMatrix();
	CVec3Dfp64::GetRow(tmp,3) = _pBody2->GetPosition();
	CMat4Dfp32 P2 = ConvertMatrix(tmp);
	*/

	Obb1.SetPosition(P1);
	CVec3Dfp32 Extent1 = Box1.m_Max - Box1.m_Min;
	//Extent1 += CVec3Dfp32(0.001f);
	Obb1.SetDimensions(Extent1);

	P2.Unit3x3();
	Obb2.SetPosition(P2);
	CVec3Dfp32 Extent2 = Box2.m_Max - Box2.m_Min;
//	Extent2 += CVec3Dfp32(0.001f);
	//Extent2[2]= 0.01f;
	Obb2.m_C[2] += 28;
	Obb2.SetDimensions(Extent2);

	CCollisionInfo CollisionInfo[10];
	CXR_PhysicsContext Context(P1);
	//int nCollisions = pPhysModel1->Phys_CollideBox(&Context, Obb2, 0, CollisionInfo, 10);

	int nCollisions = Phys_Collide_OBB(Obb1,Obb2,CollisionInfo,10);


	int nTotCol = 0;
	for (int i = 0; i < nCollisions && i < _MaxCollisions; i++)
	{
		CVec3Dfp64 n = ConvertVector(CollisionInfo[i].m_Plane.n);
		fp64 d = CollisionInfo[i].m_Distance;

		/*
		if (n * CVec3Dfp64(0,0,-1) > 0.98f)
		{
			int breakme = 1;
		}
		*/

		_pContactInfo[nTotCol].m_pRigidBody1 = (CRigidBody *) _pBody1;
		_pContactInfo[nTotCol].m_pRigidBody2 = NULL;
		_pContactInfo[nTotCol].m_Normal = n;
		_pContactInfo[nTotCol].m_PointOfCollision = ConvertVector(CollisionInfo[i].m_Pos);
		_pContactInfo[nTotCol].m_distance = d;
		_pContactInfo[nTotCol].m_isColliding = CollisionInfo[i].m_bIsCollision;
		_pContactInfo[nTotCol].m_UserData1 = pObj1->m_iObject;
		_pContactInfo[nTotCol].m_UserData2 = _pObj2->m_iObject;
		nTotCol++;

		//		M_TRACEALWAYS("%s\n",_pContactInfo[i].m_Normal.GetString().Str());
		//		M_TRACEALWAYS("%s\n",CollisionInfo[i].m_Plane.n.GetString().Str());
	}
	return nTotCol;

}

int CWO_DynamicsCollider::Collide2(const CWD_RigidBody *_pBody1, 
								   const CWObject_CoreData *pObj2, 
								   CContactInfo *_pContactInfo, 
								   int _MaxCollisions,
								   CMapData *_pWorldData)
{
	MSCOPESHORT(CWO_DynamicsCollider::Collide2);

	M_ASSERT(_pWorldData != NULL, "");

	const CWObject_CoreData *pObj1 = _pBody1->m_pCoreData;
	//const CWObject_CoreData *pObj2 = _pBody2->m_pCoreData;

	CBox3Dfp32 Box;
	CWD_RigidBody::GetBoundBox(_pWorldData, pObj1, Box);

	const CWO_PhysicsPrim& Prim2 = pObj2->GetPhysState().m_Prim[0];
	CXR_Model* pModel2 = _pWorldData->GetResource_Model(Prim2.m_iPhysModel);
	M_ASSERT( pModel2, "" );

	CXR_PhysicsModel* pPhysModel2 = pModel2->Phys_GetInterface();
	M_ASSERT( pPhysModel2, "" );

	CPhysOBB Obb;

	CMat43fp64 tmp;
	tmp = _pBody1->GetOrientationMatrix();
	CVec3Dfp64::GetRow(tmp,3) = _pBody1->GetPosition();
	CMat4Dfp32 P1 = ConvertMatrix(tmp);

	//tmp = _pBody2->GetOrientationMatrix();
	//CVec3Dfp64::GetRow(tmp,3) = _pBody2->GetPosition();
	CMat4Dfp32 P2;
	P2.Unit();
	//= ConvertMatrix(tmp);

	CVec3Dfp32 Extent = Box.m_Max - Box.m_Min;
	//Extent *= 0.5;
	Obb.SetPosition(P1);
	Obb.SetDimensions(Extent);

	// TODO: Hårdkodat och testas ej...
	CCollisionInfo CollisionInfo[100];
	CXR_PhysicsContext Context(P2);
//	int nCollisions = pPhysModel1->Phys_CollideBox(&Context, Obb, 0, CollisionInfo, 10);

	int nCollisions = pPhysModel2->Phys_CollideBox(&Context, Obb, pObj1->GetPhysState().m_MediumFlags, CollisionInfo, 100);
	/*bool collide  = pPhysModel2->Phys_IntersectBox(&Context, Obb, Obb, pObj1->GetPhysState().m_MediumFlags, CollisionInfo);
	int nCollisions = 0;
	if (collide) 
	{
		nCollisions++;
		CollisionInfo[0].Clear();
		pPhysModel2->Phys_IntersectBox(&Context, Obb, Obb, pObj1->GetPhysState().m_MediumFlags, CollisionInfo);
	}*/
	//int nCollisions = pPhysModel1->Phys_CollideBox(&Context, Obb, 0, CollisionInfo, 10);

	int nTotCol = 0;
	for (int i = 0; i < 100 && i < nCollisions && i < _MaxCollisions; i++)
	{
		nTotCol++;
		_pContactInfo[i].m_pRigidBody1 = (CRigidBody *) _pBody1;
		_pContactInfo[i].m_pRigidBody2 = NULL;
		_pContactInfo[i].m_Normal = -ConvertVector(CollisionInfo[i].m_Plane.n);
		_pContactInfo[i].m_PointOfCollision = ConvertVector(CollisionInfo[i].m_Pos);
		_pContactInfo[i].m_distance = fabs(CollisionInfo[i].m_Distance);
		//_pContactInfo[i].m_distance = 0;
		_pContactInfo[i].m_isColliding = CollisionInfo[i].m_bIsCollision;

		//		M_TRACEALWAYS("%s\n",_pContactInfo[i].m_Normal.GetString().Str());
		//		M_TRACEALWAYS("%s\n",CollisionInfo[i].m_Plane.n.GetString().Str());


	}
	return nTotCol;
}


int CWO_DynamicsCollider::CollideSolidBsp(const CWD_RigidBody *_pBody1, 
										  CXR_Model_BSP2 *_pBSP2,
										  const CWObject_CoreData *pObj2, 
										  CContactInfo *_pContactInfo, 
										  int _MaxCollisions,
										  CMapData *_pWorldData,
										  CWorld_PhysState *_pPhysState)
{
	MSCOPESHORT(CWO_DynamicsCollider::CollideSolidBsp);

	M_ASSERT(_pWorldData != NULL, "");

	const CWObject_CoreData *pObj1 = _pBody1->m_pCoreData;

	CBox3Dfp32 Box;
	CWD_RigidBody::GetBoundBox(_pWorldData, pObj1, Box);

	const CWO_PhysicsPrim& Prim1 = _pBody1->m_pCoreData->GetPhysState().m_Prim[0];

	const CWO_PhysicsPrim& Prim2 = pObj2->GetPhysState().m_Prim[0];
	CXR_Model* pModel2 = _pWorldData->GetResource_Model(Prim2.m_iPhysModel);
	M_ASSERT( pModel2, "" );

	CXR_PhysicsModel* pPhysModel2 = pModel2->Phys_GetInterface();
	M_ASSERT( pPhysModel2, "" );

	CMat43fp64 tmp;
	tmp = _pBody1->GetOrientationMatrix();
	CVec3Dfp64::GetRow(tmp,3) = _pBody1->GetPosition();
	CMat4Dfp32 P1 = ConvertMatrix(tmp);
	P1.Transpose3x3();

	const CMat4Dfp32 &BspTransform = pObj2->GetPositionMatrix();

	CCollisionInfo CollisionInfo[100];
	CXR_PhysicsContext Context(BspTransform);
	Context.m_PhysGroupMaskThis = Prim2.m_PhysModelMask;
	Context.m_PhysGroupMaskCollider = Prim1.m_PhysModelMask;
	Context.m_pWC = _pPhysState->Debug_GetWireContainer();

	CVec3Dfp32 CenterOfMass(_pBody1->m_MassCenter[0], _pBody1->m_MassCenter[1], _pBody1->m_MassCenter[2]);

	int nCollisions = pPhysModel2->Phys_CollideBSP2(&Context, _pBSP2, P1, Prim1.GetOffset() - CenterOfMass, pObj1->GetPhysState().m_MediumFlags, CollisionInfo, 100);

	int nTotCol = 0;
	for (int i = 0; i < 100 && i < nCollisions && i < _MaxCollisions; i++)
	{
		_pContactInfo[nTotCol].m_pRigidBody1 = (CRigidBody *) _pBody1;
		_pContactInfo[nTotCol].m_pRigidBody2 = NULL;
		_pContactInfo[nTotCol].m_Normal = -ConvertVector(CollisionInfo[i].m_Plane.n);
		_pContactInfo[nTotCol].m_PointOfCollision = ConvertVector(CollisionInfo[i].m_Pos);
		_pContactInfo[nTotCol].m_distance = fabs(CollisionInfo[i].m_Distance);
		_pContactInfo[nTotCol].m_isColliding = CollisionInfo[i].m_bIsCollision;
		_pContactInfo[nTotCol].m_UserData1 = pObj1->m_iObject;
		_pContactInfo[nTotCol].m_UserData2 = pObj2->m_iObject;

		//M_TRACEALWAYS("PointOfCollision = %s\n",_pContactInfo[nTotCol].m_PointOfCollision.GetString().Str());
		//M_TRACEALWAYS("Normal = %s\n",_pContactInfo[nTotCol].m_Normal.GetString().Str());

		if (IsValidVec3(_pContactInfo[nTotCol].m_Normal))
		{
			nTotCol++;
		}
		else
		{
			int breakme = 0;
		}
	}
	return nTotCol;
}

void CWO_DynamicsCollider::PreCollide(CWorld *_pWorld)
{

}

#if 0
// Dummy O(n^2) version
int CWO_DynamicsCollider::Collide(const CWorld *_pWorld, 
								  const TArray<CRigidBody *>& _BodyList,
								  CContactInfo *_pCollisionInfo, 
								  int _MaxCollisions,
								  void *_pArgument1,
								  void *_pArgument2)
{
	//const TArray<CRigidBody *>& BodyList = _pWorld->GetRigidBodyList();
	int nBodies = _BodyList.Len();
	int nTotCollisions = 0;

	CMapData *pWorldData = (CMapData *) _pArgument1;
	CWorld_PhysState *pPhysState = (CWorld_PhysState *) _pArgument2;

	for (int i = 0; i < nBodies; i++)
	{
		for (int j = i+1; j < nBodies; j++)
		{
			const CRigidBody *pBody1 = _BodyList[i];
			const CRigidBody *pBody2 = _BodyList[j];
			if (!(!pBody1->IsActive() && !pBody2->IsActive()))
			{
				int nColl = Collide((CWD_RigidBody *) pBody1, 
					(CWD_RigidBody *) pBody2, 
					&_pCollisionInfo[nTotCollisions], _MaxCollisions - nTotCollisions,
					pWorldData);
				nTotCollisions += nColl;
			}
		}
	}

	return nTotCollisions;
}
#else
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

static CCollisionHash s_TestColl;

#if 0
static void FindClosestPlane(const CXR_IndexedSolidDesc32& _SolidDesc, const CPlane3Dfp32& _Plane1)
{
	for (int i = 0; i < _SolidDesc.m_pF.Len(); i++)
	{
		const CXR_IndexedSolidFace32& Face = _SolidDesc.m_pF[i];
		for (int j = 0; j < Face.m_nV; j++)
		{
			CVec3Dfp32 v = _SolidDesc.m_pV[_SolidDesc.m_piV[Face.m_iiV + j]];
//			int foobar = 1;
		}
	}

}
#endif

static void SetupCollision(const CWObject_CoreData *_pObject1,
						   const CWObject_CoreData *_pObject2,
						   CMapData *_pWorldData,
						   CMat4Dfp32& _Transform1, 
						   CMat4Dfp32& _Transform2, 
						   CXR_PhysicsContext& _Context)
{
	const CWO_PhysicsPrim& PhysicsPrim1 = _pObject1->GetPhysState().m_Prim[0];
	const CWO_PhysicsPrim& PhysicsPrim2 = _pObject2->GetPhysState().m_Prim[0];

	CVec3Dfp32 CenterOfMass1(0.0f);
	CVec3Dfp32 CenterOfMass2(0.0f);
	CMat4Dfp64 tmp;

	// Get transform
	CMat4Dfp32 Transform1, Transform2;
	if (_pObject1->m_pRigidBody)
	{
		_pObject1->m_pRigidBody->GetCorrectTransform(tmp);
		Transform1 = ConvertMatrix(tmp);
		CenterOfMass1 = _pObject1->m_pRigidBody->m_MassCenter.Getfp32();
	}
	else
	{
		Transform1 = _pObject1->GetPositionMatrix();
	}

	if (_pObject2->m_pRigidBody)
	{
		_pObject2->m_pRigidBody->GetCorrectTransform(tmp);
		Transform2 = ConvertMatrix(tmp);
		CenterOfMass2 = _pObject2->m_pRigidBody->m_MassCenter.Getfp32();
	}
	else
	{
		Transform2 = _pObject2->GetPositionMatrix();
	}

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

static const CXR_PhysicsModel *GetPhysicsModel(const CWObject_CoreData *_pObject, CMapData *_pWorldData)
{
	const CWO_PhysicsPrim& Prim = _pObject->GetPhysState().m_Prim[0];
	CXR_Model* pModel = _pWorldData->GetResource_Model(Prim.m_iPhysModel);
	if (pModel == NULL) return NULL;

	return pModel->Phys_GetInterface();
}

static CXR_Model *GetModel(const CWObject_CoreData *_pObject, CMapData *_pWorldData)
{
	const CWO_PhysicsPrim& Prim = _pObject->GetPhysState().m_Prim[0];
	return _pWorldData->GetResource_Model(Prim.m_iPhysModel);
}


static void TransposeOBB(CPhysOBB& _OBB)
{
	CMat4Dfp32 M;
	M.GetRow(0) = _OBB.m_A[0];
	M.GetRow(1) = _OBB.m_A[1];
	M.GetRow(2) = _OBB.m_A[2];

	M.Transpose3x3();
	_OBB.m_A[0] = M.GetRow(0);
	_OBB.m_A[1] = M.GetRow(1);
	_OBB.m_A[2] = M.GetRow(2);
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
		_pContactInfo[i].m_PointOfCollision = _pCollisionInfo[i].m_Pos.Getfp64();
		_pContactInfo[i].m_distance = _pCollisionInfo[i].m_Distance;
		_pContactInfo[i].m_isColliding = _pCollisionInfo[i].m_bIsCollision;
		_pContactInfo[i].m_UserData1 = _pObject1->m_iObject;
		_pContactInfo[i].m_UserData2 = _pObject2->m_iObject;
	}
	return i;
}

static int CollideSolidBspNew(const CWObject_CoreData *_pObject1,
							  const CWObject_CoreData *_pObject2,
							  CMapData *_pWorldData,
							  CWorld_PhysState *_pPhysState,
							  CContactInfo *_pContactInfo, 
							  int _MaxCollisions)
{
	MSCOPESHORT(CWO_DynamicsCollider::CollideSolidBspNew);

	CXR_PhysicsContext Context;

	CMat4Dfp32 Transform1, Transform2;
	SetupCollision(_pObject1, _pObject2, _pWorldData, Transform1, Transform2, Context);
	Context.m_pWC = _pPhysState->Debug_GetWireContainer();

	CXR_Model *pModel1 = GetModel(_pObject1, _pWorldData);
	CXR_Model *pModel2 = GetModel(_pObject2, _pWorldData);

	CBox3Dfp32 Box;
	CWD_RigidBody::GetBoundBox(_pWorldData, _pObject1, Box);
	CPhysOBB OBB;
	CVec3Dfp32 Extent = Box.m_Max - Box.m_Min;

	// TODO: Detta är helt crazy. Den övre 3x3 är rätt här
	// men Phys_CollideBox förväntar sig en transponerad...
	Transform1.Transpose3x3();
	OBB.SetPosition(Transform1);
	OBB.SetDimensions(Extent);


	const int MaxObjectCollisions = 100;
	CCollisionInfo CollisionInfo[MaxObjectCollisions];
/*	int nCollisions = pModel2->Phys_GetInterface()->Phys_CollideBSP2(&Context, safe_cast<CXR_Model_BSP2>(pModel1) , Transform1, 
																	 CVec3Dfp32(0.0f), _pObject1->GetPhysState().m_MediumFlags, 
																	 CollisionInfo, MaxObjectCollisions);

*/
	int nCollisions = pModel2->Phys_GetInterface()->Phys_CollideBox(&Context, OBB, _pObject1->GetPhysState().m_MediumFlags, CollisionInfo, MaxObjectCollisions);

	// TODO: REMOVE
	if (nCollisions > 30)
	{
		int breakme = 0;
	}


	M_ASSERT(nCollisions <= _MaxCollisions, "!");

	int nTotCol = 0;
	for (int i = 0; i < MaxObjectCollisions && i < nCollisions && i < _MaxCollisions; i++)
	{
		_pContactInfo[nTotCol].m_pRigidBody1 = (CRigidBody *) _pObject1->m_pRigidBody;
		_pContactInfo[nTotCol].m_pRigidBody2 = NULL;
		_pContactInfo[nTotCol].m_Normal = -ConvertVector(CollisionInfo[i].m_Plane.n);
		_pContactInfo[nTotCol].m_PointOfCollision = ConvertVector(CollisionInfo[i].m_Pos);
		_pContactInfo[nTotCol].m_distance = fabs(CollisionInfo[i].m_Distance);
		_pContactInfo[nTotCol].m_isColliding = CollisionInfo[i].m_bIsCollision;
		_pContactInfo[nTotCol].m_UserData1 = _pObject1->m_iObject;
		_pContactInfo[nTotCol].m_UserData2 = _pObject2->m_iObject;

		if (IsValidVec3(_pContactInfo[nTotCol].m_Normal))
		{
			nTotCol++;
		}
		else
		{
			int breakme = 0;
		}
	}
	return nTotCol;
}

static int CollideSolidSolid(const CWObject_CoreData *_pObject1,
							 const CWObject_CoreData *_pObject2,
							 CMapData *_pWorldData,
							 CContactInfo *_pContactInfo,
							 int _MaxCollisions)
{
	CXR_PhysicsContext Context;

	CMat4Dfp32 Transform1, Transform2;
	SetupCollision(_pObject1, _pObject2, _pWorldData, Transform1, Transform2, Context);

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
	CCollisionInfo CollisionInfo[MaxCollisions];
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

			if (!(MediumDesc1.m_MediumFlags & XW_MEDIUM_SOLID && MediumDesc2.m_MediumFlags && XW_MEDIUM_SOLID)) continue;
			if (!(PhysicsPrim1.m_PhysModelMask & M_Bit(MediumDesc1.m_iPhysGroup))) continue;
			if (!(PhysicsPrim2.m_PhysModelMask & M_Bit(MediumDesc2.m_iPhysGroup))) continue;

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

				iCollision += CopyCollisionInfo(nC, CollisionInfo, &_pContactInfo[iCollision], _MaxCollisions,
												(CRigidBody *) _pObject1->m_pRigidBody,
												(CRigidBody *) _pObject2->m_pRigidBody,
												_pObject1, _pObject2);

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

				{
					CSolidPolyhedronSeparating TempSolid1(&SolidDescr1, pIndexedSolids1->m_lSolids[is1].m_nVertices);
					CSolidPolyhedronSeparating TempSolid2(&SolidDescr2, pIndexedSolids2->m_lSolids[is2].m_nVertices);
					nC = TPolyhedraPolyhedraCollider<CSolidPolyhedronSeparating, CSolidPolyhedronSeparating, fp32>
									::Collide(&TempSolid1, Transform1,
											&TempSolid2, Transform2, CenterAxis, PointOfCollisions, Normals, Depths);
				}

				for (int i = 0; i < nC; i++, iCollision++)
				{
					_pContactInfo[iCollision].m_pRigidBody1 = (CRigidBody *) _pObject1->m_pRigidBody;
					_pContactInfo[iCollision].m_pRigidBody2 = (CRigidBody *) _pObject2->m_pRigidBody;
					_pContactInfo[iCollision].m_Normal = Normals[i].Getfp64();
					_pContactInfo[iCollision].m_PointOfCollision = PointOfCollisions[i].Getfp64();
					_pContactInfo[iCollision].m_distance = Depths[i];
					_pContactInfo[iCollision].m_isColliding = true;
					_pContactInfo[iCollision].m_UserData1 = _pObject1->m_iObject;
					_pContactInfo[iCollision].m_UserData2 = _pObject2->m_iObject;
				}
			}
		}
	}
	M_ASSERT(iCollision <= _MaxCollisions, "!");
	return iCollision;
}


static int GenericCollide(const CWD_RigidBody *_pBody1, 
						  CXR_Model_BSP2 *_pBSP2,
						  const CWObject_CoreData *pObj2, 
						  CContactInfo *_pContactInfo, 
						  int _MaxCollisions,
						  CMapData *_pWorldData,
						  CWorld_PhysState *_pPhysState)
{
	M_ASSERT(_pWorldData != NULL, "");

	const CWObject_CoreData *pObj1 = _pBody1->m_pCoreData;

	CBox3Dfp32 Box;
	CWD_RigidBody::GetBoundBox(_pWorldData, pObj1, Box);

	const CWO_PhysicsPrim& Prim1 = _pBody1->m_pCoreData->GetPhysState().m_Prim[0];

	const CWO_PhysicsPrim& Prim2 = pObj2->GetPhysState().m_Prim[0];
	CXR_Model* pModel2 = _pWorldData->GetResource_Model(Prim2.m_iPhysModel);
	M_ASSERT( pModel2, "" );

	CXR_PhysicsModel* pPhysModel2 = pModel2->Phys_GetInterface();
	M_ASSERT( pPhysModel2, "" );

	CMat43fp64 tmp;
	tmp = _pBody1->GetOrientationMatrix();
	CVec3Dfp64::GetRow(tmp,3) = _pBody1->GetPosition();
	CMat4Dfp32 P1 = ConvertMatrix(tmp);
	P1.Transpose3x3();

	const CMat4Dfp32 &BspTransform = pObj2->GetPositionMatrix();

	CCollisionInfo CollisionInfo[100];
	CXR_PhysicsContext Context(BspTransform);
	Context.m_PhysGroupMaskThis = Prim2.m_PhysModelMask;
	Context.m_PhysGroupMaskCollider = Prim1.m_PhysModelMask;
	Context.m_pWC = _pPhysState->Debug_GetWireContainer();

	CVec3Dfp32 CenterOfMass(_pBody1->m_MassCenter[0], _pBody1->m_MassCenter[1], _pBody1->m_MassCenter[2]);

	int nCollisions = pPhysModel2->Phys_CollideBSP2(&Context, _pBSP2, P1, Prim1.GetOffset() - CenterOfMass, pObj1->GetPhysState().m_MediumFlags, CollisionInfo, 100);

	int nTotCol = 0;
	for (int i = 0; i < 100 && i < nCollisions && i < _MaxCollisions; i++)
	{
		_pContactInfo[nTotCol].m_pRigidBody1 = (CRigidBody *) _pBody1;
		_pContactInfo[nTotCol].m_pRigidBody2 = NULL;
		_pContactInfo[nTotCol].m_Normal = -ConvertVector(CollisionInfo[i].m_Plane.n);
		_pContactInfo[nTotCol].m_PointOfCollision = ConvertVector(CollisionInfo[i].m_Pos);
		_pContactInfo[nTotCol].m_distance = fabs(CollisionInfo[i].m_Distance);
		_pContactInfo[nTotCol].m_isColliding = CollisionInfo[i].m_bIsCollision;
		_pContactInfo[nTotCol].m_UserData1 = pObj1->m_iObject;
		_pContactInfo[nTotCol].m_UserData2 = pObj2->m_iObject;

		//M_TRACEALWAYS("PointOfCollision = %s\n",_pContactInfo[nTotCol].m_PointOfCollision.GetString().Str());
		//M_TRACEALWAYS("Normal = %s\n",_pContactInfo[nTotCol].m_Normal.GetString().Str());

		if (IsValidVec3(_pContactInfo[nTotCol].m_Normal))
		{
			nTotCol++;
		}
		else
		{
			int breakme = 0;
		}
	}
	return nTotCol;
}


static void DebugRender(CMapData *_pWorldData, CWireContainer *_pWireContainer, const CWObject_CoreData *_pObject)
{
	CRigidBody *pBody = _pObject->m_pRigidBody;

	//CPixel32 OBBColour(220, 0, 0, 255);
	//CPixel32 SolidColour(0, 220, 0, 255);
	CPixel32 OBBColour(0, 220, 0, 255);
	CPixel32 SolidColour(0, 0, 220, 255);


	if (pBody)
	{
		const CWO_PhysicsPrim& Prim = _pObject->GetPhysState().m_Prim[0];
		if (Prim.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL)
		{
			CVec3Dfp32 CenterOfMass(0.0f);
			CMat4Dfp64 tmp;

			// Get transform
			CMat4Dfp32 T;
			_pObject->m_pRigidBody->GetCorrectTransform(tmp);
			T = ConvertMatrix(tmp);
			CenterOfMass = _pObject->m_pRigidBody->m_MassCenter.Getfp32();

			CVec3Dfp32 TotalOffset = Prim.GetOffset() - CenterOfMass;

			T.GetRow(3) = TotalOffset * T;

			CXR_Model* pModel = _pWorldData->GetResource_Model(Prim.m_iPhysModel);
			if (!pModel)
				return;

			CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();
			if (!pPhysModel)
				return;

			int MediumFlags = _pObject->GetPhysState().m_MediumFlags;

			int ModelClass = pModel->GetModelClass();
			if (ModelClass == CXR_MODEL_CLASS_BSP2)
			{
				CXR_Model_BSP2 *pModelBSP2 = (CXR_Model_BSP2 *) pModel;
				CXR_IndexedSolidContainer32 *pIndexedSolids = pModelBSP2->m_spIndexedSolids;
				if (pIndexedSolids != NULL)
				{
					TAP<const CXR_IndexedSolid32> pSolids = pIndexedSolids->m_lSolids;
					TAP<const CXR_MediumDesc> pMediums = pIndexedSolids->m_lMediums;

					for (int i = 0; i < pSolids.Len(); i++)
					{
						int iMedium = pSolids[i].m_iMedium;
						const CXR_MediumDesc& MediumDesc = pMediums[iMedium];

						if (!(MediumDesc.m_MediumFlags & MediumFlags)) continue;
						if (!(Prim.m_PhysModelMask & M_Bit(MediumDesc.m_iPhysGroup))) continue;

						CXR_IndexedSolidDesc32 soliddesr;
						pIndexedSolids->GetSolid(i, soliddesr);

						CVec3Dfp32 Ext = (soliddesr.m_BoundBox.m_Max - soliddesr.m_BoundBox.m_Min) * 0.5f;

						CMat4Dfp32 Tmp = T;
						CVec3Dfp32 Center;
						soliddesr.m_BoundBox.GetCenter(Center);
						Tmp.GetRow(3) = Center * T;

						if (pIndexedSolids->m_lIsObbs[i])
							_pWireContainer->RenderOBB(Tmp, Ext, OBBColour, 1.0f / 20.0f, false);
						else
							_pWireContainer->RenderOBB(Tmp, Ext, SolidColour, 1.0f / 20.0f, false);
					}
				}
			}
		}
	}
#endif
}

CIndexPool16 g_TestedObjects;

static int DoCollide(const CWorld *_pWorld, 
					 const TArray<CRigidBody *>& _BodyList,
					 CContactInfo *_pCollisionInfo, 
					 int _MaxCollisions,
					 CMapData *_pWorldData,
					 CWorld_PhysState *_pPhysState)
{
	CIndexPool16 TestedObjects;
	TestedObjects.Create(1000);

	/*
	uint16 ObjectIntersectFlags1 = PhysState1.m_ObjectIntersectFlags;

	//if (!PhysState2.m_nPrim)
	//	return 0;

	// Should test all primitives unless we make it an official rule that only Prim 0 can be used for dynamics.
	const CWO_PhysicsPrim& PhysPrim1 = PhysState1.m_Prim[0];
	const CWO_PhysicsPrim& PhysPrim2 = PhysState2.m_Prim[0];
	if (!((PhysState1.m_ObjectFlags & PhysPrim1.m_ObjectFlagsMask & PhysState2.m_ObjectIntersectFlags) ||
		(PhysState2.m_ObjectFlags & PhysPrim2.m_ObjectFlagsMask & ObjectIntersectFlags1)))
		return 0;
*/

/*
	TAP_RCD<CRigidBody *> pBodies = _BodyList;
	
	for (int i = 0; i < pBodies; i++)
	{
	const CWD_RigidBody *pBody1 = (CWD_RigidBody *) pBodies[i];
	const CWObject_CoreData *pObject1 = pBody1->m_pCoreData;

	if (_pWorld->IsStationary(pBody1))
	continue;

	CWO_EnumParams_Box EnumBoxParams;
	const CWO_PhysicsState& PhysState = pObject1->GetPhysState();
	if (!PhysState.m_nPrim)
	continue;

	CWD_RigidBody::GetBoundBox(pWorldData, pObj, BoundBox);
	EnumBoxParams.m_ObjectFlags = PhysState.m_ObjectIntersectFlags;		// <-- Swapped these assignments	-mh
	EnumBoxParams.m_ObjectIntersectFlags = PhysState.m_ObjectFlags;
	EnumBoxParams.m_ObjectNotifyFlags = 0;
	BoundBox.m_Min -= CVec3Dfp32(1,1,1);
	BoundBox.m_Max += CVec3Dfp32(1,1,1);
	BoundBox.Transform(pObj->GetPositionMatrix(), EnumBoxParams.m_Box);

	EnumBoxParams.m_ObjectFlags |= OBJECT_FLAGS_PLAYERPHYSMODEL;
	EnumBoxParams.m_ObjectIntersectFlags |= OBJECT_FLAGS_PLAYERPHYSMODEL;
	uint16 ObjectIntersectFlags = PhysState.m_ObjectIntersectFlags;
	ObjectIntersectFlags |= OBJECT_FLAGS_PLAYERPHYSMODEL;

	int nPresumptiveColl = pPhysState->m_spSpaceEnum->EnumerateBox(EnumBoxParams, EnumIds, N_ENUM_IDS);
	}
	*/

	return 0;
}

static int CollideSolidSolid2(const CWObject_CoreData *_pObject1,
							  const CWObject_CoreData *_pObject2,
							  CXR_Model *_pModel1, 
							  CXR_Model *_pModel2,
							  const CWO_PhysicsPrim& _PhysicsPrim1,
							  const CWO_PhysicsPrim& _PhysicsPrim2,
							  CMapData *_pWorldData,
							  CContactInfo *_pContactInfo,
							  int _MaxCollisions)
{
	CXR_PhysicsContext Context;

	CMat4Dfp32 Transform1, Transform2;
	SetupCollision(_pObject1, _pObject2, _pWorldData, Transform1, Transform2, Context);

	M_ASSERT(_pModel1->GetModelClass() == CXR_MODEL_CLASS_BSP2 && _pModel2->GetModelClass() == CXR_MODEL_CLASS_BSP2, "Internal error!");

	CXR_Model_BSP2 *pBSP1 = (CXR_Model_BSP2 *) _pModel1;
	CXR_Model_BSP2 *pBSP2 = (CXR_Model_BSP2 *) _pModel2;

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
	CCollisionInfo CollisionInfo[MaxCollisions];
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

			if (!(MediumDesc1.m_MediumFlags & XW_MEDIUM_SOLID && MediumDesc2.m_MediumFlags && XW_MEDIUM_SOLID)) continue;
			if (!(_PhysicsPrim1.m_PhysModelMask & M_Bit(MediumDesc1.m_iPhysGroup))) continue;
			if (!(_PhysicsPrim2.m_PhysModelMask & M_Bit(MediumDesc2.m_iPhysGroup))) continue;

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

				iCollision += CopyCollisionInfo(nC, CollisionInfo, &_pContactInfo[iCollision], _MaxCollisions,
											    (CRigidBody *) _pObject1->m_pRigidBody,
											    (CRigidBody *) _pObject2->m_pRigidBody,
												_pObject1, _pObject2);
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

				CSolidPolyhedronSeparating TempSolid1(&SolidDescr1, pIndexedSolids1->m_lSolids[is1].m_nVertices);
				CSolidPolyhedronSeparating TempSolid2(&SolidDescr2, pIndexedSolids2->m_lSolids[is2].m_nVertices);
				nC = TPolyhedraPolyhedraCollider<CSolidPolyhedronSeparating, CSolidPolyhedronSeparating, fp32>
							::Collide(&TempSolid1, Transform1,
							&TempSolid2, Transform2, CenterAxis, PointOfCollisions, Normals, Depths);

				iCollision += CopyCollisionInfo(nC, CollisionInfo, &_pContactInfo[iCollision], _MaxCollisions,
											    (CRigidBody *) _pObject1->m_pRigidBody,
											    (CRigidBody *) _pObject2->m_pRigidBody,
												_pObject1, _pObject2);
			}
		}
	}
	M_ASSERT(iCollision <= _MaxCollisions, "Internal error!");
	return iCollision;
}

int CWO_DynamicsCollider::CollideSolidBsp2(const CWObject_CoreData *_pObject1,
										   const CWObject_CoreData *_pObject2,
										   CXR_Model *_pModel1, 
										   CXR_Model *_pModel2,
										   const CWO_PhysicsPrim& _PhysicsPrim1,
										   const CWO_PhysicsPrim& _PhysicsPrim2,
										   CContactInfo *_pContactInfo, 
										   int _MaxCollisions,
										   CMapData *_pWorldData,
										   CWorld_PhysState *_pPhysState)
{
	MSCOPESHORT(CWO_DynamicsCollider::CollideSolidBsp2);

	CXR_Model_BSP2 *pBSP2 = safe_cast<CXR_Model_BSP2>(_pModel2);

	const CWD_RigidBody *pRigidBody1 = (CWD_RigidBody *) _pObject1->m_pRigidBody;
	CXR_PhysicsModel* pPhysModel2 = _pModel2->Phys_GetInterface();

	CBox3Dfp32 Box;
	CWD_RigidBody::GetBoundBox(_pWorldData, _pObject1, Box);

	CMat4Dfp32 T1;
	pRigidBody1->GetCorrectTransform(T1);
	const CMat4Dfp32 &T2 = _pObject2->GetPositionMatrix();

	const int MaxCollisions = 100;
	CCollisionInfo CollisionInfo[MaxCollisions];

	CXR_PhysicsContext Context(T2, _PhysicsPrim2.m_PhysModelMask, _PhysicsPrim1.m_PhysModelMask, NULL, NULL);
	Context.m_pWC = _pPhysState->Debug_GetWireContainer();

	CVec3Dfp32 CenterOfMass(pRigidBody1->m_MassCenter[0], pRigidBody1->m_MassCenter[1], pRigidBody1->m_MassCenter[2]);
	int nCollisions = pPhysModel2->Phys_CollideBSP2(&Context, pBSP2, T1, _PhysicsPrim1.GetOffset() - CenterOfMass, _pObject1->GetPhysState().m_MediumFlags, CollisionInfo, MaxCollisions);

	int nTotCol = 0;
	for (int i = 0; i < MaxCollisions && i < nCollisions && i < _MaxCollisions; i++)
	{
		_pContactInfo[nTotCol].m_pRigidBody1 = (CRigidBody *) pRigidBody1;
		_pContactInfo[nTotCol].m_pRigidBody2 = NULL;
		_pContactInfo[nTotCol].m_Normal = -ConvertVector(CollisionInfo[i].m_Plane.n);
		_pContactInfo[nTotCol].m_PointOfCollision = ConvertVector(CollisionInfo[i].m_Pos);
		_pContactInfo[nTotCol].m_distance = fabs(CollisionInfo[i].m_Distance);
		_pContactInfo[nTotCol].m_isColliding = CollisionInfo[i].m_bIsCollision;
		_pContactInfo[nTotCol].m_UserData1 = _pObject1->m_iObject;
		_pContactInfo[nTotCol].m_UserData2 = _pObject2->m_iObject;
	}
	return nTotCol;
}

int CWO_DynamicsCollider::DoCollidePair(const CWorld *_pWorld, 
										ColliderContext *_pColliderContext,
										CWObject_CoreData *_pObject1,
										CWObject_CoreData *_pObject2,
										CContactInfo *_pCollisionInfo, 
										int _MaxCollisions,
										CMapData *_pWorldData,
										CWorld_PhysState *_pPhysState)
{
	int iObject1 = _pObject1->m_iObject;
	int iObject2 = _pObject2->m_iObject;

	CWD_RigidBody *pRigidBody1 = (CWD_RigidBody *) _pObject1;
	CWD_RigidBody *pRigidBody2 = (CWD_RigidBody *) _pObject2;

	M_ASSERT(_pObject1 != _pObject2, "Internal error!");
	M_ASSERT(!_pColliderContext->IsTested(iObject1, iObject2), "Internal error!");
	M_ASSERT(!_pWorld->IsConnected(pRigidBody1, pRigidBody2), "Internal error!");

	int nCollisions = 0;

	const CWO_PhysicsState& PhysState1 = _pObject1->GetPhysState();
	const CWO_PhysicsState& PhysState2 = _pObject2->GetPhysState();
	const CWO_PhysicsPrim& Prim1 = PhysState1.m_Prim[0];
	const CWO_PhysicsPrim& Prim2 = PhysState2.m_Prim[0];

	if (!(Prim1.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL) && (Prim2.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL))
		return 0;

	CXR_Model* pModel1 = _pWorldData->GetResource_Model(Prim1.m_iPhysModel);
	CXR_Model* pModel2 = _pWorldData->GetResource_Model(Prim2.m_iPhysModel);
	int ModelClass1 = pModel1->GetModelClass();
	int ModelClass2 = pModel2->GetModelClass();

	/*
		TODO: Skicka in model eller physinteface i de verkliga kollisionsrutinerna.
		De plockar ut samma saker igen!
	 */

	if (pRigidBody2 == NULL && _pObject2->IsClass(class_WorldPlayerPhys) && pRigidBody1->IsActive())
	{
		if (!_pWorld->IsConnectedToWorld(pRigidBody1))
		{
			if (ModelClass1 == CXR_MODEL_CLASS_BSP2)
			{
				CXR_Model_BSP2 *pModelBSP2_1 = (CXR_Model_BSP2 *) pModel1;
				nCollisions = CollideSolidBsp(pRigidBody1, pModelBSP2_1, 
											  _pObject2,
											  _pCollisionInfo, _MaxCollisions,
											  _pWorldData,
											  _pPhysState);
			}
		}
	}
	else if (pRigidBody2 != NULL) 
	{
		if (!(!pRigidBody1->IsActive() && !pRigidBody2->IsActive()))
		{
			if (ModelClass1 == CXR_MODEL_CLASS_BSP2 && ModelClass2 == CXR_MODEL_CLASS_BSP2)
			{
				CXR_Model_BSP2 *pModelBSP2_1 = (CXR_Model_BSP2 *) pModel1;
				CXR_Model_BSP2 *pModelBSP2_2 = (CXR_Model_BSP2 *) pModel2;
				CXR_IndexedSolidContainer32 *pIndexedSolids1 = pModelBSP2_1->m_spIndexedSolids;
				CXR_IndexedSolidContainer32 *pIndexedSolids2 = pModelBSP2_2->m_spIndexedSolids;
				// TODO: Remove "test"?
				if (pIndexedSolids1 == NULL || pIndexedSolids2 == NULL) return 0;
				nCollisions = CollideSolidSolid2(_pObject1, _pObject2, pModel1, pModel2, Prim1, Prim2, _pWorldData, _pCollisionInfo, _MaxCollisions);
			}
		}
	}


	return nCollisions;
}

int CWO_DynamicsCollider::Collide(const CWorld *_pWorld, 
								  const TArray<CRigidBody *>& _BodyList,
								  CContactInfo *_pCollisionInfo, 
								  int _MaxCollisions,
								  void *_pArgument1,
								  void *_pArgument2)
{
	MSCOPESHORT(CWO_DynamicsCollider::Collide_2);
	
	int CollisionPrecision = _pWorld->GetCollisionPrecision();

	//const TArray<CRigidBody *>& BodyList = _pWorld->GetRigidBodyList();
	int nBodies = _BodyList.Len();
	int nTotCollisions = 0;

	CMapData *pWorldData = (CMapData *) _pArgument1;
	CWorld_PhysState *pPhysState = (CWorld_PhysState *) _pArgument2;

	s_TestColl.SetEmpty();
	
	const int N_ENUM_IDS = 200;
	int16 EnumIds[N_ENUM_IDS];

	for (int i = 0; i < nBodies; i++)
	{
		const CWD_RigidBody *pBody = (CWD_RigidBody *) _BodyList[i];

#ifndef M_RTM
		CMat4Dfp64 foo;
		pBody->GetTransform(foo);
		CMat4Dfp32 bar = foo.Getfp32();
		bar.Transpose3x3();

//		pPhysState->Debug_RenderMatrix(bar, 1.0f/10.0f, false);
#endif

		if (_pWorld->IsStationary(pBody))
			continue;

#ifndef M_RTM
		if (pPhysState->Debug_GetWireContainer())
			DebugRender(pWorldData, pPhysState->Debug_GetWireContainer(), pBody->m_pCoreData);
#endif

		//		int EnumerateBox(const CWO_EnumParams_Box& _Params, int16* _pEnumRetIDs, int _MaxEnumIDs);
		//		EnumBox.m_Box = 

		const CWObject_CoreData *pObj = pBody->m_pCoreData;

		CBox3Dfp32 BoundBox;
		CWD_RigidBody::GetBoundBox(pWorldData, pObj, BoundBox);

		CWO_EnumParams_Box EnumBoxParams;
		const CWO_PhysicsState& PhysState = pObj->GetPhysState();
		if (!PhysState.m_nPrim)
			continue;

		EnumBoxParams.m_ObjectFlags = PhysState.m_ObjectIntersectFlags;		// <-- Swapped these assignments	-mh
		EnumBoxParams.m_ObjectIntersectFlags = PhysState.m_ObjectFlags;
		EnumBoxParams.m_ObjectNotifyFlags = 0;
		BoundBox.m_Min -= CVec3Dfp32(1,1,1);
		BoundBox.m_Max += CVec3Dfp32(1,1,1);
		BoundBox.Transform(pObj->GetPositionMatrix(), EnumBoxParams.m_Box);

		EnumBoxParams.m_ObjectFlags |= OBJECT_FLAGS_PLAYERPHYSMODEL;
		EnumBoxParams.m_ObjectIntersectFlags |= OBJECT_FLAGS_PLAYERPHYSMODEL;
		uint16 ObjectIntersectFlags = PhysState.m_ObjectIntersectFlags;

		if (true)
		{

			ObjectIntersectFlags |= OBJECT_FLAGS_PLAYERPHYSMODEL;
			//ObjectIntersectFlags &= ~OBJECT_FLAGS_PHYSMODEL;
		}

		int nPresumptiveColl = pPhysState->m_spSpaceEnum->EnumerateBox(EnumBoxParams, EnumIds, N_ENUM_IDS);

		for (int j = 0; j < nPresumptiveColl; j++)
		{
			CWObject_CoreData *pObj2 = pPhysState->Object_GetCD(EnumIds[j]);
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

			const CRigidBody *pBody2 = pObj2->m_pRigidBody;

//			if (_pWorld->IsStationary(pBody) && pBody2 && _pWorld->IsStationary(pBody2))
//				continue;

//			if (_pWorld->IsStationary(pBody) && pBody2 == NULL)
//				continue;

			int id1 = pObj->m_iObject;
			int id2 = pObj2->m_iObject;

			M_ASSERT(id1 < N_MAX_COLL_HACK, "");
			M_ASSERT(id2 < N_MAX_COLL_HACK, "");

			if (pBody != NULL && !s_TestColl.Test(id1, id2))
			{
				const CWO_PhysicsPrim& Prim1 = pObj->GetPhysState().m_Prim[0];
				const CWO_PhysicsPrim& Prim2 = pObj2->GetPhysState().m_Prim[0];

				CXR_Model* pModel2 = pWorldData->GetResource_Model(Prim2.m_iPhysModel);

				if (pModel2)
				{
					const CMat4Dfp32 &LiqTransform = pObj2->GetPositionMatrix();
					CBox3Dfp32 Box;
					CWD_RigidBody::GetBoundBox(pWorldData, pObj, Box);
					Box.m_Min -= pBody->m_MassCenter.Getfp32();
					Box.m_Max -= pBody->m_MassCenter.Getfp32();

					if (pObj2->GetPhysState().m_PhysFlags & OBJECT_PHYSFLAGS_MEDIUMQUERY)
					{
						CMat4Dfp64 Ttmp;
						CMat4Dfp32 T;
						pBody->GetCorrectTransform(Ttmp);
						T = Ttmp.Getfp32();

						((CWD_RigidBody *) pBody)->SetStationary(false);

						s_TestColl.Add(id1, id2);

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
							m_lBouyancyObjects.Add((CWD_RigidBody *) pBody);
					}
				}
			}

#if 1
			/*
				BSP collide test
			 */
//			if (pBody != pBody2 && pBody2 == NULL && pObj2->IsClass(class_WorldSpawn) && pBody->IsActive())
			if (pBody != pBody2 && pBody2 == NULL && pObj2->IsClass(class_WorldPlayerPhys) && pBody->IsActive())
			{
				if (!_pWorld->IsConnectedToWorld(pBody) && !s_TestColl.Test(id1, id2))
				{
					const CWO_PhysicsPrim& Prim = pObj2->GetPhysState().m_Prim[0];
					CXR_Model* pModel = pWorldData->GetResource_Model(Prim.m_iPhysModel);
					if (!pModel)
						continue;
					CXR_PhysicsModel* pPhysModel = pModel->Phys_GetInterface();
					if (!pPhysModel)
						continue;
					//pPhysModel->Phys_IntersectBox()

#if 1
					int nColl = 0;
					const CWO_PhysicsPrim& Prim1 = pObj->GetPhysState().m_Prim[0];
					if (Prim1.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL)
					{
						CXR_Model* pModel1 = pWorldData->GetResource_Model(Prim1.m_iPhysModel);
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
#if 1
								
								nColl = CollideSolidBsp((CWD_RigidBody *) pBody, pModelBSP2_1, 
														pObj2,
														&_pCollisionInfo[nTotCollisions], _MaxCollisions - nTotCollisions,
														pWorldData,
														pPhysState);
#else

								nColl = CollideSolidBspNew(pObj, pObj2, pWorldData, pPhysState, &_pCollisionInfo[nTotCollisions], _MaxCollisions - nTotCollisions);

#endif

							}
						}
					}
#else

					int nColl = Collide2((CWD_RigidBody *) pBody, 
										pObj2, 
										&_pCollisionInfo[nTotCollisions], _MaxCollisions - nTotCollisions,
										pWorldData);
#endif

#if 0
					for (int q = 0; q < nColl; q++)
					{
						M_TRACEALWAYS("%s\n",_pCollisionInfo[nTotCollisions].m_PointOfCollision.GetString().Str());
					}
					if (nColl > 0) 
					{
						
						M_TRACEALWAYS("%d %d ------------------\n",id1,id2);
					}
#endif

					nTotCollisions += nColl;

					s_TestColl.Add(id1, id2);
				}
			}
#endif

#if 0
			if (pBody2 == NULL)
			{
				int id = pObj2->m_iClass;
				if (id != 216 && id != 226 && id != 20 && id != 238)
					M_TRACEALWAYS("%d\n",id);

				if (id == 20 || id == 238)
				{
					int foobar123 = 123;
					M_TRACEALWAYS("%d\n",id);

				}
			}
#endif

			bool bIsCharacter = (PhysState2.m_ObjectFlags & OBJECT_FLAGS_CHARACTER) != 0;

			// TODO: Temporärt avstängd...
#if 1
			if (pBody2 == NULL && bIsCharacter && pBody->IsActive())
			{				
				int nColl = Collide3((CWD_RigidBody *) pBody, 
									 pObj2,
									 &_pCollisionInfo[nTotCollisions], _MaxCollisions - nTotCollisions,
									 pWorldData,
									 pPhysState);				
				nTotCollisions += nColl;

				if (!s_TestColl.Test(id1, id2))
					s_TestColl.Add(id1, id2);


/*				int id = pObj2->m_iClass;
				if (id != 216 && id != 226)
					M_TRACEALWAYS("%d\n",id);*/
			}
#endif

			if (!s_TestColl.Test(id1, id2))
			{
				if (pBody2 != NULL && pBody != pBody2 && !_pWorld->IsConnected(pBody, pBody2)) {
					if (!(!pBody->IsActive() && !pBody2->IsActive()))
					{

						int nColl = 0;


						//InspectSolidData(pWorldData, pObj);
						//InspectSolidData(pWorldData, pObj2);

						bool solidsolid = false;


						const CWO_PhysicsPrim& Prim1 = pObj->GetPhysState().m_Prim[0];
						const CWO_PhysicsPrim& Prim2 = pObj2->GetPhysState().m_Prim[0];


						if (CollisionPrecision == 0)
						{
						if ((Prim1.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL) && (Prim2.m_PrimType == OBJECT_PRIMTYPE_PHYSMODEL))
						{
							CXR_Model* pModel1 = pWorldData->GetResource_Model(Prim1.m_iPhysModel);
							CXR_Model* pModel2 = pWorldData->GetResource_Model(Prim2.m_iPhysModel);
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

								nColl += CollideSolidSolid(pObj, pObj2, pWorldData, &_pCollisionInfo[nTotCollisions], _MaxCollisions - nTotCollisions);
								//CollideSolidSolid(pObj, pObj2, pWorldData, &_pCollisionInfo[nTotCollisions], _MaxCollisions - nTotCollisions);

								if (false)
								for (int is1 = 0; is1 < pIndexedSolids1->m_lSolids.Len(); is1++)
								{
									for (int is2 = 0; is2 < pIndexedSolids2->m_lSolids.Len(); is2++)
									{
										
										int iMedium1 = pIndexedSolids1->m_lSolids[is1].m_iMedium;
										int iMedium2 = pIndexedSolids2->m_lSolids[is2].m_iMedium;

										const CXR_MediumDesc& MediumDesc1 = pIndexedSolids1->m_lMediums[iMedium1];
										const CXR_MediumDesc& MediumDesc2 = pIndexedSolids2->m_lMediums[iMedium2];

										if (!(MediumDesc1.m_MediumFlags & XW_MEDIUM_SOLID && MediumDesc2.m_MediumFlags && XW_MEDIUM_SOLID)) continue;
										if (!(Prim1.m_PhysModelMask & M_Bit(MediumDesc1.m_iPhysGroup))) continue;
										if (!(Prim2.m_PhysModelMask & M_Bit(MediumDesc2.m_iPhysGroup))) continue;

										CVec3Dfp32 p1, p2;

										CMat4Dfp64 trans1, trans2;
										((CWD_RigidBody *) pBody)->GetTransform(trans1);
										((CWD_RigidBody *) pBody2)->GetTransform(trans2);

										CMat4Dfp32 trans1fp32, trans2fp32;
										trans1fp32 = ConvertMatrix2(trans1);
										trans2fp32 = ConvertMatrix2(trans2);

										trans1fp32.Transpose3x3();
										trans2fp32.Transpose3x3();

										CVec3Dfp32 mc1(pBody->m_MassCenter[0], pBody->m_MassCenter[1], pBody->m_MassCenter[2]);
										CVec3Dfp32 mc2(pBody2->m_MassCenter[0], pBody2->m_MassCenter[1], pBody2->m_MassCenter[2]);

										CVec3Dfp32 Offset1 = Prim1.GetOffset() - mc1;
										CVec3Dfp32 Offset2 = Prim2.GetOffset() - mc2;

										CBox3Dfp32 Box1 = pIndexedSolids1->m_lSolids[is1].m_BoundBox;
										Box1.m_Min += Offset1;
										Box1.m_Max += Offset1;
										CBox3Dfp32 Box2 = pIndexedSolids2->m_lSolids[is2].m_BoundBox;
										Box2.m_Min += Offset2;
										Box2.m_Max += Offset2;

										CBox3Dfp32 Box1Transformed, Box2Transformed;
										Box1.Transform(trans1fp32, Box1Transformed);
										Box2.Transform(trans2fp32, Box2Transformed);

										Box1Transformed.m_Min -= CVec3Dfp32(0.2f);
										Box1Transformed.m_Max += CVec3Dfp32(0.2f);

										Box2Transformed.m_Min -= CVec3Dfp32(0.2f);
										Box2Transformed.m_Max += CVec3Dfp32(0.2f);

										CPixel32 Col(0,0,220);

										if (!Box1Transformed.IsInside(Box2Transformed))
										{
											Col = CPixel32(0,220,0);
											continue;
										}
#ifndef M_RTM
										if (pPhysState->Debug_GetWireContainer())
										{
											pPhysState->Debug_RenderAABB(Box1Transformed, Col, 1.0f/10.0f, false);
											pPhysState->Debug_RenderAABB(Box2Transformed, Col, 1.0f/10.0f, false);

										}
#endif

										CXR_IndexedSolidDesc32 SolidDescr1, SolidDescr2;
										pIndexedSolids1->GetSolid(is1, SolidDescr1);
										pIndexedSolids2->GetSolid(is2, SolidDescr2);
										CSolidPolyhedron2 polyhed1(&SolidDescr1, pIndexedSolids1->m_lSolids[is1].m_nVertices, trans1fp32, Offset1);
										CSolidPolyhedron2 polyhed2(&SolidDescr2, pIndexedSolids2->m_lSolids[is2].m_nVertices, trans2fp32, Offset2);
										//CSolidPolyhedron polyhed1(pIndexedSolids1, is1, trans1fp32, Prim1.GetOffset() - mc1);
										//CSolidPolyhedron polyhed2(pIndexedSolids2, is2, trans2fp32, Prim2.GetOffset() - mc2);

#ifndef M_RTM
										if (pPhysState->Debug_GetWireContainer())
										{
											polyhed1.Render(pPhysState->Debug_GetWireContainer());
											polyhed2.Render(pPhysState->Debug_GetWireContainer());
										}
#endif

										CMat4Dfp32 ident;
										ident.Unit();

										trans1fp32.GetRow(3) = Offset1 * trans1fp32;
										trans2fp32.GetRow(3) = Offset2 * trans2fp32;


										

										CVec3Dfp32 PointOfCollisions[8];
										CVec3Dfp32 Normals[8];
										fp32 Depths[8];


										CVec3Dfp32 Center1 = (SolidDescr1.m_BoundBox.m_Max + SolidDescr1.m_BoundBox.m_Min) * 0.5f;
										CVec3Dfp32 Center2 = (SolidDescr2.m_BoundBox.m_Max + SolidDescr2.m_BoundBox.m_Min) * 0.5f;
										Center1 *= trans1fp32;
										Center2 *= trans2fp32;
										CVec3Dfp32 CenterAxis = Center2 - Center1;
										CenterAxis.Normalize();
										
										int N;
										{
											CSolidPolyhedronSeparating TempSolid1(&SolidDescr1, pIndexedSolids1->m_lSolids[is1].m_nVertices);
											CSolidPolyhedronSeparating TempSolid2(&SolidDescr2, pIndexedSolids2->m_lSolids[is2].m_nVertices);
											N =	TPolyhedraPolyhedraCollider<CSolidPolyhedronSeparating, CSolidPolyhedronSeparating, fp32>
												::Collide(&TempSolid1, trans1fp32,
														&TempSolid2, trans2fp32, CenterAxis, PointOfCollisions, Normals, Depths);
										}

												

										for (int iColl = 0; iColl < N; iColl++)
										{
											_pCollisionInfo[nTotCollisions + nColl + iColl].m_pRigidBody1 = (CRigidBody *) pBody;
											_pCollisionInfo[nTotCollisions + nColl + iColl].m_pRigidBody2 = (CRigidBody *) pBody2;
											_pCollisionInfo[nTotCollisions + nColl + iColl].m_Normal = Normals[iColl].Getfp64();
											_pCollisionInfo[nTotCollisions + nColl + iColl].m_PointOfCollision = PointOfCollisions[iColl].Getfp64();
											_pCollisionInfo[nTotCollisions + nColl + iColl].m_distance = Depths[iColl];
											_pCollisionInfo[nTotCollisions + nColl + iColl].m_isColliding = true;
											_pCollisionInfo[nTotCollisions + nColl + iColl].m_UserData1 = pObj->m_iObject;
											_pCollisionInfo[nTotCollisions + nColl + iColl].m_UserData2 = pObj2->m_iObject;
										}

										nColl += N;


		//								solidsolid = false;

										if (false)
										{
											MSCOPESHORT(CWO_DynamicsCollider::Solid_To_Solid);
											if (TGJK<CSolidPolyhedron2, CSolidPolyhedron2>::MarginPenetrationDepth(polyhed1, ident, polyhed2, ident, p1, p2, 0.01f*32, 0.01f*32))
											{
												if (nTotCollisions + nColl >= _MaxCollisions) break;
												
												_pCollisionInfo[nTotCollisions + nColl].m_pRigidBody1 = (CRigidBody *) pBody;
												_pCollisionInfo[nTotCollisions + nColl].m_pRigidBody2 = (CRigidBody *) pBody2;
												_pCollisionInfo[nTotCollisions + nColl].m_Normal = ConvertVector(p2 - p1);
												_pCollisionInfo[nTotCollisions + nColl].m_Normal.Normalize();
												_pCollisionInfo[nTotCollisions + nColl].m_PointOfCollision = ConvertVector(p2);
												_pCollisionInfo[nTotCollisions + nColl].m_distance = p1.Distance(p2);
												_pCollisionInfo[nTotCollisions + nColl].m_isColliding = true;
												_pCollisionInfo[nTotCollisions + nColl].m_UserData1 = pObj->m_iObject;
												_pCollisionInfo[nTotCollisions + nColl].m_UserData2 = pObj2->m_iObject;

												bool isvalid = true;
												if (!IsValidVec3(_pCollisionInfo[nTotCollisions + nColl].m_Normal))
												{
													//int breakme = 0;
													//M_BREAKPOINT;

													isvalid = false;
													//M_TRACEALWAYS("Normal: %s\n", _pCollisionInfo[nTotCollisions + nColl].m_Normal.GetString().Str());
													//M_TRACEALWAYS("Distance: %f\n", _pCollisionInfo[nTotCollisions + nColl].m_distance);
												}

												CPlane3Dfp32 Plane1, Plane2;
	//											FindClosestPlane(SolidDescr1, Plane1);
	//											FindClosestPlane(SolidDescr2, Plane2);

												if (_pCollisionInfo[nTotCollisions + nColl].m_distance > 5)
												{
//													int breakme = 0;
												}


												if (isvalid)
													nColl += 1;
												//nTotCollisions++; // TODO: HACK!!!!

											}
										}
									}
								}
							}
						}
						}
						else
						{
							// TODO: Temporärt hack tills dess att man kan skapa "äkta" boxar (inte bsp).
							if (Prim1.m_PrimType == OBJECT_PRIMTYPE_BOX && Prim2.m_PrimType == OBJECT_PRIMTYPE_BOX)
							{
								nColl = CollideAsBoxes((CWD_RigidBody *) pBody, 
													(CWD_RigidBody *) pBody2, 
														&_pCollisionInfo[nTotCollisions], _MaxCollisions - nTotCollisions,
														pWorldData);
							}
							else if (Prim1.m_PrimType == OBJECT_PRIMTYPE_BOX)
							{
								nColl = Collide((CWD_RigidBody *) pBody2, 
												(CWD_RigidBody *) pBody, 
												&_pCollisionInfo[nTotCollisions], _MaxCollisions - nTotCollisions,
												pWorldData);
							}
							else
							{
/*								nColl = Collide((CWD_RigidBody *) pBody, 
												(CWD_RigidBody *) pBody2, 
												&_pCollisionInfo[nTotCollisions], _MaxCollisions - nTotCollisions,
												pWorldData);
												*/
								
								nColl = CollideAsBoxes((CWD_RigidBody *) pBody, 
									(CWD_RigidBody *) pBody2, 
									&_pCollisionInfo[nTotCollisions], _MaxCollisions - nTotCollisions,
									pWorldData);
									

							}
						}


						nTotCollisions += nColl;
						s_TestColl.Add(id1, id2);
					}
				}
			}
		}
	}

	return nTotCollisions;
}



void CWO_DynamicsCollider::PreApplyExternalForces(void *_pArgument1, void *_pArgument2)
{
	CWorld_Server *pServer = (CWorld_Server *) _pArgument2;

	m_lBouyancyObjects.QuickSetLen(0);
}


void CWO_DynamicsCollider::ApplyExternalForces(void *_pArgument1, void *_pArgument2)
{

	CMapData *pWorldData = (CMapData *) _pArgument1;
	CWorld_Server *pServer = (CWorld_Server *) _pArgument2;

	TAP<CRigidBody *> pBouyancyObjects = m_lBouyancyObjects;

	CMat4Dfp64 Ttmp;
	CMat4Dfp32 T;
	CBox3Dfp32 Box;

	for (int i = 0; i < pBouyancyObjects.Len(); i++)
	{
		CWD_RigidBody *pBody = (CWD_RigidBody *) pBouyancyObjects[i];

		/*
		const CWO_PhysicsPrim& LiquidPrim = BouyancyObject.m_pLiquidObject->GetPhysState().m_Prim[0];
		CXR_Model* pLiquidModel = pWorldData->GetResource_Model(LiquidPrim.m_iPhysModel);
		const CMat4Dfp32 &LiqTransform = BouyancyObject.m_pLiquidObject->GetPositionMatrix();
		*/

		pBody->GetCorrectTransform(Ttmp);
		T = Ttmp.Getfp32();

		CWD_RigidBody::GetBoundBox(pWorldData, pBody->m_pCoreData, Box);
		Box.m_Min -= pBody->m_MassCenter.Getfp32();
		Box.m_Max -= pBody->m_MassCenter.Getfp32();

		CBouyancyFunctions::CBouyancyForce Force = CBouyancyFunctions::CalculateBouyancyForce(pServer, pBody->m_pCoreData->m_iObject, T, Box);

#if 1

		CRigidBodyState *pState = (CRigidBodyState *) pBody->GetBodyState();

		CVec3Dfp32 Size = (Box.m_Max - Box.m_Min) * (10.0f / 32.0f);
		fp32 Volume = Size.k[0] * Size.k[1] * Size.k[2];

		CVec3Dfp32 AreaX = CVec3Dfp32(Size[1] * Size[2], 0.0f, 0.0f);
		CVec3Dfp32 AreaY = CVec3Dfp32(0.0f, Size[0] * Size[2], 0.0f);
		CVec3Dfp32 AreaZ = CVec3Dfp32(0.0f, 0.0f, Size[0] * Size[1]);

		CMat4Dfp64 Ttmp;
		CMat4Dfp32 T;
		pBody->GetCorrectTransform(Ttmp);
		T = Ttmp.Getfp32();
		T.UnitNot3x3();

		AreaX *= T;
		AreaY *= T;
		AreaZ *= T;

		CVec3Dfp32 Vel = pState->m_velocity.Getfp32();
		Vel.Normalize();

		fp32 KX = M_Fabs(Vel * AreaX);
		fp32 KY = M_Fabs(Vel * AreaY);
		fp32 KZ = M_Fabs(Vel * AreaZ);

		fp32 K = KX + KY + KZ;

		pServer->Phys_AddImpulse(pBody->m_pCoreData->m_iObject, Force.m_ApplyAt, Force.m_Force);

#ifndef M_RTM
		pServer->Debug_RenderVector(Force.m_ApplyAt, CVec3Dfp32(0.0f, 0.0f, 32.0f), 0xffff0000, 1.0f/20.0f, false);
#endif

		CVec3Dfp64 PointVelocity;
		pBody->GetVelocityAt(Force.m_ApplyAt.Getfp64(), PointVelocity);
		PointVelocity -= pBody->GetVelocity();

//		pServer->Phys_AddImpulse(pBody->m_pCoreData->m_iObject, Force.m_ApplyAt, PointVelocity.Getfp32() * -0.001);


		K = Clamp(K, 0.0f, 800.0f);
		if (Force.m_Force.LengthSqr() > _FP32_EPSILON)
		{
			fp32 Factor = Force.m_VolumeInLiquid / Volume;
			Factor *= BUOYANCYLIQUIDDRAG;
			CVec3Dfp32 FrictionForce = -pState->m_velocity.Getfp32() * K * Factor;
			pServer->Phys_AddForce(pBody->m_pCoreData->m_iObject, FrictionForce);
		}

#endif

	}
}

void CWO_DynamicsCollider::PostApplyExternalForces(void *_pArgument1, void *_pArgument2)
{
	CMapData *pWorldData = (CMapData *) _pArgument1;
	CWorld_Server *pServer = (CWorld_Server *) _pArgument2;
	TAP<CRigidBody *> pBouyancyObjects = m_lBouyancyObjects;

	for (int i = 0; i < pBouyancyObjects.Len(); i++)
	{
		CWD_RigidBody *pBody = (CWD_RigidBody *) pBouyancyObjects[i];

		CRigidBodyState *pState = (CRigidBodyState *) pBody->GetBodyState();

		pState->m_angularvelocity *= 0.98;
	}
}


void CWO_DynamicsCollider::GetBoundingBox(const CRigidBody *_pRigidBody, CBox3Dfp64& _Box, void *_pArgument1, void *_pArgument2)
{
	CMapData *pWorldData = (CMapData *) _pArgument1;
	//CWorld_PhysState *pPhysState = (CWorld_PhysState *) _pArgument2;

	CWD_RigidBody *pRB = (CWD_RigidBody *) _pRigidBody;
	CBox3Dfp32 B;
	CWD_RigidBody::GetBoundBox(pWorldData, pRB->m_pCoreData, B);
	_Box.m_Min = B.m_Min.Getfp64();
	_Box.m_Max = B.m_Max.Getfp64();
}

CWO_DynamicsDebugRenderer::CWO_DynamicsDebugRenderer(CWorld_PhysState *_pPhysState)
{
	m_pWPhysState = _pPhysState;
}

void CWO_DynamicsDebugRenderer::Render(const CContactInfo& _ContactInfo)
{
	//M_TRACEALWAYS("%s\n",ConvertVector(_ContactInfo.m_PointOfCollision).GetString().Str());
	//M_TRACEALWAYS("%s\n",ConvertVector(_ContactInfo.m_Normal*64.0).GetString().Str());

	// TODO:
	// m_PointOfCollision är skalad till SI-enheter.
	// Kanske ska man inte göra det här...

//	const fp32 Duration = 1.0/10.0;
#if 1
	const fp32 Duration = 1.0 / 20.0f;
	m_pWPhysState->Debug_RenderVector(ConvertVector(_ContactInfo.m_PointOfCollision)*32.0,
									  ConvertVector(_ContactInfo.m_Normal*16.0),
									  CPixel32(220,0,0,255),Duration, false);
#endif
}

void CWO_DynamicsDebugRenderer::Render(const CRigidBody *_pRigidBody)
{
#if 0
	if (_pRigidBody->IsStationary())
	{
		CMat4Dfp64 mat;
		//_pRigidBody->GetOrientation(mat);
		_pRigidBody->GetTransform(mat);

		CMat4Dfp32 mat2 = mat.Getfp32();

		m_pWPhysState->Debug_RenderMatrix(mat2,1.0/10.0);
	}
#endif
}

#endif // USE_DYNAMICSENGINE1
