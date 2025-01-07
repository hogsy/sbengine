#ifndef WAG2I_h
#define WAG2I_h

//--------------------------------------------------------------------------------

#include "WAG2I_Context.h"
#include "WAG2I_Token.h"

#include "../../../XR/XRAnimGraph2/AnimGraph2Defs.h"
#include "../../../XR/XRAnimGraph2/AnimGraph2.h"
#include "../WDataRes_AnimGraph2.h"
//#include "../WAnimGraph2Instance/WAG2_ClientData.h"
#include "../WAnimGraph2Instance/WAG2I_Defs.h"
#include "../WAnimGraph2Instance/WAG2I_StateInstPacked.h"
#include "../WAnimGraph2Instance/WAG2I_SIID.h"

#include "../WPhysState.h"
#include "../WObjCore.h"

#ifdef PLATFORM_PS2
# define WAG2I_CLIENTUPDATE_DIRECTACCESS
#endif

// Direct access activated for now
//#if !defined(PLATFORM_XBOX) || (defined(PLATFORM_XBOX) && !defined(M_Profile))
//#if defined(PLATFORM_XBOX)
#define WAG2I_CLIENTUPDATE_NORMAL 0
#define WAG2I_CLIENTUPDATE_DIRECTACCESS 1
//#endif

//--------------------------------------------------------------------------------
template <class T, int S>
class TAG2StaticArray
{
public:
	T m_aData[S];
	int32 m_Used;

	TAG2StaticArray()
	{
		m_Used = 0;
	}
	
	int Len() const { return m_Used; }
	void SetLen(int _Len)
	{
		M_ASSERT(_Len<S, "TAG2StaticArray: Damn it!!!!!!");
		m_Used = _Len;
	}

	int Add(const T &_Element)
	{
		M_ASSERT(m_Used<S, "TAG2StaticArray: Damn it");
		m_aData[m_Used++] = _Element;
		return m_Used-1;
	}

	void Del(int _Index)
	{
		//m_aData[_Index] = m_aData[--m_Used];
		M_ASSERT(_Index >= 0 && _Index < m_Used, "TAGStaticArray: Del");

		for(int32 i = _Index; i < m_Used-1; i++)
			m_aData[i] = m_aData[i+1];
		m_Used--;
	}

	const T *GetBasePtr() const { return m_aData; }
	T *GetBasePtr() { return m_aData; }

	const T &operator[](const int _Index) const { return m_aData[_Index]; }
	T &operator[](const int _Index) { return m_aData[_Index]; }
};

//--------------------------------------------------------------------------------
class CAG2Res
{
public:
	int32 m_NameHash;
	int32 m_iResource;
	CAG2Res(int32 _iRes, int32 _NameHash)
	{
		m_NameHash = _NameHash;
		m_iResource = _iRes;
	}
	CAG2Res() {m_NameHash = 0; m_iResource = -1;}

	M_INLINE void operator=(const CAG2Res& _AGRes)
	{
		CopyFrom(_AGRes);
	}

	bool operator== (const CAG2Res& _AGRes) const { return m_NameHash == _AGRes.m_NameHash && m_iResource == _AGRes.m_iResource; }
	bool operator!= (const CAG2Res& _AGRes) const { return m_NameHash != _AGRes.m_NameHash || m_iResource != _AGRes.m_iResource; }

	M_INLINE void CopyFrom(const CAG2Res& _AGRes)
	{
		m_NameHash = _AGRes.m_NameHash;
		m_iResource = _AGRes.m_iResource;
	}

	M_INLINE void OnCreateClientUpdate(uint8*& _pData) const
	{
		PTR_PUTINT32(_pData,m_NameHash);
		PTR_PUTINT32(_pData,m_iResource);
	}

	M_INLINE void OnClientUpdate(const uint8*& _pData)
	{
		PTR_GETINT32(_pData,m_NameHash);
		PTR_GETINT32(_pData,m_iResource);
	}
};

