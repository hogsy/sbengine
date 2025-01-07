
#ifndef	__CCONSTRAINTSYSTEMCLIENT_H_INCLUDED
#define	__CCONSTRAINTSYSTEMCLIENT_H_INCLUDED

#include "../../../Shared/MOS/XR/XRSkeleton.h"

// CConstraintSystemClient holds the data needed to run the ragdoll on the client
class CConstraintSystemClient : public CReferenceCount
{
public:
	CConstraintSystemClient();
	~CConstraintSystemClient();

	void Clear();
	// Increases the nbr of tracks if needed, returns true when a grow was executed
	bool GrowTracks(int _nTracks);

	void Init(int _nTracks);
	void EvalAnim(fp32 _Frac,class CXR_AnimLayer* _pLayers, int _nLayers,class CXR_Skeleton* _pSkel,class CXR_SkeletonInstance* _pSkelInst, CMat4Dfp32& _WMat,int _Flags, fp32 _OverrideRagdollHeight = 0.0f);
	bool IsValid();

	void CopyFrom(const CConstraintSystemClient& _From);
	void Pack(uint8 *&_pD, class CMapData* _pMapData) const;
	void Unpack(const uint8 *&_pD, class CMapData* _pMapData);

	int m_UnPackCount;

	CMat4Dfp32						m_OrgMat;
	int8							m_State;		// System is ready when true
	enum {
		NOTREADY = 0,		// Not yet started up
		GETFIRSTFRAME = 1,
		BLENDFRAMES = 2,
		READY = 4,			// We run entirely in ragdoll with collision detection
		STOPPED = 5,		// We've stopped, set m_State to READY again if dragged etc
	};


	// Calculated quats and vecs to interpolate against
	uint16							m_nTracks;	// Max of rot and move tracks
	TArray<CQuatfp32>				m_lRot0;
	TArray<CVec3Dfp32>				m_lMove0;

	TArray<CQuatfp32>				m_lRot1;
	TArray<CVec3Dfp32>				m_lMove1;
};

typedef TPtr<CConstraintSystemClient> spCConstraintSystemClient;

// Simple struct for storing settings for CConstraintSystem
// We keep them in a separate structure so that they can be set wothout creating the large
// CConstraintSystem object (ca 18 KB as of this writing).
struct SConstraintSystemSettings
{
	SConstraintSystemSettings();

	enum
	{	
		RAGDOLL_NONE	= 0,
		RAGDOLL_DEFAULT	= 1,
		RAGDOLL_NEW	= 2,
	};
	uint8	m_SkeletonType;
	uint8	m_ConstraintIterations;
	uint8	m_BlendFrames;
	fp32		m_Rebound;
	fp32		m_StopThreshold;
	CXR_Skeleton* m_lpClothSkeleton[CWO_NUMMODELINDICES];
	CXR_Skeleton* m_pSkeleton;
	CXR_SkeletonInstance* m_pSkelInstance;
};
typedef TPtr<SConstraintSystemSettings> spSConstraintSystemSettings;

#endif	// __CCONSTRAINTSYSTEMCLIENT_H_INCLUDED
