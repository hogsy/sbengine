#include "PCH.h"
#include "XRCloth.h"

//#pragma optimize( "", off )
//#pragma inline_depth(0)
//#pragma check_stack(on)

#include "XRClothCommon.h"
#include "Solids/XRThinSolid.h"
#include "../XRModels/Model_TriMesh/WTriMesh.h"		// needed for error reporting
#include "../../MCC/MRTC_VPUManager.h"

#ifndef PLATFORM_CONSOLE
# include "Solids/XWSolid.h"
#endif


//#define CLOTH_SPU_EMU
//#define CLOTH_USE_FULLBLEND



// FIX ME!
//#include <float.h>
//#include "C://Documents and Settings//christian murray//My Documents//Visual Studio Projects//Testbed//GJKSupport.h"

/*
	Blind data i maya:

	polyBlindData -id 0 -associationType "face" -longDataName "ln" -booleanData 1;
	polyColorBlindData  -ncr 0 -ncg 0 -ncb 0 -ccr 0 -ccg 1 -ccb 1 -id 0 -num 1 -m 1 -dt "boolean" -n "ln" -v "1" -cr 1 -cg 0 -cb 0;


	Vertex:
	polyColorBlindData  -ncr 1 -ncg 1 -ncb 1 -ccr 0 -ccg 1 -ccb 1 -id 0 -num 1 -m 1 -dt "int" -n "GroupID" -v "0" -cr 1 -cg 0 -cb 0



 */

/*
	- Gör inte uträkningar för ben som inte "används"

 */

// -------------------------------------------------------------------
//  CXR_SkeletonCloth
// -------------------------------------------------------------------

/*
	Artiklar:

	- HITTA DENNA! "Procedural Approach to Generate Real Time Motions of Cloth"

 */

/*
http://forum.europeanservers.net/cgi-bin/d.eur?907624

Hi,

The two main problems in real-time cloth simulation are integration and cloth-cloth collisions.
Numerical integration is difficult because differential equations we have to integrate are stiff. So explicit methods don't work if we use high spring constants. But if we use low spring constants, another problem raises: over-elongation. If you want to improve our current model, we must find some post-correction methods resolving the problem of over-elongation. Implicit methods are good but consume a lot of cpu-time.
Someone proposes us to use the verlet integrator but is-it stable for stiff equations ?

The second problem is cloth-cloth collisions. Complexity of current methods doesn't allow real time animation.

These are the two problems we work on. If some people have ideas, they can discuss them in this forum.

Thanks.
Répondre

Retour à la liste des messages


Re: Numerical integration		Tom, le 05 Aou à 13:08	
I think post-correction will never perfectly fix the over-elongation issue, but it remains the only acceptable solution for real-time. The self-collision could maybe be simplified by using an octree structure and a the cloth curvature angle. If big enough, then that means the cloth is likely to be bent enough to present self collisions. The collision response is another part of the problem :)
Répondre
Re: Numerical integration (Verlet vs. Implicit)		Mikko Kauppila, le 05 Aou à 13:10	
The Verlet method indeed is quite stable (not perfectly so, I think), but the problem is that it requires a fixed time step (I think it's possible to reconstruct the method so it supports arbitrary steps, too).

To freely quote Baraff: One of the main problems in cloth is that if you apply a pull to one corner of the cloth, the opposite corner must react to the pull *quickly*. Considering that we use the euler method and have a NxN rectangular cloth grid, the opposite corner gets no information about the pull before N euler steps. In other words, explicit techniques only have local interactions - and this applies to the Verlet method too. Explicit tehniques (and the Verlet method) are only suitable for small values of N, therefore.

Implicit techniques solve this problem by reformulating the problem into a linear system, which supports global interactions naturally.

You said implicit methods consume a lot of CPU time. Let's prove this wrong. Considering the above and the requirements for stability, the fascinating fact that explicit methods are algorithmically of complexity O(n) is destroyed (considering we use some method to reduce the time step if instability is found to arise). As pointed out by Baraff, the resulting algorithm can be even of complexity O(n^2) - or more, depending on the stiffness. On the other hand, implicit method was found to be roughly O(n^1.4).

- Mikko Kauppila
Répondre
Re(2): Numerical integration (Verlet vs. Implicit)		Sean Lynch, le 28 Dec à 21:43	
Doesn't this assume only one relaxation step per integration step? One could simply iterate the relaxation step until the amoun of motion was below a threshold, or do it enough times that the opposite end of the cloth reacts fast enough. You could have arbitrarily large cloths this way.
Répondre

 */


//#define GETBONETRANSFORM(i) _pSkelInstance->m_pBoneTransform[i]
#define GETBONETRANSFORM(i) pBoneTransform[i]


CXR_SkeletonCloth::CXR_SkeletonCloth() 
{
	m_IsCreated = false;
	m_bInvalid = false;
	m_iCloth = -1;
	m_MaxID = -1;

	m_bAdjustFloor = true;
	m_FloorOffset = 0.03f * 32.0f;

	m_BlendMode = CLOTH_SKINBLEND;

	CXR_Cloth::InitParams(m_lParams);
}