//--------------------------------------------------------------------------------
class CWAG2I : public CReferenceCount
{
	protected:
		enum
		{
			AG2I_DIRTYFLAG_RESOURCE			= 1 << 0,
			AG2I_DIRTYFLAG_TOKENS			= 1 << 1,
			AG2I_DIRTYFLAG_TOKENS_REMOVED	= 1 << 2,
			AG2I_DIRTYFLAG_OVERLAY			= 1 << 3,
			AG2I_DIRTYFLAG_OVERLAY_LIPSYNC	= 1 << 4,
			AG2I_DIRTYFLAG_PACKTYPE			= 1 << 5,

		};
		uint32						m_iRandseed;

		TArray<CAG2Res>				m_lAnimGraph2Res;
		TArray<spCXRAG2>			m_lspAnimGraph2;

		TArray<CWAG2I_Token>		m_lTokens;

		CXRAG2_Animation				m_OverlayAnim;
		// Lipsync overlay animation, assume it starts at the same time as the other one
		CXRAG2_Animation				m_OverlayAnimLipSync;
		CMTime						m_OverlayAnim_StartTime;
		int8						m_OverLayAnimLipSyncBaseJoint;
		int16						m_iOverlayKey;
		bool						m_bDisableAll;
		bool						m_bForceRefresh;
		
		// Dirtyflag for clientupdate
		uint8						m_DirtyFlag;
		int8						m_LastPackMode;
		bool						m_bNeedUpdate : 1;

		class CWO_ClientData_AnimGraph2Interface* m_pEvaluator;

		friend class CWAG2I_Token;
		friend class CWAG2I_StateInstance;
		friend class CWAG2I_SIQ;
		friend class CXRAG2_PropertyRecorder;

	public:

#ifndef M_RTM
		bool						m_bDisableDebug;
#endif

		CWAG2I() { Clear(); }
		CWAG2I(const CWAG2I& _Copy) { CopyFrom(&_Copy); }

		void SetEvaluator(CWO_ClientData_AnimGraph2Interface* _pEvaluator);
		const CWO_ClientData_AnimGraph2Interface* GetEvaluator() const { return m_pEvaluator; }
		CWO_ClientData_AnimGraph2Interface* GetEvaluator() { return m_pEvaluator; }

		void Log(CStr _Prefix, CStr _Name) const;

		uint32 GetRandseed() const { return m_iRandseed; }
		int8 GetLastPackMode() const { return m_LastPackMode; }
		void SetLastPackMode(int8 _AGPackMode) { m_LastPackMode = _AGPackMode; }

		bool GetNeedUpdate() const { return m_bNeedUpdate; }

		uint8 GetNumTokens() const { return m_lTokens.Len(); }
		const CWAG2I_Token* GetToken(uint8 _iToken) { return &(m_lTokens[_iToken]); }

		const CWAG2I_Token* GetTokenFromID(int8 _TokenID) const;
		CWAG2I_Token* GetTokenFromID(int8 _TokenID, bool _bCreateNonExistent = false);
		int32 GetTokenIndexFromID(int8 _TokenID);
		
		// Overloadable, but superclass functions must be called from within overloading functions.
		void Clear();
		void CopyFrom(const CWAG2I* _pAnimGraph2Instance);
		void Write(CCFile* _pFile) const;
		void Read(CWAG2I_Context* _pContext, CCFile* _pFile);

		void EndTimeLeap(CWAG2I_Context* _pContext);

		bool AcquireAllResources(const CWAG2I_Context* _pContext);
		bool AcquireAllResourcesToken(const CWAG2I_Context* _pContext);
		bool AcquireAllResourcesFromMapData(CMapData* _pMapData);

