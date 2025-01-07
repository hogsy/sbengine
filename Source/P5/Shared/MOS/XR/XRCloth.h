#ifndef _INC_XRCloth
#define _INC_XRCloth

#include "../../Shared/mos/xr/XRSkeleton.h"
#include "XRAnimData.h"


//#define CLOTH_DISABLE
		


class CClothBoundSphere
{
public:
	CClothBoundSphere()
	{
	}

	CClothBoundSphere(const CVec3Dfp32 _Center, fp32 _Radius)
	{
		m_Center = _Center;
		m_Radius = _Radius;
	}

	CVec3Dfp32 m_Center;
	fp32 m_Radius;
};

class CClothSolid
{
public:
	CClothSolid()
	{
		m_iFirstPlane = ~0;
		m_nPlanes = ~0;
		m_iNode = ~0;
	}

	CClothSolid(uint16 _iFirstPlane, uint16 _nPlanes, uint16 _iNode)
	{
		m_iFirstPlane = _iFirstPlane;
		m_nPlanes = _nPlanes;
		m_iNode = _iNode;
	}

	uint16 m_iFirstPlane;
	uint16 m_nPlanes;
	uint16 m_iNode;
};

class CXR_SkeletonCloth
{
public:

	enum IterationOrder
	{
		UpOrder,
		DownOrder
	};

	CXR_SkeletonCloth();

	void SetBlendMode(ClothBlendMode _BlendMode)
	{
		m_BlendMode = _BlendMode;
	}

	ClothBlendMode GetBlendMode() const
	{
		return m_BlendMode;
	}

	void Create(int _iCloth,
				const CXR_Skeleton* _pSkel, 
				CXR_SkeletonInstance* _pSkelInstance,
				CXR_Model** _lpModels,
				int _nModels);

	void UpdateParameters(const CXR_Skeleton *_pSkel);

	void Reset()
	{
		m_FirstIteration = true;
	}

	void Step(CXR_Model** _lpModels, int _nModels, 
			  const CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, 
			  fp32 _dt,
			  fp32 _Blend = -1.0f,
			  int32 _SimFreq = -1,
			  void *_pPhysState = NULL);
	void StepClothWrapper(const CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, fp32 _dt, fp32 _Blend, int32 _SimFreq);

	void GetNeighbourJoints(const CXR_Skeleton* _pSkel, int _iJoint, TStaticArray<int, 10>& _liNeighbours);
	void SetupJointVertices(const CXR_Skeleton* _pSkel);

	void SortCyclicOrder(const CXR_Skeleton* _pSkel, TStaticArray<int, 10>& _list);
	CVec3Dfp32 NewellNormalApproximation(const CXR_Skeleton* _pSkel, const TAP_RCD<int> _piVertices);
	CVec3Dfp32 NewellClothNormalApproximation(const CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, const TAP_RCA<int>& _ivertices);
	CVec3Dfp32 NewellSkeletonNormalApproximation(const CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, const TAP_RCA<int>& _ivertices);

	// Resets the cloth state
	void ResetCloth(const CXR_Skeleton* _pSkel, const CXR_SkeletonInstance* _pSkelInst);

	CClothWrapper m_CW;
	TArray<CXR_ClothConstraint> m_lStructureConstraints;
	TArray<CXR_ClothConstraint> m_lShearConstraints;

	TThinArrayAlign<int,16> m_lIds;
	TBitArray<MAX_BONE_COUNT_IN_BITARRAY> M_ALIGN(16) m_FixedJoints;
	TBitArray<MAX_BONE_COUNT_IN_BITARRAY> M_ALIGN(16) m_CollisionBones;

	TArray<CVec4Dfp32> m_lJointPosition;

	TThinArrayAlign<fp32,16> m_lClosestFixedJointDistance;

	TArray<CVec4Dfp32> m_lPrevJointPosition;

	TArray<CMat4Dfp32> m_lBoneTransform;
	TArray<CMat4Dfp32> m_lLastBoneTransform;

	TArray<CMat4Dfp32> m_lReferenceSystemInverted;

	TArray<CMat4Dfp32> m_lBoneVelocity;
	TArray<CVec3Dfp32> m_lAngularBoneVelocity;

	TArray<CJointVertsIndicies> m_liJointVertices;

	// Skinning
	TArray<CXR_ClothBoneWeightsData> m_lClothBoneWeights;

	// Bounding sphere test
	TArray<TArray<CClothBoundSphere> > m_llBoundingSpheres;
	TArray<TArray<CClothBoundSphere> > m_llBoundingSpheresTransformed;

	TArray<TCapsule<fp32> >	m_lNormalCapsules;
	TArray<TCapsule<fp32> >	m_lNormalCapsulesTransformed;
	TArray<TCapsule<fp32> >	m_lInvertedCapsules;
	TArray<TCapsule<fp32> >	m_lInvertedCapsulesTransformed;
	TThinArrayAlign<uint64,16>		m_lInsideCapsuleJointMasks;
	

	void SetupCapsules(const CXR_Skeleton* _pSkel, CXR_Model** _lpModels, int _nModels);
	void TransformCapsules(CXR_Model** _lpModels, int _nModels, CXR_SkeletonInstance* _pSkelInstance);

	void SetupCollisionBones(CXR_Model **_pModel, int _nModels);

	void SetupBoundingSpheres(CXR_Model **_pModel, int _nModels);
	void TransformBoundingSpheres(CXR_Model **_pModel, int _nModels, CXR_SkeletonInstance* _pSkelInstance);
	// Bounding sphere test

	TArray<TArray<CClothSolid> > m_llSolids;
	TArray<CPlane3Dfp32> m_lSolidPlanes;
	TArray<CPlane3Dfp32> m_lSolidPlanesTransformed;
	void SetupSolids(CXR_Model **_pModel, int _nModels);
	void TransformSolids(CXR_Model **_pModel, int _nModels, CXR_SkeletonInstance* _pSkelInstance);

	void SetAdjustFloor(bool _bAdjustFloor) 
	{
		m_bAdjustFloor = _bAdjustFloor;
	}

	bool GetAdjustFloor() const
	{
		return m_bAdjustFloor;
	}

	void SetFloorOffset(fp32 _Offset)
	{
		m_FloorOffset = _Offset;
	}

	fp32 GetFloorOffset() const
	{
		return m_FloorOffset;
	}

	fp32 m_FloorOffset;
	bool m_bAdjustFloor;
	bool m_FirstIteration;
	bool m_IsCreated;
	bool m_bInvalid;
	int m_iCloth;
	int m_MaxID;

	fp32 M_ALIGN(16) m_lParams[NUM_CLOTHPARAMS];

	CMTime m_LastUpdate;
	CMat4Dfp32 m_LastUpdatePosition;

	ClothBlendMode m_BlendMode;
};


#endif
