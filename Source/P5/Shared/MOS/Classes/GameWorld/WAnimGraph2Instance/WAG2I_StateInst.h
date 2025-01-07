#ifndef WAG2I_StateInst_h
#define WAG2I_StateInst_h

//--------------------------------------------------------------------------------

#include "../../../XR/XRAnimGraph2/AnimGraph2Defs.h"
#include "../../../XR/XRAnimGraph2/AnimGraph2.h"
//#include "WAG2I.h"
//#include "WAG2I_StateInstPacked.h"

class CWAG2I;
class CWAG2I_SIP;
class CWAG2I_SIID;
class CWAG2I_Token;

//--------------------------------------------------------------------------------

// Will only be set on client.
#define STATEINSTANCEFLAGS_PREDMISS_ADD			0x01
#define STATEINSTANCEFLAGS_PREDMISS_REMOVE		0x02
#define STATEINSTANCEFLAGS_PREDMISS_UPDATE		0x04

//--------------------------------------------------------------------------------

class CEventLayer
{
public:
	CXR_AnimLayer m_Layer;
	int16* m_pKey;
};

class CWAG2I_StateInstance // <m_EnqueueTime, m_iEnterAction>
{
	protected:
		friend class CXRAG2_PropertyRecorder;
		enum
		{
			AG2I_STATEINST_DIRTYFLAG_ENQUEUETIME		= 1 << 0,
			AG2I_STATEINST_DIRTYFLAG_ENTERTIME			= 1 << 1,
			AG2I_STATEINST_DIRTYFLAG_LEAVETIME			= 1 << 2,
			AG2I_STATEINST_DIRTYFLAG_ENTERMOVETOKEN		= 1 << 3,
			AG2I_STATEINST_DIRTYFLAG_LEAVEMOVETOKEN		= 1 << 4,
			AG2I_STATEINST_DIRTYFLAG_ANIMGRAPHID		= 1 << 5,
			AG2I_STATEINST_DIRTYFLAG_SYNCANIMSCALE		= 1 << 6,
		};

		CWAG2I* m_pAG2I;
		//CWAG2I_Token* m_pToken;
		// 68 bytes of fp32
		//  6 bytes of int16
		//  6 bytes of int8/uint8
		//  2 bits of bool (1 byte)
		// 81 bytes per instance
		CMTime m_EnqueueTime;
		CMTime m_EnterTime;
		CMTime m_LeaveTime;
		CMTime m_BlendInStartTime;
		CMTime m_BlendInEndTime;
		CMTime m_BlendOutStartTime;
		CMTime m_BlendOutEndTime;

		fp32 m_AnimTime;

		fp32 m_AnimLoopDuration;
		fp32 m_TimeScale;

		fp32 m_Enter_AnimBlendDuration;
		fp32 m_Enter_AnimBlendDelay;
		fp32 m_Enter_AnimTimeOffset;

		fp32 m_InvBlendInDuration;


		fp32 m_Leave_AnimBlendDuration;
		fp32 m_Leave_AnimBlendDelay;

		fp32 m_InvBlendOutDuration;

		fp32 m_ProceedCondition_Constant;

		// ... (testing)
		// Syncpoints from anim (so we don't need to dig them out each time...)
		// Add one for beginning and end? Or handle that separate?... (sep for now..)
		CXR_Anim_SyncPoints m_SyncPoints1;
		CXR_Anim_SyncPoints m_SyncPoints2;

		// For each syncpoint have a timescale for that slice 
		// (slice is prev sync point to next one)
		TThinArray<fp32> m_lTimeScales1;
		TThinArray<fp32> m_lTimeScales2;
		// AnimTimes for each syncpoint for current syncscale
		TThinArray<fp32> m_lTimes;
		TThinArray<int16> m_liLastEventKey;
		TThinArray<int16> m_lNumLoops;
		// Scale of animation (0 = anim1, 1 = anim2) (in evalutator...)
		fp32 m_SyncAnimScale;
		fp32 m_AnchorTime;
		fp32 m_Duration1;
		fp32 m_Duration2;
		

