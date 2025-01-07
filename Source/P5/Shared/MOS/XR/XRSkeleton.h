
#ifndef _INC_XRSkeleton
#define _INC_XRSkeleton

#include "MCC.h"
#include "../MSystem/Raster/MRender.h"
#include "XRAnim.h"

#define CXR_SKELETON_MAXROTATIONS	ANIM_MAXROTTRACKS
#define CXR_SKELETON_MAXMOVEMENTS	256

#define CXR_SKELETON_INVALIDTRACKMASK 0xff



/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CXR_SkeletonNode

	Comments:		
\*____________________________________________________________________*/
#define CXR_SKELETONNODE_VERSION	0x0100

class CXR_SkeletonNode
{
public:
	CVec3Dfp4 m_LocalCenter;				// This is the bones pivot in model-space.
	uint16 m_Flags;
	int16 m_iiNodeChildren;
	uint8 m_nChildren;
	mutable uint8 m_iTrackMask;
	int16 m_iNodeParent;
	fp4 m_RotationScale;
	fp4 m_MovementScale;
	int16 m_iRotationSlot;
	int16 m_iMovementSlot;

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
	CMat43fp4 m_LocalPos;

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

class CXR_ClothConstraint 
{
public:
	CXR_ClothConstraint()
	{
		m_id1=-1;
		m_id2=-1;
		m_weight=-1;
		m_type=-1;

		m_iRejectPlane = -1;
		m_iRejectSolid = -1;

		m_Fixed1 = false;
		m_Fixed2 = false;
	}

	CXR_ClothConstraint(int32 _id1, int32 _id2, fp8 _weight, int _type);
	void Read(CDataFile* _pDFile);
	void Write(CDataFile* _pDFile) const;

	bool m_Fixed1, m_Fixed2;
	int32 m_id1, m_id2;
	fp8 m_weight;
	fp8 m_restlength;
	int32 m_type;

	int32 m_iRejectPlane, m_iRejectSolid, m_CollisionType;
	CPlane3Dfp4 m_CollisionPlane;
	CVec3Dfp4 m_CollisionPoint, m_CollisionBodyCenter;
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CXR_ClothBoneWeights

	Comments:		
\*____________________________________________________________________*/
class CXR_ClothBoneWeights
{
public:
	CXR_ClothBoneWeights()
	{
		m_iClothJoint = -1;
	}

	void Read(CDataFile* _pDFile);
	void Write(CDataFile* _pDFile) const;

	TList_Vector<int16> m_liBoneIndex;
	TList_Vector<fp4> m_lWeights;
	int16 m_iClothJoint;
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CXR_Cloth

	Comments:		
\*____________________________________________________________________*/
#define CXR_CLOTH_VERSION	0xBABE

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

class CXR_Cloth
{
public:
	CXR_Cloth();
	CXR_Cloth(int _groupid);
	void Read(CDataFile* _pDFile);
	void Write(CDataFile* _pDFile) const;
	static void InitParams(fp4 *_pParams);
#ifndef M_RTM
	bool ParseParamaters(const CRegistry &_Reg); // Returns true if changed
#endif

	int32 m_GroupId;
	TList_Vector<CXR_ClothConstraint> m_lConstraints;
	TList_Vector<int> m_FixedJoints;
	TList_Vector<int> m_lJoints;
	TList_Vector<int> m_lCollisionSolids;
	TList_Vector<int> m_lCollisionCapsules;
	TList_Vector<CXR_ClothBoneWeights> m_lClothBoneWeights;
	CStr m_Name;

	bool m_bIsInitiated;
	int m_MaxID;

	fp4 m_lParams[NUM_CLOTHPARAMS];
};


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CXR_SkeletonInstance

	Comments:		
\*____________________________________________________________________*/
class CXR_SkeletonInstance : public CReferenceCount
{
	MRTC_DECLARE;

public:
//	TArray<CMat43fp4_MP> m_lBoneLocalPos;		// Bone position in bone's parent space.
//	TArray<CMat43fp4_MP, TList_Vector_CExtraSpace<4> > m_lBoneTransform;		// Transform from model-space to animated world-space (or whatever space supplied to EvalAnim() )
	// We allocate this array with 4 extra bytes at end to guard for use in SSE

//	TArray<fp4>	m_lBoneScale;				// Scale of each bone relative to parent

//	TArray<CQuatfp4> m_lTracksRot;			// Result of EvalAnim() is in these two, correspond roughly to the transform of m_lBoneLocalPos.
//	TArray<CVec3Dfp4> m_lTracksMove;

	CMat43fp4_MP* m_pBoneLocalPos;
	CMat43fp4_MP* m_pBoneTransform;
	fp4* m_pBoneScale;
	CQuatfp4* m_pTracksRot;
	CVec3Dfp4* m_pTracksMove;

	uint16	m_Flags;
	uint16	m_nBoneLocalPos;
	uint16	m_nBoneTransform;
	uint16	m_nBoneScale;
	uint16	m_nTracksRot;
	uint16	m_nTracksMove;

#ifndef M_RTM
	class CXR_Skeleton* m_pDebugSkel;
#endif

