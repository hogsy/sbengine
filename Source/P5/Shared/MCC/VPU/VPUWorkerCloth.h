#include "MRTC_VPU.h"

//#pragma optimize( "", off )
//#pragma inline_depth(0)

#include "../../Mos/XR/XRAnimData.h"
#include "../../Mos/XR/XRClothCommon.h"



uint32 VPUWorker_Cloth(CVPU_JobInfo& _JobInfo)
{
//	M_TRACE("Cloth VPU: Memory left: %d\r\n", MRTC_System_VPU::OS_LocalHeapMemLeft());
	/*
	JobDef.AddSimpleBuffer( 0, m_lJointPosition.GetBasePtr(),m_lJointPosition.Len(),VPU_INOUT_BUFFER);
	JobDef.AddSimpleBuffer( 1, m_lParams,NUM_CLOTHPARAMS,VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer( 2, m_lShearConstraints.GetBasePtr(),m_lShearConstraints.Len(),VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer( 3, _pSkel->m_lCloth[m_iCloth].m_lConstraints.GetBasePtr(),_pSkel->m_lCloth[m_iCloth].m_lConstraints.Len(), VPU_IN_BUFFER);//kolla
	JobDef.AddSimpleBuffer( 4, m_lIds.GetBasePtr(), m_lIds.Len(), VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer( 5, m_lFixedJoints.GetBasePtr(), m_lFixedJoints.Len(), VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer( 6, m_lNormalCapsules.GetBasePtr(), m_lNormalCapsules.Len(), VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer( 7, m_lInvertedCapsules.GetBasePtr(), m_lInvertedCapsules.Len(), VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer( 8, _pSkelInstance->m_pBoneTransform, _pSkelInstance->m_nBoneTransform, VPU_INOUT_BUFFER);
	JobDef.AddSimpleBuffer( 9, m_lBoneTransform.GetBasePtr(), m_lBoneTransform.Len(), VPU_INOUT_BUFFER); //kolla
	JobDef.AddSimpleBuffer(10, m_lLastBoneTransform.GetBasePtr(), m_lLastBoneTransform.Len(), VPU_INOUT_BUFFER);
	JobDef.AddSimpleBuffer(11, m_lCollisionBones.GetBasePtr(), m_lCollisionBones.Len(), VPU_INOUT_BUFFER); //kolla
	JobDef.AddSimpleBuffer(12, _pSkel->m_lNodes.GetBasePtr(), _pSkel->m_lNodes.Len(), VPU_INOUT_BUFFER); // kolla				// This is the bones pivot in model-space
	JobDef.AddSimpleBuffer(13, m_lInsideCapsuleJointMasks.GetBasePtr(),m_lInsideCapsuleJointMasks.Len(), VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer(15, m_liJointVertices.GetBasePtr(), m_liJointVertices.Len(), VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer(16, m_lPrevJointPosition.GetBasePtr(), m_lPrevJointPosition.Len(), VPU_INOUT_BUFFER);
	JobDef.AddSimpleBuffer(17, m_lClosestFixedJointDistance.GetBasePtr(), m_lClosestFixedJointDistance.Len(), VPU_IN_BUFFER);
	JobDef.AddSimpleBuffer(18, m_lClothBoneWeights.GetBasePtr(), m_lClothBoneWeights.Len(), VPU_IN_BUFFER); 
	CVPU_ParamData ParamData0;
	ParamData0.m_LongData[0]=uint32(m_bAdjustFloor);
	ParamData0.m_LongData[1]=uint32(m_BlendMode);
	JobDef.AddParamData(19,ParamData0);
	CVPU_ParamData ParamData1;
	ParamData1.m_fp32Data[0]=m_FloorOffset;
	ParamData1.m_fp32Data[1]=_dt;
	ParamData1.m_fp32Data[2]=_Blend;
	ParamData1.m_fp32Data[3]=_SimFreq;
	JobDef.AddParamData(20,ParamData1);
	*/
#if defined(VPU_NAMEDEVENTS) && defined(CLOTH_NAMEDEVENTS)
	M_NAMEDEVENT("VPUWorker_Cloth", 0xff0040c0);
#endif
	CVPU_SimpleBuffer<CVec4Dfp32> JointPositionBuffer(_JobInfo.GetSimpleBuffer(0));
	CVPU_SimpleBuffer<fp32> ParamBuffer(_JobInfo.GetSimpleBuffer(1));
	CVPU_SimpleBuffer<CXR_ClothConstraintData> ShearConstraintBuffer(_JobInfo.GetSimpleBuffer(2));
	CVPU_SimpleBuffer<CXR_ClothConstraintData> SkeletonConstraintBuffer(_JobInfo.GetSimpleBuffer(3));
	CVPU_SimpleBuffer<int> IdBuffer(_JobInfo.GetSimpleBuffer(4));
	CVPU_SimpleBuffer<TBitArray<MAX_BONE_COUNT_IN_BITARRAY> > FixedJointsBuffer(_JobInfo.GetSimpleBuffer(5));
	CVPU_SimpleBuffer<TCapsule<fp32> > NormalCapsulesBuffer(_JobInfo.GetSimpleBuffer(6));
	CVPU_SimpleBuffer<TCapsule<fp32> > InvertedCapsulesBuffer(_JobInfo.GetSimpleBuffer(7));
	CVPU_SimpleBuffer<CMat4Dfp32> SkelBoneTransformBuffer(_JobInfo.GetSimpleBuffer(8));
	CVPU_SimpleBuffer<CMat4Dfp32> BoneTransformBuffer(_JobInfo.GetSimpleBuffer(9));
	CVPU_SimpleBuffer<CMat4Dfp32> LastBoneTransformBuffer(_JobInfo.GetSimpleBuffer(10));
	CVPU_SimpleBuffer<TBitArray<MAX_BONE_COUNT_IN_BITARRAY> > CollisonBoneBuffer(_JobInfo.GetSimpleBuffer(11));
	CVPU_SimpleBuffer<CXR_SkeletonNodeData> SkelNodeBuffer(_JobInfo.GetSimpleBuffer(12));
	CVPU_SimpleBuffer<uint64> InsideCapsuleJointMasksBuffer(_JobInfo.GetSimpleBuffer(13));
	CVPU_SimpleBuffer<CJointVertsIndicies> JointVerticies(_JobInfo.GetSimpleBuffer(15));
	CVPU_SimpleBuffer<CVec4Dfp32> PrevJointPosition(_JobInfo.GetSimpleBuffer(16));
	CVPU_SimpleBuffer<fp32> ClosestFixedJointDistance(_JobInfo.GetSimpleBuffer(17));
	CVPU_SimpleBuffer<CXR_ClothBoneWeightsData> ClothBoneWeights(_JobInfo.GetSimpleBuffer(18));

	CClothWrapper CW;
	CW.m_bAdjustFloor=_JobInfo.GetLParam(19,0)==0 ? 0:1;
	CW.m_BlendMode=ClothBlendMode(_JobInfo.GetLParam(19,1));
	CW.m_FloorOffset=_JobInfo.GetFParam(20,0);
	CW.m_dt=_JobInfo.GetFParam(20,1);
	CW.m_Blend=_JobInfo.GetFParam(20,2);
	CW.m_SimFreq=_JobInfo.GetFParam(20,3);

	CW.m_pJointPosition = JointPositionBuffer.GetBuffer(CW.m_nJointPosition);
	CW.m_pParams =ParamBuffer.GetBuffer(CW.m_nParam);
	CW.m_pShearConstraints= ShearConstraintBuffer.GetBuffer(CW.m_nShearConstraints);
	CW.m_pSkeletonConstraints= SkeletonConstraintBuffer.GetBuffer(CW.m_nSkeletonConstraints);
	CW.m_pIds=IdBuffer.GetBuffer(CW.m_nIds);
	uint32 dummy;
	CW.m_pFixedJoints=FixedJointsBuffer.GetBuffer(dummy);
	CW.m_pNormalCapsules = NormalCapsulesBuffer.GetBuffer(CW.m_nNormalCapsules);
	CW.m_pNormalCapsulesTransformed = (TCapsule<fp32>*) MRTC_System_VPU::OS_LocalHeapAlloc(CW.m_nNormalCapsules*sizeof(TCapsule<fp32>));
	CW.m_pInvertedCapsules = InvertedCapsulesBuffer.GetBuffer(CW.m_nInvertedCapsules);
	CW.m_pInvertedCapsulesTransformed = (TCapsule<fp32>*) MRTC_System_VPU::OS_LocalHeapAlloc(CW.m_nInvertedCapsules*sizeof(TCapsule<fp32>));
	CW.m_pBoneTransform = BoneTransformBuffer.GetBuffer(CW.m_nBoneTransform);
	CW.m_pSkelBoneTransform = SkelBoneTransformBuffer.GetBuffer(CW.m_nSkelBoneTransform);
	CW.m_pLastBoneTransform = LastBoneTransformBuffer.GetBuffer(CW.m_nLastBoneTransform);
	CW.m_pCollisionBones = CollisonBoneBuffer.GetBuffer(dummy);
	CW.m_pSkelNodeDatas = SkelNodeBuffer.GetBuffer(CW.m_nSkelNodeDatas);
	CW.m_piJointVerticies = JointVerticies.GetBuffer(CW.m_niJointVerticies);
	CW.m_pPrevJointPosition = PrevJointPosition.GetBuffer(CW.m_nPrevJointPosition);
	CW.m_pClothBoneWeights = ClothBoneWeights.GetBuffer(CW.m_nClothBoneWeights);
	CW.m_pClosestFixedJointDistance = ClosestFixedJointDistance.GetBuffer(CW.m_nClosestFixedJointDistance);
	CW.m_pInsideCapsuleJointMasks = InsideCapsuleJointMasksBuffer.GetBuffer(CW.m_nInsideCapsuleJointMasks);
	CW.Step();
	
//	M_TRACE("Cloth VPU: Memory left: %d\r\n", MRTC_System_VPU::OS_LocalHeapMemLeft());
	return 0;
};


