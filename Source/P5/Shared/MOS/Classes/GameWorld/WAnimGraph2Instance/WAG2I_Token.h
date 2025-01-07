#ifndef WAG2I_Token_h
#define WAG2I_Token_h

//--------------------------------------------------------------------------------

#include "WAG2I_StateInst.h"
#include "WAG2I_StateInstPacked.h"
#include "WAG2I_SIID.h"

class CWAG2I;
class CWAG2I_Context;

//--------------------------------------------------------------------------------

#define AG2I_STATEINSTANCEINDEX_NULL ((uint8)(-1))

//--------------------------------------------------------------------------------

#define TOKENFLAGS_PREDMISS_ADD		0x01
#define TOKENFLAGS_PREDMISS_REMOVE	0x02
#define TOKENFLAGS_NOREFRESH		0x04

//--------------------------------------------------------------------------------

class CWAG2I_Token : public CObj
{
	MRTC_DECLARE;
protected:
	friend class CWAG2I;
	friend class CXRAG2_PropertyRecorder;
	enum
	{
		AG2I_TOKEN_DIRTYFLAG_STATEINSTANCE			= 1 << 0,
		AG2I_TOKEN_DIRTYFLAG_GRAPHBLOCK				= 1 << 1,
		AG2I_TOKEN_DIRTYFLAG_PLAYINGSIID			= 1 << 2,
	};

	TArray<CWAG2I_StateInstance>	m_lStateInstances;
	CMTime							m_RefreshGameTime;
	CWAG2I*							m_pAG2I;
	CWAG2I_SIID						m_PlayingSIID;
	CMTime							m_PreviousPlayingSequenceTime;
	int16							m_iPreviousPlayingSequence;
	CAG2GraphBlockIndex				m_iGraphBlock;
	CAG2NodeIndex					m_CachedBaseNodeIndex;
	CAG2AnimGraphID					m_CachedAnimGraphIndex;
	uint8							m_Flags;
	int8							m_ID;

	// Dirtyflag 
	uint8							m_DirtyFlag;

	void EvalNodeInit();
	uint16 EvalNode(const CWAG2I_Context* _pContext, const CXRAG2_ConditionNodeV2* _pNode, fp32& _TimeFraction) const;
	uint16 EvalNodeAction(const CWAG2I_Context* _pContext, uint16 _NodeAction, fp32& _TimeFraction);

public:

	CWAG2I_Token() { Clear(); }
	CWAG2I_Token(int8 _ID, CWAG2I* _pAG2I) { Clear(); m_ID = _ID; SetAG2I(_pAG2I); }

	void Clear();

	int8 GetID() const { return m_ID; }

	void CopyFrom(const CWAG2I_Token& _Token, CWAG2I* _pOurAG2I);
	void Write(CCFile* _pFile) const;
	void Read(CWAG2I_Context* _pContext, CCFile* _pFile);

	M_AGINLINE void SetAG2I(CWAG2I* _pAG2I) { m_pAG2I = _pAG2I; SetQueueAG2I(_pAG2I); }
	M_AGINLINE CWAG2I* GetAG2I() { return m_pAG2I; }

	void SetQueueAG2I(CWAG2I* _pAG2I)
	{
		for (int i = 0; i < m_lStateInstances.Len(); i++)
			m_lStateInstances[i].SetAG2I(_pAG2I);
	}

	M_AGINLINE void SetGraphBlock(CAG2GraphBlockIndex _iGraphBlock) { if (m_iGraphBlock != _iGraphBlock) m_iGraphBlock = _iGraphBlock; m_DirtyFlag |= AG2I_TOKEN_DIRTYFLAG_GRAPHBLOCK; }
	M_AGINLINE CAG2GraphBlockIndex GetGraphBlock() const { return m_iGraphBlock; }

	M_AGINLINE uint8 GetNumStateInstances() const { return m_lStateInstances.Len(); }
	M_AGINLINE const CWAG2I_StateInstance* GetStateInstance(uint8 _iSI) const { return &(m_lStateInstances[_iSI]); }

	const CWAG2I_StateInstance* GetTokenStateInstance() const;
	CWAG2I_StateInstance* GetTokenStateInstance();
	const CWAG2I_StateInstance* GetTokenStateInstanceUpdate() const;
	CWAG2I_StateInstance* GetTokenStateInstanceUpdate();

	int16 GetStateIndex() const;
	int8 GetAnimGraphIndex() const;
	CMTime GetEnterStateTime() const;
	//		bool IsFirstRefresh() const { return m_bFirstRefresh; }

	CMTime GetRefreshGameTime() const { return m_RefreshGameTime; }
	void SetRefreshGameTime(CMTime _RefreshGameTime) { m_RefreshGameTime = _RefreshGameTime; }

	void EnterState(const CWAG2I_Context* _pContext, int16 _iMoveToken, CAG2AnimGraphID _iAnimGraph, fp32 _ForceOffset = 0.0f);
	void LeaveState(const CWAG2I_Context* _pContext, int16 _iMoveToken, CAG2AnimGraphID _iAnimGraph);