		int16 m_iState;
		//int16 m_iEnterAction;
		int16 m_iEnterMoveToken;
		//int16 m_iLeaveAction;
		int16 m_iLeaveMoveToken;

		uint8 m_Flags;
		uint8 m_Priority;
		int8 m_iLoopControlAnimLayer;
		uint8 m_ProceedCondition_iProperty;
		uint8 m_ProceedCondition_PropertyType;
		uint8 m_ProceedCondition_iOperator;
		int8 m_iAnimGraph;
		uint8 m_DirtyFlag;

		uint8 m_LeaveStateFrom;

		uint16	m_bHasAnimation:1;
		uint16	m_ProceedCondition_bDefault:1;
		uint16	m_bSkipForceKeep:1;
		uint16	m_bGotLeaveState:1;
		uint16	m_bGotLeaveStateInit:1;
		uint16	m_bGotEnterStateInit:1;
		uint16	m_bGotTerminate:1;
		uint16	m_bHasSyncAnim:1;
		uint16	m_bHasPerfectMovement:1;
		uint16	m_bNoPrevExactMove:1;
		uint16	m_bShouldDisableRefresh:1;
		uint16	m_bHasAdaptiveTimeScale:1;
		uint16	m_bDontBlendOut:1;

		friend class CWAG2I_SIQ;
		friend class CWAG2I_Token;

	public:

		CWAG2I_StateInstance() { Clear(); }
		CWAG2I_StateInstance(const CWAG2I_SIID* _pSIID, CWAG2I* _pAG2I, CWAG2I_Token* _pSIQ);
		void Clear();
		void operator = (const CWAG2I_StateInstance& _StateInst);
		M_AGINLINE void SetAG2I(CWAG2I* _pAG2I) { m_pAG2I = _pAG2I; }
		M_AGINLINE CWAG2I* GetAG2I() const { return m_pAG2I; }

		//M_AGINLINE void SetToken(CWAG2I_Token* _pToken) { m_pToken = _pToken; }
		//M_AGINLINE const CWAG2I_Token* GetToken() const { return m_pToken; }
		void Write(CCFile* _pFile) const;
		void Read(CCFile* _pFile);

		void ResetPMFlags() { m_Flags = 0; }
		bool IsPMAdded() const { return ((m_Flags & STATEINSTANCEFLAGS_PREDMISS_ADD) != 0); }
		bool IsPMRemoved() const { return ((m_Flags & STATEINSTANCEFLAGS_PREDMISS_REMOVE) != 0); }
		bool IsPMUpdated() const { return ((m_Flags & STATEINSTANCEFLAGS_PREDMISS_UPDATE) != 0); }

		int16 GetStateIndex() const { return m_iState; }

		CMTime GetEnqueueTime() const { return m_EnqueueTime; }
		//int16 GetEnterActionIndex() const { return m_iEnterAction; }
		int16 GetEnterMoveTokenIndex() const { return m_iEnterMoveToken; }
		CMTime GetEnterTime() const { return m_EnterTime; }
		//int16 GetLeaveActionIndex() const { return m_iLeaveAction; }
		int16 GetLeaveMoveTokenIndex() const { return m_iLeaveMoveToken; }
		CMTime GetLeaveTime() const { return m_LeaveTime; }
		M_AGINLINE fp32 GetAnimLoopDurationE() const { return m_AnimLoopDuration; }

		//fp32 GetAnimTime(CWAG2I_Context* _pContext, int8 _iAnimLayer) const;
		fp32 GetAnimLoopDuration(const class CWAG2I_Context* _pContext);
		fp32 GetLoopTimeScale(const CWAG2I_Context* _pContext) const;

		fp32 GetEnterAnimBlendDuration_Cached() const { return m_Enter_AnimBlendDuration; }
		fp32 GetEnterAnimTimeOffset_Cached() const { return m_Enter_AnimTimeOffset; }
		fp32 GetAnimLoopDuration_Cached() const { return m_AnimLoopDuration; }
		fp32 GetTimeScale_Cached() const { return m_TimeScale; }
		int8 GetLoopControlAnimLayerIndex_Cached() const { return m_iLoopControlAnimLayer; }
		int8 GetAnimGraphIndex() const { return m_iAnimGraph; }
		bool HasAnimation() const { return m_bHasAnimation; }
		bool HasSpecificAnimation(int32 _iAnim) const;