void CXR_SkeletonCloth::Create(int _iCloth,
							   const CXR_Skeleton *_pSkel,
							   CXR_SkeletonInstance* _pSkelInstance,
                               CXR_Model** _lpModels,
                               int _nModels)
{
	MSCOPESHORT(CXR_SkeletonCloth::Create);

	m_iCloth = _iCloth;

	UpdateParameters(_pSkel);

	const CXR_Cloth& Cloth = _pSkel->m_lCloth[m_iCloth];
	if (Cloth.m_lJoints.Len() == 0)
	{
		M_TRACE("WARNING: Cloth.m_lJoints.Len() == 0\n");
		return;
	}

	//	TODO: Detta är inte snyggt...
	//		Använd CIndexPool16 istället

	int maxid = Cloth.m_MaxID;
	m_MaxID = maxid;
	m_lIds = Cloth.m_lJoints;

	M_ASSERT(maxid < _pSkelInstance->m_nBoneTransform, "Cloth error!");
	M_ASSERT(maxid+1 < MAX_BONE_COUNT_IN_BITARRAY, "Cloth error!");

	m_FixedJoints.Clear();
	m_lInsideCapsuleJointMasks.SetLen(maxid+1);
	for (int i = 0; i < maxid+1; i++) 
	{
		m_lInsideCapsuleJointMasks[i] = 0;
	}
	for (int i = 0; i < Cloth.m_FixedJoints.Len(); i++) 
	{
		int iFixedJoint = Cloth.m_FixedJoints[i];
		m_FixedJoints.Set1(iFixedJoint);
	}


	m_lJointPosition.SetLen(maxid+1);
	m_lPrevJointPosition.SetLen(maxid+1);

	m_liJointVertices.SetLen(0);
	m_liJointVertices.SetLen(maxid+1);

	m_lClothBoneWeights.SetLen(maxid+1);

	m_lClosestFixedJointDistance.SetLen(maxid + 1);

	//
	// Init curr/prev bone transform arrays
	//
	m_lLastBoneTransform.SetLen(_pSkelInstance->m_nBoneTransform);
	m_lBoneTransform.SetLen(_pSkelInstance->m_nBoneTransform);
	ResetCloth(_pSkel, _pSkelInstance);

	for (int i = 0; i < Cloth.m_lClothBoneWeights.Len(); i++)
	{
		const CXR_ClothBoneWeights& ClothBoneWeight = Cloth.m_lClothBoneWeights[i];
		m_lClothBoneWeights[ClothBoneWeight.GetClothJointIndex()] = ClothBoneWeight;
	}

	m_lStructureConstraints.QuickSetLen(0);	
	m_lShearConstraints.QuickSetLen(0);
	for (int i = 0; i < Cloth.m_lConstraints.Len(); i++)
	{
		const CXR_ClothConstraint& Constraint = Cloth.m_lConstraints[i];

		if (Constraint.m_type == 0)
			m_lStructureConstraints.Add(Constraint);

		else if (Constraint.m_type == 1)
			m_lShearConstraints.Add(Constraint);
	}

	const CVec4Dfp32 MinSqrDst(_FP32_MAX,_FP32_MAX,_FP32_MAX,_FP32_MAX);
	for (int i = 0; i < m_lIds.Len(); i++)
	{
		int id = m_lIds[i];
		if (m_FixedJoints.Get(id))
		{
			m_lClosestFixedJointDistance[id] = _FP32_MAX;
			continue;
		}
		vec128 pos = _pSkel->m_lNodes[id].m_LocalCenterv;
		vec128 MinSqrDist = MinSqrDst.v;
		for (int j = 0; j < m_lIds.Len(); j++)
		{
			int id2 = m_lIds[j];
			if (m_FixedJoints.Get(id2))
			{
				vec128 pos2 = _pSkel->m_lNodes[id2].m_LocalCenterv;
				vec128 diff = M_VSub(pos,pos2);
				vec128 TmpSqrDist = M_VDp3(diff,diff);
				MinSqrDist = M_VSel(M_VSub(MinSqrDist,TmpSqrDist),TmpSqrDist,MinSqrDist);
			}
		}
		fp32 MinDist = M_VGetX_Slow(M_VSqrt(MinSqrDist));
		m_lClosestFixedJointDistance[id] = MinDist;
	}

	SetupJointVertices(_pSkel);

	// detect joint vertex errors
	TAP_RCD<int> pIds = m_lIds;
	for (int i = 0; i < pIds.Len(); i++)
	{
		if (!m_liJointVertices[pIds[i]].m_count)
		{
			ConOutL("§cf00ERROR: Invalid cloth setup!");
			M_TRACEALWAYS("ERROR: Invalid cloth setup!\n");
			for (uint j = 0; j < _nModels; j++)
			{
				CXR_Model_TriangleMesh* pTriMesh = safe_cast<CXR_Model_TriangleMesh>(_lpModels[j]);
				if (pTriMesh)
				{
					ConOutL(CStrF("§cf00 - %s", pTriMesh->m_FileName.Str()));
					M_TRACEALWAYS("- %s\n", pTriMesh->m_FileName.Str());
				}
			}
			m_bInvalid = true;
			return;
		}
	}


	{
		SetupCollisionBones(_lpModels, _nModels);
		SetupSolids(_lpModels, _nModels);
		SetupCapsules(_pSkel, _lpModels, _nModels);
	}
	// Setup inside capsule joint masks
	for (int i = 0; i < Cloth.m_InsideCapsuleJoints.Len(); i++) 
	{
		int iInsideCapsuleJoint = Cloth.m_InsideCapsuleJoints[i];
		uint64& mask = m_lInsideCapsuleJointMasks[iInsideCapsuleJoint];
		for (int j = 0;j<m_lInvertedCapsules.Len();j++)
		{
			vec128 jnt = _pSkel->m_lNodes[iInsideCapsuleJoint].m_LocalCenterv;
			TCapsule<fp32> c = m_lInvertedCapsules[j];
			vec128 a = InvertedCapsuleDelta(c,jnt);
			a = M_VDp3(a,a);
			if (M_VGetX_Slow(a) == 0.0f)
				mask |= uint64(1) << j;
		}

	}

	m_IsCreated = true;
	m_bInvalid = false;
}