		// Load animations from graphblocks matched against impulses
		void TagAnimSetFromImpulses(const CWAG2I_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const TArray<CXRAG2_Impulse>& _lImpulses);
		void TagAnimSetFromBlockReaction(const CWAG2I_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const CXRAG2_Impulse& _BlockImpulse, const TArray<CXRAG2_Impulse>& _BlockReactions, bool _bEvenNoPreCache = false);
		// Goes through all blocks matching block impulses, matching reaction impulse target switch with given propertyvals
		void TagAnimSetFromBlockReactionSwitchState(const CWAG2I_Context* _pContext, CMapData* _pMapData, CWorldData* _pWData, const TArray<CXRAG2_Impulse>& _lBlockImpulses, const TArray<CXRAG2_Impulse>& _lReactionImpulses,const TArray<int32>& _lActionVals);


		void UpdateImpulseState(const CWAG2I_Context* _pContext);

		// Resources
		void AddResourceIndex_AnimGraph2(int32 _iAnimGraph2Res, const CStr& _AGName);
		void SetResourceIndex_AnimGraph2(int32 _iAnimGraph2Res, const CStr& _AGName, int32 _iSlot);
		void ClearSessionInfo(int32 _iSlot);
		int32 GetNumResource_AnimGraph2() const { return m_lAnimGraph2Res.Len(); }
		int32 GetResourceIndex_AnimGraph2(CAG2AnimGraphID _iAnimGraph) const { return m_lAnimGraph2Res[_iAnimGraph].m_iResource; };
		void SetOverlayAnim(int32 _iAnimContainerResource, int16 _iAnimSeq, CMTime _StartTime, uint8 _Flags = 0);
		void SetOverlayAnimLipSync(int32 _iAnimContainerResource, int16 _iAnimSeq, int8 _BaseJoint);
		void ClearOverlayAnim();
		bool HasOverlayAnim() { return m_OverlayAnim.IsValid(); }
		void DisableAll() { ClearTokens(); m_bDisableAll = true; }
		bool GetDisableAll() { return m_bDisableAll; }
		void ClearTokens() { m_lTokens.Clear(); }
		void ClearExtraTokens() { if (m_lTokens.Len()) m_lTokens.SetLen(1); }

