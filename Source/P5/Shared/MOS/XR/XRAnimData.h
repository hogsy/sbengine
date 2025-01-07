#ifndef _INC_XRAMIMDATA
#define _INC_XRAMIMDATA

enum
{
	CLOTHPARAM_COMPRESSIONFACTOR = 0,
	CLOTHPARAM_STRETCHFACTOR,
	CLOTHPARAM_SHEARCOMPRESSIONFACTOR,
	CLOTHPARAM_SHEARSTRETCHFACTOR,
	CLOTHPARAM_CLOTHDENSITY,
	CLOTHPARAM_DAMPFACTOR,
	CLOTHPARAM_NITERATIONSTEPS,
	CLOTHPARAM_SIMULATIONFREQUENCY,
	CLOTHPARAM_IMPULSEFACTOR,
	CLOTHPARAM_BLENDFACTOR,
	CLOTHPARAM_BLENDFALLOFFACTOR,
	CLOTHPARAM_MINBLENDFACTOR,

	NUM_CLOTHPARAMS,
};


class CXR_ClothConstraintData
{
public:
	union
	{
		struct  
		{
			fp32 m_bFixed1, m_bFixed2;
			fp32 m_restlength;
			fp32 __pad__;
		};

		vec128 m_bFixed1_bFixed2_m_RestLength;							//16
	};
	fp64 m_weight;														//96
	int16 m_id1, m_id2;													//104
	int16 m_type;														//108

};

class CXR_ClothBoneWeightsData
{
protected:
	enum { MAX_BONE_COUNT = 8 };
	union 
	{
		fp32 m_Weights[MAX_BONE_COUNT];
		vec128 m_Weightsv[2];
	};
	union 
	{
		int16 m_iBoneIndex[MAX_BONE_COUNT];
		vec128 m_iBoneIndexv;
	};
	uint16 m_BoneCount;
	int16 m_iClothJoint;

	M_FORCEINLINE bool LegalIndex(uint _Index) const { return (m_BoneCount <= MAX_BONE_COUNT) && (_Index < m_BoneCount); }

public:
	CXR_ClothBoneWeightsData()
		: m_BoneCount(0)
		, m_iClothJoint(-1)
	{ }

	CXR_ClothBoneWeightsData(const CXR_ClothBoneWeightsData& c)
	{
		m_Weightsv[0] = c.m_Weightsv[0];
		m_Weightsv[1] = c.m_Weightsv[1];
		m_iBoneIndexv = c.m_iBoneIndexv;
		m_BoneCount = c.m_BoneCount;
		m_iClothJoint = c.m_iClothJoint;
	}

	M_FORCEINLINE uint GetBoneCount() const { return m_BoneCount; }
	M_FORCEINLINE int16 GetClothJointIndex() const { return m_iClothJoint; }
	M_FORCEINLINE void SetClothJointIndex(int16 iClothJoint) { m_iClothJoint = iClothJoint; }
	M_FORCEINLINE int16 GetBoneIndex(uint _Index) const { M_ASSERT(LegalIndex(_Index),""); return m_iBoneIndex[_Index]; }
	M_FORCEINLINE fp32 GetBoneWeight(uint _Index) const { M_ASSERT(LegalIndex(_Index),""); return m_Weights[_Index]; }

	M_FORCEINLINE void Add(fp32 _Weight, int16 _BoneIndex)
	{
		if (m_BoneCount >= MAX_BONE_COUNT)
		{			
			fp32 MinWeight = _FP32_MAX;
			uint16 MWsIndex = ~0;
			for (int i = 0; i < 8; i++)
			{
				if (m_Weights[i] < MinWeight)
				{
					MinWeight = m_Weights[i];
					MWsIndex = m_iBoneIndex[i];
				}
				m_Weights[MWsIndex] = _Weight;
				m_iBoneIndex[MWsIndex] = _BoneIndex;
			}
		}
		else
		{
			m_Weights[m_BoneCount] = _Weight;
			m_iBoneIndex[m_BoneCount] = _BoneIndex;
			m_BoneCount++;
		}
	}
};