void CXR_SkeletonCloth::SetupBoundingSpheres(CXR_Model **_pModel, int _nModels)
{
	m_llBoundingSpheres.SetLen(0);
	m_llBoundingSpheres.SetLen(_nModels);
	m_llBoundingSpheresTransformed.SetLen(_nModels);

	for (int i = 0; i < _nModels; i++)
	{
		int nSolids= _pModel[i]->GetParam(CXR_MODEL_PARAM_N_THINSOLIDS);
#ifndef PLATFORM_CONSOLE
		CXR_ThinSolid *pSolids= (CXR_ThinSolid *) _pModel[i]->GetParam(CXR_MODEL_PARAM_THINSOLIDS);
#endif

		m_llBoundingSpheres[i].SetLen(nSolids);
		m_llBoundingSpheresTransformed[i].SetLen(nSolids);

		for (int j = 0; j < nSolids; j++)
		{
#ifndef PLATFORM_CONSOLE
			CSolid solid;
			int nPlanes = pSolids[j].m_lPlanes.Len();
			for (int k = 0; k < nPlanes; k++)
			{
				CPlane3Dfp32 plane = pSolids[j].m_lPlanes[k];
				CPlane3Dfp64 planefp64;
				planefp64.d = plane.d;
				planefp64.n = plane.n.Getfp64();
				solid.AddPlane(planefp64);
			}
			solid.UpdateMesh();

			m_llBoundingSpheres[i][j] = CClothBoundSphere(solid.m_BoundPos, solid.m_BoundRadius);
			m_llBoundingSpheresTransformed[i][j] = CClothBoundSphere(solid.m_BoundPos, solid.m_BoundRadius);
#else
			m_llBoundingSpheres[i][j] = CClothBoundSphere(0, 0);
			m_llBoundingSpheresTransformed[i][j] = CClothBoundSphere(0, 0);
#endif
		}
	}
}

void CXR_SkeletonCloth::TransformBoundingSpheres(CXR_Model **_pModel, int _nModels, CXR_SkeletonInstance* _pSkelInstance)
{
	MSCOPESHORT(CXR_SkeletonCloth::TransformBoundingSpheres);

	CMat4Dfp32 M;

	TAP_RCD<const CMat4Dfp32> pBoneTransform = m_lBoneTransform;

	M_ASSERT(m_llBoundingSpheres.Len() == _nModels, "Cloth error!");

	for(int j = 0; j < _nModels; j++)
	{
		int nsolids = _pModel[j]->GetParam(CXR_MODEL_PARAM_N_THINSOLIDS);
		CXR_ThinSolid *pSolids= (CXR_ThinSolid *) _pModel[j]->GetParam(CXR_MODEL_PARAM_THINSOLIDS);

		CClothBoundSphere *pBoundSpheres = m_llBoundingSpheres[j].GetBasePtr();
		CClothBoundSphere *pBoundSpheresTransformed = m_llBoundingSpheresTransformed[j].GetBasePtr();

		for (int i = 0; i < nsolids; i++) 
		{
			const CXR_ThinSolid &solid= pSolids[i];
			int iNode = solid.m_iNode;
			
			CVec3Dfp32 center = pBoundSpheres[i].m_Center;
			center *= GETBONETRANSFORM(iNode);
			pBoundSpheresTransformed[i].m_Center = center;
		}
	}
}

void CXR_SkeletonCloth::SetupSolids(CXR_Model **_pModel, int _nModels)
{
	m_llSolids.SetLen(_nModels);
	m_lSolidPlanes.SetLen(0);

	for (int i = 0; i < _nModels; i++)
	{
		int nSolids= _pModel[i]->GetParam(CXR_MODEL_PARAM_N_THINSOLIDS);
		CXR_ThinSolid *pSolids= (CXR_ThinSolid *) _pModel[i]->GetParam(CXR_MODEL_PARAM_THINSOLIDS);

		m_llSolids[i].SetLen(nSolids);

		for (int j = 0; j < nSolids; j++)
		{
			int nPlanes = pSolids[j].m_lPlanes.Len();
			CClothSolid clothsolid(m_lSolidPlanes.Len(), nPlanes, pSolids[j].m_iNode);
			m_llSolids[i][j] = clothsolid;

			for (int k = 0; k < nPlanes; k++)
			{
				CPlane3Dfp32 plane = pSolids[j].m_lPlanes[k];
				m_lSolidPlanes.Add(plane);
			}
		}
	}
	m_lSolidPlanesTransformed.SetLen(m_lSolidPlanes.Len());
}