	CXR_SkeletonInstance();
	virtual ~CXR_SkeletonInstance();

	void Create(uint _nNodes);
	void CreateTracks(uint16 _nRot, uint16 _nMove);
	void Duplicate(CXR_SkeletonInstance* _pDest) const;

	int GetNumBones() const { return m_nBoneTransform; };
	CMat43fp4_MP* GetBoneTransforms() { return m_pBoneTransform; };

	void ApplyScale(fp4 _Scale);
	void ApplyScale(fp4 _Scale, const TArray<int>& _liBoneSet);
	void ApplyScale(fp4 _Scale, const int _pliBoneSet[], int _BonesInSet);

	const CMat43fp4_MP& GetBoneTransform(int _iIndex) const
	{ 
		return m_pBoneTransform[_iIndex]; 
	}

	void GetBoneTransform(uint _iIndex, CMat43fp4_MP* _pDest) const
	{
		*_pDest = m_pBoneTransform[_iIndex];
	}

	const CMat43fp4_MP& GetBoneLocalPos(uint _iIndex) const
	{
		return m_pBoneLocalPos[_iIndex];
	}

	void GetBoneLocalPos(uint _iIndex, CMat43fp4_MP* _pDest) const
	{
		*_pDest	= m_pBoneLocalPos[_iIndex];
	}

	void SetBoneLocalPos(uint _iIndex, CMat43fp4_MP* _pSrc)
	{
		m_pBoneLocalPos[_iIndex] = *_pSrc;
	}

	bool BlendInstance(const CXR_SkeletonInstance* _pInstance, const CXR_SkeletonInstance* _pTarget, fp4 _Blend) const;
};


typedef TPtr<CXR_SkeletonInstance> spCXR_SkeletonInstance;


/*¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯¯*\
	Class:			CXR_AnimLayer

	Comments:		
\*____________________________________________________________________*/
#define CXR_ANIMLAYER_ATTACHNOREL				1
#define CXR_ANIMLAYER_ATTACHNOREL_RECURSIVE		2
#define CXR_ANIMLAYER_ATTACHPLANE_Z				4
#define CXR_ANIMLAYER_ATTACHPLANE_Z_RECURSIVE	8
#define CXR_ANIMLAYER_USEMOVE					16
#define CXR_ANIMLAYER_REVERSE					32
#define CXR_ANIMLAYER_FREEZE					64
#define CXR_ANIMLAYER_TAGSYNC					128
#define CXR_ANIMLAYER_CHECKALLANIMEVENTS		256
#define CXR_ANIMLAYER_FORCELAYERVELOCITY		512
#define CXR_ANIMLAYER_LAYERNOVELOCITY			1024
#define CXR_ANIMLAYER_ADDITIVEBLEND				2048
#define CXR_ANIMLAYER_TAGUSEANIMCAMERA			4096
#define CXR_ANIMLAYER_REMOVEPREVNODELAYERS		8192
#define CXR_ANIMLAYER_NOPRECACHE				16384
#define CXR_ANIMLAYER_VALUECOMPARE				32768

class CXR_AnimLayer
{
public:
	spCXR_Anim_SequenceData m_spSequence;
	uint16 m_iBlendBaseNode;
	uint16 m_Flags;
	fp4 m_Blend;
	fp4 m_BlendIn, m_BlendOut;
	fp4 m_Time;
	CMTime m_ContinousTime;
	fp4 m_TimeScale;
	fp4 m_TimeOffset;

#ifdef	AG_DEBUG
	CStr m_DebugMessage;
#endif
	CXR_AnimLayer()
	{
	}

	CXR_AnimLayer(CXR_Anim_Base* _pAnim, int _iSeq, const CMTime& _Time, fp4 _TimeScale, fp4 _Blend, int _iBlendBaseNode, int _Flags = 0)
	{
		Create2(_pAnim, _iSeq, _Time, _TimeScale, _Blend, _iBlendBaseNode, _Flags);
	}

	CXR_AnimLayer(const CXR_Anim_SequenceData *_pSequence, const CMTime& _Time, fp4 _TimeScale, fp4 _Blend, int _iBlendBaseNode, int _Flags = 0)
	{
		Create3(_pSequence, _Time, _TimeScale, _Blend, _iBlendBaseNode, _Flags);
	}

	void Create2(CXR_Anim_Base* _pAnim, int _iSeq, const CMTime& _Time, fp4 _TimeScale, fp4 _Blend, int _iBlendBaseNode, int _Flags = 0)
	{
		m_spSequence = _pAnim->GetSequence(_iSeq);
		m_iBlendBaseNode = _iBlendBaseNode;
		m_Flags = _Flags;
		m_Blend = _Blend;
		m_Time = _Time.GetTime();
		m_TimeScale = _TimeScale;
		m_BlendIn = 1.0f;
		m_BlendOut = 1.0f;
		m_TimeOffset = 0.0f;
	}