		bool ProceedQuery(const CWAG2I_Context* _pContext, fp32& _TimeFraction) const;

		//fp32 GetAnimTimeOffset(CWAG2I_Context* _pContext, int8 _iAnimTimeOffsetType);
		//void AdjustAnimTimeOffset();
		void FindBreakoutSequence(const CWAG2I_Context* _pContext, int16& _iSequence, CMTime& _AnimTime) const;

		// Sync to end velocity
		void EnterState_InitSyncVelocity(const CWAG2I_Context* _pContext, bool _bStart = false);

		// Synced animations
		bool EnterState_InitSyncAnims(const CWAG2I_Context* _pContext);
		void UpdateSyncAnimScale(const CWAG2I_Context* _pContext, fp32 _SyncAnimScale);
		void GetSyncAnimTime(const CWAG2I_Context* _pContext, int32 _iSlot, fp32& _Time, fp32& _TimeScale) const;

		// Have adaptive timescales
		bool EnterState_AdaptiveTimeScale(const CWAG2I_Context* _pContext);
		void UpdateAdaptiveTimeScale(const CWAG2I_Context* _pContext, fp32 _SyncAnimScale);
		void GetAdaptiveTime(const CWAG2I_Context* _pContext, int32 _iSlot, fp32& _Time, fp32& _TimeScale) const;



		const fp32& GetEnterAnimTimeOffset() const { return m_Enter_AnimTimeOffset; }

		bool EnterState_Setup(const CWAG2I_Context* _pContext, int16 _iEnterMoveToken);
		//bool EnterStateFromGraphBlock_Setup(const CWAG2I_Context* _pContext, int16 _iEnterMoveToken);
		void EnterState_Initiate(const CWAG2I_Context* _pContext);
		bool LeaveState_Setup(const CWAG2I_Context* _pContext, int16 _iLeaveMoveToken);
		void LeaveState_Initiate(const CWAG2I_Context* _pContext);

		void PredictionMiss_Add(const CWAG2I_Context* _pContext, const CWAG2I_SIP* _pSIP);
		void PredictionMiss_Remove(const CWAG2I_Context* _pContext);
		void SyncWithSIP(const CWAG2I_Context* _pContext, const CWAG2I_SIP* _pSIP);

		void PackSIP(CWAG2I_SIP* _pSIP) const;
		void UnpackSIP(const CWAG2I_Context* _pContext, const CWAG2I_SIP* _pSIP);

		bool OnCreateClientUpdate(uint8*& _pData, const CWAG2I_StateInstance* _pStateInstMirror) const;
		void OnClientUpdate(CWAG2I_Context* _pContext, CWAG2I_SIID _SIID, const uint8*& _pData);
		void UpdateFromMirror(CWAG2I_Context* _pContext, CWAG2I_StateInstance* _pStateInst);

		// Similar to enterstate_setup
		void CreateStateFromUpdate(const uint8*& _pData);

		void GetAnimLayers(const CWAG2I_Context* _pContext, bool _bBlendOut, CXR_AnimLayer* _pLayers, int& _nLayers, bool _ForceKeepLayers = false) const;
		void GetAnimLayerSeqs(const CWAG2I_Context* _pContext, CXR_AnimLayer* _pLayers, int& _nLayers) const;
		void GetEventLayers(const CWAG2I_Context* _pContext, CEventLayer* _pLayers, int& _nLayers);
		void GetValueCompareLayers(const CWAG2I_Context* _pContext, CXR_AnimLayer* _pLayers, int& _nLayers, int32 _Value) const;
		bool GetAnimLayerFirst(const CWAG2I_Context* _pContext, CXR_AnimLayer& _Layer, fp32& _AdjustmentOffset) const;
		bool GetSpecificAnimLayer(const CWAG2I_Context* _pContext, CXR_AnimLayer& _Layer, int _iSI, int32 _iAnim, int32 _StartTick) const;

};

//--------------------------------------------------------------------------------

#endif /* WAG2I_StateInst_h */