		uint32 Refresh(const CWAG2I_Context* _pContext);
		// Only refresh specific token
		void RefreshToken(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, CAG2AnimGraphID _iAG);
		void RefreshPredictionMisses(const CWAG2I_Context* _pContext);
		CAG2StateIndex GetAnimLayers(const CWAG2I_Context* _pContext, CXR_AnimLayer* _pLayers, int& _nLayers, int _iDisableStateInstanceAnimsCallbackMsg);
		void GetValueCompareLayers(const CWAG2I_Context* _pContext, CXR_AnimLayer* _pLayers, int& _nLayers, int32 _Value);
		void GetTopEventLayers(const CWAG2I_Context* _pContext, CEventLayer* _pLayers, CAG2TokenID* _pTokenIDs, int& _nLayers);
		void GetEventLayers(const CWAG2I_Context* _pContext, CEventLayer* _pLayers, CAG2TokenID* _pTokenIDs, int& _nLayers);
		void GetAllAnimLayers(const CWAG2I_Context* _pContext, CXR_AnimLayer* _pLayers, CAG2TokenID* _pTokenIDs, int& _nLayers, int _iDisableStateInstanceAnimsCallbackMsg, bool _bOnlySeqs = false);
		void GetAnimLayersFromToken(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, CXR_AnimLayer* _pLayers, int& _nLayers, int _iDisableStateInstanceAnimsCallbackMsg);
		bool GetAnimLayerFromState(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, CXR_AnimLayer& _Layer, CAG2StateIndex _iState);
		void GetTopEventLayersFromToken(const CWAG2I_Context* _pContext, CAG2TokenID _iToken, CEventLayer* _pLayers, int& _nLayers);
		int32 GetAnimVelocity(const CWAG2I_Context* _pContext, CVec3Dfp32& _MoveVelocity, CQuatfp32& _RotVelocity, int _iDisableStateInstanceAnimsCallbackMsg);
		bool GetAnimVelocityToDestination(const CWAG2I_Context* _pContext, CVec3Dfp32& _MoveVelocity, CQuatfp32& _RotVelocity, CAG2StateIndex _iState);
		bool GetAnimVelocityFromDestination(const CWAG2I_Context* _pContext, CVec3Dfp32& _MoveVelocity, CQuatfp32& _RotVelocity, CAG2StateIndex _iState);
		void GetRotVelocityToDest(const CWAG2I_Context* _pContext, CVec3Dfp32& _MoveVelocity, CAxisRotfp32& _RotVel, CAG2StateIndex _iState, fp32 _RotAngleTotal);
		bool GetSpecificAnimLayer(const CWAG2I_Context* _pContext, CXR_AnimLayer& _pLayer, int32 _iToken, int32 _iAnim, int32 _StartTick) const;
		// If we only need the rotational velocity
		bool GetTopLayerTotalAnimOffset(const CWAG2I_Context* _pContext, CVec3Dfp32& _MoveOffset, CQuatfp32& _RotOffset, CAG2TokenID _iToken);
		//bool GetAnimOffset(const CWAG2I_Context* _pContext, CVec3Dfp32& _MoveOffset, CQuatfp32& _RotOffset);
		//bool GetAnimRelOffset(const CWAG2I_Context* _pContext, CVec3Dfp32 _MoveOffset, const CQuatfp32& _RotOffset, CVec3Dfp32& _MoveRelOffset, CQuatfp32& _RotRelOffset);
		bool GetAnimRotVelocity(const CWAG2I_Context* _pContext, CQuatfp32& _RotVelocity, int _iDisableStateInstanceAnimsCallbackMsg);
		bool HasAnimVelocity(const CWAG2I_Context* _pContext, int _iDisableStateInstanceAnimsCallbackMsg);
		void CheckAnimEvents(const CWAG2I_Context* _pContext, int _iCallbackMessage, uint32 _ScanMask = ~0, bool _bOnlyTop = true);
		void CheckTokenAnimEvents(const CWAG2I_Context* _pContext, int _iCallbackMessage, CAG2TokenID _iToken, uint32 _ScanMask = ~0);
		fp32 GetCurrentLoopDuration(const CWAG2I_Context* _pContext, CAG2TokenID _iToken = 0);		
		// 
		void FindBreakoutPoints(const CWAG2I_Context* _pContext, int16 _iState, CAG2AnimGraphID _iAnimGraph, CXR_Anim_BreakoutPoints& _Points, fp32 _Offset) const;
		void FindEntryPoints(const CWAG2I_Context* _pContext, int16 _iState, CAG2AnimGraphID _iAnimGraph, CXR_Anim_EntryPoints& _Points) const;
		//
		bool FindEntryPoint(const CWAG2I_Context* _pContext, int16 _iState, CAG2AnimGraphID _iAnimGraph, int16 _iPrevSequence, CMTime _PrevTime, fp32& _Offset) const;

		// Find absolute distance for a certain offset in an animation, animation is found through given moveaction
		// No the most beautiful or useful functions, but they work
		bool GetAbsoluteAnimOffset(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAnimGraph, int32 _iAnim, fp32 _TimeOffset, CVec3Dfp32& _MoveOffset, CQuatfp32& _RotOffset) const;
		CAG2AnimIndex GetAnimFromAction(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAnimGraph, CStr& _MoveAction, fp32& _TimeOffset) const;
		CAG2AnimIndex GetAnimFromReaction(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAnimGraph, CXRAG2_Impulse _Impulse, int8 _iToken, fp32& _TimeOffset) const;
		bool GetAnimFromFirstActionInState(const CWAG2I_Context* _pContext, CAG2AnimIndex _iAnim, int32 _iToken, CAG2AnimIndex& _iTargetAnim, CAG2ActionIndex& _iTargetAction) const;