	void Create3(const class CXR_Anim_SequenceData* _pSequence, const CMTime& _Time, fp4 _TimeScale, fp4 _Blend, int _iBlendBaseNode, int _Flags = 0)
	{
		if (_pSequence)
			M_ASSERT(_pSequence->MRTC_ReferenceCount(), "!");

		m_spSequence = const_cast<CXR_Anim_SequenceData*>(_pSequence);
		m_iBlendBaseNode = _iBlendBaseNode;
		m_Flags = _Flags;
		m_Blend = _Blend;
		m_Time = _Time.GetTime();
		m_TimeScale = _TimeScale;
		m_BlendIn = 1.0f;
		m_BlendOut = 1.0f;
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
//	void BlendTracks_r(uint16 _iNode, fp4 _Blend, const CQuatfp4* _pQSrc, CQuatfp4* _pQDst, const CVec3Dfp4* _pMSrc, CVec3Dfp4* _pMDst, uint8* _pQDone, uint8* _pMDone, uint16 _Flags) const;
//	void BlendTracks_i(uint16 _iNode, fp4 _Blend, const CQuatfp4* _pQSrc, CQuatfp4* _pQDst, const CVec3Dfp4* _pMSrc, CVec3Dfp4* _pMDst, uint8* _pQDone, uint8* _pMDone, uint16 _Flags) const;
	void InitNode_r(uint16 _iNode, CXR_SkeletonInstance* _pInst, const CMat43fp4_MP* _pRot, const CVec3Dfp4* _pMove, uint16 _nRot, uint16 _nMove) const;
	void InitNode_i(uint16 _iNode, CXR_SkeletonInstance* _pInst, const CMat43fp4_MP* _pRot, const CVec3Dfp4* _pMove, uint16 _nRot, uint16 _nMove) const;
	void EvalNode_r(uint16 _iNode, const CMat43fp4_MP* _pTransform, CXR_SkeletonInstance* _pInst) const;
	void EvalNode_i(uint16 _iNode, const CMat43fp4_MP* _pTransform, CXR_SkeletonInstance* _pInst) const;
	void InitEvalNode_i(uint16 _iNode, const CMat43fp4_MP& _Transform, CXR_SkeletonInstance* _pInst, const CQuatfp4* _pRot, const CVec3Dfp4* _pMove, uint16 _nRot, uint16 _nMove) const;
	void EvalPosition_r(uint16 _iNode, const CMat43fp4_MP* _pTransform, CXR_SkeletonInstance* _pInst) const;

public:
#ifdef PLATFORM_CONSOLE
	TThinArray<uint16> m_liNodes;
	TThinArray<CXR_SkeletonNode> m_lNodes;
	TThinArray<CXR_SkeletonAttachPoint> m_lAttachPoints;
#else
	TList_Vector<uint16> m_liNodes;
	TList_Vector<CXR_SkeletonNode> m_lNodes;
	TList_Vector<CXR_SkeletonAttachPoint> m_lAttachPoints;
#endif
//	TThinArray<CXR_Anim_TrackMask> m_lNodeTrackMask;
	mutable TThinArray<spCXR_Anim_TrackMask> m_lspNodeTrackMask;
	
//	TList_Vector<CXR_ClothConstraint> m_lClothConstraints;
	TList_Vector<CXR_Cloth> m_lCloth;

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

	void BlendTracks(uint16 _iNode, fp4 _Blend, const CQuatfp4* _pQSrc, CQuatfp4* _pQDst, const CVec3Dfp4* _pMSrc, CVec3Dfp4* _pMDst, uint8* _pQDone, uint8* _pMDone, uint16 _Flags, const CXR_Anim_TrackMask& _NodeMask) const;
	void InitNode(uint16 _iNode, CXR_SkeletonInstance* _pInst, const CMat43fp4_MP* _pRot, const CVec3Dfp4* _pMove, uint16 _nRot, uint16 _nMove) const;
	void EvalNode(uint16 _iNode, const CMat43fp4_MP* _pTransform, CXR_SkeletonInstance* _pInst) const;
	void InitEvalNode(uint16 _iNode, const CMat43fp4_MP& _Transform, CXR_SkeletonInstance* _pInst) const;

	void EvalPosition(uint16 _iNode, const CMat43fp4_MP* _pTransform, CXR_SkeletonInstance* _pInst) const;

	void EvalNodeQuat(uint16 _iNode, const CQuatfp4* _pQSrc, CQuatfp4& _Dst) const;
	void EvalTracks(CXR_AnimLayer* _pLayer, uint _nLayers, CXR_SkeletonInstance* _pSkelInst, uint _Flags = 0, const CXR_Anim_TrackMask* _pTrackMaskOr = NULL) const;
	dllvirtual void EvalAnim(CXR_AnimLayer* _pLayers, uint _nLayers, CXR_SkeletonInstance* _pSkelInst, CMat43fp4& _WMat, uint _Flags = 0, const CXR_Anim_TrackMask* _pTrackMaskOr = NULL) const;

	CMat4Dfp4 EvalNode(uint16 _iNode, CXR_Anim_Keyframe* _pFrame);

	const CXR_SkeletonAttachPoint* GetAttachPoint(int _iAttach);

	void Read(CDataFile* _pDFile);
	void Write(CDataFile* _pDFile) const;
};

typedef TPtr<CXR_Skeleton> spCXR_Skeleton;


#endif