void CXR_SkeletonCloth::TransformSolids(CXR_Model **_pModel, int _nModels, CXR_SkeletonInstance* _pSkelInstance)
{
	MSCOPESHORT(CXR_SkeletonCloth::TransformSolids);

	const CMat4Dfp32* pBoneTransform = m_lBoneTransform.GetBasePtr();

	CPlane3Dfp32 *pSolidPlanes = m_lSolidPlanes.GetBasePtr();
	CPlane3Dfp32 *pSolidPlanesTransformed = m_lSolidPlanesTransformed.GetBasePtr();

	for(int j = 0; j < _nModels; j++)
	{
		CClothSolid *pClothSolids = m_llSolids[j].GetBasePtr();
		int nsolids = m_llSolids[j].Len();

		for (int i = 0; i < nsolids; i++) 
		{
			const CClothSolid& clothsolid = pClothSolids[i];
			int iNode = clothsolid.m_iNode;

			const CMat4Dfp32& M = GETBONETRANSFORM(iNode);

			int nPlanes = clothsolid.m_nPlanes;
			for (int k = 0; k < nPlanes; k++)
			{
				int planeindex = k+clothsolid.m_iFirstPlane;
				CPlane3Dfp32 plane = pSolidPlanes[planeindex];
				plane.Transform(M);
				pSolidPlanesTransformed[planeindex] = plane;
			}
		}
	}
}


void CXR_SkeletonCloth::SetupCapsules(const CXR_Skeleton* _pSkel, CXR_Model** _lpModels, int _nModels)
{
	const CXR_Cloth& cloth = _pSkel->m_lCloth[m_iCloth];
	const uint nCapsules = cloth.m_lCollisionCapsules.Len();

	TAP_RCD<const int> piCapsules = cloth.m_lCollisionCapsules;

	for (int i = 0; i < _nModels; i++)
	{
		if (_lpModels[i]->GetSkeleton() == _pSkel) // only use capsules from "our" model   (ugly!)
		{
			const uint nModelCapsules = _lpModels[i]->GetParam(CXR_MODEL_PARAM_N_CAPSULES);
			const TCapsule<fp32>* pCapsules = (TCapsule<fp32>*)_lpModels[i]->GetParam(CXR_MODEL_PARAM_CAPSULES);
			{
				uint nNormalCapsules = 0;
				uint nInvertedCapsules = 0;
				for (int j = 0; j < nCapsules; j++)
				{
					int iCapsule = piCapsules[j];
					M_ASSERTHANDLER(iCapsule < nModelCapsules, "Capsule index out of range!", continue);
					if (pCapsules[iCapsule].IsInverted())
						nInvertedCapsules++;
					else
						nNormalCapsules++;
				}
				m_lNormalCapsules.SetLen(nNormalCapsules);
				m_lNormalCapsulesTransformed.SetLen(nNormalCapsules);
				m_lInvertedCapsules.SetLen(nInvertedCapsules);
				m_lInvertedCapsulesTransformed.SetLen(nInvertedCapsules);
			}
			uint nInv = 0;
			uint nNrm = 0;
			for (int j = 0; j < nCapsules; j++)
			{
				int iCapsule = piCapsules[j];
				M_ASSERTHANDLER(iCapsule < nModelCapsules, "Capsule index out of range!", continue);
				if (pCapsules[iCapsule].IsInverted())
				{
					m_lInvertedCapsules[nInv] = pCapsules[iCapsule];
					m_lInvertedCapsulesTransformed[nInv] = pCapsules[iCapsule];
					nInv++;
				}
				else
				{
					m_lNormalCapsules[nNrm] = pCapsules[iCapsule];
					m_lNormalCapsulesTransformed[nNrm] = pCapsules[iCapsule];
					nNrm++;
				}
			}
			return;
		}
	}
	ConOutL(CStr("ERROR: SetupCapsules, no matching skeleton!"));
	m_lNormalCapsules.Clear();
	m_lNormalCapsulesTransformed.Clear();
	m_lInvertedCapsules.Clear();
	m_lInvertedCapsulesTransformed.Clear();
}



void CXR_SkeletonCloth::SetupCollisionBones(CXR_Model **_pModel, int _nModels)
{
	m_CollisionBones.Clear();

	for (int i = 0; i < _nModels; i++)
	{
#ifdef CLOTH_ENABLE_CAPSULES
		int nCapsules = _pModel[i]->GetParam(CXR_MODEL_PARAM_N_CAPSULES);
		TCapsule<fp32>* pCapsules = (TCapsule<fp32>*) _pModel[i]->GetParam(CXR_MODEL_PARAM_CAPSULES);

		for (int j = 0; j < nCapsules; j++)
			m_CollisionBones.Set1(pCapsules[j].m_UserValue);
#endif

#ifdef CLOTH_ENABLE_SOLIDS
		int nSolids= _pModel[i]->GetParam(CXR_MODEL_PARAM_N_THINSOLIDS);
		CXR_ThinSolid *pSolids= (CXR_ThinSolid *) _pModel[i]->GetParam(CXR_MODEL_PARAM_THINSOLIDS);

		for (int j = 0; j < nSolids; j++)
			m_CollisionBones.Set1(pSolids[j].m_iNode);
#endif
	}
}