		// Finds a matching graphblock for current token and find constant
		bool GetConstantFromGraphBlock(CXRAG2_Impulse _Impulse, CAG2TokenID _Token, CAG2StateConstantID _iConstant, fp32& _Constant);

		// Send impulse from gameplay code, the animgraph will try to find a matching reaction
		// and move state or graphblock accordingly. Ex send response-hurt and the ag will try
		// to find a hurt response reaction
		bool SendImpulse(const CWAG2I_Context* _pContext, const CXRAG2_Impulse& _Impulse);
		bool SendImpulse(const CWAG2I_Context* _pContext, const CXRAG2_Impulse& _Impulse, int8 _iToken, bool _bForce = false);

		// Evaluate incoming movetoken (which should be to a switchstate) and set the movetoken
		// index to be the result of the evaluation
		bool EvaluateSwitchState(const CWAG2I_Context* _pContext, CAG2MoveTokenIndex& _iMoveToken, CAG2AnimGraphID _iAnimGraph);

		CAG2GraphBlockIndex GraphBlockExists(const CXRAG2_Impulse& _Impulse, CAG2AnimGraphID _iAnimGraph = 0);

		void PredictionMiss_AddToken(CWAG2I_Context* _pContext, int8 _TokenID);
		void PredictionMiss_RemoveToken(CWAG2I_Context* _pContext, int8 _TokenID);

		int OnCreateClientUpdate2(uint8* _pData, const CWAG2I* _pMirror, int8 _AGPackType) const;
		static int OnClientUpdate2(CWAG2I_Context* _pContext, CWAG2I* _pAG2IMirror, CWAG2I* _pAG2I, const uint8* _pData);
		// Update the animgraph from a server mirror (that only plays current animation...)
		void UpdateFromMirror(CWAG2I_Context* _pContext, CWAG2I* _pAG2IMirror);

		void RemoveToken(int32 _iToken);
		void RemoveTokenByID(CAG2TokenID _iToken);

	public:

		uint32 GetTokenStateFlags(const CWAG2I_Context *_pContext, int8 _TokenID, uint8 _iFlags) const;
		uint32 GetStateFlags(const CXRAG2_State* _pState, uint8 _iFlags) const;
		bool GetTokenStateConstantValue(const CWAG2I_Context *_pContext, int8 _TokenID, CAG2AnimGraphID _iAnimGraph, uint16 _StateConstantID, fp32& _Value) const;
		fp32 GetTokenStateConstantValueDef(const CWAG2I_Context *_pContext, int8 _TokenID, CAG2AnimGraphID _iAnimGraph, uint16 _StateConstantID, fp32 _DefaultValue) const;
		bool GetStateConstantValue(CAG2StateIndex _iState, uint16 _StateConstantID, CAG2AnimGraphID _iAnimGraph, fp32& _Value) const;
		fp32 GetStateConstantValueDef(CAG2StateIndex _iState, uint16 _StateConstantID, CAG2AnimGraphID _iAnimGraph, fp32 _DefaultValue) const;
		bool GetStateConstantValue(const CXRAG2_State* _pState, uint16 _StateConstantID, CAG2AnimGraphID _iAnimGraph, fp32& _Value) const;
		fp32 GetStateConstantValueDef(const CXRAG2_State* _pState, uint16 _StateConstantID, CAG2AnimGraphID _iAnimGraph, fp32 _DefaultValue) const;

		void ForceRefresh() { m_bForceRefresh = true; }
		bool RestartAG(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAG = 0);

	public:

static	int PackDiffSIPs(TArray<CWAG2I_SIP>& _lDiffSIPs, uint8* _pData);
static	int UnpackDiffSIPs(TArray<CWAG2I_SIP>& _lDiffSIPs, const uint8* _pData);
		//void UpdateStateInstance(CWAG2I_StateInstance_Packed* _pSIP);