class CXR_SkeletonNodeData
{
public:
	union 
	{
		CVec3Dfp32Aggr m_LocalCenter;				// This is the bones pivot in model-space.
		vec128 m_LocalCenterv;
	};
	fp32 m_RotationScale;
	fp32 m_MovementScale;
	uint16 m_Flags;
	int16 m_iiNodeChildren;
	int16 m_iNodeParent;
	int16 m_iRotationSlot;
	int16 m_iMovementSlot;
	uint8 m_nChildren;
	mutable uint8 m_iTrackMask;
};

class CJointVertsIndicies
{
public:
	enum { MaxIndexCount=10 };
	CJointVertsIndicies()
	{
		m_count=0;
	}
	void Add(int index)
	{
		if (m_count>=MaxIndexCount)
			return;
		m_iJointVerts[m_count]=index;
		m_count++;
	}
	int m_count;
	int m_iJointVerts[MaxIndexCount];
};

enum ClothBlendMode
{
	CLOTH_SKINBLEND,
	CLOTH_ANIMATIONBLEND,
};

enum { MAX_BONE_COUNT_IN_BITARRAY=512 };

class CClothWrapper
{
public:
	uint32 m_nJointPosition;
	CVec4Dfp32 *m_pJointPosition;
	uint32 m_nParam;
	fp32 *m_pParams;
	uint32 m_nShearConstraints;
	CXR_ClothConstraintData *m_pShearConstraints;
	uint32 m_nSkeletonConstraints;
	CXR_ClothConstraintData *m_pSkeletonConstraints;
	uint32 m_nIds;
	int* m_pIds;
	uint32 m_nNormalCapsules;
	TCapsule<fp32>* m_pNormalCapsules;
	TCapsule<fp32>* m_pNormalCapsulesTransformed;
	uint32 m_nInvertedCapsules;
	TCapsule<fp32>* m_pInvertedCapsules;
	TCapsule<fp32>* m_pInvertedCapsulesTransformed;
	uint32 m_nLastBoneTransform;
	CMat4Dfp32* m_pLastBoneTransform;
	uint32 m_nBoneTransform;
	CMat4Dfp32* m_pBoneTransform;
	uint32 m_nSkelBoneTransform;
	CMat4Dfp32* m_pSkelBoneTransform;
	TBitArray<MAX_BONE_COUNT_IN_BITARRAY>* m_pFixedJoints;
	TBitArray<MAX_BONE_COUNT_IN_BITARRAY>* m_pCollisionBones;
	uint32 m_nSkelNodeDatas;
	CXR_SkeletonNodeData* m_pSkelNodeDatas;				// This is the bones pivot in model-space.
	uint32 m_niJointVerticies;
	CJointVertsIndicies* m_piJointVerticies;
	uint32 m_nPrevJointPosition;
	CVec4Dfp32* m_pPrevJointPosition;
	uint32 m_nClothBoneWeights;
	CXR_ClothBoneWeightsData* m_pClothBoneWeights;
	uint32 m_nClosestFixedJointDistance;
	fp32* m_pClosestFixedJointDistance;
	uint32 m_nInsideCapsuleJointMasks;
	uint64* m_pInsideCapsuleJointMasks;
	fp32 m_dt;
	fp32 m_Blend;
	fp32 m_SimFreq;

	ClothBlendMode m_BlendMode;
	bool m_bAdjustFloor;
	fp32 m_FloorOffset;

	M_FORCEINLINE void AdjustLength4(int _ConstraintType);
	M_FORCEINLINE void AdjustFloorConstraint(fp32 _FloorOffset);
	M_FORCEINLINE void AdjustVertexCapsuleConstraintTransformed(int _inode);
	M_FORCEINLINE void AdjustVertexCapsuleConstraintTransformed2(int _inode1, int _inode2);
	M_FORCEINLINE void InterpolateSkeleton(fp32 _Fraction);
	M_FORCEINLINE void TransformCapsules();
	M_FORCEINLINE void Integrate (fp32 _Damp,fp64 _dt,fp32 _Fraction,int _iStep,fp64 *_MeanSqError);
	M_FORCEINLINE void PostIntegrate();
	M_FORCEINLINE vec128 GetWorldPosition(int _ijoint);
	M_FORCEINLINE void SkinClothBones(fp32 _Blend, bool _Usefalloff);
	M_FORCEINLINE void Step();
};



#endif // _INC_XRAMIMDATA