void CXR_SkeletonCloth::UpdateParameters(const CXR_Skeleton *_pSkel)
{
	const CXR_Cloth& Cloth = _pSkel->m_lCloth[m_iCloth];
	memcpy(m_lParams, Cloth.m_lParams, sizeof(m_lParams));

#ifdef CLOTH_VERBOSE
	M_TRACEALWAYS("CXR_SkeletonCloth::UpdateParameters\n");
	M_TRACEALWAYS("m_iCloth = %d\n", m_iCloth);
	M_TRACEALWAYS("m_Name: %s\n", Cloth.m_Name.Str());
	M_TRACEALWAYS("m_CompressionFactor: %f\n", m_lParams[CLOTHPARAM_COMPRESSIONFACTOR]);
	M_TRACEALWAYS("m_StretchFactor: %f\n", m_lParams[CLOTHPARAM_STRETCHFACTOR]);
	M_TRACEALWAYS("m_ShearCompressionFactor: %f\n", m_lParams[CLOTHPARAM_SHEARCOMPRESSIONFACTOR]);
	M_TRACEALWAYS("m_ShearStretchFactor: %f\n", m_lParams[CLOTHPARAM_SHEARSTRETCHFACTOR]);
	M_TRACEALWAYS("m_ClothDensity: %f\n", m_lParams[CLOTHPARAM_CLOTHDENSITY]);
	M_TRACEALWAYS("m_DampFactor: %f\n", m_lParams[CLOTHPARAM_DAMPFACTOR]);
	M_TRACEALWAYS("m_nIterationSteps: %d\n", m_lParams[CLOTHPARAM_NITERATIONSTEPS]);
	M_TRACEALWAYS("m_SimulationFrequency: %f\n", m_lParams[CLOTHPARAM_SIMULATIONFREQUENCY]);
	M_TRACEALWAYS("m_ImpulsFactor: %f\n", m_lParams[CLOTHPARAM_IMPULSEFACTOR]);
	M_TRACEALWAYS("m_BlendFactor: %f\n", m_lParams[CLOTHPARAM_BLENDFACTOR]);
	M_TRACEALWAYS("m_BlendFallofFactor: %f\n", m_lParams[CLOTHPARAM_BLENDFALLOFFACTOR]);
	M_TRACEALWAYS("m_MinBlendFactor: %f\n", m_lParams[CLOTHPARAM_MINBLENDFACTOR]);
#endif

}



#ifdef PLATFORM_XENON
#define CLOTH_TRACE
#endif

#ifdef PLATFORM_XENON
#ifdef CLOTH_TRACE
bool g_bTraceCloth = false;
#endif
#endif

