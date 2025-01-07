
#ifndef _INC_XRSkeleton
#define _INC_XRSkeleton

#include "MCC.h"
#include "../MSystem/Raster/MRender.h"
#include "XRAnim.h"
#include "XRAnimData.h"

#define CXR_SKELETON_MAXROTATIONS	ANIM_MAXROTTRACKS
#define CXR_SKELETON_MAXMOVEMENTS	256

#define CXR_SKELETON_INVALIDTRACKMASK 0xff



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CXR_SkeletonNode

	Comments:		
\*____________________________________________________________________*/
#define CXR_SKELETONNODE_VERSION	0x0100

class CXR_SkeletonNode : public CXR_SkeletonNodeData
{
public:
	CXR_SkeletonNode();
	void Read(CDataFile* _pDFile);
	void Write(CDataFile* _pDFile) const;
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CXR_SkeletonAttachPoint

	Comments:		
\*____________________________________________________________________*/
#define CXR_SKELETONATTACH_VERSION	0x0101

class CXR_SkeletonAttachPoint
{
public:
	uint16 m_iNode;
	uint16 __m_Padding;
	CMat4Dfp32 m_LocalPos;

	CXR_SkeletonAttachPoint();
	void Read(CDataFile* _pDFile);
	void Write(CDataFile* _pDFile) const;

	bool Parse(CStr _Str);
};



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CXR_ClothConstraint

	Comments:		
\*____________________________________________________________________*/
#define CXR_CLOTHCONSTRAINT_VERSION	0xBABE
#define CXR_CLOTHBONEWEIGHTS_VERSION 0xBABE


class CXR_ClothConstraint : public CXR_ClothConstraintData
{
public:
	CXR_ClothConstraint()
	{
		m_id1=-1;
		m_id2=-1;
		m_weight=-1;
		m_type=-1;

		m_bFixed1 = 0.0f;
		m_bFixed2 = 0.0f;
	}

	CXR_ClothConstraint(int32 _id1, int32 _id2, fp64 _weight, int _type);
	void Read(CDataFile* _pDFile);
	void Write(CDataFile* _pDFile) const;
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CXR_ClothBoneWeights

	Comments:		
\*____________________________________________________________________*/
class CXR_ClothBoneWeights : public CXR_ClothBoneWeightsData
{
public:
	CXR_ClothBoneWeights()
	{
		m_iClothJoint = -1;
		m_BoneCount=0;
	}
	void Read(CDataFile* _pDFile);
	void Write(CDataFile* _pDFile) const;
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CXR_Cloth

	Comments:		
\*____________________________________________________________________*/
#define CXR_CLOTH_VERSION	0xBABE

class CXR_Cloth
{
public:
	CXR_Cloth();
	CXR_Cloth(int _groupid);
	void Read(CDataFile* _pDFile, const class CXR_Skeleton* _pSkel);
	void Write(CDataFile* _pDFile, const class CXR_Skeleton* _pSkel) const;
	static void InitParams(fp32 *_pParams);
#ifndef M_RTM
	bool ParseParamaters(const CRegistry &_Reg); // Returns true if changed
#endif

	int32 m_GroupId;
	TArray<CXR_ClothConstraint> m_lConstraints;
	TArray<int> m_FixedJoints;
	TArray<int> m_InsideCapsuleJoints;
	TThinArrayAlign<int,16> m_lJoints;
	TArray<int> m_lCollisionSolids;
	TArray<int> m_lCollisionCapsules;
	TArray<CXR_ClothBoneWeights> m_lClothBoneWeights;
	CStr m_Name;

	int m_MaxID;

	fp32 m_lParams[NUM_CLOTHPARAMS];
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CXR_SkeletonInstance

	Comments:		
\*____________________________________________________________________*/
class CXR_SkeletonInstance : public CReferenceCount
{
	MRTC_DECLARE;

public:
//	TArray<CMat43fp32> m_lBoneLocalPos;		// Bone position in bone's parent space.
//	TArray<CMat43fp32, TArray_CExtraSpace<4> > m_lBoneTransform;		// Transform from model-space to animated world-space (or whatever space supplied to EvalAnim() )
	// We allocate this array with 4 extra bytes at end to guard for use in SSE

//	TArray<fp32>	m_lBoneScale;				// Scale of each bone relative to parent

//	TArray<CQuatfp32> m_lTracksRot;			// Result of EvalAnim() is in these two, correspond roughly to the transform of m_lBoneLocalPos.
//	TArray<CVec3Dfp32> m_lTracksMove;