		bool EvaluateCondition(const CWAG2I_Context* _pContext, const CXRAG2_ConditionNodeV2* _pNode, fp32& _TimeFraction);
		void InvokeEffects(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAnimGraph, int16 _iBaseEffectInstance, uint8 _nEffects);

		void CheckAnimEvents(const CWAG2I_Context* _pContext, CEventLayer* _pLayers, CAG2TokenID* _pTokenIDs, int32 _nLayers, int _iCallbackMessage, uint32 _ScanMask) const;
		void CheckAnimEvents(const CWAG2I_Context* _pContext, CXR_Anim_SequenceData* _pAnimSeqData, CMTime _StartTime, CMTime _EndTime, fp32 _LayerTime, CAG2TokenID _iToken, int _iCallbackMessage, uint32 _ScanMask, int16& _iKey) const;

	public:

		// FIXME: Provide functionality of a 'PerformAction' intead.
		void DoActionEffects(const CWAG2I_Context* _pContext, int16 _iAction, CAG2AnimGraphID _iAnimGraph);
		void MoveAction(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAnimGraph, int8 _ActionTokenID, int16 _iMoveToken, fp32 _ForceOffset = 0.0f, int32 _MaxQueued = 0);
		void MoveGraphBlock(const CWAG2I_Context* _pContext, CAG2AnimGraphID _iAnimGraph, int8 _ActionTokenID, int16 _iMoveToken, fp32 _ForceOffset = 0.0f, int32 _MaxQueued = 0);

		// FIXME: Abstract action indices completely. Force usage of HashKeys or Names instead.
		int16 GetActionIndexFromHashKey(uint32 _ActionHashKey, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetActionIndexFromHashKey(_ActionHashKey); }

		void ClearAnimListCache();

	public:

		const CXRAG2_ConditionNodeV2* GetNode(int16 _iNode, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetNode(_iNode); }
		const CXRAG2_StateConstant* GetStateConstant(int16 _iStateConstant, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetStateConstant(_iStateConstant); }
		const CXRAG2_Action* GetAction(int16 _iAction, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetAction(_iAction); }
		const CXRAG2_Reaction* GetReaction(int16 _iReaction, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetReaction(_iReaction); }
		const CXRAG2_GraphBlock* GetGraphBlock(int16 _iGraphBlock, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetGraphBlock(_iGraphBlock); }
		const CXRAG2_MoveToken* GetMoveToken(int16 _iMoveToken, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetMoveToken(_iMoveToken); }
		const CXRAG2_State* GetState(int16 _iState, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetState(_iState); }
		const CXRAG2_SwitchState* GetSwitchState(int16 _iSwitchState, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetSwitchState(_iSwitchState); }
		const CXRAG2_SwitchStateActionVal* GetSwitchStateActionVal(int16 _iSwitchStateActionVal, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetSwitchStateActionVal(_iSwitchStateActionVal); }
		const CXRAG2_AnimLayer* GetAnimLayer(int16 _iAnimLayer, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetAnimLayer(_iAnimLayer); }

		void GetGraphBlockConstants(CXRAG2_Impulse _Impulse, TArray<CXRAG2_StateConstant>& _lConstants, CAG2AnimGraphID _iAnimGraph = 0) const 
		{
			if (m_lspAnimGraph2.ValidPos(_iAnimGraph))
				m_lspAnimGraph2[_iAnimGraph]->GetGraphBlockConstants(_Impulse, _lConstants);
			else
				M_ASSERT(0,"AnimgraphIndex out of range"); 
		}

		CAG2AnimGraphID GetAnimGraphIDFromNameHash(int32 _Hash) const;

