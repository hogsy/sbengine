#ifndef WAGI_StateInst_h
#define WAGI_StateInst_h

//--------------------------------------------------------------------------------

#include "../../../XR/XRAnimGraph/AnimGraphDefs.h"
#include "../../../XR/XRAnimGraph/AnimGraph.h"
//#include "WAGI.h"
//#include "WAGI_StateInstPacked.h"

class CWAGI;
class CWAGI_SIP;
class CWAGI_SIID;
class CWAGI_SIQ;

//--------------------------------------------------------------------------------

// Will only be set on client.
#define STATEINSTANCEFLAGS_PREDMISS_ADD			0x01
#define STATEINSTANCEFLAGS_PREDMISS_REMOVE		0x02
#define STATEINSTANCEFLAGS_PREDMISS_UPDATE		0x04

//--------------------------------------------------------------------------------

class CWAGI_StateInstance // <m_EnqueueTime, m_iEnterAction>
{
	private:

		CWAGI* m_pAGI;
		CWAGI_SIQ* m_pSIQ;

/*
		// 104 bytes of unaligned junk
		uint8 m_Flags;

		fp32 m_AnimTime;

		fp32 m_EnqueueTime;
		int16 m_iState;
		uint8 m_Priority;
		int8 m_iLoopControlAnimLayer;
		fp32 m_AnimLoopDuration;
		bool m_bHasAnimation;

		// Enter
		int16 m_iEnterAction;
		fp32 m_EnterTime;

		fp32 m_Enter_AnimBlendDuration;
		fp32 m_Enter_AnimBlendDelay;
		fp32 m_Enter_AnimTimeOffset;
		int8 m_Enter_iAnimTimeOffsetType;

		fp32 m_BlendInStartTime;
		fp32 m_BlendInEndTime;
		fp32 m_InvBlendInDuration;

		// Leave
		int16 m_iLeaveAction;
		fp32 m_LeaveTime;

		fp32 m_Leave_AnimBlendDuration;
		fp32 m_Leave_AnimBlendDelay;

		fp32 m_BlendOutStartTime;
		fp32 m_BlendOutEndTime;
		fp32 m_InvBlendOutDuration;

		// Proceed Condition
		bool m_ProceedCondition_bDefault;
		uint8 m_ProceedCondition_iProperty;
		uint8 m_ProceedCondition_iOperator;
		fp32 m_ProceedCondition_Constant;
*/
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

		int16 m_iState;
		int16 m_iEnterAction;
		int16 m_iLeaveAction;

		uint8 m_Flags;
		uint8 m_Priority;
		int8 m_iLoopControlAnimLayer;
		int8 m_Enter_iAnimTimeOffsetType;
		uint8 m_ProceedCondition_iProperty;
		uint8 m_ProceedCondition_iOperator;

		uint8	m_bHasAnimation:1;
		uint8	m_ProceedCondition_bDefault:1;
		uint8	m_bSkipForceKeep:1;

		friend class CWAGI_SIQ;

	public:

		CWAGI_StateInstance() {}
		CWAGI_StateInstance(const CWAGI_SIID* _pSIID, CWAGI* _pAGI, CWAGI_SIQ* _pSIQ);

		void SetAGI(CWAGI* _pAGI) { m_pAGI = _pAGI; }
		const CWAGI* GetAGI() const { return m_pAGI; }

		void SetSIQ(CWAGI_SIQ* _pSIQ) { m_pSIQ = _pSIQ; }
		const CWAGI_SIQ* GetSIQ() const { return m_pSIQ; }

		void ResetPMFlags() { m_Flags = 0; }
		bool IsPMAdded() const { return ((m_Flags & STATEINSTANCEFLAGS_PREDMISS_ADD) != 0); }
		bool IsPMRemoved() const { return ((m_Flags & STATEINSTANCEFLAGS_PREDMISS_REMOVE) != 0); }
		bool IsPMUpdated() const { return ((m_Flags & STATEINSTANCEFLAGS_PREDMISS_UPDATE) != 0); }

		int16 GetStateIndex() const { return m_iState; }

		CMTime GetEnqueueTime() const { return m_EnqueueTime; }
		int16 GetEnterActionIndex() const { return m_iEnterAction; }
		CMTime GetEnterTime() const { return m_EnterTime; }
		int16 GetLeaveActionIndex() const { return m_iLeaveAction; }
		CMTime GetLeaveTime() const { return m_LeaveTime; }

		//fp32 GetAnimTime(CWAGI_Context* _pContext, int8 _iAnimLayer) const;
		fp32 GetAnimLoopDuration(const class CWAGI_Context* _pContext);
		fp32 GetTimeScale(const CWAGI_Context* _pContext) const;

		fp32 GetEnterAnimBlendDuration_Cached() const { return m_Enter_AnimBlendDuration; }
		fp32 GetEnterAnimTimeOffset_Cached() const { return m_Enter_AnimTimeOffset; }
		fp32 GetAnimLoopDuration_Cached() const { return m_AnimLoopDuration; }
		fp32 GetTimeScale_Cached() const { return m_TimeScale; }
		int8 GetLoopControlAnimLayerIndex_Cached() const { return m_iLoopControlAnimLayer; }
		bool HasAnimation() const { return m_bHasAnimation; }
		bool HasSpecificAnimation(int32 _iAnim) const;

		bool ProceedQuery(const CWAGI_Context* _pContext, fp32& _TimeFraction) const;

		//fp32 GetAnimTimeOffset(CWAGI_Context* _pContext, int8 _iAnimTimeOffsetType);
		//void AdjustAnimTimeOffset();

		const fp32& GetEnterAnimTimeOffset() const { return m_Enter_AnimTimeOffset; }

		bool EnterState_Setup(const CWAGI_Context* _pContext, int16 _iEnterAction);
		void EnterState_Initiate(const CWAGI_Context* _pContext);
		bool LeaveState_Setup(const CWAGI_Context* _pContext, int16 _iLeaveAction);
		void LeaveState_Initiate(const CWAGI_Context* _pContext);

		void PredictionMiss_Add(const CWAGI_Context* _pContext, const CWAGI_SIP* _pSIP);
		void PredictionMiss_Remove(const CWAGI_Context* _pContext);
		void SyncWithSIP(const CWAGI_Context* _pContext, const CWAGI_SIP* _pSIP);

		void PackSIP(CWAGI_SIP* _pSIP) const;
		void UnpackSIP(const CWAGI_Context* _pContext, const CWAGI_SIP* _pSIP);

		void GetAnimLayers(const CWAGI_Context* _pContext, bool _bBlendOut, CXR_AnimLayer* _pLayers, int& _nLayers, int _iSI);
		bool GetSpecificAnimLayer(const CWAGI_Context* _pContext, CXR_AnimLayer& _Layer, int _iSI, int32 _iAnim, int32 _StartTick) const;

};

//--------------------------------------------------------------------------------

#endif /* WAGI_StateInst_h */