	CMat4Dfp32* m_pBoneLocalPos;
	CMat4Dfp32* m_pBoneTransform;
	fp32* m_pBoneScale;
	CQuatfp32* m_pTracksRot;
	CVec4Dfp32* m_pTracksMove;

	uint16	m_Flags;
	uint16	m_nBoneLocalPos;
	uint16	m_nBoneTransform;
	uint16	m_nBoneScale;
	uint16	m_nTracksRot;
	uint16	m_nTracksMove;
	uint16  m_VpuTaskId;
	fp32	m_MoveScale;			// MoveTracks are scaled by this value (default: 1.0)

#ifndef M_RTM
	class CXR_Skeleton* m_pDebugSkel;
#endif

	CXR_SkeletonInstance();
	virtual ~CXR_SkeletonInstance();

	void Create(uint _nNodes);
	void CreateTracks(uint16 _nRot, uint16 _nMove);
	void Duplicate(CXR_SkeletonInstance* _pDest) const;

	int GetNumBones() const { return m_nBoneTransform; };
	CMat4Dfp32* GetBoneTransforms() { return m_pBoneTransform; };

	void ApplyScale(fp32 _Scale);
	void ApplyScale(fp32 _Scale, const TArray<int>& _liBoneSet);
	void ApplyScale(fp32 _Scale, const int _pliBoneSet[], int _BonesInSet);

	const CMat4Dfp32& GetBoneTransform(int _iIndex) const
	{ 
		return m_pBoneTransform[_iIndex]; 
	}

	void GetBoneTransform(uint _iIndex, CMat4Dfp32* _pDest) const
	{
		*_pDest = m_pBoneTransform[_iIndex];
	}

	const CMat4Dfp32& GetBoneLocalPos(uint _iIndex) const
	{
		return m_pBoneLocalPos[_iIndex];
	}

	void GetBoneLocalPos(uint _iIndex, CMat4Dfp32* _pDest) const
	{
		*_pDest	= m_pBoneLocalPos[_iIndex];
	}

	void SetBoneLocalPos(uint _iIndex, CMat4Dfp32* _pSrc)
	{
		m_pBoneLocalPos[_iIndex] = *_pSrc;
	}

	bool BlendInstance(const CXR_SkeletonInstance* _pInstance, const CXR_SkeletonInstance* _pTarget, fp32 _Blend) const;
};


typedef TPtr<CXR_SkeletonInstance> spCXR_SkeletonInstance;


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CXR_AnimLayer

	Comments:		
\*____________________________________________________________________*/
enum
{
	CXR_ANIMLAYER_ATTACHNOREL				= M_Bit(0),
	CXR_ANIMLAYER_ATTACHNOREL_RECURSIVE		= M_Bit(1),
	CXR_ANIMLAYER_ATTACHPLANE_Z				= M_Bit(2),
	CXR_ANIMLAYER_ATTACHPLANE_Z_RECURSIVE	= M_Bit(3),
	CXR_ANIMLAYER_USEMOVE					= M_Bit(4),
	CXR_ANIMLAYER_IGNOREBASENODE			= M_Bit(5),
	CXR_ANIMLAYER_FREEZE					= M_Bit(6),
	CXR_ANIMLAYER_TAGSYNC					= M_Bit(7),
	CXR_ANIMLAYER_CHECKALLANIMEVENTS		= M_Bit(8),
	CXR_ANIMLAYER_FORCELAYERVELOCITY		= M_Bit(9),
	CXR_ANIMLAYER_LAYERNOVELOCITY			= M_Bit(10),
	CXR_ANIMLAYER_ADDITIVEBLEND				= M_Bit(11),
	CXR_ANIMLAYER_TAGUSEANIMCAMERA			= M_Bit(12),
	CXR_ANIMLAYER_REMOVEPREVNODELAYERS		= M_Bit(13),
	CXR_ANIMLAYER_NOPRECACHE				= M_Bit(14),
	CXR_ANIMLAYER_VALUECOMPARE				= M_Bit(15),
	CXR_ANIMLAYER_FORCEAFTERVOCAP			= M_Bit(16),
	CXR_ANIMLAYER_LAYERISIDLE				= M_Bit(17),
};


class CXR_AnimLayer
{
public:
	CMTime m_ContinousTime;
	spCXR_Anim_SequenceData m_spSequence;
	fp32 m_Blend;
	fp32 m_Time;
	fp32 m_TimeScale;
	fp32 m_TimeOffset;
	uint32 m_Flags;
	uint16 m_iBlendBaseNode;

#ifdef	AG_DEBUG
	CStr m_DebugMessage;
#endif
	CXR_AnimLayer()
	{
	}