void CXR_SkeletonCloth::StepClothWrapper(const CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, fp32 _dt, fp32 _Blend, int32 _SimFreq)
{
#if defined(CLOTH_NAMEDEVENTS)
	M_NAMEDEVENT("ClothTaskSetup", 0x5f2040c0);
#endif
	const CXR_Cloth& Cloth = _pSkel->m_lCloth[m_iCloth];
	if (m_bInvalid || (Cloth.m_lJoints.Len() == 0))
		return;

	// need to const cast some cloth data (but mustn't be touched!)
	CXR_ClothConstraint* pClothConstraints = const_cast<CXR_ClothConstraint*>( Cloth.m_lConstraints.GetBasePtr() );
	CXR_SkeletonNode* pSkelNodes = const_cast<CXR_SkeletonNode*>( _pSkel->m_lNodes.GetBasePtr() );

	CVPU_JobDefinition JobDef;
	JobDef.AddSimpleBuffer( 0, m_lJointPosition.GetBasePtr(),m_lJointPosition.Len(),VPU_INOUT_BUFFER);
	JobDef.AddSimpleBuffer( 1, m_lParams,NUM_CLOTHPARAMS,VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer( 2, m_lShearConstraints.GetBasePtr(),m_lShearConstraints.Len(),VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer( 3, pClothConstraints, Cloth.m_lConstraints.Len(), VPU_IN_BUFFER);//kolla
	JobDef.AddSimpleBuffer( 4, m_lIds.GetBasePtr(), m_lIds.Len(), VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer( 5, &m_FixedJoints, 1, VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer( 6, m_lNormalCapsules.GetBasePtr(), m_lNormalCapsules.Len(), VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer( 7, m_lInvertedCapsules.GetBasePtr(), m_lInvertedCapsules.Len(), VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer( 8, _pSkelInstance->m_pBoneTransform, _pSkelInstance->m_nBoneTransform, VPU_INOUT_BUFFER);
	JobDef.AddSimpleBuffer( 9, m_lBoneTransform.GetBasePtr(), m_lBoneTransform.Len(), VPU_INOUT_BUFFER); //kolla
	JobDef.AddSimpleBuffer(10, m_lLastBoneTransform.GetBasePtr(), m_lLastBoneTransform.Len(), VPU_INOUT_BUFFER);
	JobDef.AddSimpleBuffer(11, &m_CollisionBones, 1, VPU_INOUT_BUFFER); //kolla
	JobDef.AddSimpleBuffer(12, pSkelNodes, _pSkel->m_lNodes.Len(), VPU_IN_BUFFER); // kolla				// This is the bones pivot in model-space
	JobDef.AddSimpleBuffer(13, m_lInsideCapsuleJointMasks.GetBasePtr(),m_lInsideCapsuleJointMasks.Len(), VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer(15, m_liJointVertices.GetBasePtr(), m_liJointVertices.Len(), VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer(16, m_lPrevJointPosition.GetBasePtr(), m_lPrevJointPosition.Len(), VPU_INOUT_BUFFER);
	JobDef.AddSimpleBuffer(17, m_lClosestFixedJointDistance.GetBasePtr(), m_lClosestFixedJointDistance.Len(), VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer(18, m_lClothBoneWeights.GetBasePtr(), m_lClothBoneWeights.Len(), VPU_IN_BUFFER); 
	CVPU_ParamData ParamData0;
	ParamData0.m_LongData[0] = uint32(m_bAdjustFloor);
	ParamData0.m_LongData[1] = uint32(m_BlendMode);
	JobDef.AddParamData(19,ParamData0);
	CVPU_ParamData ParamData1;
	ParamData1.m_fp32Data[0] = m_FloorOffset;
	ParamData1.m_fp32Data[1] = _dt;
	ParamData1.m_fp32Data[2] = _Blend;
	ParamData1.m_fp32Data[3] = _SimFreq;
	JobDef.AddParamData(20,ParamData1);
	JobDef.SetJob(MHASH2('CLOT','H'));
	_pSkelInstance->m_VpuTaskId = MRTC_ThreadPoolManager::VPU_AddTask(JobDef,VpuWorkersContext);
//	MRTC_ThreadPoolManager::VPU_BlockOnTask(_pSkelInstance->m_VpuTaskId,VpuWorkersContext);
}

int foocounter = 0;
void CXR_SkeletonCloth::Step(CXR_Model **_pModel, int _nModels, 
							 const CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, 
							 fp32 _dt, fp32 _Blend, int32 _SimFreq, void* _pPhysState)
{
	MSCOPESHORT(CXR_SkeletonCloth::Step);

	foocounter++;

	
	CMTime Timer;
	{
		TMeasure(Timer);

#if !defined(CLOTH_SPU_EMU) 
		StepClothWrapper(_pSkel,_pSkelInstance,_dt,_Blend,_SimFreq);
#else

		CClothWrapper CW;

		CW.m_nJointPosition = m_lJointPosition.Len();
		CW.m_pJointPosition = m_lJointPosition.GetBasePtr();
		CW.m_nParam = NUM_CLOTHPARAMS;
		CW.m_pParams = m_lParams;
		CW.m_nShearConstraints = m_lShearConstraints.Len();
		CW.m_pShearConstraints = m_lShearConstraints.GetBasePtr();
		CW.m_nSkeletonConstraints = _pSkel->m_lCloth[m_iCloth].m_lConstraints.Len();
		CW.m_pSkeletonConstraints = const_cast<CXR_ClothConstraint*>( _pSkel->m_lCloth[m_iCloth].m_lConstraints.GetBasePtr() );
		CW.m_nIds = m_lIds.Len();
		CW.m_pIds = m_lIds.GetBasePtr();
		CW.m_pFixedJoints = &m_FixedJoints;
		CW.m_nNormalCapsules = m_lNormalCapsules.Len();
		CW.m_pNormalCapsules = m_lNormalCapsules.GetBasePtr();
		CW.m_pNormalCapsulesTransformed = m_lNormalCapsulesTransformed.GetBasePtr();
		CW.m_nInvertedCapsules = m_lInvertedCapsules.Len();
		CW.m_pInvertedCapsules = m_lInvertedCapsules.GetBasePtr();
		CW.m_pInvertedCapsulesTransformed = m_lInvertedCapsulesTransformed.GetBasePtr();
		CW.m_nLastBoneTransform = m_lLastBoneTransform.Len();
		CW.m_pLastBoneTransform = m_lLastBoneTransform.GetBasePtr();
		CW.m_nBoneTransform = m_lBoneTransform.Len();
		CW.m_pBoneTransform = m_lBoneTransform.GetBasePtr();
		CW.m_nSkelBoneTransform = _pSkelInstance->m_nBoneTransform;
		CW.m_pSkelBoneTransform = _pSkelInstance->m_pBoneTransform;
		CW.m_pCollisionBones = &m_CollisionBones;
		CW.m_nSkelNodeDatas = _pSkel->m_lNodes.Len();
		CW.m_pSkelNodeDatas = const_cast<CXR_SkeletonNode*>( _pSkel->m_lNodes.GetBasePtr() );
		CW.m_niJointVerticies = m_liJointVertices.Len();
		CW.m_piJointVerticies = m_liJointVertices.GetBasePtr();
		CW.m_nPrevJointPosition = m_lPrevJointPosition.Len();
		CW.m_pPrevJointPosition = m_lPrevJointPosition.GetBasePtr();
		CW.m_nClothBoneWeights = m_lClothBoneWeights.Len();
		CW.m_pClothBoneWeights = m_lClothBoneWeights.GetBasePtr();
		CW.m_nClosestFixedJointDistance = m_lClosestFixedJointDistance.Len();
		CW.m_pClosestFixedJointDistance = m_lClosestFixedJointDistance.GetBasePtr();
		CW.m_nInsideCapsuleJointMasks = m_lInsideCapsuleJointMasks.Len();
		CW.m_pInsideCapsuleJointMasks = m_lInsideCapsuleJointMasks.GetBasePtr();

		CW.m_dt = _dt;
		CW.m_Blend = _Blend;
		CW.m_SimFreq = _SimFreq;

		CW.m_BlendMode = m_BlendMode;
		CW.m_bAdjustFloor = m_bAdjustFloor;
		CW.m_FloorOffset = m_FloorOffset;

		CW.Step();
#endif
	}

/*
	fp32 MS  = Timer.GetTime() * 1.0e3f;
	if (foocounter > 2000)
		M_TRACEALWAYS("Cloth %d: %f\n",m_iCloth, MS);

	if (foocounter > 2020)
		foocounter = 0;
*/
}



/* not used
static CVec3Dfp32 DebugTest(const TCapsule<fp32>& capsule, const CVec3Dfp32& point)
{


	CVec3Dfp32 ab = capsule.m_Point2 - capsule.m_Point1;
	fp32 t = (point - capsule.m_Point1) * ab;
	{
		fp32 denom = ab * ab;
		fp32 invdenom = 1.0f / denom;
		//			M_ASSERT(M_Fabs(invdenom - capsule.m_InvSqrDistance) < 0.01f, "error!");
	}
	t = Clamp01(t * capsule.m_InvSqrDistance);

	CVec3Dfp32 closest = capsule.m_Point1 + ab * t;
	CVec3Dfp32 CollisionNormal = closest - point;

	fp32 DistanceSqr = CollisionNormal.LengthSqr();
	fp32 radius = capsule.m_Radius;// + 32.0f * 0.0f;

	CollisionNormal *= (radius * M_InvSqrt(DistanceSqr)) - 1.0f;   // CollisionNormal.Normalize();   CollisionNormal *= (radius - Distance);

	if (DistanceSqr <= Sqr(radius))
	{
		return CollisionNormal;
	}
	else
	{
		return CVec3Dfp32(0,0,0);
	}
}
*/

CVec3Dfp32 CXR_SkeletonCloth::NewellNormalApproximation(const CXR_Skeleton* _pSkel, const TAP_RCD<int> _piVertices)
{
	CVec3Dfp32 normal(0,0,0);
	CVec3Dfp32 refpnt(0,0,0);

	TAP_RCD<const CXR_SkeletonNode> pNodes = _pSkel->m_lNodes;

	int nvert = _piVertices.Len();
	for (int i = 0; i < nvert; i++)
	{
		CVec3Dfp32Aggr u = pNodes[_piVertices[i]].m_LocalCenter;
		CVec3Dfp32Aggr v = pNodes[_piVertices[(i+1) % nvert]].m_LocalCenter;

		normal.k[0] += (u.k[1] - v.k[1]) * (u.k[2] + v.k[2]);
		normal.k[1] += (u.k[2] - v.k[2]) * (u.k[0] + v.k[0]);
		normal.k[2] += (u.k[0] - v.k[0]) * (u.k[1] + v.k[1]);		

		refpnt += u;
	}

	normal.Normalize();
	return normal;
}

CVec3Dfp32 CXR_SkeletonCloth::NewellClothNormalApproximation(const CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, const TAP_RCA<int>& _ivertices)
{
	CVec3Dfp32 normal(0,0,0);
	CVec3Dfp32 refpnt(0,0,0);

	int nvert = _ivertices.Len();
	for (int i = 0; i<nvert; i++)
	{
		CVec3Dfp32 u = (CVec3Dfp32 &) m_lJointPosition[_ivertices[i]];
		CVec3Dfp32 v = (CVec3Dfp32 &) m_lJointPosition[_ivertices[(i+1) % nvert]];

		normal.k[0] += (u.k[1] - v.k[1]) * (u.k[2] + v.k[2]);
		normal.k[1] += (u.k[2] - v.k[2]) * (u.k[0] + v.k[0]);
		normal.k[2] += (u.k[0] - v.k[0]) * (u.k[1] + v.k[1]);		

		refpnt += u;
	}

	normal.Normalize();
	return normal;
}

/* not used
CVec3Dfp32 CXR_SkeletonCloth::NewellSkeletonNormalApproximation(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, const TAP_RCA<int>& _ivertices)
{
	MSCOPESHORT(CXR_SkeletonCloth::NewellSkeletonNormalApproximation);

	CVec3Dfp32 normal(0,0,0);
	CVec3Dfp32 refpnt(0,0,0);

	CMat4Dfp32 *pBoneTransform = m_lBoneTransform.GetBasePtr();

	int nvert = _ivertices.Len();
	for (int i = 0; i<nvert; i++)
	{
		CVec3Dfp32 u = _pSkel->m_lNodes[_ivertices[i]].m_LocalCenter;
		CVec3Dfp32 v = _pSkel->m_lNodes[_ivertices[(i+1) % nvert]].m_LocalCenter;

		CMat4Dfp32 &mu = GETBONETRANSFORM(_ivertices[i]);
		CMat4Dfp32 &mv = GETBONETRANSFORM(_ivertices[(i+1) % nvert]);

		u *= mu;
		v *= mv;

		normal.k[0] += (u.k[1] - v.k[1]) * (u.k[2] + v.k[2]);
		normal.k[1] += (u.k[2] - v.k[2]) * (u.k[0] + v.k[0]);
		normal.k[2] += (u.k[0] - v.k[0]) * (u.k[1] + v.k[1]);		

		refpnt += u;
	}

	normal.Normalize();
	return normal;
}
*/

template<class T>
static void ReverseList(TAP_RCD<T> _list)
{
	int n = _list.Len();
	for (int i = 0; i<n/2; i++)
	{
		T tmp = _list[i];
		_list[i] = _list[n-i-1];
		_list[n-i-1] = tmp;
	}
}

#define FIRSTCLOTHNODE 100

void CXR_SkeletonCloth::SetupJointVertices(const CXR_Skeleton* _pSkel)
{
	for (int i = 0; i<m_lIds.Len(); i++)
	{
		int jointindex = m_lIds[i];

		TStaticArray<int, 10> neighbours;

		GetNeighbourJoints(_pSkel, jointindex, neighbours);

		SortCyclicOrder(_pSkel, neighbours);
		CVec3Dfp32 normal = NewellNormalApproximation(_pSkel,neighbours);
		for (int j = 0; j<neighbours.Len(); j++)
		{
			int index = neighbours[j];
			if (index == jointindex) continue;
			const int jcount = m_liJointVertices[index].m_count;
			if (jcount > 0)
			{
				TAP_RCD<int> ijv;
				ijv.Set(m_liJointVertices[index].m_iJointVerts,jcount);
				CVec3Dfp32 neighbournormal = NewellNormalApproximation(_pSkel, ijv);
				if (neighbournormal * normal < 0)
				{
					ReverseList(TAP_RCD<int>(neighbours));
				}
				break;
			}
		}
		m_liJointVertices[jointindex].m_count = neighbours.Len();
		memcpy(m_liJointVertices[jointindex].m_iJointVerts, neighbours.GetBasePtr(), neighbours.Len() * sizeof(int));
	}
}

void CXR_SkeletonCloth::SortCyclicOrder(const CXR_Skeleton* _pSkel, TStaticArray<int, 10>& _list)
{
	if (_list.Len() == 0)
		return;

	CVec3Dfp32 center(0,0,0);
	CVec3Dfp32 minv(_FP32_MAX,_FP32_MAX,_FP32_MAX);
	CVec3Dfp32 maxv(_FP32_MIN,_FP32_MIN,_FP32_MIN);

	for (int i = 0; i < _list.Len(); i++) 
	{
		CVec3Dfp32 p = _pSkel->m_lNodes[_list[i]].m_LocalCenter;
		center += p;
		minv.k[0] = Min(minv.k[0], p.k[0]);
		minv.k[1] = Min(minv.k[1], p.k[1]);
		minv.k[2] = Min(minv.k[2], p.k[2]);

		maxv.k[0] = Max(maxv.k[0], p.k[0]);
		maxv.k[1] = Max(maxv.k[1], p.k[1]);
		maxv.k[2] = Max(maxv.k[2], p.k[2]);
	}
	int minaxis = 0;
	int axis1 = 1;
	int axis2 = 2;
	fp64 mindist = fabs(maxv.k[0] - minv.k[0]);

	if (fabs(maxv.k[1] - minv.k[1]) < mindist) 
	{
		mindist = fabs(maxv.k[1] - minv.k[1]) < mindist;
		minaxis = 1;
		axis1 = 0;
		axis2 = 2;
	}
	if (fabs(maxv.k[2] - minv.k[2]) < mindist) 
	{
		minaxis = 2;
		axis1 = 0;
		axis2 = 1;
	}

	center *= 1.0f/_list.Len();

	TArray<int> l;
	l.SetLen(_list.Len());
	for (int i = 0; i < _list.Len(); i++)
	{
		l[i] = _list[i];
	}
	_list.SetLen(0);

	do
	{
		fp32 shortestangle = _FP32_MAX;
		int iclosest = -1;		

		for (int i = 0; i<l.Len(); i++)
		{
			int searchindex = l[i];

			fp32 x = _pSkel->m_lNodes[searchindex].m_LocalCenter[axis1] - center.k[axis1];
			fp32 y = _pSkel->m_lNodes[searchindex].m_LocalCenter[axis2] - center.k[axis2];

			fp32 a = atan2(x,y);

			if (a < shortestangle)
			{
				shortestangle = a;
				iclosest = i;
			}
		}
		if (iclosest != -1)
		{
			int closest = l[iclosest];
			_list.Add(closest);
			l.Del(iclosest);
		}
	}
	while (l.Len() > 0);
}

void CXR_SkeletonCloth::GetNeighbourJoints(const CXR_Skeleton* _pSkel, int _iJoint, TStaticArray<int, 10>& _liNeighbours)
{
	/*
	TODO: Det verkar kunna bli duplikat här.
	Varför, fixa? Kan ha göra med felaktigt uppsatta skelett i Maya.
	*/

	const CXR_Cloth& cloth = _pSkel->m_lCloth[m_iCloth];
	_liNeighbours.SetLen(0);
	TAP_RCD<const CXR_ClothConstraint> pConstraints = cloth.m_lConstraints;
	for (int i = 0; i < pConstraints.Len(); i++)
	{
		const CXR_ClothConstraint& constraint = pConstraints[i];

		if (constraint.m_type == 0) 
		{
			if (constraint.m_id1 == _iJoint)
			{
				_liNeighbours.Add(constraint.m_id2);
			}
			else if (constraint.m_id2 == _iJoint)
			{
				_liNeighbours.Add(constraint.m_id1);
			}	
		}
	}
}


void CXR_SkeletonCloth::ResetCloth(const CXR_Skeleton* _pSkel, const CXR_SkeletonInstance* _pSkelInst)
{
	const CMat4Dfp32* M_RESTRICT pBoneTransform = _pSkelInst->m_pBoneTransform;
	uint nBoneTransform = _pSkelInst->m_nBoneTransform;

	for (uint i = 0; i < nBoneTransform; i++)
	{
		m_lLastBoneTransform[i] = pBoneTransform[i];		
		m_lBoneTransform[i] = pBoneTransform[i];
	}

	TAP<int> pIds = m_lIds;
	for (uint i = 0; i < pIds.Len(); i++)
	{				
		int id = pIds[i];
		const CXR_SkeletonNode& Node = _pSkel->m_lNodes[id];
		CVec3Dfp32 pt = Node.m_LocalCenter * pBoneTransform[id];
		m_lJointPosition[id] = pt;
		m_lPrevJointPosition[id] = pt;
	}
}