		const CXRAG2* GetAnimGraph(CAG2AnimGraphID _iAnimGraph) const
		{
			M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range");
			M_ASSERT(m_lspAnimGraph2[_iAnimGraph],"Animgraph Resource not set");
			return ((const CXRAG2*)m_lspAnimGraph2[_iAnimGraph]);
		}
		CXRAG2* GetAnimGraph(CAG2AnimGraphID _iAnimGraph)
		{
			M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range");
			M_ASSERT(m_lspAnimGraph2[_iAnimGraph],"Animgraph Resource not set");

			return ((CXRAG2*)m_lspAnimGraph2[_iAnimGraph]);
		}
#ifndef M_RTM
		const CStr GetExportedActionName(int _iExportedActionName, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetExportedActionName(_iExportedActionName); }
		const CStr GetExportedGraphBlockName(int _iBlock, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetExportedGraphBlockName(_iBlock); }
		const CStr GetStateName(int _iState, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetExportedStateName(_iState); }
		const CStr GetSwitchStateName(int _iState, CAG2AnimGraphID _iAnimGraph) const { M_ASSERT(m_lspAnimGraph2.ValidPos(_iAnimGraph),"AnimgraphIndex out of range"); return m_lspAnimGraph2[_iAnimGraph]->GetExportedSwitchStateName(_iState); }
#endif

	public:

#ifdef M_RTM
		M_INLINE static bool DebugEnabled(const CWAG2I_Context* _pContext) {return false;}
#else
		static bool DebugEnabled(const CWAG2I_Context* _pContext)

		{
			if(_pContext->m_pWPhysState && _pContext->m_pObj)
			{
				uint32 AG2IDebugFlags = 0;

				MACRO_GetRegisterObject(CSystem, pSys, "SYSTEM");
				CRegistry* pReg = pSys ? pSys->GetEnvironment() : NULL;
				if (pReg != NULL)
					AG2IDebugFlags = pReg->GetValuei("AG2I_DEBUG_FLAGS");

				bool bDebugServer = (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_SERVER) != 0;
				bool bDebugClient = (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_CLIENT) != 0;
				bool bDebugPlayer = (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_PLAYER) != 0;
				bool bDebugNonPlayer = (AG2IDebugFlags & AG2I_DEBUGFLAGS_ENTERSTATE_NONPLAYERS) != 0;

				bool bIsPlayer = ((_pContext->m_pObj->GetPhysState().m_ObjectFlags & OBJECT_FLAGS_PLAYER) != 0);
				if (((_pContext->m_pWPhysState->IsServer() && bDebugServer) ||
					(_pContext->m_pWPhysState->IsClient() && bDebugClient)) &&
					((bIsPlayer && bDebugPlayer) ||	(!bIsPlayer && bDebugNonPlayer)))
					return true;
			}

			return false;
		}

		//Get debug info about current state for all tokens
		CStr DebugStateInfo(const CWAG2I_Context* _pContext);
#endif
};
typedef TPtr<CWAG2I> spCWAG2I;

// Contains 1 or more wagis
class CWAG2I_Mirror : public CReferenceCount
{
public:
	TThinArray<spCWAG2I> m_lspWAG2I;
	CWAG2I_Mirror(int32 _NumMirrors)
	{
		m_lspWAG2I.SetLen(_NumMirrors);
		for (int32 i = 0; i < _NumMirrors; i++)
		{
			m_lspWAG2I[i] = MNew(CWAG2I);
		}
	}

	M_INLINE CWAG2I* GetWAG2I(int32 _iWAG2I)
	{
		if (_iWAG2I >= m_lspWAG2I.Len())
			return NULL;

		return m_lspWAG2I[_iWAG2I];
	}

	M_INLINE const CWAG2I* GetWAG2I(int32 _iWAG2I) const
	{
		if (_iWAG2I >= m_lspWAG2I.Len())
			return NULL;

		return m_lspWAG2I[_iWAG2I];
	}
};

typedef TPtr<CWAG2I_Mirror> spCWAG2I_Mirror;

//--------------------------------------------------------------------------------

#endif /* WAG2I_h */