	CXR_AnimLayer(CXR_Anim_Base* _pAnim, int _iSeq, const CMTime& _Time, fp32 _TimeScale, fp32 _Blend, int _iBlendBaseNode, int _Flags = 0)
	{
		Create2(_pAnim, _iSeq, _Time, _TimeScale, _Blend, _iBlendBaseNode, _Flags);
	}

	CXR_AnimLayer(const CXR_Anim_SequenceData *_pSequence, const CMTime& _Time, fp32 _TimeScale, fp32 _Blend, int _iBlendBaseNode, int _Flags = 0)
	{
		Create3(_pSequence, _Time, _TimeScale, _Blend, _iBlendBaseNode, _Flags);
	}

	void Create2(CXR_Anim_Base* _pAnim, int _iSeq, const CMTime& _Time, fp32 _TimeScale, fp32 _Blend, int _iBlendBaseNode, int _Flags = 0)
	{
		m_spSequence = _pAnim->GetSequence(_iSeq);
		m_iBlendBaseNode = _iBlendBaseNode;
		m_Flags = _Flags;
		m_Blend = _Blend;
		m_Time = _Time.GetTime();
		m_TimeScale = _TimeScale;
		m_TimeOffset = 0.0f;
	}

	void Create3(const class CXR_Anim_SequenceData* _pSequence, const CMTime& _Time, fp32 _TimeScale, fp32 _Blend, int _iBlendBaseNode, int _Flags = 0)
	{
		if (_pSequence)
			M_ASSERT(_pSequence->MRTC_ReferenceCount(), "!");

		m_spSequence = const_cast<CXR_Anim_SequenceData*>(_pSequence);
		m_iBlendBaseNode = _iBlendBaseNode;
		m_Flags = _Flags;
		m_Blend = _Blend;
		m_Time = _Time.GetTime();
		m_TimeScale = _TimeScale;
		m_TimeOffset = 0.0f;
	}
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CXR_Skeleton

	Comments:		
\*____________________________________________________________________*/
enum
{
	CXR_SKELETON_APPLYOBJECTPOSITION = 1,	// The resulting position is taken from the animation, not from the ingoing WMat
};

class CXR_Skeleton : public CReferenceCount
{
	MRTC_DECLARE;

protected:
//	void BlendTracks_r(uint16 _iNode, fp32 _Blend, const CQuatfp32* _pQSrc, CQuatfp32* _pQDst, const CVec3Dfp32* _pMSrc, CVec3Dfp32* _pMDst, uint8* _pQDone, uint8* _pMDone, uint16 _Flags) const;
//	void BlendTracks_i(uint16 _iNode, fp32 _Blend, const CQuatfp32* _pQSrc, CQuatfp32* _pQDst, const CVec3Dfp32* _pMSrc, CVec3Dfp32* _pMDst, uint8* _pQDone, uint8* _pMDone, uint16 _Flags) const;
	void InitNode_r(uint16 _iNode, CXR_SkeletonInstance* _pInst, const CMat43fp32* _pRot, const CVec3Dfp32* _pMove, uint16 _nRot, uint16 _nMove) const;
	void InitNode_i(uint16 _iNode, CXR_SkeletonInstance* _pInst, const CMat4Dfp32* _pRot, const CVec3Dfp32* _pMove, uint16 _nRot, uint16 _nMove) const;
	void EvalNode_r(uint16 _iNode, const CMat43fp32* _pTransform, CXR_SkeletonInstance* _pInst) const;
	void EvalNode_i(uint16 _iNode, const CMat4Dfp32* _pTransform, CXR_SkeletonInstance* _pInst) const;
	void InitEvalNode_i(uint16 _iNode, const CMat4Dfp32& _Transform, CXR_SkeletonInstance* _pInst, const CQuatfp32* _pRot, const CVec4Dfp32* _pMove, uint16 _nRot, uint16 _nMove) const;
	void EvalPosition_r(uint16 _iNode, const CMat4Dfp32* _pTransform, CXR_SkeletonInstance* _pInst) const;

public:
#ifdef PLATFORM_CONSOLE
	TThinArray<uint16> m_liNodes;
	TThinArray<CXR_SkeletonNode> m_lNodes;
	TThinArray<CXR_SkeletonAttachPoint> m_lAttachPoints;
#else
	TArray<uint16> m_liNodes;
	TArray<CXR_SkeletonNode> m_lNodes;
	TArray<CXR_SkeletonAttachPoint> m_lAttachPoints;
#endif
//	TThinArray<CXR_Anim_TrackMask> m_lNodeTrackMask;
	mutable TThinArray<spCXR_Anim_TrackMask> m_lspNodeTrackMask;
	
//	TArray<CXR_ClothConstraint> m_lClothConstraints;
	TArray<CXR_Cloth> m_lCloth;