	void EnterGraphBlock(const CWAG2I_Context* _pContext, int16 _iMoveToken, CAG2AnimGraphID _iAnimGraph, fp32 _ForceOffset = 0.0f);
	void LeaveGraphBlock(const CWAG2I_Context* _pContext, int16 _iMoveToken, CAG2AnimGraphID _iAnimGraph);

	void EndTimeLeap(CWAG2I_Context* _pContext);

	uint32 Refresh(CWAG2I_Context* _pContext);

	bool OnCreateClientUpdate(uint8*& _pData, const CWAG2I_Token* pTokenMirror) const;
	bool OnClientUpdate(CWAG2I_Context* _pContext, const uint8*& _pData);
	void UpdateFromMirror(CWAG2I_Context* _pContext, CWAG2I_Token* _pMirror);

#ifndef M_RTM
	static void SetDebugOut(bool _bVal);
	void DebugPrintMoveToken(const CWAG2I_Context* _pContext, int16 _iMoveToken, CAG2AnimGraphID _iAnimGraph);
#endif


	// Stateinstqueue
	void RefreshQueue(const CWAG2I_Context* _pContext);
	void RefreshPredictionMisses(const CWAG2I_Context* _pContext);
	bool OnEnterState(const CWAG2I_Context* _pContext, int16 _iEnterMoveToken, CAG2AnimGraphID _iAnimGraph, fp32 _ForceOffset = 0.0f);
	bool OnLeaveState(const CWAG2I_Context* _pContext, int16 _iLeaveMoveToken, CAG2AnimGraphID _iAnimGraph);

	bool OnEnterGraphBlock(const CWAG2I_Context* _pContext, int16 _iEnterMoveToken, CAG2AnimGraphID _iAnimGraph);
	bool OnLeaveGraphBlock(const CWAG2I_Context* _pContext, int16 _iLeaveMoveToken, CAG2AnimGraphID _iAnimGraph);

	bool DisableStateInstanceAnims(const CWAG2I_Context* _pContext, const CWAG2I_StateInstance* _pStateInstance, int _iDisableStateInstanceAnimsCallbackMsg) const;
	CAG2StateIndex GetAnimLayers(const CWAG2I_Context* _pContext, CXR_AnimLayer* _pLayers, int& _nLayers, int _iDisableStateInstanceAnimsCallbackMsg, bool _bAllowBlend) const;
	bool GetAnimLayerFromState(const CWAG2I_Context* _pContext, CXR_AnimLayer& _Layer, CAG2StateIndex _iState) const;
	void GetValueCompareLayers(const CWAG2I_Context* _pContext, CXR_AnimLayer* _pLayers, int& _nLayers, int32 _Value) const;
	void GetTopEventLayers(const CWAG2I_Context* _pContext, CEventLayer* _pLayers, int& _nLayers, bool _bAllowBlend);
	void GetEventLayers(const CWAG2I_Context* _pContext, CEventLayer* _pLayers, int& _nLayers, bool _bAllowBlend);
	void GetAllAnimLayers(const CWAG2I_Context* _pContext, CXR_AnimLayer* _pLayers, int& _nLayers, int _iDisableStateInstanceAnimsCallbackMsg, bool _bAllowBlend, bool _bOnlySeqs = false) const;
	bool GetSpecificAnimLayer(const CWAG2I_Context* _pContext, CXR_AnimLayer& _Layer, int32 _iAnim, int32 _StartTick) const;
	const CWAG2I_StateInstance* GetSpecificState(const CWAG2I_Context* _pContext, int32 _iAnim) const;

	void PredictionMiss_AddStateInstance(CWAG2I_Context* _pContext, const CWAG2I_SIP* _pSIP);
	void PredictionMiss_RemoveStateInstance(CWAG2I_Context* _pContext, const CWAG2I_SIID* _pSIID);
	void PredictionMiss_Remove(CWAG2I_Context* _pContext);

	void ForceMaxStateInstances(int32 _MaxQueued);

	void ResetPMFlags();
	bool IsPMAdded() { return ((m_Flags & TOKENFLAGS_PREDMISS_ADD) != 0); }
	bool IsPMRemoved() { return ((m_Flags & TOKENFLAGS_PREDMISS_REMOVE) != 0); }

	//		void RemoveMatchingStateInstance(const CWAG2I_SIID* _pRemSIID);
	CWAG2I_StateInstance* CreateAndEnqueueStateInstance(const CWAG2I_Context* _pContext, CWAG2I_SIID* _pSIID);
	CWAG2I_StateInstance* GetMatchingStateInstance(const CWAG2I_SIID* _pSIID, bool _bCreateNonExistent);

	bool RemoveStateInstance(const CWAG2I_SIID& _Siid);

	void RemoveStateInstance(int32 _iStateInstance);
	CWAG2I_SIID GetPlayingSIID() const { return m_PlayingSIID; }
	void SetPlayingSIID(const CWAG2I_SIID& _PlayingSIID) { m_PlayingSIID = _PlayingSIID; }
	uint8 ResolvePlayingSIID();
};

//--------------------------------------------------------------------------------

#endif /* WAG2I_Token_h */
