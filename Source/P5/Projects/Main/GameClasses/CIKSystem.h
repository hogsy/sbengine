/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	File:			Inverse kinamatics system.

	Author:			Fredrik Johansson
					Jakob Ericsson

	Copyright:		2005 Starbreeze Studios AB

	Contents:		CIKSystem

	Comments:

	History:
		051207:		Created file
		060109:		Rewrote everything
\*____________________________________________________________________________________________*/
#ifndef __CIKSystem_h__
#define __CIKSystem_h__

#include "../../../Shared/MOS/XR/XRSkeleton.h"
//#include "WObj_Char.h"


const int THRESHOLD = 0;
const int MAX_ITERATIONS = 10;
const int MAX_NODES = 5;


// IKBone struct
struct SIKBone
{
	int					m_iNodeID;
	int					m_iParentID;
	CQuatfp32			m_CurrentRot;
	CVec3Dfp32			m_CurrentEndEff;
	CVec3Dfp32			m_WorldPos;

	SIKBone::SIKBone()
	{
		m_iNodeID = 0;
		m_iParentID = 0;
		m_CurrentRot.Unit();

		m_WorldPos.k[0] = 0.0f;
		m_WorldPos.k[1] = 0.0f;
		m_WorldPos.k[2] = 0.0f;
	}
};


class CIKSystem : public CReferenceCount
{
public:

	enum 
	{
		IK_MODE_LEFT_HAND = 0,
		IK_MODE_RIGHT_HAND,
		IK_MODE_FEET,
		IK_MODE_LOOK,
		IK_MODE_LEFT_FOOT,
		IK_MODE_RIGHT_FOOT,

		IK_MODE_DARKLING_RIGHT_BACK_FOOT,
		IK_MODE_DARKLING_LEFT_BACK_FOOT,
		IK_MODE_DARKLING_RIGHT_FRONT_FOOT,
		IK_MODE_DARKLING_LEFT_FRONT_FOOT,
	};

	enum
	{
		/*
		// Darkling specifika
		*RightMidLeg		92	,	-1
		*RightRightToe		93	,	-1
		*RightLeftToe		94	,	-1
		*RightHeelToe1		95	,	-1
		*RightLowLeg		3	,	-1	

		*LeftMidLeg			96	,	-1
		*LeftRightToe		97	,	-1
		*LeftLeftToe		98	,	-1
		*LeftHeelToe1		99	,	-1
		*LeftLowLeg			6	,	-1			
		*attach_right		22	,	2,  NoSkinning
		*attach_left		23	,	3,  NoSkinning
		*/
		DARKLING_ROTTRACK_RIGHT_MIDLEG = 92,
		DARKLING_ROTTRACK_RIGHT_RIGHTTOE = 93,
		DARKLING_ROTTRACK_RIGHT_LEFTTOE = 94,
		DARKLING_ROTTRACK_RIGHT_HEALTOE1 = 95,
		DARKLING_ROTTRACK_RIGHT_LOWLEG = 3,

		DARKLING_ROTTRACK_LEFT_MIDLEG = 96,
		DARKLING_ROTTRACK_LEFT_RIGHTTOE = 97,
		DARKLING_ROTTRACK_LEFT_LEFTTOE = 98,
		DARKLING_ROTTRACK_LEFT_HEALTOE1 = 99,
		DARKLING_ROTTRACK_LEFT_LOWLEG = 6,
	};

	CIKSystem();
	virtual ~CIKSystem() {};

	bool IKSolve(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CXR_Model* _pModel, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);
	
	void ProcessNodeArrray(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, const CXR_SkeletonAttachPoint* _pAttachWeapon,
		const CXR_SkeletonAttachPoint* _pAttachLeftHand, const uint8 _iNumberOfNodes, const SIKBone* _iNodes, CMat4Dfp32& _TargetPos, CWorld_PhysState* _pWPhysState); 
	
	void PostEvalHands(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CMat4Dfp32& _BaseMat);

	void SetIKMode(uint8 _iMode) { m_iIKMode = _iMode; };
	void SetTargetAngle(fp32 _fTargetAngle) { m_fTargetAngle = _fTargetAngle; };

	void FindNextIKTarget(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CXR_Model* _pModel, CWObject_CoreData* _pObj, CWO_Character_ClientData *_pCDFirst, CWorld_PhysState* _pWPhysState);

	void DoDualHandIK(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, const CMat4Dfp32 *_pTarget, CWorld_PhysState* _pWPhysState, CWO_Character_ClientData *_pCDFirst, const CVec3Dfp32 *_pDesiredPointDir = NULL, const CMat4Dfp32 *_pGrabRailIfNoReach = NULL);
	void GetCircleCircleIntersectionPoints(fp32 _C1Radius, fp32 _C2XPos, fp32 _C2Radius, CVec2Dfp32* _pResult1, CVec2Dfp32* _pResult2);
	void SetupAndAdjustMatrix(int8 _iJoint, int8 _iChildJoint, const CVec3Dfp32* _pJointPosWorld, const CVec3Dfp32* _pChildJointPosWorld, CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CWorld_PhysState* _pWPhysState);


private:
	
	void LowerSkeleton(CXR_Skeleton* _pSkel, CXR_SkeletonInstance* _pSkelInstance, CXR_Model* _pModel, CWObject_CoreData* _pObj, CWorld_PhysState* _pWPhysState);

	uint8		m_iIKMode;
	fp32			m_fTargetAngle;

	SIKBone		m_sNodes[MAX_NODES];
    CVec3Dfp32	m_TargetPos;		// Target position in world coordinates
	
};

#endif