	uint m_nUsedRotations;
	uint m_nUsedMovements;

	CXR_Anim_TrackMask m_TrackMask;
	CXR_Anim_TrackMask m_NodeMask;

	mutable MRTC_CriticalSection m_Lock;

public:
	CXR_Skeleton();
	void Init();
	void InitTrackMask();
	void CreateTrackNodeMask_r(uint16 _iNode, CXR_Anim_TrackMask& _Mask, CXR_Anim_TrackMask& _NodeMask) const;
	void GetTrackMask(uint16 _iNode, CXR_Anim_TrackMask*&_pTrackMask, CXR_Anim_TrackMask*& _pNodeMask, CXR_Anim_TrackMask*& _pCoverageMask) const;
	const CXR_Anim_TrackMask *GetTrackMask() const { return &m_TrackMask; }

	void BlendTracks(uint16 _iNode, fp32 _Blend, const CQuatfp32* _pQSrc, CQuatfp32* _pQDst, const CVec4Dfp32* _pMSrc, CVec4Dfp32* _pMDst, uint8* _pQDone, uint8* _pMDone, uint16 _Flags, const CXR_Anim_TrackMask& _NodeMask) const;
	void InitNode(uint16 _iNode, CXR_SkeletonInstance* _pInst, const CMat4Dfp32* _pRot, const CVec3Dfp32* _pMove, uint16 _nRot, uint16 _nMove) const;
	void EvalNode(uint16 _iNode, const CMat4Dfp32* _pTransform, CXR_SkeletonInstance* _pInst) const;
	void InitEvalNode(uint16 _iNode, const CMat4Dfp32& _Transform, CXR_SkeletonInstance* _pInst) const;
	void EvalNode_IK_Special(uint16 _iNode, const CMat4Dfp32* _pTransform, CXR_SkeletonInstance* _pInst, int _iEvalNumberOfChildren = -1) const;

	void EvalPosition(uint16 _iNode, const CMat4Dfp32* _pTransform, CXR_SkeletonInstance* _pInst) const;

	void EvalNodeQuat(uint16 _iNode, const CQuatfp32* _pQSrc, CQuatfp32& _Dst) const;
	void EvalTracks(CXR_AnimLayer* _pLayer, uint _nLayers, CXR_SkeletonInstance* _pSkelInst, uint _Flags = 0, const CXR_Anim_TrackMask* _pTrackMaskOr = NULL) const;
	dllvirtual void EvalAnim(CXR_AnimLayer* _pLayers, uint _nLayers, CXR_SkeletonInstance* _pSkelInst, CMat4Dfp32& _WMat, uint _Flags = 0, const CXR_Anim_TrackMask* _pTrackMaskOr = NULL) const;

	CMat4Dfp32 EvalNode(uint16 _iNode, CXR_Anim_Keyframe* _pFrame);

	const CXR_SkeletonAttachPoint* GetAttachPoint(int _iAttach);
	void CalculateLocalMatrices(CXR_SkeletonInstance* _pSkelInst, const CMat4Dfp32& _BaseMat);
	void CalculateLocalMatrices(CXR_SkeletonInstance* _pSkelInst, const CMat4Dfp32& _BaseMat, uint16* _liNodes, int32 _nNodes);

	void Read(CDataFile* _pDFile);
	void Write(CDataFile* _pDFile) const;
};

typedef TPtr<CXR_Skeleton> spCXR_Skeleton;


#endif

